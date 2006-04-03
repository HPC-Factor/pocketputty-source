#ifndef PUTTY_MISC_H
#define PUTTY_MISC_H

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

char *dupstr(const char *s);
char *dupcat(const char *s1, ...);
char *dupprintf(const char *fmt, ...);
char *dupvprintf(const char *fmt, va_list ap);

void base64_encode_atom(unsigned char *data, int n, char *out);

struct bufchain_granule;
typedef struct bufchain_tag {
    struct bufchain_granule *head, *tail;
    int buffersize;		       /* current amount of buffered data */
} bufchain;

void bufchain_init(bufchain *ch);
void bufchain_clear(bufchain *ch);
int bufchain_size(bufchain *ch);
void bufchain_add(bufchain *ch, const void *data, int len);
void bufchain_prefix(bufchain *ch, void **data, int *len);
void bufchain_consume(bufchain *ch, int len);
void bufchain_fetch(bufchain *ch, void *data, int len);

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
void debug_printf(char *fmt, ...);
void debug_wprintf(wchar_t *frm,...);

#ifdef _UNICODE
#define _debug_tprintf(x) debug_wprintf x
#else
#define _debug_tprintf(x) debug_printf x
#endif

void debug_memdump(void *buf, int len, int L);
//Aleq #define debug(x) (debug_printf x)
#define RDB_debug(x, y) (_debug_tprintf (y))
#define debug(x) (_debug_tprintf (x))
#define dmemdump(buf,len) debug_memdump (buf, len, 0);
#define dmemdumpl(buf,len) debug_memdump (buf, len, 1);
#else
#define debug(x)
#define dmemdump(buf,len)
#define dmemdumpl(buf,len)
#define _debug_tprintf(x) 
#endif

#ifndef lenof
#define lenof(x) ( (sizeof((x))) / (sizeof(*(x))))
#endif

#ifndef min
#define min(x,y) ( (x) < (y) ? (x) : (y) )
#endif
#ifndef max
#define max(x,y) ( (x) > (y) ? (x) : (y) )
#endif

#ifdef _UNICODE
#define _tchar(x) MAKEWORD(x, 0)
#else
#define _tchar(x) x
#endif


#endif
