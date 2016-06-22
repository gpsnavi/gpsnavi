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
 * RP_Neighbor.c
 *
 *  Created on: 2015/11/06
 *      Author: masutani
 */

#include "sms-core/SMCoreRP/SMCoreRPInternal.h"

#if 1	/* AIKAWA */
static T_DHC_REQ_PARCEL sRPMapReqTab = {};		/* 探索内で唯一の地図リクエスト用変数 */
//static T_DHC_RES_DATA sRPMapResTab = {};			/* 探索内で唯一の地図応答用変数 */
#endif

/*-------------------------------------------------------------------
 * 定数定義
 *-------------------------------------------------------------------*/
#define rad2deg(a)				((a) / M_PI * 180.0)	/* rad を deg に */
#define deg2rad(a)				((a) / 180.0 * M_PI)	/* deg を rad に */
#define RP_NBR_LINKSELECTVOL	(10)					/* 近傍採択数 */
#define RP_NBR_LINKCLIPSIZE		(1200)					/* 近傍採択範囲 単位：m */
#define RP_NBR_LEAVECOST_O		(200)					/* m 20.0秒 */
#define RP_NBR_LEAVECOST_D		(120)					/* 12秒 */
#define RP_NBR_ROUNDCOST		(100)					/* 10.0秒 */
#define RP_NBR_REVERSECOST		(800)					/* 80.0秒 */
#define RP_NBR_MATCHLINKCOST	(9000)					/* 900.0秒 */
#define RP_NBR_BLOCKSIZE		(1500)					/* Block */
#define RP_NBR_POIGATE			(9000)					/* 施設入り口目的地用コスト 900.0秒 */

// O側D側を指定する trimNeighborLinkList()用
typedef enum _E_SIDEOD {
	e_SIDE_O = 0,		// O側
	e_SIDE_D			// D側
} E_SIDEOD;

/*-------------------------------------------------------------------
 * 構造体定義
 *-------------------------------------------------------------------*/
typedef struct _RC_CLIPAREA {
	UINT32 parcelId;				// パーセルID
	UINT16 lbX;						// パーセル内 クリップ左下X
	UINT16 lbY;						// パーセル内 クリップ左下Y
	UINT16 rtX;						// パーセル内 クリップ右上X
	UINT16 rtY;						// パーセル内 クリップ右上Y
	INT16 diffX;					// パーセル跨ぎ時の差分座標
	INT16 diffY;					// パーセル跨ぎ時の差分座標

	UINT32 clipListOfs;				// 形状配列オフセット
	UINT32 clipListVol;				// パーセル内クリップリンク数

	MAL_HDL shapeBin;				// 形状データ先頭
	UINT32 xRealLenTop;				// 実長データ(X方向上辺)：１正規化単位（m）の10,000倍
	UINT32 xRealLenBottom;			// 実長データ(X方向下辺)：１正規化単位（m）の10,000倍
	UINT32 yRealLenLeft;			// 実長データ(Y方向左辺)：１正規化単位（m）の10,000倍
	UINT32 yRealLenRight;			// 実長データ(Y方向右辺)：１正規化単位（m）の10,000倍
} RC_CLIPAREA;

typedef struct _RC_CLIPLINKLIST {
	RC_CLIPAREA *area;				// クリップエリア
	UINT32 areaVol;					// エリア数
	UINT32 listCurrent;				// 形状Ofs格納数
	UINT32 listSize;				// 形状Ofs配列数
	UINT32* list;					// 形状Ofs配列
	SCRP_POINT center;				// クリップ基準点
} RC_CLIPLINKLIST;

typedef struct _RC_VERTICALPARAM {
	DOUBLE cx;						// 点CのX
	DOUBLE cy;						// 点CのY
	DOUBLE ax;						// 線分AB始点X
	DOUBLE ay;						// 線分AB始点Y
	DOUBLE bx;						// 線分AB終点X
	DOUBLE by;						// 線分AB終点Y
	DOUBLE abAngle;					// 線分ABの角度
	DOUBLE abLen;					// 線分ABの距離
	DOUBLE vx;						// 線分ABへの点Cから垂線が降りる点V
	DOUBLE vy;						// 線分ABへの点Cから垂線が降りる点V
	DOUBLE cvLen;					// 線分ABへの点Cから垂線距離
	DOUBLE avRatio;					// 線分ABへの点Cから垂線が降りる点Vの内分率
} RC_VERTICALPARAM;

typedef struct _RC_NEIGHBORSELECT {
	INT16 tail;						// -1:データ無し 0～:格納データ末端
	UINT16 size;					// テーブルサイズ
	SCRP_NEIGHBORLINK list[RP_NBR_LINKSELECTVOL];
} RC_NEIGHBORSELECT;

// 道路種別→近傍収集区別番号 変換配列（国内用）
const static INT16 mSectCnv[16] = { 0, 0, 0, -1, 1, 1, 1, 1, 1, 1, 2, 2, -1, -1, -1, -1 };

/*-------------------------------------------------------------------
 * 宣言
 *-------------------------------------------------------------------*/
static E_SC_RESULT RC_MakeNeighborData(SCRP_POINT* aPoint, SCRP_NEIGHBORINFO* aNeighbor);
static Bool isNeighborSelectLink(T_MapShapeRecord* record);
static E_SC_RESULT sortNeighborList(SCRP_NEIGHBORLINK* list, UINT16 size);
static E_SC_RESULT makeNeighborLinkList(RC_CLIPLINKLIST* aLinkList, SCRP_NEIGHBORINFO *aNbrInfo);
static E_SC_RESULT calcVerticalPoint(RC_VERTICALPARAM *vp);
static E_SC_RESULT makeClipLinkList(SCRP_POINT* aCenter, UINT16 aSize, RC_CLIPLINKLIST** aList);
static E_SC_RESULT mallocClipLinkList(RC_CLIPLINKLIST* aList);
static void freeClipData(RC_CLIPLINKLIST* aList);
static E_SC_RESULT setOSideCost(SCRP_NEIGHBORINFO* aNbr, SCRP_SEARCHSETTING* aSetting, Bool aOnRoad);
static E_SC_RESULT setDSideCost(SCRP_NEIGHBORINFO* aNeighbor);
static E_SC_RESULT setDSideCostPoiGate(SCRP_NEIGHBORINFO* aNeighbor);
static E_SC_RESULT trimNeighborLinkList(SCRP_NEIGHBORINFO* aNeighbor, E_SIDEOD aPointType);
static E_SC_RESULT makeNeighborClipArea(SCRP_POINT* aBase, UINT16 aRadiusX, UINT16 aRadiusY, SCRP_POINT* aRtArea, SCRP_POINT* aLbArea);
static E_SC_RESULT setClipAreaInfo(RC_CLIPLINKLIST* aClip, SCRP_PCLRECT *aBase, SCRP_POINT* aRtArea, SCRP_POINT* aLbArea);
static E_SC_RESULT getRealLenTryLv2(UINT32 aParcelId, UINT32 *aBottom, UINT32 *aLeft);
static E_SC_RESULT getRealLen(UINT32 aParcelId, UINT32 *bottom, UINT32 *left);

/**
 * @brief 近傍情報にデータインデックス（ネットワークデータ）を格納する
 * @param aNetTab
 * @param aNeighbor
 * @note ネットワークデータのINDEXは0始まりに戻したものを格納する。
 * @note 地図データのINDEXは1始まり。
 */
E_SC_RESULT RC_SetNbrLinkIndex(SCRP_NEIGHBORINFO* aNeighbor) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aNeighbor) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SCRP_MAPREADTBL readTbl = {};
	UINT32 mapKind = SC_DHC_GetKindMask(e_DATA_KIND_ROAD);
	SCRP_NEIGHBORLINK* nbrLink = aNeighbor->neighborLink;
	UINT16 index = 0;
	UINT32 i = 0;

	for (i = 0; i < aNeighbor->nbrLinkVol; i++, nbrLink++) {

		// 地図読み込み
		result = RC_ReadListMap(&nbrLink->parcelId, 1, mapKind, &readTbl);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RC_ReadListMap error. [0x%08x] "HERE, result);
			break;
		}
		if (NULL == readTbl.mapList->road) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "road map not found... "HERE);
			result = e_SC_RESULT_MAP_GETERR;
			break;
		}

		index = SC_MA_BinSearchNwRecord(readTbl.mapList->road, nbrLink->linkId, SC_MA_BINSRC_TYPE_LINK);
		nbrLink->dataIndex = index - 1;

		// 地図解放
		result = RC_FreeMapTbl(&readTbl, true);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RC_FreeMapTbl error. [0x%08x] "HERE, result);
			break;
		}
		// 地図解放後に正常値かをチェック
		if (ALL_F16 == index) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "search index fail. link=0x%08x idx=0x%08x "HERE, nbrLink->linkId, index);
			result = e_SC_RESULT_FAIL;
			break;
		}
	}
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief 近傍情報のチェック
 * @param 区間管理
 * @memo 最近傍が同一リンクの場合エラーコードを設定し終了。
 *       １．第一区間：近傍リンクは垂線長が長い方を無効としてフラグを設定する。
 *           nouseフラグは基本最近傍にはつけないことに注意。
 *       ２．第二区間以降：出発近傍のすべてをD側では未使用とする
 */
