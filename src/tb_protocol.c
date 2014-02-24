/*
 * tb_protocol.c
 *
 *  Created on: 5/12/2013
 *      Author: michael
 */

#include "tb_protocol.h"
#include "tb_utp.h"
#include "tb_udp.h"
#include "tb_udt.h"
#include "tb_stream.h"
#include "tb_common.h"

#include <sys/socket.h>
#include <stdlib.h>
#include <udt.h>
#include <utp.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <string.h>

const int socket_error = -1;

tb_protocol_t
*tb_create_protocol(PROTOCOL prot)
{
	tb_protocol_t *p = malloc(sizeof(tb_protocol_t));

	//Set the functions for the specified protocol
	switch(prot){
	case TCP :
		p->f_sock = &socket;
		p->f_bind = (funct_bind)&bind;
		p->f_connect = &connect;
		p->f_listen = &listen;
		p->f_recv = (funct_recv)&recv;
		p->f_send = (funct_send)&send;
		p->f_accept = (funct_accept)&accept;
		p->f_close = &close;
		p->type = SOCK_STREAM;
		p->error_no = &socket_error;
		p->eot = 0;
		p->sock_flags = 0;
		p->f_recfrom = NULL;
		p->f_sendto = NULL;
		p->protocol = TCP;
		p->f_error = &tb_socket_error;
		p->f_opt = NULL;
		p->f_exit = NULL;
		p->f_setup = NULL;
		p->prot_opts = USE_BLOCKING;
		p->f_ep_event = NULL;

		break;
	case UDP :
		p->f_sock = (funct_socket)&socket;
		p->f_bind = &bind;
		p->f_connect = (funct_connect)&connect;
		p->f_listen = NULL;
		p->f_send = (funct_send)&send;
		p->f_recv = NULL;
		p->type = SOCK_DGRAM;
		p->f_close = &close;
		p->f_exit = NULL;
		p->error_no = &socket_error;
		p->eot = 0;
		p->sock_flags = 0;
		p->f_recfrom = (funct_recvfrom)&recvfrom;
		p->f_sendto = (funct_sendto)&sendto;
		p->protocol = UDP;
		p->f_error = &tb_socket_error;
		p->f_opt = NULL;
		p->f_setup = NULL;
		p->prot_opts = USE_BLOCKING;
		p->f_ep_event = NULL;

		break;
	case UDT :
		p->f_sock = (funct_socket)&udt_socket;
		p->f_bind = (funct_bind)&udt_bind;
		p->f_connect = (funct_connect)&udt_connect;
		p->f_listen = &udt_listen;
		p->f_recv = (funct_recv)&udt_recv;
		p->f_send = (funct_send)&udt_send;
		p->type = SOCK_STREAM;
		p->f_close = &udt_close;
		p->f_accept = (funct_accept)&udt_accept;
		p->f_exit = (funct_exit)&udt_cleanup;
		p->error_no = &UDT_ERROR;
		p->eot = -1;
		p->sock_flags = 0;
		p->f_recfrom = NULL;
		p->f_sendto = NULL;
		p->protocol = UDT;
		p->f_error = &tb_udt_error;
		p->f_opt = NULL;
		p->f_setup = NULL;
		p->prot_opts = USE_BLOCKING;
		p->f_ep_event = NULL;

		break;
	case uTP :
		p->f_sock = (funct_socket)&tb_utp_socket;
		p->f_bind = (funct_bind)&bind;
		p->f_connect = (funct_connect)&tb_utp_connect;
		p->f_listen = NULL;
		p->f_recv = NULL;
		p->f_send = (funct_send)&tb_utp_send;
		p->type = SOCK_DGRAM;
		p->f_close = NULL;
		p->f_accept = NULL;
		p->f_exit = NULL;
		p->error_no = &socket_error;
		p->eot = -128;
		p->sock_flags = 0;
		p->f_recfrom = NULL;
		p->f_sendto = NULL;
		p->protocol = uTP;
		p->f_error = tb_utp_error_handle;
		p->f_opt = NULL;
		p->f_setup = NULL;
		p->prot_opts = USE_BLOCKING;
		p->f_ep_event = NULL;

		break;
	case eUDP:
		p->f_sock = (funct_socket)&socket;
		p->f_bind = (funct_bind)&bind;
		p->f_connect = (funct_connect)&connect;
		p->f_listen = (funct_listen)&listen;
		p->f_recv = NULL;
		p->f_send = NULL;
		p->type = SOCK_DGRAM;
		p->f_close = &close;
		p->f_accept = NULL;
		p->f_exit = NULL;
		p->error_no = &socket_error;
		p->eot = 0;
		p->sock_flags = 0;
		p->f_recfrom = NULL;
		p->f_sendto = (funct_sendto)&sendto;
		p->protocol = eUDP;
		p->f_error = &tb_socket_error;
		p->f_opt = NULL;
		p->f_setup = NULL;
		p->prot_opts = USE_EPOLL;
		p->f_ep_event = &tb_udp_event;

		break;
	case DCCP:
		p->f_sock = (funct_socket)&socket;
		p->f_bind = (funct_bind)&bind;
		p->f_connect = (funct_connect)&connect;
		p->f_listen = (funct_listen)&listen;
		p->f_recv = (funct_recv)&recv;
		p->f_send = (funct_send)&send;
		p->type = SOCK_DCCP;
		p->f_close = &close;
		p->f_accept = (funct_accept)&accept;
		p->f_exit = NULL;
		p->error_no = &socket_error;
		p->eot = 0;
		p->sock_flags = 0;
		p->f_recfrom = NULL;
		p->f_sendto = NULL;
		p->protocol = DCCP;
		p->f_error = &tb_dccp_error;
		p->f_opt = NULL;
		p->f_setup = NULL;
		p->prot_opts = USE_BLOCKING;
		p->f_ep_event = NULL;

		break;
	default:
		fprintf(stderr, "No such protocol: %d\n", prot);
		exit(-1);
	}

	return p;
}

