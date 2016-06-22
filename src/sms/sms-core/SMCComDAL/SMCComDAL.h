/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef _SMCCOM_DAL_H_
#define _SMCCOM_DAL_H_

//-----------------------------------
// 無効値定義
//-----------------------------------
#define	SCC_DAL_INVALID_VALUE_8					0xff
#define	SCC_DAL_INVALID_VALUE_16				0xffff
#define	SCC_DAL_INVALID_VALUE_32				0xffffffff

#define SCC_DAL_SQL_LEN							(512)					// SQL文 文字列長
#define SCC_DAL_DBG_PATH_LEN					(256)					// スケジュールＤＢを格納しているフォルダ名称文字列長

#define	SCC_DAL_REGION_NAME						(196)
#define	SCC_DAL_SECTION_NAME					(196)
#define	SCC_DAL_AREA_NAME						(196)
#define	SCC_DAL_APP_VERSION						(32)
#define	SCC_DAL_DATA_KIND						(68)
#define	SCC_DAL_KIND64							(68)
#define	SCC_DAL_KIND32							(36)
#define	SCC_DAL_TABLE_NAME						(68)
#define	SCC_DAL_SYS_NAME						(132)
#define	SCC_DAL_CUSTOM_NAME						(132)
#define	SCC_DAL_CUSTOM_VERSION					(68)

// データ種別
#define	SCC_DAL_KIND_BASE						"BASE"					// パーセルテーブル
#define	SCC_DAL_KIND_MGR						"MGR"					// 管理系テーブル
#define	SCC_DAL_KIND_PARCEL						"PARCEL"				// パーセルテーブル
#define	SCC_DAL_KIND_AREA_CLS					"AREA_CLS"				// 地域クラステーブル
#define	SCC_DAL_KIND_SYSTEM_INFORMATION			"SYSTEM_INFORMATION"	// 地図情報管理テーブル
#define	SCC_DAL_KIND_UPDATE_STATUS				"UPDATE_STATUS"			// 更新ステータス管理テーブル
#define	SCC_DAL_KIND_DOWNLOAD_AREA_MAP			"DOWNLOAD_AREA_MAP"		// エリア地図データダウンロード管理テーブル
#define	SCC_DAL_KIND_DOWNLOAD_SYS				"DOWNLOAD_SYS"			// システムファイルダウンロード管理テーブル
#define	SCC_DAL_KIND_DOWNLOAD_CUSTOM			"DOWNLOAD_CUSTOM"		// カスタマイズデータダウンロード管理テーブル
#define	SCC_DAL_KIND_TRAFFIC					"TRAFFIC"				// 交通情報テーブル
#define	SCC_DAL_KIND_FONT						"FONT"					// フォントデータ
#define	SCC_DAL_KIND_TTS						"TTS"					// TTS辞書データ
#define	SCC_DAL_KIND_OTHER						"OTHER"					// 他ファイル
#define	SCC_DAL_KIND_NAVI_SYMBOL				"NAVI_SYMBOL"			// 地図記号データ
#define	SCC_DAL_KIND_NAVI_POI					"NAVI_POI"				// 施設アイコンデータ
#define	SCC_DAL_KIND_NAVI_MAP					"NAVI_MAP"				// 地図デザインデータ
#define	SCC_DAL_KIND_NAVI_SKIN					"NAVI_SKIN"				// ナビスキン
#define	SCC_DAL_KIND_NAVI_ICON					"NAVI_ICON"				// ナビアイコン
#define	SCC_DAL_KIND_NAVI_CAR					"NAVI_CAR"				// 自車マーク
#define	SCC_DAL_KIND_NAVI_RC					"NAVI_RC"				// 経路計算
#define	SCC_DAL_KIND_NAVI_RG					"NAVI_RG"				// 経路誘導

#define	SCC_DAL_SYS_NAME_DATA					"DATA"					// appData

// DOWNLOAD_FLAG
typedef enum _E_SCC_DAL_DLFLAG {
	SCC_DAL_DL_FLAG_NODOWNLOAD = 0,				// ダウンロードされていない
	SCC_DAL_DL_FLAG_DOWNLOAD,					// ダウンロードされている
	SCC_DAL_DL_FLAG_DEL							// ダウンロードされていたが削除された
} E_SCC_DAL_DLFLAG;

