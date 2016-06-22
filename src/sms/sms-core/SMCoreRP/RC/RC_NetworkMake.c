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
 * RP_NetworkMake.c
 *
 *  Created on: 2015/11/06
 *      Author: masutani
 */

#include "sms-core/SMCoreRP/SMCoreRPInternal.h"

// 接続先リンク方向フラグ設定
#define RP_GET_CNCTSIDE(id)		(((SC_MA_NWID_SUB_CNCTSIDE_RVS & id) == SC_MA_NWID_SUB_CNCTSIDE_RVS) ? 1 : 0)
/* リンクテーブル確保ブロック
 * TODO 確保領域調節 */
#define RP_NETLINK_BLOCKSIZE (200000)

static E_SC_RESULT mallocParcelInfo(SCRP_NETCONTROLER *aNetCtrl, UINT16 aSize);
static E_SC_RESULT mallocLinkInfo(SCRP_NETCONTROLER *aNetCtrl, UINT32 aSize);
static E_SC_RESULT makeParcelInfo(SCRP_NETCONTROLER *aNetCtrl, SCRP_MAPREADTBL* aMapRead, UINT32 *aTotalLinkVol);
static E_SC_RESULT makeLinkInfo(SCRP_NETCONTROLER *aNetCtrl, SCRP_PCLRECT* aLowRect, E_RP_RTCALCSTEP aStep);
static E_SC_RESULT makeLinkInfoForId(SCRP_NETCONTROLER *aNetCtrl, SCRP_PCLRECT* aLowRect, E_RP_RTCALCSTEP aStep);
static E_SC_RESULT makeLinkInfoForCnct(SCRP_NETCONTROLER *aNetCtrl);
static E_SC_RESULT setLinkCnctIndex(SCRP_NETCONTROLER *aNetCtrl);
static E_SC_RESULT setAreaCandLinkFlag(SCRP_NETCONTROLER *aNetCtrl, E_RP_RTCALCSTEP aStep);
static E_SC_RESULT setDestParcelFlag(SCRP_NETCONTROLER *aNetCtrl, SCRP_SECTCONTROLER *aSectCtrl);
static E_SC_RESULT setOuterMostParcelFlag(SCRP_SECTCONTROLER *aSectCtrl, SCRP_NETCONTROLER *aNetCtrl);
static E_SC_RESULT setDownConnectParcelFlag(SCRP_SECTCONTROLER *aSectCtrl, SCRP_NETCONTROLER *aNetCtrl);
static Bool isDownConnectLink(UINT32 aLv2PclId, MAL_HDL aLinkEx, UINT8 aOr, SCRP_PCLRECT* aLowRect);
static SCRP_PCLINFO* searchParcelInfo(SCRP_NETCONTROLER *aNetCtrl, UINT32 aParcelId);
static Bool judgeCalcLink(T_MapBaseLinkInfo *aBaseLinkInfo);
static UINT32 getNextParcelID(UINT32 aParcelId, UINT8 aDir);

#if _ROUTECALC_QUICK_SAMPLE
static E_SC_RESULT trimP2PLink(SCRP_NETCONTROLER *aNetCtrl);
static E_SC_RESULT trimNextConnectLink(SCRP_NETCONTROLER*, SCRP_NETDATA*);
#endif

/**
 * @brief 目的地リンクフラグ設定処理
 * @param ネットワーク管理
 * @param 近傍テーブル
 */
E_SC_RESULT RC_SetDestLinkFlag(SCRP_NETCONTROLER *aNetCtrl, SCRP_NEIGHBORINFO* aNeighbor) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SCRP_NEIGHBORLINK* nbrLink;
	SCRP_LINKINFO* linkTab;
	UINT32 i, e;
	UINT16 hitCnt = 0;

	if (NULL == aNetCtrl || NULL == aNeighbor) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	nbrLink = aNeighbor->neighborLink;
	for (i = 0; i < aNeighbor->nbrLinkVol; i++, nbrLink++) {
		if (nbrLink->noUse) {
			continue;
		}

		// リンクテーブル検索
		linkTab = NULL;
		for (e = 0; e < aNetCtrl->parcelInfoVol; e++) {
			if (!RCPI_GET_DESTFLG(aNetCtrl->parcelInfo[e].flag)) {
				continue;
			}
			if (nbrLink->parcelId == aNetCtrl->parcelInfo[e].parcelId) {
				linkTab = aNetCtrl->linkTable + aNetCtrl->parcelInfo[e].linkIdx + nbrLink->dataIndex;
				break;
			}
		}
		if (NULL == linkTab) {
			continue;
		}

		// データチェック
		if (linkTab->detaIndex != nbrLink->dataIndex) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "network/neighbor link index is fraud. [%d!=%d]. "HERE, nbrLink->dataIndex, linkTab->detaIndex);
			result = e_SC_RESULT_FAIL;
			break;
		}
		// リンク情報更新
		linkTab->linkNet[nbrLink->orFlag].flag |= RCND_DESTLINK;
#if 0
		SC_LOG_InfoPrint(SC_TAG_RC, "set dest link flg. pcl=0x%08x link=0x%08x "HERE, nbrLink->parcelId, nbrLink->linkId);
