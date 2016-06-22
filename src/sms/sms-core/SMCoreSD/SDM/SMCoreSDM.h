/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SC_SDM_H
#define SC_SDM_H

//---------------------------------------------------------------------------------
// 定数定義
//---------------------------------------------------------------------------------

//---------------------------------------------------------------------------------
// 構造体定義
//---------------------------------------------------------------------------------

//---------------------------------------------------------------------------------
// 外部I/F定義
//---------------------------------------------------------------------------------
E_SC_RESULT SC_SDM_Init(const Char *mapPath);
void SC_SDM_Final();
void SC_SDM_StartReq(const pthread_msq_msg_t *msg);
void SC_SDM_StartRes(const pthread_msq_msg_t *msg);
void SC_SDM_StopReq(const pthread_msq_msg_t *msg);
void SC_SDM_StopRes(const pthread_msq_msg_t *msg);
void SC_SDM_StartResSDD(const pthread_msq_msg_t *msg);
void SC_SDM_UploadReq(const pthread_msq_msg_t *msg);


#endif // #ifndef SC_SDM_H
