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
 * SMCoreRPInternalApi.h
 *
 *  Created on: 2015/11/11
 *      Author: masutani
 */

#ifndef SMCORERPINTERNALAPI_H_
#define SMCORERPINTERNALAPI_H_

/**
 * RC_Area
 */
E_SC_RESULT RC_Area(SCRP_LEVELTBL* aLevel, SCRP_NETCONTROLER* aNetCtrl, SCRP_NEIGHBORINFO* aONbor, SCRP_NEIGHBORINFO* aDNbor);
E_SC_RESULT RC_AreaOnlyLv1(SCRP_LEVELTBL* aLevel, SCRP_NETCONTROLER* aNetTab, SCRP_NEIGHBORINFO* aONbor, SCRP_NEIGHBORINFO* aDNbor);
E_SC_RESULT RC_AreaOnlyLv2(SCRP_LEVELTBL* aLevel, SCRP_NETCONTROLER* aNetTab, SCRP_NEIGHBORINFO* aONbor, SCRP_NEIGHBORINFO* aDNbor);
E_SC_RESULT RC_GetParcelDensity(T_DHC_ROAD_DENSITY *aDensity, UINT32 aPid, UINT16 aX, UINT16 aY);
E_SC_RESULT RC_AreaStackPush(RC_AREASTATEINFO* info);
E_SC_RESULT RC_AreaStackPop(RC_AREASTATEINFO* info);

/**
 * RC_AreaDiv
 */
void RC_AreaDivFree(SCRP_LEVELTBL* aLevel);
E_SC_RESULT RC_AreaDiv(SCRP_LEVELTBL_OLD* aLevel, SCRP_NEIGHBORINFO* aONbor, SCRP_NEIGHBORINFO* aDNbor,
		T_DHC_ROAD_DENSITY* density_area_ptr);
E_SC_RESULT RC_AreaDiv_Change_ParcelID_Pos(INT32 target_level,		// 変換するﾚﾍﾞﾙ
		UINT32 base_parcelID,		// 変換するﾊﾟｰｾﾙID
		DOUBLE base_x,				// 変換する正規化Ｘ座標
		DOUBLE base_y,				// 変換する正規化Ｙ座標
		UINT32* target_parcelID,	// 変換するﾊﾟｰｾﾙID
		DOUBLE* target_x,			// 変換する正規化Ｘ座標
		DOUBLE* target_y			// 変換する正規化Ｙ座標
		);
E_SC_RESULT RC_Div_Search_Area(INT32 process_f,					//	処理タイプ
																//		０：分割探索エリア（T_DivAreaInfo、T_ParcelList）を生成
																//		１：分割探索エリア（T_DivAreaInfo、T_ParcelList）に設定
		SCRP_LEVELTBL_OLD* aLevel,						//	トップレベル探索エリア
		INT32 baseParcelId,				//	非分割エリア左下絶対パーセルＩＤ（相対位置0:0の時）
		UINT32 O_parcelID,					//	出発地点の絶対パーセルＩＤ（レベル1）
		UINT32 D_parcelID,					//	目的地点の絶対パーセルＩＤ（レベル1）
		INT32 parcel_range_cnt_per_block,	//	分割探索エリアの一辺の最大パーセル数(DIV_NETWORK_MAX_PARCEL_RANGE_CNT)
		INT32 parcel_cnt_per_block,		//	ブロック当たりのパーセル読み込み数（DIV_NETWORK_PARCEL_CNT）
		INT32 max_parcel_cnt,				//	分割探索エリアの最大パーセル数
		INT32 max_link_cnt,				//	分割探索エリアの最大リンク本数

		INT32 block_start,				//	ブロック開始・相対位置
		INT32 block_end,					//	ブロック終了・相対位置
		INT32 block_delt,					//	ブロック増減値
		INT32 block_range,				//	ブロック方向の範囲

		INT32 layer_start,				//	階層開始・相対位置
		INT32 layer_end,					//	階層終了・相対位置
		INT32 layer_delt,					//	階層増減値
		INT32 layer_range,				//	階層の範囲

		INT32 block_f,					//	矩形の形状	０：横長	１：縦長
		T_DHC_ROAD_DENSITY* density_area_ptr,			//	道路密度データ取得関数の出力値
		SCRP_NEIGHBORINFO* aONbor,						//	出発地点
		SCRP_NEIGHBORINFO* aDNbor,						//	目的地点
		INT32 divarea_cnt,				//	分割エリア数（処理タイプ：１の時、有効）
		INT32 parcel_cnt					//	パーセル数（処理タイプ：１の時、有効）
		);
