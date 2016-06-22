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
 * RP_Debug.c
 *
 *  Created on: 2014/01/25
 *      Author: masutani
 */

#include "SMCoreRPInternal.h"

/**
 * @brief 探索情報テーブル：推奨経路ダンプ
 * @param [I]推奨経路テーブル
 */
void RPDBG_DumpRoute(SC_RP_RouteMng* aRouteMng, Bool aLinkDmp, Bool aFormDmp) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	SC_RP_ParcelInfo* pclInfo = NULL;
	SC_RP_LinkInfo* linkInfo = NULL;
	SC_RP_SectInfo* sectInfo = NULL;
	SC_RP_FormInfo* formInfo = NULL;
	UINT32 i, e, u;

	if (NULL == aRouteMng) {
		return;
	}

	SC_LOG_DebugPrint(SC_TAG_RC, "++ Route ++");
	SC_LOG_DebugPrint(SC_TAG_RC, "| mapVersion=%s routeVersion=%s", aRouteMng->mapVer, aRouteMng->routeVer);

	// 区間情報ダンプ
	SC_LOG_DebugPrint(SC_TAG_RC, "+-----------------------------------------------------------");
	for (i = 0; i < aRouteMng->sectVol; i++) {
		char buf[256] = {};
		sectInfo = aRouteMng->sectInfo + i;
		sprintf(buf, "sectNo.%2d priority=%2d dist=%5d hwDist=%5d time=%6d pcl=%3d-%3d link=%4d-%4d form=%4d-%4d" //
				, sectInfo->sectNumber //
				, sectInfo->priority //
				, sectInfo->sectDist //
				, sectInfo->sectHWDist //
				, sectInfo->sectTime //
				, sectInfo->parcelIdx //
				, sectInfo->parcelVol //
				, sectInfo->linkIdx //
				, sectInfo->linkVol //
				, sectInfo->formIdx //
				, sectInfo->formVol //
				);
		SC_LOG_DebugPrint(SC_TAG_RC, "| %s", buf);
	}

	// パーセル情報ダンプ
	SC_LOG_DebugPrint(SC_TAG_RC, "+-----------------------------------------------------------");
	for (i = 0; i < aRouteMng->parcelVol; i++) {
		char buf[256] = {};
		pclInfo = aRouteMng->parcelInfo + i;
		sprintf(buf, "pcl=0x%08x preDist=%5d level=%d LinkOfs/Vol[%5d/%5d] RegOfs/Vol[%5d/%5d] SubOfs/Vol[%5d/%5d]" //
				, pclInfo->parcelId //
				, pclInfo->preDist //
				, pclInfo->level //
				, pclInfo->linkIdx //
				, pclInfo->linkVol //
				, pclInfo->regIdx //
				, pclInfo->regVol //
				, pclInfo->subLinkIdx //
				, pclInfo->subLinkVol //
				);
		SC_LOG_DebugPrint(SC_TAG_RC, "| %s", buf);
	}

	// リンク情報ダンプ
	if (aLinkDmp) {
		SC_LOG_DebugPrint(SC_TAG_RC, "+-----------------------------------------------------------");
		for (i = 0; i < aRouteMng->linkVol; i++) {
			char buf[256] = {};
			linkInfo = aRouteMng->linkInfo + i;
			sprintf(buf, "LinkId[0x%08x(%d)] dist(%4d) time(%5d) termflg(%d) level=%d FormOfs/Vol[%5d/%5d] RegOfs/Vol[%5d/%5d]" //
					, linkInfo->linkId //
					, linkInfo->orFlag //
					, linkInfo->dist //
					, linkInfo->travelTime //
					, linkInfo->termFlag //
					, linkInfo->level //
					, linkInfo->formIdx //
					, linkInfo->formVol //
					, linkInfo->regIdx //
					, linkInfo->regVol //
					);
			SC_LOG_DebugPrint(SC_TAG_RC, "| %s", buf);
		}
	}

	// 形状情報ダンプ
	if (aFormDmp) {
		SC_LOG_DebugPrint(SC_TAG_RC, "+-----------------------------------------------------------");
		for (i = 0; i < aRouteMng->sectVol; i++) {
			sectInfo = aRouteMng->sectInfo + i;
			for (e = 0; e < sectInfo->linkVol; e++) {
				linkInfo = aRouteMng->linkInfo + sectInfo->linkIdx + e;
				for (u = 0; u < linkInfo->formVol; u++) {
					formInfo = aRouteMng->formInfo + sectInfo->formIdx + linkInfo->formIdx + u;

					SC_LOG_DebugPrint(SC_TAG_RC, "| LinkId[0x%08x]  from[%4d][%4d]", linkInfo->linkId, formInfo->x, formInfo->y);
				}
			}
		}
	}
	SC_LOG_DebugPrint(SC_TAG_RC, "+-----------------------------------------------------------");

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
}

/**
 * ネットワークダンプ
 */
