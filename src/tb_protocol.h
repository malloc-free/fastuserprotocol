/*
 * tb_protocol.h
 *
 *  Created on: 5/12/2013
 *      Author: michael
 */

#ifndef TB_PROTOCOL_H_
#define TB_PROTOCOL_H_

#include "tb_epoll.h"

#include <sys/socket.h>

/**
 * @enum
 *
 * @brief An enum that specifies values for the different
 * protocols.
 */
typedef enum
{
	TCP = 0, 	 ///< Regular, garden variety TCP. Yawn.
	UDP = 1,	 ///< Regular, garden variety UDP.
	UDT = 2,	 ///< Flash and fun UDT, please.
	aUDT = 3, 	 ///< Yet to be implemented adaptive UDT.
	uTP = 4,     ///< Arrrr a pirates favourite protocol.
	eUDP = 5,	 ///< UDP with epoll.
	DCCP = 6	 ///< DCCP, in linux kernal.
}
PROTOCOL;

/**
 * @enum
 *
 * @brief An enum that specifies valuse for the different
 * protocol congestion control algorithms.
 */
typedef enum
{
	TCP_VEGAS = 1, ///< The TCP vegas control algorithm.
	TCP_CUBIC,	   ///< The TCP CUBIC control algorithm.
	UDT_DAIMID,    ///< UDT's default control algorithm.
	UDT_ADAPTIVE,  ///< The new algorithm added to UDT.
	UDP_NONE       ///< UDP does not use congestion control.
}
CONGESTION_CONTROL;

/**
 * @enum
 *
 * @brief Flags that specify protocol behaviour.
 */
typedef enum
{
	USE_BLOCKING, 	///< Use standard blocking sockets.
	USE_EPOLL 		///< Use Epoll for sockets.
}
PROT_OPT;

/**
 * @typedef
 * @brief The function for socket creation.
 */
typedef int (*funct_socket)(int domain, int type, int protocol);

/**
 * @typedef
 * @brief The function for closing connections.
 */
typedef int (*funct_close)(int fd);

/**
 * @typedef
 * @brief The function for binding to a socket.
 */
typedef int (*funct_bind)(int fd, const struct sockaddr *addr, socklen_t len);

/**
 * @typedef
 * @brief The function for connection.
 */
typedef int (*funct_connect)(int fd, const struct sockaddr *addr, socklen_t len);

/**
 * @typedef
 * @brief The function for sending data.
 */
typedef int (*funct_send)(int fd, void *buf, size_t n, int sock_flags);

/**
 * @typedef
 * @brief The function for receiving data.
 */
typedef int (*funct_recv)(int fd, void *buf, size_t n, int sock_flags);

/**
 * @typedef
 * @brief The function for listening on a socket.
 */
typedef int (*funct_listen)(int fd, int n);

/**
 * @typedef
 * @brief The function for accepting a connection.
 */
typedef int (*funct_accept)(int fd, const struct sockaddr *addr, socklen_t *len);

/**
 * @typedef
 * @brief The function for sending datagrams to an address.
 */
typedef int (*funct_sendto)(int fd, const void *buf, size_t len,
		int flags, const struct sockaddr *dest_addr, socklen_t addrlen);

/**
 * @typedef
 * @brief Used by datagram based servers to receive data
 */
typedef int (*funct_recvfrom)(int fd, void *buf, int len, unsigned int flags,
		const struct sockaddr *to, socklen_t *tolen);

/**
 * @typedef
 * @brief The function called when the system should be shut down.
 */
typedef void (*funct_exit)();

/**
 * @typedef
 * @brief The function called when an error has occured.
 */
typedef int (*funct_error)(int value, int err_no);

/**
 * @typedef
 * @brief The function to be called when setting options.
 */
typedef int (*funct_options)(int sock, int level, int optname,
		void *optval, int optlen);

/**
 * @typedef
 * @brief The function to call when the protocol needs to be setup.
 */
typedef int (*funct_setup)(void *data);


/** @struct <tb_protocol_t> [tb_protocol.h]
 *
 * @brief Defines the API for the chosen protocol.
 *
 * This struct describes the protocol to be used in communication.
 * The pointers to the functions required for communication with
 * each of the protocols is stored in this struct.
 *
 */
