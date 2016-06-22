/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

/*-------------------------------------------------------------------
 * File：RM_Entrance.c
 * Info：メッセージ受け口
 *-------------------------------------------------------------------*/

#include "SMCoreRPInternal.h"

static void receiveFMmsgForInit(pthread_msq_msg_t* aMsg);
static void receiveFMmsgForStandby(pthread_msq_msg_t* aMsg);
static void receiveFMmsgForRouteCalc(pthread_msq_msg_t* aMsg);
static void receiveRCmsgForInit(pthread_msq_msg_t* aMsg);
static void receiveRCmsgForStandby(pthread_msq_msg_t* aMsg);
static void receiveRCmsgForRouteCalc(pthread_msq_msg_t* aMsg);

static void sendReRouteRequestToRC(pthread_msq_msg_t* aMsg, UINT16 aSettingIdx);
static void sendSingleRouteRequestToRC(pthread_msq_msg_t* aMsg, UINT16 aSettingIdx);
static void sendReRouteResponseToFM(pthread_msq_msg_t* aMsg);
static void sendSingleRouteResponseToFM(pthread_msq_msg_t* aMsg);
static void sendFailResponseToFM(UINT32 aMsgId);
static E_SC_RESULT sendMessage(pthread_msq_id_t* aQueue, pthread_msq_msg_t* aMsg);

static void updateSearchID();
static E_SC_RESULT setRoutePlanErrorTip();

static UINT32 mSearchID = 0;		// RM<->RC 間のリクエストレスポンス整合性を保つためのID

/**
 * 送信元別メッセージ振り分け処理
 * @param [I]受信メッセージ
 */
void SC_RM_MsgAnalyze(pthread_msq_msg_t* aMsg) {
	SC_LOG_DebugPrint(SC_TAG_RM, SC_LOG_START);

	E_RP_STATE state = RPM_GetState();

	if ((SC_CORE_MSQID_FM)== pthread_msq_msg_issender(aMsg)) {
		switch(state) {
		case e_RC_STATE_INIT:
			receiveFMmsgForInit(aMsg);
			break;
		case e_RC_STATE_STANDBY:
			receiveFMmsgForStandby(aMsg);
			break;
		case e_RC_STATE_ROUTECALC:
			receiveFMmsgForRouteCalc(aMsg);
			break;
		default:
			break;
		}
	} else if (SC_CORE_MSQID_RC == pthread_msq_msg_issender(aMsg)) {
		switch(state) {
		case e_RC_STATE_INIT:
			receiveRCmsgForInit(aMsg);
			break;
		case e_RC_STATE_STANDBY:
			receiveRCmsgForStandby(aMsg);
			break;
		case e_RC_STATE_ROUTECALC:
			receiveRCmsgForRouteCalc(aMsg);
			break;
		default:
			break;
		}
	} else {
		// Unknown Sender
		SC_LOG_ErrorPrint(SC_TAG_RM, "[Msg] Unknown user message."HERE);
	}

	SC_LOG_DebugPrint(SC_TAG_RM, SC_LOG_END);
}

/**
 * RC初期状態処理
 * @param [I]受信メッセージ
 * @memo 初期状態でのメッセージ受信は受け付けない
 */
static void receiveFMmsgForInit(pthread_msq_msg_t* aMsg) {
	// error
	SC_LOG_ErrorPrint(SC_TAG_RM, "[Msg] FMMsg_IsInitProssece unknown msg receive."HERE);
	SC_LOG_DebugPrint(SC_TAG_RM, "[Msg] %x,%x,%x,%x,%x,%x,%x,", aMsg->data[0], aMsg->data[1], aMsg->data[2], aMsg->data[3], aMsg->data[4],
			aMsg->data[5], aMsg->data[6]);
}

/**
 * RC探索待機状態処理
 * @param [I]受信メッセージ
 */
