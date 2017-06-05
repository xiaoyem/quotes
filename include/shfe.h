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

#ifndef SHFE_INCLUDED
#define SHFE_INCLUDED

/* FIXME */
#pragma pack(push, 1)
struct hbtimeout {
	char			ftd_type;         /* FTDTypeNone */
	char			ftd_extd_length;  /* 0x06 */
	unsigned short		ftd_cont_length;
	char			tag_type;         /* 0x07 */
	char			tag_length;       /* 0x04 */
	unsigned		timeout;
};
struct heartbeat {
	char			ftd_type;         /* FTDTypeNone */
	char			ftd_extd_length;  /* 0x02 */
	unsigned short		ftd_cont_length;
	char			tag_type;         /* 0x05 */
	char			tag_length;
};
struct shfeheader {
	char			ftd_type;         /* FTDTypeCompressed? */
	char			ftd_extd_length;
	unsigned short		ftd_cont_length;
	char			version;          /* Version */
	char			ftdc_type;        /* Type */
	char			unenc_length;     /* UnEncLen */
	char			chain;            /* Chain */
	unsigned short		seq_series;       /* SequenceSeries */
	unsigned		seq_number;       /* SequenceNumber */
	unsigned		prv_number;       /* ? */
	unsigned short		fld_count;        /* FieldCount */
	unsigned short		ftdc_cont_length; /* FTDCContentLength */
	unsigned		rid;              /* RequestID */
	char			buf[0];
};
struct login {
	struct shfeheader	header;
	unsigned short		type;
	unsigned short		length;
	char			td_day[9];        /* TradingDay */
	char			brokerid[11];     /* BrokerID */
	char			userid[16];       /* UserID */
	char			passwd[41];       /* Password */
	char			upi[11];          /* UserProductInfo */
	char			ipi[11];          /* InterfaceProductInfo */
	char			pi[11];           /* ProtocolInfo */
	char			mac[21];          /* MacAddress */
	char			ot_passwd[41];    /* OneTimePassword */
	char			client_ip[16];    /* ClientIPAddress */
	unsigned short		type1;
	unsigned short		length1;
	short			seq_series1;      /* SequenceSeries */
	int			seq_number1;      /* SequenceNo */
	unsigned short		type4;
	unsigned short		length4;
	short			seq_series4;
	int			seq_number4;
};
struct subscribe {
	struct shfeheader	header;
	unsigned short		type;
	unsigned short		length;
	char			instid[31];       /* InstrumentID */
};
struct info {
	int			errid;            /* ErrorID */
	char			errmsg[81];       /* ErrorMsg */
};
struct quote {
	char			td_day[9];        /* TradingDay */
	char			instid[31];       /* InstrumentID */
	char			excid[9];         /* ExchangeID */
	char			exc_instid[31];   /* ExchangeInstID */
	double			last;             /* LastPrice */
	double			presettle;        /* PreSettlementPrice */
	double			preclose;         /* PreClosePrice */
	double			preopenint;       /* PreOpenInterest */
	double			open;             /* OpenPrice */
	double			high;             /* HighestPrice */
	double			low;              /* LowestPrice */
	int			volume;           /* Volume */
	double			turnover;         /* Turnover */
	double			openint;          /* OpenInterest */
	double			close;            /* ClosePrice */
	double			settle;           /* SettlementPrice */
	double			upperlimit;       /* UpperLimitPrice */
	double			lowerlimit;       /* LowerLimitPrice */
	double			predelta;         /* PreDelta */
	double			delta;            /* CurrDelta */
	char			time[9];          /* UpdateTime */
	int			msec;             /* UpdateMillisec */
	double			bid1;             /* BidPrice1 */
	int			bvol1;            /* BidVolume1 */
	double			ask1;             /* AskPrice1 */
	int			avol1;            /* AskVolume1 */
	double			bid2;             /* BidPrice2 */
	int			bvol2;            /* BidVolume2 */
	double			ask2;             /* AskPrice2 */
	int			avol2;            /* AskVolume2 */
	double			bid3;             /* BidPrice3 */
	int			bvol3;            /* BidVolume3 */
	double			ask3;             /* AskPrice3 */
	int			avol3;            /* AskVolume3 */
	double			bid4;             /* BidPrice4 */
	int			bvol4;            /* BidVolume4 */
	double			ask4;             /* AskPrice4 */
	int			avol4;            /* AskVolume4 */
	double			bid5;             /* BidPrice5 */
	int			bvol5;            /* BidVolume5 */
	double			ask5;             /* AskPrice5 */
	int			avol5;            /* AskVolume5 */
	double			average;          /* AveragePrice */
	char			at_day[9];        /* ActionDay */
};
#pragma pack(pop)

#endif /* SHFE_INCLUDED */

