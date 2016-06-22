/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_NOTIFY_REG_H
#define SMCCOM_NOTIFY_REG_H

// GCMプッシュ通知登録情報
typedef struct _SMGCMPUSHINFO {
	INT32			operation;
	INT32			notifyType;
	INT32			platformType;
	Char			*deviceToken;
} SMGCMPUSHINFO;

E_SC_RESULT CC_NotifyReg_SendRecv(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, const SMGCMPUSHINFO *pushInfo, Char *recv, INT32 recvBufSize, Char *apiStatus);

#endif // #ifndef SMCCOM_NOTIFY_REG_H
