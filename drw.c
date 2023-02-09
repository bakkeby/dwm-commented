/* See LICENSE file for copyright and license details. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

/* The drw library comes from libsl which contains some functionality that is common for several
 * suckless project, e.g. dmenu and dwm.
 *
 * @see https://git.suckless.org/libsl/files.html
 */

#include "drw.h"
#include "util.h"

/* UTF-8 is a popular encoding of multi-byte Unicode code-points into eight-bit octets.
 *
 * The functions below are in relation to working out how many bytes a single UTF-8 character spans
 * and is used in the drw_text function when progressing through text one UTF-8 character at a time
 * to work out what font has a glyph representing said character.
 *
 * We are not going to go into detail when it comes to how UTF-8 is decoded - there should be
 * better references regarding that elsewhere.
 *
 * @see https://rosettacode.org/wiki/UTF-8
 * @see https://rosettacode.org/wiki/UTF-8_encode_and_decode
 */
#define UTF_INVALID 0xFFFD
#define UTF_SIZ     4

/* These arrays contain binary values that relate to the UTF-8 format. */
static const unsigned char utfbyte[UTF_SIZ + 1] = {0x80,    0, 0xC0, 0xE0, 0xF0};
static const unsigned char utfmask[UTF_SIZ + 1] = {0xC0, 0x80, 0xE0, 0xF0, 0xF8};
static const long utfmin[UTF_SIZ + 1] = {       0,    0,  0x80,  0x800,  0x10000};
static const long utfmax[UTF_SIZ + 1] = {0x10FFFF, 0x7F, 0x7FF, 0xFFFF, 0x10FFFF};

/* Internal function to decode a single byte.
 *
 * @called_from utf8decode in relation to validating and decoding UTF-8 text
 *
 * Internal call stack:
 *    ~ -> drawbar -> drw_text -> utf8decode -> utf8decodebyte
 */
static long
utf8decodebyte(const char c, size_t *i)
{
	/* This loops through all the UTF-8 bytes and byte masks in the corresponding arrays and
	 * checks if the character falls under one of the four buckets. */
	for (*i = 0; *i < (UTF_SIZ + 1); ++(*i))
		if (((unsigned char)c & utfmask[*i]) == utfbyte[*i])
			/* If a character with the mask applied matched the entry in utfbyte then return the
			 * chracter less the bits of the mask. */
			return (unsigned char)c & ~utfmask[*i];
	return 0;
}

/* Internal function to validate that a UTF-8 value is between certain values.
 *
 * @called_from utf8decode in relation to validating and decoding UTF-8 text
 *
 * Internal call stack:
 *    ~ -> drawbar -> drw_text -> utf8decode -> utf8validate
 */
static size_t
utf8validate(long *u, size_t i)
{
	/* This checks whether the given value is between the byte ranges defined for each of the four
	 * buckets. */
	if (!BETWEEN(*u, utfmin[i], utfmax[i]) || BETWEEN(*u, 0xD800, 0xDFFF))
		*u = UTF_INVALID;
	for (i = 1; *u > utfmax[i]; ++i)
		;
	return i;
}

/* This is a function that works out how many bytes a given UTF-8 character spans.
 *
 * @called_from drw_text to get the number of bytes a multi-byte UTF-8 character uses
 * @calls utf8decodebyte to work out what data a UTF-8 byte holds
 * @calls utf8validate to check a decoded UTF-8 character if the byte length could not be derived
 *
 * Internal call stack:
 *    ~ -> drawbar -> drw_text -> utf8decode
 */
static size_t
utf8decode(const char *c, long *u, size_t clen)
{
	size_t i, j, len, type;
	long udecoded;

	*u = UTF_INVALID;
	if (!clen)
		return 0;

	/* Decode the first byte of the character string. */
	udecoded = utf8decodebyte(c[0], &len);

	/* If this is a single-byte UTF-8 character then the length of 1 (byte). */
	if (!BETWEEN(len, 1, UTF_SIZ))
		return 1;

	/* Otherwise keep looping through bytes until we find the last one. The variable j keeps track
	 * of the number of bytes we have checked.  */
	for (i = 1, j = 1; i < clen && j < len; ++i, ++j) {
		udecoded = (udecoded << 6) | utf8decodebyte(c[i], &type);
		/* If type is a non-zero value then we know how many bytes the UTF-8 character holds and we
		 * can return that directly. The type value will be positive if the utf8decodebyte function
		 * had to loop over over more than one byte / mask check. */
		if (type)
			return j;
	}
	/* If the for loop above exited prematurely then that suggests that the multi-byte UTF-8
	 * character is incomplete. We return 0 in this case as we can't draw half a character. This
	 * will also be in the event that we have a multi-byte character with more than 4 bytes. */
	if (j < len)
		return 0;

	/* If we have come this far then run the decoded value through utf8validate to get the length
	 * of the character. */
	*u = udecoded;
	utf8validate(u, len);

	return len;
}

/* Function to create the drawable, which is an internal structure used to hold the drawable pixel
 * map, the graphics context and other references.
 *
 * @called_from setup to create the drawable
 * @calls ecalloc to allocate memory for the drawable
 * @calls XCreatePixmap https://tronche.com/gui/x/xlib/pixmap-and-cursor/XCreatePixmap.html
 * @calls DefaultDepth https://linux.die.net/man/3/defaultdepth
 * @calls XCreateGC https://tronche.com/gui/x/xlib/GC/XCreateGC.html
 * @calls XSetLineAttributes https://tronche.com/gui/x/xlib/GC/convenience-functions/XSetLineAttributes.html
 * @see http://tinf2.vub.ac.be/~dvermeir/manuals/xlib/GC/manipulating.html
 *
 * Internal call stack:
 *    main -> setup -> drw_create
 */
