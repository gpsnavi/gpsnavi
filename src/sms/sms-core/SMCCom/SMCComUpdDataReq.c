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

#define	CC_UPDDATA_APPDATA							"appData.tar.gz"
#define	CC_UPDDATA_REGNMAP							"regionalMap.tar.gz"

static SMPROGRESSINFO	smProgressInfo;
static SMMAPUPDSTATUS	smUpdStatus;

static E_SC_RESULT CC_UpdBase(SMCAL *smcal, T_CC_CMN_SMS_API_PRM *apiParam, const SMUPDINFO *mapUpdInfo, INT32 appDataVersion, const Char *dlTempDirPath, const SCC_UPDATEDATA *updData, SMMAPUPDSTATUS *updStatus, SCC_CANCEL_FNC cancelCBFnc, Char *dlFilePath, Char *saveFilePath, DBOBJECT **db);
static E_SC_RESULT CC_UpdArea(SMCAL *smcal, T_CC_CMN_SMS_API_PRM *apiParam, const SMUPDINFO *mapUpdInfo, const Char *dlTempDirPath, const SCC_UPDATEDATA *updData, SCC_CANCEL_FNC cancelCBFnc, Char *dlFilePath, Char *saveFilePath, DBOBJECT **db);
static void CC_UpdData_UpdApplicationIni(const SMUPDINFO *mapUpdInfo);

