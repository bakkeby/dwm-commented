/* See LICENSE file for copyright and license details.
 *
 * dynamic window manager is designed like any other X client as well. It is
 * driven through handling X events. In contrast to other X clients, a window
 * manager selects for SubstructureRedirectMask on the root window, to receive
 * events about window (dis-)appearance. Only one X connection at a time is
 * allowed to select for this event mask.
 *
 * The event handlers of dwm are organized in an array which is accessed
 * whenever a new event has been fetched. This allows event dispatching
 * in O(1) time.
 *
 * Each child of the root window is called a client, except windows which have
 * set the override_redirect flag. Clients are organized in a linked client
 * list on each monitor, the focus history is remembered through a stack list
 * on each monitor. Each client contains a bit array to indicate the tags of a
 * client.
 *
 * Keys and tagging rules are organized as arrays and defined in config.h.
 *
 * Some general concepts and terminology:
 *
 *    X
 *       The X Window System (X11, or simply X) is a windowing system for bitmap displays and is
 *       common on Unix-like operating systems. This provides the basic framework for a GUI
 *       environment and it handles the drawing and moving of windows as well as the interaction
 *       with a mouse and keyboard.
 *
 *    Xlib
 *       An X Window System protocol client library written in C containing functions for
 *       interacting with an X server.
 *
 *    Window
 *       A window is the graphical representation of a program. This is what you see and interact
 *       with when using your computer.
 *
 *    Window Manager
 *       A window manager is a program that is responsible for controlling the placement and
 *       appearance of windows within a windowing system. It can be part of a desktop environment
 *       or be used on its own.
 *
 *    Desktop Environment
 *       A desktop environment bundles together a variety of components to provide common graphical
 *       user interface elements such as icons, toolbars, wallpapers, and desktop widgets.
 *       Additionally, most desktop environments include a set of integrated applications and
 *       utilities.
 *
 *       Typically desktop environments provide their own window manager and come with a built-in
 *       compositor for handling transparency and special effects when moving or switching between
 *       windows.
 *
 *    Screen
 *       This can refer to a physical display, but in the context of dwm this can also refer to the
 *       visible screen space that is covered by the root window. This spans all physical monitors.
 *
 *    Monitor
 *       In the context of dwm the Monitor represents the drawable area of the screen and holds
 *       properties such as the size and position of the screen as well as per-monitor settings
 *       such as the selected layout, the position of the bar as well as a list of the clients
 *       that are assigned to the given monitor.
 *
 *    Client
 *       In the context of dwm the Client represents a window that is managed by the window
 *       manager. The client holds a series of properties such as the size and position of the
 *       window, various size restrictions, miscellaneous flags indicating state as well as
 *       references to other clients within the same workspace.
 *
 *       A set of clients is represented in form of a linked list, which means that one client has
 *       a reference to the next, and so on.
 *
 *    Layout
 *       In the context of dwm a Layout controls how clients are arranged (tiled) respective to
 *       other visible clients.
 *
 *       Most layouts have a larger "master" area where the main window(s) of interest reside and
 *       one or more "stack" areas where the remaining windows are placed.
 *
 *       Floating clients stay on top of tiled clients and remain in their floating position
 *       regardless of how other clients are tiled.
 *
 *    Workspace
 *       In the context of window managers in general a workspace is what holds (or owns) a window
 *       or a set of windows. Conceptually dwm has one workspace per monitor and controls what
 *       clients are shown on that workspace through the use of tags. Each workspace can have a
 *       different layout.
 *
 *       That dwm has workspaces generally goes unsaid due to the workspace being embedded into the
 *       monitor making it transparent to the user. If in doubt then note how the Monitor struct
 *       holds information that is unrelated to the screen space it occupies, as in it is used for
 *       more than one thing.
 *
 *       For the sake of consistency we will be referring to the monitor rather than the workspace
 *       for all upcoming text.
 *
 *    Tags
 *       In the context of dwm the visibility of clients are handled through the use of tags. In
 *       dwm the number of tags is user defined but limited to be between 1 and 32 tags. The number
 *       of tags will be the same on all monitors, but are controlled separately from monitor to
 *       monitor.
 *
 *       Multiple tags can can be viewed at the same time on a monitor, and a client can be shown
 *       on multiple tags.
 *
 *       The famous pertag patch for dwm enables a separate layout (and other options) to be set on
 *       a per tag basis. It works by keeping a record of what layout etc. is used on each
 *       individual tag. When you switch to another tag the settings for that tag is copied across
 *       to the Monitor. This creates a form of hybrid between workspaces and tags while taking
 *       away from the purity and concept behind tags.
 *
 *    View
 *       In the context of dwm a view is the presentation of clients on a per-monitor basis.
 *
 *    Bar
 *       The bar in dwm shows what tags are being viewed, what layout is used, the name of the
 *       selected client (if any) and a plain text status that will default to "dwm-<version>".
 *
 *    EWMH
 *       The Extended Window Manager Hints is an X Window System standard for communication between
 *       window managers and the windows they manage. These standards formulate protocols for the
 *       mediation of access to shared X resources. Communication occurs via X properties and
 *       client messages. The EWMH is a comprehensive set of protocols to implement a desktop
 *       environment.
 *
 *       dwm only supports a handful of EWMH properties and client messages out of the box.
 *
 *    The event loop
 *       In X everything is event driven. What this means is that when you click on a window then
 *       that will generate an event that is propagated back to the window manager via the X server.
 *
 *       The window manager tells the X server what types of events it is interested in receiving
 *       as well as what form of button or key presses the window manager is listening for.
 *
 *       When a window wants to go into fullscreen then it sends a client message event indicating
 *       its desire to go into fullscreen. When you move a mouse cursor over a window then the
 *       window manager will receive a notification that the mouse cursor crossed over from one
 *       window to another. When a new window is created the window manager will receive an event
 *       to that effect, the same goes when a window is closed.
 *
 *       When you hit the keybinding to focus on the next client then that will result in a key
 *       press event because the window manager told the X server that it was interested in getting
 *       such an event when the user hit MOD+k. When handling that event the window manager works
 *       out that it is associated with the focusstack function and calls that accordingly. When
 *       that action is complete it falls back to looking for new events to process.
 *
 *       Once dwm hits the run function every action or operation that the window manager does
 *       after that is reacting to events received by the X server.
 *
 * To understand everything else, start reading main() as that is what sets everything up before
 * entering the event loop.
 *
 * @see https://www.cl.cam.ac.uk/~mgk25/ucs/icccm.pdf
 *
 * Below is a high level call stack for dwm:
 *
 *    main
 *    ├── checkotherwm
 *    ├── die
 *    ├── setup
 *    │  ├── ecalloc
 *    │  │  └── die
 *    │  ├── updategeom
 *    │  │  ├── ecalloc
 *    │  │  │  └── die
 *    │  │  ├── createmon
 *    │  │  │  └── ecalloc
 *    │  │  │     └── die
 *    │  │  ├── wintomon
 *    │  │  │  ├── wintoclient
 *    │  │  │  ├── rectomon
 *    │  │  │  └── getrootptr
 *    │  │  ├── updatebarpos
 *    │  │  └── isuniquegeom
 *    │  ├── drw_create
 *    │  │  └── ecalloc
 *    │  ├── drw_scm_create
 *    │  │  ├── drw_clr_create
 *    │  │  │  └── die
 *    │  │  └── ecalloc
 *    │  ├── drw_cur_create
 *    │  │  └── ecalloc
 *    │  ├── grabkeys
 *    │  │  └── updatenumlockmask
 *    │  ├── updatebars
 *    │  ├── focus
 *    │  └── drw_fontset_create
 *    │     └── xfont_create
 *    │        └── ecalloc
 *    ├── scan
 *    │  └── getstate
 *    ├── run
 *    │  ├── propertynotify
 *    │  │  ├── wintoclient
 *    │  │  ├── updatewmhints
 *    │  │  ├── updatetitle
 *    │  │  │  └── gettextprop
 *    │  │  ├── updatestatus
 *    │  │  │  ├── gettextprop
 *    │  │  │  └── drawbar
 *    │  │  ├── updatewindowtype
 *    │  │  │  └── getatomprop
 *    │  │  ├── drawbar
 *    │  │  ├── drawbars
 *    │  │  │  └── drawbar
 *    │  │  └── arrange
 *    │  ├── unmapnotify
 *    │  │  ├── wintoclient
 *    │  │  ├── unmanage
 *    │  │  │  ├── updateclientlist
 *    │  │  │  ├── setclientstate
 *    │  │  │  ├── focus
 *    │  │  │  ├── detachstack
 *    │  │  │  ├── detach
 *    │  │  │  └── arrange
 *    │  │  └── setclientstate
 *    │  ├── enternotify
 *    │  │  ├── wintoclient
 *    │  │  ├── wintomon
 *    │  │  │  ├── wintoclient
 *    │  │  │  └── recttomon
 *    │  │  ├── unfocus
 *    │  │  └── focus
 *    │  ├── destroynotify
 *    │  │  ├── wintoclient
 *    │  │  └── unmanage
 *    │  │     ├── updateclientlist
 *    │  │     ├── setclientstate
 *    │  │     ├── focus
 *    │  │     ├── detachstack
 *    │  │     ├── detach
 *    │  │     └── arrange
 *    │  ├── maprequest
 *    │  │  ├── manage
 *    │  │  │  ├── ecalloc
 *    │  │  │  │  └── die
 *    │  │  │  ├── wintoclient
 *    │  │  │  ├── updatewmhints
 *    │  │  │  ├── updatesizehints
 *    │  │  │  ├── grabbuttons
 *    │  │  │  │  └── updatenumlockmask
 *    │  │  │  ├── unfocus
 *    │  │  │  ├── setclientstate
 *    │  │  │  ├── updatetitle
 *    │  │  │  │  └── gettextprop
 *    │  │  │  ├── updatewindowtype
 *    │  │  │  │  └── getatomprop
 *    │  │  │  ├── focus
 *    │  │  │  ├── configure
 *    │  │  │  ├── attachstack
 *    │  │  │  ├── attach
 *    │  │  │  ├── arrange
 *    │  │  │  └── applyrules
 *    │  │  └── wintoclient
 *    │  ├── clientmessage
 *    │  │  ├── wintoclient
 *    │  │  ├── seturgent
 *    │  │  └── setfullscreen
 *    │  │     ├── resizeclient
 *    │  │     └── arrange
 *    │  ├── configurerequest
 *    │  │  ├── wintoclient
 *    │  │  └── configure
 *    │  ├── buttonpress
 *    │  │  ├── wintoclient
 *    │  │  ├── wintomon
 *    │  │  │  ├── wintoclient
 *    │  │  │  └── recttomon
 *    │  │  ├── unfocus
 *    │  │  ├── spawn
 *    │  │  │  └── die
 *    │  │  ├── togglefloating
 *    │  │  │  ├── resize
 *    │  │  │  └── arrange
 *    │  │  ├── resizemouse
 *    │  │  │  ├── sendmon
 *    │  │  │  │  ├── unfocus
 *    │  │  │  │  ├── focus
 *    │  │  │  │  ├── detachstack
 *    │  │  │  │  ├── detach
 *    │  │  │  │  ├── attachstack
 *    │  │  │  │  ├── attach
 *    │  │  │  │  └── arrange
 *    │  │  │  ├── resize
 *    │  │  │  ├── recttomon
 *    │  │  │  ├── maprequest
 *    │  │  │  ├── focus
 *    │  │  │  ├── expose
 *    │  │  │  ├── restack
 *    │  │  │  │  └── drawbar
 *    │  │  │  ├── configurerequest
 *    │  │  │  └── togglefloating
 *    │  │  │     └── arrange
 *    │  │  ├── movemouse
 *    │  │  │  ├── sendmon
 *    │  │  │  │  ├── unfocus
 *    │  │  │  │  ├── focus
 *    │  │  │  │  ├── detachstack
 *    │  │  │  │  ├── detach
 *    │  │  │  │  ├── attachstack
 *    │  │  │  │  ├── attach
 *    │  │  │  │  └── arrange
 *    │  │  │  ├── resize
 *    │  │  │  ├── recttomon
 *    │  │  │  ├── maprequest
 *    │  │  │  ├── getrootptr
 *    │  │  │  ├── focus
 *    │  │  │  ├── expose
 *    │  │  │  ├── restack
 *    │  │  │  │  └── drawbar
 *    │  │  │  ├── configurerequest
 *    │  │  │  └── togglefloating
 *    │  │  │     └── arrange
 *    │  │  ├── view
 *    │  │  │  ├── focus
 *    │  │  │  └── arrange
 *    │  │  ├── toggleview
 *    │  │  │  ├── focus
 *    │  │  │  └── arrange
 *    │  │  ├── toggletag
 *    │  │  │  ├── focus
 *    │  │  │  └── arrange
 *    │  │  ├── tag
 *    │  │  │  ├── focus
 *    │  │  │  └── arrange
 *    │  │  ├── focus
 *    │  │  ├── setlayout
 *    │  │  │  └── drawbar
 *    │  │  ├── restack
 *    │  │  │  └── drawbar
 *    │  │  └── drw_fontset_getwidth
 *    │  │     └── drw_text
 *    │  │        ├── die
 *    │  │        ├── drw_font_getexts
 *    │  │        └── xfont_free
 *    │  ├── configurenotify
 *    │  │  ├── updategeom
 *    │  │  │  ├── ecalloc
 *    │  │  │  │  └── die
 *    │  │  │  ├── createmon
 *    │  │  │  │  └── ecalloc
 *    │  │  │  │     └── die
 *    │  │  │  ├── wintomon
 *    │  │  │  │  ├── wintoclient
 *    │  │  │  │  ├── rectomon
 *    │  │  │  │  └── getrootptr
 *    │  │  │  ├── updatebarpos
 *    │  │  │  ├── isuniquegeom
 *    │  │  │  ├── detachstack
 *    │  │  │  ├── cleanupmon
 *    │  │  │  ├── attachstack
 *    │  │  │  └── attach
 *    │  │  ├── drw_resize
 *    │  │  ├── updatebars
 *    │  │  ├── resizeclient
 *    │  │  ├── focus
 *    │  │  └── arrange
 *    │  ├── keypress
 *    │  │  ├── focusmon
 *    │  │  │  ├── unfocus
 *    │  │  │  ├── focus
 *    │  │  │  └── dirtomon
 *    │  │  ├── spawn
 *    │  │  │  └── die
 *    │  │  ├── togglefloating
 *    │  │  │  ├── resize
 *    │  │  │  └── arrange
 *    │  │  ├── quit
 *    │  │  ├── zoom
 *    │  │  │  ├── nexttiled
 *    │  │  │  └── pop
 *    │  │  │     ├── focus
 *    │  │  │     ├── detach
 *    │  │  │     ├── attach
 *    │  │  │     └── arrange
 *    │  │  ├── killclient
 *    │  │  │  └── sendevent
 *    │  │  ├── view
 *    │  │  │  ├── focus
 *    │  │  │  └── arrange
 *    │  │  ├── toggleview
 *    │  │  │  ├── focus
 *    │  │  │  └── arrange
 *    │  │  ├── toggletag
 *    │  │  │  ├── focus
 *    │  │  │  └── arrange
 *    │  │  ├── tag
 *    │  │  │  ├── focus
 *    │  │  │  └── arrange
 *    │  │  ├── focusstack
 *    │  │  │  ├── focus
 *    │  │  │  └── restack
 *    │  │  │     └── drawbar
 *    │  │  ├── setlayout
 *    │  │  │  ├── drawbar
 *    │  │  │  └── arrange
 *    │  │  ├── tagmon
 *    │  │  │  ├── sendmon
 *    │  │  │  │  ├── unfocus
 *    │  │  │  │  ├── focus
 *    │  │  │  │  ├── detachstack
 *    │  │  │  │  ├── detach
 *    │  │  │  │  ├── attachstack
 *    │  │  │  │  ├── attach
 *    │  │  │  │  └── arrange
 *    │  │  │  └── dirtomon
 *    │  │  ├── togglebar
 *    │  │  │  └── arrange
 *    │  │  ├── setmfact
 *    │  │  │  └── arrange
 *    │  │  └── incnmaster
 *    │  │     └── arrange
 *    │  ├── focusin
 *    │  │  └── setfocus
 *    │  │     └── sendevent
 *    │  ├── expose
 *    │  │  ├── wintomon
 *    │  │  │  ├── wintoclient
 *    │  │  │  └── recttomon
 *    │  │  └── drawbar
 *    │  ├── motionnotify
 *    │  │  ├── recttomon
 *    │  │  └── focus
 *    │  ├── scan
 *    │  │  └── manage
 *    │  │     └── ecalloc
 *    │  │        └── die
 *    │  ├── mappingnotify
 *    │  │  └── grabkeys
 *    │  │     └── updatenumlockmask
 *    │  ├── setup
 *    │  │  └── updatestatus
 *    │  │     ├── gettextprop
 *    │  │     └── drawbar
 *    │  └── updatewindowtype
 *    │     └── setfullscreen
 *    │        ├── resizeclient
 *    │        └── arrange
 *    └── cleanup
 *       ├── unmanage
 *       │  └── updateclientlist
 *       ├── view
 *       │  ├── focus
 *       │  └── arrange
 *       ├── cleanupmon
 *       ├── drw_cur_free
 *       └── drw_free
 *          └── drw_fontset_free
 *             ├── drw_fontset_free
 *             └── xfont_free
 *
 * The above skips details for commonly called functions like drawbar, resize, arrange, focus and
 * unfocus.
 */
#include <errno.h>
#include <locale.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif /* XINERAMA */
#include <X11/Xft/Xft.h>

#include "drw.h"
#include "util.h"

/* macros */

/* The BUTTONMASK macro is used as part of the MOUSEMASK macro and it is directly used in the
 * grabbuttons function. It indicates that we are interested in receiving events both when a mouse
 * button is pressed and when it is released. */
#define BUTTONMASK              (ButtonPressMask|ButtonReleaseMask)
/* The CLEANMASK macro removes Num Lock mask and Lock mask from a given bit mask.
 * Refer to the numlockmask variable comment for more details on the Num Lock mask.
 * The CLEANMASK macro is used in the keypress and buttonpress functions. */
#define CLEANMASK(mask)         (mask & ~(numlockmask|LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
/* Calculates how much a monitor's window area intersects with a given size and position.
 * See the writeup in the recttomon function for more information on this. */
#define INTERSECT(x,y,w,h,m)    (MAX(0, MIN((x)+(w),(m)->wx+(m)->ww) - MAX((x),(m)->wx)) \
                               * MAX(0, MIN((y)+(h),(m)->wy+(m)->wh) - MAX((y),(m)->wy)))
/* This macro returns true if any of the given client's tags is on any of the tags currently being
 * viewed on the monitor. */
#define ISVISIBLE(C)            ((C->tags & C->mon->tagset[C->mon->seltags]))
/* This calculates the number of items of an array. */
#define LENGTH(X)               (sizeof X / sizeof X[0])
/* The MOUSEMASK macro is used in the movemouse and resizemouse user functions and it indicates
 * that we are interested in receiving events when the mouse cursor moves in addition to when the
 * button is released. */
#define MOUSEMASK               (BUTTONMASK|PointerMotionMask)
/* The actual width of a client window includes the border and this macro helps calculate that. */
#define WIDTH(X)                ((X)->w + 2 * (X)->bw)
/* The actual height of a client window includes the border and this macro helps calculate that. */
#define HEIGHT(X)               ((X)->h + 2 * (X)->bw)
/* The TAGMASK macro gives a binary value that represents a valid bitmask according to how many
 * tags are defined.
 *
 * As an example dwm by default comes with nine tags and the bitmask is a 32 bit integer. In this
 * case the TAGMASK macro would return a binary value like this:
 *
 *    00000000000000000000000111111111
 *
 * but if the configuration was changed so that there are icons defined for four tags then the
 * TAGMASK macro would return a binary value like this:
 *
 *    00000000000000000000000000001111
 *
 * The TAGMASK is used in various places to restrict and to validate bitmask values used in the
 * context of what tags are viewed by the monitor and what tags are assigned to a client.
 */
#define TAGMASK                 ((1 << LENGTH(tags)) - 1)
/* The TEXTW macro returns the width of a given text string plus the left and right padding.
 *
 * Due to that not all fonts have every glyph and we have a primary font and fallback fonts this
 * macro calls drw_fontset_getwidth which does the exact same thing as when text is drawn, just
 * that it returns how far the cursor has moved rather than actually drawing the text. */
#define TEXTW(X)                (drw_fontset_getwidth(drw, (X)) + lrpad)

/* Enumerators (enums) are user defined data types in C. They are mainly used to assign names to
 * integral constants to make a program easier to read and maintain.
 *
 * As an example the below cursor enum would result in CurNormal having a value of 0, CurResize
 * having a value of 1 and CurMove having a value of 2. Often an additional constant is added to
 * an enum representing the last of the given type, e.g. CurLast having a value of 3.
 *
 * The last value is commonly used when needing to looping through the various options / constants.
 * It is possible to specify the value of each constant, but this is not used here.
 */
enum { CurNormal, CurResize, CurMove, CurLast }; /* cursor */
/* If you need to add your own colour schemes then this is where you would add them. */
enum { SchemeNorm, SchemeSel }; /* color schemes */
/* This represents the various extended window manager hint atoms that dwm supports. */
enum { NetSupported, NetWMName, NetWMState, NetWMCheck,
       NetWMFullscreen, NetActiveWindow, NetWMWindowType,
       NetWMWindowTypeDialog, NetClientList, NetLast }; /* EWMH atoms */
/* This represents various window manager hint atoms that dwm supports. */
enum { WMProtocols, WMDelete, WMState, WMTakeFocus, WMLast }; /* default atoms */
/* This represents the various click options that can be used when defining mouse button press
 * bindings in the buttons array in the configuration file. */
enum { ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle,
       ClkClientWin, ClkRootWin, ClkLast }; /* clicks */

/* In C, a struct(ure) can be thought of as a user defined data type. They define how much space
 * is needed to allocate memory to hold values for each of the variables inside the structure.
 *
 * While arrays allow for variables that can hold several data types of the same kind to be
 * defined, a structure allows for data items of different kinds to be defined.
 *
 * A union is conceptually similar to structures, with the difference being that of memory
 * allocation. In a structure each variable (also referred to as a member) has allocated space
 * whereas in a union all the variables share the same memory. The Arg type that are passed to
 * user functions is a union which means that the argument can be an integer, an unsigned integer,
 * a float value or a pointer, but the argument can only hold a single value.
 */
typedef union {
	int i;
	unsigned int ui;
	float f;
	const void *v;
} Arg;

/* The definition of a button, used in the configuration file when setting up mouse button
 * bindings.
 *
 * static Button buttons[] = {
 *    // click                event mask      button          function        argument
 *    { ClkLtSymbol,          0,              Button1,        setlayout,      {0} },
 *    { ClkLtSymbol,          0,              Button3,        setlayout,      {.v = &layouts[2]} },
 *    ...
 * };
 */
typedef struct {
	unsigned int click;
	unsigned int mask;
	unsigned int button;
	void (*func)(const Arg *arg);
	const Arg arg;
} Button;

/* Here we are declaring that we are going to have a struct called Monitor without going into
 * details of what exactly this struct looks like. This is because the Client struct refers to
 * the current monitor, while the monitor is going to have a list of clients. We define these
 * structs alphabetically thus the Client is defined before the Monitor. */
typedef struct Monitor Monitor;
typedef struct Client Client;

/* The Client struct represents a window that is managed by the window manager. */
struct Client {
	/* The name holds the window title. */
	char name[256];
	/* The mina and maxa represents the minimum and maximum aspect ratios as per size hints. */
	float mina, maxa;
	/* The client x, y coordinates and size (width, height). */
	int x, y, w, h;
	/* These variables represent the client's previous size and position and they are maintained
	 * in the resizeclient function. In practice in a stock dwm they are only used when a
	 * fullscreen window exits fullscreen. */
	int oldx, oldy, oldw, oldh;
	/* These variables are all in relation to size hints.
	 *    basew - base width
	 *    baseh - base height
	 *    incw - width increment
	 *    inch - height increment
	 *    minw - minimum width
	 *    minh - minimum height
	 *    maxw - maximum width
	 *    maxh - maximum height
	 *    hintsvalid - flag indicating whether size hints need to be refreshed
	 */
	int basew, baseh, incw, inch, maxw, maxh, minw, minh, hintsvalid;
	/* Border width (bw) and old border width. The old border width is set in the manage
	 * function and is used in the unmanage function in the event that the window was not
	 * destroyed. The setfullscreen function also relies on this variable. See comment in the
	 * unmanage function. */
	int bw, oldbw;
	/* This represents the tags the client is shown on. This is a bitmask where each bit
	 * represents whether the client is shown on that tag.
	 *
	 * As an example consider the hexadecimal value of 0x51 (decimal 81) which has a binary
	 * value of:
	 *    001010001  - bitmask
	 *    987654321  - tags
	 *
	 * This would mean that the client is shown on tags 1, 5 and 7.
	 */
	unsigned int tags;
	/* Various status flags.
	 *    isfixed      - means that the client is fixed in size due to minimum and maximum size
	 *                   hints being the same value
	 *    isfloating   - indicates whether the client is floating or not
	 *    isurgent     - indicates whether the client is urgent or not
	 *    neverfocus   - some windows do not want the window manager to give them input focus as
	 *                   they are mere passive windows that do not handle any input, the
	 *                   neverfocus indicates that the client should never receive input focus
	 *                   as indicated by the window manager hints for the window
	 *                   (see updatewmhints and setfocus functions)
	 *    oldstate     - represents the previous state in the event that the client goes into
	 *                   fullscreen, the variable is only used to indicate whether the client
	 *                   was floating or not
	 *    isfullscreen - indicates whether the window is in fullscreen
	 */
	int isfixed, isfloating, isurgent, neverfocus, oldstate, isfullscreen;
	/* The next client in the client list, which is a linked list. The client list controls the
	 * order in which clients are tiled. */
	Client *next;
	/* The next client in the stacking order list, which is also a linked list. The stacking
	 * order indicates which window is on top of others as well as the order in which clients
	 * had focus. */
	Client *snext;
	/* The monitor this client belongs to. */
	Monitor *mon;
	/* The managed window that this client represents. */
	Window win;
};

/* The definition of a key, used in the configuration file when setting up key bindings.
 *
 * static Key keys[] = {
 *    // modifier                     key        function        argument
 *    { MODKEY,                       XK_d,      incnmaster,     {.i = -1 } },
 *    { MODKEY,                       XK_h,      setmfact,       {.f = -0.05} },
 *    ...
 * };
 */
typedef struct {
	unsigned int mod;
	KeySym keysym;
	void (*func)(const Arg *);
	const Arg arg;
} Key;

/* The definition of a layout, used in the configuration file when setting up layouts.
 *
 * static const Layout layouts[] = {
 * 	// symbol     arrange function
 * 	{ "[]=",      tile },    // first entry is default
 * 	{ "><>",      NULL },    // no layout function means floating behavior
 * 	{ "[M]",      monocle },
 * };
 */
typedef struct {
	const char *symbol;
	void (*arrange)(Monitor *);
} Layout;

/* This represents individual monitors (screens) if Xinerama is used, or a single monitor
 * representing the entire screen space if Xinerama is not enabled. */
struct Monitor {
	/* This holds the layout symbol text, typically as defined in the layouts array. This is
	 * used when drawing the layout symbol on the bar. The reason why this is defined for the
	 * monitor rather than simply using the layout symbol as defined in the layouts array is
	 * that some layouts, like the monocle layout for example, may alter the layout symbol
	 * depending on how many clients are present. */
	char ltsymbol[16];
	/* The master / stack factor which controls how big a proportion of the window tiling area
	 * is reserved for the master area compared to the stack area. The default value is
	 * configured in the configuration file and the value is adjusted via the setmfact function.
	 * The value has a range from 0.05 to 0.95. */
	float mfact;
	/* This represents the number of clients that are to be tiled in the master area. This has
	 * no upper limit but cannot be less than 0. The default value is configured in the
	 * configuration file and the value is adjusted via the incnmaster function. */
	int nmaster;
	/* This represents the monitor number, or the monitor index if you wish. */
	int num;
	/* The by variable defines the bar windows position on the y axis and this is set in the
	 * updatebarpos function. */
	int by;               /* bar geometry */
	/* These variables represents the position and dimensions of the monitor.
	 *    mx - monitor position on the x-axis
	 *    my - monitor position on the y-axis
	 *    mw - the monitor's width
	 *    mh - the monitor's height
	 */
	int mx, my, mw, mh;   /* screen size */
	/* These variables represents the position and dimensions of the window area, as in the part
	 * of the monitor where windows are tiled. This is the space of the monitor excluding the
	 * bar window. These are set in the updatebarpos function.
	 *    wx - window area position on the x-axis
	 *    wy - window area position on the y-axis
	 *    ww - the window area's width
	 *    wh - the window area's height
	 */
	int wx, wy, ww, wh;   /* window area  */
	/* The seltags variable is either 0 or 1 and represents the currently selected tagset.
	 *
	 * This allows for a clever mechanism where one can easily flip between the current and
	 * previous tagset by simply flipping the value of seltags:
	 *
	 *    selmon->seltags ^= 1;
	 *
	 * For this reason when referring to the selected tags for a monitor you will often find
	 * these kind of patterns:
	 *
	 *    m->tagset[m->seltags]
	 *    selmon->tagset[selmon->seltags]
	 *    c->mon->tagset[c->mon->seltags]
	 *
	 * In principle this could just have been defined as two variables for the monitor.
	 *
	 *    m->tags
	 *    m->prevtags
	 *
	 * which would make the above patterns slightly easier to read, i.e.
	 *
	 *    m->tags
	 *    selmon->tags
	 *    c->mon->tags
	 *
	 * The benefit of using this mechanism, however, is that we save on a single line of code
	 * in the view function when the argument is 0 and we toggle back to the previous view.
	 */
	unsigned int seltags;
	/* The sellt variable is either 0 or 1 and represents the currently selected layout. This
	 * follows the same mechanism as seltags above giving patterns such a:
	 *
	 *    m->lt[m->sellt]
	 *    selmon->lt[selmon->sellt]
	 *    c->mon->lt[c->mon->sellt]
	 */
	unsigned int sellt;
	/* This array holds the previously and currently viewed tags for the monitor, the index of
	 * which is indicated by the seltags variable. */
	unsigned int tagset[2];
	/* Internal flag indicating whether the bar is shown or not. */
	int showbar;
	/* Internal flag indicating whether the bar is shown at the top or at the bottom. */
	int topbar;
	/* The client list. This represents the start of a linked list of clients which determines
	 * the order in which clients are tiled. */
	Client *clients;
	/* This represents the monitor's selected client. */
	Client *sel;
	/* The stacking order list. This represents the order in which client windows are stacked on
	 * top of each other, as well as the order in which clients had last focus. */
	Client *stack;
	/* Monitors are also managed as a linked list with the mons variable referring to the first
	 * monitor. The next variable on the monitor refers to the next monitor in the list. */
	Monitor *next;
	/* This is the bar window which is used to draw the bar. Each monitor has their own bar. */
	Window barwin;
	/* This array holds the previous and current layout for the monitor, the index of which is
	 * indicated by the sellt variable. */
	const Layout *lt[2];
};

/* The definition of a rule, used in the configuration file when setting up client rules.
 *
 * static const Rule rules[] = {
 *    // xprop(1):
 *    //    WM_CLASS(STRING) = instance, class
 *    //    WM_NAME(STRING) = title
 *    //
 *    // class      instance    title       tags mask     isfloating   monitor
 *    { "Gimp",     NULL,       NULL,       0,            1,           -1 },
 *    { "Firefox",  NULL,       NULL,       1 << 8,       0,           -1 },
 * };
 *
 * See the applyrules function for how the rules are applied.
 */
typedef struct {
	const char *class;
	const char *instance;
	const char *title;
	unsigned int tags;
	int isfloating;
	int monitor;
} Rule;

/* Function declarations. All functions are declared for visibility and overview reasons. The
 * declarations as well as the functions themselves are sorted alphabetically so that they should
 * be easier to find and maintain. */
static void applyrules(Client *c);
static int applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact);
static void arrange(Monitor *m);
static void arrangemon(Monitor *m);
static void attach(Client *c);
static void attachstack(Client *c);
static void buttonpress(XEvent *e);
static void checkotherwm(void);
static void cleanup(void);
static void cleanupmon(Monitor *mon);
static void clientmessage(XEvent *e);
static void configure(Client *c);
static void configurenotify(XEvent *e);
static void configurerequest(XEvent *e);
static Monitor *createmon(void);
static void destroynotify(XEvent *e);
static void detach(Client *c);
static void detachstack(Client *c);
static Monitor *dirtomon(int dir);
static void drawbar(Monitor *m);
static void drawbars(void);
static void enternotify(XEvent *e);
static void expose(XEvent *e);
static void focus(Client *c);
static void focusin(XEvent *e);
static void focusmon(const Arg *arg);
static void focusstack(const Arg *arg);
static Atom getatomprop(Client *c, Atom prop);
static int getrootptr(int *x, int *y);
static long getstate(Window w);
static int gettextprop(Window w, Atom atom, char *text, unsigned int size);
static void grabbuttons(Client *c, int focused);
static void grabkeys(void);
static void incnmaster(const Arg *arg);
static void keypress(XEvent *e);
static void killclient(const Arg *arg);
static void manage(Window w, XWindowAttributes *wa);
static void mappingnotify(XEvent *e);
static void maprequest(XEvent *e);
static void monocle(Monitor *m);
static void motionnotify(XEvent *e);
static void movemouse(const Arg *arg);
static Client *nexttiled(Client *c);
static void pop(Client *c);
static void propertynotify(XEvent *e);
static void quit(const Arg *arg);
static Monitor *recttomon(int x, int y, int w, int h);
static void resize(Client *c, int x, int y, int w, int h, int interact);
static void resizeclient(Client *c, int x, int y, int w, int h);
static void resizemouse(const Arg *arg);
static void restack(Monitor *m);
static void run(void);
static void scan(void);
static int sendevent(Client *c, Atom proto);
static void sendmon(Client *c, Monitor *m);
static void setclientstate(Client *c, long state);
static void setfocus(Client *c);
static void setfullscreen(Client *c, int fullscreen);
static void setlayout(const Arg *arg);
static void setmfact(const Arg *arg);
static void setup(void);
static void seturgent(Client *c, int urg);
static void showhide(Client *c);
static void spawn(const Arg *arg);
static void tag(const Arg *arg);
static void tagmon(const Arg *arg);
static void tile(Monitor *m);
static void togglebar(const Arg *arg);
static void togglefloating(const Arg *arg);
static void toggletag(const Arg *arg);
static void toggleview(const Arg *arg);
static void unfocus(Client *c, int setfocus);
static void unmanage(Client *c, int destroyed);
static void unmapnotify(XEvent *e);
static void updatebarpos(Monitor *m);
static void updatebars(void);
static void updateclientlist(void);
static int updategeom(void);
static void updatenumlockmask(void);
static void updatesizehints(Client *c);
static void updatestatus(void);
static void updatetitle(Client *c);
static void updatewindowtype(Client *c);
static void updatewmhints(Client *c);
static void view(const Arg *arg);
static Client *wintoclient(Window w);
static Monitor *wintomon(Window w);
static int xerror(Display *dpy, XErrorEvent *ee);
static int xerrordummy(Display *dpy, XErrorEvent *ee);
static int xerrorstart(Display *dpy, XErrorEvent *ee);
static void zoom(const Arg *arg);

/* variables */
/* This is used as a fallback text for windows that do not have a windwow title / name set */
static const char broken[] = "broken";
/* This array of characters holds the status text */
static char stext[256];
/* This holds the default screen value, used when creating windows and handling the display etc. */
static int screen;
static int sw, sh;           /* X display screen geometry width, height */
static int bh;               /* bar height */
static int lrpad;            /* sum of left and right padding for text */

/* This is the reference we store the X error handler in and it is used in the xerror function for
 * any errors that are not simply ignored. Search the code for xerrorxlib to see how this set and
 * used. */
static int (*xerrorxlib)(Display *, XErrorEvent *);

/* The static global numlockmask variable that holds the modifier that is related to the
 * Num Lock key.
 *
 * This is set in the updatenumlockmask function based on the presence of the Num Lock key in the
 * modifier map. This is then later used to subscribe to KeyPress and ButtonPress events involving
 * the Num Lock keys in the grabkeys and grabbuttons functions.
 *
 * Additionally the variable is used in the CLEANMASK macro which omits the Num Lock and Caps Lock
 * modifiers from a given modifier mask. This macro is then used in both the buttonpress and
 * keypress functions to ignore Num Lock and Caps Lock when looking for matching keypress or button
 * press combinations.
 */
static unsigned int numlockmask = 0;

/* This is what maps event types to the functions that handles those event types.
 *
 * This is primarily used in the run function which handles the event loop, but it is also used
 * in the movemouse and resizemouse functions that temporarily hooks into the event loop for a few
 * select event types while client windows are being moved or resized.
 */
