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
 * RP_LinkLevelConvert.c
 *
 *  Created on: 2016/01/25
 *      Author: masutani
 */

#include "sms-core/SMCoreRP/SMCoreRPInternal.h"

#define SCRP_WKFORM_BLOCKSIZE				(200)		// 形状作業領域確保ブロックサイズ
#define SCRP_RESLINK_BLOCKSIZE				(200)		// 形状作業領域確保ブロックサイズ
typedef struct _SCRP_LVCHANGE_LINKINFO {
	UINT32 linkId;							// リンクID
	UINT32 formOfs;							// 形状オフセット
	UINT16 formVol;							// 開始座標X
} SCRP_LVCHANGE_LINKINFO;

typedef struct _SCRP_LVCHANGE_FORMINFO {
	UINT32 parcelId;						// パーセルID
	UINT32 linkId;							// リンクID
	UINT32 formOfs;							// 形状オフセット
	UINT16 stX;								// 開始座標X
	UINT16 stY;								// 開始座標Y
	UINT16 edX;								// 終端座標X
	UINT16 edY;								// 終端座標Y
	UINT32 upLinkId;						// 上位リンクID
	UINT16 index;							// インデックス？？？
} SCRP_LVCHANGE_FORMINFO;

typedef struct _SCRP_LVCHANGE_FORMTBL {
	SCRP_LVCHANGE_FORMINFO* formList;		// 確保領域
	UINT32 formListvol;						// 確保領域数
	UINT32 formListCrnt;					// 使用位置
} SCRP_LVCHANGE_FORMTBL;

static E_SC_RESULT makeLowLevelFormList(UINT32 aTargetParcelId, UINT32 aTargetLinkId, SCRP_LVCHANGE_FORMTBL* aFormTbl,
		SCRP_LVCHANGE_TBL* aLvChangeTbl, SCRP_LVCHANGE_RES* aResultSubLink);
static E_SC_RESULT getLowLevelFormList(MAL_HDL aShape, UINT32 aUpperLinkId, UINT16 aDiffX, UINT16 aDiffY, UINT32 aLowParcelId,
		SCRP_LVCHANGE_FORMTBL* aFormTab);
static E_SC_RESULT getDownStLinkId(SCRP_MAPREADTBL* aLv2MapTbl, UINT32 aLv2LinkId, UINT32* aLv1LinkId, UINT32* aLv1ParcelId);
static E_SC_RESULT mallocFormWorkTab(SCRP_LVCHANGE_FORMTBL* aFormTbl, UINT32 aAddSize);
static E_SC_RESULT mallocResLinkInfo(SCRP_LVCHANGE_TBL* aLvChangeTbl, UINT32 aAddSize);

/**
 * @brief リンクレベル変換テーブル解放
 * @param 変換テーブル
 * @param 結果情報格納領域メモリタイプ指定
 */
E_SC_RESULT RP_LinkLevelConvertTblFree(SCRP_LVCHANGE_TBL* aLvChangeTbl, E_SC_MEM_TYPE aMemType) {
	if (NULL == aLvChangeTbl) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	if (NULL != aLvChangeTbl->linkId) {
		RP_MemFree(aLvChangeTbl->linkId, aMemType);
		aLvChangeTbl->linkId = NULL;
	}
	if (NULL != aLvChangeTbl->resLinkInfo) {
		RP_MemFree(aLvChangeTbl->resLinkInfo, aMemType);
		aLvChangeTbl->resLinkInfo = NULL;
	}
	aLvChangeTbl->linkIdVol = 0;
	aLvChangeTbl->resLinkInfoVol = 0;
	aLvChangeTbl->parcelId = 0;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief リンクレベル変換
 * @param 変換テーブル
 * @param 結果情報格納領域メモリタイプ指定
 * @memo 1本でも変換に失敗した場合エラー応答を行う
 */
E_SC_RESULT RP_LinkLevelConvert(SCRP_LVCHANGE_TBL* aLvChangeTbl, E_SC_MEM_TYPE aMemType) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aLvChangeTbl) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SCRP_MAPREADTBL lv2MapTbl = {};
	SCRP_MAPREADTBL lv1MapTbl = {};
	SCRP_PCLRECT lv1PclRect = {};
	UINT32 lv2LinkId = 0;
	//UINT32 lv2LinkStX = 0;
	UINT32 lv2FormOfs = 0;
	//UINT32 lv2RoadIdx = 0;
	UINT32 lv2ParcelId = aLvChangeTbl->parcelId;
	UINT32 lv1ParcelId = SC_MESH_GetUnderLevelParcelID(aLvChangeTbl->parcelId, RP_LEVEL1);
	UINT32 lv1StLinkId = 0;
	UINT32 lv1StLinkParcelId = 0;
	UINT32 totalSubLinkVol = 0;
	UINT32 i, e;
	UINT32 x, y;
	SCRP_LVCHANGE_FORMTBL wkTab = {};

