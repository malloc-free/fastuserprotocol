/*
 * tb_stream.c
 *
 *  Created on: 7/02/2014
 *      Author: michael
 */
#include "tb_stream.h"

#include "tb_protocol.h"
#include "tb_listener.h"
#include "tb_testbed.h"
#include "tb_common.h"
#include "tb_udp.h"
#include "tb_utp.h"
#include "udt.h"
#include "tb_testbed.h"
#include "tb_session.h"

#include <utp.h>
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
#include <errno.h>

/////////////////////////// Server Functions ///////////////////////

int
tb_stream_server(tb_listener_t *listener)
{
	PRT_INFO("Listening");
	tb_listen(listener);
	listener->status = TB_LISTENING;

	while(!(listener->command & TB_E_LOOP)){
		PRT_INFO("Accepting");
		LOG(listener, "Accepting", LOG_INFO);

		tb_session_t *session = tb_create_server_session();

		session->sock_d = accept(listener->sock_d,
				(struct sockaddr*)session->addr_in,
				&session->addr_len);

		if(session->sock_d == -1)
		{
			PRT_ERR("Error: tb_stream_server: accept");
			tb_abort(listener);
		}

		LOG_ADD(listener, "Connected address:", session->addr_in);
		session->data = malloc(listener->bufsize);
		session->data_size = listener->bufsize;
		listener->curr_session = session;
		listener->status = TB_CONNECTED;

		listener->sec = 0;
		time_t s_time = time(0);

		do
		{
			tb_recv_data(listener, session);

			session->total_bytes += session->last_trans;
			listener->total_tx_rx += session->last_trans;
		}
		while(session->last_trans != 0 && !(listener->command & TB_E_LOOP));

		time_t f_time = time(0);
		listener->sec = difftime(f_time, s_time);

		//Lock while destroying session and closing connection
		pthread_mutex_trylock(listener->stat_lock);

		fprintf(stdout, "Session closed\n");
		listener->status = TB_DISCONNECTED;

		//Close and destroy current session
		close(session->sock_d);
		tb_destroy_session(session);
		listener->curr_session = NULL;
		PRT_INFO("Session destroyed");

		//Close connection
		listener->protocol->f_close(session->sock_d);

		pthread_mutex_unlock(listener->stat_lock);

		listener->command = listener->s_tx_end;
	}

	listener->protocol->f_close(listener->sock_d);
	listener->sock_d = -1;

	return listener->total_tx_rx;
}

int
tb_stream_m_server(tb_listener_t *listener)
{
	tb_listen(listener);
	listener->status = TB_LISTENING;
	tb_epoll_t *epoll = tb_create_e_poll(listener->sock_d, 10, 0, (void*)listener);
	epoll->f_event = &tb_stream_event;
	epoll->w_time = 50;

	while(!(listener->command & TB_E_LOOP))
	{
		tb_poll_for_events(epoll);
	}

	return listener->total_tx_rx;
}

int
tb_stream_event(int events, void *data)
{
	PRT_INFO("Event called");

	tb_e_data *e_data = (tb_e_data*)data;
	tb_listener_t *listener = e_data->data;

	assert(listener->sock_d == e_data->fd);

	while(1)
	{
		tb_session_t *session = tb_create_session();

		tb_start_time(session->connect_t);
		session->sock_d = accept(listener->sock_d, (struct sockaddr*)session->addr_in,
				&session->addr_len);
		tb_finish_time(session->connect_t);

		if(session->sock_d == -1)
		{
			perror("Error: tb_stream_event: ");
			tb_destroy_session(session);

			return 0;
		}

		session->pack_size = listener->bufsize;
		session->data = malloc(listener->bufsize);

		session->id = ++listener->session_list->current_max_id;
		if(listener->session_list->start == NULL)
		{
			listener->session_list->start = session;
			listener->session_list->end = session;
		}
		else
		{
			listener->session_list->start->n_session = session;
			listener->session_list->end = session;
		}

		PRT_INFO("Starting session");
		pthread_create(session->s_thread, NULL, &tb_stream_m_server_conn,
				(void*)session);

		listener->status = TB_CONNECTED;
	}

	return 0;
}