static void receiveFMmsgForStandby(pthread_msq_msg_t* aMsg) {
	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	//SCRP_SEARCHSETTING *setting;
	UINT16 setIdx = 0;

	switch (aMsg->data[SC_MSG_MSG_ID]) {
	case e_SC_MSGID_REQ_RM_RTSINGLE:
		// 探索条件をリングバッファへ設定
		result = RPM_AddRouteSearchSetting(aMsg, mSearchID, &setIdx);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RM, "[Msg] RM_RoadSearchSettingObj state(0x%x)" HERE, result);
			break;
		}
		// 送信
		sendSingleRouteRequestToRC(aMsg, setIdx);
		// Id更新
		updateSearchID();
		// ステータスを計算中へ更新
		RPM_SetState(e_RC_STATE_ROUTECALC);
		break;
	case e_SC_MSGID_REQ_RM_REROUTE:
		// 探索条件をリングバッファへ設定
		result = RPM_AddRouteSearchSetting(aMsg, mSearchID, &setIdx);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RM, "[Msg] RM_RoadSearchSettingObj state(0x%x)" HERE, result);
			break;
		}
		// 送信
		sendReRouteRequestToRC(aMsg, setIdx);
		// Id更新
		updateSearchID();
		// ステータスを計算中へ更新
		RPM_SetState(e_RC_STATE_ROUTECALC);
		break;
	case e_SC_MSGID_REQ_RM_CANCEL:
		// error 非計算時何もしない
		SC_LOG_ErrorPrint(SC_TAG_RM, "[Msg] RM_GetSearchSettingObj state(0x%x)" HERE, result);
		break;
	default:
		//メッセージ内容不正
		SC_LOG_ErrorPrint(SC_TAG_RM, "[Msg] Unknown request message."HERE);
		return;
	}
	// エラー応答
	if (e_SC_RESULT_SUCCESS != result) {
		switch (aMsg->data[SC_MSG_MSG_ID]) {
		case e_SC_MSGID_REQ_RM_RTSINGLE:
			setRoutePlanErrorTip();
			sendFailResponseToFM(e_SC_MSGID_RES_FM_RTSINGLE);
			break;
		case e_SC_MSGID_REQ_RM_CANCEL:
			break;
		default:
			break;
		}
	}
}

/**
 * RC探索状態処理
 * @param [I]受信メッセージ
 */
static void receiveFMmsgForRouteCalc(pthread_msq_msg_t* aMsg) {
	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	//SCRP_SEARCHSETTING *setting;
	UINT16 setIdx = 0;

	switch (aMsg->data[SC_MSG_MSG_ID]) {
	case e_SC_MSGID_REQ_RM_RTSINGLE:
		// TODO 探索中の探索条件があるかチェック
		RPM_CancelFlagOn();

		// 探索条件をリングバッファへ設定
		result = RPM_AddRouteSearchSetting(aMsg, mSearchID, &setIdx);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RM, "[Msg] RM_RoadSearchSettingObj state(0x%x)" HERE, result);
			break;
		}
		// 送信
		sendSingleRouteRequestToRC(aMsg, setIdx);
		// Id更新
		updateSearchID();
		break;
	case e_SC_MSGID_REQ_RM_CANCEL:
		// キャンセル：次回RCからの探索応答でキャンセル応答返す。
		// キャンセルフラグON
		RPM_CancelFlagOn();
		SC_LOG_DebugPrint(SC_TAG_RM, "[Msg] RM Cancel msg receive. state(0x%x)", result);
		break;
	default:
		//非処理メッセージ
		SC_LOG_ErrorPrint(SC_TAG_RM, "[Msg] Unknown request message."HERE);
		return;
	}
	// エラー応答
	if (e_SC_RESULT_SUCCESS != result) {
		switch (aMsg->data[SC_MSG_MSG_ID]) {
		case e_SC_MSGID_REQ_RM_RTSINGLE:
			setRoutePlanErrorTip();
			sendFailResponseToFM(e_SC_MSGID_RES_FM_RTSINGLE);
			break;
		case e_SC_MSGID_REQ_RM_CANCEL:
			break;
		default:
			break;
		}
	}
}

