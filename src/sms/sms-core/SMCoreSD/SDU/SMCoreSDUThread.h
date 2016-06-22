/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SC_SDU_THREAD_H
#define SC_SDU_THREAD_H

//-----------------------------------
// 外部I/F定義
//-----------------------------------
E_SC_RESULT SC_SDU_Initialize();
E_SC_RESULT SC_SDU_Finalize();
void *SC_SDU_ThreadMain(void *param);

#endif // #ifndef SC_SDU_THREAD_H
