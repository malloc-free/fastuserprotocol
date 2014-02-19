/*
 * tb_session.h
 *
 *  Created on: 25/11/2013
 *      Author: michael
 */

#ifndef TB_SESSION_H_
#define TB_SESSION_H_

#include "tb_epoll.h"
#include "tb_common.h"
#include "tb_session.h"
#include "tb_protocol.h"
#include "tb_logging.h"

#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <time.h>
#include <netdb.h>
#include <pthread.h>

typedef enum
{
	SESSION_CREATED = 0,
	SESSION_ADDR_RES,
	SESSION_CONNECTED,
	SESSION_COMPLETE,
	SESSION_DISCONNECTED
}
tb_session_status;

typedef enum
{
	S_CONTINUE = 1,
	S_EXIT = 2,
	S_ABORT = 4,
	S_E_LOOP = S_EXIT | S_ABORT
}
tb_session_command;

typedef struct
{
	int id;			///< Id for this session.

	//Raw network data.
	char *address;	///< The address to connect/bind to.
	char *port;    ///< The port to connect/bind to.

	//Network data
	struct sockaddr_storage *addr_in; ///< For receiving connections.
	socklen_t addr_len;	///< Length of sockaddr_storage.
	struct addrinfo *addr_info; ///< Connection info.
	struct addrinfo *addr_hints; ///< Connection hints.
	int sock_d; ///< The socket descriptor for this session.
	int pack_size; ///< The size of the pack to use.

	//Data information
	char *file_name;
	char *data;
	size_t data_size;
	int int_data; ///< If the session creates the data, it is destroyed.

	//Transfer information.
	int last_trans;
	long long total_bytes;
	tb_prot_stats_t *stats;

	//Status of the session.
	tb_session_status status;
	int *l_status;		///< The status of the session.
	long long *l_txrx;

	//Threading info
	pthread_t *s_thread;	///< Thread for this session.
	pthread_mutex_t *stat_lock; ///< Lock for stat collection.
	void *n_session;		///< Link to next session (Linked list).

	//Global session info
	pthread_mutex_t *nac_lock; ///< Lock for num_active_conn
	int *num_active_conn;  ///< Num active connections

	//Time info
	tb_time_t *transfer_t; ///< Transfer time.
	tb_time_t *connect_t;  ///< Connection time.

	//Logging info
	int log_enabled;	///< 1 if logging is enabled.
	tb_log_t *log_info; ///< Log info to log to.

	//Protocol specific info
	void *info; 	///< Used for other info.

	//Other info to be referenced.
	void *other_info;	///< Mostly used to reference the session_list;
}
tb_session_t;

typedef struct
{
	int current_max_id;
	tb_session_t *start;
	tb_session_t *end;
	int *num_active_conn;
	pthread_mutex_t *nac_lock;
	void *userdata;
}
tb_session_list_t;

/////////////// Session List Functions //////////

/**
 * @brief Add a session to a list.
 *
 * Just appends a session to the list, and allocates an appropriate id
 * to the session.
 */
inline void
tb_session_add(tb_session_list_t *list, tb_session_t *session) __attribute__ ((always_inline));

/**
 * @brief Add a session to a list.
 */
inline void
tb_session_add_to(tb_session_list_t *list, tb_session_t *session) __attribute__ ((always_inline));

/**
 * @brief Increment num_active_conn, and return new value
 *
 * Thread safe way to increment num_active_conn.
 *
 * @param list tb_session_list_t to decrement.
 * @return The current number of active connections after incrementing.
 */
inline int
tb_session_list_inc(tb_session_list_t *list) __attribute__ ((always_inline));

/**
 * @brief Decrement num_active_conn, and return new value.
 *
 * Thread safe way to increment num_active_conn.
 *
 * @param list tb_session_list_t to increment.
 * @return The current number of active connections after decrementing.
 */
inline int
tb_session_list_dec(tb_session_list_t *list) __attribute__ ((always_inline));

/**
 * @brief Search for a session with the specified id.
 *
 * Search for the id of the specified session in the linked list. Return
 * a pointer to the session if found, or NULL if not. Just iterates
 * through the list, not a binary search or anything. As we are only going
 * to have 4 connections, it should suffice.
 *
 * @param list The list to search
 * @param id The id of the session to find
 * @return A pointer to the session if it is present, or null if not found.
 */
inline tb_session_t
*tb_session_list_search(tb_session_list_t *list, int id) __attribute__ ((always_inline));

/////////////// Session Functions //////////////

/**
 * @brief Create a session for servers.
 */
tb_session_t
*tb_create_server_session();

/**
 * @brief Create a session
 *
 * With this version of create_session, the sockaddr_storage
 * is created, the file name is set to NULL, addr_info
 * and hints are null.
 */
tb_session_t
*tb_create_session();

/**
 * @brief Full version of create session.
 *
 * This version creates the hints and info field, and
 * resolves the provided address. If the data_size
 * parameter is greater than zero, the session buffer
 * is also created.
 */
tb_session_t
*tb_create_session_full(char *addr, char *port, int sock_type,
		unsigned int data_size);

/**
 * @brief Free memory used for a session.
 *
 * Frees memory used by a tb_session_t struct. Releases
 * all memory for associated fields.
 */
void
tb_destroy_session(tb_session_t *session);

/**
 * @brief Set the addrinfo for the session.
 *
 * Fills out the addrinfo field for the session.
 */
int
tb_set_addrinfo(tb_session_t *session, int type);

/**
 * @brief Print the times for a given session.
 *
 * Prints to stdout the connection and transfer
 * times for the given session.
 *
 * @pre The times in the session need to have been
 * set, using the timer functions tb_start_time,
 * tb_finish_time and tb_calculate_time
 * @param session The session to print the times for.
 *
 */
void
tb_print_times(tb_session_t *session);

////////////////// Logging Functions //////////////

/**
 * @brief Generate a string to be used in logging functions.
 */
inline char
*tb_gen_log_str(tb_session_t *session, const char *info) __attribute__ ((always_inline));

/**
 * @brief Log info for the given session.
 */
inline void
tb_log_session_info(tb_session_t *session, const char *info,
		tb_log_type_t type, int err_no) __attribute__((always_inline));

#endif /* TB_SESSION_H_ */
