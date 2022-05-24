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
 * To understand everything else, start reading main().
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
#define BUTTONMASK              (ButtonPressMask|ButtonReleaseMask)
/* The CLEANMASK macro removes Num Lock mask and Lock mask from a given bit mask.
 * Refer to the numlockmask variable comment for more details on the Num Lock mask.
 * The CLEANMASK macro is used in the keypress and buttonpress functions. */
#define CLEANMASK(mask)         (mask & ~(numlockmask|LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
/* Calculates how much a monitor's window area intersects with a given size and position.
 * See the writeup in the recttomon function for more information on this. */
#define INTERSECT(x,y,w,h,m)    (MAX(0, MIN((x)+(w),(m)->wx+(m)->ww) - MAX((x),(m)->wx)) \
                               * MAX(0, MIN((y)+(h),(m)->wy+(m)->wh) - MAX((y),(m)->wy)))
#define ISVISIBLE(C)            ((C->tags & C->mon->tagset[C->mon->seltags]))
#define LENGTH(X)               (sizeof X / sizeof X[0])
#define MOUSEMASK               (BUTTONMASK|PointerMotionMask)
#define WIDTH(X)                ((X)->w + 2 * (X)->bw)
#define HEIGHT(X)               ((X)->h + 2 * (X)->bw)
#define TAGMASK                 ((1 << LENGTH(tags)) - 1)
#define TEXTW(X)                (drw_fontset_getwidth(drw, (X)) + lrpad)

/* enums */
enum { CurNormal, CurResize, CurMove, CurLast }; /* cursor */
enum { SchemeNorm, SchemeSel }; /* color schemes */
enum { NetSupported, NetWMName, NetWMState, NetWMCheck,
       NetWMFullscreen, NetActiveWindow, NetWMWindowType,
       NetWMWindowTypeDialog, NetClientList, NetLast }; /* EWMH atoms */
enum { WMProtocols, WMDelete, WMState, WMTakeFocus, WMLast }; /* default atoms */
enum { ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle,
       ClkClientWin, ClkRootWin, ClkLast }; /* clicks */

typedef union {
	int i;
	unsigned int ui;
	float f;
	const void *v;
} Arg;

typedef struct {
	unsigned int click;
	unsigned int mask;
	unsigned int button;
	void (*func)(const Arg *arg);
	const Arg arg;
} Button;

typedef struct Monitor Monitor;
typedef struct Client Client;
struct Client {
	char name[256];
	float mina, maxa;
	int x, y, w, h;
	int oldx, oldy, oldw, oldh;
	int basew, baseh, incw, inch, maxw, maxh, minw, minh, hintsvalid;
	int bw, oldbw;
	unsigned int tags;
	int isfixed, isfloating, isurgent, neverfocus, oldstate, isfullscreen;
	Client *next;
	Client *snext;
	Monitor *mon;
	Window win;
};

typedef struct {
	unsigned int mod;
	KeySym keysym;
	void (*func)(const Arg *);
	const Arg arg;
} Key;

typedef struct {
	const char *symbol;
	void (*arrange)(Monitor *);
} Layout;

struct Monitor {
	char ltsymbol[16];
	float mfact;
	int nmaster;
	int num;
	int by;               /* bar geometry */
	int mx, my, mw, mh;   /* screen size */
	int wx, wy, ww, wh;   /* window area  */
	unsigned int seltags;
	unsigned int sellt;
	unsigned int tagset[2];
	int showbar;
	int topbar;
	Client *clients;
	Client *sel;
	Client *stack;
	Monitor *next;
	Window barwin;
	const Layout *lt[2];
};

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
static void pop(Client *);
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
static void sigchld(int unused);
static void spawn(const Arg *arg);
static void tag(const Arg *arg);
static void tagmon(const Arg *arg);
static void tile(Monitor *);
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
/* bh is short for bar height, as in how tall the bar is.
 * blw is short for bar layout width and it is set in the drawbar function based on the layout
 * symbol and it is read in the buttonpress function when determining whether the user clicked on
 * the layout symbol or not.
 */
static int bh, blw = 0;      /* bar geometry */
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
 *
 * As per the header comment at the top of this file we have that:
 *    The event handlers of dwm are organized in an array which is accessed whenever
 *    a new event has been fetched. This allows event dispatching in O(1) time.
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
/* Two references to hold the first and selected monitors. */
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

/* function implementations */
void
applyrules(Client *c)
{
	const char *class, *instance;
	unsigned int i;
	const Rule *r;
	Monitor *m;
	XClassHint ch = { NULL, NULL };

	/* rule matching */
	c->isfloating = 0;
	c->tags = 0;
	XGetClassHint(dpy, c->win, &ch);
	class    = ch.res_class ? ch.res_class : broken;
	instance = ch.res_name  ? ch.res_name  : broken;

	for (i = 0; i < LENGTH(rules); i++) {
		r = &rules[i];
		if ((!r->title || strstr(c->name, r->title))
		&& (!r->class || strstr(class, r->class))
		&& (!r->instance || strstr(instance, r->instance)))
		{
			c->isfloating = r->isfloating;
			c->tags |= r->tags;
			for (m = mons; m && m->num != r->monitor; m = m->next);
			if (m)
				c->mon = m;
		}
	}
	if (ch.res_class)
		XFree(ch.res_class);
	if (ch.res_name)
		XFree(ch.res_name);
	c->tags = c->tags & TAGMASK ? c->tags & TAGMASK : c->mon->tagset[c->mon->seltags];
}