#endif

		// 発見
		hitCnt++;
	}

	// ログ出力のみ
	if (0 == hitCnt) {
		SC_LOG_WarnPrint(SC_TAG_RC, "destLink not found... "HERE);
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief 探索情報テーブル構築
 * @param ネットワーク管理
 * @param 区間管理情報
 */
E_SC_RESULT RC_MakeNetwork(SCRP_NETCONTROLER *aNetCtrl, SCRP_SECTCONTROLER* aSectCtrl, E_RP_RTCALCSTEP aStep) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	// プロセス登録
	RPC_SetCalcProcess(e_RC_PROC_MAKENETWORK);

	if (NULL == aSectCtrl || NULL == aNetCtrl || NULL == aNetCtrl->levelTbl->divInfo) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SCRP_MAPREADTBL readTab = {};
	SCRP_DIVAREA *divArea = aNetCtrl->levelTbl->divInfo + (aNetCtrl->levelArea->divIdx + aNetCtrl->calculatingDivIdx);

	do {
#if _RPLAPTIME_MAKENET // ▲時間計測
		RP_SetLapTime();
#endif
		if (0 == divArea->pclVol) {
			result = e_SC_RESULT_BADPARAM;
			SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
			break;
		}
		// 地図読み込み
		result = RC_ReadAreaMap(&divArea->pclRect, SC_DHC_GetKindMask(e_DATA_KIND_ROAD), &readTab);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "RC_ReadAreaMap error. [0x%08x] "HERE, result);
			break;
		}
		// 地図が1枚も読み込めない場合（例：海上，未ダウンロード）
		if (0 == readTab.mapVol) {
			SC_LOG_DebugPrint(SC_TAG_RC, "read parcel size zero accept.");
			return (e_SC_RESULT_SUCCESS);
		}
		// パーセル数 over flow
		if (SCRP_PCLINFO_MAX < readTab.mapVol) {
			result = e_SC_RESULT_FAIL;
			SC_LOG_ErrorPrint(SC_TAG_RC, "parcel size over flow. total=%d "HERE, readTab.mapVol);
			break;
		}

#if _RPLAPTIME_MAKENET // ▲時間計測
		RP_SetLapTimeWithStr("netmake-mapread");
#endif
		// パーセルテーブル領域確保
		result = mallocParcelInfo(aNetCtrl, readTab.mapVol);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "mallocParcelInfo error. [0x%08x] "HERE, result);
			break;
		}

		// パーセルテーブル構築
		UINT32 totalLinkVol = 0;
		result = makeParcelInfo(aNetCtrl, &readTab, &totalLinkVol);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "makeParcelInfo error. [0x%08x] "HERE, result);
			break;
		}
		// リンクが1本もない
		if (0 == totalLinkVol) {
			SC_LOG_DebugPrint(SC_TAG_RC, "read link size zero accept.");
			result = e_SC_RESULT_SUCCESS;
			break;
		}

		// 下位接続パーセルフラグ設定
		if (e_RP_STEP_LV2TOP == aStep) {
			result = setDownConnectParcelFlag(aSectCtrl, aNetCtrl);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "setDownConnectParcelFlag error. [0x%08x] "HERE, result);
				break;
			}
		}
		// 最外殻パーセルフラグ設定
		if (e_RP_STEP_LV1O == aStep) {
			result = setOuterMostParcelFlag(aSectCtrl, aNetCtrl);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "setOuterMostParcelFlag error. [0x%08x] "HERE, result);
				break;
			}
		}

#if _RPLAPTIME_MAKENET // ▲時間計測
		RP_SetLapTime();
#endif
		// リンクテーブル領域確保
		result = mallocLinkInfo(aNetCtrl, totalLinkVol);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "mallocLinkInfo error. [0x%08x] "HERE, result);
			break;
		}

		// リンクテーブル構築
		if (e_RP_STEP_LV2TOP == aStep) {
			result = makeLinkInfo(aNetCtrl, &aSectCtrl->levelTable.areaTable[2].pclRect, aStep);
		} else {
			result = makeLinkInfo(aNetCtrl, NULL, aStep);
		}
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "makeLinkInfo error. [0x%08x] "HERE, result);
			break;
		}

#if _RPLAPTIME_MAKENET // ▲時間計測
		RP_SetLapTimeWithStr("netmake-linkmake");
#endif
		// 目的地フラグ設定
		if (e_RP_STEP_LV1TOP == aStep || e_RP_STEP_LV1D == aStep) {
			result = setDestParcelFlag(aNetCtrl, aSectCtrl);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "setDestParcelFlag error. [0x%08x] "HERE, result);
				break;
			}
		}

		// 候補用リンクフラグ設定
		result = setAreaCandLinkFlag(aNetCtrl, aStep);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "setAreaCandLinkFlag error. [0x%08x] "HERE, result);
			break;
		}

	} while (0);

	if (NULL != readTab.mapList) {
		RP_MemFree(readTab.mapList, e_MEM_TYPE_ROUTEPLAN);
	}

#if _RP_LOG_NETWORK
	RPDBG_ShowNetwork(aNetCtrl);
#endif

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief パーセルテーブル領域確保
 * @param ネットワーク管理
 * @param 確保サイズ
 */
static E_SC_RESULT mallocParcelInfo(SCRP_NETCONTROLER *aNetCtrl, UINT16 aSize) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aNetCtrl || 0 == aSize) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. size=%d "HERE, aSize);
		return (e_SC_RESULT_BADPARAM);
	}

	//領域確保
	aNetCtrl->parcelInfo = RP_MemAlloc(sizeof(SCRP_PCLINFO) * aSize, e_MEM_TYPE_ROUTEPLAN);
	if (NULL == aNetCtrl->parcelInfo) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error. "HERE);
		return (e_SC_RESULT_MALLOC_ERR);
	}
	aNetCtrl->parcelInfoVol = aSize;
	// 0クリア
	RP_Memset0(aNetCtrl->parcelInfo, sizeof(SCRP_PCLINFO) * aSize);

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief リンクテーブル領域確保
 * @param ネットワーク管理
 * @param 確保サイズ
 */
static E_SC_RESULT mallocLinkInfo(SCRP_NETCONTROLER *aNetCtrl, UINT32 aSize) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aNetCtrl || 0 == aSize) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. size=%d "HERE, aSize);
		return (e_SC_RESULT_BADPARAM);
	}

	// 領域確保
	if (RP_NETLINK_BLOCKSIZE < aSize) {
		aNetCtrl->linkTable = RP_MemAlloc(sizeof(SCRP_LINKINFO) * aSize, e_MEM_TYPE_ROUTEPLAN);
	} else {
		aNetCtrl->linkTable = RP_MemAlloc(sizeof(SCRP_LINKINFO) * RP_NETLINK_BLOCKSIZE, e_MEM_TYPE_ROUTEPLAN);
	}
	if (NULL == aNetCtrl->linkTable) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "RP_MemAlloc error. "HERE);
		return (e_SC_RESULT_MALLOC_ERR);
	}
	aNetCtrl->linkTableVol = aSize;
	// 0クリア
	RP_Memset0(aNetCtrl->linkTable, sizeof(SCRP_LINKINFO) * aSize);

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief パーセルテーブル初期情報設定
 * @param ネットワーク管理
 * @param 地図読み込みテーブル
 * @param [O]総リンク数
 */
