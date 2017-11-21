/*
 * Copyright (c) 2017, Xiaoye Meng
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "shfe.h"
#include "cffex.h"

/* FIXME */
static void usage(void) {
	fprintf(stderr, "Usage: ./test-recver <mc_ip> <mc_port>\n");
	exit(1);
}

int main(int argc, char **argv) {
	int sock, flags;
	struct sockaddr_in sa;
	struct ip_mreq imr;
	struct pollfd rfd[1];

	if (argc != 3)
		usage();
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		fprintf(stderr, "creating socket: %s", strerror(errno));
		return -1;
	}
	if ((flags = fcntl(sock, F_GETFL)) == -1) {
		fprintf(stderr, "fcntl(F_GETFL): %s", strerror(errno));
		return -1;
	}
	if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1) {
		fprintf(stderr, "fcntl(F_SETFL,O_NONBLOCK): %s", strerror(errno));
		return -1;
	}
	/* setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &val, sizeof val); */
	memset(&sa, '\0', sizeof sa);
	sa.sin_family      = AF_INET;
	sa.sin_port        = htons(atoi(argv[2]));
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sock, (struct sockaddr *)&sa, sizeof sa) == -1) {
		fprintf(stderr, "bind: %s", strerror(errno));
		close(sock);
		return -1;
	}
	imr.imr_multiaddr.s_addr = inet_addr(argv[1]);
	imr.imr_interface.s_addr = htonl(INADDR_ANY);
	if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &imr, sizeof imr) == -1) {
		fprintf(stderr, "setsockopt IP_ADD_MEMBERSHIP: %s", strerror(errno));
		close(sock);
		return -1;
	}
	rfd[0].fd     = sock;
	rfd[0].events = POLLIN;
	while (1) {
		struct timespec timeout = {0, 5};
		char buf[1024];
		socklen_t slen = sizeof sa;
		ssize_t nbytes;

		/* FIXME */
		ppoll(rfd, 1, &timeout, NULL);
		if (rfd[0].revents & POLLIN && (nbytes =
			recvfrom(sock, buf, sizeof buf, 0, (struct sockaddr *)&sa, &slen)) > 0) {
			if (nbytes == sizeof (struct quote_zj)) {
				struct quote_zj *quote = (struct quote_zj *)buf;

				fprintf(stdout, "%s,%s,%d,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,"
					"%d,%f,%f,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,"
					"%f,%d,%f,%d,%s,%s,%03d\n",
					quote->td_day,
					quote->sgid,
					quote->sid,
					quote->presettle,
					quote->preclose,
					quote->preopenint,
					quote->predelta,
					quote->open,
					quote->high,
					quote->low,
					quote->close,
					quote->upperlimit,
					quote->lowerlimit,
					quote->settle,
					quote->delta,
					quote->last,
					quote->volume,
					quote->turnover,
					quote->openint,
					quote->bid1,
					quote->bvol1,
					quote->ask1,
					quote->avol1,
					quote->bid2,
					quote->bvol2,
					quote->bid3,
					quote->bvol3,
					quote->ask2,
					quote->avol2,
					quote->ask3,
					quote->avol3,
					quote->bid4,
					quote->bvol4,
					quote->bid5,
					quote->bvol5,
					quote->ask4,
					quote->avol4,
					quote->ask5,
					quote->avol5,
					quote->instid,
					quote->time,
					quote->msec);
			} else {
				struct quote_sh *quote = (struct quote_sh *)buf;

				fprintf(stdout, "%s,%s,%s,%s,%f,%f,%f,%f,%f,%f,%f,%d,%f,%f,%f,%f,"
					"%f,%f,%f,%f,%s,%03d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,"
					"%f,%d,%f,%d,%f,%d,%f,%d,%f,%s\n",
					quote->td_day,
					quote->instid,
					quote->excid,
					quote->exc_instid,
					quote->last,
					quote->presettle,
					quote->preclose,
					quote->preopenint,
					quote->open,
					quote->high,
					quote->low,
					quote->volume,
					quote->turnover,
					quote->openint,
					quote->close,
					quote->settle,
					quote->upperlimit,
					quote->lowerlimit,
					quote->predelta,
					quote->delta,
					quote->time,
					quote->msec,
					quote->bid1,
					quote->bvol1,
					quote->ask1,
					quote->avol1,
					quote->bid2,
					quote->bvol2,
					quote->ask2,
					quote->avol2,
					quote->bid3,
					quote->bvol3,
					quote->ask3,
					quote->avol3,
					quote->bid4,
					quote->bvol4,
					quote->ask4,
					quote->avol4,
					quote->bid5,
					quote->bvol5,
					quote->ask5,
					quote->avol5,
					quote->average,
					quote->at_day);
			}
		}
	}
	return 0;
}

