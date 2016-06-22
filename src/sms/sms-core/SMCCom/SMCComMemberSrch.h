/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_MEMBER_SRCH_H
#define SMCCOM_MEMBER_SRCH_H

//---------------------------------------------------------------------------------
//インクルード
//---------------------------------------------------------------------------------


//---------------------------------------------------------------------------------
//列挙型定義
//---------------------------------------------------------------------------------


//---------------------------------------------------------------------------------
//構造体定義
//---------------------------------------------------------------------------------
// メンバ情報
typedef struct _SMMEMBERINFO {
	Char		guid[CC_CMN_GUID_STR_SIZE];
	Char		userName[SCC_CHAT_MAXCHAR_USERNAME];
	Char		lastDate[SCC_CHAT_MAXCHAR_LASTDATE];
	Char		avtSmlURL[SCC_CHAT_MAXCHAR_URL];
	Char		avtMidURL[SCC_CHAT_MAXCHAR_URL];
	Char		avtLrgURL[SCC_CHAT_MAXCHAR_URL];
	Bool		latFlg;
	Bool		lonFlg;
	Char		reserve2[3];
	DOUBLE		lat;
	DOUBLE		lon;
	INT32		join_status;
} SMMEMBERINFO;


//---------------------------------------------------------------------------------
//外部関数宣言
//---------------------------------------------------------------------------------
E_SC_RESULT CC_MemberSrch_SendRecv(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, const Char *roomNo, SMMEMBERINFO *MemberInf, INT32 *userNum, Char *recv, INT32 recvBufSize, Char *apiStatus);

#endif // #ifndef SMCCOM_MEMBER_SRCH_H