int
applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact)
{
	int baseismin;
	Monitor *m = c->mon;

	/* set minimum possible */
	*w = MAX(1, *w);
	*h = MAX(1, *h);
	if (interact) {
		if (*x > sw)
			*x = sw - WIDTH(c);
		if (*y > sh)
			*y = sh - HEIGHT(c);
		if (*x + *w + 2 * c->bw < 0)
			*x = 0;
		if (*y + *h + 2 * c->bw < 0)
			*y = 0;
	} else {
		if (*x >= m->wx + m->ww)
			*x = m->wx + m->ww - WIDTH(c);
		if (*y >= m->wy + m->wh)
			*y = m->wy + m->wh - HEIGHT(c);
		if (*x + *w + 2 * c->bw <= m->wx)
			*x = m->wx;
		if (*y + *h + 2 * c->bw <= m->wy)
			*y = m->wy;
	}
	if (*h < bh)
		*h = bh;
	if (*w < bh)
		*w = bh;
	if (resizehints || c->isfloating || !c->mon->lt[c->mon->sellt]->arrange) {
		if (!c->hintsvalid)
			updatesizehints(c);
		/* see last two sentences in ICCCM 4.1.2.3 */
		baseismin = c->basew == c->minw && c->baseh == c->minh;
		if (!baseismin) { /* temporarily remove base dimensions */
			*w -= c->basew;
			*h -= c->baseh;
		}
		/* adjust for aspect limits */
		if (c->mina > 0 && c->maxa > 0) {
			if (c->maxa < (float)*w / *h)
				*w = *h * c->maxa + 0.5;
			else if (c->mina < (float)*h / *w)
				*h = *w * c->mina + 0.5;
		}
		if (baseismin) { /* increment calculation requires this */
			*w -= c->basew;
			*h -= c->baseh;
		}
		/* adjust for increment value */
		if (c->incw)
			*w -= *w % c->incw;
		if (c->inch)
			*h -= *h % c->inch;
		/* restore base dimensions */
		*w = MAX(*w + c->basew, c->minw);
		*h = MAX(*h + c->baseh, c->minh);
		if (c->maxw)
			*w = MIN(*w, c->maxw);
		if (c->maxh)
			*h = MIN(*h, c->maxh);
	}
	return *x != c->x || *y != c->y || *w != c->w || *h != c->h;
}

void
arrange(Monitor *m)
{
	if (m)
		showhide(m->stack);
	else for (m = mons; m; m = m->next)
		showhide(m->stack);
	if (m) {
		arrangemon(m);
		restack(m);
	} else for (m = mons; m; m = m->next)
		arrangemon(m);
}

void
arrangemon(Monitor *m)
{
	strncpy(m->ltsymbol, m->lt[m->sellt]->symbol, sizeof m->ltsymbol);
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
 *    run -> keypress -> tagmon / movemouse / resizemouse -> sendmon -> attach
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
 *    - for managing the order in which client windows placed on top of each other
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
 *    run -> keypress -> tagmon / movemouse / resizemouse -> sendmon -> attachstack
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
		 * on the layout symbol.
		 *
		 * The variable blw is short for bar layout width and this is set in the drawbar
		 * function when drawing the bar. The need for having a global variable to hold this
		 * information rather than just using TEXTW(m->ltsymbol) is bizarre to say the least,
		 * but presumably this has to do with that it is possible that the layout symbol of
		 * the monitor has been changed since the bar was last drawn (or that may have been
		 * the case in previous iterations of dwm). The variable is set based on the last bar
		 * that was drawn which in principle could be for another monitor - in which case the
		 * blw value could be wrong for the current monitor. */
		} else if (ev->x < x + blw)
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
 * @calls free to memory used by the the colour schemes
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

/*
 * You can test this by finding the window ID of a given window using xwininfo (e.g. 0x5000002) or
 * using xdotool search (94371846) and using xdo or xdotool to activate that window.
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
void
clientmessage(XEvent *e)
{
	XClientMessageEvent *cme = &e->xclient;
	Client *c = wintoclient(cme->window);

	if (!c)
		return;
	if (cme->message_type == netatom[NetWMState]) {
		if (cme->data.l[1] == netatom[NetWMFullscreen]
		|| cme->data.l[2] == netatom[NetWMFullscreen])
			setfullscreen(c, (cme->data.l[0] == 1 /* _NET_WM_STATE_ADD    */
				|| (cme->data.l[0] == 2 /* _NET_WM_STATE_TOGGLE */ && !c->isfullscreen)));
	} else if (cme->message_type == netatom[NetActiveWindow]) {
		if (c != selmon->sel && !c->isurgent)
			seturgent(c, 1);
	}
}

