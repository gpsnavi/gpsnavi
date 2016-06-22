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
 * RP_RouteControl.c
 *
 *  Created on: 2015/11/06
 *      Author: masutani
 */

#include "sms-core/SMCoreRP/SMCoreRPInternal.h"

/* 経由地探索開始リンク検索数
 * TODO 正式な値ではないが一通の場合追いかける本数については要検討(100%救うことはできないのでは) */
#define RP_NEXTSTARTLINK_VOL (2)

/*-------------------------------------------------------------------
 * 宣言
 *-------------------------------------------------------------------*/
static E_SC_RESULT setRoutingFunc();
static E_SC_RESULT releaseMasterRoute();
static E_SC_RESULT setNextCond(INT16 aSectIdx, SCRP_SECTCONTROLER* aSectCtrl);
static E_SC_RESULT setRouteHeadInfo(SC_RP_RouteMng* aRouteMng);
static E_SC_RESULT reRouteDataStandby(SCRP_SECTCONTROLER* aSectCtrl);
static E_SC_RESULT checkLimitedDistance(SCRP_SEARCHSETTING *aSearchSetting);
static E_SC_RESULT getAllNeighborLinks(SCRP_MANAGER *aRcManager);
static E_SC_RESULT setNextStartLink(SCRP_NEIGHBORINFO* aNeighbor, INT16 aSectIdx);
static E_SC_RESULT routeSectNumberUpdate(SC_RP_RouteMng* aRt, UINT16 aStartNumber);
static void sectionFinal(SCRP_SECTCONTROLER* aSectCtrl, Bool aIsRetry);
static void routePlanFinal(SCRP_MANAGER *aRcManager);
static void routeIdCountUp();
static Bool judgeRetryCalc(SCRP_SECTCONTROLER* aSectCtrl);

/* 経路ID */
static UINT32 mRouteID = SC_RP_RTIDINIT;

/**
 * 単経路探索制御
 * @param 検索ID
 * @param 設定ページ
 */
void RC_RtControlSingle(UINT32 aSearchID, UINT16 aSettingIdx) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SCRP_MANAGER *rcManager = RPC_GetRouteCalcManager();
	SCRP_SECTCONTROLER sectCtrl = {};

#if _RP_LOG_MEMINFO
	SC_MEM_Dump_Type(e_MEM_TYPE_ROUTEPLAN);
	SC_MEM_Dump_Type(e_MEM_TYPE_ROUTECAND);
	SC_MEM_Dump_Type(e_MEM_TYPE_ROUTEMNG);