Drw *
drw_create(Display *dpy, int screen, Window root, unsigned int w, unsigned int h)
{
	Drw *drw = ecalloc(1, sizeof(Drw));

	drw->dpy = dpy;
	drw->screen = screen;
	drw->root = root;
	drw->w = w;
	drw->h = h;
	/* The drawable is a pixel map that is used to draw things that are later copied to the bar
	 * window. */
	drw->drawable = XCreatePixmap(dpy, root, w, h, DefaultDepth(dpy, screen));

	/* The GC is the graphics context that is used in relation to colours. The graphics context
	 * is passed to other function such as XSetLineAttributes, XSetForeground, XFillRectangle,
	 * XDrawRectangle, XCopyArea and XFreeGC. */
	drw->gc = XCreateGC(dpy, root, 0, NULL);

	/* This sets the line attributes for the graphics context. This affects how lines and
	 * rectangles are drawn.
	 *
	 * Noting some details from http://tinf2.vub.ac.be/~dvermeir/manuals/xlib/GC/manipulating.html
	 * regarding the options.
	 *
	 *    The line-style defines which sections of a line are drawn:
	 *        LineSolid       The full path of the line is drawn.
	 *        LineDoubleDash  The full path of the line is drawn, but the even dashes are filled
	 *                        differently than the odd dashes with CapButt style used where even
	 *                        and odd dashes meet.
	 *        LineOnOffDash   Only the even dashes are drawn, and cap-style applies to all internal
	 *                        ends of the individual dashes, except CapNotLast is treated as
	 *                        CapButt.
	 *
	 *    The cap-style defines how the endpoints of a path are drawn:
	 *        CapNotLast      This is equivalent to CapButt except that for a line-width of zero
	 *                        the final endpoint is not drawn.
	 *        CapButt         The line is square at the endpoint (perpendicular to the slope of the
	 *                        line) with no projection beyond.
	 *        CapRound        The line has a circular arc with the diameter equal to the line-width,
	 *                        centered on the endpoint. (This is equivalent to CapButt for
	 *                        line-width of zero).
	 *        CapProjecting   The line is square at the end, but the path continues beyond the
	 *                        endpoint for a distance equal to half the line-width. (This is
	 *                        equivalent to CapButt for line-width of zero).
	 *
	 *    The join-style defines how corners are drawn for wide lines:
	 *        JoinMiter       The outer edges of two lines extend to meet at an angle. However,
	 *                        if the angle is less than 11 degrees, then a JoinBevel join-style is
	 *                        used instead.
	 *        JoinRound       The corner is a circular arc with the diameter equal to the
	 *                        line-width, centered on the joinpoint.
	 *        JoinBevel       The corner has CapButt endpoint styles with the triangular notch
	 *                        filled.
	 *
	 *    For a line with coincident endpoints (x1=x2, y1=y2), when the cap-style is applied to
	 *    both endpoints, the semantics depends on the line-width and the cap-style:
	 *        CapNotLast      thin   The results are device-dependent, but the desired effect is
	 *                               that nothing is drawn.
	 *        CapButt         thin   The results are device-dependent, but the desired effect is
	 *                               that a single pixel is drawn.
	 *        CapRound        thin   The results are the same as for CapButt/thin.
	 *        CapProjecting   thin   The results are the same as for CapButt/thin.
	 *        CapButt         wide   Nothing is drawn.
	 *        CapRound        wide   The closed path is a circle, centered at the endpoint, and
	 *                               with the diameter equal to the line-width.
	 *        CapProjecting   wide   The closed path is a square, aligned with the coordinate axes,
	 *                               centered at the endpoint, and with the sides equal to the
	 *                               line-width.
	 */
	XSetLineAttributes(dpy, drw->gc, 1, LineSolid, CapButt, JoinMiter);

	return drw;
}

/* Function that resizes the drawable pixel map.
 *
 * @called_from drw_resize to change the size of the drawable pixmap when the scren size changes
 * @calls XFreePixmap https://tronche.com/gui/x/xlib/pixmap-and-cursor/XFreePixmap.html
 * @calls XCreatePixmap https://tronche.com/gui/x/xlib/pixmap-and-cursor/XCreatePixmap.html
 * @calls DefaultDepth https://linux.die.net/man/3/defaultdepth
 *
 * Internal call stack:
 *    run -> configurenotify -> updategeom -> drw_resize
 */
void
drw_resize(Drw *drw, unsigned int w, unsigned int h)
{
	if (!drw)
		return;

	/* Set the new height and width for internal reference. */
	drw->w = w;
	drw->h = h;

	/* The drawable pixel map cannot simply be resized, so we remove it and create a new one with
	 * the desired dimensions. */
	if (drw->drawable)
		XFreePixmap(drw->dpy, drw->drawable);
	drw->drawable = XCreatePixmap(drw->dpy, drw->root, w, h, DefaultDepth(drw->dpy, drw->screen));
}

/* This frees the drawable and its fonts.
 *
 * @called_from cleanup to handle the freeing of the drawable
 * @calls XFreePixmap https://tronche.com/gui/x/xlib/pixmap-and-cursor/XFreePixmap.html
 * @calls XFreeGC https://tronche.com/gui/x/xlib/GC/XFreeGC.html
 * @calls drw_fontset_free to free all fonts
 * @calls free to free the drawable
 *
 * Internal call stack:
 *    main -> cleanup -> drw_free
 */
void
drw_free(Drw *drw)
{
	/* Free our Drawable instance. */
	XFreePixmap(drw->dpy, drw->drawable);
	/* Free our GC (graphics context). */
	XFreeGC(drw->dpy, drw->gc);
	/* Call drw_fontset_free to loop through and free all the fonts that have been loaded. */
	drw_fontset_free(drw->fonts);
	/* Finally free our internal Drw structure. */
	free(drw);
}

