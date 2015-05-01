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
#include <errno.h>

#include "bootimgtool.h"

struct variant *variants[] = {
    &variant_standard,
    &variant_qcom,
    &variant_fsl,
    NULL
};

/**
 * The program name as a global variable, set by main.
 */
const char *progname;

/**
 * Read img->image, interpret header, and fill relevant fields in img.
 * Exit on error.
 */
static void bootimg_read_image(struct bootimg *img, struct variant *var) {
    if (iomap_open(&img->image) < 0) {
        perror(img->image.name);
        exit(EXIT_FAILURE);
    }

    if (var->read(img) < 0)
        exit(EXIT_FAILURE);
}

/**
 * Write a new image file to img->image.name.
 * Exit on error.
 */
static void bootimg_write_image(struct bootimg *img, struct variant *var) {
    int fd = io_open_write(img->image.name);
    if (fd == -1) {
        perror(img->image.name);
        exit(EXIT_FAILURE);
    }

    if (var->write(img, fd) < 0)
        exit(EXIT_FAILURE);

    if (close(fd) < 0) {
        perror(img->image.name);
        exit(EXIT_FAILURE);
    }
}

/**
 * Read img->params and fill relevant fields in img.
 * Exit on error.
 */
static void bootimg_read_params(struct bootimg *img) {
    char *content = io_read_text(img->params.name);
    if (content == NULL) {
        perror(img->params.name);
        exit(EXIT_FAILURE);
    }

    char *ptr = content, *ptr2;
    char *key = NULL, *value = NULL;
    int lineno = 0;
    while (*ptr != '\0') {
        lineno++;

        // Leading whitespace
        ptr += strspn(ptr, " \t");
        if (*ptr == '\n') {
            ptr++;
            continue;
        }
        // Key
        key = ptr;
        if (*key == '=') {
            fprintf(stderr, "%s:%d: empty key, skipping line\n",
                    img->params.name, lineno);
            ptr = strchr(ptr, '\n') + 1;
            continue;
        }
        // Delimiter, and trailing key whitespace
        ptr = strpbrk(ptr, "=\n");
        if (*ptr == '\n') {
            fprintf(stderr, "%s:%d: invalid syntax, skipping line\n",
                    img->params.name, lineno);
            ptr++;
            continue;
        }
        ptr2 = ptr++;
        *(ptr2--) = '\0';
        while (ptr2 > key && (*ptr2 == ' ' || *ptr2 == '\t'))
            *(ptr2--) = '\0';
        // Leading value whitespace
        ptr += strspn(ptr, " \t");
        // Value, and trailing value whitespace
        value = ptr;
        ptr = strchr(ptr, '\n');
        ptr2 = ptr++;
        *(ptr2--) = '\0';
        while (ptr2 > value && (*ptr2 == ' ' || *ptr2 == '\t'))
            *(ptr2--) = '\0';

        // Interpret key and value
        if (strcmp(key, "page_size") == 0) {
            img->page_size = strtoul(value, NULL, 0);
        } else if(strcmp(key, "kernel_addr") == 0) {
            img->kernel_addr = strtoul(value, NULL, 0);
        } else if(strcmp(key, "ramdisk_addr") == 0) {
            img->ramdisk_addr = strtoul(value, NULL, 0);
        } else if(strcmp(key, "second_addr") == 0) {
            img->second_addr = strtoul(value, NULL, 0);
        } else if(strcmp(key, "dt_addr") == 0 ||
                  strcmp(key, "devicetree_addr") == 0) {
            img->dt_addr = strtoul(value, NULL, 0);
        } else if(strcmp(key, "tags_addr") == 0) {
            img->tags_addr = strtoul(value, NULL, 0);
        } else if(strcmp(key, "name") == 0) {
            if (strlen(value) >= BOOT_NAME_SIZE)
                fprintf(stderr, "%s:%d: name too long, chopped\n",
                        img->params.name, lineno);
            strncpy(img->name, value, BOOT_NAME_SIZE - 1);
            img->name[BOOT_NAME_SIZE - 1] = '\0';
        } else if(strcmp(key, "cmdline") == 0) {
            if (strlen(value) >= MAX_CMDLINE_SIZE)
                fprintf(stderr, "%s:%d: cmdline too long, chopped\n",
                        img->params.name, lineno);
            strncpy(img->cmdline, value, MAX_CMDLINE_SIZE - 1);
            img->cmdline[MAX_CMDLINE_SIZE - 1] = '\0';
        } else {
            fprintf(stderr, "%s:%d: unknown key '%s', skipping line\n",
                    img->params.name, lineno, key);
        }
    }

    free(content);
}

