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
 * SMCorePoiFavorite.cpp
 *
 *  Created on: 2015/11/05
 *      Author: masutani
 */

#include "SMCoreDALInternal.h"


static  char  poi_db_full_name[256];
static	void log_dbg( const char *fmt, ...);

/**
 * @brief ＰＯＩ用フォルダ初期化
 * @param[in] db_path 	ＰＯＩ・ＤＢの格納フォルダ
 * @return				処理結果
 */
E_PAL_RESULT SC_POI_FAVORITE_TBL_Initialize(const char* db_path) {
	int rc;
	sqlite3 *db;
	sqlite3_stmt *st;
	//bool rc_bool = false;
	char *zErrMsg = 0;

	sprintf(poi_db_full_name, "%spoi.db", db_path);

	rc = sqlite3_open(poi_db_full_name, &db);
	if (rc != SQLITE_OK) {
		log_dbg("SC_POI_Initialize   rc != SQLITE_OK  pat...%s   d\n", poi_db_full_name);
		return (e_PAL_RESULT_ACCESS_ERR);
	}

	std::string param_str;
	std::string result;

	// お気に入りテーブル
	param_str = "SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name='SM_GC_POI_FAVORITE_TBL';";
	rc = sqlite3_prepare_v2(db, (const char*)param_str.c_str(), -1, &st, NULL);
	while ((rc = sqlite3_step(st)) == SQLITE_ROW) {
		char *text = (char*) sqlite3_column_text(st, 0);
		result = text;
		log_dbg("JNI  table_cnt...%s", result.c_str());
		break;
	}
	sqlite3_finalize(st);

	if (result == "0") {
		const char* sql_stm = "create table SM_GC_POI_FAVORITE_TBL("
				" userid text,"
				" dbtime text,"
				" contentstime text,"
				" ctgry_code text,"
				" id text,"
				" url text,"
				" name text,"
				" pos_name text,"
				" contents text,"
				" binary_data blob,"
				" binary_data_len integer,"
				" lat integer,"
				" log integer"
				");";

		rc = sqlite3_exec(db, sql_stm, NULL, 0, &zErrMsg);
		if (rc != SQLITE_OK) {
			log_dbg("create table SM_GC_POI_FAVORITE_TBL   rc != SQLITE_OK  \n");
			return (e_PAL_RESULT_ACCESS_ERR);
		}

		log_dbg("お気に入りテーブル登録");
	} else {
		log_dbg("お気に入りテーブル登録済み");
	}

	rc = sqlite3_close(db);

	return (e_PAL_RESULT_SUCCESS);

}

/**
 * @brief お気に入りテーブルの追加
 * @param[in] 		お気に入りテーブル・データ
 * @return			SQL処理結果
 */