/* Function to load a single font.
 *
 * This function is an implementation detail. Library users should use drw_fontset_create instead.
 *
 * @called_from drw_fontset_create to load named fonts
 * @calls XftFontOpenName https://www.x.org/archive/X11R7.5/doc/man/man3/Xft.3.html
 * @calls XftFontClose https://www.x.org/archive/X11R7.5/doc/man/man3/Xft.3.html
 * @calls XftFontOpenPattern https://www.x.org/archive/X11R7.5/doc/man/man3/Xft.3.html
 * @calls FcPatternGetBool https://www.freedesktop.org/software/fontconfig/fontconfig-devel/fcpatternget-type.html
 * @calls die in the event that no font was specified
 * @calls ecalloc to allocate memory for the new font
 *
 * Internal call stack:
 *    main -> setup -> drw_fontset_create -> xfont_create
 */
static Fnt *
xfont_create(Drw *drw, const char *fontname, FcPattern *fontpattern)
{
	Fnt *font;
	XftFont *xfont = NULL;
	FcPattern *pattern = NULL;

	/* Loading font using the font name. */
	if (fontname) {
		/* Using the pattern found at font->xfont->pattern does not yield the
		 * same substitution results as using the pattern returned by
		 * FcNameParse; using the latter results in the desired fallback
		 * behaviour whereas the former just results in missing-character
		 * rectangles being drawn, at least with some fonts. */
		if (!(xfont = XftFontOpenName(drw->dpy, drw->screen, fontname))) {
			fprintf(stderr, "error, cannot load font from name: '%s'\n", fontname);
			return NULL;
		}
		if (!(pattern = FcNameParse((FcChar8 *) fontname))) {
			fprintf(stderr, "error, cannot parse font name to pattern: '%s'\n", fontname);
			XftFontClose(drw->dpy, xfont);
			return NULL;
		}
	/* Fall back to loading fonts via a font pattern in the event that a name is not provided. */
	} else if (fontpattern) {
		if (!(xfont = XftFontOpenPattern(drw->dpy, fontpattern))) {
			fprintf(stderr, "error, cannot load font from pattern.\n");
			return NULL;
		}
	} else {
		die("no font specified.");
	}

	/* Allocate memory for the new font. Note that Fnt is an internal structure defined in drw.h. */
	font = ecalloc(1, sizeof(Fnt));
	font->xfont = xfont;
	font->pattern = pattern;

	/* The font height is the distance between the fonts' ascent and descent values.
	 *
	 * The ascent value is the recommended distance above the baseline for single spaced text while
	 * the descent value is the recommended distance below the baseline.
	 *
	 * Refer to the following stackoverflow topic that demonstrates this with an image.
	 * https://stackoverflow.com/questions/27631736/meaning-of-top-ascent-baseline-descent-bottom-and-leading-in-androids-font
	 */
	font->h = xfont->ascent + xfont->descent;
	/* Set the display for the font. Used when making calls to XftTextExtentsUtf8 and
	 * XftFontClose. */
	font->dpy = drw->dpy;

	return font;
}

/* Function to free a font.
 *
 * @called_from drw_text to free a loaded font that was not desireable
 * @called_from drw_fontset_free to free all fonts
 * @calls FcPatternDestroy https://www.freedesktop.org/software/fontconfig/fontconfig-devel/fcpatterndestroy.html
 * @calls XftFontClose https://www.x.org/archive/X11R7.5/doc/man/man3/Xft.3.html
 * @calls free to release the memory used by the font
 *
 * Internal call stack:
 *    ~ -> drawbar -> drw_text -> xfont_free
 *    ~ -> drawbar -> drw_fontset_getwidth -> drw_text -> xfont_free
 *    run -> buttonpress -> drw_fontset_getwidth -> drw_text -> xfont_free
 *    main -> cleanup -> drw_free -> drw_fontset_free -> xfont_free
 */
static void
xfont_free(Fnt *font)
{
	if (!font)
		return;
	/* Clear the font pattern if it exists. */
	if (font->pattern)
		FcPatternDestroy(font->pattern);
	/* Close the font - marks the font as unused for the display. */
	XftFontClose(font->dpy, font->xfont);
	/* Release the memory used for the font. */
	free(font);
}

/* Function to loop through and load fonts based on names in an array.
 *
 * @called_from setup to load the fonts defined in the fonts array in the configuration file
 * @calls xfont_create to load each font
 *
 * Internal call stack:
 *    main -> setup -> drw_fontset_create
 */
Fnt*
drw_fontset_create(Drw* drw, const char *fonts[], size_t fontcount)
{
	Fnt *cur, *ret = NULL;
	size_t i;

	/* General guard, should never happen in practice. */
	if (!drw || !fonts)
		return NULL;

	/* Loop through all fonts. Note how the loop starts at 1 rather than 0, this has to do with
	 * that we are loading the last font first, then working our way upwards to the first font.
	 * This is also reflected in how we add the new font to the head of the linked list of fonts
	 * used. */
	for (i = 1; i <= fontcount; i++) {
		/* Try to load the given font */
		if ((cur = xfont_create(drw, fonts[fontcount - i], NULL))) {
			/* If the font could be loaded insert this new font at the head of the linked list of
			 * fonts. */
			cur->next = ret;
			ret = cur;
		}
	}

	/* Return the linked list of fonts created. */
	return (drw->fonts = ret);
}

