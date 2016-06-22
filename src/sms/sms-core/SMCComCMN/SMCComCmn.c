/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#include "SMCComCMNInternal.h"

#define	CC_CMN_FILE_READ_WRITE_SIZE			(1024 * 1024)
#define	CC_CMNDL_FILE_READ_BUFF_SIZE		(1024)

Char	rootDirPath[SCC_MAX_PATH];
Char	dataDirPath[SCC_MAX_PATH];
Char	configDirPath[SCC_MAX_PATH];
Bool	isCancel;
Bool	isCancel_Polling;
Bool	isLogined;

T_CC_CMN_CONNECT_INFO connectInfo;

//************************************************
//文字列⇒数値変換
//************************************************
/**
 * @brief statusの文字列を数値に変換
 * @param [IN]apStatus <status>の文字列のポインタ
 * @retval OK : 0
 * @retval NG : -1
 */
INT32 CC_ConvertStsStrToNum(const Char *apStatus)
{
	INT32 status;

	if ('I' == apStatus[4]) {
		/* OK */
		status = CC_CMN_XML_CIC_RES_STS_OK;
	} else {
		/* NG */
		status = CC_CMN_XML_CIC_RES_STS_NG;
	}

	return (status);
}

/**
 * @brief ディレクトリ作成
 * @param[in] dirPath 作成するディレクトリのフルパス（最大長はNULL終端文字含めてSC_MAX_PATH）
 * @return 処理結果(E_SC_RESULT)
 * @warning パスにフォルダのフルパスを指定する場合は、末尾に/を付加すること。
 *          パスは、ファイルのフルパスでも可。
 */
E_SC_RESULT CC_MakeDir(const Char *dirPath)
{
	Char			path[SCC_MAX_PATH] = {};
	Char			*pPath = NULL;
	Char			*chr = NULL;
	INT32			cnt = 0;
	//UINT32			errCode = 0;
	struct stat		st = {};

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	if (CC_ISINVALIDSTR(dirPath)) {
		SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[dirPath], " HERE);
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
					SCC_LOG_ErrorPrint(SC_TAG_CC, "mkdir error(0x%08x), " HERE, errno);
					SCC_LOG_ErrorPrint(SC_TAG_CC, "path=%s, " HERE, path);
					return (e_SC_RESULT_FILE_ACCESSERR);
				}else{
					// 正常終了したら、sync()する
					sync();
				}
			}
			*chr = '/';
		}
		pPath = chr + 1;
		cnt++;
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief ディレクトリ削除
 * @param[in] dirPath 削除するディレクトリのフルパス（最大長はNULL終端文字含めてSC_MAX_PATH）
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_DeleteDir(const Char *dirPath)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	Char			*path = NULL;		// ファイルパス
	DIR				*dir = NULL;			// ディレクトリ情報
	struct dirent	*dent = NULL;			// dirent 構造体へのポインタ
	struct stat		sts = {};				// ファイルの情報

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISINVALIDSTR(dirPath)) {
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// ディレクトリを開く
		dir = opendir((char*)dirPath);
		if (NULL == dir) {
			if (ENOENT == errno) {
				// ディレクトリが存在しない場合は、正常終了する
				SCC_LOG_InfoPrint(SC_TAG_CC, "dir not found[%s], " HERE, dirPath);
			} else {
				SCC_LOG_ErrorPrint(SC_TAG_CC, "opendir error[%s] (0x%08x), " HERE, dirPath, errno);
				ret = e_SC_RESULT_FILE_ACCESSERR;
			}
			break;
		}

		// メモリ確保
		path = (Char*)SCC_MALLOC(SCC_MAX_PATH);
		if (NULL == path) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// ディレクトリを読み込む
		while (NULL != (dent = readdir(dir))) {
			if (0 == strcmp(dent->d_name, ".") || 0 == strcmp(dent->d_name, "..")) {
				// 読み飛ばす
				continue;
			}

			// ディレクトリ・ファイルフルパス
			if ('/' == dirPath[strlen(dirPath) - 1]) {
				sprintf((char*)path, "%s%s", dirPath, dent->d_name);
			} else {
				sprintf((char*)path, "%s/%s", dirPath, dent->d_name);
			}

			// ファイルの状態取得
			if (0 != stat((char*)path, &sts)) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, "stat error[%s] (0x%08x), " HERE, dirPath, errno);
				ret = e_SC_RESULT_FILE_ACCESSERR;
				break;
			}
			if (S_IWUSR != (sts.st_mode & S_IWUSR) ||
				S_IWGRP != (sts.st_mode & S_IWGRP) ||
				S_IWOTH != (sts.st_mode & S_IWOTH)) {
				// 読み取り専用属性を外す
				chmod((char*)path, (sts.st_mode | S_IWUSR | S_IWGRP | S_IWOTH));
			}

			if (S_ISDIR(sts.st_mode)) {
				// ディレクトリ配下削除
				ret = CC_DeleteDir(path);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, "CC_DeleteDir error, " HERE);
					break;
				}
			} else {
				// ファイル削除
				ret = CC_DeleteFile(path);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, "CC_DeleteFile error, " HERE);
					break;
				}
			}
		}

		// ディレクトリを閉じる
		closedir(dir);
		dir = NULL;

		if (e_SC_RESULT_SUCCESS == ret) {
			// ディレクトリ削除
			ret = CC_DeleteFile(dirPath);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, "CC_DeleteFile error, " HERE);
				break;
			}
		}
	} while (0);

	if (NULL != dir) {
		// ディレクトリを閉じる
		closedir(dir);
	}

	// メモリ解放
	if (NULL != path) {
		SCC_FREE(path);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief ファイル削除
 * @param[in] filePath 削除するファイルのフルパス（最大長はNULL終端文字含めてSC_MAX_PATH）
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_DeleteFile(const Char *filePath)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	if (0 != remove((char*)filePath)) {
		// 削除エラー
		SCC_LOG_ErrorPrint(SC_TAG_CC, "remove error[%s] (0x%08x), " HERE, filePath, errno);
		ret = e_SC_RESULT_FILE_ACCESSERR;
	}

	return (ret);
}

