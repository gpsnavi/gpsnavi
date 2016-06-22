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
E_PAL_RESULT  SC_POI_POINT_TBL_Initialize(const char* db_path)
{

	sprintf(poi_db_full_name, "%spoi.db", db_path);

	//log_dbg("SC_POI_POINT_TBL_Initialize   LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL   \n");
	//log_dbg("SC_POI_POINT_TBL_Initialize   LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL   \n");

	//log_dbg("SC_POI_POINT_TBL_Initialize   pat...%s   %s%\n", poi_db_full_name, __FILE__, __LINE__);

	//log_dbg("SC_POI_POINT_TBL_Initialize   LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL   \n");
	//log_dbg("SC_POI_POINT_TBL_Initialize   LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL   \n");

    int rc;
	sqlite3 *db;
	sqlite3_stmt *st;
	//bool  rc_bool = false;
	char *zErrMsg = 0;

	rc = sqlite3_open(poi_db_full_name, &db);
	if(rc != SQLITE_OK)	{
		log_dbg("SC_POI_Initialize   rc != SQLITE_OK  pat...%s   \n", poi_db_full_name);
		return(e_PAL_RESULT_ACCESS_ERR);
	}

	//rc = sqlite3_exec(db, "DROP TABLE IF EXISTS SM_GC_POI_POINT_TBL", NULL, NULL, &zErrMsg);



	std::string  param_str;
	std::string result;

	//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	//		登録地テーブル
	param_str = "SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name='SM_GC_POI_POINT_TBL';";
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
		const char* sql_stm = "create table SM_GC_POI_POINT_TBL("
												 " data_type integer,"	\
												 " userid text,"		\
												 " datetime text,"		\
												 " gemid text,"			\
												 " gemspotid text,"		\
												 " pos_name text,"		\
												 " lat integer,"		\
												 " log integer"			\
												 ");";

		rc = sqlite3_exec(db, sql_stm, NULL, 0, &zErrMsg);
		if(rc != SQLITE_OK)	{
			log_dbg("create table SM_GC_POI_POINT_TBL   rc != SQLITE_OK  \n");
			return(e_PAL_RESULT_ACCESS_ERR);
		}

		log_dbg("登録地テーブル登録");
	}
	else	{
		log_dbg("登録地テーブル登録済み");
	}


	rc = sqlite3_close(db);

	return(e_PAL_RESULT_SUCCESS);

}
/**
 * @brief 登録地テーブルの追加
 * @param[in] 		登録地テーブル・データ
 * @return			SQL処理結果
 */
