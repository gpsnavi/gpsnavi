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

/**
 * @brief 初期化処理を行う
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SDU_Initialize()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_SDU, SC_LOG_START);

	// 初期化
	ret = SC_SDU_Init();

	SC_LOG_DebugPrint(SC_TAG_SDU, SC_LOG_END);

	return (ret);
}

/**
 * @brief 終了化処理を行う
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SDU_Finalize()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_SDU, SC_LOG_START);

	// 終了化
	SC_SDU_Final();

	SC_LOG_DebugPrint(SC_TAG_SDU, SC_LOG_END);

	return (ret);
}

/**
 * @brief スレッドメイン関数
 * @param[in] param スレッドメイン関数引数構造体のポインタ
 * @return NULL
 */
void *SC_SDU_ThreadMain(void *param)
{
	//E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	rc = 0;
	pthread_msq_msg_t	msg = {};

	SC_LOG_DebugPrint(SC_TAG_SDU, SC_LOG_START);

	while (true != SC_Thread_GetIsFinish()) {
		// メッセージ初期化
		memset(&msg, 0, sizeof(pthread_msq_msg_t));

		// メッセージ受信
		rc = pthread_msq_msg_receive(SC_CORE_MSQID_SDU, &msg);
		if (PTHREAD_MSQ_OK != rc) {
			SC_LOG_ErrorPrint(SC_TAG_SDU, "pthread_msq_msg_receive err, " HERE);
			continue;
		}

		// 受信メッセージをログ出力
		SC_LOG_DebugPrint(SC_TAG_SDU,
				"recvMsg=0x%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x, " HERE,
				msg.data[0],  msg.data[1],  msg.data[2],  msg.data[3],  msg.data[4],
				msg.data[5],  msg.data[6],  msg.data[7],  msg.data[8],  msg.data[9]);

		// メッセージ種別IDチェック
		if (true != SC_MSG_CHECK_MSG_ID(&msg)) {
			SC_LOG_ErrorPrint(SC_TAG_SDU, "SC_MSG_CHECK_MSG_ID err, " HERE);
		}

		// メッセージディスパッチ
		SC_MSG_MsgDispatch(&msg, SC_CORE_MSQID_SDU);
	}

	SC_LOG_DebugPrint(SC_TAG_SDU, SC_LOG_END);

	return (NULL);
}
