/*
   Copyright (C) 2002-2010 Karl J. Runge <runge@karlrunge.com> 
   All rights reserved.

This file is part of x11vnc.

x11vnc is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

x11vnc is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with x11vnc; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
or see <http://www.gnu.org/licenses/>.

In addition, as a special exception, Karl J. Runge
gives permission to link the code of its release of x11vnc with the
OpenSSL project's "OpenSSL" library (or with modified versions of it
that use the same license as the "OpenSSL" library), and distribute
the linked executables.  You must obey the GNU General Public License
in all respects for all of the code used other than "OpenSSL".  If you
modify this file, you may extend this exception to your version of the
file, but you are not obligated to do so.  If you do not wish to do
so, delete this exception statement from your version.
*/

/* -- keyboard.c -- */

#include "x11vnc.h"
#include "xwrappers.h"
#include "xrecord.h"
#include "xinerama.h"
#include "pointer.h"
#include "userinput.h"
#include "win_utils.h"
#include "rates.h"
#include "cleanup.h"
#include "allowed_input_t.h"
#include "unixpw.h"
#include "v4l.h"
#include "linuxfb.h"
#include "uinput.h"
#include "macosx.h"
#include "screen.h"

void get_keystate(int *keystate);
void clear_modifiers(int init);
int track_mod_state(rfbKeySym keysym, rfbBool down, rfbBool set);
void clear_keys(void);
void clear_locks(void);
int get_autorepeat_state(void);
int get_initial_autorepeat_state(void);
void autorepeat(int restore, int bequiet);
void check_add_keysyms(void);
int add_keysym(KeySym keysym);
void delete_added_keycodes(int bequiet);
void initialize_remap(char *infile);
int sloppy_key_check(int key, rfbBool down, rfbKeySym keysym, int *new_kc);
void switch_to_xkb_if_better(void);
char *short_kmbcf(char *str);
void initialize_allowed_input(void);
void initialize_modtweak(void);
void initialize_keyboard_and_pointer(void);
void get_allowed_input(rfbClientPtr client, allowed_input_t *input);
double typing_rate(double time_window, int *repeating);
int skip_cr_when_scaling(char *mode);
void keyboard(rfbBool down, rfbKeySym keysym, rfbClientPtr client);


static void delete_keycode(KeyCode kc, int bequiet);
static int count_added_keycodes(void);
static void add_remap(char *line);
static void add_dead_keysyms(char *str);
static void initialize_xkb_modtweak(void);
static void xkb_tweak_keyboard(rfbBool down, rfbKeySym keysym,
    rfbClientPtr client);
static void tweak_mod(signed char mod, rfbBool down);
static void modifier_tweak_keyboard(rfbBool down, rfbKeySym keysym,
    rfbClientPtr client);
static void pipe_keyboard(rfbBool down, rfbKeySym keysym, rfbClientPtr client);


/*
 * Routine to retreive current state keyboard.  1 means down, 0 up.
 */
void get_keystate(int *keystate) {
#if NO_X11
	RAWFB_RET_VOID
	if (!keystate) {}
	return;
#else
	int i, k;
	char keys[32];

	RAWFB_RET_VOID
	
	/* n.b. caller decides to X_LOCK or not. */
	XQueryKeymap(dpy, keys);
	for (i=0; i<32; i++) {
		char c = keys[i];

		for (k=0; k < 8; k++) {
			if (c & 0x1) {
				keystate[8*i + k] = 1;
			} else {
				keystate[8*i + k] = 0;
			}
			c = c >> 1;
		}
	}
#endif	/* NO_X11 */
}

/*
 * Try to KeyRelease any non-Lock modifiers that are down.
 */
void clear_modifiers(int init) {
#if NO_X11
	RAWFB_RET_VOID
	if (!init) {}
	return;
#else
	static KeyCode keycodes[256];
	static KeySym  keysyms[256];
	static char *keystrs[256];
	static int kcount = 0, first = 1;
	int keystate[256];
	int i, j, minkey, maxkey, syms_per_keycode;
	KeySym *keymap;
	KeySym keysym;
	KeyCode keycode;

	RAWFB_RET_VOID

	/* n.b. caller decides to X_LOCK or not. */
	if (first) {
		/*
		 * we store results in static arrays, to aid interrupted
		 * case, but modifiers could have changed during session...
		 */
		XDisplayKeycodes(dpy, &minkey, &maxkey);

		keymap = XGetKeyboardMapping(dpy, minkey, (maxkey - minkey + 1),
		    &syms_per_keycode);

		for (i = minkey; i <= maxkey; i++) {
		    for (j = 0; j < syms_per_keycode; j++) {
			char *str;
			keysym = keymap[ (i - minkey) * syms_per_keycode + j ];
			if (keysym == NoSymbol || ! ismodkey(keysym)) {
				continue;
			}
			keycode = XKeysymToKeycode(dpy, keysym);
			if (keycode == NoSymbol) {
				continue;
			}
			keycodes[kcount] = keycode;
			keysyms[kcount]  = keysym;
			str = XKeysymToString(keysym);
			if (! str) str = "null";
			keystrs[kcount]  = strdup(str);
			kcount++;
		    }
		}
		XFree_wr((void *) keymap);
		first = 0;
	}
	if (init) {
		return;
	}
	
	get_keystate(keystate);
	for (i=0; i < kcount; i++) {
		keysym  = keysyms[i];
		keycode = keycodes[i];

		if (! keystate[(int) keycode]) {
			continue;
		}

		if (clear_mods) {
			rfbLog("clear_modifiers: up: %-10s (0x%x) "
			    "keycode=0x%x\n", keystrs[i], keysym, keycode);
		}
		XTestFakeKeyEvent_wr(dpy, keycode, False, CurrentTime);
	}
	XFlush_wr(dpy);
#endif	/* NO_X11 */
}

static KeySym simple_mods[] = {
	XK_Shift_L, XK_Shift_R,
	XK_Control_L, XK_Control_R,
	XK_Meta_L, XK_Meta_R,
	XK_Alt_L, XK_Alt_R,
	XK_Super_L, XK_Super_R,
	XK_Hyper_L, XK_Hyper_R,
	XK_Mode_switch,
	NoSymbol
};
#define NSIMPLE_MODS 13

int track_mod_state(rfbKeySym keysym, rfbBool down, rfbBool set) {
	KeySym sym = (KeySym) keysym;	
	static rfbBool isdown[NSIMPLE_MODS];
	static int first = 1;
	int i, cnt = 0;

	/*
	 * simple tracking method for the modifier state without
	 * contacting the Xserver.  Ignores, of course what keys are
	 * pressed on the physical display.
	 *
	 * This is unrelated to our mod_tweak and xkb stuff.
	 * Just a simple thing for wireframe/scroll heuristics, 
	 * sloppy keys etc.
	 */

	if (first) {
		for (i=0; i<NSIMPLE_MODS; i++) {
			isdown[i] = FALSE;
		}
		first = 0;
	}

	if (sym != NoSymbol) {
		for (i=0; i<NSIMPLE_MODS; i++) {
			if (sym == simple_mods[i]) {
				if (set) {
					isdown[i] = down;
					return 1;
				} else {
					if (isdown[i]) {
						return 1;
					} else {
						return 0;
					}
				}
				break;
			}
		}
		/* not a modifier */
		if (set) {
			return 0;
		} else {
			return -1;
		}
	}

	/* called with NoSymbol: return number currently pressed: */
	for (i=0; i<NSIMPLE_MODS; i++) {
		if (isdown[i]) {
			cnt++;
		}
	}
	return cnt;
}

/*
 * Attempt to set all keys to Up position.  Can mess up typing at the
 * physical keyboard so use with caution.
 */
void clear_keys(void) {
	int k, keystate[256];

	RAWFB_RET_VOID
	
	/* n.b. caller decides to X_LOCK or not. */
	get_keystate(keystate);
	for (k=0; k<256; k++) {
		if (keystate[k]) {
			KeyCode keycode = (KeyCode) k;
			rfbLog("clear_keys: keycode=%d\n", keycode);
			XTestFakeKeyEvent_wr(dpy, keycode, False, CurrentTime);
		}
	}
	XFlush_wr(dpy);
}
		

void clear_locks(void) {
#if NO_X11
	RAWFB_RET_VOID
	return;
#else
	XModifierKeymap *map;
	int i, j, k = 0;
	unsigned int state = 0;

	RAWFB_RET_VOID

	/* n.b. caller decides to X_LOCK or not. */
#if LIBVNCSERVER_HAVE_XKEYBOARD
	if (xkb_present) {
		XkbStateRec kbstate;
		XkbGetState(dpy, XkbUseCoreKbd, &kbstate);
		rfbLog("locked:  0x%x\n", kbstate.locked_mods);
		rfbLog("latched: 0x%x\n", kbstate.latched_mods);
		rfbLog("compat:  0x%x\n", kbstate.compat_state);
		state = kbstate.locked_mods;
		if (! state) {
			state = kbstate.compat_state;
		}
	} else 
#endif
	{
		state = mask_state();
		/* this may contain non-locks too... */
		rfbLog("state:   0x%x\n", state);
	}
	if (! state) {
		return;
	}
	map = XGetModifierMapping(dpy);
	if (! map) {
		return;
	}
	for (i = 0; i < 8; i++) {
		int did = 0;
		for (j = 0; j < map->max_keypermod; j++) {
			if (! did && state & (0x1 << i)) {
				if (map->modifiermap[k]) {
					KeyCode key = map->modifiermap[k];
					KeySym ks = XKeycodeToKeysym(dpy, key, 0);
					char *nm = XKeysymToString(ks);
					rfbLog("toggling: %03d / %03d -- %s\n", key, ks, nm ? nm : "BadKey");
					did = 1;
					XTestFakeKeyEvent_wr(dpy, key, True, CurrentTime);
					usleep(10*1000);
					XTestFakeKeyEvent_wr(dpy, key, False, CurrentTime);
					XFlush_wr(dpy);
				}
			}
			k++;
		}
	}
	XFreeModifiermap(map);
	XFlush_wr(dpy);
	rfbLog("state:   0x%x\n", mask_state());
#endif
}

/*
 * Kludge for -norepeat option: we turn off keystroke autorepeat in
 * the X server when clients are connected.  This may annoy people at
 * the physical display.  We do this because 'key down' and 'key up'
 * user input events may be separated by 100s of ms due to screen fb
 * processing or link latency, thereby inducing the X server to apply
 * autorepeat when it should not.  Since the *client* is likely doing
 * keystroke autorepeating as well, it kind of makes sense to shut it
 * off if no one is at the physical display...
 */
static int save_auto_repeat = -1;

int get_autorepeat_state(void) {
#if NO_X11
	RAWFB_RET(0)
	return 0;
#else
	XKeyboardState kstate;

	RAWFB_RET(0)

	X_LOCK;
	XGetKeyboardControl(dpy, &kstate);
	X_UNLOCK;
	return kstate.global_auto_repeat;
#endif	/* NO_X11 */
}

int get_initial_autorepeat_state(void) {
	if (save_auto_repeat < 0) {
		save_auto_repeat = get_autorepeat_state();
	}
	return save_auto_repeat;
}

void autorepeat(int restore, int bequiet) {
#if NO_X11
	RAWFB_RET_VOID
	if (!restore || !bequiet) {}
	return;
#else
	int global_auto_repeat;
	XKeyboardControl kctrl;

	RAWFB_RET_VOID

	if (restore) {
		if (save_auto_repeat < 0) {
			return;		/* nothing to restore */
		}
		global_auto_repeat = get_autorepeat_state();
		/* read state and skip restore if equal (e.g. no clients) */
		if (global_auto_repeat == save_auto_repeat) {
			return;
		}

		X_LOCK;
		kctrl.auto_repeat_mode = save_auto_repeat;
		XChangeKeyboardControl(dpy, KBAutoRepeatMode, &kctrl);
		XFlush_wr(dpy);
		X_UNLOCK;

		if (! bequiet && ! quiet) {
			rfbLog("Restored X server key autorepeat to: %d\n",
			    save_auto_repeat);
		}
	} else {
		global_auto_repeat = get_autorepeat_state();
		if (save_auto_repeat < 0) {
			/*
			 * we only remember the state at startup
			 * to avoid confusing ourselves later on.
			 */
			save_auto_repeat = global_auto_repeat;
		}

		X_LOCK;
		kctrl.auto_repeat_mode = AutoRepeatModeOff;
		XChangeKeyboardControl(dpy, KBAutoRepeatMode, &kctrl);
		XFlush_wr(dpy);
		X_UNLOCK;

		if (! bequiet && ! quiet) {
			rfbLog("Disabled X server key autorepeat.\n");
			if (no_repeat_countdown >= 0) {
				rfbLog("  to force back on run: 'xset r on' (%d "
				    "times)\n", no_repeat_countdown+1);
			}
		}
	}
#endif	/* NO_X11 */
}

