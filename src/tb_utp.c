/*
 * tb_utp_utp.c
 *
 *  Created on: 24/12/2013
 *      Author: michael
 */

#include "tb_utp.h"
#include "tb_epoll.h"
#include "tb_common.h"
#include "tb_listener.h"
#include "tb_testbed.h"

#include <utp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

char animate[8] = {'|', '/', '-', '\\', '|', '/', '-', '\\'};
int a_index = 0;

tb_utp_t
*tb_utp_setup()
{
	PRT_INFO("utp setup called");

	tb_utp_t *utp = malloc(sizeof(tb_utp_t));

	utp->socket = NULL;
	utp->sock_fd = -1;

	struct UTPFunctionTable *callbacks =
			malloc(sizeof(struct UTPFunctionTable));

	callbacks->on_read = &tb_utp_read,
	callbacks->on_write = &tb_utp_write,
	callbacks->get_rb_size = &tb_utp_get_Rcv_buff,
	callbacks->on_state = &tb_utp_state_change,
	callbacks->on_error = &tb_utp_error,
	callbacks->on_overhead = &tb_utp_overhead;

	utp->call_backs = callbacks;
	utp->addr_len = (socklen_t)sizeof(struct sockaddr_storage);
	utp->addr_s = malloc(utp->addr_len);

	utp->rec_buffer = NULL;
	utp->buffer = NULL;
	utp->buffer_size = 0;
	utp->rec_buff_size = 0;

	utp->e_id = -1;

	utp->recv_total = 0;

	utp->state = UTP_STATE_CREATED;

	return utp;
}

////////////////// Functions for uTP ///////////////////

void
tb_utp_read(void *userdata, const byte *bytes, size_t count)
{
	tb_utp_t *utp = (tb_utp_t*)userdata;
	utp->read_bytes += count;
	utp->sent_total += count;
	//memcpy(utp->buffer, bytes, count);
}

void
tb_utp_write(void *userdata, byte *bytes, size_t count)
{
	tb_utp_t *utp = (tb_utp_t*)userdata;
	utp->write_bytes += count;
	utp->recv_total += count;
	memcpy(bytes, utp->s_data, count);
}

size_t
tb_utp_get_Rcv_buff(void *userdata)
{
	return 0;
}

void
tb_utp_state_change(void *userdata, int state)
{
	tb_utp_t *utp = (tb_utp_t*)userdata;
	utp->state = state;

#ifdef _DEBUG_UTP
	if(state == UTP_STATE_CONNECT)
	{
		PRT_INFO("State: Connected")
	}
	else if(state == UTP_STATE_EOF)
	{
		PRT_INFO("State: EOF")
	}
	else if(state == UTP_STATE_DESTROYING)
	{
		PRT_INFO("State: Destroying")
	}
#endif

}

void
tb_utp_error(void *userdata, int errcode)
{
	tb_utp_t *utp = (tb_utp_t*)userdata;
	utp->state = UTP_STATE_ERROR;

	fprintf(stderr, "Error: tb_utp_error: %s\n", strerror(errcode));
}

void
tb_utp_overhead(void *userdata, int send, size_t count, int type)
{

}

void
tb_utp_send_to(void *userdata, const byte *p, size_t len,
		const struct sockaddr *to, socklen_t tolen)
{
	tb_utp_t *utp = (tb_utp_t*)userdata;

	if(sendto(utp->sock_fd, p, len, 0, to, tolen) < 0)
	{
		perror("Error: tb_utp_send_to: send_to\n");
		exit(1);
	}
}

void
tb_utp_incoming(void *userdata, struct UTPSocket *socket)
{
	PRT_INFO("Incoming connection");
	tb_utp_t *utp = (tb_utp_t*)userdata;
	utp->socket = socket;
	UTP_SetSockopt(utp->socket, SO_RCVBUF, utp->so_rcvbuf);
	UTP_SetCallbacks(utp->socket, utp->call_backs, utp);
	utp->state = UTP_STATE_CONNECT;
}

/////////////// Standard socket API //////////////////////

