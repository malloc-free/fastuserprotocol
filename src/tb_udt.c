/*
 * tb_udt.h
 *
 *  Created on: 7/02/2014
 *      Author: michael
 */
#include "tb_udt.h"

#include "tb_protocol.h"
#include "tb_listener.h"
#include "tb_testbed.h"
#include "tb_common.h"
#include "tb_udp.h"
#include "tb_utp.h"
#include "udt.h"
#include "tb_testbed.h"
#include "tb_session.h"
#include "tb_stream.h"

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

///////////////////////// Server Functions //////////////////////////

int
tb_udt_server(tb_listener_t *listener)
{
	if(udt_listen(listener->sock_d, 10) == UDT_ERROR)
	{
		fprintf(stderr, "udt_listen: %s", udt_getlasterror_desc());
		tb_abort(listener);
	}

	PRT_INFO("Listening");
	listener->status = TB_LISTENING;

	while(listener->command != TB_ABORT && listener->command != TB_EXIT){
		PRT_INFO("Accepting");
		LOG(listener, "Accepting", LOG_INFO);

		tb_session_t *session = tb_create_server_session();

		session->sock_d = udt_accept(listener->sock_d,
				(struct sockaddr*)session->addr_in,
				(int*)&session->addr_len);

		if(session->sock_d == UDT_ERROR)
		{
			fprintf(stderr, "udt_accept: %s", udt_getlasterror_desc());
			tb_abort(listener);
		}

		//LOG_ADD(listener, "Connected address:", session->o_addr_info);
		session->data = malloc(listener->bufsize);
		session->data_size = listener->bufsize;
		listener->curr_session = session;
		listener->status = TB_CONNECTED;

		//Critical loop.
		while(!(listener->command & TB_E_LOOP))
		{
			session->last_trans =
					udt_recv(session->sock_d, session->data,
							session->data_size, 0);

			if(session->last_trans == UDT_ERROR)
			{
				int rc = udt_getlasterror_code();
				if(rc == 2001)
				{
					break;
				}
				else
				{
					fprintf(stderr, "udt_recv: %stb", udt_getlasterror_desc());
					tb_abort(listener);
				}
			}

			session->total_bytes += session->last_trans;
			listener->total_tx_rx += session->last_trans;
		}


		//Lock stat collection while closing connection and destroying session.
		pthread_mutex_trylock(listener->stat_lock);

		fprintf(stdout, "Session closed\n");
		listener->status = TB_DISCONNECTED;

		PRT_INFO("Destroying session");
		listener->curr_session = NULL;
		udt_close(session->sock_d);
		tb_destroy_session(session);
		PRT_INFO("Session destroyed");
		udt_close(listener->sock_d);

		pthread_mutex_unlock(listener->stat_lock);

		//Exit if prompted to.
		listener->command = listener->s_tx_end;
	}

	listener->protocol->f_close(listener->sock_d);
	listener->sock_d = -1;

	return listener->total_tx_rx;
}

int
tb_udt_m_server(tb_listener_t *listener)
{
	PRT_INFO("Starting udt_m_server");

	udt_listen(listener->sock_d, 10);
	listener->status = TB_LISTENING;

	int syn = 0;
	if(udt_setsockopt(listener->sock_d, SOL_SOCKET, UDT_RCVSYN, &syn, sizeof(syn))
			< 0)
	{
		fprintf(stderr, "Error: tb_udt_m_server: udt_setsockopt: %s",
				udt_getlasterror_desc());
		tb_abort(listener);
	}

	int epoll = udt_epoll_create();
	int events = EPOLLIN | EPOLLET;
	if(udt_epoll_add_usock(epoll, listener->sock_d, &events) < 0)
	{
		fprintf(stderr, "Error: tb_udt_m_server: udt_epoll_add_usock: %s",
				udt_getlasterror_desc());
		tb_abort(listener);
	}

	listener->session_list->userdata = (void*)&listener->command;

	int epoll_ready = 0;
	int r_num = 0;

	//Poll for events on the socket.

	while(!(listener->command & TB_E_LOOP))
	{
		r_num = udt_epolls_wait2(epoll, &epoll_ready, &r_num, NULL, 0, 50, NULL, 0, NULL, 0);

		if(r_num > 0)
		{
			PRT_INFO("Event");
			tb_udt_event(listener);
		}
	}

	LOG_INFO(listener, "exit main loop");
	pthread_mutex_trylock(listener->stat_lock);

	udt_close(listener->sock_d);
	listener->status = TB_DISCONNECTED;

	pthread_mutex_unlock(listener->stat_lock);

	return listener->total_tx_rx;
}

int
tb_udt_event(tb_listener_t *listener)
{
	while(1)
	{
		tb_session_t *session = tb_create_session();

		session->sock_d = udt_accept(listener->sock_d, (struct sockaddr*)session->addr_in,
				(int*)&session->addr_len);

		if(session->sock_d == UDT_ERROR)
		{
			fprintf(stderr, "Error: tb_udt_event: %s", udt_getlasterror_desc());
			tb_destroy_session(session);

			return 0;
		}

		session->stats->protocol = listener->protocol->protocol;
		session->pack_size = listener->bufsize;
		session->data = malloc(listener->bufsize);

		//Set socket to blocking - we block on each individual connection.
		int syn = 1;
		if(udt_setsockopt(session->sock_d, SOL_SOCKET, UDT_RCVSYN, &syn, sizeof(syn))
					< 0)
		{
			fprintf(stderr, "Error: tb_udt_m_server: udt_setsockopt: %s",
					udt_getlasterror_desc());
			tb_abort(listener);
		}

		//Add the session to the linked list, and add the list to otherdata.
		tb_session_add(listener->session_list, session);
		tb_session_list_inc(listener->session_list);
		session->other_info = listener->session_list;

		PRT_I_D("Starting session: %d", session->id);
		pthread_create(session->s_thread, NULL, &tb_udt_m_server_conn,
				(void*)session);

		listener->status = TB_CONNECTED;
	}

	return 0;
}

