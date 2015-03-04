## FAQ

### I just installed uzbl but it doesn't do much. What now?

There's no "uzbl" command, that's just the name for the whole collection of
tools. The command you're looking for is `uzbl-browser` or `uzbl-tabbed`.

The central program `uzbl-core` doesn't do many useful things by itself, it's
meant for integration with other tools and scripts. See README.

Once you've got `uzbl-browser` or `uzbl-tabbed` running, the first thing to do
is look at the [keybindings](http://uzbl.org/keybindings.php). The second thing
to do is explore the default configuration (`~/.config/uzbl/config`).

### Help, Uzbl isn't responding to any of my keyboard commands!

If the left side of the status bar says `[]` instead of `[Cmd]` when you start
`uzbl-browser`, then the event manager isn't starting or its plugins aren't
loading.

If you're trying to run uzbl without installing it, the
`test-uzbl-browser-sandbox` targets in the makefile should help. Persistent
changes may be stored in the `sandbox/home` directory.

### Where are the widgets (forward button, back button, search bar, etc)?

There are none. What we do have is a powerful statusbar and lots of keybinding
possibilities.

### Why can each `uzbl-core`/`uzbl-browser` process only show one page?

This allows a simple implementation of both `uzbl-core` and `uzbl-browser`, and
it makes things more robust. But read the next entry...

### How can I have multiple pages in one window?

So, given that `uzbl-core` and `uzbl-browser` only deal with one page at a time
(see above), how can you have a window with multiple pages?

Basically this involves concerns on two sides:

* window management
  - can I keep all pages together in 1 X window so that I can move all of them
    to a different workspace at once
  - can I "split off" pages into separate windows (i.e. move only one specific
    page/window to a different desktop) or merge windows together?
  - can I integrate uzbl pages/windows into WM features? (alt-tab, tiling
    layouts, taskbar, ...)
* application-level
  - realtime overview of all page titles of all uzbl instances
  - representation styles which are tightly coupled to the application such as
    treeviews that show from which you page you opened others, or page state
    (loading etc)

Uzbl itself can hardly be a limiting factor, as it supports/has:

* Xembed (GtkPlug mode) so you can embed a uzbl-browser or uzbl-core into
  another window;
* an events system you can have realtime updates of window title, pageload
  state, etc.; and
* a command interface to programmatically change it's behavior.

And then there is the style of representation (tabs, tree overviews, visual
thumbnails etc) which can be handled from the WM side or the application side.

There are multiple approaches, each with pros and cons.

* Tabbing in the WM: XMonads tabbed layout, wmii's stacked layout, fluxbox or
  kwin tabs and so on.
* Visual overview in the WM: commonly used with dwm or Awesome's tiling layouts
  with master/slave areas. The [dynamic zoom script][] is useful here.
* A container application which embeds multiple uzbl-browsers and provide
  tablists, tree views, and more. Examples:
  - [uzbl-tabbed][] (officially supported)
  - [uzbltreetab][]
  - [uzbltab][]
  - [suckless tabbed][]
* An application to mimic tabbing independently of WM support. The only thing
  you need to do is focus/maximize the instance you want, keep the others out
  of sight and use tools like dmenu/xbindkeys and wmctrl to switch instances.
  This allows you to use application-specific properties (such as uzbl tag,
  name etc). For more ideas on such an approach, see
  docs/multiple-instances-management.
  Examples:
  - [wmctrl-based](http://www.uzbl.org/wiki/metacity-tabs) (works on at least Metacity)
  - [wmii](http://www.uzbl.org/wiki/wmii)

There are really a lot of options. You need to think about what you need, what
you want and what you don't care about.

On the wiki you'll find a lot of related scripts, some of them providing new
workflows (do you really need open windows for all pages you intend to read, or
is a list enough? [articlequeue](http://www.uzbl.org/wiki/article_queue.py)),
some providing integration with WMs such as
[awesome](http://www.uzbl.org/wiki/awesome), and more.

### Okay, what can I actually do? What commands are there? How do I get more information?

Commands and other features are documented in README files. Read them.

Other great resources are the example config (`~/.uzbl/config/config`), the
scripts included with uzbl, and the wiki.

### Why can't I type anything in forms?

By default uzbl is modal (like `vi`). It starts in command mode, not in insert
mode. If you don't like this you can easily change it.

When you are in command mode, the left side of the status bar should say
`[Cmd]`. In command mode you can trigger actions inside uzbl with the minimum
amount of keypresses, but webpages won't see your keypresses, because they're
all being interpreted by uzbl.

After going into insert mode (by default this is the 'i' binding), the status
bar should say `[Ins]`. Your keypresses are not interpreted but passed on, so
you can enter text into forms or use keybindings that are interpreted by the
page's javascript. Press Esc to go out of insert mode.

### Do you support Flash? JavaScript? AJAX? Recent html/css/.. standards? Java/media plugins?

Yes, Webkit takes care of all of that. Not that we like all of these, but you
can use them if you want.

If you build uzbl against GTK3, plugins such as Flash are known not to work
(they use GTK2 and the symbols conflict). If you want to use Flash, build with
GTK3.

We use the NPAPI plugin architecture (just like Mozilla) so just install the
plugins normally, and things should work.

### How can I send commands to uzbl from an external script?

Every `uzbl-core` instance creates a fifo and a socket on the filesystem that
you can use to communicate with it. By default these are located at
`/tmp/uzbl_fifo_[name]` and `/tmp/uzbl_socket_[name]`.

The example scripts and keybindings have many examples of this.

### What's the difference between the socket file and the fifo?

Fifos are easy to work with. You can write commands to them like any other file
into them, but they are unidirectional (you can send uzbl commands, but you
can't receive responses back from it).

Sockets are bidirectional but more complex. In shell scripts you can use
`socat` to work with sockets. Other languages have their own ways of connecting
and writing to sockets.

When writing scripts fifos are usually the fastest method (because you do not
need to fork another process), so fifo is preferred unless you need a response.

### Uzbl uses too much memory, especially when multiple windows are open

It's not as bad as its looks! Linux (and other systems) report memory usage in
a confusing way.

You need to be aware of the difference between RSS and VSS. See [this
page](http://virtualthreads.blogspot.com/2006/02/understanding-memory-usage-on-linux.html)
for a good explanation.

Dynamic libraries (libwebkit, libgtk, etc) that are used by multiple processes
are only stored in RAM once.

### What the hell is this 'XDG' stuff??

You'll notice our example/default scripts and configs use variables such as
`$XDG_CONFIG_HOME` and `$XDG_DATA_HOME`. The are part of the [xdg basedir
spec][]. The idea is that it keeps your `$HOME` clean and separates config,
data and cache.

If these variables are not defined on your system, it could be that you need to
install an xdg package.

If you don't like this, no one is stopping you from changing the scripts and
configs to point to a single `$HOME/.uzbl` directory or whatever you want.

### Does the world really need another browser?

We did try a lot of browsers, and we do not suffer
[NIH](http://en.wikipedia.org/wiki/Not_Invented_Here).

We believe that the approach taken by way too many browsers is wrong. We do not
want browsers that try to do everything, instead we prefer a system where
different applications work together. We also like having a browser that is
extensible in whatever language you're most comfortable with.

We also like open source. We take a lot of things from other projects and we
also try to contribute to other projects.

### What? You call all of this user-friendly?

Yes. If you don't agree, don't use it :)

[dynamic zoom script]: http://www.uzbl.org/wiki/dynamic_zooming
[uzbl-tabbed]:         http://www.uzbl.org/wiki/uzbl_tabbed
[uzbltreetab]:         http://www.uzbl.org/wiki/uzbltreetab
[uzbltab]:             http://www.uzbl.org/wiki/uzbltab
[suckless tabbed]:     http://tools.suckless.org/tabbed
[xdg basedir spec]:    http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html
