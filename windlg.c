#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#if !(UNDER_CE > 0 && UNDER_CE < 400)
#include <assert.h>
#include <time.h>
#else
#include "wceassrt.h"
#include "wcetime.h"
#endif

#include <ctype.h>

#include "putty.h"
#include "ssh.h"

#ifdef _WIN32_WCE
#include "wce_res.h"
#else
#include "win_res.h"
#endif

#include "storage.h"
#include "dialog.h"

#include <commctrl.h>
#include <commdlg.h>

#ifdef _WIN32_WCE
#include <shellapi.h>
#include <aygshell.h>
#endif

#ifdef MSVC4
#define TVINSERTSTRUCT  TV_INSERTSTRUCT
#define TVITEM          TV_ITEM
#define ICON_BIG        1
#endif

/*
 * These are the various bits of data required to handle the
 * portable-dialog stuff in the config box. Having them at file
 * scope in here isn't too bad a place to put them; if we were ever
 * to need more than one config box per process we could always
 * shift them to a per-config-box structure stored in GWL_USERDATA.
 */
static struct controlbox *ctrlbox;
/*
 * ctrls_base holds the OK and Cancel buttons: the controls which
 * are present in all dialog panels. ctrls_panel holds the ones
 * which change from panel to panel.
 */
static struct winctrls ctrls_base, ctrls_panel;
static struct dlgparam dp;

static char **events = NULL;
static int nevents = 0, negsize = 0;

static int requested_help;

extern Config cfg;		       /* defined in window.c */

struct sesslist sesslist;	       /* exported to window.c */

#define PRINTER_DISABLED_STRING "None (printing disabled)"

void force_normal(HWND hwnd)
{
#ifndef _WIN32_WCE
    static int recurse = 0;

    WINDOWPLACEMENT wp;

    if (recurse)
	return;
    recurse = 1;

    wp.length = sizeof(wp);
    if (GetWindowPlacement(hwnd, &wp) && wp.showCmd == SW_SHOWMAXIMIZED) {
	wp.showCmd = SW_SHOWNORMAL;
	SetWindowPlacement(hwnd, &wp);
    }
    recurse = 0;
#endif
}

static int CALLBACK LogProc(HWND hwnd, UINT msg,
			    WPARAM wParam, LPARAM lParam)
{
    int i;

    switch (msg) {
      case WM_INITDIALOG:
	{
	    TCHAR *str = _duptprintf(_T("%s Event Log"), appname);
	    SetWindowText(hwnd, str);
	    sfree(str);
	}
	{
	    static int tabs[4] = { 78, 108 };
	    SendDlgItemMessage(hwnd, IDN_LIST, LB_SETTABSTOPS, 2,
			       (LPARAM) tabs);
	}
	for (i = 0; i < nevents; i++)
	    SendDlgItemMessage(hwnd, IDN_LIST, LB_ADDSTRING,
			       0, (LPARAM) events[i]);
	return 1;
      case WM_COMMAND:
	switch (LOWORD(wParam)) {
	  case IDOK:
	  case IDCANCEL:
	    logbox = NULL;
	    SetActiveWindow(GetParent(hwnd));
	    DestroyWindow(hwnd);
	    return 0;
	  case IDN_COPY:
	    if (HIWORD(wParam) == BN_CLICKED ||
		HIWORD(wParam) == BN_DOUBLECLICKED) {
		int selcount;
		int *selitems;
		selcount = SendDlgItemMessage(hwnd, IDN_LIST,
					      LB_GETSELCOUNT, 0, 0);
		if (selcount == 0) {   /* don't even try to copy zero items */
		    MessageBeep(0);
		    break;
		}

		selitems = snewn(selcount, int);
		if (selitems) {
		    int count = SendDlgItemMessage(hwnd, IDN_LIST,
						   LB_GETSELITEMS,
						   selcount,
						   (LPARAM) selitems);
		    int i;
		    int size;
		    char *clipdata;
		    static unsigned char sel_nl[] = SEL_NL;

		    if (count == 0) {  /* can't copy zero stuff */
			MessageBeep(0);
			break;
		    }

		    size = 0;
		    for (i = 0; i < count; i++)
			size +=
			    strlen(events[selitems[i]]) + sizeof(sel_nl);

		    clipdata = snewn(size, char);
		    if (clipdata) {
			char *p = clipdata;
			for (i = 0; i < count; i++) {
			    char *q = events[selitems[i]];
			    int qlen = strlen(q);
			    memcpy(p, q, qlen);
			    p += qlen;
			    memcpy(p, sel_nl, sizeof(sel_nl));
			    p += sizeof(sel_nl);
			}
			write_aclip(NULL, clipdata, size, TRUE);
			sfree(clipdata);
		    }
		    sfree(selitems);

		    for (i = 0; i < nevents; i++)
			SendDlgItemMessage(hwnd, IDN_LIST, LB_SETSEL,
					   FALSE, i);
		}
	    }
	    return 0;
	}
	return 0;
      case WM_CLOSE:
	logbox = NULL;
	SetActiveWindow(GetParent(hwnd));
	DestroyWindow(hwnd);
	return 0;
    }
    return 0;
}

