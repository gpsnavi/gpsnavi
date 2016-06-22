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
 * RP_Area.c
 *
 *  Created on: 2015/11/06
 *      Author: masutani
 */

#include "sms-core/SMCoreRP/SMCoreRPInternal.h"
#if _SCMPDBG_SHOWROUTEAREA
#include "sms-core/SMCoreMP/SCMPDBG_DrawRouteArea.h"
#endif

/*-------------------------------------------------------------------
 * 定数定義
 *-------------------------------------------------------------------*/
#define LV1ODBASE_SIDEPCL_MAX			(6)						/* Lv1基準エリア：一辺最大数 */
#define LV1ODBASE_DENSITY_MIN			(3000)					/* Lv1基準エリア：保障密度 */
#define LV1ODBASE_UPCNT_NORMAL			(2)						/* Lv1基準エリア：拡張保障回数 */
#define LV1ODBASE_UPCNT_HIGHWAY			(4)						/* Lv1基準エリア：拡張保障回数 出発リンク高速道路 */
#define LV1ODBASE_REAL_Y_DEF			(0.816f)				/* パーセル縦横比取得失敗時のデフォルト値（東京の実長比にしています） */

#if _RP_USE_OLDAREA /* 旧エリア処理（Lv1のみに対応）*/

#define AREATOP_RATIO_RANGE				(70)					/* トップエリア 縦横比値(*100) */
#define LV1TOP_UPCNT_MAX				(25)					/* 拡張回数制限 65km程度 */
#define LV1TOP_DENSITY_MIN				(512 * 36)				// レベル１トップエリア データ最小値 500byte *36枚

#else

#define AREATOP_RATIO_RANGE				(70)					/* Lv1Lv2トップ：縦横拡張基準比（*100） */
#define LV1TOP_UPCNT_MAX				(6)						/* Lv1トップ：拡張回数制限 */
#define LV1TOP_DENSITY_MIN				(7000)					/* Lv1トップ：保障データ数 */
#define	LV1TOP_PCL_MAX					(49)					/* Lv1トップ：拡張枚数制限 */

#endif

#define RC_JUDGE_RECOMMENDLV_PCL_LOW	(7 * 8)					/* 推奨レベル判定:パーセル枚数閾値1 */
#define RC_JUDGE_RECOMMENDLV_PCL_MID	(8 * 9)					/* 推奨レベル判定:パーセル枚数閾値2 */
#define RC_JUDGE_RECOMMENDLV_LINK_LOW	(100000)				/* 推奨レベル判定:リンク本数閾値1 */
#define RC_JUDGE_RECOMMENDLV_LINK_MID	(200000)				/* 推奨レベル判定:リンク本数閾値2 */
#define RC_JUDGE_RECOMMENDLV_DISTANCE	(10000)					/* 推奨レベル判定:OD距離10km */

#define AREA_MAKE_FINISH				(0)						/* エリア作成終了 */
#define AREA_MAKE_NEXT					(1)						/* 次エリア作成 */
#define AREA_MAKE_FAILED				(2)						/* エリア作成失敗 */

#define AREA_EXTENT_DIAGONAL			(0)						/* 対角拡張 */
#define AREA_EXTENT_SN					(1)						/* 上下拡張 */
#define AREA_EXTENT_EW					(2)						/* 左右拡張 */
#define AREA_EXTENT_ROUND				(3)						/* 1週拡張 */

#define BASE_AREA_TYPE_O				(0)						/* makeSideBaseAreaパラメタ：O側 */
#define BASE_AREA_TYPE_D				(1)						/* makeSideBaseAreaパラメタ：D側 */

/*-------------------------------------------------------------------
 * 変数定義
 *-------------------------------------------------------------------*/
/* スタックインデックス(グローバル) */
INT8 gDivStackIdx = RC_DIVAREA_STACK_IDX_INIT;
/* スタックテーブル */
static RC_DIVAREA_STACK mDivStack[RC_DIVAREA_STACK_MAX];
/* レベル１エリア作成フラグ RC_Area#RC_AreaOnlyLv1()にてのみ設定される */
static Bool mOnlyLv1Flg = false;
static Bool mOnlyLv2Flg = false;

/*-------------------------------------------------------------------
 * プロトタイプ宣言
 *-------------------------------------------------------------------*/
static E_SC_RESULT makeCoverArea(T_AreaInfo* aBase1, T_AreaInfo* aBase2, T_AreaInfo* aArea);
static E_SC_RESULT makeAreaLv1Top(T_AreaInfo* aOArea, T_AreaInfo* aDArea, T_AreaInfo* aTop, T_DHC_ROAD_DENSITY* aAreaDensity, INT32* aState);
static E_SC_RESULT makeSideBaseArea(SCRP_NEIGHBORINFO* aNbr, INT32 aType, T_AreaInfo* aArea);
static E_SC_RESULT makeAreaLv1(SCRP_LEVELTBL* aLeveltbl, T_AreaInfo *oBaseArea, T_AreaInfo *dBaseArea, T_AreaInfo *topArea,
		UINT32 startParcelIdLv1, UINT32 goalParcelIdLv1);
static E_SC_RESULT makeAreaLv1ToLv2(SCRP_LEVELTBL* aLeveltbl, T_AreaInfo *oBaseArea, T_AreaInfo *dBaseArea, UINT32 Lv1startparcelId,
		UINT32 Lv1goalparcelId, INT32 abs_sft_x, INT32 abs_sft_y);
static E_SC_RESULT makeSideBaseAreaLv2(T_AreaInfo *BaseArea, E_RC_AREATYPE areaType, SCRP_LEVELAREA *BaseAreaLv2, UINT32 Lv1shiftx,
		UINT32 Lv1shifty);
static E_SC_RESULT extCheckBaseAreaLv2(E_RC_AREATYPE areaType, INT16 Lv1BaseShift_x, INT16 Lv1BaseShift_y, INT16* ext_x, INT16* ext_y);
static E_SC_RESULT makeAreaLv2Top(SCRP_LEVELAREA* aOAreaLv2, SCRP_LEVELAREA* aDAreaLv2, SCRP_LEVELAREA* aTopLv2);
static E_SC_RESULT makeCoverAreaLv2(SCRP_LEVELAREA* aBase1Lv2, SCRP_LEVELAREA* aBase2Lv2, SCRP_LEVELAREA* topAreaLv2);
static E_SC_RESULT judgeRecommendLv(T_AreaInfo *topAreaLv1, INT8 *recLv,DOUBLE odDistance);
static E_SC_RESULT convertLevelTblToOld(SCRP_LEVELTBL *aLevel, SCRP_LEVELTBL_OLD *aLevelOld);

/**
 * 沖縄パッチ
 */
#define AREATOP_RATIO_RANGE_OKINAWA		(100)						// トップエリア 沖縄パッチ 縦横比値(*100)
const static DOUBLE PROMOTE_OKINAWA_LAT = 26.475;					// 沖縄パッチ判定中心緯度
const static DOUBLE PROMOTE_OKINAWA_LON = 127.9820835;				// 沖縄パッチ判定中心経度
const static LONG PROMOTE_OKINAWA_INSIDE = 56157;					// 沖縄パッチ判定距離
static Bool mOkinawaPatch = false;	// 沖縄パッチ判定
static Bool judgeInsideOkinawa(SCRP_NEIGHBORINFO* aONbor, SCRP_NEIGHBORINFO* aDNbor);

/**
 * @brief 強制レベル１エリア作成ラッパー関数
 * @param [O]レベル管理テーブル
 * @param ネットワーク管理
 * @param O側近傍情報
 * @param D側近傍情報
 * @memo 強制的ににレベル１でのエリア作成を行います
 *       RC_Area.c#judgeRecommendLv()判定結果に対する強制力を持つ
 */
E_SC_RESULT RC_AreaOnlyLv1(SCRP_LEVELTBL* aLevel, SCRP_NETCONTROLER* aNetTab, SCRP_NEIGHBORINFO* aONbor, SCRP_NEIGHBORINFO* aDNbor) {
	E_SC_RESULT result = e_SC_RESULT_FAIL;
	mOnlyLv1Flg = true;
	result = RC_Area(aLevel, aNetTab, aONbor, aDNbor);
	mOnlyLv1Flg = false;
	return (result);
}

/**
 * @brief 強制レベル２エリア作成ラッパー関数
 * @param [O]レベル管理テーブル
 * @param ネットワーク管理
 * @param O側近傍情報
 * @param D側近傍情報
 * @memo 強制的ににレベル２でのエリア作成を行います
 *       RC_Area.c#judgeRecommendLv()判定結果に対する強制力を持つ
 */
E_SC_RESULT RC_AreaOnlyLv2(SCRP_LEVELTBL* aLevel, SCRP_NETCONTROLER* aNetTab, SCRP_NEIGHBORINFO* aONbor, SCRP_NEIGHBORINFO* aDNbor) {
	E_SC_RESULT result = e_SC_RESULT_FAIL;
	mOnlyLv2Flg = true;
	result = RC_Area(aLevel, aNetTab, aONbor, aDNbor);
	mOnlyLv2Flg = false;
	return (result);
}

/**
 * @brief	エリア
 * @param	[O]レベル管理テーブル
 * @param	[I]探索情報テーブル
 * @param	[I]O側近傍情報
 * @param	[I]D側近傍情報
 */
