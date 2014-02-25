/*
 * main.c
 *
 *  Created on: 5/12/2013
 *      Author: michael
 */

#include "tb_testbed.h"
#include "tb_protocol.h"
#include "tb_listener.h"
#include "tb_session.h"
#include "tb_common.h"
#include "tb_utp.h"
#include "tb_udp.h"
#include "tb_sock_opt.h"
#include "tb_logging.h"
#include "tb_endpoint.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <time.h>
#include <assert.h>
#include <signal.h>
#include <sched.h>
#include <netdb.h>
#include <errno.h>

//The time between reads.
const unsigned long long m_sleep = 100000;

int
main(int argc, char *argv[])
{
	return tb_parse(argc, argv);
}

int
test()
{
	PRT_ERR("Nothing to test");

	return 0;
}

/**
 * argv[0] = TestBed
 * argv[1] = Type (server, client)
 * argv[2] = Address
 * argv[3] = Port
 * argv[4] = Protocol (tcp, udp, udt)
 * argv[5] = Bufsize (MB)
 * argv[6] = File type (size, file)
 * argv[7] = Filename/Fliesize
 */
int
tb_parse(int argc, char *argv[])
{
	ENDPOINT_TYPE type;

	fprintf(stdout, CLEAR MOVE_CSR(0, 0) GREEN "starting\n" RESET);

	if(argc < 6 || argc > 8)
	{
		fprintf(stderr, "usage: Main type address port protocol bufsize "
				"[filename]\n");
		fprintf(stderr, "If client type is required, please specify "
				"[filetype] followed by either file size or name\n");
		exit(-1);
	}

	if(strcmp("server", argv[TYPE]) == 0)
	{
		type = SERVER;
		fprintf(stdout, "Creating server\n");

	}
	else if(strcmp("client", argv[TYPE]) == 0 && argc == 8)
	{
		type = CLIENT;
		PRT_INFO("Creating single connection client\n");
	}
	else if(strcmp("mserver", argv[TYPE]) == 0)
	{
		type = mSERVER;
		PRT_INFO("Creating multiple connection server");
	}
	else if(strcmp("mclient", argv[TYPE]) == 0)
	{
		type = mCLIENT;
		PRT_INFO("");
	}
	else
	{
		fprintf(stderr, "type must be server or client\n");
		exit(-1);
	}

	PROTOCOL protocol;

	if(strcmp("tcp", argv[PROT]) == 0)
	{
		fprintf(stdout, "Setting to TCP\n");
		protocol = TCP;
	}
	else if(strcmp("udp", argv[PROT]) == 0)
	{
		fprintf(stdout, "Setting to UDP\n");
		protocol = UDP;
	}
	else if(strcmp("udt", argv[PROT]) == 0)
	{
		fprintf(stdout, "Setting to UDT\n");
		protocol = UDT;
	}
	else if(strcmp("audt", argv[PROT]) == 0)
	{
		fprintf(stdout, "Setting to aUDT\n");
		protocol = aUDT;
	}
	else if(strcmp("utp", argv[PROT]) == 0)
	{
		fprintf(stdout, "Setting to utp\n");
		protocol = uTP;
	}
	else if(strcmp("eudp", argv[PROT]) == 0)
	{
		fprintf(stdout, "Setting to eudp\n");
		protocol = eUDP;
	}
	else if(strcmp("dccp", argv[PROT]) == 0)
	{
		fprintf(stdout, "Setting to dccp\n");
		protocol = DCCP;
	}
	else
	{
		fprintf(stderr, "That protocol is not supported\n");
		exit(-1);
	}

	fprintf(stdout, "Connecting to %s on port %s using protocol %d:%s\n",
			argv[2], argv[3], protocol, argv[4]);

	int bufsize;
	sscanf(argv[BUF_SIZE], "%d", &bufsize);

	tb_listener_t *listener = tb_create_listener(type, argv[ADDRESS],
			argv[PORT], protocol, bufsize);

	if(listener->e_type == CLIENT || listener->e_type == mCLIENT)
	{
		if(strcmp("size", argv[FILE_TYPE]) == 0)
		{
			fprintf(stdout, "Using filesize\n");
			sscanf(argv[FILE_NAME], "%d", &listener->file_size);
			listener->file_size *= 1024;
			float sz = listener->file_size / 1024.0;
			fprintf(stdout, "fliesize (KB) = %d\n", listener->file_size);
			fprintf(stdout, "filesize (MB) = %f\n", sz);
		}
		else if(strcmp("file", argv[FILE_TYPE]) == 0)
		{
			fprintf(stdout, "Using filename\n");
			listener->filename = strdup(argv[FILE_NAME]);
			fprintf(stdout, "filename = %s\n", listener->filename);
		}
		else
		{
			fprintf(stderr, "File type must be size or file\n");
			tb_abort(listener);
		}
	}

	if(listener->bufsize == 0)
	{
		if(listener->e_type == CLIENT || listener->e_type == mCLIENT)
		{
			PRT_INFO("Bufsize is 0, setting bufsize to filesize");
			listener->bufsize = listener->file_size;
		}
		else
		{
			PRT_ERR("e_type is SERVER, must supply buffer size");
			PRT_INFO("Setting to default of 1408 bytes");
			listener->bufsize = 1408;
		}
	}

	PRT_I_D("Buf size = %d\n", listener->bufsize);

	listener->d_exit = 1;

	listener->f_exit = (funct_l_exit)&tb_exit;
	listener->f_abort = (funct_l_abort)&tb_abort;

	tb_start(listener);

	pthread_exit(NULL);
}

