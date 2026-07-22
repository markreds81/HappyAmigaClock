#ifndef HAC_COMPILER_H
#define HAC_COMPILER_H

#include <exec/types.h>
#include <exec/libraries.h>

#include <intuition/intuition.h>
#include <graphics/gfxbase.h>

#include <clib/exec_protos.h>
#include <clib/intuition_protos.h>
#include <clib/graphics_protos.h>

#if defined(__SASC)

#include <pragmas/exec_pragmas.h>
#include <pragmas/intuition_pragmas.h>
#include <pragmas/graphics_pragmas.h>

#elif defined(__VBCC__)

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>

#else

#error Unsupported compiler

#endif

#endif