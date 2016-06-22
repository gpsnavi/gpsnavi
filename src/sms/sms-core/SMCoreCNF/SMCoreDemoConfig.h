/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCORE_DEMO_CONFIG_INI_H
#define SMCORE_DEMO_CONFIG_INI_H


//-----------------------------------
// 構造体定義
//-----------------------------------
typedef struct _SC_DEMO_CONFIG {
	// [Init Pos]
	struct {
		LONG	lon;			// Lon
		LONG	lat;			// Lat
	} initPos;
} SC_DEMO_CONFIG;

//-----------------------------------
// 外部I/F定義
//-----------------------------------
E_SC_RESULT SC_CONFIG_LoadDemoConfig(const Char *fileName, SC_DEMO_CONFIG *config);
E_SC_RESULT SC_CONFIG_SaveDemoConfig(const Char *fileName, SC_DEMO_CONFIG *config);

#endif // #ifndef SMCORE_DEMO_CONFIG_INI_H
