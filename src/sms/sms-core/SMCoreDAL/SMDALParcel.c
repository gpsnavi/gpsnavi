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
 * SMDALParcel.c
 *
 *  Created on: 2014/08/04
 *      Author: masutani
 */


#include "SMCoreDALInternal.h"

// クエリ
static Char* da_sql_get_size[] = {
		SC_DA_SQL_GET_PARCEL_SHAPE_SIZE,		// 形状
		SC_DA_SQL_GET_PARCEL_BKGD_SIZE,			// 背景
		SC_DA_SQL_GET_PARCEL_ROAD_SIZE,			// ネットワーク
		SC_DA_SQL_GET_PARCEL_GUIDE_SIZE,		// 誘導
		SC_DA_SQL_GET_PARCEL_NAME_SIZE,			// 名称
		SC_DA_SQL_GET_PARCEL_DENSITY_SIZE,		// 密度
		SC_DA_SQL_GET_PARCEL_MARK_SIZE,			// 記号背景
		SC_DA_SQL_GET_PARCEL_ROAD_NAME_SIZE,	// 道路名称
		SC_DA_SQL_GET_PARCEL_BASIS_SIZE,		// パーセル基本情報
		SC_DA_SQL_GET_PARCEL_BKGD_AREA_CLS_SIZE,// 背景地域クラス
};

/*-------------------------------------------------------------------
 * プロトタイプ
 *-------------------------------------------------------------------*/
UINT32 DA_GetKindMask(UINT8 kind);
char* DA_GetPclCol(UINT8 kind);

/**
 * @brief SC_DA_GetPclDataSize
 * @note データサイズ取得
 */
SC_DA_RESULT SC_DA_GetPclDataSize(sqlite3* sqliteObj, T_DAL_PID *pstPid, UINT32 *pBinSize, UINT16 type) {
#ifdef	ANDROID
	__android_log_print(ANDROID_LOG_DEBUG, SC_TAG_DAL, "SC_DA_GetPclDataSize() start");
#endif	// ANDROID

	SC_DA_RESULT dal_res = SC_DA_RES_SUCCESS;
	INT32 sqlite_res = SQLITE_OK;

	char query[SC_DA_SQL_LEN];
	sqlite3_stmt *stmt;

	do {
		if (NULL == sqliteObj) {
			dal_res = SC_DA_RES_BADPARAM;
			break;
		}

		sprintf(query, da_sql_get_size[type], pstPid->parcelId);
		sqlite_res = sqlite3_prepare(sqliteObj, query, strlen(query), &stmt, NULL);
		if (SQLITE_OK != sqlite_res) {
			dal_res = SC_DA_RES_RDB_ACCESSERR;
			break;
		}

		while ((sqlite_res = sqlite3_step(stmt)) == SQLITE_ROW) {
			*pBinSize = sqlite3_column_int(stmt, 0);
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
				"SC_DA_GetPclData() type(%d) derr(0x%08x) serr(0x%08x) File %s(%d)", type, dal_res, sqlite_res,
				__FILE__, __LINE__);
#endif	// ANDROID
	}
#ifdef	ANDROID
	__android_log_print(ANDROID_LOG_DEBUG, SC_TAG_DAL, "SC_DA_GetPclDataSize() end");
#endif	// ANDROID

	return (dal_res);
}

/**
 * @brief SC_DA_GetPclData
 * @note データ取得
 */
