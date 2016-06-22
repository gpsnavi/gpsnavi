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
 * SMDAL.c
 *
 *  Created on:
 *      Author:
 */

#include "SMCoreDALInternal.h"

//-----------------------------------
// 定数定義
//-----------------------------------
enum {
	e_TYPE_SHAPE = 0,							// 形状
	e_TYPE_BKGD,								// 背景
	e_TYPE_ROAD,								// 道路
	e_TYPE_GUIDE,								// 誘導
	e_TYPE_NAME,								// 名称
	e_TYPE_DENSITY,								// 密度
	e_TYPE_MARK,								// 背景記号
	e_TYPE_ROAD_NAME,							// 道路名称
	e_TYPE_PARCEL_BASIS,						// 基本情報
	e_TYPE_BKGD_AREA_CLS,						// 地域クラス背景
	e_TYPE_END
};

#define DAL_MUTEX_LOCK(mutex)																\
	{																						\
		E_SC_RESULT result = SC_LockMutex(&mutex);											\
		if (result != e_SC_RESULT_SUCCESS) {												\
			SC_LOG_ErrorPrint(SC_TAG_DAL, "SC_LockMutext error. [0x%08x] "HERE, result);	\
			return (SC_DA_RES_FAIL);														\
		}																					\
	}
#define DAL_MUTEX_UNLOCK(mutex)																\
	{																						\
		E_SC_RESULT result = SC_UnLockMutex(&mutex);										\
		if (result != e_SC_RESULT_SUCCESS) {												\
			SC_LOG_ErrorPrint(SC_TAG_DAL, "SC_UnLockMutext error. [0x%08x] "HERE, result);	\
			return (SC_DA_RES_FAIL);														\
		}																					\
	}

//-----------------------------------
// 変数定義
//-----------------------------------
static sqlite3 *SQLITEDB = NULL;				//sqlite3オブジェクト
static T_DAL_DBSYSTEMINFO m_SystemInfo = {};	//SYSTEM_INFORMATION
static char READDBPATH[SC_DA_PATH_LEN] = {};	//DBパス

//-----------------------------------
// 宣言
//-----------------------------------
static SC_DA_RESULT SC_DA_DBDisconnect(sqlite3*);
static sqlite3* SC_DA_DBConnect(const char*);
// Mutex
static SC_MUTEX mMutex = SC_MUTEX_INITIALIZER;

/**
 * @brief sqlite3接続
 * @param dbPath ファイルパス
 * @return sqlite3オブジェクト
 */
static sqlite3* SC_DA_DBConnect(const char* dbPath) {
	sqlite3* sqliteDB = NULL;
	INT32 sqliteRet = SQLITE_ERROR;
	INT32 retry = 0;

	while (DAL_RETRY_MAX > retry) {
		sqliteRet = sqlite3_open(dbPath, &sqliteDB);
		if (SQLITE_OK == sqliteRet) {
			break;
		}
		// Log出力＋リトライ
		retry++;
		SC_LOG_ErrorPrint(SC_TAG_DAL, "sqlite3_open fail. ret=%x retry=%d", sqliteRet, retry);
	}
	if (SQLITE_OK != sqliteRet) {
		SC_LOG_ErrorPrint(SC_TAG_DAL, "sqlite3_open fail. ret=%x", sqliteRet);
	}
	return (sqliteDB);
}

/**
 * @brief sqlite3切断
 * @param sqliteDB sqlite3オブジェクト
 */
static SC_DA_RESULT SC_DA_DBDisconnect(sqlite3* sqliteDB) {
	SC_DA_RESULT ret = SC_DA_RES_FAIL;
	INT32 sqliteRet = SQLITE_ERROR;
	INT32 retry = 0;

	while (DAL_RETRY_MAX > retry) {
		sqliteRet = sqlite3_close(sqliteDB);
		if (SQLITE_OK == sqliteRet) {
			SC_LOG_InfoPrint(SC_TAG_DAL, "sqlite3_close success.");
			ret = SC_DA_RES_SUCCESS;
			break;
		}
		// Log出力＋リトライ
		retry++;
		SC_LOG_ErrorPrint(SC_TAG_DAL, "sqlite3_close fail. ret=%x retry=%d", sqliteRet, retry);
	}
	if (SQLITE_OK != sqliteRet) {
		SC_LOG_ErrorPrint(SC_TAG_DAL, "sqlite3_close fail. ret=%x", sqliteRet);
	}
	return (ret);
}