/* Function to recursively free fonts in an array.
 *
 * @called_from drw_free to free the all fonts
 * @called_from drw_fontset_free (itself) to free the remaining fonts
 * @calls drw_fontset_free (itself) to free the remaining fonts
 * @calls xfont_free to release the memory used for this font
 *
 * Internal call stack:
 *    main -> cleanup -> drw_free -> drw_fontset_free -> drw_fontset_free
 *                                       ^__________________/
 */
void
drw_fontset_free(Fnt *font)
{
	/* Only enter if we have any more fonts to free. */
	if (font) {
		/* Calls drw_fontset_free recursively. */
		drw_fontset_free(font->next);
		/* Release the memory used by the font. */
		xfont_free(font);
	}
}

/* Wrapper function to create an XftColor based on a given name.
 *
 * @called_from setup to create the various colour schemes
 * @calls XftColorAllocName https://www.x.org/archive/X11R7.5/doc/man/man3/Xft.3.html
 * @calls DefaultVisual https://linux.die.net/man/3/defaultvisual
 * @calls DefaultColormap https://linux.die.net/man/3/defaultcolormap
 * @calls die in the event that a colour could not be created
 *
 * Internal call stack:
 *    main -> setup -> drw_scm_create -> drw_clr_create
 */
void
drw_clr_create(Drw *drw, Clr *dest, const char *clrname)
{
	if (!drw || !dest || !clrname)
		return;

	/* This creates the colour. The colour name can be on the form of "#3399ab", "#ccc" or "red". */
	if (!XftColorAllocName(drw->dpy, DefaultVisual(drw->dpy, drw->screen),
	                       DefaultColormap(drw->dpy, drw->screen),
	                       clrname, dest))
		die("error, cannot allocate color '%s'", clrname);

	/* It is worth noting that the created colours do not have anything set for the alpha channel
	 * which is used in the context of compositors and transparency. As such using a compositor
	 * to apply transparency will also affect the window borders. A workaround for this is to
	 * remove transparency from the created colours by setting the alpha value to be 100% opaque.
	 *
	 *    dest->pixel |= 0xff << 24;
	 */
}

/* Wrapper to create color schemes. The caller has to call free(3) on the
 * returned color scheme when done using it.
 *
 * A colour scheme contains multiple (XftColor) colours. In dwm a colour scheme contains by default
 * three colours; the forground colour, the background colour and the border colour.
 *
 * @called_from setup to create the various colour schemes
 * @calls drw_clr_create to create the individual colours
 * @calls ecalloc to allocate memory for the colour scheme
 *
 * Internal call stack:
 *    main -> setup -> drw_scm_create
 */
Clr *
drw_scm_create(Drw *drw, const char *clrnames[], size_t clrcount)
{
	size_t i;
	Clr *ret;

	/* Need at least two colors for a scheme. Why that restriction exists is not clear, but
	 * presumably if you only needed a single colour then you would just call drw_clr_create
	 * and use the returned colour directly. The rest are merely guards in case something is
	 * not as it should be. The last ecalloc allocates memory to hold the number of colours in
	 * the colour scheme. */
	if (!drw || !clrnames || clrcount < 2 || !(ret = ecalloc(clrcount, sizeof(XftColor))))
		return NULL;

	/* Loop through all the colours and create them while storing them in the XftColor array
	 * (which represents our colour scheme). */
	for (i = 0; i < clrcount; i++)
		drw_clr_create(drw, &ret[i], clrnames[i]);
	/* Return the created colour scheme. */
	return ret;
}

/* Function to set or change the given font set.
 *
 * This function is not used in dwm.
 */
void
drw_setfontset(Drw *drw, Fnt *set)
{
	if (drw)
		drw->fonts = set;
}

/* Function to set the next colour scheme to use when drawing text or othwerwise.
 *
 * @called_from drawbar to change colours for different segments of the bar
 *
 * Internal call stack:
 *    ~ -> drawbar -> drw_setscheme
 */
void
drw_setscheme(Drw *drw, Clr *scm)
{
	/* General guard in the unlikely event that drw would be NULL. */
	if (drw)
		/* Set the scheme in our drawable struct. */
		drw->scheme = scm;
}

/* Function to draw a filled or hollow rectangle.
 *
 * @called_from drawbar to draw rectangles on the bar
 * @calls XSetForeground https://tronche.com/gui/x/xlib/GC/convenience-functions/XSetForeground.html
 * @calls XFillRectangle https://tronche.com/gui/x/xlib/graphics/filling-areas/XFillRectangle.html
 * @calls XDrawRectangle https://tronche.com/gui/x/xlib/graphics/drawing/XDrawRectangle.html
 *
 * Internal call stack:
 *    ~ -> drawbar -> drw_rect
 */
void
drw_rect(Drw *drw, int x, int y, unsigned int w, unsigned int h, int filled, int invert)
{
	/* General guard, should never happen in practice. */
	if (!drw || !drw->scheme)
		return;

	/* This sets the foreground colour to the current colour scheme's foreground pixel. If the
	 * colours are inverted then the current colour scheme's background pixel is used instead. */
	XSetForeground(drw->dpy, drw->gc, invert ? drw->scheme[ColBg].pixel : drw->scheme[ColFg].pixel);
	/* If the rectangle should be solid then we call the XFillRectangle function to draw it. */
	if (filled)
		XFillRectangle(drw->dpy, drw->drawable, drw->gc, x, y, w, h);
	/* Otherwise we call the XDrawRectangle function to draw a hollow rectangle. */
	else
		XDrawRectangle(drw->dpy, drw->drawable, drw->gc, x, y, w - 1, h - 1);
}

