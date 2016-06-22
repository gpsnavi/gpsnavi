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


#define ROUTE_COST_SEC_MODE_PATH			"Mode Path"
#define ROUTE_COST_KEY_FOLDER				"Folder"
#define ROUTE_COST_KEY_FILE					"File"

#define COST_SEC_SPEED						"Speed"
#define COST_SEC_WEIGHT						"Weight"
#define COST_SEC_TURN						"Turn"
#define COST_KEY_ROAD						"Road"
#define COST_KEY_DIR						"Dir"
#define COST_KEY_APPLYROAD					"ApplyRoad"

#if 1
// 道路種別・リンク種別毎の初期速度
UINT32 SC_ROUTE_COST_INIT_SPEED[SC_SRCHMODE_NUM][SC_ROADTYPE_NUM][SC_LINKTYPE_NUM] =
	{
		// Highway
		{{91,100,50,42,61,5,42,5,5,5,50,5,5},
		{55,60,30,25,36,5,25,5,5,5,30,5,5},
		{73,80,40,33,48,5,33,5,5,5,40,5,5},
		{5,5,5,5,5,5,5,5,5,5,5,5,5},
		{62,68,34,28,41,5,28,5,5,5,34,5,5},
		{45,50,25,21,30,5,21,5,5,5,25,5,5},
		{55,60,30,25,36,5,25,5,5,5,30,5,5},
		{41,45,23,19,27,5,19,5,5,5,23,5,5},
		{36,40,20,17,24,5,17,5,5,5,20,5,5},
		{27,30,15,13,18,5,13,5,5,5,15,5,5},
		{18,20,10,8,12,5,8,5,5,5,10,5,5},
		{18,20,10,8,12,5,8,5,5,5,10,5,5},
		{8,8,8,8,8,5,8,5,5,5,8,5,5},
		{5,5,5,5,5,5,5,5,5,5,5,5,5},
		{5,5,5,5,5,5,5,5,5,5,5,5,5},
		{5,5,5,5,5,5,5,5,5,5,5,5,5}},

		// Normal
		{{91,100,50,42,61,5,42,5,5,5,50,5,5},
		{55,60,30,25,36,5,25,5,5,5,30,5,5},
		{73,80,40,33,48,5,33,5,5,5,40,5,5},
		{5,5,5,5,5,5,5,5,5,5,5,5,5},
		{62,68,34,28,41,5,28,5,5,5,34,5,5},
		{45,50,25,21,30,5,21,5,5,5,25,5,5},
		{55,60,30,25,36,5,25,5,5,5,30,5,5},
		{41,45,23,19,27,5,19,5,5,5,23,5,5},
		{36,40,20,17,24,5,17,5,5,5,20,5,5},
		{27,30,15,13,18,5,13,5,5,5,15,5,5},
		{18,20,10,8,12,5,8,5,5,5,10,5,5},
		{18,20,10,8,12,5,8,5,5,5,10,5,5},
		{8,8,8,8,8,5,8,5,5,5,8,5,5},
		{5,5,5,5,5,5,5,5,5,5,5,5,5},
		{5,5,5,5,5,5,5,5,5,5,5,5,5},
		{5,5,5,5,5,5,5,5,5,5,5,5,5}},

		// Time
		{{91,100,50,42,61,5,42,5,5,5,50,5,5},
		{55,60,30,25,36,5,25,5,5,5,30,5,5},
		{73,80,40,33,48,5,33,5,5,5,40,5,5},
		{5,5,5,5,5,5,5,5,5,5,5,5,5},
		{62,68,34,28,41,5,28,5,5,5,34,5,5},
		{45,50,25,21,30,5,21,5,5,5,25,5,5},
		{55,60,30,25,36,5,25,5,5,5,30,5,5},
		{41,45,23,19,27,5,19,5,5,5,23,5,5},
		{36,40,20,17,24,5,17,5,5,5,20,5,5},
		{27,30,15,13,18,5,13,5,5,5,15,5,5},
		{18,20,10,8,12,5,8,5,5,5,10,5,5},
		{18,20,10,8,12,5,8,5,5,5,10,5,5},
		{8,8,8,8,8,5,8,5,5,5,8,5,5},
		{5,5,5,5,5,5,5,5,5,5,5,5,5},
		{5,5,5,5,5,5,5,5,5,5,5,5,5},
		{5,5,5,5,5,5,5,5,5,5,5,5,5}},

		// Distance
		{{91,100,50,42,61,5,42,5,5,5,50,5,5},
		{55,60,30,25,36,5,25,5,5,5,30,5,5},
		{73,80,40,33,48,5,33,5,5,5,40,5,5},
		{5,5,5,5,5,5,5,5,5,5,5,5,5},
		{62,68,34,28,41,5,28,5,5,5,34,5,5},
		{45,50,25,21,30,5,21,5,5,5,25,5,5},
		{55,60,30,25,36,5,25,5,5,5,30,5,5},
		{41,45,23,19,27,5,19,5,5,5,23,5,5},
		{36,40,20,17,24,5,17,5,5,5,20,5,5},
		{27,30,15,13,18,5,13,5,5,5,15,5,5},
		{18,20,10,8,12,5,8,5,5,5,10,5,5},
		{18,20,10,8,12,5,8,5,5,5,10,5,5},
		{8,8,8,8,8,5,8,5,5,5,8,5,5},
		{5,5,5,5,5,5,5,5,5,5,5,5,5},
		{5,5,5,5,5,5,5,5,5,5,5,5,5},
		{5,5,5,5,5,5,5,5,5,5,5,5,5}}
	};

