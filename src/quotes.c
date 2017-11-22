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
#include "cffex.h"

/* FIXME */
struct client {
	btree_t			btree;
	void			*quote;
	struct socket		*msock;
	struct socket		*csock;
	struct task_struct	*task;
	struct timer_list	timer;
	u32			heartbeat;
	u32			dataready;
	u32			connected;
	u32			disconnected;
	u32			prvno;
	u32			inpos;
	unsigned char		inbuf[64 * 1024 * 1024];
	unsigned char		debuf[8192];
};

/* FIXME */
#define PRINT_QUOTE_ZJ(quote) \
	printk(KERN_ERR "[%s] send quote '%s,%s,%d,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f," \
		"%d,%f,%f,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d," \
		"%s,%s,%03d' failed\n", \
		__func__, \
		quote->td_day, \
		quote->sgid, \
		quote->sid, \
		quote->presettle, \
		quote->preclose, \
		quote->preopenint, \
		quote->predelta, \
		quote->open, \
		quote->high, \
		quote->low, \
		quote->close, \
		quote->upperlimit, \
		quote->lowerlimit, \
		quote->settle, \
		quote->delta, \
		quote->last, \
		quote->volume, \
		quote->turnover, \
		quote->openint, \
		quote->bid1, \
		quote->bvol1, \
		quote->ask1, \
		quote->avol1, \
		quote->bid2, \
		quote->bvol2, \
		quote->bid3, \
		quote->bvol3, \
		quote->ask2, \
		quote->avol2, \
		quote->ask3, \
		quote->avol3, \
		quote->bid4, \
		quote->bvol4, \
		quote->bid5, \
		quote->bvol5, \
		quote->ask4, \
		quote->avol4, \
		quote->ask5, \
		quote->avol5, \
		quote->instid, \
		quote->time, \
		quote->msec)
#define PRINT_QUOTE_SH(quote) \
	printk(KERN_ERR "[%s] send quote '%s,%s,%s,%s,%f,%f,%f,%f,%f,%f,%f,%d,%f,%f,%f,%f," \
		"%f,%f,%f,%f,%s,%03d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d," \
		"%f,%d,%f,%d,%f,%s' failed\n", \
		__func__, \
		quote->td_day, \
		quote->instid, \
		quote->excid, \
		quote->exc_instid, \
		quote->last, \
		quote->presettle, \
		quote->preclose, \
		quote->preopenint, \
		quote->open, \
		quote->high, \
		quote->low, \
		quote->volume, \
		quote->turnover, \
		quote->openint, \
		quote->close, \
		quote->settle, \
		quote->upperlimit, \
		quote->lowerlimit, \
		quote->predelta, \
		quote->delta, \
		quote->time, \
		quote->msec, \
		quote->bid1, \
		quote->bvol1, \
		quote->ask1, \
		quote->avol1, \
		quote->bid2, \
		quote->bvol2, \
		quote->ask2, \
		quote->avol2, \
		quote->bid3, \
		quote->bvol3, \
		quote->ask3, \
		quote->avol3, \
		quote->bid4, \
		quote->bvol4, \
		quote->ask4, \
		quote->avol4, \
		quote->bid5, \
		quote->bvol5, \
		quote->ask5, \
		quote->avol5, \
		quote->average, \
		quote->at_day)

