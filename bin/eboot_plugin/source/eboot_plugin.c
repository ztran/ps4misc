#define _WANT_UCRED
#define _XOPEN_SOURCE 700
#define __BSD_VISIBLE 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/elf64.h>
#include <ps4/standard_io.h>
#include <kernel.h>
#include <ps4/kernel.h>

#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/user.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <ps4/socket.h>

#include <netinet/in.h>

#include <ps4/util.h>
#include <ps4/error.h>

void syscall2(uint64_t i_rdi, ...);

__asm__("syscall2: push %r11\n\
   mov $0x93a4FFFF8, %r11\n\
   mov (%r11), %r11\n\
   mov $0, %rax;\n\
   call *%r11\n\
   pop %r11\n ret");

void handle_client(int client_desc);

int main(uint64_t stackbase) {
    syscall2(594, "hi_there");
    syscall2(72, 0x1337);

    uint64_t cur_stack;

   __asm__(
    "mov %%rbp, %0" 
        : "=r" (cur_stack) :
   );

    printf("hello world, stackbase: %llx curstackptr: %llx\n", stackbase, cur_stack);
    uint64_t argc = *(uint64_t*)stackbase;
    uint64_t envc = *(uint64_t*)(stackbase + 8 + argc * 8);

    uint64_t module = *(uint64_t*)(stackbase + 8 + argc * 8 + 8 + envc * 8 + 14*8); //this points to the data section of the module

    printf("argc = %d \n", argc);

    for (int i=0; i<20; i++)
    {
        printf("stack[%x]: %llx\n", i*8, *(uint64_t*)(stackbase + i*8 ));
    }
    printf("module %llx\n", module);

    for(int i=0; i<16; i++)
        printf("%x ", *(uint8_t*)(module+i - 0x800000000));
    printf("\n");

    //call the main module with the rigth params :P

    exit(0);

    return 0;
}
