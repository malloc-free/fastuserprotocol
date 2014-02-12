/*
 * tb_session.c
 *
 *  Created on: 25/11/2013
 *      Author: michael
 */

#include "tb_session.h"
#include "tb_epoll.h"
#include "tb_common.h"
#include "tb_protocol.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <glib-2.0/glib.h>
#include <netdb.h>
#include <pthread.h>
#include <time.h>

///////////////// Session List Functions ///////////////////
void
tb_session_add(tb_session_list_t *list, tb_session_t *session)
{
	session->id = ++list->current_max_id;
	if(list->start == NULL)
	{
		list->start = session;
		list->end = session;
	}
	else
	{
		list->start->n_session = session;
		list->end = session;
	}
}
///////////////// Session Functions ///////////////////

////////////////// Creation, Destruction //////////////////

tb_session_t
*tb_create_server_session(){
	tb_session_t *data = (tb_session_t*)malloc(sizeof(tb_session_t));
	data->addr_in = malloc(sizeof(struct sockaddr_storage));
	data->addr_len = (socklen_t)sizeof(struct sockaddr_storage);
	data->status = SESSION_CREATED;
	data->s_thread = NULL;
	data->addr_hints = NULL;
	data->transfer_t = NULL;
	data->connect_t = NULL;
	data->stats = NULL;

	return data;
}

tb_session_t
*tb_create_session()
{
	tb_session_t *session = (tb_session_t*)malloc(sizeof(tb_session_t));
	session->addr_in = malloc(sizeof(struct sockaddr_storage));
	session->status = SESSION_CREATED;
	session->addr_info = NULL;
	session->addr_hints = NULL;
	session->addr_len = (socklen_t)sizeof(struct sockaddr_storage);

	//Set thread.
	session->s_thread = malloc(sizeof(pthread_t));

	//Set current status.
	session->status = SESSION_CREATED;

	//Set stats struct.
	session->stats = malloc(sizeof(tb_prot_stats_t));
	session->stats->prot_data = NULL;

	//Create stats lock.
	session->stat_lock = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(session->stat_lock, NULL);

	//Create time structures for transfer and connect.
	session->transfer_t = tb_create_time(CLOCK_MONOTONIC);
	session->connect_t = tb_create_time(CLOCK_MONOTONIC);

	session->file_name = NULL;

	return session;
}

tb_session_t
*tb_create_session_full(char *addr, char *port, int sock_type,
		unsigned int data_size)
{
	tb_session_t *session = tb_create_session();

	session->address = strdup(addr);
	session->port = strdup(port);

	if(tb_set_addrinfo(session, sock_type) == -1)
	{
		tb_destroy_session(session);
		return NULL;
	}

	if(data_size > 0)
	{
		session->data = malloc(data_size);
		session->data_size = data_size;
		session->int_data = 1;
	}

	return session;
}

void
tb_destroy_session(tb_session_t *session)
{
	assert(session != NULL);

	if(session->file_name != NULL)
	{
		PRT_INFO("Destroying file_name");
		//free(session->file_name);
	}

	if(session->data != NULL  && session->int_data)
	{
		PRT_INFO("Destroying Data");
		//free(session->data);
	}

	free(session->addr_in);

	if(session->addr_info != NULL)
	{
		PRT_INFO("Destroying addr_info");
		//freeaddrinfo(session->addr_info);
	}

	if(session->addr_hints != NULL)
	{
		PRT_INFO("Destroying hints");
		//free(session->addr_hints);
	}

	if(session->stats != NULL)
	{
		PRT_INFO("Destroying stats");
		//tb_destroy_stats(session->stats);
	}

	if(session->s_thread != NULL)
	{
		PRT_INFO("Destroying thread");
		free(session->s_thread);
	}

	if(session->transfer_t != NULL)
	{
		PRT_INFO("Destroying transfer time");
		tb_destroy_time(session->transfer_t);
	}

	if(session->connect_t != NULL)
	{
		PRT_INFO("Destroying connecti time");
		tb_destroy_time(session->connect_t);
	}

	free(session);
}

/////////////////// Network Stuff ///////////////////////////

int
tb_set_addrinfo(tb_session_t *session, int type)
{
	if(session->addr_info == NULL && type > 0)
	{
		session->addr_hints = malloc(sizeof(struct addrinfo));
		memset(session->addr_hints, 0, sizeof(struct addrinfo));
		session->addr_hints->ai_family = AF_INET;
		session->addr_hints->ai_socktype = type;
	}
	else
	{
		PRT_ERR("Error: tb_set_addrinfo: addr_hints not set and type "
				"is not set");
		return -1;
	}

	int rc = getaddrinfo(session->address, session->port,
			session->addr_hints, &session->addr_info);

	if(rc != 0)
	{
		PRT_E_S("Error: tb_set_addrinfo: \n", gai_strerror(rc));
		return -1;
	}

	session->status = SESSION_ADDR_RES;

	return 0;
}

///////////////////// Misc /////////////////////////////////

void
tb_print_times(tb_session_t *session)
{
	fprintf(stdout, "Session %d: Connect time = %lld nano seconds\n",
			session->id,
			session->connect_t->n_sec);

	fprintf(stdout, "Session %d: Connect time = %f seconds\n",
			session->id,
			session->connect_t->n_sec / (double)1000000000);

	fprintf(stdout, "Session %d: Transfer time = %lld nano seconds\n",
			session->id,
			session->transfer_t->n_sec);

	fprintf(stdout, "Session %d: Transfer time = %f seconds\n",
			session->id,
			session->transfer_t->n_sec / (double)1000000000);
}

///////////////// Data Structure Functions /////////////////////

/*
 * Used by gdsl_queue_t to allocate memory for the given element.
 */
gdsl_element_t
allocate_session(void *data){
	tb_session_t *old = (tb_session_t*)data;
	tb_session_t *wd = (tb_session_t*)malloc(sizeof(tb_session_t));
	assert(wd != NULL);
	wd->file_name = strdup(old->file_name);

	return wd;
}

/*
 * Used by gdsl_queue_t to free memory for the given element.
 */
void
free_session(gdsl_element_t data){
	free(((tb_session_t*)data)->file_name);
	free(((tb_session_t*)data)->addr_in);
	free(data);
}

/*
 * Get the key value for this tb_session_t. Used by gdsl_hash_table
 */
char
*get_key_value(void *data){

	struct sockaddr_storage *str = ((tb_session_t*)data)->addr_in;

	char *addr;

	if(str->ss_family == AF_INET)
	{
		addr = malloc(INET_ADDRSTRLEN);
		inet_ntop(AF_INET, &((struct sockaddr_in*)str)->sin_addr, addr,
				INET_ADDRSTRLEN);
	}
	else
	{
		addr = malloc(INET6_ADDRSTRLEN);
		inet_ntop(AF_INET6, &((struct sockaddr_in6*)str)->sin6_addr, addr,
				INET6_ADDRSTRLEN);
	}

	return addr;
}

gboolean
tb_session_equals(gconstpointer a, gconstpointer b)
{
	return ((&a) == (&b)) ? 1 : 0;
}
