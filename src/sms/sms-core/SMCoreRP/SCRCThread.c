/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#include "SMCoreRPInternal.h"

/**
 * @brief 初期化処理を行う
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_RC_Initialize()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);

	return (ret);
}

/**
 * @brief 終了化処理を行う
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_RC_Finalize()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);

	return (ret);
}

/**
 * @brief スレッドメイン関数
 * @param[in] param スレッドメイン関数引数構造体のポインタ
 * @return NULL
 */
void *SC_RC_ThreadMain(void *param)
{
	//E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	rc = 0;
	pthread_msq_msg_t	rmsg = {};

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	while (true != SC_Thread_GetIsFinish()) {
		// メッセージ初期化
		memset(&rmsg, 0, sizeof(pthread_msq_msg_t));

		// メッセージ受信
		rc = pthread_msq_msg_receive(SC_CORE_MSQID_RC, &rmsg);
		if (PTHREAD_MSQ_OK != rc) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "pthread_msq_msg_receive error(0x%08x), " HERE, rc);
			continue;
		}

		// 受信メッセージをログ出力
		SC_LOG_DebugPrint(SC_TAG_RC,
				"recvMsg=0x%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x, " HERE,
				rmsg.data[0],  rmsg.data[1],  rmsg.data[2],  rmsg.data[3],  rmsg.data[4],
				rmsg.data[5],  rmsg.data[6],  rmsg.data[7],  rmsg.data[8],  rmsg.data[9]);

		// メッセージ種別IDチェック
		if (true != SC_MSG_CHECK_MSG_ID(&rmsg)) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "recv msgid error\n");
		}

		// メッセージディスパッチ
		SC_MSG_MsgDispatch(&rmsg, SC_CORE_MSQID_RC);
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);

	return (NULL);
}
