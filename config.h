/*
 * Colors are standard hex color codes, prefixed by '0x'
 */
#define BORDER_FOCUSED 0x55aaaa
#define BORDER_UNFOCUSED 0xeaffff
#define BG_COLOR 0x000000;
#define FG_COLOR 0xffffff;

/*
 * The MODKEY is the modifier key used for keybinds.
 * Mod1Mask = Alt
 * Mod4Mask = Super/Windows Key
 */
#define MODKEY Mod4Mask

static const Dael_Keybinding keys[] = {
        /* Modifier | Key | function | args */
        { MODKEY, XK_d, launch_program, "dmenu_run" },
        { MODKEY, XK_Return, launch_program, "st" },


        { NoSymbol, 0, 0, 0 } /* end of keybind list (Do not remove) */
};
