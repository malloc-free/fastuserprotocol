/*
 * tb_listener.h
 *
 *  Created on: 4/12/2013
 *      Author: Michael Holmwood
 */

#ifndef TB_LISTENER_T_H_
#define TB_LISTENER_T_H_

#include "tb_protocol.h"
#include "tb_epoll.h"
#include "tb_worker.h"
#include "tb_logging.h"
#include "tb_session.h"
#include "tb_sock_opt.h"

#include <gdsl/gdsl_hash.h>
#include <gdsl/gdsl_heap.h>
#include <pthread.h>
#include <sys/socket.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <glib-2.0/glib/ghash.h>

/**
 * @enum
 *
 * @brief The type of endpoint to create.
 */
typedef enum
{
	SERVER = 0, ///< The listener is a server.
	CLIENT,		///< The listener is a client.
	mSERVER,	///< Multi connection server
	mCLIENT		///< Multi connection client.
}
ENDPOINT_TYPE;

/**
 * @enum
 *
 * Specifies the current state of the testbed.
 */
typedef enum
{
	TB_CREATED = 0, 	///< The listener has been created.
	TB_STARTING = 1,	///< The listener is starting up.
	TB_CONNECTED = 2,   ///< The listener is connected.
	TB_DISCONNECTED = 3,///< The listener is disconnected.
	TB_LISTENING = 4,   ///< The listener is listening for incoming connections.
	TB_EXITING = 5,	    ///< The listener is exiting.
	TB_ABORTING = 6,	///< The listener is aborting.
	TB_POLLING = 7		///< The listener is polling.
}
STATUS;

/**
 * @enum
 *
 * @brief Commands to be sent to the listener
 */
typedef enum
{
	TB_CONTINUE = 1,	///< Regular status
	TB_ABORT = 2,		///< Abort the current operation
	TB_EXIT = 4,		///< Exit from the current operation
	TB_E_LOOP = TB_ABORT | TB_EXIT ///< Exit from loop.
}
COMMAND;

/** @struct
 *
 *  @brief struct used to pass parameters into testbed.
 *
 */
typedef struct
{
	ENDPOINT_TYPE type;	///< The end point type.
	PROTOCOL protocol;	///< The protocol to test.
	CONGESTION_CONTROL control; ///< The congestion control algorithm to use.

	//Network Parameters.
	char *address;		///< The address to bind/connect to.
	char *port;			///< The port to bind/connect to.

	//Buffer sizes.
	int l4_r_b_size;	///< The size of the l4 read buffer.
	int l4_w_b_size;	///< The size of the l4 write buffer.

	int l3_r_b_size;	///< The size of the l3 read buffer.
	int l3_w_b_size;	///< The size of the l3 write buffer.

	//Data parameters.
	int d_size;			///< The size of the data to send.
	char *file_name;	///< The name of the file to use.

	//Control parameters.
	int d_on_exit;		///< Destroy the listener on exit.

	//Stat collection parameters.
	int monitor;		///< Monitor the connection.
	int print_stats;	///< Print stats to screen.

	//Logging options
	int log_enable;		///< Enable logging.
	char *log_path;		///< The path to save the log to.

	//Packet size
	int pack_size;		//< The size of a packet, in bytes.
}
tb_test_params_t;

/**
 * @typedef
 * @brief Function to be called when aborting.
 */
typedef void (*funct_l_exit)(void *listener);

/**
 * @typedef
 * @brief Function to be called when exiting.
 */
typedef void (*funct_l_abort)(void *listener);

/** @struct
 *  @brief struct that defines the fields for the listener class.
 */
