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
 * SMDALSystemInfo.c
 *
 *  Created on: 2014/08/04
 *      Author: masutani
 */

#include "SMCoreDALInternal.h"


/**
 * @brief SC_DA_GetSystemInformationDataSize
 * @note 地図情報管理テーブル(SYSTEM_INFORMATION) データサイズ取得
 */
SC_DA_RESULT SC_DA_GetSystemInformationDataSize(sqlite3* sqliteObj, INT32 *pBinSize, char sql_query[]) {
#ifdef	ANDROID
	__android_log_print(ANDROID_LOG_DEBUG, SC_TAG_DAL, "SC_DA_GetSystemInformationDataSize() start");
#endif	// ANDROID

	SC_DA_RESULT dal_res = SC_DA_RES_SUCCESS;
	INT32 sqlite_res = SQLITE_OK;
	//char query[SC_DA_SQL_LEN]; //SQL文
	sqlite3_stmt *stmt;

	do {
		if (NULL == sqliteObj) {
			dal_res = SC_DA_RES_BADPARAM;
			break;
		}

		// SQLITE prepare
		sqlite_res = sqlite3_prepare(sqliteObj, sql_query, strlen(sql_query), &stmt, NULL);
		if (SQLITE_OK != sqlite_res) {
			dal_res = SC_DA_RES_RDB_ACCESSERR;
			break;
		}
		// SQLITE step
		while ((sqlite_res = sqlite3_step(stmt)) == SQLITE_ROW) {
			*pBinSize = sqlite3_column_int(stmt, 0);
		}
		//SQLITE finalize
		sqlite_res = sqlite3_finalize(stmt);
		if (SQLITE_OK != sqlite_res) {
			dal_res = SC_DA_RES_RDB_ACCESSERR;
			break;
		}
	} while (0);
	if (SC_DA_RES_SUCCESS != dal_res) {
#ifdef	ANDROID
		__android_log_print(ANDROID_LOG_ERROR, SC_TAG_DAL,
				"SC_DA_GetSystemInformationDataSize() derr(0x%08x) serr(0x%08x) File %s(%d)", dal_res, sqlite_res,
				__FILE__, __LINE__);
#endif	// ANDROID
	}
#ifdef	ANDROID
	__android_log_print(ANDROID_LOG_DEBUG, SC_TAG_DAL, "SC_DA_GetSystemInformationDataSize() end:%d",*pBinSize);
#endif	// ANDROID

	return (dal_res);
}

/**
 * @brief SC_DA_GetSystemInformationData
 * @note 地図情報管理テーブル(SYSTEM_INFORMATION) データ取得
 */
SC_DA_RESULT SC_DA_GetSystemInformationData(sqlite3* sqliteObj, char strBinData[], char sql_query[]) {
#ifdef	ANDROID
	__android_log_print(ANDROID_LOG_DEBUG, SC_TAG_DAL, "SC_DA_GetSystemInformationData() start");
#endif	// ANDROID

	SC_DA_RESULT dal_res = SC_DA_RES_NODATA;
	INT32 sqlite_res = SQLITE_OK;

	sqlite3_stmt *stmt;

	do {
		if (NULL == sqliteObj) {
			dal_res = SC_DA_RES_BADPARAM;
			break;
		}

		// SQLITE prepare
		sqlite_res = sqlite3_prepare(sqliteObj, sql_query, strlen(sql_query), &stmt, NULL);
		if (SQLITE_OK != sqlite_res) {
			dal_res = SC_DA_RES_RDB_ACCESSERR;
			break;
		}

		// SQLITE step
		while ((sqlite_res = sqlite3_step(stmt)) == SQLITE_ROW) {
			INT32 size = sqlite3_column_bytes(stmt, 0);
			// 取得データの格納
			memcpy(strBinData, (const char *) sqlite3_column_blob(stmt, 0), size);
			strBinData[size]='\0';
			dal_res = SC_DA_RES_SUCCESS;
		}

		//SQLITE finalize
		sqlite_res = sqlite3_finalize(stmt);
		if (SQLITE_OK != sqlite_res) {
			dal_res = SC_DA_RES_RDB_ACCESSERR;
			break;
		}
	} while (0);

	if (SC_DA_RES_SUCCESS != dal_res) {
#ifdef	ANDROID
		__android_log_print(ANDROID_LOG_ERROR, SC_TAG_DAL,
				"SC_DA_GetSystemInformationData() derr(0x%08x) serr(0x%08x) File %s(%d)",  dal_res, sqlite_res,
				__FILE__, __LINE__);
#endif	// ANDROID
	}
#ifdef	ANDROID
	__android_log_print(ANDROID_LOG_DEBUG, SC_TAG_DAL, "SC_DA_GetSystemInformationData() end:%s",strBinData);
#endif	// ANDROID

	return (dal_res);
}


