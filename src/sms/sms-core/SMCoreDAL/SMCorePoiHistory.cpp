/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#include "SMCoreDALInternal.h"


static  char  poi_db_full_name[256];
static	void log_dbg( const char *fmt, ...);

/**
 * @brief ＰＯＩ用フォルダ初期化
 * @param[in] db_path 	ＰＯＩ・ＤＢの格納フォルダ
 * @return				処理結果
 */
E_PAL_RESULT  SC_POI_HISTORY_TBL_Initialize(const char* db_path)
{

	sprintf(poi_db_full_name, "%spoi.db", db_path);

//	log_dbg("SC_POI_HISTORY_TBL_Initialize   LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL   \n");
//	log_dbg("SC_POI_HISTORY_TBL_Initialize   LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL   \n");

//	log_dbg("SC_POI_HISTORY_TBL_Initialize   pat...%s   %s%d\n", poi_db_full_name, __FILE__, __LINE__);

//	log_dbg("SC_POI_HISTORY_TBL_Initialize   LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL   \n");
//	log_dbg("SC_POI_HISTORY_TBL_Initialize   LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL   \n");

    int rc;
	sqlite3 *db;
	sqlite3_stmt *st;
	//bool  rc_bool = false;
	char *zErrMsg = 0;

	rc = sqlite3_open(poi_db_full_name, &db);
	if(rc != SQLITE_OK)	{
		log_dbg("SC_POI_Initialize   rc != SQLITE_OK  pat...%s   d\n", poi_db_full_name);
		return(e_PAL_RESULT_ACCESS_ERR);
	}

	//rc = sqlite3_exec(db, "DROP TABLE IF EXISTS SM_GC_POI_HISTORY_TBL", NULL, NULL, &zErrMsg);



	std::string  param_str;
	std::string result;

	//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	//		履歴テーブル
	param_str = "SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name='SM_GC_POI_HISTORY_TBL';";
	rc = sqlite3_prepare_v2(db, (const char*)param_str.c_str(), -1, &st, NULL);
	while((rc = sqlite3_step(st)) == SQLITE_ROW)
	{
		char  *text 		= (char*)sqlite3_column_text(st, 0);
		result = text;
		log_dbg("JNI  table_cnt...%s", result.c_str());
		break;
	}
	sqlite3_finalize( st );

	if(result=="0")	{
		const char* sql_stm = "create table SM_GC_POI_HISTORY_TBL("
												 " userid text,"		\
												 " datetime text,"		\
												 " gemid text,"			\
												 " gemspotid text,"		\
												 " lat integer,"		\
												 " log integer"			\
												 ");";

		rc = sqlite3_exec(db, sql_stm, NULL, 0, &zErrMsg);
		if(rc != SQLITE_OK)	{
			log_dbg("create table SM_GC_POI_HISTORY_TBL   rc != SQLITE_OK  \n");
			return(e_PAL_RESULT_ACCESS_ERR);
		}

		log_dbg("履歴テーブル登録");
	}
	else	{
		log_dbg("履歴テーブル登録済み");
	}




	rc = sqlite3_close(db);

	return(e_PAL_RESULT_SUCCESS);

}
/**
 * @brief 履歴テーブルの追加
 * @param[in] 		履歴テーブル・データ
 * @return			SQL処理結果
 */
