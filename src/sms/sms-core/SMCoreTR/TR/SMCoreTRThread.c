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
 * SMCoreTRThread.c
 *
 *  Created on: 2016/01/04
 *      Author: n.kanagawa
 */
#include "sms-core/SMCoreTR/SMCoreTRInternal.h"


/**
 * @brief 初期化処理を行う
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_TR_Initialize()
{
	E_SC_RESULT			ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_START);

	// キャッシュテーブル初期化
	ret = SC_TR_CacheTblInitialize();

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_END);

	return (ret);
}

/**
 * @brief 終了化処理を行う
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_TR_Finalize()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_START);

	// キャッシュテーブル終了
	ret = SC_TR_CacheTblFinalize();

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_END);

	return (ret);
}

/**
 * @brief スレッドメイン関数
 * @param[in] param スレッドメイン関数引数構造体のポインタ
 * @return NULL
 */
void *SC_TR_ThreadMain(void *param)
{
	//E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	rc = 0;
	pthread_msq_msg_t	rmsg = {};

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_START);

	while (true != SC_Thread_GetIsFinish()) {
		// メッセージ初期化
		memset(&rmsg, 0, sizeof(pthread_msq_msg_t));

		// メッセージ受信
		rc = pthread_msq_msg_receive(SC_CORE_MSQID_TR, &rmsg);
		if (PTHREAD_MSQ_OK != rc) {
			SC_LOG_ErrorPrint(SC_TAG_TR, "[MNG] MESSAGE REC ERROR " HERE);
			continue;
		}

		// メッセージ種別IDチェック
		if (true != SC_MSG_CHECK_MSG_ID(&rmsg)) {
			SC_LOG_ErrorPrint(SC_TAG_TR, "[MNG] MESSAGE ERROR " HERE);
		}

		// メッセージディスパッチ
		SC_MSG_MsgDispatch(&rmsg, SC_CORE_MSQID_TR);
	}

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_END);

	return (NULL);
}
