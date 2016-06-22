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
 * SMDALDownloadAreaMap.c
 *
 *  Created on: 2014/08/04
 *      Author: masutani
 */

#include "SMCoreDALInternal.h"

/**----------------------------------------------------------------------------
 * SC_DA_GetDlMapAreaData
 * ダウンロードエリア データ取得
 *-----------------------------------------------------------------------------*/
SC_DA_RESULT SC_DA_LoadDownloadAreaMapData(sqlite3* sqliteObj, UINT8 *kind, T_DAL_DLAREA *downloadArea, UINT8 *dataCnt) {

	SC_DA_RESULT dal_res = SC_DA_RES_NODATA;
	INT32 sqlite_res = SQLITE_OK;
	sqlite3_stmt *stmt;
	INT32 size = 0;
	UINT8 resCnt = 0;

#ifdef	ANDROID
	__android_log_print(ANDROID_LOG_DEBUG, SC_TAG_DAL, "SC_DA_LoadDownloadAreaMapData() start");
#endif	// ANDROID

	do {
		if (NULL == kind) {
#ifdef	ANDROID
			__android_log_print(ANDROID_LOG_ERROR, SC_TAG_DAL, "SC_DA_LoadDownloadAreaMapData() param err[kind]");
#endif	// ANDROID
			dal_res = SC_DA_RES_BADPARAM;
			break;
		}
		if (NULL == sqliteObj) {
#ifdef	ANDROID
			__android_log_print(ANDROID_LOG_ERROR, SC_TAG_DAL, "SC_DA_LoadDownloadAreaMapData() param err[sqliteObj]");
#endif	// ANDROID
			dal_res = SC_DA_RES_BADPARAM;
			break;
		}

		// SQLITE prepare
		sqlite_res = sqlite3_prepare(sqliteObj, SC_DA_SQL_GET_DL_MAP_AREA_DATA, strlen(SC_DA_SQL_GET_DL_MAP_AREA_DATA), &stmt, NULL);
		if (SQLITE_OK != sqlite_res) {
#ifdef	ANDROID
			__android_log_print(ANDROID_LOG_ERROR, SC_TAG_DAL, "SC_DA_LoadDownloadAreaMapData() sqlite3_prepare err=%d", sqlite_res);
#endif	// ANDROID
			dal_res = SC_DA_RES_RDB_ACCESSERR;
			break;
		}

		// stmtの内部バッファを一旦クリア
		sqlite3_reset(stmt);

		// sqlの?の部分に、値を設定
		sqlite3_bind_text(stmt, 1, kind, strlen(kind), SQLITE_TRANSIENT);

		// SQLITE step
		while (SQLITE_ROW == (sqlite_res = sqlite3_step(stmt))) {

			// 取得データの格納
			// ダウンロード管理番号
			downloadArea[resCnt].id = sqlite3_column_int(stmt, 0);

			// データ種別
			size = sqlite3_column_bytes(stmt, 1);
			memcpy(downloadArea[resCnt].kind, (const char *) sqlite3_column_text(stmt, 1), size);
			downloadArea[resCnt].kind[size]='\0';

			// 国識別コード
			downloadArea[resCnt].countryCode = sqlite3_column_int(stmt, 2);

			// 表示順
			downloadArea[resCnt].displayNum = sqlite3_column_int(stmt, 3);

			// 国名称
			size = sqlite3_column_bytes(stmt, 4);
			memcpy(downloadArea[resCnt].countryName, (const char *) sqlite3_column_text(stmt, 4), size);
			downloadArea[resCnt].countryName[size]='\0';

			// グループ名称
			size = sqlite3_column_bytes(stmt, 5);
			memcpy(downloadArea[resCnt].areaGroup, (const char *) sqlite3_column_text(stmt, 5), size);
			downloadArea[resCnt].areaGroup[size]='\0';

			// エリア名称
			size = sqlite3_column_bytes(stmt, 6);
			memcpy(downloadArea[resCnt].areaName, (const char *) sqlite3_column_text(stmt, 6), size);
			downloadArea[resCnt].areaName[size]='\0';

			// ダウンロードフラグ
			downloadArea[resCnt].downloadFlag = sqlite3_column_int(stmt, 7);

			// ベースバージョン
			downloadArea[resCnt].baseVersion = sqlite3_column_int(stmt, 8);

			// 備考
			size = sqlite3_column_bytes(stmt, 9);
			memcpy(downloadArea[resCnt].note, (const char *) sqlite3_column_text(stmt, 9), size);
			downloadArea[resCnt].note[size]='\0';

			resCnt++;
			dal_res = SC_DA_RES_SUCCESS;
		}

		sqlite_res = sqlite3_finalize(stmt);
		if (SQLITE_OK != sqlite_res) {
			dal_res = SC_DA_RES_RDB_ACCESSERR;
			break;
		}
	} while (0);

	if (SC_DA_RES_SUCCESS != dal_res) {
#ifdef	ANDROID
		__android_log_print(ANDROID_LOG_ERROR, SC_TAG_DAL,
				"SC_DA_LoadDownloadAreaMapData() kind(%s) derr(0x%08x) serr(0x%08x) File %s(%d)", kind, dal_res, sqlite_res,
				__FILE__, __LINE__);
#endif	// ANDROID
	}

	// 件数を設定
	*dataCnt = resCnt;

#ifdef	ANDROID
	__android_log_print(ANDROID_LOG_DEBUG, SC_TAG_DAL, "SC_DA_LoadDownloadAreaMapData() end");
#endif	// ANDROID
	return (dal_res);
}