/**
 * RC初期状態処理
 * @param [I]受信メッセージ
 * @memo 初期状態でのRCからのメッセージ受信はあり得ない
 */
static void receiveRCmsgForInit(pthread_msq_msg_t* aMsg) {
	// error
	SC_LOG_ErrorPrint(SC_TAG_RM, "[Msg] RCMsg_IsInitProssece unknown msg receive."HERE);
	SC_LOG_DebugPrint(SC_TAG_RM, "[Msg] %x,%x,%x,%x,%x,%x,%x,", aMsg->data[0], aMsg->data[1], aMsg->data[2], aMsg->data[3], aMsg->data[4],
			aMsg->data[5], aMsg->data[6]);
}

/**
 * RC探索待機状態処理
 * @param [I]受信メッセージ
 * @memo スタンバイ中のRCからのメッセージ受信はあり得ない
 */
static void receiveRCmsgForStandby(pthread_msq_msg_t* aMsg) {
	// error
	SC_LOG_ErrorPrint(SC_TAG_RM, "[Msg] RCMsg_IsStandbyProssece unknown msg receive."HERE);
	SC_LOG_DebugPrint(SC_TAG_RM, "[Msg] %x,%x,%x,%x,%x,%x,%x,", aMsg->data[0], aMsg->data[1], aMsg->data[2], aMsg->data[3], aMsg->data[4],
			aMsg->data[5], aMsg->data[6]);
}

/**
 * RC探索状態処理
 * @param [I]受信メッセージ
 */
static void receiveRCmsgForRouteCalc(pthread_msq_msg_t* aMsg) {
	switch (aMsg->data[SC_MSG_MSG_ID]) {
	case e_SC_MSGID_RES_RC_RTSINGLE:
		//応答:経路計算

		/* [キャンセル]
		 * キャンセルフラグがON状態でRCから探索終了応答があった場合、
		 * 探索応答を探索キャンセルとしてFMへ応答する。
		 * 又、キャンセルが実行されるシチュエーションは複数経路画面からの戻り操作である為
		 * 探索終了により公開済みとしている経路に関してはそのままにする。
		 */
		if (true == RPC_IsCancelRequest()) {
			if (aMsg->data[SC_MSG_RES_RT_RESULT] == e_SC_RESULT_ROUTE_CANCEL) {
				// ステータス一致：キャンセルフラグによる探索中止が成功した
				SC_LOG_DebugPrint(SC_TAG_RM, "[Msg] Cancel success.");
			} else {
				// ステータス不一致：キャンセル対象の探索と思われるのキャンセル自体の失敗とする
				SC_LOG_DebugPrint(SC_TAG_RM, "[Msg] Cancel not success. msg edit.");
			}
		}
		// 探索条件削除
		RPM_RemoveRouteSearchSetting();
		// ステータスをスタンバイへ更新
		RPM_SetState(e_RC_STATE_STANDBY);
		//探索終了MSG
		sendSingleRouteResponseToFM(aMsg);
		break;
	case e_SC_MSGID_RES_RC_REROUTE:
		//応答:経路計算
		if (true == RPC_IsCancelRequest()) {
			if (aMsg->data[SC_MSG_RES_RT_RESULT] == e_SC_RESULT_ROUTE_CANCEL) {
				// ステータス一致：キャンセルフラグによる探索中止が成功した
				SC_LOG_DebugPrint(SC_TAG_RM, "[Msg] Cancel success.");
			} else {
				// ステータス不一致：キャンセル対象の探索と思われるのキャンセル自体の失敗とする
				SC_LOG_DebugPrint(SC_TAG_RM, "[Msg] Cancel not success. msg edit.");
			}
		}
		// 探索条件削除
		RPM_RemoveRouteSearchSetting();
		// ステータスをスタンバイへ更新
		RPM_SetState(e_RC_STATE_STANDBY);
		//探索終了MSG
		sendReRouteResponseToFM(aMsg);
		break;
	default:
		//非処理メッセージ
		SC_LOG_ErrorPrint(SC_TAG_RM, "[Msg] Unknown request message."HERE);
		return;
	}
}

