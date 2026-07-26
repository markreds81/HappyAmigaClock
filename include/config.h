#ifndef CONFIG_H
#define CONFIG_H

#include "amiga.h"

struct AppConfig
{
    BOOL ac_ShowSeconds;
    ULONG ac_InvertMinutes;
    BOOL ac_StartDark;
};

/*
 * Reads SECONDS=YES|NO, INVERT=<minutes> and MODE=LIGHT|DARK using
 * ReadArgs() from Shell or FindToolType() from the program icon under
 * Workbench. INVERT=0 disables palette alternation and retains MODE.
 */
BOOL ConfigLoad(struct AppConfig *config, int argc, char **argv);

#endif /* CONFIG_H */
