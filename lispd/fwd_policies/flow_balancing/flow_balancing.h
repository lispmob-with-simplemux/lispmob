/*
 *
 * Copyright (C) 2011, 2015 Cisco Systems, Inc.
 * Copyright (C) 2015 CBA research group, Technical University of Catalonia.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#ifndef FLOW_BALANCING_H_
#define FLOW_BALANCING_H_

#include "../fwd_policy.h"
#include "../../control/lisp_ctrl_device.h"


typedef struct fb_dev_parm_ {
    lisp_dev_type_e     dev_type;
    glist_t *           loc_loct;
}fb_dev_parm;

/*
 * Used to select the locator to be used for an identifier according to locators' priority and weight.
 *  v4_balancing_locators_vec: If we just have IPv4 RLOCs
 *  v6_balancing_locators_vec: If we just hace IPv6 RLOCs
 *  balancing_locators_vec: If we have IPv4 & IPv6 RLOCs
 *  For each packet, a hash of its tuppla is calculaed. The result of this hash is one position of the array.
 */

typedef struct balancing_locators_vecs_ {
    locator_t **v4_balancing_locators_vec;
    locator_t **v6_balancing_locators_vec;
    locator_t **balancing_locators_vec;
    int v4_locators_vec_length;
    int v6_locators_vec_length;
    int locators_vec_length;
} balancing_locators_vecs;

#endif /* FLOW_BALANCING_H_ */
