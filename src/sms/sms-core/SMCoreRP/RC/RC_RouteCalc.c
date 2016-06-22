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
 * RP_RouteCalc.c
 *
 *  Created on: 2015/11/06
 *      Author: masutani
 */

#include "sms-core/SMCoreRP/SMCoreRPInternal.h"

typedef E_SC_RESULT (*FIND_STARTNET_FUNC)(SCRP_CANDSTARTLINK* aCandStLink, SCRP_NETCONTROLER* aNetCtrl, UINT32* aLinkIdx);

static E_SC_RESULT makeClossLinkTable(SCRP_NETCONTROLER* aNetCtrl, UINT32 aHeap, SCRC_CROSSLINKTBL* aLinkList);
static E_SC_RESULT getNextPclConnectLink(SCRP_NETCONTROLER* aNetCtrl, SCRP_LINKINFO *aLinkInfo, SCRC_TARGETLINKINFO* aTarget);
static E_SC_RESULT calcRouteFeatureCost(SCRP_NETCONTROLER* aNetCtrl, SCRC_CROSSLINKTBL* aCrossLinkTbl);
static E_SC_RESULT findStartNetwork(SCRP_CANDSTARTLINK* aCandStLink, SCRP_NETCONTROLER* aNetCtrl, UINT32* aLinkIdx);
static E_SC_RESULT findStartNetworkLv1D(SCRP_CANDSTARTLINK* aCandStLink, SCRP_NETCONTROLER* aNetCtrl, UINT32* aLinkIdx);
static E_SC_RESULT findStartNetworkLv2Top(SCRP_CANDSTARTLINK* aCandStLink, SCRP_NETCONTROLER* aNetCtrl, UINT32* aLinkIdx);

static const UINT8 mOdrRevIdx[2] = { 1, 0 };
static const UINT8 mOnewayFlg[2] = { 0x02, 0x01 };

/**
 * @brief ダイクストラ法経路探索
 * @param ネットワーク管理
 */
E_SC_RESULT RC_DepthFirstDijkstra(SCRP_NETCONTROLER* aNetCtrl) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);
#if _RPLAPTIME_DIJKSTRA // ★時間計測
	RP_SetLapTime();
#endif

	SCRC_CROSSLINKTBL crossLinkTbl = {}; // 計算対象交差点情報
	E_SC_RESULT result = e_SC_RESULT_SUCCESS;

	// プロセス登録
	RPC_SetCalcProcess(e_RC_PROC_ROUTECALC);

	if (NULL == aNetCtrl) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// DIJKSTRAメインループ
	while (aNetCtrl->heap.heapEnd) {

		// 交差点情報生成
		result = makeClossLinkTable(aNetCtrl, aNetCtrl->heap.heap[0], &crossLinkTbl);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RC_NextCalcLinkMake error. [0x%08x] "HERE, result);
			break;
		}

		// 先頭削除
		RC_HeapDelete(aNetCtrl, 0);
		result = calcRouteFeatureCost(aNetCtrl, &crossLinkTbl);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "calcRouteFeatureCost error. [0x%08x] "HERE, result);
			break;
		}
	}

#if _RPLAPTIME_DIJKSTRA // ★時間計測
	RP_SetLapTimeWithStr("dijkstra.");
#endif

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief 次回、探索対象リンク収集
 * @param ネットワーク管理
 * @param ヒープ
 * @param [O]交差点リンク情報(計算用)
 */
