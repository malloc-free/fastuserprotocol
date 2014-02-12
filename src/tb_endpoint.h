/*
 * tb_endpoint.h
 *
 *  Created on: 7/02/2014
 *      Author: michael
 */

#ifndef TB_ENDPOINT_H_
#define TB_ENDPOINT_H_

#include "tb_listener.h"
#include "tb_session.h"
#include "tb_common.h"

/**
 * @brief Runs the client.
 *
 * Runs the client, called from pthread_create.
 *
 * @pre data must be of type tb_listener_t.
 * @param data Instance of tb_listener_t to run as client.
 * @return As pthread_exit is called, this does not return.
 */
void
*tb_client(void *data) __attribute__ ((__noreturn__));

/**
 * @brief Runs the server
 *
 * This starts a server instance, called from pthread_create.
 *
 * @pre data must be of type tb_listener_t.
 * @param data This is an instance of tb_listener_t.
 * @return As pthread_exit is called, this does not return.
 */
void
*tb_server(void *data) __attribute__ ((__noreturn__));

#endif /* TB_ENDPOINT_H_ */
