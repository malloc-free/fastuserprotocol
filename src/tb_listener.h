/*
 * tb_listener.h
 *
 *  Created on: 4/12/2013
 *      Author: Michael Holmwood
 *
 *	tb_listener.h contains all of the functions for creating, destroying, and
 *	modifying the listener struct.
 *
 *	The tb_listener_t struct contains all of the parameters required to run a test.
 *	When using TestBed as an executable, tb_create_listener is called, and this
 *	returns a pointer to a listener struct built to the correct specifications.
 *	tb_listener_t also contains the stats information for the primary listener
 *	connection, and also a list of sessions if performing a multi-conneciton
 *	test.
 *
 */

#ifndef TB_LISTENER_T_H_
#define TB_LISTENER_T_H_

//In module imports.
#include "tb_protocol.h"
#include "tb_epoll.h"
#include "tb_logging.h"
#include "tb_session.h"
#include "tb_sock_opt.h"

//External imports.
#include <pthread.h>
#include <sys/socket.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>

/**
 * @enum <ENDPOINT_TYPE> [tb_listener.h]
 *
 * @brief The type of endpoint to create.
 */
typedef enum
{
	SERVER = 0, ///< The listener is a server.
	CLIENT,		///< The listener is a client.
	mSERVER,	///< Multi connection server.
	mCLIENT		///< Multi connection client.
}
ENDPOINT_TYPE;

/**
 * @enum <STATUS> [tb_listener.h]
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
	TB_CONTINUE = 1,				///< Regular status
	TB_ABORT = 2,					///< Abort the current operation
	TB_EXIT = 4,					///< Exit from the current operation
	TB_E_LOOP = TB_ABORT | TB_EXIT  ///< Exit from loop.
}
COMMAND;

/** @struct <tb_test_params_t> [tb_listener.h]
 *
 *  @brief struct used to pass parameters into testbed.
 *
 *  This struct is used to pass the parameters required for a particular
 *  test into fuction tb_create_endpoint. This information is used to
 *  create a listener to carry out the particular test.
 *
 */
typedef struct
{
	ENDPOINT_TYPE type;			///< The end point type.
	PROTOCOL protocol;			///< The protocol to test.
	CONGESTION_CONTROL control; ///< The congestion control algorithm to use.

	//Network Parameters.
	char *address;				///< The address to bind/connect to.
	char *port;					///< The port to bind/connect to.

	//Buffer sizes.
	int l4_r_b_size;			///< The size of the l4 read buffer.
	int l4_w_b_size;			///< The size of the l4 write buffer.

	int l3_r_b_size;			///< The size of the l3 read buffer.
	int l3_w_b_size;			///< The size of the l3 write buffer.

	//Data parameters.
	int d_size;					///< The size of the data to send.
	char *file_name;			///< The name of the file to use.

	//Control parameters.
	int d_on_exit;				///< Destroy the listener on exit.

	//Stat collection parameters.
	int monitor;				///< Monitor the connection.
	int print_stats;			///< Print stats to screen.

	//Logging options
	int log_enable;				///< Enable logging.
	char *log_path;				///< The path to save the log to.

	//Packet size
	int pack_size;				///< The size of a packet, in bytes.
}
tb_test_params_t;

/**
 * @typedef
 * @brief Function to be called when aborting.
 *
 * @param data Pointer to pass into the exiting function. Should be a listener.
 */
typedef void (*funct_l_exit)(void *data);

/**
 * @typedef
 * @brief Function to be called when aborting.
 *
 * This function only differs in that it should attempt to close any open
 * connections as well as exiting.
 *
 * @param data Pointer to pass into the aborting function. Should be a listener.
 */
typedef void (*funct_l_abort)(void *data);

/** @struct <tb_listener_t> [tb_listener.h]
 *
 *  @brief struct that defines the fields for the listener class.
 *
 *  This is the 'main' struct used in testbed. This is effectively a 'context'.
 *  It contains all of the data required to carry out a test. This information
 *   is set using command line parameters (when used as an application) or
 *   using the tb_test_param_t struct, passed into the tb_create_endpoint
 *   function. This avoid the need for any global variables in the TestBed.
 */
