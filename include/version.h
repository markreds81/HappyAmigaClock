#ifndef VERSION_H
#define VERSION_H

#define APP_NAME "HappyAmigaClock"
#define APP_VERSION_MAJOR 1
#define APP_VERSION_MINOR 0
#define APP_VERSION_STRING "1.0"
#define APP_VERSION_DATE "26.07.2026"

/* Leading NUL is the standard Amiga convention for an embedded $VER tag. */
#define APP_VERSION_TAG                                                        \
    "\0$VER: " APP_NAME " " APP_VERSION_STRING " (" APP_VERSION_DATE ")\r\n"

#endif /* VERSION_H */
