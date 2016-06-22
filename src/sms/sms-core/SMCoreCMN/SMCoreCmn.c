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

#define LIMIT 1.0e-8    /* 許容誤差 */

static Char BasicKeyword1[256];
static Char BasicKeyword2[256];

/**
 * @brief Mutexを生成する
 * @param[in] mutex ミューテックスオブジェクトのポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_CreateMutex(SC_MUTEX *mutex)
{
	// パラメータチェック
	if (NULL == mutex) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[mutex], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	// Mutex初期化
	pthread_mutex_init(mutex, NULL);

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief Mutexを破棄する
 * @param[in] mutex ミューテックスオブジェクトのポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_DestroyMutex(SC_MUTEX *mutex)
{
	INT32 ret = 0;

	// パラメータチェック
	if (NULL == mutex) {
		//SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[mutex], " HERE);
#ifdef	ANDROID
		__android_log_print(SC_LOG_LV_ERROR, SC_TAG_CORE, (char*)"param error[mutex], " HERE);
#endif	// ANDROID
		return (e_SC_RESULT_BADPARAM);
	}

	// Mutex破棄
	ret = pthread_mutex_destroy(mutex);
	if (0 != ret) {
		//SC_LOG_ErrorPrint(SC_TAG_CORE, "pthread_mutex_destroy error(0x%08x), " HERE, errno);
#ifdef	ANDROID
		__android_log_print(SC_LOG_LV_ERROR, SC_TAG_CORE, (char*)"pthread_mutex_destroy error(0x%08x), " HERE, errno);
#endif	// ANDROID
		return (e_SC_RESULT_FAIL);
	}

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief Mutexをロックする
 * @param[in] mutex ミューテックスオブジェクトのポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_LockMutex(SC_MUTEX *mutex)
{
	INT32 ret = 0;

	// パラメータチェック
	if (NULL == mutex) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[mutex], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// Mutexロック
	ret = pthread_mutex_lock(mutex);
	if (0 != ret) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "pthread_mutex_lock error(0x%08x), " HERE, errno);
		return (e_SC_RESULT_FAIL);
	}

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief Mutexをアンロックする
 * @param[in] mutex ミューテックスオブジェクトのポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_UnLockMutex(SC_MUTEX *mutex)
{
	INT32 ret = 0;

	// パラメータチェック
	if (NULL == mutex) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[mutex], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// Mutexアンロック
	ret = pthread_mutex_unlock(mutex);
	if (0 != ret) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "pthread_mutex_unlock error(0x%08x), " HERE, errno);
		return (e_SC_RESULT_FAIL);
	}

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief セマフォを生成する
 * @param[in] sem セマフォオブジェクトのポインタ
 * @param[in] val セマフォの初期値
 * @return 処理結果(E_SC_RESULT)
 */
#ifdef __SMS_APPLE__
E_SC_RESULT SC_CreateSemaphore(SC_SEMAPHORE **sem, Char *sem_name, UINT32 val)
{
	// パラメータチェック
	if (NULL == sem || NULL == sem_name) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[mutex], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	// セマフォ初期化
	sem_t* new_sem = sem_open((char*) sem_name, O_CREAT, S_IRWXU, val);
	if (SEM_FAILED == new_sem) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "sem_open error, " HERE);
		return (e_SC_RESULT_FAIL);
	}
	*sem = new_sem;

	return (e_SC_RESULT_SUCCESS);
}
#else
E_SC_RESULT SC_CreateSemaphore(SC_SEMAPHORE *sem, UINT32 val)
{
	// パラメータチェック
	if (NULL == sem) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[mutex], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	// セマフォ初期化
	if (0 != sem_init(sem, 0, val)) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "sem_init error(0x%08x), " HERE, errno);
		return (e_SC_RESULT_FAIL);
	}

	return (e_SC_RESULT_SUCCESS);
}
#endif /* __SMS_APPLE__ */

/**
 * @brief セマフォを破棄する
 * @param[in] sem セマフォオブジェクトのポインタ
 * @return 処理結果(E_SC_RESULT)
 */
