/*
 * Porting support in PuTTY
 *
 * Functions and defines for porting - unicode TCHAR defines, etc.
 *
 * (c) 2004, PocketPuTTY project, Ales Berka
 */

#ifndef PUTTY_PORTING_H
#define PUTTY_PORTING_H
/* *************************************************************************************** */

/*
 * TCHAR defines
 */

#ifdef _UNICODE
/* 
 * Unicode strings
 *
 */
#define _logtevent logwevent
#define _sk_twrite(s,buf,len) { char *abuf = snewn(len, char); wcstombs(abuf, buf, len); sk_write(s, abuf, len); sfree(abuf); }
#define _write_setting_ts write_setting_wcs
#define _read_setting_ts read_setting_wcs
#define _gppts gppwcs
#define _platform_default_ts platform_default_wcs

// wcemisc.h
#define _duptcs dupwcs
#define _duptcat dupwcat
#define _duptprintf dupwprintf
#define _dupvtprintf dupvwprintf
#define filename_from_tcs filename_from_wcs

	#ifdef DEBUG
		#define _dputts dputwcs
	#else
	#endif

#define _tcs2str(x) unicode2ansi(x)
#define _str2tcs(x) ansi2unicode(x)

#define _tsfree(x) sfree(x)
#define _strncpyt wcstombs
#define _unsigned_tchar wchar_t
#define _mungetcs mungewcs
#define _unmungetcs unmungewcs

#else
/*
 *	ANSI strings
 *
 */
#define _logtevent logevent
#define _sk_twrite(s,buf,len) sk_twrite(s,buf,len)
#define _write_setting_ts write_setting_s
#define _read_setting_ts read_setting_s
#define _gppts gpps
#define _platform_default_ts platform_default_s

// misc.h
#define _duptcs dupstr
#define _duptcat dupcat
#define _duptprintf dupprintf
#define _dupvtprintf dupvprintf

#define filename_from_tcs filename_from_str

	#ifdef DEBUG
		#define _dputts dputs
	#else
	#endif

#define _tcs2str(x) (x)
#define _str2tcs(x) (x)

#define _tsfree(x)
#define _strncpyt strncpy

#define _unsigned_tchar unsigned char
#define _mungetcs mungestr
#define _unmungetcs unmungestr

#endif


void logwevent(void *frontend, const wchar_t *string);
// TODO: logevent predelat na unicode

void debug_wprintf(wchar_t *fmt, ...);
Filename filename_from_wcs(const wchar_t *str);

/*
 *	GUI/Registry/util wrappers
 */

BOOL TextOut(HDC hdc, int nXStart, int nYStart, LPCWSTR lpwString, int wcbString);
HFONT CreateFont(int nHeight, int nWidth, int nEscapement, int nOrientation, int fnWeight,
  DWORD fdwItalic, DWORD fdwUnderline, DWORD fdwStrikeOut, DWORD fdwCharSet, 
  DWORD fdwOutputPrecision, DWORD fdwClipPrecision, DWORD fdwQuality, DWORD fdwPitchAndFamily,
  LPCTSTR lpszFace);
BOOL GetKeyboardState(PBYTE lpKeyState);
int ToAsciiEx(UINT uVirtKey, UINT uScanCode, PBYTE lpKeyState, LPWORD lpChar, UINT uFlags, HKL dwhkl);
LONG RegCreateKey(HKEY hKey, LPCTSTR lpSubKey, PHKEY phkResult);
LONG RegOpenKey(HKEY hKey, LPCTSTR lpSubKey, PHKEY phkResult);
LONG RegEnumKey(HKEY hKey, DWORD dwIndex, LPTSTR lpName, DWORD cchName);
BOOL remove(TCHAR *filename);

typedef struct {
    UINT Msg;
    SOCKET socket;
} WSAAsyncEmulationParam_struct;
typedef WSAAsyncEmulationParam_struct WSAAsyncEmulationParam;

#define IsZoomed(x) TRUE
#define GetMessageTime GetTickCount
#define FlashWindow(hwnd, flash)
#define Beep(a,b)
#define update_specials_menu(x)

/*
 *	Missing constants, defines
 */
#define BN_DOUBLECLICKED	5
#define SWP_NOREDRAW		0x0008
#define WS_OVERLAPPEDWINDOW	(WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
#define PC_NOCOLLAPSE		0x04
#define NORMAL_PRIORITY_CLASS	0
#define GetCharWidth32W GetCharWidth32
/*
 *	// ((*s)->write) (s, abuf, len);
 */

/*
 *	Utility functions
 */
char *unicode2ansi(const wchar_t *source);
wchar_t *ansi2unicode(const char *source);



#ifdef _WIN32_WCE
#define MulDiv(x,y,z) (((x)*(y))/(z))
#define _IONBF 0x0004
#define BUFSIZ 512
#endif

#if (UNDER_CE > 0 && UNDER_CE < 400)
//
// Pocket PC 2000, 2002
//
BOOL CheckDlgButton(HWND hwnd, int nIDButton, UINT nCheck);
UINT IsDlgButtonChecked(HWND hwnd, int nIDButton);

#endif

/* *************************************************************************************** */
#endif

