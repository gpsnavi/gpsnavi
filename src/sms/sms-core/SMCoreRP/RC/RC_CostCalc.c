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
 * RP_CostCalc.c
 *
 *  Created on: 2015/11/06
 *      Author: masutani
 */

#include "sms-core/SMCoreRP/SMCoreRPInternal.h"

#define SCRP_COSTCALCFUNKLIST_SIZE (8) // コスト計算関数リスト数
// コスト計算関数ポインタ型
typedef E_SC_RESULT (*SCRP_COSTCALC_F)(SCRP_NETCONTROLER*, SCRC_IOCALCTBL*, SCRC_RESULTCOSTS*);

// コスト計算関数テーブル
typedef struct _SCRP_COSTFUNCLIST {
	SCRP_COSTCALC_F func;
} SCRP_COSTFUNCLIST;

// コスト計算データテーブル
typedef struct _SCRP_COSTDATA {
	UINT16 ferryCost;						// フェリーコスト
	UINT16 timeRegCost;						// 時間規制コスト
	UINT16 seasonRegCost;					// 季節規制コスト
	UINT16 advanceLinkCost;					// その他特殊コスト

	UINT32 baseCost[ROADTYPE_SIZE][LINKTYPE_SIZE];
	UINT32 costUnit[ROADTYPE_SIZE][LINKTYPE_SIZE];
	UINT32 angleCost[TRI_PART_SIZE][TRI_PART_SIZE];
	UINT32 angleApplyFlag[ROADTYPE_SIZE][ROADTYPE_SIZE];

	SCRP_COSTFUNCLIST costCalcFuncList[SCRP_COSTCALCFUNKLIST_SIZE];
} SCRP_COSTDATA;

/*-------------------------------------------------------------------
 * 宣言
 *-------------------------------------------------------------------*/
static E_SC_RESULT setCostCalcData(UINT32 mode);
static E_SC_RESULT calcAngleCost(SCRP_NETCONTROLER* aNetCtrl, SCRC_IOCALCTBL* aInOut, SCRC_RESULTCOSTS* aCosts);
static E_SC_RESULT calcTravelCostDistance(SCRP_NETCONTROLER* aNetCtrl, SCRC_IOCALCTBL* aInOut, SCRC_RESULTCOSTS* aCosts);
static E_SC_RESULT calcNormalPenalty(SCRP_NETCONTROLER* aNetCtrl, SCRC_IOCALCTBL* aInOut, SCRC_RESULTCOSTS* aCosts);
static E_SC_RESULT calcTravelCostTime(SCRP_NETCONTROLER* aNetCtrl, SCRC_IOCALCTBL* aInOut, SCRC_RESULTCOSTS* aCosts);
static E_SC_RESULT calcRegulationCost(SCRP_NETCONTROLER* aNetCtrl, SCRC_IOCALCTBL* aInOut, SCRC_RESULTCOSTS* aCosts);
static E_SC_RESULT calcDummyFunc(SCRP_NETCONTROLER* aNetCtrl, SCRC_IOCALCTBL* aInOut, SCRC_RESULTCOSTS* aCosts);
static E_SC_RESULT getP2PAngle(SCRP_POINT *aPoint1, SCRP_POINT *aPoint2, DOUBLE* aAngle);

// コストデータ
static SCRP_COSTDATA mCostData = {};

/**
 * コスト計算用関数設定
 * @param 探索設定情報
 */
