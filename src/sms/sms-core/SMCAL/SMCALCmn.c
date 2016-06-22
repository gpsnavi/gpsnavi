/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#include "SMCALInternal.h"

/**
 * @brief Mutexを生成する
 * @param[in] mutex ミューテックスオブジェクトのポインタ
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_CreateMutex(SC_CAL_MUTEX *mutex)
{
	// パラメータチェック
	if (NULL == mutex) {
		//SC_CAL_LOG_ErrorPrint(SC_CAL_TAG, "param error[mutex], " HERE);
		return (e_SC_CAL_RESULT_BADPARAM);
	}
	// Mutex初期化
	pthread_mutex_init(mutex, NULL);

	return (e_SC_CAL_RESULT_SUCCESS);
}

/**
 * @brief Mutexを破棄する
 * @param[in] mutex ミューテックスオブジェクトのポインタ
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_DestroyMutex(SC_CAL_MUTEX *mutex)
{
	INT32 ret = 0;

	// パラメータチェック
	if (NULL == mutex) {
		//SC_CAL_LOG_ErrorPrint(SC_CAL_TAG, "param error[mutex], " HERE);
		return (e_SC_CAL_RESULT_BADPARAM);
	}

	// Mutex破棄
	ret = pthread_mutex_destroy(mutex);
	if (0 != ret) {
		//SC_CAL_LOG_ErrorPrint(SC_CAL_TAG, "pthread_mutex_destroy error(0x%08x), " HERE, errno);
		return (e_SC_CAL_RESULT_FAIL);
	}

	return (e_SC_CAL_RESULT_SUCCESS);
}

/**
 * @brief Mutexをロックする
 * @param[in] mutex ミューテックスオブジェクトのポインタ
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_LockMutex(SC_CAL_MUTEX *mutex)
{
	INT32 ret = 0;

	// パラメータチェック
	if (NULL == mutex) {
		//SC_CAL_LOG_ErrorPrint(SC_CAL_TAG, "param error[mutex], " HERE);
		return (e_SC_CAL_RESULT_BADPARAM);
	}

	// Mutexロック
	ret = pthread_mutex_lock(mutex);
	if (0 != ret) {
		//SC_CAL_LOG_ErrorPrint(SC_CAL_TAG, "pthread_mutex_lock error(0x%08x), " HERE, errno);
		return (e_SC_CAL_RESULT_FAIL);
	}

	return (e_SC_CAL_RESULT_SUCCESS);
}

/**
 * @brief Mutexをアンロックする
 * @param[in] mutex ミューテックスオブジェクトのポインタ
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_UnLockMutex(SC_CAL_MUTEX *mutex)
{
	INT32 ret = 0;

	// パラメータチェック
	if (NULL == mutex) {
		//SC_CAL_LOG_ErrorPrint(SC_CAL_TAG, "param error[mutex], " HERE);
		return (e_SC_CAL_RESULT_BADPARAM);
	}

	// Mutexアンロック
	ret = pthread_mutex_unlock(mutex);
	if (0 != ret) {
		//SC_CAL_LOG_ErrorPrint(SC_CAL_TAG, "pthread_mutex_unlock error(0x%08x), " HERE, errno);
		return (e_SC_CAL_RESULT_FAIL);
	}

	return (e_SC_CAL_RESULT_SUCCESS);
}

/**
 * @brief セマフォを生成する
 * @param[in] sem セマフォオブジェクトのポインタ
 * @param[in] val セマフォの初期値
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_CreateSemaphore(SC_CAL_SEMAPHORE *sem, UINT32 val)
{
	INT32 ret = 0;

	// パラメータチェック
	if (NULL == sem) {
		//SC_CAL_LOG_ErrorPrint(SC_CAL_TAG, "param error[mutex], " HERE);
		return (e_SC_CAL_RESULT_BADPARAM);
	}
	// セマフォ初期化
	ret = sem_init(sem, 0, val);
	if (0 != ret) {
		//SC_CAL_LOG_ErrorPrint(SC_CAL_TAG, "sem_init error(0x%08x), " HERE, errno);
		return (e_SC_CAL_RESULT_FAIL);
	}

	return (e_SC_CAL_RESULT_SUCCESS);
}

/**
 * @brief セマフォを破棄する
 * @param[in] sem セマフォオブジェクトのポインタ
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_DestroySemaphore(SC_CAL_SEMAPHORE *sem)
{
	INT32 ret = 0;

	// パラメータチェック
	if (NULL == sem) {
		//SC_CAL_LOG_ErrorPrint(SC_CAL_TAG, "param error[mutex], " HERE);
		return (e_SC_CAL_RESULT_BADPARAM);
	}

	// セマフォ破棄
	ret = sem_destroy(sem);
	if (0 != ret) {
		//SC_CAL_LOG_ErrorPrint(SC_CAL_TAG, "sem_destroy error(0x%08x), " HERE, errno);
		return (e_SC_CAL_RESULT_FAIL);
	}

	return (e_SC_CAL_RESULT_SUCCESS);
}

/**
 * @brief セマフォをロックする
 * @param[in] sem セマフォオブジェクトのポインタ
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_LockSemaphore(SC_CAL_SEMAPHORE *sem)
{
	INT32 ret = 0;

	// パラメータチェック
	if (NULL == sem) {
		//SC_CAL_LOG_ErrorPrint(SC_CAL_TAG, "param error[mutex], " HERE);
		return (e_SC_CAL_RESULT_BADPARAM);
	}

	// セマフォロック
	ret = sem_wait(sem);
	if (0 != ret) {
		//SC_CAL_LOG_ErrorPrint(SC_CAL_TAG, "sem_wait error(0x%08x), " HERE, errno);
		return (e_SC_CAL_RESULT_FAIL);
	}

	return (e_SC_CAL_RESULT_SUCCESS);
}

/**
 * @brief セマフォ(タイムアウトあり)をロックする
 * @param[in] sem セマフォオブジェクトのポインタ
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_LockTimeSemaphore(SC_CAL_SEMAPHORE *sem, UINT32 sec)
{
	INT32 ret = 0;
    struct timespec	timeout;

	// パラメータチェック
	if (NULL == sem) {
		//SC_CAL_LOG_ErrorPrint(SC_CAL_TAG, "param error[mutex], " HERE);
		return (e_SC_CAL_RESULT_BADPARAM);
	}

	memset(&timeout, 0x00, sizeof(timeout));
#ifdef __SMS_APPLE__
	INT32 i = 0;
	do {
		ret = sem_trywait(sem);
		if (0 == ret) {
			// ロック成功
			break;
		}
		if (i >= sec) {
			// タイムアウトエラー
			break;
		}
		i++;
		sleep(1);
	} while (0 != ret);
#else
	clock_gettime(CLOCK_REALTIME, &timeout);
	timeout.tv_sec += sec;

    // セマフォロック
	ret = sem_timedwait(sem, &timeout);
#endif /* __SMS_APPLE__ */
	if (0 != ret) {
		//SC_CAL_LOG_ErrorPrint(SC_CAL_TAG, "sem_timedwait error(0x%08x), " HERE, errno);
		return (e_SC_CAL_RESULT_FAIL);
	}

	return (e_SC_CAL_RESULT_SUCCESS);
}

