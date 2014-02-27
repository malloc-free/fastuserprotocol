/*
 * tb_logging.h
 *
 *  Created on: 28/01/2014
 *      Author: michael
 *
 *      Performs all of the logging functions for TestBed. Can create, destroy
 *      tb_log_t structs, open and close files.
 */

#ifndef TB_LOGGING_H_
#define TB_LOGGING_H_

//External includes.
#include <stdio.h>

//macros for writing log info.
#define L_INFO "[ INF ]"
#define L_ACK  "[ ACK ]"
#define L_ERR  "[ ERR ]"

/**
 * @struct <tb_log_t> [tb_logging.h]
 *
 * Carries all of the data required to log information in the TestBed.
 */
typedef struct
{
	FILE *file;			///< The file descriptor for the log file.
	char *file_path;	///< The path, as a string, for the log file.
	int file_len;		///< The length of the file.
}
tb_log_t;

/**
 * @enum <tb_log_type_t> [tb_logging.h]
 *
 * Signifies the type of data to log.
 */
typedef enum
{
	LOG_INFO = 0,	///< Log info.
	LOG_ACK,		///< Log acknowledgment.
	LOG_ERR			///< Log error.
}
tb_log_type_t;

/**
 * @enum <tb_log_format_t> [tb_logging.h]
 *
 * Specifies the format to use for the name of the log file. Each log file
 * is prepended with log_, followed by either client or server, depending on
 * the type of TestBed being run. The either the date or time can be appended
 * to this.
 */
typedef enum
{
	PLAIN = 0,	///< plain log file.
	DATE,		///< Log file to have current date.
	TIME		///< Log file to have current time.
}
tb_log_format_t;

/**
 * @brief Create a log file, using a plain name.
 *
 * Just uses a basic log file, with no date or time attached.
 *
 * @param file_path A string with the path to where the log will be written.
 *
 * @return A pointer to the newly created tb_log_t struct, or NULL on error.
 */
tb_log_t
*tb_create_log(char *file_path);

/**
 * @brief Create a log file, adding formatting to the log.
 *
 * Creates a log file, with formatting. As explained in the tb_log_format_t
 * comment, this will either be plain, with date added, and with time added.
 *
 * @param file_path Path to where the log file should be written.
 * @param tb_log_format_t The format to use when creating the log file.
 *
 * @return A pointer to the newly created tb_log_t struct, or NULL on error.
 *
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
 * @brief Get formatted time string.
 *
 * Appends the current time to a string.
 *
 * @param time_str The string to append the time to.
 * @param len The length of time_str.
 * @param format The format to use for the time portion of the string.
 */
inline void
tb_get_f_time(char *time_str, size_t len, const char *format) __attribute__((always_inline));

/**
 * @brief Log an error with the associated errono
 *
 * Print an error, and append the description obtained from the error_no if logging
 * is enabled.
 *
 * @param log The log info to use when writing the file.
 * @param log_en 1 if the information is to be saved to log file.
 * @param info The information to log.
 * @param err_no The errno to log.
 *
 */
inline void
tb_log_error_no(tb_log_t *log, int log_en, const char *info, int err_no) __attribute__((always_inline));

/**
 * @generic Log function.
 *
 * Print information to screen, and log if logging is enabled.
 *
 * @param log The log struct with logging data.
 * @param log_en 1 if logging enabled, 0 if not.
 * @param info The info to log.
 * @param type The type of info to log.
 */
inline void
tb_log_info(tb_log_t *log, int log_en, const char *info, tb_log_type_t type) __attribute__ ((always_inline));
#endif /* TB_LOGGING_H_ */