SC_DA_RESULT SC_DA_LoadPclData(sqlite3* sqliteObj, T_DAL_PID *pstPid, UINT32 mapKind, T_DAL_BINARY *pstBinData) {
//	__android_log_print(ANDROID_LOG_DEBUG, DAL_TAG, "SC_DA_LoadPclData() start");

	SC_DA_RESULT dal_res = SC_DA_RES_NODATA;
	INT32 sqlite_res = SQLITE_OK;
	UINT8* workAddr;
	INT32 size;
	Bool kindFlg = false;
	UINT16 i;
	UINT8  kind;
	UINT32 mask;
	INT32  columnType;

	char query[SC_DA_SQL_LEN];
	char select[SC_DA_COL_LEN] = {};
	sqlite3_stmt *stmt;

	do {
		if (NULL == sqliteObj || NULL == pstPid) {
			dal_res = SC_DA_RES_BADPARAM;
			break;
		}

		if (0 == mapKind) {
			dal_res = SC_DA_RES_BADPARAM;
			break;
		}

		// SELECTするカラムを作成（要求されたデータ種別のみ）
		for (kind = 0; kind < e_DATA_KIND_END; kind++) {
			// データ種別のフラグが立っていれば、カラム名を追加
			mask = DA_GetKindMask(kind);
			if (0 < mask && mask == (mapKind & mask)) {
				strcat(select, DA_GetPclCol(kind));
			}
		}
		// 最後に"PARCEL_ID"を追加する
		strcat(select, SC_DA_COL_PARCEL_ID);

		// SQL作成
		sprintf(query, SC_DA_SQL_GET_PARCEL_DATA, select, pstPid->parcelId);
		// SQL実行
		sqlite_res = sqlite3_prepare(sqliteObj, query, strlen(query), &stmt, NULL);
		if (SQLITE_OK != sqlite_res) {
			dal_res = SC_DA_RES_RDB_ACCESSERR;
			break;
		}

		workAddr = pstBinData->pBufferAddr;
		i = 0;

		while ((sqlite_res = sqlite3_step(stmt)) == SQLITE_ROW) {

			for (kind = 0; kind < e_DATA_KIND_END; kind++) {

				columnType = sqlite3_column_type(stmt, i);

				mask = DA_GetKindMask(kind);
				if (0 < mask && mask == (mapKind & mask)) {
					if (columnType == SQLITE_INTEGER) {
						size = sizeof(INT32);
					} else {
						size = sqlite3_column_bytes(stmt, i);
					}
					kindFlg = true;
				} else {
					size = 0;
				}

				// バッファオーバーチェック
				pstBinData->binDataSize = pstBinData->binDataSize + sizeof(size) + size;
				if (pstBinData->bufferSize < pstBinData->binDataSize) {
					//エラー
					dal_res = SC_DA_RES_FAIL;
					break;
				}

				// データ格納
				if (kindFlg == true) {
					if (columnType == SQLITE_INTEGER) {
						INT32 iValue = sqlite3_column_int(stmt, i);
						memcpy((workAddr + sizeof(size)), (const char *) &iValue, size);
					} else {
						memcpy((workAddr + sizeof(size)), (const char *) sqlite3_column_blob(stmt, i), size);
					}
					i++;
					kindFlg = false;
				}

				// サイズ格納
				*((UINT32*)workAddr) = size;

				// 格納先アドレスを移動
				workAddr = workAddr + sizeof(size) + size;

				dal_res = SC_DA_RES_SUCCESS;
			}
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
				"SC_DA_LoadPclData() derr(0x%08x) serr(0x%08x) File %s(%d)", dal_res, sqlite_res,
				__FILE__, __LINE__);
#endif	// ANDROID
	}
//	__android_log_print(ANDROID_LOG_DEBUG, DAL_TAG, "SC_DA_LoadPclData() end");

	return (dal_res);
}

UINT32 DA_GetKindMask(UINT8 kind) {

	UINT32 mask = 0;

	switch (kind) {
	case e_DATA_KIND_ROAD:
		mask = SC_DHC_KIND_ROAD;
		break;
	case e_DATA_KIND_SHAPE:
		mask = SC_DHC_KIND_SHAPE;
		break;
	case e_DATA_KIND_GUIDE:
		mask = SC_DHC_KIND_GUIDE;
		break;
	case e_DATA_KIND_BKGD:
		mask = SC_DHC_KIND_BKGD;
		break;
	case e_DATA_KIND_NAME:
		mask = SC_DHC_KIND_NAME;
		break;
	case e_DATA_KIND_ROAD_NAME:
		mask = SC_DHC_KIND_ROAD_NAME;
		break;
	case e_DATA_KIND_DENSITY:
		mask = SC_DHC_KIND_DENSITY;
		break;
	case e_DATA_KIND_MARK:
		mask = SC_DHC_KIND_MARK;
		break;
	case e_DATA_KIND_PARCEL_BASIS:
		mask = SC_DHC_KIND_PARCEL_BASIS;
		break;
	case e_DATA_KIND_ROAD_BASE_VERSION:
		mask = SC_DHC_KIND_ROAD_BASE_VER;
		break;
	case e_DATA_KIND_BKGD_AREA_CLS:
		mask = SC_DHC_KIND_BKGD_AREA_CLS;
		break;
	default:
		// 不明種別は来ない前提の為、何もしない
		break;
	}
	return (mask);
}

char* DA_GetPclCol(UINT8 kind) {

	char* column = NULL;

	switch (kind) {
	case e_DATA_KIND_ROAD:
		column = SC_DA_COL_PARCEL_ROAD;
		break;
	case e_DATA_KIND_SHAPE:
		column = SC_DA_COL_PARCEL_SHAPE;
		break;
	case e_DATA_KIND_GUIDE:
		column = SC_DA_COL_PARCEL_GUIDE;
		break;
	case e_DATA_KIND_BKGD:
		column = SC_DA_COL_PARCEL_BKGD;
		break;
	case e_DATA_KIND_NAME:
		column = SC_DA_COL_PARCEL_NAME;
		break;
	case e_DATA_KIND_ROAD_NAME:
		column = SC_DA_COL_PARCEL_ROAD_NAME;
		break;
	case e_DATA_KIND_DENSITY:
		column = SC_DA_COL_PARCEL_DENSITY;
		break;
	case e_DATA_KIND_MARK:
		column = SC_DA_COL_PARCEL_MARK;
		break;
	case e_DATA_KIND_PARCEL_BASIS:
		column = SC_DA_COL_PARCEL_BASIS;
		break;
	case e_DATA_KIND_ROAD_BASE_VERSION:
		column = SC_DA_COL_PARCEL_ROAD_BASE_VERSION;
		break;
	case e_DATA_KIND_BKGD_AREA_CLS:
		column = SC_DA_COL_PARCEL_BKGD_AREA_CLS;
		break;
	default:
		// 不明種別は来ない前提の為、何もしない
		break;
	}
	return (column);
}