/**
 * @brief SYSTEMINFO読み込み
 * @param sqliteDB
 * @param query
 * @param aInfo
 */
SC_DA_RESULT SC_DA_LoadSystemInfo(sqlite3* sqliteObj, char* query, T_DAL_DBSYSTEMINFO* aInfo) {
#ifdef	ANDROID
	__android_log_print(ANDROID_LOG_DEBUG, SC_TAG_DAL, "SC_DA_LoadSystemInfo() start");
#endif	// ANDROID

	sqlite3_stmt* stmt = NULL;

	char* dst = NULL;
	INT32 sqliteRet = SQLITE_ERROR;
	INT32 size = 0;
	INT32 bufSize = 0;
	//INT32 i = 0;

	if ((NULL == query) || (NULL == sqliteObj)) {
#ifdef	ANDROID
		__android_log_print(ANDROID_LOG_ERROR, SC_TAG_DAL, "SC_DA_LoadSystemInfo() bad param.");
#endif	// ANDROID
		return (SC_DA_RES_FAIL);
	}

#ifdef	ANDROID
	__android_log_print(ANDROID_LOG_DEBUG, SC_TAG_DAL, "query=%s", query);
#endif	// ANDROID

	// prepare
	if (SQLITE_OK != (sqliteRet = sqlite3_prepare(sqliteObj, query, strlen(query), &stmt, NULL))) {
#ifdef	ANDROID
		__android_log_print(ANDROID_LOG_ERROR, SC_TAG_DAL, "sqlite3_prepare fail. ret=%x", sqliteRet);
#endif	// ANDROID
		return (SC_DA_RES_RDB_ACCESSERR);
	}

	// stmtの内部バッファを一旦クリア
	sqlite3_reset(stmt);

	// SQLITE step
	while ((sqliteRet = sqlite3_step(stmt)) == SQLITE_ROW) {
		dst = NULL;
		if (strcmp(SC_DA_SQL_SYSTEM_FORMAT_VERSION, (const char *) sqlite3_column_blob(stmt, 0)) == 0) {
			dst = aInfo->system_format_version;
			bufSize = sizeof(aInfo->system_format_version);
		}
		if (strcmp(SC_DA_SQL_SYSTEM_MACHINE_TYPE, (const char *) sqlite3_column_blob(stmt, 0)) == 0) {
			dst = aInfo->system_machine_type;
			bufSize = sizeof(aInfo->system_machine_type);
		}
		if (strcmp(SC_DA_SQL_SYSTEM_APP_VERSION, (const char *) sqlite3_column_blob(stmt, 0)) == 0) {
			dst = aInfo->system_app_version;
			bufSize = sizeof(aInfo->system_app_version);
		}
		if (strcmp(SC_DA_SQL_SYSTEM_AREA, (const char *) sqlite3_column_blob(stmt, 0)) == 0) {
			dst = aInfo->system_area;
			bufSize = sizeof(aInfo->system_area);
		}
		if (strcmp(SC_DA_SQL_SYSTEM_LANG, (const char *) sqlite3_column_blob(stmt, 0)) == 0) {
			dst = aInfo->system_lang;
			bufSize = sizeof(aInfo->system_lang);
		}
		if (strcmp(SC_DA_SQL_SYSTEM_LEVEL_01, (const char *) sqlite3_column_blob(stmt, 0)) == 0) {
			dst = aInfo->system_level_01;
			bufSize = sizeof(aInfo->system_level_01);
		}
		if (strcmp(SC_DA_SQL_SYSTEM_LEVEL_02, (const char *) sqlite3_column_blob(stmt, 0)) == 0) {
			dst = aInfo->system_level_02;
			bufSize = sizeof(aInfo->system_level_02);
		}
		if (strcmp(SC_DA_SQL_SYSTEM_LEVEL_03, (const char *) sqlite3_column_blob(stmt, 0)) == 0) {
			dst = aInfo->system_level_03;
			bufSize = sizeof(aInfo->system_level_03);
		}
		if (strcmp(SC_DA_SQL_SYSTEM_LEVEL_04, (const char *) sqlite3_column_blob(stmt, 0)) == 0) {
			dst = aInfo->system_level_04;
			bufSize = sizeof(aInfo->system_level_04);
		}
		if (strcmp(SC_DA_SQL_SYSTEM_LEVEL_05, (const char *) sqlite3_column_blob(stmt, 0)) == 0) {
			dst = aInfo->system_level_05;
			bufSize = sizeof(aInfo->system_level_05);
		}
		if (strcmp(SC_DA_SQL_SYSTEM_LEVEL_06, (const char *) sqlite3_column_blob(stmt, 0)) == 0) {
			dst = aInfo->system_level_06;
			bufSize = sizeof(aInfo->system_level_06);
		}
		if (strcmp(SC_DA_SQL_SYSTEM_LEVEL_07, (const char *) sqlite3_column_blob(stmt, 0)) == 0) {
			dst = aInfo->system_level_07;
			bufSize = sizeof(aInfo->system_level_07);
		}
		if (strcmp(SC_DA_SQL_SYSTEM_LEVEL_08, (const char *) sqlite3_column_blob(stmt, 0)) == 0) {
			dst = aInfo->system_level_08;
			bufSize = sizeof(aInfo->system_level_08);
		}
		if (strcmp(SC_DA_SQL_SYSTEM_DATA_PROVIDER, (const char *) sqlite3_column_blob(stmt, 0)) == 0) {
			dst = aInfo->system_data_provider;
			bufSize = sizeof(aInfo->system_data_provider);
		}
		if (strcmp(SC_DA_SQL_SYSTEM_CHARACTER_CODE, (const char *) sqlite3_column_blob(stmt, 0)) == 0) {
			dst = aInfo->system_character_code;
			bufSize = sizeof(aInfo->system_character_code);
		}
		if (strcmp(SC_DA_SQL_SYSTEM_MAP_RANGE, (const char *) sqlite3_column_blob(stmt, 0)) == 0) {
			dst = aInfo->system_map_range;
			bufSize = sizeof(aInfo->system_map_range);
		}
		if (strcmp(SC_DA_SQL_SYSTEM_INITIAL_POSI, (const char *) sqlite3_column_blob(stmt, 0)) == 0) {
			dst = aInfo->system_initial_posi;
			bufSize = sizeof(aInfo->system_initial_posi);
		}
		if (strcmp(SC_DA_SQL_SYSTEM_MAP_COORDINATE, (const char *) sqlite3_column_blob(stmt, 0)) == 0) {
			dst = aInfo->system_map_coordinate;
			bufSize = sizeof(aInfo->system_map_coordinate);
		}
		if (strcmp(SC_DA_SQL_SYSTEM_MAP_VER, (const char *) sqlite3_column_blob(stmt, 0)) == 0) {
			dst = aInfo->system_map_ver;
			bufSize = sizeof(aInfo->system_map_ver);
		}
		if (strcmp(SC_DA_SQL_SYSTEM_MAP_VER_NO, (const char *) sqlite3_column_blob(stmt, 0)) == 0) {
			dst = aInfo->system_map_ver_no;
			bufSize = sizeof(aInfo->system_map_ver_no);
		}
		if (strcmp(SC_DA_SQL_SYSTEM_MAP_BUILD_NO, (const char *) sqlite3_column_blob(stmt, 0)) == 0) {
			dst = aInfo->system_map_build_no;
			bufSize = sizeof(aInfo->system_map_build_no);
		}
		if (strcmp(SC_DA_SQL_OSM_ORIGINAL_TIMESTAMP, (const char *) sqlite3_column_blob(stmt, 0)) == 0) {
			dst = aInfo->osm_original_timestamp;
			bufSize = sizeof(aInfo->osm_original_timestamp);
		}
		if (strcmp(SC_DA_SQL_SYSTEM_SEA_FLAG, (const char *) sqlite3_column_blob(stmt, 0)) == 0) {
			dst = aInfo->system_sea_flag;
			bufSize = sizeof(aInfo->system_sea_flag);
		}
		if (NULL != dst) {
			size = sqlite3_column_bytes(stmt, 1);
			if (bufSize > size) {
				memcpy(dst, (const char *) sqlite3_column_blob(stmt, 1), size);
				dst[size] = '\0';
			}
		} else {
#ifdef	ANDROID
			__android_log_print(ANDROID_LOG_ERROR, SC_TAG_DAL, "unknown data TYPE_CODE=%s", (const char *) sqlite3_column_blob(stmt, 0));
#endif	// ANDROID
		}
	}

	// finalize
	sqliteRet = sqlite3_finalize(stmt);
	if (SQLITE_OK != sqliteRet) {
#ifdef	ANDROID
		__android_log_print(ANDROID_LOG_ERROR, SC_TAG_DAL, "sqlite3_finalize fail. ret=%x", sqliteRet);
#endif	// ANDROID
		return (SC_DA_RES_RDB_ACCESSERR);
	}

#ifdef	ANDROID
	__android_log_print(ANDROID_LOG_DEBUG, SC_TAG_DAL, "SC_DA_LoadSystemInfo() end");
#endif	// ANDROID
	return (SC_DA_RES_SUCCESS);
}


