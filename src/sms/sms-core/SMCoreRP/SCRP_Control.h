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
 * SCRP_Control.h
 *
 *  Created on: 2015/12/10
 *      Author: masutani
 */

#ifndef SCRP_CONTROL_H_
#define SCRP_CONTROL_H_

// 探索条件管理テーブル（探索開始時の設定情報を保持する）
typedef struct _SCRP_SEARCHSETTING {
	pthread_msq_msg_t msg;						// RMが受信したリクエストメッセージ
	SMCARSTATE car;								// 車両情報（共有メモリのコピー）
	SMRPOPTION option;							// 探索条件（共有メモリのコピー）
	SMRPPOINT point[RP_ROUTE_PLACE_MAX];		// 経路地点情報（共有メモリのコピー）
	SCRP_POINT rpPoint[RP_ROUTE_PLACE_MAX];		// 経路地点情報 pointの緯度経度正規化情報
	UINT8 pointNum;								// 経路地点情報格納数
	UINT32 routeSearchRequestId;				// 探索要求ID
	Bool isReplan;								// 再探索フラグ
	UINT8 replanSect;							// 探索開始区間インデックス（再探索）
	UINT8 useLevel;								// 探索使用レベル
	Bool cancel;								// true:キャンセル要求あり
	struct _b_setting {
		UINT16 reserve :9;						// リザーブ
		UINT16 trafficJam :1;					// 6	交通情報
		UINT16 toll :1;							// 5	有料道路
		UINT16 ferry :1;						// 4	フェリー
		UINT16 seasonReg :2;					// 2~3	季節規制
		UINT16 timeReg :2;						// 0~1	時間規制
	} b_setting;
} SCRP_SEARCHSETTING;

// ネットワーク管理
typedef struct _SCRP_NETCONTROLER {
	SCRP_PCLINFO* parcelInfo;				// パーセル情報先頭
	UINT16 parcelInfoVol;					// パーセル情報数
	SCRP_LINKINFO* linkTable;				// リンク情報先頭
	UINT32 linkTableVol;					// リンク情報数
	SCRP_DIJKSTRAHEAP heap;					// ヒープテーブル
	SCRP_LEVELTBL* levelTbl;				// レベルテーブル
	SCRP_LEVELAREA* levelArea;				// 計算中レベルエリア
	UINT16 calculatingDivIdx;				// 計算中分割エリアインデックス
} SCRP_NETCONTROLER;

// 区間情報管理
typedef struct _SCRP_SECTCONTROLER {
	UINT32 routeCond;						// 区間探索条件
	SCRP_LEVELTBL levelTable;				// レベルテーブル
	SCRP_NEIGHBORINFO neighbor[2];			// 近傍情報OD
	SCRP_NETCONTROLER netTable;				// ネットワーク情報
	SCRP_CANDMANAGER candMng;				// 候補経路管理
	UINT8 sectIndex;						// 区間番号
	UINT16 sectDist;						// 区間直線距離
} SCRP_SECTCONTROLER;

// 探索管理情報
typedef struct _SCRP_MANAGER {
	SCRP_SEARCHSETTING *rcSetting;					// 設定情報
	SCRP_NEIGHBORINFO neighbor[RP_ROUTE_PLACE_MAX];	// 出発～経由～目的 MAX地点分
	UINT32 sectDist[5];								// 区間間直線距離
	UINT32 routeId;									// 経路ID
	E_SCRP_RPTYPE rpType;							// 探索タイプ
	Bool isReplan;									// リルート情報
	UINT8 sectVol;									// 総区間数
} SCRP_MANAGER;

#define RP_SEARCHSETTING_SIZE			(5)				/* 探索条件リングバッファサイズ */

/* 探索条件を管理する（RM側からの変更のみとする） */
typedef struct _SCRP_RPMSTATE {
	E_RP_STATE rpState;					// 探索状態保持変数
	SCRP_SEARCHSETTING queueRequest[RP_SEARCHSETTING_SIZE];		// 探索条件リングバッファ
	UINT16 validVol;					// リングバッファ有効データ数
	UINT16 validIdx;					// リングバッファ有効データインデックス
	UINT16 invalidVol;					// リングバッファ無効データ数
	UINT16 invalidIdx;					// リングバッファ無効データインデックス
} SCRP_RPMSTATE;

/* 探索中のステータスを保持する。（RC側からの変更のみとする） */
typedef struct _SCRP_RPCSTATE {
	E_RC_CALC_PROCESS process;			// 探索処理プロセス
	INT16 currentIdx;					// 計算中の探索条件のインデックス TODO
	INT32 totalSect;					// 全区間数
	INT32 calcSect;						// 処理中区間インデックス
	INT32 totalSplit;					// 全エリア分割数（計算中区間のもの）
	INT32 calcSplit;					// 処理中分割エリアインデックス（計算中区間のもの）
	INT32 errorCode;					// 探索結果コード
	INT32 warnCode;						// 条件付探索結果コード
	Bool isReplan;						// リルートフラグ TODO 削除
	SCRP_MANAGER routeCalcManager;		// 経路計算管理
} SCRP_RPCSTATE;

#endif /* SCRP_CONTROL_H_ */