#if 0
	SC_LOG_InfoPrint(SC_TAG_RC, "lv2=0x%08x lv1=0x%08x. linkvol=%d ", aLvChangeTbl->parcelId, lv1ParcelId, aLvChangeTbl->linkIdVol);
	for (i = 0; i < aLvChangeTbl->linkIdVol; i++) {
		SC_LOG_InfoPrint(SC_TAG_RC, "level up convert request link=0x%08x. ", *(aLvChangeTbl->linkId + i));
	}
#endif

	do {
		// レベル２形状データ読み込み
		UINT32 kind = SC_DHC_GetKindMask(e_DATA_KIND_ROAD) | SC_DHC_GetKindMask(e_DATA_KIND_SHAPE);
		result = RC_ReadListMap(&lv2ParcelId, 1, kind, &lv2MapTbl);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RC_ReadListMap error. [0x%08x] "HERE, result);
			break;
		}
		if (NULL == lv2MapTbl.mapList->shape || NULL == lv2MapTbl.mapList->road) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "map of shape read error. pcl=0x%08x "HERE, lv2ParcelId);
			result = e_SC_RESULT_FAIL;
			break;
		}
		// レベル２対応レベル１全パーセル読み込み
		lv1PclRect.parcelId = lv1ParcelId;
		lv1PclRect.xSize = 4;
		lv1PclRect.ySize = 4;
		result = RC_ReadAreaMap(&lv1PclRect, SC_DHC_GetKindMask(e_DATA_KIND_SHAPE), &lv1MapTbl);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RC_ReadAreaMap error. [0x%08x] "HERE, result);
			break;
		}

		for (i = 0; i < aLvChangeTbl->linkIdVol; i++) {

			// 形状作業領域・使用位置 初期化
			wkTab.formListCrnt = 0;
			if (NULL != wkTab.formList) {
				RP_Memset0(wkTab.formList, sizeof(SCRP_LVCHANGE_FORMINFO) * wkTab.formListvol);
			}

			// 対象リンクID取得
			lv2LinkId = *(aLvChangeTbl->linkId + i);

			// ネットワークから下位接続開始リンクID取得
			result = getDownStLinkId(&lv2MapTbl, lv2LinkId, &lv1StLinkId, &lv1StLinkParcelId);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "getDownStLinkId error. [0x%08x] "HERE, result);
				break;
			}

			// 形状索引から形状オフセット取得
			result = SC_MA_BinSearchShapeIndex(lv2MapTbl.mapList->shape, lv2LinkId, &lv2FormOfs);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "SC_MA_BinSearchShapeIndex error. "HERE, result);
				break;
			}
			// 形状取得
			MAL_HDL lv2Form = SC_MA_GetMapSharpRecord(lv2MapTbl.mapList->shape) + lv2FormOfs;

			/* 形状のXYから下位存在範囲をある程度特定する */
			UINT16 minX, minY, maxX, maxY, xyVol;
			xyVol = SC_MA_D_SHRCD_GET_XYVOL(lv2Form);
			for (e = 0; e < xyVol; e++) {
				if (0 == e) {
					minX = SC_MA_D_SHRCD_GET_XY_X(lv2Form, e);
					maxX = SC_MA_D_SHRCD_GET_XY_X(lv2Form, e);
					minY = SC_MA_D_SHRCD_GET_XY_Y(lv2Form, e);
					maxY = SC_MA_D_SHRCD_GET_XY_Y(lv2Form, e);
					continue;
				}
				x = SC_MA_D_SHRCD_GET_XY_X(lv2Form, e);
				y = SC_MA_D_SHRCD_GET_XY_Y(lv2Form, e);
				if (x < minX) {
					minX = x;
				} else if (maxX < x) {
					maxX = x;
				}
				if (y < minY) {
					minY = y;
				} else if (maxY < y) {
					maxY = y;
				}
			}
			// 座標点補正上位座標が丸められている場合があるため1ずつ広げる
			if (0 < minX) {
				minX--;
			}
			if (0 < minY) {
				minY--;
			}
			if (4096 > maxX) {
				maxX++;
			}
			if (4096 > maxY) {
				maxY++;
			}
			// 形状から下位最大最少パーセルシフト量取得
			UINT16 lbXSft, lbYSft, rtXSft, rtYSft;
			lbXSft = (minX * 4) / 4096;
			lbYSft = (minY * 4) / 4096;
			rtXSft = (maxX * 4) / 4096;
			rtYSft = (maxY * 4) / 4096;

			for (y = 0; y < 4; y++) {
				for (x = 0; x < 4; x++) {
					if (lbXSft <= x && x <= rtXSft && lbYSft <= y && y <= rtYSft) {
						// 可能性ありパーセル
						UINT32 targetPclId = SC_MESH_SftParcelId(lv1ParcelId, x, y);
						UINT16 diffX = x * 4096;
						UINT16 diffY = y * 4096;
						MAL_HDL shape = (lv1MapTbl.mapList + (y * 4 + x))->shape;

						// 地図なしは次
						if (NULL == shape) {
							continue;
						}
						result = getLowLevelFormList(shape, lv2LinkId, diffX, diffY, targetPclId, &wkTab);
						if (e_SC_RESULT_SUCCESS != result) {
							SC_LOG_ErrorPrint(SC_TAG_RC, "getLowLevelFormList error. [0x%08x] "HERE, result);
							break;
						}
					}
				}
				if (e_SC_RESULT_SUCCESS != result) {
					break;
				}
			}
			if (e_SC_RESULT_SUCCESS != result) {
				break;
			}
			if (0 == wkTab.formListCrnt) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "down convert link vol is 0... "HERE);
				result = e_SC_RESULT_FAIL;
				break;
			}

			// 応答領域確保
			if (aLvChangeTbl->resLinkInfoVol < totalSubLinkVol + wkTab.formListCrnt) {
				result = mallocResLinkInfo(aLvChangeTbl, ((totalSubLinkVol + wkTab.formListCrnt) - aLvChangeTbl->resLinkInfoVol));
				if (e_SC_RESULT_SUCCESS != result) {
					break;
				}
			}

			// レベル１でのリンク方向を並べ替える
			result = makeLowLevelFormList(lv1StLinkParcelId, lv1StLinkId, &wkTab, aLvChangeTbl,
					aLvChangeTbl->resLinkInfo + totalSubLinkVol);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "makeLowLevelFormList error. [0x%08x] "HERE, result);
				break;
			}
			// 総Lv1リンク数
			totalSubLinkVol += wkTab.formListCrnt;
		}

		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "error. "HERE);
			break;
		}
		if (0 == totalSubLinkVol) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "level convert link count zero... "HERE);
			result = e_SC_RESULT_FAIL;
			break;
		}

		// 最終結果領域をトリム
		SCRP_LVCHANGE_RES* latestLinkInfo = RP_MemAlloc(sizeof(SCRP_LVCHANGE_RES) * totalSubLinkVol, e_MEM_TYPE_ROUTEPLAN);
		if (NULL == latestLinkInfo) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error. "HERE);
			result = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		RP_Memcpy(latestLinkInfo, aLvChangeTbl->resLinkInfo, sizeof(SCRP_LVCHANGE_RES) * totalSubLinkVol);
		RP_MemFree(aLvChangeTbl->resLinkInfo, e_MEM_TYPE_ROUTEPLAN);
		aLvChangeTbl->resLinkInfo = latestLinkInfo;
		aLvChangeTbl->resLinkInfoVol = totalSubLinkVol;

