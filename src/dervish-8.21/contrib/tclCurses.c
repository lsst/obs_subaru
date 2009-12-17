/*
 * TCL support to read a set of field names and values from two TCL arrays,
 * edit with a screen editor, and then stuff back
 * BEWARE: This is still under constant development
 * CAUTION:  This program relies on the system V curses package, which
 * includes a lot of functions that are not in the normal sun or vax curses
 * package.  Note also that the system V header file curses.h does not
 * bother to prototype any functions !!!!! So we get lots of warnings when
 * compiling under strict ansi.  SUN USERS: use /usr/5include and /usr/5lib
 * to get the system V curses package.  ALL USERS: Must add -lcurses to the
 * load step.
 *
 * USAGE:  screenEdit message beginline tclArray1 ncol1 tclArray 2 ncol2 ...
 * message is a tcl array.  The first element (message(0) is a 1-line message.
 * The remaining elements (which are optional) hold a flag (null or not null).
 * If not null, the corresponding line is highlighted.  This can be toggled.
 * The message array may have expanded functionality in the future.
 * beginline is the line number where the cursor should be put
 * tclArray1 is a tcl array of values to be listed in a field
 * ncol is the number of columns per field to be displayed on the screen.
 * If ncol > 0, the field is editable; if <0, it is read-only
 */

/* 
 * The function tclWindowSizeGet (equivalent to the tcl verb windowSizeGet)
 * are for sole purpose to return the number of row and column, in characters 
 * of the current window (stdout).
 *
 * This function is supplied to compensate a deficency of the 'escape sequence'
 * mechanism provide by vt100 terminal.
 *
 * These escapes sequences are used to replace the curses library. (cf 
 * file vt100.tcl and screenEdit.tcl).
 *
 * PCG 3/11/96
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include "shTclRegSupport.h"

/* There are variable name conflicts between DERVISH and the curses library
 * I think curses defines "register"
*/
#if !defined(NO_CURSES)
#  include <curses.h>

/* On the OSF1 we are using these four functions are missing
*/

#ifdef __osf__
#define	getmaxyx(win,y,x)	((y) = getmaxy(win), (x) = getmaxx(win))
#define	getmaxy(win)		((win)->_maxy)
#define	getmaxx(win)		((win)->_maxx)
#define	notimeout(win,bf)	((win)->_use_keypad = ((win)->_use_keypad & 0xFE) | ((bf) ? (0x02) : (0x00)) )
                 /* was: (((win)->_notimeout = ((bf) ? TRUE : FALSE)),OK) */
#define subpad(win,nl,nc,by,bx)	subwin((win),(nl),(nc),(by+win->_begy),(bx+win->_begx))
int copywin(WINDOW *srcwin, WINDOW *dstwin, int sminrow, int smincol, 
            int dminrow, int dmincol, int dmaxrow, int dmaxcol, int overlay)
{
  printf("Curses Not Completely installed in OSF1.\n");
  printf("This function is a dummy for copywin.\n");
  exit();
}

