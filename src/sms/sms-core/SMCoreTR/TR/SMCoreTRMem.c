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
 * SMCoreTRMem.c
 *
 *  Created on: 2016/01/04
 *      Author: n.kanagawa
 */
#include "sms-core/SMCoreTR/SMCoreTRInternal.h"


/**
 * @brief	メモリ確保
 * @param	[I]size_t		サイズ
 * @return	確保メモリアドレス
 */
void *SC_TR_Malloc(size_t size)
{
	return (malloc(size));
}

/**
 * @brief	メモリ解放
 * @param	[I]p		解放アドレス
 */
void SC_TR_Free(void *p)
{
	if (NULL != p) {
		free(p);
	}
}
