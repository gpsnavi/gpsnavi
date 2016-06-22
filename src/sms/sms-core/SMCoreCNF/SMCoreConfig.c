/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#include "SMCoreCNFInternal.h"


// 設定ファイルのデータ識別IDチェック
#define	SC_DH_CHECK_CONFIGDATA_ID(id)	((e_SC_DH_CONFIG_DATAMANAGERCONFIG <= id) && (e_SC_DH_CONFIG_DATA_ID_END > id))

//-----------------------------------
// 構造体定義
//-----------------------------------
// 関数ポインタ
typedef E_SC_RESULT (*SC_CONFIG_Func)(const Char *fileName, void *param);
typedef struct _SC_CONFIG_FUNC_TBL {
	Char			*fileName;
	SC_CONFIG_Func	loadFunc;
	SC_CONFIG_Func	saveFunc;
} SC_CONFIG_FUNC_TBL;

//-----------------------------------
// テーブル定義
//-----------------------------------
#define	SC_CONFIG_FUNC_TBL_NUM		e_SC_DH_CONFIG_DATA_ID_END		// 設定ファイルのデータ識別ID数
// 設定・取得関数ポインタテーブル
const static SC_CONFIG_FUNC_TBL funcTbl[SC_CONFIG_FUNC_TBL_NUM] = {
	{"DataManagerConfig.ini",	(SC_CONFIG_Func)SC_CONFIG_LoadDataManagerConfig,(SC_CONFIG_Func)SC_CONFIG_SaveDataManagerConfig},
	{"GuideConfig.ini",			(SC_CONFIG_Func)SC_CONFIG_LoadGuideConfig,		(SC_CONFIG_Func)SC_CONFIG_SaveGuideConfig},
//	{"GyroConfig.ini",			(SC_CONFIG_Func)SC_CONFIG_LoadGyroConfig,		(SC_CONFIG_Func)SC_CONFIG_SaveGyroConfig},
	{"GyroConfig.ini",			(SC_CONFIG_Func)NULL,							(SC_CONFIG_Func)NULL},
	{"MapDispConfig.ini",		(SC_CONFIG_Func)SC_CONFIG_LoadMapDispConfig,	(SC_CONFIG_Func)SC_CONFIG_SaveMapDispConfig},
	{"MapStyle.ini",			(SC_CONFIG_Func)NULL,							(SC_CONFIG_Func)NULL},
	{"POIConfig.ini",			(SC_CONFIG_Func)NULL,							(SC_CONFIG_Func)NULL},
//	{"PositionConfig.ini",		(SC_CONFIG_Func)SC_CONFIG_LoadPositionConfig,	(SC_CONFIG_Func)SC_CONFIG_SavePositionConfig},
	{"PositionConfig.ini",		(SC_CONFIG_Func)NULL,							(SC_CONFIG_Func)NULL},
//	{"ReplanConfig.ini",		(SC_CONFIG_Func)SC_CONFIG_LoadReplanConfig,		(SC_CONFIG_Func)SC_CONFIG_SaveReplanConfig},
	{"ReplanConfig.ini",		(SC_CONFIG_Func)NULL,							(SC_CONFIG_Func)NULL},
	{"RoutePlanConfig.ini",		(SC_CONFIG_Func)SC_CONFIG_LoadRoutePlanConfig,	(SC_CONFIG_Func)SC_CONFIG_SaveRoutePlanConfig},
	{"Traffic.ini",				(SC_CONFIG_Func)NULL,							(SC_CONFIG_Func)NULL},
	{"DemoConfig.ini",			(SC_CONFIG_Func)SC_CONFIG_LoadDemoConfig,		(SC_CONFIG_Func)SC_CONFIG_SaveDemoConfig},
	{"RouteCostConfig.ini",		(SC_CONFIG_Func)SC_CONFIG_LoadRouteCostConfig,	(SC_CONFIG_Func)SC_CONFIG_SaveRouteCostConfig},
	{"GenreDataConfig.ini",		(SC_CONFIG_Func)SC_CONFIG_LoadGenreDataConfig,	(SC_CONFIG_Func)SC_CONFIG_SaveGenreDataConfig}
};

//-----------------------------------
// 変数定義
//-----------------------------------
static SC_MUTEX configMutex = SC_MUTEX_INITIALIZER;
static Char		configPath[SC_MAX_PATH];

//-----------------------------------
// 関数定義
//-----------------------------------
static E_SC_RESULT SC_DH_LoadConfigFileDataDispatch(SC_DH_CONFIG_DATA *data);
static E_SC_RESULT SC_DH_SaveConfigFileDataDispatch(SC_DH_CONFIG_DATA *data);


