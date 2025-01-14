/*
 * log.c - A logging facility for debugging
 *
 * Copyright (C) 1995-1998 David Firth
 * Copyright (C) 1998-2005 Atari800 development team (see DOC/CREDITS)
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

#define _POSIX_C_SOURCE 200112L /* for vsnprintf */

#include "config.h"
#include <stdarg.h>
#include <string.h>
#ifdef ANDROID
#include <android/log.h>
#endif

#include "log.h"
#include "debug.h"

#ifdef MACOSX
#  define PRINT(a) ControlManagerMessagePrint(a)
#elif defined(ANDROID)
#  define PRINT(a) __android_log_write(ANDROID_LOG_DEBUG, "A800Core", a)
#else
#  define PRINT(a) printf("%s", a)
#endif

#ifdef BUFFERED_LOG
char Log_buffer[Log_BUFFER_SIZE];
#endif

void Log_flushlog(void)
{
#ifdef BUFFERED_LOG
	if (*Log_buffer) {
		PRINT(Log_buffer);
		*Log_buffer = 0;
	}
#endif
}