int  SC_POI_addGCpointTBL(SM_GC_POI_POINT_TBL*  point_tbl_ptr)
{
	int  rc = e_PAL_RESULT_UPDATE_ERR;
	sqlite3 *db;


	rc = sqlite3_open(poi_db_full_name, &db);
	if(rc != SQLITE_OK)	{
		log_dbg("SC_POI_addGCpointTBL   sqlite3_open  rc != SQLITE_OK  pat...%s   \n", poi_db_full_name);
		return(e_PAL_RESULT_ACCESS_ERR);
	}

	sqlite3_stmt *stmt=NULL;

	const char *select_sql_0 = "select * from SM_GC_POI_POINT_TBL where userid = ? and datetime = ?";
	sqlite3_prepare_v2(db, select_sql_0, strlen(select_sql_0), &stmt, NULL);
	// stmtの内部バッファを一旦クリア
	sqlite3_reset(stmt);

	sqlite3_bind_text(stmt, 1, (const char*)point_tbl_ptr->userid.c_str(), 	strlen((const char*)point_tbl_ptr->userid.c_str()), 		SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, (const char*)point_tbl_ptr->datetime.c_str(), strlen((const char*)point_tbl_ptr->datetime.c_str()), 	SQLITE_TRANSIENT);


	// stmtのSQLを実行
	SM_GC_POI_POINT_TBL	point_tbl;
	int loop=0;
	rc = e_PAL_RESULT_ACCESS_ERR;
	int r;
	while (SQLITE_ROW == (r=sqlite3_step(stmt))){
		point_tbl.userid		= (char*)sqlite3_column_text(stmt, 1);
		point_tbl.datetime		= (char*)sqlite3_column_text(stmt, 2);
		loop++;
	}
	if(loop > 0)	{
		rc = e_PAL_RESULT_SUCCESS;
		log_dbg("SC_POI_addGCpointTBL   同一キーあり   userid...%s  datetime...%s  loop...%d  \n", point_tbl_ptr->userid.c_str(), point_tbl_ptr->datetime.c_str(),loop);
	}
	else	{
    	rc = e_PAL_RESULT_ACCESS_ERR;
    	//log_dbg("SC_POI_addGCpointTBL   select  NG   userid...%s  datetime...%s  loop...%d  \n", point_tbl_ptr->userid.c_str(), point_tbl_ptr->datetime.c_str(),loop);
    }
	// stmt を開放
	sqlite3_finalize(stmt);

	if(rc == e_PAL_RESULT_SUCCESS)	{	//	同じキーのレコードが存在する時
		sqlite3_close(db);
		return (e_PAL_RESULT_DUPLICATE_ERR);
	}


	const char *select_sql_1 = "select * from SM_GC_POI_POINT_TBL where userid = ? and pos_name = ? and lat = ? and log = ?";
	sqlite3_prepare_v2(db, select_sql_1, strlen(select_sql_1), &stmt, NULL);
	// stmtの内部バッファを一旦クリア
	sqlite3_reset(stmt);

	sqlite3_bind_text(stmt, 1, (const char*)point_tbl_ptr->userid.c_str(), 	strlen((const char*)point_tbl_ptr->userid.c_str()), 		SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, (const char*)point_tbl_ptr->pos_name.c_str(), strlen((const char*)point_tbl_ptr->pos_name.c_str()), 	SQLITE_TRANSIENT);
	sqlite3_bind_int(stmt, 3, point_tbl_ptr->lat);
	sqlite3_bind_int(stmt, 4, point_tbl_ptr->log);

	// stmtのSQLを実行
	loop=0;
	while (SQLITE_ROW == (r=sqlite3_step(stmt))){
		loop++;
	}
	// stmt を開放
	sqlite3_finalize(stmt);

	if(loop > 0)	{	//	同じデータのレコードが存在する時
		sqlite3_close(db);
		log_dbg("SC_POI_addGCpointTBL   同一データあり   userid...%s   pos_name...%s  lat...%d  log...%d \n", point_tbl_ptr->userid.c_str(), point_tbl_ptr->pos_name.c_str(), point_tbl_ptr->lat, point_tbl_ptr->log);
		return (e_PAL_RESULT_DUPLICATE_DATA_ERR);
	}
	//log_dbg("SC_POI_addGCpointTBL  userid...%s   pos_name...%s  lat...%d  log...%d \n", point_tbl_ptr->userid.c_str(), point_tbl_ptr->pos_name.c_str(), point_tbl_ptr->lat, point_tbl_ptr->log);


	const char *select_sql_2 = "select * from SM_GC_POI_POINT_TBL where userid = ? ";
	sqlite3_prepare_v2(db, select_sql_2, strlen(select_sql_2), &stmt, NULL);
	// stmtの内部バッファを一旦クリア
	sqlite3_reset(stmt);

	sqlite3_bind_text(stmt, 1, (const char*)point_tbl_ptr->userid.c_str(), 	strlen((const char*)point_tbl_ptr->userid.c_str()), 		SQLITE_TRANSIENT);

	// stmtのSQLを実行
	int slct_cnt=0;
	rc = e_PAL_RESULT_ACCESS_ERR;
	while (SQLITE_ROW == (r=sqlite3_step(stmt))){
		slct_cnt++;
	}

	// stmt を開放
	sqlite3_finalize(stmt);

	if(slct_cnt >=  SM_GC_POI_POINT_TBL_MAX_COUNT)	{
		log_dbg("SC_POI_addGCpointTBL  NG  MAX_Over   userid...%s    \n", point_tbl_ptr->userid.c_str());
		sqlite3_close(db);
		return (e_PAL_RESULT_ADD_OVER_ERR);
	}

	// stmt を生成する
	const char *sql = "insert into SM_GC_POI_POINT_TBL (data_type, userid, datetime, gemid, gemspotid, pos_name, lat, log) values(?, ?, ?, ?, ?, ?, ?, ?)";

	sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	// stmtの内部バッファを一旦クリア
	sqlite3_reset(stmt);
	sqlite3_bind_int(stmt, 	1,  point_tbl_ptr->data_type);
	sqlite3_bind_text(stmt, 2, 	(const char*)point_tbl_ptr->userid.c_str(), 	strlen((const char*)point_tbl_ptr->userid.c_str()), 	SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 3, 	(const char*)point_tbl_ptr->datetime.c_str(), 	strlen((const char*)point_tbl_ptr->datetime.c_str()), 	SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 4, 	(const char*)point_tbl_ptr->gemid.c_str(), 		strlen((const char*)point_tbl_ptr->gemid.c_str()), 		SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 5, 	(const char*)point_tbl_ptr->gemspotid.c_str(), 	strlen((const char*)point_tbl_ptr->gemspotid.c_str()), 	SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 6, 	(const char*)point_tbl_ptr->pos_name.c_str(), 	strlen((const char*)point_tbl_ptr->pos_name.c_str()), 	SQLITE_TRANSIENT);
	sqlite3_bind_int(stmt, 	7,  point_tbl_ptr->lat);
	sqlite3_bind_int(stmt, 	8,  point_tbl_ptr->log);

	// stmtのSQLを実行
	rc = e_PAL_RESULT_SUCCESS;
	loop=0;
	while (SQLITE_DONE != sqlite3_step(stmt)){
	    if (loop++>10){
	    	rc = e_PAL_RESULT_UPDATE_ERR;
	    	break;
	    }
	}

	if(rc == e_PAL_RESULT_SUCCESS)	{
		//log_dbg("SC_POI_addGCpointTBL   OK   userid...%s  datetime...%s   \n", point_tbl_ptr->userid.c_str(), point_tbl_ptr->datetime.c_str());
	}
	// stmt を開放
	sqlite3_finalize(stmt);
	sqlite3_close(db);

	return (rc);
}
/**
 * @brief 登録地テーブルの修正
 * @param[in] 		登録地テーブル・データ
 * @return			SQL処理結果
 */
