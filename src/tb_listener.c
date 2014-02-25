/*
 * tb_listener.c
 *
 *  Created on: 4/12/2013
 *      Author: michael
 */


#include "tb_listener.h"
#include "tb_protocol.h"
#include "tb_common.h"
#include "tb_logging.h"
#include "tb_sock_opt.h"
#include "tb_session.h"

#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <udt.h>
#include <assert.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>

tb_listener_t
*tb_create_listener(ENDPOINT_TYPE type, char *addr, char *port, PROTOCOL protocol,
		int bufsize)
{
	tb_listener_t *listener = malloc(sizeof(tb_listener_t));

	//Listener thread.
	listener->__l_thread = malloc(sizeof(pthread_t));

	//Setup network parameters.
	listener->bind_port = strdup(port);
	listener->bind_address = strdup(addr);
	listener->e_type = type;
	listener->protocol = tb_create_protocol(protocol);
	listener->sock_d = -1;
	listener->options = NULL;
	listener->curr_session = NULL;

	//Set the file to upload to NULL
	listener->fp = NULL;
	listener->bufsize = bufsize;

	listener->total_tx_rx = 0;
	listener->file_size = 0;
	listener->filename = NULL;
	listener->data = NULL;
	listener->num_send = 1;

	//Set the listener to NOT be destroyed on exit.
	//This is set to allow for external destruction
	//of the listener. Can be changed.
	listener->d_exit = 1;

	//Threading stuff

	//Lock and condition for stat collection
	listener->stat_lock = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(listener->stat_lock, NULL);

	listener->stat_cond = malloc(sizeof(pthread_cond_t));
	pthread_cond_init(listener->stat_cond, NULL);

	listener->status_lock = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(listener->status_lock, NULL);

	listener->__l_thread = malloc(sizeof(pthread_t));

	//Initialize stats.
	listener->stats = malloc(sizeof(tb_prot_stats_t));
	memset(listener->stats, 0, sizeof(tb_prot_stats_t));
	listener->stats->n_stats = NULL;
	listener->stats->protocol = protocol;
	listener->stats->stat_time = tb_create_time(CLOCK_MONOTONIC);

	tb_other_info *info = malloc(sizeof(tb_other_info));
	info->sys_tid = -1;
	info->l_status = TB_CREATED;

	listener->stats->other_info = (void*)info;
	listener->stats->id = -1;
	listener->read = 0;

	listener->epoll = NULL;

	//Set defaults for monitor and print
	listener->monitor = 1;
	listener->print_stats = 1;

	//Set CPU and thread info.
	listener->num_proc = get_nprocs();
	listener->cpu_affinity = 0;
	listener->main_cpu_aff = 0;
	listener->sys_tid = -1;

	//Set defaults for command and exit command.
	listener->command = TB_CONTINUE;
	listener->s_tx_end = TB_EXIT;

	//Set status of listener.
	listener->status = TB_CREATED;

#ifndef _TB_LIBRARY
	//Set logging info
	listener->log_enabled = 1;
	//Save file name depends on client or server
	if(listener->e_type == SERVER)
	{
		listener->log_info = tb_create_flog("log_server", DATE);
	}
	else
	{
		listener->log_info = tb_create_flog("log_client", DATE);
	}
#else
	listener->log_info = NULL;
	listener->log_enabled = 0;
#endif

	//Set protocol options
	listener->options = malloc(sizeof(tb_options_t));
	listener->options->protocol = protocol;
	listener->options->l3_r_b_size = 0;
	listener->options->l3_s_b_size = 0;
	listener->options->l4_s_b_size = 0;
	listener->options->l4_r_b_size = 0;

	//Setup multiple connections
	listener->session_list = malloc(sizeof(tb_session_list_t));
	listener->session_list->current_max_id = -1;
	listener->session_list->start = NULL;
	listener->session_list->end = NULL;
	listener->session_list->nac_lock = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(listener->session_list->nac_lock, NULL);
	listener->session_list->num_active_conn = malloc(sizeof(int));
	*listener->session_list->num_active_conn = 0;
	listener->session_list->num_sessions = 0;

	//Setup timing
	listener->transfer_time = tb_create_time(CLOCK_MONOTONIC);
	listener->connect_time = tb_create_time(CLOCK_MONOTONIC);

	return listener;
}

