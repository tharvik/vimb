/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2016 Daniel Carl
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 */

#include <JavaScriptCore/JavaScript.h>
#include <gio/gio.h>
#include <glib.h>
#include <libsoup/soup.h>
#include <webkit2/webkit-web-extension.h>

#include "ext-main.h"
#include "ext-dom.h"
#include "ext-util.h"

static gboolean on_authorize_authenticated_peer(GDBusAuthObserver *observer,
        GIOStream *stream, GCredentials *credentials, gpointer extension);
static void on_dbus_connection_created(GObject *source_object,
        GAsyncResult *result, gpointer data);
static void add_onload_event_observers(WebKitDOMDocument *doc,
        WebKitWebExtension *extension);
static void emit_page_created(GDBusConnection *connection, guint64 pageid);
static void emit_page_created_pending(GDBusConnection *connection);
static void queue_page_created_signal(guint64 pageid);
static void dbus_emit_signal(const char *name, WebKitWebExtension* extension,
        GVariant *data);
static void dbus_handle_method_call(GDBusConnection *conn, const char *sender,
        const char *object_path, const char *interface_name, const char *method,
        GVariant *parameters, GDBusMethodInvocation *invocation, gpointer data);
static void on_editable_change_focus(WebKitDOMEventTarget *target,
        WebKitDOMEvent *event, WebKitWebExtension *extension);
static void on_page_created(WebKitWebExtension *ext, WebKitWebPage *webpage, gpointer data);
static void on_web_page_document_loaded(WebKitWebPage *webpage, gpointer extension);
static gboolean on_web_page_send_request(WebKitWebPage *webpage, WebKitURIRequest *request,
        WebKitURIResponse *response, gpointer extension);

static const GDBusInterfaceVTable interface_vtable = {
    dbus_handle_method_call,
    NULL,
    NULL
};

static const char introspection_xml[] =
    "<node>"
    " <interface name='" VB_WEBEXTENSION_INTERFACE "'>"
    "  <signal name='EditableChangeFocus'>"
    "   <arg type='b' name='focused' direction='out'/>"
    "  </signal>"
    "  <method name='FocusInput'>"
    "  </method>"
    "  <signal name='PageCreated'>"
    "   <arg type='t' name='page_id' direction='out'/>"
    "  </signal>"
    "  <method name='SetHeaderSetting'>"
    "   <arg type='s' name='headers' direction='in'/>"
    "  </method>"
    " </interface>"
    "</node>";

/* Global struct to hold internal used variables. */
struct Ext {
    guint               regid;
    GDBusConnection     *connection;
    WebKitWebPage       *webpage;
    WebKitDOMElement    *active;
    GHashTable          *headers;
    GHashTable          *documents;
    gboolean            input_focus;
    GArray              *page_created_signals;
};
struct Ext ext = {0};


/**
 * Webextension entry point.
 */
G_MODULE_EXPORT
void webkit_web_extension_initialize_with_user_data(WebKitWebExtension *extension, GVariant *data)
{
    char *server_address;
    GDBusAuthObserver *observer;

    g_variant_get(data, "(ms)", &server_address);
    if (!server_address) {
        g_warning("UI process did not start D-Bus server");
        return;
    }

    g_signal_connect(extension, "page-created", G_CALLBACK(on_page_created), NULL);

    observer = g_dbus_auth_observer_new();
    g_signal_connect(observer, "authorize-authenticated-peer",
            G_CALLBACK(on_authorize_authenticated_peer), extension);

    g_dbus_connection_new_for_address(server_address,
            G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT, observer, NULL,
            (GAsyncReadyCallback)on_dbus_connection_created, extension);
    g_object_unref(observer);
}

static gboolean on_authorize_authenticated_peer(GDBusAuthObserver *observer,
        GIOStream *stream, GCredentials *credentials, gpointer extension)
{
    static GCredentials *own_credentials = NULL;
    GError *error = NULL;

    if (!own_credentials) {
        own_credentials = g_credentials_new();
    }

    if (credentials && g_credentials_is_same_user(credentials, own_credentials, &error)) {
        return TRUE;
    }

    if (error) {
        g_warning("Failed to authorize connection to ui: %s", error->message);
        g_error_free(error);
    }

    return FALSE;
}

