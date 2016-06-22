/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCORE_FM_THREAD_H
#define SMCORE_FM_THREAD_H

//-----------------------------------
// 外部I/F定義
//-----------------------------------
void SC_FM_ResRoute(const pthread_msq_msg_t *rcvMsg);
void SC_FM_ResGuide(const pthread_msq_msg_t *rcvMsg);
void SC_FM_ResAlert(const pthread_msq_msg_t *rcvMsg);
void SC_FM_ResChat(const pthread_msq_msg_t *rcvMsg);
void SC_FM_ResPosInfo(const pthread_msq_msg_t *rcvMsg);
void SC_FM_ResReRoute(const pthread_msq_msg_t *rcvMsg);
void SC_FM_ResStartDrive(const pthread_msq_msg_t *rcvMsg);
void SC_FM_ResStopDrive(const pthread_msq_msg_t *rcvMsg);

#endif // #ifndef SMCORE_FM_THREAD_H
