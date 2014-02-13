/*
 * tb_endpoint.c
 *
 *  Created on: 7/02/2014
 *      Author: michael
 */

#include "tb_endpoint.h"

//Common Includes
#include "tb_testbed.h"
#include "tb_protocol.h"
#include "tb_listener.h"
#include "tb_common.h"

//Protocol includes
#include "tb_udp.h"
#include "tb_utp.h"
#include "tb_udt.h"
#include "tb_stream.h"

//System Includes
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

void
*tb_client(void *data)
{
	PRT_INFO("Running client\n");

	tb_listener_t *listener = (tb_listener_t*)data;

	PRT_INFO("Creating file");
	listener->data = tb_create_test_file(listener->filename,
			&listener->file_size);

	PRT_INFO("Getting thread info");
	tb_get_cpu_info(listener);

	long long totalbytes;

	switch(listener->protocol->protocol)
	{
	case uTP:
		if(listener->e_type == CLIENT)
		{
			PRT_INFO("Running uTP client");
			totalbytes = tb_utp_client(listener);
		}
		else
		{
			PRT_INFO("Running uTP mClient");
			totalbytes = tb_utp_m_client(listener);
		}

		break;

	case TCP:
	case DCCP:
		if(listener->e_type == CLIENT)
		{
			PRT_INFO("Running stream (TCP or DCCP) socket client");
			totalbytes = tb_stream_client(listener);
		}
		else
		{
			PRT_INFO("Running multi stream (TCP or DCCP) socket client");
			totalbytes = tb_stream_m_client(listener);
		}

		break;

	case UDP:
		if(listener->e_type == CLIENT)
		{
			PRT_INFO("Running UDP client");
			totalbytes = tb_udp_client(listener);
		}
		else
		{
			PRT_INFO("Running UDP mClient");
			totalbytes = tb_udp_m_client(listener);
		}
		break;

	case UDT:
		if(listener->e_type == CLIENT)
		{
			PRT_INFO("Running UDT client");
			totalbytes = tb_udt_client(listener);
		}
		else
		{
			PRT_INFO("Running UDT mClient");
			totalbytes = tb_udt_m_client(listener);
		}

		break;

	default:
		PRT_ERR("Not a supported protocol");
		tb_abort(listener);
	}


	fprintf(stdout, "Number of bytes sent: %lld\n", totalbytes);

	tb_exit(listener);

}

void
*tb_server(void *data)
{
	tb_listener_t *listener = (tb_listener_t*)data;

	PRT_INFO("Getting thread info");
	tb_get_cpu_info(listener);

	if(listener->protocol->protocol != uTP)
	{
		PRT_INFO("Binding");
		tb_bind(listener);
	}

	switch(listener->protocol->protocol)
	{
	case uTP:
		PRT_INFO("Running uTP mode\n");
		tb_utp_server(listener);

		break;

	case UDP:
		if(listener->e_type == SERVER)
		{
			PRT_INFO("Running UDP single connection server");
			tb_udp_server(listener);
		}
		else
		{
			PRT_INFO("Running UDP multiple connection server");
			tb_udp_server(listener);
		}

		break;

	case TCP:
	case DCCP:
		PRT_INFO("Runninng Stream mode");
		if(listener->e_type == SERVER)
		{
			PRT_INFO("Server mode");
			tb_stream_server(listener);
		}
		else
		{
			PRT_INFO("mServer mode");
			tb_stream_m_server(listener);
		}

		break;

	case UDT:
		PRT_INFO("Running UDT mode");
		if(listener->e_type == SERVER)
		{
			PRT_INFO("Server mode");
			tb_udt_server(listener);
		}
		else
		{
			PRT_INFO("mServer mode");
			tb_udt_m_server(listener);
		}

		break;

	default:
		PRT_ERR("Unrecognized protocol, aborting");
		tb_abort(listener);
	}

	fprintf(stdout, "Total bytes received = %lld\n", listener->total_tx_rx);

	tb_exit(listener);
}