void RPDBG_ShowNetwork(SCRP_NETCONTROLER* aNetCtrl) {
	if (NULL == aNetCtrl) {
		return;
	}
	//const UINT16 cLowOfs[2] = { 8, 10 };
	// 上位接続
	const Bool up = false;
	// 下位接続
	const Bool down = false;
	const UINT32 downMax = 100;
	UINT32 downCount = 0;
	// 断裂リンク
	const Bool split = false;

	UINT32 i, e, or;
	for (i = 0; i < aNetCtrl->parcelInfoVol; i++) {
		SCRP_PCLINFO* pclInfo = aNetCtrl->parcelInfo + i;
		SC_LOG_InfoPrint(SC_TAG_RC, "[%3d] pcl=0x%08x flg=0x%04d join=0x%02x split=0x%02x linkVol=%6d linkIdx=%6d" //
				, i //
				, pclInfo->parcelId //パーセルID
				, pclInfo->flag // フラグ
				, pclInfo->areaJoin //接続
				, pclInfo->areaSplit //断裂
				, pclInfo->linkVol //リンク数
				, pclInfo->linkIdx //リンクインデックス
				);

		if (NULL == pclInfo->mapNetworkLinkBin || NULL == pclInfo->mapNetworkLinkExBin) {
			continue;
		}

		SCRP_LINKINFO* linkInfo = aNetCtrl->linkTable + pclInfo->linkIdx;
		for (e = 0; e < pclInfo->linkIdVol; e++, linkInfo++) {
			MAL_HDL pLink = SC_MA_A_NWRCD_LINK_GET_RECORD(pclInfo->mapNetworkLinkBin, linkInfo->detaIndex);
			MAL_HDL pExLink = SC_MA_A_NWRCD_LINKEX_GET_RECORD(pclInfo->mapNetworkLinkExBin, SC_MA_D_NWRCD_LINK_GET_EXOFS(pLink));
			SCRP_NETDATA* netData = linkInfo->linkNet;

			for (or = 0; or < 2; or++, netData++) {
				char buffer[512] = {};
				// 断裂情報ダンプ
				if (split) {
					if (RCND_GET_CANDSPLITFLG(netData->flag)) {
						sprintf(buffer, "%s isSplit(0x%04x)", buffer, netData->flag);
					}
				}
				// 上位接続情報ダンプ
				if (up) {
					if (RCND_GET_UPLEVELFLG(netData->flag)) {
						sprintf(buffer, "%s isUp(0x%04x)", buffer, netData->flag);
					}
				}
				// 下位接続情報ダンプ
				if (down) {
					if (downMax < downCount)
						continue;

					UINT32 lowPclId;
					MAL_HDL pLowLinkEx = SC_MA_A_NWRCD_EXLINK_GET_DOWNLVLINK(pExLink);
					if (RCND_GET_DOWNLEVELFLG(netData->flag)) {
						downCount++;
						lowPclId = SC_MESH_GetUnderLevelParcelID(pclInfo->parcelId, RP_LEVEL1);
						lowPclId = SC_MESH_SftParcelId(lowPclId, SC_MA_A_NWRCD_EXLINK_GET_DOWNLVSTLINK_SFTX(pLowLinkEx),
								SC_MA_A_NWRCD_EXLINK_GET_DOWNLVSTLINK_SFTY(pLowLinkEx));

						sprintf(buffer, "%s low=0x%08x 0x%08x 0x%08x (pcl=0x%08x)", buffer, //
								read4byte(pLowLinkEx), read4byte(pLowLinkEx + 4), read4byte(pLowLinkEx + 8), lowPclId);
					}
				}
				// 表示
				if (0 < strlen(buffer)) {
					SC_LOG_InfoPrint(SC_TAG_RC, " link=0x%08x(or=%d) %s", SC_MA_D_NWRCD_LINK_GET_ID(pLink), or, buffer);
				}
			}
		}
	}
}

void RPDBG_ShowStepLog(SCRP_SECTCONTROLER *aSectCtrl, E_RP_RTCALCSTEP aStep) {

	//SCRP_DIVAREA* divInfo = NULL;
	SCRP_LEVELAREA* lvArea = NULL;

	switch (aStep) {
	case e_RP_STEP_LV1TOP:
		lvArea = &aSectCtrl->levelTable.areaTable[0];
		SC_LOG_InfoPrint(SC_TAG_RC, "*---* e_RP_STEP_LV1TOP start. divVol=%2d", lvArea->divVol);
		break;
	case e_RP_STEP_LV1O:
		lvArea = &aSectCtrl->levelTable.areaTable[0];
		SC_LOG_InfoPrint(SC_TAG_RC, "*---* e_RP_STEP_LV1O   start. divVol=%2d", lvArea->divVol);
		break;
	case e_RP_STEP_LV1D:
		lvArea = &aSectCtrl->levelTable.areaTable[2];
		SC_LOG_InfoPrint(SC_TAG_RC, "*---* e_RP_STEP_LV1D   start. divVol=%2d", lvArea->divVol);
		break;
	case e_RP_STEP_LV2TOP:
		lvArea = &aSectCtrl->levelTable.areaTable[1];
		SC_LOG_InfoPrint(SC_TAG_RC, "*---* e_RP_STEP_LV2TOP start. divVol=%2d", lvArea->divVol);
		break;
	}
}

