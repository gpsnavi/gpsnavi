/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_ROOM_REG_H
#define SMCCOM_ROOM_REG_H

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
typedef struct _SMCREATEROOMINF {
	Char		*roomName;
	Char		*userList;
} SMCREATEROOMINF;


//---------------------------------------------------------------------------------
//外部関数宣言
//---------------------------------------------------------------------------------
E_SC_RESULT CC_RoomReg_SendRecv(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, const SMCREATEROOMINF *ChatRoomInf, Char *roomNo, Char *recv, INT32 recvBufSize, Char *apiStatus);

#endif // #ifndef SMCCOM_ROOM_REG_H
