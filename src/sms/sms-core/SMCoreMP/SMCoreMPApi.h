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
 * SMCoreMPApi.h
 *
 *  Created on: 2015/11/06
 *      Author: n.kanagawa
 */

#ifndef SMCOREMPAPI_H_
#define SMCOREMPAPI_H_


/**
 * ビットマップフォント情報
 */
/*typedef struct {
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
} SMMPBITMAPFONTINFO;*/

/**
 * ビットマップ情報
 */
/*typedef struct {
	// IN
	char	*path;			// ファイルパス

	// OUT
	UChar	*pBitMap;		// ビットマップポインタ
	INT32	width;			// ビットマップ幅
	INT32	height;			// ビットマップ高
} SMMPBITMAPINFO;*/

/* ビットマップフォント生成関数ポインタ */
//typedef E_SC_RESULT (*MP_BITMAPFONTFUNCPTR)(SMMPBITMAPFONTINFO* pInfo);
/* 画像ファイル読み込み関数ポインタ */
//typedef E_SC_RESULT (*MP_IMAGEFUNCPTR)(SMMPBITMAPINFO* pInfo);


#ifdef __cplusplus
extern "C" {
#endif
/**
 * SCMPThread
 */
E_SC_RESULT SC_MP_Initialize();
E_SC_RESULT SC_MP_Finalize();
void *SC_MP_ThreadMain(void *param);


/**
 * SCMPDraw
 */
/* 描画初期化 */
E_SC_RESULT SC_DRAW_Initialize(void);		// SC_MP_Initialize()内でコール
/* 描画終了処理 */
E_SC_RESULT SC_DRAW_Finalize(void);			// SC_MP_Finalize()内でコール
/* リソース初期化 */
E_SC_RESULT SC_DRAW_InitResource(INT32 maps);
/* ビューポート設定 */
E_SC_RESULT SC_DRAW_SetViewport(INT32 maps, const SMRECT* rect);
/* 描画更新 */
E_SC_RESULT SC_DRAW_Refresh(INT32 maps);
/* フリーズーム時の地図拡大比例を設定 */
E_SC_RESULT SC_DRAW_SetZoomStepRate(INT32 maps);
/*  地図の移動情報(移動角度と移動長さ)を設定 */
E_SC_RESULT SC_DRAW_MoveMapDir(INT32 maps);
/*  地図を全景表示 */
E_SC_RESULT SC_DRAW_OverviewMap(INT32 maps, INT32 overviewObj, SMRECT* pRect);
/*  スケール毎の縮尺率取得 */
FLOAT SC_DRAW_GetScaleRange(INT32 scaleLevel);
/*  表示中地図のズームレベル取得 */
INT8 SC_DRAW_GetZoomLevel(INT32 scaleLevel);
/*  スクリーン座標から緯度経度取得 */
E_SC_RESULT SC_DRAW_ScreenToGeoCode(INT32 maps, INT32 screenX, INT32 screenY, SMGEOCOORD* pGeoCoord);
/*  ビットマップフォントCB設定 */
E_SC_RESULT SC_DRAW_SetBitmapFontCB(NC_BITMAPFONTFUNCPTR pfunc);
/*  画像読み込みCB設定 */
E_SC_RESULT SC_DRAW_SetImageReadForFileCB(NC_IMAGEFUNCPTR pfunc);
/*  画像読み込みCB設定 */
E_SC_RESULT SC_DRAW_SetImageReadForImageCB(NC_IMAGEFUNCPTR pfunc);
#ifdef __cplusplus
}
#endif

#endif /* SMCOREMPAPI_H_ */
