/*
 * tb_testbed.h
 *
 *  Created on: 9/12/2013
 *      Author: Michael Holmwood
 */

#ifndef TESTBED_H_
#define TESTBED_H_

//Module imports.
#include "tb_protocol.h"
#include "tb_listener.h"
#include "tb_session.h"

/**
 * @enum <INPUT> [tb_testbed.h]
 *
 * @brief Defines the positions for the command line parameters.
 */
typedef enum
{
	TYPE = 1, ///< The type of endpoint to create (server, client)
	ADDRESS,  ///< The address to bind/connect to.
	PORT,     ///< The port to bind/connect to.
	PROT,     ///< The protocol to use.
	BUF_SIZE, ///< The size of the application buffer to use.
	FILE_TYPE,///< The type of file to use (disk file, random RAM file)
	FILE_NAME ///< The name of the file or the size of the random file
			  ///< To use.
}
INPUT;


///////// Declarations for TestBed API functions. /////////

extern tb_listener_t
*tb_create_listener(ENDPOINT_TYPE type, char *addr,
		char *port, PROTOCOL protocol, int bufsize);

extern tb_listener_t
*tb_create_endpoint(tb_test_params_t *params);

extern tb_prot_stats_t
*tb_get_protocol_stats(tb_listener_t *listener);

extern tb_prot_stats_t
*tb_ex_get_stats(tb_listener_t *listener);

/**
 * @brief Parse input from a number of strings.
 *
 * Parses values from strings, to create a server or client
 *
 * @param argc The number of input strings.
 * @param argv The strings.
 * @return 0 if the setup was successful.
 */
int
tb_parse(int argc, char *argv[]);

/**
 * @brief Starts the server/client
 *
 * This begins the server or client. Filename can be null if
 * the endpoint is a server.
 *
 * @param listener The listener to start.
 */
void
tb_start(tb_listener_t *listener);

/**
 * @brief actively monitor the server/client connection
 *
 * Monitors and updates the stats struct in the listener.
 *
 * @param listener The listener data to run.
 */
int
tb_monitor(tb_listener_t *listener);

/**
 * @brief Print the stats given.
 *
 * Print the stats for the given listener and stats structs.
 *
 * @param stats The stats struct to print.
 * @param listener The listener to print stats for.
 */
void
tb_print_stats(tb_prot_stats_t *stats, tb_listener_t *listener);

/**
 * @brief Handle system interrupts.
 *
 * Handles system interrupts, and kills the client/server if
 * one is received. Not currently implemented.
 *
 * @param value Value passed by the signal.
 */
void
tb_interrupt_handler(int value);

/**
 * @brief Aborts program.
 *
 * Aborts the program, closes the listener and underlying protocol,
 * kills any sockets or files still open. Closes any currently open
 * connections.
 *
 * @param listener The listener to close/destroy.
 */
void
tb_abort(void *data) __attribute__ ((__noreturn__));

/**
 * @brief Exits program.
 *
 * Exits the program, closes the listener and underlying protocol,
 * kills any sockes or files still open.
 *
 * @param listener The listener to close/destroy.
 */
void
tb_exit(void *data) __attribute__ ((__noreturn__));

#endif /* TESTBED_H_ */