E_SC_RESULT RC_CostCalcFuncSet(UINT32 cond) {

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	// コスト計算関数配列初期化
	RP_Memset0(&mCostData.costCalcFuncList[0], sizeof(SCRP_COSTFUNCLIST) * SCRP_COSTCALCFUNKLIST_SIZE);

	//探索条件別処理対象格納
	switch (cond) {
	case RPC_HIGHWAY:			// 高速
		mCostData.costCalcFuncList[0].func = calcTravelCostTime;
		mCostData.costCalcFuncList[1].func = calcAngleCost;
		mCostData.costCalcFuncList[2].func = calcDummyFunc;
		break;
	case RPC_NORMAL:			// 一般
		mCostData.costCalcFuncList[0].func = calcTravelCostTime;
		mCostData.costCalcFuncList[1].func = calcAngleCost;
		mCostData.costCalcFuncList[2].func = calcNormalPenalty;
		break;
	case RPC_TIME:				// 時間
		mCostData.costCalcFuncList[0].func = calcTravelCostTime;
		mCostData.costCalcFuncList[1].func = calcAngleCost;
		mCostData.costCalcFuncList[2].func = calcDummyFunc;
		break;
	case RPC_DISTANCE:			// 距離
		mCostData.costCalcFuncList[0].func = calcTravelCostDistance;
		mCostData.costCalcFuncList[1].func = calcDummyFunc;
		mCostData.costCalcFuncList[2].func = calcDummyFunc;
		break;
	case RPC_INVALID:
		SC_LOG_ErrorPrint(SC_TAG_RC, "route plan cond is RPC_INVALID"HERE);
		return (e_SC_RESULT_FAIL);
	default:
		SC_LOG_ErrorPrint(SC_TAG_RC, "route plan cond is unknown. (%d)"HERE, cond);
		return (e_SC_RESULT_FAIL);
	}

	// 共有メモリから探索コストを取得する
	E_SC_RESULT result = setCostCalcData(cond);
	if (e_SC_RESULT_SUCCESS != result) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "setCostCalcData error. [0x%08x] "HERE, result);
		return (result);
	}

#if _RP_LOG_CALCCOST
	RPDBG_ShowCalcCost(&mCostData.baseCost[0][0], &mCostData.costUnit[0][0], &mCostData.angleCost[0][0], &mCostData.angleApplyFlag[0][0]);
#endif
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief コスト計算用関数設定
 * @param 探索設定情報
 */
static E_SC_RESULT setCostCalcData(UINT32 mode) {

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SMRTCOSTINFO rtCostInfo = {};
	SMRTCOST *cost_p = NULL;
	UINT32 turn_wk[TRI_PART_SIZE] = {};
	INT32 i, e;
	INT32 cnt = 0;

	if (SC_SRCHMODE_NUM <= mode) {
		// iniファイルの内容とソフトのVerに不整合がある場合の探索失敗を防ぐ
		SC_LOG_ErrorPrint(SC_TAG_RC, "route plan code unknown. mode(%d)"HERE, mode);
		return (e_SC_RESULT_BADPARAM);
	}

	// 共有メモリから探索コスト取得
	result = SC_MNG_GetRouteCostInfo(&rtCostInfo);
	if (e_SC_RESULT_SUCCESS != result) {
		return (e_SC_RESULT_FAIL);
	}

	// 指定探索モードの探索コスト情報取得
	cost_p = &(rtCostInfo.routeCost[mode]);

	// 静的旅行時間設定
	for (i = 0; i < ROADTYPE_SIZE; i++) {
		for (e = 0; e < LINKTYPE_SIZE; e++) {
			if (0 < cost_p->speed[i][e]) {
				mCostData.baseCost[i][e] = 3600 / cost_p->speed[i][e];
			} else {
				return (e_SC_RESULT_FAIL);
			}
		}
	}

	// 乗算コスト設定
	for (i = 0; i < ROADTYPE_SIZE; i++) {
		for (e = 0; e < LINKTYPE_SIZE; e++) {
			mCostData.costUnit[i][e] = cost_p->weight[i][e];
		}
	}

	// 角度コスト設定
	for (i = 0; i < TRI_PART_SIZE; i++) {
		if (SC_TURNTYPE_NUM <= i) {
			turn_wk[i] = cost_p->turn[0];
		} else {
			turn_wk[i] = cost_p->turn[i];
		}
	}
	for (i = 0; i < TRI_PART_SIZE; i++) {
		cnt = 0;
		for (e = i; e < TRI_PART_SIZE; e++) {
			mCostData.angleCost[i][e] = turn_wk[cnt++];
		}
		cnt = TRI_PART_SIZE - 1;
		for (e = i; e >= 0; e--) {
			mCostData.angleCost[i][e] = turn_wk[cnt--];
		}
	}

	// 角度コスト適用有無設定
	for (i = 0; i < ROADTYPE_SIZE; i++) {
		for (e = 0; e < ROADTYPE_SIZE; e++) {
			mCostData.angleApplyFlag[i][e] = cost_p->turn_apply_f[i][e];
		}
	}
	return (e_SC_RESULT_SUCCESS);
}