// DATA_TYPE
typedef enum _E_SCC_DAL_UPDDATA_DATA_TYPE {
	SCC_DAL_UPDDATA_DATA_TYPE_BASE_MAP = 1,		// 広域地図
	SCC_DAL_UPDDATA_DATA_TYPE_AREA_MAP,			// エリア地図
	SCC_DAL_UPDDATA_DATA_TYPE_SYS,				// システムファイル
	SCC_DAL_UPDDATA_DATA_TYPE_CUSTOM			// カスタムファイル
} E_SCC_DAL_UPDDATA_DATA_TYPE;


// REGION_INFO
typedef struct _SCC_REGIONINFO {
	Char	regionCode[SCC_MAPDWL_MAXCHAR_RGNCODE];
	Char	regionName[SCC_DAL_REGION_NAME];
	INT64	mapPressSize;
	INT64	mapNonPressize;
	INT64	dataPressSize;
	INT64	dataNonPressSize;
	UChar	md5[CC_CMN_MD5];
} SCC_REGIONINFO;

// AREA_INFO
typedef struct _SCC_AREAINFO {
	INT32	sectionCode;
	Char	sectionName[SCC_DAL_SECTION_NAME];
	INT32	areaCode;
	Char	areaName[SCC_DAL_AREA_NAME];
} SCC_AREAINFO;

// UPDATE_DATA
typedef struct _SCC_UPDATEDATA {
	INT32	areaCode;
	Char	appVersionS[SCC_DAL_APP_VERSION];
	Char	appVersionE[SCC_DAL_APP_VERSION];
	INT32	dataVersion;
	E_SCC_DAL_UPDDATA_DATA_TYPE	dataType;
	Char	dataKind[SCC_DAL_DATA_KIND];
	INT32	fileType;
	Char	filePath[SCC_MAX_PATH];
	INT64	pressSize;
	INT64	nonPressSize;
	INT32	dataNum;
	Char	areaName[SCC_DAL_AREA_NAME];
	UChar	md5[CC_CMN_MD5];
} SCC_UPDATEDATA;

// DOWNLOAD_BASE_VERSION
typedef struct _SCC_DOWNLOADBASEVERSION {
	Char				kind[SCC_DAL_KIND64];
	Char				tableName[SCC_DAL_TABLE_NAME];
	E_SCC_DAL_DLFLAG	dlFlag;
	INT32				baseVersion;
} SCC_DOWNLOADBASEVERSION;

// DOWNLOAD_AREA_MAP
typedef struct _SCC_DOWNLOADAREAMAP {
	INT32				areaCode;
	Char				kind[SCC_DAL_KIND64];
	E_SCC_DAL_DLFLAG	dlFlag;
	INT32				baseVersion;
} SCC_DOWNLOADAREAMAP;

// DOWNLOAD_SYS
typedef struct _SCC_UPDVER_DOWNLOADSYS {
	Char	kind[SCC_DAL_KIND32];
	Char	name[SCC_DAL_SYS_NAME];				// INSERTする時のみ使用する
	INT32	version;
} SCC_UPDVER_DOWNLOADSYS;

// DOWNLOAD_CUSTOM
typedef struct _SCC_UPDVER_DOWNLOADCUSTOM {
	Char	kind[SCC_DAL_KIND32];
	INT32	setnumber;
	Char	name[SCC_DAL_CUSTOM_NAME];			// INSERTする時のみ使用する
	Char	version[SCC_DAL_CUSTOM_VERSION];
} SCC_UPDVER_DOWNLOADCUSTOM;


// DALの初期化
E_SC_RESULT SCC_DAL_Initialize(const Char *dbFilePath, DBOBJECT **db);
// DALの終了化
void SCC_DAL_Finalize(DBOBJECT *db);

// トランザクション開始
E_SC_RESULT SCC_DAL_Transaction(DBOBJECT *db);
// コミット
E_SC_RESULT SCC_DAL_Commit(DBOBJECT *db);
// ロールバック
E_SC_RESULT SCC_DAL_Rollback(DBOBJECT *db);

// ■regionListInfo.db
// REGION_INFOテーブルのデータ取得(全件)
E_SC_RESULT SCC_DAL_GetRegionInfoList(DBOBJECT *db, INT32 getRegionInfoNum, SCC_REGIONINFO *regionInfo, INT32 *regionInfoNum);
// REGION_INFOテーブルのデータ取得(1件)
E_SC_RESULT SCC_DAL_GetRegionInfo(DBOBJECT *db, SCC_REGIONINFO *regionInfo);