static int CALLBACK LicenceProc(HWND hwnd, UINT msg,
				WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
      case WM_INITDIALOG:
	{
	    TCHAR *str = _duptprintf(_T("%s Licence"), appname);
	    SetWindowText(hwnd, str);
	    sfree(str);
#ifdef _WIN32_WCE
	{
	    SHINITDLGINFO shidi;
	    
	    // Create a Done button and size it.  
	    shidi.dwMask = SHIDIM_FLAGS;
	    shidi.dwFlags = SHIDIF_DONEBUTTON | SHIDIF_SIPDOWN | SHIDIF_SIZEDLGFULLSCREEN;
	    shidi.hDlg = hwnd;
	    SHInitDialog(&shidi);
	}
	SetDlgItemText(hwnd, IDC_E_LICENCE, 
	    _T("PuTTY is copyright 1997-2004 Simon Tatham.\r\n\r\n")
	    _T("Portions \r\ncopyright Robert de Bath, Joris van Rantwijk, Delian Delchev, Andreas Schultz, Jeroen Massar, Wez Furlong, Nicolas Barry, CORE SDI S.A. and Aleš Berka\r\n\r\n")
	    _T("Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the \"Software\"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:\r\n\r\n")
	    _T("The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.\r\n\r\n")
	    _T("THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL SIMON TATHAM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.")
	);
	// Aleq: not working, why?
	// SendMessage(GetDlgItem(hwnd, IDC_E_LICENCE), EM_SETSEL, 0, 15);
	return TRUE; 
#endif
	}
	return 1;
      case WM_COMMAND:
	switch (LOWORD(wParam)) {
	  case IDOK:
	    EndDialog(hwnd, 1);
	    return 0;
	}
	return 0;
      case WM_CLOSE:
	EndDialog(hwnd, 1);
	return 0;
    }
    return 0;
}

static int CALLBACK AboutProc(HWND hwnd, UINT msg,
			      WPARAM wParam, LPARAM lParam)
{
    TCHAR *str;

    switch (msg) {
      case WM_INITDIALOG:
	str = _duptprintf(_T("About %s"), appname);
	SetWindowText(hwnd, str);
	sfree(str);
	SetDlgItemText(hwnd, IDA_TEXT1, appname);
	SetDlgItemText(hwnd, IDA_VERSION, ver);
#ifdef _WIN32_WCE
	{
	    SHINITDLGINFO shidi;
	    
	    // Create a Done button and size it.  
	    shidi.dwMask = SHIDIM_FLAGS;
	    shidi.dwFlags = SHIDIF_DONEBUTTON | SHIDIF_SIPDOWN | SHIDIF_SIZEDLGFULLSCREEN;
	    shidi.hDlg = hwnd;
	    SHInitDialog(&shidi);
	}
	return TRUE; 
#endif
	return 1;
      case WM_COMMAND:
	switch (LOWORD(wParam)) {
	  case IDOK:
	  case IDCANCEL:
	    EndDialog(hwnd, TRUE);
	    return 0;
	  case IDA_LICENCE:
#ifndef _WIN32_WCE
	    EnableWindow(hwnd, 0);
#endif
	    DialogBox(hinst, MAKEINTRESOURCE(IDD_LICENCEBOX),
		      NULL, LicenceProc);
#ifndef _WIN32_WCE
	    EnableWindow(hwnd, 1);
	    SetActiveWindow(hwnd);
#endif
	    return 0;

	  case IDA_WEB:
	    /* Load web browser */
#ifdef _WIN32_WCE
	  case IDC_POCKETPUTTY_WEB:
	      {
		  LPSHELLEXECUTEINFO lpExecInfo = snew(struct _SHELLEXECUTEINFO);
		  lpExecInfo->cbSize = sizeof(struct _SHELLEXECUTEINFO);
		  lpExecInfo->fMask = SEE_MASK_NOCLOSEPROCESS;
		  lpExecInfo->hwnd = hwnd;
		  lpExecInfo->lpVerb = _T("Open");
		  if (LOWORD(wParam) == IDA_WEB)
		    lpExecInfo->lpFile = _T("http://www.chiark.greenend.org.uk/~sgtatham/putty/");
		  else
		    lpExecInfo->lpFile = _T("http://pocketputty.aleq.cz/");
		  lpExecInfo->lpParameters = _T("");
		  lpExecInfo->nShow = SW_SHOWNORMAL;
		  ShellExecuteEx(lpExecInfo);
		  sfree(lpExecInfo);
	      }
#else
	    ShellExecute(hwnd, "open",
			 "http://www.chiark.greenend.org.uk/~sgtatham/putty/",
			 0, 0, SW_SHOWDEFAULT);
#endif
	    return 0;
	}
	return 0;
      case WM_CLOSE:
	EndDialog(hwnd, TRUE);
	return 0;
    }
    return 0;
}