E_SC_RESULT RC_Div_Set_Out_Search_Area(INT32 process_f,					//	処理タイプ
																		//		０：分割探索エリア（T_DivAreaInfo、T_ParcelList）を生成
																		//		１：分割探索エリア（T_DivAreaInfo、T_ParcelList）に設定
		SCRP_LEVELTBL_OLD* aLevel,						//	トップレベル探索エリア
		INT32 baseParcelId,				//	非分割エリア左下絶対パーセルＩＤ（相対位置0:0の時）
		UINT32 O_parcelID,					//	出発地点の絶対パーセルＩＤ（レベル1）
		UINT32 D_parcelID,					//	目的地点の絶対パーセルＩＤ（レベル1）
		INT32 parcel_range_cnt_per_block,	//	分割探索エリアの一辺の最大パーセル数(DIV_NETWORK_MAX_PARCEL_RANGE_CNT)
		INT32 parcel_cnt_per_block,		//	ブロック当たりのパーセル読み込み数（DIV_NETWORK_PARCEL_CNT）
		INT32 max_parcel_cnt,				//	分割探索エリアの最大パーセル数
		INT32 max_link_cnt,				//	分割探索エリアの最大リンク本数

		INT32 block_start,				//	ブロック開始・相対位置
		INT32 block_end,					//	ブロック終了・相対位置
		INT32 block_delt,					//	ブロック増減値
		INT32 block_range,				//	ブロック方向の範囲

		INT32 layer_start,				//	階層開始・相対位置
		INT32 layer_end,					//	階層終了・相対位置
		INT32 layer_delt,					//	階層増減値
		INT32 layer_range,				//	階層の範囲

		INT32 block_f,					//	矩形の形状	０：横長	１：縦長
		T_DHC_ROAD_DENSITY* density_area_ptr,			//	道路密度データ取得関数の出力値
		SCRP_NEIGHBORINFO* aONbor,						//	出発地点
		SCRP_NEIGHBORINFO* aDNbor,						//	目的地点
		INT32 divarea_cnt,				//	分割エリア数（処理タイプ：１の時、有効）
		T_DHC_DOWNLOAD_AREA* downloadarea_ptr,			//	ダウンロードエリア
		T_RC_DivAreaInf* T_RC_DivAreaInf_ptr			//	「分割探索エリア管理情報」
		);
E_SC_RESULT RC_AreaDiv_Create_1_Block_Search_Area(SCRP_LEVELTBL_OLD* aLevel,						//	トップレベル探索エリア
		INT32 baseParcelId,				//	非分割エリア左下絶対パーセルＩＤ（相対位置0:0の時）
		T_DHC_ROAD_DENSITY* density_area_ptr,			//	道路密度データ取得関数の出力値
		SCRP_NEIGHBORINFO* aONbor,						//	出発地点
		SCRP_NEIGHBORINFO* aDNbor,						//	目的地点
		INT32 parcel_range_cnt_per_block,	//	分割探索エリアの一辺の最大パーセル数(DIV_NETWORK_MAX_PARCEL_RANGE_CNT)
		INT32 parcel_cnt_per_block,		//	ブロック当たりのパーセル読み込み数（DIV_NETWORK_PARCEL_CNT）
		INT32 max_parcel_cnt,				//	分割探索エリアの最大パーセル数
		INT32 max_link_cnt,				//	分割探索エリアの最大リンク本数
		INT32 O_alter_pos_x,				//	非分割エリ内Ｘ方向の相対位置（出発地点）
		INT32 O_alter_pos_y,				//	非分割エリ内Ｙ方向の相対位置（出発地点）
		INT32 D_alter_pos_x,				//	非分割エリ内Ｘ方向の相対位置（目的地点）
		INT32 D_alter_pos_y,				//	非分割エリ内Ｙ方向の相対位置（目的地点）
		T_RC_DivAreaInf* T_RC_DivAreaInf_ptr			//	「分割探索エリア管理情報」
		);
E_SC_RESULT RC_AreaDiv_Set_OutWorkArea_by_Connection_f(INT32 O_alter_pos_x,			//	Ｘ方向の相対位置（出発地点）
		INT32 O_alter_pos_y,			//	Ｙ方向の相対位置（出発地点）
		INT32 D_alter_pos_x,			//	Ｘ方向の相対位置（目的地点）
		INT32 D_alter_pos_y,			//	Ｙ方向の相対位置（目的地点）
		T_RC_DivAreaInf* T_RC_DivAreaInf_ptr,	//	「分割探索エリア管理情報」
		INT32 divarea_cnt,			//	「分割探索エリア管理情報」数
		T_RC_DivAreaInf** divarea_ptr_ptr			//	「分割探索エリア管理情報」アドレス格納テーブル
		);
