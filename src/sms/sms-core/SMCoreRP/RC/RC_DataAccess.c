/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

/*
 * RC_DataAccess.c
 *
 *  Created on: 2014/06/05
 *      Author: 70251034
 */

#include "sms-core/SMCoreRP/SMCoreRPInternal.h"

/* ダウンロードエリア管理情報 */
static T_DHC_DOWNLOAD_AREA mDownLoadArea = {};

#if 1	/* AIKAWA */
static T_DHC_REQ_PARCEL sRPMapReqTab = {};		/* 探索内で唯一の地図リクエスト用変数 */
static T_DHC_RES_DATA sRPMapResTab = {};			/* 探索内で唯一の地図応答用変数 */
#endif

/**
 * @brief ダウンロードエリア管理情報取り込み
 * @note 探索開始時のダウンロードエリア管理情報を地図データから取得し、グローバル領域保管
 */
E_SC_RESULT RC_SetDownLoad_Area() {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_DHC_CASH_RESULT cashResult = e_DHC_RESULT_CASH_SUCCESS;

	// ダウンロードエリア管理情報取得(パーセル指定)
	cashResult = SC_DHC_GetDownload_Area(&mDownLoadArea, M_DHC_DOWNLOAD_AREA_PARCEL);
	if (e_DHC_RESULT_CASH_SUCCESS != cashResult) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "SC_DHC_GetDownload_Area error. [0x%08x] "HERE, cashResult);
		return (e_SC_RESULT_FAIL);
	}
#if _RP_LOG_DOWNLOADAREA
	SC_LOG_DebugPrint(SC_TAG_RC, "download area dump.. "HERE);
	UINT32 i;
	for (i = 0; i < M_DHC_DOWNLOAD_AREA_MAX; i++) {
		SC_LOG_DebugPrint(SC_TAG_RC, "[%3d] download=%d ", i, mDownLoadArea.data[i].download_f);
	}
#endif

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief ダウンロードエリア管理情報アドレス取得
 */
T_DHC_DOWNLOAD_AREA *RC_GetDownLoad_Area() {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (&mDownLoadArea);
}

/**
 * @brief ユーザ指定地図全開放
 */
void RC_MapFreeAll() {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	T_DHC_REQ_INFO mapReq = { SC_DHC_USER_RP, 0, 0 };
	E_DHC_CASH_RESULT cashResult = SC_DHC_MapFreeEx(mapReq, SC_DHC_MAPFREE_USER);
	if (e_DHC_RESULT_CASH_SUCCESS != cashResult) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "SC_DHC_MapFreeEx error. [0x%08x] "HERE, cashResult);
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
}

/**
 * @brief 探索対象エリア地図一括読み込み
 * @param 地図読みエリア
 * @param [O]読み込み結果格納領域
 */
E_SC_RESULT RC_ReadAreaMap(SCRP_PCLRECT* aPclRect, UINT32 aMapKind, SCRP_MAPREADTBL* aReadTbl) {
//	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aReadTbl || NULL == aPclRect) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	UINT32 reqVol = 0;
	UINT32 index = 0;
	UINT32 x, y, i;

	do {
		// 要求数
		reqVol = aPclRect->xSize * aPclRect->ySize;
		if (0 == reqVol || RP_DATA_REQ_MAX < reqVol) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. reqVol=%d "HERE, reqVol);
			result = e_SC_RESULT_BADPARAM;
			break;
		}
		// 作業領域確保
		aReadTbl->mapVol = reqVol;
		aReadTbl->mapList = RP_MemAlloc(sizeof(SCRP_MAPDATA) * reqVol, e_MEM_TYPE_ROUTEPLAN);
		if (NULL == aReadTbl->mapList) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error. "HERE);
			result = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		// 0クリア
		RP_Memset0(aReadTbl->mapList, sizeof(SCRP_MAPDATA) * reqVol);
		// 要求情報作成
		for (y = 0; y < aPclRect->ySize; y++) {
			for (x = 0; x < aPclRect->xSize; x++) {
				UINT32 pclid = SC_MESH_SftParcelId(aPclRect->parcelId, x, y);
				sRPMapReqTab.parcelInfo[index].parcelId = pclid;
				sRPMapReqTab.parcelInfo[index].mapKind = aMapKind;
				// ついでにパーセルIDを返却用テーブルへ格納
				(aReadTbl->mapList + index)->parcelId = pclid;
				index++;
			}
		}
		sRPMapReqTab.user = SC_DHC_USER_RP;
		sRPMapReqTab.parcelNum = reqVol;

		// 地図要求
		E_DHC_CASH_RESULT cashResult = SC_DHC_MapRead(&sRPMapReqTab, &sRPMapResTab);
		if (e_DHC_RESULT_CASH_SUCCESS != cashResult) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "SC_DHC_MapRead error. [0x%08x] "HERE, cashResult);
			result = e_SC_RESULT_FAIL;
			break;
		}
		// 道路ネットワーク
		if (SC_DHC_KIND_ROAD & aMapKind) {
			for (i = 0; i < sRPMapResTab.parcelNum; i++) {
				aReadTbl->mapList[i].road = (MAL_HDL) sRPMapResTab.parcelBin[i].binRoad;
			}
		}
		// 形状データ
		if (SC_DHC_KIND_SHAPE & aMapKind) {
			for (i = 0; i < sRPMapResTab.parcelNum; i++) {
				aReadTbl->mapList[i].shape = (MAL_HDL) sRPMapResTab.parcelBin[i].binShape;
			}
		}
	} while (0);

