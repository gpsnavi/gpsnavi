/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCORE_REPLAN_CONFIG_H
#define SMCORE_REPLAN_CONFIG_H

//-----------------------------------
// 構造体定義
//-----------------------------------
typedef struct _SC_REPLAN_CONFIG {
	// [Replan Para]
	SMREPLAN	replanPara;
} SC_REPLAN_CONFIG;

//-----------------------------------
// 外部I/F定義
//-----------------------------------
E_SC_RESULT SC_CONFIG_LoadReplanConfig(const Char *fileName, SC_REPLAN_CONFIG *config);
E_SC_RESULT SC_CONFIG_SaveReplanConfig(const Char *fileName, SC_REPLAN_CONFIG *config);

#endif // #ifndef SMCORE_REPLAN_CONFIG_H