/*
 * Null dialog procedure.
 */
static int CALLBACK NullDlgProc(HWND hwnd, UINT msg,
				WPARAM wParam, LPARAM lParam)
{
    return 0;
}

enum {
    IDCX_ABOUT = IDC_ABOUT,
    IDCX_TVSTATIC,
    IDCX_TREEVIEW,
    IDCX_STDBASE,
    IDCX_PANELBASE = IDCX_STDBASE + 32
};

struct treeview_faff {
    HWND treeview;
    HTREEITEM lastat[4];
};

static HTREEITEM treeview_insert(struct treeview_faff *faff,
				 int level, TCHAR *text, TCHAR *path)
{
    TVINSERTSTRUCT ins;
    int i;
    HTREEITEM newitem;
    ins.hParent = (level > 0 ? faff->lastat[level - 1] : TVI_ROOT);
    ins.hInsertAfter = faff->lastat[level];
#if _WIN32_IE >= 0x0400 && defined NONAMELESSUNION
#define INSITEM DUMMYUNIONNAME.item
#else
#define INSITEM item
#endif
    ins.INSITEM.mask = TVIF_TEXT | TVIF_PARAM;
    ins.INSITEM.pszText = text;
    ins.INSITEM.cchTextMax = _tcslen(text) + 1;
    ins.INSITEM.lParam = (LPARAM)path;
    newitem = TreeView_InsertItem(faff->treeview, &ins);
    if (level > 0)
	TreeView_Expand(faff->treeview, faff->lastat[level - 1],
			TVE_EXPAND);
    faff->lastat[level] = newitem;
    for (i = level + 1; i < 4; i++)
	faff->lastat[i] = NULL;
    return newitem;
}

/*
 * Create the panelfuls of controls in the configuration box.
 */
static void create_controls(HWND hwnd, TCHAR *path)
{
    struct ctlpos cp;
    int index;
    int base_id;
    struct winctrls *wc;

    if (!path[0]) {
	/*
	 * Here we must create the basic standard controls.
	 */
	ctlposinit(&cp, hwnd, 3, 3, 235);
	wc = &ctrls_base;
	base_id = IDCX_STDBASE;
    } else {
	/*
	 * Otherwise, we're creating the controls for a particular
	 * panel.
	 */
	ctlposinit(&cp, hwnd, 80, 3, 13);
	wc = &ctrls_panel;
	base_id = IDCX_PANELBASE;
    }

    for (index=-1; (index = ctrl_find_path(ctrlbox, path, index)) >= 0 ;) {
	struct controlset *s = ctrlbox->ctrlsets[index];
	winctrl_layout(&dp, wc, &cp, s, &base_id);
    }
}

/*
 * This function is the configuration box.
 */