/*
 * We periodically delete any keysyms we have added, this is to
 * lessen our effect on the X server state if we are terminated abruptly
 * and cannot clear them and also to clear out any strange little used
 * ones that would just fill up the keymapping. 
 */
void check_add_keysyms(void) {
	static time_t last_check = 0;
	int clear_freq = 300, quiet = 1, count; 
	time_t now = time(NULL);

	if (unixpw_in_progress) return;

	if (now > last_check + clear_freq) {
		count = count_added_keycodes();
		/*
		 * only really delete if they have not typed recently
		 * and we have added 8 or more.
		 */
		if (now > last_keyboard_input + 5 && count >= 8) {
			X_LOCK;
			delete_added_keycodes(quiet);
			X_UNLOCK;
		}
		last_check = now;
	}
}

static KeySym added_keysyms[0x100];

/* these are just for rfbLog messages: */
static KeySym alltime_added_keysyms[1024];
static int alltime_len = 1024;
static int alltime_num = 0;

int add_keysym(KeySym keysym) {
	static int first = 1;
	int n;
#if NO_X11
	if (first) {
		for (n=0; n < 0x100; n++) {
			added_keysyms[n] = NoSymbol;
		}
		first = 0;
	}
	RAWFB_RET(0)
	if (!keysym) {}
	return 0;
#else
	int minkey, maxkey, syms_per_keycode;
	int kc, ret = 0;
	KeySym *keymap;

	if (first) {
		for (n=0; n < 0x100; n++) {
			added_keysyms[n] = NoSymbol;
		}
		first = 0;
	}

	RAWFB_RET(0)

	if (keysym == NoSymbol) {
		return 0;
	}
	/* there can be a race before MappingNotify */
	for (n=0; n < 0x100; n++) {
		if (added_keysyms[n] == keysym) {
			return n;
		}
	}

	XDisplayKeycodes(dpy, &minkey, &maxkey);
	keymap = XGetKeyboardMapping(dpy, minkey, (maxkey - minkey + 1),
	    &syms_per_keycode);

	for (kc = minkey+1; kc <= maxkey; kc++) {
		int i, j, didmsg = 0, is_empty = 1;
		char *str;
		KeySym newks[8];

		for (n=0; n < syms_per_keycode; n++) {
			if (keymap[ (kc-minkey) * syms_per_keycode + n]
			    != NoSymbol) {
				is_empty = 0;
				break;
			}
		}
		if (! is_empty) {
			continue;
		}

		for (i=0; i<8; i++) {
			newks[i] = NoSymbol;
		}
		if (add_keysyms == 2) {
			newks[0] = keysym;	/* XXX remove me */
		} else {
			for(i=0; i < syms_per_keycode; i++) {
				newks[i] = keysym;
				if (i >= 7) break;
			}
		}

		XChangeKeyboardMapping(dpy, kc, syms_per_keycode,
		    newks, 1);

		if (alltime_num >= alltime_len) {
			didmsg = 1;	/* something weird */
		} else {
			for (j=0; j<alltime_num; j++) {
				if (alltime_added_keysyms[j] == keysym) {
					didmsg = 1;
					break;
				}
			}
		}
		if (! didmsg) {
			str = XKeysymToString(keysym);
			rfbLog("added missing keysym to X display: %03d "
			    "0x%x \"%s\"\n", kc, keysym, str ? str : "null");

			if (alltime_num < alltime_len) {
				alltime_added_keysyms[alltime_num++] = keysym;
			}
		}

		XFlush_wr(dpy);
		added_keysyms[kc] = keysym;
		ret = kc;
		break;
	}
	XFree_wr(keymap);
	return ret;
#endif	/* NO_X11 */
}

static void delete_keycode(KeyCode kc, int bequiet) {
#if NO_X11
	RAWFB_RET_VOID
	if (!kc || !bequiet) {}
	return;
#else
	int minkey, maxkey, syms_per_keycode, i;
	KeySym *keymap;
	KeySym ksym, newks[8];
	char *str;

	RAWFB_RET_VOID

	XDisplayKeycodes(dpy, &minkey, &maxkey);
	keymap = XGetKeyboardMapping(dpy, minkey, (maxkey - minkey + 1),
	    &syms_per_keycode);

	for (i=0; i<8; i++) {
		newks[i] = NoSymbol;
	}

	XChangeKeyboardMapping(dpy, kc, syms_per_keycode, newks, 1);

	if (! bequiet && ! quiet) {
		ksym = XKeycodeToKeysym(dpy, kc, 0);
		str = XKeysymToString(ksym);
		rfbLog("deleted keycode from X display: %03d 0x%x \"%s\"\n",
		    kc, ksym, str ? str : "null");
	}

	XFree_wr(keymap);
	XFlush_wr(dpy);
#endif	/* NO_X11 */
}

static int count_added_keycodes(void) {
	int kc, count = 0;
	for (kc = 0; kc < 0x100; kc++) {
		if (added_keysyms[kc] != NoSymbol) {
			count++;
		}
	}
	return count;
}

void delete_added_keycodes(int bequiet) {
	int kc;
	for (kc = 0; kc < 0x100; kc++) {
		if (added_keysyms[kc] != NoSymbol) {
			delete_keycode(kc, bequiet);
			added_keysyms[kc] = NoSymbol;
		}
	}
}

/*
 * The following is for an experimental -remap option to allow the user
 * to remap keystrokes.  It is currently confusing wrt modifiers...
 */
typedef struct keyremap {
	KeySym before;
	KeySym after;
	int isbutton;
	struct keyremap *next;
} keyremap_t;

static keyremap_t *keyremaps = NULL;

static void add_remap(char *line) {
	char str1[256], str2[256];
	KeySym ksym1, ksym2;
	int isbtn = 0;
	unsigned int i;
	static keyremap_t *current = NULL;
	keyremap_t *remap;

	if (sscanf(line, "%s %s", str1, str2) != 2) {
		rfbLogEnable(1);
		rfbLog("remap: invalid line: %s\n", line);
		clean_up_exit(1);
	}
	if (sscanf(str1, "0x%x", &i) == 1) {
		ksym1 = (KeySym) i;
	} else {
		ksym1 = XStringToKeysym(str1);
	}
	if (sscanf(str2, "0x%x", &i) == 1) {
		ksym2 = (KeySym) i;
	} else {
		ksym2 = XStringToKeysym(str2);
	}
	if (ksym2 == NoSymbol) {
		if (sscanf(str2, "Button%u", &i) == 1) {
			ksym2 = (KeySym) i;
			isbtn = 1; 
		}
	}
	if (ksym1 == NoSymbol || ksym2 == NoSymbol) {
		if (strcasecmp(str2, "NoSymbol") && strcasecmp(str2, "None")) {
			rfbLog("warning: skipping invalid remap line: %s", line);
			return;
		}
	}
	remap = (keyremap_t *) malloc((size_t) sizeof(keyremap_t));
	remap->before = ksym1;
	remap->after  = ksym2;
	remap->isbutton = isbtn;
	remap->next   = NULL;

	rfbLog("remapping: (%s, 0x%x) -> (%s, 0x%x) isbtn=%d\n", str1,
	    ksym1, str2, ksym2, isbtn);

	if (keyremaps == NULL) {
		keyremaps = remap;
	} else {
		current->next = remap;
	}
	current = remap;
}

static void add_dead_keysyms(char *str) {
	char *p, *q;
	int i;
	char *list[] = {
		"g grave dead_grave",
		"a acute dead_acute",
		"c asciicircum dead_circumflex",
		"t asciitilde dead_tilde",
		"m macron dead_macron",
		"b breve dead_breve",
		"D abovedot dead_abovedot",
		"d diaeresis dead_diaeresis",
		"o degree dead_abovering",
		"A doubleacute dead_doubleacute",
		"r caron dead_caron",
		"e cedilla dead_cedilla",
/*		"x XXX-ogonek dead_ogonek", */
/*		"x XXX-belowdot dead_belowdot", */
/*		"x XXX-hook dead_hook", */
/*		"x XXX-horn dead_horn", */
		NULL
	};

	p = str;

	while (*p != '\0') {
		if (isspace((unsigned char) (*p))) {
			*p = '\0';
		}
		p++;
	}

	if (!strcmp(str, "DEAD")) {
		for (i = 0; list[i] != NULL; i++) {
			p = list[i] + 2;
			add_remap(p);
		}
	} else if (!strcmp(str, "DEAD=missing")) {
		for (i = 0; list[i] != NULL; i++) {
			KeySym ksym, ksym2;
			int inmap = 0;

			p = strdup(list[i] + 2);
			q = strchr(p, ' ');
			if (q == NULL) {
				free(p);
				continue;
			}
			*q = '\0';
			ksym = XStringToKeysym(p);
			*q = ' ';
			if (ksym == NoSymbol) {
				free(p);
				continue;
			}
			if (XKeysymToKeycode(dpy, ksym)) {
				inmap = 1;
			}
#if LIBVNCSERVER_HAVE_XKEYBOARD
			if (! inmap && xkb_present && dpy) {
				int kc, grp, lvl;
				for (kc = 0; kc < 0x100; kc++) {
				    for (grp = 0; grp < 4; grp++) {
					for (lvl = 0; lvl < 8; lvl++) {
						ksym2 = XkbKeycodeToKeysym(dpy,
						    kc, grp, lvl);
						if (ksym2 == NoSymbol) {
							continue;
						}
						if (ksym2 == ksym) {
							inmap = 1;
							break;
						}
					}
				    }
				}
			}
#else
			if ((ksym2 = 0)) {}
#endif
			if (! inmap) {
				add_remap(p);
			}
			free(p);
		}
	} else if ((p = strchr(str, '=')) != NULL) {
		while (*p != '\0') {
			for (i = 0; list[i] != NULL; i++) {
				q = list[i];
				if (*p == *q) {
					q += 2;
					add_remap(q);
					break;
				}
			}
			p++;
		}
	}
}

/*
 * process the -remap string (file or mapping string)
 */
void initialize_remap(char *infile) {
	FILE *in;
	char *p, *q, line[256];

	if (keyremaps != NULL) {
		/* free last remapping */
		keyremap_t *next_remap, *curr_remap = keyremaps;
		while (curr_remap != NULL) {
			next_remap = curr_remap->next;
			free(curr_remap);
			curr_remap = next_remap;
		}
		keyremaps = NULL;
	}
	if (infile == NULL || *infile == '\0') {
		/* just unset remapping */
		return;
	}

	in = fopen(infile, "r"); 
	if (in == NULL) {
		/* assume cmd line key1-key2,key3-key4 */
		if (strstr(infile, "DEAD") == infile) {
			;
		} else if (!strchr(infile, '-')) {
			rfbLogEnable(1);
			rfbLog("remap: cannot open: %s\n", infile);
			rfbLogPerror("fopen");
			clean_up_exit(1);
		}
		if ((in = tmpfile()) == NULL) {
			rfbLogEnable(1);
			rfbLog("remap: cannot open tmpfile for %s\n", infile);
			rfbLogPerror("tmpfile");
			clean_up_exit(1);
		}

		/* copy in the string to file format */
		p = infile;
		while (*p) {
			if (*p == '-') {
				fprintf(in, " ");
			} else if (*p == ',' || *p == ' ' ||  *p == '\t') {
				fprintf(in, "\n");
			} else {
				fprintf(in, "%c", *p);
			}
			p++;
		}
		fprintf(in, "\n");
		fflush(in);	
		rewind(in);
	}

	while (fgets(line, 256, in) != NULL) {
		p = lblanks(line);
		if (*p == '\0') {
			continue;
		}
		if (strchr(line, '#')) {
			continue;
		}

		if (strstr(p, "DEAD") == p)  {
			add_dead_keysyms(p);
			continue;
		}
		if ((q = strchr(line, '-')) != NULL) {
			/* allow Keysym1-Keysym2 notation */
			*q = ' ';	
		}
		add_remap(p);
	}
	fclose(in);
}

/*
 * preliminary support for using the Xkb (XKEYBOARD) extension for handling
 * user input.  inelegant, slow, and incomplete currently... but initial
 * tests show it is useful for some setups.
 */
typedef struct keychar {
	KeyCode code;
	int group;
	int level;
} keychar_t;

/* max number of key groups and shift levels we consider */
#define GRP 4
#define LVL 8
static int lvl_max, grp_max, kc_min, kc_max;
static KeySym xkbkeysyms[0x100][GRP][LVL];
static unsigned int xkbstate[0x100][GRP][LVL];
static unsigned int xkbignore[0x100][GRP][LVL];
static unsigned int xkbmodifiers[0x100][GRP][LVL];
static int multi_key[0x100], mode_switch[0x100], skipkeycode[0x100];
static int shift_keys[0x100];

/*
 * for trying to order the keycodes to avoid problems, note the
 * *first* keycode bound to it.  kc_vec will be a permutation
 * of 1...256 to get them in the preferred order.
 */