/**
 * @brief データダウンロード
 * @param[in] apiParam      APIパラメータ
 * @param[in] mapUpdInfo    データ更新情報
 * @param[in] dlTempDirPath ダウンロードテンポラリフォルダ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_UpdDataReq(SMCAL *smcal,
						  T_CC_CMN_SMS_API_PRM *apiParam,
						  const SMUPDINFO *mapUpdInfo,
						  INT32 appDataVersion,
						  const Char *dlTempDirPath,
						  const SCC_UPDATEDATA *updData,
						  UINT32 updDataNum,
						  UINT32 updDataNumProgress,
						  SMMAPUPDSTATUS *updStatus,
						  const SMMAPDLCBFNC *callbackFnc)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	Char	*dlFilePath = NULL;
	Char	*saveFilePath = NULL;
	Char	*filePath = NULL;
	DBOBJECT *db = NULL;
	INT32	num = 0;
	//UINT32	dataNum = 0;
	const SCC_UPDATEDATA	*updAreaData = updData;
	SMPROGRESSCBFNC	cbFnc = {};
	//FILE	*fp = NULL;
	struct	stat st = {};

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISEOS((Char*)mapUpdInfo->installDirPath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mapUpdInfo->installDirPath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

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
		filePath = (Char*)SCC_MALLOC(SCC_MAX_PATH);
		if (NULL == filePath) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		*filePath = EOS;

		// 初期化
		memset(&smProgressInfo, 0, sizeof(SMPROGRESSINFO));
		memset(&smUpdStatus, 0, sizeof(SMMAPUPDSTATUS));
		cbFnc.cancel   = callbackFnc->cancel;
		cbFnc.progress = NULL;
		*saveFilePath  = EOS;

		// 進捗状況設定
		smProgressInfo.completeCount = 0;
		smProgressInfo.totalCount    = updDataNumProgress;

		if (CC_MAPUPDSTATUS_INIT == updStatus->status) {
			// AWSアクセス情報取得
			ret = CC_GetAWSBucketName(smcal, apiParam, e_HTTP_METHOD_GET, e_AWS_BACKET_MAP, updStatus->backetName);
			if(e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetAWSBucketName error, " HERE);
				break;
			}

			// ダウンロードデータのAWS上のパス生成
			sprintf(dlFilePath, "%s%s/%s", SCC_CMN_AWS_DIR_APPDATA, mapUpdInfo->regionCode, SCC_CMN_DATAVERSION);
			// ダウンロードデータの保存先(端末)のパス生成
			sprintf(filePath, "%s%s_%s", dlTempDirPath, mapUpdInfo->regionCode, SCC_CMN_DATAVERSION);
			// dataVersion.txtをダウンロードする
			ret = CC_Download(smcal, apiParam, dlFilePath, filePath, &cbFnc, 0, 0);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_Download error[dl=%s], [lc=%s], " HERE, dlFilePath, filePath);
				break;
			}
		}

		// updStatus->hasUpdateを見て、ダウンロードからやり直しならupdStatus->statusを変更する
		if (updStatus->hasUpdate[SCC_MAPDWL_KIND_MAP]) {
			if (CC_MAPUPDSTATUS_MAPUPDATE == updStatus->status) {
				sprintf(filePath, "%s%s", dlTempDirPath, CC_UPDDATA_REGNMAP);
				if (0 != stat(filePath, &st)) {
					// 広域地図ダウンロード
					updStatus->status = CC_MAPUPDSTATUS_MAPDL;
				}
			} else if (CC_MAPUPDSTATUS_MAPUPDATED < updStatus->status) {
				// 広域地図ダウンロード
				updStatus->status = CC_MAPUPDSTATUS_MAPDL;
			}
		} else if (updStatus->hasUpdate[SCC_MAPDWL_KIND_DATA]) {
			if (CC_MAPUPDSTATUS_DATAUPDATE == updStatus->status) {
				sprintf(filePath, "%s%s", dlTempDirPath, CC_UPDDATA_APPDATA);
				if (0 != stat(filePath, &st)) {
					// プロパティデータダウンロード
					updStatus->status = CC_MAPUPDSTATUS_DATADL;
				}
			} else if (CC_MAPUPDSTATUS_DATAUPDATED < updStatus->status) {
				// プロパティデータダウンロード
				updStatus->status = CC_MAPUPDSTATUS_DATADL;
			}
		}
		// 更新状況設定
		CC_UpdDataProgressMng_SetUpdStatus(updStatus);
		// 更新状況保存
		ret = CC_UpdDataProgressMng_SaveUpdStatus();
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_UpdDataProgressMng_SaveUpdStatus error, " HERE);
			break;
		}

		// プロパティファイルと広域地図ダウンロード
		if (mapUpdInfo->rgnMapUpdate) {
			ret = CC_UpdBase(smcal,
							 apiParam,
							 mapUpdInfo,
							 appDataVersion,
							 dlTempDirPath,
							 updData,
							 updStatus,
							 callbackFnc->cancel,
							 dlFilePath,
							 saveFilePath,
							 &db);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_UpdBase error, " HERE);
				break;
			}
		}
		if (EOS != *filePath) {
			// ファイル削除
			remove(filePath);
		}

		// application.iniファイル更新 (地図DBのフォルダパスをapplication.iniに書込み)
		CC_UpdData_UpdApplicationIni(mapUpdInfo);

		// パーセルテーブルと地域クラステーブル更新
		if (0 < mapUpdInfo->updateAreaNum) {
			if (NULL == db) {
				// DBファイルのパス生成
				if ('/' == mapUpdInfo->installDirPath[strlen(mapUpdInfo->installDirPath) - 1]) {
					sprintf((char*)filePath, "%s%s%s/%s", mapUpdInfo->installDirPath, CC_CMN_MAP_DIR_PATH, mapUpdInfo->regionCode, SCC_CMN_DB_FILE_MAP);
				} else {
					sprintf((char*)filePath, "%s/%s%s/%s", mapUpdInfo->installDirPath, CC_CMN_MAP_DIR_PATH, mapUpdInfo->regionCode, SCC_CMN_DB_FILE_MAP);
				}
				if (0 != stat(filePath, &st)) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"no map filePath=%s, " HERE, filePath);
					ret = e_SC_RESULT_MAP_UPDATE_ERR;
					break;
				}
			}
			for (num = 0; num < SCC_MAPDWL_KIND_NUM; num++) {
				if (updStatus->hasUpdate[num]) {
					updAreaData++;
				}
			}
			ret = CC_UpdArea(smcal,
							 apiParam,
							 mapUpdInfo,
							 dlTempDirPath,
							 updAreaData,
							 callbackFnc->cancel,
							 dlFilePath,
							 saveFilePath,
							 &db);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_UpdArea error, " HERE);
				break;
			}
		}
	} while (0);

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

	// DAL終了化
	if (NULL != db) {
		SCC_DAL_Finalize(db);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief ベース(プロパティファイルと広域地図)更新
 * @param[in] smcal         SMCAL
 * @param[in] apiParam      APIパラメータ
 * @param[in] mapUpdInfo    データ更新情報
 * @param[in] dlTempDirPath ダウンロードテンポラリフォルダ
 * @param[in] callbackFnc   コールバック関数ポインタ
 * @param[in] dlFilePath    ダウンロードファイルパス格納用バッファ
 * @param[in] saveFilePath  ダウンロードファイル保存先パス格納用バッファ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_UpdBase(SMCAL *smcal,
					   T_CC_CMN_SMS_API_PRM *apiParam,
					   const SMUPDINFO *mapUpdInfo,
					   INT32 appDataVersion,
					   const Char *dlTempDirPath,
					   const SCC_UPDATEDATA *updData,
					   SMMAPUPDSTATUS *updStatus,
					   SCC_CANCEL_FNC cancelCBFnc,
					   Char *dlFilePath,
					   Char *saveFilePath,
					   DBOBJECT **db)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	Char	*filePath = NULL;
	SMPROGRESSCBFNC	callbackFnc = {};
	SCC_DOWNLOADBASEVERSION	*dlBaseVer = NULL;
	SCC_UPDVER_DOWNLOADSYS	dlSys = {};
	Bool	isRollback = false;
	UINT32	startByte = 0;
	UINT32	endByte = 0;
	INT32	num = 0;
	INT32	idx = 0;
	struct	stat st = {};
	//UChar	md5Str[(CC_CMN_MD5 * 2) + 1] = {};
	//UChar	md5[CC_CMN_MD5] = {};
	FILE	*fp = NULL;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// メモリ確保
		dlBaseVer = (SCC_DOWNLOADBASEVERSION*)SCC_MALLOC(sizeof(SCC_DOWNLOADBASEVERSION) * 5);
		if (NULL == dlBaseVer) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		filePath = (Char*)SCC_MALLOC(SCC_MAX_PATH);
		if (NULL == filePath) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// コールバック関数ポインタ設定
		callbackFnc.cancel = (SCC_CANCEL_FNC)cancelCBFnc;

		/*** 広域地図 ***/
		if (updStatus->hasUpdate[SCC_MAPDWL_KIND_MAP]) {
			if (CC_MAPUPDSTATUS_MAPDL >= updStatus->status) {
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"Map Download, " HERE);
				// ダウンロードデータのAWS上のパス生成
				sprintf(dlFilePath, "%s%s/%s%s", SCC_CMN_AWS_DIR_APPDATA, mapUpdInfo->regionCode, SCC_CMN_AWS_DIR_MAP, CC_UPDDATA_REGNMAP);
				// ダウンロードデータの保存先(端末)のパス生成
				sprintf(saveFilePath, "%s%s", dlTempDirPath, CC_UPDDATA_REGNMAP);

				// 広域地図ダウンロード
				updStatus->status = CC_MAPUPDSTATUS_MAPDL;
				CC_UpdDataProgressMng_SetStatus(CC_MAPUPDSTATUS_MAPDL);

				// 進捗設定
				if (0 == stat(saveFilePath, &st)) {
					smProgressInfo.doneSize = (INT64)st.st_size;
				} else {
					smProgressInfo.doneSize = 0;
				}
				smProgressInfo.totalSize = updData[idx].pressSize;
				sprintf(smProgressInfo.msg, "%s", SCC_CMN_PROGRESS_MSG_RGNMAP_DL);
				CC_UpdDataProgressMng_SetProgress(&smProgressInfo);
				// 進捗情報保存
				ret = CC_UpdDataProgressMng_SaveUpdStatus();
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_UpdDataProgressMng_SaveUpdStatus, " HERE);
					break;
				}

				// コールバック関数ポインタ設定
				callbackFnc.progress = (SCC_PROGRESS_FNC)CC_UpdData_ProgressCallbackDL;

				startByte = smProgressInfo.doneSize;
				endByte   = smProgressInfo.totalSize;
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"startByte=%d - endByte=%d, " HERE, startByte, endByte);
				if (startByte < endByte) {
					// ダウンロード
					ret = CC_Download(smcal, apiParam, dlFilePath, saveFilePath, &callbackFnc, startByte, endByte);
					if (e_SC_RESULT_SUCCESS != ret) {
						SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_Download error[dl=%s], [save=%s], " HERE, dlFilePath, saveFilePath);
						break;
					}
				}
				// ダウンロードしたファイルをチェックする
				ret = CC_CmnDL_CheckFile(saveFilePath, updData[idx].md5, updData[idx].pressSize);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_CmnDL_CheckFile error[path=%s], " HERE, saveFilePath);
					if ((e_SC_RESULT_CANCEL != ret) && (e_SC_RESULT_SERVER_STOP != ret)) {
						// tar.gzファイル削除
						remove(saveFilePath);
					}
					break;
				}
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"msg=%s, " HERE, smProgressInfo.msg);
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"doneSize=%lld, totalSize=%lld, completeCount=%u, totalCount=%u, " HERE,
								   smProgressInfo.doneSize, smProgressInfo.totalSize,
								   smProgressInfo.completeCount, smProgressInfo.totalCount);
			}

			if (CC_MAPUPDSTATUS_MAPUPDATE >= updStatus->status) {
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"Map Update, " HERE);
				// tar.gzファイルのパス生成
				sprintf(saveFilePath, "%s%s", dlTempDirPath, CC_UPDDATA_REGNMAP);
				// 広域地図更新
				updStatus->status = CC_MAPUPDSTATUS_MAPUPDATE;
				CC_UpdDataProgressMng_SetStatus(CC_MAPUPDSTATUS_MAPUPDATE);
				// 進捗設定
				smProgressInfo.doneSize  = 0;
				smProgressInfo.totalSize = updData[idx].nonPressSize;
				sprintf(smProgressInfo.msg, "%s", SCC_CMN_PROGRESS_MSG_RGNMAP_TARGZ);
				CC_UpdDataProgressMng_SetProgress(&smProgressInfo);
				// 進捗情報保存
				ret = CC_UpdDataProgressMng_SaveUpdStatus();
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_UpdDataProgressMng_SaveUpdStatus, " HERE);
					break;
				}

				// コールバック関数ポインタ設定
				callbackFnc.progress = (SCC_PROGRESS_FNC)CC_UpdData_ProgressCallbackTargz;
				if ('/' == mapUpdInfo->installDirPath[strlen(mapUpdInfo->installDirPath) - 1]) {
					sprintf((char*)filePath, "%s", mapUpdInfo->installDirPath);
				} else {
					sprintf((char*)filePath, "%s/", mapUpdInfo->installDirPath);
				}

				// tar.gz解凍
				ret = CC_UnTgz(saveFilePath, filePath, &callbackFnc);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_UnTgz error[zip=%s], [unzip=%s], " HERE, saveFilePath, mapUpdInfo->installDirPath);
					// 広域地図ダウンロード
					updStatus->status = CC_MAPUPDSTATUS_MAPDL;
					CC_UpdDataProgressMng_SetStatus(CC_MAPUPDSTATUS_MAPDL);
					// 進捗情報保存
					if (e_SC_RESULT_SUCCESS != CC_UpdDataProgressMng_SaveUpdStatus()) {
						SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_UpdDataProgressMng_SaveUpdStatus, " HERE);
					}
					if (e_SC_RESULT_CANCEL != ret) {
						// tar.gzファイル削除
						remove(saveFilePath);
					}
					break;
				}
			}

			if (CC_MAPUPDSTATUS_MAPVERUPDATE >= updStatus->status) {
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"Map Version Update, " HERE);
				// 広域地図バージョン更新
				updStatus->status = CC_MAPUPDSTATUS_MAPVERUPDATE;
				CC_UpdDataProgressMng_SetStatus(CC_MAPUPDSTATUS_MAPVERUPDATE);
				// 進捗設定
				smProgressInfo.doneSize  = 1;
				smProgressInfo.totalSize = 1;
				sprintf(smProgressInfo.msg, "%s", SCC_CMN_PROGRESS_MSG_RGNMAP_VERSION);
				CC_UpdDataProgressMng_SetProgress(&smProgressInfo);
				// 進捗情報保存
				ret = CC_UpdDataProgressMng_SaveUpdStatus();
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_UpdDataProgressMng_SaveUpdStatus, " HERE);
					break;
				}

				if (!updStatus->hasUpdate[SCC_MAPDWL_KIND_DATA]) {
					// プロパティファイルのバージョンを現状のまま使用する
					if (NULL == *db) {
						// DBファイルのパス生成
						if ('/' == mapUpdInfo->installDirPath[strlen(mapUpdInfo->installDirPath) - 1]) {
							sprintf((char*)filePath, "%s%s%s/%s", mapUpdInfo->installDirPath, CC_CMN_MAP_DIR_PATH, mapUpdInfo->regionCode, SCC_CMN_DB_FILE_MAP);
						} else {
							sprintf((char*)filePath, "%s/%s%s/%s", mapUpdInfo->installDirPath, CC_CMN_MAP_DIR_PATH, mapUpdInfo->regionCode, SCC_CMN_DB_FILE_MAP);
						}

						if (0 != stat((const char*)filePath, &st)) {
							SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"no map filePath=%s, " HERE, filePath);
							ret = e_SC_RESULT_MAP_UPDATE_ERR;
							break;
						}
						// DAL初期化
						ret = SCC_DAL_Initialize(filePath, db);
						if (e_SC_RESULT_SUCCESS != ret) {
							SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Initialize error, " HERE);
							break;
						}
					}

					// トランザクション開始
					ret = SCC_DAL_Transaction(*db);
					if (e_SC_RESULT_SUCCESS != ret) {
						SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Transaction error, " HERE);
						break;
					}

					// DOWNLOAD_BASE_VERSIONテーブルの更新データ設定
					strcpy(dlBaseVer->kind, SCC_DAL_KIND_MGR);
					strcpy(dlBaseVer->tableName, SCC_DAL_KIND_DOWNLOAD_SYS);
					dlBaseVer->dlFlag = SCC_DAL_DL_FLAG_DOWNLOAD;
					dlBaseVer->baseVersion = appDataVersion;

					// DOWNLOAD_BASE_VERSIONテーブル更新
					ret = SCC_DAL_UpdDLBaseVersion(*db, dlBaseVer, 1, false);
					if (e_SC_RESULT_SUCCESS != ret) {
						SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_UpdDLBaseVersion error, " HERE);
						isRollback = true;
						break;
					}

					// DOWNLOAD_SYSテーブルの更新データ設定
					strcpy(dlSys.kind, SCC_DAL_KIND_OTHER);
					strcpy(dlSys.name, SCC_DAL_SYS_NAME_DATA);
					dlSys.version = appDataVersion;

					// DOWNLOAD_SYSテーブル更新
					ret = SCC_DAL_UpdDLSys(*db, &dlSys, 1, false);
					if (e_SC_RESULT_SUCCESS != ret) {
						SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_UpdDLBaseVersion error, " HERE);
						isRollback = true;
						break;
					}

					// コミット
					ret = SCC_DAL_Commit(*db);
					if (e_SC_RESULT_SUCCESS != ret) {
						isRollback = true;
						break;
					}
				}
				smProgressInfo.completeCount++;
				CC_UpdDataProgressMng_SetProgress(&smProgressInfo);
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"msg=%s, " HERE, smProgressInfo.msg);
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"doneSize=%lld, totalSize=%lld, completeCount=%u, totalCount=%u, " HERE,
								   smProgressInfo.doneSize, smProgressInfo.totalSize,
								   smProgressInfo.completeCount, smProgressInfo.totalCount);
				// 広域地図更新完了
				updStatus->status = CC_MAPUPDSTATUS_MAPUPDATED;
				CC_UpdDataProgressMng_SetStatus(CC_MAPUPDSTATUS_MAPUPDATED);
				// 進捗情報保存
				ret = CC_UpdDataProgressMng_SaveUpdStatus();
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_UpdDataProgressMng_SaveUpdStatus, " HERE);
					break;
				}
			}
			idx++;
		}
		// ダウンロードデータの保存先(端末)のパス生成
		sprintf(saveFilePath, "%s%s", dlTempDirPath, CC_UPDDATA_REGNMAP);
		// tar.gzファイル削除
		remove(saveFilePath);

		/*** プロパティファイル ***/
		if (updStatus->hasUpdate[SCC_MAPDWL_KIND_DATA]) {
			if (CC_MAPUPDSTATUS_DATADL >= updStatus->status) {
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"Data Download, " HERE);
				// ダウンロードデータのAWS上のパス生成
				sprintf(dlFilePath, "%s%s/%s%s", SCC_CMN_AWS_DIR_APPDATA, mapUpdInfo->regionCode, SCC_CMN_AWS_DIR_DATA, CC_UPDDATA_APPDATA);
				// ダウンロードデータの保存先(端末)のパス生成
				sprintf(saveFilePath, "%s%s", dlTempDirPath, CC_UPDDATA_APPDATA);
				// プロパティデータダウンロード
				updStatus->status = CC_MAPUPDSTATUS_DATADL;
				CC_UpdDataProgressMng_SetStatus(CC_MAPUPDSTATUS_DATADL);
				// 進捗設定
				if (0 == stat(saveFilePath, &st)) {
					smProgressInfo.doneSize = (INT64)st.st_size;
				} else {
					smProgressInfo.doneSize = 0;
				}
				smProgressInfo.totalSize = updData[idx].pressSize;
				sprintf(smProgressInfo.msg, "%s", SCC_CMN_PROGRESS_MSG_APPDATA_DL);
				CC_UpdDataProgressMng_SetProgress(&smProgressInfo);

				// コールバック関数ポインタ設定
				callbackFnc.progress = (SCC_PROGRESS_FNC)CC_UpdData_ProgressCallbackDL;

				startByte = smProgressInfo.doneSize;
				endByte   = smProgressInfo.totalSize;
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"startByte=%d - endByte=%d, " HERE, startByte, endByte);
				if (startByte < endByte) {
					// ダウンロード
					ret = CC_Download(smcal, apiParam, dlFilePath, saveFilePath, &callbackFnc, startByte, endByte);
					if (e_SC_RESULT_SUCCESS != ret) {
						SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_Download error[dl=%s], [save=%s], " HERE, dlFilePath, saveFilePath);
						break;
					}
				}
				// ダウンロードしたファイルをチェックする
				ret = CC_CmnDL_CheckFile(saveFilePath, updData[idx].md5, updData[idx].pressSize);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_CmnDL_CheckFile error[path=%s], " HERE, saveFilePath);
					if ((e_SC_RESULT_CANCEL != ret) && (e_SC_RESULT_SERVER_STOP != ret)) {
						// tar.gzファイル削除
						remove(saveFilePath);
					}
					break;
				}
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"msg=%s, " HERE, smProgressInfo.msg);
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"doneSize=%lld, totalSize=%lld, completeCount=%u, totalCount=%u, " HERE,
								   smProgressInfo.doneSize, smProgressInfo.totalSize,
								   smProgressInfo.completeCount, smProgressInfo.totalCount);
			}

			if (CC_MAPUPDSTATUS_DATAUPDATE >= updStatus->status) {
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"Data Update, " HERE);
				// tar.gzファイルのパス生成
				sprintf(saveFilePath, "%s%s", dlTempDirPath, CC_UPDDATA_APPDATA);
				// プロパティデータ更新
				updStatus->status = CC_MAPUPDSTATUS_DATAUPDATE;
				CC_UpdDataProgressMng_SetStatus(CC_MAPUPDSTATUS_DATAUPDATE);
				// 進捗設定
				smProgressInfo.doneSize  = 0;
				smProgressInfo.totalSize = updData[num].dataNum;
				sprintf(smProgressInfo.msg, "%s%s", updData[num].areaName, SCC_CMN_PROGRESS_MSG_AREAMAP_UPD);

				smProgressInfo.doneSize  = 0;
				smProgressInfo.totalSize = updData[idx].nonPressSize;
				sprintf(smProgressInfo.msg, "%s", SCC_CMN_PROGRESS_MSG_APPDATA_TARGZ);
				CC_UpdDataProgressMng_SetProgress(&smProgressInfo);
				// 進捗情報保存
				ret = CC_UpdDataProgressMng_SaveUpdStatus();
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_UpdDataProgressMng_SaveUpdStatus, " HERE);
					break;
				}

				// コールバック関数ポインタ設定
				callbackFnc.progress = (SCC_PROGRESS_FNC)CC_UpdData_ProgressCallbackTargz;

				// tar.gz解凍
				ret = CC_UnTgz(saveFilePath, mapUpdInfo->propertyPath, &callbackFnc);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_UnTgz error[zip=%s], [unzip=%s], " HERE, saveFilePath, mapUpdInfo->propertyPath);
					// プロパティデータダウンロード
					updStatus->status = CC_MAPUPDSTATUS_DATADL;
					CC_UpdDataProgressMng_SetStatus(CC_MAPUPDSTATUS_DATADL);
					// 進捗情報保存
					if (e_SC_RESULT_SUCCESS != CC_UpdDataProgressMng_SaveUpdStatus()) {
						SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_UpdDataProgressMng_SaveUpdStatus, " HERE);
					}
					if (e_SC_RESULT_CANCEL != ret) {
						// tar.gzファイル削除
						remove(saveFilePath);
					}
					break;
				}
			}
			if (CC_MAPUPDSTATUS_DATAVERUPDATE >= updStatus->status) {
				// プロパティデータバージョン更新
				updStatus->status = CC_MAPUPDSTATUS_DATAVERUPDATE;
				CC_UpdDataProgressMng_SetStatus(CC_MAPUPDSTATUS_DATAVERUPDATE);
				// 進捗情報保存
				ret = CC_UpdDataProgressMng_SaveUpdStatus();
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_UpdDataProgressMng_SaveUpdStatus, " HERE);
					break;
				}

				if (NULL == *db) {
					// DBファイルのパス生成
					if ('/' == mapUpdInfo->installDirPath[strlen(mapUpdInfo->installDirPath) - 1]) {
						sprintf((char*)filePath, "%s%s%s/%s", mapUpdInfo->installDirPath, CC_CMN_MAP_DIR_PATH, mapUpdInfo->regionCode, SCC_CMN_DB_FILE_MAP);
					} else {
						sprintf((char*)filePath, "%s/%s%s/%s", mapUpdInfo->installDirPath, CC_CMN_MAP_DIR_PATH, mapUpdInfo->regionCode, SCC_CMN_DB_FILE_MAP);
					}

					if (0 != stat((const char*)filePath, &st)) {
						SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"no map filePath=%s, " HERE, filePath);
						ret = e_SC_RESULT_MAP_UPDATE_ERR;
						break;
					}
					// DAL初期化
					ret = SCC_DAL_Initialize(filePath, db);
					if (e_SC_RESULT_SUCCESS != ret) {
						SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Initialize error, " HERE);
						break;
					}
				}

				// トランザクション開始
				ret = SCC_DAL_Transaction(*db);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Transaction error, " HERE);
					break;
				}

				// DOWNLOAD_BASE_VERSIONテーブルの更新データ設定
				strcpy(dlBaseVer->kind, SCC_DAL_KIND_MGR);
				strcpy(dlBaseVer->tableName, SCC_DAL_KIND_DOWNLOAD_SYS);
				dlBaseVer->dlFlag = SCC_DAL_DL_FLAG_DOWNLOAD;
				dlBaseVer->baseVersion = updData[idx].dataVersion;

				// 進捗設定
				smProgressInfo.doneSize  = 0;
				smProgressInfo.totalSize = 2;
				sprintf(smProgressInfo.msg, "%s", SCC_CMN_PROGRESS_MSG_RGNMAP_TARGZ);
				CC_UpdDataProgressMng_SetProgress(&smProgressInfo);

				// DOWNLOAD_BASE_VERSIONテーブル更新
				ret = SCC_DAL_UpdDLBaseVersion(*db, dlBaseVer, 1, false);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_UpdDLBaseVersion error, " HERE);
					isRollback = true;
					break;
				}

				// DOWNLOAD_SYSテーブルの更新データ設定
				strcpy(dlSys.kind, SCC_DAL_KIND_OTHER);
				strcpy(dlSys.name, SCC_DAL_SYS_NAME_DATA);
				dlSys.version = updData[idx].dataVersion;

				// 進捗設定
				smProgressInfo.doneSize  = 1;
				smProgressInfo.totalSize = 2;
				sprintf(smProgressInfo.msg, "%s", SCC_CMN_PROGRESS_MSG_RGNMAP_TARGZ);
				CC_UpdDataProgressMng_SetProgress(&smProgressInfo);

				// DOWNLOAD_SYSテーブル更新
				ret = SCC_DAL_UpdDLSys(*db, &dlSys, 1, false);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_UpdDLBaseVersion error, " HERE);
					isRollback = true;
					break;
				}

				// コミット
				ret = SCC_DAL_Commit(*db);
				if (e_SC_RESULT_SUCCESS != ret) {
					isRollback = true;
					break;
				}

				// 進捗設定
				smProgressInfo.doneSize = 2;
				smProgressInfo.completeCount++;
				CC_UpdDataProgressMng_SetProgress(&smProgressInfo);
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"msg=%s, " HERE, smProgressInfo.msg);
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"doneSize=%lld, totalSize=%lld, completeCount=%u, totalCount=%u, " HERE,
								   smProgressInfo.doneSize, smProgressInfo.totalSize,
								   smProgressInfo.completeCount, smProgressInfo.totalCount);
				// プロパティデータ更新完了
				updStatus->status = CC_MAPUPDSTATUS_DATAUPDATED;
				CC_UpdDataProgressMng_SetStatus(CC_MAPUPDSTATUS_DATAUPDATED);
				// 進捗情報保存
				ret = CC_UpdDataProgressMng_SaveUpdStatus();
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_UpdDataProgressMng_SaveUpdStatus, " HERE);
					break;
				}
			}
			idx++;
		}
		// ダウンロードデータの保存先(端末)のパス生成
		sprintf(saveFilePath, "%s%s", dlTempDirPath, CC_UPDDATA_APPDATA);
		// tar.gzファイル削除
		remove(saveFilePath);
	} while (0);

	if (isRollback) {
		// ロールバック
		if (e_SC_RESULT_SUCCESS != SCC_DAL_Rollback(*db)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Rollback error, " HERE);
		}
	}

	// ファイルクローズ
	if (NULL != fp) {
		fclose(fp);
	}

	// メモリ解放
	if (NULL != filePath) {
		SCC_FREE(filePath);
	}
	if (NULL != dlBaseVer) {
		SCC_FREE(dlBaseVer);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief エリア更新
 * @param[in] apiParam      APIパラメータ
 * @param[in] mapUpdInfo    データ更新情報
 * @param[in] dlTempDirPath ダウンロードテンポラリフォルダ
 * @param[in] callbackFnc   コールバック関数ポインタ
 * @param[in] smcal         SMCAL
 * @param[in] dlFilePath    ダウンロードファイルパス格納用バッファ
 * @param[in] saveFilePath  ダウンロードファイル保存先パス格納用バッファ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_UpdArea(SMCAL *smcal,
					   T_CC_CMN_SMS_API_PRM *apiParam,
					   const SMUPDINFO *mapUpdInfo,
					   const Char *dlTempDirPath,
					   const SCC_UPDATEDATA *updData,
					   SCC_CANCEL_FNC cancelCBFnc,
					   Char *dlFilePath,
					   Char *saveFilePath,
					   DBOBJECT **db)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	SMPROGRESSCBFNC	callbackFnc = {};
	Char	*filePath = NULL;
	SCC_DOWNLOADAREAMAP	*dlAreaMap = NULL;
	Bool	isRollback = false;
	upDTcheck_States	updStatus[2] = {SCC_MAPDWL_CHECK_NONE};
	struct	stat st = {};

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// メモリ確保
		filePath = (Char*)SCC_MALLOC(SCC_MAX_PATH);
		if (NULL == filePath) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		dlAreaMap = (SCC_DOWNLOADAREAMAP*)SCC_MALLOC(sizeof(SCC_DOWNLOADAREAMAP));
		if (NULL == dlAreaMap) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		if (NULL == *db) {
			// DBファイルのパス生成
			if ('/' == mapUpdInfo->installDirPath[strlen(mapUpdInfo->installDirPath) - 1]) {
				sprintf((char*)filePath, "%s%s%s/%s", mapUpdInfo->installDirPath, CC_CMN_MAP_DIR_PATH, mapUpdInfo->regionCode, SCC_CMN_DB_FILE_MAP);
			} else {
				sprintf((char*)filePath, "%s/%s%s/%s", mapUpdInfo->installDirPath, CC_CMN_MAP_DIR_PATH, mapUpdInfo->regionCode, SCC_CMN_DB_FILE_MAP);
			}

			if (0 == stat((const char*)filePath, &st)) {
				// DAL初期化
				ret = SCC_DAL_Initialize(filePath, db);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Initialize error, " HERE);
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"path=%s, " HERE, filePath);
					break;
				}
			}
		}

		// コールバック関数ポインタ設定
		callbackFnc.cancel   = (SCC_CANCEL_FNC)cancelCBFnc;

		// エリア単位の地図ダウンロード
		for (num = 0; num < mapUpdInfo->updateAreaNum; num++) {
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"area[%d] Update, " HERE, mapUpdInfo->updateArea[num]);
			// キャンセルチェック
			if ((NULL != cancelCBFnc) && (cancelCBFnc())) {
				SCC_LOG_WarnPrint(SC_TAG_CC, (Char*)"cancel, " HERE);
				ret = e_SC_RESULT_CANCEL;
				break;
			}

			// 初期化
			smProgressInfo.doneSize  = 0;
			smProgressInfo.totalSize = 0;

			// バージョンチェック
			ret = CC_GetAreaInfoList_CheckVersion(updData, *db, mapUpdInfo->updateArea[num], &updStatus[0]);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetAreaInfoList_CheckVersion error, " HERE);
				break;
			}
			// バージョンチェック
			ret = CC_GetAreaInfoList_CheckVersion(&updData[mapUpdInfo->updateAreaNum + num], *db, mapUpdInfo->updateArea[num], &updStatus[1]);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetAreaInfoList_CheckVersion error, " HERE);
				break;
			}
			if (SCC_MAPDWL_CHECK_NONE == updStatus[0]) {
				// 更新済み
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"area[%d] update none, " HERE, mapUpdInfo->updateArea[num]);
				smProgressInfo.doneSize  = updData[num].pressSize;
				smProgressInfo.totalSize = updData[num].pressSize;
			}
			if (SCC_MAPDWL_CHECK_NONE == updStatus[1]) {
				// 更新済み
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"area[%d] update none, " HERE, mapUpdInfo->updateArea[num]);
				smProgressInfo.doneSize  += updData[mapUpdInfo->updateAreaNum + num].pressSize;
				smProgressInfo.totalSize += updData[mapUpdInfo->updateAreaNum + num].pressSize;
			}

			if ((SCC_MAPDWL_CHECK_NONE == updStatus[0]) && (SCC_MAPDWL_CHECK_NONE == updStatus[1])) {
				// 更新済み
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"area[%d] update none, " HERE, mapUpdInfo->updateArea[num]);
				smProgressInfo.completeCount++;
				sprintf(smProgressInfo.msg, "%s%s", updData[num].areaName, SCC_CMN_PROGRESS_MSG_AREAMAP_UPD);
				CC_UpdDataProgressMng_SetProgress(&smProgressInfo);
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"msg=%s, " HERE, smProgressInfo.msg);
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"doneSize=%lld, totalSize=%lld, completeCount=%u, totalCount=%u, " HERE,
								   smProgressInfo.doneSize, smProgressInfo.totalSize,
								   smProgressInfo.completeCount, smProgressInfo.totalCount);
				// 更新中のDL地域コードリストのインデックス
				CC_UpdDataProgressMng_SetAreaIdx(num + 1);
				continue;
			} else if ((SCC_MAPDWL_CHECK_NO_DATA == updStatus[0]) || (SCC_MAPDWL_CHECK_NO_DATA == updStatus[1]) ||
				(SCC_MAPDWL_CHECK_APPUPDATE_NEED == updStatus[0]) || (SCC_MAPDWL_CHECK_APPUPDATE_NEED == updStatus[1])) {
				ret = e_SC_RESULT_MAP_UPDATE_ERR;
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"updStatus error(updStatus=%d), " HERE, updStatus);
				break;
			}

			// トランザクション開始
			ret = SCC_DAL_Transaction(*db);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Transaction error, " HERE);
				break;
			}

			if (SCC_MAPDWL_CHECK_NONE != updStatus[0]) {
				// ダウンロードデータのAWS上のパス生成
				sprintf(dlFilePath,
						"%s%s/%slocalMap%03d.tar.gz",
						SCC_CMN_AWS_DIR_APPDATA,
						mapUpdInfo->regionCode,
						SCC_CMN_AWS_DIR_MAP,
						mapUpdInfo->updateArea[num]);
				// ダウンロードデータの保存先(端末)のパス生成
				sprintf(saveFilePath, "%slocalMap%03d.tar.gz", dlTempDirPath, mapUpdInfo->updateArea[num]);

				// エリア地図ダウンロード
				CC_UpdDataProgressMng_SetStatus(CC_MAPUPDSTATUS_AREADL);
				// 進捗設定
				smProgressInfo.doneSize  = 0;
				smProgressInfo.totalSize = updData[num].pressSize;
				sprintf(smProgressInfo.msg, "%s%s", updData[num].areaName, SCC_CMN_PROGRESS_MSG_AREAMAP_DL);
				CC_UpdDataProgressMng_SetProgress(&smProgressInfo);
				// コールバック関数ポインタ設定
				callbackFnc.progress = (SCC_PROGRESS_FNC)CC_UpdData_ProgressCallbackDL;

				// ダウンロード
				ret = CC_Download(smcal, apiParam, dlFilePath, saveFilePath, &callbackFnc, 0, updData[num].pressSize);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_Download error[dl=%s], [save=%s], " HERE, dlFilePath, saveFilePath);
					// tar.gzファイル削除
					remove(saveFilePath);
					break;
				}
				// ダウンロードしたファイルをチェックする
				ret = CC_CmnDL_CheckFile(saveFilePath, updData[num].md5, updData[num].pressSize);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_CmnDL_CheckFile error[path=%s], " HERE, saveFilePath);
					if ((e_SC_RESULT_CANCEL != ret) && (e_SC_RESULT_SERVER_STOP != ret)) {
						// tar.gzファイル削除
						remove(saveFilePath);
					}
					break;
				}
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"msg=%s, " HERE, smProgressInfo.msg);
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"doneSize=%lld, totalSize=%lld, completeCount=%u, totalCount=%u, " HERE,
								   smProgressInfo.doneSize, smProgressInfo.totalSize,
								   smProgressInfo.completeCount, smProgressInfo.totalCount);

				// エリア地図解凍
				CC_UpdDataProgressMng_SetStatus(CC_MAPUPDSTATUS_AREATARGZ);
				// 進捗設定
				smProgressInfo.doneSize  = 0;
				smProgressInfo.totalSize = updData[num].dataNum;
				sprintf(smProgressInfo.msg, "%s%s", updData[num].areaName, SCC_CMN_PROGRESS_MSG_AREAMAP_UPD);
				CC_UpdDataProgressMng_SetProgress(&smProgressInfo);
				// コールバック関数ポインタ設定
				callbackFnc.progress = NULL;

				// tar.gz解凍
				ret = CC_UnTgz(saveFilePath, dlTempDirPath, &callbackFnc);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_UnTgz error[zip=%s], [unzip=%s], " HERE, saveFilePath, mapUpdInfo->installDirPath);
					// tar.gzファイル削除
					remove(saveFilePath);
					break;
				}

				// エリア地図更新
				CC_UpdDataProgressMng_SetStatus(CC_MAPUPDSTATUS_AREAUPDATE);
				CC_UpdDataProgressMng_SetProgress(&smProgressInfo);
				// コールバック関数ポインタ設定
				callbackFnc.progress = (SCC_PROGRESS_FNC)CC_UpdData_ProgressCallbackDB;

				// tar.gzファイル削除
				remove(saveFilePath);

				// エリア単位の地図のCSVファイルパス生成
				sprintf((char*)filePath, "%s%s%s/%03d_parcel.csv", dlTempDirPath, CC_CMN_MAP_DIR_PATH, mapUpdInfo->regionCode, mapUpdInfo->updateArea[num]);

				// パーセルテーブル更新
				ret = SCC_DAL_UpdateParcel(*db, filePath, false, &callbackFnc);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_UpdateParcel error(%s), " HERE, filePath);
					isRollback = true;
					remove(filePath);
					break;
				}
				remove(filePath);

				// エリア地図バージョン更新
				CC_UpdDataProgressMng_SetStatus(CC_MAPUPDSTATUS_AREAVERUPDATE);
				// エリア地図データダウンロード管理テーブルの更新データ設定
				dlAreaMap->areaCode = mapUpdInfo->updateArea[num];
				strcpy(dlAreaMap->kind, SCC_DAL_KIND_PARCEL);
				dlAreaMap->dlFlag = SCC_DAL_DL_FLAG_DOWNLOAD;
				dlAreaMap->baseVersion = updData[num].dataVersion;

				// 進捗設定
				smProgressInfo.doneSize  = 0;
				smProgressInfo.totalSize = 1;
				sprintf(smProgressInfo.msg, "%s%s", updData[num].areaName, SCC_CMN_PROGRESS_MSG_AREAMAP_UPD);
				CC_UpdDataProgressMng_SetProgress(&smProgressInfo);

				// エリア地図データダウンロード管理テーブル更新
				ret = SCC_DAL_UpdDLAreaMap(*db, dlAreaMap, 1, false);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_UpdDLBaseVersion error, " HERE);
					isRollback = true;
					remove(filePath);
					break;
				}

				// 進捗設定
				smProgressInfo.doneSize = 1;
				CC_UpdDataProgressMng_SetProgress(&smProgressInfo);
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"msg=%s, " HERE, smProgressInfo.msg);
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"doneSize=%lld, totalSize=%lld, completeCount=%u, totalCount=%u, " HERE,
								   smProgressInfo.doneSize, smProgressInfo.totalSize,
								   smProgressInfo.completeCount, smProgressInfo.totalCount);

				// エリア単位の地図のCSVファイル削除
				remove(filePath);
			}

			if (SCC_MAPDWL_CHECK_NONE != updStatus[1]) {
				// ダウンロードデータのAWS上のパス生成
				sprintf(dlFilePath,
						"%s%s/%sareaCls%03d.tar.gz",
						SCC_CMN_AWS_DIR_APPDATA,
						mapUpdInfo->regionCode,
						SCC_CMN_AWS_DIR_MAP,
						mapUpdInfo->updateArea[num]);
				// ダウンロードデータの保存先(端末)のパス生成
				sprintf(saveFilePath, "%sareaCls%03d.tar.gz", dlTempDirPath, mapUpdInfo->updateArea[num]);

				// 地域クラスデータダウンロード
				CC_UpdDataProgressMng_SetStatus(CC_MAPUPDSTATUS_AREACLSDL);
				// 進捗設定
				smProgressInfo.doneSize  = 0;
				smProgressInfo.totalSize = updData[mapUpdInfo->updateAreaNum + num].pressSize;
				sprintf(smProgressInfo.msg, "%s%s", updData[mapUpdInfo->updateAreaNum + num].areaName, SCC_CMN_PROGRESS_MSG_AREACLSMAP_DL);
				CC_UpdDataProgressMng_SetProgress(&smProgressInfo);
				// コールバック関数ポインタ設定
				callbackFnc.progress = (SCC_PROGRESS_FNC)CC_UpdData_ProgressCallbackDL;

				// ダウンロード
				ret = CC_Download(smcal, apiParam, dlFilePath, saveFilePath, &callbackFnc, 0, updData[mapUpdInfo->updateAreaNum + num].pressSize);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_Download error[dl=%s], [save=%s], " HERE, dlFilePath, saveFilePath);
					// tar.gzファイル削除
					remove(saveFilePath);
					break;
				}
				// ダウンロードしたファイルをチェックする
				ret = CC_CmnDL_CheckFile(saveFilePath, updData[mapUpdInfo->updateAreaNum + num].md5, updData[mapUpdInfo->updateAreaNum + num].pressSize);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_CmnDL_CheckFile error[path=%s], " HERE, saveFilePath);
					if ((e_SC_RESULT_CANCEL != ret) && (e_SC_RESULT_SERVER_STOP != ret)) {
						// tar.gzファイル削除
						remove(saveFilePath);
					}
					break;
				}
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"msg=%s, " HERE, smProgressInfo.msg);
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"doneSize=%lld, totalSize=%lld, completeCount=%u, totalCount=%u, " HERE,
								   smProgressInfo.doneSize, smProgressInfo.totalSize,
								   smProgressInfo.completeCount, smProgressInfo.totalCount);