tb_listener_t
*tb_create_endpoint(tb_test_params_t *params)
{
	assert(params->address != NULL);
	assert(params->port != NULL);
	assert(params->type <= 3 && params->type >= 0);
	assert(params->protocol >= TCP && params->protocol <= DCCP);

	if(params->file_name == NULL)
	{
		assert(params->d_size > 0);
	}
	else
	{
		assert(params->d_size <= 0);
	}

	tb_listener_t *listener = tb_create_listener(
			params->type, params->address, params->port,
			params->protocol, params->pack_size);

	//File size is in KB.
	listener->file_size = params->d_size * 1024;
	listener->filename = params->file_name;

	if(params->pack_size == 0)
	{
		PRT_INFO("Pack size is 0, setting default (1408)");
		LOG(listener, "Setting default pack size: 1408", LOG_INFO);
		listener->bufsize = 1408;
	}
	else
	{
		PRT_I_D("Using pack size %d", params->pack_size);
		listener->bufsize = params->pack_size;
	}

	//Transfer protocol options
	listener->options->l3_r_b_size = params->l3_r_b_size;
	listener->options->l3_s_b_size = params->l3_w_b_size;
	listener->options->l4_s_b_size = params->l4_w_b_size;
	listener->options->l4_r_b_size = params->l4_r_b_size;
	listener->options->control = params->control;

	listener->print_stats = params->print_stats;

	//Setup and create log.
	if(params->log_enable)
	{
		PRT_I_S("Log enabled, creating log", params->log_path);

		listener->log_info = tb_create_flog(params->log_path, DATE);

		if(listener->log_info == NULL)
		{
			PRT_ERR("Could not create log info");
		}
		else
		{
			PRT_I_S("Log created", listener->log_info->file_path);
			listener->log_enabled = 1;
		}
	}

	listener->d_exit = 0;

	return listener;
}


void
tb_get_cpu_info(tb_listener_t *listener)
{
	listener->sys_tid = syscall(SYS_gettid);
}

void
tb_destroy_listener(tb_listener_t *listener)
{
	free(listener->__l_thread);
	free(listener->bind_address);
	free(listener->bind_port);

	pthread_cond_destroy(listener->stat_cond);
	pthread_mutex_destroy(listener->stat_lock);
	pthread_mutex_destroy(listener->status_lock);

	//Do NOT want to forget to do this :-)
	if(listener->data != NULL)
	{
		PRT_INFO("Destroying data");
		free(listener->data);
	}
	else
	{
		PRT_INFO("No data to destroy");
	}

	if(listener->epoll != NULL)
	{
		PRT_INFO("Destroying epoll");
		tb_destroy_epoll(listener->epoll);
	}
	else
	{
		PRT_INFO("No epoll to destroy");
	}

	if(listener->log_info != NULL)
	{
		PRT_INFO("Destroying log");
		tb_destroy_log(listener->log_info);
	}
	else
	{
		PRT_INFO("No log to destroy");
	}

	free(listener->session_list);

	tb_destroy_time(listener->stats->stat_time);

	free(listener->stats);
	free(listener->options);

	tb_destroy_protocol(listener->protocol);

	PRT_INFO("protocol destroyed");

	//Destroy timers.
	tb_destroy_time(listener->connect_time);
	tb_destroy_time(listener->transfer_time);

	free(listener);
}

