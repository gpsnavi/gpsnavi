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
 * RP_Heap.c
 *
 *  Created on: 2015/11/06
 *      Author: masutani
 */

#include "sms-core/SMCoreRP/SMCoreRPInternal.h"

/**
 * @brief ヒープ追加
 * @param ネットワーク管理
 * @param 追加データ
 * @memo 性能確保のためパラメタチェック等は行わない
 */
void RC_HeapInsert(SCRP_NETCONTROLER* aNetCtrl, UINT32 aData) {
	UINT32* heapTop = aNetCtrl->heap.heap;
	SCRP_NETDATA* mamLink = NULL;
	SCRP_NETDATA* chiLink = NULL;
	SCRP_NETDATA* lLink = NULL;
	SCRP_NETDATA* rLink = NULL;
	UINT32 current = aNetCtrl->heap.heapEnd;
	UINT32 up = 0;

	//末尾に追加
	*(heapTop + current) = aData;
	mamLink = RCNET_GET_HEAPNETDATA(aNetCtrl, aData);
	mamLink->heap = current;

	//Insertデータのソート
	while (1) {
		if (0 == current) {
			break;
		}
		if (0 == (current % 2)) {
			//左枝と比較
			lLink = RCNET_GET_HEAPNETDATA(aNetCtrl, *(heapTop + (current - 1)));
			rLink = RCNET_GET_HEAPNETDATA(aNetCtrl, *(heapTop + current));
			if (lLink->costSum < rLink->costSum) {
				//end!!
				break;
			} else {
				up = (current - 1) / 2;
			}
		} else if (aNetCtrl->heap.heapEnd != current) {
			//右枝と比較
			lLink = RCNET_GET_HEAPNETDATA(aNetCtrl, *(heapTop + current));
			rLink = RCNET_GET_HEAPNETDATA(aNetCtrl, *(heapTop + (current + 1)));
			if (lLink->costSum < rLink->costSum) {
				//交換
				up = current / 2;
			} else {
				//end!!
				break;
			}
		} else {
			up = current / 2;
		}
		//親と比較
		mamLink = RCNET_GET_HEAPNETDATA(aNetCtrl, *(heapTop + up));
		chiLink = RCNET_GET_HEAPNETDATA(aNetCtrl, *(heapTop + current));
		if (mamLink->costSum > chiLink->costSum) {
			//交換
			swap(UINT32, *(heapTop + up), *(heapTop + current));
			swap(UINT32, mamLink->heap, chiLink->heap);
			current = up;
		} else {
			//end!!
			break;
		}
	}
	aNetCtrl->heap.heapEnd++;
}

/**
 * @brief ヒープ削除
 * @param ネットワーク管理
 * @param 削除データ
 * @memo 削除データは０番目固定０番目以外はロジック違う
 * @memo 性能確保のためパラメタチェック等は行わない
 */
void RC_HeapDelete(SCRP_NETCONTROLER* aNetCtrl, UINT32 aData) {
	UINT32* heapTop = aNetCtrl->heap.heap;
	SCRP_NETDATA* mamLink = NULL;
	SCRP_NETDATA* lLink = NULL;
	SCRP_NETDATA* rLink = NULL;
	UINT32 current = 0;
	UINT32 down = 0;

	//先頭に末尾交換
	mamLink = RCNET_GET_HEAPNETDATA(aNetCtrl, *heapTop);
	mamLink->heap = SCRP_HEAP_V;

	aNetCtrl->heap.heapEnd--;
	if (0 == aNetCtrl->heap.heapEnd) {
		return;
	}
	*heapTop = *(heapTop + aNetCtrl->heap.heapEnd);
	mamLink = RCNET_GET_HEAPNETDATA(aNetCtrl, *heapTop);
	mamLink->heap = 0;

	while (1) {
		//末端超え確認
		if (aNetCtrl->heap.heapEnd < (current * 2 + 2)) {
			break;
		} else if (aNetCtrl->heap.heapEnd == (current * 2 + 2)) {
			down = (current * 2 + 1);

			//子と比較
			mamLink = RCNET_GET_HEAPNETDATA(aNetCtrl, *(heapTop + current));
			lLink = RCNET_GET_HEAPNETDATA(aNetCtrl, *(heapTop + down));
			if (lLink->costSum < mamLink->costSum) {
				swap(UINT32, *(heapTop + current), *(heapTop + down));
				swap(UINT32, mamLink->heap, lLink->heap);
				current = down;
			} else {
				break;
			}
		} else if (aNetCtrl->heap.heapEnd > (current * 2 + 2)) {
			down = (current * 2 + 1);

			//子と比較
			mamLink = RCNET_GET_HEAPNETDATA(aNetCtrl, *(heapTop + current));
			lLink = RCNET_GET_HEAPNETDATA(aNetCtrl, *(heapTop + down));
			rLink = RCNET_GET_HEAPNETDATA(aNetCtrl, *(heapTop + (down + 1)));
			if (lLink->costSum > rLink->costSum) {
				if (rLink->costSum < mamLink->costSum) {
					swap(UINT32, *(heapTop + current), *(heapTop + down + 1));
					swap(UINT32, mamLink->heap, rLink->heap);
					current = down + 1;
				} else {
					break;
				}
			} else {
				if (lLink->costSum < mamLink->costSum) {
					swap(UINT32, *(heapTop + current), *(heapTop + down));
					swap(UINT32, mamLink->heap, lLink->heap);
					current = down;
				} else {
					break;
				}
			}
		} else {
			//end!!
			break;
		}
	}
}

