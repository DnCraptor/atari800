#ifndef MEMORY_H_
#define MEMORY_H_

#include "config.h"
#include <string.h>	/* memcpy, memset */

#include "atari.h"

extern size_t MEMORY_mem_base; 

#define MEMORY_dGetByte(x)				(read8psram(MEMORY_mem_base + x))
#define MEMORY_dPutByte(x, y)			(write8psram(MEMORY_mem_base + x, y))

#ifndef WORDS_BIGENDIAN
#ifdef WORDS_UNALIGNED_OK
#define MEMORY_dGetWord(x)				UNALIGNED_GET_WORD(MEMORY_mem+(x), memory_read_word_stat)
#define MEMORY_dPutWord(x, y)			UNALIGNED_PUT_WORD(MEMORY_mem+(x), (y), memory_write_word_stat)
#define MEMORY_dGetWordAligned(x)		UNALIGNED_GET_WORD(MEMORY_mem+(x), memory_read_aligned_word_stat)
#define MEMORY_dPutWordAligned(x, y)	UNALIGNED_PUT_WORD(MEMORY_mem+(x), (y), memory_write_aligned_word_stat)
#else	/* WORDS_UNALIGNED_OK */
#define MEMORY_dGetWord(x)				(read16psram(MEMORY_mem_base + x))
#define MEMORY_dPutWord(x, y)			(write16psram(MEMORY_mem_base + x, y))
/* faster versions of MEMORY_jdGetWord and MEMORY_dPutWord for even addresses */
/* TODO: guarantee that memory is UWORD-aligned and use UWORD access */
#define MEMORY_dGetWordAligned(x)		MEMORY_dGetWord(x)
#define MEMORY_dPutWordAligned(x, y)	MEMORY_dPutWord(x, y)
#endif	/* WORDS_UNALIGNED_OK */
#else	/* WORDS_BIGENDIAN */
/* can't do any word optimizations for big endian machines */
#define MEMORY_dGetWord(x)				(MEMORY_mem[x] + (MEMORY_mem[(x) + 1] << 8))
#define MEMORY_dPutWord(x, y)			(MEMORY_mem[x] = (UBYTE) (y), MEMORY_mem[(x) + 1] = (UBYTE) ((y) >> 8))
#define MEMORY_dGetWordAligned(x)		MEMORY_dGetWord(x)
#define MEMORY_dPutWordAligned(x, y)	MEMORY_dPutWord(x, y)
#endif	/* WORDS_BIGENDIAN */

#define MEMORY_dCopyFromMem(from, to, size)		for(size_t __i = 0; __i < size; ++__i) \
 ((uint8_t*)to)[__i] = read8psram(MEMORY_mem_base + from + __i)
#define MEMORY_dCopyToMem(from, to, size)		for(size_t __i = 0; __i < size; ++__i) \
 write8psram(MEMORY_mem_base + to + __i, ((uint8_t*)from)[__i])
#define MEMORY_dFillMem(addr1, value, length)	for(size_t __i = 0; __i < length; ++__i) \
 write8psram(MEMORY_mem_base + addr1 + __i, value)

#define MEMORY_CopyToMem16(to, from, size) for(size_t __i = 0; __i < size; __i += 2) \
 write16psram(MEMORY_mem_base + to + __i, (uint16_t*)(from + __i))
#define MEMORY_CopyFromMem16(to, from, size) for(size_t __i = 0; __i < size; __i += 2) \
 *(uint8_t*)(to + __i) = read16psram(MEMORY_mem_base + from + __i)

///extern UBYTE MEMORY_mem[65536 + 2];

/* RAM size in kilobytes.
   Valid values for Atari800_MACHINE_800 are: 16, 48, 52.
   Valid values for Atari800_MACHINE_XLXE are: 16, 64, 128, 192, RAM_320_RAMBO,
   RAM_320_COMPY_SHOP, 576, 1088.
   The only valid value for Atari800_MACHINE_5200 is 16. */
#define MEMORY_RAM_320_RAMBO       320
#define MEMORY_RAM_320_COMPY_SHOP  321
extern int MEMORY_ram_size;

#define MEMORY_RAM       0
#define MEMORY_ROM       1
#define MEMORY_HARDWARE  2

#ifndef PAGED_ATTRIB

extern UBYTE MEMORY_attrib[65536];
/* Reads a byte from ADDR. Can potentially have side effects, when reading
   from hardware area. */
#define MEMORY_GetByte(addr)		(MEMORY_attrib[addr] == MEMORY_HARDWARE ? MEMORY_HwGetByte(addr, FALSE) : MEMORY_mem[addr])
/* Reads a byte from ADDR, but without any side effects. */
#define MEMORY_SafeGetByte(addr)		(MEMORY_attrib[addr] == MEMORY_HARDWARE ? MEMORY_HwGetByte(addr, TRUE) : MEMORY_mem[addr])
#define MEMORY_PutByte(addr, byte)	 do { if (MEMORY_attrib[addr] == MEMORY_RAM) MEMORY_mem[addr] = byte; else if (MEMORY_attrib[addr] == MEMORY_HARDWARE) MEMORY_HwPutByte(addr, byte); } while (0)
#define MEMORY_SetRAM(addr1, addr2) memset(MEMORY_attrib + (addr1), MEMORY_RAM, (addr2) - (addr1) + 1)
#define MEMORY_SetROM(addr1, addr2) memset(MEMORY_attrib + (addr1), MEMORY_ROM, (addr2) - (addr1) + 1)
#define MEMORY_SetHARDWARE(addr1, addr2) memset(MEMORY_attrib + (addr1), MEMORY_HARDWARE, (addr2) - (addr1) + 1)