void
tb_start(tb_listener_t *listener)
{
	PRT_INFO("Starting");
	LOG(listener, "Starting", LOG_INFO);

	tb_print_listener(listener);

	PRT_INFO("tb_start: Resolving address");
	if(tb_resolve_address(listener) == -1)
	{
		tb_abort(listener);
	}

	if(listener->protocol->protocol != uTP)
	{
		tb_create_socket(listener);

	}

	PRT_INFO("Setting socket options");
	if(tb_set_sock_opt(listener->options, listener->sock_d) == -1)
	{
		tb_abort(listener);
	}


	if(listener->e_type == SERVER || listener->e_type == mSERVER)
	{
		PRT_INFO("Creating server thread");
		pthread_create(listener->__l_thread, NULL, &tb_server, listener);
	}

	//This is here, in case we decided to look at downloading files
	//from the server.
	else if(listener->e_type == CLIENT || listener->e_type == mCLIENT)
	{
		PRT_INFO("Creating client thread");
		pthread_create(listener->__l_thread, NULL, &tb_client, (void*)listener);
	}
	else if(listener->e_type == SERVER || listener->e_type == mSERVER)
	{
		fprintf(stdout, "Not of recognized type: %d", listener->e_type);
		tb_abort(listener);
	}
	else
	{
		exit(-1);
	}

	//Set the affinity of the listener to 0 for server, 1 for client.
	PRT_INFO("Setting affinity for listener thread");

	cpu_set_t set;
	CPU_ZERO(&set);
	CPU_SET(listener->e_type, &set);

	if(pthread_setaffinity_np(*listener->__l_thread, sizeof(cpu_set_t), &set)
			!= 0)
	{
		PRT_ERR("Cannot set affinity");
	}

	//Monitor events
	if(listener->monitor)
	{
		//Set affinity for main thread.
		PRT_INFO("Setting affinity for main thread");

		pthread_t this_t = pthread_self();
		CPU_ZERO(&set);
		CPU_SET(listener->e_type + 2, &set);

		if(pthread_setaffinity_np(this_t, sizeof(cpu_set_t), &set) != 0)
		{
			PRT_ERR("Cannot set affinity");
		}

		tb_monitor(listener);
	}


	if(listener->d_exit)
	{
		PRT_INFO("Destroying Listener");
		tb_destroy_listener(listener);
	}
}

