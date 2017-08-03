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

#ifndef CFFEX_INCLUDED
#define CFFEX_INCLUDED

/* FIXME */
#pragma pack(push, 1)
struct cffexheader {
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
	unsigned		unknown;          /* FIXME */
	char			buf[0];
};
struct login_zj {
	struct cffexheader	header;
	unsigned short		type;
	unsigned short		length;
	char			td_day[9];        /* TradingDay */
	char			userid[16];       /* UserID */
	char			brokerid[11];     /* BrokerID */
	char			passwd[41];       /* Password */
	char			upi[41];          /* UserProductInfo */
	char			ipi[41];          /* InterfaceProductInfo */
	char			pi[41];           /* ProtocolInfo */
	char			ip[21];           /* IPAddress */
	char			mac[21];          /* MacAddress */
	int			dcid;             /* DataCenterID */
	int			upfs;             /* UserProductFileSize */
	unsigned short		type1;
	unsigned short		length1;
	int			seq_series1;      /* SequenceSeries */
	int			seq_number1;      /* SequenceNo */
	unsigned short		type4;
	unsigned short		length4;
	int			seq_series4;
	int			seq_number4;
	unsigned short		type64;
	unsigned short		length64;
	int			seq_series64;
	int			seq_number64;
};
struct subscribe_zj {
	struct cffexheader	header;
	unsigned short		type;
	unsigned short		length;
	char			instid[31];       /* InstrumentID */
	struct cffexheader	header2;
	unsigned short		type1;
	unsigned short		length1;
	int			seq_series1;      /* SequenceSeries */
	int			seq_number1;      /* SequenceNo */
	unsigned short		type4;
	unsigned short		length4;
	int			seq_series4;
	int			seq_number4;
	unsigned short		type64;
	unsigned short		length64;
	int			seq_series64;
	int			seq_number64;
};
struct loginrsp {
	char			td_day[9];        /* TradingDay */
	char			brokerid[11];     /* BrokerID */
	char			userid[16];       /* UserID */
	char			log_time[9];      /* LoginTime */
	char			max_locid[21];    /* MaxOrderLocalID */
	char			ts_name[61];      /* TradingSystemName */
	int			dcid;             /* DataCenterID */
	int			pf_size;          /* PrivateFlowSize */
	int			uf_size;          /* UserFlowSize */
};
#pragma pack(pop)

#endif /* CFFEX_INCLUDED */

