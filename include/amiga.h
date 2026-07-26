#ifndef HAC_AMIGA_H
#define HAC_AMIGA_H

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/io.h>
#include <exec/memory.h>
#include <exec/ports.h>

#include <intuition/intuition.h>

#include <graphics/gfxbase.h>
#include <graphics/rastport.h>
#include <graphics/text.h>

#include <devices/timer.h>
#include <devices/audio.h>
#include <devices/inputevent.h>

#include <dos/dos.h>
#include <dos/rdargs.h>

#include <workbench/startup.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>

#include <clib/exec_protos.h>
#include <clib/intuition_protos.h>
#include <clib/graphics_protos.h>
#include <clib/dos_protos.h>
#include <clib/alib_protos.h>
#include <clib/icon_protos.h>

#if defined(__SASC)

#include <pragmas/exec_pragmas.h>
#include <pragmas/intuition_pragmas.h>
#include <pragmas/graphics_pragmas.h>
#include <pragmas/dos_pragmas.h>
#include <pragmas/icon_pragmas.h>

#elif defined(__VBCC__) || defined(__GNUC__)

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/dos.h>
#include <proto/alib.h>
#include <proto/icon.h>

#else

#error Unsupported compiler

#endif

#endif
