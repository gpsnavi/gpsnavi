/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_MEM_H
#define SMCCOM_MEM_H

//-----------------------------------
// メモリサイズ定義
//-----------------------------------
#define	SCC_MEM_SIZE_DYNAMIC		(1024 * 1024 * 25)	// 動的確保メモリサイズ
#define	SCC_MEM_SIZE_DYNAMIC2		(1024 * 1024 * 20)	// 動的確保メモリサイズ

//-----------------------------------
// 列挙型定義
//-----------------------------------
/**
 * @brief 動的メモリ利用のカテゴリをあらわす。
 */
typedef enum _E_SCC_MEM_TYPE {
    CC_MEM_TYPE_DYNAMIC = 0,		// 動的メモリ
    CC_MEM_TYPE_DYNAMIC2,			// 動的メモリ

    CC_MEM_TYPE_END 				// 終端マーク
} E_SCC_MEM_TYPE;

//-----------------------------------
// 構造体定義
//-----------------------------------

//-----------------------------------
// 外部I/F定義
//-----------------------------------
#ifdef __cplusplus
extern "C" {
#endif	// #ifdef __cplusplus
E_SC_RESULT SCC_MEM_Initialize(SIZE size, E_SCC_MEM_TYPE type);
E_SC_RESULT SCC_MEM_Finalize(E_SCC_MEM_TYPE type);
void *SCC_MEM_Alloc(SIZE size, E_SCC_MEM_TYPE type);
void SCC_MEM_Free(void *ptr, E_SCC_MEM_TYPE type);
void SCC_MEM_FreeAll(E_SCC_MEM_TYPE type);
void SCC_MEM_Dump();
void SCC_MEM_Dump_Type(E_SCC_MEM_TYPE type);
#ifdef __cplusplus
}
#endif	// #ifdef __cplusplus

#endif // #ifndef SMCCOM_MEM_H