static void (*handler[LASTEvent]) (XEvent *) = {
	[ButtonPress] = buttonpress,
	[ClientMessage] = clientmessage,
	[ConfigureRequest] = configurerequest,
	[ConfigureNotify] = configurenotify,
	[DestroyNotify] = destroynotify,
	[EnterNotify] = enternotify,
	[Expose] = expose,
	[FocusIn] = focusin,
	[KeyPress] = keypress,
	[MappingNotify] = mappingnotify,
	[MapRequest] = maprequest,
	[MotionNotify] = motionnotify,
	[PropertyNotify] = propertynotify,
	[UnmapNotify] = unmapnotify
};
/* This initialises the wmatom and netatom arrays which holds X atom references */
static Atom wmatom[WMLast], netatom[NetLast];
/* The global running variable indicates whether the window manager is running. When set to 0 then
 * the window manager will exit out of the event loop in the run function and make dwm exit the
 * process gracefully. */
static int running = 1;
/* This holds the various mouse cursor types used by the window manager. */
static Cur *cursor[CurLast];
/* This holds a reference to the array of colour schemes. */
static Clr **scheme;
/* This holds a reference to the display that we have opened. */
static Display *dpy;
/* This holds a reference to the drawable that we have created. Refer to the struct definition in
 * the drw.h file. */
static Drw *drw;
/* Two references to hold the first and the selected monitor. */
static Monitor *mons, *selmon;
/* Two window references, one for the root window and one for the supporting window. More on the
 * latter in the setup function. */
static Window root, wmcheckwin;

/* Configuration, allows nested code to access above variables */
#include "config.h"

/* Compile-time check if all tags fit into an unsigned int bit array. This causes a compilation
 * error if the user has added more than 31 entries in the tags array. This NumTags struct does
 * not actually cost anything because the compiler is free to discard it as it is not used by
 * anything. */
struct NumTags { char limitexceeded[LENGTH(tags) > 31 ? -1 : 1]; };

/* Function implementations. Functions are ordered alphabetically and function names always
 * start on a new line to make them easier to find. */

/* This function applies client rules for a client window.
 *
 * Example rules from the default configuration:
 *
 *    static const Rule rules[] = {
 *       // class      instance    title       tags mask     isfloating   monitor
 *       { "Gimp",     NULL,       NULL,       0,            1,           -1 },
 *       { "Firefox",  NULL,       NULL,       1 << 8,       0,           -1 },
 *    };
 *
 * The first three fields are rule matching filters while the last three are rule options. What
 * this means is that a client window must match all of the class, instance and title filters in
 * order to get the tags mask, floating state and monitor options applied.
 *
 * If a rule filter is NULL then it does not apply (e.g. like instance and title filters above).
 *
 * Rule matching uses the strstr function to compare the rule filter to the corresponding class,
 * instance and title values for the window. The strstr function is case sensitive and checks if
 * a substring exists in another string.
 *
 * As an example if a rule has an instance filter of "st" then a client that has an instance name
 * of "Manifesto Pro" will match that rule because the instance name contains "st". Adding more
 * than one filter (e.g. class and instance) may help in these kind of situations.
 *
 * The fields, as well as the order of the fields, are determined by the Rule struct (given that
 * the type of the rules array is Rule).
 *
 *    typedef struct {
 *       const char *class;
 *       const char *instance;
 *       const char *title;
 *       unsigned int tags;
 *       int isfloating;
 *       int monitor;
 *    } Rule;
 *
 * It is worth noting that when some application windows are initially mapped they may have
 * different title, class and instance hints compared to when you run xprop on them. It may be
 * that these hints are changed by the application after dwm has checked the rules. An example
 * of this are LibreOffice programs that come through with class and instance hints of "soffice"
 * due to having a common launcher application named as such.
 *
 * One common misunderstanding when it comes to rules is that the tags mask is a binary mask
 * rather than just a number like 8 to place a client on tag 8. The reason for this is simply due
 * to convenience as all tags handling are binary masks, but also because a user may want to have
 * a rule that places a given client on both tag 5 and tag 7.
 *
 * Also worth noting that in (ANSI) C99 you can use designated initialisers to initialise a
 * structure. What this means is that you can initialise your rules like this:
 *
 *    static const Rule rules[] = {
 *       { .class = "Gimp", .isfloating = 1, .monitor = -1 },
 *       { .class = "Firefox", .tags = 1 << 8, .monitor = -1 },
 *    };
 *
 * Any fields that are not initialised will default to 0. This can be useful when using many
 * patches that add more rule filters or options.
 *
 * @called_from manage to apply client rules for new windows being managed
 * @calls XGetClassHint https://tronche.com/gui/x/xlib/ICC/client-to-window-manager/XGetClassHint.html
 * @calls strstr to look for substring matches in a window's class, instance and title strings
 * @calls XFree https://tronche.com/gui/x/xlib/display/XFree.html
 * @see https://tronche.com/gui/x/xlib/ICC/client-to-window-manager/wm-class.html
 * @see https://dwm.suckless.org/customisation/rules/
 * @see https://dwm.suckless.org/customisation/tagmask/
 *
 * Internal call stack:
 *    run -> maprequest -> manage -> applyrules
 */
void
applyrules(Client *c)
{
	const char *class, *instance;
	unsigned int i;
	const Rule *r;
	Monitor *m;
	/* Placeholder to store the client's class hints in */
	XClassHint ch = { NULL, NULL };

	/* Rule matching */
	c->isfloating = 0;
	c->tags = 0;
	/* This reads the class hint for the client's window. As in this property of
	 * the window:
	 *
	 *    $ xprop | grep WM_CLASS
	 *    WM_CLASS(STRING) = "st", "St"
	 *
	 * The first value is the instance name and the second is the class. In the unlikely
	 * scenario that a window does not have this property set then the class and instance will
	 * default to "broken".
	 */
	XGetClassHint(dpy, c->win, &ch);
	class    = ch.res_class ? ch.res_class : broken;
	instance = ch.res_name  ? ch.res_name  : broken;

	/* This loops through all Rule entries in the rules array. */
	for (i = 0; i < LENGTH(rules); i++) {
		/* The current rule (r) */
		r = &rules[i];
		/* Checking matching filters for class, instance and title. */
		if ((!r->title || strstr(c->name, r->title))
		&& (!r->class || strstr(class, r->class))
		&& (!r->instance || strstr(instance, r->instance)))
		{
			/* This applies the rule options:
			 *    - what monitor the client is to be shown on
			 *    - tags mask
			 *    - whether the client is floating or not
			 */
			c->isfloating = r->isfloating;
			/* Note that this adds rather than sets tags. */
			c->tags |= r->tags;
			/* This loops through all monitors trying to find one that matches the monitor
			 * rule value. If the rule value is -1 then we simply exhaust the list and m
			 * will be NULL and thus not set. */
			for (m = mons; m && m->num != r->monitor; m = m->next);
			if (m)
				c->mon = m;
			/* Note the omission of a break; here. This means that despite having found a
			 * matching client rule we still continue looking for others. In practice what
			 * this means is that the last rule to apply is the one that will take
			 * precedence, while the client's tags will be a union of the tags mask for
			 * all matching rules. Situations where this applies is fairly rare. */
		}
	}

	/* The class and instance names need to be freed (if returned by XGetClassHint). */
	if (ch.res_class)
		XFree(ch.res_class);
	if (ch.res_name)
		XFree(ch.res_name);

	/* This guard checks whether the client is to be shown on a valid tag. If it is not
	 * then we show the client on whatever tag(s) the client's monitor has active. */
	c->tags = c->tags & TAGMASK ? c->tags & TAGMASK : c->mon->tagset[c->mon->seltags];
}

/* This function assesses whether a resize for a window is needed or not considering the window's
 * size hints which adds restrictions on the window's size or how the window is sized.
 *
 * @called_from resize to apply a window's size hints for given dimensions before resizing
 * @calls updatesizehints to read the size hints of a window
 *
 * Internal call stack:
 *    ~ -> resize -> applysizehints
 */
int
applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact)
{
	/* It is worth noting here that the x, y, w, and h parameters are pointers to the variables
	 * that the resize function passes as references. This is because this function manipulates
	 * those values directly. This is also why every use of these variables in this function
	 * has the pointer (*) prefix, e.g. *w.
	 *
	 * If the parameters were normal int variables then the arguments would be passed by value,
	 * meaning that applysizehints would get copies of said values leaving the variables in the
	 * calling resize function untouched. */

	/* Variable used to indicate whether the window is at its minimum size. */
	int baseismin;
	Monitor *m = c->mon;

	/* A window's height and width must be at least 1 pixel, this is merely a safeguard for
	 * values of 0 or below being passed. */
	*w = MAX(1, *w);
	*h = MAX(1, *h);

	/* Here we make a distinction between whether the resize is being done by the system or
	 * interactively by the user.
	 *
	 * The practical difference is that resizes done by the system must leave the window within
	 * the monitor's window area, while we allow the window to be moved freely within the
	 * available screen area.
	 */
	if (interact) {
		/* When the user interacts with a window this checks whether a window is placed
		 * outside of the screen area and restricts how far outside the window can go.
		 * It can be thought of a safeguard to prevent floating windows from becoming
		 * unreachable. The screen height and width naturally reflect a rectangle and the
		 * below does not attempt to prevent windows ending up out of view in Xinerama setups
		 * where the monitors have different heights and widths. */
		if (*x > sw)
			*x = sw - WIDTH(c);
		if (*y > sh)
			*y = sh - HEIGHT(c);
		if (*x + *w + 2 * c->bw < 0)
			*x = 0;
		if (*y + *h + 2 * c->bw < 0)
			*y = 0;
	} else {
		/* When clients are resized by the system in relation to automatic tiling or
		 * otherwise then we restrict the position to be at least partially within the
		 * designated monitor's window area (as in where the windows are tiled, which is the
		 * monitor space less the bar window). In principle these are merely safeguards to
		 * prevent a window from appearing at an unreachable space outside of the window
		 * area. */
		if (*x >= m->wx + m->ww)
			*x = m->wx + m->ww - WIDTH(c);
		if (*y >= m->wy + m->wh)
			*y = m->wy + m->wh - HEIGHT(c);
		if (*x + *w + 2 * c->bw <= m->wx)
			*x = m->wx;
		if (*y + *h + 2 * c->bw <= m->wy)
			*y = m->wy;
	}

	/* This arbitrarily restricts the minimum horizontal and vertical size of a window to be
	 * that of the bar height. */
	if (*h < bh)
		*h = bh;
	if (*w < bh)
		*w = bh;

	/* Now it comes to actually comparing the window's size hints with the new size.
	 * By default we always respect size hints for floating windows, and as such we will enter
	 * the if statement here if the client is floating or we are in floating layout. The
	 * exception to this is tiled windows where we can optionally choose to respect size hints
	 * for tiled windows by leave the resizehints setting in the configuration file as 1, or we
	 * can set that to 0 to explicitly ignore the size hints for tiled clients. */
	if (resizehints || c->isfloating || !c->mon->lt[c->mon->sellt]->arrange) {
		/* We may have received a property notification about changes to size hints since the
		 * last time we resized this client, in which case we will need to get the new hints
		 * before proceeding. */
		if (!c->hintsvalid)
			updatesizehints(c);

		/* See last two sentences in ICCCM 4.1.2.3 which relates to the WM_NORMAL_HINTS
		 * property.
		 *
		 * Ref. https://www.cl.cam.ac.uk/~mgk25/ucs/icccm.pdf we have that:
		 *
		 *    The min_aspect and max_aspect fields are fractions with the numerator first and
		 *    the denominator second, and they allow a client to specify the range of aspect
		 *    ratios it prefers. Window managers that honor aspect ratios should take into
		 *    account the base size in determining the preferred window size. If a base size
		 *    is provided along with the aspect ratio fields, the base size should be
		 *    subtracted from the window size prior to checking that the aspect ratio falls
		 *    in range. If a base size is not provided, nothing should be subtracted from the
		 *    window size. (The minimum size is not to be used in place of the base size for
		 *    this purpose.)
		 *
		 * In simpler terms if the size hints provided both a base size and aspect ratio
		 * hints, then the aspect ratio calculations should be made without taking the base
		 * size into account.
		 *
		 * The same documentation also says that if the base width and height are missing
		 * then one should assume the minimum size and we do so in the updatesizehints
		 * function. But as per the above documentation the minimum size is not to be used
		 * in place of the base size for the purpose of calculating the aspect ratio.
		 *
		 * As such here we check if the base size is the same as the minimum size, in which
		 * case we assume that the base size was not provided as such but that we derived it
		 * from the minimum size hints. */
		baseismin = c->basew == c->minw && c->baseh == c->minh;

		/* As per the notes from ICCCM 4.1.2.3 we remove the base size from the new height
		 * and width before doing the aspect ratio calculations, unless the base size hints
		 * were not provided. */
		if (!baseismin) { /* temporarily remove base dimensions */
			*w -= c->basew;
			*h -= c->baseh;
		}

		/* Here we adjust the width and height based on the aspect ratio limits. First we
		 * check whether we have aspect ratio restrictions. */
		if (c->mina > 0 && c->maxa > 0) {
			/* We only correct either the width or the height. If the width is fine in
			 * terms of the aspect ratio then we move on to check the height.
			 *
			 * Possibly the logic here is intuitive, but if you are wondering why we swap
			 * from w / h to h / w between checking the minimum and maximum aspect ratios
			 * then that has to do with how we set mina and maxa in the first place in the
			 * updatesizehints function:
			 *
			 *    	c->mina = (float)size.min_aspect.y / size.min_aspect.x;
			 *    	c->maxa = (float)size.max_aspect.x / size.max_aspect.y;
			 */
			if (c->maxa < (float)*w / *h)
				*w = *h * c->maxa + 0.5;
			else if (c->mina < (float)*h / *w)
				*h = *w * c->mina + 0.5;
		}

		/* Some applications only allow the size to be changed in certain increments. A good
		 * example of this is the Simple Terminal emulator (st) which has size hints that
		 * only allow increments of line height on the y axis and column width on the x axis.
		 * The reason for this is so that st can draw whole lines / columns with the given
		 * size. Resizing a stock st window using the mouse can give a jagged look and feel.
		 *
		 * Refer to these additional details from ICCCM 4.1.2.3.
		 *
		 *    The min_width and min_height elements specify the minimum size that the window
		 *    can be for the client to be useful. The max_width and max_height elements
		 *    specify the maximum size. The base_width and base_height elements in
		 *    conjunction with width_inc and height_inc define an arithmetic progression of
		 *    preferred window widths and heights for nonnegative integers i and j:
		 *
		 *       width = base_width + (i × width_inc)
		 *       height = base_height + (j × height_inc)
		 *
		 *    Window managers are encouraged to use i and j instead of width and height in
		 *    reporting window sizes to users. If a base size is not provided, the minimum
		 *    size is to be used in its place and vice versa.
		 *
		 * What this means is that before check for and take size increments into account we
		 * need to remove the base size from the height and width values.
		 */
		if (baseismin) { /* increment calculation requires this */
			*w -= c->basew;
			*h -= c->baseh;
		}


		/* Below we check if there are incremental width and height restrictions, and if so
		 * then we make sure that the size is a multiple of said increment by deducting the
		 * remainder. */
		if (c->incw)
			*w -= *w % c->incw;
		if (c->inch)
			*h -= *h % c->inch;

		/* Restore base dimensions.
		 *
		 * Here we make sure that the window size is not less than the minimum size.
		 * The reason for adding the base size to the width and height is because regardless
		 * of whether the baseismin variable is true or not we will have removed the base
		 * size from the height or width in relation to checking aspect ratio or in relation
		 * to checking size increments. */
		*w = MAX(*w + c->basew, c->minw);
		*h = MAX(*h + c->baseh, c->minh);

		/* Finally we check if the client is still within the maximum size (if specified),
		 * and if it is not then we reduce the size to be the maximum. */
		if (c->maxw)
			*w = MIN(*w, c->maxw);
		if (c->maxh)
			*h = MIN(*h, c->maxh);
	}

	/* Here we check whether the size and position has changed after applying size hints before
	 * returning. This tells the resize function whether we need to go ahead with the resizing
	 * the window or not.
	 *
	 * Again the st example with incremental size hints demonstrates this nicely in that as long
	 * as the mouse cursor doesn't move far enough to add (or remove) another row or column then
	 * there is no need for a resize. */
	return *x != c->x || *y != c->y || *w != c->w || *h != c->h;
}

/* The arrange call handles moving clients into and out of view depending on what tags are shown
 * and it triggers a re-arrangement of windows according to the selected layout.
 *
 * Passing a NULL value to arrange results in the above happening for all monitors. Passing a
 * specific monitor to the arrange function results in the above happening for that monitor and
 * in addition a restack is applied which will call drawbar.
 *
 * @called_from configurenotify if the monitor setup has changed
 * @called_from incnmaster after the number of master clients have been adjusted
 * @called_from manage upon managing a new client
 * @called_from pop in relation to a call to zoom to make a client window the new master
 * @called_from propertynotify in relation to a window becoming floating due to being transient
 * @called_from sendmon after moving a client window to another monitor
 * @called_from setfullscreen when a fullscreen window exits fullscreen
 * @called_from setlayout if there visible client windows to be arranged after changing layout
 * @called_from setmfact after the master / stack factor has been changed
 * @called_from tag when changing tags for the selected client
 * @called_from togglebar after revealing or hiding the bar
 * @called_from togglefloating after changing the floating state for the selected client
 * @called_from toggletag after changing a client's tags
 * @called_from toggleview after bringing tags into or out of view
 * @called_from unmanage after unmanaging a window
 * @called_from view after changing the tag(s) viewed
 * @calls showhide to move client windows into and out of view
 * @calls arrangemon to trigger re-arrangement of windows according to the selected layout
 * @calls restack to draw the bar and adjust which clients are shown above others
 *
 * Internal call stack:
 *    run -> configurenotify -> arrange
 *    run -> buttonpress -> tag -> arrange
 *    run -> buttonpress -> movemouse / resizemouse -> sendmon -> arrange
 *    run -> buttonpress -> movemouse / resizemouse -> togglefloating -> arrange
 *    run -> buttonpress -> togglefloating -> arrange
 *    run -> buttonpress -> toggletag -> arrange
 *    run -> buttonpress -> toggleview -> arrange
 *    run -> buttonpress -> view -> arrange
 *    run -> keypress -> incnmaster -> arrange
 *    run -> keypress -> setlayout -> arrange
 *    run -> keypress -> setmfact -> arrange
 *    run -> keypress -> zoom -> pop -> arrange
 *    run -> keypress -> tagmon -> sendmon -> arrange
 *    run -> keypress -> tag -> arrange
 *    run -> keypress -> togglebar -> arrange
 *    run -> keypress -> togglefloating -> arrange
 *    run -> keypress -> toggletag -> arrange
 *    run -> keypress -> toggleview -> arrange
 *    run -> keypress -> view -> arrange
 *    run -> maprequest -> manage -> arrange
 *    run -> propertynotify -> arrange
 *    run -> clientmessage / updatewindowtype -> setfullscreen -> arrange
 *    run -> destroynotify / unmapnotify -> unmanage -> arrange
 *    main -> cleanup -> view -> arrange
 */
void
arrange(Monitor *m)
{
	/* If we have been given a specific monitor then call showhide to move windows into and out
	 * of view for that monitor. */
	if (m)
		showhide(m->stack);
	/* Otherwise we call showhide for all monitors */
	else for (m = mons; m; m = m->next)
		showhide(m->stack);

	/* In similar vein, if we have been given a specific monitor then we call arrangemon to
	 * re-arrange client windows according to the layout. */
	if (m) {
		arrangemon(m);
		/* The one additional thing we do when given a specific monitor is to call restack.
		 * The purpose of that function is to raise the selected window above others if it is
		 * floating and to move all tiled windows below the bar window. In practice this only
		 * has an effect in maybe one or two scenarios where arrange is called, for example
		 * it will likely have an effect when togglefloating calls arrange as the selected
		 * client will have changed the floating state. As such the primary reason for the
		 * restack call here is likely to trigger drawbar for the monitor. */
		restack(m);
	/* Otherwise we call arrangemon for all monitors */
	} else for (m = mons; m; m = m->next)
		arrangemon(m);
}

/* This sets / updates the layout symbol for the monitor and calls the layout arrange function
 * (tile or monocle) to resize and reposition client windows.
 *
 * @called_from arrange to handle layout arrangements
 * @calls monocle to resize and reposition client windows
 * @calls tile to resize and reposition client windows
 *
 * Internal call stack:
 *    ~ -> arrange -> arrangemon
 */
void
arrangemon(Monitor *m)
{
	/* This copies the layout symbol of the selected layout to the monitor's layout string,
	 * which is later used in drawbar when printing the layout symbol on the bar. */
	strncpy(m->ltsymbol, m->lt[m->sellt]->symbol, sizeof m->ltsymbol);

	/* If floating layout is used then the arrange function will be NULL, otherwise we call
	 * that arrange function. This will be the tile function or the monocle function depending
	 * on what layout is selected. */
	if (m->lt[m->sellt]->arrange)
		m->lt[m->sellt]->arrange(m);
}

/* This inserts a client at the top of the monitor's client list.
 *
 * The client list is a linked list of client structures where one client refers to the next.
 * This list is primarily used:
 *    - for managing the order in which client windows are tiled and
 *    - to keep track of the clients that are managed by the window manager on a specific monitor
 *
 * Usage warnings:
 *    - a client must be detached before it can be attached again
 *    - there is no guard preventing an already attached client from being attached again
 *    - attaching an already attached client will most likely result in an infinite loop and dwm
 *      freezing with high CPU usage
 *    - when moving clients from one monitor to another the client needs to be detached from both
 *      the client list and the client stack before the monitor is changed
 *    - detaching a client does not remove its references to other clients, this results in
 *      dangling pointers
 *
 * @called_from manage when the window manager manages a new window
 * @called_from pop reattach a client to become the new master client
 * @called_from sendmon to attach the client to the new monitor
 * @called_from updategeom when the number of monitors is reduced
 *
 * Internal call stack:
 *    run -> maprequest -> manage -> attach
 *    run -> keypress -> zoom -> pop -> attach
 *    run -> keypress -> tagmon -> sendmon -> attach
 *    run -> buttonpress -> movemouse / resizemouse -> sendmon -> attach
 *    run -> configurenotify -> updategeom -> attach
 */
void
attach(Client *c)
{
	/* This sets the given client's next reference to the head of the list, then it sets the
	 * head of the list to become the given client. In practice the given client becomes the
	 * first client in the linked list. */
	c->next = c->mon->clients;
	c->mon->clients = c;
}

/* This inserts a client at the top of the monitor's stacking order.
 *
 * The stacking order is a linked list of client structures where one client refers to the next.
 * This list is primarily used:
 *    - for managing the order in which client windows are placed on top of each other
 *
 * Refer to the writeup for the attach function for usage warnings.
 *
 * @called_from manage when the window manager manages a new window
 * @called_from sendmon to attach the client to the new monitor
 * @called_from updategeom when the number of monitors is reduced
 *
 * Internal call stack:
 *    ~ -> focus -> attachstack
 *    run -> maprequest -> manage -> attachstack
 *    run -> keypress -> tagmon -> sendmon -> attachstack
 *    run -> buttonpress -> movemouse / resizemouse -> sendmon -> attachstack
 *    run -> configurenotify -> updategeom -> attachstack
 */
void
attachstack(Client *c)
{
	/* This sets the given client's snext reference to the head of the list, then it sets the
	 * head of the list to become the given client. In practice the given client becomes the
	 * first client in the stack. */
	c->snext = c->mon->stack;
	c->mon->stack = c;
}

/* This handles ButtonPress events coming from the X server.
 *
 * Most of this function has to do with working out what exactly the user clicked on, which is
 * represented by the clicks enum:
 *
 *    enum { ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle,
 *           ClkClientWin, ClkRootWin, ClkLast };
 *
 * In practice this boils down to:
 *   - the root window or
 *   - a client window or
 *   - the bar window, in which case the part of the bar the user clicked on was:
 *      -- one of the tag icons or
 *      -- the layout symbol or
 *      -- the window title or
 *      -- the status text
 *
 * Once that is known it will loop through all the button bindings as defined in the buttons array
 * in the configuration file to look for bindings that match the click type combined with the mouse
 * button pressed along with modifier keys. If such a button binding is found then the function
 * set in the array will be executed with given argument.
 *
 * For details on how things are set up in order to get these ButtonPress events in the first place
 * refer to the grabbuttons function.
 *
 * @called_from run (the event handler)
 * @calls XAllowEvents https://tronche.com/gui/x/xlib/input/XAllowEvents.html
 * @calls wintomon to check whether the button click happened on another monitor
 * @calls wintoclient to check whether the button click was on a client window
 * @calls focus to change focus to a different client or a different monitor
 * @calls unfocus to unfocus the selected client on the previous monitor
 * @calls restack to bring the clicked client to the front in case it is floating
 * @calls functions as defined in the buttons array
 * @see grabbuttons for how the window manager registers for button presses on windows
 *
 * Internal call stack:
 *    run -> buttonpress
 */
void
buttonpress(XEvent *e)
{
	unsigned int i, x, click;
	Arg arg = {0}; /* Argument to store the tag index in case the user clicks on a tag icon */
	Client *c;
	Monitor *m;
	XButtonPressedEvent *ev = &e->xbutton;

	/* Default to clicking on the root window, as in the background wallpaper */
	click = ClkRootWin;
	/* If the button click was on a monitor other than the currently focused monitor, then
	 * change the selected monitor to the monitor the user clicked on. */
	if ((m = wintomon(ev->window)) && m != selmon) {
		/* The unfocus call here is primarily because we could be moving from a monitor with
		 * clients to one with none, in which case we would want to revert the input focus
		 * back to the root window (note the passing of 1 for the setfocus parameter to
		 * unfocus). Additionally we want the client border to change colour to indicate that
		 * it does not have focus. */
		unfocus(selmon->sel, 1);
		/* This just sets the selected monitor to the monitor the mouse click happened on */
		selmon = m;
		/* Focus on the last focused client on this monitor (if any) */
		focus(NULL);
	}
	/* This checks if the mouse click was on the bar, in which case we need to work out what
	 * part of the bar the user clicked on. */
	if (ev->window == selmon->barwin) {
		/* This section of code calculates what the user clicked on by looking at the text
		 * width of tag icons, the layout symbol and the status text.
		 *
		 * In practice you may think of it as going through the same steps as drawing the
		 * bar, but this time we only calculate where things would have been drawn if we
		 * were to draw them. As such the following code must mirror exactly what the
		 * drawbar function does as otherwise that can lead to mouse click misalignment.
		 */
		i = x = 0;
		/* This do while loop goes through each tag icon and increments x with the width of
		 * the tag. Then it will check if the x position is larger than the mouse click
		 * position and if it is then it will break out of the while loop.
		 *
		 * On the other hand if it is not then it will increment the iterator i and continue
		 * with the next tag icon until it has gone through all tags.
		 */
		do
			x += TEXTW(tags[i]);
		while (ev->x >= x && ++i < LENGTH(tags));
		/* If the previous do while loop stopped before it had gone through all tags then
		 * that means that the user clicked on one of the tags in the bar. */
		if (i < LENGTH(tags)) {
			/* We set the click type to ClkTagBar to indicate that the user clicked on
			 * one of the tags, and the i iterator shows what tag was clicked. */
			click = ClkTagBar;
			/* The view, toggleview, tag and toggletag functions takes a bitmask argument
			 * rather than a simple index hence we do a binary shift when setting the
			 * argument for those functions. E.g. when the user clicks on tag 6 then the
			 * iterator i will be 5, and shifting 1 five times to the left gives a binary
			 * bitmask value of 00100000. */
			arg.ui = 1 << i;
		/* If we did exhaust all the tag icons then we check if the user must have clicked
		 * on the layout symbol. */
		} else if (ev->x < x + TEXTW(selmon->ltsymbol))
			click = ClkLtSymbol;
		/* If the click was to the right of the layout symbol then we need to check if the
		 * click was on the status text, which is drawn to the far right of the bar. We do
		 * not actually know the width of the client window title */
		else if (ev->x > selmon->ww - (int)TEXTW(stext))
			click = ClkStatusText;
		/* The click was not to the left and not to the right, so the click must have been
		 * on the window title. */
		else
			click = ClkWinTitle;
	/* If the click was not on the bar window then check if the click was on one of the client
	 * windows. If it was not then the click is assumed to have been on the root window. */
	} else if ((c = wintoclient(ev->window))) {
		/* If we click on a client window then give that window input focus. We want this to
		 * be the selected client. */
		focus(c);
		/* If the window we clicked on was a floating window then we want that window to be
		 * shown on top of other floating windows. The restack call takes care of that. */
		restack(selmon);
		/* This has specifically to do with events sent to windows that grab the mouse
		 * pointer when you click on them. The XAllowEvents with the ReplayPointer event mode
		 * releases that pointer grab allowing for the event to be reprocessed. The important
		 * thing is that this call has no effect if the window does not grab the mouse
		 * pointer. Refer to the documentation for XAllowEvents for more details. */
		XAllowEvents(dpy, ReplayPointer, CurrentTime);
		click = ClkClientWin;
	}
	/* This section finally loops through all the button bindings looking for matches. */
	for (i = 0; i < LENGTH(buttons); i++)
		/* Let's break down the matching process here with an example.
		 *
		 *      click                 event mask      button        function        argument
		 *    { ClkClientWin,         MODKEY,         Button3,      resizemouse,    {0} },
		 *
		 * This checks that the click type, e.g. ClkClientWin, matches:
		 *    click == buttons[i].click &&
		 *
		 * Button bindings without a function are simply ignored:
		 *    buttons[i].func &&
		 *
		 * This checks that the button matches, e.g. Button3 - right click:
		 *    buttons[i].button == ev->button
		 *
		 * This checks that the modifier matches:
		 *    CLEANMASK(buttons[i].mask) == CLEANMASK(ev->state)
		 *
		 * It is worth noting that the CLEANMASK macro removes Num Lock and Caps Lock mask
		 * from the button mask as well as the event state. This has to do with that we want
		 * these button bindings to work regardless of whether Num Lock and/or Caps Lock is
		 * enabled or not. See the grabbuttons function for how the subscribe to all the
		 * button combinations to cover this.
		 */
		if (click == buttons[i].click && buttons[i].func && buttons[i].button == ev->button
		&& CLEANMASK(buttons[i].mask) == CLEANMASK(ev->state))
			/* If we have a match then we call the associated function with the given
			 * argument, unless the user clicked on the tags in which case we pass the
			 * argument with the bitmask we set previously. */
			buttons[i].func(click == ClkTagBar && buttons[i].arg.i == 0 ? &arg : &buttons[i].arg);
			/* Note that there is no break; following this, which means that we will
			 * continue searching through the button bindings for more matches. As such
			 * it is possible to have more than one thing happen when a button is clicked
			 * by having the same button bindings multiple times referring to different
			 * functions. */
}

/* This checks whether another window manager is already running and if so then dwm is exited
 * gracefully.
 *
 * @called_from main to check whether another window manager is already running
 * @calls XSetErrorHandler https://tronche.com/gui/x/xlib/event-handling/protocol-errors/XSetErrorHandler.html
 * @calls XSelectInput https://tronche.com/gui/x/xlib/event-handling/XSelectInput.html
 * @calls DefaultRootWindow a macro that returns the root window for the default screen
 * @calls XSync https://tronche.com/gui/x/xlib/event-handling/XSync.html
 * @see https://tronche.com/gui/x/xlib/display/display-macros.html
 * @see xerrorstart which bails on any X error
 *
 * Internal call stack:
 *    main -> checkotherwm
 */
void
checkotherwm(void)
{
	/* This sets the X error handler to xerrorstart which will make dwm exit on any X error */
	xerrorxlib = XSetErrorHandler(xerrorstart);
	/* This causes an error if some other window manager is running. This has to do with that
	 * X only allows for a single connection to do this.
	 *
	 * As outlined in the header comment of dwm.c:
	 *    In contrast to other X clients, a window manager selects for SubstructureRedirectMask
	 *    on the root window, to receive events about window (dis-)appearance. Only one X
	 *    connection at a time is allowed to select for this event mask.
	 */
	XSelectInput(dpy, DefaultRootWindow(dpy), SubstructureRedirectMask);
	/* This flushes the output buffer and then waits until all requests have been
	 * received and processed by the X server. */
	XSync(dpy, False);
	/* Set the error handler back to the normal one. */
	XSetErrorHandler(xerror);
	/* Repeat */
	XSync(dpy, False);
}

/* This function handles all cleanup that is needed before exiting which involves:
 *    - unmanaging all windows that are managed by the window manager
 *    - releasing all keybindings
 *    - tearing down all monitors
 *    - freeing cursors
 *    - freeing colour schemes and their colours
 *    - destroying the supporting window
 *    - free the drawable
 *    - reverting input focus and clearing the _NET_ACTIVE_WINDOW property of the root window
 *
 * @called_from main before exiting dwm_NET_SUPPORTING_WM_CHECK
 * @calls XDestroyWindow https://tronche.com/gui/x/xlib/window/XDestroyWindow.html
 * @calls XDeleteProperty https://tronche.com/gui/x/xlib/window-information/XDeleteProperty.html
 * @calls XSetInputFocus https://tronche.com/gui/x/xlib/input/XSetInputFocus.html
 * @calls XSync https://tronche.com/gui/x/xlib/event-handling/XSync.html
 * @calls XUngrabKey https://tronche.com/gui/x/xlib/input/XUngrabKey.html
 * @calls view to bring all clients into view
 * @calls unmanage to stop managing all windows managed by the window manager
 * @calls cleanupmon to tear down each monitor
 * @calls drw_cur_free to free all mouse cursor options
 * @calls drw_free to free the drawable
 * @calls free to memory used by the colour schemes
 *
 * Internal call stack:
 *    main -> cleanup
 */
void
cleanup(void)
{
	/* This sets up an argument that will select all tags */
	Arg a = {.ui = ~0};
	Layout foo = { "", NULL }; /* A dummy floating layout */
	Monitor *m;
	size_t i;

	/* This calls view with the argument that selects and views all tags. This is what makes
	 * dwm briefly show all client windows for a fraction of a second when exiting.
	 *
	 * The purpose of this may not be obvious, but it has to do with how dwm hides client
	 * windows by moving them to a negative x position. In principle the X session could be
	 * taken over by another window manager, which would cause some issues and/or confusion
	 * given that some windows will be in an unreachable location.
	 *
	 * As such the view call here makes sure to pull all clients into view before dwm exits.
	 */
	view(&a);
	/* This sets the selected monitor's layout to the dummy floating layout. The purpose of
	 * this is presumably to negate a flood of arrange calls when calling unmanage for each
	 * client. If that is the case then it would make more sense having this inside the for
	 * loop so that this applies to all monitors when exiting, not just the selected one. */
	selmon->lt[selmon->sellt] = &foo;
	/* Loop through each monitor and unmanage all clients until the stack is exhausted */
	for (m = mons; m; m = m->next)
		while (m->stack)
			unmanage(m->stack, 0);
	/* This releases any keybindings (grabbed keys) */
	XUngrabKey(dpy, AnyKey, AnyModifier, root);
	/* Loop through and tear down all monitors */
	while (mons)
		cleanupmon(mons);
	/* Loop through and free each cursor */
	for (i = 0; i < CurLast; i++)
		drw_cur_free(drw, cursor[i]);
	/* Loop through and free each colour scheme */
	for (i = 0; i < LENGTH(colors); i++)
		free(scheme[i]);
	/* Free the memory used for the scheme struct as well */
	free(scheme);
	/* Destroy the supporting window, refer to the setup function for more details on this */
	XDestroyWindow(dpy, wmcheckwin);
	/* Free the drawable structure */
	drw_free(drw);
	/* This flushes the output buffer and then waits until all requests have been
	 * received and processed by the X server. */
	XSync(dpy, False);
	/* This reverts the input focus back to the root window */
	XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
	/* This deletes the _NET_ACTIVE_WINDOW property of the root window as the window manager
	 * no longer manages any windows. */
	XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
}

/* This function deals with tearing down a monitor which involves:
 *    - tidying monitor references
 *    - destroying the bar window
 *    - freeing the memory used by the given monitor struct
 *
 * @called_from cleanup to delete bar windows and free memory before exiting
 * @called_from updategeom to delete bar windows and free memory in the event of less monitors
 * @calls XUnmapWindow https://tronche.com/gui/x/xlib/window/XUnmapWindow.html
 * @calls XDestroyWindow https://tronche.com/gui/x/xlib/window/XDestroyWindow.html
 * @calls free to release memory used by the given monitor struct
 *
 * Internal call stack:
 *    run -> configurenotify -> updategeom -> cleanupmon
 *    main -> cleanup -> cleanupmon
 */
void
cleanupmon(Monitor *mon)
{
	Monitor *m;

	/* The mons (monitors) variable holds the reference to the first monitor in a linked list.
	 * If the monitor being removed is the first monitor in this linked list, then we detach
	 * the monitor by setting the mons variable to be the next monitor in the list.
	 *
	 * In other words
	 *    mon -> a -> b -> c -> NULL
	 *    ^-- mons
	 *
	 * becomes
	 *    mon -> a -> b -> c -> NULL
	 *           ^-- mons
	 */
	if (mon == mons)
		mons = mons->next;
	/* Otherwise we loop through each monitor until we find the previous monitor (which has
	 * the given monitor as the next monitor). We then detach the given monitor by having the
	 * previous monitor skip over it.
	 *
	 * In other words
	 *    a -> b -> mon -> c -> NULL
	 *
	 * becomes
	 *    a -> b -> c -> NULL
	 */
	else {
		for (m = mons; m && m->next != mon; m = m->next);
		m->next = mon->next;
	}
	/* Call to unmap the window so that it is no longer shown */
	XUnmapWindow(dpy, mon->barwin);
	/* Call to destroy the window */
	XDestroyWindow(dpy, mon->barwin);
	/* Finally free up memory used by the monitor struct */
	free(mon);
}