static E_SC_RESULT makeClossLinkTable(SCRP_NETCONTROLER *aNetCtrl, UINT32 aHeap, SCRC_CROSSLINKTBL* aCalcData) {

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SCRP_LINKINFO* baseLink = NULL; // ループストッパー
	SCRP_NETDATA* baseNet = NULL;
	SCRP_NETDATA* preNetData = NULL;
	SCRC_TARGETLINKINFO* target = &aCalcData->linkList[0];
	UINT32 i;

	// 次計算対象の基準リンクデータ格納
	baseLink = RCNET_GET_LINKINFO(aNetCtrl, aHeap);
	baseNet = (baseLink->linkNet + RCID_GET_ORIDX(aHeap));

	// 基準リンクデータ格納
	aCalcData->listVol = 0;
	aCalcData->baseLink.linkIndex = aHeap;
	aCalcData->baseLink.linkTable = baseLink;
	aCalcData->baseLink.linkNet = baseNet;
	// パーセル情報取得(ヒープに接続情報は格納されないので自身のPclInfoを積む)
	aCalcData->baseLink.pclInfo = RCNET_GET_NEXTPCLINFO(aNetCtrl, baseNet);
	// コスト引き継ぎ
	aCalcData->baseCost = baseNet->costSum;

	// ヒープのリンク逆方向を事前リンクに設定
	preNetData = (baseLink->linkNet + mOdrRevIdx[RCID_GET_ORIDX(aHeap)]);

	//接続先パーセル情報テーブル取得
	for (i = 0; i < RP_CLOSS_MAX; i++) {

		SCRP_LINKINFO* linkInfo = RCNET_GET_NEXTLINKINFO(aNetCtrl, preNetData);
		SCRP_NETDATA* netData = linkInfo->linkNet + RCNET_GET_NEXTORIDX(preNetData);

		// リンクor接続ID
		if (RCND_GET_CONNECTLINK(netData->flag)) {
			// 接続情報の場合、接続先のリンク情報を取得
			result = getNextPclConnectLink(aNetCtrl, linkInfo, target);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "RC_GetNextLink error. next=0x%08x "HERE, netData->nextLink);
				break;
			}
		} else {
			target->pclInfo = RCNET_GET_NEXTPCLINFO(aNetCtrl, preNetData);
			target->linkTable = linkInfo;
			target->linkNet = netData;
			target->linkIndex = preNetData->nextLink;
		}
		// 同一リンクで終了
		if (linkInfo == baseLink) {
			break;
		}
		// save
		preNetData = netData;
		// update
		aCalcData->listVol++;
		target++;
	}

	return (result);
}

/**
 * @brief 図郭リンク専用接続先リンク情報取得
 * @param ネットワーク管理
 * @param 基準リンク情報
 * @param [O]計算対象リンクテーブル
 */
static E_SC_RESULT getNextPclConnectLink(SCRP_NETCONTROLER* aNetCtrl, SCRP_LINKINFO *aLinkInfo, SCRC_TARGETLINKINFO* aTarget) {

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SCRP_PCLINFO* nextPclInfo = NULL;
	SCRP_NETDATA* preNetData = NULL;
	SCRP_LINKINFO* startLinkInfo = NULL; // ループストッパー
	UINT32 i;

	// 最外殻判定(次パーセルに接続する側) 注：pclInfoへはNULLを格納すること。
	if (ALL_F32 == aLinkInfo->linkNet[SCRP_LINKRVS].nextLink) {
		aTarget->pclInfo = NULL;
		aTarget->linkIndex = ALL_F32;
		aTarget->linkTable = aLinkInfo;
		aTarget->linkNet = &aLinkInfo->linkNet[SCRP_LINKODR];
		return (e_SC_RESULT_SUCCESS);
	}

	// 次パーセルの接続情報取得
	nextPclInfo = RCNET_GET_NEXTPCLINFO(aNetCtrl, &aLinkInfo->linkNet[SCRP_LINKRVS]);
	startLinkInfo = RCNET_GET_NEXTLINKINFO(aNetCtrl, &aLinkInfo->linkNet[SCRP_LINKRVS]);
	preNetData = &startLinkInfo->linkNet[SCRP_LINKODR];

	for (i = 0; i < RP_CLOSS_MAX; i++) {

		SCRP_LINKINFO* linkInfo = RCNET_GET_NEXTLINKINFO(aNetCtrl, preNetData);
		SCRP_NETDATA* netData = linkInfo->linkNet + RCNET_GET_NEXTORIDX(preNetData);

		// 接続ID情報以外であれば終了
		if (!RCND_GET_CONNECTLINK(netData->flag)) {
			aTarget->pclInfo = nextPclInfo;
			aTarget->linkTable = RCNET_GET_NEXTLINKINFO(aNetCtrl, preNetData);
			aTarget->linkNet = netData;
			aTarget->linkIndex = preNetData->nextLink;
			break;
		}

		// 接続関係が破綻している可能性。無限ループ防止
		if (linkInfo == startLinkInfo) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "can't find next connect link... %d,%d "HERE, aLinkInfo->detaIndex, linkInfo->detaIndex);
			result = e_SC_RESULT_FAIL;
			break;
		}

		// save
		preNetData = netData;
	}
	return (result);
}

