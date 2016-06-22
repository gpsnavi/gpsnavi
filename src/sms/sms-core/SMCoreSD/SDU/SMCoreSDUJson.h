/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef _SMPHYDJSON_H_
#define _SMPHYDJSON_H_

E_SC_RESULT SC_SDU_CreateJsonFile(const SCC_AUTHINFO *authInfo, const Char * startTime, const SMPHYDDATA *data, INT32 dataNum, const Char *filePath, Bool isTerminal);

#endif /* _SMPHYDJSON_H_ */

