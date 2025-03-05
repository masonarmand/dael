dael window manager
===================

Dael is a minimalist window manager designed to meet my specific
needs/workflow. I wanted a window manager that was lightweight, simple,
and only managed windows (without including things like status bars). I
also didn't want complex window management - as I personally usually just
have 1-2 windows open at a time - so you only need to remember a few simple
keybinds to operate the window manager. I also wanted something that had only
basic configuration options. It is my personal belief that a window manager is
only meant to be a tool to manage windows, and should not be 'eye candy', so
this window manager has a few basic color options but nothing else.

This window manager is inspired by dwm, although it doesn't share the same
source code.

Window Management
=================

One window is the master window, the rest of the windows are slaves of the
master window. Tiling will look like this:
--------------
|        |   |
|        |___|
| Master |   |
|        |___|
|        |   |
--------------

window management functions
`kill_window`      | kill/close a window
`launch_program`   | summon a window/task
`append_workspace` | create a workspace
`next_workspace`   | go to next workspace
`prev_workspace`   | go to previous workspace
`focus_next`       | moves focus to previous window
`focus_prev`       | moves focus to next window
`swap_master`      | swaps currently focused window with master window

Workspaces
==========
Workspaces function like nodes in a linked list. You can cycle through the
nodes and append or remove them.

Configuration
=============
Configuration - like dwm - is done through editing the config.h header
file and re-compiling.
