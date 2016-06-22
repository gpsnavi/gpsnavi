/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_OAUTHULRREQ_H
#define SMCCOM_OAUTHURLREQ_H

typedef struct _T_CC_OAUTH_URL {
	Char		url[CC_CMN_OAUTH_URL_SIZE];
	Char		sessionId[CC_CMN_OAUTH_SESSION_ID_SIZE];
	Char		reserve[2];
} T_CC_OAUTH_URL;


//---------------------------------------------------------------------------------
//外部関数宣言
//---------------------------------------------------------------------------------
E_SC_RESULT CC_OAuthUrlReq_SendRecv(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, const Char *serviceId, Char *url, Char *sessionId, Char *recv, INT32 recvBufSize, Char *apiStatus);

#endif // #ifndef SMCCOM_OAUTHURLREQ_H
