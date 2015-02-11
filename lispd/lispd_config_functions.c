/*
 * lispd_config_functions.c
 *
 * This file is part of LISP Mobile Node Implementation.
 * Handle lispd command line and config file
 * Parse command line args using gengetopt.
 * Handle config file with libconfuse.
 *
 * Copyright (C) 2011 Cisco Systems, Inc, 2011. All rights reserved.
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
 *    Alberto López     <alopez@ac.upc.edu>
 *
 */

#include <netdb.h>

#include "lispd_config_functions.h"
#include "lispd_lib.h"
#include "lmlog.h"

/***************************** FUNCTIONS DECLARATION *************************/
glist_t *fqdn_to_addresses(
        char        *addr_str,
        const int   preferred_afi);
/********************************** FUNCTIONS ********************************/

no_addr_loct *
no_addr_loct_new_init(
        locator_t * loct,
        char *      iface,
        int         afi)
{
    no_addr_loct * nloct = NULL;

    nloct = (no_addr_loct *)xzalloc(sizeof(no_addr_loct));
    if (nloct == NULL){
        return (NULL);
    }
    nloct->locator = loct;
    nloct->iface_name = strdup(iface);
    nloct->afi = afi;
    return (nloct);
}

void
no_addr_loct_del(no_addr_loct * nloct)
{
    free(nloct->iface_name);
    free(nloct);
}

no_addr_loct *
get_no_addr_loct_from_list(
        glist_t     *list,
        locator_t   *locator)
{
    glist_entry_t * it      = NULL;
    no_addr_loct *  nloct   = NULL;

    glist_for_each_entry(it,list){
        nloct = (no_addr_loct *)glist_entry_data(it);
        /* Comparing memory position */
        if (nloct->locator == locator){
            return (nloct);
        }
    }

    return (NULL);
}
void
validate_rloc_probing_parameters(
        int *interval,
        int *retries,
        int *retries_int)
{

    if (*interval < 0) {
        *interval = 0;
    }

    if (*interval > 0) {
        LMLOG(DBG_1, "RLOC Probing Interval: %d", *interval);
    } else {
        LMLOG(DBG_1, "RLOC Probing disabled");
    }

    if (*interval != 0) {
        if (*retries > LISPD_MAX_RETRANSMITS) {
            *retries = LISPD_MAX_RETRANSMITS;
            LMLOG(LWRN, "RLOC Probing retries should be between 0 and %d. "
                    "Using %d retries", LISPD_MAX_RETRANSMITS,
                    LISPD_MAX_RETRANSMITS);
        } else if (*retries < 0) {
            *retries = 0;
            LMLOG(LWRN, "RLOC Probing retries should be between 0 and %d. "
                    "Using 0 retries", LISPD_MAX_RETRANSMITS);
        }

        if (*retries > 0) {
            if (*retries_int < LISPD_MIN_RETRANSMIT_INTERVAL) {
                *retries_int = LISPD_MIN_RETRANSMIT_INTERVAL;
                LMLOG(LWRN, "RLOC Probing interval retries should be between "
                        "%d and RLOC Probing interval. Using %d seconds",
                        LISPD_MIN_RETRANSMIT_INTERVAL,
                        LISPD_MIN_RETRANSMIT_INTERVAL);
            } else if (*retries_int > *interval) {
                *retries_int = *interval;
                LMLOG(LWRN, "RLOC Probing interval retries should be between "
                        "%d and RLOC Probing interval. Using %d seconds",
                         LISPD_MIN_RETRANSMIT_INTERVAL, *interval);
            }
        }
    }
}

int
validate_priority_weight(int p, int w)
{
    /* Check the parameters */
    if (p < (MAX_PRIORITY - 1)|| p > UNUSED_RLOC_PRIORITY) {
        LMLOG(LERR, "Configuration file: Priority %d out of range [%d..%d]",
                p, MAX_PRIORITY, MIN_PRIORITY);
        return (BAD);
    }

    if (w > MAX_WEIGHT || w < MIN_WEIGHT) {
        LMLOG(LERR, "Configuration file: Weight %d out of range [%d..%d]",
                p, MIN_WEIGHT, MAX_WEIGHT);
        return (BAD);
    }
    return (GOOD);
}

/*
 *  add a map-resolver to the list
 */

