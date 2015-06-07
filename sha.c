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

#include "sha.h"

int sha_init(sha_ctx *ctx) {
    return SHA1_Init(ctx);
}

int sha_update(sha_ctx *ctx, const void *data, size_t len) {
    return SHA1_Update(ctx, data, len);
}

int sha_final(sha_ctx *ctx, char *digest) {
    return SHA1_Final((unsigned char *) digest, ctx);
}
