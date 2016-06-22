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
 * SMCoreDALApi.h
 *
 *  Created on: 2015/11/05
 *      Author: masutani
 */

#ifndef SMCOREDALAPI_H_
#define SMCOREDALAPI_H_

/**
 * DAL
 */
#ifdef __cplusplus
extern "C" {
#endif
//-----------------------------------
// API定義
//-----------------------------------
SC_DA_RESULT SC_DA_Initialize(const char* data_folder);											// DAL初期化
SC_DA_RESULT SC_DA_Finalize();																	// DAL終了化

//--------------------------#PARCEL--------------------------
SC_DA_RESULT SC_DA_GetPclRoadDataSize(T_DAL_PID *pstPid, UINT32 *pRoadBinSize);					// パーセル道路データサイズ取得
SC_DA_RESULT SC_DA_GetPclBkgdDataSize(T_DAL_PID *pstPid, UINT32 *pBkgdBinSize);					// パーセル背景データサイズ取得
SC_DA_RESULT SC_DA_GetPclShapeDataSize(T_DAL_PID *pstPid, UINT32 *pShapeBinSize);				// パーセル形状データサイズ取得
SC_DA_RESULT SC_DA_GetPclGuideDataSize(T_DAL_PID *pstPid, UINT32 *pGuideBinSize);				// 誘導データサイズ取得
SC_DA_RESULT SC_DA_GetPclNameDataSize(T_DAL_PID *pstPid, UINT32 *pNameBinSize);					// パーセル名称データサイズ取得
SC_DA_RESULT SC_DA_GetPclDensityDataSize(T_DAL_PID *pstPid, UINT32 *pDensityBinSize);			// 密度データサイズ取得
SC_DA_RESULT SC_DA_GetPclMarkDataSize(T_DAL_PID *pstPid, UINT32 *pMarkBinSize);					// パーセル記号背景データサイズ取得
SC_DA_RESULT SC_DA_GetPclRoadNameDataSize(T_DAL_PID *pstPid, UINT32 *pRoadNameBinSize);			// パーセル道路名称データサイズ取得
SC_DA_RESULT SC_DA_GetPclBasisDataSize(T_DAL_PID *pstPid, UINT32 *pBasisBinSize);				// パーセル基本情報データサイズ取得
SC_DA_RESULT SC_DA_GetPclBkgdAreaClsDataSize(T_DAL_PID *pstPid, UINT32 *pBkgdAreaClsBinSize);	// 地域クラス背景データサイズ取得
SC_DA_RESULT SC_DA_GetPclData(T_DAL_PID *pstPid, UINT32 mapKind, T_DAL_BINARY *pstBinData);	// パーセル情報データ取得

//--------------------------#SYSTEM_INFORMATION--------------------------
SC_DA_RESULT SC_DA_GetSystemFormatVersionSize(INT32 *pSize);									//SYSTEM_FORMAT_VERSION サイズ取得
SC_DA_RESULT SC_DA_GetSystemFormatVersionData(char strBinData[]);								//SYSTEM_FORMAT_VERSIONデータ取得
SC_DA_RESULT SC_DA_GetSystemMachineTypeSize(INT32 *pSize);										//SYSTEM_MACHINE_TYPE サイズ取得
SC_DA_RESULT SC_DA_GetSystemMachineTypeData(char strBinData[]);									//SYSTEM_MACHINE_TYPE データ取得
SC_DA_RESULT SC_DA_GetSystemAppVersionSize(INT32 *pSize);										//SYSTEM_APP_VERSION サイズ取得
SC_DA_RESULT SC_DA_GetSystemAppVersionData(char strBinData[]);									//SYSTEM_APP_VERSION データ取得
SC_DA_RESULT SC_DA_GetSystemAreaSize(INT32 *pSize);												//SYSTEM_AREA サイズ取得
SC_DA_RESULT SC_DA_GetSystemAreaData(char strBinData[]);										//SYSTEM_AREA データ取得
SC_DA_RESULT SC_DA_GetSystemLangSize(INT32 *pSize);												//SYSTEM_LANG サイズ取得
SC_DA_RESULT SC_DA_GetSystemLangData(char strBinData[]);										//SYSTEM_LANG データ取得
SC_DA_RESULT SC_DA_GetSystemLevel01Size(INT32 *pSize);											//SYSTEM_LEVEL_01 サイズ取得
SC_DA_RESULT SC_DA_GetSystemLevel01Data(char strBinData[]);										//SYSTEM_LEVEL_01 データ取得
SC_DA_RESULT SC_DA_GetSystemLevel02Size(INT32 *pSize);											//SYSTEM_LEVEL_02 サイズ取得
SC_DA_RESULT SC_DA_GetSystemLevel02Data(char strBinData[]);										//SYSTEM_LEVEL_02 データ取得
SC_DA_RESULT SC_DA_GetSystemLevel03Size(INT32 *pSize);											//SYSTEM_LEVEL_03 サイズ取得
SC_DA_RESULT SC_DA_GetSystemLevel03Data(char strBinData[]);										//SYSTEM_LEVEL_03 データ取得
SC_DA_RESULT SC_DA_GetSystemLevel04Size(INT32 *pSize);											//SYSTEM_LEVEL_04 サイズ取得
SC_DA_RESULT SC_DA_GetSystemLevel04Data(char strBinData[]);										//SYSTEM_LEVEL_04 データ取得
SC_DA_RESULT SC_DA_GetSystemLevel05Size(INT32 *pSize);											//SYSTEM_LEVEL_05 サイズ取得
SC_DA_RESULT SC_DA_GetSystemLevel05Data(char strBinData[]);										//SYSTEM_LEVEL_05 データ取得
SC_DA_RESULT SC_DA_GetSystemLevel06Size(INT32 *pSize);											//SYSTEM_LEVEL_06 サイズ取得
SC_DA_RESULT SC_DA_GetSystemLevel06Data(char strBinData[]);										//SYSTEM_LEVEL_06 データ取得
SC_DA_RESULT SC_DA_GetSystemLevel07Size(INT32 *pSize);											//SYSTEM_LEVEL_07 サイズ取得
SC_DA_RESULT SC_DA_GetSystemLevel07Data(char strBinData[]);										//SYSTEM_LEVEL_07 データ取得
SC_DA_RESULT SC_DA_GetSystemLevel08Size(INT32 *pSize);											//SYSTEM_LEVEL_08 サイズ取得
SC_DA_RESULT SC_DA_GetSystemLevel08Data(char strBinData[]);										//SYSTEM_LEVEL_08 データ取得
SC_DA_RESULT SC_DA_GetSystemDataProviderSize(INT32 *pSize);										//SYSTEM_DATA_PROVIDER サイズ取得
SC_DA_RESULT SC_DA_GetSystemDataProviderData(char strBinData[]);								//SYSTEM_DATA_PROVIDER データ取得
SC_DA_RESULT SC_DA_GetSystemCharacterCodeSize(INT32 *pSize);									//SYSTEM_CHARACTER_CODE サイズ取得
SC_DA_RESULT SC_DA_GetSystemCharacterCodeData(char strBinData[]);								//SYSTEM_CHARACTER_CODE データ取得
SC_DA_RESULT SC_DA_GetSystemMapRangeSize(INT32 *pSize);											//SYSTEM_MAP_RANGE サイズ取得
SC_DA_RESULT SC_DA_GetSystemMapRangeData(char strBinData[]);									//SYSTEM_MAP_RANGE データ取得
SC_DA_RESULT SC_DA_GetSystemMapVerDataSize(INT32 *piMapVerSize);								// 地図バージョン文字列 サイズ取得
SC_DA_RESULT SC_DA_GetSystemMapVerData(char strBinMapVerData[]);								// 地図バージョン文字列 データ取得
SC_DA_RESULT SC_DA_GetSystemMapVerNoDataSize(INT32 *piMapVerNoSize);							// 地図のデータバ-ジョン サイズ取得
SC_DA_RESULT SC_DA_GetSystemMapVerNoData(char strBinMapVerNoData[]);							// 地図のデータバ-ジョン データ取得
SC_DA_RESULT SC_DA_GetSystemMapBuildNoDataSize(INT32 *piMapBuildNoSize);						// 地図のデータビルド番号 サイズ取得
SC_DA_RESULT SC_DA_GetSystemMapBuildNoData(char strBinMapBuildNoData[]);						// 地図のデータビルド番号 データ取得
SC_DA_RESULT SC_DA_GetOsmOriginalTimestampDataSize(INT32 *pOsmOriginalTimestampSize);			// 地図のオリジナル(OSM)のデータバ-ジョン サイズ取得
SC_DA_RESULT SC_DA_GetOsmOriginalTimestampData(char strBinOsmOriginalTimestampData[]);			// 地図のオリジナル(OSM)のデータバ-ジョン データ取得
SC_DA_RESULT SC_DA_GetSystemInfo(const char*, T_DAL_DBSYSTEMINFO*);								// 基本情報取得
SC_DA_RESULT SC_DA_GetSystemSeaFlagData(char strBinData[]);											//SYSTEM_SEA_FLAG データ取得

//--------------------------#DOWNLOAD_AREA_MAP--------------------------
SC_DA_RESULT SC_DA_GetDlAreaMapData(UINT8 *kind, T_DAL_DLAREA *downloadArea, UINT8 *dataCnt);	// ダウンロードエリア情報 データ取得

//--------------------------#AREA_CLS-----------------------------------
SC_DA_RESULT SC_DA_GetAreaClsList(SMAREACLSCODE* areaClsCode, T_DAL_AREA_CLS_LIST* areaClsList);
#ifdef __cplusplus
}
#endif

