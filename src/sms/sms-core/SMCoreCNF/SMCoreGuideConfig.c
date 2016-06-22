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


#define	GUIDE_SEC_BROADLENGTH							"BroadLength"
#define	GUIDE_KEY_LENGTH								"length"

#define	GUIDE_GUIDEGRAPHICSHOWDIST						"GuideGraphicShowDist"
#define	GUIDE_KEY_HIGHDWAY								"highdway"
#define	GUIDE_KEY_NORMALWAY								"normalway"

#define	GUIDE_SEC_DYNGRAPHICSIZE						"DynGraphicSize"
#define	GUIDE_KEY_HEIGHT								"height"
#define	GUIDE_KEY_WIDTH									"width"

#define	GUIDE_SEC_SIGNBROADSHOWDIST						"SignbroadShowDist"
#define	GUIDE_KEY_DISTANCE								"distance"

#define	GUIDE_SEC_CARMODEBOARDFREQUENCY					"CarModeBoardFrequency"
#define	GUIDE_KEY_SIZE									"size"
#define	GUIDE_KEY_SELECTED								"selected"

#define	GUIDE_SEC_CARMODEGUIDEVOICETRIGGER				"CarModeGuideVoiceTrigger"
#define	GUIDE_KEY_ADVANCE								"advance"

#define	GUIDE_SEC_ONFOOTBOARDFREQUENCY					"OnFootBoardFrequency"

#define	GUIDE_SEC_ONFOOTGUIDEVOICETRIGGER				"OnFootGuideVoiceTrigger"

#define	GUIDE_SEC_JOINPPOINT							"JoinpPoint"

#define	GUIDE_SEC_PLAYINTERVAL							"PlayInterval"
#define	GUIDE_KEY_INTERTIME								"intertime"

#define	GUIDE_SEC_TURNBYTURNDIS							"TurnByTurnDis"
#define	GUIDE_KEY_TBTDIS								"tbtDis"

#define	GUIDE_SEC_COMPLEXGUDPNTDIS						"ComplexGudPntDis"
#define	GUIDE_KEY_HIGHTWAYDIST							"hightwayDist"
#define	GUIDE_KEY_NORMALWAYDIST							"normalwayDist"

#define	GUIDE_SEC_SPEEDLIMITED							"SpeedLimited"
#define	GUIDE_KEY_PLAYINTERVAL							"PlayInterval"

#define	GUIDE_SEC_TRAFFICCAMERA							"TrafficCamera"
#define	GUIDE_KEY_MINDISTOSHOW							"minDisToShow"
#define	GUIDE_KEY_MAXDISTOSHOW							"maxDisToShow"

#define	GUIDE_SEC_CUSTOMTRAFFICCAMERABROADLOWLEVEL		"CustomTrafficCameraBroadLowLevel"

#define	GUIDE_SEC_CUSTOMTRAFFICCAMERABROADNORMALLEVEL	"CustomTrafficCameraBroadNormalLevel"

#define	GUIDE_SEC_CUSTOMTRAFFICCAMERABROADHIGHLEVEL		"CustomTrafficCameraBroadHighLevel"

#define	GUIDE_SEC_LINKSEARCHLENGTH						"LinkSearchLength"

#define	GUIDE_SEC_WARNINGBROADNORMALLEVEL				"WarningBroadNormalLevel"

#define	GUIDE_SEC_HINTOPTION							"HintOption"
#define	GUIDE_KEY_HINTDIRECTION							"HintDirection"

static void SC_CONFIG_CallBackSetValue(const Char *secName, const Char *keyName, const Char *value, void *config);


/**
 * @brief  GuideConfig.iniファイルから値を読み込む
 * @param[in] fileName  GuideConfig.iniファイルのフルパス
 * @param[in] config    GuideConfig.iniファイルから取得した値を格納する構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_CONFIG_LoadGuideConfig(const Char *fileName, SC_GUIDE_CONFIG *config)
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
		memset(config, 0, sizeof(SC_GUIDE_CONFIG));

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
 * @brief  GuideConfig.iniファイルに書き込む
 * @param[in] fileName  GuideConfig.iniファイルのフルパス
 * @param[in] config    GuideConfig.iniファイルから取得した値を格納する構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_CONFIG_SaveGuideConfig(const Char *fileName, SC_GUIDE_CONFIG *config)
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
		// [BroadLength]
		fprintf(fp, "[%s]\n", GUIDE_SEC_BROADLENGTH);
		// length
		fprintf(fp, "%s=%d\n", GUIDE_KEY_LENGTH, config->broadLength.length);
		fprintf(fp, "\n");
#endif

		// [GuideGraphicShowDist]
		fprintf(fp, "[%s]\n", GUIDE_GUIDEGRAPHICSHOWDIST);
		// highdway
		fprintf(fp, "%s=%d\n", GUIDE_KEY_HIGHDWAY, config->guideGraphicShowDist.highdway);
		// normalway
		fprintf(fp, "%s=%d\n", GUIDE_KEY_NORMALWAY, config->guideGraphicShowDist.normalway);
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
	SC_GUIDE_CONFIG	*configData = NULL;

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
	configData = (SC_GUIDE_CONFIG*)config;

#if 0
	// [BroadLength]
	if (0 == strcmp(secName, GUIDE_SEC_BROADLENGTH)) {
		// length
		if (0 == strcmp(keyName, GUIDE_KEY_LENGTH)) {
			// value
			configData->broadLength.length = atoi(value);
		}
	}
#endif

	// [GuideGraphicShowDist]
	if (0 == strcmp(secName, GUIDE_GUIDEGRAPHICSHOWDIST)) {
		// highdway
		if (0 == strcmp(keyName, GUIDE_KEY_HIGHDWAY)) {
			// value
			configData->guideGraphicShowDist.highdway = atoi(value);
		}
		// normalway
		else if (0 == strcmp(keyName, GUIDE_KEY_NORMALWAY)) {
			// value
			configData->guideGraphicShowDist.normalway = atoi(value);
		}
	}
}
