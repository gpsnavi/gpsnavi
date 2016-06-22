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
 * SMCoreRPDef.h
 *
 *  Created on: 2015/11/11
 *      Author: masutani
 */

#ifndef SMCORERPDEF_H_
#define SMCORERPDEF_H_

#define _RP_PATCH_RAIKAMUTURN			(1)			/* パッチ：ライカムUターンパッチ */
#define _RP_PATCH_OKINAWAAREA			(0)			/* パッチ：沖縄エリアパッチ コンパイルスイッチ：0無効 1有効 */
#define _RP_USE_OLDAREA					(0)			/* 他：旧エリア生成処理の使用可否 強制的にLv1探索が実行される */
#define _RP_ROUTE_CONVERTLV				(1)			/* 他：経路レベル変換を行う */
#define _RP_ROUTE_LV2SPLITCHECKADDWP	(1)			/* 他：レベル２を含む経路や断裂経路をチェックしてから経由地追加を行う */
#define _RP_ROUTE_LV2ROUTESTRIP			(1)			/* 他：レベル２を含む経路や断裂経路をカットする。このフラグを落とし且つMP_DrawRoute.cの一部を有効にすると無効経路のみ断裂表示が可能になります。 */

#define _ROUTECALC_QUICK_SAMPLE			(0)			/* 探索高速化サンプル処理（パーセル跨ぎネットワーク除去） */
#define _RPLAPTIME_MAKENET				(0)			/* 性能測定：ネットワーク生成 */
#define _RPLAPTIME_DIJKSTRA				(0)			/* 性能測定：ダイクストラ */
#define _RPLAPTIME_MAKECAND				(0)			/* 性能測定：候補経路生成 */
#define _RPLAPTIME_ROUTEMAKE			(0)			/* 性能測定：推奨経路生成 */

#define _RP_LOG_MASTERROUTE				(0)			/* ログ：推奨経路ダンプ */
#define _RP_LOG_NEIGBORLINK				(0)			/* ログ：近傍リンクダンプ */
#define _RP_LOG_LEVELTBL				(0)			/* ログ：レベルテーブルダンプ */
#define _RP_LOG_RTCALCSTEP				(1)			/* ログ：計算ステップ情報 */
#define _RP_LOG_MEMINFO					(0)			/* ログ：メモリ関連ダンプ */
#define _RP_LOG_DOWNLOADAREA			(0)			/* ログ：ダウンロードエリア情報ダンプ */
#define _RP_LOG_AREAINFO				(0)			/* ログ：エリア情報ダンプ */
#define _RP_LOG_NETWORK					(0)			/* ログ：ネットワーク情報ダンプ */
#define _RP_LOG_CANDINFO				(0)			/* ログ：候補経路情報ダンプ */
#define _RP_LOG_CALCCOST				(0)			/* ログ：コスト計算用テーブルダンプ */

#define _RP_CHECK_NET					(0)			/* 整合性チェック */
#define _RP_CHECK_CAND					(0)			/* 整合性チェック */
#define _RP_CHECK_ROUTE					(0)			/* 整合性チェック */

#define ALL_F32							(0xFFFFFFFF)
#define ALL_F16							(0xFFFF)
#define ALL_F8							(0xFF)

#define SCRP_ON							(1)
#define SCRP_OFF						(0)

#define RP_MSG_SEND_RETRYCOUNT			(3)				/* MSG送信回数 */

#define RP_LEVEL1						(MAP_LEVEL1)	/* レベル１ */
#define RP_LEVEL2						(MAP_LEVEL2)	/* レベル２ */
#define RP_LEVEL3						(MAP_LEVEL3)	/* レベル３ */
#define RP_LEVEL4						(MAP_LEVEL4)	/* レベル４ */
#define RP_MIN_LEVEL					(RP_LEVEL1)		/* 探索最下層 */
#define RP_MAX_LEVEL					(RP_LEVEL2)		/* 探索最上層 */

#define RP_ROUTE_PLACE_MAX				(SC_CORE_RP_PLACE_MAX)	/* 経路地点最大数 */
#define RP_CLOSS_MAX					(16)			/* 交差点数 */
#define RP_LIMITED_DISTANCE				(250000)		/* 探索可能距離 */

