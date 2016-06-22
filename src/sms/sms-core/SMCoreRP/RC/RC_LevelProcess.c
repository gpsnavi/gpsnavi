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
 * RP_LevelProcess.c
 *
 *  Created on: 2015/11/06
 *      Author: masutani
 */

#include "sms-core/SMCoreRP/SMCoreRPInternal.h"

static E_SC_RESULT calcRouteStep(SCRP_SECTCONTROLER* aSectCtrl, E_RP_RTCALCSTEP aStep);
static void RC_SplitCalcFinal(SCRP_NETCONTROLER* aNetTab);

/**
 * @brief レベル間探索処理
 * @param 区間管理情報
 */
E_SC_RESULT RC_LevelProcess(SCRP_SECTCONTROLER *aSectCtrl) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT result = e_SC_RESULT_FAIL;
	E_RP_RTCALCSTEP stepScenario[e_RP_STEP_END] = {};

	if (NULL == aSectCtrl) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	aSectCtrl->netTable.levelTbl = &aSectCtrl->levelTable;

	do {
		// 探索条件に対応する関数を設定する
		result = RC_CostCalcFuncSet(aSectCtrl->routeCond);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RC_CostCalcFuncSet error. [0x%08x] "HERE, result);
			break;
		}
		// 初期処理
		result = RC_CandMngInit(&aSectCtrl->candMng);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RC_CandMngInit error. [0x%08x] "HERE, result);
			break;
		}

		// 近傍情報へ地図データインデックスを設定する：O側
		result = RC_SetNbrLinkIndex(&aSectCtrl->neighbor[0]);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RC_SetNbrLinkIndex error. [0x%08x] "HERE, result);
			break;
		}
		// 近傍情報へ地図データインデックスを設定する：D側
		result = RC_SetNbrLinkIndex(&aSectCtrl->neighbor[1]);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RC_SetNbrLinkIndex error. [0x%08x] "HERE, result);
			break;
		}
		// 近傍情報の同一リンク判定＆同一リンク存在時不使用フラグ設定
		result = RC_SetNeighborLinkUseFlg(aSectCtrl);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RC_SetNeighborLinkUseFlg error. [0x%08x] "HERE, result);
			break;
		}
#if _RP_LOG_NEIGBORLINK
		RPDBG_ShowNeigborLink(aSectCtrl);
#endif

		// ステップシナリオを作成
		switch (aSectCtrl->levelTable.topLevel) {
		case RP_LEVEL1:
			stepScenario[0] = e_RP_STEP_LV1TOP;
			stepScenario[1] = e_RP_STEP_END;
			break;
		case RP_LEVEL2:
			stepScenario[0] = e_RP_STEP_LV1O;
			stepScenario[1] = e_RP_STEP_LV2TOP;
			stepScenario[2] = e_RP_STEP_LV1D;
			stepScenario[3] = e_RP_STEP_END;
			break;
		default:
			stepScenario[0] = e_RP_STEP_END;
			result = e_SC_RESULT_BADPARAM;
			break;
		}

		// 探索ステップ別
		UINT16 stepIdx = 0;
		while (e_RP_STEP_END != stepScenario[stepIdx]) {

#if _RP_LOG_RTCALCSTEP
			RPDBG_ShowStepLog(aSectCtrl, stepScenario[stepIdx]);
#endif

			switch(stepScenario[stepIdx]){
			case e_RP_STEP_LV1TOP:
				aSectCtrl->netTable.levelArea = &aSectCtrl->levelTable.areaTable[0];
				break;
			case e_RP_STEP_LV1O:
				aSectCtrl->netTable.levelArea = &aSectCtrl->levelTable.areaTable[0];
				break;
			case e_RP_STEP_LV1D:
				aSectCtrl->netTable.levelArea = &aSectCtrl->levelTable.areaTable[2];
				break;
			case e_RP_STEP_LV2TOP:
				aSectCtrl->netTable.levelArea = &aSectCtrl->levelTable.areaTable[1];
				break;
			}

			// 分割候補初期化
			result = RC_SplitCandMngInit(&aSectCtrl->candMng, aSectCtrl->netTable.levelArea->divVol);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "RC_SplitCandMngInit error. [0x%08x] "HERE, result);
				break;
			}
			result = calcRouteStep(aSectCtrl, stepScenario[stepIdx]);
			if (e_SC_RESULT_ROUTE_CANCEL == result) {
				break;
			} else if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "calc step error. [0x%08x] "HERE, result);
				break;
			}
#if _RP_LOG_CANDINFO
			RPDBG_ShowCandInfo(aSectCtrl, stepScenario[stepIdx], 0);
#endif
			// 分割候補解放
			RC_SplitCandFree(&aSectCtrl->candMng);
			stepIdx++;

