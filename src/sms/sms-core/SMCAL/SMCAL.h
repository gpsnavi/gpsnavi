/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCAL_H
#define SMCAL_H

#define	SC_CAL_SIGNATURE_SIZE				256

#define	OUTPUT_SEND_RECV_LOG				// ログファイル出力を有効にするマクロ
//#undef	OUTPUT_SEND_RECV_LOG				// ログファイル出力を無効にするマクロ

//-----------------------------------
// Content-Type
//-----------------------------------
typedef enum _E_CONTEXT_TYPE {
	E_TEXT_XML = 0,		// XML
	E_APP_BIN,			// バイナリ
	E_APP_TEXT,			// テキスト
	E_OTHER				// その他
} E_CONTEXT_TYPE;

//-----------------------------------
// HTTP種別
//-----------------------------------
typedef enum _E_HTTP_TYPE {
	E_HTTP = 0,			// HTTP
	E_HTTPS				// HTTPS
} E_HTTP_TYPE;

//-----------------------------------
// 構造体定義
//-----------------------------------
typedef struct _SMCALOPT {
	Bool					isBasicAuth;			// Basic認証
	Bool					isAws;					// 接続先がAWSか否か
	Bool					isResOutputFile;		// サーバからのレスポンスをファイル出力するか否か
	Bool					isResume;				// レジュームダウンロードをするか否か
	Bool					isResAcceptJson;		// レスポンスをJSON形式で受信するか否か
	Bool					reserve[3];
// Basic認証がtrueの場合、設定する --->
	struct {
		Char				basicAuthId[128 + 1];	// Basic認証ID
		Char				basicAuthPwd[128 + 1];	// Basic認証パスワード
		Char				reserve[2];
	} basic;
// Basic認証がtrueの場合、設定する <---
	INT32					timeout;				// タイムアウト時間(s) ※0以下が指定された場合、デフォルト値を使用する
	SC_CAL_CANCEL_FNC		cancel;					// 中断確認用コールバック関数ポインタ
	SC_CAL_PROGRESS_FNC		progress;				// 進捗通知用コールバック関数ポインタ
// サーバからのレスポンスをファイル出力するか否かがtrueの場合、設定する --->
	Char					resFilePath[SC_CAL_MAX_PATH];
// サーバからのレスポンスをファイル出力するか否かがtrueの場合、設定する <---
// レジュームダウンロードをするか否かがtrueの場合、設定する --->
	UINT32					resumeStratPos;
	UINT32					resumeEndPos;
// レジュームダウンロードをするか否かがtrueの場合、設定する --->
} SMCALOPT;

typedef struct _SMCAL_SSL {
	void		*ctx;
	void		*ssl;
	void		*bio;
} SMCAL_SSL;


typedef struct SMCAL {
#ifdef _WIN32
	SOCKET		sock;			// ソケット
#else
	INT32		sock;			// ソケット
#endif
	INT32		comTimeout;		// タイムアウト時間
	Char		*sendBuf;		// 送信バッファ
	INT32		sendLen;
	Char		*recvBuf;		// 受信バッファ
	INT32		recvLen;
	SMCALLOG	log;
#ifdef OUTPUT_SEND_RECV_LOG
	Char		workBuf[1024];	// ワークバッファ
	INT32		sendIdx;
	INT32		recvIdx;
#endif
	Bool		isHttps;		// httpsか否か
	Char		reserve[3];
	SMCAL_SSL	ssl;
} SMCAL;

typedef enum _E_SMCALPOSTPARAM_DATATYPE {
	SMCALPOSTPARAM_DATATYPE_FILE = 0,
	SMCALPOSTPARAM_DATATYPE_TEXT
} E_SMCALPOSTPARAM_DATATYPE;

typedef struct _SMCALPOSTPARAM {
	E_SMCALPOSTPARAM_DATATYPE	type;		// データタイプ
	Char						*name;		// データ名
	Char						*data;		// データ
	UINT32						len;		// データ長
	FILE						*fp;		// ファイルポインタ(SMCAL内で使用する)
} SMCALPOSTPARAM;
#endif // #ifndef SMCAL_H
