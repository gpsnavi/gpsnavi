/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_NOTICELIST_REQ_H
#define SMCCOM_NOTICELIST_REQ_H

// お知らせ情報一覧情報
typedef struct _SMNOTICEINFO {
	INT32		noticeId;
	Char		notice[SCC_CMN_NOTICE_LIST_STR_SIZE];
	INT32		unreadFlg;
} SMNOTICEINFO;

E_SC_RESULT CC_NoticeListReq_SendRecv(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, const Char *lang, INT32 type, INT32 limit, SMNOTICEINFO *noticeInfo, INT32 *noticeInfoNum, Char *noticeNum, Char *recv, INT32 recvBufSize, Char *apiStatus);

#endif // #ifndef SMCCOM_NOTICELIST_REQ_H
