/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_MESSAGE_PROCESS_H
#define SMCCOM_MESSAGE_PROCESS_H

E_SC_RESULT CC_MessageProcess_SendRecv(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, const INT32 *msgGuidList, INT32 msgGuidListNum, INT32 type, Char *recv, INT32 recvBufSize, Char *apiStatus);

#endif // #ifndef SMCCOM_MESSAGE_PROCESS_H
