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
 * SMCoreTRTDispatch.c
 *
 *  Created on: 2016/01/04
 *      Author: n.kanagawa
 */
#include "sms-core/SMCoreTR/SMCoreTRInternal.h"


/**
 * @brief	受信メッセージ振り分け処理
 * @param	[I]受信メッセージ
 */
void SC_TRT_MsgAnalyze(const pthread_msq_msg_t *msg)
{
	//E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_TRT, SC_LOG_START);

	switch (msg->data[SC_MSG_MSG_ID]) {
	case e_SC_MSGID_REQ_TRT_START:		// タイマ開始
		SC_LOG_InfoPrint(SC_TAG_TRT, "[Msg] e_SC_MSGID_REQ_TRT_START:");
		// タイマスタート
		SC_TRT_StartTimer(msg->data[SC_MSG_REQ_TR_TIMER]);
		break;

	case e_SC_MSGID_EVT_FINISH:			// 終了通知
		SC_LOG_InfoPrint(SC_TAG_TRT, "recv msgid finish, " HERE);
		// タイマストップ
		SC_TRT_StopTimer();
		break;

	default:							// 不正なメッセージ種別ID
		SC_LOG_InfoPrint(SC_TAG_TRT, "recv msgid error, " HERE);
		break;
	}

	SC_LOG_DebugPrint(SC_TAG_TRT, SC_LOG_END);
}