/* This handles ClientMessage events coming from the X server.
 *
 * dwm only handles two types of client messages and these are:
 *
 *    _NET_WM_STATE > _NET_WM_STATE_FULLSCREEN and
 *    _NET_ACTIVE_WINDOW
 *
 * It is possible to expand this function to handle other states and message types.
 *
 * @called_from run (the event handler)
 * @calls wintoclient to find the client the given event is for
 * @calls setfullscreen to make a window fullscreen when _NET_WM_STATE_FULLSCREEN is received
 * @calls seturgent to mark a client as urgent when _NET_ACTIVE_WINDOW is received
 * @see https://tronche.com/gui/x/xlib/events/client-communication/client-message.html
 * @see https://specifications.freedesktop.org/wm-spec/1.3/ar01s05.html
 * @see https://specifications.freedesktop.org/wm-spec/wm-spec-1.3.html#idm45805407959456
 *
 * Internal call stack:
 *    run -> clientmessage
 */
void
clientmessage(XEvent *e)
{
	XClientMessageEvent *cme = &e->xclient;
	/* Find the client the window is in relation to. */
	Client *c = wintoclient(cme->window);

	/* If we are not managing this window then ignore the event. */
	if (!c)
		return;

	/* This handles the _NET_WM_STATE message type.
	 *
	 * To change the state of a mapped window, a client MUST send a _NET_WM_STATE client message
	 * to the root window.
	 *
	 *    window  = the respective client window
	 *    message_type = _NET_WM_STATE
	 *    format = 32
	 *    data.l[0] = the action, as listed below
	 *    data.l[1] = first property to alter
	 *    data.l[2] = second property to alter
	 *    data.l[3] = source indication
	 *    other data.l[] elements = 0
	 *
	 * _NET_WM_STATE_REMOVE        0  // remove/unset property
	 * _NET_WM_STATE_ADD           1  // add/set property
	 * _NET_WM_STATE_TOGGLE        2  // toggle property
	 *
	 * This message allows two properties to be changed simultaneously, specifically to allow
	 * both horizontal and vertical maximisation to be altered together. As such we need to
	 * check both the first and second property when handling state property types.
	 *
	 * Possible atoms are:
	 *
	 *    _NET_WM_STATE_MODAL
	 *    _NET_WM_STATE_STICKY
	 *    _NET_WM_STATE_MAXIMIZED_VERT
	 *    _NET_WM_STATE_MAXIMIZED_HORZ
	 *    _NET_WM_STATE_SHADED
	 *    _NET_WM_STATE_SKIP_TASKBAR
	 *    _NET_WM_STATE_SKIP_PAGER
	 *    _NET_WM_STATE_HIDDEN
	 *    _NET_WM_STATE_FULLSCREEN
	 *    _NET_WM_STATE_ABOVE
	 *    _NET_WM_STATE_BELOW
	 *    _NET_WM_STATE_DEMANDS_ATTENTION
	 *
	 * Out of the above dwm only supports _NET_WM_STATE_FULLSCREEN by default.
	 */
	if (cme->message_type == netatom[NetWMState]) {
		/* If the property being changed is _NET_WM_STATE_FULLSCREEN then */
		if (cme->data.l[1] == netatom[NetWMFullscreen]
		|| cme->data.l[2] == netatom[NetWMFullscreen])
			/* call setfullscreen for the client window passing:
			 *    1 if the action is to add fullscreen
			 *    1 if the action is to toggle fullscreen and the client is not fullscreen
			 *    0 otherwise to exit fullscreen
			 */
			setfullscreen(c, (cme->data.l[0] == 1 /* _NET_WM_STATE_ADD    */
				|| (cme->data.l[0] == 2 /* _NET_WM_STATE_TOGGLE */ && !c->isfullscreen)));

	/* The below handles the _NET_ACTIVE_WINDOW message type and the action taken by dwm is to
	 * set the urgency flag for the client unless it is the selected window.
	 *
	 * A client marked as urgent will result in the tag the client is present on to have its
	 * colours inverted (as in the foreground and background colours swapping place) on the bar
	 * unless the tag is selected.
	 *
	 * You can test this by finding the window ID of a given window using xwininfo (e.g.
	 * 0x5000002) or using xdotool search (94371846) and using xdo or xdotool to activate that
	 * window.
	 *
	 *    $ xdo activate 0x5a00006
	 *    $ xdotool windowactivate 94371846
	 *
	 * Should you need to convert between decimal and hexadecimal window IDs we have:
	 *
	 *    $ echo $((0x5a00006))
	 *    94371846
	 *
	 *    $ printf '0x%x\n' 94371846
	 *    0x5a00006
	 */
	} else if (cme->message_type == netatom[NetActiveWindow]) {
		if (c != selmon->sel && !c->isurgent)
			seturgent(c, 1);
	}
}

/* This propagates the border width, size and position back to the client window.
 *
 * @called_from configurerequest in relation to external requests to resize client windows
 * @called_from manage to propagate the border width to the window
 * @called_from resizeclient to propagate size changes to the window
 * @calls XSendEvent https://tronche.com/gui/x/xlib/event-handling/XSendEvent.html
 * @see https://tronche.com/gui/x/xlib/events/processing-overview.html
 * @see https://tronche.com/gui/x/xlib/events/window-state-change/configure.html
 * @see https://tronche.com/gui/x/xlib/window/attributes/override-redirect.html
 *
 * Internal call stack:
 *    run -> configurerequest -> configure
 *    run -> maprequest -> manage -> configure
 *    ~ -> resize -> resizeclient -> configure
 */
void
configure(Client *c)
{
	XConfigureEvent ce;

	/* The type of this event is ConfigureNotify. The rest of the attributes are event data
	 * that we want to propagate like the client's border width, the position and size. */
	ce.type = ConfigureNotify;
	ce.display = dpy;
	ce.event = c->win;
	ce.window = c->win;
	ce.x = c->x;
	ce.y = c->y;
	ce.width = c->w;
	ce.height = c->h;
	ce.border_width = c->bw;
	/* Whether the client window is above or below a certain window. This is irrelevant so we
	 * pass None. */
	ce.above = None;
	/* Whether the window manages itself. The window manager manages this client window so we
	 * set this to False. */
	ce.override_redirect = False;
	XSendEvent(dpy, c->win, False, StructureNotifyMask, (XEvent *)&ce);
}

/* This handles ConfigureNotify events coming from the X server.
 *
 * This can happen for example if you run an xrandr command that enables a monitor or
 * changes resolution. Only ConfigureNotify events for the root window are handled.
 *
 * @called_from run (the event handler)
 * @calls updategeom to update the number of monitors, their sizes and positions
 * @calls drw_resize to adjust the drawable space
 * @calls updatebars to create new bar windows in case we have new monitors
 * @calls resizeclient to restore fullscreen client windows
 * @calls XMoveResizeWindow https://tronche.com/gui/x/xlib/window/XMoveResizeWindow.html
 * @calls focus to give back input focus to the last used window as it may have been lost
 * @calls arrange to resize and reposition tiled clients as the monitor may have changed
 *
 * Internal call stack:
 *    run -> configurenotify -> updategeom
 */
void
configurenotify(XEvent *e)
{
	Monitor *m;
	Client *c;
	XConfigureEvent *ev = &e->xconfigure;
	int dirty;

	/* TODO: updategeom handling sucks, needs to be simplified (famous last words) */
	if (ev->window == root) {
		/* If the width and height of the event differs from the stored screen width and
		 * height then we will want do the corrections in the if statement below regardless
		 * of whether updategeom detects a change in the number of monitors. We could just
		 * have a case of the resolution of the monitor changing. */
		dirty = (sw != ev->width || sh != ev->height);
		/* This updates the screen width and the sceen height global variables */
		sw = ev->width;
		sh = ev->height;
		/* Have updategeom run a check to see if we have any new or less monitors and
		 * enter regardless if the screen size has changed. */
		if (updategeom() || dirty) {
			/* This next line changes the screen drawable area. That it resizes the
			 * drawable area to be the height of the bar is most likely a bug considering
			 * that the drawable area is created with the dimensions of the screen in the
			 * setup function:
			 *
			 *    drw = drw_create(dpy, screen, root, sw, sh);
			 *
			 * This does not seem to affect the operability of dwm, however, as all that
			 * the window manager draws is the bar which does not exceed the bar height.
			 */
			drw_resize(drw, sw, bh);
			/* This call to updatebars is to create new bar windows in the event that the
			 * call to updategeom resulted in new monitors to be created. */
			updatebars();
			/* Loop through each monitor to make correction */
			for (m = mons; m; m = m->next) {
				/* For every client on the monitor check to see if any of them was in
				 * fullscreen and if so then resize them to restore fullscreen. */
				for (c = m->clients; c; c = c->next)
					if (c->isfullscreen)
						resizeclient(c, m->mx, m->my, m->mw, m->mh);
				/* This resizes and repositions the bar according to the new
				 * position and size of the monitor. */
				XMoveResizeWindow(dpy, m->barwin, m->wx, m->by, m->ww, bh);
			}
			/* Give focus back to the last viewed client in case input focus got lost
			 * as part of the monitor updates. */
			focus(NULL);
			/* A final arrange call to resize and reposition tiled clients following the
			 * monitor changes. */
			arrange(NULL);
		}
	}
}

/* This handles ConfigureRequest events coming from the X server.
 *
 * The configure window request attempts to reconfigure a window's size, position, border, and
 * stacking order.
 *
 * @called_from run (the event handler)
 * @called_from movemouse as it forwards events to configurerequest
 * @called_from resizemouse as it forwards events to configurerequest
 * @calls XMoveResizeWindow https://tronche.com/gui/x/xlib/window/XMoveResizeWindow.html
 * @calls XConfigureWindow https://tronche.com/gui/x/xlib/window/XConfigureWindow.html
 * @calls XSync https://tronche.com/gui/x/xlib/event-handling/XSync.html
 * @calls wintoclient to find the client the event is in relation to
 * @calls configure to let the client window know what has changed
 * @see https://tronche.com/gui/x/xlib/events/structure-control/configure.html
 *
 * Internal call stack:
 *    run -> configurerequest
 *    run -> buttonpress -> movemouse / resizemouse -> configurerequest
 */
void
configurerequest(XEvent *e)
{
	Client *c;
	Monitor *m;
	XConfigureRequestEvent *ev = &e->xconfigurerequest;
	XWindowChanges wc;

	/* If the event is in relation to a client window (as in a managed window). */
	if ((c = wintoclient(ev->window))) {
		/* If the event includes a value for the border width then we accept that and set
		 * that value for the client. Notably we ignore any other data the event contains
		 * if it includes a value for the border width. */
		if (ev->value_mask & CWBorderWidth)
			c->bw = ev->border_width;
		/* Otherwise if the client is floating or if we are in floating layout */
		else if (c->isfloating || !selmon->lt[selmon->sellt]->arrange) {
			m = c->mon;
			/* If the event includes a position on the X axis then accept a change in
			 * position. What is worth noting here is that dwm treats the event X position
			 * as being relative to the monitor the window is on. This has to do with how
			 * dwm treats different monitors as separate areas.
			 *
			 * This can cause issues with some programs that assume the X and Y positions
			 * are absolute references (as they are on stacking window managers) causing
			 * the window to move to an unexpected location depending on monitor setup.
			 */
			if (ev->value_mask & CWX) {
				c->oldx = c->x;
				c->x = m->mx + ev->x;
			}

			/* The same applies if the event includes a position on the Y axis. */
			if (ev->value_mask & CWY) {
				c->oldy = c->y;
				c->y = m->my + ev->y;
			}

			/* If the event includes information on a new width then accept that. */
			if (ev->value_mask & CWWidth) {
				c->oldw = c->w;
				c->w = ev->width;
			}

			/* If the event includes information on a new height then accept that. */
			if (ev->value_mask & CWHeight) {
				c->oldh = c->h;
				c->h = ev->height;
			}

			/* If the client's position and size is such that the window would go across
			 * the right hand monitor border then center the window on the X axis on the
			 * client's monitor. This only applies when the client window is explicitly
			 * floating and not when the client is tiled but floating layout is used. */
			if ((c->x + c->w) > m->mx + m->mw && c->isfloating)
				c->x = m->mx + (m->mw / 2 - WIDTH(c) / 2); /* center in x direction */

			/* If the client's position and size is such that the window would go across
			 * the bottom monitor border then center the window on the Y axis on the
			 * client's monitor. This only applies when the client window is explicitly
			 * floating and not when the client is tiled but floating layout is used. */
			if ((c->y + c->h) > m->my + m->mh && c->isfloating)
				c->y = m->my + (m->mh / 2 - HEIGHT(c) / 2); /* center in y direction */

			/* If the event contained a positional change only (no change to height and
			 * width) then we make a call to configure to inform the client about the new
			 * position. Why exactly we only do this for positional changes for floating
			 * windows only is not entirely clear, but the git commit history suggests
			 * that this is specifically in relation to client windows that are fixed in
			 * size and thus are floating. */
			if ((ev->value_mask & (CWX|CWY)) && !(ev->value_mask & (CWWidth|CWHeight)))
				configure(c);

			/* If the client window is visible then we call XMoveResizeWindow to apply the
			 * size and position adjustments. Note that client windows that request a
			 * move or resize while they are not visible will, however, not be moved or
			 * resized. */
			if (ISVISIBLE(c))
				XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
		/* Otherwise the client is tiled. In this case we just make a call to configure which
		 * informs the window what its size and border width is. What this actually means is
		 * that the client window requested a change and the window manager rejects that
		 * request by responding with a ConfigureNotify event informing the client window
		 * what its size, position and border width is. */
		} else
			configure(c);
	/* If the configure request was not in relation to a managed window then we merely pass on
	 * the window changes requested to the target window. */
	} else {
		wc.x = ev->x;
		wc.y = ev->y;
		wc.width = ev->width;
		wc.height = ev->height;
		wc.border_width = ev->border_width;
		wc.sibling = ev->above;
		wc.stack_mode = ev->detail;
		XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
	}

	/* This flushes the output buffer and then waits until all requests have been received and
	 * processed by the X server. */
	XSync(dpy, False);
}

/* This creates and returns a new monitor structure.
 *
 * The monitor's position and size are set in the updategeom function which handles monitor setup.
 *
 * @called_from updategeom to create new monitors
 * @calls ecalloc to allocate memory for the new structure (see util.c)
 * @calls strncpy to copy the default layout symbol into the monitor layout symbol
 *
 * Internal call stack:
 *    run -> configurenotify -> updategeom -> createmon
 *    main -> setup -> updategeom -> createmon
 */
Monitor *
createmon(void)
{
	Monitor *m;

	/* Allocate memory to hold the new monitor. */
	m = ecalloc(1, sizeof(Monitor));

	/* This sets the current and previous tagset to 1, as in the first tag is selected by
	 * default. */
	m->tagset[0] = m->tagset[1] = 1;

	/* We set the default master / stack factor, number of clients in the master area, whether
	 * to show the bar by default and its location based on the corresponding variables set in
	 * the configuration file. */
	m->mfact = mfact;
	m->nmaster = nmaster;
	m->showbar = showbar;
	m->topbar = topbar;

	/* This sets the first layout as selected by default (which is the tile layout as per the
	 * default configuration). */
	m->lt[0] = &layouts[0];

	/* This sets the previous layout as the last layout (which is the monocle layout as per the
	 * default configuration). */
	m->lt[1] = &layouts[1 % LENGTH(layouts)];

	/* This copies the layout symbol from the first layout into the monitor's layout symbol.
	 * This is later used when drawing the layout symbol on the bar. */
	strncpy(m->ltsymbol, layouts[0].symbol, sizeof m->ltsymbol);

	/* Return the newly created monitor. */
	return m;
}

/* This handles DestroyNotify events coming from the X server.
 *
 * This happens when a client application destroys a window.
 *
 * @called_from run (the event handler)
 * @calls wintoclient to find the client the given event is related to
 * @calls unmanage to stop managing the window and remove the client
 * @see https://tronche.com/gui/x/xlib/events/window-state-change/destroy.html
 * @see https://linux.die.net/man/3/xdestroywindowevent
 *
 * Internal call stack:
 *    run -> destroynotify
 */
void
destroynotify(XEvent *e)
{
	Client *c;
	XDestroyWindowEvent *ev = &e->xdestroywindow;

	/* Find the client the event is for. If the event is for a client window that is not managed
	 * by the window manager then we do nothing. */
	if ((c = wintoclient(ev->window)))
		/* Stop managing the window and remove the client. */
		unmanage(c, 1);
}

/* This removes a client from the monitor's client list.
 *
 * The client list is a linked list of client structures where one client refers to the next.
 * This list is primarily used:
 *    - for managing the order in which client windows are tiled and
 *    - to keep track of the clients that are managed by the window manager on a specific monitor
 *
 * Refer to the writeup for the attach function for usage warnings.
 *
 * @called_from pop to reattach a client to become the new master client
 * @called_from sendmon to attach the client to the new monitor
 *
 * Internal call stack:
 *    run -> keypress -> zoom -> pop -> detach
 *    run -> keypress -> tagmon -> sendmon -> detach
 *    run -> buttonpress -> movemouse / resizemouse -> sendmon -> detach
 *    run -> destroynotify / unmapnotify -> unmanage -> detach
 */
void
detach(Client *c)
{
	Client **tc;

	/* The use of double pointers (**tc) can make this piece of code challenging to read
	 * for users new to C who may not be that well versed in pointers and references.
	 *
	 * The problem scenario:
	 *    - we need to find the given client (c) in a linked list
	 *    - c refers to the next client in the list (n)
	 *    - then we to find the client prior (p) to client c
	 *    - then we need to have p refer to n instead of c in order to skip c
	 *
	 * An example implementation may look something like this:
	 *
	 *    Client *tc, *p;
	 *
	 *    if (c == c->mon->clients)
	 *       c->mon->clients = c->next;
	 *    else {
	 *       for (tc = c->mon->clients; tc && tc != c; p = tc && tc = tc->next);
	 *       p->next = c->next;
	 *    }
	 *
	 * By using double pointers the above is written with less lines of code as we are
	 * playing around with pointer addresses directly.
	 *
	 * Let's try to break this down.
	 *
	 * The variable **tc is declared as a pointer to a pointer to variable of type Client.
	 *
	 * The Monitor struct holds a pointer *clients to a variable of type Client.
	 *
	 * The tc variable is set to point to the address of c->mon->clients, as in it is pointing
	 * to the pointer in the Monitor struct.
	 *    tc = &c->mon->clients
	 *
	 * If c->mon->clients point to NULL or c->mon->clients is the given client, then stop
	 * the for loop.
	 *    *tc && *tc != c
	 *
	 * If the first client was the given client, then tc will still be referring to the
	 * pointer of c->mon->clients. So the next line would then just change the pointer of
	 * c->mon->clients to point to c->next.
	 *    *tc = c->next;
	 *
	 * But in the event that the given client was not the first client in the list we change
	 * tc to point to the address (&) of the pointer to the next client in the list.
	 *    &(*tc)->next
	 *
	 * When we hit the condition again we check if that next pointer points to NULL or if it
	 * points to the given client.
	 *    *tc && *tc != c
	 *
	 * If it doesn't then we keep moving the iterator from the next pointer to the next pointer
	 * rather than client to client until we find that the current next pointer refers to the
	 * given client, at which point we exit the for loop.
	 *
	 * What happens next is that we make the pointer pointing to the given client point to the
	 * next client in the list instead.
	 *    *tc = c->next;
	 */
	for (tc = &c->mon->clients; *tc && *tc != c; tc = &(*tc)->next);
	*tc = c->next;
}

/* This removes a client from the monitor's stacking order.
 *
 * The stacking order is a linked list of client structures where one client refers to the next.
 * This list is primarily used:
 *    - for managing the order in which client windows placed on top of each other
 *
 * Refer to the writeup for the attach function for usage warnings.
 *
 * @called_from sendmon to attach the client to the new monitor
 * @called_from updategeom when the number of monitors is reduced
 *
 * Internal call stack:
 *    ~ -> focus -> detachstack
 *    run -> keypress -> tagmon -> sendmon -> detachstack
 *    run -> buttonpress -> movemouse / resizemouse -> sendmon -> detachstack
 *    run -> destroynotify / unmapnotify -> unmanage -> detachstack
 *    run -> configurenotify -> updategeom -> detachstack
 */
void
detachstack(Client *c)
{
	Client **tc, *t;

	/* For a breakdown of what this does refer to the writeup in the detach function. */
	for (tc = &c->mon->stack; *tc && *tc != c; tc = &(*tc)->snext);
	*tc = c->snext;

	/* Additionally if the client being removed happens to be the selected client, then find
	 * the next visible client in the stack and set that to become the selected client. */
	if (c == c->mon->sel) {
		for (t = c->mon->stack; t && !ISVISIBLE(t); t = t->snext);
		c->mon->sel = t;
	}
}

/* This works out the next or previous monitor depending on whether the input direction is a
 * positive or negative number.
 *
 * This is used when moving clients across monitors or changing focus between monitors using
 * keybindings.
 *
 * @called_from tagmon to find the adjacent monitor to send a client to
 * @called_from focusmon to find the adjacent monitor to receive focus
 *
 * Internal call stack:
 *    run -> keypress -> tagmon / focusmon -> dirtomon
 */
Monitor *
dirtomon(int dir)
{
	Monitor *m = NULL;

	/* If we are looking for the next monitor ... */
	if (dir > 0) {
		/* ... then that makes things fairly easy, just pick the monitor coming after the
		 * selected monitor, but if this is the last monitor then pick the first monitor. */
		if (!(m = selmon->next))
			m = mons;
	/* Otherwise if we are looking for the previous monitor then we first need to check if the
	 * current monitor is the first monitor ... */
	} else if (selmon == mons)
		/* ... if it is then we loop through until we find the last monitor (which is not
		 * followed by another monitor). */
		for (m = mons; m->next; m = m->next);
	else
		/* If it isn't then loop through all monitors until we find the one that comes before
		 * the current monitor. */
		for (m = mons; m->next != selmon; m = m->next);
	return m;
}

/* This function handles the drawing of the bar.
 *
 * The logic that is applied here with regards to what is drawn must be accurately reflected in the
 * buttonpress function which works out what the user clicked on by going through the same logical
 * steps without drawing anything.
 *
 * @called from drawbars to update the bars on all monitors
 * @called from expose if the bar window is exposed (damaged)
 * @called from propertynotify if the selected client's title changes
 * @called from restack for convenience
 * @called from setlayout to update the bar after layout update
 * @called from updatestatus to update the bar after status update
 * @calls drw_setscheme to set the next colour scheme
 * @calls drw_text to draw text on the bar
 * @calls drw_rect to draw the client indicator on tags and the floating indicator for the title
 * @calls drw_map to place the finished drawing on the bar window
 *
 * Internal call stack:
 *    ~ -> arrange -> restack -> drawbar
 *    ~ -> focus -> drawbars -> drawbar
 *    run -> buttonpress -> restack -> drawbar
 *    run -> buttonpress -> movemouse / resizemouse -> restack -> drawbar
 *    run -> buttonpress -> setlayout -> drawbar
 *    run -> expose -> drawbar
 *    run -> keypress -> focusstack -> restack -> drawbar
 *    run -> keypress -> setlayout -> drawbar
 *    run -> setup -> updatestatus -> drawbar
 *    run -> propertynotify -> drawbars -> drawbar
 *    run -> propertynotify -> drawbar
 *    run -> propertynotify -> updatestatus -> drawbar
 */
void
drawbar(Monitor *m)
{
	/* Variables:
	 *    x - holds the x position within the bar window
	 *    w - holds temporary width values when drawing the bar
	 *    tw - short for text width, holds the status text width
	 *    i - common iterator
	 *    occ - bitmask that holds occupied tags
	 *    urg - bitmask that holds tags with clients that have the urgent flag set
	 *
	 * Then we have two variables in relation to the indicator used for occupied tags and for
	 * floating windows, which is a small square (i.e. a box).
	 *    boxs - this represents the box offset in terms of the x and y axis, it determines the
	 *           distance between the box and the top of the bar as well as the distance between
	 *           the box and the start of the tag. What the s stand for is not obvious.
	 *           Speculation but possibly this might be short for "box start".
	 *    boxw - this represents the box size in terms of the height and width, it determines
	 *           how big the box is. What the w stand for is not obvious here either, but one
	 *           can assume that it is short for "box width".
	 */
	int x, w, tw = 0;
	int boxs = drw->fonts->h / 9;
	int boxw = drw->fonts->h / 6 + 2;
	unsigned int i, occ = 0, urg = 0;
	Client *c;

	/* If the bar is not shown then don't spend any effort drawing the bar. As such hiding the
	 * bar has a positive effect on performance. */
	if (!m->showbar)
		return;

	/* Draw status first so it can be overdrawn by tags later. The main reason for this is that
	 * we want as much of the status shown as possible and it is just easier to draw the status
	 * first and let other things like tags overwrite it if necessary compared to having to
	 * calculate how much we can fit at a later time. */
	if (m == selmon) { /* status is only drawn on selected monitor */
		/* Set the normal colour scheme before drawing the text. This affects the foreground
		 * and background colour of the status text. */
		drw_setscheme(drw, scheme[SchemeNorm]);
		/* Calculate the width of the status text. The TEXTW macro includes lrpad by default
		 * and we do not want to include that here, we just want to know the size of the
		 * text. We also do not want the status crammed all the way to the edge of the bar,
		 * so we add 2 pixels worth of padding that will be added on the right hand side due
		 * to the position we start drawing the text from. */
		tw = TEXTW(stext) - lrpad + 2; /* 2px right padding */

		/* The below handles the actual drawing of the status text, the position calculated
		 * by subtracting the text width from the monitor's window width.
		 *
		 * There is more writeup on the drw_text function in drw.c, but since this is the
		 * first time we this in dwm.c let's have a quick breakdown.
		 *
		 *    drw_text(
		 *       drw,        - the drawable
		 *       m->ww - tw, - the x position
		 *       0,          - the y position
		 *       tw,         - the width
		 *       bh,         - the height
		 *       0,          - left padding (typically lrpad / 2)
		 *       stext,      - the text to be drawn
		 *       0           - inverted (swaps foreground and background colours)
		 *    );
		 *
		 * Notable here is the omission of an lrpad value and this has specifically to do
		 * with that we subtracted that from the text width. Another thing that may not be
		 * obvious to someone new to this is that we do not include the monitor position when
		 * passing the x and y values. This has to do with that the position is relative to
		 * the bar window and not the bar window's location.
		 */
		drw_text(drw, m->ww - tw, 0, tw, bh, 0, stext, 0);
	}

	/* This loops through all clients on the monitor and derives two bitmask variables
	 * indicating what tags are occupied by clients and what tags are occupied by urgent
	 * clients. */
	for (c = m->clients; c; c = c->next) {
		/* The or-equals operator means the union of occ and c->tags, it is short for
		 *    occ = occ | c->tags;
		 */
		occ |= c->tags;
		/* We do the same for urgent clients. */
		if (c->isurgent)
			urg |= c->tags;
	}

	/* This could have been initialised earlier to save on a single line of code, but as it
	 * stands it clearly indicates that here we start to draw from the beginning of the bar.
	 * This is a clarification following the above where we started drawing the status on the
	 * opposite side of the bar. */
	x = 0;
	/* We start by looping through all tags. */
	for (i = 0; i < LENGTH(tags); i++) {
		/* The user can define their own tag symbols (or text) so the width of each tag can
		 * differ from tag to tag. */
		w = TEXTW(tags[i]);

		/* Here we set the colour scheme to use when drawing the tag text. The gist of it is
		 * that we use SchemeSel if the tag is being viewed and SchemeNorm otherwise.
		 *
		 *    m->tagset[m->seltags] - this is the bitmask representing the viewed tags
		 *    1 << i                - this represents the bitmask for the tag we are
		 *                            currently processing
		 *    m->tags... & 1 << i   - the intersection between the two binaries will be true
		 *                            if the current tag is viewed
		 *
		 * After which we end up with either:
		 *
		 *    drw_setscheme(drw, scheme[SchemeSel]);
		 * or
		 *    drw_setscheme(drw, scheme[SchemeNorm]);
		 */
		drw_setscheme(drw, scheme[m->tagset[m->seltags] & 1 << i ? SchemeSel : SchemeNorm]);

		/* Draw the tag text (tags[i]). Note the last argument which inverts the colours of
		 * the tag if it is occupied by an urgent client. Invert in this context means to
		 * swap the foreground and background colours when drawing the text.
		 *
		 *    urg          - this is the bitmask representing tags with urgent clients
		 *    1 << i       - this represents the bitmask for the tag we are currently
		 *                   processing
		 *    urg & 1 << i - the intersection between the two binaries will be true if the
		 *                   current tag has urgent clients
		 */
		drw_text(drw, x, 0, w, bh, lrpad / 2, tags[i], urg & 1 << i);

		/* If the current tag is occupied by clients then draw the small indicator box. */
		if (occ & 1 << i)
			/* This draws a rectangle using boxs as an offset on the x and y axis and
			 * boxw as both the height and width.
			 *
			 * The last value urg & 1 << i indicates whether to use the background (1) or
			 * foreground (0) colour for the rectangle.
			 *
			 * The second to last value indicates whether the rectangle is filled / solid.
			 *
			 *    m == selmon && selmon->sel && selmon->sel->tags & 1 << i
			 *
			 * This is a mouthful but what this says is:
			 *
			 *    m == selmon &&             - if this is the selected monitor and
			 *    selmon->sel &&             - that monitor has a selected client and
			 *    selmon->sel->tags & 1 << i - that client is on the current tag
			 *                               = then fill the tag
			 *
			 * In more simple words you could say that the bar will show a solid box
			 * indicator for all tags where the currently focused client resides.
			 */
			drw_rect(drw, x + boxs, boxs, boxw, boxw,
				m == selmon && selmon->sel && selmon->sel->tags & 1 << i,
				urg & 1 << i);
		/* We are done drawing, move our draw "cursor" to the next tag. */
		x += w;
	}
	w = TEXTW(m->ltsymbol); /* The width of the layout symbol. */
	/* Reset the colour scheme back to normal. */
	drw_setscheme(drw, scheme[SchemeNorm]);
	/* Just draw the layout symbol. Note how the drw_text function returns how far the cursor
	 * moved while drawing the text. */
	x = drw_text(drw, x, 0, w, bh, lrpad / 2, m->ltsymbol, 0);

	/* This checks if there is any space left to draw the window title (while setting w to the
	 * remaining width at the same time). */
	if ((w = m->ww - tw - x) > bh) {
		/* If we have a selected client then show that client's window title. */
		if (m->sel) {
			/* Set the colour scheme to SchemeSel, but only if this is the selected
			 * monitor. */
			drw_setscheme(drw, scheme[m == selmon ? SchemeSel : SchemeNorm]);
			/* Just draw the window title. */
			drw_text(drw, x, 0, w, bh, lrpad / 2, m->sel->name, 0);
			/* If the selected client is floating then draw an indicator similar to that
			 * of tags. This will be a box of the same size and position that always uses
			 * the foreground colour and will be solid only in the event that the client
			 * is fixed in size (has the same minimum and maximum size hints). */
			if (m->sel->isfloating)
				drw_rect(drw, x + boxs, boxs, boxw, boxw, m->sel->isfixed, 0);
		/* If we do not have any clients then draw a blank space to clear anything that may
		 * have been drawn before (e.g. status text or a previous window title). */
		} else {
			/* Quite strictly the used colour scheme is still SchemeNorm as we set it
			 * before drawing the layout symbol, but we set it again to make it clear what
			 * colour scheme we expect this to have. */
			drw_setscheme(drw, scheme[SchemeNorm]);
			/* Here we draw a rectangle in the area where the window title would normally
			 * have been drawn. Note the explicit filled value of 1 a we want a solid
			 * block and the last inverted value of 1 which means we want the darker
			 * background colour. */
			drw_rect(drw, x, 0, w, bh, 1, 1);
		}
	}
	/* Finally place our finished drawing on the bar window by mapping it. */
	drw_map(drw, m->barwin, 0, 0, m->ww, bh);
}

/* This updates the bar on all monitors.
 *
 * @called_from focus to update the bars following focus changes
 * @called_from propertynotify to update the bars following update of window manager hints
 *
 * Internal call stack:
 *    ~ -> focus -> drawbars
 *    run -> propertynotify -> drawbars
 */
void
drawbars(void)
{
	Monitor *m;

	/* Loop through each monitor */
	for (m = mons; m; m = m->next)
		/* and update the bar for that monitor */
		drawbar(m);
}

/* This handles EnterNotify events coming from the X server.
 *
 * These kind of events can be received when the mouse cursor moves from one window to another,
 * as in the mouse cursor enters a new window.
 *
 * What this function primarily does is to change the selected monitor depending on what window the
 * event was for, but it will also handle focus changes between client windows as mouse cursor
 * enters windows. This is what is referred to as sloppy focus, as opposed to requiring the user to
 * click on windows to select them.
 *
 * @called_from run (the event handler)
 * @calls wintoclient to find the client the event is in relation to (if any)
 * @calls wintomon to find the monitor the event is in relation to if it is not related to a client
 * @calls unfocus to remove focus from the selected client when changing monitors
 * @calls focus to give input focus to a given client or the next in line when changing monitors
 * @see https://tronche.com/gui/x/xlib/events/window-entry-exit/normal.html
 * @see https://tronche.com/gui/x/xlib/events/window-entry-exit/#XCrossingEvent
 *
 * Internal call stack:
 *    run -> enternotify
 */
void
enternotify(XEvent *e)
{
	Client *c;
	Monitor *m;
	XCrossingEvent *ev = &e->xcrossing;

	/* The event mode can be:
	 *    - NotifyNormal
	 *    - NotifyGrab
	 *    - NotifyUngrab
	 *
	 * When the mouse pointer moves from window A to window B and A is an inferior of B, then
	 * the X server:
	 *    - generates a LeaveNotify event on window A, with the detail member of the
	 *      XLeaveWindowEvent structure set to NotifyAncestor
	 *    - generates a LeaveNotify event on each window between window A and window B,
	 *      exclusive, with the detail member of each XLeaveWindowEvent structure set to
	 *      NotifyVirtual
	 *    - generates an EnterNotify event on window B, with the detail member of the
	 *      XEnterWindowEvent structure set to NotifyInferior
	 *
	 * This next line says that:
	 *    - it accepts any events for the root window
	 *    - it will ignore events that have the NotifyGrab or NotifyUngrab mode and
	 *    - it will ignore events with NotifyNormal that have NotifyInferior as the event detail
	 *
	 * The reason for why events that have the event detail of NotifyInferior is ignored is not
	 * entirely clear.
	 */
	if ((ev->mode != NotifyNormal || ev->detail == NotifyInferior) && ev->window != root)
		return;

	/* Find the client this event is for (if any). */
	c = wintoclient(ev->window);

	/* If the event is in relation to a client then we use the client's monitor. If the event
	 * is not in relation to a client then call wintomon to work out which monitor this window
	 * is on. */
	m = c ? c->mon : wintomon(ev->window);

	/* If the monitor the event is related to is not the selected monitor, then we need to
	 * change monitor so that the one we found becomes the selected one. */
	if (m != selmon) {
		/* Before we change monitor we need to unfocus the selected client on the previous
		 * monitor and revert input focus to the root window. */
		unfocus(selmon->sel, 1);
		/* Setting the selected monitor to be monitor the event is in relation to. */
		selmon = m;
	/* If the monitor is the same and the event was not related to a client, or that client
	 * is the currently selected client, then bail. */
	} else if (!c || c == selmon->sel)
		return;

	/* Note that c may be NULL here, so this call either focuses on the client the event was
	 * for, or we give input focus to the last client that had focus on this monitor. */
	focus(c);
}