/**
 * @brief セマフォをアンロックする
 * @param[in] sem セマフォオブジェクトのポインタ
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_UnLockSemaphore(SC_CAL_SEMAPHORE *sem)
{
	INT32 ret = 0;

	// パラメータチェック
	if (NULL == sem) {
		//SC_CAL_LOG_ErrorPrint(SC_CAL_TAG, "param error[sem], " HERE);
		return (e_SC_CAL_RESULT_BADPARAM);
	}

	// セマフォアンロック
	ret = sem_post(sem);
	if (0 != ret) {
		//SC_CAL_LOG_ErrorPrint(SC_CAL_TAG, "sem_post error(0x%08x), " HERE, errno);
		return (e_SC_CAL_RESULT_FAIL);
	}

	return (e_SC_CAL_RESULT_SUCCESS);
}

/**
 * @brief ディレクトリ作成
 * @param[in] dirPath 作成するディレクトリのフルパス（最大長はNULL終端文字含めてSC_MAX_PATH）
 * @return 処理結果(E_SC_CAL_RESULT)
 * @warning パスにフォルダのフルパスを指定する場合は、末尾に/を付加すること。
 *          パスは、ファイルのフルパスでも可。
 */
E_SC_CAL_RESULT SC_CAL_MakeDir(const Char *dirPath)
{
	Char			path[SC_CAL_MAX_PATH] = {};
	Char			*pPath = NULL;
	Char			*chr = NULL;
	INT32			cnt = 0;
	//UINT32			errCode = 0;
	struct stat		st = {};

	if (NULL == dirPath) {
		//SC_CAL_LOG_ErrorPrint(SC_CAL_TAG, (Char*)"param error[dirPath], " HERE);
		return (e_SC_CAL_RESULT_BADPARAM);
	}

	// コピー先フォルダ作成
	strncpy((char*)path, (char*)dirPath, (sizeof(path) - 1));
	chr = strrchr((char*)path, '/');
	if (NULL != chr) {
		if ((INT32)strlen(path) > (INT32)(chr - path)) {
			*(chr + 1) = EOS;
		}
	}

	pPath = &path[0];
	while (EOS != *pPath) {
		// 先頭から'/'を検索
		//LOG_PRINT_ERROR(SC_TAG_NA, pPath);
		chr = (Char*)strchr((char*)pPath, '/');
		if (NULL == chr) {
			// 見つからなかったので終了
			break;
		}
		if (0 < cnt) {
			*chr = EOS;
			if (0 != stat((char*)path, &st)) {
				// ディレクトリ作成
				if (0 != mkdir((char*)path, (S_IRWXU | S_IRWXG | S_IRWXO))) {
					//SC_CAL_LOG_ErrorPrint(SC_CAL_TAG, (Char*)"mkdir error(0x%08x), " HERE, errno);
					//SC_CAL_LOG_ErrorPrint(SC_CAL_TAG, (Char*)"path=%s, " HERE, path);
					return (e_SC_CAL_RESULT_FILE_ACCESSERR);
				}
			}
			*chr = '/';
		}
		pPath = chr + 1;
		cnt++;
	}

	return (e_SC_CAL_RESULT_SUCCESS);
}