/**
 * @brief ファイルの存在チェック作成
 * @param[in] filePath  チェックするファイルのフルパス
 * @return ファイル有無（true:存在する、false:存在しない）
 */
Bool CC_CheckFileExist(const Char *filePath)
{
	Bool	ret = false;
	struct stat st = {};

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	if (!CC_ISINVALIDSTR(filePath)) {
		// ファイルの有無チェック
		if (0 == stat(filePath, &st)) {
			ret = true;
		}
	} else {
		SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[filePath], " HERE);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}


/**
 * @brief Mutexを生成する
 * @param[in] mutex ミューテックスオブジェクトのポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SCC_CreateMutex(SCC_MUTEX *mutex)
{
	// パラメータチェック
	if (CC_ISNULL(mutex)) {
		SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[mutex], " HERE);
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
E_SC_RESULT SCC_DestroyMutex(SCC_MUTEX *mutex)
{
	INT32 ret = 0;

	// パラメータチェック
	if (CC_ISNULL(mutex)) {
		LOG_PRINT_ERROR(SC_TAG_CC, (char*)"param error[mutex]");
		return (e_SC_RESULT_BADPARAM);
	}

	// Mutex破棄
	ret = pthread_mutex_destroy(mutex);
	if (0 != ret) {
		LOG_PRINT_ERRORNO(SC_TAG_CC, (char*)"pthread_mutex_destroy error(0x%08x)", errno);
		return (e_SC_RESULT_FAIL);
	}

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief Mutexをロックする
 * @param[in] mutex ミューテックスオブジェクトのポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SCC_LockMutex(SCC_MUTEX *mutex)
{
	INT32 ret = 0;

	// パラメータチェック
	if (CC_ISNULL(mutex)) {
		SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[mutex], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// Mutexロック
	ret = pthread_mutex_lock(mutex);
	if (0 != ret) {
		SCC_LOG_ErrorPrint(SC_TAG_CC, "pthread_mutex_lock error(0x%08x), " HERE, errno);
		return (e_SC_RESULT_FAIL);
	}

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief Mutexをアンロックする
 * @param[in] mutex ミューテックスオブジェクトのポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SCC_UnLockMutex(SCC_MUTEX *mutex)
{
	INT32 ret = 0;

	// パラメータチェック
	if (CC_ISNULL(mutex)) {
		SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[mutex], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// Mutexアンロック
	ret = pthread_mutex_unlock(mutex);
	if (0 != ret) {
		SCC_LOG_ErrorPrint(SC_TAG_CC, "pthread_mutex_unlock error(0x%08x), " HERE, errno);
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
E_SC_RESULT SCC_CreateSemaphore(SCC_SEMAPHORE *sem, UINT32 val)
{
	// パラメータチェック
	if (CC_ISNULL(sem)) {
		SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[mutex], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	// セマフォ初期化
	if (0 != sem_init(sem, 0, val)) {
		SCC_LOG_ErrorPrint(SC_TAG_CC, "sem_init error(0x%08x), " HERE, errno);
		return (e_SC_RESULT_FAIL);
	}

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief セマフォを破棄する
 * @param[in] sem セマフォオブジェクトのポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SCC_DestroySemaphore(SCC_SEMAPHORE *sem)
{
	//INT32 ret = 0;

	// パラメータチェック
	if (CC_ISNULL(sem)) {
		SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[mutex], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// セマフォ破棄
	if (0 != sem_destroy(sem)) {
		SCC_LOG_ErrorPrint(SC_TAG_CC, "sem_destroy error(0x%08x), " HERE, errno);
		return (e_SC_RESULT_FAIL);
	}

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief セマフォをロックする
 * @param[in] sem セマフォオブジェクトのポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SCC_LockSemaphore(SCC_SEMAPHORE *sem)
{
	INT32 ret = 0;

	// パラメータチェック
	if (CC_ISNULL(sem)) {
		SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[mutex], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// セマフォロック
	ret = sem_wait(sem);
	if (0 != ret) {
		SCC_LOG_ErrorPrint(SC_TAG_CC, "sem_wait error(0x%08x), " HERE, errno);
		return (e_SC_RESULT_FAIL);
	}

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief セマフォ(タイムアウトあり)をロックする
 * @param[in] sem セマフォオブジェクトのポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SCC_LockTimeSemaphore(SCC_SEMAPHORE *sem, UINT32 sec)
{
	INT32 ret = 0;
    struct timespec	timeout;

	// パラメータチェック
	if (CC_ISNULL(sem)) {
		SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[mutex], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

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
	memset(&timeout, 0x00, sizeof(timeout));
	clock_gettime(CLOCK_REALTIME, &timeout);
	timeout.tv_sec += sec;

    // セマフォロック
	ret = sem_timedwait(sem, &timeout);
#endif /* __SMS_APPLE__ */
	if (0 != ret) {
		SCC_LOG_ErrorPrint(SC_TAG_CC, "sem_timedwait error(0x%08x), " HERE, errno);
		return (e_SC_RESULT_FAIL);
	}

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief セマフォをアンロックする
 * @param[in] sem セマフォオブジェクトのポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SCC_UnLockSemaphore(SCC_SEMAPHORE *sem)
{
	INT32 ret = 0;

	// パラメータチェック
	if (CC_ISNULL(sem)) {
		SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[sem], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// セマフォアンロック
	ret = sem_post(sem);
	if (0 != ret) {
		SCC_LOG_ErrorPrint(SC_TAG_CC, "sem_post error(0x%08x), " HERE, errno);
		return (e_SC_RESULT_FAIL);
	}

	return (e_SC_RESULT_SUCCESS);
}

// ルートフォルダパス(/～/jp.co.hitachi.smsfv.aa/)設定
#ifdef __SMS_APPLE__
void SCC_SetRootDirPath(const Char *rootDir)
{
	Char	*chr = NULL;

	rootDirPath[0] = EOS;
	if ((!CC_ISINVALIDSTR(rootDir)) && (sizeof(rootDirPath) > (strlen(rootDir)))){
		strcpy((char*)rootDirPath, (char*)rootDir);
		if ((rootDirPath[strlen((char*)rootDirPath) - 1] != '/') &&
				(sizeof(rootDirPath) > (strlen((char*)rootDir) + 1))) {
			strcat((char*)rootDirPath, "/");
		}
	}
}
#else
void SCC_SetRootDirPath(const Char *dirPath)
{
	Char	*chr = NULL;

	rootDirPath[0] = EOS;
	if ((!CC_ISINVALIDSTR(dirPath)) && (sizeof(rootDirPath) > (strlen(dirPath)))) {
		// ルートフォルダ名検索
		chr = strstr(dirPath, SCC_CMN_ROOT_DIR);
		if (NULL != chr) {
			chr = strstr((chr + strlen((char*)SCC_CMN_ROOT_DIR)), CC_CMN_DATA_DIR_NAME);
			if (NULL != chr) {
				// "Data"の前までをルートフォルダのパスとする
				strncpy((char*)rootDirPath, (char*)dirPath, ((UINT32)(chr - dirPath)));
				rootDirPath[((UINT32)(chr - dirPath))] = EOS;
			}
		}
	}
}
#endif /* __SMS_APPLE__ */

// ルートフォルダパス(/～/jp.co.hitachi.smsfv.aa/)取得
const Char *SCC_GetRootDirPath()
{
	return ((const Char*)rootDirPath);
}

// Dataフォルダパス(/～/jp.co.hitachi.smsfv.aa/Data/)設定
void SCC_SetDataDirPath(const Char *dirPath)
{
	dataDirPath[0] = EOS;
	if ((!CC_ISINVALIDSTR(dirPath)) && (sizeof(dataDirPath) > (strlen(dirPath)))) {
		strcpy((char*)dataDirPath, (char*)dirPath);
		if ((dataDirPath[strlen((char*)dataDirPath) - 1] != '/') &&
			(sizeof(dataDirPath) > (strlen((char*)dirPath) + 1))) {
			strcat((char*)dataDirPath, "/");
		}
	}
}

// Dataフォルダパス(/～/jp.co.hitachi.smsfv.aa/Data/)取得
const Char *SCC_GetDataDirPath()
{
	return ((const Char*)dataDirPath);
}

// Configフォルダパス(/～/jp.co.hitachi.smsfv.aa/Data/Config/)設定
void SCC_SetConfigDirPath(const Char *dirPath)
{
	configDirPath[0] = EOS;
	if ((!CC_ISINVALIDSTR(dirPath)) && (sizeof(configDirPath) > (strlen(dirPath)))) {
		strcpy((char*)configDirPath, (char*)dirPath);
		if ((configDirPath[strlen((char*)configDirPath) - 1] != '/') &&
			(sizeof(configDirPath) > (strlen((char*)dirPath) + 1))) {
			strcat((char*)configDirPath, "/");
		}
	}
}

// Configフォルダパス(/～/jp.co.hitachi.smsfv.aa/Data/Config/)取得
const Char *SCC_GetConfigDirPath()
{
	return ((const Char*)configDirPath);
}

/**
 * @brief キャンセル
 */
void SCC_Cancel()
{
	isCancel = true;
}
void SCC_Cancel_Polling()
{
	isCancel_Polling = true;
}

/**
 * @brief キャンセルクリア
 */
void SCC_CancelClear()
{
	isCancel = false;
}
void SCC_CancelClear_Polling()
{
	isCancel_Polling = false;
}

/**
 * @brief キャンセルかチェック
 */
Bool SCC_IsCancel()
{
	return (isCancel);
}
Bool SCC_IsCancel_Polling()
{
	return (isCancel_Polling);
}

/**
 * @brief ログイン
 */
void SCC_Login()
{
	isLogined = true;
}

/**
 * @brief ログアウト
 */
void SCC_Logout()
{
	isLogined = false;
}

/*
 * 文字列中の大文字を小文字に変換する
 * @param [IN/OUT] str  変換したい文字列
 * @return 変換後の文字列の先頭アドレス
 */
Char *SCC_ToLowerString(Char *str)
{
	return (SCC_ToLowerStringLen(str, strlen(str)));
}

/*
 * 文字列中の大文字を小文字に変換する
 * @param [IN/OUT] str  変換したい文字列
 * @return 変換後の文字列の先頭アドレス
 */
Char *SCC_ToLowerStringLen(Char *str, UINT32 len)
{
	INT32	i = 0;

	while (len > i) {
		str[i] = tolower((unsigned char)str[i]);
		i++;
	}

    return (str);
}

/*
 * ファイル(フォルダも可)を移動する
 * @param [IN] srcPath  移動元ファイルパス
 * @param [IN] dstPath  移動先ファイルパス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SCC_MoveFile(const Char *srcPath, const Char *dstPath)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISINVALIDSTR(srcPath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[srcPath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISINVALIDSTR(dstPath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[dstPath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// ファイル移動
		if (0 != rename((const char*)srcPath, (const char*)dstPath)) {
			ret = e_SC_RESULT_FILE_ACCESSERR;
			SCC_LOG_ErrorPrint(SC_TAG_CC, "rename error(0x%08x), " HERE, errno);
			SCC_LOG_ErrorPrint(SC_TAG_CC, "srcPath=%s, " HERE, srcPath);
			SCC_LOG_ErrorPrint(SC_TAG_CC, "dstPath=%s, " HERE, dstPath);
			break;
		}
	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/*
 * ファイルをコピーする(ファイルサイズ分メモリ確保して一括コピー)
 * @param [IN] srcPath  コピー元ファイルパス
 * @param [IN] dstPath  コピー先ファイルパス
 * @param [IN] isRemove コピー元ファイルを削除するか(true:削除する、false:削除しない)
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SCC_CopyFile(const Char *srcPath, const Char *dstPath, Bool isRemove)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	FILE	*srcFp = NULL;
	FILE	*dstFp = NULL;
	struct stat st = {};
	Char	*data = NULL;
	UINT32	len = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISINVALIDSTR(srcPath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[srcPath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISINVALIDSTR(dstPath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[dstPath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (!strcmp(srcPath, dstPath)) {
			SCC_LOG_DebugPrint(SC_TAG_CC, "no copy, " HERE);
			break;
		}

		// ファイルオープン
		srcFp = fopen(srcPath, "rb");
		if (NULL == srcFp) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"fopen error(0x%08x), " HERE, errno);
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"srcPath=%s, " HERE, srcPath);
			ret = e_SC_RESULT_FILE_OPENERR;
			break;
		}
		// ファイルサイズ取得
		if (0 != stat((const char*)srcPath, &st)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"stat error, " HERE);
			ret = e_SC_RESULT_FILE_ACCESSERR;
			break;
		}
		// メモリ確保
		data = (Char*)SCC_MALLOC(st.st_size + 1);
		if (NULL == data) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		// ファイルリード
		len = fread(data, 1, st.st_size, srcFp);
		if (st.st_size != len) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"fread error(0x%08x), " HERE, errno);
			ret = e_SC_RESULT_FILE_ACCESSERR;
			break;
		}

		// ファイルオープン
		dstFp = fopen(dstPath, "wb");
		if (NULL == dstFp) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"fopen error(0x%08x), " HERE, errno);
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"dstPath=%s, " HERE, dstPath);
			ret = e_SC_RESULT_FILE_OPENERR;
			break;
		}
		// ファイルライト
		len = fwrite(data, 1, st.st_size, dstFp);
		if (st.st_size != len) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"fwrite error(0x%08x), " HERE, errno);
			ret = e_SC_RESULT_FILE_ACCESSERR;
			break;
		}
		if (isRemove) {
			remove(srcPath);
		}
	} while (0);

	// メモリ解放
	if (NULL != data) {
		SCC_FREE(data);
	}

	// ファイルクローズ
	if (NULL != srcFp) {
		fclose(srcFp);
	}
	if (NULL != dstFp) {
		fclose(dstFp);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/*
 * ファイルをコピーする(1MBメモリ確保して分割コピー)
 * @param [IN] srcPath  コピー元ファイルパス
 * @param [IN] dstPath  コピー先ファイルパス
 * @param [IN] isRemove コピー元ファイルを削除するか(true:削除する、false:削除しない)
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SCC_CopyFile2(const Char *srcPath, const Char *dstPath, Bool isRemove)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	FILE	*srcFp = NULL;
	FILE	*dstFp = NULL;
	Char	*data = NULL;
	INT32	readSize = 0;
	INT32	writeSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISINVALIDSTR(srcPath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[srcPath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISINVALIDSTR(dstPath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[dstPath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (!strcmp(srcPath, dstPath)) {
			SCC_LOG_DebugPrint(SC_TAG_CC, "no copy, " HERE);
			break;
		}

		// ファイルオープン
		srcFp = fopen(srcPath, "rb");
		if (NULL == srcFp) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"fopen error(0x%08x), " HERE, errno);
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"srcPath=%s, " HERE, srcPath);
			ret = e_SC_RESULT_FILE_OPENERR;
			break;
		}
		// メモリ確保
		data = (Char*)SCC_MALLOC(CC_CMN_FILE_READ_WRITE_SIZE);
		if (NULL == data) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// ファイルオープン
		dstFp = fopen(dstPath, "wb");
		if (NULL == dstFp) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"fopen error(0x%08x), " HERE, errno);
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"dstPath=%s, " HERE, dstPath);
			ret = e_SC_RESULT_FILE_OPENERR;
			break;
		}

		// EOFまで繰り返す
		while (0 == feof(srcFp)) {
			// ファイルリード
			readSize = fread(data, 1, CC_CMN_FILE_READ_WRITE_SIZE, srcFp);
			if (0 >= readSize) {
				if (0 == feof(srcFp)) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"fread error(0x%08x), " HERE, errno);
					ret = e_SC_RESULT_FILE_ACCESSERR;
				}
				break;
			}

			// ファイルライト
			writeSize = fwrite(data, 1, readSize, dstFp);
			if (readSize != writeSize) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"fwrite error(0x%08x), " HERE, errno);
				ret = e_SC_RESULT_FILE_ACCESSERR;
				break;
			}
		}
		if (e_SC_RESULT_SUCCESS != ret) {
			break;
		}

		if (isRemove) {
			// ファイル削除
			remove(srcPath);
		}
	} while (0);

	// メモリ解放
	if (NULL != data) {
		SCC_FREE(data);
	}

	// ファイルクローズ
	if (NULL != srcFp) {
		fclose(srcFp);
	}
	if (NULL != dstFp) {
		fclose(dstFp);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/*
 * ディレクトリをコピーする
 * @param [IN] srcPath  コピー元ディレクトリパス(パスの末尾は'/'を付けること)
 * @param [IN] dstPath  コピー先ディレクトリパス(パスの末尾は'/'を付けること)
 * @param [IN] isRemove コピー元ディレクトリを削除するか(true:削除する、false:削除しない)
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SCC_CopyDir(const Char *srcDirPath, const Char *dstDirPath, Bool isRemove)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	FILE	*srcFp = NULL;
	FILE	*dstFp = NULL;
	DIR		*dir = NULL;
	struct dirent	*dent = NULL;
	Char	*srcPath = NULL;
	Char	*dstPath = NULL;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISINVALIDSTR(srcDirPath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[srcDirPath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISINVALIDSTR(dstDirPath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[dstDirPath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (!strcmp(srcDirPath, dstDirPath)) {
			SCC_LOG_DebugPrint(SC_TAG_CC, "no copy, " HERE);
			break;
		}

		// ディレクトリを開く
		dir = opendir((const char*)srcDirPath);
		if (NULL == dir) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "opendir error[%s] (0x%08x), " HERE, srcDirPath, errno);
			ret = e_SC_RESULT_FILE_ACCESSERR;
			break;
		}

		// メモリ確保
		srcPath = (Char*)SCC_MALLOC(SCC_MAX_PATH);
		if (NULL == srcPath) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		dstPath = (Char*)SCC_MALLOC(SCC_MAX_PATH);
		if (NULL == dstPath) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// ディレクトリを読み込む
		while (NULL != (dent = readdir(dir))) {
			if (0 == strcmp(dent->d_name, ".") || 0 == strcmp(dent->d_name, "..")) {
				// 読み飛ばす
				continue;
			}

			// パス生成
			sprintf((char*)srcPath, "%s%s", srcDirPath, dent->d_name);
			sprintf((char*)dstPath, "%s%s", dstDirPath, dent->d_name);

			if (DT_DIR == dent->d_type) {
				// ディレクトリ
				strcat((char*)dstPath, (char*)"/");

				// ディレクトリ作成
				ret = CC_MakeDir(dstPath);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, "CC_MakeDir error, " HERE);
					break;
				}

				// ディレクトリコピー(再起関数)
				ret = SCC_CopyDir(srcPath, dstPath, isRemove);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, "SCC_CopyDir error, " HERE);
					break;
				}
			} else if (DT_REG == dent->d_type) {
				// ファイル
				// ファイルコピー
				ret = SCC_CopyFile2(srcPath, dstPath, isRemove);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, "SCC_CopyFile error, " HERE);
					break;
				}
			}
		}

		// ディレクトリを閉じる
		closedir(dir);

		if (e_SC_RESULT_SUCCESS != ret) {
			break;
		}

		if (isRemove) {
			// ディレクトリ削除(戻り値は見ない)
			CC_DeleteDir(srcPath);
		}
	} while (0);

	// メモリ解放
	if (NULL != srcPath) {
		SCC_FREE(srcPath);
	}
	if (NULL != dstPath) {
		SCC_FREE(dstPath);
	}

	// ファイルクローズ
	if (NULL != srcFp) {
		fclose(srcFp);
	}
	if (NULL != dstFp) {
		fclose(dstFp);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/*
 * 画像ファイルのMIME TYPEを取得する
 * @param [IN] filePath 画像ファイルパス
 * @param [IN] mimeType 画像のMIME TYPE
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_OK以外 (コール関数の戻り値)
 */
E_SC_RESULT SCC_GetImageMIMEType(const Char *filePath, CC_IMAGE_MIMETYPE *mimeType)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	FILE	*fp = NULL;
	Char	data[12] = {};
	INT32	len = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISINVALIDSTR(filePath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[filePath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(mimeType)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[mimeType], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// ファイルオープン
		fp = fopen(filePath, "rb");
		if (NULL == fp) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "fopen error[filePath=%s], " HERE, filePath);
			ret = e_SC_RESULT_FILE_OPENERR;
			break;
		}

		// ファイルリード
		len = fread(data, 1, (sizeof(data) - 1), fp);
		if ((sizeof(data) - 1) != len) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "fread error[filePath=%s], " HERE, filePath);
			ret = e_SC_RESULT_FILE_ACCESSERR;
			break;
		}

		// 大文字を小文字に変換
		SCC_ToLowerStringLen(data, len);
		// ファイルフォーマットチェック
		if (('j' == data[6]) && ('f' == data[7]) && ('i' == data[8]) && ('f' == data[9])) {
			// jpg(JFIF形式)
			*mimeType = CC_IMAGE_MIMETYPE_JPG;
		} else if (('e' == data[6]) && ('x' == data[7]) && ('i' == data[8]) && ('f' == data[9])) {
			// jpg(Exif形式)
			*mimeType = CC_IMAGE_MIMETYPE_JPG;
		} else if (('p' == data[1]) && ('n' == data[2]) && ('g' == data[3])) {
			// png
			*mimeType = CC_IMAGE_MIMETYPE_PNG;
		} else if (('g' == data[0]) && ('i' == data[1]) && ('f' == data[2])) {
			// gif
			*mimeType = CC_IMAGE_MIMETYPE_GIF;
		} else {
			*mimeType = CC_IMAGE_MIMETYPE_NONE;
		}
	} while (0);

	if (NULL != fp) {
		fclose(fp);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief 16進数文字列をバイトの16進数コードの文字列に変換
 * @param [in]  str     変換する文字列
 * @param [in]  strLen  変換する文字列長
 * @param [out] hexStr  変換後の文字列
 * @retval 正常終了  : DAL_DATA_OK
 * @retval 異常終了  : DAL_DATA_ERROR
 * @note "0123456789ABCDEF" => "0x01 0x23 0x45 0x67 0x89 0xAB 0xCD 0xEF"
 */
E_SC_RESULT CC_ChgHexString(const UChar *str, UINT32 strLen, UChar *hexStr)
{
	UINT32	i = 0;
	UINT8	num[2] = {};

	while (i < strLen) {
		if ('0' <= str[i] && '9' >= str[i]) {
			num[i % 2] = (str[i] & 0x0f);
		} else if ('a' == str[i] || 'A' == str[i]) {
			num[i % 2] = 0x0a;
		} else if ('b' == str[i] || 'B' == str[i]) {
			num[i % 2] = 0x0b;
		} else if ('c' == str[i] || 'C' == str[i]) {
			num[i % 2] = 0x0c;
		} else if ('d' == str[i] || 'D' == str[i]) {
			num[i % 2] = 0x0d;
		} else if ('e' == str[i] || 'E' == str[i]) {
			num[i % 2] = 0x0e;
		} else if ('f' == str[i] || 'F' == str[i]) {
			num[i % 2] = 0x0f;
		} else {
			return (e_SC_RESULT_FAIL);
		}

		if (0 != i % 2) {
			hexStr[i / 2] = (UINT8)(((num[0] << 4) & 0xf0) | num[1]);
		}
		i++;
	}

	return (e_SC_RESULT_SUCCESS);
}

/*
 * ファイルサイズをチェックする
 * @param [IN] filePath ファイルパス
 * @param [IN] fileSize ファイルサイズ
 * @retval 正常終了  :true
 * @retval エラー終了:false
 */
Bool CC_CheckFileSize(const Char *filePath, UINT32 fileSize)
{
	Bool	ret = false;
	struct stat	st = {};

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		if (NULL == filePath) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[filePath], " HERE);
			break;
		}
		if (0 != stat(filePath, &st)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "file not found(0x%08x), filePath=%s, " HERE, errno, filePath);
			break;
		}
		if (fileSize != (UINT32)st.st_size) {
			SCC_LOG_DebugPrint(SC_TAG_CC, "file size unmatch[filePath=%s, %u != %u], " HERE, filePath, fileSize, (UINT32)st.st_size);
			break;
		}
		ret = true;
	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/*
 * ネットワーク接続先情報を更新する。
 * @param [IN] T_CC_CMN_CONNECT_INFO conn
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_OK以外 (コール関数の戻り値)
 */