// 探索条件取得マクロ
#define RP_SETTING_TIMEREG_ON			SCRP_ON			/* 時間規制考慮する */
#define RP_SETTING_TIMEREG_OFF			SCRP_OFF		/* 時間規制考慮しない */
#define RP_SETTING_TIMEREG_AVOID		(2)				/* 時間規制リンクを使わない */
#define RP_SETTING_SEASONREG_ON			SCRP_ON			/* 時間規制考慮する */
#define RP_SETTING_SEASONREG_OFF		SCRP_OFF		/* 時間規制考慮しない */
#define RP_SETTING_SEASONREG_AVOID		(2)				/* 時間規制リンクを使わない */
#define RP_SETTING_FERRY_ON				SCRP_ON			/* フェリー航路利用する */
#define RP_SETTING_FERRY_OFF			SCRP_OFF		/* フェリー航路利用しない */
#define RP_SETTING_TOLL_ON				SCRP_ON			/* 有料道路利用する */
#define RP_SETTING_TOLL_OFF				SCRP_OFF		/* 有料道路利用しない */
#define RP_SETTING_TRAFFICJAM_ON		SCRP_ON			/* 交通情報利用する */
#define RP_SETTING_TRAFFICJAM_OFF		SCRP_OFF		/* 交通情報利用しない */

#define RM_SETTING_USESIDE				(0)				/* 設定内容使用面 */
#define RM_SETTING_UNUSESIDE			(1)				/* 設定内容未使用面 */

#define RP_DATA_REQ_MAX					(128)			/* TODO データ要求最大値 */

// 経路探索ステータス
typedef enum _E_RP_STATE {
	e_RC_STATE_INIT = 0,					// 初期状態
	e_RC_STATE_STANDBY,						// 探索待機中
	e_RC_STATE_ROUTECALC,					// 探索中
	e_RC_STATE_END							// 終端
} E_RP_STATE;

// 経路計算プロセス
typedef enum _E_RC_CALC_PROCESS {
	e_RC_PROC_STANDBY = 0,					// 探索待機中
	e_RC_PROC_MAKEAREA,						// エリア生成
	e_RC_PROC_NEIGHBOR,						// 近傍処理
	e_RC_PROC_MAKENETWORK,					// ネットワークテーブル生成
	e_RC_PROC_ROUTECALC,					// 探索中（ダイクストラ）
	e_RC_PROC_MAKECAND,						// 候補経路生成
	e_RC_PROC_MAKEROUTE,					// 推奨経路生成
	e_RC_PROC_END,							// 終端
} E_RC_CALC_PROCESS;

// 探索エリア種別
typedef enum _E_SCRP_AREA {
	e_AREA_MAKE_LV1_TOP = 0,				// レベル１トップ
	e_AREA_MAKE_LV1_O,						// レベル１O側
	e_AREA_MAKE_LV1_D,						// レベル１D側
	e_AREA_MAKE_LV2_TOP,					// レベル２トップ
	e_AREA_MAKE_END							// 終端
} E_SCRP_AREA;

// 探索ステップ
typedef enum _E_RP_RTCALCSTEP {
	e_RP_STEP_LV1TOP = 0,					// Lv1Top
	e_RP_STEP_LV1O,							// Lv1OSide
	e_RP_STEP_LV1D,							// Lv1DSide
	e_RP_STEP_LV2TOP,						// Lv2Top
	e_RP_STEP_END,							// 終端
} E_RP_RTCALCSTEP;

// （非推奨）探索タイプ
typedef enum _E_SCRP_RPTYPE {
	e_RP_RPTYPE_SINGLE = 0,					// 単経路探索
	e_RP_RPTYPE_REROUTE,					// 再探索
	e_RP_RPTYPE_END,						// 終端
} E_SCRP_RPTYPE;

// （未使用）ユーザ情報
typedef enum _E_RP_USER {
	e_RP_USER_WALK = 0,						// 徒歩
	e_RP_USER_CAR,							// 車
	e_RP_USER_BICYCLE,						// 自転車
	e_RP_USER_END,							// 終端
} E_RP_USER;

// 共通：地図アドレス格納用
typedef struct _SCRP_MAPDATA {
	UINT32 parcelId;						// パーセルID
	MAL_HDL road;							// 道路ネットワーク
	MAL_HDL shape;							// 形状データ
	MAL_HDL basis;							// パーセル基本情報
} SCRP_MAPDATA;

// 共通：地図読み込み結果格納テーブル
typedef struct _SCRP_MAPREADTBL {
	UINT32 mapVol;							// 地図数
	SCRP_MAPDATA* mapList;					// 地図アドレスリスト
} SCRP_MAPREADTBL;

// 共通：パーセル矩形エリア情報
typedef struct _SCRP_PCLRECT {
	UINT32 parcelId;						// パーセルID
	UINT16 xSize;							// X方向枚数
	UINT16 ySize;							// Y方向枚数
} SCRP_PCLRECT;

// 共通：座標情報（正規化）
typedef struct _SCRP_POINT {
	UINT32 parcelId;						// パーセルID
	UINT16 x;								// X座標(0～4096)
	UINT16 y;								// Y座標(0～4096)
} SCRP_POINT;

#endif /* SMCORERPDEF_H_ */