#else /* PAGED_ATTRIB */

typedef UBYTE (*MEMORY_rdfunc)(UWORD addr, int no_side_effects);
typedef void (*MEMORY_wrfunc)(UWORD addr, UBYTE value);
extern MEMORY_rdfunc MEMORY_readmap[256];
extern MEMORY_rdfunc MEMORY_safe_readmap[256];
extern MEMORY_wrfunc MEMORY_writemap[256];
void MEMORY_ROM_PutByte(UWORD addr, UBYTE byte);
/* Reads a byte from ADDR. Can potentially have side effects, when reading
   from hardware area. */
#define MEMORY_GetByte(addr) (MEMORY_readmap[(addr) >> 8] ? (*MEMORY_readmap[(addr) >> 8])(addr, FALSE) : read8psram(MEMORY_mem_base + addr))
/* Reads a byte from ADDR, but without any side effects. */
#define MEMORY_SafeGetByte(addr) (MEMORY_readmap[(addr) >> 8] ? (*MEMORY_readmap[(addr) >> 8])(addr, TRUE) : read8psram(MEMORY_mem_base + addr))
#define MEMORY_PutByte(addr,byte)	(MEMORY_writemap[(addr) >> 8] ? ((*MEMORY_writemap[(addr) >> 8])(addr, byte), 0) : write8psram(MEMORY_mem_base + addr, byte))
#define MEMORY_SetRAM(addr1, addr2) do { \
		int i; \
		for (i = (addr1) >> 8; i <= (addr2) >> 8; i++) { \
			MEMORY_readmap[i] = NULL; \
			MEMORY_writemap[i] = NULL; \
		} \
	} while (0)
#define MEMORY_SetROM(addr1, addr2) do { \
		int i; \
		for (i = (addr1) >> 8; i <= (addr2) >> 8; i++) { \
			MEMORY_readmap[i] = NULL; \
			MEMORY_writemap[i] = MEMORY_ROM_PutByte; \
		} \
	} while (0)

#endif /* PAGED_ATTRIB */

#include <pico/platform.h>

extern const unsigned char __in_flash() __aligned(4096) MEMORY_basic[8192];
extern const unsigned char __in_flash() __aligned(4096) MEMORY_os[16384]; // OS_B
extern const unsigned char __in_flash() __aligned(4096) ATARIXL_ROM[8192];
extern const unsigned char __in_flash() __aligned(4096) MEMORY_xegame[8192]; // TBA
///extern UBYTE MEMORY_basic[8192];
///extern UBYTE MEMORY_os[16384];
///extern UBYTE MEMORY_xegame[8192];

extern int MEMORY_xe_bank;
extern int MEMORY_selftest_enabled;

extern int MEMORY_have_basic;
extern int MEMORY_cartA0BF_enabled;

/* Verifies if SIZE is a correct value for RAM size. */
int MEMORY_SizeValid(int size);
void MEMORY_InitialiseMachine(void);
void MEMORY_StateSave(UBYTE SaveVerbose);
void MEMORY_StateRead(UBYTE SaveVerbose, UBYTE StateVersion);
void MEMORY_CopyFromMem(UWORD from, UBYTE *to, int size);
void MEMORY_CopyToMem(const UBYTE *from, UWORD to, int size);
void MEMORY_HandlePORTB(UBYTE byte, UBYTE oldval);
void MEMORY_Cart809fDisable(void);
void MEMORY_Cart809fEnable(void);
void MEMORY_CartA0bfDisable(void);
void MEMORY_CartA0bfEnable(void);
#define MEMORY_CopyFromCart(addr1, addr2, src) for(size_t __i = 0; __i < (addr2) - (addr1) + 1; ++__i) \
 write8psram(MEMORY_mem_base + (addr1) + __i, ((uint8_t*)src)[__i])
#define MEMORY_CopyToCart(addr1, addr2, dst) for(size_t __i = 0; __i < (addr2) - (addr1) + 1; ++__i) \
 ((uint8_t*)dst)[__i] = read8psram(MEMORY_mem_base + (addr1) + __i)
void MEMORY_GetCharset(UBYTE *cs);

/* Mosaic and Axlon 400/800 RAM extensions */
extern int MEMORY_mosaic_num_banks;
extern int MEMORY_axlon_0f_mirror;
extern int MEMORY_axlon_num_banks;

/* Controls presence of MapRAM memory modification for XL/XE mode. */
extern int MEMORY_enable_mapram;

#ifndef PAGED_MEM
/* Reads a byte from the specified special address (not RAM or ROM). */
UBYTE MEMORY_HwGetByte(UWORD addr, int safe);

/* Stores a byte at the specified special address (not RAM or ROM). */
void MEMORY_HwPutByte(UWORD addr, UBYTE byte);
#endif /* PAGED_MEM */

#endif /* MEMORY_H_ */
