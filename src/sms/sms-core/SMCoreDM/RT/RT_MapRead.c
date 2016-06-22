/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

/*-----------------------------------------------------------------------------------------------*/
/* File：RT_MapDataGet.c                                                                         */
/* Info：地図データ取得                                                                          */
/*-----------------------------------------------------------------------------------------------*/

#include "sms-core/SMCoreDM/SMCoreDMInternal.h"

/**
 * @brief	地図データ要求テーブル設定
 */
E_SC_RESULT RT_MAP_SetReqTbl(UINT32 parcel_id, T_DHC_REQ_PARCEL *mapReqPcl, RT_MAPREQ_t *reqtbl)
{

	UINT32		base_id;
	UINT16		xlp;
	UINT16		ylp;
	UINT16		cnt = 0;

	if (NULL == mapReqPcl || NULL == reqtbl) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[MAP] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 要求テーブル初期化
	memset(reqtbl, 0x00, sizeof(RT_MAPREQ_t));

	// 矩形左下パーセルID算出
	base_id = SC_MESH_SftParcelId(parcel_id, -1, -1);
	// エリア内のパーセル情報読み込み
	for (ylp = 0 ; ylp < 3 ; ylp++) {
		for (xlp = 0 ; xlp < 3 ; xlp++, cnt++) {
			// 左下パーセルIDからXY方向先のパーセルID取得
			reqtbl->data[cnt].parcel_id = SC_MESH_SftParcelId(base_id, xlp, ylp);
			mapReqPcl->parcelInfo[cnt].parcelId = reqtbl->data[cnt].parcel_id;
			mapReqPcl->parcelInfo[cnt].mapKind = SC_DHC_GetKindMask(e_DATA_KIND_ROAD)
												| SC_DHC_GetKindMask(e_DATA_KIND_SHAPE)
												| SC_DHC_GetKindMask(e_DATA_KIND_GUIDE);
		}
	}
	reqtbl->data_vol = cnt;
	mapReqPcl->parcelNum = reqtbl->data_vol;

	return (e_SC_RESULT_SUCCESS);

}

/**
 * @brief	地図取得
 * @param	[I]パーセルID
 * @param	[I]地図種別
 */
E_SC_RESULT RT_MAP_DataRead(T_DHC_REQ_PARCEL *mapReqPcl, RT_MAPREQ_t *reqtbl)
{
#if 0
	E_SC_RESULT			ret;
#else
	E_DHC_CASH_RESULT	ret;
#endif
	T_DHC_RES_DATA		mapResData = {};
	UINT16				ilp;
	//static UINT8		*map = NULL;

	if (NULL == mapReqPcl || NULL == reqtbl) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[MAP] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 要求パラメータ設定
	mapReqPcl->user     = SC_DHC_USER_RG;

	// 地図取得
	ret = SC_DHC_MapRead(mapReqPcl, &mapResData);
	if (e_DHC_RESULT_CASH_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[MAP] SC_DHC_MapRead. ret(0x%x)"HERE, ret);
	} else {
		SC_LOG_DebugPrint(SC_TAG_RT, "[tblmake] Map read success. ");
	}

	// 部分的に地図データを取得している場合がある為、正常/異常にかかわらず値の設定を行う
	for (ilp = 0 ; ilp < reqtbl->data_vol ; ilp++) {
		reqtbl->data[ilp].road_p = mapResData.parcelBin[ilp].binRoad;
		reqtbl->data[ilp].sharp_p = mapResData.parcelBin[ilp].binShape;
		reqtbl->data[ilp].guide_p = mapResData.parcelBin[ilp].binGuide;
		reqtbl->data[ilp].basis_p = mapResData.parcelBin[ilp].binParcelBasis;
		SC_LOG_DebugPrint(SC_TAG_RT, "[tblmake] parcelId[0x%08x]", reqtbl->data[ilp].parcel_id);
		SC_LOG_DebugPrint(SC_TAG_RT, "[tblmake]   Kind[0x%x] Addr[%08x]", e_DATA_KIND_ROAD, reqtbl->data[ilp].road_p);
		SC_LOG_DebugPrint(SC_TAG_RT, "[tblmake]   Kind[0x%x] Addr[%08x]", e_DATA_KIND_SHAPE, reqtbl->data[ilp].sharp_p);
		SC_LOG_DebugPrint(SC_TAG_RT, "[tblmake]   Kind[0x%x] Addr[%08x]", e_DATA_KIND_GUIDE, reqtbl->data[ilp].guide_p);
		SC_LOG_DebugPrint(SC_TAG_RT, "[tblmake]   Kind[0x%x] Addr[%08x]", e_DATA_KIND_PARCEL_BASIS, reqtbl->data[ilp].basis_p);
	}

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief	地図開放
 * @param	[I]パーセルID
 * @param	[I]地図種別
 */
E_SC_RESULT RT_MAP_DataFree(RT_MAPREQ_t *reqtbl)
{
#if 0
	E_SC_RESULT			ret;
#else
	E_DHC_CASH_RESULT	ret;
#endif
	T_DHC_REQ_PARCEL	aReqPcl;
	UINT16				ilp;

	if (NULL == reqtbl) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[MAP] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 要求パラメータ設定
	aReqPcl.user = SC_DHC_USER_RG;
	aReqPcl.parcelNum = reqtbl->data_vol;
	for (ilp = 0 ; ilp < reqtbl->data_vol ; ilp++) {
		aReqPcl.parcelInfo[ilp].parcelId = reqtbl->data[ilp].parcel_id;
		aReqPcl.parcelInfo[ilp].mapKind = 0;

		// ネットワークデータ
		if (NULL != reqtbl->data[ilp].road_p) {
			aReqPcl.parcelInfo[ilp].mapKind = aReqPcl.parcelInfo[ilp].mapKind | SC_DHC_GetKindMask(e_DATA_KIND_ROAD);
		}

		// 道路形状データ
		if (NULL != reqtbl->data[ilp].sharp_p) {
			aReqPcl.parcelInfo[ilp].mapKind = aReqPcl.parcelInfo[ilp].mapKind | SC_DHC_GetKindMask(e_DATA_KIND_SHAPE);
		}

		// 誘導データ
		if (NULL != reqtbl->data[ilp].guide_p) {
			aReqPcl.parcelInfo[ilp].mapKind = aReqPcl.parcelInfo[ilp].mapKind | SC_DHC_GetKindMask(e_DATA_KIND_GUIDE);
		}

		// パーセル基本情報データ
		if (NULL != reqtbl->data[ilp].basis_p) {
			aReqPcl.parcelInfo[ilp].mapKind = aReqPcl.parcelInfo[ilp].mapKind | SC_DHC_GetKindMask(e_DATA_KIND_PARCEL_BASIS);
		}

	}

	// 地図開放
	ret = SC_DHC_MapFree(&aReqPcl);
	if (e_DHC_RESULT_CASH_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[MAP] SC_DHC_MapFree. ret(0x%x)"HERE, ret);
	}

	return (e_SC_RESULT_SUCCESS);
}

