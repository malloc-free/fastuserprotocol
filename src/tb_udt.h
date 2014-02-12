/*
 * tb_udt.h
 *
 *  Created on: 7/02/2014
 *      Author: michael
 */

#ifndef TB_UDT_H_
#define TB_UDT_H_

#include "tb_listener.h"

/**
 * @brief Run the server with UDT.
 *
 * This runs the TB server with the UDT protocol.
 *
 * @param listener The listener to run with.
 */
int
tb_udt_server(tb_listener_t *listener);

/**
 * @brief Run a multi-connection server with UDT.
 *
 * Runs a server that can handle multiple connections
 * using the UDT protocol.
 */
int
tb_udt_m_server(tb_listener_t *listener);

/**
 * @brief Accept a new incoming connection.
 *
 * Called when udt_epolls_wait2 indicates that a new
 * incoming connection is being made.
 */
int
tb_udt_event(tb_listener_t *listener);

/**
 * @brief Read data from a connection.
 *
 * Called to read data from a new connection. Passed
 * in as a function pointer in the pthread_create
 * function is called.
 *
 * @pre data must be a tb_session_t struct.
 * @param data void pointer to a tb_session_t struct.
 * @return pointer to int, 0 if connection terminated normally, -1 if not.
 */
void
*tb_udt_m_server_conn(void *data);

/**
 * @brief Upload a file using udt.
 */
int
tb_udt_client(tb_listener_t *listener);

/**
 * @brief Upload a file using multiple connections with UDT.
 *
 * Uses multiple connections to upload files using the UDT
 * protocol. The number of connections to be used is
 * stored in the num_connections field in tb_listener_t.
 *
 * @param listener A vaild tb_listener_t struct, with connection data.
 */
int
tb_udt_m_client(tb_listener_t *listener);

/**
 * @brief Write data to a connection.
 *
 * Called when creating new client connections to
 * multi-connection servers. Passed as a function pointer
 * to pthread_create.
 */
void
*tb_udt_m_connection(void *data);

#endif /* TB_UDT_H_ */
