/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#include "SMCComInternal.h"

#define SCC_UPDDATA_URI_MAX_LEN				(2048 + 1)
#define SCC_UPDDATA_RECIVE_BUFFER_SIZE		2049

static E_SC_RESULT CC_CheckUpdate_GetRegionInfo(SMCAL *smcal, T_CC_CMN_SMS_API_PRM *parm, const Char *dlTempDirPath, const SMPROGRESSCBFNC *callbackFnc, Char *filePath, Char *saveFilePath, SCC_REGIONINFO *rgnInfo);
static E_SC_RESULT CC_CheckUpdate_GetMapUpdateInfo(SMCAL *smcal, T_CC_CMN_SMS_API_PRM *parm, const Char *dlTempDirPath, const SMPROGRESSCBFNC *callbackFnc, const SCC_REGIONINFO *rgnInfo, Char *filePath, Char *saveFilePath, SCC_UPDATEDATA *upddata);
static E_SC_RESULT CC_CheckUpdate_GetLcMapUpdateInfo(const Char *dbFilePath, E_SCC_DAL_DLFLAG *dlFlg, INT32 *baseVersion);
static E_SC_RESULT CC_CheckUpdate_CheckVersion(const SCC_UPDATEDATA *upddata, const E_SCC_DAL_DLFLAG *dlFlg, const INT32 *baseVersion, Bool hasMap, SMCHECKUPDINFO *chkUpdInf, Bool errComeback);

