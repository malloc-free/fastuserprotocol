/*
 * perf_tests.c
 *
 *  Created on: 1/02/2014
 *      Author: michael
 */

#include "../src/tb_protocol.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

void
test_funct_perf();

inline int
test_send(int fd, void *buf, size_t n, int sock_flags) __attribute__((always_inline));

long
calc_time(struct timespec start, struct timespec end);

#define ITERATIONS 10000000l
#define NANOS 1000000000

int
main(void)
{
	test_funct_perf();
	return 0;
}

void
test_funct_perf()
{
	unsigned long x;
	long r_time;

	tb_protocol_t *protocol = malloc(sizeof(tb_protocol_t));
	protocol->f_send = &test_send;

	struct timespec start, end;

	clock_gettime(CLOCK_MONOTONIC, &start);


	for(x = 0l; x < ITERATIONS; x++)
	{
		protocol->f_send(0, NULL, 0, 0);
	}

	clock_gettime(CLOCK_MONOTONIC, &end);

	r_time = calc_time(start, end);

	fprintf(stdout, "Time for function pointer %f\n", ((double)r_time) / ITERATIONS);

	clock_gettime(CLOCK_MONOTONIC, &start);

	for(x = 0l; x < ITERATIONS; x++)
	{
		test_send(0, NULL, 0, 0);
	}

	clock_gettime(CLOCK_MONOTONIC, &end);

	r_time = calc_time(start, end);

	fprintf(stdout, "Time for function %f\n", ((double)r_time) / ITERATIONS);

	free(protocol);
}

long
calc_time(struct timespec start, struct timespec end)
{
	double r_time;

	r_time = (end.tv_sec * NANOS) + end.tv_nsec;
	r_time -= (start.tv_sec * NANOS) + start.tv_nsec;

	return r_time;
}

int
test_send(int fd, void *buf, size_t n, int sock_flags)
{
	return 0;
}