#if 0
		// 全テーブルダンプ
		for (i = 0; i < totalSubLinkVol; i++) {
			SC_LOG_DebugPrint(SC_TAG_RC, "pcl=0x%08x link=0x%08x formOfs=0x%08x ", (aLvChangeTbl->resLinkInfo + i)->parcelId,
					(aLvChangeTbl->resLinkInfo + i)->linkId, (aLvChangeTbl->resLinkInfo + i)->formOfs);
		}
#endif
	} while (0);

	if (e_SC_RESULT_SUCCESS != RC_FreeMapTbl(&lv2MapTbl, true)) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "RC_FreeMapTbl error. [0x%08x] "HERE, result);
	}
	if (e_SC_RESULT_SUCCESS != RC_FreeMapTbl(&lv1MapTbl, true)) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "RC_FreeMapTbl error. [0x%08x] "HERE, result);
	}
	if (NULL != wkTab.formList) {
		RP_MemFree(wkTab.formList, e_MEM_TYPE_ROUTEPLAN);
	}
	if (e_SC_RESULT_SUCCESS != result && NULL != aLvChangeTbl->resLinkInfo) {
		RP_MemFree(aLvChangeTbl->resLinkInfo, e_MEM_TYPE_ROUTEPLAN);
		aLvChangeTbl->resLinkInfo = NULL;
		aLvChangeTbl->resLinkInfoVol = 0;
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief 下位リンク形状リスト(最終結果)取得
 */
static E_SC_RESULT makeLowLevelFormList(UINT32 aTargetParcelId, UINT32 aTargetLinkId, SCRP_LVCHANGE_FORMTBL* aFormTbl,
		SCRP_LVCHANGE_TBL* aLvChangeTbl, SCRP_LVCHANGE_RES* aResultSubLink) {

	if (NULL == aFormTbl || NULL == aLvChangeTbl) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	UINT16 targetOr = SC_MA_D_NWID_GET_SUB_CNCTSIDE(aTargetLinkId); // リンク方向取得
	//UINT16 x = 0, y = 0;
	UINT16 setIdx = 0;
	UINT16 crntOr = 0;
	UINT16 nextOr = 0;
	UINT32 i, e;
	UINT16 stFind = 0;
	SCRP_LVCHANGE_FORMINFO* crntInfo = NULL;
	SCRP_LVCHANGE_FORMINFO* nextInfo = NULL;

#if 0
	SC_LOG_InfoPrint(SC_TAG_RC, "param. "HERE);
	for (e = 0; e < aFormTbl->formListCrnt; e++) {
		SCRP_LVCHANGE_FORMINFO* test = aFormTbl->formList + e;
		SC_LOG_InfoPrint(SC_TAG_RC, "init!!  pcl=0x%08x link=0x%08x st=%5d,%5d ed=%5d,%5d index=%d", test->parcelId, test->linkId, test->stX, test->stY, test->edX, test->edY, test->index);
	}
#endif
	for (i = 0; i < aFormTbl->formListCrnt; i++) {
		crntInfo = (aFormTbl->formList + i);

		// リンク一致判定
		if (aTargetParcelId != crntInfo->parcelId) {
			continue;
		}
		if (SC_MA_D_NWID_GET_PNT_ID(aTargetLinkId) != SC_MA_D_NWID_GET_PNT_ID(crntInfo->linkId)) {
			continue;
		}
		// 順逆
		if (targetOr == 1) {
			if (1 == SC_MA_D_NWID_GET_SUB_CNCTSIDE(crntInfo->upLinkId)) {
				crntOr = 1;
			} else {
				crntOr = 2;
			}
		} else {
			if (1 == SC_MA_D_NWID_GET_SUB_CNCTSIDE(crntInfo->upLinkId)) {
				crntOr = 2;
			} else {
				crntOr = 1;
			}
		}
		(aResultSubLink + setIdx)->linkId = crntInfo->linkId;
		(aResultSubLink + setIdx)->formOfs = crntInfo->formOfs;
		(aResultSubLink + setIdx)->parcelId = crntInfo->parcelId;
		if (crntOr == 1) {
			(aResultSubLink + setIdx)->linkId |= 0x02000000;
		} else {
			(aResultSubLink + setIdx)->linkId |= 0x04000000;
		}
		crntInfo->index = 0;
		stFind += 1;
		setIdx += 1;
		if (1 == aFormTbl->formListCrnt) {
			// 1リンクの場合以後の処理は不要
			break;
		}
#if 0
	SC_LOG_InfoPrint(SC_TAG_RC, "ppp. target=%d crnt=%d uplink0x%08x ", targetOr, crntOr, crntInfo->upLinkId);
	for (e = 0; e < aFormTbl->formListCrnt; e++) {
		SCRP_LVCHANGE_FORMINFO* test = aFormTbl->formList + e;
		SC_LOG_InfoPrint(SC_TAG_RC, "enit!!  pcl=0x%08x link=0x%08x st=%5d,%5d ed=%5d,%5d index=%d", test->parcelId, test->linkId, test->stX, test->stY, test->edX, test->edY, test->index);
	}
#endif

		while (setIdx < aFormTbl->formListCrnt) {
			for (e = 0; e < aFormTbl->formListCrnt; e++) {
				nextInfo = (aFormTbl->formList + e);
				if (ALL_F16 != nextInfo->index) {
					// 設定済み
					continue;
				}
				// 一致する座標が存在するか確認（座標は補正済みのものとする）
				nextOr = 0;
				if (1 == crntOr) {
					if (crntInfo->edX == nextInfo->stX && crntInfo->edY == nextInfo->stY) {
						nextOr = 1;
					}
					if (crntInfo->edX == nextInfo->edX && crntInfo->edY == nextInfo->edY) {
						nextOr = 2;
					}
				} else {
					if (crntInfo->stX == nextInfo->stX && crntInfo->stY == nextInfo->stY) {
						nextOr = 1;
					}
					if (crntInfo->stX == nextInfo->edX && crntInfo->stY == nextInfo->edY) {
						nextOr = 2;
					}
				}
				if (0 == nextOr) {
					continue;
				}
				// 一致した形状データを格納する
				(aResultSubLink + setIdx)->linkId = nextInfo->linkId;
				(aResultSubLink + setIdx)->formOfs = nextInfo->formOfs;
				(aResultSubLink + setIdx)->parcelId = nextInfo->parcelId;
				if (nextOr == 1) {
					(aResultSubLink + setIdx)->linkId |= 0x02000000;
				} else {
					(aResultSubLink + setIdx)->linkId |= 0x04000000;
				}
				nextInfo->index = e;
				setIdx += 1;

				// 次のために情報を詰めておく
				crntInfo = nextInfo;
				crntOr = nextOr;
				break;
			}
			if (aFormTbl->formListCrnt <= e) {
				SC_LOG_DebugPrint(SC_TAG_RC, "not found connect link... pcl=0x%08x link=0x%08x "HERE, crntInfo->parcelId, crntInfo->linkId);
				break;
			}
		}
		// 結果判定
		if (setIdx < aFormTbl->formListCrnt) {
			if (1 == stFind) {
				// 接続先が見つからない場合経路方向終端側である可能性（いろいろ初期化して終了）
				setIdx = 0;
				for (e = 0; e < aFormTbl->formListCrnt; e++) {
					(aFormTbl->formList + e)->index = ALL_F16;
				}
				SC_LOG_DebugPrint(SC_TAG_RC, "not found link... pcl=0x%08x link=0x%08x "HERE, aTargetParcelId, aTargetLinkId);
			} else {
				// 何かおかしい
				SC_LOG_ErrorPrint(SC_TAG_RC, "unknown error. pcl=0x%08x link=0x%08x "HERE, aTargetParcelId, aTargetLinkId);
				result = e_SC_RESULT_FAIL;
				break;
			}
		} else {
			// 全要素格納->終了
			break;
		}
	}

	// 設定なしの場合エラーとする
	if (0 == setIdx) {
#if 0
		SC_LOG_ErrorPrint(SC_TAG_RC, "error. setidx=%d ", setIdx);
		for (i = 0; i < aFormTbl->formListCrnt; i++) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "link=0x%08x pcl=0x%08x sublink=0x%08x ", aTargetLinkId, (aFormTbl->formList + i)->parcelId,
					(aFormTbl->formList + i)->linkId);
		}
#endif
		SC_LOG_ErrorPrint(SC_TAG_RC, "low level link list set error. pcl=0x%08x link=0x%08x "HERE, aTargetParcelId, aTargetLinkId);
		result = e_SC_RESULT_FAIL;
	}

	return (result);
}

