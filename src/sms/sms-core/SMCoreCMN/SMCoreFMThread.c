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
 * SMCoreFMThread.c
 *
 *  Created on: 2015/11/05
 *      Author:
 */

#include "SMCoreCMNInternal.h"

static SC_SEMAPHORE	*fmSemId;
static SC_SEMAPHORE	*fmSDSemId;


/**
 * @brief 初期化処理を行う
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_FM_Initialize()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_FM, SC_LOG_START);

	// セマフォID取得
	fmSemId = (SC_SEMAPHORE*)SC_MNG_GetSemId();
	fmSDSemId = (SC_SEMAPHORE*)SC_MNG_GetSDSemId();

	SC_LOG_DebugPrint(SC_TAG_FM, SC_LOG_END);

	return (ret);
}

/**
 * @brief 終了化処理を行う
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_FM_Finalize()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_FM, SC_LOG_START);

	SC_LOG_DebugPrint(SC_TAG_FM, SC_LOG_END);

	return (ret);
}

/**
 * @brief スレッドメイン関数
 * @param[in] param スレッドメイン関数引数構造体のポインタ
 * @return NULL
 */
void *SC_FM_ThreadMain(void *param)
{
	//E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	rc = 0;
	pthread_msq_msg_t	rmsg = {};

	SC_LOG_DebugPrint(SC_TAG_FM, SC_LOG_START);

	while (true != SC_Thread_GetIsFinish()) {
		// メッセージ初期化
		memset(&rmsg, 0, sizeof(pthread_msq_msg_t));

		// メッセージ受信
		rc = pthread_msq_msg_receive(SC_CORE_MSQID_FM, &rmsg);
		if (PTHREAD_MSQ_OK != rc) {
			SC_LOG_ErrorPrint(SC_TAG_FM, "pthread_msq_msg_receive error(0x%08x), " HERE, rc);
			continue;
		}

		// 受信メッセージをログ出力
		SC_LOG_DebugPrint(SC_TAG_FM,
				"recvMsg=0x%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x, " HERE,
				rmsg.data[0],  rmsg.data[1],  rmsg.data[2],  rmsg.data[3],  rmsg.data[4],
				rmsg.data[5],  rmsg.data[6],  rmsg.data[7],  rmsg.data[8],  rmsg.data[9]);

		// メッセージ種別IDチェック
		if (true != SC_MSG_CHECK_MSG_ID(&rmsg)) {
			SC_LOG_ErrorPrint(SC_TAG_FM, "recv msgid error, " HERE);
			continue;
		}

		// メッセージディスパッチ
		SC_MSG_MsgDispatch(&rmsg, SC_CORE_MSQID_FM);
	}

	SC_LOG_DebugPrint(SC_TAG_FM, SC_LOG_END);

	return (NULL);
}

/**
 * @brief 探索応答
 * @param[in] msg   受信メッセージ構造体のポインタ
 * @return NULL
 */
void SC_FM_ResRoute(const pthread_msq_msg_t *rcvMsg) {

	SC_LOG_DebugPrint(SC_TAG_FM, SC_LOG_START);

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	INT32 routeId;

	// 結果取得
	result = rcvMsg->data[SC_MSG_RES_RT_RESULT];
	routeId = rcvMsg->data[SC_MSG_RES_RT_ROUTEID];

	switch (result) {
	case e_SC_RESULT_SUCCESS:
		if (e_SC_RESULT_SUCCESS != SC_MNG_SetExistRoute(true)) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MNG_SetExistRoute error, " HERE);
		}
		SC_LOG_DebugPrint(SC_TAG_CORE, "Route calc result success. code=0x%08x, " HERE, result);
		break;
	case e_SC_RESULT_ROUTE_CANCEL:
		// キャンセル
		SC_LOG_DebugPrint(SC_TAG_CORE, "Route calc result cancel. code=0x%08x, " HERE, result);
		break;
	default:
		// 探索失敗
		SC_LOG_DebugPrint(SC_TAG_CORE, "Route calc result error. code=0x%08x, " HERE, result);
		break;
	}

	// 処理結果設定
	SC_MNG_SetResult(result);

	// セマフォロック解除
	if (e_SC_RESULT_SUCCESS != SC_UnLockSemaphore(fmSemId)) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_UnLockSemaphore error, " HERE);
	}

	SC_LOG_DebugPrint(SC_TAG_FM, SC_LOG_END);
}

/**
 * @brief 再探索応答
 * @param[in] msg   受信メッセージ構造体のポインタ
 * @return NULL
 */