int SC_POI_addGCfavoriteTBL(SM_GC_POI_FAVORITE_TBL* favorite_tbl_ptr) {
	int rc = e_PAL_RESULT_UPDATE_ERR;
	sqlite3 *db;
	sqlite3_stmt *stmt = NULL;

	rc = sqlite3_open(poi_db_full_name, &db);
	if (rc != SQLITE_OK) {
		log_dbg("SC_POI_addGCfavoriteTBL   sqlite3_open  rc != SQLITE_OK  pat...%s   d\n", poi_db_full_name);
		return (e_PAL_RESULT_ACCESS_ERR);
	}

	const char *select_sql_0 = "select * from SM_GC_POI_FAVORITE_TBL where userid = ? and dbtime = ?";
	sqlite3_prepare_v2(db, select_sql_0, strlen(select_sql_0), &stmt, NULL);
	// stmtの内部バッファを一旦クリア
	sqlite3_reset(stmt);

	sqlite3_bind_text(stmt, 1, (const char*)favorite_tbl_ptr->userid.c_str(), strlen((const char*)favorite_tbl_ptr->userid.c_str()), SQLITE_TRANSIENT );
	sqlite3_bind_text(stmt, 2, (const char*)favorite_tbl_ptr->dbtime.c_str(), strlen((const char*)favorite_tbl_ptr->dbtime.c_str()), SQLITE_TRANSIENT );

	// stmtのSQLを実行
	int loop = 0;
	rc = e_PAL_RESULT_ACCESS_ERR;
	int r;
	while (SQLITE_ROW == (r = sqlite3_step(stmt))) {
		loop++;
	}
	if (loop > 0) {
		rc = e_PAL_RESULT_SUCCESS;
		log_dbg("SC_POI_chgGCfavoriteTBL   select  OK   userid...%s  dbtime...%s  loop...%d  \n", favorite_tbl_ptr->userid.c_str(),
				favorite_tbl_ptr->dbtime.c_str(), loop);
	} else {
		rc = e_PAL_RESULT_ACCESS_ERR;
		log_dbg("SC_POI_chgGCfavoriteTBL   select  NG   userid...%s  dbtime...%s  loop...%d  \n", favorite_tbl_ptr->userid.c_str(),
				favorite_tbl_ptr->dbtime.c_str(), loop);
	}
	// stmt を開放
	sqlite3_finalize(stmt);

	// 同じキーのレコードが存在する時
	if (rc == e_PAL_RESULT_SUCCESS) {
		sqlite3_close(db);
		return (e_PAL_RESULT_DUPLICATE_ERR);
	}

	// 	stmt を生成する
	//	年月日の新しいものから並べ替える。
	const char *select_sql = "select * from SM_GC_POI_FAVORITE_TBL where userid = ? order by dbtime desc";
	sqlite3_prepare_v2(db, select_sql, strlen(select_sql), &stmt, NULL);
	// stmtの内部バッファを一旦クリア
	sqlite3_reset(stmt);
	sqlite3_bind_text(stmt, 1, (const char*)favorite_tbl_ptr->userid.c_str(), strlen((const char*)favorite_tbl_ptr->userid.c_str()), SQLITE_TRANSIENT );

	rc = e_PAL_RESULT_UPDATE_ERR;
	int sel_cnt = 0;
	SM_GC_POI_FAVORITE_TBL favorite_tbl;
	std::vector<SM_GC_POI_FAVORITE_TBL> favorite_tbl_list;
	while (SQLITE_ROW == (r = sqlite3_step(stmt))) {
		sel_cnt++;
		rc = e_PAL_RESULT_SUCCESS;
	}
	// stmt を開放
	sqlite3_finalize(stmt);

	if (sel_cnt >= SM_GC_POI_FAVORITE_TBL_MAX_COUNT) {
		log_dbg("SC_POI_addGCfavoriteTBL  NG  MAX_Over   userid...%s  dbtime...%s  \n", favorite_tbl_ptr->userid.c_str(),
				favorite_tbl_ptr->dbtime.c_str());
		sqlite3_close(db);
		return (e_PAL_RESULT_ADD_OVER_ERR);
	}

	// stmt を生成する
	const char *sql =
			"insert into SM_GC_POI_FAVORITE_TBL (userid, dbtime, contentstime, ctgry_code, id, url, name, pos_name, contents, binary_data, binary_data_len, lat, log) values(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

	sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	// stmtの内部バッファを一旦クリア
	sqlite3_reset(stmt);

	sqlite3_bind_text(stmt, 1, (const char*)favorite_tbl_ptr->userid.c_str(), strlen((const char*)favorite_tbl_ptr->userid.c_str()), SQLITE_TRANSIENT );
	sqlite3_bind_text(stmt, 2, (const char*)favorite_tbl_ptr->dbtime.c_str(), strlen((const char*)favorite_tbl_ptr->dbtime.c_str()), SQLITE_TRANSIENT );
	sqlite3_bind_text(stmt, 3, (const char*)favorite_tbl_ptr->contentstime.c_str(), strlen((const char*)favorite_tbl_ptr->contentstime.c_str()), SQLITE_TRANSIENT );
	sqlite3_bind_text(stmt, 4, (const char*)favorite_tbl_ptr->ctgry_code.c_str(), strlen((const char*)favorite_tbl_ptr->ctgry_code.c_str()), SQLITE_TRANSIENT );
	sqlite3_bind_text(stmt, 5, (const char*)favorite_tbl_ptr->id.c_str(), strlen((const char*)favorite_tbl_ptr->id.c_str()), SQLITE_TRANSIENT );
	sqlite3_bind_text(stmt, 6, (const char*)favorite_tbl_ptr->url.c_str(), strlen((const char*)favorite_tbl_ptr->url.c_str()), SQLITE_TRANSIENT );
	sqlite3_bind_text(stmt, 7, (const char*)favorite_tbl_ptr->name.c_str(), strlen((const char*)favorite_tbl_ptr->name.c_str()), SQLITE_TRANSIENT );
	sqlite3_bind_text(stmt, 8, (const char*)favorite_tbl_ptr->pos_name.c_str(), strlen((const char*)favorite_tbl_ptr->pos_name.c_str()), SQLITE_TRANSIENT );
	sqlite3_bind_text(stmt, 9, (const char*)favorite_tbl_ptr->contents.c_str(), strlen((const char*)favorite_tbl_ptr->contents.c_str()), SQLITE_TRANSIENT );
	if (favorite_tbl_ptr->binary_data_len) {
		sqlite3_bind_blob(stmt, 10, favorite_tbl_ptr->binary_data, favorite_tbl_ptr->binary_data_len, NULL);
	}
	sqlite3_bind_int(stmt, 11, favorite_tbl_ptr->binary_data_len);
	sqlite3_bind_int(stmt, 12, favorite_tbl_ptr->lat);
	sqlite3_bind_int(stmt, 13, favorite_tbl_ptr->log);

	// stmtのSQLを実行
	rc = e_PAL_RESULT_SUCCESS;
	loop = 0;
	while (SQLITE_DONE != sqlite3_step(stmt)) {
		if (loop++ > 10) {
			rc = e_PAL_RESULT_UPDATE_ERR;
			break;
		}
	}

	if (rc == e_PAL_RESULT_SUCCESS) {
		log_dbg("SC_POI_addGCfavoriteTBL  OK   userid...%s  dbtime...%s  \n", favorite_tbl_ptr->userid.c_str(),
				favorite_tbl_ptr->dbtime.c_str());
	} else {
		log_dbg("SC_POI_addGCfavoriteTBL  NG   userid...%s  dbtime...%s  \n", favorite_tbl_ptr->userid.c_str(),
				favorite_tbl_ptr->dbtime.c_str());
	}

	// stmt を開放
	sqlite3_finalize(stmt);
	sqlite3_close(db);

	return (rc);
}

