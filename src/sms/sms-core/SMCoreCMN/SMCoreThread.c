/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#include "SMCoreCMNInternal.h"

//static Bool isFinish;									// 終了するか否か
Bool isFinish;									// 終了するか否か

/**
 * @brief スレッドの初期化を行う
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_Thread_Initialize()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	isFinish = false;

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief スレッドの終了化を行う
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_Thread_Finalize()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	isFinish = false;

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief スレッドの生成を行う
 * @param[out] threadId スレッドIDのポインタ
 * @param[in]  func     スレッドメイン関数のポインタ
 * @param[in]  param    スレッドメイン関数の引数のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_Thread_Create(SC_THREAD_ID *threadId, SC_THREAD_MAIN_FUNC func, void *param)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32		pret = 0;

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == threadId) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[threadId], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == func) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[func], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}


		// スレッド生成
		pret = pthread_create(threadId, NULL, func, param);
		if (0 != pret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "pthread_create error(0x%08x), " HERE, errno);
			ret = e_SC_RESULT_FAIL;
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief スレッドが終了するまで待機する
 * @param[in] threadId スレッドID
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_Thread_Join(SC_THREAD_ID threadId)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32		pret = 0;

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// スレッド終了まで待機
		pret = pthread_join(threadId, NULL);
		if (0 != pret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "pthread_join error(0x%08x), " HERE, errno);
			ret = e_SC_RESULT_FAIL;
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}
