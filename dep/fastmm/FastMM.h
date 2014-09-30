
#ifndef __FastMM_H
#define __FastMM_H

#include <stddef.h> /* Need size_t from here. */

#define _FASTMM_IMPORT_  __declspec(dllimport)
#define _FASTMM_CALL_    __cdecl

extern "C"
{
    _FASTMM_IMPORT_ void * _FASTMM_CALL_ FastMM_malloc(size_t size);
    _FASTMM_IMPORT_ void   _FASTMM_CALL_ FastMM_free(void* ptr);
}

#endif
