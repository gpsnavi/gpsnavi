/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_CHKMAPUPD_H
#define SMCCOM_CHKMAPUPD_H

//---------------------------------------------------------------------------------
//インクルード
//---------------------------------------------------------------------------------


//---------------------------------------------------------------------------------
//列挙型定義
//---------------------------------------------------------------------------------


//---------------------------------------------------------------------------------
//構造体定義
//---------------------------------------------------------------------------------
// データ更新チェック情報
typedef struct _SMCHECKUPDINFO {
	Char		regionCode[SCC_MAPDWL_MAXCHAR_RGNCODE];
	Char		path[SCC_MAPDWL_MAXCHAR_FLDPATH];
	INT32		updateStatus;
	INT32		tempFileSize;
	INT32		importSize;
	Bool		hasUpdate[SCC_MAPDWL_KIND_NUM];
	INT32		version[SCC_MAPDWL_KIND_NUM];
} SMCHECKUPDINFO;


//---------------------------------------------------------------------------------
//外部関数宣言
//---------------------------------------------------------------------------------
E_SC_RESULT CC_CheckUpdate(SMCAL *smcal, T_CC_CMN_SMS_API_PRM *parm, SMCHECKUPDINFO *chkUpdInf, const Char *dlTempDirPath, const SMMAPDLCBFNC *dlCBFnc, Bool errComeback);

#endif // #ifndef SMCCOM_CHKMAPUPD_H
