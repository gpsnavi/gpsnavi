/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_RGNINFOLIST_H
#define SMCCOM_RGNINFOLIST_H

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
E_SC_RESULT CC_GetRgnInfoList(SMCAL *smcal, T_CC_CMN_SMS_API_PRM *parm, SMRGNINFO *rgnI, INT32 *rgnNum, const Char *dlTempDirPath, const SMMAPDLCBFNC *callbackFnc);

#endif // #ifndef SMCCOM_RGNINFOLIST_H