static int CALLBACK GenericMainDlgProc(HWND hwnd, UINT msg,
				       WPARAM wParam, LPARAM lParam)
{
    HWND hw, treeview;
    struct treeview_faff tvfaff;
    int ret;

    switch (msg) {
      case WM_INITDIALOG:
	dp.hwnd = hwnd;
	create_controls(hwnd, _T(""));     /* Open and Cancel buttons etc */
	SetWindowText(hwnd, dp.wintitle);
	SetWindowLong(hwnd, GWL_USERDATA, 0);
#ifndef _WIN32_WCE
        if (help_path)
            SetWindowLong(hwnd, GWL_EXSTYLE,
                          GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_CONTEXTHELP);
        else 
#endif
	{
            HWND item = GetDlgItem(hwnd, IDC_HELPBTN);
            if (item)
                DestroyWindow(item);
        }
        requested_help = FALSE;
#ifndef _WIN32_WCE
	SendMessage(hwnd, WM_SETICON, (WPARAM) ICON_BIG,
		    (LPARAM) LoadIcon(hinst, MAKEINTRESOURCE(IDI_CFGICON)));
#endif
	/*
	 * Centre the window.
	 */
	{			       /* centre the window */
	    RECT rs, rd;

	    hw = GetDesktopWindow();
	    if (GetWindowRect(hw, &rs) && GetWindowRect(hwnd, &rd))
		MoveWindow(hwnd,
			   (rs.right + rs.left + rd.left - rd.right) / 2,
			   (rs.bottom + rs.top + rd.top - rd.bottom) / 2,
			   rd.right - rd.left, rd.bottom - rd.top, TRUE);
	}

	/*
	 * Create the tree view.
	 */
	{
	    RECT r;
	    WPARAM font;
	    HWND tvstatic;

	    r.left = 3;
	    r.right = r.left + 75;
	    r.top = 3;
	    r.bottom = r.top + 10;
	    MapDialogRect(hwnd, &r);
	    tvstatic = CreateWindowEx(0, _T("STATIC"), _T("Cate&gory:"),
				      WS_CHILD | WS_VISIBLE,
				      r.left, r.top,
				      r.right - r.left, r.bottom - r.top,
				      hwnd, (HMENU) IDCX_TVSTATIC, hinst,
				      NULL);
	    font = SendMessage(hwnd, WM_GETFONT, 0, 0);
	    SendMessage(tvstatic, WM_SETFONT, font, MAKELPARAM(TRUE, 0));

	    r.left = 3;
	    r.right = r.left + 75;
	    r.top = 13;
	    r.bottom = r.top + 219;
	    MapDialogRect(hwnd, &r);
	    treeview = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, _T(""),
				      WS_CHILD | WS_VISIBLE |
				      WS_TABSTOP | TVS_HASLINES |
				      TVS_DISABLEDRAGDROP | TVS_HASBUTTONS
				      | TVS_LINESATROOT |
				      TVS_SHOWSELALWAYS, r.left, r.top,
				      r.right - r.left, r.bottom - r.top,
				      hwnd, (HMENU) IDCX_TREEVIEW, hinst,
				      NULL);
	    font = SendMessage(hwnd, WM_GETFONT, 0, 0);
	    SendMessage(treeview, WM_SETFONT, font, MAKELPARAM(TRUE, 0));
	    tvfaff.treeview = treeview;
	    memset(tvfaff.lastat, 0, sizeof(tvfaff.lastat));
	}

	/*
	 * Set up the tree view contents.
	 */
	{
	    HTREEITEM hfirst = NULL;
	    int i;
	    TCHAR *path = NULL;

	    for (i = 0; i < ctrlbox->nctrlsets; i++) {
		struct controlset *s = ctrlbox->ctrlsets[i];
		HTREEITEM item;
		int j;
		TCHAR *c;

		if (!s->pathname[0])
		    continue;
		j = path ? ctrl_path_compare(s->pathname, path) : 0;
		if (j == INT_MAX)
		    continue;	       /* same path, nothing to add to tree */

		/*
		 * We expect never to find an implicit path
		 * component. For example, we expect never to see
		 * A/B/C followed by A/D/E, because that would
		 * _implicitly_ create A/D. All our path prefixes
		 * are expected to contain actual controls and be
		 * selectable in the treeview; so we would expect
		 * to see A/D _explicitly_ before encountering
		 * A/D/E.
		 */
		assert(j == ctrl_path_elements(s->pathname) - 1);

		c = _tcsrchr(s->pathname, _T('/'));
		if (!c)
			c = s->pathname;
		else
			c++;

		item = treeview_insert(&tvfaff, j, c, s->pathname);
		if (!hfirst)
		    hfirst = item;

		path = s->pathname;
	    }

	    /*
	     * Put the treeview selection on to the Session panel.
	     * This should also cause creation of the relevant
	     * controls.
	     */
	    TreeView_SelectItem(treeview, hfirst);
	}

	/*
	 * Set focus into the first available control.
	 */
	{
	    int i;
	    struct winctrl *c;

	    for (i = 0; (c = winctrl_findbyindex(&ctrls_panel, i)) != NULL;
		 i++) {
		if (c->ctrl) {
		    dlg_set_focus(c->ctrl, &dp);
		    break;
		}
	    }
	}

	SetWindowLong(hwnd, GWL_USERDATA, 1);
	return 0;
      case WM_LBUTTONUP:
	/*
	 * Button release should trigger WM_OK if there was a
	 * previous double click on the session list.
	 */
	ReleaseCapture();
	if (dp.ended)
	    SaneEndDialog(hwnd, dp.endresult ? 1 : 0);
	break;
      case WM_NOTIFY:
	if (LOWORD(wParam) == IDCX_TREEVIEW &&
	    ((LPNMHDR) lParam)->code == TVN_SELCHANGED) {
	    HTREEITEM i =
		TreeView_GetSelection(((LPNMHDR) lParam)->hwndFrom);
	    TVITEM item;
	    TCHAR buffer[64];
 
 	    SendMessage (hwnd, WM_SETREDRAW, FALSE, 0);
 
	    item.hItem = i;
	    item.pszText = buffer;
	    item.cchTextMax = sizeof(buffer);
	    item.mask = TVIF_TEXT | TVIF_PARAM;
	    TreeView_GetItem(((LPNMHDR) lParam)->hwndFrom, &item);
	    {
		/* Destroy all controls in the currently visible panel. */
		int k;
		HWND item;
		struct winctrl *c;

		while ((c = winctrl_findbyindex(&ctrls_panel, 0)) != NULL) {
		    for (k = 0; k < c->num_ids; k++) {
			item = GetDlgItem(hwnd, c->base_id + k);
			if (item)
			    DestroyWindow(item);
		    }
		    winctrl_rem_shortcuts(&dp, c);
		    winctrl_remove(&ctrls_panel, c);
		    sfree(c->data);
		    sfree(c);
		}
	    }
	    create_controls(hwnd, (TCHAR *)item.lParam);

	    dlg_refresh(NULL, &dp);    /* set up control values */
 
	    SendMessage (hwnd, WM_SETREDRAW, TRUE, 0);
 	    InvalidateRect (hwnd, NULL, TRUE);

	    SetFocus(((LPNMHDR) lParam)->hwndFrom);	/* ensure focus stays */
	    return 0;
	}
	break;
      case WM_COMMAND:
      case WM_DRAWITEM:
      default:			       /* also handle drag list msg here */
	/*
	 * Only process WM_COMMAND once the dialog is fully formed.
	 */
	if (GetWindowLong(hwnd, GWL_USERDATA) == 1) {
	    ret = winctrl_handle_command(&dp, msg, wParam, lParam);
	    if (dp.ended && GetCapture() != hwnd)
		SaneEndDialog(hwnd, dp.endresult ? 1 : 0);
	} else
	    ret = 0;
	return ret;
      case WM_HELP:
#ifndef _WIN32_WCE
        if (help_path) {
	    if (winctrl_context_help(&dp, hwnd,
				     ((LPHELPINFO)lParam)->iCtrlId))
                requested_help = TRUE;
	    else
                MessageBeep(0);
        }
#endif
        break;
      case WM_CLOSE:
#ifndef _WIN32_WCE
        if (requested_help) {
            WinHelp(hwnd, help_path, HELP_QUIT, 0);
            requested_help = FALSE;
        }
#endif
	SaneEndDialog(hwnd, 0);
	return 0;

	/* Grrr Explorer will maximize Dialogs! */
      case WM_SIZE:
	if (wParam == SIZE_MAXIMIZED)
	    force_normal(hwnd);
	return 0;

    }
    return 0;
}