/**
 * @brief 経路計算
 * @param ネットワーク管理
 * @param 交差点リンクテーブル
 * @memo 性能確保のためパラメタチェック等は行わない
 * @memo Uターンリンクは交差点テーブルへ格納しないように変更したため判定を取り除きました 20160108
 */
static E_SC_RESULT calcRouteFeatureCost(SCRP_NETCONTROLER* aNetCtrl, SCRC_CROSSLINKTBL* aCrossLinkTbl) {

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SCRC_IOCALCTBL inout = {};
	SCRC_RESULTCOSTS costs = {};
	SCRC_TARGETLINKINFO* target = &aCrossLinkTbl->linkList[0];
	UINT32 i;

	// InLinkデータ取得
	inout.inLink = &aCrossLinkTbl->baseLink;
	inout.inLinkBin = SC_MA_A_NWRCD_LINK_GET_RECORD(inout.inLink->pclInfo->mapNetworkLinkBin, inout.inLink->linkTable->detaIndex);

	// 差路分計算
	for (i = 0; i < aCrossLinkTbl->listVol; i++, target++) {

		// 探索対象外リンク
		if (RCND_GET_EXCLUDELINK(target->linkNet->flag)) {
			continue;
		}
		// 最外郭
		if (NULL == target->pclInfo) {
			continue;
		}

		// 退出(計算対象)リンク情報格納
		inout.outLink = target;
		inout.outLinkBin = SC_MA_A_NWRCD_LINK_GET_RECORD(target->pclInfo->mapNetworkLinkBin, target->linkTable->detaIndex);

		// 一通判定
		if (0 == (mOnewayFlg[RCID_GET_ORIDX(target->linkIndex)] & ~SC_MA_D_BASE_LINK_GET_ONEWAY(inout.outLinkBin))) {
			continue;
		}

		//TODO 規制判定
#if _RP_PATCH_RAIKAMUTURN /* 2015/07/31 ライカムパッチ */
		if (RC_JudgePatchReg(aNetCtrl, inout.inLink, inout.outLink)) {
			continue;
		}
#endif /* __RAIKAM_PATCH__ */

		// コスト算出
		result = RC_CostCalc(aNetCtrl, &inout, &costs);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RC_CostCalc error. in/out=%d/%d [0x%08x] "HERE, inout.inLink->linkIndex, inout.outLink->linkIndex,
					result);
			break;
		}

		// オーバーフローチェック
		if (ULONG_MAX < costs.totalCost + aCrossLinkTbl->baseCost) {
			SC_LOG_WarnPrint(SC_TAG_RC, "calc cost overflow... pcl=0x%08x link=0x%08x-0x%08x"HERE, target->pclInfo->parcelId,
					inout.inLink->linkIndex, inout.outLink->linkIndex);
			continue;
		}
		// 履歴登録＆コスト更新
		costs.totalCost += aCrossLinkTbl->baseCost;

#if 0	/* 探索コスト計算調査用ログ出力処理 確認したいパーセルID＋リンクIDを指定すると計算中の情報が表示されます。 */
		RPDBG_ShowTargetCalcNowLink(aNetCtrl, aCrossLinkTbl, 0x1abc13df, 0x280001ae, i, costs.totalCost, &costs);
#endif
		if (costs.totalCost < target->linkNet->costSum) {
			target->linkNet->inLinkHist = inout.inLink->linkIndex;
			target->linkNet->costSum = costs.totalCost;
			// ヒープ更新or追加
			if (SCRP_HEAP_UV == target->linkNet->heap) {
				RC_HeapInsert(aNetCtrl, target->linkIndex);
			} else {
				RC_HeapUpdate(aNetCtrl, target->linkIndex);
				// 開始リンクフラグはコスト更新の場合落とす
				target->linkNet->flag = RCND_MASK_STARTFLG(target->linkNet->flag);
			}
		}
	}

	return (result);
}

/**
 * @brief 探索開始リンクをヒープへ積む
 * @param 区間管理情報
 * @param 探索ステップ
 * @param 分割区間インデックス
 */