E_SC_RESULT RC_SetNeighborLinkUseFlg(SCRP_SECTCONTROLER *aSectCtrl) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aSectCtrl) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	SCRP_NEIGHBORINFO* oInfo = &aSectCtrl->neighbor[0];
	SCRP_NEIGHBORINFO* dInfo = &aSectCtrl->neighbor[1];
	SCRP_NEIGHBORLINK* oNbrLink;
	SCRP_NEIGHBORLINK* dNbrLink;
	SCRP_NEIGHBORLINK* oMostNbrLink;
	SCRP_NEIGHBORLINK* dMostNbrLink;
	UINT32 i, e;

	// 同一リンクエラーチェック
	if (0 < oInfo->nbrLinkVol && 0 < dInfo->nbrLinkVol) {
		oNbrLink = oInfo->neighborLink;
		dNbrLink = dInfo->neighborLink;
		if (dNbrLink->parcelId == oNbrLink->parcelId && dNbrLink->linkId == oNbrLink->linkId) {
			// エラーコード設定
			RPC_SetErrorCode(EC_SAME_ROAD);
			return (e_SC_RESULT_FAIL);
		}
	}

	// ODそれぞれ最近傍取得
	oMostNbrLink = oInfo->neighborLink;
	dMostNbrLink = dInfo->neighborLink;

	oNbrLink = oInfo->neighborLink;
	for (i = 0; i < oInfo->nbrLinkVol; i++, oNbrLink++) {
		dNbrLink = dInfo->neighborLink;
		for (e = 0; e < dInfo->nbrLinkVol; e++, dNbrLink++) {
			// 第一区間
			if (0 == aSectCtrl->sectIndex) {
				if (dNbrLink->parcelId == oNbrLink->parcelId && dNbrLink->linkId == oNbrLink->linkId) {
					// O側最近傍の場合D側を使用しない
					if (oMostNbrLink->parcelId == oNbrLink->parcelId && oMostNbrLink->linkId == oNbrLink->linkId) {
						dNbrLink->noUse = 1;
						continue;
					}
					// D側最近傍の場合O側を使用しない
					if (dMostNbrLink->parcelId == dNbrLink->parcelId && dMostNbrLink->linkId == dNbrLink->linkId) {
						oNbrLink->noUse = 1;
						continue;
					}
					// その他は遠い方を使用しない
					if (dNbrLink->leavDist < oNbrLink->leavDist) {
						oNbrLink->noUse = 1;
					} else {
						dNbrLink->noUse = 1;
					}
				}
			}
			// 第二区間以降
			else {
				if (dNbrLink->parcelId == oNbrLink->parcelId && dNbrLink->linkId == oNbrLink->linkId) {
					dNbrLink->noUse = 1;
				}
			}
		}
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 指定地点近傍リンク採択処理
 * @param 地点
 * @param 近傍情報テーブル
 * @memo リンク長ゼロ近傍のトリムは行わない
 */
static E_SC_RESULT RC_MakeNeighborData(SCRP_POINT* aPoint, SCRP_NEIGHBORINFO* aNeighbor) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	RC_CLIPLINKLIST* clipLinkList = NULL;

	// プロセス登録
	RPC_SetCalcProcess(e_RC_PROC_NEIGHBOR);

	if (NULL == aNeighbor || NULL == aPoint) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 基準位置
	aNeighbor->point.parcelId = aPoint->parcelId;
	aNeighbor->point.x = aPoint->x;
	aNeighbor->point.y = aPoint->y;

	do {
		// 近傍リンク収集（特定矩形内のリンクを収集する）
		result = makeClipLinkList(aPoint, RP_NBR_LINKCLIPSIZE, &clipLinkList);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "makeClipLinkList error. [0x%08x] "HERE, result);
			break;
		}
		// 近傍リンクリストゼロ
		if (NULL == clipLinkList || 0 == clipLinkList->listCurrent) {
			SC_LOG_InfoPrint(SC_TAG_RC, "neighbor link is 0. "HERE);
			result = e_SC_RESULT_AROUND_NO_LINK;
			break;
		}

		// 近傍リンク採択
		result = makeNeighborLinkList(clipLinkList, aNeighbor);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "makeNeighborLinkList. [0x%08x] "HERE, result);
			break;
		}
		if (0 == aNeighbor->nbrLinkVol) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "Neigbor link not found. "HERE);
			result = e_SC_RESULT_AROUND_NO_LINK;
			break;
		}
	} while (0);

	// 解放
	if (NULL != clipLinkList) {
		freeClipData(clipLinkList);
		RP_MemFree(clipLinkList, e_MEM_TYPE_ROUTEPLAN);
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief 開始地点近傍作成
 * @param 探索条件
 * @param 地点インデックス
 * @param [O]近傍情報
 */
E_SC_RESULT RC_NeighborOSide(SCRP_SEARCHSETTING* aSetting, UINT8 aPointIdx, SCRP_NEIGHBORINFO* aNeighbor) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SCRP_POINT point = {};

	// プロセス登録
	RPC_SetCalcProcess(e_RC_PROC_NEIGHBOR);

	if (NULL == aNeighbor || NULL == aSetting) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 中心座標取得
	point.parcelId = aSetting->rpPoint[aPointIdx].parcelId;
	point.x = aSetting->rpPoint[aPointIdx].x;
	point.y = aSetting->rpPoint[aPointIdx].y;

	do {
		// 近傍リンク採択
		result = RC_MakeNeighborData(&point, aNeighbor);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RC_MakeNeighborData error. [0x%08x] "HERE, result);
			break;
		}

		// リンク長ゼロ近傍をトリムする
		result = trimNeighborLinkList(aNeighbor, e_SIDE_O);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "trimNeighborLinkList error. [0x%08x] "HERE, result);
			break;
		}
		// 初期コスト付加
		result = setOSideCost(aNeighbor, aSetting, aSetting->car.onRoad);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "setOSideCost error. [0x%08x] "HERE, result);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief 目的地近傍作成
 * @param 探索条件
 * @param 地点インデックス
 * @param [O]近傍情報
 */
E_SC_RESULT RC_NeighborDSide(SCRP_SEARCHSETTING* aSetting, UINT8 aWayNum, SCRP_NEIGHBORINFO* aNeighbor) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SCRP_POINT point = {};

	// プロセス登録
	RPC_SetCalcProcess(e_RC_PROC_NEIGHBOR);

	if (NULL == aNeighbor || NULL == aSetting) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 中心座標取得
	point.parcelId = aSetting->rpPoint[aWayNum].parcelId;
	point.x = aSetting->rpPoint[aWayNum].x;
	point.y = aSetting->rpPoint[aWayNum].y;

	do {
		// 近傍リンク採択
		result = RC_MakeNeighborData(&point, aNeighbor);
		if (e_SC_RESULT_AROUND_NO_LINK == result) {
			// D近傍は未発見を許容する
			result = e_SC_RESULT_SUCCESS;
			break;
		} else if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RC_MakeNeighborData error. [0x%08x] "HERE, result);
			break;
		}

		// リンク長ゼロ近傍をトリムする
		result = trimNeighborLinkList(aNeighbor, e_SIDE_D);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "trimNeighborLinkList error. [0x%08x] "HERE, result);
			break;
		}
		// 初期コスト付加
		result = setDSideCost(aNeighbor);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "setDSideCost error. [0x%08x] "HERE, result);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief 施設入り口目的地近傍処理
 * @param 探索条件
 * @param 地点インデックス
 * @param [O]近傍情報
 * @note 最近傍リンクに対するコスト以外を極大に設定することで入り口リンクへの到達率を上げる処理となる。
 */
E_SC_RESULT RC_NeighborDSidePoiGate(SCRP_SEARCHSETTING* aSetting, UINT8 aWayNum, SCRP_NEIGHBORINFO* aNeighbor) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SCRP_POINT  point = {};

	// プロセス登録
	RPC_SetCalcProcess(e_RC_PROC_NEIGHBOR);

	if (NULL == aNeighbor || NULL == aSetting) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 中心座標取得
	point.parcelId = aSetting->rpPoint[aWayNum].parcelId;
	point.x = aSetting->rpPoint[aWayNum].x;
	point.y = aSetting->rpPoint[aWayNum].y;

	do {
		// 近傍リンク採択
		result = RC_MakeNeighborData(&point, aNeighbor);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RC_MakeNeighborData error. [0x%08x] "HERE, result);
			break;
		}

		// リンク長ゼロ近傍をトリムする
		result = trimNeighborLinkList(aNeighbor, e_SIDE_D);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "trimNeighborLinkList error. [0x%08x] "HERE, result);
			break;
		}
		// 初期コスト付加
		result = setDSideCostPoiGate(aNeighbor);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "setDSideCostPoiGate error. [0x%08x] "HERE, result);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief 自車位置周辺近傍情報作成
 * @param 探索条件
 * @param [O]近傍情報
 */
E_SC_RESULT RC_NeighborCarPoint(SCRP_SEARCHSETTING* aSetting, SCRP_NEIGHBORINFO* aNeighbor) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SCRP_POINT  point = {};
	DOUBLE lat, lon, x, y;

	// プロセス登録
	RPC_SetCalcProcess(e_RC_PROC_NEIGHBOR);

	if (NULL == aNeighbor || NULL == aSetting) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 中心座標取得（自車位置から）
	lat = (DOUBLE) aSetting->car.coord.latitude / 1024;
	lon = (DOUBLE) aSetting->car.coord.longitude / 1024;
	SC_Lib_ChangeTitude2PID(lat, lon, RP_LEVEL1, &point.parcelId, &x, &y);
	point.x = (UINT16) x;
	point.y = (UINT16) y;

	do {
		// 近傍リンク採択
		result = RC_MakeNeighborData(&point, aNeighbor);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RC_MakeNeighborData error. [0x%08x] "HERE, result);
			break;
		}

		// リンク長ゼロ近傍をトリムする
		result = trimNeighborLinkList(aNeighbor, e_SIDE_O);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "trimNeighborLinkList error. [0x%08x] "HERE, result);
			break;
		}
		// 初期コスト付加
		result = setOSideCost(aNeighbor, aSetting, aSetting->car.onRoad);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "setOSideCost error. [0x%08x] "HERE, result);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief	リルート用目的地近傍情報作成処理
 * @param	[I]目的地座標
 * @param	[I]パーセルID
 * @param	[I]リンクID
 * @param	[I]リンク方向
 * @param	[O]近傍情報格納用テーブル
 * @memo	推奨経路の情報から近傍情報の再作成を行う。
 */