typedef struct
{
	PROTOCOL prot_id;		///< The id of the protocol to use.
	//Listener Data Structures
	int __num_threads;		///< The number of threads for listener.

	//pthread fields
	pthread_t __l_thread;			///< The thread the listener is on.
	int sys_tid;					///< The id returned from syscall
	pthread_mutex_t stat_lock; 	///< Locks the stats field for writing.
	pthread_cond_t stat_cond; 		///< The condition for releasing external threads.
	pthread_mutex_t status_lock; 	///< Lock for safe set/get of listener status.

	//cpu info
	int num_proc;					///< The number of processors available.
	int cpu_affinity;				///< The affinity for the listener thread.
	int main_cpu_aff;				///< The affinity for the main thread.

	//Listener setup
	ENDPOINT_TYPE e_type;   		///< The type of endpoint this is.

	//Network setup
	char *bind_address;				///< The bind address for this listener.
	char *bind_port;				///< The bind port for this listener.
	int bufsize;		    		///< The size of the buffer to use.
	tb_protocol_t *protocol;		///< The protocol to use.
	int sock_d;						///< The socket descriptor.
	tb_options_t *options;  		///< The options for the protocol.
	void *prot_data;				///< Protocol specific data.
	int num_connections;    		///< The number of connections to create;
	struct addrinfo *addr_info;  	///< The addrinfo for the endpoint.
	int flags;						///< The flags for the endpoint.

	//Data setup
	int file_size;					///< The size of the file to generate.
	FILE *fp;               		///< A pointer to the file to read/write to/from.
	char *filename;					///< The name of the file to load.
	char *data;						///< The data to send
	int num_send;					///< Send multiple times.

	//Stats
	long long total_tx_rx;			///< the total amount of data tx/rx.
	tb_time_t *transfer_time;  		///< Measure the time for transfer.
	tb_time_t *connect_time;   		///< Time to establish a connection.

	//Stat collection control
	tb_prot_stats_t *stats; 		///< The current stats.
	int read;						///< Current stats have been read.
	int monitor;					///< Monitor for stat collection.
	int print_stats;				///< Print stats to screen.

	//Control
	STATUS status;					///< The current status of the listener.
	COMMAND command;				///< Commands to stop, etc.
	COMMAND s_tx_end;				///< The command to process if a tx ends.
	int d_exit;						///< Destroy on exit.

	//Test Parameters
	tb_test_params_t *params;		///< The parameters for the test.

	//Logging info
	int log_enabled;				///< Logging enabled/disabled.
	tb_log_t *log_info;				///< Information for logging.

	//misc
	tb_session_t *curr_session; 	///< The current session for the listener.
	tb_epoll_t *epoll;				///< Used by protocols that require epoll;
	tb_session_list_t *session_list;///< Session list.

	//Callbacks
	funct_l_exit f_exit;			///< Called when aborting.
	funct_l_abort f_abort;			///< Called when exiting.
}
tb_listener_t;

/*******************************************************************
 * Pretty much the one and only global we are using here. This is used
 * to stop execution when SIGINT (i.e. Ctrl-C) has been pressed by the
 * user.
 */

tb_listener_t *tb_global_l;

/**
 * @brief Signal handler, currently registered to handle SIGINT.
 */
void
sig_action(int sig, siginfo_t* info, void* arg);

/*******************************************************************/
/**
 * @struct <tb_other_info> [tb_listener.h]
 *
 * @brief struct that contains information on the current status of
 * the listener.
 *
 * This struct is appended to the tb_prot_stats_t struct. It contains
 * information on the current status of the TestBed. This is appended to
 * the stats struct as 'other_info', and provides external applications
 * with information on the current status of the TestBed.
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
 * @pre The listener must be of a single connection type.
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
 * @brief Get time statistics for listener and sessions.
 *
 * Calculates and sets the time values for connections and transfers.
 * Called last thing by the monitor function before exiting.
 * A bit of a hack, but works.
 *
 * @param listener The listener to get the time stats for.
 */
void
tb_get_time_stats(tb_listener_t *listener);

/**
 * @brief A thread safe way to get access to stats
 *
 * This method controls access to the stats generated by testbed.
 * These stats are updated every 100s. When new stats have been collected, a
 * flag is set to indicate this, and the function will block until new data
 * is ready, or the status of the listener is set to 'TB_EXITING' or 'TB_ABORTING'.
 *
 * One issue with this function -
 * it actually returns the very stats struct used by the listener to collect
 * information. This means values in the struct are liable to change after this
 * function is called. This is less than ideal. Originally it used to pass out
 * a copy of the stats struct, however we were encountering problems with
 * the destruction of these copies. So this function is deprecated.
 *
 * @param listener The listener for which to get the stats from.
 * @return tb_prot_stats_t with the stats inserted.
 */
tb_prot_stats_t
*tb_ex_get_stats(tb_listener_t *listener);

/**
 * @brief Get a copy of the stats struct.
 *
 * This is a safer way to get stats, as the values are copied
 * into the provided struct, rather than passing out the exact same struct used
 * by the listener. Like the deprecated function tb_ex_get_stats, new data is
 * copied, and a flag is set indicating this has been read. Successive calls
 * to tb_ex_get_stat_cpy will block until new data is ready, or the status of the
 * listener is 'TB_EXITING' or 'TB_ABORTING'.
 *
 * @param listener The listener to get stats for.
 * @param stats The stats struct to copy to.
 */
void
tb_ex_get_stat_cpy(tb_listener_t *listener, tb_prot_stats_t *stats);

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
 * The details in the struct are used to create a listener
 * for use in testing.
 *
 * @param params A struct with all of the required details for a test.
 * @return the endpoint to test with.
 */
tb_listener_t
*tb_create_endpoint(tb_test_params_t *params);

/**
 *  @brief Destroy a listener.
 *
 *  Destroys the listener, and associated variable and structures. Also
 *  prints a bit of
 *
 *  @param listener The listener to destroy.
 */
