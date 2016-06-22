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
 * SMCoreTRApi.h
 *
 *  Created on: 2016/01/04
 *      Author: n.kanagawa
 */

#ifndef SMCORETRAPI_H_
#define SMCORETRAPI_H_


#ifdef __cplusplus
extern "C" {
#endif

/**
 * SMCoreTRThread
 */
E_SC_RESULT SC_TR_Initialize();
E_SC_RESULT SC_TR_Finalize();
void *SC_TR_ThreadMain(void *param);
void SC_TR_MsgAnalyze(pthread_msq_msg_t* aMsg);

void SC_TR_SendCarposUpdateMsg();
void SC_TR_SendScrollUpdateMsg();
//void SC_TR_SendManualUpdateMsg();
//void SC_TR_SendTimerUpdateMsg();

Bool SC_TR_LockCacheTbl();
Bool SC_TR_UnLockCacheTbl();
TR_CONGESTION_INFO_t *SC_TR_GetCongestion(UINT32 parcelId);

/**
 * SMCoreTRTThread
 */
E_SC_RESULT SC_TRT_Initialize();
E_SC_RESULT SC_TRT_Finalize();
void *SC_TRT_ThreadMain(void *param);
void SC_TRT_MsgAnalyze(const pthread_msq_msg_t *msg);

void SC_TRT_SendStratTimerMsg(void *sender, INT32 timer);
void SC_TRT_StopTimer();

#ifdef __cplusplus
}
#endif

#endif /* SMCORETRAPI_H_ */