static int kc_vec[0x100];
static int kc1_shift, kc1_control, kc1_caplock, kc1_alt;
static int kc1_meta, kc1_numlock, kc1_super, kc1_hyper;
static int kc1_mode_switch, kc1_iso_level3_shift, kc1_multi_key;
	
int sloppy_key_check(int key, rfbBool down, rfbKeySym keysym, int *new_kc) {
	if (!sloppy_keys) {
		return 0;
	}

	RAWFB_RET(0)
#if NO_X11
	if (!key || !down || !keysym || !new_kc) {}
	return 0;
#else
	
	if (!down && !keycode_state[key] && !IsModifierKey(keysym)) {
		int i, cnt = 0, downkey = -1;
		int nmods_down = track_mod_state(NoSymbol, FALSE, FALSE);
		int mods_down[256];

		if (nmods_down) {
			/* tracking to skip down modifier keycodes. */
			for(i=0; i<256; i++) {
				mods_down[i] = 0;
			}
			i = 0;
			while (simple_mods[i] != NoSymbol) {
				KeySym ksym = simple_mods[i];
				KeyCode k = XKeysymToKeycode(dpy, ksym);
				if (k != NoSymbol && keycode_state[(int) k]) {
					mods_down[(int) k] = 1;
				}
				
				i++;
			}
		}
		/*
		 * the keycode is already up... look for a single one
		 * (non modifier) that is down
		 */
		for (i=0; i<256; i++) {
			if (keycode_state[i]) {
				if (nmods_down && mods_down[i]) {
					continue;
				}
				cnt++;
				downkey = i;
			}
		}
		if (cnt == 1) {
			if (debug_keyboard) {
				fprintf(stderr, "    sloppy_keys: %d/0x%x "
				    "-> %d/0x%x  (nmods: %d)\n", (int) key,
				    (int) key, downkey, downkey, nmods_down);
			}
			*new_kc = downkey;
			return 1;
		}
	}
	return 0;
#endif	/* NO_X11 */
}

#if !LIBVNCSERVER_HAVE_XKEYBOARD || SKIP_XKB

/* empty functions for no xkb */
static void initialize_xkb_modtweak(void) {}
static void xkb_tweak_keyboard(rfbBool down, rfbKeySym keysym,
    rfbClientPtr client) {
	if (!client || !down || !keysym) {} /* unused vars warning: */
}
void switch_to_xkb_if_better(void) {}

#else

void switch_to_xkb_if_better(void) {
	KeySym keysym, *keymap;
	int miss_noxkb[256], miss_xkb[256], missing_noxkb = 0, missing_xkb = 0;
	int i, j, k, n, minkey, maxkey, syms_per_keycode;
	int syms_gt_4 = 0;
	int kc, grp, lvl;

	/* non-alphanumeric on us keyboard */
	KeySym must_have[] = {
		XK_exclam,
		XK_at,
		XK_numbersign,
		XK_dollar,
		XK_percent,
/*		XK_asciicircum, */
		XK_ampersand,
		XK_asterisk,
		XK_parenleft,
		XK_parenright,
		XK_underscore,
		XK_plus,
		XK_minus,
		XK_equal,
		XK_bracketleft,
		XK_bracketright,
		XK_braceleft,
		XK_braceright,
		XK_bar,
		XK_backslash,
		XK_semicolon,
/*		XK_apostrophe, */
		XK_colon,
		XK_quotedbl,
		XK_comma,
		XK_period,
		XK_less,
		XK_greater,
		XK_slash,
		XK_question,
/*		XK_asciitilde, */
/*		XK_grave, */
		NoSymbol
	};

	if (! use_modifier_tweak || got_noxkb) {
		return;
	}
	if (use_xkb_modtweak) {
		/* already using it */
		return;
	}
	RAWFB_RET_VOID

	XDisplayKeycodes(dpy, &minkey, &maxkey);

	keymap = XGetKeyboardMapping(dpy, minkey, (maxkey - minkey + 1),
	    &syms_per_keycode);

	/* handle alphabetic char with only one keysym (no upper + lower) */
	for (i = minkey; i <= maxkey; i++) {
		KeySym lower, upper;
		/* 2nd one */
		keysym = keymap[(i - minkey) * syms_per_keycode + 1];
		if (keysym != NoSymbol) {
			continue;
		}
		/* 1st one */
		keysym = keymap[(i - minkey) * syms_per_keycode + 0];
		if (keysym == NoSymbol) {
			continue;
		}
		XConvertCase(keysym, &lower, &upper);
		if (lower != upper) {
			keymap[(i - minkey) * syms_per_keycode + 0] = lower;
			keymap[(i - minkey) * syms_per_keycode + 1] = upper;
		}
	}

	k = 0;
	while (must_have[k] != NoSymbol) {
		int gotit = 0;
		KeySym must = must_have[k];
		for (i = minkey; i <= maxkey; i++) {
		    for (j = 0; j < syms_per_keycode; j++) {
			keysym = keymap[(i-minkey) * syms_per_keycode + j];
			if (j >= 4) {
			    if (k == 0 && keysym != NoSymbol) {
				/* for k=0 count the high keysyms */
				syms_gt_4++;
				if (debug_keyboard > 1) {
					char *str = XKeysymToString(keysym);
					fprintf(stderr, "- high keysym mapping"
					    ": at %3d j=%d "
					    "'%s'\n", i, j, str ? str : "null");
				}
			    }
			    continue;
			}
			if (keysym == must) {
				if (debug_keyboard > 1) {
					char *str = XKeysymToString(must);
					fprintf(stderr, "- at %3d j=%d found "
					    "'%s'\n", i, j, str ? str : "null");
				}
				/* n.b. do not break, see syms_gt_4 above. */
				gotit = 1;
			}
		    }
		}
		if (! gotit) {
			if (debug_keyboard > 1) {
				char *str = XKeysymToString(must);
				KeyCode kc = XKeysymToKeycode(dpy, must);
				fprintf(stderr, "- did not find 0x%lx '%s'\t"
				    "Ks2Kc: %d\n", must, str ? str:"null", kc); 
				if (kc != None) {
					int j2;
					for(j2=0; j2<syms_per_keycode; j2++) {
						keysym = keymap[(kc-minkey) *
						    syms_per_keycode + j2];
						fprintf(stderr, "  %d=0x%lx",
						    j2, keysym);
					}
					fprintf(stderr, "\n");
				}
			}
			missing_noxkb++;
			miss_noxkb[k] = 1;
		} else {
			miss_noxkb[k] = 0;
		}
		k++;
	}
	n = k;

	XFree_wr(keymap);
	if (missing_noxkb == 0 && syms_per_keycode > 4 && syms_gt_4 >= 0) {
		/* we used to have syms_gt_4 >= 8, now always on. */
		if (! raw_fb_str) {
			rfbLog("\n");
			rfbLog("XKEYBOARD: number of keysyms per keycode %d is greater\n", syms_per_keycode);
			rfbLog("  than 4 and %d keysyms are mapped above 4.\n", syms_gt_4);
			rfbLog("  Automatically switching to -xkb mode.\n");
			rfbLog("  If this makes the key mapping worse you can\n");
			rfbLog("  disable it with the \"-noxkb\" option.\n");
			rfbLog("  Also, remember \"-remap DEAD\" for accenting characters.\n");
			rfbLog("\n");
		}

		use_xkb_modtweak = 1;
		return;

	} else if (missing_noxkb == 0) {
		if (! raw_fb_str) {
			rfbLog("\n");
			rfbLog("XKEYBOARD: all %d \"must have\" keysyms accounted for.\n", n);
			rfbLog("  Not automatically switching to -xkb mode.\n");
			rfbLog("  If some keys still cannot be typed, try using -xkb.\n");
			rfbLog("  Also, remember \"-remap DEAD\" for accenting characters.\n");
			rfbLog("\n");
		}
		return;
	}

	for (k=0; k<n; k++) {
		miss_xkb[k] = 1;
	}

	for (kc = 0; kc < 0x100; kc++) {
	    for (grp = 0; grp < GRP; grp++) {
		for (lvl = 0; lvl < LVL; lvl++) {
			/* look up the Keysym, if any */
			keysym = XkbKeycodeToKeysym(dpy, kc, grp, lvl);
			if (keysym == NoSymbol) {
				continue;
			}
			for (k=0; k<n; k++) {
				if (keysym == must_have[k]) {
					miss_xkb[k] = 0;
				}
			}
		}
	    }
	}

	for (k=0; k<n; k++) {
		if (miss_xkb[k]) {
			missing_xkb++;
		}
	}

	rfbLog("\n");
	if (missing_xkb < missing_noxkb) {
		rfbLog("XKEYBOARD:\n");
		rfbLog("Switching to -xkb mode to recover these keysyms:\n");
	} else {
		rfbLog("XKEYBOARD: \"must have\" keysyms better accounted"
		    " for\n");
		rfbLog("under -noxkb mode: not switching to -xkb mode:\n");
	}

	rfbLog("   xkb  noxkb   Keysym  (\"X\" means present)\n");
	rfbLog("   ---  -----   -----------------------------\n");
	for (k=0; k<n; k++) {
		char *xx, *xn, *name;

		keysym = must_have[k];
		if (keysym == NoSymbol) {
			continue;
		}
		if (!miss_xkb[k] && !miss_noxkb[k]) {
			continue;
		}
		if (miss_xkb[k]) {
			xx = "   ";
		} else {
			xx = " X ";
		}
		if (miss_noxkb[k]) {
			xn = "   ";
		} else {
			xn = " X ";
		}
		name = XKeysymToString(keysym);
		rfbLog("   %s  %s     0x%lx  %s\n", xx, xn, keysym,
		    name ? name : "null");
	}
	rfbLog("\n");

	if (missing_xkb < missing_noxkb) {
		rfbLog("  If this makes the key mapping worse you can\n");
		rfbLog("  disable it with the \"-noxkb\" option.\n");
		rfbLog("\n");

		use_xkb_modtweak = 1;

	} else {
		rfbLog("  If some keys still cannot be typed, try using"
		    " -xkb.\n");
		rfbLog("  Also, remember \"-remap DEAD\" for accenting"
		    " characters.\n");
	}
	rfbLog("\n");
}

/* sets up all the keymapping info via Xkb API */

