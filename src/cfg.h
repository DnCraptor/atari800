
extern int CFG_save_on_exit;
///
int CFG_MatchTextParameter(char const *param, char const * const cfg_strings[], int cfg_strings_size);

/* Load Atari800 text configuration file. */
int CFG_LoadConfig(const char *alternate_config_filename);

/* Writes Atari800 text configuration file. */
int CFG_WriteConfig(void);
