/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#include "sms-core/internal/smsstd.h"
#include "sms-core/internal/smstypedef.h"

#include "navicore.h"

#include "sms-core/SMCoreLC/SMCoreLCApi.h"

static NC_LOCATORCBFUNCPTR mCBFunc = NULL;

/**
 * ロケータ位置情報更新コールバック設定処理
 */
E_SC_RESULT LC_SetLocationUpdateCallback(NC_LOCATORCBFUNCPTR pFunc) {
	mCBFunc = pFunc;
	return (e_SC_RESULT_SUCCESS);
}

/**
 * ロケータ位置情報更新コールバック
 */
void LC_LocationUpdateCallback()
{
	if (NULL == mCBFunc) {
		return;
	}
	(mCBFunc)();
}

/**
 * @brief	初期化
 * @return	処理結果(正常:true、異常:false)
 */
Bool SC_SensorData_Initialize()
{
	Bool	ret = true;

	return (ret);
}

/**
 * @brief	終了化
 * @return	処理結果(正常:true、異常:false)
 */
Bool SC_SensorData_Finalize()
{
	Bool	ret = true;

	return (ret);
}
