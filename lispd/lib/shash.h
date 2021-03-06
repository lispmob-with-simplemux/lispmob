/*
 * shash.h
 *
 * This file is part of LISP Mobile Node Implementation.
 *
 * Copyright (C) 2014 Universitat Politècnica de Catalunya.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Please send any bug reports or fixes you make to the email address(es):
 *    LISP-MN developers <devel@lispmob.org>
 *
 * Written or modified by:
 *    Florin Coras <fcoras@ac.upc.edu>
 */

#ifndef SHASH_H_
#define SHASH_H_

#include "../elibs/khash/khash.h"
#include "generic_list.h"

//KHASH_MAP_INIT_STR(str, void *)
KHASH_INIT(str, char *, void *, 1, kh_str_hash_func, kh_str_hash_equal)

/* Prototype for a pointer to a free key function. */
typedef void (*free_key_fn_t)(const void *key);


typedef struct shash {
    khash_t(str) *htable;
    free_key_fn_t free_key_fn;
} shash_t;




shash_t *shash_new();
shash_t *shash_new_managed(free_key_fn_t df);
void shash_del(shash_t *);
void shash_insert(shash_t *, char *,  void *);
void shash_remove(shash_t *, char *);
void *shash_lookup(shash_t *, char *);
void shash_destroy(shash_t *sh);
glist_t *shash_keys(shash_t *sh);
glist_t *shash_values(shash_t *sh);



#endif /* SHASH_H_ */
