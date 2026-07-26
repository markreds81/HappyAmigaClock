#ifndef CONFIG_H
#define CONFIG_H

#include "amiga.h"

struct AppConfig
{
    BOOL ac_ShowSeconds;
};

/*
 * Reads SECONDS=YES|NO using ReadArgs() from Shell or FindToolType() from
 * the program icon under Workbench. On a pre-V36 DOS, Shell launches keep
 * the defaults because ReadArgs() is unavailable.
 */
BOOL ConfigLoad(struct AppConfig *config, int argc, char **argv);

#endif /* CONFIG_H */
