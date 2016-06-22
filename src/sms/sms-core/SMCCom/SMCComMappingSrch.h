/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_MAPPING_SRCH_H
#define SMCCOM_MAPPING_SRCH_H

#define	CC_MAPPINGSRCH_PARCEL_ID_NUM			36
#define	CC_MAPPINGSRCH_GROUP_ID_NUM				10

// マッピング検索条件
typedef struct _SMMAPPINGSRCH {
	Char			*mappingId;
	INT32			maxCnt;
	INT32			genreNum;
	INT32			genre[CC_CMN_MAPPING_GENRE_NUM];
	INT32			parcelIdNum;
	UINT32			parcelId[CC_MAPPINGSRCH_PARCEL_ID_NUM];
	INT32			accessFlg;
	INT32			followerFlg;
	Char			*groupId;
	Char			*keyword;
	INT32			rating;
	INT32			lastDatetm;
	INT32			sort;
} SMMAPPINGSRCH;

// マッピング検索結果
typedef struct _SMMAPPINGSRCHRES {
	DOUBLE			lat;
	DOUBLE			lon;
	UINT32			parcelId;
	INT32			x;
	INT32			y;
	Char			mappingId[CC_CMN_MAPPING_ID_SIZE];
	INT32			genre;
	Char			user[SCC_MAX_USERNAME];
	INT32			ownFlg;
	Char			guid[CC_CMN_GUID_STR_SIZE];
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
	Char			groupId[CC_CMN_MAPPING_GROUPID_SIZE];
	Char			groupName[CC_CMN_MAPPING_GROUPNAME_SIZE];
	LONG			commentCnt;
	Char			access[CC_CMN_MAPPING_ACCESS_SIZE];
	INT32			canDelete;
} SMMAPPINGSRCHRES;

E_SC_RESULT CC_MappingSrch_SendRecv(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, const SMMAPPINGSRCH *mappingSerch, SMMAPPINGSRCHRES *mappingInfo, INT32 *mappingInfoNum, LONG *allNum, INT32 *lastFlg, Char *recv, INT32 recvBufSize, Char *apiStatus);

#endif // #ifndef SMCCOM_MAPPING_SRCH_H
