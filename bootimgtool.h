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

#ifndef BOOTIMGTOOL_H
#define BOOTIMGTOOL_H

#include "bootimg.h"
#include "io.h"

#define MAX_CMDLINE_SIZE (BOOT_ARGS_SIZE + BOOT_EXTRA_ARGS_SIZE)

#define ROUND_PAGE(size, pagesize) \
    ((((size) + (pagesize) - 1) / (pagesize)) * (pagesize))

enum action {
    ACTION_UNDEFINED,
    ACTION_INFO,
    ACTION_EXTRACT,
    ACTION_CREATE,
};

struct bootimg {
    struct iomap image;

    struct iomap params;

    struct iomap kernel;
    struct iomap ramdisk;
    struct iomap second;
    struct iomap dt;

    unsigned kernel_addr;
    unsigned ramdisk_addr;
    unsigned second_addr;
    unsigned dt_addr;
    unsigned tags_addr;

    unsigned page_size;

    char name[BOOT_NAME_SIZE];
    char cmdline[MAX_CMDLINE_SIZE];
};

struct variant {
    /**
     * Name of the variant as given in arguments.
     */
    const char *name;

    /**
     * Short description for inclusion in the help message.
     */
    const char *description;

    /**
     * Read img->image, interpret header, and fill relevant fields in img.
     * In particular, img->{kernel,ramdisk,second,dt}.data should point to the
     * corresponding part in img->image.data.
     *
     * @param img A boot image with img->image.name filled.
     * @return 0 on success, -1 on error (an error message should have been
     *         written on stderr)
     */
    int (*read)(struct bootimg *img);

    /**
     * Create a new image to img->image.name with the information from the
     * other fields.
     *
     * @param img A complete boot image.
     * @param fd File descriptor to write to.
     * @return 0 on success, -1 on error (an error message should have been
     *         written on stderr)
     */
    int (*write)(struct bootimg *img, int fd);
};

// Implemented variants
extern struct variant variant_standard;

#endif // BOOTIMGTOOL_H

