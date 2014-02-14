/*
 * tb_macro.h
 *
 *  Created on: 9/01/2014
 *      Author: michael
 */

#ifndef TB_COMMON_H_
#define TB_COMMON_H_

#include <sys/socket.h>
#include <time.h>

#define BLACK "\x1b[22;30m"
#define RED "\x1b[22;31m"
#define GREEN "\x1b[22;32m"
#define BLUE "\x1b[22;34m"
#define RESET "\x1b[39m"
#define CLEAR "\x1b[2J"
#define LINE "\n===================================\n"
#define MOVE_CSR(n, m) "\x1b[" #n ";" #m "H"
#define ACK(str) GREEN str RESET
#define ERR(str) RED str RESET
#define INFO(str) BLUE str RESET
#define PRT_ERR(str) fprintf(stderr, ERR(str) "\n")
#define PRT_ACK(str) fprintf(stdout, ACK(str) "\n")
#define PRT_INFO(str) fprintf(stdout, INFO(str) "\n")
#define PRT_ERR_PARAM(str, mod, param) fprintf(stderr, ACK(str)mod, param)
#define PRT_I_D(str, num) fprintf(stdout, INFO(str) "\n", num)
#define PRT_I_S(str, s) fprintf(stdout, INFO(str) ":%s\n", s)
#define PRT_E_S(str, s) fprintf(stdout, ERR(str) ":%s", s)

#define LOG(l, i, t) if(l->log_enabled) tb_write_log(l->log_info, i, t)
#define LOG_ADD(l, i, s) if(l->log_enabled) tb_address(l, i, s)
#define LOG_E_NO(l, str, eno) tb_log_error_no(l->log_info, l->log_enabled, str, eno)
#define LOG_INFO(l, i) tb_log_info(l->log_info, l->log_enabled, i, LOG_INFO)
#define LOG_S_E_NO(s, str, eno) tb_log_session_info(s, str, LOG_ERR, eno)
#define LOG_S_INFO(s, str) tb_log_session_info(s, str, LOG_INFO, 0)

//////////////////// Network Functions /////////////////////////////

inline void
tb_print_address(struct sockaddr_storage *store) __attribute__((always_inline));

inline char
*tb_get_address(struct sockaddr_storage *store) __attribute__((always_inline));


//////////////////// Timer Functions /////////////////////////////

/**
 * @brief A struct to hold start, stop and elapsed times.
 */
typedef struct
{
	clockid_t clk_id;
	struct timespec *start;
	struct timespec *finish;

	long long n_sec;
}
tb_time_t;

/**
 * @brief Create a time struct.
 *
 * @param clk_id The id of the type of clock to use.
 */
tb_time_t
*tb_create_time(clockid_t clk_id);

/**
 * @brief Destroy a tb_time_t struct.
 */
void
tb_destroy_time(tb_time_t *time);

/**
 * @brief Record the start time.
 */
inline void
tb_start_time(tb_time_t *time) __attribute__((always_inline));

/**
 * @brief Record the finish time.
 */
inline void
tb_finish_time(tb_time_t *time) __attribute__((always_inline));

/**
 * @brief Calculate the time difference between start and stop.
 */
inline void
tb_calculate_time(tb_time_t *time) __attribute__((always_inline));

//////////////////// File Handling Functions /////////////////////

/**
 *	@brief Load the file to be used for sending using the cilent.
 *
 *	@param listener The listener to create the file for.
 */
char
*tb_create_test_file(char *file_name, int *file_size);

/**
 * @brief Load file.
 */
char
*tb_load_test_file(char *file_name, int *file_size);

/**
 * @brief Load random file of specified size
 *
 * Loads a pre-generated file of the specified size,
 * or generates it if it does not exist.
 */
char
*tb_load_random_file(int size);

/**
 * @brief Create a random file.
 */
char
*tb_create_random(char *path, int size);

#endif /* TB_COMMON_H_ */