/* This function handles the drawing of text as well as calculating the width of text when called
 * via drw_fontset_getwidth.
 *
 * The general flow is to:
 *    - work out how many bytes the next multi-byte UTF-8 character spans
 *    - find a font that has a glyph for that character, this may involve searching for and loading
 *      additional fonts
 *    - work out the width of the character when drawn with that font
 *    - draw the as many characters as possible using the specific font, but fall back to the
 *      primary font if that has a glyph for the next character
 *    - if the text is too long to be shown, then end the text by drawing an ellipsis (...)
 *
 * @called_from drawbar to draw text on the bar (e.g. status, tags, layout symbol)
 * @called_from drw_fontset_getwidth to calculate the width of text
 * @called_from drw_fontset_getwidth_clamp to calculate the width of text up to a certain value
 * @called_from drw_text itself to draw the ellipsis if the text is too long to fit
 * @calls FcCharSetCreate https://www.freedesktop.org/software/fontconfig/fontconfig-devel/fccharsetcreate.html
 * @calls FcCharSetAddChar https://www.freedesktop.org/software/fontconfig/fontconfig-devel/fccharsetaddchar.html
 * @calls FcPatternDuplicate https://www.freedesktop.org/software/fontconfig/fontconfig-devel/fcpatternduplicate.html
 * @calls FcPatternAddCharSet https://www.freedesktop.org/software/fontconfig/fontconfig-devel/fcpatternadd-type.html
 * @calls FcPatternAddBool https://www.freedesktop.org/software/fontconfig/fontconfig-devel/fcpatternadd-type.html
 * @calls FcConfigSubstitute https://www.freedesktop.org/software/fontconfig/fontconfig-devel/fcconfigsubstitute.html
 * @calls FcDefaultSubstitute https://www.freedesktop.org/software/fontconfig/fontconfig-devel/fcdefaultsubstitute.html
 * @calls FcCharSetDestroy https://www.freedesktop.org/software/fontconfig/fontconfig-devel/fccharsetdestroy.html
 * @calls FcPatternDestroy https://www.freedesktop.org/software/fontconfig/fontconfig-devel/fcpatterndestroy.html
 * @calls XSetForeground https://tronche.com/gui/x/xlib/GC/convenience-functions/XSetForeground.html
 * @calls XFillRectangle https://tronche.com/gui/x/xlib/graphics/filling-areas/XFillRectangle.html
 * @calls XftDrawCreate https://www.x.org/archive/X11R7.5/doc/man/man3/Xft.3.html
 * @calls XftCharExists https://www.x.org/archive/X11R7.5/doc/man/man3/Xft.3.html
 * @calls XftFontMatch https://www.x.org/archive/X11R7.5/doc/man/man3/Xft.3.html
 * @calls XftDrawDestroy https://www.x.org/archive/X11R7.5/doc/man/man3/Xft.3.html
 * @calls XftDrawStringUtf8 https://www.x.org/archive/X11R7.5/doc/man/man3/Xft.3.html
 * @calls DefaultVisual https://linux.die.net/man/3/defaultvisual
 * @calls DefaultColormap https://linux.die.net/man/3/defaultcolormap
 * @calls xfont_create in the event additional fallback fonts need to be loaded
 * @calls xfont_free if a loaded font did not contain the desired glyph
 * @calls utf8decode to work out the number of bytes in a multi-byte UTF-8 character
 * @calls drw_font_getexts to work out the width of a character when drawn using a specific font
 * @calls drw_fontset_getwidth to work out the width of the ellipsis
 * @calls drw_text to draw the ellipsis
 * @calls die in the event that something goes wrong loading a font
 *
 * Internal call stack:
 *    ~ -> drawbar -> drw_text
 *    ~ -> drawbar -> drw_fontset_getwidth -> drw_text
 *    run -> buttonpress -> drw_fontset_getwidth -> drw_text
 */