E_SC_RESULT RC_Area(SCRP_LEVELTBL* aLevel, SCRP_NETCONTROLER* aNetTab, SCRP_NEIGHBORINFO* aONbor, SCRP_NEIGHBORINFO* aDNbor) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT result = e_SC_RESULT_FAIL;
	T_AreaInfo oBaseArea = {};
	T_AreaInfo dBaseArea = {};
	T_AreaInfo topArea = {};
	T_DHC_ROAD_DENSITY areaDensity = {};
	INT32 makeState = AREA_MAKE_FAILED;
	INT32 xSftLv1 =0;
	INT32 ySftLv1 =0;
	INT8 recLv = RP_LEVEL1;
	INT32 intRet =0;
	DOUBLE odDistance =0;

	// プロセス登録
	RPC_SetCalcProcess(e_RC_PROC_MAKEAREA);

	if ((NULL == aLevel) || (NULL == aNetTab) || (NULL == aONbor) || (NULL == aDNbor)) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[Area] Bad param."HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 初期化
	RP_Memset0(aLevel, sizeof(SCRP_LEVELTBL));

	// パッチ判定
	mOkinawaPatch = judgeInsideOkinawa(aONbor, aDNbor);

	// Lv1トップエリア作成
	do {
		// 近傍座標からのOD基準エリア作成
		result = makeSideBaseArea(aONbor, BASE_AREA_TYPE_O, &oBaseArea);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[Area] makeSideBaseArea. [0x%08x] "HERE, result);
			break;
		}
		result = makeSideBaseArea(aDNbor, BASE_AREA_TYPE_D, &dBaseArea);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[Area] makeSideBaseArea. [0x%08x] "HERE, result);
			break;
		}

		// OD基準エリアからトップエリアを作成する
		result = makeAreaLv1Top(&oBaseArea, &dBaseArea, &topArea, &areaDensity, &makeState);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[Area] makeAreaLv1Top. [0x%08x] "HERE, result);
			break;
		}
		if (AREA_MAKE_FAILED == makeState) {
			result = e_SC_RESULT_FAIL;
			SC_LOG_ErrorPrint(SC_TAG_RC, "error."HERE);
			break;
		}
		if (AREA_MAKE_FINISH == makeState) {
			break;
		}
	} while (0);

	odDistance = RP_Lib_CalcODLength(aONbor->point,aDNbor->point);
	if(0 == odDistance){
		//ODが同一地点、warnPrintのみ
		SC_LOG_WarnPrint(SC_TAG_RC, "[RC_Area]Warning odDistance is 0");
	}
	if (e_SC_RESULT_SUCCESS == result) {

#if 0
		//ダンプ確認
		SC_LOG_InfoPrint(SC_TAG_RC, "[RC_Area]oBaseAreaLv1_parcelId	[0x%08x]", oBaseArea.pclRect.parcelId);
		SC_LOG_InfoPrint(SC_TAG_RC, "[RC_Area]oBaseAreaLv1_xSize	[%4d]", oBaseArea.pclRect.xSize);
		SC_LOG_InfoPrint(SC_TAG_RC, "[RC_Area]oBaseAreaLv1_ySize	[%4d]", oBaseArea.pclRect.ySize);
		SC_LOG_InfoPrint(SC_TAG_RC, "[RC_Area]dBaseAreaLv1_parcelId	[0x%08x]", dBaseArea.pclRect.parcelId);
		SC_LOG_InfoPrint(SC_TAG_RC, "[RC_Area]dBaseAreaLv1_xSize	[%4d]", dBaseArea.pclRect.xSize);
		SC_LOG_InfoPrint(SC_TAG_RC, "[RC_Area]dBaseAreaLv1_ySize	[%4d]", dBaseArea.pclRect.ySize);
		SC_LOG_InfoPrint(SC_TAG_RC, "[RC_Area]Lv1Top_parcelId		[0x%08x]", topArea.pclRect.parcelId);
		SC_LOG_InfoPrint(SC_TAG_RC, "[RC_Area]Lv1Top_xSize			[%4d]", topArea.pclRect.xSize);
		SC_LOG_InfoPrint(SC_TAG_RC, "[RC_Area]Lv1Top_ySize			[%4d]", topArea.pclRect.ySize);
		SC_LOG_InfoPrint(SC_TAG_RC, "[RC_Area]odDistance			[%f]", odDistance);
#endif

#if _RP_USE_OLDAREA /* 旧エリア処理（Lv1のみに対応）*/
		// 基準エリア結果格納
		SCRP_LEVELTBL_OLD levelTblOld = {};
		RP_Memcpy(&(levelTblOld.topArea), &topArea, sizeof(T_AreaInfo));
		levelTblOld.topLevel = RP_LEVEL1;
		levelTblOld.topArea.divAreaVol = levelTblOld.topArea.pclRect.xSize * levelTblOld.topArea.pclRect.ySize;

		result = RC_AreaDiv(&levelTblOld, aONbor, aDNbor, &areaDensity);
		if (result != e_SC_RESULT_SUCCESS) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RC_AreaDiv error. [0x%08x] "HERE, result);
			aLevel->areaTable[0].divVol = 0;
		} else {
			// 旧テーブルを新テーブルへコンバート
			result = convertLevelTblToOld(aLevel, &levelTblOld);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "convertLevelTblToOld error. [0x%08x] "HERE, result);
			}
		}
#else
		//推奨レベル判定
		result = judgeRecommendLv(&topArea, &recLv,odDistance);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "judgeRecommendLv error. [0x%08x]"HERE, result);
			return (result);
		}
		SC_LOG_InfoPrint(SC_TAG_RC, "[RC_Area]recommendLv[%d] ",recLv);

		if (RP_LEVEL2 == recLv) {
			intRet = SC_MESH_GetAlterPos(aONbor->point.parcelId, aDNbor->point.parcelId, RP_LEVEL1, &xSftLv1, &ySftLv1);
			if (0 != intRet) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "[RC_Area]GetAlterPos error."HERE);
				result = e_SC_RESULT_FAIL;
				return (result);
			}
			//Lv2エリア作成
			result = makeAreaLv1ToLv2(aLevel, &oBaseArea, &dBaseArea, aONbor->point.parcelId, aDNbor->point.parcelId, xSftLv1, ySftLv1);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_WarnPrint(SC_TAG_RC, "[RC_Area]makeAreaLv1ToLv2 fail  next try makeAreaLv1.[0x%08x]"HERE, result);
				//Lv2でうまくいかないのでLv1の分割処理でやってみる
				//基準エリア結果格納
				result = makeAreaLv1(aLevel, &oBaseArea, &dBaseArea,&topArea ,aONbor->point.parcelId, aDNbor->point.parcelId);
				if (e_SC_RESULT_SUCCESS != result) {
					SC_LOG_ErrorPrint(SC_TAG_RC, "[RC_Area]RC_AreaLv1 error.[0x%08x]"HERE, result);
					return (result);
				}
			}
		} else if (RP_LEVEL1 == recLv) {

			result = makeAreaLv1(aLevel, &oBaseArea, &dBaseArea,&topArea ,aONbor->point.parcelId, aDNbor->point.parcelId);
			if (e_SC_RESULT_SUCCESS != result) {
				//Lv1ではうまくいかない
				SC_LOG_WarnPrint(SC_TAG_RC, "[RC_Area]makeAreaLv1 fail  next try makeAreaLv1ToLv2.[0x%08x]"HERE, result);
				intRet = SC_MESH_GetAlterPos(aONbor->point.parcelId, aDNbor->point.parcelId, RP_LEVEL1, &xSftLv1, &ySftLv1);
				if (0 != intRet) {
					SC_LOG_ErrorPrint(SC_TAG_RC, "[RC_Area]GetAlterPos error."HERE);
					result = e_SC_RESULT_FAIL;
					return (result);
				}
				//Lv2エリア作成
				result = makeAreaLv1ToLv2(aLevel, &oBaseArea, &dBaseArea, aONbor->point.parcelId, aDNbor->point.parcelId, xSftLv1, ySftLv1);
				if (e_SC_RESULT_SUCCESS != result) {
					SC_LOG_ErrorPrint(SC_TAG_RC, "[RC_Area]makeAreaLv1ToLv2 error.[0x%08x]"HERE, result);
					return (result);
				}
			}
		} else {
			//推奨レベルが想定外
			SC_LOG_ErrorPrint(SC_TAG_RC, "judgeRecommendLv result error.recLv[%d]"HERE, recLv);
			return (e_SC_RESULT_FAIL);
		}
#endif
	} else {
		SC_LOG_ErrorPrint(SC_TAG_RC, "error."HERE);
	}
	if (NULL != areaDensity.data) {
		RP_MemFree(areaDensity.data, e_MEM_TYPE_ROUTEPLAN);
	}

#if _SCMPDBG_SHOWROUTEAREA
	{
		SCMPDBG_AREAINFO* debug = SCMPDBG_GetRouteAreaInfo();
		UINT32 max = aLevel->divInfoVol;
		UINT16 setIdx = 0;
		UINT16 i;
		for (i = 0; i < aLevel->divInfoVol; i++) {
			SCRP_DIVAREA* divArea = aLevel->divInfo + i;
			debug->rect[setIdx].pcl = divArea->pclRect.parcelId;
			debug->rect[setIdx].x = divArea->pclRect.xSize;
			debug->rect[setIdx].y = divArea->pclRect.ySize;
			setIdx++;
		}
		debug->rectVol = setIdx;
	}
#endif
#if _RP_LOG_AREAINFO
	SCRP_LEVELTBL aLevelDmp = {};
	RP_Memcpy(&aLevelDmp, aLevel, sizeof(SCRP_LEVELTBL));
	RPDBG_ShowAreaInfo(&aLevelDmp, &oBaseArea, &dBaseArea);
#endif
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief エリアをスタックに追加する
 * @param info [I]エリア矩形、接続方向、格納優先度
 */
