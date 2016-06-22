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
 * SMCoreRPApi.h
 *
 *  Created on: 2015/11/06
 *      Author: masutani
 */

#ifndef SMCORERPAPI_H_
#define SMCORERPAPI_H_

#ifdef __cplusplus
extern "C" {
#endif
/**
 * SCRCThread
 */
E_SC_RESULT SC_RC_Initialize();
E_SC_RESULT SC_RC_Finalize();
void *SC_RC_ThreadMain(void *param);

/**
 * SCRMThread
 */
E_SC_RESULT SC_RM_Initialize();
E_SC_RESULT SC_RM_Finalize();
void *SC_RM_ThreadMain(void *param);

/**
 * SMRP
 */
/* メッセージ振り分け処理 */
void SC_RM_MsgAnalyze(pthread_msq_msg_t* aMsg);
/* メッセージ振り分け処理 */
void SC_RC_MsgAnalyze(pthread_msq_msg_t* aMsg);
/* 経路参照登録IF */
E_SC_RESULT SC_RP_ReadRouteExit(UINT32 aRtId, UINT32 aRtType, UINT32 aUser);
/* 経路参照解除IF */
E_SC_RESULT SC_RP_ReadRouteEntry(UINT32 aRtId, UINT32 aRtType, UINT32 aUser, SC_RP_RouteMng** aRtBin);
/* 経路ID・種別取得IF */
E_SC_RESULT SC_RP_GetCurrentRouteId(UINT32* aRtId, UINT32* aRtType);
#ifdef __cplusplus
}
#endif

#endif /* SMCORERPAPI_H_ */