E_SC_RESULT SCC_SetConnectInfo( T_CC_CMN_CONNECT_INFO conn )
{

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	memset( &connectInfo, 0x00, sizeof(T_CC_CMN_CONNECT_INFO) );
	strcpy( connectInfo.portalAPI, conn.portalAPI );
	strcpy( connectInfo.smsSpAPI,  conn.smsSpAPI );
	strcpy( connectInfo.smsAuthAPI,conn.smsAuthAPI );
	strcpy( connectInfo.MUPSAPI  , conn.MUPSAPI   );
	strcpy( connectInfo.keyword1 , conn.keyword1  );
	strcpy( connectInfo.keyword2 , conn.keyword2  );

	SCC_LOG_InfoPrint(SC_TAG_CC, "portalAPI[%s], " HERE, connectInfo.portalAPI);
	SCC_LOG_InfoPrint(SC_TAG_CC, "smsSpAPI[%s],  " HERE, connectInfo.smsSpAPI);
	SCC_LOG_InfoPrint(SC_TAG_CC, "smsAuthAPI[%s]," HERE, connectInfo.smsAuthAPI);
	SCC_LOG_InfoPrint(SC_TAG_CC, "MUPSAPI  [%s], " HERE, connectInfo.MUPSAPI  );
	SCC_LOG_InfoPrint(SC_TAG_CC, "keyword1 [%s], " HERE, connectInfo.keyword1 );
	SCC_LOG_InfoPrint(SC_TAG_CC, "keyword2 [%s], " HERE, connectInfo.keyword2 );

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return CC_CMN_RESULT_OK;
}