E_SC_RESULT RC_AreaStackPush(RC_AREASTATEINFO* info) {

	if (NULL == info) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (gDivStackIdx >= (RC_DIVAREA_STACK_MAX - 1) || gDivStackIdx < -1) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "areaStackPush error. gDivStackIdx[%d] "HERE, gDivStackIdx);
		return (e_SC_RESULT_FAIL);
	}
	gDivStackIdx++;
	mDivStack[gDivStackIdx].pclRect.parcelId = info->pclRect.parcelId;
	mDivStack[gDivStackIdx].pclRect.xSize = info->pclRect.xSize;
	mDivStack[gDivStackIdx].pclRect.ySize = info->pclRect.ySize;
	mDivStack[gDivStackIdx].areaType = info->areaType;
	mDivStack[gDivStackIdx].joinLeft = info->joinLeft;
	mDivStack[gDivStackIdx].joinRight = info->joinRight;
	mDivStack[gDivStackIdx].joinTop = info->joinTop;
	mDivStack[gDivStackIdx].joinBottom = info->joinBottom;
	mDivStack[gDivStackIdx].primaryX = info->primaryX;
	mDivStack[gDivStackIdx].primaryY = info->primaryY;
	mDivStack[gDivStackIdx].lastDivSideX = info->lastDivSideX;
	mDivStack[gDivStackIdx].lastDivSideY = info->lastDivSideY;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * パーセル密度情報取得
 * @param aDensity 結果格納用
 * @param aPid 左下パーセルID
 * @param aX X枚数
 * @param aY Y枚数
 * @note 密度格納領域は本関数内で確保する。領域の開放はコール元で処理すること。
 */
E_SC_RESULT RC_GetParcelDensity(T_DHC_ROAD_DENSITY *aDensity, UINT32 aPid, UINT16 aX, UINT16 aY) {

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	E_DHC_CASH_RESULT resultDhc = e_DHC_RESULT_CASH_SUCCESS;

	if (NULL == aDensity) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param"HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	do {
		RP_Memset0(aDensity, sizeof(T_DHC_ROAD_DENSITY));

		aDensity->baseParcelId = aPid;
		aDensity->x = aX;
		aDensity->y = aY;
		aDensity->data = (T_DHC_ROAD_DENSITY_DATA*) RP_MemAlloc(sizeof(T_DHC_ROAD_DENSITY_DATA) * aX * aY, e_MEM_TYPE_ROUTEPLAN);
		if (NULL == aDensity->data) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[Area] RP_MemAlloc error."HERE);
			result = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		RP_Memset0(aDensity->data, sizeof(T_DHC_ROAD_DENSITY_DATA) * aX * aY);
		resultDhc = SC_DHC_GetRoadDensity(aDensity);
		if (e_DHC_RESULT_CASH_SUCCESS != resultDhc) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[Area] SC_DHC_GetRoadDensity error.[0x%08x]"HERE, resultDhc);
			result = e_SC_RESULT_FAIL;

			// 失敗時はメモリ開放を行う
			RP_MemFree(aDensity->data, e_MEM_TYPE_ROUTEPLAN);
			aDensity->data = NULL;
			break;
		}
	} while (0);

	return (result);
}

/**
 * @brief エリアをスタックから取り出す
 * @param info [O]エリア矩形、接続方向、格納優先度
 */
E_SC_RESULT RC_AreaStackPop(RC_AREASTATEINFO* info) {

	if (NULL == info) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (gDivStackIdx < 0) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "gDivStackIdx error. %d "HERE, gDivStackIdx);
		return (e_SC_RESULT_FAIL);
	}
	info->pclRect.parcelId = mDivStack[gDivStackIdx].pclRect.parcelId;
	info->pclRect.xSize = mDivStack[gDivStackIdx].pclRect.xSize;
	info->pclRect.ySize = mDivStack[gDivStackIdx].pclRect.ySize;
	info->areaType = mDivStack[gDivStackIdx].areaType;
	info->joinLeft = mDivStack[gDivStackIdx].joinLeft;
	info->joinRight = mDivStack[gDivStackIdx].joinRight;
	info->joinTop = mDivStack[gDivStackIdx].joinTop;
	info->joinBottom = mDivStack[gDivStackIdx].joinBottom;
	info->primaryX = mDivStack[gDivStackIdx].primaryX;
	info->primaryY = mDivStack[gDivStackIdx].primaryY;
	info->lastDivSideX = mDivStack[gDivStackIdx].lastDivSideX;
	info->lastDivSideY = mDivStack[gDivStackIdx].lastDivSideY;
	gDivStackIdx--;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * O側基準エリア生成処理
 * @param aNbr [I]近傍情報
 * @param aType [I]OD指定
 * @param aArea [O]エリア作成結果
 * @note 2*2～6*6の範囲でエリアは構築する
 *       O側の場合、最近傍道路種別が首都高速，都市間高速である場合最低4*4となる
 *       拡張はリンク数がBASE_AREA_DENSITY_MINを越えた時点で終了する
 */