int
add_server(
        char                *str_addr,
        glist_t             *list)
{
    lisp_addr_t *       addr        = NULL;
    glist_t *           addr_list   = NULL;
    glist_entry_t *     it          = NULL;

    addr_list = parse_ip_addr(str_addr);

    if (addr_list == NULL){
        LMLOG(LERR, "Error parsing address. Ignoring server with address %s",
                        str_addr);
        return (BAD);
    }
    glist_for_each_entry(it, addr_list) {
        addr = glist_entry_data(it);

        /* Check that the afi of the map server matches with the default rloc afi
         * (if it's defined). */
        if (default_rloc_afi != AF_UNSPEC && default_rloc_afi != lisp_addr_ip_afi(addr)){
            LMLOG(LWRN, "The server %s will not be added due to the selected "
                    "default rloc afi (-a option)", str_addr);
            continue;
        }
        glist_add_tail(lisp_addr_clone(addr), list);
        LMLOG(DBG_3,"The server %s has been added to the list",lisp_addr_to_char(addr));
    }

    glist_destroy(addr_list);


    return(GOOD);
}


int
add_map_server(
        lisp_xtr_t *    xtr,
        char *          str_addr,
        int             key_type,
        char *          key,
        uint8_t         proxy_reply)
{
    lisp_addr_t *       addr        = NULL;
    map_server_elt *    ms          = NULL;
    glist_t *           addr_list   = NULL;
    glist_entry_t *     it          = NULL;

    if (str_addr == NULL || key_type == 0 || key == NULL){
        LMLOG(LERR, "Configuraton file: Wrong Map Server configuration. "
                "Check configuration file");
        exit_cleanup();
    }

    if (key_type != HMAC_SHA_1_96){
        LMLOG(LERR, "Configuraton file: Only SHA-1 (1) authentication is supported");
        exit_cleanup();
    }

    addr_list = parse_ip_addr(str_addr);

    if (addr_list == NULL){
        LMLOG(LERR, "Error parsing address. Ignoring Map Server %s",
                        str_addr);
        return (BAD);
    }
    glist_for_each_entry(it, addr_list) {
        addr = glist_entry_data(it);

        /* Check that the afi of the map server matches with the default rloc afi
         * (if it's defined). */
        if (default_rloc_afi != AF_UNSPEC && default_rloc_afi != lisp_addr_ip_afi(addr)){
            LMLOG(LWRN, "The map server %s will not be added due to the selected "
                    "default rloc afi (-a option)", str_addr);
            continue;
        }
        // XXX Create method to do it authomatically
        ms = xzalloc(sizeof(map_server_elt));

        ms->address     = lisp_addr_clone(addr);
        ms->key_type    = key_type;
        ms->key         = strdup(key);
        ms->proxy_reply = proxy_reply;

        glist_add(ms, xtr->map_servers);
    }

    glist_destroy(addr_list);

    return(GOOD);
}


int
add_proxy_etr_entry(
        lisp_xtr_t *    xtr,
        char *          str_addr,
        int             priority,
        int             weight)
{
    glist_t *           addr_list   = NULL;
    glist_entry_t *     it          = NULL;
    lisp_addr_t *       addr        = NULL;
    locator_t *         locator     = NULL;

    if (str_addr == NULL){
        LMLOG(LERR, "Configuration file: No interface specified for PETR. "
                "Discarding!");
        return (BAD);
    }

    if (validate_priority_weight(priority, weight) != GOOD) {
        return(BAD);
    }

    addr_list = parse_ip_addr(str_addr);
    if (addr_list == NULL){
        LMLOG(LERR, "Error parsing RLOC address. Ignoring proxy-ETR %s",
                        str_addr);
        return (BAD);
    }
    glist_for_each_entry(it, addr_list) {
        addr = glist_entry_data(it);
        if (default_rloc_afi != AF_UNSPEC
                && default_rloc_afi != lisp_addr_ip_afi(addr)) {
            LMLOG(LWRN, "The PETR %s will not be added due to the selected "
                    "default rloc afi", str_addr);
            continue;
        }

        /* Create locator representing the proxy-etr and add it to the mapping */
        locator = locator_init_remote_full(addr, UP, priority, weight, 255, 0);

        if (locator != NULL) {
            if (mapping_add_locator(xtr->petrs->mapping, locator)!= GOOD){
                locator_del(locator);
                continue;
            }
        }
    }

    glist_destroy(addr_list);

    return(GOOD);
}