#ifdef __SMS_APPLE__
E_SC_RESULT SC_DestroySemaphore(SC_SEMAPHORE *sem, Char *sem_name)
{
	INT32 ret = 0;

	// パラメータチェック
	if (NULL == sem || NULL == sem_name) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[mutex], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	if (0 != sem_close(sem)) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "sem_close error, " HERE);
		return (e_SC_RESULT_FAIL);
	}
	// セマフォ破棄
	if (0 != sem_unlink((char*) sem_name)) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "sem_unlink error, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	return (e_SC_RESULT_SUCCESS);
}
#else
E_SC_RESULT SC_DestroySemaphore(SC_SEMAPHORE *sem)
{
	//INT32 ret = 0;

	// パラメータチェック
	if (NULL == sem) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[mutex], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// セマフォ破棄
	if (0 != sem_destroy(sem)) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "sem_destroy error(0x%08x), " HERE, errno);
		return (e_SC_RESULT_FAIL);
	}

	return (e_SC_RESULT_SUCCESS);
}
#endif /* __SMS_APPLE__ */

/**
 * @brief セマフォをロックする
 * @param[in] sem セマフォオブジェクトのポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_LockSemaphore(SC_SEMAPHORE *sem)
{
	INT32 ret = 0;

	// パラメータチェック
	if (NULL == sem) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[mutex], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// セマフォロック
	ret = sem_wait(sem);
	if (0 != ret) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "sem_wait error(0x%08x), " HERE, errno);
		return (e_SC_RESULT_FAIL);
	}

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief セマフォ(タイムアウトあり)をロックする
 * @param[in] sem セマフォオブジェクトのポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_LockTimeSemaphore(SC_SEMAPHORE *sem, UINT32 sec)
{
	INT32 ret = 0;
    struct timespec	timeout;
#ifdef __SMS_APPLE__
	INT32 i = 0;
#endif /* __SMS_APPLE__ */

	// パラメータチェック
	if (NULL == sem) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[mutex], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	memset(&timeout, 0x00, sizeof(timeout));
#ifdef __SMS_APPLE__
#else
	clock_gettime(CLOCK_REALTIME, &timeout);
	timeout.tv_sec += sec;
#endif /* __SMS_APPLE__ */

    // セマフォロック
#ifdef __SMS_APPLE__
	i = 0;
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
	ret = sem_timedwait(sem, &timeout);
#endif /* __SMS_APPLE__ */
	if (0 != ret) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "sem_timedwait error(0x%08x), " HERE, errno);
		return (e_SC_RESULT_FAIL);
	}

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief セマフォをアンロックする
 * @param[in] sem セマフォオブジェクトのポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_UnLockSemaphore(SC_SEMAPHORE *sem)
{
	INT32 ret = 0;

	// パラメータチェック
	if (NULL == sem) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[sem], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// セマフォアンロック
	ret = sem_post(sem);
	if (0 != ret) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "sem_post error(0x%08x), " HERE, errno);
		return (e_SC_RESULT_FAIL);
	}

	return (e_SC_RESULT_SUCCESS);
}


/**
 * @brief 文字列中の先頭末尾の空白の削除
 * @param [in/out] str 対象文字列
 * @return 空白削除後の文字数
 */
INT32 SC_Trim(Char *str)
{
	INT32	i;
	INT32	len;
	INT32	count = 0;

	// パラメータチェック
	if (NULL == str) {
		return (-1);
	}
	len = strlen((char*)str) + 1;

	// 文字列長を取得する
	i = strlen((char*)str);
	len = i;

	// 末尾から順に空白でない位置を探す
	while ((0 <= --i) && (' ' == str[i])) {
		count++;
	}

	// NULL終端文字を付加する
	str[i+1] = EOS;

	// 先頭から順に空白でない位置を探す
	i = 0;
	while ((EOS != str[i]) && (' ' == str[i])) {
		i++;
	}
#ifdef __SMS_APPLE__
	char* tmp = malloc(strlen((char*) str) + 1);
	strcpy(tmp, (char*) &str[i]);
	strcpy((char*) str, tmp);
	free(tmp);
#else
	strcpy((char*)str, (char*)&str[i]);
#endif /* __SMS_APPLE__ */

	return (len - (i + count));
}

/**
 * @brief xorチェックサム判定
 * @return true：一致 false：不一致
 */
Bool SC_CheckSumJudge(UINT8* data, UINT32 size, UINT8 check) {

	UINT8 ans = 0;

	// 結果
	if (e_SC_RESULT_SUCCESS != SC_CheckSumCalc(data, size, &ans)) {
		return (false);
	}

	if (check == ans) {
		return (true);
	} else {
		return (false);
	}
}

/**
 * @brief xorチェックサム計算
 * @return 結果
 */