int SC_POI_addGChistoryTBL(SM_GC_POI_HISTORY_TBL* history_tbl_ptr) {
	int rc = e_PAL_RESULT_UPDATE_ERR;
	sqlite3 *db;

	//strcpy(poi_db_full_name, "/mnt/sdcard/Android/data/jp.co.hitachi.smsfv.aa/files/Map/JPN/poi.db");

	rc = sqlite3_open(poi_db_full_name, &db);
	if (rc != SQLITE_OK) {
		log_dbg("SC_POI_addGChistoryTBL   sqlite3_open  rc != SQLITE_OK  pat...%s   d\n", poi_db_full_name);
		return (e_PAL_RESULT_ACCESS_ERR);
	}
	sqlite3_stmt *stmt = NULL;

	//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

	// stmt を生成する
	const char *sql = "insert into SM_GC_POI_HISTORY_TBL (userid, datetime, gemid, gemspotid, lat, log) values(?, ?, ?, ?, ?,?)";
	sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	// stmtの内部バッファを一旦クリア
	sqlite3_reset(stmt);

	sqlite3_bind_text(stmt, 1, (const char*)history_tbl_ptr->userid.c_str(), 	strlen((const char*)history_tbl_ptr->userid.c_str()), 		SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, (const char*)history_tbl_ptr->datetime.c_str(), 	strlen((const char*)history_tbl_ptr->datetime.c_str()), 		SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 3, (const char*)history_tbl_ptr->gemid.c_str(), 		strlen((const char*)history_tbl_ptr->gemid.c_str()), 		SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 4, (const char*)history_tbl_ptr->gemspotid.c_str(), 	strlen((const char*)history_tbl_ptr->gemspotid.c_str()), 	SQLITE_TRANSIENT);
	sqlite3_bind_int(stmt, 	5,  history_tbl_ptr->lat);
	sqlite3_bind_int(stmt, 	6,  history_tbl_ptr->log);

	// stmtのSQLを実行
	rc = e_PAL_RESULT_SUCCESS;
	int loop=0;
	while (SQLITE_DONE != sqlite3_step(stmt)){
	    if (loop++>10){
	    	rc = e_PAL_RESULT_UPDATE_ERR;
	    	break;
	    }
	}
	if(rc == e_PAL_RESULT_SUCCESS)	{
		//log_dbg("SC_POI_addGChistoryTBL   OK   userid...%s  datetime...%s   \n", history_tbl_ptr->userid.c_str(), history_tbl_ptr->datetime.c_str());
	}

	// stmt を開放
	sqlite3_finalize(stmt);


	//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

	// 	stmt を生成する
	//	年月日の新しいものから並べ替える。
	const char *select_sql = "select * from SM_GC_POI_HISTORY_TBL where userid = ? order by datetime desc";
	sqlite3_prepare_v2(db, select_sql, strlen(select_sql), &stmt, NULL);
	// stmtの内部バッファを一旦クリア
	sqlite3_reset(stmt);

	sqlite3_bind_text(stmt, 1, (const char*)history_tbl_ptr->userid.c_str(), strlen((const char*)history_tbl_ptr->userid.c_str()), SQLITE_TRANSIENT);

	rc = e_PAL_RESULT_SELECT_ERR;
	int sel_cnt = 0;
	int r;
	SM_GC_POI_HISTORY_TBL	history_tbl;
	std::vector<SM_GC_POI_HISTORY_TBL>  history_tbl_list;
	while (SQLITE_ROW == (r=sqlite3_step(stmt))){
		history_tbl.userid		= (char*)sqlite3_column_text(stmt, 0);
		history_tbl.datetime	= (char*)sqlite3_column_text(stmt, 1);
		history_tbl.gemid		= (char*)sqlite3_column_text(stmt, 2);
		history_tbl.gemspotid	= (char*)sqlite3_column_text(stmt, 3);
		history_tbl.lat 		= sqlite3_column_int(stmt, 4);
		history_tbl.log 		= sqlite3_column_int(stmt, 5);

		sel_cnt++;

		if(sel_cnt >  SM_GC_POI_HISTORY_TBL_MAX_COUNT)	{
			history_tbl_list.push_back(history_tbl);
		}

		rc = e_PAL_RESULT_SUCCESS;
	}

	log_dbg("SC_POI_addGChistoryTBL   削除件数...%d 件   \n", history_tbl_list.size());
	// stmt を開放
	sqlite3_finalize(stmt);

	//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


	for(int  i=0;i<(int)history_tbl_list.size();i++)	{
		SM_GC_POI_HISTORY_TBL	history_tbl_del = history_tbl_list.at(i);

		// stmt を生成する
		const char *delete_sql = "delete  from SM_GC_POI_HISTORY_TBL where userid = ?  and datetime = ?";
		sqlite3_prepare_v2(db, delete_sql, strlen(delete_sql), &stmt, NULL);
		// stmtの内部バッファを一旦クリア
		sqlite3_reset(stmt);
		sqlite3_bind_text(stmt, 1, (const char*)history_tbl_del.userid.c_str(), 	strlen((const char*)history_tbl_del.userid.c_str()), 	SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 2, (const char*)history_tbl_del.datetime.c_str(), 	strlen((const char*)history_tbl_del.datetime.c_str()), 	SQLITE_TRANSIENT);

		// stmtのSQLを実行
		loop=0;
		rc = e_PAL_RESULT_DELETE_ERR;
		while (1){
			if(SQLITE_DONE == sqlite3_step(stmt))	{
				rc = e_PAL_RESULT_SUCCESS;
				break;
			}
		    if (loop++>10){
		    	rc = e_PAL_RESULT_DELETE_ERR;
		    	break;
		    }
		}
		if(rc == e_PAL_RESULT_SUCCESS)	{
			log_dbg("SC_POI_addGChistoryTBL  del  OK   userid...%s  datetime...%s  \n", history_tbl_del.userid.c_str(), history_tbl_del.datetime.c_str());
		}
		else	{
	    	log_dbg("SC_POI_addGChistoryTBL  del  NG   userid...%s  datetime...%s  \n", history_tbl_del.userid.c_str(), history_tbl_del.datetime.c_str());
	    }

		// stmt を開放
		sqlite3_finalize(stmt);
	}


	sqlite3_close(db);

	return  (rc);
}
/**
 * @brief 履歴テーブルの修正
 * @param[in] 		履歴テーブル・データ
 * @return			SQL処理結果
 */
