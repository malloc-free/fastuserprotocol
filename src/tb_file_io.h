/*
 * tb_file_io.h
 *
 *  Created on: 23/01/2014
 *      Author: michael
 */

#ifndef TB_FILE_IO_H_
#define TB_FILE_IO_H_

#include <stdio.h>
#include <fcntl.h>
#include <libaio.h>

/**
 * @typdef
 * @enum
 *
 * The type of io we will be performing.
 */
typedef enum
{
	IO_SYNC = 0,
	IO_BUFF_SYNC,
	IO_ASYNC,
	IO_MMAP,
}
tb_io_type;

/**
 * @typdef
 * @enum
 *
 * The flags for the file being opened. If we are
 * saving a new file from the network, then create
 * if it does not exist.
 */
typedef enum
{
	READ_WRITE_ONLY = O_RDWR,		//< Open the file, fail if not found
	READ_WRITE_CREATE = O_RDWR | O_CREAT  //< Open the file, create if not existing
}
tb_o_flags;


/**
 * @brief The struct that contains information about the file.
 *
 * TODO: Must make tb_file_t opaque, and hide all of the different
 * details in a pointer, or cast it to something else.
 */

typedef struct
{
	char *file_path;	//< The path to the file.
	tb_io_type type;	//< The type of io used on this file.
	int f_desc;			//< The file descriptor.

	//memmap stuff
	void *mm_id;
	int f_len;

	//AIO stuff
	io_context_t *ctxp;
	int max_events;

	//Buffered sync I/O stuff
	FILE *b_f_desc;
}
tb_file_t;

/**
 * *@brief Create tb_file_t
 */
static inline tb_file_t
*tb_create_file(char *file_path, tb_io_type type) __attribute__ ((__always_inline__));

/**
 * @brief Destroy tb_file_t
 */
static inline int
tb_destroy_file(tb_file_t *file) __attribute__ ((__always_inline__));

static inline void
tb_close_destroy(tb_file_t *file) __attribute__((__always_inline__));

static inline int
tb_get_file_length(int file_len) __attribute__((__always_inline__));

/**
 * @brief Setup mmap for io
 */
tb_file_t
*tb_setup_mmap(char *file_path, tb_o_flags flags, int file_len);

/**
 * @brief Write to a file using mmap.
 */
int
tb_write_mmap(tb_file_t *file, char *buff, int off, int len);

/**
 * @brief Read a file using mmap.
 */
int
tb_read_mmap(tb_file_t *file, char *buf, int off, int len);

/**
 * @brief close mmap.
 */
int
tb_close_mmap(tb_file_t *file);

/**
 * @brief Setup for synchronous io.
 */
tb_file_t
*tb_setup_s(char *file_path, tb_o_flags flags, int file_len);

/**
 * @brief Read a file synchronously.
 */
int
tb_read_s(tb_file_t *file, char *buf, int off, int len);

/**
 * @brief Write to a file synchronously.
 */
int
tb_write_s(tb_file_t *file, char *buf, int off, int len);

/**
 * @brief Close for synchronous io.
 */
int
tb_close_s(tb_file_t *file);

tb_file_t
*tb_setup_buffs(char *file_path, tb_o_flags flags, int file_len);

int
tb_read_buffs(tb_file_t *file, char *buf, int off, int len);

int
tb_write_buffs(tb_file_t *file, char *buf, int off, int len);

int
tb_close_buffs(tb_file_t *file);

/**
 * @brief Setup asynchronous io
 */
tb_file_t
*tb_setup_as(char *file_path);

/**
 * @brief Read a file asynchronously.
 */
int
tb_read_as(tb_file_t *file);

/**
 * @brief Write to a file asynchronously.
 */
int
tb_write_as(tb_file_t *file);

/**
 * @brief Close asynchronous io.
 */
int
tb_close_as(tb_file_t *file);


#endif /* TB_FILE_IO_H_ */
