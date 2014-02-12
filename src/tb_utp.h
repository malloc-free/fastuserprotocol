/*
 * tb_utp_wrapper.h
 *
 *  Created on: 24/12/2013
 *      Author: michael
 */

#ifndef TB_UTP_WRAPPER_H_
#define TB_UTP_WRAPPER_H_

#include "tb_listener.h"
#include "tb_epoll.h"

#include <utp.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>

struct UTPSocket;

typedef struct
{
	int id;	///< Id for this utp struct.
	struct UTPSocket *socket; ///< The UTP socket.
	int sock_fd; ///< The UDP socket associated with the UTP socket.
	char *buffer; ///< The send buffer to be used for temporary data.
	char *rec_buffer; ///< The receive buffer.
	int read_bytes; ///< Total number of bytes
	int write_bytes; ///< the number of bytes written on this socket.
	int buffer_size; ///< The size of the buffer to use with this socket.
	int rec_buff_size; ///< The size of the receive buffer.
	int e_id; ///< The id of the epoll instance used with this utp socket.
	int recv_total; ///<
	int sent_total;
	int state;
	tb_epoll_t *epoll;
	struct sockaddr_storage *addr_s;
	struct UTPFunctionTable *call_backs;
	socklen_t addr_len;

	char *s_data;
	int s_data_size;

	char *r_data;
	int r_data_size;

	//Threading stuff
	pthread_mutex_t *lock;

	int so_sndbuf;	///< The size of the send buffer to use.
	int so_rcvbuf;  ///< The size of the receive buffer to use.
}
tb_utp_t;

/////////////// uTP management functions /////////////

typedef enum
{
	UTP_STATE_CREATED = 0,
	UTP_STATE_ERROR = 5
}
tb_utp_state;

/**
 * @brief Setup and create the tb_utp_t struct
 *
 * Sets up a struct for carrying utp information for transmission of data using
 * the utp protocol.
 *
 * @return tb_utp_t Struct with appropriate information for the utp protocol.
 */
tb_utp_t
*tb_utp_setup();

inline int
tb_utp_get_state() __attribute__((always_inline));

int
tb_utp_set_buffers(size_t send_buf, size_t recv_buf);

////////////// Functions for uTP ////////////////////

void
tb_utp_read(void *userdata, const byte *bytes, size_t count);

void
tb_utp_write(void *userdata, byte *bytes, size_t count);

size_t
tb_utp_get_Rcv_buff(void *userdata);

void
tb_utp_state_change(void *userdata, int state);

void
tb_utp_error(void *userdata, int errcode);

void
tb_utp_overhead(void *userdata, int send, size_t count, int type);

void
tb_utp_send_to(void *userdata, const byte *p, size_t len,
		const struct sockaddr *to, socklen_t tolen);

/**
 * @brief callback, for incoming connections.
 *
 * Called by the uTP library when incoming data is deemed
 * to be data for a new connection.
 */
void
tb_utp_incoming(void *userdata, struct UTPSocket *socket);

/**
 * @brief Set the given option on a utp socket.
 *
 * Sets the provided value for the given option. Returns
 * 1 on success.
 */
int
tb_utp_setsockopt(int opt, int val);

///////////// Standard socket API /////////////////////

int
tb_utp_socket(tb_utp_t *utp, int domain, int socktype, int protocol);

int
tb_utp_connect(tb_utp_t *utp, const struct sockaddr *addr, socklen_t len);

int
tb_utp_send(tb_utp_t *utp, void *buf, size_t n);

int
tb_utp_recv_data();

int
tb_utp_recv_from(int fd, void *buf, size_t n, unsigned int flags,
		const struct sockaddr *to, socklen_t *tolen);

/**
 * @brief Callback used by epoll events.
 */
int
tb_utp_event(int events, void *data);

/**
 * @brief Receive data from a uTP socket.
 */
int
tb_utp_recv(tb_utp_t *utp, char *buff, int size);

int
tb_utp_funct_exit();

int
tb_utp_error_handle(int value, int err_no);

int
tb_utp_close(tb_utp_t *utp);

/////////////// Client Functions ////////////////////////

/**
 * @brief Run a client using multiple connections.
 */
int
tb_utp_m_client(tb_listener_t *listener);

/**
 * @brief Run
 */
void
*tb_utp_m_client_conn(void *data);

/**
 * @brief Upload using the uTP protocol.
 *
 * Upload data using the uTP protocol.
 */
int
tb_utp_client(tb_listener_t *listener);

//////////////// Server Functions /////////////////////

/**
 * @brief Create a multiconnection server using uTP.
 */
int
tb_utp_m_server(tb_listener_t *listener);

/**
 * @brief Event called by epoll.
 */
int
tb_utp_m_event(int events, void *data);

/**
 * @brief Called when creating a new connection.
 */
void
tb_utp_m_new_conn(void *userdata, struct UTPSocket *socket);

/**
 * @brief Run a server with uTP
 */
int
tb_utp_server(tb_listener_t *listener);

#endif /* TB_UTP_WRAPPER_H_ */
