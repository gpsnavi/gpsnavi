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
 * smscore.h
 *
 *  Created on: 2015/11/04
 *      Author: masutani
 *
 *  app外部公開関数に必要な宣言を記載する。
 *  jniとして公開する際にはjava側に同値の定数定義が必要となるものがあるので注意してください。
 */

#ifndef SMSCORE_H_
#define SMSCORE_H_

#include <navicoredef.h>

// TODO
#define CORE_VERSION	"0.0.7"
#define API_VERSION "1.0.20160329-00"

#ifdef __cplusplus
extern "C" {
#endif

// シミュレータ環境
typedef enum _E_SC_SIMULATE {
	e_SIMULATE_AVAILABLE,		// シミュレータ環境で動作する
	e_SIMULATE_UNAVAILABLE		// シミュレート環境で動作しない
} E_SC_SIMULATE;
// シミュレータ状態
typedef enum _E_SC_SIMSTATE {
	e_SIMULATE_STATE_READY,
	e_SIMULATE_STATE_STRAT,		// シミュレート中
	e_SIMULATE_STATE_STOP,		// シミュレート停止中
	e_SIMULATE_STATE_SUSPEND	// シミュレート一時停止中
} E_SC_SIMSTATE;

#if 0 // navicoredef.hへ移動
//-----------------------------------
// 探索条件
//-----------------------------------
#define	RT_OPTION 					0			// リセットタイプ：属性
#define	RT_SPEEDint					1			// リセットタイプ：速度
#define	RT_ALL						2			// リセットタイプ：全部
/**
 * 探索条件はRPdefine.RPconstクラスにて定数定義されるものと同値とすること。
 */
#define RPC_INVALID					-1			// 探索条件：無効
#define RPC_HIGHWAY					0			// 探索条件：有料優先
#define RPC_NORMAL					1			// 探索条件：一般優先
#define RPC_TIME					2			// 探索条件：時間優先
#define RPC_DISTANCE				3			// 探索条件：距離優先
#define RPC_SIZE					4			// 探索条件の数量
/**
 * 地点データタイプはDMdefine.DMconstクラスにて定数定義されるものと同値とすること。
 */
#define PLACE_SELECT				0			// 地点データタイプ：選択地点
#define PLACE_REGISTE				1			// 地点データタイプ：登録地
#define PLACE_HISTORY				2			// 地点データタイプ：履歴
#define PLACE_GEM					3			// 地点データタイプ：GEM
#define PLACE_POI					4			// 地点データタイプ：施設
#define PLACE_POI_GATE				5			// 地点データタイプ：施設入り口
/**
 * リルートタイプはDMdefine.DMconstクラスにて定数定義されるものと同値とすること。
 */
#define REROUTE_NORMAL				0			// リルートタイプ：通常
#define REROUTE_POI_GATE			1			// リルートタイプ：施設入り口
/**
 * 規制タイプはRPdefine.RPconstクラスにて定数定義されるものと同値とすること。
 */
#define	RTU_CAR						0			// 規制タイプ：車規制
#define	RTU_MOTOR					1			// 規制タイプ：二輪車規制
#define	RTU_NONE					2			// 規制タイプ：規制無視
/**
 * 車両タイプはRPdefine.RPconstクラスにて定数定義されるものと同値とすること。
 */
#define	CTY_LIGHT_CAR				0			// 車両タイプ：軽型車
#define	CTY_NORMAL_CAR				1			// 車両タイプ：一般車
#define	CTY_MIDDLE_CAR				2			// 車両タイプ：中型車
#define	CTY_LARGE_CAR				3			// 車両タイプ：大型車
#define	CTY_GREAT_LARGE_CAR			4			// 車両タイプ：特大車
#define	CTY_SIZE					CTY_GREAT_LARGE_CAR	// 車両タイプ数量
/**
 * エラータイプ／警告タイプはRPdefine.RPconstクラスにて定数定義されるものと同値とすること。
 */
#define	EC_OK						0x00000000	// エラータイプ：探索成功
#define	EC_CANCEL					0x00000001	// エラータイプ：探索キャンセル
#define	EC_FAILED					0x00000002	// エラータイプ：探索失敗
#define	EC_SAME_ROAD				0x00000004	// エラータイプ：距離が近すぎる
#define	EC_WAYPOINT_MAX				0x00000008	// エラータイプ：地点数オーバー
#define	EC_LIMITED_DISTANCE_OVER	0x00000010	// エラータイプ：制限距離オーバー
#define	EC_NOROUTE					0x00000020	// エラータイプ：経路がない
#define	EC_AROUND_NO_LINK			0x00000040	// エラータイプ：周辺にリンクが見つからない
#define	EC_ROUTE_SPLIT				0x00010000	// 警告タイプ：断裂経路
/**
 * LinkタイプはRPdefine.RPconstクラスにて定数定義されるものと同値とすること。
 */
#define	LST_START					0			// Linkタイプ：出発地
#define	LST_DEST					1			// Linkタイプ：目的地
#define	LST_BYPASS					2			// Linkタイプ：経由地
#define	LST_NORMAL					3			// Linkタイプ：一般
#endif // navicoredef.hへ移動
// シュミレーションエラーコード
#if 0 // navicoredef.hへ移動
#define	e_SC_RESULT_SIM_FAILD		0x00000000	// 失敗
#define	e_SC_RESULT_SIM_SUCCESS		0x00000001	// 成功
#endif // navicoredef.hへ移動
#define	e_SC_RESULT_SIM_FINISH		0x6FFF03C8	// 目的地到達
#define	e_SC_RESULT_SIM_STOP		0x6FFF03C3	// 停止状態
#define	e_SC_RESULT_SIM_INITFAILD	0x6E0103C2	// 初期化失敗
#define	e_SC_RESULT_SIM_NOROUTE		0x6FFF03C4	// 経路なし
#define	e_SC_RESULT_SIM_ERROR		0x6FFF03C7	// 内部エラー
#define	e_SC_RESULT_SIM_RTNOTMATCH	0x6FFF03C6	// ルート探索結果にマッチングしてない
//-----------------------------------
// 交通情報
//-----------------------------------
// 提供情報源
#define TRAFFIC_SRCID_PROBE			0x0001		// プローブ
#define TRAFFIC_SRCID_VICS			0x0002		// VICS
// 渋滞度
#define TRAFFIC_CNGS_LVL_UNKNOWN	0x0001		// 不明
#define TRAFFIC_CNGS_LVL_0			0x0002		// 渋滞なし
#define TRAFFIC_CNGS_LVL_1			0x0004		// 混雑
#define TRAFFIC_CNGS_LVL_2			0x0008		// 渋滞
#define TRAFFIC_CNGS_LVL_ALL		(TRAFFIC_CNGS_LVL_0|TRAFFIC_CNGS_LVL_1|TRAFFIC_CNGS_LVL_2)
// 道路種別
#define TRAFFIC_ROAD_KIND_ALL		0xFFFF		// 全道路種別
// 位置モード
#define TRAFFIC_POS_CAR				0			// 自車位置
#define TRAFFIC_POS_SCROLL			1			// スクロール位置
//-----------------------------------
// 構造体定義
//-----------------------------------
#if 0 // navicoredef.hへ移動
#define	SC_PLACE_NAME_LEN			128		// 地点名称長
#endif // navicoredef.hへ移動
#define	SC_CROSS_NAME_SIZE			512		// 交差点名称サイズ
#if 0 // navicoredef.hへ移動
#define	SC_BROADSTRING_LEN			512		// 誘導点名長（道路名/交差点名/看板名、誘導点種別を示す文字列）
#endif // navicoredef.hへ移動
#define SC_VOICE_TTS_LEN			512		// 音声ＴＴＳ長
#define	SC_VOICE_ID_LIST_SIZE		32		// 音声TTS文言ID数
#if 0 // navicoredef.hへ移動
#define	SC_LANE_NUM					10		// レーン情報数
#endif // navicoredef.hへ移動
#define	SC_USER_ID					(128 + 4)	// ユーザID
#define	SC_USER_NAME				(60 + 4)	// ユーザ名
#define	SC_TIME_MISS				8		// 24時間制時刻(xx:xx形式)
#define SC_DIST_TIME				8		//到着までかかる時間(到着予想時刻ではない)
#if 0 // navicoredef.hへ移動
#define SC_SAL_GEM_ID_LEN			256		// GEMID
#endif // navicoredef.hへ移動
#define SC_ROADTYPE_NUM				16
#define SC_LINKTYPE_NUM				13
#define SC_TURNTYPE_NUM				16
#define SC_SRCHMODE_NUM				RPC_SIZE

#if 0 // navicoredef.hへ移動
/**
 * 経路地点最大数はRPdefine.RPconstクラスにて定数定義されるものと同値とすること。
 */
#define SC_CORE_RP_PLACE_MAX		7	// 地点情報数
#endif // navicoredef.hへ移動
//-----------------------------------
// 地域クラス
//-----------------------------------
typedef enum {
	e_AREA_CLS1 = 0,	// 地域クラス1
	e_AREA_CLS2,		// 地域クラス2
	e_AREA_CLS3,		// 地域クラス3
	e_AREA_CLS4,		// 地域クラス4
	e_AREA_CLS5,		// 地域クラス5
	e_AREA_CLS6,		// 地域クラス6

	e_AREA_CLS_MAX		// 地域クラス最大値
} E_SC_AREA_CLS;

//-----------------------------------
// アプリ設定
//-----------------------------------
#define	SYS_LANGUAGE_INIT			0			//
#define	SYS_LANGUAGE_JP				1			// 言語：日本語
#define	SYS_LANGUAGE_EN				2			// 言語：英語
#define	SYS_REGION_INIT				0			//
#define	SYS_REGION_JPN				1			// リージョン：日本
#define	SYS_REGION_NAM				2			// リージョン：北米
#if 0 // navicoredef.hへ移動
// 経緯度座標
typedef struct _SMGEOCOORD {
	LONG longitude;						// 経度単位：1/1024秒
	LONG latitude;						// 緯度単位：1/1024秒
} SMGEOCOORD;
#endif // navicoredef.hへ移動

#if 0 // navicoredefex.hへ移動		AIKAWA.AIKAWA
// 車両状態情報
typedef struct _SMCARSTATE {
	SMGEOCOORD coord;					// 経緯度位置座標
	FLOAT speed;						// 車両瞬時速度 単位: m/s
	INT32 dir;							// 車両進行方向 単位:度
	Bool onRoad;						// 車両位置は道路上か否か（true:道路上にある、false:道路上にない）
	Bool isRouteSelected;				// 車両位置は経路上か否か（true:経路上にある、false:経路上にない）
	Char reserve[2];					// 予約
	INT32 roadClass;					// マッチングした道路クラス
	INT32 linkId;						// 自車位置のリンクID
	LONG parcelId;						// 自車位置のパーセルID
	INT32 parcelDiv;					// 自車位置のパーセル分割識別子
	Char gpsTime[20];					// 位置情報を取得したGPS時刻
} SMCARSTATE;
#endif // navicoredef.hへ移動

// 矩形
typedef struct _SMRECT {
	INT32 left;							// 座標
	INT32 right;						// 座標
	INT32 top;							// 座標
	INT32 bottom;						// 座標
} SMRECT;

#if 0 // navicoredef.hへ移動
// 地点情報詳細
typedef struct _SMPLACEINFO {
	SMGEOCOORD geo;						// 緯度経度
	Char name[SC_PLACE_NAME_LEN];		// 名称
	INT32 dataType;						// データタイプ
	Char dataId[SC_SAL_GEM_ID_LEN];		// データID
} SMPLACEINFO;

// 地点情報
typedef struct _SMRPPOINT {
	SMGEOCOORD coord;					// 地点座標
	Char nodeName[SC_PLACE_NAME_LEN];	// 地点名称
	INT32 rpPointType;					// 地点タイプ LST_START/LST_DEST/LST_BYPASS
	INT32 cond;							// 計画条件
	Bool isPassed;						// 経由地を経由するか否か
	Char reserve[3];					// 予約
	LONG rpPointIndex;					// 位置番号　出発点0,1,2,···,終点n
	INT32 placeType;					// PLACE_SELECT/PLACE_POI_GATE...
	INT32 reRouteType;					// REROUTE_NORMAL/REROUTE_POI_GATE
	SMPLACEINFO poiPlace;				// 地点情報
} SMRPPOINT;
#endif // navicoredef.hへ移動

// 経路探索属性
typedef struct _SMRPOPTION {
	INT32 rpCond;						// 探索条件   SC_RP_ENUMTYPEの探索条件を参照
	LONG appendCond;					// 付加条件   SC_RP_ENUMTYPEの付加条件を参照
	INT32 regulationType;				// 規制情報   SC_RP_ENUMTYPEの規制を参照
	INT32 vehicleType;					// 車両タイプ SC_RP_ENUMTYPEの車両タイプを参照
	INT32 tollType;						// 料金タイプ
} SMRPOPTION;

#if 0 // navicoredef.hへ移動
// 地点の詳細情報
typedef struct _SMPLACE {
	SMGEOCOORD crdPos;					// 地点座標
	Char name[SC_PLACE_NAME_LEN];		// 地点名称
	INT32 rpCond;						// 地点探索条件（現在地点から次の地点までの探索条件）
	INT32 matchMode;					// マッチモード
} SMPLACE;

// 探索詳細エラー情報
typedef struct _SMRPTIPINFO {
	INT32 tipClass;						// エラータイプ
	BYTE tipIndex;						// エラー発生した地点番号(0～)
	Bool isRePlan;						// リルートするかしないか
	Char reserve[2];					// 予約
	LONG appendOption;					// 付加情報
	INT32 warnCode;						// 条件付探索成功ワーニングコード
} SMRPTIPINFO;
#endif // navicoredef.hへ移動

// シミュレータ
typedef struct _SMSIMULATE {
	E_SC_SIMULATE simulate;				// シミュレータ環境かどうか
	INT32 speed;						// シミュレータ速度
	INT32 state;						// シミュレータ状態
} SMSIMULATE;

// リルート条件閾値
typedef struct _SMREPLAN {
	INT32 distance;						// 自車が探索ルートから離れた距離[m]
	INT32 angle;						// 自車角度と最寄の探索ルートのリンクとの角度差[度]
} SMREPLAN;

#if 0 // navicoredef.hへ移動
// レーン情報
typedef struct _SMSINGLELANE {
	INT32 laneFlag;						// 進行方向のレーン情報
	INT32 laneHightLight;				// ハイライトで表示するレーン
	INT32 advisableLaneFlag;			// 推奨レーンをハイライト表示
} SMSINGLELANE;

// リアルタイム案内情報
typedef struct _SMREALTIMEGUIDEDATA {
	INT32 turnDir;						// 次の交差点の曲がり方向
	LONG remainDistToNextTurn;			// 次の曲がる開始までの距離
	Bool showTrafficLight;				// 次の交差点に信号機のあり/無し
	LONG remainTimeToNextPlace;			// 次の経由地や目的地までの残り時間
	Bool destination;					// remainDistToNextPlaceが、それぞれ目的地まで残距離、残り時間か否かを意味する
	UINT16 bypass;						// 現在地から前方で、最も近い地点にある以下の番号を示す
	LONG remainDistToNextPlace;			// 次の経由地や目的地までの残距離
	Char nextBroadString[SC_BROADSTRING_LEN];	// 次の表示すべき誘導点名（道路名/交差点名/看板名、誘導点種別を示す文字列）
	INT32 roadLaneNum;					// 車線数
	SMSINGLELANE roadLane[SC_LANE_NUM];	// 車線
	LONG passedDistance;				// 出発地からカーマークが移動した距離
	Bool aheadPoint;					// 道なりか否か
	Bool valid;							// SC_Guide_GetRealTimeInfoによって取得するGetJNREALTIMEGUIDEDATAの有効性のこと
	INT32 graphMaxShowDist;				// 誘導点拡大図が表示される最大距離
	INT32 roadType;						// 道路の種別
	INT32 roadSituation;				// 案内点の種別
	INT32 nextBypassIndex;				// 次の交差点を経由地にする場合の、当該経由地のインデックス
	INT32 roadLaneAtGuidePointNum;		// 案内点のレーン情報数
	SMSINGLELANE roadLaneAtGuidePoint[SC_LANE_NUM];	// 案内点のレーン情報
	SMGEOCOORD coord;					// 交差点座標
} SMREALTIMEGUIDEDATA;
#endif // navicoredef.hへ移動

// 音声ＴＴＳ用 ID追加マクロ
#define RT_SET_VOICE_LIST(dst, val)							\
		{													\
			(dst)->voice_list[(dst)->current] = (val);		\
			(dst)->current++;								\
		}

// 音声ＴＴＳ ID情報
typedef struct {
	UINT8 current;								// 使用位置
	UINT16 voice_list[SC_VOICE_ID_LIST_SIZE];	// 文言IDリスト
	UINT16 valiavbleNum;						// 可変数値
	UINT8 valiavbleName[SC_CROSS_NAME_SIZE];	// 可変文字列
} RT_VOICE_t;

// 音声ＴＴＳ情報
typedef struct _SMVOICETTS {
	INT32 priority;							// 発話優先度
//	INT32			len;					// サイズ (byte)
//	Char			tts[SC_VOICE_TTS_LEN];	// ＴＴＳ文字列

	RT_VOICE_t tts;
} SMVOICETTS;

// 交差点情報
typedef struct {
	UINT32 lengthToStart;				// 出発地からの距離
	INT32 roadDir;						// ターン方向
	INT32 turnListIcon;					// アイコン種類
	Char name[SC_BROADSTRING_LEN];		// 誘導点名称
	SMGEOCOORD rdPos;					// 誘導点緯度経度
	UINT32 linkId;						// 誘導点進入リンクID
	INT32 roadType;						// 誘導点進入道路種別
	BYTE byPassIndex;					// 経由地点番号
	INT32 roadSituation;				// 案内点の種別
} SMTURNINFO;

// ターンリスト情報
typedef struct {
	INT32 turnNum;						// 交差点情報数
	SMTURNINFO *turnInfo;				// 交差点情報(実体はRGスレッド管理)
} SMTURNLIST;

// ターンバイターンリスト表示用交差点情報
typedef struct {
	LONG remainDist;					// 交差点までの残距離
	INT32 turnDir;						// 案内方向
	INT32 roadSituation;				// 案内種別
	INT32 passNo;						// 案内地点番号
	INT32 wayPointNo;					// 経由地点番号
	Bool showTrafficLight;				// 次の交差点に信号機のあり/無し
	Char name[SC_BROADSTRING_LEN];		// 誘導点名称
	SMGEOCOORD coord;					// 交差点座標
} SMGUIDEDATA;

// 経路ヘッダ情報
typedef struct {
	UINT32 length;						// 経路総距離
	UINT32 avetime;						// 経路所要時間
	UINT32 hwaylength;					// 経路高速距離
	UINT32 tolllength;					// 経路有料距離
	UINT32 tollfee;						// 経路料金
} SMROUTEHEADINFO;

// 交差点拡大図サイズ
typedef struct {
	INT32 width;						// [I/O]Ｘ方向サイズ
	INT32 height;						// [I/O]Ｙ方向サイズ
} SMSCDYNAMICGRAPHISIZE;

#if 0 // navicoredefex.hへ移動		AIKAWA.AIKAWA
// ユーザ定義アイコンの構造体
typedef struct _SMMAPDYNUDI {
	INT32 Longititude;					// 経度座標、値範囲は383385600～619315200
	INT32 Latitude;						// 緯度座標、値範囲は44236800～201523200
	INT32 IconID;						// アイコンID
} SMMAPDYNUDI;
#endif // navicoredefex.hへ移動

// 拡大図情報
typedef struct _SMBITMAPINFO {
	Char *bitmap;
	UINT32 bitmapsize;
	UINT32 bufsize;
} SMBITMAPINFO;

// 探索コスト
typedef struct _SMRTCOST {
	UINT32 speed[SC_ROADTYPE_NUM][SC_LINKTYPE_NUM];			// 平均走行速度
	UINT32 weight[SC_ROADTYPE_NUM][SC_LINKTYPE_NUM];		// 加重コスト
	UINT32 turn[SC_TURNTYPE_NUM];							// 転向コスト
	UINT8 turn_apply_f[SC_ROADTYPE_NUM][SC_ROADTYPE_NUM];	// 進脱道路種別毎の転向コスト適用有無
} SMRTCOST;

// 探索コスト情報
typedef struct _SMRTCOSTINFO {
	SMRTCOST routeCost[SC_SRCHMODE_NUM];					// 初期コスト
} SMRTCOSTINFO;

// ジャンル
typedef struct _SMGENRE {
	UINT32 code;											// コード
	Char name[64];											// 名称
} SMGENRE;

// ジャンルデータ
typedef struct _SMGENREDATA {
	Char format[32];										// フォーマット
	Char date[32];											// 日付
	UINT32 num;												// ジャンル数
	SMGENRE *genre;											// ジャンル
} SMGENREDATA;

// リンク情報
typedef struct _SMLINKDATA {
	UINT32 parcelId;										// パーセルID
	UINT32 linkId;											// リンクID
} SMLINKDATA;

// 経路バックアップ
typedef struct _SMROUTEBACKUP {
	SMRPPOINT point[SC_CORE_RP_PLACE_MAX];
	INT32 guide_status;
	INT32 pointNum;
	INT32 region;
} SMROUTEBACKUP;

// 地域クラスコード
typedef struct _SMAREACLSCODE {
	UINT16 code[e_AREA_CLS_MAX];
} SMAREACLSCODE;

// マッピングアラート情報
typedef struct _SMMAPPINGALERT {
	SMGEOCOORD posi;										// アラート経緯度位置
	INT32 x;												// アラート表示スクリーン座標
	INT32 y;												// アラート表示スクリーン座標
	INT32 udi;												// UDI
} SMMAPPINGALERT;

// 交通情報
typedef struct _SMTRAFFIC {
	UINT16 srcId;											// 表示対象提供情報源(bit)
	UINT16 cngsLvl;											// 表示対象渋滞度(bit)
	UINT16 roadKind;										// 表示対象道路種別(bit)
	UINT16 updateTime;										// 自動更新時間(秒) 0:OFF/1以上:ON
	Bool disp;												// 表示ON/OFF
} SMTRAFFIC;

//-----------------------------------
// 無効値定義
//-----------------------------------
#define	D_SC_INVALID_VALUE_8	0xff
#define	D_SC_INVALID_VALUE_16	0xffff
#define	D_SC_INVALID_VALUE_32	0xffffffff

#if 0 // navicoredef.hへ移動
// 地図表示モード
typedef enum _SC_MAP_VIEW_MODE {
	SC_MDM_HEADUP = 0, 		// ヘッディングアップモード
	SC_MDM_NORTHUP,			// ノースアップモード
	SC_MDM_BIRDVIEW			// バードビューモード
} SC_MAP_VIEW_MODE;
#endif // navicoredef.hへ移動

#if 0 // navicoredef.hへ移動		AIKAWA.AIKAWA
// ロケーション
typedef enum _SC_CARLOCATION_TYPE {
	e_SC_CARLOCATION_NOW = 0,
	e_SC_CARLOCATION_REAL,			// 実ロケーション
	e_SC_CARLOCATION_SIMU			// シミュレートロケーション
} E_SC_CARLOCATION_TYPE;
#endif // navicoredefex.hへ移動

#ifdef __cplusplus
}
#endif

#endif /* SMSCORE_H_ */