#endif

	// 探索種別（シングルルート）
	RPC_SetRoutePlanType(e_SC_ROUTETYPE_SINGLE);

	// 推奨経路管理掃除(カレント経路ID以外の経路削除)
	SC_RP_RouteCleaning();

	// 経路管理初期化
	RC_ClearRouteMakeInfo();

	// ローカル経路ID更新
	routeIdCountUp();

	RP_SetLapTime(); // ▼時間計測

	do {
		// ダウンロードエリア情報取り込み
		result = RC_SetDownLoad_Area();
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RC_SetDownLoad_Area error. [0x%08x] ."HERE, result);
			break;
		}

		// 探索条件設定
		if (e_SC_RESULT_SUCCESS != RPC_SetCurrentRouteSearchSetting()) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RPC_SetCurrentRouteSearchSetting error. "HERE);
			result = e_SC_RESULT_FAIL;
			break;
		}

		// 探索可能距離チェック
		result = checkLimitedDistance(rcManager->rcSetting);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "checkLimitedDistance error. [0x%08x] "HERE, result);
			break;
		}

		// 探索設定に対応する関数を設定する
		result = setRoutingFunc();
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "setRoutingFunc error. [0x%08x] "HERE, result);
			break;
		}

		// 全地点の近傍情報を収集する
		result = getAllNeighborLinks(rcManager);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "getAllNeighborLinks error. [0x%08x] "HERE, result);
			break;
		}

		// トータル区間数登録
		RPC_SetTotalSect(rcManager->rcSetting->pointNum - 1);
		rcManager->sectVol = (rcManager->rcSetting->pointNum - 1);
		if (RP_ROUTE_PLACE_MAX < rcManager->rcSetting->pointNum) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "Way point count over flow. "HERE);
			result = e_SC_RESULT_ROUTE_ERR;
			break;
		}

		// 推奨経路ステータスの初期化
		RC_CreateSectMngTbl(rcManager->sectVol);

		// 経由地 + 1 ループ
		UINT32 section = 0;
		for (section = 0; section < rcManager->sectVol; section++) {
			SC_LOG_InfoPrint(SC_TAG_RC, "_/ _/ _/ route calc... section[%d/%d] _/ _/ _/", (section + 1), rcManager->sectVol);
			RP_SetLapTimeWithStr("start sect loop."); // ▼時間計測

			// プロセス登録
			RPC_SetCalcSect(section);
			// 区間インデックス登録
			sectCtrl.sectIndex = section;

			// キャンセル判定
			if (true == RPC_IsCancelRequest()) {
				result = e_SC_RESULT_ROUTE_CANCEL;
				break;
			}

			// 次回探索用の近傍情報を区間管理情報へ設定する
			if (0 == section) {
				RP_Memcpy(&(sectCtrl.neighbor[SCRP_NBROWN]), &(rcManager->neighbor[section]), sizeof(SCRP_NEIGHBORINFO));
			} else {
				result = setNextStartLink(&sectCtrl.neighbor[SCRP_NBROWN], section);
			}
			RP_Memcpy(&(sectCtrl.neighbor[SCRP_NBRDST]), &(rcManager->neighbor[section + 1]), sizeof(SCRP_NEIGHBORINFO));
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "setNextStartLink error. [0x%08x] "HERE, result);
				break;
			}

			// 区間探索条件設定
			result = setNextCond(section, &sectCtrl);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "setNextCond error. [0x%08x] "HERE, result);
				result = e_SC_RESULT_BADPARAM;
				break;
			}

			INT8 retry = 0;
			for (retry = 0; retry < 2; retry++) {
#if _RP_USE_OLDAREA // 旧エリア処理の場合強制Lv1をコールする
				if (retry == 0) {
					result = RC_AreaOnlyLv1(&sectCtrl.levelTable, &sectCtrl.netTable, &sectCtrl.neighbor[SCRP_NBROWN],
							&sectCtrl.neighbor[SCRP_NBRDST]);
				} else {
					SC_LOG_ErrorPrint(SC_TAG_RC, "retry error. retry=%d [0x%08x] "HERE, retry, result);
					break;
				}
#else
				// 探索エリア（前回レベル１エリアの場合のみリトライ）
				if (retry == 0) {
					result = RC_Area(&sectCtrl.levelTable, &sectCtrl.netTable, &sectCtrl.neighbor[SCRP_NBROWN],
							&sectCtrl.neighbor[SCRP_NBRDST]);
				} else if (judgeRetryCalc(&sectCtrl)) {
					// 区間終了処理（近傍初期化しない版）
					sectionFinal(&sectCtrl, true);
					// 強制レベル２エリア生成
					result = RC_AreaOnlyLv2(&sectCtrl.levelTable, &sectCtrl.netTable, &sectCtrl.neighbor[SCRP_NBROWN],
							&sectCtrl.neighbor[SCRP_NBRDST]);
				} else {
					SC_LOG_DebugPrint(SC_TAG_RC, "no retry. retry=%d [0x%08x] "HERE, retry, result);
					break;
				}
				if (e_SC_RESULT_SUCCESS != result) {
					SC_LOG_ErrorPrint(SC_TAG_RC, "RC_Area error. retry=%d [0x%08x] "HERE, retry, result);
					continue;
				}
#endif

#if _RP_LOG_LEVELTBL
				RPDBG_ShowSectLvInfo(&sectCtrl);
#endif
				// ネットワーク構築処理＋候補経路迄
				// 各階層処理実行
				result = RC_LevelProcess(&sectCtrl);
				if (e_SC_RESULT_SUCCESS != result) {
					SC_LOG_ErrorPrint(SC_TAG_RC, "RC_LevelProcess error. [0x%08x] "HERE, result);
					continue;
				}

				// 推奨経路作成
				result = RC_RouteMake(rcManager, &sectCtrl, section);
				if (e_SC_RESULT_SUCCESS != result) {
					SC_LOG_ErrorPrint(SC_TAG_RC, "RC_RouteMake error. [0x%08x] "HERE, result);
					continue;
				}
				// 推奨経路生成まで成功している場合リトライしない
				break;
			}
			if (e_SC_RESULT_SUCCESS != result) {
				break;
			}
			// 区間終了処理
			sectionFinal(&sectCtrl, false);

			RP_SetLapTimeWithStr("end sect proccess."); // ▼時間計測
		}
		if (e_SC_RESULT_SUCCESS != result) {
			// 区間終了処理
			sectionFinal(&sectCtrl, false);
			break;
		}

		// キャンセル判定
		if (true == RPC_IsCancelRequest()) {
			result = e_SC_RESULT_ROUTE_CANCEL;
			break;
		}
		// 推奨経路登録
		result = releaseMasterRoute();
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "releaseMasterRoute error. [0x%08x] "HERE, result);
			// TODO 推奨経路の登録失敗
		}
	} while (0);

	// エラーコード登録
	RPC_SetProcess2ErrorCode(result);

	// 探索結果共有メモリ登録
	if (e_SC_RESULT_SUCCESS != RP_SetRoutePlanTip()) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "RP_SetRoutePlanTip error. "HERE);
	}

	RP_SetLapTimeWithStr("finish route calc."); // ▼時間計測

	switch (result) {
	case e_SC_RESULT_SUCCESS:
		SC_RP_RouteSetId(mRouteID, rcManager->rpType);
		// 応答MSG送信
		RC_SendSingleRouteResponse(result, mRouteID, RPC_GetCurrentSettingIdx());
		break;
	case e_SC_RESULT_ROUTE_CANCEL:
		// 応答MSG送信
		RC_SendSingleRouteResponse(result, SC_RP_RTIDINIT, RPC_GetCurrentSettingIdx());
		break;
	default:
		// 応答MSG送信
		RC_SendSingleRouteResponse(result, SC_RP_RTIDINIT, RPC_GetCurrentSettingIdx());
		break;
	}

	// 探索失敗時推奨経路作業領域を解放する
	if (e_SC_RESULT_SUCCESS != result) {
		RC_ResultFailFinalRouteMakeTbl();
	}
	// 終了処理
	routePlanFinal(rcManager);

	//メモリ全開放
	RP_MemClean(e_MEM_TYPE_ROUTEPLAN);
	RP_MemClean(e_MEM_TYPE_ROUTECAND);
