/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_PKG_COMMON_H
#define SMCCOM_PKG_COMMON_H

#define	CC_PKGCOMMON_FRAMEWORK_ELLG			0
#define	CC_PKGCOMMON_FRAMEWORK_LARAVEL		1
#define	CC_PKGCOMMON_METHOD_GET				0
#define	CC_PKGCOMMON_METHOD_POST			1


typedef struct _SMPKGCOMMONPARAM {
	INT32	framework;
	Char	*apiName;
	INT32	method;
	Char	*param;
} SMPKGCOMMONPARAM;

E_SC_RESULT CC_PkgCommon_SendRecv(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, const SMPKGCOMMONPARAM *apiParam, const SMPKGWHITELIST *pkgWhiteList, UINT32 pkgWhiteListNum, Char *jsonFilePath, Char *recv, INT32 recvBufSize);

#endif // #ifndef SMCCOM_PKG_COMMON_H