static E_SC_RESULT makeSideBaseArea(SCRP_NEIGHBORINFO* aNbr, INT32 aType, T_AreaInfo* aArea) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;

	T_DHC_ROAD_DENSITY density = {};
	T_DHC_ROAD_DENSITY_DATA *data = NULL;

	UINT32 checkMinLink = 0;
	INT16 checkMaxPcl = 0;
	INT16 checkUpCnt = 0;
	INT16 baseShift[2][2] = {};
	INT16 update = 0;
	INT16 max = LV1ODBASE_SIDEPCL_MAX * LV1ODBASE_SIDEPCL_MAX;

	if (NULL == aNbr || NULL == aArea) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[Area] Bad param."HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	data = (T_DHC_ROAD_DENSITY_DATA*) RP_MemAlloc(sizeof(T_DHC_ROAD_DENSITY_DATA) * max, e_MEM_TYPE_ROUTEPLAN);
	if (NULL == data) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error. "HERE);
		return (e_SC_RESULT_MALLOC_ERR);
	}
	RP_Memset0(data, sizeof(T_DHC_ROAD_DENSITY_DATA) * max);

	// 拡張保障回数および保障リンク数設定
	if (BASE_AREA_TYPE_O == aType) {
		if (0 == aNbr->nbrLinkVol || NULL == aNbr->neighborLink) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[Area] Bad param."HERE);
			return (e_SC_RESULT_BADPARAM);
		}

		if (SC_MA_ROAD_TYPE_HIGHWAY_2 == aNbr->neighborLink->roadKind
				|| SC_MA_ROAD_TYPE_HIGHWAY_1 == aNbr->neighborLink->roadKind) {
			checkUpCnt = LV1ODBASE_UPCNT_HIGHWAY;
		} else {
			checkUpCnt = LV1ODBASE_UPCNT_NORMAL;
		}
		checkMinLink = LV1ODBASE_DENSITY_MIN;
		checkMaxPcl = LV1ODBASE_SIDEPCL_MAX;
	} else {
		checkUpCnt = LV1ODBASE_UPCNT_NORMAL;
		checkMinLink = LV1ODBASE_DENSITY_MIN;
		checkMaxPcl = LV1ODBASE_SIDEPCL_MAX;
	}

	// 位置関係別加算値取得
	if (aNbr->point.x < (MAP_SIZE / 2)) {
		baseShift[0][0] = -1;
		baseShift[1][0] = 0;
	} else {
		baseShift[0][0] = 0;
		baseShift[1][0] = -1;
	}
	if (aNbr->point.y < (MAP_SIZE / 2)) {
		baseShift[0][1] = -1;
		baseShift[1][1] = 0;
	} else {
		baseShift[0][1] = 0;
		baseShift[1][1] = -1;
	}

	do {
		density.baseParcelId = aNbr->point.parcelId;
		density.x = 1;
		density.y = 1;
		density.data = data;

		while (density.x < checkMaxPcl && density.y < checkMaxPcl) {
			// 最低拡張回数保障後、密度確認
			if (checkUpCnt <= update) {
				E_DHC_CASH_RESULT resultDhc = SC_DHC_GetRoadDensity(&density);
				if (e_DHC_RESULT_CASH_SUCCESS != resultDhc) {
					result = e_SC_RESULT_FAIL;
					break;
				}
				if (checkMinLink < (density.totalDensity * SC_MA_REAL_DENSITY)) {
					break;
				}
			}

			density.baseParcelId = SC_MESH_SftParcelId(density.baseParcelId, baseShift[update % 2][0], baseShift[update % 2][1]);
			density.x++;
			density.y++;

			update++;
		}

		if (e_SC_RESULT_SUCCESS == result) {
			aArea->pclRect.parcelId = density.baseParcelId;
			aArea->pclRect.xSize = density.x;
			aArea->pclRect.ySize = density.y;
		}
	} while (0);

	if (NULL != data) {
		RP_MemFree(data, e_MEM_TYPE_ROUTEPLAN);
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief	基準エリア２つから包含エリアを作成
 * @param	[I]基準１
 * @param	[I]基準２
 * @param	[O]包含エリア格納用
 *
 * ■を包含するエリアを作成
 *  □□■■  →  ■■■■
 *  □□■■  →  ■■■■
 *  ■■□□  →  ■■■■
 *  ■■□□  →  ■■■■
 */
static E_SC_RESULT makeCoverArea(T_AreaInfo* aBase1, T_AreaInfo* aBase2, T_AreaInfo* aArea)
{
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	INT8 level = 0;				// Level
	INT32 xSft = 0;				// X方向シフト量
	INT32 ySft = 0;				// Y方向シフト量
	UINT32 lbId = 0;			// パーセルID
	UINT32 rtId = 0;			// パーセルID
	UINT32 rtIdBase1 = 0;		// パーセルID
	UINT32 rtIdBase2 = 0;		// パーセルID

	if ((NULL == aBase1) || (NULL == aBase2)) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[Area] Bad param."HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// レベル取得
	level = SC_MESH_GetLevel(aBase1->pclRect.parcelId);
	if (level != SC_MESH_GetLevel(aBase2->pclRect.parcelId)) {
		// レベルが違う
		SC_LOG_ErrorPrint(SC_TAG_RC, "[Area] Parcel level unmatch."HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	// 左下特定
	if (0 != SC_MESH_GetAlterPos(aBase1->pclRect.parcelId, aBase2->pclRect.parcelId, level, &xSft, &ySft)) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[Area] SC_MESH_GetAlterPos."HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (0 < xSft) {
		xSft = 0;
	}
	if (0 < ySft) {
		ySft = 0;
	}
	// 基準エリア格納
	lbId = SC_MESH_SftParcelId(aBase1->pclRect.parcelId, xSft, ySft);
	// 右上特定
	rtIdBase1 = SC_MESH_SftParcelId(aBase1->pclRect.parcelId, (aBase1->pclRect.xSize - 1), (aBase1->pclRect.ySize - 1));
	rtIdBase2 = SC_MESH_SftParcelId(aBase2->pclRect.parcelId, (aBase2->pclRect.xSize - 1), (aBase2->pclRect.ySize - 1));
	if (0 != SC_MESH_GetAlterPos(rtIdBase1, rtIdBase2, level, &xSft, &ySft)) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[Area] SC_MESH_GetAlterPos."HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (xSft < 0) {
		xSft = 0;
	}
	if (ySft < 0) {
		ySft = 0;
	}
	rtId = SC_MESH_SftParcelId(rtIdBase1, xSft, ySft);
	// 右上とのシフト量格納
	if (0 != SC_MESH_GetAlterPos(lbId, rtId, level, &xSft, &ySft)) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[Area] SC_MESH_GetAlterPos."HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	// シフト量＋１（枚数）を格納
	aArea->pclRect.parcelId = lbId;
	aArea->pclRect.xSize = (xSft + 1);
	aArea->pclRect.ySize = (ySft + 1);
	//aArea->parcelVol = (aArea->x * aArea->y);

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief レベル１トップエリア作成処理
 * @param [I]同一レベルO側基準エリア
 * @param [I]同一レベルD側基準エリア
 * @param [O]結果エリア(AREA_MAKE_FINISHの場合のみ値を保障)
 * @param [O]密度情報
 * @param [O]エリア作成ステータス
 * @return e_SC_RESULT_SUCCESS 正常終了
 *         e_SC_RESULT_SUCCESS以外 異常終了
 * @memo TODO エリア拡張後の枚数制限
 * @memo 注意：エリアを基準包含エリアよりかも小さくはしないように
 */
static E_SC_RESULT makeAreaLv1Top(T_AreaInfo* aOArea, T_AreaInfo* aDArea, T_AreaInfo* aTop, T_DHC_ROAD_DENSITY* aAreaDensity,
		INT32 *aState)
{
	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	//E_DHC_CASH_RESULT resultDhc = e_DHC_RESULT_CASH_SUCCESS;

	T_AreaInfo topArea = {};

	INT16 ratioSft[2] = {};
	INT16 ratioAdd[2] = {};
	UINT32 xReal = 0;
	UINT32 yReal = 0;
	UINT32 *real[2] = {};
	UINT32 realBasesY = 0;
	UINT8 iRatio; // 縦横比
	UINT8 updateCount = 0;

	INT32 range = AREATOP_RATIO_RANGE;
	//INT16 patchUpCount = 0;

	if (NULL == aOArea || NULL == aDArea || NULL == aTop || NULL == aAreaDensity || NULL == aState) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "param error."HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 処理結果初期化
	*aState = AREA_MAKE_FAILED;

	// OD包含エリア作成
	result = makeCoverArea(aOArea, aDArea, &topArea);
	if (e_SC_RESULT_SUCCESS != result) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[Area] RC_MakeCoverArea.[0x%08x]"HERE, result);
		return (result);
	}

	// 実長取得 topAreaからの場合リンクデータ無しの可能性が少し高くなる為O側から取得
	realBasesY = RP_Lib_GetParcelRealXYRatio(aOArea->pclRect.parcelId, LV1ODBASE_REAL_Y_DEF) * 100.0f;

	// XY比から正方形に近づける
	xReal = topArea.pclRect.xSize * 100;
	yReal = topArea.pclRect.ySize * realBasesY;
	if (xReal < yReal) {
		ratioSft[0] = -1;
		ratioAdd[0] = 2;
		real[0] = &xReal;
		real[1] = &yReal;
		iRatio = ((FLOAT) *real[0] / (FLOAT) *real[1]) * 100;
	} else {
		ratioSft[1] = -1;
		ratioAdd[1] = 2;
		real[0] = &yReal;
		real[1] = &xReal;
		iRatio = ((FLOAT) *real[0] / (FLOAT) *real[1]) * 100;
	}

	if (mOkinawaPatch) {
		range = AREATOP_RATIO_RANGE_OKINAWA;
	}

	// XY比をAREA_RATIO_RANGE超えさせる
	while (iRatio <= range) {

		topArea.pclRect.xSize += ratioAdd[0];
		topArea.pclRect.ySize += ratioAdd[1];
		topArea.pclRect.parcelId = SC_MESH_SftParcelId(topArea.pclRect.parcelId, ratioSft[0], ratioSft[1]);

		xReal = topArea.pclRect.xSize * 100;
		yReal = topArea.pclRect.ySize * realBasesY;
		iRatio = ((FLOAT) *real[0] / (FLOAT) *real[1]) * 100;

		SC_LOG_DebugPrint(SC_TAG_RC, "[Area] Ratio v/h update. ratio=%d [0x%08x](%d,%d)", iRatio, topArea.pclRect.parcelId, topArea.pclRect.xSize,
				topArea.pclRect.ySize);
	}

	do {
		// 密度取得
		result = RC_GetParcelDensity(aAreaDensity, topArea.pclRect.parcelId, topArea.pclRect.xSize, topArea.pclRect.ySize);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "getParcelDensity error. [0x%08x]"HERE, result);
			break;
		}

		while (1) {
			const INT16 sft[2][2] = { { -1, -1 }, { 0, 0 } }; // [奇遇][XY]
			const INT16 add[2] = { 1, 1 }; // [XY]

			// エリア内リンク本数が最低本数を超えた
			if (LV1TOP_DENSITY_MIN < (aAreaDensity->totalDensity * SC_MA_REAL_DENSITY)) {
				SC_LOG_DebugPrint(SC_TAG_RC, "[Area] top area range in. %d ", aAreaDensity->totalDensity * SC_MA_REAL_DENSITY);
				break;
			}
			if (LV1TOP_UPCNT_MAX < updateCount) {
				SC_LOG_DebugPrint(SC_TAG_RC, "[Area] top area extend count is over extend max. %d", (topArea.pclRect.xSize * topArea.pclRect.ySize));
				break;
			}
#if	_RP_USE_OLDAREA
			//旧エリア処理には入っていない処理
#else
			//拡張結果が基準枚数を超えた
			if (LV1TOP_PCL_MAX < topArea.pclRect.xSize *topArea.pclRect.ySize) {
				SC_LOG_DebugPrint(SC_TAG_RC, "[Area] extend pcl [%d]is over LV1TOP_PCL_MAX[%d]", (topArea.pclRect.xSize * topArea.pclRect.ySize),LV1TOP_PCL_MAX);
				break;
			}
#endif

			// 対角線上に1枚ずつ拡張
			topArea.pclRect.parcelId = SC_MESH_SftParcelId(topArea.pclRect.parcelId, sft[updateCount % 2][0], sft[updateCount % 2][1]);
			topArea.pclRect.xSize += add[0];
			topArea.pclRect.ySize += add[1];

			// 個々の密度はいらないから解放
			if (NULL != aAreaDensity->data) {
				RP_MemFree(aAreaDensity->data, e_MEM_TYPE_ROUTEPLAN);
			}

			// 密度取得
			result = RC_GetParcelDensity(aAreaDensity, topArea.pclRect.parcelId, topArea.pclRect.xSize, topArea.pclRect.ySize);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "getParcelDensity error. [0x%08x]"HERE, result);
				break;
			}

			// カウント
			updateCount++;
		}
		/*
		 * 沖縄Patch 生成されたエリアを1回り拡張する
		 */
		if (mOkinawaPatch) {
			SC_LOG_DebugPrint(SC_TAG_RC, "run okinawa patch. update top area.");

			topArea.pclRect.parcelId = SC_MESH_SftParcelId(topArea.pclRect.parcelId, -1, -1);
			topArea.pclRect.xSize += 2;
			topArea.pclRect.ySize += 2;
			if (NULL != aAreaDensity->data) {
				RP_MemFree(aAreaDensity->data, e_MEM_TYPE_ROUTEPLAN);
			}
			result = RC_GetParcelDensity(aAreaDensity, topArea.pclRect.parcelId, topArea.pclRect.xSize, topArea.pclRect.ySize);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "getParcelDensity error. [0x%08x]"HERE, result);
				break;
			}
		}
	} while (0);

	if (e_SC_RESULT_SUCCESS == result) {
		aTop->pclRect.parcelId = topArea.pclRect.parcelId;
		aTop->pclRect.xSize = topArea.pclRect.xSize;
		aTop->pclRect.ySize = topArea.pclRect.ySize;
		aTop->divAreaVol = (aTop->pclRect.xSize * aTop->pclRect.ySize);
		aTop->divArea_p = NULL;
		*aState = AREA_MAKE_FINISH;

		// log
		SC_LOG_InfoPrint(SC_TAG_RC, "[Area] Make area info.\n  parcel vol: %d\n  parcel link vol: %d", aTop->divAreaVol,
				aAreaDensity->totalDensity * SC_MA_REAL_DENSITY);
	} else {
		if (NULL != aAreaDensity->data) {
			RP_MemFree(aAreaDensity->data, e_MEM_TYPE_ROUTEPLAN);
			aAreaDensity->data = NULL;
		}
	}

	return (result);
}

/**
 * @brief	エリアLv1分割処理準備
 * @param	[O]レベル管理テーブル
 * @param	[I]O側近傍情報
 * @param	[I]D側近傍情報
 * @param	[I]出発地目的地パーセルId
 */
