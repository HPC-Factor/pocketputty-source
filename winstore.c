/*
 * winstore.c: Windows-specific implementation of the interface
 * defined in storage.h.
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "putty.h"
#include "storage.h"

static const TCHAR *const puttystr = PUTTY_REG_POS _T("\\Sessions");

static TCHAR seedpath[2 * MAX_PATH + 10] = _T("\0");

static const char hex[17] = "0123456789ABCDEF";
static const wchar_t whex[17] = L"0123456789ABCDEF";

static void mungestr(const char *in, char *out)
{
    int candot = 0;

    while (*in) {
	if (*in == ' ' || *in == '\\' || *in == '*' || *in == '?' ||
	    *in == '%' || *in < ' ' || *in > '~' || (*in == '.'
						     && !candot)) {
	    *out++ = '%';
	    *out++ = hex[((unsigned char) *in) >> 4];
	    *out++ = hex[((unsigned char) *in) & 15];
	} else
	    *out++ = *in;
	in++;
	candot = 1;
    }
    *out = '\0';
    return;
}

static void mungewcs(const wchar_t *in, wchar_t *out)
{
    int candot = 0;

    while (*in) {
	if (*in == L' ' || *in == L'\\' || *in == L'*' || *in == L'?' ||
	    *in == L'%' || *in < L' ' || *in > L'~' || (*in == L'.')
						     && !candot) {
	    *out++ = L'%';
#ifdef _UNICODE
	    {
	        TCHAR t = *in;
		*out++ = whex[(((WORD) t) >> 12)];
		*out++ = whex[(((WORD) t) >> 8) & 15];
		*out++ = whex[(((WORD) t) >> 4) & 15];
		*out++ = whex[((WORD) t)        & 15];
	    }
#else
	    *out++ = hex[((unsigned char) *in) >> 4];
	    *out++ = hex[((unsigned char) *in) & 15];
#endif
	} else
	    *out++ = *in;
	in++;
	candot = 1;
    }
    *out = L'\0';
    return;
}
static void unmungestr(const char *in, char *out, int outlen)
{
    while (*in) {
	if (*in == '%' && in[1] && in[2]) {
	    int i, j;

	    i = in[1] - '0';
	    i -= (i > 9 ? 7 : 0);
	    j = in[2] - '0';
	    j -= (j > 9 ? 7 : 0);

	    *out++ = (i << 4) + j;
	    if (!--outlen)
		return;
	    in += 3;
	} else {
	    *out++ = *in++;
	    if (!--outlen)
		return;
	}
    }
    *out = _T('\0');
    return;
}
static void unmungewcs(const wchar_t *in, wchar_t *out, int outlen)
{
    while (*in) {
	if (*in == L'%' && in[1] && in[2] && in[3] && in[4]) {
	    int i, j, k, l;

	    i = in[1] - L'0';
	    i -= (i > 9 ? 7 : 0);
	    j = in[2] - L'0';
	    j -= (j > 9 ? 7 : 0);
	    k = in[3] - L'0';
	    k -= (k > 9 ? 7 : 0);
	    l = in[4] - L'0';
	    l -= (l > 9 ? 7 : 0);

	    *out++ = (i << 12) + (j << 8) + (k << 4) + l;
	    if (!--outlen)
		return;
	    in += 3;
	} else {
	    *out++ = *in++;
	    if (!--outlen)
		return;
	}
    }
    *out = L'\0';
    return;
}

void *open_settings_w(const TCHAR *sessionname, TCHAR **errmsg)
{
    HKEY subkey1, sesskey;
    int ret;
    TCHAR *p;

    *errmsg = NULL;

    if (!sessionname || !*sessionname)
	sessionname = _T("Default Settings");

    p = snewn(3 * _tcslen(sessionname) + 1, TCHAR);
    _mungetcs(sessionname, p);

    ret = RegCreateKey(HKEY_CURRENT_USER, puttystr, &subkey1);
    if (ret != ERROR_SUCCESS) {
	sfree(p);
        *errmsg = _duptprintf(_T("Unable to create registry key\n")
                              _T("HKEY_CURRENT_USER%s"), puttystr);
	return NULL;
    }
    ret = RegCreateKey(subkey1, p, &sesskey);
    sfree(p);
    RegCloseKey(subkey1);
    if (ret != ERROR_SUCCESS) {
        *errmsg = _duptprintf(_T("Unable to create registry key\n")
                              _T("HKEY_CURRENT_USER%s\\%s"), puttystr, p);
	return NULL;
    }
    return (void *) sesskey;
}

void write_setting_s(void *handle, const TCHAR *key, const char *value)
{
    if (handle)
	RegSetValueEx((HKEY) handle, key, 0, REG_SZ, value,
		      1 + strlen(value));
}

void write_setting_wcs(void *handle, const TCHAR *key, const wchar_t *value)
{
    if (handle)
	RegSetValueEx((HKEY) handle, key, 0, REG_SZ, (LPBYTE) value,
		      (1 + wcslen(value)) * sizeof(wchar_t));
}

//#ifdef _UNICODE
//#define write_setting_tcs write_setting_wcs
//#else
//#define write_setting_tcs write_setting_s
//#endif

void write_setting_i(void *handle, const TCHAR *key, int value)
{
    if (handle)
	RegSetValueEx((HKEY) handle, key, 0, REG_DWORD,
		      (CONST BYTE *) & value, sizeof(value));
}

void close_settings_w(void *handle)
{
    RegCloseKey((HKEY) handle);
}

void *open_settings_r(const TCHAR *sessionname)
{
    HKEY subkey1, sesskey;
    TCHAR *p;

    if (!sessionname || !*sessionname)
	sessionname = _T("Default Settings");

    p = snewn(3 * _tcslen(sessionname) + 1, TCHAR);
    _mungetcs(sessionname, p);

    if (RegOpenKey(HKEY_CURRENT_USER, puttystr, &subkey1) != ERROR_SUCCESS) {
	sesskey = NULL;
    } else {
	if (RegOpenKey(subkey1, p, &sesskey) != ERROR_SUCCESS) {
	    sesskey = NULL;
	}
	RegCloseKey(subkey1);
    }

    sfree(p);

    return (void *) sesskey;
}

char *read_setting_s(void *handle, const TCHAR *key, char *buffer, int buflen)
{
    DWORD type, size;
    size = buflen;

    if (!handle ||
	RegQueryValueEx((HKEY) handle, key, 0,
			&type, buffer, &size) != ERROR_SUCCESS ||
	type != REG_SZ) return NULL;
    else
	return buffer;
}

TCHAR *read_setting_wcs(void *handle, const TCHAR *key, wchar_t *buffer, int buflen)
{
    DWORD type, size;
    size = buflen;

    if (!handle ||
	RegQueryValueEx((HKEY) handle, key, 0,
			&type, (LPBYTE) buffer, &size) != ERROR_SUCCESS ||
	type != REG_SZ) return NULL;
    else
	return buffer;
}

int read_setting_i(void *handle, const TCHAR *key, int defvalue)
{
    DWORD type, val, size;
    size = sizeof(val);

    if (!handle ||
	RegQueryValueEx((HKEY) handle, key, 0, &type,
			(BYTE *) & val, &size) != ERROR_SUCCESS ||
	size != sizeof(val) || type != REG_DWORD)
	return defvalue;
    else
	return val;
}

int read_setting_fontspec(void *handle, const TCHAR *name, FontSpec *result)
{
    TCHAR *settingname;
    FontSpec ret;

    if (!_read_setting_ts(handle, name, ret.name, sizeof(ret.name)))
	return 0;
    settingname = _duptcat(name, _T("IsBold"), NULL);
    ret.isbold = read_setting_i(handle, settingname, -1);
    sfree(settingname);
    if (ret.isbold == -1) return 0;
    settingname = _duptcat(name, _T("CharSet"), NULL);
    ret.charset = read_setting_i(handle, settingname, -1);
    sfree(settingname);
    if (ret.charset == -1) return 0;
    settingname = _duptcat(name, _T("Height"), NULL);
    ret.height = read_setting_i(handle, settingname, INT_MIN);
    sfree(settingname);
    if (ret.height == INT_MIN) return 0;
    if (ret.height < 0) {
	int oldh, newh;
	HDC hdc = GetDC(NULL);
	int logpix = GetDeviceCaps(hdc, LOGPIXELSY);
	ReleaseDC(NULL, hdc);

	oldh = -ret.height;
	newh = MulDiv(oldh, 72, logpix) + 1;
	if (MulDiv(newh, logpix, 72) > oldh)
	    newh--;
	ret.height = newh;
    }
    *result = ret;
    return 1;
}

void write_setting_fontspec(void *handle, const TCHAR *name, FontSpec font)
{
    TCHAR *settingname;

    _write_setting_ts(handle, name, font.name);
    settingname = _duptcat(name, _T("IsBold"), NULL);
    write_setting_i(handle, settingname, font.isbold);
    sfree(settingname);
    settingname = _duptcat(name, _T("CharSet"), NULL);
    write_setting_i(handle, settingname, font.charset);
    sfree(settingname);
    settingname = _duptcat(name, _T("Height"), NULL);
    write_setting_i(handle, settingname, font.height);
    sfree(settingname);
}

int read_setting_filename(void *handle, const TCHAR *name, Filename *result)
{
    return !!read_setting_s(handle, name, result->path, sizeof(result->path));
}

void write_setting_filename(void *handle, const TCHAR *name, Filename result)
{
    write_setting_s(handle, name, result.path);
}

void close_settings_r(void *handle)
{
    RegCloseKey((HKEY) handle);
}

void del_settings(const TCHAR *sessionname)
{
    HKEY subkey1;
    TCHAR *p;

    if (RegOpenKey(HKEY_CURRENT_USER, puttystr, &subkey1) != ERROR_SUCCESS)
	return;

    p = snewn(3 * _tcslen(sessionname) + 1, TCHAR);
    _mungetcs(sessionname, p);
    RegDeleteKey(subkey1, p);
    sfree(p);

    RegCloseKey(subkey1);
}

struct enumsettings {
    HKEY key;
    int i;
};

void *enum_settings_start(void)
{
    struct enumsettings *ret;
    HKEY key;

    if (RegOpenKey(HKEY_CURRENT_USER, puttystr, &key) != ERROR_SUCCESS)
	return NULL;

    ret = snew(struct enumsettings);
    if (ret) {
	ret->key = key;
	ret->i = 0;
    }

    return ret;
}

TCHAR *enum_settings_next(void *handle, TCHAR *buffer, int buflen)
{
    struct enumsettings *e = (struct enumsettings *) handle;
    TCHAR *otherbuf;
    otherbuf = snewn(3 * buflen, TCHAR);
    if (RegEnumKey(e->key, e->i++, otherbuf, 3 * buflen) == ERROR_SUCCESS) {
	_unmungetcs(otherbuf, buffer, buflen);
	sfree(otherbuf);
	return buffer;
    } else {
	sfree(otherbuf);
	return NULL;
    }
}

void enum_settings_finish(void *handle)
{
    struct enumsettings *e = (struct enumsettings *) handle;
    RegCloseKey(e->key);
    sfree(e);
}

static void hostkey_regname(char *buffer, const char *hostname,
			    int port, const char *keytype)
{
    int len;
    strcpy(buffer, keytype);
    strcat(buffer, "@");
    len = strlen(buffer);
    len += sprintf(buffer + len, "%d:", port);
    mungestr(hostname, buffer + strlen(buffer));
}

int verify_host_key(const char *hostname, int port,
		    const char *keytype, const char *key)
{
    char *otherstr, *regname;
    int len;
    HKEY rkey;
    DWORD readlen;
    DWORD type;
    int ret, compare;
#ifdef _UNICODE
    TCHAR *wregname;
#endif;

    len = 1 + strlen(key);

    /*
     * Now read a saved key in from the registry and see what it
     * says.
     */
    otherstr = snewn(len, char);
    regname = snewn(3 * (strlen(hostname) + strlen(keytype)) + 15, char);

    hostkey_regname(regname, hostname, port, keytype);
    if (RegOpenKey(HKEY_CURRENT_USER, PUTTY_REG_POS _T("\\SshHostKeys"),
		   &rkey) != ERROR_SUCCESS)
	return 1;		       /* key does not exist in registry */

    readlen = len;
