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
 * SMCoreTRT.c
 *
 *  Created on: 2016/01/04
 *      Author: n.kanagawa
 */
#include "sms-core/SMCoreTR/SMCoreTRInternal.h"


// タイマストップフラグ
static Bool	mIsTimerStop = true;

// プロトタイプ
static void sendTimerTOMsg(void *receiver, INT32 timer);


/**
 * @brief タイマスタートメッセージ送信
 */
void SC_TRT_SendStratTimerMsg(void *sender, INT32 timer)
{
	pthread_msq_msg_t sendMsg = {};

	SC_LOG_DebugPrint(SC_TAG_TRT, SC_LOG_START);

	sendMsg.data[SC_MSG_MSG_ID] = e_SC_MSGID_REQ_TRT_START;
	sendMsg.data[SC_MSG_REQ_TR_TIMER] = timer;

	// 送信メッセージをログ出力
	SC_LOG_DebugPrint(SC_TAG_TRT,
			"sendMsg=0x%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x, " HERE,
			sendMsg.data[0],  sendMsg.data[1],  sendMsg.data[2],  sendMsg.data[3],  sendMsg.data[4],
			sendMsg.data[5],  sendMsg.data[6],  sendMsg.data[7],  sendMsg.data[8],  sendMsg.data[9]);

	// メッセージ送信
	if (PTHREAD_MSQ_OK != pthread_msq_msg_send(SC_CORE_MSQID_TRT, &sendMsg, sender)) {
		SC_LOG_ErrorPrint(SC_TAG_TRT, "pthread_msq_msg_send error, " HERE);
	}

	SC_LOG_DebugPrint(SC_TAG_TRT, SC_LOG_END);
}

/**
 * @brief T.Oメッセージ送信
 */
static void sendTimerTOMsg(void *receiver, INT32 timer)
{
	pthread_msq_msg_t sendMsg = {};

	SC_LOG_DebugPrint(SC_TAG_TRT, SC_LOG_START);

	// タイムアウト通知メッセージ生成
	sendMsg.data[SC_MSG_MSG_ID] = e_SC_MSGID_EVT_TIMEOUT;
	sendMsg.data[SC_MSG_REQ_TR_TIMER] = timer;

	// 送信メッセージをログ出力
	SC_LOG_DebugPrint(SC_TAG_TRT,
			"sendMsg=0x%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x, " HERE,
			sendMsg.data[0],  sendMsg.data[1],  sendMsg.data[2],  sendMsg.data[3],  sendMsg.data[4],
			sendMsg.data[5],  sendMsg.data[6],  sendMsg.data[7],  sendMsg.data[8],  sendMsg.data[9]);

	// タイムアウト通知メッセージ送信
	if (PTHREAD_MSQ_OK != pthread_msq_msg_send(receiver, &sendMsg, SC_CORE_MSQID_TRT)) {
		SC_LOG_ErrorPrint(SC_TAG_TRT, "pthread_msq_msg_send error, " HERE);
	}

	SC_LOG_DebugPrint(SC_TAG_TRT, SC_LOG_END);
}

/**
 * @brief タイマ開始
 */
void SC_TRT_StartTimer(const INT32 timer)
{
	INT32	cnt = 0;

	SC_LOG_DebugPrint(SC_TAG_TRT, SC_LOG_START);

	mIsTimerStop = false;

	for (cnt=0; cnt<timer; cnt++) {
		if (SC_Thread_GetIsFinish()) {
			return;
		}
		if (mIsTimerStop) {
			break;
		}
		//SC_LOG_DebugPrint(SC_TAG_TRT, "★Timer sleep %d", cnt+1);
		sleep (1);
	}

	if (!mIsTimerStop) {
		// T.Oメッセージ送信
		sendTimerTOMsg(SC_CORE_MSQID_TR, timer);
	}

	SC_LOG_DebugPrint(SC_TAG_TRT, SC_LOG_END);
}

/**
 * @brief タイマ停止
 */
void SC_TRT_StopTimer()
{
	SC_LOG_DebugPrint(SC_TAG_TRT, SC_LOG_START);

	mIsTimerStop = true;

	SC_LOG_DebugPrint(SC_TAG_TRT, SC_LOG_END);
}
