/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_RNAME_ALT_H
#define SMCCOM_RNAME_ALT_H

//---------------------------------------------------------------------------------
//インクルード
//---------------------------------------------------------------------------------


//---------------------------------------------------------------------------------
//列挙型定義
//---------------------------------------------------------------------------------


//---------------------------------------------------------------------------------
//構造体定義
//---------------------------------------------------------------------------------
// ルーム作成情報
typedef struct _SMCHANGEROOMNAME {
	Char		*roomNo;
	Char		*roomName;
} SMCHANGEROOMNAME;


//---------------------------------------------------------------------------------
//外部関数宣言
//---------------------------------------------------------------------------------
E_SC_RESULT CC_RnameAlt_SendRecv(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, const SMCHANGEROOMNAME *ChangeRoomName, Char *recv, INT32 recvBufSize, Char *apiStatus);

#endif // #ifndef SMCCOM_RNAME_ALT_H
