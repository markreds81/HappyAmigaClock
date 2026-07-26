#include "config.h"

extern struct Library *IconBase;
extern struct DosLibrary *DOSBase;

#define ARG_TEMPLATE "SECONDS/K"
#define ARG_SECONDS 0
#define ARG_COUNT 1
#define READARGS_VERSION 36

static BOOL parseYesNo(STRPTR value, BOOL *result)
{
    if (MatchToolValue(value, "YES") ||
        MatchToolValue(value, "ON") ||
        MatchToolValue(value, "TRUE"))
    {
        *result = TRUE;
        return TRUE;
    }

    if (MatchToolValue(value, "NO") ||
        MatchToolValue(value, "OFF") ||
        MatchToolValue(value, "FALSE"))
    {
        *result = FALSE;
        return TRUE;
    }

    return FALSE;
}

static BOOL loadShellConfig(struct AppConfig *config)
{
    LONG options[ARG_COUNT];
    struct RDArgs *parsed;
    WORD i;
    BOOL valid = TRUE;

    /*
     * ReadArgs() is a V36 call.  Under Kickstart 1.3 it must not be
     * invoked; the application simply retains its defaults.
     */
    if (!DOSBase || DOSBase->dl_lib.lib_Version < READARGS_VERSION)
        return TRUE;

    for (i = 0; i < ARG_COUNT; i++)
        options[i] = 0;

    parsed = ReadArgs(ARG_TEMPLATE, options, NULL);
    if (!parsed)
        return FALSE;

    if (options[ARG_SECONDS] != 0)
        valid = parseYesNo((STRPTR)options[ARG_SECONDS],
                           &config->ac_ShowSeconds);

    FreeArgs(parsed);
    return valid;
}

static BOOL loadWorkbenchConfig(struct AppConfig *config,
                                struct WBStartup *startup)
{
    struct WBArg *programArg;
    struct DiskObject *icon;
    BPTR oldDir;
    STRPTR value;
    BOOL valid = TRUE;

    if (!startup || startup->sm_NumArgs < 1)
        return TRUE;

    programArg = &startup->sm_ArgList[0];
    oldDir = CurrentDir(programArg->wa_Lock);
    icon = GetDiskObject(programArg->wa_Name);

    if (icon)
    {
        value = (STRPTR)FindToolType((CONST_STRPTR *)icon->do_ToolTypes,
                                    "SECONDS");
        if (value)
            valid = parseYesNo(value, &config->ac_ShowSeconds);
        FreeDiskObject(icon);
    }

    CurrentDir(oldDir);
    return valid;
}

BOOL ConfigLoad(struct AppConfig *config, int argc, char **argv)
{
    config->ac_ShowSeconds = TRUE;

    if (argc == 0)
        return loadWorkbenchConfig(config, (struct WBStartup *)argv);

    return loadShellConfig(config);
}
