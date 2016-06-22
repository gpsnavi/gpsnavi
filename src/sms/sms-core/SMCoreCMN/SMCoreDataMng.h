/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

/*
 * SMCoreDataMng.h
 *
 *  Created on: 2014/02/27
 *      Author: 70251034
 */

#ifndef SMCOREDATAMNG_H_
#define SMCOREDATAMNG_H_

/*-------------------------------------------------------------------
 * 定数定義
 *-------------------------------------------------------------------*/
#define SC_DM_MAPREQ_MAX			(160)			// 1要求MAX数(CashのMAX_READ_CNTと同数) TODO
/*-------------------------------------------------------------------
 * 構造体定義
 *-------------------------------------------------------------------*/
typedef struct _SC_DM_MAPREQ {
//	UINT8* bin;						// 結果格納アドレス
	UINT32 pclId;					// パーセルID
	UINT32 kind;					// 地図種別
} SC_DM_MAPREQ;
/*-------------------------------------------------------------------
 * 宣言
 *-------------------------------------------------------------------*/
E_SC_RESULT SC_DM_DataMngInit();
E_SC_RESULT SC_DM_DataMngFinalize();
E_SC_RESULT SC_DM_GetFinishCnt(E_SC_USER_ID aUser, UINT32 aReqId, UINT32* aCnt);
E_SC_RESULT SC_DM_MapRead_SemRes(pthread_msq_msg_t *rcvMsg);
E_SC_RESULT SC_DM_MapRead_MsgRes(pthread_msq_msg_t *rcvMsg);
E_SC_RESULT SC_DM_MapFree_NoRes(pthread_msq_msg_t *rcvMsg);
E_SC_RESULT SC_DM_MapFree_MsgRes(pthread_msq_msg_t *rcvMsg);

#endif /* SMCOREDATAMNG_H_ */