void
*tb_udt_m_server_conn(void *data)
{
	tb_session_t *session = (tb_session_t*)data;
	session->status = SESSION_CONNECTED;
	PRT_I_D("Session %d: Started connection", session->id);

	int *retval = malloc(sizeof(int)), rc;

	tb_start_time(session->transfer_t);
	do
	{
		rc = udt_recv(session->sock_d, session->data, session->pack_size, 0);

		if(rc == UDT_ERROR)
		{
			if(udt_getlasterror_code() == 2001)
			{
				break;
			}
			else
			{
				fprintf(stderr, "Session %d: Error: tb_udt_m_connection:"
						" udt_recv: %s", session->id, udt_getlasterror_desc());

				pthread_mutex_trylock(session->stat_lock);
				udt_close(session->sock_d);
				session->status = SESSION_DISCONNECTED;
				pthread_mutex_unlock(session->stat_lock);

				*retval = -1;
				return retval;
			}
		}

		session->total_bytes += rc;
	}
	while(rc != 0);

	pthread_mutex_trylock(session->stat_lock);
	udt_close(session->sock_d);
	session->status = SESSION_DISCONNECTED;

	//Decrement number of active conn, if none left, command main loop to bail.
	tb_session_list_t *list = (tb_session_list_t*)session->other_info;
	if(!tb_session_list_dec(list))
	{
		LOG_S_INFO(session, "Last session, exiting");
		*(int*)list->userdata = TB_EXIT;
	}

	fprintf(stdout, "Session %d received %lld\n", session->id, session->total_bytes);
	pthread_mutex_unlock(session->stat_lock);

	PRT_I_D("Session %d: Ended connection", session->id);
	return retval;
}

///////////////////////// Client Functions //////////////////////////

int
tb_udt_client(tb_listener_t *listener)
{
	tb_connect(listener);
	PRT_INFO("Connected");
	listener->status = TB_CONNECTED;

	int sz = listener->bufsize, rc, rmbytes = 0;

	while(listener->total_tx_rx < listener->file_size &&
			listener->command != TB_ABORT && listener->command != TB_EXIT)
	{
		rmbytes = listener->file_size - listener->total_tx_rx;

		if(rmbytes < listener->bufsize)
		{
			sz = rmbytes;
		}

		rc = udt_send(listener->sock_d, listener->data + listener->total_tx_rx,
				sz, 0);

		if(rc == UDT_ERROR)
		{
			fprintf(stderr, "udt_send: %s", udt_getlasterror_desc());
			tb_abort(listener);
		}

		listener->total_tx_rx += rc;
	}

	//Lock up stat collection and close the socket.
	pthread_mutex_trylock(listener->stat_lock);

	LOG(listener, "Closing Connection", LOG_INFO);
	udt_close(listener->sock_d);
	listener->sock_d = -1;
	listener->status = TB_DISCONNECTED;

	pthread_mutex_unlock(listener->stat_lock);

	return listener->total_tx_rx;
}

int
tb_udt_m_client(tb_listener_t *listener)
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

		session->sock_d = udt_socket(session->addr_info->ai_family,
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

		tb_session_add(listener->session_list, session);

		PRT_I_D("Creating thread for session %d", session->id);
		//Send the thread on its merry way.
		pthread_create(session->s_thread, NULL, &tb_udt_m_connection,
				(void*)session);
	}

	//Wait for connections, then declare connection status.
	while(listener->session_list->start->status == SESSION_CREATED);
	listener->status = TB_CONNECTED;

	int *retval;

	//Close and clear all sessions.
	tb_session_t *curr_session = listener->session_list->start;
	while(curr_session != NULL)
	{
		PRT_I_D("Joining with session %d",curr_session->id);
		pthread_join(*curr_session->s_thread, (void**)&retval);
		pthread_mutex_lock(curr_session->stat_lock);
		udt_close(curr_session->sock_d);
		pthread_mutex_unlock(curr_session->stat_lock);

		curr_session = curr_session->n_session;
	}

	listener->status = TB_DISCONNECTED;

	return listener->total_tx_rx;
}

void
*tb_udt_m_connection(void *data)
{
	int *retval = malloc(sizeof(int));
	tb_session_t *session = (tb_session_t*)data;

	PRT_I_D("Session %d: started connection", session->id);

	//Create the connection and time the connection time.
	tb_start_time(session->connect_t);
	if(udt_connect(session->sock_d, session->addr_info->ai_addr,
			session->addr_info->ai_addrlen) == -1)
	{
		fprintf(stderr, "Session %d: tb_udt_m_connection: %s", session->id,
				udt_getlasterror_desc());

		*retval = -1;

		return retval;
	}
	tb_finish_time(session->connect_t);

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

		if((rc = udt_send(session->sock_d, session->data + session->total_bytes,
				sz, 0)) == UDT_ERROR)
		{
			fprintf(stderr, "Session %d: tb_udt_m_connection: udt_send: %s",
					session->id, udt_getlasterror_desc());

			*retval = -1;
			return retval;
		}


		session->total_bytes += rc;
	}
	tb_finish_time(session->transfer_t);

	PRT_I_D("Session %d: ended connection", session->id);
	session->status = SESSION_COMPLETE;

	tb_print_times(session);

	*retval = 0;

	return retval;
}

