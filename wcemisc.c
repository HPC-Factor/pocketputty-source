#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

#if !(UNDER_CE > 0 && UNDER_CE < 400)
#include <assert.h>
#else
#include "wceassrt.h"
#endif

#include "putty.h"
#include "wcemisc.h"

/* ----------------------------------------------------------------------
 * String handling routines.
 */


wchar_t *dupwcs(const wchar_t *s)
{
    wchar_t *p = NULL;
    if (s) {
        int len = wcslen(s);
        p = snewn(len + 1, wchar_t);
        wcscpy(p, s);
    }
    return p;
}

/* Allocate the concatenation of N strings. Terminate arg list with NULL. */
wchar_t *dupwcat(const wchar_t *s1, ...)
{
    int len;
    wchar_t *p, *q, *sn;
    va_list ap;

    len = wcslen(s1);
    va_start(ap, s1);
    while (1) {
	sn = va_arg(ap, wchar_t *);
	if (!sn)
	    break;
	len += wcslen(sn);
    }
    va_end(ap);

    p = snewn(len + 1, wchar_t);
    wcscpy(p, s1);
    q = p + wcslen(p);

    va_start(ap, s1);
    while (1) {
	sn = va_arg(ap, wchar_t *);
	if (!sn)
	    break;
	_tcscpy(q, sn);
	q += wcslen(q);
    }
    va_end(ap);

    return p;
}

/*
 * Do an sprintf(), but into a custom-allocated buffer.
 * 
 * Irritatingly, we don't seem to be able to do this portably using
 * vsnprintf(), because there appear to be issues with re-using the
 * same va_list for two calls, and the excellent C99 va_copy is not
 * yet widespread. Bah. Instead I'm going to do a horrid, horrid
 * hack, in which I trawl the format string myself, work out the
 * maximum length of each format component, and resize the buffer
 * before printing it.
 */
wchar_t *dupwprintf(const wchar_t *fmt, ...)
{
    wchar_t *ret;
    va_list ap;
    va_start(ap, fmt);
    ret = dupvwprintf(fmt, ap);
    va_end(ap);
    return ret;
}
wchar_t *dupvwprintf(const wchar_t *fmt, va_list ap)
{
    wchar_t *buf;
    int len, size;

    buf = snewn(512, wchar_t);
    size = 512;

    while (1) {
//ifdef _WINDOWS
//efine _vsntprintf _vsnprintf
//elif defined(_WIN32_WCE)
//define _vsntprintf _vsnwprintf
//endif
	len = _vsnwprintf(buf, size, fmt, ap);
	if (len >= 0 && len < size) {
	    /* This is the C99-specified criterion for snprintf to have
	     * been completely successful. */
	    return buf;
	} else if (len > 0) {
	    /* This is the C99 error condition: the returned length is
	     * the required buffer size not counting the NUL. */
	    size = len + 1;
	} else {
	    /* This is the pre-C99 glibc error condition: <0 means the
	     * buffer wasn't big enough, so we enlarge it a bit and hope. */
	    size += 512;
	}
	buf = sresize(buf, size, wchar_t);
    }
}

/* ----------------------------------------------------------------------
 * Base64 encoding routine. This is required in public-key writing
 * but also in HTTP proxy handling, so it's centralised here.
 */

void tbase64_encode_atom(unsigned char *data, int n, TCHAR *out)
{
    static const TCHAR base64_chars[] =
	_T("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");

    unsigned word;

    word = data[0] << 16;
    if (n > 1)
	word |= data[1] << 8;
    if (n > 2)
	word |= data[2];
    out[0] = base64_chars[(word >> 18) & 0x3F];
    out[1] = base64_chars[(word >> 12) & 0x3F];
    if (n > 1)
	out[2] = base64_chars[(word >> 6) & 0x3F];
    else
	out[2] = '=';
    if (n > 2)
	out[3] = base64_chars[word & 0x3F];
    else
	out[3] = '=';
}


/* ----------------------------------------------------------------------
 * Debugging routines.
 */

#ifdef DEBUG

static FILE *debug_fp = NULL;
static HANDLE debug_hdl = INVALID_HANDLE_VALUE;
static int debug_got_console = 0;

void dputwcs(TCHAR *buf)
{
    DWORD dw;

    if (!debug_got_console) {
	if (AllocConsole()) {
	    debug_got_console = 1;
	    debug_hdl = GetStdHandle(STD_OUTPUT_HANDLE);
	}
    }
    if (!debug_fp) {
#ifdef _IPAQ
	debug_fp = fopen("\\My Documents\\debug.log", "w");
#else
	debug_fp = fopen("\\Storage Card\\debug.log", "w");
#endif
    }

    if (debug_hdl != INVALID_HANDLE_VALUE) {
	WriteFile(debug_hdl, buf, _tcslen(buf) * sizeof(TCHAR), &dw, NULL);
    }
    _fputts(buf, debug_fp);
    fflush(debug_fp);
}

void debug_wprintf(wchar_t *fmt, ...)
{
    wchar_t *buf;
    va_list ap;

    va_start(ap, fmt);
    buf = dupvwprintf(fmt, ap);
    dputwcs(buf);
    sfree(buf);
    va_end(ap);
}


#endif				/* def DEBUG */


Filename filename_from_wcs(const wchar_t *str) {
    Filename result;
    char *astr = unicode2ansi(str);
    result = filename_from_str(astr);
    sfree(astr);
    return (result);
}