/* FIXME */
static int usefemas = 0;
static char *multicast_ip;
static int multicast_port;
static char *quote_ip;
static int quote_port;
static char *brokerid, *userid, *passwd, *contracts[2048];
static int count;
module_param(usefemas,       int, 0000);
module_param(multicast_ip, charp, 0000);
module_param(multicast_port, int, 0000);
module_param(quote_ip,     charp, 0000);
module_param(quote_port,     int, 0000);
module_param(brokerid,     charp, 0000);
module_param(userid,       charp, 0000);
module_param(passwd,       charp, 0000);
module_param_array(contracts, charp, &count, 0000);
static struct client ct;

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
	to.timeout         = usefemas ? 0x14000000 : 0x27000000;
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
	struct net *net = sock_net(c->csock->sk);
	struct net_device *dev;

	if (usefemas) {
		struct login_zj lo;
		struct sockaddr_in sa;
		int len;

		memset(&lo, '\0', sizeof lo);
		lo.header.ftd_type         = 0x02;
		lo.header.ftd_cont_length  = 0x3c01;
		lo.header.version          = 0x01;
		lo.header.unenc_length     = 0x01;
		lo.header.chain            = 0x4c;
		lo.header.seq_number       = 0x01500000;
		lo.header.fld_count        = 0x0400;
		lo.header.ftdc_cont_length = 0x2201;
		lo.type                    = 0x0130;
		lo.length                  = 0xfa00;
		strncat(lo.userid,   userid,   sizeof lo.userid   - 1);
		strncat(lo.brokerid, brokerid, sizeof lo.brokerid - 1);
		strncat(lo.passwd,   passwd,   sizeof lo.passwd   - 1);
		/* FIXME */
		lo.ipi[0]                  = 'L';
		lo.ipi[1]                  = 'n';
		lo.ipi[2]                  = 'x';
		lo.ipi[3]                  = '6';
		lo.ipi[4]                  = '4';
		lo.ipi[5]                  = ' ';
		lo.ipi[6]                  = 'F';
		lo.ipi[7]                  = 'e';
		lo.ipi[8]                  = 'm';
		lo.ipi[9]                  = 'a';
		lo.ipi[10]                 = 's';
		lo.ipi[11]                 = 'C';
		lo.ipi[12]                 = 'l';
		lo.ipi[13]                 = 'a';
		lo.ipi[14]                 = 's';
		lo.ipi[15]                 = 's';
		lo.ipi[16]                 = 'i';
		lo.ipi[17]                 = 'c';
		lo.ipi[18]                 = 'A';
		lo.ipi[19]                 = 'P';
		lo.ipi[20]                 = 'I';
		lo.ipi[21]                 = ' ';
		lo.ipi[22]                 = 'v';
		lo.ipi[23]                 = '1';
		lo.ipi[24]                 = '.';
		lo.ipi[25]                 = '0';
		lo.ipi[26]                 = '3';
		lo.ipi[27]                 = ' ';
		lo.ipi[28]                 = 'L';
		lo.ipi[29]                 = '4';
		lo.ipi[30]                 = '0';
		lo.ipi[31]                 = '0';
		lo.pi[0]                   = 'F';
		lo.pi[1]                   = 'T';
		lo.pi[2]                   = 'D';
		lo.pi[3]                   = 'C';
		lo.pi[4]                   = ' ';
		lo.pi[5]                   = '0';
		/* FIXME */
		c->csock->ops->getname(c->csock, (struct sockaddr *)&sa, &len, 0);
		snprintf(lo.ip, sizeof lo.ip, "%pI4", &sa.sin_addr);
		for_each_netdev(net, dev)
			if (strcmp(dev->name, "lo")) {
				snprintf(lo.mac, sizeof lo.mac, "%02X:%02X:%02X:%02X:%02X:%02X%c",
					dev->dev_addr[0],
					dev->dev_addr[1],
					dev->dev_addr[2],
					dev->dev_addr[3],
					dev->dev_addr[4],
					dev->dev_addr[5], 0x0a);
				break;
			}
		lo.upfs                    = 0xd09c0000;
		lo.type1                   = 0x3330;
		lo.length1                 = 0x0800;
		lo.seq_series1             = 0x01000000;
		lo.type4                   = 0x3330;
		lo.length4                 = 0x0800;
		lo.seq_series4             = 0x04000000;
		lo.type64                  = 0x3330;
		lo.length64                = 0x0800;
		lo.seq_series64            = 0x64000000;
		lo.seq_number64            = 0xffffffff;
		if (quotes_send(c->csock, (unsigned char *)&lo, sizeof lo) < 0)
			printk(KERN_ERR "[%s] send failed\n", __func__);
	} else {
		struct login_sh lo;
		struct timeval time;
		struct tm tm;

		memset(&lo, '\0', sizeof lo);
		lo.header.ftd_type         = 0x02;
		lo.header.ftd_cont_length  = 0xea00;
		lo.header.version          = 0x01;
		lo.header.unenc_length     = 0x0b;
		lo.header.chain            = 0x4c;
		lo.header.seq_number       = 0x00300000;
		lo.header.fld_count        = 0x0300;
		lo.header.ftdc_cont_length = 0xd400;
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
}

