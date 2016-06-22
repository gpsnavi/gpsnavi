/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_PKG_WHITELIST_H
#define SMCCOM_PKG_WHITELIST_H

E_SC_RESULT CC_Pkg_GetWhiteList(SMCAL *smcal, T_CC_CMN_SMS_API_PRM *parm, SMPACKAGEINFO *pkgInfo, SMPKGWHITELIST *pkgWhiteList, UINT32 *pkgWhiteListNum);

#endif // #ifndef SMCCOM_PKG_WHITELIST_H
