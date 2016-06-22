/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_MAPPING_HISTORY_REQ_H
#define SMCCOM_MAPPING_HISTORY_REQ_H

// マッピング編集履歴
typedef struct _SMMAPPINGHISTORY {
	INT32			historyId;
	INT32			detailNo;
	Char			guid[CC_CMN_GUID_STR_SIZE];
	Char			user[CC_CMN_USERNAME_STR_SIZE];
	Char			avtSmlUrl[CC_CMN_MAPPING_URL_SIZE];
	Char			avtMidUrl[CC_CMN_MAPPING_URL_SIZE];
	Char			avtLrgUrl[CC_CMN_MAPPING_URL_SIZE];
	INT32			cause;
	INT32			altType;
	INT32			altDatetm;
} SMMAPPINGHISTORY;

E_SC_RESULT CC_MappingHistoryReq_SendRecv(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, const Char *mappingId, INT32 startPos, INT32 maxCnt, SMMAPPINGHISTORY *mappingInfo, INT32 *mappingInfoNum, LONG *allNum, Char *recv, INT32 recvBufSize, Char *apiStatus);

#endif // #ifndef SMCCOM_MAPPING_HISTORY_REQ_H
