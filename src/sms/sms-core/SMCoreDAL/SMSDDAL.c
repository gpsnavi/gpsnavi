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


#define	SC_SDDAL_SQL_TABLE_EXIST_CHECK				"SELECT count(*) FROM sqlite_master WHERE TYPE='table' AND NAME=?"
#define	SC_SDDAL_SQL_CREATETABLE_PHYD_SENSOR_DATA	"CREATE TABLE PHYD_SENSOR_DATA(TIME TEXT(17) NOT NULL,ID INT NOT NULL,TIMESTAMP TEXT(17) NOT NULL,LATNETWORK DOUBLE,LONNETWORK DOUBLE,LATGPS DOUBLE,LONGPS DOUBLE,ACCX DOUBLE,ACCY DOUBLE,ACCZ DOUBLE,ORIENTX DOUBLE,ORIENTY DOUBLE,ORIENTZ DOUBLE,MAGNETX DOUBLE,MAGNETY DOUBLE,MAGNETZ DOUBLE,GYROX DOUBLE,GYROY DOUBLE,GYROZ DOUBLE,GRAVITYX DOUBLE,GRAVITYY DOUBLE,GRAVITYZ DOUBLE,PRIMARY KEY(TIME,ID))"
#define	SC_SDDAL_SQL_SELECT_COUNT_PHYD_SENSOR_DATA	"SELECT count(*) FROM PHYD_SENSOR_DATA"
#define	SC_SDDAL_SQL_INSERT_PHYD_SENSOR_DATA		"INSERT INTO PHYD_SENSOR_DATA(TIME,ID,TIMESTAMP,LATNETWORK,LONNETWORK,LATGPS,LONGPS,ACCX,ACCY,ACCZ,ORIENTX,ORIENTY,ORIENTZ,MAGNETX,MAGNETY,MAGNETZ,GYROX,GYROY,GYROZ,GRAVITYX,GRAVITYY,GRAVITYZ) VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"
#define	SC_SDDAL_SQL_DELETE_PHYD_SENSOR_DATA		"DELETE FROM PHYD_SENSOR_DATA WHERE TIME=? AND ID<=?"
#define	SC_SDDAL_SQL_DELETE_ALL_PHYD_SENSOR_DATA	"DELETE FROM PHYD_SENSOR_DATA"
#define	SC_SDDAL_SQL_SELECT_PHYD_SENSOR_DATA		"SELECT ID,TIMESTAMP,LATNETWORK,LONNETWORK,LATGPS,LONGPS,ACCX,ACCY,ACCZ,ORIENTX,ORIENTY,ORIENTZ,MAGNETX,MAGNETY,MAGNETZ,GYROX,GYROY,GYROZ,GRAVITYX,GRAVITYY,GRAVITYZ FROM PHYD_SENSOR_DATA WHERE TIME=? AND ID>=? ORDER BY ID ASC"
#define	SC_SDDAL_SQL_SELECT_MAX_ID					"SELECT MAX(ID) FROM PHYD_SENSOR_DATA WHERE TIME=?"

#define	SC_SDDAL_TIME_SIZE							17

static DBOBJECT	*db;
static pthread_mutex_t	*mutex;
static pthread_mutex_t	mutexObj;

static void SC_SDDAL_SetDoubleValue(sqlite3_stmt *stmt, INT32 idx, DOUBLE value);
static DOUBLE SC_SDDAL_GetDoubleValue(sqlite3_stmt *stmt, INT32 idx);

/**
 * @brief DB初期化
 * @param	[in]dbFilePath  DBファイルパス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SDDAL_Initialize(const Char *dbFilePath)
{
	E_SC_RESULT	ret	= e_SC_RESULT_SUCCESS;
	INT32		sqliteRet = SQLITE_OK;

	do {
		// パラメータチェック
		if (NULL == dbFilePath) {
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 初期化
		db = NULL;
		mutex = &mutexObj;
		memset(mutex, 0, sizeof(SC_MUTEX));

		// Mutex生成
		pthread_mutex_init(mutex, NULL);

		// DBオープン
		sqliteRet = sqlite3_open(dbFilePath, &db);
		if (SQLITE_OK != sqliteRet) {
			ret = e_SC_RESULT_RDB_ACCESSERR;
			break;
		}
	} while (0);

	return (ret);
}

/**
 * @brief DB終了化
 * @return 処理結果(E_SC_RESULT)
 */
