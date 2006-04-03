/*
 * wincfg.c - the Windows-specific parts of the PuTTY configuration
 * box.
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

static void about_handler(union control *ctrl, void *dlg,
			  void *data, int event)
{
    HWND *hwndp = (HWND *)ctrl->generic.context.p;

    if (event == EVENT_ACTION) {
	modal_about_box(*hwndp);
    }
}

static void help_handler(union control *ctrl, void *dlg,
			 void *data, int event)
{
    HWND *hwndp = (HWND *)ctrl->generic.context.p;

    if (event == EVENT_ACTION) {
	show_help(*hwndp);
    }
}

void win_setup_config_box(struct controlbox *b, HWND *hwndp, int has_help,
			  int midsession)
{
    struct controlset *s;
    union control *c;
    TCHAR *str;

    if (!midsession) {
	/*
	 * Add the About and Help buttons to the standard panel.
	 */
	s = ctrl_getset(b, _T(""), _T(""), _T(""));
	c = ctrl_pushbutton(s, _T("About"), _T('a'), HELPCTX(no_help),
			    about_handler, P(hwndp));
	c->generic.column = 0;
	if (has_help) {
	    c = ctrl_pushbutton(s, _T("Help"), _T('h'), HELPCTX(no_help),
				help_handler, P(hwndp));
	    c->generic.column = 1;
	}
    }

    /*
     * Full-screen mode is a Windows peculiarity; hence
     * scrollbar_in_fullscreen is as well.
     */
    s = ctrl_getset(b, _T("Window"), _T("scrollback"),
		    _T("Control the scrollback in the window"));
    ctrl_checkbox(s, _T("Display scrollbar in full screen mode"), _T('i'),
		  HELPCTX(window_scrollback),
		  dlg_stdcheckbox_handler,
		  I(offsetof(Config,scrollbar_in_fullscreen)));
    /*
     * Really this wants to go just after `Display scrollbar'. See
     * if we can find that control, and do some shuffling.
     */
    {
        int i;
        for (i = 0; i < s->ncontrols; i++) {
            c = s->ctrls[i];
            if (c->generic.type == CTRL_CHECKBOX &&
                c->generic.context.i == offsetof(Config,scrollbar)) {
                /*
                 * Control i is the scrollbar checkbox.
                 * Control s->ncontrols-1 is the scrollbar-in-FS one.
                 */
                if (i < s->ncontrols-2) {
                    c = s->ctrls[s->ncontrols-1];
                    memmove(s->ctrls+i+2, s->ctrls+i+1,
                            (s->ncontrols-i-2)*sizeof(union control *));
                    s->ctrls[i+1] = c;
                }
                break;
            }
        }
    }

    /*
     * Windows has the AltGr key, which has various Windows-
     * specific options.
     */
    s = ctrl_getset(b, _T("Terminal/Keyboard"), _T("features"),
		    _T("Enable extra keyboard features:"));
    ctrl_checkbox(s, _T("AltGr acts as Compose key"), _T('t'),
		  HELPCTX(keyboard_compose),
		  dlg_stdcheckbox_handler, I(offsetof(Config,compose_key)));
    ctrl_checkbox(s, _T("Control-Alt is different from AltGr"), _T('d'),
		  HELPCTX(keyboard_ctrlalt),
		  dlg_stdcheckbox_handler, I(offsetof(Config,ctrlaltkeys)));

    /*
     * Windows allows an arbitrary .WAV to be played as a bell, and
     * also the use of the PC speaker. For this we must search the
     * existing controlset for the radio-button set controlling the
     * `beep' option, and add extra buttons to it.
     * 
     * Note that although this _looks_ like a hideous hack, it's
     * actually all above board. The well-defined interface to the
     * per-platform dialog box code is the _data structures_ `union
     * control', `struct controlset' and so on; so code like this
     * that reaches into those data structures and changes bits of
     * them is perfectly legitimate and crosses no boundaries. All
     * the ctrl_* routines that create most of the controls are
     * convenient shortcuts provided on the cross-platform side of
     * the interface, and template creation code is under no actual
     * obligation to use them.
     */
    s = ctrl_getset(b, _T("Terminal/Bell"), _T("style"), _T("Set the style of bell"));
    {
	int i;
	for (i = 0; i < s->ncontrols; i++) {
	    c = s->ctrls[i];
	    if (c->generic.type == CTRL_RADIO &&
		c->generic.context.i == offsetof(Config, beep)) {
		assert(c->generic.handler == dlg_stdradiobutton_handler);
		c->radio.nbuttons += 2;
		c->radio.buttons =
		    sresize(c->radio.buttons, c->radio.nbuttons, TCHAR *);
		c->radio.buttons[c->radio.nbuttons-1] =
		    _duptcs(_T("Play a custom sound file"));
		c->radio.buttons[c->radio.nbuttons-2] =
		    _duptcs(_T("Beep using the PC speaker"));
		c->radio.buttondata =
		    sresize(c->radio.buttondata, c->radio.nbuttons, intorptr);
		c->radio.buttondata[c->radio.nbuttons-1] = I(BELL_WAVEFILE);
		c->radio.buttondata[c->radio.nbuttons-2] = I(BELL_PCSPEAKER);
		if (c->radio.shortcuts) {
		    c->radio.shortcuts =
			sresize(c->radio.shortcuts, c->radio.nbuttons, TCHAR);
		    c->radio.shortcuts[c->radio.nbuttons-1] = NO_SHORTCUT;
		    c->radio.shortcuts[c->radio.nbuttons-2] = NO_SHORTCUT;
		}
		break;
	    }
	}
    }
    ctrl_filesel(s, _T("Custom sound file to play as a bell:"), NO_SHORTCUT,
		 FILTER_WAVE_FILES, FALSE, _T("Select bell sound file"),
		 HELPCTX(bell_style),
		 dlg_stdfilesel_handler, I(offsetof(Config, bell_wavefile)));

    /*
     * While we've got this box open, taskbar flashing on a bell is
     * also Windows-specific.
     */
    ctrl_radiobuttons(s, _T("Taskbar/caption indication on bell:"), _T('i'), 3,
		      HELPCTX(bell_taskbar),
		      dlg_stdradiobutton_handler,
		      I(offsetof(Config, beep_ind)),
		      _T("Disabled"), I(B_IND_DISABLED),
		      _T("Flashing"), I(B_IND_FLASH),
		      _T("Steady"), I(B_IND_STEADY), NULL);

    /*
     * The sunken-edge border is a Windows GUI feature.
     */
    s = ctrl_getset(b, _T("Window/Appearance"), _T("border"),
		    _T("Adjust the window border"));
    ctrl_checkbox(s, _T("Sunken-edge border (slightly thicker)"), _T('s'),
		  HELPCTX(appearance_border),
		  dlg_stdcheckbox_handler, I(offsetof(Config,sunken_edge)));

    /*
     * Cyrillic Lock is a horrid misfeature even on Windows, and
     * the least we can do is ensure it never makes it to any other
     * platform (at least unless someone fixes it!).
     */
    s = ctrl_getset(b, _T("Window/Translation"), _T("input"),
		    _T("Enable character set translation on input data"));
    ctrl_checkbox(s, _T("Caps Lock acts as Cyrillic switch"), _T('s'),
		  HELPCTX(translation_cyrillic),
		  dlg_stdcheckbox_handler,
		  I(offsetof(Config,xlat_capslockcyr)));

    /*
     * Windows has the weird OEM font mode, which gives us some
     * additional options when working with line-drawing
     * characters.
     */
    str = _duptprintf(_T("Adjust how %s displays line drawing characters"), appname);
    s = ctrl_getset(b, _T("Window/Translation"), _T("linedraw"), str);
    sfree(str);
    {
	int i;
	for (i = 0; i < s->ncontrols; i++) {
	    c = s->ctrls[i];
	    if (c->generic.type == CTRL_RADIO &&
		c->generic.context.i == offsetof(Config, vtmode)) {
		assert(c->generic.handler == dlg_stdradiobutton_handler);
		c->radio.nbuttons += 3;
		c->radio.buttons =
		    sresize(c->radio.buttons, c->radio.nbuttons, TCHAR *);
		c->radio.buttons[c->radio.nbuttons-3] =
		    _duptcs(_T("Font has XWindows encoding"));
		c->radio.buttons[c->radio.nbuttons-2] =
		    _duptcs(_T("Use font in both ANSI and OEM modes"));
		c->radio.buttons[c->radio.nbuttons-1] =
		    _duptcs(_T("Use font in OEM mode only"));
		c->radio.buttondata =
		    sresize(c->radio.buttondata, c->radio.nbuttons, intorptr);
		c->radio.buttondata[c->radio.nbuttons-3] = I(VT_XWINDOWS);
		c->radio.buttondata[c->radio.nbuttons-2] = I(VT_OEMANSI);
		c->radio.buttondata[c->radio.nbuttons-1] = I(VT_OEMONLY);
		if (!c->radio.shortcuts) {
		    int j;
		    c->radio.shortcuts = snewn(c->radio.nbuttons, TCHAR);
		    for (j = 0; j < c->radio.nbuttons; j++)
			c->radio.shortcuts[j] = NO_SHORTCUT;
		} else {
		    c->radio.shortcuts = sresize(c->radio.shortcuts,
						 c->radio.nbuttons, TCHAR);
		}
		c->radio.shortcuts[c->radio.nbuttons-3] = _T('x');
		c->radio.shortcuts[c->radio.nbuttons-2] = _T('b');
		c->radio.shortcuts[c->radio.nbuttons-1] = _T('e');
		break;
	    }
	}
    }

    /*
     * RTF paste is Windows-specific.
     */
    s = ctrl_getset(b, _T("Window/Selection"), _T("trans"),
		    _T("Translation of pasted characters"));
    ctrl_checkbox(s, _T("Paste to clipboard in RTF as well as plain text"), _T('f'),
		  HELPCTX(selection_rtf),
		  dlg_stdcheckbox_handler, I(offsetof(Config,rtf_paste)));

    /*
     * Windows often has no middle button, so we supply a selection
     * mode in which the more critical Paste action is available on
     * the right button instead.
     */
    s = ctrl_getset(b, _T("Window/Selection"), _T("mouse"),
		    _T("Control use of mouse"));
    ctrl_radiobuttons(s, _T("Action of mouse buttons:"), NO_SHORTCUT, 1,
		      HELPCTX(selection_buttons),
		      dlg_stdradiobutton_handler,
		      I(offsetof(Config, mouse_is_xterm)),
		      "Windows (Right pastes, Middle extends)", 'w', I(0),
		      "xterm (Right extends, Middle pastes)", 'x', I(1), NULL);
    /*
     * This really ought to go at the _top_ of its box, not the
     * bottom, so we'll just do some shuffling now we've set it
     * up...
     */
    c = s->ctrls[s->ncontrols-1];      /* this should be the new control */
    memmove(s->ctrls+1, s->ctrls, (s->ncontrols-1)*sizeof(union control *));
    s->ctrls[0] = c;

    /*
     * Logical palettes don't even make sense anywhere except Windows.
     */
    s = ctrl_getset(b, _T("Window/Colours"), _T("general"),
		    _T("General options for colour usage"));
    ctrl_checkbox(s, _T("Attempt to use logical palettes"), _T('l'),
		  HELPCTX(colours_logpal),
		  dlg_stdcheckbox_handler, I(offsetof(Config,try_palette)));
    ctrl_checkbox(s, _T("Use system colours"), _T('s'),
                  HELPCTX(colours_system),
                  dlg_stdcheckbox_handler, I(offsetof(Config,system_colour)));


    /*
     * Resize-by-changing-font is a Windows insanity.
     */
    s = ctrl_getset(b, _T("Window"), _T("size"), _T("Set the size of the window"));
    ctrl_radiobuttons(s, _T("When window is resized:"), _T('z'), 1,
		      HELPCTX(window_resize),
		      dlg_stdradiobutton_handler,
		      I(offsetof(Config, resize_action)),
		      _T("Change the number of rows and columns"), I(RESIZE_TERM),
		      _T("Change the size of the font"), I(RESIZE_FONT),
		      _T("Change font size only when maximised"), I(RESIZE_EITHER),
		      _T("Forbid resizing completely"), I(RESIZE_DISABLED), NULL);

    /*
     * Most of the Window/Behaviour stuff is there to mimic Windows
     * conventions which PuTTY can optionally disregard. Hence,
     * most of these options are Windows-specific.
     */
    s = ctrl_getset(b, _T("Window/Behaviour"), _T("main"), NULL);
    ctrl_checkbox(s, _T("Window closes on ALT-F4"), _T('4'),
		  HELPCTX(behaviour_altf4),
		  dlg_stdcheckbox_handler, I(offsetof(Config,alt_f4)));
    ctrl_checkbox(s, _T("System menu appears on ALT-Space"), _T('y'),
		  HELPCTX(behaviour_altspace),
		  dlg_stdcheckbox_handler, I(offsetof(Config,alt_space)));
    ctrl_checkbox(s, _T("System menu appears on ALT alone"), _T('l'),
		  HELPCTX(behaviour_altonly),
		  dlg_stdcheckbox_handler, I(offsetof(Config,alt_only)));
    ctrl_checkbox(s, _T("Ensure window is always on top"), _T('e'),
		  HELPCTX(behaviour_alwaysontop),
		  dlg_stdcheckbox_handler, I(offsetof(Config,alwaysontop)));
    ctrl_checkbox(s, _T("Full screen on Alt-Enter"), _T('f'),
		  HELPCTX(behaviour_altenter),
		  dlg_stdcheckbox_handler,
		  I(offsetof(Config,fullscreenonaltenter)));
}
