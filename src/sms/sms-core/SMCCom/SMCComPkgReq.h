/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_PKG_CODE_REQ_H
#define SMCCOM_PKG_CODE_REQ_H

E_SC_RESULT CC_PkgReq_SendRecv(SMCAL *smcal, T_CC_CMN_SMS_API_PRM *parm, const Char *dirPath, Bool isDownload, SMPACKAGEGROUPINFO *pkgInfo, INT32 *pkgInfoNum, Char *recv, INT32 recvBufSize, Char *apiStatus);

#endif // #ifndef SMCCOM_PKG_CODE_REQ_H
