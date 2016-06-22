/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

/*
 * SMCoreFuncMngConfig.c
 *
 *  Created on: 2015/11/05
 *      Author:
 */

#include "SMCoreCMNInternal.h"

//-----------------------------------
// 変数定義
//-----------------------------------

//-----------------------------------
// 関数定義
//-----------------------------------
static E_SC_RESULT SC_MNG_LoadDataManagerConfig();
#if 0
static E_SC_RESULT SC_MNG_LoadGuideConfig();
static E_SC_RESULT SC_MNG_LoadGyroConfig();
#endif
static E_SC_RESULT SC_MNG_LoadMapDispConfig();
#if 0
static E_SC_RESULT SC_MNG_LoadPositionConfig();
#endif
static E_SC_RESULT SC_MNG_LoadReplanConfig();
static E_SC_RESULT SC_MNG_LoadRoutePlanConfig();
static E_SC_RESULT SC_MNG_LoadRoutePlanConfig();
static E_SC_RESULT SC_MNG_LoadDemoConfig();
static E_SC_RESULT SC_MNG_LoadRouteCostConfig();
static E_SC_RESULT SC_MNG_LoadGenreDataConfig();
static E_SC_RESULT SC_MNG_MakeDir(const Char *dirPath);


/**
 * @brief 設定ファイルを読み込む(毎回)
 * @return 処理結果(E_SC_RESULT)
 * @memo ConfinのInitializeを含む
 * @memo 必ず設定ファイルから読み込む項目は本処理にて共有メモリへ展開する。
 * @memo 事前にSC_DH_LoadShareData()を処理済みであること。
 */
E_SC_RESULT SC_MNG_LoadSecureConfig(const Char *confDirPath)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	//struct stat		st = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// 初期化
		ret = SC_DH_ConfigInitialize(confDirPath);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_ConfigInitialize error, " HERE);
			break;
		}

		// ディレクトリが存在しない場合、作成する
		ret = SC_MNG_MakeDir(confDirPath);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MNG_MakeDir error(0x%08x), %s, " HERE, errno, confDirPath);
			break;
		}

		// ReplanConfig.ini
		ret = SC_MNG_LoadReplanConfig();
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MNG_LoadReplanConfig error, " HERE);
			//break;
		}

		// DemoConfig.ini
		ret = SC_MNG_LoadDemoConfig();
		if (e_SC_RESULT_SUCCESS == ret) {
			SC_LOG_InfoPrint(SC_TAG_CORE, "SC_MNG_LoadDemoConfig find inifile!, " HERE);
			//break;
		}

		// RouteCostConfig.ini
		ret = SC_MNG_LoadRouteCostConfig();
		if (e_SC_RESULT_SUCCESS == ret) {
			SC_LOG_InfoPrint(SC_TAG_CORE, "SC_MNG_LoadRouteCostConfig find inifile!, " HERE);
			//break;
		}

		// GenreDataConfig.ini
		ret = SC_MNG_LoadGenreDataConfig();
		if (e_SC_RESULT_SUCCESS == ret) {
			SC_LOG_InfoPrint(SC_TAG_CORE, "SC_MNG_LoadGenreDataConfig find inifile!, " HERE);
			//break;
		}

		ret = e_SC_RESULT_SUCCESS;
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 設定ファイルを読み込む
 * @return 処理結果(E_SC_RESULT)
 * @memo SC_DH_LoadShareData()によるリストア失敗時INIファイルから初期値を取得する処理。
 * @memo 事前にSC_MNG_LoadSecureConfig()を処理済みであること。
 */
E_SC_RESULT SC_MNG_LoadConfig(const Char *confDirPath)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	//struct stat		st = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
#if 0
		// 初期化
		ret = SC_DH_ConfigInitialize(confDirPath);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_ConfigInitialize error, " HERE);
			break;
		}

		// ディレクトリが存在しない場合、作成する
		ret = SC_MNG_MakeDir(confDirPath);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MNG_MakeDir error(0x%08x), %s, " HERE, errno, confDirPath);
			break;
		}
#endif
		// DataManagerConfig.ini
		ret = SC_MNG_LoadDataManagerConfig();
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MNG_LoadDataManagerConfig error, " HERE);
			//break;
		}