void modal_about_box(HWND hwnd)
{
    EnableWindow(hwnd, 0);
    DialogBox(hinst, MAKEINTRESOURCE(IDD_ABOUTBOX), hwnd, AboutProc);
    EnableWindow(hwnd, 1);
    SetActiveWindow(hwnd);
}

void show_help(HWND hwnd)
{
#ifndef _WIN32_WCE
    if (help_path) {
	WinHelp(hwnd, help_path,
		help_has_contents ? HELP_FINDER : HELP_CONTENTS,
		0);
	requested_help = TRUE;
    }
#endif
}

void defuse_showwindow(void)
{
    /*
     * Work around the fact that the app's first call to ShowWindow
     * will ignore the default in favour of the shell-provided
     * setting.
     */
    {
	HWND hwnd;
	hwnd = CreateDialog(hinst, MAKEINTRESOURCE(IDD_ABOUTBOX),
			    NULL, NullDlgProc);
	ShowWindow(hwnd, SW_HIDE);
	SetActiveWindow(hwnd);
	DestroyWindow(hwnd);
    }
}

static int CALLBACK OpenConnectionProc(HWND hwnd, UINT msg,
			      WPARAM wParam, LPARAM lParam)
{
//    TCHAR *str;

    switch (msg) {
      case WM_INITDIALOG:
/*	str = _duptprintf(_T("About %s"), appname);
	SetWindowText(hwnd, str);
	sfree(str);
	SetDlgItemText(hwnd, IDA_TEXT1, appname);
	SetDlgItemText(hwnd, IDA_VERSION, ver);
	*/
#ifdef _WIN32_WCE
	// Resize
	{
	    SHINITDLGINFO shidi;
	    
	    // Create a Done button and size it.  
	    shidi.dwMask = SHIDIM_FLAGS;
	    shidi.dwFlags = SHIDIF_DONEBUTTON | SHIDIF_SIPDOWN | SHIDIF_SIZEDLGFULLSCREEN;
	    shidi.hDlg = hwnd;
	    SHInitDialog(&shidi);
	}
//Aleq TODO: set default using cfg
	CheckDlgButton(hwnd, IDC_RB_SSH2, BST_CHECKED);
	CheckDlgButton(hwnd, IDC_CB_COMPRESSION, BST_CHECKED);
	CheckDlgButton(hwnd, IDC_CB_VERTSCROLL, BST_UNCHECKED);
	return TRUE;
#endif
	return 1;
      case WM_COMMAND:
	switch (LOWORD(wParam)) {
	case IDOK:
#ifdef _WIN32_WCE
	    // Save settings
	    {
		TCHAR whost[lenof(cfg.host)];
		GetDlgItemText(hwnd, IDC_CMB_HOST, whost, lenof(whost));
		wcstombs(cfg.host, whost, lenof(cfg.host));
		if (IsDlgButtonChecked(hwnd, IDC_RB_SSH1) == BST_CHECKED) {
		    cfg.protocol = PROT_SSH;
		    cfg.sshprot = 0;
		    cfg.port = 22;
		} else if (IsDlgButtonChecked(hwnd, IDC_RB_SSH2) == BST_CHECKED) {
		    cfg.protocol = PROT_SSH;
		    cfg.sshprot = 3;
		    cfg.port = 22;
		} else {
		    cfg.protocol = PROT_TELNET;
		    cfg.port = 23;
		}
		cfg.scrollbar_in_fullscreen = (IsDlgButtonChecked(hwnd, IDC_CB_VERTSCROLL) == BST_CHECKED);
		cfg.compression = (IsDlgButtonChecked(hwnd, IDC_CB_COMPRESSION) == BST_CHECKED);
	    }
#endif
	    EndDialog(hwnd, TRUE);
	    return 0;
	    break;

	  case IDCANCEL:
	    EndDialog(hwnd, FALSE);
	    return 0;
	    break;
	}
	return 0;
      case WM_CLOSE:
	EndDialog(hwnd, TRUE);
	return 0;
    }
    return 0;
}

