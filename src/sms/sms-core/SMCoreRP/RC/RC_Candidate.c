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
 * RP_Candidate.c
 *
 *  Created on: 2015/11/06
 *      Author: masutani
 */

#include "sms-core/SMCoreRP/SMCoreRPInternal.h"

typedef E_SC_RESULT (*SET_CONNECT_FUNC)(SCRP_PCLINFO* aPclInfo, T_MapNWLink* aPLink, SCRP_CANDSTARTLINK* aCandStLink);

/*-------------------------------------------------------------------
 * 宣言
 *-------------------------------------------------------------------*/
static E_SC_RESULT mallocCandData(SCRP_CANDMANAGER* aCandMng, UINT32 aAddSize);
static E_SC_RESULT mallocStLink(SCRP_CANDMANAGER* aCandMng, UINT32 aAddSize);
static E_SC_RESULT mallocSplitRoute(SCRP_CANDMANAGER* aCandMng, UINT32 aSize);
static E_SC_RESULT mallocSplitCand(SCRP_CANDMANAGER* aCandMng, UINT32 aAddSize);
static E_SC_RESULT mallocSplitStLink(SCRP_CANDMANAGER* aCandMng, UINT32 aAddSize);
static E_SC_RESULT setCandData(SCRP_CANDDATA*, UINT32, SCRP_PCLINFO*, SCRP_LINKINFO*, SCRP_NETDATA*);
static E_SC_RESULT makeStartCandLv1Top(SCRP_SECTCONTROLER* aSectCtrl, SCRP_NETCONTROLER* aNetCtrl);
static E_SC_RESULT makeStartCandLv1O(SCRP_SECTCONTROLER* aSectCtrl, SCRP_NETCONTROLER* aNetCtrl);
static E_SC_RESULT makeStartCandLv1D(SCRP_SECTCONTROLER* aSectCtrl, SCRP_NETCONTROLER* aNetCtrl);
static E_SC_RESULT makeStartCandLv2Top(SCRP_SECTCONTROLER* aSectCtrl, SCRP_NETCONTROLER* aNetCtrl);
static E_SC_RESULT makeSplitCand(SCRP_SECTCONTROLER* aSectCtrl, SCRP_NETCONTROLER* aNetCtrl);
static SCRP_CANDSTARTLINK* searchSplitCandStLink(SCRP_CANDMANAGER* aCandMng, UINT32 aPclId, UINT32 aLinkId, UINT8 aDir);
static E_SC_RESULT copyLatestSplitCand(SCRP_CANDMANAGER* aCandMng, E_RP_RTCALCSTEP aStep);
static E_SC_RESULT setSplitStartCandFormJoin(SCRP_SECTCONTROLER* aSectCtrl, SCRP_PCLINFO* aPclInfo, SCRP_LINKINFO* aLinkInfo,
		SCRP_NETDATA* aNetData, SET_CONNECT_FUNC pFunc, UINT32 *aCount);
static E_SC_RESULT setLinkExDataConnect(SCRP_PCLINFO* aPclInfo, T_MapNWLink* aPLink, SCRP_CANDSTARTLINK* aCandStLink);
static E_SC_RESULT setLinkExDataUpLvConnect(SCRP_PCLINFO* aPclInfo, T_MapNWLink* aPLink, SCRP_CANDSTARTLINK* aCandStLink);
static E_SC_RESULT setLinkExDataDownLvConnect(SCRP_PCLINFO* aPclInfo, T_MapNWLink* aPLink, SCRP_CANDSTARTLINK* aCandStLink);
/*-------------------------------------------------------------------
 * マクロ定義
 *-------------------------------------------------------------------*/
// 候補登録済みマーク
#define CAND_SET_REGISTMARK(net, idx)									\
	{																	\
		(net)->heap = idx;												\
		(net)->flag |= RCND_REGISTEDCAND;								\
	}
// 分割候補使用位置取得
#define CAND_GET_PREVSPLITCAND(mng)		(mng)->splitCand + ((mng)->splitCandCurrent - 1)
// 分割候補取得＋使用位置更新
#define CAND_GET_CRNTSPLITCAND(mng)		(mng)->splitCand + (mng)->splitCandCurrent++
// 分割開始取得＋使用位置更新
#define CAND_GET_CRNTSPLITSTLINK(mng)	(mng)->splitStLink + (mng)->splitStLinkCurrent++

/**
 * @brief 候補経路初期化
 * @param 候補経路管理
 * @note すべての候補経路情報を初期化する
 */
E_SC_RESULT RC_CandMngInit(SCRP_CANDMANAGER* aCandMng) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aCandMng) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	E_SC_RESULT result = e_SC_RESULT_SUCCESS;

	// 候補初期化
	RC_CandFree(aCandMng);
	// 分割候補初期化
	RC_SplitCandFree(aCandMng);
	// 0クリア
	RP_Memset0(aCandMng, sizeof(SCRP_CANDMANAGER));

	do {
		// 領域確保
		result = mallocCandData(aCandMng, RC_CAND_SIZE);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "mallocCandData error. [0x%08x] "HERE, result);
			break;
		}
		result = mallocStLink(aCandMng, RC_CAND_STLINK_SIZE);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "mallocStLink error. [0x%08x] "HERE, result);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief 分割エリア候補経路初期化
 * @param 候補経路管理
 * @param 分割エリア数
 * @memo 各分割エリア計算前に実行する
 */
E_SC_RESULT RC_SplitCandMngInit(SCRP_CANDMANAGER* aCandMng, UINT32 aSplitVol) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aCandMng) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	E_SC_RESULT result = e_SC_RESULT_SUCCESS;

	// 各テーブル初期化
	RC_SplitCandFree(aCandMng);

	do {
		// 領域確保
		result = mallocSplitRoute(aCandMng, aSplitVol);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "mallocSplitRoute error. "HERE);
			break;
		}
		result = mallocSplitCand(aCandMng, RC_CAND_SIZE);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "mallocSplitCand error. "HERE);
			break;
		}
		result = mallocSplitStLink(aCandMng, RC_CAND_STLINK_SIZE);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "mallocSplitStLink error. "HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief 候補経路解放
 * @param 候補経路管理
 */
void RC_CandFree(SCRP_CANDMANAGER* aCandMng) {

	if (NULL == aCandMng) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return;
	}
	// Cand領域の解放
	if (NULL != aCandMng->cand) {
		RP_MemFree(aCandMng->cand, e_MEM_TYPE_ROUTECAND);
	}
	if (NULL != aCandMng->stLink) {
		RP_MemFree(aCandMng->stLink, e_MEM_TYPE_ROUTECAND);
	}

	// Cand領域初期化
	aCandMng->cand = NULL;
	aCandMng->candSize = 0;
	aCandMng->candCurrent = 0;

	aCandMng->stLink = NULL;
	aCandMng->stLinkSize = 0;
	aCandMng->stLinkCurrent = 0;
}

/**
 * @brief 分割候補経路解放
 * @param 候補経路管理
 */
void RC_SplitCandFree(SCRP_CANDMANAGER* aCandMng) {

	if (NULL == aCandMng) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return;
	}
	// SplitCand領域の解放
	if (NULL != aCandMng->splitCand) {
		RP_MemFree(aCandMng->splitCand, e_MEM_TYPE_ROUTECAND);
	}
	if (NULL != aCandMng->splitStLink) {
		RP_MemFree(aCandMng->splitStLink, e_MEM_TYPE_ROUTECAND);
	}
	if (NULL != aCandMng->splitCandTblInfo) {
		RP_MemFree(aCandMng->splitCandTblInfo, e_MEM_TYPE_ROUTECAND);
	}

	// SplitCand領域初期化
	aCandMng->splitCandTblInfo = NULL;
	aCandMng->splitCandTblInfoVol = 0;

	aCandMng->splitCand = NULL;
	aCandMng->splitCandSize = 0;
	aCandMng->splitCandCurrent = 0;

	aCandMng->splitStLink = NULL;
	aCandMng->splitStLinkSize = 0;
	aCandMng->splitStLinkCurrent = 0;
}