#if _RP_LOG_MEMINFO
	SC_MEM_Dump_Type(e_MEM_TYPE_ROUTEMNG);
#endif

	// プロセス登録
	RPC_SetCalcProcess(e_RC_PROC_STANDBY);

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return;
}

/**
 * @brief 再探索制御
 * @param 検索ID
 * @param 設定ページ
 */
void RC_RtControlRePlan(UINT32 aSearchID, UINT16 aSettingIdx) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SCRP_MANAGER *rcManager = RPC_GetRouteCalcManager();
	SCRP_SECTCONTROLER sectCtrl = {};

#if _RP_LOG_MEMINFO
	SC_MEM_Dump_Type(e_MEM_TYPE_ROUTEPLAN);
	SC_MEM_Dump_Type(e_MEM_TYPE_ROUTECAND);
	SC_MEM_Dump_Type(e_MEM_TYPE_ROUTEMNG);
#endif

	// 探索種別（リルート）
	RPC_SetRoutePlanType(e_SC_ROUTETYPE_REROUTE);

	// 推奨経路管理掃除(カレント経路ID以外の経路削除)
	SC_RP_RouteCleaning();

	// 経路管理初期化
	RC_ClearRouteMakeInfo();

	// ローカル経路ID更新
	routeIdCountUp();

	RP_SetLapTime(); // ▼時間計測

	do {
		// ダウンロードエリア情報取り込み
		result = RC_SetDownLoad_Area();
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RC_SetDownLoad_Area error. [0x%08x] "HERE, result);
			break;
		}

		// 探索条件設定
		if (e_SC_RESULT_SUCCESS != RPC_SetCurrentRouteSearchSetting()) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RPC_SetCurrentRouteSearchSetting error. "HERE);
			result = e_SC_RESULT_FAIL;
			break;
		}

		// 探索設定に対応する関数を設定する
		result = setRoutingFunc();
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "setRoutingFunc error. [0x%08x] "HERE, result);
			break;
		}

		// 探索可能距離チェック
		result = checkLimitedDistance(rcManager->rcSetting);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "checkLimitedDistance error. "HERE, result);
			break;
		}

		// 近傍取得及び経路保存
		result = reRouteDataStandby(&sectCtrl);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "reRouteDataStandby error. [0x%08x] "HERE, result);
			break;
		}

		// 再探索時は実際に計算される区関数と経路全体の区関数は異なる
		RPC_SetTotalSect(rcManager->rcSetting->pointNum - 1);
		// 現区間0登録
		RPC_SetCalcSect(0);
		// 区間インデックス登録
		sectCtrl.sectIndex = 0;

		// 推奨経路テーブル生成(1固定)
		result = RC_CreateSectMngTbl(1);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RC_CreateSectMngTbl error. [0x%08x] "HERE, result);
			break;
		}

		SC_LOG_InfoPrint(SC_TAG_RC, "_/ _/ _/ route calc... re:route section[%d] _/ _/ _/", rcManager->rcSetting->replanSect);

		RP_SetLapTimeWithStr("start sect."); // ▼時間計測

		// キャンセル判定
		if (true == RPC_IsCancelRequest()) {
			result = e_SC_RESULT_ROUTE_CANCEL;
			break;
		}

		INT8 retry = 0;
		for (retry = 0; retry < 2; retry++) {
#if _RP_USE_OLDAREA // 旧エリア処理の場合強制Lv1をコールする
			result = RC_AreaOnlyLv1(&sectCtrl.levelTable, &sectCtrl.netTable, &sectCtrl.neighbor[SCRP_NBROWN], &sectCtrl.neighbor[SCRP_NBRDST]);
#else
			// 探索エリア：近傍情報は1,2へ格納済み
			if (0 == retry) {
				result = RC_Area(&sectCtrl.levelTable, &sectCtrl.netTable, &rcManager->neighbor[0], &rcManager->neighbor[1]);
			} else if (judgeRetryCalc(&sectCtrl)) {
				// 区間終了処理（近傍初期化しない版）
				sectionFinal(&sectCtrl, true);
				// 強制レベル２エリア生成
				result = RC_AreaOnlyLv2(&sectCtrl.levelTable, &sectCtrl.netTable, &rcManager->neighbor[0], &rcManager->neighbor[1]);
			} else {
				SC_LOG_DebugPrint(SC_TAG_RC, "no retry. retry=%d [0x%08x] "HERE, retry, result);
				break;
			}
#endif
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "RC_Area error. [0x%08x] "HERE, result);
				continue;
			}
			// ネットワーク構築処理＋候補経路迄
			result = RC_LevelProcess(&sectCtrl);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "RC_LevelProcess error. [0x%08x] "HERE, result);
				continue;
			}

			// 推奨経路作成
			result = RC_RouteMake(rcManager, &sectCtrl, 0);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "RC_RouteMake error. [0x%08x] "HERE, result);
				continue;
			}
			// 推奨経路生成まで成功している場合リトライしない
			break;
		}
		if (e_SC_RESULT_SUCCESS != result) {
			break;
		}
		// 保存してある流用経路を組み立て
		result = RC_JointRePlanRoute();
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RC_JointRePlanRoute error. [0x%08x] "HERE, result);
			break;
		}
		// キャンセル判定
		if (true == RPC_IsCancelRequest()) {
			result = e_SC_RESULT_ROUTE_CANCEL;
			break;
		}
		result = releaseMasterRoute();
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "releaseMasterRoute error. [0x%08x] "HERE, result);
			// TODO 推奨経路の登録失敗
			break;
		}
	} while (0);

	// 区間終了処理
	sectionFinal(&sectCtrl, false);

	// エラーコード登録
	RPC_SetProcess2ErrorCode(result);

	// 探索結果共有メモリ登録
	if (e_SC_RESULT_SUCCESS != RP_SetRoutePlanTip()) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "RP_SetRoutePlanTip error. "HERE);
	}

	RP_SetLapTimeWithStr("finish route calc."); // ▼時間計測

	switch (result) {
	case e_SC_RESULT_SUCCESS:
		// 経路マネージャへ確定経路IDを登録
		SC_RP_RouteSetId(mRouteID, rcManager->rpType);
		// 応答メッセージ送信
		RC_SendRePlanResponse(result, mRouteID, RPC_GetCurrentSettingIdx());
		break;
	case e_SC_RESULT_ROUTE_CANCEL:
		// 応答メッセージ送信
		RC_SendRePlanResponse(result, SC_RP_RTIDINIT, RPC_GetCurrentSettingIdx());
		break;
	default:
		// 応答メッセージ送信
		RC_SendRePlanResponse(result, SC_RP_RTIDINIT, RPC_GetCurrentSettingIdx());
		break;
	}

	// 探索失敗時推奨経路作業領域を解放する
	if (e_SC_RESULT_SUCCESS != result) {
		RC_ResultFailFinalRouteMakeTbl();
	}
	// 終了処理
	routePlanFinal(rcManager);

	//メモリ全開放
	RP_MemClean(e_MEM_TYPE_ROUTEPLAN);
	RP_MemClean(e_MEM_TYPE_ROUTECAND);
