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
 * RC_RouteMake.c
 * 推奨経路作成処理
 *  Created on: 2013/12/03
 *      Author: 70251034
 */

#include "sms-core/SMCoreRP/SMCoreRPInternal.h"

/*-------------------------------------------------------------------
 * 定数定義
 *-------------------------------------------------------------------*/
#define RC_ROUTE_WKMEM_SIZE		2000		// 推奨経路サイズ
#define RC_FORM_WKMEM_SIZE		3000		// 形状情報ワークブロック
#define RC_GET_SPLIT_LINK_DIST_RATIO1(a)		((a) / 10 * 8)
#define RC_GET_SPLIT_LINK_DIST_RATIO2(a)		((a) / 10 * 10)
#define RC_GET_SPLIT_LINK_DIST_RATIO3(a)		((a) / 10 * 11)
#define RC_GET_SPLIT_LINK_DIST_RATIO4(a)		((a) / 20 * 25)
#define RC_GET_SPLIT_LINK_COST_RATIO1(a)		((a) / 10 * 13)
#define RC_GET_SPLIT_LINK_COST_RATIO2(a)		((a) / 10 * 9)
#define RC_GET_SPLIT_LINK_COST_RATIO3(a)		((a) / 10 * 10)
#define RC_GET_SPLIT_LINK_COST_RATIO4(a)		((a) / 10 * 10)

#define RP_ROUTELINK_BLOCK_SIZE		300			// 経路リンク情報メモリ確保用ブロックサイズ
#define RP_ROUTEPCL_BLOCK_SIZE		50			// 経路パーセル情報メモリ確保用ブロックサイズ
/*-------------------------------------------------------------------
 * マクロ定義
 *-------------------------------------------------------------------*/
// 形状ワーク取得
#define RCRT_GET_WKFORM2NEXT(wkForm)			(wkForm.wkFormInfo + wkForm.formCrnt++)
// 候補経路ポインタ作業位置取得＋作業位置インクリメント
#define RCRT_GETMV_CRNTCAND(candWk)				(candWk->ppCandData + candWk->candCrnt++)
// 保存用経路管理ポインタ取得
#define RCRT_GET_RELEASEDROUTEINFO()			(&mReleasedRouteInfo)
// 最終経路管理ポインタ取得
#define RCRT_GET_SAVEROUTEINFO()				(&mSaveRouteInfo)

// 推奨経路テーブルへ形状オフセットを格納する
#define RCRT_SET_FORMOFS_FORROUTE(linkInfo, formOfs)		{						\
				linkInfo->formIdx = (UINT16) (formOfs & 0x0000FFFF);				\
				linkInfo->formVol = (UINT16) ((formOfs >> 16) & 0x0000FFFF);		\
	}
// 推奨経路テーブルから形状オフセットを取り出す
#define RCRT_GET_FORMOFS_FROMROUTE(linkInfo)		(linkInfo->formIdx & 0x0000FFFF) | ((linkInfo->formVol << 16) & 0xFFFF0000)

// 静的旅行時間を取得する
#define RCRT_GET_SIMPLETRAVELTIME(linkInfo)			((linkInfo->dist * LINK_TRAVEL_TIME_BASE[linkInfo->roadKind][linkInfo->linkKind]) / 100)

/*-------------------------------------------------------------------
 * 定数定義
 *-------------------------------------------------------------------*/
// 旅行時間算出用テーブル（コスト計算とかぶるけど別ものなので注意）
const static UINT32 LINK_TRAVEL_TIME_BASE[ROADTYPE_SIZE][LINKTYPE_SIZE] = 	//
		{ { 52, 48, 96, 115, 79, 1, 115, 1, 1, 1, 96, 1, 1 },			// 0:都市間高速
				{ 52, 48, 96, 115, 79, 1, 115, 1, 1, 1, 96, 1, 1 },			// 1:都市内高速
				{ 66, 60, 120, 144, 99, 1, 144, 1, 1, 1, 120, 1, 1 },		// 2:有料道路
				{ 720, 720, 720, 720, 720, 1, 720, 1, 1, 1, 720, 1, 1 },	// 3:他
				{ 66, 60, 120, 144, 99, 1, 144, 1, 1, 1, 120, 1, 1 },		// 4:国道
				{ 99, 90, 180, 216, 148, 1, 216, 1, 1, 1, 180, 1, 1 },		// 5:県道
				{ 79, 72, 144, 172, 118, 1, 172, 1, 1, 1, 144, 1, 1 },		// 6:主要地方道
				{ 113, 102, 205, 246, 169, 1, 246, 1, 1, 1, 205, 1, 1 },	// 7:一般道１（13m～）
				{ 132, 120, 240, 288, 198, 1, 288, 1, 1, 1, 240, 1, 1 },	// 8:一般道２（5.5m～13m）
				{ 264, 240, 480, 576, 396, 1, 576, 1, 1, 1, 480, 1, 1 },	// 9:一般道３（3m～5.5m）
				{ 264, 240, 480, 576, 396, 1, 576, 1, 1, 1, 480, 1, 1 },	// 10:細道路１（3m～5.5m）
				{ 360, 327, 654, 785, 540, 1, 785, 1, 1, 1, 654, 1, 1 },	// 11:細道路２（3m～5.5m）
				{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },					// 12:他 自転車専用等
				{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },					// 13:フェリー
				{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 11, 1, 1 },					// 14:カートレイン
				{ 720, 720, 720, 720, 720, 1, 720, 1, 1, 1, 720, 1, 1 }		// 15:歩道
		};
// 断裂採択道路種別読み替え
const static UINT32 mSplitRoadKindChange[16] = { 0, 0, 0, 3, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3 };

/*-------------------------------------------------------------------
 * 構造体定義
 *-------------------------------------------------------------------*/
// 推奨経路作成用始終端リンク情報
typedef struct _RCRT_TERMINFO {
	UINT32 fillDist;				// 塗りつぶし距離
	UINT32 linkDist;				// リンク距離
	UINT32 stSubIdx;				// 形状開始インデックス 0ならば形状を通常通り格納
	UINT32 formVol;					// 形状数
	UINT32 formVolDiff;				// 形状数 減算分
	UINT32 editIdx;					// 形状編集Index
	UINT16 vX;						// stSubIdx 0ならば参照不要
	UINT16 vY;						// stSubIdx 0ならば参照不要
} RCRT_TERMINFO;

// 形状作業用
typedef struct _RCRT_FORM_WKTBL {
	SC_RP_FormInfo* wkFormInfo;		// 形状作業領域
	UINT32 formSize;				// 形状領域サイズ
	UINT32 formCrnt;				// 形状使用位置
} RCRT_FORM_WKTBL;

// 候補データ作業用
typedef struct _RCRT_CAND_WKTBL {
	SCRP_CANDDATA** ppCandData;		// 候補経路ポインタ格納用領域
	UINT32 candSize;				// 候補領域サイズ
	UINT32 candCrnt;				// 候補使用位置
	Bool startIsNeigbor;			// 開始リンク元情報は近傍
} RCRT_CAND_WKTBL;

// 推奨経路管理
typedef struct _RCRT_SAVEROUTEINFO {
	UINT16 sectCurrent;		// 最終経路管理の作業区間位置
	SC_RP_RouteMng routeMng;		// 区間探索時の保存
} RCRT_SAVEROUTEINFO;

// リリース済み推奨経路管理
typedef struct _RCRT_RELEASEDROUTEINFO {
	UINT16 saveSectVol;				// 最終経路管理の作業区間位置
	SC_RP_RouteMng routeMng;		// 区間探索時の保存
} RCRT_RELEASEDROUTEINFO;

// 経路保存管理
static RCRT_SAVEROUTEINFO mSaveRouteInfo = {};
static RCRT_RELEASEDROUTEINFO mReleasedRouteInfo = {};

/* プロトタイプ宣言 */
static void freeRouteManager(SC_RP_RouteMng* aRoute, E_SC_MEM_TYPE aMemType);
static E_SC_RESULT mallocWorkCand(RCRT_CAND_WKTBL* aCollectionMng);
static E_SC_RESULT mallocWorkFormInfo(RCRT_FORM_WKTBL* aFromWkTbl);
static E_SC_RESULT mallocRouteSectInfo(SC_RP_RouteMng* aRouteMng, UINT32 aAddSize);
static E_SC_RESULT mallocRoutePclInfo(SC_RP_RouteMng* aRouteMng, UINT32 aAddSize);
static E_SC_RESULT mallocFormInfo(SC_RP_RouteMng* aRouteMng, UINT32 aAddSize);
static E_SC_RESULT mallocRouteLinkInfo(SC_RP_RouteMng* aRouteMng, UINT32 aAddSize);
static E_SC_RESULT routeCandSelecter(SCRP_SECTCONTROLER* aSectCtrl, UINT32 aStCandIdx, RCRT_CAND_WKTBL* aCandWkTbl);
static E_SC_RESULT routeCandSearchNbrLinkMinCost(SCRP_SECTCONTROLER* aSectCtrl, SCRP_CANDTBLINFO* aCandInfo, UINT32* aStCandIndex);
static E_SC_RESULT routeCandSearchNextStep(SCRP_SECTCONTROLER* aSectCtrl, SCRP_CANDTBLINFO* aCandInfo, SCRP_CANDDATA* aPreCand,
		UINT32* aStCandIndex);
static E_SC_RESULT routeCandSearchSplitLinkMinCost(SCRP_SECTCONTROLER* aSectCtrl, SCRP_CANDTBLINFO* aCandInfo, UINT32* aStCandIndex);
static E_SC_RESULT makeRouteCandList(SCRP_SECTCONTROLER* aSectCtrl, RCRT_CAND_WKTBL* aCandWkTbl);
static Bool checkSameTarmLink(SC_RP_RouteMng* aNewRouteMng);
static E_SC_RESULT searchSameTarmLink(SC_RP_RouteMng* aNewRouteMng, SCRP_SECTCONTROLER* aSectCtrl);
static Bool checkSameLevelOverArea(SCRP_SECTCONTROLER* aSectCtrl);
static UINT32 searchSameLvConnectCand(SCRP_SECTCONTROLER* aSectCtrl, UINT8 level, RCRT_CAND_WKTBL* aCandWkTbl);
static E_SC_RESULT getTravelTime(SCRP_MAPDATA* aMapInfo, UINT32 aLinkId, UINT32* aResultCost);
static T_MapNWLink* getRoadLinkInfo(SCRP_MAPDATA* aMapData, UINT32 aLinkId);
static E_SC_RESULT setRouteMapVer(SC_RP_RouteMng* aRoutMng);
static E_SC_RESULT setRouteSectInfo(SC_RP_RouteMng* aRouteManager, UINT8 aSectIdx);
static E_SC_RESULT makeRoutePclLinkTbl(RCRT_CAND_WKTBL* aCandWkTbl, SC_RP_RouteMng* aRouteMng);
static E_SC_RESULT makeRoutePclLinkTblNormal(SCRP_CANDDATA** ppCand, INT32 candIdxMark, UINT32 targetLinkVol, SC_RP_RouteMng* aRouteMng,
		UINT32* aPclCrnt, UINT32* aLinkCrnt);
static E_SC_RESULT makeRoutePclLinkTblForLvConvert(SCRP_LVCHANGE_TBL* aLvChange, SC_RP_RouteMng* aRouteMng, UINT32* aPclCrnt,
		UINT32* aLinkCrnt);
static E_SC_RESULT makeRouteFormTbl(SCRP_SECTCONTROLER* aSectCtrl, SC_RP_RouteMng* aRouteMng);
static E_SC_RESULT makeRouteTarmLinkInfo(SCRP_SECTCONTROLER* aSectCtrl, SC_RP_RouteMng* aRouteMng, RCRT_FORM_WKTBL* aFormWkTbl);
static E_SC_RESULT splitRouteTrim(SCRP_SECTCONTROLER* aSectCtrl, SC_RP_RouteMng* aRouteMng);
static E_SC_RESULT routeTblStrip(UINT32 aRemParcelVol, UINT32 aRemLinkVol, SC_RP_RouteMng* aRouteMng);
static E_SC_RESULT jointRouteManager(SC_RP_RouteMng* aNewSectRouteMng, UINT8 aSectVol, SCRP_SECTCONTROLER* aSectCtrl);
static E_SC_RESULT getTermNbrLinkInfo(SCRP_NEIGHBORINFO* aNeigbor, UINT32 aParcelId, SC_RP_LinkInfo* aLinkInfo, Bool aIsStart,
		RCRT_TERMINFO* aTermInfo);
static Bool checkSectPointNear(SCRP_SECTCONTROLER* aSectCtrl);

/**
 * @brief 推奨経路アドレス取得
 */
SC_RP_RouteMng* RC_GetMasterRoute() {
	RCRT_SAVEROUTEINFO* saveRoute = RCRT_GET_SAVEROUTEINFO();
	return (&saveRoute->routeMng);
}

/**
 * @brief 経路作成処理
 * @param 探索管理
 * @param 区間管理
 * @param 区間インデックス
 */
E_SC_RESULT RC_RouteMake(SCRP_MANAGER* aRPManager, SCRP_SECTCONTROLER* aSectCtrl, UINT16 aSection) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

#if _RPLAPTIME_ROUTEMAKE // ★時間計測
	RP_SetLapTime();
