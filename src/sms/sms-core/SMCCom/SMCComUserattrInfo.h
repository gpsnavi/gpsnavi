/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_USERATTR_INFO_H
#define SMCCOM_USERATTR_INFO_H

typedef struct _SMUSERPROFILE {
	Char	userName[CC_CMN_USERNAME_STR_SIZE];
	Char	profile[CC_CMN_PROFILE_STR_SIZE];
	Char	comment[CC_CMN_COMMENT_STR_SIZE];
	Char	address[CC_CMN_ADDRESS_STR_SIZE];
	Char	hobby[CC_CMN_HOBBY_STR_SIZE];
	Char	specialSkill[CC_CMN_SKIL_STR_SIZE];
	Char	mail[CC_CMN_MALEADDR_STR_SIZE];
	Char	avatar[CC_CMN_AVATART_STR_SIZE];
} SMUSERPROFILE;

E_SC_RESULT CC_UserattrInfo_SendRecv(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, const Char *srchGuid, SMUSERPROFILE *userInfo, Char *recv, UINT32 recvBufSize, Char *apiStatus, Bool isPolling);

#endif // #ifndef SMCCOM_USERATTR_INFO_H
