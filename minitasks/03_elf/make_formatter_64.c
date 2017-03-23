#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>

#define FORMAT_BEGIN 18
#define PRINTF_BEGIN 33

typedef void (*formatter)(int);

char formatter_code[] = {
    0x55,                                            // push   %rbp
    0x48, 0x89, 0xe5,                                // mov    %rsp,%rbp
    0x48, 0x83, 0xec, 0x10,                          // sub    $0x10,%rsp
    0x89, 0x7d, 0xfc,                                // mov    %edi,-0x4(%rbp)
    0x8b, 0x45, 0xfc,                                // mov    -0x4(%rbp),%eax
    0x89, 0xc6,                                      // mov    %eax,%esi
    0x48, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00,        // movabs $0x0,%rdi
    0x00, 0x00, 0x00, 0xb8, 0x00, 0x00, 0x00, 0x00,  // mov    $0x0,%eax
    0x48, 0xba, 0x00, 0x00, 0x00, 0x00, 0x00,        // movabs $0x0,%rdx
    0x00, 0x00, 0x00, 0xff, 0xd2,                    // callq  *%rdx
    0x90,                                            // nop
    0xc9,                                            // leaveq
    0xc3                                             // retq
};

formatter make_formatter(const char *format)
{
    void *virt_printf_address =
        mmap(NULL, sizeof(formatter_code),
             PROT_EXEC | PROT_READ | PROT_WRITE | PROT_EXEC,
             MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    if (virt_printf_address == MAP_FAILED)
        perror("mmap failed");

    int64_t temp_code = (int64_t)(virt_printf_address);

    memcpy((void *)temp_code, formatter_code, sizeof(formatter_code));

    memcpy((void *)(temp_code + FORMAT_BEGIN), &format, sizeof(format));

    int64_t printf_addr = (int64_t)&printf;
    memcpy((void *)(temp_code + PRINTF_BEGIN), (void *)&printf_addr,
           sizeof(printf_addr));

    return (formatter)virt_printf_address;
}

int main()
{
    formatter x08_format = make_formatter("%08x\n");
    formatter xalt_format = make_formatter("%#x\n");
    formatter d_format = make_formatter("%d\n");
    formatter verbose_format = make_formatter("Liczba: 0x%9d!\n");

    x08_format(0x1234);
    xalt_format(0x5678);
    d_format(0x9abc);
    verbose_format(0xdef0);

    return 0;
}