/**
 * @brief DAL初期化
 * @param dbPath
 */
SC_DA_RESULT SC_DA_Initialize(const char* dbPath) {
	SC_LOG_DebugPrint(SC_TAG_DAL, "SC_DA_Initialize() start");

	SC_DA_RESULT ret = SC_DA_RES_FAIL;
	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	INT32 sqliteRet = SQLITE_ERROR;
	INT32 retry = 0;

	if (NULL == dbPath) {
		SC_LOG_ErrorPrint(SC_TAG_DAL, "SC_DA_Initialize() bad param.");
		return (SC_DA_RES_FAIL);
	}

	SC_LOG_InfoPrint(SC_TAG_DAL, "param=%s", dbPath);

	do {
		// Mutex生成
		result = SC_CreateMutex(&mMutex);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_DAL, "SC_CreateMutext error, [0x%08x] " HERE, result);
			break;
		}
		// パス初期化
		memset(READDBPATH, 0, sizeof(READDBPATH));

		if (NULL != SQLITEDB) {
			SC_LOG_ErrorPrint(SC_TAG_DAL, "SQLITEDB is not null. do finalize.");
			if (SC_DA_RES_SUCCESS != SC_DA_Finalize()) {
				break;
			}
		}
		while (DAL_RETRY_MAX > retry) {
			sqliteRet = sqlite3_open(dbPath, &SQLITEDB);
			if (SQLITE_OK == sqliteRet) {
				break;
			}
			// Log出力＋リトライ
			retry++;
			SC_LOG_ErrorPrint(SC_TAG_DAL, "sqlite3_open fail. ret=%x retry=%d", sqliteRet, retry);
		}
		if (SQLITE_OK != sqliteRet) {
			break;
		}

		// DBフォルダ名格納
		if (SC_DA_PATH_LEN <= strlen(dbPath)) {
			strncpy(READDBPATH, dbPath, SC_DA_PATH_LEN);
		} else {
			strcpy(READDBPATH, dbPath);
		}
		// 成功
		ret = SC_DA_RES_SUCCESS;
		SC_LOG_InfoPrint(SC_TAG_DAL, "sqlite3_open success. save path=%s", READDBPATH);
	} while (0);

	if (SC_DA_RES_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_DAL, "sqlite3_open fail. do finalize. ret=%x", ret);
		SC_DA_Finalize();
	}

	// SYSTEMINFO取得
	ret = SC_DA_GetSystemInfo(NULL, &m_SystemInfo);
	if (SC_DA_RES_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_DAL, "SC_DA_GetSystemInfo() fail. ret=%x", ret);
	}

	SC_LOG_DebugPrint(SC_TAG_DAL, "SC_DA_Initialize() end");
	return (ret);
}

/**
 * @brief DAL終了化
 */
SC_DA_RESULT SC_DA_Finalize() {
	SC_LOG_DebugPrint(SC_TAG_DAL, "SC_DA_Finalize() start.");

	SC_DA_RESULT ret = SC_DA_RES_FAIL;
	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	INT32 sqliteRet = SQLITE_ERROR;
	INT32 retry = 0;

	if (NULL != SQLITEDB) {
		SC_LOG_InfoPrint(SC_TAG_DAL, "save path=%s", READDBPATH);
		while (DAL_RETRY_MAX > retry) {
			sqliteRet = sqlite3_close(SQLITEDB);
			if (SQLITE_OK == sqliteRet) {
				ret = SC_DA_RES_SUCCESS;
				SC_LOG_InfoPrint(SC_TAG_DAL, "sqlite3_close success.");
				break;
			}
			// Log出力＋リトライ
			retry++;
			SC_LOG_ErrorPrint(SC_TAG_DAL, "sqlite3_close fail. ret=%x retry=%d", sqliteRet, retry);
		}
	} else {
		SC_LOG_ErrorPrint(SC_TAG_DAL, "SQLITEDB is null.");
	}

	// 初期化
	SQLITEDB = NULL;
	memset(READDBPATH, 0, sizeof(READDBPATH));

	// Mutex破棄
	result = SC_DestroyMutex(&mMutex);
	if (e_SC_RESULT_SUCCESS != result) {
		SC_LOG_ErrorPrint(SC_TAG_DAL, "SC_DestroyMutex error, [0x%08x] " HERE, result);
	}

	SC_LOG_DebugPrint(SC_TAG_DAL, "SC_DA_Finalize() end. ret=%x", ret);
	return (ret);
}