int  SC_POI_chgGCpointTBL(SM_GC_POI_POINT_TBL*  point_tbl_ptr)
{
	int  rc = e_PAL_RESULT_UPDATE_ERR;
	sqlite3 *db;

	rc = sqlite3_open(poi_db_full_name, &db);
	if(rc != SQLITE_OK)	{
		log_dbg("SC_POI_chgGCpointTBL   sqlite3_open  rc != SQLITE_OK  pat...%s   \n", poi_db_full_name);
		return(e_PAL_RESULT_ACCESS_ERR);
	}

	sqlite3_stmt *stmt=NULL;


	const char *select_sql = "select * from SM_GC_POI_POINT_TBL where userid = ? and datetime = ?";
	sqlite3_prepare_v2(db, select_sql, strlen(select_sql), &stmt, NULL);
	// stmtの内部バッファを一旦クリア
	sqlite3_reset(stmt);

	sqlite3_bind_text(stmt, 1, (const char*)point_tbl_ptr->userid.c_str(), 	strlen((const char*)point_tbl_ptr->userid.c_str()), 	SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, (const char*)point_tbl_ptr->datetime.c_str(), strlen((const char*)point_tbl_ptr->datetime.c_str()), 	SQLITE_TRANSIENT);


	// stmtのSQLを実行
	SM_GC_POI_POINT_TBL	point_tbl;
	int loop=0;
	rc = e_PAL_RESULT_UPDATE_ERR;
	int r;
	while (SQLITE_ROW == (r=sqlite3_step(stmt))){
		point_tbl.userid		= (char*)sqlite3_column_text(stmt, 1);
		point_tbl.datetime		= (char*)sqlite3_column_text(stmt, 2);
		loop++;
	}
	if(loop > 0)	{
		rc = e_PAL_RESULT_SUCCESS;
		//log_dbg("SC_POI_chgGCpointTBL   select  OK   userid...%s  datetime...%s  loop...%d  \n", point_tbl_ptr->userid.c_str(), point_tbl_ptr->datetime.c_str(),loop);
	}
	else	{
    	rc = e_PAL_RESULT_UPDATE_ERR;
    	log_dbg("SC_POI_chgGCpointTBL   select  NG   userid...%s  datetime...%s  loop...%d  \n", point_tbl_ptr->userid.c_str(), point_tbl_ptr->datetime.c_str(),loop);
    }
	// stmt を開放
	sqlite3_finalize(stmt);

	if(rc == e_PAL_RESULT_UPDATE_ERR)	{
		sqlite3_close(db);
		return (rc);
	}


	// stmt を生成する
	const char *sql = "update SM_GC_POI_POINT_TBL set data_type = ?, gemid = ?,gemspotid = ?,pos_name= ?, lat = ?,log = ?  where userid = ? and datetime = ?";
	sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	// stmtの内部バッファを一旦クリア
	sqlite3_reset(stmt);

	sqlite3_bind_int(stmt, 	1,  point_tbl_ptr->data_type);
	sqlite3_bind_text(stmt, 2, 	(const char*)point_tbl_ptr->gemid.c_str(), 		strlen((const char*)point_tbl_ptr->gemid.c_str()), 		SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 3, 	(const char*)point_tbl_ptr->gemspotid.c_str(), 	strlen((const char*)point_tbl_ptr->gemspotid.c_str()), 	SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 4, 	(const char*)point_tbl_ptr->pos_name.c_str(), 	strlen((const char*)point_tbl_ptr->pos_name.c_str()), 	SQLITE_TRANSIENT);
	sqlite3_bind_int(stmt, 	5,  point_tbl_ptr->lat);
	sqlite3_bind_int(stmt, 	6,  point_tbl_ptr->log);

	sqlite3_bind_text(stmt, 7, (const char*)point_tbl.userid.c_str(), 		strlen((const char*)point_tbl.userid.c_str()), 		SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 8, (const char*)point_tbl.datetime.c_str(), 	strlen((const char*)point_tbl.datetime.c_str()), 	SQLITE_TRANSIENT);


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
//			rc, point_tbl_ptr->userid.c_str(),point_tbl_ptr->datetime.c_str());

	return (rc);
}
/**
 * @brief 登録地テーブルの削除
 * @param[in] 		登録地テーブル・データ
 * @return			SQL処理結果
 */
