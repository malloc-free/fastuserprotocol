/*
 * tb_session.h
 *
 *  Created on: 25/11/2013
 *      Author: Michael Holmwood
 *
 * This file contains the structs used for handling sessions used in the server
 * side of single connection tests, and in multi-connection server/client tests.
 *
 * Originally TestBed was using workers, but this proved unnecessary when we
 * removed any file I/O from the tests.
 *
 */

#ifndef TB_SESSION_H_
#define TB_SESSION_H_

//Module includes.
#include "tb_epoll.h"
#include "tb_common.h"
#include "tb_session.h"
#include "tb_protocol.h"
#include "tb_logging.h"

//External includes.
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <time.h>
#include <netdb.h>
#include <pthread.h>

/**
 * @enum <tb_session_status> [tb_session.h]
 * @brief Status of the session.
 */
typedef enum
{
	SESSION_CREATED = 0,	///< Session has been created.
	SESSION_ADDR_RES,		///< Session address is resolved.
	SESSION_CONNECTED,		///< Session is connected to peer.
	SESSION_COMPLETE,		///< Session transmission is complete.
	SESSION_DISCONNECTED	///< Session is disconnected from peer.
}
tb_session_status;

/**
 * @enum <tb_session_command> [tb_session.h]
 * @brief Commands that apply to sessions.
 */
typedef enum
{
	S_CONTINUE = 1,				///< Standard operation, continue.
	S_EXIT = 2,					///< Exit from the current operation.
	S_ABORT = 4,				///< Abort from the current operation.
	S_E_LOOP = S_EXIT | S_ABORT ///< Exit from main loop.
}
tb_session_command;

/**
 * @enum <tb_session_t> [tb_session.h]
 * @brief The data for a session.
 */
typedef struct
{
	int id;								///< Id for this session.

	//Raw network data.
	char *address;						///< The address to connect/bind to.
	char *port;    						///< The port to connect/bind to.

	//Network data
	struct sockaddr_storage *addr_in; 	///< For receiving connections.
	socklen_t addr_len;					///< Length of sockaddr_storage.
	struct addrinfo *addr_info; 		///< Connection info.
	struct addrinfo *addr_hints; 		///< Connection hints.
	int sock_d; 						///< The socket descriptor for this session.
	int pack_size; 						///< The size of the pack to use.

	//Data information
	char *file_name;					///< Name for the file, not used currently.
	char *data;							///< Data for tx/rx.
	size_t data_size;					///< Size data.
	int int_data; 						///< If the session creates the data, it is destroyed.

	//Transfer information.
	int last_trans;						///< Number of bytes sent/recv in last tx/rx.
	long long total_bytes;				///< Total number of bytes tx/rx.
	tb_prot_stats_t *stats;				///< Protocol stats for this session.

	//Status of the session.
	tb_session_status status;			///< Current status of the session.
	int *l_status;						///< The status of the session.
	long long *l_txrx;					///< Pointer to listener tx/rx.

	//Threading info
	pthread_t s_thread;				///< Thread for this session.
	pthread_mutex_t stat_lock; 		///< Lock for stat collection.
	void *n_session;					///< Link to next session (Linked list).

	//Global session info
	pthread_mutex_t *nac_lock; 			///< Lock for num_active_conn
	int *num_active_conn;  				///< Num active connections

	//Time info
	tb_time_t *transfer_t; 				///< Transfer time.
	tb_time_t *connect_t;  				///< Connection time.

	//Logging info
	int log_enabled;					///< 1 if logging is enabled.
	tb_log_t *log_info; 				///< Log info to log to.

	//Protocol specific info
	void *info; 						///< Used for other info.

	//Other info to be referenced.
	void *other_info;					///< Mostly used to reference the session_list;
}
tb_session_t;

/**
 * @struct <tb_session_list_t> [tb_session.h]
 * @brief Singly linked list for sessions.
 */
typedef struct
{
	int current_max_id;			///< The current max id for sessions in this list.
	int num_sessions;			///< Number of sessions in list.
	tb_session_t *start;		///< Pointer to first session in list.
	tb_session_t *end;			///< Pointer to last session in list.
	int *num_active_conn;		///< Number of active sessions.
	pthread_mutex_t *nac_lock;	///< Lock for num_active_conn.
	void *userdata;				///< Userdata for this list.
}
tb_session_list_t;

/////////////// Session List Functions //////////

/**
 * @brief Add a session to a list.
 *
 * Just appends a session to the list, and allocates an appropriate id
 * to the session.
 *
 * @param list List to add session to.
 * @param session The session to add to the list.
 */
inline void
tb_session_add(tb_session_list_t *list, tb_session_t *session) __attribute__ ((always_inline));

/**
 * @brief Add a session to a list.
 *
 * Adds to a list without setting the session id.
 *
 * @param list List to add session to.
 * @param sessios Session to add.
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
 * @param list The list to search.
 * @param id The id of the session to find.
 *
 * @return A pointer to the session if it is present, or null if not found.
 */
inline tb_session_t
*tb_session_list_search(tb_session_list_t *list, int id) __attribute__ ((always_inline));

/////////////// Session Functions //////////////

/**
 * @brief Create a session for servers.
 *
 * Creates a session to be used with servers, so the addr_info fields
 * are not used in the session struct.
 *
 * @return A new tb_session_t struct.
 */
tb_session_t
*tb_create_server_session();

/**
 * @brief Create a session
 *
 * With this version of create_session, the sockaddr_storage
 * is created, the file name is set to NULL, addr_info
 * and hints are null.
 *
 * @return A new tb_session_t struct, or NULL on error.
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
 *
 * @param addr The address to connect/bind to.
 * @param port The port to connect/bind to.
 * @param sock_type Type of socket to connect with.
 * @param data_size If not null, create the buffer for this session.
 *
 * @return A new tb_session_t struct, or NULL on error.
 */
tb_session_t
*tb_create_session_full(char *addr, char *port, int sock_type,
		unsigned int data_size);

/**
 * @brief Free memory used for a session.
 *
 * Frees memory used by a tb_session_t struct. Releases
 * all memory for associated fields.
 *
 * @param session The session to destroy, and free memory for.
 */
void
tb_destroy_session(tb_session_t *session);

/**
 * @brief Set the addrinfo for the session.
 *
 * Fills out the addrinfo field for the session.
 *
 * @param session The session to set the addrinfo for.
 * @param type The type of socket to set.
 *
 * @return -1 on error, 0 otherwise.
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
 *
 * Creates an appropriate string to be used in logging.
 *
 * @param session The session to log the info for.
 * @param info The info to log.
 *
 * @return A string with the session info.
 */
inline char
*tb_gen_log_str(tb_session_t *session, const char *info) __attribute__ ((always_inline));

/**
 * @brief Log info for the given session.
 *
 * Logs session specific info.
 *
 * @param session The session to log info for.
 * @param info The info to log.
 * @param type The type of info to log.
 * @param err_no The error number, if logging an error.
 *
 */
inline void
tb_log_session_info(tb_session_t *session, const char *info,
		tb_log_type_t type, int err_no) __attribute__((always_inline));

#endif /* TB_SESSION_H_ */