#if 0
		// GuideConfig.ini
		ret = SC_MNG_LoadGuideConfig();
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MNG_LoadGuideConfig error, " HERE);
			//break;
		}

		// GyroConfig.ini
		ret = SC_MNG_LoadGyroConfig();
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MNG_LoadGyroConfig error, " HERE);
			//break;
		}
#endif

		// MapDispConfig.ini
		ret = SC_MNG_LoadMapDispConfig();
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MNG_LoadMapDispConfig error, " HERE);
			//break;
		}

#if 0
		// PositionConfig.ini
		ret = SC_MNG_LoadPositionConfig();
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MNG_LoadPositionConfig error, " HERE);
			//break;
		}
#endif

		// RoutePlanConfig.ini
		ret = SC_MNG_LoadRoutePlanConfig();
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MNG_LoadRoutePlanConfig error, " HERE);
			//break;
		}
		ret = e_SC_RESULT_SUCCESS;
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief DataManagerConfig.iniファイルを読み込み、取得した値を常駐メモリに設定する
 * @return 処理結果(E_SC_RESULT)
 */
static E_SC_RESULT SC_MNG_LoadDataManagerConfig()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_CONFIG_DATA	data = {};
	SC_DATA_MANAGER_CONFIG	mngConfig = {};
	SMCARSTATE	carState = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータ設定
		data.dataId = e_SC_DH_CONFIG_DATAMANAGERCONFIG;
		data.data   = &mngConfig;
		// 設定ファイルを読み込む
		ret = SC_DH_LoadConfigFileData(&data);
		if (e_SC_RESULT_NODATA == ret) {
			ret = e_SC_RESULT_SUCCESS;
			break;
		}
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_LoadConfigFileData(DataManagerConfig.ini) error, " HERE);
			break;
		}
		// 常駐メモリに設定するデータを設定
		// 経度
		carState.coord.longitude = mngConfig.initPos.lon;
		// 緯度
		carState.coord.latitude  = mngConfig.initPos.lat;
		// 車両状態情報設定
		ret = SC_MNG_SetCarState(&carState, e_SC_CARLOCATION_REAL);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, (Char*)"SC_MNG_SetCarState error(0x%08x), " HERE, ret);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

#if 0
/**
 * @brief GuideConfig.iniファイルを読み込み、取得した値を常駐メモリに設定する
 * @return 処理結果(E_SC_RESULT)
 */
static E_SC_RESULT SC_MNG_LoadGuideConfig()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_CONFIG_DATA	data = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータ設定
		data.dataId = e_SC_DH_CONFIG_GUIDECONFIG;
		//data.data   = &;
		// 設定ファイルを読み込む
		ret = SC_DH_LoadConfigFileData(&data);
		if (e_SC_RESULT_NODATA == ret) {
			ret = e_SC_RESULT_SUCCESS;
			break;
		}
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_LoadConfigFileData(GuideConfig.ini) error, " HERE);
			break;
		}
		// 常駐メモリに設定するデータを設定
		// TODO
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief GyroConfig.iniファイルを読み込み、取得した値を常駐メモリに設定する
 * @return 処理結果(E_SC_RESULT)
 */
static E_SC_RESULT SC_MNG_LoadGyroConfig()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_CONFIG_DATA	data = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータ設定
		data.dataId = e_SC_DH_CONFIG_GYROCONFIG;
		//data.data   = &;
		// 設定ファイルを読み込む
		ret = SC_DH_LoadConfigFileData(&data);
		if (e_SC_RESULT_NODATA == ret) {
			ret = e_SC_RESULT_SUCCESS;
			break;
		}
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_LoadConfigFileData(GyroConfig.ini) error, " HERE);
			break;
		}
		// 常駐メモリに設定するデータを設定
		// TODO
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}
#endif

/**
 * @brief MapDispConfig.iniファイルを読み込み、取得した値を常駐メモリに設定する
 * @return 処理結果(E_SC_RESULT)
 */
static E_SC_RESULT SC_MNG_LoadMapDispConfig()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_CONFIG_DATA	data = {};
	SC_MAP_DISP_CONFIG	mapDispConfig = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータ設定
		data.dataId = e_SC_DH_CONFIG_MAPDISPCONFIG;
		data.data   = &mapDispConfig;
		// 設定ファイルを読み込む
		ret = SC_DH_LoadConfigFileData(&data);
		if (e_SC_RESULT_NODATA == ret) {
			ret = e_SC_RESULT_SUCCESS;
			break;
		}
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_LoadConfigFileData(MapDispConfig.ini) error, " HERE);
			break;
		}
		// 常駐メモリに設定するデータを設定
		// TODO
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