/**
 * @brief お気に入りテーブルの修正
 * @param[in] 		お気に入りテーブル・データ
 * @return			SQL処理結果
 */
int SC_POI_chgGCfavoriteTBL(SM_GC_POI_FAVORITE_TBL* favorite_tbl_ptr) {
	int rc = e_PAL_RESULT_UPDATE_ERR;
	sqlite3 *db;
	sqlite3_stmt *stmt = NULL;

	rc = sqlite3_open(poi_db_full_name, &db);
	if (rc != SQLITE_OK) {
		log_dbg("SC_POI_chgGCfavoriteTBL   sqlite3_open  rc != SQLITE_OK  pat...%s   d\n", poi_db_full_name);
		return (e_PAL_RESULT_ACCESS_ERR);
	}

	const char *select_sql = "select * from SM_GC_POI_FAVORITE_TBL where userid = ? and dbtime = ?";
	sqlite3_prepare_v2(db, select_sql, strlen(select_sql), &stmt, NULL);
	// stmtの内部バッファを一旦クリア
	sqlite3_reset(stmt);

	sqlite3_bind_text(stmt, 1, (const char*)favorite_tbl_ptr->userid.c_str(), strlen((const char*)favorite_tbl_ptr->userid.c_str()), SQLITE_TRANSIENT );
	sqlite3_bind_text(stmt, 2, (const char*)favorite_tbl_ptr->dbtime.c_str(), strlen((const char*)favorite_tbl_ptr->dbtime.c_str()), SQLITE_TRANSIENT );

	// stmtのSQLを実行
	int loop = 0;
	rc = e_PAL_RESULT_UPDATE_ERR;
	int r;
	while (SQLITE_ROW == (r = sqlite3_step(stmt))) {
		loop++;
	}
	if (loop > 0) {
		rc = e_PAL_RESULT_SUCCESS;
		log_dbg("SC_POI_chgGCfavoriteTBL   select  OK   userid...%s  dbtime...%s  loop...%d  \n", favorite_tbl_ptr->userid.c_str(),
				favorite_tbl_ptr->dbtime.c_str(), loop);
	} else {
		rc = e_PAL_RESULT_UPDATE_ERR;
		log_dbg("SC_POI_chgGCfavoriteTBL   select  NG   userid...%s  dbtime...%s  loop...%d  \n", favorite_tbl_ptr->userid.c_str(),
				favorite_tbl_ptr->dbtime.c_str(), loop);
	}
	// stmt を開放
	sqlite3_finalize(stmt);

	if (rc == e_PAL_RESULT_UPDATE_ERR) {
		sqlite3_close(db);
		return (rc);
	}

	// stmt を生成する
	const char *sql =
			"update SM_GC_POI_FAVORITE_TBL set contentstime = ?,ctgry_code = ?,id = ?,url = ?,name = ?,pos_name = ?,contents = ?,binary_data = ?,binary_data_len = ?,lat = ?,log = ?  where userid = ? and dbtime = ?";
	sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	// stmtの内部バッファを一旦クリア
	sqlite3_reset(stmt);

	sqlite3_bind_text(stmt, 1, (const char*)favorite_tbl_ptr->contentstime.c_str(), strlen((const char*)favorite_tbl_ptr->contentstime.c_str()), SQLITE_TRANSIENT );
	sqlite3_bind_text(stmt, 2, (const char*)favorite_tbl_ptr->ctgry_code.c_str(), strlen((const char*)favorite_tbl_ptr->ctgry_code.c_str()), SQLITE_TRANSIENT );
	sqlite3_bind_text(stmt, 3, (const char*)favorite_tbl_ptr->id.c_str(), strlen((const char*)favorite_tbl_ptr->id.c_str()), SQLITE_TRANSIENT );
	sqlite3_bind_text(stmt, 4, (const char*)favorite_tbl_ptr->url.c_str(), strlen((const char*)favorite_tbl_ptr->url.c_str()), SQLITE_TRANSIENT );
	sqlite3_bind_text(stmt, 5, (const char*)favorite_tbl_ptr->name.c_str(), strlen((const char*)favorite_tbl_ptr->name.c_str()), SQLITE_TRANSIENT );
	sqlite3_bind_text(stmt, 6, (const char*)favorite_tbl_ptr->pos_name.c_str(), strlen((const char*)favorite_tbl_ptr->pos_name.c_str()), SQLITE_TRANSIENT );
	sqlite3_bind_text(stmt, 7, (const char*)favorite_tbl_ptr->contents.c_str(), strlen((const char*)favorite_tbl_ptr->contents.c_str()), SQLITE_TRANSIENT );

	if (favorite_tbl_ptr->binary_data_len) {
		sqlite3_bind_blob(stmt, 8, favorite_tbl_ptr->binary_data, favorite_tbl_ptr->binary_data_len, NULL);
	}

	sqlite3_bind_int(stmt, 9, favorite_tbl_ptr->binary_data_len);
	sqlite3_bind_int(stmt, 10, favorite_tbl_ptr->lat);
	sqlite3_bind_int(stmt, 11, favorite_tbl_ptr->log);
	sqlite3_bind_text(stmt, 12, (const char*)favorite_tbl_ptr->userid.c_str(), strlen((const char*)favorite_tbl_ptr->userid.c_str()), SQLITE_TRANSIENT );
	sqlite3_bind_text(stmt, 13, (const char*)favorite_tbl_ptr->dbtime.c_str(), strlen((const char*)favorite_tbl_ptr->dbtime.c_str()), SQLITE_TRANSIENT );

	// stmtのSQLを実行
	loop = 0;
	rc = e_PAL_RESULT_UPDATE_ERR;
	while (1) {
		if (SQLITE_DONE == sqlite3_step(stmt)) {
			rc = e_PAL_RESULT_SUCCESS;
			break;
		}
		if (loop++ > 10) {
			rc = e_PAL_RESULT_UPDATE_ERR;
			break;
		}
	}

	if (favorite_tbl_ptr->binary_data) {
		delete[] favorite_tbl_ptr->binary_data;
	}

	// stmt を開放
	sqlite3_finalize(stmt);
	sqlite3_close(db);

	return (rc);
}