E_SC_RESULT SC_CheckSumCalc(UINT8* data, UINT32 size, UINT8* sum) {

	UINT32 i = 0;
	UINT8 ans = 0;

	if (NULL == data) {
		return (e_SC_RESULT_BADPARAM);
	}

	// sum計算
	while (i < size) {
		ans ^= data[i++];
	}
	*sum = ans;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief BASICキーワードの設定
 * @param [in] str1/str2 ID/PW
 * @return
 */
E_SC_RESULT  SC_SetBasicKeyword(Char *str1, Char *str2)
{
	if( NULL != str1 ){strcpy( BasicKeyword1, str1 );}
	if( NULL != str2 ){strcpy( BasicKeyword2, str2 );}

	return (e_SC_RESULT_SUCCESS);
}
/**
 * @brief BASICキーワードの取得
 * @param [in] str1/str2 ID/PW
 * @return
 */
E_SC_RESULT  SC_GetBasicKeyword(Char *str1, Char *str2)
{
	strcpy( str1, BasicKeyword1 );
	strcpy( str2, BasicKeyword2 );

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief	DOUBLE型変数比較関数
 * @return	true:一致
 * @return	false:不一致
 */
Bool CompareDouble(DOUBLE param1, DOUBLE param2)
{
	DOUBLE diff;

	diff = param1 - param2;
	if (diff <= LIMIT && diff >= -LIMIT) {
		return (true);
	} else {
		return (false);
	}
}

/**
 * @brief ディレクトリ作成
 * @param[in] dirPath 作成するディレクトリのフルパス（最大長はNULL終端文字含めてSC_MAX_PATH）
 * @return 処理結果(E_SC_RESULT)
 * @warning パスにフォルダのフルパスを指定する場合は、末尾に/を付加すること。
 *          パスは、ファイルのフルパスでも可。
 */
E_SC_RESULT SC_MakeDir(const Char *dirPath)
{
	Char			path[SC_MAX_PATH] = {};
	Char			*pPath = NULL;
	Char			*chr = NULL;
	INT32			cnt = 0;
	//UINT32			errCode = 0;
	struct stat		st = {};

	if ((NULL == dirPath) || (EOS == *dirPath)) {
		return (e_SC_RESULT_BADPARAM);
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
					return (e_SC_RESULT_FILE_ACCESSERR);
				}
			}
			*chr = '/';
		}
		pPath = chr + 1;
		cnt++;
	}

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief リトルエンディアンか否か
 * @return リトルエンディアン：true、リトルエンディアン以外：false
 */
Bool IsLittleEndian()
{
	INT32	num = 1;

	return ((Bool)(*(Char*)&num));
}

// 戻り値変換（E_SC_CAL_RESULT → E_SC_RESULT）
E_SC_RESULT ConvertResult(E_SC_CAL_RESULT calRet)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	switch (calRet) {
	case e_SC_CAL_RESULT_SUCCESS:
		ret = e_SC_RESULT_SUCCESS;
		break;
	case e_SC_CAL_RESULT_CANCEL:
		ret = e_SC_RESULT_CANCEL;
		break;
	case e_SC_CAL_RESULT_FAIL:
		ret = e_SC_RESULT_FAIL;
		break;
	case e_SC_CAL_RESULT_MALLOC_ERR:
		ret = e_SC_RESULT_MALLOC_ERR;
		break;
	case e_SC_CAL_RESULT_BADPARAM:
		ret = e_SC_RESULT_BADPARAM;
		break;
	case e_SC_CAL_RESULT_FILE_ACCESSERR:
		ret = e_SC_RESULT_FILE_ACCESSERR;
		break;
	case e_SC_CAL_RESULT_TCP_CONNECT_ERROR:
		ret = e_SC_RESULT_TCP_CONNECT_ERROR;
		break;
	case e_SC_CAL_RESULT_TCP_COMMUNICATION_ERR:
		ret = e_SC_RESULT_TCP_COMMUNICATION_ERR;
		break;
	case e_SC_CAL_RESULT_TCP_TIMEOUT:
		ret = e_SC_RESULT_TCP_TIMEOUT;
		break;
	case e_SC_CAL_RESULT_TCP_DISCONNECTED:
		ret = e_SC_RESULT_TCP_DISCONNECTED;
		break;
	default:
		ret = e_SC_RESULT_FAIL;
		break;
	}

	return (ret);
}