#if 0
/**
 * @brief	コスト計算関数
 * @param	[I]探索情報テーブル
 * @param	[I]侵入退出情報
 * @param	[O]コスト結果
 */
E_SC_RESULT RC_CostCalc(SCRP_NETCONTROLER* aNetTab, SCRC_IOCALCTBL* aInOut, SCRC_RESULTCOSTS* aCosts) {

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	UINT16 i = 0;
	SCRP_COSTCALC_F func = mCostData.costCalcFuncList[i++].func;

	aCosts->advanceCost = 0;
	aCosts->crossCost = 0;
	aCosts->travelCost = 0;
	aCosts->totalCost = 0;

	while (NULL != func) {
		result = func(aNetTab, aInOut, aCosts);
		if (e_SC_RESULT_SUCCESS != result) {
			aCosts->advanceCost = 0;
			aCosts->crossCost = 0;
			aCosts->travelCost = 0;
			aCosts->totalCost = ALL_F32;
			SC_LOG_ErrorPrint(SC_TAG_RC, "[Cost] func error. state(0x%x)"HERE, result);
			SC_LOG_DebugPrint(SC_TAG_RC, "[Cost] func error. loop[%d]", i);
			break;
		}
		func = mCostData.costCalcFuncList[i++].func;
	}
	return (result);
}
#else

/**
 * @brief コスト計算関数
 * @param ネットワーク管理
 * @param 侵入退出情報
 * @param [O]コスト結果
 */
E_SC_RESULT RC_CostCalc(SCRP_NETCONTROLER* aNetCtrl, SCRC_IOCALCTBL* aInOut, SCRC_RESULTCOSTS* aCosts) {

	aCosts->advanceCost = 0;
	aCosts->crossCost = 0;
	aCosts->travelCost = 0;
	aCosts->totalCost = 0;

	// 各コスト計算関数呼び出し
	mCostData.costCalcFuncList[0].func(aNetCtrl, aInOut, aCosts);
	mCostData.costCalcFuncList[1].func(aNetCtrl, aInOut, aCosts);
	mCostData.costCalcFuncList[2].func(aNetCtrl, aInOut, aCosts);

	// 合算
	aCosts->totalCost = (aCosts->crossCost + aCosts->travelCost + aCosts->advanceCost);

	return (e_SC_RESULT_SUCCESS);
}
#endif

/**
 * @brief コスト計算関数：旅行時間のみ算出
 * @param ネットワーク管理
 * @param 侵入退出情報（退出情報のみでOK）
 * @param [O]コスト結果
 */
