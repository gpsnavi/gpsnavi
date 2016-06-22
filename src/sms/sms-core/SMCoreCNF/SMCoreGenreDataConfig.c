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


#define	GENRE_DATA_SEC_VERSION_INFO					"VERSION_INFO"
#define	GENRE_DATA_KEY_FORMAT						"FORMAT"
#define	GENRE_DATA_KEY_DATE							"DATE"
#define GENRE_DATA_SEC_GENRE1						"GENRE1"
#define GENRE_DATA_KEY_NUM							"NUM"
#define GENRE_DATA_KEY_REC							"REC"

static void SC_CONFIG_CallBackSetValue(const Char *secName, const Char *keyName, const Char *value, void *config);


/**
 * @brief  GenreDataConfig.iniファイルから値を読み込む
 * @param[in] fileName  GenreDataConfig.iniファイルのフルパス
 * @param[in] config    GenreDataConfig.iniファイルから取得した値を格納する構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_CONFIG_LoadGenreDataConfig(const Char *fileName, SC_GENRE_DATA_CONFIG *config)
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
		memset(config, 0, sizeof(SC_GENRE_DATA_CONFIG));

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
* @brief  GenreDataConfig.iniファイルから値を読み込む
 * @param[in] fileName  GenreDataConfig.iniファイルのフルパス
 * @param[in] config    GenreDataConfig.iniファイルから取得した値を格納する構造体のポインタ
  * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_CONFIG_SaveGenreDataConfig(const Char *fileName, SC_GENRE_DATA_CONFIG *config)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

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
	SC_GENRE_DATA_CONFIG	*configData = NULL;
	Char					cmpName[SC_MAX_PATH]={};
	UINT32					ilp;

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
	configData = (SC_GENRE_DATA_CONFIG*)config;

	// [VERSION_INFO]
	if (0 == strcmp(secName, GENRE_DATA_SEC_VERSION_INFO)) {
		// FORMAT
		if (0 == strcmp(keyName, GENRE_DATA_KEY_FORMAT)) {
			// value
			if (sizeof(configData->versionInfo.format) > strlen(value)) {
				// value
				strcpy(configData->versionInfo.format, value);
			}
		}
		// Lat
		else if (0 == strcmp(keyName, GENRE_DATA_KEY_DATE)) {
			// value
			if (sizeof(configData->versionInfo.date) > strlen(value)) {
				// value
				strcpy(configData->versionInfo.date, value);
			}
		}
	}
	// [GENRE1]
	else if (0 == strcmp(secName, GENRE_DATA_SEC_GENRE1)) {
		// NUM
		if (0 == strcmp(keyName, GENRE_DATA_KEY_NUM)) {
			// value
			configData->genre.num = atoi(value);
			if (0 < configData->genre.num) {
				configData->genre.rec = (SC_GENRE_CONFIG *)malloc(sizeof(SC_GENRE_CONFIG) * configData->genre.num);
			} else {
				configData->genre.rec = NULL;
			}
		}
		else {
			if ((0 < configData->genre.num) && (NULL != configData->genre.rec)) {
				for (ilp = 0 ; ilp < configData->genre.num ; ilp++) {
					// 比較文字列の初期化
					memset((char *)cmpName, 0x00, sizeof(cmpName));

					// 比較文字列生成
					sprintf(cmpName, "%s%d", GENRE_DATA_KEY_REC, (ilp+1));

					// REC*
					if (0 == strcmp(keyName, cmpName)) {
						sscanf(value, "%d,%[^,]",(UINT32 *)&(configData->genre.rec[ilp].code),(Char *)&(configData->genre.rec[ilp].name[0]));
						break;
					}
				}
			}
		}
	}
}
