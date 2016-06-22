/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_PARKINGENTRANCE_REQ_H
#define SMCCOM_PARKINGENTRANCE_REQ_H

//---------------------------------------------------------------------------------
//インクルード
//---------------------------------------------------------------------------------

#define CC_PARKINGINFO_MAXNUM				100							// 駐車場情報最大件数【TBD】

//---------------------------------------------------------------------------------
//列挙型定義
//---------------------------------------------------------------------------------


//---------------------------------------------------------------------------------
//構造体定義
//---------------------------------------------------------------------------------
// ユーザ基本情報
typedef struct _SMPARKINGINFO {
	INT32			parkingId;
	Char			parkingName[CC_CMN_PARKING_PARKINGNAME];
	Char			entranceName[CC_CMN_PARKING_ENTRANCENAME];
	DOUBLE			entranceLat;
	DOUBLE			entranceLon;
	INT32			emptyRate;
	INT32			emptyRateGetdatetm;
	INT32			priority;
} SMPARKINGINFO;

typedef struct _SMSTOREPARKINGINFO {
	INT32			storeId;
	Char			storeName[CC_CMN_PARKING_STORENAME];
	INT32			parkingNum;
	SMPARKINGINFO	parking[CC_PARKINGINFO_MAXNUM];
} SMSTOREPARKINGINFO;


//---------------------------------------------------------------------------------
//外部関数宣言
//---------------------------------------------------------------------------------
E_SC_RESULT CC_ParkingrateReq_SendRecv(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, INT32 companyId, INT32 storeId, SMSTOREPARKINGINFO *parkingInfo, Char *recv, UINT32 recvBufSize, Char *apiStatus);

#endif // #ifndef SMCCOM_PARKINGENTRANCE_REQ_H
