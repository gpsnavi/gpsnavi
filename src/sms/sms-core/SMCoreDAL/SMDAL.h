/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef _SMDAL_H_
#define _SMDAL_H_

//-----------------------------------
// 定数定義
//-----------------------------------
#define SC_DA_PATH_LEN				(256)				// 地図パス 文字列長(バイト数)
// 地図基本情報構造体用定数 (#SYSTEM_INFO)
#define	SC_DA_SYSTEMINFO_TEXT_LEN	(64)				// SYSTEMINFO
// ダウンロードエリア情報構造体用定数 (#DOWNLOAD_AREA_MAP)
#define	SC_DA_DOWN_KIND				(64)				// KIND
#define	SC_DA_DOWN_NAME				(64)				// AREA_NAME
#define	SC_DA_DOWN_NOTE				(128)				// NOTE
// 地域クラス名称情報構造体用定数 (#AREA_CLS)
#define	SC_DA_AREA_NAME				(128)				// AREA_NAME


//-----------------------------------
// 構造体定義
//-----------------------------------
// 地図取得IF用 パーセルID構造体
typedef struct {
	UINT32 parcelId;										// パーセルID
	UINT16 divideId;										// 分割識別子
} T_DAL_PID;

// 地図取得IF用 バイナリカラム情報
typedef struct _T_DAL_BINARY {
	UINT32 bufferSize;										// バイナリ読み込み用バッファのサイズ
	UINT8 * pBufferAddr;									// バッファの先頭アドレス
	UINT32 binDataSize;										// 読み込んだバイナリデータのサイズ
} T_DAL_BINARY;

// ダウンロードエリア情報構造体(#DOWNLOAD_AREA_MAP)
typedef struct _T_DAL_DLAREA {
	UINT8 id;												// ダウンロード管理番号
	Char kind[SC_DA_DOWN_KIND];								// データ種別
	UINT16 countryCode;										// 国識別コード
	UINT16 displayNum;										// 表示順
	Char countryName[SC_DA_DOWN_NAME];						// 国名称
	Char areaGroup[SC_DA_DOWN_NAME];						// グループ名称
	Char areaName[SC_DA_DOWN_NAME];							// エリア名称
	UINT8 downloadFlag;										// ダウンロードフラグ
	UINT32 baseVersion;										// ベースバージョン
	Char note[SC_DA_DOWN_NOTE];								// 備考
} T_DAL_DLAREA;

// 地図基本情報構造体(#SYSTEM_INFO)
typedef struct _T_DAL_DBSYSTEMINFO {
	Char system_format_version[SC_DA_SYSTEMINFO_TEXT_LEN];	//フォーマットバージョン。
	Char system_machine_type[SC_DA_SYSTEMINFO_TEXT_LEN];	//車載機の種類を示す文字列
	Char system_app_version[SC_DA_SYSTEMINFO_TEXT_LEN];		//ナビアプリケーションプログラムのバージョンを識別する文字列。
	Char system_area[3];									//収録地域を示す。
	Char system_lang[SC_DA_SYSTEMINFO_TEXT_LEN];			//格納する言語の種別を示す。
	Char system_level_01[20];								//データの種類ごとにどのレベルで構成されているかを示す。
	Char system_level_02[20];
	Char system_level_03[20];
	Char system_level_04[20];
	Char system_level_05[20];
	Char system_level_06[20];
	Char system_level_07[20];
	Char system_level_08[20];
	Char system_data_provider[SC_DA_SYSTEMINFO_TEXT_LEN];	//地図データの作成者を表す文字列。
	Char system_character_code[SC_DA_SYSTEMINFO_TEXT_LEN];	//格納する文字コードを表す文字列。
	Char system_map_range[20];								//地図の収録範囲を示す。
	Char system_initial_posi[SC_DA_SYSTEMINFO_TEXT_LEN];	//初期位置を示す。
	Char system_map_coordinate[6];							//地図の正規化座標範囲を表す文字列。
	Char system_map_ver[SC_DA_SYSTEMINFO_TEXT_LEN];			//地図バージョン文字列を示す。
	Char system_map_ver_no[9];								//地図のデータバ-ジョンを示す。
	Char system_map_build_no[6];							//地図のビルド番号を示す。
	Char osm_original_timestamp[21];						//OSMデータのXMLヘッダに記述されているtimestamp情報
	Char system_sea_flag[2];								//地図の収録範囲内であるが、収録されていないパーセルの背景描画方法を示す。
} T_DAL_DBSYSTEMINFO;

// 地域クラス名称情報構造体
typedef struct _T_DAL_AREA_CLS {
	Char name[SC_DA_AREA_NAME];			// 地域クラス名称
	Char yomi[SC_DA_AREA_NAME];			// 地域クラス名称読み
} T_DAL_AREA_CLS;
// 地域クラス情報構造体
typedef struct _T_DAL_AREA_CLS_LIST {
	T_DAL_AREA_CLS areaCls[e_AREA_CLS_MAX];
} T_DAL_AREA_CLS_LIST;

#endif /* _SMDAL_H_ */
