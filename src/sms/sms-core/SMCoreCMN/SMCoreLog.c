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

//-----------------------------------
// 変数定義
//-----------------------------------
#ifdef ANDROID
static SC_LOG_TYPE	logType;		// ログ出力先のタイプ
static SC_LOG_LV	logLvMin;		// 出力対象の最小ログレベル
#ifndef NDEBUG
static FILE			*logFp;			// ログファイル用ファイルポインタ
static SC_MUTEX		logMutexObj;
static SC_MUTEX		*logMutex;
#endif
static Bool			isInitialized;
#endif	// ANDROID

//-----------------------------------
// 関数定義
//-----------------------------------
static E_SC_RESULT SC_LOG_PrintMain(SC_LOG_LV lv, const Char *tag, const Char *fmt, va_list valist);
static E_SC_RESULT SC_LOG_MakeDir(const Char *apPath);

/**
 * @brief ログ初期化
 * @param[in] type      ログのタイプ
 * @param[in] lvMin     出力対象の最小ログレベル
 * @param[in] filePath  ログ用ファイルのフルパス
 * @return 処理結果(E_SC_RESULT)
 * @warning リリース時はtype=SC_LOG_TYPE_STD、lvMin=SC_LOG_LV_ERR固定とする。
 */
E_SC_RESULT SC_LOG_Initialize(SC_LOG_TYPE type, SC_LOG_LV lvMin, const Char *filePath)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

#ifdef ANDROID
	LOG_PRINT_START(SC_TAG_CORE);

#ifndef NDEBUG
	logType  = type;
	logLvMin = lvMin;
	logFp    = NULL;
	logMutex = NULL;

	do {
		if (SC_LOG_TYPE_FILE & logType) {
			if (NULL == filePath) {
				break;
			}

			if (e_SC_RESULT_SUCCESS != SC_LOG_MakeDir(filePath)) {
				break;
			}

			logFp = fopen((char*)filePath, (char*)"wt");
			if (NULL == logFp) {
				break;
			}

			// Mutex生成
			if (e_SC_RESULT_SUCCESS != SC_CreateMutex(&logMutexObj)) {
				LOG_PRINT_ERROR(SC_TAG_CORE, "SC_CreateMutext error");
				fclose(logFp);
				break;
			}
			logMutex = &logMutexObj;
		}
	} while (0);
#else
	logType  = SC_LOG_TYPE_STD;
	logLvMin = SC_LOG_LV_ERROR;
#endif

	isInitialized = true;

	LOG_PRINT_END(SC_TAG_CORE);
#endif	// ANDROID

	return (ret);
}