#endif
	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	//SCRP_CANDDATA *lastLink = NULL;
	//UINT32 linkCnt = 0;
	//UINT16 pclCnt = 0;
	SC_RP_RouteMng sectRouteMng = {};
	RCRT_CAND_WKTBL candWkTbl = {};

	// プロセス登録
	RPC_SetCalcProcess(e_RC_PROC_MAKEROUTE);

	if (NULL == aRPManager || NULL == aSectCtrl) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	do {

		// ワーク領域へ候補経路アドレスリストを展開（非対応レベルはここではじかれる）
		result = makeRouteCandList(aSectCtrl, &candWkTbl);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "makeRouteCand error. [0x%08x] "HERE, result);
			break;
		}

		// パーセル・リンク情報作成
		result = makeRoutePclLinkTbl(&candWkTbl, &sectRouteMng);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "setRouteLinkInfo error. [0x%08x] "HERE, result);
			break;
		}

		// 形状点情報の作成処理
		result = makeRouteFormTbl(aSectCtrl, &sectRouteMng);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "setRouteFormInfo error. [0x%08x] "HERE, result);
			break;
		}

		// 断裂経路の場合経路の切り捨て処理を行う
		result = splitRouteTrim(aSectCtrl, &sectRouteMng);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "splitRouteTrim error. [0x%08x] "HERE, result);
			break;
		}

		// 作業領域の経路管理と前区間までの経路をつなぎ合わせる
		result = jointRouteManager(&sectRouteMng, aRPManager->sectVol, aSectCtrl);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "jointRouteManager error. [0x%08x] "HERE, result);
			break;
		}
	} while (0);

	// 候補リスト解放
	if (NULL != candWkTbl.ppCandData) {
		RP_MemFree(candWkTbl.ppCandData, e_MEM_TYPE_ROUTEPLAN);
	}

	// 今回区間のデータを開放
	freeRouteManager(&sectRouteMng, e_MEM_TYPE_ROUTEMNG);

	// 地図全開放
	RC_MapFreeAll();

#if _RPLAPTIME_ROUTEMAKE // ★時間計測
	RP_SetLapTimeWithStr("routemake.");
#endif
#if _RP_LOG_MASTERROUTE
	if (e_SC_RESULT_SUCCESS == result) {
		RPDBG_DumpRoute(&mSaveRouteInfo.routeMng, true, false);
	}
#endif

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief 推奨経路管理内部テーブル初期処理
 * @memo 新規探索開始時に必ず実行すること
 */
void RC_ClearRouteMakeInfo() {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	RCRT_SAVEROUTEINFO* saveRouteInfo = RCRT_GET_SAVEROUTEINFO();
	RCRT_RELEASEDROUTEINFO* releasedRouteInfo = RCRT_GET_RELEASEDROUTEINFO();

	RP_Memset0(saveRouteInfo, sizeof(RCRT_SAVEROUTEINFO));
	RP_Memset0(releasedRouteInfo, sizeof(RCRT_RELEASEDROUTEINFO));

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
}

/**
 * @brief 推奨経路管理内部テーブル終了処理
 */
void RC_FinalRouteMakeTbl() {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	RCRT_SAVEROUTEINFO* saveRouteInfo = RCRT_GET_SAVEROUTEINFO();
	RCRT_RELEASEDROUTEINFO* releasedRouteInfo = RCRT_GET_RELEASEDROUTEINFO();

	RP_Memset0(saveRouteInfo, sizeof(RCRT_SAVEROUTEINFO));
	RP_Memset0(releasedRouteInfo, sizeof(RCRT_RELEASEDROUTEINFO));

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
}

/**
 * @brief 探索失敗時作業領域の解放を行う終了処理
 */
void RC_ResultFailFinalRouteMakeTbl() {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	RCRT_SAVEROUTEINFO* saveRouteInfo = RCRT_GET_SAVEROUTEINFO();
	freeRouteManager(&saveRouteInfo->routeMng, e_MEM_TYPE_ROUTEMNG);
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
}

/**
 * @brief 推奨経路区間テーブル確保
 * @param 探索予定区間数
 * @memo 区間探索開始前に実行する
 */
E_SC_RESULT RC_CreateSectMngTbl(UINT16 aSectVol) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (0 == aSectVol) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	RCRT_SAVEROUTEINFO* saveRoute = RCRT_GET_SAVEROUTEINFO();

	do {
		// 区間情報領域確保
		result = mallocRouteSectInfo(&saveRoute->routeMng, aSectVol);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "mallocRouteSectInfo error. [0x%08x] "HERE, result);
			break;;
		}

		// 地図バージョンの登録
		result = setRouteMapVer(&saveRoute->routeMng);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "setRouteMapVer error. [0x%08x] "HERE, result);
			break;;
		}
		// 経路バージョンの登録
		if (SC_RP_ROUTEVER_SIZE >= strlen(RC_ROUTE_VERSION)) {
			strncpy(saveRoute->routeMng.routeVer, RC_ROUTE_VERSION, strlen(RC_ROUTE_VERSION));
		} else {
			SC_LOG_DebugPrint(SC_TAG_RC, "route version size is over... "HERE);
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief リルート用管理情報保存
 * @param コピー元経路
 * @param コピー開始区間INDEX
 * @param コピー区間数
 * @memo リリース済み経路の通過区間以降を保存する。保存した経路は探索後に結合処理を行う。
 */
E_SC_RESULT RC_BackupReleasedRoute(SC_RP_RouteMng* aReleasedRouteMng, UINT16 aStartIdx, UINT16 aBackupSize) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT result = e_SC_RESULT_FAIL;
	RCRT_RELEASEDROUTEINFO* releasedRouteInfo = RCRT_GET_RELEASEDROUTEINFO();

	if (NULL == aReleasedRouteMng) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (0 == aBackupSize || aReleasedRouteMng->sectVol < (aStartIdx + aBackupSize)) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	// 経路切り取り
	result = RC_RtCutting(aReleasedRouteMng, aStartIdx, aBackupSize, &releasedRouteInfo->routeMng);
	if (e_SC_RESULT_SUCCESS != result) {
		SC_LOG_DebugPrint(SC_TAG_RC, "RC_RtCutting error. [0x%08x] "HERE, result);
	}
	// バックアップサイズを保持
	releasedRouteInfo->saveSectVol = aBackupSize;

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief リルート用経路結合
 * @memo リルート対象の区間探索後にコールする。
 *       前提：リルート探索開始時にリルート対象以外の経路の保持
 */
E_SC_RESULT RC_JointRePlanRoute() {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	RCRT_SAVEROUTEINFO* saveRoute = RCRT_GET_SAVEROUTEINFO();
	RCRT_RELEASEDROUTEINFO* releasedRoute = RCRT_GET_RELEASEDROUTEINFO();
	SC_RP_RouteMng newRouteBkup = {};
	SC_RP_RouteMng resultRouteMng = {};
	SC_RP_RouteMng* joinRouteList[2] = {};

	// 保持経路がなければ処理しない
	if (0 == releasedRoute->saveSectVol) {
		SC_LOG_DebugPrint(SC_TAG_RC, "no released route... " HERE);
		return (e_SC_RESULT_SUCCESS);
	}

	do {
		// 今の経路管理情報バックアップ
		RP_Memcpy(&newRouteBkup, &saveRoute->routeMng, sizeof(SC_RP_RouteMng));

		// 結合する経路をリストへ詰める
		joinRouteList[0] = &saveRoute->routeMng;
		joinRouteList[1] = &releasedRoute->routeMng;

		// 経路結合
		result = RC_RtJoin(joinRouteList, 2, &resultRouteMng);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RC_RtJoin error. [0x%08x] " HERE, result);
			break;
		}

		// 経路結合成功時
		RP_Memcpy(&saveRoute->routeMng, &resultRouteMng, sizeof(SC_RP_RouteMng));

	} while (0);

	// 作業領域解放
	freeRouteManager(&releasedRoute->routeMng, e_MEM_TYPE_ROUTEMNG);
	freeRouteManager(&newRouteBkup, e_MEM_TYPE_ROUTEMNG);

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief 経路内の断裂有無を判定する。
 * @param 推奨経路
 */
Bool RC_CheckSplitRoute(SC_RP_RouteMng* aRouteMng) {

	if (NULL == aRouteMng) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. " HERE);
		return (false);
	}

	UINT32 i;
	for (i = 0; i < aRouteMng->sectVol; i++) {
		if (ALL_F16 != (aRouteMng->sectInfo + i)->splitIdx) {
			return (true);
		}
	}
	return (false);
}

/**
 * @brief 推奨経路開放処理
 * @param 推奨経路管理
 */
static void freeRouteManager(SC_RP_RouteMng* aRoute, E_SC_MEM_TYPE aMemType) {

	if (NULL == aRoute) {
		return;
	}

	if (NULL != aRoute->parcelInfo) {
		RP_MemFree(aRoute->parcelInfo, aMemType);
	}
	if (NULL != aRoute->linkInfo) {
		RP_MemFree(aRoute->linkInfo, aMemType);
	}
	if (NULL != aRoute->formInfo) {
		RP_MemFree(aRoute->formInfo, aMemType);
	}
	if (NULL != aRoute->regInfo) {
		RP_MemFree(aRoute->regInfo, aMemType);
	}
	if (NULL != aRoute->sectInfo) {
		RP_MemFree(aRoute->sectInfo, aMemType);
	}
	RP_Memset0(aRoute, sizeof(SC_RP_RouteMng));
}

/**
 * @brief 候補リンクワーク領域確保
 * @param 候補経路ワーク管理
 */
static E_SC_RESULT mallocWorkCand(RCRT_CAND_WKTBL* aCollectionMng) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aCollectionMng) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 領域確保＋0クリア
	UINT32 newSize = aCollectionMng->candSize + RC_ROUTE_WKMEM_SIZE;
	SCRP_CANDDATA** newBuf = RP_MemAlloc(sizeof(SCRP_CANDDATA*) * newSize, e_MEM_TYPE_ROUTEPLAN);
	if (NULL == newBuf) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error. "HERE);
		return (e_SC_RESULT_MALLOC_ERR);
	}
	RP_Memset0(newBuf, sizeof(SCRP_CANDDATA*) * newSize);

	// データ復帰
	if (0 < aCollectionMng->candSize) {
		// kana暫定
		//RP_Memcpy(newBuf, aCollectionMng->ppCandData, sizeof(SCRP_CANDDATA) * aCollectionMng->candSize);
		RP_Memcpy(newBuf, aCollectionMng->ppCandData, sizeof(SCRP_CANDDATA*) * aCollectionMng->candSize);
		RP_MemFree(aCollectionMng->ppCandData, e_MEM_TYPE_ROUTEPLAN);
	}

	// 結果格納
	aCollectionMng->ppCandData = newBuf;
	aCollectionMng->candSize = newSize;

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 形状情報ワーク領域確保
 * @param 形状情報ワーク管理
 */
static E_SC_RESULT mallocWorkFormInfo(RCRT_FORM_WKTBL* aFromWkTbl) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aFromWkTbl) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	UINT32 newSize = aFromWkTbl->formSize + RC_FORM_WKMEM_SIZE;
	SC_RP_FormInfo* newBuf = RP_MemAlloc(sizeof(SC_RP_FormInfo) * newSize, e_MEM_TYPE_ROUTEPLAN);
	if (NULL == newBuf) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error. "HERE);
		return (e_SC_RESULT_MALLOC_ERR);
	}
	RP_Memset0(newBuf, sizeof(SC_RP_FormInfo) * newSize);

	// 旧データ復帰
	if (0 < aFromWkTbl->formSize) {
		RP_Memcpy(newBuf, aFromWkTbl->wkFormInfo, sizeof(SC_RP_FormInfo) * aFromWkTbl->formSize);
		RP_MemFree(aFromWkTbl->wkFormInfo, e_MEM_TYPE_ROUTEPLAN);
	}

	// 結果格納
	aFromWkTbl->wkFormInfo = newBuf;
	aFromWkTbl->formSize = newSize;

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 推奨経路区間管理情報確保
 * @param 推奨経路管理
 */
static E_SC_RESULT mallocRouteSectInfo(SC_RP_RouteMng* aRouteMng, UINT32 aAddSize) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aRouteMng) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	UINT32 newSize = aRouteMng->sectVol + aAddSize;
	if (SC_RP_SECT_MAX < newSize) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "route section count over... "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	SC_RP_SectInfo* newBuf = RP_MemAlloc(sizeof(SC_RP_SectInfo) * newSize, e_MEM_TYPE_ROUTEMNG);
	if (NULL == newBuf) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error. "HERE);
		return (e_SC_RESULT_MALLOC_ERR);
	}
	RP_Memset0(newBuf, sizeof(SC_RP_SectInfo) * newSize);

	// 旧データ復帰
	if (0 < aRouteMng->sectVol) {
		RP_Memcpy(newBuf, aRouteMng->sectInfo, sizeof(SC_RP_SectInfo) * aRouteMng->sectVol);
		RP_MemFree(aRouteMng->sectInfo, e_MEM_TYPE_ROUTEMNG);
	}

	// 結果格納
	aRouteMng->sectInfo = newBuf;
	aRouteMng->sectVol = newSize;

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 推奨経路パーセル情報確保
 * @param 推奨経路管理
 */
static E_SC_RESULT mallocRouteLinkInfo(SC_RP_RouteMng* aRouteMng, UINT32 aAddSize) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aRouteMng) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	UINT32 newSize = aRouteMng->linkVol + aAddSize;
	SC_RP_LinkInfo* newBuf = RP_MemAlloc(sizeof(SC_RP_LinkInfo) * newSize, e_MEM_TYPE_ROUTEMNG);
	if (NULL == newBuf) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error. "HERE);
		return (e_SC_RESULT_MALLOC_ERR);
	}
	RP_Memset0(newBuf, sizeof(SC_RP_LinkInfo) * newSize);

	// 旧データ復帰
	if (0 < aRouteMng->linkVol) {
		RP_Memcpy(newBuf, aRouteMng->linkInfo, sizeof(SC_RP_LinkInfo) * aRouteMng->linkVol);
		RP_MemFree(aRouteMng->linkInfo, e_MEM_TYPE_ROUTEMNG);
	}

	// 結果格納
	aRouteMng->linkInfo = newBuf;
	aRouteMng->linkVol = newSize;

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 推奨経路パーセル情報確保
 * @param 推奨経路管理
 */
