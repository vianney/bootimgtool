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

#ifndef IO_H
#define IO_H

#include <stdbool.h>

/**
 * If this global variable is true, files will be overwritten without asking
 * user for confirmation.
 */
extern bool io_force;

/**
 * Read the complete contents of a text file.
 * For easier parsing, this function ensures that the contents terminate with
 * a newline, adding one if needed.
 *
 * @param name The name of the file to read.
 * @return A null-terminated string with the content or NULL on error (read
 *         errno for reason).  Caller is responsible for freeing the string.
 */
char *io_read_text(const char *name);

/**
 * Open a file for writing. Ask user for confirmation if the file exists, unless
 * io_force == 1.
 *
 * @param name The name of the file to open.
 * @return The file descriptor of the open file, or -1 on error (read errno for
 *         reason).
 */
int io_open_write(const char *name);

/**
 * Write data to an open file descriptor, padding with 0 to pagesize.
 *
 * @param fd File descriptor opened in write mode.
 * @param data Data to write.
 * @param size Number of bytes from data to write.
 * @param pagesize Size of a page.
 * @return 0 on success, -1 on error (read errno for reason).
 */
int io_write_padded(int fd, const void *data, unsigned size, unsigned pagesize);

/**
 * The iomap structure is used in two ways:
 *
 * 1) With iomap_open and iomap_close to open a file in read-only mode, mapping
 *    the whole contents to the data field with mmap.
 *
 * 2) With iomap_save to write the contents of data to a file named name.
 */
struct iomap {
    const char *name;
    int fd;
    const char *data;
    unsigned size;
};

/**
 * Open file in read-only and map contents to f->data.
 *
 * @param f The file to open (only f->name must be initialized).
 * @return 0 on success, -1 on error (read errno for reason).
 */
int iomap_open(struct iomap *f);

/**
 * Close an open file.
 *
 * @param f The file (f->fd must be valid).
 * @return 0 on success, -1 on error (read errno for reason).
 */
int iomap_close(struct iomap *f);

/**
 * Write f->data to a disk file named f->name.
 *
 * @param f Data to write (f->data and f->size) and filename (f->name).
 * @return 0 on success, -1 on error (read errno for reason).
 */
int iomap_save(const struct iomap *f);

#endif // IO_H