#ifdef _UNICODE
    wregname = ansi2unicode(regname);
    ret = RegQueryValueEx(rkey, wregname, NULL, &type, otherstr, &readlen);
#else
    ret = RegQueryValueEx(rkey, regname, NULL, &type, otherstr, &readlen);
#endif

    if (ret != ERROR_SUCCESS && ret != ERROR_MORE_DATA &&
	!strcmp(keytype, "rsa")) {
	/*
	 * Key didn't exist. If the key type is RSA, we'll try
	 * another trick, which is to look up the _old_ key format
	 * under just the hostname and translate that.
	 */

#ifdef _UNICODE
    	TCHAR *justhost = wregname + 1 + _tcscspn(wregname, _T(":"));
#else
	char *justhost = regname + 1+ strcspn(regname, ":");
#endif
	char *oldstyle = snewn(len + 10, char);	/* safety margin */
	readlen = len;
	ret = RegQueryValueEx(rkey, justhost, NULL, &type,
			      oldstyle, &readlen);

	if (ret == ERROR_SUCCESS && type == REG_SZ) {
	    /*
	     * The old format is two old-style bignums separated by
	     * a slash. An old-style bignum is made of groups of
	     * four hex digits: digits are ordered in sensible
	     * (most to least significant) order within each group,
	     * but groups are ordered in silly (least to most)
	     * order within the bignum. The new format is two
	     * ordinary C-format hex numbers (0xABCDEFG...XYZ, with
	     * A nonzero except in the special case 0x0, which
	     * doesn't appear anyway in RSA keys) separated by a
	     * comma. All hex digits are lowercase in both formats.
	     */
	    char *p = otherstr;
	    char *q = oldstyle;
	    int i, j;

	    for (i = 0; i < 2; i++) {
		int ndigits, nwords;
		*p++ = '0';
		*p++ = 'x';
		ndigits = strcspn(q, "/");	/* find / or end of string */
		nwords = ndigits / 4;
		/* now trim ndigits to remove leading zeros */
		while (q[(ndigits - 1) ^ 3] == '0' && ndigits > 1)
		    ndigits--;
		/* now move digits over to new string */
		for (j = 0; j < ndigits; j++)
		    p[ndigits - 1 - j] = q[j ^ 3];
		p += ndigits;
		q += nwords * 4;
		if (*q) {
		    q++;	       /* eat the slash */
		    *p++ = ',';	       /* add a comma */
		}
		*p = '\0';	       /* terminate the string */
	    }

	    /*
	     * Now _if_ this key matches, we'll enter it in the new
	     * format. If not, we'll assume something odd went
	     * wrong, and hyper-cautiously do nothing.
	     */
	    if (!strcmp(otherstr, key))
#ifdef _UNICODE
		RegSetValueEx(rkey, wregname, 0, REG_SZ, otherstr,
			      strlen(otherstr) + 1);
#else
		RegSetValueEx(rkey, regname, 0, REG_SZ, otherstr,
			      strlen(otherstr) + 1);
#endif
	}
    }

    RegCloseKey(rkey);

    compare = strcmp(otherstr, key);

    sfree(otherstr);
    sfree(regname);
