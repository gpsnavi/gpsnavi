/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SC_PU_UPLOAD_H
#define SC_PU_UPLOAD_H

//-----------------------------------
// 外部I/F定義
//-----------------------------------
E_SC_RESULT SC_PU_Upload_Initialize();
E_SC_RESULT SC_PU_Upload_Finalize();
void SC_PU_Upload(pthread_msq_msg_t *msg);
//void SC_PU_Download(pthread_msq_msg_t *msg);
void SC_PU_SendUploadResult(E_SC_RESULT result);

#endif // #ifndef SC_PU_UPLOAD_H