int do_config(void)
{
    int ret;
#ifdef _WIN32_WCE
    ret = DialogBox(hinst, MAKEINTRESOURCE(IDD_OPENCONNECTION), NULL, OpenConnectionProc);
#else

    ctrlbox = ctrl_new_box();
    setup_config_box(ctrlbox, &sesslist, FALSE, 0);
#ifdef _WIN32_WCE
    win_setup_config_box(ctrlbox, &dp.hwnd, FALSE, FALSE);
#else
    win_setup_config_box(ctrlbox, &dp.hwnd, (help_path != NULL), FALSE);
#endif
    dp_init(&dp);
    winctrl_init(&ctrls_base);
    winctrl_init(&ctrls_panel);
    dp_add_tree(&dp, &ctrls_base);
    dp_add_tree(&dp, &ctrls_panel);
    dp.wintitle = _duptprintf(_T("%s Configuration"), appname);
    dp.errtitle = _duptprintf(_T("%s Error"), appname);
    dp.data = &cfg;
    dp.shortcuts['g'] = TRUE;	       /* the treeview: `Cate&gory' */

    get_sesslist(&sesslist, TRUE);
    ret =
	SaneDialogBox(hinst, MAKEINTRESOURCE(IDD_MAINBOX), NULL,
		  GenericMainDlgProc);
    get_sesslist(&sesslist, FALSE);

    ctrl_free_box(ctrlbox);
    winctrl_cleanup(&ctrls_panel);
    winctrl_cleanup(&ctrls_base);
    dp_cleanup(&dp);
#endif

    return ret;
}

int do_reconfig(HWND hwnd)
{
    Config backup_cfg;
    int ret;

    backup_cfg = cfg;		       /* structure copy */

    ctrlbox = ctrl_new_box();
    setup_config_box(ctrlbox, NULL, TRUE, cfg.protocol);
#ifdef _WIN32_WCE
    win_setup_config_box(ctrlbox, &dp.hwnd, FALSE, TRUE);
#else
    win_setup_config_box(ctrlbox, &dp.hwnd, (help_path != NULL), TRUE);
#endif
    dp_init(&dp);
    winctrl_init(&ctrls_base);
    winctrl_init(&ctrls_panel);
    dp_add_tree(&dp, &ctrls_base);
    dp_add_tree(&dp, &ctrls_panel);
    dp.wintitle = _duptprintf(_T("%s Reconfiguration"), appname);
    dp.errtitle = _duptprintf(_T("%s Error"), appname);
    dp.data = &cfg;
    dp.shortcuts['g'] = TRUE;	       /* the treeview: `Cate&gory' */

    ret = SaneDialogBox(hinst, MAKEINTRESOURCE(IDD_MAINBOX), NULL,
		  GenericMainDlgProc);

    ctrl_free_box(ctrlbox);
    winctrl_cleanup(&ctrls_base);
    winctrl_cleanup(&ctrls_panel);
    sfree(dp.errtitle);
    dp_cleanup(&dp);

    if (!ret)
	cfg = backup_cfg;	       /* structure copy */

    return ret;
}