/**
 * @brief 候補経路作成処理
 * @param 区間管理
 * @param ネットワーク管理
 * @param 探索ステップ
 */
E_SC_RESULT RC_CandMake(SCRP_SECTCONTROLER* aSectCtrl, SCRP_NETCONTROLER* aNetCtrl, E_RP_RTCALCSTEP aStep) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	RPC_SetCalcProcess(e_RC_PROC_MAKECAND); // プロセス登録

	if (NULL == aSectCtrl || NULL == aNetCtrl) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}
#if _RPLAPTIME_MAKECAND // ▲時間計測
	RP_SetLapTime();
#endif

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	//SCRP_CANDMANAGER* candMng = &aSectCtrl->candMng;

	// 候補開始リンク情報回収＆候補リンク情報を作成
	switch (aStep) {
	case e_RP_STEP_LV1TOP:
		result = makeStartCandLv1Top(aSectCtrl, aNetCtrl);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "makeStartCandLv1Top error. [0x%08x] "HERE, result);
			break;
		}
		result = makeSplitCand(aSectCtrl, aNetCtrl);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "makeSplitCand error. [0x%08x] "HERE, result);
			break;
		}
		break;
	case e_RP_STEP_LV1O:
		result = makeStartCandLv1O(aSectCtrl, aNetCtrl);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "makeStartCandLv1O error. [0x%08x] "HERE, result);
			break;
		}
		result = makeSplitCand(aSectCtrl, aNetCtrl);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "makeSplitCand error. [0x%08x] "HERE, result);
			break;
		}
		break;
	case e_RP_STEP_LV1D:
		result = makeStartCandLv1D(aSectCtrl, aNetCtrl);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "makeStartCandLv1D error. [0x%08x] "HERE, result);
			break;
		}
		result = makeSplitCand(aSectCtrl, aNetCtrl);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "makeSplitCand error. [0x%08x] "HERE, result);
			break;
		}
		break;
	case e_RP_STEP_LV2TOP:
		result = makeStartCandLv2Top(aSectCtrl, aNetCtrl);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "makeStartCandLv2Top error. [0x%08x] "HERE, result);
			break;
		}
		result = makeSplitCand(aSectCtrl, aNetCtrl);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "makeSplitCand error. [0x%08x] "HERE, result);
			break;
		}
		break;
	default:
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		result = e_SC_RESULT_BADPARAM;
		break;
	}

#if _RPLAPTIME_MAKECAND // ▲時間計測
	RP_SetLapTimeWithStr("candmake");
#endif
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief 候補経路本体への統合処理
 * @param 候補経路管理
 * @param 探索ステップ
 */
E_SC_RESULT RC_CandPromotion(SCRP_CANDMANAGER* aCandMng, E_RP_RTCALCSTEP aStep) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);
	// プロセス登録
	RPC_SetCalcProcess(e_RC_PROC_MAKECAND);

	if (NULL == aCandMng) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	E_SC_RESULT result = e_SC_RESULT_SUCCESS;

	// 候補経路統合処理
	switch (aStep) {
	case e_RP_STEP_LV1O:
		result = copyLatestSplitCand(aCandMng, aStep);
		break;
	case e_RP_STEP_LV1D:
		result = copyLatestSplitCand(aCandMng, aStep);
		break;
	case e_RP_STEP_LV1TOP:
		result = copyLatestSplitCand(aCandMng, aStep);
		break;
	case e_RP_STEP_LV2TOP:
		result = copyLatestSplitCand(aCandMng, aStep);
		break;
	default:
		result = e_SC_RESULT_BADPARAM;
		break;
	}

	// SplitCand領域の解放
	RC_SplitCandFree(aCandMng);

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief 候補リンクリスト領域確保
 * @param 候補経路管理
 * @param 追加サイズ
 * @memo 領域確保失敗時に旧領域の解放は本関数では行わない。
 */
static E_SC_RESULT mallocCandData(SCRP_CANDMANAGER* aCandMng, UINT32 aAddSize) {

	if (NULL == aCandMng || 0 == aAddSize) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "param error."HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 追加サイズを加算した領域を確保
	UINT32 newSize = aCandMng->candSize + aAddSize;
	SCRP_CANDDATA* newCand = RP_MemAlloc(sizeof(SCRP_CANDDATA) * newSize, e_MEM_TYPE_ROUTECAND);
	if (NULL == newCand) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error. "HERE);
		return (e_SC_RESULT_MALLOC_ERR);
	}

	// 0クリア
	RP_Memset0(newCand, sizeof(SCRP_CANDDATA) * newSize);

	// 旧領域がある場合コピー
	if (NULL != aCandMng->cand && 0 < aCandMng->candSize) {
		RP_Memcpy(newCand, aCandMng->cand, sizeof(SCRP_CANDDATA) * aCandMng->candSize);
		RP_MemFree(aCandMng->cand, e_MEM_TYPE_ROUTECAND);
	}
	// 書き換え
	aCandMng->cand = newCand;
	aCandMng->candSize = newSize;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 候補開始リンク領域確保
 * @param 候補経路管理
 * @param 追加サイズ
 * @memo 領域確保失敗時に旧領域の解放は本関数では行わない。
 */
static E_SC_RESULT mallocStLink(SCRP_CANDMANAGER* aCandMng, UINT32 aAddSize) {

	if (NULL == aCandMng || 0 == aAddSize) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "param error."HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 追加サイズを加算した領域を確保
	UINT32 newSize = aCandMng->stLinkSize + aAddSize;
	SCRP_CANDSTARTLINK* newStLink = RP_MemAlloc(sizeof(SCRP_CANDSTARTLINK) * newSize, e_MEM_TYPE_ROUTECAND);
	if (NULL == newStLink) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error."HERE);
		return (e_SC_RESULT_MALLOC_ERR);
	}

	// 0クリア
	RP_Memset0(newStLink, sizeof(SCRP_CANDSTARTLINK) * newSize);

	// 旧領域がある場合コピー
	if (NULL != aCandMng->stLink && 0 < aCandMng->stLinkSize) {
		RP_Memcpy(newStLink, aCandMng->stLink, sizeof(SCRP_CANDSTARTLINK) * aCandMng->stLinkSize);
		RP_MemFree(aCandMng->stLink, e_MEM_TYPE_ROUTECAND);
	}
	// 書き換え
	aCandMng->stLink = newStLink;
	aCandMng->stLinkSize = newSize;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 候補リンクリスト領域確保
 * @param 候補経路管理
 * @param 確保サイズ
 */
static E_SC_RESULT mallocSplitRoute(SCRP_CANDMANAGER* aCandMng, UINT32 aSize) {

	if (NULL == aCandMng || 0 == aSize) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	// 領域がクリアされていない場合解放処理
	if (NULL != aCandMng->splitCandTblInfo) {
		RP_MemFree(aCandMng->splitCandTblInfo, e_MEM_TYPE_ROUTECAND);
		aCandMng->splitCandTblInfo = NULL;
		aCandMng->splitCandTblInfoVol = 0;
	}

	SCRP_CANDTBLINFO *candInfo = RP_MemAlloc(sizeof(SCRP_CANDTBLINFO) * aSize, e_MEM_TYPE_ROUTECAND);
	if (NULL == candInfo) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error. "HERE);
		return (e_SC_RESULT_MALLOC_ERR);
	}
	RP_Memset0(candInfo, sizeof(SCRP_CANDTBLINFO) * aSize);

	aCandMng->splitCandTblInfo = candInfo;
	aCandMng->splitCandTblInfoVol = aSize;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 候補リンクリスト領域確保（分割）
 * @param 候補経路管理
 * @param 追加サイズ
 * @memo 領域確保失敗時に旧領域の解放は本関数では行わない。
 */