void
tb_print_listener(tb_listener_t *listener)
{
	PRT_ACK(LINE "Listener:" LINE);

	fprintf(stdout, "port = %s\n", listener->bind_port);
	fprintf(stdout, "address = %s\n", listener->bind_address);
	fprintf(stdout, "d_size = %d\n", listener->file_size);
	fprintf(stdout, "buffsize = %d\n", listener->bufsize);

	if(listener->filename != NULL)
	{
		fprintf(stdout, "filename = %s\n", listener->filename);
	}

	char *type;

	switch(listener->e_type)
	{
	case SERVER:
		type = "Server";
		break;

	case CLIENT:
		type = "Client";
		break;

	case mSERVER:
		type = "mServer";
		break;

	case mCLIENT:
		type = "mClient";
		break;

	}

	fprintf(stdout, "e_type = %s\n", type);
	tb_print_protocol(listener->protocol);

	PRT_ACK(LINE);
}

tb_prot_stats_t
*tb_ex_get_stats(tb_listener_t *listener)
{
	pthread_mutex_trylock(listener->stat_lock);

	if(listener->read == 1 && (listener->status != TB_ABORTING &&
			listener->status != TB_EXITING))
	{
		pthread_cond_wait(listener->stat_cond, listener->stat_lock);
	}

	//Start recording time if not started, otherwise get time of collection.
	if(!listener->stats->stat_time->started)
	{
		tb_start_time(listener->stats->stat_time);
	}
	else
	{
		tb_finish_time(listener->stats->stat_time);
	}

	listener->read = 1;
	pthread_mutex_unlock(listener->stat_lock);

	return listener->stats;
}

void
tb_ex_get_stat_cpy(tb_listener_t *listener, tb_prot_stats_t *stats)
{
	pthread_mutex_trylock(listener->stat_lock);

	if(listener->read == 1 && (listener->status != TB_ABORTING &&
			listener->status != TB_EXITING))
	{
		pthread_cond_wait(listener->stat_cond, listener->stat_lock);
	}

	//Start recording time if not started, otherwise get time of collection.
	if(!listener->stats->stat_time->started)
	{
		tb_start_time(listener->stats->stat_time);
	}
	else
	{
		tb_finish_time(listener->stats->stat_time);
	}

	listener->stats->id++;
	listener->read = 1;
	memcpy(stats, listener->stats, sizeof(tb_prot_stats_t));

	pthread_mutex_unlock(listener->stat_lock);

}

void
tb_set_l_stats(tb_listener_t *listener)
{
	//Thread safe set the current status.
	pthread_mutex_trylock(listener->stat_lock);

	//Get the current read for the listener.
	listener->stats->current_read = listener->total_tx_rx;

	if(listener->status == TB_CONNECTED)
	{
		if(listener->e_type == SERVER)
		{
			tb_session_t *session = listener->curr_session;
			tb_get_stats(listener->stats, session->sock_d);
		}
		else
		{
			tb_get_stats(listener->stats,
					listener->sock_d);
		}
	}

	tb_other_info *info = (tb_other_info*)listener->stats->other_info;
	info->l_status = listener->status;
	info->sys_tid = listener->sys_tid;

	listener->read = 0;

	pthread_cond_signal(listener->stat_cond);
	pthread_mutex_unlock(listener->stat_lock);

}

