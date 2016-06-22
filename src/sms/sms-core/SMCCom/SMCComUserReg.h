/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_USERREG_H
#define SMCCOM_USERREG_H

//------------------------------------------------------------------------------------------------------------------------
//
//	SMCCom_UserRegヘッダ（内部公開用）
//
//------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------
//インクルード
//---------------------------------------------------------------------------------


//---------------------------------------------------------------------------------
//列挙型定義
//---------------------------------------------------------------------------------


//---------------------------------------------------------------------------------
//構造体定義
//---------------------------------------------------------------------------------
/**
* @brief ユーザー登録情報
*/
typedef struct _T_CC_USERREG_INFO {
	INT32*			pStatus;
	Char*			pApiStatus;
	Char*			pActStatus;		// TR3.5 アクティベーション対応
} T_CC_USERREG_INFO;

/**
* @brief ユーザー登録パーサー
*/
typedef struct _T_CC_USERREG_PARSER {
	INT32				state;
	Char				*buf;
	T_CC_USERREG_INFO	userRegInf;
} T_CC_USERREG_PARSER;


//---------------------------------------------------------------------------------
//外部関数宣言
//---------------------------------------------------------------------------------
E_SC_RESULT CC_UserReg_SendRecv(SMCAL* smcal, T_CC_CMN_SMS_API_PRM* pPrm, Char* pRecvBuf, UINT32 RecvBuf_sz, Char* pApiSts);	// User.reg送信・受信処理


#endif // #ifndef SMCCOM_USERREG_H
