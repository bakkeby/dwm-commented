/* See LICENSE file for copyright and license details. */

/* This is the default configuration file for dwm. At compile time if a file config.h does not
 * exist then the default configuration file config.def.h will be copied to config.h.
 *
 * The config.h file is the user's personal configuration file that can be tailored to their
 * preferences.
 *
 * The relatively simple relationship between the two files is often misunderstood, however, due
 * to using patching tools like patch or git apply and being confused about why changes that were
 * applied to the default configuration were not applied to their personal configuration file.
 *
 * This in turn often lead to misleading recommendations to edit the default configuration file
 * instead and to delete config.h prior to compiling.
 *
 * While this may work in practice the downside of this approach is that:
 *    - personal configuration is exposed in the default configuration file and
 *    - personal configuration needs to be committed in version control and
 *    - changes to the default configuration file opens for more conflicts when applying patches
 *      as the underlying code has changed
 *
 * In general it is not complicated to work out what changes a patch makes to the default
 * configuration file and to copy those changes into the personal configuration file. Consider
 * doing a diff between the two files or simply look at the patch file to work out what has
 * changed.
 */

/* Settings related to appearance. */

/* The border pixel determines the size of the window border. */
static const unsigned int borderpx  = 1;        /* border pixel of windows */
/* The snap pixel controls two things:
 *    - how close to the window area border a window must be before it "snaps" (or docks) against
 *      that border when moving a floating window using the mouse
 *    - how far the mouse needs to move before a tiled window "snaps" out to become floating when
 *      moving or resizing a window using the mouse
 */
static const unsigned int snap      = 32;       /* snap pixel */
/* Whether the bar is shown by default on startup or not. */
static const int showbar            = 1;        /* 0 means no bar */
/* Whether the bar is shown at the top or at the bottom of the monitor. */
static const int topbar             = 1;        /* 0 means bottom bar */
/* This defines the primary font and optionally fallback fonts. If a glyph does not exist for a
 * character (code point) in the primary font then fallback fonts will be checked.
 * If the fallback fonts also do not have that character then system fonts will be checked for the
 * missing character. If a system font was found then that font will be added to the list of
 * fallback fonts for future reference.
 *
 * Note that "monospace" is not an actual font, it is an alias for another font which the system
 * denotes as the main monospace font. E.g.
 *
 *    $ fc-match monospace
 *    NotoSansMono-Regular.ttf: "Noto Sans Mono" "Regular"
 *
 * Use fc-list to find specific fonts to use, e.g.
 *
 *    $ fc-list | grep DejaVu
 *    /usr/share/fonts/TTF/DejaVuSansMono.ttf: DejaVu Sans Mono:style=Book
 *
 * Then add the family to the fonts array, e.g.
 *
 *    static const char *fonts[] = { "DejaVu Sans Mono:style=Book:pixelsize=16" };
 *
 * A note about pixelsize vs size; 1 pixel (px) is usually assumed to be 1/96th of an inch while
 * 1 point (pt) is assumed to be 1/72nd of an inch. Therefore a (point) size of 12 is the same as
 * a pixelsize of 16.
 *
 * For general information on font setup refer to:
 *    https://wiki.archlinux.org/title/font_configuration
 *
 * The fonts array here will only be read once when the fonts are initially loaded.
 */
static const char *fonts[]          = { "monospace:size=10" };
/* This specifies the font used for dmenu when called via dwm. */
static const char dmenufont[]       = "monospace:size=10";

/* The variables here are merely intended to give a names to the colour codes.
 *
 * A very common misunderstanding of this is new starters making changes to the colour codes
 * directly to change the appearance of the bar. E.g.
 *
 *    static const char col_gray1[] = "#E35A00";
 *
 * This will change the background colour for the bar, but that colour will no longer be gray as
 * the variable name suggests.
 *
 * The intention is that you name your own variables, e.g.
 *
 *    static const char col_orange[] = "#E35A00";
 *
 * And use that variable in the colors array, e.g.
 *
 *    [SchemeNorm] = { col_gray3, col_orange, col_gray2 },
 *
 * It is also possible to have these colour codes inline, e.g.
 *
 *    [SchemeNorm] = { "#bbbbbb", "#E35A00", "#444444" },
 *
 * Another approach is to use more generic names like normfgcolor, normbgcolor, etc. and
 * leave the colors array as-is when changing colours. This is particularly used in relation to
 * Xresources.
 */
static const char col_gray1[]       = "#222222";
static const char col_gray2[]       = "#444444";
static const char col_gray3[]       = "#bbbbbb";
static const char col_gray4[]       = "#eeeeee";
static const char col_cyan[]        = "#005577";
static const char *colors[][3]      = {
	/*               fg         bg         border   */
	[SchemeNorm] = { col_gray3, col_gray1, col_gray2 },
	[SchemeSel]  = { col_gray4, col_cyan,  col_cyan  },
};

