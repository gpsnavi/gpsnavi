/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCORE_DATA_MANAGER_CONFIG_INI_H
#define SMCORE_DATA_MANAGER_CONFIG_INI_H


//-----------------------------------
// 構造体定義
//-----------------------------------
typedef struct _SC_DATA_MANAGER_CONFIG {
#if 0
	// [MatchOption]
	struct {
		INT32	highway;		// HighwayMatchDistance
	} matchOption;
#endif
	// [Init Pos]
	struct {
		LONG	lon;			// Lon
		LONG	lat;			// Lat
	} initPos;
	// [UserDataVersion]
	struct {
		Char	version[32];	// Version
	} userDataVersion;
} SC_DATA_MANAGER_CONFIG;

//-----------------------------------
// 外部I/F定義
//-----------------------------------
E_SC_RESULT SC_CONFIG_LoadDataManagerConfig(const Char *fileName, SC_DATA_MANAGER_CONFIG *config);
E_SC_RESULT SC_CONFIG_SaveDataManagerConfig(const Char *fileName, SC_DATA_MANAGER_CONFIG *config);

#endif // #ifndef SMCORE_DATA_MANAGER_CONFIG_INI_H
