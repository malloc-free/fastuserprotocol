/*
 * tb_sock_opt.c
 *
 *  Created on: 18/01/2014
 *      Author: michael
 */

#include "tb_sock_opt.h"
#include "tb_protocol.h"
#include "tb_common.h"
#include "tb_utp.h"

#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>

/**UDT l4 default buffer sizes.
 * Based on the defaults set in the library.
 */
const int UDT_SND_BUFF = 1024 * 1024 * 10;
const int UDT_RCV_BUFF = 1024 * 1024 * 10;

//UDP l4 default buffer sizes.
const int UDP_SND_BUFF = 1024 * 256;
const int UDP_RCV_BUFF = 1024 * 1024;

//TCP l4 default buffer sizes.
const int TCP_SND_BUFF = 1024 * 256;
const int TCP_RCV_BUFF = 1024 * 1024;

/**uDP default buffer sizes.
 * Based on the sizes given in the example code.
 */
const int uTP_SND_BUFF = 300 * 200;
const int uTP_RCV_BUFF = 300 * 200;

//DCCP l3 default buffer sizes
const int DCCP_SND_BUFF = 1024 * 256;
const int DCCP_RCV_BUFF = 1024 * 1024;

int
tb_set_sock_opt(tb_options_t *options, int fd)
{
	int rc;

	switch(options->protocol)
	{
	case TCP:
		rc = tb_set_tcp_opts(options, fd);
		break;

	case UDP:
		rc = tb_set_udp_opts(options, fd);
		break;

	case UDT:
		rc = tb_set_udt_opts(options, fd);
		break;

	case aUDT:
		rc = tb_set_audt_opts(options, fd);
		break;

	case uTP:
		rc = tb_set_utp_opts(options, fd);
		break;

	case eUDP:
		rc = tb_set_eudp_opts(options, fd);
		break;

	case DCCP:
		rc = tb_set_dccp_opts(options, fd);
		break;

	default:
		PRT_ERR("No such protocol");
		return -1;

	}

	return rc;
}

int
tb_set_bsd_sock_opts(tb_options_t *options, int fd)
{
	socklen_t size = (socklen_t)sizeof(options->l3_r_b_size);

	if(setsockopt(fd, SOL_SOCKET, SO_RCVBUF,
			(void*)&options->l3_r_b_size, size) != 0)
	{
		perror("Error: setsockopt: RCVBUF");
		return -1;
	}

	size = (socklen_t)sizeof(options->l3_s_b_size);

	if(setsockopt(fd, SOL_SOCKET, SO_SNDBUF,
			(void*)&options->l3_s_b_size, size) != 0)
	{
		perror("Error: setsockopt: SNDBUF");
		return -1;
	}

	int s_size = 0;
	int r_size = 0;

	getsockopt(fd, SOL_SOCKET, SO_SNDBUF,
			(void*)&s_size, &size);

	getsockopt(fd, SOL_SOCKET, SO_RCVBUF,
			(void*)&r_size, &size);

	PRT_I_D("set sndbuf size%d", options->l3_s_b_size);
	PRT_I_D("actual sndbuf size%d", s_size);
	PRT_I_D("set rcvbuf size%d", options->l3_r_b_size);
	PRT_I_D("actual rcvbuf size%d", r_size);

	int on = 1;

	if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof on) != 0)
	{
		perror(RED "Error: tb_set_udp_opts: setsockopt" RESET);
		return -1;
	}

	return 0;
}

int
tb_set_tcp_opts(tb_options_t *options, int fd)
{
	PRT_ACK("Setting tcp options");

	if(options->l3_r_b_size == 0)
	{
		options->l3_r_b_size = TCP_RCV_BUFF;
	}

	if(options->l3_s_b_size == 0)
	{
		options->l3_s_b_size = TCP_SND_BUFF;
	}

	return tb_set_bsd_sock_opts(options, fd);
}

int
tb_set_udp_opts(tb_options_t *options, int fd)
{
	PRT_INFO("Setting UDP options");

	if(options->l3_r_b_size == 0)
	{
		PRT_INFO("Setting default receive buffer");
		options->l3_r_b_size = UDP_RCV_BUFF;
	}

	if(options->l3_s_b_size == 0)
	{
		PRT_INFO("Using defalt UDP send buffer");
		options->l3_s_b_size = UDP_SND_BUFF;
	}

	return tb_set_bsd_sock_opts(options, fd);
}

