/*
 * tb_udp_wapper.h
 *
 *  Created on: 9/01/2014
 *      Author: michael
 */

#ifndef TB_UDP_WAPPER_H_
#define TB_UDP_WAPPER_H_

#include "tb_epoll.h"
#include "tb_listener.h"
#include "tb_session.h"

#include <sys/socket.h>
#include <pthread.h>

////////////// UDP Client Functions /////////////

/**
 * @struct <tb_udp_session_t> [tb_udp.h]
 */
typedef struct
{
	pthread_cond_t *data_cond;
	pthread_mutex_t *data_lock;
	tb_session_list_t *list;
}
tb_udp_session_t;

/**
 * @struct <tb_queue_data_t> [tb_udp.h]
 */
typedef struct
{
	char *data;
	int data_len;
	void *n_data;
}
tb_queue_data_t;

/**
 * @struct <tb_queue_t> [tb_udp.h]
 */
typedef struct
{
	tb_queue_data_t *start;
	tb_queue_data_t *end;
}
tb_queue_t;

//////////////////// Data Structure Functions ///////////////////

/**
 * @brief Add data to the queue.
 */
inline void
tb_queue_add(tb_queue_t *queue, tb_queue_data_t *data) __attribute__ ((always_inline));

//////////////////// UDP session functions /////////////////////

/**
 * @brief Create udp session info.
 */
tb_udp_session_t
*tb_create_udp_session();

/**
 * @brief Destroy udp session info.
 */
void
tb_destroy_udp_session();

/////////////////// Client Functions ///////////////////////////
/**
 * @brief Upload a file using udp with epoll eof ack.
 *
 * The difference between upload_random_udp and udp is
 * that this uses the epoll mechanism to listenen for an
 * ack of the eof. This insures that transmission is complete
 * in the event of packet loss.
 */
int
tb_udp_client(tb_listener_t *listener);

/**
 * @brief Callback function for udp
 *
 * This is the function that is called when the eof from
 * the server has be acknowledged.
 */
int
tb_udp_ack(int events, void *data);

/**
 * @brief Called to create and start a udp client
 */
int
tb_udp_m_client(tb_listener_t *listener);

/**
 * @brief Called to close a multi connection client.
 */
void
tb_udp_m_close_conn(tb_listener_t *listener);

/**
 * @brief Use a multi-connection client with udp.
 */
void
*tb_udp_m_client_conn(void *data);

///////////// UDP Server Functions //////////////

/**
 * @brief Run a server using UDP, and epoll to signal
 * transmission end.
 */
int
tb_udp_server(tb_listener_t *listener);

/**
 * @brief Called to create and start a udp server, that handles
 * multiple connections.
 */
int
tb_udp_m_server(tb_listener_t *listener);

/**
 * @brief Epoll events function.
 *
 * This is called when events occur on sockets
 * that are polled by epoll. Processes incoming
 * data.
 */
int
tb_udp_event(int events, void* data);

#endif /* TB_UDP_WAPPER_H_ */
