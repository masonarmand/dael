dael window manager (w.i.p)
===================

Dael is a minimalist window manager designed to meet my specific
needs/workflow. I wanted a window manager that was lightweight, simple,
and only managed windows (without including things like status bars). I
also didn't want complex window management - as I personally usually just
have 1-2 windows open at a time - so you only need to remember a few simple
keybinds to operate the window manager. I also wanted something that has only
basic configuration options. It is my personal belief that a window manager is
only meant to be a tool to manage windows, and should not be 'eye candy', so
this window manager has a few basic color options but nothing else.

This window manager is inspired by dwm, although it doesn't share the same
source code.

If you're wondering what 'dael' means: it is the first track on Autechre's
1995 album Tri Repetae: https://en.wikipedia.org/wiki/Tri_Repetae

Window Management
=================

Default Tiling Mode:
One window is the master window, the rest of the windows are slaves of the
master window. Tiling will look like this:
--------------
|        |   |
|        |___|
| Master |   |
|        |___|
|        |   |
--------------
Monocle Tiling Mode:
The currently focused window becomes full screen with no borders.
--------------
|            |
|            |
|  Focused   |
|  Window    |
|            |
--------------


window management functions
`kill_window`       | kill/close a window
`launch_program`    | summon a window/task
`append_workspace`  | create a workspace
`next_workspace`    | go to next workspace
`prev_workspace`    | go to previous workspace
`focus_next`        | moves focus to previous window
`focus_prev`        | moves focus to next window
`swap_master`       | swaps currently focused window with master window
`increase_size`     | increase size of the master window
`decrease_size`     | decrease size of the master window
`cycle_tiling_mode` | go to next tiling mode

Workspaces
==========
Workspaces function like nodes in a linked list. You can cycle through the
nodes and append more.

Configuration
=============
Configuration - like dwm - is done through editing the config.h header
file and re-compiling.

Installation
============
run `sudo make install`

Sandbox
=======
if you have Xephyr installed, you can do:
```
make
./run_sandbox.sh
```
to run the window manager inside of your own window manager to test it.
