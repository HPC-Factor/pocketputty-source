/*
 * windefs.c: default settings that are specific to Windows.
 */

#include "putty.h"

#include <commctrl.h>

FontSpec platform_default_fontspec(const TCHAR *name)
{
    FontSpec ret;
    if (!_tcscmp(name, _T("Font"))) {
#ifdef _WIN32_WCE
	_tcscpy(ret.name, _T("Courier New"));
	ret.isbold = 0;
	ret.charset = ANSI_CHARSET;
	ret.height = 8;
#else
	_tcscpy(ret.name, _T("Courier New"));
	ret.isbold = 0;
	ret.charset = ANSI_CHARSET;
	ret.height = 10;
#endif
    } else {
	ret.name[0] = _T('\0');
    }
    return ret;
}

Filename platform_default_filename(const TCHAR *name)
{
    Filename ret;
    if (!_tcscmp(name, _T("LogFileName")))
#ifdef _WIN32_WCE
	strcpy(ret.path, "\\putty.log");
#else
	strcpy(ret.path, "putty.log");
#endif
    else
	*ret.path = '\0';
    return ret;
}

char *platform_default_s(const TCHAR *name)
{
    return NULL;
}

wchar_t *platform_default_wcs(const TCHAR *name) {
    return (NULL);
}

int platform_default_i(const TCHAR *name, int def)
{
    return def;
}
