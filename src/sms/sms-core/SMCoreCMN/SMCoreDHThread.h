/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCORE_DH_THREAD_H
#define SMCORE_DH_THREAD_H

//-----------------------------------
// 外部I/F定義
//-----------------------------------
E_SC_RESULT SC_DH_Initialize();
E_SC_RESULT SC_DH_Finalize();
void *SC_DH_ThreadMain(void *param);
void SC_DH_MsgDispatch(pthread_msq_msg_t *rcvMsg);

// TODO 探索用地図データ取得の同期関数追加

#endif // #ifndef SMCORE_DH_THREAD_H
