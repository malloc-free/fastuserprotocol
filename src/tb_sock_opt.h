/*
 * tb_sock_opt.h
 *
 *  Created on: 18/01/2014
 *      Author: michael
 */

#ifndef TB_SOCK_OPT_H_
#define TB_SOCK_OPT_H_

#include "tb_protocol.h"

/**
 * @struct
 * @brief The configurable options for protocols.
 */
typedef struct
{
	PROTOCOL protocol;	///< The protocol for these options.
	int l4_r_b_size;	///< The size of the l4 read buffer.
	int l4_s_b_size;	///< The size of the l4 write buffer.
	int l3_r_b_size;	///< The size of the l3 read buffer.
	int l3_s_b_size;	///< The size of the l3 write buffer.
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
 * @brief Set options for BSD sockets. Used by TCP,
 * UDP and eUDP.
 */
int
tb_set_bsd_sock_opts(tb_options_t *options, int fd);

/**
 * @brief Set options for TCP
 */
int
tb_set_tcp_opts(tb_options_t *options, int fd);

/**
 * @brief Set options for UDP
 */
int
tb_set_udp_opts(tb_options_t *options, int fd);

/**
 * @brief Set options for UDT
 */
int
tb_set_udt_opts(tb_options_t *options, int fd);

/**
 * @brief Set options for aUDT
 */
int
tb_set_audt_opts(tb_options_t *options, int fd);

/**
 * @brief Set options for uTP
 */
int
tb_set_utp_opts(tb_options_t *options, int fd);

/**
 * @brief Set options for eUDP
 */
int
tb_set_eudp_opts(tb_options_t *options, int fd);

/**
 * @brief Set options for DCCP
 */
int
tb_set_dccp_opts(tb_options_t *options, int fd);

#endif /* TB_SOCK_OPT_H_ */
