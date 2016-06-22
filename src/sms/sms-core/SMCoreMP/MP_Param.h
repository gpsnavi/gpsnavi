/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef _MP_PARAM_H
#define _MP_PARAM_H

#ifdef __cplusplus
extern "C" {
#endif

typedef UINT32	DRAWPARAM;	// 描画パラメータ

// ライン種別
#define MP_LINE_NMR				0	// 線
#define MP_LINE_PLG				1	// 線(ポリゴン)
#define MP_LINE_DOT				2	// 破線

// 設定有無
#define MP_OFF					0	// 無
#define MP_ON					1	// 有


/**
 * @brief  縮尺パラメータ
 */
typedef struct _SCALE_PARAM
{
	INT8	zoomLevel;		// ズームレベル
	INT8	parcelLevel;	// パーセルレベル
	UINT16	zoomFlg;		// ズーム許可フラグ
	FLOAT	scale;			// スケール
	FLOAT	zoomIn;			// ズームイン
	FLOAT	zoomOut;		// ズームアウト
} SCALE_PARAM_t;

/**
 * @brief  描画パラメータ分割
 */
typedef union _DRAWPARAM_DIV {
	struct{
		UINT32	w					:  8;	// bit  7～ 0 - 線の太さ
		UINT32	wo					:  8;	// bit 15～ 8 - 縁の太さ
		UINT32	f					:  1;	// bit 16     - 縁の有無（0：無, 1：有）
		UINT32	e					:  1;	// bit 17     - 端の処理（0：無, 1：○）
		UINT32	l					:  1;	// bit 18     - 描画方法（0：ライン, 1：ポリライン）
		UINT32	reserve				: 13;	// bit 31～19 - 予約
	} b;
	UINT32 d;
} DRAWPARAM_DIV;

/**
 * @brief  カラー
 */
typedef struct _MP_COLOR
{
	RGBACOLOR	color;		// 色
	RGBACOLOR	colorOut;	// 縁の色
} MP_COLOR_t;

/**
 * @brief  パラメータ
 */
typedef struct _MP_PARAM
{
	DRAWPARAM	param;		// パラメータ
} MP_PARAM_t;

/**
 * @brief  フォント
 */
typedef struct _MP_FONT
{
	RGBACOLOR	color;			// 色
	RGBACOLOR	outLineColor;	// 縁色
	RGBACOLOR	bkgdColor;		// 背景色
	UINT32		size;			// サイズ
	UINT8		outLineFlg;		// 1縁有、0縁無
	UINT8		offset;			// 表示位置オフセット
} MP_FONT_t;


// マクロ
// パラメータ(w:サイズ, wo:縁サイズ. f:縁の有無, e:端の処理, l:描画方法)
#define SET_LINE_PARAM(w,wo,f,e,l)	((DRAWPARAM)(((UINT32)w)|(((UINT32)wo)<<8)|(((UINT32)f)<<16)|(((UINT32)e)<<17)|(((UINT32)l)<<18)))
#define SET_POLYGON_PARAM(w,wo)		((DRAWPARAM)(((UINT32)w)|(((UINT32)wo)<<8)))
#define SET_NAME_PARAM(w,wo)		((DRAWPARAM)(((UINT32)w)|(((UINT32)wo)<<8)))
#define GET_WIDTH(param)			((BYTE)((param&0x000000FF)    ))	// 線の太さ
#define GET_WIDTH_OUT(param)		((BYTE)((param&0x0000FF00)>> 8))	// 縁の太さ
#define GET_OUTFLG(param)			((BYTE)((param&0x00010000)>>16))	// 縁の有無
#define GET_EDGE(param)				((BYTE)((param&0x00020000)>>17))	// 端の処理
#define GET_LINE(param)				((BYTE)((param&0x000C0000)>>18))	// 描画方法
#define GET_DOT(param)				((BYTE)((param&0x0FF00000)>>20))	// ドット幅
#define SET_DOT_LINE_PARAM(w,wo,f,e,l,d)	(SET_LINE_PARAM(w,wo,f,e,l)|(((UINT32)d)<<20))

// 列挙型
// アイコンタイプ
typedef enum _MP_ICON_TYPE
{
	MP_ICON_TYPE_CAR = 0,		// カーマーク
	MP_ICON_TYPE_START,			// スタート(出発値)
	MP_ICON_TYPE_DEST,			// ゴール(目的地)
	MP_ICON_TYPE_TRANSIT,		// 経由地
	MP_ICON_TYPE_CURSOR,		// カーソル
	MP_ICON_TYPE_MAX
} MP_ICON_TYPE_e;

// 初期化
Bool Param_Initialize(void);

// 縮尺パラメータ
INT8 ParamScale_ZoomLevel(UINT16 scale);
INT8 ParamScale_ParcelLevel(UINT16 scale);
UINT16 ParamScale_ZoomFlg(UINT16 scale);
UINT16 ParamScale_ZoomToScale(INT8 zoomLevel);
FLOAT ParamScale_Scale(UINT16 scale);
FLOAT ParamScale_ZoomIn(UINT16 scale);
FLOAT ParamScale_ZoomOut(UINT16 scale);

// 道路パラメータ
RGBACOLOR ParamRoad_LineColor(UINT16 roadKind);
RGBACOLOR ParamRoad_OLineColor(UINT16 roadKind);
DRAWPARAM ParamRoad_Param(UINT16 scale, UINT16 roadKind);

// 背景パラメータ
RGBACOLOR ParamBkgd_BaseColor(UINT16 type);
RGBACOLOR ParamBkgd_Color(UINT16 bkgdType, UINT16 bkgdKind);
DRAWPARAM ParamBkgd_Param(UINT16 scale, UINT16 bkgdType);
DRAWPARAM ParamBkgdLine_Param(UINT16 scale, UINT16 bkgdType, UINT16 bkgdKind);
Bool ParamBkgd_NonDisplay(UINT16 scale, UINT16 bkgdType, UINT16 bkgdKind, UINT32 figureType);
Bool ParamBkgd_NonDisplayMove(UINT16 scale, UINT16 bkgdType, UINT16 bkgdKind);

// 名称パラメータ
RGBACOLOR ParamName_Color(UINT16 nameType, UINT16 nameKind);
RGBACOLOR ParamName_OutLineColor(UINT16 nameType, UINT16 nameKind);
RGBACOLOR ParamName_BkgdColor(UINT16 nameType, UINT16 nameKind);
FLOAT ParamName_FontSize(UINT16 scale, UINT16 nameType, UINT16 nameKind);
INT32 ParamName_OutLine(UINT16 nameType, UINT16 nameKind);
INT32 ParamName_Offset(UINT16 nameType, UINT16 nameKind);

// 記号背景パラメータ
UINT32 ParamMark_MapSymbol(UINT16 markType, UINT16 markKind);

// 道路名称パラメータ
RGBACOLOR ParamRoadName_Color(UINT16 roadKind);
RGBACOLOR ParamRoadName_OutLineColor(UINT16 roadKind);
FLOAT ParamRoadName_FontSize(UINT16 roadKind);
FLOAT ParamRoadName_RoadNoFontSize(UINT16 roadKind);
UINT32 ParamRoadName_DivCnt(FLOAT scale);
UINT32 ParamRoadName_IconID(UINT16 roadKind);

// 経路パラメータ
RGBACOLOR ParamRoute_Color(void);
RGBACOLOR ParamRoute_SplitLinkColor(void);
UINT32 ParamRoute_Size(UINT16 scale);
UINT32 ParamRoute_SplitLinkSize(UINT16 scale);

// アイコンパラメータ
UINT32 ParamIcon_IconID(MP_ICON_TYPE_e iconType);

// 渋滞情報パラメータ
RGBACOLOR ParamCongestion_Color(UINT16 cngsLvl);
FLOAT ParamCongestion_LineSize(UINT16 scaleLevel, UINT16 roadKind);
FLOAT ParamCongestion_OutLineSize(UINT16 scaleLevel, UINT16 roadKind);


#ifdef __cplusplus
}
#endif

#endif	// _MP_PARAM_H
