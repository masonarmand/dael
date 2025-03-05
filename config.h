/*
 * Colors are standard hex color codes, prefixed by '0x'
 */
#define BORDER_SIZE 1
#define BORDER_FOCUSED 0xff0000
#define BORDER_UNFOCUSED 0x043764
#define BG_COLOR 0x000000
#define FG_COLOR 0xffffff

/* font */
#define FONT "-misc-fixed-bold-r-normal--13-120-75-75-C-70-iso10646-1"

/*
 * The MODKEY is the modifier key used for keybinds.
 * Mod1Mask = Alt
 * Mod4Mask = Super/Windows Key
 */
#define MODKEY Mod4Mask

static const Dael_Keybinding config_keys[] = {
        /*
          Modifier------------Keycode----function----------args---------*/
        { MODKEY,             XK_d,      launch_program,   "dmenu_run" },
        { MODKEY,             XK_Return, launch_program,   "st"        },
        { MODKEY | ShiftMask, XK_q,      kill_window                   },
        { MODKEY | ShiftMask, XK_e,      quit                          },
        { MODKEY | ShiftMask, XK_l,      next_workspace                },
        { MODKEY | ShiftMask, XK_h,      prev_workspace                },
        { MODKEY | ShiftMask, XK_w,      append_workspace              },
        { MODKEY,             XK_m,      cycle_tiling_mode             },
        { MODKEY,             XK_l,      focus_next                    },
        { MODKEY,             XK_h,      focus_prev                    },
        { MODKEY,             XK_space,  swap_master                   },



        { NoSymbol, 0, 0, 0 } /* end of keybind list (Do not remove) */
};