E_SC_RESULT RC_ReRouteNbrMake(SCRP_POINT* aBase, UINT32 aPclId, UINT32 aLinkId, UINT8 aOrFlag, SCRP_NEIGHBORINFO* aNeigbor) {

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	MAL_HDL shapeBin;
	MAL_HDL basisBin;
	INT32 xSftW, ySftW;
	RC_CLIPLINKLIST clipLinkList = {};
	UINT32 mapKind = SC_DHC_GetKindMask(e_DATA_KIND_SHAPE) | SC_DHC_GetKindMask(e_DATA_KIND_PARCEL_BASIS);
	SCRP_MAPREADTBL mapReadTbl = {};

	// プロセス登録
	RPC_SetCalcProcess(e_RC_PROC_NEIGHBOR);

	// パラメータチェック
	if (NULL == aBase || NULL == aNeigbor) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	do {
		// 初期化
		RP_Memset0(&clipLinkList, sizeof(RC_CLIPLINKLIST));
		result = mallocClipLinkList(&clipLinkList);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "mallocClipLinkList error. [0x%08x] "HERE, result);
			break;
		}
		clipLinkList.area = RP_MemAlloc(sizeof(RC_CLIPAREA), e_MEM_TYPE_ROUTEPLAN);
		if (NULL == clipLinkList.area) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error."HERE);
			result = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		RP_Memset0(clipLinkList.area, sizeof(RC_CLIPAREA));

		// 基準位置
		clipLinkList.center.parcelId = aBase->parcelId;
		clipLinkList.center.x = aBase->x;
		clipLinkList.center.y = aBase->y;

		// エリア情報
		clipLinkList.areaVol = 1;
		clipLinkList.area[0].parcelId = aPclId;
		clipLinkList.area[0].clipListVol = 1;
		SC_MESH_GetAlterPos(aBase->parcelId, aPclId, RP_LEVEL1, &xSftW, &ySftW);
		clipLinkList.area[0].diffX = xSftW * MAP_SIZE;
		clipLinkList.area[0].diffY = ySftW * MAP_SIZE;
		clipLinkList.area[0].clipListOfs = 0;

		// 地図取得
		result = RC_ReadListMap(&clipLinkList.area[0].parcelId, 1, mapKind, &mapReadTbl);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RC_ReadListMap error. [0x%08x] "HERE, result);
			break;
		}
		if (NULL == mapReadTbl.mapList->shape || NULL == mapReadTbl.mapList->basis) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "shape map not found... "HERE);
			result = e_SC_RESULT_FAIL;
			break;
		}
		basisBin = mapReadTbl.mapList->basis;
		clipLinkList.area[0].shapeBin = mapReadTbl.mapList->shape;

		// リンクIDで検索
		shapeBin = SC_MA_GetMapSharpRecord(clipLinkList.area[0].shapeBin);
		if (NULL == shapeBin) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "SC_MA_GetMapSharpRecord error. "HERE);
			result = e_SC_RESULT_FAIL;
			break;
		}
		result = SC_MA_BinSearchShapeIndex(clipLinkList.area[0].shapeBin, SC_MA_NWID_PNT_ID & aLinkId, clipLinkList.list);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "SC_MA_BinSearchShapeIndex error. [0x%08x] "HERE, result);
			break;
		}
		// 実長データ格納
		clipLinkList.area[0].xRealLenTop = SC_MA_D_BASIS_GET_X_TOP(basisBin);
		clipLinkList.area[0].xRealLenBottom = SC_MA_D_BASIS_GET_X_BOTTOM(basisBin);
		clipLinkList.area[0].yRealLenLeft = SC_MA_D_BASIS_GET_Y_LEFT(basisBin);
		clipLinkList.area[0].yRealLenRight = SC_MA_D_BASIS_GET_Y_RIGHT(basisBin);

		// 近傍リンク収集
		result = makeNeighborLinkList(&clipLinkList, aNeigbor);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "makeNeighborLinkList error. [0x%08x] "HERE, result);
			break;
		}
		if (2 != aNeigbor->nbrLinkVol) {
			result = e_SC_RESULT_FAIL;
			break;
		}
		// 指定方向の情報のみに絞る
		aNeigbor->nbrLinkVol = 1;
		aNeigbor->neighborLink->orFlag = aOrFlag;
		aNeigbor->neighborLink->cost = aNeigbor->neighborLink->leavDist * RP_NBR_LEAVECOST_O;
		// エリア格納
		aNeigbor->point.parcelId = aBase->parcelId;
		aNeigbor->point.x = aBase->x;
		aNeigbor->point.y = aBase->y;
	} while (0);

	// 後処理
	freeClipData(&clipLinkList);
	if (0 == aNeigbor->nbrLinkVol) {
		result = e_SC_RESULT_FAIL;
	}
	// 地図要求領域解放
	if (NULL != mapReadTbl.mapList) {
		RP_MemFree(mapReadTbl.mapList, e_MEM_TYPE_ROUTEPLAN);
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief リンク指定近傍情報生成処理
 * @param パーセルID
 * @param リンクID
 * @param 近傍情報格納用テーブル
 * @memo コスト付与は行わない
 */
E_SC_RESULT RP_MakeNeighborLinkInfo(SCRP_POINT* aPoint, UINT32 aPclId, UINT32 aLinkId, SCRP_NEIGHBORLINK* aNeighborLink) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SCRP_NEIGHBORINFO neighbor = {};
	RC_CLIPLINKLIST clipLinkList = {};
	UINT32 mapKind = SC_DHC_GetKindMask(e_DATA_KIND_SHAPE) | SC_DHC_GetKindMask(e_DATA_KIND_PARCEL_BASIS);
	SCRP_MAPREADTBL mapReadTbl = {};
	MAL_HDL shapeBin = NULL;
	MAL_HDL basisBin = NULL;
	INT32 sftX = 0, sftY = 0;

	// プロセス登録
	RPC_SetCalcProcess(e_RC_PROC_NEIGHBOR);

	if (NULL == aPoint || NULL == aNeighborLink) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	do {
		// 初期化
		RP_Memset0(&clipLinkList, sizeof(RC_CLIPLINKLIST));
		result = mallocClipLinkList(&clipLinkList);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "mallocClipLinkList error. [0x%08x] "HERE, result);
			break;
		}
		clipLinkList.area = RP_MemAlloc(sizeof(RC_CLIPAREA), e_MEM_TYPE_ROUTEPLAN);
		if (NULL == clipLinkList.area) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error."HERE);
			result = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		RP_Memset0(clipLinkList.area, sizeof(RC_CLIPAREA));

		// 基準位置
		clipLinkList.center.parcelId = aPoint->parcelId;
		clipLinkList.center.x = aPoint->x;
		clipLinkList.center.y = aPoint->y;
		// エリア情報
		SC_MESH_GetAlterPos(aPoint->parcelId, aPclId, RP_LEVEL1, &sftX, &sftY);
		clipLinkList.areaVol = 1;
		clipLinkList.area[0].parcelId = aPclId;
		clipLinkList.area[0].clipListVol = 1;
		clipLinkList.area[0].clipListOfs = 0;
		clipLinkList.area[0].diffX = sftX * MAP_SIZE;
		clipLinkList.area[0].diffY = sftY * MAP_SIZE;

		// 地図取得
		result = RC_ReadListMap(&aPclId, 1, mapKind, &mapReadTbl);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RC_ReadListMap error. [0x%08x] "HERE, result);
			break;
		}
		if (NULL == mapReadTbl.mapList->shape || NULL == mapReadTbl.mapList->basis) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RC_ReadListMap error. pcl=0x%08x [0x%08x] "HERE, aPclId, result);
			result = e_SC_RESULT_MAP_GETERR;
			break;
		}
		clipLinkList.area[0].shapeBin = mapReadTbl.mapList->shape;
		basisBin = mapReadTbl.mapList->basis;

		// リンクIDで検索
		shapeBin = SC_MA_GetMapSharpRecord(clipLinkList.area[0].shapeBin);
		if (NULL == shapeBin) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "SC_MA_GetMapSharpRecord error. "HERE);
			result = e_SC_RESULT_FAIL;
			break;
		}
		result = SC_MA_BinSearchShapeIndex(clipLinkList.area[0].shapeBin, SC_MA_NWID_PNT_ID & aLinkId, clipLinkList.list);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "SC_MA_BinSearchShapeIndex error. [0x%08x] "HERE, result);
			break;
		}
		// 実長データ格納
		clipLinkList.area[0].xRealLenTop = SC_MA_D_BASIS_GET_X_TOP(basisBin);
		clipLinkList.area[0].xRealLenBottom = SC_MA_D_BASIS_GET_X_BOTTOM(basisBin);
		clipLinkList.area[0].yRealLenLeft = SC_MA_D_BASIS_GET_Y_LEFT(basisBin);
		clipLinkList.area[0].yRealLenRight = SC_MA_D_BASIS_GET_Y_RIGHT(basisBin);

		// 近傍リンク収集
		result = makeNeighborLinkList(&clipLinkList, &neighbor);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "makeNeighborLinkList. [0x%08x] "HERE, result);
			break;
		}
		if (0 == neighbor.nbrLinkVol) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "neighbor link vol error. "HERE);
			result = e_SC_RESULT_FAIL;
			break;
		}

		// 結果格納
		RP_Memcpy(aNeighborLink, neighbor.neighborLink, sizeof(SCRP_NEIGHBORLINK));
	} while (0);

	// 後処理
	freeClipData(&clipLinkList);

	// メモリ解放
	if (NULL != neighbor.neighborLink) {
		RP_MemFree(neighbor.neighborLink, e_MEM_TYPE_ROUTEPLAN);
	}
	if (NULL != mapReadTbl.mapList) {
		RP_MemFree(mapReadTbl.mapList, e_MEM_TYPE_ROUTEPLAN);
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief 近傍リンクとして採択する対象のリンクか否かを判定する。
 * @memo TODO 海外は別途定数必要
 */
static Bool isNeighborSelectLink(T_MapShapeRecord* record) {
	if (NULL == record) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (false);
	}

	// 道路種別
	if (0 > mSectCnv[record->linkBaseInfo.b_code.roadKind]) {
		return (false);
	}
	// 計画道路
	if (0 != record->linkBaseInfo.b_code.plan) {
		return (false);
	}
	// 階段リンク
	if (0 != record->linkBaseInfo.b_code.stairs) {
		return (false);
	}

	return (true);
}

/**
 * @brief 剥離距離(leaveDist)昇順バブルソート
 *        パーセルID(parcelId)が初期値のデータは無視する
 */