typedef struct
{
	//Listener Data Structures
	gdsl_hash_t __sessions; ///< The active sessions for this
							///< listener. Maps address to
							///< listener.

	gdsl_heap_t __workers; 	///< The workers for this listener.
	int __num_threads;		///< The number of threads for listener.

	GHashTable *sessions;   ///< Active sessions.
	//pthread fields
	pthread_t *__l_thread;	///< The thread the listener is on.
	int sys_tid;			///< The id returned from syscall
	pthread_mutex_t *stat_lock; ///< Locks the stats field for writing.
	pthread_cond_t *stat_cond; ///< The condition for releasing external threads.
	pthread_mutex_t *status_lock; ///< Lock for safe set/get of listener status.

	//cpu info
	int num_proc;			///< The number of processors available.
	int cpu_affinity;		///< The affinity for the listener thread.
	int main_cpu_aff;		///< The affinity for the main thread.

	//Listener setup
	ENDPOINT_TYPE e_type;   ///< The type of endpoint this is.

	//Network setup
	char *bind_address;		///< The bind address for this listener.
	char *bind_port;		///< The bind port for this listener.
	int bufsize;		    ///< The size of the buffer to use.
	tb_protocol_t *protocol;///< The protocol to use.
	int sock_d;				///< The socket descriptor.
	tb_options_t *options;  ///< The options for the protocol.
	void *prot_data;		///< Protocol specific data.
	int num_connections;    ///< The number of connections to create;

	//Data setup
	int file_size;			///< The size of the file to generate.
	FILE *fp;               ///< A pointer to the file to read/write to/from.
	char *filename;			///< The name of the file to load.
	char *data;				///< The data to send
	int num_send;			///< Send multiple times.

	struct addrinfo *addr_info;  ///< The addrinfo for the endpoint.
	int flags;				///< The flags for the endpoint.


	//Stats
	long long total_tx_rx;		///< the total amount of data tx/rx.
	double sec;				///< The seconds it took for the last transfer.

	//Stat collection control
	tb_prot_stats_t *stats; ///< The current stats.
	int read;				///< Current stats have been read.
	int monitor;			///< Monitor for stat collection.
	int print_stats;		///< Print stats to screen.

	//Control
	STATUS status;			///< The current status of the listener.
	COMMAND command;		///< Commands to stop, etc.
	COMMAND s_tx_end;		///< The command to process if a tx ends.
	int d_exit;				///< Destroy on exit.

	//Test Parameters
	tb_test_params_t *params;	///< The parameters for the test.

	//Logging info
	int log_enabled;		///< Logging enabled/disabled.
	tb_log_t *log_info;		///< Information for logging.

	//misc
	tb_session_t *curr_session; //< The current session for the listener.
	tb_epoll_t *epoll;		///< Used by protocols that require epoll;
	tb_session_list_t *session_list; ///< Session list.

	//Callbacks
	funct_l_exit f_exit;	///< Called when aborting.
	funct_l_abort f_abort;	///< Called when exiting.
}
tb_listener_t;

/**
 * @struct
 * @brief struct that contains information on the current status of
 * the listener.
 */
typedef struct
{
	int l_status;		///< The current status of the listener.
	int sys_tid;		///< The tid of the thread according to the sys.
	int cpu_affinity;	///< The current affinity of the cpu.
}
tb_other_info;

/**
 * @brief Gets the stats for the listener.
 *
 * This method collects the stats for single connection
 * servers. The stats are saved in the listener->stats
 * field.
 *
 * @param listener The listener to collect stats for.
 */
void
tb_set_l_stats(tb_listener_t *listener);

/**
 * @brief Gets the stats for the listener.
 *
 * This method collects the stats for multiple connection
 * servers. The stats are saved in each of the sessions
 * stats structs, and the total number of bytes sent
 * are saved in the listener->stats struct.
 *
 * @pre The listener must be of the multiple connection type.
 * @param listener The listener to collect stats for.
 */
void
tb_set_m_stats(tb_listener_t *listener);

/**
 * @brief A thread safe way to get access to stats
 *
 * This method controls access to the stats generated by testbed.
 * These stats are updated every second, and can be read once. If
 * the data that can be obtained by this function has already been
 * read, it blocks until new data has arrived.
 *
 * @param listener The listener for which to get the stats from.
 * @return tb_prot_stats_t with the stats inserted.
 */
tb_prot_stats_t
*tb_ex_get_stats(tb_listener_t *listener);

/**
 *  @brief Create a listener with the supplied parameters.
 *
 *	Creates a listener and the accociated data structures.
 *
 *	@param type The type of endpoint to create.
 *  @param addr The address to bind to.
 *  @param port The port to bind to.
 *  @param num_threads The number of worker threads to use.
 *
 *  @return The newly created listener.
 */
tb_listener_t
*tb_create_listener(ENDPOINT_TYPE type, char *addr, char *port, PROTOCOL protocol,
		int bufsize);