/**
 * @brief 下位リンク形状情報リスト取得
 * @param 形状データ
 * @param 上位リンクID
 * @param [O]結果格納用形状テーブル
 */
static E_SC_RESULT getLowLevelFormList(MAL_HDL aShape, UINT32 aUpperLinkId, UINT16 aDiffX, UINT16 aDiffY, UINT32 aLowParcelId,
		SCRP_LVCHANGE_FORMTBL* aFormTab) {

	if (NULL == aShape || NULL == aFormTab) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	UINT32 upperLinkIdx = 0;
	UINT32 upperIdx = 0;
	UINT32 upperVol = 0;
	MAL_HDL upperIdxRec = SC_MA_A_SHBIN_GET_LV2UPPER_IDXLINK(aShape);
	MAL_HDL upperRec = SC_MA_A_SHBIN_GET_UPPER_LINK(aShape);
	MAL_HDL formTop = SC_MA_GetMapSharpRecord(aShape);
	UINT32 i;

	do {
		// 上位リンクID索引レコードから検索
		result = SC_MA_BinSearchShapeUpperIndex(aShape, aUpperLinkId, &upperLinkIdx);
		if (e_SC_RESULT_SUCCESS != result) {
			// 見つからないことは許容する
			//SC_LOG_InfoPrint(SC_TAG_RC, "SC_MA_BinSearchShapeUpperIndex. upper link can't found. [0x%08x] "HERE, result);
			result = e_SC_RESULT_SUCCESS;
			break;
		}

		// 上位該当リンク関連の情報を収集
		upperIdx = SC_MA_GET_SHBIN_IDXUPLINK_UPIDX(upperIdxRec, upperLinkIdx);
		upperVol = SC_MA_GET_SHBIN_IDXUPLINK_UPVOL(upperIdxRec, upperLinkIdx);

		if (aFormTab->formListvol < aFormTab->formListCrnt + upperVol) {
			result = mallocFormWorkTab(aFormTab, ((aFormTab->formListCrnt + upperVol) - aFormTab->formListvol));
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "mallocFormWorkTab error. [0x%08x] "HERE, result);
				break;
			}
		}

		for (i = 0; i < upperVol; i++) {
			UINT32 linkId = SC_MA_GET_SHBIN_UPLINK_ID(upperRec, upperIdx + i - 1);
			UINT32 formOfs = 0;

			// リンクID索引レコードから検索
			result = SC_MA_BinSearchShapeIndex(aShape, linkId, &formOfs);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "SC_MA_BinSearchShapeIndex error. [0x%08x] "HERE, result);
				break;
			}
			// 形状データ取得
			MAL_HDL targetForm = formTop + formOfs;

			// 結果格納
			SCRP_LVCHANGE_FORMINFO* info = aFormTab->formList + aFormTab->formListCrnt;
			info->index = ALL_F16;
			info->formOfs = formOfs / 4; // 元の値に戻す
			info->linkId = SC_MA_D_SHRCD_GET_LINKID(targetForm);
			info->parcelId = aLowParcelId;
			UINT32 xyVol = SC_MA_D_SHRCD_GET_XYVOL(targetForm);
			info->stX = aDiffX + SC_MA_D_SHRCD_GET_XY_X(targetForm, 0);
			info->stY = aDiffY + SC_MA_D_SHRCD_GET_XY_Y(targetForm, 0);
			info->edX = aDiffX + SC_MA_D_SHRCD_GET_XY_X(targetForm, xyVol - 1);
			info->edY = aDiffY + SC_MA_D_SHRCD_GET_XY_Y(targetForm, xyVol - 1);

			// 上位接続リンク情報格納 TODO レベル２固定
			if (0 != SC_MA_D_SHRCD_GET_UPLINKVOL(targetForm)) {
				info->upLinkId = read4byte(SC_MA_A_SHRCD_GET_UPLINK(targetForm));
			}
			// 使用位置更新
			aFormTab->formListCrnt++;