/*
 * Create the locators associated with the address of the iface and assign them
 * to the mapping_t and the iface_locators
 * @param iface Interface containing the rlocs associated to the mapping
 * @param if_loct Structure that associate iface with locators
 * @param m Mapping where to add the new locators
 * @param p4 priority of the IPv4 RLOC. 1..255 -1 the IPv4 address is not used
 * @param w4 weight of the IPv4 RLOC
 * @param p4 priority of the IPv6 RLOC. 1..255 -1 the IPv6 address is not used
 * @param w4 weight of the IPv6 RLOC
 * @return GOOD if finish correctly or an error code otherwise
 */
int
link_iface_and_mapping(
        iface_t *iface,
        iface_locators *if_loct,
        mapping_t *m,
        int p4,
        int w4,
        int p6,
        int w6)
{
    locator_t *locator = NULL;

    /* Add mapping to the list of mappings associated to the interface */
    if (glist_contain(m, if_loct->mappings) == FALSE){
        glist_add(m,if_loct->mappings);
    }

    /* Create IPv4 locator and assign to the mapping */
    if ((p4 >= 0) && (default_rloc_afi != AF_INET6)) {
        locator = locator_init_local_full(iface->ipv4_address,
                iface->status, p4, w4, 255, 0,
                &(iface->out_socket_v4));
        if (!locator) {
            return(BAD);
        }

        if (mapping_add_locator(m, locator) != GOOD) {
            return(BAD);
        }
        glist_add(locator,if_loct->ipv4_locators);
    }

    /* Create IPv6 locator and assign to the mapping  */
    if ((p6 >= 0) && (default_rloc_afi != AF_INET)) {

        locator = locator_init_local_full(iface->ipv6_address,
                iface->status, p6, w6, 255, 0,
                &(iface->out_socket_v6));

        if (!locator) {
            return(BAD);
        }

        if (mapping_add_locator(m, locator) != GOOD) {
            return(BAD);
        }
        glist_add(locator,if_loct->ipv6_locators);
    }

    return(GOOD);
}


int
add_rtr_iface(
        lisp_xtr_t  *xtr,
        char        *iface_name,
        int         p,
        int         w)
{
    iface_t         *iface   = NULL;
    iface_locators  *if_loct = NULL;
    lisp_addr_t     aux_address;

    if (iface_name == NULL){
        LMLOG(LERR, "Configuration file: No interface specified for RTR. "
                "Discarding!");
        return (BAD);
    }

    if (validate_priority_weight(p, w) != GOOD) {
        return(BAD);
    }

    /* Check if the interface already exists. If not, add it*/
    if ((iface = get_interface(iface_name)) == NULL) {
        iface = add_interface(iface_name);
        if (!iface) {
            LMLOG(LWRN, "add_rtr_iface: Can't create interface %s",
                    iface_name);
            return(BAD);
        }
    }

    if_loct = (iface_locators *)shash_lookup(xtr->iface_locators_table,iface_name);
    if (if_loct == NULL){
        if_loct = iface_locators_new(iface_name);
        shash_insert(xtr->iface_locators_table, iface_name, if_loct);
    }

    if (!xtr->all_locs_map) {
        lisp_addr_ip_from_char("0.0.0.0", &aux_address);
        xtr->all_locs_map = mapping_init_local(&aux_address);
    }

    if (link_iface_and_mapping(iface, if_loct, xtr->all_locs_map, p, w, p, w)
            != GOOD) {
        return(BAD);
    }

    return(GOOD);
}


lisp_site_prefix_t *
build_lisp_site_prefix(
        lisp_ms_t *     ms,
        char *          eidstr,
        uint32_t        iid,
        int             key_type,
        char *          key,
        uint8_t         more_specifics,
        uint8_t         proxy_reply,
        uint8_t         merge,
        htable_t *      lcaf_ht)
{
    lisp_addr_t *eid_prefix = NULL;
    lisp_addr_t *ht_prefix = NULL;
    lisp_site_prefix_t *site = NULL;

    if (iid > MAX_IID) {
        LMLOG(LERR, "Configuration file: Instance ID %d out of range [0..%d], "
                "disabling...", iid, MAX_IID);
        iid = 0;
    }

    if (iid < 0) {
        iid = 0;
    }

    /* DON'T DELETE eid_prefix */
    eid_prefix = lisp_addr_new();
    if (lisp_addr_ippref_from_char(eidstr, eid_prefix) != GOOD) {
        lisp_addr_del(eid_prefix);
        /* if not found, try in the hash table */
        ht_prefix = htable_lookup(lcaf_ht, eidstr);
        if (!ht_prefix) {
            LMLOG(LERR, "Configuration file: Error parsing RLOC address %s",
                    eidstr);
            return (NULL);
        }
        eid_prefix = lisp_addr_clone(ht_prefix);
    }

    site = lisp_site_prefix_init(eid_prefix, iid, key_type, key,
            more_specifics, proxy_reply, merge);
    lisp_addr_del(eid_prefix);
    return(site);
}