void
tb_set_m_stats(tb_listener_t *listener)
{
	pthread_mutex_trylock(listener->stat_lock);
	listener->stats->current_read = 0;
	listener->total_tx_rx = 0;

	tb_session_t *session = listener->session_list->start;
	int num_sessions = 0;

	//Clear out the stats.
	tb_prot_stats_t *stats = listener->stats;
	stats->rtt = 0.0;
	stats->rtt_var = 0.0;
	stats->send_rate = 0.0;
	stats->recv_rate = 0.0;
	stats->send_window = 0.0;
	stats->recv_window = 0.0;
	stats->send_p_loss = 0;
	stats->recv_p_loss = 0;

	//Iterate through each of the sessions, get stats for each,
	//add them to the listener stats, and average them.
	while(session)
	{
		pthread_mutex_trylock(session->stat_lock);

		//Add to the total bytes read for the listener.
		listener->stats->current_read += session->total_bytes;
		listener->total_tx_rx += session->total_bytes;

		//Collect stats for the session if connected.
		if(session->status == SESSION_CONNECTED)
		{
			++num_sessions;
			tb_get_stats(session->stats, session->sock_d);

			//Add to stats to find average.
			stats->rtt += session->stats->rtt;
			stats->rtt_var += session->stats->rtt_var;
			stats->send_rate += session->stats->send_rate;
			stats->recv_rate += session->stats->recv_rate;
			stats->send_window += session->stats->send_window;
			stats->recv_window += session->stats->recv_window;
			stats->send_p_loss += session->stats->send_p_loss;
			stats->recv_p_loss += session->stats->recv_p_loss;
		}

		pthread_mutex_unlock(session->stat_lock);

		session = session->n_session;
	}

	//If we have any sessions, find the average.
	if(num_sessions)
	{
		stats->rtt /= num_sessions;
		stats->rtt_var /= num_sessions;
		stats->send_rate /= num_sessions;
		stats->recv_rate /= num_sessions;
		stats->send_window /= num_sessions;
		stats->recv_window /= num_sessions;
		stats->send_p_loss /= num_sessions;
		stats->recv_p_loss /= num_sessions;
	}

	tb_other_info *info = (tb_other_info*)listener->stats->other_info;
	info->l_status = listener->status;
	info->sys_tid = listener->sys_tid;

	listener->read = 0;

	pthread_cond_signal(listener->stat_cond);
	pthread_mutex_unlock(listener->stat_lock);
}

void
tb_get_time_stats(tb_listener_t *listener)
{
	tb_session_t *session = listener->session_list->start;

	//Iterate through the sessions and collect data.
	while(session)
	{
		session->stats->connect_time = session->connect_t->n_sec;
		session->stats->transfer_time = session->transfer_t->n_sec;

		session = session->n_session;
	}

	//Get the listener data.
	listener->stats->connect_time = listener->connect_time->n_sec;
	listener->stats->transfer_time = listener->transfer_time->n_sec;
}

int
tb_listen(tb_listener_t *listener)
{
	int rc = 0;

	tb_protocol_t *p = listener->protocol;

	rc = p->f_listen(listener->sock_d, 10);

	if(rc == -1)
	{
		tb_error(listener, "Error: listen : listen", errno);
		listener->f_abort(listener);
	}

	return rc;
}

tb_session_t
*tb_accept(tb_listener_t *listener)
{

	tb_protocol_t *p = listener->protocol;

	tb_session_t *session = tb_create_server_session();

	session->sock_d = p->f_accept(listener->sock_d,
			(struct sockaddr*)session->addr_in,
			&session->addr_len);

	if(session->sock_d == -1)
	{
		tb_error(listener, "Error: accept", errno);
		return session;
	}

	return session;
}

tb_session_t
*tb_epoll_accept(tb_listener_t *listener)
{

	tb_session_t *session = tb_create_server_session();

	return session;
}

int
tb_resolve_address(tb_listener_t *listener)
{
	struct addrinfo hints;
	int rc;

	tb_protocol_t *prot = listener->protocol;

	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_INET;
	hints.ai_socktype = prot->type;
	//hints.ai_flags = listener->protocol->sock_flags;

	rc = getaddrinfo(listener->bind_address,
			listener->bind_port, &hints, &listener->addr_info);

	if(rc != 0)
	{
		fprintf(stdout, "Error: resolve : getaddrinfo %s\n", gai_strerror(rc));
		listener->f_abort(listener);
	}

	fprintf(stdout, BLUE "ai_socktype %d\n" RESET, listener->addr_info->ai_socktype);
	fprintf(stdout, BLUE "ai_protocol %d\n" RESET, listener->addr_info->ai_protocol);
	fprintf(stdout, BLUE "ai_family %d\n" RESET, listener->addr_info->ai_family);

	return 0;
}

