/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SC_PT_H
#define SC_PT_H

//-----------------------------------
// 外部I/F定義
//-----------------------------------
void SC_PT_MsgAnalyze(pthread_msq_msg_t *msg);
void SC_PT_TimerStop();

#endif // #ifndef SC_PT__H
