/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef _MP_DEF_H
#define _MP_DEF_H

// 無効値
#define MP_INVALID_VALUE_32		0xFFFFFFFF
#define MP_INVALID_VALUE_16		0xFFFF
#define MP_INVALID_VALUE_8		0xFF

// 文字列最大値
#define MP_STRING_MAX_LEN		256

// 読み込みパーセル数
#define MP_READ_PARCEL_CNT_1	1
#define MP_READ_PARCEL_CNT_4	4
#define MP_READ_PARCEL_CNT_9	9
#define MP_READ_PARCEL_CNT_25	25

// キャッシュパーセル数
#define MP_CACHE_PARCEL_CNT		25
// ポリライン数
#define MP_POLYLINE_CNT				100000
// リンク補間点数
#define MP_LINK_POINT_CNT			2000
// 背景ポリゴン数
#define MP_BKGD_POLYGON_POINT_CNT	10000
// 頂点バッファ最大値(xy別)
#define MP_VERTEX_BUFFER_MAX		30000
// 三角形頂点数
#define MP_TRIANGLE_CNT				3
//// 名称管理最大数
//#define MP_NAME_CNT_MAX				2048
// 道路名称管理最大数
#define MP_ROAD_NAME_CNT_MAX		512

// VBO管理数
#define MP_VBO_CNT_MAX				32

// 地図正規化座標
#define MP_MAP_SIZE				4096
#define MP_MAP_SIZE_HALF		MP_MAP_SIZE/2

// 時間
#define MP_SEC_3600				3600.0

// 角度
#define MP_ANGLE_360			360
#define MP_ANGLE_270			270
#define MP_ANGLE_180			180
#define MP_ANGLE_90				90
#define MP_ANGLE_0				0

// 方向
#define MP_DIR_R_T				0	// 右上
#define MP_DIR_R_B				1	// 右下
#define MP_DIR_L_T				2	// 左上
#define MP_DIR_L_B				3	// 左下
// 円周率
#define MP_PI					3.141592653589793238462

#define PATH_SIZE				256
#define ICON_CNT_MAX			2048

// 縮尺レベル
#define MP_SCALE_LEVEL_MIN		0
#define MP_SCALE_LEVEL_MAX		13
#define MP_SCALE_LEVEL_CNT		MP_SCALE_LEVEL_MAX + 1

// ズームレベル
#define MP_ZOOM_LEVEL_CNT_MAX	20

// レベル
enum {
	MP_LEVEL_1 = 1, MP_LEVEL_2, MP_LEVEL_3, MP_LEVEL_4, MP_LEVEL_5, MP_LEVEL_6
};

// エリア情報最大値
#define MP_AREA_INFO_CNT_MAX	8

// 地図収録範囲デフォルト
#define MP_DEFAULT_MAP_RANGE_T_LAT	(2880 * 60)	// 上端緯度
#define MP_DEFAULT_MAP_RANGE_B_LAT	(1279 * 60)	// 下端緯度
#define MP_DEFAULT_MAP_RANGE_L_LON	(6960 * 60)	// 左端経度
#define MP_DEFAULT_MAP_RANGE_R_LON	(9360 * 60)	// 右端経度

// 言語種別コード
typedef enum _MP_LANG_CODE {
	MP_LANG_CODE_0 = 0,	// （RESERVED）
	MP_LANG_CODE_JPN,	// 日本語

	MP_LANG_CODE_MAX
} MP_LANG_CODE_e;

// 道路種別コード
typedef enum _MP_ROAD_KIND_CODE {
	MP_ROAD_KIND_CODE_0 = 0,
	MP_ROAD_KIND_CODE_1,
	MP_ROAD_KIND_CODE_2,
	MP_ROAD_KIND_CODE_3,
	MP_ROAD_KIND_CODE_4,
	MP_ROAD_KIND_CODE_5,
	MP_ROAD_KIND_CODE_6,
	MP_ROAD_KIND_CODE_7,
	MP_ROAD_KIND_CODE_8,
	MP_ROAD_KIND_CODE_9,
	MP_ROAD_KIND_CODE_10,
	MP_ROAD_KIND_CODE_11,
	MP_ROAD_KIND_CODE_12,
	MP_ROAD_KIND_CODE_13,
	MP_ROAD_KIND_CODE_14,
	MP_ROAD_KIND_CODE_15,

	MP_ROAD_KIND_CODE_MAX
} MP_ROAD_KIND_CODE_e;

