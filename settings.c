/*
 * settings.c: read and write saved sessions.
 */

#include <stdio.h>
#include <stdlib.h>
#include "putty.h"
#include "storage.h"

/*
 * Tables of string <-> enum value mappings
 */
struct keyval { char *s; int v; };

static const struct keyval ciphernames[] = {
    { "aes",	    CIPHER_AES },
    { "blowfish",   CIPHER_BLOWFISH },
    { "3des",	    CIPHER_3DES },
    { "WARN",	    CIPHER_WARN },
    { "des",	    CIPHER_DES }
};

static void gpps(void *handle, const TCHAR *name, const char *def,
		 char *val, int len)
{
    if (!read_setting_s(handle, name, val, len)) {
	char *pdef;

	pdef = platform_default_s(name);
	if (pdef) {
	    strncpy(val, pdef, len);
	    sfree(pdef);
	} else {
	    strncpy(val, def, len);
	}

	val[len - 1] = '\0';
    }
}

static void gppwcs(void *handle, const TCHAR *name, const wchar_t *def,
		 wchar_t *val, int len)
{
    if (!read_setting_wcs(handle, name, val, len)) {
	TCHAR *pdef;

	pdef = platform_default_wcs(name);
	if (pdef) {
	    wcsncpy(val, pdef, len);
	    sfree(pdef);
	} else {
	    wcsncpy(val, def, len);
	}

	val[len - 1] = _T('\0');
    }
}

/*
 * gppfont and gppfile cannot have local defaults, since the very
 * format of a Filename or Font is platform-dependent. So the
 * platform-dependent functions MUST return some sort of value.
 */
static void gppfont(void *handle, const TCHAR *name, FontSpec *result)
{
    if (!read_setting_fontspec(handle, name, result))
	*result = platform_default_fontspec(name);
}
static void gppfile(void *handle, const TCHAR *name, Filename *result)
{
    if (!read_setting_filename(handle, name, result))
	*result = platform_default_filename(name);
}

static void gppi(void *handle, TCHAR *name, int def, int *i)
{
    def = platform_default_i(name, def);
    *i = read_setting_i(handle, name, def);
}

static int key2val(const struct keyval *mapping, int nmaps, char *key)
{
    int i;
    for (i = 0; i < nmaps; i++)
	if (!strcmp(mapping[i].s, key)) return mapping[i].v;
    return -1;
}

static const char *val2key(const struct keyval *mapping, int nmaps, int val)
{
    int i;
    for (i = 0; i < nmaps; i++)
	if (mapping[i].v == val) return mapping[i].s;
    return NULL;
}

/*
 * Helper function to parse a comma-separated list of strings into
 * a preference list array of values. Any missing values are added
 * to the end and duplicates are weeded.
 * XXX: assumes vals in 'mapping' are small +ve integers
 */
static void gprefs(void *sesskey, TCHAR *name, char *def,
		   const struct keyval *mapping, int nvals,
		   int *array)
{
    char commalist[80];
    int n;
    unsigned long seen = 0;	       /* bitmap for weeding dups etc */
    gpps(sesskey, name, def, commalist, sizeof(commalist));

    /* Grotty parsing of commalist. */
    n = 0;
    do {
	int v;
	char *key;
	key = strtok(n==0 ? commalist : NULL, ","); /* sorry */
	if (!key) break;
	if (((v = key2val(mapping, nvals, key)) != -1) &&
	    !(seen & 1<<v)) {
	    array[n] = v;
	    n++;
	    seen |= 1<<v;
	}
    } while (n < nvals);
    /* Add any missing values (backward compatibility ect). */
    {
	int i;
	for (i = 0; i < nvals; i++) {
	    if (!(seen & 1<<mapping[i].v)) {
		array[n] = mapping[i].v;
		n++;
	    }
	}
    }
}

/* 
 * Write out a preference list.
 */
static void wprefs(void *sesskey, TCHAR *name,
		   const struct keyval *mapping, int nvals,
		   int *array)
{
    char buf[80] = "";	/* XXX assumed big enough */
    int l = sizeof(buf)-1, i;
    buf[l] = '\0';
    for (i = 0; l > 0 && i < nvals; i++) {
	const char *s = val2key(mapping, nvals, array[i]);
	if (s) {
	    int sl = strlen(s);
	    if (i > 0) {
		strncat(buf, ",", l);
		l--;
	    }
	    strncat(buf, s, l);
	    l -= sl;
	}
    }
    write_setting_s(sesskey, name, buf);
}

TCHAR *save_settings(TCHAR *section, int do_host, Config * cfg)
{
    void *sesskey;
    TCHAR *errmsg;

    sesskey = open_settings_w(section, &errmsg);
    if (!sesskey)
	return errmsg;
    save_open_settings(sesskey, do_host, cfg);
    close_settings_w(sesskey);
    return NULL;
}

