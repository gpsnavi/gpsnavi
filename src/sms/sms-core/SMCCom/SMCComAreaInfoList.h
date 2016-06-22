/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_AREAINFOLIST_H
#define SMCCOM_AREAINFOLIST_H

//---------------------------------------------------------------------------------
//インクルード
//---------------------------------------------------------------------------------


//---------------------------------------------------------------------------------
//列挙型定義
//---------------------------------------------------------------------------------


//---------------------------------------------------------------------------------
//構造体定義
//---------------------------------------------------------------------------------


//---------------------------------------------------------------------------------
//外部関数宣言
//---------------------------------------------------------------------------------
E_SC_RESULT CC_GetAreaInfoList(SMCAL *smcal, T_CC_CMN_SMS_API_PRM *parm, const Char *rgn_code, SMSECINFO *sectInf, INT32 *sectNum, INT32 *areaNum, const Char *dlTempDirPath, const SMMAPDLCBFNC *dlCBFnc);
E_SC_RESULT CC_GetUpdDataInfo(SMCAL *smcal, const T_CC_CMN_SMS_API_PRM *parm, const SMUPDINFO *mapUpdInfo, Bool *hasUpdate, const Char *dlTempDirPath, SCC_UPDATEDATA *updData, SCC_UPDATEDATA *updAppData);
E_SC_RESULT CC_GetAreaInfoList_CheckVersion(const SCC_UPDATEDATA *upddata, DBOBJECT *map_db, INT32 areaCode, upDTcheck_States *updateStatus);
// TODO:TR5.0暫定　広域地図の更新がある場合は、エリア地図の更新は必須とする
void  CC_SetIsRegionMapUpdate(Bool isResionMapUpdate);
// TODO:TR5.0暫定　広域地図の更新がある場合は、エリア地図の更新は必須とする
#endif // #ifndef SMCCOM_AREAINFOLIST_H