/**
 * Write img parameters to img->params.name.
 * Exit on error.
 */
static void bootimg_write_params(struct bootimg *img) {
    int fd = io_open_write(img->params.name);
    if (fd < 0) {
        perror(img->params.name);
        exit(EXIT_FAILURE);
    }

    FILE *f = fdopen(fd, "w");
    fprintf(f, "page_size = %u\n", img->page_size);
    if (img->kernel_addr)
        fprintf(f, "kernel_addr = 0x%08x\n", img->kernel_addr);
    if (img->ramdisk_addr)
        fprintf(f, "ramdisk_addr = 0x%08x\n", img->ramdisk_addr);
    if (img->second_addr)
        fprintf(f, "second_addr = 0x%08x\n", img->second_addr);
    if (img->dt_addr)
        fprintf(f, "dt_addr = 0x%08x\n", img->dt_addr);
    if (img->tags_addr)
        fprintf(f, "tags_addr = 0x%08x\n", img->tags_addr);
    if (img->name[0])
        fprintf(f, "name = %s\n", img->name);
    if (img->cmdline[0])
        fprintf(f, "cmdline = %s\n", img->cmdline);

    if (fclose(f) != 0) {
        perror(img->params.name);
        exit(EXIT_FAILURE);
    }
}

/**
 * Read a single part.  Exit on error.
 * Silently ignore inexistent files (set size to 0).
 */
static inline void read_iomap(struct iomap *f) {
    f->size = 0;
    if (iomap_open(f) < 0 && errno != ENOENT) {
        perror(f->name);
        exit(EXIT_FAILURE);
    }
}

/**
 * Read kernel, ramdisk, second, and/or dt image files.
 * Exit on error.
 */
static void bootimg_read_parts(struct bootimg *img) {
    read_iomap(&img->kernel);
    read_iomap(&img->ramdisk);
    read_iomap(&img->second);
    read_iomap(&img->dt);
}

/**
 * Extract a single part.  Exit on error.
 */
static inline void extract_iomap(const struct iomap *f) {
    if (f->size) {
        if (iomap_save(f) < 0) {
            perror(f->name);
            exit(EXIT_FAILURE);
        }
    }
}

/**
 * Extract kernel, ramdisk, second, and/or dt parts in img.
 * Exit on error.
 */
static void bootimg_extract_parts(struct bootimg *img) {
    extract_iomap(&img->kernel);
    extract_iomap(&img->ramdisk);
    extract_iomap(&img->second);
    extract_iomap(&img->dt);
}

/**
 * Print information about the boot image.
 */
static void bootimg_print_info(struct bootimg *img) {
    printf("Image size: %u\n", img->image.size);
    printf("Page size: %u\n", img->page_size);
    printf("Kernel size:       %u\n", img->kernel.size);
    printf("Ramdisk size:      %u\n", img->ramdisk.size);
    printf("Second stage size: %u\n", img->second.size);
    printf("Device tree size:  %u\n", img->dt.size);
    printf("Kernel load address:       0x%08x\n", img->kernel_addr);
    printf("Ramdisk load address:      0x%08x\n", img->ramdisk_addr);
    printf("Second stage load address: 0x%08x\n", img->second_addr);
    printf("Device tree load address:  0x%08x\n", img->dt_addr);
    printf("Tags load address:         0x%08x\n", img->tags_addr);
    printf("Product name: %s\n", img->name);
    printf("Command line: %s\n", img->cmdline);
}

/**
 * Initialize bootimg with default values.
 */
static void init_bootimg(struct bootimg *img) {
    memset(img, 0, sizeof(struct bootimg));
    img->params.name = "parameters.cfg";
    img->kernel.name = "zImage";
    img->ramdisk.name = "ramdisk.img";
    img->second.name = "second.img";
    img->dt.name = "dt.img";
}

/**
 * Print usage information to stderr.
 */