//--------------------------#PARCEL--------------------------
/**
 * @brief SC_DA_GetPclRoadDataSize
 * @note パーセル道路データサイズ取得
 */
SC_DA_RESULT SC_DA_GetPclRoadDataSize(T_DAL_PID *pstPid, UINT32 *pRoadBinSize) {
	DAL_MUTEX_LOCK(mMutex);
	SC_DA_RESULT result = SC_DA_GetPclDataSize(SQLITEDB, pstPid, pRoadBinSize, e_TYPE_ROAD);
	DAL_MUTEX_UNLOCK(mMutex);
	return (result);
}

/**
 * @brief SC_DA_GetPclBkgdDataSize
 * @note パーセル背景データサイズ取得
 */
SC_DA_RESULT SC_DA_GetPclBkgdDataSize(T_DAL_PID *pstPid, UINT32 *pBkgdBinSize) {
	DAL_MUTEX_LOCK(mMutex);
	SC_DA_RESULT result = SC_DA_GetPclDataSize(SQLITEDB, pstPid, pBkgdBinSize, e_TYPE_BKGD);
	DAL_MUTEX_UNLOCK(mMutex);
	return (result);
}

/**
 * @brief SC_DA_GetPclShapeDataSize
 * @note パーセル形状データサイズ取得
 */
SC_DA_RESULT SC_DA_GetPclShapeDataSize(T_DAL_PID *pstPid, UINT32 *pShapeBinSize) {
	DAL_MUTEX_LOCK(mMutex);
	SC_DA_RESULT daResult = SC_DA_GetPclDataSize(SQLITEDB, pstPid, pShapeBinSize, e_TYPE_SHAPE);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}

/**
 * @brief SC_DA_GetPclGuideDataSize
 * @note パーセル誘導データサイズ取得
 */
SC_DA_RESULT SC_DA_GetPclGuideDataSize(T_DAL_PID *pstPid, UINT32 *pGuideBinSize) {
	DAL_MUTEX_LOCK(mMutex);
	SC_DA_RESULT daResult = SC_DA_GetPclDataSize(SQLITEDB, pstPid, pGuideBinSize, e_TYPE_GUIDE);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}

/**
 * @brief SC_DA_GetPclNameDataSize
 * @note パーセル名称データサイズ取得
 */
SC_DA_RESULT SC_DA_GetPclNameDataSize(T_DAL_PID *pstPid, UINT32 *pNameBinSize) {
	DAL_MUTEX_LOCK(mMutex);
	SC_DA_RESULT daResult = SC_DA_GetPclDataSize(SQLITEDB, pstPid, pNameBinSize, e_TYPE_NAME);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}

/**
 * @brief SC_DA_GetPclDensityDataSize
 * @note パーセル密度データサイズ取得
 */
SC_DA_RESULT SC_DA_GetPclDensityDataSize(T_DAL_PID *pstPid, UINT32 *pDensityBinSize) {
	DAL_MUTEX_LOCK(mMutex);
	SC_DA_RESULT daResult = SC_DA_GetPclDataSize(SQLITEDB, pstPid, pDensityBinSize, e_TYPE_DENSITY);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}

/**
 * @brief SC_DA_GetPclMarkDataSize
 * @note パーセル記号背景データサイズ取得
 */
SC_DA_RESULT SC_DA_GetPclMarkDataSize(T_DAL_PID *pstPid, UINT32 *pMarkBinSize) {
	DAL_MUTEX_LOCK(mMutex);
	SC_DA_RESULT daResult = SC_DA_GetPclDataSize(SQLITEDB, pstPid, pMarkBinSize, e_TYPE_MARK);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}

/**
 * @brief SC_DA_GetPclRoadNameDataSize
 * @note パーセル道路名称データサイズ取得
 */
SC_DA_RESULT SC_DA_GetPclRoadNameDataSize(T_DAL_PID *pstPid, UINT32 *pRoadNameBinSize) {
	DAL_MUTEX_LOCK(mMutex);
	SC_DA_RESULT daResult = SC_DA_GetPclDataSize(SQLITEDB, pstPid, pRoadNameBinSize, e_TYPE_ROAD_NAME);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}