void RPDBG_ShowCandSizeInfo(SCRP_SECTCONTROLER *aSectCtrl, E_RP_RTCALCSTEP aStep, UINT16 split) {
	SCRP_PCLRECT* rect;

	switch (aStep) {
	case e_RP_STEP_LV1TOP:
		rect = &aSectCtrl->levelTable.divInfo[aSectCtrl->levelTable.areaTable[0].divIdx + split].pclRect;
		break;
	case e_RP_STEP_LV1O:
		rect = &aSectCtrl->levelTable.divInfo[aSectCtrl->levelTable.areaTable[0].divIdx + split].pclRect;
		break;
	case e_RP_STEP_LV1D:
		rect = &aSectCtrl->levelTable.divInfo[aSectCtrl->levelTable.areaTable[2].divIdx + split].pclRect;
		break;
	case e_RP_STEP_LV2TOP:
		rect = &aSectCtrl->levelTable.divInfo[aSectCtrl->levelTable.areaTable[1].divIdx + split].pclRect;
		break;
	}
	// 分割エリアダンプ
	SC_LOG_InfoPrint(SC_TAG_RC, "    * div calc split=[%2d] area=0x%08x(%4d,%4d)", split + 1 // ステップ
			, rect->parcelId // パーセル
			, rect->xSize // X
			, rect->ySize // Y
			);
	// 候補経路 サイズダンプ
	SC_LOG_InfoPrint(SC_TAG_RC, "    * cand size cand=%d(%d) stLink=%d(%d) split size cand=%d(%d) stLink=%d(%d)" //
			, aSectCtrl->candMng.candSize // 候補サイズ
			, aSectCtrl->candMng.candCurrent // 候補使用位置
			, aSectCtrl->candMng.stLinkSize // 候補開始サイズ
			, aSectCtrl->candMng.stLinkCurrent // 候補開始使用位置
			, aSectCtrl->candMng.splitCandSize // 分割候補サイズ
			, aSectCtrl->candMng.splitCandCurrent // 分割候補使用位置
			, aSectCtrl->candMng.splitStLinkSize // 分割候補開始サイズ
			, aSectCtrl->candMng.splitStLinkCurrent // 分割候補開始使用位置
			);
}

void RPDBG_ShowCandInfo(SCRP_SECTCONTROLER *aSectCtrl, E_RP_RTCALCSTEP aStep, UINT16 split) {

	SCRP_CANDSTARTLINK* stCand;
	SCRP_CANDTBLINFO* candInfo;
	switch (aStep) {
	case e_RP_STEP_LV1TOP:
		candInfo = &aSectCtrl->candMng.candTblInfo[RC_CAND_IDX_LV1TOP];
		break;
	case e_RP_STEP_LV1O:
		candInfo = &aSectCtrl->candMng.candTblInfo[RC_CAND_IDX_LV1O];
		break;
	case e_RP_STEP_LV1D:
		candInfo = &aSectCtrl->candMng.candTblInfo[RC_CAND_IDX_LV1D];
		break;
	case e_RP_STEP_LV2TOP:
		candInfo = &aSectCtrl->candMng.candTblInfo[RC_CAND_IDX_LV2TOP];
		break;
	}
	SC_LOG_InfoPrint(SC_TAG_RC, "    * cand info dmp st=%5d(size=%5d) cand=%5d(size=%5d)" //
			, candInfo->stLinkIdx // インデックス
			, candInfo->stLinkSize // サイズ
			, candInfo->candIdx // インデックス
			, candInfo->candSize // サイズ
			);
	stCand = aSectCtrl->candMng.stLink + candInfo->stLinkIdx;
	UINT32 i;
	for (i = 0; i < candInfo->stLinkSize; i++, stCand++) {
		SC_LOG_InfoPrint(SC_TAG_RC, "    * stCand pcl=0x%08x link=0x%08x flg=0x%04x level=%d Connect[pcl=0x%08x link-0x%08x] " //
				, stCand->parcelId // パーセル
				, stCand->linkId // リンク
				, stCand->flag // フラグ
				, stCand->connectLevel // フラグ
				, stCand->st.parcelId // パーセル
				, stCand->st.linkId // リンク
				);
	}
}

void RPDBG_ShowSectLvInfo(SCRP_SECTCONTROLER* sectCtrl) {
	// LevelInfoダンプ
	SC_LOG_InfoPrint(SC_TAG_RC, " top=%02d pcl=%4d div=%4d", sectCtrl->levelTable.topLevel, sectCtrl->levelTable.pclStateVol,
			sectCtrl->levelTable.divInfoVol);
	UINT32 i, e, u;
	for (i = 0; i < 3; i++) {
		SC_LOG_InfoPrint(SC_TAG_RC, " step=%1d 0x%08x(%2d,%2d) div=%3d(idx=%d)" //
				, i // ステップ
				, sectCtrl->levelTable.areaTable[i].pclRect.parcelId // パーセル
				, sectCtrl->levelTable.areaTable[i].pclRect.xSize // X
				, sectCtrl->levelTable.areaTable[i].pclRect.ySize // Y
				, sectCtrl->levelTable.areaTable[i].divVol // 分割数
				, sectCtrl->levelTable.areaTable[i].divIdx // 分割インデックス
				);
		SCRP_DIVAREA* divInfo = sectCtrl->levelTable.divInfo + sectCtrl->levelTable.areaTable[i].divIdx;
		for (e = 0; e < sectCtrl->levelTable.areaTable[i].divVol; e++) {
			SC_LOG_InfoPrint(SC_TAG_RC, "       div[%3d] pcl=0x%08x(%4d,%4d)" //
					, e // 分割インデックス
					, (divInfo + e)->pclRect.parcelId // パーセル
					, (divInfo + e)->pclRect.xSize // X
					, (divInfo + e)->pclRect.ySize // Y
					);
			SCRP_AREAPCLSTATE* pclState = sectCtrl->levelTable.pclState + (divInfo + e)->pclIdx;
			for (u = 0; u < (divInfo + e)->pclVol; u++) {
				SC_LOG_InfoPrint(SC_TAG_RC, "             pcl=0x%08x join=0x%02x split=0x%02x dens=%4d" //
						, u // パーセルインデックス
						, (pclState + u)->join_f // 接続フラグ
						, (pclState + u)->split_f // 断裂フラグ
						, (pclState + u)->linkDensity // 密度
						);
			}
		}
	}
}

