/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#include "SMCComDALInternal.h"

// カラムデータサイズ
#define	SCC_DAL_CONMADATA_SIZE						(6 * 1024 * 1024)
// SQL文のバッファサイズ
#define	SCC_DAL_SQL_SIZE							(SCC_DAL_CONMADATA_SIZE + 1024)
// PARCELテーブルのカラム数
#define	SCC_DAL_TABLE_CLM_NUM_PARCEL				13
// PARCELテーブルのカラムのインデックス
#define	SCC_DAL_TABLE_CLM_IDX_PARCEL				0
#define	SCC_DAL_TABLE_CLM_IDX_PARCEL_BASIS			1
#define	SCC_DAL_TABLE_CLM_IDX_ROAD_SHAPE			2
#define	SCC_DAL_TABLE_CLM_IDX_ROAD_NETWORK			3
#define	SCC_DAL_TABLE_CLM_IDX_BKGD					4
#define	SCC_DAL_TABLE_CLM_IDX_BKGD_AREA_CLS			5
#define	SCC_DAL_TABLE_CLM_IDX_MARK					6
#define	SCC_DAL_TABLE_CLM_IDX_ROAD_NAME				7
#define	SCC_DAL_TABLE_CLM_IDX_NAME					8
#define	SCC_DAL_TABLE_CLM_IDX_GUIDE					9
#define	SCC_DAL_TABLE_CLM_IDX_ROAD_DENSITY			10
#define	SCC_DAL_TABLE_CLM_IDX_ROAD_BASE_VERSION		11
#define	SCC_DAL_TABLE_CLM_IDX_BKGD_BASE_VERSION		12
// PARCELテーブルの道路系カラム数
#define	SCC_DAL_PARCEL_ROAD_CLM_NUM					5
// PARCELテーブルの背景系カラム数
#define	SCC_DAL_PARCEL_BKGD_CLM_NUM					5

// AREA_CLSテーブルのカラム数
#define	SCC_DAL_TABLE_CLM_NUM_AREA_CLS					16
// AREA_CLSテーブルのカラムのインデックス
#define	SCC_DAL_TABLE_CLM_IDX_AREA_CLS_DELETE_FLAG		0
#define	SCC_DAL_TABLE_CLM_IDX_AREA_CLS_CLS6_CODE		1
#define	SCC_DAL_TABLE_CLM_IDX_AREA_CLS_CLS5_CODE		2
#define	SCC_DAL_TABLE_CLM_IDX_AREA_CLS_CLS4_CODE		3
#define	SCC_DAL_TABLE_CLM_IDX_AREA_CLS_CLS3_CODE		4
#define	SCC_DAL_TABLE_CLM_IDX_AREA_CLS_CLS2_CODE		5
#define	SCC_DAL_TABLE_CLM_IDX_AREA_CLS_CLS1_CODE		6
#define	SCC_DAL_TABLE_CLM_IDX_AREA_CLS_REGION_CODE		7
#define	SCC_DAL_TABLE_CLM_IDX_AREA_CLS_LAN_CD			8
#define	SCC_DAL_TABLE_CLM_IDX_AREA_CLS_NAME				9
#define	SCC_DAL_TABLE_CLM_IDX_AREA_CLS_YOMI				10
#define	SCC_DAL_TABLE_CLM_IDX_AREA_CLS_PRIORITY_FLAG	11
#define	SCC_DAL_TABLE_CLM_IDX_AREA_CLS_POSI_LATITUDE	12
#define	SCC_DAL_TABLE_CLM_IDX_AREA_CLS_POSI_LONGITUDE	13
#define	SCC_DAL_TABLE_CLM_IDX_AREA_CLS_ID				14
#define	SCC_DAL_TABLE_CLM_IDX_AREA_CLS_BASE_VERSION		15
// 削除フラグ
#define	SCC_DAL_DELETE_FLAG								'1'
// 改行コード
#define	SCC_DAL_CR									'\r'
#define	SCC_DAL_LF									'\n'

// ■regionListInfo.db
// REGION_INFOテーブル
#define	SCC_DAL_SQL_SELECT_REGION_INFO_LIST	"SELECT REGION_CODE,REGION_NAME,MAP_PRESS_SIZE,MAP_NON_PRESS_SIZE,DATA_PRESS_SIZE,DATA_NON_PRESS_SIZE,DATAVERSIONINFO_MD5 FROM REGION_INFO ORDER BY REGION_CODE ASC"
#define	SCC_DAL_SQL_SELECT_REGION_INFO		"SELECT REGION_NAME,MAP_PRESS_SIZE,MAP_NON_PRESS_SIZE,DATA_PRESS_SIZE,DATA_NON_PRESS_SIZE,DATAVERSIONINFO_MD5 FROM REGION_INFO WHERE REGION_CODE=?"

// ■dataVersionInfo.db
// AREA_INFOテーブル
#define	SCC_DAL_SQL_SELECT_AREA_INFO		"SELECT SECTION_CODE,SECTION_NAME,AREA_CODE,AREA_NAME FROM AREA_INFO ORDER BY SECTION_CODE,AREA_CODE ASC"
// UPDATE_DATAテーブル
#define	SCC_DAL_SQL_SELECT_UPDATE_DATA_LIST	"SELECT UPDATE_DATA.AREA_CODE,APP_VERSION_S,APP_VERSION_E,DATA_VERSION,DATA_TYPE,DATA_KIND,FILE_TYPE,FILE_PATH,PRESS_SIZE,NON_PRESS_SIZE,DATA_NUM,AREA_INFO.AREA_NAME,MD5 FROM UPDATE_DATA LEFT JOIN AREA_INFO ON UPDATE_DATA.AREA_CODE=AREA_INFO.AREA_CODE ORDER BY UPDATE_DATA.AREA_CODE ASC"
#define	SCC_DAL_SQL_SELECT_UPDATE_DATA		"SELECT APP_VERSION_S,APP_VERSION_E,DATA_VERSION,FILE_TYPE,FILE_PATH,PRESS_SIZE,NON_PRESS_SIZE,DATA_NUM,AREA_NAME,MD5 FROM UPDATE_DATA LEFT JOIN AREA_INFO ON UPDATE_DATA.AREA_CODE=AREA_INFO.AREA_CODE WHERE UPDATE_DATA.AREA_CODE=? AND UPDATE_DATA.DATA_TYPE=? AND UPDATE_DATA.DATA_KIND=?"

// ■Polaris.db
// パーセルテーブル（PARCEL）
#define	SCC_DAL_SQL_SELECT_PARCEL_VERSION	"SELECT ROAD_BASE_VERSION,BKGD_BASE_VERSION FROM PARCEL WHERE PARCEL_ID=?"
#define	SCC_DAL_SQL_INSERT_PARCEL			"INSERT INTO PARCEL(PARCEL_ID,PARCEL_BASIS,ROAD_SHAPE,ROAD_NETWORK,BKGD,BKGD_AREA_CLS,MARK,ROAD_NAME,NAME,GUIDE,ROAD_DENSITY,ROAD_BASE_VERSION,BKGD_BASE_VERSION) VALUES("
// 地域クラス名称テーブル（AREA_CLS）
#define	SCC_DAL_SQL_SELECT_AREA_CLS_VERSION	"SELECT AREA_BASE_VERSION FROM AREA_CLS WHERE AREA_CLS6_CODE=? AND AREA_CLS5_CODE=? AND AREA_CLS4_CODE=? AND AREA_CLS3_CODE=? AND AREA_CLS2_CODE=? AND AREA_CLS1_CODE=? AND LAN_CD=? AND ID=?"
#define	SCC_DAL_SQL_INSERT_AREA_CLS			"INSERT INTO AREA_CLS(AREA_CLS6_CODE,AREA_CLS5_CODE,AREA_CLS4_CODE,AREA_CLS3_CODE,AREA_CLS2_CODE,AREA_CLS1_CODE,REGION_CODE,LAN_CD,AREA_CLS_NAME,AREA_CLS_YOMI,PRIORITY_FLAG,POSI_LATITUDE,POSI_LONGITUDE,ID,AREA_BASE_VERSION) VALUES("
#define	SCC_DAL_SQL_DELETE_AREA_CLS			"DELETE FROM AREA_CLS WHERE AREA_CLS6_CODE=? AND AREA_CLS5_CODE=? AND AREA_CLS4_CODE=? AND AREA_CLS3_CODE=? AND AREA_CLS2_CODE=? AND AREA_CLS1_CODE=? AND LAN_CD=? AND ID=?"
// ベース地図データダウンロード管理テーブル（DOWNLOAD_BASE_VERSION）
#define	SCC_DAL_SQL_UPDATE_DL_BASE_VER		"UPDATE DOWNLOAD_BASE_VERSION SET DOWNLOAD_FLAG=?,BASE_VERSION=? WHERE KIND=? AND TABLE_NAME=?"
#define	SCC_DAL_SQL_INSERT_DL_BASE_VER		"INSERT INTO DOWNLOAD_BASE_VERSION(KIND,TABLE_NAME,DOWNLOAD_FLAG,BASE_VERSION,NOTE) VALUES(?,?,?,?,?)"
#define	SCC_DAL_SQL_SELECT_DL_BASE_VER		"SELECT DOWNLOAD_FLAG,BASE_VERSION FROM DOWNLOAD_BASE_VERSION WHERE KIND=? AND TABLE_NAME=?"
// エリア地図データダウンロード管理テーブル（DOWNLOAD_AREA_MAP）
#define	SCC_DAL_SQL_UPDATE_DL_AREA_MAP		"UPDATE DOWNLOAD_AREA_MAP SET DOWNLOAD_FLAG=?,BASE_VERSION=? WHERE ID=? AND KIND=?"
#define	SCC_DAL_SQL_SELECT_DL_AREA_MAP		"SELECT DOWNLOAD_FLAG,BASE_VERSION FROM DOWNLOAD_AREA_MAP WHERE ID=? AND KIND=?"
#define	SCC_DAL_SQL_SELECT_DL_AREA_MAP_LIST	"SELECT ID,KIND,DOWNLOAD_FLAG,BASE_VERSION FROM DOWNLOAD_AREA_MAP ORDER BY ID,KIND ASC"
// システムファイルダウンロード管理テーブル（DOWNLOAD_SYS）
#define	SCC_DAL_SQL_UPDATE_DL_SYS			"UPDATE DOWNLOAD_SYS SET VERSION=? WHERE KIND=?"
#define	SCC_DAL_SQL_INSERT_DL_SYS			"INSERT INTO DOWNLOAD_SYS(KIND,NAME,VERSION) VALUES(?,?,?)"
#define	SCC_DAL_SQL_SELECT_DL_SYS			"SELECT VERSION FROM DOWNLOAD_SYS WHERE KIND=?"
#define	SCC_DAL_SQL_SELECT_DL_SYS_LIST		"SELECT KIND,VERSION FROM DOWNLOAD_SYS"
// カスタマイズデータダウンロード管理テーブル（DOWNLOAD_CUSTOM）
#define	SCC_DAL_SQL_UPDATE_DL_CUSTOM		"UPDATE DOWNLOAD_CUSTOM SET VERSION=? WHERE KIND=? AND SETNUMBER=?"
#define	SCC_DAL_SQL_INSERT_DL_CUSTOM		"INSERT INTO DOWNLOAD_CUSTOM(KIND,SETNUMBER,NAME,VERSION) VALUES(?,?,?,?)"
#define	SCC_DAL_SQL_SELECT_DL_CUSTOM		"SELECT VERSION FROM DOWNLOAD_CUSTOM WHERE KIND=? AND SETNUMBER=?"
#define	SCC_DAL_SQL_SELECT_DL_CUSTOM_LIST	"SELECT KIND,SETNUMBER,VERSION FROM DOWNLOAD_CUSTOM"

// PARCELテーブルのカラム名
static Char	*parcelTablClm[SCC_DAL_TABLE_CLM_NUM_PARCEL] =
{
	"PARCEL_ID",
	"PARCEL_BASIS",
	"ROAD_SHAPE",
	"ROAD_NETWORK",
	"BKGD",
	"BKGD_AREA_CLS",
	"MARK",
	"ROAD_NAME",
	"NAME",
	"GUIDE",
	"ROAD_DENSITY",
	"ROAD_BASE_VERSION",
	"BKGD_BASE_VERSION"
};
// AREA_CLSテーブルのカラム名
static Char	*areaClsTablClm[SCC_DAL_TABLE_CLM_NUM_AREA_CLS] =
{
	NULL,					// 削除フラグ
	"AREA_CLS6_CODE",
	"AREA_CLS5_CODE",
	"AREA_CLS4_CODE",
	"AREA_CLS3_CODE",
	"AREA_CLS2_CODE",
	"AREA_CLS1_CODE",
	"REGION_CODE",
	"LAN_CD",
	"AREA_CLS_NAME",
	"AREA_CLS_YOMI",
	"PRIORITY_FLAG",
	"POSI_LATITUDE",
	"POSI_LONGITUDE",
	"ID",
	"AREA_BASE_VERSION"
};

static void SCC_DAL_GetData(Char *conmaData, Char **clmList, INT32 clmListNum);
static E_SC_RESULT SCC_DAL_GetCsvClmList(const Char *clmKeyList, INT32 clmListNum, Char *conmaData, Char **clmList, FILE **fp);
static E_SC_RESULT SCC_DAL_GetDataVersion(DBOBJECT *db, const Char *sql, const Char *clmKeyList, const Char **clmList, INT32 clmListNum, INT32 *versionList, INT32 versionListNum);