typedef struct
{
	PROTOCOL protocol;
	void *parameters;
	int type;
	const int *error_no;
	int eot;
	int sock_flags;
	PROT_OPT prot_opts;
	funct_socket f_sock;
	funct_bind f_bind;
	funct_connect f_connect;
	funct_send f_send;
	funct_recv f_recv;
	funct_listen f_listen;
	funct_accept f_accept;
	funct_close f_close;
	funct_exit f_exit;
	funct_recvfrom f_recfrom;
	funct_sendto f_sendto;
	funct_error f_error;
	funct_options f_opt;
	funct_setup f_setup;
	func_event f_ep_event;
}
tb_protocol_t;

/** @struct <tb_prot_stats_t> [tb_protocol.h]
 *
 * @brief Protocol information struct
 *
 * This struct contains all of the statistics on the currently tested
 * protocol.
 *
 */
typedef struct
{
	int id;				///< The id for this stat.
	double rtt;			///< The current rtt.
	double rtt_var;    	///< The current rttvar.
	int byte_sec;		///< The bytes/sec as measured by the testbed.
	double av_byte_sec; ///< The average bytes/sec, as measured by testbed.
	int peak_byte_sec;	///< Peak transfer rate.
	double send_rate;	///< The send rate.
	double recv_rate;	///< The recv rate.
	double send_window; ///< The send window.
	double recv_window; ///< The current recv window for the socket.
	void *other_info; 	///< Other info added here.

	long num_m_seconds;	///< Total transfer time in microseconds.
	long long current_read;	///< The current rx/tx byte read.
	long total_bytes;	///< Total received bytes.
	unsigned long ex_time;  ///< Total time for transfer.

	int send_p_loss;	///< Number of lost packets (sender side).
	int recv_p_loss;	///< Number of lost packets (recivers side).

	int w_buff_size;	///< The current send buff size.
	int r_buff_size; 	///< The current recv buff size.

	void *prot_data;	///< Protocol specific data.
	PROTOCOL protocol;	///< Protocol for these stats.
}
tb_prot_stats_t;

/**
 * @brief Destroy stats struct.
 */
void
tb_destroy_stats(tb_prot_stats_t *stats);

/**
 * @brief Handle errors for udt.
 */
int
tb_udt_error(int value, int err_no);

/**
 * @brief Handle errors for dccp
 */
int
tb_dccp_error(int value, int err_no);

/**
 * @brief Handle errors for socket.
 */
int
tb_socket_error(int value, int err_no);

/**
 * @brief Get the stats for the given protocol, with the
 * given descriptor.
 */
void
tb_get_stats(tb_prot_stats_t *stats, int fd);

/**
 * @brief Get udt stats.
 */
void
get_udt_stats(tb_prot_stats_t *stats, int fd);

/**
 * @brief Get dccp stats.
 */
void
get_dccp_stats(tb_prot_stats_t *stats, int fd);

/**
 * @brief Get tcp stats.
 */
void
get_tcp_stats(tb_prot_stats_t *stats, int fd);

/**
 * @brief Get udp stats.
 */
void
get_udp_stats(tb_prot_stats_t *stats, int fd);

/**
 * @brief Get utp stats.
 */
void
get_utp_stats(tb_prot_stats_t *stats, int fd);

/**
 * @brief Get generic bsd stats.
 */
void
tb_get_bsd_stats(tb_prot_stats_t *stats, int fd);

/**
 * @brief Create a new Protocol
 *
 * Creates the protocol, and loads the struct with all of
 * the relevant functions to perform communication.
 *
 * @pre The protocol must be one of the supported types.
 * @param prot The protocol to use in the tests.
 * @return The struct tb_protocol_t
 */
tb_protocol_t
*tb_create_protocol(PROTOCOL prot);

/**
 * @brief Destroy the given protocol structure.
 *
 * Destroys the struct, freeing up memory.
 *
 * @pre protocol must be of type tb_protocol_t.
 * @param protocol The struct to destroy.
 */
void
tb_destroy_protocol(tb_protocol_t *protocol);

/**
 * @brief Print the values for this protocol.
 *
 * @param listener The protocol to print the values for.
 */
void
tb_print_protocol(tb_protocol_t *protocol);

#endif /* TB_PROTOCOL_H_ */
