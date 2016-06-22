/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_MAPPING_COMMENT_REQ_H
#define SMCCOM_MAPPING_COMMENT_REQ_H

// マッピングコメント
typedef struct _SMMAPPINGCOMMENTINFO {
	INT32			commentId;
	Char			guid[CC_CMN_GUID_STR_SIZE];
	Char			user[CC_CMN_USERNAME_STR_SIZE];
	Char			avtSmlUrl[CC_CMN_MAPPING_URL_SIZE];
	Char			avtMidUrl[CC_CMN_MAPPING_URL_SIZE];
	Char			avtLrgUrl[CC_CMN_MAPPING_URL_SIZE];
	Char			comment[CC_CMN_MAPPING_COMMENT_SIZE];
	INT32			rgstDatetm;
	INT32			altDatetm;
} SMMAPPINGCOMMENTINFO;

E_SC_RESULT CC_MappingCommentReq_SendRecv(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, const Char *mappingId, INT32 startPos, INT32 maxCnt, SMMAPPINGCOMMENTINFO *commentInfo, INT32 *mappingInfoNum, LONG *allNum, Char *recv, INT32 recvBufSize, Char *apiStatus);

#endif // #ifndef SMCCOM_MAPPING_COMMENT_REQ_H