#if 0
/**
 * @brief PositionConfig.iniファイルを読み込み、取得した値を常駐メモリに設定する
 * @return 処理結果(E_SC_RESULT)
 */
static E_SC_RESULT SC_MNG_LoadPositionConfig()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_CONFIG_DATA	data = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータ設定
		data.dataId = e_SC_DH_CONFIG_POSITIONCONFIG;
		//data.data   = &;
		// 設定ファイルを読み込む
		ret = SC_DH_LoadConfigFileData(&data);
		if (e_SC_RESULT_NODATA == ret) {
			ret = e_SC_RESULT_SUCCESS;
			break;
		}
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_LoadConfigFileData(PositionConfig.ini) error, " HERE);
			break;
		}
		// 常駐メモリに設定するデータを設定
		// TODO
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}
#endif

/**
 * @brief ReplanConfig.iniファイルを読み込み、取得した値を常駐メモリに設定する
 * @return 処理結果(E_SC_RESULT)
 */
static E_SC_RESULT SC_MNG_LoadReplanConfig()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_CONFIG_DATA	data = {};
	SC_REPLAN_CONFIG	rePlanConfig = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータ設定
		data.dataId = e_SC_DH_CONFIG_REPLANCONFIG;
		data.data   = &rePlanConfig;
		// 設定ファイルを読み込む
		ret = SC_DH_LoadConfigFileData(&data);
		if (e_SC_RESULT_NODATA == ret) {
			ret = e_SC_RESULT_SUCCESS;
			break;
		}
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_LoadConfigFileData(ReplanConfig.ini) error, " HERE);
			break;
		}
		// 常駐メモリに設定するデータを設定
		// TODO
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief RoutePlanConfig.iniファイルを読み込み、取得した値を常駐メモリに設定する
 * @return 処理結果(E_SC_RESULT)
 */
static E_SC_RESULT SC_MNG_LoadRoutePlanConfig()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_CONFIG_DATA	data = {};
	SC_ROUTE_PLAN_CONFIG	rpConfig ={};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータ設定
		data.dataId = e_SC_DH_CONFIG_ROUTEPLANCONFIG;
		data.data   = &rpConfig;
		// 設定ファイルを読み込む
		ret = SC_DH_LoadConfigFileData(&data);
		if (e_SC_RESULT_NODATA == ret) {
			ret = e_SC_RESULT_SUCCESS;
			break;
		}
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_LoadConfigFileData(RoutePlanConfig.ini) error, " HERE);
			break;
		}
		// 探索条件設定
		ret = SC_MNG_SetRPOption(&rpConfig.option);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, (Char*)"SC_MNG_SetRPOption error(0x%08x), " HERE, ret);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief DemoConfig.iniファイルを読み込み、取得した値を常駐メモリに設定する
 * @return 処理結果(E_SC_RESULT)
 */
static E_SC_RESULT SC_MNG_LoadDemoConfig()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_CONFIG_DATA	data = {};
	SC_DEMO_CONFIG	demoConfig = {};
	SMCARSTATE	carState = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータ設定
		data.dataId = e_SC_DH_CONFIG_DEMOCONFIG;
		data.data   = &demoConfig;
		// 設定ファイルを読み込む
		ret = SC_DH_LoadConfigFileData(&data);
		if (e_SC_RESULT_NODATA == ret) {
			ret = e_SC_RESULT_SUCCESS;
			break;
		}
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_LoadConfigFileData(DemoConfig.ini) error, " HERE);
			break;
		}
		// 常駐メモリに設定するデータを設定
		// 経度
		carState.coord.longitude = demoConfig.initPos.lon;
		// 緯度
		carState.coord.latitude  = demoConfig.initPos.lat;
		// 車両状態情報設定
		ret = SC_MNG_SetCarState(&carState, e_SC_CARLOCATION_REAL);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, (Char*)"SC_MNG_SetCarState error(0x%08x), " HERE, ret);
			break;
		}

		// デモモード設定 (無条件ON)
		ret = SC_MNG_SetDemoMode(true);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, (Char*)"SC_MNG_SetDemoMode error(0x%08x), " HERE, ret);
			break;
		}

	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief DemoConfig.iniファイルを読み込み、取得した値を常駐メモリに設定する
 * @return 処理結果(E_SC_RESULT)
 */