int  SC_POI_chgGChistoryTBL(SM_GC_POI_HISTORY_TBL*  history_tbl_ptr)
{
	int  rc = e_PAL_RESULT_UPDATE_ERR;
	sqlite3 *db;

	//strcpy(poi_db_full_name, "/mnt/sdcard/Android/data/jp.co.hitachi.smsfv.aa/files/Map/JPN/poi.db");

	rc = sqlite3_open(poi_db_full_name, &db);
	if(rc != SQLITE_OK)	{
		log_dbg("SC_POI_addGChistoryTBL   sqlite3_open  rc != SQLITE_OK  pat...%s   d\n", poi_db_full_name);
		return(e_PAL_RESULT_ACCESS_ERR);
	}
	sqlite3_stmt *stmt=NULL;

	const char *select_sql = "select * from SM_GC_POI_HISTORY_TBL where userid = ? and datetime = ?";
	sqlite3_prepare_v2(db, select_sql, strlen(select_sql), &stmt, NULL);
	// stmtの内部バッファを一旦クリア
	sqlite3_reset(stmt);

	sqlite3_bind_text(stmt, 1, (const char*)history_tbl_ptr->userid.c_str(), 	strlen((const char*)history_tbl_ptr->userid.c_str()), 	SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, (const char*)history_tbl_ptr->datetime.c_str(), 	strlen((const char*)history_tbl_ptr->datetime.c_str()), 	SQLITE_TRANSIENT);

	// stmtのSQLを実行
	int loop=0;
	rc = e_PAL_RESULT_UPDATE_ERR;
	int r;
	while (SQLITE_ROW == (r=sqlite3_step(stmt))){
		loop++;
	}
	if(loop > 0)	{
		rc = e_PAL_RESULT_SUCCESS;
		//log_dbg("SC_POI_chgGCpointTBL   select  OK   userid...%s  datetime...%s  loop...%d  \n", history_tbl_ptr->userid.c_str(), history_tbl_ptr->datetime.c_str(),loop);
	}
	else	{
    	rc = e_PAL_RESULT_UPDATE_ERR;
    	log_dbg("SC_POI_chgGCpointTBL   select  NG   userid...%s  datetime...%s  loop...%d  \n", history_tbl_ptr->userid.c_str(), history_tbl_ptr->datetime.c_str(),loop);
    }
	// stmt を開放
	sqlite3_finalize(stmt);

	if(rc == e_PAL_RESULT_UPDATE_ERR)	{
		sqlite3_close(db);
		return (rc);
	}

	// stmt を生成する
	const char *sql = "update SM_GC_POI_HISTORY_TBL set gemid = ?,gemspotid = ?,lat = ?,log = ?  where userid = ? and datetime = ?";
	sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	// stmtの内部バッファを一旦クリア
	sqlite3_reset(stmt);

	sqlite3_bind_text(stmt, 1, (const char*)history_tbl_ptr->gemid.c_str(), 		strlen((const char*)history_tbl_ptr->gemid.c_str()), 	SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, (const char*)history_tbl_ptr->gemspotid.c_str(), 	strlen((const char*)history_tbl_ptr->gemspotid.c_str()), SQLITE_TRANSIENT);
	sqlite3_bind_int(stmt, 	3,  history_tbl_ptr->lat);
	sqlite3_bind_int(stmt, 	4,  history_tbl_ptr->log);

	sqlite3_bind_text(stmt, 5, (const char*)history_tbl_ptr->userid.c_str(), 	strlen((const char*)history_tbl_ptr->userid.c_str()), 	SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 6, (const char*)history_tbl_ptr->datetime.c_str(), 	strlen((const char*)history_tbl_ptr->datetime.c_str()), 	SQLITE_TRANSIENT);


	// stmtのSQLを実行
	loop=0;
	rc = e_PAL_RESULT_UPDATE_ERR;
	while (1){
		if(SQLITE_DONE == sqlite3_step(stmt))	{
			rc = e_PAL_RESULT_SUCCESS;
			break;
		}
	    if (loop++>10){
	    	rc = e_PAL_RESULT_UPDATE_ERR;
	    	break;
	    }
	}


	// stmt を開放
	sqlite3_finalize(stmt);
	sqlite3_close(db);

//	log_dbg("SC_POI_chgGCpointTBL   sqlite3_step  rc...%d   userid...%s  datetime...%s  \n",
//			rc, history_tbl_ptr->userid.c_str(),history_tbl_ptr->datetime.c_str());

	return (rc);
}
/**
 * @brief 履歴テーブルの削除
 * @param[in] 		履歴テーブル・データ
 * @return			SQL処理結果
 */