static void on_dbus_connection_created(GObject *source_object,
        GAsyncResult *result, gpointer data)
{
    static GDBusNodeInfo *node_info = NULL;
    GDBusConnection *connection;
    GError *error = NULL;

    if (!node_info) {
        node_info = g_dbus_node_info_new_for_xml(introspection_xml, NULL);
    }

    connection = g_dbus_connection_new_for_address_finish(result, &error);
    if (error) {
        g_warning("Failed to connect to UI process: %s", error->message);
        g_error_free(error);
        return;
    }

    /* register the webextension object */
    ext.regid = g_dbus_connection_register_object(
            connection,
            VB_WEBEXTENSION_OBJECT_PATH,
            node_info->interfaces[0],
            &interface_vtable,
            WEBKIT_WEB_EXTENSION(data),
            NULL,
            &error);

    if (!ext.regid) {
        g_warning("Failed to register web extension object: %s", error->message);
        g_error_free(error);
        g_object_unref(connection);
        return;
    }

    emit_page_created_pending(connection);
    ext.connection = connection;
}

/**
 * Add observers to doc event for given document and all the contained iframes
 * too.
 */
static void add_onload_event_observers(WebKitDOMDocument *doc,
        WebKitWebExtension *extension)
{
    WebKitDOMEventTarget *target;

    /* Add the document to the table of known documents or if already exists
     * return to not apply observers multiple times. */
    if (!g_hash_table_add(ext.documents, doc)) {
        return;
    }

    /* We have to use default view instead of the document itself in case this
     * function is called with content document of an iframe. Else the event
     * observing does not work. */
    target = WEBKIT_DOM_EVENT_TARGET(webkit_dom_document_get_default_view(doc));

    webkit_dom_event_target_add_event_listener(target, "focus",
            G_CALLBACK(on_editable_change_focus), TRUE, extension);
    webkit_dom_event_target_add_event_listener(target, "blur",
            G_CALLBACK(on_editable_change_focus), TRUE, extension);
    /* Check for focused editable elements also if they where focused before
     * the event observer where set up. */
    /* TODO this is not needed for strict-focus=on */
    on_editable_change_focus(target, NULL, extension);
}

/**
 * Emit the page created signal that is used in the ui process to finish the
 * dbus proxy connection.
 */
static void emit_page_created(GDBusConnection *connection, guint64 pageid)
{
    GError *error = NULL;

    /* propagate the signal over dbus */
    g_dbus_connection_emit_signal(G_DBUS_CONNECTION(connection), NULL,
            VB_WEBEXTENSION_OBJECT_PATH, VB_WEBEXTENSION_INTERFACE,
            "PageCreated", g_variant_new("(t)", pageid), &error);

    if (error) {
        g_warning("Failed to emit signal PageCreated: %s", error->message);
        g_error_free(error);
    }
}

/**
 * Emit queued page created signals.
 */
static void emit_page_created_pending(GDBusConnection *connection)
{
    int i;
    guint64 pageid;

    if (!ext.page_created_signals) {
        return;
    }

    for (i = 0; i < ext.page_created_signals->len; i++) {
        pageid = g_array_index(ext.page_created_signals, guint64, i);
        emit_page_created(connection, pageid);
    }

    g_array_free(ext.page_created_signals, TRUE);
    ext.page_created_signals = NULL;
}

/**
 * Write the page id of the created page to a queue to send them to the ui
 * process when the dbus connection is established.
 */
static void queue_page_created_signal(guint64 pageid)
{
    if (!ext.page_created_signals) {
        ext.page_created_signals = g_array_new(FALSE, FALSE, sizeof(guint64));
    }
    ext.page_created_signals = g_array_append_val(ext.page_created_signals, pageid);
}

/**
 * Emits a signal over dbus.
 *
 * @name:   Signal name to emit.
 * @data:   GVariant value used as value for the signal or NULL.
 */
static void dbus_emit_signal(const char *name, WebKitWebExtension* extension,
        GVariant *data)
{
    GError *error = NULL;

    if (!ext.connection) {
        return;
    }

    /* propagate the signal over dbus */
    g_dbus_connection_emit_signal(ext.connection, NULL,
            VB_WEBEXTENSION_OBJECT_PATH, VB_WEBEXTENSION_INTERFACE, name,
            data, &error);
    if (error) {
        g_warning("Failed to emit signal '%s': %s", name, error->message);
        g_error_free(error);
    }
}

/**
 * Handle dbus method calls.
 */
