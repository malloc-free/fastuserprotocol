/*
 * tb_sock_opt.h
 *
 *  Created on: 18/01/2014
 *      Author: Michael Holmwood.
 *
 * This file contains the functions necessary to set the various options for
 * each protocol, and also defines the default values for these options. These
 * should really have been moved to the files with the rest of the protocol
 * specific functions are, will do that in the next iteration.
 */

#ifndef TB_SOCK_OPT_H_
#define TB_SOCK_OPT_H_

#include "tb_protocol.h"

/**
 * @struct
 * @brief The configurable options for protocols.
 *
 */
typedef struct
{
	PROTOCOL protocol;			///< The protocol for these options.
	int l4_r_b_size;			///< The size of the l4 read buffer.
	int l4_s_b_size;			///< The size of the l4 write buffer.
	int l3_r_b_size;			///< The size of the l3 read buffer.
	int l3_s_b_size;			///< The size of the l3 write buffer.
	CONGESTION_CONTROL control; ///< The congestion control to use.
}
tb_options_t;

/**
 * @brief Set options for individual protocols.
 *
 * This function is used to set the options on the
 * currently used protocol.
 *
 * @param listener The currently used listener.
 *
 * @return 0 if options were set, -1 otherwise.
 */
int
tb_set_sock_opt(tb_options_t *options, int fd);

/**
 * @brief Set options for BSD sockets.
 *
 * This is used by any of the protocols that use BSD sockets.
 *
 * @param options The options to set for the socket.
 * @param sock_d The socket descriptor to set the options for.
 *
 * @return -1 on error, 0 otherwise.
 */
int
tb_set_bsd_sock_opts(tb_options_t *options, int sock_d);

/**
 * @brief Set options for TCP.
 *
 * Set specific options for TCP sockets.
 *
 * @param options The options to set for the socket.
 * @param sock_d The socket to set options for.
 *
 * @return -1 on error, 0 otherwise.
 */
int
tb_set_tcp_opts(tb_options_t *options, int sock_d);

/**
 * @brief Set options for UDP.
 *
 * Set UDP specific options for sockets.
 *
 * @param options The options to set for the socket.
 * @param sock_d The socket to set options for.
 *
 * @return -1 on error, 0 otherwise.
 */
int
tb_set_udp_opts(tb_options_t *options, int sock_d);

/**
 * @brief Set options for UDT.
 *
 * Set UDT specific options for sockets.
 *
 * @param options The options to set for the socket.
 * @param sock_d The socket to set options for.
 *
 * @return -1 on error, 0 otherwise.
 */
int
tb_set_udt_opts(tb_options_t *options, int sock_d);

/**
 * @brief Set options for UDT.
 *
 * Not currently used. If aUDT had been implemented, this function would
 * have been used to modify aUDT specific options.
 *
 * @param options The options to set for the socket.
 * @param sock_d The socket to set options for.
 *
 * @return -1 on error, 0 otherwise.
 */
int
tb_set_audt_opts(tb_options_t *options, int sock_d);

/**
 * @brief Set options for uTP.
 *
 * Set uTP specific options for sockets.
 *
 * @param options The options to set for the socket.
 * @param sock_d The socket to set options for.
 *
 * @return -1 on error, 0 otherwise.
 *
 */
int
tb_set_utp_opts(tb_options_t *options, int sock_d);

/**
 * @brief Set options for eUDP.
 *
 * A deprecated function, used for setting options for UDP tests that used
 * epoll. Multi-connection tests now used protocol specific functions.
 *
 * @param options The options to set for the socket.
 * @param sock_d The socket to set the options for.
 *
 * @return -1 on error, 0 otherwise.
 */
int
tb_set_eudp_opts(tb_options_t *options, int sock_d) __attribute__ ((deprecated));

/**
 * @brief Set options for DCCP.
 *
 * Set DCCP specific options for sockets.
 *
 * @param options The options to set for the socket.
 * @param sock_d The socket to set options for.
 *
 * @return -1 on error, 0 otherwise.
 */
int
tb_set_dccp_opts(tb_options_t *options, int fd);

#endif /* TB_SOCK_OPT_H_ */