/* This handles Expose events coming from the X server.
 *
 * The X server can report Expose events to clients wanting information about when the contents of
 * window regions have been lost. A window region can get lost if another window overlaps it, in
 * which case the overlapped part of the window is damaged.
 *
 * When the other window overlapping it is moved then that damaged region of the window is exposed
 * (shown) to the user.
 *
 * In the context of dwm the window manager creates one window per monitor and this window is used
 * to show the bar. As such dwm is interested in knowing whether the bar window is damaged.
 *
 * If you look at the updatebars function then we indicate interest in being notified of expose
 * events by passing the ExposureMask event mask when the bar window is created.
 *
 *     XSetWindowAttributes wa = {
 *         .override_redirect = True,
 *         .background_pixmap = ParentRelative,
 *         .event_mask = ButtonPressMask|ExposureMask
 *     };
 *
 * Now this function is also called by the movemouse and resizemouse functions. This has to do with
 * that dwm is a single process program and when you click and drag a window around with the mouse
 * then the process is stuck in a while loop inside run -> buttonpress -> movemouse until you
 * release the button. In other words the event loop is held up while a move or resize takes place
 * and as such status updates stop happening as an example.
 *
 * While the mouse is being moved or resized the respective function checks the event queue for
 * certain events and Expose events are one of the ones checked:
 *
 *     XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
 *
 * This means that if an Expose event comes in while a window is being moved around then that will
 * be caught by the movemouse function which will forward the event to this function. This is to
 * ensure that the bar is redrawn if you move a window over the bar and away again.
 *
 * For a better understanding of this try removing ExposureMask| from the XMaskEvent in the
 * movemouse function and move a window over the bar. When the window is moved away then that will
 * leave a blank area where the bar was because the bar is no longer redrawn when these expose
 * events come through.
 *
 * It is worth nothing that if you apply the alpha patch then graphics are handled differently due
 * to transparency which means that the bar window will not be damaged by another window
 * overlapping it, thus there will be no Expose events coming through.
 *
 * @called_from run (the event handler)
 * @called_from movemouse as it forwards events to expose
 * @called_from resizemouse as it forwards events to expose
 * @calls wintomon to find the monitor the exposed window resides on
 * @calls drawbar to redraw the bar to repair any damage
 * @see updatebars where the bar window is created
 * @see movemouse for how it forwards events to certain event handlers
 * @see resizemouse for how it forwards events to certain event handlers
 * @see https://tronche.com/gui/x/xlib/events/exposure/expose.html
 *
 * Internal call stack:
 *    run -> expose
 *    run -> buttonpress -> movemouse / resizemouse -> expose
 */
void
expose(XEvent *e)
{
	Monitor *m;
	XExposeEvent *ev = &e->xexpose;
	/* We only enter the if statement to redraw the bar if the event count is 0. The event count
	 * indicates how many more of these events are waiting in the queue, and by checking for 0
	 * we make sure to only update the bar when there are no more events to process.
	 * The wintomon call works out which monitor the exposed bar window is on.
	 */
	if (ev->count == 0 && (m = wintomon(ev->window)))
		drawbar(m);
}

/* This function gives input focus to a given client.
 *
 * If the given client is NULL then focus will be given to the next visible client in the stacking
 * order. What this means is that the window that last had focus will receive input focus.
 *
 * If there are no more visible clients then input focus will be reverted to the root window.
 *
 * @called_from buttonpress if the event results in focus going to a different monitor
 * @called_from configurenotify if monitor setup changes
 * @called_from enternotify to focus on window entered with the mouse cursor
 * @called_from focusmon to focus on the window that last had focus on a monitor
 * @called_from focusstack to focus on the next or previous client in the client list
 * @called_from manage to focus on the newly managed client
 * @called_from motionnotify to focus on the window that last had focus on a monitor
 * @called_from movemouse to give input focus to the client after moving it to another monitor
 * @called_from resizemouse to give input focus to the client after resizing it to another monitor
 * @called_from pop to focus on the client becoming the new master
 * @called_from sendmon to set focus after moving a client to another monitor
 * @called_from setup to revert input focus back to the root window
 * @called_from tag to give focus to the next client in the stack after moving a client to a tag
 * @called_from toggletag to give focus to the next client in the stack after toggling a tag
 * @called_from toggleview to give focus after toggling tags into or out of view
 * @called_from unmanage to give focus to the next client in the stack after unmanaging a client
 * @called_from view to give focus after changing view
 * @calls XSetWindowBorder https://tronche.com/gui/x/xlib/window/XSetWindowBorder.html
 * @calls XSetInputFocus https://tronche.com/gui/x/xlib/input/XSetInputFocus.html
 * @calls XDeleteProperty https://tronche.com/gui/x/xlib/window-information/XDeleteProperty.html
 * @calls unfocus to unfocus and restore window border for the previously selected window
 * @calls seturgent to remove urgency flag if set
 * @calls detachstack to place the client window at the top of the stacking order
 * @calls attachstack to place the client window at the top of the stacking order
 * @calls grabbuttons as we listen for different button presses for a window that has focus
 * @calls setfocus to give the target client input focus
 * @calls drawbars to update the bars on all monitors
 * @see https://tronche.com/gui/x/xlib/events/input-focus/
 *
 * Internal call stack:
 *    run -> buttonpress -> focus
 *    run -> buttonpress -> movemouse / resizemouse -> focus
 *    run -> buttonpress -> movemouse / resizemouse -> sendmon -> focus
 *    run -> buttonpress -> tag -> focus
 *    run -> buttonpress -> toggletag -> focus
 *    run -> buttonpress -> toggleview -> focus
 *    run -> buttonpress -> view -> focus
 *    run -> configurenotify -> focus
 *    run -> destroynotify / unmapnotify -> unmanage -> focus
 *    run -> enternotify -> focus
 *    run -> keypress -> focusmon -> focus
 *    run -> keypress -> focusstack -> focus
 *    run -> keypress -> zoom -> pop -> focus
 *    run -> keypress -> tagmon -> sendmon -> focus
 *    run -> keypress -> tag -> focus
 *    run -> keypress -> toggletag -> focus
 *    run -> keypress -> toggleview -> focus
 *    run -> keypress -> view -> focus
 *    run -> maprequest -> manage -> focus
 *    run -> motionnotify -> focus
 *    main -> setup -> focus
 *    main -> cleanup -> view -> focus
 */
void
focus(Client *c)
{
	/* If the given client is NULL, or it happens to not be visible, then search the first
	 * visible client in the stacking order list. */
	if (!c || !ISVISIBLE(c))
		for (c = selmon->stack; c && !ISVISIBLE(c); c = c->snext);
	/* If there is a selected client on the current monitor and that is different to the client
	 * receiving focus, then we call unfocus on that client. */
	if (selmon->sel && selmon->sel != c)
		unfocus(selmon->sel, 0);
	/* If we are giving focus to a specific client then */
	if (c) {
		/* if that client is on a different monitor than the selected monitor, then we set
		 * the selected monitor to be that of the client. */
		if (c->mon != selmon)
			selmon = c->mon;
		/* If the client has the urgency flag set then we remove that as the window is
		 * receiving attention now. */
		if (c->isurgent)
			seturgent(c, 0);
		/* The detachstack and attachstack calls makes it so that the client becomes on top
		 * of the stacking order, making it the last window to have received focus. */
		detachstack(c);
		attachstack(c);
		/* We grab buttons for the window again as we are listening on less button presses
		 * for windows that have input focus. */
		grabbuttons(c, 1);
		/* We change the colour of the window border to give a visual clue as to what window
		 * has focus. */
		XSetWindowBorder(dpy, c->win, scheme[SchemeSel][ColBorder].pixel);
		/* The call to setfocus handles the actual library calls to give the window input
		 * focus. This is handled separately as setfocus can also be called from the focusin
		 * event handler function. */
		setfocus(c);
	/* It may be that there are no visible clients on the monitor, in which case we revert the
	 * input focus back to the root window. */
	} else {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		/* We also delete the _NET_ACTIVE_WINDOW property on the root window as this property
		 * tells other windows which window is currently active - and at this moment the
		 * window manager has no window that is active. */
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}
	/* Set the selected client to be the one receiving focus, or to NULL in the event that there
	 * are no visible clients. */
	selmon->sel = c;
	/* Finally update the bars on all monitors. This is in case the focus change resulted in the
	 * selected monitor changing. */
	drawbars();
}

/* This handles FocusIn events coming from the X server.
 *
 * There are some broken focus acquiring windows that needs extra handling to work properly.
 *
 * @called_from run (the event handler)
 * @calls setfocus to tell the window to take focus
 * @see https://tronche.com/gui/x/xlib/events/input-focus/
 *
 * Internal call stack:
 *    run -> focusin
 */
void
focusin(XEvent *e)
{
	XFocusChangeEvent *ev = &e->xfocus;

	/* This sets focus back to the selected window if the window the event is for is not the
	 * window for the selected client. */
	if (selmon->sel && ev->window != selmon->sel->win)
		setfocus(selmon->sel);
}

/* User function to focus on an adjacent monitor in a given direction.
 *
 * @called_from keypress in relation to keybindings
 * @calls dirtomon to work out the next monitor in the given direction
 * @calls unfocus to unfocus the selected client on the current monitor
 * @calls focus to focus on the last focused client on the next monitor
 *
 * Internal call stack:
 *    run -> keypress -> focusmon
 */
void
focusmon(const Arg *arg)
{
	Monitor *m;

	/* Bail if this is a single monitor setup */
	if (!mons->next)
		return;

	/* Find the next monitor in a given direction. There is a guard here to bail if the monitor
	 * returned by dirtomon is the same monitor as we are currently on. In principle this should
	 * never happen. */
	if ((m = dirtomon(arg->i)) == selmon)
		return;

	/* Unfocus the selected client (if any) on the current monitor before changing focus. */
	unfocus(selmon->sel, 0);

	/* Set the selected monitor to the next monitor. */
	selmon = m;

	/* Focus on the last focused window on the new monitor (if any). */
	focus(NULL);
}

/* User function to change focus between visible windows on the selected monitor.
 *
 * @called_from keypress in relation to keybindings
 * @calls focus to give input focus to the next client
 * @calls restack to place the selected client, if floating, above other floating windows
 *
 * Internal call stack:
 *    run -> keypress -> focusmon
 */
void
focusstack(const Arg *arg)
{
	Client *c = NULL, *i;

	/* Bail if there is no selected client on the current monitor, or if the selected client is
	 * fullscreen and we disallow focus to drift from fullscreen windows. */
	if (!selmon->sel || (selmon->sel->isfullscreen && lockfullscreen))
		return;

	/* If the input value is positive then we move forward to find the next visible client. */
	if (arg->i > 0) {
		/* This searches through the client list for the next visible client. */
		for (c = selmon->sel->next; c && !ISVISIBLE(c); c = c->next);
		/* If we have exhausted the list and there are no more visible clients, then we wrap
		 * around and search for the first visible client in the list. */
		if (!c)
			for (c = selmon->clients; c && !ISVISIBLE(c); c = c->next);
	/* Otherwise we move backward to find the prior visible client. */
	} else {
		/* Start from the beginning of the linked list and find the last visible client that
		 * is not the selected client. */
		for (i = selmon->clients; i != selmon->sel; i = i->next)
			if (ISVISIBLE(i))
				c = i;
		/* If there are no visible clients prior to the selected one then we simply continue
		 * where the previous for loop left off and we search for the very last visible
		 * client. */
		if (!c)
			for (; i; i = i->next)
				if (ISVISIBLE(i))
					c = i;
	}
	/* If we did find a client, and in principle we should as there is at least one visible
	 * window, then we give input focus to that client. */
	if (c) {
		focus(c);
		/* The explicit restack here is in the event that the client window is floating, or
		 * we are using the floating layout, in which case we want the selected client to be
		 * placed above other floating windows. */
		restack(selmon);
	}
}

/* This reads a property value of a given atom for a client's window.
 *
 * In dwm this is used to read a client's window state as well as window type.
 *
 * @called_from updatewindowtype to retrieve the window state and window type
 * @calls XGetWindowProperty https://tronche.com/gui/x/xlib/window-information/XGetWindowProperty.html
 * @calls XFree https://tronche.com/gui/x/xlib/display/XFree.html
 *
 * Internal call stack:
 *    run -> propertynotify -> updatewindowtype -> getatomprop
 *    run -> maprequest -> manage -> updatewindowtype -> getatomprop
 */
Atom
getatomprop(Client *c, Atom prop)
{
	/* Here we have three dummy variables that we need to pass to XGetWindowProperty but the
	 * values that are written we simply ignore.
	 *
	 * The dummy variables are:
	 *    di - dummy integer
	 *    dl - dummy unsigned long
	 *    da - dummy atom
	 */
	int di;
	unsigned long dl;
	unsigned char *p = NULL; /* The prop_return variable for the of XGetWindowProperty call. */
	Atom da, atom = None;

	/* This reads the given window property. If the property could be read successfully then
	 * we enter the if statement, otherwise we end up returning a default atom of None. */
	if (XGetWindowProperty(dpy, c->win, prop, 0L, sizeof atom, False, XA_ATOM,
		&da, &di, &dl, &dl, &p) == Success && p) {
		/* Capture the value of the prop_return to our local atom which we will return. */
		atom = *(Atom *)p;
		/* Free the prop_return variable. */
		XFree(p);
	}
	return atom;
}

/* This is a wrapper function that gets the mouse pointer coordinates and stores those in the
 * given x and y pointers.
 *
 * @called_from wintomon to find the monitor where the mouse pointer is
 * @called_from movemouse as it needs the mouse coordinates for calculation reasons
 * @calls XQueryPointer https://tronche.com/gui/x/xlib/window-information/XQueryPointer.html
 *
 * Internal call stack:
 *    run -> buttonpress -> movemouse -> getrootptr
 *    run -> configurenotify -> updategeom -> wintomon -> getrootptr
 *    main -> setup -> updategeom -> wintomon -> getrootptr
 */
int
getrootptr(int *x, int *y)
{
	/* These are all dummy variables that are only there because we need to have something
	 * to pass to XQueryPointer. We are only interested in two values which are the x and y
	 * coordinates of the mouse. The dummy values are simply ignored. */
	int di; /* dummy int */
	unsigned int dui; /* dummy unsigned int */
	Window dummy;

	return XQueryPointer(dpy, root, &dummy, &dummy, x, y, &di, &di, &dui);
}

/* This function retrieves the window state for a given window.
 *
 * The window can be in one of the following states:
 *
 *    NormalState     - a normal visible window
 *    WithdrawnState  - a window that is not visible to the user in any way
 *    IconicState     - a window that is not visible to the user, but may be represented by an icon
 *                      in a taskbar as an example - more commonly one would refer to such windows
 *                      as being minimised
 *
 * @called_from scan to check if a window is in iconic state
 * @calls XGetWindowProperty https://tronche.com/gui/x/xlib/window-information/XGetWindowProperty.html
 * @calls XFree https://tronche.com/gui/x/xlib/display/XFree.html
 * @see https://tronche.com/gui/x/xlib/ICC/client-to-window-manager/wm-hints.html
 *
 * Internal call stack:
 *    main -> scan -> getstate
 */
long
getstate(Window w)
{
	int format;
	long result = -1;
	unsigned char *p = NULL;
	unsigned long n, extra;
	Atom real;

	/* This reads the WM_STATE property of a given window. If the property could not be read
	 * then -1 will be returned. */
	if (XGetWindowProperty(dpy, w, wmatom[WMState], 0L, 2L, False, wmatom[WMState],
		&real, &format, &n, &extra, (unsigned char **)&p) != Success)
		return -1;
	/* If the property had a value then set that as the function's return value. */
	if (n != 0)
		result = *p;
	/* Data returned by XGetWindowProperty must be freed by the caller. */
	XFree(p);

	/* Returns the value of the WM_STATE property or -1 in the exceptional event that the
	 * property had no value. */
	return result;
}

/* This reads a text property of a given window and copies the data to the designated string.
 *
 * In dwm this is used to read the root window name property to update the status text and to read
 * a client's window title.
 *
 * The size argument restricts how many bytes are written to the designated text variable.
 *
 * @called_from updatestatus to update the status text when the root window name changes
 * @called_from updatetitle to read the window title of a client window
 * @calls XGetTextProperty https://tronche.com/gui/x/xlib/ICC/client-to-window-manager/XGetTextProperty.html
 * @calls XmbTextPropertyToTextList https://tronche.com/gui/x/xlib/ICC/client-to-window-manager/XmbTextPropertyToTextList.html
 * @calls XFreeStringList https://tronche.com/gui/x/xlib/ICC/client-to-window-manager/XFreeStringList.html
 * @calls XFree https://tronche.com/gui/x/xlib/display/XFree.html
 * @calls strncpy to copy the text property string to a given variable
 * @see https://tronche.com/gui/x/xlib/ICC/client-to-window-manager/converting-string-lists.html
 *
 * Internal call stack:
 *    run -> setup -> updatestatus -> gettextprop
 *    run -> propertynotify -> updatestatus -> gettextprop
 *    run -> propertynotify -> updatetitle -> gettextprop
 *    run -> maprequest -> manage -> updatetitle -> gettextprop
 */
int
gettextprop(Window w, Atom atom, char *text, unsigned int size)
{
	/* The double pointer here has to do with the interface of the XmbTextPropertyToTextList
	 * function expecting that. */
	char **list = NULL;
	int n;
	XTextProperty name;

	/* Safeguards, bail if being passed bad values. */
	if (!text || size == 0)
		return 0;
	/* Set text to be an empty string. */
	text[0] = '\0';
	/* If we could not read the text property, or the text property could be read but contained
	 * no data, then we exit early indicating that data was not read by returning 0 (false). */
	if (!XGetTextProperty(dpy, w, &name, atom) || !name.nitems)
		return 0;

	/* The property data can either be a text string or it can be a list of text strings. */
	if (name.encoding == XA_STRING) {
		/* If it is a text string then just copy the data to the designated text variable. */
		strncpy(text, (char *)name.value, size - 1);
	/* If it is a list of text strings then we need to retrieve it by calling
	 * XmbTextPropertyToTextList and the returned list needs to be freed when we are
	 * finished with it. */
	} else if (XmbTextPropertyToTextList(dpy, &name, &list, &n) >= Success && n > 0 && *list) {
		strncpy(text, *list, size - 1);
		XFreeStringList(list);
	}
	/* This is to make sure that the end of the text marks the end of the string, in the event
	 * that the property held more data than we could store. */
	text[size - 1] = '\0';
	/* Free the XTextProperty value returned by XGetTextProperty. */
	XFree(name.value);
	return 1;
}

/* This tells the X server what mouse button press scenarios we are interested in receiving
 * notifications for.
 *
 * @called_by focus because we subscribe to different button notifications for focused windows
 * @called_by unfocus because we subscribe to different button notifications for unfocused windows
 * @called_by manage to grab buttons in case the client starts on another tag due to client rules
 * @calls XUngrabButton https://tronche.com/gui/x/xlib/input/XUngrabButton.html
 * @calls XGrabButton https://tronche.com/gui/x/xlib/input/XGrabButton.html
 * @calls updatenumlockmask to make sure the Num Lock modifier is correct
 *
 * Internal call stack:
 *    ~ -> focus -> grabbuttons
 *    ~ -> unfocus -> grabbuttons
 *    run -> maprequest -> manage -> grabbuttons
 */
void
grabbuttons(Client *c, int focused)
{
	/* First make sure that we have the right modifier for the Num Lock key */
	updatenumlockmask();
	/* Technically there isn't any practical reason why the below is placed within a separate
	 * block, but it is most likely done so because the function otherwise breaks the general
	 * pattern of declaring all variables at the start of the function, but here we need to make
	 * the call to updatenumlockmask() first because we use the numlockmask variable when
	 * declaring the modifiers array. The alternative could be to place all of the below in a
	 * separate function but that would be less clean than simply adding it all in a block. */
	{
		unsigned int i, j;
		/* The list of modifiers we are interested in. No additional modifier, the Caps Lock
		 * mask, the Num Lock mask, and Caps Lock and Num Lock mask together. */
		unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
		/* Call to release any buttons we may have grabbed before. */
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);

		/* If the client is not focused then we are interested in any button press activity
		 * related to the client window. Only the focus function calls grabbuttons passing
		 * focused as 1.
		 */
		if (!focused)
			XGrabButton(dpy, AnyButton, AnyModifier, c->win, False,
				BUTTONMASK, GrabModeSync, GrabModeSync, None, None);

		/* Loop through all the button bindings as defined in the configuration file and
		 * look for all bindings related to clicking on a client window (ClkClientWin).
		 *
		 * As a practical example let's look at this button binding:
		 *
		 *    { ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
		 *
		 * The binding is for clicking MOD+left mouse button (Button1) on a client window
		 * to move it around.
		 *
		 * To make this happen we need to let the X server know that we want to receive a
		 * ButtonPress event if the user holds down the modifier key and clicks using the
		 * left mouse button on the given window. More so we want this to work regardless
		 * of whether Num Lock or Caps Lock is enabled.
		 *
		 * The inner for loop runs through the modifiers that we listed in the modifiers
		 * array earlier and combines each modifier with the modifier defined in the button
		 * bindings array.
		 *
		 * This will lead to the window manager receiving ButtonPress notifications for this
		 * window in the following scenarios:
		 *    - user holds down MODKEY and clicks Button1
		 *    - user holds down MODKEY and clicks Button1 while Num Lock is on
		 *    - user holds down MODKEY and clicks Button1 while Lock is on
		 *    - user holds down MODKEY and clicks Button1 while both Num Lock and Lock is on
		 */
		for (i = 0; i < LENGTH(buttons); i++)
			if (buttons[i].click == ClkClientWin)
				/* Loop through all the modifiers we are interested in */
				for (j = 0; j < LENGTH(modifiers); j++)
					/* Grab the button to tell the X server that we are interested
					 * in receiving ButtonPress notifications when the user clicks
					 * on the button in combination with the given modifier. */
					XGrabButton(dpy, buttons[i].button,
						buttons[i].mask | modifiers[j],
						c->win, False, BUTTONMASK,
						GrabModeAsync, GrabModeSync, None, None);
	}
}

/* This tells the X server what key press scenarios we are interested in receiving notifications
 * for.
 *
 * @called_from mappingnotify in the event of keyboard or keyboard layout change
 * @called_from setup to grab the keys initially
 * @calls XDisplayKeycodes https://tronche.com/gui/x/xlib/input/XDisplayKeycodes.html
 * @calls XUngrabKey https://tronche.com/gui/x/xlib/input/XUngrabKey.html
 * @calls XGetKeyboardMapping https://tronche.com/gui/x/xlib/input/XGetKeyboardMapping.html
 * @calls XGrabKey https://tronche.com/gui/x/xlib/input/XGrabKey.html
 * @calls updatenumlockmask
 * @see https://tronche.com/gui/x/xlib/utilities/keyboard/
 *
 * Internal call stack:
 *    run -> mappingnotify -> grabkeys
 *    main -> setup -> grabkeys
 */
void
grabkeys(void)
{
	/* First make sure that we have the right modifier for the Num Lock key */
	updatenumlockmask();
	/* Technically there isn't any practical reason why the below is placed within a separate
	 * block, but it is most likely done so because the function otherwise breaks the general
	 * pattern of declaring all variables at the start of the function, but here we need to make
	 * the call to updatenumlockmask() first because we use the numlockmask variable when
	 * declaring the modifiers array. The alternative could be to place all of the below in a
	 * separate function but that would be less clean than simply adding it all in a block. */
	{
		unsigned int i, j, k;
		/* The list of modifiers we are interested in. No additional modifier, the Lock mask,
		 * the Num Lock mask, and Lock and Num Lock mask together. */
		unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
		int start, end, skip;
		KeySym *syms;

		/* Call to release any keys we may have grabbed before. */
		XUngrabKey(dpy, AnyKey, AnyModifier, root);

		/* It is not uncommon for a keysym to map to multiple keycodes. Here we make a call to
		 * XDisplayKeycodes to get the number of keycodes that are supported by the specific
		 * display, then we call XGetKeyboardMapping to get all the keysym for all key code within
		 * the given range.
		 *
		 * The reasoning here is that if we have a keybinding for, say, XF86AudioPlay then we want
		 * to subscribe to all key press events for all codes that XF86AudioPlay maps to.
		 *
		 * Take the following example:
		 *
		 *     % xmodmap -pke | grep XF86AudioPlay
		 *     keycode 172 = XF86AudioPlay XF86AudioPause XF86AudioPlay XF86AudioPause
		 *     keycode 208 = XF86AudioPlay NoSymbol XF86AudioPlay
		 *     keycode 215 = XF86AudioPlay NoSymbol XF86AudioPlay
		 *
		 * Here the keyboard may be sending the keycode of 172 while a pair of headphones may be
		 * sending through the keycode of 215. In order for both the keyboard and headphones to
		 * work we need to grab both keys when going through and setting up keybindings.
		 */
		XDisplayKeycodes(dpy, &start, &end);
		/* This may not be immediately obvious, but the skip variable here will hold the number of
		 * keysyms per key code. This is later used to know how many items to skip when looking up
		 * keysyms for a given key code, hence the variable name of "skip". */
		syms = XGetKeyboardMapping(dpy, start, end - start + 1, &skip);
		/* In the unlikely event that XGetKeyboardMapping can't allocate memory to hold the
		 * returned syms then syms will be NULL. This is a scenario that is unlikely to happen in
		 * practice and the code is primarily just a precaution. */
		if (!syms)
			return;

		/* Loop through all the key bindings as defined in the configuration file.
		 *
		 * As a practical example let's look at this key binding:
		 *
		 *    { MODKEY,                       XK_b,      togglebar,      {0} },
		 *
		 * The binding is for clicking MOD+b to toggle the display of the bar on and off.
		 *
		 * To make this happen we need to let the X server know that we want to receive a
		 * KeyPress event if the user holds down the modifier key and clicks the b key.
		 * More so we want this to work regardless of whether Num Lock or Caps Lock is
		 * enabled.
		 *
		 * The inner for loop runs through the modifiers that we listed in the modifiers
		 * array earlier and combines each with the modifier defined in the key bindings
		 * array.
		 *
		 * This will lead to the window manager receiving KeyPress notifications in the
		 * following scenarios:
		 *    - user holds down MODKEY and clicks b
		 *    - user holds down MODKEY and clicks b while Num Lock is on
		 *    - user holds down MODKEY and clicks b while Caps Lock is on
		 *    - user holds down MODKEY and clicks b while both Num Lock and Caps Lock is on
		 *
		 * It is worth noting that dwm only uses top level key bindings. For example
		 * consider the default keybinding for killclient which is MOD+Shift+c:
		 *
		 *    { MODKEY|ShiftMask,             XK_c,      killclient,     {0} },
		 *
		 * The Shift key will generate an upper case C, so one might be inclined to think
		 * that the following would work as well:
		 *
		 *    { MODKEY,                       XK_C,      killclient,     {0} },
		 *
		 * but it will not work, however, due to how the logic in the keypress function
		 * handles modifiers, key codes and keysyms.
		 */
		for (k = start; k <= end; k++)
			for (i = 0; i < LENGTH(keys); i++)
				/* Check that the keysym listed in the keys array matches the key. The skip
				 * variable holds the number of keysyms per key code, so we multiply that to
				 * find the first keysym for a given key code. Note that we only match on the
				 * very first keysym for a key (not all keysyms for all keys).
				 *
				 * This has to do with how dwm intentionally only supports top level keysyms as
				 * due to how modifier keys are handled, e.g. XK_colon is not supported but
				 * XK_semicolon is.
				 *
				 *    $ xmodmap -pke | grep colon
				 *    keycode  47 = semicolon colon semicolon colon dead_acute dead_doubleacute
				 *                  dead_acute
				 */
				if (keys[i].keysym == syms[(k - start) * skip])
					/* Loop through all the modifiers we are interested in */
					for (j = 0; j < LENGTH(modifiers); j++)
						/* Grab the key to tell the X server that we are interested
						 * in receiving KeyPress notifications when the user clicks
						 * on the key in combination with the given modifier. */
						XGrabKey(dpy, k,
							keys[i].mod | modifiers[j],
							root, True,
							GrabModeAsync, GrabModeAsync);
		XFree(syms);
	}
}

/* User function to increment or decrement the number of client windows in the master area.
 *
 * @called_from keypress in relation to keybindings
 * @calls updatebarpos to adjust the monitor's window area
 * @calls XMoveResizeWindow https://tronche.com/gui/x/xlib/window/XMoveResizeWindow.html
 * @calls arrange to reposition and resize tiled clients
 *
 * Internal call stack:
 *    run -> keypress -> incnmaster
 */
void
incnmaster(const Arg *arg)
{
	/* This adjusts the number of master (nmaster) clients with the given argument. The
	 * MAX(..., 0) is just a safeguard to prevent the nmaster value from becoming negative. */
	selmon->nmaster = MAX(selmon->nmaster + arg->i, 0);
	/* A full arrange to resize and reposition clients accordingly. In principle this could have
	 * been an arrangemon call as we do not need to bring new clients into view, apply a restack
	 * or to update the bar. */
	arrange(selmon);
}

#ifdef XINERAMA
/* Xinerama can give multiple geometries when querying for screens and we only want to consider
 * unique geometries as separate monitors. This helper function is used by the updategeom function
 * to luke out duplicate geometries by checking if the x and y positions as well as the width and
 * height are identical to previous screens processed.
 *
 * @called_by updategeom to de-duplicate geometries returned by XineramaQueryScreens
 *
 * Internal call stack:
 *    run -> configurenotify -> updategeom -> isuniquegeom
 *    main -> setup -> updategeom -> isuniquegeom
 */
static int
isuniquegeom(XineramaScreenInfo *unique, size_t n, XineramaScreenInfo *info)
{
	/* n in this case refers to how many rows worth of data the unique variable has. The while
	 * loop ensures that we go through and compare each of the rows in our unique array with
	 * the data in the given xinerama info. */
	while (n--)
		/* If the given xinerama info matches any of the existing rows in our unique array
		 * then we return 0, the xinerama info is not unique. */
		if (unique[n].x_org == info->x_org && unique[n].y_org == info->y_org
		&& unique[n].width == info->width && unique[n].height == info->height)
			return 0;
	return 1;
}
#endif /* XINERAMA */

/* This handles KeyPress events coming from the X server.
 *
 * @called_from run (the event handler)
 * @calls XKeycodeToKeysym https://tronche.com/gui/x/xlib/utilities/keyboard/XKeycodeToKeysym.html
 * @calls functions as defined in the keys array
 * @see grabkeys for how the window manager subscribes to key presses
 *
 * Internal call stack:
 *    run -> keypress
 */
void
keypress(XEvent *e)
{
	unsigned int i;
	KeySym keysym;
	XKeyEvent *ev;

	ev = &e->xkey;

	/* The XKeycodeToKeysym function uses internal Xlib tables and returns the keysym defined
	 * for the given key code. The last argument of 0 is for the index of the key code vector
	 * which means that we are only interested in top level keysyms.
	 *
	 * As a demonstration let's run the xev (event tester) tool and press the "d" key on the
	 * keyboard. The output of xev should say something along the lines of:
	 *
	 *    KeyPress event, serial 35, synthetic NO, window 0x8400001,
	 *    root 0x6be, subw 0x0, time 29383033, (56,550), root:(5618,835),
	 *    state 0x0, keycode 40 (keysym 0x64, d), same_screen YES,
	 *    XLookupString gives 1 bytes: (64) "d"
	 *    XmbLookupString gives 1 bytes: (64) "d"
	 *    XFilterEvent returns: False
	 *
	 * The XKeyEvent key code (ev->keycode) in this scenario would have the value of 40 as
	 * shown shown in the output above. The keysym returned by the XKeycodeToKeysym function
	 * would have the decimal value of 100 (hexadecimal 0x64) which is XK_d because we
	 * specifically asked for the top level symbolic key. If we had passed 1 to get the second
	 * level keys then the keysym returned would be decimal 68 (hexadecimal 0x44) which is XK_D.
	 *
	 * For simplicity dwm only supports keybindings using top level keybindings. As such
	 * keybindings involving the shift key will be including the ShiftMask in the modifier key
	 * rather than using second level keysyms for the key. E.g.
	 *
	 *    { MODKEY|ShiftMask,             XK_c,      killclient,     {0} },
	 *
	 * Removing the ShiftMask and adding XK_C as the key will not work due to how the ShiftMask
	 * is taken into account when comparing the modifier and the event state.
	 */
	keysym = XKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0);
	/* Loop through each keybinding */
	for (i = 0; i < LENGTH(keys); i++)
		/* Let's break down the matching process here with an example.
		 *
		 *      modifier                      key        function        argument
		 *    { MODKEY,                       XK_i,      incnmaster,     {.i = +1 } },
		 *
		 * This checks that the keysym, e.g. XK_i, matches:
		 *    keysym == keys[i].keysym
		 *
		 * This checks that the modifier matches:
		 *    && CLEANMASK(keys[i].mod) == CLEANMASK(ev->state)
		 *
		 * Keybindings that do not have a function are simply ignored:
		 *    && keys[i].func
		 *
		 * It is worth noting that the CLEANMASK macro removes Num Lock and Caps Lock mask
		 * from the modifier as well as the event state. This has to do with that we want
		 * these keybindings to work regardless of whether Num Lock and/or Caps Lock is
		 * enabled or not. See the grabkeys function for how we subscribe to all of the key
		 * combinations to cover this.
		 */
		if (keysym == keys[i].keysym
		&& CLEANMASK(keys[i].mod) == CLEANMASK(ev->state)
		&& keys[i].func)
			/* This calls the function associated with the keybinding with the given
			 * argument, e.g. calling incnmaster with the +1 argument. */
			keys[i].func(&(keys[i].arg));
			/* Note that there is no break; following this, which means that we will
			 * continue searching through the key bindings for more matches. As such
			 * it is possible to have more than one thing happen when a key combination
			 * is pressed by having the same keybinding multiple times referring to
			 * different functions. */
}

/* User function to close the selected client,
 *
 * @called_from keypress in relation to keybindings
 * @calls sendevent telling the window to terminate
 * @calls XGrabServer https://tronche.com/gui/x/xlib/window-and-session-manager/XGrabServer.html
 * @calls XSetErrorHandler https://tronche.com/gui/x/xlib/event-handling/protocol-errors/XSetErrorHandler.html
 * @calls XKillClient https://tronche.com/gui/x/xlib/window-and-session-manager/XKillClient.html
 * @calls XSync https://tronche.com/gui/x/xlib/event-handling/XSync.html
 * @calls XUngrabServer https://tronche.com/gui/x/xlib/window-and-session-manager/XGrabServer.html
 *
 * Internal call stack:
 *    run -> keypress -> killclient -> sendevent
 */
void
killclient(const Arg *arg)
{
	if (!selmon->sel)
		return;
	/* This sends an event with the WM_DELETE_WINDOW property to the selected client's window
	 * giving it a chance to handle the termination itself.
	 *
	 * The event is sent if the window has WM_DELETE_WINDOW listed as a protocol under the
	 * WM_PROTOCOLS property of the window.
	 *
	 * $ xprop | grep WM_PROTOCOLS
	 * WM_PROTOCOLS(ATOM): protocols  WM_DELETE_WINDOW, _NET_WM_PING
	 *
	 * If the window does not have that property then we enter this if statement and take
	 * hostile actions against that window.
	 *
	 * Most windows will have this protocol set so we rarely end up inside this if statement.
	 *
	 * Note that sending this message to the window does not in any way guarantee that the
	 * window will be closed, it is just a request telling the application that we want the
	 * window to close and it is up to the application to handle that.
	 *
	 * An example of this is killing an editor which receives the delete window event and
	 * then asks the user whether they want to save the file before closing. The user in this
	 * case often have the option to cancel to stop the window from closing.
	 */
	if (!sendevent(selmon->sel, wmatom[WMDelete])) {
		/* This disables processing of requests and close downs on all other connections than
		 * the one this request arrived on. */
		XGrabServer(dpy);
		/* The dummy X error handler is set so that in case the X server blows any errors in
		 * relation to what we are about to do next then those errors are simply ignored. */
		XSetErrorHandler(xerrordummy);
		/* This defines what will happen to the client's resources at connection close */
		XSetCloseDownMode(dpy, DestroyAll);
		/* This call forces a close-down of the client that created the resource */
		XKillClient(dpy, selmon->sel->win);
		/* This flushes the output buffer and then waits until all requests have been
		 * received and processed by the X server. */
		XSync(dpy, False);
		/* Revert to the normal X error handler */
		XSetErrorHandler(xerror);
		/* This restarts processing of requests and close downs on other connections */
		XUngrabServer(dpy);
	}
}

/* The manage function is what makes the window manager manage the given window. It determines how
 * the window is going to be managed based on client rules and various window properties and
 * states. A managed window is represented by a client, which in turn is added to the client list
 * and stacking order list for the designated monitor.
 *
 * @called_from maprequest to manage new clients
 * @called_from scan to manage existing windows
 * @calls XGetTransientForHint https://tronche.com/gui/x/xlib/ICC/client-to-window-manager/XGetTransientForHint.html
 * @calls XConfigureWindow https://tronche.com/gui/x/xlib/window/XConfigureWindow.html
 * @calls XSetWindowBorder https://tronche.com/gui/x/xlib/window/XSetWindowBorder.html
 * @calls XSelectInput https://tronche.com/gui/x/xlib/event-handling/XSelectInput.html
 * @calls XRaiseWindow https://tronche.com/gui/x/xlib/window/XRaiseWindow.html
 * @calls ecalloc to allocate space for the new client (see util.c)
 * @calls updatetitle to read and store the client's window title
 * @calls wintoclient to find the parent client for a transient window
 * @calls applyrules to search for and to apply client rules that matches the client window
 * @calls configure to propagate the border width
 * @calls updatewindowtype to apply window type hardcoded rules
 * @calls updatesizehints to read a client's size hints
 * @calls updatewmhints to read and proces a client's window management hints
 * @calls grabbuttons to subscribe to button press events on the window
 * @calls attach to add the client to the client list
 * @calls attachstack to add the client to the stacking order list
 * @calls XChangeProperty https://tronche.com/gui/x/xlib/window-information/XChangeProperty.html
 * @calls XMoveResizeWindow https://tronche.com/gui/x/xlib/window/XMoveResizeWindow.html
 * @calls XMapWindow https://tronche.com/gui/x/xlib/window/XMapWindow.html
 * @calls setclientstate to set the window state to normal
 * @calls unfocus to unfocus the previously focused client on the current monitor
 * @calls arrange to resize and reposition clients
 * @calls focus to give input focus to the new client
 *
 * Internal call stack:
 *    run -> maprequest -> manage
 *    run -> scan -> manage
 */