#if _RP_LOG_MEMINFO
	SC_MEM_Dump_Type(e_MEM_TYPE_ROUTEMNG);
#endif

	// プロセス登録
	RPC_SetCalcProcess(e_RC_PROC_STANDBY);

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return;
}

/**
 * @brief 経路探索関数設定処理
 */
static E_SC_RESULT setRoutingFunc() {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SCRP_MANAGER *rpManager = RPC_GetRouteCalcManager();

	// 規制情報判定関数設定
	result = RC_RegJudgeFuncSet(rpManager->rcSetting);
	if (e_SC_RESULT_SUCCESS != result) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "RC_RegJudgeFuncSet error. [0x%08x] "HERE, result);
		return (result);
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief リルート処理準備
 * @param 区間管理
 * @memo １．前回経路のうちリルート対象の区間以降の区間情報をコピーする
 *       ２．出発近傍・目的近傍 情報生成
 */
static E_SC_RESULT reRouteDataStandby(SCRP_SECTCONTROLER* aSectCtrl) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aSectCtrl) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	SC_RP_RouteMng* routeMng = NULL;
	UINT32 routeId = 0;
	UINT32 routeType = 0;

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SCRP_MANAGER *rcManager = RPC_GetRouteCalcManager();
	SCRP_SEARCHSETTING* setting = rcManager->rcSetting;
	SC_RP_LinkInfo* lastLinkInfo = NULL;
	SC_RP_ParcelInfo* lastPclInfo = NULL;
	SC_RP_SectInfo* sectInfo = NULL;
	UINT16 nextSect = 0;				// 次区間
	UINT16 saveSectVol = 0;				// 保存区間数 = 条件区間 - 開始区間 - 1
	UINT16 crntSect = 0;				// 現区間
	SCRP_NEIGHBORLINK* destNeigbor = NULL;
	SCRP_NEIGHBORINFO* stNeigborInfo = &rcManager->neighbor[0];
	SCRP_NEIGHBORINFO* edNeigborInfo = &rcManager->neighbor[1];

	do {
		// 経路ID取得
		result = SC_RP_GetCurrentRouteId(&routeId, &routeType);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RM, "SC_RP_GetCurrentRouteId error. [0x%08x] "HERE, result);
			break;
		}
		// 経路参照登録
		result = SC_RP_ReadRouteEntry(routeId, routeType, SC_RP_USER_RP, &routeMng);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RM, "SC_RP_ReadRouteEntry error. [0x%08x] "HERE, result);
			break;
		}

		// 出発地近傍情報を車両状態から作成
		result = RC_NeighborCarPoint(rcManager->rcSetting, stNeigborInfo);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RC_NeighborCarPoint error. [0x%08x] "HERE, result);
			break;
		}

		// 次以降の残り区間数
		saveSectVol = (setting->pointNum - 1) - (setting->replanSect + 1);
		// 次区間インデックス
		nextSect = setting->replanSect + 1;
		// 現区間インデックス
		crntSect = setting->replanSect;

		// 更新区間の始終端リンク取得
		if (0 < saveSectVol) {

			// 推奨経路保存
			result = RC_BackupReleasedRoute(routeMng, nextSect, saveSectVol);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RM, "RC_SaveLatestRoute error. [0x%08x] "HERE, result);
				break;
			}

			// 現区間管理情報取得
			sectInfo = routeMng->sectInfo + crntSect;

			// 現区間最終リンク取得
			lastPclInfo = routeMng->parcelInfo + (sectInfo->parcelIdx + sectInfo->parcelVol - 1);
			lastLinkInfo = routeMng->linkInfo + (sectInfo->linkIdx + sectInfo->linkVol - 1);

			// 目的地近傍リンクへ設定
			destNeigbor = (SCRP_NEIGHBORLINK*) RP_MemAlloc(sizeof(SCRP_NEIGHBORINFO), e_MEM_TYPE_ROUTEPLAN);
			if (NULL == destNeigbor) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error. "HERE);
				result = e_SC_RESULT_MALLOC_ERR;
				break;
			}
			RP_Memset0(destNeigbor, sizeof(SCRP_NEIGHBORINFO));
			// 近傍情報設定（目的地リンクは1本のみで確定させる）
			edNeigborInfo->nbrLinkVol = 1;
			edNeigborInfo->neighborLink = destNeigbor;

			// 近傍情報作成（目的地は現区間インデックス＋１の地点情報）
			result = RC_ReRouteNbrMake(&setting->rpPoint[crntSect + 1], lastPclInfo->parcelId, lastLinkInfo->linkId, lastLinkInfo->orFlag,
					edNeigborInfo);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "RC_ReRouteNbrMake error. [0x%08x] "HERE, result);
				break;
			}
		} else {
			// 現区間＝最終区間
			result = RC_NeighborDSide(setting, nextSect, edNeigborInfo);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "RC_NeighborDSide error. [0x%08x] "HERE, result);
				break;
			}
		}
		// 探索条件設定
		result = setNextCond(crntSect, aSectCtrl);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "setNextCond. [0x%08x] "HERE, result);
			break;
		}
		RP_Memcpy(&(aSectCtrl->neighbor[SCRP_NBROWN]), stNeigborInfo, sizeof(SCRP_NEIGHBORINFO));
		RP_Memcpy(&(aSectCtrl->neighbor[SCRP_NBRDST]), edNeigborInfo, sizeof(SCRP_NEIGHBORINFO));

		// 探索処理区間は現区間のみ
		rcManager->sectVol = 1;

	} while (0);

	// メモリ解放
	if (e_SC_RESULT_SUCCESS != result) {
		if (NULL != stNeigborInfo->neighborLink) {
			RP_MemFree(stNeigborInfo->neighborLink, e_MEM_TYPE_ROUTEPLAN);
			stNeigborInfo->neighborLink = NULL;
		}
		if (NULL != edNeigborInfo->neighborLink) {
			RP_MemFree(edNeigborInfo->neighborLink, e_MEM_TYPE_ROUTEPLAN);
			edNeigborInfo->neighborLink = NULL;
		}
		stNeigborInfo->nbrLinkVol = 0;
		edNeigborInfo->nbrLinkVol = 0;
	}

	// 経路参照終了
	if (NULL != routeMng) {
		SC_RP_ReadRouteExit(routeId, routeType, SC_RP_USER_RP);
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief 探索条件を区間情報へ設定する(探索条件不正値チェックを兼ねる)
 * @param 区間インデックス
 * @param 区間管理
 */
static E_SC_RESULT setNextCond(INT16 aSectIdx, SCRP_SECTCONTROLER* aSectCtrl) {

	SCRP_MANAGER *rcManager = RPC_GetRouteCalcManager();
	if (NULL == aSectCtrl || NULL == rcManager) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	UINT32 cond = rcManager->rcSetting->point[aSectIdx].cond;
	switch (cond) {
	case RPC_HIGHWAY:
		aSectCtrl->routeCond = cond;
		break;
	case RPC_NORMAL:
		aSectCtrl->routeCond = cond;
		break;
	case RPC_TIME:
		aSectCtrl->routeCond = cond;
		break;
	case RPC_DISTANCE:
		aSectCtrl->routeCond = cond;
		break;
	default:
		SC_LOG_ErrorPrint(SC_TAG_RC, "rpCond error. %d "HERE, cond);
		return (e_SC_RESULT_FAIL);
	}
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 次回探索開始リンク採択処理
 * @param 近傍テーブル
 * @param 区間インデックス
 * @memo 前区間の推奨経路（最終リンク）を次回探索開始リンクとして採択
 *       ≠近傍リンクです
 *       近傍情報は推奨経路生成時に必要である為ここで再度近傍情報を生成しています。
 *       初回に生成した近傍情報を使用しない理由は、行き止まりリンク対応により次回開始リンクが近傍情報に含まれていない可能性があるためです。
 *       TODO 行き止まりリンクであることを考慮し、推奨経路２リンクまでを近傍リンクとして採択する
 *            採択リンク数は要調整。あくまで応急処置であることを意識すること
 */
static E_SC_RESULT setNextStartLink(SCRP_NEIGHBORINFO* aNeighbor, INT16 aSectIdx) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SC_RP_RouteMng* routeMng = NULL;
	SCRP_NEIGHBORLINK* newNeighbor = NULL;
	SCRP_MANAGER *rpManager = RPC_GetRouteCalcManager();
	UINT32 i;
	INT32 u;
	UINT16 setVol = 0;
	SC_RP_SectInfo *sectInfo = NULL;

	if (NULL == aNeighbor || NULL == rpManager) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	do {
		// 次回開始近傍領域
		newNeighbor = (SCRP_NEIGHBORLINK*) RP_MemAlloc(sizeof(SCRP_NEIGHBORLINK) * RP_NEXTSTARTLINK_VOL, e_MEM_TYPE_ROUTEPLAN);
		if (NULL == newNeighbor) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "malloc error."HERE);
			result = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		RP_Memset0(newNeighbor, sizeof(SCRP_NEIGHBORLINK) * RP_NEXTSTARTLINK_VOL);

		// 推奨経路情報を取得する
		routeMng = RC_GetMasterRoute();
		sectInfo = &routeMng->sectInfo[aSectIdx - 1];

#if _RP_ROUTE_LV2SPLITCHECKADDWP
		// 断裂の場合探索失敗
		if (ALL_F16 != sectInfo->splitIdx) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "latest route is split. "HERE);
			result = e_SC_RESULT_FAIL;
			break;
		}
		// レベル2の場合探索失敗
		for (i = 0; i < sectInfo->parcelVol; i++) {
			if (RP_LEVEL1 != (routeMng->parcelInfo + i)->level) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "latest route have lv2 parcel. "HERE);
				result = e_SC_RESULT_FAIL;
			}
		}
		if (e_SC_RESULT_SUCCESS != result) {
			break;
		}