/*
 * ネットワーク接続先情報(ポータル APIリクエスト送付先)を取得する。
 * @retval
 */
const char *SCC_GetPortalAPI()
{
	return (connectInfo.portalAPI);
}
/*
 * ネットワーク接続先情報(ポータル sms_sp APIリクエスト送付先)を取得する。
 * @retval
 */
const char *SCC_GetSmsSpAPI()
{
	return (connectInfo.smsSpAPI);
}
/*
 * ネットワーク接続先情報(ポータル sms_auth APIリクエスト送付先)を取得する。
 * @retval
 */
const char *SCC_GetSmsAuthAPI()
{
	return (connectInfo.smsAuthAPI);
}
/*
 * ネットワーク接続先情報(MUPS APIリクエスト送付先)を取得する。
 * @retval
 */
const char *SCC_GetMUPSAPI()
{
	return (connectInfo.MUPSAPI);
}
/*
 * ネットワーク接続先情報(BASIC認証 ID)を取得する。
 * @retval
 */
const char *SCC_GetKeyword1()
{
	return (connectInfo.keyword1);
}
/*
 * ネットワーク接続先情報(BASIC認証 PW)を取得する。
 * @retval
 */
const char *SCC_GetKeyword2()
{
	return (connectInfo.keyword2);
}