/**
 * @brief 更新データ有無チェック
 * @param[in]     smcal         SMCAL
 * @param[in]     parm          APIパラメータ
 * @param[in/out] chkUpdInf     データ更新情報
 * @param[in]     dlTempDirPath ダウンロードテンポラリフォルダ
 * @param[in]     callbackFnc   コールバック関数情報
 * @param[in]     errComeback   異常復帰
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_RESULT CC_CheckUpdate(SMCAL *smcal,
						   T_CC_CMN_SMS_API_PRM *parm,
						   SMCHECKUPDINFO *chkUpdInf,
						   const Char *dlTempDirPath,
						   const SMMAPDLCBFNC *dlCBFnc,
						   Bool errComeback)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SMRGNSETTING rgnSet = {};
	Char	*filePath = NULL;
	Char	*saveFilePath = NULL;
	DBOBJECT	*db = NULL;
	SCC_REGIONINFO	*rgnInfo = NULL;
	SCC_UPDATEDATA	*upddata = NULL;
	struct	stat st = {};
	Bool	mapDBErr = false;
	E_SCC_DAL_DLFLAG	dlFlg[2] = {SCC_DAL_DL_FLAG_NODOWNLOAD};
	INT32	baseVersion[2] = {};
	SMPROGRESSCBFNC	callbackFnc = {};
	//Bool	versionUnmatch = false;
	INT32	num = 0;
	Bool	isErrComeback = errComeback;
	//UChar	md5[CC_CMN_MD5] = {};
	Bool	restored = false;
	Bool	hasMap = true;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// application.iniファイルリード
		ret = CC_Read_application_ini(&rgnSet, false, &restored);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_Read_application_ini error, " HERE);
			break;
		}

		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"CC_CheckUPDate() errComeback = %d, " HERE, errComeback);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"CC_CheckUPDate() now_Region = %s, " HERE, rgnSet.now_Region);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"CC_CheckUPDate() folder_num = %d, " HERE, rgnSet.folder_num);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"CC_CheckUPDate() Region = %s, " HERE, rgnSet.dt_Folder[0].Region);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"CC_CheckUPDate() folder_Path = %s, " HERE, rgnSet.dt_Folder[0].folder_Path);

		// メモリ確保
		filePath = (Char*)SCC_MALLOC(SCC_MAX_PATH);
		if (NULL == filePath) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		saveFilePath = (Char*)SCC_MALLOC(SCC_MAX_PATH);
		if (NULL == saveFilePath) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		rgnInfo = (SCC_REGIONINFO*)SCC_MALLOC(sizeof(SCC_REGIONINFO));
		if (NULL == rgnInfo) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		upddata = (SCC_UPDATEDATA*)SCC_MALLOC(sizeof(SCC_UPDATEDATA) * 2);
		if (NULL == upddata) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// コールバック関数のポインタ設定
		callbackFnc.cancel   = (SCC_CANCEL_FNC)dlCBFnc->cancel;
		callbackFnc.progress = NULL;

		if (CC_ISEOS((Char*)chkUpdInf->regionCode)) {
			// 仕向け地の指定なし
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"region code nothing, " HERE);
			if (CC_ISEOS((Char*)rgnSet.now_Region)) {
				// 現在の仕向け設定もなし
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"no map data, " HERE);
				// リージョンデータ未ダウンロード
				chkUpdInf->updateStatus = SCC_MAPDWL_CHECK_RGN_AREA_DWLNONE;
				chkUpdInf->importSize   = 0;
				chkUpdInf->tempFileSize = 0;
				memset(chkUpdInf->hasUpdate, false, sizeof(chkUpdInf->hasUpdate));
				memset(chkUpdInf->version, 0, sizeof(chkUpdInf->version));
				break;
			} else {
				// 現在の仕向け設定あり
				// 現在の仕向けで更新チェック
				strcpy((char*)rgnInfo->regionCode, (char*)rgnSet.now_Region);

				// TODO:TR5.0暫定　広域地図の更新がある場合は、広域地図の更新は必須とする
				// 仕向け地と地図(MAP)フォルダのパスを設定する
				strcpy((char*)chkUpdInf->regionCode, (char*)rgnSet.now_Region);
				// 地図DBファイルパス生成
				for(num = 0; rgnSet.folder_num > num; num++){
					if(0 == strcmp((char*)chkUpdInf->regionCode, (char*)rgnSet.dt_Folder[num].Region)){
						// 現在仕向けの地図(MAP)フォルダのパス設定
						strcpy((char*)chkUpdInf->path, (char*)rgnSet.dt_Folder[num].folder_Path);
						break;
					}
				}
				// TODO:TR5.0暫定　広域地図の更新がある場合は、広域地図の更新は必須とする
			}
		} else {
			// 仕向け地の指定あり
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"region code=%s, " HERE, chkUpdInf->regionCode);
			// 指定された仕向けで更新チェック
			strcpy((char*)rgnInfo->regionCode, (char*)chkUpdInf->regionCode);
		}

		// AWS S3から仕向け情報取得
		ret = CC_CheckUpdate_GetRegionInfo(smcal, parm, dlTempDirPath, &callbackFnc, filePath, saveFilePath, rgnInfo);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_CheckUpdate_GetRegionInfo error, " HERE);
			break;
		}

		// AWS S3から広域地図およびプロパティデータの更新情報取得
		ret = CC_CheckUpdate_GetMapUpdateInfo(smcal, parm, dlTempDirPath, &callbackFnc, rgnInfo, filePath, saveFilePath, upddata);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_CheckUpdate_GetMapUpdateInfo error, " HERE);
			break;
		}

		if (errComeback) {
			// 異常復帰
			for (num = 0; num <= SCC_MAPDWL_KIND_DATA; num++) {
				if (chkUpdInf->hasUpdate[num]) {
					dlFlg[num] = SCC_DAL_DL_FLAG_NODOWNLOAD;
				} else {
					dlFlg[num] = SCC_DAL_DL_FLAG_DOWNLOAD;
				}
			}
			if (chkUpdInf->hasUpdate[SCC_MAPDWL_KIND_MAP]) {
				// Polaris.dbファイル有無チェック
				sprintf(filePath, "%s%s%s/%s", SCC_GetRootDirPath(), SCC_CMN_AWS_DIR_MAP, rgnInfo->regionCode, SCC_CMN_DB_FILE_MAP);
				if (0 != stat((char*)filePath, &st)) {
					hasMap = false;
				}
			}
		} else {
			// 異常復帰ではない
			// Polaris.dbファイル有無チェック
			sprintf(filePath, "%s%s%s/%s", SCC_GetRootDirPath(), SCC_CMN_AWS_DIR_MAP, rgnInfo->regionCode, SCC_CMN_DB_FILE_MAP);
			if (0 == stat((char*)filePath, &st)) {
				// 端末の広域地図およびプロパティデータの更新情報取得
				ret = CC_CheckUpdate_GetLcMapUpdateInfo(filePath, dlFlg, baseVersion);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_CheckUpdate_GetLcMapUpdateInfo error, " HERE);
					mapDBErr = true;
				}
			} else {
				mapDBErr = true;
			}

			if (mapDBErr) {
				// Polaris.dbファイルなし または Polaris.dbファイルのアクセスエラー
				// バージョンチェック
				ret = CC_CheckUpdate_CheckVersion(upddata, NULL, NULL, false, chkUpdInf, errComeback);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_CheckUpdate_CheckVersion error, " HERE);
					break;
				}
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"no map filePath=%s, " HERE, filePath);
				break;
			}
		}

		// バージョンチェック
		ret = CC_CheckUpdate_CheckVersion(upddata, dlFlg, baseVersion, hasMap, chkUpdInf, isErrComeback);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_CheckUpdate_CheckVersion error, " HERE);
			break;
		}
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"CC_CheckUPDate() updateStatus = %d, " HERE, chkUpdInf->updateStatus);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"CC_CheckUPDate() tempFileSize = %d, " HERE, chkUpdInf->tempFileSize);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"CC_CheckUPDate() importSize   = %d, " HERE, chkUpdInf->importSize);
	} while (0);

	// メモリ解放
	if (NULL != filePath) {
		SCC_FREE(filePath);
	}
	if (NULL != saveFilePath) {
		SCC_FREE(saveFilePath);
	}
	if (NULL != rgnInfo) {
		SCC_FREE(rgnInfo);
	}
	if (NULL != upddata) {
		SCC_FREE(upddata);
	}

	if (NULL != db) {
		// DAL終了化
		SCC_DAL_Finalize(db);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief AWS S3から仕向け情報取得
 * @param[in]  smcal            SMCAL
 * @param[in]  parm             APIパラメータ
 * @param[in]  dlTempDirPath    ダウンロードテンポラリフォルダ
 * @param[in]  callbackFnc      コールバック関数情報
 * @param[in]  regionCode       仕向け地コード
 * @param[in]  filePath         ファイルパス格納用バッファ(ワーク領域)
 * @param[in]  saveFilePath     ファイルパス格納用バッファ(ワーク領域)
 * @param[out] rgnInfo          仕向け情報
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_RESULT CC_CheckUpdate_GetRegionInfo(SMCAL *smcal,
										 T_CC_CMN_SMS_API_PRM *parm,
										 const Char *dlTempDirPath,
										 const SMPROGRESSCBFNC *callbackFnc,
										 Char *filePath,
										 Char *saveFilePath,
										 SCC_REGIONINFO *rgnInfo)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	DBOBJECT	*db = NULL;
	UChar		md5Str[CC_CMN_MD5 * 2] = {};
	UChar		md5[CC_CMN_MD5] = {};
	FILE		*fp = NULL;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// regionListInfo.tar.gzのAWS上のパス生成
		sprintf(filePath, "%s%s", SCC_CMN_AWS_DIR_APPDATA, SCC_CMN_TARGZ_FILE_REGION_LIST_MD5);
		// ダウンロードデータの保存先(端末)のパス生成
		sprintf(saveFilePath, "%s%s", dlTempDirPath, SCC_CMN_TARGZ_FILE_REGION_LIST_MD5);

		// ダウンロード
		ret = CC_Download(smcal, parm, filePath, saveFilePath, callbackFnc, 0, 0);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_Download error[dl=%s], [lc=%s], " HERE, filePath, saveFilePath);
			remove(saveFilePath);
			break;
		}
		// ダウンロードしたファイルからMD5を読み込む
		fp = fopen(saveFilePath, "r");
		if (NULL == fp) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"fopen error(0x%08x), path=%s, " HERE, errno, saveFilePath);
			ret = e_SC_RESULT_FILE_OPENERR;
			remove(saveFilePath);
			break;
		}
		// ファイルリード
		if (sizeof(md5Str) != fread(md5Str, 1, sizeof(md5Str), fp)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"fread error(0x%08x), path=%s, " HERE, errno, saveFilePath);
			ret = e_SC_RESULT_FILE_ACCESSERR;
			remove(saveFilePath);
			break;
		}
		// 16進数文字列をバイトの16進数コードの文字列に変換
		if (e_SC_RESULT_SUCCESS != CC_ChgHexString(md5Str, sizeof(md5Str), md5)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ChgHexString error, " HERE);
			ret = e_SC_RESULT_FAIL;
			remove(saveFilePath);
			break;
		}
		remove(saveFilePath);

		// regionListInfo.tar.gzのAWS上のパス生成
		sprintf(filePath, "%s%s", SCC_CMN_AWS_DIR_APPDATA, SCC_CMN_TARGZ_FILE_REGION_LIST);
		// ダウンロードデータの保存先(端末)のパス生成
		sprintf(saveFilePath, "%s%s", dlTempDirPath, SCC_CMN_TARGZ_FILE_REGION_LIST);

		// ダウンロード
		ret = CC_Download(smcal, parm, filePath, saveFilePath, callbackFnc, 0, 0);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_Download error[dl=%s], [lc=%s], " HERE, filePath, saveFilePath);
			remove(saveFilePath);
			break;
		}

		// ダウンロードしたファイルをチェックする
		ret = CC_CmnDL_CheckFile(saveFilePath, md5, 0);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_CmnDL_CheckFile error[path=%s], " HERE, saveFilePath);
			ret = e_SC_RESULT_MAP_GETERR;
			remove(saveFilePath);
			break;
		}

		// DBファイルのパス生成
		sprintf((char*)filePath, "%s%s", dlTempDirPath, SCC_CMN_DB_FILE_REGION_LIST);
		// パーミッション変更
		chmod(filePath, 0666);
		// tar.gz解凍後のファイルが存在する場合は、ダウンロード前に削除する
		remove(filePath);

		// tar.gz解凍
		ret = CC_UnTgz(saveFilePath, dlTempDirPath, callbackFnc);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_UnTgz error[zip=%s], [unzip=%s], " HERE, saveFilePath, dlTempDirPath);
			remove(saveFilePath);
			break;
		}
		remove(saveFilePath);

		// DBファイルのパス生成
		sprintf((char*)filePath, "%s%s", dlTempDirPath, SCC_CMN_DB_FILE_REGION_LIST);

		// DAL初期化
		ret = SCC_DAL_Initialize(filePath, &db);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Initialize error, " HERE);
			remove(filePath);
			break;
		}

		// regionListInfo.dbのREGION_INFOテーブルから仕向け情報取得
		ret = SCC_DAL_GetRegionInfo(db, rgnInfo);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_GetRegionInfo error, " HERE);
			break;
		}
	} while (0);

	// ファイルクローズ
	if (NULL != fp) {
		fclose(fp);
	}

	if (NULL != db) {
		// DAL終了化
		SCC_DAL_Finalize(db);
		remove(filePath);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief AWS S3から広域地図およびプロパティデータの更新情報取得
 * @param[in]  smcal            SMCAL
 * @param[in]  parm             APIパラメータ
 * @param[in]  dlTempDirPath    ダウンロードテンポラリフォルダ
 * @param[in]  callbackFnc      コールバック関数情報
 * @param[in]  rgnInfo          仕向け地情報
 * @param[in]  filePath         ファイルパス格納用バッファ(ワーク領域)
 * @param[in]  saveFilePath     ファイルパス格納用バッファ(ワーク領域)
 * @param[out] upddata          更新情報
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_RESULT CC_CheckUpdate_GetMapUpdateInfo(SMCAL *smcal,
											T_CC_CMN_SMS_API_PRM *parm,
											const Char *dlTempDirPath,
											const SMPROGRESSCBFNC *callbackFnc,
											const SCC_REGIONINFO *rgnInfo,
											Char *filePath,
											Char *saveFilePath,
											SCC_UPDATEDATA *upddata)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	DBOBJECT	*db = NULL;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// dataVersionInfo.tar.gzのAWS上のパス生成
		sprintf(filePath, "%s%s/%s", SCC_CMN_AWS_DIR_APPDATA, rgnInfo->regionCode, SCC_CMN_TARGZ_FILE_DATAVERSIONINFO);
		// ダウンロードデータの保存先(端末)のパス生成
		sprintf(saveFilePath, "%s%s", dlTempDirPath, SCC_CMN_TARGZ_FILE_DATAVERSIONINFO);

		// ダウンロード
		ret = CC_Download(smcal, parm, filePath, saveFilePath, callbackFnc, 0, 0);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_Download error[dl=%s], [lc=%s], " HERE, filePath, saveFilePath);
			remove(saveFilePath);
			break;
		}
		// ダウンロードしたファイルをチェックする
		ret = CC_CmnDL_CheckFile(saveFilePath, rgnInfo->md5, 0);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_CmnDL_CheckFile error[path=%s], " HERE, saveFilePath);
			remove(saveFilePath);
			break;
		}

		// tar.gz解凍
		ret = CC_UnTgz(saveFilePath, dlTempDirPath, callbackFnc);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_UnTgz error[zip=%s], [unzip=%s], " HERE, saveFilePath, dlTempDirPath);
			remove(saveFilePath);
			break;
		}
		remove(saveFilePath);

		// DBファイルのパス生成
		sprintf((char*)filePath, "%s%s", dlTempDirPath, SCC_CMN_DB_FILE_DATAVERSIONINFO);

		// DAL初期化
		ret = SCC_DAL_Initialize(filePath, &db);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Initialize error, " HERE);
			break;
		}

		// 検索条件設定
		upddata[SCC_MAPDWL_KIND_MAP].areaCode = 0;
		upddata[SCC_MAPDWL_KIND_MAP].dataType = SCC_DAL_UPDDATA_DATA_TYPE_BASE_MAP;
		strcpy(upddata[SCC_MAPDWL_KIND_MAP].dataKind, SCC_DAL_KIND_BASE);

		// 広域地図データの更新情報取得
		ret = SCC_DAL_GetUpdateData(db, &upddata[SCC_MAPDWL_KIND_MAP]);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_GetUpdateData(map) error, " HERE);
			remove(filePath);
			break;
		}

		// 検索条件設定
		upddata[SCC_MAPDWL_KIND_DATA].areaCode = 0;
		upddata[SCC_MAPDWL_KIND_DATA].dataType = SCC_DAL_UPDDATA_DATA_TYPE_SYS;
		strcpy(upddata[SCC_MAPDWL_KIND_DATA].dataKind, SCC_DAL_KIND_OTHER);

		// プロパティデータの更新情報取得
		ret = SCC_DAL_GetUpdateData(db, &upddata[SCC_MAPDWL_KIND_DATA]);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_GetUpdateData(data) error, " HERE);
			break;
		}
	} while (0);

	if (NULL != db) {
		// DAL終了化
		SCC_DAL_Finalize(db);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief 端末の広域地図およびプロパティデータの更新情報取得
 * @param[in]  dbFilePath       Polaris.dbファイルのパス
 * @param[out] dlFlg            DLフラグ
 * @param[out] baseVersion      ベースバージョン
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_RESULT CC_CheckUpdate_GetLcMapUpdateInfo(const Char *dbFilePath,
											  E_SCC_DAL_DLFLAG *dlFlg,
											  INT32 *baseVersion)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	DBOBJECT	*db = NULL;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// DAL初期化
		ret = SCC_DAL_Initialize(dbFilePath, &db);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Initialize error, " HERE);
			break;
		}

		// 端末のPolaris.dbから広域地図のDLフラグとベースバージョン取得
		ret = SCC_DAL_GetDLBaseVersion(db, SCC_DAL_KIND_BASE, SCC_DAL_KIND_PARCEL, &dlFlg[0], &baseVersion[0]);
		if (e_SC_RESULT_NODATA == ret) {
			ret = e_SC_RESULT_SUCCESS;
			*dlFlg = SCC_DAL_DL_FLAG_NODOWNLOAD;
			*baseVersion = 0;
		} else if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_CheckUpdate_GetMapUpdateInfo error, " HERE);
			break;
		}
		// 端末のPolaris.dbからプロパティデータのDLフラグとベースバージョン取得
		ret = SCC_DAL_GetDLBaseVersion(db, SCC_DAL_KIND_MGR, SCC_DAL_KIND_DOWNLOAD_SYS, &dlFlg[1], &baseVersion[1]);
		if (e_SC_RESULT_NODATA == ret) {
			ret = e_SC_RESULT_SUCCESS;
			*dlFlg = SCC_DAL_DL_FLAG_NODOWNLOAD;
			*baseVersion = 0;
		} else if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_CheckUpdate_GetMapUpdateInfo error, " HERE);
			break;
		}
	} while (0);

	if (NULL != db) {
		// DAL終了化
		SCC_DAL_Finalize(db);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief 広域地図とプロパティデータのバージョンチェック
 * @param[in]  upddata          更新情報
 * @param[in]  dlFlg            広域地図とプロパティデータのDLフラグ
 * @param[in]  baseVersion      広域地図とプロパティデータのベースバージョン
 * param[in]   hasMap           広域地図の有無
 * @param[out] chkUpdInf        データ更新情報
 * @param[in]  errComeback      異常復帰
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_RESULT CC_CheckUpdate_CheckVersion(const SCC_UPDATEDATA *upddata,
										const E_SCC_DAL_DLFLAG *dlFlg,
										const INT32 *baseVersion,
										Bool hasMap,
										SMCHECKUPDINFO *chkUpdInf,
										Bool errComeback)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	//Char	*chr = NULL;
	CC_APP_VERSION	appVer = {};
	CC_APP_VERSION	appVerS = {};
	CC_APP_VERSION	appVerE = {};
	upDTcheck_States	updateStatus[2] = {SCC_MAPDWL_CHECK_NONE};
	INT64	tempFileSize[2] = {};
	INT64	importSize[2] = {};
	INT64	tempFileSizeSum = 0;
	INT64	importSizeSum = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"errComeback=%d, " HERE, errComeback);

	// 端末のアプリバージョン分割
	ret = CC_CmnDL_SplitAppVersion((Char*)API_VERSION, &appVer);
	if (e_SC_RESULT_SUCCESS != ret) {
		SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_CmnDL_SplitAppVersion error, " HERE);
		SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
		return (ret);
	}

	for (num = 0; num <= SCC_MAPDWL_KIND_DATA; num++) {
		// 更新データのアプリバージョン分割
		ret = CC_CmnDL_SplitAppVersion(upddata[num].appVersionS, &appVerS);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_CmnDL_SplitAppVersion error, " HERE);
			break;
		}
		ret = CC_CmnDL_SplitAppVersion(upddata[num].appVersionE, &appVerE);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_CmnDL_SplitAppVersion error, " HERE);
			break;
		}
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"API_VERSION=%s, appVersionS=%s, appVersionE=%s, " HERE,
						   API_VERSION, upddata[num].appVersionS, upddata[num].appVersionE);

		// アプリバージョンチェック
		if ((appVer.ver1 < appVerS.ver1) ||
			((appVer.ver1 == appVerS.ver1) && (appVer.ver2 <  appVerS.ver2)) ||
			((appVer.ver1 == appVerS.ver1) && (appVer.ver2 == appVerS.ver2) && (appVer.ver3 <  appVerS.ver3)) ||
			((appVer.ver1 == appVerS.ver1) && (appVer.ver2 == appVerS.ver2) && (appVer.ver3 == appVerS.ver3) && (appVer.ver4 < appVerS.ver4))) {
			// Polaris.dbファイルの有無
			if ((!errComeback) && (!hasMap)) {
				// 異常復帰ではない かつ 端末にPolaris.dbファイルなし
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"Polaris.db not found, " HERE);
				// アプリの更新が必要
				updateStatus[num] = SCC_MAPDWL_CHECK_APPUPDATE_NEED;
				tempFileSize[num] = 0;
				importSize[num]   = 0;
				break;
			}
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"no update data. app version old, " HERE);
			continue;
		}
		if ((appVer.ver1 > appVerE.ver1) ||
			((appVer.ver1 == appVerE.ver1) && (appVer.ver2 >  appVerE.ver2)) ||
			((appVer.ver1 == appVerE.ver1) && (appVer.ver2 == appVerE.ver2) && (appVer.ver3 >  appVerE.ver3)) ||
			((appVer.ver1 == appVerE.ver1) && (appVer.ver2 == appVerE.ver2) && (appVer.ver3 == appVerE.ver3) && (appVer.ver4 > appVerE.ver4))) {
			// 未対応のアプリのバージョン(対応しているアプリのバージョンより新しい)
			// 通常ありえない
			if ((!errComeback) && (!hasMap)) {
				// 異常復帰ではない かつ 端末にPolaris.dbファイルなし
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"Polaris.db not found, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"app version error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"app_ver=%s, ver_s=%s, ver_e=%s, " HERE, API_VERSION, upddata[num].appVersionS, upddata[num].appVersionE);
				ret = e_SC_RESULT_FAIL;
				break;
			} else {
				// 更新可能な地図データなし
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"no update data. app version new, " HERE);
				updateStatus[num] = SCC_MAPDWL_CHECK_NONE;
				tempFileSize[num] = 0;
				importSize[num]   = 0;
			}
		}

		if ((!errComeback) && (!hasMap)) {
			// 異常復帰ではない かつ 端末にPolaris.dbファイルなし
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"Polaris.db not found, " HERE);
			// 地図データが存在しない
			updateStatus[num] = SCC_MAPDWL_CHECK_NO_DATA;
			tempFileSize[num] = (upddata[num].pressSize * 3);
			importSize[num]   = upddata[num].nonPressSize;
			continue;
		}

		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"dlFlg[%d]=%d, " HERE, num, dlFlg[num]);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"baseVersion[%d]=%d, " HERE, num, baseVersion[num]);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"upddata[%d].dataVersion=%d, " HERE, num, upddata[num].dataVersion);

		if (!errComeback) {
			// DLフラグとベースバージョンチェック
			if (0 == num) {
				if (baseVersion[num] < upddata[num].dataVersion) {
					// 更新可能な地図データあり（更新任意）
					updateStatus[num] = SCC_MAPDWL_CHECK_POSSIBLE;
					tempFileSize[num] = (upddata[num].pressSize * 3);
					importSize[num]   = upddata[num].nonPressSize;
					continue;
				}
			} else {
				if ((SCC_DAL_DL_FLAG_DOWNLOAD != dlFlg[num]) ||
					(baseVersion[num] < upddata[num].dataVersion)) {
					// 更新可能な地図データあり（更新必須）
					updateStatus[num] = SCC_MAPDWL_CHECK_NEED;
					tempFileSize[num] = (upddata[num].pressSize * 3);
					importSize[num]   = upddata[num].nonPressSize;
					continue;
				}
			}
		} else {
			// 異常復帰の場合は、ベースバージョンチェックは不要
			// 異常前にチェックした結果を使用する
			if (chkUpdInf->hasUpdate[num]) {
				// 更新可能な地図データあり（更新必須）
				updateStatus[num] = SCC_MAPDWL_CHECK_NEED;
			} else {
				updateStatus[num] = SCC_MAPDWL_CHECK_NONE;
			}
		}
	}

	if (e_SC_RESULT_SUCCESS == ret) {
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"updateStatus[0]=%d, " HERE, updateStatus[SCC_MAPDWL_KIND_MAP]);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"updateStatus[1]=%d, " HERE, updateStatus[SCC_MAPDWL_KIND_DATA]);
		// TODO:TR5.0暫定　広域地図の更新がある場合は、広域地図の更新は必須とする
		if (SCC_MAPDWL_CHECK_POSSIBLE == updateStatus[SCC_MAPDWL_KIND_MAP]) {
			// 広域地図の更新は必須に変更する
			updateStatus[SCC_MAPDWL_KIND_MAP] = SCC_MAPDWL_CHECK_NEED;
		}
		// TODO:TR5.0暫定　更新がある場合は必須とする

		if ((SCC_MAPDWL_CHECK_NONE == updateStatus[SCC_MAPDWL_KIND_MAP]) &&
			(SCC_MAPDWL_CHECK_NONE == updateStatus[SCC_MAPDWL_KIND_DATA])) {
			// 更新なし
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"no update data, " HERE);
			chkUpdInf->updateStatus = SCC_MAPDWL_CHECK_NONE;
			chkUpdInf->tempFileSize = 0;
			chkUpdInf->importSize   = 0;
			memset(chkUpdInf->hasUpdate, false, sizeof(chkUpdInf->hasUpdate));
			memset(chkUpdInf->version, 0, sizeof(chkUpdInf->version));
		} else if ((SCC_MAPDWL_CHECK_APPUPDATE_NEED == updateStatus[SCC_MAPDWL_KIND_MAP]) ||
			(SCC_MAPDWL_CHECK_APPUPDATE_NEED == updateStatus[SCC_MAPDWL_KIND_DATA])) {
			// アプリの更新が必要
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"need app update, " HERE);
			chkUpdInf->updateStatus = SCC_MAPDWL_CHECK_APPUPDATE_NEED;
			chkUpdInf->tempFileSize = 0;
			chkUpdInf->importSize   = 0;
			memset(chkUpdInf->hasUpdate, false, sizeof(chkUpdInf->hasUpdate));
			memset(chkUpdInf->version, 0, sizeof(chkUpdInf->version));
		} else if ((SCC_MAPDWL_CHECK_NO_DATA == updateStatus[SCC_MAPDWL_KIND_MAP]) ||
			(SCC_MAPDWL_CHECK_NO_DATA == updateStatus[SCC_MAPDWL_KIND_DATA])) {
			// 地図データが存在しない
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"no map data, " HERE);
			chkUpdInf->updateStatus  = SCC_MAPDWL_CHECK_NO_DATA;
			memset(chkUpdInf->hasUpdate, false, (sizeof(chkUpdInf->hasUpdate)));
			memset(chkUpdInf->version, 0, sizeof(chkUpdInf->version));
			chkUpdInf->hasUpdate[SCC_MAPDWL_KIND_MAP] = true;
			chkUpdInf->hasUpdate[SCC_MAPDWL_KIND_DATA] = true;
			chkUpdInf->tempFileSize = (((tempFileSize[SCC_MAPDWL_KIND_DATA] + tempFileSize[SCC_MAPDWL_KIND_MAP]) + (SCC_SIZE_MB - 1)) / SCC_SIZE_MB);
			chkUpdInf->importSize   = (((importSize[SCC_MAPDWL_KIND_DATA] + importSize[SCC_MAPDWL_KIND_MAP]) + (SCC_SIZE_MB - 1)) / SCC_SIZE_MB);
		} else if ((SCC_MAPDWL_CHECK_NEED == updateStatus[SCC_MAPDWL_KIND_MAP]) ||
			(SCC_MAPDWL_CHECK_NEED == updateStatus[SCC_MAPDWL_KIND_DATA])) {
			// 更新必須
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"need update data, " HERE);
			chkUpdInf->updateStatus  = SCC_MAPDWL_CHECK_NEED;
			memset(chkUpdInf->hasUpdate, false, (sizeof(chkUpdInf->hasUpdate)));
			memset(chkUpdInf->version, 0, sizeof(chkUpdInf->version));
			if (SCC_MAPDWL_CHECK_NEED == updateStatus[SCC_MAPDWL_KIND_MAP]) {
				chkUpdInf->hasUpdate[SCC_MAPDWL_KIND_MAP] = true;
				tempFileSizeSum += tempFileSize[SCC_MAPDWL_KIND_MAP];
				importSizeSum   += importSize[SCC_MAPDWL_KIND_MAP];
			} else {
				chkUpdInf->hasUpdate[SCC_MAPDWL_KIND_MAP] = false;
			}
			if (SCC_MAPDWL_CHECK_NEED == updateStatus[SCC_MAPDWL_KIND_DATA]) {
				chkUpdInf->hasUpdate[SCC_MAPDWL_KIND_DATA] = true;
				tempFileSizeSum += tempFileSize[SCC_MAPDWL_KIND_DATA];
				importSizeSum   += importSize[SCC_MAPDWL_KIND_DATA];
			} else {
				chkUpdInf->hasUpdate[SCC_MAPDWL_KIND_DATA] = false;
			}
			chkUpdInf->tempFileSize = ((tempFileSizeSum + (SCC_SIZE_MB - 1)) / SCC_SIZE_MB);
			chkUpdInf->importSize   = ((importSizeSum + (SCC_SIZE_MB - 1)) / SCC_SIZE_MB);
		} else if ((SCC_MAPDWL_CHECK_POSSIBLE == updateStatus[SCC_MAPDWL_KIND_MAP]) ||
			(SCC_MAPDWL_CHECK_POSSIBLE == updateStatus[SCC_MAPDWL_KIND_DATA])) {
			// 更新任意
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"new update data, " HERE);
			chkUpdInf->updateStatus = SCC_MAPDWL_CHECK_POSSIBLE;
			chkUpdInf->tempFileSize = (((tempFileSize[SCC_MAPDWL_KIND_MAP] + tempFileSize[SCC_MAPDWL_KIND_DATA]) + (SCC_SIZE_MB - 1)) / SCC_SIZE_MB);
			chkUpdInf->importSize   = (((importSize[SCC_MAPDWL_KIND_MAP] + importSize[SCC_MAPDWL_KIND_DATA]) + (SCC_SIZE_MB - 1)) / SCC_SIZE_MB);
			memset(chkUpdInf->hasUpdate, false, (sizeof(chkUpdInf->hasUpdate)));
			memset(chkUpdInf->version, 0, sizeof(chkUpdInf->version));
			if (SCC_MAPDWL_CHECK_NONE != updateStatus[SCC_MAPDWL_KIND_DATA]) {
				chkUpdInf->hasUpdate[SCC_MAPDWL_KIND_DATA] = true;
				tempFileSizeSum += tempFileSize[SCC_MAPDWL_KIND_DATA];
				importSizeSum   += importSize[SCC_MAPDWL_KIND_DATA];
			} else {
				chkUpdInf->hasUpdate[SCC_MAPDWL_KIND_DATA] = false;
			}
			if (SCC_MAPDWL_CHECK_NONE != updateStatus[SCC_MAPDWL_KIND_MAP]) {
				chkUpdInf->hasUpdate[SCC_MAPDWL_KIND_MAP] = true;
				tempFileSizeSum += tempFileSize[SCC_MAPDWL_KIND_MAP];
				importSizeSum   += importSize[SCC_MAPDWL_KIND_MAP];
			} else {
				chkUpdInf->hasUpdate[SCC_MAPDWL_KIND_MAP] = false;
			}
			chkUpdInf->tempFileSize = ((tempFileSizeSum + (SCC_SIZE_MB - 1)) / SCC_SIZE_MB);
			chkUpdInf->importSize   = ((importSizeSum + (SCC_SIZE_MB - 1)) / SCC_SIZE_MB);
		} else {
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"no update data, " HERE);
			chkUpdInf->updateStatus = SCC_MAPDWL_CHECK_NONE;
			chkUpdInf->tempFileSize = 0;
			chkUpdInf->importSize   = 0;
			memset(chkUpdInf->hasUpdate, false, sizeof(chkUpdInf->hasUpdate));
			memset(chkUpdInf->version, 0, sizeof(chkUpdInf->version));
		}
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}