E_SC_RESULT RC_SetStartLink(SCRP_SECTCONTROLER* aSectCtrl, E_RP_RTCALCSTEP aStep, UINT16 aSplit) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	if (NULL == aSectCtrl) {
		return (e_SC_RESULT_BADPARAM);
	}

	switch (aStep) {
	case e_RP_STEP_LV1TOP:
		if (0 == aSplit) {
			result = RC_StartNbrLinkSet(&aSectCtrl->netTable, &aSectCtrl->neighbor[SCRP_NBROWN]);
		} else {
			result = RC_StartCandLinkSet(&aSectCtrl->netTable, &aSectCtrl->candMng, aStep);
		}
		break;
	case e_RP_STEP_LV1O:
		result = RC_StartNbrLinkSet(&aSectCtrl->netTable, &aSectCtrl->neighbor[SCRP_NBROWN]);
		break;
	case e_RP_STEP_LV1D:
		result = RC_StartCandLinkSet(&aSectCtrl->netTable, &aSectCtrl->candMng, aStep);
		break;
	case e_RP_STEP_LV2TOP:
		result = RC_StartCandLinkSet(&aSectCtrl->netTable, &aSectCtrl->candMng, aStep);
		break;
	default:
		return (e_SC_RESULT_BADPARAM);
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief 探索開始リンク設定（近傍）
 * @param ネットワーク管理
 * @param 近傍情報
 */
E_SC_RESULT RC_StartNbrLinkSet(SCRP_NETCONTROLER *aNetCtrl, SCRP_NEIGHBORINFO* aNeighbor) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	const UINT8 owFlg[2] = { 0x01, 0x02 };
	SCRP_NEIGHBORLINK* nbrLink = NULL;
	SCRC_IOCALCTBL inout = {};
	SCRC_RESULTCOSTS costs = {};
	SCRC_TARGETLINKINFO calcInfo = {};
	MAL_HDL binLink = NULL;
	UINT32 i, e;

	if (NULL == aNetCtrl || NULL == aNeighbor) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	inout.outLink = &calcInfo;
	nbrLink = aNeighbor->neighborLink;

	// 近傍リンクを開始リンクとしてヒープへ登録する
	for (i = 0; i < aNeighbor->nbrLinkVol; i++, nbrLink++) {
		if (nbrLink->noUse) {
			continue;
		}
		SCRP_PCLINFO* pclInfo = NULL;
		SCRP_LINKINFO* linkTab = NULL;
		UINT32 linkId = 0;
		UINT32 linkIdx = 0;
		// パーセル情報特定
		for (e = 0; e < aNetCtrl->parcelInfoVol; e++) {
			if ((aNetCtrl->parcelInfo + e)->parcelId == nbrLink->parcelId) {
				pclInfo = (aNetCtrl->parcelInfo + e);
				break;
			}
		}
		if (NULL == pclInfo) {
			SC_LOG_WarnPrint(SC_TAG_RC, "neighbor parcel not found in network. pcl=0x%08x", nbrLink->parcelId);
			continue;
		}
		// リンク情報特定
		linkTab = aNetCtrl->linkTable + (pclInfo->linkIdx + nbrLink->dataIndex);

		// リンクID確認
		binLink = SC_MA_A_NWRCD_LINK_GET_RECORD(pclInfo->mapNetworkLinkBin, linkTab->detaIndex);
		linkId = SC_MA_D_NWRCD_LINK_GET_ID(binLink);
		if (linkId != nbrLink->linkId) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "neighborLink index unmatched. pcl=0x%08x link=0x%08x!=0x%08x "HERE, nbrLink->linkId, linkId);
			continue;
		}

		// ベースリンク情報作成
		linkIdx = RCID_MASK_PCLIDX(pclInfo->index);
		linkIdx |= RCID_MASK_LINKIDX(pclInfo->linkIdx + nbrLink->dataIndex);
		linkIdx |= RCID_MASK_ORIDX(nbrLink->orFlag);

		// 一通＆通行不可判定
		if (0 != SC_MA_D_BASE_LINK_GET_ONEWAY(binLink)) {
			if (0 == (owFlg[RCID_GET_ORIDX(linkIdx)] & SC_MA_D_BASE_LINK_GET_ONEWAY(binLink))) {
				SC_LOG_WarnPrint(SC_TAG_RC, "can't advance. link=0x%08x "HERE, SC_MA_D_NWRCD_LINK_GET_ID(binLink) );
				continue;
			}
		}
		// コスト計算
		inout.outLink->linkIndex = linkIdx;
		inout.outLink->linkTable = linkTab;
		inout.outLink->linkNet = (linkTab->linkNet + nbrLink->orFlag);
		inout.outLink->pclInfo = pclInfo;
		inout.outLinkBin = binLink;
		result = RC_NoCrossCostCalc(aNetCtrl, &inout, &costs);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RC_CostCalc error. [0x%08x] "HERE, result);
			continue;
		}

		// コストとヒープは初期状態の場合のみ近傍初期コスト
		if (SCRP_HEAP_UV == inout.outLink->linkNet->heap) {
			UINT32 ratioCost = 0;
			UINT32 remainDist = 0;
			if (RCID_GET_ORIDX(inout.outLink->linkIndex)) {
				remainDist = (UINT32) lround(nbrLink->remainDist);
				ratioCost = (costs.totalCost * (remainDist / nbrLink->linkDist));
			} else {
				remainDist = nbrLink->linkDist - (UINT32) lround(nbrLink->remainDist);
				ratioCost = (costs.totalCost * (remainDist / nbrLink->linkDist));
			}
			// 塗りつぶし長に準じたコストにする
			inout.outLink->linkNet->costSum = nbrLink->cost + ratioCost;
			inout.outLink->linkNet->heap = SCRP_HEAP_V;
			// 開始リンクフラグ設定
			inout.outLink->linkNet->flag |= RCND_STARTLINK;

			RC_HeapInsert(aNetCtrl, inout.outLink->linkIndex);
		}
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 探索開始リンク設定（候補経路から）
 * @param ネットワーク管理
 * @param 候補経路管理
 * @param 探索ステップ
 */
