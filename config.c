//config - sizeof
//MessageBox

























/*
 * config.c - the platform-independent parts of the PuTTY
 * configuration box.
 */

#if !(UNDER_CE > 0 && UNDER_CE < 400)
#include <assert.h>
#else
#include "wceassrt.h"
#endif
#include <stdlib.h>

#include "putty.h"
#include "dialog.h"
#include "storage.h"
#include "wcemisc.h"

#define PRINTER_DISABLED_STRING _T("None (printing disabled)")

static void protocolbuttons_handler(union control *ctrl, void *dlg,
				    void *data, int event)
{
    int button, defport;
    Config *cfg = (Config *)data;
    /*
     * This function works just like the standard radio-button
     * handler, except that it also has to change the setting of
     * the port box. We expect the context parameter to point at
     * the `union control' structure for the port box.
     */
    if (event == EVENT_REFRESH) {
	for (button = 0; button < ctrl->radio.nbuttons; button++)
	    if (cfg->protocol == ctrl->radio.buttondata[button].i)
		break;
	/* We expected that `break' to happen, in all circumstances. */
	assert(button < ctrl->radio.nbuttons);
	dlg_radiobutton_set(ctrl, dlg, button);
    } else if (event == EVENT_VALCHANGE) {
	int oldproto = cfg->protocol;
	button = dlg_radiobutton_get(ctrl, dlg);
	assert(button >= 0 && button < ctrl->radio.nbuttons);
	cfg->protocol = ctrl->radio.buttondata[button].i;
	if (oldproto != cfg->protocol) {
	    defport = -1;
	    switch (cfg->protocol) {
	      case PROT_SSH: defport = 22; break;
	      case PROT_TELNET: defport = 23; break;
	      case PROT_RLOGIN: defport = 513; break;
	    }
	    if (defport > 0 && cfg->port != defport) {
		cfg->port = defport;
		dlg_refresh((union control *)ctrl->radio.context.p, dlg);
	    }
	}
    }
}

static void numeric_keypad_handler(union control *ctrl, void *dlg,
				   void *data, int event)
{
    int button;
    Config *cfg = (Config *)data;
    /*
     * This function works much like the standard radio button
     * handler, but it has to handle two fields in Config.
     */
    if (event == EVENT_REFRESH) {
	if (cfg->nethack_keypad)
	    button = 2;
	else if (cfg->app_keypad)
	    button = 1;
	else
	    button = 0;
	assert(button < ctrl->radio.nbuttons);
	dlg_radiobutton_set(ctrl, dlg, button);
    } else if (event == EVENT_VALCHANGE) {
	button = dlg_radiobutton_get(ctrl, dlg);
	assert(button >= 0 && button < ctrl->radio.nbuttons);
	if (button == 2) {
	    cfg->app_keypad = FALSE;
	    cfg->nethack_keypad = TRUE;
	} else {
	    cfg->app_keypad = (button != 0);
	    cfg->nethack_keypad = FALSE;
	}
    }
}

static void cipherlist_handler(union control *ctrl, void *dlg,
			       void *data, int event)
{
    Config *cfg = (Config *)data;
    if (event == EVENT_REFRESH) {
	int i;

	static const struct { TCHAR *s; int c; } ciphers[] = {
	    { _T("3DES"),					CIPHER_3DES },
	    { _T("Blowfish"),				CIPHER_BLOWFISH },
	    { _T("DES"),					CIPHER_DES },
	    { _T("AES (SSH 2 only)"),		CIPHER_AES },
	    { _T("-- warn below here --"),	CIPHER_WARN }
	};

	/* Set up the "selected ciphers" box. */
	/* (cipherlist assumed to contain all ciphers) */
	dlg_update_start(ctrl, dlg);
	dlg_listbox_clear(ctrl, dlg);
	for (i = 0; i < CIPHER_MAX; i++) {
	    int c = cfg->ssh_cipherlist[i];
	    int j;
	    TCHAR *cstr = NULL;
	    for (j = 0; j < (sizeof ciphers) / (sizeof ciphers[0]); j++) {
		if (ciphers[j].c == c) {
		    cstr = ciphers[j].s;
		    break;
		}
	    }
	    dlg_listbox_addwithid(ctrl, dlg, cstr, c);
	}
	dlg_update_done(ctrl, dlg);

    } else if (event == EVENT_VALCHANGE) {
	int i;

	/* Update array to match the list box. */
	for (i=0; i < CIPHER_MAX; i++)
	    cfg->ssh_cipherlist[i] = dlg_listbox_getid(ctrl, dlg, i);

    }
}

#ifndef _WIN32_WCE
static void printerbox_handler(union control *ctrl, void *dlg,
			       void *data, int event)
{
    Config *cfg = (Config *)data;
    if (event == EVENT_REFRESH) {
	int nprinters, i;
	printer_enum *pe;

	dlg_update_start(ctrl, dlg);
	/*
	 * Some backends may wish to disable the drop-down list on
	 * this edit box. Be prepared for this.
	 */
	if (ctrl->editbox.has_list) {
	    dlg_listbox_clear(ctrl, dlg);
	    dlg_listbox_add(ctrl, dlg, PRINTER_DISABLED_STRING);
	    pe = printer_start_enum(&nprinters);
	    for (i = 0; i < nprinters; i++)
		dlg_listbox_add(ctrl, dlg, printer_get_name(pe, i));
	    printer_finish_enum(pe);
	}
	dlg_editbox_set(ctrl, dlg,
			(*cfg->printer ? cfg->printer :
			 PRINTER_DISABLED_STRING));
	dlg_update_done(ctrl, dlg);
    } else if (event == EVENT_VALCHANGE) {
	dlg_editbox_get(ctrl, dlg, cfg->printer, sizeof(cfg->printer));
	if (!strcmp(cfg->printer, PRINTER_DISABLED_STRING))
	    *cfg->printer = '\0';
    }
}
#endif

/*static void codepage_handler(union control *ctrl, void *dlg,
			     void *data, int event)
{
    Config *cfg = (Config *)data;
    if (event == EVENT_REFRESH) {
	int i;
	const char *cp;

	dlg_update_start(ctrl, dlg);

	strcpy(cfg->line_codepage, cp_name(decode_codepage(cfg->line_codepage)));

	dlg_listbox_clear(ctrl, dlg);
	for (i = 0; (cp = cp_enumerate(i)) != NULL; i++)
#ifdef _UNICODE
	{
	    
	    TCHAR *wcp = ansi2unicode(cp);
	    dlg_listbox_add(ctrl, dlg, wcp);
	    sfree(wcp);
	}
#else
	    dlg_listbox_add(ctrl, dlg, wcp);
#endif
#ifdef _UNICODE
	wcp = ansi2unicode(cfg->line_codepage);
	dlg_editbox_set(ctrl, dlg, cfg->line_codepage);
	sfree(wcp);
#else
	dlg_editbox_set(ctrl, dlg, cfg->line_codepage);
#endif
	dlg_update_done(ctrl, dlg);
    } else if (event == EVENT_VALCHANGE) {
#ifdef _UNICODE
	TCHAR[sizeof(cfg->line_codepage)] lcp;
	dlg_editbox_get(ctrl, dlg, lcp,
			sizeof(lcp) / sizeof(TCHAR));
	wcstombs(cfg->line_codepage, lcp, sizeof(cfg->line_codepage));
#else
	dlg_editbox_get(ctrl, dlg, cfg->line_codepage,
			sizeof(cfg->line_codepage));
#endif
	strcpy(cfg->line_codepage, cp_name(decode_codepage(cfg->line_codepage)));
    }
}
*/

static void codepage_handler(union control *ctrl, void *dlg,
			     void *data, int event)
{
    Config *cfg = (Config *)data;
    if (event == EVENT_REFRESH) {
	int i;
	TCHAR *cp;
	dlg_update_start(ctrl, dlg);
	strcpy(cfg->line_codepage,
	    cp_name(decode_codepage(cfg->line_codepage)));
	dlg_listbox_clear(ctrl, dlg);
	for (i = 0; (cp = _str2tcs(cp_enumerate(i))) != NULL; i++) {
	    dlg_listbox_add(ctrl, dlg, cp);
	    _tsfree(cp);
	}
	cp = _str2tcs(cfg->line_codepage);
	dlg_editbox_set(ctrl, dlg, cp);
	_tsfree(cp);
	dlg_update_done(ctrl, dlg);
    } else if (event == EVENT_VALCHANGE) {
#ifdef _UNICODE
	wchar_t *cfg_line_codepage = snewn(sizeof(cfg->line_codepage), wchar_t);
	dlg_editbox_get(ctrl, dlg, cfg_line_codepage, sizeof(cfg->line_codepage));
	wcstombs(cfg->line_codepage, cfg_line_codepage, sizeof(cfg->line_codepage));
	sfree(cfg_line_codepage);
#else
	dlg_editbox_get(ctrl, dlg, cfg->line_codepage,
			sizeof(cfg->line_codepage));
#endif
	strcpy(cfg->line_codepage,
	       cp_name(decode_codepage(cfg->line_codepage)));
    }
}


static void sshbug_handler(union control *ctrl, void *dlg,
			   void *data, int event)
{
    if (event == EVENT_REFRESH) {
	dlg_update_start(ctrl, dlg);
	dlg_listbox_clear(ctrl, dlg);
	dlg_listbox_addwithid(ctrl, dlg, _T("Auto"), AUTO);
	dlg_listbox_addwithid(ctrl, dlg, _T("Off"), FORCE_OFF);
	dlg_listbox_addwithid(ctrl, dlg, _T("On"), FORCE_ON);
	switch (*(int *)ATOFFSET(data, ctrl->listbox.context.i)) {
	  case AUTO:      dlg_listbox_select(ctrl, dlg, 0); break;
	  case FORCE_OFF: dlg_listbox_select(ctrl, dlg, 1); break;
	  case FORCE_ON:  dlg_listbox_select(ctrl, dlg, 2); break;
	}
	dlg_update_done(ctrl, dlg);
    } else if (event == EVENT_SELCHANGE) {
	int i = dlg_listbox_index(ctrl, dlg);
	if (i < 0)
	    i = AUTO;
	else
	    i = dlg_listbox_getid(ctrl, dlg, i);
	*(int *)ATOFFSET(data, ctrl->listbox.context.i) = i;
    }
}