/**
 * @brief お気に入りテーブルの削除
 * @param[in] 		お気に入りテーブル・データ
 * @return			SQL処理結果
 */
int SC_POI_delGCfavoriteTBL(SM_GC_POI_FAVORITE_TBL* favorite_tbl_ptr) {
	int rc = e_PAL_RESULT_DELETE_ERR;
	sqlite3 *db;

	rc = sqlite3_open(poi_db_full_name, &db);
	if (rc != SQLITE_OK) {
		log_dbg("SC_POI_delGCfavoriteTBL   sqlite3_open  rc != SQLITE_OK  pat...%s   d\n", poi_db_full_name);
		return (e_PAL_RESULT_ACCESS_ERR);
	}

	sqlite3_stmt *stmt = NULL;
	const char *select_sql = "select * from SM_GC_POI_FAVORITE_TBL where userid = ? and dbtime = ?";
	sqlite3_prepare_v2(db, select_sql, strlen(select_sql), &stmt, NULL);
	// stmtの内部バッファを一旦クリア
	sqlite3_reset(stmt);

	sqlite3_bind_text(stmt, 1, (const char*)favorite_tbl_ptr->userid.c_str(), strlen((const char*)favorite_tbl_ptr->userid.c_str()), SQLITE_TRANSIENT );
	sqlite3_bind_text(stmt, 2, (const char*)favorite_tbl_ptr->dbtime.c_str(), strlen((const char*)favorite_tbl_ptr->dbtime.c_str()), SQLITE_TRANSIENT );

	// stmtのSQLを実行
	int loop = 0;
	rc = e_PAL_RESULT_DELETE_ERR;
	int r;
	while (SQLITE_ROW == (r = sqlite3_step(stmt))) {
		loop++;
	}
	if (loop > 0) {
		rc = e_PAL_RESULT_SUCCESS;
		log_dbg("SC_POI_delGCfavoriteTBL   select  OK   userid...%s  dbtime...%s  loop...%d  \n", favorite_tbl_ptr->userid.c_str(),
				favorite_tbl_ptr->dbtime.c_str(), loop);
	} else {
		rc = e_PAL_RESULT_DELETE_ERR;
		log_dbg("SC_POI_delGCfavoriteTBL   select  NG   userid...%s  dbtime...%s  loop...%d  \n", favorite_tbl_ptr->userid.c_str(),
				favorite_tbl_ptr->dbtime.c_str(), loop);
	}
	// stmt を開放
	sqlite3_finalize(stmt);

	if (rc == e_PAL_RESULT_DELETE_ERR) {
		sqlite3_close(db);
		return (rc);
	}

	// stmt を生成する
	const char *sql = "delete from SM_GC_POI_FAVORITE_TBL where userid = ?  and dbtime = ?";
	sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	// stmtの内部バッファを一旦クリア
	sqlite3_reset(stmt);
	sqlite3_bind_text(stmt, 1, (const char*)favorite_tbl_ptr->userid.c_str(), strlen((const char*)favorite_tbl_ptr->userid.c_str()), SQLITE_TRANSIENT );
	sqlite3_bind_text(stmt, 2, (const char*)favorite_tbl_ptr->dbtime.c_str(), strlen((const char*)favorite_tbl_ptr->dbtime.c_str()), SQLITE_TRANSIENT );

	// stmtのSQLを実行
	loop = 0;
	rc = e_PAL_RESULT_DELETE_ERR;
	while (1) {
		if (SQLITE_DONE == sqlite3_step(stmt)) {
			rc = e_PAL_RESULT_SUCCESS;
			break;
		}
		if (loop++ > 10) {
			rc = e_PAL_RESULT_DELETE_ERR;
			break;
		}
	}

	// stmt を開放
	sqlite3_finalize(stmt);
	sqlite3_close(db);

	log_dbg("SC_POI_delGCfavoriteTBL   rc...%d   userid...%s  dbtime...%s  \n", rc, favorite_tbl_ptr->userid.c_str(),
			favorite_tbl_ptr->dbtime.c_str());

	return (rc);
}

