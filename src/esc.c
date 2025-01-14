/*
 * esc.c - Patch the OS with escape sequences
 *
 * Copyright (c) 1995-1998 David Firth
 * Copyright (c) 1998-2008 Atari800 development team (see DOC/CREDITS)
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
#include "atari.h"
#include "cassette.h"
#include "cpu.h"
#include "devices.h"
#include "esc.h"
#include "log.h"
#include "memory.h"
#include "pia.h"
#include "sio.h"
#include "sysrom.h"
#ifndef BASIC
///#include "ui.h"
#endif
#include <stdlib.h>

#ifdef LIBATARI800
#include "libatari800/cpu_crash.h"
#endif

int ESC_enable_sio_patch = TRUE;

/* Now we check address of every escape code, to make sure that the patch
   has been set by the emulator and is not a CIM in Atari program.
   Also switch() for escape codes has been changed to array of pointers
   to functions. This allows adding port-specific patches (e.g. modem device)
   using ESC_Add, Devices_UpdateHATABSEntry etc. without modifying
   atari.c/devices.c. Unfortunately it can't be done for patches in Atari OS,
   because the OS in XL/XE can be disabled.
*/
static UWORD esc_address[256];
static ESC_FunctionType esc_function[256];

/* Esc function that removes the wait loop when reading the tape leader. For
   use with standard Atari OSes only. */
static void CassetteLeaderLoad(void)
{
	CASSETTE_LeaderLoad();

	/* registers for SETVBV: third system timer, ~0.1 sec */
	CPU_regA = 3;
	CPU_regX = 0;
	CPU_regY = 5;
}

/* Esc function that removes the wait loop when writing the tape leader. For
   use with standard Atari OSes only. */
static void CassetteLeaderSave(void)
{
	CASSETTE_LeaderSave();

	/* registers for SETVBV: third system timer, ~0.1 sec */
	CPU_regA = 3;
	CPU_regX = 0;
	CPU_regY = 5;
}

#if EMUOS_ALTIRRA
/* Esc function that removes the wait loop in AltirraOS's "CassetteWait"
   routine when reading or writing the tape leader. */
static void CassetteLeaderAltirra(void)
{
	if (CPU_regX == 1)
		CASSETTE_LeaderLoad();
	else
		CASSETTE_LeaderSave();
	/* Routine expects Y = 1 on exit. */
	CPU_regY = 1;
}
#endif /* EMUOS_ALTIRRA */

void ESC_ClearAll(void)
{
	int i;
	for (i = 0; i < 256; i++)
		esc_function[i] = NULL;
}

void ESC_Add(UWORD address, UBYTE esc_code, ESC_FunctionType function)
{
	esc_address[esc_code] = address;
	esc_function[esc_code] = function;
	MEMORY_dPutByte(address, 0xf2);			/* ESC */
	MEMORY_dPutByte(address + 1, esc_code);	/* ESC CODE */
}

void ESC_AddEscRts(UWORD address, UBYTE esc_code, ESC_FunctionType function)
{
	esc_address[esc_code] = address;
	esc_function[esc_code] = function;
	MEMORY_dPutByte(address, 0xf2);			/* ESC */
	MEMORY_dPutByte(address + 1, esc_code);	/* ESC CODE */
	MEMORY_dPutByte(address + 2, 0x60);		/* RTS */
}

/* 0xd2 is ESCRTS, which works same as pair of ESC and RTS (I think so...).
   So this function does effectively the same as ESC_AddEscRts,
   except that it modifies 2, not 3 bytes in Atari memory.
   I don't know why it is done that way, so I simply leave it
   unchanged (0xf2/0xd2 are used as in previous versions).
*/
void ESC_AddEscRts2(UWORD address, UBYTE esc_code, ESC_FunctionType function)
{
	esc_address[esc_code] = address;
	esc_function[esc_code] = function;
	MEMORY_dPutByte(address, 0xd2);			/* ESCRTS */
	MEMORY_dPutByte(address + 1, esc_code);	/* ESC CODE */
}

void ESC_Remove(UBYTE esc_code)
{
	esc_function[esc_code] = NULL;
}

void ESC_Run(UBYTE esc_code)
{
	if (esc_address[esc_code] == CPU_regPC - 2 && esc_function[esc_code] != NULL) {
		esc_function[esc_code]();
		return;
	}
#ifdef CRASH_MENU
	CPU_regPC -= 2;
	UI_crash_address = CPU_regPC;
	UI_crash_afterCIM = CPU_regPC + 2;
	UI_crash_code = MEMORY_dGetByte(UI_crash_address);
	UI_Run();
#else /* CRASH_MENU */
	CPU_cim_encountered = 1;
	Log_print("Invalid ESC code %02x at address %04x", esc_code, CPU_regPC - 2);
#if defined(LIBATARI800) && defined(HAVE_SETJMP)
	longjmp(libatari800_cpu_crash, LIBATARI800_INVALID_ESCAPE_OPCODE);
#endif /* LIBATARI800 && HAVE_SETJMP */
#ifndef __PLUS
	if (!Atari800_Exit(TRUE))
		exit(0);
#else /* __PLUS */
	Atari800_Exit(TRUE);
#endif /* __PLUS */
#endif /* CRASH_MENU */
}