void
configure(Client *c)
{
	XConfigureEvent ce;

	ce.type = ConfigureNotify;
	ce.display = dpy;
	ce.event = c->win;
	ce.window = c->win;
	ce.x = c->x;
	ce.y = c->y;
	ce.width = c->w;
	ce.height = c->h;
	ce.border_width = c->bw;
	ce.above = None;
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
			 * the window manager draws is the bar which does not exceeed the bar height.
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

/*
 * @called_from movemouse as it forwards events to configurerequest
 * @called_from resizemouse as it forwards events to configurerequest
 *
 * Internal call stack:
 *    run -> configurerequest
 *    run -> keypress -> movemouse / resizemouse -> configurerequest
 */
void
configurerequest(XEvent *e)
{
	Client *c;
	Monitor *m;
	XConfigureRequestEvent *ev = &e->xconfigurerequest;
	XWindowChanges wc;

	if ((c = wintoclient(ev->window))) {
		if (ev->value_mask & CWBorderWidth)
			c->bw = ev->border_width;
		else if (c->isfloating || !selmon->lt[selmon->sellt]->arrange) {
			m = c->mon;
			if (ev->value_mask & CWX) {
				c->oldx = c->x;
				c->x = m->mx + ev->x;
			}
			if (ev->value_mask & CWY) {
				c->oldy = c->y;
				c->y = m->my + ev->y;
			}
			if (ev->value_mask & CWWidth) {
				c->oldw = c->w;
				c->w = ev->width;
			}
			if (ev->value_mask & CWHeight) {
				c->oldh = c->h;
				c->h = ev->height;
			}
			if ((c->x + c->w) > m->mx + m->mw && c->isfloating)
				c->x = m->mx + (m->mw / 2 - WIDTH(c) / 2); /* center in x direction */
			if ((c->y + c->h) > m->my + m->mh && c->isfloating)
				c->y = m->my + (m->mh / 2 - HEIGHT(c) / 2); /* center in y direction */
			if ((ev->value_mask & (CWX|CWY)) && !(ev->value_mask & (CWWidth|CWHeight)))
				configure(c);
			if (ISVISIBLE(c))
				XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
		} else
			configure(c);
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
	XSync(dpy, False);
}

Monitor *
createmon(void)
{
	Monitor *m;

	m = ecalloc(1, sizeof(Monitor));
	m->tagset[0] = m->tagset[1] = 1;
	m->mfact = mfact;
	m->nmaster = nmaster;
	m->showbar = showbar;
	m->topbar = topbar;
	m->lt[0] = &layouts[0];
	m->lt[1] = &layouts[1 % LENGTH(layouts)];
	strncpy(m->ltsymbol, layouts[0].symbol, sizeof m->ltsymbol);
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
 *    run -> keypress -> tagmon / movemouse / resizemouse -> sendmon -> detach
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
 *    run -> keypress -> tagmon / movemouse / resizemouse -> sendmon -> detachstack
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

void
drawbar(Monitor *m)
{
	int x, w, tw = 0;
	int boxs = drw->fonts->h / 9;
	int boxw = drw->fonts->h / 6 + 2;
	unsigned int i, occ = 0, urg = 0;
	Client *c;

	if (!m->showbar)
		return;

	/* draw status first so it can be overdrawn by tags later */
	if (m == selmon) { /* status is only drawn on selected monitor */
		drw_setscheme(drw, scheme[SchemeNorm]);
		tw = TEXTW(stext) - lrpad + 2; /* 2px right padding */
		drw_text(drw, m->ww - tw, 0, tw, bh, 0, stext, 0);
	}

	for (c = m->clients; c; c = c->next) {
		occ |= c->tags;
		if (c->isurgent)
			urg |= c->tags;
	}
	x = 0;
	for (i = 0; i < LENGTH(tags); i++) {
		w = TEXTW(tags[i]);
		drw_setscheme(drw, scheme[m->tagset[m->seltags] & 1 << i ? SchemeSel : SchemeNorm]);
		drw_text(drw, x, 0, w, bh, lrpad / 2, tags[i], urg & 1 << i);
		if (occ & 1 << i)
			drw_rect(drw, x + boxs, boxs, boxw, boxw,
				m == selmon && selmon->sel && selmon->sel->tags & 1 << i,
				urg & 1 << i);
		x += w;
	}
	w = blw = TEXTW(m->ltsymbol);
	drw_setscheme(drw, scheme[SchemeNorm]);
	x = drw_text(drw, x, 0, w, bh, lrpad / 2, m->ltsymbol, 0);

	if ((w = m->ww - tw - x) > bh) {
		if (m->sel) {
			drw_setscheme(drw, scheme[m == selmon ? SchemeSel : SchemeNorm]);
			drw_text(drw, x, 0, w, bh, lrpad / 2, m->sel->name, 0);
			if (m->sel->isfloating)
				drw_rect(drw, x + boxs, boxs, boxw, boxw, m->sel->isfixed, 0);
		} else {
			drw_setscheme(drw, scheme[SchemeNorm]);
			drw_rect(drw, x, 0, w, bh, 1, 1);
		}
	}
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

void
enternotify(XEvent *e)
{
	Client *c;
	Monitor *m;
	XCrossingEvent *ev = &e->xcrossing;

	if ((ev->mode != NotifyNormal || ev->detail == NotifyInferior) && ev->window != root)
		return;
	c = wintoclient(ev->window);
	m = c ? c->mon : wintomon(ev->window);
	if (m != selmon) {
		unfocus(selmon->sel, 1);
		selmon = m;
	} else if (!c || c == selmon->sel)
		return;
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
 * then the process is stuck in a while loop inside run -> keypress -> movemouse until you release
 * the button. In other words the event loop is held up while a move or resize takes place and as
 * such status updates stop happening as an example.
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
 *    run -> keypress -> movemouse / resizemouse -> expose
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

void
focus(Client *c)
{
	if (!c || !ISVISIBLE(c))
		for (c = selmon->stack; c && !ISVISIBLE(c); c = c->snext);
	if (selmon->sel && selmon->sel != c)
		unfocus(selmon->sel, 0);
	if (c) {
		if (c->mon != selmon)
			selmon = c->mon;
		if (c->isurgent)
			seturgent(c, 0);
		detachstack(c);
		attachstack(c);
		grabbuttons(c, 1);
		XSetWindowBorder(dpy, c->win, scheme[SchemeSel][ColBorder].pixel);
		setfocus(c);
	} else {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}
	selmon->sel = c;
	drawbars();
}

/* there are some broken focus acquiring clients needing extra handling */
void
focusin(XEvent *e)
{
	XFocusChangeEvent *ev = &e->xfocus;

	if (selmon->sel && ev->window != selmon->sel->win)
		setfocus(selmon->sel);
}

void
focusmon(const Arg *arg)
{
	Monitor *m;

	if (!mons->next)
		return;
	if ((m = dirtomon(arg->i)) == selmon)
		return;
	unfocus(selmon->sel, 0);
	selmon = m;
	focus(NULL);
}

void
focusstack(const Arg *arg)
{
	Client *c = NULL, *i;

	if (!selmon->sel || (selmon->sel->isfullscreen && lockfullscreen))
		return;
	if (arg->i > 0) {
		for (c = selmon->sel->next; c && !ISVISIBLE(c); c = c->next);
		if (!c)
			for (c = selmon->clients; c && !ISVISIBLE(c); c = c->next);
	} else {
		for (i = selmon->clients; i != selmon->sel; i = i->next)
			if (ISVISIBLE(i))
				c = i;
		if (!c)
			for (; i; i = i->next)
				if (ISVISIBLE(i))
					c = i;
	}
	if (c) {
		focus(c);
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
 *    run -> keypress -> movemouse -> getrootptr
 *    run -> configurenotify -> updategeom -> wintomon -> getrootptr
 *    setup -> updategeom -> wintomon -> getrootptr
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

long
getstate(Window w)
{
	int format;
	long result = -1;
	unsigned char *p = NULL;
	unsigned long n, extra;
	Atom real;

	if (XGetWindowProperty(dpy, w, wmatom[WMState], 0L, 2L, False, wmatom[WMState],
		&real, &format, &n, &extra, (unsigned char **)&p) != Success)
		return -1;
	if (n != 0)
		result = *p;
	XFree(p);
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
	if (name.encoding == XA_STRING)
		/* If it is a text string then just copy the data to the designated text variable. */
		strncpy(text, (char *)name.value, size - 1);
	else {
		/* If it is a list of text strings then we need to retrieve it by calling
		 * XmbTextPropertyToTextList and the returned list needs to be freed when we are
		 * finished with it. */
		if (XmbTextPropertyToTextList(dpy, &name, &list, &n) >= Success && n > 0 && *list) {
			strncpy(text, *list, size - 1);
			XFreeStringList(list);
		}
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
 *    ~ -> unfous -> grabbuttons
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
 * @calls XUngrabKey https://tronche.com/gui/x/xlib/input/XUngrabKey.html
 * @calls XGrabKey https://tronche.com/gui/x/xlib/input/XGrabKey.html
 * @calls XKeysymToKeycode https://tronche.com/gui/x/xlib/utilities/keyboard/XKeysymToKeycode.html
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
		unsigned int i, j;
		/* The list of modifiers we are interested in. No additional modifier, the Lock mask,
		 * the Num Lock mask, and Lock and Num Lock mask together. */
		unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
		KeyCode code;

		/* Call to release any keys we may have grabbed before. */
		XUngrabKey(dpy, AnyKey, AnyModifier, root);
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
		 * array earlier and combines each modifier with the modifier defined in the key
		 * bindings array.
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
		for (i = 0; i < LENGTH(keys); i++)
			if ((code = XKeysymToKeycode(dpy, keys[i].keysym)))
				/* Loop through all the modifiers we are interested in */
				for (j = 0; j < LENGTH(modifiers); j++)
					/* Grab the key to tell the X server that we are interested
					 * in receiving KeyPress notifications when the user clicks
					 * on the key in combination with the given modifier. */
					XGrabKey(dpy, code, keys[i].mod | modifiers[j], root,
						True, GrabModeAsync, GrabModeAsync);
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
 *    setup -> updategeom -> isuniquegeom
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

void
manage(Window w, XWindowAttributes *wa)
{
	Client *c, *t = NULL;
	Window trans = None;
	XWindowChanges wc;

	c = ecalloc(1, sizeof(Client));
	c->win = w;
	/* geometry */
	c->x = c->oldx = wa->x;
	c->y = c->oldy = wa->y;
	c->w = c->oldw = wa->width;
	c->h = c->oldh = wa->height;
	c->oldbw = wa->border_width;

	updatetitle(c);
	if (XGetTransientForHint(dpy, w, &trans) && (t = wintoclient(trans))) {
		c->mon = t->mon;
		c->tags = t->tags;
	} else {
		c->mon = selmon;
		applyrules(c);
	}

	if (c->x + WIDTH(c) > c->mon->mx + c->mon->mw)
		c->x = c->mon->mx + c->mon->mw - WIDTH(c);
	if (c->y + HEIGHT(c) > c->mon->my + c->mon->mh)
		c->y = c->mon->my + c->mon->mh - HEIGHT(c);
	c->x = MAX(c->x, c->mon->mx);
	/* only fix client y-offset, if the client center might cover the bar */
	c->y = MAX(c->y, ((c->mon->by == c->mon->my) && (c->x + (c->w / 2) >= c->mon->wx)
		&& (c->x + (c->w / 2) < c->mon->wx + c->mon->ww)) ? bh : c->mon->my);
	c->bw = borderpx;

	wc.border_width = c->bw;
	XConfigureWindow(dpy, w, CWBorderWidth, &wc);
	XSetWindowBorder(dpy, w, scheme[SchemeNorm][ColBorder].pixel);
	configure(c); /* propagates border_width, if size doesn't change */
	updatewindowtype(c);
	updatesizehints(c);
	updatewmhints(c);
	XSelectInput(dpy, w, EnterWindowMask|FocusChangeMask|PropertyChangeMask|StructureNotifyMask);
	grabbuttons(c, 0);
	if (!c->isfloating)
		c->isfloating = c->oldstate = trans != None || c->isfixed;
	if (c->isfloating)
		XRaiseWindow(dpy, c->win);
	attach(c);
	attachstack(c);
	XChangeProperty(dpy, root, netatom[NetClientList], XA_WINDOW, 32, PropModeAppend,
		(unsigned char *) &(c->win), 1);
	XMoveResizeWindow(dpy, c->win, c->x + 2 * sw, c->y, c->w, c->h); /* some windows require this */
	setclientstate(c, NormalState);
	if (c->mon == selmon)
		unfocus(selmon->sel, 0);
	c->mon->sel = c;
	arrange(c->mon);
	XMapWindow(dpy, c->win);
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
 *    run -> keypress -> movemouse / resizemouse -> maprequest
 */
void
maprequest(XEvent *e)
{
	static XWindowAttributes wa;
	XMapRequestEvent *ev = &e->xmaprequest;

	/* This fetches the window attributes from the X server which contains the a lot of
	 * information besides the width and height of the window. Refer to the documentation for
	 * XGetWindowAttributes for more information. */
	if (!XGetWindowAttributes(dpy, ev->window, &wa))
		return;
	/* From the documentation we have that:
	 *   The override_redirect member is set to indicate whether this window overrides structure
	 *   control facilities and can be True or False. Window manager clients should ignore the
	 *   window if this member is True.
	 *
	 * What that boils down to is that if a window has that override-redirect flag set then it
	 * means that the window manages itself and does not want the window manager to exert any
	 * control over the window. A good example of this is dmenu which controls the size and
	 * position on its own and does not want the window manager to intervene.
	 */
	if (wa.override_redirect)
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
	 * is determined by the the window that has focus which will be above other tiled windows in
	 * the stack.
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

void
movemouse(const Arg *arg)
{
	int x, y, ocx, ocy, nx, ny;
	Client *c;
	Monitor *m;
	XEvent ev;
	Time lasttime = 0;

	if (!(c = selmon->sel))
		return;
	if (c->isfullscreen) /* no support moving fullscreen windows by mouse */
		return;
	restack(selmon);
	ocx = c->x;
	ocy = c->y;
	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursor[CurMove]->cursor, CurrentTime) != GrabSuccess)
		return;
	if (!getrootptr(&x, &y))
		return;
	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			if ((ev.xmotion.time - lasttime) <= (1000 / 60))
				continue;
			lasttime = ev.xmotion.time;

			nx = ocx + (ev.xmotion.x - x);
			ny = ocy + (ev.xmotion.y - y);
			if (abs(selmon->wx - nx) < snap)
				nx = selmon->wx;
			else if (abs((selmon->wx + selmon->ww) - (nx + WIDTH(c))) < snap)
				nx = selmon->wx + selmon->ww - WIDTH(c);
			if (abs(selmon->wy - ny) < snap)
				ny = selmon->wy;
			else if (abs((selmon->wy + selmon->wh) - (ny + HEIGHT(c))) < snap)
				ny = selmon->wy + selmon->wh - HEIGHT(c);
			if (!c->isfloating && selmon->lt[selmon->sellt]->arrange
			&& (abs(nx - c->x) > snap || abs(ny - c->y) > snap))
				togglefloating(NULL);
			if (!selmon->lt[selmon->sellt]->arrange || c->isfloating)
				resize(c, nx, ny, c->w, c->h, 1);
			break;
		}
	} while (ev.type != ButtonRelease);
	XUngrabPointer(dpy, CurrentTime);
	if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
		sendmon(c, m);
		selmon = m;
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
 *    ~ -> arrangemon -> tile / monocle -> nexttiled
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
 *    run -> keypress -> movemouse / resizemouse -> recttomon
 *    run -> buttonpress / enternotify / expose -> wintomon -> recttomon
 *    ~ -> updategeom -> wintomon -> rectomon
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
 * then they can freely move that window half-way between two monitors, but if the resize is not
 * interactive then the resize placement and size is restricted to the monitor window area. Refer
 * to the applysizehints function for more details.
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
 *    ~ -> arrangemon -> monocle / tile -> resize
 *    run -> keypress -> movemouse / resizemouse / togglefloating -> resize
 */
void
resize(Client *c, int x, int y, int w, int h, int interact)
{
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
 *    ~ -> clientmessage / updatewindowtype -> setfullscreen -> resizeclient
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
	 * dimensions or position has changed. This is handled in the configure function.
	 */
	configure(c);
	/* This flushes the output buffer and then waits until all requests have been received and
	 * processed by the X server. */
	XSync(dpy, False);
}

void
resizemouse(const Arg *arg)
{
	int ocx, ocy, nw, nh;
	Client *c;
	Monitor *m;
	XEvent ev;
	Time lasttime = 0;

	if (!(c = selmon->sel))
		return;
	if (c->isfullscreen) /* no support resizing fullscreen windows by mouse */
		return;
	restack(selmon);
	ocx = c->x;
	ocy = c->y;
	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursor[CurResize]->cursor, CurrentTime) != GrabSuccess)
		return;
	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1, c->h + c->bw - 1);
	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			if ((ev.xmotion.time - lasttime) <= (1000 / 60))
				continue;
			lasttime = ev.xmotion.time;

			nw = MAX(ev.xmotion.x - ocx - 2 * c->bw + 1, 1);
			nh = MAX(ev.xmotion.y - ocy - 2 * c->bw + 1, 1);
			if (c->mon->wx + nw >= selmon->wx && c->mon->wx + nw <= selmon->wx + selmon->ww
			&& c->mon->wy + nh >= selmon->wy && c->mon->wy + nh <= selmon->wy + selmon->wh)
			{
				if (!c->isfloating && selmon->lt[selmon->sellt]->arrange
				&& (abs(nw - c->w) > snap || abs(nh - c->h) > snap))
					togglefloating(NULL);
			}
			if (!selmon->lt[selmon->sellt]->arrange || c->isfloating)
				resize(c, c->x, c->y, nw, nh, 1);
			break;
		}
	} while (ev.type != ButtonRelease);
	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1, c->h + c->bw - 1);
	XUngrabPointer(dpy, CurrentTime);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
	if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
		sendmon(c, m);
		selmon = m;
		focus(NULL);
	}
}

void
restack(Monitor *m)
{
	Client *c;
	XEvent ev;
	XWindowChanges wc;

	drawbar(m);
	if (!m->sel)
		return;
	if (m->sel->isfloating || !m->lt[m->sellt]->arrange)
		XRaiseWindow(dpy, m->sel->win);
	if (m->lt[m->sellt]->arrange) {
		wc.stack_mode = Below;
		wc.sibling = m->barwin;
		for (c = m->stack; c; c = c->snext)
			if (!c->isfloating && ISVISIBLE(c)) {
				XConfigureWindow(dpy, c->win, CWSibling|CWStackMode, &wc);
				wc.sibling = c->win;
			}
	}
	XSync(dpy, False);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
}

void
run(void)
{
	XEvent ev;
	/* main event loop */
	XSync(dpy, False);
	while (running && !XNextEvent(dpy, &ev))
		if (handler[ev.type])
			handler[ev.type](&ev); /* call handler */
}

void
scan(void)
{
	unsigned int i, num;
	Window d1, d2, *wins = NULL;
	XWindowAttributes wa;

	if (XQueryTree(dpy, root, &d1, &d2, &wins, &num)) {
		for (i = 0; i < num; i++) {
			if (!XGetWindowAttributes(dpy, wins[i], &wa)
			|| wa.override_redirect || XGetTransientForHint(dpy, wins[i], &d1))
				continue;
			if (wa.map_state == IsViewable || getstate(wins[i]) == IconicState)
				manage(wins[i], &wa);
		}
		for (i = 0; i < num; i++) { /* now the transients */
			if (!XGetWindowAttributes(dpy, wins[i], &wa))
				continue;
			if (XGetTransientForHint(dpy, wins[i], &d1)
			&& (wa.map_state == IsViewable || getstate(wins[i]) == IconicState))
				manage(wins[i], &wa);
		}
		if (wins)
			XFree(wins);
	}
}

void
sendmon(Client *c, Monitor *m)
{
	if (c->mon == m)
		return;
	unfocus(c, 1);
	detach(c);
	detachstack(c);
	c->mon = m;
	c->tags = m->tagset[m->seltags]; /* assign tags of target monitor */
	attach(c);
	attachstack(c);
	focus(NULL);
	arrange(NULL);
}

void
setclientstate(Client *c, long state)
{
	long data[] = { state, None };

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
 * @see https://tronche.com/gui/x/xlib/events/client-communication/client-message.html#XClientMessageEvent
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
		 * This can be seen by running this command and clicking on the background
		 * (wallpaper) which is the root window.
		 *
		 * $ xprop | grep _NET_ACTIVE_WINDOW
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

void
setfullscreen(Client *c, int fullscreen)
{
	if (fullscreen && !c->isfullscreen) {
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
			PropModeReplace, (unsigned char*)&netatom[NetWMFullscreen], 1);
		c->isfullscreen = 1;
		c->oldstate = c->isfloating;
		c->oldbw = c->bw;
		c->bw = 0;
		c->isfloating = 1;
		resizeclient(c, c->mon->mx, c->mon->my, c->mon->mw, c->mon->mh);
		XRaiseWindow(dpy, c->win);
	} else if (!fullscreen && c->isfullscreen){
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
			PropModeReplace, (unsigned char*)0, 0);
		c->isfullscreen = 0;
		c->isfloating = c->oldstate;
		c->bw = c->oldbw;
		c->x = c->oldx;
		c->y = c->oldy;
		c->w = c->oldw;
		c->h = c->oldh;
		resizeclient(c, c->x, c->y, c->w, c->h);
		arrange(c->mon);
	}
}