#endif /* __osf__ */
/* Prototypes for curses functions - needed to compile under C++ */
#ifdef __cplusplus
extern "C"
{
/* Some args are conceptually const, but SVR4 (and others?) get it wrong. */
#define _C_const /* const */
chtype     (winch)(WINDOW*);

WINDOW * (newwin)(int lines, int cols, int sy, int sx);
WINDOW * (subwin)(WINDOW *w, int lines, int cols, int sbegy, int sbegx);
WINDOW * (subpad)(WINDOW *w, int lines, int cols, int begy, int begx);
WINDOW * (newpad)(int lines, int cols);
/*WINDOW * (initscr)();*/
int      (box) (WINDOW*, int, int);
int      (delwin)(WINDOW*);
int      (getcurx)(WINDOW*);
int      (getcury)(WINDOW*);
int      (mvcur)(int, int, int, int);
int      (overlay)(WINDOW*, WINDOW*);
int      (overwrite)(WINDOW*, WINDOW*);
int      (scroll)(WINDOW*);
int      (touchwin)(WINDOW*);
int      (waddch)(WINDOW*, int);
int      (waddstr)(WINDOW*, _C_const char*);
int      (wclear)(WINDOW*);
int      (wclrtobot)(WINDOW*);
int      (wclrtoeol)(WINDOW*);
int      (wdelch)(WINDOW*);
int      (wdeleteln)(WINDOW*);
int      (werase)(WINDOW*);
int      (wgetch)(WINDOW*);
int      (wgetstr)(WINDOW*, char*);
int      (winsch)(WINDOW*, int);
int      (winsertln)(WINDOW*);
int      (wmove)(WINDOW*, int, int);
int      (wrefresh)(WINDOW*);
char     *(wstandend)(WINDOW*);
char     *(wstandout)(WINDOW*);
int	 (noecho)();
int	 (nonl)();
int	 (beep)();
int	 (keypad)(WINDOW *win, bool bf);
int	 (idlok)(WINDOW *win, bool bf);
int getmaxy(WINDOW *win);
int getmaxx(WINDOW *win);
int getcury(WINDOW *win);
int getcurx(WINDOW *win);
int copywin(WINDOW *srcwin, WINDOW *dstwin, int sminrow, int smincol, 
            int dminrow, int dmincol, int dmaxrow, int dmaxcol, int overlay);

#define _CURSES_FORMAT_ARG const char* fmt,
int      (wprintw)(WINDOW*, _CURSES_FORMAT_ARG ...);
int      (printw)(_CURSES_FORMAT_ARG ...);
int      (mvwprintw)(WINDOW*, int y, int x, _CURSES_FORMAT_ARG ...);
int      (wscanw)(WINDOW*, _CURSES_FORMAT_ARG ...);
int      (mvwscanw)(WINDOW*, int, int, _CURSES_FORMAT_ARG ...);
int      (endwin)();
}
#endif  /* ifdef cpluplus */

/* Common keyboard characters that are never defined where you need them */

#define BS 0x8
#define FF 0xc
#define NL 0xa
#define CR 0xd
#define TAB 0x9
#define VTAB 0xb
#define DEL 0x7f	/* This seems to be the delete key on my Mac */
#define ESC 0x1b

/* Control characters */

#define CTRL_A 1
#define CTRL_B 2
#define CTRL_C 3
#define CTRL_D 4
#define CTRL_E 5
#define CTRL_F 6
#define CTRL_G 7
#define CTRL_H 8
#define CTRL_I 9
#define CTRL_J 10
#define CTRL_K 11
#define CTRL_L 12
#define CTRL_M 13
#define CTRL_N 14
#define CTRL_O 15
#define CTRL_P 16
#define CTRL_Q 17
#define CTRL_R 18
#define CTRL_S 19
#define CTRL_T 20
#define CTRL_U 21
#define CTRL_V 22
#define CTRL_W 23
#define CTRL_X 24
#define CTRL_Y 25
#define CTRL_Z 26

/* Macros */

#define start(x,y)  ((x)/(y))*(y)
#define max(x,y) ((x) > (y)) ? (x) : (y)
#define min(x,y) ((x) < (y)) ? (x) : (y)

/* Static storage */

/* Max of 10 fields */
#define NFIELDMAX 10
/* win is entire viewable window; winview is for pad*/
static WINDOW *win = NULL;
static WINDOW *winview = NULL;
static WINDOW *pad = NULL;	/* Storage for all fields */
static WINDOW *message = NULL;	/* Message window is bottom 4 lines of win */
static WINDOW *padsub[NFIELDMAX];
static int nfield;
static chtype *flag = NULL;
static int isarray=0;

/*****************************************************************************/
/******************************** wmovenull *******************************/
/*****************************************************************************/
/* Move the cursor to y, x, or the first NULL in the line, whichever is 1st */

static void wmovenull (WINDOW *window, int y, int x) {
   int i;
   for (i=0; i<=x; i++) {
	if (mvwinch(window, y, i) == ACS_DIAMOND) return;
	}
   return;
   }
   
/*****************************************************************************/
/******************************** dscan **************************************/
/*****************************************************************************/
/* Append a diamond in this line if none is there */