static E_SC_RESULT mallocSplitCand(SCRP_CANDMANAGER* aCandMng, UINT32 aAddSize) {

	if (NULL == aCandMng || 0 == aAddSize) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "param error."HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	UINT32 newSize = aCandMng->splitCandSize + aAddSize;
	SCRP_CANDDATA *newCand = RP_MemAlloc(sizeof(SCRP_CANDDATA) * newSize, e_MEM_TYPE_ROUTECAND);
	if (NULL == newCand) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "cand malloc error."HERE);
		return (e_SC_RESULT_MALLOC_ERR);
	}

	// 0クリア
	RP_Memset0(newCand, sizeof(SCRP_CANDDATA) * newSize);

	// 旧領域がある場合コピー
	if (NULL != aCandMng->splitCand) {
		RP_Memcpy(newCand, aCandMng->splitCand, sizeof(SCRP_CANDDATA) * aCandMng->splitCandSize);
		RP_MemFree(aCandMng->splitCand, e_MEM_TYPE_ROUTECAND);
	}
	// 書き換え
	aCandMng->splitCand = newCand;
	aCandMng->splitCandSize = newSize;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 候補開始リンクリスト領域確保（分割）
 * @param 候補経路管理
 * @param 追加サイズ
 * @memo 領域確保失敗時に旧領域の解放は本関数では行わない。
 */
static E_SC_RESULT mallocSplitStLink(SCRP_CANDMANAGER* aCandMng, UINT32 aAddSize) {

	if (NULL == aCandMng || 0 == aAddSize) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "param error."HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	UINT32 newSize = aCandMng->splitStLinkSize + aAddSize;
	SCRP_CANDSTARTLINK* newStLink = RP_MemAlloc(sizeof(SCRP_CANDSTARTLINK) * newSize, e_MEM_TYPE_ROUTECAND);
	if (NULL == newStLink) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "cand malloc error."HERE);
		return (e_SC_RESULT_MALLOC_ERR);
	}

	// 0クリア
	RP_Memset0(newStLink, sizeof(SCRP_CANDSTARTLINK) * newSize);

	// 旧領域がある場合コピー
	if (NULL != aCandMng->splitStLink) {
		RP_Memcpy(newStLink, aCandMng->splitStLink, sizeof(SCRP_CANDSTARTLINK) * aCandMng->splitStLinkSize);
		RP_MemFree(aCandMng->splitStLink, e_MEM_TYPE_ROUTECAND);
	}
	// 書き換え
	aCandMng->splitStLink = newStLink;
	aCandMng->splitStLinkSize = newSize;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 候補開始リンク情報作成：レベル１トップ専用
 * @param 区間管理
 * @param ネットワーク管理
 * @memo 分割候補開始リンク
 *       目的地候補開始リンクを回収
 */
