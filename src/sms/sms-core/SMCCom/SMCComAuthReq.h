/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_AUTHREQ_H
#define SMCCOM_AUTHREQ_H

//------------------------------------------------------------------------------------------------------------------------
//
//	SMCComAuthReqヘッダ（内部公開用）
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
* @brief ユーザー認証情報
*/
typedef struct _T_CC_AUTHREQ_INFO {
	INT32*			pStatus;
	Char*			pMessage;
	Char*			pApiStatus;
	Char*			pGuid;
	Char*			pUserSig;
	Char*			pActStatus;		// TR3.5 アクティベーション対応
	Char*			pPolicyLateFlg;
	Char*			pLang;
	INT32*			pRatingDialog;
	INT32*			pPkgFlg;
} T_CC_AUTHREQ_INFO;

/**
* @brief ユーザー認証パーサー
*/
typedef struct _T_CC_AUTHREQ_PARSER {
	INT32				state;
	Char				*buf;
	T_CC_AUTHREQ_INFO	authReqInf;
} T_CC_AUTHREQ_PARSER;


//---------------------------------------------------------------------------------
//外部関数宣言
//---------------------------------------------------------------------------------
E_SC_RESULT CC_AuthReq_SendRecv(SMCAL* smcal, T_CC_CMN_SMS_API_PRM* pPrm, Char* pRecvBuf, UINT32 RecvBuf_sz, Char* apiSts, Bool isPolling);	// Auth.Req送信・受信処理
#endif // #ifndef SMCCOM_AUTHREQ_H