static void initialize_xkb_modtweak(void) {
	KeySym ks;
	int kc, grp, lvl, k;
	unsigned int state;

/*
 * Here is a guide:

Workarounds arrays:

multi_key[]     indicates which keycodes have Multi_key (Compose)
                bound to them.
mode_switch[]   indicates which keycodes have Mode_switch (AltGr)
                bound to them.
shift_keys[]    indicates which keycodes have Shift bound to them.
skipkeycode[]   indicates which keycodes are to be skipped
                for any lookups from -skip_keycodes option. 

Groups and Levels, here is an example:
                                                                  
      ^          --------                                      
      |      L2 | A   AE |                                      
    shift       |        |                                      
    level    L1 | a   ae |                                      
                 --------                                      
                  G1  G2                                        
                                                                
                  group ->                                      

Traditionally this it all a key could do.  L1 vs. L2 selected via Shift
and G1 vs. G2 selected via Mode_switch.  Up to 4 Keysyms could be bound
to a key.  See initialize_modtweak() for an example of using that type
of keymap from XGetKeyboardMapping().

Xkb gives us up to 4 groups and 63 shift levels per key, with the
situation being potentially different for each key.  This is complicated,
and I don't claim to understand it all, but in the following we just think
of ranging over the group and level indices as covering all of the cases.
This gives us an accurate view of the keymap.  The main tricky part
is mapping between group+level and modifier state.

On current linux/XFree86 setups (Xkb is enabled by default) the
information from XGetKeyboardMapping() (evidently the compat map)
is incomplete and inaccurate, so we are really forced to use the
Xkb API.

xkbkeysyms[]      For a (keycode,group,level) holds the KeySym (0 for none)
xkbstate[]        For a (keycode,group,level) holds the corresponding
                  modifier state needed to get that KeySym
xkbignore[]       For a (keycode,group,level) which modifiers can be
                  ignored (the 0 bits can be ignored).
xkbmodifiers[]    For the KeySym bound to this (keycode,group,level) store
                  the modifier mask.   
 *
 */

	RAWFB_RET_VOID

	/* initialize all the arrays: */
	for (kc = 0; kc < 0x100; kc++) {
		multi_key[kc] = 0;
		mode_switch[kc] = 0;
		skipkeycode[kc] = 0;
		shift_keys[kc] = 0;

		for (grp = 0; grp < GRP; grp++) {
			for (lvl = 0; lvl < LVL; lvl++) {
				xkbkeysyms[kc][grp][lvl] = NoSymbol;
				xkbmodifiers[kc][grp][lvl] = -1;
				xkbstate[kc][grp][lvl] = -1;
			}
		}
	}

	/*
	 * the array is 256*LVL*GRP, but we can make the searched region
	 * smaller by computing the actual ranges.
	 */
	lvl_max = 0;
	grp_max = 0;
	kc_max = 0;
	kc_min = 0x100;

	/* first keycode for a modifier type (multi_key too) */
	kc1_shift = -1;
	kc1_control = -1;
	kc1_caplock = -1;
	kc1_alt = -1;
	kc1_meta = -1;
	kc1_numlock = -1;
	kc1_super = -1;
	kc1_hyper = -1;
	kc1_mode_switch = -1;
	kc1_iso_level3_shift = -1;
	kc1_multi_key = -1;

	/*
	 * loop over all possible (keycode, group, level) triples
	 * and record what we find for it:
	 */
	if (debug_keyboard) {
		rfbLog("initialize_xkb_modtweak: XKB keycode -> keysyms "
		    "mapping info:\n");
	}
	for (kc = 0; kc < 0x100; kc++) {
	    for (grp = 0; grp < GRP; grp++) {
		for (lvl = 0; lvl < LVL; lvl++) {
			unsigned int ms, mods;
			int state_save = -1, mods_save = -1;
			KeySym ks2;

			/* look up the Keysym, if any */
			ks = XkbKeycodeToKeysym(dpy, kc, grp, lvl);
			xkbkeysyms[kc][grp][lvl] = ks;

			/* if no Keysym, on to next */
			if (ks == NoSymbol) {
				continue;
			}
			/*
			 * for various workarounds, note where these special
			 * keys are bound to.
			 */
			if (ks == XK_Multi_key) {
				multi_key[kc] = lvl+1;
			}
			if (ks == XK_Mode_switch) {
				mode_switch[kc] = lvl+1;
			}
			if (ks == XK_Shift_L || ks == XK_Shift_R) {
				shift_keys[kc] = lvl+1;
			}

			if (ks == XK_Shift_L || ks == XK_Shift_R) {
				if (kc1_shift == -1) {
					kc1_shift = kc;
				}
			}
			if (ks == XK_Control_L || ks == XK_Control_R) {
				if (kc1_control == -1) {
					kc1_control = kc;
				}
			}
			if (ks == XK_Caps_Lock || ks == XK_Caps_Lock) {
				if (kc1_caplock == -1) {
					kc1_caplock = kc;
				}
			}
			if (ks == XK_Alt_L || ks == XK_Alt_R) {
				if (kc1_alt == -1) {
					kc1_alt = kc;
				}
			}
			if (ks == XK_Meta_L || ks == XK_Meta_R) {
				if (kc1_meta == -1) {
					kc1_meta = kc;
				}
			}
			if (ks == XK_Num_Lock) {
				if (kc1_numlock == -1) {
					kc1_numlock = kc;
				}
			}
			if (ks == XK_Super_L || ks == XK_Super_R) {
				if (kc1_super == -1) {
					kc1_super = kc;
				}
			}
			if (ks == XK_Hyper_L || ks == XK_Hyper_R) {
				if (kc1_hyper == -1) {
					kc1_hyper = kc;
				}
			}
			if (ks == XK_Mode_switch) {
				if (kc1_mode_switch == -1) {
					kc1_mode_switch = kc;
				}
			}
			if (ks == XK_ISO_Level3_Shift) {
				if (kc1_iso_level3_shift == -1) {
					kc1_iso_level3_shift = kc;
				}
			}
			if (ks == XK_Multi_key) {	/* not a modifier.. */
				if (kc1_multi_key == -1) {
					kc1_multi_key = kc;
				}
			}

			/*
			 * record maximum extent for group/level indices
			 * and keycode range:
			 */
			if (grp > grp_max) {
				grp_max = grp;
			}
			if (lvl > lvl_max) {
				lvl_max = lvl;
			}
			if (kc > kc_max) {
				kc_max = kc;
			}
			if (kc < kc_min) {
				kc_min = kc;
			}

			/*
			 * lookup on *keysym* (i.e. not kc, grp, lvl)
			 * and get the modifier mask.  this is 0 for
			 * most keysyms, only non zero for modifiers.
			 */
			ms = XkbKeysymToModifiers(dpy, ks);
			xkbmodifiers[kc][grp][lvl] = ms;

			/*
			 * Amusing heuristic (may have bugs).  There are
			 * 8 modifier bits, so 256 possible modifier
			 * states.  We loop over all of them for this
			 * keycode (simulating Key "events") and ask
			 * XkbLookupKeySym to tell us the Keysym.  Once it
			 * matches the Keysym we have for this (keycode,
			 * group, level), gotten via XkbKeycodeToKeysym()
			 * above, we then (hopefully...) know that state
			 * of modifiers needed to generate this keysym.
			 *
			 * Yes... keep your fingers crossed.
			 *
			 * Note that many of the 256 states give the
			 * Keysym, we just need one, and we take the
			 * first one found.
			 */
			state = 0;
			while(state < 256) {
				if (XkbLookupKeySym(dpy, kc, state, &mods,
				    &ks2)) {

					/* save these for workaround below */
					if (state_save == -1) {
						state_save = state;
						mods_save = mods;
					}
					if (ks2 == ks) {
						/*
						 * zero the irrelevant bits
						 * by anding with mods.
						 */
						xkbstate[kc][grp][lvl]
						    = state & mods;
						/*
						 * also remember the irrelevant
						 * bits since it is handy.
						 */
						xkbignore[kc][grp][lvl] = mods;

						break;
					}
				}
				state++;
			}
			if (xkbstate[kc][grp][lvl] == (unsigned int) -1
			    && grp == 1) {
				/*
				 * Hack on Solaris 9 for Mode_switch
				 * for Group2 characters.  We force the 
				 * Mode_switch modifier bit on.
				 * XXX Need to figure out better what is
				 * happening here.  Is compat on somehow??
				 */
				unsigned int ms2;
				ms2 = XkbKeysymToModifiers(dpy, XK_Mode_switch);

				xkbstate[kc][grp][lvl]
				    = (state_save & mods_save) | ms2;

				xkbignore[kc][grp][lvl] = mods_save | ms2;
			}

			if (debug_keyboard) {
				char *str;
				fprintf(stderr, "  %03d  G%d L%d  mod=%s ",
				    kc, grp+1, lvl+1, bitprint(ms, 8));
				fprintf(stderr, "state=%s ",
				    bitprint(xkbstate[kc][grp][lvl], 8));
				fprintf(stderr, "ignore=%s ",
				    bitprint(xkbignore[kc][grp][lvl], 8));
				str = XKeysymToString(ks);
				fprintf(stderr, " ks=0x%08lx \"%s\"\n",
				    ks, str ? str : "null");
			}
		}
	    }
	}

	/*
	 * kc_vec will be used in some places to find modifiers, etc
	 * we apply some permutations to it as workarounds.
	 */
	for (kc = 0; kc < 0x100; kc++) {
		kc_vec[kc] = kc;
	}

	if (kc1_mode_switch != -1 && kc1_iso_level3_shift != -1) {
		if (kc1_mode_switch < kc1_iso_level3_shift) {
			/* we prefer XK_ISO_Level3_Shift: */
			kc_vec[kc1_mode_switch] = kc1_iso_level3_shift;
			kc_vec[kc1_iso_level3_shift] = kc1_mode_switch;
		}
	}
	/* any more? need to watch for undoing the above. */

	/*
	 * process the user supplied -skip_keycodes string.
	 * This is presumably a list if "ghost" keycodes, the X server
	 * thinks they exist, but they do not.  ghosts can lead to
	 * ambiguities in the reverse map: Keysym -> KeyCode + Modstate,
	 * so if we can ignore them so much the better.  Presumably the
	 * user can never generate them from the physical keyboard.
	 * There may be other reasons to deaden some keys.
	 */
	if (skip_keycodes != NULL) {
		char *p, *str = strdup(skip_keycodes);
		p = strtok(str, ", \t\n\r");
		while (p) {
			k = 1;
			if (sscanf(p, "%d", &k) != 1 || k < 0 || k >= 0x100) {
				rfbLogEnable(1);
				rfbLog("invalid skip_keycodes: %s %s\n",
				    skip_keycodes, p);
				clean_up_exit(1);
			}
			skipkeycode[k] = 1;
			p = strtok(NULL, ", \t\n\r");
		}
		free(str);
	}
	if (debug_keyboard) {
		fprintf(stderr, "grp_max=%d lvl_max=%d\n", grp_max, lvl_max);
	}
}

static short **score_hint = NULL;
/*
 * Called on user keyboard input.  Try to solve the reverse mapping
 * problem: KeySym (from VNC client) => KeyCode(s) to press to generate
 * it.  The one-to-many KeySym => KeyCode mapping makes it difficult, as
 * does working out what changes to the modifier keypresses are needed.
 */
