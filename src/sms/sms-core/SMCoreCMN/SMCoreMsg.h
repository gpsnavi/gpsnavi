/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCOREMSG_H_
#define SMCOREMSG_H_

//-----------------------------------
// 共用体定義
//-----------------------------------
// メッセージ種別ID定義
typedef enum _E_SC_MSG_ID {
	e_SC_MSGID_START = 0,

	/***** 要求 *****/
	e_SC_MSGID_REQ_MAP_DATA,				// 地図データ取得要求
	e_SC_MSGID_REQ_MAP_CASH_FREE,			// 地図データキャッシュ解放要求
	e_SC_MSGID_REQ_MAP_CANCEL,				// 地図データ要求キャンセル
	e_SC_MSGID_REQ_REFRESH_MAP,				// 地図描画要求(FM->MP)
	e_SC_MSGID_REQ_RM_INIT,					// 探索初期化(FM->RM)
	e_SC_MSGID_REQ_RC_INIT,					// 探索初期化(RM->RC)
	e_SC_MSGID_REQ_RM_RTSINGLE,				// 単経路探索要求(FM->RM)
	e_SC_MSGID_REQ_RM_RTMULTI,				// 複数経路探索要求(FM->RM)
	e_SC_MSGID_REQ_RM_REROUTE,				// 再探索要求(FM->RM)
	e_SC_MSGID_REQ_RM_CANCEL,				// 探索キャンセル要求(FM->RM)
	e_SC_MSGID_REQ_RC_RTSINGLE,				// 単経路探索要求(RM->RC)
	e_SC_MSGID_REQ_RC_RTMULTI,				// 複数経路探索要求(RM->RC)
	e_SC_MSGID_REQ_RC_REROUTE,				// 再探索要求(RM->RC)
	e_SC_MSGID_REQ_VIEW_ROUTE,				// 経路描画要求
	e_SC_MSGID_REQ_RG_GUIDESTART,			// 経路誘導開始要求(FM->RG)
	e_SC_MSGID_REQ_RG_GUIDERUN,				// 経路誘導実行要求(FM->RG)
	e_SC_MSGID_REQ_RG_GUIDESTOP,			// 経路誘導終了要求(FM->RG)
	e_SC_MSGID_REQ_RG_SIMSTART,				// シュミレーション開始要求(FM->RG)
	e_SC_MSGID_REQ_RG_SIMRUN,				// シュミレーション実行要求(FM->RG)
	e_SC_MSGID_REQ_RG_SIMEXIT,				// シュミレーション終了要求(FM->RG)
	e_SC_MSGID_REQ_PU_UPLOAD,				// Probeデータアップロード(PB->PU)
	e_SC_MSGID_REQ_PT_START,				// タイマ開始(PB->PT)
	e_SC_MSGID_REQ_RG_TURNLIST,				// ターンリスト作成要求(FM->RG)
	e_SC_MSGID_REQ_RT_GUIDEMAKE,			// 経路誘導情報作成要求(RG->RT)
	e_SC_MSGID_REQ_RT_GUIDEFREE,			// 経路誘導情報解放要求(RG->RT)
	e_SC_MSGID_REQ_RT_LISTMAKE,				// ターンリスト作成要求(RG->RT)
	e_SC_MSGID_REQ_RT_GUIDEADD,				// 経路誘導情報追加要求(RG->RT)
	e_SC_MSGID_REQ_SDM_START,				// 運転特性診断開始要求(FM->SDM)
	e_SC_MSGID_REQ_SDD_START,				// 運転特性診断開始要求(SDM->SDD)
	e_SC_MSGID_REQ_SDT_START,				// タイマ開始要求(SDM->SDT)
	e_SC_MSGID_REQ_SDU_UPLOAD,				// アップロード要求(SDM->SDU)
	e_SC_MSGID_REQ_SDM_STOP,				// 運転特性診断停止要求(FM->SDM)
	e_SC_MSGID_REQ_SDD_STOP,				// 運転特性診断停止要求(SDM->SDD)
	e_SC_MSGID_REQ_SDU_STOP,				// アップロード終了要求(SDM->SDU)
	e_SC_MSGID_REQ_TR_UPDATE,				// 交通情報更新要求(FM->TR)
	e_SC_MSGID_REQ_TRT_START,				// タイマ開始要求(TR->TRT)

	/***** 応答 *****/
	e_SC_MSGID_RES_MAP_DATA,				// 地図データ取得応答
	e_SC_MSGID_RES_MAP_CASH_FREE,			// 地図データキャッシュ開放応答
	e_SC_MSGID_RES_FM_RTSINGLE,				// 単経路探索応答(RM->FM)
	e_SC_MSGID_RES_FM_RTMULTI,				// 複数経路探索応答(RM->FM)
	e_SC_MSGID_RES_FM_REROUTE,				// 再探索応答(RM->FM)
	e_SC_MSGID_RES_RC_RTSINGLE,				// 単経路探索応答(RC->RM)
	e_SC_MSGID_RES_RC_RTMULTI,				// 複数経路探索応答(RC->RM)
	e_SC_MSGID_RES_RC_REROUTE,				// 再探索応答(RC->RM)
	e_SC_MSGID_RES_RG_GUIDESTART,			// 経路誘導開始応答(RG->FM)
	e_SC_MSGID_RES_RG_GUIDERUN,				// 経路誘導実行応答(RG->FM)
	e_SC_MSGID_RES_RG_GUIDESTOP,			// 経路誘導終了応答(RG->FM)
	e_SC_MSGID_RES_RG_SIMSTART,				// シュミレーション開始応答(RG->FM)
	e_SC_MSGID_RES_RG_SIMRUN,				// シュミレーション実行応答(RG->FM)
	e_SC_MSGID_RES_RG_SIMEXIT,				// シュミレーション終了応答(RG->FM)
	e_SC_MSGID_RES_PU_UPLOAD_FINISH,		// アップロード終了応答
	e_SC_MSGID_RES_RG_TURNLIST,				// ターンリスト作成要求(RG->FM)
	e_SC_MSGID_RES_RT_GUIDEMAKE,			// 経路誘導情報作成応答(RT->RG)
	e_SC_MSGID_RES_RT_GUIDEFREE,			// 経路誘導情報解放応答(RT->RG)
	e_SC_MSGID_RES_RT_LISTMAKE,				// ターンリスト作成応答(RG->RT)
	e_SC_MSGID_RES_SDM_START,				// 運転特性診断開始応答(SDM->FM)
	e_SC_MSGID_RES_SDD_START,				// 運転特性診断開始応答(SDD->SDM)
	e_SC_MSGID_RES_SDT_TIMERSTART,			// タイマ開始応答(SDT->SDM)
	e_SC_MSGID_RES_SDU_UPLOAD,				// アップロード完了応答(SDU->SDM)
	e_SC_MSGID_RES_SDM_STOP,				// 運転特性診断停止応答(SDM->FM)
	e_SC_MSGID_RES_SDD_STOP,				// 運転特性診断停止応答(SDD->SDM)
	e_SC_MSGID_RES_SDU_STOP,				// アップロード終了応答(SDU->SDM)

	/***** イベント／通知 *****/
	e_SC_MSGID_EVT_FINISH,					// 終了通知
	e_SC_MSGID_EVT_CAR_STOP,				// 自車位置停止
	e_SC_MSGID_EVT_DEST_MOVE,				// 目的地変更
	e_SC_MSGID_EVT_ROUTESETTING_CHANGE,		// 探索条件変更
	e_SC_MSGID_EVT_ROUTE_DISTRIBUTE,		// 経路公開
	e_SC_MSGID_EVT_TIMEOUT,					// タイムアウト通知
	e_SC_MSGID_EVT_SENSORDATA,				// センサデータ通知

	e_SC_MSGID_END
} E_SC_MSG_ID;

