/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_TOKENREQ_H
#define SMCCOM_TOKENREQ_H

//------------------------------------------------------------------------------------------------------------------------
//
//	SMCCom_TokenReqヘッダ（内部公開用）
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
* @brief トークン情報
*/
typedef struct _T_CC_TOKENREQ_INFO {
	Char*			pToken;
	INT32*			pStatus;
	Char*			pApiSts;
} T_CC_TOKENREQ_INFO;

/**
* @brief トークンパーサー
*/
typedef struct _T_CC_TOKENREQ_PARSER {
	INT32				state;
	Char				*buf;
	T_CC_TOKENREQ_INFO	tokenInf;
} T_CC_TOKENREQ_PARSER;


//---------------------------------------------------------------------------------
//外部関数宣言
//---------------------------------------------------------------------------------
E_SC_RESULT CC_TokenReq_SendRecv(SMCAL* smcal, T_CC_CMN_SMS_API_PRM* pPrm, Char* pRecvBuf, UINT32 RecvBuf_sz, Char* pApiSts);	// Token.req送信・受信処理


#endif // #ifndef SMCCOM_TOKENREQ_H