static E_SC_RESULT makeParcelInfo(SCRP_NETCONTROLER *aNetCtrl, SCRP_MAPREADTBL* aMapRead, UINT32 *aTotalLinkVol) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aNetCtrl || NULL == aMapRead || NULL == aTotalLinkVol) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SCRP_DIVAREA *divArea = aNetCtrl->levelTbl->divInfo + (aNetCtrl->levelArea->divIdx + aNetCtrl->calculatingDivIdx);
	UINT32 totalLinkVol = 0;
	UINT32 mapIdx = 0;
	UINT16 x;
	UINT16 y;

	do {
		// パーセル枚数チェック
		if (aMapRead->mapVol != divArea->pclVol) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
			result = e_SC_RESULT_BADPARAM;
			break;
		}

		// パーセルテーブル初期化データ格納
		for (y = 0; y < divArea->pclRect.ySize; y++) {
			for (x = 0; x < divArea->pclRect.xSize; x++) {
				// ネットワークデータ有無確認
				if (NULL == aMapRead->mapList[mapIdx].road) {
					RP_Memset0(&aNetCtrl->parcelInfo[mapIdx], sizeof(SCRP_PCLINFO));
					mapIdx++;
					continue;
				}

				SCRP_AREAPCLSTATE* pclState = aNetCtrl->levelTbl->pclState + (divArea->pclIdx + mapIdx);
				SCRP_PCLINFO* pclInfo = aNetCtrl->parcelInfo + mapIdx;
				MAL_HDL binRoad = aMapRead->mapList[mapIdx].road;
				UINT32 connectIdVol = 0;

				// データ設定
				pclInfo->index = mapIdx;
				pclInfo->parcelId = SC_MESH_SftParcelId(divArea->pclRect.parcelId, x, y);
				pclInfo->linkIdx = totalLinkVol;
				pclInfo->areaJoin = pclState->join_f;
				pclInfo->areaSplit = pclState->split_f;
				pclInfo->mapNetworkBin = binRoad;
				pclInfo->flag = 0;

				// リンク拡張
				if (ALL_F32 == SC_MA_D_NWBIN_GET_NWLINKEX_OFS(binRoad) ) {
					pclInfo->mapNetworkLinkExBin = NULL;
				} else {
					pclInfo->mapNetworkLinkExBin = SC_MA_A_NWBIN_GET_NWLINKEX(binRoad);
				}
				// 接続（ALLFはデータ無し）
				if (ALL_F32 != SC_MA_D_NWBIN_GET_NWCNCT_OFS(binRoad) ) {
					connectIdVol = SC_MA_D_NWBIN_GET_CNCT_RECORDVOL(binRoad);
					pclInfo->mapNetworkCnctBin = SC_MA_A_NWBIN_GET_NWRCD_CNCT(binRoad);
				} else {
					connectIdVol = 0;
					pclInfo->mapNetworkCnctBin = NULL;
				}
				// リンク（ALLFはデータ無し）
				if (ALL_F32 != SC_MA_D_NWBIN_GET_NWLINK_OFS(binRoad) ) {
					pclInfo->linkIdVol = SC_MA_D_NWBIN_GET_LINK_RECORDVOL(binRoad);
					pclInfo->mapNetworkLinkBin = SC_MA_A_NWBIN_GET_NWRCD_LINK(binRoad);
				} else {
					pclInfo->linkIdVol = 0;
					pclInfo->mapNetworkLinkBin = NULL;
				}
				pclInfo->linkVol = pclInfo->linkIdVol + connectIdVol;

				// update
				totalLinkVol += pclInfo->linkVol;
				mapIdx++;
			}
		}

		// over flow
		if (SCRP_LINKINFO_MAX < totalLinkVol) {
			result = e_SC_RESULT_FAIL;
			SC_LOG_ErrorPrint(SC_TAG_RC, "link size over flow. total=%d "HERE, totalLinkVol);
			break;
		}
	} while (0);

	// 結果格納
	*aTotalLinkVol = totalLinkVol;

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief リンク情報初期情報設定
 * @param ネットワーク管理
 */
static E_SC_RESULT makeLinkInfo(SCRP_NETCONTROLER *aNetCtrl, SCRP_PCLRECT* aLowRect, E_RP_RTCALCSTEP aStep) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;

	do {
		if (NULL == aNetCtrl) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
			result = e_SC_RESULT_BADPARAM;
			break;
		}
		result = makeLinkInfoForId(aNetCtrl, aLowRect, aStep);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "makeLinkInfoForId error. [0x%08x] "HERE, result);
			break;
		}
		result = makeLinkInfoForCnct(aNetCtrl);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "makeLinkInfoForCnct error. [0x%08x] "HERE, result);
			break;
		}
		result = setLinkCnctIndex(aNetCtrl);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "updateLinkCnctIndex error. [0x%08x] "HERE, result);
			break;
		}
#if _ROUTECALC_QUICK_SAMPLE
		result = trimP2PLink(aNetCtrl);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "updateLinkCnctIndex error. [0x%08x] "HERE, result);
			break;
		}
#endif /* _ROUTECALC_QUICK_SAMPLE */
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief リンクテーブル構築：通常リンク
 * @param ネットワーク管理
 * @memo 通常リンクの構築のみを行う。接続リンクはmakeLinkInfoForCnct()にて行うこと。
 */