int  SC_POI_delGChistoryTBL(SM_GC_POI_HISTORY_TBL*  history_tbl_ptr)
{
	int  rc = e_PAL_RESULT_DELETE_ERR;
	sqlite3 *db;

	//strcpy(poi_db_full_name, "/mnt/sdcard/Android/data/jp.co.hitachi.smsfv.aa/files/Map/JPN/poi.db");

	rc = sqlite3_open(poi_db_full_name, &db);
	if(rc != SQLITE_OK)	{
		log_dbg("SC_POI_addGChistoryTBL   sqlite3_open  rc != SQLITE_OK  pat...%s   d\n", poi_db_full_name);
		return(e_PAL_RESULT_ACCESS_ERR);
	}

	sqlite3_stmt *stmt=NULL;
	const char *select_sql = "select * from SM_GC_POI_HISTORY_TBL where userid = ? and datetime = ?";
	sqlite3_prepare_v2(db, select_sql, strlen(select_sql), &stmt, NULL);
	// stmtの内部バッファを一旦クリア
	sqlite3_reset(stmt);

	sqlite3_bind_text(stmt, 1, (const char*)history_tbl_ptr->userid.c_str(), 	strlen((const char*)history_tbl_ptr->userid.c_str()), 	SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, (const char*)history_tbl_ptr->datetime.c_str(), 	strlen((const char*)history_tbl_ptr->datetime.c_str()), 	SQLITE_TRANSIENT);

	// stmtのSQLを実行
	int loop=0;
	rc = e_PAL_RESULT_DELETE_ERR;
	int r;
	while (SQLITE_ROW == (r=sqlite3_step(stmt))){
		loop++;
	}
	if(loop > 0)	{
		rc = e_PAL_RESULT_SUCCESS;
		//log_dbg("SC_POI_delGChistoryTBL   select  OK   userid...%s  datetime...%s  loop...%d  \n", history_tbl_ptr->userid.c_str(), history_tbl_ptr->datetime.c_str(),loop);
	}
	else	{
    	rc = e_PAL_RESULT_DELETE_ERR;
    	log_dbg("SC_POI_delGChistoryTBL   select  NG   userid...%s  datetime...%s  loop...%d  \n", history_tbl_ptr->userid.c_str(), history_tbl_ptr->datetime.c_str(),loop);
    }
	// stmt を開放
	sqlite3_finalize(stmt);

	if(rc == e_PAL_RESULT_DELETE_ERR)	{
		sqlite3_close(db);
		return (rc);
	}

	// stmt を生成する
	const char *sql = "delete  from SM_GC_POI_HISTORY_TBL where userid = ?  and datetime = ?";
	sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	// stmtの内部バッファを一旦クリア
	sqlite3_reset(stmt);
	sqlite3_bind_text(stmt, 1, (const char*)history_tbl_ptr->userid.c_str(), 	strlen((const char*)history_tbl_ptr->userid.c_str()), 	SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, (const char*)history_tbl_ptr->datetime.c_str(), 	strlen((const char*)history_tbl_ptr->datetime.c_str()), 	SQLITE_TRANSIENT);

	// stmtのSQLを実行
	loop=0;
	rc = e_PAL_RESULT_DELETE_ERR;
	while (1){
		if(SQLITE_DONE == sqlite3_step(stmt))	{
			rc = e_PAL_RESULT_SUCCESS;
			break;
		}
	    if (loop++>10){
	    	rc = e_PAL_RESULT_DELETE_ERR;
	    	break;
	    }
	}

	// stmt を開放
	sqlite3_finalize(stmt);
	sqlite3_close(db);

	//log_dbg("SC_POI_delGChistoryTBL   rc...%d   userid...%s  datetime...%s  \n",rc, history_tbl_ptr->userid.c_str(),history_tbl_ptr->datetime.c_str());


	return (rc);
}

