/*
 * log_test.c
 *
 *  Created on: 29/01/2014
 *      Author: michael
 */

#include <tb_logging.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

int
main(void)
{
	tb_log_t *log = tb_create_log("logfile");

	assert(log != NULL);
	assert(tb_write_log(log, "Test log info", LOG_INFO) == 0);
	assert(tb_write_log(log, "Test log ack", LOG_ACK) == 0);
	assert(tb_write_log(log, "Test log err", LOG_ERR) == 0);

	tb_destroy_log(log);

	log = tb_create_flog("flogfile", DATE);

	assert(log != NULL);
	assert(tb_write_log(log, "Test log info", LOG_INFO) == 0);
	assert(tb_write_log(log, "Test log ack", LOG_ACK) == 0);
	assert(tb_write_log(log, "Test log err", LOG_ERR) == 0);

	return 0;
}