E_SC_RESULT RC_StartCandLinkSet(SCRP_NETCONTROLER *aNetCtrl, SCRP_CANDMANAGER* aCand, E_RP_RTCALCSTEP aStep) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	const UINT8 owFlg[2] = { 0x01, 0x02 };
	SCRP_CANDSTARTLINK* candStLink = NULL;
	MAL_HDL binLink = NULL;
	UINT32 stCandVol = 0;
	//UINT16 tblIdx = 0;
	UINT32 i;
	//UINT32 e;
	FIND_STARTNET_FUNC pFunc = NULL;

	if (NULL == aNetCtrl || NULL == aCand) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 候補開始リンクリスト取得
	switch (aStep) {
	case e_RP_STEP_LV1TOP:
		if (0 == aNetCtrl->calculatingDivIdx) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
			return (e_SC_RESULT_BADPARAM);
		}
		candStLink = aCand->splitStLink;
		stCandVol = aCand->splitStLinkCurrent;
		pFunc = findStartNetwork;
		break;
	case e_RP_STEP_LV2TOP:
		if (0 == aNetCtrl->calculatingDivIdx) {
			candStLink = aCand->stLink + aCand->candTblInfo[RC_CAND_IDX_LV1O].stLinkIdx;
			stCandVol = aCand->candTblInfo[RC_CAND_IDX_LV1O].stLinkSize;
			pFunc = findStartNetworkLv2Top;
		} else {
			candStLink = aCand->splitStLink;
			stCandVol = aCand->splitStLinkCurrent;
			pFunc = findStartNetwork;
		}
		break;
	case e_RP_STEP_LV1D:
		candStLink = aCand->stLink + aCand->candTblInfo[RC_CAND_IDX_LV2TOP].stLinkIdx;
		stCandVol = aCand->candTblInfo[RC_CAND_IDX_LV2TOP].stLinkSize;
		pFunc = findStartNetworkLv1D;
		break;
	default:
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 候補開始リンクから派生するリンクを開始リンクとしてヒープへ登録する
	for (i = 0; i < stCandVol; i++, candStLink++) {
		UINT32 linkIdx = 0;
		UINT32 linkId = 0;
		result = pFunc(candStLink, aNetCtrl, &linkIdx);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "func error. [0x%08x] "HERE, result);
			continue;
		}
		if (RCID_NIDX_INVALID == linkIdx) {
			continue;
		}

		// ネットワークインデックスを解釈
		SCRP_PCLINFO* pclInfo = RCNET_GET_PCLINFO(aNetCtrl,linkIdx);
		SCRP_LINKINFO* linkInfo = RCNET_GET_LINKINFO(aNetCtrl,linkIdx);
		SCRP_NETDATA* netData = RCNET_GET_NETDATA(aNetCtrl,linkIdx);

		// リンクID確認
		binLink = SC_MA_A_NWRCD_LINK_GET_RECORD(pclInfo->mapNetworkLinkBin, linkInfo->detaIndex);
		linkId = SC_MA_D_NWRCD_LINK_GET_ID(binLink);