// 地図データ種別ID定義
typedef enum _E_SC_MAP_DATA_ID {
	e_SC_MAP_DATA_START = 0,

	e_SC_MAP_PARCEL_ROAD,					// パーセル道路データ
	e_SC_MAP_PARCEL_BKGD,					// パーセル背景データ
	e_SC_MAP_SHAPE,							// 形状データ
	e_SC_MAP_NAME,							// 名称データ
	e_SC_MAP_ROAD_NAME,						// 道路名称データ
	e_SC_MAP_BKGD_NAME,						// 背景名称データ
	e_SC_MAP_GUIDE,							// 誘導データ
	e_SC_MAP_CHARSTR,						// 文言データ

	e_SC_MAP_DATA_END
} E_SC_MAP_DATA_ID;

// ユーザID
typedef enum _E_SC_USER_ID {
	e_SC_USER_MP = 0,						// 地図描画
	e_SC_USER_RP,							// 経路探索
	e_SC_USER_PI,							// 施設検索
	e_SC_USER_DM,							// 経路誘導
	e_SC_USER_LC,							// ロケータ

	e_SC_DH_USER_ID_END
} E_SC_USER_ID;

// 地図要求
typedef enum _E_SC_DM_MAPREQCODE{
	e_SC_DM_RESTYPE_ALL = 0,				// 全件応答
	e_SC_DM_RESTYPE_ONE,					// 初回応答
	e_SC_DM_RESTYPE_CANCEL,					// キャンセル応答
	e_SC_DM_RESTYPE_ERROR,					// エラー応答
	e_SC_DM_RESTYPE_FREE,					// 地図開放応答
	e_SC_DM_RESTYPE_SEM,					// セマフォ解除応答
	e_SC_DM_FREETYPE_USER,					// ユーザ地図開放
	e_SC_DM_FREETYPE_KIND,					// 種別地図開放
	e_SC_DM_FREETYPE_DITAIL,				// 地図指定開放

	e_SC_DM_MAPREQCODE_END,					// 終端
} E_SC_DM_MAPREQCODE;

