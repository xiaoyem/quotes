/*
 * Copyright (c) 2017, Gaohang Wu, Xiaoye Meng
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

#include <linux/inet.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/timer.h>
#include <net/sock.h>
#include <net/tcp.h>
#include "btree.h"
#include "shfe.h"

/* FIXME */
struct client {
	btree_t			btree;
	struct quote		*quote;
	struct socket		*msock;
	struct socket		*csock;
	struct task_struct	*task;
	struct timer_list	timer;
	u32			heartbeat;
	u32			dataready;
	u32			connected;
	u32			disconnected;
	u32			inpos;
	unsigned char		inbuf[64 * 1024 * 1024];
	unsigned char		debuf[8192];
};

/* FIXME */
static char *multicast_ip;
static int multicast_port;
static char *quote_ip;
static int quote_port;
static char *brokerid, *userid, *passwd, *contracts[2048];
static int count;
module_param(multicast_ip, charp, 0000);
module_param(multicast_port, int, 0000);
module_param(quote_ip,     charp, 0000);
module_param(quote_port,     int, 0000);
module_param(brokerid,     charp, 0000);
module_param(userid,       charp, 0000);
module_param(passwd,       charp, 0000);
module_param_array(contracts, charp, &count, 0000);
static struct client sh;

/* FIXME */
static int quotes_send(struct socket *sock, unsigned char *buf, int len) {
	struct msghdr msg = { .msg_flags = MSG_DONTWAIT | MSG_NOSIGNAL };
	struct kvec iov = { buf, len };

	return kernel_sendmsg(sock, &msg, &iov, 1, len);
}

/* FIXME */
static int quotes_recv(struct socket *sock, unsigned char *buf, int len) {
	struct msghdr msg = { .msg_flags = MSG_DONTWAIT | MSG_NOSIGNAL };
	struct kvec iov = { buf, len };

	return kernel_recvmsg(sock, &msg, &iov, 1, len, msg.msg_flags);
}

/* FIXME */
static inline void handle_double(double *x) {
	unsigned char *p;

	p = (unsigned char *)x;
	if (p[0] == 0x7f && p[1] == 0xef && p[2] == 0xff && p[3] == 0xff &&
		p[4] == 0xff && p[5] == 0xff && p[6] == 0xff && p[7] == 0xff)
		p[0] = p[1] = p[2] = p[3] = p[4] = p[5] = p[6] = p[7] = 0x00;
	else if (p[0] != 0x00 || p[1] != 0x00 || p[2] != 0x00 || p[3] != 0x00 ||
		p[4] != 0x00 || p[5] != 0x00 || p[6] != 0x00 || p[7] != 0x00) {
		/* courtesy of Yingzhi Zheng */
		long l = *((long *)x), *m = &l;

		l =     ((l & 0xff00000000000000) >> 56) |
			((l & 0x00ff000000000000) >> 40) |
			((l & 0x0000ff0000000000) >> 24) |
			((l & 0x000000ff00000000) >> 8 ) |
			((l & 0x00000000ff000000) << 8 ) |
			((l & 0x0000000000ff0000) << 24) |
			((l & 0x000000000000ff00) << 40) |
			((l & 0x00000000000000ff) << 56);
		*x = *((double *)m);
	}
}

/* FIXME */
static void print_buf(unsigned char *buf, int len) {
	int i;

	for (i = 0; i + 7 < len; i += 8)
		printk(KERN_INFO "[%s] <%d> %02x %02x %02x %02x %02x %02x %02x %02x\n",
			__func__, len,
			buf[i],
			buf[i + 1],
			buf[i + 2],
			buf[i + 3],
			buf[i + 4],
			buf[i + 5],
			buf[i + 6],
			buf[i + 7]);
	for (; i < len; ++i)
		printk(KERN_INFO "[%s] <%d> %02x\n", __func__, len, buf[i]);
}

/* FIXME */
static void send_hbtimeout(struct client *c) {
	struct hbtimeout to;

	to.ftd_type        = 0x00;
	to.ftd_extd_length = 0x06;
	to.ftd_cont_length = 0x0000;
	to.tag_type        = 0x07;
	to.tag_length      = 0x04;
	to.timeout         = 0x27000000;
	if (quotes_send(c->csock, (unsigned char *)&to, sizeof to) < 0)
		printk(KERN_ERR "[%s] send failed\n", __func__);
}

