/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_CHAT_HIS_H
#define SMCCOM_CHAT_HIS_H

//---------------------------------------------------------------------------------
//インクルード
//---------------------------------------------------------------------------------


//---------------------------------------------------------------------------------
//列挙型定義
//---------------------------------------------------------------------------------


//---------------------------------------------------------------------------------
//構造体定義
//---------------------------------------------------------------------------------
// チャット取得条件
typedef struct _SMCHATGETQUALI {
	Char		*roomNo;
	Char		*transaction;
	INT32		getNum;
	Bool		PInfoWatch;
	Bool		PInfoOpen;
	Char		*PInfoRoomNo;
} SMCHATGETQUALI;


//---------------------------------------------------------------------------------
//外部関数宣言
//---------------------------------------------------------------------------------
E_SC_RESULT CC_ChatHis_SendRecv(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, const SMCHATGETQUALI *getQuali, const Char *fileName, Char *filePath, Char *recv, UINT32 recvBufSize, Char *apiStatus, Bool isPolling);

#endif // #ifndef SMCCOM_CHAT_HIS_H