#define SAVEDSESSION_LEN 2048

struct sessionsaver_data {
    union control *editbox, *listbox, *loadbutton, *savebutton, *delbutton;
    union control *okbutton, *cancelbutton;
    struct sesslist *sesslist;
};

/* 
 * Helper function to load the session selected in the list box, if
 * any, as this is done in more than one place below. Returns 0 for
 * failure.
 */
static int load_selected_session(struct sessionsaver_data *ssd,
				 TCHAR *savedsession,
				 void *dlg, Config *cfg)
{
    int i = dlg_listbox_index(ssd->listbox, dlg);
    int isdef;
    if (i < 0) {
	dlg_beep(dlg);
	return 0;
    }
    isdef = !_tcscmp(ssd->sesslist->sessions[i], _T("Default Settings"));
    load_settings(ssd->sesslist->sessions[i], !isdef, cfg);
    if (!isdef) {
		_tcsncpy(savedsession, ssd->sesslist->sessions[i], SAVEDSESSION_LEN);
		savedsession[SAVEDSESSION_LEN-1] = _T('\0');
    } else {
		savedsession[0] = _T('\0');
    }
    dlg_refresh(NULL, dlg);
    /* Restore the selection, which might have been clobbered by
     * changing the value of the edit box. */
    dlg_listbox_select(ssd->listbox, dlg, i);
    return 1;
}

static void sessionsaver_handler(union control *ctrl, void *dlg,
				 void *data, int event)
{
    Config *cfg = (Config *)data;
    struct sessionsaver_data *ssd =
	(struct sessionsaver_data *)ctrl->generic.context.p;
    TCHAR *savedsession;

    /*
     * The first time we're called in a new dialog, we must
     * allocate space to store the current contents of the saved
     * session edit box (since it must persist even when we switch
     * panels, but is not part of the Config).
     * 
     * Of course, this doesn't need to be done mid-session.
     */
    if (!ssd->editbox) {
        savedsession = NULL;
    } else if (!dlg_get_privdata(ssd->editbox, dlg)) {
		savedsession = (TCHAR *)
			dlg_alloc_privdata(ssd->editbox, dlg, SAVEDSESSION_LEN);
		savedsession[0] = _T('\0');
    } else {
		savedsession = dlg_get_privdata(ssd->editbox, dlg);
    }

    if (event == EVENT_REFRESH) {
	if (ctrl == ssd->editbox) {
	    dlg_editbox_set(ctrl, dlg, savedsession);
	} else if (ctrl == ssd->listbox) {
	    int i;
	    dlg_update_start(ctrl, dlg);
	    dlg_listbox_clear(ctrl, dlg);
	    for (i = 0; i < ssd->sesslist->nsessions; i++)
		dlg_listbox_add(ctrl, dlg, ssd->sesslist->sessions[i]);
	    dlg_update_done(ctrl, dlg);
	}
    } else if (event == EVENT_VALCHANGE) {
	if (ctrl == ssd->editbox) {
	    dlg_editbox_get(ctrl, dlg, savedsession,
			    SAVEDSESSION_LEN);
	}
    } else if (event == EVENT_ACTION) {
	if (ctrl == ssd->listbox || ctrl == ssd->loadbutton) {
	    /*
	     * The user has double-clicked a session, or hit Load.
	     * We must load the selected session, and then
	     * terminate the configuration dialog _if_ there was a
	     * double-click on the list box _and_ that session
	     * contains a hostname.
	     */
	    if (load_selected_session(ssd, savedsession, dlg, cfg) &&
		(ctrl == ssd->listbox && cfg->host[0])) {
		dlg_end(dlg, 1);       /* it's all over, and succeeded */
	    }
	} else if (ctrl == ssd->savebutton) {
	    int isdef = !_tcscmp(savedsession, _T("Default Settings"));
	    if (!savedsession[0]) {
		int i = dlg_listbox_index(ssd->listbox, dlg);
		if (i < 0) {
		    dlg_beep(dlg);
		    return;
		}
		isdef = !_tcscmp(ssd->sesslist->sessions[i], _T("Default Settings"));
		if (!isdef) {
		    _tcsncpy(savedsession, ssd->sesslist->sessions[i],
			    SAVEDSESSION_LEN);
		    savedsession[SAVEDSESSION_LEN-1] = _T('\0');
		} else {
		    savedsession[0] = _T('\0');
		}
	    }
            {
                TCHAR *errmsg = save_settings(savedsession, !isdef, cfg);
                if (errmsg) {
                    dlg_error_msg(dlg, errmsg);
                    sfree(errmsg);
                }
            }
	    get_sesslist(ssd->sesslist, FALSE);
	    get_sesslist(ssd->sesslist, TRUE);
	    dlg_refresh(ssd->editbox, dlg);
	    dlg_refresh(ssd->listbox, dlg);
	} else if (ctrl == ssd->delbutton) {
	    int i = dlg_listbox_index(ssd->listbox, dlg);
	    if (i <= 0) {
		dlg_beep(dlg);
	    } else {
		del_settings(ssd->sesslist->sessions[i]);
		get_sesslist(ssd->sesslist, FALSE);
		get_sesslist(ssd->sesslist, TRUE);
		dlg_refresh(ssd->listbox, dlg);
	    }
	} else if (ctrl == ssd->okbutton) {
            if (!savedsession) {
                /* In a mid-session Change Settings, Apply is always OK. */
		dlg_end(dlg, 1);
                return;
            }
	    /*
	     * Annoying special case. If the `Open' button is
	     * pressed while no host name is currently set, _and_
	     * the session list previously had the focus, _and_
	     * there was a session selected in that which had a
	     * valid host name in it, then load it and go.
	     */
	    if (dlg_last_focused(ctrl, dlg) == ssd->listbox && !*cfg->host) {
		Config cfg2;
		if (!load_selected_session(ssd, savedsession, dlg, &cfg2)) {
		    dlg_beep(dlg);
		    return;
		}
		/* If at this point we have a valid session, go! */
		if (*cfg2.host) {
		    *cfg = cfg2;       /* structure copy */
		    dlg_end(dlg, 1);
		} else
		    dlg_beep(dlg);
	    }

	    /*
	     * Otherwise, do the normal thing: if we have a valid
	     * session, get going.
	     */
	    if (*cfg->host) {
		dlg_end(dlg, 1);
	    } else
		dlg_beep(dlg);
	} else if (ctrl == ssd->cancelbutton) {
	    dlg_end(dlg, 0);
	}
    }
}

struct charclass_data {
    union control *listbox, *editbox, *button;
};

static void charclass_handler(union control *ctrl, void *dlg,
			      void *data, int event)
{
    Config *cfg = (Config *)data;
    struct charclass_data *ccd =
	(struct charclass_data *)ctrl->generic.context.p;

    if (event == EVENT_REFRESH) {
	if (ctrl == ccd->listbox) {
	    int i;
	    dlg_update_start(ctrl, dlg);
	    dlg_listbox_clear(ctrl, dlg);
	    for (i = 0; i < 128; i++) {
		TCHAR str[100];
		_stprintf(str, _T("%d\t(0x%02X)\t%c\t%d"), i, i,
			(i >= 0x21 && i != 0x7F) ? _tchar(i) : _T(' '), cfg->wordness[i]);
		dlg_listbox_add(ctrl, dlg, str);
	    }
	    dlg_update_done(ctrl, dlg);
	}
    } else if (event == EVENT_ACTION) {
	if (ctrl == ccd->button) {
	    TCHAR str[100];
	    int i, n;
	    dlg_editbox_get(ccd->editbox, dlg, str, sizeof(str) / sizeof(TCHAR));
	    n = _ttoi(str);
	    for (i = 0; i < 128; i++) {
		if (dlg_listbox_issel(ccd->listbox, dlg, i))
		    cfg->wordness[i] = n;
	    }
	    dlg_refresh(ccd->listbox, dlg);
	}
    }
}

struct colour_data {
    union control *listbox, *redit, *gedit, *bedit, *button;
};

static const TCHAR *const colours[] = {
    _T("Default Foreground"), _T("Default Bold Foreground"),
    _T("Default Background"), _T("Default Bold Background"),
    _T("Cursor Text"), _T("Cursor Colour"),
    _T("ANSI Black"), _T("ANSI Black Bold"),
    _T("ANSI Red"), _T("ANSI Red Bold"),
    _T("ANSI Green"), _T("ANSI Green Bold"),
    _T("ANSI Yellow"), _T("ANSI Yellow Bold"),
    _T("ANSI Blue"), _T("ANSI Blue Bold"),
    _T("ANSI Magenta"), _T("ANSI Magenta Bold"),
    _T("ANSI Cyan"), _T("ANSI Cyan Bold"),
    _T("ANSI White"), _T("ANSI White Bold")
};