#endif

		for (i = 0; i < RP_NEXTSTARTLINK_VOL; i++) {
			// 前回区間に特定本数以上リンクが存在すること
			if (RP_NEXTSTARTLINK_VOL > sectInfo->linkVol) {
				continue;
			}

			SC_RP_LinkInfo* link = (routeMng->linkInfo + (routeMng->linkVol - i - 1));
			SC_RP_ParcelInfo *pclInfo = NULL;
			UINT16 total = 0;
			// 条件を満たすまでパーセルテーブル逆追い
			for (u = (sectInfo->parcelVol - 1); 0 <= u; u--) {
				pclInfo = routeMng->parcelInfo + (sectInfo->parcelIdx + u);
				total += pclInfo->linkVol;
				if (total > i) {
					break;
				}
			}
			if (0 > u) {
				SC_LOG_InfoPrint(SC_TAG_RC, "few target link vol... "HERE);
				continue;
			}

			// 該当リンクの近傍情報を生成する
			SCRP_NEIGHBORLINK neighborLink = {};
			result = RP_MakeNeighborLinkInfo(&rpManager->neighbor[aSectIdx].point, pclInfo->parcelId, link->linkId, &neighborLink);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "make neighbor link error."HERE);
				break;
			}

			// 初期コストは新区間の為０にリセット+方向格納
			neighborLink.cost = i * 100000;	// TODO ２本目以降にコスト付加
			neighborLink.orFlag = link->orFlag;

			// 結果格納
			RP_Memcpy(&newNeighbor[i], &neighborLink, sizeof(SCRP_NEIGHBORLINK));
			setVol++;
		}
		if (e_SC_RESULT_SUCCESS == result) {
			aNeighbor->neighborLink = newNeighbor;
			aNeighbor->nbrLinkVol = setVol;
			aNeighbor->point.parcelId = rpManager->neighbor[aSectIdx].point.parcelId;
			aNeighbor->point.x = rpManager->neighbor[aSectIdx].point.x;
			aNeighbor->point.y = rpManager->neighbor[aSectIdx].point.y;
		}
	} while (0);

	if (e_SC_RESULT_SUCCESS != result) {
		if (NULL != newNeighbor) {
			RP_MemFree(newNeighbor, e_MEM_TYPE_ROUTEPLAN);
		}
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief 共有メモリ登録：所要時間、距離、高速距離
 * @param 経路管理
 */
static E_SC_RESULT setRouteHeadInfo(SC_RP_RouteMng* aRouteMng) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SC_RP_SectInfo *sectInfo = NULL;
	UINT32 totalDist = 0;
	UINT32 totalHwaydist = 0;
	UINT32 totalTime = 0;
	UINT32 i = 0;

	if (NULL == aRouteMng) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	do {
		// 経路総距離、所要時間、高速道路総距離を計算
		sectInfo = aRouteMng->sectInfo;
		for (i = 0; i < aRouteMng->sectVol; i++, sectInfo++) {
			totalDist += sectInfo->sectDist;
			totalTime += sectInfo->sectTime;
			totalHwaydist += sectInfo->sectHWDist;
		}

		// 共有メモリへ経路総距離登録
		result = SC_MNG_SetRouteLength(totalDist);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "SC_MNG_SetRouteLength error. [0x%08x] "HERE, result);
			break;
		}
		// 共有メモリへ経路所要時間登録
		result = SC_MNG_SetRouteAveTime(totalTime);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "SC_MNG_SetRouteAveTime error. [0x%08x] "HERE, result);
			break;
		}
		// 共有メモリへ経路高速道路総距離登録
		result = SC_MNG_SetRouteHwayLength(totalHwaydist);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "SC_MNG_SetRouteHwayLength error. [0x%08x] "HERE, result);
			break;
		}
	} while (0);