void RC_AreaDiv_Set_Connect_Flag_f(INT32 block_delt,				//	ブロック増減値
		INT32 layer_delt,				//	階層増減値
		INT32 block_f,				//	矩形の形状	０：横長	１：縦長
		INT32 divarea_cnt,			//	「分割探索エリア管理情報」数
		T_RC_DivAreaInf** divarea_ptr_ptr			//	「分割探索エリア管理情報」アドレス格納テーブル
		);
void RC_AreaDiv_Set_Connect_f(INT32 alt_x,				// 	自パーセルに対する他方のパーセル相対位置（Ｘ方向）
		INT32 alt_y,				// 	自パーセルに対する他方のパーセル相対位置（Ｙ方向）
		INT32 x_size,				// 	分割エリア内パーセル枚数（Ｘ方向）
		INT32 y_size,				// 	分割エリア内パーセル枚数（Ｙ方向）
		SCRP_AREAPCLSTATE* T_ParcelList_ptr	//	T_ParcelList テーブル
		);
void RC_AreaDiv_Set_DisConnect(INT32 ALT_X,						// 	非分割エリア内の相対位置（Ｘ方向）
		INT32 ALT_Y,						// 	非分割エリア内の相対位置（Ｙ方向）
		INT32 x_size,						// 	分割エリア内パーセル枚数（Ｘ方向）
		INT32 y_size,						// 	分割エリア内パーセル枚数（Ｙ方向）
		T_DHC_DOWNLOAD_AREA* downloadarea_ptr,			//	ダウンロードエリア
		T_DHC_ROAD_DENSITY* density_area_ptr,			//	道路密度データ取得関数の出力値
		INT32 x_range,					//	データ幅
		INT32 y_range,					//	データ高
		INT32 L_x,						//	収納範囲をチェックする位置・最小（Ｘ方向）
		INT32 L_y,						//	収納範囲をチェックする位置・最小（Ｙ方向）
		INT32 H_x,						//	収納範囲をチェックする位置・最大（Ｘ方向）
		INT32 H_y,						//	収納範囲をチェックする位置・最大（Ｙ方向）
		SCRP_AREAPCLSTATE* T_ParcelList_ptr			//	T_ParcelList テーブル
		);
E_SC_RESULT RC_AreaDiv_CheckDownload(T_DHC_DOWNLOAD_AREA* downloadarea_ptr,			//	ダウンロードエリア
		T_DHC_ROAD_DENSITY* density_area_ptr,			//	道路密度データ取得関数の出力値
		INT32 x_range,					//	データ幅（非分割）
		INT32 y_range,					//	データ高（非分割）
		INT32 L_x,						//	収納範囲をチェックする位置・最小（Ｘ方向）
		INT32 L_y,						//	収納範囲をチェックする位置・最小（Ｙ方向）
		INT32 H_x,						//	収納範囲をチェックする位置・最大（Ｘ方向）
		INT32 H_y,						//	収納範囲をチェックする位置・最大（Ｙ方向）
		INT32 x_pos,						//	Ｘ軸方向位置（非分割）
		INT32 y_pos						//	Ｙ軸方向位置（非分割）
		);

/**
 * RC_Candidate
 */
E_SC_RESULT RC_CandMngInit(SCRP_CANDMANAGER* aCandMng);
E_SC_RESULT RC_SplitCandMngInit(SCRP_CANDMANAGER* aCandMng, UINT32 aSplitVol);
void RC_CandFree(SCRP_CANDMANAGER* aCandMng);
void RC_SplitCandFree(SCRP_CANDMANAGER* aCandMng);
E_SC_RESULT RC_CandMake(SCRP_SECTCONTROLER* aSectCtrl, SCRP_NETCONTROLER* aNetCtrl, E_RP_RTCALCSTEP aStep);
E_SC_RESULT RC_CandPromotion(SCRP_CANDMANAGER* aCandMng, E_RP_RTCALCSTEP aStep);

/**
 * RC_CostCalc
 */