static void xkb_tweak_keyboard(rfbBool down, rfbKeySym keysym,
    rfbClientPtr client) {

	int kc, grp, lvl, i, kci;
	int kc_f[0x100], grp_f[0x100], lvl_f[0x100], state_f[0x100], found;
	int ignore_f[0x100];
	unsigned int state = 0;


	/* these are used for finding modifiers, etc */
	XkbStateRec kbstate;
	int got_kbstate = 0;
	int Kc_f, Grp_f = 0, Lvl_f = 0;
#	define KLAST 10
	static int Kc_last_down[KLAST];
	static KeySym Ks_last_down[KLAST];
	static int klast = 0, khints = 1, anydown = 1;
	static int cnt = 0;

	if (!client || !down || !keysym) {} /* unused vars warning: */

	RAWFB_RET_VOID

	X_LOCK;

	if (klast == 0) {
		int i, j;
		for (i=0; i<KLAST; i++) {
			Kc_last_down[i] = -1;
			Ks_last_down[i] = NoSymbol;
		}
		if (getenv("NOKEYHINTS")) {
			khints = 0;
		}
		if (getenv("NOANYDOWN")) {
			anydown = 0;
		}
		if (getenv("KEYSDOWN")) {
			klast = atoi(getenv("KEYSDOWN"));
			if (klast < 1) klast = 1;
			if (klast > KLAST) klast = KLAST;
		} else {
			klast = 3;
		}
		if (khints && score_hint == NULL) {
			score_hint = (short **) malloc(0x100 * sizeof(short *));
			for (i=0; i<0x100; i++) {
				score_hint[i] = (short *) malloc(0x100 * sizeof(short));
			}
			
			for (i=0; i<0x100; i++) {
				for (j=0; j<0x100; j++) {
					score_hint[i][j] = -1;
				}
			}
		}
	}
	cnt++;
	if (cnt % 100 && khints && score_hint != NULL) {
		int i, j;
		for (i=0; i<0x100; i++) {
			for (j=0; j<0x100; j++) {
				score_hint[i][j] = -1;
			}
		}
	}

	if (debug_keyboard) {
		char *str = XKeysymToString(keysym);

		if (debug_keyboard > 1) {
			rfbLog("----------start-xkb_tweak_keyboard (%s) "
			    "--------\n", down ? "DOWN" : "UP");
		}

		rfbLog("xkb_tweak_keyboard: %s keysym=0x%x \"%s\"\n",
		    down ? "down" : "up", (int) keysym, str ? str : "null");
	}

	/*
	 * set everything to not-yet-found.
	 * these "found" arrays (*_f) let us dynamically consider the
	 * one-to-many Keysym -> Keycode issue.  we set the size at 256,
	 * but of course only very few will be found.
	 */
	for (i = 0; i < 0x100; i++) {
		kc_f[i]    = -1;
		grp_f[i]   = -1;
		lvl_f[i]   = -1;
		state_f[i] = -1;
		ignore_f[i] = -1;
	}
	found = 0;

	/*
	 * loop over all (keycode, group, level) triples looking for
	 * matching keysyms.  Amazingly this isn't slow (but maybe if
	 * you type really fast...).  Hash lookup into a linked list of
	 * (keycode,grp,lvl) triples would be the way to improve this
	 * in the future if needed.
	 */
	for (kc = kc_min; kc <= kc_max; kc++) {
	    for (grp = 0; grp < grp_max+1; grp++) {
		for (lvl = 0; lvl < lvl_max+1; lvl++) {
			if (keysym != xkbkeysyms[kc][grp][lvl]) {
				continue;
			}
			/* got a keysym match */
			state = xkbstate[kc][grp][lvl];

			if (debug_keyboard > 1) {
				char *s1, *s2;
				s1 = XKeysymToString(XKeycodeToKeysym(dpy,
				    kc, 0));
				if (! s1) s1 = "null";
				s2 = XKeysymToString(keysym);
				if (! s2) s2 = "null";
				fprintf(stderr, "  got match kc=%03d=0x%02x G%d"
				    " L%d  ks=0x%x \"%s\"  (basesym: \"%s\")\n",
				    kc, kc, grp+1, lvl+1, keysym, s2, s1);
				fprintf(stderr, "    need state: %s\n",
				    bitprint(state, 8));
				fprintf(stderr, "    ignorable : %s\n",
				    bitprint(xkbignore[kc][grp][lvl], 8));
			}

			/* save it if state is OK and not told to skip */
			if (state == (unsigned int) -1) {
				continue;
			}
			if (skipkeycode[kc] && debug_keyboard) {
				fprintf(stderr, "    xxx skipping keycode: %d "
				   "G%d/L%d\n", kc, grp+1, lvl+1);
			}
			if (skipkeycode[kc]) {
				continue;
			}
			if (found > 0 && kc == kc_f[found-1]) {
				/* ignore repeats for same keycode */
				continue;
			}
			kc_f[found] = kc;
			grp_f[found] = grp;
			lvl_f[found] = lvl;
			state_f[found] = state;
			ignore_f[found] = xkbignore[kc][grp][lvl];
			found++;
		}
	    }
	}

#define PKBSTATE  \
	fprintf(stderr, "    --- current mod state:\n"); \
	fprintf(stderr, "    mods      : %s\n", bitprint(kbstate.mods, 8)); \
	fprintf(stderr, "    base_mods : %s\n", bitprint(kbstate.base_mods, 8)); \
	fprintf(stderr, "    latch_mods: %s\n", bitprint(kbstate.latched_mods, 8)); \
	fprintf(stderr, "    lock_mods : %s\n", bitprint(kbstate.locked_mods, 8)); \
	fprintf(stderr, "    compat    : %s\n", bitprint(kbstate.compat_state, 8));

	/*
	 * Now get the current state of the keyboard from the X server.
	 * This seems to be the safest way to go as opposed to our
	 * keeping track of the modifier state on our own.  Again,
	 * this is fortunately not too slow.
	 */

	if (debug_keyboard > 1) {
		/* get state early for debug output */
		XkbGetState(dpy, XkbUseCoreKbd, &kbstate);
		got_kbstate = 1;
		PKBSTATE
	}

	if (!found && add_keysyms && keysym && ! IsModifierKey(keysym)) {
		int new_kc = add_keysym(keysym);
		if (new_kc != 0) {
			found = 1;
			kc_f[0] = new_kc;
			grp_f[0] = 0; 
			lvl_f[0] = 0; 
			state_f[0] = 0;
		}
	}

	if (!found && debug_keyboard) {
		char *str = XKeysymToString(keysym);
		fprintf(stderr, "    *** NO key found for: 0x%x %s  "
		    "*keystroke ignored*\n", keysym, str ? str : "null");
	}
	if (!found) {
		X_UNLOCK;
		return;
	}

	/* 
	 * we try to optimize here if found > 1
	 * e.g. minimize lvl or grp, or other things to give
	 * "safest" scenario to simulate the keystrokes.
	 */

	if (found > 1) {
		if (down) {
			int l, score[0x100];
			int best = 0, best_score = -1;
			/* need to break the tie... */
			if (! got_kbstate) {
				XkbGetState(dpy, XkbUseCoreKbd, &kbstate);
				got_kbstate = 1;
			}
			if (khints && keysym < 0x100) {
				int ks = (int) keysym, j;
				for (j=0; j< 0x100; j++) {
					score_hint[ks][j] = -1;
				}
			}
			for (l=0; l < found; l++) {
				int myscore = 0, b = 0x1, i;
				int curr, curr_state = kbstate.mods;
				int need, need_state = state_f[l];
				int ignore_state = ignore_f[l];

				/* see how many modifiers need to be changed */
				for (i=0; i<8; i++) {
					curr = b & curr_state;
					need = b & need_state;
					if (! (b & ignore_state)) {
						;
					} else if (curr == need) {
						;
					} else {
						myscore++;
					}
					b = b << 1;
				}
				myscore *= 100;

				/* throw in some minimization of lvl too: */
				myscore += 2*lvl_f[l] + grp_f[l];

				/*
				 * XXX since we now internally track
				 * keycode_state[], we could throw that into
				 * the score as well.  I.e. if it is already
				 * down, it is pointless to think we can
				 * press it down further!  E.g.
				 *   myscore += 1000 * keycode_state[kc_f[l]];
				 * Also could watch multiple modifier
				 * problem, e.g. Shift+key -> Alt
				 * keycode = 125 on my keyboard.
				 */

				score[l] = myscore;
				if (debug_keyboard > 1) {
					fprintf(stderr, "    *** score for "
					    "keycode %03d: %4d\n",
					    kc_f[l], myscore);
				}
				if (khints && keysym < 0x100 && kc_f[l] < 0x100) {
					score_hint[(int) keysym][kc_f[l]] = (short) score[l];
				}
			}
			for (l=0; l < found; l++) {
				int myscore = score[l];
				if (best_score == -1 || myscore < best_score) {
					best = l;
					best_score = myscore;
				}
			}
			Kc_f = kc_f[best];
			Grp_f = grp_f[best];
			Lvl_f = lvl_f[best];
			state = state_f[best];
			
		} else {
			/* up */
			int i, Kc_loc = -1;
			Kc_f = -1;

			/* first try the scores we remembered when the key went down: */
			if (khints && keysym < 0x100) {
				/* low keysyms, ascii, only */
				int ks = (int) keysym;
				int ok = 1, lbest = 0, l;
				short sbest = -1;
				for (l=0; l < found; l++) {
					if (kc_f[l] < 0x100) {
						int key = (int) kc_f[l];
						if (! keycode_state[key]) {
							continue;
						}
						if (score_hint[ks][key] < 0) {
							ok = 0;
							break;
						}
						if (sbest < 0 || score_hint[ks][key] < sbest) {
							sbest = score_hint[ks][key];
							lbest = l;
						}
					} else {
						ok = 0;
						break;
					}
				}
				if (ok && sbest != -1) {
					Kc_f = kc_f[lbest];
				}
				if (debug_keyboard && Kc_f != -1) {
					fprintf(stderr, "    UP: found via score_hint, s/l=%d/%d\n",
					    sbest, lbest);
				}
			}

			/* next look at our list of recently pressed down keys */
			if (Kc_f == -1) {
				for (i=klast-1; i>=0; i--) {
					/*
					 * some people type really fast and leave
					 * lots of keys down before releasing
					 * them.  this gives problem on weird
					 * qwerty+dvorak keymappings where each
					 * alpha character is on TWO keys.
					 */
					if (keysym == Ks_last_down[i]) {
						int l;
						for (l=0; l < found; l++) {
							if (Kc_last_down[i] == kc_f[l]) {
								int key = (int) kc_f[l];
								if (keycode_state[key]) {
									Kc_f = Kc_last_down[i];
									Kc_loc = i;
									break;
								}
							}
						}
					}
					if (Kc_f != -1) {
						break;
					}
				}
				if (debug_keyboard && Kc_f != -1) {
					fprintf(stderr, "    UP: found via klast, i=%d\n", Kc_loc);
				}
			}

			/* next just check for "best" one that is down */
			if (Kc_f == -1 && anydown) {
				int l;
				int best = -1, lbest = 0;
				/*
				 * If it is already down, that is
				 * a great hint.  Use it.
				 *
				 * note: keycode_state is internal and
				 * ignores someone pressing keys on the
				 * physical display (but is updated
				 * periodically to clean out stale info).
				 */
				for (l=0; l < found; l++) {
					int key = (int) kc_f[l];
					int j, jmatch = -1;

					if (! keycode_state[key]) {
						continue;
					}
					/* break ties based on lowest XKeycodeToKeysym index */
					for (j=0; j<8; j++) {
						KeySym ks = XKeycodeToKeysym(dpy, kc_f[l], j);
						if (ks != NoSymbol && ks == keysym) {
							jmatch = j;
							break;
						}
					}
					if (jmatch == -1) {
						continue;
					}
					if (best == -1 || jmatch < best) {
						best = jmatch;
						lbest = l;
					}
				}
				if (best != -1) {
					Kc_f = kc_f[lbest];
				}
				if (debug_keyboard && Kc_f != -1) {
					fprintf(stderr, "    UP: found via downlist, l=%d\n", lbest);
				}
			}

			/* next, use the first one found that is down */
			if (Kc_f == -1) {
				int l;
				for (l=0; l < found; l++) {
					int key = (int) kc_f[l];
					if (keycode_state[key]) {
						Kc_f = kc_f[l];
						break;
					}
				}
				if (debug_keyboard && Kc_f != -1) {
					fprintf(stderr, "    UP: set to first one down, kc_f[%d]!!\n", l);
				}
			}

			/* last, use the first one found */
			if (Kc_f == -1) {
				/* hope for the best... XXX check mods */
				Kc_f = kc_f[0];
				if (debug_keyboard && Kc_f != -1) {
					fprintf(stderr, "    UP: set to first one at all, kc_f[0]!!\n");
				}
			}
		}
	} else {
		Kc_f = kc_f[0];
		Grp_f = grp_f[0];
		Lvl_f = lvl_f[0];
		state = state_f[0];
	}

	if (debug_keyboard && found > 1) {
		int l;
		char *str;
		fprintf(stderr, "    *** found more than one keycode: ");
		for (l = 0; l < found; l++) {
			fprintf(stderr, "%03d ", kc_f[l]);
		}
		for (l = 0; l < found; l++) {
			str = XKeysymToString(XKeycodeToKeysym(dpy,kc_f[l],0));
			fprintf(stderr, " \"%s\"", str ? str : "null");
		}
		fprintf(stderr, ", picked this one: %03d  (last down: %03d)\n",
		    Kc_f, Kc_last_down[0]);
	}

	if (sloppy_keys) {
		int new_kc;
		if (sloppy_key_check(Kc_f, down, keysym, &new_kc)) {
			Kc_f = new_kc;
		}
	}

	if (down) {
		/*
		 * need to set up the mods for tweaking and other workarounds
		 */
		int needmods[8], sentmods[8], Ilist[8], keystate[256];
		int involves_multi_key, shift_is_down;
		int i, j, b, curr, need;
		unsigned int ms;
		KeySym ks;
		Bool dn;

		/* remember these to aid the subsequent up case: */
		for (i=KLAST-1; i >= 1; i--) {
			Ks_last_down[i] = Ks_last_down[i-1];
			Kc_last_down[i] = Kc_last_down[i-1];
		}
		Ks_last_down[0] = keysym;
		Kc_last_down[0] = Kc_f;

		if (! got_kbstate) {
			/* get the current modifier state if we haven't yet */
			XkbGetState(dpy, XkbUseCoreKbd, &kbstate);
			got_kbstate = 1;
		}

		/*
		 * needmods[] whether or not that modifier bit needs
		 *            something done to it. 
		 *            < 0 means no,
		 *            0   means needs to go up.
		 *            1   means needs to go down.
		 *
		 * -1, -2, -3 are used for debugging info to indicate
		 * why nothing needs to be done with the modifier, see below.
		 *
		 * sentmods[] is the corresponding keycode to use
		 * to achieve the needmods[] requirement for the bit.
		 */

		for (i=0; i<8; i++) {
			needmods[i] = -1;
			sentmods[i] = 0;
		}

		/*
		 * Loop over the 8 modifier bits and check if the current
		 * setting is what we need it to be or whether it should
		 * be changed (by us sending some keycode event)
		 *
		 * If nothing needs to be done to it record why:
		 *   -1  the modifier bit is ignored.
		 *   -2  the modifier bit is ignored, but is correct anyway.
		 *   -3  the modifier bit is correct.
		 */

		b = 0x1;
		for (i=0; i<8; i++) {
			curr = b & kbstate.mods;
			need = b & state;

			if (! (b & xkbignore[Kc_f][Grp_f][Lvl_f])) {
				/* irrelevant modifier bit */
				needmods[i] = -1;
				if (curr == need) needmods[i] = -2;
			} else if (curr == need) {
				/* already correct */
				needmods[i] = -3;
			} else if (! curr && need) {
				/* need it down */
				needmods[i] = 1;
			} else if (curr && ! need) {
				/* need it up */
				needmods[i] = 0;
			}

			b = b << 1;
		}

		/*
		 * Again we dynamically probe the X server for information,
		 * this time for the state of all the keycodes.  Useful
		 * info, and evidently is not too slow...
		 */
		get_keystate(keystate);

		/*
		 * We try to determine if Shift is down (since that can
		 * screw up ISO_Level3_Shift manipulations).
		 */
		shift_is_down = 0;

		for (kc = kc_min; kc <= kc_max; kc++) {
			if (skipkeycode[kc] && debug_keyboard) {
				fprintf(stderr, "    xxx skipping keycode: "
				    "%d\n", kc);
			}
			if (skipkeycode[kc]) {
				continue;
			}
			if (shift_keys[kc] && keystate[kc]) {
				shift_is_down = kc;
				break;
			}
		}

		/*
		 * Now loop over the modifier bits and try to deduce the
		 * keycode presses/release require to match the desired
		 * state.
		 */
		for (i=0; i<8; i++) {
			if (needmods[i] < 0 && debug_keyboard > 1) {
				int k = -needmods[i] - 1;
				char *words[] = {"ignorable",
				    "bitset+ignorable", "bitset"};
				fprintf(stderr, "    +++ needmods: mod=%d is "
				    "OK  (%s)\n", i, words[k]);
			}
			if (needmods[i] < 0) {
				continue;
			}

			b = 1 << i;

			if (debug_keyboard > 1) {
				fprintf(stderr, "    +++ needmods: mod=%d %s "
				    "need it to be: %d %s\n", i, bitprint(b, 8),
				    needmods[i], needmods[i] ? "down" : "up");
			}

			/*
			 * Again, an inefficient loop, this time just
			 * looking for modifiers...
			 * 
			 * note the use of kc_vec to prefer XK_ISO_Level3_Shift
			 * over XK_Mode_switch.
			 */
			for (kci = kc_min; kci <= kc_max; kci++) {
			  for (grp = 0; grp < grp_max+1; grp++) {
			    for (lvl = 0; lvl < lvl_max+1; lvl++) {
				int skip = 1, dbmsg = 0;

				kc = kc_vec[kci];

				ms = xkbmodifiers[kc][grp][lvl];
				if (! ms || ms != (unsigned int) b) {
					continue;
				}

				if (skipkeycode[kc] && debug_keyboard) {
				    fprintf(stderr, "    xxx skipping keycode:"
					" %d G%d/L%d\n", kc, grp+1, lvl+1);
				}
				if (skipkeycode[kc]) {
					continue;
				}

				ks = xkbkeysyms[kc][grp][lvl];
				if (! ks) {
					continue;
				}

				if (ks == XK_Shift_L) {
					skip = 0;
				} else if (ks == XK_Shift_R) {
					skip = 0;
				} else if (ks == XK_Mode_switch) {
					skip = 0;
				} else if (ks == XK_ISO_Level3_Shift) {
					skip = 0;
				}

				if (watch_capslock && kbstate.locked_mods & LockMask) {
				    if (keysym >= 'A' && keysym <= 'Z') {
					if (ks == XK_Shift_L || ks == XK_Shift_R) {
						if (debug_keyboard > 1) {
							fprintf(stderr, "    A-Z caplock skip Shift\n");
						}
						skip = 1;
					} else if (ks == XK_Caps_Lock) {
						if (debug_keyboard > 1) {
							fprintf(stderr, "    A-Z caplock noskip CapsLock\n");
						}
						skip = 0;
					}
				    }
				}
				/*
				 * Alt, Meta, Control, Super,
				 * Hyper, Num, Caps are skipped.
				 *
				 * XXX need more work on Locks,
				 * and non-standard modifiers.
				 * (e.g. XF86_Next_VMode using
				 * Ctrl+Alt)
				 */
				if (debug_keyboard > 1) {
					char *str = XKeysymToString(ks);
					int kt = keystate[kc];
					fprintf(stderr, "    === for mod=%s "
					    "found kc=%03d/G%d/L%d it is %d "
					    "%s skip=%d (%s)\n", bitprint(b,8),
					    kc, grp+1, lvl+1, kt, kt ?
					    "down" : "up  ", skip, str ?
					    str : "null");
				}

				if (! skip && needmods[i] !=
				    keystate[kc] && sentmods[i] == 0) {
					sentmods[i] = kc;
					dbmsg = 1;
				}

				if (debug_keyboard > 1 && dbmsg) {
					int nm = needmods[i];
					fprintf(stderr, "    >>> we choose "
					    "kc=%03d=0x%02x to change it to: "
					    "%d %s\n", kc, kc, nm, nm ?
					    "down" : "up");
				}
					
			    }
			  }
			}
		}
		for (i=0; i<8; i++) {
			/*
			 * reverse order is useful for tweaking
			 * ISO_Level3_Shift before Shift, but assumes they
			 * are in that order (i.e. Shift is first bit).
			 */
			int reverse = 1;
			if (reverse) {
				Ilist[i] = 7 - i;
			} else {
				Ilist[i] = i;
			}
		}

		/*
		 * check to see if Multi_key is bound to one of the Mods
		 * we have to tweak
		 */
		involves_multi_key = 0;
		for (j=0; j<8; j++) {
			i = Ilist[j];
			if (sentmods[i] == 0) continue;
			dn = (Bool) needmods[i];
			if (!dn) continue;
			if (multi_key[sentmods[i]]) {
				involves_multi_key = i+1;
			}
		}

		if (involves_multi_key && shift_is_down && needmods[0] < 0) {
			/*
			 * Workaround for Multi_key and shift.
			 * Assumes Shift is bit 1 (needmods[0])
			 */
			if (debug_keyboard) {
				fprintf(stderr, "    ^^^ trying to avoid "
				    "inadvertent Multi_key from Shift "
				    "(doing %03d up now)\n", shift_is_down);
			}
			XTestFakeKeyEvent_wr(dpy, shift_is_down, False,
			    CurrentTime);
		} else {
			involves_multi_key = 0;
		}

		for (j=0; j<8; j++) {
			/* do the Mod ups */
			i = Ilist[j];
			if (sentmods[i] == 0) continue;
			dn = (Bool) needmods[i];
			if (dn) continue;
			XTestFakeKeyEvent_wr(dpy, sentmods[i], dn, CurrentTime);
		}
		for (j=0; j<8; j++) {
			/* next, do the Mod downs */
			i = Ilist[j];
			if (sentmods[i] == 0) continue;
			dn = (Bool) needmods[i];
			if (!dn) continue;
			XTestFakeKeyEvent_wr(dpy, sentmods[i], dn, CurrentTime);
		}

		if (involves_multi_key) {
			/*
			 * Reverse workaround for Multi_key and shift.
			 */
			if (debug_keyboard) {
				fprintf(stderr, "    vvv trying to avoid "
				    "inadvertent Multi_key from Shift "
				    "(doing %03d down now)\n", shift_is_down);
			}
			XTestFakeKeyEvent_wr(dpy, shift_is_down, True,
			    CurrentTime);
		}

		/*
		 * With the above modifier work done, send the actual keycode:
		 */
		XTestFakeKeyEvent_wr(dpy, Kc_f, (Bool) down, CurrentTime);

		/*
		 * Now undo the modifier work:
		 */
		for (j=7; j>=0; j--) {
			/* reverse Mod downs we did */
			i = Ilist[j];
			if (sentmods[i] == 0) continue;
			dn = (Bool) needmods[i];
			if (!dn) continue;
			XTestFakeKeyEvent_wr(dpy, sentmods[i], !dn,
			    CurrentTime);
		}
		for (j=7; j>=0; j--) {
			/* finally reverse the Mod ups we did */
			i = Ilist[j];
			if (sentmods[i] == 0) continue;
			dn = (Bool) needmods[i];
			if (dn) continue;
			XTestFakeKeyEvent_wr(dpy, sentmods[i], !dn,
			    CurrentTime);
		}

	} else { /* for up case, hopefully just need to pop it up: */

		XTestFakeKeyEvent_wr(dpy, Kc_f, (Bool) down, CurrentTime);
	}
	X_UNLOCK;
}
#endif

