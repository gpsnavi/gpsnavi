/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_TERMREG_H
#define SMCCOM_TERMREG_H

//------------------------------------------------------------------------------------------------------------------------
//
//	SMCCom_TermRegヘッダ（内部公開用）
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
* @brief デバイス登録情報
*/
typedef struct _T_CC_TERMREG_INFO {
	INT32*			pStatus;
	Char*			pApiSts;
} T_CC_TERMREG_INFO;

/**
* @brief デバイス登録パーサー
*/
typedef struct _T_CC_TERMREG_PARSER {
	INT32					state;
	Char					*buf;
	T_CC_TERMREG_INFO		termRegInf;
} T_CC_TERMREG_PARSER;


//---------------------------------------------------------------------------------
//外部関数宣言
//---------------------------------------------------------------------------------
E_SC_RESULT CC_TermReg_SendRecv(SMCAL* smcal, T_CC_CMN_SMS_API_PRM* pPrm, Char* pRecvBuf, UINT32 RecvBuf_sz, Char* pApiSts);	// Term.reg送信・受信処理


#endif // #ifndef SMCCOM_TERMREG_H