// ■dataVersionInfo.db
// エリア地図データダウンロード管理テーブル（DOWNLOAD_AREA_MAP）
// AREA_INFOテーブルのデータ取得(全件)
E_SC_RESULT SCC_DAL_GetAreaInfoList(DBOBJECT *db, INT32 getAreaInfoNum, SCC_AREAINFO *areaInfo, INT32 *areaInfoNum);
// UPDATE_DATAテーブルのデータ取得(全件)
E_SC_RESULT SCC_DAL_GetUpdateDataList(DBOBJECT *db, INT32 maxUpdateDataNum, SCC_UPDATEDATA *updateData, INT32 *updateDataNum);
// UPDATE_DATAテーブルのデータ取得(1件)
E_SC_RESULT SCC_DAL_GetUpdateData(DBOBJECT *db, SCC_UPDATEDATA *updateData);

// ■Polaris.db
// PARCELテーブルのデータ更新
E_SC_RESULT SCC_DAL_UpdateParcel(DBOBJECT *db, const Char *filePath, Bool transaction, const SMPROGRESSCBFNC *callbackFnc);
// DOWNLOAD_BASE_VERSIONテーブルのデータ更新
E_SC_RESULT SCC_DAL_UpdDLBaseVersion(DBOBJECT *db, const SCC_DOWNLOADBASEVERSION *dlBaseVer, INT32 dlBaseVerNum, Bool transaction);
// DOWNLOAD_BASE_VERSIONテーブルのデータ取得(1件)
E_SC_RESULT SCC_DAL_GetDLBaseVersion(DBOBJECT *db, const Char *kind, const Char *tableName, E_SCC_DAL_DLFLAG *dlFlag, INT32 *baseVersion);
// DOWNLOAD_AREA_MAPテーブルのデータ更新
E_SC_RESULT SCC_DAL_UpdDLAreaMap(DBOBJECT *db, const SCC_DOWNLOADAREAMAP *dlAreaMap, INT32 dlAreaMapNum, Bool transaction);
// DOWNLOAD_AREA_MAPテーブルのデータ取得(1件)
E_SC_RESULT SCC_DAL_GetDLAreaMap(sqlite3 *db, INT32 areaCode, const Char *kind, E_SCC_DAL_DLFLAG *dlFlag, INT32 *baseVersion);
// DOWNLOAD_AREA_MAPテーブルのデータ取得(全件)
E_SC_RESULT SCC_DAL_GetDLAreaMapList(DBOBJECT *db, INT32 maxDlAreaMapNum, SCC_DOWNLOADAREAMAP *dlAreaMap, INT32 *dlAreaMapNum);
// DOWNLOAD_SYSテーブルのデータ更新
E_SC_RESULT SCC_DAL_UpdDLSys(DBOBJECT *db, const SCC_UPDVER_DOWNLOADSYS *dlSys, INT32 dlSysNum, Bool transaction);
// DOWNLOAD_SYSテーブルのデータ取得(1件)
E_SC_RESULT SCC_DAL_GetDLSys(DBOBJECT *db, const Char *kind, INT32 *version);
// DOWNLOAD_SYSテーブルのデータ取得(全件)
E_SC_RESULT SCC_DAL_GetDLSysList(DBOBJECT *db, INT32 maxDlSysNum, SCC_UPDVER_DOWNLOADSYS *dlSys, INT32 *dlSysNum);
// DOWNLOAD_CUSTOMテーブルのデータ更新
E_SC_RESULT SCC_DAL_UpdDLCustom(DBOBJECT *db, const SCC_UPDVER_DOWNLOADCUSTOM *dlCustom, INT32 dlCustomNum, Bool transaction);
// DOWNLOAD_CUSTOMテーブルのデータ取得(1件)
E_SC_RESULT SCC_DAL_GetDLCustom(DBOBJECT *db, const Char *kind, const Char *setnumber, INT32 *version);
// DOWNLOAD_CUSTOMテーブルのデータ取得(全件)
E_SC_RESULT SCC_DAL_GetDLCustomList(DBOBJECT *db, INT32 maxDlCustomNum, SCC_UPDVER_DOWNLOADCUSTOM *dlCustom, INT32 *dlCustomNum);
// AREA_CLSテーブルのデータ更新
E_SC_RESULT SCC_DAL_UpdateAreaCls(DBOBJECT *db, const Char *filePath, Bool transaction, const SMPROGRESSCBFNC *callbackFnc);
#endif /* _SMCCOM_DAL_H_ */
