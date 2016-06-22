/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_POSINFO_SEARCH_H
#define SMCCOM_POSINFO_SEARCH_H

E_SC_RESULT CC_PosInfoSearch_SendRecv(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, const Char *roomNo, SMPOSITIONINFO *posInfo, INT32 *posInfoNum, const Char *fileName, Char *recv, UINT32 recvBufSize, Char *apiStatus, Bool isPolling);

#endif // #ifndef SMCCOM_POSINFO_SEARCH_H