int
drw_text(Drw *drw, int x, int y, unsigned int w, unsigned int h, unsigned int lpad, const char *text, int invert)
{
	/* Initialising a series of variables:
	 *    i - iterator used in the context of checking known glyphs that we know there exists no
	 *        font for
	 *    ty - text y position, used in the context of fallback fonts that may have a different
	 *         height to the primary font
	 *    ellipsis_x - the position that the ellipsis will be drawn at, if needed
	 *    ellipsis_w - the remaining space available to draw the ellipsis
	 *    ellipsis_len - keeps track of the number of bytes in the multi-byte UTF-8 character that
	 *                   is drawn at the ellipsis_x position
	 *    tmpw - temporary width holding the width of the last UTF-8 character checked
	 *    ew - represents the extent width, as in width of a UTF-8 character
	 *    d - holds the XftDraw structure used in the event that text is to be drawn
	 *    usedfont - the currently used font
	 *    curfont - the font that was found to hold a glyph for the given UTF-8 character
	 *    nextfont - the next font to use
	 *    utf8strlen - the number of continuous bytes that can be drawn together with the used font
	 *    utf8charlen - the number of bytes in the current multi-byte UTF-8 character
	 *    render - whether to render text or just calculate the width of the text
	 *    utf8codepoint - represents the code point for the current UTF-8 character
	 *    utf8str -
	 *    fccharset - used in the context of loading additional fallback fonts
	 *    fcpattern - used in the context of loading additional fallback fonts
	 *    match - used in the context of loading additional fallback fonts
	 *    result - used in the context of loading additional fallback fonts
	 *    charexists - internal flag indicating whether a character exists within the current fonts
	 *    overflow - internal flag indicating whether the text is too long to fit in the given width
	 */
	int i, ty, ellipsis_x = 0;
	unsigned int tmpw, ew, ellipsis_w = 0, ellipsis_len;
	XftDraw *d = NULL;
	Fnt *usedfont, *curfont, *nextfont;
	int utf8strlen, utf8charlen, render = x || y || w || h;
	long utf8codepoint = 0;
	const char *utf8str;
	FcCharSet *fccharset;
	FcPattern *fcpattern;
	FcPattern *match;
	XftResult result;
	int charexists = 0, overflow = 0;
	/* Keep track of a couple codepoints for which we have no match. This is a performance
	 * optimisation to avoid spending wasteful time searching for a font that does not exit over
	 * and over again. Here we reserve space to hold up to 64 known code points (characters) for
	 * which we know there is no font that has a glyph for that character. */
	enum { nomatches_len = 64 };
	static struct { long codepoint[nomatches_len]; unsigned int idx; } nomatches;
	/* The actual width of the ellipsis. This is static so that we will only ever have to calculate
	 * the width of the "..." string more than once. */
	static unsigned int ellipsis_width = 0;

	/* General guard to prevent anything bad from happening in the event that this function is
	 * called before we have everything we need set up (like colour schemes, fonts, etc.). */
	if (!drw || (render && (!drw->scheme || !w)) || !text || !drw->fonts)
		return 0;

	/* If we are only calculating the width of the text then do not impose any limit on the width,
	 * unless this is called from drw_fontset_getwidth_clamp which passes in a maximum width via
	 * the invert variable. In this context the meaning of the invert parameter is overloaded in
	 * the sense that it is used for something other than what it is intended for, but the invert
	 * variable is otherwise not used when calculating the width of a given text and the
	 * overloading is limited to the one line below. */
	if (!render) {
		w = invert ? invert : ~invert;
	} else {
		/* If we are rendering the text then the first thing we do is to set the foreground color
		 * and drawing a rectangle covering the width of the text. This is to clear anything that
		 * may have been drawn before. */
		XSetForeground(drw->dpy, drw->gc, drw->scheme[invert ? ColFg : ColBg].pixel);
		XFillRectangle(drw->dpy, drw->drawable, drw->gc, x, y, w, h);
		/* We prepare the XftDraw structure that will be used to draw the text later. */
		d = XftDrawCreate(drw->dpy, drw->drawable,
		                  DefaultVisual(drw->dpy, drw->screen),
		                  DefaultColormap(drw->dpy, drw->screen));
		/* Apply the left padding to the starting position of the text. Reduce the width
		 * accordingly. */
		x += lpad;
		w -= lpad;
	}

	/* Start with the primary font. */
	usedfont = drw->fonts;
	/* If this is the first time we are actually drawing text, then ellipsis_width will be 0 and
	 * we call drw_fontset_getwidth to get the actual width of the ellipsis ("...") text. The
	 * ellipsis_width variable is static which means that it will keep this value for all future
	 * calls to this function, which in turn means that the width of the ellipsis will only be
	 * calculated once for the as long as the program runs. */
	if (!ellipsis_width && render)
		ellipsis_width = drw_fontset_getwidth(drw, "...");

	/* Keep doing the below until we run out of text or we run out of space to draw the text. */
	while (1) {
		/* This is the first loop or we have previously drawn a character or a set of characters.
		 * reset the various with variables back to 0. */
		ew = ellipsis_len = utf8strlen = 0;
		/* We set utf8str to be the text (or what is left of it). This holds a reference to where
		 * we are in the text string before we enter the while loop below. This is later used when
		 * drawing the text after we have exited the while loop which traverses through the text
		 * array. */
		utf8str = text;
		nextfont = NULL;

		/* Keep doing this as long as we have text or we break out of the loop due to overflowing,
		 * the character does not exist or we need to switch to another font. */
		while (*text) {
			/* We call utf8decode to work out how many bytes the multi-byte UTF-8 character at the
			 * current position holds. The variable could have been named utf8bytelen, but it is
			 * named as it is due to that text is stored in an array of char and a char in c holds
			 * the same amount of data as one byte. The call to utf8decode also gives us the
			 * utf8codepoint which represents the actual Unicode character. */
			utf8charlen = utf8decode(text, &utf8codepoint, UTF_SIZ);
			/* Now we loop through all fonts, starting at the primary font. Notably this means that
			 * the primary font is always going to take precedence over fallback fonts provided that
			 * the primary font has a glyph for the current character. */
			for (curfont = drw->fonts; curfont; curfont = curfont->next) {
				/* Then we check if the current font has a glyph for that code point. If the glyph
				 * does not exist then we check the next font. It is possible that none of the
				 * loaded fonts has the specific glyph, in which case nextfont will remain with
				 * the value of NULL. */
				charexists = charexists || XftCharExists(drw->dpy, curfont->xfont, utf8codepoint);

				/* If the current font has the glyph then */
				if (charexists) {
					/* Call drw_font_getexts to get the width of the character (in pixels) when
					 * drawn using the current font (at the size the font was loaded with). Store
					 * the result in the temporary width variable (tmpw). */
					drw_font_getexts(curfont, text, utf8charlen, &tmpw, NULL);

					/* This ellipsis check stores the position, width and byte length of the
					 * previous character that was processed. This is in order to know where the
					 * ellipsis should be drawn in the event that the text is too long to fit in
					 * the available width. The extent width (ew) variable is set further down for
					 * the current character. This should make more sense after having read through
					 * the whole for loop at least once. */
					if (ew + ellipsis_width <= w) {
						/* Keep track where the ellipsis still fits. */
						ellipsis_x = x + ew;
						ellipsis_w = w - ew;
						ellipsis_len = utf8strlen;
					}

					/* If the addition of the current UTF-8 character would result in overflow, as
					 * in taking more space than we have available to us, then ... */
					if (ew + tmpw > w) {
						/* ... we set the overflow flag to 1 so that we break out of this and the
						 * outer while loop. */
						overflow = 1;
						/* If we are not rendering anything then this drw_text call must have been
						 * made from drw_fontset_getwidth_clamp which is interested in the width
						 * after the overflow. As in how much more space does this take. As such
						 * we increment the x position with the width of the current character as
						 * well. */
						if (!render)
							x += tmpw;
						/* Otherwise revert utf8strlen back to the last character where the
						 * ellipsis would still fit. */
						else
							utf8strlen = ellipsis_len;
					} else if (curfont == usedfont) {
						/* If the found font is the same as what we used for the previous character
						 * then we can draw these consecutively. Just add the values to the running
						 * total of number of characters that can be drawn and the combined extent
						 * width. */
						utf8strlen += utf8charlen;
						/* This moves the text cursor to the next UTF-8 character by moving forward
						 * the number of bytes the current character has. */
						text += utf8charlen;
						ew += tmpw;
					} else {
						/* Oh, we are in a situation where the next UTF-8 character to be drawn is
						 * for another font. Before we can do that though we need to draw the text
						 * we have already processed that we know use the same font. We set the
						 * nextfont to the current font so that we can switch to that after having
						 * drawn the text up until this point. */
						nextfont = curfont;
					}
					/* In any event we have found the font that has the desired glyph, so we do not
					 * need to loop through any more fonts. Thus we break here. */
					break;
				}
			}

			/* If we have exceeded the available width (overflow), or the character does not exist
			 * in any of the fonts loaded, or in the case that we need to change to another font
			 * then we need to break out of the while loop. */
			if (overflow || !charexists || nextfont)
				break;
			/* Otherwise we set charexists to 0. */
			else
				charexists = 0;
		}

		/* If we have text to draw then */
		if (utf8strlen) {
			/* and if we are actually drawing the text then */
			if (render) {
				/* Calculate the text y position relative to the used font. When using fallback
				 * fonts then these may have a different size than the primary font. The
				 * calculation aims to center the font text in the middle of the defined height
				 * for the text. */
				ty = y + (h - usedfont->h) / 2 + usedfont->xfont->ascent;
				/* This is the bit that actually draws text up until this point using the currently
				 * used font. It will use the current scheme's foreground colour for the text unless
				 * invert is true, in which case the background colour is used for the text. */
				XftDrawStringUtf8(d, &drw->scheme[invert ? ColBg : ColFg],
				                  usedfont->xfont, x, ty, (XftChar8 *)utf8str, utf8strlen);
			}
			/* Move the x position (or cursor) up the width of the text drawn. Reduce the remaining
			 * width accordingly. */
			x += ew;
			w -= ew;
		}

		/* If we are drawing text and we have run out of space, then draw the ellipsis at the last
		 * known position where we know that the ellipsis will still fit. The ellipsis_w variable
		 * may be larger than the ellipsis_width variable, this is because we will want to make
		 * sure that the entire remaining area of text is cleared as well. */
		if (render && overflow)
			drw_text(drw, ellipsis_x, y, ellipsis_w, h, 0, "...", invert);

		/* If we have run out of text or if we have run out of space to draw the text, then break
		 * out of the main while loop, finishing the drw_text call. */
		if (!*text || overflow) {
			break;
		/* Otherwise if we have a next font to use then we set usedfont to be the next font. We
		 * also make sure to set charexists to 0 so that we don't skip the call to XftCharExists
		 * when entering the for loop that goes through all the fonts again. */
		} else if (nextfont) {
			charexists = 0;
			usedfont = nextfont;
		} else {
			/* Regardless of whether or not a fallback font is found, the
			 * character must be drawn. */
			charexists = 1;

			/* Here we go through known cases of code points that have no matching fonts. This is
			 * a performance optimisation that voids the need to waste time searching for a font
			 * that offers a certain glyph and there are no such font in the system. */
			for (i = 0; i < nomatches_len; ++i) {
				/* avoid calling XftFontMatch if we know we won't find a match */
				if (utf8codepoint == nomatches.codepoint[i])
					/* Here we have a goto statement that skips most of the code below. */
					goto no_match;
			}

			/* Here we create a character set and add our code point to that character set.
			 * We then use this to make the Xft library search for a font that matches the
			 * criteria that we set up. Documentation on fontconfig is relatively scarce, but
			 * most if not all Fc* functions should have their respective man pages. */
			fccharset = FcCharSetCreate();
			FcCharSetAddChar(fccharset, utf8codepoint);

			if (!drw->fonts->pattern) {
				/* Refer to the comment in xfont_create for more information. */
				die("the first font in the cache must be loaded from a font string.");
			}

			fcpattern = FcPatternDuplicate(drw->fonts->pattern);
			FcPatternAddCharSet(fcpattern, FC_CHARSET, fccharset);
			FcPatternAddBool(fcpattern, FC_SCALABLE, FcTrue);

			FcConfigSubstitute(NULL, fcpattern, FcMatchPattern);
			FcDefaultSubstitute(fcpattern);

			/* The matching font, if any. */
			match = XftFontMatch(drw->dpy, drw->screen, fcpattern, &result);

			/* Cleanup, free the character set and fontconfig pattern. */
			FcCharSetDestroy(fccharset);
			FcPatternDestroy(fcpattern);

			/* If we did find a matching font then */
			if (match) {
				/* Set the used font to be this new font. */
				usedfont = xfont_create(drw, NULL, match);
				/* Sanity check that the character does exist for this newly loaded font. */
				if (usedfont && XftCharExists(drw->dpy, usedfont->xfont, utf8codepoint)) {
					/* If the character does exist in the newly loaded font then all good.
					 * Here we loop through and append the newly loaded font at the end of the
					 * linked list of loaded fonts. */
					for (curfont = drw->fonts; curfont->next; curfont = curfont->next)
						; /* NOP */
					curfont->next = usedfont;
				} else {
					/* In the unfortunate event that the matching font does not have a glyph for
					 * the given code point then it means that there are no fonts in the system
					 * that has that glyph. Free and unload the font found and record add the bad
					 * code point to the nomatches array so that we don't waste more time searching
					 * for this character. */
					xfont_free(usedfont);
					nomatches.codepoint[++nomatches.idx % nomatches_len] = utf8codepoint;
no_match:
					/* Set the used font back to the primary font. */
					usedfont = drw->fonts;
				}
			}
		}
	}
	/* If we are drawing the text then we have finished by now so we clean up after ourselves by
	 * freeing our XftDraw structure. */
	if (d)
		XftDrawDestroy(d);

	/* Finally we return the x position following the drawn text, or just x in the event that we
	 * are only after the text width. The w here represents the remaining space. */
	return x + (render ? w : 0);
}