static E_SC_RESULT mallocRoutePclInfo(SC_RP_RouteMng* aRouteMng, UINT32 aAddSize) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aRouteMng) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	UINT32 newSize = aRouteMng->parcelVol + aAddSize;
	SC_RP_ParcelInfo* newBuf = RP_MemAlloc(sizeof(SC_RP_ParcelInfo) * newSize, e_MEM_TYPE_ROUTEMNG);
	if (NULL == newBuf) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error. "HERE);
		return (e_SC_RESULT_MALLOC_ERR);
	}
	RP_Memset0(newBuf, sizeof(SC_RP_ParcelInfo) * newSize);

	// 旧データ復帰
	if (0 < aRouteMng->parcelVol) {
		RP_Memcpy(newBuf, aRouteMng->parcelInfo, sizeof(SC_RP_ParcelInfo) * aRouteMng->parcelVol);
		RP_MemFree(aRouteMng->parcelInfo, e_MEM_TYPE_ROUTEMNG);
	}

	// 結果格納
	aRouteMng->parcelInfo = newBuf;
	aRouteMng->parcelVol = newSize;

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 形状情報領域確保
 * @param 推奨経路作業管理
 */
static E_SC_RESULT mallocFormInfo(SC_RP_RouteMng* aRouteMng, UINT32 aAddSize) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aRouteMng) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	UINT32 newSize = aRouteMng->formVol + aAddSize;
	SC_RP_FormInfo* newBuf = RP_MemAlloc(sizeof(SC_RP_FormInfo) * newSize, e_MEM_TYPE_ROUTEMNG);
	if (NULL == newBuf) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error. "HERE);
		return (e_SC_RESULT_MALLOC_ERR);
	}
	RP_Memset0(newBuf, sizeof(SC_RP_FormInfo) * newSize);

	// 旧データ復帰
	if (0 < aRouteMng->formVol) {
		RP_Memcpy(newBuf, aRouteMng->formInfo, sizeof(SC_RP_FormInfo) * aRouteMng->formVol);
		RP_MemFree(aRouteMng->formInfo, e_MEM_TYPE_ROUTEMNG);
	}

	// 結果格納
	aRouteMng->formInfo = newBuf;
	aRouteMng->formVol = newSize;

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 推奨経路作成用ワークへ候補経路アドレスを登録する。
 * @param 区間管理
 * @param ワーク候補リスト
 * @memo TODO 同一レベル接続の複数レベル対応
 */
static E_SC_RESULT makeRouteCandList(SCRP_SECTCONTROLER* aSectCtrl, RCRT_CAND_WKTBL* aCandWkTbl) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SCRP_CANDSTARTLINK* minCostStCand = NULL;
	//SCRP_CANDTBLINFO* candInfo = NULL;
	SCRP_CANDTBLINFO* candInfoList[4] = { NULL, NULL, NULL, NULL };
	UINT32 stCandIdx = ALL_F32;
	Bool sameArea = false;

	if (NULL == aSectCtrl) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	do {
		// ターゲットの候補管理取得
		switch (aSectCtrl->levelTable.topLevel) {
		case RP_LEVEL1:
			candInfoList[0] = &aSectCtrl->candMng.candTblInfo[RC_CAND_IDX_LV1TOP];
			break;
		case RP_LEVEL2:
			candInfoList[0] = &aSectCtrl->candMng.candTblInfo[RC_CAND_IDX_LV1D];
			candInfoList[1] = &aSectCtrl->candMng.candTblInfo[RC_CAND_IDX_LV2TOP];
			candInfoList[2] = &aSectCtrl->candMng.candTblInfo[RC_CAND_IDX_LV1O];
			sameArea = checkSameLevelOverArea(aSectCtrl);
			break;
		default:
			result = e_SC_RESULT_BADPARAM;
			SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
			break;
		}
		if (e_SC_RESULT_SUCCESS != result) {
			break;
		}

		// 出発地到達までのステップ数分の処理を行う
		UINT8 idx = 0;
		while (NULL != candInfoList[idx]) {
			UINT32 candIdx = ALL_F32;
			Bool isSplit = false;
			if (ALL_F32 == stCandIdx) {
				// 最小コスト候補開始検索
				result = routeCandSearchNbrLinkMinCost(aSectCtrl, candInfoList[idx], &stCandIdx);
				if (e_SC_RESULT_SUCCESS != result) {
					SC_LOG_ErrorPrint(SC_TAG_RC, "routeCandSearchNbrLinkMinCost error. [0x%08x] "HERE, result);
					break;
				}
				if (ALL_F32 == stCandIdx) {
					// 最小コスト検索（SplitFlagリンクからのみ）
					result = routeCandSearchSplitLinkMinCost(aSectCtrl, candInfoList[idx], &stCandIdx);
					if (e_SC_RESULT_SUCCESS != result) {
						SC_LOG_ErrorPrint(SC_TAG_RC, "routeCandSearchSplitLinkMinCost error. [0x%08x] "HERE, result);
						break;
					}
					// 当該ステップでは見つからない場合次
					if (ALL_F32 == stCandIdx) {
						idx++;
						continue;
					}
					// 開始リンクの情報元設定
					aCandWkTbl->startIsNeigbor = false;
					// 近傍以外から開始リンクを見つけた
					isSplit = true;
				} else {
					// 開始リンクの情報元設定
					aCandWkTbl->startIsNeigbor = true;
				}
			} else {
				// 2回目以降は接続情報特定から
				SCRP_CANDDATA* candwk = *(aCandWkTbl->ppCandData + aCandWkTbl->candCrnt - 1);
				result = routeCandSearchNextStep(aSectCtrl, candInfoList[idx], candwk, &stCandIdx);
				if (e_SC_RESULT_SUCCESS != result) {
					SC_LOG_ErrorPrint(SC_TAG_RC, "routeCandSearchNextStep error. "HERE);
					break;
				}
			}

			// 同一レベル接続検索（D側処理後）
			if (sameArea && idx == 1) {
				candIdx = searchSameLvConnectCand(aSectCtrl, RP_LEVEL1, aCandWkTbl);
				if (ALL_F32 != candIdx) {
					// lv2Topを飛ばす為インクリメント
					idx++;
				}
			}
			if (ALL_F32 == candIdx) {
				// 候補開始リンク
				minCostStCand = aSectCtrl->candMng.stLink + stCandIdx;
				candIdx = minCostStCand->candIdx;

				// Lv1→Lv2接続
				if (!isSplit && minCostStCand->connectLevel == RP_LEVEL1) {
					aCandWkTbl->candCrnt -= 1;
				}
				// Lv2→Lv1接続
				if (!isSplit && minCostStCand->connectLevel == RP_LEVEL2) {
					aCandWkTbl->candCrnt -= 1;
				}
			}

			// 候補リスト作成
			result = routeCandSelecter(aSectCtrl, candIdx, aCandWkTbl);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "routeCandSelecter error. "HERE);
				break;
			}
			idx++;
		}
	} while (0);

	// 開始リンクゼロ
	if (e_SC_RESULT_SUCCESS == result) {
		if (ALL_F32 == stCandIdx || 0 == aCandWkTbl->candCrnt) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "cand selecter error. stCandIdx=0x%08x candCrnt=%d "HERE, stCandIdx, aCandWkTbl->candCrnt);
			result = e_SC_RESULT_FAIL;
		}
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief 最小コスト候補開始リンク検索：近傍
 * @param 区間管理
 * @param 候補テーブル情報
 * @param [O]候補開始リンクインデックス
 */
static E_SC_RESULT routeCandSearchNbrLinkMinCost(SCRP_SECTCONTROLER* aSectCtrl, SCRP_CANDTBLINFO* aCandInfo, UINT32* aStCandIndex) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aSectCtrl || NULL == aCandInfo || NULL == aStCandIndex) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SCRP_CANDSTARTLINK* stCand = NULL;
	SCRP_NEIGHBORLINK* neigbor = aSectCtrl->neighbor[SCRP_NBRDST].neighborLink;
	SCRP_NEIGHBORLINK* preNeigbor = NULL;
	UINT32 preCost = ALL_F32;
	UINT32 resultIdx = ALL_F32;
	UINT32 i, e;

	// 最小コスト近傍検索
	for (i = 0; i < aSectCtrl->neighbor[SCRP_NBRDST].nbrLinkVol; i++, neigbor++) {
		stCand = aSectCtrl->candMng.stLink + aCandInfo->stLinkIdx;
		for (e = 0; e < aCandInfo->stLinkSize; e++, stCand++) {
			// 完全一致リンク判定
			if (neigbor->parcelId != stCand->parcelId || neigbor->linkId != stCand->linkId) {
				continue;
			}
			if (neigbor->orFlag != RCND_GET_ORIDX(stCand->flag)) {
				continue;
			}
			if (preCost < (stCand->cost + neigbor->cost)) {
				continue;
			}
			if (preCost == (stCand->cost + neigbor->cost)) {
				if (NULL != preNeigbor && preNeigbor->leavDist <= neigbor->leavDist) {
					continue;
				}
			}
#if 0
			SC_LOG_InfoPrint(SC_TAG_RC, "nbr hit ... pcl=0x%08x link=0x%08x leav=%f stCand=%5d", neigbor->parcelId, neigbor->linkId,
					neigbor->leavDist, stCand->candIdx);
#endif
			// リンク情報を保存
			preNeigbor = neigbor;
			preCost = stCand->cost + neigbor->cost;
			// Index登録
			resultIdx = aCandInfo->stLinkIdx + e;
			break;
		}
	}
	// 結果格納
	*aStCandIndex = resultIdx;

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief 候補開始リンク検索：候補
 * @param 区間管理
 * @param 候補テーブル情報
 * @param [O]候補開始リンクインデックス
 */
