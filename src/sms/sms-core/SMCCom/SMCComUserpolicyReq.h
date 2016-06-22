/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_USERPOLICYREQ_H
#define SMCCOM_USERPOLICYREQ_H

//------------------------------------------------------------------------------------------------------------------------
//
//	SMCCom_UserpolicyReqヘッダ（内部公開用）
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
* @brief 規約同意文取得情報
*/
typedef struct _T_CC_USERPOLOCYREQ_INFO {
	INT32*			pStatus;
	Char*			pApiStatus;
	Char*			pVer;
	Char*			pLang;
	Char*			pPolicy;
	const Char*		lang;
} T_CC_USERPOLICYREQ_INFO;

/**
* @brief 規約同意文取得パーサー
*/
typedef struct _T_CC_USERPOLICYREQ_PARSER {
	INT32					state;
	Char					*buf;
	T_CC_USERPOLICYREQ_INFO	userpolicyInf;
} T_CC_USERPOLICYREQ_PARSER;


//---------------------------------------------------------------------------------
//外部関数宣言
//---------------------------------------------------------------------------------
E_SC_RESULT CC_UserpolicyReq_SendRecv(SMCAL* smcal, T_CC_CMN_SMS_API_PRM* pPrm, const Char *lang, Char* pRecvBuf, UINT32 RecvBuf_sz, Char* pApiSts);	// Userpolicy.req送信・受信処理


#endif // #ifndef SMCCOM_USERPOLICYREQ_H