/* This copies graphics from the drawable and places that on the designated window.
 *
 * @called_from drawbar to copy the bar graphics to the bar window
 * @calls XCopyArea https://tronche.com/gui/x/xlib/graphics/XCopyArea.html
 * @calls XSync https://tronche.com/gui/x/xlib/event-handling/XSync.html
 *
 * Internal call stack:
 *    ~ -> drawbar -> drw_map
 */
void
drw_map(Drw *drw, Window win, int x, int y, unsigned int w, unsigned int h)
{
	if (!drw)
		return;

	/* This copies the graphics drawn on the drawable too the designated window (in this case the
	 * bar window). */
	XCopyArea(drw->dpy, drw->drawable, win, drw->gc, x, y, w, h, x, y);
	/* This flushes the output buffer and then waits until all requests have been
	 * received and processed by the X server. */
	XSync(drw->dpy, False);
}

/* This copies graphics from the drawable and places that on the designated window.
 *
 * @called_from TEXTW macro to get the width of a given text string
 * @called_from drawbar via TEXTW macro
 * @called_from buttonpress via TEXTW macro
 * @calls XCopyArea https://tronche.com/gui/x/xlib/graphics/XCopyArea.html
 * @calls XSync https://tronche.com/gui/x/xlib/event-handling/XSync.html
 *
 * Internal call stack:
 *    ~ -> drawbar -> drw_fontset_getwidth
 *    run -> buttonpress -> drw_fontset_getwidth
 */
