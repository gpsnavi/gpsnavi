/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_GROUP_SEARCH_H
#define SMCCOM_GROUP_SEARCH_H

// サークル情報
typedef struct _SMGROUP {
	Char		groupId[SCC_CMN_GROUP_ID];
	Char		name[SCC_CMN_GROUP_NAME];
	Char		discriptionBrief[SCC_CMN_DISCRIPTION_BRIEF];
	Char		access[SCC_MAX_ACCESS];
	Char		pictSmlUrl[SCC_MAX_URL];
	Char		pictMidUrl[SCC_MAX_URL];
	Char		pictLrgUrl[SCC_MAX_URL];
	INT32		memberNum;
	INT32		index;
} SMGROUP;

E_SC_RESULT CC_GroupSearch_SendRecv(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, INT32 type, const Char *groupName, INT32 offset, INT32 limit, SMGROUP *groupInfo, INT32 *groupInfoNum, Char *groupNum, Char *recv, INT32 recvBufSize, Char *apiStatus);

#endif // #ifndef SMCCOM_GROUP_SEARCH_H
