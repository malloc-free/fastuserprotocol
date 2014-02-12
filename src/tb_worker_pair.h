/*
 * tb_worker_pair.h
 *
 *  Created on: 25/11/2013
 *      Author: michael
 */

#ifndef TB_WORKER_PAIR_H_
#define TB_WORKER_PAIR_H_

#include "tb_worker.h"
#include "tb_session.h"
#include <gdsl/gdsl_hash.h>

typedef struct{
	tb_worker_t *work;
	tb_session_t *data;
} tb_worker_pair_t;

char *get_pair_key(void *data);
gdsl_element_t *allocate_tuple(void *data);
void free_tuple(gdsl_element_t *data);

#endif /* TB_WORKER_PAIR_H_ */
