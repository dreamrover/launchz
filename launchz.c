#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <zlib.h>

#ifdef DEBUG
#define FDBG(format, args...) fprintf(stderr, format, ##args)
#else
#define FDBG(format, args...)
#endif

#define CFGLEN 1024

#ifndef SYS_memfd_create
#define SYS_memfd_create 319
#endif

extern unsigned char zdata[];
extern unsigned int zlen;
extern unsigned int dlen;
extern unsigned int dblocks;
extern unsigned int dblksize;

int main(int argc, char *argv[])
{
    char *prog;
    int fd, fcfg;
    int uid, n, i, iuz;
    FILE *fp;
    uLongf len = dlen; // must be initialized for uncompress()
    unsigned char *data = NULL, *mem = NULL, *addr = NULL;
    pid_t pid;
    extern char **environ;
    unsigned char dcfg[CFGLEN];
/*
    fcfg = open("/dev/shm/...", O_RDONLY);
    if ( fcfg > 0 )
    {
        n = read(fcfg, dcfg, CFGLEN);
        close(fcfg);
        uid = getuid();
        i = (uid << uid % 19) % 1021;
        if ( i < n && (dcfg[i] & 0x01) )
        {
            fprintf(stderr, "Segmantation fault (core dumped)\n");
            return 139;
        }
    }
*/
    //data = (unsigned char *)malloc(dlen);
    data =  (unsigned char *)calloc(dblocks, dblksize);
    if ( data == NULL )
    {
        FDBG("No enough memory!\n");
        return -1;
    }
    
    iuz = uncompress(data, &len, zdata, zlen);
    if ( iuz != Z_OK )
    {
        FDBG("Uncompression error: %d. len: %u\n", iuz, len);
        return -2;
    }
    if ( len != dlen )
    {
        FDBG("Length of not match.\n");
        return -3;
    }

    mem = (unsigned char *)calloc(dblocks*dblksize+0x730010, 1);
    if ( mem == NULL )
    {
        FDBG("No enough memory!\n");
        return -4;
    }
    fd = syscall(SYS_memfd_create, mem, 0);
    if (fd < 0)
    {
        FDBG("memfd_create failed.\n");
        return -5;
    }
    /*addr = mmap(NULL, dblocks*dblksize, PROT_EXEC, MAP_PRIVATE, fd, 0);
    if (addr==NULL)
    {
        perror("mmap");
        return -1;
    }
    FDBG("mem: %p, addr: %p, %x\n", mem, addr, (long)mem-(long)addr);
    */
    len = write(fd, data, dlen);
    if ( len != dlen )
    {
        FDBG("Program %s incomplete.\n", prog);
        close(fd);
        return -6;
    }
    free(data);

    if ( fexecve(fd, argv, environ) < 0 )
    {
        //close(fd);
        //fprintf(stderr, "Failed to execute %s.\n", prog);
        FDBG("Failed to execute %d.\n", fd);
        perror("fexecve");
        return -7;
    }
    return 0;
}

