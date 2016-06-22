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
 * SMDALAreaCls.c
 *
 *  Created on: 2015/05/19
 *      Author: kanagawa
 */

#include "SMCoreDALInternal.h"

/**
 * @brief SC_DA_LoadAreaCls
 * @note 地域クラス取得
 */
SC_DA_RESULT SC_DA_LoadAreaCls(sqlite3* sqliteObj, UINT16 getLanCd, E_SC_AREA_CLS getAreaClsCode, SMAREACLSCODE* pAreaClsCode,
		T_DAL_AREA_CLS* pAreaCls) {
	SC_LOG_DebugPrint(SC_TAG_DAL, SC_LOG_START);

	SC_DA_RESULT dal_res = SC_DA_RES_SUCCESS;
	INT32 sqlite_res = SQLITE_OK;
	char sql_query[SC_DA_SQL_LEN] = {}; //SQL文
	sqlite3_stmt *stmt = NULL;
	INT32 size = 0;

	do {
		if (NULL == sqliteObj) {
			dal_res = SC_DA_RES_BADPARAM;
			break;
		}

		memset(pAreaCls, 0x00, sizeof(T_DAL_AREA_CLS));

		if (0 == pAreaClsCode->code[getAreaClsCode]) {
			// 取得コードが無効値(0)の場合
			dal_res = SC_DA_RES_SUCCESS;
			break;
		}

		// SQL文生成
		sprintf(sql_query, SC_DA_SQL_GET_AREA_CLS_DATA,
				(getAreaClsCode >= e_AREA_CLS3 ? pAreaClsCode->code[e_AREA_CLS3] : 0),	// 地域クラス3
				(getAreaClsCode >= e_AREA_CLS4 ? pAreaClsCode->code[e_AREA_CLS4] : 0),	// 地域クラス4
				(getAreaClsCode >= e_AREA_CLS5 ? pAreaClsCode->code[e_AREA_CLS5] : 0),	// 地域クラス5
				(getAreaClsCode >= e_AREA_CLS6 ? pAreaClsCode->code[e_AREA_CLS6] : 0),	// 地域クラス6
				(getAreaClsCode >= e_AREA_CLS2 ? pAreaClsCode->code[e_AREA_CLS2] : 0),	// 地域クラス2
				(getAreaClsCode >= e_AREA_CLS1 ? pAreaClsCode->code[e_AREA_CLS1] : 0),	// 地域クラス1
				getLanCd);																// 言語種別

		// SQLITE prepare
		sqlite_res = sqlite3_prepare(sqliteObj, sql_query, strlen(sql_query), &stmt, NULL);
		if (SQLITE_OK != sqlite_res) {
			dal_res = SC_DA_RES_RDB_ACCESSERR;
			break;
		}

		// SQLITE step
		while ((sqlite_res = sqlite3_step(stmt)) == SQLITE_ROW) {
			// 地域クラス名称
			size = sqlite3_column_bytes(stmt, 0);
			if (size > 0) {
				memcpy((void*) pAreaCls->name, (const void*) sqlite3_column_text(stmt, 0), size);
				pAreaCls->name[size] = '\0';
			}

			// 地域クラス読み
			size = sqlite3_column_bytes(stmt, 1);
			if (size > 0) {
				memcpy((void*) pAreaCls->yomi, (const void*) sqlite3_column_text(stmt, 1), size);
				pAreaCls->yomi[size] = '\0';
			}

			break;
		}

		//SQLITE finalize
		sqlite_res = sqlite3_finalize(stmt);
		if (SQLITE_OK != sqlite_res) {
			dal_res = SC_DA_RES_RDB_ACCESSERR;
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_DAL, SC_LOG_END);
	return (dal_res);
}