static void colour_handler(union control *ctrl, void *dlg,
			    void *data, int event)
{
    Config *cfg = (Config *)data;
    struct colour_data *cd =
	(struct colour_data *)ctrl->generic.context.p;
    int update = FALSE, r, g, b;

    if (event == EVENT_REFRESH) {
	if (ctrl == cd->listbox) {
	    int i;
	    dlg_update_start(ctrl, dlg);
	    dlg_listbox_clear(ctrl, dlg);
	    for (i = 0; i < lenof(colours); i++)
		dlg_listbox_add(ctrl, dlg, colours[i]);
	    dlg_update_done(ctrl, dlg);
	    dlg_editbox_set(cd->redit, dlg, _T(""));
	    dlg_editbox_set(cd->gedit, dlg, _T(""));
	    dlg_editbox_set(cd->bedit, dlg, _T(""));
	}
    } else if (event == EVENT_SELCHANGE) {
	if (ctrl == cd->listbox) {
	    /* The user has selected a colour. Update the RGB text. */
	    int i = dlg_listbox_index(ctrl, dlg);
	    if (i < 0) {
		dlg_beep(dlg);
		return;
	    }
	    r = cfg->colours[i][0];
	    g = cfg->colours[i][1];
	    b = cfg->colours[i][2];
	    update = TRUE;
	}
    } else if (event == EVENT_VALCHANGE) {
	if (ctrl == cd->redit || ctrl == cd->gedit || ctrl == cd->bedit) {
	    /* The user has changed the colour using the edit boxes. */
	    TCHAR buf[80];
	    int i, cval;

	    dlg_editbox_get(ctrl, dlg, buf, lenof(buf));
	    cval = _ttoi(buf) & 255;

	    i = dlg_listbox_index(cd->listbox, dlg);
	    if (i >= 0) {
		if (ctrl == cd->redit)
		    cfg->colours[i][0] = cval;
		else if (ctrl == cd->gedit)
		    cfg->colours[i][1] = cval;
		else if (ctrl == cd->bedit)
		    cfg->colours[i][2] = cval;
	    }
	}
    } else if (event == EVENT_ACTION) {
	if (ctrl == cd->button) {
	    int i = dlg_listbox_index(cd->listbox, dlg);
	    if (i < 0) {
		dlg_beep(dlg);
		return;
	    }
	    /*
	     * Start a colour selector, which will send us an
	     * EVENT_CALLBACK when it's finished and allow us to
	     * pick up the results.
	     */
	    dlg_coloursel_start(ctrl, dlg,
				cfg->colours[i][0],
				cfg->colours[i][1],
				cfg->colours[i][2]);
	}
    } else if (event == EVENT_CALLBACK) {
	if (ctrl == cd->button) {
	    int i = dlg_listbox_index(cd->listbox, dlg);
	    /*
	     * Collect the results of the colour selector. Will
	     * return nonzero on success, or zero if the colour
	     * selector did nothing (user hit Cancel, for example).
	     */
	    if (dlg_coloursel_results(ctrl, dlg, &r, &g, &b)) {
		cfg->colours[i][0] = r;
		cfg->colours[i][1] = g;
		cfg->colours[i][2] = b;
		update = TRUE;
	    }
	}
    }

    if (update) {
	TCHAR buf[40];
	_stprintf(buf, _T("%d"), r); dlg_editbox_set(cd->redit, dlg, buf);
	_stprintf(buf, _T("%d"), g); dlg_editbox_set(cd->gedit, dlg, buf);
	_stprintf(buf, _T("%d"), b); dlg_editbox_set(cd->bedit, dlg, buf);
    }
}

struct environ_data {
    union control *varbox, *valbox, *addbutton, *rembutton, *listbox;
};

static void environ_handler(union control *ctrl, void *dlg,
			    void *data, int event)
{
    Config *cfg = (Config *)data;
    struct environ_data *ed =
	(struct environ_data *)ctrl->generic.context.p;

    if (event == EVENT_REFRESH) {
	if (ctrl == ed->listbox) {
	    char *p = cfg->environmt;
	    dlg_update_start(ctrl, dlg);
	    dlg_listbox_clear(ctrl, dlg);
	    while (*p) {
#ifdef _UNICODE
			wchar_t *wp = ansi2unicode(p);
			dlg_listbox_add(ctrl, dlg, wp);
			sfree(wp);
#else
			dlg_listbox_add(ctrl, dlg, p);
#endif
			p += strlen(p) + 1;
	    }
	    dlg_update_done(ctrl, dlg);
	}
    } else if (event == EVENT_ACTION) {
	if (ctrl == ed->addbutton) {
		// Aleq
	    TCHAR str[sizeof(cfg->environmt)];
	    TCHAR *p;
	    char *p2;
	    dlg_editbox_get(ed->varbox, dlg, str, sizeof(str) / sizeof(TCHAR) - 1);
	    if (!*str) {
			dlg_beep(dlg);
			return;
	    }
	    p = str + _tcslen(str);
	    *p++ = _T('\t');
	    dlg_editbox_get(ed->valbox, dlg, p, sizeof(str) / sizeof(TCHAR) - 1 - (p - str));
	    if (!*p) {
		dlg_beep(dlg);
		return;
	    }

	    p2 = cfg->environmt;
	    while (*p2) {
		while (*p2)
		    p2++;
		p2++;
	    }
	    if ((p2 - cfg->environmt) + _tcslen(str) + 2 <
		sizeof(cfg->environmt)) {
#ifdef _UNICODE
		wcstombs(p2, str, (cfg->environmt - p2 + sizeof(cfg->environmt)));
#else
		strcpy(p2, str);
#endif
		p2[_tcslen(str) + 1] = '\0';
		dlg_listbox_add(ed->listbox, dlg, str);
		dlg_editbox_set(ed->varbox, dlg, _T(""));
		dlg_editbox_set(ed->valbox, dlg, _T(""));
	    } else {
			dlg_error_msg(dlg, _T("Environment too big"));
	    }
	} else if (ctrl == ed->rembutton) {
	    int i = dlg_listbox_index(ed->listbox, dlg);
	    if (i < 0) {
			dlg_beep(dlg);
	    } else {
		char *p, *q;

		dlg_listbox_del(ed->listbox, dlg, i);
		p = cfg->environmt;
		while (i > 0) {
		    if (!*p)
			goto disaster;
		    while (*p)
			p++;
		    p++;
		    i--;
		}
		q = p;
		if (!*p)
		    goto disaster;
		while (*p)
		    p++;
		p++;
		while (*p) {
		    while (*p)
			*q++ = *p++;
		    *q++ = *p++;
		}
		*q = '\0';
		disaster:;
	    }
	}
    }
}

struct portfwd_data {
    union control *addbutton, *rembutton, *listbox;
    union control *sourcebox, *destbox, *direction;
};

static void portfwd_handler(union control *ctrl, void *dlg,
			    void *data, int event)
{
    Config *cfg = (Config *)data;
    struct portfwd_data *pfd =
	(struct portfwd_data *)ctrl->generic.context.p;

    if (event == EVENT_REFRESH) {
	if (ctrl == pfd->listbox) {
	    TCHAR *p = _str2tcs(cfg->portfwd);
	    dlg_update_start(ctrl, dlg);
	    dlg_listbox_clear(ctrl, dlg);
	    while (*p) {
		dlg_listbox_add(ctrl, dlg, p);
		p += _tcslen(p) + 1; // sizeof(TCHAR);
	    }
	    dlg_update_done(ctrl, dlg);
	    _tsfree(p);
	} else if (ctrl == pfd->direction) {
	    /*
	     * Default is Local.
	     */
	    dlg_radiobutton_set(ctrl, dlg, 0);
	}
    } else if (event == EVENT_ACTION) {
	if (ctrl == pfd->addbutton) {
	    TCHAR str[sizeof(cfg->portfwd) / sizeof(TCHAR)];
	    TCHAR *p;
	    TCHAR *origp;
	    int whichbutton = dlg_radiobutton_get(pfd->direction, dlg);
	    if (whichbutton == 0)
		str[0] = _T('L');
	    else if (whichbutton == 1)
		str[0] = _T('R');
	    else
		str[0] = _T('D');
	    dlg_editbox_get(pfd->sourcebox, dlg, str+sizeof(TCHAR), sizeof(str) - 2 * sizeof(TCHAR));
	    if (!str[1]) {
		dlg_error_msg(dlg, _T("You need to specify a source port number"));
		return;
	    }
	    p = str + _tcslen(str);
	    if (str[0] != _T('D')) {
		*p++ = _T('\t');
		dlg_editbox_get(pfd->destbox, dlg, p,
				sizeof(str) / sizeof(TCHAR) - 1 - (p - str) / sizeof(TCHAR));
		if (!*p || !_tcschr(p, _T(':'))) {
		    dlg_error_msg(dlg,
				  _T("You need to specify a destination address\n")
				  _T("in the form \"host.name:port\""));
		    return;
		}
	    } else
		*p = _T('\0');
	    origp = p = _str2tcs(cfg->portfwd);
	    while (*p) {
		while (*p)
		    p++;
		p++;
	    }
	    if ((p - origp/*cfg->portfwd*/) /* * sizeof(TCHAR)*/ + _tcslen(str) + 2 * sizeof(TCHAR) <
		sizeof(origp/*cfg->portfwd*/) / sizeof(TCHAR)) {
		_tcscpy(p, str);
		p[_tcslen(str) + 1] = _T('\0');
		dlg_listbox_add(pfd->listbox, dlg, str);
		dlg_editbox_set(pfd->sourcebox, dlg, _T(""));
		dlg_editbox_set(pfd->destbox, dlg, _T(""));
	    } else {
		dlg_error_msg(dlg, _T("Too many forwardings"));
	    }
	    _tsfree(origp);
	} else if (ctrl == pfd->rembutton) {
	    int i = dlg_listbox_index(pfd->listbox, dlg);
	    if (i < 0)
		dlg_beep(dlg);
	    else {
		TCHAR *p, *q, *origp;

		dlg_listbox_del(pfd->listbox, dlg, i);
		origp = p = _str2tcs(cfg->portfwd);
		while (i > 0) {
		    if (!*p)
			goto disaster2;
		    while (*p)
			p++;
		    p++;
		    i--;
		}
		q = p;
		if (!*p)
		    goto disaster2;
		while (*p)
		    p++;
		p++;
		while (*p) {
		    while (*p)
			*q++ = *p++;
		    *q++ = *p++;
		}
		*q = _T('\0');
		disaster2:;
		_tsfree(p);
	    }
	}
    }
}

