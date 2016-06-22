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
E_SC_RESULT SC_SDM_Initialize()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	const Char	*mapFilePath = NULL;

	SC_LOG_DebugPrint(SC_TAG_SDM, SC_LOG_START);

	// 地図フォルダのパス取得
	mapFilePath = SC_MNG_GetMapDirPath();
	if (((NULL != mapFilePath) && (EOS != *mapFilePath))) {
		// 初期化
		ret = SC_SDM_Init(mapFilePath);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_SDM, "SC_SDM_Init error, " HERE);
		}
	} else {
		SC_LOG_ErrorPrint(SC_TAG_SDM, "mapFilePath error, " HERE);
		ret = e_SC_RESULT_FAIL;
	}

	SC_LOG_DebugPrint(SC_TAG_SDM, SC_LOG_END);

	return (ret);
}

/**
 * @brief 終了化処理を行う
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SDM_Finalize()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_SDM, SC_LOG_START);

	// 終了化
	SC_SDM_Final();

	SC_LOG_DebugPrint(SC_TAG_SDM, SC_LOG_END);

	return (ret);
}

/**
 * @brief スレッドメイン関数
 * @param[in] param スレッドメイン関数引数構造体のポインタ
 * @return NULL
 */
void *SC_SDM_ThreadMain(void *param)
{
	//E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	rc = 0;
	pthread_msq_msg_t	msg = {};

	SC_LOG_DebugPrint(SC_TAG_SDM, SC_LOG_START);

	while (true != SC_Thread_GetIsFinish()) {
		// メッセージ初期化
		memset(&msg, 0, sizeof(pthread_msq_msg_t));

		// メッセージ受信
		rc = pthread_msq_msg_receive(SC_CORE_MSQID_SDM, &msg);
		if (PTHREAD_MSQ_OK != rc) {
			SC_LOG_ErrorPrint(SC_TAG_SDM, "pthread_msq_msg_receive err, " HERE);
			continue;
		}

		// 受信メッセージをログ出力
		SC_LOG_DebugPrint(SC_TAG_SDM,
				"recvMsg=0x%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x, " HERE,
				msg.data[0],  msg.data[1],  msg.data[2],  msg.data[3],  msg.data[4],
				msg.data[5],  msg.data[6],  msg.data[7],  msg.data[8],  msg.data[9]);

		// メッセージ種別IDチェック
		if (true != SC_MSG_CHECK_MSG_ID(&msg)) {
			SC_LOG_ErrorPrint(SC_TAG_SDM, "SC_MSG_CHECK_MSG_ID err, " HERE);
		}

		// メッセージディスパッチ
		SC_MSG_MsgDispatch(&msg, SC_CORE_MSQID_SDM);
	}

	SC_LOG_DebugPrint(SC_TAG_SDM, SC_LOG_END);

	return (NULL);
}