int  SC_POI_delGCpointTBL(SM_GC_POI_POINT_TBL*  point_tbl_ptr)
{
	int  rc = e_PAL_RESULT_DELETE_ERR;
	sqlite3 *db;

	rc = sqlite3_open(poi_db_full_name, &db);
	if(rc != SQLITE_OK)	{
		log_dbg("SC_POI_delGCpointTBL   sqlite3_open  rc != SQLITE_OK  pat...%s   \n", poi_db_full_name);
		return(e_PAL_RESULT_ACCESS_ERR);
	}

	sqlite3_stmt *stmt=NULL;

	const char *select_sql = "select * from SM_GC_POI_POINT_TBL where userid = ? and datetime = ?";
	sqlite3_prepare_v2(db, select_sql, strlen(select_sql), &stmt, NULL);
	// stmtの内部バッファを一旦クリア
	sqlite3_reset(stmt);

	sqlite3_bind_text(stmt, 1, (const char*)point_tbl_ptr->userid.c_str(), 	strlen((const char*)point_tbl_ptr->userid.c_str()), 		SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, (const char*)point_tbl_ptr->datetime.c_str(), strlen((const char*)point_tbl_ptr->datetime.c_str()), 	SQLITE_TRANSIENT);

	// stmtのSQLを実行
	SM_GC_POI_POINT_TBL	point_tbl;
	int loop=0;
	rc = e_PAL_RESULT_DELETE_ERR;
	int r;
	while (SQLITE_ROW == (r=sqlite3_step(stmt))){
		point_tbl.userid		= (char*)sqlite3_column_text(stmt, 1);
		point_tbl.datetime		= (char*)sqlite3_column_text(stmt, 2);
		loop++;
	}
	if(loop > 0)	{
		rc = e_PAL_RESULT_SUCCESS;
		//log_dbg("SC_POI_delGCpointTBL   select  OK   userid...%s  datetime...%s  loop...%d  \n", point_tbl_ptr->userid.c_str(), point_tbl_ptr->datetime.c_str(),loop);
	}
	else	{
    	rc = e_PAL_RESULT_DELETE_ERR;
    	log_dbg("SC_POI_delGCpointTBL   select  NG   userid...%s  datetime...%s  loop...%d  \n", point_tbl_ptr->userid.c_str(), point_tbl_ptr->datetime.c_str(),loop);
    }
	// stmt を開放
	sqlite3_finalize(stmt);

	if(rc == e_PAL_RESULT_DELETE_ERR)	{
		sqlite3_close(db);
		return (rc);
	}

	// stmt を生成する
	const char *sql = "delete from SM_GC_POI_POINT_TBL where userid = ?  and datetime = ?";
	sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	// stmtの内部バッファを一旦クリア
	sqlite3_reset(stmt);
	sqlite3_bind_text(stmt, 1, (const char*)point_tbl.userid.c_str(), 	strlen((const char*)point_tbl.userid.c_str()), 		SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, (const char*)point_tbl.datetime.c_str(), 	strlen((const char*)point_tbl.datetime.c_str()), 	SQLITE_TRANSIENT);

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

	//log_dbg("SC_POI_delGCpointTBL   rc...%d   userid...%s  datetime...%s  \n",rc, point_tbl_ptr->userid.c_str(),point_tbl_ptr->datetime.c_str());

	return (rc);
}
/**
 * @brief 登録地テーブルの検索
 * @param[in] 		検索条件データ
 * @return			SQL処理結果
 */