static E_SC_RESULT routeCandSearchNextStep(SCRP_SECTCONTROLER* aSectCtrl, SCRP_CANDTBLINFO* aCandInfo, SCRP_CANDDATA* aPreCand,
		UINT32* aStCandIndex) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aSectCtrl || NULL == aCandInfo || NULL == aStCandIndex) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SCRP_CANDSTARTLINK* stCand = NULL;
	//UINT32 preCost = ALL_F32;
	//UINT32 resultIdx = ALL_F32;
	UINT32 i;

	// 初期化
	*aStCandIndex = ALL_F32;

	for (i = 0; i < aCandInfo->stLinkSize; i++) {
		stCand = aSectCtrl->candMng.stLink + aCandInfo->stLinkIdx + i;
		// 完全一致リンク判定
		if (aPreCand->parcelId != stCand->st.parcelId) {
			continue;
		}
		if (RCID_GET_LINKIDX(aPreCand->linkId) != RCID_GET_LINKIDX(stCand->st.linkId)) {
			continue;
		}
		UINT32 or = (SC_MA_NWID_IS_CNCTSIDE_ODR(stCand->st.linkId)) ? SCRP_LINKODR : SCRP_LINKRVS;
		if (RCND_GET_ORIDX(aPreCand->flag) != or) {
			continue;
		}
#if 0
		SC_LOG_InfoPrint(SC_TAG_RC, "hit next step start... pcl=0x%08x link=0x%08x pcl=0x%08x link=0x%08x . ", aPreCand->parcelId,
				aPreCand->linkId, stCand->st.parcelId, stCand->st.linkId);
#endif
		// 結果格納
		*aStCandIndex = aCandInfo->stLinkIdx + i;
		break;
	}

	if (ALL_F32 == *aStCandIndex) {
		SC_LOG_WarnPrint(SC_TAG_RC, "can't found start cand... pcl=0x%08x link=0x%08x . ", aPreCand->parcelId, aPreCand->linkId);
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief 候補開始リンク検索：断裂
 * @param 区間管理
 * @param 候補テーブル情報
 * @param [O]開始候補リンクインデックス
 * @memo 高速・一般道・細道路 それぞれに対して最短距離の道路を採択
 *       距離閾値 10km
 *       コスト閾値 20%
 */
static E_SC_RESULT routeCandSearchSplitLinkMinCost(SCRP_SECTCONTROLER* aSectCtrl, SCRP_CANDTBLINFO* aCandInfo, UINT32* aStCandIndex) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	//const UINT32 kindCvt[17] = { 0, 0, 0, 0, 3, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3 };

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	E_DHC_CASH_RESULT cashResult = e_DHC_RESULT_CASH_SUCCESS;
	UINT32 minCostStCandIdx[4] = { ALL_F32, ALL_F32, ALL_F32, ALL_F32 };
	SCRP_CANDSTARTLINK* minCostStCand[4] = {};
	SCRP_CANDSTARTLINK* stCand = NULL;
	T_MapNWLink* pLink = NULL;
	UINT32 dist_save[4] = {};
	UINT32 dist_b = 0;
	UINT8 cvKind = 0;
	UINT32 i = 0;
	SCRP_POINT point = {};
	Bool regist_f = false;		// 登録対象フラグ
	SCRP_MAPREADTBL readTab = {};

	if (NULL == aSectCtrl || NULL == aCandInfo) {
		return (e_SC_RESULT_BADPARAM);
	}
	stCand = aSectCtrl->candMng.stLink + aCandInfo->stLinkIdx;

	// 最小コスト検索
	for (i = 0; i < aCandInfo->stLinkSize; i++, stCand++) {
		// 初期化
		regist_f = false;
		// 断裂リンクでない場合無視
		if (!RCND_GET_CANDSPLITFLG(stCand->flag)) {
			continue;
		}

		// 地図読み込み
		result = RC_ReadListMap(&stCand->parcelId, 1, SC_DHC_KIND_ROAD, &readTab);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RC_ReadListMap error. [0x%08x] "HERE, result);
			break;
		}
		if (NULL == readTab.mapList->road) {
			SC_LOG_DebugPrint(SC_TAG_RC, "map road read failed. pcl=0x%08x [0x%08x] "HERE, stCand->parcelId, cashResult);
			continue;;
		}
		pLink = getRoadLinkInfo(readTab.mapList, stCand->linkId);
		if (NULL == pLink) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "getRoadLinkInfo error. "HERE);
			result = e_SC_RESULT_FAIL;
			break;
		}
		// 種別変換
		cvKind = mSplitRoadKindChange[pLink->linkBaseInfo.b_code.roadKind];

		// 初回
		if (NULL == minCostStCand[cvKind]) {
			point.parcelId = stCand->parcelId;
			point.x = pLink->coordXSt;
			point.y = pLink->coordYSt;
			dist_b = (UINT32) RP_Lib_CalcODLength(aSectCtrl->neighbor[1].point, point);
			regist_f = true;
		}
		// 初回以降
		else {
			point.parcelId = stCand->parcelId;
			point.x = pLink->coordXSt;
			point.y = pLink->coordYSt;
			dist_b = (UINT32) RP_Lib_CalcODLength(aSectCtrl->neighbor[1].point, point);
			switch (cvKind) {
			case 0:		// 高速or有料道路
			case 1:		// 国道or県道
			case 2:		// 主要地方道以下

				// 距離差が閾値1未満→無条件登録
				if (RC_GET_SPLIT_LINK_DIST_RATIO1(dist_save[cvKind]) > dist_b) {
					regist_f = true;
				}
				// 距離差が閾値2未満→コストが閾値1未満で登録
				else if (RC_GET_SPLIT_LINK_DIST_RATIO2(dist_save[cvKind]) > dist_b) {
					if (RC_GET_SPLIT_LINK_COST_RATIO1(minCostStCand[cvKind]->cost) > stCand->cost) {
						regist_f = true;
					}
				}
				// 距離差が閾値3未満→コストが閾値2未満で登録
				else if (RC_GET_SPLIT_LINK_DIST_RATIO3(dist_save[cvKind]) > dist_b) {
					if (RC_GET_SPLIT_LINK_COST_RATIO2(minCostStCand[cvKind]->cost) > stCand->cost) {
						regist_f = true;
					}
				}
				// 距離差が閾値3以上
				else {
					// 非採択
				}
				break;
			default:
				// 非採択
				break;
			}
		}
		// 登録処理
		if (regist_f) {
			minCostStCandIdx[cvKind] = aCandInfo->stLinkIdx + i;
			minCostStCand[cvKind] = stCand;
			dist_save[cvKind] = dist_b;
		}
		// 地図解放
		result = RC_FreeMapTbl(&readTab, true);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RC_FreeMapTbl error. [0x%08x] "HERE, result);
			break;
		}
	}
	// 地図解放
	if (0 < readTab.mapVol) {
		result = RC_FreeMapTbl(&readTab, true);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RC_FreeMapTbl error. [0x%08x] "HERE, result);
			*aStCandIndex = ALL_F32;
			return (result);
		}
	}

	// 最適リンク選択
	INT32 ans = -1;

	for (i = 2; i <= 2; i--) {
		if (NULL == minCostStCand[i]) {
			continue;
		}
		if (ans == -1) {
			ans = i;
			continue;
		}

		// 距離が短い→コストが閾値3未満で登録
		if (dist_save[ans] > dist_save[i]) {
			if (RC_GET_SPLIT_LINK_COST_RATIO3(minCostStCand[ans]->cost) > minCostStCand[i]->cost) {
				ans = i;
			}
		}
		// 距離差が閾値4未満→コストが閾値3未満で登録
		else if (RC_GET_SPLIT_LINK_DIST_RATIO4(dist_save[ans]) > dist_save[i]) {
			if (RC_GET_SPLIT_LINK_COST_RATIO4(minCostStCand[ans]->cost ) > minCostStCand[i]->cost) {
				ans = i;
			}
		}
	}
	if (ans == -1) {
		*aStCandIndex = ALL_F32;
	} else {
		// 結果格納
		*aStCandIndex = minCostStCandIdx[ans];
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief 候補リンクリスト作成処理
 * @param 区間管理
 * @param 候補開始リンク
 * @memo 探索情報テーブル候補経路から推奨経路を吸い出す。
 */
static E_SC_RESULT routeCandSelecter(SCRP_SECTCONTROLER* aSectCtrl, UINT32 aStCandIdx, RCRT_CAND_WKTBL* aCandWkTbl) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aSectCtrl) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SCRP_CANDDATA* candTop = aSectCtrl->candMng.cand;
	SCRP_CANDDATA* cand = aSectCtrl->candMng.cand + aStCandIdx;

	// 候補データ収集
	while (1) {
#if 0
		SC_LOG_InfoPrint(SC_TAG_RC, "pcl=0x%08x link=0x%08x cost=%6d ", cand->parcelId, cand->linkId, cand->cost);
#endif
		// サイズチェック
		if (aCandWkTbl->candSize <= aCandWkTbl->candCrnt) {
			result = mallocWorkCand(aCandWkTbl);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "mallocWorkCand error. "HERE);
				break;
			}
		}
		// 候補リスト使用位置取得
		SCRP_CANDDATA** targetCand = RCRT_GETMV_CRNTCAND(aCandWkTbl);
		// 候補リストへ候補アドレス格納
		*targetCand = cand;

		// 開始リンク
		if (RC_CAND_STLINK == cand->next) {
			SC_LOG_DebugPrint(SC_TAG_RC, "Route start link find. pcl=0x%08x link=0x%08x ", cand->parcelId, cand->linkId);
			break;
		}
		// 無効値
		if (RC_CAND_INIT == cand->next) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "next cand init error... pcl=0x%08x link=0x%08x ", cand->parcelId, cand->linkId);
			result = e_SC_RESULT_FAIL;
			break;
		}
		cand = candTop + cand->next;
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief ワーク領域の候補経路データから推奨経路基本情報を作成する
 * @param 区間管理
 * @param 推奨経路管理
 * @memo 候補経路は逆から追いかけてデータを作成している為、逆さに読み込み経路順に並び変える必要あり
 *       当関数処理後はワーク領域は開放してOK
 *       断裂は最後に候補開始リンクデータ元から判断しフラグを設定する。
 */
static E_SC_RESULT makeRoutePclLinkTbl(RCRT_CAND_WKTBL* aCandWkTbl, SC_RP_RouteMng* aRouteMng) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aCandWkTbl || NULL == aRouteMng) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (0 == aCandWkTbl->candCrnt || 0 == aCandWkTbl->candSize) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SCRP_CANDDATA** ppCand = NULL;
	//SC_RP_ParcelInfo* pclInfo = NULL;
	//SC_RP_LinkInfo* linkInfo = NULL;
	SCRP_LVCHANGE_TBL lvChange = {};
	UINT32 crntLinkIdx = 0;
	UINT32 crntPclIdx = 0;
	INT32 candIdx = 0;
	INT32 candIdxMark = 0;
	UINT32 prePclId = 0;
	UINT32 targetLinkVol = 0;
	UINT32 level = 0;
	UINT32 i;

	ppCand = aCandWkTbl->ppCandData;
	candIdx = aCandWkTbl->candCrnt - 1;
	candIdxMark = aCandWkTbl->candCrnt - 1;
	prePclId = (*(ppCand + candIdx))->parcelId;

	// 単純にパーセルとリンクの情報を候補経路から取得する
	while (0 <= candIdxMark) {
		if (0 < candIdx && prePclId == (*(ppCand + candIdx))->parcelId) {
			prePclId = (*(ppCand + candIdx))->parcelId;
			candIdx--;
			continue;
		} else if (0 == candIdx && prePclId == (*(ppCand + candIdx))->parcelId) {
			prePclId = (*(ppCand + candIdx))->parcelId;
			candIdx--;
		}

		// 処理対象数インデックスが0の場合は最終リンクを加算する
		if (0 > candIdx) {
			targetLinkVol = candIdxMark + 1;
		} else {
			targetLinkVol = candIdxMark - candIdx;
		}

		level = MESHC_GetLevel((*(ppCand + candIdxMark))->parcelId);
#if _RP_ROUTE_CONVERTLV
		if (RP_LEVEL1 == level) {
#else
		if (RP_LEVEL1 == level || RP_LEVEL2 == level) {
#endif
			// 推奨経路生成
			result = makeRoutePclLinkTblNormal(ppCand, candIdxMark, targetLinkVol, aRouteMng, &crntPclIdx, &crntLinkIdx);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "makeRoutePclLinkTblNormal error. [0x%08x] "HERE, result);
				break;
			}
		} else if (RP_LEVEL2 == level) {
			// リンクリストレベル変換
			lvChange.parcelId = (*(ppCand + candIdxMark))->parcelId;
			lvChange.linkIdVol = targetLinkVol;

			/// 領域確保
			lvChange.linkId = RP_MemAlloc(sizeof(UINT32) * targetLinkVol, e_MEM_TYPE_ROUTEPLAN);
			if (NULL == lvChange.linkId) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error. "HERE);
				result = e_SC_RESULT_MALLOC_ERR;
				break;
			}

			for (i = 0; i < targetLinkVol; i++) {
				SCRP_CANDDATA* wkCand = *(ppCand + candIdxMark - i);
				// リンクIDと方向を格納
				lvChange.linkId[i] = SC_MA_D_NWID_GET_PNT_ID(wkCand->linkId) & 0xF9FFFFFF;
				if (SCRP_LINKODR == RCND_GET_ORIDX(wkCand->flag)) {
					lvChange.linkId[i] |= 0x02000000;
				} else {
					lvChange.linkId[i] |= 0x04000000;
				}
			}
			E_SC_RESULT cnvResult = RP_LinkLevelConvert(&lvChange, e_MEM_TYPE_ROUTEPLAN);
			if (e_SC_RESULT_SUCCESS == cnvResult) {
				result = makeRoutePclLinkTblForLvConvert(&lvChange, aRouteMng, &crntPclIdx, &crntLinkIdx);
				if (e_SC_RESULT_SUCCESS != result) {
					SC_LOG_ErrorPrint(SC_TAG_RC, "makeRoutePclLinkTblForLvConvert error. [0x%08x] "HERE, result);
					break;
				}
			} else {
				// レベル変換失敗時にはレベル２のリンクをそのまま格納する
				SC_LOG_InfoPrint(SC_TAG_RC, "RP_LinkLevelConvert error. make lv2Link route. [0x%08x] "HERE, cnvResult);
				// 推奨経路生成(Lv2)
				result = makeRoutePclLinkTblNormal(ppCand, candIdxMark, targetLinkVol, aRouteMng, &crntPclIdx, &crntLinkIdx);
				if (e_SC_RESULT_SUCCESS != result) {
					SC_LOG_ErrorPrint(SC_TAG_RC, "makeRoutePclLinkTblNormal error. [0x%08x] "HERE, result);
					break;
				}
			}
			// メモリ解放
			RP_LinkLevelConvertTblFree(&lvChange, e_MEM_TYPE_ROUTEPLAN);
		} else {
			result = e_SC_RESULT_FAIL;
			break;
		}

		if (0 > candIdx) {
			break;
		}
		// 最後にインデックスマーク
		candIdxMark = candIdx;
		prePclId = (*(ppCand + candIdx))->parcelId;
	}
	// メモリ解放
	RP_LinkLevelConvertTblFree(&lvChange, e_MEM_TYPE_ROUTEPLAN);

	// 確保領域トリム
	do {
		// リンク
		if (NULL != aRouteMng->linkInfo) {
			SC_RP_LinkInfo* lInfo = RP_MemAlloc(sizeof(SC_RP_LinkInfo) * crntLinkIdx, e_MEM_TYPE_ROUTEMNG);
			if (NULL == lInfo) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error. size=%d "HERE, sizeof(SC_RP_LinkInfo) * crntLinkIdx);
				result = e_SC_RESULT_MALLOC_ERR;
				break;
			}
			RP_Memset0(lInfo, sizeof(SC_RP_LinkInfo) * crntLinkIdx);
			if (0 < aRouteMng->linkVol) {
				RP_Memcpy(lInfo, aRouteMng->linkInfo, sizeof(SC_RP_LinkInfo) * crntLinkIdx);
				RP_MemFree(aRouteMng->linkInfo, e_MEM_TYPE_ROUTEMNG);
			}
			aRouteMng->linkInfo = lInfo;
			aRouteMng->linkVol = crntLinkIdx;
		}
		// パーセル
		if (NULL != aRouteMng->parcelInfo) {
			SC_RP_ParcelInfo* pInfo = RP_MemAlloc(sizeof(SC_RP_ParcelInfo) * crntPclIdx, e_MEM_TYPE_ROUTEMNG);
			if (NULL == pInfo) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error. vol=%d "HERE, sizeof(SC_RP_ParcelInfo) * crntPclIdx);
				result = e_SC_RESULT_MALLOC_ERR;
				break;
			}
			RP_Memset0(pInfo, sizeof(SC_RP_ParcelInfo) * crntPclIdx);
			if (0 < aRouteMng->parcelVol) {
				RP_Memcpy(pInfo, aRouteMng->parcelInfo, sizeof(SC_RP_ParcelInfo) * crntPclIdx);
				RP_MemFree(aRouteMng->parcelInfo, e_MEM_TYPE_ROUTEMNG);
			}
			aRouteMng->parcelInfo = pInfo;
			aRouteMng->parcelVol = crntPclIdx;
		}
		// 経路開始リンクの情報元が近傍以外の場合
		if (!aCandWkTbl->startIsNeigbor && 0 < aRouteMng->linkVol) {
			(aRouteMng->linkInfo + aRouteMng->linkVol - 1)->splitFlag = SC_RP_SPLIT_LINK;
		}
	} while (0);