/*
 * For tweaking modifiers wrt the Alt-Graph key, etc.
 */
#define LEFTSHIFT 1
#define RIGHTSHIFT 2
#define ALTGR 4
static char mod_state = 0;

static char modifiers[0x100];
static KeyCode keycodes[0x100];
static KeyCode left_shift_code, right_shift_code, altgr_code, iso_level3_code;

/* workaround for X11R5, Latin 1 only */
#ifndef XConvertCase
#define XConvertCase(sym, lower, upper) \
*(lower) = sym; \
*(upper) = sym; \
if (sym >> 8 == 0) { \
    if ((sym >= XK_A) && (sym <= XK_Z)) \
        *(lower) += (XK_a - XK_A); \
    else if ((sym >= XK_a) && (sym <= XK_z)) \
        *(upper) -= (XK_a - XK_A); \
    else if ((sym >= XK_Agrave) && (sym <= XK_Odiaeresis)) \
        *(lower) += (XK_agrave - XK_Agrave); \
    else if ((sym >= XK_agrave) && (sym <= XK_odiaeresis)) \
        *(upper) -= (XK_agrave - XK_Agrave); \
    else if ((sym >= XK_Ooblique) && (sym <= XK_Thorn)) \
        *(lower) += (XK_oslash - XK_Ooblique); \
    else if ((sym >= XK_oslash) && (sym <= XK_thorn)) \
        *(upper) -= (XK_oslash - XK_Ooblique); \
}
#endif

char *short_kmbcf(char *str) {
	int i, saw_k = 0, saw_m = 0, saw_b = 0, saw_c = 0, saw_f = 0, n = 10;
	char *p, tmp[10];
	
	for (i=0; i<n; i++) {
		tmp[i] = '\0';
	}

	p = str;
	i = 0;
	while (*p) {
		if ((*p == 'K' || *p == 'k') && !saw_k) {
			tmp[i++] = 'K';
			saw_k = 1;
		} else if ((*p == 'M' || *p == 'm') && !saw_m) {
			tmp[i++] = 'M';
			saw_m = 1;
		} else if ((*p == 'B' || *p == 'b') && !saw_b) {
			tmp[i++] = 'B';
			saw_b = 1;
		} else if ((*p == 'C' || *p == 'c') && !saw_c) {
			tmp[i++] = 'C';
			saw_c = 1;
		} else if ((*p == 'F' || *p == 'f') && !saw_f) {
			tmp[i++] = 'F';
			saw_f = 1;
		}
		p++;
	}
	return(strdup(tmp));
}

void initialize_allowed_input(void) {
	char *str;

	if (allowed_input_normal) {
		free(allowed_input_normal);
		allowed_input_normal = NULL;
	}
	if (allowed_input_view_only) {
		free(allowed_input_view_only);
		allowed_input_view_only = NULL;
	}

	if (! allowed_input_str) {
		allowed_input_normal = strdup("KMBCF");
		allowed_input_view_only = strdup("");
	} else {
		char *p, *str = strdup(allowed_input_str);
		p = strchr(str, ',');
		if (p) {
			allowed_input_view_only = strdup(p+1);
			*p = '\0';
			allowed_input_normal = strdup(str);
		} else {
			allowed_input_normal = strdup(str);
			allowed_input_view_only = strdup("");
		}
		free(str);
	}

	/* shorten them */
	str = short_kmbcf(allowed_input_normal);
	free(allowed_input_normal);
	allowed_input_normal = str;

	str = short_kmbcf(allowed_input_view_only);
	free(allowed_input_view_only);
	allowed_input_view_only = str;

	if (screen) {
		rfbClientIteratorPtr iter;
		rfbClientPtr cl;

		iter = rfbGetClientIterator(screen);
		while( (cl = rfbClientIteratorNext(iter)) ) {
			ClientData *cd = (ClientData *) cl->clientData;

			if (! cd) {
				continue;
			}
#if 0
rfbLog("cd: %p\n", cd);
rfbLog("cd->input: %s\n", cd->input);
rfbLog("cd->login_viewonly: %d\n", cd->login_viewonly);
rfbLog("allowed_input_view_only: %s\n", allowed_input_view_only);
#endif

			if (cd->input[0] == '=') {
				;	/* custom setting */
			} else if (cd->login_viewonly) {
				if (*allowed_input_view_only != '\0') {
					cl->viewOnly = FALSE;
					cd->input[0] = '\0';
					strncpy(cd->input,
					    allowed_input_view_only, CILEN);
				} else {
					cl->viewOnly = TRUE;
				}
			} else {
				if (allowed_input_normal) {
					cd->input[0] = '\0';
					strncpy(cd->input,
					    allowed_input_normal, CILEN);
				}
			}
		}
		rfbReleaseClientIterator(iter);
	}
}

