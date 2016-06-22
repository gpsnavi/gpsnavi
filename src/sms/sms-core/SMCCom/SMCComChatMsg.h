/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_CHAT_MSG_H
#define SMCCOM_CHAT_MSG_H

//---------------------------------------------------------------------------------
//インクルード
//---------------------------------------------------------------------------------


//---------------------------------------------------------------------------------
//列挙型定義
//---------------------------------------------------------------------------------


//---------------------------------------------------------------------------------
//構造体定義
//---------------------------------------------------------------------------------
// チャットメッセージ情報
typedef struct _SMCHATMESSAGE {
	Char		*roomNo;
	Char		*message;
	Char		*msgType;
	Char		*picFilePath;
	Char		*gemId;
} SMCHATMESSAGE;


//---------------------------------------------------------------------------------
//外部関数宣言
//---------------------------------------------------------------------------------
E_SC_RESULT CC_ChatMsg_SendRecv(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, const SMCHATMESSAGE *chatMsg, Char *recv, INT32 recvBufSize, Char *apiStatus);

#endif // #ifndef SMCCOM_CHAT_MSG_H
