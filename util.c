/* See LICENSE file for copyright and license details. */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The util library comes from libsl which contains some functionality that is common for several
 * suckless project, e.g. dmenu and dwm.
 *
 * @see https://git.suckless.org/libsl/files.html
 */

#include "util.h"

/* Helper function that prints an error before exiting the process.
 *
 * @called_from ecalloc in case of error
 * @called_from sigchld in case SIGCHLD handler could not be installed
 * @called_from main in the event of unexpected arguments
 * @called_from xerrorstart in case another window manager is running
 * @calls va_start
 * @calls vfprintf
 * @calls va_end
 * @calls strlen to get the length of the error string
 * @calls perror to print the description of the error that occurred
 * @calls fputc to write a character to standard err
 * @calls exit to stop the process
 * @see https://www.tutorialspoint.com/c_standard_library/c_macro_va_start.htm
 *
 * Internal call stack:
 *    main -> setup -> drw_fontset_create -> xfont_create
 *    main -> setup -> drw_scm_create -> drw_clr_create -> die
 *    main -> setup -> ecalloc -> die
 *    main -> setup -> updategeom -> ecalloc -> die
 *    main -> setup -> updategeom -> createmon -> ecalloc -> die
 *    main -> setup -> sigchld -> die
 *    main -> die
 *    run -> buttonpress -> drw_fontset_getwidth -> drw_text -> die
 *    run -> buttonpress -> spawn -> die
 *    run -> configurenotify -> updategeom -> ecalloc -> die
 *    run -> configurenotify -> updategeom -> createmon -> ecalloc -> die
 *    run -> keypress -> spawn -> die
 *    run -> maprequest -> manage -> ecalloc -> die
 *    run -> scan -> manage -> ecalloc -> die
 *    xerrorstart -> die
 *    ~ -> drawbar -> drw_text -> die
 *    ~ -> drawbar -> drw_fontset_getwidth -> drw_text -> die
 */
void
die(const char *fmt, ...)
{
	va_list ap;

	/* Note how the function ends with , ... - this means that the function takes variable
	 * arguments. Have a look at the tutorial on these macros for more information, but the gist
	 * of it is that it allows for calls to die on this form:
	 *
	 *    die("error, cannot allocate color '%s'", clrname);
	 *
	 * where the %s is substituted for the value of the clrname variable.
	 */
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	/* If the error string ends with a colon then print the error that happened as well. */
	if (fmt[0] && fmt[strlen(fmt)-1] == ':') {
		fputc(' ', stderr);
		/* The perror function displays the description of the error that corresponds to an error
		 * code stored in the system variable errno. */
		perror(NULL);
	} else {
		fputc('\n', stderr);
	}

	/* Stop the process passing 1 to exit to signify failure. */
	exit(1);
}

/* Memory allocation wrapper around calloc that calls die in the event that memory could not be
 * allocated.
 *
 * @called_from drw_cur_create to allocate memory for a cursor
 * @called_from drw_scm_create to allocate memory for the colour scheme
 * @called_from drw_create to allocate memory for the drawable
 * @called_from setup to allocate memory for colour schemes
 * @called_from updategeom to allocate memory to hold unique screen info
 * @called_from createmon to allocate memory for new Monitor structures
 * @called_from manage to allocate memory for new Client structures
 * @calls calloc to allocate memory
 * @calls die in the event that memory could not be allocated
 *
 * Internal call stack:
 *    main -> setup -> drw_cur_create -> ecalloc
 *    main -> setup -> drw_fontset_create -> xfont_create -> ecalloc
 *    main -> setup -> drw_scm_create -> ecalloc
 *    main -> setup -> drw_create -> ecalloc
 *    main -> setup -> ecalloc
 *    main -> setup -> updategeom -> ecalloc
 *    main -> setup -> updategeom -> createmon -> ecalloc
 *    run -> configurenotify -> updategeom -> ecalloc
 *    run -> configurenotify -> updategeom -> createmon -> ecalloc
 *    run -> maprequest -> manage -> ecalloc
 *    run -> scan -> manage -> ecalloc
 */
void *
ecalloc(size_t nmemb, size_t size)
{
    void *p;

    /* If memory could not be allocated then call die to exit the window manager. */
    if (!(p = calloc(nmemb, size)))
        die("calloc:");
    return p;
}