// リンク種別１
typedef enum _MP_LINK_KIND1 {
	MP_LINK_KIND1_SEPARATE_MAIN = 0,			// 0  上下線非分離（本線）リンク
	MP_LINK_KIND1_NONSEPARATE_MAIN,			// 1  上下線分離（本線）リンク
	MP_LINK_KIND1_CONNECT1,						// 2  連結路リンク1（本線間の渡り線）
	MP_LINK_KIND1_CONNECT2,						// 3  連結路リンク2（ランプ）
	MP_LINK_KIND1_SIDE,							// 4  本線と同一路線の測道
	MP_LINK_KIND1_FRONTAGE,						// 5  Frontage Road
	MP_LINK_KIND1_SA,							// 6  SA等側線リンク
	MP_LINK_KIND1_ROTARY,						// 7  Rotary（Roundabout）リンク
	MP_LINK_KIND1_CIRCLE,						// 8  Circleリンク
	MP_LINK_KIND1_UNDEFINED_TRAFFIC_AREA,		// 9  敷地内リンク
	MP_LINK_KIND1_SMART_IC,						// 10 スマートICリンク
	MP_LINK_KIND1_PARKING,						// 11 駐車場接続リンク
	MP_LINK_KIND1_SLOPE,						// 12 スロープリンク
} _MP_LINK_KIND1_e;
// リンク種別２
// リンク種別３
// リンク種別４

// (BKGD)背景種別コード 分類ｺｰﾄﾞ
typedef enum _MP_BKGD_KIND_CODE_TYPE {
	MP_BKGD_KIND_CODE_TYPE_SURFACE = 0,			// 地表面(面)
	MP_BKGD_KIND_CODE_TYPE_CONTOUR,				// 等高線(面)
	MP_BKGD_KIND_CODE_TYPE_SECTION,				// 大字界面
	MP_BKGD_KIND_CODE_TYPE_SITE,				// 敷地(面)
	MP_BKGD_KIND_CODE_TYPE_WATER,				// 水系(面,線)
	MP_BKGD_KIND_CODE_TYPE_PARKING,				// 駐車場(面)
	MP_BKGD_KIND_CODE_TYPE_BRIDGE_GIRDER,		// 橋桁(面)
	MP_BKGD_KIND_CODE_TYPE_ROAD,				// 道路(面)
	MP_BKGD_KIND_CODE_TYPE_ROAD_LINE,			// 道路(線)
	MP_BKGD_KIND_CODE_TYPE_RAIL_LINE,			// 路線(線)
	MP_BKGD_KIND_CODE_TYPE_BORDER_LINE,			// 境界(線)
	MP_BKGD_KIND_CODE_TYPE_BUILDING,			// 建物(面,線)

	MP_BKGD_KIND_CODE_TYPE_MAX
} MP_BKGD_KIND_CODE_TYPE_e;

#define MP_BKGD_KIND_CODE_KIND_MAX	256						// (BKGD)背景種別コード 種別ｺｰﾄﾞ
#define MP_MARK_KIND_CODE_TYPE_MAX	6001					// (MARK)背景種別コード 分類ｺｰﾄﾞ
#define MP_MARK_KIND_CODE_KIND_MAX	6074					// (MARK)背景種別コード 種別ｺｰﾄﾞ
#define MP_NAME_KIND_CODE_TYPE_MAX	3						// (NAME)表示名称種別コード 分類ｺｰﾄﾞ
#define MP_NAME_KIND_CODE_KIND_MAX	9014					// (NAME)表示名称種別コード 種別ｺｰﾄﾞ
// 地表タイプ
#define MP_SURFACE_TYPE_LAND	0		// 陸
#define MP_SURFACE_TYPE_SEA		1		// 海
#define MP_SURFACE_TYPE_NO_DL	2		// 地図未ダウンロード
// 図形タイプ
#define MP_SHAPE_TYPE_POINT		0		// 点
#define MP_SHAPE_TYPE_LINE		1		// 線
#define MP_SHAPE_TYPE_POLYGON	2		// 面
#define MP_SHAPE_TYPE_NOPOLYGON	3		// 面(図形データ無し)
// 文字列タイプ
#define MP_STR_TYPE_CENTER		1		// 重心
#define MP_STR_TYPE_POINT		2		// ポイント指示
#define MP_STR_TYPE_POINT_ANGLE	3		// 角度付きポイント指示
// データ対応
#define MP_1BYTE				0		// 1バイトデータ
#define MP_2BYTE				1		// 2バイトデータ