static E_SC_RESULT sortNeighborList(SCRP_NEIGHBORLINK* list, UINT16 size) {
	UINT32 i, u;
	SCRP_NEIGHBORLINK wkNeigbor = {};

	if (NULL == list || 0 == size) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	for (i = 0; i < size; i++) {
		for (u = size - 1; u > i; u--) {
			if (ALL_F32 == list[u].parcelId || 0 == list[u].parcelId) {
				continue;
			}
			if (ALL_F32 == list[u - 1].parcelId || 0 == list[u - 1].parcelId) {
				RP_Memcpy(&wkNeigbor, &list[u], sizeof(SCRP_NEIGHBORLINK));
				RP_Memcpy(&list[u], &list[u - 1], sizeof(SCRP_NEIGHBORLINK));
				RP_Memcpy(&list[u - 1], &wkNeigbor, sizeof(SCRP_NEIGHBORLINK));
				continue;
			}
			if (list[u].leavDist < list[u - 1].leavDist) {
				RP_Memcpy(&wkNeigbor, &list[u], sizeof(SCRP_NEIGHBORLINK));
				RP_Memcpy(&list[u], &list[u - 1], sizeof(SCRP_NEIGHBORLINK));
				RP_Memcpy(&list[u - 1], &wkNeigbor, sizeof(SCRP_NEIGHBORLINK));
				continue;
			}
		}
	}
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 近傍リンク採択処理
 * @param 近傍収集リンクテーブル
 * @param 近傍情報テーブル
 * @memo 近傍リンク情報テーブルへ格納される近傍リンクは最大10本
 */
static E_SC_RESULT makeNeighborLinkList(RC_CLIPLINKLIST* aLinkList, SCRP_NEIGHBORINFO *aNbrInfo) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	// 道路種別区分判定用配列（国内）
	//const UINT16 cAnsLinkSize = 30;
	const UINT16 cSectBlockSize = 3;
	const UINT16 cSectLinkSize = 10;
	const UINT16 cSectMustSize = 2;

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;

	RC_NEIGHBORSELECT *selectList = NULL;				// 近傍採択用ワークテーブル
	RC_VERTICALPARAM verticalParam = {};				// 垂線計算用テーブル
	SCRP_NEIGHBORLINK readLink = {};
	SCRP_NEIGHBORLINK *resultNbr = NULL;
	T_MapShapeRecord* shapeRec = NULL;
	MAL_HDL shapeBin = NULL;
	MAL_HDL point = NULL;
	DOUBLE linkTotalDist = 0;
	DOUBLE centerX = 0;
	DOUBLE centerY = 0;
	DOUBLE preX = 0;
	DOUBLE preY = 0;
	DOUBLE nextX = 0;
	DOUBLE nextY = 0;
	UINT32 i = 0;
	UINT32 u = 0;
	UINT32 m = 0;
	UINT16 wkIdx = 0;

	if (NULL == aLinkList || NULL == aNbrInfo) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 作業領域確保
	selectList = RP_MemAlloc(sizeof(RC_NEIGHBORSELECT) * cSectBlockSize, e_MEM_TYPE_ROUTEPLAN);
	if (NULL == selectList) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error. "HERE);
		return (e_SC_RESULT_MALLOC_ERR);
	}
	RP_Memset0(selectList, sizeof(RC_NEIGHBORSELECT) * cSectBlockSize);

	// ポインタ配列へポインタを配置（距離は最大値で初期化）
	wkIdx = 0;
	for (i = 0; i < cSectBlockSize; i++) {
		selectList[i].size = RP_NBR_LINKSELECTVOL;
		selectList[i].tail = -1;
		for (u = 0; u < cSectLinkSize; u++) {
			selectList[i].list[u].parcelId = ALL_F32;
			wkIdx++;
		}
	}

	// 各パーセル単位で処理を進行
	for (i = 0; i < aLinkList->areaVol; i++) {
		// ownXY
		centerX = aLinkList->center.x;
		centerY = aLinkList->center.y;
		centerX *= (SC_MA_REAL_LEN(aLinkList->area[i].xRealLenBottom));		// 実長
		centerY *= (SC_MA_REAL_LEN(aLinkList->area[i].yRealLenLeft));		// 実長

		// 形状レコード先頭を取得
		shapeBin = SC_MA_GetMapSharpRecord(aLinkList->area[i].shapeBin);
		if (NULL == shapeBin) {
			SC_LOG_DebugPrint(SC_TAG_RC, "SC_RP_GetMapSharpRecord. "HERE);
			continue;
		}

		// パーセル内収集済みリンクへ処理
		for (u = 0; u < aLinkList->area[i].clipListVol; u++) {
			shapeRec = (T_MapShapeRecord*) (shapeBin + *(aLinkList->list + (aLinkList->area[i].clipListOfs + u)));

			// 近傍採択リンク判定
			if (!isNeighborSelectLink(shapeRec)) {
				continue;
			}
			// 剥離距離のみMAXを格納
			readLink.leavDist = DBL_MAX;

			UINT16 loadData = 0;
			// 始点XY座標
			point = (SC_MA_A_SHRCD_GET_XY((MAL_HDL)shapeRec) + 4);
			SC_MA_road2byte(point, loadData);
			preX = (DOUBLE) (loadData + aLinkList->area[i].diffX);		// パーセル跨ぎ時の差分座標考慮
			SC_MA_road2byte(point, loadData);
			preY = (DOUBLE) (loadData + aLinkList->area[i].diffY);		// パーセル跨ぎ時の差分座標考慮

			// 正規化座標を実長へ変換
			preX *= (SC_MA_REAL_LEN(aLinkList->area[i].xRealLenBottom));
			preY *= (SC_MA_REAL_LEN(aLinkList->area[i].yRealLenLeft));

			// 形状数ループ
			for (linkTotalDist = 0, m = 1; m < shapeRec->pointVol; m++) {
				// XY
				nextX = (DOUBLE) read2byte(point);
				nextX += aLinkList->area[i].diffX;		// パーセル跨ぎ時の差分座標考慮
				move2byte(point);
				nextY = (DOUBLE) read2byte(point);
				nextY += aLinkList->area[i].diffY;		// パーセル跨ぎ時の差分座標考慮
				move2byte(point);

				// 正規化座標を実長へ変換
				nextX *= (SC_MA_REAL_LEN(aLinkList->area[i].xRealLenBottom));
				nextY *= (SC_MA_REAL_LEN(aLinkList->area[i].yRealLenLeft));

				verticalParam.cx = centerX;
				verticalParam.cy = centerY;
				verticalParam.ax = preX;
				verticalParam.ay = preY;
				verticalParam.bx = nextX;
				verticalParam.by = nextY;

				// 垂線計算
				if (e_SC_RESULT_SUCCESS != calcVerticalPoint(&verticalParam)) {
					preX = nextX;
					preY = nextY;
					continue;
				}

				if (verticalParam.cvLen < readLink.leavDist) {
					readLink.subIndex = m;
					// 垂線距離
					readLink.leavDist = verticalParam.cvLen;
					// 当該サブリンクまでのトータル距離
					readLink.remainDist = linkTotalDist;
					// 当該サブリンク長
					readLink.subDist = (UINT16) lround(verticalParam.abLen);
					// 当該サブリンク内分距離
					if (0 > verticalParam.avRatio) {
						readLink.subRemainDist = 0;
					} else if (1 < verticalParam.avRatio) {
						readLink.subRemainDist = verticalParam.abLen;
					} else {
						readLink.subRemainDist = verticalParam.abLen * verticalParam.avRatio;
					}
					// 当該サブリンク内分率
					readLink.ratio = verticalParam.avRatio;
					// 当該サブリンク内分率
					readLink.angle = verticalParam.abAngle;
					// リンク方向は最後に補完
					readLink.orFlag = 0;
					// パーセル跨ぎ時の差分座標考慮
					readLink.x = ((verticalParam.vx / (SC_MA_REAL_LEN(aLinkList->area[i].xRealLenBottom)))- aLinkList->area[i].diffX);
					// パーセル跨ぎ時の差分座標考慮
					readLink.y = ((verticalParam.vy / (SC_MA_REAL_LEN(aLinkList->area[i].yRealLenLeft)))- aLinkList->area[i].diffY);
				}

				// リンク長加算
				linkTotalDist += verticalParam.abLen;
				// 直前座標保持
				preX = nextX;
				preY = nextY;
			}

			INT16 sect = mSectCnv[shapeRec->linkBaseInfo.b_code.roadKind];
			if (0 <= selectList[sect].tail) {
				if (selectList[sect].list[selectList[sect].tail].leavDist < readLink.leavDist) {
					continue;
				}
			}

			// リンク情報収集
			readLink.dataIndex = ALL_F16;
			readLink.parcelId = aLinkList->area[i].parcelId;
			readLink.linkId = shapeRec->linkId;
			SC_MA_CALC_LINK_DIST_U(shapeRec->linkBaseInfo.b_data.linkLength, shapeRec->linkBaseInfo.b_data.linkLengthUnit,
					readLink.linkDist);
			readLink.linkKind = shapeRec->linkBaseInfo.b_code.linkKind1;
			readLink.roadKind = shapeRec->linkBaseInfo.b_code.roadKind;
			readLink.subVol = shapeRec->pointVol;
			readLink.cost = ALL_F32;

			// 末尾にデータ追加
			if (selectList[sect].tail < selectList[sect].size - 1) {
				selectList[sect].tail++;
			}
			RP_Memcpy(&selectList[sect].list[selectList[sect].tail], &readLink, sizeof(SCRP_NEIGHBORLINK));
			// 並べ替え
			for (m = selectList[sect].tail; 0 < m; m--) {
				if (selectList[sect].list[m].leavDist < selectList[sect].list[m - 1].leavDist) {
					SCRP_NEIGHBORLINK wkBuf = {};
					RP_Memcpy(&wkBuf, &selectList[sect].list[m - 1], sizeof(SCRP_NEIGHBORLINK));
					RP_Memcpy(&selectList[sect].list[m - 1], &selectList[sect].list[m], sizeof(SCRP_NEIGHBORLINK));
					RP_Memcpy(&selectList[sect].list[m], &wkBuf, sizeof(SCRP_NEIGHBORLINK));
				}
			}
		}
	}

	/* 道路種別区分別に採択した近傍リンク情報を10本に絞り込む */
	do {
		RC_NEIGHBORSELECT resultList = {};
		SCRP_NEIGHBORLINK surplusList[30] = {};
		UINT16 surplusVol = 0;
		UINT16 setCnt = 0;
		UINT16 hitNbr = 0;
		UINT16 setResultCnt = 0;
		UINT16 tableSize = 0;

		// 近傍リンク数カウント
		for (i = 0; i < cSectBlockSize; i++) {
			if (-1 == selectList[i].tail) {
				continue;
			}
			hitNbr += (selectList[i].tail + 1);
		}

		if (0 == hitNbr) {
			SC_LOG_ErrorPrint(SC_TAG_RC, " hit nbr 0. "HERE);
			result = e_SC_RESULT_FAIL;
			break;
		}
		// 領域確保(上下線を登録するので倍数)
		if (RP_NBR_LINKSELECTVOL < hitNbr) {
			tableSize = cSectLinkSize * 2;
		} else {
			tableSize = hitNbr * 2;
		}
		resultNbr = RP_MemAlloc(sizeof(SCRP_NEIGHBORLINK) * tableSize, e_MEM_TYPE_ROUTEPLAN);
		if (NULL == resultNbr) {
			result = e_SC_RESULT_FAIL;
			break;
		}
		RP_Memset0(resultNbr, sizeof(SCRP_NEIGHBORLINK) * tableSize);
		// 区分保障を格納
		for (i = 0; i < cSectBlockSize; i++) {
			for (u = 0; u < selectList[i].size; u++) {
				if (selectList[i].tail < u) {
					break;
				}
				if (cSectMustSize > u) {
					RP_Memcpy(&resultList.list[resultList.tail], &selectList[i].list[u], sizeof(SCRP_NEIGHBORLINK));
					resultList.tail++;
				} else {
					RP_Memcpy(&surplusList[surplusVol], &selectList[i].list[u], sizeof(SCRP_NEIGHBORLINK));
					surplusVol++;
				}
			}
		}
		// 保証分バブルソート
		if (0 < resultList.tail) {
			if (e_SC_RESULT_SUCCESS != sortNeighborList(&resultList.list[0], resultList.tail)) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "sortNeighborList error. "HERE);
				result = e_SC_RESULT_FAIL;
				break;
			}
		}
		// 非保証分バブルソート
		if (0 < surplusVol) {
			if (e_SC_RESULT_SUCCESS != sortNeighborList(&surplusList[0], surplusVol)) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "sortNeighborList error. "HERE);
				result = e_SC_RESULT_FAIL;
				break;
			}
		}