/**
 * @brief ヒープ更新
 * @param ネットワーク管理
 * @param 更新データ
 * @memo 性能確保のためパラメタチェック等は行わない。
 */
void RC_HeapUpdate(SCRP_NETCONTROLER* aNetCtrl, UINT32 aData) {
	UINT32* heapTop = aNetCtrl->heap.heap;
	SCRP_NETDATA* mamLink = NULL;
	SCRP_NETDATA* chiLink = NULL;
	SCRP_NETDATA* lLink = NULL;
	SCRP_NETDATA* rLink = NULL;
	UINT32 current = 0;
	UINT32 up = 0;

	chiLink = RCNET_GET_HEAPNETDATA(aNetCtrl, aData);
	current = chiLink->heap;

	while (1) {
		if (0 == current) {
			break;
		}
		if (0 == (current % 2)) {
			//左枝と比較
			lLink = RCNET_GET_HEAPNETDATA(aNetCtrl, *(heapTop + (current - 1)));
			rLink = RCNET_GET_HEAPNETDATA(aNetCtrl, *(heapTop + current));
			if (lLink->costSum < rLink->costSum) {
				//end!!
				break;
			} else {
				up = (current - 1) / 2;
			}
		} else if (aNetCtrl->heap.heapEnd != current) {
			//右枝と比較
			lLink = RCNET_GET_HEAPNETDATA(aNetCtrl, *(heapTop + current));
			rLink = RCNET_GET_HEAPNETDATA(aNetCtrl, *(heapTop + (current + 1)));
			if (lLink->costSum < rLink->costSum) {
				up = current / 2;
			} else {
				//end!!
				break;
			}
		} else {
			up = current / 2;
		}
		//親と比較
		mamLink = RCNET_GET_HEAPNETDATA(aNetCtrl, *(heapTop + up));
		chiLink = RCNET_GET_HEAPNETDATA(aNetCtrl, *(heapTop + current));
		if (mamLink->costSum > chiLink->costSum) {
			//交換
			swap(UINT32, *(heapTop + up), *(heapTop + current));
			swap(UINT32, mamLink->heap, chiLink->heap);
			current = up;
		} else {
			//end!!
			break;
		}
	}
}

/**
 * @brief ヒープテーブル領域確保
 * @param ネットワーク管理
 */
E_SC_RESULT RC_MemAllocHeapTable(SCRP_NETCONTROLER* aNetCtrl) {

	if (NULL == aNetCtrl) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	aNetCtrl->heap.heap = RP_MemAlloc(sizeof(UINT32) * SCRP_HEAP_SIZE, e_MEM_TYPE_ROUTEPLAN);
	if (NULL == aNetCtrl->heap.heap) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error. "HERE);
		return (e_SC_RESULT_MALLOC_ERR);
	}
	RP_Memset0(aNetCtrl->heap.heap, sizeof(UINT32) * SCRP_HEAP_SIZE);
	aNetCtrl->heap.heapEnd = 0;
	aNetCtrl->heap.heapSize = SCRP_HEAP_SIZE;

	return (e_SC_RESULT_SUCCESS);
}
