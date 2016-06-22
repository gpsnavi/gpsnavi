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

//-----------------------------------
// マクロ定義
//-----------------------------------
#ifdef ANDROID
#define LOG_PRINT_START(tag)															\
{																						\
	__android_log_print(ANDROID_LOG_DEBUG, tag,											\
						"[START] %s() \n", __FUNCTION__);								\
}
#define LOG_PRINT_END(tag)																\
{																						\
	__android_log_print(ANDROID_LOG_DEBUG, tag,											\
						"[END]   %s() \n", __FUNCTION__);								\
}
#define LOG_PRINT_ERROR(tag, log)														\
{																						\
	__android_log_print(ANDROID_LOG_ERROR, tag,											\
						"### ERROR ### %s(), %s, FILE : %s(%d) \n",						\
						__FUNCTION__, log, __FILE__, __LINE__ );	\
}
#define LOG_PRINT_ERRORNO(tag, log, errorno)											\
{																						\
	__android_log_print(ANDROID_LOG_ERROR, tag,											\
						"### ERROR ### %s(), %s(0x%08x), FILE : %s(%d) \n",				\
						__FUNCTION__, log, errorno, __FILE__, 							\
						__LINE__ );														\
}
#else
// ANDROID以外は何もしない
#define LOG_PRINT_START(tag)
#define LOG_PRINT_END(tag)
#define LOG_PRINT_ERROR(tag, log)
#define LOG_PRINT_ERRORNO(tag, log, errorno)
#endif	// ANDROID

//-----------------------------------
// 変数定義
//-----------------------------------

//-----------------------------------
// 関数定義
//-----------------------------------
static E_SC_CAL_RESULT SC_CAL_LOG_PrintMain(SMCALLOG *log, SC_CAL_LOG_LV lv, const Char *tag, const Char *fmt, va_list valist);


/**
 * @brief ログ初期化
 * @param[in] type      ログのタイプ
 * @param[in] lvMin     出力対象の最小ログレベル
 * @param[in] filePath  ログ用ファイルのフルパス
 * @return 処理結果(E_SC_CAL_RESULT)
 * @warning リリース時はtype=SC_CAL_LOG_TYPE_STD、lvMin=SC_CAL_LOG_LV_ERR固定とする。
 */
E_SC_CAL_RESULT SC_CAL_LOG_Initialize(SMCALLOG *log)
{
	E_SC_CAL_RESULT	ret = e_SC_CAL_RESULT_SUCCESS;

#ifdef ANDROID
	LOG_PRINT_START(SC_CAL_TAG);

#ifndef NDEBUG
	log->logFp    = NULL;
	log->logMutex = NULL;

	do {
		if (SC_CAL_LOG_TYPE_FILE & log->logType) {
			if (NULL == log->logFile) {
				break;
			}

			if (e_SC_CAL_RESULT_SUCCESS != SC_CAL_MakeDir(log->logFile)) {
				break;
			}

			log->logFp = fopen((char*)log->logFile, (char*)"wt");
			if (NULL == log->logFp) {
				break;
			}

			// Mutex生成
			if (e_SC_CAL_RESULT_SUCCESS != SC_CAL_CreateMutex(&log->logMutexObj)) {
				LOG_PRINT_ERROR(SC_CAL_TAG, "SC_CreateMutext error");
				fclose(log->logFp);
				break;
			}
			log->logMutex = &log->logMutexObj;
		}
	} while (0);
#else
	log->logType  = SC_CAL_LOG_TYPE_STD;
	log->logLvMin = SC_CAL_LOG_LV_ERROR;
#endif

	log->isInitialized = true;

	LOG_PRINT_END(SC_CAL_TAG);
#endif	// ANDROID

	return (ret);
}