int
tb_socket_error(int value, int err_no)
{
	perror("Error");
	return -1;
}

int
tb_dccp_error(int value, int err_no)
{
	if(err_no == EAGAIN)
	{
		return 0;
	}

	return -1;
}

int
tb_udt_error(int value, int err_no)
{
	return 0;
}

void
tb_print_protocol(tb_protocol_t *protocol)
{
	fprintf(stdout, "protocol : %d\n", protocol->protocol);
}

void
tb_get_stats(tb_prot_stats_t *stats, int fd)
{
	stats->id++;

	switch(stats->protocol)
	{
	case TCP :
		get_tcp_stats(stats, fd);
		break;

	case UDP :
	case eUDP :
		get_udp_stats(stats, fd);
		break;

	case UDT :
	case aUDT :
		get_udt_stats(stats, fd);
		break;

	case uTP :
		get_utp_stats(stats, fd);
		break;

	case DCCP :
		get_dccp_stats(stats, fd);
		break;
	}

}

tb_prot_stats_t
*tb_create_stats()
{
	tb_prot_stats_t *stats = malloc(sizeof(tb_prot_stats_t));
	memset(stats, 0, sizeof(tb_prot_stats_t));

	return stats;
}

void
tb_destroy_stats(tb_prot_stats_t *stats)
{
	if(stats->prot_data != NULL)
	{
		free(stats->prot_data);
	}

	free(stats);
}

void
get_udt_stats(tb_prot_stats_t *stats, int fd)
{
	struct CPerfMon *perf = malloc(sizeof(struct CPerfMon));

	int rc = udt_perfmon(fd, perf, 0);

	if(rc == UDT_ERROR)
	{
		fprintf(stderr, "Error: get_udt_stats: %s\n", udt_getlasterror_desc());
		return;
	}

	stats->rtt = perf->msRTT;
	stats->recv_rate = perf->mbpsRecvRate;
	stats->send_rate = perf->mbpsSendRate;
	stats->send_window = perf->pktFlowWindow;
	stats->send_p_loss = perf->pktSndLoss;
	stats->recv_p_loss = perf->pktRcvLoss;

	//Free data for stat_data if required.
	if(stats->prot_data != NULL)
	{
		free(stats->prot_data);
	}

	stats->prot_data = (void*)perf;
}

void
get_udp_stats(tb_prot_stats_t *stats, int fd)
{
	tb_get_bsd_stats(stats, fd);
}

void
get_utp_stats(tb_prot_stats_t *stats, int fd)
{
	tb_get_bsd_stats(stats, fd);
}

void
get_dccp_stats(tb_prot_stats_t *stats, int fd)
{
	tb_get_bsd_stats(stats, fd);
}

void
get_tcp_stats(tb_prot_stats_t *stats, int fd)
{
	struct tcp_info *info = malloc(sizeof(struct tcp_info));

	socklen_t len = (socklen_t)sizeof(struct tcp_info);

	int rc = getsockopt(fd, SOL_TCP, TCP_INFO, (void*)info, &len);

	if(rc == -1)
	{
		perror("Error: get_tcp_stats");
		return;
	}
	else
	{
		stats->rtt = ((double)info->tcpi_rtt) / 1000.0;
		stats->rtt_var = ((double)info->tcpi_rttvar) / 1000.0;
		stats->send_window = info->tcpi_snd_cwnd;
		stats->recv_p_loss = info->tcpi_lost;
	}

	free(stats->prot_data);

	stats->prot_data = info;

	tb_get_bsd_stats(stats, fd);
}

void
tb_get_bsd_stats(tb_prot_stats_t *stats, int fd)
{
	socklen_t sz = (socklen_t)sizeof(stats->w_buff_size);

	getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (void*)&stats->w_buff_size, &sz);

	getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void*)&stats->r_buff_size, &sz);
}

void
tb_destroy_protocol(tb_protocol_t *protocol)
{
	if(protocol->f_exit != NULL)
	{
		fprintf(stdout, "f_exit performing\n");
		protocol->f_exit();
	}
	else
	{
		fprintf(stdout, "no f_exit to perform\n");
	}

	free(protocol);
}