void RPDBG_ShowNeigborLink(SCRP_SECTCONTROLER* aSectCtrl) {
	// 近傍ダンプ
	UINT32 i;
	SC_LOG_InfoPrint(SC_TAG_RC, " O側 pcl=0x%08x(%4d,%4d)" //
			, aSectCtrl->neighbor[0].point.parcelId // パーセルID
			, aSectCtrl->neighbor[0].point.x // X
			, aSectCtrl->neighbor[0].point.y // Y
			);
	for (i = 0; i < aSectCtrl->neighbor[0].nbrLinkVol; i++) {
		char buf[256] = {};
		sprintf(buf, "pcl=0x%08x link=0x%08x or=%d nouse=%d leave=%4f rem=%4f dist=%4d srem=%4f sdist=%4d cost=%5d" //
				, aSectCtrl->neighbor[0].neighborLink[i].parcelId //パーセルID
				, aSectCtrl->neighbor[0].neighborLink[i].linkId //リンクID
				, aSectCtrl->neighbor[0].neighborLink[i].orFlag //or
				, aSectCtrl->neighbor[0].neighborLink[i].noUse //use
				, aSectCtrl->neighbor[0].neighborLink[i].leavDist //距離
				, aSectCtrl->neighbor[0].neighborLink[i].remainDist //塗りつぶし距離
				, aSectCtrl->neighbor[0].neighborLink[i].linkDist //リンク長
				, aSectCtrl->neighbor[0].neighborLink[i].subRemainDist //塗りつぶし距離
				, aSectCtrl->neighbor[0].neighborLink[i].subDist //リンク長
				, aSectCtrl->neighbor[0].neighborLink[i].cost //初期コスト
				);
		SC_LOG_DebugPrint(SC_TAG_RC, " O側 %s", buf);
	}
	SC_LOG_InfoPrint(SC_TAG_RC, " D側 pcl=0x%08x(%4d,%4d)" //
			, aSectCtrl->neighbor[1].point.parcelId //パーセルID
			, aSectCtrl->neighbor[1].point.x // X
			, aSectCtrl->neighbor[1].point.y // Y
			);
	for (i = 0; i < aSectCtrl->neighbor[1].nbrLinkVol; i++) {
		char buf[256] = {};
		sprintf(buf, "pcl=0x%08x link=0x%08x or=%d nouse=%d leave=%4f rem=%4f dist=%4d srem=%4f sdist=%4d cost=%5d" //
				, aSectCtrl->neighbor[1].neighborLink[i].parcelId // パーセルID
				, aSectCtrl->neighbor[1].neighborLink[i].linkId // リンクID
				, aSectCtrl->neighbor[1].neighborLink[i].orFlag // or
				, aSectCtrl->neighbor[1].neighborLink[i].noUse // use
				, aSectCtrl->neighbor[1].neighborLink[i].leavDist // 距離
				, aSectCtrl->neighbor[1].neighborLink[i].remainDist // 塗りつぶし距離
				, aSectCtrl->neighbor[1].neighborLink[i].linkDist // リンク長
				, aSectCtrl->neighbor[1].neighborLink[i].subRemainDist // 塗りつぶし距離
				, aSectCtrl->neighbor[1].neighborLink[i].subDist // リンク長
				, aSectCtrl->neighbor[1].neighborLink[i].cost // 初期コスト
				);
		SC_LOG_DebugPrint(SC_TAG_RC, " D側 %s", buf);
	}
}

#define DP_RC_AREA_LEVELTBL				(1)
#define DP_RC_AREA_LEVELTBL_DIVINFO		(1)
#define DP_RC_AREA_LEVELTBL_PCLSTATE	(1)
#define DP_RC_AREA_Lv1_INFO				(1)
/**
 * @brief エリア情報ダンプ
 * @param レベル管理
 * @param O側情報
 * @param D側情報
 */