/**
 * @brief SC_DA_GetPclBasisDataSize
 * @note パーセル基本情報データサイズ取得
 */
SC_DA_RESULT SC_DA_GetPclBasisDataSize(T_DAL_PID *pstPid, UINT32 *pBasisBinSize) {
	DAL_MUTEX_LOCK(mMutex);
	SC_DA_RESULT daResult = SC_DA_GetPclDataSize(SQLITEDB, pstPid, pBasisBinSize, e_TYPE_PARCEL_BASIS);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}

/**
 * @brief SC_DA_GetPclBkgdAreaClsDataSize
 * @note 地域クラス背景データサイズ取得
 */
SC_DA_RESULT SC_DA_GetPclBkgdAreaClsDataSize(T_DAL_PID *pstPid, UINT32 *pBkgdAreaClsBinSize) {
	DAL_MUTEX_LOCK(mMutex);
	SC_DA_RESULT daResult = SC_DA_GetPclDataSize(SQLITEDB, pstPid, pBkgdAreaClsBinSize, e_TYPE_BKGD_AREA_CLS);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}

/**
 * @brief SC_DA_GetPclData
 * @note パーセルデータ取得
 */
SC_DA_RESULT SC_DA_GetPclData(T_DAL_PID *pstPid, UINT32 mapKind, T_DAL_BINARY *pstBinData) {
	DAL_MUTEX_LOCK(mMutex);
	SC_DA_RESULT daResult = SC_DA_LoadPclData(SQLITEDB, pstPid, mapKind, pstBinData);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}

//--------------------------#SYSTEM_INFORMATION--------------------------

