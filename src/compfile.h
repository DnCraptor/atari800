#ifndef COMPFILE_H_
#define COMPFILE_H_

#ifdef HAVR_FF_WRAP_H
#include <ff_wrap.h>
#else
#include <stdlib.h>
#include <stdio.h>
#endif

int CompFile_ExtractGZ(const char *infilename, FILE *outfp);
int CompFile_DCMtoATR(FILE *infp, FILE *outfp);

#endif /* COMPFILE_H_ */