void RPDBG_ShowAreaInfo(SCRP_LEVELTBL *aLeveltbl, T_AreaInfo *oBaseAreaLv1, T_AreaInfo *dBaseAreaLv1) {

#if DP_RC_AREA_LEVELTBL
	//ダンプ確認
	SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDmp]topLevel[%d]", aLeveltbl->topLevel);
	SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDmp]areaTable[0]parcelId[0x%08x]", aLeveltbl->areaTable[0].pclRect.parcelId);
	SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDmp]areaTable[0]xSize	[%4d]", aLeveltbl->areaTable[0].pclRect.xSize);
	SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDmp]areaTable[0]ySize	[%4d]", aLeveltbl->areaTable[0].pclRect.ySize);
	SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDmp]areaTable[0]divIdx	[%4d]", aLeveltbl->areaTable[0].divIdx);
	SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDmp]areaTable[0]divVol	[%4d]", aLeveltbl->areaTable[0].divVol);
	SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDmp]divInfoVol			[%4d]", aLeveltbl->divInfoVol);
	SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDmp]pclStateVol			[%4d]", aLeveltbl->pclStateVol);
	if (RP_LEVEL2 == aLeveltbl->topLevel) {
		SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDmp]areaTable[1]parcelId[0x%08x]", aLeveltbl->areaTable[1].pclRect.parcelId);
		SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDmp]areaTable[1]xSize	[%4d]", aLeveltbl->areaTable[1].pclRect.xSize);
		SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDmp]areaTable[1]ySize	[%4d]", aLeveltbl->areaTable[1].pclRect.ySize);
		SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDmp]areaTable[1]divIdx	[%4d]", aLeveltbl->areaTable[1].divIdx);
		SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDmp]areaTable[1]divVol	[%4d]", aLeveltbl->areaTable[1].divVol);
		SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDmp]areaTable[2]parcelId[0x%08x]", aLeveltbl->areaTable[2].pclRect.parcelId);
		SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDmp]areaTable[2]xSize	[%4d]", aLeveltbl->areaTable[2].pclRect.xSize);
		SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDmp]areaTable[2]ySize	[%4d]", aLeveltbl->areaTable[2].pclRect.ySize);
		SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDmp]areaTable[2]divIdx	[%4d]", aLeveltbl->areaTable[2].divIdx);
		SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDmp]areaTable[2]divVol	[%4d]", aLeveltbl->areaTable[2].divVol);
	}
#endif
	INT16 i, j = 0;
#if DP_RC_AREA_LEVELTBL_DIVINFO
	for (i = 0; i < aLeveltbl->divInfoVol; i++) {
		SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDmp]divArea[%3d]parcelId[0x%08x]", i + 1, aLeveltbl->divInfo->pclRect.parcelId);
		SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDmp]divArea[%3d]xSize	[%4d]", i + 1, aLeveltbl->divInfo->pclRect.xSize);
		SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDmp]divArea[%3d]ySize	[%4d]", i + 1, aLeveltbl->divInfo->pclRect.ySize);
		SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDmp]divArea[%3d]pclIdx	[%4d]", i + 1, aLeveltbl->divInfo->pclIdx);
		SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDmp]divArea[%3d]pclVol	[%4d]", i + 1, aLeveltbl->divInfo->pclVol);
#if DP_RC_AREA_LEVELTBL_PCLSTATE
		for (j = 0; j < aLeveltbl->divInfo->pclVol; j++) {
			SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDmp]pclNum[%3d]dens	[%4d]", j + 1, aLeveltbl->pclState->linkDensity);
			SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDmp]pclNum[%3d]join	[%4d]", j + 1, aLeveltbl->pclState->join_f);
			SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDmp]pclNum[%3d]split	[%4d]", j + 1, aLeveltbl->pclState->split_f);
			aLeveltbl->pclState += 1;
		}
#endif
		aLeveltbl->divInfo += 1;
	}
#endif

#if DP_RC_AREA_Lv1_INFO
	SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDmp]oBaseLv1_parcelId	[0x%08x]", oBaseAreaLv1->pclRect.parcelId);
	SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDmp]oBaseLv1_xSize	[%4d]", oBaseAreaLv1->pclRect.xSize);
	SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDmp]oBaseLv1_ySize	[%4d]", oBaseAreaLv1->pclRect.ySize);
	SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDmp]dBaseLv1_parcelId	[0x%08x]", dBaseAreaLv1->pclRect.parcelId);
	SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDmp]dBaseLv1_xSize	[%4d]", dBaseAreaLv1->pclRect.xSize);
	SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDmp]dBaseLv1_ySize	[%4d]", dBaseAreaLv1->pclRect.ySize);
#endif
	return;
}

/**
 * @brief エリア接続情報ダンプ
 * @param レベル管理
 * @memo  探索順番に反して接続フラグがあればwarnPrintする
 * 		  ただし、分割が多いと数枚はこのPrintが出てくる
 * 		  もし、warnPrintが何十枚も出てくるときは接続フラグの不具合の
 * 		  可能性があるので、#if 0を1に変えて全ての接続情報を確認する
 */