#if 0
			SC_LOG_InfoPrint(SC_TAG_RC, " corectUnderLink upLink=0x%08x Idx=0x%08x lowLink=0x%08x lowPcl=0x%08x uplink2=0x%08x count=%d"
					, aUpperLinkId
					, upperLinkIdx
					, info->linkId
					, info->parcelId
					, info->upLinkId
					, SC_MA_D_SHRCD_GET_UPLINKVOL(targetForm)
					);
#endif
		}
	} while (0);

	return (result);
}

/**
 * @brief レベル２リンクの下位接続開始リンクを取得する
 */
static E_SC_RESULT getDownStLinkId(SCRP_MAPREADTBL* aLv2MapTbl, UINT32 aLv2LinkId, UINT32* aLv1LinkId, UINT32* aLv1ParcelId) {

	if (NULL == aLv2MapTbl || NULL == aLv1LinkId || NULL == aLv1ParcelId) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	MAL_HDL pRoad = aLv2MapTbl->mapList->road;
	MAL_HDL lv2RoadEx = NULL;
	MAL_HDL lv2Road = NULL;
	UINT16 lv2RoadIdx = 0;
	UINT32 resultLv1LinkId = 0;
	UINT32 resultLv1ParcelId = 0;
	UINT32 lv1LbParcelId = 0;
	UINT8 sftX = 0, sftY = 0;

	do {
		// リンクID検索
		lv2RoadIdx = SC_MA_BinSearchNwRecord(pRoad, aLv2LinkId, SC_MA_BINSRC_TYPE_LINK);
		if (MA_ALLF16 == lv2RoadIdx) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "SC_MA_BinSearchNwRecord error. "HERE);
			result = e_SC_RESULT_FAIL;
			break;
		}
		// 拡張先頭取得
		lv2RoadEx = SC_MA_A_NWBIN_GET_NWLINKEX(pRoad);
		// リンク情報取得
		lv2Road = SC_MA_A_NWRCD_LINK_GET_RECORD(SC_MA_A_NWBIN_GET_NWRCD_LINK(pRoad), lv2RoadIdx - 1);
		// リンク拡張フラグ確認
		if (!SC_MA_D_NWRCD_LINK_GET_LINKEXOFSFLG(lv2Road)) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "link has no exoffset... "HERE);
			result = e_SC_RESULT_FAIL;
			break;
		}
		// リンク拡張データ取得
		lv2RoadEx = SC_MA_A_NWRCD_LINKEX_GET_RECORD(lv2RoadEx, SC_MA_D_NWRCD_LINK_GET_EXOFS(lv2Road));
		// 下位接続フラグ確認
		if (!SC_MA_D_NWRCD_EXLINK_GET_FLG_DOWNLV(lv2RoadEx)) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "exlink has no downlvflg... "HERE);
			result = e_SC_RESULT_FAIL;
			break;
		}
		// 下位接続リンクID取得
		MAL_HDL downLink = SC_MA_A_NWRCD_EXLINK_GET_DOWNLVLINK(lv2RoadEx);
		lv1LbParcelId = SC_MESH_GetUnderLevelParcelID(aLv2MapTbl->mapList->parcelId, RP_LEVEL1);
		if (1 == SC_MA_D_NWID_GET_SUB_CNCTSIDE(aLv2LinkId)) {
			// 方向は経路方向に対しての方向へ読み替える
			resultLv1LinkId = SC_MA_A_NWRCD_EXLINK_GET_DOWNLVSTLINK(downLink) & 0xF9FFFFFF;
			resultLv1LinkId |= 0x02000000;
			sftX = SC_MA_A_NWRCD_EXLINK_GET_DOWNLVSTLINK_SFTX(downLink);
			sftY = SC_MA_A_NWRCD_EXLINK_GET_DOWNLVSTLINK_SFTY(downLink);
			resultLv1ParcelId = SC_MESH_SftParcelId(lv1LbParcelId, sftX, sftY);
		} else {
			// 方向は経路方向に対しての方向へ読み替える
			resultLv1LinkId = SC_MA_A_NWRCD_EXLINK_GET_DOWNLVEDLINK(downLink) & 0xF9FFFFFF;
			resultLv1LinkId |= 0x04000000;
			sftX = SC_MA_A_NWRCD_EXLINK_GET_DOWNLVEDLINK_SFTX(downLink);
			sftY = SC_MA_A_NWRCD_EXLINK_GET_DOWNLVEDLINK_SFTY(downLink);
			resultLv1ParcelId = SC_MESH_SftParcelId(lv1LbParcelId, sftX, sftY);
		}

		// 結果格納
		*aLv1LinkId = resultLv1LinkId;
		*aLv1ParcelId = resultLv1ParcelId;

	} while (0);

	return (result);
}