#if 0
	{
		SC_LOG_InfoPrint(SC_TAG_RC, "coverted pclVol=%d linkVol=%d ", aRouteMng->parcelVol, aRouteMng->linkVol);
		for(i = 0; i < aRouteMng->parcelVol; i++) {
			SC_LOG_InfoPrint(SC_TAG_RC, "      pcl=0x%08x linkIdx=%3d linkVol=%4d", (aRouteMng->parcelInfo + i)->parcelId, (aRouteMng->parcelInfo + i)->linkIdx, (aRouteMng->parcelInfo + i)->linkVol);
		}
		for(i = 0; i < aRouteMng->linkVol; i++) {
			SC_LOG_InfoPrint(SC_TAG_RC, "      link=0x%08x or=%2d kind=%2d", (aRouteMng->linkInfo + i)->linkId, (aRouteMng->linkInfo + i)->orFlag, (aRouteMng->linkInfo + i)->linkKind);
		}
	}
#endif
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief 推奨経路作成
 * @param 候補ワークテーブル
 * @param 候補インデックス
 * @param リンク数
 * @param 経路管理
 * @param パーセルカレント位置
 * @param リンクカレント位置
 * @memo 断裂フラグは候補経路から継承しない
 */
static E_SC_RESULT makeRoutePclLinkTblNormal(SCRP_CANDDATA** ppCand, INT32 candIdxMark, UINT32 targetLinkVol, SC_RP_RouteMng* aRouteMng,
		UINT32* aPclCrnt, UINT32* aLinkCrnt) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == ppCand || NULL == aRouteMng || NULL == aPclCrnt || NULL == aLinkCrnt) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	UINT32 i;
	UINT32 crntPclIdx = *aPclCrnt;
	UINT32 crntLinkIdx = *aLinkCrnt;
	SC_RP_ParcelInfo* pclInfo = NULL;
	SC_RP_LinkInfo* linkInfo = NULL;

	do {
		// size check
		if (aRouteMng->parcelVol < (crntPclIdx + 1)) {
			result = mallocRoutePclInfo(aRouteMng, RP_ROUTEPCL_BLOCK_SIZE);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "mallocRoutePclInfo error. [0x%08x] "HERE, result);
				break;
			}
		}
		// size check
		if (aRouteMng->linkVol < (crntLinkIdx + targetLinkVol)) {
			result = mallocRouteLinkInfo(aRouteMng, RP_ROUTELINK_BLOCK_SIZE);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "mallocRouteLinkInfo error. [0x%08x] "HERE, result);
				break;
			}
		}

		// パーセル情報作成
		pclInfo = aRouteMng->parcelInfo + crntPclIdx;
		pclInfo->parcelId = (*(ppCand + candIdxMark))->parcelId;
		pclInfo->linkIdx = crntLinkIdx;
		pclInfo->linkVol = targetLinkVol;
		pclInfo->level = MESHC_GetLevel((*(ppCand + candIdxMark))->parcelId);
		// リンク情報作成
		for (i = 0; i < targetLinkVol; i++) {
			SCRP_CANDDATA* wkCand = *(ppCand + candIdxMark - i);
			linkInfo = aRouteMng->linkInfo + crntLinkIdx;
			linkInfo->linkId = wkCand->linkId;
			/* ▼注意▼  地図データ取得の為に一時的に格納（本来は推奨経路テーブルのオフセット） */
			RCRT_SET_FORMOFS_FORROUTE(linkInfo, wkCand->formOfs);
			linkInfo->orFlag = RCND_GET_ORIDX(wkCand->flag);
			linkInfo->termFlag = SC_RP_TERM_IS_MID;
			linkInfo->level = pclInfo->level;
			linkInfo->splitFlag = 0;
			linkInfo->travelTime = 0;
			crntLinkIdx++;
		}
		crntPclIdx++;
	} while (0);

	if (e_SC_RESULT_SUCCESS == result) {
		*aPclCrnt = crntPclIdx;
		*aLinkCrnt = crntLinkIdx;
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief レベル変換テーブルから推奨経路リンク生成
 * @param レベル変換テーブル
 * @param 経路管理
 * @param パーセルカレント位置
 * @param リンクカレント位置
 * @memo 断裂は候補から継承しない
 */
static E_SC_RESULT makeRoutePclLinkTblForLvConvert(SCRP_LVCHANGE_TBL* aLvChange, SC_RP_RouteMng* aRouteMng, UINT32* aPclCrnt,
		UINT32* aLinkCrnt) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aLvChange || NULL == aRouteMng || NULL == aPclCrnt || NULL == aLinkCrnt) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SC_RP_ParcelInfo* pclInfo = NULL;
	SC_RP_ParcelInfo* prePclInfo = NULL;
	SC_RP_LinkInfo* linkInfo = NULL;
	UINT32 linkCrnt = *aLinkCrnt;
	UINT32 pclCrnt = *aPclCrnt;
	UINT32 pclVol = 0;
	UINT32 parcelId = 0;
	UINT32 i;
	//UINT32 e;

	do {
		// パーセル数を算出する
		for (i = 0; i < aLvChange->resLinkInfoVol; i++) {
			if (parcelId != (aLvChange->resLinkInfo + i)->parcelId) {
				parcelId = (aLvChange->resLinkInfo + i)->parcelId;
				pclVol++;
			}
		}
		// size check
		if (aRouteMng->linkVol < (linkCrnt + aLvChange->resLinkInfoVol)) {
			result = mallocRouteLinkInfo(aRouteMng, RP_ROUTELINK_BLOCK_SIZE);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "mallocRouteLinkInfo error. [0x%08x] "HERE, result);
				break;
			}
		}
		// size check
		if (aRouteMng->parcelVol < (pclCrnt + pclVol)) {
			result = mallocRoutePclInfo(aRouteMng, RP_ROUTEPCL_BLOCK_SIZE);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "mallocRoutePclInfo error. [0x%08x] "HERE, result);
				break;
			}
		}
		// レベル変換テーブルからリンク情報及びパーセル情報を構築する
		parcelId = 0;
		for (i = 0; i < aLvChange->resLinkInfoVol; i++) {
			SCRP_LVCHANGE_RES* lvChangeLink = aLvChange->resLinkInfo + i;

			// パーセル情報作成
			if (parcelId != lvChangeLink->parcelId) {
				parcelId = lvChangeLink->parcelId;
				pclInfo = aRouteMng->parcelInfo + pclCrnt;
				pclInfo->parcelId = lvChangeLink->parcelId;
				pclInfo->linkIdx = linkCrnt;
				pclInfo->linkVol = 0;
				pclInfo->level = RP_LEVEL1;
				prePclInfo = pclInfo;
				pclCrnt++;
			}

			// リンク情報作成
			linkInfo = aRouteMng->linkInfo + linkCrnt;
			linkInfo->linkId = lvChangeLink->linkId & 0xF9FFFFFF;
			/* ▼注意▼  地図データ取得の為に一時的に格納（本来は推奨経路テーブルのオフセット） */
			RCRT_SET_FORMOFS_FORROUTE(linkInfo, lvChangeLink->formOfs);
			if (1 == SC_MA_D_NWID_GET_SUB_CNCTSIDE(lvChangeLink->linkId)) {
				linkInfo->orFlag = SC_RP_DIR_IS_ODR;
			} else {
				linkInfo->orFlag = SC_RP_DIR_IS_RVS;
			}
			linkInfo->termFlag = SC_RP_TERM_IS_MID;
			linkInfo->level = prePclInfo->level;
			linkInfo->splitFlag = 0;
			linkInfo->travelTime = 0;
			prePclInfo->linkVol++;
			linkCrnt++;
		}
	} while (0);

	*aLinkCrnt = linkCrnt;
	*aPclCrnt = pclCrnt;

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief 形状データ設定
 * @param [I]探索情報
 * @param [I/O]設定対象経路管理
 * @memo 区間始終端の形状データの編集を行う際にリンク情報も編集を行う
 */
static E_SC_RESULT makeRouteFormTbl(SCRP_SECTCONTROLER* aSectCtrl, SC_RP_RouteMng* aRouteMng) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;

	RCRT_FORM_WKTBL formWkTbl = {};
	SCRP_MAPREADTBL mapTab = {};
	SC_RP_ParcelInfo* pclInfo = NULL;
	SC_RP_LinkInfo* linkInfo = NULL;
	SC_RP_FormInfo* formInfo = NULL;
	UINT32 formVolCnt = 0;
	MAL_HDL pShapeRec = NULL;
	UINT32 mapReqKind = 0;
	UINT32 i, e, u;

	// 地図要求種別設定
	mapReqKind = SC_DHC_GetKindMask(e_DATA_KIND_SHAPE);
	mapReqKind |= SC_DHC_GetKindMask(e_DATA_KIND_ROAD);

	if (NULL == aRouteMng || NULL == aSectCtrl) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 始終端同一リンクはエラー扱い
	if (1 == aRouteMng->parcelVol && 1 == aRouteMng->parcelInfo->linkVol) {
		// 距離が近ければ近距離エラーコード発行
		if (checkSectPointNear(aSectCtrl)) {
			RPC_SetErrorCode(EC_SAME_ROAD);
		}
		SC_LOG_ErrorPrint(SC_TAG_RC, "term link is same... "HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	do {
		// Parcel別処理
		for (i = 0; i < aRouteMng->parcelVol; i++) {
			pclInfo = aRouteMng->parcelInfo + i;
			pclInfo->subLinkIdx = formWkTbl.formCrnt;

			// 地図読み込み
			result = RC_ReadListMap(&pclInfo->parcelId, 1, mapReqKind, &mapTab);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "RC_ReadListMap error. [0x%08x] "HERE, result);
				break;
			}
			if (NULL == mapTab.mapList->shape) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "RC_ReadListMap error. [0x%08x] "HERE, result);
				result = e_SC_RESULT_FAIL;
				break;
			}

			// 形状データ先頭取得
			pShapeRec = SC_MA_GetMapSharpRecord(mapTab.mapList->shape);
			if (NULL == pShapeRec) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "SC_MA_GetMapSharpRecord error. pcl=0x%08x "HERE, mapTab.mapList->parcelId);
				result = e_SC_RESULT_FAIL;
				break;
			}

			// Link別処理
			formVolCnt = 0;
			for (e = 0; e < pclInfo->linkVol; e++) {
				linkInfo = aRouteMng->linkInfo + pclInfo->linkIdx + e;

				// 復元したオフセットから形状取得
				UINT32 formIdx = RCRT_GET_FORMOFS_FROMROUTE(linkInfo);
				T_MapShapeRecord* pShape = (T_MapShapeRecord*) SC_MA_A_SHRCD_GET_RECORD(pShapeRec, formIdx);

				/* リンク情報格納：形状インデックス，形状数 */
				linkInfo->formIdx = formWkTbl.formCrnt;
				linkInfo->formVol = pShape->pointVol;

				/* リンク情報格納：リンク長，旅行時間，種別 */
				linkInfo->dist = SC_MA_GET_LINK_DIST(SC_MA_A_SHRCD_GET_LINKDIST((MAL_HDL) pShape));
				linkInfo->linkKind = pShape->linkBaseInfo.b_code.linkKind1;
				linkInfo->roadKind = pShape->linkBaseInfo.b_code.roadKind;
				result = getTravelTime(mapTab.mapList, pShape->linkId, &linkInfo->travelTime);
				if (e_SC_RESULT_SUCCESS != result) {
					// 静的旅行時間を設定する
					linkInfo->travelTime = RCRT_GET_SIMPLETRAVELTIME(linkInfo);
				}

				// 形状テーブルサイズ確認
				if (formWkTbl.formSize <= (formWkTbl.formCrnt + linkInfo->formVol)) {
					result = mallocWorkFormInfo(&formWkTbl);
					if (e_SC_RESULT_SUCCESS != result) {
						SC_LOG_ErrorPrint(SC_TAG_RC, "mallocFormInfo error. [0x%08x] "HERE, result);
						break;
					}
				}
				/* 形状格納 */
				MAL_HDL shape = SC_MA_A_SHRCD_GET_XY((MAL_HDL)pShape);
				move4byte(shape);
				for (u = 0; u < linkInfo->formVol; u++) {
					formInfo = RCRT_GET_WKFORM2NEXT(formWkTbl);
					formInfo->x = read2byte(shape);
					move2byte(shape);
					formInfo->y = read2byte(shape);
					move2byte(shape);
				}

				formVolCnt += linkInfo->formVol;
			}
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "error. "HERE);
				break;
			}
			// 累計数格納
			pclInfo->subLinkVol = formVolCnt;

			// 地図開放
			result = RC_FreeMapTbl(&mapTab, true);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "RC_FreeMapTbl error. "HERE);
			}
		}
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "error. "HERE);
			if (e_SC_RESULT_SUCCESS != RC_FreeMapTbl(&mapTab, true)) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "RC_FreeMapTbl error. "HERE);
			}
			break;
		}

		/* 開始終了リンクの設定 */
		result = makeRouteTarmLinkInfo(aSectCtrl, aRouteMng, &formWkTbl);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "makeRouteTarmLinkInfo error. [0x%08x] "HERE, result);
			break;
		}

		// 形状データを経路メモリへコピー
		result = mallocFormInfo(aRouteMng, formWkTbl.formCrnt);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "mallocFormInfo error. [0x%08x] "HERE, result);
			break;
		}
		RP_Memcpy(aRouteMng->formInfo, formWkTbl.wkFormInfo, sizeof(SC_RP_FormInfo) * formWkTbl.formCrnt);

	} while (0);

	// 形状作業領域解放
	if (NULL != formWkTbl.wkFormInfo) {
		RP_MemFree(formWkTbl.wkFormInfo, e_MEM_TYPE_ROUTEPLAN);
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief 開始終了リンクに関連する情報を作成する
 * @param 探索管理
 * @param 区間番号
 * @param 推奨経路管理
 * @param 形状作業テーブル
 */
static E_SC_RESULT makeRouteTarmLinkInfo(SCRP_SECTCONTROLER* aSectCtrl, SC_RP_RouteMng* aRouteMng, RCRT_FORM_WKTBL* aFormWkTbl) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aSectCtrl || NULL == aRouteMng || NULL == aFormWkTbl) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SC_RP_ParcelInfo* stPclInfo = NULL;
	SC_RP_ParcelInfo* edPclInfo = NULL;
	SC_RP_LinkInfo* stLinkInfo = NULL;
	SC_RP_LinkInfo* edLinkInfo = NULL;
	SC_RP_LinkInfo* wkLinkInfo = NULL;
	//SC_RP_FormInfo* formInfo = NULL;
	SCRP_NEIGHBORINFO* neigborInfo = NULL;
	RCRT_TERMINFO term = {};
	Bool startFlg = false;
	UINT32 parcelId = 0;
	UINT32 i;

	do {
		// 念のためリンク数チェック
		if (2 > aRouteMng->linkVol) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
			result = e_SC_RESULT_BADPARAM;
			break;
		}

		// 開始リンク取得
		stPclInfo = aRouteMng->parcelInfo;
		edPclInfo = aRouteMng->parcelInfo + aRouteMng->parcelVol - 1;
		stLinkInfo = aRouteMng->linkInfo;
		edLinkInfo = aRouteMng->linkInfo + aRouteMng->linkVol - 1;

		// フラグを設定
		stLinkInfo->termFlag = SC_RP_TERM_IS_FIRST;
		edLinkInfo->termFlag = SC_RP_TERM_IS_LAST;

		for (i = 0; i < 2; i++) {
			if (i == 0) {
				neigborInfo = &aSectCtrl->neighbor[SCRP_NBROWN];
				wkLinkInfo = stLinkInfo;
				parcelId = stPclInfo->parcelId;
				startFlg = true;
			} else {
				neigborInfo = &aSectCtrl->neighbor[SCRP_NBRDST];
				wkLinkInfo = edLinkInfo;
				parcelId = edPclInfo->parcelId;
				startFlg = false;
			}
			// 近傍情報から近傍形状等収集(始点or終点)
			result = getTermNbrLinkInfo(neigborInfo, parcelId, wkLinkInfo, startFlg, &term);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "getTermNbrLinkInfo error. [0x%08x] "HERE, result);
				break;
			}

			// 編集インデックス無効値は編集しない
			if (-1 != term.editIdx) {
#if 0
				SC_LOG_InfoPrint(SC_TAG_RC, "Befor link=0x%08x xy=%4d,%4d fvol=%4d fidx=%4d time=%4d dist=%4d",
						wkLinkInfo->linkId,
						(aFormWkTbl->wkFormInfo + wkLinkInfo->formIdx + term.editIdx)->x,
						(aFormWkTbl->wkFormInfo + wkLinkInfo->formIdx + term.editIdx)->y,
						wkLinkInfo->formVol,
						wkLinkInfo->formIdx,
						wkLinkInfo->travelTime,
						wkLinkInfo->dist
				);
#endif
				// リンク情報書き換え
				wkLinkInfo->formVol = term.formVol;
				wkLinkInfo->formIdx = wkLinkInfo->formIdx + term.stSubIdx;
				wkLinkInfo->travelTime *= ((DOUBLE) term.fillDist / (DOUBLE) term.linkDist);
				wkLinkInfo->dist = term.fillDist;
				// 形状点情報書き換え
				(aFormWkTbl->wkFormInfo + wkLinkInfo->formIdx + term.editIdx)->x = term.vX;
				(aFormWkTbl->wkFormInfo + wkLinkInfo->formIdx + term.editIdx)->y = term.vY;
#if 0
				SC_LOG_InfoPrint(SC_TAG_RC, "After link=0x%08x xy=%4d,%4d fvol=%4d fidx=%4d time=%4d dist=%4d",
						wkLinkInfo->linkId,
						(aFormWkTbl->wkFormInfo + wkLinkInfo->formIdx + term.editIdx)->x,
						(aFormWkTbl->wkFormInfo + wkLinkInfo->formIdx + term.editIdx)->y,
						wkLinkInfo->formVol,
						wkLinkInfo->formIdx,
						wkLinkInfo->travelTime,
						wkLinkInfo->dist
				);
#endif
			}
		}
		// TODO 形状テーブルのトリム
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief 近傍情報から始端終端情報を生成する
 * @param 近傍情報
 * @param パーセルID
 * @param リンク情報（生成済み推奨経路）
 * @param true：始端リンク false：終端リンク
 * @param [O]始端終端情報
 * @memo 始終端のデータ取得まで行いリンク情報、形状情報の編集は後続処理にて実施
 */
