#include "putty.h"
//include "StdAfx.h"
//include "porting.h"

void logwevent(void *frontend, const wchar_t *string) {
    char *astring = unicode2ansi(string);
    logevent(frontend, astring);
    sfree(astring);
}

BOOL TextOut(HDC hdc, int nXStart, int nYStart, LPCWSTR lpwString, int wcbString) {
    RECT *rect = snew(RECT);
    int result;
    rect->left = nXStart;
    rect->top = nYStart;
    // Aleq: ooops! Nasty temporary workaroung
    rect->right = 0;
    rect->bottom = 0;

    // Aleq: Possibly DT_NOCLIP usage?
    rect->bottom = rect->top + DrawText(hdc, lpwString, wcbString, rect, DT_CALCRECT);
    result = DrawText(hdc, lpwString, wcbString, rect, DT_LEFT);
    sfree(rect);

    return (result);
}

HFONT CreateFont(int nHeight, int nWidth, int nEscapement, int nOrientation, int fnWeight,
  DWORD fdwItalic, DWORD fdwUnderline, DWORD fdwStrikeOut, DWORD fdwCharSet, 
  DWORD fdwOutputPrecision, DWORD fdwClipPrecision, DWORD fdwQuality, DWORD fdwPitchAndFamily,
  LPCTSTR lpszFace) {
    LOGFONTW lfw;

    lfw.lfHeight = nHeight;
    lfw.lfWidth = nWidth;
    lfw.lfEscapement = nEscapement;
    lfw.lfOrientation = nOrientation;
    lfw.lfWeight = fnWeight;
    lfw.lfItalic = (BYTE) fdwItalic;
    lfw.lfUnderline = (BYTE) fdwUnderline;
    lfw.lfStrikeOut = (BYTE) fdwStrikeOut;
    lfw.lfCharSet = (BYTE) fdwCharSet;
    lfw.lfOutPrecision = (BYTE) fdwOutputPrecision;
    lfw.lfClipPrecision = (BYTE) fdwClipPrecision;
    lfw.lfQuality = (BYTE) fdwQuality;
    lfw.lfPitchAndFamily = (BYTE) fdwPitchAndFamily;
    _tcsncpy(lfw.lfFaceName, lpszFace, lenof(lfw.lfFaceName));

    return (CreateFontIndirect(&lfw));
}

BOOL GetKeyboardState(PBYTE lpKeyState) {
    int vk;
    for (vk = 0; vk <= 0xFF; vk++)
	lpKeyState[vk] = (BYTE) GetKeyState(vk);
    return TRUE;
}

int ToAsciiEx(UINT uVirtKey, UINT uScanCode, PBYTE lpKeyState, LPWORD lpChar, UINT uFlags, HKL dwhkl) {

    WORD chvalue;
    chvalue = MapVirtualKey(uVirtKey, 2);

    if (!chvalue)
	return (0);
    lpChar[0] = chvalue;
    if (chvalue & 0x80000000)
	return (2);
    else
	return (1);
}

LONG RegCreateKey(HKEY hKey, LPCTSTR lpSubKey, PHKEY phkResult) {
    DWORD dwDisp;
    return (RegCreateKeyEx(hKey, lpSubKey, 0, NULL, 0, 0, NULL, phkResult, &dwDisp));
}

LONG RegOpenKey(HKEY hKey, LPCTSTR lpSubKey, PHKEY phkResult) {
    return (RegOpenKeyEx(hKey, lpSubKey, 0, 0, phkResult));
}

LONG RegEnumKey(HKEY hKey, DWORD dwIndex, LPTSTR lpName, DWORD cchName) {
    return (RegEnumKeyEx(hKey, dwIndex, lpName, &cchName, 0, NULL, NULL, NULL));
}


char *unicode2ansi(const wchar_t *source) {
    char *result = snewn(wcslen(source) + 1, char);
    result[wcslen(source)] = '\0';
    wcstombs(result, source, wcslen(source));
    return (result);
}

wchar_t *ansi2unicode(const char *source) {
    wchar_t *result = snewn(strlen(source) + 1, wchar_t);
    result[strlen(source)] = L'\0';
    mbstowcs(result, source, strlen(source));
    return (result);
}

BOOL remove(TCHAR *filename) {
    //Aleq
    // TODO: Implementovat
    // ...
    // ...
    // ...
    // ...
    // ...
    // ...
    // ...
    // ...
    // ...
    // ...
    return (TRUE);
}

#if (UNDER_CE < 400)
//
// Pocket PC 2000, 2002 Implementations
// 

BOOL CheckDlgButton(HWND hwnd, int nIDButton, UINT nCheck) {
    SendMessage(GetDlgItem(hwnd, nIDButton), BM_SETCHECK, (WPARAM) nCheck, (LPARAM) 0);
    return TRUE;
}


UINT IsDlgButtonChecked(HWND hwnd, int nIDButton) {
    return (UINT) SendMessage(GetDlgItem(hwnd, nIDButton), BM_GETCHECK, (WPARAM) 0, (LPARAM) 0);
}

//
// Pocket PC 2000, 2002 Implementation END
//
#endif 