void SC_SDDAL_Finalize()
{
	if (NULL != db) {
		// DBクローズ
		sqlite3_close(db);
	}

	if (NULL != mutex) {
		// Mutex破棄
		pthread_mutex_destroy(mutex);
		mutex = NULL;
	}
}

/**
 * @brief トランザクション開始
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SDDAL_Transaction()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	sqliteRet = SQLITE_OK;

	do {
		// パラメータチェック
		if (NULL == db) {
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// トランザクション開始
		sqliteRet = sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL);
		if (SQLITE_OK != sqliteRet) {
			ret = e_SC_RESULT_RDB_ACCESSERR;
			break;
		}
	} while (0);

	return (ret);
}

/**
 * @brief コミット
 * @param[in]  db               SQLITE
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SDDAL_Commit()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	sqliteRet = SQLITE_OK;

	do {
		// パラメータチェック
		if (NULL == db) {
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// コミット
		sqliteRet = sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
		if (SQLITE_OK != sqliteRet) {
			ret = e_SC_RESULT_RDB_ACCESSERR;
			break;
		}
	} while (0);

	return (ret);
}

/**
 * @brief ロールバック
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SDDAL_Rollback()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	sqliteRet = SQLITE_OK;

	do {
		// パラメータチェック
		if (NULL == db) {
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// ロールバック
		sqliteRet = sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
		if (SQLITE_OK != sqliteRet) {
			ret = e_SC_RESULT_RDB_ACCESSERR;
			break;
		}
	} while (0);

	return (ret);
}

/**
 * @brief テーブル有無チェック
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SDDAL_CheckTabeleExist(const Char *tableName, Bool *exist)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32		sqliteRet = SQLITE_OK;
	sqlite3_stmt	*stmt = NULL;
	INT32		num = 0;

	do {
		// パラメータチェック
		if (NULL == db) {
			ret = e_SC_RESULT_FAIL;
			break;
		}
		if ((NULL == tableName) || (EOS == *tableName)) {
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == exist) {
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// SQLITE prepare
		sqliteRet = sqlite3_prepare(db, SC_SDDAL_SQL_TABLE_EXIST_CHECK, strlen(SC_SDDAL_SQL_TABLE_EXIST_CHECK), &stmt, NULL);
		if (SQLITE_OK != sqliteRet) {
			ret = e_SC_RESULT_RDB_ACCESSERR;
			break;
		}

		// stmtの内部バッファを一旦クリア
		sqlite3_reset(stmt);

		// sqlの?の部分に、値を設定
		sqlite3_bind_text(stmt, 1, tableName, strlen(tableName), SQLITE_TRANSIENT);

		// SQLITE step
		if (SQLITE_ROW == (sqliteRet = sqlite3_step(stmt))) {
			num = sqlite3_column_int(stmt, 0);
			if (0 == num) {
				*exist = false;
			} else {
				*exist = true;
			}
		} else {
			*exist = false;
		}
	} while (0);

	if (NULL != stmt) {
		// SQLITE finalize
		sqliteRet = sqlite3_finalize(stmt);
	}

	return (ret);
}

/**
 * @brief PHYD_SENSOR_DATAテーブル作成
 * @param[in]  transaction  トランザクションを開始するか否か（true:する、false:しない）
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SDDAL_CreateTabelePhydSensorData(Bool transaction)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	sqliteRet = SQLITE_OK;
	sqlite3_stmt	*stmt = NULL;

	do {
		// パラメータチェック
		if (NULL == db) {
			ret = e_SC_RESULT_FAIL;
			break;
		}

		if (transaction) {
			// トランザクション開始
			ret = SC_SDDAL_Transaction();
			if (e_SC_RESULT_SUCCESS != ret) {
				break;
			}
		}

		// テーブル作成
		// SQLITE prepare
		sqliteRet = sqlite3_prepare(db, SC_SDDAL_SQL_CREATETABLE_PHYD_SENSOR_DATA, strlen(SC_SDDAL_SQL_CREATETABLE_PHYD_SENSOR_DATA), &stmt, NULL);
		if (SQLITE_OK == sqliteRet) {
			// stmtの内部バッファを一旦クリア
			sqlite3_reset(stmt);

			// stmtのSQLを実行
			sqliteRet = sqlite3_step(stmt);
			if (SQLITE_DONE != sqliteRet) {
				ret = e_SC_RESULT_RDB_ACCESSERR;
			}
		} else {
			ret = e_SC_RESULT_RDB_ACCESSERR;
		}

		if (transaction) {
			if (e_SC_RESULT_SUCCESS == ret) {
				// コミット
				ret = SC_SDDAL_Commit(db);
				if (e_SC_RESULT_SUCCESS != ret) {
					// ロールバック
					SC_SDDAL_Rollback(db);
				}
			} else {
				// ロールバック
				SC_SDDAL_Rollback(db);
			}
		}
	} while (0);

	if (NULL != stmt) {
		// SQLITE finalize
		sqliteRet = sqlite3_finalize(stmt);
	}

	return (ret);
}

/**
 * @brief PHYD_SENSOR_DATAテーブルのデータ件数取得
 * @param[out] dataNum      データ件数
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SDDAL_GetPhydSensorDataCount(INT32 *dataNum)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	sqliteRet = SQLITE_OK;
	sqlite3_stmt	*stmt = NULL;

	do {
		// パラメータチェック
		if (NULL == db) {
			ret = e_SC_RESULT_FAIL;
			break;
		}
		if (NULL == dataNum) {
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// SQLITE prepare
		sqliteRet = sqlite3_prepare(db, SC_SDDAL_SQL_SELECT_COUNT_PHYD_SENSOR_DATA, strlen(SC_SDDAL_SQL_SELECT_COUNT_PHYD_SENSOR_DATA), &stmt, NULL);
		if (SQLITE_OK != sqliteRet) {
			ret = e_SC_RESULT_RDB_ACCESSERR;
			break;
		}

		// stmtの内部バッファを一旦クリア
		sqlite3_reset(stmt);

		// SQLITE step
		if (SQLITE_ROW == (sqliteRet = sqlite3_step(stmt))) {
			*dataNum = sqlite3_column_int(stmt, 0);
		} else {
			if ((SQLITE_NOTFOUND == sqliteRet) ||
				(SQLITE_DONE == sqliteRet)) {
				*dataNum = 0;
			} else {
				ret = e_SC_RESULT_RDB_ACCESSERR;
				break;
			}
		}
	} while (0);

	if (NULL != stmt) {
		// SQLITE finalize
		sqliteRet = sqlite3_finalize(stmt);
	}

	return (ret);
}

/**
 * @brief PHYD_SENSOR_DATAテーブルのデータ追加
 * @param[in]  time         運転特性診断用データ取得開始日時
 * @param[in]  data         更新するデータ
 * @param[in]  dataNum      更新するデータ数
 * @param[in]  transaction  トランザクションを開始するか否か（true:する、false:しない）
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SDDAL_AddPhydSensorData(const Char *time,
									   const SMPHYDDATA *data,
									   INT32 dataNum,
									   Bool transaction)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	sqliteRet = SQLITE_OK;
	sqlite3_stmt	*stmt = NULL;
	INT32	id = 0;
	INT32	num = 0;
	Bool	isLocked = false;

	do {
		// パラメータチェック
		if (NULL == db) {
			ret = e_SC_RESULT_FAIL;
			break;
		}
		if (NULL == mutex) {
			ret = e_SC_RESULT_FAIL;
			break;
		}
		if ((NULL == time) || (EOS == *time)) {
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == data) {
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// Mutexロック
		if (0 != pthread_mutex_lock(mutex)) {
			ret = e_SC_RESULT_FAIL;
			break;
		}
		isLocked = true;

		// 今のIDの最大値検索
		// SQLITE prepare
		sqliteRet = sqlite3_prepare(db, SC_SDDAL_SQL_SELECT_MAX_ID, strlen(SC_SDDAL_SQL_SELECT_MAX_ID), &stmt, NULL);
		if (SQLITE_OK != sqliteRet) {
			ret = e_SC_RESULT_RDB_ACCESSERR;
			break;
		}

		// stmtの内部バッファを一旦クリア
		sqlite3_reset(stmt);

		// sqlの?の部分に、値を設定
		sqlite3_bind_text(stmt, 1, time, strlen(time), SQLITE_TRANSIENT);

		// SQLITE step
		if (SQLITE_ROW == (sqliteRet = sqlite3_step(stmt))) {
			id = (sqlite3_column_int(stmt, 0) + 1);
		} else {
			if ((SQLITE_NOTFOUND == sqliteRet) ||
				(SQLITE_DONE == sqliteRet)) {
				id = 1;
			} else {
				ret = e_SC_RESULT_RDB_ACCESSERR;
				break;
			}
		}
		// SQLITE finalize
		sqliteRet = sqlite3_finalize(stmt);
		if (SQLITE_OK != sqliteRet) {
			ret = e_SC_RESULT_RDB_ACCESSERR;
			stmt = NULL;
			break;
		}
		stmt = NULL;

		if (transaction) {
			// トランザクション開始
			ret = SC_SDDAL_Transaction();
			if (e_SC_RESULT_SUCCESS != ret) {
				break;
			}
		}

		for (num = 0; num < dataNum; num++) {
			// SQLITE prepare
			sqliteRet = sqlite3_prepare(db, SC_SDDAL_SQL_INSERT_PHYD_SENSOR_DATA, strlen(SC_SDDAL_SQL_INSERT_PHYD_SENSOR_DATA), &stmt, NULL);
			if (SQLITE_OK != sqliteRet) {
				ret = e_SC_RESULT_RDB_ACCESSERR;
				break;
			}

			// stmtの内部バッファを一旦クリア
			sqlite3_reset(stmt);

			// sqlの?の部分に、値を設定
			sqlite3_bind_text(stmt, 1, time, strlen(time), SQLITE_TRANSIENT);
			sqlite3_bind_int(stmt, 2, id);
			id++;
			sqlite3_bind_text(stmt, 3, data[num].time, strlen(data[num].time), SQLITE_TRANSIENT);
			SC_SDDAL_SetDoubleValue(stmt, 4, data[num].latNetwork);
			SC_SDDAL_SetDoubleValue(stmt, 5, data[num].lonNetwork);
			SC_SDDAL_SetDoubleValue(stmt, 6, data[num].latGPS);
			SC_SDDAL_SetDoubleValue(stmt, 7, data[num].lonGPS);
			SC_SDDAL_SetDoubleValue(stmt, 8, data[num].acc[0]);
			SC_SDDAL_SetDoubleValue(stmt, 9, data[num].acc[1]);
			SC_SDDAL_SetDoubleValue(stmt, 10, data[num].acc[2]);
			SC_SDDAL_SetDoubleValue(stmt, 11, data[num].orientation[2]);
			SC_SDDAL_SetDoubleValue(stmt, 12, data[num].orientation[1]);
			SC_SDDAL_SetDoubleValue(stmt, 13, data[num].orientation[0]);
			SC_SDDAL_SetDoubleValue(stmt, 14, data[num].magneticField[0]);
			SC_SDDAL_SetDoubleValue(stmt, 15, data[num].magneticField[1]);
			SC_SDDAL_SetDoubleValue(stmt, 16, data[num].magneticField[2]);
			SC_SDDAL_SetDoubleValue(stmt, 17, data[num].gyroscope[0]);
			SC_SDDAL_SetDoubleValue(stmt, 18, data[num].gyroscope[1]);
			SC_SDDAL_SetDoubleValue(stmt, 19, data[num].gyroscope[2]);
			SC_SDDAL_SetDoubleValue(stmt, 20, data[num].gravity[0]);
			SC_SDDAL_SetDoubleValue(stmt, 21, data[num].gravity[1]);
			SC_SDDAL_SetDoubleValue(stmt, 22, data[num].gravity[2]);

			// stmtのSQLを実行
			sqliteRet = sqlite3_step(stmt);
			if ((SQLITE_DONE != sqliteRet) || (1 != sqlite3_changes(db))) {
				ret = e_SC_RESULT_RDB_ACCESSERR;
				break;
			}

			// SQLITE finalize
			sqliteRet = sqlite3_finalize(stmt);
			if (SQLITE_OK != sqliteRet) {
				ret = e_SC_RESULT_RDB_ACCESSERR;
				stmt = NULL;
				break;
			}
			stmt = NULL;
		}

		if (transaction) {
			if (e_SC_RESULT_SUCCESS == ret) {
				// コミット
				ret = SC_SDDAL_Commit(db);
				if (e_SC_RESULT_SUCCESS != ret) {
					// ロールバック
					SC_SDDAL_Rollback(db);
					break;
				}
			} else {
				// ロールバック
				SC_SDDAL_Rollback(db);
				break;
			}
		}
	} while (0);

	if (NULL != stmt) {
		// SQLITE finalize
		sqliteRet = sqlite3_finalize(stmt);
	}

	if (isLocked) {
		pthread_mutex_unlock(mutex);
	}

	return (ret);
}

/**
 * @brief PHYD_SENSOR_DATAテーブルのデータ削除
 * @param[in]  time         キー：運転特性診断用データ取得開始日時(YYYYMMDDhhmmssSSS形式)※NULL可
 * @param[in]  id           キー：ID（指定されたID以下のデータを削除対象とする）
 * @param[in]  transaction  トランザクションを開始するか否か（true:する、false:しない）
 * @return 処理結果(E_SC_RESULT)
 * @memo timeがNULLまたは""(空文字)の場合、全てのデータを削除対象にする
 */