void
manage(Window w, XWindowAttributes *wa)
{
	Client *c, *t = NULL;
	Window trans = None;
	XWindowChanges wc;

	/* Allocate memory for the new client. */
	c = ecalloc(1, sizeof(Client));
	/* Keep a reference to the window this client represents. This is used in many places. */
	c->win = w;
	/* Here we initially use the original position and size of the window as defined by the
	 * window attributes. Setting the old variables here are mostly just to have them
	 * initialised. */
	c->x = c->oldx = wa->x;
	c->y = c->oldy = wa->y;
	c->w = c->oldw = wa->width;
	c->h = c->oldh = wa->height;
	/* Store the previous border width. Intended to be used to restore the border width when
	 * unmanaging a client that is not destroyed. */
	c->oldbw = wa->border_width;

	/* Reads and stores the window title in the client's name variable. */
	updatetitle(c);

	/* A transient window is intended to be a short lived window that belong to a parent window.
	 * This could be a dialog box, a popup, a toolbox or a menu to give a few examples.
	 *
	 * In dwm transient windows are handled differently to other windows in that:
	 *    - they inherit the monitor and tags from their parent window and
	 *    - client rules do not apply to transient windows and
	 *    - transient windows are always floating
	 *
	 * Check if the window is a transient for a parent window, and if so check if this parent
	 * window (t) is managed by the window manager.
	 */
	if (XGetTransientForHint(dpy, w, &trans) && (t = wintoclient(trans))) {
		/* A transient window inherits the monitor and tags from its parent window. */
		c->mon = t->mon;
		c->tags = t->tags;
	} else {
		/* Normal windows are opened on the selected monitor by default, but can be moved to
		 * designated monitors via client rules. */
		c->mon = selmon;
		/* Search for matching client rules and apply those to the given client. */
		applyrules(c);
	}

	/* The intention of the below checks is to move the window into view at the closest border of
	 * the window area if the window were otherwise to spawn out of view if floating. The window
	 * area, as opposed to the monitor area, is used here to avoid floating windows overlapping
	 * the bar. */
	if (c->x + WIDTH(c) > c->mon->wx + c->mon->ww)
		c->x = c->mon->wx + c->mon->ww - WIDTH(c);
	if (c->y + HEIGHT(c) > c->mon->wy + c->mon->wh)
		c->y = c->mon->wy + c->mon->wh - HEIGHT(c);
	c->x = MAX(c->x, c->mon->wx);
	c->y = MAX(c->y, c->mon->wy);

	/* Set the border width as per config. */
	c->bw = borderpx;

	/* We are going to propagate the border width back to the window so we set the client's
	 * new border width in the window changes structure. */
	wc.border_width = c->bw;
	/* Now we tell the X server what we want the border width to be. */
	XConfigureWindow(dpy, w, CWBorderWidth, &wc);
	/* And we set the border colour */
	XSetWindowBorder(dpy, w, scheme[SchemeNorm][ColBorder].pixel);
	/* This sends an event to the window owner informing them about the change(s) made. */
	configure(c); /* propagates border_width, if size doesn't change */
	/* This checks if the window is fullscreen and if so then it makes it fullscreen. If the
	 * window type is dialog then the client is set to be floating. */
	updatewindowtype(c);
	/* This reads the size hints for the client, used when resizing windows. */
	updatesizehints(c);
	/* This reads window management hints for the client window. In practice it just checks
	 * whether the window is urgent or not and whether the window expects input focus or not. */
	updatewmhints(c);
	/* This tells the X server what events we are interested in receiving for this window. */
	XSelectInput(dpy, w, EnterWindowMask|FocusChangeMask|PropertyChangeMask|StructureNotifyMask);
	/* The grabbuttons tells the X server what button press events we are interested in
	 * receiving notifications for in relation to this window. */
	grabbuttons(c, 0);
	/* Transient and fixed windows are forced to be floating. */
	if (!c->isfloating)
		c->isfloating = c->oldstate = trans != None || c->isfixed;
	/* Floating windows are raised to be shown above others. */
	if (c->isfloating)
		XRaiseWindow(dpy, c->win);
	/* Add the client to the client list. New clients are always added at the top of the list
	 * making them the new master client. */
	attach(c);
	/* Add the client to the stacking order list. New additions are always added at the top of
	 * the list to indicate order in which clients had focus. */
	attachstack(c);
	/* We update the _NET_CLIENT_LIST property of the root window appending this new window. */
	XChangeProperty(dpy, root, netatom[NetClientList], XA_WINDOW, 32, PropModeAppend,
		(unsigned char *) &(c->win), 1);
	/* Basically this moves the window two times the screen width to the right. Why some windows
	 * might require this, and which windows for that matter, is not clear. */
	XMoveResizeWindow(dpy, c->win, c->x + 2 * sw, c->y, c->w, c->h); /* some windows require this */
	/* Set the client state to normal state (it may have been in iconic or withdrawn state). */
	setclientstate(c, NormalState);
	/* If the client was added on the current monitor then chances are that it is also shown.
	 * As such we unfocus the selected client. */
	if (c->mon == selmon)
		unfocus(selmon->sel, 0);
	/* The new client is assumed to be the selected client on the client's monitor. */
	c->mon->sel = c;
	/* An arrange to resize and reposition clients in the event that this new client is shown. */
	arrange(c->mon);
	/* The window is ready to be made visible to the user. We fulfill the map request for the
	 * window by mapping it. */
	XMapWindow(dpy, c->win);
	/* Finally a focus to give input focus to the next client in line (will be the new client,
	 * if shown, otherwise it will likely be the previously selected client). */
	focus(NULL);
}

/* This handles MappingNotify events coming from the X server.
 *
 * This might happen if the user inserts a new keyboard or changes the keyboard layout.
 *
 * @called_from run (the event handler)
 * @calls grabkeys to inform the X server what key combinations the window manager is interested in
 * @calls XRefreshKeyboardMapping https://tronche.com/gui/x/xlib/utilities/keyboard/XRefreshKeyboardMapping.html
 * @see https://tronche.com/gui/x/xlib/events/window-state-change/mapping.html
 *
 * Internal call stack:
 *    run -> mappingnotify -> grabkeys
 */
void
mappingnotify(XEvent *e)
{
	XMappingEvent *ev = &e->xmapping;

	/* Refreshes the stored modifier and keymap information. */
	XRefreshKeyboardMapping(ev);
	/* If the event was in relation to new keyboard mapping then we make a call to grabkeys.
	 * This to inform the X server what keypress events we are interested in receiving. */
	if (ev->request == MappingKeyboard)
		grabkeys();
}

/* This handles MapRequest events coming from the X server.
 *
 * When an application creates a new window it starts out as being unmapped and the application
 * should inform the X server when the window is ready to be displayed. It does so by sending a
 * MapRequest event for the window to the X server, which in turn forwards that event to the window
 * manager running the show. The window manager then decides whether it is going to manage that
 * window or not, and if so how. If it decides to manage the window then it will decide if and when
 * the window is mapped. If the window manager does not manage the window then the window will be
 * mapped anyway (presumably by the X server).
 *
 * @called_from run (the event handler)
 * @called_from movemouse as it forwards events to maprequest
 * @called_from resizemouse as it forwards events to maprequest
 * @calls XGetWindowAttributes https://tronche.com/gui/x/xlib/window-information/XGetWindowAttributes.html
 * @calls wintoclient to check whether the window manager is already managing this window
 * @calls manage to make the window manager manage the window and create the client
 *
 * Internal call stack:
 *    run -> maprequest -> manage
 *    run -> buttonpress -> movemouse / resizemouse -> maprequest
 */
void
maprequest(XEvent *e)
{
	static XWindowAttributes wa;
	XMapRequestEvent *ev = &e->xmaprequest;

	/* This if statement combines two if statements:
	 *
	 *    if (!XGetWindowAttributes(dpy, ev->window, &wa))
	 *       return;
	 *
	 * This fetches the window attributes from the X server which contains the a lot of
	 * information besides the width and height of the window. If for some reason we were unable
	 * to retrieve the window attributes for the given window then we bail here. Refer to the
	 * documentation for XGetWindowAttributes for more information.
	 *
	 *    if (wa.override_redirect)
	 *       return;
	 *
	 * From the documentation we have that:
	 *   The override_redirect member is set to indicate whether this window overrides structure
	 *   control facilities and can be True or False. Window manager clients should ignore the
	 *   window if this member is True.
	 *
	 * What that boils down to is that if a window has that override-redirect flag set then it
	 * means that the window manages itself and does not want the window manager to exert any
	 * control over the window. A good example of this is dmenu which controls the size and
	 * position on its own and does not want the window manager to intervene.
	 */
	if (!XGetWindowAttributes(dpy, ev->window, &wa) || wa.override_redirect)
		return;

	/* The wintoclient function returns the client that relates to the given window. If one is
	 * found then we do not proceed. This is essentially a safeguard to prevent erroneous or
	 * duplicate map request events from causing issues. If we do not find that the window is
	 * already managed by the window manager then we proceed by handing the window over to the
	 * manage function which handles the rest of the setup.
	 */
	if (!wintoclient(ev->window))
		manage(ev->window, &wa);
}

/* This is what handles the monocle layout arrangement.
 *
 * @called_from arrangemon
 * @calls snprintf to update the layout symbol of the monitor
 * @calls nexttiled to get the next tiled client
 * @calls resize to change the size and position of client windows
 *
 * Internal call stack:
 *    ~ -> arrange -> arrangemon -> monocle
 */
void
monocle(Monitor *m)
{
	unsigned int n = 0; /* number of clients */
	Client *c;

	/* This for loop is just to get a count of all visible clients on the selected tag(s).
	 * Note that this counts both tiled and floating clients. This number is then used to
	 * update the layout symbol in the bar to say e.g. [3].
	 */
	for (c = m->clients; c; c = c->next)
		if (ISVISIBLE(c))
			n++;
	/* The layout symbol of the monitor is only overwritten if there are clients visible
	 * on the selected tag(s). Look up snprintf you are unsure what this does, but the gist
	 * of it is that it replaces the %d format inside the string "[%d]" with the value of n
	 * (e.g. 3) and writes the output to the monitor layout symbol (m->ltsymbol) and it writes
	 * at most 16 bytes (sizeof m->ltsymbol) to that variable.
	 */
	if (n > 0) /* override layout symbol */
		snprintf(m->ltsymbol, sizeof m->ltsymbol, "[%d]", n);
	/* This just loops through all tiled clients and resizes them to take up the entire window
	 * area. Note that this does not have anything to do with which window is shown on top, that
	 * is determined by the window that has focus which will be above other tiled windows in the
	 * stack.
	 */
	for (c = nexttiled(m->clients); c; c = nexttiled(c->next))
		resize(c, m->wx, m->wy, m->ww - 2 * c->bw, m->wh - 2 * c->bw, 0);
}


/* This handles MotionNotify events coming from the X server.
 *
 * These events happen whenever the mouse cursor moves over the root window or other windows that
 * subscribe to motion notifications by setting the PointerMotionMask event mask. An example would
 * be to set that event mask for the bar should you want to have things happen when you hover the
 * mouse cursor over the bar. That would generate events which would then be handled in this
 * function.
 *
 * @called_from run (the event handler)
 * @calls recttomon to check which monitor the cursor is on
 * @calls unfocus in the event of the selected monitor changing
 * @calls focus in the event of the selected monitor changing
 *
 * Internal call stack:
 *    run -> motionnotify
 */
void
motionnotify(XEvent *e)
{
	static Monitor *mon = NULL;
	Monitor *m;
	XMotionEvent *ev = &e->xmotion;

	/* Ignore the motion event if it was not for the root window */
	if (ev->window != root)
		return;
	/* This makes a call to recttomon (rectangle to monitor) with the coordinates of the mouse
	 * pointer to work out which monitor the mouse cursor is on. We only enter the if statement
	 * if the monitor has changed. Note that the mon variable is static, which means that it is
	 * only declared and initialised to NULL once and it will retain the set value for all
	 * subsequent calls to this function.
	 */
	if ((m = recttomon(ev->x_root, ev->y_root, 1, 1)) != mon && mon) {
		/* In the event that the monitor has changed then we unfocus the selected client
		 * on the current monitor before switching over. */
		unfocus(selmon->sel, 1);
		/* We set the selected monitor to be the one the mouse cursor is currently on. */
		selmon = m;
		/* Passing NULL to the focus function will result in the client at the top of the
		 * stack to receive focus, i.e. the window that last had focus on this monitor. */
		focus(NULL);
	}
	/* Set the static mon variable to be the monitor the mouse cursor is on. */
	mon = m;
}

/* User function to move a (floating) window around using the mouse.
 *
 * @called_from buttonpress in relation to button bindings
 * @calls XGrabPointer https://tronche.com/gui/x/xlib/input/XGrabPointer.html
 * @calls XUngrabPointer https://tronche.com/gui/x/xlib/input/XUngrabPointer.html
 * @calls XMaskEvent https://tronche.com/gui/x/xlib/event-handling/manipulating-event-queue/XMaskEvent.html
 * @calls restack to place the selected client above other floating windows if floating
 * @calls getrootptr to find the mouse pointer coordinates
 * @calls togglefloating to make a tiled window snap out to become floating
 * @calls resize to move the window to the new position
 * @calls recttomon to work out what monitor the client is on after having been moved
 * @calls sendmon to send the client to the other monitor if the client's monitor has changed
 * @calls focus to give input focus after having moved the client to another monitor
 * @see https://tronche.com/gui/x/xlib/events/processing-overview.html
 *
 * Internal call stack:
 *    run -> buttonpress -> movemouse
 */
void
movemouse(const Arg *arg)
{
	int x, y, ocx, ocy, nx, ny;
	Client *c;
	Monitor *m;
	XEvent ev;
	Time lasttime = 0;

	/* If there is no selected client then there is nothing to do here. This is merely a
	 * safeguard in the event the function is called / used incorrectly. Under normal
	 * circumstances this should never happen. Note that this statement also sets the variable
	 * c to be the selected client. */
	if (!(c = selmon->sel))
		return;

	/* If the client is in fullscreen then we bail as we do not want the mouse move or resize
	 * functionality interfering with fullscreen windows. */
	if (c->isfullscreen) /* no support moving fullscreen windows by mouse */
		return;

	/* The only legitimate reason for a call to restack here is to place the selected client
	 * above other floating windows before we start moving it around. */
	restack(selmon);

	/* We store the original client x and y position. This is used below to calculate the new
	 * client coordinates based on the position of the mouse cursor and how far the mouse has
	 * been moved. */
	ocx = c->x;
	ocy = c->y;

	/* Here we grab the mouse pointer to tell the X server that we are interested in receiving
	 * events related to the mouse, in particular MotionNotify events. We also change the cursor
	 * to indicate that we are performing a window move operation. If we were not able to grab
	 * the pointer for whatever reason then we bail. */
	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursor[CurMove]->cursor, CurrentTime) != GrabSuccess)
		return;

	/* Here we ask for the mouse pointer coordinates and store these in the integer variables of
	 * x and y. */
	if (!getrootptr(&x, &y))
		return;

	/* Keep doing this until the button is released. */
	do {
		/* Consider that we have received a ButtonPress event, which results in the run
		 * function calling the buttonpress function that handles the event, which in turn
		 * calls movemouse and we get down to this code. This will block dwm from processing
		 * any events for as long as the user moves the window around. An example of this is
		 * that the bar will stop updating while the user has this function active.
		 *
		 * Because of this both the movemouse and resizemouse functions hook into the event
		 * queue to look for MotionNotify events, but they also subscribe to a few other
		 * types of events so as to not block everything while the window is moved around.
		 *
		 * The below code handles MotionNotify events, but forwards ConfigureRequest, Expose
		 * and MapRequest events to their respective event handlers.
		 *
		 * The XMaskEvent call asks for the next event whose type falls under the given event
		 * masks.
		 */
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);

		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			/* Events for the above event types are forwarded to their respective event
			 * handler function. */
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			/* Motion notify events can come in quick, very quick in fact, so we skip
			 * events received between certain intervals.
			 *
			 * The (1000 / 60) means that we want to process 60 events per second. This
			 * corresponds to processing one event per 16.66 milliseconds.
			 *
			 * The value is a good middle-ground in terms of performance on older systems
			 * and responsiveness. Some people may find that the interaction is not
			 * entirely smooth and they can try increasing this to process 120 events per
			 * second (1000 / 120) which means one event every 8.33 milliseconds.
			 */
			if ((ev.xmotion.time - lasttime) <= (1000 / 60))
				continue;
			lasttime = ev.xmotion.time; /* Store the previous time for comparison above */

			/* Here we calculate the new x and y coordinates which are the original
			 * coordinates plus the relative distance that the mouse cursor has moved. */
			nx = ocx + (ev.xmotion.x - x);
			ny = ocy + (ev.xmotion.y - y);

			/* The snap pixel, which is 32 as per default configuration, controls how far
			 * the window must be from the window area border until it snaps against that
			 * border.
			 *
			 * This checks whether the new position is close to the left border, and if
			 * so then we snap to that border. */
			if (abs(selmon->wx - nx) < snap)
				nx = selmon->wx;
			/* This checks whether the new position is close to the right hand border of
			 * the window area, and if so then we snap to that border. */
			else if (abs((selmon->wx + selmon->ww) - (nx + WIDTH(c))) < snap)
				nx = selmon->wx + selmon->ww - WIDTH(c);
			/* This checks whether the new position is close to the top border of the
			 * window area, and if so then we snap to that border. */
			if (abs(selmon->wy - ny) < snap)
				ny = selmon->wy;
			/* This checks whether the new position is close to the bottom border of the
			 * window area, and if so then we snap to that border. */
			else if (abs((selmon->wy + selmon->wh) - (ny + HEIGHT(c))) < snap)
				ny = selmon->wy + selmon->wh - HEIGHT(c);

			/* If the window is tiled (and we are not using floating layout), and we have
			 * moved the cursor more than snap (32) pixels, then we "snap" the client out
			 * of tiled state and make it floating (by calling togglefloating). */
			if (!c->isfloating && selmon->lt[selmon->sellt]->arrange
			&& (abs(nx - c->x) > snap || abs(ny - c->y) > snap))
				togglefloating(NULL);

			/* We only actually move the window if we are in floating layout or the window
			 * is actually floating. This has to do with that we may be dealing with a
			 * tiled window that has not yet snapped out to become floating (as per the
			 * above code). */
			if (!selmon->lt[selmon->sellt]->arrange || c->isfloating)
				resize(c, nx, ny, c->w, c->h, 1);
			break;
		}
	} while (ev.type != ButtonRelease);

	/* We no longer need to be spammed about mouse movement so we ungrab the mouse pointer.
	 * Other programs may need it. */
	XUngrabPointer(dpy, CurrentTime);

	/* The call to recttomon checks if the client position after having moved it places it on
	 * another monitor, and if so then we call sendmon to make sure that the client is handed
	 * over to that monitor for arrangement purposes. */
	if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
		sendmon(c, m);
		selmon = m;
		/* A general focus to give input focus after changing monitor. */
		focus(NULL);
	}
}

/* This returns the next tiled client on the currently selected tag(s).
 *
 * Given an input client c the function returns the next visible tiled client in the list, or NULL
 * if there are no more subsequent tiled clients.
 *
 * @called_from tile for tiling purposes
 * @called_from monocle for tiling purposes
 * @called_from zoom to check if the selected client is the master client
 *
 * Internal call stack:
 *    ~ -> arrange -> arrangemon -> tile / monocle -> nexttiled
 *    run -> keypress -> zoom -> nexttiled
 */
Client *
nexttiled(Client *c)
{
	/* Loop through all clients following the given client c and ignore clients on other tags
	 * and floating clients. */
	for (; c && (c->isfloating || !ISVISIBLE(c)); c = c->next);
	/* Return the client found, if any, will be NULL if the linked list is exhausted. */
	return c;
}

/* This function moves a client to the top of the tile stack, making it the new master window.
 *
 * @called_from zoom to move the selected client to become the new master
 * @calls detach to remove the client from where it currently is in the tile stack
 * @calls attach to add the client at the top of the tile stack
 * @calls focus to give the client becoming the new master input focus
 * @calls arrange to resize and reposition all tiled windows
 *
 * Internal call stack:
 *    run -> keypress -> zoom -> pop
 */
void
pop(Client *c)
{
	/* The detach and attach calls take the given client and moves it out the tile stack (list
	 * of clients) and adds it again at the start of the list making it the new master window. */
	detach(c);
	attach(c);
	focus(c); /* Make sure the given window has input focus */
	arrange(c->mon); /* Rearrange all tiled windows as the order has changed */
}

void
propertynotify(XEvent *e)
{
	Client *c;
	Window trans;
	XPropertyEvent *ev = &e->xproperty;

	if ((ev->window == root) && (ev->atom == XA_WM_NAME))
		updatestatus();
	else if (ev->state == PropertyDelete)
		return; /* ignore */
	else if ((c = wintoclient(ev->window))) {
		switch(ev->atom) {
		default: break;
		case XA_WM_TRANSIENT_FOR:
			if (!c->isfloating && (XGetTransientForHint(dpy, c->win, &trans)) &&
				(c->isfloating = (wintoclient(trans)) != NULL))
				arrange(c->mon);
			break;
		case XA_WM_NORMAL_HINTS:
			c->hintsvalid = 0;
			break;
		case XA_WM_HINTS:
			updatewmhints(c);
			drawbars();
			break;
		}
		if (ev->atom == XA_WM_NAME || ev->atom == netatom[NetWMName]) {
			updatetitle(c);
			if (c == c->mon->sel)
				drawbar(c->mon);
		}
		if (ev->atom == netatom[NetWMWindowType])
			updatewindowtype(c);
	}
}

/* User function to quit / exit the window manager.
 *
 * @called_from keypress in relation to keybindings
 *
 * Internal call stack:
 *    run -> keypress -> quit
 */
void
quit(const Arg *arg)
{
	/* All this does is set the running global variable to 0, which makes the event handler
	 * in the run function exit the while loop. This makes the run function return allowing
	 * main to continue to perform cleanup and exiting the process.
	 */
	running = 0;
}

/* The "rectangle to monitor" function takes x and y coordinates as well as height and width and
 * it works out which monitor this rectangle is on (or overlaps it the most if the rectangle spans
 * more than one window).
 *
 * This is also used when working out which monitor the mouse pointer resides on, in which case
 * the x and y coordinates are those of the mouse pointer and the heigh and width are simply 1 to
 * refer to that particular point.
 *
 * @called_from motionnotify to check which monitor the mouse cursor is on
 * @called_from movemouse to check if a moved client window has moved over to another monitor
 * @called_from resizemouse to check if a moved client window has moved over to another monitor
 * @called_from wintomon to check which monitor the mouse cursor is on
 *
 * Internal call stack:
 *    run -> motionnotify -> recttomon
 *    run -> buttonpress -> movemouse / resizemouse -> recttomon
 *    run -> buttonpress / enternotify / expose -> wintomon -> recttomon
 *    run -> configurenotify -> updategeom -> wintomon -> rectomon
 *    main -> setup -> updategeom -> wintomon -> rectomon
 */
Monitor *
recttomon(int x, int y, int w, int h)
{
	Monitor *m, *r = selmon;
	int a, area = 0;

	/* INTERSECT is a macro that calculates how much of the area of the given rectangle is
	 * covered by the given monitor.
	 *
	 * The macro is only used in this function so it makes sense to list it here for reference.
	 *
	 * #define INTERSECT(x,y,w,h,m)  (MAX(0, MIN((x)+(w),(m)->wx+(m)->ww) - MAX((x),(m)->wx)) \
	 *                              * MAX(0, MIN((y)+(h),(m)->wy+(m)->wh) - MAX((y),(m)->wy)))
	 *
	 * The MAX(0, ...) are there to prevent negative values from giving false positives. For
	 * readability reasons we can remove these and also get rid of some parentheses.
	 *
	 *      (MIN(x + w, wx + ww) - MAX(x, wx))
	 *    * (MIN(y + h, wy + wh) - MAX(y, wy))
	 *
	 *  x,  y,  w,  h - think of these as a window's coordinates + width and height
	 * wx, wy, ww, wh - think of these as the monitor's window area position and size
	 *
	 * The first line handles the horizontal dimension while the second handles the vertical
	 * dimension.
	 *    MIN(x + w, wx + ww) - this caps the right hand border covered by the monitor
	 *    MAX(x, wx)) - this caps the left hand border covered by the monitor
	 *
	 * Subtracting the left hand border from the right hand border gives the width that the
	 * monitor covers of the window.
	 *
	 *    MIN(y + h, wy + wh) - this caps the top border covered by the monitor
	 *    MAX(y, wy) - this caps the bottom border covered by the monitor
	 *
	 * Subtracting the bottom border from the top border gives the height that the monitor
	 * covers of the window.
	 *
	 * Finally multiplying the covered height and covered width gives the area of the
	 * intersection between the monitor window area and the given window.
	 */
	for (m = mons; m; m = m->next)
		/* Here we loop through each monitor and works out which of the monitors has the
		 * largest intersection of the given rectangle. The leading monitor being stored
		 * in the variable r and the current maximum is stored in the variable named area.
		 */
		if ((a = INTERSECT(x, y, w, h, m)) > area) {
			area = a;
			r = m;
		}
	return r;
}

/* The resize function resizes a client window to the dimensions given, but only if the new
 * position and dimensions would lead to a change in size or position after size hints have
 * been taken into account.
 *
 * The interact flag indicates whether the resize is due to the user manually interacting with the
 * window or if it is due to automated processes like tiling arrangements. The difference has to
 * do with the boundaries that are imposed; if the user is manually resizing or moving the window
 * then they can freely move that window in the available screen space, but if the resize is not
 * interactive then the placement is restricted to be, at least partially, within the monitor's
 * window area. Refer to the applysizehints function for more details.
 *
 * @called_from monocle for tiling purposes
 * @called_from movemouse to change position of the client window
 * @called_from resizemouse to change the size of the client window
 * @called_from showhide to move the window back into view when shown
 * @called_from tile for tiling purposes
 * @called_from togglefloating to resize the window taking size hints into account
 * @calls applysizehints to check if the window needs resizing taking size hints into account
 * @calls resizeclient to resize the client
 *
 * Internal call stack:
 *    ~ -> arrange -> showhide -> resize
 *    ~ -> arrange -> arrangemon -> monocle / tile -> resize
 *    run -> buttonpress -> movemouse / resizemouse / togglefloating -> resize
 *    run -> keypress -> togglefloating -> resize
 */
void
resize(Client *c, int x, int y, int w, int h, int interact)
{
	/* Note how the above variables are passed by reference to applysizehints. This is because
	 * that function manipulates the variables before we pass them on to resizeclient (provided
	 * that a resize is deemed necessary). If these were not passed by reference then they would
	 * be passed by value which means that applysizehints would receive copies of said values
	 * leaving the variables here unaltered. */
	if (applysizehints(c, &x, &y, &w, &h, interact))
		resizeclient(c, x, y, w, h);
}

/* This handles the actual resize of the a client window.
 *
 * @called_from resize to change the size of the client window after size hints have been checked
 * @called_from setfullscreen when moving a window into and out of fullscreen
 * @called_from configurenotify to restore fullscreen after a monitor change
 * @calls XConfigureWindow https://tronche.com/gui/x/xlib/window/XConfigureWindow.html
 * @calls XSync https://tronche.com/gui/x/xlib/event-handling/XSync.html
 * @calls configure to send an event to the window indicating that the size has changed
 *
 * Internal call stack:
 *    ~ -> resize -> resizeclient
 *    run -> clientmessage / updatewindowtype -> setfullscreen -> resizeclient
 *    run -> configurenotify -> resizeclient
 */
void
resizeclient(Client *c, int x, int y, int w, int h)
{
	XWindowChanges wc;

	/* There are three things happening here:
	 *    - the existing x, y, w and h are stored in oldx, oldy, oldw and oldh respectively and
	 *    - the new x, y, w and h are stored in the client for future reference and
	 *    - the new x, y, w and h are stored in the XWindowChanges structure
	 *
	 * The oldx, oldy, etc. values are only ever used in the setfullscreen function, so setting
	 * those here is only in relation to the resizeclient call being made in that function.
	 */
	c->oldx = c->x; c->x = wc.x = x;
	c->oldy = c->y; c->y = wc.y = y;
	c->oldw = c->w; c->w = wc.width = w;
	c->oldh = c->h; c->h = wc.height = h;
	wc.border_width = c->bw;
	/* This calls reconfigures the window's size, position and border according to the
	 * XWindowChanges structure that have been populated with data above. */
	XConfigureWindow(dpy, c->win, CWX|CWY|CWWidth|CWHeight|CWBorderWidth, &wc);
	/* In principle the above call should be enough to change the size and position of the
	 * window, but not all applications behave the same way and some need to be told that their
	 * window has changed - so we send a XConfigureEvent to notify the window owner that the
	 * dimensions or position has changed. This is handled in the configure function. */
	configure(c);
	/* This flushes the output buffer and then waits until all requests have been received and
	 * processed by the X server. */
	XSync(dpy, False);
}

/* User function to resize a (floating) window using the mouse.
 *
 * @called_from buttonpress in relation to button bindings
 * @calls XGrabPointer https://tronche.com/gui/x/xlib/input/XGrabPointer.html
 * @calls XUngrabPointer https://tronche.com/gui/x/xlib/input/XUngrabPointer.html
 * @calls XWarpPointer https://tronche.com/gui/x/xlib/input/XWarpPointer.html
 * @calls XMaskEvent https://tronche.com/gui/x/xlib/event-handling/manipulating-event-queue/XMaskEvent.html
 * @calls XCheckMaskEvent https://tronche.com/gui/x/xlib/event-handling/manipulating-event-queue/XCheckMaskEvent.html
 * @calls restack to place the selected client above other floating windows if floating
 * @calls togglefloating to make a tiled window snap out to become floating
 * @calls resize to change the size of the window while respecting size hints
 * @calls recttomon to work out what monitor the client is on after having been resized
 * @calls sendmon to send the client to the other monitor if the client's monitor has changed
 * @calls focus to give input focus after having moved the client to another monitor
 * @see https://tronche.com/gui/x/xlib/events/processing-overview.html
 *
 * Internal call stack:
 *    run -> buttonpress -> resizemouse
 */
void
resizemouse(const Arg *arg)
{
	int ocx, ocy, nw, nh;
	Client *c;
	Monitor *m;
	XEvent ev;
	Time lasttime = 0;

	/* If there is no selected client then there is nothing to do here. This is merely a
	 * safeguard in the event the function is called / used incorrectly. Under normal
	 * circumstances this should never happen. Note that this statement also sets the variable
	 * c to be the selected client. */
	if (!(c = selmon->sel))
		return;

	/* If the client is in fullscreen then we bail as we do not want the mouse move or resize
	 * functionality interfering with fullscreen windows. */
	if (c->isfullscreen) /* no support resizing fullscreen windows by mouse */
		return;

	/* The only legitimate reason for a call to restack here is to place the selected client
	 * above other floating windows before we start resizing it. */
	restack(selmon);

	/* We store the original client x and y position. This is used below to restrict the size
	 * of the window preventing it from becoming negative. */
	ocx = c->x;
	ocy = c->y;

	/* Here we grab the mouse pointer to tell the X server that we are interested in receiving
	 * events related to the mouse, in particular MotionNotify events. We also change the cursor
	 * to indicate that we are performing a window resize operation. If we were not able to grab
	 * the pointer for whatever reason then we bail. */
	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursor[CurResize]->cursor, CurrentTime) != GrabSuccess)
		return;

	/* Here we move the mouse cursor to the bottom right corner of the window. This is known to
	 * cause issues on some setups where the transformation matrix for the mouse has been
	 * changed using xinput. */
	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1, c->h + c->bw - 1);

	/* Keep doing this until the button is released. */
	do {
		/* Consider that we have received a ButtonPress event, which results in the run
		 * function calling the buttonpress function that handles the event, which in turn
		 * calls resizemouse and we get down to this code. This will block dwm from
		 * processing any events for as long as the user resizes the window. An example of
		 * this is that the bar will stop updating while the user has this function active.
		 *
		 * Because of this both the movemouse and resizemouse functions hook into the event
		 * queue to look for MotionNotify events, but they also subscribe to a few other
		 * types of events so as to not block everything while the window being resized.
		 *
		 * The below code handles MotionNotify events, but forwards ConfigureRequest, Expose
		 * and MapRequest events to their respective event handlers.
		 *
		 * The XMaskEvent call asks for the next event whose type falls under the given event
		 * masks.
		 */
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);

		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			/* Events for the above event types are forwarded to their respective event
			 * handler function. */
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			/* Motion notify events can come in quick, very quick in fact, so we skip
			 * events received between certain intervals.
			 *
			 * The (1000 / 60) means that we want to process 60 events per second. This
			 * corresponds to processing one event per 16.66 milliseconds.
			 *
			 * The value is a good middle-ground in terms of performance on older systems
			 * and responsiveness. Some people may find that the interaction is not
			 * entirely smooth and they can try increasing this to process 120 events per
			 * second (1000 / 120) which means one event every 8.33 milliseconds.
			 */
			if ((ev.xmotion.time - lasttime) <= (1000 / 60))
				continue;
			lasttime = ev.xmotion.time; /* Store the previous time for comparison above */

			/* This calculates the new width based on the coordinates of the window and
			 * the distance that the mouse cursor has moved. The MAX is a guard to prevent
			 * the size to become negative should the mouse cursor be moved past the
			 * position of the client. */
			nw = MAX(ev.xmotion.x - ocx - 2 * c->bw + 1, 1);
			nh = MAX(ev.xmotion.y - ocy - 2 * c->bw + 1, 1);

			/* This if statement guards against unreasonable sizes. In practice it
			 * validates that the new size is within the position and size of the selected
			 * monitor in order to allow a tiled client to snap out and become floating. */
			if (c->mon->wx + nw >= selmon->wx && c->mon->wx + nw <= selmon->wx + selmon->ww
			&& c->mon->wy + nh >= selmon->wy && c->mon->wy + nh <= selmon->wy + selmon->wh)
			{
				/* If the client is tiled and we are not using the floating layout,
				 * then check if we have changed the size more than snap (32) pixels
				 * before we make the client snap out of tiled state and become
				 * floating. We let togglefoating handle the transition. */
				if (!c->isfloating && selmon->lt[selmon->sellt]->arrange
				&& (abs(nw - c->w) > snap || abs(nh - c->h) > snap))
					togglefloating(NULL);
			}

			/* We only actually resize the window if we are in floating layout or the
			 * window is actually floating. This has to do with that we may be dealing
			 * with a tiled window that has not yet snapped out to become floating (as
			 * per the above code). */
			if (!selmon->lt[selmon->sellt]->arrange || c->isfloating)
				resize(c, c->x, c->y, nw, nh, 1);
			break;
		}
	} while (ev.type != ButtonRelease);

	/* We warp the cursor again to be at the bottom left of the window. In principle it should
	 * already be there, but depending on size hints it may not be. This is just correcting in
	 * case it is not. */
	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1, c->h + c->bw - 1);

	/* We no longer need to be spammed about mouse movement so we ungrab the mouse pointer.
	 * Other programs may need it. */
	XUngrabPointer(dpy, CurrentTime);

	/* This seemingly benign line of code is actually very important. What this does is that
	 * it checks the X event queue if there are any EnterNotify events waiting as a result
	 * of the resize action above and simply swallows (ignores) them. This avoids situations
	 * where two overlapping windows begin to flicker back and forth due to competing and
	 * continuously generated EnterNotify events. */
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));

	/* The call to recttomon checks if the client position and size is more on another monitor
	 * after we have resized it, and if so then we call sendmon to make sure that the client is
	 * handed over to that monitor for arrangement purposes. */
	if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
		sendmon(c, m);
		selmon = m;
		/* A general focus to give input focus after changing monitor. */
		focus(NULL);
	}
}

/* The restack function's primary responsibility is to:
 *    - make the selected client above others if it is floating and
 *    - to place all tiled clients below the bar
 *
 * Additionally the restack function incorporates a call to drawbar out of convenience as the two
 * are often called together.
 *
 * @called_from arrange to restack a monitor as part of a full arrange
 * @called_from buttonpress to make floating windows clicked on become above other windows
 * @called_from focusstack to make floating windows receiving focus to become above other windows
 * @called_from movemouse to make the window being moved above others if floating
 * @called_from resizemouse to make the window being reized above others if floating
 * @calls XRaiseWindow https://tronche.com/gui/x/xlib/window/XRaiseWindow.html
 * @calls XConfigureWindow https://tronche.com/gui/x/xlib/window/XConfigureWindow.html
 * @calls XSync https://tronche.com/gui/x/xlib/event-handling/XSync.html
 * @calls XCheckMaskEvent https://tronche.com/gui/x/xlib/event-handling/manipulating-event-queue/XCheckWindowEvent.html
 * @calls drawbar as restack and drawbar are often two calls that are done together
 *
 * Internal call stack:
 *    ~ -> arrange -> restack
 *    run -> buttonpress -> restack
 *    run -> keypress -> focusstack -> restack
 *    run -> buttonpress -> movemouse / resizemouse -> restack
 */