/* Parses an EID/RLOC (IP or LCAF) and returns a list of 'lisp_addr_t'.
 * Caller must free the returned value */
glist_t *
parse_lisp_addr(
        char *      addr_str,
        htable_t *  lcaf_ht)
{
    glist_t *       addr_list   = NULL;
    lisp_addr_t *   addr        = NULL;
    lisp_addr_t *   lcaf        = NULL;
    int             res         = 0;

    addr = lisp_addr_new();

    if (strstr(addr_str,"/") == NULL){
        // Address may be an IP
        res = lisp_addr_ip_from_char(addr_str, addr);
    }else{
        // Address may be a prefix
        res = lisp_addr_ippref_from_char(addr_str, addr);
    }

    if (res != GOOD){
        lisp_addr_del(addr);
        addr = NULL;
        /* if not found, try in the hash table */
        lcaf = htable_lookup(lcaf_ht, addr_str);
        if (lcaf != NULL) {
            addr = lisp_addr_clone(lcaf);
        }
    }

    if (addr != NULL){
        addr_list = glist_new_managed((glist_del_fct)lisp_addr_del);
        if (addr_list != NULL){
            glist_add (addr,addr_list);
        }
    }else{
        addr_list = fqdn_to_addresses(addr_str,default_rloc_afi);
    }

    if (addr_list == NULL || glist_size(addr_list) == 0){
        LMLOG(LERR, "Configuration file: Error parsing address %s",addr_str);
    }

    return(addr_list);
}


/* Parses a char (IP or FQDN) into a list of 'lisp_addr_t'.
 * Caller must free the returned value */
glist_t *
parse_ip_addr(char *addr_str)
{
    glist_t *       addr_list   = NULL;
    lisp_addr_t *   addr        = NULL;
    int             res         = 0;

    addr = lisp_addr_new();

    res = lisp_addr_ip_from_char(addr_str, addr);

    if (res == GOOD){
        addr_list = glist_new_managed((glist_del_fct)lisp_addr_del);
        if (addr_list != NULL){
            glist_add (addr,addr_list);
        }
    }else{
        lisp_addr_del(addr);
        addr_list = fqdn_to_addresses(addr_str,default_rloc_afi);
    }

    if (addr_list == NULL || glist_size(addr_list) == 0){
        LMLOG(LERR, "Configuration file: Error parsing address %s",addr_str);
    }

    return(addr_list);
}

