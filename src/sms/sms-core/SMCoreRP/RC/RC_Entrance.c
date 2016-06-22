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
 * RP_Entrance.c
 *
 *  Created on: 2015/11/06
 *      Author: masutani
 */

#include "sms-core/SMCoreRP/SMCoreRPInternal.h"

static E_SC_RESULT checkRouteSearchSetting(pthread_msq_msg_t* aMsg);
static E_SC_RESULT sendMessage(pthread_msq_id_t* aQueue, pthread_msq_msg_t* aMsg);

/**
 * 送信元別メッセージ処理
 * @param [I]受信メッセージ
 */
void SC_RC_MsgAnalyze(pthread_msq_msg_t* aMsg) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (SC_CORE_MSQID_RM!= pthread_msq_msg_issender(aMsg)) {
		// RM以外受信拒否
		SC_LOG_ErrorPrint(SC_TAG_RC, "[Msg] Unknown user message."HERE);
		return;
	}

	SC_LOG_DebugPrint(SC_TAG_RC,
			"recvMsg=0x%08x %08x %08x %08x %08x %08x, " HERE,
			aMsg->data[0],  aMsg->data[1],  aMsg->data[2],  aMsg->data[3],  aMsg->data[4],
			aMsg->data[5],  aMsg->data[6]);

	// 時間計測
	RP_ClearLapTime();

	// メッセージ内容別処理
	switch (aMsg->data[SC_MSG_MSG_ID]) {
	case e_SC_MSGID_REQ_RC_RTSINGLE:

		// 初期化
		RPC_InitState();
		// 探索条件インデックスを設定する
		RPC_SetCurrentSettingIdx(aMsg->data[SC_MSG_REQ_RT_SETIDX]);

		// 探索条件とメッセージチェック
		if (e_SC_RESULT_SUCCESS != checkRouteSearchSetting(aMsg)) {
			RPC_SetResult2ErrorCode(e_SC_RESULT_BADPARAM);
			RC_SendSingleRouteResponse(e_SC_RESULT_FAIL, aMsg->data[SC_MSG_REQ_RT_SRCID], aMsg->data[SC_MSG_REQ_RT_SETIDX]);
			break;
		}

		// 単経路探索
		RC_RtControlSingle(aMsg->data[SC_MSG_REQ_RT_SRCID], aMsg->data[SC_MSG_REQ_RT_SETIDX]);
		break;

	case e_SC_MSGID_REQ_RC_REROUTE:

		// 初期化
		RPC_InitState();
		// 再探索フラグON
		RPC_SetReplanFlag(true);
		// 探索条件インデックスを設定する
		RPC_SetCurrentSettingIdx(aMsg->data[SC_MSG_REQ_RT_SETIDX]);

		// 探索条件とメッセージチェック
		if (e_SC_RESULT_SUCCESS != checkRouteSearchSetting(aMsg)) {
			RPC_SetResult2ErrorCode(e_SC_RESULT_BADPARAM);
			RC_SendRePlanResponse(e_SC_RESULT_FAIL, aMsg->data[SC_MSG_REQ_RT_SRCID], aMsg->data[SC_MSG_REQ_RT_SETIDX]);
			break;
		}

		// 再探索
		RC_RtControlRePlan(aMsg->data[SC_MSG_REQ_RT_SRCID], aMsg->data[SC_MSG_REQ_RT_SETIDX]);
		break;

	default:
		// メッセージ内容不正
		SC_LOG_ErrorPrint(SC_TAG_RC, "[Msg] Unknown request message."HERE);
		return;
	}

	// 時間計測
	RP_OutputLapTime();

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
}

/**
 * @brief 設定情報とメッセージ情報の整合性チェック
 * @param メッセージ
 */
