/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_GEMSPOT_ID_SEARCH_H
#define SMCCOM_GEMSPOT_ID_SEARCH_H

// GEM検索条件
typedef struct _SMGEMSPOTIDSEARCH {
	Char			spotId[CC_CMN_GEMSPOT_SPOT_ID];
	Char			reserve[3];
} SMGEMSPOTIDSEARCH;

// GEMSPOT詳細情報
typedef struct _SMGEMSPOTINFO {
	DOUBLE		lat;
	DOUBLE		lon;
	Char		spotName[CC_CMN_GEMSPOT_SPOT_NAME];
	Char		tel[CC_CMN_GEMSPOT_TEL];
	Char		address[CC_CMN_GEMSPOT_ADDRESS];
	Char		spotInfo[CC_CMN_GEMSPOT_SPOT_INFO];
	INT32		cls1Id;
	INT32		cls2Id;
	Char		cls1Name[CC_CMN_GEMSPOT_CLS1_NAME];
	Char		cls2Name[CC_CMN_GEMSPOT_CLS2_NAME];
	Char		reserve[2];
} SMGEMSPOTINFO;

E_SC_RESULT CC_GemspotIdSearch_SendRecv(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, INT64 gemSpotId, SMGEMSPOTINFO *gemSpot, Char *recv, INT32 recvBufSize, Char *apiStatus);

#endif // #ifndef SMCCOM_GEMSPOT_ID_SEARCH_H