// 位置情報種別
typedef enum _E_SC_POSINFO_KIND {
	e_SC_POSINFO_KIND_DST = 0,				// 目的地位置
	e_SC_POSINFO_KIND_DST_OWN,				// 目的地位置＋発信者位置

	e_SC_POSKIND_END,						// 終端
} E_SC_POSINFO_KIND;

// 交通情報
typedef enum _E_SC_TR_EVENT {
	e_SC_TR_EVENT_CARPOS = 0,				// 自車位置更新
	e_SC_TR_EVENT_SCROLL,					// スクロール位置更新
	e_SC_TR_EVENT_MANUAL,					// 手動更新
	e_SC_TR_EVENT_TIMER,					// タイマー更新

	e_SC_TR_EVENT_END,						// 終端
} E_SC_TR_EVENT;

//-----------------------------------
// 構造体定義
//-----------------------------------
// メッセージ処理関数のポインタ
typedef void (*SC_MsgFunc)(pthread_msq_msg_t *msg);
// メッセージテーブル構造体
typedef struct _SC_MSG_DATAINFO {
	E_SC_MSG_ID			msgID;				// メッセージID
	pthread_msq_id_t	*msq;				// 送信先メッセージキュー
	SC_MsgFunc			Func;				// メッセージ受信後のメッセージ処理関数
} SC_MSG_DATAINFO;

//-----------------------------------
// マクロ定義
//-----------------------------------
// メッセージ種別IDチェック
#define	SC_MSG_CHECK_MSG_ID(msg)		((e_SC_MSGID_START < ((pthread_msq_msg_t*)msg)->data[SC_MSG_MSG_ID]) && \
										 (e_SC_MSGID_END   > ((pthread_msq_msg_t*)msg)->data[SC_MSG_MSG_ID]))
// 地図データ種別IDチェック
#define	SC_MSG_CHECK_MAP_DATA_ID(msg)	((e_SC_MAP_DATA_START < ((pthread_msq_msg_t*)msg)->data[SC_MSG_MAP_ID]) && \
										 (e_SC_MAP_DATA_END   > ((pthread_msq_msg_t*)msg)->data[SC_MSG_MAP_ID]))

/***** メッセージデータ *****/
// ■共通
#define	SC_MSG_MSG_ID				0		// メッセージ種別ID

// ■地図データ取得用
// 【地図データ取得要求】
#define SC_MSG_REQ_MAP_READ_RESTYPE		1		// 応答方式
#define SC_MSG_REQ_MAP_READ_REQID		2		// 要求ID
#define SC_MSG_REQ_MAP_READ_USER_ID		3		// 要求ユーザID
#define SC_MSG_REQ_MAP_READ_TBL			4		// 要求テーブルアドレス
#define SC_MSG_REQ_MAP_READ_VOL			5		// 要求数
#define SC_MSG_REQ_MAP_READ_SYNCSEM		6		// セマフォアドレス
// 【地図データ開放要求】
#define SC_MSG_REQ_MAP_FREE_FREETYPE	1		// 開放方式
#define SC_MSG_REQ_MAP_FREE_REQID		2		// 要求ID
#define SC_MSG_REQ_MAP_FREE_USER_ID		3		// 要求ユーザID
#define SC_MSG_REQ_MAP_FREE_MAPKIND		4		// 地図種別
#define SC_MSG_REQ_MAP_FREE_TBL			5		// 要求テーブルアドレス
#define SC_MSG_REQ_MAP_FREE_VOL			6		// 要求数
// 【地図データ応答】
#define SC_MSG_RES_MAP_RESTYPE			1		// 応答種別
#define SC_MSG_RES_MAP_REQID			2		// 要求ID
#define SC_MSG_RES_MAP_TBL				3		// 要求テーブル
// 【地図データキャンセル】
#define SC_MSG_REQ_MAP_CAN_REQID		1		// キャンセル対象要求ID
#define SC_MSG_REQ_MAP_CAN_USER_ID		2		// 要求ユーザID