//	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief 探索対象エリア地図一括読み込み
 * @param パーセルIDリスト
 * @param パーセルIDリスト数
 * @param 地図要求種別
 * @param [O]読み込み結果格納領域
 * @memo 地図読み込み結果格納領域内のデータリストの領域は呼び元で解放すること
 */
E_SC_RESULT RC_ReadListMap(UINT32* aPclList, UINT32 aListVol, UINT32 aMapKind, SCRP_MAPREADTBL* aReadTbl) {
//	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aPclList || NULL == aReadTbl) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	UINT32 i;

	do {
		// 要求数チェック
		if (0 == aListVol || RP_DATA_REQ_MAX < aListVol) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. aListVol=%d "HERE, aListVol);
			result = e_SC_RESULT_BADPARAM;
			break;
		}
		aReadTbl->mapVol = aListVol;
		aReadTbl->mapList = RP_MemAlloc(sizeof(SCRP_MAPDATA) * aListVol, e_MEM_TYPE_ROUTEPLAN);
		if (NULL == aReadTbl->mapList) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error. "HERE);
			result = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		// 0クリア
		RP_Memset0(aReadTbl->mapList, sizeof(SCRP_MAPDATA) * aListVol);
		// 要求情報作成
		for (i = 0; i < aListVol; i++) {
			sRPMapReqTab.parcelInfo[i].parcelId = *(aPclList + i);
			sRPMapReqTab.parcelInfo[i].mapKind = aMapKind;
			// ついでにパーセルIDを返却用テーブルへ格納
			(aReadTbl->mapList + i)->parcelId = *(aPclList + i);
		}
		sRPMapReqTab.user = SC_DHC_USER_RP;
		sRPMapReqTab.parcelNum = aListVol;

		// 地図取得要求
		E_DHC_CASH_RESULT cashResult = SC_DHC_MapRead(&sRPMapReqTab, &sRPMapResTab);
		if (e_DHC_RESULT_CASH_SUCCESS != cashResult) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "SC_DHC_MapRead error. [0x%08x] "HERE, cashResult);
			result = e_SC_RESULT_MAP_GETERR;
			break;
		}
		// 道路ネットワーク
		if (SC_DHC_KIND_ROAD & aMapKind) {
			for (i = 0; i < sRPMapResTab.parcelNum; i++) {
				aReadTbl->mapList[i].road = (MAL_HDL) sRPMapResTab.parcelBin[i].binRoad;
			}
		}
		// 形状データ
		if (SC_DHC_KIND_SHAPE & aMapKind) {
			for (i = 0; i < sRPMapResTab.parcelNum; i++) {
				aReadTbl->mapList[i].shape = (MAL_HDL) sRPMapResTab.parcelBin[i].binShape;
			}
		}
		// パーセル基本情報
		if (SC_DHC_KIND_PARCEL_BASIS & aMapKind) {
			for (i = 0; i < sRPMapResTab.parcelNum; i++) {
				aReadTbl->mapList[i].basis = (MAL_HDL) sRPMapResTab.parcelBin[i].binParcelBasis;
			}
		}
	} while (0);

	// メモリ解放
	if (e_SC_RESULT_MAP_GETERR == result) {
		RP_MemFree(aReadTbl->mapList, e_MEM_TYPE_ROUTEPLAN);
		aReadTbl->mapList = NULL;
		aReadTbl->mapVol = 0;
	}

//	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief 探索対象エリア地図解放処理
 * @param 解放対象地図テーブル
 * @memo リストメモリ解放は行わない
 */
E_SC_RESULT RC_FreeMapTbl(SCRP_MAPREADTBL* aReadTbl, Bool aMemfree) {
//	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aReadTbl) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	UINT32 i;

	do {
		// 要求数チェック
		if (0 == aReadTbl->mapVol || NULL == aReadTbl->mapList || RP_DATA_REQ_MAX < aReadTbl->mapVol) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. vol=%d "HERE, aReadTbl->mapVol);
			result = e_SC_RESULT_BADPARAM;
			break;
		}
		// 要求情報作成
		for (i = 0; i < aReadTbl->mapVol; i++) {
			sRPMapReqTab.parcelInfo[i].parcelId = (aReadTbl->mapList + i)->parcelId;
			// データが存在する種別を詰める
			UINT32 kind = 0;
			if (NULL != (aReadTbl->mapList + i)->road) {
				kind |= SC_DHC_GetKindMask(e_DATA_KIND_ROAD);
			}
			if (NULL != (aReadTbl->mapList + i)->shape) {
				kind |= SC_DHC_GetKindMask(e_DATA_KIND_SHAPE);
			}
			if (NULL != (aReadTbl->mapList + i)->basis) {
				kind |= SC_DHC_GetKindMask(e_DATA_KIND_PARCEL_BASIS);
			}
			sRPMapReqTab.parcelInfo[i].mapKind = kind;
		}
		sRPMapReqTab.user = SC_DHC_USER_RP;
		sRPMapReqTab.parcelNum = aReadTbl->mapVol;

		// 地図解放要求
		E_DHC_CASH_RESULT cashResult = SC_DHC_MapFree(&sRPMapReqTab);
		if (e_DHC_RESULT_CASH_SUCCESS != cashResult) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "SC_DHC_MapFree error. [0x%08x] "HERE, cashResult);
			result = e_SC_RESULT_FAIL;
			break;
		}
	} while (0);

	if (aMemfree) {
		if (NULL != aReadTbl->mapList) {
			RP_MemFree(aReadTbl->mapList, e_MEM_TYPE_ROUTEPLAN);
			aReadTbl->mapList = NULL;
			aReadTbl->mapVol = 0;
		}
	}

//	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}
