/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCORE_CONFIG_H
#define SMCORE_CONFIG_H

//-----------------------------------
// 列挙型定義
//-----------------------------------
typedef enum _E_SC_DH_CONFIG_DATA_ID {
	e_SC_DH_CONFIG_DATAMANAGERCONFIG = 0,	// DataManagerConfig.ini
	e_SC_DH_CONFIG_GUIDECONFIG,				// GuideConfig.ini
	e_SC_DH_CONFIG_GYROCONFIG,				// GyroConfig.ini
	e_SC_DH_CONFIG_MAPDISPCONFIG,			// MapDispConfig.ini
	e_SC_DH_CONFIG_MAPSTYLE,				// MapStyle.ini
	e_SC_DH_CONFIG_POICONFIG,				// POIConfig.ini
	e_SC_DH_CONFIG_POSITIONCONFIG,			// PositionConfig.ini
	e_SC_DH_CONFIG_REPLANCONFIG,			// ReplanConfig.ini
	e_SC_DH_CONFIG_ROUTEPLANCONFIG,			// RoutePlanConfig.ini
	e_SC_DH_CONFIG_TRAFFIC,					// Traffic.ini
	e_SC_DH_CONFIG_DEMOCONFIG,				// DemoConfig.ini
	e_SC_DH_CONFIG_ROUTECOST,				// RouteCostConfig.ini
	e_SC_DH_CONFIG_GENREDATA,				// GenreDataConfig.ini

	e_SC_DH_CONFIG_DATA_ID_END
} E_SC_DH_CONFIG_DATA_ID;

//-----------------------------------
// 構造体定義
//-----------------------------------
// 設定ファイルデータ入出力構造体
typedef struct _SC_DH_CONFIG_DATA {
	E_SC_DH_CONFIG_DATA_ID	dataId;			// [I]データ識別ID
	void					*data;			// [I/O]入力データ格納領域のポインタ
} SC_DH_CONFIG_DATA;

//-----------------------------------
// 外部I/F定義
//-----------------------------------
E_SC_RESULT SC_DH_ConfigInitialize(const Char *confDirPath);
E_SC_RESULT SC_DH_ConfigFinalize();
E_SC_RESULT SC_DH_LoadConfigFileData(SC_DH_CONFIG_DATA *configData);
E_SC_RESULT SC_DH_SaveConfigFileData(SC_DH_CONFIG_DATA *configData);

#endif // #ifndef SMCORE_CONFIG_H
