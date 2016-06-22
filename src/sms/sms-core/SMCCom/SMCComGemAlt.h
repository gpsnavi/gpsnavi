/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_GEM_ALT_H
#define SMCCOM_GEM_ALT_H

// GEM情報
typedef struct _SMGEMUPD {
	Char			*gemId;
	DOUBLE			lat;
	DOUBLE			lon;
	Char			*text;
	Char			*pic;
	INT32			accessFlg;
	INT32			pictureDel;
} SMGEMUPD;

E_SC_RESULT CC_GemAlt_SendRecv(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, const SMGEMUPD *gem, Char *recv, INT32 recvBufSize, Char *apiStatus);

#endif // #ifndef SMCCOM_GEM_ALT_H
