/*
 * tb_utp_wrapper.h
 *
 *  Created on: 24/12/2013
 *      Author: Michael Holmwood
 *
 *	 tb_utp.h contains all of the functions to create and run multi and single
 *	 connection clients and servers. We initially attempted to make this a
 *	 wrapper of sorts, to make it fit in with the whole attempt at abstracting
 *	 the network function for all protocols. It did work in the end, but as
 *	 the abstract way of dealing with the five protocols was abandoned,
 *	 we could use the library in more the fashion it was intended to be used.
 *
 *	 The first section contains all of the callbacks that uTP requires for
 *	 operation. The next section has all of the actual network functions. Lastly
 *	 come the client and server functions (which should really be migrated into
 *	 their own file).
 */

#ifndef TB_UTP_WRAPPER_H_
#define TB_UTP_WRAPPER_H_

//Module Imports
#include "tb_listener.h"
#include "tb_epoll.h"
#include "tb_common.h"

//External Imports
#include <utp.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>

//Declaration for the UTPSocket
struct UTPSocket;

/**
 * @struct <tb_utp_t> [tb_utp.h]
 *
 * This struct contains all of the data external to the utp library that
 * is required to use the protocol.
 */
typedef struct
{
	int id;						///< Id for this utp struct.
	struct 	UTPSocket *socket; 	///< The UTP socket.
	int sock_fd; 				///< The UDP socket associated with the UTP socket.
	char *buffer; 				///< The send buffer to be used for temporary data.
	char *rec_buffer; 			///< The receive buffer.
	int read_bytes; 			///< Total number of bytes
	int write_bytes; 			///< the number of bytes written on this socket.
	int buffer_size; 			///< The size of the buffer to use with this socket.
	int rec_buff_size; 			///< The size of the receive buffer.
	int e_id; 					///< The id of the epoll instance used with this utp socket.
	int recv_total; 			///< Total number of bytes received.
	int sent_total; 			///< Total number of bytes sent.
	int state; 					///< Current state of the utp socket.
	tb_epoll_t *epoll; 			///< epoll instance for polling main socket.
	struct sockaddr_storage
		*addr_s; 				///< Address handling.
	struct UTPFunctionTable
		*call_backs;			///< Callbacks for uTP functions.
	socklen_t addr_len; 		///< Length of addr_s.

	char *s_data; 				///< Buffer for recv/send data.
	int s_data_size; 			///< Size of recv/send buffer.

	char *r_data;				///< The receive buffer
	int r_data_size;			///< The size of the receive buffer.

	//Threading stuff
	pthread_mutex_t *lock;		///< Lock up utp functions for multi-connection.

	int so_sndbuf;				///< The size of the send buffer to use.
	int so_rcvbuf;  			///< The size of the receive buffer to use.

	//Timing Stuff
	tb_time_t *last_rec; 		///< The last time a byte was received.
	long long time_out; 		///< time out in nanoseconds.
	int measure_time;   		///< Test for timeout.
}
tb_utp_t;

/////////////// uTP management functions /////////////

/**
 * @enum <tb_utp_state> [tb_utp.h]
 * @brief Extended UTP states, for TB use.
 */
typedef enum
{
	UTP_STATE_CREATED = 0,	///< UTP struct has been created.
	UTP_STATE_ERROR = 5		///< UTP in error state.
}
tb_utp_state;

////////////// Callbacks for uTP ////////////////////

/**
 * @brief Called by the uTP library when reading data from the network.
 *
 * @param userdata The userdata associated with this callback.
 * @param bytes The data read from the network.
 * @param count The number of bytes read from the network.
 */
void
tb_utp_read(void *userdata, const byte *bytes, size_t count);

/**
 * @brief Called by uTP when writing data to the network.
 *
 * @param userdata The userdata associated with this callback.
 * @param bytes The data buffer to write data to.
 * @param count The number of bytes to write to the network.
 */
void
tb_utp_write(void *userdata, byte *bytes, size_t count);

/**
 * @brief Fetch the amount of data in the receive buffer.
 *
 * @param userdata The userdata associated with this callback.
 *
 * @return The amount of data in the receive buffer.
 */
size_t
tb_utp_get_Rcv_buff(void *userdata);

/**
 * @brief Called by uTP when the connection state changes.
 *
 * @param userdata The userdata associated with this callback.
 * @param state The state of the utp connection.
 */
void
tb_utp_state_change(void *userdata, int state);

/**
 * @brief Called when an error occurs with a uTP connection.
 *
 * @param userdata The userdata associated with this callback.
 * @param errcode The code for the error.
 */
void
tb_utp_error(void *userdata, int errcode);

/**
 * @brief Called when stuff happens on a uTP connection.
 *
 * @param The userdata associated with this callback.
 * @param send The send thing.
 * @param count The count thing.
 * @param type The type of send or count thing, I forget.
 */
void
tb_utp_overhead(void *userdata, int send, size_t count, int type);

/**
 * @brief Called when uTP decides to send some data via UDP.
 *
 * @param userdata The userdata associated with this callback.
 * @param p The buffer with data to send.
 * @param len The length of the data to send.
 * @param to The sockaddr struct with the peer information.
 * @param tolen The size of the sockaddr struct.
 */
void
tb_utp_send_to(void *userdata, const byte *p, size_t len,
		const struct sockaddr *to, socklen_t tolen);

/**
 * @brief callback, for incoming connections.
 *
 * Called by the uTP library when incoming data is deemed
 * to be data for a new connection.
 *
 * @param userdata The userdata associated with this callback.
 * @param socket The new UTP socket for the incoming conneciton.
 */