static E_SC_RESULT makeStartCandLv1Top(SCRP_SECTCONTROLER* aSectCtrl, SCRP_NETCONTROLER* aNetCtrl) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aSectCtrl || NULL == aNetCtrl) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SCRP_PCLINFO* pclInfo = NULL;
	SCRP_CANDTBLINFO* candRoute = &aSectCtrl->candMng.splitCandTblInfo[aNetCtrl->calculatingDivIdx];
	UINT32 i, e, or;

	// 管理情報初期化
	candRoute->stLinkIdx = aSectCtrl->candMng.splitStLinkCurrent;
	candRoute->stLinkSize = 0;

	pclInfo = aNetCtrl->parcelInfo;
	for (i = 0; i < aNetCtrl->parcelInfoVol; i++, pclInfo++) {
		// 目的地パーセル or 接続パーセル or 断裂パーセル
		if (0 == pclInfo->areaJoin && 0 == pclInfo->areaSplit && !RCPI_GET_DESTFLG(pclInfo->flag)) {
			continue;
		}
		SCRP_LINKINFO* linkInfo = aNetCtrl->linkTable + pclInfo->linkIdx;
		for (e = 0; e < pclInfo->linkIdVol; e++, linkInfo++) {
			// 順逆
			SCRP_NETDATA* netData = linkInfo->linkNet;
			for (or = 0; or < 2; or++, netData++) {
				// 計算済み確認
				if (SCRP_HEAP_V != netData->heap) {
					continue;
				}
				// 候補開始リンク判定
				if (!RCND_GET_DESTLINKFLG(netData->flag) && !RCND_GET_CANDJOINFLG(netData->flag) && !RCND_GET_CANDSPLITFLG(netData->flag)) {
					continue;
				}
				UINT32 count = 0;
				result = setSplitStartCandFormJoin(aSectCtrl, pclInfo, linkInfo, netData, setLinkExDataConnect, &count);
				if (e_SC_RESULT_SUCCESS != result) {
					SC_LOG_ErrorPrint(SC_TAG_RC, "setStartCandFormJoin error. [0x%08x] "HERE, result);
					return (result);
				}
				candRoute->stLinkSize += count;
			}
		}
	}
	if (0 == candRoute->stLinkSize) {
		SC_LOG_InfoPrint(SC_TAG_RC, "this divarea can't found start cand link... divIdx=%d " HERE, aNetCtrl->calculatingDivIdx);
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 候補開始リンク情報作成：レベル１Ｏ側専用
 * @param 区間管理
 * @param ネットワーク管理
 * @memo 上位接続候補開始リンクを回収
 */
static E_SC_RESULT makeStartCandLv1O(SCRP_SECTCONTROLER* aSectCtrl, SCRP_NETCONTROLER* aNetCtrl) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aSectCtrl || NULL == aNetCtrl) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SCRP_PCLINFO* pclInfo = NULL;
	SCRP_LINKINFO* linkInfo = NULL;
	SCRP_CANDTBLINFO* candRoute = &aSectCtrl->candMng.splitCandTblInfo[aNetCtrl->calculatingDivIdx];
	UINT32 i, e, or;

	// 管理情報初期化
	candRoute->stLinkIdx = aSectCtrl->candMng.splitStLinkCurrent;
	candRoute->stLinkSize = 0;

	pclInfo = aNetCtrl->parcelInfo;
	for (i = 0; i < aNetCtrl->parcelInfoVol; i++, pclInfo++) {
		// 最外殻パーセル or 断裂パーセル
		if (0 == pclInfo->areaSplit && !RCPI_GET_OUTERMOST(pclInfo->flag)) {
			continue;
		}
		linkInfo = aNetCtrl->linkTable + pclInfo->linkIdx;
		for (e = 0; e < pclInfo->linkIdVol; e++, linkInfo++) {
			SCRP_NETDATA* netData = linkInfo->linkNet;
			for (or = 0; or < 2; or++, netData++) {
				if (RCND_GET_STARTLINKFLG(netData->flag)) {
					continue;
				}
				if (SCRP_HEAP_V != netData->heap) {
					continue;
				}
				SCRP_NETDATA* targetNetData = NULL;
				SCRP_LINKINFO* targetLinkInfo = NULL;
				SCRP_PCLINFO* targetPclInfo = NULL;
				/* ▼エリア外殻
				 *   探索履歴追跡（関数コールのオーバーヘッドを考慮し関数化はしません）
				 *   TODO 外殻＆断裂 の場合に断裂用の候補経路を生成しなくて良いのかという問題… */
				if (RCND_GET_AREAENDFLG(netData->flag)) {
					targetNetData = netData;
					targetLinkInfo = linkInfo;
					targetPclInfo = pclInfo;
					while (1) {
						if (RCND_GET_STARTLINKFLG(targetNetData->flag)) {
							targetNetData = NULL;
							break;
						}
						if (ALL_F32 == targetNetData->inLinkHist) {
							SC_LOG_WarnPrint(SC_TAG_RC, "inLinkHist is invalid error. "HERE);
							targetNetData = NULL;
							break;
						}
						// 上位接続発見時に終了
						if (SCRP_LINKODR == RCND_GET_ORIDX(targetNetData->flag)) {
							if (RCND_GET_UPLEVELFLG((targetNetData + 1)->flag))
								break;
						} else {
							if (RCND_GET_UPLEVELFLG((targetNetData - 1)->flag))
								break;
						}
						targetPclInfo = RCNET_GET_HISTPCLINFO(aNetCtrl, targetNetData);
						targetLinkInfo = RCNET_GET_HISTLINKINFO(aNetCtrl, targetNetData);
						targetNetData = RCNET_GET_HISTNETDATA(aNetCtrl, targetNetData);
					}
				} else if (RCND_GET_CANDSPLITFLG(netData->flag)) {
					// 断裂候補開始リンク
					targetNetData = netData;
					targetLinkInfo = linkInfo;
					targetPclInfo = pclInfo;
				}
				if (targetNetData == NULL) {
					continue;
				}

				// 候補開始リンクへ登録する
				UINT32 count = 0;
				result = setSplitStartCandFormJoin(aSectCtrl, targetPclInfo, targetLinkInfo, targetNetData, setLinkExDataUpLvConnect,
						&count);
				if (e_SC_RESULT_SUCCESS != result) {
					SC_LOG_ErrorPrint(SC_TAG_RC, "setStartCandFormJoin error. [0x%08x] "HERE, result);
					return (result);
				}
				candRoute->stLinkSize += count;
			}
		}
	}
	if (0 == candRoute->stLinkSize) {
		SC_LOG_InfoPrint(SC_TAG_RC, "this divarea can't found start cand link... divIdx=%d " HERE, aNetCtrl->calculatingDivIdx);
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 候補開始リンク情報作成：レベル１トップ専用
 * @param 区間管理
 * @param ネットワーク管理
 * @memo 断裂候補開始リンク
 *       目的地候補開始リンクを回収
 */
static E_SC_RESULT makeStartCandLv1D(SCRP_SECTCONTROLER* aSectCtrl, SCRP_NETCONTROLER* aNetCtrl) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aSectCtrl || NULL == aNetCtrl) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SCRP_PCLINFO* pclInfo = NULL;
	SCRP_CANDTBLINFO* candRoute = &aSectCtrl->candMng.splitCandTblInfo[aNetCtrl->calculatingDivIdx];
	UINT32 i, e, or;

	// 管理情報初期化
	candRoute->stLinkIdx = aSectCtrl->candMng.splitStLinkCurrent;
	candRoute->stLinkSize = 0;

	pclInfo = aNetCtrl->parcelInfo;
	for (i = 0; i < aNetCtrl->parcelInfoVol; i++, pclInfo++) {
		// 目的地パーセル or 断裂パーセル
		if (0 == pclInfo->areaSplit && !RCPI_GET_DESTFLG(pclInfo->flag)) {
			continue;
		}
		SCRP_LINKINFO* linkInfo = aNetCtrl->linkTable + pclInfo->linkIdx;
		for (e = 0; e < pclInfo->linkIdVol; e++, linkInfo++) {
			// 順逆
			SCRP_NETDATA* netData = linkInfo->linkNet;
			for (or = 0; or < 2; or++, netData++) {
				// 計算済み確認
				if (SCRP_HEAP_V != netData->heap) {
					continue;
				}
				// 目的地リンク・断裂リンク判定
				if (!RCND_GET_DESTLINKFLG(netData->flag) && !RCND_GET_CANDSPLITFLG(netData->flag)) {
					continue;
				}
				UINT32 count = 0;
				result = setSplitStartCandFormJoin(aSectCtrl, pclInfo, linkInfo, netData, setLinkExDataConnect, &count);
				if (e_SC_RESULT_SUCCESS != result) {
					SC_LOG_ErrorPrint(SC_TAG_RC, "setStartCandFormJoin error. [0x%08x] "HERE, result);
					return (result);
				}
				candRoute->stLinkSize += count;
			}
		}
	}
	if (0 == candRoute->stLinkSize) {
		SC_LOG_InfoPrint(SC_TAG_RC, "this divarea can't found start cand link... divIdx=%d " HERE, aNetCtrl->calculatingDivIdx);
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 候補開始リンク情報作成：レベル２トップ専用
 * @param 区間管理
 * @param ネットワーク管理
 * @memo 分割候補開始リンク
 *       下位接続候補開始リンクを回収
 */
static E_SC_RESULT makeStartCandLv2Top(SCRP_SECTCONTROLER* aSectCtrl, SCRP_NETCONTROLER* aNetCtrl) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aSectCtrl || NULL == aNetCtrl) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SCRP_PCLINFO* pclInfo = NULL;
	SCRP_LINKINFO* linkInfo = NULL;
	SCRP_CANDTBLINFO* candRoute = &aSectCtrl->candMng.splitCandTblInfo[aNetCtrl->calculatingDivIdx];
	UINT32 i, e, or;

	// 管理情報初期化
	candRoute->stLinkIdx = aSectCtrl->candMng.splitStLinkCurrent;
	candRoute->stLinkSize = 0;

	pclInfo = aNetCtrl->parcelInfo;
	for (i = 0; i < aNetCtrl->parcelInfoVol; i++, pclInfo++) {
		/* ▼下位接続パーセル
		 *   探索履歴追跡（関数コールのオーバーヘッドを考慮し関数化しない） */
		if (RCPI_DOWNCNCT == RCPI_GET_DOWNCNCT(pclInfo->flag)) {
			linkInfo = aNetCtrl->linkTable + pclInfo->linkIdx;
			for (e = 0; e < pclInfo->linkIdVol; e++, linkInfo++) {
				SCRP_NETDATA* netData = linkInfo->linkNet;
				for (or = 0; or < 2; or++, netData++) {
					if (SCRP_HEAP_V != netData->heap) {
						continue;
					}
					if (RCND_GET_STARTLINKFLG(netData->flag)) {
						continue;
					}
					if (SCRP_LINKODR == or) {
						if (!RCND_GET_DOWNLEVELFLG((netData + 1)->flag))
							continue;
					} else {
						if (!RCND_GET_DOWNLEVELFLG((netData - 1)->flag))
							continue;
					}
					SCRP_NETDATA* preNetData = NULL;
					SCRP_LINKINFO* preLinkInfo = NULL;
					SCRP_PCLINFO* prePclInfo = NULL;
					SCRP_NETDATA* nextNetData = netData;
					SCRP_LINKINFO* nextLinkInfo = linkInfo;
					SCRP_PCLINFO* nextPclInfo = pclInfo;
					// 探索履歴の'逆側'が下位へ接続していることが条件
					while (1) {
						if (RCND_GET_STARTLINKFLG(nextNetData->flag)) {
							nextNetData = NULL;
							break;
						}
						if (ALL_F32 == nextNetData->inLinkHist) {
							SC_LOG_WarnPrint(SC_TAG_RC, "inLinkHist is invalid error. "HERE);
							nextNetData = NULL;
							break;
						}
						preNetData = nextNetData;
						preLinkInfo = nextLinkInfo;
						prePclInfo = nextPclInfo;
						nextLinkInfo = RCNET_GET_HISTLINKINFO(aNetCtrl, preNetData);
						nextPclInfo = RCNET_GET_HISTPCLINFO(aNetCtrl, preNetData);
						nextNetData = RCNET_GET_HISTNETDATA(aNetCtrl, preNetData);
						// 下位接続でなくなったら終了
						if (SCRP_LINKODR == RCND_GET_ORIDX(nextNetData->flag)) {
							if (!RCND_GET_DOWNLEVELFLG((nextNetData + 1)->flag))
								break;
						} else {
							if (!RCND_GET_DOWNLEVELFLG((nextNetData - 1)->flag))
								break;
						}
					}
					if (NULL == nextNetData) {
						continue;
					}
					// 下位接続でなくなる直前のリンクのみを候補開始リンクへ登録する
					UINT32 count = 0;
					result = setSplitStartCandFormJoin(aSectCtrl, prePclInfo, preLinkInfo, preNetData, setLinkExDataDownLvConnect, &count);
					if (e_SC_RESULT_SUCCESS != result) {
						SC_LOG_ErrorPrint(SC_TAG_RC, "setStartCandFormJoin error. [0x%08x] "HERE, result);
						return (result);
					}
					candRoute->stLinkSize += count;
				}
			}
		}
		// ▼Join or Split接続パーセル
		else if (0 != pclInfo->areaJoin || 0 != pclInfo->areaSplit) {
			linkInfo = aNetCtrl->linkTable + pclInfo->linkIdx;
			for (e = 0; e < pclInfo->linkIdVol; e++, linkInfo++) {
				SCRP_NETDATA* netData = linkInfo->linkNet;
				for (or = 0; or < 2; or++, netData++) {
					if (SCRP_HEAP_V != netData->heap) {
						continue;
					}
					if (RCND_GET_STARTLINKFLG(netData->flag)) {
						continue;
					}
					UINT32 count = 0;
					if (RCND_GET_CANDJOINFLG(netData->flag)) {
						result = setSplitStartCandFormJoin(aSectCtrl, pclInfo, linkInfo, netData, setLinkExDataConnect, &count);
					} else if (RCND_GET_CANDSPLITFLG(netData->flag)) {
						result = setSplitStartCandFormJoin(aSectCtrl, pclInfo, linkInfo, netData, setLinkExDataConnect, &count);
					} else {
						continue;
					}
					if (e_SC_RESULT_SUCCESS != result) {
						SC_LOG_ErrorPrint(SC_TAG_RC, "setStartCandFormJoin error. [0x%08x] "HERE, result);
						return (result);
					}
					candRoute->stLinkSize += count;
				}
			}
		}
	}
	if (0 == candRoute->stLinkSize) {
		SC_LOG_InfoPrint(SC_TAG_RC, "this divarea can't found start cand link... divIdx=%d " HERE, aNetCtrl->calculatingDivIdx);
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 目的地パーセルの候補開始リンクを回収
 * @param 区間管理
 * @param パーセル情報
 * @param リンク情報
 * @param リンク拡張情報格納用関数ポインタ
 * @param [O]リンク格納数 成功で1が格納される
 * @memo レベル１トップ及びレベル２トップの下位非接続
 */
static E_SC_RESULT setSplitStartCandFormJoin(SCRP_SECTCONTROLER* aSectCtrl, SCRP_PCLINFO* aPclInfo, SCRP_LINKINFO* aLinkInfo,
		SCRP_NETDATA* aNetData, SET_CONNECT_FUNC pFunc, UINT32 *aCount) {

	if (NULL == aSectCtrl || NULL == aPclInfo || NULL == aLinkInfo || NULL == aCount || NULL == pFunc) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SCRP_CANDMANAGER* candMng = &aSectCtrl->candMng;
	SCRP_CANDSTARTLINK* stLink = NULL;
	UINT32 i;
	Bool sameLinkFlag = false;

	// 初期化
	*aCount = 0;
	// ネットワークデータ取得
	T_MapNWLink* pLink = (T_MapNWLink*) SC_MA_A_NWRCD_LINK_GET_RECORD(aPclInfo->mapNetworkLinkBin, aLinkInfo->detaIndex);

	// 同一リンク登録済み確認
	for (i = 0; i < candMng->splitStLinkCurrent; i++) {
		stLink = candMng->splitStLink + i;
		if (aPclInfo->parcelId == stLink->parcelId && pLink->id == stLink->linkId
				&& (RCND_GET_ORIDX(aNetData->flag ) == RCND_GET_ORIDX(stLink->flag ))) {
			break;
		}
	}
	if (i < candMng->splitStLinkCurrent) {
		// 下位接続情報格納処理の場合
		if (setLinkExDataDownLvConnect == pFunc) {
			// 下位接続情報生成済みであるならば情報を書き換えて終了
			if ((candMng->splitStLink + i)->connectLevel == RP_LEVEL1) {
				if ((candMng->splitStLink + i)->cost > aNetData->costSum) {
					(candMng->splitStLink + i)->candIdx = RC_CAND_STLINK;
					(candMng->splitStLink + i)->flag = aNetData->flag;
					(candMng->splitStLink + i)->cost = aNetData->costSum;
					(candMng->splitStLink + i)->netIdx = RCID_MAKE_RCID(aPclInfo->linkIdx + aLinkInfo->detaIndex, aPclInfo->index,
							RCND_GET_ORIDX(aNetData->flag));
				}
				return (e_SC_RESULT_SUCCESS);
			} else {
				// 下位接続情報としてデータを書き換え
				sameLinkFlag = true;
			}
		} else {
			// 低コストのデータを採用する
			if ((candMng->splitStLink + i)->cost > aNetData->costSum) {
				(candMng->splitStLink + i)->candIdx = RC_CAND_STLINK;
				(candMng->splitStLink + i)->flag = aNetData->flag;
				(candMng->splitStLink + i)->cost = aNetData->costSum;
				(candMng->splitStLink + i)->netIdx = RCID_MAKE_RCID(aPclInfo->linkIdx + aLinkInfo->detaIndex, aPclInfo->index,
						RCND_GET_ORIDX(aNetData->flag));
			}
			return (e_SC_RESULT_SUCCESS);
		}
	}

	// サイズチェック
	if (candMng->splitStLinkSize <= candMng->splitStLinkCurrent) {
		result = mallocSplitStLink(candMng, RC_CAND_STLINK_SIZE);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "mallocSplitStLink error. [0x%08x] "HERE, result);
			return (result);
		}
	}

	// 候補格納アドレス取得
	if (sameLinkFlag) {
		stLink = candMng->splitStLink + i;
	} else {
		stLink = CAND_GET_CRNTSPLITSTLINK(candMng);
	}

	// 結果格納
	stLink->candIdx = RC_CAND_STLINK;
	stLink->dataIdx = aLinkInfo->detaIndex;
	stLink->cost = aNetData->costSum;
	stLink->flag = aNetData->flag;
	stLink->parcelId = aPclInfo->parcelId;
	stLink->linkId = pLink->id;
	stLink->linkKind = pLink->linkBaseInfo.b_code.linkKind1;
	stLink->roadKind = pLink->linkBaseInfo.b_code.roadKind;
	stLink->netIdx = RCID_MAKE_RCID(aPclInfo->linkIdx + aLinkInfo->detaIndex, aPclInfo->index, RCND_GET_ORIDX(aNetData->flag));

	// 上位下位接続情報（拡張情報内のデータを格納する）
	result = pFunc(aPclInfo, pLink, stLink);
	if (e_SC_RESULT_SUCCESS != result) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "func error. [0x%08x] "HERE, result);
		return (result);
	}

	// 格納数加算
	if (!sameLinkFlag) {
		*aCount = 1;
	}

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief リンク拡張データ格納処理：同一レベル接続情報
 * @param パーセル情報
 * @param 地図データポインタ：リンク情報
 * @param 候補開始リンクアドレス
 */
