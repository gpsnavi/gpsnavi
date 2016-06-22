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
 * navicoredef.h
 *
 *  Created on: 2016/03/14
 *      Author:
 */

#ifndef NAVICOREDEF_H_
#define NAVICOREDEF_H_

/**
 * 型
 */
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
#ifdef Bool	// kana暫定
#undef Bool
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

/**
 * 処理結果
 */
#define NC_SUCCESS				(0)
#define NC_ERROR				(-1)
#define NC_PARAM_ERROR			(-2)

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
 * エラータイプ／警告タイプ
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
 * Linkタイプ
 */
#define	LST_START					0			// Linkタイプ：出発地
#define	LST_DEST					1			// Linkタイプ：目的地
#define	LST_BYPASS					2			// Linkタイプ：経由地
#define	LST_NORMAL					3			// Linkタイプ：一般
/**
 * シュミレーションエラーコード
 */
#define	NC_RESULT_SIM_FAILD			0x00000000	// 失敗
#define	NC_RESULT_SIM_SUCCESS		0x00000001	// 成功

/**
 * 経路地点最大数
 */
#define SC_CORE_RP_PLACE_MAX		7	// 地点情報数

/**
 * CarMoveStatus
 */
#define	CMS_STOP					0			// 停止
#define	CMS_MOVE					1			// 移動
#define	CMS_ARRIVE					2			// 到着

/**
 * 構造体定義
 */
#define	SC_PLACE_NAME_LEN			128		// 地点名称長
#define	SC_BROADSTRING_LEN			512		// 誘導点名長（道路名/交差点名/看板名、誘導点種別を示す文字列）
#define	SC_LANE_NUM					10		// レーン情報数
#define SC_SAL_GEM_ID_LEN			256		// GEMID

/**
 * 地図表示モード
 */
typedef enum _SC_MAP_VIEW_MODE {
	SC_MDM_HEADUP = 0, 		// ヘッディングアップモード
	SC_MDM_NORTHUP,			// ノースアップモード
	SC_MDM_BIRDVIEW			// バードビューモード
} SC_MAP_VIEW_MODE;

/* ロケーション更新関数ポインタ */
typedef void (*NC_LOCATORCBFUNCPTR)();

/**
 * ビットマップフォント情報
 */
typedef struct {
	// IN
	char	*str;			// 入力文字列
	FLOAT	fontSize;		// フォントサイズ
	INT32	outLineFlg;		// フチ有無
	UINT32	color;			// 文字色
	UINT32	outLineColor;	// フチ色
	UINT32	bkgdColor;		// 背景色
	FLOAT	rotation;		// 角度
	Bool 	lineBreak;		// 反転

	// OUT
	UChar	*pBitMap;		// ビットマップポインタ
	INT32	width;			// ビットマップ幅
	INT32	height;			// ビットマップ高
	INT32	strWidth;		// 文字列幅
	INT32	strHeight;		// 文字列高
} NCBITMAPFONTINFO;
/* ビットマップフォント生成関数ポインタ */
typedef INT32 (*NC_BITMAPFONTFUNCPTR)(NCBITMAPFONTINFO* pInfo);

/**
 * ビットマップ情報
 */
typedef struct {
	// IN
	char	*path;			// ファイルパス

	char	*image;			// データイメージ
	INT32	dataSize;		// ダータサイズ

	// OUT
	UChar	*pBitMap;		// ビットマップポインタ
	INT32	width;			// ビットマップ幅
	INT32	height;			// ビットマップ高

} NCBITMAPINFO;
/* 画像ファイル読み込み関数ポインタ */
typedef INT32 (*NC_IMAGEFUNCPTR)(NCBITMAPINFO* pInfo);

/**
 * 経緯度座標
 */
typedef struct _SMGEOCOORD {
	LONG longitude;						// 経度単位：1/1024秒
	LONG latitude;						// 緯度単位：1/1024秒
} SMGEOCOORD;

/**
 * 地点情報詳細
 */
typedef struct _SMPLACEINFO {
	SMGEOCOORD geo;						// 緯度経度
	Char name[SC_PLACE_NAME_LEN];		// 名称
	INT32 dataType;						// データタイプ
	Char dataId[SC_SAL_GEM_ID_LEN];		// データID
} SMPLACEINFO;

/**
 * 地点情報
 */
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

/**
 * 探索詳細エラー情報
 */
typedef struct _SMRPTIPINFO {
	INT32 tipClass;						// エラータイプ
	BYTE tipIndex;						// エラー発生した地点番号(0～)
	Bool isRePlan;						// リルートするかしないか
	Char reserve[2];					// 予約
	LONG appendOption;					// 付加情報
	INT32 warnCode;						// 条件付探索成功ワーニングコード
} SMRPTIPINFO;

/**
 * レーン情報
 */
typedef struct _SMSINGLELANE {
	INT32 laneFlag;						// 進行方向のレーン情報
	INT32 laneHightLight;				// ハイライトで表示するレーン
	INT32 advisableLaneFlag;			// 推奨レーンをハイライト表示
} SMSINGLELANE;

/**
 * リアルタイム案内情報
 */
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

#include "navicoredefex.h"

#endif /* NAVICOREDEF_H_ */
