/**
 * Copyright (c) 2010 William Light <wrl@illest.net>
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#define _GNU_SOURCE /* for asprintf */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>

#include <lo/lo.h>
#include <monome.h>

#include "serialosc.h"
#include "osc.h"


char *osc_path(const char *path, const char *prefix) {
	char *buf;

	if( asprintf(&buf, "%s/%s", prefix, path) < 0 ) {
		fprintf(stderr, "aieee, could not allocate memory in "
				"osc_path(), bailing out!\n");
		_exit(EXIT_FAILURE);
	}

	return buf;
}

int osc_event_loop(const sosc_state_t *state) {
	struct pollfd fds[2];

	fds[0].fd = monome_get_fd(state->monome);
	fds[1].fd = lo_server_get_socket_fd(state->server);

	fds[0].events = POLLIN | POLLOUT;
	fds[1].events = POLLIN;

	do {
		/* block until either the monome or liblo have data */
		if( poll(fds, 2, -1) < 0 )
			switch( errno ) {
			case EINVAL:
				perror("error in poll()");
				return 1;

			case EINTR:
			case EAGAIN:
				continue;
			}

		/* is the monome still connected? */
		if( fds[0].revents & (POLLHUP | POLLERR) )
			return 1;

		/* is there data available for reading from the monome? */
		if( fds[0].revents & POLLIN )
			monome_event_handle_next(state->monome);

		/* how about from OSC? */
		if( fds[1].revents & POLLIN && fds[0].revents & POLLOUT )
			lo_server_recv_noblock(state->server, 0);

	} while( 1 );
}
