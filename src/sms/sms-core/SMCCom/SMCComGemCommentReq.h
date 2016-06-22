/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_GEMCOMMENT_REQ_H
#define SMCCOM_GEMCOMMENT_REQ_H

// GEMコメント情報
typedef struct _SMGEMCOMMENTINFO {
	INT32		commentId;
	Char		guid[SCC_MAX_ID];
	Char		user[SCC_MAX_USERNAME];
	Char		avaSmlUrl[SCC_MAX_URL];
	Char		avaMidUrl[SCC_MAX_URL];
	Char		avaLrgUrl[SCC_MAX_URL];
	Char		profileUrl[SCC_MAX_URL];
	Char		comment[SCC_CMN_GEM_COMMENT];
	Char		commentPictUrl[SCC_MAX_URL];
	INT32		rgstDatetm;
	INT32		altDatetm;
} SMGEMCOMMENTINFO;

E_SC_RESULT CC_GemCommentReq_SendRecv(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, const Char *gemId, INT32 limit, SMGEMCOMMENTINFO *gemInfo, INT32 *gemInfoNum, Char *recv, INT32 recvBufSize, Char *apiStatus);

#endif // #ifndef SMCCOM_GEMCOMMENT_REQ_H
