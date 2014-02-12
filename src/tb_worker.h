/*!
 * tb_worker.h
 *
 *  Created on: 20/11/2013
 *  Author: Michael Holmwood
 */

#ifndef TB_WORKER_H_
#define TB_WORKER_H_

#include <gdsl/gdsl_queue.h>
#include <pthread.h>
#include "tb_session.h"

/** @struct <tb_worker_t> [tb_worker.h]
 *  @brief struct that defines fields for worker.
 *
 *	This struct defines the fields for class tb_worker. The
 *	queue contains new work for the worker. The queue_lock
 *	is used to make the queue thread safe. The worker will
 *	sleep if the work queue is empty. Newly added work wakes
 *	the thread up using the queue_cond.
 *
 */
typedef struct
{
	int __id;	///< The unique id of this worker.
	gdsl_queue_t __queue; ///< The work for this worker.
	pthread_mutex_t *__queue_lock; ///< The lock for the queue.
	pthread_cond_t *__queue_cond; ///< The condition for the queue.
	int __stop; ///< Stops the thread, 0 to continue, 1 to stop.
} tb_worker_t;

/**
 * 	@brief Start work.
 *
 * 	This is the function that is passed to pthread_create. The
 * 	worker struct is passed in by pthread_create as per the
 * 	api
 *
 * 	@see pthread.h
 * 	@see pthread_create
 * 	@pre param Must be of struct type worker, cannot be NULL.
 * 	@param worker The worker to do the specified work.
 * 	@return void
 */
void
*do_work(void *worker);

/**
 * @brief Destroy the given worker, freeing memory.
 *
 * @note
 * @pre w must be of struct type worker.
 * @param w The worker to destroy
 */
void
destroy_worker(tb_worker_t *w);

/**
 * @brief Create a new worker, allocating memory as required.
 *
 * @note Each worker gets a new, unique id that is generated at the
 * time of creation
 * @return worker* Pointer to new tb_worker_t.
 */
tb_worker_t
*create_worker();

/**
 * @brief Compare two workers, required by gdsl_heap.
 *
 * gdsl_heap_t is a max heap, so workers with a lesser number
 * of tasks will return > 0, same number of tasks = 0, and greater
 * number of tasks < 0.
 *
 * @pre E and value must be both of struct type worker.
 * @pre E and value cannot be NULL.
 * @param E The element to compare.
 * @param value The element to compare against.
 * @return -1, 0 and 1 if E is greater, equal, or less than.
 */
long int
compare_workers(gdsl_element_t E, void *value);

/**
 * @brief Get work for this worker.
 *
 * Gets a unit of work for this worker. Blocks if the queue is
 * empty.
 *
 * @pre worker must be of struct type worker.
 * @pre worker cannot be NULL
 * @param worker The worker to fetch the work for.
 * @return Work to be done by this tb_worker_t.
 */
tb_session_t
*get_work(tb_worker_t *worker);

/**
 * @brief Add work to a worker.
 *
 * Add work to the given worker. Is thread-safe, so it will
 * block if another thread is adding work to the queue.
 * The work is added to the work queue in the given worker.
 *
 * @pre worker must be of struct type tb_worker_t.
 * @pre data must be of type tb_session_t.
 * @Neither parameters can be NULL.
 * @param worker The worker to add work to.
 * @param data The work to be added.
 * @return void
 */
void
add_work(tb_worker_t *worker, tb_session_t *data);

#endif /* TB_WORKER_H_ */