/* FIXME */
static void subscribe(struct client *c, const char *contract) {
	if (usefemas) {
		struct subscribe_zj sb;

		memset(&sb, '\0', sizeof sb);
		sb.header.ftd_type          = 0x02;
		sb.header.ftd_cont_length   = 0x3d00;
		sb.header.version           = 0x01;
		sb.header.unenc_length      = 0x01;
		sb.header.chain             = 0x43;
		sb.header.seq_series        = 0x0100;
		sb.header.seq_number        = 0x72500000;
		sb.header.prv_number        = htonl(++c->prvno);
		sb.header.fld_count         = 0x0100;
		sb.header.ftdc_cont_length  = 0x2300;
		sb.type                     = 0x5230;
		sb.length                   = 0x1f00;
		strncat(sb.instid, contract, sizeof sb.instid - 1);
		sb.header2.ftd_type         = 0x02;
		sb.header2.ftd_cont_length  = 0x3e00;
		sb.header2.version          = 0x01;
		sb.header2.unenc_length     = 0x01;
		sb.header2.chain            = 0x4c;
		sb.header2.seq_series       = 0x0100;
		sb.header2.seq_number       = 0x72500000;
		sb.header2.prv_number       = htonl(++c->prvno);
		sb.header2.fld_count        = 0x0300;
		sb.header2.ftdc_cont_length = 0x2400;
		sb.type1                    = 0x3330;
		sb.length1                  = 0x0800;
		sb.seq_series1              = 0x01000000;
		sb.type4                    = 0x3330;
		sb.length4                  = 0x0800;
		sb.seq_series4              = 0x04000000;
		sb.type64                   = 0x3330;
		sb.length64                 = 0x0800;
		sb.seq_series64             = 0x64000000;
		sb.seq_number64             = 0xffffffff;
		if (quotes_send(c->csock, (unsigned char *)&sb, sizeof sb) < 0)
			printk(KERN_ERR "[%s] send failed\n", __func__);
	} else {
		struct subscribe_sh sb;

		memset(&sb, '\0', sizeof sb);
		sb.header.ftd_type          = 0x02;
		sb.header.ftd_cont_length   = 0x3900;
		sb.header.version           = 0x01;
		sb.header.unenc_length      = 0x0b;
		sb.header.chain             = 0x4c;
		sb.header.seq_number        = 0x01440000;
		sb.header.fld_count         = 0x0100;
		sb.header.ftdc_cont_length  = 0x2300;
		sb.type                     = 0x4124;
		sb.length                   = 0x1f00;
		strncat(sb.instid, contract, sizeof sb.instid - 1);
		if (quotes_send(c->csock, (unsigned char *)&sb, sizeof sb) < 0)
			printk(KERN_ERR "[%s] send failed\n", __func__);
	}
}

/* FIXME */
static inline void handle_51packet(struct client *c, struct quote_zj *quote) {
	btree_node_t node;
	int i;
	struct quote_zj *cquote;

	if ((node = btree_find(c->btree, quote->instid, &i))) {
		c->quote = btree_node_value(node, i);
		cquote = (struct quote_zj *)c->quote;
		*cquote = *quote;
	} else if ((c->quote = kzalloc(sizeof (struct quote_zj), GFP_KERNEL))) {
		cquote = (struct quote_zj *)c->quote;
		*cquote = *quote;
		btree_insert(c->btree, cquote->instid, cquote);
	} else {
		printk(KERN_ERR "[%s] error locating quote for '%s'\n", __func__, quote->instid);
		return;
	}
	cquote->sid    = ntohl(quote->sid);
	handle_double(&cquote->presettle);
	handle_double(&cquote->preclose);
	handle_double(&cquote->preopenint);
	handle_double(&cquote->predelta);
	handle_double(&cquote->open);
	handle_double(&cquote->high);
	handle_double(&cquote->low);
	handle_double(&cquote->close);
	handle_double(&cquote->upperlimit);
	handle_double(&cquote->lowerlimit);
	handle_double(&cquote->settle);
	handle_double(&cquote->delta);
	handle_double(&cquote->last);
	cquote->volume = ntohl(cquote->volume);
	handle_double(&cquote->turnover);
	handle_double(&cquote->openint);
	handle_double(&cquote->bid1);
	cquote->bvol1  = ntohl(cquote->bvol1);
	handle_double(&cquote->ask1);
	cquote->avol1  = ntohl(cquote->avol1);
	handle_double(&cquote->bid2);
	handle_double(&cquote->bid3);
	handle_double(&cquote->ask2);
	handle_double(&cquote->ask3);
	handle_double(&cquote->bid4);
	handle_double(&cquote->bid5);
	handle_double(&cquote->ask4);
	handle_double(&cquote->ask5);
	cquote->msec   = ntohl(cquote->msec);
	if (quotes_send(c->msock, (unsigned char *)cquote, sizeof *cquote) < 0)
		PRINT_QUOTE_ZJ(cquote);
}

