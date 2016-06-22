/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_GEMLIKE_REQ_H
#define SMCCOM_GEMLIKE_REQ_H

// GEMLike情報
typedef struct _SMGEMLIKEINFO {
	INT32		likeId;
	Char		guid[SCC_MAX_ID];
	Char		user[SCC_MAX_USERNAME];
	Char		avaSmlUrl[SCC_MAX_URL];
	Char		avaMidUrl[SCC_MAX_URL];
	Char		avaLrgUrl[SCC_MAX_URL];
	Char		profileUrl[SCC_MAX_URL];
	INT32		rgstDatetm;
} SMGEMLIKEINFO;

E_SC_RESULT CC_GemLikeReq_SendRecv(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, const Char *gemId, INT32 limit, SMGEMLIKEINFO *likeInfo, INT32 *likeInfoNum, Char *recv, INT32 recvBufSize, Char *apiStatus);

#endif // #ifndef SMCCOM_GEMLIKE_REQ_H