int
tb_utp_socket(tb_utp_t *utp, int domain, int socktype, int protocol)
{
	assert(utp != NULL);

	utp->sock_fd = socket(domain, socktype, protocol);

	if(utp->sock_fd == -1)
	{
		perror("Error: tb_utp_socket: socket");
		return -1;
	}

	utp->epoll = tb_create_e_poll(utp->sock_fd, 5, 0, utp);
	utp->epoll->f_event = &tb_utp_event;
	utp->epoll->w_time = 50;

	return utp->sock_fd;
}

int
tb_utp_connect(tb_utp_t *utp, const struct sockaddr *addr, socklen_t len)
{
	PRT_INFO("Create socket");

	utp->socket = UTP_Create(&tb_utp_send_to, utp, addr, len);

	PRT_INFO("Set options");

	UTP_SetSockopt(utp->socket, SO_SNDBUF, utp->so_sndbuf);

	PRT_INFO("Set callbacks");

	UTP_SetCallbacks(utp->socket, utp->call_backs, utp);

	PRT_INFO("Connect");

	UTP_Connect(utp->socket);

	tb_poll_for_events(utp->epoll);

	return 0;
}

int
tb_utp_send(tb_utp_t *utp, void *buf, size_t n)
{
	int rc;

	utp->write_bytes = 0;
	utp->s_data = buf;

	UTP_CheckTimeouts();

	if(utp->state == UTP_STATE_CONNECT ||
			utp->state == UTP_STATE_WRITABLE)
	{
		rc = UTP_Write(utp->socket, n);

		if(rc == 0)
		{
			utp->state = 0;
		}
	}
	else
	{
		rc = tb_utp_recv(utp, utp->rec_buffer, utp->rec_buff_size);

		if(rc < 0)
		{
			return -1;
		}
	}

	return utp->write_bytes;
}

int
tb_utp_event(int events, void *data)
{
	tb_utp_t *utp = (tb_utp_t*)((tb_e_data*)data)->data;

	int len, rc;
	do{
		len = recvfrom(utp->sock_fd, utp->rec_buffer,
						utp->rec_buff_size, 0,
						(struct sockaddr*)utp->addr_s, &utp->addr_len);

		if(len < 0)
		{
			int err_no = errno;

			if(err_no == ECONNRESET || err_no == EMSGSIZE)
			{
				continue;
			}

			if(err_no == EWOULDBLOCK)
			{
				return 0;
			}

			return -1;
		}

		rc = UTP_IsIncomingUTP(&tb_utp_incoming, &tb_utp_send_to, utp,
								(const byte*)utp->rec_buffer, (size_t)len,
								(const struct sockaddr*)utp->addr_s, utp->addr_len);

		if(rc == 0)
		{
			fprintf(stderr, "Error: utp_recv_from: UTP_IsIncoming\n");
			return -1;
		}
	}
	while(len > 0);

	return 0;
}

int
tb_utp_recv(tb_utp_t *utp, char *buff, int size)
{
	utp->read_bytes = 0;
	tb_poll_for_events(utp->epoll);
	UTP_CheckTimeouts();

	return utp->read_bytes;
}

int
tb_utp_funct_exit()
{
	return 0;
}

int
tb_utp_error_handle(int value, int err_no)
{
	return value;
}

int
tb_utp_close(tb_utp_t *utp)
{
	fprintf(stdout, "Closing\n");

	if(utp->socket != NULL)
	{
		UTP_Close(utp->socket);
		UTP_CheckTimeouts();

		while(utp->state != UTP_STATE_DESTROYING)
		{
			tb_utp_recv(utp, utp->rec_buffer, utp->rec_buff_size);
			UTP_CheckTimeouts();
		}
	}

	if(utp->sock_fd != -1)
	{
		close(utp->sock_fd);
	}

	free(utp->buffer);
	free(utp->addr_s);
	free(utp->call_backs);
	free(utp);

	return 0;
}

///////////////////// Client Functions ////////////////////////

