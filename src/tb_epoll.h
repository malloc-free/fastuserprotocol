/*
 * tb_epoll.h
 *
 *  Created on: 9/01/2014
 *      Author: Michael Holmwood
 *
 *      Used to manage epoll functions. Provides a callback system for
 *      when events occur. Each callback can be specific to the
 *      socket that has events waiting on it.
 */

#ifndef TB_EPOLL_H_
#define TB_EPOLL_H_

#include <sys/epoll.h>

/**
 * @brief Callback for events on the socket.
 *
 * This function is called when an event occurs for the
 * given fd.
 *
 */
typedef int (*func_event)(int events, void *data);

/**
 * @struct <tb_epoll_t> [tb_epoll.h]
 *
 * This struct contains the data used for epoll management.
 */
typedef struct
{
	int e_id;						///< ID for this epoll. Assigned by the OS.
	int w_time;						///< Wait time-out.
	int max_events;					///< Maximum number of events to poll for.
	int sock_d;						///< Socket descriptor for this epoll.
	struct epoll_event *grp_events;	///< Events to poll for.
	struct epoll_event *events;		///< Specific events for this descriptor.
	func_event f_event;				///< Callback for when events occur.
}
tb_epoll_t;

/**
 * @struct <tb_e_data> [tb_epoll.h]
 *
 * Allows for multiple data types to be returned by epoll_wait and events.
 * With epoll, a Union is used to store user data, and this data can be
 * either a void* or a socket descriptor. We want both, so this struct
 * is stored in the void*.
 */
typedef struct
{
	int fd;
	void *data;
}
tb_e_data;

/**
 * The default events used if none are supplied at the time that the
 * tb_epoll_t is created.
 */
const int event_defaults;

/**
 * @brief Create an epoll socket.
 *
 * Allocates the required amount of data for a tb_epoll_t struct,
 * and creates an epoll instance using the supplied socket descriptor.
 *
 * @param sock_d The socket to watch for events.
 * @param max_events The maximum number of events to wait for on a socket.
 * @param grp_events The events to wait for on all sockets in this epoll.
 * @param data The user data stored with the epoll.
 * @return An epoll struct with all of the required data.
 */
tb_epoll_t
*tb_create_e_poll(int sock_d, int max_events, int grp_events,
		void *data);

/**
 * @brief Free memory used by an instance of tb_epoll_t.
 *
 * Frees the memory for tb_epoll_t and its associated fields.
 *
 * @param e_poll The tb_epoll_t struct to destroy.
 */
void
tb_destroy_epoll(tb_epoll_t *e_poll);

/**
 * @brief Add a socket to an existing epoll instance.
 *
 * Adds a new socket to an existing epoll instance. This socket
 * is set to non-blocking, and is watched when tb_poll_for_events
 * is called with the supplied struct.
 *
 * @param e_poll The epoll instance to add the socket to.
 * @param sock_d The socket descriptor to add to the instance of epoll.
 */
int
tb_add_socket(tb_epoll_t *e_poll, int sock_d);

/**
 * @brief Watch sockets associated with the supplied instance of epoll.
 *
 * Polls for events associated with the supplied instance of epoll.
 * When events occur, the supplied callback function is called, and
 * the user data supplied in the struct is used.
 *
 * @param e_poll The instance of epoll to watch for.
 */
int
tb_poll_for_events(tb_epoll_t *e_poll);

#endif /* TB_EPOLL_H_ */
