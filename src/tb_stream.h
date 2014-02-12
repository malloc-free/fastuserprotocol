/*
 * tb_stream.h
 *
 *  Created on: 7/02/2014
 *      Author: michael
 */

#ifndef TB_STREAM_H_
#define TB_STREAM_H_

#include "tb_listener.h"

/**
 * @brief Run a server without epoll (blocking socket).
 *
 * Runs a server without using epoll, so the socket blocks. Also used
 * by UDT, aUDT and other protocols that are not implemented in sockets.
 *
 * @param listener The listener to run the server with.
 */
int
tb_stream_server(tb_listener_t *listener);

/**
 * @brief Run a multiple connection server.
 *
 * @param listener The listener to create the server for
 */
int
tb_stream_m_server(tb_listener_t *listener);

/**
 * @brief Called by epoll
 */
int
tb_stream_event(int events, void *data);

/**
 * @brief Individual stream connections for multiple stream servers.
 */
void
*tb_stream_m_server_conn(void *data);

/**
 * @brief Upload using stream sockets.
 *
 * Upload using bsd stream based protocols
 * (TCP, DCCP) with multiple connections.
 */
int
tb_stream_m_client(tb_listener_t *listener);

/**
 * @brief A single stream connection.
 */
void
*tb_stream_connection(void *data);

/**
 * @brief Upload with a single connection.
 */
int
tb_stream_client(tb_listener_t *listener);


#endif /* TB_STREAM_H_ */
