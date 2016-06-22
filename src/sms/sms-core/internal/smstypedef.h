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
 * smstypedef.h
 *
 *  Created on: 2015/11/04
 *      Author: masutani
 *
 *  型定義及びエラーコードの定義を行います。
 *  外部ヘッダを参照するような定義は基本NGです。
 */

#ifndef SMSTYPEDEF_H_
#define SMSTYPEDEF_H_

/* IOS */
#ifdef __APPLE__
#define __SMS_APPLE__
#endif

#ifndef NULL
#define NULL (void*)0
#endif
#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif
#ifndef Char
#if 0	/* changed 2016.04.12 by T.Aikawa */
typedef signed char Char;
#else
typedef char Char;
#endif
#endif
#ifndef UChar
typedef unsigned char UChar;
#endif
#ifndef Bool
typedef unsigned char Bool;
#endif
#ifndef INT8
typedef signed char INT8;
#endif
#ifndef INT16
typedef signed short INT16;
#endif
#ifndef INT32
typedef signed int INT32;
#endif
#ifndef INT64
typedef signed long long INT64;
#endif
#ifndef LONG
typedef signed long LONG;
#endif
#ifndef UINT8
typedef unsigned char UINT8;
#endif
#ifndef UINT16
typedef unsigned short UINT16;
#endif
#ifndef UINT32
typedef unsigned int UINT32;
#endif
#ifndef UINT64
typedef unsigned long long UINT64;
#endif
#ifndef ULONG
typedef unsigned long ULONG;
#endif
#ifndef FLOAT
typedef float FLOAT;
#endif
#ifndef DOUBLE
typedef double DOUBLE;
#endif
#ifndef BYTE
typedef unsigned char BYTE;
#endif
#ifndef SIZE
typedef size_t SIZE;
#endif
#ifdef _WIN32
typedef HANDLE SC_MUTEX;
typedef HANDLE SC_SEMAPHORE;
#else
typedef pthread_mutex_t SC_MUTEX;
typedef sem_t SC_SEMAPHORE;
#endif
#ifndef DBOBJECT
typedef sqlite3 DBOBJECT;
#endif

/**
 * 中断確認用コールバック関数
 */
typedef Bool (*CANCEL_FNC)(void);
/**
 * 進捗通知用コールバック関数
 */
typedef void (*PROGRESS_FNC)(INT32 num, INT32 allNum);

/**
 * エラーコード定義
 */