static void dscan (WINDOW *window, int y) {
   int i, xmax, ymax;

   getmaxyx (window, ymax, xmax);

   for (i=0; i<xmax; i++) {
	if (mvwinch(window, y, i) == ACS_DIAMOND) return;
	}
   mvwinsch (window, y, xmax-1, (int)ACS_DIAMOND);
   return;
   }
   
/*****************************************************************************/
/***************************** outMessage ************************************/
/*****************************************************************************/

static void outMessage(char *string) {
/* Print an informational message */
	   wmove (message, 0, 0);
/* Have fun - use enhanced video */
	   wstandout(message);
/* The Updating Values message is misleading, so don't print */
/*	   wprintw (message, "%s",string); */
	   wstandend(message);
	   wrefresh(message);
   }

/*****************************************************************************/
/***************************** setVar  ************************************/
/*****************************************************************************/
/* Copy values back to array */
static void setVar (Tcl_Interp *interp, char **arg, WINDOW *window[],
   int ncurrent, int nf, int PADLINE, int PADCOL[], int editable[]) {

   char buffer[133];
   char valbuffer[133];
   int i,j;
   int x,y;
   int n;

   getyx (window[ncurrent], y, x);
   for (n=0; n<nf; n++) {
	if (editable[n] == 0) continue;
	for (i = 0; i < PADLINE; i++) {
	   for (j=0; j<PADCOL[n] - 1; j++) {
		valbuffer[j] = (char)mvwinch(window[n], i, j);
		if (mvwinch(window[n], i, j) == ACS_DIAMOND) break;
		}
	   valbuffer[j] = '\0';
	   sprintf (buffer, "%d", i+1);
	   Tcl_SetVar2 (interp, arg[2*(n+1)+1], buffer, valbuffer, 0);
	   }
	}
   if (isarray == 1) {
	for (i=0; i<PADLINE; i++) {
	   sprintf (buffer, "%d", i+1);
	   if (flag[i] == 0) {
		Tcl_SetVar2(interp, arg[1], buffer, "", 0);
		}
	   else {
		Tcl_SetVar2(interp, arg[1], buffer, "1", 0);
		}
	   }
	}
	Tcl_ResetResult (interp);
	wmove (window[ncurrent], y, x);
	return;
   }

/*****************************************************************************/
/***************************** Exit ******************************************/
/*****************************************************************************/
static void tclExit (Tcl_Interp *interp, WINDOW *window, char ctrl) {
   char buffer[133];
   int x,y;
   int n;

   getyx (window, y, x);
   sprintf (buffer, "%c %d", ctrl, y+1); /* Array index is 1+ line no. */
   Tcl_SetResult (interp, buffer, TCL_VOLATILE);

   if (flag != NULL) {
	shFree(flag);
	flag = NULL;
	}
/* OK Let's try zapping windows */
/* Be careful to do in reverse order */
   for (n=nfield-1; n>=0; n--) {
	delwin(padsub[n]);	padsub[n] = NULL;
	}
   delwin(pad);			pad = NULL;
   delwin(message);		message = NULL;
   delwin(winview);		winview = NULL;
/* Don't delete the top window! And don't call initscr more than once later on.
   delwin(win);   
*/
   endwin();
   }

/*****************************************************************************/
/***************************** tclWindowSizeGet ******************************/
/*****************************************************************************/

static char *tclWindowSizeGet_use =
"Usage: windowSizeGet";
static char *tclWindowSizeGet_hlp =
   "Return the size of the current window in number of rows and columns, expressed in number of characters.";

#include <sys/ioctl.h>
#include <unistd.h> /* for STDOUT_FILENO */

static int
tclWindowSizeGet (
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )

{

  struct winsize size;
  int result;
  char buffer[100];

  result = ioctl ( STDOUT_FILENO, TIOCGWINSZ, &size );

  if (result < 0 ) {
    Tcl_SetResult (interp, "Error getting the size of the window", TCL_STATIC);
    return TCL_ERROR;
  };
  
  sprintf(buffer,"%hu %hu", size.ws_row, size.ws_col);
  Tcl_SetResult ( interp, buffer, TCL_VOLATILE);

  return TCL_OK;

};