static void dbus_handle_method_call(GDBusConnection *conn, const char *sender,
        const char *object_path, const char *interface_name, const char *method,
        GVariant *parameters, GDBusMethodInvocation *invocation, gpointer extension)
{
    char *value;

    if (!g_strcmp0(method, "FocusInput")) {
        ext_dom_focus_input(webkit_web_page_get_dom_document(ext.webpage));
        g_dbus_method_invocation_return_value(invocation, NULL);
    } else if (!g_strcmp0(method, "SetHeaderSetting")) {
        g_variant_get(parameters, "(s)", &value);

        if (ext.headers) {
            soup_header_free_param_list(ext.headers);
            ext.headers = NULL;
        }
        ext.headers = soup_header_parse_param_list(value);
        g_dbus_method_invocation_return_value(invocation, NULL);
    }
}

/**
 * Callback called if a editable element changes it focus state.
 * Event target may be a WebKitDOMDocument (in case of iframe) or a
 * WebKitDOMDOMWindow.
 */
static void on_editable_change_focus(WebKitDOMEventTarget *target,
        WebKitDOMEvent *event, WebKitWebExtension *extension)
{
    gboolean input_focus;
    WebKitDOMDocument *doc;
    WebKitDOMElement *active;

    if (WEBKIT_DOM_IS_DOM_WINDOW(target)) {
        g_object_get(target, "document", &doc, NULL);
    } else {
        /* target is a doc document */
        doc = WEBKIT_DOM_DOCUMENT(target);
    }
    active = webkit_dom_document_get_active_element(doc);
    /* Don't do anything if there is no active element or the active element
     * is the same as before. */
    if (!active || active == ext.active) {
        return;
    }
    if (WEBKIT_DOM_IS_HTML_IFRAME_ELEMENT(active)) {
        WebKitDOMHTMLIFrameElement *iframe;
        WebKitDOMDocument *subdoc;

        iframe = WEBKIT_DOM_HTML_IFRAME_ELEMENT(active);
        subdoc = webkit_dom_html_iframe_element_get_content_document(iframe);
        add_onload_event_observers(subdoc, extension);
        return;
    }

    ext.active = active;

    /* Check if the active element is an editable element. */
    input_focus = ext_dom_is_editable(active);
    if (input_focus != ext.input_focus) {
        ext.input_focus = input_focus;

        dbus_emit_signal("EditableChangeFocus", extension,
                g_variant_new("(b)", input_focus));
    }
}

/**
 * Callback for web extensions page-created signal.
 */
static void on_page_created(WebKitWebExtension *extension, WebKitWebPage *webpage, gpointer data)
{
    guint64 pageid = webkit_web_page_get_id(webpage);

    /* Save the new created page in the extension data for later use. */
    ext.webpage = webpage;
    if (ext.connection) {
        emit_page_created(ext.connection, pageid);
    } else {
        queue_page_created_signal(pageid);
    }

    g_object_connect(webpage,
            "signal::send-request", G_CALLBACK(on_web_page_send_request), extension,
            "signal::document-loaded", G_CALLBACK(on_web_page_document_loaded), extension,
            NULL);
}

/**
 * Callback for web pages document-loaded signal.
 */
static void on_web_page_document_loaded(WebKitWebPage *webpage, gpointer extension)
{
    /* If there is a hashtable of known document - detroy this and create a
     * new hashtable. */
    if (ext.documents) {
        g_hash_table_unref(ext.documents);
    }
    ext.documents = g_hash_table_new(g_direct_hash, g_direct_equal);

    add_onload_event_observers(webkit_web_page_get_dom_document(webpage),
            WEBKIT_WEB_EXTENSION(extension));
}

/**
 * Callback for web pages send-request signal.
 */
static gboolean on_web_page_send_request(WebKitWebPage *webpage, WebKitURIRequest *request,
        WebKitURIResponse *response, gpointer extension)
{
    char *name, *value;
    SoupMessageHeaders *headers;
    GHashTableIter iter;

    if (!ext.headers) {
        return FALSE;
    }

    /* Change request headers according to the users preferences. */
    headers = webkit_uri_request_get_http_headers(request);
    if (!headers) {
        return FALSE;
    }

    g_hash_table_iter_init(&iter, ext.headers);
    while (g_hash_table_iter_next(&iter, (gpointer*)&name, (gpointer*)&value)) {
        /* Null value is used to indicate that the header should be
         * removed completely. */
        if (value == NULL) {
            soup_message_headers_remove(headers, name);
        } else {
            soup_message_headers_replace(headers, name, value);
        }
    }

    return FALSE;
}