/**
 * MAL
 */
#ifdef __cplusplus
extern "C" {
#endif
UINT16 SC_MA_BinSearchNwRecord(MAL_HDL aRoad, UINT32 aKey, INT32 aType);
E_SC_RESULT SC_MA_BinSearchShapeIndex(UINT8* aShape, UINT32 key, UINT32* aOffset);
E_SC_RESULT SC_MA_BinSearchShapeUpperIndex(UINT8* aShapeBin, UINT32 aKey, UINT32* aIndex);
MAL_HDL SC_MA_GetMapSharpRecord(MAL_HDL aBin);
UINT32 SC_MA_GetGuideLinkOffset(UINT32 , UINT8 *);
UINT32 MA_BinSearchOffset(const UINT32*, UINT32 , UINT32 , UINT8*);
#ifdef __cplusplus
}
#endif

/**
 * PAL
 */
#ifdef __cplusplus
extern "C" {
E_PAL_RESULT SC_POI_Initialize(const char* db_path);
}
void SC_POI_sort(std::vector<SM_GC_POI_QSORT_WORK_TBL>* sort_data_ptr);
E_PAL_RESULT SC_POI_POINT_TBL_Initialize(const char* db_path);
int SC_POI_addGCpointTBL(SM_GC_POI_POINT_TBL* point_tbl_ptr);
int SC_POI_chgGCpointTBL(SM_GC_POI_POINT_TBL* point_tbl_ptr);
int SC_POI_delGCpointTBL(SM_GC_POI_POINT_TBL* point_tbl_ptr);
int SC_POI_selectGCpointTBL(SC_POI_SEARCH_COND_TBL* cond_tbl_ptr, std::vector<SM_GC_POI_QSORT_WORK_TBL> *sort_data_list,
		std::vector<SM_GC_POI_POINT_TBL> *point_tbl_list);