/*****************************************************************************/
/***************************** tclCurses  ************************************/
/*****************************************************************************/

static char *module = "contrib";
static char *tclCurses_use =
"Usage: screenEdit <message> <begin-line> <Array1> <len1> <Array2> <len2> ...";
static char *tclCurses_hlp =
   "Edit up to 10 fields using a screen editor as passed through TCL arrays";

static int
tclCurses (
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )

{
   int WINLINE;		/* Number of lines in view window */
   int WINCOL;		/* Number of columns in view window */

   int PADSTART[NFIELDMAX];	/* Starting column for field name */
   int PADCOL[NFIELDMAX];	/* Number of columns for field name */
   int PADLINE;		/* Number of lines; same for all fields */
   int PADSIZE;		/* Full width of pad */
   int nline[NFIELDMAX];	/* TCL Array sizes */
   int editable[10];	/* 0 if not editable, 1 if yet */
   int n;
   int nchar;
   int i, j;
   int size;
   char buffer[133];
   char valbuffer[133];
   char c, c2, c3;
   chtype ch;
   char *name;
   int x,y;
   int beginline;	/* Beginning line in array when we first display */
   int startline;
   int startcol;
   chtype NOT_A_BOLD;

   nfield = (argc-3)/2;
   if (nfield <= 0 || nfield > 10 || (nfield * 2) +3 != argc) {
      Tcl_SetResult(interp,tclCurses_use,TCL_STATIC);
      return(TCL_ERROR);
   }

/* Input is nfield arrays, and nfield field widths.
 * Array sizes must be the same.  If the field width is negative, the field is
 * displayed in a read-only column; otherwise the values are displayed in
 * an editable
 * column.  Array elements run from 0 to n.  Element 0 is displayed
 * in a banner field and is not editable.
*/

/* Get the number of elements in each array. Use Tcl_Eval. */
   for (n=0; n<nfield; n++) {
	sprintf (buffer,"%s %s\n", "array size", argv[2*(n+1)+1]);
	if (Tcl_Eval (interp, buffer) != TCL_OK)
	   return TCL_ERROR;
	if (sscanf (interp->result, "%d",&nline[n]) == 0) {
	   Tcl_SetResult (interp, "Bad return from sscanf",TCL_STATIC);
	   return TCL_ERROR;
	   }
/* Get field widths */
	if (Tcl_GetInt (interp, argv[2*(n+2)], &PADCOL[n]) != TCL_OK)
	   return TCL_ERROR;
	if (PADCOL[n] < 0) {
	   PADCOL[n] = -PADCOL[n];
	   editable[n] = 0;
	} else {
	   editable[n] = 1;
	   }
	if (PADCOL[n] == 0) {
	   Tcl_SetResult (interp, "Column width must be > 0",TCL_STATIC);
	   return TCL_ERROR;
	   }
	}

/* Check for correctness of array lengths.  Also set starting columns in each
 * field
*/
   n=1;
   PADSTART[0] = 0;
   while (n < nfield) {
	if (nline[n] != nline[0]) {
	   Tcl_SetResult (interp,
		"Number of elements in name and value arrays are different",
		TCL_STATIC);
	   return TCL_ERROR;
	   }
/* Allow no space between fields */
	PADSTART[n] = PADSTART[n-1] + PADCOL[n-1];
	n++;
	}

/* If only column heading are supplied, that is not very interesting */
/* Well, maybe it is.  I hope curses can handle 0 length windows */
   if (nline[0] < 1) return TCL_OK;

/* PADLINE is the number of editable lines, which is 1 less than the length
 * of the TCL arrays; element 0 is used as a comment.
*/
   PADLINE = nline[0] - 1;
/* Full width of pad */
   PADSIZE = max (PADSTART[nfield-1] + PADCOL[nfield-1], COLS);

/* Get begin line */
   if (Tcl_GetInt (interp, argv[2], &beginline) != TCL_OK) return TCL_ERROR;


/* Begin curses mode */
/* Init the screen only once per invokation of the program!!! SGI doesn't care,
 * but the sun get extremely angry.  If we were supporting VMS, I think it
 * might have yet a different behavior.
*/
   if (win == NULL) {
	win = initscr();
   } else {
	erase();
	werase(win);
	clear();
	wclear(win);
	}
   WINLINE = LINES - 6;		/* LINES is filled in by initscr() */
   if (WINLINE <= 1) {
	Tcl_SetResult (interp, "Window is too small", TCL_STATIC);
	endwin();
	return TCL_ERROR;
	}
   WINCOL = min (COLS, PADSIZE);	/* COLS is filled in by initscr() */

/* Create a viewing window. */
   WINLINE = min (WINLINE, PADLINE+2);
   winview = subwin (win, WINLINE, WINCOL, 0, 0);
   if (winview == 0) {
	Tcl_SetResult (interp, "Unable to allocate viewing window", TCL_STATIC);
	endwin();
	return TCL_ERROR;
	}
   keypad(winview, FALSE);
   idlok (winview, FALSE);

/* Message window */
   message = subwin(win, 5, COLS, LINES-6, 0);
   if (message == NULL) printw ("Couldn't start message window\n");
/* Print an information message */
   wmove (message, 1, 0);
   wprintw (message, "\
<CTRL-E> End of line <CTRL-K> Delete line <CTRL-C> Abort       <CTRL-N>\n\
<CTRL-D> Page down   <CTRL-R> Refresh     <CTRL-Z> Exit        <CTRL-P>\n\
<CTRL-U> Page up     <CTRL-H> Help?       <CTRL-B> Break       <CTRL-L>\n\
<CTRL-A> Align Field <CTRL-F> Next Field  <CTRL-W>   <CTRL-X>  <CTRL-I>");

/* Display input message */
/* Be accommodating.  If error reading message, don't fret */
   if ((name = Tcl_GetVar2 (interp, argv[1], "0", 0)) == NULL) {
	name=argv[1];
	isarray = 0;
	}
   else {
	isarray = 1;
	}
   nchar = strlen(name);
   nchar = min(nchar, COLS);
   for (i=0; i<nchar; i++) {
	if (nchar == 0) break;
	mvwaddch(message, 0, i, (int)(name[i] | A_BOLD));
	}

/* Set keyboard options */
   cbreak();
/* Try raw mode; cntrl-z and cntrl-c interact badly with curses otherwise,
 * probably because ftcl traps them */
   raw();
   noecho();
   nonl();
   keypad(win, FALSE);
/* Don't use timeout when interpreting <ESC> sequences.  My MAC is having
 * trouble with repeating arrow key strokes, and maybe this will fix it
*/
   notimeout(win, TRUE);

/* Create a full sized pad to store all fields outside of message window */
/* PADLINE is the number of array elements; pad by 2 to store a line of ---
 * above and below  */

   pad = newpad (PADLINE+2, PADSIZE);
   if (pad == 0) {
	Tcl_SetResult (interp, "Unable to allocate pad", TCL_STATIC);
	endwin();
	return TCL_ERROR;
	}

/* Stick ---- in the first and last lines */
   for (j=0; j<COLS; j++) {
	mvwaddch (pad, 0, j, '-');
	mvwaddch (pad, PADLINE+1, j, '-');
	}

/* Allocate space for flags */
   if (PADLINE > 0) flag = (chtype *)shMalloc(PADLINE*sizeof(chtype));
/* Check for message flags and do reverse video if present */
   for (i=0; i<PADLINE; i++) {
	sprintf (buffer, "%d", i+1);
	if ((name = Tcl_GetVar2 (interp, argv[1], buffer, 0)) != NULL) {
	   if (strcmp(name,"") != 0) {
		flag[i] = A_BOLD;
	   } else {
		flag[i] = 0;
		}
	} else {
	   flag[i] = 0;
	   }
	}

/* Now create subpads.  These start in second line of pad. */
   for (n=0; n<nfield; n++) {
	padsub[n] = subpad (pad, PADLINE, PADCOL[n], 1, PADSTART[n]);
	if (padsub[n] == NULL) {
	   Tcl_SetResult (interp, "Unable to allocate subpad", TCL_STATIC);
	   endwin();
	   return TCL_ERROR;
	   }
/* Fill in the value field */
	for (i = 0; i < PADLINE; i++) {
	   sprintf (buffer, "%d", i+1);
	   name = Tcl_GetVar2 (interp, argv[2*(n+1)+1], buffer, 0);
	   size = min(strlen(name), PADCOL[n]-1);
	   for (j=0; j<size; j++) {
		mvwaddch (padsub[n], i, j, name[j]);
		}
/* Insert diamond at end of line in editable fields */
	   if (editable[n]) mvwaddch (padsub[n], i, j, (int)ACS_DIAMOND);
	   }
/* Enter element 0 of arrays into first line of pad.*/
/* Have fun - make characters bold */
	name = Tcl_GetVar2 (interp, argv[2*(n+1)+1], "0", 0);
	size = min(strlen(name), PADCOL[n]);
	for (j=0; j<size; j++) {
	   mvwaddch (pad, 0, j+PADSTART[n], (int)(name[j] | A_BOLD));
	   }
	}


/* Reset result; sometimes stuff gets left behind */
   Tcl_ResetResult (interp);

   x = 0;
/* beginline is index in input array to start at.  Adjust in case it is wacko */
   if (beginline < 1) beginline = 1;
   if (beginline > PADLINE) beginline = PADLINE;
   y = beginline-1;
/* startcol is the column in pad that aligns with the 1st column of the viewing
 * window */
   startcol = 0;

/* Which field? Put in first editable field */
   n=0;
   for (i=0; i<nfield; i++) {
	if (editable[i]) {n=i; break;}
	}
   wmove (padsub[n], y, x);
/* Global refresh at the start */
   touchwin(win);
   refresh();

   NOT_A_BOLD = -A_BOLD - 1;
/********************* Main loop to process keystrokes **********************/
/* Printable characters are dumped to the screen. Control characters and
 * special keypad characters do various things.  It should be pretty easy
 * to edit, add to this list.  I have picked editing commands that are like what
 * the old SAO Forth editor used */

   while (1) {
	getyx (padsub[n], y, x);
/* y is cursor position in subpad; increment by 1 to get position in pad */
	y++;
/*	touchline (pad, y); */
	touchwin (pad);
	startline = start(y, WINLINE);
	startline = min (startline, PADLINE + 2 - WINLINE);
/* For lines that are marked in the flag array, make all characters BOLD */
/* Do reversal of fortune for all characters except diamonds. */
	for (i=1; i<=PADLINE; i++) {
	   for (j=0; j<PADSIZE; j++) {
		if (mvwinch(pad, i, j) == ACS_DIAMOND) continue;
	mvwaddch(pad, i, j, (int)((mvwinch(pad, i, j) & NOT_A_BOLD) | flag[i-1]));
		}
	   }
	copywin (pad, winview, startline, startcol,
		0, 0, WINLINE-1, WINCOL-1, FALSE);
/* Move curser position to that in subpad */
	wmove (winview, y - startline, x + PADSTART[n] - startcol);
	ch = wgetch(winview);		/* Fetch the next typed character */
/* Strip off attributes */
	ch = (ch & A_CHARTEXT);
	c = (char)ch;
/* If we get an escape character, look ahead at the next 2 characters.
 * Catch arrow keys explicitly.  Other function keys should result in beeps.
 * For unknown reasons, IRIX 5.2 terminal driver does not capture arrow
 * keys properly all the time, resulting in mucho garbage.  IRIX 4 and sun
 * do not have this problem.  Don't you just love computers?
*/
	if (c == ESC) {
	   ch = wgetch(winview);
	   ch = (ch & A_CHARTEXT);
	   c2 = (char)ch;
	   ch = wgetch(winview);
	   ch = (ch & A_CHARTEXT);
	   c3 = (char)ch;
	   if (c2 == '[' && c3 == 'A') ch = KEY_UP;
	   else if (c2 == '[' && c3 == 'B') ch = KEY_DOWN;
	   else if (c2 == '[' && c3 == 'C') ch = KEY_RIGHT;
	   else if (c2 == '[' && c3 == 'D') ch = KEY_LEFT;
	   else {
		ch = 'E';
		flushinp();
		}
	   }
/* Process all ascii combinations first */
   if (isascii(ch)) {
	if (isprint(c)) {		/* Printable character - go print it */
	   if (editable[n] == 0) continue;
	   if (x < PADCOL[n]-1) {
		winsch(padsub[n], (int)ch);
		getyx (padsub[n], y, x);
		dscan (padsub[n], y);
		wmove (padsub[n], y, x+1);
		}
	   }
	else if (c == CR) {	/* CR: Go to beginning of line*/
	   getyx (padsub[n], y, x);	/* or advance to next line */
	   if (x == 0) {
	      if (y < PADLINE-1) wmove (padsub[n], y+1, x);
	      }
	   else {
	      wmove (padsub[n], y, 0);
	      }
	   }
	else if (c == CTRL_R) {		/* Refresh screen */
/*	   touchwin (subpad[n]); This statement won't compile !? */
	   touchwin (pad);
	   touchwin(winview);
	   wrefresh(winview);
	   touchwin(win);
	   wrefresh(win);
	   }
/* Internal editing commands */
	else if (c == CTRL_U) {  /* Page up */
	   getyx (padsub[n], y, x);
	   y = max(y - WINLINE, 0);
	   wmove (padsub[n], y, 0);
	   }
	else if (c == CTRL_D) {  /* Page down */
	   getyx (padsub[n], y, x);
	   y = min(y + WINLINE, PADLINE-1);
	   wmove (padsub[n], y, 0);
	   }
	else if (c == CTRL_K) {		/* Clear line */
	   if (editable[n] == 0) continue;
	   getyx (padsub[n], y, x);
	   wmove (padsub[n], y, 0);
	   wclrtoeol(padsub[n]);
	   mvwaddch (padsub[n], y, 0, (int)ACS_DIAMOND);
	   wmove (padsub[n], y, 0);
	   }
	else if (c == BS || c == DEL) { /* Delete previous character */
	   if (editable[n] == 0) continue;
	   getyx (padsub[n], y, x);
	   if (x > 0) {
		wmove (padsub[n], y, x-1);
		wdelch(padsub[n]);
		}
	   }
	else if (c == CTRL_E) { /* Go to end-of-line */
	   getyx (padsub[n], y, x);
	   wmovenull (padsub[n], y, PADCOL[n]-1);
	   }
	else if (c == CTRL_J) { /* Reverse reverse video */
	   getyx (padsub[n], y, x);
	   if (flag != NULL) {
		if (flag[y] == 0) {
		   flag[y] = A_BOLD;
		} else {
		   flag[y] = 0;
		   }
		}
	   }
	else if (c == CTRL_F) { /* Go to next field */
	   getyx (padsub[n], y, x);
	   n++;
	   if (n >= nfield) n=0;
	   wmove (padsub[n], y, 0);
	   }
	else if (c == CTRL_A) { /* Align left edge of current column with
				   left edge of viewing window.  Or toggle */
	   if (startcol == 0) {
		startcol = min(PADSTART[n], PADSIZE - WINCOL);
	   } else {
		startcol = 0;
		}
	   }
/* Multitude of exit keys.  Control C aborts, control B breaks (does not update
 * TCL arrays but returns TCL_OK), others update TCL arrays and exit.
 * All return * the control character and the current line number
 * in the result buffer.
 */
	else if (c == CTRL_C) {		/* Abort and return error */
	   tclExit (interp, padsub[n], 'C');
	   endwin();
	   return TCL_ERROR;
	   }
	else if (c == CTRL_B) {		/* Break; don't update values */
	   tclExit (interp, padsub[n], 'B');
	   endwin();
	   return TCL_OK;
	   }
	else if (c == CTRL_H) {  /* Return cursor line and control char */
	   outMessage ("Updating values");
	   setVar (interp, argv, padsub, n, nfield, PADLINE, PADCOL, editable);
	   tclExit (interp, padsub[n], 'H');
	   return TCL_OK;
	   }
	else if (c == CTRL_I) {  /* Return cursor line and control char */
	   outMessage ("Updating values");
	   setVar (interp, argv, padsub, n, nfield, PADLINE, PADCOL, editable);
	   tclExit (interp, padsub[n], 'I');
	   return TCL_OK;
	   }
	else if (c == CTRL_L) {  /* Return cursor line and control char */
	   outMessage ("Updating values");
	   setVar (interp, argv, padsub, n, nfield, PADLINE, PADCOL, editable);
	   tclExit (interp, padsub[n], 'L');
	   return TCL_OK;
	   }
	else if (c == CTRL_N) {  /* Return cursor line and control char */
	   outMessage ("Updating values");
	   setVar (interp, argv, padsub, n, nfield, PADLINE, PADCOL, editable);
	   tclExit (interp, padsub[n], 'N');
	   return TCL_OK;
	   }
	else if (c == CTRL_P) {  /* Return cursor line and control char */
	   outMessage ("Updating values");
	   setVar (interp, argv, padsub, n, nfield, PADLINE, PADCOL, editable);
	   tclExit (interp, padsub[n], 'P');
	   return TCL_OK;
	   }
	else if (c == CTRL_W) {		/* Update TCL array and exit */
	   outMessage ("Updating values");
	   setVar (interp, argv, padsub, n, nfield, PADLINE, PADCOL, editable);
	   tclExit (interp, padsub[n], 'W');
	   return TCL_OK;
	   }
	else if (c == CTRL_X) {	/* Update TCL array & exit from the editor. */
	   outMessage ("Updating values");
	   setVar (interp, argv, padsub, n, nfield, PADLINE, PADCOL, editable);
	   tclExit (interp, padsub[n], 'X');
	   return TCL_OK;
	   }
	else if (c == CTRL_Z) {		/* Normal exit */
	   outMessage ("Updating values");
	   setVar (interp, argv, padsub, n, nfield, PADLINE, PADCOL, editable);
	   tclExit (interp, padsub[n], 'Z');
	   return TCL_OK;
	   }
	else {
	   beep();
	   }
/* Now process all non-ascii combination (like arrow keys) */
   } else {
	if (ch == KEY_LEFT) {
	   getyx (padsub[n], y, x);
	   if (x > 0) wmove (padsub[n], y, x-1);
	   }
	else if (ch == KEY_RIGHT) {
	   getyx (padsub[n], y, x);
	   if (x < PADCOL[n] - 1) wmovenull (padsub[n], y, x+1);
	   }
	else if (ch == KEY_UP) {
	   getyx (padsub[n], y, x);
	   if (y > 0) {
		wmovenull (padsub[n], y-1, x);
		}
	   }
	else if (ch == KEY_DOWN) {
	   getyx (padsub[n], y, x);
	   if (y < PADLINE-1) {
		y++;
		wmovenull (padsub[n], y, x);
		}
	   }
	else if (ch == KEY_DC) {
	   if (editable[n] == 0) continue;
	   getyx (padsub[n], y, x);
	   if (x > 0) {
		wmove (padsub[n], y, x-1);
		wdelch(padsub[n]);
		}
	   }
	else {
	   beep();
	   }
   }   /* End of non-ascii processing */
	}

/* I should never get here */
/*   endwin(); */
/*   return TCL_OK; */
   }

/****************************************************************************/

/*
 * Declare my new tcl verbs to tcl
 */
void
shTclCursesDeclare (Tcl_Interp *interp) 
{
   shTclDeclare(interp,"screenEdit", 
		(Tcl_CmdProc *)tclCurses,
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclCurses_hlp, tclCurses_use);

   shTclDeclare(interp,"windowSizeGet",
		(Tcl_CmdProc *)tclWindowSizeGet,
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL,
		module, tclWindowSizeGet_hlp, tclWindowSizeGet_use);
   return;
}
#else

void
shTclCursesDeclare (Tcl_Interp *interp) /* NOTUSED */
{
   ;
}

#endif
