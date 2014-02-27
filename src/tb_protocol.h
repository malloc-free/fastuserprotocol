/*
 * tb_protocol.h
 *
 *  Created on: 5/12/2013
 *      Author: michael
 */

#ifndef TB_PROTOCOL_H_
#define TB_PROTOCOL_H_

#include "tb_epoll.h"
#include "tb_common.h"

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
	eUDP = 5,	 ///< UDP with epoll. No longer used.
	DCCP = 6	 ///< DCCP, in linux kernel.
}
PROTOCOL;

/**
 * @enum
 *
 * @brief An enum that specifies valuse for the different
 * protocol congestion control algorithms. This was not actually used
 * in the TestBed, but is something to look at in the future.
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
 * @brief Flags that specify protocol behaviour. Not used in the end,
 * something that could be used for future iterations.
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


/**
 * @struct <tb_protocol_t> [tb_protocol.h]
 * @brief Defines the API for the chosen protocol.
 *
 * This struct describes the protocol to be used in communication.
 * The pointers to the functions required for communication with
 * each of the protocols is stored in this struct.
 *
 */
typedef struct
{
	PROTOCOL protocol;			///< The protocol identifier.
	void *parameters;			///< Generic parameters struct.
	int type;					///< The type of socket to use.
	const int *error_no;		///< The expected error_no for a protocol.
								///< Generally -1.
	int eot;					///< End-of-transmission number, 0 for BSD Sockets.
	int sock_flags;				///< Socket flags to be used.
	PROT_OPT prot_opts;			///< Options struct to set protocol options.
	funct_socket f_sock;		///< Function to create a socket.
	funct_bind f_bind;			///< Function to bind a socket.
	funct_connect f_connect;	///< Function to connect.
	funct_send f_send;			///< Function to send data.
	funct_recv f_recv;			///< Function to recveive data.
	funct_listen f_listen;		///< Function to listenen on a socket.
	funct_accept f_accept;		///< Function to accept a connection.
	funct_close f_close;		///< Function to close a socket.
	funct_exit f_exit;			///< Function to exit/clean up.
	funct_recvfrom f_recfrom;	///< Function to receive a datagram.
	funct_sendto f_sendto;		///< Function to send a datagram.
	funct_error f_error;		///< Function to call on error.
	funct_options f_opt;		///< Function to call to set options.
	funct_setup f_setup;		///< Function to setup a protocol.
	func_event f_ep_event;		///< Function to call on epoll event.
}
tb_protocol_t;

/** @struct <tb_prot_stats_t> [tb_protocol.h]
 *
 * @brief Protocol information struct
 *
 * This struct contains all of the statistics on the currently tested
 * protocol. Not all of the stats can be collected for all of the
 * protocols, so they are filled in where possible. An example would
 * be for UDP, where no statistics are provided by the protocol.
 * Reliable protocols, such as TCP and UDT, generate a reasonable
 * amount of statistics, as they rely on those values to perform
 * congestion control and provide reliability.
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

	long num_m_seconds;		///< Total transfer time in microseconds.
	long long current_read;	///< The current rx/tx byte read.
	long total_bytes;		///< Total received bytes.
	unsigned long ex_time;  ///< Total time for transfer.

	int send_p_loss;	///< Number of lost packets (sender side).
	int recv_p_loss;	///< Number of lost packets (recivers side).

	int w_buff_size;	///< The current send buff size.
	int r_buff_size; 	///< The current recv buff size.

	void *prot_data;	///< Protocol specific data.
	PROTOCOL protocol;	///< Protocol for these stats.

	//Time stats
	long long connect_time;  ///< The time taken to connect.
	long long transfer_time; ///< The time taken for transfer.
	tb_time_t *stat_time;	 ///< The timer for stat collection.

	void *n_stats;			 ///< Link to the next set of stats.
}
tb_prot_stats_t;

/**
 * @brief Create and allocate memory for a stats struct.
 *
 * Generate an empty stats struct. Used by external applications when
 * using cTestBed as a library.
 *
 * @return Provides a newly minted stats struct, for stats collection.
 */
tb_prot_stats_t
*tb_create_stats();

/**
 * @brief Destroy stats struct.
 *
 * Frees all of the memory associated with the stats struct.
 *
 * @param stats The stats struct to destroy.
 */
void
tb_destroy_stats(tb_prot_stats_t *stats);

/**
 * @brief Handle errors for udt.
 *
 * Called when an error has occured with UDT. To be deprecated in the
 * future.
 *
 * @param val The value of the error.
 * @param err_no The errorno associated with the error, if it exists.
 */
int
tb_udt_error(int value, int err_no) __attribute__ ((deprecated));

/**
 * @brief Handle errors for dccp.
 *
 * Called when errors occur with DCCP. To be deprecated in the future.
 *
 * @param value The value of the error
 * @param err_no The errorno associated with the error, if it exists.
 */
int
tb_dccp_error(int value, int err_no) __attribute__ ((deprecated));

/**
 * @brief Handle errors for socket.
 *
 * Called to handle errors with BSD sockets. To be deprecated in the
 * future.
 *
 * @param value The value for the error.
 * @param err_no The errorno associated with the error, if it exists.
 */
int
tb_socket_error(int value, int err_no) __attribute__ ((deprecated));

/**
 * @brief Get the stats for the given protocol, with the
 * given descriptor.
 *
 * Fetches protocol specific stats. All protocols and their implementations
 * have varying stats and methods of collection.
 *
 * @param stats The stats struct to populate with data.
 * @param sock_d The socket descripton to collect the stats for.
 */
void
tb_get_stats(tb_prot_stats_t *stats, int sock_d);

/**
 * @brief Get udt stats.
 *
 * Fetch the stats for UDT, for the specified socket descriptor.
 *
 * @param stats The stats struct to populate with data.
 * @sock_d The socket descriptor to get stats for.
 */
void
get_udt_stats(tb_prot_stats_t *stats, int sock_d);

/**
 * @brief Get dccp stats.
 *
 * Fetch the stats for DCCP, for the specified socket descriptor.
 *
 * @param stats The stats struct to populate with data.
 * @sock_d The socket descriptor to get stats for.
 */
void
get_dccp_stats(tb_prot_stats_t *stats, int sock_d);

/**
 * @brief Get tcp stats.
 *
 * Fetch the stats for TCP, for the specified socket descriptor.
 *
 * @param stats The stats struct to populate with data.
 * @sock_d The socket descriptor to get stats for.
 */
void
get_tcp_stats(tb_prot_stats_t *stats, int sock_d);

/**
 * @brief Get UDP stats.
 *
 * Fetch the stats for a UDP socket.
 *
 * @param stats The stats struct to populate with data.
 * @sock_d The socket to get the stats for.
 */
void
get_udp_stats(tb_prot_stats_t *stats, int sock_d);

/**
 * @brief Get uTP stats.
 *
 * Get stats for a uTP socket.
 *
 * @param stats The stats struct to populate with data.
 * @sock_d The socket to get the stats for.
 */
void
get_utp_stats(tb_prot_stats_t *stats, int sock_d);

/**
 * @brief Get generic BSD stats.
 *
 * Get generic stats for any BSD socket.
 *
 * @param stats The stats struct to populate with data.
 * @param sock_d The socket to get stats for.
 */
void
tb_get_bsd_stats(tb_prot_stats_t *stats, int sock_d);

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