unsigned int
drw_fontset_getwidth(Drw *drw, const char *text)
{
	/* If we have no drawable, have no fonts or the text is NULL then bail. This is
	 * just a general guard that should never happen in practice. */
	if (!drw || !drw->fonts || !text)
		return 0;
	/* Calls drw_text with parameters indicating that we are only interested in the
	 * width of the text and that no effort should be done drawing the text. */
	return drw_text(drw, 0, 0, 0, 0, 0, text, 0);
}

/* Helper function in the context of finding the longest of a set of strings. This finds the next
 * size when the text is too large to fit in the given width. This is primarily designed for dmenu
 * which shares the same drw code.
 *
 * @calls drw_text to get the width of a given text string after overflow
 *
 * This function is currently not used within dwm.
 */
unsigned int
drw_fontset_getwidth_clamp(Drw *drw, const char *text, unsigned int n)
{
	unsigned int tmp = 0;
	if (drw && drw->fonts && text && n)
		tmp = drw_text(drw, 0, 0, 0, 0, 0, text, n);
	return MIN(n, tmp);
}

/* Wrapper function for XftTextExtentsUtf8 to get the text width for a string of characters using
 * a particular font.
 *
 * @called_from drw_text to get the width of a single character for a given font
 * @calls XftTextExtentsUtf8 https://linux.die.net/man/3/xft
 *
 * Internal call stack:
 *    ~ -> drawbar -> drw_text -> drw_font_getexts
 *    ~ -> drawbar -> drw_fontset_getwidth -> drw_text -> drw_font_getexts
 *    run -> buttonpress -> drw_fontset_getwidth -> drw_text -> drw_font_getexts
 */
void
drw_font_getexts(Fnt *font, const char *text, unsigned int len, unsigned int *w, unsigned int *h)
{
	XGlyphInfo ext;

	/* General guard to protect against NULL values. Should not happen in practice. */
	if (!font || !text)
		return;

	/* This call gets the width of the given text reading len bytes. */
	XftTextExtentsUtf8(font->dpy, font->xfont, (XftChar8 *)text, len, &ext);
	/* The x offset of the extent tells the width of the text usign the given font. */
	if (w)
		*w = ext.xOff;
	/* The height of the text is always the font height. */
	if (h)
		*h = font->h;
}

/* Function to create and return a given font cursor.
 *
 * @called_from setup to create various mouse cursors
 * @calls XCreateFontCursor https://tronche.com/gui/x/xlib/pixmap-and-cursor/XCreateFontCursor.html
 * @calls ecalloc to allocate memory for the given cursor
 * @see https://tronche.com/gui/x/xlib/appendix/b/
 *
 * Internal call stack:
 *    main -> setup -> drw_cur_create
 */
Cur *
drw_cur_create(Drw *drw, int shape)
{
	Cur *cur;

	if (!drw || !(cur = ecalloc(1, sizeof(Cur))))
		return NULL;

	cur->cursor = XCreateFontCursor(drw->dpy, shape);

	return cur;
}

/* Function to free a given font cursor.
 *
 * @called_from cleanup to free memory before exiting the program
 * @calls XFreeCursor https://tronche.com/gui/x/xlib/pixmap-and-cursor/XFreeCursor.html
 * @calls free to release the memory used by the given cursor
 *
 * Internal call stack:
 *    main -> cleanup -> drw_cur_free
 */
void
drw_cur_free(Drw *drw, Cur *cursor)
{
	if (!cursor)
		return;

	XFreeCursor(drw->dpy, cursor->cursor);
	free(cursor);
}