void
restack(Monitor *m)
{
	Client *c;
	XEvent ev;
	XWindowChanges wc;

	/* The drawbar call here stands out as being misplaced as it has nothing to do with the
	 * objective of the function, neither does the restacking affect anything in the bar.
	 * Most likely it has been added out of convenience because often when restack is called we
	 * also want to call drawbar for other reasons. Including that call in here saves a few
	 * lines of code. */
	drawbar(m);

	/* Bail if there is no selected client on the given monitor. */
	if (!m->sel)
		return;

	/* If the selected client is floating, or if we are using floating layout, then place the
	 * selected window above all other windows. */
	if (m->sel->isfloating || !m->lt[m->sellt]->arrange)
		XRaiseWindow(dpy, m->sel->win);

	/* If we are not using floating layout then we place all tiled clients below the bar
	 * window (in terms of how windows stack on top of each other). */
	if (m->lt[m->sellt]->arrange) {
		wc.stack_mode = Below;
		wc.sibling = m->barwin;
		/* Loop through each client in the stacking order list */
		for (c = m->stack; c; c = c->snext)
			/* If we have a tiled client that is visible, then we place that below the
			 * bar window. */
			if (!c->isfloating && ISVISIBLE(c)) {
				XConfigureWindow(dpy, c->win, CWSibling|CWStackMode, &wc);
				/* Note that the sibling is changed from the bar window to the last
				 * window that was changed. This makes it so that the order of the
				 * windows are preserved, just that all the tiled windows are below
				 * the bar window. For this reason we are looping through the stacking
				 * order list (m->stack) rather than the client list. */
				wc.sibling = c->win;
			}
	}
	/* This flushes the output buffer and then waits until all requests have been
	 * received and processed by the X server. */
	XSync(dpy, False);
	/* This seemingly benign line of code is actually very important. What this does is that
	 * it checks the X event queue if there are any EnterNotify events waiting as a result
	 * of the change in stacking order above and simply swallows (ignores) them. This avoids
	 * situations where two overlapping windows begin to flicker back and forth due to competing
	 * and continuously generated EnterNotify events. */
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
}

/* The run function is what starts the event handler, which is the heart of dwm.
 *
 * The event handler will keep going until:
 *    - a fatal error occurs or
 *    - the user triggers the quit function which sets the running variable to 0
 *
 * As per the header comment of this file we have that:
 *
 *    The event handlers of dwm are organized in an array which is accessed whenever
 *    a new event has been fetched. This allows event dispatching in O(1) time.
 *
 * @called_by main to start the event handler
 * @calls XNextEvent https://tronche.com/gui/x/xlib/event-handling/manipulating-event-queue/XNextEvent.html
 * @calls XSync https://tronche.com/gui/x/xlib/event-handling/XSync.html
 * @calls buttonpress to handle ButtonPress event types
 * @calls clientmessage to handle ClientMessage event types
 * @calls configurerequest to handle ConfigureRequest event types
 * @calls configurenotify to handle ConfigureNotify event types
 * @calls destroynotify to handle DestroyNotify event types
 * @calls enternotify to handle EnterNotify event types
 * @calls expose to handle Expose event types
 * @calls focusin to handle FocusIn event types
 * @calls keypress to handle KeyPress event types
 * @calls mappingnotify to handle MappingNotify event types
 * @calls maprequest to handle MapRequest event types
 * @calls motionnotify to handle MotionNotify event types
 * @calls propertynotify to handle PropertyNotify event types
 * @calls unmapnotify to handle UnmapNotify event types
 *
 * Internal call stack:
 *    main -> run
 */
void
run(void)
{
	XEvent ev;
	/* main event loop */
	XSync(dpy, False);

	/* The XNextEvent function copies the first event from the event queue into the specified
	 * XEvent structure and then removes it from the queue. If the event queue is empty, then
	 * XNextEvent flushes the output buffer and blocks until an event is received. */
	while (running && !XNextEvent(dpy, &ev))
		/* This calls the function corresponding to the specific event type. If we do not
		 * have an event handler for the given event type then the event is ignored. Refer
		 * to the handler array for how the event types and functions are mapped. */
		if (handler[ev.type])
			handler[ev.type](&ev); /* call handler */
}

/* This queries the X server to find windows that can be managed by the window manager.
 *
 * @called_from main to find existing windows that can be managed
 * @calls XQueryTree https://tronche.com/gui/x/xlib/window-information/XQueryTree.html
 * @calls XGetWindowAttributes https://tronche.com/gui/x/xlib/window-information/XGetWindowAttributes.html
 * @calls XGetTransientForHint https://tronche.com/gui/x/xlib/ICC/client-to-window-manager/XGetTransientForHint.html
 * @calls XFree https://tronche.com/gui/x/xlib/display/XFree.html
 * @calls getstate to check if the window state is iconic
 * @calls manage to make the window manager manage this window as a client
 * @see manage for how transient windows are handled
 *
 * Internal call stack:
 *    main -> scan
 */
void
scan(void)
{
	unsigned int i, num;
	/* The d1 and d2 are dummy windows needed for the XQueryTree and XGetTransientForHint calls,
	 * but the values are ignored. */
	Window d1, d2, *wins = NULL;
	XWindowAttributes wa;

	/* This asks the X server for a list of windows under the given root window. */
	if (XQueryTree(dpy, root, &d1, &d2, &wins, &num)) {
		/* A transient window is intended to be a short lived window that belong to a parent
		 * window. This might be a dialog box or a toolbox for example.
		 *
		 * In dwm transient windows are handled differently to other windows in that:
		 *    - they inherit the monitor and tags from their parent window and
		 *    - client rules do not apply to transient windows and
		 *    - transient windows are always floating
		 *
		 * In order for the above to work correctly when loading existing windows into dwm
		 * on startup we need to make sure to manage the parent windows first.
		 *
		 * As such we have two loops here where the first one goes through all normal windows
		 * and a second loop that only handles transient windows.
		 */
		for (i = 0; i < num; i++) {
			/* Skip the window if:
			 *    - it is a transient window or
			 *    - it has the override-redirect flag indicating that it handles position
			 *      and size on its own and do not want a window manager interfering or
			 *    - we fail to read the window attributes for that window (in which case
			 *      it is not the kind of window that the end user would interact with)
			 */
			if (!XGetWindowAttributes(dpy, wins[i], &wa)
			|| wa.override_redirect || XGetTransientForHint(dpy, wins[i], &d1))
				continue;

			/* If the window is in a viewable map state or if the window is in an iconic
			 * state then we manage that window. The iconic state means that the window
			 * is not visible to the user, but that it can still have representation in
			 * a bar. In more common words one would say that the window is minimised.
			 *
			 * The possible map states are:
			 *    - IsUnmapped
			 *    - IsUnviewable and
			 *    - IsViewable
			 */
			if (wa.map_state == IsViewable || getstate(wins[i]) == IconicState)
				manage(wins[i], &wa);
		}

		/* This second for loop goes through and handles all the transient windows. */
		for (i = 0; i < num; i++) { /* now the transients */
			/* As for normal windows we bail if we can not read the window attributes. */
			if (!XGetWindowAttributes(dpy, wins[i], &wa))
				continue;

			/* If the window is transient and in a viewable state, or if it is in iconic
			 * state, then we manage that window.
			 *
			 * Notably a check to see if the window has the override-redirect flag is not
			 * present here. This is likely an oversight, but transient windows for a
			 * self-managing window is probably extremely rare. */
			if (XGetTransientForHint(dpy, wins[i], &d1)
			&& (wa.map_state == IsViewable || getstate(wins[i]) == IconicState))
				manage(wins[i], &wa);
		}
		/* Data returned by XQueryTree must be freed by the caller. */
		if (wins)
			XFree(wins);
	}
}

/* This function handles moving a given client to a designated monitor.
 *
 * @called_from tagmon to send the selected client to a monitor in a given direction
 * @called_from movemouse if the majority of the window is on another monitor after being moved
 * @called_from resizemouse if the majority of the window is on another monitor after being resized
 *
 * Internal call stack:
 *    run -> keypress -> tagmon -> sendmon
 *    run -> buttonpress -> movemouse / resizemouse -> sendmon
 */
void
sendmon(Client *c, Monitor *m)
{
	/* If the client is already on the target monitor then bail. */
	if (c->mon == m)
		return;

	/* Unfocus the client and revert input focus back to the root window before we move the
	 * client across to the new monitor. */
	unfocus(c, 1);
	/* We need to remove the given client from the previous monitor's client list as well as the
	 * stacking order list before we can move the client. */
	detach(c);
	detachstack(c);
	/* Set the client's monitor to be the target monitor. */
	c->mon = m;
	/* The client inherits the tag(s) the target monitor, as in the currently viewed tags on
	 * that monitor. */
	c->tags = m->tagset[m->seltags]; /* assign tags of target monitor */
	/* Add the client to the target monitor's client list. */
	attach(c);
	/* Add the client to the target monitor's stacking order list. */
	attachstack(c);

	/* Apply a general focus to focus on the next client on the current monitor. Note that the
	 * monitor focus does not follow the client being moved. */
	focus(NULL);
	/* Apply a full arrange across all monitors. Logically this is only needed for the target
	 * monitor and the previous monitor, but that would require more lines of code. */
	arrange(NULL);
}

/* This function sets a client window's state.
 *
 * The window can be in one of the following states:
 *
 *    NormalState     - a normal visible window
 *    WithdrawnState  - a window that is not visible to the user in any way
 *    IconicState     - a window that is not visible to the user, but may be represented by an icon
 *                      in a taskbar as an example - more commonly one would refer to such windows
 *                      as being minimised
 *
 * @called_from manage to set the client's state to normal
 * @called_from unmanage to set the client's state to withdrawn
 * @called_from unmapnotify to set the client's state to withdrawn
 * @calls XChangeProperty https://tronche.com/gui/x/xlib/window-information/XChangeProperty.html
 *
 * Internal call stack:
 *    run -> maprequest -> manage -> setclientstate
 *    run -> destroynotify / unmapnotify -> unmanage -> setclientstate
 *    run -> unmapnotify -> setclientstate
 */
void
setclientstate(Client *c, long state)
{
	long data[] = { state, None };

	/* This sets the WM_STATE property of the client window to the given state. */
	XChangeProperty(dpy, c->win, wmatom[WMState], wmatom[WMState], 32,
		PropModeReplace, (unsigned char *)data, 2);
}

/* This function sends a client message event to a client window provided that the client window
 * advertises support for the given protocol in the WM_PROTOCOLS property.
 *
 * If the client window does not advertise support for the given protocol then the function
 * returns 0.
 *
 * @called_from killclient to tell the window to close (WM_DELETE_WINDOW)
 * @called_from setfocus to tell the window to take focus (WM_TAKE_FOCUS)
 * @calls XGetWMProtocols https://tronche.com/gui/x/xlib/ICC/client-to-window-manager/XGetWMProtocols.html
 * @calls XSendEvent https://tronche.com/gui/x/xlib/event-handling/XSendEvent.html
 * @calls XFree https://tronche.com/gui/x/xlib/display/XFree.html
 * @see https://tronche.com/gui/x/xlib/events/structures.html
 * @see https://tronche.com/gui/x/xlib/events/client-communication/client-message.html
 * @returns 1 if the client window supports the given protocol, 0 otherwise
 *
 * Internal call stack:
 *    ~ -> focus -> setfocus -> sendevent
 *    run -> keypress -> killclient -> sendevent
 *    run -> focusin -> setfocus -> sendevent
 */
int
sendevent(Client *c, Atom proto)
{
	int n; /* to store the number of protocols */
	Atom *protocols;
	int exists = 0; /* whether the desired protocol exists */
	XEvent ev; /* the event structure we are about to send */

	/* The XGetWMProtocols call retrieves the protocols that the client window advertises
	 * via the WM_PROTOCOLS property. In the following example we can see that a window
	 * indicates support for the WM_DELETE_WINDOW and the WM_TAKE_FOCUS protocols, which means
	 * that it accepts receiving client messages using these message types.
	 *
	 * $ xprop | grep WM_PROTOCOLS
	 * WM_PROTOCOLS(ATOM): protocols  WM_DELETE_WINDOW, WM_TAKE_FOCUS, _NET_WM_PING
	 *
	 * Not all windows supports all message types, so the below checks whether the given
	 * protocol is supported by the window.
	 */
	if (XGetWMProtocols(dpy, c->win, &protocols, &n)) {
		while (!exists && n--)
			exists = protocols[n] == proto;
		XFree(protocols);
	}
	/* We only send the event if the client window supports the message type. */
	if (exists) {
		/* If you want to know more about the values set here then refer to the page on
		 * ClientMessage Events (XClientMessageEvent) listed in the function comment above. */
		ev.type = ClientMessage;
		ev.xclient.window = c->win;
		ev.xclient.message_type = wmatom[WMProtocols];
		ev.xclient.format = 32;
		ev.xclient.data.l[0] = proto;
		ev.xclient.data.l[1] = CurrentTime;
		/* This sends the event to the client window. */
		XSendEvent(dpy, c->win, False, NoEventMask, &ev);
	}
	return exists;
}

/* This function tells the window to take focus.
 *
 * @called_by focus to tell the client to take focus
 * @called_by focusin to tell the client to take focus
 * @calls XSetInputFocus https://tronche.com/gui/x/xlib/input/XSetInputFocus.html
 * @calls XChangeProperty https://tronche.com/gui/x/xlib/window-information/XChangeProperty.html
 * @calls sendevent to tell the window to take focus
 * @see updatewmhints for how c->neverfocus is set
 *
 * Internal call stack:
 *    ~ -> focus -> setfocus -> sendevent
 *    run -> focusin -> setfocus -> sendevent
 */
void
setfocus(Client *c)
{
	/* Some windows may provide window manager hints indicating that that it does not handle
	 * input focus. Refer to the updatewmhints function for details on how the c->neverfocus
	 * variable is set in relation to the WM hint.
	 *
	 * One example of this is xclock which is a just a window showing a clock. If you are in
	 * a terminal for example and focus on the xclock window then the xclock border will change
	 * colour as that client is selected, but the input focus will remain in the previous
	 * window (as demonstrated by typing something).
	 */
	if (!c->neverfocus) {
		/* This is what gives input focus to the client, allowing you to type and otherwise
		 * interact with it. */
		XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
		/* This sets the _NET_ACTIVE_WINDOW property for the root window to refer to the
		 * window that is active (has input focus).
		 *
		 * This can be seen by running this command:
		 *
		 * $ xprop -root | grep _NET_ACTIVE_WINDOW
		 * _NET_ACTIVE_WINDOW(WINDOW): window id # 0x5000002
		 */
		XChangeProperty(dpy, root, netatom[NetActiveWindow],
			XA_WINDOW, 32, PropModeReplace,
			(unsigned char *) &(c->win), 1);
	}
	/* This tells the window to take focus by sending a client message event with the
	 * WM_TAKE_FOCUS message type. The event is only sent if the window supports that protocol.
	 */
	sendevent(c, wmatom[WMTakeFocus]);
}

/* This sets or removes fullscreen for a given client.
 *
 * The fullscreen parameter indicates whether the client should be in fullscreen or not.
 *
 * This function has two if statements that handle going into and going out of fullscreen.
 * If the client window is already in the desired state then no action will be taken.
 *
 * @called_from clientmessage in relation to _NET_WM_STATE and _NET_WM_STATE_FULLSCREEN messages
 * @called_from updatewindowtype in relation to _NET_WM_STATE window property
 * @calls XChangeProperty https://tronche.com/gui/x/xlib/window-information/XChangeProperty.html
 * @calls XRaiseWindow https://tronche.com/gui/x/xlib/window/XRaiseWindow.html
 * @calls resizeclient to resize the client going into or out of fullscreen
 * @calls arrange to re-arrange clients when the client exits fullscreen
 *
 * Internal call stack:
 *    run -> clientmessage / updatewindowtype -> setfullscreen
 */
void
setfullscreen(Client *c, int fullscreen)
{
	/* If the desired state is fullscreen and the client window is not in fullscreen then */
	if (fullscreen && !c->isfullscreen) {
		/* This changes the _NET_WM_STATE property of the client window to indicate that the
		 * client is now in fullscreen. */
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
			PropModeReplace, (unsigned char*)&netatom[NetWMFullscreen], 1);
		/* Internal flag used to prevent some actions when a client is in fullscreen. For
		 * example this blocks move or resize being performed on a fullscreen window. */
		c->isfullscreen = 1;
		/* Record the state of the client before it went into fullscreen. This because we
		 * want to revert to that when the client exits fullscreen. */
		c->oldstate = c->isfloating;
		c->oldbw = c->bw;
		/* We do not want a border to be drawn for the fullscreen window. */
		c->bw = 0;
		/* A fullscreen window is floating above other windows. This is primarily to keep
		 * the fullscreen window as-is on top of others in case an arrange or restack should
		 * take place. */
		c->isfloating = 1;
		/* Resize the client to span the entire monitor. Note that we use the monitor
		 * position and size here rather than the coordinates and size of the monitor window
		 * area. More importantly we are calling resizeclient here rather than resize as we
		 * do not want size hints to interfere with the size. */
		resizeclient(c, c->mon->mx, c->mon->my, c->mon->mw, c->mon->mh);
		/* Finally we raise the window to be above everything else. */
		XRaiseWindow(dpy, c->win);
	/* If the desired state is to exit fullscreen and the client window is in fullscreen then */
	} else if (!fullscreen && c->isfullscreen){
		/* This changes the _NET_WM_STATE property of the client window to indicate that the
		 * client is no longer in fullscreen. */
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
			PropModeReplace, (unsigned char*)0, 0);
		/* Change the internal flag to say that the client is not in fullscreen. */
		c->isfullscreen = 0;
		/* Restore the old state, border width, size and position of the client. */
		c->isfloating = c->oldstate;
		c->bw = c->oldbw;
		c->x = c->oldx;
		c->y = c->oldy;
		c->w = c->oldw;
		c->h = c->oldh;
		/* When exiting fullscreen we again make a call to resizeclient rather than resize.
		 * This time it is not because we do not want size hints to interfere, but because
		 * having set the position and size above the call to applysizehints would result in
		 * it returning 0 because as far as it is concerned the size has not changed (and the
		 * resize would not take place). The alternative would be to not set the old
		 * positional and size data, but instead pass these directly to the resize call.
		 * The gotcha with that, however, is that the size and position may need to be
		 * adjusted taking the border width into account. Just restoring the positional and
		 * size data and calling resizeclient directly is a simpler solution. */
		resizeclient(c, c->x, c->y, c->w, c->h);
		/* Finally a full arrange which is more of a catch all in terms of cleaning up any
		 * inconsistencies. Most of the work done by this call will likely be redundant.
		 * In all likelihood the most important aspect of the full arrange call will be the
		 * restack which will take the raised window and place it among all the other windows
		 * (below the bar if tiled, above otherwise). */
		arrange(c->mon);
	}
}

/* User function to set the layout.
 *
 * @called_from keypress in relation to keybindings
 * @called_from buttonpress in relation to button bindings
 * @calls strncpy to update the monitor's layout symbol
 * @calls arrange to reposition and resize clients after the mfact has been changed
 * @calls drawbar to reposition and resize clients after the mfact has been changed
 *
 * Internal call stack:
 *    run -> keypress -> setlayout
 *    run -> buttonpress -> setlayout
 */
void
setlayout(const Arg *arg)
{
	/* Toggle the selected layout if:
	 *    - a NULL argument was passed to setlayout or
	 *    - an argument with value of 0 was passed to setlayout or
	 *    - if the new layout is different to the previous layout
	 */
	if (!arg || !arg->v || arg->v != selmon->lt[selmon->sellt])
		selmon->sellt ^= 1;

	/* Awkwardly this function expects a valid pointer to a layout to be passed as the
	 * argument, e.g.
	 *
	 *    { MODKEY,                       XK_t,      setlayout,      {.v = &layouts[0]} },
	 *    { MODKEY,                       XK_f,      setlayout,      {.v = &layouts[1]} },
	 *    { MODKEY,                       XK_m,      setlayout,      {.v = &layouts[2]} },
	 *
	 * The reason for this, as opposed to just passing 0, 1, 2 as the argument, appears to be
	 * to be able to just revert to the previous layout by passing 0.
	 *
	 *    { MODKEY,                       XK_space,  setlayout,      {0} },
	 *
	 * The moving to the previous layout is triggered by the !arg->v check above resulting in
	 * the selmon->sellt variable being toggled, while the below setting of the layout only
	 * happens if we have a value for arg->v (which we won't have when passing 0).
	 *
	 * Passing an invalid pointer as the layout reference will result in dwm crashing.
	 */
	if (arg && arg->v)
		/* This sets the selected montor's selected layout to the layout provided as the
		 * argument. */
		selmon->lt[selmon->sellt] = (Layout *)arg->v;

	/* Copy the layout symbol of the given layout into the monitor's layout symbol. This is
	 * later used when drawing the layout symbol on the bar. */
	strncpy(selmon->ltsymbol, selmon->lt[selmon->sellt]->symbol, sizeof selmon->ltsymbol);

	/* If there are visible clients on the current monitor then we apply a full arrange to make
	 * clients resize and reposition according to the new layout. */
	if (selmon->sel)
		arrange(selmon);
	/* If there are no visible clients, however, then we do not need to arrange any clients.
	 * We just update the bar to show the new layout symbol. */
	else
		drawbar(selmon);

	/* As an implementation detail for the above - we might as well have just called arrange
	 * rather than checking whether we have any selections or not. The arrange call would have
	 * called arrangemon which would have copied the layout symbol into the monitor and it
	 * would also have called restack which would have called drawbar. */
}

/* User function to set or adjust the master / stack factor (or ratio if you wish) for the
 * selected monitor.
 *
 * The mfact is a floating value with:
 *    - a minimum value of 0.05 (5% of the window area) and
 *    - a maximum value of 0.95 (95% of the window area)
 *
 * As per the default configuration this factor is adjusted with increments or decrements of 0.05
 * using the MOD+l and MOD+h keybindings.
 *
 * The default mfact value is 0.55 giving the master area slightly more space than the stack area.
 *
 * Optionally the user can pass a value greater than 1.0 to set an absolute value, in which case
 * 1.0 will be subtracted from the given value. For example the following keybinding would
 * explicitly set the mfact value to 0.5:
 *
 *     { MODKEY,                       XK_u,      setmfact,       {.f = 1.50} },
 *
 * When setting the mfact value absolutely the value given (less the subtracted 1.0) must fall
 * within the minimum and maximum boundaries for the master / stack factor - otherwise the value
 * will simply be ignored.
 *
 * @called_from keypress in relation to keybindings
 * @calls arrange to reposition and resize clients after the mfact has been changed
 *
 * Internal call stack:
 *    run -> keypress -> setmfact
 */
void
setmfact(const Arg *arg)
{
	float f; /* The next factor value */

	/* If the selected layout for the selected monitor is floating layout (as indicated by
	 * having a NULL arrange function as defined in the layouts array), or if the function is
	 * called without an argument, then we do nothing. */
	if (!arg || !selmon->lt[selmon->sellt]->arrange)
		return;
	/* If the given float argument is less than 1.0 then make a relative adjustment of the mfact
	 * value, otherwise set the mfact value absolutely. */
	f = arg->f < 1.0 ? arg->f + selmon->mfact : arg->f - 1.0;
	/* Check that the next factor value is within the bounds of the minimum of 0.5 and the
	 * maximum of 0.95. If it is not then we bail out here */
	if (f < 0.05 || f > 0.95)
		return;
	/* Set the master / stack factor to the new value */
	selmon->mfact = f;
	/* This makes a call to arrange so that the tiled windows are resized and repositioned
	 * following the change to the master / stack factor. In principle this could have been a
	 * call to arrangemon(selmon) as all that is needed is for the clients to be tiled again.
	 *
	 * The call to arrange will result in subsequent calls to showhide, arrangemon, restack and
	 * drawbar to redraw the bar. While not strictly necessary in this case the performance
	 * overhead is negligible and arrange is a catch all that can prevent obscure issues.
	 */
	arrange(selmon);
}

/* The setup call will initialise everything that we need for operational purposes.
 *
 * This involves:
 *    - setting up monitors
 *    - loading fonts
 *    - creating colours for the colour schemes
 *    - creating cursors
 *    - creating the bars
 *    - setting window manager hints
 *    - telling the X server what kind of events the window manager is interested in
 *
 * @called from main as part of initialisation process
 * @calls XInternAtom https://tronche.com/gui/x/xlib/window-information/XInternAtom.html
 * @calls XCreateSimpleWindow https://tronche.com/gui/x/xlib/window/XCreateWindow.html
 * @calls XChangeProperty https://tronche.com/gui/x/xlib/window-information/XChangeProperty.html
 * @calls XDeleteProperty https://tronche.com/gui/x/xlib/window-information/XDeleteProperty.html
 * @calls XChangeWindowAttributes https://tronche.com/gui/x/xlib/window/XChangeWindowAttributes.html
 * @calls XSelectInput https://tronche.com/gui/x/xlib/event-handling/XSelectInput.html
 * @calls DefaultScreen https://linux.die.net/man/3/defaultscreen
 * @calls DisplayWidth https://linux.die.net/man/3/displaywidth
 * @calls DisplayHeight https://linux.die.net/man/3/displayheight
 * @calls RootWindow https://linux.die.net/man/3/rootwindow
 * @calls sigaction https://man7.org/linux/man-pages/man2/sigaction.2.html
 * @calls sigemptyset https://man7.org/linux/man-pages/man3/sigemptyset.3p.html
 * @calls waitpid https://linux.die.net/man/3/waitpid
 * @calls ecalloc to allocate space for the colour schemes (see util.c)
 * @calls drw_create to create the drawable (see drw.c)
 * @calls drw_fontset_create to create the font set (see drw.c)
 * @calls drw_cur_create to create cursors (see drw.c)
 * @calls drw_scm_create to create the colour schemes (see drw.c)
 * @calls focus to set input focus on the root window
 * @calls grabkeys to register for keypress notifications
 * @calls updatebars to create the bar window for each monitor
 * @calls updategeom to create the monitors based on Xinerama information
 * @calls updatestatus to initialise the status text variable
 * @see https://tronche.com/gui/x/xlib/display/display-macros.html
 * @see https://tronche.com/gui/x/xlib/introduction/overview.html
 * @see https://specifications.freedesktop.org/wm-spec/1.3/ar01s03.html
 * @see https://tronche.com/gui/x/xlib/events/processing-overview.html
 * @see https://pubs.opengroup.org/onlinepubs/9699919799/functions/_Exit.html#tag_16_01_03_01
 * @see https://www.gnu.org/software/libc/manual/html_node/Interrupted-Primitives.html
 *
 * Internal call stack:
 *    main -> setup
 */
void
setup(void)
{
	int i;
	XSetWindowAttributes wa;
	Atom utf8string;
	struct sigaction sa;

	/* Do not transform children into zombies when they terminate. */

	/* The sigemptyset function initialises an empty signal set; the return value indicating
	 * success or failure is ignored. */
	sigemptyset(&sa.sa_mask);

	/* This sets the signal action flags that we want:
	 *
	 * SA_NOCLDSTOP - The default behaviour is that whenever a process dies or stops, or when
	 *                a stopped child process continues execution, the kernel posts the SIGCHLD
	 *                signal to the parent process. The SA_NOCLDSTOP flag means that no SIGCHLD
	 *                will occur when a child process stops or resumes, i.e. we will only
	 *                receive a SIGCHLD when the child process dies (terminates).
	 * SA_NOCLDWAIT - This flags means that in the case when a SIGCHLD is received then do not
	 *                transform the child process into a zombie process.
	 * SA_RESTART   - It is possible for a signal to arrive and be handled while a primitive I/O
	 *                operation such as open or read is waiting on an I/O device to respond. The
	 *                POSIX way of handling this scenario is to make the primitive fail straight
	 *                away with the EINTR error code. This means that POSIX applications that use
	 *                signal handlers must check for EINTR errors after calls to library functions
	 *                that can return it, and forgetting to check this is a common source of error.
	 *                BSD on the other hand avoids EINTR entirely by providing a more convenient
	 *                approach: to restart the interrupted primitive rather than making it fail.
	 *                Using this approach one do not have to be concerned about checking for EINTR
	 *                errors. The SA_RESTART flag tells the signal action handler to behave like
	 *                BSD does by making certain system calls restartable across signals.
	 */
	sa.sa_flags = SA_NOCLDSTOP | SA_NOCLDWAIT | SA_RESTART;
	/* This will set the signal handler and SIG_IGN will simply ignore any signals and we only
	 * expect the SIGCHLD signal to be received when a child process dies. */
	sa.sa_handler = SIG_IGN;
	/* This sets our desired action when a SIGCHLD signal is received. */
	sigaction(SIGCHLD, &sa, NULL);

	/* Clean up any zombies (inherited from .xinitrc etc) immediately. The need for this may not
	 * be immediately obvious, but for example when the .xinitrc script runs it may spawn other
	 * processes. Typically at the end the exec command will be used, which results in the
	 * process itself to be replaced with whatever is being executed. Consider the following
	 * scenario:
	 *
	 *    sleep 10 &
	 *    exec dwm
	 *
	 * Now let's say that this is being evaluated with a PID 1111. The script will run the command
	 * of sleep 10 as a child process running in the background (e.g. PID 3291 with parent process
	 * 1111). Then exec dwm will replace the process evaluating .xinitrc and start executing as
	 * dwm (i.e. PID 1111 is now executing dwm). The result of this is that the sleep is now
	 * (still) a child process of dwm. When that child process exists after the 10 seconds have
	 * elapsed it will result in a SIGCHLD and it will be dealt with by the sigaction handler as
	 * defined above.
	 *
	 * What's with the waitpid then? Let's consider this other scenario:
	 *
	 *    cat &
	 *    exec dwm
	 *
	 * Here the command of cat is run as a child process (e.g. PID 5183 with parent process 1111).
	 * As before the exec will replace the current process and start executing dwm. The cat process
	 * will have died before the sigaction handler was set up before so it will just end up as a
	 * defunct zombie process waiting to terminate. The waitpid here will find and deal with (i.e.
	 * ignore) any child processes that are waiting to terminate thus avoiding zombie processes.
	 *
	 * The function signature is:
	 *
	 *    waitpid (pid_t pid, int *status-ptr, int options)
	 *
	 * We pass the pid of -1 to say that we are interested in any process (as opposed to getting
	 * information on a particular process). This could also have been set to WAIT_ANY.
	 *
	 * We pass NULL for the status pointer as we do not care about looking up status information
	 * for child processes.
	 *
	 * For the options we pass the WNOHANG flag which just means that the function should return
	 * immediately instead of waiting in the event that there are no child processes found.
	 *
	 * The outer while is in case there are more than one zombie process waiting to terminate.
	 */
	while (waitpid(-1, NULL, WNOHANG) > 0);

	/* Initialise the screen.
	 *
	 * The DefaultScreen macro returns the default screen number. The screen number is used
	 * to retrieve the height and width of the screen as well as the root window.
	 *
	 * The screen number is also used to find the default depth and visual when creating the
	 * bar window(s).
	 */
	screen = DefaultScreen(dpy);
	sw = DisplayWidth(dpy, screen);
	sh = DisplayHeight(dpy, screen);

	/* The root window is the window at the top of the window hierarchy and it covers each of
	 * the display screens. If you set a background wallpaper then those graphics are drawn on
	 * the root window. The root window plays an important role in the window manager as we
	 * refer to it when creating windows, when scanning for windows, when finding the mouse
	 * coordinates, when grabbing key and button presses and more. A window manager also
	 * communicates its capabilities and support to other windows by setting properties on the
	 * root window. */
	root = RootWindow(dpy, screen);

	/* This sets up the drawable (drw) which is an internal structure defined in drw.h which
	 * holds the root window, the connection to the X server, the screen number, the colour
	 * schemes, fonts, the graphics context and the drawable pixel map. */
	drw = drw_create(dpy, screen, root, sw, sh);

	/* This goes through all the fonts in the fonts array defined in the configuration file and
	 * loads them. If we were not able to load any fonts then we can't proceed as the bar
	 * depends on having a font.*/
	if (!drw_fontset_create(drw, fonts, LENGTH(fonts)))
		die("no fonts could be loaded.");

	/* The left + right padding for text drawn on the bar is set to the height of the
	 * (primary) font. This means that the left padding of text will be half of that. */
	lrpad = drw->fonts->h;

	/* The bar height set to be the (primary) font height + 2 pixels, one pixel below and one
	 * pixel above the text. */
	bh = drw->fonts->h + 2;

	/* The call to updategeom creates the monitor(s) based on Xinerama information, or it
	 * creates a single monitor that spans all screens in the event that Xinerama is not
	 * enabled for the screen or dwm is compiled without Xinerama support. */
	updategeom();

	/* Initialise atoms. This looks up the atom ID numbers for later use. */
	/* The utf8string is only used once when setting the WM_NAME property of the supporting
	 * window. */
	utf8string = XInternAtom(dpy, "UTF8_STRING", False);

	/* Looking up Window Management atoms:
	 *    WMProtocols - used in sendevent
	 *    WMDelete - used in killclient
	 *    WMState - used in getstate and setclientstate
	 *    WMTakeFocus - used in setfocus
	 */
	wmatom[WMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", False);
	wmatom[WMDelete] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	wmatom[WMState] = XInternAtom(dpy, "WM_STATE", False);
	wmatom[WMTakeFocus] = XInternAtom(dpy, "WM_TAKE_FOCUS", False);

	/* Looking up net atoms:
	 *    NetActiveWindow - used in cleanup, clientmessage, focus, setfocus and unfocus
	 *    NetSupported - used in setup to indicate what window manager hints are available
	 *    NetWMName - used in updatetitle, propertynotify and setup
	 *    NetWMState - used in clientmessage, setfullscreen, updatewindowtype
	 *    NetWMCheck - used in setup to indicate supporting window
	 *    NetWMFullscreen - used in clientmessage, setfullscreen and updatewindowtype
	 *    NetWMWindowType - used in propertynotify and updatewindowtype
	 *    NetWMWindowTypeDialog - used in updatewindowtype
	 *    NetClientList - used in manage, setup and updateclientlist
	 */
	netatom[NetActiveWindow] = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
	netatom[NetSupported] = XInternAtom(dpy, "_NET_SUPPORTED", False);
	netatom[NetWMName] = XInternAtom(dpy, "_NET_WM_NAME", False);
	netatom[NetWMState] = XInternAtom(dpy, "_NET_WM_STATE", False);
	netatom[NetWMCheck] = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
	netatom[NetWMFullscreen] = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
	netatom[NetWMWindowType] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
	netatom[NetWMWindowTypeDialog] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
	netatom[NetClientList] = XInternAtom(dpy, "_NET_CLIENT_LIST", False);

	/* Initialise different cursors for when resizing and moving windows. */
	cursor[CurNormal] = drw_cur_create(drw, XC_left_ptr);
	cursor[CurResize] = drw_cur_create(drw, XC_sizing);
	cursor[CurMove] = drw_cur_create(drw, XC_fleur);

	/* Initialise colour schemes. Allocate memory to hold pointers to all colour schemes. */
	scheme = ecalloc(LENGTH(colors), sizeof(Clr *));

	/* Loop through all the entries in the colors array */
	for (i = 0; i < LENGTH(colors); i++)
		/* Then create the colour scheme for each entry. The last argument 3 represents the
		 * number of colours in each colour scheme array (foreground, background and border
		 * colours). */
		scheme[i] = drw_scm_create(drw, colors[i], 3);

	/* Initialise the bars. The call to updatebars creates the bar window for each monitor. */
	updatebars();

	/* The call to updatestatus is only to initialise the status text (stext) variable with
	 * "dwm-6.3" and to update the bar. */
	updatestatus();

	/* Supporting window for NetWMCheck. In order to be taken seriously and to be considered as
	 * a valid, compliant and proper window manager we need to have a dummy window representing
	 * the window manager.
	 *
	 * As per https://specifications.freedesktop.org/wm-spec/1.3/ar01s03.html we have that:
	 *    The Window Manager MUST set the _NET_SUPPORTING_WM_CHECK property on the root window
	 *    to be the ID of a child window created by himself, to indicate that a compliant window
	 *    manager is active. The child window MUST also have the _NET_SUPPORTING_WM_CHECK
	 *    property set to the ID of the child window. The child window MUST also have the
	 *    _NET_WM_NAME property set to the name of the Window Manager.
	 *
	 *    Rationale: The child window is used to distinguish an active Window Manager from a
	 *    stale _NET_SUPPORTING_WM_CHECK property that happens to point to another window. If
	 *    the _NET_SUPPORTING_WM_CHECK window on the client window is missing or not properly
	 *    set, clients SHOULD assume that no conforming Window Manager is present.
	 *
	 * We can see this in practice by running the below commands:
	 *
	 *    $ xprop -root | grep _NET_SUPPORTING_WM_CHECK\(
	 *    _NET_SUPPORTING_WM_CHECK(WINDOW): window id # 0x60002d
	 *
	 *    $ xprop -id 0x60002d
	 *    _NET_WM_NAME(UTF8_STRING) = "dwm"
	 *    _NET_SUPPORTING_WM_CHECK(WINDOW): window id # 0x60002d
	 *
	 * The next line creates our 1x1 pixel supporting window.
	 */
	wmcheckwin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);

	/* This sets the _NET_SUPPORTING_WM_CHECK property on the supporting window referring to
	 * its own window ID. */
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMCheck], XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &wmcheckwin, 1);

	/* This sets the _NET_WM_NAME property on the supporting window with the name of the window
	 * manager. */
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMName], utf8string, 8,
		PropModeReplace, (unsigned char *) "dwm", 3);

	/* This sets the _NET_SUPPORTING_WM_CHECK on the root window referring to the supporting
	 * window's window ID. */
	XChangeProperty(dpy, root, netatom[NetWMCheck], XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &wmcheckwin, 1);

	/* EWMH support per view. */

	/* This sets the _NET_SUPPORTED list which indicates what extended window manager hints
	 * the window manager supports.
	 *
	 *    xprop -root | grep _NET_SUPPORTED
	 *    _NET_SUPPORTED(ATOM) = _NET_SUPPORTED, _NET_WM_NAME, _NET_WM_STATE,
	 *    _NET_SUPPORTING_WM_CHECK, _NET_WM_STATE_FULLSCREEN, _NET_ACTIVE_WINDOW,
	 *    _NET_WM_WINDOW_TYPE, _NET_WM_WINDOW_TYPE_DIALOG, _NET_CLIENT_LIST
	 */
	XChangeProperty(dpy, root, netatom[NetSupported], XA_ATOM, 32,
		PropModeReplace, (unsigned char *) netatom, NetLast);

	/* This deletes the _NET_CLIENT_LIST property that holds a reference to all windows that are
	 * managed by the window manager. The reason why this is deleted is that we are initialising
	 * the window manager and the property may exist on the root window from a previous window
	 * manager. The data in this property would be misleading as dwm is not managing the
	 * referenced windows at this point in time. */
	XDeleteProperty(dpy, root, netatom[NetClientList]);

	/* This sets the default cursor to be the left pointer. */
	wa.cursor = cursor[CurNormal]->cursor;

	/* This sets the event mask which indicates to the X server what events this window manager
	 * is interested in receiving notifications for.
	 *
	 *    SubstructureRedirectMask - to receive ConfigureRequest and MapRequest events
	 *    SubstructureNotifyMask   - to receive ConfigureNotify, DestroyNotify, MapNotify and
	 *                               UnmapNotify events (indicating a change for child windows)
	 *    ButtonPressMask          - to receive ButtonPress events
	 *    PointerMotionMask        - to receive MotionNotify events
	 *    EnterWindowMask          - to receive EnterNotify events
	 *    LeaveWindowMask          - to receive LeaveNotify events
	 *    StructureNotifyMask      - to receive ConfigureNotify, DestroyNotify, MapNotify and
	 *                               UnmapNotify events (indicating a change for a window)
	 *    PropertyChangeMask       - to receive PropertyNotify events
	 *
	 * The above event masks do open for other other event types not listed above to come
	 * through. These will be ignored by the event handler.
	 *
	 * See https://tronche.com/gui/x/xlib/events/processing-overview.html for a list of event
	 * types and what event masks they correspond to.
	 */
	wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask
		|ButtonPressMask|PointerMotionMask|EnterWindowMask
		|LeaveWindowMask|StructureNotifyMask|PropertyChangeMask;

	/* This sets the cursor and the event mask specified above for the root window. */
	XChangeWindowAttributes(dpy, root, CWEventMask|CWCursor, &wa);

	/* This call requests that the X server report events associated with the event mask
	 * specified above. */
	XSelectInput(dpy, root, wa.event_mask);

	/* This goes through all the keybindings to tell the X server what key press combinations
	 * the window manager wants to receive KeyPress events for. */
	grabkeys();

	/* Given that we are only starting up and have not yet called scan to find any client
	 * windows the focus call below is to revert the input focus back to the root window. */
	focus(NULL);
}

