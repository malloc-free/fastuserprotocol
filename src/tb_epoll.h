/*
 * tb_epoll.h
 *
 *  Created on: 9/01/2014
 *      Author: michael
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

typedef struct
{
	int e_id;
	int w_time;
	int max_events;
	int sock_d;
	struct epoll_event *grp_events;
	struct epoll_event *events;
	func_event f_event;
}
tb_epoll_t;

typedef struct
{
	int fd;
	void *data;
}
tb_e_data;

const int event_defaults;

tb_epoll_t
*tb_create_e_poll(int sock_d, int max_events, int grp_events,
		void *data);

void
tb_destroy_epoll(tb_epoll_t *e_poll);

int
tb_add_socket(tb_epoll_t *e_poll, int sock_d);

int
tb_poll_for_events(tb_epoll_t *e_poll);

#endif /* TB_EPOLL_H_ */
