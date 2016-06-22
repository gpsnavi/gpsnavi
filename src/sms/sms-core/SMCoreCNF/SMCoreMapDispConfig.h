/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCORE_MAP_DISP_CONFIG_H
#define SMCORE_MAP_DISP_CONFIG_H

//-----------------------------------
// 構造体定義
//-----------------------------------
typedef struct _SC_MAP_DISP_CONFIG {
	// [Run]
	struct {
		INT32		simulate;
	} run;
} SC_MAP_DISP_CONFIG;

//-----------------------------------
// 外部I/F定義
//-----------------------------------
E_SC_RESULT SC_CONFIG_LoadMapDispConfig(const Char *fileName, SC_MAP_DISP_CONFIG *config);
E_SC_RESULT SC_CONFIG_SaveMapDispConfig(const Char *fileName, SC_MAP_DISP_CONFIG *config);

#endif // #ifndef SMCORE_MAP_DISP_CONFIG_H