// 道路種別・リンク種別毎の初期コスト
UINT32 SC_ROUTE_COST_INIT_WEIGHT[SC_SRCHMODE_NUM][SC_ROADTYPE_NUM][SC_LINKTYPE_NUM] =
	{
		// Highway
		{{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{120,120,120,120,120,120,120,120,120,120,120,120,120},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{110,110,110,110,110,110,110,110,110,110,110,110,110},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100}},

		// Normal
		{{220,220,220,220,220,220,220,220,220,220,220,220,220},
		{220,220,220,220,220,220,220,220,220,220,220,220,220},
		{220,220,220,220,220,220,220,220,220,220,220,220,220},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100}},

		// Time
		{{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100}},

		// Distance
		{{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100},
		{100,100,100,100,100,100,100,100,100,100,100,100,100}}
	};

// 道路種別毎の初期転向コスト
UINT32 SC_ROUTE_COST_INIT_TURN[SC_SRCHMODE_NUM][SC_TURNTYPE_NUM] =
	{
		// Highway
		{12000,6000,5500,5000,4000,3500,500,300,0,200,300,1500,3000,3000,3500,4000},
		// Normal
		{12000,6000,5500,5000,4000,3500,500,300,0,200,300,1500,3000,3000,3500,4000},
		// Time
		{12000,6000,5500,5000,4000,3500,500,300,0,200,300,1500,3000,3000,3500,4000},
		// Distance
		{12000,6000,5500,5000,4000,3500,500,300,0,200,300,1500,3000,3000,3500,4000}
	};

// 道路種別毎の初期転向コスト
UINT8 SC_ROUTE_COST_INIT_TURN_APPLY[SC_SRCHMODE_NUM][SC_ROADTYPE_NUM][SC_ROADTYPE_NUM] =
	{
		// Highway
		{{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1}},

		// Normal
		{{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1}},

		// Time
		{{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1}},

		// Distance
		{{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1}}
	};
#endif

static void SC_CONFIG_CallBackSetValue1(const Char *secName, const Char *keyName, const Char *value, void *config);
static void SC_CONFIG_CallBackSetValue2(const Char *secName, const Char *keyName, const Char *value, void *config);