int
tb_monitor(tb_listener_t *listener)
{
	//Wait for socket to be set.
	while(listener->sock_d == -1);

	int prev_bytes = 0, peak_bytes = 0;
	char animate[8] = {'|', '/', '-', '\\', '|', '/', '-', '\\'};
	int index = 0;

	cpu_set_t set;
	CPU_ZERO(&set);

	pthread_getaffinity_np(*listener->__l_thread, sizeof(cpu_set_t), &set);

	unsigned int x = 0;

	for(; x < listener->num_proc; x++)
	{
		if(CPU_ISSET(x, &set))
		{
			listener->cpu_affinity = x;
			break;
		}
	}

	CPU_ZERO(&set);

	pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &set);

	for(; x < listener->num_proc; x++)
	{
		if(CPU_ISSET(x, &set))
		{
			listener->main_cpu_aff = x;
			break;
		}
	}

	while(listener->status != TB_EXITING &&
			listener->status != TB_ABORTING){

		usleep(m_sleep);

		if(listener->status == TB_CONNECTED)
		{
			//Connected, read stats.
			if(listener->e_type == CLIENT || listener->e_type == SERVER)
			{
				tb_set_l_stats(listener);
			}
			else
			{
				tb_set_m_stats(listener);
			}

			listener->stats->byte_sec = listener->stats->current_read - prev_bytes;
			prev_bytes = listener->stats->current_read;
			listener->stats->total_bytes += listener->stats->byte_sec;
			listener->stats->num_m_seconds++;

			listener->stats->av_byte_sec = (double)listener->stats->total_bytes /
					(double)listener->stats->num_m_seconds;

			listener->stats->byte_sec *= 10;
			listener->stats->av_byte_sec *= 10;

			if(listener->print_stats)
			{
				tb_print_stats(listener->stats, listener);
			}

			if(listener->stats->byte_sec > peak_bytes)
			{
				peak_bytes = listener->stats->peak_byte_sec
						= listener->stats->byte_sec;
			}
		}
		else
		{
			//Not currently connected, report status.
			fprintf(stdout, "\rWaiting for transfer %c",
					animate[index++]);

			fflush(stdout);

			if(index == 8)
			{
				index = 0;
			}

			if(listener->e_type == CLIENT || listener->e_type == SERVER)
			{
				tb_set_l_stats(listener);
			}
			else
			{
				tb_set_m_stats(listener);
			}
		}
	}

	//Get the time stats for the test.
	tb_get_time_stats(listener);

	fprintf(stdout, "\nTime for transfer: %f seconds\n",
				listener->transfer_time->n_sec / (double)1000000000);

	fprintf(stdout, "Time for connection: %lld nano seconds\n",
			listener->connect_time->n_sec);

	//Iterate through the stats for each session, if they exist.
	tb_prot_stats_t *stats = listener->stats->n_stats;
	if(stats)
	{
		PRT_INFO("Print stats for sessions");
	}

	while(stats)
	{
		fprintf(stdout, "Time for transfer: %f seconds\n",
					stats->transfer_time / (double)1000000000);

		fprintf(stdout, "Time for connection: %lld nano seconds",
				stats->connect_time);

		stats = stats->n_stats;
	}

	//Final Read.
	if(listener->e_type == CLIENT || listener->e_type == SERVER)
	{
		tb_set_l_stats(listener);
	}
	else
	{
		tb_set_m_stats(listener);
	}

	fprintf(stdout, "Main thread exiting\n");

	return 0;
}

void
tb_print_stats(tb_prot_stats_t *stats, tb_listener_t *listener)
{
	fprintf(stdout, CLEAR MOVE_CSR(1, 1) LINE);

	//Get the string representation for the protocol.
	fprintf(stdout, "protocol: %d\n", listener->protocol->protocol);

	//Print stats
	fprintf(stdout, "Total bytes transferred = %lld\n", stats->current_read);
	fprintf(stdout, "file size = %d\n", listener->file_size);
	fprintf(stdout, "rtt = %f\n", stats->rtt);
	fprintf(stdout, "rttvar = %f\n", stats->rtt_var);
	fprintf(stdout, "bytes/second = %d\n", stats->byte_sec);
	fprintf(stdout, "bytes/second average = %.2f\n", stats->av_byte_sec);
	fprintf(stdout, "peak bytes/second = %d\n", stats->peak_byte_sec);
	fprintf(stdout, "peak Kbytes/second = %.2f\n", stats->peak_byte_sec / (double)1024);
	fprintf(stdout, "KB/s = %f\n", stats->byte_sec / (double)1024);
	fprintf(stdout, "MB/s = %f\n", stats->byte_sec / (double)(1024 * 1024));
	fprintf(stdout, "av KB/s = %f\n", (stats->av_byte_sec / (double)1024));
	fprintf(stdout, "prot recv rate (Mb/s) = %f\n", stats->recv_rate);
	fprintf(stdout, "prot recv rate (KB/s) = %f\n", (stats->recv_rate / 8.0) * 1024.0);
	fprintf(stdout, "cpu affinity = %d\n", listener->cpu_affinity);
	fprintf(stdout, "main thread cpu affinity = %d\n", listener->main_cpu_aff);
	fprintf(stdout, "recv_p_loss = %d\n", stats->recv_p_loss);
	fprintf(stdout, "send_p_loss = %d\n", stats->send_p_loss);
	fprintf(stdout, "Current recv buff = %d\n", stats->r_buff_size);
	fprintf(stdout, "Current send buff = %d\n", stats->w_buff_size);
	fprintf(stdout, "Current send window = %f\n", stats->send_window);
}

////////////////////// Abort and Exit Functions ///////////////////

void
tb_exit(void *data)
{
	tb_listener_t *listener = (tb_listener_t*)data;
	listener->status = TB_EXITING;
	PRT_INFO("Exiting");
	LOG(listener, "Exiting", LOG_INFO);

	pthread_exit(NULL);
}

void
tb_abort(void *data)
{
	tb_listener_t *listener = (tb_listener_t*)data;

	if(listener->sock_d != -1)
	{
		pthread_mutex_lock(listener->stat_lock);
		PRT_INFO("Disconnecting");
		listener->protocol->f_close(listener->sock_d);
		listener->status = TB_DISCONNECTED;
		pthread_mutex_unlock(listener->stat_lock);
	}

	listener->status = TB_ABORTING;
	PRT_ERR("Aborting");
	LOG(listener, "Aborting", LOG_ERR);

	pthread_exit(NULL);
}
