/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_DL_H
#define SMCCOM_DL_H

typedef enum _SMPKGDLDATATYPE {
	PKGDLDATATYPE_ICON = 0,
	PKGDLDATATYPE_PKG,
	PKGDLDATATYPE_OTHER
} SMPKGDLDATATYPE;

E_SC_RESULT CC_Download(SMCAL *smcal, T_CC_CMN_SMS_API_PRM *parm, const Char *awsFilePath, const Char *outFilePath, const SMPROGRESSCBFNC *dlCBFnc, UINT32 resumeStartPos, UINT32 resumeEndPos);
E_SC_RESULT CC_DownloadPackage(SMCAL *smcal, T_CC_CMN_SMS_API_PRM *parm, SMPKGDLDATATYPE dlDataType, const SMPACKAGEINFO *pkgInfo, const Char *dlDirPath);

#endif // #ifndef SMCCOM_DL_H
