#ifndef WINDOW_H
#define WINDOW_H

#include "amiga.h"

/* Opens a borderless window covering the whole active Workbench screen. */
struct Window *OpenAppWindow(void);

/* Closes a window previously returned by OpenAppWindow(). */
void CloseAppWindow(struct Window *win);

/*
 * Drains and replies every pending IntuiMessage for 'win'.
 * Returns TRUE if the user asked to quit (ESC key), FALSE otherwise.
 */
BOOL WindowProcessMessages(struct Window *win);

#endif /* WINDOW_H */
