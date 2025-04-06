#ifndef FF_WRAP_H
#define FF_WRAP_H
#include <stdarg.h>
#include "ff.h"

#undef FILE
typedef FIL FILE;
#define NULL 0

/*
struct dirent {
    ino_t          d_ino;       // Номер inode файла (может использоваться для определения уникальности файла)
    off_t          d_off;       // Смещение до следующего dirent в каталоге (не во всех реализациях)
    unsigned short d_reclen;    // Длина записи dirent (не всегда используется)
    unsigned char  d_type;      // Тип файла (DT_REG, DT_DIR и т. д.)
    char*          d_name;      // Имя файла (NULL-терминированная строка)
};
*/
struct dirent {
	FSIZE_t	fsize;			/* File size */
	WORD	fdate;			/* Modified date */
	WORD	d_type;			/* Modified time */
	BYTE	fattrib;		/* File attribute */
#if FF_USE_LFN
	TCHAR	altname[FF_SFN_BUF + 1];/* Altenative file name */
	TCHAR	d_name[FF_LFN_BUF + 1];	/* Primary file name */
#else
	TCHAR	d_name[12 + 1];	/* File name */
#endif
};

inline static int S_ISDIR_E(struct dirent* st) {
    return st->fattrib & AM_DIR;
}

struct stat {
    /*
    dev_t     st_dev;     // Идентификатор устройства
    ino_t     st_ino;     // Номер inode
    */
    BYTE    st_mode;    // Битовая маска прав доступа и типа файла
    FSIZE_t st_size;    // Размер файла в байтах
  ///  WORD    st_mtime;   // Время последнего изменения
    /*
    nlink_t   st_nlink;   // Количество жёстких ссылок
    uid_t     st_uid;     // UID владельца
    gid_t     st_gid;     // GID группы владельца
    dev_t     st_rdev;    // Идентификатор устройства (если это устройство)
    blksize_t st_blksize; // Размер блока ввода-вывода
    blkcnt_t  st_blocks;  // Количество занятых блоков
    time_t    st_atime;   // Время последнего доступа
    time_t    st_ctime;   // Время последнего изменения статуса (inode)
    */
};

inline static int stat(const char *path, struct stat *st) {
    FRESULT res;
    FILINFO fno;

    // Вызываем функцию FatFS для получения информации о файле
    res = f_stat(path, &fno);
    if (res != FR_OK) {
        return -1; // Ошибка при получении информации о файле
    }

    // Заполняем структуру stat
    memset(st, 0, sizeof(struct stat)); // Обнуляем структуру stat

    // Преобразуем информацию из fno в структуру stat
    st->st_mode = fno.fattrib; // Устанавливаем тип файла (директория или обычный файл)
    st->st_size = fno.fsize; // Размер файла
  ///  st->st_mtime = fno.fdate; // Время модификации файла (можно конвертировать по необходимости)

    return 0; // Успешное завершение
}

static FILINFO __FILINFO;
inline static struct dirent* readdir(DIR *d) {
    if (f_readdir(d, &__FILINFO) != FR_OK || __FILINFO.fname[0] == '\0') {
        return 0;
    }
    return (struct dirent*)&__FILINFO;
}

inline static int S_ISDIR(struct stat* st) {
    return st->st_mode & AM_DIR;
}

#undef feof
#define feof(f) f_eof(f)

extern FIL __files[4]; // global
#undef fopen
#define fopen ___fopen
inline static FIL* ___fopen(const char* fn, const char* mode) {
    for (int i = 0; i < sizeof(__files) / sizeof(FIL); ++i) {
        if (__files[i].obj.fs == 0) {
            FRESULT r = f_open(&__files[i], fn, mode[0] == 'r' ? FA_READ : FA_WRITE); // TODO:
            if (r != FR_OK) {
                return 0;
            }
            return &__files[i];
        }
    }
    return 0;
}

#undef fclose
#define fclose ___fclose
inline static void ___fclose(FIL* f) {
    f_close(f);
    f->obj.fs = 0;
}

extern FIL __nofil;
static FIL* stdin = &__nofil; /// TODO:
static FIL *__stderr = &__nofil;
#define stdout __stderr
#define stderr __stderr
#define FILENAME_MAX 256

#undef fputc
#define fputc ___fputc
inline static void ___fputc(char c, FIL* F) { char _c = c; UINT wr; f_write(F, &_c, 1, &wr); }

#define EOF -1
inline static int _fgetc(FIL* F) { char _c; UINT wr; f_read(F, &_c, 1, &wr); return wr != 1 ? EOF : _c; }

#undef fread
#define fread ___fread
inline static int ___fread(void *n, int m, int len, FIL* f) {
    UINT r = 0;
	f_read(f, n, len, &r);
	return r;
}

#undef fwrite
#define fwrite ___fwrite
inline static int ___fwrite(void *n, int m, int len, FIL* f) {
    UINT r = 0;
	f_write(f, n, len, &r);
	return r;
}

#undef fseek
#define fseek ___fseek
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
inline static int ___fseek(FIL * f, int pos, int mode) {
	return f_lseek(f, mode == SEEK_SET ? pos : (mode == SEEK_END ? f_size(f) + pos : f_tell(f) + pos)) != FR_OK;
}

#undef fgets
#define fgets ___fgets
inline static int ___fgets(char* string, const UINT sz, FIL* fp) {
	if (f_size(fp) == f_tell(fp)) return 0;
	UINT i = 0;
    while(f_size(fp) > f_tell(fp) && i < sz - 1) {
		char c = _fgetc(fp);
		if (c == '\r') continue;
		if (c == 0 || c == EOF || c == '\n') {
			break;
		}
		string[i++] = c;
		
	}
	string[i] = 0;
	return 1;
}

static DIR dir;
static inline DIR* opendir(const char* d) {
    return f_opendir(&dir, d) == FR_OK ? &dir : 0;
}

static inline void closedir(DIR * d) {
	f_closedir(d);
}

#undef fprintf
#define fprintf ___fprintf
int ___fprintf(FIL* F, const char *format, ...);

#define printf(...)

int	sprintf (char *__restrict, const char *__restrict, ...) __attribute__ ((__format__ (__printf__, 2, 3)));
int	snprintf (char *__restrict, unsigned int sz, const char *__restrict, ...) __attribute__ ((__format__ (__printf__, 3, 4)));

#define fgetc(f) ___fgetc(f)
static inline int ___fgetc (FILE* f) {
    UINT br;
    char c;
    if (f_read(f, &c, 1, &br) != FR_OK || br != 1) return EOF;
    return c;
}

#endif