/**
 * @brief  RouteCostConfig.iniファイルから値を読み込む
 * @param[in] fileName  DemoConfig.iniファイルのフルパス
 * @param[in] config    DemoConfig.iniファイルから取得した値を格納する構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_CONFIG_LoadRouteCostConfig(const Char *fileName, SC_ROUTE_COST_CONFIG *config)
{
	E_SC_RESULT			ret = e_SC_RESULT_SUCCESS;
	SC_CONFIG_INI_Func	func1 = SC_CONFIG_CallBackSetValue1;
	SC_CONFIG_INI_Func	func2 = SC_CONFIG_CallBackSetValue2;
	FILE				*fp1  = NULL;
	FILE				*fp2  = NULL;
	INT32				len   = 0;
	UINT16				ilp;
	Char				path[SC_MAX_PATH]={};
	Char				target_path[SC_MAX_PATH]={};

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
		memset(config, 0, sizeof(SC_ROUTE_COST_CONFIG));

		// iniファイルオープン
		ret = SC_CONFIG_FileOpen(fileName, (Char*)"r", &fp1);
		if (e_SC_RESULT_NODATA == ret) {
			SC_LOG_WarnPrint(SC_TAG_DH, "file not found(%s), " HERE, fileName);
			break;
		}
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_DH, (Char*)"SC_CONFIG_FileOpen error(%s), " HERE, fileName);
			break;
		}

		// INIファイルから設定値取得
		ret = SC_CONFIG_GetIniFileValue(func1, fp1, config);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_DH, (Char*)"SC_CONFIG_GetIniFileValue error, " HERE);
			break;
		}

		len = (INT32)strlen(fileName);
		for (ilp = len ; ilp > 0 ; ilp--) {
			if ('/' == fileName[ilp - 1]) {
				strncpy((char*)path, (char*)fileName, ilp);
				break;
			}
		}
		if (ilp <= 0) {
			SC_LOG_ErrorPrint(SC_TAG_DH, (Char*)"param error[config], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 探索モード数
		for (ilp = 0 ; ilp < SC_SRCHMODE_NUM ; ilp++) {
			// 設定ファイルのパス初期化
			memset((char *)target_path, 0x00, sizeof(target_path));

			if (0 < strlen(config->mode.file[ilp])) {
				// 設定ファイルのパス設定
				sprintf(target_path, "%s%s/%s", path, config->mode.folder, config->mode.file[ilp]);

				// iniファイルオープン
				ret = SC_CONFIG_FileOpen(target_path, (Char*)"r", &fp2);
				if (e_SC_RESULT_NODATA == ret) {
					SC_LOG_WarnPrint(SC_TAG_DH, "file not found(%s), " HERE, target_path);
					continue;
				}
				if (e_SC_RESULT_SUCCESS != ret) {
					SC_LOG_ErrorPrint(SC_TAG_DH, (Char*)"SC_CONFIG_FileOpen error(%s), " HERE, target_path);
					continue;
				}

				// INIファイルから設定値取得
				ret = SC_CONFIG_GetIniFileValue(func2, fp2, &(config->mode.cost[ilp]));
				if (e_SC_RESULT_SUCCESS != ret) {
					SC_LOG_ErrorPrint(SC_TAG_DH, (Char*)"SC_CONFIG_GetIniFileValue error, " HERE);
				}
				else {
					// 読み込み成功フラグＯＮ
					config->mode.cost[ilp].valid_f = true;
				}

				// INIファイルクローズ
				SC_CONFIG_FileClose(target_path, (Char*)"r", fp2);
				fp2 = NULL;
			}
		}

	} while (0);

	if (fp1 != NULL) {
		// INIファイルクローズ
		SC_CONFIG_FileClose(fileName, (Char*)"r", fp1);
		fp1 = NULL;
	}

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}

/**
 * @brief  DemoConfig.iniファイルに書き込む
 * @param[in] fileName  DemoConfig.iniファイルのフルパス
 * @param[in] config    DemoConfig.iniファイルから取得した値を格納する構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_CONFIG_SaveRouteCostConfig(const Char *fileName, SC_ROUTE_COST_CONFIG *config)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	//FILE	*fp = NULL;

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
void SC_CONFIG_CallBackSetValue1(const Char *secName, const Char *keyName, const Char *value, void *config)
{
	SC_ROUTE_COST_CONFIG	*configData = NULL;
	Char					cmpName[SC_MAX_PATH]={};
	UINT16					ilp;

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
	configData = (SC_ROUTE_COST_CONFIG*)config;


	// [Mode Path]
	if (0 == strcmp(secName, ROUTE_COST_SEC_MODE_PATH)) {
		// Folder
		if (0 == strcmp(keyName, ROUTE_COST_KEY_FOLDER)) {
			if (sizeof(configData->mode.folder) > strlen(value)) {
				// value
				strcpy(configData->mode.folder, value);
			}
		} else {
			for (ilp = 0 ; ilp < SC_SRCHMODE_NUM ; ilp++) {
				// 比較文字列の初期化
				memset((char *)cmpName, 0x00, sizeof(cmpName));

				// 比較文字列生成
				sprintf(cmpName, "%s%d", ROUTE_COST_KEY_FILE, ilp);

				// FileX
				if (0 == strcmp(keyName, cmpName)) {
					if (sizeof(configData->mode.file[ilp]) > strlen(value)) {
						// value
						strcpy(configData->mode.file[ilp], value);
					}
					break;
				}
			}
		}
	}
}

/**
 * @brief  INIファイルから読み込んだ値を設定するコールバック関数
 * @param[in] secName   INIファイルから読み込んだセクション名
 * @param[in] keyName   INIファイルから読み込んだキー名
 * @param[in] value     INIファイルから読み込んだ値
 */