// ■地図描画用
#define	SC_MSG_REQ_RFRSH_MPS		1		// 表示操作対象地図

// ■経路描画用
#define	SC_MSG_REQ_VIEW_RT			1		// 描画する経路

// ■経路探索用
// 【要求】
#define	SC_MSG_REQ_RT_PLAN			1		// 経路探索条件
#define	SC_MSG_REQ_RT_SRCID			2		// 探索ID
#define	SC_MSG_REQ_RT_USELEVEL		3		// 探索使用レベル
#define	SC_MSG_REQ_RT_SETIDX		4		// 条件インデックス
// 【応答】
#define	SC_MSG_RES_RT_RESULT		1		// 処理結果
#define	SC_MSG_RES_RT_ROUTEID		2		// 推奨経路ID
#define	SC_MSG_RES_RT_RESCODE		3		// 結果コード
#define	SC_MSG_RES_RT_SETIDX		4		// 条件インデックス

// ■経路誘導用
// 【応答】
#define	SC_MSG_RES_RG_RESULT		1		// 処理結果
#define	SC_MSG_RES_RT_RESULT		1		// 処理結果

// ■アップロード完了用
// 【通知】
#define	SC_MSG_RES_UPLOAD_RESULT	1		// アップロード結果

// ■タイマ用
// 【要求】
#define	SC_MSG_REQ_PT_TIMER			1		// タイマ(秒)

// ■プローブデータ用
// 【要求】
#define SC_MSG_REQ_PRBDT_MEM_ADDR	1		// プローブデータメモリアドレス
#define SC_MSG_REQ_PRBDT_MEM_SIZE	2		// プローブデータメモリサイズ
#define SC_MSG_REQ_PRBDT_FILE_NAME	3		// プローブデータファイル名
#define SC_MSG_REQ_POSDT_MEM_ADDR	4		// 位置情報共有データメモリアドレス
#define SC_MSG_REQ_POSDT_MEM_SIZE	5		// 位置情報共有データメモリサイズ
#define SC_MSG_REQ_POSDT_FILE_NAME	6		// 位置情報共有データファイル名
#define SC_MSG_REQ_API_PARAM		7

// ■アラート用
#define SC_MSG_ALERTREQ_TYPE		1		// イベント
#define SC_MSG_ALERTREQ_PARAM1		2		// 入出力情報
#define SC_MSG_ALERTREQ_PARAM2		3		// 入出力情報
#define SC_MSG_ALERTREQ_PARAM3		4		// 入出力情報
#define SC_MSG_ALERTREQ_RESULT		2		// 結果
#define SC_ALERT_SCROLLTYPE_OFF		0		// スクロールモードOFF
#define SC_ALERT_SCROLLTYPE_ON		1		// スクロールモードON

// ■運転特性診断
// 【要求】
#define SC_MSG_REQ_SD_STARTTIME		1		// 運転特性診断用データ取得開始日時(YYYYMMDDhhmmssSSS形式)
#define SC_MSG_REQ_SD_TIMER			1		// タイマ値[秒]
// 【応答】
#define SC_MSG_RES_SD_RESULT		1		// 処理結果
#define SC_MSG_RES_SD_TRIP_ID		2		// トリップID
// 【通知】
#define SC_MSG_EVT_SD_DATA			1		// センサデータ
#define SC_MSG_EVT_SD_DATA_NUM		2		// センサデータ数

// ■交通情報
// 【要求】
#define SC_MSG_REQ_TR_EVT			1		// イベント
#define SC_MSG_REQ_TR_TIMER			2		// タイマ値[秒]

#endif // #ifndef SMCOREMSG_H_