static E_SC_RESULT makeLinkInfoForId(SCRP_NETCONTROLER *aNetCtrl, SCRP_PCLRECT* aLowRect, E_RP_RTCALCSTEP aStep) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aNetCtrl) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	UINT32 i, e, or;

	for (i = 0; i < aNetCtrl->parcelInfoVol; i++) {
		SCRP_PCLINFO *pclInfo = aNetCtrl->parcelInfo + i;
		SCRP_LINKINFO *linkTab = aNetCtrl->linkTable + pclInfo->linkIdx;
		T_MapNWLink* pLink = (T_MapNWLink*) pclInfo->mapNetworkLinkBin;

		// Link順逆初期情報格納
		for (e = 0; e < pclInfo->linkIdVol; e++) {
			UINT16 sameLinkIdx[2] = { 0, 0 };
			UINT32 nextLinkId[2] = { 0, 0 };

			// dataIndex
			linkTab->detaIndex = e;

			// 接続Index（1始まりなので-1）
			sameLinkIdx[SCRP_LINKODR] = pLink->sameStIdx - 1;
			sameLinkIdx[SCRP_LINKRVS] = pLink->sameEdIdx - 1;

			// 接続先始終端情報追加
			nextLinkId[SCRP_LINKODR] = pLink->sameStId;
			nextLinkId[SCRP_LINKRVS] = pLink->sameEdId;

			for (or = 0; or < 2; or++) {
				SCRP_NETDATA* linkNet = linkTab->linkNet + or;
				linkNet->heap = SCRP_HEAP_UV;
				linkNet->costSum = ALL_F32;
				linkNet->inLinkHist = ALL_F32;
				linkNet->flag = 0;
				linkNet->flag |= (RCND_LINKOR & or);
				// 探索対象フラグ
				if (!judgeCalcLink(&pLink->linkBaseInfo)) {
					linkNet->flag |= RCND_EXCLUDELINK;
				}
				/* 上位,下位接続フラグ設定 */
				if (pclInfo->mapNetworkLinkExBin && SC_MA_D_NWRCD_LINK_GET_LINKEXOFSFLG(pLink)) {
					MAL_HDL ex = SC_MA_A_NWRCD_LINKEX_GET_RECORD(pclInfo->mapNetworkLinkExBin, SC_MA_D_NWRCD_LINK_GET_EXOFS(pLink));

					// 上位接続情報フラグ確認
					if (e_RP_STEP_LV1O == aStep) {
						if (SC_MA_D_NWRCD_EXLINK_GET_FLG_UPLV(ex)) {
							// 順方向の場合 始点側存在レベルで判定
							if (SCRP_LINKODR == or && 0x0000000F != SC_MA_D_NWRCD_EXLINK_GET_UPLVINFO_STLV(ex)) {
								linkNet->flag |= RCND_UPLEVEL;
							}
							// 逆方向の場合 終点側存在レベルで判定
							if (SCRP_LINKRVS == or && 0x0000000F != SC_MA_D_NWRCD_EXLINK_GET_UPLVINFO_EDLV(ex)) {
								linkNet->flag |= RCND_UPLEVEL;
							}
						}
					}

					// 下位接続情報フラグ確認 TODO 何かいい方法…
					if (e_RP_STEP_LV2TOP == aStep) {
						if (RCPI_GET_DOWNCNCT(pclInfo->flag) && SC_MA_D_NWRCD_EXLINK_GET_FLG_DOWNLV(ex)) {
							// 下位エリア包含判定
							if (isDownConnectLink(pclInfo->parcelId, ex, or, aLowRect)) {
								linkNet->flag |= RCND_DOWNLEVEL;
							}
						}
					}
				}

				// 接続先情報
				UINT32 nwLinkIdx = 0;
				if (SC_MA_NWID_IS_PNT_TYPE_CNCT(nextLinkId[or])) {
					nwLinkIdx = pclInfo->linkIdx + pclInfo->linkIdVol + sameLinkIdx[or];
				} else {
					nwLinkIdx = pclInfo->linkIdx + sameLinkIdx[or];
				}
				linkNet->nextLink = RCID_MASK_PCLIDX(i);
				linkNet->nextLink |= RCID_MASK_ORIDX(RP_GET_CNCTSIDE(nextLinkId[or]));
				linkNet->nextLink |= RCID_MASK_LINKIDX(nwLinkIdx);
			}
			// next
			linkTab++;
			pLink++;
		}
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief リンクテーブル構築：接続リンク
 * @param ネットワーク管理
 * @memo 接続リンクの構築のみ行う。パーセル外接続インデックスはupdateLinkCnctIndex()にて行うこと。
 */
static E_SC_RESULT makeLinkInfoForCnct(SCRP_NETCONTROLER *aNetCtrl) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aNetCtrl) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	UINT32 i, e, or;
	UINT8 cntWay[] = { 0, 0x20, 0x40, 0x80, 0x08, 0x0, 0x10, 0x01, 0x02, 0x04 };

	for (i = 0; i < aNetCtrl->parcelInfoVol; i++) {
		SCRP_PCLINFO *pclInfo = aNetCtrl->parcelInfo + i;
		SCRP_LINKINFO *linkTab = aNetCtrl->linkTable + (pclInfo->linkIdx + pclInfo->linkIdVol);
		T_MapNWConnection* pCnct = (T_MapNWConnection*) pclInfo->mapNetworkCnctBin;

		for (e = 0; e < (pclInfo->linkVol - pclInfo->linkIdVol); e++) {
			UINT16 sameLinkIdx[2] = { 0, 0 };
			UINT32 nextLinkId[2] = { 0, 0 };

			// dataIndex
			linkTab->detaIndex = pclInfo->linkIdVol + e;

			// 接続Index（1始まりなので-1）
			sameLinkIdx[SCRP_LINKODR] = pCnct->sameStIdx - 1;
			sameLinkIdx[SCRP_LINKRVS] = pCnct->sameEdIdx - 1;

			// 接続先始終端情報追加
			nextLinkId[SCRP_LINKODR] = pCnct->sameStId;
			nextLinkId[SCRP_LINKRVS] = pCnct->sameEdId;

			// パーセル外接続の場合は後続処理で格納し直し
			for (or = 0; or < 2; or++) {
				SCRP_NETDATA* linkNet = linkTab->linkNet + or;
				linkNet->costSum = ALL_F32;
				linkNet->heap = SCRP_HEAP_UV;
				linkNet->inLinkHist = ALL_F32;
				linkNet->flag = 0;
				linkNet->flag |= (RCND_LINKOR & or);
				linkNet->flag |= RCND_CONNECTLINK;
				// 自パーセルへの接続ではない場合接続フラグを判定する
				if (SC_MA_NWID_IS_PNT_TYPE_CNCT(nextLinkId[or])) {
					if (pclInfo->areaJoin & cntWay[((nextLinkId[or] >> 27) & 0xFF)]) {
						linkNet->flag |= RCND_CANDJOIN;
					}
					if (pclInfo->areaSplit & cntWay[((nextLinkId[or] >> 27) & 0xFF)]) {
						linkNet->flag |= RCND_CANDSPLIT;
					}
				}

				// 接続先リンクインデックス
				UINT32 nwLinkIdx = 0;
				if (SC_MA_NWID_IS_PNT_TYPE_CNCT(nextLinkId[or])) {
					nwLinkIdx = pclInfo->linkIdx + pclInfo->linkIdVol + sameLinkIdx[or];
				} else {
					nwLinkIdx = pclInfo->linkIdx + sameLinkIdx[or];
				}
				linkNet->nextLink = RCID_MASK_PCLIDX(i);
				linkNet->nextLink |= RCID_MASK_ORIDX(RP_GET_CNCTSIDE(nextLinkId[or]));
				linkNet->nextLink |= RCID_MASK_LINKIDX(nwLinkIdx);
			}
			// next
			linkTab++;
			pCnct++;
		}
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 接続リンクインデックス更新
 * @param ネットワーク管理
 */
static E_SC_RESULT setLinkCnctIndex(SCRP_NETCONTROLER *aNetCtrl) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aNetCtrl) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	UINT32 i, e;
	SCRP_PCLINFO *pclInfo = &aNetCtrl->parcelInfo[0];

	for (i = 0; i < aNetCtrl->parcelInfoVol; i++, pclInfo++) {
		SCRP_LINKINFO *linkTab = aNetCtrl->linkTable + (pclInfo->linkIdx + pclInfo->linkIdVol);

		// 接続情報から開始
		for (e = 0; e < (pclInfo->linkVol - pclInfo->linkIdVol); e++, linkTab++) {

			SCRP_PCLINFO* nextPclInfo = NULL;
			MAL_HDL pLink = SC_MA_A_NWRCD_CNCT_GET_RECORD(pclInfo->mapNetworkCnctBin, e);
			UINT32 nextLinkId = SC_MA_D_NWRCD_CNCT_GET_EDID(pLink);
			UINT32 nextPclId;

			// 接続方向からパーセルID取得
			nextPclId = getNextParcelID(pclInfo->parcelId, SC_MA_D_NWID_GET_SUB_CNCTDIR(nextLinkId));
			if (ALL_F32 == nextPclId) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "SC_MESH_GetNextParcelID error. "HERE);
				result = e_SC_RESULT_FAIL;
				break;
			}
			// パーセルIDからテーブル検索（未発見時はエリア外殻）
			nextPclInfo = searchParcelInfo(aNetCtrl, nextPclId);
			if (NULL == nextPclInfo) {
				linkTab->linkNet[SCRP_LINKRVS].nextLink = ALL_F32;
				linkTab->linkNet[SCRP_LINKRVS].flag |= RCND_AREAENDLINK;
#if 0
				// TODO 要検討 最外殻リンクフラグ継承
				SCRP_LINKINFO* inLink = RCNET_GET_NEXTLINKINFO(aNetCtrl, &linkTab->linkNet[SCRP_LINKODR]);
				if (SCRP_LINKODR == RCNET_GET_NEXTORIDX(&linkTab->linkNet[SCRP_LINKODR])) {
					inLink->linkNet[SCRP_LINKRVS].flag |= RCND_AREAENDLINK;
				} else {
					inLink->linkNet[SCRP_LINKODR].flag |= RCND_AREAENDLINK;
				}
#endif
				continue;
			}

			// リンクIDバイナリ検索
			UINT16 nextIdx = SC_MA_BinSearchNwRecord(nextPclInfo->mapNetworkBin, nextLinkId, SC_MA_BINSRC_TYPE_CNCT);
			if (ALL_F16 == nextIdx) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "RC_NextConnectIdBinSearch error. pcl=0x%08x link=0x%08x "HERE, nextPclId, nextLinkId);
				result = e_SC_RESULT_FAIL;
				break;
			}

			// リンクインデックスを生成
			UINT32 linkIdx = nextPclInfo->linkIdx + nextPclInfo->linkIdVol + (nextIdx - 1);
			linkTab->linkNet[SCRP_LINKRVS].nextLink = RCID_MASK_LINKIDX(linkIdx);
			linkTab->linkNet[SCRP_LINKRVS].nextLink |= RCID_MASK_PCLIDX(nextPclInfo->index);
			linkTab->linkNet[SCRP_LINKRVS].nextLink |= RCID_MASK_ORIDX(SCRP_LINKRVS);
		}
		if (e_SC_RESULT_SUCCESS != result) {
			break;
		}
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (result);
}

