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
#include <pthread.h>

//////////////////// Data Structure Functions /////////////////////

void
tb_queue_add(tb_queue_t *queue, tb_queue_data_t *data)
{
	 if(!queue->start)
	 {
		 queue->start = data;
		 queue->end = data;
	 }
	 else
	 {
		 queue->end->n_data = data;
		 queue->end = data;
	 }
}

/////////////////// UDP Session Functions ////////////////////////

tb_udp_session_t
*tb_create_udp_session()
{
	tb_udp_session_t *session = malloc(sizeof(tb_udp_session_t));

	return session;
}

void
tb_destroy_udp_session(tb_udp_session_t *session)
{
	pthread_cond_destroy(&session->data_cond);
	pthread_mutex_destroy(&session->data_lock);
	free(session);
}

/////////////////// Client Functions /////////////////////////////

int
tb_udp_client(tb_listener_t *listener)
{
	tb_start_time(listener->connect_time);
	tb_connect(listener);
	tb_finish_time(listener->connect_time);

	listener->status = TB_CONNECTED;

	int num_bytes, rc, snd_buff_sz, sz;

	sz = listener->bufsize;

	listener->epoll = tb_create_e_poll(listener->sock_d, 5, 0,
			listener);

	//Give the server 50 milliseconds to respond to the eof.
	listener->epoll->w_time = 50;
	listener->epoll->f_event = &tb_udp_ack;

	socklen_t buf_sz = (socklen_t)sizeof(snd_buff_sz);

	rc = getsockopt(listener->sock_d, SOL_SOCKET, SO_SNDBUF, &snd_buff_sz,
			&buf_sz);

	if(rc == -1)
	{
		LOG_E_NO(listener, "Error: tb_upload_random_udp: getsockopt", errno);
		tb_abort(listener);
	}

	//Main loop, start timing.
	tb_start_time(listener->transfer_time);
	while(listener->total_tx_rx < listener->file_size)
	{
		if(ioctl(listener->sock_d, TIOCOUTQ, &num_bytes) == -1)
		{
			LOG_E_NO(listener, "Error: tb_upload_random_udp: ioctl", errno);
			tb_abort(listener);
		}

		if((listener->file_size - listener->total_tx_rx) < listener->bufsize)
		{
			sz = listener->file_size - listener->total_tx_rx;
		}

		if((snd_buff_sz - num_bytes) > sz)
		{
			rc = send(listener->sock_d, listener->data + listener->total_tx_rx,
							sz, 0);

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
	tb_finish_time(listener->transfer_time);

	LOG_INFO(listener, "Waiting for ack");

	//Send eof, and wait for ack. If it would have blocked, ignore and
	//continue to wait.
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

	pthread_mutex_trylock(&listener->stat_lock);

	LOG_INFO(listener, "Closing Connection");
	listener->protocol->f_close(listener->sock_d);
	listener->sock_d = -1;
	listener->status = TB_DISCONNECTED;

	pthread_mutex_unlock(&listener->stat_lock);

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

/**
 * Let's play the 'spot the copypasta. Yeah should work on making things
 * a bit more abstract :-)
 */
int
tb_udp_m_client(tb_listener_t *listener)
{
	const int num_conn = 4;
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

		if(listener->stats->n_stats == NULL)
		{
			listener->stats->n_stats = session->stats;
		}

		PRT_I_D("Creating thread for session %d", session->id);

		//Send the thread on its merry way.
		pthread_create(&session->s_thread, NULL, &tb_udp_m_client_conn,
				(void*)session);
	}

	//Spin and wait. Controlled using an enum. Then start timing connection.
	while(listener->session_list->start->status == SESSION_CREATED);
	tb_start_time(listener->transfer_time);
	listener->status = TB_CONNECTED;

	int *retval;

	tb_session_t *curr_session = listener->session_list->start;

	//Wait for each session to complete, then close the connection.
	while(curr_session != NULL)
	{
		PRT_I_D("Joining with session %d",curr_session->id);
		pthread_join(curr_session->s_thread, (void**)&retval);
		pthread_mutex_lock(&curr_session->stat_lock);
		close(curr_session->sock_d);
		pthread_mutex_unlock(&curr_session->stat_lock);

		free(retval);
		curr_session = curr_session->n_session;
	}
	tb_finish_time(listener->transfer_time);

	//Close the connection.
	pthread_mutex_lock(&listener->stat_lock);

	tb_udp_m_close_conn(listener);
	listener->status = TB_DISCONNECTED;
	pthread_mutex_lock(&listener->stat_lock);

	return listener->total_tx_rx;
}

void
tb_udp_m_close_conn(tb_listener_t *listener)
{
	listener->epoll = tb_create_e_poll(listener->sock_d, 5, 0,
				listener);

	//Give the server 50 milliseconds to respond to the eof.
	listener->epoll->w_time = 50;
	listener->epoll->f_event = &tb_udp_ack;

	struct msghdr msg;
	struct iovec vec[3];
	msg.msg_name = listener->addr_info->ai_addr;
	msg.msg_namelen = listener->addr_info->ai_addrlen;
	msg.msg_iov = vec;
	msg.msg_iovlen = 3;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_flags = 0;

	vec[0].iov_base = 0;
	vec[0].iov_len = 0;
	vec[1].iov_base = 0;
	vec[1].iov_len = 0;
	vec[2].iov_base = 0;
	vec[2].iov_len = 0;

	//Send eof, and wait for ack. If it would have blocked, ignore and
	//continue to wait.
	while(listener->command != TB_EXIT)
	{
		if(sendmsg(listener->sock_d, &msg, 0) < 0)
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
}

void
*tb_udp_m_client_conn(void *data)
{
	int *retval = malloc(sizeof(int));

	tb_session_t *session = (tb_session_t*)data;

	//Connect, time.
	tb_start_time(session->transfer_t);
	if(connect(session->sock_d, session->addr_info->ai_addr,
			session->addr_info->ai_addrlen) < 0)
	{
		*retval = -1;
		return retval;
	}
	tb_finish_time(session->transfer_t);

	session->status = SESSION_CONNECTED;

	int sent = 0;

	struct msghdr msg;
	struct iovec vec[3];

	msg.msg_name = session->addr_info->ai_addr;
	msg.msg_namelen = session->addr_info->ai_addrlen;
	msg.msg_iov = vec;
	msg.msg_iovlen = 3;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_flags = 0;

	int id = session->id, sz = 0;

	vec[0].iov_base = &id;
	vec[0].iov_len = sizeof(int);

	vec[1].iov_base = &sz;
	vec[1].iov_len = sizeof(int);

	//Start timing, send data.
	tb_start_time(session->transfer_t);
	do
	{
		vec[2].iov_base = session->data + sent;
		vec[2].iov_len = session->pack_size;

		if(session->data_size - session->total_bytes < session->pack_size)
		{
			vec[2].iov_len = session->data_size - session->total_bytes;
		}

		*(int*)vec[1].iov_base = vec[2].iov_len;

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
	tb_finish_time(session->transfer_t);

	//Send exit message, and bail.
	sendmsg(session->sock_d, &msg, 0);
	session->status = SESSION_COMPLETE;

	*retval = 0;
	return retval;
}

///////////////////// UDP Server Functions //////////////////////

int
tb_udp_server(tb_listener_t *listener)
{
	listener->status = TB_LISTENING;

	listener->curr_session = tb_create_server_session();
	listener->curr_session->data = malloc(listener->bufsize);
	listener->curr_session->data_size = listener->bufsize;

	tb_recv_from(listener, listener->curr_session);

	listener->total_tx_rx += listener->curr_session->last_trans;

	listener->status = TB_CONNECTED;
	LOG_INFO(listener, "UDP Connected");

	//Start timing, receive data.
	tb_start_time(listener->transfer_time);
	while(listener->command != TB_ABORT && listener->command != TB_EXIT)
	{
		tb_recv_from(listener, listener->curr_session);

		if(listener->curr_session->last_trans == listener->protocol->eot)
		{
			PRT_INFO("Sending ack");
			LOG_INFO(listener, "UDP Sending ack");
			tb_send_to(listener, listener->curr_session);
			break;
		}
		else
		{
			listener->total_tx_rx += listener->curr_session->last_trans;
		}
	}
	tb_finish_time(listener->transfer_time);

	LOG_INFO(listener, "Closing UDP Connection");

	//Lock 'n' close.
	pthread_mutex_lock(&listener->stat_lock);
	close(listener->sock_d);
	listener->sock_d = -1;
	listener->status = TB_DISCONNECTED;
	pthread_mutex_unlock(&listener->stat_lock);

	return listener->total_tx_rx;
}

int
tb_udp_m_server(tb_listener_t *listener)

{
	listener->status = TB_LISTENING;

	tb_epoll_t *epoll = tb_create_e_poll(listener->sock_d, 10, 0,
			(void*)listener);

	//Setup session data.
	listener->curr_session = tb_create_server_session();
	listener->curr_session->data = malloc(listener->bufsize);
	listener->curr_session->data_size = listener->bufsize;

	epoll->w_time = 50;
	epoll->f_event = &tb_udp_event;

	//Start polling
	while(!(listener->command & TB_E_LOOP))
	{
		if(tb_poll_for_events(epoll) < 0)
		{
			return -1;
		}
	}

	tb_finish_time(listener->transfer_time);

	//Lock and disconnect.
	pthread_mutex_lock(&listener->stat_lock);
	close(listener->sock_d);
	listener->sock_d = -1;
	listener->status = TB_DISCONNECTED;
	pthread_mutex_lock(&listener->stat_lock);

	LOG_INFO(listener, "exiting m server");

	return 0;
}

int
tb_udp_event(int events, void *data)
{
	int rc, sock_d;

	tb_e_data *e_data = (tb_e_data*)data;
	tb_listener_t *listener = (tb_listener_t*)e_data->data;
	sock_d = e_data->fd;

	tb_session_t *l_session = listener->curr_session;

	if(listener->status == TB_LISTENING)
	{
		listener->status = TB_CONNECTED;
		tb_start_time(listener->transfer_time);
	}

	struct msghdr msg;
	struct iovec vec[3];

	//Setup message.
	int id = -1, sz = 0;

	vec[0].iov_base = &id;
	vec[0].iov_len = sizeof(int);

	vec[1].iov_base = &sz;
	vec[1].iov_len = sizeof(int);

	vec[2].iov_base = l_session->data;
	vec[2].iov_len = l_session->data_size;

	msg.msg_name = (struct sockaddr_t*)l_session->addr_in;
	msg.msg_namelen = (socklen_t)l_session->addr_len;
	msg.msg_iov = vec;
	msg.msg_iovlen = 2;
	msg.msg_flags = 0;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;

	while(1)
	{
		rc = recvmsg(sock_d, &msg, 0);
		int id = *(int*)vec[0].iov_base;

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
		else if(*(int*)vec[1].iov_base > 0)
		{
			tb_session_t *session =
					tb_session_list_search(listener->session_list, id);

			//If no session exists for this id, create one.
			if(!session)
			{
				session = tb_create_session();
				session->id = id;
				tb_session_add_to(listener->session_list, session);
				tb_session_list_inc(listener->session_list);
				PRT_I_D("Created session %d", session->id);
			}

			//If the session attached to the listener is null, set it now.
			if(l_session->n_session == NULL)
			{
				l_session->n_session = session;
			}

			session->total_bytes += rc;
		}
		else
		{
			PRT_INFO("Call for exit");

			vec[0].iov_base = 0;
			vec[0].iov_len = 0;
			vec[1].iov_base = 0;
			vec[1].iov_len = 0;
			vec[2].iov_base = 0;
			vec[2].iov_len = 0;

			sendmsg(listener->sock_d, &msg, 0);
			listener->command = TB_EXIT;
			break;
		}
	}

	return 0;
}