static inline void handle_30packet(struct client *c, unsigned short *type, unsigned short *length) {
	struct quote_zj *cquote;

	switch (*type) {
	case 0x5030:
		{
			struct mdtime_zj *mdtime = (struct mdtime_zj *)((unsigned char *)type + 4);
			btree_node_t node;
			int i;

			if ((node = btree_find(c->btree, mdtime->instid, &i))) {
				c->quote = btree_node_value(node, i);
				cquote = (struct quote_zj *)c->quote;
			} else if ((c->quote = kzalloc(sizeof (struct quote_zj), GFP_KERNEL))) {
				cquote = (struct quote_zj *)c->quote;
				memcpy(cquote->instid, mdtime->instid, sizeof cquote->instid);
				btree_insert(c->btree, cquote->instid, cquote);
			} else {
				printk(KERN_ERR "[%s] error locating quote for '%s'\n",
					__func__, mdtime->instid);
				return;
			}
			if (strcmp(cquote->time, mdtime->time))
				memcpy(cquote->time, mdtime->time, sizeof cquote->time);
			cquote->msec       = ntohl(mdtime->msec);
		}
		break;
	case 0x4130:
		{
			struct mdbase_zj *mdbase = (struct mdbase_zj *)((unsigned char *)type + 4);

			if (unlikely(c->quote == NULL)) {
				printk(KERN_ERR "[%s] unrecoverable error\n", __func__);
				return;
			}
			cquote = (struct quote_zj *)c->quote;
			if (strcmp(cquote->td_day, mdbase->td_day))
				memcpy(cquote->td_day, mdbase->td_day, sizeof cquote->td_day);
			cquote->presettle  = mdbase->presettle;
			handle_double(&cquote->presettle);
			cquote->preclose   = mdbase->preclose;
			handle_double(&cquote->preclose);
			cquote->preopenint = mdbase->preopenint;
			handle_double(&cquote->preopenint);
			cquote->predelta   = mdbase->predelta;
			handle_double(&cquote->predelta);
		}
		break;
	case 0x4230:
		{
			struct mdstatic_zj *mdstatic = (struct mdstatic_zj *)
					((unsigned char *)type + 4);

			if (unlikely(c->quote == NULL)) {
				printk(KERN_ERR "[%s] unrecoverable error\n", __func__);
				return;
			}
			cquote = (struct quote_zj *)c->quote;
			cquote->open       = mdstatic->open;
			handle_double(&cquote->open);
			cquote->high       = mdstatic->high;
			handle_double(&cquote->high);
			cquote->low        = mdstatic->low;
			handle_double(&cquote->low);
			cquote->close      = mdstatic->close;
			handle_double(&cquote->close);
			cquote->upperlimit = mdstatic->upperlimit;
			handle_double(&cquote->upperlimit);
			cquote->lowerlimit = mdstatic->lowerlimit;
			handle_double(&cquote->lowerlimit);
			cquote->settle     = mdstatic->settle;
			handle_double(&cquote->settle);
			cquote->delta      = mdstatic->delta;
			handle_double(&cquote->delta);
		}
		break;
	case 0x4330:
		{
			struct mdlast_zj *mdlast = (struct mdlast_zj *)((unsigned char *)type + 4);

			if (unlikely(c->quote == NULL)) {
				printk(KERN_ERR "[%s] unrecoverable error\n", __func__);
				return;
			}
			cquote = (struct quote_zj *)c->quote;
			cquote->last       = mdlast->last;
			handle_double(&cquote->last);
			cquote->volume     = ntohl(mdlast->volume);
			cquote->turnover   = mdlast->turnover;
			handle_double(&cquote->turnover);
			cquote->openint    = mdlast->openint;
			handle_double(&cquote->openint);
		}
		break;
	case 0x4530:
		{
			struct mdbest_zj *mdbest = (struct mdbest_zj *)((unsigned char *)type + 4);

			if (unlikely(c->quote == NULL)) {
				printk(KERN_ERR "[%s] unrecoverable error\n", __func__);
				return;
			}
			cquote = (struct quote_zj *)c->quote;
			cquote->bid1       = mdbest->bid1;
			handle_double(&cquote->bid1);
			cquote->bvol1      = ntohl(mdbest->bvol1);
			cquote->ask1       = mdbest->ask1;
			handle_double(&cquote->ask1);
			cquote->avol1      = ntohl(mdbest->avol1);
		}
		break;
	case 0x4630:
		{
			struct mdbid23_zj *mdbid23 = (struct mdbid23_zj *)
					((unsigned char *)type + 4);

			if (unlikely(c->quote == NULL)) {
				printk(KERN_ERR "[%s] unrecoverable error\n", __func__);
				return;
			}
			cquote = (struct quote_zj *)c->quote;
			/* FIXME */
			cquote->bid2       = mdbid23->bid2;
			handle_double(&cquote->bid2);
			cquote->bid3       = mdbid23->bid3;
			handle_double(&cquote->bid3);
		}
		break;
	case 0x4730:
		{
			struct mdask23_zj *mdask23 = (struct mdask23_zj *)
					((unsigned char *)type + 4);

			if (unlikely(c->quote == NULL)) {
				printk(KERN_ERR "[%s] unrecoverable error\n", __func__);
				return;
			}
			/* FIXME */
			cquote = (struct quote_zj *)c->quote;
			cquote->ask2       = mdask23->ask2;
			handle_double(&cquote->ask2);
			cquote->ask3       = mdask23->ask3;
			handle_double(&cquote->ask3);
		}
		break;
	case 0x4830:
		{
			struct mdbid45_zj *mdbid45 = (struct mdbid45_zj *)
					((unsigned char *)type + 4);

			if (unlikely(c->quote == NULL)) {
				printk(KERN_ERR "[%s] unrecoverable error\n", __func__);
				return;
			}
			cquote = (struct quote_zj *)c->quote;
			/* FIXME */
			cquote->bid4       = mdbid45->bid4;
			handle_double(&cquote->bid4);
			cquote->bid5       = mdbid45->bid5;
			handle_double(&cquote->bid5);
		}
		break;
	case 0x4930:
		{
			struct mdask45_zj *mdask45 = (struct mdask45_zj *)
					((unsigned char *)type + 4);

			if (unlikely(c->quote == NULL)) {
				printk(KERN_ERR "[%s] unrecoverable error\n", __func__);
				return;
			}
			cquote = (struct quote_zj *)c->quote;
			/* FIXME */
			cquote->ask4       = mdask45->ask4;
			handle_double(&cquote->ask4);
			cquote->ask5       = mdask45->ask5;
			handle_double(&cquote->ask5);
		}
		break;
	default:
		print_buf((unsigned char *)type, *length + 4);
		break;
	}
}

