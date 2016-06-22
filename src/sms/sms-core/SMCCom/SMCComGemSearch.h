/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_GEM_SEARCH_H
#define SMCCOM_GEM_SEARCH_H

// GEM検索条件
typedef struct _SMGEMSEARCH {
	DOUBLE			lat;
	DOUBLE			lon;
	INT32			radius;
	Char			*keyword;
	Char			*groupId;
	Char			*id;
	INT32			maxCnt;
	INT32			accessFlg;
	INT32			sort;
} SMGEMSEARCH;

E_SC_RESULT CC_GemSearch_SendRecv(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, const SMGEMSEARCH *gemSerch, SMGEMINFO *gemInfo, INT32 *gemInfoNum, INT32 *lastFlg, Char *recv, INT32 recvBufSize, Char *apiStatus);

#endif // #ifndef SMCCOM_GEM_SEARCH_H
