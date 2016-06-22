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

static const SMMAPDLCBFNC	*smMapDLCBFnc;

static E_SC_RESULT CC_DLRegionInfoDB(SMCAL *smcal, T_CC_CMN_SMS_API_PRM *apiParam, const Char *dlTempDirPath, const SMMAPDLCBFNC *callbackFnc, Char *dlFilePath, Char *saveFilePath);

/**
 * @brief リージョン情報リスト取得
 * @param[in]     smcal         SMCAL
 * @param[in]     parm          APIパラメータ
 * @param[out]    rgnI          リージョン情報リスト
 * @param[in/out] rgnNum        リージョン数（in時：MAX値  out時：取得した実データの地方数）
 * @param[in]     dlTempDirPath ダウンロードテンポラリフォルダ
 * @param[in]     callbackFnc   コールバック関数情報
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_RESULT CC_GetRgnInfoList(SMCAL *smcal,
								T_CC_CMN_SMS_API_PRM *parm,
								SMRGNINFO *rgnI,
								INT32 *rgnNum,
								const Char *dlTempDirPath,
								const SMMAPDLCBFNC *callbackFnc)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	Char	*dlFilePath = NULL;
	Char	*saveFilePath = NULL;
	Char	*filePath = NULL;
	DBOBJECT *db = NULL;
	INT32	num = 0;

	SCC_REGIONINFO	*rgnInf = NULL;
	Char *rgn_cd = NULL;
	INT32 rgnInfoNum = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// メモリ確保
		dlFilePath = (Char*)SCC_MALLOC(SCC_MAX_PATH);
		if (NULL == dlFilePath) {
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

		// 初期化
		smMapDLCBFnc = callbackFnc;

		// メモリ確保
		filePath = (Char*)SCC_MALLOC(SCC_MAX_PATH);
		if (NULL == filePath) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// リージョン情報DBファイルのパス生成
		if ('/' == dlTempDirPath[strlen(dlTempDirPath) - 1]) {
			sprintf((char*)filePath, "%s%s", dlTempDirPath, SCC_CMN_DB_FILE_REGION_LIST);
		} else {
			sprintf((char*)filePath, "%s/%s", dlTempDirPath, SCC_CMN_DB_FILE_REGION_LIST);
		}
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"regionList_Info_DB file path(Device) = %s, " HERE, filePath);

		// パーミッション変更
		chmod(filePath, 0666);
		// tar.gz解凍後のファイルが存在する場合は、ダウンロード前に削除する
		remove(filePath);

		// リージョン情報DB(regionListInfo.db)ダウンロード
		ret = CC_DLRegionInfoDB(smcal, parm, dlTempDirPath, callbackFnc, dlFilePath, saveFilePath);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_DLVersionInfoDB error, " HERE);
			break;
		}

		// リージョン情報DBのDAL初期化
		ret = SCC_DAL_Initialize(filePath, &db);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Initialize error, " HERE);
			break;
		}

		if (NULL != db) {

			// メモリ確保
			rgnInf = (SCC_REGIONINFO*)SCC_MALLOC(sizeof(SCC_REGIONINFO) * (*rgnNum));
			if (NULL == rgnInf) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
				ret = e_SC_RESULT_MALLOC_ERR;
				break;
			}

			// REGION_INFOテーブル情報取得
			ret = SCC_DAL_GetRegionInfoList(db, *rgnNum, rgnInf, &rgnInfoNum);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_GetRegionInfo error, " HERE);
				break;
			}
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"SCC_DAL_GetRegionInfo() rgnInf_count = %d, " HERE, rgnInfoNum);

//*****************************************************************
#if 0
INT32 rgn_cnt = 0;
SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"rgnNum:%d, " HERE, rgnInfoNum);
for(rgn_cnt = 0; rgnInfoNum > rgn_cnt; rgn_cnt++){
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"rgnCD:%s  rgnNM:%s  MAP_PRESS_SZ:%d  MAP_NON_PRESS_SZ:%d  DATA_PRESS_SZ:%d  DATA_NON_PRESS_SZ:%d, " HERE,
			rgnInf[rgn_cnt].regionCode, rgnInf[rgn_cnt].regionName, rgnInf[rgn_cnt].mapPressSize, rgnInf[rgn_cnt].mapNonPressize,
			rgnInf[rgn_cnt].dataPressSize, rgnInf[rgn_cnt].dataNonPressSize);
}
#endif
//*****************************************************************

			// リージョン情報リスト作成
			for(num = 0; rgnInfoNum > num; num++){
				// 仕向け地コード設定
				strcpy(rgnI[num].regionCode, rgnInf[num].regionCode);

				// 仕向け地名称設定
				strcpy(rgnI[num].regionName, rgnInf[num].regionName);

				// 広域地図データ、プロパティファイルの一時ファイルサイズ設定
				rgnI[num].tempFileSize = ((((rgnInf[num].mapNonPressize + rgnInf[num].dataNonPressSize) * 3) + (SCC_SIZE_MB - 1)) / SCC_SIZE_MB);

				// 広域地図データ、プロパティファイルの解凍後サイズ設定
				rgnI[num].importSize = (((rgnInf[num].mapNonPressize + rgnInf[num].dataNonPressSize) + (SCC_SIZE_MB - 1)) / SCC_SIZE_MB);
			}

			// break？
			if((NULL == rgn_cd) && (rgnInfoNum > num)){
				ret = e_SC_RESULT_FAIL;
				rgnInfoNum = 0;
			}

			// リージョン情報数設定
			*rgnNum = rgnInfoNum;
		}

	} while (0);

	// DAL終了化
	if (NULL != db) {
		SCC_DAL_Finalize(db);
		remove(filePath);
	}

	// メモリ解放
	if (NULL != dlFilePath) {
		SCC_FREE(dlFilePath);
	}
	if (NULL != saveFilePath) {
		SCC_FREE(saveFilePath);
	}
	if (NULL != filePath) {
		SCC_FREE(filePath);
	}
	if (NULL != rgnInf) {
		SCC_FREE(rgnInf);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief リージョン情報DB(regionListInfo.db)ダウンロード
 * @param[in] smcal         SMCAL
 * @param[in] apiParam      APIパラメータ
 * @param[in] dlTempDirPath ダウンロードテンポラリフォルダ
 * @param[in] callbackFnc   コールバック関数ポインタ
 * @param[in] dlFilePath    ダウンロード元ファイルパス格納用バッファ
 * @param[in] saveFilePath  ダウンロードファイル保存先パス格納用バッファ
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_RESULT CC_DLRegionInfoDB(SMCAL *smcal,
								T_CC_CMN_SMS_API_PRM *apiParam,
								const Char *dlTempDirPath,
								const SMMAPDLCBFNC *callbackFnc,
								Char *dlFilePath,
								Char *saveFilePath)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	//E_SC_CAL_RESULT	calRet = e_SC_CAL_RESULT_SUCCESS;
	//Char	*filePath = NULL;
	SMPROGRESSCBFNC	progressCBFnc = {};
	//SCC_DOWNLOADBASEVERSION	*dlBaseVer = NULL;
	//INT32	dlBaseVerNum = 0;
	UChar	md5Str[CC_CMN_MD5 * 2] = {};
	UChar	md5[CC_CMN_MD5] = {};
	FILE	*fp = NULL;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// コールバック関数ポインタ設定
		progressCBFnc.cancel   = (SCC_CANCEL_FNC)callbackFnc->cancel;
		progressCBFnc.progress = NULL;

		// ダウンロードデータのAWS上のパス生成
		sprintf(dlFilePath, "%s%s", SCC_CMN_AWS_DIR_APPDATA, SCC_CMN_TARGZ_FILE_REGION_LIST_MD5);
		// ダウンロードデータの保存先(端末)のパス生成
		sprintf(saveFilePath, "%s%s", dlTempDirPath, SCC_CMN_TARGZ_FILE_REGION_LIST_MD5);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"CC_DLVersionInfoDB() dlFilePath = %s, " HERE, dlFilePath);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"CC_DLVersionInfoDB() saveFilePath = %s, " HERE, saveFilePath);

		// ダウンロード
		ret = CC_Download(smcal, apiParam, dlFilePath, saveFilePath, &progressCBFnc, 0, 0);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_Download error[dl=%s], [save=%s], " HERE, dlFilePath, saveFilePath);
			break;
		}
		// ダウンロードしたファイルからMD5を読み込む
		fp = fopen(saveFilePath, "r");
		if (NULL == fp) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"fopen error(0x%08x), path=%s, " HERE, errno, saveFilePath);
			remove(saveFilePath);
			ret = e_SC_RESULT_FILE_OPENERR;
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

		// ダウンロードデータのAWS上のパス生成
		sprintf(dlFilePath, "%s%s", SCC_CMN_AWS_DIR_APPDATA, SCC_CMN_TARGZ_FILE_REGION_LIST);
		// ダウンロードデータの保存先(端末)のパス生成
		sprintf(saveFilePath, "%s%s", dlTempDirPath, SCC_CMN_TARGZ_FILE_REGION_LIST);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"CC_DLVersionInfoDB() dlFilePath = %s, " HERE, dlFilePath);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"CC_DLVersionInfoDB() saveFilePath = %s, " HERE, saveFilePath);

		// ダウンロード
		ret = CC_Download(smcal, apiParam, dlFilePath, saveFilePath, &progressCBFnc, 0, 0);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_Download error[dl=%s], [save=%s], " HERE, dlFilePath, saveFilePath);
			break;
		}
		// ダウンロードしたファイルをチェックする
		ret = CC_CmnDL_CheckFile(saveFilePath, md5, 0);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_CmnDL_CheckFile error[path=%s], " HERE, saveFilePath);
			remove(saveFilePath);
			ret = e_SC_RESULT_MAP_GETERR;
			break;
		}

		// tar.gz解凍
		ret = CC_UnTgz(saveFilePath, dlTempDirPath, &progressCBFnc);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_UnTgz error[zip=%s], [unzip=%s], " HERE, saveFilePath, dlTempDirPath);	// TODO
			break;
		}

	} while (0);

	// ファイルクローズ
	if (NULL != fp) {
		fclose(fp);
	}

	// ダウンロードしたファイル削除(エラーはチェックしない)
//	remove(saveFilePath);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

