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

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <zmq.h>

#include "lispd_api.h"
#include "lib/lmlog.h"
#include "lib/util.h"
#include "liblisp/liblisp.h"


int
lmapi_init_client(lmapi_connection_t *conn)
{
    int error;

    conn->context = zmq_ctx_new();

    //Request-Reply communication pattern (Client side)
    conn->socket = zmq_socket(conn->context, ZMQ_REQ);

    //Attachment point for other processes
    error = zmq_connect(conn->socket, IPC_FILE);

    if (error != 0){
        LMLOG(LDBG_2,"LMAPI: Error while ZMQ binding on client: %s\n",zmq_strerror (error));
        goto err;
    }

    LMLOG(LDBG_2,"LMAPI: API client initiated using ZMQ\n");

    return (GOOD);

err:
    LMLOG(LERR,"LMAPI: The API client couldn't be initialized.\n");
    return (BAD);
}


void
lmapi_end(lmapi_connection_t *conn)
{
    LMLOG(LDBG_2,"LMAPI: Closing ZMQ-based API\n");

    zmq_close (conn->socket);
    zmq_ctx_destroy (conn->context);

    LMLOG(LDBG_2,"LMAPI: Closed ZMQ-based API\n");
}

uint8_t *
lmapi_hdr_push(uint8_t *buf, lmapi_msg_hdr_t * hdr)
{
    uint8_t *ptr;

    memcpy(buf,hdr,sizeof(lmapi_msg_hdr_t));

    ptr = CO(buf,sizeof(lmapi_msg_hdr_t));

    return (ptr);
}

void fill_lmapi_hdr(lmapi_msg_hdr_t *hdr, lmapi_msg_device_e dev,
        lmapi_msg_target_e trgt, lmapi_msg_opr_e opr,
        lmapi_msg_type_e type, int dlen)
{
    hdr->device = (uint8_t) dev;
    hdr->target = (uint8_t) trgt;
    hdr->operation = (uint8_t) opr;
    hdr->type = (uint8_t) type;
    hdr->datalen = (uint32_t) dlen;
}

int
lmapi_result_msg_new(uint8_t **buf,lmapi_msg_device_e  dev,
        lmapi_msg_target_e trgt, lmapi_msg_opr_e opr,
        lmapi_msg_result_e res)
{
    lmapi_msg_hdr_t hdr;
    uint8_t *ptr;

    fill_lmapi_hdr(&hdr,dev,trgt,opr,LMAPI_TYPE_RESULT,sizeof(lmapi_msg_result_e));
    *buf = xzalloc(sizeof(lmapi_msg_hdr_t)+sizeof(lmapi_msg_result_e));
    ptr = lmapi_hdr_push(*buf,&hdr);
    memcpy(ptr, &res,sizeof(lmapi_msg_result_e));

    return (sizeof(lmapi_msg_hdr_t)+sizeof(lmapi_msg_result_e));
}


int
lmapi_recv(lmapi_connection_t *conn, void *buffer, int flags)
{
    int nbytes;
    int zmq_flags = 0;
    zmq_pollitem_t items [1];
    int poll_timeout;
    int poll_rc;

    if (flags == LMAPI_DONTWAIT){
        zmq_flags = ZMQ_DONTWAIT;
        poll_timeout = 0; //Return immediately
    }else{
    	poll_timeout = -1; //Wait indefinitely
    }

    items[0].socket = conn->socket;
    items[0].events = ZMQ_POLLIN; //Check for incoming packets on socket

    // Poll for packets on socket for poll_timeout time
    poll_rc = zmq_poll (items, 1, poll_timeout);

    if (poll_rc == 0) { //There is nothing to read on the socket
    	return (LMAPI_NOTHINGTOREAD);
    }

    LMLOG(LDBG_3,"LMAPI: Data available in API socket\n");

    nbytes = zmq_recv(conn->socket, buffer, MAX_API_PKT_LEN, zmq_flags);
    LMLOG(LDBG_3,"LMAPI: Bytes read from API socket: %d. ",nbytes);

    if (nbytes == -1){
    	LMLOG(LERR,"LMAPI: Error while ZMQ receiving: %s\n",zmq_strerror (errno));
    	return (LMAPI_ERROR);
    }

    return (nbytes);
}



int
lmapi_send(lmapi_connection_t *conn, void *msg, int len, int flags)
{
    int nbytes;

    LMLOG(LDBG_3,"LMAPI: Ready to send %d bytes through API socket\n",len);

    nbytes = zmq_send(conn->socket,msg,len,0);

    LMLOG(LDBG_3,"LMAPI: Bytes transmitted over API socket: %d. ",nbytes);

    if (nbytes == -1){
        	LMLOG(LERR,"LMAPI: Error while ZMQ sending: %s\n",zmq_strerror (errno));
    }

    return (GOOD);
}

int
lmapi_apply_config(lmapi_connection_t *conn, int dev, int trgt, int opr,
        uint8_t *data, int dlen)
{
	lmapi_msg_hdr_t *hdr;
	uint8_t *buffer;
	uint8_t *dta_ptr;
	uint8_t *res_ptr;
	int len;

	buffer = xzalloc(MAX_API_PKT_LEN);
	hdr = (lmapi_msg_hdr_t *) buffer;
	dta_ptr = CO(buffer,sizeof(lmapi_msg_hdr_t));

	fill_lmapi_hdr(hdr,dev,trgt,opr,LMAPI_TYPE_REQUEST,dlen);
	memcpy(dta_ptr,data,dlen);

	len = dlen + sizeof(lmapi_msg_hdr_t);
	lmapi_send(conn,buffer,len,LMAPI_NOFLAGS);
	free(buffer);

	buffer = xzalloc(MAX_API_PKT_LEN);

	//Blocks until reply
	len = lmapi_recv(conn,buffer,LMAPI_NOFLAGS);

	hdr = (lmapi_msg_hdr_t *) buffer;

	//We expect an OK/ERR result
	if ((hdr->type != LMAPI_TYPE_RESULT) || (hdr->datalen != sizeof(lmapi_msg_result_e))){
	    goto err;
	}

	res_ptr = CO(buffer,sizeof(lmapi_msg_hdr_t));
	if (*res_ptr != LMAPI_RES_OK){
	    //TODO support fine-grain errors
	    goto err;
	}

	// All good
	free (buffer);
	return (LMAPI_RES_OK);

err:

    free (buffer);
    return (LMAPI_RES_ERR);

}