void save_open_settings(void *sesskey, int do_host, Config *cfg)
{
    int i;
    char *p;

    write_setting_i(sesskey, _T("Present"), 1);
    if (do_host) {
	write_setting_s(sesskey, _T("HostName"), cfg->host);
	write_setting_filename(sesskey, _T("LogFileName"), cfg->logfilename);
	write_setting_i(sesskey, _T("LogType"), cfg->logtype);
	write_setting_i(sesskey, _T("LogFileClash"), cfg->logxfovr);
    }
    p = "raw";
    for (i = 0; backends[i].name != NULL; i++)
	if (backends[i].protocol == cfg->protocol) {
	    p = backends[i].name;
	    break;
	}
    write_setting_s(sesskey, _T("Protocol"), p);
    write_setting_i(sesskey, _T("PortNumber"), cfg->port);
    /* The CloseOnExit numbers are arranged in a different order from
     * the standard FORCE_ON / FORCE_OFF / AUTO. */
    write_setting_i(sesskey, _T("CloseOnExit"), (cfg->close_on_exit+2)%3);
    write_setting_i(sesskey, _T("WarnOnClose"), !!cfg->warn_on_close);
    write_setting_i(sesskey, _T("PingInterval"), cfg->ping_interval / 60);	/* minutes */
    write_setting_i(sesskey, _T("PingIntervalSecs"), cfg->ping_interval % 60);	/* seconds */
    write_setting_i(sesskey, _T("TCPNoDelay"), cfg->tcp_nodelay);
    write_setting_s(sesskey, _T("TerminalType"), cfg->termtype);
    write_setting_s(sesskey, _T("TerminalSpeed"), cfg->termspeed);

    /* proxy settings */
    write_setting_s(sesskey, _T("ProxyExcludeList"), cfg->proxy_exclude_list);
    write_setting_i(sesskey, _T("ProxyDNS"), (cfg->proxy_dns+2)%3);
    write_setting_i(sesskey, _T("ProxyLocalhost"), cfg->even_proxy_localhost);
    write_setting_i(sesskey, _T("ProxyMethod"), cfg->proxy_type);
    write_setting_s(sesskey, _T("ProxyHost"), cfg->proxy_host);
    write_setting_i(sesskey, _T("ProxyPort"), cfg->proxy_port);
    write_setting_s(sesskey, _T("ProxyUsername"), cfg->proxy_username);
    write_setting_s(sesskey, _T("ProxyPassword"), cfg->proxy_password);
    write_setting_s(sesskey, _T("ProxyTelnetCommand"), cfg->proxy_telnet_command);

    {
		char buf[2 * sizeof(cfg->environmt)], *p, *q;
		p = buf;
		q = cfg->environmt;
		while (*q) {
			while (*q) {
				int c = *q++;
				if (c == '=' || c == ',' || c == '\\')
					*p++ = '\\';
				if (c == '\t')
					c = '=';
				*p++ = c;
			}
			*p++ = ',';
			q++;
		}
		*p = '\0';
		write_setting_s(sesskey, _T("Environment"), buf);
    }
    write_setting_s(sesskey, _T("UserName"), cfg->username);
    write_setting_s(sesskey, _T("LocalUserName"), cfg->localusername);
    write_setting_i(sesskey, _T("NoPTY"), cfg->nopty);
    write_setting_i(sesskey, _T("Compression"), cfg->compression);
    write_setting_i(sesskey, _T("AgentFwd"), cfg->agentfwd);
    write_setting_i(sesskey, _T("ChangeUsername"), cfg->change_username);
    wprefs(sesskey, _T("Cipher"), ciphernames, CIPHER_MAX,
	   cfg->ssh_cipherlist);
    write_setting_i(sesskey, _T("AuthTIS"), cfg->try_tis_auth);
    write_setting_i(sesskey, _T("AuthKI"), cfg->try_ki_auth);
    write_setting_i(sesskey, _T("SshProt"), cfg->sshprot);
    write_setting_i(sesskey, _T("SSH2DES"), cfg->ssh2_des_cbc);
    write_setting_filename(sesskey, _T("PublicKeyFile"), cfg->keyfile);
    write_setting_s(sesskey, _T("RemoteCommand"), cfg->remote_cmd);
    write_setting_i(sesskey, _T("RFCEnviron"), cfg->rfc_environ);
    write_setting_i(sesskey, _T("PassiveTelnet"), cfg->passive_telnet);
    write_setting_i(sesskey, _T("BackspaceIsDelete"), cfg->bksp_is_delete);
    write_setting_i(sesskey, _T("RXVTHomeEnd"), cfg->rxvt_homeend);
    write_setting_i(sesskey, _T("LinuxFunctionKeys"), cfg->funky_type);
    write_setting_i(sesskey, _T("NoApplicationKeys"), cfg->no_applic_k);
    write_setting_i(sesskey, _T("NoApplicationCursors"), cfg->no_applic_c);
    write_setting_i(sesskey, _T("NoMouseReporting"), cfg->no_mouse_rep);
    write_setting_i(sesskey, _T("NoRemoteResize"), cfg->no_remote_resize);
    write_setting_i(sesskey, _T("NoAltScreen"), cfg->no_alt_screen);
    write_setting_i(sesskey, _T("NoRemoteWinTitle"), cfg->no_remote_wintitle);
    write_setting_i(sesskey, _T("NoRemoteQTitle"), cfg->no_remote_qtitle);
    write_setting_i(sesskey, _T("NoDBackspace"), cfg->no_dbackspace);
    write_setting_i(sesskey, _T("NoRemoteCharset"), cfg->no_remote_charset);
    write_setting_i(sesskey, _T("ApplicationCursorKeys"), cfg->app_cursor);
    write_setting_i(sesskey, _T("ApplicationKeypad"), cfg->app_keypad);
    write_setting_i(sesskey, _T("NetHackKeypad"), cfg->nethack_keypad);
    write_setting_i(sesskey, _T("AltF4"), cfg->alt_f4);
    write_setting_i(sesskey, _T("AltSpace"), cfg->alt_space);
    write_setting_i(sesskey, _T("AltOnly"), cfg->alt_only);
    write_setting_i(sesskey, _T("ComposeKey"), cfg->compose_key);
    write_setting_i(sesskey, _T("CtrlAltKeys"), cfg->ctrlaltkeys);
    write_setting_i(sesskey, _T("TelnetKey"), cfg->telnet_keyboard);
    write_setting_i(sesskey, _T("TelnetRet"), cfg->telnet_newline);
    write_setting_i(sesskey, _T("LocalEcho"), cfg->localecho);
    write_setting_i(sesskey, _T("LocalEdit"), cfg->localedit);
    write_setting_s(sesskey, _T("Answerback"), cfg->answerback);
    write_setting_i(sesskey, _T("AlwaysOnTop"), cfg->alwaysontop);
    write_setting_i(sesskey, _T("FullScreenOnAltEnter"), cfg->fullscreenonaltenter);
    write_setting_i(sesskey, _T("HideMousePtr"), cfg->hide_mouseptr);
    write_setting_i(sesskey, _T("SunkenEdge"), cfg->sunken_edge);
    write_setting_i(sesskey, _T("WindowBorder"), cfg->window_border);
    write_setting_i(sesskey, _T("CurType"), cfg->cursor_type);
    write_setting_i(sesskey, _T("BlinkCur"), cfg->blink_cur);
    write_setting_i(sesskey, _T("Beep"), cfg->beep);
    write_setting_i(sesskey, _T("BeepInd"), cfg->beep_ind);
    write_setting_filename(sesskey, _T("BellWaveFile"), cfg->bell_wavefile);
    write_setting_i(sesskey, _T("BellOverload"), cfg->bellovl);
    write_setting_i(sesskey, _T("BellOverloadN"), cfg->bellovl_n);
    write_setting_i(sesskey, _T("BellOverloadT"), cfg->bellovl_t);
    write_setting_i(sesskey, _T("BellOverloadS"), cfg->bellovl_s);
    write_setting_i(sesskey, _T("ScrollbackLines"), cfg->savelines);
    write_setting_i(sesskey, _T("DECOriginMode"), cfg->dec_om);
    write_setting_i(sesskey, _T("AutoWrapMode"), cfg->wrap_mode);
    write_setting_i(sesskey, _T("LFImpliesCR"), cfg->lfhascr);
    write_setting_i(sesskey, _T("WinNameAlways"), cfg->win_name_always);
    _write_setting_ts(sesskey, _T("WinTitle"), cfg->wintitle);
    write_setting_i(sesskey, _T("TermWidth"), cfg->width);
    write_setting_i(sesskey, _T("TermHeight"), cfg->height);
    write_setting_fontspec(sesskey, _T("Font"), cfg->font);
    write_setting_i(sesskey, _T("FontVTMode"), cfg->vtmode);
    write_setting_i(sesskey, _T("UseSystemColours"), cfg->system_colour);
    write_setting_i(sesskey, _T("TryPalette"), cfg->try_palette);
    write_setting_i(sesskey, _T("BoldAsColour"), cfg->bold_colour);
    for (i = 0; i < 22; i++) {
	TCHAR buf[20];
	char buf2[30];
	_stprintf(buf, _T("Colour%d"), i);
	sprintf(buf2, "%d,%d,%d", cfg->colours[i][0],
		cfg->colours[i][1], cfg->colours[i][2]);
	write_setting_s(sesskey, buf, buf2);
    }
    write_setting_i(sesskey, _T("RawCNP"), cfg->rawcnp);
    write_setting_i(sesskey, _T("PasteRTF"), cfg->rtf_paste);
    write_setting_i(sesskey, _T("MouseIsXterm"), cfg->mouse_is_xterm);
    write_setting_i(sesskey, _T("RectSelect"), cfg->rect_select);
    write_setting_i(sesskey, _T("MouseOverride"), cfg->mouse_override);
    for (i = 0; i < 256; i += 32) {
	TCHAR buf[20];
	char buf2[256];
	int j;
	_stprintf(buf, _T("Wordness%d"), i);
	*buf2 = '\0';
	for (j = i; j < i + 32; j++) {
	    sprintf(buf2 + strlen(buf2), "%s%d",
		    (*buf2 ? "," : ""), cfg->wordness[j]);
	}
	write_setting_s(sesskey, buf, buf2);
    }
    write_setting_s(sesskey, _T("LineCodePage"), cfg->line_codepage);
    write_setting_s(sesskey, _T("Printer"), cfg->printer);
    write_setting_i(sesskey, _T("CapsLockCyr"), cfg->xlat_capslockcyr);
    write_setting_i(sesskey, _T("ScrollBar"), cfg->scrollbar);
    write_setting_i(sesskey, _T("ScrollBarFullScreen"), cfg->scrollbar_in_fullscreen);
    write_setting_i(sesskey, _T("ScrollOnKey"), cfg->scroll_on_key);
    write_setting_i(sesskey, _T("ScrollOnDisp"), cfg->scroll_on_disp);
    write_setting_i(sesskey, _T("EraseToScrollback"), cfg->erase_to_scrollback);
    write_setting_i(sesskey, _T("LockSize"), cfg->resize_action);
    write_setting_i(sesskey, _T("BCE"), cfg->bce);
    write_setting_i(sesskey, _T("BlinkText"), cfg->blinktext);
    write_setting_i(sesskey, _T("X11Forward"), cfg->x11_forward);
    write_setting_s(sesskey, _T("X11Display"), cfg->x11_display);
    write_setting_i(sesskey, _T("X11AuthType"), cfg->x11_auth);
    write_setting_i(sesskey, _T("LocalPortAcceptAll"), cfg->lport_acceptall);
    write_setting_i(sesskey, _T("RemotePortAcceptAll"), cfg->rport_acceptall);
    {
		char buf[2 * sizeof(cfg->portfwd) /* / sizeof(TCHAR) */], *p, *q;
		p = buf;
		q = cfg->portfwd;
		while (*q) {
			while (*q) {
				int c = *q++;
				if (c == '=' || c == ',' || c == '\\')
					*p++ = '\\';
				if (c == '\t')
					c = '=';
				*p++ = c;
			}
			*p++ = ',';
			q++;
		}
		*p = '\0';
		write_setting_s(sesskey, _T("PortForwardings"), buf);
    }
    write_setting_i(sesskey, _T("BugIgnore1"), 2-cfg->sshbug_ignore1);
    write_setting_i(sesskey, _T("BugPlainPW1"), 2-cfg->sshbug_plainpw1);
    write_setting_i(sesskey, _T("BugRSA1"), 2-cfg->sshbug_rsa1);
    write_setting_i(sesskey, _T("BugHMAC2"), 2-cfg->sshbug_hmac2);
    write_setting_i(sesskey, _T("BugDeriveKey2"), 2-cfg->sshbug_derivekey2);
    write_setting_i(sesskey, _T("BugRSAPad2"), 2-cfg->sshbug_rsapad2);
    write_setting_i(sesskey, _T("BugDHGEx2"), 2-cfg->sshbug_dhgex2);
    write_setting_i(sesskey, _T("BugPKSessID2"), 2-cfg->sshbug_pksessid2);
    write_setting_i(sesskey, _T("StampUtmp"), cfg->stamp_utmp);
    write_setting_i(sesskey, _T("LoginShell"), cfg->login_shell);
    write_setting_i(sesskey, _T("ScrollbarOnLeft"), cfg->scrollbar_on_left);
    write_setting_fontspec(sesskey, _T("BoldFont"), cfg->boldfont);
    write_setting_i(sesskey, _T("ShadowBold"), cfg->shadowbold);
    write_setting_i(sesskey, _T("ShadowBoldOffset"), cfg->shadowboldoffset);
}