/* FIXME */
static inline void handle_12packet(struct client *c, struct quote_sh *quote) {
	btree_node_t node;
	int i;
	struct quote_sh *cquote;

	if ((node = btree_find(c->btree, quote->instid, &i))) {
		c->quote = btree_node_value(node, i);
		cquote = (struct quote_sh *)c->quote;
		*cquote = *quote;
	} else if ((c->quote = kzalloc(sizeof (struct quote_sh), GFP_KERNEL))) {
		cquote = (struct quote_sh *)c->quote;
		*cquote = *quote;
		btree_insert(c->btree, cquote->instid, cquote);
	} else {
		printk(KERN_ERR "[%s] error locating quote for '%s'\n", __func__, quote->instid);
		return;
	}
	handle_double(&cquote->last);
	handle_double(&cquote->presettle);
	handle_double(&cquote->preclose);
	handle_double(&cquote->preopenint);
	handle_double(&cquote->open);
	handle_double(&cquote->high);
	handle_double(&cquote->low);
	cquote->volume = ntohl(cquote->volume);
	handle_double(&cquote->turnover);
	handle_double(&cquote->openint);
	handle_double(&cquote->close);
	handle_double(&cquote->settle);
	handle_double(&cquote->upperlimit);
	handle_double(&cquote->lowerlimit);
	handle_double(&cquote->predelta);
	handle_double(&cquote->delta);
	cquote->msec   = ntohl(cquote->msec);
	handle_double(&cquote->bid1);
	cquote->bvol1  = ntohl(cquote->bvol1);
	handle_double(&cquote->ask1);
	cquote->avol1  = ntohl(cquote->avol1);
	handle_double(&cquote->bid2);
	handle_double(&cquote->ask2);
	handle_double(&cquote->bid3);
	handle_double(&cquote->ask3);
	handle_double(&cquote->bid4);
	handle_double(&cquote->ask4);
	handle_double(&cquote->bid5);
	handle_double(&cquote->ask5);
	handle_double(&cquote->average);
	if (quotes_send(c->msock, (unsigned char *)cquote, sizeof *cquote) < 0)
		PRINT_QUOTE_SH(cquote);
}

