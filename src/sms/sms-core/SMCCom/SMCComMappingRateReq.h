/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_MAPPING_RATE_REQ_H
#define SMCCOM_MAPPING_RATE_REQ_H

// マッピングに評価を付けたユーザ情報
typedef struct _SMMAPPINGRATEINFO {
	Char		guid[SCC_MAX_ID];
	Char		user[SCC_MAX_USERNAME];
	Char		avaSmlUrl[SCC_MAX_URL];
	Char		avaMidUrl[SCC_MAX_URL];
	Char		avaLrgUrl[SCC_MAX_URL];
	INT32		rgstDatetm;
	INT32		altDatetm;
} SMMAPPINGRATEINFO;

E_SC_RESULT CC_MappingRateReq_SendRecv(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, const Char *mappingId, INT32 ratingType, INT32 startPos, INT32 maxCnt, SMMAPPINGRATEINFO *rateInfo, INT32 *rateInfoNum, LONG *allNum, Char *recv, INT32 recvBufSize, Char *apiStatus);

#endif // #ifndef SMCCOM_MAPPING_RATE_REQ_H