/* FIXME */
static inline void send_heartbeat(struct client *c) {
	struct heartbeat hb;

	hb.ftd_type        = 0x00;
	hb.ftd_extd_length = 0x02;
	hb.ftd_cont_length = 0x0000;
	hb.tag_type        = 0x05;
	hb.tag_length      = 0x00;
	if (quotes_send(c->csock, (unsigned char *)&hb, sizeof hb) < 0)
		printk(KERN_ERR "[%s] send failed\n", __func__);
}

static void login(struct client *c) {
	struct login lo;
	struct timeval time;
	struct tm tm;
	struct net *net = sock_net(c->csock->sk);
	struct net_device *dev;

	memset(&lo, '\0', sizeof lo);
	lo.header.ftd_type         = 0x02;
	lo.header.ftd_cont_length  = 0xea00;
	lo.header.version          = 0x01;
	lo.header.unenc_length     = 0x0b;
	lo.header.chain            = 0x4c;
	lo.header.seq_number       = 0x00300000;
	lo.header.fld_count        = 0x0300;
	lo.header.ftdc_cont_length = 0xd400;
	lo.header.rid              = 0x01000000;
	lo.type                    = 0x0210;
	lo.length                  = 0xbc00;
	do_gettimeofday(&time);
	time_to_tm(time.tv_sec, 0, &tm);
	snprintf(lo.td_day, sizeof lo.td_day, "%04ld%02d%02d",
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
	strncat(lo.brokerid, brokerid, sizeof lo.brokerid - 1);
	strncat(lo.userid,   userid,   sizeof lo.userid   - 1);
	strncat(lo.passwd,   passwd,   sizeof lo.passwd   - 1);
	/* FIXME */
	lo.ipi[0]                  = 'T';
	lo.ipi[1]                  = 'H';
	lo.ipi[2]                  = 'O';
	lo.ipi[3]                  = 'S';
	lo.ipi[4]                  = 'T';
	lo.ipi[5]                  = ' ';
	lo.ipi[6]                  = 'U';
	lo.ipi[7]                  = 's';
	lo.ipi[8]                  = 'e';
	lo.ipi[9]                  = 'r';
	lo.pi[0]                   = 'F';
	lo.pi[1]                   = 'T';
	lo.pi[2]                   = 'D';
	lo.pi[3]                   = 'C';
	lo.pi[4]                   = ' ';
	lo.pi[5]                   = '0';
	/* FIXME */
	for_each_netdev(net, dev)
		if (strcmp(dev->name, "lo")) {
			snprintf(lo.mac, sizeof lo.mac, "%02X:%02X:%02X:%02X:%02X:%02X",
				dev->dev_addr[0],
				dev->dev_addr[1],
				dev->dev_addr[2],
				dev->dev_addr[3],
				dev->dev_addr[4],
				dev->dev_addr[5]);
			break;
		}
	lo.type1                   = 0x0110;
	lo.length1                 = 0x0600;
	lo.seq_series1             = 0x0100;
	lo.type4                   = 0x0110;
	lo.length4                 = 0x0600;
	lo.seq_series4             = 0x0400;
	if (quotes_send(c->csock, (unsigned char *)&lo, sizeof lo) < 0)
		printk(KERN_ERR "[%s] send failed\n", __func__);
}

/* FIXME */
static void subscribe(struct client *c, const char *contract) {
	struct subscribe sb;

	memset(&sb, '\0', sizeof sb);
	sb.header.ftd_type         = 0x02;
	sb.header.ftd_cont_length  = 0x3900;
	sb.header.version          = 0x01;
	sb.header.unenc_length     = 0x0b;
	sb.header.chain            = 0x4c;
	sb.header.seq_number       = 0x01440000;
	sb.header.fld_count        = 0x0100;
	sb.header.ftdc_cont_length = 0x2300;
	sb.type                    = 0x4124;
	sb.length                  = 0x1f00;
	strncat(sb.instid, contract, sizeof sb.instid - 1);
	if (quotes_send(c->csock, (unsigned char *)&sb, sizeof sb) < 0)
		printk(KERN_ERR "[%s] send failed\n", __func__);
}

/* FIXME */
static inline void handle_12packet(struct client *c, struct quote *quote) {
	btree_node_t node;
	int i;

	if ((node = btree_find(c->btree, quote->instid, &i))) {
		c->quote = (struct quote *)btree_node_value(node, i);
		*c->quote = *quote;
	} else if ((c->quote = kzalloc(sizeof *c->quote, GFP_KERNEL))) {
		*c->quote = *quote;
		btree_insert(c->btree, c->quote->instid, c->quote);
	} else {
		printk(KERN_ERR "[%s] error locating quote for '%s'\n", __func__, quote->instid);
		return;
	}
	handle_double(&c->quote->last);
	handle_double(&c->quote->presettle);
	handle_double(&c->quote->preclose);
	handle_double(&c->quote->preopenint);
	handle_double(&c->quote->open);
	handle_double(&c->quote->high);
	handle_double(&c->quote->low);
	c->quote->volume = ntohl(c->quote->volume);
	handle_double(&c->quote->turnover);
	handle_double(&c->quote->openint);
	handle_double(&c->quote->close);
	handle_double(&c->quote->settle);
	handle_double(&c->quote->upperlimit);
	handle_double(&c->quote->lowerlimit);
	handle_double(&c->quote->predelta);
	handle_double(&c->quote->delta);
	c->quote->msec   = ntohl(c->quote->msec);
	handle_double(&c->quote->bid1);
	c->quote->bvol1  = ntohl(c->quote->bvol1);
	handle_double(&c->quote->ask1);
	c->quote->avol1  = ntohl(c->quote->avol1);
	handle_double(&c->quote->bid2);
	handle_double(&c->quote->ask2);
	handle_double(&c->quote->bid3);
	handle_double(&c->quote->ask3);
	handle_double(&c->quote->bid4);
	handle_double(&c->quote->ask4);
	handle_double(&c->quote->bid5);
	handle_double(&c->quote->ask5);
	handle_double(&c->quote->average);
	if (quotes_send(c->msock, (unsigned char *)c->quote, sizeof *c->quote) < 0)
		printk(KERN_ERR "[%s] send quote '%s,%s,%s,%s,%f,%f,%f,%f,%f,%f,%f,%d,%f,%f,%f,%f,"
			"%f,%f,%f,%f,%s,%03d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,"
			"%f,%d,%f,%d,%f,%s' failed\n",
			__func__,
			c->quote->td_day,
			c->quote->instid,
			c->quote->excid,
			c->quote->exc_instid,
			c->quote->last,
			c->quote->presettle,
			c->quote->preclose,
			c->quote->preopenint,
			c->quote->open,
			c->quote->high,
			c->quote->low,
			c->quote->volume,
			c->quote->turnover,
			c->quote->openint,
			c->quote->close,
			c->quote->settle,
			c->quote->upperlimit,
			c->quote->lowerlimit,
			c->quote->predelta,
			c->quote->delta,
			c->quote->time,
			c->quote->msec,
			c->quote->bid1,
			c->quote->bvol1,
			c->quote->ask1,
			c->quote->avol1,
			c->quote->bid2,
			c->quote->bvol2,
			c->quote->ask2,
			c->quote->avol2,
			c->quote->bid3,
			c->quote->bvol3,
			c->quote->ask3,
			c->quote->avol3,
			c->quote->bid4,
			c->quote->bvol4,
			c->quote->ask4,
			c->quote->avol4,
			c->quote->bid5,
			c->quote->bvol5,
			c->quote->ask5,
			c->quote->avol5,
			c->quote->average,
			c->quote->at_day);
}

static inline void handle_24packet(struct client *c, unsigned short *type, unsigned short *length) {
	switch (*type) {
	case 0x3924:
		{
			struct mdtime *mdtime = (struct mdtime *)((unsigned char *)type + 4);
			btree_node_t node;
			int i;

			if ((node = btree_find(c->btree, mdtime->instid, &i)))
				c->quote = (struct quote *)btree_node_value(node, i);
			else if ((c->quote = kzalloc(sizeof *c->quote, GFP_KERNEL))) {
				memcpy(c->quote->instid, mdtime->instid, sizeof c->quote->instid);
				btree_insert(c->btree, c->quote->instid, c->quote);
			} else {
				printk(KERN_ERR "[%s] error locating quote for '%s'\n",
					__func__, mdtime->instid);
				return;
			}
			if (strcmp(c->quote->time, mdtime->time))
				memcpy(c->quote->time, mdtime->time, sizeof c->quote->time);
			c->quote->msec       = ntohl(mdtime->msec);
			if (strcmp(c->quote->at_day, mdtime->at_day))
				memcpy(c->quote->at_day, mdtime->at_day, sizeof c->quote->at_day);
		}
		break;
	case 0x3424:
		{
			struct mdbest *mdbest = (struct mdbest *)((unsigned char *)type + 4);

			if (unlikely(c->quote == NULL)) {
				printk(KERN_ERR "[%s] unrecoverable error\n", __func__);
				return;
			}
			c->quote->bid1       = mdbest->bid1;
			handle_double(&c->quote->bid1);
			c->quote->bvol1      = ntohl(mdbest->bvol1);
			c->quote->ask1       = mdbest->ask1;
			handle_double(&c->quote->ask1);
			c->quote->avol1      = ntohl(mdbest->avol1);
		}
		break;
	case 0x3124:
		{
			struct mdbase *mdbase = (struct mdbase *)((unsigned char *)type + 4);

			if (unlikely(c->quote == NULL)) {
				printk(KERN_ERR "[%s] unrecoverable error\n", __func__);
				return;
			}
			if (strcmp(c->quote->td_day, mdbase->td_day))
				memcpy(c->quote->td_day, mdbase->td_day, sizeof c->quote->td_day);
			c->quote->presettle  = mdbase->presettle;
			handle_double(&c->quote->presettle);
			c->quote->preclose   = mdbase->preclose;
			handle_double(&c->quote->preclose);
			c->quote->preopenint = mdbase->preopenint;
			handle_double(&c->quote->preopenint);
			c->quote->predelta   = mdbase->predelta;
			handle_double(&c->quote->predelta);
		}
		break;
	case 0x3224:
		{
			struct mdstatic *mdstatic = (struct mdstatic *)((unsigned char *)type + 4);

			if (unlikely(c->quote == NULL)) {
				printk(KERN_ERR "[%s] unrecoverable error\n", __func__);
				return;
			}
			c->quote->open       = mdstatic->open;
			handle_double(&c->quote->open);
			c->quote->high       = mdstatic->high;
			handle_double(&c->quote->high);
			c->quote->low        = mdstatic->low;
			handle_double(&c->quote->low);
			c->quote->close      = mdstatic->close;
			handle_double(&c->quote->close);
			c->quote->upperlimit = mdstatic->upperlimit;
			handle_double(&c->quote->upperlimit);
			c->quote->lowerlimit = mdstatic->lowerlimit;
			handle_double(&c->quote->lowerlimit);
			c->quote->settle     = mdstatic->settle;
			handle_double(&c->quote->settle);
			c->quote->delta      = mdstatic->delta;
			handle_double(&c->quote->delta);
		}
		break;
	case 0x3324:
		{
			struct mdlast *mdlast = (struct mdlast *)((unsigned char *)type + 4);

			if (unlikely(c->quote == NULL)) {
				printk(KERN_ERR "[%s] unrecoverable error\n", __func__);
				return;
			}
			c->quote->last       = mdlast->last;
			handle_double(&c->quote->last);
			c->quote->volume     = ntohl(mdlast->volume);
			c->quote->turnover   = mdlast->turnover;
			handle_double(&c->quote->turnover);
			c->quote->openint    = mdlast->openint;
			handle_double(&c->quote->openint);
		}
		break;
	case 0x3524:
		{
			struct mdbid23 *mdbid23 = (struct mdbid23 *)((unsigned char *)type + 4);

			if (unlikely(c->quote == NULL)) {
				printk(KERN_ERR "[%s] unrecoverable error\n", __func__);
				return;
			}
			/* FIXME */
			c->quote->bid2       = mdbid23->bid2;
			handle_double(&c->quote->bid2);
			c->quote->bid3       = mdbid23->bid3;
			handle_double(&c->quote->bid3);
		}
		break;
	case 0x3624:
		{
			struct mdask23 *mdask23 = (struct mdask23 *)((unsigned char *)type + 4);

			if (unlikely(c->quote == NULL)) {
				printk(KERN_ERR "[%s] unrecoverable error\n", __func__);
				return;
			}
			/* FIXME */
			c->quote->ask2       = mdask23->ask2;
			handle_double(&c->quote->ask2);
			c->quote->ask3       = mdask23->ask3;
			handle_double(&c->quote->ask3);
		}
		break;
	case 0x3724:
		{
			struct mdbid45 *mdbid45 = (struct mdbid45 *)((unsigned char *)type + 4);

			if (unlikely(c->quote == NULL)) {
				printk(KERN_ERR "[%s] unrecoverable error\n", __func__);
				return;
			}
			/* FIXME */
			c->quote->bid4       = mdbid45->bid4;
			handle_double(&c->quote->bid4);
			c->quote->bid5       = mdbid45->bid5;
			handle_double(&c->quote->bid5);
		}
		break;
	case 0x3824:
		{
			struct mdask45 *mdask45 = (struct mdask45 *)((unsigned char *)type + 4);

			if (unlikely(c->quote == NULL)) {
				printk(KERN_ERR "[%s] unrecoverable error\n", __func__);
				return;
			}
			/* FIXME */
			c->quote->ask4       = mdask45->ask4;
			handle_double(&c->quote->ask4);
			c->quote->ask5       = mdask45->ask5;
			handle_double(&c->quote->ask5);
		}
		break;
	case 0x8124:
		{
			double *average = (double *)((unsigned char *)type + 4);

			if (unlikely(c->quote == NULL)) {
				printk(KERN_ERR "[%s] unrecoverable error\n", __func__);
				return;
			}
			c->quote->average    = *average;
			handle_double(&c->quote->average);
		}
		break;
	default:
		print_buf((unsigned char *)type, *length + 4);
		break;
	}
}

/* FIXME */
static void quotes_sock_data_ready(struct sock *sk, int unused) {
	struct client *c = (struct client *)sk->sk_user_data;

	/* printk(KERN_INFO "[%s] state = %d\n", __func__, sk->sk_state); */
	if (sk->sk_state != TCP_CLOSE && sk->sk_state != TCP_CLOSE_WAIT)
		atomic_inc((atomic_t *)&c->dataready);
}

/* FIXME */
static void quotes_sock_write_space(struct sock *sk) {
	printk(KERN_INFO "[%s] state = %d\n", __func__, sk->sk_state);
}

/* FIXME */
static void quotes_sock_state_change(struct sock *sk) {
	struct client *c = (struct client *)sk->sk_user_data;

	printk(KERN_INFO "[%s] state = %d\n", __func__, sk->sk_state);
	switch (sk->sk_state) {
	case TCP_CLOSE:
		printk(KERN_INFO "[%s] TCP_CLOSE\n", __func__);
	case TCP_CLOSE_WAIT:
		printk(KERN_INFO "[%s] TCP_CLOSE_WAIT\n", __func__);
		atomic_set((atomic_t *)&c->disconnected, 1);
		break;
	case TCP_ESTABLISHED:
		printk(KERN_INFO "[%s] TCP_ESTABLISHED\n", __func__);
		atomic_set((atomic_t *)&c->connected, 1);
		break;
	default:
		break;
	}
}

static void process_inbuf(struct client *c) {
	unsigned char *start = c->inbuf;

	while (c->inpos >= 4) {
		unsigned char  ftd_type     = start[0];
		unsigned char  ftd_extd_len = start[1];
		unsigned short ftd_cont_len = ntohs(*((unsigned short *)(start + 2)));

		/* packet is incomplete */
		if (unlikely(c->inpos < ftd_cont_len + 4))
			break;
		if (ftd_type == 0x02 && ftd_extd_len == 0x00 && ftd_cont_len > 0) {
			int i, j;
			struct shfeheader *sh = (struct shfeheader *)c->debuf;

			/* FIXME */
			c->debuf[0] = start[0];
			c->debuf[1] = start[1];
			c->debuf[2] = start[2];
			c->debuf[3] = start[3];
			c->debuf[4] = start[4];
			c->debuf[5] = start[5];
			c->debuf[6] = start[6];
			c->debuf[7] = start[7];
			for (i = 8, j = 8; i < ftd_cont_len + 4; ++i)
				if (start[i] == 0xe0)
					c->debuf[j++] = start[i++ + 1];
				else if (start[i] >= 0xe1 && start[i] <= 0xef) {
					int k, n = start[i] - 0xe0;

					for (k = 0; k < n; ++k)
						c->debuf[j++] = 0x00;
				} else
					c->debuf[j++] = start[i];
			/* print_buf(c->debuf, j); */
			switch (sh->seq_number) {
			case 0x01300000:
				{
					struct info *info = (struct info *)(sh->buf + 4);

					if (info->errid == 0)
						for (i = 0; i < count; ++i)
							if (strcmp(contracts[i], ""))
								subscribe(c, contracts[i]);
				}
				break;
			case 0x02440000:
				{
					struct info *info = (struct info *)(sh->buf + 4);
					char *contract = (char *)(sh->buf + 4 + sizeof *info + 4);

					if (info->errid != 0)
						printk(KERN_INFO "[%s] subscribe '%s' failed\n",
							__func__, contract);
				}
				break;
			case 0x01f10000:
				{
					unsigned short *type   = (unsigned short *)sh->buf;
					unsigned short *length = (unsigned short *)(sh->buf + 2);
					struct quote *quote    = (struct quote *)(sh->buf + 4);

					if (*type == 0x1200 && *length == 0x6201)
						handle_12packet(c, quote);
				}
				break;
			case 0x03f10000:
				{
					unsigned short count   = ntohs(sh->fld_count);
					unsigned short *type   = (unsigned short *)sh->buf;
					unsigned short *length = (unsigned short *)(sh->buf + 2);

					for (i = 0; i < count; ++i) {
						handle_24packet(c, type, length);
						type   = (unsigned short *)((unsigned char *)type +
							4 + ntohs(*length));
						length = (unsigned short *)((unsigned char *)type +
							2);
					}
					if (count > 0 && c->quote && quotes_send(c->msock,
						(unsigned char *)c->quote, sizeof *c->quote) < 0)
						printk(KERN_ERR "[%s] send quote '%s,%s,%s,%s,%f,"
							"%f,%f,%f,%f,%f,%f,%d,%f,%f,%f,%f,%f,%f,"
							"%f,%f,%s,%03d,%f,%d,%f,%d,%f,%d,%f,%d,"
							"%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%s'"
							" failed\n",
							__func__,
							c->quote->td_day,
							c->quote->instid,
							c->quote->excid,
							c->quote->exc_instid,
							c->quote->last,
							c->quote->presettle,
							c->quote->preclose,
							c->quote->preopenint,
							c->quote->open,
							c->quote->high,
							c->quote->low,
							c->quote->volume,
							c->quote->turnover,
							c->quote->openint,
							c->quote->close,
							c->quote->settle,
							c->quote->upperlimit,
							c->quote->lowerlimit,
							c->quote->predelta,
							c->quote->delta,
							c->quote->time,
							c->quote->msec,
							c->quote->bid1,
							c->quote->bvol1,
							c->quote->ask1,
							c->quote->avol1,
							c->quote->bid2,
							c->quote->bvol2,
							c->quote->ask2,
							c->quote->avol2,
							c->quote->bid3,
							c->quote->bvol3,
							c->quote->ask3,
							c->quote->avol3,
							c->quote->bid4,
							c->quote->bvol4,
							c->quote->ask4,
							c->quote->avol4,
							c->quote->bid5,
							c->quote->bvol5,
							c->quote->ask5,
							c->quote->avol5,
							c->quote->average,
							c->quote->at_day);
				}
				break;
			default:
				print_buf(c->debuf, j);
				break;
			}
		} else if (ftd_type == 0x00 && ftd_extd_len == 0x02) {
			/* printk(KERN_INFO "[%s] receiving heartbeat\n", __func__); */
			ftd_cont_len = ftd_extd_len;
		} else {
			printk(KERN_ERR "[%s] unknown packet type = 0x%02x, length = %d\n",
				__func__, ftd_type, ftd_cont_len);
			/* FIXME */
			c->inpos = 0;
			return;
		}
		start    += ftd_cont_len + 4;
		c->inpos -= ftd_cont_len + 4;
	}
	if (start != c->inbuf && c->inpos != 0)
		memmove(c->inbuf, start, c->inpos);
}

/* FIXME */
static void timer_func(unsigned long data) {
	struct client *c = (struct client *)data;

	atomic_set((atomic_t *)&c->heartbeat, 1);
	init_timer(&c->timer);
	c->timer.expires  = jiffies + 15 * HZ;
	c->timer.function = timer_func;
	c->timer.data     = (unsigned long)c;
	add_timer(&c->timer);
}

/* FIXME */
static void set_sock_callbacks(struct socket *sock, struct client *c) {
	struct sock *sk = sock->sk;

	sk->sk_user_data    = (void *)c;
	sk->sk_data_ready   = quotes_sock_data_ready;
	sk->sk_write_space  = quotes_sock_write_space;
	sk->sk_state_change = quotes_sock_state_change;
}

/* FIXME */
static void quote_free(void *value) {
	struct quote *quote = (struct quote *)value;

	kfree(quote);
}

/* FIXME */
static int quotes_connect(struct socket *sock, const char *ip, int port, int flags) {
	struct sockaddr_in sa;

	memset(&sa, '\0', sizeof sa);
	sa.sin_family      = AF_INET;
	sa.sin_addr.s_addr = in_aton(ip);
	sa.sin_port        = htons(port);
	return sock->ops->connect(sock, (struct sockaddr *)&sa, sizeof sa, flags);
}

static int quotes_thread(void *data) {
	struct client *c = (struct client *)data;

	while (!kthread_should_stop()) {
		if (atomic_read((atomic_t *)&c->heartbeat)) {
			/* printk(KERN_INFO "[%s] sending heartbeat\n", __func__); */
			send_heartbeat(c);
			atomic_set((atomic_t *)&c->heartbeat, 0);
		}
		if (atomic_read((atomic_t *)&c->dataready)) {
			int len;

			len = quotes_recv(c->csock, c->inbuf + c->inpos, sizeof c->inbuf - c->inpos);
			if (len > 0) {
				c->inpos += len;
				/* FIXME */
				if (c->inpos > sizeof c->inbuf) {
					printk(KERN_ERR "[%s] max input buffer length reached\n",
						__func__);
					c->inpos = 0;
				} else
					process_inbuf(c);
			}
			atomic_dec((atomic_t *)&c->dataready);
		}
		if (atomic_read((atomic_t *)&c->connected)) {
			init_timer(&c->timer);
			c->timer.expires  = jiffies + 15 * HZ;
			c->timer.function = timer_func;
			c->timer.data     = (unsigned long)c;
			add_timer(&c->timer);
			c->inpos = 0;
			printk(KERN_INFO "[%s] sending heartbeat timeout\n", __func__);
			send_hbtimeout(c);
			printk(KERN_INFO "[%s] logging in\n", __func__);
			login(c);
			atomic_set((atomic_t *)&c->connected, 0);
		}
		if (atomic_read((atomic_t *)&c->disconnected)) {
			int one = 1, val = 8 * 1024 * 1024, ret;

			if (timer_pending(&c->timer))
				del_timer(&c->timer);
			atomic_set((atomic_t *)&c->heartbeat, 0);

loop:
			/* FIXME */
			schedule_timeout_uninterruptible(15 * HZ);
			if (sock_create_kern(PF_INET, SOCK_STREAM, IPPROTO_TCP, &c->csock) < 0) {
				printk(KERN_ERR "[%s] error creating client socket\n", __func__);
				goto loop;
			}
			/* FIXME */
			c->csock->sk->sk_allocation = GFP_ATOMIC;
			set_sock_callbacks(c->csock, c);
			kernel_setsockopt(c->csock, SOL_TCP, TCP_NODELAY, (char *)&one, sizeof one);
			kernel_setsockopt(c->csock, SOL_SOCKET, SO_RCVBUF, (char *)&val, sizeof val);
			kernel_setsockopt(c->csock, SOL_SOCKET, SO_SNDBUF, (char *)&val, sizeof val);
			/* FIXME */
			if ((ret = quotes_connect(c->csock, quote_ip, quote_port, O_NONBLOCK))
				== -EINPROGRESS) {
			} else if (ret < 0) {
				printk(KERN_ERR "[%s] error reconnecting quote address\n", __func__);
				sock_release(c->csock);
				goto loop;
			}
			atomic_set((atomic_t *)&c->disconnected, 0);
		}
	}
	return 0;
}

static int __init quotes_init(void) {
	int val = 8 * 1024 * 1024, one = 1, ret;

	if (multicast_ip == NULL || !strcmp(multicast_ip, "") || multicast_port == 0 ||
		quote_ip == NULL || !strcmp(quote_ip, "") || quote_port == 0 || brokerid == NULL ||
		!strcmp(brokerid, "") || userid == NULL || !strcmp(userid, "") || passwd == NULL ||
		!strcmp(passwd, "") || count == 0 || (count == 1 && !strcmp(contracts[0], ""))) {
		printk(KERN_ERR "[%s] multicast_ip, multicast_port, quote_ip, quote_port, "
			"brokerid, userid, passwd or contracts can't be NULL\n", __func__);
		return -EINVAL;
	}
	if ((sh.btree = btree_new(256, NULL, NULL, quote_free)) == NULL) {
		printk(KERN_ERR "[%s] error allocating quote cache\n", __func__);
		return -ENOMEM;
	}
	if (sock_create_kern(PF_INET, SOCK_DGRAM, IPPROTO_UDP, &sh.msock) < 0) {
		printk(KERN_ERR "[%s] error creating multicast socket\n", __func__);
		return -EIO;
	}
	/* FIXME */
	kernel_setsockopt(sh.msock, SOL_SOCKET, SO_SNDBUF, (char *)&val, sizeof val);
	if (quotes_connect(sh.msock, multicast_ip, multicast_port, 0) < 0) {
		printk(KERN_ERR "[%s] error connecting multicast address\n", __func__);
		goto end;
	}
	if (sock_create_kern(PF_INET, SOCK_STREAM, IPPROTO_TCP, &sh.csock) < 0) {
		printk(KERN_ERR "[%s] error creating client socket\n", __func__);
		goto end;
	}
	/* FIXME */
	sh.csock->sk->sk_allocation = GFP_ATOMIC;
	set_sock_callbacks(sh.csock, &sh);
	kernel_setsockopt(sh.csock, SOL_TCP, TCP_NODELAY, (char *)&one, sizeof one);
	kernel_setsockopt(sh.csock, SOL_SOCKET, SO_RCVBUF, (char *)&val, sizeof val);
	kernel_setsockopt(sh.csock, SOL_SOCKET, SO_SNDBUF, (char *)&val, sizeof val);
	/* FIXME */
	if ((ret = quotes_connect(sh.csock, quote_ip, quote_port, O_NONBLOCK)) == -EINPROGRESS) {
	} else if (ret < 0) {
		printk(KERN_ERR "[%s] error connecting quote address\n", __func__);
		sock_release(sh.csock);
		goto end;
	}
	sh.task = kthread_create(quotes_thread, &sh, "quotes_sh");
	if (IS_ERR(sh.task)) {
		printk(KERN_ERR "[%s] error creating quotes thread\n", __func__);
		sock_release(sh.csock);
		goto end;
	}
	kthread_bind(sh.task, 1);
	wake_up_process(sh.task);
	return 0;

end:
	sock_release(sh.msock);
	return -EIO;
}

/* FIXME */
static void __exit quotes_exit(void) {
	if (timer_pending(&sh.timer))
		del_timer(&sh.timer);
	kthread_stop(sh.task);
	sock_release(sh.csock);
	sock_release(sh.msock);
	btree_free(&sh.btree);
}

module_init(quotes_init);
module_exit(quotes_exit);
MODULE_AUTHOR("Xiaoye Meng");
MODULE_LICENSE("Dual BSD/GPL");