static E_SC_RESULT getTermNbrLinkInfo(SCRP_NEIGHBORINFO* aNeigbor, UINT32 aParcelId, SC_RP_LinkInfo* aLinkInfo, Bool aIsStart,
		RCRT_TERMINFO* aTermInfo) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SCRP_NEIGHBORLINK* neigbor = NULL;
	Bool trendOdr = true;
	UINT32 i;

	if (NULL == aNeigbor || NULL == aLinkInfo || NULL == aTermInfo) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 初期化
	RP_Memset0(aTermInfo, sizeof(RCRT_TERMINFO));

	do {
		// 断裂は固定値格納
		if (!aIsStart && SC_RP_SPLIT_LINK == aLinkInfo->splitFlag) {
			SC_LOG_WarnPrint(SC_TAG_RC, "link is split... "HERE);
			result = e_SC_RESULT_FAIL;
			break;
		}
		// 一致する近傍情報を検索
		for (i = 0; i < aNeigbor->nbrLinkVol; i++) {
			if (aParcelId != (aNeigbor->neighborLink + i)->parcelId) {
				continue;
			}
			if (aLinkInfo->linkId != (aNeigbor->neighborLink + i)->linkId) {
				continue;
			}
			if (aLinkInfo->orFlag != (aNeigbor->neighborLink + i)->orFlag) {
				continue;
			}
			neigbor = aNeigbor->neighborLink + i;
			break;
		}
		if (NULL == neigbor) {
			SC_LOG_WarnPrint(SC_TAG_RC, "neigbor not found... "HERE);
			result = e_SC_RESULT_FAIL;
			break;
		}

		// 編集インデックス・開始インデックス 設定
		if (aIsStart) {
			if (SCRP_LINKODR == aLinkInfo->orFlag) {
				trendOdr = false;
			} else {
				trendOdr = true;
			}
		} else {
			if (SCRP_LINKODR == aLinkInfo->orFlag) {
				trendOdr = true;
			} else {
				trendOdr = false;
			}
		}

#if 0
		SC_LOG_InfoPrint(SC_TAG_RC, "nbr dist=%4d remain=%5f subremain=%5f ratio=%5f or=%d"
				,neigbor->linkDist
				,neigbor->remainDist
				,neigbor->subRemainDist
				,neigbor->ratio
				,trendOdr
		);
#endif
		// 塗りつぶし長・形状数 設定
		if (trendOdr) {
			// 塗りつぶしリンク中間開始
			aTermInfo->fillDist = (UINT32) lround(neigbor->remainDist + neigbor->subRemainDist);
			aTermInfo->formVol = neigbor->subIndex + 1;
			aTermInfo->editIdx = aTermInfo->formVol - 1;
			aTermInfo->formVolDiff = neigbor->subVol - aTermInfo->formVol;
			aTermInfo->stSubIdx = 0;
			if (2 < aTermInfo->formVol && CompareDouble((DOUBLE) 0., neigbor->ratio)) {
				aTermInfo->formVol -= 1;
				aTermInfo->editIdx -= 1;
			}
		} else {
			// 塗りつぶしリンク始端開始
			aTermInfo->fillDist = neigbor->linkDist - (UINT32) lround(neigbor->remainDist + neigbor->subRemainDist);
			aTermInfo->formVol = (aLinkInfo->formVol - (neigbor->subIndex - 1));
			aTermInfo->editIdx = 0;
			aTermInfo->formVolDiff = aLinkInfo->formVol - aTermInfo->formVol;
			aTermInfo->stSubIdx = neigbor->subIndex - 1;
			if ((2 < aTermInfo->formVol) && (CompareDouble((DOUBLE) 1., neigbor->ratio))) {
				aTermInfo->formVol -= 1;
				aTermInfo->stSubIdx += 1;
			}
		}
		// リンク長 設定
		aTermInfo->linkDist = neigbor->linkDist;
		aTermInfo->vX = neigbor->x;
		aTermInfo->vY = neigbor->y;

	} while (0);

	if (e_SC_RESULT_SUCCESS != result) {
		aTermInfo->formVolDiff = 0;
		aTermInfo->formVol = aLinkInfo->formVol;
		aTermInfo->fillDist = aLinkInfo->dist;
		aTermInfo->linkDist = aLinkInfo->dist;
		aTermInfo->stSubIdx = 0;
		aTermInfo->editIdx = -1;
		aTermInfo->vX = 0;
		aTermInfo->vY = 0;
	}
#if 0
	SC_LOG_InfoPrint(SC_TAG_RC, "Term xy=%4d,%4d editIdx=%2d stSubIdx=%2d formVol=%2d formVolDiff=%2d fill=%4d dist=%4d"
			,aTermInfo->vX
			,aTermInfo->vY
			,aTermInfo->editIdx
			,aTermInfo->stSubIdx
			,aTermInfo->formVol
			,aTermInfo->formVolDiff
			,aTermInfo->fillDist
			,aTermInfo->linkDist
	);
#endif

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 断裂経路トリム
 * @param 区間管理
 * @param 経路管理
 * @memo 経路にレベル２が含まれる場合or経路が断裂している場合、
 *       最近傍リンクを最終リンクとして以降の経路情報を削除する。
 *       最終リンクにはSPLITフラグを立てる。
 */
static E_SC_RESULT splitRouteTrim(SCRP_SECTCONTROLER* aSectCtrl, SC_RP_RouteMng* aRouteMng) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aSectCtrl || NULL == aRouteMng) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SCRP_POINT* point = &aSectCtrl->neighbor[1].point;
	SC_RP_LinkInfo* linkInfo = NULL;
	SC_RP_ParcelInfo* pclInfo = NULL;
	UINT32 lv2LinkIdx = ALL_F32;
	UINT32 lv2PclIdx = ALL_F32;
	Bool split_f = false;
	Bool level2_f = false;
	UINT32 i, e;
	UINT32 latestPclVol = aRouteMng->parcelVol;
	UINT32 latestLinkVol = aRouteMng->linkVol;

	do {
		pclInfo = aRouteMng->parcelInfo;
		for (i = 0; i < aRouteMng->parcelVol; i++, pclInfo++) {
			linkInfo = aRouteMng->linkInfo + pclInfo->linkIdx;
			for (e = 0; e < pclInfo->linkVol; e++, linkInfo++) {
				if (SC_RP_SPLIT_LINK == linkInfo->splitFlag) {
					if (i == aRouteMng->parcelVol - 1 && e == pclInfo->linkVol - 1) {
						// 断裂が最終リンクである
						split_f = true;
					} else {
						// 断裂が中間リンクである
						result = e_SC_RESULT_FAIL;
						break;
					}
				}
				if (!level2_f && RP_LEVEL2 == linkInfo->level) {
					// 初レベル２情報
					level2_f = true;
					lv2PclIdx = i;
					lv2LinkIdx = pclInfo->linkIdx + e;
				}
			}
			if (e_SC_RESULT_SUCCESS != result) {
				break;
			}
		}
		if (e_SC_RESULT_SUCCESS != result) {
			break;
		}

		UINT32 vPclIdx = 0, vLinkIdx = 0, vFormIdx = 0;
		if (split_f) {
			// 断裂経路時には断裂リンク位置を調整する
			result = RC_FindMostNearRoute(aRouteMng, point, &vPclIdx, &vLinkIdx, &vFormIdx);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "RC_FindMostNearRoute error. [0x%08x] "HERE, result);
				break;
			}
			// 開始リンク0は許容しない（そのままの経路とする）
			if (0 == vLinkIdx) {
				break;
			}
		}
#if _RP_ROUTE_LV2ROUTESTRIP
		/* 断裂リンク以降のリンクを切り捨てる。切り捨てる場合末尾リンクはSplitを立てる。 */
		// 経路末尾を特定
		if (split_f) {
			if (level2_f) {
				if (vLinkIdx < lv2LinkIdx) {
					latestPclVol = vPclIdx + 1;
					latestLinkVol = vLinkIdx + 1;
				} else {
					latestPclVol = lv2PclIdx;
					latestLinkVol = lv2LinkIdx;
				}
			} else {
				latestPclVol = vPclIdx + 1;
				latestLinkVol = vLinkIdx + 1;
			}
		} else if (level2_f) {
			latestPclVol = lv2PclIdx;
			latestLinkVol = lv2LinkIdx;
		} else {
			// NOP
			break;
		}
		// 不要経路の切り捨て
		result = routeTblStrip(latestPclVol, latestLinkVol, aRouteMng);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "routeTblStrip error. [0x%08x] "HERE, result);
			break;
		}
		// 最終リンクへ断裂フラグを立てる
		linkInfo = aRouteMng->linkInfo + aRouteMng->linkVol - 1;
		linkInfo->splitFlag = SC_RP_SPLIT_LINK;
#else
		// 断裂フラグ設定
		if (split_f) {
			// レベル２よりも前に短い垂線長のリンクがある場合断裂フラグを立てる
			if (level2_f) {
				if (vLinkIdx < lv2LinkIdx) {
					(aRouteMng->linkInfo + vLinkIdx)->splitFlag = SC_RP_SPLIT_LINK;
				}
			} else {
				(aRouteMng->linkInfo + vLinkIdx)->splitFlag = SC_RP_SPLIT_LINK;
			}
		}
#endif
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief 特定のサイズへ経路情報をカットする
 * @param 残すパーセルサイズ
 * @param 残すリンクサイズ
 * @memo レベル２や断裂リンク位置の調整により不要になる経路情報を削除するために使用する。
 *       あえてレベル２や断裂リンクの不要情報を残しておくべき場合は使用しない。
 */
