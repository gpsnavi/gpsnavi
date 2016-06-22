/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#include "sms-core/SMCoreSD/SMCoreSDInternal.h"

#define SC_SCDT_TIMEOUT_RES_WAIT_MAXNUM			3

static pthread_mutex_t	*mutex;
static pthread_mutex_t	mutexObj;
static Bool		isTimerStop;
static INT32	timeOutResWaitNum;

/**
 * @brief 初期化
 */
void SC_SDT_Init()
{
	// 初期化
	mutex = &mutexObj;
	memset(mutex, 0, sizeof(SC_MUTEX));
	isTimerStop = false;
	timeOutResWaitNum = 0;

	// Mutex生成
	pthread_mutex_init(mutex, NULL);
}

/**
 * @brief 終了化
 */
void SC_SDT_Final()
{
	// 初期化
	isTimerStop = false;
	timeOutResWaitNum = 0;

	if (NULL != mutex) {
		// Mutex破棄
		pthread_mutex_destroy(mutex);
		mutex = NULL;
	}
}

/**
 * @brief タイマ開始
 * @param [I]msg    受信メッセージ
 */
void SC_SDT_StartTimer(const pthread_msq_msg_t *msg)
{
	INT32	 cnt = 0;
	pthread_msq_msg_t sendMsg = {};
	INT32	timer = 0;
	Bool	isLocked = false;

	SC_LOG_DebugPrint(SC_TAG_SDT, SC_LOG_START);

	isTimerStop = false;
	timer = msg->data[SC_MSG_REQ_SD_TIMER];

	while (1) {
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
		if (isTimerStop) {
			break;
		}

		// Mutexロック
		if (0 == pthread_mutex_lock(mutex)) {
			isLocked = true;
		}

		if (SC_SCDT_TIMEOUT_RES_WAIT_MAXNUM > timeOutResWaitNum) {
			// タイムアウト通知メッセージ生成
			sendMsg.data[SC_MSG_MSG_ID] = e_SC_MSGID_EVT_TIMEOUT;

			// 送信メッセージをログ出力
			SC_LOG_DebugPrint(SC_TAG_SDT,
					"sendMsg=0x%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x, " HERE,
					sendMsg.data[0],  sendMsg.data[1],  sendMsg.data[2],  sendMsg.data[3],  sendMsg.data[4],
					sendMsg.data[5],  sendMsg.data[6],  sendMsg.data[7],  sendMsg.data[8],  sendMsg.data[9]);

			// タイムアウト通知メッセージ送信
			if (PTHREAD_MSQ_OK != pthread_msq_msg_send(SC_CORE_MSQID_SDM, &sendMsg, SC_CORE_MSQID_SDT)) {
				SC_LOG_ErrorPrint(SC_TAG_SDT, "pthread_msq_msg_send error, " HERE);
			}
			timeOutResWaitNum++;
		}

		if (isLocked) {
			// Mutexアンロック
			pthread_mutex_unlock(mutex);
		}
	}

	SC_LOG_DebugPrint(SC_TAG_SDT, SC_LOG_END);
}

/**
 * @brief タイマ停止
 */
void SC_SDT_StopTimer()
{
	isTimerStop = true;
}

/**
 * @brief タイムアウト通知応答
 */
void SC_SDT_TimeoutRes()
{
	SC_LOG_DebugPrint(SC_TAG_SDT, SC_LOG_START);
	Bool	isLocked = false;

	// Mutexロック
	if (0 == pthread_mutex_lock(mutex)) {
		isLocked = true;
	}

	if (0 < timeOutResWaitNum) {
		timeOutResWaitNum--;
	}

	if (isLocked) {
		// Mutexアンロック
		pthread_mutex_unlock(mutex);
	}

	SC_LOG_DebugPrint(SC_TAG_SDT, SC_LOG_END);
}
