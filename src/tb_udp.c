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
		perror("Error: tb_upload_random_udp: getsockopt");
		tb_abort(listener);
	}

	while(listener->total_tx_rx < listener->file_size)
	{
		if(ioctl(listener->sock_d, TIOCOUTQ, &num_bytes) == -1)
		{
			perror("Error: tb_upload_random_udp: ioctl");
			tb_abort(listener);
		}

		if((snd_buff_sz - num_bytes) > listener->bufsize)
		{
			listener->total_tx_rx += tb_send_data(listener,
							listener->data + listener->total_tx_rx,
							listener->bufsize);
		}
	}

	LOG(listener, "Waiting for ack", LOG_INFO);

	//Send eof, and wait for ack.
	while(listener->command != TB_EXIT)
	{
		if(listener->protocol->f_send(listener->sock_d, listener->data,
				0, 0) < 0)
		{
			PRT_INFO("Connection closed on server side, exiting");
			break;
		}

		tb_poll_for_events(listener->epoll);
	}

	pthread_mutex_trylock(listener->stat_lock);

	LOG(listener, "Closing Connection", LOG_INFO);
	listener->protocol->f_close(listener->sock_d);
	listener->sock_d = -1;
	listener->status = TB_DISCONNECTED;

	pthread_mutex_unlock(listener->stat_lock);

	return listener->total_tx_rx;
}

int
tb_udp_ack(int events, void *data)
{
	PRT_INFO("Ack received");

	//Cast to listener, and call for an exit.
	tb_listener_t *listener = (tb_listener_t*)((tb_e_data*)data)->data;
	listener->command = TB_EXIT;

	return 0;
}

int
tb_udp_m_client(tb_listener_t *listener)
{
	tb_connect(listener);

	int sent = 0;

	struct msghdr msg;
	struct iovec vec;

	msg.msg_name = listener->addr_info->ai_addr;
	msg.msg_namelen = listener->addr_info->ai_addrlen;
	msg.msg_iov = &vec;
	msg.msg_iovlen = 1;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_flags = 0;

	do
	{
		vec.iov_base = listener->data + sent;
		vec.iov_len = 1024;

		if(listener->file_size - listener->total_tx_rx < 1024)
		{
			vec.iov_len = listener->file_size - listener->total_tx_rx;
		}

		sent = sendmsg(listener->sock_d, &msg, 0);

		if(sent == -1)
		{
			perror("tb_udp_send: sendmsg");
			return -1;
		}

		listener->total_tx_rx += sent;
	}
	while(listener->total_tx_rx < listener->file_size);

	vec.iov_base = 0;
	vec.iov_len = 0;

	sendmsg(listener->sock_d, &msg, 0);

	return 0;
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
	LOG(listener, "Connected", LOG_INFO);

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
			LOG(listener, "Sending ack", LOG_INFO);
			tb_send_to(listener, listener->curr_session);
		}
		else
		{
			listener->total_tx_rx += listener->curr_session->last_trans;
		}
	}

	LOG(listener, "Closing Connection", LOG_INFO);
	listener->protocol->f_close(listener->sock_d);
	listener->sock_d = -1;

	return listener->total_tx_rx;
}

int
tb_udp_epoll_server(tb_listener_t *listener)
{

}

int
tb_udp_m_server(tb_listener_t *listener)

{
	int rc;

	tb_set_epoll(listener);

	listener->status = TB_LISTENING;

	tb_session_t *session = tb_create_session();
	session->data = malloc(listener->bufsize);
	session->data_size = listener->bufsize;
	listener->curr_session = session;

	do
	{
		if(tb_poll_for_events(listener->epoll) < 0)
		{
			return -1;
		}
	}
	while(listener->curr_session->status != SESSION_COMPLETE);

	return 0;
}

int
tb_udp_event(int events, void *data)
{
	//PRT_ACK("Event received")

	int rc, sock_d;

	tb_e_data *e_data = (tb_e_data*)data;
	tb_listener_t *listener = (tb_listener_t*)e_data->data;
	sock_d = e_data->fd;

	tb_session_t *session = listener->curr_session;

	if(listener->status == TB_LISTENING)
	{
		listener->status = TB_CONNECTED;
	}

	struct msghdr msg;
	struct iovec vec;

	vec.iov_base = listener->curr_session->data;
	vec.iov_len = listener->curr_session->data_size;

	msg.msg_name = (struct sockaddr_t*)session->addr_in;
	msg.msg_namelen = (socklen_t)sizeof(session->addr_in);
	msg.msg_iov = &vec;
	msg.msg_iovlen = 1;
	msg.msg_flags = 0;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;

	//tb_worker_t *worker = tb_get_worker(listener, session);

	do
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
			listener->total_tx_rx += rc;
		}
	}
	while(rc != 0);

	session->status = SESSION_COMPLETE;

	return 0;
}