static E_SC_RESULT routeTblStrip(UINT32 aRemParcelVol, UINT32 aRemLinkVol, SC_RP_RouteMng* aRouteMng) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aRouteMng) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;

	do {
		// 残すパーセル数が現状のパーセル数収まっていることをチェック
		if (aRouteMng->parcelVol < aRemParcelVol) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
			result = e_SC_RESULT_BADPARAM;
			break;
		}
		// 残す最終パーセルが残すリンク数内に収まっていることをチェック
		SC_RP_ParcelInfo* pclInfo = aRouteMng->parcelInfo + aRemParcelVol - 1;
		if ((pclInfo->linkIdx + pclInfo->linkVol) < aRemLinkVol) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
			result = e_SC_RESULT_BADPARAM;
			break;
		}

		// パーセル処理
		pclInfo = RP_MemAlloc(sizeof(SC_RP_ParcelInfo) * aRemParcelVol, e_MEM_TYPE_ROUTEMNG);
		if (NULL == pclInfo) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error. vol=%d "HERE, sizeof(SC_RP_ParcelInfo) * aRemParcelVol);
			result = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		RP_Memset0(pclInfo, sizeof(SC_RP_ParcelInfo) * aRemParcelVol);
		if (0 < aRouteMng->parcelVol) {
			RP_Memcpy(pclInfo, aRouteMng->parcelInfo, sizeof(SC_RP_ParcelInfo) * aRemParcelVol);
			RP_MemFree(aRouteMng->parcelInfo, e_MEM_TYPE_ROUTEMNG);
		}
		aRouteMng->parcelInfo = pclInfo;
		aRouteMng->parcelVol = aRemParcelVol;

		// リンク処理
		SC_RP_LinkInfo* linkInfo = RP_MemAlloc(sizeof(SC_RP_LinkInfo) * aRemLinkVol, e_MEM_TYPE_ROUTEMNG);
		if (NULL == linkInfo) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error. size=%d "HERE, sizeof(SC_RP_LinkInfo) * aRemLinkVol);
			result = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		RP_Memset0(linkInfo, sizeof(SC_RP_LinkInfo) * aRemLinkVol);
		if (0 < aRouteMng->linkVol) {
			RP_Memcpy(linkInfo, aRouteMng->linkInfo, sizeof(SC_RP_LinkInfo) * aRemLinkVol);
			RP_MemFree(aRouteMng->linkInfo, e_MEM_TYPE_ROUTEMNG);
		}
		aRouteMng->linkInfo = linkInfo;
		aRouteMng->linkVol = aRemLinkVol;

		// 形状処理
		SC_RP_LinkInfo* lastLinkInfo = aRouteMng->linkInfo + aRouteMng->linkVol - 1;
		UINT32 remFormVol = lastLinkInfo->formIdx + lastLinkInfo->formVol;
		SC_RP_FormInfo* formInfo = RP_MemAlloc(sizeof(SC_RP_FormInfo) * remFormVol, e_MEM_TYPE_ROUTEMNG);
		if (NULL == formInfo) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error. size=%d "HERE, sizeof(SC_RP_FormInfo) * remFormVol);
			result = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		RP_Memset0(formInfo, sizeof(SC_RP_FormInfo) * remFormVol);
		if (0 < aRouteMng->formVol) {
			RP_Memcpy(formInfo, aRouteMng->formInfo, sizeof(SC_RP_FormInfo) * remFormVol);
			RP_MemFree(aRouteMng->formInfo, e_MEM_TYPE_ROUTEMNG);
		}
		aRouteMng->formInfo = formInfo;
		aRouteMng->formVol = remFormVol;

		// 最終パーセルのリンク数等を再書き換え
		pclInfo = aRouteMng->parcelInfo + aRouteMng->parcelVol - 1;
		pclInfo->linkVol = aRouteMng->linkVol - pclInfo->linkIdx;
		pclInfo->subLinkVol = aRouteMng->formVol - pclInfo->subLinkIdx;
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief 区間データをマスターへ結合
 * @param 新区間分の経路管理
 * @param 区間数
 * @param 区間管理
 * @memo 結合後のデータサイズが無い場合は何もしない
 */
static E_SC_RESULT jointRouteManager(SC_RP_RouteMng* aNewSectRouteMng, UINT8 aSectVol, SCRP_SECTCONTROLER* aSectCtrl) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	UINT32 newSize = 0;
	SC_RP_SectInfo* preSect = NULL;
	SC_RP_SectInfo* dstSect = NULL;
	SC_RP_ParcelInfo* newPclInfo = NULL;
	SC_RP_LinkInfo* newLinkInfo = NULL;
	SC_RP_FormInfo* newFormInfo = NULL;
	SC_RP_RegInfo* newRegInfo = NULL;
	RCRT_SAVEROUTEINFO* saveRoute = RCRT_GET_SAVEROUTEINFO();
	SC_RP_RouteMng* mstMng = &saveRoute->routeMng;

	if (NULL == aSectCtrl) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	do {
		// 登録対象区間チェック
		if (mstMng->sectVol <= saveRoute->sectCurrent) {
			// 区間登録済み数超え
			SC_LOG_ErrorPrint(SC_TAG_RC, "RtSect save count over. sectVol[%d] settingCnt[%d] "HERE, mstMng->sectVol,
					saveRoute->sectCurrent);
			result = e_SC_RESULT_FAIL;
			break;
		}
		// 登録用区間アドレス
		dstSect = (mstMng->sectInfo + saveRoute->sectCurrent);
		if (0 == saveRoute->sectCurrent) {
			preSect = dstSect;
		} else {
			// 経由区間開始リンクが前回経路最終リンク以外から開始している場合経路を削る
			if (!checkSameTarmLink(aNewSectRouteMng)) {
				result = searchSameTarmLink(aNewSectRouteMng, aSectCtrl);
				if (e_SC_RESULT_SUCCESS != result) {
					SC_LOG_ErrorPrint(SC_TAG_RC, "searchSameTarmLink error. [0x%08x] "HERE, result);
					break;
				}
			}
			preSect = (mstMng->sectInfo + saveRoute->sectCurrent - 1);
		}
		// 今回区間情報作成(リンク等は後)
		dstSect->priority = aSectCtrl->routeCond;
		dstSect->sectDist = preSect->sectDist;

		// ■SC_RP_ParcelInfo入れ替え
		newSize = (mstMng->parcelVol + aNewSectRouteMng->parcelVol);
		if (newSize) {
			newPclInfo = (SC_RP_ParcelInfo*) RP_MemAlloc(sizeof(SC_RP_ParcelInfo) * newSize, e_MEM_TYPE_ROUTEMNG);
			if (NULL == newPclInfo) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error. "HERE);
				result = e_SC_RESULT_MALLOC_ERR;
				break;
			}
			RP_Memset0(newPclInfo, sizeof(SC_RP_ParcelInfo) * newSize);
			RP_Memcpy(newPclInfo, mstMng->parcelInfo, sizeof(SC_RP_ParcelInfo) * mstMng->parcelVol);
			RP_Memcpy((newPclInfo + mstMng->parcelVol), aNewSectRouteMng->parcelInfo,
					sizeof(SC_RP_ParcelInfo) * aNewSectRouteMng->parcelVol);
			// 旧エリア開放
			if (NULL != mstMng->parcelInfo) {
				RP_MemFree(mstMng->parcelInfo, e_MEM_TYPE_ROUTEMNG);
			}
			mstMng->parcelInfo = newPclInfo;
			mstMng->parcelVol = newSize;
		}

		// ■SC_RP_LinkInfo入れ替え
		newSize = (mstMng->linkVol + aNewSectRouteMng->linkVol);
		if (newSize) {
			newLinkInfo = (SC_RP_LinkInfo*) RP_MemAlloc(sizeof(SC_RP_LinkInfo) * newSize, e_MEM_TYPE_ROUTEMNG);
			if (NULL == newLinkInfo) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error. "HERE);
				result = e_SC_RESULT_MALLOC_ERR;
				break;
			}
			RP_Memset0(newLinkInfo, sizeof(SC_RP_LinkInfo) * newSize);
			RP_Memcpy(newLinkInfo, mstMng->linkInfo, sizeof(SC_RP_LinkInfo) * mstMng->linkVol);
			RP_Memcpy((newLinkInfo + mstMng->linkVol), aNewSectRouteMng->linkInfo, sizeof(SC_RP_LinkInfo) * aNewSectRouteMng->linkVol);
			// 旧エリア開放
			if (NULL != mstMng->linkInfo) {
				RP_MemFree(mstMng->linkInfo, e_MEM_TYPE_ROUTEMNG);
			}
			mstMng->linkInfo = newLinkInfo;
			mstMng->linkVol = newSize;
		}

		// ■SC_RP_FormInfo入れ替え
		newSize = (mstMng->formVol + aNewSectRouteMng->formVol);
		if (newSize) {
			newFormInfo = (SC_RP_FormInfo*) RP_MemAlloc(sizeof(SC_RP_FormInfo) * newSize, e_MEM_TYPE_ROUTEMNG);
			if (NULL == newFormInfo) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error. "HERE);
				result = e_SC_RESULT_MALLOC_ERR;
				break;
			}
			RP_Memset0(newFormInfo, sizeof(SC_RP_FormInfo) * newSize);
			RP_Memcpy(newFormInfo, mstMng->formInfo, sizeof(SC_RP_FormInfo) * mstMng->formVol);
			RP_Memcpy((newFormInfo + mstMng->formVol), aNewSectRouteMng->formInfo, sizeof(SC_RP_FormInfo) * aNewSectRouteMng->formVol);
			// 旧エリア開放
			if (NULL != mstMng->formInfo) {
				RP_MemFree(mstMng->formInfo, e_MEM_TYPE_ROUTEMNG);
			}
			mstMng->formInfo = newFormInfo;
			mstMng->formVol = newSize;
		}

		// ■SC_RP_RegInfo入れ替え
		newSize = (mstMng->regVol + aNewSectRouteMng->regVol);
		if (newSize) {
			newRegInfo = (SC_RP_RegInfo*) RP_MemAlloc(sizeof(SC_RP_RegInfo) * newSize, e_MEM_TYPE_ROUTEMNG);
			if (NULL == newRegInfo) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error. "HERE);
				result = e_SC_RESULT_MALLOC_ERR;
				break;
			}
			RP_Memset0(newRegInfo, sizeof(SC_RP_RegInfo) * newSize);
			RP_Memcpy(newRegInfo, mstMng->regInfo, sizeof(SC_RP_RegInfo) * mstMng->regVol);
			RP_Memcpy((newRegInfo + mstMng->regVol), aNewSectRouteMng->regInfo, sizeof(SC_RP_RegInfo) * aNewSectRouteMng->regVol);
			// 旧エリア開放
			if (NULL != mstMng->regInfo) {
				RP_MemFree(mstMng->regInfo, e_MEM_TYPE_ROUTEMNG);
			}
			mstMng->regInfo = newRegInfo;
			mstMng->regVol = newSize;
		}

		// ■SC_RP_SectInfo入れ替え
		dstSect->parcelIdx = preSect->parcelIdx + preSect->parcelVol;
		dstSect->parcelVol = aNewSectRouteMng->parcelVol;
		dstSect->linkIdx = preSect->linkIdx + preSect->linkVol;
		dstSect->linkVol = aNewSectRouteMng->linkVol;
		dstSect->formIdx = preSect->formIdx + preSect->formVol;
		dstSect->formVol = aNewSectRouteMng->formVol;

		// 区間座標設定
		dstSect->parcelId = aSectCtrl->neighbor[SCRP_NBRDST].point.parcelId;
		dstSect->x = aSectCtrl->neighbor[SCRP_NBRDST].point.x;
		dstSect->y = aSectCtrl->neighbor[SCRP_NBRDST].point.y;

		// 区間情報作成（旅行時間・距離）
		result = setRouteSectInfo(mstMng, saveRoute->sectCurrent);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "setRouteSectInfo error. [0x%08x] "HERE, result);
			break;
		}
	} while (0);

	if (result == e_SC_RESULT_SUCCESS) {
		// 区間情報登録数更新
		saveRoute->sectCurrent++;
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief 指定区間の情報設定
 * @param aRtMng [I/O]経路管理
 * @param aSectIdx [I]区間No
 */
static E_SC_RESULT setRouteSectInfo(SC_RP_RouteMng* aRouteManager, UINT8 aSectIdx) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	UINT32 sectDist = 0;
	UINT32 sectHWDist = 0;
	UINT32 sectTime = 0;
	UINT32 i, e;

	SC_RP_SectInfo* sectInfo = NULL;
	SC_RP_LinkInfo* linkInfo = NULL;
	SC_RP_ParcelInfo* pclInfo = NULL;

	if (NULL == aRouteManager) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 区間とパーセル先頭取得
	sectInfo = (aRouteManager->sectInfo + aSectIdx);
	pclInfo = (aRouteManager->parcelInfo + sectInfo->parcelIdx);

	// 断裂初期化
	sectInfo->splitIdx = SC_RP_SPLIT_INDEX_INIT;

	for (i = 0; i < sectInfo->parcelVol; i++, pclInfo++) {
		// リンクテーブル先頭
		linkInfo = (aRouteManager->linkInfo + sectInfo->linkIdx) + pclInfo->linkIdx;
		// 前回距離を格納
		pclInfo->preDist = sectDist;
		for (e = 0; e < pclInfo->linkVol; e++, linkInfo++) {
			// 区間距離に加算
			sectDist += linkInfo->dist;
			if (SC_MA_ROAD_TYPE_TOLLWAY > linkInfo->roadKind) {
				sectHWDist += linkInfo->dist;
			}
			sectTime += linkInfo->travelTime;

			// 断裂INDEX
			if (linkInfo->splitFlag) {
				sectInfo->splitIdx = (pclInfo->linkIdx + e);
			}
		}
	}
	// 結果格納
	sectInfo->sectDist = sectDist;
	sectInfo->sectHWDist = sectHWDist;
	sectInfo->sectTime = sectTime;

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 新区間の開始リンクと前回区間の終了リンクが同一であるかチェックする
 * @param 新区間経路管理
 */
static Bool checkSameTarmLink(SC_RP_RouteMng* aNewRouteMng) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aNewRouteMng) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (true);
	}
	Bool bret = false;
	RCRT_SAVEROUTEINFO* saveRoute = RCRT_GET_SAVEROUTEINFO();
	SC_RP_RouteMng* mstMng = &saveRoute->routeMng;

	//SC_RP_SectInfo* newSect = aNewRouteMng->sectInfo;
	SC_RP_ParcelInfo* newPclInfo = aNewRouteMng->parcelInfo;
	SC_RP_LinkInfo* newLinkInfo = aNewRouteMng->linkInfo;

	SC_RP_SectInfo* preSect = mstMng->sectInfo + saveRoute->sectCurrent - 1;
	SC_RP_ParcelInfo* prePclInfo = mstMng->parcelInfo + preSect->parcelIdx + preSect->parcelVol - 1;
	SC_RP_LinkInfo* preLinkInfo = mstMng->linkInfo + preSect->linkIdx + preSect->linkVol - 1;

	if (preLinkInfo->linkId == newLinkInfo->linkId && preLinkInfo->orFlag == newLinkInfo->orFlag
			&& prePclInfo->parcelId == newPclInfo->parcelId) {
		bret = true;
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (bret);
}

