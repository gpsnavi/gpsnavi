/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#pragma once

typedef struct _BIN_DATA {
	UINT8*	p_data;
	UINT32	Size;
} T_BIN_DATA;

typedef struct _ROAD_NAME_INFO {
	T_POINT	pos;		// 座標
	T_POINT	endPos;		// 終端座標
	FLOAT	angle;		// 角度
	UINT32	ofs;		// 道路名称オフセット
	UINT16	roadKind;	// 道路種別
	Bool	turnOver;	// 反転フラグ
} T_ROAD_NAME_INFO;

// VBO情報リスト
typedef struct _VBO_INFO_LIST {
	T_VBO_INFO	vbo[MP_VBO_CNT_MAX];	// VBO情報
	INT32		vboCnt;					// VBO情報数
	Bool		setFlg;					// VBO設定済みフラグ
} T_VBO_INFO_LIST;

// エリア情報
typedef struct _AREA_INFO {
	UINT8	cnt;						// データ数
	UINT8	no[MP_AREA_INFO_CNT_MAX];	// エリア番号
} T_AREA_INFO;

// パーセルデータ構造体
typedef struct _PARCEL_DATA {
	UINT32	parcel_id;		// パーセルID
	INT32	level;			// レベル

	// 地図解析クラス
	SMRoadShapeAnalyze*	p_RoadShapeAnalyze;
	SMBkgdAnalyze*		p_BkgdAnalyze;
	SMNameAnalyze*		p_NameAnalyze;
	SMMarkAnalyze*		p_MarkAnalyze;
	SMRoadNameAnalyze*	p_RoadNameAnalyze;

	// 地図データ格納領域
	T_BIN_DATA			RoadShape;	// 道路形状
	T_BIN_DATA			Bkgd;		// 背景
	T_BIN_DATA			Name;		// 名称
	T_BIN_DATA			Mark;		// 記号背景
	T_BIN_DATA			RoadName;	// 道路名称
	T_BIN_DATA			Basis;		// パーセル基本情報
	UINT32* 			pRdBaseVer;	// 道路系ベースバージョン

	// 海陸フラグ
	UINT8				bkgdBaseFlg;

	// エリア情報
	T_AREA_INFO			areaInfo;

	// パーセル情報
	PARCEL_INFO_t		ParcelInfo;

	// 名称ビットマップフォント
	//UINT32				bitMapFontCnt;
	//T_BITMAPFONT_DATA	bitMapFont[MP_NAME_CNT_MAX];
	MP_FontMng			*pNameFontMng;

	// 道路名称情報
	UINT32				roadNameInfoCnt;
	T_ROAD_NAME_INFO	roadNameInfo[MP_ROAD_NAME_CNT_MAX];
	MP_FontMng			*pRoadNameFontMng;

	// 背景VBO情報
	T_VBO_INFO_LIST		bkgdVBO[MP_BKGD_KIND_CODE_TYPE_MAX];

	// 路線番号衝突判定
	MP_CollisionMesh	*pRoadNoCollisionMesh;
} T_PARCEL_DATA;

#ifdef __cplusplus
extern "C" {
#endif

E_SC_RESULT DrawMapInitialize(void);
E_SC_RESULT DrawMapFinalize(Bool vboNoDeleteFlg);
E_SC_RESULT DrawMap(void);

#ifdef __cplusplus
}	// end of the 'extern "C"' block
#endif
