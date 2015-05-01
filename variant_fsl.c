/*
 * Copyright (C) 2015  Vianney le Clément de Saint-Marcq <vleclement@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "bootimgtool.h"
#include "sha.h"

int fsl_read(struct bootimg *img) {
    struct boot_img_hdr *hdr = (struct boot_img_hdr *) img->image.data;
    if (memcmp(hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE) != 0) {
        fprintf(stderr, "Magic not found\n");
        return -1;
    }
    img->kernel.size = hdr->kernel_size;
    img->kernel_addr = hdr->kernel_addr;
    img->ramdisk.size = hdr->ramdisk_size;
    img->ramdisk_addr = hdr->ramdisk_addr;
    img->dt.size = hdr->second_size;
    img->dt_addr = hdr->second_addr;
    img->tags_addr = hdr->tags_addr;
    img->page_size = hdr->page_size;
    strncpy(img->name, hdr->name, BOOT_NAME_SIZE);
    img->name[BOOT_NAME_SIZE-1] = '\0'; // ensure null-terminated
    strncpy(img->cmdline, hdr->cmdline, BOOT_ARGS_SIZE);

    const char *ptr = img->image.data + img->page_size;
    if (img->kernel.size) {
        img->kernel.data = ptr;
        ptr += ROUND_PAGE(img->kernel.size, img->page_size);
    }
    if (img->ramdisk.size) {
        img->ramdisk.data = ptr;
        ptr += ROUND_PAGE(img->ramdisk.size, img->page_size);
    }
    if (img->dt.size) {
        img->dt.data = ptr;
        ptr += ROUND_PAGE(img->dt.size, img->page_size);
    }

    unsigned req_size = ptr - img->image.data;
    if (req_size > img->image.size) {
        fprintf(stderr, "Image too small, need %u bytes, but got %u.\n",
                req_size, img->image.size);
        return -1;
    }

    return 0;
}

int fsl_write(struct bootimg *img, int fd) {
    if (img->second.size)
        fprintf(stderr, "Warning: fsl variant does not support second stage bootloader, ignoring.\n");

    struct boot_img_hdr hdr;
    memset(&hdr, 0, sizeof(boot_img_hdr));
    memcpy(hdr.magic, BOOT_MAGIC, BOOT_MAGIC_SIZE);
    hdr.kernel_size = img->kernel.size;
    hdr.kernel_addr = img->kernel_addr;
    hdr.ramdisk_size = img->ramdisk.size;
    hdr.ramdisk_addr = img->ramdisk_addr;
    hdr.second_size = img->dt.size;
    hdr.second_addr = img->dt_addr;
    hdr.tags_addr = img->tags_addr;
    hdr.page_size = img->page_size;
    memcpy(hdr.name, img->name, BOOT_NAME_SIZE);
    strncpy(hdr.cmdline, img->cmdline, BOOT_ARGS_SIZE - 1);
    hdr.cmdline[BOOT_ARGS_SIZE - 1] = '\0';
    if (strlen(img->cmdline) >= (BOOT_ARGS_SIZE - 1))
        fprintf(stderr, "Warning: cmdline too long (got %lu, max %d), chopped\n",
                strlen(img->cmdline), BOOT_ARGS_SIZE - 1);

    sha_ctx hash;
    sha_init(&hash);
    sha_update(&hash, img->kernel.data, img->kernel.size);
    sha_update(&hash, &img->kernel.size, sizeof(img->kernel.size));
    sha_update(&hash, img->ramdisk.data, img->ramdisk.size);
    sha_update(&hash, &img->ramdisk.size, sizeof(img->ramdisk.size));
    sha_update(&hash, img->dt.data, img->dt.size);
    sha_update(&hash, &img->dt.size, sizeof(img->dt.size));
    char digest[SHA_DIGEST_LENGTH];
    sha_final(&hash, digest);
    memcpy(hdr.id, digest,
           SHA_DIGEST_LENGTH > sizeof(hdr.id) ? sizeof(hdr.id) : SHA_DIGEST_LENGTH);

    if (io_write_padded(fd, &hdr, sizeof(boot_img_hdr), img->page_size) < 0 ||
        io_write_padded(fd, img->kernel.data, img->kernel.size, img->page_size) < 0 ||
        io_write_padded(fd, img->ramdisk.data, img->ramdisk.size, img->page_size) < 0 ||
        io_write_padded(fd, img->dt.data, img->dt.size, img->page_size) < 0) {
        perror(img->image.name);
        return -1;
    }

    return 0;
}

struct variant variant_fsl = {
    .name = "fsl",
    .description = "Freescale with device tree in place of second stage bootloader",
    .read = fsl_read,
    .write = fsl_write,
};