/**
 * @brief 外殻接続リンクフラグ設定処理
 * @param ネットワーク管理
 */
static E_SC_RESULT setAreaCandLinkFlag(SCRP_NETCONTROLER *aNetCtrl, E_RP_RTCALCSTEP aStep) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aNetCtrl) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	SCRP_NETDATA* preLinkNet = NULL;
	SCRP_NETDATA* linkNet = NULL;
	SCRP_NETDATA* linkNetRev = NULL;
	SCRP_LINKINFO* cnctTab = NULL;
	SCRP_LINKINFO* linkTab = NULL;
	SCRP_PCLINFO* pclInfo = aNetCtrl->parcelInfo;
	UINT32 i, e;

	for (i = 0; i < aNetCtrl->parcelInfoVol; i++, pclInfo++) {
		switch (aStep) {
		case e_RP_STEP_LV1TOP:
		case e_RP_STEP_LV2TOP:
			// トップは接続と断裂フラグ設置
			if (0 == pclInfo->areaJoin && 0 == pclInfo->areaSplit) {
				continue;
			}
			break;
		case e_RP_STEP_LV1D:
			// Lv1Dは断裂フラグ設置
			if (0 == pclInfo->areaSplit) {
				continue;
			}
			break;
		case e_RP_STEP_LV1O:
			// Lv1Oは外殻と断裂フラグ設置
			break;
		default:
			continue;
		}
		// 接続リンク先頭
		cnctTab = &aNetCtrl->linkTable[pclInfo->linkIdx + pclInfo->linkIdVol];

		// 接続情報からリンク情報を追う
		for (e = 0; e < (pclInfo->linkVol - pclInfo->linkIdVol); e++, cnctTab++) {

			// フラグは終点
			preLinkNet = cnctTab->linkNet + SCRP_LINKRVS;
			// 候補確認（断裂or分割）
			if (!RCND_GET_CANDJOINFLG(preLinkNet->flag)
					&& (!RCND_GET_CANDSPLITFLG(preLinkNet->flag)) && (!RCND_GET_AREAENDFLG(preLinkNet->flag))){
					continue;
				}
			// 同一接続は始点
			preLinkNet = &cnctTab->linkNet[SCRP_LINKODR];

			// 同一リンク検索
			while (1) {
				// フラグ確認逆転用
				UINT16 revs[] = { SCRP_LINKRVS, SCRP_LINKODR };
				UINT16 dir = RCNET_GET_NEXTORIDX(preLinkNet);
				linkTab = RCNET_GET_NEXTLINKINFO(aNetCtrl, preLinkNet);
				linkNet = linkTab->linkNet + dir;
				linkNetRev = linkTab->linkNet + revs[dir];

				/*
				 * 該当リンク発見
				 * 最外殻接続リンクとして候補開始リンクとして採用する
				 * TODO ここでフラグを立てるのは誤りである可能性？
				 *      分割探索orトップ探索orOD側探索で採用する候補開始リンクが異なる…
				 */
				if (!RCND_GET_CONNECTLINK(linkNetRev->flag)) {
					linkNetRev->flag |= RCND_GET_CANDFLGS(cnctTab->linkNet[SCRP_LINKRVS].flag);
					break;
				}
				// 1周した場合終了
				if (cnctTab == linkTab) {
					SC_LOG_WarnPrint(SC_TAG_RC, "can't find out side connect link. pcl=0x%08x linkIdx=%d", pclInfo->parcelId,
							linkTab->detaIndex);
					break;
				}
				preLinkNet = linkNet;
			}
		}
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 目的地パーセルフラグ設定
 * @param ネットワーク管理
 * @param 区間管理情報
 * @memo 目的地が存在する場合のみ処理すること（e_RP_STEP_LV1TOP || e_RP_STEP_LV1D）
 */
static E_SC_RESULT setDestParcelFlag(SCRP_NETCONTROLER *aNetCtrl, SCRP_SECTCONTROLER *aSectCtrl) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aSectCtrl || NULL == aNetCtrl) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	SCRP_NEIGHBORLINK* nbrLink = aSectCtrl->neighbor[SCRP_NBRDST].neighborLink;
	UINT32 i, e;

	for (i = 0; i < aNetCtrl->parcelInfoVol; i++) {
		// 近傍情報と合致するパーセルIDであればフラグを立てる
		for (e = 0; e < aSectCtrl->neighbor[SCRP_NBRDST].nbrLinkVol; e++) {
			if (aNetCtrl->parcelInfo[i].parcelId == nbrLink[e].parcelId) {
				aNetCtrl->parcelInfo[i].flag |= RCPI_DEST;
				break;
			}
		}
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 最外殻パーセルフラグ設定
 * @param ネットワーク管理
 * @param 区間管理情報
 */
static E_SC_RESULT setOuterMostParcelFlag(SCRP_SECTCONTROLER *aSectCtrl, SCRP_NETCONTROLER *aNetCtrl) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aSectCtrl || NULL == aNetCtrl) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	SCRP_DIVAREA *divArea = aNetCtrl->levelTbl->divInfo + (aNetCtrl->levelArea->divIdx + aNetCtrl->calculatingDivIdx);
	UINT32 mapIdx = 0;
	UINT32 x, y;

	// パーセルテーブル初期化データ格納
	for (y = 0; y < divArea->pclRect.ySize; y++) {
		for (x = 0; x < divArea->pclRect.xSize; x++) {
			SCRP_PCLINFO* pclInfo = aNetCtrl->parcelInfo + mapIdx++;
			if (0 == y || y == divArea->pclRect.ySize - 1) {
				pclInfo->flag |= RCPI_OUTERMOST;
				continue;
			}
			if (0 == x || x == divArea->pclRect.xSize - 1) {
				pclInfo->flag |= RCPI_OUTERMOST;
				continue;
			}
		}
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 下位接続パーセルフラグ設定
 * @param 区間管理情報
 * @param ネットワーク管理
 * @memo 基準となるパーセルからの各左下・右上シフト量から一部でも被りがあれば、下位接続フラグを立てる。
 *       下位が存在する場合のみ処理すること（e_RP_STEP_LV2TOP）
 *       TODO レベル２以外は未対応
 */
static E_SC_RESULT setDownConnectParcelFlag(SCRP_SECTCONTROLER *aSectCtrl, SCRP_NETCONTROLER *aNetCtrl) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aSectCtrl || NULL == aNetCtrl) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	UINT32 i;
	//UINT32 e;
	SCRP_PCLINFO* pclInfo = NULL;
	SCRP_PCLRECT *rect = &aSectCtrl->levelTable.areaTable[2].pclRect;
	UINT32 currentPcl = 0;
	UINT32 baseLb = rect->parcelId;
	INT32 a_RtX = rect->xSize;
	INT32 a_RtY = rect->ySize;
	INT32 b_LbX = 0, b_LbY = 0;
	INT32 b_RtX = 0, b_RtY = 0;
	Bool xCover = false;
	Bool yCover = false;

	pclInfo = aNetCtrl->parcelInfo;
	for (i = 0; i < aNetCtrl->parcelInfoVol; i++, pclInfo++) {
		// 地図データなしの場合無視
		if (0 == pclInfo->parcelId) {
			continue;
		}
		// 下位左下パーセル
		currentPcl = SC_MESH_GetUnderLevelParcelID(pclInfo->parcelId, RP_LEVEL1);
		// 各シフト量算出
		if (-1 == SC_MESH_GetAlterPos(baseLb, currentPcl, RP_LEVEL1, &b_LbX, &b_LbY)) {
			continue;
		}
		// LV2のみ想定している為固定4
		b_RtX = b_LbX + 4;
		b_RtY = b_LbY + 4;
		// パーセルエリアAにパーセルエリアBがかぶっているかをチェック
		if (0 <= b_LbX) {
			if (a_RtX <= b_LbX) {
				// X軸被り無し→可能性なし
				xCover = false;
			} else {
				// X軸に関しては被り
				xCover = true;
			}
		} else {
			if (0 <= b_RtX) {
				// X軸に関しては被り
				xCover = true;
			} else {
				// X軸被り無し→可能性なし
				xCover = false;
			}
		}
		if (0 <= b_LbY) {
			if (a_RtY <= b_LbY) {
				// Y軸被り無し→可能性なし
				yCover = false;
			} else {
				// Y軸に関しては被り
				yCover = true;
			}
		} else {
			if (0 <= b_RtY) {
				// Y軸に関しては被り
				yCover = true;
			} else {
				// Y軸被り無し→可能性なし
				yCover = false;
			}
		}
		if (xCover && yCover) {
			// 一部被りあり
			pclInfo->flag |= RCPI_DOWNCNCT;
		}
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 下位接続リンクのパーセルが下位探索エリアに含まれるかの判定
 * @param 下位接続リンク情報アドレス
 * @param 下位パーセルエリア
 * @memo レベル２→レベル１専用
 *       TODO レベル２以外は未対応
 */
static Bool isDownConnectLink(UINT32 aLv2PclId, MAL_HDL aLinkEx, UINT8 aOr, SCRP_PCLRECT* aLowRect) {

	if (NULL == aLinkEx || NULL == aLowRect) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "bad param. "HERE);
		return (false);
	}
	MAL_HDL downLink = SC_MA_A_NWRCD_EXLINK_GET_DOWNLVLINK(aLinkEx);
	UINT32 currentPcl = aLowRect->parcelId;
	INT32 aSftX = aLowRect->xSize;
	INT32 aSftY = aLowRect->ySize;
	INT32 bSftX = 0, bSftY = 0;
	INT16 sftX = 0;
	INT16 sftY = 0;
	UINT32 lv1ParcelId = SC_MESH_GetUnderLevelParcelID(aLv2PclId, RP_LEVEL1);

	// シフト量取得
	if (SCRP_LINKODR == aOr) {
		sftX = SC_MA_A_NWRCD_EXLINK_GET_DOWNLVSTLINK_SFTX(downLink);
		sftY = SC_MA_A_NWRCD_EXLINK_GET_DOWNLVSTLINK_SFTY(downLink);
	} else {
		sftX = SC_MA_A_NWRCD_EXLINK_GET_DOWNLVEDLINK_SFTX(downLink);
		sftY = SC_MA_A_NWRCD_EXLINK_GET_DOWNLVEDLINK_SFTY(downLink);
	}

	// 対象リンクのパーセルID
	lv1ParcelId = SC_MESH_SftParcelId(lv1ParcelId, sftX, sftY);

	if (-1 == SC_MESH_GetAlterPos(currentPcl, lv1ParcelId, RP_LEVEL1, &bSftX, &bSftY)) {
		return (false);
	}
	if (0 <= bSftX && 0 <= bSftY) {
		if (bSftX < aSftX && bSftY < aSftY) {
#if 0 // 下位変換ようにとりあえず消さない
			UINT32 linkid = 0;
			if (aOr) {
				linkid = SC_MA_A_NWRCD_EXLINK_GET_DOWNLVSTLINK(downLink);
			} else {
				linkid = SC_MA_A_NWRCD_EXLINK_GET_DOWNLVEDLINK(downLink);
			}
			SC_LOG_InfoPrint(SC_TAG_RC, "  lv2=0x%08x lv1=0x%08x sft=%2d,%2d bSft=%2d,%2d lv1Link=0x%08x (low=0x%08x(%d,%d)"
					, aLv2PclId
					, lv1ParcelId
					, sftX
					, sftY
					, bSftX
					, bSftY
					, linkid
					, aLowRect->parcelId
					, aLowRect->xSize
					, aLowRect->ySize
					);
#endif
			return (true);
		}
	}

	return (false);
}