/**
 * @brief ログ終了化
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_LOG_Finalize(SMCALLOG *log)
{
	E_SC_CAL_RESULT	ret = e_SC_CAL_RESULT_SUCCESS;

#ifdef ANDROID
	LOG_PRINT_START(SC_CAL_TAG);

#ifndef NDEBUG
	if (SC_CAL_LOG_TYPE_FILE & log->logType) {
		if (NULL != log->logFp) {
			// Mutexロック
			ret = SC_CAL_LockMutex(log->logMutex);
			if (e_SC_CAL_RESULT_SUCCESS != ret) {
				LOG_PRINT_ERROR(SC_CAL_TAG, "SC_CAL_LockMutext error");
			}

			fclose(log->logFp);
			log->logFp = NULL;

			// Mutexアンロック
			ret = SC_CAL_UnLockMutex(log->logMutex);
			if (e_SC_CAL_RESULT_SUCCESS != ret) {
				LOG_PRINT_ERROR(SC_CAL_TAG, "SC_CAL_UnLockMutex error");
			}

			// Mutex破棄
			ret = SC_CAL_DestroyMutex(log->logMutex);
			if (e_SC_CAL_RESULT_SUCCESS != ret) {
				LOG_PRINT_ERROR(SC_CAL_TAG, "SC_CAL_DestroyMutext error");
			}
		}
	}
	log->logMutex = NULL;
#endif

	log->isInitialized = false;

	LOG_PRINT_END(SC_CAL_TAG);
#endif	// ANDROID

	return (ret);
}

/**
 * @brief ログ出力
 * @param[in] lv  ログレベル
 * @param[in] tag タグ
 * @param[in] fmt 書式指定に従った文字列
 * @param[in] ... 可変個引数リスト
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_LOG_Print(SMCALLOG *log, SC_CAL_LOG_LV lv, const Char *tag, const Char *fmt, ...)
{
	E_SC_CAL_RESULT	ret = e_SC_CAL_RESULT_SUCCESS;
	va_list valist;

	va_start(valist, fmt);

	ret = SC_CAL_LOG_PrintMain(log, lv, tag, fmt, valist);

	va_end(valist);

	return (ret);
}

/**
 * @brief ログ出力(デバッグ)
 * @param[in] tag タグ
 * @param[in] fmt 書式指定に従った文字列
 * @param[in] ... 可変個引数リスト
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_LOG_DebugPrint(SMCALLOG *log, const Char *tag, const Char *fmt, ...)
{
	E_SC_CAL_RESULT	ret = e_SC_CAL_RESULT_SUCCESS;
	va_list valist;

	va_start(valist, fmt);

	ret = SC_CAL_LOG_PrintMain(log, SC_CAL_LOG_LV_DEBUG, tag, fmt, valist);

	va_end(valist);

	return (ret);
}

/**
 * @brief ログ出力(情報)
 * @param[in] tag タグ
 * @param[in] fmt 書式指定に従った文字列
 * @param[in] ... 可変個引数リスト
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_LOG_InfoPrint(SMCALLOG *log, const Char *tag, const Char *fmt, ...)
{
	E_SC_CAL_RESULT	ret = e_SC_CAL_RESULT_SUCCESS;
	va_list valist;

	va_start(valist, fmt);

	ret = SC_CAL_LOG_PrintMain(log, SC_CAL_LOG_LV_INFO, tag, fmt, valist);

	va_end(valist);

	return (ret);
}

/**
 * @brief ログ出力(警告)
 * @param[in] tag タグ
 * @param[in] fmt 書式指定に従った文字列
 * @param[in] ... 可変個引数リスト
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_LOG_WarnPrint(SMCALLOG *log, const Char *tag, const Char *fmt, ...)
{
	E_SC_CAL_RESULT	ret = e_SC_CAL_RESULT_SUCCESS;
	va_list valist;

	va_start(valist, fmt);

	ret = SC_CAL_LOG_PrintMain(log, SC_CAL_LOG_LV_WARN, tag, fmt, valist);

	va_end(valist);

	return (ret);
}

/**
 * @brief ログ出力(エラー)
 * @param[in] tag タグ
 * @param[in] fmt 書式指定に従った文字列
 * @param[in] ... 可変個引数リスト
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_LOG_ErrorPrint(SMCALLOG *log, const Char *tag, const Char *fmt, ...)
{
	E_SC_CAL_RESULT	ret = e_SC_CAL_RESULT_SUCCESS;
	va_list valist;

	va_start(valist, fmt);

	ret = SC_CAL_LOG_PrintMain(log, SC_CAL_LOG_LV_ERROR, tag, fmt, valist);

	va_end(valist);

	return (ret);
}

/**
 * @brief ログ出力(メイン)
 * @param[in] lv  ログレベル
 * @param[in] tag タグ
 * @param[in] fmt 書式指定に従った文字列
 * @param[in] valist 可変個引数リスト
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_LOG_PrintMain(SMCALLOG *log, SC_CAL_LOG_LV lv, const Char *tag, const Char *fmt, va_list valist)
{
	E_SC_CAL_RESULT	ret = e_SC_CAL_RESULT_SUCCESS;

#ifdef ANDROID
	android_LogPriority	alp;

	do {
		if (true != log->isInitialized) {
#ifdef DEBUG
			LOG_PRINT_ERROR(SC_CAL_TAG, "no initialized");
#endif
			log->logType  = SC_CAL_LOG_TYPE_STD;
			log->logLvMin = SC_CAL_LOG_LV_ERROR;
		}

		// 出力対象の最小ログレベル未満の場合は何も出さない
		if (log->logLvMin > lv) {
			//LOG_PRINT_ERROR(SC_CAL_TAG, "param error[logLvMi]");
			ret = e_SC_CAL_RESULT_BADPARAM;
			break;
		}

		if (NULL == tag) {
			//LOG_PRINT_ERROR(SC_CAL_TAG, "param error[tag]");
			ret = e_SC_CAL_RESULT_BADPARAM;
			break;
		}

#ifndef NDEBUG
		if (SC_CAL_LOG_TYPE_FILE & log->logType) {
			if (NULL != log->logFp) {
				// Mutexロック
				if (e_SC_CAL_RESULT_SUCCESS != SC_CAL_LockMutex(log->logMutex)) {
					SC_CAL_LOG_ErrorPrint(log, SC_CAL_TAG, (Char*)"SC_CAL_LockMutext error, " HERE);
				} else {
					vfprintf(log->logFp, (char*)fmt, valist);

					// Mutexアンロック
					if (e_SC_CAL_RESULT_SUCCESS != SC_CAL_UnLockMutex(log->logMutex)) {
						SC_CAL_LOG_ErrorPrint(log, SC_CAL_TAG, (Char*)"SC_CAL_UnLockMutex error, " HERE);
					}
				}
			}
		}
#endif

#ifdef _WIN32
		if (0 > vprintf((char*)fmt, valist)) {
			LOG_PRINT_ERROR(SC_CAL_TAG, "log print error");
			ret = e_SC_CAL_RESULT_FAIL;
			break;
		}
#else
		if (SC_CAL_LOG_TYPE_STD & log->logType) {
			switch (lv) {
			case SC_CAL_LOG_LV_DEBUG:
				alp = ANDROID_LOG_DEBUG;
				break;
			case SC_CAL_LOG_LV_INFO:
				alp = ANDROID_LOG_INFO;
				break;
			case SC_CAL_LOG_LV_WARN:
				alp = ANDROID_LOG_WARN;
				break;
			case SC_CAL_LOG_LV_ERROR:
				alp = ANDROID_LOG_ERROR;
				break;
			default:
				LOG_PRINT_ERROR(SC_CAL_TAG, "param error[logType]");
				ret = e_SC_CAL_RESULT_FAIL;
				break;
			}
			if (e_SC_CAL_RESULT_SUCCESS != ret) {
				break;
			}
			if (0 > __android_log_vprint(alp, (char*)tag, (char*)fmt, valist)) {
				LOG_PRINT_ERROR(SC_CAL_TAG, "log print error");
				ret = e_SC_CAL_RESULT_FAIL;
				break;
			}
		}
#endif
	} while (0);

	va_end(valist);
#endif	// ANDROID

	return (ret);
}