/**
 * @brief Create a listener using a tb_test_params_t struct
 *
 * The details in the struct are used to create an endpoint
 * for use in testing.
 *
 * @param params A struct with all of the required details for a test.
 * @return the endpoint to test with.
 */
tb_listener_t
*tb_create_endpoint(tb_test_params_t *params);

/**
 * @brief Get a worker for the supplied session.
 */
tb_worker_t
*tb_get_worker(tb_listener_t *listener, tb_session_t *session);

/**
 *  @brief Destroy a listener.
 *
 *  Destroys the listener, and its associated data structures.
 *
 *  @param listener The listener to destroy.
 */
void
tb_destroy_listener(tb_listener_t *listener);

/**
 * @brief Set the affinity and scheduling priority for a given
 * thread
 *
 * This sets the affinity for a thread
 */
int
tb_set_therad_param(tb_listener_t *listener);

/**
 * Gets the tid from syscall and pthread_self, sets them in the
 * listener.
 */
void
tb_get_cpu_info(tb_listener_t *listener);

/**
 * @brief Print a listener.
 *
 * Prints the given listener.
 *
 * @param listener The listener to print the values for.
 */
void
tb_print_listener(tb_listener_t *listener);

/**
 *	@brief Resolve the address for the given listener.
 *
 *	@param listener The listener to resolve the address for.
 *	@return 0 if there was no error, -1 otherwise.
 */
int
tb_resolve_address(tb_listener_t *listener);

/**
 * @brief Set sockopt
 *
 * Set the sockoptions for the given listener.
 *
 * @param listener The listener to set the sockopts for
 * @return -1 on error, 0 otherwise.
 */
int
tb_set_sockopt(tb_listener_t *listener);

/**
 * @brief Create a socket
 *
 * Creates a new socket for the specified listener.
 *
 * @param The listener to create the socket for.
 */
inline int
tb_create_socket(tb_listener_t *listener) __attribute__ ((always_inline));

/**
 *	@brief Bind and listen.
 *
 *	Binds the listener to the specified port and ip address,
 *	and then begins listening.
 *
 *	@param listener The listener to listen.
 *	@return 0
 */
inline int
tb_listen(tb_listener_t *listener) __attribute__ ((always_inline));

/**
 * @brief Accept an incoming connection.
 *
 * Accepts incoming connections. This operation blocks, as we are using
 * blocking I/O currently.
 *
 * @deprecated
 * @param listener The listener to accept the connection on.
 * @return tb_session_t A newly created session.
 */
inline tb_session_t
*tb_accept(tb_listener_t *listener) __attribute__ ((always_inline)) __attribute__((deprecated));

/**
 * @brief Attempt to connect.
 *
 * Attempts to connect to the specified address and port.
 */
inline int
tb_connect(tb_listener_t *listener) __attribute__ ((always_inline));

/**
 * @brief Bind the listener to its address
 *
 * Binds the listener to the address specified by address.
 *
 * @param listener The listener to bind.
 * @return 0 on success, -1 on failure.
 */
inline int
tb_bind(tb_listener_t *listener) __attribute__ ((always_inline));

/**
 * @brief Send data.
 */
inline int
tb_send_data(tb_listener_t *listener, char *buff, int size) __attribute__ ((always_inline));

/**
 * @brief Receive data.
 */
inline int
tb_recv_data(tb_listener_t *listener, tb_session_t *session) __attribute__ ((always_inline));

inline int
tb_send_to(tb_listener_t *listener, tb_session_t *session) __attribute__ ((always_inline));
/**
 * @brief recv from - used by UDP.
 */
inline int
tb_recv_from(tb_listener_t *listener, tb_session_t *session) __attribute__ ((always_inline));

/**
 * @brief collect stats for listener.
 */
void
tb_get_prot_stats(tb_listener_t *listener);

/**
 * @brief Set epoll for this listener.
 */
inline int
tb_set_epoll(tb_listener_t *listener) __attribute__ ((always_inline));

/**
 * @brief Format an error to the log file.
 */
inline void
tb_error(tb_listener_t *listener, const char *info, int errno) __attribute__ ((always_inline));

inline void
tb_address(tb_listener_t *listener, const char *info, struct sockaddr_storage *store) __attribute__((always_inline));

#endif /* TB_LISTENER_T_H_ */