#if 0 // ★ログ出力
		{
			UINT16 t = 0;
			for (t = 0; t < resultList.tail; t++) {
				SC_LOG_InfoPrint(SC_TAG_RC, " resultList[%d] pcl=0x%08x link=0x%08x", t, resultList.list[t].parcelId,
						resultList.list[t].linkId);
			}
			for (t = 0; t < surplusVol; t++) {
				SC_LOG_InfoPrint(SC_TAG_RC, " surplus[%d] pcl=0x%08x link=0x%08x", t, surplusList[t].parcelId, surplusList[t].linkId);
			}
		}
#endif
		// 結果格納（保証分）
		for (i = 0; i < resultList.tail; i++) {
			if (cSectLinkSize <= setCnt) {
				break;
			}
			if (ALL_F32 == resultList.list[i].parcelId) {
				break;
			}
			RP_Memcpy(&resultNbr[setResultCnt], &resultList.list[i], sizeof(SCRP_NEIGHBORLINK));
			resultNbr[setResultCnt++].orFlag = 0;
			RP_Memcpy(&resultNbr[setResultCnt], &resultList.list[i], sizeof(SCRP_NEIGHBORLINK));
			resultNbr[setResultCnt++].orFlag = 1;
			setCnt++;
		}
		// 結果格納（非保証分）
		for (i = 0; i < surplusVol; i++) {
			if (cSectLinkSize <= setCnt) {
				break;
			}
			if (ALL_F32 == surplusList[i].parcelId) {
				break;
			}
			RP_Memcpy(&resultNbr[setResultCnt], &surplusList[i], sizeof(SCRP_NEIGHBORLINK));
			resultNbr[setResultCnt++].orFlag = 0;
			RP_Memcpy(&resultNbr[setResultCnt], &surplusList[i], sizeof(SCRP_NEIGHBORLINK));
			resultNbr[setResultCnt++].orFlag = 1;
			setCnt++;
		}
		aNbrInfo->neighborLink = resultNbr;
		aNbrInfo->nbrLinkVol = setResultCnt;
	} while (0);

	if (e_SC_RESULT_SUCCESS != result && NULL != resultNbr) {
		RP_MemFree(resultNbr, e_MEM_TYPE_ROUTEPLAN);
	}
	if (NULL != selectList) {
		RP_MemFree(selectList, e_MEM_TYPE_ROUTEPLAN);
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief 線分ABに対し点Cから垂線を引いた場合の各値の算出
 *        垂線到達点Vはベクトル計算
 * @param [IO]垂線計算用テーブル
 */
static E_SC_RESULT calcVerticalPoint(RC_VERTICALPARAM *vp) {

	DOUBLE ax, ay, bx, by, ratio;

	if (NULL == vp) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	// 線分ABが点である場合不正パラメタ扱いとする
	if ((CompareDouble(vp->ax, vp->bx)) && (CompareDouble(vp->ay, vp->by))) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 線分ABへの点Cからの垂線座標ベクトル計算
	ax = vp->bx - vp->ax;
	ay = vp->by - vp->ay;
	bx = vp->cx - vp->ax;
	by = vp->cy - vp->ay;
	ratio = (ax * bx + ay * by) / (ax * ax + ay * ay);

	if (ratio <= 0) {
		// -外分点
		vp->vx = vp->ax;
		vp->vy = vp->ay;
	} else if (1 <= ratio) {
		// +外分点
		vp->vx = vp->bx;
		vp->vy = vp->by;
	} else {
		// 内分点
		vp->vx = vp->ax + ratio * ax;
		vp->vy = vp->ay + ratio * ay;
	}
	// 比
	vp->avRatio = ratio;
	// 垂線長
	vp->cvLen = pow((vp->vx - vp->cx) * (vp->vx - vp->cx) + (vp->vy - vp->cy) * (vp->vy - vp->cy), 0.5);
	// 線分AB長
	vp->abLen = pow((vp->bx - vp->ax) * (vp->bx - vp->ax) + (vp->by - vp->ay) * (vp->by - vp->ay), 0.5);
	// 線分ABの角度
	vp->abAngle = atan2(vp->by - vp->ay, vp->bx - vp->ax);
	if (vp->abAngle < 0.0) {
		vp->abAngle += deg2rad(360.0);
	}
	vp->abAngle = rad2deg(vp->abAngle);

#if 0 // Log出力
	SC_LOG_InfoPrint(SC_TAG_RC, "A(%f,%f)B(%f,%f)C(%f,%f) Z(%f,%f) vLen=%f aLen=%f ratio=%f angle=%f"
			, vp->ax, vp->ay, vp->bx, vp->by, vp->cx, vp->cy, vp->vx, vp->vy, vp->cvLen, vp->abLen, vp->avRatio, vp->abAngle);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief クリップエリアに触れるリンクの形状情報アドレスリストを作成する
 * @param 中心
 * @param クリップエリア範囲指定 単位：m
 * @param [O]クリップ情報格納用
 */
static E_SC_RESULT makeClipLinkList(SCRP_POINT* aCenter, UINT16 aSize, RC_CLIPLINKLIST** aResultList) {

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	T_MapShapeDir* shapeDir = NULL;
	T_MapShapeRecord* shapeRec = NULL;
	UINT32* indexBin = NULL;
	MAL_HDL pclBin = NULL;
	MAL_HDL shapeBin = NULL;
	MAL_HDL point = NULL;
	UINT16 loadData = 0;
	UINT32 sublp = 0;
	UINT32 e = 0;
	UINT32 i = 0;
	INT32 xSft = 0;
	INT32 ySft = 0;
	DOUBLE preX = 0;
	DOUBLE preY = 0;
	DOUBLE nextX = 0;
	DOUBLE nextY = 0;

	SCRP_POINT lb = {};
	SCRP_POINT rt = {};
	SCRP_PCLRECT area = {};
	RC_CLIPLINKLIST *clipList = NULL;

	if (NULL == aCenter || NULL == aResultList || 0 == aSize) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 対象パーセルの実長を取得
	UINT32 sizeX = 0;
	UINT32 sizeY = 0;
	UINT32 realBottom = 0;
	UINT32 realLeft = 0;

	result = getRealLenTryLv2(aCenter->parcelId, &realBottom, &realLeft);
	if (e_SC_RESULT_SUCCESS != result) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "getRealLenTryLv2 error. [0x%08x] "HERE, result);
		return (result);
	}
	sizeX = aSize * 10000 / realBottom;
	sizeY = aSize * 10000 / realLeft;

	do {
		// 領域確保
		clipList = RP_MemAlloc(sizeof(RC_CLIPLINKLIST), e_MEM_TYPE_ROUTEPLAN);
		if (NULL == clipList) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error. "HERE);
			result = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// 初期化
		RP_Memset0(clipList, sizeof(RC_CLIPLINKLIST));

		// リスト領域確保
		mallocClipLinkList(clipList);

		// エリア情報
		clipList->center.parcelId = aCenter->parcelId;
		clipList->center.x = aCenter->x;
		clipList->center.y = aCenter->y;

		// クリップ範囲右上左下算出
		result = makeNeighborClipArea(&clipList->center, sizeX, sizeY, &rt, &lb);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "makeNeighborClipArea error. [0x%08x] "HERE, result);
			break;
		}

		// シフト量取得
		SC_MESH_GetAlterPos(lb.parcelId, rt.parcelId, RP_LEVEL1, &xSft, &ySft);
		if ((0 > xSft) || (0 > ySft)) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "neigbor area is undar flow... "HERE);
			result = e_SC_RESULT_FAIL;
			break;
		}
		area.parcelId = lb.parcelId;
		area.xSize = (xSft + 1);
		area.ySize = (ySft + 1);

		//クリップエリアの領域確保
		clipList->area = RP_MemAlloc(sizeof(RC_CLIPAREA) * (area.xSize * area.ySize), e_MEM_TYPE_ROUTEPLAN);
		clipList->areaVol = 0;
		if (NULL == clipList->area) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error. "HERE);
			result = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		RP_Memset0(clipList->area, sizeof(RC_CLIPAREA) * (area.xSize * area.ySize));

		// クリップエリア情報を詰める + 地図読み込み
		result = setClipAreaInfo(clipList, &area, &rt, &lb);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "setClipAreaInfo error. [0x%08x] "HERE, result);
			break;
		}

		for (i = 0; i < clipList->areaVol; i++) {

			// データオフセット格納
			clipList->area[i].clipListOfs = clipList->listCurrent;

			pclBin = clipList->area[i].shapeBin;

			// 形状レコード先頭を取得
			shapeBin = SC_MA_GetMapSharpRecord(pclBin);
			if (NULL == shapeBin) {
				// 形状情報なし
				SC_LOG_DebugPrint(SC_TAG_RC, "SC_RP_GetMapSharpRecord not found shape record. "HERE);
				continue;
			}

			// 形状索引先頭を取得
			T_MapShapeIndexRecord* indexInfo;
			indexBin = (UINT32*) SC_MA_A_SHBIN_GET_INDEX_LINK(pclBin);
			indexInfo = (T_MapShapeIndexRecord*) indexBin;
			indexBin += 2;
			shapeDir = (T_MapShapeDir*) SC_MA_A_SHBIN_GET_DIR(pclBin);

			UINT16 xRtSub;		// リンク左下X
			UINT16 yRtSub;		// リンク左下Y
			UINT16 xLbSub;		// リンク右上X
			UINT16 yLbSub;		// リンク右上Y

			for (e = 0; e < indexInfo->linkVol; e++) {
				//shapeRec = (T_MapShapeRecord*) (shapeBin + *(indexBin + reclp));
				shapeRec = (T_MapShapeRecord*) (shapeBin + (*(indexBin + e) * 4));
				point = (SC_MA_A_SHRCD_GET_XY((MAL_HDL)shapeRec) + 4);
				// 始点XY
				SC_MA_road2byte(point, loadData);
				preX = (DOUBLE) loadData;
				SC_MA_road2byte(point, loadData);
				preY = (DOUBLE) loadData;

				xRtSub = preX;
				xLbSub = preX;
				yRtSub = preY;
				yLbSub = preY;

				// 形状数ループ
				for (sublp = 1; sublp < shapeRec->pointVol; sublp++) {
					// XY
					nextX = (DOUBLE) read2byte(point);
					move2byte(point);
					nextY = (DOUBLE) read2byte(point);
					move2byte(point);

					// 右上左下MAXの更新
					if (xRtSub < nextX) {
						xRtSub = nextX;
					} else if (xLbSub > nextX) {
						xLbSub = nextX;
					}
					if (yRtSub < nextY) {
						yRtSub = nextY;
					} else if (yLbSub > nextY) {
						yLbSub = nextY;
					}

					preX = nextX;
					preY = nextY;
				}

				// リンクエリアとクリップエリアのかぶりを確認
				// 右上
				if ((xLbSub <= clipList->area[i].rtX) && (yLbSub <= clipList->area[i].rtY)) {
					// 左下
					if ((clipList->area[i].lbX <= xRtSub) && (clipList->area[i].lbY <= yRtSub)) {
						// サイズチェック
						if (clipList->listSize <= clipList->listCurrent) {
							result = mallocClipLinkList(clipList);
							if (e_SC_RESULT_SUCCESS != result) {
								SC_LOG_ErrorPrint(SC_TAG_RC, "mallocClipLinkList error. [0x%08x] "HERE, result);
								break;
							}
						}
						// 形状データオフセット格納
						*(clipList->list + clipList->listCurrent) = (*(indexBin + e) * 4);

						clipList->area[i].clipListVol++;
						clipList->listCurrent++;
					}
				}
			}
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "error. "HERE);
				break;
			}
		}
	} while (0);

	// 結果確認
	if (e_SC_RESULT_SUCCESS == result) {
		*aResultList = clipList;
	} else {
		freeClipData(clipList);
		*aResultList = NULL;
	}

	return (result);