static E_SC_RESULT makeAreaLv1(SCRP_LEVELTBL* aLeveltbl, T_AreaInfo *oBaseArea, T_AreaInfo *dBaseArea, T_AreaInfo *topAreaLv1,
		UINT32 startParcelIdLv1, UINT32 goalParcelIdLv1) {

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);
	E_SC_RESULT result = e_SC_RESULT_FAIL;

	if (NULL == aLeveltbl || NULL == oBaseArea || NULL == dBaseArea || NULL == topAreaLv1) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[makeAreaLv1] Bad param."HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	T_DHC_ROAD_DENSITY startAreaDensity = {};
	T_DHC_ROAD_DENSITY goalAreaDensity = {};
	UINT8 topLevel = RP_LEVEL1;
	UINT32 startParcelDensity = 0;
	UINT32 goalParcelDensity = 0;
	UINT16 totalPclVolTop = 0;

	do {
		result = RC_GetParcelDensity(&startAreaDensity, startParcelIdLv1, 1, 1);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[makeAreaLv1]getParcelDensity error. [0x%08x]"HERE, result);
			break;
		} else {
			startParcelDensity = startAreaDensity.totalDensity;
		}

		result = RC_GetParcelDensity(&goalAreaDensity, goalParcelIdLv1, 1, 1);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[makeAreaLv1]getParcelDensity error. [0x%08x]"HERE, result);
			break;
		} else {
			goalParcelDensity = goalAreaDensity.totalDensity;
		}
		//スタックインデックス初期化
		gDivStackIdx = RC_DIVAREA_STACK_IDX_INIT;

		//エリアプッシュ
		RC_AREASTATEINFO areaInfo = {};
		areaInfo.areaType = e_AREA_TYPE_OD;
		areaInfo.joinLeft = 0;
		areaInfo.joinBottom = 0;
		areaInfo.joinRight = 0;
		areaInfo.joinTop = 0;
		areaInfo.primaryX = -1;
		areaInfo.primaryY = -1;
		areaInfo.lastDivSideX = e_RC_LASTDIVSIDEFLG_XDEFAULT;
		areaInfo.lastDivSideY = e_RC_LASTDIVSIDEFLG_YDEFAULT;
		areaInfo.pclRect.parcelId = topAreaLv1->pclRect.parcelId;
		areaInfo.pclRect.xSize = topAreaLv1->pclRect.xSize;
		areaInfo.pclRect.ySize = topAreaLv1->pclRect.ySize;
		result = RC_AreaStackPush(&areaInfo);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[makeAreaLv1]areaStackPush error. [0x%08x]"HERE, result);
			break;
		}
		totalPclVolTop = topAreaLv1->pclRect.xSize * topAreaLv1->pclRect.ySize;

		RC_BASEAREATOPLVINFO baseAreaTopLvInf = {};
		RP_Memcpy(&baseAreaTopLvInf.oBaseAreaTopLv.pclRect, &oBaseArea->pclRect, sizeof(SCRP_PCLRECT));
		RP_Memcpy(&baseAreaTopLvInf.dBaseAreaTopLv.pclRect, &dBaseArea->pclRect, sizeof(SCRP_PCLRECT));
		baseAreaTopLvInf.startParcelDensityTopLv = startParcelDensity;
		baseAreaTopLvInf.goalParcelDensityTopLv = goalParcelDensity;
		baseAreaTopLvInf.startParcelIdTopLv = startParcelIdLv1;
		baseAreaTopLvInf.goalParcelIdTopLv = goalParcelIdLv1;
		RP_Memcpy(&baseAreaTopLvInf.oBaseArea, oBaseArea, sizeof(T_AreaInfo));
		RP_Memcpy(&baseAreaTopLvInf.dBaseArea, dBaseArea, sizeof(T_AreaInfo));
		baseAreaTopLvInf.totalPclVolTopLv = totalPclVolTop;
		RP_Memcpy(&baseAreaTopLvInf.areaTopLv.pclRect, &topAreaLv1->pclRect, sizeof(SCRP_PCLRECT));

		//topLevel分割
		result = RC_AreaDivTop(topLevel, aLeveltbl, &baseAreaTopLvInf);
		// 処理結果判定
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[makeAreaLv1] RC_AreaDivTop error.[0x%08x]"HERE, result);
			break;
		}
	} while (0);

	// start密度データ解放
	if (NULL != startAreaDensity.data) {
		RP_MemFree(startAreaDensity.data, e_MEM_TYPE_ROUTEPLAN);
		startAreaDensity.data = NULL;
	}
	// goal密度データ解放
	if (NULL != goalAreaDensity.data) {
		RP_MemFree(goalAreaDensity.data, e_MEM_TYPE_ROUTEPLAN);
		goalAreaDensity.data = NULL;
	}
	return (result);
}
/**
 * @brief	Lv2エリア作成+分割準備
 * @param	[O]レベル管理テーブル
 * @param	[I]O側近傍情報
 * @param	[I]D側近傍情報
 * @param	[I]出発地目的地2点間シフト量
 */
static E_SC_RESULT makeAreaLv1ToLv2(SCRP_LEVELTBL *aLeveltbl, T_AreaInfo *oBaseArea, T_AreaInfo *dBaseArea, UINT32 startParcelIdLv1,
		UINT32 goalParcelIdLv1, INT32 odXSftLv1, INT32 odYSftLv1) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT result = e_SC_RESULT_FAIL;

	SCRP_LEVELAREA oBaseAreaLv2 = {};
	SCRP_LEVELAREA dBaseAreaLv2 = {};
	SCRP_LEVELAREA topAreaLv2 = {};
	T_DHC_ROAD_DENSITY startAreaDensity = {};
	T_DHC_ROAD_DENSITY goalAreaDensity = {};
	UINT8 topLevel = RP_LEVEL2;
	UINT32 startParcelDensity = 0;
	UINT32 goalParcelDensity = 0;

	if (NULL == aLeveltbl || NULL == oBaseArea || NULL == dBaseArea) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[makeAreaLv1ToLv2] Bad param."HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// トップエリア（迄）作成
	topLevel = RP_LEVEL2;
	// 近傍座標からのOD基準エリア作成
	UINT32 startParcelIdLv2 = SC_MESH_GetUpperParcelID(startParcelIdLv1);
	UINT32 goalParcelIdLv2 = SC_MESH_GetUpperParcelID(goalParcelIdLv1);

	result = makeSideBaseAreaLv2(oBaseArea, e_AREA_TYPE_O, &oBaseAreaLv2, odXSftLv1, odYSftLv1);
	if (e_SC_RESULT_SUCCESS != result) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[makeAreaLv1ToLv2] makeSideBaseAreaLv2 error.[0x%08x]"HERE, result);
		return (result);
	}

	result = makeSideBaseAreaLv2(dBaseArea, e_AREA_TYPE_D, &dBaseAreaLv2, odXSftLv1, odYSftLv1);
	if (e_SC_RESULT_SUCCESS != result) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[makeAreaLv1ToLv2] makeSideBaseAreaLv2 error.[0x%08x]"HERE, result);
		return (result);
	}
	// OD基準エリアからトップエリアを作成する
	result = makeAreaLv2Top(&oBaseAreaLv2, &dBaseAreaLv2, &topAreaLv2);
	// 処理結果判定
	if (e_SC_RESULT_SUCCESS != result) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[makeAreaLv1ToLv2] makeAreaLv2Top error.[0x%08x]"HERE, result);
		return (result);
	}
	do {
		result = RC_GetParcelDensity(&startAreaDensity, startParcelIdLv2, 1, 1);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[makeAreaLv1ToLv2] getParcelDensity error. [0x%08x]"HERE, result);
			break;
		} else {
			startParcelDensity = startAreaDensity.totalDensity;
		}

		result = RC_GetParcelDensity(&goalAreaDensity, goalParcelIdLv2, 1, 1);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[makeAreaLv1ToLv2] getParcelDensity error. [0x%08x]"HERE, result);
			break;
		} else {
			goalParcelDensity = goalAreaDensity.totalDensity;
		}

		//スタックインデックス初期化
		gDivStackIdx = RC_DIVAREA_STACK_IDX_INIT;

		//エリアプッシュ
		RC_AREASTATEINFO areaInfo = {};
		areaInfo.areaType = e_AREA_TYPE_OD;
		areaInfo.joinLeft = 0;
		areaInfo.joinBottom = 0;
		areaInfo.joinRight = 0;
		areaInfo.joinTop = 0;
		areaInfo.primaryX = -1;
		areaInfo.primaryY = -1;
		areaInfo.lastDivSideX = e_RC_LASTDIVSIDEFLG_XDEFAULT;
		areaInfo.lastDivSideY = e_RC_LASTDIVSIDEFLG_YDEFAULT;
		areaInfo.pclRect.parcelId = topAreaLv2.pclRect.parcelId;
		areaInfo.pclRect.xSize = topAreaLv2.pclRect.xSize;
		areaInfo.pclRect.ySize = topAreaLv2.pclRect.ySize;
		result = RC_AreaStackPush(&areaInfo);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[makeAreaLv1ToLv2] areaStackPush error.[0x%08x]"HERE, result);
			break;
		}
		UINT16 totalPclVolTop = topAreaLv2.pclRect.xSize * topAreaLv2.pclRect.ySize;

		RC_BASEAREATOPLVINFO baseAreaTopLvInf = {};
		RP_Memcpy(&baseAreaTopLvInf.oBaseAreaTopLv.pclRect, &oBaseAreaLv2.pclRect, sizeof(SCRP_PCLRECT));
		RP_Memcpy(&baseAreaTopLvInf.dBaseAreaTopLv.pclRect, &dBaseAreaLv2.pclRect, sizeof(SCRP_PCLRECT));
		baseAreaTopLvInf.startParcelDensityTopLv = startParcelDensity;
		baseAreaTopLvInf.goalParcelDensityTopLv = goalParcelDensity;
		baseAreaTopLvInf.startParcelIdTopLv = startParcelIdLv2;
		baseAreaTopLvInf.goalParcelIdTopLv = goalParcelIdLv2;
		RP_Memcpy(&baseAreaTopLvInf.oBaseArea, oBaseArea, sizeof(T_AreaInfo));
		RP_Memcpy(&baseAreaTopLvInf.dBaseArea, dBaseArea, sizeof(T_AreaInfo));
		baseAreaTopLvInf.totalPclVolTopLv = totalPclVolTop;
		RP_Memcpy(&baseAreaTopLvInf.areaTopLv.pclRect, &topAreaLv2.pclRect, sizeof(SCRP_PCLRECT));

		//topLevel分割
		result = RC_AreaDivTop(topLevel, aLeveltbl, &baseAreaTopLvInf);
		// 処理結果判定
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[makeAreaLv1ToLv2] RC_AreaDivTop error.[0x%08x]"HERE, result);
			break;
		}
	} while (0);

	// start密度データ解放
	if (NULL != startAreaDensity.data) {
		RP_MemFree(startAreaDensity.data, e_MEM_TYPE_ROUTEPLAN);
		startAreaDensity.data = NULL;
	}
	// goal密度データ解放
	if (NULL != goalAreaDensity.data) {
		RP_MemFree(goalAreaDensity.data, e_MEM_TYPE_ROUTEPLAN);
		goalAreaDensity.data = NULL;
	}
	return (result);
}

