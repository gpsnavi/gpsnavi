/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_MESSAGE_INBOX_H
#define SMCCOM_MESSAGE_INBOX_H

// パーソナルお知らせ情報
typedef struct _SMPERSONALINFO {
	INT32	msgGuid;
	Char	guid[CC_CMN_GUID_STR_SIZE];
	Char	userName[CC_CMN_USERNAME_STR_SIZE];
	Char	avtSmlURL[SCC_MAX_URL];
	Char	avtMidURL[SCC_MAX_URL];
	Char	avtLrgURL[SCC_MAX_URL];
	Char	msgTitle[SCC_CMN_PRSNLINFO_MSG_TITLE];
	Char	msgDetail[SCC_CMN_PRSNLINFO_MSG_DETAIL];
	INT32	msgTime;
	INT32	status;
	INT32	index;
	Char	mappingId[CC_CMN_MAPPING_ID_SIZE];
} SMPERSONALINFO;

E_SC_RESULT CC_MessageInbox_SendRecv(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, INT32 offset, INT32 limit, INT32 msgStatus, SMPERSONALINFO *personalInfo, INT32 *personalInfoNum, Char *personalNum, Char *recv, INT32 recvBufSize, Char *apiStatus);

#endif // #ifndef SMCCOM_MESSAGE_INBOX_H