void SC_CONFIG_CallBackSetValue2(const Char *secName, const Char *keyName, const Char *value, void *config)
{
	SC_COST_CONFIG			*configData = NULL;
	Char					cmpName[SC_MAX_PATH]={};
	UINT16					ilp;
	//UINT16					idx;

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
	configData = (SC_COST_CONFIG*)config;

	// [Speed]
	if (0 == strcmp(secName, COST_SEC_SPEED)) {
		for (ilp = 0 ; ilp < SC_ROADTYPE_NUM ; ilp++) {
			// 比較文字列の初期化
			memset((char *)cmpName, 0x00, sizeof(cmpName));
			// 比較文字列生成
			sprintf(cmpName, "%s%d", COST_KEY_ROAD, ilp);

			// RoadX
			if (0 == strcmp(keyName, cmpName)) {
				sscanf(value, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
						(UINT32 *)&(configData->speed.road[ilp][0]),(UINT32 *)&(configData->speed.road[ilp][1]),
						(UINT32 *)&(configData->speed.road[ilp][2]),(UINT32 *)&(configData->speed.road[ilp][3]),
						(UINT32 *)&(configData->speed.road[ilp][4]),(UINT32 *)&(configData->speed.road[ilp][5]),
						(UINT32 *)&(configData->speed.road[ilp][6]),(UINT32 *)&(configData->speed.road[ilp][7]),
						(UINT32 *)&(configData->speed.road[ilp][8]),(UINT32 *)&(configData->speed.road[ilp][9]),
						(UINT32 *)&(configData->speed.road[ilp][10]),(UINT32 *)&(configData->speed.road[ilp][11]),
						(UINT32 *)&(configData->speed.road[ilp][12]));
				break;
			}
		}
	}
	// [Weight]
	else if (0 == strcmp(secName, COST_SEC_WEIGHT)) {
		for (ilp = 0 ; ilp < SC_ROADTYPE_NUM ; ilp++) {
			// 比較文字列の初期化
			memset((char *)cmpName, 0x00, sizeof(cmpName));
			// 比較文字列生成
			sprintf(cmpName, "%s%d", COST_KEY_ROAD, ilp);

			// RoadX
			if (0 == strcmp(keyName, cmpName)) {
				sscanf(value, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
						(UINT32 *)&(configData->weight.road[ilp][0]),(UINT32 *)&(configData->weight.road[ilp][1]),
						(UINT32 *)&(configData->weight.road[ilp][2]),(UINT32 *)&(configData->weight.road[ilp][3]),
						(UINT32 *)&(configData->weight.road[ilp][4]),(UINT32 *)&(configData->weight.road[ilp][5]),
						(UINT32 *)&(configData->weight.road[ilp][6]),(UINT32 *)&(configData->weight.road[ilp][7]),
						(UINT32 *)&(configData->weight.road[ilp][8]),(UINT32 *)&(configData->weight.road[ilp][9]),
						(UINT32 *)&(configData->weight.road[ilp][10]),(UINT32 *)&(configData->weight.road[ilp][11]),
						(UINT32 *)&(configData->weight.road[ilp][12]));
				break;
			}
		}
	}
	// [Turn]
	else if (0 == strcmp(secName, COST_SEC_TURN)) {
		// Dir
		if (0 == strcmp(keyName, COST_KEY_DIR)) {
			sscanf(value, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
						(UINT32 *)&(configData->turn.dir[0]),(UINT32 *)&(configData->turn.dir[1]),
						(UINT32 *)&(configData->turn.dir[2]),(UINT32 *)&(configData->turn.dir[3]),
						(UINT32 *)&(configData->turn.dir[4]),(UINT32 *)&(configData->turn.dir[5]),
						(UINT32 *)&(configData->turn.dir[6]),(UINT32 *)&(configData->turn.dir[7]),
						(UINT32 *)&(configData->turn.dir[8]),(UINT32 *)&(configData->turn.dir[9]),
						(UINT32 *)&(configData->turn.dir[10]),(UINT32 *)&(configData->turn.dir[11]),
						(UINT32 *)&(configData->turn.dir[12]),(UINT32 *)&(configData->turn.dir[13]),
						(UINT32 *)&(configData->turn.dir[14]),(UINT32 *)&(configData->turn.dir[15]));
		}
		else {
			for (ilp = 0 ; ilp < SC_ROADTYPE_NUM ; ilp++) {
				// 比較文字列の初期化
				memset((char *)cmpName, 0x00, sizeof(cmpName));
				// 比較文字列生成
				sprintf(cmpName, "%s%d", COST_KEY_APPLYROAD, ilp);

				// RoadX
				if (0 == strcmp(keyName, cmpName)) {
					sscanf(value, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
							(UINT32 *)&(configData->turn.apply_f[ilp][0]),(UINT32 *)&(configData->turn.apply_f[ilp][1]),
							(UINT32 *)&(configData->turn.apply_f[ilp][2]),(UINT32 *)&(configData->turn.apply_f[ilp][3]),
							(UINT32 *)&(configData->turn.apply_f[ilp][4]),(UINT32 *)&(configData->turn.apply_f[ilp][5]),
							(UINT32 *)&(configData->turn.apply_f[ilp][6]),(UINT32 *)&(configData->turn.apply_f[ilp][7]),
							(UINT32 *)&(configData->turn.apply_f[ilp][8]),(UINT32 *)&(configData->turn.apply_f[ilp][9]),
							(UINT32 *)&(configData->turn.apply_f[ilp][10]),(UINT32 *)&(configData->turn.apply_f[ilp][11]),
							(UINT32 *)&(configData->turn.apply_f[ilp][12]),(UINT32 *)&(configData->turn.apply_f[ilp][13]),
							(UINT32 *)&(configData->turn.apply_f[ilp][14]),(UINT32 *)&(configData->turn.apply_f[ilp][15]));
					break;
				}
			}
		}
	}
}