int
tb_create_socket(tb_listener_t *listener)
{
	PRT_ACK("Create socket");
	listener->sock_d =
			listener->protocol->f_sock(listener->addr_info->ai_family,
			listener->addr_info->ai_socktype,
			listener->addr_info->ai_protocol);

	if(listener->sock_d == -1)
	{
		tb_error(listener, "Error: socket", errno);
		listener->f_abort(listener);
	}

	return 0;
}

int
tb_connect(tb_listener_t *listener)
{
	PRT_ACK("Connecting");
	int rc;

	rc = listener->protocol->f_connect(listener->sock_d,
					listener->addr_info->ai_addr,
					listener->addr_info->ai_addrlen);

	if(rc == -1)
	{
		tb_error(listener, "Error: tb_connect", errno);
		listener->f_abort(listener);
	}

	return 0;
}

int
tb_bind(tb_listener_t *listener)
{
	int rc;
	rc =
		listener->protocol->f_bind(listener->sock_d,
		listener->addr_info->ai_addr,
		listener->addr_info->ai_addrlen);

	if(rc == -1)
	{
		tb_error(listener, "Error: bind", errno);
		listener->f_abort(listener);
	}

	return 0;
}

int
tb_send_data(tb_listener_t *listener, char *buff, int size)
{
	int sent_bytes;
	tb_protocol_t *p = listener->protocol;

	sent_bytes =
			p->f_send(listener->sock_d, buff, size, 0);

	if(sent_bytes == -1)
	{
		int err_no = errno;

		if(listener->protocol->f_error(sent_bytes, err_no) != 0)
		{
			listener->f_abort(listener);
		}

		sent_bytes = 0;
	}

	return sent_bytes;
}

int
tb_recv_data(tb_listener_t *listener, tb_session_t *session)
{

	session->last_trans =
			listener->protocol->f_recv(session->sock_d, session->data, session->data_size, 0);

	if(session->last_trans == -1)
	{
		int err_no = errno;

		if(listener->protocol->f_error(session->last_trans, err_no) != 0)
		{
			listener->f_abort(listener);
		}
	}

	return 0;
}

int
tb_send_to(tb_listener_t *listener, tb_session_t *session)
{
	int rc;

	rc = listener->protocol->f_sendto(listener->sock_d, session->data,
			session->data_size, 0, (struct sockaddr*)session->addr_in, session->addr_len);

	if(rc == -1)
	{
		tb_error(listener, "Error: send_to", errno);
		listener->f_abort(listener);
	}

	return 0;
}

int
tb_recv_from(tb_listener_t *listener, tb_session_t *session)
{

	session->last_trans = listener->protocol->f_recfrom(listener->sock_d, session->data,
			session->data_size, 0, (struct sockaddr*)session->addr_in, &session->addr_len);

	if(session->last_trans == -1)
	{
		tb_error(listener, "Error: recv_from", errno);
		listener->f_abort(listener);
	}

	return 0;
}

int
tb_set_epoll(tb_listener_t *listener)
{
	listener->epoll = tb_create_e_poll(listener->sock_d, 64, 0, listener);

	if(listener->epoll == NULL)
	{
		PRT_ERR("Error: tb_set_epoll: Cannot create epoll");
		listener->f_abort(listener);
	}

	listener->epoll->f_event = listener->protocol->f_ep_event;
	listener->epoll->w_time = -1;

	return 0;
}

void
tb_error(tb_listener_t *listener, const char *info, int err_no)
{
	char log_str[50];

	snprintf(log_str, sizeof(log_str), "%s%s", info, strerror(err_no));

	if(listener->log_enabled)
	{
		tb_write_log(listener->log_info, log_str, LOG_ERR);
	}

	fprintf(stderr, log_str);
	fprintf(stderr, "\n");
}

void
tb_address(tb_listener_t *listener, const char* info, struct sockaddr_storage *store)
{
	char log_str[50];
	char *addr = tb_get_address(store);
	snprintf(log_str, sizeof(log_str), "%s : %s", info, addr);
	LOG(listener, addr, LOG_INFO);
	free(addr);
}