/**
 * @brief 履歴テーブルの検索
 * @param[in] 		検索条件データ
 * @return			SQL処理結果
 */
int SC_POI_selectGChistoryTBL(SC_POI_SEARCH_COND_2_TBL* cond_tbl_ptr, std::vector<SM_GC_POI_HISTORY_TBL> *history_tbl_list) {
	int rc = e_PAL_RESULT_SELECT_ERR;
	sqlite3 *db;

	if (NULL == cond_tbl_ptr || NULL == history_tbl_list) {
		return (e_PAL_RESULT_PARAM_ERR);
	}

	rc = sqlite3_open(poi_db_full_name, &db);
	if (rc != SQLITE_OK) {
		log_dbg("SC_POI_selectGChistoryTBL   sqlite3_open  rc != SQLITE_OK  pat...%s   d\n", poi_db_full_name);
		return (rc);
	}
	// 	stmt を生成する
	//	年月日の新しいものから並べ替える。
	const char *sql = "select * from SM_GC_POI_HISTORY_TBL where userid = ? order by datetime desc";
	sqlite3_stmt *stmt = NULL;
	sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	// stmtの内部バッファを一旦クリア
	sqlite3_reset(stmt);

	sqlite3_bind_text(stmt, 1, (const char*)cond_tbl_ptr->userid.c_str(), strlen((const char*)cond_tbl_ptr->userid.c_str()), SQLITE_TRANSIENT );

	int r;
//	std::vector<SM_GC_POI_HISTORY_TBL>  history_tbl_list;
	while (SQLITE_ROW == (r = sqlite3_step(stmt))) {
		SM_GC_POI_HISTORY_TBL history_tbl;
		history_tbl.userid = (char*) sqlite3_column_text(stmt, 0);
		history_tbl.datetime = (char*) sqlite3_column_text(stmt, 1);
		history_tbl.gemid = (char*) sqlite3_column_text(stmt, 2);
		history_tbl.gemspotid = (char*) sqlite3_column_text(stmt, 3);
		history_tbl.lat = sqlite3_column_int(stmt, 4);
		history_tbl.log = sqlite3_column_int(stmt, 5);

		history_tbl_list->push_back(history_tbl);
	}

	if (history_tbl_list->size() == 0) {
		// stmt を開放
		sqlite3_finalize(stmt);
		sqlite3_close(db);
		return (e_PAL_RESULT_NO_DATA_ERR);	// 該当データなし
	}

	// stmt を開放
	sqlite3_finalize(stmt);
	sqlite3_close(db);

	return (e_PAL_RESULT_SUCCESS);
}

/**
 * @brief 履歴テーブ数の返却
 * @param[in] 		検索条件データ
 * @return			テーブル数 or SQL処理結果
 */
int  SC_POI_getCountGChistoryTBL(
								SC_POI_SEARCH_COND_2_TBL*  cond_tbl_ptr
							)
{
	int  rc = e_PAL_RESULT_SELECT_ERR;
	sqlite3 *db;

	rc = sqlite3_open(poi_db_full_name, &db);
	if(rc != SQLITE_OK)	{
		log_dbg("SC_POI_getCountGChistoryTBL   sqlite3_open  rc != SQLITE_OK  pat...%s   d\n", poi_db_full_name);
		return(-99);
	}
	// 	stmt を生成する
	//	年月日の新しいものから並べ替える。
	const char *sql = "select * from SM_GC_POI_HISTORY_TBL where userid = ? order by datetime desc";
	sqlite3_stmt *stmt=NULL;
	sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	// stmtの内部バッファを一旦クリア
	sqlite3_reset(stmt);

	sqlite3_bind_text(stmt, 1, (const char*)cond_tbl_ptr->userid.c_str(), strlen((const char*)cond_tbl_ptr->userid.c_str()), SQLITE_TRANSIENT);

	int  loop = 0;
	int r;
	std::vector<SM_GC_POI_HISTORY_TBL>  history_tbl_list;
	while (SQLITE_ROW == (r=sqlite3_step(stmt))){
		loop++;
	}

	// stmt を開放
	sqlite3_finalize(stmt);
	sqlite3_close(db);

	return (loop);
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
#endif /* ANDROID */
}
