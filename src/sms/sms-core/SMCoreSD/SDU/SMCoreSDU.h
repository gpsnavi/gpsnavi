/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SC_SDU_H
#define SC_SDU_H

//-----------------------------------
// 外部I/F定義
//-----------------------------------
E_SC_RESULT SC_SDU_Init();
void SC_SDU_Final();
void SC_SDU_StartReq(const pthread_msq_msg_t *msg);
void SC_SDU_StopReq(const pthread_msq_msg_t *msg);

void SC_SDU_UploadReq(const pthread_msq_msg_t *msg);
void SC_SDU_UploadStopReq(const pthread_msq_msg_t *msg);
void SC_SDU_SendUploadResult(E_SC_RESULT result);
void SC_SDU_SendUploadStopResult(E_SC_RESULT result, const Char *startTime);

#endif // #ifndef SC_SDU_H