int SC_POI_getCountGCpointTBL(SC_POI_SEARCH_COND_TBL* cond_tbl_ptr);

E_PAL_RESULT SC_POI_HISTORY_TBL_Initialize(const char* db_path);
int SC_POI_addGChistoryTBL(SM_GC_POI_HISTORY_TBL* history_tbl_ptr);
int SC_POI_chgGChistoryTBL(SM_GC_POI_HISTORY_TBL* history_tbl_ptr);
int SC_POI_delGChistoryTBL(SM_GC_POI_HISTORY_TBL* history_tbl_ptr);
int SC_POI_selectGChistoryTBL(SC_POI_SEARCH_COND_2_TBL* cond_tbl_ptr, std::vector<SM_GC_POI_HISTORY_TBL> *history_tbl_list);
int SC_POI_getCountGChistoryTBL(SC_POI_SEARCH_COND_2_TBL* cond_tbl_ptr);

E_PAL_RESULT SC_POI_FAVORITE_TBL_Initialize(const char* db_path);
int SC_POI_addGCfavoriteTBL(SM_GC_POI_FAVORITE_TBL* favorite_tbl_ptr);
int SC_POI_chgGCfavoriteTBL(SM_GC_POI_FAVORITE_TBL* favorite_tbl_ptr);
int SC_POI_delGCfavoriteTBL(SM_GC_POI_FAVORITE_TBL* favorite_tbl_ptr);
int SC_POI_selectGCfavoriteTBL(SC_POI_SEARCH_COND_TBL* cond_tbl_ptr, std::vector<SM_GC_POI_QSORT_WORK_TBL> *sort_data_list,
		std::vector<SM_GC_POI_FAVORITE_TBL> *favorite_tbl_list);
