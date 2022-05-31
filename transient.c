/* cc transient.c -o transient -lX11 */

#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

/* Why do we have a transient.c file in the dwm source code? It is not sourced by any of the other
 * files and it is not referred to in the Makefile either.
 *
 * This is a not so uncommon question and the answer is that transient is just a small test tool
 * to help test dwm features that involves transient windows.
 *
 * The first line at the top of the file tells you how you can compile this file.
 *
 * Running the compiled transient executable will open a simple 400x400 blank window. The window
 * title is "floating" and it is intended to start as floating due to setting the minimum and
 * maximum size hints to the same value of 400, which should cause the window to be "fixed" in
 * size. The window starts at position 100,100.
 *
 * After five seconds it will open another smaller window that has the WM_TRANSIENT_FOR property
 * set referring to the parent window. The name of the transient window is "transient" and should
 * be floating because it is transient. The window has a size of 100x100 and starts at position
 * 50,50 and will overlap the parent window. Client rules will not apply for this window as it is
 * transient.
 *
 * @calls XOpenDisplay https://tronche.com/gui/x/xlib/display/opening.html
 * @calls XCloseDisplay https://tronche.com/gui/x/xlib/display/XCloseDisplay.html
 * @calls XMapWindow https://tronche.com/gui/x/xlib/window/XMapWindow.html
 * @calls DefaultRootWindow a macro that returns the root window for the default screen
 * @calls XCreateSimpleWindow https://tronche.com/gui/x/xlib/window/XCreateWindow.html
 * @calls XSetWMNormalHints https://tronche.com/gui/x/xlib/ICC/client-to-window-manager/XSetWMNormalHints.html
 * @calls XStoreName https://tronche.com/gui/x/xlib/ICC/client-to-window-manager/XStoreName.html
 * @calls XMapWindow https://tronche.com/gui/x/xlib/window/XMapWindow.html
 * @calls XSelectInput https://tronche.com/gui/x/xlib/event-handling/XSelectInput.html
 * @calls XNextEvent https://tronche.com/gui/x/xlib/event-handling/manipulating-event-queue/XNextEvent.html
 * @calls XSetTransientForHint https://tronche.com/gui/x/xlib/ICC/client-to-window-manager/XSetTransientForHint.html
 * @calls exit https://linux.die.net/man/3/exit
 * @calls sleep https://linux.die.net/man/3/sleep
 */

int main(void) {
	Display *d;
	Window r, f, t = None;
	XSizeHints h;
	XEvent e;

	/* This opens the display and bails if it can't. */
	d = XOpenDisplay(NULL);
	if (!d)
		exit(1);
	/* Get the root window for the display. */
	r = DefaultRootWindow(d);

	/* This creates the main (parent) window at position 100,100 with a size of 400x400. The three
	 * values at the end are for border width, border colour and background colour. */
	f = XCreateSimpleWindow(d, r, 100, 100, 400, 400, 0, 0, 0);
	/* This sets the minimum and maximum size hints to 400x400, which should make the client fixed
	 * in size and thus become a floating window that can not become tiled. */
	h.min_width = h.max_width = h.min_height = h.max_height = 400;
	/* Hint flags to say that we provided hints for minimum and maximum size. */
	h.flags = PMinSize | PMaxSize;
	/* This sets the hints specified above for the window. */
	XSetWMNormalHints(d, f, &h);
	/* This sets the name (title) for the main window to "floating". */
	XStoreName(d, f, "floating");
	/* Map the window so that it can be seen. */
	XMapWindow(d, f);

	/* This tells the window manager we are interested in receiving Expose events in relation to
	 * this window. */
	XSelectInput(d, f, ExposureMask);
	while (1) {
		/* This will fetch the next event in the queue. Any event is simply ignored. */
		XNextEvent(d, &e);

		/* If we have not yet created a transient window then */
		if (t == None) {
			/* we first wait five seconds. */
			sleep(5);
			/* Then we create the new window at position 50,50 with a size of 100x100. */
			t = XCreateSimpleWindow(d, r, 50, 50, 100, 100, 0, 0, 0);
			/* This sets the WM_TRANSIENT_FOR property for the new window referring to the floating
			 * window as its parent window. */
			XSetTransientForHint(d, t, f);
			/* This sets the name (title) for the transient window to "transient". */
			XStoreName(d, t, "transient");
			/* Map the window so that it can be seen. */
			XMapWindow(d, t);
			/* This tells the window manager we are interested in receiving Expose events in
			 * relation to this window. */
			XSelectInput(d, t, ExposureMask);
		}
	}

	/* Call to close the display and exit. This code is for all practical reasons unreachable as
	 * the while loop above is infinite and we will end up forcibly killing the window to make it
	 * go away. */
	XCloseDisplay(d);
	exit(0);
}
