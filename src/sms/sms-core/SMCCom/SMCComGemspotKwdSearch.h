/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_GEMSPOT_KWD_SEARCH_H
#define SMCCOM_GEMSPOT_KWD_SEARCH_H

// GEM検索条件
typedef struct _SMGEMSPOTKWDSEARCH {
	Char			*keyword;
	INT64			startPos;
	INT32			maxCnt;
	DOUBLE			lat;
	DOUBLE			lon;
	INT32			radius;
	INT32			sort;
} SMGEMSPOTKWDSEARCH;

// GEMSPOT情報
typedef struct _SMGEMSPOT {
	INT64		spotId;
	DOUBLE		lat;
	DOUBLE		lon;
	INT32		cls1Id;
	INT32		cls2Id;
	Char		spotName[CC_CMN_GEMSPOT_SPOT_NAME];
	Char		cls1Name[CC_CMN_GEMSPOT_CLS1_NAME];
	Char		cls2Name[CC_CMN_GEMSPOT_CLS2_NAME];
	Char		reserve[1];
} SMGEMSPOT;

E_SC_RESULT CC_GemspotKwdSearch_SendRecv(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, const SMGEMSPOTKWDSEARCH *gemSpotSerch, SMGEMSPOT *gemSpot, INT32 *gemSpotNum, INT64 *gemSpotAllNum, Char *recv, INT32 recvBufSize, Char *apiStatus);

#endif // #ifndef SMCCOM_GEMSPOT_KWD_SEARCH_H
