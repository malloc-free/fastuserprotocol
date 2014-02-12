/*
 * tb_worker_pair.c
 *
 *  Created on: 25/11/2013
 *      Author: michael
 */

#include "tb_worker_pair.h"
#include <stdlib.h>
#include <string.h>
#include <gdsl.h>

char
*get_pair_key(void *data){
	tb_session_t *s = ((tb_worker_pair_t*)data)->data;

	return get_key_value(s);
}

gdsl_element_t
*allocate_tuple(void *data){
	tb_worker_pair_t *t = malloc(sizeof(tb_worker_pair_t));

	memcpy((void*)t, data, sizeof(tb_worker_pair_t));

	return (gdsl_element_t*)t;
}

void
free_tuple(gdsl_element_t *data){
	free(data);
}