int SC_POI_selectGCpointTBL(SC_POI_SEARCH_COND_TBL* cond_tbl_ptr, std::vector<SM_GC_POI_QSORT_WORK_TBL> *sort_data_list,
		std::vector<SM_GC_POI_POINT_TBL> *point_tbl_list) {
	int rc = e_PAL_RESULT_SELECT_ERR;
	sqlite3 *db;

	if (NULL == cond_tbl_ptr || NULL == sort_data_list || NULL == point_tbl_list) {
		return (e_PAL_RESULT_PARAM_ERR);
	}

	rc = sqlite3_open(poi_db_full_name, &db);
	if (rc != SQLITE_OK) {
		log_dbg("SC_POI_selectGCpointTBL   sqlite3_open  rc != SQLITE_OK  pat...%s   \n", poi_db_full_name);
		return (rc);
	}

	// stmt を生成する
	std::string sql_param = "";

	sql_param = "select * from SM_GC_POI_POINT_TBL where userid = ?";

	if (cond_tbl_ptr->data_type != -1)
		sql_param += " and data_type = ?";

	if (cond_tbl_ptr->high_datetime != "" && cond_tbl_ptr->low_datetime != "") {
		if (cond_tbl_ptr->high_datetime == cond_tbl_ptr->low_datetime) {
			sql_param += " and datetime = ?";
		} else {
			sql_param += " and datetime <= ?";
			sql_param += " and datetime >= ?";
		}
	} else {
		if (cond_tbl_ptr->high_datetime != "")
			sql_param += " and datetime <= ?";
		if (cond_tbl_ptr->low_datetime != "")
			sql_param += " and datetime >= ?";
	}
	sql_param += " order by datetime desc";

	//log_dbg("SC_POI_selectGCpointTBL   sql_param...%s   \n", sql_param.c_str());

	sqlite3_stmt *stmt = NULL;
	sqlite3_prepare_v2(db, (const char*)sql_param.c_str(), strlen((const char*)sql_param.c_str()), &stmt, NULL);
	// stmtの内部バッファを一旦クリア
	sqlite3_reset(stmt);

	int add_cnt = 1;

	sqlite3_bind_text(stmt, add_cnt, (const char*)cond_tbl_ptr->userid.c_str(), strlen((const char*)cond_tbl_ptr->userid.c_str()), SQLITE_TRANSIENT );
	add_cnt++;

	if (cond_tbl_ptr->data_type != -1) {
		sqlite3_bind_int(stmt, add_cnt, cond_tbl_ptr->data_type);
		add_cnt++;
	}

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
//	std::vector<SM_GC_POI_POINT_TBL> point_tbl_list;
	int sort_f = 0;

	if(cond_tbl_ptr->c_lat != 0x4f1a0000  &&  cond_tbl_ptr->c_log != 0x4f1a0000  &&  cond_tbl_ptr->len != -1)	sort_f = 1;

	while (SQLITE_ROW == (r=sqlite3_step(stmt))){
		SM_GC_POI_POINT_TBL	point_tbl;
		point_tbl.data_type 	= sqlite3_column_int(stmt, 0);
		point_tbl.userid		= (char*)sqlite3_column_text(stmt, 1);
		point_tbl.datetime		= (char*)sqlite3_column_text(stmt, 2);
		point_tbl.gemid			= (char*)sqlite3_column_text(stmt, 3);
		point_tbl.gemspotid		= (char*)sqlite3_column_text(stmt, 4);
		point_tbl.pos_name		= (char*)sqlite3_column_text(stmt, 5);
		point_tbl.lat 			= sqlite3_column_int(stmt, 6);
		point_tbl.log 			= sqlite3_column_int(stmt, 7);
		point_tbl.len			= 0;

		if(sort_f)	{
			point_tbl.len = (int)SC_POI_GetRealLen(	//	時（秒でないよ）
													(double)cond_tbl_ptr->c_lat/(1024. * 3600.),
													(double)cond_tbl_ptr->c_log/(1024. * 3600.),

													(double)point_tbl.lat/(1024. * 3600.),
													(double)point_tbl.log/(1024. * 3600.)
												);

			if(point_tbl.len > cond_tbl_ptr->len)	{	//	距離の閾値を超えたら処理対象外
				continue;
			}
		}

//		log_dbg("++++++++   %d.  data_type...%d  userid...%s  datetime...%s  gemid...%s  gemspotid...%s  pos_name...%s  lat...%d  log...%d  len...%d",
//				loop,
//				point_tbl.data_type,
//				point_tbl.userid.c_str(),
//				point_tbl.datetime.c_str(),
//				point_tbl.gemid.c_str(),
//				point_tbl.gemspotid.c_str(),
//				point_tbl.pos_name.c_str(),
//				point_tbl.lat,
//				point_tbl.log,
//				point_tbl.len);

		point_tbl_list->push_back(point_tbl);	//	処理対象だけ退避

		loop++;
	}

	if (loop == 0) {
		// stmt を開放
		sqlite3_finalize(stmt);
		sqlite3_close(db);
		return (e_PAL_RESULT_NO_DATA_ERR);	// 該当データなし
	}

	//	ソートの準備。ソートワークにデータを設定する。
//	std::vector<SM_GC_POI_QSORT_WORK_TBL>	sort_data_list;

	for (int i = 0; i < point_tbl_list->size(); i++) {
		SM_GC_POI_QSORT_WORK_TBL worktbl;
		SM_GC_POI_POINT_TBL point_tbl = point_tbl_list->at(i);

		//	sortワークへ
		worktbl.id = i;
		worktbl.len = point_tbl.len;

		sort_data_list->push_back(worktbl);
	}
	//	ソートを行う。
	if (sort_f) {
		SC_POI_sort(sort_data_list);
	}

	// stmt を開放
	sqlite3_finalize(stmt);
	sqlite3_close(db);

	return (e_PAL_RESULT_SUCCESS);
}