/* This sets or removes the urgency hint for the given client.
 *
 * If the given urg value is 1 then:
 *    - c->isurgent will be set to 1 and
 *    - the XUrgencyHint will be added to the client's window manager hints
 *
 * If the given urg value is 0 then:
 *    - c->isurgent will be set to 0 and
 *    - the XUrgencyHint will be removed from the client's window manager hints
 *
 * If a client on has the urgency bit set then this can cause the colours for the tag to be
 * inverted (background and foreground colours swap) in the bar. See the drawbar function for how
 * the c->isurgent value is used to achieve this.
 *
 * @called_from focus to remove the urgency bit when the client receives focus
 * @called_from clientmessage if a _NET_ACTIVE_WINDOW message type is received and the client is
 *                            not the selected client
 * @calls XGetWMHints https://tronche.com/gui/x/xlib/ICC/client-to-window-manager/XGetWMHints.html
 * @calls XSetWMHints https://tronche.com/gui/x/xlib/ICC/client-to-window-manager/XSetWMHints.html
 * @calls XFree https://tronche.com/gui/x/xlib/display/XFree.html
 * @see updatewmhints which also sets c->isurgent based on the urgency hint
 * @see drawbar for how the c->isurgent is used to indicate urgent tags in the bar
 * @see clientmessage for an example showing how you can test the setting of the urgency bit
 *
 * Internal call stack:
 *    ~ -> focus -> seturgent
 *    run -> clientmessage -> seturgent
 */
void
seturgent(Client *c, int urg)
{
	XWMHints *wmh;

	/* This sets the internal urgency flag for the client */
	c->isurgent = urg;
	/* This retrieves window manager hints for the client window. If the client does not have
	 * any then we bail here. */
	if (!(wmh = XGetWMHints(dpy, c->win)))
		return;
	/* This updates the window manager hints flags by either adding or removing the XUrgencyHint
	 * bit. This is a straightforward binary operation, but may warrant some explaining for
	 * those less familiar with binary operators.
	 *
	 *    (wmh->flags | XUrgencyHint)  - The binary or (|) combines the existing flags with
	 *                                   the flag of XUrgencyHint, as in keep the existing
	 *                                   flags and add XUrgencyHint.
	 *    (wmh->flags & ~XUrgencyHint) - The binary and (&) only includes the bits that are
	 *                                   present on both sides of the operator and the bitwise
	 *                                   not (~) operator inverts the value of XUrgencyHint.
	 *                                   What the statement says is to keep all flags except
	 *                                   XUrgencyHint, as in remove that flag from the
	 *                                   existing flags.
	 *
	 * The ternary operator is just a shorthand for if, else. This could also have been written
	 * like this:
	 *
	 *    if (urg)
	 *        wmh->flags |= XUrgencyHint;
	 *    else
	 *        wmh->flags &= ~XUrgencyHint;
	 *
	 * What is more readable is subjective, but given the project's goal of being a window
	 * manager of less than 2000 source lines of code (SLOC) it is clear that the ternary
	 * operator is preferred.
	 */
	wmh->flags = urg ? (wmh->flags | XUrgencyHint) : (wmh->flags & ~XUrgencyHint);
	/* This updates the window manager hints for the client window following the above change */
	XSetWMHints(dpy, c->win, wmh);
	XFree(wmh);
}

/* This is a recursive function that moves client windows into or out of view depending on whether
 * the tag(s) they are shown on are viewed or not.
 *
 * Client windows are shown top down when the are moved into view, and are hidden bottom up when
 * moved out of view.
 *
 * As an example let's say that we have a floating layout with a series of windows that are placed
 * on top of each other. The order of the clients is determined by the monitor stack where the
 * window that most recently had focus is at the top of the stack and the least recently used
 * window will be at the bottom of the stack.
 *
 * When moving client windows into view we start to move clients from the top of the stack, then we
 * call showhide again to move the next client in the stack.
 *
 * If this was the other way around then the window at the far bottom of the stack would be moved
 * into view first, then the next after that, and so on until the last window that is shown on top
 * of all the other windows is moved into view. This would give a very noticeable effect as the X
 * server would have to draw each window as they are moved in and overlap.
 *
 * When windows are shown top down then the window at the top of the stack is moved into view first
 * and the X server will not have to draw anything for any of the subsequent windows whose view is
 * obscured by the topmost window.
 *
 * The same logic applies when moving windows out of view. If we were to hide windows top down then
 * the topmost window would be moved out of view first, revealing the windows beneath it. The next
 * window would reveal more windows and so on and the X server would have to draw each window being
 * revealed. By hiding windows bottom up we avoid that as the topmost window obscures the view of
 * the windows below it until that too is moved away.
 *
 * Most window managers use the IconicState and NormalState window states for the purpose of moving
 * windows out of and into view. The iconic state means that the window is not shown on the screen
 * but is shown as an icon in the bar (i.e. it is minimised).
 *
 * In dwm the windows are merely moved out of view to a negative X position (which is determined by
 * the width of the client). In practice this means that the window is placed on the immediate left
 * of the leftmost monitor, just out of view.
 *
 * @called_from arrange to bring clients into and out of view depending on what tags are shown
 * @called_from showhide in a recursive manner for each client in the client stack
 * @calls XMoveWindow https://tronche.com/gui/x/xlib/window/XMoveWindow.html
 * @calls resize for all visible floating clients
 * @calls showhide in a recursive manner for each client in the client stack
 *
 * Internal call stack:
 *    ~ -> arrange -> showhide -> showhide
 *                       ^___________/
 */
void
showhide(Client *c)
{
	if (!c)
		return;
	if (ISVISIBLE(c)) {
		/* Show clients top down. */
		XMoveWindow(dpy, c->win, c->x, c->y);
		/* This applies a resize call if the window is floating or the floating layout is
		 * used, as long as the window is not also in fullscreen.
		 *
		 * The only practical need for this resize call would be in the event that the size
		 * hints of a window has been updated while it has been out of view.
		 */
		if ((!c->mon->lt[c->mon->sellt]->arrange || c->isfloating) && !c->isfullscreen)
			resize(c, c->x, c->y, c->w, c->h, 0);

		showhide(c->snext);
	} else {
		/* Hide clients bottom up */
		showhide(c->snext);
		/* Move the window out of sight. */
		XMoveWindow(dpy, c->win, WIDTH(c) * -2, c->y);
	}
}

/* This starts a new program by executing a given execvp command.
 *
 * @called_from keypress in relation to keybindings
 * @called_from buttonpress in relation to keybindings
 * @calls fork https://linux.die.net/man/2/fork
 * @calls close https://linux.die.net/man/2/close
 * @calls sigaction https://man7.org/linux/man-pages/man2/sigaction.2.html
 * @calls sigemptyset https://man7.org/linux/man-pages/man3/sigemptyset.3p.html
 * @calls ConnectionNumber https://linux.die.net/man/3/connectionnumber
 * @calls setsid https://linux.die.net/man/2/setsid
 * @calls execvp https://linux.die.net/man/3/execvp
 * @calls die to print errors and exit dwm (see util.c)
 * @see https://man7.org/linux/man-pages/man2/fork.2.html
 * @see https://tronche.com/gui/x/xlib/display/display-macros.html
 *
 * Internal call stack:
 *    run -> keypress -> spawn
 *    run -> buttonpress -> spawn
 */
void
spawn(const Arg *arg)
{
	struct sigaction sa;

	/* If we are executing the dmenu command then we manipulate the value that we pass to
	 * dmenu_run via the -m argument by setting it to the selected monitor.
	 *
	 * For reference here is what the dmenu command looks like in config.def.h:
	 *
	 *    static char dmenumon[2] = "0"; // component of dmenucmd, manipulated in spawn()
	 *    static const char *dmenucmd[] = { "dmenu_run", "-m", dmenumon, "-fn", ...
	 *
	 * The reason for passing this argument to dmenu is to control which monitor dmenu spawns
	 * on. This is likely due to legacy reasons given that dmenu is capable of working out which
	 * monitor currently has input focus on its own. It is perfectly safe to delete the two
	 * lines below and remove the -m argument from the command.
	 */
	if (arg->v == dmenucmd)
		dmenumon[0] = '0' + selmon->num;

	/* This next call does some magic that may be hard to grasp. Let's demystify.
	 *
	 * This call to fork forks the process. What this actually means is that it creates a new
	 * (duplicate) process of the current process; in other words we end up with two dwm
	 * processes.
	 *
	 * For process 1 (this process) the fork() call returns the process ID of the new process.
	 * As such we do not enter the if statement and we return from the spawn function and dwm
	 * eventually goes back to the event loop after checking the remaining key bindings.
	 *
	 * For process 2 (the new process) the fork() call returns 0 and we enter the if statement.
	 * This then calls execvp which replaces the current process image with a new process image,
	 * as in it becomes the new process. If the call to execvp fails for whatever reason then
	 * it will continue to print an error and call exit to stop the process.
	 *
	 * Processes spawned via dwm will have the dwm process as its parent process.
	 */
	if (fork() == 0) {
		/* If we have a connection to the X server then close that before proceeding. */
		if (dpy)
			close(ConnectionNumber(dpy));

		/* The call to setsid creates a new session and sets the process group ID. This is
		 * needed because a child created via fork inherits its parent's session ID and we
		 * need our own because this session ID will be preserved across the execvp call. */
		setsid();

		/* This restores SIGCHLD sighandler to default before spawning a program.
		 *
		 * From sigaction(2):
		 * A child created via fork(2) inherits a copy of its parent's signal dispositions.
		 * During an execve(2), the dispositions of handled signals are reset to the default;
		 * the dispositions of ignored signals are left unchanged.
		 *
		 * The reason why this is needed is that some programs would not start due to inheriting
		 * the signal handler of dwm which ignores all signals. */
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;
		sa.sa_handler = SIG_DFL;
		sigaction(SIGCHLD, &sa, NULL);

		/* The execvp causes the program that is currently being run (dwm in this case) to
		 * be replaced with a new program and with a newly initialised stack, heap and data
		 * segments. If this is successful then this is the last thing this process does in
		 * the dwm code. */
		execvp(((char **)arg->v)[0], (char **)arg->v);
		/* If the execvp fails for whatever reason, then we are still here executing dwm
		 * code. So we make a call to die to print an error to say that we failed to execute the
		 * command before calling exit to ensure that this process stops running. */
		die("dwm: execvp '%s' failed:", ((char **)arg->v)[0]);
	}
}

/* The tag function moves the selected client to a given tag.
 *
 * This is referenced in the TAGKEYS macro which sets up keybindings for each individual tag.
 *
 * @called_from keypress in relation to keybindings
 * @called_from buttonpress in relation to button bindings
 * @calls focus to give input focus to the next client in the stack
 * @calls arrange as the client may have been been moved out of view
 * @see TAGMASK macro
 *
 * Internal call stack:
 *    run -> keypress -> tag
 *    run -> buttonpress -> tag
 */
void
tag(const Arg *arg)
{
	/* Don't proceed if there are no selected clients or the given argument is not for any
	 * valid tag. */
	if (selmon->sel && arg->ui & TAGMASK) {
		/* This sets the new tagmask for the selected client. */
		selmon->sel->tags = arg->ui & TAGMASK;
		/* Give input focus to the next client in the stack as the client may have been
		 * moved to a tag that is not viewed. */
		focus(NULL);
		/* A full arrange to resize and reposition the remaining clients as well as to
		 * update the bar. */
		arrange(selmon);
	}
}

/* User function to move a the selected client to a monitor in a given direction.
 *
 * @called_from keypress in relation to keybindings
 * @calls dirtomon to work out which monitor the direction refers to
 * @calls sendmon to send the client to the monitor returned by dirtomon
 *
 * Internal call stack:
 *    run -> keypress -> tagmon
 */
void
tagmon(const Arg *arg)
{
	/* Bail if there are no selected client or if we have a single monitor */
	if (!selmon->sel || !mons->next)
		return;
	/* The call to dirtomon works out which monitor is next or previous in line, while the
	 * sendtomon function handles the actual transfer of ownership of the client between
	 * monitors. */
	sendmon(selmon->sel, dirtomon(arg->i));
}

/* This is what handles the tile layout arrangement.
 *
 * @called_from arrangemon
 * @calls nexttiled to get the next tiled client
 * @calls resize to change the size and position of client windows
 *
 * Internal call stack:
 *    ~ -> arrange -> arrangemon -> tile
 */
void
tile(Monitor *m)
{
	/* Variables:
	 *    i - iterator, represents number of clients processed
	 *    n - total number of clients
	 *    h - calculated client height
	 *    mw - calculated monitor width
	 *    my - calculated master area y position relative to the window area
	 *    ty - calculated stack area y position relative to window area (tile y, the naming is
	 *         likely a remnant from a time before the nmaster patch was applied upstream -
	 *         before that the master area had only one client and the remaining clients would
	 *         be tiled in the tile area)
	 */
	unsigned int i, n, h, mw, my, ty;
	Client *c;

	/* This loop just counts the number of tiled clients storing the count in the variable n. */
	for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);
	/* If we have no tiled clients then there is nothing to do, stop processing now. */
	if (n == 0)
		return;

	/* The general idea here is that we have a master area where the master client(s) are tiled
	 * and a stack area where the remaining clients are tiled.
	 *
	 * The number of clients in the master area is controlled using nmaster.
	 *
	 * In principle the code below is not that complicated, but something that does make it a
	 * bit convoluted are the two exceptional cases where:
	 *    - nmaster is 0, in which case only the stack area is drawn and
	 *    - nmaster is greater than n, in which case only the master area is drawn
	 */

	/* If we have enough clients for both the master and the stack area then we split the
	 * window area in two by applying the master stack factor (mfact). */
	if (n > m->nmaster)
		/* But in the exceptional case that nmaster is 0 then we also set the master area
		 * width to 0. This because all the clients will be drawn in the stack area, and
		 * the stack area subtracts mw from the available width. */
		mw = m->nmaster ? m->ww * m->mfact : 0;
	else
		/* If we have less clients than nmaster then all clients will be drawn in the
		 * master area and thus the master area takes up the entire window area. */
		mw = m->ww;

	/* This loops through all clients initialising i, the master y (my), and the stack y (ty)
	 * to 0 while incrementing i for each client processed. */
	for (i = my = ty = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++)
		/* If this client goes into the master area (this includes the case where all
		 * clients go into the master area). */
		if (i < m->nmaster) {
			/* Here we calculate the height of the client based on the remaining space
			 * and the number of clients left to place.
			 *
			 *    (m->wh - my)        - the remaining space
			 *    MIN(n, m->nmaster)  - this covers for the exceptional case where
			 *                          nmaster is greater than the number of clients,
			 *                          imagine if nmaster is 8 and we have 6 clients
			 *    (MIN(...) - i)      - the number of remaining clients
			 *
			 * Putting this together we have that the height h is the remaining space
			 * divided by the remaining clients.
			 */
			h = (m->wh - my) / (MIN(n, m->nmaster) - i);

			/* This resizes and positions the client accordingly.
			 *
			 *    m->wx          - the window area x position
			 *    m->wy + my     - the window area y position + master client y position
			 *    mw - (2*c->bw) - the width of the client, defined earlier to be either
			 *                     the entire width of the monitor window area or the
			 *                     width of the master area after mfact has been
			 *                     applied, we subtract the border width from the size
			 *    h - (2*c->bw)  - the calculated height of the client, we subtract the
			 *                     border width from the size
			 *    0              - this resize is not the result of the user interacting
			 *                     with the window
			 */
			resize(c, m->wx, m->wy + my, mw - (2*c->bw), h - (2*c->bw), 0);

			/* We increment the master y position with the height of the client after
			 * the resize so that we know where the next client can be positioned.
			 *
			 * The if statement is a guard to prevent the my variable growing larger
			 * than the window area height, in which case the height calculation above
			 * would result in a negative value - and a negative value for an unsigned
			 * int results in a really really big number causing a crash. */
			if (my + HEIGHT(c) < m->wh)
				my += HEIGHT(c);
		/* Otherwise the client goes into the stack area (this includes the case where
		 * nmaster is 0 and all clients go into the stack area). */
		} else {
			/* Here we calculate the height of the client based on the remaining space
			 * and the number of clients left to place.
			 *
			 *    (m->wh - ty)        - the remaining space
			 *    (n - i)             - the number of remaining clients
			 */
			h = (m->wh - ty) / (n - i);

			/* This resizes and positions the client accordingly.
			 *
			 *    m->wx + mw     - the window area x position + master width gives the
			 *                     stack area x position (mw can be 0)
			 *    m->wy + ty     - the window area y position + stack client y position
			 *    m->ww - mw     - the width of the client in the stack area is the
			 *      - (2*c->bw)    remaining space after master width has been deducted,
			 *                     we subtract the border width from the size
			 *    h - (2*c->bw)  - the calculated height of the client, we subtract the
			 *                     border width from the size
			 *    0              - this resize is not the result of the user interacting
			 *                     with the window
			 */
			resize(c, m->wx + mw, m->wy + ty, m->ww - mw - (2*c->bw), h - (2*c->bw), 0);

			/* We increment the stack y position with the height of the client after
			 * the resize so that we know where the next client can be positioned. */
			if (ty + HEIGHT(c) < m->wh)
				ty += HEIGHT(c);
		}

	/* Now following that how come the implementation is so complicated in that it continuously
	 * calculates the remaining space for each client? Why does it not just simply divide the
	 * available space by the number of clients and leave it at that?
	 *
	 * The reason for why it is implemented in this way has specifically to do with size hints
	 * in that a client like the simple terminal (st) for example would not be able to utilise
	 * all the space given. By default size hints are respected in tiled resizals, and by
	 * calculating the size one client at a time and only incrementing by the size that was
	 * used after size hints has been applied the space usage is more or less optimised.
	 * Another thing to consider is that no matter how you divide the available space there
	 * will always be the case where some divisions will give remainder pixels that are not
	 * allocated. The way windows are tiled here the last client to be tiled in each respective
	 * area will receive the remaining space. This is why the bottom client in the stack area
	 * often appears larger than the rest.
	 */
}

/* User function to toggle the display on and off on the selected monitor.
 *
 * @called_from keypress in relation to keybindings
 * @calls updatebarpos to adjust the monitor's window area
 * @calls XMoveResizeWindow https://tronche.com/gui/x/xlib/window/XMoveResizeWindow.html
 * @calls arrange to reposition and resize tiled clients
 *
 * Internal call stack:
 *    run -> keypress -> togglebar
 */
void
togglebar(const Arg *arg)
{
	/* Toggle the internal flag indicating whether the bar is shown or not */
	selmon->showbar = !selmon->showbar;
	/* The call to updatebarpos makes dwm adjust:
	 *    - the bar y position (m->by) depending on whether the bar is shown or not and
	 *    - the monitor's window area
	 */
	updatebarpos(selmon);
	/* This moves the bar window into or out of view depending on the value of m->by which is
	 * set by the updatebarpos call above */
	XMoveResizeWindow(dpy, selmon->barwin, selmon->wx, selmon->by, selmon->ww, bh);
	/* A final arrange call to make tiled clients take advantage of the additional space when
	 * the bar is toggled away, or to give room for the bar when the bar is shown. */
	arrange(selmon);
}

/* User function to toggle the floating status for the selected client.
 *
 * @called_from keypress in relation to keybindings
 * @called_from buttonpress in relation to button bindings
 * @called_from movemouse when a tiled window snaps out and becomes floating
 * @called_from resizemouse when a tiled window snaps out and becomes floating
 * @calls resize to apply size hints for the client when it becomes floating
 * @calls arrange to reposition and resize tiled clients
 *
 * Internal call stack:
 *    run -> keypress -> togglefloating
 *    run -> buttonpress -> movemouse / resizemouse -> togglefloating
 *    run -> buttonpress -> togglefloating
 */
void
togglefloating(const Arg *arg)
{
	/* If there is no selected client then bail. */
	if (!selmon->sel)
		return;
	/* If the selected client is in fullscreen then bail. */
	if (selmon->sel->isfullscreen) /* no support for fullscreen windows */
		return;
	/* Toggle the floating state of the selected client. While most windows can toggle freely
	 * between floating and tiled some windows that are fixed in size can not and these remain
	 * floating. A window is considered to be fixed in size if its size hints say that it has
	 * a minimum size and a maximum size that is equal to the minimum size. */
	selmon->sel->isfloating = !selmon->sel->isfloating || selmon->sel->isfixed;
	if (selmon->sel->isfloating)
		/* Here we have an explicit call to resize if the window has become floating. This
		 * call is only to allow the size hints for the window to be applied in the event
		 * that the resizehints setting has been set to 0, which makes dwm disrepect a
		 * client's size hints when tiled.
		 */
		resize(selmon->sel, selmon->sel->x, selmon->sel->y,
			selmon->sel->w, selmon->sel->h, 0);
	/* A final arrange call to reposition and resize tiled clients */
	arrange(selmon);
}

/* The toggletag function adds or removes tags in which a client window is to be shown on.
 *
 * This is referenced in the TAGKEYS macro which sets up keybindings for each individual tag.
 *
 * @called_from keypress in relation to keybindings
 * @called_from buttonpress in relation to button bindings
 * @calls focus to give input focus to the next in the stack if the client is no longer shown
 * @calls arrange as the client window may have been toggled away from the current monitor
 * @see TAGMASK macro
 *
 * Internal call stack:
 *    run -> keypress -> toggletag
 *    run -> buttonpress -> toggletag
 */
void
toggletag(const Arg *arg)
{
	unsigned int newtags;

	/* Bail if there are no visible clients */
	if (!selmon->sel)
		return;
	/* This sets the tag mask for the client.
	 *
	 * Let's say that the selected client is shown on the three tags 1, 5 and 6.
	 * The current tags bitmask of said client would then be:
	 *    000110001
	 *       ^^   ^
	 *    987654321
	 *
	 * Now let's say that the user hits the keybinding to toggle tag 5 for the client.
	 *
	 * The TAGKEYS macro passes an bit shifted unsigned int as the argument.
	 *
	 *    { MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} },
	 *
	 * In the keys array where the macro is used we can tell that for tag 5 the value of
	 * 1 is shifted leftwards 4 times.
	 *
	 *    	TAGKEYS(                        XK_5,                      4)
	 *
	 * 1 << 4 becomes 000010000 in binary.
	 *
	 * The ^ is a binary exclusive or (XOR) operator which copies the bit if it is set in one
	 * operand but not both.
	 *
	 * So the operation becomes:
	 *      000110001
	 *    ^ 000010000
	 *      ---------
	 *    = 000100001
	 *
	 * Meaning that the tags mask for the client now only holds tag 1 and 6.
	 *
	 * The arg->ui & TAGMASK is a safeguard that restricts the argument value to only hold as
	 * many bits as there are tags.
	 *
	 * As an example let's say that the user were to change the tags array to only hold four
	 * tags:
	 *    static const char *tags[] = { "1", "2", "3", "4" };
	 *
	 * but leave the TAGKEYS macros binding keys for up to tag 9. This would make it possible
	 * to tag a client to make it visible on tag 9 which does not exist, then untoggle the
	 * client from the current view.
	 *
	 * In this scenario the newtags variable would still have a value as the ninth bit is set
	 * to 1, which means that we would enter the if (newtags) { statement and set the client's
	 * tags to the new bitmask. The problem with this is that the client window would now be
	 * out of view and it would not be possible to bring that client window into view again.
	 *
	 * By capping the input argument to only allow bits for as many tags there are avoids
	 * problems like this.
	 *
	 * The TAGMASK macro is defined as:
	 *    #define TAGMASK                 ((1 << LENGTH(tags)) - 1)
	 */
	newtags = selmon->sel->tags ^ (arg->ui & TAGMASK);
	/* If the client is shown on at least one tag then allow the new tagset to be set. */
	if (newtags) {
		/* This sets the new tag mask for the selected client */
		selmon->sel->tags = newtags;
		/* It is possible that the client window disappeared from the current view in which
		 * case we should give focus to the next client in line. We also apply a full arrange
		 * in order to resize and reposition clients to fill the gap the client left behind.
		 *
		 * In the event that the client did not leave the current view the focus call will
		 * not make any difference, but the full arrange will make a call to restack which
		 * in turn calls drawbar so that you can see the tag indicators change. */
		focus(NULL);
		arrange(selmon);
	}
}

/* The toggleview function brings tags into or out of view.
 *
 * This is referenced in the TAGKEYS macro which sets up keybindings for each individual tag.
 *
 * @called_from keypress in relation to keybindings
 * @called_from buttonpress in relation to button bindings
 * @calls focus as the selected client may have been on a tag that was toggled away
 * @calls arrange as the client windows shown may have changed
 * @see TAGMASK macro
 *
 * Internal call stack:
 *    run -> keypress -> toggleview
 *    run -> buttonpress -> toggleview
 */
void
toggleview(const Arg *arg)
{
	/* This creates a new tagmask based on the selected monitor's selected tagset toggling
	 * the tagmask given as an argument. Refer to the writeup in the toggletag function should
	 * you need more information on how this works. */
	unsigned int newtagset = selmon->tagset[selmon->seltags] ^ (arg->ui & TAGMASK);
	/* This prevents the scenario of toggling away the last viewed tag. I.e. there must be at
	 * least one tag viewed. */
	if (newtagset) {
		/* This sets the new tag set for the selected monitor */
		selmon->tagset[selmon->seltags] = newtagset;
		/* The client that had focus may have been on a tag that was toggled away, so give
		 * input focus to the next client in the stack. */
		focus(NULL);
		/* A full arrange as the constellation of client windows viewed may have changed. */
		arrange(selmon);
	}
}

/* This removes focus for a given client.
 *
 * The setfocus argument will revert the input focus to the root window. This is typically
 * done when focus drifts from one monitor to another and is needed because there may not
 * be any client windows on the new monitor that will receive input focus. If the input
 * focus is not reverted to the root window in this scenario then it would be possible to
 * continue typing in the previous window despite having moved focus to another monitor.
 *
 * @called_from buttonpress when focus changes between monitors
 * @called_from enternotify when focus changes between monitors
 * @called_from motionnotify when focus changes between monitors
 * @called_from focusmon when focus changes between monitors
 * @called_from sendmon when focus changes between monitors
 * @called_from focus to unfocus the previously focused client
 * @called_from manage to unfocus the previously focused client
 * @calls grabbuttons to
 *
 * Internal call stack:
 *    ~ -> focus -> unfocus
 *    run -> buttonpress / enternotify -> unfocus
 *    run -> maprequest -> manage -> unfocus
 *    run -> keypress -> focusmon -> unfocus
 *    run -> keypress -> tagmon -> sendmon -> unfocus
 *    run -> buttonpress -> movemouse / resizemouse -> sendmon -> unfocus
 */
void
unfocus(Client *c, int setfocus)
{
	if (!c)
		return;
	/* Grab buttons as we are now interested in receiving any any mouse button click events
	 * for this window. */
	grabbuttons(c, 0);
	/* Revert the window border back to normal so that it doesn't appear like it is focused. */
	XSetWindowBorder(dpy, c->win, scheme[SchemeNorm][ColBorder].pixel);
	if (setfocus) {
		/* If focus is drifting from one monitor to another then we want to make sure that
		 * the previous window loses input focus by giving input focus back to the root
		 * window. */
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		/* We also drop the _NET_ACTIVE_WINDOW property from the root window to
		 * indicate that the window manager has no window in focus. */
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}
}

/* This function controls what happens when the window manager stops managing a window.
 *
 * @called_from cleanup to unmanage all client windows before exiting dwm
 * @called_from destroynotify to stop managing the window and remove the client
 * @called_from unmapnotify to stop managing the window and remove the client
 * @calls XGrabServer https://tronche.com/gui/x/xlib/window-and-session-manager/XGrabServer.html
 * @calls XSetErrorHandler https://tronche.com/gui/x/xlib/event-handling/protocol-errors/XSetErrorHandler.html
 * @calls XSelectInput https://tronche.com/gui/x/xlib/event-handling/XSelectInput.html
 * @calls XConfigureWindow https://tronche.com/gui/x/xlib/window/XConfigureWindow.html
 * @calls XUngrabButton https://tronche.com/gui/x/xlib/input/XUngrabButton.html
 * @calls XSync https://tronche.com/gui/x/xlib/event-handling/XSync.html
 * @calls detach to remove the client from the tile stack
 * @calls detachstack to remove the client from the stacking order
 * @calls setclientstate to set the client state to withdrawn state
 * @calls free to release memory used by the client struct
 * @calls updateclientlist to remove the window from the _NET_CLIENT_LIST property
 *
 * Internal call stack:
 *    run -> destroynotify / unmapnotify -> unmanage
 */
void
unmanage(Client *c, int destroyed)
{
	Monitor *m = c->mon;
	XWindowChanges wc;

	/* Remove the given client from both the client list and the stack order list. */
	detach(c);
	detachstack(c);
	/* If the window has already been destroyed then we don't have to take any further action
	 * with regards to the window itself. The function parameter destroyed will be true (1) if
	 * unmanage is called from the destroynotify function.
	 */
	if (!destroyed) {
		/* In principle this is intended to set the client's border width back to what it was
		 * before dwm started managing it. This can be deduced by that in the manage function
		 * we set c->oldbw to the border width of the original window attributes. There is no
		 * guarantee, however, that the c->oldbw will still hold this value as the
		 * setfullscreen function relies on the same variable to store the client's border
		 * width before going into fullscreen. */
		wc.border_width = c->oldbw;
		/* This disables processing of requests and close downs on all other connections than
		 * the one this request arrived on. */
		XGrabServer(dpy); /* avoid race conditions */
		/* Here we set the dummy X error handler just in case what we do next is going to
		 * generate an error that would otherwise cause dwm to exit. */
		XSetErrorHandler(xerrordummy);
		/* This call tells the X server that we are no longer interested in receiving events for
		 * the given window. If the window stops being managed by the window manager and is being
		 * managed by some other program, like tabbed for example, then we do not want the window
		 * manager interferring by reacting to FocusIn events for said window. */
		XSelectInput(dpy, c->win, NoEventMask);
		/* This is to clear / remove the border that is drawn around the window */
		XConfigureWindow(dpy, c->win, CWBorderWidth, &wc); /* restore border */
		/* Stop listening for any button press notification for this window */
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
		/* Change the client state to withdrawn (not shown) */
		setclientstate(c, WithdrawnState);
		/* This flushes the output buffer and then waits until all requests have been
		 * received and processed by the X server. */
		XSync(dpy, False);
		/* Revert to the normal X error handler */
		XSetErrorHandler(xerror);
		/* This restarts processing of requests and close downs on other connections */
		XUngrabServer(dpy);
	}
	/* Free memory consumed by the client structure */
	free(c);
	/* Focus on the next client in the stacking order */
	focus(NULL);
	/* As we have one less window being managed by the window manager we should update the
	 * _NET_CLIENT_LIST property of the root window. */
	updateclientlist();
	/* Finally an arrange call to allow the remaining tiled clients to take advantage of the
	 * space the unmanaged window left behind. */
	arrange(m);
}

/* This handles UnmapNotify events coming from the X server.
 *
 * This can happen if a client's window state goes from from mapped to unmapped.
 *
 * You can test this by finding the window ID of a given window using xwininfo (e.g. 0x5000002) or
 * using xdotool search (94371846) and using xdo or xdotool to map and unmap that window.
 *
 *    $ xdo hide 0x5a00006
 *    $ xdo show 0x5a00006
 *    $ xdotool windowunmap 94371846
 *    $ xdotool windowmap 94371846
 *
 * @called_from run (the event handler)
 * @calls wintoclient to find the client the given event is related to
 * @calls setclientstate to move the client to withdrawn state
 * @calls unmanage to stop managing the window and remove the client
 * @see https://tronche.com/gui/x/xlib/events/window-state-change/unmap.html
 * @see https://linux.die.net/man/3/xunmapevent
 * @see https://tronche.com/gui/x/xlib/event-handling/XSendEvent.html
 * @see https://tronche.com/gui/x/xlib/ICC/client-to-window-manager/XWithdrawWindow.html
 *
 * Internal call stack:
 *    run -> unmapnotify
 */
void
unmapnotify(XEvent *e)
{
	Client *c;
	XUnmapEvent *ev = &e->xunmap;

	/* Find the client the event is for. If the event is for a client window that is not managed
	 * by the window manager then we do nothing. */
	if ((c = wintoclient(ev->window))) {
		/* The ev->send_event indicates whether the request originates from an XSendEvent
		 * call or not. If that is the case then it means that the unmapping of the window
		 * was intentional (e.g. the user ran a command to do so or the application manages
		 * the window that way).
		 *
		 * As such if the unmapping was intentional then we set the window state to withdrawn
		 * which means that the window is simply not visible.
		 */
		if (ev->send_event)
			setclientstate(c, WithdrawnState);
		/* On the other hand if the unmapping was not intentional, not originating from an
		 * XSendEvent call, then that must mean that the unmap notification originates from
		 * the X server. As such we should stop managing this window and remove the client.
		 */
		else
			unmanage(c, 0);
	}
}

/* This is what creates the bar window for each monitor.
 *
 * @called_from setup to initialise the bars
 * @called_from configurenotify in the event that the monitors or screen changes
 * @calls XCreateWindow https://tronche.com/gui/x/xlib/window/XCreateWindow.html
 * @calls XDefineCursor https://tronche.com/gui/x/xlib/window/XDefineCursor.html
 * @calls XMapRaised https://tronche.com/gui/x/xlib/window/XMapRaised.html
 * @calls XSetClassHint https://tronche.com/gui/x/xlib/ICC/client-to-window-manager/XSetClassHint.html
 * @calls DefaultDepth https://linux.die.net/man/3/defaultdepth
 * @calls DefaultVisual https://linux.die.net/man/3/defaultvisual
 * @see https://tronche.com/gui/x/xlib/display/display-macros.html
 * @see https://tronche.com/gui/x/xlib/window/attributes/
 *
 * Internal call stack:
 *    main -> setup -> updatebars
 *    run -> configurenotify -> updatebars
 */
