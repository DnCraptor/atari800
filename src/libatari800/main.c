/*
 * libatari800/main.c - Atari800 as a library - main interface
 *
 * Copyright (c) 2001-2002 Jacek Poplawski
 * Copyright (C) 2001-2014 Atari800 development team (see DOC/CREDITS)
 * Copyright (c) 2016-2019 Rob McMullen
 *
 * This file is part of the Atari800 emulator project which emulates
 * the Atari 400, 800, 800XL, 130XE, and 5200 8-bit computers.
 *
 * Atari800 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Atari800 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Atari800; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "config.h"
#include <stdlib.h>
#include <string.h>

/* Atari800 includes */
#include "atari.h"
#include "akey.h"
#include "afile.h"
#include "../input.h"
#include "antic.h"
#include "cpu.h"
#include "platform.h"
#include "memory.h"
#include "screen.h"
#include "../sound.h"
#include "util.h"
///#include "videomode.h"
#include "sio.h"
#include "cartridge.h"
#ifndef BASIC
///#include "ui.h"
#endif
#include "cfg.h"
#include "libatari800/main.h"
#include "libatari800/init.h"
#include "libatari800/input.h"
#include "libatari800/video.h"
#include "libatari800/sound.h"
#include "libatari800/statesav.h"

/* mainloop includes */
#include "antic.h"
#include "devices.h"
#include "gtia.h"
#include "pokey.h"
#ifdef PBI_BB
#include "pbi_bb.h"
#endif
#if defined(PBI_XLD) || defined (VOICEBOX)
#include "votraxsnd.h"
#endif

int PLATFORM_Configure(char *option, char *parameters)
{
	return LIBATARI800_ReadConfig(option, parameters);
}

void PLATFORM_ConfigSave(FIL *fp)
{
	;
}

int PLATFORM_Initialise(int *argc, char *argv[])
{
	printf("PLATFORM_Initialise");
	int i, j;
	int help_only = FALSE;

	for (i = j = 1; i < *argc; i++) {
		if (strcmp(argv[i], "-help") == 0) {
			help_only = TRUE;
		}
		argv[j++] = argv[i];
	}
	*argc = j;

	if (!help_only) {
		if (!LIBATARI800_Initialise()) {
			return FALSE;
		}
	}

	if (!LIBATARI800_Video_Initialise(argc, argv)
		|| !Sound_Initialise(argc, argv)
		|| !LIBATARI800_Input_Initialise(argc, argv))
		return FALSE;

	return TRUE;
}

#ifdef LIB_UI
void LIBATARI800_Frame(void)
{
	switch (INPUT_key_code) {
	case AKEY_COLDSTART:
		printf("Atari800_Coldstart");
		Atari800_Coldstart();
		break;
	case AKEY_WARMSTART:
		printf("Atari800_Warmstart");
		Atari800_Warmstart();
		break;
	case AKEY_UI:
		printf("PLATFORM_Exit");
		PLATFORM_Exit(TRUE);  /* run monitor */
		break;
	default:
		break;
	}

#ifdef PBI_BB
	PBI_BB_Frame(); /* just to make the menu key go up automatically */
#endif
#if defined(PBI_XLD) || defined (VOICEBOX)
	VOTRAXSND_Frame(); /* for the Votrax */
#endif
	Devices_Frame();
	INPUT_Frame();
	GTIA_Frame();
	//printf("ANTIC_Frame");
	ANTIC_Frame(TRUE);
	//printf("INPUT_DrawMousePointer");
	INPUT_DrawMousePointer();
	//printf("Screen_DrawAtariSpeed");
	Screen_DrawAtariSpeed(Util_time());
	//printf("Screen_DrawDiskLED");
	Screen_DrawDiskLED();
	//printf("Screen_Draw1200LED");
	Screen_Draw1200LED();
	//printf("POKEY_Frame");
	POKEY_Frame();
	//printf("Sound_Update");
	Sound_Update();
	//printf("Atari800_nframes: %d", Atari800_nframes);
	Atari800_nframes++;
}
#endif

/* Stub routines to replace text-based UI */

int UI_SelectCartTypeM(int k) {
	libatari800_error_code = LIBATARI800_UNIDENTIFIED_CART_TYPE;
	return CARTRIDGE_NONE;
}

int UI_InitialiseM(int *argc, char *argv[]) {
	return TRUE;
}

void UI_RunM(void) {
	;
}

