/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

/*-------------------------------------------------------------------
 * File：RP_MemJack.c
 * Info：RCRM共通メモリIF
 *-------------------------------------------------------------------*/

#include "SMCoreRPInternal.h"

/**
 * @brief メモリ取得
 * @param size メモリサイズ
 * @param type メモリ種別
 */
void* RP_MemAlloc(size_t size, E_SC_MEM_TYPE type) {
#if _RP_LOG_MEMINFO
	void* mem = SC_MEM_Alloc(size, type);
	if (NULL == mem) {
		SC_MEM_Dump_Type(type);
	}
	return (mem);
#else
	return (SC_MEM_Alloc(size, type));
#endif
}

/**
 * @brief メモリ開放
 * @param mem メモリアドレス
 * @param type メモリ種別
 */
void RP_MemFree(void* mem, E_SC_MEM_TYPE type) {
	SC_MEM_Free(mem, type);
}

/**
 * @brief メモリ全開放
 * @param type メモリタイプ
 */
void RP_MemClean(E_SC_MEM_TYPE type) {
#if _RP_LOG_MEMINFO
	SC_LOG_DebugPrint(SC_TAG_RC, "SC_MEM_FreeAll. type %x", type);
	SC_MEM_Dump_Type(type);
	SC_MEM_FreeAll(type);
#else
	SC_MEM_FreeAll(type);
#endif
}

/**
 * @brief メモリ初期化
 * @param mem メモリアドレス
 * @param size サイズ
 */
void* RP_Memset0(void* mem, size_t size) {
	return (memset(mem, 0, size));
}

/**
 * @brief メモリコピー
 * @param des コピー先アドレス
 * @param src コピー元アドレス
 * @param size コピーサイズ
 */
void RP_Memcpy(void* des, const void* src, size_t size) {
	memcpy(des, src, size);
}
