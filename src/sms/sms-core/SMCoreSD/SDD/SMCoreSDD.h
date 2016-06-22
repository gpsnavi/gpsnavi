/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SC_SDD_H
#define SC_SDD_H

//---------------------------------------------------------------------------------
// 定数定義
//---------------------------------------------------------------------------------


//---------------------------------------------------------------------------------
// 構造体定義
//---------------------------------------------------------------------------------


//---------------------------------------------------------------------------------
// 外部I/F定義
//---------------------------------------------------------------------------------
E_SC_RESULT SC_SDD_Init();
void SC_SDD_Final();
void SC_SDD_StartReq(const pthread_msq_msg_t *msg);
void SC_SDD_StopReq(const pthread_msq_msg_t *msg);
void SC_SDD_SensorDataRecv(const pthread_msq_msg_t *msg);

#endif // #ifndef SC_SDD_H