/**
 * @brief パーセル情報テーブル内検索
 * @param ネットワーク管理
 * @param 検索パーセルID
 */
static SCRP_PCLINFO* searchParcelInfo(SCRP_NETCONTROLER *aNetCtrl, UINT32 aParcelId) {

	UINT16 lp = 0;

	if (NULL == aNetCtrl) {
		return (NULL );
	}

	for (lp = 0; lp < aNetCtrl->parcelInfoVol; lp++) {
		if (aParcelId == (aNetCtrl->parcelInfo + lp)->parcelId) {
			return (aNetCtrl->parcelInfo + lp);
		}
	}
	return (NULL );
}

/**
 * @brief 計算対象リンクか否かを判定する
 * @param リンク基本情報
 * @memo TODO 海外は別途定数必要
 */
static Bool judgeCalcLink(T_MapBaseLinkInfo *aBaseLinkInfo) {

	Bool roadJudge[16] = { true, true, true, false, true, true, true, true, true, true, true, true, false, false, false, false };

	if (NULL == aBaseLinkInfo) {
		return (false);
	}
	// 計画道路
	if (0 != aBaseLinkInfo->b_code.plan) {
		return (false);
	}
	// 階段リンク
	if (0 != aBaseLinkInfo->b_code.stairs) {
		return (false);
	}
	return (roadJudge[aBaseLinkInfo->b_code.roadKind]);
}

