/* Copyright (C) 2006 - 2013 ScriptDev2 <http://www.scriptdev2.com/>
* This program is free software licensed under GPL version 2
* Please see the included DOCS/LICENSE.TXT for more information */

#include "../../../shared/revision_nr.h"

#ifdef WIN32
#   define MANGOS_DLL_EXPORT extern "C" __declspec(dllexport)
#elif defined( __GNUC__ )
#   define MANGOS_DLL_EXPORT extern "C"
#else
#   define MANGOS_DLL_EXPORT extern "C" export
#endif

MANGOS_DLL_EXPORT
char const* GetMangosRevStr()
{
    return REVISION_NR;
}