void setup_config_box(struct controlbox *b, struct sesslist *sesslist,
		      int midsession, int protocol)
{
    struct controlset *s;
    struct sessionsaver_data *ssd;
    struct charclass_data *ccd;
    struct colour_data *cd;
    struct environ_data *ed;
    struct portfwd_data *pfd;
    union control *c;
    TCHAR *str;

    ssd = (struct sessionsaver_data *)
	ctrl_alloc(b, sizeof(struct sessionsaver_data));
    memset(ssd, 0, sizeof(*ssd));
    ssd->sesslist = (midsession ? NULL : sesslist);

    /*
     * The standard panel that appears at the bottom of all panels:
     * Open, Cancel, Apply etc.
     */
    s = ctrl_getset(b, _T(""), _T(""), _T(""));
    ctrl_columns(s, 5, 20, 20, 20, 20, 20);
    ssd->okbutton = ctrl_pushbutton(s,
				    (midsession ? _T("Apply") : _T("Open")),
				    (char)(midsession ? _T('a') : _T('o')),
					HELPCTX(no_help), 
				    sessionsaver_handler, P(ssd));
    ssd->okbutton->button.isdefault = TRUE;
    ssd->okbutton->generic.column = 3;
    ssd->cancelbutton = ctrl_pushbutton(s, _T("Cancel"), _T('c'), HELPCTX(no_help),
					sessionsaver_handler, P(ssd));
    ssd->cancelbutton->button.iscancel = TRUE;
    ssd->cancelbutton->generic.column = 4;
    /* We carefully don't close the 5-column part, so that platform-
     * specific add-ons can put extra buttons alongside Open and Cancel. */

    /*
     * The Session panel.
     */
    str = _duptprintf(_T("Basic options for your %s session"), appname);
    ctrl_settitle(b, _T("Session"), str);
    sfree(str);

    if (!midsession) {
	s = ctrl_getset(b, _T("Session"), _T("hostport"),
			_T("Specify your connection by host name or IP address"));
	ctrl_columns(s, 2, 75, 25);
	c = ctrl_editbox(s, _T("Host Name (or IP address)"), _T('n'), 100,
			 HELPCTX(session_hostname),
			 dlg_stdeditbox_handler, I(offsetof(Config,host)),
			 I(sizeof(((Config *)0)->host)));
	c->generic.column = 0;
	c = ctrl_editbox(s, _T("Port"), _T('p'), 100, HELPCTX(session_hostname),
			 dlg_stdeditbox_handler,
			 I(offsetof(Config,port)), I(-1));
	c->generic.column = 1;
	ctrl_columns(s, 1, 100);
	if (backends[3].name == NULL) {
	    ctrl_radiobuttons(s, _T("Protocol:"), NO_SHORTCUT, 3,
			      HELPCTX(session_hostname),
			      protocolbuttons_handler, P(c),
			      _T("Raw"), _T('r'), I(PROT_RAW),
			      _T("Telnet"), _T('t'), I(PROT_TELNET),
			      _T("Rlogin"), _T('i'), I(PROT_RLOGIN),
			      NULL);
	} else {
	    ctrl_radiobuttons(s, _T("Protocol:"), NO_SHORTCUT, 4,
			      HELPCTX(session_hostname),
			      protocolbuttons_handler, P(c),
			      _T("Raw"), _T('r'), I(PROT_RAW),
			      _T("Telnet"), _T('t'), I(PROT_TELNET),
			      _T("Rlogin"), _T('i'), I(PROT_RLOGIN),
			      _T("SSH"), _T('s'), I(PROT_SSH),
			      NULL);
	}

	s = ctrl_getset(b, _T("Session"), _T("savedsessions"),
			_T("Load, save or delete a stored session"));
	ctrl_columns(s, 2, 75, 25);
	ssd->sesslist = sesslist;
	ssd->editbox = ctrl_editbox(s, _T("Saved Sessions"), _T('e'), 100,
				    HELPCTX(session_saved),
				    sessionsaver_handler, P(ssd), P(NULL));
	ssd->editbox->generic.column = 0;
	/* Reset columns so that the buttons are alongside the list, rather
	 * than alongside that edit box. */
	ctrl_columns(s, 1, 100);
	ctrl_columns(s, 2, 75, 25);
	ssd->listbox = ctrl_listbox(s, NULL, NO_SHORTCUT,
				    HELPCTX(session_saved),
				    sessionsaver_handler, P(ssd));
	ssd->listbox->generic.column = 0;
	ssd->listbox->listbox.height = 7;
	ssd->loadbutton = ctrl_pushbutton(s, _T("Load"), _T('l'),
					  HELPCTX(session_saved),
					  sessionsaver_handler, P(ssd));
	ssd->loadbutton->generic.column = 1;
	ssd->savebutton = ctrl_pushbutton(s, _T("Save"), _T('v'),
					  HELPCTX(session_saved),
					  sessionsaver_handler, P(ssd));
	ssd->savebutton->generic.column = 1;
	ssd->delbutton = ctrl_pushbutton(s, _T("Delete"), _T('d'),
					 HELPCTX(session_saved),
					 sessionsaver_handler, P(ssd));
	ssd->delbutton->generic.column = 1;
	ctrl_columns(s, 1, 100);
    }

    s = ctrl_getset(b, _T("Session"), _T("otheropts"), NULL);
    c = ctrl_radiobuttons(s, _T("Close window on exit:"), _T('w'), 4,
			  HELPCTX(session_coe),
			  dlg_stdradiobutton_handler,
			  I(offsetof(Config, close_on_exit)),
			  _T("Always"), I(FORCE_ON),
			  _T("Never"), I(FORCE_OFF),
			  _T("Only on clean exit"), I(AUTO), NULL);

    /*
     * The Session/Logging panel.
     */
    ctrl_settitle(b, _T("Session/Logging"), _T("Options controlling session logging"));

    s = ctrl_getset(b, _T("Session/Logging"), _T("main"), NULL);
    /*
     * The logging buttons change depending on whether SSH packet
     * logging can sensibly be available.
     */
    {
	TCHAR *sshlogname;
	if ((midsession && protocol == PROT_SSH) ||
	    (!midsession && backends[3].name != NULL))
	    sshlogname = _T("Log SSH packet data");
	else
	    sshlogname = NULL;	       /* this will disable the button */
	ctrl_radiobuttons(s, _T("Session logging:"), NO_SHORTCUT, 1,
			  HELPCTX(logging_main),
			  dlg_stdradiobutton_handler,
			  I(offsetof(Config, logtype)),
			  _T("Logging turned off completely"), _T('t'), I(LGTYP_NONE),
			  _T("Log printable output only"), _T('p'), I(LGTYP_ASCII),
			  _T("Log all session output"), _T('l'), I(LGTYP_DEBUG),
			  sshlogname, _T('s'), I(LGTYP_PACKETS),
			  NULL);
    }
    ctrl_filesel(s, _T("Log file name:"), _T('f'),
		 NULL, TRUE, _T("Select session log file name"),
		 HELPCTX(logging_filename),
		 dlg_stdfilesel_handler, I(offsetof(Config, logfilename)));
    ctrl_text(s, _T("(Log file name can contain &Y, &M, &D for date,")
	      _T(" &T for time, and &H for host name)"),
	      HELPCTX(logging_filename));
    ctrl_radiobuttons(s, _T("What to do if the log file already exists:"), _T('e'), 1,
		      HELPCTX(logging_exists),
		      dlg_stdradiobutton_handler, I(offsetof(Config,logxfovr)),
		      _T("Always overwrite it"), I(LGXF_OVR),
		      _T("Always append to the end of it"), I(LGXF_APN),
		      _T("Ask the user every time"), I(LGXF_ASK), NULL);

    /*
     * The Terminal panel.
     */
    ctrl_settitle(b, _T("Terminal"), _T("Options controlling the terminal emulation"));

    s = ctrl_getset(b, _T("Terminal"), _T("general"), _T("Set various terminal options"));
    ctrl_checkbox(s, _T("Auto wrap mode initially on"), _T('w'),
		  HELPCTX(terminal_autowrap),
		  dlg_stdcheckbox_handler, I(offsetof(Config,wrap_mode)));
    ctrl_checkbox(s, _T("DEC Origin Mode initially on"), _T('d'),
		  HELPCTX(terminal_decom),
		  dlg_stdcheckbox_handler, I(offsetof(Config,dec_om)));
    ctrl_checkbox(s, _T("Implicit CR in every LF"), _T('r'),
		  HELPCTX(terminal_lfhascr),
		  dlg_stdcheckbox_handler, I(offsetof(Config,lfhascr)));
    ctrl_checkbox(s, _T("Use background colour to erase screen"), _T('e'),
		  HELPCTX(terminal_bce),
		  dlg_stdcheckbox_handler, I(offsetof(Config,bce)));
    ctrl_checkbox(s, _T("Enable blinking text"), _T('n'),
		  HELPCTX(terminal_blink),
		  dlg_stdcheckbox_handler, I(offsetof(Config,blinktext)));
    ctrl_editbox(s, _T("Answerback to ^E:"), _T('s'), 100,
		 HELPCTX(terminal_answerback),
		 dlg_stdeditbox_handler, I(offsetof(Config,answerback)),
		 I(sizeof(((Config *)0)->answerback)));

    s = ctrl_getset(b, _T("Terminal"), _T("ldisc"), _T("Line discipline options"));
    ctrl_radiobuttons(s, _T("Local echo:"), _T('l'), 3,
		      HELPCTX(terminal_localecho),
		      dlg_stdradiobutton_handler,I(offsetof(Config,localecho)),
		      _T("Auto"), I(AUTO),
		      _T("Force on"), I(FORCE_ON),
		      _T("Force off"), I(FORCE_OFF), NULL);
    ctrl_radiobuttons(s, _T("Local line editing:"), _T('t'), 3,
		      HELPCTX(terminal_localedit),
		      dlg_stdradiobutton_handler,I(offsetof(Config,localedit)),
		      _T("Auto"), I(AUTO),
		      _T("Force on"), I(FORCE_ON),
		      _T("Force off"), I(FORCE_OFF), NULL);

#ifndef _WIN32_WCE
    s = ctrl_getset(b, _T("Terminal"), _T("printing"), _T("Remote-controlled printing"));
    ctrl_combobox(s, _T("Printer to send ANSI printer output to:"), _T('p'), 100,
		  HELPCTX(terminal_printing),
		  printerbox_handler, P(NULL), P(NULL));
#endif

    /*
     * The Terminal/Keyboard panel.
     */
    ctrl_settitle(b, _T("Terminal/Keyboard"),
		  _T("Options controlling the effects of keys"));

    s = ctrl_getset(b, _T("Terminal/Keyboard"), _T("mappings"),
		    _T("Change the sequences sent by:"));
    ctrl_radiobuttons(s, _T("The Backspace key"), _T('b'), 2,
		      HELPCTX(keyboard_backspace),
		      dlg_stdradiobutton_handler,
		      I(offsetof(Config, bksp_is_delete)),
		      _T("Control-H"), I(0), _T("Control-? (127)"), I(1), NULL);
    ctrl_radiobuttons(s, _T("The Home and End keys"), _T('e'), 2,
		      HELPCTX(keyboard_homeend),
		      dlg_stdradiobutton_handler,
		      I(offsetof(Config, rxvt_homeend)),
		      _T("Standard"), I(0), _T("rxvt"), I(1), NULL);
    ctrl_radiobuttons(s, _T("The Function keys and keypad"), _T('f'), 3,
		      HELPCTX(keyboard_funkeys),
		      dlg_stdradiobutton_handler,
		      I(offsetof(Config, funky_type)),
		      _T("ESC[n~"), I(0), _T("Linux"), I(1), _T("Xterm R6"), I(2),
		      _T("VT400"), I(3), _T("VT100+"), I(4), _T("SCO"), I(5), NULL);

    s = ctrl_getset(b, _T("Terminal/Keyboard"), _T("appkeypad"),
		    _T("Application keypad settings:"));
    ctrl_radiobuttons(s, _T("Initial state of cursor keys:"), _T('r'), 3,
		      HELPCTX(keyboard_appcursor),
		      dlg_stdradiobutton_handler,
		      I(offsetof(Config, app_cursor)),
		      _T("Normal"), I(0), _T("Application"), I(1), NULL);
    ctrl_radiobuttons(s, _T("Initial state of numeric keypad:"), _T('n'), 3,
		      HELPCTX(keyboard_appkeypad),
		      numeric_keypad_handler, P(NULL),
		      _T("Normal"), I(0), _T("Application"), I(1), _T("NetHack"), I(2),
		      NULL);

    /*
     * The Terminal/Bell panel.
     */
    ctrl_settitle(b, _T("Terminal/Bell"),
		  _T("Options controlling the terminal bell"));

    s = ctrl_getset(b, _T("Terminal/Bell"), _T("style"), _T("Set the style of bell"));
    ctrl_radiobuttons(s, _T("Action to happen when a bell occurs:"), _T('b'), 1,
		      HELPCTX(bell_style),
		      dlg_stdradiobutton_handler, I(offsetof(Config, beep)),
		      _T("None (bell disabled)"), I(BELL_DISABLED),
		      _T("Make default system alert sound"), I(BELL_DEFAULT),
		      _T("Visual bell (flash window)"), I(BELL_VISUAL), NULL);

    s = ctrl_getset(b, _T("Terminal/Bell"), _T("overload"),
		    _T("Control the bell overload behaviour"));
    ctrl_checkbox(s, _T("Bell is temporarily disabled when over-used"), _T('d'),
		  HELPCTX(bell_overload),
		  dlg_stdcheckbox_handler, I(offsetof(Config,bellovl)));
    ctrl_editbox(s, _T("Over-use means this many bells..."), _T('m'), 20,
		 HELPCTX(bell_overload),
		 dlg_stdeditbox_handler, I(offsetof(Config,bellovl_n)), I(-1));
    ctrl_editbox(s, _T("... in this many seconds"), _T('t'), 20,
		 HELPCTX(bell_overload),
		 dlg_stdeditbox_handler, I(offsetof(Config,bellovl_t)),
		 I(-TICKSPERSEC));
    ctrl_text(s, _T("The bell is re-enabled after a few seconds of silence."),
	      HELPCTX(bell_overload));
    ctrl_editbox(s, _T("Seconds of silence required"), _T('s'), 20,
		 HELPCTX(bell_overload),
		 dlg_stdeditbox_handler, I(offsetof(Config,bellovl_s)),
		 I(-TICKSPERSEC));

    /*
     * The Terminal/Features panel.
     */
    ctrl_settitle(b, _T("Terminal/Features"),
		  _T("Enabling and disabling advanced terminal features"));

    s = ctrl_getset(b, _T("Terminal/Features"), _T("main"), NULL);
    ctrl_checkbox(s, _T("Disable application cursor keys mode"), _T('u'),
		  HELPCTX(features_application),
		  dlg_stdcheckbox_handler, I(offsetof(Config,no_applic_c)));
    ctrl_checkbox(s, _T("Disable application keypad mode"), _T('k'),
		  HELPCTX(features_application),
		  dlg_stdcheckbox_handler, I(offsetof(Config,no_applic_k)));
    ctrl_checkbox(s, _T("Disable xterm-style mouse reporting"), _T('x'),
		  HELPCTX(features_mouse),
		  dlg_stdcheckbox_handler, I(offsetof(Config,no_mouse_rep)));
    ctrl_checkbox(s, _T("Disable remote-controlled terminal resizing"), _T('s'),
		  HELPCTX(features_resize),
		  dlg_stdcheckbox_handler,
		  I(offsetof(Config,no_remote_resize)));
    ctrl_checkbox(s, _T("Disable switching to alternate terminal screen"), _T('w'),
		  HELPCTX(features_altscreen),
		  dlg_stdcheckbox_handler, I(offsetof(Config,no_alt_screen)));
    ctrl_checkbox(s, _T("Disable remote-controlled window title changing"), _T('t'),
		  HELPCTX(features_retitle),
		  dlg_stdcheckbox_handler,
		  I(offsetof(Config,no_remote_wintitle)));
    ctrl_checkbox(s, _T("Disable remote window title querying (SECURITY)"),
		  _T('q'), HELPCTX(features_qtitle), dlg_stdcheckbox_handler,
		  I(offsetof(Config,no_remote_qtitle)));
    ctrl_checkbox(s, _T("Disable destructive backspace on server sending ^?"), _T('b'),
		  HELPCTX(features_dbackspace),
		  dlg_stdcheckbox_handler, I(offsetof(Config,no_dbackspace)));
    ctrl_checkbox(s, _T("Disable remote-controlled character set configuration"),
		  _T('r'), HELPCTX(features_charset), dlg_stdcheckbox_handler,
		  I(offsetof(Config,no_remote_charset)));

    /*
     * The Window panel.
     */
    str = _duptprintf(_T("Options controlling %s's window"), appname);
    ctrl_settitle(b, _T("Window"), str);
    sfree(str);

    s = ctrl_getset(b, _T("Window"), _T("size"), _T("Set the size of the window"));
    ctrl_columns(s, 2, 50, 50);
    c = ctrl_editbox(s, _T("Rows"), _T('r'), 100,
		     HELPCTX(window_size),
		     dlg_stdeditbox_handler, I(offsetof(Config,height)),I(-1));
    c->generic.column = 0;
    c = ctrl_editbox(s, _T("Columns"), _T('m'), 100,
		     HELPCTX(window_size),
		     dlg_stdeditbox_handler, I(offsetof(Config,width)), I(-1));
    c->generic.column = 1;
    ctrl_columns(s, 1, 100);

    s = ctrl_getset(b, _T("Window"), _T("scrollback"),
		    _T("Control the scrollback in the window"));
    ctrl_editbox(s, _T("Lines of scrollback"), _T('s'), 50,
		 HELPCTX(window_scrollback),
		 dlg_stdeditbox_handler, I(offsetof(Config,savelines)), I(-1));
    ctrl_checkbox(s, _T("Display scrollbar"), _T('d'),
		  HELPCTX(window_scrollback),
		  dlg_stdcheckbox_handler, I(offsetof(Config,scrollbar)));
    ctrl_checkbox(s, _T("Reset scrollback on keypress"), _T('k'),
		  HELPCTX(window_scrollback),
		  dlg_stdcheckbox_handler, I(offsetof(Config,scroll_on_key)));
    ctrl_checkbox(s, _T("Reset scrollback on display activity"), _T('p'),
		  HELPCTX(window_scrollback),
		  dlg_stdcheckbox_handler, I(offsetof(Config,scroll_on_disp)));
    ctrl_checkbox(s, _T("Push erased text into scrollback"), _T('e'),
		  HELPCTX(window_erased),
		  dlg_stdcheckbox_handler,
		  I(offsetof(Config,erase_to_scrollback)));

    /*
     * The Window/Appearance panel.
     */
    str = _duptprintf(_T("Configure the appearance of %s's window"), appname);
    ctrl_settitle(b, _T("Window/Appearance"), str);
    sfree(str);

    s = ctrl_getset(b, _T("Window/Appearance"), _T("cursor"),
		    _T("Adjust the use of the cursor"));
    ctrl_radiobuttons(s, _T("Cursor appearance:"), NO_SHORTCUT, 3,
		      HELPCTX(appearance_cursor),
		      dlg_stdradiobutton_handler,
		      I(offsetof(Config, cursor_type)),
		      _T("Block"), _T('l'), I(0),
		      _T("Underline"), _T('u'), I(1),
		      _T("Vertical line"), _T('v'), I(2), NULL);
    ctrl_checkbox(s, _T("Cursor blinks"), _T('b'),
		  HELPCTX(appearance_cursor),
		  dlg_stdcheckbox_handler, I(offsetof(Config,blink_cur)));

    s = ctrl_getset(b, _T("Window/Appearance"), _T("font"),
		    _T("Font settings"));
    ctrl_fontsel(s, _T("Font used in the terminal window"), _T('n'),
		 HELPCTX(appearance_font),
		 dlg_stdfontsel_handler, I(offsetof(Config, font)));

    s = ctrl_getset(b, _T("Window/Appearance"), _T("mouse"),
		    _T("Adjust the use of the mouse pointer"));
    ctrl_checkbox(s, _T("Hide mouse pointer when typing in window"), _T('p'),
		  HELPCTX(appearance_hidemouse),
		  dlg_stdcheckbox_handler, I(offsetof(Config,hide_mouseptr)));

    s = ctrl_getset(b, _T("Window/Appearance"), _T("border"),
		    _T("Adjust the window border"));
    ctrl_editbox(s, _T("Gap between text and window edge:"), NO_SHORTCUT, 20,
		 HELPCTX(appearance_border),
		 dlg_stdeditbox_handler,
		 I(offsetof(Config,window_border)), I(-1));

    /*
     * The Window/Behaviour panel.
     */
    str = _duptprintf(_T("Configure the behaviour of %s's window"), appname);
    ctrl_settitle(b, _T("Window/Behaviour"), str);
    sfree(str);

    s = ctrl_getset(b, _T("Window/Behaviour"), _T("title"),
		    _T("Adjust the behaviour of the window title"));
    ctrl_editbox(s, _T("Window title:"), _T('t'), 100,
		 HELPCTX(appearance_title),
		 dlg_stdeditbox_handler, I(offsetof(Config,wintitle)),
		 I(sizeof(((Config *)0)->wintitle)));
    ctrl_checkbox(s, _T("Separate window and icon titles"), _T('i'),
		  HELPCTX(appearance_title),
		  dlg_stdcheckbox_handler,
		  I(CHECKBOX_INVERT | offsetof(Config,win_name_always)));

    s = ctrl_getset(b, _T("Window/Behaviour"), _T("main"), NULL);
    ctrl_checkbox(s, _T("Warn before closing window"), _T('w'),
		  HELPCTX(behaviour_closewarn),
		  dlg_stdcheckbox_handler, I(offsetof(Config,warn_on_close)));

    /*
     * The Window/Translation panel.
     */
    ctrl_settitle(b, _T("Window/Translation"),
		  _T("Options controlling character set translation"));

    s = ctrl_getset(b, _T("Window/Translation"), _T("trans"),
		    _T("Character set translation on received data"));
    ctrl_combobox(s, _T("Received data assumed to be in which character set:"),
		  _T('r'), 100, HELPCTX(translation_codepage),
		  codepage_handler, P(NULL), P(NULL));

    str = _duptprintf(_T("Adjust how %s displays line drawing characters"), appname);
    s = ctrl_getset(b, _T("Window/Translation"), _T("linedraw"), str);
    sfree(str);
    ctrl_radiobuttons(s, _T("Handling of line drawing characters:"), NO_SHORTCUT,1,
		      HELPCTX(translation_linedraw),
		      dlg_stdradiobutton_handler,
		      I(offsetof(Config, vtmode)),
		      _T("Use Unicode line drawing code points"),_T('u'),I(VT_UNICODE),
		      _T("Poor man's line drawing (+, - and |)"),_T('p'),I(VT_POORMAN),
		      NULL);

    /*
     * The Window/Selection panel.
     */
    ctrl_settitle(b, _T("Window/Selection"), _T("Options controlling copy and paste"));

    s = ctrl_getset(b, _T("Window/Selection"), _T("trans"),
		    _T("Translation of pasted characters"));
    ctrl_checkbox(s, _T("Paste VT100 line drawing chars as lqqqk"),_T('d'),
		  HELPCTX(selection_linedraw),
		  dlg_stdcheckbox_handler, I(offsetof(Config,rawcnp)));
	
    s = ctrl_getset(b, _T("Window/Selection"), _T("mouse"),
		    _T("Control use of mouse"));
    ctrl_checkbox(s, _T("Shift overrides application's use of mouse"), _T('p'),
		  HELPCTX(selection_shiftdrag),
		  dlg_stdcheckbox_handler, I(offsetof(Config,mouse_override)));
    ctrl_radiobuttons(s,
		      _T("Default selection mode (Alt+drag does the other one):"),
		      NO_SHORTCUT, 2,
		      HELPCTX(selection_rect),
		      dlg_stdradiobutton_handler,
		      I(offsetof(Config, rect_select)),
		      _T("Normal"), _T('n'), I(0),
		      _T("Rectangular block"), _T('r'), I(1), NULL);

    s = ctrl_getset(b, _T("Window/Selection"), _T("charclass"),
		    _T("Control the select-one-word-at-a-time mode"));
    ccd = (struct charclass_data *)
	ctrl_alloc(b, sizeof(struct charclass_data));
    ccd->listbox = ctrl_listbox(s, _T("Character classes:"), _T('e'),
				HELPCTX(selection_charclasses),
				charclass_handler, P(ccd));
    ccd->listbox->listbox.multisel = 1;
    ccd->listbox->listbox.ncols = 4;
    ccd->listbox->listbox.percentages = snewn(4, int);
    ccd->listbox->listbox.percentages[0] = 15;
    ccd->listbox->listbox.percentages[1] = 25;
    ccd->listbox->listbox.percentages[2] = 20;
    ccd->listbox->listbox.percentages[3] = 40;
    ctrl_columns(s, 2, 67, 33);
    ccd->editbox = ctrl_editbox(s, _T("Set to class"), _T('t'), 50,
				HELPCTX(selection_charclasses),
				charclass_handler, P(ccd), P(NULL));
    ccd->editbox->generic.column = 0;
    ccd->button = ctrl_pushbutton(s, _T("Set"), _T('s'),
				  HELPCTX(selection_charclasses),
				  charclass_handler, P(ccd));
    ccd->button->generic.column = 1;
    ctrl_columns(s, 1, 100);

    /*
     * The Window/Colours panel.
     */
    ctrl_settitle(b, _T("Window/Colours"), _T("Options controlling use of colours"));

    s = ctrl_getset(b, _T("Window/Colours"), _T("general"),
		    _T("General options for colour usage"));
    ctrl_checkbox(s, _T("Bolded text is a different colour"), _T('b'),
		  HELPCTX(colours_bold),
		  dlg_stdcheckbox_handler, I(offsetof(Config,bold_colour)));

    str = _duptprintf(_T("Adjust the precise colours %s displays"), appname);
    s = ctrl_getset(b, _T("Window/Colours"), _T("adjust"), str);
    sfree(str);
    ctrl_text(s, _T("Select a colour from the list, and then click the")
	      _T(" Modify button to change its appearance."),
	      HELPCTX(colours_config));
    ctrl_columns(s, 2, 67, 33);
    cd = (struct colour_data *)ctrl_alloc(b, sizeof(struct colour_data));
    cd->listbox = ctrl_listbox(s, _T("Select a colour to adjust:"), _T('u'),
			       HELPCTX(colours_config), colour_handler, P(cd));
    cd->listbox->generic.column = 0;
    cd->listbox->listbox.height = 7;
    c = ctrl_text(s, _T("RGB value:"), HELPCTX(colours_config));
    c->generic.column = 1;
    cd->redit = ctrl_editbox(s, _T("Red"), _T('r'), 50, HELPCTX(colours_config),
			     colour_handler, P(cd), P(NULL));
    cd->redit->generic.column = 1;
    cd->gedit = ctrl_editbox(s, _T("Green"), _T('n'), 50, HELPCTX(colours_config),
			     colour_handler, P(cd), P(NULL));
    cd->gedit->generic.column = 1;
    cd->bedit = ctrl_editbox(s, _T("Blue"), _T('e'), 50, HELPCTX(colours_config),
			     colour_handler, P(cd), P(NULL));
    cd->bedit->generic.column = 1;
    cd->button = ctrl_pushbutton(s, _T("Modify"), _T('m'), HELPCTX(colours_config),
				 colour_handler, P(cd));
    cd->button->generic.column = 1;
    ctrl_columns(s, 1, 100);

    /*
     * The Connection panel. This doesn't show up if we're in a
     * non-network utility such as pterm. We tell this by being
     * passed a protocol < 0.
     */
    if (protocol >= 0) {
	ctrl_settitle(b, _T("Connection"), _T("Options controlling the connection"));

	if (!midsession) {
	    s = ctrl_getset(b, _T("Connection"), _T("data"),
			    _T("Data to send to the server"));
	    ctrl_editbox(s, _T("Terminal-type string"), _T('t'), 50,
			 HELPCTX(connection_termtype),
			 dlg_stdeditbox_handler, I(offsetof(Config,termtype)),
			 I(sizeof(((Config *)0)->termtype)));
	    ctrl_editbox(s, _T("Auto-login username"), _T('u'), 50,
			 HELPCTX(connection_username),
			 dlg_stdeditbox_handler, I(offsetof(Config,username)),
			 I(sizeof(((Config *)0)->username)));
	}

	s = ctrl_getset(b, _T("Connection"), _T("keepalive"),
			_T("Sending of null packets to keep session active"));
	ctrl_editbox(s, _T("Seconds between keepalives (0 to turn off)"), _T('k'), 20,
		     HELPCTX(connection_keepalive),
		     dlg_stdeditbox_handler, I(offsetof(Config,ping_interval)),
		     I(-1));

	if (!midsession) {
	    s = ctrl_getset(b, _T("Connection"), _T("tcp"),
			    _T("Low-level TCP connection options"));
	    ctrl_checkbox(s, _T("Disable Nagle's algorithm (TCP_NODELAY option)"),
			  _T('n'), HELPCTX(connection_nodelay),
			  dlg_stdcheckbox_handler,
			  I(offsetof(Config,tcp_nodelay)));
	}

    }

    if (!midsession) {
	/*
	 * The Connection/Proxy panel.
	 */
	ctrl_settitle(b, _T("Connection/Proxy"),
		      _T("Options controlling proxy usage"));

	s = ctrl_getset(b, _T("Connection/Proxy"), _T("basics"), NULL);
	ctrl_radiobuttons(s, _T("Proxy type:"), _T('t'), 3,
			  HELPCTX(proxy_type),
			  dlg_stdradiobutton_handler,
			  I(offsetof(Config, proxy_type)),
			  _T("None"), I(PROXY_NONE),
			  _T("SOCKS 4"), I(PROXY_SOCKS4),
			  _T("SOCKS 5"), I(PROXY_SOCKS5),
			  _T("HTTP"), I(PROXY_HTTP),
			  _T("Telnet"), I(PROXY_TELNET),
			  NULL);
	ctrl_columns(s, 2, 80, 20);
	c = ctrl_editbox(s, _T("Proxy hostname"), _T('y'), 100,
			 HELPCTX(proxy_main),
			 dlg_stdeditbox_handler,
			 I(offsetof(Config,proxy_host)),
			 I(sizeof(((Config *)0)->proxy_host)));
	c->generic.column = 0;
	c = ctrl_editbox(s, _T("Port"), _T('p'), 100,
			 HELPCTX(proxy_main),
			 dlg_stdeditbox_handler,
			 I(offsetof(Config,proxy_port)),
			 I(-1));
	c->generic.column = 1;
	ctrl_columns(s, 1, 100);
	ctrl_editbox(s, _T("Exclude Hosts/IPs"), _T('e'), 100,
		     HELPCTX(proxy_exclude),
		     dlg_stdeditbox_handler,
		     I(offsetof(Config,proxy_exclude_list)),
		     I(sizeof(((Config *)0)->proxy_exclude_list)));
	ctrl_checkbox(s, _T("Consider proxying local host connections"), _T('x'),
		      HELPCTX(proxy_exclude),
		      dlg_stdcheckbox_handler,
		      I(offsetof(Config,even_proxy_localhost)));
	ctrl_radiobuttons(s, _T("Do DNS name lookup at proxy end:"), _T('d'), 3,
			  HELPCTX(proxy_dns),
			  dlg_stdradiobutton_handler,
			  I(offsetof(Config, proxy_dns)),
			  _T("No"), I(FORCE_OFF),
			  _T("Auto"), I(AUTO),
			 _T( "Yes"), I(FORCE_ON), NULL);
	ctrl_editbox(s, _T("Username"), _T('u'), 60,
		     HELPCTX(proxy_auth),
		     dlg_stdeditbox_handler,
		     I(offsetof(Config,proxy_username)),
		     I(sizeof(((Config *)0)->proxy_username)));
	c = ctrl_editbox(s, _T("Password"), _T('w'), 60,
			 HELPCTX(proxy_auth),
			 dlg_stdeditbox_handler,
			 I(offsetof(Config,proxy_password)),
			 I(sizeof(((Config *)0)->proxy_password)));
	c->editbox.password = 1;
	ctrl_editbox(s, _T("Telnet command"), _T('m'), 100,
		     HELPCTX(proxy_command),
		     dlg_stdeditbox_handler,
		     I(offsetof(Config,proxy_telnet_command)),
		     I(sizeof(((Config *)0)->proxy_telnet_command)));
    }

    /*
     * The Telnet panel exists in the base config box, and in a
     * mid-session reconfig box _if_ we're using Telnet.
     */
    if (!midsession || protocol == PROT_TELNET) {
	/*
	 * The Connection/Telnet panel.
	 */
	ctrl_settitle(b, _T("Connection/Telnet"),
		      _T("Options controlling Telnet connections"));

	if (!midsession) {
	    s = ctrl_getset(b, _T("Connection/Telnet"), _T("data"),
			    _T("Data to send to the server"));
	    ctrl_editbox(s, _T("Terminal-speed string"), _T('s'), 50,
			 HELPCTX(telnet_termspeed),
			 dlg_stdeditbox_handler, I(offsetof(Config,termspeed)),
			 I(sizeof(((Config *)0)->termspeed)));
	    ctrl_text(s, _T("Environment variables:"), HELPCTX(telnet_environ));
	    ctrl_columns(s, 2, 80, 20);
	    ed = (struct environ_data *)
		ctrl_alloc(b, sizeof(struct environ_data));
	    ed->varbox = ctrl_editbox(s, _T("Variable"), _T('v'), 60,
				      HELPCTX(telnet_environ),
				      environ_handler, P(ed), P(NULL));
	    ed->varbox->generic.column = 0;
	    ed->valbox = ctrl_editbox(s, _T("Value"), _T('l'), 60,
				      HELPCTX(telnet_environ),
				      environ_handler, P(ed), P(NULL));
	    ed->valbox->generic.column = 0;
	    ed->addbutton = ctrl_pushbutton(s, _T("Add"), _T('d'),
					    HELPCTX(telnet_environ),
					    environ_handler, P(ed));
	    ed->addbutton->generic.column = 1;
	    ed->rembutton = ctrl_pushbutton(s, _T("Remove"), _T('r'),
					    HELPCTX(telnet_environ),
					    environ_handler, P(ed));
	    ed->rembutton->generic.column = 1;
	    ctrl_columns(s, 1, 100);
	    ed->listbox = ctrl_listbox(s, NULL, NO_SHORTCUT,
				       HELPCTX(telnet_environ),
				       environ_handler, P(ed));
	    ed->listbox->listbox.height = 3;
	    ed->listbox->listbox.ncols = 2;
	    ed->listbox->listbox.percentages = snewn(2, int);
	    ed->listbox->listbox.percentages[0] = 30;
	    ed->listbox->listbox.percentages[1] = 70;
	}

	s = ctrl_getset(b, _T("Connection/Telnet"), _T("protocol"),
			_T("Telnet protocol adjustments"));

	if (!midsession) {
	    ctrl_radiobuttons(s, _T("Handling of OLD_ENVIRON ambiguity:"),
			      NO_SHORTCUT, 2,
			      HELPCTX(telnet_oldenviron),
			      dlg_stdradiobutton_handler,
			      I(offsetof(Config, rfc_environ)),
			      _T("BSD (commonplace)"), _T('b'), I(0),
			      _T("RFC 1408 (unusual)"), _T('f'), I(1), NULL);
	    ctrl_radiobuttons(s, _T("Telnet negotiation mode:"), _T('t'), 2,
			      HELPCTX(telnet_passive),
			      dlg_stdradiobutton_handler,
			      I(offsetof(Config, passive_telnet)),
			      _T("Passive"), I(1), _T("Active"), I(0), NULL);
	}
	ctrl_checkbox(s, _T("Keyboard sends telnet Backspace and Interrupt"), _T('k'),
		      HELPCTX(telnet_specialkeys),
		      dlg_stdcheckbox_handler,
		      I(offsetof(Config,telnet_keyboard)));
	ctrl_checkbox(s, _T("Return key sends telnet New Line instead of ^M"),
		      NO_SHORTCUT, HELPCTX(telnet_newline),
		      dlg_stdcheckbox_handler,
		      I(offsetof(Config,telnet_newline)));
    }

    if (!midsession) {

	/*
	 * The Connection/Rlogin panel.
	 */
	ctrl_settitle(b, _T("Connection/Rlogin"),
		      _T("Options controlling Rlogin connections"));

	s = ctrl_getset(b, _T("Connection/Rlogin"), _T("data"),
			_T("Data to send to the server"));
	ctrl_editbox(s, _T("Terminal-speed string"), _T('s'), 50,
		     HELPCTX(rlogin_termspeed),
		     dlg_stdeditbox_handler, I(offsetof(Config,termspeed)),
		     I(sizeof(((Config *)0)->termspeed)));
	ctrl_editbox(s, _T("Local username:"), _T('l'), 50,
		     HELPCTX(rlogin_localuser),
		     dlg_stdeditbox_handler, I(offsetof(Config,localusername)),
		     I(sizeof(((Config *)0)->localusername)));

    }

    /*
     * All the SSH stuff is omitted in PuTTYtel.
     */

    if (!midsession && backends[3].name != NULL) {

	/*
	 * The Connection/SSH panel.
	 */
	ctrl_settitle(b, _T("Connection/SSH"),
		      _T("Options controlling SSH connections"));

	s = ctrl_getset(b, _T("Connection/SSH"), _T("data"),
			_T("Data to send to the server"));
	ctrl_editbox(s, _T("Remote command:"), _T('r'), 100,
		     HELPCTX(ssh_command),
		     dlg_stdeditbox_handler, I(offsetof(Config,remote_cmd)),
		     I(sizeof(((Config *)0)->remote_cmd)));

	s = ctrl_getset(b, _T("Connection/SSH"), _T("protocol"), _T("Protocol options"));
	ctrl_checkbox(s, _T("Don't allocate a pseudo-terminal"), _T('p'),
		      HELPCTX(ssh_nopty),
		      dlg_stdcheckbox_handler,
		      I(offsetof(Config,nopty)));
	ctrl_checkbox(s, _T("Enable compression"), _T('e'),
		      HELPCTX(ssh_compress),
		      dlg_stdcheckbox_handler,
		      I(offsetof(Config,compression)));
	ctrl_radiobuttons(s, _T("Preferred SSH protocol version:"), NO_SHORTCUT, 4,
			  HELPCTX(ssh_protocol),
			  dlg_stdradiobutton_handler,
			  I(offsetof(Config, sshprot)),
			  _T("1 only"), _T('l'), I(0),
			  _T("1"), _T('1'), I(1),
			  _T("2"), _T('2'), I(2),
			  _T("2 only"), _T('n'), I(3), NULL);

	s = ctrl_getset(b, _T("Connection/SSH"), _T("encryption"), _T("Encryption options"));
	c = ctrl_draglist(s, _T("Encryption cipher selection policy:"), _T('s'),
			  HELPCTX(ssh_ciphers),
			  cipherlist_handler, P(NULL));
	c->listbox.height = 6;
	
	ctrl_checkbox(s, _T("Enable legacy use of single-DES in SSH 2"), _T('i'),
		      HELPCTX(ssh_ciphers),
		      dlg_stdcheckbox_handler,
		      I(offsetof(Config,ssh2_des_cbc)));

	/*
	 * The Connection/SSH/Auth panel.
	 */
	ctrl_settitle(b, _T("Connection/SSH/Auth"),
		      _T("Options controlling SSH authentication"));

	s = ctrl_getset(b, _T("Connection/SSH/Auth"), _T("methods"),
			_T("Authentication methods"));
	ctrl_checkbox(s, _T("Attempt TIS or CryptoCard auth (SSH1)"), _T('m'),
		      HELPCTX(ssh_auth_tis),
		      dlg_stdcheckbox_handler,
		      I(offsetof(Config,try_tis_auth)));
	ctrl_checkbox(s, _T("Attempt \"keyboard-interactive\" auth (SSH2)"),
		      _T('i'), HELPCTX(ssh_auth_ki),
		      dlg_stdcheckbox_handler,
		      I(offsetof(Config,try_ki_auth)));

	s = ctrl_getset(b, _T("Connection/SSH/Auth"), _T("params"),
			_T("Authentication parameters"));
	ctrl_checkbox(s, _T("Allow agent forwarding"), _T('f'),
		      HELPCTX(ssh_auth_agentfwd),
		      dlg_stdcheckbox_handler, I(offsetof(Config,agentfwd)));
	ctrl_checkbox(s, _T("Allow attempted changes of username in SSH2"), _T('u'),
		      HELPCTX(ssh_auth_changeuser),
		      dlg_stdcheckbox_handler,
		      I(offsetof(Config,change_username)));
	ctrl_filesel(s, _T("Private key file for authentication:"), _T('k'),
		     FILTER_KEY_FILES, FALSE, _T("Select private key file"),
		     HELPCTX(ssh_auth_privkey),
		     dlg_stdfilesel_handler, I(offsetof(Config, keyfile)));

	/*
	 * The Connection/SSH/Tunnels panel.
	 */
	ctrl_settitle(b, _T("Connection/SSH/Tunnels"),
		      _T("Options controlling SSH tunnelling"));

	s = ctrl_getset(b, _T("Connection/SSH/Tunnels"), _T("x11"), _T("X11 forwarding"));
	ctrl_checkbox(s, _T("Enable X11 forwarding"), _T('e'),
		      HELPCTX(ssh_tunnels_x11),
		      dlg_stdcheckbox_handler,I(offsetof(Config,x11_forward)));
	ctrl_editbox(s, _T("X display location"), _T('x'), 50,
		     HELPCTX(ssh_tunnels_x11),
		     dlg_stdeditbox_handler, I(offsetof(Config,x11_display)),
		     I(sizeof(((Config *)0)->x11_display)));
	ctrl_radiobuttons(s, _T("Remote X11 authentication protocol"), _T('u'), 2,
			  HELPCTX(ssh_tunnels_x11auth),
			  dlg_stdradiobutton_handler,
			  I(offsetof(Config, x11_auth)),
			  _T("MIT-Magic-Cookie-1"), I(X11_MIT),
			  _T("XDM-Authorization-1"), I(X11_XDM), NULL);

	s = ctrl_getset(b, _T("Connection/SSH/Tunnels"), _T("portfwd"),
			_T("Port forwarding"));
	ctrl_checkbox(s, _T("Local ports accept connections from other hosts"), _T('t'),
		      HELPCTX(ssh_tunnels_portfwd_localhost),
		      dlg_stdcheckbox_handler,
		      I(offsetof(Config,lport_acceptall)));
	ctrl_checkbox(s, _T("Remote ports do the same (SSH v2 only)"), _T('p'),
		      HELPCTX(ssh_tunnels_portfwd_localhost),
		      dlg_stdcheckbox_handler,
		      I(offsetof(Config,rport_acceptall)));

	ctrl_columns(s, 3, 55, 20, 25);
	c = ctrl_text(s, _T("Forwarded ports:"), HELPCTX(ssh_tunnels_portfwd));
	c->generic.column = COLUMN_FIELD(0,2);
	/* You want to select from the list, _then_ hit Remove. So tab order
	 * should be that way round. */
	pfd = (struct portfwd_data *)ctrl_alloc(b,sizeof(struct portfwd_data));
	pfd->rembutton = ctrl_pushbutton(s, _T("Remove"), _T('r'),
					 HELPCTX(ssh_tunnels_portfwd),
					 portfwd_handler, P(pfd));
	pfd->rembutton->generic.column = 2;
	pfd->rembutton->generic.tabdelay = 1;
	pfd->listbox = ctrl_listbox(s, NULL, NO_SHORTCUT,
				    HELPCTX(ssh_tunnels_portfwd),
				    portfwd_handler, P(pfd));
	pfd->listbox->listbox.height = 3;
	pfd->listbox->listbox.ncols = 2;
	pfd->listbox->listbox.percentages = snewn(2, int);
	pfd->listbox->listbox.percentages[0] = 20;
	pfd->listbox->listbox.percentages[1] = 80;
	ctrl_tabdelay(s, pfd->rembutton);
	ctrl_text(s, _T("Add new forwarded port:"), HELPCTX(ssh_tunnels_portfwd));
	/* You want to enter source, destination and type, _then_ hit Add.
	 * Again, we adjust the tab order to reflect this. */
	pfd->addbutton = ctrl_pushbutton(s, _T("Add"), _T('d'),
					 HELPCTX(ssh_tunnels_portfwd),
					 portfwd_handler, P(pfd));
	pfd->addbutton->generic.column = 2;
	pfd->addbutton->generic.tabdelay = 1;
	pfd->sourcebox = ctrl_editbox(s, _T("Source port"), _T('s'), 40,
				      HELPCTX(ssh_tunnels_portfwd),
				      portfwd_handler, P(pfd), P(NULL));
	pfd->sourcebox->generic.column = 0;
	pfd->destbox = ctrl_editbox(s, _T("Destination"), _T('i'), 67,
				    HELPCTX(ssh_tunnels_portfwd),
				    portfwd_handler, P(pfd), P(NULL));
	pfd->direction = ctrl_radiobuttons(s, NULL, NO_SHORTCUT, 3,
					   HELPCTX(ssh_tunnels_portfwd),
					   portfwd_handler, P(pfd),
					   _T("Local"), _T('l'), P(NULL),
					   _T("Remote"), _T('m'), P(NULL),
					   _T("Dynamic"), _T('y'), P(NULL),
					   NULL);
	ctrl_tabdelay(s, pfd->addbutton);
	ctrl_columns(s, 1, 100);

	/*
	 * The Connection/SSH/Bugs panel.
	 */
	ctrl_settitle(b, _T("Connection/SSH/Bugs"),
		      _T("Workarounds for SSH server bugs"));

	s = ctrl_getset(b, _T("Connection/SSH/Bugs"), _T("main"),
			_T("Detection of known bugs in SSH servers"));
	ctrl_droplist(s, _T("Chokes on SSH1 ignore messages"), _T('i'), 20,
		      HELPCTX(ssh_bugs_ignore1),
		      sshbug_handler, I(offsetof(Config,sshbug_ignore1)));
	ctrl_droplist(s, _T("Refuses all SSH1 password camouflage"), _T('s'), 20,
		      HELPCTX(ssh_bugs_plainpw1),
		      sshbug_handler, I(offsetof(Config,sshbug_plainpw1)));
	ctrl_droplist(s, _T("Chokes on SSH1 RSA authentication"), _T('r'), 20,
		      HELPCTX(ssh_bugs_rsa1),
		      sshbug_handler, I(offsetof(Config,sshbug_rsa1)));
	ctrl_droplist(s, _T("Miscomputes SSH2 HMAC keys"), _T('m'), 20,
		      HELPCTX(ssh_bugs_hmac2),
		      sshbug_handler, I(offsetof(Config,sshbug_hmac2)));
	ctrl_droplist(s, _T("Miscomputes SSH2 encryption keys"), _T('e'), 20,
		      HELPCTX(ssh_bugs_derivekey2),
		      sshbug_handler, I(offsetof(Config,sshbug_derivekey2)));
	ctrl_droplist(s, _T("Requires padding on SSH2 RSA signatures"), _T('p'), 20,
		      HELPCTX(ssh_bugs_rsapad2),
		      sshbug_handler, I(offsetof(Config,sshbug_rsapad2)));
	ctrl_droplist(s, _T("Chokes on Diffie-Hellman group exchange"), _T('d'), 20,
		      HELPCTX(ssh_bugs_dhgex2),
		      sshbug_handler, I(offsetof(Config,sshbug_dhgex2)));
	ctrl_droplist(s, _T("Misuses the session ID in PK auth"), _T('n'), 20,
		      HELPCTX(ssh_bugs_pksessid2),
		      sshbug_handler, I(offsetof(Config,sshbug_pksessid2)));
    }
}