/**
 * @brief 形状作業領域メモリ確保
 * @param 形状作業テーブル
 * @param 追加サイズ
 */
static E_SC_RESULT mallocFormWorkTab(SCRP_LVCHANGE_FORMTBL* aFormTbl, UINT32 aAddSize) {

	if (NULL == aFormTbl) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	// ブロック単位へまとめる
	if (SCRP_WKFORM_BLOCKSIZE > aAddSize) {
		aAddSize = SCRP_WKFORM_BLOCKSIZE;
	}

	UINT32 newSize = aFormTbl->formListvol + aAddSize;
	SCRP_LVCHANGE_FORMINFO* newBuf = RP_MemAlloc(sizeof(SCRP_LVCHANGE_FORMINFO) * newSize, e_MEM_TYPE_ROUTEPLAN);
	if (NULL == newBuf) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error. "HERE);
		return (e_SC_RESULT_MALLOC_ERR);
	}
	// 0クリア
	RP_Memset0(newBuf, sizeof(SCRP_LVCHANGE_FORMINFO) * newSize);

	if (0 < aFormTbl->formListvol) {
		RP_Memcpy(newBuf, aFormTbl->formList, sizeof(SCRP_LVCHANGE_FORMINFO) * aFormTbl->formListvol);
		RP_MemFree(aFormTbl->formList, e_MEM_TYPE_ROUTEPLAN);
	}

	// 結果
	aFormTbl->formList = newBuf;
	aFormTbl->formListvol = newSize;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 形状作業領域メモリ確保
 * @param レベル変換テーブル
 * @param 追加サイズ
 */