/**
 * @brief アプリバージョン文字列を分割する
 * @param[in]  strAppVer        アプリバージョン文字列
 * @param[out] appVersion       分解したアプリバージョン
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_RESULT CC_CmnDL_SplitAppVersion(const Char *strAppVer, CC_APP_VERSION *appVersion)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	Char	appVer[36] = {};
	Char	*ver = NULL;
	Char	*chr = NULL;

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// バージョンは、X.X.YYYYMMDD-XX形式
		strcpy((char*)appVer, (char*)strAppVer);
		ver = (Char*)appVer;

		// バージョンを分割
		chr = strchr((char*)ver, '.');
		if (NULL == chr) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"APP_VERSION format error[%s], " HERE, strAppVer);
			ret = e_SC_RESULT_FAIL;
			break;
		}
		*chr = EOS;
		appVersion->ver1 = atoi((char*)ver);
		ver = (chr + 1);

		// バージョンを分割
		chr = strchr((char*)ver, '.');
		if (NULL == chr) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"APP_VERSION format error[%s], " HERE, strAppVer);
			ret = e_SC_RESULT_FAIL;
			break;
		}
		*chr = EOS;
		appVersion->ver2 = atoi((char*)ver);
		ver = (chr + 1);

		// バージョンを分割
		chr = strchr((char*)ver, '-');
		if (NULL == chr) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"APP_VERSION format error[%s], " HERE, strAppVer);
			ret = e_SC_RESULT_FAIL;
			break;
		}
		*chr = EOS;
		appVersion->ver3 = atoi((char*)ver);
		ver = (chr + 1);

		appVersion->ver4 = atoi((char*)ver);
	} while (0);

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief ファイルをチェックする
 * @param[in] filePath  ファイルパス
 * @param[in] md5       MD5ハッシュ値
 * @param[in] fileSize  ファイルサイズ
 * @return 正常終了:true
 * @return 異常終了:false
 */