/**----------------------------------------------------------------------------
 * SC_DA_GetSystemFormatVersionSize
 * 地図情報管理 (SYSTEM_FORMAT_VERSION) サイズ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemFormatVersionSize(INT32 *pSize) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_SIZE, SC_DA_SQL_SYSTEM_FORMAT_VERSION);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationDataSize(SQLITEDB, pSize, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemMapVerData
 * 地図情報管理 (SYSTEM_FORMAT_VERSION)  データ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemFormatVersionData(char strBinData[]) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_DATA, SC_DA_SQL_SYSTEM_FORMAT_VERSION);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationData(SQLITEDB, strBinData, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemMachineTypeSize
 * 地図情報管理 (SYSTEM_MACHINE_TYPE) サイズ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemMachineTypeSize(INT32 *pSize) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_SIZE, SC_DA_SQL_SYSTEM_MACHINE_TYPE);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationDataSize(SQLITEDB, pSize, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystem
 * 地図情報管理 (SYSTEM_MACHINE_TYPE)  データ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemMachineTypeData(char strBinData[]) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_DATA, SC_DA_SQL_SYSTEM_MACHINE_TYPE);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationData(SQLITEDB, strBinData, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemAppVersionSize
 * 地図情報管理 (SYSTEM_APP_VERSION) サイズ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemAppVersionSize(INT32 *pSize) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_SIZE, SC_DA_SQL_SYSTEM_APP_VERSION);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationDataSize(SQLITEDB, pSize, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemAppVersionData
 * 地図情報管理 (SYSTEM_APP_VERSION)  データ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemAppVersionData(char strBinData[]) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_DATA, SC_DA_SQL_SYSTEM_APP_VERSION);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationData(SQLITEDB, strBinData, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemAreaSize
 * 地図情報管理 (SYSTEM_AREA) サイズ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemAreaSize(INT32 *pSize) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_SIZE, SC_DA_SQL_SYSTEM_AREA);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationDataSize(SQLITEDB, pSize, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemAreaData
 * 地図情報管理 (SYSTEM_AREA)  データ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemAreaData(char strBinData[]) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_DATA, SC_DA_SQL_SYSTEM_AREA);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationData(SQLITEDB, strBinData, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemLangSize
 * 地図情報管理 (SYSTEM_LANG) サイズ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemLangSize(INT32 *pSize) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_SIZE, SC_DA_SQL_SYSTEM_LANG);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationDataSize(SQLITEDB, pSize, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemLangData
 * 地図情報管理 (SYSTEM_LANG)  データ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemLangData(char strBinData[]) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_DATA, SC_DA_SQL_SYSTEM_LANG);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationData(SQLITEDB, strBinData, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemLevel01Size
 * 地図情報管理 (SYSTEM_LEVEL_01) サイズ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemLevel01Size(INT32 *pSize) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_SIZE, SC_DA_SQL_SYSTEM_LEVEL_01);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationDataSize(SQLITEDB, pSize, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemLevel01Data
 * 地図情報管理 (SYSTEM_LEVEL_01)  データ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemLevel01Data(char strBinData[]) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_DATA, SC_DA_SQL_SYSTEM_LEVEL_01);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationData(SQLITEDB, strBinData, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemLevel02Size
 * 地図情報管理 (SYSTEM_LEVEL_02) サイズ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemLevel02Size(INT32 *pSize) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_SIZE, SC_DA_SQL_SYSTEM_LEVEL_02);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationDataSize(SQLITEDB, pSize, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemLevel02Data
 * 地図情報管理 (SYSTEM_LEVEL_02)  データ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemLevel02Data(char strBinData[]) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_DATA, SC_DA_SQL_SYSTEM_LEVEL_02);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationData(SQLITEDB, strBinData, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemLevel03Size
 * 地図情報管理 (SYSTEM_LEVEL_03) サイズ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemLevel03Size(INT32 *pSize) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_SIZE, SC_DA_SQL_SYSTEM_LEVEL_03);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationDataSize(SQLITEDB, pSize, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemLevel03Data
 * 地図情報管理 (SYSTEM_LEVEL_03)  データ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemLevel03Data(char strBinData[]) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_DATA, SC_DA_SQL_SYSTEM_LEVEL_03);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationData(SQLITEDB, strBinData, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemLevel04Size
 * 地図情報管理 (SYSTEM_LEVEL_04) サイズ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemLevel04Size(INT32 *pSize) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_SIZE, SC_DA_SQL_SYSTEM_LEVEL_04);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationDataSize(SQLITEDB, pSize, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemLevel04Data
 * 地図情報管理 (SYSTEM_LEVEL_04)  データ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemLevel04Data(char strBinData[]) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_DATA, SC_DA_SQL_SYSTEM_LEVEL_04);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationData(SQLITEDB, strBinData, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemLevel05Size
 * 地図情報管理 (SYSTEM_LEVEL_05) サイズ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemLevel05Size(INT32 *pSize) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_SIZE, SC_DA_SQL_SYSTEM_LEVEL_05);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationDataSize(SQLITEDB, pSize, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemLevel05Data
 * 地図情報管理 (SYSTEM_LEVEL_05)  データ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemLevel05Data(char strBinData[]) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_DATA, SC_DA_SQL_SYSTEM_LEVEL_05);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationData(SQLITEDB, strBinData, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemLevel06Size
 * 地図情報管理 (SYSTEM_LEVEL_06) サイズ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemLevel06Size(INT32 *pSize) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_SIZE, SC_DA_SQL_SYSTEM_LEVEL_06);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationDataSize(SQLITEDB, pSize, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemLevel06Data
 * 地図情報管理 (SYSTEM_LEVEL_06)  データ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemLevel06Data(char strBinData[]) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_DATA, SC_DA_SQL_SYSTEM_LEVEL_06);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationData(SQLITEDB, strBinData, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemLevel07Size
 * 地図情報管理 (SYSTEM_LEVEL_07) サイズ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemLevel07Size(INT32 *pSize) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_SIZE, SC_DA_SQL_SYSTEM_LEVEL_07);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationDataSize(SQLITEDB, pSize, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemLevel07Data
 * 地図情報管理 (SYSTEM_LEVEL_07)  データ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemLevel07Data(char strBinData[]) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_DATA, SC_DA_SQL_SYSTEM_LEVEL_07);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationData(SQLITEDB, strBinData, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemLevel08Size
 * 地図情報管理 (SYSTEM_LEVEL_08) サイズ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemLevel08Size(INT32 *pSize) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_SIZE, SC_DA_SQL_SYSTEM_LEVEL_08);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationDataSize(SQLITEDB, pSize, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemLevel08Data
 * 地図情報管理 (SYSTEM_LEVEL_08)  データ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemLevel08Data(char strBinData[]) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_DATA, SC_DA_SQL_SYSTEM_LEVEL_08);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationData(SQLITEDB, strBinData, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemDataProviderSize
 * 地図情報管理 (SYSTEM_DATA_PROVIDER) サイズ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemDataProviderSize(INT32 *pSize) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_SIZE, SC_DA_SQL_SYSTEM_DATA_PROVIDER);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationDataSize(SQLITEDB, pSize, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemDataProviderData
 * 地図情報管理 (SYSTEM_DATA_PROVIDER)  データ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemDataProviderData(char strBinData[]) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_DATA, SC_DA_SQL_SYSTEM_DATA_PROVIDER);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationData(SQLITEDB, strBinData, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemCharacterCodeSize
 * 地図情報管理 (SYSTEM_CHARACTER_CODE) サイズ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemCharacterCodeSize(INT32 *pSize) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_SIZE, SC_DA_SQL_SYSTEM_CHARACTER_CODE);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationDataSize(SQLITEDB, pSize, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemCharacterCodeData
 * 地図情報管理 (SYSTEM_CHARACTER_CODE)  データ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemCharacterCodeData(char strBinData[]) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_DATA, SC_DA_SQL_SYSTEM_CHARACTER_CODE);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationData(SQLITEDB, strBinData, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemMapRangeSize
 * 地図情報管理 (SYSTEM_MAP_RANGE) サイズ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemMapRangeSize(INT32 *pSize) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_SIZE, SC_DA_SQL_SYSTEM_MAP_RANGE);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationDataSize(SQLITEDB, pSize, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemMapRangeData
 * 地図情報管理 (SYSTEM_MAP_RANGE)  データ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemMapRangeData(char strBinData[]) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_DATA, SC_DA_SQL_SYSTEM_MAP_RANGE);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationData(SQLITEDB, strBinData, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemMapVerDataSize
 * 地図情報管理 地図バージョン文字列(SYSTEM_MAP_VER) サイズ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemMapVerDataSize(INT32 *pMapVerSize) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_SIZE, SC_DA_SQL_SYSTEM_MAP_VER);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationDataSize(SQLITEDB, pMapVerSize, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemMapVerData
 * 地図情報管理 地図バージョン文字列(SYSTEM_MAP_VER)  データ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemMapVerData(char strBinMapVerData[]) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_DATA, SC_DA_SQL_SYSTEM_MAP_VER);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationData(SQLITEDB, strBinMapVerData, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemMapVerNoDataSize
 * 地図情報管理 地図のデータバ-ジョン(SYSTEM_MAP_VER_NO) サイズ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemMapVerNoDataSize(INT32 *pMapVerNoSize) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_SIZE, SC_DA_SQL_SYSTEM_MAP_VER_NO);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationDataSize(SQLITEDB, pMapVerNoSize, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemMapVerNoData
 * 地図情報管理 地図のデータバ-ジョン(SYSTEM_MAP_VER_NO) データ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemMapVerNoData(char strBinMapVerNoData[]) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_DATA, SC_DA_SQL_SYSTEM_MAP_VER_NO);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationData(SQLITEDB, strBinMapVerNoData, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemMapBuildNoDataSize
 * 地図情報管理 地図のデータビルド番号(SYSTEM_MAP_BUILD_NO) サイズ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemMapBuildNoDataSize(INT32 *pMapBuildNoSize) {
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_SIZE, SC_DA_SQL_SYSTEM_MAP_BUILD_NO);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationDataSize(SQLITEDB, pMapBuildNoSize, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemMapBuildNoData
 * 地図情報管理 地図のデータビルド番号(SYSTEM_MAP_BUILD_NO) データ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemMapBuildNoData(char strBinMapBuildNoData[]) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_DATA, SC_DA_SQL_SYSTEM_MAP_BUILD_NO);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationData(SQLITEDB, strBinMapBuildNoData, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetOsmOriginalTimestampDataSize
 * 地図情報管理 地図のオリジナル(OSM)のデータバ-ジョン(OSM_ORIGINAL_TIMESTAMP) サイズ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetOsmOriginalTimestampDataSize(INT32 *pOsmOriginalTimestampSize) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_SIZE, SC_DA_SQL_OSM_ORIGINAL_TIMESTAMP);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationDataSize(SQLITEDB, pOsmOriginalTimestampSize, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetOsmOriginalTimestampData
 * 地図情報管理 地図のオリジナル(OSM)のデータバ-ジョン(OSM_ORIGINAL_TIMESTAMP) データ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetOsmOriginalTimestampData(char strBinOsmOriginalTimestampData[]) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_DATA, SC_DA_SQL_OSM_ORIGINAL_TIMESTAMP);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationData(SQLITEDB, strBinOsmOriginalTimestampData, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}
/**----------------------------------------------------------------------------
 * SC_DA_GetSystemSeaFlag
 * 地図情報管理 収録されていないパーセルの背景描画方法(SYSTEM_SEA_FLAG) データ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_GetSystemSeaFlagData(char strBinData[]) {
	DAL_MUTEX_LOCK(mMutex);
	char query[SC_DA_SQL_LEN];
	sprintf(query, SC_DA_SQL_GET_SYSTEM_INFORMATION_DATA, SC_DA_SQL_SYSTEM_SEA_FLAG);
	SC_DA_RESULT daResult = SC_DA_GetSystemInformationData(SQLITEDB, strBinData, query);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}

/**
 * @brief SYSTEMINFO取得
 * @param dbPath
 * @param dbInfo
 */
