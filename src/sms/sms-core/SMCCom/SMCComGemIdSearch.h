/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_GEMID_SEARCH_H
#define SMCCOM_GEMID_SEARCH_H

E_SC_RESULT CC_GemIdSearch_SendRecv(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, const Char *gemId, SMGEMINFO *gemInfo, Char *recv, INT32 recvBufSize, Char *apiStatus);

#endif // #ifndef SMCCOM_GEMID_SEARCH_H