static void print_usage() {
    fprintf(stderr, "Usage: %s [options] <action> <bootimg>\n\n", progname);
    fprintf(stderr, "Actions:\n"
                    "  -i, --info                Print information about bootimg\n"
                    "  -x, --extract             Extract bootimg\n"
                    "  -c, --create              Assemble bootimg\n"
                    "  -h, --help                Print this help message and exit\n"
                    "\n"
                    "Options:\n"
                    "  -p, --parameters=FILE     Read/Write parameters from/to FILE\n"
                    "  -k, --kernel=FILE         Read/Write kernel image from/to FILE\n"
                    "  -r, --ramdisk=FILE        Read/Write ramdisk image from/to FILE\n"
                    "  -s, --second=FILE         Read/Write second stage image from/to FILE\n"
                    "  -d, --dt, --devicetree=FILE  Read/Write device tree from/to FILE\n"
                    "  -v, --variant=VARIANT     Select format variant VARIANT\n"
                    "  -f, --force               Overwrite files without asking\n");

    fprintf(stderr, "\nDefault file names:\n");
    struct bootimg defaults;
    init_bootimg(&defaults);
    fprintf(stderr, "  kernel: %s\n", defaults.kernel.name);
    fprintf(stderr, "  ramdisk: %s\n", defaults.ramdisk.name);
    fprintf(stderr, "  second: %s\n", defaults.second.name);
    fprintf(stderr, "  devicetree: %s\n", defaults.dt.name);

    fprintf(stderr, "\nVariants:\n");
    struct variant **var = variants;
    while (*var != NULL) {
        fprintf(stderr, "  %s: %s\n", (*var)->name, (*var)->description);
        var++;
    }
}

/**
 * Show an error message and usage notice, and exit with status EXIT_FAILURE.
 */
static void exit_usage_error(const char *fmt, ...) {
    va_list ap;
    fprintf(stderr, "%s: ", progname);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    print_usage();
    exit(EXIT_FAILURE);
}

/**
 * Parse arguments.  Exit on error.
 *
 * @param argc [in] Number of arguments.
 * @param argv [in] Arguments.
 * @param action [out] Requested action.
 * @param var [out] Requested variant.
 * @param img [out] Bootimg.
 */
static void parse_args(int argc, char *argv[], enum action *action,
                       struct variant **var, struct bootimg *img) {
    struct option longopts[] = {
        {"info",       no_argument,       NULL, 'i'},
        {"extract",    no_argument,       NULL, 'x'},
        {"create",     no_argument,       NULL, 'c'},
        {"parameters", required_argument, NULL, 'p'},
        {"kernel",     required_argument, NULL, 'k'},
        {"ramdisk",    required_argument, NULL, 'r'},
        {"second",     required_argument, NULL, 's'},
        {"dt",         required_argument, NULL, 'd'},
        {"devicetree", required_argument, NULL, 'd'},
        {"variant",    required_argument, NULL, 'v'},
        {"force",      no_argument,       NULL, 'f'},
        {"help",       no_argument,       NULL, 'h'},
        {NULL,         0,                 NULL, 0  },
    };
    int c;
    struct variant **v;

    while ((c = getopt_long(argc, argv, "ixcp:k:r:s:d:v:fh", longopts, NULL)) != -1) {
        switch (c) {
        case 'i': *action = ACTION_INFO;            break;
        case 'x': *action = ACTION_EXTRACT;         break;
        case 'c': *action = ACTION_CREATE;          break;
        case 'p': img->params.name = optarg;        break;
        case 'k': img->kernel.name = optarg;        break;
        case 'r': img->ramdisk.name = optarg;       break;
        case 's': img->second.name = optarg;        break;
        case 'd': img->dt.name = optarg;            break;
        case 'v':
            v = variants;
            while (*v != NULL) {
                if (strcmp(optarg, (*v)->name) == 0)
                    break;
                v++;
            }
            if (*v == NULL)
                exit_usage_error("unknown variant '%s'\n", optarg);
            *var = *v;
            break;
        case 'f': io_force = true;                  break;
        case 'h':
            print_usage();
            exit(EXIT_SUCCESS);
        default: // '?'
            print_usage();
            exit(EXIT_FAILURE);
        }
    }

    if (optind == argc)
        exit_usage_error("missing bootimg\n");
    if (optind < argc - 1)
        exit_usage_error("too many arguments\n");
    img->image.name = argv[optind];
}

int main(int argc, char *argv[]) {
    enum action action = ACTION_UNDEFINED;
    struct variant *var = variants[0];
    struct bootimg img;

    progname = argv[0];
    init_bootimg(&img);
    parse_args(argc, argv, &action, &var, &img);

    switch (action) {
    case ACTION_INFO:
        bootimg_read_image(&img, var);
        bootimg_print_info(&img);
        break;
    case ACTION_EXTRACT:
        bootimg_read_image(&img, var);
        bootimg_write_params(&img);
        bootimg_extract_parts(&img);
        break;
    case ACTION_CREATE:
        bootimg_read_params(&img);
        bootimg_read_parts(&img);
        bootimg_write_image(&img, var);
        break;
    default:
        exit_usage_error("missing action\n");
    }

    return EXIT_SUCCESS;
}
