#include <stdio.h>

#if !(UNDER_CE > 0 && UNDER_CE < 400)
#include <assert.h>
#else
#include "wceassrt.h"
#endif

#include <stdlib.h>
#include "putty.h"

/*
 * Some command-line parameters need to be saved up until after
 * we've loaded the saved session which will form the basis of our
 * eventual running configuration. For this we use the macro
 * SAVEABLE, which notices if the `need_save' parameter is set and
 * saves the parameter and value on a list.
 * 
 * We also assign priorities to saved parameters, just to slightly
 * ameliorate silly ordering problems. For example, if you specify
 * a saved session to load, it will be loaded _before_ all your
 * local modifications such as -L are evaluated; and if you specify
 * a protocol and a port, the protocol is set up first so that the
 * port can override its choice of port number.
 * 
 * (In fact -load is not saved at all, since in at least Plink the
 * processing of further command-line options depends on whether or
 * not the loaded session contained a hostname. So it must be
 * executed immediately.)
 */

#define NPRIORITIES 2

struct cmdline_saved_param {
    TCHAR *p, *value;
};
struct cmdline_saved_param_set {
    struct cmdline_saved_param *params;
    int nsaved, savesize;
};

/*
 * C guarantees this structure will be initialised to all zero at
 * program start, which is exactly what we want.
 */
static struct cmdline_saved_param_set saves[NPRIORITIES];

static void cmdline_save_param(TCHAR *p, TCHAR *value, int pri)
{
    if (saves[pri].nsaved >= saves[pri].savesize) {
	saves[pri].savesize = saves[pri].nsaved + 32;
	saves[pri].params = sresize(saves[pri].params, saves[pri].savesize,
				    struct cmdline_saved_param);
    }
    saves[pri].params[saves[pri].nsaved].p = p;
    saves[pri].params[saves[pri].nsaved].value = value;
    saves[pri].nsaved++;
}

#define SAVEABLE(pri) do { \
    if (need_save) { cmdline_save_param(p, value, pri); return ret; } \
} while (0)

char *cmdline_password = NULL;

static int cmdline_get_line(const char *prompt, char *str,
                            int maxlen, int is_pw)
{
    static int tried_once = 0;

    assert(is_pw && cmdline_password);

    if (tried_once) {
		return 0;
    } else {
		strncpy(str, cmdline_password, maxlen);
		str[maxlen - 1] = '\0';
		tried_once = 1;
		return 1;
    }
}

/*
 * Here we have a flags word which describes the capabilities of
 * the particular tool on whose behalf we're running. We will
 * refuse certain command-line options if a particular tool
 * inherently can't do anything sensible. For example, the file
 * transfer tools (psftp, pscp) can't do a great deal with protocol
 * selections (ever tried running scp over telnet?) or with port
 * forwarding (even if it wasn't a hideously bad idea, they don't
 * have the select() infrastructure to make them work).
 */
int cmdline_tooltype = 0;

static int cmdline_check_unavailable(int flag, TCHAR *p)
{
    if (cmdline_tooltype & flag) {
		cmdline_error(_T("option \"%s\" not available in this tool"), p);
		return 1;
    }
    return 0;
}

#define UNAVAILABLE_IN(flag) do { \
    if (cmdline_check_unavailable(flag, p)) return ret; \
} while (0)

/*
 * Process a standard command-line parameter. `p' is the parameter
 * in question; `value' is the subsequent element of argv, which
 * may or may not be required as an operand to the parameter.
 * Return value is 2 if both arguments were used; 1 if only p was
 * used; 0 if the parameter wasn't one we recognised; -2 if it
 * should have been 2 but value was NULL.
 */

#define RETURN(x) do { \
    if ((x) == 2 && !value) return -2; \
    ret = x; \
    if (need_save < 0) return x; \
} while (0)

