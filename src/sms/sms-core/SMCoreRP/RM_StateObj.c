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
 * RM_StateObj.c
 *
 * 経路探索状態・探索条件管理
 *
 *  Created on: 2015/08/25
 *      Author: masutani
 */

#include "SMCoreRPInternal.h"

#define RP_LAPTIME_SIZE				100		/* 探索性能測定ラップタイムバッファサイズ */

static clock_t mLapTime[RP_LAPTIME_SIZE] = {};
static Char mLapTimeMsg[RP_LAPTIME_SIZE][64] = {};
static UINT16 mLapIdx = 0;
static SCRP_RPMSTATE mRPMState = {};
static SCRP_RPCSTATE mRPCState = {};

/* プロトタイプ宣言 */
static E_SC_RESULT addValidIdx();
static E_SC_RESULT removeValidIdx();
static Bool checkValidIdx(UINT16 aIdx);
static E_SC_RESULT makeRouteSearchSetting(pthread_msq_msg_t* aMsg, SCRP_SEARCHSETTING *aSetting);

/**
 * @brief 性能測定：ラップタイム記録
 */
void RP_SetLapTime() {
	if (RP_LAPTIME_SIZE <= mLapIdx) {
//		SC_LOG_InfoPrint((Char*) "SMCoreRC-Time", "rp lap time size over flow.");
		return;
	}
	memset(&mLapTimeMsg[mLapIdx][0], 0, 64);
	mLapTime[mLapIdx++] = clock();
}

/**
 * @brief 性能測定：ラップタイム記録(MSG付き)
 */
void RP_SetLapTimeWithStr(Char *msg) {
	if (RP_LAPTIME_SIZE <= mLapIdx) {
//		SC_LOG_InfoPrint((Char*) "SMCoreRC-Time", "rp lap time size over flow.");
		return;
	}
	strcpy(&mLapTimeMsg[mLapIdx][0], msg);
	mLapTime[mLapIdx++] = clock();
}

/**
 * @brief 性能測定：ラップタイムクリア
 */
void RP_ClearLapTime() {
	mLapIdx = 0;
	RP_Memset0(&mLapTime[0], sizeof(clock_t) * RP_LAPTIME_SIZE);
	RP_Memset0(&mLapTimeMsg[0], 64 * RP_LAPTIME_SIZE);
}

/**
 * @brief 性能測定：ラップタイム出力
 */
void RP_OutputLapTime() {

	UINT16 i = 1;
	DOUBLE start = (DOUBLE) mLapTime[0] / CLOCKS_PER_SEC;
	DOUBLE total = 0;

	while ((LONG) mLapTime[i] != 0) {
		DOUBLE diff = ((DOUBLE) mLapTime[i] / CLOCKS_PER_SEC) - ((DOUBLE) mLapTime[i - 1] / CLOCKS_PER_SEC);
		SC_LOG_InfoPrint((Char*) "SMCoreRC-Time", "lap  %8f (%s)", diff, &mLapTimeMsg[i][0]);
		i++;
	}
	//total = ((DOUBLE) mLapTime[i - 1] / CLOCKS_PER_SEC) - start;
	clock_t now = clock();
	total = ((DOUBLE) now / CLOCKS_PER_SEC) - start;

	SC_LOG_InfoPrint((Char*) "SMCoreRC-Time", "total  %8f", total);
}

/**
 * @brief 探索結果共有メモリ設定
 * @return 共有メモリ設定結果
 */
E_SC_RESULT RP_SetRoutePlanTip() {

	SC_DH_SHARE_RPTIPINFO tip = {};

	// 再探索
	tip.tipInfo.isRePlan = mRPCState.isReplan;
	// 探索結果＋ワーニングコード
	tip.tipInfo.tipClass = mRPCState.errorCode;
	tip.tipInfo.warnCode = mRPCState.warnCode;
	// 探索失敗区間インデックス
	tip.tipInfo.tipIndex = (BYTE) mRPCState.calcSect;
	// オプションにはプロセスを設定
	tip.tipInfo.appendOption = mRPCState.process;

	return (SC_MNG_SetRPTip(&tip.tipInfo));
}