/**
 * @brief 登録地テーブル数の返却
 * @param[in] 		検索条件データ
 * @return			テーブル数 or SQL処理結果
 */
int  SC_POI_getCountGCpointTBL(
								SC_POI_SEARCH_COND_TBL*  cond_tbl_ptr
							)
{
	int  rc = e_PAL_RESULT_SELECT_ERR;
	sqlite3 *db;

	rc = sqlite3_open(poi_db_full_name, &db);
	if(rc != SQLITE_OK)	{
		log_dbg("SC_POI_getCountGCpointTBLL   sqlite3_open  rc != SQLITE_OK  pat...%s   \n", poi_db_full_name);
		return(-99);
	}
	// stmt を生成する
	int  where_f = 0;
	int  cond_f  = 0;
	std::string  sql_param = "select * from SM_GC_POI_POINT_TBL";

	if(where_f == 0)	{
		sql_param += " where";
		where_f = 1;
	}
	if(cond_f == 1)	{
		sql_param += " and";
	}
	sql_param += " userid = ?";
	cond_f = 1;


	if(cond_tbl_ptr->data_type 		!= -1)	{
		if(where_f == 0)	{
			sql_param += " where";
			where_f = 1;
		}
		if(cond_f == 1)	{
			sql_param += " and";
		}
		sql_param += " data_type = ?";
		cond_f = 1;
	}

	if(cond_tbl_ptr->high_datetime 	!= ""  &&  cond_tbl_ptr->low_datetime 	!= "")	{
		if(where_f == 0)	{
			sql_param += " where";
			where_f = 1;
		}
		if(cond_f == 1)	{
			sql_param += " and";
		}
		if(cond_tbl_ptr->high_datetime == cond_tbl_ptr->low_datetime)	{
			sql_param += " datetime = ?";
		}
		else	{
			sql_param += " datetime <= ?";
			if(cond_f == 1)	{
				sql_param += " and";
			}
			sql_param += " datetime >= ?";
		}
		cond_f = 1;
	}
	else	{

		if(cond_tbl_ptr->high_datetime 	!= "")	{
			if(where_f == 0)	{
				sql_param += " where";
				where_f = 1;
			}
			if(cond_f == 1)	{
				sql_param += " and";
			}
			sql_param += " datetime <= ?";
			cond_f = 1;
		}
		if(cond_tbl_ptr->low_datetime 	!= "")	{
			if(where_f == 0)	{
				sql_param += " where";
				where_f = 1;
			}
			if(cond_f == 1)	{
				sql_param += " and";
			}
			sql_param += " datetime >= ?";
			cond_f = 1;
		}
	}


	//log_dbg("SC_POI_getCountGCpointTBLL   sql_param...%s   \n", sql_param.c_str());

	sqlite3_stmt *stmt=NULL;
	sqlite3_prepare_v2(db, (const char*)sql_param.c_str(), strlen((const char*)sql_param.c_str()), &stmt, NULL);
	// stmtの内部バッファを一旦クリア
	sqlite3_reset(stmt);

	int  add_cnt = 1;
	sqlite3_bind_text(stmt, add_cnt, (const char*)cond_tbl_ptr->userid.c_str(), 	strlen((const char*)cond_tbl_ptr->userid.c_str()), 	SQLITE_TRANSIENT);
	add_cnt++;

	if(cond_tbl_ptr->data_type 		!= -1)	{
		sqlite3_bind_int(stmt, 	add_cnt,  cond_tbl_ptr->data_type);		add_cnt++;
	}

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

	if(cond_tbl_ptr->c_lat != 0x4f1a0000  &&  cond_tbl_ptr->c_log != 0x4f1a0000  &&  cond_tbl_ptr->len != -1)	sort_f = 1;

	while (SQLITE_ROW == (r=sqlite3_step(stmt))){
		if(sort_f)	{
			SM_GC_POI_POINT_TBL	point_tbl;
			point_tbl.lat 			= sqlite3_column_int(stmt, 6);
			point_tbl.log 			= sqlite3_column_int(stmt, 7);
			point_tbl.len			= 0;
			point_tbl.len = (int)SC_POI_GetRealLen(	//	時（秒でないよ）
													(double)cond_tbl_ptr->c_lat/(1024. * 3600.),
													(double)cond_tbl_ptr->c_log/(1024. * 3600.),

													(double)point_tbl.lat/(1024. * 3600.),
													(double)point_tbl.log/(1024. * 3600.)
												);

			if(point_tbl.len > cond_tbl_ptr->len)	{	//	距離の閾値を超えたら処理対象外
				continue;
			}
		}
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