#if 0
	SC_LOG_InfoPrint(SC_TAG_RC, "Route Information. routeType=%d dist=%4d hway=%4d time=%4d ", aRouteMng->routeType, totalDist,
			totalHwaydist, totalTime);
#endif

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief 全地点に対する近傍情報の生成を行う
 * @param 探索管理
 */
static E_SC_RESULT getAllNeighborLinks(SCRP_MANAGER *aRcManager) {

	if (NULL == aRcManager) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	UINT32 i;
	for (i = 0; i < aRcManager->rcSetting->pointNum; i++) {
		if (0 == i) {
			result = RC_NeighborOSide(aRcManager->rcSetting, i, &aRcManager->neighbor[i]);
		} else {
			if (PLACE_POI_GATE == aRcManager->rcSetting->point[i].placeType) {
				result = RC_NeighborDSidePoiGate(aRcManager->rcSetting, i, &aRcManager->neighbor[i]);
			} else {
				result = RC_NeighborDSide(aRcManager->rcSetting, i, &aRcManager->neighbor[i]);
			}
		}
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RC_NeighborDSide [0x%08x] "HERE, result);
			break;
		}
	}
	return (result);
}

/**
 * @brief 推奨経路区間番号設定処理
 * @param 推奨経路
 * @param 開始区間番号
 */