/**
 * @brief 新区間の開始リンクと前回区間のリンクが同一でない場合前回区間のリンク情報を削る処理
 * @param 新区間経路管理
 */
static E_SC_RESULT searchSameTarmLink(SC_RP_RouteMng* aNewRouteMng, SCRP_SECTCONTROLER* aSectCtrl) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aNewRouteMng) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	RCRT_SAVEROUTEINFO* saveRoute = RCRT_GET_SAVEROUTEINFO();
	SC_RP_RouteMng* mstMng = &saveRoute->routeMng;

	//SC_RP_SectInfo* newSect = aNewRouteMng->sectInfo;
	SC_RP_ParcelInfo* newPclInfo = aNewRouteMng->parcelInfo;
	SC_RP_LinkInfo* newLinkInfo = aNewRouteMng->linkInfo;

	SC_RP_SectInfo* preSect = mstMng->sectInfo + saveRoute->sectCurrent - 1;
	SC_RP_ParcelInfo* prePclInfo = mstMng->parcelInfo + preSect->parcelIdx;
	SC_RP_LinkInfo* preLinkInfo = mstMng->linkInfo + preSect->linkIdx;
	SC_RP_LinkInfo* targetLinkInfo = NULL;
	RCRT_TERMINFO termInfo = {};
	Bool find = false;
	UINT32 i, e;

	if (0 == saveRoute->sectCurrent) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	do {
		for (i = 0; i < preSect->parcelVol; i++, prePclInfo++) {
			if (prePclInfo->parcelId != newPclInfo->parcelId) {
				continue;
			}
			targetLinkInfo = preLinkInfo + prePclInfo->linkIdx;
			for (e = 0; e < prePclInfo->linkVol; e++, targetLinkInfo++) {
				if (targetLinkInfo->linkId == newLinkInfo->linkId && targetLinkInfo->orFlag == newLinkInfo->orFlag) {
					find = true;
					break;
				}
			}
			if (find) {
				break;
			}
		}
		if (!find) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "can't found start link. "HERE);
			result = e_SC_RESULT_FAIL;
			break;
		}
		if (targetLinkInfo->termFlag & SC_RP_TERM_IS_FIRST) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "find route link is first. "HERE);
			// 同一リンクエラー。前回区間でのエラーとするため区間は-1を設定
			if (0 < aSectCtrl->sectIndex) {
				RPC_SetCalcSect(aSectCtrl->sectIndex - 1);
			}
			RPC_SetErrorCode(EC_SAME_ROAD);
			result = e_SC_RESULT_FAIL;
			break;
		}

		// 終端フラグ
		targetLinkInfo->termFlag = SC_RP_TERM_IS_LAST;

		// 出発地側の近傍に前回最終リンクの近傍情報に一致するものが格納済み
		result = getTermNbrLinkInfo(&aSectCtrl->neighbor[0], prePclInfo->parcelId, targetLinkInfo, false, &termInfo);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "getTermNbrLinkInfo error. [0x%08x] "HERE, result);
			break;
		}
		// 編集インデックス無効値は編集しない
		if (-1 != termInfo.editIdx) {
			// リンク情報書き換え
			targetLinkInfo->formVol = termInfo.formVol;
			targetLinkInfo->formIdx = targetLinkInfo->formIdx + termInfo.stSubIdx;
			targetLinkInfo->travelTime *= ((DOUBLE) termInfo.fillDist / (DOUBLE) termInfo.linkDist);
			targetLinkInfo->dist = termInfo.fillDist;
			// 形状点情報書き換え
			SC_RP_FormInfo* wkFormInfo = mstMng->formInfo + preSect->formIdx + targetLinkInfo->formIdx + termInfo.editIdx;
			wkFormInfo->x = termInfo.vX;
			wkFormInfo->y = termInfo.vY;
		}

		// パーセル情報書き換え
		prePclInfo->linkVol = e + 1;
		prePclInfo->subLinkVol = targetLinkInfo->formIdx + targetLinkInfo->formVol - prePclInfo->subLinkIdx;

		// 区間情報書き換え
		preSect->parcelVol = i + 1;
		preSect->linkVol = prePclInfo->linkIdx + prePclInfo->linkVol;
		preSect->formVol = prePclInfo->subLinkIdx + prePclInfo->subLinkVol;

		// マスター書き換え
		mstMng->parcelVol = preSect->parcelIdx + preSect->parcelVol;
		mstMng->linkVol = preSect->linkIdx + preSect->linkVol;
		mstMng->formVol = preSect->formIdx + preSect->formVol;

		result = setRouteSectInfo(mstMng, saveRoute->sectCurrent - 1);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "setRouteSectInfo error. [0x%08x] "HERE, result);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief レベル１エリアかぶり判定
 * @param
 */
static Bool checkSameLevelOverArea(SCRP_SECTCONTROLER* aSectCtrl) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aSectCtrl) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (false);
	}

	Bool bResult = false;
	SCRP_PCLRECT* oRect = &aSectCtrl->levelTable.areaTable[0].pclRect;
	SCRP_PCLRECT* dRect = &aSectCtrl->levelTable.areaTable[2].pclRect;
	INT32 sftX = 0, sftY = 0;
	Bool xCover = false;
	Bool yCover = false;

	if (-1 == SC_MESH_GetAlterPos(oRect->parcelId, dRect->parcelId, RP_LEVEL1, &sftX, &sftY)) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (false);
	}

	if (0 <= sftX) {
		if (sftX < oRect->xSize) {
			xCover = true;
		} else {
			xCover = false;
		}
	} else {
		if (0 <= dRect->xSize + sftX) {
			xCover = true;
		} else {
			xCover = false;
		}
	}

	if (0 <= sftY) {
		if (sftY < oRect->ySize) {
			yCover = true;
		} else {
			yCover = false;
		}
	} else {
		if (0 <= dRect->ySize + sftY) {
			yCover = true;
		} else {
			yCover = false;
		}
	}

	if (xCover && yCover) {
		bResult = true;
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (bResult);
}

/**
 * @brief 同一レベル候補経路から接続先を検索する
 * @param 区間管理
 * @param 接続レベル
 * @param ワーク候補リスト
 * @memo 作成済みワーク候補リストのリンク何れかが同一レベルの候補テーブルにあるかを全検索します。
 *       検索結果コストが安ければ採用し経路とします。
 *       発見段階でワーク候補リストのインデックス等書き換える為注意が必要です。
 *       TODO 規制等データがある場合は確認が必要です。
 */
static UINT32 searchSameLvConnectCand(SCRP_SECTCONTROLER* aSectCtrl, UINT8 aLevel, RCRT_CAND_WKTBL* aCandWkTbl) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aSectCtrl || NULL == aCandWkTbl) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (ALL_F32);
	}

	UINT32 findIdx = ALL_F32;
	UINT32 i, e;
	SCRP_CANDDATA** ppCand = aCandWkTbl->ppCandData;
	SCRP_CANDTBLINFO* candInfo = NULL;

	switch (aLevel) {
	case RP_LEVEL1:
		candInfo = &aSectCtrl->candMng.candTblInfo[RC_CAND_IDX_LV1O];
		break;
	default:
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (ALL_F32);
	}

	for (i = 0; i < aCandWkTbl->candCrnt; i++, ppCand++) {
		SCRP_CANDDATA* cand = aSectCtrl->candMng.cand + candInfo->candIdx;
		for (e = 0; e < candInfo->candSize; e++, cand++) {
			if (cand->parcelId != (*ppCand)->parcelId) {
				continue;
			}
			if (cand->linkId != (*ppCand)->linkId) {
				continue;
			}
			if (RCND_GET_ORIDX(cand->flag) != RCND_GET_ORIDX((*ppCand)->flag)) {
				continue;
			}
			if ((*ppCand)->cost < cand->cost) {
				continue;
			}
			// 発見
			aCandWkTbl->candCrnt = i;
			findIdx = candInfo->candIdx + e;

			SC_LOG_InfoPrint(SC_TAG_RC, "same level cand connect! pcl=0x%08x link=0x%08x flag=0x%04x "HERE, cand->parcelId, cand->linkId,
					cand->flag);
			SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
			return (findIdx);
		}
	}

	return (ALL_F32);
}

/**
 * @brief 平均旅行時間取得（単位：0.1秒）
 * @param [I]aNetTable
 * @param [I]linkId リンクID
 * @memo ネットワークデータからリンクIDで検索を行い、該当リンクの平均旅行時間を取得する
 */
static E_SC_RESULT getTravelTime(SCRP_MAPDATA* aMapInfo, UINT32 aLinkId, UINT32* aResultCost) {

	if (NULL == aMapInfo || NULL == aMapInfo->road) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	T_MapNWLink* pLink = NULL;
	UINT32 cost = 0;
	MAL_HDL bin = NULL;

	// リンク検索
	UINT16 index = SC_MA_BinSearchNwRecord(aMapInfo->road, aLinkId, SC_MA_BINSRC_TYPE_LINK);
	if (ALL_F16 == index) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "can't find link. link=0x%08x "HERE, aLinkId);
		return (e_SC_RESULT_FAIL);
	}
	// アドレス特定
	bin = SC_MA_A_NWBIN_GET_NWRCD_LINK(aMapInfo->road);
	pLink = (T_MapNWLink*) SC_MA_A_NWRCD_LINK_GET_RECORD(bin, index - 1);

	// 旅行時間算出
	SC_MA_CALC_LINK_TRAVELTIME_U(pLink->dir.travelTime, pLink->dir.travelUnit, cost);

	// 結果格納
	*aResultCost = cost;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 地図Ver設定処理
 * @param 経路管理
 */
static E_SC_RESULT setRouteMapVer(SC_RP_RouteMng* aRoutMng) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	SC_DA_RESULT daResult = SC_DA_RES_SUCCESS;
	INT32 size;

	if (NULL == aRoutMng) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 初期化
	RP_Memset0(aRoutMng->mapVer, sizeof(Char) * SC_RP_MAPVER_SIZE);

	// バージョンサイズ
	daResult = SC_DA_GetSystemMapVerNoDataSize(&size);
	if (SC_DA_RES_SUCCESS != daResult) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "SC_DA_GetSystemMapVerNoDataSize error. [0x%08x] "HERE, daResult);
		return (e_SC_RESULT_FAIL);
	}
	if (SC_RP_MAPVER_SIZE < size) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "map version size over... "HERE);
		return (e_SC_RESULT_FAIL);
	}
	// バージョン
	daResult = SC_DA_GetSystemMapVerNoData(aRoutMng->mapVer);
	if (SC_DA_RES_SUCCESS != daResult) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "SC_DA_GetSystemMapVerNoData error. [0x%08x] "HERE, daResult);
		return (e_SC_RESULT_FAIL);
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief	リンク情報取得処理
 * @param	[I]aNetTable
 * @param	[I]linkId リンクID
 */
static T_MapNWLink* getRoadLinkInfo(SCRP_MAPDATA* aMapData, UINT32 aLinkId) {

	T_MapNWLink* pLink = NULL;
	MAL_HDL bin = NULL;
	UINT16 index = ALL_F16;

	if (NULL == aMapData || NULL == aMapData->road) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (NULL );
	}

	// リンク情報取得
	index = SC_MA_BinSearchNwRecord(aMapData->road, aLinkId, SC_MA_BINSRC_TYPE_LINK);
	if (ALL_F16 == index) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "SC_MA_BinSearchNwRecord failed. pcl=0x%08x link=0x%08x "HERE, aMapData->parcelId, aLinkId);
		return (NULL );
	}
	bin = SC_MA_A_NWBIN_GET_NWRCD_LINK(aMapData->road);
	pLink = (T_MapNWLink*) SC_MA_A_NWRCD_LINK_GET_RECORD(bin, index - 1);

	return (pLink);
}

/**
 * @brief 近距離判定1500m未満の場合近距離と判断させる
 * @param 区間管理
 */
static Bool checkSectPointNear(SCRP_SECTCONTROLER* aSectCtrl) {
	if (NULL == aSectCtrl) {
		return (false);
	}
	DOUBLE odDistance = RP_Lib_CalcODLength(aSectCtrl->neighbor[0].point, aSectCtrl->neighbor[1].point);
	if (1500 < odDistance) {
		return (false);
	} else {
		return (true);
	}
}