E_SC_RESULT RC_CostCalc(SCRP_NETCONTROLER* aNetCtrl, SCRC_IOCALCTBL* aInOut, SCRC_RESULTCOSTS* aCosts);
E_SC_RESULT RC_NoCrossCostCalc(SCRP_NETCONTROLER* aNetCtrl, SCRC_IOCALCTBL* aInOut, SCRC_RESULTCOSTS* aCosts);
E_SC_RESULT RC_CostCalcFuncSet(UINT32 cond);

/**
 * RC_DataAccess
 */
UINT8* RC_MapRead(UINT32 aParcelId, UINT8 aKind);
void RC_MapFree(UINT32 aParcelId, UINT8 aKind);
void RC_MapFreeAll();
E_SC_RESULT RC_ReadAreaMap(SCRP_PCLRECT* aPclRect, UINT32 aMapKind, SCRP_MAPREADTBL* aReadTbl);
E_SC_RESULT RC_ReadListMap(UINT32* aPclList, UINT32 aListVol, UINT32 aMapKind, SCRP_MAPREADTBL* aReadTbl);
E_SC_RESULT RC_FreeMapTbl(SCRP_MAPREADTBL* aReadTbl, Bool aMemfree);
E_SC_RESULT RC_SetDownLoad_Area();
T_DHC_DOWNLOAD_AREA *RC_GetDownLoad_Area();

/**
 * RC_Entrance
 */
void RC_SendSingleRouteResponse(E_SC_RESULT aResult, UINT32 aRtId, UINT16 aSettingIdx);
void RC_SendRePlanResponse(E_SC_RESULT aResult, UINT32 aRtId, UINT16 aSettingIdx);

/**
 * RC_Heap
 */
void RC_HeapInsert(SCRP_NETCONTROLER* aNetCtrl, UINT32 aData);
void RC_HeapDelete(SCRP_NETCONTROLER* aNetCtrl, UINT32 aData);
void RC_HeapUpdate(SCRP_NETCONTROLER* aNetCtrl, UINT32 aData);
E_SC_RESULT RC_MemAllocHeapTable(SCRP_NETCONTROLER* aNetCtrl);

/**
 * RC_LevelProcess
 */
E_SC_RESULT RC_LevelProcess(SCRP_SECTCONTROLER *aSectCtrl);

/**
 * RC_Neighbor
 */
E_SC_RESULT RC_SetNbrLinkIndex(SCRP_NEIGHBORINFO* aNeighbor);
E_SC_RESULT RC_SetNeighborLinkUseFlg(SCRP_SECTCONTROLER *aSectCtrl);
E_SC_RESULT RC_NeighborOSide(SCRP_SEARCHSETTING* aSetting, UINT8 aWayNum, SCRP_NEIGHBORINFO* aNeighbor);
E_SC_RESULT RC_NeighborDSide(SCRP_SEARCHSETTING* aSetting, UINT8 aWayNum, SCRP_NEIGHBORINFO* aNeighbor);
E_SC_RESULT RC_NeighborDSidePoiGate(SCRP_SEARCHSETTING* aSetting, UINT8 aWayNum, SCRP_NEIGHBORINFO* aNeighbor);
E_SC_RESULT RC_NeighborCarPoint(SCRP_SEARCHSETTING* aSetting, SCRP_NEIGHBORINFO* aNeighbor);
E_SC_RESULT RC_ReRouteNbrMake(SCRP_POINT* aBase, UINT32 aPclId, UINT32 aLinkId, UINT8 aOrFlag, SCRP_NEIGHBORINFO* aNbr);
E_SC_RESULT RP_MakeNeighborLinkInfo(SCRP_POINT* aPoint, UINT32 aPclId, UINT32 aLinkId, SCRP_NEIGHBORLINK* aNeighborLink);
E_SC_RESULT RC_FindMostNearRoute(SC_RP_RouteMng* aRouteMng, SCRP_POINT* aPoint, UINT32* aPclIdx, UINT32* aLinkIdx, UINT32* aFormIdx);

/**
 * RC_NetworkMake
 */
E_SC_RESULT RC_MakeNetwork(SCRP_NETCONTROLER *aNetCtrl, SCRP_SECTCONTROLER* aNetMng, E_RP_RTCALCSTEP aStep);
E_SC_RESULT RC_SetDestLinkFlag(SCRP_NETCONTROLER *aNetCtrl, SCRP_NEIGHBORINFO* aNbr);
UINT32 getNewNetLinkIndex(UINT32 aPclIdx, UINT32 aLinkIdx, UINT32 aOR);

/**
 * RC_RouteCalc
 */
