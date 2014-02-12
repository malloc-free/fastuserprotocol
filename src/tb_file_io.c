/*
 * tb_file.c
 *
 *  Created on: 23/01/2014
 *      Author: michael
 */

#include "tb_file_io.h"
#include "tb_common.h"

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <libaio.h>

tb_file_t
*tb_create_file(char *file_path, tb_io_type type)
{
	tb_file_t *file = malloc(sizeof(tb_file_t));
	file->file_path = strdup(file_path);

	return file;
}

int
tb_destroy_file(tb_file_t *file)
{
	free(file->file_path);
	free(file);
	return 0;
}

void
tb_close_destroy(tb_file_t *file)
{
	close(file->f_desc);
	tb_destroy_file(file);
}

int
tb_get_file_length(int fd)
{
	int len;

#ifdef _DEBUG_IO
	PRT_INFO("Seeking end")
#endif

	len = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);

#ifdef _DEBUG_IO
	PRT_I_D("file length = %d", len)
#endif

	return len;
}

/**
 * @brief Setup mmap for io
 */
tb_file_t
*tb_setup_mmap(char *file_path, tb_o_flags flags, int file_len){
	tb_file_t *file =  tb_create_file(file_path, IO_MMAP);

	if((file->f_desc = open(file_path, flags)) == -1)
	{
		PRT_ERR("Could not open file");
		tb_destroy_file(file);

		return NULL;
	}


	if(flags == READ_WRITE_CREATE)
	{
		file->f_len = file_len;
	}
	else if((file->f_len = tb_get_file_length(file->f_desc)) == -1)
	{
		perror("Error: tb_get_file_length");
		tb_close_destroy(file);

		return NULL;
	}


	if(ftruncate(file->f_desc, file->f_len) == -1)
	{
		perror("Error: tb_setup_mmap: ftruncate");
		tb_close_destroy(file);

		return NULL;
	}

	if((file->mm_id = mmap(NULL, file->f_len, PROT_READ |
			PROT_WRITE, MAP_SHARED, file->f_desc, 0)) == NULL)
	{
		PRT_ERR("Error: tb_setup_mmap: "
				"Could not create memory map, closing file");
		close(file->f_desc);
		tb_destroy_file(file);

		return NULL;
	}

	return file;
}

/**
 * @brief Write to a file using mmap.
 */
int
tb_write_mmap(tb_file_t *file, char *buff, int off, int len)
{
	memcpy(file->mm_id + off, buff, len);

	return 0;
}

/**
 * @brief Read a file using mmap.
 */
int
tb_read_mmap(tb_file_t *file, char *buff, int off, int len)
{
	memcpy(buff, file->mm_id + off, len);

	return 0;
}

/**
 * @brief close mmap.
 */
int
tb_close_mmap(tb_file_t *file)
{
	munmap(file->mm_id, file->f_len);
	tb_close_destroy(file);

	return 0;
}

/**
 * @brief Setup for synchronous io.
 */
tb_file_t
*tb_setup_s(char *file_path, tb_o_flags flags, int file_len)
{
	tb_file_t *file = tb_create_file(file_path, IO_SYNC);

	if((file->f_desc = open(file->file_path, flags)) == -1)
	{
		perror("Error, tb_setup_s, open");
		tb_close_destroy(file);

		return NULL;
	}

	if(flags == READ_WRITE_CREATE)
	{
		file->f_len = file_len;
	}
	else if((file_len = tb_get_file_length(file->f_desc)) == -1)
	{
		perror("Error, tb_setup_s, tb_get_file_length");
		tb_close_destroy(file);

		return NULL;
	}

	return file;
}

/**
 * @brief Close for synchronous io.
 */
int
tb_close_s(tb_file_t *file)
{
	tb_close_destroy(file);

	return 0;
}

/**
 * @brief Read a file synchronously.
 */
int
tb_read_s(tb_file_t *file, char *buf, int off, int len)
{
	if(lseek(file->f_desc, off, SEEK_SET) == -1)
	{
		perror("Error: tb_read_s: lseek");
		return -1;
	}

	if(read(file->f_desc, buf, len) == -1)
	{
		perror("Error: tb_read_s: read");
		return -1;
	}

	return 0;
}

/**
 * @brief Write to a file synchronously.
 */
int
tb_write_s(tb_file_t *file, char *buf, int off, int len)
{
	if(lseek(file->f_desc, off, SEEK_SET) == -1)
	{
		perror("Error: tb_write_s: lseek");
		return -1;
	}

	if(write(file->f_desc, buf, len) == -1)
	{
		perror("Error: tb_write_s: write");
		return -1;
	}

	return 0;
}

tb_file_t
*tb_setup_buffs(char *file_path, tb_o_flags flags, int file_len)
{
#ifdef _DEBUG_IO
	PRT_INFO("Setup buffs")
#endif

	tb_file_t *file = tb_create_file(file_path, IO_BUFF_SYNC);

	PRT_I_S("Open file", file_path);

	if((file->b_f_desc = fopen(file_path, "r+")) == NULL)
	{
		if(flags == READ_WRITE_CREATE)
		{
			file->b_f_desc = fopen(file_path, "w+");
		}

		if(file->b_f_desc == NULL)
		{
			perror("Error: tb_setup_buffs: fopen");
			return NULL;
		}
	}

#ifdef _DEBUG_IO
	PRT_INFO("Check file size")
#endif

	if(flags == READ_WRITE_CREATE)
	{
		file->f_len = file_len;
	}
	else if((file->f_len = fseek(file->b_f_desc, 0, SEEK_END)))
	{
		perror("Error, tb_setup_s, tb_get_file_length");
		fclose(file->b_f_desc);
		free(file->b_f_desc);

		tb_destroy_file(file);

		return NULL;
	}

#ifdef _DEBUG_IO
	PRT_INFO("Seek start")
#endif

	fseek(file->b_f_desc, 0, SEEK_SET);

	return file;
}

int
tb_write_buffs(tb_file_t *file, char *buf, int off, int len)
{
	fseek(file->b_f_desc, off, SEEK_SET);

	if(fwrite(buf, 1, len, file->b_f_desc) == EOF)
	{
		perror("Error: tb_wrie_buffs: fwrite");
		return -1;
	}

	return 0;
}

int
tb_read_buffs(tb_file_t *file, char *buf, int off, int len)
{
	fseek(file->b_f_desc, off, SEEK_SET);

	if(fread(buf, 1, len, file->b_f_desc) == EOF)
	{
		perror("Error: tb_read_buffs: fread");
		return -1;
	}

	return 0;
}

int
tb_close_buffs(tb_file_t *file)
{
	fclose(file->b_f_desc);
	tb_destroy_file(file);

	return 0;
}

/**
 * @brief Setup asynchronous io
 */
tb_file_t
*tb_setup_as(char *file_path)
{

	tb_file_t *file = tb_create_file(file_path, IO_ASYNC);
//
//	file->ctxp = malloc(sizeof(io_context_t));
//
//	if(io_setup(64, file->ctxp) == -1)
//	{
//		perror("Cannot setup aio");
//
//		return NULL;
//	}

	return file;
}

/**
 * @brief Read a file asynchronously.
 */
int
tb_read_as(tb_file_t *file)
{
	return 0;
}

/**
 * @brief Write to a file asynchronously.
 */
int
tb_write_as(tb_file_t *file)
{
	return 0;
}

/**
 * @brief Close asynchronous io.
 */
int
tb_close_as(tb_file_t *file)
{
	tb_destroy_file(file);

	return 0;
}