SC_DA_RESULT SC_DA_GetSystemInfo(const char* dbPath, T_DAL_DBSYSTEMINFO* dbInfo) {
	SC_LOG_DebugPrint(SC_TAG_DAL, SC_LOG_START);

	sqlite3* sqliteObj = NULL;
	SC_DA_RESULT ret = SC_DA_RES_FAIL;

	SC_LOG_InfoPrint(SC_TAG_DAL, "param=%s", dbPath);

	if (NULL == dbPath) {
		if (NULL != SQLITEDB) {
			ret = SC_DA_LoadSystemInfo(SQLITEDB, SC_DA_SQL_GET_SYSTEM_INFO_DATA, dbInfo);
			if (SC_DA_RES_SUCCESS != ret) {
				SC_LOG_DebugPrint(SC_TAG_DAL, "SC_DA_LoadSystemInfo() fail.");
			}
		} else {
			SC_LOG_DebugPrint(SC_TAG_DAL, "SC_DA_GetSystemInfo() bad param.");
		}
	} else {
		if ((NULL != SQLITEDB) && (0 == strcmp(dbPath, READDBPATH))) {
			ret = SC_DA_LoadSystemInfo(SQLITEDB, SC_DA_SQL_GET_SYSTEM_INFO_DATA, dbInfo);
			if (SC_DA_RES_SUCCESS != ret) {
				SC_LOG_DebugPrint(SC_TAG_DAL, "SC_DA_LoadSystemInfo() fail.");
			}
		} else {
			// connect
			sqliteObj = SC_DA_DBConnect(dbPath);
			if (NULL == sqliteObj) {
				SC_LOG_DebugPrint(SC_TAG_DAL, "SC_DA_DBConnect() fail.");
				return (SC_DA_RES_RDB_ACCESSERR);
			}

			ret = SC_DA_LoadSystemInfo(sqliteObj, SC_DA_SQL_GET_SYSTEM_INFO_DATA, dbInfo);
			if (SC_DA_RES_SUCCESS != ret) {
				// 後続処理を続行
				SC_LOG_DebugPrint(SC_TAG_DAL, "SC_DA_LoadSystemInfo() fail.");
			}
			if (NULL != sqliteObj) {
				// disconnect
				if (SC_DA_RES_SUCCESS != SC_DA_DBDisconnect(sqliteObj)) {
					ret = SC_DA_RES_RDB_ACCESSERR;
					SC_LOG_DebugPrint(SC_TAG_DAL, "SC_DA_DBDisconnect() fail.");
				}
			}
		}
	}
#if 0
	SC_LOG_DebugPrint(SC_TAG_DAL, "system_format_version  %s",dbInfo->system_format_version);
	SC_LOG_DebugPrint(SC_TAG_DAL, "system_machine_type    %s",dbInfo->system_machine_type);
	SC_LOG_DebugPrint(SC_TAG_DAL, "system_app_version     %s",dbInfo->system_app_version);
	SC_LOG_DebugPrint(SC_TAG_DAL, "system_area            %s",dbInfo->system_area);
	SC_LOG_DebugPrint(SC_TAG_DAL, "system_lang            %s",dbInfo->system_lang);
	SC_LOG_DebugPrint(SC_TAG_DAL, "system_level_01        %s",dbInfo->system_level_01);
	SC_LOG_DebugPrint(SC_TAG_DAL, "system_level_02        %s",dbInfo->system_level_02);
	SC_LOG_DebugPrint(SC_TAG_DAL, "system_level_03        %s",dbInfo->system_level_03);
	SC_LOG_DebugPrint(SC_TAG_DAL, "system_level_04        %s",dbInfo->system_level_04);
	SC_LOG_DebugPrint(SC_TAG_DAL, "system_level_05        %s",dbInfo->system_level_05);
	SC_LOG_DebugPrint(SC_TAG_DAL, "system_level_06        %s",dbInfo->system_level_06);
	SC_LOG_DebugPrint(SC_TAG_DAL, "system_level_07        %s",dbInfo->system_level_07);
	SC_LOG_DebugPrint(SC_TAG_DAL, "system_level_08        %s",dbInfo->system_level_08);
	SC_LOG_DebugPrint(SC_TAG_DAL, "system_data_provider   %s",dbInfo->system_data_provider);
	SC_LOG_DebugPrint(SC_TAG_DAL, "system_character_code  %s",dbInfo->system_character_code);
	SC_LOG_DebugPrint(SC_TAG_DAL, "system_map_range       %s",dbInfo->system_map_range);
	SC_LOG_DebugPrint(SC_TAG_DAL, "system_initial_posi    %s",dbInfo->system_initial_posi);
	SC_LOG_DebugPrint(SC_TAG_DAL, "system_map_coordinate  %s",dbInfo->system_map_coordinate);
	SC_LOG_DebugPrint(SC_TAG_DAL, "system_map_ver         %s",dbInfo->system_map_ver);
	SC_LOG_DebugPrint(SC_TAG_DAL, "system_map_ver_no      %s",dbInfo->system_map_ver_no);
	SC_LOG_DebugPrint(SC_TAG_DAL, "system_map_build_no    %s",dbInfo->system_map_build_no);
	SC_LOG_DebugPrint(SC_TAG_DAL, "osm_original_timestamp %s",dbInfo->osm_original_timestamp);
	SC_LOG_DebugPrint(SC_TAG_DAL, "system_sea_flag        %s",dbInfo->system_sea_flag);
#endif
	SC_LOG_DebugPrint(SC_TAG_DAL, SC_LOG_END);
	return (ret);
}

