/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCORE_INI_H
#define SMCORE_INI_H


// 関数ポインタ
typedef void (*SC_CONFIG_INI_Func)(const Char *secName, const Char *keyName, const Char *value, void *config);


//-----------------------------------
// 外部I/F定義
//-----------------------------------
E_SC_RESULT SC_CONFIG_FileOpen(const Char *fileName, const Char *mode, FILE **fp);
E_SC_RESULT SC_CONFIG_FileClose(const Char *fileName, const Char *mode, FILE *fp);
E_SC_RESULT SC_CONFIG_GetIniFileValue(SC_CONFIG_INI_Func func, FILE *fp, void *config);

#endif // #ifndef SMCORE_INI_H