void RPDBG_CheckAreaJoinFlg(SCRP_LEVELTBL *aLeveltbl) {

	UINT32 targetParcelId = 0x00;
	UINT32 targetcheckParcelId = 0x00;
	//UINT16 targetXsize = 0;
	//UINT16 targetYSize = 0;
	UINT8 flgCnt = 0;
	UINT8 targetJoin = 0x00;
	UINT16 m = 0;
	UINT16 n = 0;
	UINT16 s = 0;
	UINT16 t = 0;
	UINT32 lastCheckParcelId = 0x00;
	UINT8 i = 0;
	UINT8 j = 0;
	UINT32 checkEndAreaCnt = 0;
	SCRP_AREAPCLSTATE *pclst = aLeveltbl->pclState;
	SCRP_DIVAREA *targetDiv = aLeveltbl->divInfo;
	SCRP_DIVAREA *lastCheckDiv = aLeveltbl->divInfo;

	for (i = 0; i < aLeveltbl->divInfoVol; i++, targetDiv++, checkEndAreaCnt++) {
		targetParcelId = targetDiv->pclRect.parcelId;
		for (n = 0; n < targetDiv->pclRect.ySize; n++) {
			for (m = 0; m < targetDiv->pclRect.xSize; m++, pclst++) {
				targetParcelId = targetDiv->pclRect.parcelId;
				targetParcelId = SC_MESH_SftParcelId(targetParcelId, m, n);
#if 0
				//全てのパーセルのjoinを知りたいとき
				SC_LOG_InfoPrint(SC_TAG_RC, "[JoinFlgAll]parcelId	[0x%08x]divNumber[%4d]x[%4d]y{%4d]join[%4d]",
						targetParcelId, i + 1, m, n, pclst->join_f);
#endif
				for (flgCnt = 0; flgCnt < 8; flgCnt++) {
					targetJoin = 0x01;
					targetJoin = targetJoin << flgCnt;
					if (0 != (targetJoin & pclst->join_f)) {
						//接続フラグあり
#if 0
						//接続があるパーセルはどの方向にフラグが立っているか知りたいとき
						SC_LOG_InfoPrint(SC_TAG_RC, "[JoinFlgExist]parcelId	[0x%08x]divNumber[%4d]x[%4d]y{%4d]flg[%4d]",
								targetParcelId, i + 1, m, n, flgCnt + 1);
#endif
					} else {
						continue;
					}

					switch (flgCnt) {
					case 0:
						targetcheckParcelId = SC_MESH_SftParcelId(targetParcelId, -1, -1);
						break;
					case 1:
						targetcheckParcelId = SC_MESH_SftParcelId(targetParcelId, 0, -1);
						break;
					case 2:
						targetcheckParcelId = SC_MESH_SftParcelId(targetParcelId, 1, -1);
						break;
					case 3:
						targetcheckParcelId = SC_MESH_SftParcelId(targetParcelId, -1, 0);
						break;
					case 4:
						targetcheckParcelId = SC_MESH_SftParcelId(targetParcelId, 1, 0);
						break;
					case 5:
						targetcheckParcelId = SC_MESH_SftParcelId(targetParcelId, -1, 1);
						break;
					case 6:
						targetcheckParcelId = SC_MESH_SftParcelId(targetParcelId, 0, 1);
						break;
					case 7:
						targetcheckParcelId = SC_MESH_SftParcelId(targetParcelId, 1, 1);
						break;
					default:
						targetcheckParcelId = SC_MESH_SftParcelId(targetParcelId, 0, 0);
						break;
					}

					lastCheckDiv = aLeveltbl->divInfo;
					for (j = 0; j < checkEndAreaCnt; j++, lastCheckDiv++) {
						lastCheckParcelId = lastCheckDiv->pclRect.parcelId;
						for (t = 0; t < lastCheckDiv->pclRect.ySize; t++) {
							for (s = 0; s < lastCheckDiv->pclRect.xSize; s++) {
								lastCheckParcelId = lastCheckDiv->pclRect.parcelId;
								lastCheckParcelId = SC_MESH_SftParcelId(lastCheckParcelId, s, t);
								if (targetcheckParcelId == lastCheckParcelId) {
									//進行と逆方向にjoin
									SC_LOG_WarnPrint(SC_TAG_RC,
											"[CheckAreaJoinFlg]parcelId	[0x%08x]parcelcheckId[0x%08x]divNumber[%4d]x[%4d]y{%4d]flg[%4d]",
											targetParcelId, targetcheckParcelId, i + 1, m, n, flgCnt + 1);

								}
							}
						}
					}
				}
			}
		}
	}
	return;
}


void RPDBG_CheckIntegrityCand(SCRP_SECTCONTROLER* aSect) {
	SCRP_CANDDATA* cand = aSect->candMng.cand;
	UINT32 i;

	for (i = 0; i < 3; i++) {
		SC_LOG_InfoPrint(SC_TAG_RC, "cand=%5d size=%5d stcand=%5d size=%5d", aSect->candMng.candTblInfo[i].candIdx,
				aSect->candMng.candTblInfo[i].candSize, aSect->candMng.candTblInfo[i].stLinkIdx, aSect->candMng.candTblInfo[i].stLinkSize);
	}

	// ▼探索開始リンク以外でnextがALLF32のデータチェック
	for (i = 0; i < aSect->candMng.candCurrent; i++, cand++) {
		if (ALL_F32 == cand->next && !RCND_GET_STARTLINKFLG(cand->flag)) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "idx=%5d pcl=0x%08x link=0x%08x next=0x%08x", i, cand->parcelId, cand->linkId, cand->next);
		}
	}
	// TODO ▼同一リンクが開始リンクとして登録されていないかチェック
}