void
tb_destroy_listener(tb_listener_t *listener);

/**
 * @brief Get info for cpu.
 *
 * Gets the tid from syscall and pthread_self, sets them in the
 * listener.
 *
 * @param listener The listener to get cpu info for.
 */
void
tb_get_cpu_info(tb_listener_t *listener);

/**
 * @brief Print a listener.
 *
 * Prints the given listener to stdout.
 *
 * @param listener The listener to print the values for.
 */
void
tb_print_listener(tb_listener_t *listener);


///////////////////////// Network Functions /////////////////////////////////

/**
 * The functions below were an attempt to provide abstractions to general
 * network functions. However, tests showed that this may have a performance
 * impact on testing, so we were slowly moving to concrete functions. This
 * migration has not been completed, however the more important functions
 * (such as send and recev) have been changed.
 *
 */

////////////////////////////////////////////////////////////////////////////

/**
 *	@brief Resolve the address for the given listener.
 *
 *	Resolves the connect/bind address for a listener.
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
 *
 * @return -1 on error, 0 otherwise //**Needs updating, migrating to void.
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
 *
 *	@return -1 on error, 0 otherwise. //**Needs updating, migrating to void
 */
inline int
tb_listen(tb_listener_t *listener) __attribute__ ((always_inline));

/**
 * @brief Accept an incoming connection.
 *
 * Accepts incoming connections. This operation blocks, as we are using
 * blocking I/O currently.
 *
 * @param listener The listener to accept the connection on.
 *
 * @return tb_session_t A newly created session. //**Needs updating, migrating to void.
 */
inline tb_session_t
*tb_accept(tb_listener_t *listener) __attribute__ ((always_inline)) __attribute__((deprecated));

/**
 * @brief Attempt to connect with a listener.
 *
 * Attempts to connect to the specified address and port.
 *
 * @param listener The listener to connect for.
 * @return -1 on error, 0 otherwise.
 */
inline int
tb_connect(tb_listener_t *listener) __attribute__ ((always_inline));

/**
 * @brief Bind the listener to its address
 *
 * Binds the listener to the address specified by address.
 *
 * @param listener The listener to bind.
 * @return -1 on error, 0 otherwise.
 */
inline int
tb_bind(tb_listener_t *listener) __attribute__ ((always_inline));

/**
 * @brief Send data on a stream protocol.
 *
 * @param listener The listener to send data for.
 * @param buff The buffer containing the data to send.
 * @param size The size in bytes of the data to send.
 *
 * @return -1 on error, 0 otherwise.
 */
inline int
tb_send_data(tb_listener_t *listener, char *buff, int size) __attribute__ ((always_inline));

/**
 * @brief Receive data on a stream protocol.
 *
 * @param listener The listener to receive data for.
 * @param session The session to refer to.
 * @return -1 on error, 0 otherwise.
 */
inline int
tb_recv_data(tb_listener_t *listener, tb_session_t *session) __attribute__ ((always_inline));

/**
 * @brief send_to - used by datagram protocols
 */
inline int
tb_send_to(tb_listener_t *listener, tb_session_t *session) __attribute__ ((always_inline));

/**
 * @brief recv from - used by datagram protocols.
 *
 * The only protocol to use this was UDP. UDT could have been adapted to use it
 * as well. Deprecated.
 */
inline int
tb_recv_from(tb_listener_t *listener, tb_session_t *session)
__attribute__ ((always_inline)) __attribute__ ((deprecated));

/**
 * @brief collect stats for listener.
 *
 * This collects stats for the given listener. The information is collected
 * in the stats struct within the listener struct.
 */
void
tb_get_prot_stats(tb_listener_t *listener);

/**
 * @brief Set epoll for this listener.
 *
 * This sets up epoll for the given listener. It is deprecated as multi-connection
 * tests now setup epoll manually.
 *
 * @param listener The listener to set up the epoll instance for.
 * @return 0 if set ok, or -1 if an error occurs.
 */
inline int
tb_set_epoll(tb_listener_t *listener)
__attribute__ ((always_inline)) __attribute__ ((deprecated));

/**
 * @brief Format an error to the log file.
 *
 * This simply writes an error to the log. It is deprecated as it has been
 * Superseded by tb_log_error in tb_common.h. Once all of the abstract network
 * functions in tb_listener.h are no longer used, then this function will be
 * removed.
 */
inline void
tb_error(tb_listener_t *listener, const char *info, int errno)
__attribute__ ((always_inline)) __attribute__ ((deprecated));

/**
 * @brief Used to log a bind/connect address.
 *
 * This will write to the log the address provided in the sockaddr_storage struct.
 *
 * @param listener The listener to log the address for.
 * @param info An info string to append to the log.
 * @param storage The sockaddr_storage struct to get the address from.
 */
inline void
tb_address(tb_listener_t *listener, const char *info, struct sockaddr_storage *store)
__attribute__((always_inline));

#endif /* TB_LISTENER_T_H_ */