E_SC_RESULT SC_SDDAL_DelPhydSensorData(const Char *time,
									   INT32 id,
									   Bool transaction)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	sqliteRet = SQLITE_OK;
	sqlite3_stmt	*stmt = NULL;
	Bool	isLocked = false;

	do {
		// パラメータチェック
		if (NULL == db) {
			ret = e_SC_RESULT_FAIL;
			break;
		}
		if (NULL == mutex) {
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// Mutexロック
		if (0 != pthread_mutex_lock(mutex)) {
			ret = e_SC_RESULT_FAIL;
			break;
		}
		isLocked = true;

		if (transaction) {
			// トランザクション開始
			ret = SC_SDDAL_Transaction();
			if (e_SC_RESULT_SUCCESS != ret) {
				break;
			}
		}

		// SQLITE prepare
		if ((NULL != time) && (EOS != *time)) {
			sqliteRet = sqlite3_prepare(db, SC_SDDAL_SQL_DELETE_PHYD_SENSOR_DATA, strlen(SC_SDDAL_SQL_DELETE_PHYD_SENSOR_DATA), &stmt, NULL);
		} else {
			sqliteRet = sqlite3_prepare(db, SC_SDDAL_SQL_DELETE_ALL_PHYD_SENSOR_DATA, strlen(SC_SDDAL_SQL_DELETE_ALL_PHYD_SENSOR_DATA), &stmt, NULL);
		}
		if (SQLITE_OK == sqliteRet) {
			// stmtの内部バッファを一旦クリア
			sqlite3_reset(stmt);

			// sqlの?の部分に、値を設定
			if ((NULL != time) && (EOS != *time)) {
				sqlite3_bind_text(stmt, 1, time, strlen(time), SQLITE_TRANSIENT);
				sqlite3_bind_int(stmt,  2, id);
			}

			// stmtのSQLを実行
			sqliteRet = sqlite3_step(stmt);
			if ((SQLITE_DONE != sqliteRet)) {
				ret = e_SC_RESULT_RDB_ACCESSERR;
			}
		} else {
			ret = e_SC_RESULT_RDB_ACCESSERR;
		}

		if (transaction) {
			if (e_SC_RESULT_SUCCESS == ret) {
				// コミット
				ret = SC_SDDAL_Commit(db);
				if (e_SC_RESULT_SUCCESS != ret) {
					// ロールバック
					SC_SDDAL_Rollback(db);
					break;
				}
			} else {
				// ロールバック
				SC_SDDAL_Rollback(db);
				break;
			}
		}
	} while (0);

	if (NULL != stmt) {
		// SQLITE finalize
		sqliteRet = sqlite3_finalize(stmt);
	}

	if (isLocked) {
		pthread_mutex_unlock(mutex);
	}

	return (ret);
}

