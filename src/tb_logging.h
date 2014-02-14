/*
 * tb_logging.h
 *
 *  Created on: 28/01/2014
 *      Author: michael
 */

#ifndef TB_LOGGING_H_
#define TB_LOGGING_H_

#include <stdio.h>

#define L_INFO "[ INF ]"
#define L_ACK  "[ ACK ]"
#define L_ERR  "[ ERR ]"

typedef struct
{
	FILE *file;
	char *file_path;
	int file_len;
}
tb_log_t;

typedef enum
{
	LOG_INFO = 0,
	LOG_ACK,
	LOG_ERR
}
tb_log_type_t;

typedef enum
{
	PLAIN = 0,
	DATE,
	TIME
}
tb_log_format_t;

/**
 * @brief Create a log file, using a plain name.
 */
tb_log_t
*tb_create_log(char *file_path);

/**
 * @brief Creat a log file, adding formatting to the log
 * file name.
 */
tb_log_t
*tb_create_flog(char *file_path, tb_log_format_t format);

/**
 * @brief Destroy the log struct.
 *
 * Frees memory for the tb_log_t struct.
 */
void
tb_destroy_log(tb_log_t *log);

/**
 * @brief Write a line to log file.
 *
 * Writes the supplied line to the specified log file, prepending
 * the type, date and time to this string, eg:
 *
 * [ INF ] [ Wed Jan 29 10:30:33 2014 ] Info
 */
int
tb_write_log(tb_log_t *log, char *info, tb_log_type_t type);

/**
 * @brief Get formatted time
 */
inline void
tb_get_f_time(char *time_str, size_t len, const char *format) __attribute__((always_inline));

/**
 * @brief Log an error with the associated errono
 */
inline void
tb_log_error_no(tb_log_t *log, int log_en, const char *info, int err_no) __attribute__((always_inline));

/**
 * @generic Log function.
 */
inline void
tb_log_info(tb_log_t *log, int log_en, const char *info, tb_log_type_t type) __attribute__ ((always_inline));
#endif /* TB_LOGGING_H_ */
