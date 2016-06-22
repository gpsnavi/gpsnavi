/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_DEVICEIDREQ_H
#define SMCCOM_DEVICEIDREQ_H

//------------------------------------------------------------------------------------------------------------------------
//
//	SMCCom_DeviceidReqヘッダ（内部公開用）
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
* @brief デバイスID取得情報
*/
typedef struct _T_CC_DEVICEIDREQ_INFO {
	INT32*			pStatus;
	Char*			pDevId;
} T_CC_DEVICEIDREQ_INFO;

/**
* @brief デバイスID取得パーサー
*/
typedef struct _T_CC_DEVICEIDREQ_PARSER {
	INT32					state;
	Char					*buf;
	T_CC_DEVICEIDREQ_INFO	deiceidInf;
} T_CC_DEVICEIDREQ_PARSER;


//---------------------------------------------------------------------------------
//外部関数宣言
//---------------------------------------------------------------------------------
E_SC_RESULT CC_DeviceidReq_SendRecv(SMCAL* smcal, T_CC_CMN_SMS_API_PRM* pPrm, Char* pRecvBuf, UINT32 RecvBuf_sz, Char* pApiSts, Bool isPolling);	// Deviceid.req送信・受信処理


#endif // #ifndef SMCCOM_DEVICEIDREQ_H