/**
 * @brief PHYD_SENSOR_DATAテーブルのデータ取得
 * @param[in]  maxDataNum       取得するデータ最大数
 * @param[in]  time             運転特性診断用データ取得開始日時(YYYYMMDDhhmmssSSS形式)
 * @param[in]  id               開始ID(1 ～)
 * @param[out] data             取得したデータ
 * @param[out] dataNum          取得したデータ数
 * @param[out] lastFlg          最終位置フラグ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SDDAL_GetPhydSensorDataList(INT32 maxDataNum,
										   const Char *time,
										   INT32 id,
										   SMPHYDDATA *data,
										   INT32 *dataNum,
										   Bool *lastFlg)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	sqliteRet = SQLITE_OK;
	sqlite3_stmt	*stmt = NULL;
	INT32	size = 0;
	INT32	num = 0;
	Bool	isLocked = false;

	do {
		// パラメータチェック
		if (NULL == db) {
		ret = e_SC_RESULT_FAIL;
			break;
		}
		if (NULL == mutex) {
			ret = e_SC_RESULT_FAIL;
			break;
		}
		if (0 >= maxDataNum) {
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((NULL == time) || (EOS == *time)) {
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (0 >= id) {
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == data) {
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == dataNum) {
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == lastFlg) {
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 初期化
		*dataNum = 0;
		memset(data, 0, (sizeof(SMPHYDDATA) * maxDataNum));
		*lastFlg = false;

		// Mutexロック
		if (0 != pthread_mutex_lock(mutex)) {
			ret = e_SC_RESULT_FAIL;
			break;
		}
		isLocked = true;

		// SQLITE prepare
		sqliteRet = sqlite3_prepare(db, SC_SDDAL_SQL_SELECT_PHYD_SENSOR_DATA, strlen(SC_SDDAL_SQL_SELECT_PHYD_SENSOR_DATA), &stmt, NULL);
		if (SQLITE_OK != sqliteRet) {
			ret = e_SC_RESULT_RDB_ACCESSERR;
			break;
		}

		// stmtの内部バッファを一旦クリア
		sqlite3_reset(stmt);

		// sqlの?の部分に、値を設定
		sqlite3_bind_text(stmt, 1, time, strlen(time), SQLITE_TRANSIENT);
		sqlite3_bind_int(stmt,  2, id);

		// SQLITE step
		while (SQLITE_ROW == (sqliteRet = sqlite3_step(stmt))) {
			if (num == maxDataNum) {
				// データ取得を終了する
				sqliteRet = SQLITE_DONE;
				*lastFlg = true;
				break;
			}
			// ID
			data[num].id = sqlite3_column_int(stmt, 0);

			// TIMESTAMP
			size = sqlite3_column_bytes(stmt, 1);
			if (SC_SDDAL_TIME_SIZE != size) {
				continue;
			}
			memcpy(data[num].time, (const char*)sqlite3_column_text(stmt, 1), size);
			data[num].time[size] = EOS;

			// LATNETWORK
			data[num].latNetwork = SC_SDDAL_GetDoubleValue(stmt, 2);
			// LONNETWORK
			data[num].lonNetwork = SC_SDDAL_GetDoubleValue(stmt, 3);
			// LATGPS
			data[num].latGPS = SC_SDDAL_GetDoubleValue(stmt, 4);
			// LONGPS
			data[num].lonGPS = SC_SDDAL_GetDoubleValue(stmt, 5);
			// ACCX
			data[num].acc[0] = SC_SDDAL_GetDoubleValue(stmt, 6);
			// ACCY
			data[num].acc[1] = SC_SDDAL_GetDoubleValue(stmt, 7);
			// ACCZ
			data[num].acc[2] = SC_SDDAL_GetDoubleValue(stmt, 8);
			// ORIENTX
			data[num].orientation[2] = SC_SDDAL_GetDoubleValue(stmt, 9);
			// ORIENTY
			data[num].orientation[1] = SC_SDDAL_GetDoubleValue(stmt, 10);
			// ORIENTZ
			data[num].orientation[0] = SC_SDDAL_GetDoubleValue(stmt, 11);
			// MAGNETX
			data[num].magneticField[0] = SC_SDDAL_GetDoubleValue(stmt, 12);
			// MAGNETY
			data[num].magneticField[1] = SC_SDDAL_GetDoubleValue(stmt, 13);
			// MAGNETZ
			data[num].magneticField[2] = SC_SDDAL_GetDoubleValue(stmt, 14);
			// GYROX
			data[num].gyroscope[0] = SC_SDDAL_GetDoubleValue(stmt, 15);
			// GYROY
			data[num].gyroscope[1] = SC_SDDAL_GetDoubleValue(stmt, 16);
			// GYROZ
			data[num].gyroscope[2] = SC_SDDAL_GetDoubleValue(stmt, 17);
			// GRAVITYX
			data[num].gravity[0] = SC_SDDAL_GetDoubleValue(stmt, 18);
			// GRAVITYY
			data[num].gravity[1] = SC_SDDAL_GetDoubleValue(stmt, 19);
			// GRAVITYZ
			data[num].gravity[2] = SC_SDDAL_GetDoubleValue(stmt, 20);

			num++;
		}
		if (SQLITE_DONE != sqliteRet) {
			ret = e_SC_RESULT_RDB_ACCESSERR;
			break;
		}

		// 件数を設定
		*dataNum = num;
	} while (0);

	if (NULL != stmt) {
		// SQLITE finalize
		sqliteRet = sqlite3_finalize(stmt);
	}

	if (isLocked) {
		pthread_mutex_unlock(mutex);
	}

	return (ret);
}

/**
 * @brief PHYD_SENSOR_DATAテーブルのデータ取得(全件)
 * @param[in]  stmt             sqlite3_stmt
 * @param[in]  idx              インデックス(1 ～)
 * @param[in]  value            double型の値
 */
void SC_SDDAL_SetDoubleValue(sqlite3_stmt *stmt, INT32 idx, DOUBLE value)
{
	if (DBL_MAX != value) {
		sqlite3_bind_double(stmt, idx, value);
	} else {
		sqlite3_bind_null(stmt, idx);
	}
}

/**
 * @brief PHYD_SENSOR_DATAテーブルのデータ取得(全件)
 * @param[in]  stmt             sqlite3_stmt
 * @param[in]  idx              インデックス(1 ～)
 * @return double型の値
 */
DOUBLE SC_SDDAL_GetDoubleValue(sqlite3_stmt *stmt, INT32 idx)
{
	DOUBLE	value = 0;

	if (SQLITE_NULL != sqlite3_column_type(stmt, idx)) {
		value = sqlite3_column_double(stmt, idx);
	} else {
		value = DBL_MAX;
	}

	return (value);
}
