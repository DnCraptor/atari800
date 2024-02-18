#ifndef COMPFILE_H_
#define COMPFILE_H_

#include "ff.h"  /* FILE */

int CompFile_ExtractGZ(const char *infilename, FIL *outfp);
int CompFile_DCMtoATR(FIL *infp, FIL *outfp);

#endif /* COMPFILE_H_ */