#if 0	// いずれか接触の可能性あり以下詳細判定（回避エリア等では場合）
						// 始点XY
						road2byte(point, loadData);
						preX = (DOUBLE) loadData;
						road2byte(point, loadData);
						preY = (DOUBLE) loadData;

						// 形状数ループ
						for (sublp = 1; sublp < shapeRec->pointVol; sublp++) {

							xRtSub = preX;
							yRtSub = preY;
							xLbSub = preX;
							yLbSub = preY;

							// XY
							nextX = (DOUBLE) read2byte(point);
							move2byte(point);
							nextY = (DOUBLE) read2byte(point);
							move2byte(point);

							// 右上左下MAXの更新
							if (xRtSub < nextX) {
								xRtSub = nextX;
							} else if (xLbSub > nextX) {
								xLbSub = nextX;
							}
							if (yRtSub < nextY) {
								yRtSub = nextY;
							} else if (yLbSub > nextY) {
								yLbSub = nextY;
							}
							if ((xLbSub <= xRt) && (yLbSub <= yRt)) {
								if ((xLb <= xRtSub) && (yLb <= yRtSub)) {
									// 接触
									// 形状データオフセット格納
									if((aList->listVol - 1) <= aList->recordvol){
										// TO DO エリア再確保
									}
									*(aList->linkOfs + aList->recordvol) = *(indexBin + reclp);
									aList->recordvol += 1;
									break;
								}
							}

							preX = nextX;
							preY = nextY;
						}
#endif
}

/**
 * @brief リンクリスト領域確保
 * @param クリップリンクリスト
 */