static E_SC_RESULT SC_MNG_LoadRouteCostConfig()
{
	E_SC_RESULT				ret = e_SC_RESULT_SUCCESS;
	SC_DH_CONFIG_DATA		data = {};
	SC_ROUTE_COST_CONFIG	routeCostConfig = {};
	SC_COST_CONFIG			*i_cost_p;
	SMRTCOSTINFO			rtCostInfo = {};
	SMRTCOST				*o_cost_p;
	UINT32					ilp;
	UINT32					xlp;
	UINT32					ylp;

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// 常駐メモリに初期データを設定する
		for (ilp = 0 ; ilp < SC_SRCHMODE_NUM ; ilp++) {
			o_cost_p = &(rtCostInfo.routeCost[ilp]);
			memcpy(o_cost_p->speed,			SC_ROUTE_COST_INIT_SPEED[ilp], 		sizeof(UINT32) * SC_ROADTYPE_NUM * SC_LINKTYPE_NUM);
			memcpy(o_cost_p->weight, 		SC_ROUTE_COST_INIT_WEIGHT[ilp],		sizeof(UINT32) * SC_ROADTYPE_NUM * SC_LINKTYPE_NUM);
			memcpy(o_cost_p->turn, 			SC_ROUTE_COST_INIT_TURN[ilp],		sizeof(UINT32) * SC_TURNTYPE_NUM);
			memcpy(o_cost_p->turn_apply_f, 	SC_ROUTE_COST_INIT_TURN_APPLY[ilp],	sizeof(UINT8)  * SC_ROADTYPE_NUM * SC_ROADTYPE_NUM);
		}
		// 常駐メモリに経路コスト情報設定
		ret = SC_MNG_SetRouteCostInfo(&rtCostInfo);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, (Char*)"SC_MNG_SetRouteCostInfo error(0x%08x), " HERE, ret);
			break;
		}

		// パラメータ設定
		data.dataId = e_SC_DH_CONFIG_ROUTECOST;
		data.data   = &routeCostConfig;
		// 設定ファイルを読み込む
		ret = SC_DH_LoadConfigFileData(&data);
		if (e_SC_RESULT_NODATA == ret) {
			break;
		}
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_LoadConfigFileData(RouteCostConfig.ini) error, " HERE);
			break;
		}

		// 常駐メモリに設定ファイルデータを設定する
		for (ilp = 0 ; ilp < SC_SRCHMODE_NUM ; ilp++) {
			i_cost_p = &(routeCostConfig.mode.cost[ilp]);
			o_cost_p = &(rtCostInfo.routeCost[ilp]);

			if (i_cost_p->valid_f) {
				memcpy(o_cost_p->speed,			i_cost_p->speed.road,	sizeof(UINT32) * SC_ROADTYPE_NUM * SC_LINKTYPE_NUM);
				memcpy(o_cost_p->weight, 		i_cost_p->weight.road,	sizeof(UINT32) * SC_ROADTYPE_NUM * SC_LINKTYPE_NUM);
				memcpy(o_cost_p->turn, 			i_cost_p->turn.dir, 	sizeof(UINT32) * SC_TURNTYPE_NUM);
				for (xlp = 0 ; xlp < SC_ROADTYPE_NUM ; xlp++) {
					for(ylp = 0 ; ylp < SC_ROADTYPE_NUM ; ylp++){
						o_cost_p->turn_apply_f[xlp][ylp] = i_cost_p->turn.apply_f[xlp][ylp];
					}
				}
			}
		}
		// 常駐メモリに経路コスト情報設定
		ret = SC_MNG_SetRouteCostInfo(&rtCostInfo);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, (Char*)"SC_MNG_SetRouteCostInfo error(0x%08x), " HERE, ret);
			break;
		}

	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief DemoConfig.iniファイルを読み込み、取得した値を常駐メモリに設定する
 * @return 処理結果(E_SC_RESULT)
 */
