#include "config.h"

extern struct Library *IconBase;
extern struct DosLibrary *DOSBase;

#define ARG_TEMPLATE "SECONDS/K,INVERT/K/N,MODE/K"
#define ARG_SECONDS 0
#define ARG_INVERT 1
#define ARG_MODE 2
#define ARG_COUNT 3
#define READARGS_VERSION 36
#define DEFAULT_INVERT_MINUTES 720UL
#define MAX_INVERT_MINUTES 65535UL

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

static BOOL parseMinutes(STRPTR value, ULONG *result)
{
    ULONG number = 0;

    if (!value || *value == '\0')
        return FALSE;

    while (*value)
    {
        ULONG digit;

        if (*value < '0' || *value > '9')
            return FALSE;

        digit = (ULONG)(*value++ - '0');
        if (number > (MAX_INVERT_MINUTES - digit) / 10)
            return FALSE;
        number = number * 10 + digit;
    }

    *result = number;
    return TRUE;
}

static BOOL parseMode(STRPTR value, BOOL *startDark)
{
    if (MatchToolValue(value, "LIGHT"))
    {
        *startDark = FALSE;
        return TRUE;
    }
    if (MatchToolValue(value, "DARK"))
    {
        *startDark = TRUE;
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

    if (valid && options[ARG_INVERT] != 0)
    {
        LONG minutes = *(LONG *)options[ARG_INVERT];

        if (minutes < 0 || (ULONG)minutes > MAX_INVERT_MINUTES)
            valid = FALSE;
        else
            config->ac_InvertMinutes = (ULONG)minutes;
    }

    if (valid && options[ARG_MODE] != 0)
        valid = parseMode((STRPTR)options[ARG_MODE],
                          &config->ac_StartDark);

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

        value = (STRPTR)FindToolType((CONST_STRPTR *)icon->do_ToolTypes,
                                    "INVERT");
        if (valid && value)
            valid = parseMinutes(value, &config->ac_InvertMinutes);

        value = (STRPTR)FindToolType((CONST_STRPTR *)icon->do_ToolTypes,
                                    "MODE");
        if (valid && value)
            valid = parseMode(value, &config->ac_StartDark);
        FreeDiskObject(icon);
    }

    CurrentDir(oldDir);
    return valid;
}

BOOL ConfigLoad(struct AppConfig *config, int argc, char **argv)
{
    config->ac_ShowSeconds = TRUE;
    config->ac_InvertMinutes = DEFAULT_INVERT_MINUTES;
    config->ac_StartDark = FALSE;

    if (argc == 0)
        return loadWorkbenchConfig(config, (struct WBStartup *)argv);

    return loadShellConfig(config);
}
