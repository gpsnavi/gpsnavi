/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_MAPPING_LIKE_REQ_H
#define SMCCOM_MAPPING_LIKE_REQ_H

// マッピングにLIKEを付けたユーザ情報
typedef struct _SMMAPPINGLIKEINFO {
	INT32		likeId;
	Char		guid[SCC_MAX_ID];
	Char		user[SCC_MAX_USERNAME];
	Char		avaSmlUrl[SCC_MAX_URL];
	Char		avaMidUrl[SCC_MAX_URL];
	Char		avaLrgUrl[SCC_MAX_URL];
	INT32		rgstDatetm;
} SMMAPPINGLIKEINFO;

E_SC_RESULT CC_MappingLikeReq_SendRecv(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, const Char *mappingId, INT32 startPos, INT32 maxCnt, SMMAPPINGLIKEINFO *likeInfo, INT32 *likeInfoNum, LONG *allNum, Char *recv, INT32 recvBufSize, Char *apiStatus);

#endif // #ifndef SMCCOM_MAPPING_LIKE_REQ_H
