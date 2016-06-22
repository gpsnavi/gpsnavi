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
 * SMCoreTR.h
 *
 *  Created on: 2016/01/04
 *      Author: n.kanagawa
 */
#ifndef SMCORETR_H_
#define SMCORETR_H_

//-----------------------------------
// I/F定義
//-----------------------------------
void SC_TR_CacheDiffUpdate(const UINT16 user, UINT32 parcelId);
void SC_TR_CacheAllUpdate(const UINT16 user);

#endif /* SMCORETR_H_ */
