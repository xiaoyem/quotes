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
	char			buf[0];
};
#pragma pack(pop)

#endif /* CFFEX_INCLUDED */

