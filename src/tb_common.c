/*
 * tb_common.c
 *
 *  Created on: 23/01/2014
 *      Author: michael
 */
#include "tb_common.h"
#include "tb_logging.h"

#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>
#include <string.h>

void
tb_print_address(struct sockaddr_storage *store)
{
	char *addr = tb_get_address(store);
	PRT_I_S("Address:", tb_get_address(store));
	free(addr);
}

char
*tb_get_address(struct sockaddr_storage *store)
{
	char *addr;

	if(store->ss_family == AF_INET)
	{
		addr = malloc(INET_ADDRSTRLEN);
		inet_ntop(AF_INET, &((struct sockaddr_in*)store)->sin_addr, addr,
				INET_ADDRSTRLEN);
	}
	else
	{
		addr = malloc(INET6_ADDRSTRLEN);
		inet_ntop(AF_INET6, &((struct sockaddr_in6*)store)->sin6_addr, addr,
				INET6_ADDRSTRLEN);
	}

	return addr;
}

tb_time_t
*tb_create_time(clockid_t clk_id)
{
	tb_time_t *time = malloc(sizeof(tb_time_t));
	time->clk_id = clk_id;
	time->start = malloc(sizeof(struct timespec));
	time->finish = malloc(sizeof(struct timespec));
	time->n_sec = 0;
	time->started = 0;

	return time;
}

void
tb_destroy_time(tb_time_t *time)
{
	free(time->start);
	free(time->finish);
	free(time);
}

void
tb_start_time(tb_time_t *time)
{
	clock_gettime(time->clk_id, time->start);
	time->started = 1;
}

void
tb_finish_time(tb_time_t *time)
{
	clock_gettime(time->clk_id, time->finish);
	tb_calculate_time(time);
	time->stopped = 1;
}

void
tb_calculate_time(tb_time_t *time)
{
	 time->n_sec = (time->finish->tv_sec * 1000000000) + time->finish->tv_nsec;
	 time->n_sec -= (time->start->tv_sec * 1000000000) + time->start->tv_nsec;
}

/////////////////// File Handling Functions //////////////////////

char
*tb_create_test_file(char *file_name, int *file_size)
{
	char *data;

	if(file_name != NULL)
	{
		fprintf(stdout, "Attempting to open file: %s\n",
				file_name);

		data = tb_load_test_file(file_name, file_size);
	}
	else if(file_size != 0)
	{
		fprintf(stdout, "Attempting to create random file of size %d\n",
				*file_size);

		data = tb_load_random_file(*file_size);

		PRT_INFO("Random creation success");
	}
	else
	{
		return NULL;
	}

	return data;
}

char
*tb_load_test_file(char *file_name, int *file_size)
{

	FILE *fp  = fopen(file_name, "rb");

	if(fp == NULL)
	{
		return NULL;
	}

	*file_size = fseek(fp, 0, SEEK_END);
	if(*file_size == EOF)
	{
		return NULL;
	}

	fseek(fp, 0, SEEK_SET);

	char *data = malloc(*file_size);
	if((fread(data, sizeof(char), *file_size, fp))
			== EOF)
	{
		return NULL;
	}

	return data;
}

char
*tb_load_random_file(int size)
{
	FILE *rand_file;
	char num_str[10];
	char cat_str[20];
	sprintf(num_str, "%d", size);
	snprintf(cat_str, sizeof(cat_str), "%s.ran", num_str);

	PRT_I_S("Opening file", cat_str);

	if((rand_file = fopen(cat_str, "rb")) == NULL)
	{
		PRT_INFO("File does not exist, creating");

		return tb_create_random(cat_str, size);
	}

	char *r_data = malloc(size);

	if(fread(r_data, sizeof(char), size, rand_file) == EOF)
	{
		perror("Cannot read data");
		fclose(rand_file);
		free(r_data);

		PRT_INFO("Could not load file, creating");
		//If reading fails, attempt to create a random file.
		return tb_create_random(cat_str, size);
	}

	return r_data;
}

char
*tb_create_random(char *path, int size)
{
	PRT_I_D("Creating file of size %d", size);

	FILE *rand_file;

	if((rand_file = fopen(path, "wb")) == NULL)
	{
		perror("tb_create_random: fopen");
		return NULL;
	}

	char *r_data = malloc(size);

	if(r_data != NULL)
	{
		unsigned int x;

		srand((unsigned int) time(NULL));

		for(x = 0; x < size; x++)
		{
			r_data[x] = rand();
		}
	}
	else
	{
		PRT_ERR("Cannot create random file, aborting");
		free(r_data);
		return NULL;
	}


	if((fwrite(r_data, sizeof(char), size, rand_file)) == EOF)
	{
		perror("tb_create_random: fwrite");
		return NULL;
	}

	return r_data;
}
