/* See LICENSE file for copyright and license details. */

/* This is an internal structure that holds a single cursor. */
typedef struct {
	Cursor cursor;
} Cur;

/* This is an internal structure that represents a font. */
typedef struct Fnt {
	/* The display. */
	Display *dpy;
	/* The font height. */
	unsigned int h;
	/* The actual font. */
	XftFont *xfont;
	/* The fontconfig pattern, used when searching for fonts. */
	FcPattern *pattern;
	/* The next font in the linked list. */
	struct Fnt *next;
} Fnt;

/* This represents the columns for colour schemes.
 *
 * E.g. as defined in the dwm configuration file:
 *
 *    static const char *colors[][3]      = {
 *       //               fg         bg         border
 *       [SchemeNorm] = { col_gray3, col_gray1, col_gray2 },
 *       [SchemeSel]  = { col_gray4, col_cyan,  col_cyan  },
 *    };
 */
enum { ColFg, ColBg, ColBorder }; /* Clr scheme index */
typedef XftColor Clr;

/* This is an internal structure representing the drawable, used when drawing the bar. */
typedef struct {
	/* The width and height of the drawable. */
	unsigned int w, h;
	/* The display. */
	Display *dpy;
	/* The screen as returned by DefaultScreen for the given display. */
	int screen;
	/* The root window. */
	Window root;
	/* The drawable pixel map. */
	Drawable drawable;
	/* The graphics context that handles colours. */
	GC gc;
	/* The currently used colour scheme. */
	Clr *scheme;
	/* A linked list of loaded fonts. */
	Fnt *fonts;
} Drw;

/* Drawable abstraction */
Drw *drw_create(Display *dpy, int screen, Window win, unsigned int w, unsigned int h);
void drw_resize(Drw *drw, unsigned int w, unsigned int h);
void drw_free(Drw *drw);

/* Fnt abstraction */
Fnt *drw_fontset_create(Drw* drw, const char *fonts[], size_t fontcount);
void drw_fontset_free(Fnt* set);
unsigned int drw_fontset_getwidth(Drw *drw, const char *text);
unsigned int drw_fontset_getwidth_clamp(Drw *drw, const char *text, unsigned int n);
void drw_font_getexts(Fnt *font, const char *text, unsigned int len, unsigned int *w, unsigned int *h);

/* Colorscheme abstraction */
void drw_clr_create(Drw *drw, Clr *dest, const char *clrname);
Clr *drw_scm_create(Drw *drw, const char *clrnames[], size_t clrcount);

/* Cursor abstraction */
Cur *drw_cur_create(Drw *drw, int shape);
void drw_cur_free(Drw *drw, Cur *cursor);

/* Drawing context manipulation */
void drw_setfontset(Drw *drw, Fnt *set);
void drw_setscheme(Drw *drw, Clr *scm);

/* Drawing functions */
void drw_rect(Drw *drw, int x, int y, unsigned int w, unsigned int h, int filled, int invert);
int drw_text(Drw *drw, int x, int y, unsigned int w, unsigned int h, unsigned int lpad, const char *text, int invert);

/* Map functions */
void drw_map(Drw *drw, Window win, int x, int y, unsigned int w, unsigned int h);
