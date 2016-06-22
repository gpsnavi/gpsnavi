/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_PROBEPOST_REQ_H
#define SMCCOM_PROBEPOST_REQ_H

E_SC_RESULT CC_ProbePostReq_SendRecv(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, SMAWSINFO *aws, Char *recv, INT32 recvBufSize, Char *apiStatus);

#endif // #ifndef SMCCOM_PROBEPOST_REQ_H
