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
 * SMCoreDMApi.h
 *
 *  Created on: 2015/11/09
 *      Author: masutani
 */

#ifndef SMCOREDMAPI_H_
#define SMCOREDMAPI_H_

/**
 * SMCoreRTThread
 */
E_SC_RESULT SC_RT_Initialize();
E_SC_RESULT SC_RT_Finalize();
void *SC_RT_ThreadMain(void *param);

/**
 * SMCoreRGThread
 */
E_SC_RESULT SC_RG_Initialize();
E_SC_RESULT SC_RG_Finalize();
void *SC_RG_ThreadMain(void *param);

/**
 * SMDM
 */
/* @brief	メッセージ振り分け処理 */
void SC_RG_MsgAnalyze(pthread_msq_msg_t* aMsg);
/* @brief	メッセージ振り分け処理 */
void SC_RT_MsgAnalyze(pthread_msq_msg_t* aMsg);


#endif /* SMCOREDMAPI_H_ */