void load_settings(TCHAR *section, int do_host, Config * cfg)
{
    void *sesskey;

    sesskey = open_settings_r(section);
    load_open_settings(sesskey, do_host, cfg);
    close_settings_r(sesskey);
}

void load_open_settings(void *sesskey, int do_host, Config *cfg)
{
    int i;
    char prot[10];

    cfg->ssh_subsys = 0;	       /* FIXME: load this properly */
    cfg->remote_cmd_ptr = cfg->remote_cmd;
    cfg->remote_cmd_ptr2 = NULL;

    if (do_host) {
	gpps(sesskey, _T("HostName"), "", cfg->host, sizeof(cfg->host));
    } else {
	cfg->host[0] = '\0';	       /* blank hostname */
    }
    gppfile(sesskey, _T("LogFileName"), &cfg->logfilename);
    gppi(sesskey, _T("LogType"), 0, &cfg->logtype);
    gppi(sesskey, _T("LogFileClash"), LGXF_ASK, &cfg->logxfovr);

    gpps(sesskey, _T("Protocol"), "default", prot, 10);
    cfg->protocol = default_protocol;
    cfg->port = default_port;
    for (i = 0; backends[i].name != NULL; i++)
	if (!strcmp(prot, backends[i].name)) {
	    cfg->protocol = backends[i].protocol;
	    gppi(sesskey, _T("PortNumber"), default_port, &cfg->port);
	    break;
	}

    /* The CloseOnExit numbers are arranged in a different order from
     * the standard FORCE_ON / FORCE_OFF / AUTO. */
    gppi(sesskey, _T("CloseOnExit"), 1, &i); cfg->close_on_exit = (i+1)%3;
    gppi(sesskey, _T("WarnOnClose"), 1, &cfg->warn_on_close);
    {
	/* This is two values for backward compatibility with 0.50/0.51 */
	int pingmin, pingsec;
	gppi(sesskey, _T("PingInterval"), 0, &pingmin);
	gppi(sesskey, _T("PingIntervalSecs"), 0, &pingsec);
	cfg->ping_interval = pingmin * 60 + pingsec;
    }
    gppi(sesskey, _T("TCPNoDelay"), 1, &cfg->tcp_nodelay);
    gpps(sesskey, _T("TerminalType"), "xterm", cfg->termtype,
	 sizeof(cfg->termtype));
    gpps(sesskey, _T("TerminalSpeed"), "38400,38400", cfg->termspeed,
	 sizeof(cfg->termspeed));

    /* proxy settings */
    gpps(sesskey, _T("ProxyExcludeList"), "", cfg->proxy_exclude_list,
	 sizeof(cfg->proxy_exclude_list));
    gppi(sesskey, _T("ProxyDNS"), 1, &i); cfg->proxy_dns = (i+1)%3;
    gppi(sesskey, _T("ProxyLocalhost"), 0, &cfg->even_proxy_localhost);
    gppi(sesskey, _T("ProxyMethod"), -1, &cfg->proxy_type);
    if (cfg->proxy_type == -1) {
        int i;
        gppi(sesskey, _T("ProxyType"), 0, &i);
        if (i == 0)
            cfg->proxy_type = PROXY_NONE;
        else if (i == 1)
            cfg->proxy_type = PROXY_HTTP;
        else if (i == 3)
            cfg->proxy_type = PROXY_TELNET;
        else if (i == 4)
            cfg->proxy_type = PROXY_CMD;
        else {
            gppi(sesskey, _T("ProxySOCKSVersion"), 5, &i);
            if (i == 5)
                cfg->proxy_type = PROXY_SOCKS5;
            else
                cfg->proxy_type = PROXY_SOCKS4;
        }
    }
    gpps(sesskey, _T("ProxyHost"), "proxy", cfg->proxy_host,
	 sizeof(cfg->proxy_host));
    gppi(sesskey, _T("ProxyPort"), 80, &cfg->proxy_port);
    gpps(sesskey, _T("ProxyUsername"), "", cfg->proxy_username,
	 sizeof(cfg->proxy_username));
    gpps(sesskey, _T("ProxyPassword"), "", cfg->proxy_password,
	 sizeof(cfg->proxy_password));
    gpps(sesskey, _T("ProxyTelnetCommand"), "connect %host %port\\n",
	 cfg->proxy_telnet_command, sizeof(cfg->proxy_telnet_command));

    {
		char buf[2 * sizeof(cfg->environmt)], *p, *q;
		gpps(sesskey, _T("Environment"), "", buf, sizeof(buf));
		p = buf;
		q = cfg->environmt;
		while (*p) {
			while (*p && *p != ',') {
				int c = *p++;
				if (c == '=')
					c = '\t';
				if (c == '\\')
					c = *p++;
				*q++ = c;
			}
			if (*p == ',')
			p++;
			*q++ = '\0';
		}
		*q = '\0';
    }
    gpps(sesskey, _T("UserName"), "", cfg->username, sizeof(cfg->username));
    gpps(sesskey, _T("LocalUserName"), "", cfg->localusername,
	 sizeof(cfg->localusername));
    gppi(sesskey, _T("NoPTY"), 0, &cfg->nopty);
    gppi(sesskey, _T("Compression"), 0, &cfg->compression);
    gppi(sesskey, _T("AgentFwd"), 0, &cfg->agentfwd);
    gppi(sesskey, _T("ChangeUsername"), 0, &cfg->change_username);
    gprefs(sesskey, _T("Cipher"), "\0",
	   ciphernames, CIPHER_MAX, cfg->ssh_cipherlist);
    gppi(sesskey, _T("SshProt"), 2, &cfg->sshprot);
    gppi(sesskey, _T("SSH2DES"), 0, &cfg->ssh2_des_cbc);
    gppi(sesskey, _T("AuthTIS"), 0, &cfg->try_tis_auth);
    gppi(sesskey, _T("AuthKI"), 1, &cfg->try_ki_auth);
    gppfile(sesskey, _T("PublicKeyFile"), &cfg->keyfile);
    gpps(sesskey, _T("RemoteCommand"), "", cfg->remote_cmd,
	 sizeof(cfg->remote_cmd));
    gppi(sesskey, _T("RFCEnviron"), 0, &cfg->rfc_environ);
    gppi(sesskey, _T("PassiveTelnet"), 0, &cfg->passive_telnet);
    gppi(sesskey, _T("BackspaceIsDelete"), 1, &cfg->bksp_is_delete);
    gppi(sesskey, _T("RXVTHomeEnd"), 0, &cfg->rxvt_homeend);
    gppi(sesskey, _T("LinuxFunctionKeys"), 0, &cfg->funky_type);
    gppi(sesskey, _T("NoApplicationKeys"), 0, &cfg->no_applic_k);
    gppi(sesskey, _T("NoApplicationCursors"), 0, &cfg->no_applic_c);
    gppi(sesskey, _T("NoMouseReporting"), 0, &cfg->no_mouse_rep);
    gppi(sesskey, _T("NoRemoteResize"), 0, &cfg->no_remote_resize);
    gppi(sesskey, _T("NoAltScreen"), 0, &cfg->no_alt_screen);
    gppi(sesskey, _T("NoRemoteWinTitle"), 0, &cfg->no_remote_wintitle);
    gppi(sesskey, _T("NoRemoteQTitle"), 1, &cfg->no_remote_qtitle);
    gppi(sesskey, _T("NoDBackspace"), 0, &cfg->no_dbackspace);
    gppi(sesskey, _T("NoRemoteCharset"), 0, &cfg->no_remote_charset);
    gppi(sesskey, _T("ApplicationCursorKeys"), 0, &cfg->app_cursor);
    gppi(sesskey, _T("ApplicationKeypad"), 0, &cfg->app_keypad);
    gppi(sesskey, _T("NetHackKeypad"), 0, &cfg->nethack_keypad);
    gppi(sesskey, _T("AltF4"), 1, &cfg->alt_f4);
    gppi(sesskey, _T("AltSpace"), 0, &cfg->alt_space);
    gppi(sesskey, _T("AltOnly"), 0, &cfg->alt_only);
    gppi(sesskey, _T("ComposeKey"), 0, &cfg->compose_key);
    gppi(sesskey, _T("CtrlAltKeys"), 1, &cfg->ctrlaltkeys);
    gppi(sesskey, _T("TelnetKey"), 0, &cfg->telnet_keyboard);
    gppi(sesskey, _T("TelnetRet"), 1, &cfg->telnet_newline);
    gppi(sesskey, _T("LocalEcho"), AUTO, &cfg->localecho);
    gppi(sesskey, _T("LocalEdit"), AUTO, &cfg->localedit);
#ifdef _WIN32_WCE
    gpps(sesskey, _T("Answerback"), "PocketPuTTY", cfg->answerback,
	 sizeof(cfg->answerback));
#else
    gpps(sesskey, _T("Answerback"), "PuTTY", cfg->answerback,
	 sizeof(cfg->answerback));
#endif
    gppi(sesskey, _T("AlwaysOnTop"), 0, &cfg->alwaysontop);
    gppi(sesskey, _T("FullScreenOnAltEnter"), 0, &cfg->fullscreenonaltenter);
    gppi(sesskey, _T("HideMousePtr"), 0, &cfg->hide_mouseptr);
    gppi(sesskey, _T("SunkenEdge"), 0, &cfg->sunken_edge);
#ifdef _WIN32_WCE
    gppi(sesskey, _T("WindowBorder"), 0, &cfg->window_border);
#else
    gppi(sesskey, _T("WindowBorder"), 1, &cfg->window_border);
#endif
    gppi(sesskey, _T("CurType"), 0, &cfg->cursor_type);
    gppi(sesskey, _T("BlinkCur"), 0, &cfg->blink_cur);
    /* pedantic compiler tells me I can't use &cfg->beep as an int * :-) */
    gppi(sesskey, _T("Beep"), 1, &cfg->beep);
    gppi(sesskey, _T("BeepInd"), 0, &cfg->beep_ind);
    gppfile(sesskey, _T("BellWaveFile"), &cfg->bell_wavefile);
    gppi(sesskey, _T("BellOverload"), 1, &cfg->bellovl);
    gppi(sesskey, _T("BellOverloadN"), 5, &cfg->bellovl_n);
    gppi(sesskey, _T("BellOverloadT"), 2*TICKSPERSEC, &cfg->bellovl_t);
    gppi(sesskey, _T("BellOverloadS"), 5*TICKSPERSEC, &cfg->bellovl_s);
    gppi(sesskey, _T("ScrollbackLines"), 200, &cfg->savelines);
    gppi(sesskey, _T("DECOriginMode"), 0, &cfg->dec_om);
    gppi(sesskey, _T("AutoWrapMode"), 1, &cfg->wrap_mode);
    gppi(sesskey, _T("LFImpliesCR"), 0, &cfg->lfhascr);
    gppi(sesskey, _T("WinNameAlways"), 1, &cfg->win_name_always);
    _gppts(sesskey, _T("WinTitle"), _T(""), cfg->wintitle, sizeof(cfg->wintitle) / sizeof(TCHAR));
    gppi(sesskey, _T("TermWidth"), 80, &cfg->width);
    gppi(sesskey, _T("TermHeight"), 25, &cfg->height);
    gppfont(sesskey, _T("Font"), &cfg->font);
    gppi(sesskey, _T("FontVTMode"), VT_UNICODE, (int *) &cfg->vtmode);
    gppi(sesskey, _T("UseSystemColours"), 0, &cfg->system_colour);
    gppi(sesskey, _T("TryPalette"), 0, &cfg->try_palette);
    gppi(sesskey, _T("BoldAsColour"), 1, &cfg->bold_colour);
    for (i = 0; i < 22; i++) {
	static const char *const defaults[] = {
	    "187,187,187", "255,255,255", "0,0,0", "85,85,85", "0,0,0",
	    "0,255,0", "0,0,0", "85,85,85", "187,0,0", "255,85,85",
	    "0,187,0", "85,255,85", "187,187,0", "255,255,85", "0,0,187",
	    "85,85,255", "187,0,187", "255,85,255", "0,187,187",
	    "85,255,255", "187,187,187", "255,255,255"
	};
	TCHAR buf[20];
	char buf2[30];
	int c0, c1, c2;
	_stprintf(buf, _T("Colour%d"), i);
	gpps(sesskey, buf, defaults[i], buf2, sizeof(buf2));
	if (sscanf(buf2, "%d,%d,%d", &c0, &c1, &c2) == 3) {
	    cfg->colours[i][0] = c0;
	    cfg->colours[i][1] = c1;
	    cfg->colours[i][2] = c2;
	}
    }
    gppi(sesskey, _T("RawCNP"), 0, &cfg->rawcnp);
    gppi(sesskey, _T("PasteRTF"), 0, &cfg->rtf_paste);
    gppi(sesskey, _T("MouseIsXterm"), 0, &cfg->mouse_is_xterm);
    gppi(sesskey, _T("RectSelect"), 0, &cfg->rect_select);
    gppi(sesskey, _T("MouseOverride"), 1, &cfg->mouse_override);
    for (i = 0; i < 256; i += 32) {
	static const char *const defaults[] = {
	    "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0",
	    "0,1,2,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1",
	    "1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,2",
	    "1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1",
	    "1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1",
	    "1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1",
	    "2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2",
	    "2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2"
	};
	TCHAR buf[20];
	char buf2[256], *p;
	int j;
	_stprintf(buf, _T("Wordness%d"), i);
	gpps(sesskey, buf, defaults[i / 32], buf2, sizeof(buf2));
	p = buf2;
	for (j = i; j < i + 32; j++) {
	    char *q = p;
	    while (*p && *p != ',')
		p++;
	    if (*p == ',')
		*p++ = '\0';
	    cfg->wordness[j] = atoi(q);
	}
    }
    /*
     * The empty default for LineCodePage will be converted later
     * into a plausible default for the locale.
     */
    gpps(sesskey, _T("LineCodePage"), "", cfg->line_codepage,
	 sizeof(cfg->line_codepage));
    gpps(sesskey, _T("Printer"), "", cfg->printer, sizeof(cfg->printer));
    gppi (sesskey, _T("CapsLockCyr"), 0, &cfg->xlat_capslockcyr);
    gppi(sesskey, _T("ScrollBar"), 1, &cfg->scrollbar);
    gppi(sesskey, _T("ScrollBarFullScreen"), 0, &cfg->scrollbar_in_fullscreen);
    gppi(sesskey, _T("ScrollOnKey"), 0, &cfg->scroll_on_key);
    gppi(sesskey, _T("ScrollOnDisp"), 1, &cfg->scroll_on_disp);
    gppi(sesskey, _T("EraseToScrollback"), 1, &cfg->erase_to_scrollback);
    gppi(sesskey, _T("LockSize"), 0, &cfg->resize_action);
    gppi(sesskey, _T("BCE"), 1, &cfg->bce);
    gppi(sesskey, _T("BlinkText"), 0, &cfg->blinktext);
    gppi(sesskey, _T("X11Forward"), 0, &cfg->x11_forward);
    gpps(sesskey, _T("X11Display"), "localhost:0", cfg->x11_display,
	 sizeof(cfg->x11_display));
    gppi(sesskey, _T("X11AuthType"), X11_MIT, &cfg->x11_auth);

    gppi(sesskey, _T("LocalPortAcceptAll"), 0, &cfg->lport_acceptall);
    gppi(sesskey, _T("RemotePortAcceptAll"), 0, &cfg->rport_acceptall);
    {
		char buf[2 * sizeof(cfg->portfwd)], *p, *q;
		gpps(sesskey, _T("PortForwardings"), "", buf, sizeof(buf));
		p = buf;
		q = cfg->portfwd;
		while (*p) {
			while (*p && *p != ',') {
				int c = *p++;
				if (c == '=')
					c = '\t';
				if (c == '\\')
					c = *p++;
				*q++ = c;
			}
			if (*p == ',')
				p++;
			*q++ = '\0';
		}
		*q = '\0';
    }
    gppi(sesskey, _T("BugIgnore1"), 0, &i); cfg->sshbug_ignore1 = 2-i;
    gppi(sesskey, _T("BugPlainPW1"), 0, &i); cfg->sshbug_plainpw1 = 2-i;
    gppi(sesskey, _T("BugRSA1"), 0, &i); cfg->sshbug_rsa1 = 2-i;
    {
	int i;
	gppi(sesskey, _T("BugHMAC2"), 0, &i); cfg->sshbug_hmac2 = 2-i;
	if (cfg->sshbug_hmac2 == AUTO) {
	    gppi(sesskey, _T("BuggyMAC"), 0, &i);
	    if (i == 1)
		cfg->sshbug_hmac2 = FORCE_ON;
	}
    }
    gppi(sesskey, _T("BugDeriveKey2"), 0, &i); cfg->sshbug_derivekey2 = 2-i;
    gppi(sesskey, _T("BugRSAPad2"), 0, &i); cfg->sshbug_rsapad2 = 2-i;
    gppi(sesskey, _T("BugDHGEx2"), 0, &i); cfg->sshbug_dhgex2 = 2-i;
    gppi(sesskey, _T("BugPKSessID2"), 0, &i); cfg->sshbug_pksessid2 = 2-i;
    gppi(sesskey, _T("StampUtmp"), 1, &cfg->stamp_utmp);
    gppi(sesskey, _T("LoginShell"), 1, &cfg->login_shell);
    gppi(sesskey, _T("ScrollbarOnLeft"), 0, &cfg->scrollbar_on_left);
    gppi(sesskey, _T("ShadowBold"), 0, &cfg->shadowbold);
    gppfont(sesskey, _T("BoldFont"), &cfg->boldfont);
    gppfont(sesskey, _T("WideFont"), &cfg->widefont);
    gppfont(sesskey, _T("WideBoldFont"), &cfg->wideboldfont);
    gppi(sesskey, _T("ShadowBoldOffset"), 1, &cfg->shadowboldoffset);
}