static E_SC_RESULT routeSectNumberUpdate(SC_RP_RouteMng* aRouteMng, UINT16 aStartNumber) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aRouteMng) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	UINT16 number = aStartNumber;
	UINT32 i;
	for (i = 0; i < aRouteMng->sectVol; i++) {
		(aRouteMng->sectInfo + i)->sectNumber = number;
		number++;
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 探索可能距離判定
 * @param 探索設定パラメタ
 */
static E_SC_RESULT checkLimitedDistance(SCRP_SEARCHSETTING *aSearchSetting) {

	DOUBLE totalDist = 0;
	UINT32 i;

	if (NULL == aSearchSetting || 2 > aSearchSetting->pointNum) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 地点間直線距離合算
	for (i = 0; i < aSearchSetting->pointNum - 1; i++) {
		totalDist += RP_Lib_CalcODLength(aSearchSetting->rpPoint[i], aSearchSetting->rpPoint[i + 1]);
	}

	// 探索可能距離チェック
	if (RP_LIMITED_DISTANCE < (UINT32) lround(totalDist)) {
		// エラーコードを設定する
		RPC_SetErrorCode(EC_LIMITED_DISTANCE_OVER);
		SC_LOG_ErrorPrint(SC_TAG_RC, "RP_LIMITED_DISTANCE %d < %f "HERE, RP_LIMITED_DISTANCE, totalDist);
		return (e_SC_RESULT_FAIL);
	}

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 区間探索終了処理
 * @param 探索情報管理
 */
static void sectionFinal(SCRP_SECTCONTROLER* aSectCtrl, Bool aIsRetry) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	// ◆Area系
	if (NULL != aSectCtrl->levelTable.pclState && 0 != aSectCtrl->levelTable.pclStateVol) {
		RP_MemFree(aSectCtrl->levelTable.pclState, e_MEM_TYPE_ROUTEPLAN);
	}
	if (NULL != aSectCtrl->levelTable.divInfo && 0 != aSectCtrl->levelTable.divInfoVol) {
		RP_MemFree(aSectCtrl->levelTable.divInfo, e_MEM_TYPE_ROUTEPLAN);
	}
	RP_Memset0(&aSectCtrl->levelTable, sizeof(SCRP_LEVELTBL));

	// ◆Network系
	if (NULL != aSectCtrl->netTable.parcelInfo) {
		RP_MemFree(aSectCtrl->netTable.parcelInfo, e_MEM_TYPE_ROUTEPLAN);
	}
	if (NULL != aSectCtrl->netTable.linkTable) {
		RP_MemFree(aSectCtrl->netTable.linkTable, e_MEM_TYPE_ROUTEPLAN);
	}
	if (NULL != aSectCtrl->netTable.heap.heap) {
		RP_MemFree(aSectCtrl->netTable.heap.heap, e_MEM_TYPE_ROUTEPLAN);
	}
	RP_Memset0(&aSectCtrl->netTable, sizeof(SCRP_NETCONTROLER));

	// ◆Cand系
	if (NULL != aSectCtrl->candMng.cand) {
		RP_MemFree(aSectCtrl->candMng.cand, e_MEM_TYPE_ROUTECAND);
	}
	if (NULL != aSectCtrl->candMng.stLink) {
		RP_MemFree(aSectCtrl->candMng.stLink, e_MEM_TYPE_ROUTECAND);
	}
	if (NULL != aSectCtrl->candMng.splitCand) {
		RP_MemFree(aSectCtrl->candMng.splitCand, e_MEM_TYPE_ROUTECAND);
	}
	if (NULL != aSectCtrl->candMng.splitStLink) {
		RP_MemFree(aSectCtrl->candMng.splitStLink, e_MEM_TYPE_ROUTECAND);
	}
	if (NULL != aSectCtrl->candMng.splitCandTblInfo) {
		RP_MemFree(aSectCtrl->candMng.splitCandTblInfo, e_MEM_TYPE_ROUTECAND);
	}
	RP_Memset0(&aSectCtrl->candMng, sizeof(SCRP_CANDMANAGER));

	if (!aIsRetry) {
		if (0 != aSectCtrl->sectIndex && NULL != aSectCtrl->neighbor[SCRP_NBROWN].neighborLink) {
			RP_MemFree(aSectCtrl->neighbor[SCRP_NBROWN].neighborLink, e_MEM_TYPE_ROUTEPLAN);
			aSectCtrl->neighbor[SCRP_NBROWN].neighborLink = NULL;
		}
		// 区間管理クリア
		RP_Memset0(aSectCtrl, sizeof(SCRP_SECTCONTROLER));
	}
	// 地図全開放
	RC_MapFreeAll();

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
}

/**
 * @brief 探索管理情報クリア処理
 * @param 探索管理
 */
static void routePlanFinal(SCRP_MANAGER *aRcManager) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aRcManager) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return;
	}
	UINT32 i;

	// 近傍情報領域解放
	for (i = 0; i < RP_ROUTE_PLACE_MAX; i++) {
		if (NULL != aRcManager->neighbor[i].neighborLink) {
			RP_MemFree(aRcManager->neighbor[i].neighborLink, e_MEM_TYPE_ROUTEPLAN);
		}
	}
	// 管理情報クリア
	RP_Memset0(aRcManager, sizeof(SCRP_MANAGER));
	// 推奨経路作業管理領域クリア
	RC_FinalRouteMakeTbl();
	// 地図全開放
	RC_MapFreeAll();

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
}