/**
 * @brief RP探索条件，探索状態管理情報初期化
 * @return e_SC_RESULT_SUCCESS：正常終了
 * @return e_SC_RESULT_SUCCESS以外：異常終了
 */
E_SC_RESULT RPM_InitState() {

	// 初期化
	RP_Memset0(&mRPMState, sizeof(SCRP_RPMSTATE));

	// ステータスを初期状態へ
	mRPMState.rpState = e_RC_STATE_INIT;
	mRPMState.invalidIdx = 0;
	mRPMState.invalidVol = RP_SEARCHSETTING_SIZE;
	mRPMState.validIdx = 0;
	mRPMState.validVol = 0;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief RPステータス取得関数
 */
E_RP_STATE RPM_GetState() {
	return (mRPMState.rpState);
}

/**
 * @brief RPステータス設定関数
 */
void RPM_SetState(E_RP_STATE aState) {
	mRPMState.rpState = aState;
}

/**
 * @brief 探索条件をリングバッファへ設定
 * @param 受信メッセージ（FM->）
 * @param 検索ID（1要求毎に異なる）
 * @param [O]探索条件が設定されたインデックス
 */
E_SC_RESULT RPM_AddRouteSearchSetting(pthread_msq_msg_t *aMsg, UINT32 aSearchId, UINT16 *aResultIdx) {

	if (NULL == aMsg || NULL == aResultIdx) {
		SC_LOG_ErrorPrint(SC_TAG_RM, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 現在の探索設定を取得
	SCRP_SEARCHSETTING newSearchSetting = {};
	E_SC_RESULT result = makeRouteSearchSetting(aMsg, &newSearchSetting);
	if (e_SC_RESULT_SUCCESS != result) {
		SC_LOG_ErrorPrint(SC_TAG_RM, "makeRouteSearchSetting error. [0x%08x] "HERE, result);
		return (result);
	}

	// 検索ID設定(受信タイミングによる不正防止)
	newSearchSetting.routeSearchRequestId = aSearchId;

	// キューへ詰める（有効が1拡張される）
	UINT16 setIdx = mRPMState.invalidIdx;
	result = addValidIdx();
	if (e_SC_RESULT_SUCCESS != result) {
		SC_LOG_ErrorPrint(SC_TAG_RM, "addValidIdx error. [0x%08x] "HERE, result);
		return (result);
	}

	// 有効テーブル末端へ設定する
	RP_Memcpy(&mRPMState.queueRequest[setIdx], &newSearchSetting, sizeof(SCRP_SEARCHSETTING));
	*aResultIdx = setIdx;

#if 0
	SC_LOG_InfoPrint(SC_TAG_RM, "valid_%d,%d invalid_%d,%d %d", mRPMState.validIdx, mRPMState.validVol, mRPMState.invalidIdx,
			mRPMState.invalidVol, setIdx);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 探索条件をリングバッファから削除
 */
E_SC_RESULT RPM_RemoveRouteSearchSetting() {
	return (removeValidIdx());
}

/**
 * @brief 探索条件をリングバッファから取得(Index指定)
 */
SCRP_SEARCHSETTING* RPM_GetRouteSearchSetting(UINT16 aIdx) {

	// 有効Idxかチェック
	if (!checkValidIdx(aIdx)) {
		SC_LOG_ErrorPrint(SC_TAG_RM, "checkValidIdx invalid. idx=%d "HERE, aIdx);
		return (NULL );
	}
	return (&mRPMState.queueRequest[aIdx]);
}

/**
 * @brief 探索条件をリングバッファから取得(カレント)
 */
SCRP_SEARCHSETTING* RPM_GetCurrentRouteSearchSetting() {
	return (RPM_GetRouteSearchSetting(RPC_GetCurrentSettingIdx()));
}

/**
 * @brief 探索条件比較関数
 * @return true:一致
 * @return false:不一致
 */
Bool RPM_JudgeSearchSetting(const SCRP_SEARCHSETTING *aSrcA, const SCRP_SEARCHSETTING *aSrcB) {

	// フェリー利用・各種規制考慮・有料道路利用・交通情報利用
	if (aSrcA->b_setting.ferry != aSrcB->b_setting.ferry) {
		return (false);
	}
	if (aSrcA->b_setting.seasonReg != aSrcB->b_setting.seasonReg) {
		return (false);
	}
	if (aSrcA->b_setting.timeReg != aSrcB->b_setting.timeReg) {
		return (false);
	}
	if (aSrcA->b_setting.toll != aSrcB->b_setting.toll) {
		return (false);
	}
	if (aSrcA->b_setting.trafficJam != aSrcB->b_setting.trafficJam) {
		return (false);
	}
	// 探索条件
	if (aSrcA->option.rpCond != aSrcB->option.rpCond) {
		return (false);
	}
	// 全地点比較
	if (aSrcA->pointNum != aSrcB->pointNum) {
		return (false);
	}
	UINT16 lp;
	for (lp = 0; lp < aSrcA->pointNum; lp++) {
		if ((aSrcA->rpPoint[lp].parcelId != aSrcB->rpPoint[lp].parcelId) || (aSrcA->rpPoint[lp].x != aSrcB->rpPoint[lp].x)
				|| (aSrcA->rpPoint[lp].y != aSrcB->rpPoint[lp].y) || (aSrcA->point[lp].placeType != aSrcB->point[lp].placeType)
				|| (aSrcA->point[lp].cond != aSrcB->point[lp].cond)) {
			return (false);
		}
	}
	return (true);
}

/**
 * @brief 直近の有効データに対してキャンセルフラグを立てる
 *        有効データがない場合は何もしない
 */
void RPM_CancelFlagOn() {
	if (mRPMState.validVol == 0) {
		SC_LOG_ErrorPrint(SC_TAG_RM, "valid vol is zero. "HERE);
		return;
	}
	SCRP_SEARCHSETTING* setting = RPM_GetCurrentRouteSearchSetting();
	if (NULL == setting) {
		SC_LOG_ErrorPrint(SC_TAG_RM, "RPM_GetCurrentRouteSearchSetting error. "HERE);
		return;
	}
	setting->cancel = true;
}

/**
 * @brief 経路計算初期化
 * @memo メモリ解放等は行わずテーブル内容を初期化する
 */
void RPC_InitState() {

	mRPCState.process = e_RC_PROC_STANDBY;
	mRPCState.currentIdx = ALL_F16;
	mRPCState.isReplan = false;
	mRPCState.errorCode = EC_OK;
	mRPCState.warnCode = EC_OK;
	mRPCState.totalSect = 0;
	mRPCState.calcSect = 0;
	mRPCState.totalSplit = 0;
	mRPCState.calcSplit = 0;
	RPC_InitRouteCalcManager();
}

/**
 * 経路計算ステータス情報取得
 */
SCRP_RPCSTATE *RPC_GetState() {
	return (&mRPCState);
}

/**
 * 探索条件インデックス格納
 * @param 探索条件インデックス
 * @memo 探索開始時に必ず実行すること
 */
void RPC_SetCurrentSettingIdx(INT16 index) {
	SC_LOG_InfoPrint(SC_TAG_RM, "setCurrentSetting. index=%d "HERE, index);
	mRPCState.currentIdx = index;
}

/**
 * 探索条件インデックス取得
 * @param 探索条件インデックス
 * @memo 探索開始時に必ず実行すること
 */
UINT16 RPC_GetCurrentSettingIdx() {
	SC_LOG_InfoPrint(SC_TAG_RM, "getCurrentSetting. current=%d "HERE, mRPCState.currentIdx);
	return (mRPCState.currentIdx);
}

/**
 * プロセス格納
 * @param process プロセス
 */
void RPC_SetCalcProcess(E_RC_CALC_PROCESS process) {
	mRPCState.process = process;
}

/**
 * リルートフラグ格納
 * @param replan リルートフラグ
 */
void RPC_SetReplanFlag(Bool replan) {
	mRPCState.isReplan = replan;
}

/**
 * エラーコード格納
 * EC_OK以外の場合のみ格納する。EC_OKは基本RM_InitRouteCalcState()でのみ格納される。
 * @param error エラーコード
 */
void RPC_SetErrorCode(INT32 error) {
	if (EC_OK == mRPCState.errorCode) {
		mRPCState.errorCode = error;
	}
}

/**
 * エラーコード取得
 * @return エラーコード
 */
INT32 RPC_GetErrorCode() {
	return (mRPCState.errorCode);
}

/**
 * ワーニングコード格納
 * @param warn ワーニングコード
 *        ワーニング無し：EC_OK
 *        断裂経路：EC_ROUTE_SPLIT
 */
void RPC_SetWarnCode(INT32 warn) {
	mRPCState.warnCode = warn;
}

/**
 * 全区間数格納
 * @param sect 区間数
 */
void RPC_SetTotalSect(INT32 sect) {
	mRPCState.totalSect = sect;
}

/**
 * 計算中区間インデックス格納
 * @param sect 区間数
 */
void RPC_SetCalcSect(INT32 sect) {
	mRPCState.calcSect = sect;
}

/**
 * 全分割数格納
 * @param split 分割数
 */
void RPC_SetTotalSplit(INT32 split) {
	mRPCState.totalSplit = split;
}

/**
 * 計算中分割位置インデックス格納
 * @param split 分割位置
 */
void RPC_SetCalcSplit(INT32 split) {
	mRPCState.calcSplit = split;
}

/**
 * プロセスとリザルトから判断しエラーコードを格納する
 * @param result リザルトコード
 */
void RPC_SetProcess2ErrorCode(E_SC_RESULT result) {

	if (e_SC_RESULT_SUCCESS == result) {
		return;
	}

	switch (mRPCState.process) {
	case e_RC_PROC_STANDBY:
		RPC_SetResult2ErrorCode(result);
		break;
	case e_RC_PROC_NEIGHBOR:
		RPC_SetResult2ErrorCode(result);
		break;
	case e_RC_PROC_MAKEAREA:
		RPC_SetResult2ErrorCode(result);
		break;
	case e_RC_PROC_ROUTECALC:
		RPC_SetResult2ErrorCode(result);
		break;
	case e_RC_PROC_MAKECAND:
		if (e_SC_RESULT_FAIL == result) {
			RPC_SetErrorCode(EC_NOROUTE);				// ルートがありませんでした。
		} else if (e_SC_RESULT_MALLOC_ERR == result) {
			// 候補経路メモリオーバー
			RPC_SetErrorCode(EC_LIMITED_DISTANCE_OVER);		// 出発地から目的地までの距離が遠すぎます。
		} else {
			RPC_SetResult2ErrorCode(result);
		}
		break;
	case e_RC_PROC_MAKEROUTE:
		if (e_SC_RESULT_FAIL == result) {
			RPC_SetErrorCode(EC_NOROUTE);				// ルートがありませんでした。
		} else {
			RPC_SetResult2ErrorCode(result);
		}
		break;
	default:
		RPC_SetResult2ErrorCode(result);
	}
}

/**
 * リザルトから判断しエラーコードを格納する
 * @param result リザルトコード
 */
void RPC_SetResult2ErrorCode(E_SC_RESULT result) {

	switch (result) {
	case e_SC_RESULT_SUCCESS:
		break;
	case e_SC_RESULT_MALLOC_ERR:
		RPC_SetErrorCode(EC_FAILED);				// 探索に失敗しました。
		break;
	case e_SC_RESULT_ROUTE_CANCEL:
		RPC_SetErrorCode(EC_CANCEL);
		break;
	case e_SC_RESULT_BADPARAM:
		RPC_SetErrorCode(EC_FAILED);				// 探索に失敗しました。
		break;
	case e_SC_RESULT_FAIL:
		RPC_SetErrorCode(EC_FAILED);				// 探索に失敗しました。
		break;
	case e_SC_RESULT_AROUND_NO_LINK:
		RPC_SetErrorCode(EC_AROUND_NO_LINK);		// 周辺に道路が見つかりませんでした。
		break;
	default:
		RPC_SetErrorCode(EC_FAILED);				// 探索に失敗しました。
		break;
	}
}

/**
 * @brief キャンセルフラグ取得
 * @memo RC
 */
Bool RPC_IsCancelRequest() {
	SCRP_SEARCHSETTING* setting = RPM_GetCurrentRouteSearchSetting();
	if (NULL == setting) {
		SC_LOG_ErrorPrint(SC_TAG_RM, "RPM_GetCurrentRouteSearchSetting error. "HERE);
		return (false);
	}
	return (setting->cancel);
}

/**
 * @brief 経路計算管理取得
 */
void RPC_InitRouteCalcManager() {
	RP_Memset0(&mRPCState.routeCalcManager, sizeof(SCRP_MANAGER));
	mRPCState.routeCalcManager.isReplan = false;
}

/**
 * @brief 経路計算管理取得
 */
SCRP_MANAGER* RPC_GetRouteCalcManager() {
	return (&mRPCState.routeCalcManager);
}

/**
 * 探索条件を探索管理テーブルへ登録する UTIL
 */
void RPC_SetRoutePlanType(E_SCRP_RPTYPE type) {
	mRPCState.routeCalcManager.rpType = type;
}

/**
 * 探索条件を探索管理テーブルへ登録する UTIL
 */
E_SC_RESULT RPC_SetCurrentRouteSearchSetting() {
	SCRP_SEARCHSETTING *setting = RPM_GetRouteSearchSetting(RPC_GetCurrentSettingIdx());
	if (NULL == setting) {
		return (e_SC_RESULT_FAIL);
	}
	mRPCState.routeCalcManager.rcSetting = setting;
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 探索条件を探索管理情報へ設定する
 */
E_SC_RESULT RPC_SetRouteSearchSetting(SCRP_SEARCHSETTING *aSetting) {
	if (NULL == aSetting) {
		SC_LOG_ErrorPrint(SC_TAG_RM, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	mRPCState.routeCalcManager.rcSetting = aSetting;
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 有効なIdxかチェックする
 */
static Bool checkValidIdx(UINT16 aIdx) {

#if 0
	SC_LOG_InfoPrint(SC_TAG_RM, "valid_%d,%d invalid_%d,%d %d", mRPMState.validIdx, mRPMState.validVol, mRPMState.invalidIdx,
			mRPMState.invalidVol, aIdx);
#endif
	if (RP_SEARCHSETTING_SIZE <= aIdx) {
		return (false);
	}
	// １
	if (mRPMState.validIdx <= aIdx) {
		if (aIdx < mRPMState.validIdx + mRPMState.validVol) {
			return (true);
		} else {
			return (false);
		}
	} else {
		UINT16 valid = RP_SEARCHSETTING_SIZE - mRPMState.validIdx;
		if (aIdx < mRPMState.validVol - valid) {
			return (true);
		} else {
			return (false);
		}
	}
}

/**
 * @brief 有効データ登録（終点側へ）
 */
static E_SC_RESULT addValidIdx() {

	// 無効データ領域有無判定
	if (0 == mRPMState.invalidVol) {
		SC_LOG_ErrorPrint(SC_TAG_RM, "vol is zero. "HERE);
		return (e_SC_RESULT_FAIL);
	}
	// 有効データ加算
	mRPMState.validVol++;

	// 無効データ減算
	if (RP_SEARCHSETTING_SIZE <= (mRPMState.invalidIdx + 1)) {
		mRPMState.invalidIdx = 0;
	} else {
		mRPMState.invalidIdx++;
	}
	mRPMState.invalidVol--;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 有効データ削除（始点側を）
 */
static E_SC_RESULT removeValidIdx() {

	// 無効データ領域有無判定
	if (0 == mRPMState.validVol) {
		SC_LOG_ErrorPrint(SC_TAG_RM, "vol is zero. "HERE);
		return (e_SC_RESULT_FAIL);
	}
	// 有効データ加算
	mRPMState.invalidVol++;

	// 無効データ減算
	if (RP_SEARCHSETTING_SIZE <= (mRPMState.validIdx + 1)) {
		mRPMState.validIdx = 0;
	} else {
		mRPMState.validIdx++;
	}
	mRPMState.validVol--;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 探索条件読み込み関数
 * @param aMsg 探索要求メッセージ
 * @param aSetting 探索条件格納用
 * @return E_SC_SUCCESS 正常終了
 * TODO 再探索時のパラメタ追加
 */
static E_SC_RESULT makeRouteSearchSetting(pthread_msq_msg_t* aMsg, SCRP_SEARCHSETTING *aSetting) {

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_RPPOINT point = {};
	INT32 num = 0;
	DOUBLE x;
	DOUBLE y;
	DOUBLE lat;
	DOUBLE lon;

	do {
		if (NULL == aSetting || NULL == aMsg) {
			SC_LOG_ErrorPrint(SC_TAG_RM, "param error. "HERE);
			result = e_SC_RESULT_BADPARAM;
			break;
		}
		RP_Memset0(aSetting, sizeof(SCRP_SEARCHSETTING));

		// メッセージを記録
		RP_Memcpy(&aSetting->msg, &aMsg, sizeof(pthread_msq_msg_t));

		// キャンセルフラグをOFF
		aSetting->cancel = false;

		// 探索使用レベル 無効値の場合 MAXを設定する
		aSetting->useLevel = aMsg->data[SC_MSG_REQ_RT_USELEVEL];
		if (RP_MIN_LEVEL > aSetting->useLevel || RP_MAX_LEVEL < aSetting->useLevel) {
			aSetting->useLevel = RP_MAX_LEVEL;
		}

		// 車両情報取得
		result = SC_MNG_GetCarState(&aSetting->car, e_SC_CARLOCATION_NOW);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RM, "SC_MNG_GetCarState error. "HERE);
			break;
		}

		// 探索条件取得
		result = SC_MNG_GetRPOption(&aSetting->option);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RM, "SC_MNG_GetRPOption error. "HERE);
			break;
		}

		// 時間規制
		aSetting->b_setting.timeReg = RP_SETTING_TIMEREG_OFF;
		// 季節規制
		aSetting->b_setting.seasonReg = RP_SETTING_SEASONREG_OFF;
		// フェリー
		aSetting->b_setting.ferry = RP_SETTING_FERRY_OFF;
		// 有料道路
		aSetting->b_setting.toll = RP_SETTING_TOLL_ON;
		// 交通情報
		aSetting->b_setting.trafficJam = RP_SETTING_TRAFFICJAM_OFF;

		// 出発地、経由地、目的地取得
		result = SC_MNG_GetAllRPPlace(point.point, &point.pointNum);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RM, "SC_MNG_GetAllRPPlace error. "HERE);
			break;
		}
		// 地点数
		aSetting->pointNum = point.pointNum;

		// 地点情報コピー
		for (num = 0; num < point.pointNum; num++) {
			RP_Memcpy(&aSetting->point[num], &point.point[num], sizeof(SMRPPOINT));
		}

		// 緯度経度正規化
		for (num = 0; num < point.pointNum; num++) {
			lat = (DOUBLE) point.point[num].coord.latitude / 1024;
			lon = (DOUBLE) point.point[num].coord.longitude / 1024;
			SC_Lib_ChangeTitude2PID(lat, lon, 1, &aSetting->rpPoint[num].parcelId, &x, &y);
			aSetting->rpPoint[num].x = (UINT16) x;
			aSetting->rpPoint[num].y = (UINT16) y;
		}

		// 探索開始区間設定
		for (num = 1; num < point.pointNum; num++) {
			// 未通過地点発見
			if (false == point.point[num].isPassed) {
				aSetting->replanSect = num - 1;
				break;
			}
		}
	} while (0);

	return (result);
}