E_SC_RESULT RC_DepthFirstDijkstra(SCRP_NETCONTROLER *aNetCtrl);
E_SC_RESULT RC_SetStartLink(SCRP_SECTCONTROLER* aSectCtrl, E_RP_RTCALCSTEP aStep, UINT16 aSplit);
E_SC_RESULT RC_StartNbrLinkSet(SCRP_NETCONTROLER *aNetCtrl, SCRP_NEIGHBORINFO* aNeighbor);
E_SC_RESULT RC_StartCandLinkSet(SCRP_NETCONTROLER *aNetCtrl, SCRP_CANDMANAGER* aCand, E_RP_RTCALCSTEP aStep);

/**
 * RC_RouteControl
 */
void RC_RtControlSingle(UINT32 aSearchID, UINT16 aSettingPage);
void RC_RtControlRePlan(UINT32 aSearchID, UINT16 aSettingPage);

/**
 * RC_RouteEdit
 */
E_SC_RESULT RC_RtCutting(SC_RP_RouteMng* aBaseRt, UINT8 aSectIdx, UINT8 aSectVol, SC_RP_RouteMng* aResultRt);
E_SC_RESULT RC_RtJoin(SC_RP_RouteMng** aRtList, UINT16 aRtVol, SC_RP_RouteMng* aResultRt);

/**
 * RC_RouteMake
 */
E_SC_RESULT RC_RouteMake(SCRP_MANAGER* aRCManager, SCRP_SECTCONTROLER* aNetMng, UINT16 aSectId);
SC_RP_RouteMng* RC_GetMasterRoute();
E_SC_RESULT RC_JointRePlanRoute();
E_SC_RESULT RC_BackupReleasedRoute(SC_RP_RouteMng* aReleasedRouteMng, UINT16 aStartIdx, UINT16 aBackupSize);
void RC_FinalRouteMakeTbl();
void RC_ResultFailFinalRouteMakeTbl();
void RC_ClearRouteMakeInfo();
E_SC_RESULT RC_CreateSectMngTbl(UINT16 aSectVol);
Bool RC_CheckSplitRoute(SC_RP_RouteMng*);

/**
 * RC_RouteReg
 */
E_SC_RESULT RC_RegJudgeFuncSet(SCRP_SEARCHSETTING* aSetting);
UINT16 RC_JudgeTimeReg(SCRP_NETCONTROLER* aNetCtrl, SCRC_IOCALCTBL* aInoutData);
UINT16 RC_JudgeSeasonReg(SCRP_NETCONTROLER* aNetCtrl, SCRC_IOCALCTBL* aInoutData);
Bool RC_JudgeOkinawaUTurn(SCRP_NETCONTROLER* aNetCtrl, SCRC_TARGETLINKINFO *aInLink, SCRC_TARGETLINKINFO *aOutLink);
Bool RC_JudgePatchReg(SCRP_NETCONTROLER* aNetCtrl, SCRC_TARGETLINKINFO *aInLink, SCRC_TARGETLINKINFO *aOutLink);

/**
 * RM_StateObj
 */
E_SC_RESULT RPM_InitState();
E_RP_STATE RPM_GetState();
void RPM_SetState(E_RP_STATE);
E_SC_RESULT RPM_AddRouteSearchSetting(pthread_msq_msg_t *aMsg, UINT32 aSearchId, UINT16 *aResultIdx);
E_SC_RESULT RPM_RemoveRouteSearchSetting();
SCRP_SEARCHSETTING* RPM_GetRouteSearchSetting(UINT16 aIdx);
SCRP_SEARCHSETTING* RPM_GetCurrentRouteSearchSetting();
void RPM_CancelFlagOn();
Bool RPM_JudgeSearchSetting(const SCRP_SEARCHSETTING *aSrcA, const SCRP_SEARCHSETTING *aSrcB);
void RPC_InitState();
SCRP_RPCSTATE *RPC_GetState();
SCRP_MANAGER* RPC_GetRouteCalcManager();
UINT16 RPC_GetCurrentSettingIdx();
void RPC_SetCurrentSettingIdx(INT16 index);
void RPC_SetCalcProcess(E_RC_CALC_PROCESS process);
void RPC_SetReplanFlag(Bool replan);
void RPC_SetErrorCode(INT32 error);
INT32 RPC_GetErrorCode();
void RPC_SetWarnCode(INT32 warn);
void RPC_SetTotalSect(INT32 sect);
void RPC_SetCalcSect(INT32 sect);
void RPC_SetTotalSplit(INT32 split);
void RPC_SetCalcSplit(INT32 split);
void RPC_SetProcess2ErrorCode(E_SC_RESULT result);
void RPC_SetResult2ErrorCode(E_SC_RESULT result);
void RPC_InitRouteCalcManager();
void RPC_SetRoutePlanType(E_SCRP_RPTYPE type);
E_SC_RESULT RPC_SetCurrentRouteSearchSetting();
E_SC_RESULT RPC_SetRouteSearchSetting(SCRP_SEARCHSETTING *aSetting);
Bool RPC_IsCancelRequest();
E_SC_RESULT RP_SetRoutePlanTip();
void RP_SetLapTime();
void RP_ClearLapTime();
void RP_OutputLapTime();
/**
 * RP_Lib
 */
