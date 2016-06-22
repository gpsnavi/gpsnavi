/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_GEM_REG_H
#define SMCCOM_GEM_REG_H

// GEM情報
typedef struct _SMGEMREG {
	DOUBLE			lat;
	DOUBLE			lon;
	Char			*text;
	Char			*pic;
	Char			*circleGuid;
	INT32			accessFlg;
	UINT32			gem_datetime;
	Char			gemId[SCC_MAX_ID];
	Char			gemUrl[SCC_MAX_URL];
} SMGEMREG;

E_SC_RESULT CC_GemReg_SendRecv(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, SMGEMREG *gem, Char *recv, INT32 recvBufSize, Char *apiStatus);

#endif // #ifndef SMCCOM_GEM_REG_H
