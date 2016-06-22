/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#include "SMCoreDHCInternal.h"

/*
 * 変数定義
 */
static UINT8* m_startAddr;		// 確保したメモリの先頭アドレス
static UINT8* m_nonUsedAddr;	// 確保したメモリの未使用領域先頭アドレス
static UINT8* m_endAddr;		// 確保したメモリの終端アドレス

/*
 * マクロ定義
 */
#define DHC_MEMADDR_DMP()																		\
		SC_LOG_DebugPrint(SC_TAG_DHC, "m_startAddr[%p] m_nonUsedAddr[%p] m_endAddr[%p]" HERE,	\
			m_startAddr, m_nonUsedAddr, m_endAddr)

/**
 * @brief キャッシュメモリ確保
 * @param memSize
 * @memo  先頭からDATA_MEM_SIZE分はデータ保持用メモリに割り当て
 */
E_SC_RESULT DHC_CashMemAlloc(UINT32 memSize)
{
	SC_LOG_DebugPrint(SC_TAG_DHC, SC_LOG_START);
	SC_LOG_DebugPrint(SC_TAG_DHC, "CashBuffer MemAlloc memSize[%d]", memSize);

	if (m_startAddr != NULL) {
		SC_MEM_Free(m_startAddr, e_MEM_TYPE_CASH);
	}

	// キャッシュ領域取得
	m_startAddr = (UINT8*) SC_MEM_Alloc((size_t) memSize, e_MEM_TYPE_CASH);
	if(NULL == m_startAddr){
		return (e_SC_RESULT_MALLOC_ERR);
	}

	// 管理アドレス設定
	m_nonUsedAddr = m_startAddr;
	m_endAddr = m_startAddr + memSize;

	DHC_MEMADDR_DMP();

	SC_LOG_DebugPrint(SC_TAG_DHC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief サイズ分のメモリ取得
 * @param size
 */
void* DHC_GetCashMemory(UINT32 size)
{
	void * addr = NULL;
	UINT32 boundSize = (size + 3) / 4 * 4;

	if ((m_nonUsedAddr + boundSize) < m_endAddr) {
		addr = m_nonUsedAddr;
		m_nonUsedAddr = m_nonUsedAddr + boundSize;
		//DHC_MEMADDR_DMP();
	} else {
		DHC_MEMADDR_DMP();
		SC_LOG_DebugPrint(SC_TAG_DHC, "GetMemory size[%d] boundSize[%d]" HERE, size, boundSize);
		SC_LOG_ErrorPrint(SC_TAG_DHC, "Malloc Error, " HERE);
	}

	return (addr);
}

/**
 * @brief キャッシュメモリ解放処理
 */
void DHC_CashMemFree()
{
	SC_LOG_DebugPrint(SC_TAG_DHC, SC_LOG_START);

	DHC_MEMADDR_DMP();

	if (m_startAddr != NULL) {
		SC_MEM_Free(m_startAddr, e_MEM_TYPE_CASH);
	}

	m_startAddr = NULL;
	m_nonUsedAddr = NULL;
	m_endAddr = NULL;

	SC_LOG_DebugPrint(SC_TAG_DHC, SC_LOG_END);
}
