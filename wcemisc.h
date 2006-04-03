#ifndef PUTTY_WCEMISC_H
#define PUTTY_WCEMISC_H

#include "puttymem.h"

#include <stdarg.h>		       /* for va_list */

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

typedef struct Filename Filename;
typedef struct FontSpec FontSpec;

wchar_t *dupwcs(const wchar_t *s);
wchar_t *dupwcat(const wchar_t *s1, ...);
wchar_t *dupwprintf(const wchar_t *fmt, ...);
wchar_t *dupvwprintf(const wchar_t *fmt, va_list ap);

void tbase64_encode_atom(unsigned char *data, int n, TCHAR *out);

/*
 * Debugging functions.
 *
 * Output goes to debug.log
 *
 * debug(()) (note the double brackets) is like printf().
 *
 * dmemdump() and dmemdumpl() both do memory dumps.  The difference
 * is that dmemdumpl() is more suited for when where the memory is is
 * important (say because you'll be recording pointer values later
 * on).  dmemdump() is more concise.
 */

#ifdef DEBUG
void debug_wprintf(wchar_t *fmt, ...);
#endif


#ifdef DEBUG
#define tdebug(x) (debug_tprintf x)
#else
#define tdebug(x)
#endif







Filename filename_from_wcs(const wchar_t *str);

#define strnicmp _strnicmp


#endif


/*
 * Miscellaneous exports from the platform-specific code.
 */
 

 /*
Filename filename_from_str(const char *string);

#ifdef _WIN32_WCE
Filename filename_from_tcs(const TCHAR *string);
#endif

const char *filename_to_str(const Filename *fn);
int filename_equal(Filename f1, Filename f2);
int filename_is_null(Filename fn);
char *get_username(void);	       /* return value needs freeing */
