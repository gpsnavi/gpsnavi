/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_UNREAD_SRCH_H
#define SMCCOM_UNREAD_SRCH_H

//---------------------------------------------------------------------------------
//インクルード
//---------------------------------------------------------------------------------


//---------------------------------------------------------------------------------
//列挙型定義
//---------------------------------------------------------------------------------


//---------------------------------------------------------------------------------
//構造体定義
//---------------------------------------------------------------------------------
// 未読メッセージ有ルーム情報
typedef struct _SMUNREADMSGROOM {
	Char		roomNo[SCC_CHAT_MAXCHAR_ROOMNO];
	Char		lastDate[SCC_CHAT_MAXCHAR_LASTDATE];
	Char		transaction[SCC_CHAT_MAXCHAR_TRANSACTION];
} SMUNREADMSGROOM;


//---------------------------------------------------------------------------------
//外部関数宣言
//---------------------------------------------------------------------------------
E_SC_RESULT CC_UnreadSrch_SendRecv(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, SMUNREADMSGROOM *unrdMsgRm, INT32 *getnum, Char *recv, INT32 recvBufSize, Char *apiStatus, Bool isPolling);

#endif // #ifndef SMCCOM_UNREAD_SRCH_H