locator_t*
clone_customize_locator(
        lisp_ctrl_dev_t     *dev,
        locator_t*          locator,
        glist_t*            no_addr_loct_l,
        uint8_t             type)
{
    char *              iface_name      = NULL;
    locator_t *         new_locator     = NULL;
    iface_t *           iface           = NULL;
    lisp_addr_t *       rloc            = NULL;
    lisp_addr_t *       aux_rloc        = NULL;
    int                 rloc_ip_afi     = AF_UNSPEC;
    int *               out_socket      = 0;
    no_addr_loct *      nloct           = NULL;
    lisp_xtr_t *        xtr             = NULL;
    shash_t *           iface_lctrs     = NULL;
    iface_locators *    if_loct         = NULL;

    rloc = locator_addr(locator);
    /* LOCAL locator */
    if (type == LOCAL_LOCATOR) {
        /* Decide IP address to be used to lookup the interface */
        if (lisp_addr_is_lcaf(rloc) == TRUE) {
            aux_rloc = lcaf_get_ip_addr(lisp_addr_get_lcaf(rloc));
            if (aux_rloc == NULL) {
                LMLOG(LERR, "Configuration file: Can't determine RLOC's IP "
                        "address %s", lisp_addr_to_char(rloc));
                lisp_addr_del(rloc);
                return(NULL);
            }
        } else if (lisp_addr_is_no_addr(rloc)){
            aux_rloc = rloc;
            nloct = get_no_addr_loct_from_list(no_addr_loct_l,locator);
            if (nloct == NULL){
                return (NULL);
            }
            iface_name = nloct->iface_name;
            rloc_ip_afi = nloct->afi;
        } else{
            aux_rloc = rloc;
            /* Find the interface name associated to the RLOC */
            if (!(iface_name = get_interface_name_from_address(aux_rloc))) {
                LMLOG(LERR, "Configuration file: Can't find interface for RLOC %s",
                        lisp_addr_to_char(aux_rloc));
                return(NULL);
            }
            rloc_ip_afi = lisp_addr_ip_afi(aux_rloc);
        }

        /* Find the interface */
        if (!(iface = get_interface(iface_name))) {
            if (!(iface = add_interface(iface_name))) {
                return(NULL);
            }
        }

        out_socket = (rloc_ip_afi == AF_INET) ? &(iface->out_socket_v4) : &(iface->out_socket_v6);

        new_locator = locator_init_local_full(rloc, iface->status,
                            locator_priority(locator), locator_weight(locator),
                            255, 0, out_socket);

        /* Associate locator with iface */
        if (dev->mode == xTR_MODE || dev->mode == MN_MODE){
            xtr  = CONTAINER_OF(ctrl_dev, lisp_xtr_t, super);
            iface_lctrs = xtr->iface_locators_table;

            if_loct = (iface_locators *)shash_lookup(iface_lctrs, iface_name);

            if (if_loct == NULL){
                if_loct = iface_locators_new(iface_name);
                shash_insert(xtr->iface_locators_table, iface_name, if_loct);
            }

            if (rloc_ip_afi == AF_INET){
                glist_add(new_locator,if_loct->ipv4_locators);
            }else{
                glist_add(new_locator,if_loct->ipv6_locators);
            }
        }
    /* REMOTE locator */
    } else {
        new_locator = locator_init_remote_full(rloc, UP, locator_priority(locator), locator_weight(locator), 255, 0);
        if (new_locator != NULL) {
            locator_set_type(new_locator,type);
        }

    }

    return(new_locator);
}


/*
 *  Converts the hostname into IPs which are added to a list of lisp_addr_t
 *  @param addr_str String conating fqdn address or de IP address
 *  @param preferred_afi Indicates the afi of the IPs to be added in the list
 *  @return List of addresses (glist_t *)
 */
glist_t *fqdn_to_addresses(
        char        *addr_str,
        const int   preferred_afi)
{
    glist_t *           addr_list               = NULL;
    lisp_addr_t *       addr                    = NULL;
    struct addrinfo     hints;
    struct addrinfo *   servinfo                = NULL;
    struct addrinfo *   p                       = NULL;
    struct sockaddr *   s_addr                  = NULL;

    addr_list = glist_new_managed((glist_del_fct)lisp_addr_del);

    memset(&hints, 0, sizeof hints);

    hints.ai_family = preferred_afi;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = IPPROTO_UDP;    /* we are interested in UDP only */

    if ((err = getaddrinfo( addr_str, 0, &hints, &servinfo)) != 0) {
        LMLOG( LWRN, "fqdn_to_addresses: %s", gai_strerror(err));
        return( NULL );
    }
    /* iterate over addresses */
    for (p = servinfo; p != NULL; p = p->ai_next) {

        if ((addr = lisp_addr_new_lafi(LM_AFI_IP))== NULL){
            LMLOG( LWRN, "fqdn_to_addresses: Unable to allocate memory for lisp_addr_t");
            continue;
        }

        s_addr = p->ai_addr;

        switch(s_addr->sa_family){
        case AF_INET:
            ip_addr_init(lisp_addr_ip(addr),&(((struct sockaddr_in *)s_addr)->sin_addr),s_addr->sa_family);
            break;
        case AF_INET6:
            ip_addr_init(lisp_addr_ip(addr),&(((struct sockaddr_in6 *)s_addr)->sin6_addr),s_addr->sa_family);
            break;
        default:
            break;
        }

        LMLOG( DBG_1, "converted addr_str [%s] to address [%s]", addr_str, lisp_addr_to_char(addr));
        /* depending on callback return, we continue or not */

        glist_add(addr,addr_list);
    }
    freeaddrinfo(servinfo); /* free the linked list */
    return (addr_list);
}