void
tb_utp_incoming(void *userdata, struct UTPSocket *socket);

/**
 * @brief Set the given option on a utp socket.
 *
 * Sets the provided value for the given option. Returns
 * 1 on success.
 *
 * @param opt The option to set.
 * @param val The value to set.
 *
 * @return -1 on error, otherwise 0.
 */
int
tb_utp_setsockopt(int opt, int val);

///////////// Standard socket API /////////////////////

/**
 * As stated eariler, most of the functions below were intended to be the
 * same as the sockets API. However, when we moved to a less abstract design
 * for the TestBed, most of this was changed.
 */

/**
 * @brief Create a uTP socket
 *
 * Creates a new utp socket for transmission. The domain, socktype and protocol
 * are all generally going to be the same, as we are using UDP as the underlying
 * layer 3 protocol in all cases.
 *
 * @param utp The utp context to create the socket for.
 * @param domain The domain for the connection.
 * @param socktype The type of socket to use.
 * @param protocol The protocol to use.
 */
int
tb_utp_socket(tb_utp_t *utp, int domain, int socktype, int protocol);

/**
 * @brief Connect using a uTP context.
 *
 * Create a connection with a uTP based server.
 *
 * @param utp The utp context to connect with.
 * @param addr The address to connect with.
 * @param len The length of the sockaddr.
 *
 * @return -1 on error, 0 otherwise.
 */
int
tb_utp_connect(tb_utp_t *utp, const struct sockaddr *addr, socklen_t len);

/**
 * @brief Send data using uTP
 *
 * Send data using, you guessed it, uTP!
 *
 * @param utp The context to use to send the data.
 * @param buf The buffer containing the data to send.
 * @param n The size of the data to send.
 *
 * @return -1 on error, 0 otherwise.
 */
int
tb_utp_send(tb_utp_t *utp, void *buf, size_t n);

/**
 * @brief Callback used by epoll events.
 *
 * The callback that is called when incoming events occur on the UDP socket
 * used by uTP.
 *
 * @param events The events occuring on the socket.
 * @param data A pointer to the utp context for this event.
 *
 * @return -1 on error, 0 otherwise.
 */
int
tb_utp_event(int events, void *data);

/**
 * @brief Receive data from a uTP socket.
 *
 * Used to receive data on a uTP connection.
 *
 * @param utp The utp context to receive data with.
 * @param buff The buffer to receive the data with.
 * @param size The size of the buffer.
 *
 * @return -1 on error, 0 otherwise.
 */
int
tb_utp_recv(tb_utp_t *utp, char *buff, int size);

/**
 * @brief Called when exiting uTP.
 *
 * Not currently used.
 *
 * @return pi to a million decimal places.
 */
int
tb_utp_funct_exit();

/**
 * @brief Called when errors occur.
 *
 * Used by the abstract network functions, when errors occur.
 *
 * @param value The value of the error.
 * @param err_no The value of errno.
 *
 * @return A sanitized error value.
 */
int
tb_utp_error_handle(int value, int err_no);

/**
 * @brief Called when closing a uTP connection.
 *
 * @param utp The uTP context to close.
 *
 * @return -1 on error, 0 otherwise.
 */
int
tb_utp_close(tb_utp_t *utp);

/////////////// Client Functions ////////////////////////

/**
 * @brief Run a client using multiple connections.
 *
 * Called to create and start a multi-connectio client with uTP.
 *
 * @param listener The listener to use when creating the client.
 *
 * @return The number of bytes sent.
 */
int
tb_utp_m_client(tb_listener_t *listener);

/**
 * @brief A single client connection.
 *
 * Used in multiple threads, each thread handling a single connection, one
 * of the four used in multiple connection clients. A pointer to this function
 * is passed into pthreads_create.
 *
 * @param data The session struct to use with this connection.
 *
 * @return A pointer to an int, the value of which is -1 on error, 0 otherwise.
 */
void
*tb_utp_m_client_conn(void *data);

/**
 * @brief Create and start a single connection client using uTP.
 *
 * Upload random data using a single connection uTP client.
 *
 * @param listener The listener to use when creating this client.
 *
 * @return The number of bytes sent.
 */
int
tb_utp_client(tb_listener_t *listener);

//////////////// Server Functions /////////////////////

/**
 * @brief Create a multiconnection server using uTP.
 *
 * Creates a multi-connection server using the uTP protocol. Not currently used.
 *
 * @param listener The listener to use to create the connection.
 *
 * @return The number of bytes received.
 */
int
tb_utp_m_server(tb_listener_t *listener);

/**
 * @brief Event called by epoll.
 *
 * Called when an event occurs on the UDP socket used by this uTP context.
 *
 * @param events The events that have occured on the socket.
 * @param data The uTP context associated with this socket.
 *
 * @return -1 on error, 0 otherwise.
 */
int
tb_utp_m_event(int events, void *data);

/**
 * @brief Called when creating a new connection.
 *
 * Not currently used, just a stub.
 *
 * @param userdata Teh userdata associated with this callback.
 * @param socket The UTP socket for this new connection.
 */
void
tb_utp_m_new_conn(void *userdata, struct UTPSocket *socket);

/**
 * @brief Run a single connection server with uTP.
 *
 * Starts and runs a single connection server using the uTP protocol.
 *
 * @param listener The listener to run the server with.
 *
 * @return The number of bytes received.
 */
int
tb_utp_server(tb_listener_t *listener);

#endif /* TB_UTP_WRAPPER_H_ */