//				// 地域クラスデータ解凍
//				CC_UpdDataProgressMng_SetStatus(CC_MAPUPDSTATUS_AREACLSTARGZ);
//				// 進捗設定
//				smProgressInfo.doneSize  = 0;
//				smProgressInfo.totalSize = updData[mapUpdInfo->updateAreaNum + num].dataNum;
//				sprintf(smProgressInfo.msg, "%s%s", updData[mapUpdInfo->updateAreaNum + num].areaName, SCC_CMN_PROGRESS_MSG_AREACLSMAP_UPD);
//				CC_UpdDataProgressMng_SetProgress(&smProgressInfo);
				// コールバック関数ポインタ設定
				callbackFnc.progress = NULL;

				// tar.gz解凍
				ret = CC_UnTgz(saveFilePath, dlTempDirPath, &callbackFnc);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_UnTgz error[zip=%s], [unzip=%s], " HERE, saveFilePath, mapUpdInfo->installDirPath);
					// tar.gzファイル削除
					remove(saveFilePath);
					break;
				}

//				// 地域クラス更新
//				CC_UpdDataProgressMng_SetStatus(CC_MAPUPDSTATUS_AREACLSUPDATE);
//				CC_UpdDataProgressMng_SetProgress(&smProgressInfo);
//				// コールバック関数ポインタ設定
//				callbackFnc.progress = (SCC_PROGRESS_FNC)CC_UpdData_ProgressCallbackDB;

				// tar.gzファイル削除
				remove(saveFilePath);

				// エリア単位の地図のCSVファイルパス生成
				sprintf((char*)filePath, "%s%s%s/%03d_area_cls.csv", dlTempDirPath, CC_CMN_MAP_DIR_PATH, mapUpdInfo->regionCode, mapUpdInfo->updateArea[num]);

				// 地域クラステーブル更新
				ret = SCC_DAL_UpdateAreaCls(*db, filePath, false, &callbackFnc);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_UpdateAreaCls error(%s), " HERE, filePath);
					isRollback = true;
					remove(filePath);
					break;
				}
				remove(filePath);

				if (SCC_MAPDWL_CHECK_NONE == updStatus[0]) {
//					// 地域クラス地図バージョン更新
//					CC_UpdDataProgressMng_SetStatus(CC_MAPUPDSTATUS_AREACLSVERUPDATE);
					// エリア地図データダウンロード管理テーブルの更新データ設定
					dlAreaMap->areaCode = mapUpdInfo->updateArea[num];
					strcpy(dlAreaMap->kind, SCC_DAL_KIND_PARCEL);
					dlAreaMap->dlFlag = SCC_DAL_DL_FLAG_DOWNLOAD;
					dlAreaMap->baseVersion = updData[mapUpdInfo->updateAreaNum + num].dataVersion;

//					// 進捗設定
//					smProgressInfo.doneSize  = 0;
//					smProgressInfo.totalSize = 1;
//					sprintf(smProgressInfo.msg, "%s%s", updData[num].areaName, SCC_CMN_PROGRESS_MSG_AREACLSMAP_UPD);
//					CC_UpdDataProgressMng_SetProgress(&smProgressInfo);

					// エリア地図データダウンロード管理テーブル更新
					ret = SCC_DAL_UpdDLAreaMap(*db, dlAreaMap, 1, false);
					if (e_SC_RESULT_SUCCESS != ret) {
						SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_UpdDLBaseVersion error, " HERE);
						isRollback = true;
						remove(filePath);
						break;
					}
				}
				// エリア単位の地図のCSVファイル削除
				remove(filePath);
			}

			// コミット
			ret = SCC_DAL_Commit(*db);
			if (e_SC_RESULT_SUCCESS != ret) {
				isRollback = true;
				break;
			}

			// 進捗設定
			smProgressInfo.doneSize = 1;
			smProgressInfo.completeCount++;
			CC_UpdDataProgressMng_SetProgress(&smProgressInfo);
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"msg=%s, " HERE, smProgressInfo.msg);
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"doneSize=%lld, totalSize=%lld, completeCount=%u, totalCount=%u, " HERE,
							   smProgressInfo.doneSize, smProgressInfo.totalSize,
							   smProgressInfo.completeCount, smProgressInfo.totalCount);

			// 更新中のDL地域コードリストのインデックス
			CC_UpdDataProgressMng_SetAreaIdx(num + 1);
		}
	} while(0);

	if (isRollback) {
		// ロールバック
		if (e_SC_RESULT_SUCCESS != SCC_DAL_Rollback(*db)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Rollback error, " HERE);
		}
	}

	// メモリ解放
	if (NULL != filePath) {
		SCC_FREE(filePath);
	}
	if (NULL != dlAreaMap) {
		SCC_FREE(dlAreaMap);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief application.ini更新関数
 * @param[in] mapUpdInfo    データ更新情報
 */
void CC_UpdData_UpdApplicationIni(const SMUPDINFO *mapUpdInfo)
{
	Char	*filePath = NULL;
	SMRGNSETTING *rgn_setting = NULL;
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	UINT16	cnt = 0;
	Bool	restored = false;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {

		// メモリ確保
		filePath = (Char*)SCC_MALLOC(SCC_MAX_PATH);
		if (NULL == filePath) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// 地図DBのルートパス生成
		if ('/' == mapUpdInfo->installDirPath[strlen(mapUpdInfo->installDirPath) - 1]) {
			sprintf((char*)filePath, "%s%s%s", mapUpdInfo->installDirPath, CC_CMN_MAP_DIR_PATH, mapUpdInfo->regionCode);
		} else {
			sprintf((char*)filePath, "%s/%s%s", mapUpdInfo->installDirPath, CC_CMN_MAP_DIR_PATH, mapUpdInfo->regionCode);
		}

		// メモリ確保
		rgn_setting = (SMRGNSETTING*)SCC_MALLOC(sizeof(SMRGNSETTING));
		if (NULL == rgn_setting) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// application.iniファイルリード
		ret = CC_Read_application_ini(rgn_setting, true, &restored);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_Read_application_ini() error, " HERE);
			break;
		}

		// 地図DB未ダウンロードの場合（初期状態）
		if(0 == rgn_setting->folder_num){
			strcpy(rgn_setting->dt_Folder[0].Region, mapUpdInfo->regionCode);
			strcpy(rgn_setting->dt_Folder[0].folder_Path, (const char*)filePath);
			rgn_setting->folder_num++;
			CC_Write_application_ini(rgn_setting);
		}
		// １回でも地図DBをダウンロードしてある場合
		else{
			for(cnt = 0; rgn_setting->folder_num > cnt; cnt++){
				// 当該仕向けの地図DBが既に有る場合、ループを抜ける
				if(0 == strcmp(rgn_setting->dt_Folder[cnt].Region, mapUpdInfo->regionCode)){
					break;
				}
			}
			// 当該仕向けの地図DBが存在しない場合
			if((rgn_setting->folder_num == cnt) && (SCC_MAPDWL_MAXNUM_DLRGN > cnt)){
				// 当該仕向けの地図DB格納先パスを書込み
				strcpy(rgn_setting->dt_Folder[cnt].Region, mapUpdInfo->regionCode);
				strcpy(rgn_setting->dt_Folder[cnt].folder_Path, (const char*)filePath);
				rgn_setting->folder_num++;
				CC_Write_application_ini(rgn_setting);
			}
		}

	} while(0);

	// メモリ解放
	if (NULL != filePath) {
		SCC_FREE(filePath);
	}
	if (NULL != rgn_setting) {
		SCC_FREE(rgn_setting);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}