void initialize_modtweak(void) {
#if NO_X11
	RAWFB_RET_VOID
	return;
#else
	KeySym keysym, *keymap;
	int i, j, minkey, maxkey, syms_per_keycode;
	int use_lowest_index = 0;

	if (use_xkb_modtweak) {
		initialize_xkb_modtweak();
		return;
	}
	memset(modifiers, -1, sizeof(modifiers));
	for (i=0; i<0x100; i++) {
		keycodes[i] = NoSymbol;
	}

	RAWFB_RET_VOID

	if (getenv("MODTWEAK_LOWEST")) {
		use_lowest_index = 1;
	}

	X_LOCK;
	XDisplayKeycodes(dpy, &minkey, &maxkey);

	keymap = XGetKeyboardMapping(dpy, minkey, (maxkey - minkey + 1),
	    &syms_per_keycode);

	/* handle alphabetic char with only one keysym (no upper + lower) */
	for (i = minkey; i <= maxkey; i++) {
		KeySym lower, upper;
		/* 2nd one */
		keysym = keymap[(i - minkey) * syms_per_keycode + 1];
		if (keysym != NoSymbol) {
			continue;
		}
		/* 1st one */
		keysym = keymap[(i - minkey) * syms_per_keycode + 0];
		if (keysym == NoSymbol) {
			continue;
		}
		XConvertCase(keysym, &lower, &upper);
		if (lower != upper) {
			keymap[(i - minkey) * syms_per_keycode + 0] = lower;
			keymap[(i - minkey) * syms_per_keycode + 1] = upper;
		}
	}
	for (i = minkey; i <= maxkey; i++) {
		if (debug_keyboard) {
			if (i == minkey) {
				rfbLog("initialize_modtweak: keycode -> "
				    "keysyms mapping info:\n");
			}
			fprintf(stderr, "  %03d  ", i);
		}
		for (j = 0; j < syms_per_keycode; j++) {
			if (debug_keyboard) {
				char *sym;
#if 0
				sym =XKeysymToString(XKeycodeToKeysym(dpy,i,j));
#else
				keysym = keymap[(i-minkey)*syms_per_keycode+j];
				sym = XKeysymToString(keysym);
#endif
				fprintf(stderr, "%-18s ", sym ? sym : "null");
				if (j == syms_per_keycode - 1) {
					fprintf(stderr, "\n");
				}
			}
			if (j >= 4) {
				/*
				 * Something wacky in the keymapping.
				 * Ignore these non Shift/AltGr chords
				 * for now... n.b. we try to automatically
				 * switch to -xkb for this case.
				 */
				continue;
			}
			keysym = keymap[ (i - minkey) * syms_per_keycode + j ];
			if ( keysym >= ' ' && keysym < 0x100
			    && i == XKeysymToKeycode(dpy, keysym) ) {
				if (use_lowest_index && keycodes[keysym] != NoSymbol) {
					continue;
				}
				keycodes[keysym] = i;
				modifiers[keysym] = j;
			}
		}
	}

	left_shift_code = XKeysymToKeycode(dpy, XK_Shift_L);
	right_shift_code = XKeysymToKeycode(dpy, XK_Shift_R);
	altgr_code = XKeysymToKeycode(dpy, XK_Mode_switch);
	iso_level3_code = NoSymbol;
#ifdef XK_ISO_Level3_Shift
	iso_level3_code = XKeysymToKeycode(dpy, XK_ISO_Level3_Shift);
#endif

	XFree_wr ((void *) keymap);

	X_UNLOCK;
#endif	/* NO_X11 */
}

/*
 * does the actual tweak:
 */
static void tweak_mod(signed char mod, rfbBool down) {
	rfbBool is_shift = mod_state & (LEFTSHIFT|RIGHTSHIFT);
	Bool dn = (Bool) down;
	KeyCode altgr = altgr_code;

	RAWFB_RET_VOID

	if (mod < 0) {
		if (debug_keyboard) {
			rfbLog("tweak_mod: Skip:  down=%d index=%d\n", down,
			    (int) mod);
		}
		return;
	}
	if (debug_keyboard) {
		rfbLog("tweak_mod: Start:  down=%d index=%d mod_state=0x%x"
		    " is_shift=%d\n", down, (int) mod, (int) mod_state,
		    is_shift);
	}

	if (use_iso_level3 && iso_level3_code) {
		altgr = iso_level3_code;
	}

	X_LOCK;
	if (is_shift && mod != 1) {
	    if (mod_state & LEFTSHIFT) {
		XTestFakeKeyEvent_wr(dpy, left_shift_code, !dn, CurrentTime);
	    }
	    if (mod_state & RIGHTSHIFT) {
		XTestFakeKeyEvent_wr(dpy, right_shift_code, !dn, CurrentTime);
	    }
	}
	if ( ! is_shift && mod == 1 ) {
	    XTestFakeKeyEvent_wr(dpy, left_shift_code, dn, CurrentTime);
	}
	if ( altgr && (mod_state & ALTGR) && mod != 2 ) {
	    XTestFakeKeyEvent_wr(dpy, altgr, !dn, CurrentTime);
	}
	if ( altgr && ! (mod_state & ALTGR) && mod == 2 ) {
	    XTestFakeKeyEvent_wr(dpy, altgr, dn, CurrentTime);
	}
	X_UNLOCK;

	if (debug_keyboard) {
		rfbLog("tweak_mod: Finish: down=%d index=%d mod_state=0x%x"
		    " is_shift=%d\n", down, (int) mod, (int) mod_state,
		    is_shift);
	}
}

/*
 * tweak the modifier under -modtweak
 */
static void modifier_tweak_keyboard(rfbBool down, rfbKeySym keysym,
    rfbClientPtr client) {
#if NO_X11
	RAWFB_RET_VOID
	if (!down || !keysym || !client) {}
	return;
#else
	KeyCode k;
	int tweak = 0;

	RAWFB_RET_VOID

	if (use_xkb_modtweak) {
		xkb_tweak_keyboard(down, keysym, client);
		return;
	}
	if (debug_keyboard) {
		rfbLog("modifier_tweak_keyboard: %s keysym=0x%x\n",
		    down ? "down" : "up", (int) keysym);
	}

#define ADJUSTMOD(sym, state) \
	if (keysym == sym) { \
		if (down) { \
			mod_state |= state; \
		} else { \
			mod_state &= ~state; \
		} \
	}

	ADJUSTMOD(XK_Shift_L, LEFTSHIFT)
	ADJUSTMOD(XK_Shift_R, RIGHTSHIFT)
	ADJUSTMOD(XK_Mode_switch, ALTGR)

	if ( down && keysym >= ' ' && keysym < 0x100 ) {
		unsigned int state = 0;
		tweak = 1;
		if (watch_capslock && keysym >= 'A' && keysym <= 'Z') {
			X_LOCK;
			state = mask_state();
			X_UNLOCK;
		}
		if (state & LockMask) {
			/* capslock set for A-Z, so no tweak */
			X_LOCK;
			k = XKeysymToKeycode(dpy, (KeySym) keysym);
			X_UNLOCK;
			tweak = 0;
		} else {
			tweak_mod(modifiers[keysym], True);
			k = keycodes[keysym];
		}
	} else {
		X_LOCK;
		k = XKeysymToKeycode(dpy, (KeySym) keysym);
		X_UNLOCK;
	}
	if (k == NoSymbol && add_keysyms && ! IsModifierKey(keysym)) {
		int new_kc = add_keysym(keysym);
		if (new_kc) {
			k = new_kc;
		}
	}

	if (sloppy_keys) {
		int new_kc;
		if (sloppy_key_check((int) k, down, keysym, &new_kc)) {
			k = (KeyCode) new_kc;
		}
	}

	if (debug_keyboard) {
		char *str = XKeysymToString(keysym);
		rfbLog("modifier_tweak_keyboard: KeySym 0x%x \"%s\" -> "
		    "KeyCode 0x%x%s\n", (int) keysym, str ? str : "null",
		    (int) k, k ? "" : " *ignored*");
	}
	if ( k != NoSymbol ) {
		X_LOCK;
		XTestFakeKeyEvent_wr(dpy, k, (Bool) down, CurrentTime);
		X_UNLOCK;
	}

	if ( tweak ) {
		tweak_mod(modifiers[keysym], False);
	}
#endif	/* NO_X11 */
}

void initialize_keyboard_and_pointer(void) {

#ifdef MACOSX
	if (macosx_console) {
		initialize_remap(remap_file);
		initialize_pointer_map(pointer_remap);
	}
#endif

	RAWFB_RET_VOID

	if (use_modifier_tweak) {
		initialize_modtweak();
	}

	initialize_remap(remap_file);
	initialize_pointer_map(pointer_remap);

	X_LOCK;
	clear_modifiers(1);
	if (clear_mods == 1) {
		clear_modifiers(0);
	}
	if (clear_mods == 3) {
		clear_locks();
	}
	X_UNLOCK;
}

void get_allowed_input(rfbClientPtr client, allowed_input_t *input) {
	ClientData *cd;
	char *str;

	input->keystroke = 0;
	input->motion    = 0;
	input->button    = 0;
	input->clipboard = 0;
	input->files     = 0;

	if (! client) {
		input->keystroke = 1;
		input->motion    = 1;
		input->button    = 1;
		input->clipboard = 1;
		input->files     = 1;
		return;
	}

	cd = (ClientData *) client->clientData;

	if (! cd) {
		return;
	}
	
	if (cd->input[0] != '-') {
		str = cd->input;
	} else if (client->viewOnly) {
		if (allowed_input_view_only) {
			str = allowed_input_view_only;
		} else {
			str = "";
		}
	} else {
		if (allowed_input_normal) {
			str = allowed_input_normal;
		} else {
			str = "KMBCF";
		}
	}
if (0) fprintf(stderr, "GAI: %s - %s\n", str, cd->input);

	while (*str) {
		if (*str == 'K') {
			input->keystroke = 1;
		} else if (*str == 'M') {
			input->motion = 1;
		} else if (*str == 'B') {
			input->button = 1;
		} else if (*str == 'C') {
			input->clipboard = 1;
		} else if (*str == 'F') {
			input->files = 1;
		}
		str++;
	}
}

static void apply_remap(rfbKeySym *keysym, int *isbutton) {
	if (keyremaps) {
		keyremap_t *remap = keyremaps;
		while (remap != NULL) {
			if (remap->before == *keysym) {
				*keysym = remap->after;
				*isbutton = remap->isbutton;
				if (debug_keyboard) {
					char *str1, *str2;
					X_LOCK;
					str1 = XKeysymToString(remap->before);
					str2 = XKeysymToString(remap->after);
					rfbLog("keyboard(): remapping keysym: "
					    "0x%x \"%s\" -> 0x%x \"%s\"\n",
					    (int) remap->before,
					    str1 ? str1 : "null",
					    (int) remap->after,
					    remap->isbutton ? "button" :
					    str2 ? str2 : "null");
					X_UNLOCK;
				}
				break;
			}
			remap = remap->next;
		}
	}
}

/* for -pipeinput mode */
static void pipe_keyboard(rfbBool down, rfbKeySym keysym, rfbClientPtr client) {
	int can_input = 0, uid = 0, isbutton = 0;
	allowed_input_t input;
	char *name;
	ClientData *cd = (ClientData *) client->clientData;

	apply_remap(&keysym, &isbutton);

	if (isbutton) {
		int mask, button = (int) keysym;
		int x = cursor_x, y = cursor_y;
		char *b, bstr[32];

		if (!down) {
			return;
		}
		if (debug_keyboard) {
			rfbLog("keyboard(): remapping keystroke to button %d"
			    " click\n", button);
		}
		dtime0(&last_key_to_button_remap_time);

		/*
		 * This in principle can be a little dicey... i.e. even
		 * remap the button click to keystroke sequences!
		 * Usually just will simulate the button click.
		 */

		/* loop over possible multiclicks: Button123 */
		sprintf(bstr, "%d", button);
		b = bstr;
		while (*b != '\0') {
			char t[2];
			int butt;
			t[0] = *b;
			t[1] = '\0';
			if (sscanf(t, "%d", &butt) == 1) {
				mask = 1<<(butt-1);
				pointer_event(mask, x, y, client);
				mask = 0;
				pointer_event(mask, x, y, client);
			}
			b++;
		}
		return;
	}

	if (pipeinput_int == PIPEINPUT_VID) {
		v4l_key_command(down, keysym, client);
	} else if (pipeinput_int == PIPEINPUT_CONSOLE) {
		console_key_command(down, keysym, client);
	} else if (pipeinput_int == PIPEINPUT_UINPUT) {
		uinput_key_command(down, keysym, client);
	} else if (pipeinput_int == PIPEINPUT_MACOSX) {
		macosx_key_command(down, keysym, client);
	} else if (pipeinput_int == PIPEINPUT_VNC) {
		vnc_reflect_send_key((uint32_t) keysym, down);
	}
	if (pipeinput_fh == NULL) {
		return;
	}

	if (! view_only) {
		get_allowed_input(client, &input);
		if (input.keystroke) {
			can_input = 1;	/* XXX distinguish later */
		}
	}
	if (cd) {
		uid = cd->uid;
	}
	if (! can_input) {
		uid = -uid;
	}

	X_LOCK;
	name = XKeysymToString(keysym);
	X_UNLOCK;

	fprintf(pipeinput_fh, "Keysym %d %d %u %s %s\n", uid, down,
	    keysym, name ? name : "null", down ? "KeyPress" : "KeyRelease");

	fflush(pipeinput_fh);
	check_pipeinput();
}

typedef struct keyevent {
	rfbKeySym sym;
	rfbBool down;
	double time;
} keyevent_t;

#define KEY_HIST 256
static int key_history_idx = -1;
static keyevent_t key_history[KEY_HIST];

double typing_rate(double time_window, int *repeating) {
	double dt = 1.0, now = dnow();
	KeySym key = NoSymbol;
	int i, idx, cnt = 0, repeat_keys = 0;

	if (key_history_idx == -1) {
		if (repeating) {
			*repeating = 0;
		}
		return 0.0;
	}
	if (time_window > 0.0) {
		dt = time_window;
	}
	for (i=0; i<KEY_HIST; i++) {
		idx = key_history_idx - i;
		if (idx < 0) {
			idx += KEY_HIST;
		}
		if (! key_history[idx].down) {
			continue;
		}
		if (now > key_history[idx].time + dt) {
			break;
		}
		cnt++;
		if (key == NoSymbol) {
			key = key_history[idx].sym;
			repeat_keys = 1;
		} else if (key == key_history[idx].sym) {
			repeat_keys++;
		}
	}

	if (repeating) {
		if (repeat_keys >= 2) {
			*repeating = repeat_keys;
		} else {
			*repeating = 0;
		}
	}

	/*
	 * n.b. keyrate could seem very high with libvncserver buffering them
	 * so avoid using small dt.
	 */
	return ((double) cnt)/dt;
}