#if _RP_CHECK_CAND
			RPDBG_CheckIntegrityCand(aSectCtrl);
#endif

		}
	} while (0);

	if (e_SC_RESULT_SUCCESS != result) {
		// 候補経路解放
		RC_CandFree(&aSectCtrl->candMng);
	}
	// 探索情報テーブル及び地図解放
	RC_SplitCalcFinal(&aSectCtrl->netTable);

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief ステップを探索する
 * @param 区間管理情報
 * @param 実行ステップ
 * @param レベルエリア情報
 */
static E_SC_RESULT calcRouteStep(SCRP_SECTCONTROLER* aSectCtrl, E_RP_RTCALCSTEP aStep) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	UINT32 split = 0;

	if (NULL == aSectCtrl) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	do {
		// 分割エリア数登録
		RPC_SetTotalSplit(aSectCtrl->netTable.levelArea->divVol);

		for (split = 0; split < aSectCtrl->netTable.levelArea->divVol; split++) {
			if (true == RPC_IsCancelRequest()) {
				result = e_SC_RESULT_ROUTE_CANCEL;
				SC_LOG_DebugPrint(SC_TAG_RC, "RPC_IsCancelRequest cancel...");
				break;
			}

#if _RP_LOG_RTCALCSTEP
			// 分割候補経路関連ダンプ
			RPDBG_ShowCandSizeInfo(aSectCtrl, aStep, split);
#endif

			// 分割エリア登録
			RPC_SetCalcSplit(split);

			// 分割情報設定
			aSectCtrl->netTable.calculatingDivIdx = split;

			// ネットワーク生成
			result = RC_MakeNetwork(&aSectCtrl->netTable, aSectCtrl, aStep);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "RC_NetMake error. [0x%08x] "HERE, result);
				break;
			}

			// 最終ステップである場合、目的地リンクにフラグを立てる TODO 要確認
			if (e_RP_STEP_LV1D == aStep || e_RP_STEP_LV1TOP == aStep) {
				result = RC_SetDestLinkFlag(&aSectCtrl->netTable, &aSectCtrl->neighbor[SCRP_NBRDST]);
				if (e_SC_RESULT_SUCCESS != result) {
					SC_LOG_ErrorPrint(SC_TAG_RC, "RC_SetDestLinkFlag error. [0x%08x] "HERE, result);
					break;
				}
			}
			// heap
			result = RC_MemAllocHeapTable(&aSectCtrl->netTable);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "RC_HeapTableMemAlloc error. [0x%08x] "HERE, result);
				break;
			}

			// 探索開始リンクをヒープへ格納
			result = RC_SetStartLink(aSectCtrl, aStep, split);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "RC_SetStartLink error. [0x%08x] "HERE, result);
				break;
			}
			// Dijkstra
			result = RC_DepthFirstDijkstra(&aSectCtrl->netTable);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "RC_Dijkstra error. [0x%08x] "HERE, result);
				break;
			}
			// Cand
			result = RC_CandMake(aSectCtrl, &aSectCtrl->netTable, aStep);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "RC_CandMake error. [0x%08x] "HERE, result);
				break;
			}
			// 探索情報テーブル及び地図解放
			RC_SplitCalcFinal(&aSectCtrl->netTable);
		}
		// 候補経路の更新
		if (e_SC_RESULT_SUCCESS == result) {
			result = RC_CandPromotion(&aSectCtrl->candMng, aStep);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "RC_CandPromotion error. [0x%08x] "HERE, result);
				break;
			}
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief 探索情報テーブル初期化及び地図解放処理
 * @param aNetTab
 */
static void RC_SplitCalcFinal(SCRP_NETCONTROLER* aNetTab) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	// 地図解放
	RC_MapFreeAll();

	if (NULL == aNetTab) {
		return;
	}
	// ParcelInfo
	if (NULL != aNetTab->parcelInfo) {
		RP_MemFree(aNetTab->parcelInfo, e_MEM_TYPE_ROUTEPLAN);
		aNetTab->parcelInfo = NULL;
	}
	aNetTab->parcelInfoVol = 0;

	// LinkTab
	if (NULL != aNetTab->linkTable) {
		RP_MemFree(aNetTab->linkTable, e_MEM_TYPE_ROUTEPLAN);
		aNetTab->linkTable = NULL;
	}
	aNetTab->linkTableVol = 0;

	// Heap
	if (NULL != aNetTab->heap.heap) {
		RP_MemFree(aNetTab->heap.heap, e_MEM_TYPE_ROUTEPLAN);
		aNetTab->heap.heap = NULL;
	}
	aNetTab->heap.heapSize = 0;
	aNetTab->heap.heapEnd = 0;

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
}