static inline void handle_24packet(struct client *c, unsigned short *type, unsigned short *length) {
	struct quote_sh *cquote;

	switch (*type) {
	case 0x3924:
		{
			struct mdtime_sh *mdtime = (struct mdtime_sh *)((unsigned char *)type + 4);
			btree_node_t node;
			int i;

			if ((node = btree_find(c->btree, mdtime->instid, &i))) {
				c->quote = btree_node_value(node, i);
				cquote = (struct quote_sh *)c->quote;
			} else if ((c->quote = kzalloc(sizeof (struct quote_sh), GFP_KERNEL))) {
				cquote = (struct quote_sh *)c->quote;
				memcpy(cquote->instid, mdtime->instid, sizeof cquote->instid);
				btree_insert(c->btree, cquote->instid, cquote);
			} else {
				printk(KERN_ERR "[%s] error locating quote for '%s'\n",
					__func__, mdtime->instid);
				return;
			}
			if (strcmp(cquote->time, mdtime->time))
				memcpy(cquote->time, mdtime->time, sizeof cquote->time);
			cquote->msec       = ntohl(mdtime->msec);
			if (strcmp(cquote->at_day, mdtime->at_day))
				memcpy(cquote->at_day, mdtime->at_day, sizeof cquote->at_day);
		}
		break;
	case 0x3424:
		{
			struct mdbest_sh *mdbest = (struct mdbest_sh *)((unsigned char *)type + 4);

			if (unlikely(c->quote == NULL)) {
				printk(KERN_ERR "[%s] unrecoverable error\n", __func__);
				return;
			}
			cquote = (struct quote_sh *)c->quote;
			cquote->bid1       = mdbest->bid1;
			handle_double(&cquote->bid1);
			cquote->bvol1      = ntohl(mdbest->bvol1);
			cquote->ask1       = mdbest->ask1;
			handle_double(&cquote->ask1);
			cquote->avol1      = ntohl(mdbest->avol1);
		}
		break;
	case 0x3124:
		{
			struct mdbase_sh *mdbase = (struct mdbase_sh *)((unsigned char *)type + 4);

			if (unlikely(c->quote == NULL)) {
				printk(KERN_ERR "[%s] unrecoverable error\n", __func__);
				return;
			}
			cquote = (struct quote_sh *)c->quote;
			if (strcmp(cquote->td_day, mdbase->td_day))
				memcpy(cquote->td_day, mdbase->td_day, sizeof cquote->td_day);
			cquote->presettle  = mdbase->presettle;
			handle_double(&cquote->presettle);
			cquote->preclose   = mdbase->preclose;
			handle_double(&cquote->preclose);
			cquote->preopenint = mdbase->preopenint;
			handle_double(&cquote->preopenint);
			cquote->predelta   = mdbase->predelta;
			handle_double(&cquote->predelta);
		}
		break;
	case 0x3224:
		{
			struct mdstatic_sh *mdstatic = (struct mdstatic_sh *)
					((unsigned char *)type + 4);

			if (unlikely(c->quote == NULL)) {
				printk(KERN_ERR "[%s] unrecoverable error\n", __func__);
				return;
			}
			cquote = (struct quote_sh *)c->quote;
			cquote->open       = mdstatic->open;
			handle_double(&cquote->open);
			cquote->high       = mdstatic->high;
			handle_double(&cquote->high);
			cquote->low        = mdstatic->low;
			handle_double(&cquote->low);
			cquote->close      = mdstatic->close;
			handle_double(&cquote->close);
			cquote->upperlimit = mdstatic->upperlimit;
			handle_double(&cquote->upperlimit);
			cquote->lowerlimit = mdstatic->lowerlimit;
			handle_double(&cquote->lowerlimit);
			cquote->settle     = mdstatic->settle;
			handle_double(&cquote->settle);
			cquote->delta      = mdstatic->delta;
			handle_double(&cquote->delta);
		}
		break;
	case 0x3324:
		{
			struct mdlast_sh *mdlast = (struct mdlast_sh *)((unsigned char *)type + 4);

			if (unlikely(c->quote == NULL)) {
				printk(KERN_ERR "[%s] unrecoverable error\n", __func__);
				return;
			}
			cquote = (struct quote_sh *)c->quote;
			cquote->last       = mdlast->last;
			handle_double(&cquote->last);
			cquote->volume     = ntohl(mdlast->volume);
			cquote->turnover   = mdlast->turnover;
			handle_double(&cquote->turnover);
			cquote->openint    = mdlast->openint;
			handle_double(&cquote->openint);
		}
		break;
	case 0x3524:
		{
			struct mdbid23_sh *mdbid23 = (struct mdbid23_sh *)
					((unsigned char *)type + 4);

			if (unlikely(c->quote == NULL)) {
				printk(KERN_ERR "[%s] unrecoverable error\n", __func__);
				return;
			}
			cquote = (struct quote_sh *)c->quote;
			/* FIXME */
			cquote->bid2       = mdbid23->bid2;
			handle_double(&cquote->bid2);
			cquote->bid3       = mdbid23->bid3;
			handle_double(&cquote->bid3);
		}
		break;
	case 0x3624:
		{
			struct mdask23_sh *mdask23 = (struct mdask23_sh *)
					((unsigned char *)type + 4);

			if (unlikely(c->quote == NULL)) {
				printk(KERN_ERR "[%s] unrecoverable error\n", __func__);
				return;
			}
			/* FIXME */
			cquote = (struct quote_sh *)c->quote;
			cquote->ask2       = mdask23->ask2;
			handle_double(&cquote->ask2);
			cquote->ask3       = mdask23->ask3;
			handle_double(&cquote->ask3);
		}
		break;
	case 0x3724:
		{
			struct mdbid45_sh *mdbid45 = (struct mdbid45_sh *)
					((unsigned char *)type + 4);

			if (unlikely(c->quote == NULL)) {
				printk(KERN_ERR "[%s] unrecoverable error\n", __func__);
				return;
			}
			cquote = (struct quote_sh *)c->quote;
			/* FIXME */
			cquote->bid4       = mdbid45->bid4;
			handle_double(&cquote->bid4);
			cquote->bid5       = mdbid45->bid5;
			handle_double(&cquote->bid5);
		}
		break;
	case 0x3824:
		{
			struct mdask45_sh *mdask45 = (struct mdask45_sh *)
					((unsigned char *)type + 4);

			if (unlikely(c->quote == NULL)) {
				printk(KERN_ERR "[%s] unrecoverable error\n", __func__);
				return;
			}
			cquote = (struct quote_sh *)c->quote;
			/* FIXME */
			cquote->ask4       = mdask45->ask4;
			handle_double(&cquote->ask4);
			cquote->ask5       = mdask45->ask5;
			handle_double(&cquote->ask5);
		}
		break;
	case 0x8124:
		{
			double *average = (double *)((unsigned char *)type + 4);

			if (unlikely(c->quote == NULL)) {
				printk(KERN_ERR "[%s] unrecoverable error\n", __func__);
				return;
			}
			cquote = (struct quote_sh *)c->quote;
			cquote->average    = *average;
			handle_double(&cquote->average);
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

			if (usefemas) {
				struct cffexheader *ch = (struct cffexheader *)start;

				switch (ch->seq_number) {
				case 0x02500000:
					{
						struct info *info = (struct info *)(ch->buf + 4 +
								sizeof (struct loginrsp) + 4);

						if (info->errid == 0)
							for (i = 0; i < count; ++i)
								if (strcmp(contracts[i], ""))
									subscribe(c, contracts[i]);
					}
					break;
				case 0x71500000:
					{
						unsigned short *type   = (unsigned short *)ch->buf;
						unsigned short *length = (unsigned short *)
								(ch->buf + 2);
						struct quote_zj *quote = (struct quote_zj *)
								(ch->buf + 4);

						if (*type == 0x5130 && *length == 0x3601)
							handle_51packet(c, quote);

					}
					break;
				case 0x70500000:
					{
						unsigned short count   = ntohs(ch->fld_count);
						unsigned short *type   = (unsigned short *)ch->buf;
						unsigned short *length = (unsigned short *)
								(ch->buf + 2);
						struct quote_zj *cquote;

						for (i = 0; i < count; ++i) {
							handle_30packet(c, type, length);
							type   = (unsigned short *)
								((unsigned char *)type + 4 +
								ntohs(*length));
							length = (unsigned short *)
								((unsigned char *)type + 2);
						}
						cquote = (struct quote_zj *)c->quote;
						if (count > 0 && cquote && quotes_send(c->msock,
							(unsigned char *)cquote,
							sizeof *cquote) < 0)
							PRINT_QUOTE_ZJ(cquote);
					}
					break;
				default:
					print_buf(start, ftd_cont_len + 4);
					break;
				}
			} else {
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
						char *contract = (char *)
								(sh->buf + 4 + sizeof *info + 4);

						if (info->errid != 0)
							printk(KERN_INFO "[%s] subscribe '%s'"
								" failed\n",
								__func__, contract);
					}
					break;
				case 0x01f10000:
					{
						unsigned short *type   = (unsigned short *)sh->buf;
						unsigned short *length = (unsigned short *)
								(sh->buf + 2);
						struct quote_sh *quote = (struct quote_sh *)
								(sh->buf + 4);

						if (*type == 0x1200 && *length == 0x6201)
							handle_12packet(c, quote);
					}
					break;
				case 0x03f10000:
					{
						unsigned short count   = ntohs(sh->fld_count);
						unsigned short *type   = (unsigned short *)sh->buf;
						unsigned short *length = (unsigned short *)
								(sh->buf + 2);
						struct quote_sh *cquote;

						for (i = 0; i < count; ++i) {
							handle_24packet(c, type, length);
							type   = (unsigned short *)
								((unsigned char *)type + 4 +
								ntohs(*length));
							length = (unsigned short *)
								((unsigned char *)type + 2);
						}
						cquote = (struct quote_sh *)c->quote;
						if (count > 0 && cquote && quotes_send(c->msock,
							(unsigned char *)cquote,
							sizeof *cquote) < 0)
							PRINT_QUOTE_SH(cquote);
					}
					break;
				default:
					print_buf(c->debuf, j);
					break;
				}
			}
		} else if (ftd_type == 0x00 && ftd_extd_len == 0x02) {
			/* printk(KERN_INFO "[%s] receiving heartbeat\n", __func__); */
			ftd_cont_len = ftd_extd_len;
		} else if (ftd_type == 0x00 && ftd_extd_len == 0x06) {
			printk(KERN_INFO "[%s] receiving heartbeat timeout\n", __func__);
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
	int tv = usefemas ? 21 : 15;

	atomic_set((atomic_t *)&c->heartbeat, 1);
	init_timer(&c->timer);
	c->timer.expires  = jiffies + tv * HZ;
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
			int tv = usefemas ? 21 : 15;

			init_timer(&c->timer);
			c->timer.expires  = jiffies + tv * HZ;
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
			schedule_timeout_uninterruptible(5 * HZ);
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
	if ((ct.btree = btree_new(256, NULL, NULL, quote_free)) == NULL) {
		printk(KERN_ERR "[%s] error allocating quote cache\n", __func__);
		return -ENOMEM;
	}
	if (sock_create_kern(PF_INET, SOCK_DGRAM, IPPROTO_UDP, &ct.msock) < 0) {
		printk(KERN_ERR "[%s] error creating multicast socket\n", __func__);
		return -EIO;
	}
	/* FIXME */
	kernel_setsockopt(ct.msock, SOL_SOCKET, SO_SNDBUF, (char *)&val, sizeof val);
	if (quotes_connect(ct.msock, multicast_ip, multicast_port, 0) < 0) {
		printk(KERN_ERR "[%s] error connecting multicast address\n", __func__);
		goto end;
	}
	if (sock_create_kern(PF_INET, SOCK_STREAM, IPPROTO_TCP, &ct.csock) < 0) {
		printk(KERN_ERR "[%s] error creating client socket\n", __func__);
		goto end;
	}
	/* FIXME */
	ct.csock->sk->sk_allocation = GFP_ATOMIC;
	set_sock_callbacks(ct.csock, &ct);
	kernel_setsockopt(ct.csock, SOL_TCP, TCP_NODELAY, (char *)&one, sizeof one);
	kernel_setsockopt(ct.csock, SOL_SOCKET, SO_RCVBUF, (char *)&val, sizeof val);
	kernel_setsockopt(ct.csock, SOL_SOCKET, SO_SNDBUF, (char *)&val, sizeof val);
	/* FIXME */
	if ((ret = quotes_connect(ct.csock, quote_ip, quote_port, O_NONBLOCK)) == -EINPROGRESS) {
	} else if (ret < 0) {
		printk(KERN_ERR "[%s] error connecting quote address\n", __func__);
		sock_release(ct.csock);
		goto end;
	}
	ct.task = kthread_create(quotes_thread, &ct, usefemas ? "quote_zj" : "quotes_sh");
	if (IS_ERR(ct.task)) {
		printk(KERN_ERR "[%s] error creating quotes thread\n", __func__);
		sock_release(ct.csock);
		goto end;
	}
	kthread_bind(ct.task, 1);
	wake_up_process(ct.task);
	return 0;

end:
	sock_release(ct.msock);
	return -EIO;
}

/* FIXME */
static void __exit quotes_exit(void) {
	if (timer_pending(&ct.timer))
		del_timer(&ct.timer);
	kthread_stop(ct.task);
	sock_release(ct.csock);
	sock_release(ct.msock);
	btree_free(&ct.btree);
}

module_init(quotes_init);
module_exit(quotes_exit);
MODULE_AUTHOR("Xiaoye Meng");
MODULE_LICENSE("Dual BSD/GPL");

