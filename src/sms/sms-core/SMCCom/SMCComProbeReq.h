/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_PROBE_REQ_H
#define SMCCOM_PROBE_REQ_H

// プローブ情報検索条件
typedef struct _SMPROBESRCH {
	Char			*fromTime;
	Char			*toTime;
} SMPROBESRCH;

// プローブ情報
typedef struct _SMPROBEINFO {
	Bool			gpsLatFlg;
	Bool			gpsLonFlg;
	Bool			mapLatFlg;
	Bool			mapLonFlg;
	DOUBLE			gpsLat;
	DOUBLE			gpsLon;
	DOUBLE			mapLat;
	DOUBLE			mapLon;
	LONG			parcelId;
	Char			time[CC_CMN_PROBEINFO_TIME + 1];
	Char			reserve;
} SMPROBEINFO;

// プローブ情報
typedef struct _SMPROBEINFONUM {
	INT32			probeInfoNum;
} SMPROBEINFONUM;

E_SC_RESULT CC_ProbeReq_SendRecv(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, const SMPROBESRCH *probeSrch, SMPROBEINFO *probeInfo, SMPROBEINFONUM *probeInfoNum, Char *recv, INT32 recvBufSize, Char *apiStatus);

#endif // #ifndef SMCCOM_PROBE_REQ_H
