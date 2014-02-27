/*
 * tb_udp.h
 *
 *  Created on: 9/01/2014
 *      Author: Michael Holmwood
 *
 *  tb_udp.h contains the functions for creating and running UDP single and
 *  multi-connection servers and clients.
 *
 *  Because UDP is connection less, we have to handle multiple connections differently
 *  to the connection orientated protocols. We use messaging instead, and messages
 *  for each connection are labeled using headers, so the server knows which
 *  receiving session to pass the data to.
 *
 *  Another consideration - how to handle the closing of connections. When
 *  transmission is complete, the client sends an empty packet to signal the end
 *  of transmission every 50ms until it receives an ack from the server, or
 *  data sent to a closed server socket is refused (signifying that the ack has
 *  been lost - gotta love UDP!).
 */

#ifndef TB_UDP_WAPPER_H_
#define TB_UDP_WAPPER_H_

//Module includes.
#include "tb_epoll.h"
#include "tb_listener.h"
#include "tb_session.h"

//External includes.
#include <sys/socket.h>
#include <pthread.h>

/////////////////////// UDP Client Functions /////////////////////

/**
 * @struct <tb_udp_session_t> [tb_udp.h]
 *
 * @brief udp session struct, was intended to be used with UDP.
 */
typedef struct
{
	pthread_cond_t *data_cond;	///< condition for data.
	pthread_mutex_t *data_lock; ///< Lock for data
	tb_session_list_t *list;    ///< List of sessions.
}
tb_udp_session_t;

/**
 * @struct <tb_queue_data_t> [tb_udp.h]
 * @brief Data element for tb_queue_t.
 */
typedef struct
{
	char *data;		///< Data.
	int data_len;	///< Length of data.
	void *n_data;	///< Next data in the list.
}
tb_queue_data_t;

/**
 * @struct <tb_queue_t> [tb_udp.h]
 * @brief Tiny queue implementation for data. Not currently used.
 */
typedef struct
{
	tb_queue_data_t *start; ///< The first data element in the queue.
	tb_queue_data_t *end;	///< the last data element in the queue.
}
tb_queue_t;

//////////////////// Data Structure Functions ///////////////////

/**
 * @brief Add data to the queue.
 *
 * Add an element to the data queue.
 *
 * @param queue The queue to add the element to.
 * @param data The data to add to the queue.
 */
inline void
tb_queue_add(tb_queue_t *queue, tb_queue_data_t *data) __attribute__ ((always_inline));

//////////////////// UDP session functions /////////////////////

/**
 * @brief Create udp session info.
 *
 * Create a udp session to be used with multi-connection server/client. Not
 * currently used.
 *
 * @return A new tb_utp_session_t struct.
 */
tb_udp_session_t
*tb_create_udp_session();

/**
 * @brief Destroy udp session info.
 *
 * Free memory from a tb_utp_t struct. Not currently used.
 *
 * @param session The session to destroy.
 */
void
tb_destroy_udp_session(tb_udp_session_t *session);

/////////////////// Client Functions ///////////////////////////
/**
 * @brief Upload a file using udp with epoll eof ack.
 *
 * The difference between upload_random_udp and udp is
 * that this uses the epoll mechanism to listenen for an
 * ack of the eof. This insures that transmission is complete
 * in the event of packet loss.
 *
 * @param listener The listener to run the client from.
 *
 * @return The number of bytes sent.
 */
int
tb_udp_client(tb_listener_t *listener);

/**
 * @brief Callback function for udp
 *
 * This is the function that is called when the eof from
 * the server has be acknowledged.
 *
 * @param events The events passed from epoll.
 * @param data A pointer to a listener struct.
 *
 * @return -1 on error, otherwise 0.
 */
int
tb_udp_ack(int events, void *data);

/**
 * @brief Called to create and start a UDP client.
 *
 * The main function for a multi-connection UDP client.
 *
 * @param listener The listener struct to work from.
 *
 * @return The number of bytes sent.
 */
int
tb_udp_m_client(tb_listener_t *listener);

/**
 * @brief Called to close a multi connection client.
 *
 * Multi-UDP clients need to send a message to signal the end of the connection.
 * This function sends an empty message every 50ms, and exits when either an
 * ack is received from the server, or a message failure occurs (meaning that
 * the ack was lost in transit
 *
 * @param listener The listener struct to close the connection on.
 */
void
tb_udp_m_close_conn(tb_listener_t *listener);

/**
 * @brief Use a multi-connection client with udp.
 *
 * A single connection in a multi-connection client. A pointer to this function
 * is passed into pthread_create, and run in a separate thread. Each thread
 * then transmits one part of the total message (currently set at 4 parts).
 *
 * @param data A tb_session_t struct, for a single connection.
 */
void
*tb_udp_m_client_conn(void *data);

///////////// UDP Server Functions //////////////

/**
 * @brief Run a single connection UDP server.
 *
 * Runs a UDP server, bound and listening on the port and address specified
 * by the listener.
 *
 * @param listener The listener to run the server for.
 */
int
tb_udp_server(tb_listener_t *listener);

/**
 * @brief Called to start a multi-connection UDP server.
 *
 * Creates a multi-connection UDP server. Currently uses 4 connections for
 * uploading of data.
 *
 * @param The listener struct to run the server for.
 *
 * @return The number of bytes received.
 */
int
tb_udp_m_server(tb_listener_t *listener);

/**
 * @brief Epoll events function.
 *
 * This is a callback for when events occur on the UDP socket.
 *
 * @param events The events on the socket.
 * @param data The listener, which is assigned as user data for the epoll instance
 * 			   and passed into the callback when events occur.
 */
int
tb_udp_event(int events, void* data);

#endif /* TB_UDP_WAPPER_H_ */