// SMCore
typedef enum _E_SC_RESULT {
	e_SC_RESULT_SUCCESS = 0x00000000,

	//---------------------------------------
	//通知系カテゴリ(0x0101)
	//---------------------------------------
	e_SC_RESULT_CAT_INFO = 0x01010000,

	e_SC_RESULT_NODATA,						// マップデータ レコード無しの通知
	e_SC_RESULT_NOTABLE,
	e_SC_RESULT_NOENTRY,
	e_SC_RESULT_DATALIMIT,

	//---------------------------------------
	//警告系カテゴリ(0x0102)
	//---------------------------------------
	e_SC_RESULT_CAT_WARN = 0x01020000,
	e_SC_RESULT_CANCEL,

	//---------------------------------------
	//異常系カテゴリ(0x0103)
	//---------------------------------------
	e_SC_RESULT_CAT_ERROR = 0x01030000,

	e_SC_RESULT_FAIL,						// 内部エラー
	e_SC_RESULT_MALLOC_ERR,					// メモリ確保失敗
	e_SC_RESULT_BADPARAM,					// 引数エラー
	e_SC_RESULT_RDB_ACCESSERR,				// RDBアクセスエラー
	e_SC_RESULT_FILE_OPENERR,				// ファイルオープンエラー
	e_SC_RESULT_FILE_ACCESSERR,				// ファイルアクセスエラー
	e_SC_RESULT_MAP_GETERR,					// 地図データ取得エラー
	e_SC_RESULT_ROUTE_ERR,					// 探索エラー
	e_SC_RESULT_ROUTE_CANCEL,				// 探索キャンセル
	e_SC_RESULT_AROUND_NO_LINK,				// 探索近傍リンク未発見
	e_SC_RESULT_TCP_CONNECT_ERROR,			// TCP回線接続エラー
	e_SC_RESULT_TCP_COMMUNICATION_ERR,		// TCP通信エラー
	e_SC_RESULT_TCP_TIMEOUT,				// TCP通信タイムアウト
	e_SC_RESULT_TCP_DISCONNECTED,			// TCP回線が切断された
	e_SC_RESULT_DISKSIZE_ERROR,				// ディスクサイズ容量不足
	e_SC_RESULT_NEDD_APP_UPDATE,			// アプリの更新が必要
	e_SC_RESULT_SERVER_STOP,				// サーバ停止
	e_SC_RESULT_MAP_DATA_ERR,				// 地図更新データエラー
	e_SC_RESULT_MAP_UPDATE_ERR,				// 地図更新エラー
	e_SC_RESULT_SMS_API_ERR,				// センタAPIエラー
	e_SC_RESULT_NOT_FOUND_MAP,				// 地図データが見つからない
	e_SC_RESULT_POLICY_AGREE,				// 最新の規約の同意が必要
	e_SC_RESULT_NONACTIVATION,				// アクティベーションが必要
	e_SC_RESULT_URL_REDIRECT,				// URLリダイレクト
	e_SC_RESULT_OAUTH_TOKEN_ERROR,			// OAuth連携トークンエラー
	e_SC_RESULT_UNAUTHORIZED_API,			// 使用許可していないポータルAPIが指定された
	e_SC_RESULT_DATA_ERR,					// データ不正
	e_SC_RESULT_NEDD_PKG_UPDATE				// パッケージの更新が必要
} E_SC_RESULT;
// DAL
typedef enum {
	SC_DA_RES_SUCCESS = 0x00000000,

	//---------------------------------------
	//通知系カテゴリ(0x0101)
	//---------------------------------------
	SC_DA_RES_CAT_INFO = 0x01010000,

	SC_DA_RES_NODATA,						// マップデータ レコード無しの通知
	SC_DA_RES_NOTABLE,
	SC_DA_RES_NOENTRY,
	SC_DA_RES_MAP_NEW,

	//---------------------------------------
	//警告系カテゴリ(0x0102)
	//---------------------------------------
	SC_DA_RES_CAT_WARN = 0x01020000,

	SC_DA_RES_CANCEL,
	SC_DA_RES_MAP_OLD,

	//---------------------------------------
	//異常系カテゴリ(0x0103)
	//---------------------------------------
	SC_DA_RES_CAT_ERROR = 0x01030000,

	SC_DA_RES_FAIL,						// 内部エラー
	SC_DA_RES_NOMEMORY,					// メモリ不足
	SC_DA_RES_MALLOC_ERR,				// メモリ確保失敗
	SC_DA_RES_BADPARAM,					// 引数エラー
	SC_DA_RES_RDB_ACCESSERR,			// RDBアクセスエラー
	SC_DA_RES_MAP_FORMATERR,			// マップフォーマットエラー
	SC_DA_RES_FILE_ACCESSERR,			// ファイルアクセスエラー
	SC_DA_RES_COMMUNICATION_ERR,		// 通信エラー
	SC_DA_RES_COMMUNICATION_TIMEOUT,	// 通信タイムアウト
	SC_DA_RES_UPDDATA_LACK,				// 更新データファイル不足
	SC_DA_RES_UPDDATA_ERR,				// 更新データファイル不正
	SC_DA_RES_UPDDATA_INVALID,			// 更新不可データ
	SC_DA_RES_LICENCE_ERR,				// ライセンスキーまたはライセンスファイル不正
	SC_DA_RES_UPD_DISKSIZE_LACK,		// 更新先ディスク空き容量不足
	SC_DA_RES_UPDDATA_DIFFERENT,		// 更新中断前と更新データが異なる
	SC_DA_RES_DBUPD_ERR,				// DB更新エラー
	SC_DA_RES_FILEUPD_ERR,				// ファイル更新エラー
	SC_DA_RES_VERUPD_ERR,				// バージョン更新エラー
	SC_DA_RES_UPD_SEQUENCE_ERR			// API呼び出しシーケンスエラー
} SC_DA_RESULT;
// PAL
typedef enum _E_PAL_RESULT {
	e_PAL_RESULT_SUCCESS = 0x00000000,

	//---------------------------------------
	//異常系カテゴリ(0x0103)
	//---------------------------------------
	e_PAL_RESULT_CAT_ERROR = 0x01030000,

	//e_PAL_RESULT_FAIL,				// 内部エラー
	//e_PAL_RESULT_MALLOC_ERR,			// メモリ確保失敗
	e_PAL_RESULT_PARAM_ERR,				// 引数エラー
	e_PAL_RESULT_UPDATE_ERR,			// RDB更新エラー
	e_PAL_RESULT_DELETE_ERR,			// RDB削除エラー
	e_PAL_RESULT_SELECT_ERR,			// RDB検索エラー
	e_PAL_RESULT_NO_DATA_ERR,			// 該当データなし
	e_PAL_RESULT_ADD_OVER_ERR,			// 収納数・上限値オーバー
	e_PAL_RESULT_DUPLICATE_ERR,			// キー重複エラー
	e_PAL_RESULT_DUPLICATE_DATA_ERR,	// データ重複エラー
	e_PAL_RESULT_ACCESS_ERR,			// ファイルアクセスエラー

} E_PAL_RESULT;
// SAL
typedef enum {
	SC_SA_RES_SUCCESS = 0x00000000,

	//---------------------------------------
	//通知系カテゴリ(0x0101)
	//---------------------------------------
	SC_SA_RES_CAT_INFO = 0x01010000,

	SC_SA_RES_NODATA,						// マップデータ レコード無しの通知
	SC_SA_RES_NOTABLE,
	SC_SA_RES_NOENTRY,
	SC_SA_RES_MAP_NEW,

	//---------------------------------------
	//警告系カテゴリ(0x0102)
	//---------------------------------------
	SC_SA_RES_CAT_WARN = 0x01020000,
	SC_SA_RES_CANCEL,
	SC_SA_RES_MAP_OLD,

	//---------------------------------------
	//異常系カテゴリ(0x0103)
	//---------------------------------------
	SC_SA_RES_CAT_ERROR = 0x01030000,

	SC_SA_RES_FAIL,					// 内部エラー
	SC_SA_RES_NOMEMORY,				// メモリ不足
	SC_SA_RES_MALLOC_ERR,			// メモリ確保失敗
	SC_SA_RES_BADPARAM,				// 引数エラー
	SC_SA_RES_RDB_ACCESSERR,			// RDBアクセスエラー
	SC_SA_RES_MAP_FORMATERR,			// マップフォーマットエラー
	SC_SA_RES_FILE_ACCESSERR,		// ファイルアクセスエラー
	SC_SA_RES_COMMUNICATION_ERR,		// 通信エラー
	SC_SA_RES_COMMUNICATION_TIMEOUT,	// 通信タイムアウト
	SC_SA_RES_UPDDATA_LACK,			// 更新データファイル不足
	SC_SA_RES_UPDDATA_ERR,			// 更新データファイル不正
	SC_SA_RES_UPDDATA_INVALID,		// 更新不可データ
	SC_SA_RES_LICENCE_ERR,			// ライセンスキーまたはライセンスファイル不正
	SC_SA_RES_UPD_DISKSIZE_LACK,		// 更新先ディスク空き容量不足
	SC_SA_RES_UPDDATA_DIFFERENT,		// 更新中断前と更新データが異なる
	SC_SA_RES_DBUPD_ERR,				// DB更新エラー
	SC_SA_RES_FILEUPD_ERR,			// ファイル更新エラー
	SC_SA_RES_VERUPD_ERR,			// バージョン更新エラー
	SC_SA_RES_UPD_SEQUENCE_ERR		// API呼び出しシーケンスエラー

} SC_SA_RESULT;
// DHC
typedef enum _DHC_CASH_RESULT {
	//---------------------------------------
	//キャッシュ応答用カテゴリ(0x0104)
	//---------------------------------------
	e_DHC_RESULT_CASH_SUCCESS = 0x01040000,
	e_DHC_RESULT_CASH_READVOLOVER,		// 読み込み枚数超え
	e_DHC_RESULT_CASH_READSIZE,			// 読み込みサイズ超え
	e_DHC_RESULT_CASH_CASHVOLOVER,		// 読み込み枚数超え
	e_DHC_RESULT_CASH_CASHSIZE,			// 読み込みサイズ超え
	e_DHC_RESULT_CASH_CASHSIZEMISSMATCH,			// キャッシュ削除時サイズ不整合
	e_DHC_RESULT_CASH_UNKNOWN_KIND,		// 不明地図種別
	e_DHC_RESULT_CASH_UNMATCH,			// データ不整合
	e_DHC_RESULT_CASH_UNKNOWN_PCL,		// 対象のパーセルが見つからない
	e_DHC_RESULT_CASH_FAIL				// 内部エラー
} E_DHC_CASH_RESULT;

#endif /* SMSTYPEDEF_H_ */
