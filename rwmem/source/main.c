#define _WANT_UCRED
#define _XOPEN_SOURCE 700
#define __BSD_VISIBLE 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#include <unistd.h>
#include <errno.h>

#include <sys/syscall.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <sys/ptrace.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <ps4/standard_io.h>
#include <ps4/kernel.h>


#include "kmain.h"

#define DEBUG_SIZE 0x1000

#define IP(a, b, c, d) (((a) << 0) + ((b) << 8) + ((c) << 16) + ((d) << 24))
#define TCP_NODELAY 1


int my_jailbreak(struct thread *td, void *uap);

/* Function for building virtual mapping list*/

struct kinfo_vmentry *
kinfo_getvmmap(pid_t pid, int *cntp)
{
	int mib[4];
	int error;
	int cnt;
	size_t len;
	char *buf, *bp, *eb;
	struct kinfo_vmentry *kiv, *kp, *kv;

	*cntp = 0;
	len = 0;
	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_VMMAP;
	mib[3] = pid;

	error = sysctl(mib, 4, NULL, &len, NULL, 0);
	if (error)
		return (NULL);
	len = len * 4 / 3;
	buf = malloc(len);
	if (buf == NULL)
		return (NULL);
	error = sysctl(mib, 4, buf, &len, NULL, 0);
	if (error) {
		free(buf);
		return (NULL);
	}
	/* Pass 1: count items */
	cnt = 0;
	bp = buf;
	eb = buf + len;
	while (bp < eb) {
		kv = (struct kinfo_vmentry *)(uintptr_t)bp;
		if (kv->kve_structsize == 0)
			break;
		bp += kv->kve_structsize;
		cnt++;
	}

	kiv = calloc(cnt, sizeof(*kiv));
	if (kiv == NULL) {
		free(buf);
		return (NULL);
	}
	bp = buf;
	eb = buf + len;
	kp = kiv;
	/* Pass 2: unpack */
	while (bp < eb) {
		kv = (struct kinfo_vmentry *)(uintptr_t)bp;
		if (kv->kve_structsize == 0)
			break;
		/* Copy/expand into pre-zeroed buffer */
		memcpy(kp, kv, kv->kve_structsize);
		/* Advance to next packed record */
		bp += kv->kve_structsize;
		/* Set field size to fixed length, advance */
		kp->kve_structsize = sizeof(*kp);
		kp++;
	}
	free(buf);
	*cntp = cnt;
	return (kiv);	/* Caller must free() return value */
}


int get_pid_with_name(char *name) {
    #define CTL_KERN 1
    #define KERN_PROC 14
    #define KERN_PROC_PID 1

    int mib[4];
    size_t len;

    char *dump = malloc(PAGE_SIZE);

    int i = 1;
    for (i = 1; i < 100; i += 1) {
        size_t mlen = 4;
        sysctlnametomib("kern.proc.pid", mib, &mlen);
        mib[3] = i;
        len = PAGE_SIZE;

        int ret = sysctl(mib, 4, dump, &len, NULL, 0);
        if(ret != -1 && len > 0) {
            struct kinfo_proc *proc = (struct kinfo_proc *)dump;

   	


            printf("[+] PID : %d, name : %s\n", i, proc->ki_comm);
            if (strcmp(proc->ki_comm, name) == 0) {
                free(dump);
                return i;
            }
        }
    }
    free(dump);
    return -1;
}


void read_process_form_sys(int pid, uint64_t base, void *buff, int size) {
    int64_t ret;
    char *mem = malloc(sizeof(struct ReadMemArgs));
    memset(mem, 0, sizeof(struct ReadMemArgs));
    struct ReadMemArgs *args = (struct ReadMemArgs *)mem;
    args->pid = pid;
    args->offset = base;
    args->buff = buff;
    args->len = size;

    ps4KernelExecute((void*)sys_read_process_mem, args, &ret, NULL);
    free(mem);
}

void write_process_form_sys(int pid, uint64_t base, void *buff, int size) {
    int64_t ret;
    char *mem = malloc(sizeof(struct WriteMemArgs));
    memset(mem, 0, sizeof(struct WriteMemArgs));
    struct WriteMemArgs *args = (struct WriteMemArgs *)mem;
    args->pid = pid;
    args->offset = base;
    args->buff = buff;
    args->len = size;

    ps4KernelExecute((void*)sys_write_process_mem, args, &ret, NULL);
    free(mem);
}


char * readdata_hex(pid, pos, dumpsize) {
    char *readbuff = malloc(dumpsize); 
    memset(readbuff, 0, dumpsize);

    read_process_form_sys(pid, pos, readbuff, dumpsize); 
    ps4StandardIoPrintHexDump(readbuff, dumpsize);

    return readbuff;
}

//dest will be 0x683f94
//lea eax, 0x868000
__asm("alol: \
    movq    %rdi, (%rax)     \n\
    ret \
  ");

void alol();

