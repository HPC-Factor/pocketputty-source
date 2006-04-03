#include "wcestuff.h"
#include <tchar.h>

#define WCE_WIN_PATH _T("\\Windows")

UINT GetWindowsDirectory(LPTSTR lpBuffer, UINT uSize) {

	// lpBuffer = "";
	_stprintf(lpBuffer, WCE_WIN_PATH);
	return (_tcslen(WCE_WIN_PATH));
}

BOOL AllocConsole(void) {
    return (FALSE);
}

HANDLE GetStdHandle(DWORD nStdHandle) {
    return (INVALID_HANDLE_VALUE);
}

BOOL GetUserName(LPSTR lpBuffer, LPDWORD nSize) {
    return (FALSE);
}