int SC_POI_getCountGCfavoriteTBL(SC_POI_SEARCH_COND_TBL* cond_tbl_ptr);
double SC_POI_GetRealLen(double slatitude, double slongitude, double elatitude, double elongitude);
#endif // __cplusplus

/**
 * SDDAL
 */
#ifdef __cplusplus
extern "C" {
#endif
// DALの初期化
E_SC_RESULT SC_SDDAL_Initialize(const Char *dbFilePath);
// DALの終了化
void SC_SDDAL_Finalize();
// トランザクション開始
E_SC_RESULT SC_SDDAL_Transaction();
// コミット
E_SC_RESULT SC_SDDAL_Commit();
// ロールバック
E_SC_RESULT SC_SDDAL_Rollback();
// テーブル有無チェック
E_SC_RESULT SC_SDDAL_CheckTabeleExist(const Char *tableName, Bool *exist);
// PHYD_SENSOR_DATAテーブル作成
E_SC_RESULT SC_SDDAL_CreateTabelePhydSensorData(Bool transaction);
// PHYD_SENSOR_DATAテーブルのデータ件数取得
E_SC_RESULT SC_SDDAL_GetPhydSensorDataCount(INT32 *dataNum);
// PHYD_SENSOR_DATAテーブルのデータ追加
E_SC_RESULT SC_SDDAL_AddPhydSensorData(const Char *time, const SMPHYDDATA *data, INT32 dataNum, Bool transaction);
// PHYD_SENSOR_DATAテーブルのデータ削除
E_SC_RESULT SC_SDDAL_DelPhydSensorData(const Char *time, INT32 id, Bool transaction);
// PHYD_SENSOR_DATAテーブルのデータ取得
E_SC_RESULT SC_SDDAL_GetPhydSensorDataList(INT32 maxDataNum, const Char *time, INT32 id, SMPHYDDATA *data, INT32 *dataNum, Bool *lastFlg);
#ifdef __cplusplus
}
#endif

/**
 * お気に入り・登録地・履歴 それぞれのJNI処理を抜き出したAPI
 * ANDROID以外では無効化
 */
#ifdef ANDROID
#ifdef __cplusplus
#include <jni.h>
int SC_POI_selectGCpointTBL_forJni(JNIEnv * env, jclass thiz, jobject in_obj, jobject out_obj, jmethodID mid_ListAdd, jclass item_cls,
		jmethodID item_method, jfieldID out_data_type_id, jfieldID out_userid_id, jfieldID out_datetime_id, jfieldID out_gemid_id,
		jfieldID out_gemspotid_id, jfieldID out_pos_name_id, jfieldID out_lat_id, jfieldID out_log_id, jfieldID out_len_id,
		SC_POI_SEARCH_COND_TBL* cond_tbl_ptr);
int SC_POI_selectGCfavoriteTBL_forJni(JNIEnv * env, jclass thiz, jobject in_obj, jobject out_obj, jmethodID mid_ListAdd, jclass item_cls,
		jmethodID item_method, jfieldID out_userid_id, jfieldID out_dbtime_id, jfieldID out_contentstime_id, jfieldID out_ctgry_code_id,
		jfieldID out_id_id, jfieldID out_url_id, jfieldID out_name_id, jfieldID out_pos_name_id, jfieldID out_contents_id,
		jfieldID out_binary_data_id, jfieldID out_binary_data_len_id, jfieldID out_lat_id, jfieldID out_log_id,
		SC_POI_SEARCH_COND_TBL* cond_tbl_ptr);
int SC_POI_selectGChistoryTBL_forJni(JNIEnv * env, jclass thiz, jobject in_obj, jobject out_obj, jmethodID mid_ListAdd, jclass item_cls,
		jmethodID item_method, jfieldID out_userid_id, jfieldID out_datetime_id, jfieldID out_gemid_id, jfieldID out_gemspotid_id,
		jfieldID out_lat_id, jfieldID out_log_id, SC_POI_SEARCH_COND_2_TBL* cond_tbl_ptr);
#endif /* __cplusplus */
#endif /* ANDROID */

#endif /* SMCOREDALAPI_H_ */
