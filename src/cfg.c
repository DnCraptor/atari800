#include "cfg.h"
#include "log.h"
#include "esc.h"
#include "util.h"
#include "atari.h"
#include "debug.h"
#include "memory.h"
#include "sysrom.h"
#include "binload.h"
#include "devices.h"
#include "pokeysnd.h"

int CFG_MatchTextParameter(char const *param, char const * const cfg_strings[], int cfg_strings_size)
{
	int i;
	for (i = 0; i < cfg_strings_size; i ++) {
		if (Util_stricmp(param, cfg_strings[i]) == 0) {
			return i;
		}
	}
	/* Unrecognised value */
	return -1;
}

static char rtconfig_filename[FILENAME_MAX];

/* If another default path config path is defined use it
   otherwise use the default one */
#ifndef DEFAULT_CFG_NAME
#define DEFAULT_CFG_NAME "\\atari800\\atari800.cfg"
#endif

int CFG_LoadConfig(const char *alternate_config_filename) {
	printf("CFG_LoadConfig(%s)", alternate_config_filename);
	PLATFORM_ConfigInit();
	if (alternate_config_filename != NULL && *alternate_config_filename > 0) {
		Util_strlcpy(rtconfig_filename, alternate_config_filename, FILENAME_MAX);
	}
	/* else use the default config name under the HOME folder */
	else {
		char *home = getenv("HOME");
		if (home != NULL)
			Util_catpath(rtconfig_filename, home, DEFAULT_CFG_NAME);
		else
			Util_strlcpy(rtconfig_filename, DEFAULT_CFG_NAME, FILENAME_MAX);
	}
	FIL f;
	if (f_open(&f, rtconfig_filename, FA_READ) != FR_OK) {
		Log_print("User config file '%s' not found.", rtconfig_filename);
		return 0;
	}
	FIL * fp = &f;
	char string[1024] = { 0 };
	if (fgets(string, sizeof(string), fp) != 0) {
		Log_print("Using Atari800 config file: %s\nCreated by %s", rtconfig_filename, string);
	}
	while (fgets(string, sizeof(string), fp)) {
		char *ptr;
		Util_chomp(string);
		Util_trim(string);
		/* Check for comments */
		if (string[0] == '#') {
			continue;
		}
		ptr = strchr(string, '=');
		if (ptr != NULL) {
			*ptr++ = '\0';
			Util_trim(string);
			Util_trim(ptr);
			if (SYSROM_ReadConfig(string, ptr)) {
				// ??
			}
			else if (strcmp(string, "ATARI_FILES_DIR") == 0) {
		///		if (UI_n_atari_files_dir >= UI_MAX_DIRECTORIES)
		///			Log_print("All ATARI_FILES_DIR slots used!");
		///		else
		///			Util_strlcpy(UI_atari_files_dir[UI_n_atari_files_dir++], ptr, FILENAME_MAX);
			}
			else if (strcmp(string, "SAVED_FILES_DIR") == 0) {
		///		if (UI_n_saved_files_dir >= UI_MAX_DIRECTORIES)
		///			Log_print("All SAVED_FILES_DIR slots used!");
		///		else
		///			Util_strlcpy(UI_saved_files_dir[UI_n_saved_files_dir++], ptr, FILENAME_MAX);
			}
			else if (strcmp(string, "SHOW_HIDDEN_FILES") == 0) {
		///		UI_show_hidden_files = Util_sscanbool(ptr);
			} else if (strcmp(string, "DISK_DIR") == 0 || strcmp(string, "ROM_DIR") == 0
				  || strcmp(string, "EXE_DIR") == 0 || strcmp(string, "STATE_DIR") == 0) {
				/* ignore blank and "." values */
		///		if (ptr[0] != '\0' && (ptr[0] != '.' || ptr[1] != '\0'))
		///			was_obsolete_dir = TRUE;
			}
			else if (strcmp(string, "H1_DIR") == 0)
				Util_strlcpy(Devices_atari_h_dir[0], ptr, FILENAME_MAX);
			else if (strcmp(string, "H2_DIR") == 0)
				Util_strlcpy(Devices_atari_h_dir[1], ptr, FILENAME_MAX);
			else if (strcmp(string, "H3_DIR") == 0)
				Util_strlcpy(Devices_atari_h_dir[2], ptr, FILENAME_MAX);
			else if (strcmp(string, "H4_DIR") == 0)
				Util_strlcpy(Devices_atari_h_dir[3], ptr, FILENAME_MAX);
			else if (strcmp(string, "HD_READ_ONLY") == 0)
				Devices_h_read_only = Util_sscandec(ptr);
			else if (strcmp(string, "HD_DEVICE_NAME") == 0)
				Devices_h_device_name = *ptr;
			else if (strcmp(string, "PRINT_COMMAND") == 0) {
				if (!Devices_SetPrintCommand(ptr))
					Log_print("Unsafe PRINT_COMMAND ignored");
			}
			else if (strcmp(string, "ACCURATE_SKIPPED_FRAMES") == 0)
				Atari800_collisions_in_skipped_frames = Util_sscanbool(ptr);
			else if (strcmp(string, "SCREEN_REFRESH_RATIO") == 0)
				Atari800_refresh_rate = Util_sscandec(ptr);
			else if (strcmp(string, "DISABLE_BASIC") == 0)
				Atari800_disable_basic = Util_sscanbool(ptr);
			else if (strcmp(string, "ENABLE_SIO_PATCH") == 0) {
				ESC_enable_sio_patch = Util_sscanbool(ptr);
			}
			else if (strcmp(string, "ENABLE_SLOW_XEX_LOADING") == 0) {
				BINLOAD_slow_xex_loading = Util_sscanbool(ptr);
			}
			else if (strcmp(string, "ENABLE_H_PATCH") == 0) {
				Devices_enable_h_patch = Util_sscanbool(ptr);
			}
			else if (strcmp(string, "ENABLE_P_PATCH") == 0) {
				Devices_enable_p_patch = Util_sscanbool(ptr);
			}
			else if (strcmp(string, "ENABLE_R_PATCH") == 0) {
				Devices_enable_r_patch = Util_sscanbool(ptr);
			}

			else if (strcmp(string, "ENABLE_NEW_POKEY") == 0) {
#ifdef SOUND
				POKEYSND_enable_new_pokey = Util_sscanbool(ptr);
#endif /* SOUND */
			}
			else if (strcmp(string, "STEREO_POKEY") == 0) {
#ifdef STEREO_SOUND
				POKEYSND_stereo_enabled = Util_sscanbool(ptr);
#ifdef SOUND_THIN_API
				Sound_desired.channels = POKEYSND_stereo_enabled ? 2 : 1;
#endif /* SOUND_THIN_API */
#endif /* STEREO_SOUND */
			}
			else if (strcmp(string, "SPEAKER_SOUND") == 0) {
#ifdef CONSOLE_SOUND
				POKEYSND_console_sound_enabled = Util_sscanbool(ptr);
#endif
			}
			else if (strcmp(string, "SERIO_SOUND") == 0) {
#ifdef SERIO_SOUND
				POKEYSND_serio_sound_enabled = Util_sscanbool(ptr);
#endif
			}
			else if (strcmp(string, "MACHINE_TYPE") == 0) {
				if (strcmp(ptr, "Atari 400/800") == 0 ||
				    /* Also recognise legacy values of this parameter */
				    strcmp(ptr, "Atari OS/A") == 0 ||
				    strcmp(ptr, "Atari OS/B") == 0)
					Atari800_machine_type = Atari800_MACHINE_800;
				else if (strcmp(ptr, "Atari XL/XE") == 0)
					Atari800_machine_type = Atari800_MACHINE_XLXE;
				else if (strcmp(ptr, "Atari 5200") == 0)
					Atari800_machine_type = Atari800_MACHINE_5200;
				else
					Log_print("Invalid machine type: %s", ptr);
			}
			else if (strcmp(string, "RAM_SIZE") == 0) {
		/**		if (strcmp(ptr, "320 (RAMBO)") == 0)
					MEMORY_ram_size = MEMORY_RAM_320_RAMBO;
				else if (strcmp(ptr, "320 (COMPY SHOP)") == 0)
					MEMORY_ram_size = MEMORY_RAM_320_COMPY_SHOP;
				else {
					int size = Util_sscandec(ptr);
					if (MEMORY_SizeValid(size))
						MEMORY_ram_size = size;
					else
						Log_print("Invalid RAM size: %s", ptr);
				}*/
			}
			else if (strcmp(string, "DEFAULT_TV_MODE") == 0) {
				if (strcmp(ptr, "PAL") == 0)
					Atari800_tv_mode = Atari800_TV_PAL;
				else if (strcmp(ptr, "NTSC") == 0)
					Atari800_tv_mode = Atari800_TV_NTSC;
				else
					Log_print("Invalid TV Mode: %s", ptr);
			}
			else if (strcmp(string, "MOSAIC_RAM_NUM_BANKS") == 0) {
				int num = Util_sscandec(ptr);
				if (num >= 0 && num <= 64)
					MEMORY_mosaic_num_banks = num;
				else
					Log_print("Invalid Mosaic RAM number of banks: %s", ptr);
			}
			else if (strcmp(string, "AXLON_RAM_NUM_BANKS") == 0) {
				int num = Util_sscandec(ptr);
				if (num == 0 || num == 8 || num == 16 || num == 32 || num == 64 || num == 128 || num == 256)
					MEMORY_axlon_num_banks = num;
				else
					Log_print("Invalid Mosaic RAM number of banks: %s", ptr);
			}
			else if (strcmp(string, "ENABLE_MAPRAM") == 0)
				MEMORY_enable_mapram = Util_sscanbool(ptr);
			else if (strcmp(string, "BUILTIN_BASIC") == 0)
				Atari800_builtin_basic = Util_sscanbool(ptr);
			else if (strcmp(string, "KEYBOARD_LEDS") == 0)
				Atari800_keyboard_leds = Util_sscanbool(ptr);
			else if (strcmp(string, "F_KEYS") == 0)
				Atari800_f_keys = Util_sscanbool(ptr);
			else if (strcmp(string, "BUILTIN_GAME") == 0)
				Atari800_builtin_game = Util_sscanbool(ptr);
			else if (strcmp(string, "KEYBOARD_DETACHED") == 0)
				Atari800_keyboard_detached = Util_sscanbool(ptr);
			else if (strcmp(string, "1200XL_JUMPER") == 0)
				Atari800_jumper = Util_sscanbool(ptr);
			else if (strcmp(string, "CFG_SAVE_ON_EXIT") == 0) {
		///		CFG_save_on_exit = Util_sscanbool(ptr);
			}
			/* Add module-specific configurations here */
			else if (PBI_ReadConfig(string,ptr)) {
			}
			else if (CARTRIDGE_ReadConfig(string, ptr)) {
			}
			else if (CASSETTE_ReadConfig(string, ptr)) {
			}
			else if (RTIME_ReadConfig(string, ptr)) {
			}
#ifdef XEP80_EMULATION
			else if (XEP80_ReadConfig(string, ptr)) {
			}
#endif
#ifdef AF80
			else if (AF80_ReadConfig(string,ptr)) {
			}
#endif
#ifdef BIT3
			else if (BIT3_ReadConfig(string,ptr)) {
			}
#endif
#if !defined(BASIC) && !defined(CURSES_BASIC)
			else if (Colours_ReadConfig(string, ptr)) {
			}
			else if (ARTIFACT_ReadConfig(string, ptr)) {
			}
			else if (Screen_ReadConfig(string, ptr)) {
			}
#endif
#ifdef NTSC_FILTER
			else if (FILTER_NTSC_ReadConfig(string, ptr)) {
			}
#endif
#if SUPPORTS_CHANGE_VIDEOMODE
			else if (VIDEOMODE_ReadConfig(string, ptr)) {
			}
#endif
#if defined(SOUND) && defined(SOUND_THIN_API)
			else if (Sound_ReadConfig(string, ptr)) {
			}
#endif /* defined(SOUND) && defined(SOUND_THIN_API) */
#if defined(HAVE_LIBPNG) || defined(HAVE_LIBZ) || defined(AUDIO_RECORDING) || defined(VIDEO_RECORDING)
			else if (File_Export_ReadConfig(string, ptr)) {
			}
#endif
			else {
#ifdef SUPPORTS_PLATFORM_CONFIGURE
				if (!PLATFORM_Configure(string, ptr)) {
					Log_print("Unrecognized variable or bad parameters: '%s=%s'", string, ptr);
				}
#else
				Log_print("Unrecognized variable: %s", string);
#endif
			}
		}
		else {
			Log_print("Ignored config line: %s", string);
		}
	}
	f_close(&f);
	return 1;
}