void SC_FM_ResReRoute(const pthread_msq_msg_t *rcvMsg) {

	SC_LOG_DebugPrint(SC_TAG_FM, SC_LOG_START);

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	INT32 routeId;

	// 結果取得
	result = rcvMsg->data[SC_MSG_RES_RT_RESULT];
	routeId = rcvMsg->data[SC_MSG_RES_RT_ROUTEID];
	//resCode = rcvMsg->data[SC_MSG_RES_RT_RESCODE];

	switch (result) {
	case e_SC_RESULT_SUCCESS:
		if (e_SC_RESULT_SUCCESS != SC_MNG_SetExistRoute(true)) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MNG_SetExistRoute error, " HERE);
		}
		SC_LOG_DebugPrint(SC_TAG_CORE, "Route calc result success. code=0x%08x, " HERE, result);
		break;
	case e_SC_RESULT_ROUTE_CANCEL:
		// キャンセル
		SC_LOG_DebugPrint(SC_TAG_CORE, "Route calc result cancel. code=0x%08x, " HERE, result);
		break;
	default:
		// 探索失敗
		SC_LOG_DebugPrint(SC_TAG_CORE, "Route calc result error. code=0x%08x, " HERE, result);
		break;
	}

	// 処理結果設定
	SC_MNG_SetResult(result);

	// セマフォロック解除
	if (e_SC_RESULT_SUCCESS != SC_UnLockSemaphore(fmSemId)) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_UnLockSemaphore error. " HERE);
	}

	SC_LOG_DebugPrint(SC_TAG_FM, SC_LOG_END);
}

/**
 * @brief 経路誘導応答
 * @param[in] msg   受信メッセージ構造体のポインタ
 * @return NULL
 */
void SC_FM_ResGuide(const pthread_msq_msg_t *rcvMsg)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_FM, SC_LOG_START);

	if (0 != rcvMsg->data[SC_MSG_RES_RG_RESULT]) {
		ret = e_SC_RESULT_FAIL;
	}

	// 処理結果設定
	SC_MNG_SetResult(ret);

	// セマフォロック解除
	if (e_SC_RESULT_SUCCESS != SC_UnLockSemaphore(fmSemId)) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_UnLockSemaphore error, " HERE);
		ret = e_SC_RESULT_FAIL;
	}

	SC_LOG_DebugPrint(SC_TAG_FM, SC_LOG_END);
}

/**
 * @brief 運転特性診断開始応答
 * @param[in] msg   受信メッセージ構造体のポインタ
 */
void SC_FM_ResStartDrive(const pthread_msq_msg_t *rcvMsg)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_FM, SC_LOG_START);

	if (NULL != rcvMsg) {
		ret = rcvMsg->data[SC_MSG_RES_SD_RESULT];
		if ((NULL != (Char*)rcvMsg->data[SC_MSG_RES_SD_TRIP_ID]) &&
			(EOS != *((Char*)rcvMsg->data[SC_MSG_RES_SD_TRIP_ID]))) {
			if (e_SC_RESULT_SUCCESS == ret) {
				// トリップID設定
				SC_MNG_SetTripId((Char*)rcvMsg->data[SC_MSG_RES_SD_TRIP_ID]);
			}
			// メモリ解放
			SC_MEM_Free((Char*)rcvMsg->data[SC_MSG_RES_SD_TRIP_ID], e_MEM_TYPE_SD);
		}
	} else {
		ret = e_SC_RESULT_FAIL;
	}

	// 処理結果設定
	SC_MNG_SetSDResult(ret);

	// セマフォロック解除
	if (e_SC_RESULT_SUCCESS != SC_UnLockSemaphore(fmSDSemId)) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_UnLockSemaphore error, " HERE);
		ret = e_SC_RESULT_FAIL;
	}

	SC_LOG_DebugPrint(SC_TAG_FM, SC_LOG_END);
}

/**
 * @brief 運転特性診断停止応答
 * @param[in] msg   受信メッセージ構造体のポインタ
 */
void SC_FM_ResStopDrive(const pthread_msq_msg_t *rcvMsg)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_FM, SC_LOG_START);

	if (NULL != rcvMsg) {
		ret = rcvMsg->data[SC_MSG_RES_SD_RESULT];
		if ((NULL != (Char*)rcvMsg->data[SC_MSG_RES_SD_TRIP_ID]) &&
			(EOS != *((Char*)rcvMsg->data[SC_MSG_RES_SD_TRIP_ID]))) {
			if (e_SC_RESULT_SUCCESS == ret) {
				// トリップID設定
				SC_MNG_SetTripId((Char*)rcvMsg->data[SC_MSG_RES_SD_TRIP_ID]);
			}
			// メモリ解放
			SC_MEM_Free((Char*)rcvMsg->data[SC_MSG_RES_SD_TRIP_ID], e_MEM_TYPE_SD);
		}
	} else {
		ret = e_SC_RESULT_FAIL;
	}

	// 処理結果設定
	SC_MNG_SetSDResult(ret);

	// セマフォロック解除
	if (e_SC_RESULT_SUCCESS != SC_UnLockSemaphore(fmSDSemId)) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_UnLockSemaphore error, " HERE);
		ret = e_SC_RESULT_FAIL;
	}

	SC_LOG_DebugPrint(SC_TAG_FM, SC_LOG_END);
}