/**
 * @brief ログ終了化
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_LOG_Finalize()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

#ifdef ANDROID
	LOG_PRINT_START(SC_TAG_CORE);

#ifndef NDEBUG
	if (SC_LOG_TYPE_FILE & logType) {
		if (NULL != logFp) {
			// Mutexロック
			ret = SC_LockMutex(logMutex);
			if (e_SC_RESULT_SUCCESS != ret) {
				LOG_PRINT_ERROR(SC_TAG_CORE, "SC_LockMutext error");
			}

			fclose(logFp);
			logFp = NULL;

			// Mutexアンロック
			ret = SC_UnLockMutex(logMutex);
			if (e_SC_RESULT_SUCCESS != ret) {
				LOG_PRINT_ERROR(SC_TAG_CORE, "SC_UnLockMutex error");
			}

			// Mutex破棄
			ret = SC_DestroyMutex(logMutex);
			if (e_SC_RESULT_SUCCESS != ret) {
				LOG_PRINT_ERROR(SC_TAG_CORE, "SC_DestroyMutext error");
			}
		}
	}
	logMutex = NULL;
#endif

	isInitialized = false;

	LOG_PRINT_END(SC_TAG_CORE);
#endif	// ANDROID

	return (ret);
}

/**
 * @brief ログ出力
 * @param[in] lv  ログレベル
 * @param[in] tag タグ
 * @param[in] fmt 書式指定に従った文字列
 * @param[in] ... 可変個引数リスト
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_LOG_Print(SC_LOG_LV lv, const char *tag, const char *fmt, ...)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	va_list valist;

	va_start(valist, fmt);

	ret = SC_LOG_PrintMain(lv, tag, fmt, valist);

	va_end(valist);

	return (ret);
}

/**
 * @brief ログ出力(デバッグ)
 * @param[in] tag タグ
 * @param[in] fmt 書式指定に従った文字列
 * @param[in] ... 可変個引数リスト
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_LOG_DebugPrint(const char *tag, const char *fmt, ...)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	va_list valist;

	va_start(valist, fmt);

	ret = SC_LOG_PrintMain(SC_LOG_LV_DEBUG, tag, fmt, valist);

	va_end(valist);

	return (ret);
}

/**
 * @brief ログ出力(情報)
 * @param[in] tag タグ
 * @param[in] fmt 書式指定に従った文字列
 * @param[in] ... 可変個引数リスト
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_LOG_InfoPrint(const char *tag, const char *fmt, ...)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	va_list valist;

	va_start(valist, fmt);

	ret = SC_LOG_PrintMain(SC_LOG_LV_INFO, tag, fmt, valist);

	va_end(valist);

	return (ret);
}

/**
 * @brief ログ出力(警告)
 * @param[in] tag タグ
 * @param[in] fmt 書式指定に従った文字列
 * @param[in] ... 可変個引数リスト
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_LOG_WarnPrint(const char *tag, const char *fmt, ...)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	va_list valist;

	va_start(valist, fmt);

	ret = SC_LOG_PrintMain(SC_LOG_LV_WARN, tag, fmt, valist);

	va_end(valist);

	return (ret);
}

/**
 * @brief ログ出力(エラー)
 * @param[in] tag タグ
 * @param[in] fmt 書式指定に従った文字列
 * @param[in] ... 可変個引数リスト
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_LOG_ErrorPrint(const char *tag, const char *fmt, ...)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	va_list valist;

	va_start(valist, fmt);

	ret = SC_LOG_PrintMain(SC_LOG_LV_ERROR, tag, fmt, valist);

	va_end(valist);

	return (ret);
}

/**
 * @brief ログ出力(メイン)
 * @param[in] lv  ログレベル
 * @param[in] tag タグ
 * @param[in] fmt 書式指定に従った文字列
 * @param[in] valist 可変個引数リスト
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_LOG_PrintMain(SC_LOG_LV lv, const Char *tag, const Char *fmt, va_list valist)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

#ifdef ANDROID
	android_LogPriority	alp;

	do {
		if (true != isInitialized) {
			LOG_PRINT_ERROR(SC_TAG_CORE, "no initialized");
//			logType  = SC_LOG_TYPE_STD;
//			logLvMin = SC_LOG_LV_ERROR;
		}

		// 出力対象の最小ログレベル未満の場合は何も出さない
		if (logLvMin > lv) {
//			LOG_PRINT_ERROR(SC_TAG_CORE, "param error[logLvMin]");
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		if (NULL == tag) {
//			LOG_PRINT_ERROR(SC_TAG_CORE, "param error[tag]");
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

#ifndef NDEBUG
		if (SC_LOG_TYPE_FILE & logType) {
			if (NULL != logFp) {
				// Mutexロック
				if (e_SC_RESULT_SUCCESS != SC_LockMutex(logMutex)) {
					SC_LOG_ErrorPrint(SC_TAG_CORE, (Char*)"SC_LockMutext error, " HERE);
				} else {
					vfprintf(logFp, (char*)fmt, valist);

					// Mutexアンロック
					if (e_SC_RESULT_SUCCESS != SC_UnLockMutex(logMutex)) {
						SC_LOG_ErrorPrint(SC_TAG_CORE, (Char*)"SC_UnLockMutex error, " HERE);
					}
				}
			}
		}
#endif
		if (SC_LOG_TYPE_STD & logType) {
			switch (lv) {
			case SC_LOG_LV_DEBUG:
				alp = ANDROID_LOG_DEBUG;
				break;
			case SC_LOG_LV_INFO:
				alp = ANDROID_LOG_INFO;
				break;
			case SC_LOG_LV_WARN:
				alp = ANDROID_LOG_WARN;
				break;
			case SC_LOG_LV_ERROR:
				alp = ANDROID_LOG_ERROR;
				break;
			default:
				LOG_PRINT_ERROR(SC_TAG_CORE, "param error[logType]");
				ret = e_SC_RESULT_FAIL;
				break;
			}
			if (e_SC_RESULT_SUCCESS != ret) {
				break;
			}
			if (0 > __android_log_vprint(alp, (char*)tag, (char*)fmt, valist)) {
				LOG_PRINT_ERROR(SC_TAG_CORE, "log print error");
				ret = e_SC_RESULT_FAIL;
				break;
			}
		}
	} while (0);

	va_end(valist);
#endif	// ANDROID

	return (ret);
}

/**
 * @brief ディレクトリ作成
 * @param[in] dirPath 作成するディレクトリのフルパス（最大長はNULL終端文字含めてSC_MAX_PATH）
 * @return 処理結果(E_SC_RESULT)
 * @warning パスにフォルダのフルパスを指定する場合は、末尾に/を付加すること。
 *          パスは、ファイルのフルパスでも可。
 */
E_SC_RESULT SC_LOG_MakeDir(const Char *dirPath)
{
	Char			path[SC_MAX_PATH] = {};
	Char			*pPath = NULL;
	Char			*chr = NULL;
	INT32			cnt = 0;
	//UINT32			errCode = 0;
	struct stat		st = {};

	LOG_PRINT_START(SC_TAG_CORE);

	if (NULL == dirPath) {
		LOG_PRINT_ERROR(SC_TAG_CORE, "param error[dirPath]");
		return (e_SC_RESULT_BADPARAM);
	}

	// コピー先フォルダ作成
	strncpy((char*)path, (char*)dirPath, (sizeof(path) - 1));
	chr = (Char*)strrchr((char*)path, '/');
	if (NULL != chr) {
		if ((INT32)strlen((char*)path) > (INT32)(chr - path)) {
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
					LOG_PRINT_ERRORNO(SC_TAG_CORE, "mkdir error", errno);
					LOG_PRINT_ERROR(SC_TAG_CORE, path);
					return (e_SC_RESULT_FILE_ACCESSERR);
				}
			}
			*chr = '/';
		}
		pPath = chr + 1;
		cnt++;
	}

	LOG_PRINT_END(SC_TAG_CORE);

	return (e_SC_RESULT_SUCCESS);
}
