/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_AGREEAUTHREQ_H
#define SMCCOM_AGREEAUTHREQ_H

//------------------------------------------------------------------------------------------------------------------------
//
//	SMCCom_AgreeAuthReqヘッダ（内部公開用）
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
* @brief 利用規約再同意と認証情報
*/
typedef struct _T_CC_AGREEAUTHREQ_INFO {
	Char*			pMessage;
	Char*			pApiStatus;
	Char*			pUserSig;
	Char*			pGuid;
	Char*			pActStatus;		// TR3.5 アクティベーション対応
	Char*			pPolicyLateFlg;
	INT32*			pRatingDialog;
} T_CC_AGREEAUTHREQ_INFO;

/**
* @brief 利用規約再同意と認証パーサー
*/
typedef struct _T_CC_AGREEAUTHREQ_PARSER {
	INT32				state;
	Char				*buf;
	T_CC_AGREEAUTHREQ_INFO	authReqInf;
} T_CC_AGREEAUTHREQ_PARSER;


//---------------------------------------------------------------------------------
//外部関数宣言
//---------------------------------------------------------------------------------
E_SC_RESULT CC_AgreeAuthReq_SendRecv(SMCAL* smcal, T_CC_CMN_SMS_API_PRM* pPrm, Char* pRecvBuf, UINT32 RecvBuf_sz, Char* apiSts, Bool isPolling);	// Auth.Req送信・受信処理


#endif // #ifndef SMCCOM_AGREEAUTHREQ_H
