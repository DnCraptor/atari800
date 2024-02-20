#pragma once

#ifndef logMsg
   
#ifdef MNGR_DEBUG
extern void logMsg(char* msg);
#define printf(...) { char tmp[256]; snprintf(tmp, 256, __VA_ARGS__); logMsg(tmp); }
#define DBGM_PRINT( X) printf X
#else
#define DBGM_PRINT( X)
#define printf(...)
#endif

#endif