/**
 * 単経路探索要求メッセージ送信
 * @param [I]受信メッセージ
 * @memo RM->RC (e_SC_MSGID_REQ_RC_RTSINGLE)
 *       [0] message id
 *       [1] 設定情報使用面
 *       [2] 検索ID
 */
static void sendSingleRouteRequestToRC(pthread_msq_msg_t* aMsg, UINT16 aSettingIdx) {
	SC_LOG_DebugPrint(SC_TAG_RM, SC_LOG_START);

	pthread_msq_msg_t sendMsg = {};

	sendMsg.data[SC_MSG_MSG_ID] = e_SC_MSGID_REQ_RC_RTSINGLE;
	sendMsg.data[SC_MSG_REQ_RT_PLAN] = RM_SETTING_USESIDE;
	sendMsg.data[SC_MSG_REQ_RT_SRCID] = mSearchID;
	sendMsg.data[SC_MSG_REQ_RT_SETIDX] = aSettingIdx;

	// 送信
	sendMessage((pthread_msq_id_t*) SC_CORE_MSQID_RC, &sendMsg);

	SC_LOG_DebugPrint(SC_TAG_RM, SC_LOG_END);
}

/**
 * 再探索要求メッセージ送信
 * @param [I]受信メッセージ
 * @memo RM->RC (e_SC_MSGID_REQ_RC_RTSINGLE)
 *       [0] message id
 *       [1] 設定情報使用面
 *       [2] 検索ID
 */
static void sendReRouteRequestToRC(pthread_msq_msg_t* aMsg, UINT16 aSettingIdx) {
	SC_LOG_DebugPrint(SC_TAG_RM, SC_LOG_START);

	pthread_msq_msg_t sendMsg = {};

	sendMsg.data[SC_MSG_MSG_ID] = e_SC_MSGID_REQ_RC_REROUTE;
	sendMsg.data[SC_MSG_REQ_RT_PLAN] = RM_SETTING_USESIDE;
	sendMsg.data[SC_MSG_REQ_RT_SRCID] = mSearchID;
	sendMsg.data[SC_MSG_REQ_RT_SETIDX] = aSettingIdx;

	// 送信
	sendMessage((pthread_msq_id_t*) SC_CORE_MSQID_RC, &sendMsg);

	SC_LOG_DebugPrint(SC_TAG_RM, SC_LOG_END);
}

/**
 * 探索応答メッセージ送信
 * @param [I]受信メッセージ
 * @memo RM->FM (e_SC_MSGID_RES_FM_RTSINGLE)
 *       [0] message id
 *       [1] 探索結果コード
 *       [2] 経路ID
 */
static void sendSingleRouteResponseToFM(pthread_msq_msg_t* aMsg) {
	SC_LOG_DebugPrint(SC_TAG_RM, SC_LOG_START);

	pthread_msq_msg_t sendMsg = {};

	sendMsg.data[SC_MSG_MSG_ID] = e_SC_MSGID_RES_FM_RTSINGLE;
	sendMsg.data[SC_MSG_RES_RT_RESULT] = aMsg->data[SC_MSG_RES_RT_RESULT];
	sendMsg.data[SC_MSG_RES_RT_ROUTEID] = aMsg->data[SC_MSG_RES_RT_ROUTEID];

	// 送信
	sendMessage((pthread_msq_id_t*) SC_CORE_MSQID_FM, &sendMsg);

	SC_LOG_DebugPrint(SC_TAG_RM, SC_LOG_END);
}

/**
 * 再探索応答メッセージ送信
 * @param [I]受信メッセージ
 * @memo RM->FM (e_SC_MSGID_RES_FM_RTSINGLE)
 *       [0] message id
 *       [1] 探索結果コード
 *       [2] 経路ID
 */
