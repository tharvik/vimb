---
title:  vimb - commands
layout: default
meta:   vimb browser ex commands
active: commands
---

# vimb commands

Commands that are listed below are ex-commands like in vim, that are typed
into the inputbox (the command line of vimb). The commands may vary in their
syntax or in the parts they allow, but in general they follow a simple syntax.

    :[N]cmd[name][!][ lhs][ rhs]

Where `lhs` (left hand side) must not contain any unescaped space. The syntax
of the `rhs` (right hand side) if this is available depends on the command. At
the moment the count parts `[N]` of commands is parsed, but actual there does
not exists any command that uses the count.

## open
{:#open}

\:o[pen] [*URI*]
: Open the give *URI* into current window. If URI is empty the configured
  `home-page` is opened.

\:t[abopen] [*URI*]
: Open the give *URI* into a new window. If URI is empty the configured
  `home-page` is opened.

## key mapping

Key mappings allow to alter actions of key presses. Each key mapping is
associated with a mode and only has effect when the mode is active. Following
commands allow the user to substitute one sequence of key presses by another.

Syntax: *:{**m**}map {lhs} {rhs}*

Note that the lhs ends with the first found space. If you want to use space
also in the {lhs} you have to escape this with a single '\' like shown in the
examples. Standard key mapping commands are provided for these modes m:

- **n** Normal mode: When browsing normally.
- **i** Insert mode: When interacting with text fields on a website.
- **c** Command Line mode: When typing into the command line of vimb

Most keys in key sequences are represented simply by the character that you see
on the screen when you type them. However, as a number of these characters have
special meanings, and a number of keys have no visual representation, a special
notation is required.

As special key names have the format \<...\>. Following special keys can be
used \<Left\>, \<Up\>, \<Right\>, \<Down\> for the cursor keys, \<Tab\>,
\<Esc\>, \<CR\>, \<F1\>-\<F12\> and \<C-A\>-\<C-Z\>.

\:nm[ap] {*lhs*} {*rhs*}
\:im[ap] {*lhs*} {*rhs*}
\:cm[ap] {*lhs*} {*rhs*}
: Map the key sequence *lhs* to *rhs* for the modes where the map command applies.
  The result, including *rhs*, is then further scanned for mappings. This allows
  for nested and recursive use of mappings.

: - `:cmap <C-G>h /home/user/downloads/`
    Adds a keybind to insert a file path into the input box. This could be
    useful for the ':save' command that could be used as ':save ^Gh'.
  - `:nmap <F1> :set scripts=on<CR>:open !glib<Tab><CR>`
    This will enable scripts and lookup the first bookmarked URI with the tag
    'glib' and open it immediately if F1 key is pressed.
  - `:nmap \ \  50G;o`
    Example which mappes two spaces to go to 50% of the page, start hinting
    mode.

\:nn[oremap] {*lhs*} {*rhs*}
\:ino[remap] {*lhs*} {*rhs*}
\:cno[remap] {*lhs*} {*rhs*}
: Map the key sequence *lhs* to *rhs* for the mode where the map command applies.
  Disallow mapping of *rhs*, to avoid nested and recursive mappings. Often used
  to redefine a command.

\:nu[nmap] {*lhs*}
\:iu[nmap] {*lhs*}
\:cu[nmap] {*lhs*}
: Remove the mapping of *lhs* for the applicable mode.

## bookmarks

\:bma [*tags*]
: Save the current opened uri with *tags* to the bookmark file.

\:bmr [*URI*]
: Removes all bookmarks for given *URI* or if not given the current opened page.

## shortcuts
{:#shortcuts}

Shortcuts allows to open URL build up from a named template with additional
parameters. If a shortcut named 'dd' is defined, you can use it with ':open dd
list of parameters' to open the generated URL.

Shortcuts are a good to use with search engines where the URL is nearly the
same but a single parameter is user defined.

\:shortcut-add *shortcut=URI*
: Adds a shortcut with the *shortcut* and URI template. The URI can contain
  multiple placeholders `$0-$9` that will be filled by the parameters given when
  the shortcut is called. The parameters given when the shortcut is called
  will be split into as many parameters like the highest used placeholder.

: - `:shortcut-add dl=https://duckduckgo.com/lite/?q=$0`
    to setup a search engine. Can be called by ':open dl my search phrase'.
  - `:shortcut-add gh=https://github.com/$0/$1`
    to build url from given parameters. Can be called ':open gh fanglingsu vimb'.

\:shortcut-remove *shortcut*
: Remove the search engine to the given *shortcut*.

\:shortcut-default *shortcut*
: Set the shortcut for given *shortcut* as the default. It doesn't matter if the
  *shortcut* is already in use or not to be able to set it.

## handlers
{:#handlers}

Handlers allow specifying external scripts to handle alternative URI methods.

\:handler-add *handler=cmd*
: Adds a handler to direct *cmd* links to the external *cmd*. The *cmd* can
  contain one placeholder %s that will be filled by the full URI given when
  the command is called.
: Examples:

: - `:handler-add magnet=xdg-open %s`
    to open magnet links with xdg-open.
  - `:handler-add magnet=transmission-gtk %s`
    to open magnet links directly with Transmission.
  - `:handler-add irc=irc-handler.sh %s`
    to direct `irc://<host>:<port>/<channel>` links to a wrapper for your irc client.

:handler-remove *handler*
: Remove the handler for the given URI *handler*.

## settings
{:#settings}

\:se[t] *var=value*
: Set configuration values named by *var*. To set boolean variable you should
  use 'on', 'off' or 'true' and 'false'. Colors are given as hexadecimal value
  like '#f57700'.

\:se[t] *var+=value*
: Add the *value* to a number option, or apend the *value* to a string option.
  When the option is a comma separated list, a comma is added, unless the
  value was empty.

\:set[t] *var^=value*
: Multiply the *value* to a number option, or prepend the *value* to a string
  option. When the option is a comma separated list, a comma is added, unless
  the value was empty.

\:se[t] *var-=value*
: Subtract the *value* from a number option, or remove the *value* from a
  string option, if it is there. When the option is a comma separated list, a
  comma is deleted, unless the option becomes empty.

\:se[t] *var*?
: Show the current set value of variable *var*.

\:se[t] *var*!
: Toggle the value of boolean variable *var* and display the new set value.

## queue
{:#queue}

The queue allows to mark URLs for later reading (something like a read it later
list). This list is shared between the single instances of vimb. Only available
if vimb has been compiled with QUEUE feature.

\:qpu[sh] [*URI*]
: Push URI or if not given current *URI* to the end of the queue.

\:qu[nshift] [*URI*]
: Push URI or if not given current *URI* to the beginning of the queue.

\:qp[op]
: Open the oldest queue entry in current browser window and remove it from the
  queue.

\:qc[lear]
: Removes all entries from queue.

## automatic commands
{:#autocmd}

An autocommand is a command that is executed automatically in response to some
event, such as a URI being opened. Autocommands are very powerful. Use them
with care and they will help you avoid typing many commands.

Note: The `:autocmd` command cannot be followed by another command, since any
'|' is considered part of the command.

Autocommands are built with following properties.

*group*
: When the [group] argument is not given, Vimb uses the current group as
  defined with ':augroup', otherwise, vimb uses the group defined with [group].
  Groups are useful to remove multiple grouped autocommands.

*event*
: You can specify a comma separated list of event names. No white space can be
  used in this list.
: Events:
: - `LoadProvisional` Fired if a new page is going to opened. No data has been received yet,
  the load may still fail for transport issues. Out of this reason this
  event has no associated URL to match.
  - `LoadCommited` Fired if first data chunk has arrived, meaning that the
    necessary transport requirements are established, and the load is being
    performed. This is the right event to toggle content related setting like
    'scripts', 'plugins' and such things.
  - `LoadFirstLayout` fired if the first layout with actual visible content is shown.
  - `LoadFinished` Fires when everything that was required to display on the
    page has been loaded.
  - `LoadFailed` Fired when some error occurred during the page load that
    prevented it from being completed.
  - `DownloadStart` Fired right before a download is started. This is fired
    for vimb downloads as well as external downloads if
    'down‐load-use-external' is enabled.
  - `DownloadFinished` Fired if a vimb managed download is finished. For
    external download this event is not available.
  - `DownloadFailed` Fired if a vimb managed download failed. For external
    download this event is not available.

*pat*
: Comma separated list of patterns, matches in order to check if a autocommand
  applies to the URI associated to an event. To use ',' within the single
  patterns this must be escaped as '\,'.
: Patterns:
: - `\*` Matches any sequence of characters. This includes also `/` in contrast to shell patterns.
  - `?` Matches any single character except of `/`.
  - `{one,two}` Matches 'one' or 'two'. Any `{`, `,` and `}` within this
    pattern must be escaped by a '`\`'. `*` and `?` have no special meaning
    within the curly braces.
  - `\` Use backslash to escape the special meaning of '`?*{},`' in the pattern or pattern list.

*cmd*
: Any ex command vimb understands. The leading ':' is not required. Multiple
  commands can be separated by '|'.

\:au[tocmd] [*group*] {*event*} {*pat*} {*cmd*}
: Add cmd to the list of commands that vimb will execute automatically on event
  for a URI matching pat autocmd-patterns. Vimb always adds the cmd after
  existing autocommands, so that the autocommands are executed in the order in
  which they were given.

\:au[tocmd]! [*group*] {*event*} {*pat*} {*cmd*}
: Remove all autocommands associated with *event* and which pattern match
  *pat*, and add the command *cmd*. Note that the pattern is not matches
  literally to find autocommands to remove, like vim does. Vimb matches
  the autocommand pattern with *pat*.

\:au[tocmd]! [*group*] {*event*} {*pat*}
: Remove all autocommands associated with *event* and which pattern matches
  *pat*.

\:au[tocmd]! [*group*] * {*pat*}
: Remove all autocommands with patterns matching *pat* for all events.

\:au[tocmd]! [*group*] {*event*}
: Remove all autocommands for *event*.

\:au[tocmd]! [*group*]
: Remove all autocommands.

\:aug[roup] {*name*}
: Define the autocmd group *name* for the following `:autocmd` commands. The
  name "end" selects the default group.

\:aug[roup]! {*name*}
: Delete the autocmd group *name*.

Example:

    :aug mygroup
    :  au LoadCommited * set scripts=off|set cookie-accept=never
    :  au LoadCommited http{s,}://github.com/*.https://maps.google.de/* set scripts=on
    :  au LoadFinished https://maps.google.de/* set useragent=foo
    :aug end

## misc

\:sh[ellcmd] *cmd*
: Runs given shell *cmd* synchronous and print the output into inputbox. The
  *CMD* can contain multiple '%' chars that are expanded to the current opened
  uri. Also the `~/` to home directory expansion is available.
: Runs given shell *cmd* syncronous and print the output into inputbox.
  Follwing pattern in *cmd* are expanded, `~username`, `~/`, `$VAR` and
  `${VAR}`. A '``\``' before these patterns disables the expansion.
: Example: `:sh ls -la $HOME`

:sh[ellcmd]! *cmd*
: Like `:shellcmd` but runs given shell *cmd* asyncron.
: Example: ``:sh! /bin/sh -c 'echo "`date` $VIMB_URI" >> myhistory.txt'``

\:s[ave] [*path*]
: Download current opened page into configured download directory. If *path* is
  given, download under this file name or path. Possible value for PATH are
  'page.html', 'subdir/img1.png', '~/download.html' or absolute paths
  '/tmp/file.html'.

\:q[uit]
: Close the browser. This will be refused if there are running downloads.

\:q[uit]!
: Close the browser independent from an running download.

\:reg[ister]
: Display the contents of all registers.
: Register:
: - `"a` -- `"z` 26 named registers. Vimb fills these registers only when you
    say so.
  - `"%` Contains the curent opened URI.
  - `":` Contains the most recent executed ex command.
  - `"/` Contains the most recent search-pattern.
  - `";` Contains the last hinted URL. This can be used in `x-hint-command` to
    get the URL of the hint.

\:e[val] *javascript*
: Runs the given *javascript* in the current page and display the evaluated
  value.
: This comman cannot be followed by antoher command, since any '|' is
  considered part of the command.
: Example: `:eval document.cookie`

\:e[val]! *javascript*
: Like `:e[val]`, but there is nothing print to the input box.

\:no[rmal] [*cmds*]
: Execute normal mode commands *cmds*. This makes it possible to execute normal
  mode commands typed on the input box.
: *cmds* cannot start with a space. Put a count of 1 (one) before it, "1 " is one space.
  This comman cannot be followed by antoher command, since any '|' is
  considered part of the command.
: Example: `:set scripts!|no! R`

\:no[rmal]! [*cmds*]
: Like `:no[rmal]`, but no mapping is applied to *cmds*.

\:ha[rdcopy]
: Print current document. Open a GUI dialog where you can select the printer,
  number of copies, orientation, etc.