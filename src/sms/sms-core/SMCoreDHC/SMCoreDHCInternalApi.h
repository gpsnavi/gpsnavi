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
 * SMCoreDHCInternalApi.h
 *
 *  Created on: 2015/11/13
 *      Author: masutani
 */

#ifndef SMCOREDHCINTERNALAPI_H_
#define SMCOREDHCINTERNALAPI_H_

/**
 * DHC_Cash
 */
void DHC_Debug_MemTableAllDump();
void DHC_Debug_MemTableReadUnUseDump();
void DHC_Debug_MemTableReadUseDump();
void DHC_Debug_MemTableCashUnUseDump();
void DHC_Debug_MemTableCashUseDump();

/**
 * DHC_MemAlloc
 */
/* メモリ一括確保 */
E_SC_RESULT DHC_CashMemAlloc(UINT32 memSize);
/* 使用サイズ分のメモリ取得 */
void* DHC_GetCashMemory(UINT32 size);
/* メモリ一括解放 */
void DHC_CashMemFree();

/**
 * DHC_MemMgr
 */
E_SC_RESULT DHC_MemMgrInit();
void* DHC_GetBinMemory(UINT32 size, UINT32* buffSize);
void DHC_ReleaseBinMemory(void* pAddr);
void DHC_ShowMemoryUseVol();

/**
 * DHC_RingBuffer
 */
E_SC_RESULT DHC_RingBufferInit(UINT32 dataSize, UINT32 dataCnt, UINT16 select);
void DHC_RingBufferMngClean(UINT16 select);
void* DHC_GetRingBuffer(UINT16 select);
E_SC_RESULT DHC_SetRingBuffer(void* pAddr, UINT16 select);
UINT32 DHC_GetRingBufferCnt(UINT16 select);
Bool DHC_IsInRingBufferMngArea(void* pAddr, UINT16 select);

#endif /* SMCOREDHCINTERNALAPI_H_ */
