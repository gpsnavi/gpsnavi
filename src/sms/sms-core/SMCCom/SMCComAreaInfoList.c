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

static E_SC_RESULT CC_DLVersionInfoDB(SMCAL *smcal, T_CC_CMN_SMS_API_PRM *apiParam, const Char *rgn_code, const Char *dlTempDirPath, const SMPROGRESSCBFNC *callbackFnc, Char *dlFilePath, Char *saveFilePath);

static Bool isRgnMapUpdate = false;

/**
 * @brief 地域情報リスト取得
 * @param[in]     smcal         SMCAL
 * @param[in]     parm          APIパラメータ
 * @param[in]     rgn_code      仕向け地コード
 * @param[out]    sectInf       地方情報リスト
 * @param[in/out] sectNum       地方数（in時：MAX値  out時：取得した実データの地方数）
 * @param[in/out] areaNum       地域数（in時：MAX値  out時：取得した実データの地域数）
 * @param[in]     dlTempDirPath ダウンロードテンポラリフォルダ
 * @param[in]     callbackFnc   コールバック関数情報
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_RESULT CC_GetAreaInfoList(SMCAL *smcal,
								T_CC_CMN_SMS_API_PRM *parm,
								const Char *rgn_code,
								SMSECINFO *sectInf,
								INT32 *sectNum,
								INT32 *areaNum,
								const Char *dlTempDirPath,
								const SMMAPDLCBFNC *dlCBFnc)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	Char	*dlFilePath = NULL;
	Char	*saveFilePath = NULL;
	Char	*filePath = NULL;
	Char	*map_filePath = NULL;
	DBOBJECT *db = NULL;
	DBOBJECT *map_db = NULL;
	INT32	num = 0;
	SCC_AREAINFO *areaInf = NULL;
	SCC_UPDATEDATA *updateDT = NULL;
	SCC_UPDATEDATA *updateDTAreaCls = NULL;
	SMRGNSETTING rgnSetting = {};
	INT32 areaInfoNum = 0;
	INT32 sect_cnt = 0;
	INT32 area_cnt = 0;
	SMPROGRESSCBFNC	callbackFnc = {};
	struct	stat st = {};
	Bool	restored = false;
	upDTcheck_States	updateStatus = SCC_MAPDWL_CHECK_NONE;

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
		filePath = (Char*)SCC_MALLOC(SCC_MAX_PATH);
		if (NULL == filePath) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// コールバック関数のポインタ設定
		callbackFnc.cancel   = (SCC_CANCEL_FNC)dlCBFnc->cancel;
		callbackFnc.progress = NULL;

		// バージョン情報DBファイルのパス生成
		if ('/' == dlTempDirPath[strlen(dlTempDirPath) - 1]) {
			sprintf((char*)filePath, "%s%s", dlTempDirPath, SCC_CMN_DB_FILE_DATAVERSIONINFO);
		} else {
			sprintf((char*)filePath, "%s/%s", dlTempDirPath, SCC_CMN_DB_FILE_DATAVERSIONINFO);
		}
		// パーミッション変更
		chmod(filePath, 0666);
		// tar.gz解凍後のファイルが存在する場合は、ダウンロード前に削除する
		remove(filePath);

		// バージョン情報DB(dataVersionInfo.db)ダウンロード
		ret = CC_DLVersionInfoDB(smcal, parm, rgn_code, dlTempDirPath, &callbackFnc, dlFilePath, saveFilePath);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_DLVersionInfoDB error, " HERE);
			remove(filePath);
			break;
		}

		// バージョン情報DBのDAL初期化
		ret = SCC_DAL_Initialize(filePath, &db);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Initialize error, " HERE);
			remove(filePath);
			break;
		}
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"Version_Info_DB file path = %s, " HERE, filePath);

		// メモリ確保
		map_filePath = (Char*)SCC_MALLOC(SCC_MAX_PATH);
		if (NULL == map_filePath) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			remove(filePath);
			break;
		}
		map_filePath[0] = EOS;

		// 仕向け設定情報取得
		ret = CC_Read_application_ini(&rgnSetting, true, &restored);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_Read_application_ini error, " HERE);
			remove(filePath);
			break;
		}

		// 地図DBファイルパス生成
		for(num = 0; rgnSetting.folder_num > num; num++){
			if(0 == strcmp(rgn_code, rgnSetting.dt_Folder[num].Region)){
				if ('/' == rgnSetting.dt_Folder[num].Region[strlen(rgnSetting.dt_Folder[num].Region) - 1]) {
					sprintf((char*)map_filePath, "%s%s", rgnSetting.dt_Folder[num].folder_Path, SCC_CMN_DB_FILE_MAP);
				} else {
					sprintf((char*)map_filePath, "%s/%s", rgnSetting.dt_Folder[num].folder_Path, SCC_CMN_DB_FILE_MAP);
				}
				break;
			}
		}

		// 地図DBファイルパス有の場合
		if(EOS != map_filePath[0]){
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"Map_DB file Path = %s, " HERE, map_filePath);
			// Polaris.db有無チェック
			if (0 != stat((const char*)map_filePath, &st)) {
				// エラーでも処理は継続する
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"%s not found, " HERE, SCC_CMN_DB_FILE_MAP);
				ret2 = e_SC_RESULT_NOT_FOUND_MAP;
			} else {
				// 地図DBのDAL初期化
				ret2 = SCC_DAL_Initialize(map_filePath, &map_db);
				if (e_SC_RESULT_SUCCESS != ret2) {
					// エラーでも処理は継続する
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Initialize error, " HERE);
					SCC_DAL_Finalize(map_db);
					map_db = NULL;
					ret2 = e_SC_RESULT_NOT_FOUND_MAP;
				}
			}
		}

		if (NULL != db) {
			// メモリ確保
			areaInf = (SCC_AREAINFO*)SCC_MALLOC(sizeof(SCC_AREAINFO) * (*areaNum));
			if (NULL == areaInf) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
				ret = e_SC_RESULT_MALLOC_ERR;
				remove(filePath);
				break;
			}

			// AREA_INFOテーブル情報取得
			ret = SCC_DAL_GetAreaInfoList(db, *areaNum, areaInf, &areaInfoNum);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_GetAreaInfoList error, " HERE);
				remove(filePath);
				break;
			}
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"SCC_DAL_GetAreaInfoList() areaInf_count = %d, " HERE, areaInfoNum);

//*****************************************************************
#if 0
INT32 area_cnt;
SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"areaNum:%d, " HERE, areaInfoNum);
for(area_cnt = 0; areaInfoNum > area_cnt; area_cnt++){
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"SectCD:%d  SectNM:%s  AreaCD:%d  AreaNM:%s, " HERE, areaInf[area_cnt].sectionCode,  areaInf[area_cnt].sectionName,  areaInf[area_cnt].areaCode,  areaInf[area_cnt].areaName);
}
#endif
//*****************************************************************

			// メモリ確保
			updateDT = (SCC_UPDATEDATA*)SCC_MALLOC(sizeof(SCC_UPDATEDATA));
			if (NULL == updateDT) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
				ret = e_SC_RESULT_MALLOC_ERR;
				remove(filePath);
				break;
			}
			updateDTAreaCls = (SCC_UPDATEDATA*)SCC_MALLOC(sizeof(SCC_UPDATEDATA));
			if (NULL == updateDTAreaCls) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
				ret = e_SC_RESULT_MALLOC_ERR;
				remove(filePath);
				break;
			}

			for(num = 0, sect_cnt = 0, area_cnt = 0; areaInfoNum > num; num++, area_cnt++){

				//***** 地方情報設定 (地域情報が地方毎に纏まっていることが前提) *****
				if(0 == num){
					// 地方コード設定
					sectInf[sect_cnt].sectionCode = areaInf[num].sectionCode;
					// 地方名称設定
					strcpy(sectInf[sect_cnt].sectionName, areaInf[num].sectionName);
				}
				else if((0 < num) && (areaInf[num - 1].sectionCode != areaInf[num].sectionCode)){
					// 地方数カウンタ更新前の地方情報に地域数設定
					sectInf[sect_cnt].area_num = area_cnt;
					// 地方数カウンタ更新
					sect_cnt++;
					// 地方コード設定
					sectInf[sect_cnt].sectionCode = areaInf[num].sectionCode;
					// 地方名称設定
					strcpy(sectInf[sect_cnt].sectionName, areaInf[num].sectionName);
					// 地方毎の地域数カウンタクリア
					area_cnt = 0;
				}

				//***** 地域情報設定 *****
				// 地方コード設定
				sectInf[sect_cnt].areaInfo[area_cnt].sectionCode = areaInf[num].sectionCode;
				// 地域コード設定
				sectInf[sect_cnt].areaInfo[area_cnt].areaCode = areaInf[num].areaCode;
				// 地域名称設定
				strcpy(sectInf[sect_cnt].areaInfo[area_cnt].areaName, areaInf[num].areaName);

				//***** 検索条件設定 *****
				// 地域コード設定
				updateDT->areaCode = sectInf[sect_cnt].areaInfo[area_cnt].areaCode;
				// DATA_TYPE設定
				updateDT->dataType = SCC_DAL_UPDDATA_DATA_TYPE_AREA_MAP;
				// DATA_KIND設定
				strcpy(updateDT->dataKind, SCC_DAL_KIND_PARCEL);

				// UPDATE_DATAテーブル情報取得
				ret = SCC_DAL_GetUpdateData(db, updateDT);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_GetUpdateData error, " HERE);
					break;
				}

				// 地域コード設定
				updateDTAreaCls->areaCode = sectInf[sect_cnt].areaInfo[area_cnt].areaCode;
				// DATA_TYPE設定
				updateDTAreaCls->dataType = SCC_DAL_UPDDATA_DATA_TYPE_AREA_MAP;
				// DATA_KIND設定
				strcpy(updateDTAreaCls->dataKind, SCC_DAL_KIND_AREA_CLS);

				// UPDATE_DATAテーブル情報取得
				ret = SCC_DAL_GetUpdateData(db, updateDTAreaCls);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_GetUpdateData error, " HERE);
					break;
				}

				// 一時ファイルサイズ設定(単位 Mbyte)
				sectInf[sect_cnt].areaInfo[area_cnt].tempFileSize = ((((updateDT->nonPressSize * 3) + (SCC_SIZE_MB - 1)) / SCC_SIZE_MB) +
																	 (((updateDTAreaCls->nonPressSize * 3) + (SCC_SIZE_MB - 1)) / SCC_SIZE_MB));

				// インポート後サイズ設定(単位 Mbyte)
				sectInf[sect_cnt].areaInfo[area_cnt].importSize = (((updateDT->nonPressSize + (SCC_SIZE_MB - 1)) / SCC_SIZE_MB) +
																   ((updateDTAreaCls->nonPressSize + (SCC_SIZE_MB - 1)) / SCC_SIZE_MB));

				// 更新有無チェック(PARCEL)
				ret = CC_GetAreaInfoList_CheckVersion(updateDT, map_db, sectInf[sect_cnt].areaInfo[area_cnt].areaCode, &sectInf[sect_cnt].areaInfo[area_cnt].updateStatus);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetAreaInfoList_CheckVersion error(areaCode=%d), " HERE, sectInf[sect_cnt].areaInfo[area_cnt].areaCode);
					break;
				}
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"areaCode=%d, updateStatus=%d, " HERE, sectInf[sect_cnt].areaInfo[area_cnt].areaCode, sectInf[sect_cnt].areaInfo[area_cnt].updateStatus);

				// 更新有無チェック(AREA_CLS)
				ret = CC_GetAreaInfoList_CheckVersion(updateDTAreaCls, map_db, sectInf[sect_cnt].areaInfo[area_cnt].areaCode, &updateStatus);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetAreaInfoList_CheckVersion error(areaCode=%d), " HERE, sectInf[sect_cnt].areaInfo[area_cnt].areaCode);
					break;
				}
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"areaCode=%d, updateStatus=%d, " HERE, sectInf[sect_cnt].areaInfo[area_cnt].areaCode, updateStatus);

				// ステータスはマージしたものを設定する
				// 最優度は、「更新必須」、「更新任意」の順
				if ((SCC_MAPDWL_CHECK_NEED == sectInf[sect_cnt].areaInfo[area_cnt].updateStatus) ||
					(SCC_MAPDWL_CHECK_NEED == updateStatus)) {
					updateStatus = SCC_MAPDWL_CHECK_NEED;
				} else if (SCC_MAPDWL_CHECK_NONE == sectInf[sect_cnt].areaInfo[area_cnt].updateStatus) {
					updateStatus = SCC_MAPDWL_CHECK_NONE;
				} else {
					updateStatus = sectInf[sect_cnt].areaInfo[area_cnt].updateStatus;
				}
				//*******    ダウンロードデータ情報もここで作成する   ******************
			}

			if(e_SC_RESULT_SUCCESS != ret){
				remove(filePath);
				break;
			}

			// 最後の地方情報に地域数を設定
			if((0 < num) && (areaInfoNum == num)){
				sectInf[sect_cnt].area_num = area_cnt;
			}

			// 地方情報数
			*sectNum = sect_cnt + 1;									// sect_cntが0開始のため+1する
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"SCC_DAL_GetAreaInfoList() sectInf_count = %d, " HERE, *sectNum);
			// 地域情報数
			*areaNum = areaInfoNum;
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"SCC_DAL_GetAreaInfoList() areaInf_count = %d, " HERE, *areaNum);
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
	if (NULL != map_filePath) {
		SCC_FREE(map_filePath);
	}
	if (NULL != areaInf) {
		SCC_FREE(areaInf);
	}
	if (NULL != updateDT) {
		SCC_FREE(updateDT);
	}
	if (NULL != updateDTAreaCls) {
		SCC_FREE(updateDTAreaCls);
	}

	// DAL終了化
	if (NULL != db) {
		SCC_DAL_Finalize(db);
	}
	if (NULL != map_db) {
		SCC_DAL_Finalize(map_db);
	}

	if (e_SC_RESULT_SUCCESS == ret) {
		ret = ret2;
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief 更新対象の地域情報取得
 * @param[in]     smcal             SMCAL
 * @param[in]     parm              APIパラメータ
 * @param[in]     mapUpdInfo        データ更新情報
 * @param[in]     dlTempDirPath     ダウンロードテンポラリフォルダ
 * @param[in]     updData           UPDATE_DATAテーブル情報(更新対象のみ)
 * @param[in]     updAppData        UPDATE_DATAテーブル情報(更新の有無にかかわらずプロパティファイルのみ)
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_RESULT CC_GetUpdDataInfo(SMCAL *smcal,
							  const T_CC_CMN_SMS_API_PRM *parm,
							  const SMUPDINFO *mapUpdInfo,
							  Bool *hasUpdate,
							  const Char *dlTempDirPath,
							  SCC_UPDATEDATA *updData,
							  SCC_UPDATEDATA *updAppData)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	Char	*filePath = NULL;
	DBOBJECT	*db = NULL;
	INT32	updDataNum = 0;
	INT32	num = 0;
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

		// バージョン情報DBファイルのパス生成
		if ('/' == dlTempDirPath[strlen(dlTempDirPath) - 1]) {
			sprintf((char*)filePath, "%s%s", dlTempDirPath, SCC_CMN_DB_FILE_DATAVERSIONINFO);
		} else {
			sprintf((char*)filePath, "%s/%s", dlTempDirPath, SCC_CMN_DB_FILE_DATAVERSIONINFO);
		}

		// dataVersionInfo.db有無チェック
		if (0 != stat((const char*)filePath, &st)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"%s not found[path=%s], " HERE, SCC_CMN_DB_FILE_DATAVERSIONINFO, filePath);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// バージョン情報DBのDAL初期化
		ret = SCC_DAL_Initialize(filePath, &db);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_Initialize error(path=%s), " HERE, filePath);
			break;
		}

		// 広域地図
		if (hasUpdate[SCC_MAPDWL_KIND_MAP]) {
			// 地域コード設定
			updData[updDataNum].areaCode = 0;
			// DATA_TYPE
			updData[updDataNum].dataType = SCC_DAL_UPDDATA_DATA_TYPE_BASE_MAP;
			// DATA_KIND
			strcpy(updData[updDataNum].dataKind, SCC_DAL_KIND_BASE);

			// UPDATE_DATAテーブル情報取得
			ret = SCC_DAL_GetUpdateData(db, &updData[updDataNum]);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_GetUpdateData(base) error, " HERE);
				break;
			}
			updDataNum++;
		}

		// プロパティファイルのバージョン情報取得
		// 地域コード設定
		updAppData->areaCode = 0;
		// DATA_TYPE
		updAppData->dataType = SCC_DAL_UPDDATA_DATA_TYPE_SYS;
		// DATA_KIND
		strcpy(updAppData->dataKind, SCC_DAL_KIND_OTHER);

		// UPDATE_DATAテーブル情報取得
		ret = SCC_DAL_GetUpdateData(db, updAppData);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_GetUpdateData(sys) error, " HERE);
			break;
		}

		// プロパティファイル
		if (hasUpdate[SCC_MAPDWL_KIND_DATA]) {
			memcpy(&updData[updDataNum], updAppData, sizeof(SCC_UPDATEDATA));
			updDataNum++;
		}

		// PARCEL
		for (num = 0; num < mapUpdInfo->updateAreaNum; num++, updDataNum++) {
			// 地域コード設定
			updData[updDataNum].areaCode = mapUpdInfo->updateArea[num];
			// DATA_TYPE設定
			updData[updDataNum].dataType = SCC_DAL_UPDDATA_DATA_TYPE_AREA_MAP;
			// DATA_KIND設定
			strcpy(updData[updDataNum].dataKind, SCC_DAL_KIND_PARCEL);

			// UPDATE_DATAテーブル情報取得
			ret = SCC_DAL_GetUpdateData(db, &updData[updDataNum]);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_GetUpdateData error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"areaCode=%d, dataType=%d, dataKind=%s, " HERE,
								   updData[updDataNum].areaCode, updData[updDataNum].dataType, updData[updDataNum].dataKind);
				break;
			}
		}
		if(e_SC_RESULT_SUCCESS != ret){
			break;
		}

		// AREA_CLS
		for (num = 0; num < mapUpdInfo->updateAreaNum; num++, updDataNum++) {
			// 地域コード設定
			updData[updDataNum].areaCode = mapUpdInfo->updateArea[num];
			// DATA_TYPE設定
			updData[updDataNum].dataType = SCC_DAL_UPDDATA_DATA_TYPE_AREA_MAP;
			// DATA_KIND設定
			strcpy(updData[updDataNum].dataKind, SCC_DAL_KIND_AREA_CLS);

			// UPDATE_DATAテーブル情報取得
			ret = SCC_DAL_GetUpdateData(db, &updData[updDataNum]);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_GetUpdateData error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"areaCode=%d, dataType=%d, dataKind=%s, " HERE,
								   updData[updDataNum].areaCode, updData[updDataNum].dataType, updData[updDataNum].dataKind);
				break;
			}
		}
		if(e_SC_RESULT_SUCCESS != ret){
			break;
		}
	} while (0);

	// メモリ解放
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
 * @brief バージョン情報DB(dataVersionInfo.db)ダウンロード
 * @param[in] smcal         SMCAL
 * @param[in] apiParam      APIパラメータ
 * @param[in] rgn_code      仕向け地コード
 * @param[in] dlTempDirPath ダウンロードテンポラリフォルダ
 * @param[in] callbackFnc   コールバック関数ポインタ
 * @param[in] dlFilePath    ダウンロード元ファイルパス格納用バッファ
 * @param[in] saveFilePath  ダウンロードファイル保存先パス格納用バッファ
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_RESULT CC_DLVersionInfoDB(SMCAL *smcal,
								T_CC_CMN_SMS_API_PRM *apiParam,
								const Char *rgn_code,
								const Char *dlTempDirPath,
								const SMPROGRESSCBFNC *callbackFnc,
								Char *dlFilePath,
								Char *saveFilePath)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	//E_SC_CAL_RESULT	calRet = e_SC_CAL_RESULT_SUCCESS;
	//Char	*filePath = NULL;
	//SCC_PROGRESS_FNC	progressCBFnc = NULL;
	//SCC_DOWNLOADBASEVERSION	*dlBaseVer = NULL;
	//INT32	dlBaseVerNum = 0;
	UChar	md5Str[CC_CMN_MD5 * 2] = {};
	UChar	md5[CC_CMN_MD5] = {};
	FILE	*fp = NULL;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// ダウンロードデータのAWS上のパス生成
		sprintf(dlFilePath, "%s%s/%s", SCC_CMN_AWS_DIR_APPDATA, rgn_code, SCC_CMN_TARGZ_FILE_DATAVERSIONINFO_MD5);
		// ダウンロードデータの保存先(端末)のパス生成
		sprintf(saveFilePath, "%s%s", dlTempDirPath, SCC_CMN_TARGZ_FILE_DATAVERSIONINFO_MD5);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"CC_DLVersionInfoDB() dlFilePath = %s, " HERE, dlFilePath);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"CC_DLVersionInfoDB() saveFilePath = %s, " HERE, saveFilePath);

		// ダウンロード
		ret = CC_Download(smcal, apiParam, dlFilePath, saveFilePath, callbackFnc, 0, 0);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_Download error[dl=%s], [save=%s], " HERE, dlFilePath, saveFilePath);
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

		// ダウンロードデータのAWS上のパス生成
		sprintf(dlFilePath, "%s%s/%s", SCC_CMN_AWS_DIR_APPDATA, rgn_code, SCC_CMN_TARGZ_FILE_DATAVERSIONINFO);
		// ダウンロードデータの保存先(端末)のパス生成
		sprintf(saveFilePath, "%s%s", dlTempDirPath, SCC_CMN_TARGZ_FILE_DATAVERSIONINFO);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"CC_DLVersionInfoDB() dlFilePath = %s, " HERE, dlFilePath);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"CC_DLVersionInfoDB() saveFilePath = %s, " HERE, saveFilePath);

		// ダウンロード
		ret = CC_Download(smcal, apiParam, dlFilePath, saveFilePath, callbackFnc, 0, 0);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_Download error[dl=%s], [save=%s], " HERE, dlFilePath, saveFilePath);
			break;
		}

		// ダウンロードしたファイルをチェックする
		ret = CC_CmnDL_CheckFile(saveFilePath, md5, 0);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_CmnDL_CheckFile error[path=%s], " HERE, saveFilePath);
			ret = e_SC_RESULT_MAP_GETERR;
			break;
		}

		// tar.gz解凍
		ret = CC_UnTgz(saveFilePath, dlTempDirPath, callbackFnc);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_UnTgz error[zip=%s], [unzip=%s], " HERE, saveFilePath, dlTempDirPath);
			break;
		}

	} while (0);

	// ファイルクローズ
	if (NULL != fp) {
		fclose(fp);
	}

	// ダウンロードしたファイル削除(エラーはチェックしない)
	remove(saveFilePath);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief エリア地図バージョンチェック
 * @param[in]  upddata          更新情報
 * @param[in]  map_db           DB
 * @param[in]  areaCode         地域コード
 * @param[out] updateStatus     更新有無フラグ
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_RESULT CC_GetAreaInfoList_CheckVersion(const SCC_UPDATEDATA *upddata,
											DBOBJECT *map_db,
											INT32 areaCode,
											upDTcheck_States *updateStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	//Char	*chr = NULL;
	CC_APP_VERSION	appVer = {};
	CC_APP_VERSION	appVerS = {};
	CC_APP_VERSION	appVerE = {};
	E_SCC_DAL_DLFLAG	dlFlag = SCC_DAL_DL_FLAG_NODOWNLOAD;
	INT32	baseVersion = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// 端末のアプリバージョン分割
		ret = CC_CmnDL_SplitAppVersion((Char*)API_VERSION, &appVer);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_CmnDL_SplitAppVersion error, " HERE);
			break;
		}

		// 更新データのアプリバージョン分割
		ret = CC_CmnDL_SplitAppVersion(upddata->appVersionS, &appVerS);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_CmnDL_SplitAppVersion error, " HERE);
			break;
		}
		ret = CC_CmnDL_SplitAppVersion(upddata->appVersionE, &appVerE);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_CmnDL_SplitAppVersion error, " HERE);
			break;
		}
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"API_VERSION=%s, " HERE, API_VERSION);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"appVersionS=%s, " HERE, upddata->appVersionS);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"appVersionE=%s, " HERE, upddata->appVersionE);

		// アプリバージョンチェック
		if ((appVer.ver1 < appVerS.ver1) ||
			((appVer.ver1 == appVerS.ver1) && (appVer.ver2 <  appVerS.ver2)) ||
			((appVer.ver1 == appVerS.ver1) && (appVer.ver2 == appVerS.ver2) && (appVer.ver3 <  appVerS.ver3)) ||
			((appVer.ver1 == appVerS.ver1) && (appVer.ver2 == appVerS.ver2) && (appVer.ver3 == appVerS.ver3) && (appVer.ver4 < appVerS.ver4))) {
			// 更新可能な地図データなし
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"update none. app version old, " HERE);
			*updateStatus = SCC_MAPDWL_CHECK_NONE;
			break;
		}
		if ((appVer.ver1 > appVerE.ver1) ||
			((appVer.ver1 == appVerE.ver1) && (appVer.ver2 >  appVerE.ver2)) ||
			((appVer.ver1 == appVerE.ver1) && (appVer.ver2 == appVerE.ver2) && (appVer.ver3 >  appVerE.ver3)) ||
			((appVer.ver1 == appVerE.ver1) && (appVer.ver2 == appVerE.ver2) && (appVer.ver3 == appVerE.ver3) && (appVer.ver4 > appVerE.ver4))) {
			// 未対応のアプリのバージョン(対応しているアプリのバージョンより新しい)
			// 通常ありえない
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"update none. app version new, " HERE);
			// 更新可能な地図データなし
			*updateStatus = SCC_MAPDWL_CHECK_NONE;
			break;
		}

		if (NULL == map_db) {
			// 地図DB未ダウンロード
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"no map, " HERE);
			*updateStatus = SCC_MAPDWL_CHECK_RGN_AREA_DWLNONE;
			break;
		}

		// 当該地域のDOWNLOAD_AREA_MAPテーブルのダウンロードフラグ取得
		ret = SCC_DAL_GetDLAreaMap(map_db, areaCode, SCC_DAL_KIND_PARCEL, &dlFlag, &baseVersion);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_DAL_GetUpdateData error, areaCode:%d, " HERE, areaCode);
			*updateStatus = SCC_MAPDWL_CHECK_RGN_AREA_DWLNONE;
			ret = e_SC_RESULT_SUCCESS;
			break;
		}
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"DOWNLOAD_AREA_MAP dlFlag=%d, baseVersion=%d, " HERE, dlFlag, baseVersion);

		// DLフラグチェック
		if (SCC_DAL_DL_FLAG_DOWNLOAD != dlFlag) {
			// 地域データ未ダウンロード
			*updateStatus = SCC_MAPDWL_CHECK_RGN_AREA_DWLNONE;
			break;
		}

		// ベースバージョンチェック
		if (baseVersion < upddata->dataVersion) {
			// TODO:TR5.0暫定　広域地図の更新がある場合は、エリア地図の更新は必須とする
//			// 更新可能な地図データあり（更新任意）
//			*updateStatus = SCC_MAPDWL_CHECK_POSSIBLE;
			if (isRgnMapUpdate) {
				// 広域地図を更新する場合、更新必須にする
				*updateStatus = SCC_MAPDWL_CHECK_NEED;
			} else {
				// 更新可能な地図データあり（更新任意）
				*updateStatus = SCC_MAPDWL_CHECK_POSSIBLE;
			}
			// TODO:TR5.0暫定　広域地図の更新がある場合は、エリア地図の更新は必須とする
			break;
		}
		*updateStatus = SCC_MAPDWL_CHECK_NONE;
	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

// TODO:TR5.0暫定　広域地図の更新がある場合は、エリア地図の更新は必須とする
/**
 * @brief 広域地図の更新有無設定
 * @param[in]  isRegionMapUpdate    広域地図の更新があるか否か(true：あり、false:なし)
 */
void  CC_SetIsRegionMapUpdate(Bool isRegionMapUpdate)
{
	isRgnMapUpdate = isRegionMapUpdate;
}
// TODO:TR5.0暫定　広域地図の更新がある場合は、エリア地図の更新は必須とする
