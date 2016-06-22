/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCORE_LOG_H
#define SMCORE_LOG_H

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

typedef enum _SC_LOG_TYPE {
	SC_LOG_TYPE_NONE = 0,			// ログ出力なし
	SC_LOG_TYPE_STD,				// 標準の出力のみ対象
	SC_LOG_TYPE_FILE,				// ファイルの出力のみ対象
	SC_LOG_TYPE_BOTH				// 両方の出力が対象
} SC_LOG_TYPE;

typedef enum _SC_LOG_LV {
	SC_LOG_LV_DEBUG = 0,			// デバッグ
	SC_LOG_LV_INFO,					// 情報
	SC_LOG_LV_WARN,					// 警告
	SC_LOG_LV_ERROR					// エラー
} SC_LOG_LV;

#endif // #ifndef SMCORE_LOG_H
