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
 * 構造体定義
 */
typedef struct _RingBufferMng {
	void** dataList_p;					// データリスト先頭アドレス
	UINT32 nextGetIdx;				// データ取得位置インデックス
	UINT32 nextSetIdx;				// データ格納位置インデックス
	UINT32 maxDataCnt;					// 最大データ数
	UINT8* start_p;						// データ領域先頭アドレス
	UINT8* end_p;						// データ領域終端アドレス
} T_RingBufferMng;

/*
 * 変数定義
 */
static T_RingBufferMng m_Mng[e_CHASH_MEM_END] = {};

/**
 * @brief リンクバッファメモリ割り当て
 * @param dataSize
 * @param dataCnt
 * @param select
 */
E_SC_RESULT DHC_RingBufferInit(UINT32 dataSize, UINT32 dataCnt, UINT16 select)
{
	SC_LOG_DebugPrint(SC_TAG_DHC, SC_LOG_START);
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	// メンバ変数初期化
	m_Mng[select].nextGetIdx = 0;			// データ取得位置インデックス
	m_Mng[select].nextSetIdx = 0;			// データ格納位置インデックス

	// 最大データ数は+1した値を設定
	m_Mng[select].maxDataCnt = dataCnt + 1;

	// データ領域を確保
	m_Mng[select].start_p = (UINT8*) DHC_GetCashMemory(dataSize * dataCnt);
	if (m_Mng[select].start_p != NULL) {
		m_Mng[select].end_p = m_Mng[select].start_p + dataSize * dataCnt;

		// データリスト領域を確保
		m_Mng[select].dataList_p = (void**) DHC_GetCashMemory(sizeof(void*) * m_Mng[select].maxDataCnt);

		// データリストにアドレスを登録
		if (m_Mng[select].dataList_p != NULL) {
			UINT32 i;
			for (i = 0; i < dataCnt; i++) {
				ret = DHC_SetRingBuffer((m_Mng[select].start_p + i * dataSize), select);
				if (e_SC_RESULT_SUCCESS != ret) {
					break;
				}
			}
		} else {
			ret = e_SC_RESULT_FAIL;
		}
	} else {
		m_Mng[select].end_p = NULL;
		m_Mng[select].dataList_p = NULL;
		ret = e_SC_RESULT_FAIL;
	}

	SC_LOG_DebugPrint(SC_TAG_DHC, "dataSize[%d] dataCnt[%d] select[%d]", dataSize, dataCnt, select);
	SC_LOG_DebugPrint(SC_TAG_DHC, "startAddr[%p] endAddr[%p] dataList[%p]", m_Mng[select].start_p,
			m_Mng[select].end_p, m_Mng[select].dataList_p);
	SC_LOG_DebugPrint(SC_TAG_DHC, SC_LOG_END);

	return (ret);
}

/**
 * @brief 管理情報初期化
 * @param select
 */
void DHC_RingBufferMngClean(UINT16 select)
{
	SC_LOG_DebugPrint(SC_TAG_DHC, SC_LOG_START);

	m_Mng[select].dataList_p = NULL;
	m_Mng[select].start_p = NULL;
	m_Mng[select].end_p = NULL;
	m_Mng[select].maxDataCnt = 0;
	m_Mng[select].nextSetIdx = 0;
	m_Mng[select].nextGetIdx = 0;

	SC_LOG_DebugPrint(SC_TAG_DHC, SC_LOG_END);
}

/**
 * @brief リングバッファ取得
 * @param select
 */
void* DHC_GetRingBuffer(UINT16 select)
{
	void* pAddr = NULL;

	if (DHC_GetRingBufferCnt(select) > 0) {
		// 取得位置のデータを設定
		pAddr = m_Mng[select].dataList_p[m_Mng[select].nextGetIdx];

		// 取得位置を更新
		m_Mng[select].nextGetIdx++;
		if (m_Mng[select].nextGetIdx >= m_Mng[select].maxDataCnt) {
			m_Mng[select].nextGetIdx = 0;
		}
	}

	return (pAddr);
}

/**
 * @brief アドレスをリングバッファに追加する
 * @param pAddr
 * @param select
 */
E_SC_RESULT DHC_SetRingBuffer(void* pAddr, UINT16 select)
{

	if (NULL == pAddr) {
		SC_LOG_ErrorPrint(SC_TAG_DHC, "DHC_SetRingBuffer badparam, " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	if (DHC_GetRingBufferCnt(select) < m_Mng[select].maxDataCnt) {
		// 取得位置のデータを設定
		m_Mng[select].dataList_p[m_Mng[select].nextSetIdx] = pAddr;

		// 取得位置を更新
		m_Mng[select].nextSetIdx++;
		if (m_Mng[select].nextSetIdx >= m_Mng[select].maxDataCnt) {
			m_Mng[select].nextSetIdx = 0;
		}
	} else {
		// 管理可能データ数を超えているためエラー表示のみ
		SC_LOG_ErrorPrint(SC_TAG_DHC, "Buffer Data Max, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief リングバッファ数取得
 * @param select
 */
UINT32 DHC_GetRingBufferCnt(UINT16 select)
{
	UINT32 dataCnt = 0;

	if (m_Mng[select].nextGetIdx > m_Mng[select].nextSetIdx) {
		dataCnt = m_Mng[select].maxDataCnt - m_Mng[select].nextGetIdx + m_Mng[select].nextSetIdx;
	} else {
		dataCnt = m_Mng[select].nextSetIdx - m_Mng[select].nextGetIdx;
	}

//	SC_LOG_DebugPrint(SC_TAG_DHC, "dataCnt[%d] maxDataCnt[%d]nextGetIdx[%d]nextSetIdx[%d]", dataCnt,
//			m_Mng[select].maxDataCnt, m_Mng[select].nextGetIdx, m_Mng[select].nextSetIdx);

	return (dataCnt);
}

/**
 * @brief アドレスの範囲がバッファセクションに該当するかを判定する。
 * @param pAddr
 * @param select
 */
Bool DHC_IsInRingBufferMngArea(void* pAddr, UINT16 select)
{
	Bool b_inArea = false;

	if ((m_Mng[select].start_p <= (UINT8*) pAddr) && ((UINT8*) pAddr < m_Mng[select].end_p)) {
		b_inArea = true;
	}

	return (b_inArea);
}
