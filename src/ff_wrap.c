#include <ff_wrap.h>

FIL __files[4] = { 0 }; // global

#define __VALIST __gnuc_va_list
int	vsnprintf (char *__restrict, size_t, const char *__restrict, __VALIST)
               _ATTRIBUTE ((__format__ (__printf__, 3, 0)));

int ___fprintf(FIL* F, const char *format, ...) {
    char buffer[FILENAME_MAX];  // Буфер для форматированной строки
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buffer, FILENAME_MAX, format, args);  // Форматируем строку
    va_end(args);

    if (len < 0) return len;  // Ошибка форматирования

    UINT bw;
    FRESULT res = f_write(F, buffer, len, &bw);  // Записываем в файл

    return (res == FR_OK) ? bw : -1;  // Возвращаем количество записанных байтов или ошибку
}
