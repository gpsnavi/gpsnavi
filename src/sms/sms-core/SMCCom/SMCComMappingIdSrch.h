/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_MAPPING_ID_SRCH_H
#define SMCCOM_MAPPING_ID_SRCH_H

// マッピングID検索結果
typedef struct _SMMAPPINGIDSRCHRES {
	DOUBLE			lat;
	DOUBLE			lon;
	UINT32			parcelId;
	INT32			x;
	INT32			y;
	Char			mappingId[CC_CMN_MAPPING_ID_SIZE];
	INT32			genre;
	Char			user[SCC_MAX_USERNAME];
	Char			avtSmlUrl[CC_CMN_MAPPING_URL_SIZE];
	Char			avtMidUrl[CC_CMN_MAPPING_URL_SIZE];
	Char			avtLrgUrl[CC_CMN_MAPPING_URL_SIZE];
	Char			text[CC_CMN_MAPPING_TEXT_SIZE];
	Char			pictUrl[SCC_MAX_PATH];
	INT32			mode;
	INT32			mappingDatetm;
	INT32			rgstDatetm;
	INT32			altDatetm;
	INT32			lastDatetm;
	LONG			likeCnt;
	INT32			likeFlg;
	LONG			ratingnumYes;
	LONG			ratingnumNo;
	INT32			ratingOwn;
	Char			groupName[CC_CMN_MAPPING_GROUPNAME_SIZE];
	LONG			commentCnt;
	Char			access[CC_CMN_MAPPING_ACCESS_SIZE];
	INT32			canDelete;
	INT32			ownFlg;
	Char			guid[CC_CMN_GUID_STR_SIZE];
	Char			groupId[CC_CMN_MAPPING_GROUPID_SIZE];
} SMMAPPINGIDSRCHRES;

E_SC_RESULT CC_MappingIdSrch_SendRecv(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, const Char *mappingId, SMMAPPINGIDSRCHRES *mappingInfo,Char *recv, INT32 recvBufSize, Char *apiStatus);

#endif // #ifndef SMCCOM_MAPPING_ID_SRCH_H