void
*tb_stream_m_server_conn(void *data)
{
	tb_session_t *session = (tb_session_t*)data;
	session->status = SESSION_CONNECTED;
	PRT_I_D("Session %d: Started connection", session->id);

	int *retval = malloc(sizeof(int)), rc;

	//Main loop, and time the damn thing.
	tb_start_time(session->transfer_t);
	do
	{
		rc = recv(session->sock_d, session->data, session->pack_size, 0);

		if(rc == -1)
		{
			perror("Error: tb_stream_m_server_conn");

			pthread_mutex_trylock(session->stat_lock);
			close(session->sock_d);
			session->status = SESSION_DISCONNECTED;
			pthread_mutex_unlock(session->stat_lock);

			*retval = -1;
			return retval;
		}

		session->total_bytes += rc;
	}
	while(rc != 0);
	tb_finish_time(session->transfer_t);

	tb_print_times(session);
	fprintf(stdout, "session %d received %lld bytes", session->id,
			session->total_bytes);
	pthread_mutex_trylock(session->stat_lock);
	close(session->sock_d);
	session->status = SESSION_DISCONNECTED;
	pthread_mutex_unlock(session->stat_lock);

	PRT_I_D("Session %d: Ended connection", session->id);
	*retval = 0;
	return retval;
}

////////////////////////// Client Functions //////////////////////////

int
tb_stream_m_client(tb_listener_t *listener)
{
	const int num_conn = 4;
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

		//Add logging info
		session->log_enabled = 1;
		session->log_info = listener->log_info;

		PRT_I_D("Creating thread for session %d", session->id);
		//Send the thread on its merry way.
		pthread_create(session->s_thread, NULL, &tb_stream_connection,
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
*tb_stream_connection(void *data)
{

	int *retval = malloc(sizeof(int));
	tb_session_t *session = (tb_session_t*)data;

	LOG_S_INFO(session, "started thread");

	//Create the connection and time the connection time.
	tb_start_time(session->connect_t);
	if(connect(session->sock_d, session->addr_info->ai_addr,
			session->addr_info->ai_addrlen) == -1)
	{
		perror("Error: tb_stream_connection");
		*retval = -1;

		return retval;
	}
	tb_finish_time(session->connect_t);

	LOG_S_INFO(session, "Connected");

	session->status = SESSION_CONNECTED;

	int sz = session->pack_size, rc, rmbytes = 0;

	//Start the transfer, and begin timing.
	tb_start_time(session->transfer_t);
	while(session->total_bytes < session->data_size)
	{
		rmbytes = session->data_size - session->total_bytes;

		if(rmbytes < session->pack_size)
		{
			sz = rmbytes;
		}

		if((rc = send(session->sock_d, session->data + session->total_bytes,
				sz, 0)) == -1)
		{
			PRT_ERR("Error: tb_stream_connection");
			*retval = -1;
			return retval;
		}

		session->total_bytes += rc;
	}
	tb_finish_time(session->transfer_t);

	LOG_S_INFO(session, "Ended Connection");

	fprintf(stdout, "Session %d: sent %lld bytes", session->id, session->total_bytes);
	session->status = SESSION_COMPLETE;

	tb_print_times(session);

	*retval = 0;
	return retval;
}

int
tb_stream_client(tb_listener_t *listener)
{
	tb_connect(listener);

	listener->status = TB_CONNECTED;

	int sz = listener->bufsize, rc;
	int rmbytes = 0;

	while(listener->total_tx_rx < listener->file_size)
	{
		rmbytes = listener->file_size - listener->total_tx_rx;

		if(rmbytes < listener->bufsize)
		{
			sz = rmbytes;
		}

		rc = tb_send_data(listener,
				listener->data + listener->total_tx_rx, sz);

		listener->total_tx_rx += rc;
	}

	pthread_mutex_trylock(listener->stat_lock);

	listener->protocol->f_close(listener->sock_d);
	listener->sock_d = -1;

	listener->status = TB_DISCONNECTED;

	pthread_mutex_unlock(listener->stat_lock);

	return listener->total_tx_rx;
}

