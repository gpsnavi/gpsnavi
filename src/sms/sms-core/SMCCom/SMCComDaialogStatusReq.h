/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_DAIALOGSTATUSREQ_H
#define SMCCOM_DAIALOGSTATUSREQ_H

//------------------------------------------------------------------------------------------------------------------------
//
//	SMCCom_DaialogStatusReqヘッダ（内部公開用）
//
//------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------
//インクルード
//---------------------------------------------------------------------------------


//---------------------------------------------------------------------------------
//構造体定義
//---------------------------------------------------------------------------------
/**
* @brief 通知メッセージ情報
*/
typedef struct _T_CC_DAIALOGSTATUS_INFO {
	Char			message[CC_CMN_MESSAGE_SIZE];
	Char			messageId[CC_CMN_MESSAGE_ID_SIZE];
	Char			apiStatus[CC_CMN_XML_RES_STS_SIZE];
	Bool			cachedFlg;
	Char			reserve[2];
} T_CC_DAIALOGSTATUS_INFO;

//---------------------------------------------------------------------------------
//外部関数宣言
//---------------------------------------------------------------------------------
E_SC_RESULT CC_DaialogStatusReq_SendRecv(SMCAL* smcal, T_CC_CMN_SMS_API_PRM* param, const Char *appVer, const Char *lang, Char* recvBuf, UINT32 recvBufSize, T_CC_DAIALOGSTATUS_INFO *dlgStatusInfo);


#endif // #ifndef SMCCOM_DAIALOGSTATUSREQ_H
