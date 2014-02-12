/*
 * aff_test.c
 *
 *  Created on: 27/01/2014
 *      Author: michael
 */


#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <unistd.h>

void
*test(void *data);

int
main(void)
{
	pthread_t thread;
	pthread_create(&thread, NULL, &test, NULL);

	cpu_set_t set;
	CPU_ZERO(&set);
	CPU_SET(0, &set);


	if(pthread_setaffinity_np(thread, sizeof(cpu_set_t), &set) != 0)
	{
		fprintf(stderr, "Cannot set affinity of thread");
	}

	CPU_ZERO(&set);
	CPU_SET(4, &set);

	sleep(5);

	if(pthread_setaffinity_np(thread, sizeof(cpu_set_t), &set) != 0)
	{
		fprintf(stderr, "Cannot set affinity of thread");
	}

	pthread_exit(NULL);
}

void
*test(void *data)
{
	while(1);
	fprintf(stdout, "Exiting");

	pthread_exit(NULL);
}