int main(int argc, char **argv) {
    int dbgprint = 0;

    int64_t ret;
    printf("getuid() : %d\n", getuid());
    if (getuid() != 0) {
        ps4KernelExecute((void*)jailbreak, NULL, &ret, NULL);
        printf("jailbreak!!\n");
    }
	//ps4KernelCall(ps4KernelUartEnable);
	
	char *target = "SceShellCore";
    int pid = get_pid_with_name(target);

	/* Method to setup mappings for ptrace to read */

	struct kinfo_vmentry *kvm0, *kvm;
   	int d, cnt, w = 0;
	uint64_t start[1024];
	uint64_t stop[1024];
	uint64_t offsets[1024];
	uint64_t sizes[1024];
	memset(sizes, 0, sizeof(sizes));
	memset(start, 0, sizeof(start));
	memset(stop, 0, sizeof(stop));
  	memset(offsets, 0, sizeof(offsets));


  	kvm0 = kinfo_getvmmap(pid, &cnt);

	printf("	[+] vm entry list if for %s\n", target);
   	for (d = 0, kvm = kvm0; d<cnt; d++, kvm++, w++){
	   	printf("   		[+] process start is: 0x%016llX and process map size is: 0x%016llX\n", kvm->kve_start, kvm->kve_end - kvm->kve_start);
		printf("		[+] process location in memory is: 0x%016llX\n", kvm->kve_offset);

		start[w] = kvm->kve_start;			
		stop[w]	= kvm->kve_end;
		offsets[w] = kvm->kve_offset;
		sizes[w] = kvm->kve_end - kvm->kve_start;

        if (dbgprint)
            printf( "[-]type is:%X\n"
                "[-]offset is:0x%016llX\n"
                "[-]fileid is:0x%016llX\n"
                "[-]fsid is:0x%016llX\n"
                "[-]flags are:%X\n"
                "[-]resident page number:%X\n"
                "[-]# of priv pages:%X\n"
                "[-]proc bitmask:%X\n"
                "[-]vm obj ref count:%X\n"
                "[-]vm shadow count:%X\n"
                "[-]vnode type:%X\n"
                "[-]file size:0x%016llX\n"
                "[-]device id:0x%016llX\n"
                "[-]file mode:0x%016llX\n"
                "[-]kve status:0x%016llX\n"

                ,kvm->kve_type
                ,kvm->kve_offset
                ,kvm->kve_vn_fileid
                ,kvm->kve_vn_fsid
                ,kvm->kve_flags
                ,kvm->kve_resident
                ,kvm->kve_private_resident
                ,kvm->kve_protection
                ,kvm->kve_ref_count
                ,kvm->kve_shadow_count
                ,kvm->kve_vn_type
                ,kvm->kve_vn_size
                ,kvm->kve_vn_rdev
                ,kvm->kve_vn_mode
                ,kvm->kve_status);

	}

	/* Print out process vm mapping info for debug */
 


	printf("   [+] value 0 of start is: 0x%016llX\n    and value 0 of stop is: 0x%016llX\n", start[0], stop[0]);
	
    if (pid != -1) {
		
		/* set variables for reading and writing mem */
		size_t dumphexsize = 0x200;
		size_t dumpsize = 0x200; // size that you want to read
		size_t writesize = 0x40; // size of the data you are overwriting

		uint64_t base = start[0]; // use the number of the mapping you want to write to, starting with 0
		size_t intoBase = 0x14edfc;
		char *writebuff = malloc(writesize); 
		char *readbuff = malloc(dumpsize); 
		
		memset(writebuff, 0, writesize); // fill the write buffer with the changes you need to make, doesnt have to be a memset
        memset(readbuff, 0, dumpsize);

		// when writing or reading form mem, change intoBase to be how far into that mapping you want to read or write to
		// read_process_form_sys(pid, base + intoBase, readbuff, dumpsize); 
		// ps4StandardIoPrintHexDump(readbuff, dumphexsize);
		
        readbuff = readdata_hex(pid, start[0] + 0x892868, 0x60);
        free(readbuff);

        printf("\n");

        readbuff = readdata_hex(pid, start[0] + 0x868000, 0x60);
        free(readbuff);

        printf("\n");

        memcpy(writebuff, "\x48\x8d\x73\x40\x1e\x00", 7);
        memcpy(writebuff + 7, alol, 0x20);
        
        write_process_form_sys(pid, start[0] + 0x683f94, writebuff, writesize);

        uint64_t pos = start[0] + 0x683f94;

        write_process_form_sys(pid, start[0] + 0x892868, (char*)&pos, 8);

        write_process_form_sys(pid, start[0] + 0x8d66f6, "\x01", 1); //enable debug


        readbuff = readdata_hex(pid, start[0] + 0x683f94, 0x60);
        free(readbuff);

        //memcpy(writebuff, "\xc3\xc3", 2);
		//write_process_form_sys(pid, base + intoBase, writebuff, writesize);
		//read_process_form_sys(pid, base + intoBase, readbuff, dumpsize); 

		// printf("now reading changes..\n");

		// ps4StandardIoPrintHexDump(readbuff, dumphexsize);

		free(writebuff);
    }

    return EXIT_SUCCESS;
}