/**
 * @brief 移動先パーセル取得IFラッパー
 * @param 基準パーセルID
 * @param 移動先
 * @memo 移動先方向 [1][2][3]    [7][0][1]
 *       5中央      [4][5][6] -> [6]   [2]
 *                  [7][8][9]    [5][4][3]
 */
static UINT32 getNextParcelID(UINT32 aParcelId, UINT8 aDir) {
	UINT8 dirCnv[10] = { ALL_F8, DIR_L_TOP, DIR_TOP, DIR_R_TOP, DIR_L, ALL_F8, DIR_R, DIR_L_DOWN, DIR_DOWN, DIR_R_DOWN };

	if (ALL_F8 == dirCnv[aDir]) {
		return (aParcelId);
	} else {
		return (SC_MESH_GetNextParcelID(aParcelId, dirCnv[aDir]));
	}
}

#if _ROUTECALC_QUICK_SAMPLE	// ※高速化? 接続情報を省く→
/**
 * @brief 接続リンクインデックスをトリムする
 * @param ネットワーク管理
 */
static E_SC_RESULT trimP2PLink(SCRP_NETCONTROLER *aNetCtrl) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	UINT32 i, u;
	for (i = 0; i < aNetCtrl->parcelInfoVol; i++) {
		SCRP_PCLINFO *pclInfo = &aNetCtrl->parcelInfo[i];
		SCRP_LINKINFO *linkTable = &aNetCtrl->linkTable[pclInfo->linkIdx];

		for (u = 0; u < pclInfo->linkIdVol; u++, linkTable++) {
			T_MapNWLink* link = (T_MapNWLink*) SC_MA_A_NWRCD_LINK_GET_RECORD(pclInfo->mapNetworkLinkBin, linkTable->detaIndex);

			// 接続(終端)
			if (SC_MA_NWID_IS_PNT_TYPE_CNCT(link->sameStId)) {
				trimNextConnectLink(aNetCtrl, &linkTable->linkNet[SCRP_LINKODR]);
			}
			if (SC_MA_NWID_IS_PNT_TYPE_CNCT(link->sameEdId)) {
				trimNextConnectLink(aNetCtrl, &linkTable->linkNet[SCRP_LINKRVS]);
			}
		}
	}
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief	図郭リンク専用接続先リンク情報取得
 * @param	netTable
 */