void logevent(void *frontend, const char *string)
{
    char timebuf[40];
#ifndef _WIN32_WCE
    time_t t;
#endif

    log_eventlog(logctx, string);

    if (nevents >= negsize) {
	negsize += 64;
	events = sresize(events, negsize, char *);
    }

#ifdef _WIN32_WCE
    {
	struct tm tm;
	tm.tm_hour = 0;
	tm.tm_mday = 0;
	tm.tm_min = 0;
	tm.tm_mon = 0;
	tm.tm_sec = 0;
	tm.tm_wday = 0;
	tm.tm_yday = 0;
	tm.tm_year = 0;
	sprintf(timebuf, "%d-%d-%d %d:%d:%d\t", tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    }
#else
    time(&t);
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S\t",
	     localtime(&t));
#endif

    events[nevents] = snewn(strlen(timebuf) + strlen(string) + 1, char);
    strcpy(events[nevents], timebuf);
    strcat(events[nevents], string);
    if (logbox) {
	int count;
	SendDlgItemMessage(logbox, IDN_LIST, LB_ADDSTRING,
			   0, (LPARAM) events[nevents]);
	count = SendDlgItemMessage(logbox, IDN_LIST, LB_GETCOUNT, 0, 0);
	SendDlgItemMessage(logbox, IDN_LIST, LB_SETTOPINDEX, count - 1, 0);
    }
    nevents++;
}

void showeventlog(HWND hwnd)
{
    if (!logbox) {
	logbox = CreateDialog(hinst, MAKEINTRESOURCE(IDD_LOGBOX),
			      hwnd, LogProc);
	ShowWindow(logbox, SW_SHOWNORMAL);
    }
    SetActiveWindow(logbox);
}

void showabout(HWND hwnd)
{
    DialogBox(hinst, MAKEINTRESOURCE(IDD_ABOUTBOX), hwnd, AboutProc);
}

void verify_ssh_host_key(void *frontend, char *host, int port, char *keytype,
			 char *keystr, char *fingerprint)
{
    int ret;

#ifdef _WIN32_WCE
    // Shorter versions due to display :-)
    static const TCHAR absentmsg[] =
	_T("The server's host key is not cached in the registry. ")
	_T("The server's %s key fingerprint is:\n")
	_T("%s\n")
	_T("If you trust this host, hit Yes. ")
	_T("If you risk it this time, hit No. ")
	_T("If you do not trust this host, hit Cancel\n");

    static const TCHAR wrongmsg[] =
	_T("WARNING - POTENTIAL SECURITY BREACH!\n")
	_T("\n")
	_T("The server's host key does not match the one %s has ")
	_T("cached in the registry. ")
	_T("The new %s key fingerprint is:\n")
	_T("%s\n")
	_T("Hit Yes to update %s's cache and continue connecting, ")
	_T("hit no for connecting without updating the cache. ")
	_T("If you do not trust this host, hit Cancel\n");
#else
    static const TCHAR absentmsg[] =
	_T("The server's host key is not cached in the registry. You\n")
	_T("have no guarantee that the server is the computer you\n")
	_T("think it is.\n")
	_T("The server's %s key fingerprint is:\n")
	_T("%s\n")
	_T("If you trust this host, hit Yes to add the key to\n")
	_T("%s's cache and carry on connecting.\n")
	_T("If you want to carry on connecting just once, without\n")
	_T("adding the key to the cache, hit No.\n")
	_T("If you do not trust this host, hit Cancel to abandon the\n")
	_T("connection.\n");

    static const TCHAR wrongmsg[] =
	_T("WARNING - POTENTIAL SECURITY BREACH!\n")
	_T("\n")
	_T("The server's host key does not match the one %s has\n")
	_T("cached in the registry. This means that either the\n")
	_T("server administrator has changed the host key, or you\n")
	_T("have actually connected to another computer pretending\n")
	_T("to be the server.\n")
	_T("The new %s key fingerprint is:\n")
	_T("%s\n")
	_T("If you were expecting this change and trust the new key,\n")
	_T("hit Yes to update %s's cache and continue connecting.\n")
	_T("If you want to carry on connecting but without updating\n")
	_T("the cache, hit No.\n")
	_T("If you want to abandon the connection completely, hit\n")
	_T("Cancel. Hitting Cancel is the ONLY guaranteed safe\n") 
	_T("choice.\n");
#endif

    static const TCHAR mbtitle[] = _T("%s Security Alert");

    /*
     * Verify the key against the registry.
     */
    ret = verify_host_key(host, port, keytype, keystr);

    if (ret == 0)		       /* success - key matched OK */
	return;
    if (ret == 2) {		       /* key was different */
	int mbret;
	TCHAR *message, *title;
#ifdef _UNICODE
	wchar_t *wkeytype = ansi2unicode(keytype);
	wchar_t *wfingerprint = ansi2unicode(fingerprint);
	message = _duptprintf(wrongmsg, appname, wkeytype, wfingerprint, appname);
	sfree(wfingerprint);
	sfree(wkeytype);
#else
	message = dupprintf(wrongmsg, appname, keytype, fingerprint, appname);
#endif
	title = _duptprintf(mbtitle, appname);
	mbret = MessageBox(NULL, message, title,
			   MB_ICONWARNING | MB_YESNOCANCEL);
	sfree(message);
	sfree(title);
	if (mbret == IDYES)
	    store_host_key(host, port, keytype, keystr);
	if (mbret == IDCANCEL)
	    cleanup_exit(0);
    }
    if (ret == 1) {		       /* key was absent */
	int mbret;
	TCHAR *message, *title;
#ifdef _UNICODE
	wchar_t *wkeytype = ansi2unicode(keytype);
	wchar_t *wfingerprint = ansi2unicode(fingerprint);
	message = _duptprintf(absentmsg, wkeytype, wfingerprint, appname);
	sfree(wfingerprint);
	sfree(wkeytype);
#else
	message = dupprintf(absentmsg, keytype, fingerprint, appname);
#endif
	title = _duptprintf(mbtitle, appname);
	mbret = MessageBox(NULL, message, title,
			   MB_ICONWARNING | MB_YESNOCANCEL);
	sfree(message);
	sfree(title);
	if (mbret == IDYES)
	    store_host_key(host, port, keytype, keystr);
	if (mbret == IDCANCEL)
	    cleanup_exit(0);
    }
}

