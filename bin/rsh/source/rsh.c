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
    //syscall2(4, 1, "hi_there", 8);

    uint64_t cur_stack;

   __asm__(
    "mov %%rbp, %0" 
        : "=r" (cur_stack) :
   );

    printf("hello world, stackbase: %llx curstackptr: %llx\n", stackbase, cur_stack);
    uint64_t argc = *(uint64_t*)stackbase;
    uint64_t envc = *(uint64_t*)(stackbase + 8 + argc * 8);

    uint64_t module = *(uint64_t*)(stackbase + 8 + argc * 8 + 8 + envc * 8 + 14*8);

    printf("argc = %d \n", argc);

    for (int i=0; i<20; i++)
    {
        printf("stack[%x]: %llx\n", i*8, *(uint64_t*)(stackbase + i*8 ));
    }
    printf("module %llx\n", module);

    for(int i=0; i<16; i++)
        printf("%x ", *(uint8_t*)(module+i - 0x800000000));
    printf("\n");

    int client;
    int descriptor;

    if(ps4SocketTCPServerCreate(&descriptor, 6070, 10) != PS4_OK)
        return 0;

    int run = 1;
    while(run == 1)
    {
        client = accept(descriptor, NULL, NULL);
        if(client < 0)
            continue;

        int f = fork();
        if (f < 0) //fork not available ?
            break;

        if (f == 0)
        {
            handle_client(client);
            return 0;
        }
        close(client);
    }

    //try bind sock
    //can't, exit
    return 0;
}

void handle_client(int client_desc)
{
    printf("should handle a client\n");

    int r;
    
    uint32_t hdrsize;

    r = readall(client_desc, &hdrsize, 4); //header size first
    if (r < 1)
        return;
    char * hdr = malloc(hdrsize);
    r = readall(client_desc, &hdr, hdrsize); //header size first
    if (r < 1)
        return;

    uint cmd = * (uint32_t*) hdr;

    switch (cmd)
    {
        //most other operations can be handled with a binary file or script interpreter
        //the ability to shutdown the parent would be needed

        case 0x01:
            do_execve(client_desc);
            break;
        case 0x02:
            //put
            rcvfile(client_desc);
            break;
        case 0x03:
            //read name
            //get
            break;
    }
}

int readall(int socket, char * buf, int size)
{
    int readed;
    while (size > 0)
    {
        readed = read(socket, buf, size);
        if (readed < 1)
            return -1;
        size -= readed;
        buf += readed;
    }
    return 0;
}

int do_execve(int socket) 
{
    //should dup the descriptor to read and write file to the socket
    //read commandline
    //dup2()

    int r;
    int filename_size = 0;
    char * filename = 0;

    r = readall(socket, &filename_size, 4);
    if (r != 0)
        return -1;

    const magic = "!_MAGIC_RUN_THIS_!";
    filename = malloc(filename_size + strlen(magic) + 2);
    strcpy(filename, magic);
    filename[filename_size + strlen(magic)] = 0;
    filename[filename_size + strlen(magic) + 1] = 0;

    r = readall(socket, filename + strlen(magic), filename_size);
    if (r != 0)
        return -1;
    
    char * args[] = {filename , 0};

    char * envv[] = {0};

    execve("/system/common/lib/WebProcess.self", args, envv);
}

int rcvfile(int socket) 
{
    // int32 filesize, 
    // uint64 datasize
    // char filename
    // data
    int filename_size = 0;
    uint64_t datasize = 0;
    char * filename = 0;
    int r, readed;

    r = readall(socket, &filename_size, 4);
    if (r != 0)
        return -1;
    r = readall(socket, &datasize, 8);
    if (r != 0)
        return -1;

    filename = malloc(filename_size);
    r = readall(socket, filename, filename_size);
    if (r != 0)
        return -1;

    char * buf = malloc(4096 * 10);

    int f = open(buf, O_WRONLY | O_CREAT);

    while (datasize > 0)
    {
        int tread = 4096 * 10;
        if (tread > datasize)
            tread = datasize;

        readed = read(socket, buf, datasize);
        if (readed < 1)
            return -1;
        datasize -= readed;
        write(f, buf, readed);
    }

    printf("all writen, closing\n");

    free(filename);
    free(buf);

    close(f);
}