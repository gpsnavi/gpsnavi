/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_ROOM_SRCH_H
#define SMCCOM_ROOM_SRCH_H

//---------------------------------------------------------------------------------
//インクルード
//---------------------------------------------------------------------------------


//---------------------------------------------------------------------------------
//列挙型定義
//---------------------------------------------------------------------------------


//---------------------------------------------------------------------------------
//構造体定義
//---------------------------------------------------------------------------------
// ルーム情報
typedef struct _SMROOMINFO {
	Char		roomNo[SCC_CHAT_MAXCHAR_ROOMNO];
	Char		roomName[SCC_CHAT_MAXCHAR_ROOMNAME];
	Bool		joined;
	Bool		unRead;
	Char		lastDatetm[SCC_CHAT_MAXCHAR_LASTDATE];
	UINT16		userCnt;
	SMUSERBASEINFO	userBaseInf[SCC_CHAT_MAXUSER_NUM];
} SMROOMINFO;

typedef struct _SMROOMSRCHQUALI {
	UINT16			offset;
	UINT16			limit;
	Char			*pOrder;
	Char			*pPriority;
} SMROOMSRCHQUALI;

//typedef struct _SMROOMSEARCH {
//	UINT16			offset;
//	UINT16			limit;
//	Char			*pOrder;
//	Char			*pPriority;
//	SMROOMINFO		*roomInf;
//	INT32			*roomCnt;
//} SMROOMSEARCH;

//---------------------------------------------------------------------------------
//外部関数宣言
//---------------------------------------------------------------------------------
E_SC_RESULT CC_RoomSrch_SendRecv(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, const SMROOMSRCHQUALI *SrchQuali, SMROOMINFO *roomInf, INT32 *roomNum, const Char *fileName, Char *recv, UINT32 recvBufSize, Char *apiStatus, Bool isPolling);

#endif // #ifndef SMCCOM_ROOM_SRCH_H