/*
 * Ask whether the selected cipher is acceptable (since it was
 * below the configured 'warn' threshold).
 * cs: 0 = both ways, 1 = client->server, 2 = server->client
 */
void askcipher(void *frontend, char *ciphername, int cs)
{
    static const TCHAR mbtitle[] = _T("%s Security Alert");
    static const TCHAR msg[] =
	_T("The first %.35scipher supported by the server\n")
	_T("is %.64s, which is below the configured\n")
	_T("warning threshold.\n")
	_T("Do you want to continue with this connection?\n");
    TCHAR *message, *title;
    int mbret;
#ifdef _UNICODE
    wchar_t *wciphername = ansi2unicode(ciphername);
    message = _duptprintf(msg, ((cs == 0) ? _T("") :
			      (cs == 1) ? _T("client-to-server ") :
			      _T("server-to-client ")), wciphername);
    sfree(wciphername);
#else
    message = dupprintf(msg, ((cs == 0) ? "" :
			      (cs == 1) ? "client-to-server " :
			      "server-to-client "), ciphername);
#endif

    title = _duptprintf(mbtitle, appname);
    mbret = MessageBox(NULL, message, title,
		       MB_ICONWARNING | MB_YESNO);
    sfree(message);
    sfree(title);
    if (mbret == IDYES)
	return;
    else
	cleanup_exit(0);
}

/*
 * Ask whether to wipe a session log file before writing to it.
 * Returns 2 for wipe, 1 for append, 0 for cancel (don't log).
 */
int askappend(void *frontend, Filename filename)
{
    static const TCHAR msgtemplate[] =
	_T("The session log file \"%.*s\" already exists.\n")
	_T("You can overwrite it with a new session log,\n")
	_T("append your session log to the end of it,\n")
	_T("or disable session logging for this session.\n")
	_T("Hit Yes to wipe the file, No to append to it,\n")
	_T("or Cancel to disable logging.");
    TCHAR *message;
    TCHAR *mbtitle;
    int mbret;
#ifdef _UNICODE
    wchar_t *filename_path = ansi2unicode(filename.path);
    message = _duptprintf(msgtemplate, FILENAME_MAX, filename.path);
    sfree(filename_path);
#else
    message = dupprintf(msgtemplate, FILENAME_MAX, filename.path);
#endif
    mbtitle = _duptprintf(_T("%s Log to File"), appname);

    mbret = MessageBox(NULL, message, mbtitle,
		       MB_ICONQUESTION | MB_YESNOCANCEL);

    sfree(message);
    sfree(mbtitle);

    if (mbret == IDYES)
	return 2;
    else if (mbret == IDNO)
	return 1;
    else
	return 0;
}

/*
 * Warn about the obsolescent key file format.
 * 
 * Uniquely among these functions, this one does _not_ expect a
 * frontend handle. This means that if PuTTY is ported to a
 * platform which requires frontend handles, this function will be
 * an anomaly. Fortunately, the problem it addresses will not have
 * been present on that platform, so it can plausibly be
 * implemented as an empty function.
 */
void old_keyfile_warning(void)
{
    static const TCHAR mbtitle[] = _T("%s Key File Warning");
    static const TCHAR message[] =
	_T("You are loading an SSH 2 private key which has an\n")
	_T("old version of the file format. This means your key\n")
	_T("file is not fully tamperproof. Future versions of\n")
	_T("%s may stop supporting this private key format,\n")
	_T("so we recommend you convert your key to the new\n")
	_T("format.\n")
	_T("\n")
	_T("You can perform this conversion by loading the key\n")
	_T("into PuTTYgen and then saving it again.");

    TCHAR *msg, *title;
    msg = _duptprintf(message, appname);
    title = _duptprintf(mbtitle, appname);

    MessageBox(NULL, msg, title, MB_OK);

    sfree(msg);
    sfree(title);
}
