/* See LICENSE file for copyright and license details. */

/* Macro that chooses the maximum of two values. If used with a function call then that call may
 * be called twice due to how the macro unfolds in the code. */
#define MAX(A, B)               ((A) > (B) ? (A) : (B))
/* Macro that chooses the minimum of two values. If used with a function call then that call may
 * be called twice due to how the macro unfolds in the code. */
#define MIN(A, B)               ((A) < (B) ? (A) : (B))
/* Macro that is true if a value is between two values. If used with a function call then that
 * call will happen twice due to how the macro unfolds in the code. */
#define BETWEEN(X, A, B)        ((A) <= (X) && (X) <= (B))

/* Function declarations. */
void die(const char *fmt, ...);
void *ecalloc(size_t nmemb, size_t size);