#ifdef _UNICODE
    sfree(wregname);
#endif

    if (ret == ERROR_MORE_DATA ||
	(ret == ERROR_SUCCESS && type == REG_SZ && compare))
	return 2;		       /* key is different in registry */
    else if (ret != ERROR_SUCCESS || type != REG_SZ)
	return 1;		       /* key does not exist in registry */
    else
	return 0;		       /* key matched OK in registry */
}

void store_host_key(const char *hostname, int port,
		    const char *keytype, const char *key)
{
    char *regname;
    HKEY rkey;
#ifdef _UNICODE
    TCHAR *wregname;
#endif;

    regname = snewn(3 * (strlen(hostname) + strlen(keytype)) + 15, char);

    hostkey_regname(regname, hostname, port, keytype);

    if (RegCreateKey(HKEY_CURRENT_USER, PUTTY_REG_POS _T("\\SshHostKeys"),
		     &rkey) != ERROR_SUCCESS)
	return;			       /* key does not exist in registry */
#ifdef _UNICODE
    wregname = ansi2unicode(regname);
    RegSetValueEx(rkey, wregname, 0, REG_SZ, key, strlen(key) + 1);
    sfree(wregname);
#else
    RegSetValueEx(rkey, regname, 0, REG_SZ, key, strlen(key) + 1);
#endif
    RegCloseKey(rkey);
}