E_SC_RESULT RC_NoCrossCostCalc(SCRP_NETCONTROLER* aNetCtrl, SCRC_IOCALCTBL* aInOut, SCRC_RESULTCOSTS* aCosts) {

	aCosts->advanceCost = 0;
	aCosts->crossCost = 0;
	aCosts->travelCost = 0;
	aCosts->totalCost = 0;

	// 旅行時間算出のみを行う
	calcTravelCostTime(aNetCtrl, aInOut, aCosts);

	// 合算
	aCosts->totalCost = aCosts->travelCost;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 交差点コスト計算関数
 * @param ネットワーク管理
 * @param 侵入退出情報
 * @param [O]コスト結果
 * @memo 角度コスト＋交差点コスト
 *       0.1秒コスト
 */
static E_SC_RESULT calcAngleCost(SCRP_NETCONTROLER* aNetCtrl, SCRC_IOCALCTBL* aInOut, SCRC_RESULTCOSTS* aCosts) {
	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SCRP_POINT point1 = {};
	SCRP_POINT point2 = {};
	UINT32 inKind = 0;
	UINT32 outKind = 0;
	UINT16 outDir = 0;
	UINT16 inDir = 0;
	DOUBLE angle = 0;

	// 侵入/退出 道路種別取得
	inKind = SC_MA_D_BASE_LINK_GET_ROAD_TYPE(aInOut->inLinkBin);
	outKind = SC_MA_D_BASE_LINK_GET_ROAD_TYPE(aInOut->outLinkBin);

	if (1 == mCostData.angleApplyFlag[inKind][outKind]) {
		// 侵入リンク（逆）角度
		if (RCID_GET_ORIDX(aInOut->inLink->linkIndex)) {
			inDir = SC_MA_D_NWRCD_LINK_GET_STDIR(aInOut->inLinkBin);
		} else {
			inDir = SC_MA_D_NWRCD_LINK_GET_EDDIR(aInOut->inLinkBin);
		}
		// 退出リンク角度
		if (RCID_GET_ORIDX(aInOut->outLink->linkIndex)) {
			outDir = SC_MA_D_NWRCD_LINK_GET_EDDIR(aInOut->outLinkBin);
		} else {
			outDir = SC_MA_D_NWRCD_LINK_GET_STDIR(aInOut->outLinkBin);
		}

		// 角度不明（形状データを利用しないため低精度）
		if (0x000000FF == inDir) {
			point1.parcelId = aInOut->inLink->pclInfo->parcelId;
			point1.x = SC_MA_D_NWRCD_LINK_GET_ST_X(aInOut->inLinkBin);
			point1.y = SC_MA_D_NWRCD_LINK_GET_ST_Y(aInOut->inLinkBin);
			point2.parcelId = aInOut->inLink->pclInfo->parcelId;
			point2.x = SC_MA_D_NWRCD_LINK_GET_ED_X(aInOut->inLinkBin);
			point2.y = SC_MA_D_NWRCD_LINK_GET_ED_Y(aInOut->inLinkBin);

			if (RCID_GET_ORIDX(aInOut->inLink->linkIndex)) {
				result = getP2PAngle(&point1, &point2, &angle);
			} else {
				result = getP2PAngle(&point2, &point1, &angle);
			}
			if (e_SC_RESULT_SUCCESS != result) {
				aCosts->crossCost = 0;
				return (e_SC_RESULT_SUCCESS);
			}
			// 北0時計回り→東0反時計回りへ変換、1/360->1/17へ変換
			inDir = AGL_CALC_ANGLE(RP_ANGLE_N2E_CNV(angle));
		} else {
			// リンク方向データの正規化を解除し、1/360->1/17へ変換
			inDir = AGL_CALC_ANGLE(SC_MA_DENORMALIZE(inDir));
		}
		if (0x000000FF == outDir) {
			point1.parcelId = aInOut->outLink->pclInfo->parcelId;
			point1.x = SC_MA_D_NWRCD_LINK_GET_ST_X(aInOut->outLinkBin);
			point1.y = SC_MA_D_NWRCD_LINK_GET_ST_Y(aInOut->outLinkBin);
			point2.parcelId = aInOut->outLink->pclInfo->parcelId;
			point2.x = SC_MA_D_NWRCD_LINK_GET_ED_X(aInOut->outLinkBin);
			point2.y = SC_MA_D_NWRCD_LINK_GET_ED_Y(aInOut->outLinkBin);

			if (RCID_GET_ORIDX(aInOut->outLink->linkIndex)) {
				result = getP2PAngle(&point2, &point1, &angle);
			} else {
				result = getP2PAngle(&point1, &point2, &angle);
			}
			if (e_SC_RESULT_SUCCESS != result) {
				aCosts->crossCost = 0;
				return (e_SC_RESULT_SUCCESS);
			}
			// 北0時計回り→東0反時計回りへ変換、1/360->1/17へ変換
			outDir = AGL_CALC_ANGLE(RP_ANGLE_N2E_CNV(angle));
		} else {
			// リンク方向データの正規化を解除し、1/360->1/17へ変換
			outDir = AGL_CALC_ANGLE(SC_MA_DENORMALIZE(outDir));
		}

		// 角度テーブルからコスト算出
		aCosts->crossCost = mCostData.angleCost[inDir][outDir] / 100;

#if 0
	} else if (2 == flag) {
		// TODO 高速道路異常角度ペナルティ
#endif
	}

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 時間コスト計算関数
 * @param ネットワーク管理
 * @param 侵入退出情報
 * @param [O]コスト結果
 * @memo リンク長 * 旅行時間 * 条件依存倍率
 *       0.1秒コスト
 */
static E_SC_RESULT calcTravelCostTime(SCRP_NETCONTROLER* aNetCtrl, SCRC_IOCALCTBL* aInOut, SCRC_RESULTCOSTS* aCosts) {

	UINT16 binDist = SC_MA_D_BASE_LINK_GET_LINKDIST(aInOut->outLinkBin);
	UINT16 binTime = SC_MA_D_NWRCD_LINK_GET_TRAVELTIME(aInOut->outLinkBin);
	UINT16 rType = SC_MA_D_BASE_LINK_GET_ROAD_TYPE(aInOut->outLinkBin);
	UINT16 lType1 = SC_MA_D_BASE_LINK_GET_LINK1_TYPE(aInOut->outLinkBin);

	// 平均旅行時間取得(Ferryは1/10値)
	UINT32 time = SC_MA_GET_LINK_TRAVELTIME(binTime);
	// 探索種別乗算値
	UINT32 unit = mCostData.costUnit[rType][lType1];
	UINT32 base = mCostData.baseCost[rType][lType1];
	if (0 != time) {
		// 乗算(倍率で÷100)
		aCosts->travelCost = (time * unit) / 100;
	} else {
		// リンク長取得
		UINT32 dist = SC_MA_GET_LINK_DIST(binDist);
		// 乗算(静的旅行時間と倍率分で÷10000)
		aCosts->travelCost = (dist * base * unit) / 10000;
	}

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 距離コスト計算関数
 * @param ネットワーク管理
 * @param 侵入退出情報
 * @param [O]コスト結果
 */
static E_SC_RESULT calcTravelCostDistance(SCRP_NETCONTROLER* aNetCtrl, SCRC_IOCALCTBL* aInOut, SCRC_RESULTCOSTS* aCosts) {

	// リンク長取得
	UINT16 binDist = SC_MA_D_BASE_LINK_GET_LINKDIST(aInOut->outLinkBin);
	UINT32 dist = SC_MA_GET_LINK_DIST(binDist);
	// 乗算
	aCosts->travelCost = dist;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 高速乗りコスト
 * @param ネットワーク管理
 * @param 侵入退出情報
 * @param [O]コスト結果
 * @memo 一般優先で使用する
 */
static E_SC_RESULT calcNormalPenalty(SCRP_NETCONTROLER* aNetCtrl, SCRC_IOCALCTBL* aInOut, SCRC_RESULTCOSTS* aCosts) {

	// 退出が有料道路かつ侵入が一般の場合
	if (SC_MA_ROAD_TYPE_TOLLWAY >= SC_MA_D_BASE_LINK_GET_ROAD_TYPE(aInOut->outLinkBin)) {
		if (SC_MA_ROAD_TYPE_TOLLWAY < SC_MA_D_BASE_LINK_GET_ROAD_TYPE(aInOut->inLinkBin)) {
			aCosts->advanceCost = ON_TOLL_COST;
		}
	}
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 規制コスト計算関数
 * @param ネットワーク管理
 * @param 侵入退出情報
 * @param [O]コスト結果
 */
static E_SC_RESULT calcRegulationCost(SCRP_NETCONTROLER* aNetCtrl, SCRC_IOCALCTBL* aInOut, SCRC_RESULTCOSTS* aCosts) {
	// 時間規制内
	if (aInOut->reg.b_flag.timeRegIn) {
		aCosts->travelCost += mCostData.timeRegCost;
	}
	// 季節規制内
	if (aInOut->reg.b_flag.seasonRegIn) {
		aCosts->travelCost += mCostData.seasonRegCost;
	}
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief ダミー関数
 * @param ネットワーク管理
 * @param 侵入退出情報
 * @param [O]コスト結果
 */
static E_SC_RESULT calcDummyFunc(SCRP_NETCONTROLER* aNetCtrl, SCRC_IOCALCTBL* aInOut, SCRC_RESULTCOSTS* aCosts) {
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 点1点2を結ぶ直線の角度算出
 * @param 点1
 * @param 点2
 * @param 算出角度
 * @memo 点の正規化座標は緯度経度へ変換後角度を算出する
 */
static E_SC_RESULT getP2PAngle(SCRP_POINT *aPoint1, SCRP_POINT *aPoint2, DOUBLE* aAngle) {

	DOUBLE sLat, sLon;
	DOUBLE eLat, eLon;
	INT32 level = SC_MESH_GetLevel(aPoint1->parcelId);

	if (-1 == SC_MESH_ChgParcelIDToTitude(level, aPoint1->parcelId, (DOUBLE) aPoint1->x, (DOUBLE) aPoint1->y, &sLat, &sLon)) {
		return (e_SC_RESULT_FAIL);
	}
	if (-1 == SC_MESH_ChgParcelIDToTitude(level, aPoint2->parcelId, (DOUBLE) aPoint2->x, (DOUBLE) aPoint2->y, &eLat, &eLon)) {
		return (e_SC_RESULT_FAIL);
	}

	// 時に戻す
	sLat = (sLat / 3600) / 180 * M_PI;
	sLon = (sLon / 3600) / 180 * M_PI;
	eLat = (eLat / 3600) / 180 * M_PI;
	eLon = (eLon / 3600) / 180 * M_PI;

	// 緯度経度からラジアン
	DOUBLE y = cos(eLat) * sin(eLon - sLon);
	DOUBLE x = cos(sLat) * sin(eLat) - sin(sLat) * cos(eLat) * cos(eLon - sLon);
	DOUBLE rad = atan2(y, x);

	if (rad < 0.0) {
		rad = rad + (360.0 / 180.0 * M_PI);
	}
	*aAngle = rad * 180 / M_PI;

#if 0
	SC_LOG_DebugPrint(SC_TAG_RC, "1=%f,%f  2=%f,%f  xy=%f,%f angle=%f", sLat, sLon, eLat, eLon, x, y, *aAngle);
	SC_LOG_DebugPrint(SC_TAG_RC, "RC_AngleCalc. rad=%f angle=%f", rad, *aAngle);
#endif

	return (e_SC_RESULT_SUCCESS);
}


#if 0 // test
#define MAP_SIZE_2 MAP_SIZE * 2
#define MAP_SIZE_8 MAP_SIZE * 8
#define MAP_SIZE_32 MAP_SIZE * 32
#define MAP_SIZE_128 MAP_SIZE * 128
/**
 * @brief		ＰＩＤ座標系を緯度・経度に変換する
 *
 * @param		[in] lvl 変換するﾚﾍﾞﾙ
 * @param		[in] parcel_id 変換するﾊﾟｰｾﾙID
 * @param		[in] x 変換する正規化X座標
 * @param		[in] y 変換する正規化Y座標
 * @param		[out] latitude 変換後の緯度（秒）
 * @param		[out] longitude 変換後の経度（秒）
 * @return		パーセルID
 */
E_SC_RESULT SC_MESH_ChgParcelIDToTitude2(SMP2GPARAM *param) {

	switch (param->level) {
		case 1:
		param->latitude = ((DOUBLE) (((INT32) (param->parcelId >> 23)) & 0x000000ff) + (DOUBLE) ((param->parcelId >> 20) & 0x00000007) / 8
				+ (DOUBLE) ((param->parcelId >> 16) & 0x00000003) / 32 + (DOUBLE) param->y / MAP_SIZE_32) * 2400;
		param->longitude = ((DOUBLE) ((INT32) ((param->parcelId & 0x80000000) | ((param->parcelId << 15) & 0x7fc00000)) >> 22) + 100
				+ (DOUBLE) ((param->parcelId >> 4) & 0x00000007) / 8 + (DOUBLE) ((param->parcelId) & 0x00000003) / 32
				+ (DOUBLE) param->x / MAP_SIZE_32) * 3600;
		break;
		case 2:
		param->latitude = ((DOUBLE) (((INT32) (param->parcelId >> 23)) & 0x000000ff) + (DOUBLE) ((param->parcelId >> 20) & 0x00000007) / 8
				+ (DOUBLE) param->y / MAP_SIZE_8) * 2400;
		param->longitude = ((DOUBLE) ((INT32) ((param->parcelId & 0x80000000) | ((param->parcelId << 15) & 0x7fc00000)) >> 22) + 100
				+ (DOUBLE) ((param->parcelId >> 4) & 0x00000007) / 8 + (DOUBLE) param->x / MAP_SIZE_8) * 3600;
		break;
		default:
		return (e_SC_RESULT_BADPARAM);
	}

	return (e_SC_RESULT_SUCCESS);
}
#endif