static E_SC_RESULT trimNextConnectLink(SCRP_NETCONTROLER* aNetTable, SCRP_NETDATA *aNetLink) {

	SCRP_NETDATA* aResN[16] = {};
	UINT32 aResV[16] = {};
	UINT8 vSet = 0;
	UINT8 nSet = 0;
	UINT16 cnt;

	SCRP_NETDATA* o_CNPre;
	SCRP_LINKINFO* o_CLPre;
	SCRP_PCLINFO* o_PPre;

	SCRP_NETDATA* o_CNBk;
	SCRP_NETDATA* o_CN;
	SCRP_LINKINFO* o_CL;

	SCRP_NETDATA* n_CNPre;

	SCRP_NETDATA* n_CNBk;
	SCRP_NETDATA* n_CN;
	SCRP_LINKINFO* n_CL;

	// 前回情報
	//o_CNBk = aNetLink;
	o_CNPre = aNetLink;
	aResN[nSet++] = o_CNPre;
	// 外出れる？
	if (ALL_F32 == aNetLink->nextLink) {
		return (e_SC_RESULT_BADPARAM);
	}

	while (1) {

		// ①
		o_CL = RCNET_GET_NEXTLINKINFO(aNetTable, o_CNPre);
		o_CN = o_CL->linkNet + RCNET_GET_NEXTORIDX(o_CNPre);

		// 接続以外＝一周
		if (!RCND_GET_CONNECTLINK(o_CN->flag)) {
			aResV[vSet++] = o_CNPre->nextLink;
			break;
		}
		// 外出れる？
		if (ALL_F32 == o_CL->linkNet[SCRP_LINKRVS].nextLink) {
			o_CNPre = o_CN;
			continue;
		}

		// ②
		n_CL = RCNET_GET_NEXTLINKINFO(aNetTable, &o_CL->linkNet[SCRP_LINKRVS]);
		n_CN = &n_CL->linkNet[OWN_SIDE];
		n_CNBk = n_CNPre = n_CN;

		//cnt = 0;
		// ③
		while (1) {
			n_CL = RCNET_GET_NEXTLINKINFO(aNetTable, n_CNPre);
			n_CN = n_CL->linkNet + RCNET_GET_NEXTORIDX(n_CNPre);

			if (!RCND_GET_CONNECTLINK(n_CN->flag)) {
				// HIT
				aResN[nSet++] = n_CN;
				aResV[vSet++] = n_CNPre->nextLink;
				break;
			}
			// 接続ID
			n_CNPre = n_CN;

			// 一周
			if (n_CNBk == n_CNPre) {
				//error
				SC_LOG_ErrorPrint(SC_TAG_RC, "[NMake] Bound closs count max."HERE);
				SC_LOG_DebugPrint(SC_TAG_RC, "[NMake] Bound closs count max. baseIdx[%d]", n_CL->detaIndex);
				break;
			}
#if 0
			// デバッグ用 接続関係が破綻している場合無限ループになるのを防ぐ
			cnt++;
			// 多い
			if (RP_CLOSS_MAX <= cnt) {
				//error
				SC_LOG_ErrorPrint(SC_TAG_RC, "[NMake] Bound closs count max."HERE);
				SC_LOG_DebugPrint(SC_TAG_RC, "[NMake] Bound closs count max. baseIdx[%d]", n_CL->detaIndex);
				break;
			}
#endif
		}
		// 次
		o_CNPre = o_CN;
	}
	for (cnt = 0; cnt < nSet; cnt++) {
		aResN[cnt]->nextLink = aResV[cnt];
		//SC_LOG_DebugPrint(SC_TAG_RC, "[NMake] Bound closs. next[0x%08x] old[0x%08x]", aResV[cnt], aResN[cnt]->nextLink);
	}
	return (e_SC_RESULT_SUCCESS);
}
#endif
