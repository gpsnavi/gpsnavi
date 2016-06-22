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
 *  Created on: 2016/05/06
 *      Author:
 */

#ifndef NAVICOREDEFEX_H_
#define NAVICOREDEFEX_H_

#define NC_MP_MAP_MAIN	(1)		/* 暫定的に定義する By T.Aikawa */

// ロケーション
typedef enum _SC_CARLOCATION_TYPE {
	e_SC_CARLOCATION_NOW = 0,
	e_SC_CARLOCATION_REAL,			// 実ロケーション
	e_SC_CARLOCATION_SIMU			// シミュレートロケーション
} E_SC_CARLOCATION_TYPE;

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

// ユーザ定義アイコンの構造体
typedef struct _SMMAPDYNUDI {
	INT32 Longititude;					// 経度座標、値範囲は383385600～619315200
	INT32 Latitude;						// 緯度座標、値範囲は44236800～201523200
	INT32 IconID;						// アイコンID
} SMMAPDYNUDI;

/**
 * 描画終了情報
 */
typedef struct {
	INT32		maps;
	FLOAT		rotate;			// 地図回転角度(北0度 時計回り)
} NCDRAWENDINFO;
/* 描画終了時のHMI側処理 */
typedef INT32 (*NC_DRAWENDINFOFUNCPTR)(NCDRAWENDINFO* pInfo);

INT32 NC_DM_GetCarState(SMCARSTATE *carState, INT32 mode);
INT32 NC_DM_SetIconInfo(const SMMAPDYNUDI *iconInfo, INT32 iconNum);
INT32 NC_DM_SetDynamicUDIDisplay(const Bool *dispInfo, INT32 dispNum);
INT32 NC_MP_ScreenToGeoCode(INT32 maps, INT32 screenX, INT32 screenY, SMGEOCOORD* pGeoCoord);
INT32 NC_MP_SetMapDrawEndCB(NC_DRAWENDINFOFUNCPTR pfunc);

#endif /* NAVICOREDEFEX_H_ */

