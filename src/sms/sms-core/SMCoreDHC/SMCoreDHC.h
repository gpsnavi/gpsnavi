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
 * SMCoreDHC.h
 *
 *  Created on: 2014/06/19
 *      Author: 70251034
 */

#ifndef SMCOREDHC_H_
#define SMCOREDHC_H_

// 注：ユーザが増えた場合は追加すること
#define SC_DHC_USER_MAP			(0x0001)						// ユーザ：MAP
#define SC_DHC_USER_RP			(0x0002)						// ユーザ：RP
#define SC_DHC_USER_RG			(0x0004)						// ユーザ：RG
#define SC_DHC_USER_LC			(0x0008)						// ユーザ：LC
#define SC_DHC_USER_PI			(0x0010)						// ユーザ：PI
#define SC_DHC_USER_DH			(0x0020)						// ユーザ：DH

#define SC_DHC_MAPFREE_USER		(0)								// 地図解放タイプ：ユーザ
#define SC_DHC_MAPFREE_KIND		(1)								// 地図解放タイプ：地図種別

#define SC_DHC_CROSS_AREA_VOL	(8)								// 1パーセル(レベル1)内に存在する、エリア情報の最大数

#define M_DHC_DOWNLOAD_AREA_MAX		(251)						// ダウンロードエリアの最大数
#define M_DHC_DOWNLOAD_AREA_PARCEL	(0)							// パーセル値
#define M_DHC_DOWNLOAD_AREA_TRAFFIC	(1)							// 交通情報

#define M_DHC_DOWNLOAD_AREA_OFF		(0)							// ダウンロードされていない
#define M_DHC_DOWNLOAD_AREA_ON		(1)							// ダウンロードされている
#define M_DHC_DOWNLOAD_AREA_DEL		(2)							// ダウンロードされていたが削除された

#define M_DHC_DOWNLOAD_AREA_NAME_LEN_MAX	(64)				// ダウンロードエリア名称最大値

// データ種別(ダウンロード管理テーブル)
#define M_DHC_DLAREA_KIND_PARCEL	"PARCEL"					// パーセル
#define M_DHC_DLAREA_KIND_TRAFFIC	"TRAFFIC"					// 交通情報

#define SC_DHC_MAX_PARCEL_VOL		(256)						// 1度の地図データ要求で指定可能なパーセル数

// 注：テーブル(PARCEL)に格納されるデータ種別が増えた場合は追加すること
//     並び順はE_DHC_DATA_KINDに合わせること
#define SC_DHC_KIND_ROAD			(0x80000000)				// データ種別：ネットワーク
#define SC_DHC_KIND_SHAPE			(0x40000000)				// データ種別：形状
#define SC_DHC_KIND_GUIDE			(0x20000000)				// データ種別：誘導
#define SC_DHC_KIND_BKGD			(0x10000000)				// データ種別：背景
#define SC_DHC_KIND_NAME			(0x08000000)				// データ種別：表示名称
#define SC_DHC_KIND_ROAD_NAME		(0x04000000)				// データ種別：道路名称
#define SC_DHC_KIND_DENSITY			(0x00800000)				// データ種別：道路密度
#define SC_DHC_KIND_MARK			(0x00400000)				// データ種別：記号背景
#define SC_DHC_KIND_PARCEL_BASIS	(0x00200000)				// データ種別：パーセル基本情報
#define SC_DHC_KIND_ROAD_BASE_VER	(0x00100000)				// データ種別：道路系ベースバージョン
#define SC_DHC_KIND_BKGD_AREA_CLS	(0x00080000)				// データ種別：地域クラス背景

typedef enum _E_DHC_DATA_KIND {
	e_DATA_KIND_ROAD = 0,				// 道路データ
	e_DATA_KIND_SHAPE,					// 形状データ
	e_DATA_KIND_GUIDE,					// 誘導データ
	e_DATA_KIND_BKGD,					// 背景データ
	e_DATA_KIND_NAME,					// 名称データ
	e_DATA_KIND_ROAD_NAME,				// 道路名称データ
	e_DATA_KIND_BKGD_NAME,				// 背景名称データ
	e_DATA_KIND_CHARSTR,				// 文言データ
	e_DATA_KIND_DENSITY,				// 密度データ
	e_DATA_KIND_MARK,					// 記号背景データ
	e_DATA_KIND_PARCEL_BASIS,			// パーセル基本情報データ
	e_DATA_KIND_ROAD_BASE_VERSION,		// 道路系ベースバージョン
	e_DATA_KIND_BKGD_AREA_CLS,			// 地域クラス背景データ

	e_DATA_KIND_END,					// 終端
} E_DHC_DATA_KIND;

