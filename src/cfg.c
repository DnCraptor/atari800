#include "cfg.h"
#include "util.h"

int CFG_MatchTextParameter(char const *param, char const * const cfg_strings[], int cfg_strings_size)
{
	int i;
	for (i = 0; i < cfg_strings_size; i ++) {
		if (Util_stricmp(param, cfg_strings[i]) == 0)
			return i;
	}
	/* Unrecognised value */
	return -1;
}

int CFG_LoadConfig(const char *alternate_config_filename) {
    /// TODO:
	return 1;
}
