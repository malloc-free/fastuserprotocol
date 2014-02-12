/*
 * io_test.c
 *
 *  Created on: 23/01/2014
 *      Author: michael
 */

#include "../src/tb_file_io.h"
#include "../src/tb_common.h"

#include <assert.h>
#include <stdio.h>

void
test_bs(char *path);

void
test_mm(char *path);

void
test_s(char *path);

int
main(void)
{
	PRT_INFO("Testing mmap");
	test_mm("hello.txt");

	PRT_INFO("Testing sync I/O");
	test_s("hello.txt");

	PRT_INFO("Testing buffered sync I/O");
	test_bs("hello.txt");

	return 0;
}

void
test_bs(char *path)
{
	tb_file_t *file = tb_setup_buffs(path,
			READ_WRITE_ONLY, 0);

	assert(file != NULL);

	char buff[10];
	tb_read_buffs(file, buff, 0, 1);

	buff[1] = '\0';

	PRT_I_S("output = ", buff);

	buff[0] = 'b';

	tb_write_buffs(file, buff, 0, 1);

	fflush(file->b_f_desc);

	tb_close_buffs(file);
}

void
test_mm(char *path)
{
	tb_file_t *file = tb_setup_mmap(path,
				READ_WRITE_ONLY, 0);

	assert(file != NULL);

	PRT_INFO("Writing")
	tb_write_mmap(file, "w", 0, 1);

	char buff[10];

	PRT_INFO("Reading")
	tb_read_mmap(file, buff, 0, 1);

	buff[1] = '\0';

	PRT_I_S("output = ", buff);

	tb_close_mmap(file);

}

void
test_s(char *path)
{
	tb_file_t *file = tb_setup_s(path,
			READ_WRITE_ONLY, 0);

	assert(file != NULL);

	PRT_INFO("Reading")
	char buff[10];

	tb_read_s(file, buff, 0, 1);

	buff[1] = '\0';

	PRT_I_S("output = ", buff);

	tb_close_s(file);
}