static void sendReRouteResponseToFM(pthread_msq_msg_t* aMsg) {
	SC_LOG_DebugPrint(SC_TAG_RM, SC_LOG_START);

	pthread_msq_msg_t sendMsg = {};

	sendMsg.data[SC_MSG_MSG_ID] = e_SC_MSGID_RES_FM_REROUTE;
	sendMsg.data[SC_MSG_RES_RT_RESULT] = aMsg->data[SC_MSG_RES_RT_RESULT];
	sendMsg.data[SC_MSG_RES_RT_ROUTEID] = aMsg->data[SC_MSG_RES_RT_ROUTEID];

	// 送信
	sendMessage((pthread_msq_id_t*) SC_CORE_MSQID_FM, &sendMsg);

	SC_LOG_DebugPrint(SC_TAG_RM, SC_LOG_END);
}

/**
 * Entrance内エラー時FM応答
 * @param [I]受信メッセージ
 * @memo RM->FM (e_SC_MSGID_RES_FM_RTSINGLE)
 *       [0] message id
 *       [1] 探索結果コード
 */
static void sendFailResponseToFM(UINT32 aMsgId) {
	SC_LOG_DebugPrint(SC_TAG_RM, SC_LOG_START);

	pthread_msq_msg_t sendMsg = {};

	sendMsg.data[SC_MSG_MSG_ID] = aMsgId;
	sendMsg.data[SC_MSG_RES_RT_RESULT] = e_SC_RESULT_ROUTE_ERR;

	// 送信
	sendMessage((pthread_msq_id_t*) SC_CORE_MSQID_FM, &sendMsg);

	SC_LOG_DebugPrint(SC_TAG_RM, SC_LOG_END);
}

/**
 * メッセージ送信
 * @param 送信先Queue
 * @param 送信メッセージ構造体
 */
static E_SC_RESULT sendMessage(pthread_msq_id_t* aQueue, pthread_msq_msg_t* aMsg) {
	SC_LOG_DebugPrint(SC_TAG_RM, SC_LOG_START);

	UINT8 retry = 0;
	INT32 pthreadRet = PTHREAD_MSQ_OK;

	if (NULL == aQueue || NULL == aMsg) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param." HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	while (RP_MSG_SEND_RETRYCOUNT > retry) {
		pthreadRet = pthread_msq_msg_send(aQueue, aMsg, SC_CORE_MSQID_RM);
		if (PTHREAD_MSQ_OK == pthreadRet) {
			break;
		}
		// リトライカウントアップ
		retry++;
		SC_LOG_ErrorPrint(SC_TAG_RM, "[Msg] pthread_msq_msg_send failed. retry(%d)"HERE, retry);
	}
	if (RP_MSG_SEND_RETRYCOUNT <= retry) {
		SC_LOG_ErrorPrint(SC_TAG_RM, "[Msg] pthread_msq_msg_send failed. retry max." HERE);
		return (e_SC_RESULT_FAIL);
	}

	SC_LOG_DebugPrint(SC_TAG_RM, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * 検索IDカウント更新
 */
static void updateSearchID() {
	if (ALL_F32 == mSearchID) {
		mSearchID = 0;
	} else {
		mSearchID++;
	}
}

/**
 * 要求段階で処理エラーとなった場合のエラーコード設定処理
 */
static E_SC_RESULT setRoutePlanErrorTip() {

	SC_DH_SHARE_RPTIPINFO tip = {};

	tip.tipInfo.tipClass = EC_NOROUTE;
	tip.tipInfo.isRePlan = false;
	if (e_SC_RESULT_SUCCESS != SC_MNG_SetRPTip(&tip.tipInfo)) {
		SC_LOG_ErrorPrint(SC_TAG_RM, "SC_MNG_SetRPTip error, " HERE);
		return (e_SC_RESULT_FAIL);
	}
	return (e_SC_RESULT_SUCCESS);
}