/**
 * Lv2基準エリア生成処理
 * @param BaseAreaLv1 		[I]Lv1の基準エリア
 * @param areaType 			[I]エリアタイプ（OD,Oのみ,Dのみ,ODなし）
 * @param BaseAreaLv2 		[O]Lv2基準エリア作成結果
 * @param odXSftLv1			[I]Lv1出発地目的地横シフト量
 * @param odYSftLv1			[I]Lv1出発地目的地縦シフト量
 * @note 3*3～4*4の範囲でエリアは構築する
 *      Lv1の基準エリアがすっぽりLv2一枚に収まっている場合最低3*3となる
 */
static E_SC_RESULT makeSideBaseAreaLv2(T_AreaInfo *BaseAreaLv1,E_RC_AREATYPE areaType, SCRP_LEVELAREA *BaseAreaLv2, UINT32 odXSftLv1, UINT32 odYSftLv1) {

	E_SC_RESULT result = e_SC_RESULT_FAIL;

	if ( NULL == BaseAreaLv1 || NULL == BaseAreaLv2) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[makeSideBaseAreaLv2] Bad param."HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	/*渡された基準エリアの四隅をレベル2にあげ
	 * 上位で6枚以上またがっていれば、4*4になるように
	 */
	UINT32 Lv1LB_ParcelId = BaseAreaLv1->pclRect.parcelId;						//Lv1基準左下パーセルID
	INT16 Lv1BaseShiftX = (BaseAreaLv1->pclRect.xSize) - 1;
	INT16 Lv1BaseShiftY = (BaseAreaLv1->pclRect.ySize) - 1;
	UINT32 Lv1RT_ParcelId = SC_MESH_SftParcelId(Lv1LB_ParcelId, Lv1BaseShiftX, Lv1BaseShiftY);	//Lv1基準右上パーセルID

	UINT32 Upper_LBparcelId = SC_MESH_GetUpperParcelID(Lv1LB_ParcelId);
	UINT32 Upper_RTparcelId = SC_MESH_GetUpperParcelID(Lv1RT_ParcelId);

	INT32 xSft = 0;
	INT32 ySft = 0;
	INT16 xExt = 0;
	INT16 yExt = 0;

	SC_MESH_GetAlterPos(Upper_LBparcelId, Upper_RTparcelId, RP_LEVEL2, &xSft, &ySft);

	//Lv1の基準エリアを覆うように
	UINT32 baseId = Upper_LBparcelId;
	UINT16 xSize = xSft +1;
	UINT16 ySize = ySft +1;

	//最低限基準ができたので、正方形にする
	INT16 baseAreaExtType;		//0:対角拡張、1:上下拡張、2:左右拡張、3:2枚ずつ拡張

	if(2==xSft && 2 == ySft){
		baseAreaExtType = AREA_EXTENT_DIAGONAL;
	}else if(2 == xSft && 1 == ySft){
		baseAreaExtType = AREA_EXTENT_SN;
	}else if(1 == xSft && 2 == ySft){
		baseAreaExtType = AREA_EXTENT_EW;
	}else if(1 == xSft && 1 == ySft){
		baseAreaExtType = AREA_EXTENT_DIAGONAL;
	}else if(1 == xSft && 0 == ySft){
		baseAreaExtType = AREA_EXTENT_SN;
	}else if(0 == xSft && 1 == ySft){
		baseAreaExtType = AREA_EXTENT_EW;
	}else if(0 == xSft && 0 == ySft){
		baseAreaExtType = AREA_EXTENT_ROUND;
	}else{
		SC_LOG_ErrorPrint(SC_TAG_RC, "[makeSideBaseAreaLv2]GetAlterPos bad result.xSft[%d]ySft[%d]"HERE,xSft,ySft);
		return  (e_SC_RESULT_FAIL);
	}

	//拡張方向を決定する
	result = extCheckBaseAreaLv2(areaType, odXSftLv1, odYSftLv1, &xExt, &yExt);
	if (e_SC_RESULT_SUCCESS != result) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[makeSideBaseAreaLv2] extCheckBaseAreaLv2 error.[0x%08x]"HERE, result);
		return (result);
	}

	switch(baseAreaExtType){
	case AREA_EXTENT_DIAGONAL:
		if(1 == xExt && 1 == yExt){
			BaseAreaLv2->pclRect.parcelId = baseId;
			BaseAreaLv2->pclRect.xSize = xSize +1;
			BaseAreaLv2->pclRect.ySize = ySize +1;
		}else if(-1 == xExt && 1 == yExt){
			BaseAreaLv2->pclRect.parcelId = SC_MESH_SftParcelId(baseId, -1, 0);
			BaseAreaLv2->pclRect.xSize = xSize +1;
			BaseAreaLv2->pclRect.ySize = ySize +1;
		}else if(1 == xExt && -1 == yExt){
			BaseAreaLv2->pclRect.parcelId = SC_MESH_SftParcelId(baseId, 0, -1);
			BaseAreaLv2->pclRect.xSize = xSize +1;
			BaseAreaLv2->pclRect.ySize = ySize +1;
		}else if(-1 == xExt && -1 == yExt){
			BaseAreaLv2->pclRect.parcelId = SC_MESH_SftParcelId(baseId, -1, -1);
			BaseAreaLv2->pclRect.xSize = xSize +1;
			BaseAreaLv2->pclRect.ySize = ySize +1;
		}else{
			SC_LOG_ErrorPrint(SC_TAG_RC, "[makeSideBaseAreaLv2] extCheckBaseAreaLv2 bad result.xExt[%d]yExt[%d]"HERE,xExt,yExt);
			return (e_SC_RESULT_FAIL);
		}
		break;
	case AREA_EXTENT_SN:
		if(1 == xExt){
			BaseAreaLv2->pclRect.parcelId = SC_MESH_SftParcelId(baseId, 0, -1);
			BaseAreaLv2->pclRect.xSize = xSize +1;
			BaseAreaLv2->pclRect.ySize = ySize +2;
		}else if(-1 == xExt){
			BaseAreaLv2->pclRect.parcelId = SC_MESH_SftParcelId(baseId, -1, -1);
			BaseAreaLv2->pclRect.xSize = xSize +1;
			BaseAreaLv2->pclRect.ySize = ySize +2;
		}else{
			SC_LOG_ErrorPrint(SC_TAG_RC, "[makeSideBaseAreaLv2] extCheckBaseAreaLv2 bad result.xExt[%d]"HERE,xExt);
			return (e_SC_RESULT_FAIL);
		}

		break;
	case AREA_EXTENT_EW:
		if(1 == yExt){
			BaseAreaLv2->pclRect.parcelId = SC_MESH_SftParcelId(baseId, -1, 0);
			BaseAreaLv2->pclRect.xSize = xSize +2;
			BaseAreaLv2->pclRect.ySize = ySize +1;
		}else if(-1 == yExt){
			BaseAreaLv2->pclRect.parcelId = SC_MESH_SftParcelId(baseId, -1, -1);
			BaseAreaLv2->pclRect.xSize = xSize +2;
			BaseAreaLv2->pclRect.ySize = ySize +1;
		}else{
			SC_LOG_ErrorPrint(SC_TAG_RC, "[makeSideBaseAreaLv2] extCheckBaseAreaLv2 bad result.yExt[%d]"HERE,yExt);
			return (e_SC_RESULT_FAIL);
		}

		break;
	case AREA_EXTENT_ROUND:
		BaseAreaLv2->pclRect.parcelId = SC_MESH_SftParcelId(baseId, -1, -1);
		BaseAreaLv2->pclRect.xSize = xSize +2;
		BaseAreaLv2->pclRect.ySize = ySize +2;

		break;
	default:
		SC_LOG_ErrorPrint(SC_TAG_RC, "[makeSideBaseAreaLv2]error baseAreaExtType[%d]."HERE,baseAreaExtType);
		return (e_SC_RESULT_FAIL);
		break;
	}

	return e_SC_RESULT_SUCCESS;
}
/**
 * Lv2基準エリア拡張方向決定
 * @param areaType 			[I]エリアタイプ（OD,Oのみ,Dのみ,ODなし）
 * @param odXSftLv1			[I]Lv1出発地目的地横シフト量
 * @param odYSftLv1			[I]Lv1出発地目的地縦シフト量
 * @param xExt				[O]拡張方向_横
 * @param yExt				[O]拡張方向_縦
 * @note 相対位置と反対側に拡張するようにフラグを詰める
 */
