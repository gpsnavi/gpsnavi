/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SC_SDT_H
#define SC_SDT_H

//-----------------------------------
// 外部I/F定義
//-----------------------------------
void SC_SDT_Init();
void SC_SDT_Final();
void SC_SDT_MsgAnalyze(const pthread_msq_msg_t *msg);
void SC_SDT_StartTimer(const pthread_msq_msg_t *msg);
void SC_SDT_StopTimer();
void SC_SDT_TimeoutRes();

#endif // #ifndef SC_SDT_H