int skip_cr_when_scaling(char *mode) {
	int got = 0;
	
	if (!scaling) {
		return 0;
	}

	if (scaling_copyrect != scaling_copyrect0) {
		/* user override via -scale: */
		if (! scaling_copyrect) {
			return 1;
		} else {
			return 0;
		}
	}
	if (*mode == 's') {
		got = got_scrollcopyrect;
	} else if (*mode == 'w') {
		got = got_wirecopyrect;
	}
	if (scaling_copyrect || got) {
		int lat, rate;
		int link = link_rate(&lat, &rate);
		if (link == LR_DIALUP) {
			return 1;
		} else if (rate < 25) {
			/* the fill-in of the repair may be too slow */
			return 1;
		} else {
			return 0;
		}
	} else {
		return 1;
	}
}

/*
 * key event handler.  See the above functions for contortions for
 * running under -modtweak.
 */
static rfbClientPtr last_keyboard_client = NULL;

void keyboard(rfbBool down, rfbKeySym keysym, rfbClientPtr client) {
	KeyCode k;
	int idx, isbutton = 0;
	allowed_input_t input;
	time_t now = time(NULL);
	double tnow;
	static int skipped_last_down;
	static rfbBool last_down;
	static rfbKeySym last_keysym = NoSymbol;
	static rfbKeySym max_keyrepeat_last_keysym = NoSymbol;
	static double max_keyrepeat_last_time = 0.0;
	static double max_keyrepeat_always = -1.0;

	if (threads_drop_input) {
		return;
	}

	dtime0(&tnow);
	got_keyboard_calls++;

	if (debug_keyboard) {
		char *str;
		X_LOCK;
		str = XKeysymToString((KeySym) keysym);
		X_UNLOCK;
		rfbLog("# keyboard(%s, 0x%x \"%s\") uip=%d  %.4f\n",
		    down ? "down":"up", (int) keysym, str ? str : "null",
		    unixpw_in_progress, tnow - x11vnc_start);
	}

	if (keysym <= 0) {
		rfbLog("keyboard: skipping 0x0 keysym\n");
		return;
	}
	
	if (unixpw_in_progress) {
		if (unixpw_denied) {
			rfbLog("keyboard: ignoring keystroke 0x%x in "
			    "unixpw_denied=1 state\n", (int) keysym);
			return;
		}
		if (client != unixpw_client) {
			rfbLog("keyboard: skipping other client in unixpw\n");
			return;
		}

		unixpw_keystroke(down, keysym, 0);

		return;
	}

	if (skip_duplicate_key_events) {
		if (keysym == last_keysym && down == last_down) {
			if (debug_keyboard) {
				rfbLog("skipping dup key event: %d 0x%x\n",
				    down, keysym);
			}
			return;
		}
	}

	if (skip_lockkeys) {
		/*  we don't handle XK_ISO*_Lock or XK_Kana_Lock ... */
		if (keysym == XK_Scroll_Lock || keysym == XK_Num_Lock ||
		    keysym == XK_Caps_Lock || keysym == XK_Shift_Lock) {
			if (debug_keyboard) {
				rfbLog("skipping lock key event: %d 0x%x\n",
				    down, keysym);
			}
			return;
		} else if (keysym >= XK_KP_0 && keysym <= XK_KP_9) {
			/* ugh this is probably what they meant... assume NumLock. */
			if (debug_keyboard) {
				rfbLog("changed KP digit to regular digit: %d 0x%x\n",
				    down, keysym);
			}
			keysym = (keysym - XK_KP_0) + XK_0;
		} else if (keysym == XK_KP_Decimal) {
			if (debug_keyboard) {
				rfbLog("changed XK_KP_Decimal to XK_period: %d 0x%x\n",
				    down, keysym);
			}
			keysym = XK_period; 
		}
	}

	INPUT_LOCK;

	last_down = down;
	last_keysym = keysym;
	last_keyboard_time = tnow;

	last_rfb_down = down;
	last_rfb_keysym = keysym;
	last_rfb_keytime = tnow;
	last_rfb_key_accepted = FALSE;

	if (key_history_idx == -1) {
		for (idx=0; idx<KEY_HIST; idx++) {
			key_history[idx].sym = NoSymbol;
			key_history[idx].down = FALSE;
			key_history[idx].time = 0.0;
		}
	}
	idx = ++key_history_idx;
	if (key_history_idx >= KEY_HIST) {
		key_history_idx = 0;
		idx = 0;
	}
	key_history[idx].sym = keysym;
	key_history[idx].down = down;
	key_history[idx].time = tnow;

	if (down && (keysym == XK_Alt_L || keysym == XK_Super_L)) {
		int i, k, run = 0, ups = 0;
		double delay = 1.0;
		KeySym ks;
		for (i=0; i<16; i++) {
			k = idx - i;
			if (k < 0) k += KEY_HIST;
			if (!key_history[k].down) {
				ups++;
				continue;
			}
			ks = key_history[k].sym;
			if (key_history[k].time < tnow - delay) {
				break;
			} else if (ks == keysym && ks == XK_Alt_L) {
				run++;
			} else if (ks == keysym && ks == XK_Super_L) {
				run++;
			} else {
				break;
			}
		}
		if (ups < 2) {
			;
		} else if (run == 3 && keysym == XK_Alt_L) {
			rfbLog("3*Alt_L, calling: refresh_screen(0)\n");
			refresh_screen(0);
		} else if (run == 4 && keysym == XK_Alt_L) {
			rfbLog("4*Alt_L, setting: do_copy_screen\n");
			do_copy_screen = 1;
		} else if (run == 5 && keysym == XK_Alt_L) {
			;
		} else if (run == 3 && keysym == XK_Super_L) {
			rfbLog("3*Super_L, calling: set_xdamage_mark()\n");
			set_xdamage_mark(0, 0, dpy_x, dpy_y);
		} else if (run == 4 && keysym == XK_Super_L) {
			rfbLog("4*Super_L, calling: check_xrecord_reset()\n");
			check_xrecord_reset(1);
		} else if (run == 5 && keysym == XK_Super_L) {
			rfbLog("5*Super_L, calling: push_black_screen(0)\n");
			push_black_screen(0);
		}
	}

#ifdef MAX_KEYREPEAT 
	if (max_keyrepeat_always < 0.0) {
		if (getenv("MAX_KEYREPEAT")) {
			max_keyrepeat_always = atof(getenv("MAX_KEYREPEAT"));
		} else {
			max_keyrepeat_always = 0.0;
		}
	}
	if (max_keyrepeat_always > 0.0) {
		max_keyrepeat_time = max_keyrepeat_always;
	}
#else
	if (0) {max_keyrepeat_always=0;}
#endif
	if (!down && skipped_last_down) {
		int db = debug_scroll;
		if (keysym == max_keyrepeat_last_keysym) {
			skipped_last_down = 0;
			if (db) rfbLog("--- scroll keyrate skipping 0x%lx %s "
			    "%.4f  %.4f\n", keysym, down ? "down":"up  ",
			    tnow - x11vnc_start, tnow - max_keyrepeat_last_time); 
			INPUT_UNLOCK;
			return;
		}
	}
	if (down && max_keyrepeat_time > 0.0) {
		int skip = 0;
		int db = debug_scroll;

		if (max_keyrepeat_last_keysym != NoSymbol &&
		    max_keyrepeat_last_keysym != keysym) {
			;
		} else {
			if (tnow < max_keyrepeat_last_time+max_keyrepeat_time) {
				skip = 1;
			}
		}
		max_keyrepeat_time = 0.0;
		if (skip) {
			if (db) rfbLog("--- scroll keyrate skipping 0x%lx %s "
			    "%.4f  %.4f\n", keysym, down ? "down":"up  ",
			    tnow - x11vnc_start, tnow - max_keyrepeat_last_time); 
			max_keyrepeat_last_keysym = keysym;
			skipped_last_down = 1;
			INPUT_UNLOCK;
			return;
		} else {
			if (db) rfbLog("--- scroll keyrate KEEPING  0x%lx %s "
			    "%.4f  %.4f\n", keysym, down ? "down":"up  ",
			    tnow - x11vnc_start, tnow - max_keyrepeat_last_time); 
		}
	}
	max_keyrepeat_last_keysym = keysym;
	max_keyrepeat_last_time = tnow;
	skipped_last_down = 0;
	last_rfb_key_accepted = TRUE;

	if (pipeinput_fh != NULL || pipeinput_int) {
		pipe_keyboard(down, keysym, client);	/* MACOSX here. */
		if (! pipeinput_tee) {
			if (! view_only || raw_fb) {	/* raw_fb hack */
				last_keyboard_client = client;
				last_event = last_input = now;
				last_keyboard_input = now;
		
				last_keysym = keysym;

				last_rfb_down = down;
				last_rfb_keysym = keysym;
				last_rfb_keytime = tnow;
				last_rfb_key_injected = dnow();

				got_user_input++;
				got_keyboard_input++;
			}
			INPUT_UNLOCK;
			return;
		}
	}

	if (view_only) {
		INPUT_UNLOCK;
		return;
	}
	get_allowed_input(client, &input);
	if (! input.keystroke) {
		INPUT_UNLOCK;
		return;
	}

	track_mod_state(keysym, down, TRUE);	/* ignores remaps */

	last_keyboard_client = client;
	last_event = last_input = now;
	last_keyboard_input = now;

	last_keysym = keysym;

	last_rfb_down = down;
	last_rfb_keysym = keysym;
	last_rfb_keytime = tnow;
	last_rfb_key_injected = dnow();

	got_user_input++;
	got_keyboard_input++;
	
	RAWFB_RET_VOID


	apply_remap(&keysym, &isbutton);

	if (use_xrecord && ! xrecording && down) {

		if (!strcmp(scroll_copyrect, "never")) {
			;
		} else if (!strcmp(scroll_copyrect, "mouse")) {
			;
		} else if (skip_cr_when_scaling("scroll")) {
			;
		} else if (! xrecord_skip_keysym(keysym)) {
			snapshot_stack_list(0, 0.25);
			xrecord_watch(1, SCR_KEY);
			xrecord_set_by_keys = 1;
			xrecord_keysym = keysym;
		} else {
			if (debug_scroll) {
				char *str = XKeysymToString(keysym);
				rfbLog("xrecord_skip_keysym: %s\n",
				    str ? str : "NoSymbol");
			}
		}
	}

	if (isbutton) {
		int mask, button = (int) keysym;
		char *b, bstr[32];

		if (! down) {
			INPUT_UNLOCK;
			return;	/* nothing to send */
		}
		if (debug_keyboard) {
			rfbLog("keyboard(): remapping keystroke to button %d"
			    " click\n", button);
		}
		dtime0(&last_key_to_button_remap_time);

		X_LOCK;
		/*
		 * This in principle can be a little dicey... i.e. even
		 * remap the button click to keystroke sequences!
		 * Usually just will simulate the button click.
		 */

		/* loop over possible multiclicks: Button123 */
		sprintf(bstr, "%d", button);
		b = bstr;
		while (*b != '\0') {
			char t[2];
			int butt;
			t[0] = *b;
			t[1] = '\0';
			if (sscanf(t, "%d", &butt) == 1) {
				mask = 1<<(butt-1);
				do_button_mask_change(mask, butt);	/* down */
				mask = 0;
				do_button_mask_change(mask, butt);	/* up */
			}
			b++;
		}
		XFlush_wr(dpy);
		X_UNLOCK;
		INPUT_UNLOCK;
		return;
	}

	if (use_modifier_tweak) {
		modifier_tweak_keyboard(down, keysym, client);
		X_LOCK;
		XFlush_wr(dpy);
		X_UNLOCK;
		INPUT_UNLOCK;
		return;
	}

	X_LOCK;

	k = XKeysymToKeycode(dpy, (KeySym) keysym);

	if (k == NoSymbol && add_keysyms && ! IsModifierKey(keysym)) {
		int new_kc = add_keysym(keysym);
		if (new_kc) {
			k = new_kc;
		}
	}
	if (debug_keyboard) {
		char *str = XKeysymToString(keysym);
		rfbLog("keyboard(): KeySym 0x%x \"%s\" -> KeyCode 0x%x%s\n",
		    (int) keysym, str ? str : "null", (int) k,
		    k ? "" : " *ignored*");
	}

	if ( k != NoSymbol ) {
		XTestFakeKeyEvent_wr(dpy, k, (Bool) down, CurrentTime);
		XFlush_wr(dpy);
	}

	X_UNLOCK;
	INPUT_UNLOCK;
}