static E_SC_RESULT mallocClipLinkList(RC_CLIPLINKLIST* aList) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	UINT32* bkList = NULL;
	UINT32 bkSize = 0;

	if (NULL == aList) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	if ((0 < aList->listSize) && (NULL != aList->list)) {
		bkList = aList->list;
		bkSize = aList->listSize;
	}
	aList->listSize += RP_NBR_BLOCKSIZE;
	aList->list = RP_MemAlloc(sizeof(UINT32) * aList->listSize, e_MEM_TYPE_ROUTEPLAN);
	if (NULL == aList->list) {
		// 以降続行不可。領域の解放はControlにて
		aList->listSize = bkSize;
		aList->list = bkList;
		SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error. "HERE);
		return (e_SC_RESULT_MALLOC_ERR);
	}
	RP_Memset0(aList->list, sizeof(UINT32) * aList->listSize);
	if (bkList) {
		RP_Memcpy(aList->list, bkList, sizeof(UINT32) * bkSize);
		RP_MemFree(bkList, e_MEM_TYPE_ROUTEPLAN);
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 近傍情報へのコスト付加処理
 * @param aNbr [I/O]近傍情報
 * @param aSetting [I]探索条件
 * @param aOnRoad [I]リンクマッチコスト付加
 * @memo 近傍情報に自車位置からの剥離距離及び角度に応じた初期コストを与える。
 *       aOnRoadがtrueの場合リンクマッチング用のコストを付加する。
 *       但し採択リンクにマッチングリンクが含まれていない場合意味を成さない。
 */
static E_SC_RESULT setOSideCost(SCRP_NEIGHBORINFO* aNbr, SCRP_SEARCHSETTING* aSetting, Bool aOnRoad) {

	// 方位補正 剥離距離
	static const INT32 ANGLE_CORRECTION_DIST = 70;
	static const INT32 ANGLE_CORRECTION_S = 20;
	static const INT32 ANGLE_CORRECTION_M = 35;
	static const INT32 ANGLE_CORRECTION_L = 50;
	// リンクコスト減算,加算割合
	static const INT32 NBR_ORDER_COST[] = { 50, 70, 80, 100 };
	static const INT32 NBR_REVERSE_COST[] = { 100, 50, 30, 0 };

	SCRP_NEIGHBORLINK* nLink = NULL;
	UINT32 penaltyCost = 0;
	UINT32 i;
	UINT32 cost = 0;
	DOUBLE mostNearLeave = 0;
	DOUBLE leaveDist = 0;
	INT32 diffAgl = 0;
	Bool oFollow = false;
	Bool rFollow = false;

	if (NULL == aSetting || NULL == aNbr || NULL == aNbr->neighborLink) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	nLink = aNbr->neighborLink;
	mostNearLeave = nLink->leavDist;

	// 初期コストの設定
	if (aOnRoad) {
		for (i = 0; i < aNbr->nbrLinkVol; i++) {
			if ((aSetting->car.parcelId == nLink[i].parcelId) && (aSetting->car.linkId == nLink[i].linkId)) {
				penaltyCost = RP_NBR_MATCHLINKCOST;
				break;
			}
		}
	}

	for (i = 0; i < aNbr->nbrLinkVol; i++) {
		// ini
		oFollow = false;
		rFollow = false;

		if (0 == nLink[i].orFlag) {
			diffAgl = aSetting->car.dir - (INT32) nLink[i].angle;
		} else {
			// 逆方向の場合読み替え
			diffAgl = aSetting->car.dir - (INT32) (nLink[i].angle < 180.0f ? nLink[i].angle + 180.0f : nLink[i].angle - 180.0f);
		}
		diffAgl = ((diffAgl < 0) ? (diffAgl * -1) : diffAgl);

		// 15km over or onRoad
		if (15.0 < aSetting->car.speed || aSetting->car.onRoad) {
			// ±20 odaer
			if ((0 <= diffAgl && diffAgl < 20) || (340 < diffAgl && diffAgl <= 360)) {
				oFollow = true;
			}
			// ±20 reverse
			else if (160 < diffAgl && diffAgl < 200) {
				rFollow = true;
			}
		}

		// マッチングリンクを優遇
		if ((true == aSetting->car.onRoad) && (aSetting->car.parcelId == nLink[i].parcelId)
				&& (aSetting->car.linkId == nLink[i].linkId)) {
			if (oFollow) {
				nLink[i].cost = 0;
				nLink[i].followFlag = 1;
				continue;
			}
		}

		// 離れている場合方位補正なし
		if (ANGLE_CORRECTION_DIST < nLink[i].leavDist) {
			oFollow = false;
			rFollow = false;
		}
		leaveDist = nLink[i].leavDist - mostNearLeave;

		// 方向に対して初期コスト補正を行う
		if (!oFollow && !rFollow) {
			cost = leaveDist * RP_NBR_LEAVECOST_O;
			cost += penaltyCost;
		}
		// 軽くする(補正コストは距離別に判定を行う)
		else if (oFollow) {
			nLink[i].followFlag = 1;
			cost = leaveDist * RP_NBR_LEAVECOST_O;
			cost += penaltyCost;

			if (ANGLE_CORRECTION_S > nLink[i].leavDist) {
				cost = (cost * NBR_ORDER_COST[0]) / 100;
			} else if (ANGLE_CORRECTION_M > nLink[i].leavDist) {
				cost = (cost * NBR_ORDER_COST[1]) / 100;
			} else if (ANGLE_CORRECTION_L > nLink[i].leavDist) {
				cost = (cost * NBR_ORDER_COST[2]) / 100;
			} else {
				cost = (cost * NBR_ORDER_COST[3]) / 100;
			}
		}
		// 重くする(補正コストは距離別に判定を行う)
		else {
			cost = leaveDist * RP_NBR_LEAVECOST_O;
			cost += penaltyCost;

			if (ANGLE_CORRECTION_S > nLink[i].leavDist) {
				cost += (RP_NBR_REVERSECOST * NBR_REVERSE_COST[0]) / 100;
			} else if (ANGLE_CORRECTION_M > nLink[i].leavDist) {
				cost += (RP_NBR_REVERSECOST * NBR_REVERSE_COST[1]) / 100;
			} else if (ANGLE_CORRECTION_L > nLink[i].leavDist) {
				cost += (RP_NBR_REVERSECOST * NBR_REVERSE_COST[2]) / 100;
			} else {
				cost += (RP_NBR_REVERSECOST * NBR_REVERSE_COST[3]) / 100;
			}
		}

		nLink[i].cost = cost;
	}
	return (e_SC_RESULT_SUCCESS);
}

/**
 * D側近傍に対する初期コストを設定する
 * 距離に準じた初期コストの付加を行う。
 * @param aNeighbor [I/O]近傍情報
 */
static E_SC_RESULT setDSideCost(SCRP_NEIGHBORINFO* aNeighbor) {

	SCRP_NEIGHBORLINK *nLink = NULL;
	//DOUBLE minCost = 0;
	UINT32 i;

	if (NULL == aNeighbor || NULL == aNeighbor->neighborLink) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	nLink = aNeighbor->neighborLink;
	for (i = 0; i < aNeighbor->nbrLinkVol; i++) {
		nLink[i].cost = nLink[i].leavDist * RP_NBR_LEAVECOST_D;
	}

	return (e_SC_RESULT_SUCCESS);
}

/**
 * 施設入り口に対する初期コストを設定する
 * 最近傍に対する初期コストを0とし、それ以外に大コストを付加する。
 * @param aNeighbor [I/O]近傍情報
 */
static E_SC_RESULT setDSideCostPoiGate(SCRP_NEIGHBORINFO* aNeighbor) {
	SCRP_NEIGHBORLINK *nLink = NULL;
	DOUBLE minLeave;
	UINT32 i;

	if (NULL == aNeighbor || NULL == aNeighbor->neighborLink) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	nLink = aNeighbor->neighborLink;
	minLeave = DBL_MAX;

	// すべてのリンクにゲートコストを付与
	for (i = 0; i < aNeighbor->nbrLinkVol; i++) {
		nLink[i].cost = RP_NBR_POIGATE;
		if (minLeave > nLink[i].leavDist) {
			minLeave = nLink[i].leavDist;
		}
	}
	// 最少剥離リンクのコストを0に
	for (i = 0; i < aNeighbor->nbrLinkVol; i++) {
		if (fabs(minLeave - nLink[i].leavDist) < DBL_EPSILON) {
			nLink[i].cost = 0;
		}
	}
	return (e_SC_RESULT_SUCCESS);
}

/**
 * 近傍リンクリスト内のリンク長ゼロリンクをトリムする<br>
 * 元のメモリ確保領域に対して上書きを行い、nbrLinkVolの書き換えを行う。領域の縮小は行わない。
 * @param aNeighbor [I/O]近傍情報
 */
static E_SC_RESULT trimNeighborLinkList(SCRP_NEIGHBORINFO* aNeighbor, E_SIDEOD aPointType) {

	SCRP_NEIGHBORLINK *buf = NULL;
	SCRP_NEIGHBORLINK *src = NULL;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	UINT16 setIdx = 0;
	UINT16 i = 0;

	if (NULL == aNeighbor || NULL == aNeighbor->neighborLink|| 0 == aNeighbor->nbrLinkVol) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	do {
		src = aNeighbor->neighborLink;
		buf = RP_MemAlloc(sizeof(SCRP_NEIGHBORLINK) * aNeighbor->nbrLinkVol, e_MEM_TYPE_ROUTEPLAN);
		if (NULL == buf) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error. "HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		// 作業領域への情報コピー及び元領域のクリアを行う。
		RP_Memcpy(buf, src, sizeof(SCRP_NEIGHBORLINK) * aNeighbor->nbrLinkVol);
		RP_Memset0(src, sizeof(SCRP_NEIGHBORLINK) * aNeighbor->nbrLinkVol);

		for (i = 0; i < aNeighbor->nbrLinkVol; i++) {

			/*
			 * リンク長０での不採用パターン
			 *   出発地側
			 *     順方向 + リンク長<=塗りつぶしMAX
			 *     逆方向 + 塗りつぶし=0
			 *   目的地側
			 *     逆方向 + リンク長<=塗りつぶしMAX
			 *     順方向 + 塗りつぶし=0
			 */
			if ((e_SIDE_O == aPointType && 0 == buf[i].orFlag) || (e_SIDE_D == aPointType && 0 != buf[i].orFlag)) {
				if (buf[i].linkDist <= (buf[i].remainDist + buf[i].subRemainDist)) {
					continue;
				}
			} else {
				//
				if (buf[i].remainDist == 0 && buf[i].subRemainDist == 0) {
					continue;
				}
			}
			RP_Memcpy(&src[setIdx++], &buf[i], sizeof(SCRP_NEIGHBORLINK));
		}
		aNeighbor->nbrLinkVol = setIdx;
	} while (0);

	if (NULL != buf) {
		RP_MemFree(buf, e_MEM_TYPE_ROUTEPLAN);
	}

	return (ret);
}

/**
 * 指定正規座標範囲左下及び右上取得処理
 * @param aBase 基準位置
 * @param aRadiusX 指定範囲
 * @param aRadiusY 指定範囲
 * @param [O]aRtArea 右上
 * @param [O]aLbArea 左上
 */
static E_SC_RESULT makeNeighborClipArea(SCRP_POINT* aBase, UINT16 aRadiusX, UINT16 aRadiusY, SCRP_POINT* aRtArea, SCRP_POINT* aLbArea) {

	if (NULL == aBase || NULL == aRtArea || NULL == aLbArea || 0 == aRadiusX || 0 == aRadiusY) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	INT16 sftX, sftY;
	INT16 x, y;

	// 左下特定
	x = aBase->x - aRadiusX;
	y = aBase->y - aRadiusY;
	sftX = 0;
	sftY = 0;

	if (0 > x) {
		sftX = -1 * (abs(x / MAP_SIZE) + 1);
		x = (x % MAP_SIZE) + MAP_SIZE;
	}
	if (0 > y) {
		sftY = -1 * (abs(y / MAP_SIZE) + 1);
		y = (y % MAP_SIZE) + MAP_SIZE;
	}

	aLbArea->parcelId = SC_MESH_SftParcelId(aBase->parcelId, sftX, sftY);
	if (ALL_F32 == aLbArea->parcelId) {
		return (e_SC_RESULT_FAIL);
	}
	aLbArea->x = x;
	aLbArea->y = y;

	// 右上特定
	x = aBase->x + aRadiusX;
	y = aBase->y + aRadiusY;
	sftX = 0;
	sftY = 0;

	if (MAP_SIZE < x) {
		sftX = x / MAP_SIZE;
		x = x % MAP_SIZE;
	}
	if (MAP_SIZE < y) {
		sftY = y / MAP_SIZE;
		y = y % MAP_SIZE;
	}
	aRtArea->parcelId = SC_MESH_SftParcelId(aBase->parcelId, sftX, sftY);
	if (ALL_F32 == aLbArea->parcelId) {
		return (e_SC_RESULT_FAIL);
	}
	aRtArea->x = x;
	aRtArea->y = y;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * クリップエリア情報設定＋地図データ読み込み
 * TODO 密度データから地図データの有無を判断
 * @param aClip [O] クリップエリア情報
 * @param aArea [I] クリップエリアベース 左下パーセル＋X*Y 情報
 * @param aRtArea [I] 右上クリップエリア
 * @param aLbArea [I] 左下クリップエリア
 */
static E_SC_RESULT setClipAreaInfo(RC_CLIPLINKLIST* aClip, SCRP_PCLRECT *aArea, SCRP_POINT* aRtArea, SCRP_POINT* aLbArea) {

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SCRP_MAPREADTBL readTbl = {};
	UINT32 mapKind = SC_DHC_GetKindMask(e_DATA_KIND_SHAPE) | SC_DHC_GetKindMask(e_DATA_KIND_PARCEL_BASIS);

	UINT16 setIdx = 0;
	UINT32 pclId = 0;
	UINT32 x, y;
	UINT16 lbX, lbY;
	UINT16 rtX, rtY;
	INT32 sftX, sftY;

	if (NULL == aClip || NULL == aArea || NULL == aRtArea || NULL == aLbArea) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	for (y = 0; y < aArea->ySize; y++) {
		for (x = 0; x < aArea->xSize; x++) {

			// パーセルID算出
			pclId = SC_MESH_SftParcelId(aLbArea->parcelId, x, y);
			if (ALL_F32 == pclId) {
				continue;
			}

			// 左下とのシフト量からエリア内左下座標取得
			if (-1 == SC_MESH_GetAlterPos(aLbArea->parcelId, pclId, RP_LEVEL1, &sftX, &sftY)) {
				continue;
			}
			(0 == sftX) ? (lbX = aLbArea->x) : (lbX = 0);
			(0 == sftY) ? (lbY = aLbArea->y) : (lbY = 0);

			// 右上とのシフト量からエリア内右上座標取得
			if (-1 == SC_MESH_GetAlterPos(aRtArea->parcelId, pclId, RP_LEVEL1, &sftX, &sftY)) {
				continue;
			}
			(0 == sftX) ? (rtX = aRtArea->x) : (rtX = MAP_SIZE);
			(0 == sftY) ? (rtY = aRtArea->y) : (rtY = MAP_SIZE);

			// 地図データ取得
			result = RC_ReadListMap(&pclId, 1, mapKind, &readTbl);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "RC_ReadListMap error. [0x%08x] "HERE, result);
				break;
			}

			// 道路なしパーセルor未ダウンロードor内部エラー
			if (NULL == readTbl.mapList->shape || NULL == readTbl.mapList->basis) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "shape and basis not found... "HERE);
				// メモリ解放
				RP_MemFree(readTbl.mapList, e_MEM_TYPE_ROUTEPLAN);
				continue;
			}

			// パーセル情報格納
			aClip->area[setIdx].parcelId = pclId;
			aClip->area[setIdx].shapeBin = readTbl.mapList->shape;
			aClip->area[setIdx].clipListVol = 0;
			aClip->area[setIdx].clipListOfs = 0;
			aClip->area[setIdx].xRealLenTop = SC_MA_D_BASIS_GET_X_TOP(readTbl.mapList->basis);
			aClip->area[setIdx].xRealLenBottom = SC_MA_D_BASIS_GET_X_BOTTOM(readTbl.mapList->basis);
			aClip->area[setIdx].yRealLenLeft = SC_MA_D_BASIS_GET_Y_LEFT(readTbl.mapList->basis);
			aClip->area[setIdx].yRealLenRight = SC_MA_D_BASIS_GET_Y_RIGHT(readTbl.mapList->basis);

			// 基準パーセルとの差分値取得
			SC_MESH_GetAlterPos(aClip->center.parcelId, pclId, RP_LEVEL1, &sftX, &sftY);
			aClip->area[setIdx].diffX = sftX * MAP_SIZE;
			aClip->area[setIdx].diffY = sftY * MAP_SIZE;

			aClip->area[setIdx].rtX = rtX;
			aClip->area[setIdx].rtY = rtY;
			aClip->area[setIdx].lbX = lbX;
			aClip->area[setIdx].lbY = lbY;

			setIdx++;
			aClip->areaVol++;

			// メモリ解放
			RP_MemFree(readTbl.mapList, e_MEM_TYPE_ROUTEPLAN);
		}
		if (e_SC_RESULT_SUCCESS != result) {
			break;
		}
	}
	return (result);
}