void RPDBG_CheckIntegrityNet(SCRP_SECTCONTROLER* aSect) {
	//SCRP_CANDDATA* cand = aSect->candMng.cand;
	UINT32 i, e;
	//UINT32 u;
	SCRP_PCLINFO* pclInfo = aSect->netTable.parcelInfo;
	SCRP_LINKINFO* linkInfo = NULL;
	//SCRP_PCLRECT* rect = &aSect->levelTable.areaTable[3].pclRect;

	for (i = 0; i < aSect->netTable.parcelInfoVol; i++, pclInfo++) {
		linkInfo = aSect->netTable.linkTable;
		for (e = 0; e < pclInfo->linkIdx; i++, linkInfo++) {
			// ▼接続情報外で接続情報が格納されていないかチェック
			if (pclInfo->linkIdVol < e) {
				if (!RCND_GET_CONNECTLINK(linkInfo->linkNet[0].flag)) {
					SC_LOG_ErrorPrint(SC_TAG_RC, "1 idx=%5d pcl=0x%08x linkIdx=0x%08x", e, pclInfo->parcelId, linkInfo->detaIndex);
				}
				if (!RCND_GET_CONNECTLINK(linkInfo->linkNet[1].flag)) {
					SC_LOG_ErrorPrint(SC_TAG_RC, "2 idx=%5d pcl=0x%08x linkIdx=0x%08x", e, pclInfo->parcelId, linkInfo->detaIndex);
				}
			} else {
				if (RCND_GET_CONNECTLINK(linkInfo->linkNet[0].flag)) {
					SC_LOG_ErrorPrint(SC_TAG_RC, "3 idx=%5d pcl=0x%08x linkIdx=0x%08x", e, pclInfo->parcelId, linkInfo->detaIndex);
				}
				if (RCND_GET_CONNECTLINK(linkInfo->linkNet[1].flag)) {
					SC_LOG_ErrorPrint(SC_TAG_RC, "4 idx=%5d pcl=0x%08x linkIdx=0x%08x", e, pclInfo->parcelId, linkInfo->detaIndex);
				}
			}
			// TODO 下位パーセル外で下位パーセル接続リンク情報が設定されていないかチェック
		}
	}
}

void RPDBG_CheckIntegrityRoute(SC_RP_RouteMng* aRoute) {
	UINT32 prePclId = 0, preLinkId = 0;
	UINT16 preX = 0, preY = 0;
	UINT32 i, e, u;

	SC_RP_SectInfo* sect = aRoute->sectInfo;
	for (i = 0; i < aRoute->sectVol; i++, sect++) {
		SC_RP_ParcelInfo* pclInfo = aRoute->parcelInfo + sect->parcelIdx;
		for (e = 0; e < sect->parcelVol; e++, pclInfo++) {
			SC_RP_LinkInfo* linkInfo = aRoute->linkInfo + pclInfo->linkIdx;
			for (u = 0; u < pclInfo->linkVol; u++, linkInfo++) {
				// ▼推奨経路のダブりチェック
				if (prePclId == pclInfo->parcelId && preLinkId == linkInfo->linkId) {
					SC_LOG_ErrorPrint(SC_TAG_RC, "same! pcl=0x%08x linkId=0x%08x", pclInfo->parcelId, linkInfo->linkId);
				}
				// ▼推奨経路の形状点接続チェック
				SC_RP_FormInfo* edForm;
				SC_RP_FormInfo* stForm;
				stForm = aRoute->formInfo + sect->formIdx + linkInfo->formIdx;
				edForm = aRoute->formInfo + sect->formIdx + linkInfo->formIdx + linkInfo->formVol - 1;
				// 座標を正規化
				if (prePclId != pclInfo->parcelId) {
					if (preX == 0) {
						preX = 4096;
					} else if (preX == 4096) {
						preX = 0;
					}
					if (preY == 0) {
						preY = 4096;
					} else if (preY == 4096) {
						preY = 0;
					}
				}
				if (SC_RP_DIR_IS_ODR == linkInfo->orFlag) {
					if (preX != stForm->x || preY != stForm->y) {
						SC_LOG_ErrorPrint(SC_TAG_RC, "not connect form pcl=0x%08x linkId=0x%08x or=%d tarm=0x%02x xy=%4d,%4d != %4d,%4d", pclInfo->parcelId, linkInfo->linkId, linkInfo->orFlag, linkInfo->termFlag, preX, preY, stForm->x, stForm->y);
					}
					preX = edForm->x;
					preY = edForm->y;
				} else {
					if (preX != edForm->x || preY != edForm->y) {
						SC_LOG_ErrorPrint(SC_TAG_RC, "not connect form pcl=0x%08x linkId=0x%08x or=%d tarm=0x%02x xy=%4d,%4d!=%4d,%4d", pclInfo->parcelId, linkInfo->linkId, linkInfo->orFlag, linkInfo->termFlag, preX, preY, edForm->x, edForm->y);
					}
					preX = stForm->x;
					preY = stForm->y;
				}
				prePclId = pclInfo->parcelId;
				preLinkId = linkInfo->linkId;
			}
		}
	}
}

/**
 * ダイクストラ後のヒープ値の異常チェック
 */
void RPDBG_CheckIntegrityNetHeap(SCRP_NETCONTROLER *aNetCtrl) {
	UINT32 i;
	UINT32 vVol = 0;
	UINT32 uVol = 0;
	UINT32 nVol = 0;
	UINT32 eVol = 0;

	for (i = 0; i < aNetCtrl->linkTableVol; i++) {
		if (SCRP_HEAP_V == (aNetCtrl->linkTable + i)->linkNet[0].heap) {
			vVol++;
		} else if (SCRP_HEAP_UV == (aNetCtrl->linkTable + i)->linkNet[0].heap) {
			uVol++;
		} else if (SCRP_HEAP_NU == (aNetCtrl->linkTable + i)->linkNet[0].heap) {
			nVol++;
		} else {
			SC_LOG_ErrorPrint(SC_TAG_RC, " o error hit 0x%08x %9d %9d", (aNetCtrl->linkTable + i)->linkNet[0].inLinkHist,
					(aNetCtrl->linkTable + i)->linkNet[0].heap, i);
			eVol++;
		}
		if (SCRP_HEAP_V == (aNetCtrl->linkTable + i)->linkNet[1].heap) {
			vVol++;
		} else if (SCRP_HEAP_UV == (aNetCtrl->linkTable + i)->linkNet[1].heap) {
			uVol++;
		} else if (SCRP_HEAP_NU == (aNetCtrl->linkTable + i)->linkNet[1].heap) {
			nVol++;
		} else {
			SC_LOG_ErrorPrint(SC_TAG_RC, " d error hit 0x%08x %9d %9d", (aNetCtrl->linkTable + i)->linkNet[1].inLinkHist,
					(aNetCtrl->linkTable + i)->linkNet[1].heap, i);
			eVol++;
		}
	}
	// ダイクストラ後VorUVorNU以外の値になる場合NG
	SC_LOG_DebugPrint(SC_TAG_RC, " vVol=%d", vVol);
	SC_LOG_DebugPrint(SC_TAG_RC, " uVol=%d", uVol);
	SC_LOG_DebugPrint(SC_TAG_RC, " nVol=%d", nVol);
	SC_LOG_DebugPrint(SC_TAG_RC, " eVol=%d", eVol);
}