static E_SC_RESULT SC_MNG_LoadGenreDataConfig()
{
	E_SC_RESULT				ret = e_SC_RESULT_SUCCESS;
	SC_DH_CONFIG_DATA		data = {};
	SC_GENRE_DATA_CONFIG	genreDataConfig = {};
	SMGENREDATA				genreData = {};
	UINT32					size;
	UINT32					ilp;

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータ設定
		data.dataId = e_SC_DH_CONFIG_GENREDATA;
		data.data   = &genreDataConfig;
		// 設定ファイルを読み込む
		ret = SC_DH_LoadConfigFileData(&data);
		if (e_SC_RESULT_NODATA == ret) {
			ret = e_SC_RESULT_SUCCESS;
			break;
		}
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_LoadConfigFileData(GenreDataConfig.ini) error, " HERE);
			break;
		}
		// 常駐メモリに設定するデータを設定
		memcpy(genreData.format, genreDataConfig.versionInfo.format, sizeof(Char) * 32);
		memcpy(genreData.date, genreDataConfig.versionInfo.date, sizeof(Char) * 32);
		genreData.num   = genreDataConfig.genre.num;
		genreData.genre = NULL;

		// ジャンル数あり
		if (genreData.num) {
			size = sizeof(SMGENRE) * genreData.num;

			// 動的メモリ確保
			genreData.genre = SC_MEM_Alloc(size, e_MEM_TYPE_GENREDATA);
			if (NULL == genreData.genre) {
				SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MEM_Alloc(GenreDataConfig.ini) error, " HERE);
				ret = e_SC_RESULT_MALLOC_ERR;
				break;
			}
			// ジャンル数分データコピー
			for (ilp = 0 ; ilp < genreData.num ; ilp++) {
				(genreData.genre + ilp)->code = (genreDataConfig.genre.rec + ilp)->code;
				memcpy((genreData.genre + ilp)->name, (genreDataConfig.genre.rec + ilp)->name, sizeof(Char) * 64);
			}
		}

		// 常駐メモリにジャンルデータ設定
		ret = SC_MNG_SetGenreData(&genreData);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, (Char*)"SC_MNG_SetGenreData error(0x%08x), " HERE, ret);
			break;
		}
	} while (0);

	// iniファイルから読み込む際に確保したメモリ解放
	if (NULL != genreDataConfig.genre.rec) {
		free(genreDataConfig.genre.rec);
	}

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief ディレクトリ作成
 * @param[in] dirPath 作成するディレクトリのフルパス（最大長はNULL終端文字含めてSC_MAX_PATH）
 * @return 処理結果(E_SC_RESULT)
 * @warning パスにフォルダのフルパスを指定する場合は、末尾に/を付加すること。
 *          パスは、ファイルのフルパスでも可。
 */
E_SC_RESULT SC_MNG_MakeDir(const Char *dirPath)
{
	Char			path[SC_MAX_PATH] = {};
	Char			*pPath = NULL;
	Char			*chr = NULL;
	INT32			cnt = 0;
	//UINT32			errCode = 0;
	struct stat		st = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	if (NULL == dirPath) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[dirPath], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// コピー先フォルダ作成
	strncpy((char*)path, (char*)dirPath, (sizeof(path) - 1));
	chr = strrchr((char*)path, '/');
	if (NULL != chr) {
		if ((INT32)strlen(path) > (INT32)(chr - path)) {
			*(chr + 1) = EOS;
		}
	}

	pPath = &path[0];
	while (EOS != *pPath) {
		// 先頭から'/'を検索
		chr = (Char*)strchr((char*)pPath, '/');
		if (NULL == chr) {
			// 見つからなかったので終了
			break;
		}
		if (0 < cnt) {
			*chr = EOS;
			if (0 != stat((char*)path, &st)) {
				// ディレクトリ作成
				if (0 != mkdir((char*)path, (S_IRWXU | S_IRWXG | S_IRWXO))) {
					SC_LOG_ErrorPrint(SC_TAG_CORE, "mkdir error(0x%08x), " HERE, errno);
					SC_LOG_ErrorPrint(SC_TAG_CORE, "path=%s, " HERE, path);
					return (e_SC_RESULT_FILE_ACCESSERR);
				}
			}
			*chr = '/';
		}
		pPath = chr + 1;
		cnt++;
	}

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (e_SC_RESULT_SUCCESS);
}
