/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_LOG_H
#define SMCCOM_LOG_H

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

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _SCC_LOG_TYPE {
	SCC_LOG_TYPE_NONE = 0,			// ログ出力なし
	SCC_LOG_TYPE_STD,				// 標準の出力のみ対象
	SCC_LOG_TYPE_FILE,				// ファイルの出力のみ対象
	SCC_LOG_TYPE_BOTH				// 両方の出力が対象
} SCC_LOG_TYPE;

typedef enum _SCC_LOG_LV {
	SCC_LOG_LV_DEBUG = 0,			// デバッグ
	SCC_LOG_LV_INFO,					// 情報
	SCC_LOG_LV_WARN,					// 警告
	SCC_LOG_LV_ERROR					// エラー
} SCC_LOG_LV;

E_SC_RESULT SCC_LOG_Initialize(SCC_LOG_TYPE type, SCC_LOG_LV lvMin, const Char *filePath);
E_SC_RESULT SCC_LOG_Print(SCC_LOG_LV lv, const Char *tag, const Char *fmt, ...);
E_SC_RESULT SCC_LOG_DebugPrint(const Char *tag, const Char *fmt, ...);
E_SC_RESULT SCC_LOG_InfoPrint(const Char *tag, const Char *fmt, ...);
E_SC_RESULT SCC_LOG_WarnPrint(const Char *tag, const Char *fmt, ...);
E_SC_RESULT SCC_LOG_ErrorPrint(const Char *tag, const Char *fmt, ...);
E_SC_RESULT SCC_LOG_Finalize();

#ifdef __cplusplus
}
#endif

#endif // #ifndef SMCCOM_LOG_H