int
tb_set_udt_opts(tb_options_t *options, int fd)
{
	int rc;

	if(options->l4_r_b_size == 0)
	{
		PRT_INFO("Using default UDT receive buffer");
		options->l4_r_b_size = UDT_RCV_BUFF;
	}

	if(options->l4_s_b_size == 0)
	{
		PRT_INFO("Using default UDT send buffer");
		options->l4_s_b_size = UDT_SND_BUFF;
	}

	if(options->l3_r_b_size == 0)
	{
		PRT_INFO("Using default UDP receive buffer");
		options->l3_r_b_size = UDP_RCV_BUFF;
	}

	if(options->l3_s_b_size == 0)
	{
		PRT_INFO("Using default UDP send buffer");
				options->l3_s_b_size = UDP_SND_BUFF;
	}

	PRT_I_D("Setting UDT read buffer: %d", options->l4_r_b_size);
	rc = udt_setsockopt(fd, 0,
			UDT_RCVBUF, &options->l4_r_b_size, sizeof(UDT_RCV_BUFF));

	if(rc == -1)
	{
		fprintf(stderr, "Error setting udt rcv buffer:");
		fprintf(stderr, " %s\n", udt_getlasterror_desc());

		return -1;
	}

	PRT_I_D("Setting UDT write buffer: %d", options->l4_s_b_size);
	rc = udt_setsockopt(fd, 0, UDT_SNDBUF, &options->l4_s_b_size,
			sizeof(UDT_SND_BUFF));

	if(rc == -1)
	{
		fprintf(stderr, "Error setting udt send buffer: ");
		fprintf(stderr, " %s\n", udt_getlasterror_desc());

		return -1;
	}

	PRT_I_D("Setting UDP receive buffer: %d", options->l3_r_b_size);
		rc = udt_setsockopt(fd, 0,
				UDP_RCVBUF, &options->l3_r_b_size, sizeof(UDP_RCV_BUFF));

	if(rc == -1)
	{
		fprintf(stderr, "Error setting udp rcv buffer:");
		fprintf(stderr, " %s\n", udt_getlasterror_desc());

		return -1;
	}

	PRT_I_D("Setting UDP send buffer: %d", options->l3_r_b_size);
		rc = udt_setsockopt(fd, 0,
				UDP_SNDBUF, &options->l3_r_b_size, sizeof(UDP_SND_BUFF));

	if(rc == -1)
	{
		fprintf(stderr, "Error setting udp snd buffer:");
		fprintf(stderr, " %s\n", udt_getlasterror_desc());

		return -1;
	}

	return 0;
}

int
tb_set_audt_opts(tb_options_t *options, int fd)
{
	return 0;
}

int
tb_set_utp_opts(tb_options_t *options, int fd)
{
	PRT_INFO("Setting uTP options");

	if(options->l4_r_b_size == 0)
	{
		options->l4_r_b_size = uTP_RCV_BUFF;
	}

	if(options->l4_s_b_size == 0)
	{
		options->l4_s_b_size = uTP_SND_BUFF;
	}

	if(options->l3_r_b_size == 0)
	{
		options->l3_r_b_size = UDP_RCV_BUFF;
	}

	if(options->l3_s_b_size == 0)
	{
		options->l3_s_b_size = UDP_SND_BUFF;
	}


	return 0;
}

int
tb_set_eudp_opts(tb_options_t *options, int fd)
{
	PRT_ACK("Setting eUDP options");

	return tb_set_udp_opts(options, fd);
}

int
tb_set_dccp_opts(tb_options_t *options, int fd)
{
	PRT_ACK("Setting DCCP socket options");

	if(options->l3_r_b_size == 0)
	{
		options->l3_r_b_size = DCCP_RCV_BUFF;
	}

	if(options->l3_s_b_size == 0)
	{
		options->l3_s_b_size = DCCP_SND_BUFF;
	}

	return tb_set_bsd_sock_opts(options, fd);
}