/**
 * @brief 推奨経路用経路IDカウント
 */
static void routeIdCountUp() {
	// 無効地なら初期化
	if (0 == mRouteID || (SC_RP_RTIDINIT - 1) <= mRouteID) {
		mRouteID = 1;
	} else {
		// 更新
		mRouteID++;
	}
}

/**
 * @brief 推奨経路登録処理
 * @param [I/O]探索情報管理
 * @memo 作成済みの推奨経路を公開経路領域へ登録する
 */
static E_SC_RESULT releaseMasterRoute() {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SCRP_MANAGER *rcManager = RPC_GetRouteCalcManager();
	SC_RP_RouteMng* newRouteMng = NULL;
	SC_RP_RouteMng* masterRouteMng = RC_GetMasterRoute(); // 作成済みの経路管理

#if _RP_CHECK_ROUTE
	RPDBG_CheckIntegrityRoute(masterRouteMng);
#endif

	do {
		if (NULL == rcManager || NULL == masterRouteMng) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
			result = e_SC_RESULT_BADPARAM;
			break;
		}
		// 新経路管理領域取得
		newRouteMng = (SC_RP_RouteMng*) RP_MemAlloc(sizeof(SC_RP_RouteMng), e_MEM_TYPE_ROUTEMNG);
		if (NULL == newRouteMng) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error. "HERE);
			result = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// 公開用に情報コピー
		RP_Memcpy(newRouteMng, masterRouteMng, sizeof(SC_RP_RouteMng));
		// 経路種別設定
		newRouteMng->routeType = rcManager->rpType;
		// 区間番号付与
		result = routeSectNumberUpdate(newRouteMng, rcManager->rcSetting->replanSect);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "routeSectNumberUpdate error. [0x%08x] "HERE, result);
			break;
		}

		// 断裂経路判定 断裂経路があれば エラーコードを立てる
		if (RC_CheckSplitRoute(newRouteMng)) {
			RPC_SetWarnCode(EC_ROUTE_SPLIT);
		}

		// TODO 経路の整合性チェック
		// 断裂フラグの有無

		// 経路公開登録
		result = SC_RP_RouteAdd(mRouteID, rcManager->rpType, newRouteMng);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "SC_RP_RouteAdd error. id=%d type=%d [0x%08x] "HERE, mRouteID, rcManager->rpType, result);
			break;
		}

		// 推奨経路ヘッダ情報を共有メモリに登録
		result = setRouteHeadInfo(masterRouteMng);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "setRouteHeadInfo error. [0x%08x] "HERE, result);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief リトライ対象か判断する
 * @param 区間管理
 * @memo 設定される可能性のあるエラーコードはキャンセル・同一リンク・直線距離抑止であり、これらの場合リトライ不要
 */
static Bool judgeRetryCalc(SCRP_SECTCONTROLER* aSectCtrl) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aSectCtrl) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (false);
	}
	// トップレベルが１の場合のみリトライ
	if (RP_LEVEL2 == aSectCtrl->levelTable.topLevel) {
		return (false);
	}
	// キャンセル・同一リンクはリトライしない
	INT32 errorCode = RPC_GetErrorCode();
	switch (errorCode) {
	case EC_CANCEL:
		return (false);
	case EC_SAME_ROAD:
		return (false);
	case EC_LIMITED_DISTANCE_OVER:
		return (false);
	default:
		return (true);
	}
}
