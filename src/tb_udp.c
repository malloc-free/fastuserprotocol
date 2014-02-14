/*
 * tb_udp.c
 *
 *  Created on: 9/01/2014
 *      Author: michael
 */

#include "tb_udp.h"
#include "tb_epoll.h"
#include "tb_session.h"
#include "tb_common.h"
#include "tb_listener.h"
#include "tb_worker.h"
#include "tb_utp.h"
#include "tb_testbed.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <time.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <udt.h>
#include <errno.h>

const int DATA_SIZE = 1024;

int
tb_udp_client(tb_listener_t *listener)
{
	tb_connect(listener);

	listener->status = TB_CONNECTED;

	int num_bytes, rc, snd_buff_sz;

	listener->epoll = tb_create_e_poll(listener->sock_d, 5, 0,
			listener);

	//Give the server 50 milliseconds to respond to the eof.
	listener->epoll->w_time = 50;
	listener->epoll->f_event = &tb_udp_ack;

	socklen_t sz = (socklen_t)sizeof(snd_buff_sz);

	rc = getsockopt(listener->sock_d, SOL_SOCKET, SO_SNDBUF, &snd_buff_sz,
			&sz);

	if(rc == -1)
	{
		LOG_E_NO(listener, "Error: tb_upload_random_udp: getsockopt", errno);
		tb_abort(listener);
	}

	while(listener->total_tx_rx < listener->file_size)
	{
		if(ioctl(listener->sock_d, TIOCOUTQ, &num_bytes) == -1)
		{
			LOG_E_NO(listener, "Error: tb_upload_random_udp: ioctl", errno);
			tb_abort(listener);
		}

		if((snd_buff_sz - num_bytes) > listener->bufsize)
		{
			rc = send(listener->sock_d, listener->data + listener->total_tx_rx,
							listener->bufsize, 0);

			if(rc < 0)
			{
				LOG_E_NO(listener, "Error: send: ", errno);
				tb_abort(listener);
			}
			else
			{
				listener->total_tx_rx += rc;
			}
		}
	}

	LOG_INFO(listener, "Waiting for ack");

	//Send eof, and wait for ack.
	while(listener->command != TB_EXIT)
	{
		if(send(listener->sock_d, listener->data, 0, 0) < 0)
		{
			int err_no = errno;

			if(err_no != EWOULDBLOCK)
			{
				LOG_E_NO(listener, "Error: udp_send: ", errno);
				break;
			}
		}

		tb_poll_for_events(listener->epoll);
	}

	pthread_mutex_trylock(listener->stat_lock);

	LOG_INFO(listener, "Closing Connection");
	listener->protocol->f_close(listener->sock_d);
	listener->sock_d = -1;
	listener->status = TB_DISCONNECTED;

	pthread_mutex_unlock(listener->stat_lock);

	return listener->total_tx_rx;
}

int
tb_udp_ack(int events, void *data)
{
	//Cast to listener, and call for an exit.
	tb_listener_t *listener = (tb_listener_t*)((tb_e_data*)data)->data;
	LOG_INFO(listener, "Ack received");
	listener->command = TB_EXIT;

	return 0;
}

int
tb_udp_m_client(tb_listener_t *listener)
{
	const int num_conn = 1;
	int x = 0;

	for(; x < num_conn; x++)
	{
		tb_session_t *session = tb_create_session_full(listener->bind_address,
				listener->bind_port, SOCK_DGRAM, 0);

		if(session == NULL)
		{
			LOG_E_NO(listener, "Error creating session", errno);
			tb_abort(listener);
		}

		session->sock_d = socket(session->addr_info->ai_family,
					session->addr_info->ai_socktype, session->addr_info->ai_protocol);

		if(session->sock_d == -1)
		{
			perror("Error: tb_stream_client: ");
			tb_abort(listener);
		}

		//Set session parameters.
		session->l_status = (int*)&listener->status;
		session->l_txrx = &listener->total_tx_rx;

		//Allocate the session the appropriate amount of data
		//for the number of connections
		session->data = listener->data +
				((listener->file_size / num_conn) * x);
		session->data_size = listener->file_size / num_conn;

		fprintf(stdout, BLUE "listener %d allocated %zu bytes" RESET,
				session->id, session->data_size);

		session->stats->protocol = listener->protocol->protocol;
		session->n_session = NULL;
		session->pack_size = listener->bufsize;


		//Add this session to the end of the list, and to the start
		//if the list is empty.
		session->id = ++listener->session_list->current_max_id;

		PRT_I_D("Adding session %d", session->id);
		if(listener->session_list->start == NULL)
		{
			listener->session_list->start = session;
			listener->session_list->end = session;
		}
		else
		{
			listener->session_list->end->n_session = session;
			listener->session_list->end = session;
		}

		PRT_I_D("Creating thread for session %d", session->id);
		//Send the thread on its merry way.
		pthread_create(session->s_thread, NULL, &tb_udp_m_client_conn,
				(void*)session);
	}
	while(listener->session_list->start->status == SESSION_CREATED);
	listener->status = TB_CONNECTED;

	int *retval;

	tb_session_t *curr_session = listener->session_list->start;

	//Wait for each session to complete, then close the connection.
	while(curr_session != NULL)
	{
		PRT_I_D("Joining with session %d",curr_session->id);
		pthread_join(*curr_session->s_thread, (void**)&retval);
		pthread_mutex_lock(curr_session->stat_lock);
		close(curr_session->sock_d);
		pthread_mutex_unlock(curr_session->stat_lock);

		curr_session = curr_session->n_session;
	}

	listener->status = TB_DISCONNECTED;

	return listener->total_tx_rx;
}