void
updatebars(void)
{
	Monitor *m;
	/* The window attributes for the bar. */
	XSetWindowAttributes wa = {
		/* We set override-redirect to true so that we won't ever end up in the situation of
		 * the bar being managed as a client in dwm. */
		.override_redirect = True,
		/* This makes it so that the background pixmap of the window's parent is used. */
		.background_pixmap = ParentRelative,
		/* This tells the X server that we are interested in receiving ButtonPress and Expose
		 * events in relation to this window. */
		.event_mask = ButtonPressMask|ExposureMask
	};
	/* The class hint for the bar window. This is set later with the XSetClassHint call. */
	XClassHint ch = {"dwm", "dwm"};
	/* Here we loop through each monitor */
	for (m = mons; m; m = m->next) {
		/* and if the monitor already have a bar window then we skip to the next. */
		if (m->barwin)
			continue;
		/* This creates the bar window with the width of the monitor and the defined bar
		 * height. The window is initially created at the top but the position will
		 * ultimately be determined by the updatebarpos function. */
		m->barwin = XCreateWindow(dpy, root, m->wx, m->by, m->ww, bh, 0, DefaultDepth(dpy, screen),
				CopyFromParent, DefaultVisual(dpy, screen),
				/* The below flags correspond to the attributes we set in the wa
				 * structure; in other words the flags tells the XCreateWindow
				 * function what fields we set values for in that structure. */
				CWOverrideRedirect|CWBackPixmap|CWEventMask, &wa);
		/* This defines that we use the normal cursor when we move the mouse pointer over
		 * the bar. */
		XDefineCursor(dpy, m->barwin, cursor[CurNormal]->cursor);
		/* This maps the bar window making it visible and also raised above other windows. */
		XMapRaised(dpy, m->barwin);
		/* This sets the class hint of the bar to "dwm", e.g.
		 *
		 *    $ xprop
		 *    WM_CLASS(STRING) = "dwm", "dwm"
		 *
		 * This allows for compositors as an example to make exceptions for the bar.
		 */
		XSetClassHint(dpy, m->barwin, &ch);
	}
}

/* This updates the bar position and sets the monitor's window area accordingly.
 *
 * @called_from togglebar to adjust the monitor's window area after showing or hiding the bar
 * @called_from updategeom to set the bar position and the monitor's window area for new monitors
 *
 * Internal call stack:
 *    run -> configurenotify -> updategeom -> updatebarpos
 *    main -> setup -> updategeom -> updatebarpos
 */
void
updatebarpos(Monitor *m)
{
	/* This sets the monitor window area to span the entire monitor. If the bar is shown then
	 * we adjust for that later.
	 *
	 * You may notice the omission of setting the window area width and x position here. This
	 * is because the bar is only at the top or at the bottom, hence the horizontal aspects
	 * of the window area is not influenced by the presence or non-presence of the bar.
	 */
	m->wy = m->my;
	m->wh = m->mh;
	if (m->showbar) {
		m->wh -= bh;
		m->by = m->topbar ? m->wy : m->wy + m->wh;
		m->wy = m->topbar ? m->wy + bh : m->wy;
	} else
		/* The bar is not shown and because we already set the window area to span the
		 * entire monitor all we need to do now is to set the bar position to a negative
		 * position so that it is not shown. By setting it to -bh the bar is just above
		 * the visible area of the monitor. */
		m->by = -bh;
}

/* This updates the _NET_CLIENT_LIST property on the root window.
 *
 * The _NET_CLIENT_LIST property is a list of window IDs that the window manager manages. When a
 * new window is managed then the window ID is appended to this list and this takes place in the
 * manage function.
 *
 * When windows are unmanaged (i.e. closed) then it is not straightforward to manipulate the list
 * in this property and the simplest solution is to just delete the list and generate a new one
 * based on all the remaining clients that are still managed.
 *
 * You can check this using the follwoing command.
 *
 *    $ xprop -root | grep _NET_CLIENT_LIST\(
 *    _NET_CLIENT_LIST(WINDOW): window id # 0x4e00002, 0x7000002, 0x6e00001, 0x6200002
 *
 * @called_from unmanage to remove unmanaged windows from _NET_CLIENT_LIST
 * @calls XDeleteProperty https://tronche.com/gui/x/xlib/window-information/XDeleteProperty.html
 * @calls XChangeProperty https://tronche.com/gui/x/xlib/window-information/XChangeProperty.html
 * @see manage for how it updates _NET_CLIENT_LIST for new managed windows
 * @see https://specifications.freedesktop.org/wm-spec/1.3/ar01s03.html
 *
 * Internal call stack:
 *    run -> destroynotify / unmapnotify -> unmanage -> updateclientlist
 *    main -> cleanup -> unmanage -> updateclientlist
 */
void
updateclientlist(void)
{
	Client *c;
	Monitor *m;

	/* Delete the existing client list */
	XDeleteProperty(dpy, root, netatom[NetClientList]);
	/* Loop through each monitor */
	for (m = mons; m; m = m->next)
		/* and loop through each client for each monitor */
		for (c = m->clients; c; c = c->next)
			/* then add (append) the window ID of each client to the client list */
			XChangeProperty(dpy, root, netatom[NetClientList],
				XA_WINDOW, 32, PropModeAppend,
				(unsigned char *) &(c->win), 1);
}

/* This sets up monitors if the window manager is compiled with the Xinerama library. If dwm is
 * compiled without the Xinerama library, or in the event that Xinerama is not active for the
 * screen, then the available screen space is set up as a single workspace that spans all monitors.
 *
 * @called_from setup to set up monitor(s) on startup
 * @called_from configurenotify if monitor(s) or screen change during runtime
 * @calls XineramaIsActive https://linux.die.net/man/3/xineramaisactive
 * @calls XineramaQueryScreens https://linux.die.net/man/3/xineramaqueryscreens
 * @calls XFree https://tronche.com/gui/x/xlib/display/XFree.html
 * @calls ecalloc to allocate space to hold unique screen info (see util.c)
 * @calls memcpy to copy screen info
 * @calls isuniquegeom to check for duplicate screen info
 * @calls createmon to set up new monitors
 * @calls updatebarpos to set the bar position and window area accordingly
 * @calls detachstack to relocate clients in the event of less monitors
 * @calls attachstack to relocate clients in the event of less monitors
 * @calls attach to relocate clients in the event of less monitors
 * @calls cleanupmon in the event that we have less monitors
 * @calls free to release resources after parsing Xinerama screens
 * @calls wintomon to find the monitor the mouse cursor resides on
 *
 * Internal call stack:
 *    run -> configurenotify -> updategeom
 *    main -> setup -> updategeom
 */
int
updategeom(void)
{
	int dirty = 0;

#ifdef XINERAMA
	/* This checks if Xinerama is active on the screen (we would expect this to be true). */
	if (XineramaIsActive(dpy)) {
		int i, j, n, nn;
		Client *c;
		Monitor *m;
		/* The XineramaQueryScreens function returns info about each individual output device
		 * within the Xinerama Screen. The variable nn here refers to the number of entries
		 * the info list. This list can in principle contain duplicate geometries so we are
		 * going to de-duplicate it by checking for unique geometries. */
		XineramaScreenInfo *info = XineramaQueryScreens(dpy, &nn);
		XineramaScreenInfo *unique = NULL;

		/* This loops through all monitors in order to find the current monitor count (n). */
		for (n = 0, m = mons; m; m = m->next, n++);
		/* Only consider unique geometries as separate screens. Here we allocate space to
		 * hold up to nn unique XineramaScreenInfo entries. */
		unique = ecalloc(nn, sizeof(XineramaScreenInfo));
		/* We then loop through each of the nn elements of the info array provided by
		 * XineramaQueryScreens and verify that this geometry has not been seen before.
		 * If it has not then we copy that element into the list of unique geometries,
		 * otherwise if the screen info is not unique then it is skipped. */
		for (i = 0, j = 0; i < nn; i++)
			if (isuniquegeom(unique, j, &info[i]))
				memcpy(&unique[j++], &info[i], sizeof(XineramaScreenInfo));
		/* Data returned by XineramaQueryScreens must be freed. */
		XFree(info);
		nn = j;

		/* New monitors if nn > n. If this is the first time this function is run (via setup)
		 * then the number of exiting monitors (n) will be 0, but otherwise we would only
		 * process this for-loop if we have new monitors to add. */
		for (i = n; i < nn; i++) {
			/* This sets m to the last monitor. */
			for (m = mons; m && m->next; m = m->next);
			/* If we have a last monitor then just create a new monitor after that. */
			if (m)
				m->next = createmon();
			/* Otherwise this is the first monitor being created, so we set mons which
			 * refers to the first monitor in the (linked) list of monitors. */
			else
				mons = createmon();
		}
		/* Now we need to loop through each monitor and update the monitor coordinates as
		 * well as the size of each monitor. */
		for (i = 0, m = mons; i < nn && m; m = m->next, i++)
			/* This says that we only enter the if statement if we are looping through
			 * new monitors (i >= n), or if the position or size has changed. It is here
			 * to avoid needlessly marking dirty = 1 when the geometries has not changed
			 * for a monitor. */
			if (i >= n
			|| unique[i].x_org != m->mx || unique[i].y_org != m->my
			|| unique[i].width != m->mw || unique[i].height != m->mh)
			{
				dirty = 1;
				/* This sets the monitor number, or index if you wish. */
				m->num = i;
				/* This sets the monior position and size as well as the window area
				 * to the position and dimensions provided by Xinerama. */
				m->mx = m->wx = unique[i].x_org;
				m->my = m->wy = unique[i].y_org;
				m->mw = m->ww = unique[i].width;
				m->mh = m->wh = unique[i].height;
				/* Update the bar position and the window area accordingly. */
				updatebarpos(m);
			}
		/* Removed monitors if n > nn. If this is the first time this function is run (via
		 * setup) then we will not enter this for loop. In the event that a monitor has been
		 * removed then we need to move all the clients on that monitor to one that is still
		 * visible. For simplicity clients are moved to the first (often primary) monitor. */
		for (i = nn; i < n; i++) {
			/* This finds the last monitor (m). */
			for (m = mons; m && m->next; m = m->next);
			/* Then we repeat the below for all clients on monitor m, setting c to the
			 * first client in the list. */
			while ((c = m->clients)) {
				dirty = 1;
				/* This takes the current client out of the client list, setting the
				 * next in line to become the new head of the list. This is also why
				 * there is no separate detach(c) call here. */
				m->clients = c->next;
				/* Remove the client from the stacking order before changing the
				 * monitor. */
				detachstack(c);
				/* Setting the client's monitor to be the first monitor before calling
				 * attach and attachstack there. */
				c->mon = mons;
				attach(c);
				attachstack(c);
			}
			/* If the monitor being removed was the selected monitor, then make the first
			 * monitor the selected one. */
			if (m == selmon)
				selmon = mons;
			/* Call cleanupmon to free up resources, remove the bar window, etc. */
			cleanupmon(m);
		}
		/* Free the resources we used to create our unique XineramaScreenInfo array. */
		free(unique);
	} else
#endif /* XINERAMA */
	/* In the event that dwm is compiled without the Xinerama library, or XineramaIsActive
	 * indicates that Xinerama is not active for the screen, then we fall back to setting up
	 * a single workspace (monitor) that spans all screens. */
	{ /* default monitor setup */
		/* If a single monitor does not exist, then create it. */
		if (!mons)
			mons = createmon();
		/* Enter the if statement if the screen width or height has changed. */
		if (mons->mw != sw || mons->mh != sh) {
			dirty = 1;
			/* This sets the size of the monitor. The position is not set and will default
			 * to 0x0. */
			mons->mw = mons->ww = sw;
			mons->mh = mons->wh = sh;
			/* Update the bar position and the window area according to the new size. */
			updatebarpos(mons);
		}
	}
	/* If we have made any change in terms of monitors, position or sizes then the dirty flag
	 * will have been set. In this case we revert the selected monitor back to the first
	 * monitor, then we try to set the selected monitor based on the location of the mouse
	 * pointer by calling wintomon. In practice the setting of selmon to mons prior to calling
	 * wintomon is only on the off chance that we should fail to query for the mouse pointer,
	 * in which case wintomon would eventually return selmon. */
	if (dirty) {
		selmon = mons;
		selmon = wintomon(root);
	}
	/* Return the dirty flag to indicate whether this call to updategeom resulted in any change
	 * to monitor setup. This return value is used in the configurenotify function. */
	return dirty;
}

/* This function sets or updates the internal Num Lock mask variable.
 *
 * As per the tronche documentation we have that:
 *    The operation of keypad keys is controlled by the KeySym named XK_Num_Lock, by attaching
 *    that KeySym to some KeyCode and attaching that KeyCode to any one of the modifiers Mod1
 *    through Mod5. This modifier is called the numlock modifier.
 *
 * What this boils down to is that it is not straightforward to reference the Num Lock mask
 * compared to, say, ControlMask or ShiftMask.
 *
 * To work out what the Num Lock mask is we need to loop through all the modifier mappings to find
 * the one that refers to the Num Lock button, and that is exactly what this function does.
 *
 * Once this is found the modifier is stored in the global and static numlockmask variable. This
 * is later used in the context of handling key and button presses.
 *
 * @called_from grabbuttons to make sure the numlock modifier is correct before grabbing buttons
 * @called_from grabkeys to make sure the numlock modifier is correct before grabbing keys
 * @calls XGetModifierMapping https://tronche.com/gui/x/xlib/input/XGetModifierMapping.html
 * @calls XKeysymToKeycode https://tronche.com/gui/x/xlib/utilities/keyboard/XKeysymToKeycode.html
 * @calls XFreeModifiermap https://tronche.com/gui/x/xlib/input/XFreeModifiermap.html
 * @see https://tronche.com/gui/x/xlib/utilities/keyboard/
 * @see https://tronche.com/gui/x/xlib/input/keyboard-encoding.html#XModifierKeymap
 *
 * Internal call stack:
 *    ~ -> focus / unfocus -> grabbuttons -> updatenumlockmask
 *    run -> maprequest -> manage -> grabbuttons -> updatenumlockmask
 *    run -> mappingnotify -> grabkeys -> updatenumlockmask
 *    main -> setup -> grabkeys -> updatenumlockmask
 */
void
updatenumlockmask(void)
{
	unsigned int i, j;
	XModifierKeymap *modmap;

	/* Clear the num lock mask variable to cover for the edge case where the modifier is
	 * disabled and there is no Num Lock key to be found. */
	numlockmask = 0;
	/* This retrieves a new modifier mapping structure that contains the keys being used as
	 * modifiers. */
	modmap = XGetModifierMapping(dpy);
	/* We loop through each modifier */
	for (i = 0; i < 8; i++)
		/* and we loop through each key per modifier */
		for (j = 0; j < modmap->max_keypermod; j++)
			/* If the modifier key is the Num Lock key then store the bitwise mask of
			 * the modifier in the global numlockmask variable. */
			if (modmap->modifiermap[i * modmap->max_keypermod + j]
				== XKeysymToKeycode(dpy, XK_Num_Lock))
				numlockmask = (1 << i);
	/* The caller of XGetModifierMapping is responsible for freeing the memory used by the
	 * returned modifier key map. */
	XFreeModifiermap(modmap);
}

/* This updates the size hints for a client window.
 *
 * Size hints are a way for an application to tell what kind of sizes are appropriate for the given
 * window.
 *
 * Example restrictions that might apply for a window:
 *    - it could have a minimum size
 *    - it could have a maximum size
 *    - it might require a fixed aspect ratio
 *    - it might require incremental size changes
 *
 * The function sets a series of variables that are later used when calculating the appropriate
 * size of a window when size hints are respected.
 *
 * @called_by applysizehints
 * @calls XGetWMNormalHints https://tronche.com/gui/x/xlib/ICC/client-to-window-manager/XGetWMNormalHints.html
 * @see https://tronche.com/gui/x/icccm/sec-4.html#s-4.1.2.3
 *
 * Internal call stack:
 *    run -> maprequest -> manage -> updatesizehints
 *    ~ -> resize -> applysizehints -> updatesizehints
 */
void
updatesizehints(Client *c)
{
	long msize;
	XSizeHints size;

	/* If we fail to read the normal window management hints then we set size.flags to PSize
	 * so that it has a value and that we go through all of the remaining if statements below
	 * to set the default values. */
	if (!XGetWMNormalHints(dpy, c->win, &size, &msize))
		/* size is uninitialized, ensure that size.flags aren't used */
		size.flags = PSize;

	/* The base size defines the desired size of the window. */
	if (size.flags & PBaseSize) {
		c->basew = size.base_width;
		c->baseh = size.base_height;
	/* If the hints do not contain any base size flag then we fall back to setting the minimum
	 * size as the base size. */
	} else if (size.flags & PMinSize) {
		c->basew = size.min_width;
		c->baseh = size.min_height;
	/* If the hints do not contain any minimum size either then we just set the base height and
	 * width to 0. */
	} else
		c->basew = c->baseh = 0;

	/* This sets the resize increment hint, e.g. the window can only be resized horizontally
	 * with an increment of 24 pixels. */
	if (size.flags & PResizeInc) {
		c->incw = size.width_inc;
		c->inch = size.height_inc;
	/* If there are no resize increment hints then set these to 0 (no restriction). */
	} else
		c->incw = c->inch = 0;

	/* This sets the maximum size of the window. */
	if (size.flags & PMaxSize) {
		c->maxw = size.max_width;
		c->maxh = size.max_height;
	/* If there is no maximum size then we set these to 0 (no restriction). */
	} else
		c->maxw = c->maxh = 0;

	/* This sets the minimum size of the window. */
	if (size.flags & PMinSize) {
		c->minw = size.min_width;
		c->minh = size.min_height;
	/* If there is no minimum size restriction then fall back to using the base size as the
	 * minimum size. */
	} else if (size.flags & PBaseSize) {
		c->minw = size.base_width;
		c->minh = size.base_height;
	/* If there is no base size either then set the minimum size to 0 (no restriction). */
	} else
		c->minw = c->minh = 0;

	/* This sets the aspect ratio restrictions for the window. */
	if (size.flags & PAspect) {
		c->mina = (float)size.min_aspect.y / size.min_aspect.x;
		c->maxa = (float)size.max_aspect.x / size.max_aspect.y;
	/* If there is no aspect ratio hint then we set this to 0 (no restriction). */
	} else
		c->maxa = c->mina = 0.0;

	/* Here we determine if the client window is fixed in size. This is an edge case where the
	 * size hints for the window say that the minimum size is the same as the maximum size,
	 * which means that the window can't be resized. A window that can't be resized is kind of
	 * unfortunate for a tiling window manager, hence windows that are marked as fixed are
	 * always going to be floating. */
	c->isfixed = (c->maxw && c->maxh && c->maxw == c->minw && c->maxh == c->minh);
	/* One may think of hintsvalid as a flag that indicates whether size hints need to be
	 * updated or not. Here we set that to 1 to indicate that size hints are now up to date. */
	c->hintsvalid = 1;
}

/* This updates the status text by reading the WM_NAME property of the root window.
 *
 * One can test this by running xsetroot like this:
 *    $ xsetroot -name "status text"
 *
 * @called_from setup to initialise stext and trigger the initial drawing of the bar
 * @called_from propertynotify whenever the WM_NAME property of the root window changes
 * @calls gettextprop to read the WM_NAME text property of the root name
 * @calls strcpy to set the default status text to "dwm-6.3"
 * @calls drawbar to update the bar as the status text has changed
 * @see https://dwm.suckless.org/status_monitor/
 *
 * Internal call stack:
 *    run -> setup -> updatestatus
 *    run -> propertynotify -> updatestatus
 */
void
updatestatus(void)
{
	/* This retrieves the text property of WM_NAME from the root window and stores that in the
	 * status text (stext) variable which is later used when drawing the bar. */
	if (!gettextprop(root, XA_WM_NAME, stext, sizeof(stext)))
		strcpy(stext, "dwm-"VERSION);
	/* Update the bar as the status text has changed */
	drawbar(selmon);
}

/* This updates the window title for the client.
 *
 * This is used for showing the window title in the bar and when trying to match client rules when
 * the windows is first managed.
 *
 * @called_from manage to set the window title when the client is first managed
 * @called_from propertynotify when WM_NAME or _NET_WM_NAME events are received
 * @calls gettextprop to get the text property for the client window
 * @calls strcpy to mark clients that have no name as broken
 *
 * Internal call stack:
 *    run -> maprequest -> manage -> updatetitle
 *    run -> propertynotify -> updatetitle
 */
void
updatetitle(Client *c)
{
	/* Get the text property of a given client's window.
	 *
	 * The window title can be seen using xprop and clicking on the client window, e.g.
	 *
	 *    $ xprop | grep _NET_WM_NAME
	 *    _NET_WM_NAME(UTF8_STRING) = "~"
	 *
	 * One can set a new title for a window using xdotool, but note that many applications
	 * tend to manage the window title on their own and as such may overwrite what you set
	 * using external tools like this.
	 *
	 *    xdotool selectwindow set_window --name "new title"
	 */
	if (!gettextprop(c->win, netatom[NetWMName], c->name, sizeof c->name))
		/* Fall back to checking WM_NAME if the window does not have a _NET_WM_NAME
		 * property. */
		gettextprop(c->win, XA_WM_NAME, c->name, sizeof c->name);
	/* Some windows do not have a window title set, in which case we fall back to using the
	 * text "broken" to indicate this. It is better than displaying nothing in the window
	 * title.
	 *
	 * The text is defined in the global variable named broken:
	 *    static const char broken[] = "broken";
	 *
	 * The strcpy call copies all bytes (characters) from the broken array into the client
	 * name variable.
	 */
	if (c->name[0] == '\0') /* hack to mark broken clients */
		strcpy(c->name, broken);
}

/* This reads window properties to update the window type.
 *
 * In practice this just checks whether the window is in fullscreen and whether it is a dialog box
 * in which case the window becomes floating.
 *
 * @called_from propertynotify when notified of changes to _NET_WM_WINDOW_TYPE
 * @called_from manage to make dialog windows floating by default
 * @calls getatomprop to get the value of _NET_WM_STATE and _NET_WM_WINDOW_TYPE
 * @calls setfullscreen in the event that the window is in fullscreen state
 *
 * Internal call stack:
 *    run -> propertynotify -> updatewindowtype
 *    run -> maprequest -> manage -> updatewindowtype
 */
void
updatewindowtype(Client *c)
{
	/* This reads the property value of _NET_WM_STATE (if present), e.g.
	 *
	 *    $ xprop | grep _NET_WM_STATE
	 *    _NET_WM_STATE(ATOM) = _NET_WM_STATE_FULLSCREEN
	 */
	Atom state = getatomprop(c, netatom[NetWMState]);

	/* This reads the property value of _NET_WM_WINDOW_TYPE, e.g.
	 *
	 *    $ xprop | grep _NET_WM_WINDOW_TYPE
	 *    _NET_WM_WINDOW_TYPE(ATOM) = _NET_WM_WINDOW_TYPE_DIALOG
	 */
	Atom wtype = getatomprop(c, netatom[NetWMWindowType]);

	/* If the WM state property indicates that we are in fullscreen then we make a call to
	 * setfullscreen to make the window fullscreen within dwm as well. */
	if (state == netatom[NetWMFullscreen])
		setfullscreen(c, 1);
	/* If the window type suggests a dialog box then we make that window floating. */
	if (wtype == netatom[NetWMWindowTypeDialog])
		c->isfloating = 1;
}

/* This reads window management hints for a given client window.
 *
 * @called_by propertynotify when notified of changes to the WM_HINTS property
 * @called_by manage to read window management hints for new clients
 * @calls XGetWMHints https://tronche.com/gui/x/xlib/ICC/client-to-window-manager/XGetWMHints.html
 * @calls XSetWMHints https://tronche.com/gui/x/xlib/ICC/client-to-window-manager/XSetWMHints.html
 * @calls XFree https://tronche.com/gui/x/xlib/display/XFree.html
 * @see https://tronche.com/gui/x/xlib/ICC/client-to-window-manager/wm-hints.html
 * @see seturgent which also manpulates these hints
 *
 * Internal call stack:
 *    run -> propertynotify -> updatewmhints
 *    run -> maprequest -> manage -> updatewmhints
 */
void
updatewmhints(Client *c)
{
	XWMHints *wmh;

	/* This call reads the window management hints for the client's window. */
	if ((wmh = XGetWMHints(dpy, c->win))) {
		/* If the hints could be read then check if the urgency hint is present. If it is and
		 * the given client is the selected client on the monitor then clear that hint. */
		if (c == selmon->sel && wmh->flags & XUrgencyHint) {
			/* The below binary operation says keep all flags except XUrgencyHint. */
			wmh->flags &= ~XUrgencyHint;
			/* This updates / overwrites the window management hints for the window. */
			XSetWMHints(dpy, c->win, wmh);
		} else
			/* Here we set c->isurgent depending on whether the urgency hint has been set
			 * or not. */
			c->isurgent = (wmh->flags & XUrgencyHint) ? 1 : 0;
		/* The second flag that is checked is whether there is any input hint present. */
		if (wmh->flags & InputHint)
			/* If the input hint is present then wmh->input indicates whether or not the
			 * application relies on the window manager to get keyboard input. If it does
			 * not then that means that the window should not get input focus. A good
			 * example of this is the xclock program which provides this hint as it is
			 * just an X window showing a clock and it does not handle keyboard input. */
			c->neverfocus = !wmh->input;
		else
			c->neverfocus = 0;
		/* We need to free the XWMHints structure returned by XGetWMHints. */
		XFree(wmh);

		/* NB: there are other hint flags in the structure but most of these are related to
		 * the window icon and the rest are simply unused by dwm. */
	}
}

/* The view function changes the view to a given bitmask.
 *
 * By default the view function is used to view individual tags, but as the argument is a bitmask
 * it is perfectly possible to set up keybindings to view predefined sets of tags.
 *
 * This is referenced in the TAGKEYS macro which sets up keybindings for each individual tag.
 *
 * @called_from keypress in relation to keybindings
 * @called_from buttonpress in relation to button bindings
 * @called_from cleanup to bring all client windows into view before exiting
 * @calls focus to give input focus to the last viewed client on the viewed tag
 * @calls arrange as the client windows shown may have changed
 * @see TAGMASK macro
 *
 * Internal call stack:
 *    run -> keypress -> view
 *    run -> buttonpress -> view
 *    main -> cleanup -> view
 */
void
view(const Arg *arg)
{
	/* If the given bitmask is the same as what is currently shown then do nothing. This makes
	 * it so that if you are on tag 7 and you hit MOD+7 then nothing happens. */
	if ((arg->ui & TAGMASK) == selmon->tagset[selmon->seltags])
		return;
	/* This toggles between the previous and current tagset. */
	selmon->seltags ^= 1; /* toggle sel tagset */
	/* This sets the new tagset, unless the given unsigned int argument is 0. This has
	 * specifically to do with the MOD+Tab keybinding that passes 0 as the bitmask to toggle
	 * between the current and previous tagset.
	 *
	 *    { MODKEY,                       XK_Tab,    view,           {0} },
	 */
	if (arg->ui & TAGMASK)
		selmon->tagset[selmon->seltags] = arg->ui & TAGMASK;
	/* Focus on the first visible client in the stack as the view has changed */
	focus(NULL);
	/* Finally a full arrange call to hide clients that are not shown and to bring into view
	 * the clients that are, to tile them and to update the bar. */
	arrange(selmon);
}

/* Internal function to search for a client that is associated with a given window.
 *
 * This is called from most event handling functions to translate window IDs to client references.
 *
 * @called_from buttonpress to check whether a mouse button click was on a client window
 * @called_from clientmessage to find the client the received message event is for
 * @called_from configurerequest to find the client the received request event is for
 * @called_from destroynotify to find the client the destroy notification is for
 * @called_from enternotify to find the client the enter notification is for
 * @called_from manage to search for the parent window of transient windows
 * @called_from maprequest to sanity check that the window is not already managed by the WM
 * @called_from propertynotify to find the client the property notification is for
 * @called_from unmapnotify to find the client the unmap notification is for
 * @called_from wintomon to find the monitor the given window is on (via the client)
 *
 * Internal call stack:
 *    run -> maprequest -> manage -> wintoclient
 *    run -> buttonpress / enternotify / expose -> wintomon -> wintoclient
 *    run -> buttonpress / clientmessage / configurerequest / maprequest -> wintoclient
 *    run -> destroynotify / enternotify / propertynotify / unmapnotify -> wintoclient
 *    run -> configurenotify -> updategeom -> wintomon -> wintoclient
 *    main -> setup -> updategeom -> wintomon -> wintoclient
 */
Client *
wintoclient(Window w)
{
	Client *c;
	Monitor *m;

	/* Loop through all monitors */
	for (m = mons; m; m = m->next)
		/* Loop through all clients for each monitor */
		for (c = m->clients; c; c = c->next)
			/* If we find the client associated with the given window then return it */
			if (c->win == w)
				return c;
	/* Return NULL to indicate that we did not find a client related to the given window. This
	 * means that the window is not managed by the window manager. */
	return NULL;
}

/* Internal function to find the monitor a given window is on.
 *
 * @called_from buttonpress to work out which monitor a button press was made on
 * @called_from enternotify to work out what monitor the enter notification is for
 * @called_from expose to work out what monitor the expose event was is
 * @called_from updategeom to find the monitor the mouse cursor resides on
 * @calls getrooptr to find the mouse pointer coordinates if window is the root window
 * @calls recttomon to find the monitor the mouse pointer is on
 * @calls wintoclient to find the client related to the given window
 *
 * Internal call stack:
 *    run -> buttonpress / enternotify / expose -> wintomon
 *    run -> configurenotify -> updategeom -> wintomon
 *    main -> setup -> updategeom -> wintomon
 */
Monitor *
wintomon(Window w)
{
	int x, y;
	Client *c;
	Monitor *m;

	/* If the given window is the root window then retrieve the mouse coordinates and pass
	 * these on to recttomon to work out what monitor the mouse pointer is on. */
	if (w == root && getrootptr(&x, &y))
		return recttomon(x, y, 1, 1);
	/* Loop through all monitors */
	for (m = mons; m; m = m->next)
		/* Check if the given window is the bar window, if so then we know what monitor that
		 * window resides on. */
		if (w == m->barwin)
			return m;
	/* Check if the given window is one of the managed clients, and if so then return the
	 * monitor that client is assigned to. */
	if ((c = wintoclient(w)))
		return c->mon;
	/* If we come here then we are at a loss; the given window is not known to us and it is
	 * not managed by the window manager. Fall back to just returning the currently selected
	 * monitor.
	 */
	return selmon;
}

/* There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's). Other types of errors call Xlibs
 * default error handler, which may call exit. */
int
xerror(Display *dpy, XErrorEvent *ee)
{
	if (ee->error_code == BadWindow
	|| (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch)
	|| (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolyFillRectangle && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolySegment && ee->error_code == BadDrawable)
	|| (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch)
	|| (ee->request_code == X_GrabButton && ee->error_code == BadAccess)
	|| (ee->request_code == X_GrabKey && ee->error_code == BadAccess)
	|| (ee->request_code == X_CopyArea && ee->error_code == BadDrawable))
		return 0;
	fprintf(stderr, "dwm: fatal error: request code=%d, error code=%d\n",
		ee->request_code, ee->error_code);
	return xerrorxlib(dpy, ee); /* may call exit */
}

/* A dummy X error handler that just ignores any X errors.
 *
 * @see killclient that sets this function as the error handler when terminating windows
 */
int
xerrordummy(Display *dpy, XErrorEvent *ee)
{
	return 0;
}

/* Startup Error handler to check if another window manager is already running.
 *
 * @calls die to print errors and exit dwm (see util.c)
 * @see checkotherwm which sets this function as the X error handler
 */
int
xerrorstart(Display *dpy, XErrorEvent *ee)
{
	die("dwm: another window manager is already running");
	return -1;
}

/* User function to move the selected client to become the new master client. If the selected
 * client is the master client then the master and the next tiled window will swap places.
 *
 * @called_from keypress in relation to keybindings
 * @calls nexttiled to check if the selected client is the master client
 * @calls pop to move the client to the top of the tile stack, making it the new master window
 *
 * Internal call stack:
 *    run -> keypress -> zoom
 */
void
zoom(const Arg *arg)
{
	Client *c = selmon->sel;

	/* If the selected layout for the selected monitor is floating layout (as indicated by
	 * having a NULL arrange function as defined in the layouts array), if there is no selected
	 * client or if the selected client is floating, then we do nothing. */
	if (!selmon->lt[selmon->sellt]->arrange || !c || c->isfloating)
		return;

	/* This one statment combines different pieces of logic:
	 *    - it first checks whether the selected client is the master client (first tiled client)
	 *    - if it is not then we exit the if statement and we stick with the selected client to
	 *      pass it to pop
	 *    - if it is then we select the next tiled client after that, which will be the second
	 *      tiled client
	 *    - if such a client does not exist (as in there is only one tiled client) then we return
	 *    - otherwise we keep that client to pass to pop
	 */
	if (c == nexttiled(selmon->clients) && !(c = nexttiled(c->next)))
		return;

	/* Call pop to move the client to the top of the tile stack so that it becomes the new
	 * master client. */
	pop(c);
}

/* As per the header comment of this file we have that:
 *
 *    To understand everything else, start reading main().
 *
 * The reason for this is simply that main is the first function that is executed when dwm starts:
 *    - it sets everything up and
 *    - it makes dwm enter the event loop by calling the run function and
 *    - it triggers cleanup of resources when the window manager is exiting
 *
 * @calls XOpenDisplay https://tronche.com/gui/x/xlib/display/opening.html
 * @calls XCloseDisplay https://tronche.com/gui/x/xlib/display/XCloseDisplay.html
 * @calls die in the event of unexpected arguments
 * @calls setlocale to try and set the locale for locale-sensitive C library functions
 * @calls XSupportsLocale to check if locale is supported in the event that setlocale fails
 * @calls checkotherwm to verify that no other window manager is running
 * @calls setup to initialise the screen and everything else needed for operational purposes
 * @calls pledge https://man.openbsd.org/pledge.2
 * @calls scan to look for existing X windows that can be managed
 * @calls run to enter the event loop
 * @calls cleanup to free up resources when the window manager is exiting
 */
int
main(int argc, char *argv[])
{
	/* The window manager only supports a single command line option and that option is the -v
	 * flag to output the version of dwm. This first if statement checks if there is an argument
	 * and if that argument is -v then it prints the version of dwm and exits. The argument
	 * count is 2 in this case because the first argument is the name of the executable. */
	if (argc == 2 && !strcmp("-v", argv[1]))
		die("dwm-"VERSION);
	/* Bail outputting a usage string if there are any arguments other than -v. */
	else if (argc != 1)
		die("usage: dwm [-v]");
	/* Some library functions may be locale sensitive and the below tries to set the locale for
	 * such cases. The LC_CTYPE category determines how single-byte vs multi-byte characters are
	 * handled when it comes to text, what classifies as alpha, digits, etc. and also how
	 * such character classes behave. */
	if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		fputs("warning: no locale support\n", stderr);
	/* The XOpenDisplay function returns a Display structure that serves as the connection to
	 * the X server and that contains all the information about that X server. This is to be
	 * used in every subsequent xlib call we make. */
	if (!(dpy = XOpenDisplay(NULL)))
		die("dwm: cannot open display");
	/* Before proceeding we need to sanity check that no other window manager is running because
	 * the X server would not allow two running side by side. The reason for that has to do with
	 * how events are propagated when windows appear and disappear. The checkotherwm call will
	 * result in dwm exiting with an error if another window manager is detected. */
	checkotherwm();
	/* If we have come so far then we are good to start up. The setup call will initialise
	 * everything else that is needed for operational purposes. This involves setting up
	 * monitors, loading fonts, creating colours for the colour schemes, creating cursors,
	 * creating the bars, setting window manager hints and most importantly telling the X server
	 * what kind of events the window manager is listening for. */
	setup();
#ifdef __OpenBSD__
	/* In BSD systems the pledge call lists a series of "promises" that the program swears to
	 * live by. The pledge restricts what the program can do. In practice the below states
	 * that the program needs to be able to do the following to operate:
	 *    stdio - use a series of standard in/out functions
	 *    rpath - use system calls that have read-only effects on the file system
	 *    proc - allows for process relationship operations like fork, setsid, etc.
	 *    exec - allows a process to call execve
	 *
	 * Refer to the pledge page for more details on these. If the program were to break these
	 * promises by, for example, using one of the functions under wpath that can result in write
	 * effects on the file system then the process will be killed.
	 */
	if (pledge("stdio rpath proc exec", NULL) == -1)
		die("pledge");
#endif /* __OpenBSD__ */
	/* The scan function will search for existing X windows that can be managed by the window
	 * manager and it will pass those windows on to the manage function. */
	scan();
	/* The call to run will make dwm enter the event loop and it will remain there until the
	 * window manager is ready to exit. */
	run();
	/* When exiting dwm the event loop is stopped and the call to run returns. Now we proceed
	 * with cleaning up and freeing all the resources we initialised in the setup function. */
	cleanup();
	/* Finally we close the connection to the X server before we end the process. */
	XCloseDisplay(dpy);
	return EXIT_SUCCESS;
}
