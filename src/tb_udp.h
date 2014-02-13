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

////////////// UDP Client Functions /////////////

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

///////////// UDP Server Functions //////////////

/**
 * @brief Run a server using UDP, and epoll to signal
 * transmission end.
 */
int
tb_udp_server(tb_listener_t *listener);

/**
 * @brief Called to create and start a udp server.
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