void
*tb_udp_m_client_conn(void *data)
{
	int *retval = malloc(sizeof(int));

	tb_session_t *session = (tb_session_t*)data;

	connect(session->sock_d, session->addr_info->ai_addr,
			session->addr_info->ai_addrlen);

	session->status = SESSION_CONNECTED;

	int sent = 0;

	struct msghdr msg;
	struct iovec vec;

	msg.msg_name = session->addr_info->ai_addr;
	msg.msg_namelen = session->addr_info->ai_addrlen;
	msg.msg_iov = &vec;
	msg.msg_iovlen = 1;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_flags = 0;

	do
	{
		vec.iov_base = session->data + sent;
		vec.iov_len = session->pack_size;

		if(session->data_size - session->total_bytes < session->pack_size)
		{
			vec.iov_len = session->data_size - session->total_bytes;
		}

		sent = sendmsg(session->sock_d, &msg, 0);

		if(sent == -1)
		{
			perror("tb_udp_send: sendmsg");
			*retval = -1;
			return retval;
		}

		session->total_bytes += sent;
	}
	while(session->total_bytes < session->data_size);

	session->status = SESSION_COMPLETE;

	*retval = 0;

	return retval;
}
///////////////////// UDP Server Functions //////////////////////

int
tb_udp_server(tb_listener_t *listener)
{
	time_t s_time, f_time;

	listener->status = TB_LISTENING;

	listener->curr_session = tb_create_server_session();
	listener->curr_session->data = malloc(listener->bufsize);
	listener->curr_session->data_size = listener->bufsize;

	tb_recv_from(listener, listener->curr_session);

	listener->total_tx_rx += listener->curr_session->last_trans;

	s_time = time(0);
	listener->status = TB_CONNECTED;
	LOG_INFO(listener, "UDP Connected");

	while(listener->command != TB_ABORT && listener->command != TB_EXIT)
	{
		tb_recv_from(listener, listener->curr_session);

		if(listener->curr_session->last_trans == listener->protocol->eot)
		{
			f_time = time(0);
			listener->sec = difftime(f_time, s_time);
			listener->status = TB_LISTENING;
			listener->command = listener->s_tx_end;
			PRT_INFO("Sending ack");
			LOG_INFO(listener, "UDP Sending ack");
			tb_send_to(listener, listener->curr_session);
		}
		else
		{
			listener->total_tx_rx += listener->curr_session->last_trans;
		}
	}

	LOG_INFO(listener, "Closing UDP Connection");

	pthread_mutex_lock(listener->stat_lock);
	close(listener->sock_d);
	listener->sock_d = -1;
	pthread_mutex_unlock(listener->stat_lock);

	return listener->total_tx_rx;
}

int
tb_udp_m_server(tb_listener_t *listener)

{
	listener->status = TB_LISTENING;

	tb_epoll_t *epoll = tb_create_e_poll(listener->sock_d, 10, 0,
			(void*)listener);

	tb_session_t *session = tb_create_session();
	session->data = malloc(listener->bufsize);
	session->data_size = listener->bufsize;
	tb_session_add(listener->session_list, session);
	epoll->w_time = 50;
	epoll->f_event = &tb_udp_event;

	do
	{
		if(tb_poll_for_events(epoll) < 0)
		{
			return -1;
		}
	}
	while(listener->session_list->start->status != SESSION_COMPLETE);

	return 0;
}

int
tb_udp_event(int events, void *data)
{
	int rc, sock_d;

	tb_e_data *e_data = (tb_e_data*)data;
	tb_listener_t *listener = (tb_listener_t*)e_data->data;
	sock_d = e_data->fd;

	tb_session_t *session = listener->session_list->start;

	if(listener->status == TB_LISTENING)
	{
		listener->status = TB_CONNECTED;
	}

	struct msghdr msg;
	struct iovec vec;

	vec.iov_base = session->data;
	vec.iov_len = session->data_size;

	msg.msg_name = (struct sockaddr_t*)session->addr_in;
	msg.msg_namelen = (socklen_t)session->addr_len;
	msg.msg_iov = &vec;
	msg.msg_iovlen = 1;
	msg.msg_flags = 0;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;

	while(1)
	{
		rc = recvmsg(sock_d, &msg, 0);

		if(rc < 0)
		{
			int err_no = errno;

			if(err_no == EWOULDBLOCK)
			{
				return 0;
			}
			else
			{
				return -1;
			}
		}
		else if(rc > 0)
		{
			session->total_bytes += rc;
		}
	}

	session->status = SESSION_COMPLETE;

	return 0;
}