static E_SC_RESULT setLinkExDataConnect(SCRP_PCLINFO* aPclInfo, T_MapNWLink* aPLink, SCRP_CANDSTARTLINK* aCandStLink) {

	if (NULL == aPclInfo || NULL == aPLink || NULL == aCandStLink) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 念のための初期化
	aCandStLink->connectLevel = 0;
	aCandStLink->st.linkId = 0;
	aCandStLink->st.parcelId = 0;
	aCandStLink->st.pclSftX = 0;
	aCandStLink->st.pclSftY = 0;

	// リンク情報から形状取得
	if (!SC_MA_D_NWRCD_LINK_GET_LINKEXOFSFLG(aPLink)) {
		aCandStLink->formOfs = aPLink->exOrShape;
		return (e_SC_RESULT_SUCCESS);
	}
	// リンク拡張情報から形状取得
	MAL_HDL exLink = (aPclInfo->mapNetworkLinkExBin + SC_MA_D_NWRCD_LINK_GET_EXOFS(aPLink) * 4);
	if (SC_MA_D_NWRCD_EXLINK_GET_FLG_FORMOFS(exLink)) {
		aCandStLink->formOfs = SC_MA_D_NWRCD_EXLINK_GET_FORMOFS(exLink);
	}
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief リンク拡張データ格納処理：上位接続情報
 * @param パーセル情報
 * @param 地図データポインタ：リンク情報
 * @param 候補開始リンクアドレス
 * @memo TODO 1つはデータがある前提のつくり
 */
static E_SC_RESULT setLinkExDataUpLvConnect(SCRP_PCLINFO* aPclInfo, T_MapNWLink* aPLink, SCRP_CANDSTARTLINK* aCandStLink) {

	if (NULL == aPclInfo || NULL == aPLink || NULL == aCandStLink) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 初期化
	aCandStLink->connectLevel = 0;

	// リンク情報から形状取得
	if (!SC_MA_D_NWRCD_LINK_GET_LINKEXOFSFLG(aPLink)) {
		aCandStLink->formOfs = aPLink->exOrShape;
		return (e_SC_RESULT_SUCCESS);
	}
	// リンク拡張情報から形状取得
	MAL_HDL exLink = (aPclInfo->mapNetworkLinkExBin + SC_MA_D_NWRCD_LINK_GET_EXOFS(aPLink) * 4);
	if (SC_MA_D_NWRCD_EXLINK_GET_FLG_FORMOFS(exLink)) {
		aCandStLink->formOfs = SC_MA_D_NWRCD_EXLINK_GET_FORMOFS(exLink);
	}
	// 上位レベル
	if (SC_MA_D_NWRCD_EXLINK_GET_FLG_UPLV(exLink)) {
		// 上位接続リンクID取得
		MAL_HDL uplvLink = SC_MA_A_NWRCD_EXLINK_GET_UPLVLINK(exLink);
		aCandStLink->st.linkId = read4byte(uplvLink);
		aCandStLink->st.parcelId = SC_MESH_GetUpperParcelID(aPclInfo->parcelId);
		aCandStLink->st.pclSftX = 0;
		aCandStLink->st.pclSftY = 0;

		// 自リンク逆方向の場合上位接続リンク方向反転
		if (SCRP_LINKRVS == RCND_GET_ORIDX(aCandStLink->flag)) {
			if (0x0000000F != SC_MA_D_NWRCD_EXLINK_GET_UPLVINFO_STLV(exLink)) {
				if (aCandStLink->st.linkId & 0x02000000) {
					aCandStLink->st.linkId = (aCandStLink->st.linkId & 0xF9FFFFFF) | 0x04000000;
				} else {
					aCandStLink->st.linkId = (aCandStLink->st.linkId & 0xF9FFFFFF) | 0x02000000;
				}
			} else {
				// データ上ありえない
				SC_LOG_ErrorPrint(SC_TAG_RC, "error. "HERE);
				return (e_SC_RESULT_FAIL);
			}
		}
	}

	// 接続レベル登録
	aCandStLink->connectLevel = RP_LEVEL2;

#if 0
	SC_LOG_InfoPrint(SC_TAG_RC, "up level connect is ... 0x%08x .-> 0x%08x pcl=0x%08x", aPLink->id, aCandStLink->st.linkId,
			aCandStLink->st.parcelId);
#endif
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief リンク拡張データ格納処理：下位接続情報
 * @param パーセル情報
 * @param 地図データポインタ：リンク情報
 * @param 候補開始リンクアドレス
 * @memo 下位レベル接続フラグ(SCRP_CANDSTARTLINK#flag)は立たないケースも存在する。下位の接続は接続レベルで判断すること
 */
static E_SC_RESULT setLinkExDataDownLvConnect(SCRP_PCLINFO* aPclInfo, T_MapNWLink* aPLink, SCRP_CANDSTARTLINK* aCandStLink) {

	if (NULL == aPclInfo || NULL == aPLink || NULL == aCandStLink) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 初期化
	aCandStLink->connectLevel = 0;

	if (!SC_MA_D_NWRCD_LINK_GET_LINKEXOFSFLG(aPLink)) {
		aCandStLink->formOfs = aPLink->exOrShape;
		return (e_SC_RESULT_SUCCESS);
	}
	MAL_HDL exLink = SC_MA_A_NWRCD_LINKEX_GET_RECORD(aPclInfo->mapNetworkLinkExBin, SC_MA_D_NWRCD_LINK_GET_EXOFS(aPLink));
	// 形状
	if (SC_MA_D_NWRCD_EXLINK_GET_FLG_FORMOFS(exLink)) {
		aCandStLink->formOfs = SC_MA_D_NWRCD_EXLINK_GET_FORMOFS(exLink);
	}
	// 下位レベル
	if (SC_MA_D_NWRCD_EXLINK_GET_FLG_DOWNLV(exLink)) {
		// 下位接続情報取得
		MAL_HDL downlvLink = SC_MA_A_NWRCD_EXLINK_GET_DOWNLVLINK(exLink);
		if (SCRP_LINKODR == RCND_GET_ORIDX(aCandStLink->flag)) {
			aCandStLink->st.linkId = read4byte(downlvLink + 4);
			aCandStLink->st.pclSftX = (read2byte(downlvLink + 10) & 0x00FF);
			aCandStLink->st.pclSftY = (read2byte(downlvLink + 10) >> 8) & 0x00FF;
		} else {
			aCandStLink->st.linkId = read4byte(downlvLink);
			aCandStLink->st.pclSftX = (read2byte(downlvLink + 8) & 0x00FF);
			aCandStLink->st.pclSftY = (read2byte(downlvLink + 8) >> 8) & 0x00FF;
		}

		// 自リンク逆方向の場合下位接続リンク方向反転
		if (SCRP_LINKRVS == RCND_GET_ORIDX(aCandStLink->flag)) {
			if (aCandStLink->st.linkId & 0x02000000) {
				aCandStLink->st.linkId = (aCandStLink->st.linkId & 0xF9FFFFFF) | 0x04000000;
			} else {
				aCandStLink->st.linkId = (aCandStLink->st.linkId & 0xF9FFFFFF) | 0x02000000;
			}
		}

		// パーセルID作成
		aCandStLink->st.parcelId = SC_MESH_GetUnderLevelParcelID(aPclInfo->parcelId, RP_LEVEL1);
		aCandStLink->st.parcelId = SC_MESH_SftParcelId(aCandStLink->st.parcelId, (INT16) aCandStLink->st.pclSftX,
				(INT16) aCandStLink->st.pclSftY);

		// 接続レベル登録
		aCandStLink->connectLevel = RP_LEVEL1;
	}

#if 0
	SC_LOG_InfoPrint(SC_TAG_RC, "down level connect is ... pcl=0x%08x(%d,%d) link=0x%08x flg=0x%04x 0x%08x ", aCandStLink->st.parcelId,
			aCandStLink->st.pclSftX, aCandStLink->st.pclSftY, aCandStLink->st.linkId, aCandStLink->flag, aCandStLink->netIdx);
#endif
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 候補テーブル作成（分割）
 * @param aNetTab[I]探索情報テーブル
 * @param aCandRoute[O]候補経路情報
 * @memo 当関数内では探索情報テーブルのHeapにCandのIdxを格納する
 *       分割エリア＆順探索 での候補テーブル作成処理
 *       注：レベル接続部では候補経路データにダブりが発生する。発生するダブりは推奨経路接続時に除去する。
 */
static E_SC_RESULT makeSplitCand(SCRP_SECTCONTROLER* aSectCtrl, SCRP_NETCONTROLER* aNetCtrl) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aSectCtrl || NULL == aNetCtrl) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SCRP_PCLINFO* pclInfo = NULL;
	SCRP_LINKINFO* crntLink = NULL;
	SCRP_NETDATA* crntNet = NULL;
	SCRP_NETDATA* prevNet = NULL;
	SCRP_CANDTBLINFO* candTblInfo = &aSectCtrl->candMng.splitCandTblInfo[aNetCtrl->calculatingDivIdx];
	SCRP_CANDDATA* prevCand = NULL;
	SCRP_CANDDATA* crntCand = NULL;
	SCRP_CANDMANAGER* candMng = &aSectCtrl->candMng;
	SCRP_CANDSTARTLINK* candStLink = NULL;
	SCRP_CANDSTARTLINK* wkStLink = NULL;
	UINT32 reachStCnt = 0;
	UINT32 i;

	candStLink = candMng->splitStLink;

	do {
		// 全開始リンクから
		for (i = 0; i < candMng->splitStLinkCurrent; i++, candStLink++) {
			// 初期値の場合のみ処理対象（今回区間外は低コスト時の書き換えの可能性）
			if (RC_CAND_STLINK != candStLink->candIdx) {
				continue;
			}
			// オーバーフローチェック
			if (candMng->splitCandSize <= candMng->splitCandCurrent) {
				result = mallocSplitCand(candMng, RC_CAND_SIZE);
				if (e_SC_RESULT_SUCCESS != result) {
					SC_LOG_ErrorPrint(SC_TAG_RC, "mallocSplitCand error. [0x%08x] "HERE, result);
					break;
				}
			}
			// 次回候補テーブル取得
			crntCand = CAND_GET_CRNTSPLITCAND(candMng);

			// 初回情報取得
			pclInfo = RCNET_GET_PCLINFO(aNetCtrl, candStLink->netIdx);
			crntLink = RCNET_GET_LINKINFO(aNetCtrl, candStLink->netIdx);
			crntNet = RCNET_GET_NETDATA(aNetCtrl, candStLink->netIdx);

			// 候補経路登録済みならINDEXを格納して次
			if (RCND_GET_REGISTEDCANDFLG(crntNet->flag)) {
				candStLink->candIdx = crntNet->heap;
				continue;
			}

			// 探索開始リンク
			if (RCND_GET_STARTLINKFLG(crntNet->flag)) {
				// ◆候補登録
				result = setCandData(crntCand, RC_CAND_STLINK, pclInfo, crntLink, crntNet);
				if (e_SC_RESULT_SUCCESS != result) {
					SC_LOG_ErrorPrint(SC_TAG_RC, "setCandData error. [0x%08x] "HERE, result);
					break;
				}

				//---------------------------------------------------------------------------------------
				// 探索開始リンク＆候補開始リンクである可能性。
				// 分割２区間目以降であればいずれかの分割エリアの候補開始リンク情報にデータがある。
				// 候補開始リンク情報から検索してCandIndexを格納する。
				// 見つからなければ自車近傍の開始リンクであるという判断にする。
				//---------------------------------------------------------------------------------------
				wkStLink = searchSplitCandStLink(candMng, crntCand->parcelId, crntCand->linkId, RCND_GET_ORIDX(crntCand->flag));
				if (NULL != wkStLink) {
					crntCand->next = wkStLink->candIdx;
				}
				// 開始リンク到達フラグ
				candStLink->candIdx = candMng->splitCandCurrent - 1;
				reachStCnt += 1;
				continue;
			}
			// 開始リンク以外での侵入情報なし＝不正情報
			if (ALL_F32 == crntNet->inLinkHist) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "inLinkHist is init. pcl=0x%08x link=0x%08x idx=0x%08x flag=0x%04x connect=%d "HERE,
						candStLink->parcelId, candStLink->linkId, candStLink->netIdx, candStLink->flag, candStLink->connectLevel);
				result = e_SC_RESULT_FAIL;
				break;
			}

			// ◆候補登録
			result = setCandData(crntCand, candMng->splitCandCurrent, pclInfo, crntLink, crntNet);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "setCandData error. [0x%08x] "HERE, result);
				break;
			}
			candStLink->candIdx = candMng->splitCandCurrent - 1;

			// 候補登録済みマーク
			CAND_SET_REGISTMARK(crntNet, candMng->splitCandCurrent - 1);

			// オーバーフローチェック
			if (candMng->splitCandSize <= candMng->splitCandCurrent) {
				result = mallocSplitCand(candMng, RC_CAND_SIZE);
				if (e_SC_RESULT_SUCCESS != result) {
					SC_LOG_ErrorPrint(SC_TAG_RC, "mallocSplitCand error. [0x%08x] "HERE, result);
					break;
				}
			}
			// 前回・次回候補テーブル取得
			prevCand = CAND_GET_PREVSPLITCAND(candMng);
			crntCand = CAND_GET_CRNTSPLITCAND(candMng);

			// Index指定でリンク情報テーブル取得
			prevNet = crntNet;
			// 侵入リンク情報取得
			pclInfo = RCNET_GET_HISTPCLINFO(aNetCtrl, prevNet);
			crntLink = RCNET_GET_HISTLINKINFO(aNetCtrl, prevNet);
			crntNet = RCNET_GET_HISTNETDATA(aNetCtrl, prevNet);

			while (crntLink) {
				// 候補経路登録済み確認
				if (RCND_GET_REGISTEDCANDFLG(crntNet->flag)) {
					// 登録済みのIndexを与えてブレイク
					prevCand->next = crntNet->heap;
					break;
				}
				// 計算済みの場合、不正情報につき接続関係を切断
				if (SCRP_HEAP_V != crntNet->heap) {
					prevCand->next = RC_CAND_INIT;
					SC_LOG_ErrorPrint(SC_TAG_RC, "heap connect error. heap=%d "HERE, prevNet->heap);
					result = e_SC_RESULT_FAIL;
					break;
				}
				// 探索開始リンク確認
				if (RCND_GET_STARTLINKFLG(crntNet->flag)) {
					// →候補登録
					result = setCandData(crntCand, RC_CAND_STLINK, pclInfo, crntLink, crntNet);
					if (e_SC_RESULT_SUCCESS != result) {
						SC_LOG_ErrorPrint(SC_TAG_RC, "setCandData error. [0x%08x] "HERE, result);
						break;
					}

					// 探索開始地点でなければ候補INDEX張り替え
					wkStLink = searchSplitCandStLink(candMng, pclInfo->parcelId, crntCand->linkId, RCND_GET_ORIDX(crntNet->flag));
					if (NULL != wkStLink) {
						prevCand->next = wkStLink->candIdx;
					}
					// 開始リンク到達フラグ
					reachStCnt += 1;
					break;
#if 0
					SC_LOG_InfoPrint(SC_TAG_RC, "[cand] 0x%08x 0x%08x( %x ) cost=%d  cand=%d next=%d", crntCand->parcelId, crntCand->linkId,
							crntCand->flag, crntCand->cost, candMng->splitCandCurrent - 1, crntCand->next);
#endif
				}
				// 開始リンク以外での侵入情報なしの場合、不正情報につき接続関係を切断
				if (ALL_F32 == crntNet->inLinkHist) {
					prevCand->next = RC_CAND_INIT;
					SC_LOG_ErrorPrint(SC_TAG_RC, "inLink not found. idx=%d "HERE, crntLink->detaIndex);
					result = e_SC_RESULT_FAIL;
					break;
				}

				// ◆候補登録
				result = setCandData(crntCand, candMng->splitCandCurrent, pclInfo, crntLink, crntNet);
				if (e_SC_RESULT_SUCCESS != result) {
					SC_LOG_ErrorPrint(SC_TAG_RC, "setCandData error. [0x%08x] "HERE, result);
					break;
				}

				// 候補登録済みマーク
				CAND_SET_REGISTMARK(crntNet, candMng->splitCandCurrent - 1);

				// オーバーフローチェック
				if (candMng->splitCandSize <= candMng->splitCandCurrent) {
					result = mallocSplitCand(candMng, RC_CAND_SIZE);
					if (e_SC_RESULT_SUCCESS != result) {
						SC_LOG_ErrorPrint(SC_TAG_RC, "mallocSplitCand error. [0x%08x] "HERE, result);
						break;
					}
				}
				// 前回・次回候補テーブル取得
				prevCand = CAND_GET_PREVSPLITCAND(candMng);
				crntCand = CAND_GET_CRNTSPLITCAND(candMng);

				// 次回探索情報テーブル取得
				prevNet = crntNet;
				pclInfo = RCNET_GET_HISTPCLINFO(aNetCtrl, prevNet);
				crntLink = RCNET_GET_HISTLINKINFO(aNetCtrl, prevNet);
				crntNet = RCNET_GET_HISTNETDATA(aNetCtrl, prevNet);

#if 0
				SC_LOG_InfoPrint(SC_TAG_RC, "pcl=0x%08x link=0x%08x(flg=0x%04x) cost=%6d  cand=%5d  next...-> flag=0x%04x, hist=0x%08x", prevCand->parcelId,
						prevCand->linkId, prevCand->flag, prevCand->cost, candMng->splitCandCurrent - 1, crntNet->flag, crntNet->inLinkHist);
#endif
				// 注：break前に必ずCandにNextを格納する
			}
			if (result != e_SC_RESULT_SUCCESS) {
				break;
			}
		}
		if (result != e_SC_RESULT_SUCCESS) {
			break;
		}
		// 候補経路設定数
		candTblInfo->candSize = (candMng->splitCandCurrent - candTblInfo->candIdx);
	} while (0);

	// 開始リンク非到達：分割エリアの場合許容値
	if (0 == reachStCnt) {
		SC_LOG_InfoPrint(SC_TAG_RC, "this area is not reach start link. "HERE);
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief 候補開始リンクリスト検索
 * @param 候補経路管理
 * @param パーセルID
 * @param リンクID
 * @param リンク方向
 */
static SCRP_CANDSTARTLINK* searchSplitCandStLink(SCRP_CANDMANAGER* aCandMng, UINT32 aPclId, UINT32 aLinkId, UINT8 aDir) {

	SCRP_CANDSTARTLINK* findStLink = NULL;
	SCRP_CANDSTARTLINK* stLink = NULL;
	SCRP_CANDTBLINFO* route = NULL;
	UINT32 i, e;

	if (NULL == aCandMng) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (NULL );
	}

	for (i = 0; i < aCandMng->splitCandTblInfoVol; i++) {
		route = (aCandMng->splitCandTblInfo + i);
		stLink = aCandMng->splitStLink + route->stLinkIdx;

		// 複数HITはあり得る
		for (e = 0; e < route->stLinkSize; e++, stLink++) {
			if (aPclId == stLink->parcelId && aLinkId == stLink->linkId && aDir == RCND_GET_ORIDX(stLink->flag)) {
				// 初回区間以外で開始リンクの場合
				if (0 != i && RC_CAND_STLINK == stLink->candIdx) {
					SC_LOG_WarnPrint(SC_TAG_RC, "unknown start link... "HERE);
					continue;
				}
				findStLink = stLink;
				break;
			}
		}
	}
	return (findStLink);
}