void
setlayout(const Arg *arg)
{
	if (!arg || !arg->v || arg->v != selmon->lt[selmon->sellt])
		selmon->sellt ^= 1;
	if (arg && arg->v)
		selmon->lt[selmon->sellt] = (Layout *)arg->v;
	strncpy(selmon->ltsymbol, selmon->lt[selmon->sellt]->symbol, sizeof selmon->ltsymbol);
	if (selmon->sel)
		arrange(selmon);
	else
		drawbar(selmon);
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
	 * The call to arrange will result in subsequent calls to showhide arrangemon, restack and
	 * drawbar to redraw the bar. While not strictly necessary in this case the performance
	 * overhead is negligible and arrange is a catch all that can prevent obscure issues.
	 */
	arrange(selmon);
}

void
setup(void)
{
	int i;
	XSetWindowAttributes wa;
	Atom utf8string;

	/* clean up any zombies immediately */
	sigchld(0);

	/* init screen */
	screen = DefaultScreen(dpy);
	sw = DisplayWidth(dpy, screen);
	sh = DisplayHeight(dpy, screen);
	root = RootWindow(dpy, screen);
	drw = drw_create(dpy, screen, root, sw, sh);
	if (!drw_fontset_create(drw, fonts, LENGTH(fonts)))
		die("no fonts could be loaded.");
	lrpad = drw->fonts->h;
	bh = drw->fonts->h + 2;
	updategeom();
	/* init atoms */
	utf8string = XInternAtom(dpy, "UTF8_STRING", False);
	wmatom[WMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", False);
	wmatom[WMDelete] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	wmatom[WMState] = XInternAtom(dpy, "WM_STATE", False);
	wmatom[WMTakeFocus] = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
	netatom[NetActiveWindow] = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
	netatom[NetSupported] = XInternAtom(dpy, "_NET_SUPPORTED", False);
	netatom[NetWMName] = XInternAtom(dpy, "_NET_WM_NAME", False);
	netatom[NetWMState] = XInternAtom(dpy, "_NET_WM_STATE", False);
	netatom[NetWMCheck] = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
	netatom[NetWMFullscreen] = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
	netatom[NetWMWindowType] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
	netatom[NetWMWindowTypeDialog] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
	netatom[NetClientList] = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
	/* init cursors */
	cursor[CurNormal] = drw_cur_create(drw, XC_left_ptr);
	cursor[CurResize] = drw_cur_create(drw, XC_sizing);
	cursor[CurMove] = drw_cur_create(drw, XC_fleur);
	/* init appearance */
	scheme = ecalloc(LENGTH(colors), sizeof(Clr *));
	for (i = 0; i < LENGTH(colors); i++)
		scheme[i] = drw_scm_create(drw, colors[i], 3);
	/* init bars */
	updatebars();
	updatestatus();
	/* supporting window for NetWMCheck */
	wmcheckwin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMCheck], XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &wmcheckwin, 1);
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMName], utf8string, 8,
		PropModeReplace, (unsigned char *) "dwm", 3);
	XChangeProperty(dpy, root, netatom[NetWMCheck], XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &wmcheckwin, 1);
	/* EWMH support per view */
	XChangeProperty(dpy, root, netatom[NetSupported], XA_ATOM, 32,
		PropModeReplace, (unsigned char *) netatom, NetLast);
	XDeleteProperty(dpy, root, netatom[NetClientList]);
	/* select events */
	wa.cursor = cursor[CurNormal]->cursor;
	wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask
		|ButtonPressMask|PointerMotionMask|EnterWindowMask
		|LeaveWindowMask|StructureNotifyMask|PropertyChangeMask;
	XChangeWindowAttributes(dpy, root, CWEventMask|CWCursor, &wa);
	XSelectInput(dpy, root, wa.event_mask);
	grabkeys();
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