void ESC_PatchOS(void)
{
	int patched = Devices_PatchOS();
	if (ESC_enable_sio_patch) {
		UWORD addr_l;
		UWORD addr_s;
		UBYTE check_s_0;
		UBYTE check_s_1;
#if EMUOS_ALTIRRA
		int altirra = FALSE;
#endif /* EMUOS_ALTIRRA */

		/* patch Open() of C: so we know when a leader is processed */
		switch (Atari800_os_version) {
		case SYSROM_A_NTSC:
		case SYSROM_B_NTSC:
		case SYSROM_800_CUSTOM:
			addr_l = 0xef74;
			addr_s = 0xefbc;
			check_s_0 = 0xa0;
			check_s_1 = 0x80;
			break;
		case SYSROM_A_PAL:
			addr_l = 0xef74;
			addr_s = 0xefbc;
			check_s_0 = 0xa0;
			check_s_1 = 0xc0;
			break;
		case SYSROM_AA00R10:
			addr_l = 0xed47;
			addr_s = 0xed94;
			check_s_0 = 0xa9;
			check_s_1 = 0x03;
			break;
		case SYSROM_AA01R11:
		case SYSROM_BB00R1:
		case SYSROM_BB01R2:
		case SYSROM_BB02R3:
		case SYSROM_BB02R3V4:
		case SYSROM_BB01R3:
		case SYSROM_BB01R4_OS:
		case SYSROM_BB01R59:
		case SYSROM_BB01R59A:
		case SYSROM_XL_CUSTOM:
			addr_l = 0xfd13;
			addr_s = 0xfd60;
			check_s_0 = 0xa9;
			check_s_1 = 0x03;
			break;
		case SYSROM_CC01R4:
			addr_l = 0xef74;
			addr_s = 0xefbc;
			check_s_0 = 0xa9;
			check_s_1 = 0x03;
			break;
#if EMUOS_ALTIRRA
		case SYSROM_ALTIRRA_800:
			altirra = TRUE;
			addr_l = 0xef91; /* points to CassetteWait */
			/* Fall through. */
		case SYSROM_ALTIRRA_XL:
			altirra = TRUE;
			addr_l = 0xee4a; /* points to CassetteWait */
			break;
#endif /* EMUOS_ALTIRRA */
		default:
			return;
		}
#if EMUOS_ALTIRRA
		if (altirra)
			ESC_AddEscRts(addr_l, ESC_COPENLOAD, CassetteLeaderAltirra);
		else
#endif /* EMUOS_ALTIRRA */
		{
			/* don't hurt non-standard OSes that may not support cassette at all  */
			if (MEMORY_dGetByte(addr_l)     == 0xa9 && MEMORY_dGetByte(addr_l + 1) == 0x03
			 && MEMORY_dGetByte(addr_l + 2) == 0x8d && MEMORY_dGetByte(addr_l + 3) == 0x2a
			 && MEMORY_dGetByte(addr_l + 4) == 0x02
			 && MEMORY_dGetByte(addr_s)     == check_s_0
			 && MEMORY_dGetByte(addr_s + 1) == check_s_1
			 && MEMORY_dGetByte(addr_s + 2) == 0x20 && MEMORY_dGetByte(addr_s + 3) == 0x5c
			 && MEMORY_dGetByte(addr_s + 4) == 0xe4) {
				ESC_Add(addr_l, ESC_COPENLOAD, CassetteLeaderLoad);
				ESC_Add(addr_s, ESC_COPENSAVE, CassetteLeaderSave);
			}
		}
		ESC_AddEscRts(0xe459, ESC_SIOV, SIO_Handler);
		patched = TRUE;
	}
	else {
		ESC_Remove(ESC_COPENLOAD);
		ESC_Remove(ESC_COPENSAVE);
		ESC_Remove(ESC_SIOV);
	};
	if (patched){
		UWORD addr;
		switch (Atari800_os_version) {
		case SYSROM_AA00R10:
			addr = 0xc3d2;
			break;
		case SYSROM_AA01R11:
			addr = 0xc346;
			break;
		case SYSROM_BB00R1:
			addr = 0xc32b;
			break;
		case SYSROM_BB01R2:
		case SYSROM_BB02R3:
		case SYSROM_BB01R3:
		case SYSROM_BB01R4_OS:
		case SYSROM_BB01R59:
		case SYSROM_BB01R59A:
		case SYSROM_XL_CUSTOM:
			addr = 0xc31d;
			break;
		case SYSROM_BB02R3V4:
			addr = 0xc32c;
			break;
		case SYSROM_CC01R4:
			addr = 0xc2e0;
			break;
		default:
			/* Don't disable checksum test. */
			return;
		}
		/* Disable setting NGFLAG on wrong OS checksum. */
		MEMORY_dPutByte(addr, 0xea);
		MEMORY_dPutByte(addr+1, 0xea);
	}
}

void ESC_UpdatePatches(void)
{
	/* Patch only if OS enabled. */
	if (Atari800_machine_type != Atari800_MACHINE_5200 &&
	    (Atari800_machine_type != Atari800_MACHINE_XLXE || (PIA_PORTB & 1) != 0)) {
		int const os_rom_start = Atari800_machine_type == Atari800_MACHINE_800 ? 0xd800 : 0xc000;
		/* Restore unpatched OS */
		if (os_rom_start < 0xd000)
			MEMORY_dCopyToMem(MEMORY_os, os_rom_start, 0xd000 - os_rom_start);
		MEMORY_dCopyToMem(MEMORY_os + 0xd800 - os_rom_start, 0xd800, 0x2800);
		/* Set patches */
		ESC_PatchOS();
		Devices_UpdatePatches();
	}
}
