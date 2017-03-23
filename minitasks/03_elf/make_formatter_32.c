// Compilation:
// gcc -m32 -g make_formatter.c -o make_formatter

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>

#define FORMAT_BEGIN 7
#define PRINTF_BEGIN 12
#define PRINTF_END 16

typedef void (*formatter)(int);

char formatter_code[] = {
    0x55, 0x89, 0xe5, 0xff, 0x75, 0x08,
    0x68, 0x00, 0x00, 0x00, 0x00,  // format specifier printf_address
    0xe8, 0xfc, 0xff, 0xff, 0xff,  // printf printf_address
    0xc9, 0xc3};

formatter make_formatter(const char *format) {
    void *virt_printf_address =
        mmap(NULL, sizeof(formatter_code), PROT_READ | PROT_WRITE | PROT_EXEC,
             MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    if (virt_printf_address == MAP_FAILED)
        perror("mmap failed");

    long temp_code = (long)(virt_printf_address);

    memcpy((void *)temp_code, formatter_code, sizeof(formatter_code));

    memcpy((void *)(temp_code + FORMAT_BEGIN), &format, sizeof(format));

    long printf_addr = (long)&printf - (long)(temp_code + PRINTF_END);
    memcpy((void *)(temp_code + PRINTF_BEGIN), (void *)&printf_addr,
           sizeof(printf_addr));

    return (formatter)virt_printf_address;
}

int main() {
    formatter x08_format = make_formatter("%08x\n");
    formatter xalt_format = make_formatter("%#x\n");
    formatter d_format = make_formatter("%d\n");
    formatter verbose_format = make_formatter("Liczba: %9d!\n");

    x08_format(0x1234);
    xalt_format(0x5678);
    d_format(0x9abc);
    verbose_format(0xdef0);

    return 0;
}
