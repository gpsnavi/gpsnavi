/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_USERSRCH_H
#define SMCCOM_USERSRCH_H

//---------------------------------------------------------------------------------
//インクルード
//---------------------------------------------------------------------------------


//---------------------------------------------------------------------------------
//列挙型定義
//---------------------------------------------------------------------------------


//---------------------------------------------------------------------------------
//構造体定義
//---------------------------------------------------------------------------------
// ユーザ基本情報
typedef struct _SMUSERINFO {
	Char		guid[CC_CMN_GUID_STR_SIZE];
	Char		userName[SCC_CHAT_MAXCHAR_USERNAME];
	Char		lastDate[SCC_CHAT_MAXCHAR_LASTDATE];
	Char		avtSmlURL[SCC_CHAT_MAXCHAR_URL];
	Char		avtMidURL[SCC_CHAT_MAXCHAR_URL];
	Char		avtLrgURL[SCC_CHAT_MAXCHAR_URL];
} SMUSERINFO;

typedef struct _SMUSERSRCH {
	Char			*pOnly;
	Char			*pUserName;
	INT32			offset;
	INT32			limit;
	Char			*pOrder;
} SMUSERSRCH;


//---------------------------------------------------------------------------------
//外部関数宣言
//---------------------------------------------------------------------------------
E_SC_RESULT CC_User_Srch_SendRecv(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, const SMUSERSRCH *userSearch, SMUSERINFO *userInfo, INT32 *userInfoNum, Char *userNum, const Char *fileName, Char *recv, UINT32 recvBufSize, Char *apiStatus);

#endif // #ifndef SMCCOM_USERSRCH_H
