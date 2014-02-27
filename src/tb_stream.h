/*
 * tb_stream.h
 *
 *  Created on: 7/02/2014
 *      Author: Michael Holmwood
 *
 *	tb_stream.h contains all of the functions for performing client and server
 *	operations on BSD stream sockets (TCP and DCCP).
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
 *
 * @return The number of bytes received.
 */
int
tb_stream_server(tb_listener_t *listener);

/**
 * @brief Run a multiple connection server.
 *
 * @param listener The listener to create the server for.
 *
 * @return The number of bytes recevied.
 */
int
tb_stream_m_server(tb_listener_t *listener);

/**
 * @brief Called by epoll.
 *
 * This function is used as a callback for epoll, when events
 * occur on the main socket.
 *
 * @param events The events that occured on the socket.
 * @param data The data (the tb_listener_struct in this case) associated with
 * 		  the socket descriptor.
 *
 * @return -1 on error, otherwise 0.
 */
int
tb_stream_event(int events, void *data);

/**
 * @brief Individual stream connections for multiple stream servers.
 *
 * An individual connection for the server. Threads are created as new incoming
 * connections are created. A pointer to this function is passed into pthread_create,
 * and data is a pointer to a session struct.
 *
 * @param data A pointer to a session struct.
 */
void
*tb_stream_m_server_conn(void *data);

/**
 * @brief Upload using stream sockets.
 *
 * Upload using bsd stream based protocols
 * (TCP, DCCP) with multiple connections.
 *
 * @param listener The listener to used for the multi-stream client.
 *
 * @return The number of bytes sent.
 */
int
tb_stream_m_client(tb_listener_t *listener);

/**
 * @brief One of many connections for a multi-connection client.
 *
 * An individual connection for a multi-connection client. Threads are created
 * to handle each socket in the client. A pointer to this function is passed into
 * pthread_create, and the data is a pointer to a session struct.
 *
 * @param data A pointer to a session struct.
 *
 * @return -1 on error, 0 otherwise.
 */
void
*tb_stream_connection(void *data);

/**
 * @brief Upload with a single connection.
 *
 * Used with single connection clients.
 *
 * @param listener The listener to use with the client.
 *
 * @return The number of bytes sent.
 */
int
tb_stream_client(tb_listener_t *listener);


#endif /* TB_STREAM_H_ */
