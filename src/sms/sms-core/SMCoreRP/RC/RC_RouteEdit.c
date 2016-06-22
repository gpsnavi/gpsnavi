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
 * RC_RouteEdit.c
 *
 *  Created on: 2014/04/01
 *      Author: 70251034
 */

#include "sms-core/SMCoreRP/SMCoreRPInternal.h"

/**
 * @brief	パーセル情報切り取り
 * @param	[I]元推奨経路
 * @param	[O]結果推奨経路
 * @param	[I]切り取り始点
 * @param	[I]切り取り数
 */
static E_SC_RESULT RC_RtCuttingParcel(SC_RP_RouteMng* aBaseRt, SC_RP_RouteMng* aEditRt, UINT8 aSectIdx, UINT8 aSectVol)
{
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT ret = e_SC_RESULT_FAIL;
	SC_RP_ParcelInfo* newBuf = NULL;
	UINT32 totalVol = 0;
	UINT32 idx;
	UINT16 i;

	do {
		if (aEditRt->sectVol < aSectVol) {
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		for (i = 0; i < aSectVol; i++) {
			aEditRt->sectInfo[i].parcelIdx = totalVol;
			aEditRt->sectInfo[i].parcelVol = aBaseRt->sectInfo[i + aSectIdx].parcelVol;
			totalVol += aBaseRt->sectInfo[i + aSectIdx].parcelVol;
		}
		// 領域確保
		newBuf = (SC_RP_ParcelInfo*) RP_MemAlloc(sizeof(SC_RP_ParcelInfo) * totalVol, e_MEM_TYPE_ROUTEMNG);
		if (NULL == newBuf) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[RtEdit] RP_MemAlloc."HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		// 初期化
		RP_Memset0(newBuf, sizeof(SC_RP_ParcelInfo) * totalVol);
		// コピー
		idx = aBaseRt->sectInfo[aSectIdx].parcelIdx;
		RP_Memcpy(newBuf, &aBaseRt->parcelInfo[idx], sizeof(SC_RP_ParcelInfo) * totalVol);
		aEditRt->parcelInfo = newBuf;
		aEditRt->parcelVol = totalVol;

		ret = e_SC_RESULT_SUCCESS;
	} while (0);

	if (ret != e_SC_RESULT_SUCCESS) {
		if (NULL != newBuf) {
			RP_MemFree(newBuf, e_MEM_TYPE_ROUTEMNG);
		}
		SC_LOG_ErrorPrint(SC_TAG_RC, "[RtEdit] Route cut process error. (%x)"HERE, ret);
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (ret);
}

/**
 * @brief	リンク情報切り取り
 * @param	[I]元推奨経路
 * @param	[O]結果推奨経路
 * @param	[I]切り取り始点
 * @param	[I]切り取り数
 */
static E_SC_RESULT RC_RtCuttingLink(SC_RP_RouteMng* aBaseRt, SC_RP_RouteMng* aEditRt, UINT8 aSectIdx, UINT8 aSectVol)
{
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT ret = e_SC_RESULT_FAIL;
	SC_RP_LinkInfo* newBuf = NULL;
	UINT32 totalVol = 0;
	UINT32 idx;
	UINT16 i;

	do {
		if (aEditRt->sectVol < aSectVol) {
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		for (i = 0; i < aSectVol; i++) {
			aEditRt->sectInfo[i].linkIdx = totalVol;
			aEditRt->sectInfo[i].linkVol = aBaseRt->sectInfo[i + aSectIdx].linkVol;
			totalVol += aBaseRt->sectInfo[i + aSectIdx].linkVol;
		}
		// 領域確保
		newBuf = (SC_RP_LinkInfo*) RP_MemAlloc(sizeof(SC_RP_LinkInfo) * totalVol, e_MEM_TYPE_ROUTEMNG);
		if (NULL == newBuf) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[RtEdit] RP_MemAlloc."HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		// 初期化
		RP_Memset0(newBuf, sizeof(SC_RP_LinkInfo) * totalVol);
		// コピー
		idx = aBaseRt->sectInfo[aSectIdx].linkIdx;
		RP_Memcpy(newBuf, &aBaseRt->linkInfo[idx], sizeof(SC_RP_LinkInfo) * totalVol);
		aEditRt->linkInfo = newBuf;
		aEditRt->linkVol = totalVol;

		ret = e_SC_RESULT_SUCCESS;
	} while (0);

	if (ret != e_SC_RESULT_SUCCESS) {
		if (NULL != newBuf) {
			RP_MemFree(newBuf, e_MEM_TYPE_ROUTEMNG);
		}
		SC_LOG_ErrorPrint(SC_TAG_RC, "[RtEdit] Route cut process error. (%x)"HERE, ret);
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (ret);
}

/**
 * @brief	形状情報切り取り
 * @param	[I]元推奨経路
 * @param	[O]結果推奨経路
 * @param	[I]切り取り始点
 * @param	[I]切り取り数
 */
static E_SC_RESULT RC_RtCuttingForm(SC_RP_RouteMng* aBaseRt, SC_RP_RouteMng* aEditRt, UINT8 aSectIdx, UINT8 aSectVol)
{
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT ret = e_SC_RESULT_FAIL;
	SC_RP_FormInfo* newBuf = NULL;
	UINT32 totalVol = 0;
	UINT32 idx;
	UINT16 i;

	do {
		if (aEditRt->sectVol < aSectVol) {
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		for (i = 0; i < aSectVol; i++) {
			aEditRt->sectInfo[i].formIdx = totalVol;
			aEditRt->sectInfo[i].formVol = aBaseRt->sectInfo[i + aSectIdx].formVol;
			totalVol += aBaseRt->sectInfo[i + aSectIdx].formVol;
		}
		// 領域確保
		newBuf = (SC_RP_FormInfo*) RP_MemAlloc(sizeof(SC_RP_FormInfo) * totalVol, e_MEM_TYPE_ROUTEMNG);
		if (NULL == newBuf) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[RtEdit] RP_MemAlloc."HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		// 初期化
		RP_Memset0(newBuf, sizeof(SC_RP_FormInfo) * totalVol);
		// コピー
		idx = aBaseRt->sectInfo[aSectIdx].formIdx;
		RP_Memcpy(newBuf, &aBaseRt->formInfo[idx], sizeof(SC_RP_FormInfo) * totalVol);
		aEditRt->formInfo = newBuf;
		aEditRt->formVol = totalVol;

		ret = e_SC_RESULT_SUCCESS;
	} while (0);

	if (ret != e_SC_RESULT_SUCCESS) {
		if (NULL != newBuf) {
			RP_MemFree(newBuf, e_MEM_TYPE_ROUTEMNG);
		}
		SC_LOG_ErrorPrint(SC_TAG_RC, "[RtEdit] Route cut process error. (%x)"HERE, ret);
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (ret);
}

/**
 * @brief	区間情報切り取り
 * @param	[I]元推奨経路
 * @param	[O]結果推奨経路
 * @param	[I]切り取り始点
 * @param	[I]切り取り数
 */
static E_SC_RESULT RC_RtCuttingSect(SC_RP_RouteMng* aBaseRt, SC_RP_RouteMng* aEditRt, UINT8 aSectIdx, UINT8 aSectVol)
{
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT ret = e_SC_RESULT_FAIL;
	SC_RP_SectInfo* newBuf;
	//UINT32 totalVol = 0;
	//UINT32 idx;
	UINT16 i;

	do {
		newBuf = (SC_RP_SectInfo*) RP_MemAlloc(sizeof(SC_RP_SectInfo) * aSectVol, e_MEM_TYPE_ROUTEMNG);
		if (NULL == newBuf) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[RtEdit] RP_MemAlloc."HERE);
			break;
		}
		RP_Memset0(newBuf, sizeof(SC_RP_SectInfo) * aSectVol);

		// 引き継ぎ情報(リンク・形状・パーセル・規制 に関連したもの以外)
		for (i = 0; i < aSectVol; i++) {
			newBuf[i].sectDist = aBaseRt->sectInfo[i + aSectIdx].sectDist;
			newBuf[i].sectHWDist = aBaseRt->sectInfo[i + aSectIdx].sectHWDist;
			newBuf[i].sectTime = aBaseRt->sectInfo[i + aSectIdx].sectTime;
			newBuf[i].priority = aBaseRt->sectInfo[i + aSectIdx].priority;

			newBuf[i].sectNumber = aBaseRt->sectInfo[i + aSectIdx].sectNumber;
			newBuf[i].splitIdx = aBaseRt->sectInfo[i + aSectIdx].splitIdx;
			newBuf[i].parcelId = aBaseRt->sectInfo[i + aSectIdx].parcelId;
			newBuf[i].x = aBaseRt->sectInfo[i + aSectIdx].x;
			newBuf[i].y = aBaseRt->sectInfo[i + aSectIdx].y;
		}

		// 区間情報
		aEditRt->sectInfo = newBuf;
		aEditRt->sectVol = aSectVol;
		ret = e_SC_RESULT_SUCCESS;
	} while (0);

	if (ret != e_SC_RESULT_SUCCESS) {
		if (NULL != newBuf) {
			RP_MemFree(newBuf, e_MEM_TYPE_ROUTEMNG);
		}
		SC_LOG_ErrorPrint(SC_TAG_RC, "[RtEdit] Route cut process error. (%x)"HERE, ret);
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (ret);
}

/**
 * @brief	区間情報切り取り
 * @param	[I]マスター推奨経路
 * @param	[I]切り取り始点
 * @param	[I]切り取り数
 * @param	[O]切り取り結果区間推奨経路
 */
E_SC_RESULT RC_RtCutting(SC_RP_RouteMng* aBaseRt, UINT8 aSectIdx, UINT8 aSectVol, SC_RP_RouteMng* aResultRt)
{
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SC_RP_RouteMng* editRt = aResultRt;

	do {
		if ((aSectIdx + aSectVol) > aBaseRt->sectVol) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[RtEdit] Bad param."HERE);
			result = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == editRt) {
			result = e_SC_RESULT_BADPARAM;
			break;
		}

		// 管理情報初期化
		RP_Memset0(editRt, sizeof(SC_RP_RouteMng));

		// 管理情報引き継ぎ
		RP_Memcpy(editRt->mapVer, aBaseRt->mapVer, sizeof(Char) * SC_RP_MAPVER_SIZE);
		RP_Memcpy(editRt->routeVer, aBaseRt->routeVer, sizeof(Char) * SC_RP_ROUTEVER_SIZE);

		// 区間情報
		result = RC_RtCuttingSect(aBaseRt, editRt, aSectIdx, aSectVol);
		if (e_SC_RESULT_SUCCESS != result) {
			break;
		}

		// パーセル情報
		result = RC_RtCuttingParcel(aBaseRt, editRt, aSectIdx, aSectVol);
		if (e_SC_RESULT_SUCCESS != result) {
			break;
		}

		// リンク情報
		result = RC_RtCuttingLink(aBaseRt, editRt, aSectIdx, aSectVol);
		if (e_SC_RESULT_SUCCESS != result) {
			break;
		}

		// 形状情報
		result = RC_RtCuttingForm(aBaseRt, editRt, aSectIdx, aSectVol);
		if (e_SC_RESULT_SUCCESS != result) {
			break;
		}
	} while (0);

	if (result != e_SC_RESULT_SUCCESS) {
		if (NULL != editRt->parcelInfo) {
			RP_MemFree(editRt->parcelInfo, e_MEM_TYPE_ROUTEMNG);
		}
		if (NULL != editRt->linkInfo) {
			RP_MemFree(editRt->linkInfo, e_MEM_TYPE_ROUTEMNG);
		}
		if (NULL != editRt->formInfo) {
			RP_MemFree(editRt->formInfo, e_MEM_TYPE_ROUTEMNG);
		}
		if (NULL != editRt->regInfo) {
			RP_MemFree(editRt->regInfo, e_MEM_TYPE_ROUTEMNG);
		}
		// 初期化
		RP_Memset0(editRt, sizeof(SC_RP_RouteMng));
		SC_LOG_ErrorPrint(SC_TAG_RC, "[RtEdit] Route cut process error. (%x)"HERE, result);
	}
#if 0
	// 区間情報変化ダンプ
	UINT32 i;
	if (ret == e_SC_RESULT_SUCCESS) {
		SC_LOG_DebugPrint(SC_TAG_RC, "[RtEdit] Mng pcl[%4d->%4d] link[%4d->%4d] form[%4d->%4d] reg[%4d->%4d]"
				, aBaseRt->parcelVol
				, editRt->parcelVol
				, aBaseRt->linkVol
				, editRt->linkVol
				, aBaseRt->formVol
				, editRt->formVol
				, aBaseRt->regVol
				, editRt->regVol
				);
		for (i = 0; i < aBaseRt->sectVol; i++) {
			if(i < aSectIdx){
				SC_LOG_DebugPrint(SC_TAG_RC, "[RtEdit] No%02d Vol pcl[%4d->----] link[%4d->----] form[%4d->----]"
						, i
						, aBaseRt->sectInfo[i].parcelVol
						, aBaseRt->sectInfo[i].linkVol
						, aBaseRt->sectInfo[i].formVol
						);
				SC_LOG_DebugPrint(SC_TAG_RC, "[RtEdit]      Idx pcl[%4d->----] link[%4d->----] form[%4d->----]"
						, aBaseRt->sectInfo[i].parcelIdx
						, aBaseRt->sectInfo[i].linkIdx
						, aBaseRt->sectInfo[i].formIdx
						);
			} else {
				SC_LOG_DebugPrint(SC_TAG_RC, "[RtEdit] No%02d Vol pcl[%4d->%4d] link[%4d->%4d] form[%4d->%4d]"
						, i
						, aBaseRt->sectInfo[i].parcelVol
						, editRt->sectInfo[i-aSectIdx].parcelVol
						, aBaseRt->sectInfo[i].linkVol
						, editRt->sectInfo[i-aSectIdx].linkVol
						, aBaseRt->sectInfo[i].formVol
						, editRt->sectInfo[i-aSectIdx].formVol
						);
				SC_LOG_DebugPrint(SC_TAG_RC, "[RtEdit]      Idx pcl[%4d->%4d] link[%4d->%4d] form[%4d->%4d]"
						, aBaseRt->sectInfo[i].parcelIdx
						, editRt->sectInfo[i-aSectIdx].parcelIdx
						, aBaseRt->sectInfo[i].linkIdx
						, editRt->sectInfo[i-aSectIdx].linkIdx
						, aBaseRt->sectInfo[i].formIdx
						, editRt->sectInfo[i-aSectIdx].formIdx
						);
			}
		}
	}
	// 推奨経路ダンプ
	//RP_Debug_Dump_Route(resultRt);
#endif

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/*****************************************************************************************************************************/

/**
 * @brief	推奨経路結合(パーセル)
 * @param	[I]結合推奨経路
 * @param	[I]結合対象経路数
 * @param	[O]結合結果推奨経路
 */
static E_SC_RESULT RC_RtJoinParcel(SC_RP_RouteMng** aRtList, UINT8 aRtVol, SC_RP_RouteMng* aEditRt)
{
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT ret = e_SC_RESULT_FAIL;
	SC_RP_ParcelInfo* newBuf;
	UINT32 totalVol = 0;
	UINT32 totalSect = 0;
	UINT32 idx = 0;
	UINT32 sectIdx = 0;
	UINT16 i;
	UINT16 e;

	do {
		for (i = 0; i < aRtVol; i++) {
			totalVol += aRtList[i]->parcelVol;
		}
		// 領域確保
		newBuf = (SC_RP_ParcelInfo*) RP_MemAlloc(sizeof(SC_RP_ParcelInfo) * totalVol, e_MEM_TYPE_ROUTEMNG);
		if (NULL == newBuf) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[RtEdit] RP_MemAlloc."HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		RP_Memset0(newBuf, sizeof(SC_RP_ParcelInfo) * totalVol);

		// INDEX情報，VOL更新
		idx = 0;
		totalSect = 0;
		for (i = 0; i < aRtVol; i++) {
			RP_Memcpy(&newBuf[idx], aRtList[i]->parcelInfo, sizeof(SC_RP_ParcelInfo) * aRtList[i]->parcelVol);
			for (e = 0; e < aRtList[i]->sectVol; e++) {
				sectIdx = e + totalSect;
				aEditRt->sectInfo[sectIdx].parcelIdx = idx;
				aEditRt->sectInfo[sectIdx].parcelVol = aRtList[i]->sectInfo[e].parcelVol;
				idx += aRtList[i]->sectInfo[e].parcelVol;
			}
			totalSect += aRtList[i]->sectVol;
		}

		aEditRt->parcelInfo = newBuf;
		aEditRt->parcelVol = totalVol;
		SC_LOG_DebugPrint(SC_TAG_RC, "[RtEdit] Sect MallocSize. (%d)", sizeof(SC_RP_ParcelInfo) * totalVol);

		ret = e_SC_RESULT_SUCCESS;
	} while (0);

	if (ret != e_SC_RESULT_SUCCESS) {
		if (NULL != newBuf) {
			RP_MemFree(newBuf, e_MEM_TYPE_ROUTEMNG);
		}
		SC_LOG_ErrorPrint(SC_TAG_RC, "[RtEdit] Route joint error. (%x)"HERE, ret);
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (ret);
}

/**
 * @brief	推奨経路結合(リンク)
 * @param	[I]結合推奨経路
 * @param	[I]結合対象経路数
 * @param	[O]結合結果推奨経路
 */
static E_SC_RESULT RC_RtJoinLink(SC_RP_RouteMng** aRtList, UINT8 aRtVol, SC_RP_RouteMng* aEditRt)
{
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT ret = e_SC_RESULT_FAIL;
	SC_RP_LinkInfo* newBuf;
	UINT32 totalVol = 0;
	UINT32 totalSect = 0;
	UINT32 idx = 0;
	UINT32 sectIdx = 0;
	UINT16 i;
	UINT16 e;

	do {
		for (i = 0; i < aRtVol; i++) {
			totalVol += aRtList[i]->linkVol;
		}
		// 領域確保
		newBuf = (SC_RP_LinkInfo*) RP_MemAlloc(sizeof(SC_RP_LinkInfo) * totalVol, e_MEM_TYPE_ROUTEMNG);
		if (NULL == newBuf) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[RtEdit] RP_MemAlloc."HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		RP_Memset0(newBuf, sizeof(SC_RP_LinkInfo) * totalVol);

		// INDEX情報，VOL更新
		idx = 0;
		totalSect = 0;
		for (i = 0; i < aRtVol; i++) {
			RP_Memcpy(&newBuf[idx], aRtList[i]->linkInfo, sizeof(SC_RP_LinkInfo) * aRtList[i]->linkVol);
			for (e = 0; e < aRtList[i]->sectVol; e++) {
				sectIdx = e + totalSect;
				aEditRt->sectInfo[sectIdx].linkIdx = idx;
				aEditRt->sectInfo[sectIdx].linkVol = aRtList[i]->sectInfo[e].linkVol;
				idx += aRtList[i]->sectInfo[e].linkVol;
			}
			totalSect += aRtList[i]->sectVol;
		}

		aEditRt->linkInfo = newBuf;
		aEditRt->linkVol = totalVol;
		SC_LOG_DebugPrint(SC_TAG_RC, "[RtEdit] Sect MallocSize. (%d)", sizeof(SC_RP_LinkInfo) * totalVol);

		ret = e_SC_RESULT_SUCCESS;
	} while (0);

	if (ret != e_SC_RESULT_SUCCESS) {
		if (NULL != newBuf) {
			RP_MemFree(newBuf, e_MEM_TYPE_ROUTEMNG);
		}
		SC_LOG_ErrorPrint(SC_TAG_RC, "[RtEdit] Route joint error. (%x)"HERE, ret);
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (ret);
}

/**
 * @brief	推奨経路結合(形状)
 * @param	[I]結合推奨経路
 * @param	[I]結合対象経路数
 * @param	[O]結合結果推奨経路
 */
static E_SC_RESULT RC_RtJoinForm(SC_RP_RouteMng** aRtList, UINT8 aRtVol, SC_RP_RouteMng* aEditRt)
{
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT ret = e_SC_RESULT_FAIL;
	SC_RP_FormInfo* newBuf;
	UINT32 totalVol = 0;
	UINT32 totalSect = 0;
	UINT32 idx = 0;
	UINT32 sectIdx = 0;
	UINT16 i;
	UINT16 e;

	do {
		for (i = 0; i < aRtVol; i++) {
			totalVol += aRtList[i]->formVol;
		}
		// 領域確保
		newBuf = (SC_RP_FormInfo*) RP_MemAlloc(sizeof(SC_RP_FormInfo) * totalVol, e_MEM_TYPE_ROUTEMNG);
		if (NULL == newBuf) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[RtEdit] RP_MemAlloc."HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		RP_Memset0(newBuf, sizeof(SC_RP_FormInfo) * totalVol);

		// INDEX情報，VOL更新
		idx = 0;
		totalSect = 0;
		for (i = 0; i < aRtVol; i++) {
			RP_Memcpy(&newBuf[idx], aRtList[i]->formInfo, sizeof(SC_RP_FormInfo) * aRtList[i]->formVol);
			for (e = 0; e < aRtList[i]->sectVol; e++) {
				sectIdx = e + totalSect;
				aEditRt->sectInfo[sectIdx].formIdx = idx;
				aEditRt->sectInfo[sectIdx].formVol = aRtList[i]->sectInfo[e].formVol;
				idx += aRtList[i]->sectInfo[e].formVol;
			}
			totalSect += aRtList[i]->sectVol;
		}

		aEditRt->formInfo = newBuf;
		aEditRt->formVol = totalVol;
		SC_LOG_DebugPrint(SC_TAG_RC, "[RtEdit] Sect MallocSize. (%d)", sizeof(SC_RP_FormInfo) * totalVol);

		ret = e_SC_RESULT_SUCCESS;
	} while (0);

	if (ret != e_SC_RESULT_SUCCESS) {
		if (NULL != newBuf) {
			RP_MemFree(newBuf, e_MEM_TYPE_ROUTEMNG);
		}
		SC_LOG_ErrorPrint(SC_TAG_RC, "[RtEdit] Route joint error. (%x)"HERE, ret);
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (ret);
}

/**
 * @brief	推奨経路結合(区間)
 * @param	[I]結合推奨経路
 * @param	[I]結合対象経路数
 * @param	[O]結合結果推奨経路
 */
static E_SC_RESULT RC_RtJoinSect(SC_RP_RouteMng** aRtList, UINT8 aRtVol, SC_RP_RouteMng* aEditRt)
{
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT ret = e_SC_RESULT_FAIL;
	SC_RP_SectInfo* newBuf;
	UINT32 totalVol = 0;
	UINT32 totalSect = 0;
	UINT32 sectIdx = 0;
	UINT16 i;
	UINT16 e;

	do {
		for (i = 0; i < aRtVol; i++) {
			totalVol += aRtList[i]->sectVol;
		}
		// 領域確保
		newBuf = (SC_RP_SectInfo*) RP_MemAlloc(sizeof(SC_RP_SectInfo) * totalVol, e_MEM_TYPE_ROUTEMNG);
		if (NULL == newBuf) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[RtEdit] RP_MemAlloc."HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		RP_Memset0(newBuf, sizeof(SC_RP_SectInfo) * totalVol);

		// INDEX情報，VOL更新
		totalSect = 0;
		for (i = 0; i < aRtVol; i++) {
			for (e = 0; e < aRtList[i]->sectVol; e++) {
				sectIdx = e + totalSect;
				newBuf[sectIdx].sectDist = aRtList[i]->sectInfo[e].sectDist;
				newBuf[sectIdx].sectHWDist = aRtList[i]->sectInfo[e].sectHWDist;
				newBuf[sectIdx].sectTime = aRtList[i]->sectInfo[e].sectTime;
				newBuf[sectIdx].parcelId = aRtList[i]->sectInfo[e].parcelId;
				newBuf[sectIdx].x = aRtList[i]->sectInfo[e].x;
				newBuf[sectIdx].y = aRtList[i]->sectInfo[e].y;
				newBuf[sectIdx].splitIdx = aRtList[i]->sectInfo[e].splitIdx;
			}
			totalSect += aRtList[i]->sectVol;
		}

		aEditRt->sectInfo = newBuf;
		aEditRt->sectVol = totalVol;
		SC_LOG_DebugPrint(SC_TAG_RC, "[RtEdit] Sect MallocSize. (%d)", sizeof(SC_RP_SectInfo) * totalVol);

		ret = e_SC_RESULT_SUCCESS;
	} while (0);

	if (ret != e_SC_RESULT_SUCCESS) {
		if (NULL != newBuf) {
			RP_MemFree(newBuf, e_MEM_TYPE_ROUTEMNG);
		}
		SC_LOG_ErrorPrint(SC_TAG_RC, "[RtEdit] Route joint error. (%x)"HERE, ret);
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (ret);
}

/**
 * @brief	推奨経路結合
 * @param	[I]結合推奨経路
 * @param	[I]結合対象経路数
 * @param	[O]結合結果推奨経路
 */
E_SC_RESULT RC_RtJoin(SC_RP_RouteMng** aRtList, UINT16 aRtVol, SC_RP_RouteMng* aResultRt)
{
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SC_RP_RouteMng* editRt = aResultRt;
	UINT32 i;

	do {
		if (NULL == aRtList || NULL == aResultRt || 2 > aRtVol) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
			result = e_SC_RESULT_BADPARAM;
			break;
		}

		for (i = 0; i < aRtVol - 1; i++) {
			if (0 != memcmp(aRtList[i]->mapVer, aRtList[i + 1]->mapVer, sizeof(Char) * SC_RP_MAPVER_SIZE)) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "map version unmatch. [%s]!=[%s] "HERE, aRtList[i]->mapVer, aRtList[i + 1]->mapVer);
				result = e_SC_RESULT_BADPARAM;
				break;
			}
			if (0 != memcmp(aRtList[i]->routeVer, aRtList[i + 1]->routeVer, sizeof(Char) * SC_RP_ROUTEVER_SIZE)) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "route version unmatch. [%s]!=[%s] "HERE, aRtList[i]->routeVer, aRtList[i + 1]->routeVer);
				result = e_SC_RESULT_BADPARAM;
				break;
			}
		}
		if (e_SC_RESULT_SUCCESS != result) {
			break;
		}

		// 管理情報初期化
		RP_Memset0(editRt, sizeof(SC_RP_RouteMng));

		// 管理情報引き継ぎ TODO Ver違い
		RP_Memcpy(editRt->mapVer, aRtList[0]->mapVer, sizeof(Char) * SC_RP_MAPVER_SIZE);
		RP_Memcpy(editRt->routeVer, aRtList[0]->routeVer, sizeof(Char) * SC_RP_ROUTEVER_SIZE);

		// 区間情報
		result = RC_RtJoinSect(aRtList, aRtVol, editRt);
		if (e_SC_RESULT_SUCCESS != result) {
			break;
		}

		// パーセル情報
		result = RC_RtJoinParcel(aRtList, aRtVol, editRt);
		if (e_SC_RESULT_SUCCESS != result) {
			break;
		}

		// リンク情報
		result = RC_RtJoinLink(aRtList, aRtVol, editRt);
		if (e_SC_RESULT_SUCCESS != result) {
			break;
		}

		// 形状情報
		result = RC_RtJoinForm(aRtList, aRtVol, editRt);
		if (e_SC_RESULT_SUCCESS != result) {
			break;
		}
	} while (0);

	if (result != e_SC_RESULT_SUCCESS) {
		if (NULL != editRt->parcelInfo) {
			RP_MemFree(editRt->parcelInfo, e_MEM_TYPE_ROUTEMNG);
		}
		if (NULL != editRt->linkInfo) {
			RP_MemFree(editRt->linkInfo, e_MEM_TYPE_ROUTEMNG);
		}
		if (NULL != editRt->formInfo) {
			RP_MemFree(editRt->formInfo, e_MEM_TYPE_ROUTEMNG);
		}
		if (NULL != editRt->sectInfo) {
			RP_MemFree(editRt->sectInfo, e_MEM_TYPE_ROUTEMNG);
		}
		// 管理情報初期化
		RP_Memset0(editRt, sizeof(SC_RP_RouteMng));
		SC_LOG_ErrorPrint(SC_TAG_RC, "[RtEdit] Route joint error."HERE);
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}