// 交通情報
// 渋滞度
#define MP_CNGS_LVL_UNKNOWN		0		// 不明
#define MP_CNGS_LVL_0			1		// 渋滞なし
#define MP_CNGS_LVL_1			2		// 混雑
#define MP_CNGS_LVL_2			3		// 渋滞
#define MP_CNGS_LVL_3			4		// 大渋滞

// カラー
typedef unsigned int			RGBACOLOR;	// カラーRGBA値
#define SET_RGBA(r,g,b,a)		(RGBACOLOR)((BYTE)(r)|((UINT16)((BYTE)(g))<<8)|(((UINT32)(BYTE)(b))<<16)|(((UINT32)(BYTE)(a))<<24))
#define GET_R(rgba)				((BYTE)(rgba))
#define GET_G(rgba)				((BYTE)((rgba)>> 8))
#define GET_B(rgba)				((BYTE)((rgba)>>16))
#define GET_A(rgba)				((BYTE)((rgba)>>24))
#define GET_FR(rgba)			((FLOAT)((FLOAT)GET_R(rgba)/255.0f))
#define GET_FG(rgba)			((FLOAT)((FLOAT)GET_G(rgba)/255.0f))
#define GET_FB(rgba)			((FLOAT)((FLOAT)GET_B(rgba)/255.0f))
#define GET_FA(rgba)			((FLOAT)((FLOAT)GET_A(rgba)/255.0f))

#define MP_RGBA_NON				SET_RGBA(  0,  0,  0,  0)
#define MP_RGBA_BLACK			SET_RGBA(  0,  0,  0,255)
#define MP_RGBA_WHITE			SET_RGBA(255,255,255,255)
#define MP_RGBA_RED				SET_RGBA(255,  0,  0,255)
#define MP_RGBA_GREEN			SET_RGBA(  0,255,  0,255)
#define MP_RGBA_BLUE			SET_RGBA(  0,  0,255,255)
#define MP_RGBA_YELLOW			SET_RGBA(255,255,  0,255)

// オフセット
#define MP_OFFSET_CENTER			0
#define MP_OFFSET_TOP				1
#define MP_OFFSET_RIGHT				2
#define MP_OFFSET_BOTTOM			3
#define MP_OFFSET_LEFT				4

// デフォルトアイコンサイズ
#define MP_DEFAULT_ICON_SIZE		32
#define MP_DEFAULT_ICON_HALF_SIZE	MP_DEFAULT_ICON_SIZE/2

// 道路名称サイズ
#define MP_ROAD_NAME_MAX_SIZE		256

// 4 bytes alignment
#define MP_ALIGNMENT4(_x)			(((_x)+3) & ~3)

// 角度 → ラジアンへ変換
#define MP_DEG_TO_RAD(_deg)			(FLOAT)((_deg) * M_PI / (FLOAT)MP_ANGLE_180)
// ラジアン → 角度へ変換
#define MP_RAD_TO_DEG(_rad)			(FLOAT)((_rad) * (FLOAT)MP_ANGLE_180 / M_PI)
// 角度を東方向0°反時計回り → 北方向0°時計回りに変換
#define MP_E0CCW_TO_N0CW(_deg)		(FLOAT)(90.f>=(_deg) ? (90.f-(_deg)) : (450.f-(_deg)))

#endif // _MP_DEF_H
