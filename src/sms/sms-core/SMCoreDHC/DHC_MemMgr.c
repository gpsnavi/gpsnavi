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

static INT32 mMemSizeUseMax[8] = {};					// メモリブロック使用最大値保持（チューニング用）

UINT32 g_MaxCashSize;
UINT32 g_MaxReadSize;
// ★リングバッファ変更時はg_MemSizeInfoとE_CHASH_MEM_BLOCKを変更すること。
static T_MemSizeInfo g_MemSizeInfo[] = {
		{ MEM_SIZE_4, MEM_VOL_4, e_CHASH_MEM_BLOCK1 },
		{ MEM_SIZE_15K, MEM_VOL_15K, e_CHASH_MEM_BLOCK2 },
		{ MEM_SIZE_100K, MEM_VOL_100K, e_CHASH_MEM_BLOCK3 },
		{ MEM_SIZE_200K, MEM_VOL_200K, e_CHASH_MEM_BLOCK4 },
		{ MEM_SIZE_400K, MEM_VOL_400K, e_CHASH_MEM_BLOCK5 },
		{ MEM_SIZE_500K, MEM_VOL_500K, e_CHASH_MEM_BLOCK6 },
		{ MEM_SIZE_1M, MEM_VOL_1M, e_CHASH_MEM_BLOCK7 },
		{ MEM_SIZE_3M, MEM_VOL_3M, e_CHASH_MEM_BLOCK8 },
};

/**
 * @brief リングバッファ初期化
 */
E_SC_RESULT DHC_MemMgrInit()
{
	SC_LOG_DebugPrint(SC_TAG_DHC, SC_LOG_START);

	UINT32 i;
	UINT32 memSize = 0;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	// 総サイズ計算
	memSize = DATA_MEM_SIZE;
	for (i = 0; i < e_CHASH_MEM_END; i++) {
		memSize += g_MemSizeInfo[i].size * g_MemSizeInfo[i].vol;
	}
	g_MaxCashSize = (memSize / 3) * 1;
	g_MaxReadSize = (memSize / 3) * 2;

	// メモリ確保
	ret = DHC_CashMemAlloc(memSize);
	if (e_SC_RESULT_SUCCESS != ret) {
		return (ret);
	}

	// リングバッファ初期化
	for (i = 0; i < e_CHASH_MEM_END; i++) {
		ret = DHC_RingBufferInit(g_MemSizeInfo[i].size, g_MemSizeInfo[i].vol, g_MemSizeInfo[i].select);
		if (e_SC_RESULT_SUCCESS != ret) {
			return (ret);
		}
	}

	SC_LOG_DebugPrint(SC_TAG_DHC, SC_LOG_END);
	return (ret);
}

/**
 * @brief サイズに見合うメモリ領域を取得
 * @param size
 * @param buffSize
 */
void* DHC_GetBinMemory(UINT32 size, UINT32* buffSize)
{
	void * addr = NULL;
	UINT32 i;

	if (NULL == buffSize) {
		SC_LOG_ErrorPrint(SC_TAG_DHC, "MemMgr_GetBinMemory badparam");
		return (NULL);
	}

	// 該当サイズ検索
	for (i = 0; i < e_CHASH_MEM_END; i++) {
		if (size <= g_MemSizeInfo[i].size) {
			addr = DHC_GetRingBuffer(g_MemSizeInfo[i].select);
			if( NULL != addr ){
				*buffSize = g_MemSizeInfo[i].size;
#if _SCDHC_SAVE_USESIZE
				if (mMemSizeUseMax[i] < (g_MemSizeInfo[i].vol - DHC_GetRingBufferCnt(i))) {
					mMemSizeUseMax[i] = (g_MemSizeInfo[i].vol - DHC_GetRingBufferCnt(i));
				}
#endif
				break;
			}
		}
	}

	// サイズに一致しない
	if (addr == NULL) {
		*buffSize = 0;
		SC_LOG_ErrorPrint(SC_TAG_DHC, "size unmatch [%d]", size);
	}

	return (addr);
}

/**
 * @brief 不要になったバッファを未使用バッファとして登録する。
 * @param pAddr
 */
void DHC_ReleaseBinMemory(void* pAddr)
{
	UINT32 i;

	if (NULL == pAddr) {
		SC_LOG_ErrorPrint(SC_TAG_DHC, "MemMgr_ReleaseBinMemory badparam");
		return;
	}
	for (i = 0; i < e_CHASH_MEM_END; i++) {
		if (DHC_IsInRingBufferMngArea(pAddr, g_MemSizeInfo[i].select)) {
			DHC_SetRingBuffer(pAddr, g_MemSizeInfo[i].select);
			break;
		}
	}
	if (e_CHASH_MEM_END <= i) {
		// 不正領域
		SC_LOG_ErrorPrint(SC_TAG_DHC, "address unmatch [%p]", pAddr);
	}
}

/**
 * @brief キャッシュ終了処理
 */
void DHC_MemMgrFinish()
{
	SC_LOG_DebugPrint(SC_TAG_DHC, SC_LOG_START);
	UINT32 i;

	for (i = 0; i < e_CHASH_MEM_END; i++) {
		DHC_RingBufferMngClean(g_MemSizeInfo[i].select);
	}

	// メモリ解放処理
	DHC_CashMemFree();

	SC_LOG_DebugPrint(SC_TAG_DHC, SC_LOG_END);
}

/**
 * @brief メモリブロック使用最大数ダンプ
 * @memo チューニング用
 */
void DHC_ShowMemoryUseVol() {
	UINT32 i;
	SC_LOG_InfoPrint(SC_TAG_DHC, " DHC MemoryManager info...");
	for (i = 0; i < e_CHASH_MEM_END; i++) {
		SC_LOG_InfoPrint(SC_TAG_DHC, " memtype=%d bufsize=%7dByte usemax=%3d/%3d ", i, mMemSizeUseMax[i], g_MemSizeInfo[i].size,
				g_MemSizeInfo[i].vol);
	}
}