// 地図要求テーブル
typedef struct{
	UINT16	user;						// ユーザ
	UINT32	parcelId;					// パーセルID
	INT32	mapKind;					// 地図データ
}T_DHC_REQ_INFO;
// 道路密度データ
typedef struct {
	UINT8 density;						// パーセル別密度リスト
	UINT8 areaId[SC_DHC_CROSS_AREA_VOL];// パーセル別エリアリスト
} T_DHC_ROAD_DENSITY_DATA;
// 道路密度要求テーブル
typedef struct {
	UINT32 baseParcelId;				// パーセルID
	UINT16 x;							// X方向パーセル枚数
	UINT16 y;							// Y方向パーセル枚数
	UINT32 totalDensity;				// 総密度
	T_DHC_ROAD_DENSITY_DATA* data;		// パーセル別密度リスト
} T_DHC_ROAD_DENSITY;
// 地域クラスコード要求テーブル
typedef struct {
	UINT32 parcelId;					// パーセルID
	INT16 x;							// X座標
	INT16 y;							// Y座標
	SMAREACLSCODE areaClsCode;			// 地域クラスコード
} T_DHC_AREA_CLS_CODE;
// ダウンロード状況テーブル
typedef struct {
	UINT8 download_f;					// ダウンロードフラグ
} T_DHC_DOWNLOAD_AREA_DATA;
typedef struct {
	T_DHC_DOWNLOAD_AREA_DATA data[M_DHC_DOWNLOAD_AREA_MAX];	// ダウンロードエリアデータ
} T_DHC_DOWNLOAD_AREA;
// ダウンロードエリア名称テーブル
typedef struct {
	Char areaName[M_DHC_DOWNLOAD_AREA_NAME_LEN_MAX];		// エリア名称
} T_DHC_DOWNLOAD_AREA_NAME_DATA;
typedef struct {
	T_DHC_DOWNLOAD_AREA_NAME_DATA data[M_DHC_DOWNLOAD_AREA_MAX];	// ダウンロードエリア名称データ
} T_DHC_DOWNLOAD_AREA_NAME;
// パーセル&種別テーブル
typedef struct{
	UINT32	parcelId;					// パーセルID
	UINT32	mapKind;					// データ種別
}T_DHC_REQ_PARCEL_INFO;
// 地図要求テーブル
typedef struct{
	UINT16	user;						// ユーザ
	UINT16	parcelNum;					// パーセル数
	T_DHC_REQ_PARCEL_INFO	parcelInfo[SC_DHC_MAX_PARCEL_VOL];	// パーセル&種別情報
}T_DHC_REQ_PARCEL;
// 地図バイナリデータ
typedef struct{
	UINT32	parcelId;					// パーセルID
	void*	binRoad;					// 道路データ
	void*	binBkgd;					// 背景データ
	void*	binName;					// 名称データ
	void*	binRoadName;				// 道路名称データ
	void*	binGuide;					// 誘導データ
	void*	binShape;					// 形状データ
	void*	binDensity;					// 密度データ
	void*	binMark;					// 記号背景データ
	void*	binParcelBasis;				// パーセル基本情報データ
	void*	iRoadBaseVersion;			// 道路系ベースバージョン
	void*	binBkgdAreaCls;				// 地域クラス背景データ
}T_DHC_RES_PARCEL_BIN;
// 地図応答テーブル
typedef struct{
	UINT16	parcelNum;					// パーセル数
	T_DHC_RES_PARCEL_BIN	parcelBin[256];	// 地図バイナリデータ
}T_DHC_RES_DATA;

#endif /* SMCOREDHC_H_ */