int
tb_utp_m_client(tb_listener_t *listener)
{
	const int num_conn = 2;


	pthread_mutex_t *utp_lock = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(utp_lock, NULL);

	tb_utp_t *utp = tb_utp_setup();

	PRT_INFO("UTP Creating socket");
	tb_utp_socket(utp, listener->addr_info->ai_family,
					listener->addr_info->ai_socktype,
					listener->addr_info->ai_protocol);

	utp->so_sndbuf = listener->options->l4_s_b_size;

	utp->buffer = malloc(1024 * 2);
	utp->buffer_size = 1024 * 2;

	utp->rec_buffer = malloc(1024 * 2);
	utp->rec_buff_size = 1024 * 2;

	if(setsockopt(utp->sock_fd, SOL_SOCKET, SO_RCVBUF,
					(void*)&listener->options->l3_r_b_size,
					sizeof(listener->options->l3_r_b_size)) != 0)
	{
		perror("Error: setsockopt: RCVBUF");
		return -1;
	}

	utp->lock = utp_lock;

	if(setsockopt(utp->sock_fd, SOL_SOCKET, SO_SNDBUF,
			(void*)&listener->options->l3_s_b_size,
			sizeof(listener->options->l3_s_b_size)) != 0)
	{
		perror("Error: setsockopt: SNDBUF");
		return -1;
	}

	int x = 0;

	for(; x < num_conn; x++)
	{
		tb_session_t *session = tb_create_session_full(listener->bind_address,
				listener->bind_port, SOCK_STREAM, 0);

		if(session == NULL)
		{
			PRT_ERR("Error creating session");
			tb_abort(listener);
		}

		//Create a clone of the socket, add id.
		tb_utp_t *clone_utp = malloc(sizeof(tb_utp_t));
		memcpy(clone_utp, utp, sizeof(tb_utp_t));
		clone_utp->id = x;

		//Set session parameters.
		session->l_status = (int*)&listener->status;
		session->l_txrx = &listener->total_tx_rx;

		//Allocate the session the appropriate amount of data
		//for the number of connections
		session->data = listener->data +
				((listener->file_size / num_conn) * x);
		session->data_size = listener->file_size / num_conn;

		fprintf(stdout, BLUE "listener %d allocated %zu bytes\n" RESET,
				session->id, session->data_size);

		session->stats->protocol = listener->protocol->protocol;
		session->n_session = NULL;
		session->pack_size = listener->bufsize;

		tb_session_add(listener->session_list, session);

		session->info = (void*)clone_utp;

		PRT_I_D("Creating thread for session %d", session->id);
		//Send the thread on its merry way.
		pthread_create(session->s_thread, NULL, &tb_utp_m_client_conn,
				(void*)session);
	}

	//Wait for connections, then declare connection status.
	while(listener->session_list->start->status == SESSION_CREATED);
	PRT_INFO("Listener: Connected");
	listener->status = TB_CONNECTED;

	int *retval;

	//Close and clear all sessions.
	tb_session_t *curr_session = listener->session_list->start;
	while(curr_session != NULL)
	{
		PRT_I_D("Joining with session %d",curr_session->id);
		pthread_join(*curr_session->s_thread, (void**)&retval);
		pthread_mutex_lock(curr_session->stat_lock);

		pthread_mutex_unlock(curr_session->stat_lock);

		curr_session = curr_session->n_session;
	}

	pthread_mutex_destroy(utp_lock);

	listener->status = TB_DISCONNECTED;

	return listener->total_tx_rx;
}