/* These define the tag icons (or text) used in the bar while the number of strings in the array
 * determine the number of tags being used by dwm. This has an upper limit of 32 tags and anything
 * above that will result in a compilation error. */
static const char *tags[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };

/* This array controls the client rules which consists of three rule matching filters (the class,
 * instance and title) and three rule options (tags, whether the client is floating or not and the
 * monitor it is supposed to start on).
 *
 * Refer to the writeup of the applyrules function for more details on this.
 */
static const Rule rules[] = {
	/* xprop(1):
	 *	WM_CLASS(STRING) = instance, class
	 *	WM_NAME(STRING) = title
	 */
	/* class      instance    title       tags mask     isfloating   monitor */
	{ "Gimp",     NULL,       NULL,       0,            1,           -1 },
	{ "Firefox",  NULL,       NULL,       1 << 8,       0,           -1 },
};

/* layout(s) */

/* The master / stack factor controls how much of the window area is designated for the master area
 * vs the stack area for the tile layout. Refer to the writeup for the setmfact function for more
 * details. */
static const float mfact     = 0.55; /* factor of master area size [0.05..0.95] */
/* The nmaster variable controls the number of clients that are placed in the master area when
 * tiled. Refer to the incnmaster function writeup for more details. */
static const int nmaster     = 1;    /* number of clients in master area */
/* This controls whether or not the window manager will respect the size hints of a client window
 * when the client is tiled. Refer to the applysizehints function writeup for more details. */
static const int resizehints = 1;    /* 1 means respect size hints in tiled resizals */
/* The lockfullscreen variable controls whether or not focus is allowed to drift from a fullscreen
 * window. Refer to the writeup of the focusstack function for which this feature is isolated. */
static const int lockfullscreen = 1; /* 1 will force focus on the fullscreen window */

/* This array contains the list of available layout options.
 *
 * When dwm starts the first layout in the list is the default layout and the last layout in the
 * array will be set as the previous layout.
 *
 * The layout symbol will be copied into the monitor's layout symbol when the layout is set. The
 * layout function may make changes to the layout symbol, for example the monocle layout that shows
 * the number of clients visible.
 *
 * Refer to the setlayout function writeup for more details.
 */
static const Layout layouts[] = {
	/* symbol     arrange function */
	{ "[]=",      tile },    /* first entry is default */
	{ "><>",      NULL },    /* no layout function means floating behavior */
	{ "[M]",      monocle },
};

/* key definitions */

/* This defines the primary modifier used by dwm. It is a macro which means that at compile time
 * all the references to MODKEY below will be replaced with the content of this macro.
 *
 * To see the available modifiers run the xmodmap command in a terminal, but typically there will
 * be a setup along the lines of:
 *
 *    Mod1Mask - the Alt key (and/or Meta key)
 *    Mod2Mask - Num_Lock
 *    Mod3Mask - often not used
 *    Mod4Mask - the Super / Windows key (and/or Hyper key)
 *    Mod5Mask - ISO_Level3_Shift (AltGr) and/or Mode_switch
 *
 * Note that you can use xmodmap to change e.g. the right control key to become another
 * modifier should you need it.
 */
#define MODKEY Mod1Mask

/* TAGKEYS is another macro that just avoids having to repeat the same thing nine times
 * for each tag.
 *
 * Consider this being used in the keys array further down.
 *
 *    TAGKEYS(                        XK_3,                      2)
 *
 * In this case the KEY variable will be XK_3 and the TAG value will be 2. This would then
 * expand in the keys array to:
 *
 *    { MODKEY,                       XK_3,     view,           {.ui = 1 << 2} }, \
 *    { MODKEY|ControlMask,           XK_3,     toggleview,     {.ui = 1 << 2} }, \
 *    { MODKEY|ShiftMask,             XK_3,     tag,            {.ui = 1 << 2} }, \
 *    { MODKEY|ControlMask|ShiftMask, XK_3,     toggletag,      {.ui = 1 << 2} },
 *
 * Using a macro also makes it easier to change the modifiers used for the functions
 * if need be.
 */
#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
	{ MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} },

/* Helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
/* The dmenumon variable holds a reference to the current monitor number, to be passed to dmenu.
 * This is quite strictly not necessary as dmenu can work out on its own what monitor has focus.
 * Refer to the writeup in the spawn function for more details on this. */
static char dmenumon[2] = "0"; /* component of dmenucmd, manipulated in spawn() */
/* The command to launch dmenu. dmenu is a simple program that takes a series of options as input
 * and presents these to the user via a menu, when the user selects an option then that option is
 * printed to standard out. dmenu is often confused with dmenu_run, which is a shell script that
 * looks for executable commands, presents these options to the user, and runs whatever the user
 * selected.
 *
 * In the dmenu command we specify via command line arguments the font and colours that dmenu
 * should use. This is to make it appear stylistically similar to the bar in dwm.
 */