int cmdline_process_param(TCHAR *p, TCHAR *value, int need_save, Config *cfg)
{
    int ret = 0;

    if (!_tcscmp(p, _T("-load"))) {
		RETURN(2);
		/* This parameter must be processed immediately rather than being
		 * saved. */
		do_defaults(value, cfg);
		return 2;
    }
    if (!_tcscmp(p, _T("-ssh"))) {
	RETURN(1);
	UNAVAILABLE_IN(TOOLTYPE_FILETRANSFER | TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
	default_protocol = cfg->protocol = PROT_SSH;
	default_port = cfg->port = 22;
	return 1;
    }
    if (!_tcscmp(p, _T("-telnet"))) {
	RETURN(1);
	UNAVAILABLE_IN(TOOLTYPE_FILETRANSFER | TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
	default_protocol = cfg->protocol = PROT_TELNET;
	default_port = cfg->port = 23;
	return 1;
    }
    if (!_tcscmp(p, _T("-rlogin"))) {
	RETURN(1);
	UNAVAILABLE_IN(TOOLTYPE_FILETRANSFER | TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
	default_protocol = cfg->protocol = PROT_RLOGIN;
	default_port = cfg->port = 513;
	return 1;
    }
    if (!_tcscmp(p, _T("-raw"))) {
	RETURN(1);
	UNAVAILABLE_IN(TOOLTYPE_FILETRANSFER | TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
	default_protocol = cfg->protocol = PROT_RAW;
    }
    if (!_tcscmp(p, _T("-v"))) {
	RETURN(1);
	flags |= FLAG_VERBOSE;
    }
    if (!_tcscmp(p, _T("-l"))) {
	RETURN(2);
	UNAVAILABLE_IN(TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
	_strncpyt(cfg->username, value, sizeof(cfg->username));
	cfg->username[sizeof(cfg->username) - 1] = '\0';
    }
    if ((!_tcscmp(p, _T("-L")) || !_tcscmp(p, _T("-R")) || !_tcscmp(p, _T("-D")))) {
	TCHAR *fwd;
	char *q, *qq, *ptr;
	int dynamic, i=0;
	RETURN(2);
	UNAVAILABLE_IN(TOOLTYPE_FILETRANSFER | TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
	dynamic = !_tcscmp(p, _T("-D"));
	fwd = value;
	ptr = cfg->portfwd;
	/* if multiple forwards, find end of list */
	if (ptr[0]=='R' || ptr[0]=='L' || ptr[0] == 'D') {
	    for (i = 0; i < sizeof(cfg->portfwd) - 2; i++)
		if (ptr[i]=='\000' && ptr[i+1]=='\000')
		    break;
	    ptr = ptr + i + 1;  /* point to next forward slot */
	}
#ifdef _UNICODE
	wcstombs(&ptr[0], &p[1], 1); /* insert a 'L', 'R' or 'D' at the start */
#else
	ptr[0] = p[1];  /* insert a 'L', 'R' or 'D' at the start */
#endif
	if (_tcsclen(fwd) > sizeof(cfg->portfwd) - i - 2) {
	    cmdline_error(_T("out of space for port forwardings"));
	    return ret;
	}
	_strncpyt(ptr + 1, fwd, sizeof(cfg->portfwd) - i);
	if (!dynamic) {
	    /*
	     * We expect _at least_ two colons in this string. The
	     * possible formats are `sourceport:desthost:destport',
	     * or `sourceip:sourceport:desthost:destport' if you're
	     * specifying a particular loopback address. We need to
	     * replace the one between source and dest with a \t;
	     * this means we must find the second-to-last colon in
	     * the string.
	     */
	    q = qq = strchr(ptr, ':');
	    while (qq) {
		char *qqq = strchr(qq+1, ':');
		if (qqq)
		    q = qq;
		qq = qqq;
	    }
	    if (q) *q = '\t';	       /* replace second-last colon with \t */
	}
	cfg->portfwd[sizeof(cfg->portfwd) - 1] = _T('\0');
	cfg->portfwd[sizeof(cfg->portfwd) - 2] = _T('\0');
	ptr[strlen(ptr)+1] = '\000';    /* append two '\000' */
    }
    if (!_tcscmp(p, _T("-m"))) {
	TCHAR *filename;
	char *command;
	int cmdlen, cmdsize;
	FILE *fp;
	int c, d;

	RETURN(2);
	UNAVAILABLE_IN(TOOLTYPE_FILETRANSFER | TOOLTYPE_NONNETWORK);
	SAVEABLE(0);

	filename = value;

	cmdlen = cmdsize = 0;
	command = NULL;
	fp = _tfopen(filename, _T("r"));
	if (!fp) {
	    cmdline_error(_T("unable to open command ")
			  _T("file \"%s\""), filename);
	    return ret;
	}
	do {
	    c = fgetc(fp);
	    d = c;
	    if (c == _TEOF)
		d = 0;
	    if (cmdlen >= cmdsize) {
		cmdsize = cmdlen + 512;
		command = sresize(command, cmdsize, char);
	    }
	    command[cmdlen++] = d;
	} while (c != EOF);
	cfg->remote_cmd_ptr = command;
	cfg->remote_cmd_ptr2 = NULL;
	cfg->nopty = TRUE;      /* command => no terminal */
    }
    if (!_tcscmp(p, _T("-P"))) {
	RETURN(2);
	UNAVAILABLE_IN(TOOLTYPE_NONNETWORK);
	SAVEABLE(1);		       /* lower priority than -ssh,-telnet */
	cfg->port = _ttoi(value);
    }
    if (!_tcscmp(p, _T("-pw"))) {
		RETURN(2);
		UNAVAILABLE_IN(TOOLTYPE_NONNETWORK);
		cmdline_password = unicode2ansi(value);
		ssh_get_line = cmdline_get_line;
		ssh_getline_pw_only = TRUE;
    }

    if (!_tcscmp(p, _T("-A"))) {
	RETURN(1);
	UNAVAILABLE_IN(TOOLTYPE_FILETRANSFER | TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
	cfg->agentfwd = 1;
    }
    if (!_tcscmp(p, _T("-a"))) {
	RETURN(1);
	UNAVAILABLE_IN(TOOLTYPE_FILETRANSFER | TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
	cfg->agentfwd = 0;
    }

    if (!_tcscmp(p, _T("-X"))) {
	RETURN(1);
	UNAVAILABLE_IN(TOOLTYPE_FILETRANSFER | TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
	cfg->x11_forward = 1;
    }
    if (!_tcscmp(p, _T("-x"))) {
	RETURN(1);
	UNAVAILABLE_IN(TOOLTYPE_FILETRANSFER | TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
	cfg->x11_forward = 0;
    }

    if (!_tcscmp(p, _T("-t"))) {
	RETURN(1);
	UNAVAILABLE_IN(TOOLTYPE_FILETRANSFER | TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
	cfg->nopty = 0;
    }
    if (!_tcscmp(p, _T("-T"))) {
	RETURN(1);
	UNAVAILABLE_IN(TOOLTYPE_FILETRANSFER | TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
	cfg->nopty = 1;
    }

    if (!_tcscmp(p, _T("-C"))) {
	RETURN(1);
	UNAVAILABLE_IN(TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
	cfg->compression = 1;
    }

    if (!_tcscmp(p, _T("-1"))) {
	RETURN(1);
	UNAVAILABLE_IN(TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
	cfg->sshprot = 0;	       /* ssh protocol 1 only */
    }
    if (!_tcscmp(p, _T("-2"))) {
	RETURN(1);
	UNAVAILABLE_IN(TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
	cfg->sshprot = 3;	       /* ssh protocol 2 only */
    }

    if (!_tcscmp(p, _T("-i"))) {
	RETURN(2);
	UNAVAILABLE_IN(TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
	cfg->keyfile = filename_from_tcs(value);
    }

    return ret;			       /* unrecognised */
}

void cmdline_run_saved(Config *cfg)
{
    int pri, i;
    for (pri = 0; pri < NPRIORITIES; pri++)
	for (i = 0; i < saves[pri].nsaved; i++)
	    cmdline_process_param(saves[pri].params[i].p,
				  saves[pri].params[i].value, 0, cfg);
}