/**
 * @brief お気に入りテーブルの検索
 * @param[in] 		検索条件データ
 * @return			SQL処理結果
 */
int SC_POI_selectGCfavoriteTBL(SC_POI_SEARCH_COND_TBL* cond_tbl_ptr, std::vector<SM_GC_POI_QSORT_WORK_TBL> *sort_data_list,
		std::vector<SM_GC_POI_FAVORITE_TBL> *favorite_tbl_list) {
	int rc = e_PAL_RESULT_SELECT_ERR;
	sqlite3 *db;

	if (NULL == cond_tbl_ptr || NULL == sort_data_list || NULL == favorite_tbl_list) {
		return (e_PAL_RESULT_PARAM_ERR);
	}

	rc = sqlite3_open(poi_db_full_name, &db);
	if (rc != SQLITE_OK) {
		log_dbg("SC_POI_selectGCfavoriteTBL   sqlite3_open  rc != SQLITE_OK  pat...%s   \n", poi_db_full_name);
		return (rc);
	}
	// stmt を生成する
	std::string sql_param = "select * from SM_GC_POI_FAVORITE_TBL where userid = ?";

	if (cond_tbl_ptr->high_datetime != "" && cond_tbl_ptr->low_datetime != "") {
		if (cond_tbl_ptr->high_datetime == cond_tbl_ptr->low_datetime) {
			sql_param += " and dbtime = ?";
		} else {
			sql_param += " and dbtime <= ?";
			sql_param += " and dbtime >= ?";
		}
	} else {
		if (cond_tbl_ptr->high_datetime != "")
			sql_param += " and dbtime <= ?";
		if (cond_tbl_ptr->low_datetime != "")
			sql_param += " and dbtime >= ?";
	}
	sql_param += " order by dbtime desc";

	sqlite3_stmt *stmt = NULL;
	sqlite3_prepare_v2(db, (const char*)sql_param.c_str(), strlen((const char*)sql_param.c_str()), &stmt, NULL);
	// stmtの内部バッファを一旦クリア
	sqlite3_reset(stmt);

	sqlite3_bind_text(stmt, 1, (const char*)cond_tbl_ptr->userid.c_str(), strlen((const char*)cond_tbl_ptr->userid.c_str()), SQLITE_TRANSIENT );
	int add_cnt = 2;
	if (cond_tbl_ptr->high_datetime != "" && cond_tbl_ptr->low_datetime != "") {
		if (cond_tbl_ptr->high_datetime == cond_tbl_ptr->low_datetime) {
			sqlite3_bind_text(stmt, add_cnt, (const char*)cond_tbl_ptr->high_datetime.c_str(), strlen((const char*)cond_tbl_ptr->high_datetime.c_str()),
					SQLITE_TRANSIENT );
			add_cnt++;
		} else {
			sqlite3_bind_text(stmt, add_cnt, (const char*)cond_tbl_ptr->high_datetime.c_str(), strlen((const char*)cond_tbl_ptr->high_datetime.c_str()),
					SQLITE_TRANSIENT );
			add_cnt++;
			sqlite3_bind_text(stmt, add_cnt, (const char*)cond_tbl_ptr->low_datetime.c_str(), strlen((const char*)cond_tbl_ptr->low_datetime.c_str()),
					SQLITE_TRANSIENT );
			add_cnt++;
		}
	} else {
		if (cond_tbl_ptr->high_datetime != "") {
			sqlite3_bind_text(stmt, add_cnt, (const char*)cond_tbl_ptr->high_datetime.c_str(), strlen((const char*)cond_tbl_ptr->high_datetime.c_str()),
					SQLITE_TRANSIENT );
			add_cnt++;
		}
		if (cond_tbl_ptr->low_datetime != "") {
			sqlite3_bind_text(stmt, add_cnt, (const char*)cond_tbl_ptr->low_datetime.c_str(), strlen((const char*)cond_tbl_ptr->low_datetime.c_str()),
					SQLITE_TRANSIENT );
			add_cnt++;
		}
	}

	int loop = 0;
	int r;
//	std::vector<SM_GC_POI_FAVORITE_TBL> favorite_tbl_list;
	int sort_f = 0;
	if (cond_tbl_ptr->c_lat != 0x4f1a0000 && cond_tbl_ptr->c_log != 0x4f1a0000 && cond_tbl_ptr->len != -1)
		sort_f = 1;

	while (SQLITE_ROW == (r = sqlite3_step(stmt))) {
		SM_GC_POI_FAVORITE_TBL favorite_tbl;
		favorite_tbl.userid = (char*) sqlite3_column_text(stmt, 0);
		favorite_tbl.dbtime = (char*) sqlite3_column_text(stmt, 1);
		favorite_tbl.contentstime = (char*) sqlite3_column_text(stmt, 2);
		favorite_tbl.ctgry_code = (char*) sqlite3_column_text(stmt, 3);
		favorite_tbl.id = (char*) sqlite3_column_text(stmt, 4);
		favorite_tbl.url = (char*) sqlite3_column_text(stmt, 5);
		favorite_tbl.name = (char*) sqlite3_column_text(stmt, 6);
		favorite_tbl.pos_name = (char*) sqlite3_column_text(stmt, 7);
		favorite_tbl.contents = (char*) sqlite3_column_text(stmt, 8);
		favorite_tbl.binary_data_len = sqlite3_column_int(stmt, 10);
		favorite_tbl.binary_data = NULL;
		if (cond_tbl_ptr->data_type == 0 || cond_tbl_ptr->data_type == 1 || cond_tbl_ptr->data_type == 2 || cond_tbl_ptr->data_type == 3) {
			if (favorite_tbl.binary_data_len) {
				UINT8* w_ptr = (UINT8*) sqlite3_column_blob(stmt, 9);
				if (w_ptr != NULL) {
					favorite_tbl.binary_data = new UINT8[favorite_tbl.binary_data_len];
					memcpy(favorite_tbl.binary_data, w_ptr, favorite_tbl.binary_data_len);
				}

			}
		} else {
			favorite_tbl.binary_data = NULL;
		}
		favorite_tbl.lat = sqlite3_column_int(stmt, 11);
		favorite_tbl.log = sqlite3_column_int(stmt, 12);
		favorite_tbl.len = 0;

		if (sort_f) {
			favorite_tbl.len = (int) SC_POI_GetRealLen(	//	時（秒でないよ）
					(double) cond_tbl_ptr->c_lat / (1024. * 3600.), (double) cond_tbl_ptr->c_log / (1024. * 3600.),

					(double) favorite_tbl.lat / (1024. * 3600.), (double) favorite_tbl.log / (1024. * 3600.));

			if (favorite_tbl.len > cond_tbl_ptr->len) {	//	距離の閾値を超えたら処理対象外
				continue;
			}
		}

		log_dbg("++++++++   %d.  userid...%s  dbtime...%s  contentstime...%s  id...%s  url...%s  name...%s  pos_name...%s  contents...%s  binary_data_len...%d  lat...%d  log...%d  len...%d",
				loop,
				favorite_tbl.userid.c_str(),
				favorite_tbl.dbtime.c_str(),
				favorite_tbl.contentstime.c_str(),
				favorite_tbl.id.c_str(),
				favorite_tbl.url.c_str(),
				favorite_tbl.name.c_str(),
				favorite_tbl.pos_name.c_str(),
				favorite_tbl.contents.c_str(),
				favorite_tbl.binary_data_len,
				favorite_tbl.lat,
				favorite_tbl.log,
				favorite_tbl.len);

		favorite_tbl_list->push_back(favorite_tbl);	//	処理対象だけ退避

		loop++;
	}
	// stmt を開放
	sqlite3_finalize(stmt);
	sqlite3_close(db);

	if (loop == 0) {
		return (e_PAL_RESULT_NO_DATA_ERR);	// 該当データなし
	}

#if 0 // データ格納部分は移植
	//	ソートの準備。ソートワークにデータを設定する。
	std::vector<SM_GC_POI_QSORT_WORK_TBL> sort_data_list;
#endif
	for (int i = 0; i < favorite_tbl_list->size(); i++) {
		SM_GC_POI_QSORT_WORK_TBL worktbl;
		SM_GC_POI_FAVORITE_TBL favorite_tbl = favorite_tbl_list->at(i);

		//	sortワークへ
		worktbl.id = i;
		worktbl.len = favorite_tbl.len;

		sort_data_list->push_back(worktbl);
	}
	//	ソートを行う。
	if (sort_f) {
		SC_POI_sort(sort_data_list);
	}

	return (e_PAL_RESULT_SUCCESS);
}