static E_SC_RESULT extCheckBaseAreaLv2(E_RC_AREATYPE areaType, INT16 odXSftLv1, INT16 odYSftLv1,INT16* xExt,INT16* yExt ) {

	E_SC_RESULT result = e_SC_RESULT_FAIL;

	if(NULL == xExt || NULL == yExt){
		SC_LOG_ErrorPrint(SC_TAG_RC, "[extCheckBaseAreaLv2] Bad param."HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (e_AREA_TYPE_O == areaType ) {
		//O側
		if(0 <= odXSftLv1){
			//拡張方向（左右）を-1とする
			*xExt =-1;
		}else{
			*xExt =1;
		}
		if(0 <= odYSftLv1){
			//拡張方向（上下）を-1にする
			*yExt =-1;
		}else{
			*yExt =1;
		}
		result = e_SC_RESULT_SUCCESS;
	}else if(e_AREA_TYPE_D == areaType){
		//D側
		if(0 <= odXSftLv1){
			//拡張方向（左右）を1とする
			*xExt =1;
		}else{
			*xExt =-1;
		}
		if(0 <= odYSftLv1){
			//拡張方向（上下）を1にする
			*yExt =1;
		}else{
			*yExt =-1;
		}
		result = e_SC_RESULT_SUCCESS;
	}else{
		SC_LOG_ErrorPrint(SC_TAG_RC, "[extCheckBaseAreaLv2] Bad param.areaType[%d]"HERE,areaType);
		result = e_SC_RESULT_BADPARAM;
	}
return (result);
}
/**
 * @brief レベル2トップエリア作成処理
 * @param [I]レベル2_O側基準エリア
 * @param [I]レベル2_D側基準エリア
 * @param [O]結果エリア(AREA_MAKE_FINISHの場合のみ値を保障)
 * @param [O]密度情報
 * @param [O]エリア作成ステータス
 * @return e_SC_RESULT_SUCCESS 正常終了
 *         e_SC_RESULT_SUCCESS以外 異常終了
 * @memo TODO エリア拡張後の枚数制限
 * @memo 注意：エリアを基準包含エリアよりかも小さくはしないように
 */
static E_SC_RESULT makeAreaLv2Top(SCRP_LEVELAREA* aOAreaLv2, SCRP_LEVELAREA* aDAreaLv2, SCRP_LEVELAREA* aTopLv2) {
	E_SC_RESULT result = e_SC_RESULT_FAIL;

	SCRP_LEVELAREA topAreaLv2 = {};

	INT16 ratioSft[2] = {};
	INT16 ratioAdd[2] = {};
	UINT32 xReal = 0;
	UINT32 yReal = 0;
	UINT32 *real[2] = {};
	UINT32 realBasesY = 0;
	UINT8 iRatio; // 縦横比

	INT32 range = AREATOP_RATIO_RANGE;

	if (NULL == aOAreaLv2 || NULL == aDAreaLv2 || NULL == aTopLv2) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[makeAreaLv2Top]bad param ."HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// OD最低包含エリア作成
	result = makeCoverAreaLv2(aOAreaLv2, aDAreaLv2, &topAreaLv2);
	if (e_SC_RESULT_SUCCESS != result) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[makeAreaLv2Top] makeCoverAreaLv2 error.[0x%08x]"HERE, result);
		return (result);
	}

	// 実長取得 topAreaからの場合リンクデータ無しの可能性が少し高くなる為O側から取得
	realBasesY = RP_Lib_GetParcelRealXYRatio(aOAreaLv2->pclRect.parcelId, LV1ODBASE_REAL_Y_DEF) * 100.0f;

	// XY比から正方形に近づける
	xReal = topAreaLv2.pclRect.xSize * 100;
	yReal = topAreaLv2.pclRect.ySize * realBasesY;

	if (xReal < yReal) {
		ratioSft[0] = -1;
		ratioAdd[0] = 2;
		real[0] = &xReal;
		real[1] = &yReal;
		iRatio = ((FLOAT) *real[0] / (FLOAT) *real[1]) * 100;
	} else {
		ratioSft[1] = -1;
		ratioAdd[1] = 2;
		real[0] = &yReal;
		real[1] = &xReal;
		iRatio = ((FLOAT) *real[0] / (FLOAT) *real[1]) * 100;
	}
	// XY比をAREA_RATIO_RANGE超えさせる
	while (iRatio <= range) {

		topAreaLv2.pclRect.xSize += ratioAdd[0];
		topAreaLv2.pclRect.ySize += ratioAdd[1];
		topAreaLv2.pclRect.parcelId = SC_MESH_SftParcelId(topAreaLv2.pclRect.parcelId, ratioSft[0], ratioSft[1]);

		xReal = topAreaLv2.pclRect.xSize * 100;
		yReal = topAreaLv2.pclRect.ySize * realBasesY;
		iRatio = ((FLOAT) *real[0] / (FLOAT) *real[1]) * 100;
	}
	if (e_SC_RESULT_SUCCESS == result) {
		aTopLv2->pclRect.parcelId = topAreaLv2.pclRect.parcelId;
		aTopLv2->pclRect.xSize = topAreaLv2.pclRect.xSize;
		aTopLv2->pclRect.ySize = topAreaLv2.pclRect.ySize;
	}
	return (result);
}

/**
 * @brief	基準エリア２つから包含エリアを作成
 * @param	[I]Lv2基準１
 * @param	[I]Lv2基準２
 * @param	[O]Lv2包含エリア格納用
 *
 * ■を包含するエリアを作成
 *  □□■■  →  ■■■■
 *  □□■■  →  ■■■■
 *  ■■□□  →  ■■■■
 *  ■■□□  →  ■■■■
 */
static E_SC_RESULT makeCoverAreaLv2(SCRP_LEVELAREA* aBase1Lv2, SCRP_LEVELAREA* aBase2Lv2, SCRP_LEVELAREA* topAreaLv2)
{
	INT32 level = 0;		// Level
	INT32 xSft = 0;			// X方向シフト量
	INT32 ySft = 0;			// Y方向シフト量
	UINT32 lbId = 0;		// パーセルID
	UINT32 rtId = 0;		// パーセルID
	UINT32 rtIdBase1 = 0;	// パーセルID
	UINT32 rtIdBase2 = 0;	// パーセルID

	if ((NULL == aBase1Lv2) || (NULL == aBase2Lv2) || (NULL == topAreaLv2)) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[makeCoverAreaLv2] Bad param."HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// レベル取得
	level = SC_MESH_GetLevel(aBase1Lv2->pclRect.parcelId);
	if (level != SC_MESH_GetLevel(aBase2Lv2->pclRect.parcelId)) {
		// レベルが違う
		SC_LOG_ErrorPrint(SC_TAG_RC, "[makeCoverAreaLv2] Parcel level unmatch."HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	// 左下特定
	if (0 != SC_MESH_GetAlterPos(aBase1Lv2->pclRect.parcelId, aBase2Lv2->pclRect.parcelId, level, &xSft, &ySft)) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[makeCoverAreaLv2] SC_MESH_GetAlterPos error."HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	if (0 < xSft) {
		xSft = 0;
	}
	if (0 < ySft) {
		ySft = 0;
	}
	// 基準エリア格納
	lbId = SC_MESH_SftParcelId(aBase1Lv2->pclRect.parcelId, xSft, ySft);
	// 右上特定
	rtIdBase1 = SC_MESH_SftParcelId(aBase1Lv2->pclRect.parcelId, (aBase1Lv2->pclRect.xSize)-1, (aBase1Lv2->pclRect.ySize)-1);
	rtIdBase2 = SC_MESH_SftParcelId(aBase2Lv2->pclRect.parcelId, (aBase1Lv2->pclRect.xSize)-1, (aBase1Lv2->pclRect.ySize)-1);
	if (0 != SC_MESH_GetAlterPos(rtIdBase1, rtIdBase2, level, &xSft, &ySft)) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[makeCoverAreaLv2] SC_MESH_GetAlterPos error."HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (xSft < 0) {
		xSft = 0;
	}
	if (ySft < 0) {
		ySft = 0;
	}
	rtId = SC_MESH_SftParcelId(rtIdBase1, xSft, ySft);
	// 右上とのシフト量格納
	if (0 != SC_MESH_GetAlterPos(lbId, rtId, level, &xSft, &ySft)) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[makeCoverAreaLv2] SC_MESH_GetAlterPos error."HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	// シフト量＋１（枚数）を格納
	topAreaLv2->pclRect.parcelId = lbId;
	topAreaLv2->pclRect.xSize = (xSft + 1);
	topAreaLv2->pclRect.ySize = (ySft + 1);

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief	探索推奨レベルの判定
 * @param	[I]Lv1包含エリア情報
 * @param	[O]推奨レベル
 */
static E_SC_RESULT judgeRecommendLv(T_AreaInfo *topAreaLv1, INT8 *recLv,DOUBLE odDistance) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == topAreaLv1 || NULL == recLv) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[judgeRecommendLv] Bad param."HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	//強制的に探索レベルを決定する
	if (true == mOnlyLv1Flg) {
		//強制レベル1
		*recLv = RP_LEVEL1;
		return (e_SC_RESULT_SUCCESS);
	}
	//強制的に探索レベルを決定する
	if (true == mOnlyLv2Flg) {
		//強制レベル2
		*recLv = RP_LEVEL2;
		return (e_SC_RESULT_SUCCESS);
	}

	if(RC_JUDGE_RECOMMENDLV_DISTANCE >= odDistance){
		//OD距離が一定値以下なのでLv1にする
		*recLv = RP_LEVEL1;
		return (e_SC_RESULT_SUCCESS);
	}

	T_DHC_ROAD_DENSITY densityAreaPtr = {};

	//レベル1Top情報
	UINT32 lv1PclVol = topAreaLv1->pclRect.xSize * topAreaLv1->pclRect.ySize;
	UINT32 lv1LinkVol = 0;
	//リンク数取得
	E_SC_RESULT result = RC_GetParcelDensity(&densityAreaPtr, topAreaLv1->pclRect.parcelId, topAreaLv1->pclRect.xSize,
			topAreaLv1->pclRect.ySize);
	if (e_SC_RESULT_SUCCESS != result) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[judgeRecommendLv]getParcelDensity error. [0x%08x]"HERE, result);
		return (result);
	}
	lv1LinkVol = densityAreaPtr.totalDensity * SC_MA_REAL_DENSITY;

