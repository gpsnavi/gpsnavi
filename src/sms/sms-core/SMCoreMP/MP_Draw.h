/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef _MP_DRAW_H
#define _MP_DRAW_H

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
// 構造体
//------------------------------------------------------------------------------
typedef struct _IDO_KEIDO {
	DOUBLE la;	// latitude 緯度
	DOUBLE lo;	// longitude 経度
} IDO_KEIDO_t;

typedef struct _PIXEL_COORD {
	INT32 x;
	INT32 y;
} PIXEL_COORD_t;

/*!
	\brief 表示情報
*/
typedef struct _VIEW_INFO
{
	// 描画管理データ
	INT32		origin_x;		// 画面幅
	INT32		origin_y;		// 画面高さ
	INT32		origin_half_x;	// 画面幅(半分)
	INT32		origin_half_y;	// 画面高さ(半分)
	INT32		dispRengXMin;	// 表示領域X
	INT32		dispRengXMax;	// 表示領域X
	INT32		dispRengYMin;	// 表示領域Y
	INT32		dispRengYMax;	// 表示領域Y
	IDO_KEIDO_t	center_point;	// 中心緯度経度(xx.xxxx)
	PIXEL_COORD_t pixelCoord;	// 中心ピクセル座標
//	INT32		center_x;		// タイル内中心座標X
//	INT32		center_y;		// タイル内中心座標Y
	INT32		zoom_level;		// タイルズームレベル
	INT32		level;			// 地図レベル
	FLOAT		car_angle;		// 自車角度(北0度 時計回り)
	FLOAT		disp_angle;		// 表示角度(北0度 時計回り)
	FLOAT		rotate;			// 地図回転角度(北0度 時計回り)
	FLOAT		scale;			// 地図倍率
	INT32		mapRange[4];	// 収録範囲(上端緯度、下端緯度、左端経度、右端経度)
	INT32		sea_flg;		// 収録されていないパーセルの背景描画方法

	// 共有データ
	SMRECT		rect;			// ビューポート
	INT32		disp_mode;		// 地図表示モード
	Bool		driver_mode;	// ドライバーモード
	Bool		scroll_mode;	// スクロールモード
	Bool		zoom_mode;	// フリーズームモード
	Bool		route;			// ルート有無
	SMCARSTATE	car_state;		// 車両状態情報
	SMGEOCOORD	geo_coord;		// 地図中心地理座標
	INT32		scale_level;	// 表示縮尺
	Bool		debug_flg;		// デバッグフラグ
	SMTRAFFIC	trfInfo;		// 交通情報

	SMMAPDYNUDI	icon_info[ICON_CNT_MAX];	// ユーザ定義ダイナミックアイコンデータ情報
	INT32		icon_num;					// ユーザ定義ダイナミックアイコンデータ情報数
	Bool		disp_info[ICON_CNT_MAX];	// ユーザ定義ダイナミックアイコンデータの表示/非表示
	INT32		disp_num;					// ユーザ定義ダイナミックアイコンデータの表示/非表示数

	// ダウンロードエリア情報(常駐)
	T_DHC_DOWNLOAD_AREA_NAME downLoadAreaName;	// ダウンロードエリア名称情報

	// 未ダウンロードエリア情報(1フレーム描画毎に設定)
	Bool		noDLFlg;							// 地図未ダウンロードフラグ(未:true, 済:false)
	UINT8		noDLAreaCnt;						// 地図未ダウンロードエリア数
	UINT8		noDLAreaNo[MP_AREA_INFO_CNT_MAX];	// 地図未ダウンロードエリアNO

	// マッピングアラート情報
	Bool			mapping_alert_disp;		// マッピングアラート表示フラグ
	SMMAPPINGALERT	mapping_alert;			// マッピングアラート情報
} T_VIEW_INFO;

// パーセル情報
typedef struct _PARCEL_INFO {
	UINT32	parcel_id;	// パーセルID
	UINT32	level;		// レベル

	// 座標係数
	DOUBLE	pixX;		// 左下ピクセル座標
	DOUBLE	pixY;		// 左下ピクセル座標
	DOUBLE	keisuuX;
	DOUBLE	keisuuY;

	// パーセル内描画領域
	FLOAT	minX;
	FLOAT	minY;
	FLOAT	maxX;
	FLOAT	maxY;

	// パーセルのX方向を1とした時のY方向の比率
	FLOAT	yRatio;
} PARCEL_INFO_t;


//-----------------------------------------------------------------------------
// プロトタイプ宣言
//-----------------------------------------------------------------------------
void MP_InitViewInfo(void);
T_VIEW_INFO* MP_GetViewInfo(void);
void MP_DRAW_SetVeiwInfo(INT32 maps);
Bool MP_DRAW_GetPixelPos(DOUBLE lat, DOUBLE lon, FLOAT* p_x, FLOAT* p_y);

E_SC_RESULT MP_DRAW_Route(void);
E_SC_RESULT MP_DRAW_RoutePoint(void);

void MP_DRAW_ChangeFromDispToLatLon(const INT32 x, const INT32 y, DOUBLE *pLat, DOUBLE *pLon);
void MP_DRAW_GetParcelInfo(const UINT32 parcelID, PARCEL_INFO_t* pParcelInfo);
Bool MP_DRAW_CheckDrawParcel(PARCEL_INFO_t* pParcelInfo);
Bool MP_DRAW_CheckMapRange(DOUBLE* latitude, DOUBLE* longitude);

#ifdef __cplusplus
}
#endif

#endif	// _MP_DRAW_H
