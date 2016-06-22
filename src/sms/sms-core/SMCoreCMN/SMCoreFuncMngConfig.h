/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCOREFUNCMNGCONFIG_H_
#define SMCOREFUNCMNGCONFIG_H_

//-----------------------------------
// 外部I/F定義
//-----------------------------------
E_SC_RESULT SC_MNG_LoadSecureConfig(const Char *confDirPath);
E_SC_RESULT SC_MNG_LoadConfig(const Char *confDirPath);


#endif // #ifndef SMCOREFUNCMNGCONFIG_H_
