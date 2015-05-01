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

#include "io.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

// Global variable definition
bool io_force = false;

char *io_read_text(const char *name) {
    struct stat sb;
    int fd, prev_errno;
    char *buf = NULL, *ptr;

    fd = open(name, O_RDONLY);
    if (fd == -1)
        return NULL;
    if (fstat(fd, &sb) == -1)
        goto done;

    buf = malloc(sb.st_size + 2);
    if (buf == NULL)
        goto done;

    if (read(fd, buf, sb.st_size) != sb.st_size) {
        free(buf);
        buf = NULL;
        goto done;
    }

    ptr = buf + sb.st_size;
    if (sb.st_size > 0 && ptr[-1] != '\n')
        *(ptr++) = '\n';
    *ptr = '\0';

done:
    prev_errno = errno;
    close(fd);
    errno = prev_errno;
    return buf;
}

int io_open_write(const char *name) {
    if (!io_force && access(name, F_OK) == 0) {
        fprintf(stderr, "Overwrite '%s'? [y/N] ", name);
        int c = getchar();
        bool overwrite = (c == 'y' || c == 'Y');
        while (c != '\n' && c != EOF)
            c = getchar();
        if (!overwrite) {
            errno = EEXIST;
            return -1;
        }
    }
    return creat(name, 0666);
}

int io_write_padded(int fd, const void *data, unsigned size, unsigned pagesize) {
    unsigned padsize;
    char *padding;
    int ret;

    if (write(fd, data, size) != size)
        return -1;

    padsize = pagesize - (size % pagesize);
    if (padsize == pagesize)
        return 0;

    padding = calloc(padsize, 1);
    ret = write(fd, padding, padsize);
    free(padding);
    return ret;
}

int iomap_open(struct iomap *f) {
    struct stat sb;
    int prev_errno;

    f->fd = open(f->name, O_RDONLY);
    if (f->fd == -1)
        return -1;

    if (fstat(f->fd, &sb) == -1)
        goto err;
    f->size = sb.st_size;

    f->data = mmap(NULL, f->size, PROT_READ, MAP_SHARED, f->fd, 0);
    if (f->data == MAP_FAILED)
        goto err;

    return 0;

err:
    prev_errno = errno;
    close(f->fd);
    errno = prev_errno;
    return -1;
}

int iomap_close(struct iomap *f) {
    munmap((void*) f->data, f->size);
    f->data = NULL;
    return close(f->fd);
}

int iomap_save(const struct iomap *f) {
    int fd, prev_errno;
    ssize_t written;

    fd = io_open_write(f->name);
    if (fd == -1)
        return -1;

    written = write(fd, f->data, f->size);

    prev_errno = errno;
    close(fd);
    errno = prev_errno;

    return (written == f->size) ? 0 : -1;
}