static E_SC_RESULT mallocResLinkInfo(SCRP_LVCHANGE_TBL* aLvChangeTbl, UINT32 aAddSize) {

	if (NULL == aLvChangeTbl) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	// ブロック単位へまとめる
	if (SCRP_RESLINK_BLOCKSIZE > aAddSize) {
		aAddSize = SCRP_RESLINK_BLOCKSIZE;
	}

	UINT32 newSize = aLvChangeTbl->resLinkInfoVol + aAddSize;
	SCRP_LVCHANGE_RES* newBuf = RP_MemAlloc(sizeof(SCRP_LVCHANGE_RES) * newSize, e_MEM_TYPE_ROUTEPLAN);
	if (NULL == newBuf) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error. "HERE);
		return (e_SC_RESULT_MALLOC_ERR);
	}
	// 0クリア
	RP_Memset0(newBuf, sizeof(SCRP_LVCHANGE_RES) * newSize);

	if (0 < aLvChangeTbl->resLinkInfoVol) {
		RP_Memcpy(newBuf, aLvChangeTbl->resLinkInfo, sizeof(SCRP_LVCHANGE_RES) * aLvChangeTbl->resLinkInfoVol);
		RP_MemFree(aLvChangeTbl->resLinkInfo, e_MEM_TYPE_ROUTEPLAN);
	}

	// 結果
	aLvChangeTbl->resLinkInfo = newBuf;
	aLvChangeTbl->resLinkInfoVol = newSize;

	return (e_SC_RESULT_SUCCESS);
}