/**
 * @brief DALの初期化
 * @param[in]  dbFilePath       DBファイルパス
 * @param[out] db               SQLITE
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SCC_DAL_Initialize(const Char *dbFilePath, DBOBJECT **db)
{
	E_SC_RESULT	ret	= e_SC_RESULT_SUCCESS;
	INT32		sqliteRet = SQLITE_OK;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == dbFilePath) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[dbFilePath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == db) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[db], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 初期化
		*db = NULL;

		// DBオープン
		sqliteRet = sqlite3_open(dbFilePath, db);
		if (SQLITE_OK != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_open error(0x%08x), " HERE, sqliteRet);
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"dbFilePath=%s, " HERE, dbFilePath);
			ret = e_SC_RESULT_RDB_ACCESSERR;
			break;
		}
	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief DALの終了化
 * @param[in]  db               SQLITE
 * @return 処理結果(E_SC_RESULT)
 */
void SCC_DAL_Finalize(DBOBJECT *db)
{
	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	if (NULL != db) {
		// DBクローズ
		sqlite3_close(db);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}

/**
 * @brief トランザクション開始
 * @param[in]  db               SQLITE
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SCC_DAL_Transaction(DBOBJECT *db)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	sqliteRet = SQLITE_OK;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == db) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[db], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// トランザクション開始
		sqliteRet = sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL);
		if (SQLITE_OK != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_exec(transaction) error(0x%08x), " HERE, sqliteRet);
			ret = e_SC_RESULT_RDB_ACCESSERR;
			break;
		}
	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief コミット
 * @param[in]  db               SQLITE
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SCC_DAL_Commit(DBOBJECT *db)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	sqliteRet = SQLITE_OK;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == db) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[db], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// コミット
		sqliteRet = sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
		if (SQLITE_OK != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_exec(commit) error(0x%08x), " HERE, sqliteRet);
			ret = e_SC_RESULT_RDB_ACCESSERR;
			break;
		}
	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief ロールバック
 * @param[in]  db               SQLITE
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SCC_DAL_Rollback(DBOBJECT *db)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	sqliteRet = SQLITE_OK;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == db) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[db], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// ロールバック
		sqliteRet = sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
		if (SQLITE_OK != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_exec(rollback) error(0x%08x), " HERE, sqliteRet);
			ret = e_SC_RESULT_RDB_ACCESSERR;
		}
	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}


//-----------------------------------------------------------------------------
// regionListInfo.db
//-----------------------------------------------------------------------------
/**
 * @brief REGION_INFOテーブルのデータ取得(全件)
 * @param[in]  db               SQLITE
 * @param[in]  getRegionInfoNum 取得するデータ最大数
 * @param[in]  regionInfo       取得したデータ
 * @param[in]  regionInfoNum    取得したデータ数
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SCC_DAL_GetRegionInfoList(DBOBJECT *db,
									  INT32 getRegionInfoNum,
									  SCC_REGIONINFO *regionInfo,
									  INT32 *regionInfoNum)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	sqliteRet = SQLITE_OK;
	sqlite3_stmt	*stmt = NULL;
	INT32	size = 0;
	INT32	num = 0;
	UChar	md5[CC_CMN_MD5 * 2] = {};

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == db) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[db], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == regionInfo) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[regionInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (0 >= getRegionInfoNum) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[getRegionInfoNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == regionInfoNum) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[regionInfoNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// SQLITE prepare
		sqliteRet = sqlite3_prepare(db, SCC_DAL_SQL_SELECT_REGION_INFO_LIST, strlen(SCC_DAL_SQL_SELECT_REGION_INFO_LIST), &stmt, NULL);
		if (SQLITE_OK != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_prepare(select) error(0x%08x), " HERE, sqliteRet);
			ret = e_SC_RESULT_RDB_ACCESSERR;
			break;
		}

		// stmtの内部バッファを一旦クリア
		sqlite3_reset(stmt);

		// SQLITE step
		while (SQLITE_ROW == (sqliteRet = sqlite3_step(stmt))) {
			// REGION_CODE
			size = sqlite3_column_bytes(stmt, 0);
			memcpy(regionInfo[num].regionCode, (const char *) sqlite3_column_text(stmt, 0), size);
			regionInfo[num].regionCode[size] = EOS;

			// REGION_NAME
			size = sqlite3_column_bytes(stmt, 1);
			memcpy(regionInfo[num].regionName, (const char *) sqlite3_column_text(stmt, 1), size);
			regionInfo[num].regionName[size] = EOS;

			// MAP_PRESS_SIZE
			regionInfo[num].mapPressSize = sqlite3_column_int64(stmt, 2);

			// MAP_NON_PRESS_SIZE
			regionInfo[num].mapNonPressize = sqlite3_column_int64(stmt, 3);

			// DATA_PRESS_SIZE
			regionInfo[num].dataPressSize = sqlite3_column_int64(stmt, 4);

			// DATA_NON_PRESS_SIZE
			regionInfo[num].dataNonPressSize = sqlite3_column_int64(stmt, 5);

			// DATAVERSIONINFO_MD5
			size = sqlite3_column_bytes(stmt, 6);
			memcpy(md5, (const char *) sqlite3_column_text(stmt, 6), size);
			// 16進数文字列をバイトの16進数コードの文字列に変換
			if (e_SC_RESULT_SUCCESS != CC_ChgHexString(md5, size, regionInfo[num].md5)) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ChgHexString error, " HERE);
				memset(regionInfo[num].md5, 0, sizeof(regionInfo[num].md5));
			}

			num++;
			if (num == getRegionInfoNum) {
				// データ取得を終了する
				sqliteRet = SQLITE_DONE;
				break;
			}
		}
		if (SQLITE_DONE != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_step error(0x%08x), " HERE, sqliteRet);
			ret = e_SC_RESULT_RDB_ACCESSERR;
			break;
		}

		// 件数を設定
		*regionInfoNum = num;
	} while (0);

	if (NULL != stmt) {
		// SQLITE finalize
		sqliteRet = sqlite3_finalize(stmt);
		if (SQLITE_OK != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_finalize error(0x%08x), " HERE, sqliteRet);
		}
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief REGION_INFOテーブルのデータ取得(1件)
 * @param[in]  db               SQLITE
 * @param[in/out] regionInfo    検索条件/取得したデータ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SCC_DAL_GetRegionInfo(DBOBJECT *db,
								  SCC_REGIONINFO *regionInfo)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	sqliteRet = SQLITE_OK;
	sqlite3_stmt	*stmt = NULL;
	INT32	size = 0;
	UChar	md5[CC_CMN_MD5 * 2] = {};

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == db) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[db], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == regionInfo) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[regionInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (EOS == *regionInfo->regionCode) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[regionInfo->regionCode], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 初期化
		regionInfo->regionName[0] = EOS;
		regionInfo->mapPressSize = 0;
		regionInfo->mapNonPressize = 0;
		regionInfo->dataPressSize = 0;
		regionInfo->dataNonPressSize = 0;

		// SQLITE prepare
		sqliteRet = sqlite3_prepare(db, SCC_DAL_SQL_SELECT_REGION_INFO, strlen(SCC_DAL_SQL_SELECT_REGION_INFO), &stmt, NULL);
		if (SQLITE_OK != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_prepare(select) error(0x%08x), " HERE, sqliteRet);
			ret = e_SC_RESULT_RDB_ACCESSERR;
			break;
		}

		// stmtの内部バッファを一旦クリア
		sqlite3_reset(stmt);

		// sqlの?の部分に、値を設定
		sqlite3_bind_text(stmt, 1, regionInfo->regionCode, strlen(regionInfo->regionCode), SQLITE_TRANSIENT);

		// SQLITE step
		if (SQLITE_ROW == (sqliteRet = sqlite3_step(stmt))) {
			// REGION_NAME
			size = sqlite3_column_bytes(stmt, 0);
			memcpy(regionInfo->regionName, (const char *) sqlite3_column_text(stmt, 0), size);
			regionInfo->regionName[size] = EOS;

			// MAP_PRESS_SIZE
			regionInfo->mapPressSize = sqlite3_column_int64(stmt, 1);

			// MAP_NON_PRESS_SIZE
			regionInfo->mapNonPressize = sqlite3_column_int64(stmt, 2);

			// DATA_PRESS_SIZE
			regionInfo->dataPressSize = sqlite3_column_int64(stmt, 3);

			// DATA_NON_PRESS_SIZE
			regionInfo->dataNonPressSize = sqlite3_column_int64(stmt, 4);

			// DATAVERSIONINFO_MD5
			size = sqlite3_column_bytes(stmt, 5);
			memcpy(md5, (const char *) sqlite3_column_text(stmt, 5), size);
			// 16進数文字列をバイトの16進数コードの文字列に変換
			if (e_SC_RESULT_SUCCESS != CC_ChgHexString(md5, size, regionInfo->md5)) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ChgHexString error, " HERE);
				memset(regionInfo->md5, 0, sizeof(regionInfo->md5));
			}
		} else {
			if ((SQLITE_NOTFOUND == sqliteRet) ||
				(SQLITE_DONE == sqliteRet)) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"no data, " HERE);
				ret = e_SC_RESULT_NODATA;
			} else {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_step(stmt) error(0x%08x), " HERE, sqliteRet);
				ret = e_SC_RESULT_RDB_ACCESSERR;
			}
			break;
		}
	} while (0);

	if (NULL != stmt) {
		// SQLITE finalize
		sqliteRet = sqlite3_finalize(stmt);
		if (SQLITE_OK != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_finalize error(0x%08x), " HERE, sqliteRet);
		}
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//-----------------------------------------------------------------------------
// dataVersionInfo.db
//-----------------------------------------------------------------------------
/**
 * @brief AREA_INFOテーブルのデータ取得(全件)
 * @param[in]  db               SQLITE
 * @param[in]  getAreaInfoNum   取得するデータ最大数
 * @param[in]  areaInfo         取得したデータ
 * @param[in]  areaInfoNum      取得したデータ数
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SCC_DAL_GetAreaInfoList(DBOBJECT *db,
									INT32 getAreaInfoNum,
									SCC_AREAINFO *areaInfo,
									INT32 *areaInfoNum)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	sqliteRet = SQLITE_OK;
	sqlite3_stmt	*stmt = NULL;
	INT32	size = 0;
	INT32	num = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == db) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[db], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == areaInfo) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[areaInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (0 >= getAreaInfoNum) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[getAreaInfoNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == areaInfoNum) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[areaInfoNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// SQLITE prepare
		sqliteRet = sqlite3_prepare(db, SCC_DAL_SQL_SELECT_AREA_INFO, strlen(SCC_DAL_SQL_SELECT_AREA_INFO), &stmt, NULL);
		if (SQLITE_OK != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_prepare(select) error(0x%08x), " HERE, sqliteRet);
			ret = e_SC_RESULT_RDB_ACCESSERR;
			break;
		}

		// stmtの内部バッファを一旦クリア
		sqlite3_reset(stmt);

		// SQLITE step
		while (SQLITE_ROW == (sqliteRet = sqlite3_step(stmt))) {
			// SECTION_CODE
			areaInfo[num].sectionCode = sqlite3_column_int(stmt, 0);

			// SECTION_NAME
			size = sqlite3_column_bytes(stmt, 1);
			memcpy(areaInfo[num].sectionName, (const char *) sqlite3_column_text(stmt, 1), size);
			areaInfo[num].sectionName[size] = EOS;

			// AREA_CODE
			areaInfo[num].areaCode = sqlite3_column_int(stmt, 2);

			// AREA_NAME
			size = sqlite3_column_bytes(stmt, 3);
			memcpy(areaInfo[num].areaName, (const char *) sqlite3_column_text(stmt, 3), size);
			areaInfo[num].areaName[size] = EOS;

			num++;
			if (num == getAreaInfoNum) {
				// データ取得を終了する
				sqliteRet = SQLITE_DONE;
				break;
			}
		}
		if (SQLITE_DONE != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_step error(0x%08x), " HERE, sqliteRet);
			ret = e_SC_RESULT_RDB_ACCESSERR;
			break;
		}

		// 件数を設定
		*areaInfoNum = num;
	} while (0);

	if (NULL != stmt) {
		// SQLITE finalize
		sqliteRet = sqlite3_finalize(stmt);
		if (SQLITE_OK != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_finalize error(0x%08x), " HERE, sqliteRet);
		}
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief UPDATE_DATAテーブルのデータリスト取得(全件)
 * @param[in]  db               SQLITE
 * @param[in]  maxUpdateDataNum 取得するデータ最大数
 * @param[out] updateData       取得したデータ
 * @param[out] updateDataNum    取得したデータ数
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SCC_DAL_GetUpdateDataList(DBOBJECT *db,
									  INT32 maxUpdateDataNum,
									  SCC_UPDATEDATA *updateData,
									  INT32 *updateDataNum)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	sqliteRet = SQLITE_OK;
	sqlite3_stmt	*stmt = NULL;
	INT32	size = 0;
	INT32	num = 0;
	UChar	md5[CC_CMN_MD5 * 2] = {};

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == db) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[db], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (0 >= maxUpdateDataNum) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[maxUpdateDataNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == updateData) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[updateData], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == updateDataNum) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[updateDataNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 初期化
		memset(updateData, 0, (sizeof(SCC_UPDATEDATA) * maxUpdateDataNum));

		// SQLITE prepare
		sqliteRet = sqlite3_prepare(db, SCC_DAL_SQL_SELECT_UPDATE_DATA_LIST, strlen(SCC_DAL_SQL_SELECT_UPDATE_DATA_LIST), &stmt, NULL);
		if (SQLITE_OK != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_prepare(select) error(0x%08x), " HERE, sqliteRet);
			ret = e_SC_RESULT_RDB_ACCESSERR;
			break;
		}

		// stmtの内部バッファを一旦クリア
		sqlite3_reset(stmt);

		// SQLITE step
		while (SQLITE_ROW == (sqliteRet = sqlite3_step(stmt))) {
			// AREA_CODE
			updateData[num].areaCode = sqlite3_column_int(stmt, 0);

			// APP_VERSION_S
			size = sqlite3_column_bytes(stmt, 1);
			memcpy(updateData[num].appVersionS, (const char *) sqlite3_column_text(stmt, 1), size);
			updateData[num].appVersionS[size] = EOS;

			// APP_VERSION_E
			size = sqlite3_column_bytes(stmt, 2);
			memcpy(updateData[num].appVersionE, (const char *) sqlite3_column_text(stmt, 2), size);
			updateData[num].appVersionE[size] = EOS;

			// DATA_VERSION
			updateData[num].dataVersion = sqlite3_column_int(stmt, 3);

			// DATA_TYPE
			updateData[num].dataType = sqlite3_column_int(stmt, 4);

			// DATA_KIND
			size = sqlite3_column_bytes(stmt, 5);
			memcpy(updateData[num].dataKind, (const char *) sqlite3_column_text(stmt, 5), size);
			updateData[num].dataKind[size] = EOS;

			// FILE_TYPE
			updateData[num].fileType = sqlite3_column_int(stmt, 6);

			// FILE_PATH
			size = sqlite3_column_bytes(stmt, 7);
			memcpy(updateData[num].filePath, (const char *) sqlite3_column_text(stmt, 7), size);
			updateData[num].filePath[size] = EOS;

			// PRESS_SIZE
			updateData[num].pressSize = sqlite3_column_int64(stmt, 8);

			// NON_PRESS_SIZE
			updateData[num].nonPressSize = sqlite3_column_int64(stmt, 9);

			// DATA_NUM
			updateData[num].dataNum = sqlite3_column_int(stmt, 10);

			// AREA_NAME
			size = sqlite3_column_bytes(stmt, 11);
			memcpy(updateData[num].areaName, (const char *) sqlite3_column_text(stmt, 11), size);
			updateData[num].areaName[size] = EOS;

			// MD5
			size = sqlite3_column_bytes(stmt, 12);
			memcpy(md5, (const char *) sqlite3_column_text(stmt, 12), size);
			// 16進数文字列をバイトの16進数コードの文字列に変換
			if (e_SC_RESULT_SUCCESS != CC_ChgHexString(md5, size, updateData[num].md5)) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ChgHexString error, " HERE);
				memset(updateData[num].md5, 0, sizeof(updateData[num].md5));
			}

			num++;
			if (num == maxUpdateDataNum) {
				// データ取得を終了する
				sqliteRet = SQLITE_DONE;
				break;
			}
		}
		if (SQLITE_DONE != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_step error(0x%08x), " HERE, sqliteRet);
			ret = e_SC_RESULT_RDB_ACCESSERR;
			break;
		}

		// 件数を設定
		*updateDataNum = num;
	} while (0);

	if (NULL != stmt) {
		// SQLITE finalize
		sqliteRet = sqlite3_finalize(stmt);
		if (SQLITE_OK != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_finalize error(0x%08x), " HERE, sqliteRet);
		}
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief UPDATE_DATAテーブルのデータ取得(1件)
 * @param[in]  db               SQLITE
 * @param[in/out] updateData    検索条件/取得したデータ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SCC_DAL_GetUpdateData(DBOBJECT *db,
								  SCC_UPDATEDATA *updateData)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	sqliteRet = SQLITE_OK;
	sqlite3_stmt	*stmt = NULL;
	INT32	size = 0;
	UChar	md5[CC_CMN_MD5 * 2] = {};

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == db) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[db], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == updateData) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[updateData], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((SCC_DAL_UPDDATA_DATA_TYPE_AREA_MAP == updateData->dataType) && (0 >= updateData->areaCode)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[areaCode], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		} else if (0 > updateData->areaCode) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[areaCode], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 初期化
		updateData->appVersionS[0] = EOS;
		updateData->appVersionE[0] = EOS;
		updateData->dataVersion = 0;
		updateData->fileType = 0;
		updateData->filePath[0] = EOS;
		updateData->pressSize = 0;
		updateData->nonPressSize = 0;
		updateData->dataNum = 0;
		updateData->areaName[0] = EOS;
		memset(updateData->md5, 0, sizeof(updateData->md5));

		// SQLITE prepare
		sqliteRet = sqlite3_prepare(db, SCC_DAL_SQL_SELECT_UPDATE_DATA, strlen(SCC_DAL_SQL_SELECT_UPDATE_DATA), &stmt, NULL);
		if (SQLITE_OK != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_prepare(select) error(0x%08x), " HERE, sqliteRet);
			ret = e_SC_RESULT_RDB_ACCESSERR;
			break;
		}

		// stmtの内部バッファを一旦クリア
		sqlite3_reset(stmt);

		// sqlの?の部分に、値を設定
		sqlite3_bind_int(stmt,  1, updateData->areaCode);
		sqlite3_bind_int(stmt,  2, updateData->dataType);
		sqlite3_bind_text(stmt, 3, updateData->dataKind, strlen(updateData->dataKind), SQLITE_TRANSIENT);

		// SQLITE step
		if (SQLITE_ROW == (sqliteRet = sqlite3_step(stmt))) {
			// APP_VERSION_S
			size = sqlite3_column_bytes(stmt, 0);
			memcpy(updateData->appVersionS, (const char *) sqlite3_column_text(stmt, 0), size);
			updateData->appVersionS[size] = EOS;

			// APP_VERSION_E
			size = sqlite3_column_bytes(stmt, 1);
			memcpy(updateData->appVersionE, (const char *) sqlite3_column_text(stmt, 1), size);
			updateData->appVersionE[size] = EOS;

			// DATA_VERSION
			updateData->dataVersion = sqlite3_column_int(stmt, 2);

			// FILE_TYPE
			updateData->fileType = sqlite3_column_int(stmt, 3);

			// FILE_PATH
			size = sqlite3_column_bytes(stmt, 4);
			memcpy(updateData->filePath, (const char *) sqlite3_column_text(stmt, 4), size);
			updateData->filePath[size] = EOS;

			// PRESS_SIZE
			updateData->pressSize = sqlite3_column_int64(stmt, 5);

			// NON_PRESS_SIZE
			updateData->nonPressSize = sqlite3_column_int64(stmt, 6);

			// DATA_NUM
			updateData->dataNum = sqlite3_column_int(stmt, 7);

			// AREA_NAME
			size = sqlite3_column_bytes(stmt, 8);
			memcpy(updateData->areaName, (const char *) sqlite3_column_text(stmt, 8), size);
			updateData->areaName[size] = EOS;

			// MD5
			size = sqlite3_column_bytes(stmt, 9);
			memcpy(md5, (const char *) sqlite3_column_text(stmt, 9), size);
			// 16進数文字列をバイトの16進数コードの文字列に変換
			if (e_SC_RESULT_SUCCESS != CC_ChgHexString(md5, size, updateData->md5)) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ChgHexString error, " HERE);
				memset(updateData->md5, 0, sizeof(updateData->md5));
			}
		} else {
			if ((SQLITE_NOTFOUND == sqliteRet) ||
				(SQLITE_DONE == sqliteRet)) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"no data, " HERE);
				ret = e_SC_RESULT_NODATA;
			} else {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_step(stmt) error(0x%08x), " HERE, sqliteRet);
				ret = e_SC_RESULT_RDB_ACCESSERR;
			}
			break;
		}
	} while (0);

	if (NULL != stmt) {
		// SQLITE finalize
		sqliteRet = sqlite3_finalize(stmt);
		if (SQLITE_OK != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_finalize error(0x%08x), " HERE, sqliteRet);
		}
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//-----------------------------------------------------------------------------
// Polaris.db
//-----------------------------------------------------------------------------
/**
 * @brief PARCELテーブル更新
 * @param[in]  db           SQLITE
 * @param[in]  filePath     CSVファイルパス
 * @param[in]  transaction  トランザクションを開始するか否か（true:する、false:しない）
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SCC_DAL_UpdateParcel(DBOBJECT *db, const Char *filePath, Bool transaction, const SMPROGRESSCBFNC *callbackFnc)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32		sqliteRet = SQLITE_OK;
	sqlite3_stmt	*stmt = NULL;
	Char	*conmaData = NULL;
	Char	*sql = NULL;
	FILE	*fp = NULL;
	Char	*clmList[SCC_DAL_TABLE_CLM_NUM_PARCEL] = {};
	INT32	num = 0;
	INT32	updClmNum = 0;
	INT32	baseVersion[2] = {};
	INT32	idx = 0;
	const INT32	roadClmList[SCC_DAL_PARCEL_ROAD_CLM_NUM] ={
			SCC_DAL_TABLE_CLM_IDX_ROAD_SHAPE,			// ROAD_SHAPE
			SCC_DAL_TABLE_CLM_IDX_ROAD_NETWORK,			// ROAD_NETWORK
			SCC_DAL_TABLE_CLM_IDX_ROAD_NAME,			// ROAD_NAME
			SCC_DAL_TABLE_CLM_IDX_GUIDE,				// GUIDE
			SCC_DAL_TABLE_CLM_IDX_ROAD_BASE_VERSION		// ROAD_BASE_VERSION
	};
	const INT32	bkgdClmList[SCC_DAL_PARCEL_BKGD_CLM_NUM] ={
			SCC_DAL_TABLE_CLM_IDX_BKGD,					// BKGD
			SCC_DAL_TABLE_CLM_IDX_BKGD_AREA_CLS,		// BKGD_AREA_CLS
			SCC_DAL_TABLE_CLM_IDX_MARK,					// MARK
			SCC_DAL_TABLE_CLM_IDX_NAME,					// NAME
			SCC_DAL_TABLE_CLM_IDX_BKGD_BASE_VERSION		// BKGD_BASE_VERSION
	};
	Char	keyList[SCC_DAL_TABLE_CLM_NUM_PARCEL] = {
					'1', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'
	};

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == db) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[db], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((NULL == filePath) || (EOS == *filePath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[filePath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// メモリ確保
		conmaData = (Char*)SCC_MALLOC2(SCC_DAL_CONMADATA_SIZE);
		if (NULL == conmaData) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC2 error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		sql = (Char*)SCC_MALLOC2(SCC_DAL_SQL_SIZE);
		if (NULL == sql) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC2 error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// ファイルオープン
		fp = fopen(filePath, "r");
		if (NULL == fp) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"fopen error(%s), " HERE, filePath);
			ret = e_SC_RESULT_FILE_OPENERR;
			break;
		}

		if (transaction) {
			// トランザクション開始
			ret = SCC_DAL_Transaction(db);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Transaction error, " HERE);
				break;
			}
		}

		while (0 == feof(fp)) {
			// キャンセルチェック
			if ((NULL != callbackFnc) && (NULL != callbackFnc->cancel) && (callbackFnc->cancel())) {
				SCC_LOG_WarnPrint(SC_TAG_CC, (Char*)"cancel, " HERE);
				ret = e_SC_RESULT_CANCEL;
				break;
			}
			// CSVファイルからカラムリスト取得
			ret = SCC_DAL_GetCsvClmList(keyList, SCC_DAL_TABLE_CLM_NUM_PARCEL, conmaData, clmList, &fp);
			if (e_SC_RESULT_NODATA == ret) {
				// EOF
				ret = e_SC_RESULT_SUCCESS;
				break;
			} else if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_GetCsvClmList error, " HERE);
				break;
			}

			// バージョン取得
			ret = SCC_DAL_GetDataVersion(db, SCC_DAL_SQL_SELECT_PARCEL_VERSION, keyList, (const Char**)clmList, SCC_DAL_TABLE_CLM_NUM_PARCEL, baseVersion, 2);
			if ((e_SC_RESULT_NODATA != ret) && (e_SC_RESULT_SUCCESS != ret)) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_GetDataVersion error, " HERE);
				break;
			}

			updClmNum = 0;
			if (e_SC_RESULT_NODATA == ret) {
				ret = e_SC_RESULT_SUCCESS;
				// INSERT SQL文生成
				strcpy(sql, SCC_DAL_SQL_INSERT_PARCEL);
				for (num = 0; num < SCC_DAL_TABLE_CLM_NUM_PARCEL; num++) {
					if (0 < updClmNum) {
						strcat(sql, ",");
					}
					if ((NULL != clmList[num]) && (EOS != *clmList[num]) && (0 != strcmp(clmList[num], "''"))) {
						sprintf((char*)&sql[strlen((char*)sql)], "%s", clmList[num]);
					} else {
						sprintf((char*)&sql[strlen((char*)sql)], "null");
					}
					updClmNum++;
				}
				sprintf((char*)&sql[strlen((char*)sql)], ")");

				// INSERT
				sqliteRet = sqlite3_exec(db, sql, NULL, NULL, NULL);
				if ((SQLITE_OK != sqliteRet) || (1 != sqlite3_changes(db))) {
					SCC_LOG_InfoPrint(SC_TAG_CC, (Char*)"sqlite3_exec(insert) error(0x%08x), " HERE, sqliteRet);
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sql=%s, " HERE, sql);
					ret = e_SC_RESULT_RDB_ACCESSERR;
					break;
				}
			} else {
				// UPDATE SQL文生成
				strcpy((char*)sql, "UPDATE PARCEL SET ");
				// PARCEL_BASIS
				if ((NULL != clmList[SCC_DAL_TABLE_CLM_IDX_PARCEL_BASIS]) && (EOS != *clmList[SCC_DAL_TABLE_CLM_IDX_PARCEL_BASIS])) {
					sprintf((char*)&sql[strlen((char*)sql)], "%s=%s", parcelTablClm[SCC_DAL_TABLE_CLM_IDX_PARCEL_BASIS], clmList[SCC_DAL_TABLE_CLM_IDX_PARCEL_BASIS]);
					updClmNum++;
				}
				// ROAD_DENSITY
				if ((NULL != clmList[SCC_DAL_TABLE_CLM_IDX_ROAD_DENSITY]) && (EOS != *clmList[SCC_DAL_TABLE_CLM_IDX_ROAD_DENSITY])) {
					if (0 < updClmNum) {
						strcat(sql, ",");
					}
					if (0 != strcmp(clmList[SCC_DAL_TABLE_CLM_IDX_ROAD_DENSITY], "''")) {
						sprintf((char*)&sql[strlen((char*)sql)], "%s=%s", parcelTablClm[SCC_DAL_TABLE_CLM_IDX_ROAD_DENSITY], clmList[SCC_DAL_TABLE_CLM_IDX_ROAD_DENSITY]);
					} else {
						sprintf((char*)&sql[strlen((char*)sql)], "%s=null", parcelTablClm[SCC_DAL_TABLE_CLM_IDX_ROAD_DENSITY]);
					}
					updClmNum++;
				}

				// 道路系バージョン
				if ((NULL != clmList[SCC_DAL_TABLE_CLM_IDX_ROAD_BASE_VERSION]) &&
					(EOS  != *clmList[SCC_DAL_TABLE_CLM_IDX_ROAD_BASE_VERSION]) &&
					(0    != strcmp(clmList[SCC_DAL_TABLE_CLM_IDX_ROAD_BASE_VERSION], "''"))) {
					// ベースバージョンチェック
					if (atoi(clmList[SCC_DAL_TABLE_CLM_IDX_ROAD_BASE_VERSION]) > baseVersion[0]) {
						for (num = 0; num < SCC_DAL_PARCEL_ROAD_CLM_NUM; num++) {
							idx = roadClmList[num];
							if ((NULL != clmList[idx]) && (EOS != *clmList[idx])) {
								if (0 < updClmNum) {
									strcat(sql, ",");
								}
								if (0 != strcmp(clmList[idx], "''")) {
									sprintf((char*)&sql[strlen((char*)sql)], "%s=%s", parcelTablClm[idx], clmList[idx]);
								} else {
									sprintf((char*)&sql[strlen((char*)sql)], "%s=null", parcelTablClm[idx]);
								}
								updClmNum++;
							}
						}
					}
				}

				// 背景系バージョン
				if ((NULL != clmList[SCC_DAL_TABLE_CLM_IDX_BKGD_BASE_VERSION]) &&
					(EOS  != *clmList[SCC_DAL_TABLE_CLM_IDX_BKGD_BASE_VERSION]) &&
					(0    != strcmp(clmList[SCC_DAL_TABLE_CLM_IDX_BKGD_BASE_VERSION], "''"))) {
					// ベースバージョンチェック
					if (atoi(clmList[SCC_DAL_TABLE_CLM_IDX_BKGD_BASE_VERSION]) > baseVersion[1]) {
						for (num = 0; num < SCC_DAL_PARCEL_BKGD_CLM_NUM; num++) {
							idx = bkgdClmList[num];
							if ((NULL != clmList[idx]) && (EOS != *clmList[idx])) {
								if (0 < updClmNum) {
									strcat(sql, ",");
								}
								if (0 != strcmp(clmList[idx], "''")) {
									sprintf((char*)&sql[strlen((char*)sql)], "%s=%s", parcelTablClm[idx], clmList[idx]);
								} else {
									sprintf((char*)&sql[strlen((char*)sql)], "%s=null", parcelTablClm[idx]);
								}
								updClmNum++;
							}
						}
					}
				}

				if (0 == updClmNum) {
					// 更新なし
					// 進捗通知
					if ((NULL != callbackFnc) && (NULL != callbackFnc->progress)) {
						callbackFnc->progress(1);
					}
					continue;
				}
				sprintf((char*)&sql[strlen((char*)sql)],
						" WHERE %s=%s", parcelTablClm[SCC_DAL_TABLE_CLM_IDX_PARCEL], clmList[SCC_DAL_TABLE_CLM_IDX_PARCEL]);

				// UPDATE
				sqliteRet = sqlite3_exec(db, sql, NULL, NULL, NULL);
				if ((SQLITE_OK != sqliteRet) || (1 != sqlite3_changes(db))) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_exec(update) error(0x%08x), " HERE, sqliteRet);
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sql=%s, " HERE, sql);
					ret = e_SC_RESULT_RDB_ACCESSERR;
					break;
				}
			}

			// 進捗通知
			if ((NULL != callbackFnc) && (NULL != callbackFnc->progress)) {
				callbackFnc->progress(1);
			}
		}
		if (transaction) {
			if (e_SC_RESULT_SUCCESS == ret) {
				// コミット
				ret = SCC_DAL_Commit(db);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Commit error, " HERE);
					// ロールバック
					if (e_SC_RESULT_SUCCESS != SCC_DAL_Rollback(db)) {
						SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Rollback error, " HERE);
					}
					break;
				}
			} else {
				// ロールバック
				if (e_SC_RESULT_SUCCESS != SCC_DAL_Rollback(db)) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Rollback error, " HERE);
				}
				break;
			}
		}
	} while (0);

	// メモリ解放
	if (NULL != conmaData) {
		SCC_FREE2(conmaData);
	}
	if (NULL != sql) {
		SCC_FREE2(sql);
	}
	// ファイルクローズ
	if (NULL != fp) {
		fclose(fp);
	}
	if (NULL != stmt) {
		// SQLITE finalize
		sqliteRet = sqlite3_finalize(stmt);
		if (SQLITE_OK != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_finalize error(0x%08x), " HERE, sqliteRet);
		}
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief データ取得
 * @param[in]  conmaData    カンマ区切りのデータ
 * @param[out] clmList      カラムリスト
 * @param[out] clmListNum   カラムリスト数
 */
void SCC_DAL_GetData(Char *conmaData, Char **clmList, INT32 clmListNum)
{
	INT32	dataLen = 0;
	INT32	len = 0;
	INT32	num = 0;
	INT32	num2 = 0;
	Char	*data = conmaData;
	Char	*chr = NULL;

	dataLen = strlen(data);

	for (num = 0; num < clmListNum; num++) {
		if (len >= dataLen) {
			break;
		}
		clmList[num] = data;
		chr = strchr(data, ',');
		if (NULL == chr) {
			chr = strchr(data, SCC_DAL_CR);
			if (NULL != chr) {
				*chr = EOS;
				chr = strchr((data + 1), SCC_DAL_LF);
				if (NULL != chr) {
					*chr = EOS;
				}
			} else {
				chr = strchr(data, SCC_DAL_LF);
				if (NULL != chr) {
					*chr = EOS;
				}
			}
			for (num2 = (num + 1); num2 < clmListNum; num2++) {
				clmList[num2] = EOS;
			}
			break;
		}
		*chr = EOS;
		len = (INT32)(chr - conmaData) + 1;
		data = (chr + 1);
	}
}

/**
 * @brief CSVファイルからカラムリスト取得
 * @param[in]  clmKeyList   カラムキーリスト
 * @param[in]  clmListNum   カラムリスト数
 * @param[out] conmaData    カンマ区切りのデータ格納バッファ
 * @param[out] clmList      カラムリスト
 * @param[out] fp           リードするCSVファイルのファイルポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SCC_DAL_GetCsvClmList(const Char *clmKeyList, INT32 clmListNum, Char *conmaData, Char **clmList, FILE **fp)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32		num = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// ファイルリード
		if (NULL == fgets(conmaData, SCC_DAL_CONMADATA_SIZE, *fp)) {
			if (0 == feof(*fp)) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"fgets error, " HERE);
				ret = e_SC_RESULT_FILE_ACCESSERR;
			} else {
				ret = e_SC_RESULT_NODATA;
			}
			break;
		}
		// データ取得
		SCC_DAL_GetData(conmaData, clmList, clmListNum);
		for (num = 0; num < clmListNum; num++) {
			if (('1' == clmKeyList[num]) &&
				(EOS == *clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_DELETE_FLAG])) {
				// 主キーがない
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"primary key(%d) error, " HERE, (num + 1));
				ret = e_SC_RESULT_MAP_DATA_ERR;
				break;
			}
		}


	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief DOWNLOAD_BASE_VERSIONテーブルのデータ更新
 * @param[in]  db           SQLITE
 * @param[in]  dlBaseVer    更新するデータ
 * @param[in]  dlAreaMapNum 更新するデータ数
 * @param[in]  transaction  トランザクションを開始するか否か（true:する、false:しない）
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SCC_DAL_UpdDLBaseVersion(DBOBJECT *db,
									 const SCC_DOWNLOADBASEVERSION *dlBaseVer,
									 INT32 dlBaseVerNum,
									 Bool transaction)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	sqliteRet = SQLITE_OK;
	sqlite3_stmt	*stmt = NULL;
	Char	*sql = NULL;
	INT32	num = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == db) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[db], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == dlBaseVer) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[dlBaseVer], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// メモリ確保
		sql = (Char*)SCC_MALLOC2(SCC_DAL_SQL_SIZE);
		if (NULL == sql) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC2 error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		if (transaction) {
			// トランザクション開始
			ret = SCC_DAL_Transaction(db);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Transaction error, " HERE);
				break;
			}
		}

		for (num = 0; num < dlBaseVerNum; num++) {
			// SQLITE prepare
			sqliteRet = sqlite3_prepare(db, SCC_DAL_SQL_UPDATE_DL_BASE_VER, strlen(SCC_DAL_SQL_UPDATE_DL_BASE_VER), &stmt, NULL);
			if (SQLITE_OK != sqliteRet) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_prepare error(0x%08x), " HERE, sqliteRet);
				ret = e_SC_RESULT_RDB_ACCESSERR;
				break;
			}

			// stmtの内部バッファを一旦クリア
			sqlite3_reset(stmt);

			// sqlの?の部分に、値を設定
			sqlite3_bind_int(stmt,  1, dlBaseVer[num].dlFlag);
			sqlite3_bind_int(stmt,  2, dlBaseVer[num].baseVersion);
			sqlite3_bind_text(stmt, 3, dlBaseVer[num].kind, strlen(dlBaseVer[num].kind), SQLITE_TRANSIENT);
			sqlite3_bind_text(stmt, 4, dlBaseVer[num].tableName, strlen(dlBaseVer[num].tableName), SQLITE_TRANSIENT);

			// stmtのSQLを実行
			sqliteRet = sqlite3_step(stmt);
			if ((SQLITE_DONE != sqliteRet) || (1 != sqlite3_changes(db))) {
				SCC_LOG_InfoPrint(SC_TAG_CC, (Char*)"sqlite3_step(update) error(0x%08x), " HERE, sqliteRet);
				// SQLITE finalize
				sqliteRet = sqlite3_finalize(stmt);
				if (SQLITE_OK != sqliteRet) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_finalize error(0x%08x), " HERE, sqliteRet);
					ret = e_SC_RESULT_RDB_ACCESSERR;
					stmt = NULL;
					break;
				}
				stmt = NULL;

				// UPDATEに失敗したらINSERTしてみる
				// SQLITE prepare
				sqliteRet = sqlite3_prepare(db, SCC_DAL_SQL_INSERT_DL_BASE_VER, strlen(SCC_DAL_SQL_INSERT_DL_BASE_VER), &stmt, NULL);
				if (SQLITE_OK != sqliteRet) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_prepare error(0x%08x), " HERE, sqliteRet);
					ret = e_SC_RESULT_RDB_ACCESSERR;
					break;
				}

				// stmtの内部バッファを一旦クリア
				sqlite3_reset(stmt);

				// sqlの?の部分に、値を設定
				sqlite3_bind_text(stmt, 1, dlBaseVer[num].kind, strlen(dlBaseVer[num].kind), SQLITE_TRANSIENT);
				sqlite3_bind_text(stmt, 2, dlBaseVer[num].tableName, strlen(dlBaseVer[num].tableName), SQLITE_TRANSIENT);
				sqlite3_bind_int(stmt,  3, dlBaseVer[num].dlFlag);
				sqlite3_bind_int(stmt,  4, dlBaseVer[num].baseVersion);
				sqlite3_bind_text(stmt, 5, "", strlen(""), SQLITE_TRANSIENT);

				// stmtのSQLを実行
				sqliteRet = sqlite3_step(stmt);
				if ((SQLITE_DONE != sqliteRet) || (1 != sqlite3_changes(db))) {
					SCC_LOG_InfoPrint(SC_TAG_CC, (Char*)"sqlite3_step(insert) error(0x%08x), " HERE, sqliteRet);
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sql=%s, " HERE, sql);
					ret = e_SC_RESULT_RDB_ACCESSERR;
					break;
				}
			}

			// SQLITE finalize
			sqliteRet = sqlite3_finalize(stmt);
			if (SQLITE_OK != sqliteRet) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_finalize error(0x%08x), " HERE, sqliteRet);
				ret = e_SC_RESULT_RDB_ACCESSERR;
				stmt = NULL;
				break;
			}
			stmt = NULL;
		}

		if (transaction) {
			if (e_SC_RESULT_SUCCESS == ret) {
				// コミット
				ret = SCC_DAL_Commit(db);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Commit error, " HERE);
					// ロールバック
					if (e_SC_RESULT_SUCCESS != SCC_DAL_Rollback(db)) {
						SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Rollback error, " HERE);
					}
					break;
				}
			} else {
				// ロールバック
				if (e_SC_RESULT_SUCCESS != SCC_DAL_Rollback(db)) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Rollback error, " HERE);
				}
				break;
			}
		}
	} while (0);

	// メモリ解放
	if (NULL != sql) {
		SCC_FREE2(sql);
	}

	if (NULL != stmt) {
		// SQLITE finalize
		sqliteRet = sqlite3_finalize(stmt);
		if (SQLITE_OK != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_finalize error(0x%08x), " HERE, sqliteRet);
		}
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//
/**
 * @brief DOWNLOAD_BASE_VERSIONテーブルのデータ取得(1件)
 * @param[in]  db           SQLITE
 * @param[in]  kind         データ種別
 * @param[in]  tableName    テーブル名
 * @param[out] dlFlag       ダウンロードフラグ
 * @param[out] baseVersion  ベースバージョン
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SCC_DAL_GetDLBaseVersion(DBOBJECT *db,
									 const Char *kind,
									 const Char *tableName,
									 E_SCC_DAL_DLFLAG *dlFlag,
									 INT32 *baseVersion)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	sqliteRet = SQLITE_OK;
	sqlite3_stmt	*stmt = NULL;
	//INT32	size = 0;
	//INT32	num = 0;
	INT32	flg = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == db) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[db], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((NULL == kind) || (EOS == *kind)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[kind], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((NULL == tableName) || (EOS == *tableName)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[tableName], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == dlFlag) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[dlFlag], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == baseVersion) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[baseVersion], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// SQLITE prepare
		sqliteRet = sqlite3_prepare(db, SCC_DAL_SQL_SELECT_DL_BASE_VER, strlen(SCC_DAL_SQL_SELECT_DL_BASE_VER), &stmt, NULL);
		if (SQLITE_OK != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_prepare(select) error(0x%08x), " HERE, sqliteRet);
			ret = e_SC_RESULT_RDB_ACCESSERR;
			break;
		}

		// stmtの内部バッファを一旦クリア
		sqlite3_reset(stmt);

		// sqlの?の部分に、値を設定
		sqlite3_bind_text(stmt, 1, kind, strlen(kind), SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 2, tableName, strlen(tableName), SQLITE_TRANSIENT);

		// SQLITE step
		if (SQLITE_ROW == (sqliteRet = sqlite3_step(stmt))) {
			// DOWNLOAD_FLAG
			flg = sqlite3_column_int(stmt, 0);
			if ((SCC_DAL_DL_FLAG_NODOWNLOAD != flg) &&
				(SCC_DAL_DL_FLAG_DOWNLOAD   != flg) &&
				(SCC_DAL_DL_FLAG_DEL        != flg)) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"DOWNLOAD_FLAG error(DOWNLOAD_FLAG:%d), " HERE, flg);
				ret = e_SC_RESULT_RDB_ACCESSERR;
				break;
			}
			*dlFlag = flg;

			// BASE_VERSION
			*baseVersion = sqlite3_column_int(stmt, 1);
		} else {
			if ((SQLITE_NOTFOUND == sqliteRet) ||
				(SQLITE_DONE == sqliteRet)) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"no data, " HERE);
				ret = e_SC_RESULT_NODATA;
			} else {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_step(stmt) error(0x%08x), " HERE, sqliteRet);
				ret = e_SC_RESULT_RDB_ACCESSERR;
			}
			break;
		}
	} while (0);

	if (NULL != stmt) {
		// SQLITE finalize
		sqliteRet = sqlite3_finalize(stmt);
		if (SQLITE_OK != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_finalize error(0x%08x), " HERE, sqliteRet);
		}
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief DOWNLOAD_AREA_MAPテーブルのデータ更新
 * @param[in]  db           SQLITE
 * @param[in]  dlAreaMap    更新するデータ
 * @param[in]  dlAreaMapNum 更新するデータ数
 * @param[in]  transaction  トランザクションを開始するか否か（true:する、false:しない）
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SCC_DAL_UpdDLAreaMap(DBOBJECT *db,
								 const SCC_DOWNLOADAREAMAP *dlAreaMap,
								 INT32 dlAreaMapNum,
								 Bool transaction)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	sqliteRet = SQLITE_OK;
	sqlite3_stmt	*stmt = NULL;
	Char	*sql = NULL;
	INT32	num = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == db) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[db], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == dlAreaMap) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[dlAreaMap], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// メモリ確保
		sql = (Char*)SCC_MALLOC2(SCC_DAL_SQL_SIZE);
		if (NULL == sql) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC2 error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		if (transaction) {
			// トランザクション開始
			ret = SCC_DAL_Transaction(db);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Transaction error, " HERE);
				break;
			}
		}

		for (num = 0; num < dlAreaMapNum; num++) {
			// SQLITE prepare
			sqliteRet = sqlite3_prepare(db, SCC_DAL_SQL_UPDATE_DL_AREA_MAP, strlen(SCC_DAL_SQL_UPDATE_DL_AREA_MAP), &stmt, NULL);
			if (SQLITE_OK != sqliteRet) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_prepare error(0x%08x), " HERE, sqliteRet);
				ret = e_SC_RESULT_RDB_ACCESSERR;
				break;
			}

			// stmtの内部バッファを一旦クリア
			sqlite3_reset(stmt);

			// sqlの?の部分に、値を設定
			sqlite3_bind_int(stmt,  1, dlAreaMap[num].dlFlag);
			sqlite3_bind_int(stmt,  2, dlAreaMap[num].baseVersion);
			sqlite3_bind_int(stmt,  3, dlAreaMap[num].areaCode);
			sqlite3_bind_text(stmt, 4, dlAreaMap[num].kind, strlen(dlAreaMap[num].kind), SQLITE_TRANSIENT);

			// stmtのSQLを実行
			if ((SQLITE_DONE != sqlite3_step(stmt)) || (1 != sqlite3_changes(db))) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_step(update) error(0x%08x), " HERE, sqliteRet);
				ret = e_SC_RESULT_RDB_ACCESSERR;
				break;
			}

			// SQLITE finalize
			sqliteRet = sqlite3_finalize(stmt);
			if (SQLITE_OK != sqliteRet) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_finalize error(0x%08x), " HERE, sqliteRet);
				ret = e_SC_RESULT_RDB_ACCESSERR;
				stmt = NULL;
				break;
			}
			stmt = NULL;
		}

		if (transaction) {
			if (e_SC_RESULT_SUCCESS == ret) {
				// コミット
				ret = SCC_DAL_Commit(db);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Commit error, " HERE);
					// ロールバック
					if (e_SC_RESULT_SUCCESS != SCC_DAL_Rollback(db)) {
						SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Rollback error, " HERE);
					}
					break;
				}
			} else {
				// ロールバック
				if (e_SC_RESULT_SUCCESS != SCC_DAL_Rollback(db)) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Rollback error, " HERE);
				}
				break;
			}
		}
	} while (0);

	// メモリ解放
	if (NULL != sql) {
		SCC_FREE2(sql);
	}

	if (NULL != stmt) {
		// SQLITE finalize
		sqliteRet = sqlite3_finalize(stmt);
		if (SQLITE_OK != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_finalize error(0x%08x), " HERE, sqliteRet);
		}
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief DOWNLOAD_AREA_MAPテーブルのデータ取得(1件)
 * @param[in]  db           SQLITE
 * @param[in]  areaCode     エリアコード(1～)
 * @param[in]  kind         データ種別(PARCEL/TRAFFIC)
 * @param[out] dlFlag       ダウンロードフラグ
 * @param[out] baseVersion  ベースバージョン
 * @return 処理結果(E_SC_RESULT)
 *         データなしの場合は、e_SC_RESULT_NODATA
 */
E_SC_RESULT SCC_DAL_GetDLAreaMap(DBOBJECT *db,
								 INT32 areaCode,
								 const Char *kind,
								 E_SCC_DAL_DLFLAG *dlFlag,
								 INT32 *baseVersion)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	sqliteRet = SQLITE_OK;
	sqlite3_stmt	*stmt = NULL;
	//INT32	size = 0;
	//INT32	num = 0;
	INT32	flg = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == db) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[db], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == kind) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[kind], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == dlFlag) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[dlFlag], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == baseVersion) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[baseVersion], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// SQLITE prepare
		sqliteRet = sqlite3_prepare(db, SCC_DAL_SQL_SELECT_DL_AREA_MAP, strlen(SCC_DAL_SQL_SELECT_DL_AREA_MAP), &stmt, NULL);
		if (SQLITE_OK != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_prepare(select) error(0x%08x), " HERE, sqliteRet);
			ret = e_SC_RESULT_RDB_ACCESSERR;
			break;
		}

		// stmtの内部バッファを一旦クリア
		sqlite3_reset(stmt);

		// sqlの?の部分に、値を設定
		sqlite3_bind_int(stmt,  1, areaCode);
		sqlite3_bind_text(stmt, 2, kind, strlen(kind), SQLITE_TRANSIENT);

		// SQLITE step
		if (SQLITE_ROW == (sqliteRet = sqlite3_step(stmt))) {
			// DOWNLOAD_FLAG
			flg = sqlite3_column_int(stmt, 0);
			if ((SCC_DAL_DL_FLAG_NODOWNLOAD != flg) &&
				(SCC_DAL_DL_FLAG_DOWNLOAD   != flg) &&
				(SCC_DAL_DL_FLAG_DEL        != flg)) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"DOWNLOAD_FLAG error(DOWNLOAD_FLAG:%d), " HERE, flg);
				ret = e_SC_RESULT_RDB_ACCESSERR;
				break;
			}
			*dlFlag = flg;

			// BASE_VERSION
			*baseVersion = sqlite3_column_int(stmt, 1);
		} else {
			if ((SQLITE_NOTFOUND == sqliteRet) ||
				(SQLITE_DONE == sqliteRet)) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"no data, " HERE);
				ret = e_SC_RESULT_NODATA;
			} else {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_step(stmt) error(0x%08x), " HERE, sqliteRet);
				ret = e_SC_RESULT_RDB_ACCESSERR;
			}
			break;
		}
	} while (0);

	if (NULL != stmt) {
		// SQLITE finalize
		sqliteRet = sqlite3_finalize(stmt);
		if (SQLITE_OK != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_finalize error(0x%08x), " HERE, sqliteRet);
		}
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief DOWNLOAD_AREA_MAPテーブルのデータ取得(全件)
 * @param[in]  db               SQLITE
 * @param[in]  maxDlAreaMapNum  取得するデータ最大数
 * @param[out] dlAreaMap        取得したデータ
 * @param[out] dlAreaMapNum     取得したデータ数
 * @return 処理結果(E_SC_RESULT)
 *         データなしの場合は、e_SC_RESULT_NODATA
 */
E_SC_RESULT SCC_DAL_GetDLAreaMapList(DBOBJECT *db,
									 INT32 maxDlAreaMapNum,
									 SCC_DOWNLOADAREAMAP *dlAreaMap,
									 INT32 *dlAreaMapNum)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	sqliteRet = SQLITE_OK;
	sqlite3_stmt	*stmt = NULL;
	INT32	size = 0;
	INT32	num = 0;
	INT32	flg = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == db) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[db], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (0 >= maxDlAreaMapNum) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[maxDlAreaMapNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == dlAreaMap) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[dlAreaMap], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == dlAreaMapNum) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[dlAreaMapNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 初期化
		memset(dlAreaMap, 0, (sizeof(SCC_DOWNLOADAREAMAP) * maxDlAreaMapNum));

		// SQLITE prepare
		sqliteRet = sqlite3_prepare(db, SCC_DAL_SQL_SELECT_DL_AREA_MAP_LIST, strlen(SCC_DAL_SQL_SELECT_DL_AREA_MAP_LIST), &stmt, NULL);
		if (SQLITE_OK != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_prepare(select) error(0x%08x), " HERE, sqliteRet);
			ret = e_SC_RESULT_RDB_ACCESSERR;
			break;
		}

		// stmtの内部バッファを一旦クリア
		sqlite3_reset(stmt);

		// SQLITE step
		while (SQLITE_ROW == (sqliteRet = sqlite3_step(stmt))) {
			// ID
			dlAreaMap[num].areaCode = sqlite3_column_int(stmt, 0);

			// KIND
			size = sqlite3_column_bytes(stmt, 1);
			memcpy(dlAreaMap[num].kind, (const char *) sqlite3_column_text(stmt, 1), size);
			dlAreaMap[num].kind[size] = EOS;

			// DOWNLOAD_FLAG
			flg = sqlite3_column_int(stmt, 2);
			if ((SCC_DAL_DL_FLAG_NODOWNLOAD != flg) &&
				(SCC_DAL_DL_FLAG_DOWNLOAD   != flg) &&
				(SCC_DAL_DL_FLAG_DEL        != flg)) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"DOWNLOAD_FLAG error(DOWNLOAD_FLAG:%d), " HERE, flg);
				ret = e_SC_RESULT_RDB_ACCESSERR;
				break;
			}
			dlAreaMap[num].dlFlag = flg;

			// BASE_VERSION
			dlAreaMap[num].baseVersion = sqlite3_column_int(stmt, 3);

			num++;
			if (num == maxDlAreaMapNum) {
				// データ取得を終了する
				sqliteRet = SQLITE_DONE;
				break;
			}
		}
		if (SQLITE_DONE != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_step error(0x%08x), " HERE, sqliteRet);
			ret = e_SC_RESULT_RDB_ACCESSERR;
			break;
		}

		// 件数を設定
		*dlAreaMapNum = num;
	} while (0);

	if (NULL != stmt) {
		// SQLITE finalize
		sqliteRet = sqlite3_finalize(stmt);
		if (SQLITE_OK != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_finalize error(0x%08x), " HERE, sqliteRet);
		}
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief DOWNLOAD_SYSテーブルのデータ更新
 * @param[in] db            SQLITE
 * @param[in] dlSys         更新するデータ　※NAMEは、UPDATEしない。INSERTのみ。
 * @param[in] dlSysNum      更新するデータ数
 * @param[in] transaction   トランザクションを開始するか否か（true:する、false:しない）
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SCC_DAL_UpdDLSys(DBOBJECT *db,
							 const SCC_UPDVER_DOWNLOADSYS *dlSys,
							 INT32 dlSysNum,
							 Bool transaction)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	sqliteRet = SQLITE_OK;
	sqlite3_stmt	*stmt = NULL;
	Char	*sql = NULL;
	INT32	num = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == db) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[db], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == dlSys) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[dlSys], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// メモリ確保
		sql = (Char*)SCC_MALLOC2(SCC_DAL_SQL_SIZE);
		if (NULL == sql) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC2 error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		if (transaction) {
			// トランザクション開始
			ret = SCC_DAL_Transaction(db);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Transaction error, " HERE);
				ret = e_SC_RESULT_RDB_ACCESSERR;
				break;
			}
		}

		for (num = 0; num < dlSysNum; num++) {
			// SQLITE prepare
			sqliteRet = sqlite3_prepare(db, SCC_DAL_SQL_UPDATE_DL_SYS, strlen(SCC_DAL_SQL_UPDATE_DL_SYS), &stmt, NULL);
			if (SQLITE_OK != sqliteRet) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_prepare error(0x%08x), " HERE, sqliteRet);
				ret = e_SC_RESULT_RDB_ACCESSERR;
				break;
			}

			// stmtの内部バッファを一旦クリア
			sqlite3_reset(stmt);

			// sqlの?の部分に、値を設定
			sqlite3_bind_int(stmt,  1, dlSys[num].version);
			sqlite3_bind_text(stmt, 2, dlSys[num].kind, strlen(dlSys[num].kind), SQLITE_TRANSIENT);

			// stmtのSQLを実行
			sqliteRet = sqlite3_step(stmt);
			if ((SQLITE_DONE != sqliteRet) || (1 != sqlite3_changes(db))) {
				SCC_LOG_InfoPrint(SC_TAG_CC, (Char*)"sqlite3_step(update) error(0x%08x), " HERE, sqliteRet);
				// SQLITE finalize
				sqliteRet = sqlite3_finalize(stmt);
				if (SQLITE_OK != sqliteRet) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_finalize error(0x%08x), " HERE, sqliteRet);
					ret = e_SC_RESULT_RDB_ACCESSERR;
					stmt = NULL;
					break;
				}
				stmt = NULL;

				// UPDATEに失敗したらINSERTしてみる
				// SQLITE prepare
				sqliteRet = sqlite3_prepare(db, SCC_DAL_SQL_INSERT_DL_SYS, strlen(SCC_DAL_SQL_INSERT_DL_SYS), &stmt, NULL);
				if (SQLITE_OK != sqliteRet) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_prepare error(0x%08x), " HERE, sqliteRet);
					ret = e_SC_RESULT_RDB_ACCESSERR;
					break;
				}

				// stmtの内部バッファを一旦クリア
				sqlite3_reset(stmt);

				// sqlの?の部分に、値を設定
				sqlite3_bind_text(stmt, 1, dlSys[num].kind, strlen(dlSys[num].kind), SQLITE_TRANSIENT);
				sqlite3_bind_text(stmt, 2, dlSys[num].name, strlen(dlSys[num].name), SQLITE_TRANSIENT);
				sqlite3_bind_int(stmt,  3, dlSys[num].version);

				// INSERT
				sqliteRet = sqlite3_step(stmt);
				if ((SQLITE_DONE != sqliteRet) || (1 != sqlite3_changes(db))) {
					SCC_LOG_InfoPrint(SC_TAG_CC, (Char*)"sqlite3_step(insert) error(0x%08x), " HERE, sqliteRet);
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sql=%s, " HERE, sql);
					ret = e_SC_RESULT_RDB_ACCESSERR;
					break;
				}
			}

			// SQLITE finalize
			sqliteRet = sqlite3_finalize(stmt);
			if (SQLITE_OK != sqliteRet) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_finalize error(0x%08x), " HERE, sqliteRet);
				ret = e_SC_RESULT_RDB_ACCESSERR;
				stmt = NULL;
				break;
			}
			stmt = NULL;
		}

		if (transaction) {
			if (e_SC_RESULT_SUCCESS == ret) {
				// コミット
				ret = SCC_DAL_Commit(db);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Commit error, " HERE);
					// ロールバック
					if (e_SC_RESULT_SUCCESS != SCC_DAL_Rollback(db)) {
						SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Rollback error, " HERE);
					}
					break;
				}
			} else {
				// ロールバック
				if (e_SC_RESULT_SUCCESS != SCC_DAL_Rollback(db)) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Rollback error, " HERE);
				}
				break;
			}
		}
	} while (0);

	// メモリ解放
	if (NULL != sql) {
		SCC_FREE2(sql);
	}

	if (NULL != stmt) {
		// SQLITE finalize
		sqliteRet = sqlite3_finalize(stmt);
		if (SQLITE_OK != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_finalize error(0x%08x), " HERE, sqliteRet);
		}
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief DOWNLOAD_SYSテーブルのデータ取得(1件)
 * @param[in]  db           SQLITE
 * @param[in]  kind         データ種別(FONT/TTS/OTHER)
 * @param[out] version      バージョン
 * @return 処理結果(E_SC_RESULT)
 *         データなしの場合は、e_SC_RESULT_NODATA
 */
E_SC_RESULT SCC_DAL_GetDLSys(DBOBJECT *db, const Char *kind, INT32 *version)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	sqliteRet = SQLITE_OK;
	sqlite3_stmt	*stmt = NULL;
	//INT32	size = 0;
	//INT32	num = 0;
	//INT32	flg = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == db) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[db], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == kind) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[kind], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == version) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[version], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// SQLITE prepare
		sqliteRet = sqlite3_prepare(db, SCC_DAL_SQL_SELECT_DL_SYS, strlen(SCC_DAL_SQL_SELECT_DL_SYS), &stmt, NULL);
		if (SQLITE_OK != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_prepare(select) error(0x%08x), " HERE, sqliteRet);
			ret = e_SC_RESULT_RDB_ACCESSERR;
			break;
		}

		// stmtの内部バッファを一旦クリア
		sqlite3_reset(stmt);

		// sqlの?の部分に、値を設定
		sqlite3_bind_text(stmt, 1, kind, strlen(kind), SQLITE_TRANSIENT);

		// SQLITE step
		if (SQLITE_ROW == (sqliteRet = sqlite3_step(stmt))) {
			// VERSION
			*version = sqlite3_column_int(stmt, 0);
		} else {
			if ((SQLITE_NOTFOUND == sqliteRet) ||
				(SQLITE_DONE == sqliteRet)) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"no data, " HERE);
				ret = e_SC_RESULT_NODATA;
			} else {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_step(stmt) error(0x%08x), " HERE, sqliteRet);
				ret = e_SC_RESULT_RDB_ACCESSERR;
			}
			break;
		}
	} while (0);

	if (NULL != stmt) {
		// SQLITE finalize
		sqliteRet = sqlite3_finalize(stmt);
		if (SQLITE_OK != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_finalize error(0x%08x), " HERE, sqliteRet);
		}
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief DOWNLOAD_SYSテーブルのデータ取得(全件)
 * @param[in]  db           SQLITE
 * @param[in]  maxDlSysNum  取得するデータ最大数
 * @param[out] dlSys        取得したデータ
 * @param[out] dlSysNum     取得したデータ数
 * @return 処理結果(E_SC_RESULT)
 *         データなしの場合は、e_SC_RESULT_NODATA
 */
E_SC_RESULT SCC_DAL_GetDLSysList(DBOBJECT *db,
								 INT32 maxDlSysNum,
								 SCC_UPDVER_DOWNLOADSYS *dlSys,
								 INT32 *dlSysNum)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	sqliteRet = SQLITE_OK;
	sqlite3_stmt	*stmt = NULL;
	INT32	size = 0;
	INT32	num = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == db) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[db], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (0 >= maxDlSysNum) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[maxDlSysNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == dlSys) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[dlSys], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == dlSysNum) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[dlSysNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 初期化
		memset(dlSys, 0, (sizeof(SCC_UPDVER_DOWNLOADSYS) * maxDlSysNum));

		// SQLITE prepare
		sqliteRet = sqlite3_prepare(db, SCC_DAL_SQL_SELECT_DL_SYS_LIST, strlen(SCC_DAL_SQL_SELECT_DL_SYS_LIST), &stmt, NULL);
		if (SQLITE_OK != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_prepare(select) error(0x%08x), " HERE, sqliteRet);
			ret = e_SC_RESULT_RDB_ACCESSERR;
			break;
		}

		// stmtの内部バッファを一旦クリア
		sqlite3_reset(stmt);

		// SQLITE step
		while (SQLITE_ROW == (sqliteRet = sqlite3_step(stmt))) {
			// KIND
			size = sqlite3_column_bytes(stmt, 0);
			memcpy(dlSys[num].kind, (const char *) sqlite3_column_text(stmt, 0), size);
			dlSys[num].kind[size] = EOS;

			// VERSION
			dlSys[num].version = sqlite3_column_int(stmt, 1);

			num++;
			if (num == maxDlSysNum) {
				// データ取得を終了する
				sqliteRet = SQLITE_DONE;
				break;
			}
		}
		if (SQLITE_DONE != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_step error(0x%08x), " HERE, sqliteRet);
			ret = e_SC_RESULT_RDB_ACCESSERR;
			break;
		}

		// 件数を設定
		*dlSysNum = num;
	} while (0);

	if (NULL != stmt) {
		// SQLITE finalize
		sqliteRet = sqlite3_finalize(stmt);
		if (SQLITE_OK != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_finalize error(0x%08x), " HERE, sqliteRet);
		}
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief DOWNLOAD_CUSTOMテーブルのデータ更新
 * @param[in] db            SQLITE
 * @param[in] dlCustom      更新するデータ　※NAMEは、UPDATEしない。INSERTのみ。
 * @param[in] dlCustomNum   更新するデータ数
 * @param[in] transaction   トランザクションを開始するか否か（true:する、false:しない）
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SCC_DAL_UpdDLCustom(DBOBJECT *db,
								const SCC_UPDVER_DOWNLOADCUSTOM *dlCustom,
								INT32 dlCustomNum,
								Bool transaction)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	sqliteRet = SQLITE_OK;
	sqlite3_stmt	*stmt = NULL;
	Char	*sql = NULL;
	INT32	num = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == db) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[db], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == dlCustom) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[dlCustom], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// メモリ確保
		sql = (Char*)SCC_MALLOC2(SCC_DAL_SQL_SIZE);
		if (NULL == sql) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC2 error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		if (transaction) {
			// トランザクション開始
			ret = SCC_DAL_Transaction(db);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Transaction error, " HERE);
				break;
			}
		}

		for (num = 0; num < dlCustomNum; num++) {
			// SQLITE prepare
			sqliteRet = sqlite3_prepare(db, SCC_DAL_SQL_UPDATE_DL_CUSTOM, strlen(SCC_DAL_SQL_UPDATE_DL_CUSTOM), &stmt, NULL);
			if (SQLITE_OK != sqliteRet) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_prepare error(0x%08x), " HERE, sqliteRet);
				ret = e_SC_RESULT_RDB_ACCESSERR;
				break;
			}

			// stmtの内部バッファを一旦クリア
			sqlite3_reset(stmt);

			// sqlの?の部分に、値を設定
			sqlite3_bind_text(stmt, 1, dlCustom[num].version, strlen(dlCustom[num].version), SQLITE_TRANSIENT);
			sqlite3_bind_text(stmt, 2, dlCustom[num].kind, strlen(dlCustom[num].kind), SQLITE_TRANSIENT);
			sqlite3_bind_int(stmt,  3, dlCustom[num].setnumber);

			// stmtのSQLを実行
			sqliteRet = sqlite3_step(stmt);
			if ((SQLITE_DONE != sqliteRet) || (1 != sqlite3_changes(db))) {
				SCC_LOG_InfoPrint(SC_TAG_CC, (Char*)"sqlite3_step(update) error(0x%08x), " HERE, sqliteRet);
				// SQLITE finalize
				sqliteRet = sqlite3_finalize(stmt);
				if (SQLITE_OK != sqliteRet) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_finalize error(0x%08x), " HERE, sqliteRet);
					ret = e_SC_RESULT_RDB_ACCESSERR;
					stmt = NULL;
					break;
				}
				stmt = NULL;

				// UPDATEに失敗したらINSERTしてみる
				// SQLITE prepare
				sqliteRet = sqlite3_prepare(db, SCC_DAL_SQL_INSERT_DL_CUSTOM, strlen(SCC_DAL_SQL_INSERT_DL_CUSTOM), &stmt, NULL);
				if (SQLITE_OK != sqliteRet) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_prepare error(0x%08x), " HERE, sqliteRet);
					ret = e_SC_RESULT_RDB_ACCESSERR;
					break;
				}

				// stmtの内部バッファを一旦クリア
				sqlite3_reset(stmt);

				// sqlの?の部分に、値を設定
				sqlite3_bind_text(stmt, 1, dlCustom[num].kind, strlen(dlCustom[num].kind), SQLITE_TRANSIENT);
				sqlite3_bind_int(stmt,  2, dlCustom[num].setnumber);
				sqlite3_bind_text(stmt, 3, dlCustom[num].name, strlen(dlCustom[num].name), SQLITE_TRANSIENT);
				sqlite3_bind_text(stmt, 4, dlCustom[num].version, strlen(dlCustom[num].version), SQLITE_TRANSIENT);

				// stmtのSQLを実行
				sqliteRet = sqlite3_step(stmt);
				if ((SQLITE_DONE != sqliteRet) || (1 != sqlite3_changes(db))) {
					SCC_LOG_InfoPrint(SC_TAG_CC, (Char*)"sqlite3_step(insert) error(0x%08x), " HERE, sqliteRet);
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sql=%s, " HERE, sql);
					ret = e_SC_RESULT_RDB_ACCESSERR;
					break;
				}
			}

			// SQLITE finalize
			sqliteRet = sqlite3_finalize(stmt);
			if (SQLITE_OK != sqliteRet) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_finalize error(0x%08x), " HERE, sqliteRet);
				ret = e_SC_RESULT_RDB_ACCESSERR;
				stmt = NULL;
				break;
			}
			stmt = NULL;
		}

		if (transaction) {
			if (e_SC_RESULT_SUCCESS == ret) {
				// コミット
				ret = SCC_DAL_Commit(db);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Commit error, " HERE);
					// ロールバック
					if (e_SC_RESULT_SUCCESS != SCC_DAL_Rollback(db)) {
						SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Rollback error, " HERE);
					}
					break;
				}
			} else {
				// ロールバック
				if (e_SC_RESULT_SUCCESS != SCC_DAL_Rollback(db)) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Rollback error, " HERE);
				}
				break;
			}
		}
	} while (0);

	// メモリ解放
	if (NULL != sql) {
		SCC_FREE2(sql);
	}

	if (NULL != stmt) {
		// SQLITE finalize
		sqliteRet = sqlite3_finalize(stmt);
		if (SQLITE_OK != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_finalize error(0x%08x), " HERE, sqliteRet);
		}
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief DOWNLOAD_CUSTOMテーブルのデータ取得(1件)
 * @param[in]  db           SQLITE
 * @param[in]  kind         データ種別(NAVI_SYMBOL/NAVI_POI/NAVI_MAP/NAVI_SKIN/NAVI_ICON/NAVI_CAR/NAVI_RC/NAVI_RG)
 * @param[in]  setnumber    セット番号
 * @param[out] version      バージョン
 * @return 処理結果(E_SC_RESULT)
 *         データなしの場合は、e_SC_RESULT_NODATA
 */
E_SC_RESULT SCC_DAL_GetDLCustom(DBOBJECT *db, const Char *kind, const Char *setnumber, INT32 *version)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	sqliteRet = SQLITE_OK;
	sqlite3_stmt	*stmt = NULL;
	//INT32	size = 0;
	//INT32	num = 0;
	//INT32	flg = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == db) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[db], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == kind) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[kind], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == setnumber) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[setnumber], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == version) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[version], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// SQLITE prepare
		sqliteRet = sqlite3_prepare(db, SCC_DAL_SQL_SELECT_DL_CUSTOM, strlen(SCC_DAL_SQL_SELECT_DL_CUSTOM), &stmt, NULL);
		if (SQLITE_OK != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_prepare(select) error(0x%08x), " HERE, sqliteRet);
			ret = e_SC_RESULT_RDB_ACCESSERR;
			break;
		}

		// stmtの内部バッファを一旦クリア
		sqlite3_reset(stmt);

		// sqlの?の部分に、値を設定
		sqlite3_bind_text(stmt, 1, kind, strlen(kind), SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 2, kind, strlen(setnumber), SQLITE_TRANSIENT);

		// SQLITE step
		if (SQLITE_ROW == (sqliteRet = sqlite3_step(stmt))) {
			// VERSION
			*version = sqlite3_column_int(stmt, 0);
		} else {
			if ((SQLITE_NOTFOUND == sqliteRet) ||
				(SQLITE_DONE == sqliteRet)) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"no data, " HERE);
				ret = e_SC_RESULT_NODATA;
			} else {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_step(stmt) error(0x%08x), " HERE, sqliteRet);
				ret = e_SC_RESULT_RDB_ACCESSERR;
			}
			break;
		}
	} while (0);

	if (NULL != stmt) {
		// SQLITE finalize
		sqliteRet = sqlite3_finalize(stmt);
		if (SQLITE_OK != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_finalize error(0x%08x), " HERE, sqliteRet);
		}
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief DOWNLOAD_CUSTOMテーブルのデータ取得(全件)
 * @param[in]  db               SQLITE
 * @param[in]  maxDlCustomNum   取得するデータ最大数
 * @param[out] dlCustom         取得したデータ
 * @param[out] dlCustomNum      取得したデータ数
 * @return 処理結果(E_SC_RESULT)
 *         データなしの場合は、e_SC_RESULT_NODATA
 */
E_SC_RESULT SCC_DAL_GetDLCustomList(DBOBJECT *db,
									INT32 maxDlCustomNum,
									SCC_UPDVER_DOWNLOADCUSTOM *dlCustom,
									INT32 *dlCustomNum)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	sqliteRet = SQLITE_OK;
	sqlite3_stmt	*stmt = NULL;
	INT32	size = 0;
	INT32	num = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == db) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[db], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (0 >= maxDlCustomNum) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[maxDlCustomNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == dlCustom) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[dlCustom], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == dlCustomNum) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[dlCustomNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 初期化
		memset(dlCustom, 0, (sizeof(SCC_UPDVER_DOWNLOADCUSTOM) * maxDlCustomNum));

		// SQLITE prepare
		sqliteRet = sqlite3_prepare(db, SCC_DAL_SQL_SELECT_DL_CUSTOM_LIST, strlen(SCC_DAL_SQL_SELECT_DL_CUSTOM_LIST), &stmt, NULL);
		if (SQLITE_OK != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_prepare(select) error(0x%08x), " HERE, sqliteRet);
			ret = e_SC_RESULT_RDB_ACCESSERR;
			break;
		}

		// stmtの内部バッファを一旦クリア
		sqlite3_reset(stmt);

		// SQLITE step
		while (SQLITE_ROW == (sqliteRet = sqlite3_step(stmt))) {
			// KIND
			size = sqlite3_column_bytes(stmt, 0);
			memcpy(dlCustom[num].kind, (const char *) sqlite3_column_text(stmt, 0), size);
			dlCustom[num].kind[size] = EOS;

			// SETNUMBER
			dlCustom[num].setnumber = sqlite3_column_int(stmt, 1);

			// VERSION
			size = sqlite3_column_bytes(stmt, 2);
			memcpy(dlCustom[num].version, (const char *) sqlite3_column_text(stmt, 2), size);
			dlCustom[num].version[size] = EOS;

			num++;
			if (num == maxDlCustomNum) {
				// データ取得を終了する
				sqliteRet = SQLITE_DONE;
				break;
			}
		}
		if (SQLITE_DONE != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_step error(0x%08x), " HERE, sqliteRet);
			ret = e_SC_RESULT_RDB_ACCESSERR;
			break;
		}

		// 件数を設定
		*dlCustomNum = num;
	} while (0);

	if (NULL != stmt) {
		// SQLITE finalize
		sqliteRet = sqlite3_finalize(stmt);
		if (SQLITE_OK != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_finalize error(0x%08x), " HERE, sqliteRet);
		}
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief AREA_CLSテーブル更新
 * @param[in]  db           SQLITE
 * @param[in]  filePath     CSVファイルパス
 * @param[in]  transaction  トランザクションを開始するか否か（true:する、false:しない）
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SCC_DAL_UpdateAreaCls(DBOBJECT *db, const Char *filePath, Bool transaction, const SMPROGRESSCBFNC *callbackFnc)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32		sqliteRet = SQLITE_OK;
	sqlite3_stmt	*stmt = NULL;
	Char	*conmaData = NULL;
	Char	*sql = NULL;
	FILE	*fp = NULL;
	Char	*clmList[SCC_DAL_TABLE_CLM_NUM_AREA_CLS] = {};
	INT32	num = 0;
	INT32	updClmNum = 0;
	INT32	baseVersion = 0;
	//INT32	idx = 0;
	Char	keyList[SCC_DAL_TABLE_CLM_NUM_AREA_CLS] = {
					'1', '1', '1', '1', '1', '1', '1', '0', '1', '0', '0', '0', '0', '0', '1', '0'
	};

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == db) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[db], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((NULL == filePath) || (EOS == *filePath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[filePath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// メモリ確保
		conmaData = (Char*)SCC_MALLOC2(SCC_DAL_CONMADATA_SIZE);
		if (NULL == conmaData) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC2 error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		sql = (Char*)SCC_MALLOC2(SCC_DAL_SQL_SIZE);
		if (NULL == sql) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC2 error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// ファイルオープン
		fp = fopen(filePath, "r");
		if (NULL == fp) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"fopen error(%s), " HERE, filePath);
			ret = e_SC_RESULT_FILE_OPENERR;
			break;
		}

		if (transaction) {
			// トランザクション開始
			ret = SCC_DAL_Transaction(db);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Transaction error, " HERE);
				break;
			}
		}

		while (0 == feof(fp)) {
			// キャンセルチェック
			if ((NULL != callbackFnc) && (NULL != callbackFnc->cancel) && (callbackFnc->cancel())) {
				SCC_LOG_WarnPrint(SC_TAG_CC, (Char*)"cancel, " HERE);
				ret = e_SC_RESULT_CANCEL;
				break;
			}
			// CSVファイルからカラムリスト取得
			ret = SCC_DAL_GetCsvClmList(keyList, SCC_DAL_TABLE_CLM_NUM_AREA_CLS, conmaData, clmList, &fp);
			if (e_SC_RESULT_NODATA == ret) {
				// EOF
				ret = e_SC_RESULT_SUCCESS;
				break;
			} else if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_GetCsvClmList error, " HERE);
				break;
			}

			// バージョン取得
			ret = SCC_DAL_GetDataVersion(db,
										 SCC_DAL_SQL_SELECT_AREA_CLS_VERSION,
										 &keyList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_CLS6_CODE],
										 (const Char**)&clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_CLS6_CODE],
										 (SCC_DAL_TABLE_CLM_NUM_AREA_CLS - 1),
										 &baseVersion,
										 1);
			if ((e_SC_RESULT_NODATA != ret) && (e_SC_RESULT_SUCCESS != ret)) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_GetDataVersion error, " HERE);
				break;
			}

			// DELETE
			if (SCC_DAL_DELETE_FLAG == *clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_DELETE_FLAG]) {
				if (e_SC_RESULT_NODATA == ret) {
					// 対象データが存在しない
					ret = e_SC_RESULT_SUCCESS;
					// 進捗通知
					if ((NULL != callbackFnc) && (NULL != callbackFnc->progress)) {
						callbackFnc->progress(1);
					}
					continue;
				}

				// SQLITE prepare
				sqliteRet = sqlite3_prepare(db, SCC_DAL_SQL_DELETE_AREA_CLS, strlen(SCC_DAL_SQL_DELETE_AREA_CLS), &stmt, NULL);
				if (SQLITE_OK != sqliteRet) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_prepare error(0x%08x), " HERE, sqliteRet);
					ret = e_SC_RESULT_RDB_ACCESSERR;
					break;
				}

				// stmtの内部バッファを一旦クリア
				sqlite3_reset(stmt);

				// sqlの?の部分に、値を設定
				sqlite3_bind_text(stmt, 1, clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_CLS6_CODE], strlen(clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_CLS6_CODE]), SQLITE_TRANSIENT);
				sqlite3_bind_text(stmt, 2, clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_CLS5_CODE], strlen(clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_CLS5_CODE]), SQLITE_TRANSIENT);
				sqlite3_bind_text(stmt, 3, clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_CLS4_CODE], strlen(clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_CLS4_CODE]), SQLITE_TRANSIENT);
				sqlite3_bind_text(stmt, 4, clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_CLS3_CODE], strlen(clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_CLS3_CODE]), SQLITE_TRANSIENT);
				sqlite3_bind_text(stmt, 5, clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_CLS2_CODE], strlen(clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_CLS2_CODE]), SQLITE_TRANSIENT);
				sqlite3_bind_text(stmt, 6, clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_CLS1_CODE], strlen(clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_CLS1_CODE]), SQLITE_TRANSIENT);
				sqlite3_bind_text(stmt, 7, clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_LAN_CD], strlen(clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_LAN_CD]), SQLITE_TRANSIENT);
				sqlite3_bind_text(stmt, 8, clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_ID], strlen(clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_ID]), SQLITE_TRANSIENT);

				// stmtのSQLを実行
				sqliteRet = sqlite3_step(stmt);
				if ((SQLITE_DONE != sqliteRet) || (1 != sqlite3_changes(db))) {
					SCC_LOG_InfoPrint(SC_TAG_CC, (Char*)"sqlite3_step(update) error(0x%08x), " HERE, sqliteRet);
					ret = e_SC_RESULT_RDB_ACCESSERR;
					// SQLITE finalize
					sqliteRet = sqlite3_finalize(stmt);
					if (SQLITE_OK != sqliteRet) {
						SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_finalize error(0x%08x), " HERE, sqliteRet);
						stmt = NULL;
						break;
					}
					stmt = NULL;
				}

				// SQLITE finalize
				sqliteRet = sqlite3_finalize(stmt);
				if (SQLITE_OK != sqliteRet) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_finalize error(0x%08x), " HERE, sqliteRet);
					ret = e_SC_RESULT_RDB_ACCESSERR;
					stmt = NULL;
					break;
				}
				stmt = NULL;
			} else {
				updClmNum = 0;
				if (ret == e_SC_RESULT_NODATA) {
					ret = e_SC_RESULT_SUCCESS;
					// INSERT SQL文生成
					strcpy(sql, SCC_DAL_SQL_INSERT_AREA_CLS);
					for (num = 1; num < SCC_DAL_TABLE_CLM_NUM_AREA_CLS; num++) {
						if (0 < updClmNum) {
							strcat(sql, ",");
						}

						if ((NULL != clmList[num]) && (EOS != *clmList[num]) && (0 != strcmp(clmList[num], "''"))) {
							sprintf((char*)&sql[strlen((char*)sql)], "%s", clmList[num]);
						} else {
							if (SCC_DAL_TABLE_CLM_IDX_AREA_CLS_NAME == num) {
								sprintf((char*)&sql[strlen((char*)sql)], "''");
							} else {
								sprintf((char*)&sql[strlen((char*)sql)], "null");
							}
						}
						updClmNum++;
					}
					sprintf((char*)&sql[strlen((char*)sql)], ")");

					// INSERT
					sqliteRet = sqlite3_exec(db, sql, NULL, NULL, NULL);
					if ((SQLITE_OK != sqliteRet) || (1 != sqlite3_changes(db))) {
						SCC_LOG_InfoPrint(SC_TAG_CC, (Char*)"sqlite3_exec(insert) error(0x%08x), " HERE, sqliteRet);
						SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sql=%s, " HERE, sql);
						ret = e_SC_RESULT_RDB_ACCESSERR;
						break;
					}
				} else {
					// UPDATE
					// AREA_BASE_VERSION
					if ((NULL != clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_BASE_VERSION]) &&
						(EOS  != *clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_BASE_VERSION]) &&
						(0    != strcmp(clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_BASE_VERSION], "''"))) {
						// ベースバージョンチェック
						if (atoi(clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_BASE_VERSION]) <= baseVersion) {
							// 更新なし
							// 進捗通知
							if ((NULL != callbackFnc) && (NULL != callbackFnc->progress)) {
								callbackFnc->progress(1);
							}
							continue;
						}
					} else {
						// 進捗通知
						if ((NULL != callbackFnc) && (NULL != callbackFnc->progress)) {
							callbackFnc->progress(1);
						}
						continue;
					}

					// UPDATE SQL文生成
					strcpy((char*)sql, "UPDATE AREA_CLS SET ");
					// REGION_CODE
					if ((NULL != clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_REGION_CODE]) && (EOS != *clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_REGION_CODE])) {
						if (0 != strcmp(clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_REGION_CODE], "''")) {
							sprintf((char*)&sql[strlen((char*)sql)], "%s=%s", areaClsTablClm[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_REGION_CODE], clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_REGION_CODE]);
						} else {
							sprintf((char*)&sql[strlen((char*)sql)], "%s=null", areaClsTablClm[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_REGION_CODE]);
						}
						updClmNum++;
					}
					// AREA_CLS_NAME
					if ((NULL != clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_NAME]) && (EOS != *clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_NAME])) {
						if (0 < updClmNum) {
							strcat(sql, ",");
						}
						if (0 != strcmp(clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_NAME], "''")) {
							sprintf((char*)&sql[strlen((char*)sql)], "%s='%s'", areaClsTablClm[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_NAME], clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_NAME]);
						} else {
							sprintf((char*)&sql[strlen((char*)sql)], "%s=''", areaClsTablClm[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_NAME]);
						}
						updClmNum++;
					}
					// AREA_CLS_YOMI
					if ((NULL != clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_YOMI]) && (EOS != *clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_YOMI])) {
						if (0 < updClmNum) {
							strcat(sql, ",");
						}
						if (0 != strcmp(clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_YOMI], "''")) {
							sprintf((char*)&sql[strlen((char*)sql)], "%s='%s'", areaClsTablClm[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_YOMI], clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_YOMI]);
						} else {
							sprintf((char*)&sql[strlen((char*)sql)], "%s=null", areaClsTablClm[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_YOMI]);
						}
						updClmNum++;
					}
					// PRIORITY_FLAG
					if ((NULL != clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_PRIORITY_FLAG]) && (EOS != *clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_PRIORITY_FLAG])) {
						if (0 < updClmNum) {
							strcat(sql, ",");
						}
						sprintf((char*)&sql[strlen((char*)sql)], "%s=%s", areaClsTablClm[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_PRIORITY_FLAG], clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_PRIORITY_FLAG]);
						updClmNum++;
					}

					// POSI_LATITUDE
					if ((NULL != clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_POSI_LATITUDE]) && (EOS != *clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_POSI_LATITUDE])) {
						if (0 < updClmNum) {
							strcat(sql, ",");
						}
						if (0 != strcmp(clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_POSI_LATITUDE], "''")) {
							sprintf((char*)&sql[strlen((char*)sql)], "%s=%s", areaClsTablClm[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_POSI_LATITUDE], clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_POSI_LATITUDE]);
						} else {
							sprintf((char*)&sql[strlen((char*)sql)], "%s=null", areaClsTablClm[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_POSI_LATITUDE]);
						}
						updClmNum++;
					}
					// POSI_LONGITUDE
					if ((NULL != clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_POSI_LONGITUDE]) && (EOS != *clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_POSI_LONGITUDE])) {
						if (0 < updClmNum) {
							strcat(sql, ",");
						}
						if (0 != strcmp(clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_POSI_LONGITUDE], "''")) {
							sprintf((char*)&sql[strlen((char*)sql)], "%s=%s", areaClsTablClm[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_POSI_LONGITUDE], clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_POSI_LONGITUDE]);
						} else {
							sprintf((char*)&sql[strlen((char*)sql)], "%s=null", areaClsTablClm[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_POSI_LONGITUDE]);
						}
						updClmNum++;
					}

					// AREA_BASE_VERSION
					if ((NULL != clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_BASE_VERSION]) && (EOS != *clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_BASE_VERSION])) {
						if (0 < updClmNum) {
							strcat(sql, ",");
						}
						sprintf((char*)&sql[strlen((char*)sql)], "%s=%s", areaClsTablClm[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_BASE_VERSION], clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_BASE_VERSION]);
						updClmNum++;
					}

					if (0 == updClmNum) {
						// 更新なし
						// 進捗通知
						if ((NULL != callbackFnc) && (NULL != callbackFnc->progress)) {
							callbackFnc->progress(1);
						}
						continue;
					}
					sprintf((char*)&sql[strlen((char*)sql)],
							" WHERE %s=%s AND %s=%s AND %s=%s AND %s=%s AND %s=%s AND %s=%s AND %s=%s AND %s=%s",
							areaClsTablClm[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_CLS6_CODE], clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_CLS6_CODE],
							areaClsTablClm[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_CLS5_CODE], clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_CLS5_CODE],
							areaClsTablClm[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_CLS4_CODE], clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_CLS4_CODE],
							areaClsTablClm[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_CLS3_CODE], clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_CLS3_CODE],
							areaClsTablClm[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_CLS2_CODE], clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_CLS2_CODE],
							areaClsTablClm[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_CLS1_CODE], clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_CLS1_CODE],
							areaClsTablClm[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_LAN_CD], clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_LAN_CD],
							areaClsTablClm[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_ID], clmList[SCC_DAL_TABLE_CLM_IDX_AREA_CLS_ID]);

					// UPDATE
					sqliteRet = sqlite3_exec(db, sql, NULL, NULL, NULL);
					if ((SQLITE_OK != sqliteRet) || (1 != sqlite3_changes(db))) {
						SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_exec(update) error(0x%08x), " HERE, sqliteRet);
						SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sql=%s, " HERE, sql);
						ret = e_SC_RESULT_RDB_ACCESSERR;
						break;
					}
				}
			}

			// 進捗通知
			if ((NULL != callbackFnc) && (NULL != callbackFnc->progress)) {
				callbackFnc->progress(1);
			}
		}
		if (transaction) {
			if (e_SC_RESULT_SUCCESS == ret) {
				// コミット
				ret = SCC_DAL_Commit(db);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Commit error, " HERE);
					// ロールバック
					if (e_SC_RESULT_SUCCESS != SCC_DAL_Rollback(db)) {
						SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Rollback error, " HERE);
					}
					break;
				}
			} else {
				// ロールバック
				if (e_SC_RESULT_SUCCESS != SCC_DAL_Rollback(db)) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Rollback error, " HERE);
				}
				break;
			}
		}
	} while (0);

	// メモリ解放
	if (NULL != conmaData) {
		SCC_FREE2(conmaData);
	}
	if (NULL != sql) {
		SCC_FREE2(sql);
	}
	// ファイルクローズ
	if (NULL != fp) {
		fclose(fp);
	}
	if (NULL != stmt) {
		// SQLITE finalize
		sqliteRet = sqlite3_finalize(stmt);
		if (SQLITE_OK != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_finalize error(0x%08x), " HERE, sqliteRet);
		}
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief DBからベースバージョンを取得する
 * @param[in]  db           DBオブジェクト
 * @param[in]  sql          SQL文
 * @param[in]  clmKeyList   カラムキーリスト
 * @param[in]  clmList      カラムリスト
 * @param[in]  clmListNum   カラムリスト数
 * @param[out] versionList  バージョンリスト
 * @param[in]  versionListNum   バージョンリスト数
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SCC_DAL_GetDataVersion(DBOBJECT *db,
								   const Char *sql,
								   const Char *clmKeyList,
								   const Char **clmList,
								   INT32 clmListNum,
								   INT32 *versionList,
								   INT32 versionListNum)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32		sqliteRet = SQLITE_OK;
	sqlite3_stmt	*stmt = NULL;
	INT32		num = 0;
	INT32		idx = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// SQLITE prepare
		sqliteRet = sqlite3_prepare(db, sql, strlen(sql), &stmt, NULL);
		if (SQLITE_OK != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_prepare(select) error(0x%08x), " HERE, sqliteRet);
			ret = e_SC_RESULT_RDB_ACCESSERR;
			break;
		}

		// stmtの内部バッファを一旦クリア
		sqlite3_reset(stmt);

		// sqlの?の部分に、値を設定
		for (num = 0, idx = 1; num < clmListNum; num++) {
			if ('1' == clmKeyList[num]) {
				if (NULL == clmList[num]) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"clmList[%d] is null, " HERE, num);
					ret = e_SC_RESULT_MAP_DATA_ERR;
					break;
				}
				sqlite3_bind_text(stmt, idx, clmList[num], strlen(clmList[num]), SQLITE_TRANSIENT);
				idx++;
			}
		}
		if (e_SC_RESULT_SUCCESS != ret) {
			break;
		}

		// SQLITE step
		if (SQLITE_ROW == (sqliteRet = sqlite3_step(stmt))) {
			for (num = 0; num < versionListNum; num++) {
				versionList[num] = sqlite3_column_int(stmt, num);
			}
		} else {
			if ((SQLITE_NOTFOUND == sqliteRet) || (SQLITE_DONE == sqliteRet)) {
				for (num = 0; num < versionListNum; num++) {
					versionList[num] = 0;
				}
				ret = e_SC_RESULT_NODATA;
			} else {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_step(stmt) error(0x%08x), " HERE, sqliteRet);
				ret = e_SC_RESULT_RDB_ACCESSERR;
				break;
			}
		}
	} while (0);

	if (NULL != stmt) {
		// SQLITE finalize
		sqliteRet = sqlite3_finalize(stmt);
		if (SQLITE_OK != sqliteRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"sqlite3_finalize error(0x%08x), " HERE, sqliteRet);
			if (e_SC_RESULT_SUCCESS == ret) {
				ret = e_SC_RESULT_RDB_ACCESSERR;
			}
		}
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}
