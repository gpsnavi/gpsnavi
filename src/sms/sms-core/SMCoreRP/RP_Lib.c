/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

/*-------------------------------------------------------------------
 * File：RP_Lib.c
 * Info：RMRC共通ライブラリ
 *-------------------------------------------------------------------*/

#include "SMCoreRPInternal.h"

/**
 * @brief 2点間距離取得
 * @param 地点1
 * @param 地点2
 * @return 2点間距離(m)
 */
DOUBLE RP_Lib_CalcODLength(SCRP_POINT aPointA, SCRP_POINT aPointB) {

	DOUBLE lat, lon, lat2, lon2;
	DOUBLE len;

	// 地点①
	SC_MESH_ChgParcelIDToTitude(RP_LEVEL1, aPointA.parcelId, (DOUBLE) aPointA.x, (DOUBLE) aPointA.y, &lat, &lon);
	// 地点②
	SC_MESH_ChgParcelIDToTitude(RP_LEVEL1, aPointB.parcelId, (DOUBLE) aPointB.x, (DOUBLE) aPointB.y, &lat2, &lon2);
	// 距離計算
	lat = lat / 3600;
	lon = lon / 3600;
	lat2 = lat2 / 3600;
	lon2 = lon2 / 3600;
	len = sc_MESH_GetRealLen((lat), (lon), (lat2), (lon2));

	return (len);
}

/**
 * 縦：横の比率を実長から取得する
 * @param aParcelId パーセルID
 * @param aDefault デフォルト値 地図データ取得時の失敗時はこの値を返却する
 * @return 縦÷横 の値
 */
FLOAT RP_Lib_GetParcelRealXYRatio(UINT32 aParcelId, FLOAT aDefault) {
	E_DHC_CASH_RESULT resultDhc;

	T_DHC_REQ_PARCEL req = {};
	T_DHC_RES_DATA resData = {};
	FLOAT ratio = 0;

	// 実長取得
	req.user = SC_DHC_USER_RP;
	req.parcelNum = 1;
	req.parcelInfo[0].parcelId = aParcelId;
	req.parcelInfo[0].mapKind = SC_DHC_GetKindMask(e_DATA_KIND_PARCEL_BASIS);

	resultDhc = SC_DHC_MapRead(&req, &resData);
	if (e_DHC_RESULT_CASH_SUCCESS == resultDhc && NULL != resData.parcelBin[0].binParcelBasis) {
		UINT32 x = SC_MA_D_BASIS_GET_X_BOTTOM(resData.parcelBin[0].binParcelBasis);
		UINT32 y = SC_MA_D_BASIS_GET_Y_LEFT(resData.parcelBin[0].binParcelBasis);

		ratio = ((FLOAT) y / (FLOAT) x);
		if (0 >= ratio) {
			ratio = aDefault;
		}

		if (e_DHC_RESULT_CASH_SUCCESS != SC_DHC_MapFree(&req)) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[Lib] SC_DHC_MapFree failed. "HERE);
		}
	} else {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[Lib] SC_DHC_MapRead failed. set Default %f "HERE, aDefault);
		ratio = aDefault;
	}

	return (ratio);
}
