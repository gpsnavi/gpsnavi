/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_GEM_TIMELINE_SEARCH_H
#define SMCCOM_GEM_TIMELINE_SEARCH_H

// GEMタイムライン検索条件
typedef struct _SMGEMTIMELINESEARCH {
	Char			*keyword;
	Char			*id;
	INT32			maxCnt;
	INT32			sort;
} SMGEMTIMELINESEARCH;

E_SC_RESULT CC_GemTimelineSearch_SendRecv(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, const SMGEMTIMELINESEARCH *gemSerch, SMGEMTIMELINEINFO *gemInfo, INT32 *gemInfoNum, INT32 *lastFlg, Char *recv, INT32 recvBufSize, Char *apiStatus);

#endif // #ifndef SMCCOM_GEM_TIMELINE_SEARCH_H