void do_defaults(TCHAR *session, Config * cfg)
{
    load_settings(session, (session != NULL && *session), cfg);
}

static int sessioncmp(const void *av, const void *bv)
{
    const TCHAR *a = *(const TCHAR *const *) av;
    const TCHAR *b = *(const TCHAR *const *) bv;

    /*
     * Alphabetical order, except that "Default Settings" is a
     * special case and comes first.
     */
    if (!_tcscmp(a, _T("Default Settings")))
	return -1;		       /* a comes first */
    if (!_tcscmp(b, _T("Default Settings")))
	return +1;		       /* b comes first */
    /*
     * FIXME: perhaps we should ignore the first & in determining
     * sort order.
     */
    return _tcscmp(a, b);	       /* otherwise, compare normally */
}

void get_sesslist(struct sesslist *list, int allocate)
{
    TCHAR otherbuf[2048];
    int buflen, bufsize, i;
    TCHAR *p, *ret;
    void *handle;

    if (allocate) {

		buflen = bufsize = 0;
		list->buffer = NULL;
		if ((handle = enum_settings_start()) != NULL) {
			do {
			ret = enum_settings_next(handle, otherbuf, lenof(otherbuf));
			if (ret) {
				int len = _tcslen(otherbuf) + 1;
				if (bufsize < buflen + len) {
					bufsize = buflen + len + 2048;
					list->buffer = sresize(list->buffer, bufsize, TCHAR);
				}
				_tcscpy(list->buffer + buflen, otherbuf);
				buflen += _tcslen(list->buffer + buflen) + 1;
			}
			} while (ret);
			enum_settings_finish(handle);
		}
		list->buffer = sresize(list->buffer, buflen + 1, TCHAR);
		list->buffer[buflen] = _T('\0');

		/*
		 * Now set up the list of sessions. Note that "Default
		 * Settings" must always be claimed to exist, even if it
		 * doesn't really.
		 */

		p = list->buffer;
		list->nsessions = 1;	       /* "Default Settings" counts as one */
		while (*p) {
			if (_tcscmp(p, _T("Default Settings")))
			list->nsessions++;
			while (*p)
				p++;
			p++;
		}

		list->sessions = snewn(list->nsessions + 1, TCHAR *);
		list->sessions[0] = _T("Default Settings");
		p = list->buffer;
		i = 1;
		while (*p) {
			if (_tcscmp(p, _T("Default Settings")))
			list->sessions[i++] = p;
			while (*p)
				p++;
			p++;
		}

		qsort(list->sessions, i, sizeof(TCHAR *), sessioncmp);
    } else {
		sfree(list->buffer);
		sfree(list->sessions);
    }
}
