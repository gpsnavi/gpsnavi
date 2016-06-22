/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_BACKUP_H
#define SMCCOM_BACKUP_H

E_SC_RESULT CC_SetBackupData(INT32 type, const Char *bkData, INT32 bkDataSize, T_CC_CMN_SMS_API_PRM *param);
E_SC_RESULT CC_GetBackupData(INT32 type, Char *bkData, INT32 *bkDataSize, const T_CC_CMN_SMS_API_PRM *param);

#endif // #ifndef SMCCOM_BACKUP_H