#if 0
		if (SC_MA_D_NWID_GET_PNT_ID(linkId) != SC_MA_D_NWID_GET_PNT_ID(candStLink->st.linkId)) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "candLink index unmatched. pcl=0x%08x link=0x%08x!=0x%08x "HERE, pclInfo->parcelId,
					candStLink->st.linkId, linkId);
			return (e_SC_RESULT_FAIL);
		}
#endif
		// 一通＆通行不可判定
		if (0 != SC_MA_D_BASE_LINK_GET_ONEWAY(binLink)) {
			if (0 == (owFlg[RCID_GET_ORIDX(linkIdx)] & SC_MA_D_BASE_LINK_GET_ONEWAY(binLink))) {
				SC_LOG_WarnPrint(SC_TAG_RC, "can't advance. link=0x%08x "HERE, SC_MA_D_NWRCD_LINK_GET_ID(binLink) );
				continue;
			}
		}

#if 0
		SC_LOG_InfoPrint(SC_TAG_RC, "set start calc heap. pcl=0x%08x link=0x%08x cost=%6d index=0x%08x "HERE, pclInfo->parcelId, linkId,
				netData->costSum, linkIdx);
#endif

		// 開始リンクフラグ設定
		if (SCRP_HEAP_UV == netData->heap) {
			netData->heap = SCRP_HEAP_V;
			netData->costSum = candStLink->cost;
			netData->flag |= RCND_STARTLINK;
			RC_HeapInsert(aNetCtrl, linkIdx);
		} else {
			// 既に登録済み（探索開始リンクの重複登録は許容しない）
			SC_LOG_WarnPrint(SC_TAG_RC, "start link regist heap yet. pcl=0x%08x link=0x%08x "HERE, candStLink->parcelId,
					candStLink->linkId);
		}
	}

	if (0 == aNetCtrl->heap.heapEnd) {
		SC_LOG_WarnPrint(SC_TAG_RC, "this step have no start link... step=%d "HERE, aStep);
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 同一レベル用探索開始リンク検索
 * @param 候補開始リンク情報
 * @param ネットワーク管理
 * @param [O]リンクインデックス
 */
static E_SC_RESULT findStartNetwork(SCRP_CANDSTARTLINK* aCandStLink, SCRP_NETCONTROLER* aNetCtrl, UINT32* aLinkIdx) {

	if (NULL == aCandStLink || NULL == aNetCtrl || NULL == aLinkIdx) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	*aLinkIdx = RCID_NIDX_INVALID;

	// 接続リンクのみを対象とする
	if (!RCND_GET_CANDJOINFLG(aCandStLink->flag)) {
		return (e_SC_RESULT_SUCCESS);
	}

	SCRP_PCLINFO* pclInfo = NULL;
	UINT32 linkIdx = 0;
	//UINT32 linkId = 0;
	UINT32 i;

	// パーセル情報特定
	for (i = 0; i < aNetCtrl->parcelInfoVol; i++) {
		if ((aNetCtrl->parcelInfo + i)->parcelId == aCandStLink->parcelId) {
			pclInfo = (aNetCtrl->parcelInfo + i);
			break;
		}
	}
	if (NULL == pclInfo) {
		return (e_SC_RESULT_SUCCESS);
	}

	// ベースリンク情報作成
	linkIdx = RCID_MASK_PCLIDX(pclInfo->index);
	linkIdx |= RCID_MASK_LINKIDX(pclInfo->linkIdx + aCandStLink->dataIdx);
	linkIdx |= RCID_MASK_ORIDX(RCND_GET_ORIDX(aCandStLink->flag));

	*aLinkIdx = linkIdx;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief レベル１D側探索開始リンク検索
 * @param 候補開始リンク情報
 * @param ネットワーク管理
 * @param [O]リンクインデックス
 */
static E_SC_RESULT findStartNetworkLv1D(SCRP_CANDSTARTLINK* aCandStLink, SCRP_NETCONTROLER* aNetCtrl, UINT32* aLinkIdx) {

	if (NULL == aCandStLink || NULL == aNetCtrl || NULL == aLinkIdx) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	*aLinkIdx = RCID_NIDX_INVALID;

	// 下位接続リンク以外は無視
	if (RP_LEVEL1 != aCandStLink->connectLevel) {
		return (e_SC_RESULT_SUCCESS);
	}
	SCRP_PCLINFO* pclInfo = NULL;
	UINT16 findIdx = 0;
	UINT32 linkIdx = 0;
	UINT32 i;

	// リンクIDとリンク方向を取得
	UINT32 targetId = SC_MA_D_NWID_GET_PNT_ID(aCandStLink->st.linkId);
	UINT32 targetDir = (SC_MA_NWID_IS_CNCTSIDE_ODR(aCandStLink->st.linkId)) ? SCRP_LINKODR : SCRP_LINKRVS;

	// パーセル情報特定
	for (i = 0; i < aNetCtrl->parcelInfoVol; i++) {
		if ((aNetCtrl->parcelInfo + i)->parcelId == aCandStLink->st.parcelId) {
			pclInfo = (aNetCtrl->parcelInfo + i);
			break;
		}
	}
	if (NULL == pclInfo) {
		return (e_SC_RESULT_SUCCESS);
	}
	// リンク情報特定
	findIdx = SC_MA_BinSearchNwRecord(pclInfo->mapNetworkBin, targetId, SC_MA_BINSRC_TYPE_LINK);
	if (ALL_F16 == findIdx) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "cand downlevel link can't found. pcl=0x%08x link=0x%08x "HERE, pclInfo->parcelId,
				aCandStLink->st.linkId);
		return (e_SC_RESULT_FAIL);
	}
	// ネットワークテーブルインデックスへ変換
	findIdx -= 1;

	// ベースリンク情報作成
	linkIdx = RCID_MASK_PCLIDX(pclInfo->index);
	linkIdx |= RCID_MASK_LINKIDX(pclInfo->linkIdx + findIdx);
	linkIdx |= RCID_MASK_ORIDX(targetDir);

	*aLinkIdx = linkIdx;
#if 0
	SC_LOG_InfoPrint(SC_TAG_RC, "lv1DStLink pcl=0x%08x link=0x%08x dir=%d idx=0x%08x ", aCandStLink->st.parcelId, aCandStLink->st.linkId,
			targetDir, linkIdx);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief レベル２トップ探索開始リンク検索：初回用
 * @param 候補開始リンク情報
 * @param ネットワーク管理
 * @param [O]リンクインデックス
 */
static E_SC_RESULT findStartNetworkLv2Top(SCRP_CANDSTARTLINK* aCandStLink, SCRP_NETCONTROLER* aNetCtrl, UINT32* aLinkIdx) {

	if (NULL == aCandStLink || NULL == aNetCtrl || NULL == aLinkIdx) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	*aLinkIdx = RCID_NIDX_INVALID;

	// 上位接続リンク以外は無視
	if (RP_LEVEL2 != aCandStLink->connectLevel) {
		return (e_SC_RESULT_SUCCESS);
	}

	SCRP_PCLINFO* pclInfo = NULL;
	UINT16 findIdx = 0;
	UINT32 linkIdx = 0;
	UINT32 i;

	// リンクIDとリンク方向を取得
	UINT32 targetId = SC_MA_D_NWID_GET_PNT_ID(aCandStLink->st.linkId);
	UINT32 targetDir = (SC_MA_NWID_IS_CNCTSIDE_ODR(aCandStLink->st.linkId)) ? SCRP_LINKODR : SCRP_LINKRVS;

	// パーセル情報特定
	for (i = 0; i < aNetCtrl->parcelInfoVol; i++) {
		if ((aNetCtrl->parcelInfo + i)->parcelId == aCandStLink->st.parcelId) {
			pclInfo = (aNetCtrl->parcelInfo + i);
			break;
		}
	}
	if (NULL == pclInfo) {
		return (e_SC_RESULT_SUCCESS);
	}
	// リンク情報特定
	findIdx = SC_MA_BinSearchNwRecord(pclInfo->mapNetworkBin, targetId, SC_MA_BINSRC_TYPE_LINK);
	if (ALL_F16 == findIdx) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "cand uplevel link can't found. pcl=0x%08x link=0x%08x "HERE, pclInfo->parcelId,
				aCandStLink->st.linkId);
		return (e_SC_RESULT_FAIL);
	}
	// ネットワークテーブルインデックスへ変換
	findIdx -= 1;

	// ベースリンク情報作成
	linkIdx = RCID_MASK_PCLIDX(pclInfo->index);
	linkIdx |= RCID_MASK_LINKIDX(pclInfo->linkIdx + findIdx);
	linkIdx |= RCID_MASK_ORIDX(targetDir);

	*aLinkIdx = linkIdx;

	return (e_SC_RESULT_SUCCESS);
}
