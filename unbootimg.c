/* tools/unbootimg/unbootimg.c
**
** Copyright 2010 szym.net
**
** A simple utility that reverses the actions of mkbootimg.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#undef NDEBUG
#include <assert.h> // FIXME?

//#define NOMINCRYPT
#ifndef NOMINCRYPT
#include "mincrypt/sha.h"
#endif

#include "bootimg.h"

int usage(void)
{
    fprintf(stderr,"usage: unbootimg <filename>\n"
                   "  Creates <filename>-mk <filename>-kernel <filename>-ramdisk.cpio.gz [<filename>-second]\n"
                   "  Remake with mkbootimg `cat <filename>-mk`\n");
    return 1;
}

int main(int argc, char **argv)
{
    const boot_img_hdr *hdr;

    const void *kernel_data = 0;
    const void *ramdisk_data = 0;
    const void *second_data = 0;
    const char *cmdline = 0;
    const char *bootimg = 0;
    const char *board = 0;
    unsigned pagesize = 2048;
    int fd;
    void *img_data = 0;
    unsigned int sz = 0;
#ifndef NOMINCRYPT
    SHA_CTX ctx;
    const uint8_t* sha;
#endif

    unsigned int base;

    char buf[256];
    FILE *fout;

    if (argc != 2) {
        return usage();
    }

    // open img file -- read all into memory
    bootimg = argv[1];
    fd = open(bootimg, O_RDONLY);
    if(fd < 0) {
        fprintf(stderr,"Could not open %s\n", bootimg);
        return 1;
    }
    // does not work
    sz = lseek(fd, 0, SEEK_END);
    assert (sz > 0);

    assert(lseek(fd, 0, SEEK_SET) == 0);

    img_data = (char*) malloc(sz);
    assert(img_data != 0);

    assert(read(fd, (char *)img_data, sz) == sz);
    close(fd);

    // parse and check hdr
    hdr = (boot_img_hdr *)img_data;

    if(memcmp(hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE) != 0) {
        fprintf(stderr,"error: kernel magic mismatch\n");
        fprintf(stderr,"%s\n", hdr->magic);
        return 1;
    }

    cmdline = hdr->cmdline;
    if(strnlen(cmdline, BOOT_ARGS_SIZE) == BOOT_ARGS_SIZE) {
        fprintf(stderr,"error: kernel commandline too large\n");
        return 1;
    }

    board = hdr->name;
    if(strnlen(board, BOOT_NAME_SIZE) == BOOT_NAME_SIZE) {
        fprintf(stderr,"error: board name too large\n");
        return 1;
    }

    if (hdr->page_size != pagesize) {
        if (hdr->page_size != 4096)
            fprintf(stderr,"WARNING: non-standard page_size!\n");
        pagesize = hdr->page_size;
    }

#define PADDED(x) ((((int)(x)-1)/pagesize+1)*pagesize)
    kernel_data = (char*)hdr + pagesize;
    ramdisk_data = (char*)kernel_data + PADDED(hdr->kernel_size);
    second_data = (char*)ramdisk_data + PADDED(hdr->ramdisk_size);
    if ((char *)second_data + PADDED(hdr->second_size) != (char *)img_data + sz) {
        fprintf(stderr,"section sizes incorrect\nkernel %x %x\nramdisk %x %x\nsecond %x %x\ntotal %x %x\n",
          (char*)kernel_data-(char*)hdr, hdr->kernel_size,
          (char*)ramdisk_data-(char*)hdr, hdr->ramdisk_size,
          (char*)second_data-(char*)hdr, hdr->second_size,
          (char *)second_data-(char*)hdr + PADDED(hdr->second_size), sz);
        if ((char *)second_data-(char*)hdr + PADDED(hdr->second_size) < sz) {
          fprintf(stderr, "...but we can still continue\n");
        } else {
          return 1;
        }
    }
#undef PADDED

    base = hdr->kernel_addr - 0x00008000;
    if ((hdr->ramdisk_addr != base + 0x01000000) ||
        (hdr->second_addr !=  base + 0x00F00000) ||
        (hdr->tags_addr !=    base + 0x00000100)) {
        fprintf(stderr,"WARNING: non-standard load addresses!\n");
    }

#ifndef NOMINCRYPT
    // check hash
    SHA_init(&ctx);
    SHA_update(&ctx, kernel_data, hdr->kernel_size);
    SHA_update(&ctx, &hdr->kernel_size, sizeof(hdr->kernel_size));
    SHA_update(&ctx, ramdisk_data, hdr->ramdisk_size);
    SHA_update(&ctx, &hdr->ramdisk_size, sizeof(hdr->ramdisk_size));
    SHA_update(&ctx, second_data, hdr->second_size);
    SHA_update(&ctx, &hdr->second_size, sizeof(hdr->second_size));
    sha = SHA_final(&ctx);
    if(memcmp(hdr->id, sha,
      SHA_DIGEST_SIZE > sizeof(hdr->id) ? sizeof(hdr->id) : SHA_DIGEST_SIZE) != 0) {
        fprintf(stderr, "WARNING: SHA checksum does not match\n");
    }
#endif

    sprintf(buf, "%s-mk", bootimg);
    fout = fopen(buf, "w");
    if (fout == 0) {
        fprintf(stderr,"could not open %s for writing", buf);
        return 1;
    }
    fprintf(fout, "--output %s --kernel %s-kernel --ramdisk %s%s %s %s%s --cmdline '%s' --board '%s' --base %x",
      bootimg, bootimg,
      hdr->ramdisk_size ? bootimg : "NONE", hdr->ramdisk_size ? "-ramdisk.cpio.gz" : "",
      hdr->second_size ? "--second" : "", hdr->second_size ? bootimg : "", hdr->second_size ? "-second" : "",
      cmdline, board, base);
    fclose(fout);

    sprintf(buf, "%s-kernel", bootimg);
    fd = open(buf, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if(fd < 0) {
        fprintf(stderr,"error: could not create '%s'\n", buf);
        return 1;
    }
    assert(write(fd, kernel_data, hdr->kernel_size) == hdr->kernel_size);
    close(fd);
    if(hdr->ramdisk_size) {
        sprintf(buf, "%s-ramdisk.cpio.gz", bootimg);
        fd = open(buf, O_CREAT | O_TRUNC | O_WRONLY, 0644);
        assert(fd >= 0);
        assert(write(fd, ramdisk_data, hdr->ramdisk_size) == hdr->ramdisk_size);
        close(fd);
    }
    if(hdr->second_size) {
        sprintf(buf, "%s-second", bootimg);
        fd = open(buf, O_CREAT | O_TRUNC | O_WRONLY, 0644);
        assert(fd >= 0);
        assert(write(fd, second_data, hdr->second_size) == hdr->second_size);
        close(fd);
    }
    return 0;
}