/**
 * @brief お気に入りテーブル数の返却
 * @param[in] 		検索条件データ
 * @return			テーブル数 or SQL処理結果
 */
int  SC_POI_getCountGCfavoriteTBL(
									SC_POI_SEARCH_COND_TBL*  cond_tbl_ptr
								)
{
	int  rc = e_PAL_RESULT_SELECT_ERR;
	sqlite3 *db;

	rc = sqlite3_open(poi_db_full_name, &db);
	if(rc != SQLITE_OK)	{
		log_dbg("SC_POI_getCountGCfavoriteTBL   sqlite3_open  rc != SQLITE_OK  pat...%s   \n", poi_db_full_name);
		return(-99);
	}

	// stmt を生成する
	std::string  sql_param = "select * from SM_GC_POI_FAVORITE_TBL where userid = ?";
	if(cond_tbl_ptr->data_type == 3)	sql_param= "select * from SM_GC_POI_FAVORITE_TBL where userid = ?  and  binary_data_len > 0";	//	お気に入り(地図あり)
	if(cond_tbl_ptr->data_type == 7)	sql_param= "select * from SM_GC_POI_FAVORITE_TBL where userid = ?  and  binary_data_len = 0";	//	お気に入り(地図なし)

	if(cond_tbl_ptr->high_datetime 	!= ""  &&  cond_tbl_ptr->low_datetime 	!= "")	{
		if(cond_tbl_ptr->high_datetime == cond_tbl_ptr->low_datetime)	{
			sql_param += " and dbtime = ?";
		}
		else	{
			sql_param += " and dbtime <= ?";
			sql_param += " and dbtime >= ?";
		}
	}
	else	{
		if(cond_tbl_ptr->high_datetime 	!= "")	sql_param += " and dbtime <= ?";
		if(cond_tbl_ptr->low_datetime 	!= "")	sql_param += " and dbtime >= ?";
	}

	sqlite3_stmt *stmt=NULL;
	sqlite3_prepare_v2(db, (const char*)sql_param.c_str(), strlen((const char*)sql_param.c_str()), &stmt, NULL);
	// stmtの内部バッファを一旦クリア
	sqlite3_reset(stmt);

	sqlite3_bind_text(stmt, 1, (const char*)cond_tbl_ptr->userid.c_str(), 	strlen((const char*)cond_tbl_ptr->userid.c_str()), 	SQLITE_TRANSIENT);
	int  add_cnt = 2;
	if(cond_tbl_ptr->high_datetime 	!= ""  &&  cond_tbl_ptr->low_datetime 	!= "")	{
		if(cond_tbl_ptr->high_datetime == cond_tbl_ptr->low_datetime)	{
			sqlite3_bind_text(stmt, add_cnt, (const char*)cond_tbl_ptr->high_datetime.c_str(), 	strlen((const char*)cond_tbl_ptr->high_datetime.c_str()), SQLITE_TRANSIENT);
			add_cnt++;
		}
		else	{
			sqlite3_bind_text(stmt, add_cnt, (const char*)cond_tbl_ptr->high_datetime.c_str(), 	strlen((const char*)cond_tbl_ptr->high_datetime.c_str()), SQLITE_TRANSIENT);
			add_cnt++;
			sqlite3_bind_text(stmt, add_cnt, (const char*)cond_tbl_ptr->low_datetime.c_str(), 	strlen((const char*)cond_tbl_ptr->low_datetime.c_str()), SQLITE_TRANSIENT);
			add_cnt++;
		}
	}
	else	{
		if(cond_tbl_ptr->high_datetime 	!= "")	{
			sqlite3_bind_text(stmt, add_cnt, (const char*)cond_tbl_ptr->high_datetime.c_str(), 	strlen((const char*)cond_tbl_ptr->high_datetime.c_str()), SQLITE_TRANSIENT);
			add_cnt++;
		}
		if(cond_tbl_ptr->low_datetime 	!= "")	{
			sqlite3_bind_text(stmt, add_cnt, (const char*)cond_tbl_ptr->low_datetime.c_str(), 	strlen((const char*)cond_tbl_ptr->low_datetime.c_str()), SQLITE_TRANSIENT);
			add_cnt++;
		}
	}

	int  loop = 0;
	int r;
	int  sort_f = 0;
	if(cond_tbl_ptr->c_lat != 0x4f1a0000  &&  cond_tbl_ptr->c_log != 0x4f1a0000  &&  cond_tbl_ptr->len != -1)		sort_f = 1;

	while (SQLITE_ROW == (r=sqlite3_step(stmt))){
		if(sort_f)	{
			SM_GC_POI_FAVORITE_TBL	favorite_tbl;
			favorite_tbl.lat 				= sqlite3_column_int(stmt, 11);
			favorite_tbl.log 				= sqlite3_column_int(stmt, 12);
			favorite_tbl.len				= 0;

			favorite_tbl.len = (int)SC_POI_GetRealLen(	//	時（秒でないよ）
														(double)cond_tbl_ptr->c_lat/(1024. * 3600.),
														(double)cond_tbl_ptr->c_log/(1024. * 3600.),

														(double)favorite_tbl.lat/(1024. * 3600.),
														(double)favorite_tbl.log/(1024. * 3600.)
													);

			if(favorite_tbl.len > cond_tbl_ptr->len)	{	//	距離の閾値を超えたら処理対象外
				continue;
			}
		}

		loop++;
	}
	// stmt を開放
	sqlite3_finalize(stmt);
	sqlite3_close(db);

	return  (loop);

}

/**
 * @brief ログ出力(デバッグ)
 * @param[in] fmt 	書式指定に従った文字列
 * @param[in] ... 	可変個引数リスト
 * @return			なし
 */
static	void log_dbg( const char *fmt, ...)
{
#ifdef ANDROID
	va_list valist;

	va_start(valist, fmt);
	__android_log_vprint(ANDROID_LOG_DEBUG, (char*) SC_TAG_PAL, (char*) fmt, valist);
	va_end(valist);
#endif
}