E_SC_RESULT CC_CmnDL_CheckFile(const Char *filePath, const UChar *md5, UINT32 fileSize)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	UChar	md5Low[CC_CMN_MD5] = {};
	UChar	md5Hash[CC_CMN_MD5] = {};
	T_CC_CMN_MD5_CTX	ctx = {};
	UChar	*data = NULL;
	FILE	*fp = NULL;
	INT32	readSize = 0;
	//Bool	isError = false;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISINVALIDSTR(filePath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[filePath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(md5)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[md5], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// メモリ確保
		data = (UChar*)SCC_MALLOC(CC_CMNDL_FILE_READ_BUFF_SIZE);
		if (NULL == data) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		memset(data, 0, CC_CMNDL_FILE_READ_BUFF_SIZE);

		if (0 != fileSize) {
			// ファイルサイズチェック
			if (!CC_CheckFileSize(filePath, fileSize)) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_CheckFileSize error, " HERE);
				ret = e_SC_RESULT_FAIL;
				break;
			}
		}

		// ファイルオープン
		fp = fopen(filePath, "rb");
		if (NULL == fp) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"fopen error(0x%08x), path=%s, " HERE, errno, filePath);
			ret = e_SC_RESULT_FILE_OPENERR;
			break;
		}

		// MD5初期化
		CC_MD5_Init(&ctx);

		// EOFまで繰り返す
		while (0 == feof(fp)) {
			// キャンセルチェック
			if (CC_ISCANCEL()) {
				SCC_LOG_WarnPrint(SC_TAG_CC, (Char*)"cancel, " HERE);
				ret = e_SC_RESULT_CANCEL;
				break;
			}
			// ファイルリード
			readSize = fread(data, 1, CC_CMNDL_FILE_READ_BUFF_SIZE, fp);
			if (0 >= readSize) {
				if (0 == feof(fp)) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"fread error(0x%08x), path=%s, " HERE, errno, filePath);
					ret = e_SC_RESULT_FILE_ACCESSERR;
				}
				break;
			}

			// MD5計算
			CC_MD5_Update(&ctx, data, readSize);
		}
		if (e_SC_RESULT_SUCCESS != ret) {
			break;
		}
		// MD5終了化
		CC_MD5_Final(md5Hash, &ctx);

		// 大文字を小文字に変換
		memcpy(md5Low, md5, sizeof(md5Low));
		SCC_ToLowerStringLen(md5Low, sizeof(md5Low));
		SCC_ToLowerStringLen(md5Hash, sizeof(md5Hash));
		// MD5チェック
		if (0 != memcmp(md5Low, md5Hash, sizeof(md5Hash))) {
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"MD5 unmatch, " HERE);
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"base=%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x, " HERE,
							   md5Low[0], md5Low[1], md5Low[2],  md5Low[3],  md5Low[4],  md5Low[5],  md5Low[6],  md5Low[7],
							   md5Low[8], md5Low[9], md5Low[10], md5Low[11], md5Low[12], md5Low[13], md5Low[14], md5Low[15]);
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"calc=%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x, " HERE,
							   md5Hash[0], md5Hash[1], md5Hash[2],  md5Hash[3],  md5Hash[4],  md5Hash[5],  md5Hash[6],  md5Hash[7],
							   md5Hash[8], md5Hash[9], md5Hash[10], md5Hash[11], md5Hash[12], md5Hash[13], md5Hash[14], md5Hash[15]);
			ret = e_SC_RESULT_FAIL;
			break;
		}
	} while (0);

	// メモリ解放
	if (NULL != data) {
		SCC_FREE(data);
	}

	// ファイルクローズ
	if (NULL != fp) {
		fclose(fp);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

// tar.gzファイル解凍
E_SC_RESULT CC_UnTgz(const Char *filePath, const Char *dstPath, const SMPROGRESSCBFNC *cbFnc)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

#if 0 /* aikawa 2016/04/14 暫定 */
	TAR_CANCEL_FNC	cancel = (TAR_CANCEL_FNC)cbFnc->cancel;
	TAR_PROGRESS_FNC	progress = (TAR_PROGRESS_FNC)cbFnc->progress;

	do {
		// パラメータチェック
		if (CC_ISINVALIDSTR(filePath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[filePath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		// tar.gzファイル解凍
		if (0 != tar2(filePath, dstPath, cancel, progress)) {
			if (CC_ISCANCEL()) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, "tar cancel, " HERE);
				ret = e_SC_RESULT_CANCEL;
			} else {
				SCC_LOG_ErrorPrint(SC_TAG_CC, "tar error, " HERE);
				ret = e_SC_RESULT_FAIL;
			}
			break;
		}
	} while (0);
#endif
	return (ret);
}

// ディスク空き容量取得
INT64 CC_GetFreeDiskSize(const Char *dirPath)
{
	INT64	freeSize = 0;				// ディスク空きサイズ[byte]
	struct statfs	sts = {};

	do {
		if (NULL == dirPath) {
			freeSize = -1;
			break;
		}
		// ファイルシステムの統計取得
		if (0 != statfs((char*)dirPath, &sts)) {
			freeSize = -1;
			break;
		}

		// ディスクの空き容量[byte]取得
		freeSize  = (sts.f_bavail * sts.f_bsize);
	} while (0);

	return (freeSize);
}

// プローブUP完了通知
E_SC_RESULT SCC_NotifyProbePostComp(SMCAL *smcal,
									const Char *fileName,
									const Char *bucketName)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	// プローブUP完了通知
	ret = CC_NotifyProbePostComp(smcal, fileName, bucketName);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

// ログイン有無取得
Bool SCC_IsLogined()
{
	return CC_ISLOGINED();
}

// 認証情報取得
E_SC_RESULT SCC_GetAuthInfo(SCC_AUTHINFO *authInfo)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	if (NULL != authInfo) {
		// 認証情報取得
		ret = CC_GetAuthInfo(&authInfo->term_id, &authInfo->term_sig, &authInfo->guid, &authInfo->user_sig);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetAuthInfo error(0x%08x), " HERE, ret);
		}
	} else {
		SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[authInfo], " HERE);
		ret = e_SC_RESULT_BADPARAM;
	}

	return (ret);
}