static E_SC_RESULT checkRouteSearchSetting(pthread_msq_msg_t* aMsg) {

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;

	if (NULL == aMsg) {
		return (e_SC_RESULT_BADPARAM);
	}
	do {
		// 設定情報取得
		SCRP_SEARCHSETTING *setting = RPM_GetRouteSearchSetting(aMsg->data[SC_MSG_REQ_RT_SETIDX]);
		if (NULL == setting) {
			result = e_SC_RESULT_FAIL;
			SC_LOG_ErrorPrint(SC_TAG_RC, "RP_GetRouteSearchSetting fail."HERE);
			break;
		}

		// 検索ID検査
		if (setting->routeSearchRequestId != aMsg->data[SC_MSG_REQ_RT_SRCID]) {
			result = e_SC_RESULT_FAIL;
			SC_LOG_ErrorPrint(SC_TAG_RC, "search id is unmatch."HERE);
			break;
		}
	} while (0);

	return (result);
}

/**
 * 単経路探索応答メッセージ送信
 * @memo RC->RM (e_SC_MSGID_RES_RM_RTSINGLE)
 *       [0] message id
 *       [1] 探索結果コード
 *       [2] 経路ID
 */
void RC_SendSingleRouteResponse(E_SC_RESULT aResult, UINT32 aRtId, UINT16 aSettingIdx) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);
	pthread_msq_msg_t sendMsg = {};

	SC_LOG_DebugPrint(SC_TAG_RC, "[Ctrl] ans %d", aResult);

	sendMsg.data[SC_MSG_MSG_ID] = e_SC_MSGID_RES_RC_RTSINGLE;
	sendMsg.data[SC_MSG_RES_RT_RESULT] = aResult;
	sendMsg.data[SC_MSG_RES_RT_ROUTEID] = aRtId;
	sendMsg.data[SC_MSG_RES_RT_SETIDX] = aSettingIdx;

	// 送信
	sendMessage((pthread_msq_id_t*) SC_CORE_MSQID_RM, &sendMsg);

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
}

/**
 * 再探索応答メッセージ送信
 * @memo RC->RM (e_SC_MSGID_RES_RM_RTSINGLE)
 *       [0] message id
 *       [1] 探索結果コード
 *       [2] 経路ID
 */
void RC_SendRePlanResponse(E_SC_RESULT aResult, UINT32 aRtId, UINT16 aSettingIdx) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);
	pthread_msq_msg_t sendMsg = {};

	SC_LOG_DebugPrint(SC_TAG_RC, "[Ctrl] ans %d", aResult);

	sendMsg.data[SC_MSG_MSG_ID] = e_SC_MSGID_RES_RC_REROUTE;
	sendMsg.data[SC_MSG_RES_RT_RESULT] = aResult;
	sendMsg.data[SC_MSG_RES_RT_ROUTEID] = aRtId;
	sendMsg.data[SC_MSG_RES_RT_SETIDX] = aSettingIdx;

	// 送信
	sendMessage((pthread_msq_id_t*) SC_CORE_MSQID_RM, &sendMsg);

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
}

/**
 * メッセージ送信
 * @param [I]送信メッセージ構造体
 * @param [I]送り先
 */
static E_SC_RESULT sendMessage(pthread_msq_id_t* aQueue, pthread_msq_msg_t* aMsg) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	UINT8 retry = 0;
	INT32 pthreadRet = PTHREAD_MSQ_OK;

	if(NULL == aQueue || NULL == aMsg){
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param." HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 送信
	while (RP_MSG_SEND_RETRYCOUNT > retry) {
		pthreadRet = pthread_msq_msg_send(aQueue, aMsg, SC_CORE_MSQID_RC);
		if (PTHREAD_MSQ_OK == pthreadRet) {
			break;
		}
		// リトライカウントアップ
		retry++;
		SC_LOG_ErrorPrint(SC_TAG_RC, "[Msg] pthread_msq_msg_send failed. retry(%d)"HERE, retry);
	}
	if (RP_MSG_SEND_RETRYCOUNT <= retry) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[Msg] pthread_msq_msg_send failed. retry max." HERE);
		return (e_SC_RESULT_FAIL);
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}