/**
 * @brief 初期化処理
 * @param[in] confDirPath ナビコアの設定ファイル格納ディレクトリのフルパス(NULL可)
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_DH_ConfigInitialize(const Char *confDirPath)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	len = 0;

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	do {
		if (NULL == confDirPath) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "param error[confDirPath], " HERE);
			configPath[0] = EOS;
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// パス長チェック
		len = (INT32)strlen(confDirPath);
#ifdef __SMS_APPLE__
		if (SC_MAX_PATH-132 < len) {
#else
		if (128 < len) {
#endif /* __SMS_APPLE__ */
			SC_LOG_ErrorPrint(SC_TAG_DH, "param len error[confDirPath], " HERE);
			configPath[0] = EOS;
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 設定ファイル格納ディレクトリ設定
		strcpy(configPath, confDirPath);
		if ('/' != configPath[len - 1]) {
			strcat(configPath, "/");
		}

		// Mutex生成
		ret = SC_CreateMutex(&configMutex);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_CreateMutext() error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}

/**
 * @brief 終了化処理
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_DH_ConfigFinalize()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	//SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);
	LOG_PRINT_START((char*)SC_TAG_DH);

	// エラーでも処理は継続する
	configPath[0] = EOS;

	//SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);
	LOG_PRINT_END((char*)SC_TAG_DH);

	return (ret);
}

/**
 * @brief 設定ファイルからデータを取得する
 * @param[in/out] data 常駐メモリデータ入出力構造体
 * @return 処理結果(E_SC_RESULT)
 * @
 */
E_SC_RESULT SC_DH_LoadConfigFileData(SC_DH_CONFIG_DATA *configData)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	Bool	isMutexLocked = false;

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == configData) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "param error[configData], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == configData->data) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "param error[configData->data], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// Mutexロック
		ret = SC_LockMutex(&configMutex);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_LockMutext error, " HERE);
			break;
		}
		isMutexLocked = true;

		// データ識別IDで指定されたデータを設定ファイルから取得する
		ret = SC_DH_LoadConfigFileDataDispatch(configData);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_DH_GetConfigDataDispatch error, " HERE);
			break;
		}
	} while (0);

	if (true == isMutexLocked) {
		// Mutexアンロック
		ret2 = SC_UnLockMutex(&configMutex);
		if (e_SC_RESULT_SUCCESS != ret2) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_UnLockMutext error, " HERE);
			if (e_SC_RESULT_SUCCESS == ret) {
				ret = ret2;
			}
		}
	}

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}

/**
 * @brief 設定ファイルにデータを保存する
 * @param[in] data 常駐メモリデータ入出力構造体
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_DH_SaveConfigFileData(SC_DH_CONFIG_DATA *configData)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	Bool	isMutexLocked = false;

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == configData) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "param error[configData], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// Mutexロック
		ret = SC_LockMutex(&configMutex);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_LockMutext error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}
		isMutexLocked = true;

		// データ識別IDで指定されたデータを設定ファイルに保存する
		ret = SC_DH_SaveConfigFileDataDispatch(configData);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_DH_SetConfigDataDispatch error, " HERE);
			break;
		}
	} while (0);

	if (true == isMutexLocked) {
		// Mutexアンロック
		ret2 = SC_UnLockMutex(&configMutex);
		if (e_SC_RESULT_SUCCESS != ret2) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_UnLockMutext error, " HERE);
			if (e_SC_RESULT_SUCCESS == ret) {
				ret = ret2;
			}
		}
	}

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}

/**
 * @brief 設定ファイルからデータを取得する
 * @param[in] data 常駐メモリデータ入出力構造体
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_DH_LoadConfigFileDataDispatch(SC_DH_CONFIG_DATA *data)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	Char	path[SC_MAX_PATH];

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	// パラメータチェック
	if (true != SC_DH_CHECK_CONFIGDATA_ID(data->dataId)) {
		SC_LOG_ErrorPrint(SC_TAG_DH, "parame error[data->dataId=%d], " HERE, data->dataId);
		return (e_SC_RESULT_BADPARAM);
	}

	if (NULL != funcTbl[data->dataId].loadFunc) {
		// 設定ファイルのパス設定
		sprintf(path, "%s%s", configPath, funcTbl[data->dataId].fileName);

		// 設定ファイルのデータ取得
		ret = funcTbl[data->dataId].loadFunc(path, data->data);
	}

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}

/**
 * @brief 設定ファイルにデータを保存する
 * @param[in] data 常駐メモリデータ入出力構造体
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_DH_SaveConfigFileDataDispatch(SC_DH_CONFIG_DATA *data)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	Char	path[SC_MAX_PATH];

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	// パラメータチェック
	if (true != SC_DH_CHECK_CONFIGDATA_ID(data->dataId)) {
		SC_LOG_ErrorPrint(SC_TAG_DH, "parame error[data->dataId], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	if (NULL != funcTbl[data->dataId].saveFunc) {
		// 設定ファイルのパス設定
		sprintf(path, "%s%s", configPath, funcTbl[data->dataId].fileName);

		// 設定ファイルのデータ設定
		ret = funcTbl[data->dataId].saveFunc(path, data->data);
	}

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}
