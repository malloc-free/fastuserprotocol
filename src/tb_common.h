/*
 * tb_common.h
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

/**
 * @brief Print the given address to stdout.
 *
 * Prints the provided address to screen in human readable form.
 *
 * @param store The sockaddr_storage to get the address from.
 */
inline void
tb_print_address(struct sockaddr_storage *store) __attribute__((always_inline));

/**
 * @brief Get the address as a string from a sockaddr_storage struct.
 *
 * Get a human readable string from a sockaddr_storage struct.
 */
inline char
*tb_get_address(struct sockaddr_storage *store) __attribute__((always_inline));


//////////////////// Timer Functions /////////////////////////////

/**
 * @struct <tb_time_t> [tb_common.h]
 *
 * @brief A struct to hold start, finish and elapsed times.
 *
 * Used in the TestBed to record times for connection and transfer of data.
 */
typedef struct
{
	clockid_t clk_id;			///< Id of the clock to use.
	struct timespec *start;  	///< Start time.
	struct timespec *finish; 	///< Finish time.
	int started;				///< Timing has started.
	int stopped;				///< Timing has stopped;
	long long n_sec;			///< Total time.
}
tb_time_t;

/**
 * @brief Create a time struct.
 *
 * Allocate memory for a tb_time_t struct.
 *
 * @param clk_id The id of the type of clock to use. Please see time.h.
 */
tb_time_t
*tb_create_time(clockid_t clk_id);

/**
 * @brief Destroy a tb_time_t struct.
 *
 * Frees memory used by a tb_time_t struct.
 *
 * @param time The struct to destroy/free.
 */
void
tb_destroy_time(tb_time_t *time);

/**
 * @brief Record the start time.
 *
 * Sets the start time, and also sets the struct to started.
 *
 * @param time The tb_time_t struct to start timing for.
 */
inline void
tb_start_time(tb_time_t *time) __attribute__((always_inline));

/**
 * @brief Record the finish time.
 *
 * Sets the finish time, and also sets the struct to stopped. Calls tb_calculate_time.
 *
 * @param time The tb_time_t struct to stop timing for.
 */
inline void
tb_finish_time(tb_time_t *time) __attribute__((always_inline));

/**
 * @brief Calculate the time difference between start and stop.
 *
 * Calculates the elapsed time between start and finish, and records this in the
 * n_sec field. Also called by tb_finish_time when the timer is stopped.
 *
 * @param time The time to calculate for.
 */
inline void
tb_calculate_time(tb_time_t *time) __attribute__((always_inline));

//////////////////// File Handling Functions /////////////////////

/**
 * @brief Load the file to be used for sending using the cilent.
 *
 * Creates a test file that will be used by the TB for testing.
 *
 * @param file_name The name of the file to use.
 * @param file_size A pointer to which the size of the file can be recorded.
 */
char
*tb_create_test_file(char *file_name, int *file_size);

/**
 * @brief Load file to be used in tests.
 *
 * @param file_name The name of the file to use
 * @param file_size A pointer to which the size of the file can be recorded.
 */
char
*tb_load_test_file(char *file_name, int *file_size);

/**
 * @brief Load random file of specified size
 *
 * Loads a pre-generated file of the specified size,
 * or generates it if it does not exist.
 *
 * @param size The size of the random file to generate.
 */
char
*tb_load_random_file(int size);

/**
 * @brief Create a random file.
 *
 * Create a random file of the specified size, and save it to the specified path.
 *
 * @param path Path to the location where the path should be saved.
 * @param size The size of the file to create.
 */
char
*tb_create_random(char *path, int size);

#endif /* TB_COMMON_H_ */
