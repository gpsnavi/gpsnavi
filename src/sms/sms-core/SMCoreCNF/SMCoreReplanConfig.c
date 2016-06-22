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


#define	RT_PLN_SEC_REPLAN_PARA				"Replan Para"
#define	RT_PLN_KEY_DISTANCE					"Distance"
#define	RT_PLN_KEY_ANGLE					"Angle"

static void SC_CONFIG_CallBackSetValue(const Char *secName, const Char *keyName, const Char *value, void *config);


/**
 * @brief  ReplanConfig.iniファイルから値を読み込む
 * @param[in] fileName  ReplanConfig.iniファイルのフルパス
 * @param[in] config    ReplanConfig.iniファイルから取得した値を格納する構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_CONFIG_LoadReplanConfig(const Char *fileName, SC_REPLAN_CONFIG *config)
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
			SC_LOG_ErrorPrint(SC_TAG_DH, (Char*)"param error[param], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 変数初期化
		memset(config, 0, sizeof(SC_REPLAN_CONFIG));

		// INIファイルオープン
		ret = SC_CONFIG_FileOpen(fileName, (Char*)"r", &fp);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_DH, (Char*)"SC_CONFIG_FileOpen error, " HERE);
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
 * @brief  ReplanConfig.iniファイルに値を書き込む
 * @param[in] fileName  ReplanConfig.iniファイルのフルパス
 * @param[in] config    ReplanConfig.iniファイルから取得した値を格納する構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_CONFIG_SaveReplanConfig(const Char *fileName, SC_REPLAN_CONFIG *config)
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
		if (ret != e_SC_RESULT_SUCCESS) {
			SC_LOG_ErrorPrint(SC_TAG_DH, (Char*)"SC_CONFIG_FileOpen error, " HERE);
			break;
		}

		// [Replan Para]
		fprintf(fp, "[%s]\n", RT_PLN_SEC_REPLAN_PARA);
		// Distance
		fprintf(fp, "%s=%d\n", RT_PLN_KEY_DISTANCE, config->replanPara.distance);
		// Angle
		fprintf(fp, "%s=%d\n", RT_PLN_KEY_ANGLE, config->replanPara.angle);
		fprintf(fp, "\n");
	} while (0);

	if (NULL != fp) {
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
	SC_REPLAN_CONFIG	*configData = NULL;

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
		SC_LOG_ErrorPrint(SC_TAG_DH, (Char*)"param error[config], " HERE);
		return;
	}
	configData = (SC_REPLAN_CONFIG*)config;

	// [Replan Para]
	if (0 == strcmp((char*)secName, RT_PLN_SEC_REPLAN_PARA)) {
		// Distance
		if (0 == strcmp(keyName, RT_PLN_KEY_DISTANCE)) {
			// value
			configData->replanPara.distance = atoi(value);
		}
		// Angle
		else if (0 == strcmp(keyName, RT_PLN_KEY_ANGLE)) {
			// value
			configData->replanPara.angle = atoi(value);
		}
	}
}
