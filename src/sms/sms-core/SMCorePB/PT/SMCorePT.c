/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#include "sms-core/SMCorePB/SMCorePBInternal.h"

static Bool	isTimerStop;

static void SC_PT_StartTimer(pthread_msq_msg_t *msg);


/**
 * @brief	受信メッセージ振り分け処理
 * @param	[I]受信メッセージ
 */
void SC_PT_MsgAnalyze(pthread_msq_msg_t *msg)
{
	//E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_PT, SC_LOG_START);

	switch (msg->data[SC_MSG_MSG_ID]) {
	case e_SC_MSGID_REQ_PT_START:		// タイマ開始
		// タイマ起動
		SC_PT_StartTimer(msg);
		break;
	case e_SC_MSGID_EVT_FINISH:			// 終了通知
		SC_LOG_DebugPrint(SC_TAG_PT, "recv msgid finish, " HERE);
		break;
	default:							// 不正なメッセージ種別ID
		SC_LOG_ErrorPrint(SC_TAG_PT, "recv msgid error, " HERE);
		break;
	}

	SC_LOG_DebugPrint(SC_TAG_PT, SC_LOG_END);
}

// タイマ開始
void SC_PT_StartTimer(pthread_msq_msg_t *msg)
{
	INT32	 cnt = 0;
	pthread_msq_msg_t sendMsg = {};
	INT32	timer = 0;

	SC_LOG_DebugPrint(SC_TAG_PT, SC_LOG_START);

	isTimerStop = false;
	timer = msg->data[SC_MSG_REQ_PT_TIMER];

	for (cnt = 0; cnt < timer; cnt++) {
		if (SC_Thread_GetIsFinish()) {
			// 終了要求
			return;
		}
		if (isTimerStop) {
			break;
		}
		// とりあえずsleepで待機する
		sleep(1);
	}

	if (!isTimerStop) {
		// タイムアウト通知メッセージ生成
		sendMsg.data[SC_MSG_MSG_ID] = e_SC_MSGID_EVT_TIMEOUT;

		// 送信メッセージをログ出力
		SC_LOG_DebugPrint(SC_TAG_PT,
				"sendMsg=0x%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x, " HERE,
				sendMsg.data[0],  sendMsg.data[1],  sendMsg.data[2],  sendMsg.data[3],  sendMsg.data[4],
				sendMsg.data[5],  sendMsg.data[6],  sendMsg.data[7],  sendMsg.data[8],  sendMsg.data[9]);

		// タイムアウト通知メッセージ送信
		if (PTHREAD_MSQ_OK != pthread_msq_msg_send(SC_CORE_MSQID_PM, &sendMsg, SC_CORE_MSQID_PT)) {
			SC_LOG_ErrorPrint(SC_TAG_PT, "pthread_msq_msg_send error, " HERE);
		}
	}

	SC_LOG_DebugPrint(SC_TAG_PT, SC_LOG_END);
}

void SC_PT_TimerStop()
{
	isTimerStop = true;
}
