/*
 * tb_worker.c
 *
 *  Created on: 20/11/2013
 *      Author: michael
 */

#include "tb_worker.h"
#include "tb_session.h"

#include <gdsl/gdsl_queue.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>

static int group_id = 0;

/* @fn
 * Function performed by this worker.
 */
void
*do_work(void *param){
	tb_worker_t *w = (tb_worker_t*)param;

	fprintf(stdout, "Id for this worker: %d\n", w->__id);

	while(!w->__stop){
		tb_session_t *data = get_work(w);
		fprintf(stdout, "Work is: %s\n", data->file_name);
		free_session(data);
	}

	printf("Worker %d: No work, exiting\n", w->__id);
	return 0;
}

/*
 * Used to create a worker.
 */
tb_worker_t
*create_worker(){
	tb_worker_t *w = malloc(sizeof(tb_worker_t));
	assert(w != NULL);
	w->__id = ++group_id;
	w->__queue = gdsl_queue_alloc("Queue", NULL, NULL);
	w->__stop = 0;
	w->__queue_lock = malloc(sizeof(pthread_mutex_t));
	w->__queue_cond = malloc(sizeof(pthread_cond_t));
	pthread_mutex_init(w->__queue_lock, NULL);
	pthread_cond_init(w->__queue_cond, NULL);
	return w;
}

/*
 * Used to destroy a worker.
 */
void
destroy_worker(tb_worker_t *w){
	gdsl_queue_free(w->__queue);
	pthread_mutex_destroy(w->__queue_lock);
	pthread_cond_destroy(w->__queue_cond);
	free(w);
}

/*
 * Add work to this worker. Blocks if the __queue is being accessed.
 */
void
add_work(tb_worker_t *worker, tb_session_t *data){
	tb_session_t *cp = malloc(sizeof(tb_session_t));
	memcpy(cp, data, sizeof(tb_session_t));
	pthread_mutex_lock(worker->__queue_lock);
	gdsl_queue_insert(worker->__queue, cp);
	pthread_cond_signal(worker->__queue_cond);
	pthread_mutex_unlock(worker->__queue_lock);
}

/*
 * Get and return the head of the __queue. Blocks if there is an
 * existing lock on the __queue, or the __queue is empty.
 */
tb_session_t
*get_work(tb_worker_t *worker){
	printf("Worker %d getting work\n", worker->__id);

	pthread_mutex_lock(worker->__queue_lock);

	tb_session_t *data;

	if((data = gdsl_queue_remove(worker->__queue)) == NULL){
		printf("Worker %d: No work, blocking\n", worker->__id);
		pthread_cond_wait(worker->__queue_cond, worker->__queue_lock);
		printf("Worker %d: Unlocked, getting work\n", worker->__id);
		data = gdsl_queue_remove(worker->__queue);
	}

	pthread_mutex_unlock(worker->__queue_lock);

	return data;
}

/*
 * Compares workers, used by gdsl_heap_t
 */
long int
compare_workers(gdsl_element_t E, void *value){
	long int lOne = (long int)gdsl_queue_get_size(((tb_worker_t*)E)->__queue);
	long int lTwo = (long int)gdsl_queue_get_size(((tb_worker_t*)value)->__queue);

	return lTwo - lOne;
}
