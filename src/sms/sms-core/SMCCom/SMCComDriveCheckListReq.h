/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_DRIVE_CHECK_LIST_REQ_H
#define SMCCOM_DRIVE_CHECK_LIST_REQ_H

#define CC_DRIVECHECK_LIMIT				100

E_SC_RESULT CC_DriveCheckListReq_SendRecv(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, const Char *tripId, INT32 limit, Char *filePath, Char *recv, INT32 recvBufSize);

#endif // #ifndef SMCCOM_DRIVE_CHECK_LIST_REQ_H