void
*tb_utp_m_client_conn(void *data)
{
	int *retval = malloc(sizeof(int)), sz;

	tb_session_t *session = (tb_session_t*)data;
	tb_utp_t *utp = (tb_utp_t*)session->info;

	PRT_I_D("Session %d: uTP Connecting", session->id);
	fprintf(stdout, "Session %d: utp id = %d\n", session->id, utp->id);

	pthread_mutex_trylock(utp->lock);
	if(tb_utp_connect(utp, session->addr_info->ai_addr,
			session->addr_info->ai_addrlen) < 0)
	{
		perror("Error: tb_utp_client: tb_utp_connect");
		*retval = -1;

		return retval;
	}

	pthread_mutex_unlock(utp->lock);

	session->status = SESSION_CONNECTED;

	sz = session->pack_size;

	PRT_I_D("Session %d: started connection", session->id);

	while(1)
	{
		if((session->data_size - session->total_bytes) < session->pack_size)
		{
			sz = session->data_size - session->total_bytes;
		}

		pthread_mutex_trylock(utp->lock);

		session->total_bytes +=
				tb_utp_send(utp, session->data +
						session->total_bytes, sz);

		pthread_mutex_unlock(utp->lock);

		if(session->total_bytes >= session->data_size)
		{
			break;
		}

		switch(utp->state)
		{
		case UTP_STATE_CONNECT:
			PRT_INFO("UTP Connected");
			session->status = SESSION_CONNECTED;
			break;
		case UTP_STATE_ERROR:
			PRT_ERR("UTP Error");

			*retval = -1;
			return retval;
		}
	}

	pthread_mutex_trylock(session->stat_lock);

	session->status = SESSION_DISCONNECTED;

	pthread_mutex_unlock(session->stat_lock);

	fprintf(stdout, "Session %d sent %lld bytes", session->id,
			session->total_bytes);
	*retval = 0;
	return retval;
}

int
tb_utp_client(tb_listener_t *listener)
{
	tb_utp_t *utp = tb_utp_setup();

	PRT_INFO("UTP Creating socket");

	tb_utp_socket(utp, listener->addr_info->ai_family,
			listener->addr_info->ai_socktype,
			listener->addr_info->ai_protocol);

	utp->so_sndbuf = listener->options->l4_s_b_size;

	utp->buffer = malloc(1024 * 2);
	utp->buffer_size = 1024 * 2;

	utp->rec_buffer = malloc(1024 * 2);
	utp->rec_buff_size = 1024 * 2;

	if(setsockopt(utp->sock_fd, SOL_SOCKET, SO_RCVBUF,
					(void*)&listener->options->l3_r_b_size,
					sizeof(listener->options->l3_r_b_size)) != 0)
	{
		perror("Error: setsockopt: RCVBUF");
		return -1;
	}

	if(setsockopt(utp->sock_fd, SOL_SOCKET, SO_SNDBUF,
			(void*)&listener->options->l3_s_b_size,
			sizeof(listener->options->l3_s_b_size)) != 0)
	{
		perror("Error: setsockopt: SNDBUF");
		return -1;
	}

	PRT_INFO("uTP Connecting");
	if(tb_utp_connect(utp, (struct sockaddr*)listener->addr_info->ai_addr,
			listener->addr_info->ai_addrlen) < 0)
	{
		perror("Error: tb_utp_client: tb_utp_connect");
		tb_abort(listener);
	}

	listener->status = TB_CONNECTED;
	listener->sock_d = utp->sock_fd;

	int sz = listener->bufsize;

	while(listener->command != TB_EXIT && listener->command != TB_ABORT)
	{
		if((listener->file_size - listener->total_tx_rx) < listener->bufsize)
		{
			sz = listener->file_size - listener->total_tx_rx;
		}

		listener->total_tx_rx +=
				tb_utp_send(utp, listener->data +
						listener->total_tx_rx, sz);

		if(listener->total_tx_rx >= listener->file_size)
		{
			break;
		}

		switch(utp->state)
		{
		case UTP_STATE_CONNECT:
			PRT_INFO("UTP Connected");
			listener->status = TB_CONNECTED;
			break;
		case UTP_STATE_ERROR:
			PRT_ERR("UTP Error");
			tb_abort(listener);
			break;
		}
	}

	pthread_mutex_trylock(listener->stat_lock);

	listener->status = TB_DISCONNECTED;
	tb_utp_close(utp);
	listener->sock_d = -1;

	pthread_mutex_unlock(listener->stat_lock);

	return listener->total_tx_rx;
}

////////////////// Server Functions ///////////////////////////

int
tb_utp_m_server(tb_listener_t *listener)
{

	return 0;
}

