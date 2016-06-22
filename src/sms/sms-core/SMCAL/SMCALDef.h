/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCAL_DEF_H
#define SMCAL_DEF_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>

#ifndef	NULL
#define	NULL	(void*)0
#endif	// #ifndef	NULL

#ifndef	true
#define	true	1
#endif	// #ifndef	true

#ifndef	false
#define	false	0
#endif	// #ifndef	false

#define		SC_CAL_MAX_PATH				260										// パス最大長
#define		EOS							'\0'									// NULL終端文字

//-----------------------------------
// 型定義
//-----------------------------------
#ifndef	Char
#if 0	/* changed 2016.04.12 by T.Aikawa */
typedef signed char Char;
#else
typedef char Char;
#endif
#endif
#ifndef	UChar
typedef 	unsigned char			UChar;
#endif
#ifndef	Bool
typedef 	unsigned char			Bool;
#endif
#ifndef	INT8
typedef 	signed char				INT8;
#endif
#ifndef	INT16
typedef 	signed short			INT16;
#endif
#ifndef	INT32
typedef 	signed int				INT32;
#endif
#ifndef	INT64
typedef 	signed long long		INT64;
#endif
#ifndef	LONG
typedef 	signed long				LONG;
#endif
#ifndef	UINT8
typedef 	unsigned char			UINT8;
#endif
#ifndef	UINT16
typedef 	unsigned short			UINT16;
#endif
#ifndef	UINT32
typedef 	unsigned int			UINT32;
#endif
#ifndef	UINT64
typedef 	unsigned long long		UINT64;
#endif
#ifndef	ULONG
typedef 	unsigned long			ULONG;
#endif
#ifndef	FLOAT
typedef 	float					FLOAT;
#endif
#ifndef	DOUBLE
typedef 	double					DOUBLE;
#endif
#ifndef	BYTE
typedef 	unsigned char			BYTE;
#endif
typedef 	pthread_mutex_t			SC_CAL_MUTEX;
typedef		sem_t					SC_CAL_SEMAPHORE;
//typedef		INT32					SMSOCKET;
typedef 	Bool (*SC_CAL_CANCEL_FNC)(void);						// 中断確認用コールバック関数
typedef 	void (*SC_CAL_PROGRESS_FNC)(UINT32 num);				// 進捗通知用コールバック関数

typedef enum _SC_CAL_LOG_TYPE {
	SC_CAL_LOG_TYPE_NONE = 0,			// ログ出力なし
	SC_CAL_LOG_TYPE_STD,				// 標準の出力のみ対象
	SC_CAL_LOG_TYPE_FILE,				// ファイルの出力のみ対象
	SC_CAL_LOG_TYPE_BOTH				// 両方の出力が対象
} SC_CAL_LOG_TYPE;

typedef enum _SC_CAL_LOG_LV {
	SC_CAL_LOG_LV_DEBUG = 0,			// デバッグ
	SC_CAL_LOG_LV_INFO,					// 情報
	SC_CAL_LOG_LV_WARN,					// 警告
	SC_CAL_LOG_LV_ERROR					// エラー
} SC_CAL_LOG_LV;


typedef struct _SMCALLOG {
	SC_CAL_LOG_TYPE		logType;		// ログ出力先のタイプ
	SC_CAL_LOG_LV		logLvMin;		// 出力対象の最小ログレベル
	Char				logFile[SC_CAL_MAX_PATH];
#ifndef NDEBUG
	FILE				*logFp;			// ログファイル用ファイルポインタ
	SC_CAL_MUTEX		logMutexObj;
	SC_CAL_MUTEX		*logMutex;
#endif
	Bool				isInitialized;
	Char				reserve[3];
} SMCALLOG;

//-----------------------------------
// エラーコード定義
//-----------------------------------
typedef enum _E_SC_CAL_RESULT {
	e_SC_CAL_RESULT_SUCCESS			= 0x00000000,

	//---------------------------------------
	//通知系カテゴリ(0x0101)
	//---------------------------------------
	e_SC_CAL_RESULT_CAT_INFO		= 0x01010000,

	//---------------------------------------
	//警告系カテゴリ(0x0102)
	//---------------------------------------
	e_SC_CAL_RESULT_CAT_WARN		= 0x01020000,
	e_SC_CAL_RESULT_CANCEL,

	//---------------------------------------
	//異常系カテゴリ(0x0103)
	//---------------------------------------
	e_SC_CAL_RESULT_CAT_ERROR		= 0x01030000,

	e_SC_CAL_RESULT_FAIL,						// 内部エラー
	e_SC_CAL_RESULT_MALLOC_ERR,					// メモリ確保失敗
	e_SC_CAL_RESULT_BADPARAM,					// 引数エラー
	e_SC_CAL_RESULT_FILE_ACCESSERR,				// ファイルアクセスエラー
	e_SC_CAL_RESULT_TCP_CONNECT_ERROR,			// TCP回線接続エラー
	e_SC_CAL_RESULT_TCP_COMMUNICATION_ERR,		// TCP通信エラー
	e_SC_CAL_RESULT_TCP_TIMEOUT,				// TCP通信タイムアウト
	e_SC_CAL_RESULT_TCP_DISCONNECTED			// TCP回線が切断された
} E_SC_CAL_RESULT;

#endif // #ifndef SMCAL_DEF_H