/**
 * @brief コスト計算用基準値テーブルダンプ
 */
void RPDBG_ShowCalcCost(UINT32* base, UINT32* unit, UINT32* angle, UINT32* aapply) {
	// 高速優先の乗算値
	UINT32 i, e;

	SC_LOG_InfoPrint(SC_TAG_RC, "BaseCost");
	for (i = 0; i < ROADTYPE_SIZE; i++) {
		char buf[256] = {};
		for (e = 0; e < LINKTYPE_SIZE; e++) {
			sprintf(buf, "%s[%3d]", buf, *(base + (i * LINKTYPE_SIZE) + e));
		}
		SC_LOG_InfoPrint(SC_TAG_RC, " %2d %s", i, buf);
	}
	SC_LOG_InfoPrint(SC_TAG_RC, "CostUnit");
	for (i = 0; i < ROADTYPE_SIZE; i++) {
		char buf[256] = {};
		for (e = 0; e < LINKTYPE_SIZE; e++) {
			sprintf(buf, "%s[%3d]", buf, *(unit + (i * LINKTYPE_SIZE) + e));
		}
		SC_LOG_InfoPrint(SC_TAG_RC, " %2d %s", i, buf);
	}
	SC_LOG_InfoPrint(SC_TAG_RC, "AngleCost");
	for (i = 0; i < TRI_PART_SIZE; i++) {
		char buf[256] = {};
		for (e = 0; e < TRI_PART_SIZE; e++) {
			sprintf(buf, "%s[%5d]", buf, *(angle + (i * TRI_PART_SIZE) + e));
		}
		SC_LOG_InfoPrint(SC_TAG_RC, " %2d %s", i, buf);
	}
	SC_LOG_InfoPrint(SC_TAG_RC, "AngleFlag");
	for (i = 0; i < ROADTYPE_SIZE; i++) {
		char buf[256] = {};
		for (e = 0; e < ROADTYPE_SIZE; e++) {
			sprintf(buf, "%s[%3d]", buf, *(aapply + (i * ROADTYPE_SIZE) + e));
		}
		SC_LOG_InfoPrint(SC_TAG_RC, " %2d %s", i, buf);
	}
}

/**
 * @brief 対象とするリンクの計算情報をダンプする
 */
void RPDBG_ShowTargetCalcNowLink(SCRP_NETCONTROLER* aNetCtrl, SCRC_CROSSLINKTBL* aCrossLinkTbl, UINT32 aParcelId, UINT32 aLinkId,
		UINT32 aIdx, UINT32 aNewCost, SCRC_RESULTCOSTS* aCost) {
	MAL_HDL pLink =
			SC_MA_A_NWRCD_LINK_GET_RECORD(aCrossLinkTbl->baseLink.pclInfo->mapNetworkLinkBin, aCrossLinkTbl->baseLink.linkTable->detaIndex);
	if (aParcelId == aCrossLinkTbl->baseLink.pclInfo->parcelId) {
		if (aLinkId == SC_MA_D_NWRCD_LINK_GET_ID(pLink) ) {
			SC_LOG_InfoPrint(SC_TAG_RC, " hitLink pcl=0x%08x link=0x%08x cost=%7d", aParcelId, aLinkId, aCrossLinkTbl->baseCost);
			UINT32 i;
			for (i = 0; i < aCrossLinkTbl->listVol; i++) {
				char buffer[512] = {};
				SCRP_LINKINFO* linkInfo = aCrossLinkTbl->linkList[i].linkTable;
				SCRP_PCLINFO* pclInfo = aCrossLinkTbl->linkList[i].pclInfo;
				SCRP_NETDATA* netData = aCrossLinkTbl->linkList[i].linkNet;
				MAL_HDL pLink = SC_MA_A_NWRCD_LINK_GET_RECORD(pclInfo->mapNetworkLinkBin, linkInfo->detaIndex);

				sprintf(buffer, " %2d pcl=0x%08x link=0x%08x cost=%7d", i, pclInfo->parcelId, SC_MA_D_NWRCD_LINK_GET_ID(pLink),
						netData->costSum);
				if (aIdx == i) {
					sprintf(buffer, "%s travel=%4d angle=%4d <- newCost=%7d", buffer, aCost->travelCost, aCost->crossCost, aNewCost);
				}
				sprintf(buffer, "%s idx=%4d ", buffer, pclInfo->linkIdx + linkInfo->detaIndex);
				SC_LOG_InfoPrint(SC_TAG_RC, "%s", buffer);
			}
		}
	}
}