/* This sets up a signal handler for child processes. The aim of this, as far as I understand,
 * is to make sure that any zombie processes are cleaned up immediately.
 *
 * @called_from setup to clean up any zombie proceses from previous runs
 * @calls signal https://linux.die.net/man/2/signal
 * @calls die to print errors and exit dwm (see util.c)
 * @calls waitpid https://linux.die.net/man/3/waitpid
 * @see https://linux.die.net/man/2/sigaction

 * Internal call stack:
 *    main -> setup -> sigchld
 */
void
sigchld(int unused)
{
	/* If we can't set up the signal handler then end the program with an error */
	if (signal(SIGCHLD, sigchld) == SIG_ERR)
		die("can't install SIGCHLD handler:");
	/* This will wait for child processes to stop or terminate */
	while (0 < waitpid(-1, NULL, WNOHANG));
}

void
spawn(const Arg *arg)
{
	if (arg->v == dmenucmd)
		dmenumon[0] = '0' + selmon->num;
	if (fork() == 0) {
		if (dpy)
			close(ConnectionNumber(dpy));
		setsid();
		execvp(((char **)arg->v)[0], (char **)arg->v);
		fprintf(stderr, "dwm: execvp %s", ((char **)arg->v)[0]);
		perror(" failed");
		exit(EXIT_SUCCESS);
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

void
tile(Monitor *m)
{
	unsigned int i, n, h, mw, my, ty;
	Client *c;

	for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);
	if (n == 0)
		return;

	if (n > m->nmaster)
		mw = m->nmaster ? m->ww * m->mfact : 0;
	else
		mw = m->ww;
	for (i = my = ty = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++)
		if (i < m->nmaster) {
			h = (m->wh - my) / (MIN(n, m->nmaster) - i);
			resize(c, m->wx, m->wy + my, mw - (2*c->bw), h - (2*c->bw), 0);
			if (my + HEIGHT(c) < m->wh)
				my += HEIGHT(c);
		} else {
			h = (m->wh - ty) / (n - i);
			resize(c, m->wx + mw, m->wy + ty, m->ww - mw - (2*c->bw), h - (2*c->bw), 0);
			if (ty + HEIGHT(c) < m->wh)
				ty += HEIGHT(c);
		}
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
 *    run -> keypress -> movemouse / resizemouse -> togglefloating
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

void
unfocus(Client *c, int setfocus)
{
	if (!c)
		return;
	grabbuttons(c, 0);
	XSetWindowBorder(dpy, c->win, scheme[SchemeNorm][ColBorder].pixel);
	if (setfocus) {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
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
		wc.border_width = c->oldbw;
		/* This disables processing of requests and close downs on all other connections than
		 * the one this request arrived on. */
		XGrabServer(dpy); /* avoid race conditions */
		/* Here we set the dummy X error handler just in case what we do next is going to
		 * generate an error that would otherwise cause dwm to exit. */
		XSetErrorHandler(xerrordummy);
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

void
updatebars(void)
{
	Monitor *m;
	XSetWindowAttributes wa = {
		.override_redirect = True,
		.background_pixmap = ParentRelative,
		.event_mask = ButtonPressMask|ExposureMask
	};
	XClassHint ch = {"dwm", "dwm"};
	for (m = mons; m; m = m->next) {
		if (m->barwin)
			continue;
		m->barwin = XCreateWindow(dpy, root, m->wx, m->by, m->ww, bh, 0, DefaultDepth(dpy, screen),
				CopyFromParent, DefaultVisual(dpy, screen),
				CWOverrideRedirect|CWBackPixmap|CWEventMask, &wa);
		XDefineCursor(dpy, m->barwin, cursor[CurNormal]->cursor);
		XMapRaised(dpy, m->barwin);
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
 *    setup -> updategeom -> updatebarpos
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
 * You can check this using the below command and clicking on the desktop background (wallpaper).
 *
 *    $ xprop | grep _NET_CLIENT_LIST
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
updateclientlist()
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

int
updategeom(void)
{
	int dirty = 0;

#ifdef XINERAMA
	if (XineramaIsActive(dpy)) {
		int i, j, n, nn;
		Client *c;
		Monitor *m;
		XineramaScreenInfo *info = XineramaQueryScreens(dpy, &nn);
		XineramaScreenInfo *unique = NULL;

		for (n = 0, m = mons; m; m = m->next, n++);
		/* only consider unique geometries as separate screens */
		unique = ecalloc(nn, sizeof(XineramaScreenInfo));
		for (i = 0, j = 0; i < nn; i++)
			if (isuniquegeom(unique, j, &info[i]))
				memcpy(&unique[j++], &info[i], sizeof(XineramaScreenInfo));
		XFree(info);
		nn = j;

		/* new monitors if nn > n */
		for (i = n; i < nn; i++) {
			for (m = mons; m && m->next; m = m->next);
			if (m)
				m->next = createmon();
			else
				mons = createmon();
		}
		for (i = 0, m = mons; i < nn && m; m = m->next, i++)
			if (i >= n
			|| unique[i].x_org != m->mx || unique[i].y_org != m->my
			|| unique[i].width != m->mw || unique[i].height != m->mh)
			{
				dirty = 1;
				m->num = i;
				m->mx = m->wx = unique[i].x_org;
				m->my = m->wy = unique[i].y_org;
				m->mw = m->ww = unique[i].width;
				m->mh = m->wh = unique[i].height;
				updatebarpos(m);
			}
		/* removed monitors if n > nn */
		for (i = nn; i < n; i++) {
			for (m = mons; m && m->next; m = m->next);
			while ((c = m->clients)) {
				dirty = 1;
				m->clients = c->next;
				detachstack(c);
				c->mon = mons;
				attach(c);
				attachstack(c);
			}
			if (m == selmon)
				selmon = mons;
			cleanupmon(m);
		}
		free(unique);
	} else
#endif /* XINERAMA */
	{ /* default monitor setup */
		if (!mons)
			mons = createmon();
		if (mons->mw != sw || mons->mh != sh) {
			dirty = 1;
			mons->mw = mons->ww = sw;
			mons->mh = mons->wh = sh;
			updatebarpos(mons);
		}
	}
	if (dirty) {
		selmon = mons;
		selmon = wintomon(root);
	}
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

void
updatesizehints(Client *c)
{
	long msize;
	XSizeHints size;

	if (!XGetWMNormalHints(dpy, c->win, &size, &msize))
		/* size is uninitialized, ensure that size.flags aren't used */
		size.flags = PSize;
	if (size.flags & PBaseSize) {
		c->basew = size.base_width;
		c->baseh = size.base_height;
	} else if (size.flags & PMinSize) {
		c->basew = size.min_width;
		c->baseh = size.min_height;
	} else
		c->basew = c->baseh = 0;
	if (size.flags & PResizeInc) {
		c->incw = size.width_inc;
		c->inch = size.height_inc;
	} else
		c->incw = c->inch = 0;
	if (size.flags & PMaxSize) {
		c->maxw = size.max_width;
		c->maxh = size.max_height;
	} else
		c->maxw = c->maxh = 0;
	if (size.flags & PMinSize) {
		c->minw = size.min_width;
		c->minh = size.min_height;
	} else if (size.flags & PBaseSize) {
		c->minw = size.base_width;
		c->minh = size.base_height;
	} else
		c->minw = c->minh = 0;
	if (size.flags & PAspect) {
		c->mina = (float)size.min_aspect.y / size.min_aspect.x;
		c->maxa = (float)size.max_aspect.x / size.max_aspect.y;
	} else
		c->maxa = c->mina = 0.0;
	c->isfixed = (c->maxw && c->maxh && c->maxw == c->minw && c->maxh == c->minh);
	c->hintsvalid = 1;
}

/* This updates the status text by reading the WM_NAME property of the root window.
 *
 * You can test this by running xsetroot like this:
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
	 * You can set a new title for a window using xdotool, but note that many applications
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

void
updatewindowtype(Client *c)
{
	Atom state = getatomprop(c, netatom[NetWMState]);
	Atom wtype = getatomprop(c, netatom[NetWMWindowType]);

	if (state == netatom[NetWMFullscreen])
		setfullscreen(c, 1);
	if (wtype == netatom[NetWMWindowTypeDialog])
		c->isfloating = 1;
}

void
updatewmhints(Client *c)
{
	XWMHints *wmh;

	if ((wmh = XGetWMHints(dpy, c->win))) {
		if (c == selmon->sel && wmh->flags & XUrgencyHint) {
			wmh->flags &= ~XUrgencyHint;
			XSetWMHints(dpy, c->win, wmh);
		} else
			c->isurgent = (wmh->flags & XUrgencyHint) ? 1 : 0;
		if (wmh->flags & InputHint)
			c->neverfocus = !wmh->input;
		else
			c->neverfocus = 0;
		XFree(wmh);
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
 *    setup -> updategeom -> wintomon -> wintoclient
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
 *    setup -> updategeom -> wintomon
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
	 * having a NULL arrange function as defined in the layouts array), or if the selected
	 * client is floating, then we do nothing. */
	if (!selmon->lt[selmon->sellt]->arrange
	|| (selmon->sel && selmon->sel->isfloating))
		return;
	/* If the selected client is already the master client (or both are NULL) then */
	if (c == nexttiled(selmon->clients))
		/* try to find the next tiled client, but bail if there is no other tiled clients */
		if (!c || !(c = nexttiled(c->next)))

			return;
	/* Call pop to move the client to the top of the tile stack so that it becomes the new
	 * master client. */
	pop(c);
}

int
main(int argc, char *argv[])
{
	if (argc == 2 && !strcmp("-v", argv[1]))
		die("dwm-"VERSION);
	else if (argc != 1)
		die("usage: dwm [-v]");
	if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		fputs("warning: no locale support\n", stderr);
	if (!(dpy = XOpenDisplay(NULL)))
		die("dwm: cannot open display");
	checkotherwm();
	setup();
#ifdef __OpenBSD__
	if (pledge("stdio rpath proc exec", NULL) == -1)
		die("pledge");
#endif /* __OpenBSD__ */
	scan();
	run();
	cleanup();
	XCloseDisplay(dpy);
	return EXIT_SUCCESS;
}
