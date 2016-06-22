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
 * SMCoreTRMem.h
 *
 *  Created on: 2016/01/04
 *      Author: n.kanagawa
 */
#ifndef SMCORETRMEM_H_
#define SMCORETRMEM_H_

//-----------------------------------
// I/F定義
//-----------------------------------
void *SC_TR_Malloc(size_t size);
void SC_TR_Free(void *p);

#endif /* SMCORETRMEM_H_ */