/**
 * @brief 対象パーセルの実長取得（LV2まで取得を試みる）
 * @param パーセルID
 * @param X実長
 * @param Y実長
 * @memo Lv2までパーセル基本情報取得を試します。
 */
static E_SC_RESULT getRealLenTryLv2(UINT32 aParcelId, UINT32 *aBottom, UINT32 *aLeft) {

	if (NULL == aBottom || NULL == aLeft) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	UINT32 orgLevel = SC_MESH_GetLevel(aParcelId);
	UINT32 nextLvPcl = aParcelId;
	UINT32 maxLv = RP_LEVEL2;
	UINT32 bottom = 0;
	UINT32 left = 0;
	UINT32 i;

	for (i = (orgLevel - 1); i < maxLv; i++) {
		// 実長取得
		result = getRealLen(nextLvPcl, &bottom, &left);
		if (e_SC_RESULT_SUCCESS == result) {
			switch (SC_MESH_GetLevel(nextLvPcl)) {
			case RP_LEVEL1:
				*aBottom = bottom;
				*aLeft = left;
				break;
			case RP_LEVEL2:
				*aBottom = bottom / 4;
				*aLeft = left / 4;
				break;
			default:
				SC_LOG_ErrorPrint(SC_TAG_RC, "unknown level parcel... "HERE);
				result = e_SC_RESULT_FAIL;
				break;
			}
			break;
		}
		nextLvPcl = SC_MESH_GetUpperParcelID(nextLvPcl);
	}
	return (result);
}

/**
 * @brief 対象パーセルの実長取得
 */
static E_SC_RESULT getRealLen(UINT32 aParcelId, UINT32 *bottom, UINT32 *left) {

	if (NULL == bottom || NULL == left) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SCRP_MAPREADTBL readTbl = {};
	UINT32 mapKind = SC_DHC_GetKindMask(e_DATA_KIND_PARCEL_BASIS);
	*bottom = 0;
	*left = 0;

	do {
		result = RC_ReadListMap(&aParcelId, 1, mapKind, &readTbl);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RC_ReadListMap error. [0x%08x] "HERE, result);
			break;
		}
		if (NULL == readTbl.mapList->basis) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "basis is null... "HERE);
			result = e_SC_RESULT_MAP_GETERR;
			break;
		}

		// 実長取得
		*bottom = SC_MA_D_BASIS_GET_X_BOTTOM(readTbl.mapList->basis);
		*left = SC_MA_D_BASIS_GET_Y_LEFT(readTbl.mapList->basis);
	} while (0);

	// 地図解放
	if (e_SC_RESULT_SUCCESS != RC_FreeMapTbl(&readTbl, true)) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "RC_FreeMapTbl error. "HERE);
		return (e_SC_RESULT_FAIL);
	}

	return (result);
}

/**
 * @brief クリップデータ解放：地図データ解放+リンクリスト領域
 * @param クリップリンクリスト
 */
static void freeClipData(RC_CLIPLINKLIST* aList) {

	E_DHC_CASH_RESULT cashResult = e_DHC_RESULT_CASH_SUCCESS;
	UINT16 count = 0;
	UINT32 i;

	if (NULL == aList) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return;
	}

	sRPMapReqTab.user = SC_DHC_USER_RP;

	// 地図解放
	for (i = 0; i < aList->areaVol; i++) {
		if ((NULL != aList->area[i].shapeBin) && (0 != aList->area[i].parcelId)) {
			sRPMapReqTab.parcelInfo[count].parcelId = aList->area[i].parcelId;
			sRPMapReqTab.parcelInfo[count].mapKind = SC_DHC_GetKindMask(e_DATA_KIND_SHAPE);
			count++;
		}
	}
	sRPMapReqTab.parcelNum = count;
	cashResult = SC_DHC_MapFree(&sRPMapReqTab);
	if (e_DHC_RESULT_CASH_SUCCESS != cashResult) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "SC_DHC_MapFree. [0x%08x] "HERE, cashResult);
	}

	// リンクリスト解放
	if (aList->list) {
		RP_MemFree(aList->list, e_MEM_TYPE_ROUTEPLAN);
	}
	if (aList->area) {
		RP_MemFree(aList->area, e_MEM_TYPE_ROUTEPLAN);
	}
	// 0クリア
	RP_Memset0(aList, sizeof(RC_CLIPLINKLIST));
}

/**
 * @brief 経路内から指定地点に対する最近傍を見つける
 * @param 経路管理
 * @param 指定地点
 * @param 発見パーセルインデックス
 * @param 発見リンクインデックス
 * @param 発見形状インデックス
 * @memo 区間は1区間のみのデータとすること
 */
E_SC_RESULT RC_FindMostNearRoute(SC_RP_RouteMng* aRouteMng, SCRP_POINT* aPoint, UINT32* aPclIdx, UINT32* aLinkIdx, UINT32* aFormIdx) {

	if (NULL == aRouteMng || NULL == aPoint || NULL == aPclIdx || NULL == aLinkIdx || NULL == aFormIdx) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SC_RP_LinkInfo* linkInfo = NULL;
	SC_RP_ParcelInfo* pclInfo = NULL;
	SC_RP_FormInfo* form = NULL;
	SC_RP_FormInfo* preForm = NULL;
	UINT32 pclId = 0;
	UINT8 level = RP_LEVEL1;
	UINT32 realX, realY;
	INT32 diffX, diffY;
	UINT16 magni = 0;
	INT32 sftX, sftY;
	UINT32 i, e, u;
	DOUBLE linkMostFormIdx = 0;
	DOUBLE mostNearLeave = DBL_MAX;
	UINT32 mostNearPclIdx = ALL_F32;
	UINT32 mostNearLinkIdx = ALL_F32;
	UINT32 mostNearFormIdx = ALL_F32;

	// 地図データ取得
	result = getRealLenTryLv2(aPoint->parcelId, &realX, &realY);
	if (e_SC_RESULT_SUCCESS != result) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "getRealLenTryLv2 error. [0x%08x] "HERE, result);
		return (result);
	}

	pclInfo = aRouteMng->parcelInfo;
	for (i = 0; i < aRouteMng->parcelVol; i++, pclInfo++) {
		level = MESHC_GetLevel(pclInfo->parcelId);
		// レベル別に倍率と差分を算出
		if (RP_LEVEL1 == level) {
			pclId = pclInfo->parcelId;
			magni = 1;
		} else if (RP_LEVEL2 == level) {
			pclId = SC_MESH_GetUnderLevelParcelID(pclInfo->parcelId, RP_LEVEL1);
			magni = 4;
		} else {
			result = e_SC_RESULT_FAIL;
			break;
		}
		// 差分算出
		SC_MESH_GetAlterPos(aPoint->parcelId, pclId, RP_LEVEL1, &sftX, &sftY);
		diffX = sftX * 4096;
		diffY = sftY * 4096;

		linkInfo = aRouteMng->linkInfo + pclInfo->linkIdx;
		for (e = 0; e < pclInfo->linkVol; e++, linkInfo++) {
			// 垂線計算
			DOUBLE linkMostNearLeave = DBL_MAX;
			form = aRouteMng->formInfo + linkInfo->formIdx;
			preForm = form++;
			for (u = 1; u < linkInfo->formVol; u++, form++) {
				RC_VERTICALPARAM verticalParam = {};
				verticalParam.cx = (DOUBLE) ((DOUBLE) aPoint->x * (DOUBLE) realX);
				verticalParam.cy = (DOUBLE) ((DOUBLE) aPoint->y * (DOUBLE) realY);
				verticalParam.ax = (DOUBLE) ((DOUBLE) (diffX + preForm->x * magni) * (DOUBLE) realX);
				verticalParam.ay = (DOUBLE) ((DOUBLE) (diffY + preForm->y * magni) * (DOUBLE) realY);
				verticalParam.bx = (DOUBLE) ((DOUBLE) (diffX + form->x * magni) * (DOUBLE) realX);
				verticalParam.by = (DOUBLE) ((DOUBLE) (diffY + form->y * magni) * (DOUBLE) realY);
				// 垂線をベクトル計算
				if (e_SC_RESULT_SUCCESS != calcVerticalPoint(&verticalParam)) {
					preForm = form;
					continue;
				}
				// リンク内で最短を採取
				if (verticalParam.cvLen < linkMostNearLeave) {
					linkMostNearLeave = verticalParam.cvLen;
					linkMostFormIdx = linkInfo->formIdx + u;
				}
				preForm = form;
			}
			// 短い結果を格納
			if (linkMostNearLeave < mostNearLeave) {
				mostNearLeave = linkMostNearLeave;
				mostNearPclIdx = i;
				mostNearLinkIdx = pclInfo->linkIdx + e;
				mostNearFormIdx = linkMostFormIdx;
			}
		}
	}

	// 結果
	*aPclIdx = mostNearPclIdx;
	*aLinkIdx = mostNearLinkIdx;
	*aFormIdx = mostNearFormIdx;

	return (result);
}
