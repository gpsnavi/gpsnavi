/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_UPD_DATA_REQ_H
#define SMCCOM_UPD_DATA_REQ_H

E_SC_RESULT CC_UpdDataReq(SMCAL *smcal, T_CC_CMN_SMS_API_PRM *apiParam, const SMUPDINFO *mapUpdInfo, INT32 appDataVersion, const Char *dlTempDirPath, const SCC_UPDATEDATA *updData, UINT32 updDataNum, UINT32 updDataNumProgress, SMMAPUPDSTATUS *updStatus, const SMMAPDLCBFNC *callbackFnc);

#endif // #ifndef SMCCOM_UPD_DATA_REQ_H