/*
 * Find the random seed file path and store it in `seedpath'.
 */
static void get_seedpath(void)
{
    HKEY rkey;
    DWORD type, size;

    size = sizeof(seedpath);

    if (RegOpenKey(HKEY_CURRENT_USER, PUTTY_REG_POS, &rkey) ==
	ERROR_SUCCESS) {
	int ret = RegQueryValueEx(rkey, _T("RandSeedFile"),
				  0, &type, (LPBYTE) seedpath, &size);
	if (ret != ERROR_SUCCESS || type != REG_SZ)
	    seedpath[0] = '\0';
	RegCloseKey(rkey);
    } else
	seedpath[0] = '\0';

    if (!seedpath[0]) {
	int len, ret;

#ifdef _WIN32_WCE
	_tcscpy(seedpath, _T("\\My Documents"));
	len = _tcslen(seedpath) - 1;
	ret = 1;

#else
	len =
	    GetEnvironmentVariable("HOMEDRIVE", seedpath,
				   sizeof(seedpath));
	ret =
	    GetEnvironmentVariable("HOMEPATH", seedpath + len,
				   sizeof(seedpath) - len);
#endif
	if (ret == 0) {		       /* probably win95; store in \WINDOWS */
	    GetWindowsDirectory(seedpath, sizeof(seedpath));
	    len = _tcslen(seedpath);
	} else
	    len += ret;
	_tcscpy(seedpath + len, _T("\\PUTTY.RND"));
    }
}