/**
 * @brief 分割候補を真候補経路へ複写
 * @param 候補経路管理
 * @param 探索ステップ
 * @memo 候補経路のコピーは使用しない可能性のあるデータを含みます。
 */
static E_SC_RESULT copyLatestSplitCand(SCRP_CANDMANAGER* aCandMng, E_RP_RTCALCSTEP aStep) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aCandMng) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 候補経路のないステップは許容する
	if (0 == aCandMng->splitStLinkCurrent || 0 == aCandMng->splitCandCurrent) {
		SC_LOG_WarnPrint(SC_TAG_RC, "this step have no cand. step=%d "HERE, aStep);
		return (e_SC_RESULT_SUCCESS);
	}

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SCRP_CANDDATA* cand = NULL;
	SCRP_CANDDATA* splitCand = NULL;
	SCRP_CANDSTARTLINK* stLink = NULL;
	SCRP_CANDSTARTLINK* splitStLink = NULL;
	SCRP_CANDTBLINFO* route = NULL;
	UINT32 i = 0;
	UINT32 size = 0;

	// 管理情報作成
	switch (aStep) {
	case e_RP_STEP_LV1TOP:
		route = &aCandMng->candTblInfo[RC_CAND_IDX_LV1TOP];
		break;
	case e_RP_STEP_LV1O:
		route = &aCandMng->candTblInfo[RC_CAND_IDX_LV1O];
		break;
	case e_RP_STEP_LV1D:
		route = &aCandMng->candTblInfo[RC_CAND_IDX_LV1D];
		break;
	case e_RP_STEP_LV2TOP:
		route = &aCandMng->candTblInfo[RC_CAND_IDX_LV2TOP];
		break;
	default:
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	route->candIdx = aCandMng->candCurrent;
	route->candSize = aCandMng->splitCandCurrent;
	route->stLinkIdx = aCandMng->stLinkCurrent;
	route->stLinkSize = aCandMng->splitStLinkCurrent;

	// ▼候補開始リンク複写
	// オーバーフローチェック
	if ((aCandMng->stLinkSize - aCandMng->stLinkCurrent) < route->stLinkSize) {
		size = aCandMng->stLinkCurrent + route->stLinkSize;
		result = mallocStLink(aCandMng, route->stLinkSize);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "mallocStLink error. [0x%08x] "HERE, result);
			return (result);
		}
	}
	// split候補開始をマスタへコピー
	stLink = aCandMng->stLink + aCandMng->stLinkCurrent;
	splitStLink = aCandMng->splitStLink;
	RP_Memcpy(stLink, splitStLink, sizeof(SCRP_CANDSTARTLINK) * route->stLinkSize);

	// 候補開始リストINDEX張り替え
	for (i = 0; i < route->stLinkSize; i++) {
		(stLink + i)->candIdx += route->candIdx;
	}
	// 作業位置更新
	aCandMng->stLinkCurrent += route->stLinkSize;

	// ▼候補リンクコピー複写
	// オーバーフローチェック
	if ((aCandMng->candSize - aCandMng->candCurrent) < route->candSize) {
		size = aCandMng->candCurrent + route->candSize;
		result = mallocCandData(aCandMng, route->candSize);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "mallocCandData error. [0x%08x] "HERE, result);
			return (result);
		}
	}
	// split候補をマスタへコピー
	cand = aCandMng->cand + aCandMng->candCurrent;
	splitCand = aCandMng->splitCand;
	RP_Memcpy(cand, splitCand, sizeof(SCRP_CANDDATA) * route->candSize);

	// 候補リストINDEX張り替え
	for (i = 0; i < route->candSize; i++) {
		if (RC_CAND_INIT <= (cand + i)->next) {
			continue;
		}
		(cand + i)->next += route->candIdx;
	}
	// 作業位置更新
	aCandMng->candCurrent += route->candSize;

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 候補経路リンク情報登録処理
 * @param [0]候補データアドレス
 * @param 次候補データIndex
 * @param パーセル情報
 * @param リンク情報
 * @param リンク情報
 */
