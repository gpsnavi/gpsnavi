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


#define	DT_MGR_SEC_MATCHOPTION				"MatchOption"
#define	DT_MGR_KEY_HIGHWAYMATCHDISTANCE		"HighwayMatchDistance"

#define	DT_MGR_SEC_INIT_POS					"Init Pos"
#define	DT_MGR_KEY_LON						"Lon"
#define	DT_MGR_KEY_LAT						"Lat"

#define	DT_MGR_SEC_USERDATAVERSION			"UserDataVersion"
#define	DT_MGR_KEY_VERSION					"Version"

static void SC_CONFIG_CallBackSetValue(const Char *secName, const Char *keyName, const Char *value, void *config);


/**
 * @brief  DataManagerConfig.iniファイルから値を読み込む
 * @param[in] fileName  DataManagerConfig.iniファイルのフルパス
 * @param[in] config    DataManagerConfig.iniファイルから取得した値を格納する構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_CONFIG_LoadDataManagerConfig(const Char *fileName, SC_DATA_MANAGER_CONFIG *config)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_CONFIG_INI_Func	func = SC_CONFIG_CallBackSetValue;
	FILE	*fp = NULL;

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == fileName) {
			SC_LOG_ErrorPrint(SC_TAG_DH, (Char*)"param error[fileName], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == config) {
			SC_LOG_ErrorPrint(SC_TAG_DH, (Char*)"param error[config], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 変数初期化
		memset(config, 0, sizeof(SC_DATA_MANAGER_CONFIG));

		// iniファイルオープン
		ret = SC_CONFIG_FileOpen(fileName, (Char*)"r", &fp);
		if (e_SC_RESULT_NODATA == ret) {
			SC_LOG_WarnPrint(SC_TAG_DH, "file not found(%s), " HERE, fileName);
			break;
		}
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_DH, (Char*)"SC_CONFIG_FileOpen error(%s), " HERE, fileName);
			break;
		}

		// INIファイルから設定値取得
		ret = SC_CONFIG_GetIniFileValue(func, fp, config);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_DH, (Char*)"SC_CONFIG_GetIniFileValue error, " HERE);
			break;
		}
	} while (0);

	if (fp != NULL) {
		// INIファイルクローズ
		SC_CONFIG_FileClose(fileName, (Char*)"r", fp);
		fp = NULL;
	}

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}

/**
 * @brief  DataManagerConfig.iniファイルに書き込む
 * @param[in] fileName  DataManagerConfig.iniファイルのフルパス
 * @param[in] config    DataManagerConfig.iniファイルから取得した値を格納する構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_CONFIG_SaveDataManagerConfig(const Char *fileName, SC_DATA_MANAGER_CONFIG *config)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	FILE	*fp = NULL;

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == fileName) {
			SC_LOG_ErrorPrint(SC_TAG_DH, (Char*)"param error[fileName], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == config) {
			SC_LOG_ErrorPrint(SC_TAG_DH, (Char*)"param error[config], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// iniファイルオープン
		ret = SC_CONFIG_FileOpen(fileName, (Char*)"w", &fp);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_DH, (Char*)"SC_CONFIG_FileOpen error, " HERE);
			break;
		}

#if 0
		// [MatchOption]
		fprintf(fp, "[%s]\n", DT_MGR_SEC_MATCHOPTION);
		// HighwayMatchDistance
		fprintf(fp, "%s=%d\n", DT_MGR_KEY_HIGHWAYMATCHDISTANCE, config->matchOption.highway);
		fprintf(fp, "\n");
#endif

		// [Init Pos]
		fprintf(fp, "[%s]\n", DT_MGR_SEC_INIT_POS);
		// Lon
		fprintf(fp, "%s=%ld\n", DT_MGR_KEY_LON, config->initPos.lon);
		// Lat
		fprintf(fp, "%s=%ld\n", DT_MGR_KEY_LAT, config->initPos.lat);
		fprintf(fp, "\n");

		// [UserDataVersion]
		fprintf(fp, "[%s]\n", DT_MGR_SEC_USERDATAVERSION);
		// Version
		fprintf(fp, "%s=%s\n", DT_MGR_KEY_VERSION, config->userDataVersion.version);
		fprintf(fp, "\n");
	} while (0);

	if (fp != NULL) {
		// INIファイルクローズ
		SC_CONFIG_FileClose(fileName, (Char*)"w", fp);
		fp = NULL;
	}

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}

/**
 * @brief  INIファイルから読み込んだ値を設定するコールバック関数
 * @param[in] secName   INIファイルから読み込んだセクション名
 * @param[in] keyName   INIファイルから読み込んだキー名
 * @param[in] value     INIファイルから読み込んだ値
 */
void SC_CONFIG_CallBackSetValue(const Char *secName, const Char *keyName, const Char *value, void *config)
{
	SC_DATA_MANAGER_CONFIG	*configData = NULL;

	// パラメータチェック
	if (NULL == secName) {
		SC_LOG_ErrorPrint(SC_TAG_DH, (Char*)"param error[secName], " HERE);
		return;
	}
	if (NULL == keyName) {
		SC_LOG_ErrorPrint(SC_TAG_DH, (Char*)"param error[keyName], " HERE);
		return;
	}
	if (NULL == value) {
		SC_LOG_ErrorPrint(SC_TAG_DH, (Char*)"param error[value], " HERE);
		return;
	}
	if (NULL == config) {
		SC_LOG_ErrorPrint(SC_TAG_DH, (Char*)"param error[configData], " HERE);
		return;
	}
	configData = (SC_DATA_MANAGER_CONFIG*)config;

#if 0
	// [MatchOption]
	if (0 == strcmp(secName, DT_MGR_SEC_MATCHOPTION)) {
		// HighwayMatchDistance
		if (0 == strcmp(keyName, DT_MGR_KEY_HIGHWAYMATCHDISTANCE)) {
			// value
			configData->matchOption.highway = atoi(value);
		}
	}
#endif
	// [Init Pos]
	if (0 == strcmp(secName, DT_MGR_SEC_INIT_POS)) {
		// Lon
		if (0 == strcmp(keyName, DT_MGR_KEY_LON)) {
			// value
			configData->initPos.lon = atol(value);
		}
		// Lat
		else if (0 == strcmp(keyName, DT_MGR_KEY_LAT)) {
			// value
			configData->initPos.lat = atol(value);
		}
	}
	// [UserDataVersion]
	else if (0 == strcmp(secName, DT_MGR_SEC_USERDATAVERSION)) {
		// Version
		if (0 == strcmp(keyName, DT_MGR_KEY_VERSION)) {
			if (sizeof(configData->userDataVersion.version) > strlen(value)) {
				// value
				strcpy(configData->userDataVersion.version, value);
			}
		}
	}
}