#if 0
	//ダンプ確認
	SC_LOG_InfoPrint(SC_TAG_RC, "[RC_Area]Lv1Top_linkVol	[%4d]", lv1LinkVol);
	SC_LOG_InfoPrint(SC_TAG_RC, "[RC_Area]Lv1Top_pclVol		[%4d]", lv1PclVol);
#endif

	//推奨レベル判定
	if (RC_JUDGE_RECOMMENDLV_LINK_MID <= lv1LinkVol) {
		//リンク判定最大をオーバーなのでLv2を推奨
		*recLv = RP_LEVEL2;
	} else if (RC_JUDGE_RECOMMENDLV_LINK_LOW <= lv1LinkVol && lv1LinkVol < RC_JUDGE_RECOMMENDLV_LINK_MID) {
		//リンク判定最小よりかは多い
		if (RC_JUDGE_RECOMMENDLV_PCL_LOW > lv1PclVol) {
			//枚数は少ないのでLv1を推奨
			*recLv = RP_LEVEL1;
		} else {
			//枚数は多めなのでLv2を推奨
			*recLv = RP_LEVEL2;
		}
	} else {
		//リンク数は少ない
		if (RC_JUDGE_RECOMMENDLV_PCL_MID <= lv1PclVol) {
			//枚数は多めなのでLv2を推奨
			*recLv = RP_LEVEL2;
		} else {
			//枚数少なくリンクも少ないのでLv1
			*recLv = RP_LEVEL1;
		}
	}
	// 個々の密度はいらないから解放
	if (NULL != densityAreaPtr.data) {
		RP_MemFree(densityAreaPtr.data, e_MEM_TYPE_ROUTEPLAN);
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * 沖縄パッチ範囲内判定
 * @param aONbor O側
 * @param aDNbor D側
 */
static Bool judgeInsideOkinawa(SCRP_NEIGHBORINFO* aONbor, SCRP_NEIGHBORINFO* aDNbor) {
#if _RP_PATCH_OKINAWAAREA

	DOUBLE oLat = 0;
	DOUBLE oLon = 0;
	DOUBLE dLat = 0;
	DOUBLE dLon = 0;
	DOUBLE len = 0;
	INT32 ret = 0;

	if (NULL == aONbor || NULL == aDNbor) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "error. "HERE);
		return (false);
	}

	ret = SC_MESH_ChgParcelIDToTitude(RP_LEVEL1, aONbor->point.parcelId, aONbor->point.x, aONbor->point.y, &oLat, &oLon);
	if (-1 == ret) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "error. "HERE);
		return (false);
	}
	len = sc_MESH_GetRealLen(oLat / 3600.0f, oLon / 3600.0f, PROMOTE_OKINAWA_LAT, PROMOTE_OKINAWA_LON);
	if (PROMOTE_OKINAWA_INSIDE < lround(len)) {
		return (false);
	}

	ret = SC_MESH_ChgParcelIDToTitude(RP_LEVEL1, aDNbor->point.parcelId, aDNbor->point.x, aDNbor->point.y, &dLat, &dLon);
	if (-1 == ret) {
		return (false);
	}
	len = sc_MESH_GetRealLen(dLat / 3600.0f, dLon / 3600.0f, PROMOTE_OKINAWA_LAT, PROMOTE_OKINAWA_LON);
	if (PROMOTE_OKINAWA_INSIDE < lround(len)) {
		return (false);
	}

	return (true);
#else
	return (false);
#endif /* PATCH_OKINAWA_ */
}

/**
 * @brief 旧テーブル(SCRP_LEVELTBL_OLD)を新テーブル(SCRP_LEVELTBL)へフォーマット変換を行う
 * @param レベル管理
 * @param 旧レベルテーブル
 */
static E_SC_RESULT convertLevelTblToOld(SCRP_LEVELTBL *aLevel, SCRP_LEVELTBL_OLD *aLevelOld) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	UINT16 count = 0;
	UINT16 idx = 0;
	UINT32 i = 0;

	if (NULL == aLevel || NULL == aLevelOld || 0 == aLevelOld->topArea.divAreaVol) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// トップレベルコピー(必ずLevel1であるはず)
	aLevel->topLevel = aLevelOld->topLevel;

	// エリア矩形コピー
	RP_Memcpy(&aLevel->areaTable[0].pclRect, &aLevelOld->topArea.pclRect, sizeof(SCRP_PCLRECT));
	aLevel->areaTable[0].divVol = aLevelOld->topArea.divAreaVol;
	aLevel->areaTable[0].divIdx = 0;

	// 分割データ格納領域確保
	aLevel->divInfoVol = aLevelOld->topArea.divAreaVol;
	aLevel->divInfo = RP_MemAlloc(sizeof(SCRP_DIVAREA) * aLevelOld->topArea.divAreaVol, e_MEM_TYPE_ROUTEPLAN);
	if (NULL == aLevel->divInfo) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error. "HERE);
		return (e_SC_RESULT_MALLOC_ERR);
	}
	RP_Memset0(aLevel->divInfo, sizeof(SCRP_DIVAREA) * aLevelOld->topArea.divAreaVol);

	// 分割データ・パーセル情報コピー
	for (i = 0; i < aLevelOld->topArea.divAreaVol; i++) {
		RP_Memcpy(&aLevel->divInfo[i].pclRect, &aLevelOld->topArea.divArea_p[i].pclRect, sizeof(SCRP_PCLRECT));
		aLevel->divInfo[i].pclVol = aLevelOld->topArea.divArea_p[i].parcelVol;
		aLevel->divInfo[i].pclIdx = count;
		count += aLevelOld->topArea.divArea_p[i].parcelVol;
	}

	// パーセル状態領域確保
	aLevel->pclStateVol = aLevelOld->topArea.parcelVol;
	aLevel->pclState = RP_MemAlloc(sizeof(SCRP_AREAPCLSTATE) * aLevelOld->topArea.parcelVol, e_MEM_TYPE_ROUTEPLAN);
	if (NULL == aLevel->pclState) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error. "HERE);
		return (e_SC_RESULT_MALLOC_ERR);
	}

	// パーセル状態コピー
	RP_Memset0(aLevel->pclState, sizeof(SCRP_AREAPCLSTATE) * aLevelOld->topArea.parcelVol);
	for (i = 0; i < aLevelOld->topArea.divAreaVol; i++) {
		RP_Memcpy(&aLevel->pclState[idx], aLevelOld->topArea.divArea_p[i].parcel_p,
				sizeof(SCRP_AREAPCLSTATE) * aLevelOld->topArea.divArea_p[i].parcelVol);
		idx += aLevelOld->topArea.divArea_p[i].parcelVol;
	}
#if 0
	// ダンプ確認
	SC_LOG_InfoPrint(SC_TAG_RC, "divinfovol  = %d  ,%d", aLevel->divInfoVol, aLevelOld->topArea.divAreaVol);
	SC_LOG_InfoPrint(SC_TAG_RC, "pclstatevol = %d  ,%d", aLevel->pclStateVol, aLevelOld->topArea.parcelVol);
	SC_LOG_InfoPrint(SC_TAG_RC, "divinfo");
	for (i = 0; i < aLevel->divInfoVol; i++) {
		SC_LOG_InfoPrint(SC_TAG_RC, "pcl = 0x%08x(%3d,%3d)  ,0x%08x(%3d,%3d)" //
				, aLevel->divInfo[i].pclRect.parcelId // パーセルID
				, aLevel->divInfo[i].pclRect.xSize // X
				, aLevel->divInfo[i].pclRect.ySize // Y
				, aLevelOld->topArea.divArea_p[i].pclRect.parcelId // パーセルID
				, aLevelOld->topArea.divArea_p[i].pclRect.xSize // X
				, aLevelOld->topArea.divArea_p[i].pclRect.ySize // Y
				);
		SC_LOG_InfoPrint(SC_TAG_RC, "vol = (%3d)  ,(%3d)", aLevel->divInfo[i].pclVol, aLevelOld.topArea.divArea_p[i].parcelVol);
		SC_LOG_InfoPrint(SC_TAG_RC, "idx = (%3d)", aLevel->divInfo[i].pclIdx);
		SC_LOG_InfoPrint(SC_TAG_RC, "state");
		UINT16 u;
		for (u = 0; u < aLevel->divInfo[i].pclVol; u++) {
			SC_LOG_InfoPrint(SC_TAG_RC, "state = (0x%04x,0x%04x,%03d),(0x%04x,0x%04x,%03d)" //
					, aLevel->pclState[aLevel->divInfo[i].pclIdx + u].join_f // 接続フラグ
					, aLevel->pclState[aLevel->divInfo[i].pclIdx + u].split_f // 断裂フラグ
					, aLevel->pclState[aLevel->divInfo[i].pclIdx + u].linkDensity // 密度
					, aLevelOld.topArea.divArea_p[i].parcel_p[u].join_f // 接続フラグ
					, aLevelOld.topArea.divArea_p[i].parcel_p[u].split_f // 断裂フラグ
					, aLevelOld.topArea.divArea_p[i].parcel_p[u].linkDensity // 密度
					);
		}
	}
#endif

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}