/**
 * @brief 交通情報を取得する
 * @param[in]  trafficSrch      交通情報検索条件
 * @param[out] trafficInfo      交通情報
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_RESULT SCC_GetTrafficInfo(const SMTRAFFICSRCH *trafficSrch, SMTRAFFICINFO *trafficInfo)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	UINT32	num = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == trafficSrch) {
			ret = e_SC_RESULT_BADPARAM;
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[trafficSrch], " HERE);
			break;
		}
		if ((0 == trafficSrch->parcelIdNum) || (CC_CMN_TRAFFICINFO_PARCEL_MAXNUM < trafficSrch->parcelIdNum)) {
			ret = e_SC_RESULT_BADPARAM;
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[trafficSrch->parcelIdNum], " HERE);
			break;
		}
		if (CC_CMN_TRAFFICINFO_ROAD_KIND_MAXNUM < trafficSrch->roadKindNum) {
			ret = e_SC_RESULT_BADPARAM;
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[trafficSrch->roadKindNum], " HERE);
			break;
		}
		for (num = 0; num < trafficSrch->roadKindNum; num++) {
			if (CC_CMN_TRAFFICINFO_ROAD_KIND_MAX < (INT32)trafficSrch->roadKind[num]) {
				ret = e_SC_RESULT_BADPARAM;
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[trafficSrch->roadKind[%d]], " HERE, num);
				break;
			}
		}
		if (e_SC_RESULT_SUCCESS != ret) {
			break;
		}
		if (NULL == trafficInfo) {
			ret = e_SC_RESULT_BADPARAM;
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[trafficInfo], " HERE);
			break;
		}

		// 交通情報取得
		ret = CC_GetTrafficInfo(trafficSrch, trafficInfo);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetTrafficInfo error, " HERE);
			break;
		}
	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}