static const char *dmenucmd[] = { "dmenu_run", "-m", dmenumon, "-fn", dmenufont, "-nb", col_gray1, "-nf", col_gray3, "-sb", col_cyan, "-sf", col_gray4, NULL };
/* dwm launches st as the terminal of choice by default. */
static const char *termcmd[]  = { "st", NULL };

/* The keys array contains user defined keybindings and the functions that said keybindings should
 * run. Refer to the grabkeys function for details on how the window manager tells the X server
 * it is interested in receiving key press events corresponding to the given key combinations.
 * Refer to the keypress function for details on how the window manager interprets the events
 * received for the key combinations and calls the designated functions. */
static const Key keys[] = {
	/* modifier                     key        function        argument */
	{ MODKEY,                       XK_p,      spawn,          {.v = dmenucmd } },
	{ MODKEY|ShiftMask,             XK_Return, spawn,          {.v = termcmd } },
	{ MODKEY,                       XK_b,      togglebar,      {0} },
	{ MODKEY,                       XK_j,      focusstack,     {.i = +1 } },
	{ MODKEY,                       XK_k,      focusstack,     {.i = -1 } },
	{ MODKEY,                       XK_i,      incnmaster,     {.i = +1 } },
	{ MODKEY,                       XK_d,      incnmaster,     {.i = -1 } },
	{ MODKEY,                       XK_h,      setmfact,       {.f = -0.05} },
	{ MODKEY,                       XK_l,      setmfact,       {.f = +0.05} },
	{ MODKEY,                       XK_Return, zoom,           {0} },
	{ MODKEY,                       XK_Tab,    view,           {0} },
	{ MODKEY|ShiftMask,             XK_c,      killclient,     {0} },
	{ MODKEY,                       XK_t,      setlayout,      {.v = &layouts[0]} },
	{ MODKEY,                       XK_f,      setlayout,      {.v = &layouts[1]} },
	{ MODKEY,                       XK_m,      setlayout,      {.v = &layouts[2]} },
	{ MODKEY,                       XK_space,  setlayout,      {0} },
	{ MODKEY|ShiftMask,             XK_space,  togglefloating, {0} },
	{ MODKEY,                       XK_0,      view,           {.ui = ~0 } },
	{ MODKEY|ShiftMask,             XK_0,      tag,            {.ui = ~0 } },
	{ MODKEY,                       XK_comma,  focusmon,       {.i = -1 } },
	{ MODKEY,                       XK_period, focusmon,       {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_comma,  tagmon,         {.i = -1 } },
	{ MODKEY|ShiftMask,             XK_period, tagmon,         {.i = +1 } },
	TAGKEYS(                        XK_1,                      0)
	TAGKEYS(                        XK_2,                      1)
	TAGKEYS(                        XK_3,                      2)
	TAGKEYS(                        XK_4,                      3)
	TAGKEYS(                        XK_5,                      4)
	TAGKEYS(                        XK_6,                      5)
	TAGKEYS(                        XK_7,                      6)
	TAGKEYS(                        XK_8,                      7)
	TAGKEYS(                        XK_9,                      8)
	{ MODKEY|ShiftMask,             XK_q,      quit,           {0} },
};

/* Mouse button definitions.
 * The buttons array contains user defined mouse button bindings and the functions that said
 * bindings should trigger. Refer to the grabbuttons function for details on how the window manager
 * tells the X server it is interested in receiving button press events corresponding to the given
 * modifier + button combinations. An event mask of 0 means no modifier.
 * Refer to the buttonpress function for details on how the window manager interprets the events
 * received for the button presses and calls the designated functions.
 *
 * What the user clicks on can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin,
 * or ClkRootWin.
 *
 * Button1 through Button5 are macros that are defined within the X11 libraries. They simply have
 * values of 1 through 5. To bind additional buttons you can either define them yourself or just
 * use the button value directly. E.g.
 *
 *    #define Button6 6
 *    #define Button7 7
 *    #define Button8 8
 *    #define Button9 9
 *
 *    { ClkClientWin,         MODKEY,         Button8,        myfunc,         {0} },
 * or
 *    { ClkClientWin,         MODKEY,         8,              myfunc,         {0} },
 **/
static const Button buttons[] = {
	/* click                event mask      button          function        argument */
	{ ClkLtSymbol,          0,              Button1,        setlayout,      {0} },
	{ ClkLtSymbol,          0,              Button3,        setlayout,      {.v = &layouts[2]} },
	{ ClkWinTitle,          0,              Button2,        zoom,           {0} },
	{ ClkStatusText,        0,              Button2,        spawn,          {.v = termcmd } },
	{ ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
	{ ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
	{ ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
	{ ClkTagBar,            0,              Button1,        view,           {0} },
	{ ClkTagBar,            0,              Button3,        toggleview,     {0} },
	{ ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
	{ ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
};