int
tb_utp_m_event(int events, void *data)
{
	tb_utp_t *utp = (tb_utp_t*)((tb_e_data*)data)->data;

	assert(utp->rec_buffer != NULL && utp->rec_buff_size != 0);

	int len, rc;
	do{

		len = recvfrom(utp->sock_fd, utp->rec_buffer,
						utp->rec_buff_size, 0,
						(struct sockaddr*)utp->addr_s, &utp->addr_len);

		if(len < 0)
		{
			int err_no = errno;

			if(err_no == EWOULDBLOCK)
			{
				return 0;
			}

			return -1;
		}

		rc = UTP_IsIncomingUTP(&tb_utp_m_new_conn, &tb_utp_send_to, utp,
								(const byte*)utp->rec_buffer, (size_t)len,
								(const struct sockaddr*)utp->addr_s, utp->addr_len);

		if(rc == 0)
		{
			fprintf(stderr, "Error: utp_recv_from: UTP_IsIncoming\n");
			return -1;
		}
	}
	while(len > 0);

	return 0;
}

void
tb_utp_m_new_conn(void *userdata, struct UTPSocket *socket)
{

}

int
tb_utp_server(tb_listener_t *listener)
{
	tb_utp_t *utp = tb_utp_setup();

	tb_utp_socket(utp, listener->addr_info->ai_family,
				listener->addr_info->ai_socktype,
				listener->addr_info->ai_protocol);

	PRT_INFO("Creating socket");
	listener->sock_d = utp->sock_fd;
	listener->curr_session = tb_create_session();
	listener->curr_session->sock_d = listener->sock_d;

	PRT_INFO("Binding socket");
	if(bind(utp->sock_fd, listener->addr_info->ai_addr,
			listener->addr_info->ai_addrlen) < 0)
	{
		perror("Error: tb_utp_server: bind");
		tb_abort(listener);
	}

	//Set the l4 buffers for uDT.
	utp->so_rcvbuf = listener->options->l4_r_b_size;

	utp->buffer = malloc(1024 * 2);
	utp->buffer_size = 1024 * 2;

	utp->rec_buffer = malloc(1024 * 2);
	utp->rec_buff_size = 1024 * 2;

	if(setsockopt(utp->sock_fd, SOL_SOCKET, SO_RCVBUF,
						(void*)&listener->options->l3_r_b_size,
						sizeof(listener->options->l3_r_b_size)) != 0)
	{
		perror("Error: setsockopt: RCVBUF");
		return -1;
	}



	if(setsockopt(utp->sock_fd, SOL_SOCKET, SO_SNDBUF,
			(void*)&listener->options->l3_s_b_size,
			sizeof(listener->options->l3_s_b_size)) != 0)
	{
		perror("Error: setsockopt: SNDBUF");
		return -1;
	}

	listener->status = TB_LISTENING;

	char *buffer = malloc(listener->bufsize);

	assert(buffer != NULL);

	PRT_INFO("Waiting for connection");
	while(listener->command != TB_ABORT && listener->command != TB_EXIT)
	{
		listener->total_tx_rx +=
				tb_utp_recv(utp, buffer, listener->bufsize);


		switch(utp->state)
		{
		case UTP_STATE_CONNECT:
			listener->status = TB_CONNECTED;
			break;

		case UTP_STATE_EOF:
			PRT_INFO("UTP eof");
			listener->command = TB_EXIT;
			break;

		case UTP_STATE_ERROR:
			PRT_ERR("UTP error");
			pthread_mutex_trylock(listener->stat_lock);

			tb_utp_close(utp);
			listener->status = TB_DISCONNECTED;
			listener->sock_d = -1;

			pthread_mutex_unlock(listener->stat_lock);

			free(buffer);
			tb_abort(listener);
		}
	}

	//Lock and disconnect.
	pthread_mutex_trylock(listener->stat_lock);

	tb_utp_close(utp);
	listener->status = TB_DISCONNECTED;
	listener->sock_d = -1;

	pthread_mutex_unlock(listener->stat_lock);

	free(buffer);

	return listener->total_tx_rx;
}

