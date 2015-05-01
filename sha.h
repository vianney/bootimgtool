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

#ifndef SHA_H
#define SHA_H

#include "config.h"

#define SHA_DIGEST_SIZE 20

#ifdef HAVE_LIBCRYPTO
#include <openssl/sha.h>
typedef SHA_CTX sha_ctx;
#else
typedef struct {} sha_ctx;
#endif

/**
 * Initialize sha_ctx.
 */
int sha_init(sha_ctx *ctx);

/**
 * Provide chunk of data to hash.
 */
int sha_update(sha_ctx *ctx, const void *data, size_t len);

/**
 * Place hash in digest, which must be large enough (at least SHA_DIGEST_SIZE
 * bytes).
 */
int sha_final(sha_ctx *ctx, char *digest);


#endif // SHA_H

