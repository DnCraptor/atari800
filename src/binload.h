#ifndef BINLOAD_H_
#define BINLOAD_H_

#include "atari.h" /* UBYTE */
#include "ff.h"

extern FIL BINLOAD_bin_file;
extern int BINLOAD_bin_file_open;

int BINLOAD_Loader(const char *filename);
extern int BINLOAD_start_binloading;
extern int BINLOAD_loading_basic;

/* Set to TRUE to enable loading of XEX with approximate disk speed */
extern int BINLOAD_slow_xex_loading;

/* Indicates that a DOS file is being currently slowly loaded. */
extern int BINLOAD_wait_active;

/* Set it to TRUE to pause the current loading of a DOS file. */
extern int BINLOAD_pause_loading;

#define BINLOAD_LOADING_BASIC_SAVED              1
#define BINLOAD_LOADING_BASIC_LISTED             2
#define BINLOAD_LOADING_BASIC_LISTED_ATARI       3
#define BINLOAD_LOADING_BASIC_LISTED_CR          4
#define BINLOAD_LOADING_BASIC_LISTED_LF          5
#define BINLOAD_LOADING_BASIC_LISTED_CRLF        6
#define BINLOAD_LOADING_BASIC_RUN                7
int BINLOAD_LoaderStart(UBYTE *buffer);

#endif /* BINLOAD_H_ */
