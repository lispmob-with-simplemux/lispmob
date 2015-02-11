/*
 * lisp_addr.h
 *
 * This file is part of LISP Mobile Node Implementation.
 *
 * Copyright (C) 2014 Universitat Politècnica de Catalunya.
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

#ifndef LISPD_ADDRESS_H_
#define LISPD_ADDRESS_H_

#include "lisp_ip.h"
#include "lisp_lcaf.h"
#include <lisp_messages.h>


/*
 * Lisp address structure
 */


typedef enum {
    LM_AFI_NO_ADDR = 0,
    LM_AFI_IP,
    LM_AFI_IPPREF,
    LM_AFI_LCAF,
} lm_afi_t;


/* TODO fcoras: The cool thing about the new lisp_addr_t
 * is that we can access in 2 ways the same data
 * either as old struct or as ip_addr_t. Still, would be nice
 * to deprecate the old struct
 */

typedef struct _lisp_addr_t lisp_addr_t;
//typedef struct _lcaf_addr_t lcaf_addr_t;

struct _lisp_addr_t {
    union {
        struct {
            int  afi;
            union {
                struct in_addr   ip;
                struct in6_addr  ipv6;
            } address;
        };
        struct {
            union {
                ip_addr_t       ip;
                ip_prefix_t     ippref;
                lcaf_addr_t     lcaf;
            };
            lm_afi_t        lafi;
        };
    };
};



inline lisp_addr_t *lisp_addr_new();
inline lisp_addr_t *lisp_addr_new_lafi(uint8_t lafi);
inline void lisp_addr_del(lisp_addr_t *laddr);
void lisp_addr_dealloc(lisp_addr_t *addr);
void lisp_addr_copy(lisp_addr_t *dst, lisp_addr_t *src);
lisp_addr_t *lisp_addr_clone(lisp_addr_t *src);
inline uint32_t lisp_addr_copy_to(void *dst, lisp_addr_t *src);
inline int lisp_addr_write(void *offset, lisp_addr_t *laddr);
int lisp_addr_parse(uint8_t *offset, lisp_addr_t *laddr);
inline int lisp_addr_cmp(lisp_addr_t *addr1, lisp_addr_t *addr2);
inline uint32_t lisp_addr_size_to_write(lisp_addr_t *laddr);
char *lisp_addr_to_char(lisp_addr_t *addr);

inline void lisp_addr_set_lafi(lisp_addr_t *addr, lm_afi_t afi);
inline void lisp_addr_set_lcaf(lisp_addr_t *laddr, lcaf_addr_t *lcaf);
inline void lisp_addr_set_plen(lisp_addr_t *laddr, uint8_t plen);
inline void lisp_addr_set_ip(lisp_addr_t *addr, ip_addr_t *ip);

static inline lm_afi_t lisp_addr_lafi(lisp_addr_t *addr)
{
    return (addr->lafi);
}

static inline ip_addr_t *lisp_addr_ip(lisp_addr_t *addr)
{
    return (&addr->ip);
}

static inline ip_prefix_t *lisp_addr_get_ippref(lisp_addr_t *addr)
{
    return (&addr->ippref);
}

static inline lcaf_addr_t *lisp_addr_get_lcaf(lisp_addr_t *addr)
{
    return (&addr->lcaf);
}

static inline int lisp_addr_is_ip(lisp_addr_t *addr)
{
    return (addr->lafi == LM_AFI_IP);
}

static inline int lisp_addr_is_ip_pref(lisp_addr_t *addr)
{
    return (addr->lafi == LM_AFI_IPPREF);
}

static inline int lisp_addr_is_no_addr(lisp_addr_t *addr)
{
    return (addr->lafi == LM_AFI_NO_ADDR);
}
static inline int lisp_addr_is_lcaf(lisp_addr_t *addr)
{
    return (addr->lafi == LM_AFI_LCAF);
}

inline uint16_t lisp_addr_ip_afi(lisp_addr_t *addr);
inline ip_addr_t *lisp_addr_ip_get_addr(lisp_addr_t *laddr);
inline uint8_t lisp_addr_ip_get_plen(lisp_addr_t *laddr);
inline void lisp_addr_ip_set_afi(lisp_addr_t *laddr, int afi);
void lisp_addr_set_ip_afi(lisp_addr_t *la, int afi);
inline void lisp_addr_ip_to_ippref(lisp_addr_t *laddr);
inline void lisp_addr_ip_init(lisp_addr_t *addr, void *data, int afi);


inline int lisp_addr_is_lcaf(lisp_addr_t *laddr);
inline void lisp_addr_lcaf_set_addr(lisp_addr_t *laddr, void *addr);
inline void *lisp_addr_lcaf_addr(lisp_addr_t *laddr);
inline lcaf_type_e lisp_addr_lcaf_type(lisp_addr_t *laddr);
inline void lisp_addr_lcaf_set_type(lisp_addr_t *laddr, int type);

inline int lisp_addr_init_from_ip(lisp_addr_t *, ip_addr_t *);
inline int lisp_addr_init_from_ippref(lisp_addr_t *, ip_addr_t *, uint8_t);
inline int lisp_addr_init_from_lcaf(lisp_addr_t *, lcaf_addr_t *);

inline uint16_t lisp_addr_iana_afi_to_lm_afi(uint16_t afi);
inline uint16_t lisp_addr_get_iana_afi(lisp_addr_t *laddr);
inline uint16_t lisp_addr_get_plen(lisp_addr_t *laddr);

inline int lisp_addr_is_mc(lisp_addr_t *addr);
lisp_addr_t *lisp_addr_get_ip_addr(lisp_addr_t *addr);
lisp_addr_t *lisp_addr_get_fwd_ip_addr(lisp_addr_t *addr, glist_t *locl_rlocs_addr);

int lisp_addr_ip_from_char(char *, lisp_addr_t *);
int lisp_addr_ippref_from_char(char *, lisp_addr_t *);

inline int lisp_addr_ip_afi_lcaf_type(lisp_addr_t *addr);


#endif /* LISPD_ADDRESS_H_ */