DOUBLE RP_Lib_CalcODLength(SCRP_POINT aPointA, SCRP_POINT aPointB);
FLOAT RP_Lib_GetParcelRealXYRatio(UINT32 aParcelId, FLOAT aDefault);

/**
 * RP_MemJack
 */
void* RP_MemAlloc(size_t size, E_SC_MEM_TYPE type);
void RP_MemFree(void* mem, E_SC_MEM_TYPE type);
void RP_MemClean(E_SC_MEM_TYPE type);
void* RP_Memset0(void* mem, size_t size);
void RP_Memcpy(void* des, const void* src, size_t size);

/**
 * RP_RouteManager
 */
E_SC_RESULT SC_RP_RouteManagerInit();
E_SC_RESULT SC_RP_RouteAdd(UINT32 aRtId, UINT32 aRtType, SC_RP_RouteMng* aBuf);
E_SC_RESULT SC_RP_RouteDelete(UINT32 aRtId, UINT32 aRtType);
E_SC_RESULT SC_RP_RouteSetId(UINT32 aRtId, UINT32 aRtType);
E_SC_RESULT SC_RP_RouteCleaning();

/**
 * RP_LinkLevelConvert
 */
E_SC_RESULT RP_LinkLevelConvert(SCRP_LVCHANGE_TBL* aParam, E_SC_MEM_TYPE aMemType);
E_SC_RESULT RP_LinkLevelConvertTblFree(SCRP_LVCHANGE_TBL* aLvChangeTbl, E_SC_MEM_TYPE aMemType);

/**
 * RP_Debug
 */
void RPDBG_DumpRoute(SC_RP_RouteMng* aRouteMng, Bool aLinkDmp, Bool aFormDmp);
void RPDBG_ShowAreaInfo(SCRP_LEVELTBL *aLeveltbl, T_AreaInfo *oBaseAreaLv1, T_AreaInfo *dBaseAreaLv1);
void RPDBG_CheckAreaJoinFlg(SCRP_LEVELTBL *aLeveltbl);
void RPDBG_ShowNetwork(SCRP_NETCONTROLER* aNetCtrl);
void RPDBG_ShowStepLog(SCRP_SECTCONTROLER *aSectCtrl, E_RP_RTCALCSTEP aStep);
void RPDBG_ShowCandSizeInfo(SCRP_SECTCONTROLER *aSectCtrl, E_RP_RTCALCSTEP aStep, UINT16 split);
void RPDBG_ShowCandInfo(SCRP_SECTCONTROLER *aSectCtrl, E_RP_RTCALCSTEP aStep, UINT16 split);
void RPDBG_ShowSectLvInfo(SCRP_SECTCONTROLER* sectCtrl);
void RPDBG_ShowNeigborLink(SCRP_SECTCONTROLER* aSectCtrl);
void RPDBG_CheckIntegrityCand(SCRP_SECTCONTROLER* aSect);
void RPDBG_CheckIntegrityNet(SCRP_SECTCONTROLER* aSect);
void RPDBG_CheckIntegrityRoute(SC_RP_RouteMng* aRoute);
void RPDBG_CheckIntegrityNetHeap(SCRP_NETCONTROLER *aNetCtrl);
void RPDBG_ShowCalcCost(UINT32* base, UINT32* unit, UINT32* angle, UINT32* aapply);
void RPDBG_ShowTargetCalcNowLink(SCRP_NETCONTROLER* aNetCtrl, SCRC_CROSSLINKTBL* aCrossLinkTbl, UINT32 aParcelId, UINT32 aLinkId,
		UINT32 aIdx, UINT32 aNewCost, SCRC_RESULTCOSTS* aCost);
#endif /* SMCORERPINTERNALAPI_H_ */