void read_random_seed(noise_consumer_t consumer)
{
    HANDLE seedf;

    if (!seedpath[0])
	get_seedpath();

    seedf = CreateFile(seedpath, GENERIC_READ,
		       FILE_SHARE_READ | FILE_SHARE_WRITE,
		       NULL, OPEN_EXISTING, 0, NULL);

    if (seedf != INVALID_HANDLE_VALUE) {
	while (1) {
	    char buf[1024];
	    DWORD len;

	    if (ReadFile(seedf, buf, sizeof(buf), &len, NULL) && len)
		consumer(buf, len);
	    else
		break;
	}
	CloseHandle(seedf);
    }
}

void write_random_seed(void *data, int len)
{
    HANDLE seedf;

    if (!seedpath[0])
	get_seedpath();

    seedf = CreateFile(seedpath, GENERIC_WRITE, 0,
		       NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (seedf != INVALID_HANDLE_VALUE) {
	DWORD lenwritten;

	WriteFile(seedf, data, len, &lenwritten, NULL);
	CloseHandle(seedf);
    }
}

/*
 * Recursively delete a registry key and everything under it.
 */
static void registry_recursive_remove(HKEY key)
{
    DWORD i;
    TCHAR name[MAX_PATH + 1];
    HKEY subkey;

    i = 0;
    while (RegEnumKey(key, i, name, lenof(name)) == ERROR_SUCCESS) {
	if (RegOpenKey(key, name, &subkey) == ERROR_SUCCESS) {
	    registry_recursive_remove(subkey);
	    RegCloseKey(subkey);
	}
	RegDeleteKey(key, name);
    }
}

void cleanup_all(void)
{
    HKEY key;
    int ret;
    TCHAR name[MAX_PATH + 1];

    /* ------------------------------------------------------------
     * Wipe out the random seed file.
     */
    if (!seedpath[0])
	get_seedpath();
    remove(seedpath);

    /* ------------------------------------------------------------
     * Destroy all registry information associated with PuTTY.
     */

    /*
     * Open the main PuTTY registry key and remove everything in it.
     */
    if (RegOpenKey(HKEY_CURRENT_USER, PUTTY_REG_POS, &key) ==
	ERROR_SUCCESS) {
	registry_recursive_remove(key);
	RegCloseKey(key);
    }
    /*
     * Now open the parent key and remove the PuTTY main key. Once
     * we've done that, see if the parent key has any other
     * children.
     */
    if (RegOpenKey(HKEY_CURRENT_USER, PUTTY_REG_PARENT,
		   &key) == ERROR_SUCCESS) {
	RegDeleteKey(key, PUTTY_REG_PARENT_CHILD);
	ret = RegEnumKey(key, 0, name, sizeof(name));
	RegCloseKey(key);
	/*
	 * If the parent key had no other children, we must delete
	 * it in its turn. That means opening the _grandparent_
	 * key.
	 */
	if (ret != ERROR_SUCCESS) {
	    if (RegOpenKey(HKEY_CURRENT_USER, PUTTY_REG_GPARENT,
			   &key) == ERROR_SUCCESS) {
		RegDeleteKey(key, PUTTY_REG_GPARENT_CHILD);
		RegCloseKey(key);
	    }
	}
    }
    /*
     * Now we're done.
     */
}
