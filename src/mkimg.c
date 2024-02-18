/*
 * mkimg.c - Make an image or .h file from a binary
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

/*
 * Usage: mkimg -input filename
 *              -image filename
 *              -header filename
 *              hexaddr1
 *              hexaddr2
 *
 * Loads the input file into a 64K area of memory. The
 * input file is in standard Atari Binary load format.
 * The memory is initially filled with zeros. After the
 * memory has been loaded the program creates either an
 * image file or a standard C header file that contains
 * the memory starting with hexaddr1 at going up to,
 * but not including, hexaddr2.
 */

///#include <stdio.h>
#include "ff.h"
#define fprintf(F, ...) { char b[256]; snprintf(b, 256, __VA_ARGS__); UINT bw; f_write(&F, b, strlen(b), &bw); }

#include <string.h>

typedef enum {
	Expect_Header1,
	Expect_Header2,
	Expect_StartLo,
	Expect_StartHi,
	Expect_FinishLo,
	Expect_FinishHi,
	Expect_Data
} State;

int main_TODO1(int argc, char *argv[])
{
	unsigned char image[65536];
	char *in_filename = NULL;
	char *image_filename = NULL;
	char *header_filename = NULL;
	int error = 0;
	int start_addr = 0;
	int finish_addr = 0;
	int addr1 = -1;
	int addr2 = -1;
	int addr;
	FIL f;
	int i;

	State state = Expect_Header1;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-input") == 0)
			in_filename = argv[++i];
		else if (strcmp(argv[i], "-image") == 0)
			image_filename = argv[++i];
		else if (strcmp(argv[i], "-header") == 0)
			header_filename = argv[++i];
		else if (addr1 == -1)
			sscanf(argv[i], "%x", &addr1);
		else if (addr2 == -1)
			sscanf(argv[i], "%x", &addr2);
		else
			error = 1;
	}

	if (!in_filename || (!image_filename && !header_filename) ||
		error || (addr1 == -1) || (addr2 == -1)) {
		printf("Usage: %s -input in_fname {-image image_fname|-header header_fname} hexaddr1 hexaddr2\n",
			   argv[0]);
		return 0;
	}
	memset(image, 0, sizeof(image));

	if (f_open(&f, in_filename, FA_READ) != FR_OK) {
		perror(in_filename);
		return 1;
	}
	while (1) {
		unsigned char byte;
		UINT rb;
		if (f_read(&f, &byte, 1, &rb) != 1)
			break;
		switch (state) {
		case Expect_Header1:
			if (byte == 0xff)
				state = Expect_Header2;
			else
				printf("Error: Expecting Header1\n");
			break;
		case Expect_Header2:
			if (byte == 0xff)
				state = Expect_StartLo;
			else
				printf("Error: Expecting Header2\n");
			break;
		case Expect_StartLo:
			start_addr = (start_addr & 0xff00) | byte;
			state = Expect_StartHi;
			break;
		case Expect_StartHi:
			start_addr = (start_addr & 0x00ff) | (byte << 8);
			state = Expect_FinishLo;
			printf("StartAddr = %x\n", start_addr);
			if (start_addr == 0xffff) {
				printf("Oops thats a header\n");
				state = Expect_StartLo;
			}
			break;
		case Expect_FinishLo:
			finish_addr = (finish_addr & 0xff00) | byte;
			state = Expect_FinishHi;
			break;
		case Expect_FinishHi:
			finish_addr = (finish_addr & 0x00ff) | (byte << 8);
			state = Expect_Data;
			addr = start_addr;
			printf("FinishAddr = %x\n", finish_addr);
			break;
		case Expect_Data:
			image[addr++] = byte;
			if (addr > finish_addr)
				state = Expect_StartLo;
			break;
		default:
			printf("Error: Invalid State\n");
			return 1;
		}
	}
	f_close(&f);
	/*
	 * Write image to file
	 */
	if (image_filename) {
		if (f_open(&f, image_filename, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
			perror(image_filename);
			return 1;
		}
		UINT wr;
		f_write(&f, &image[addr1], addr2 - addr1 + 1, &wr);
		f_close(&f);
	}
	if (header_filename) {
		char *ptr;
		int j;
		if (f_open(&f, header_filename, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
			perror(header_filename);
			return 1;
		}
		for (ptr = header_filename; *ptr; ptr++) {
			if (!( (*ptr >= 'a' && *ptr <= 'z')
			    || (*ptr >= 'A' && *ptr <= 'Z')
			    || (*ptr >= '0' && *ptr <= '9') ))
				*ptr = '_';
		}
		fprintf(f, "#ifndef _%s_\n", header_filename);
		fprintf(f, "#define _%s_\n\n", header_filename);
		fprintf(f, "static unsigned char %s[] =\n{\n\t", header_filename);
		for (i = addr1, j = 0; i < addr2; i++) {
			fprintf(f, "0x%02x,", image[i]);
			if (++j == 8) {
				fprintf(f, "\n\t");
				j = 0;
			}
		}
		fprintf(f, "0x%02x\n", image[addr2]);
		fprintf(f, "};\n");
		fprintf(f, "\n#endif\n");
		f_close(&f);
	}
	return 0;
}
