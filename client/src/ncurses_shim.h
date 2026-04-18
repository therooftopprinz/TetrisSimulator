/*
 * Minimal declarations for dynamically linked ncurses (Linux),
 * so the client can build when ncurses headers are not installed.
 * Values match ncurses 6.x key codes (see KEY_* in ncurses.h).
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _win_st WINDOW;

/* chtype / attributes (before mvaddch). */
typedef unsigned long chtype;
#define NCURSES_ATTR_SHIFT 8
#define NCURSES_BITS(mask, shift) (((chtype)(mask)) << ((shift) + NCURSES_ATTR_SHIFT))
#define A_NORMAL ((chtype)0)
#define A_COLOR (NCURSES_BITS(((1UL << 8) - 1UL), 0))
#define A_DIM NCURSES_BITS(1UL, 12)
#define COLOR_PAIR(n) (NCURSES_BITS(((unsigned)(n)) & 0xffU, 0) & A_COLOR)

#define COLOR_BLACK 0
#define COLOR_RED 1
#define COLOR_GREEN 2
#define COLOR_YELLOW 3
#define COLOR_BLUE 4
#define COLOR_MAGENTA 5
#define COLOR_CYAN 6
#define COLOR_WHITE 7

WINDOW* initscr(void);
extern WINDOW* stdscr;
int endwin(void);
int refresh(void);
int erase(void);
int getch(void);
int keypad(WINDOW*, int);
int nodelay(WINDOW*, int);
int cbreak(void);
int noecho(void);
int nonl(void);
int intrflush(WINDOW*, int);
int has_colors(void);
int start_color(void);
int mvaddch(int, int, chtype);
int mvaddnstr(int, int, const char*, int);
int move(int, int);
int clrtoeol(void);
int getmaxy(const WINDOW*);
int getmaxx(const WINDOW*);

extern int ESCDELAY;

#ifndef ERR
#define ERR (-1)
#endif

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#ifndef KEY_LEFT
#define KEY_LEFT 0404
#endif
#ifndef KEY_RIGHT
#define KEY_RIGHT 0405
#endif
#ifndef KEY_DOWN
#define KEY_DOWN 0402
#endif
#ifndef KEY_BACKSPACE
#define KEY_BACKSPACE 0407
#endif
#ifndef KEY_DC
#define KEY_DC 0512
#endif
#ifndef KEY_RESIZE
#define KEY_RESIZE 0632
#endif
#ifndef KEY_ENTER
#define KEY_ENTER 0527
#endif

int init_pair(short pair, short f, short b);
int use_default_colors(void);

#ifdef __cplusplus
}
#endif