static E_SC_RESULT setCandData(SCRP_CANDDATA* aCandData, UINT32 aNextCand, SCRP_PCLINFO* aPclInfo, SCRP_LINKINFO* aLinkInfo,
		SCRP_NETDATA* aNetData) {

	// パラメータチェック
	if (NULL == aPclInfo || NULL == aLinkInfo || NULL == aNetData) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	// アドレス取得
	MAL_HDL pLink = SC_MA_A_NWRCD_LINK_GET_RECORD(aPclInfo->mapNetworkLinkBin, aLinkInfo->detaIndex);

	aCandData->next = aNextCand;
	aCandData->parcelId = aPclInfo->parcelId;
	aCandData->cost = aNetData->costSum;
	RC_SET_FLAG_NW2CAND(aNetData->flag, aCandData->flag);
	aCandData->dataIdx = aLinkInfo->detaIndex;
	aCandData->linkId = SC_MA_D_NWRCD_LINK_GET_ID(pLink);

	// 形状オフセット格納（*4しない）
	if (!SC_MA_D_NWRCD_LINK_GET_LINKEXOFSFLG(pLink)) {
		aCandData->formOfs = SC_MA_D_NWRCD_LINK_GET_FORMOFS(pLink);
	} else {
		MAL_HDL exLink = SC_MA_A_NWRCD_LINKEX_GET_RECORD(aPclInfo->mapNetworkLinkExBin, SC_MA_D_NWRCD_LINK_GET_EXOFS(pLink));
		if (SC_MA_D_NWRCD_EXLINK_GET_FLG_FORMOFS(exLink)) {
			aCandData->formOfs = SC_MA_D_NWRCD_EXLINK_GET_FORMOFS(exLink);
		}
	}

	return (e_SC_RESULT_SUCCESS);
}