//--------------------------#DOWNLOAD_AREA_MAP--------------------------
/**
 * @brief DOWNLOAD_AREA_MAP 取得
 * @param kind
 * @param downloadArea
 * @param dataCnt
 */
SC_DA_RESULT SC_DA_GetDlAreaMapData(UINT8 *kind, T_DAL_DLAREA *downloadArea, UINT8 *dataCnt) {
	DAL_MUTEX_LOCK(mMutex);
	SC_DA_RESULT daResult = SC_DA_LoadDownloadAreaMapData(SQLITEDB, kind, downloadArea, dataCnt);
	DAL_MUTEX_UNLOCK(mMutex);
	return (daResult);
}

//--------------------------#AREA_CLS-----------------------------------
/**
 * @brief AREA_CLS 取得
 * @param pAreaClsCode
 * @param pAreaClsList
 */
SC_DA_RESULT SC_DA_GetAreaClsList(SMAREACLSCODE* pAreaClsCode, T_DAL_AREA_CLS_LIST* pAreaClsList) {
	SC_LOG_DebugPrint(SC_TAG_DAL, SC_LOG_START);
	DAL_MUTEX_LOCK(mMutex);

	SC_DA_RESULT dal_res = SC_DA_RES_SUCCESS;
	E_SC_AREA_CLS areaCldCode = e_AREA_CLS1;

	// 地域クラスコード数分ループ ※地域クラス1は未取得
	for (areaCldCode = e_AREA_CLS1; areaCldCode < e_AREA_CLS_MAX; areaCldCode++) {
		dal_res = SC_DA_LoadAreaCls(SQLITEDB, SYS_LANGUAGE_JP, areaCldCode, pAreaClsCode, &pAreaClsList->areaCls[areaCldCode]);
		if (SC_DA_RES_SUCCESS != dal_res) {
			break;
		}
	}

	DAL_MUTEX_UNLOCK(mMutex);
	SC_LOG_DebugPrint(SC_TAG_DAL, SC_LOG_START);
	return (dal_res);
}
