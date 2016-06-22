/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

///*-------------------------------------------------------------------*
// * * File：RC_AreaDivTop.c
// * Info：分割エリア作成_top
// *-------------------------------------------------------------------*/

#include "sms-core/SMCoreRP/SMCoreRPInternal.h"

extern INT8 gDivStackIdx;				// スタックインデックス

typedef struct _E_RC_DIVAREANEIGHBORINFO {
	E_RC_DIVAREA_CORNERNBRFLG neighborflgX;		// 左右隅どちらが近いか
	E_RC_DIVAREA_CORNERNBRFLG neighborflgY;		// 上下隅どちらが近いか
	INT32 closestCornerSftX;					// 最寄までの横シフト量
	INT32 closestCornerSftY;					// 最寄までの縦シフト量
} E_RC_DIVAREA_NBRINFO;

static E_SC_RESULT RC_AreaDivTopSetArea(INT16 topLevel,RC_AREASTATEINFO *beforeDivArea,RC_BASEAREATOPLVINFO *baseAreaTopLvInf,INT32 alterPosX,INT32 alterPosY);
static E_SC_RESULT RC_AreaDivTopSetQueue(INT16 topLevel,RC_DIVAREA_QUEUE *divQueue,RC_AREASTATEINFO *beforeDivArea,RC_BASEAREATOPLVINFO *baseAreaTopLvInf,INT16 procNo);
static E_SC_RESULT setPclStateTbl(INT16 topLevel,SCRP_LEVELTBL *aLeveltbl,RC_BASEAREATOPLVINFO *baseAreaTopLvInf,RC_DIVAREA_QUEUE *divQueue,UINT16 topDivVol);

static INT16 getTopAreaClosestCornerSft(INT16 topLevel, SCRP_PCLRECT *baseAreaRect, SCRP_PCLRECT *pclRect, E_RC_DIVAREA_NBRINFO *nBorInf,
		E_RC_AREATYPE areaType);
static E_SC_RESULT makeDivAreaHorizontal(INT16 topLevel, RC_AREASTATEINFO *beforeDivArea, INT32 alterPosX,
		RC_BASEAREATOPLVINFO *baseAreaTopLvInf, E_RC_DIVAREA_NBRINFO *oNBorInf, E_RC_DIVAREA_NBRINFO *dNBorInf, RC_AREASTATEINFO *area1,
		RC_AREASTATEINFO *area2);
static E_SC_RESULT makeDivAreaVertical(INT16 topLevel, RC_AREASTATEINFO *beforeDivArea, INT32 alterPosY,
		RC_BASEAREATOPLVINFO *baseAreaTopLvInf, E_RC_DIVAREA_NBRINFO *oNBorInf, E_RC_DIVAREA_NBRINFO *dNBorInf, RC_AREASTATEINFO *area1,
		RC_AREASTATEINFO *area2);
static Bool baseAreaContainCheck(SCRP_PCLRECT *pclRect, SCRP_PCLRECT *baseAreaTopLvPclRect, INT16 topLevel);
static INT16 baseAreaContainCountHorizontal(INT16 topLevel, E_RC_DIVAREA_NBRINFO *nBorInf, SCRP_PCLRECT *pclRect,
		SCRP_LEVELAREA *baseAreaLv2);
static INT16 baseAreaContainCountVertical(INT16 topLevel, E_RC_DIVAREA_NBRINFO *nBorInf, SCRP_PCLRECT *pclRect, SCRP_LEVELAREA *baseAreaLv2);
static E_SC_RESULT decideDivAreaYSize(INT16 oBaseY, INT16 dBaseY, UINT32 startParcelDensity, UINT32 goalParcelDensity, UINT16 ySize,
		E_RC_AREATYPE areaType, INT16 *y_1, INT16 *y_2);
static E_SC_RESULT decideDivAreaXSize(INT16 oBaseX, INT16 dBaseX, UINT32 startParcelDensity, UINT32 goalParcelDensity, UINT16 xSize,
		E_RC_AREATYPE areaType, INT16 *x_1, INT16 *x_2);
static E_SC_RESULT setDivJoinflg(RC_AREASTATEINFO *afterDivArea, INT16 *joinX, INT16 *joinY);
static E_SC_RESULT setDivSplitflg(INT16 joinLeft, INT16 joinRight, INT16 joinTop, INT16 joinBottom, INT16 *splitX, INT16 *splitY);
static E_SC_RESULT setAreaPCLStateLink(SCRP_PCLRECT *pclRect, SCRP_AREAPCLSTATE* pclStatePt);
static E_SC_RESULT setAreaPCLStateJoin(INT16 targetLevel, INT16 topLevel, SCRP_PCLRECT *pclRect, INT16 joinX, INT16 joinY,
		SCRP_AREAPCLSTATE *pclState);
static E_SC_RESULT setAreaPCLStateSplit(INT32 ALT_X, INT32 ALT_Y, INT32 x_size, INT32 y_size, T_DHC_DOWNLOAD_AREA* downloadarea_ptr,
		T_DHC_ROAD_DENSITY* density_area_ptr, INT32 x_range, INT32 y_range, INT32 L_x, INT32 L_y, INT32 H_x, INT32 H_y,
		SCRP_AREAPCLSTATE* T_ParcelList_ptr);
static E_SC_RESULT checkAreaDownload(T_DHC_DOWNLOAD_AREA* downloadarea_ptr, T_DHC_ROAD_DENSITY* density_area_ptr, INT32 x_range,
		INT32 y_range, INT32 L_x, INT32 L_y, INT32 H_x, INT32 H_y, INT32 x_pos, INT32 y_pos);
static INT16 areaEnqueue(RC_DIVAREA_QUEUE*divQueue, INT16 procNo, SCRP_PCLRECT pclRect, INT32 altX, INT32 altY, INT16 joinX, INT16 joinY,
		INT16 splitX, INT16 splitY);
static INT16 areaDequeue(RC_DIVAREA_QUEUE*divQueue, INT16 *procNo, SCRP_PCLRECT *pclRect, INT32 *altX, INT32 *altY, INT16 *joinX,
		INT16 *joinY, INT16 *splitX, INT16 *splitY);
static E_SC_RESULT makeAreaStackQueue(RC_DIVAREA_QUEUE* aQueue);

/**
 * @brief トップ分割メイン
 * @param トップレベル指定
 * @param [O]レベル管理
 * @param 基準トップエリア情報
 */
E_SC_RESULT RC_AreaDivTop(INT16 topLevel, SCRP_LEVELTBL *aLeveltbl, RC_BASEAREATOPLVINFO *baseAreaTopLvInf) {

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if ( (NULL == aLeveltbl) || (NULL == baseAreaTopLvInf)  ) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[RC_AreaDivTop] Bad param."HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	E_SC_RESULT result = e_SC_RESULT_FAIL;
	INT32 alterPosX = 0;		// Ｘ方向の相対位置
	INT32 alterPosY = 0;		// Ｙ方向の相対位置
	INT16 procNo = 0;			//分割処理終了数

	T_DHC_ROAD_DENSITY densityAreaPtr = {};
	RC_DIVAREA_QUEUE divQueue = {};

	// 出発地から目的地への方向を求める。
	// 出発地・目的地シフト量
	INT16 intResult = SC_MESH_GetAlterPos(baseAreaTopLvInf->startParcelIdTopLv, baseAreaTopLvInf->goalParcelIdTopLv, topLevel, &alterPosX, &alterPosY);
	if (0 != intResult) {
		SC_LOG_ErrorPrint(SC_TAG_RC, " [RC_AreaDivTop]GetAlterPos error. intResult[%d] "HERE, intResult);
		return (e_SC_RESULT_FAIL);
	}
	SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDivTop] alterPosX[%d],alterPosY[%d]"HERE,alterPosX,alterPosY);
	// Queue生成
	result = makeAreaStackQueue(&divQueue);
	if (e_SC_RESULT_SUCCESS != result) {
		SC_LOG_ErrorPrint(SC_TAG_RC, " [RC_AreaDivTop] divmakeQueue error. "HERE);
		return (e_SC_RESULT_FAIL);
	}

	while (0 <= gDivStackIdx) {
		//エリア取り出し～分割要否判定～管理テーブルにセットまで
		//分割するか判定するエリア取り出し
		RC_AREASTATEINFO beforeDivArea = {};
		result = RC_AreaStackPop(&beforeDivArea);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, " [RC_AreaDivTop]areaStackPop error. %x"HERE, result);
			break;
		}

		//パーセル数、リンク数チェック
		UINT32 areaSize = beforeDivArea.pclRect.xSize * beforeDivArea.pclRect.ySize;
		// 密度取得
		result = RC_GetParcelDensity(&densityAreaPtr, beforeDivArea.pclRect.parcelId, beforeDivArea.pclRect.xSize,
				beforeDivArea.pclRect.ySize);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[RC_AreaDivTop]getParcelDensity error. %x"HERE, result);
			break;
		}

		if (RC_DIVPARCELCNT_MAX < areaSize || RC_DIVLINKCNT_MAX < (densityAreaPtr.totalDensity * SC_MA_REAL_DENSITY)) {
			//エリア分割が必要
			//密度解放
			if (NULL != densityAreaPtr.data) {
				RP_MemFree(densityAreaPtr.data, e_MEM_TYPE_ROUTEPLAN);
				densityAreaPtr.data =NULL;
			}
			//エリア分割処理
			result = RC_AreaDivTopSetArea(topLevel,&beforeDivArea,baseAreaTopLvInf,alterPosX,alterPosY);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "[RC_AreaDivTop]RC_AreaDivTopSetArea error. %x"HERE, result);
				break;
			}
		} else {
			//このエリアは分割しなくていい
			//密度解放
			if (NULL != densityAreaPtr.data) {
				RP_MemFree(densityAreaPtr.data, e_MEM_TYPE_ROUTEPLAN);
				densityAreaPtr.data =NULL;
			}
			procNo+= 1;		//分割処理終了数を+1
			//相対位置の取得から管理テーブルに情報をセットするまで
			//管理テーブルにセット
			result = RC_AreaDivTopSetQueue(topLevel,&divQueue,&beforeDivArea, baseAreaTopLvInf,procNo);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "[RC_AreaDivTop]RC_AreaDivTopSetQueue error. %x"HERE, result);
				break;
			}
		}
	}
	if (e_SC_RESULT_SUCCESS != result) {
		// キュー解放処理
		if (NULL != divQueue.divQueuebuf) {
			RP_MemFree(divQueue.divQueuebuf, e_MEM_TYPE_ROUTEPLAN);
			divQueue.divQueuebuf = NULL;
		}
		SC_LOG_ErrorPrint(SC_TAG_RC, " [RC_AreaDivTop]error. %x"HERE, result);
		return (result);
	}

	/*すべての分割エリアを管理テーブルにセットしたので
	 *SCRP_AREAPCLSTATEテーブルに接続フラグと切断有無フラグをセットする
	 */
	UINT16 topDivVol = procNo;	//分割数=分割処理終了数

	result = setPclStateTbl(topLevel,aLeveltbl,baseAreaTopLvInf,&divQueue,topDivVol);
	if (e_SC_RESULT_SUCCESS != result) {
		SC_LOG_ErrorPrint(SC_TAG_RC, " [RC_AreaDivTop]setPclStateTbl error. %x"HERE, result);
		//キューの開放のためリターンはしない
	}
	// TODO キュー解放処理
	if (NULL != divQueue.divQueuebuf) {
		RP_MemFree(divQueue.divQueuebuf, e_MEM_TYPE_ROUTEPLAN);
		divQueue.divQueuebuf = NULL;
	}
	return (result);
}
/**
 * @brief PCLSTATEテーブルおよびレベルテーブルへの探索エリアのセット
 * @param topLevel		 	[I]トップレベル
 * @param aLeveltbl 		[IO]レベルテーブル
 * @param baseAreaTopLvInf 	[I]トップエリアベース情報
 * @param divQueue		 	[I]分割処理終了エリア格納キュー
 * @param topDivVol		[I]トップの合計分割数
 */
static E_SC_RESULT setPclStateTbl(INT16 topLevel,SCRP_LEVELTBL *aLeveltbl,RC_BASEAREATOPLVINFO *baseAreaTopLvInf,RC_DIVAREA_QUEUE *divQueue,UINT16 topDivVol){

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL ==aLeveltbl || NULL == divQueue || NULL ==baseAreaTopLvInf ) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[setPclStateTbl] Bad param."HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (0 >= topDivVol) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[setPclStateTbl] Bad param.topDivVol[%d]"HERE,topDivVol);
		return (e_SC_RESULT_BADPARAM);
	}

	E_SC_RESULT result = e_SC_RESULT_FAIL;
	INT16 intResult = 0;
	INT32 altX = 0;				// 非分割エリア内の相対位置
	INT32 altY = 0;				// 非分割エリア内の相対位置
	INT16 joinX = 0;			//分割管理用接続フラグX
	INT16 joinY = 0;			//分割管理用接続フラグY
	INT16 splitX = 0;			//分割管理用切断フラグX
	INT16 splitY = 0;			//分割管理用切断フラグX
	INT16 deqProcNo = 0;			//取り出し時処理No

	T_DHC_ROAD_DENSITY densityAreaPtr = {};
	T_DHC_DOWNLOAD_AREA* downloadarea_ptr;				//ダウンロードエリア

	//	ダウンロードエリアＩＤの取得
	downloadarea_ptr = RC_GetDownLoad_Area();

	UINT16 divIdx = 0;
	UINT16 divVol = 0;
	UINT16 pclIdx = 0;

	//収納範囲
	//INT32 xRangeMin = 0;
	//INT32 yRangeMin = 0;
	INT32 xRangeMax = 0;
	INT32 yRangeMax = 0;

	SCRP_AREAPCLSTATE *pclStateWk = NULL;
	SCRP_DIVAREA* divAreaWk = NULL;

	do {
		if (RP_LEVEL2 == topLevel) {

			UINT16 lv1oPclVol = baseAreaTopLvInf->oBaseArea.pclRect.xSize * baseAreaTopLvInf->oBaseArea.pclRect.ySize;
			UINT16 lv2maxPclVol = baseAreaTopLvInf->totalPclVolTopLv * 2;	//Lv2すべての枚数の2倍
			UINT16 lv1dPclVol = baseAreaTopLvInf->dBaseArea.pclRect.xSize * baseAreaTopLvInf->dBaseArea.pclRect.ySize;
			UINT16 totalPclVolMax = lv1oPclVol + lv2maxPclVol + lv1dPclVol;

			UINT16 lv1divVol = 2;		//分割数はO基準とD基準の2枚
			UINT16 lv2divVol = topDivVol;	//分割数=分割処理終了数
			UINT16 totalDivVol = lv1divVol + lv2divVol;

			//ボリュームを0に初期化
			aLeveltbl->divInfoVol = 0;
			aLeveltbl->pclStateVol = 0;

			// パーセル状態領域確保
			aLeveltbl->pclState = RP_MemAlloc(sizeof(SCRP_AREAPCLSTATE) * totalPclVolMax, e_MEM_TYPE_ROUTEPLAN);
			if (NULL == aLeveltbl->pclState) {
				SC_LOG_ErrorPrint(SC_TAG_RC, " [setPclStateTbl]MALLOC error. "HERE);
				result = e_SC_RESULT_MALLOC_ERR;
				break;
			}
			RP_Memset0(aLeveltbl->pclState, sizeof(SCRP_AREAPCLSTATE) * totalPclVolMax);
			//アドレスコピー
			pclStateWk = aLeveltbl->pclState;

			// 分割データ格納領域確保
			aLeveltbl->divInfo = RP_MemAlloc(sizeof(SCRP_DIVAREA) * totalDivVol, e_MEM_TYPE_ROUTEPLAN);
			if (NULL == aLeveltbl->divInfo) {
				SC_LOG_ErrorPrint(SC_TAG_RC, " [setPclStateTbl]MALLOC error. "HERE);
				result = e_SC_RESULT_MALLOC_ERR;
				break;
			}
			RP_Memset0(aLeveltbl->divInfo, sizeof(SCRP_DIVAREA) * totalDivVol);
			//アドレスコピー
			divAreaWk = aLeveltbl->divInfo;

			/* レベル１エリア処理開始 */
			aLeveltbl->areaTable[0].pclRect.parcelId = baseAreaTopLvInf->oBaseArea.pclRect.parcelId;
			aLeveltbl->areaTable[0].pclRect.xSize = baseAreaTopLvInf->oBaseArea.pclRect.xSize;
			aLeveltbl->areaTable[0].pclRect.ySize = baseAreaTopLvInf->oBaseArea.pclRect.ySize;
			aLeveltbl->areaTable[0].divIdx = 0;
			aLeveltbl->areaTable[0].divVol = 1;

			//コピーしたアドレスにセットする
			divAreaWk->pclRect.parcelId = baseAreaTopLvInf->oBaseArea.pclRect.parcelId;
			divAreaWk->pclRect.xSize = baseAreaTopLvInf->oBaseArea.pclRect.xSize;
			divAreaWk->pclRect.ySize = baseAreaTopLvInf->oBaseArea.pclRect.ySize;
			divAreaWk->pclIdx = pclIdx;
			divAreaWk->pclVol = lv1oPclVol;

			divVol = 1;
			divIdx = 1;

			//Lv1_O側リンクセット
			result = setAreaPCLStateLink(&baseAreaTopLvInf->oBaseArea.pclRect, pclStateWk);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "[setPclStateTbl] setAreaPCLStateLink error. %x"HERE, result);
				break;
			}
			//Lv1_O側接続フラグは0,0を渡す
			result = setAreaPCLStateJoin(RP_LEVEL1, topLevel, &baseAreaTopLvInf->oBaseArea.pclRect, 0, 0, pclStateWk);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, " [setPCLSTATETBL]setAreaPCLStateJoin error. %x"HERE, result);
				break;
			}
			pclIdx = lv1oPclVol;
			aLeveltbl->divInfoVol = 1;
			aLeveltbl->pclStateVol = lv1oPclVol;

			//収納範囲を更新
			xRangeMax = baseAreaTopLvInf->oBaseArea.pclRect.xSize;
			yRangeMax = baseAreaTopLvInf->oBaseArea.pclRect.ySize;
			//Lv1_O側ダウンロードフラグ
			result = RC_GetParcelDensity(&densityAreaPtr, baseAreaTopLvInf->oBaseArea.pclRect.parcelId, baseAreaTopLvInf->oBaseArea.pclRect.xSize,
					baseAreaTopLvInf->oBaseArea.pclRect.ySize);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "[setPclStateTbl]getParcelDensity error. %x"HERE, result);
				break;
			}
			result = setAreaPCLStateSplit(0, 0, xRangeMax, yRangeMax, downloadarea_ptr, &densityAreaPtr,
					xRangeMax, yRangeMax, 0, 0, xRangeMax - 1,yRangeMax - 1,
					pclStateWk);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, " [setPclStateTbl]RC_SetAreaPCLStateSplit error. %x"HERE, result);
				break;
			}
			// 密度を解放
			if (NULL != densityAreaPtr.data) {
				RP_MemFree(densityAreaPtr.data, e_MEM_TYPE_ROUTEPLAN);
				densityAreaPtr.data =NULL;
			}

			//oBaseの枚数だけポインタを進める
			pclStateWk += lv1oPclVol;
			divAreaWk += 1;

			/* レベル２分割エリア処理開始 */
			aLeveltbl->areaTable[1].pclRect.parcelId = baseAreaTopLvInf->areaTopLv.pclRect.parcelId;
			aLeveltbl->areaTable[1].pclRect.xSize = baseAreaTopLvInf->areaTopLv.pclRect.xSize;
			aLeveltbl->areaTable[1].pclRect.ySize = baseAreaTopLvInf->areaTopLv.pclRect.ySize;
			aLeveltbl->areaTable[1].divVol = lv2divVol;
			aLeveltbl->areaTable[1].divIdx = aLeveltbl->areaTable[0].divVol;

			result = RC_GetParcelDensity(&densityAreaPtr,  baseAreaTopLvInf->areaTopLv.pclRect.parcelId,  baseAreaTopLvInf->areaTopLv.pclRect.xSize,
					baseAreaTopLvInf->areaTopLv.pclRect.ySize);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "[setPclStateTbl]getParcelDensity error. %x"HERE, result);
				break;
			}

			UINT16 i = 0;
			for (i = 0; i < lv2divVol; i++) {
				// 処理対象の矩形情報をキューから取り出す
				intResult = areaDequeue(divQueue, &deqProcNo, &divAreaWk->pclRect, &altX, &altY, &joinX, &joinY, &splitX, &splitY);
				if (i != (deqProcNo - 1) || 0 != intResult) {
					SC_LOG_ErrorPrint(SC_TAG_RC, " [setPclStateTbl] areaDequeue error. i[%d]deqProcNo[%d]intResult[%d]"HERE,i,deqProcNo,intResult);
					result = e_SC_RESULT_FAIL;
					break;
				}
#if 0
				//ダンプ確認
				SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDivTop]deqProcNo	[%4d]", deqProcNo);
				SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDivTop]parcelId	[0x%08x]",divAreaWk->pclRect.parcelId);
				SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDivTop]xSize	[%4d]", divAreaWk->pclRect.xSize);
				SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDivTop]ySize	[%4d]", divAreaWk->pclRect.ySize);
				SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDivTop]altX	[%4d]", altX);
				SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDivTop]altY	[%4d]", altY);
				SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDivTop]joinX	[%4d]", joinX);
				SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDivTop]joinY	[%4d]", joinY);
#endif
				divAreaWk->pclIdx = pclIdx;
				divAreaWk->pclVol = divAreaWk->pclRect.xSize * divAreaWk->pclRect.ySize;

				//Lv2_分割エリアリンクセット
				result = setAreaPCLStateLink(&divAreaWk->pclRect, pclStateWk);
				if (e_SC_RESULT_SUCCESS != result) {
					SC_LOG_ErrorPrint(SC_TAG_RC, "[setPclStateTbl] setAreaPCLStateLink error. %x"HERE, result);
					break;
				}
				// Joinフラグ設定
				result = setAreaPCLStateJoin(RP_LEVEL2, topLevel, &divAreaWk->pclRect, joinX, joinY, pclStateWk);
				if (e_SC_RESULT_SUCCESS != result) {
					SC_LOG_ErrorPrint(SC_TAG_RC, " [setPclStateTbl]setAreaPCLStateJoin error. %x"HERE, result);
					break;
				}

				// Lv2_ダウンロードフラグ設定
				xRangeMax = divAreaWk->pclRect.xSize;
				yRangeMax = divAreaWk->pclRect.ySize;

				result = setAreaPCLStateSplit(altX, altY, xRangeMax, yRangeMax, downloadarea_ptr, &densityAreaPtr,
						baseAreaTopLvInf->areaTopLv.pclRect.xSize, baseAreaTopLvInf->areaTopLv.pclRect.ySize, altX, altY,
						altX + xRangeMax - 1, altY + yRangeMax - 1, pclStateWk);
				if (e_SC_RESULT_SUCCESS != result) {
					SC_LOG_ErrorPrint(SC_TAG_RC, " [setPclStateTbl]RC_SetAreaPCLStateSplit error. %x"HERE, result);
					break;
				}
				aLeveltbl->divInfoVol += 1;
				aLeveltbl->pclStateVol += divAreaWk->pclVol;

				divIdx += 1;
				pclIdx += divAreaWk->pclVol;
				//Lv2分割一枚の枚数だけポインタを進める
				pclStateWk += divAreaWk->pclVol;
				divAreaWk += 1;
			}			//Lv2処理終了
			//密度を解放
			if (NULL != densityAreaPtr.data) {
				RP_MemFree(densityAreaPtr.data, e_MEM_TYPE_ROUTEPLAN);
				densityAreaPtr.data = NULL;
			}

			if (e_SC_RESULT_SUCCESS != result) {
				break;
			}
			//Lv1_D情報セット
			aLeveltbl->areaTable[2].pclRect.parcelId = baseAreaTopLvInf->dBaseArea.pclRect.parcelId;
			aLeveltbl->areaTable[2].pclRect.xSize = baseAreaTopLvInf->dBaseArea.pclRect.xSize;
			aLeveltbl->areaTable[2].pclRect.ySize = baseAreaTopLvInf->dBaseArea.pclRect.ySize;
			aLeveltbl->areaTable[2].divIdx = aLeveltbl->areaTable[1].divIdx + aLeveltbl->areaTable[1].divVol;
			aLeveltbl->areaTable[2].divVol = 1;

			//コピーしたアドレスにセットする
			divAreaWk->pclRect.parcelId = baseAreaTopLvInf->dBaseArea.pclRect.parcelId;
			divAreaWk->pclRect.xSize = baseAreaTopLvInf->dBaseArea.pclRect.xSize;
			divAreaWk->pclRect.ySize = baseAreaTopLvInf->dBaseArea.pclRect.ySize;
			divAreaWk->pclIdx = pclIdx;
			divAreaWk->pclVol = lv1dPclVol;
			aLeveltbl->divInfoVol += 1;
			aLeveltbl->pclStateVol += lv1dPclVol;

			//Lv1_D側リンクセット
			result = setAreaPCLStateLink(&baseAreaTopLvInf->dBaseArea.pclRect, pclStateWk);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "[setPclStateTbl] setAreaPCLStateLink error. %x"HERE, result);
				break;
			}
			//Lv1_D側接続フラグは0,0を渡す
			result = setAreaPCLStateJoin(RP_LEVEL1, topLevel, &baseAreaTopLvInf->dBaseArea.pclRect, 0, 0, pclStateWk);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, " [setPclStateTbl]setAreaPCLStateJoin error. %x"HERE, result);
				break;
			}
			//収納範囲を更新
			xRangeMax = baseAreaTopLvInf->dBaseArea.pclRect.xSize;
			yRangeMax = baseAreaTopLvInf->dBaseArea.pclRect.ySize;
			//Lv1_D側ダウンロードフラグ
			result = RC_GetParcelDensity(&densityAreaPtr, baseAreaTopLvInf->dBaseArea.pclRect.parcelId, baseAreaTopLvInf->dBaseArea.pclRect.xSize,
					baseAreaTopLvInf->dBaseArea.pclRect.ySize);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "[setPclStateTbl]getParcelDensity error. %x"HERE, result);
				break;
			}
			result = setAreaPCLStateSplit(0, 0, xRangeMax, yRangeMax, downloadarea_ptr, &densityAreaPtr, xRangeMax, yRangeMax, 0, 0,
					xRangeMax - 1, yRangeMax - 1, pclStateWk);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, " [setPclStateTbl]RC_SetAreaPCLStateSplit error. %x"HERE, result);
				break;
			}
			// 密度を解放
			if (NULL != densityAreaPtr.data) {
				RP_MemFree(densityAreaPtr.data, e_MEM_TYPE_ROUTEPLAN);
				densityAreaPtr.data = NULL;
			}
			//トップレベルも含め処理終了のため
			aLeveltbl->topLevel = topLevel;

		}else if(RP_LEVEL1 == topLevel) {
			//レベル1
			//領域確保のため上限を定める
			UINT16 totalPclVolMax = baseAreaTopLvInf->totalPclVolTopLv * 2;	//Lv1すべての枚数の2倍
			//UINT16 divVol = topDivVol;	//分割数=分割処理終了数
			//UINT16 pclVol = baseAreaTopLvInf->areaTopLv.pclRect.xSize* baseAreaTopLvInf->areaTopLv.pclRect.ySize;

			//ボリュームを0に初期化
			aLeveltbl->divInfoVol = 0;
			aLeveltbl->pclStateVol = 0;

			// パーセル状態領域確保
			aLeveltbl->pclState = RP_MemAlloc(sizeof(SCRP_AREAPCLSTATE) * totalPclVolMax, e_MEM_TYPE_ROUTEPLAN);
			if (NULL == aLeveltbl->pclState) {
				SC_LOG_ErrorPrint(SC_TAG_RC, " [setPclStateTbl]MALLOC error. "HERE);
				result = e_SC_RESULT_MALLOC_ERR;
				break;
			}
			RP_Memset0(aLeveltbl->pclState, sizeof(SCRP_AREAPCLSTATE) * totalPclVolMax);
			//アドレスコピー
			pclStateWk = aLeveltbl->pclState;

			// 分割データ格納領域確保
			aLeveltbl->divInfo = RP_MemAlloc(sizeof(SCRP_DIVAREA) * topDivVol, e_MEM_TYPE_ROUTEPLAN);
			if (NULL == aLeveltbl->divInfo) {
				SC_LOG_ErrorPrint(SC_TAG_RC, " [setPclStateTbl]MALLOC error. "HERE);
				result = e_SC_RESULT_MALLOC_ERR;
				break;
			}
			RP_Memset0(aLeveltbl->divInfo, sizeof(SCRP_DIVAREA) * topDivVol);
			//アドレスコピー
			divAreaWk = aLeveltbl->divInfo;

			/* レベル１エリア処理開始 */
			aLeveltbl->areaTable[0].pclRect.parcelId = baseAreaTopLvInf->areaTopLv.pclRect.parcelId;
			aLeveltbl->areaTable[0].pclRect.xSize = baseAreaTopLvInf->areaTopLv.pclRect.xSize;
			aLeveltbl->areaTable[0].pclRect.ySize = baseAreaTopLvInf->areaTopLv.pclRect.ySize;
			aLeveltbl->areaTable[0].divIdx = 0;
			aLeveltbl->areaTable[0].divVol = topDivVol;


			//密度取得
			result = RC_GetParcelDensity(&densityAreaPtr,  baseAreaTopLvInf->areaTopLv.pclRect.parcelId,  baseAreaTopLvInf->areaTopLv.pclRect.xSize,
					baseAreaTopLvInf->areaTopLv.pclRect.ySize);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "[setPclStateTbl]getParcelDensity error. %x"HERE, result);
				break;
			}

			UINT16 i = 0;
			for (i = 0; i < topDivVol; i++) {

				// 処理対象の矩形情報をキューから取り出す
				intResult = areaDequeue(divQueue, &deqProcNo, &divAreaWk->pclRect, &altX, &altY, &joinX, &joinY, &splitX, &splitY);
				if (i != (deqProcNo - 1) || 0 != intResult) {
					SC_LOG_ErrorPrint(SC_TAG_RC, " [setPclStateTbl]areaDequeue error. "HERE);
					result = e_SC_RESULT_FAIL;
					break;
				}
#if 0
				//ダンプ確認
				SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDivTop]deqProcNo	[%4d]", deqProcNo);
				SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDivTop]parcelId	[0x%08x]",divAreaWk->pclRect.parcelId);
				SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDivTop]xSize	[%4d]", divAreaWk->pclRect.xSize);
				SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDivTop]ySize	[%4d]", divAreaWk->pclRect.ySize);
				SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDivTop]altX	[%4d]", altX);
				SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDivTop]altY	[%4d]", altY);
				SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDivTop]joinX	[%4d]", joinX);
				SC_LOG_InfoPrint(SC_TAG_RC, "[RC_AreaDivTop]joinY	[%4d]", joinY);
#endif
				divAreaWk->pclIdx = pclIdx;
				divAreaWk->pclVol = divAreaWk->pclRect.xSize * divAreaWk->pclRect.ySize;

				//Lv1分割エリアリンクセット
				result = setAreaPCLStateLink(&divAreaWk->pclRect, pclStateWk);
				if (e_SC_RESULT_SUCCESS != result) {
					SC_LOG_ErrorPrint(SC_TAG_RC, "[setPclStateTbl] setAreaPCLStateLink error. %x"HERE, result);
					break;
				}

				// Joinフラグ設定
				result = setAreaPCLStateJoin(RP_LEVEL1, topLevel, &divAreaWk->pclRect, joinX, joinY, pclStateWk);
				if (e_SC_RESULT_SUCCESS != result) {
					SC_LOG_ErrorPrint(SC_TAG_RC, " [setPclStateTbl]setAreaPCLStateJoin error. %x"HERE, result);
					break;
				}
				// Lv1_ダウンロードフラグ設定
				xRangeMax = divAreaWk->pclRect.xSize;
				yRangeMax = divAreaWk->pclRect.ySize;
				result = setAreaPCLStateSplit(altX, altY, xRangeMax, yRangeMax, downloadarea_ptr, &densityAreaPtr,
						baseAreaTopLvInf->areaTopLv.pclRect.xSize, baseAreaTopLvInf->areaTopLv.pclRect.ySize, altX, altY,
						altX + xRangeMax - 1, altY + yRangeMax - 1, pclStateWk);
				if (e_SC_RESULT_SUCCESS != result) {
					SC_LOG_ErrorPrint(SC_TAG_RC, " [setPclStateTbl]RC_SetAreaPCLStateSplit error. %x"HERE, result);
					break;
				}
				aLeveltbl->divInfoVol += 1;
				aLeveltbl->pclStateVol += divAreaWk->pclVol;

				divIdx += 1;
				pclIdx += divAreaWk->pclVol;
				//Lv1分割一枚の枚数だけポインタを進める
				pclStateWk += divAreaWk->pclVol;
				divAreaWk += 1;
			}
			//密度を解放
			if (NULL != densityAreaPtr.data) {
				RP_MemFree(densityAreaPtr.data, e_MEM_TYPE_ROUTEPLAN);
				densityAreaPtr.data = NULL;
			}
			//トップレベルは1のまま処理終了のため
			aLeveltbl->topLevel = topLevel;
		}else{
			//レベル1,2以外は想定外
			result = e_SC_RESULT_FAIL;
			break;
		}
	} while (0);
	if (e_SC_RESULT_SUCCESS != result) {
		if (NULL != aLeveltbl->pclState) {
			RP_MemFree(aLeveltbl->pclState, e_MEM_TYPE_ROUTEPLAN);
			SC_LOG_ErrorPrint(SC_TAG_RC, " [setPclStateTbl]aLeveltbl->pclState free. "HERE);
			aLeveltbl->pclState = NULL;
		}
		if (NULL != aLeveltbl->divInfo) {
			RP_MemFree(aLeveltbl->divInfo, e_MEM_TYPE_ROUTEPLAN);
			SC_LOG_ErrorPrint(SC_TAG_RC, " [setPclStateTbl]aLeveltbl->divInfo free. "HERE);
			aLeveltbl->divInfo = NULL;
		}
		// 密度を解放
		if (NULL != densityAreaPtr.data) {
			RP_MemFree(densityAreaPtr.data, e_MEM_TYPE_ROUTEPLAN);
			SC_LOG_ErrorPrint(SC_TAG_RC, " [setPclStateTbl]densityAreaPtr.data free. "HERE);
			densityAreaPtr.data = NULL;
		}
		SC_LOG_ErrorPrint(SC_TAG_RC, " [setPclStateTbl]error.[%x] topLevel[%d]"HERE,result, topLevel);
	}
	return (result);
}

/**
 * @brief 要分割エリアの分割方向の決定～分割後のエリアスタックプッシュまで
 * @param topLevel		 	[I]トップレベル
 * @param beforeDivArea		[I]分割前エリア
 * @param baseAreaTopLvInf 	[I]トップエリア情報
 * @param alterPosX		 	[I]出発地目的地横シフト量
 * @param alterPosY			[I]出発地目的地縦シフト量
 */
static E_SC_RESULT RC_AreaDivTopSetArea(INT16 topLevel, RC_AREASTATEINFO *beforeDivArea, RC_BASEAREATOPLVINFO *baseAreaTopLvInf, INT32 alterPosX,
		INT32 alterPosY) {

	if (NULL == beforeDivArea || NULL ==baseAreaTopLvInf ) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[RC_AreaDivTopSetArea] Bad param."HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	E_SC_RESULT result = e_SC_RESULT_FAIL;
	//INT16 intResult = 0;
	INT16 o_ret = 0;			//
	INT16 d_ret = 0;			//
	//INT32 altX = 0;				// 非分割エリア内の相対位置
	//INT32 altY = 0;				// 非分割エリア内の相対位置
	INT32 absAlterPosX = 0;		// Ｘ方向の相対位置（abs）
	INT32 absAlterPosY = 0;		// Ｙ方向の相対位置（abs）

	// abs変換
	absAlterPosX = abs(alterPosX);
	absAlterPosY = abs(alterPosY);

	//エリア最寄情報初期化
	E_RC_DIVAREA_NBRINFO oNBorInf = { e_RC_CORNER_NEIGHBORFLG_DEFAULT, e_RC_CORNER_NEIGHBORFLG_DEFAULT, 0, 0 };
	E_RC_DIVAREA_NBRINFO dNBorInf = { e_RC_CORNER_NEIGHBORFLG_DEFAULT, e_RC_CORNER_NEIGHBORFLG_DEFAULT, 0, 0 };

	//チェック関数判定用
	Bool checkret = false;
	//エリア内でどの四隅に近いか判定する
	if (e_AREA_TYPE_OD == beforeDivArea->areaType || e_AREA_TYPE_O == beforeDivArea->areaType) {
		//O側
		o_ret = getTopAreaClosestCornerSft(topLevel, &baseAreaTopLvInf->oBaseAreaTopLv.pclRect, &beforeDivArea->pclRect, &oNBorInf,
				e_AREA_TYPE_O);
		if (0 != o_ret) {
			SC_LOG_ErrorPrint(SC_TAG_RC, " [RC_AreaDivTopSetArea]getTopAreaClosestCornerSft error. "HERE);
			return (e_SC_RESULT_FAIL);
		}
		//O側基準エリアを包含できているかチェック
		checkret = baseAreaContainCheck(&beforeDivArea->pclRect, &baseAreaTopLvInf->oBaseAreaTopLv.pclRect, topLevel);
		if (true != checkret) {
			SC_LOG_ErrorPrint(SC_TAG_RC, " [RC_AreaDivTopSetArea]baseAreaContain false. "HERE);
			return (e_SC_RESULT_FAIL);
		}
	}
	if (e_AREA_TYPE_OD == beforeDivArea->areaType || e_AREA_TYPE_D == beforeDivArea->areaType) {
		//D側
		d_ret = getTopAreaClosestCornerSft(topLevel, &baseAreaTopLvInf->dBaseAreaTopLv.pclRect, &beforeDivArea->pclRect, &dNBorInf,
				e_AREA_TYPE_D);
		if (0 != d_ret) {
			SC_LOG_ErrorPrint(SC_TAG_RC, " [RC_AreaDivTopSetArea]getTopAreaClosestCornerSft error. "HERE);
			return (e_SC_RESULT_FAIL);
		}
		//D側基準エリアを包含できているかチェック
		checkret = baseAreaContainCheck(&beforeDivArea->pclRect, &baseAreaTopLvInf->dBaseAreaTopLv.pclRect, topLevel);
		if (true != checkret) {
			SC_LOG_ErrorPrint(SC_TAG_RC, " [RC_AreaDivTopSetArea]baseAreaContain false. "HERE);
			return (e_SC_RESULT_FAIL);
		}
	}

	//出力用
	RC_AREASTATEINFO area1 = {};
	RC_AREASTATEINFO area2 = {};

	if (e_AREA_TYPE_OD == beforeDivArea->areaType) {
		//初回のみ、出発地と目的地間のシフト量で切る方向を決める
		if (1 >= absAlterPosX && 1 >= absAlterPosY) {
			//2点は隣のエリアなので切れない
			SC_LOG_ErrorPrint(SC_TAG_RC, "[RC_AreaDivTopSetArea] alterPosX[%d],alterPosY[%d],absAlterPosX[%d],absAlterPosY[%d]"HERE,alterPosX,alterPosY,absAlterPosX,absAlterPosY);
			return (e_SC_RESULT_FAIL);
		}

		if (absAlterPosX <= absAlterPosY) {
			//行切り
			result = makeDivAreaHorizontal(topLevel, beforeDivArea, alterPosX, baseAreaTopLvInf, &oNBorInf, &dNBorInf, &area1, &area2);
		} else {
			//列切り
			result = makeDivAreaVertical(topLevel, beforeDivArea, alterPosY, baseAreaTopLvInf, &oNBorInf, &dNBorInf, &area1, &area2);
		}
	} else {
		//2回目以降は、枚数の比較のみで決める
		if (beforeDivArea->pclRect.xSize <= beforeDivArea->pclRect.ySize) {
			//行切り
			result = makeDivAreaHorizontal(topLevel, beforeDivArea, alterPosX, baseAreaTopLvInf, &oNBorInf, &dNBorInf, &area1, &area2);
		} else {
			//列切り
			result = makeDivAreaVertical(topLevel, beforeDivArea, alterPosY, baseAreaTopLvInf, &oNBorInf, &dNBorInf, &area1, &area2);
		}
	}
	// 処理結果判定
	if (e_SC_RESULT_SUCCESS != result) {
		SC_LOG_ErrorPrint(SC_TAG_RC, " [RC_AreaDivTopSetArea]DivArea error. %x"HERE, result);
		return (result);
	}
	//エリアプッシュ
	//エリア2を先にプッシュ
	result = RC_AreaStackPush(&area2);
	// 処理結果判定
	if (e_SC_RESULT_SUCCESS != result) {
		SC_LOG_ErrorPrint(SC_TAG_RC, " [RC_AreaDivTopSetArea]areaStackPush error. %x"HERE, result);
		return (result);
	}
	result = RC_AreaStackPush(&area1);
	// 処理結果判定
	if (e_SC_RESULT_SUCCESS != result) {
		SC_LOG_ErrorPrint(SC_TAG_RC, " [RC_AreaDivTopSetArea]areaStackPush error. %x"HERE, result);
		return (result);
	}
	return (result);
}

/**
 * @brief 分割終了エリアの相対位置・接続・切断フラグのセット
 * @param topLevel		 	[I]トップレベル
 * @param divQueue			[IO]分割エリア管理キュー
 * @param beforeDivArea 	[I]分割完了した・または分割不要のエリア
 * @param baseAreaTopLvInf	[I]トップレベルベース情報
 * @param procNo			[I]処理番号
 */
static E_SC_RESULT RC_AreaDivTopSetQueue(INT16 topLevel,RC_DIVAREA_QUEUE *divQueue,RC_AREASTATEINFO *beforeDivArea,RC_BASEAREATOPLVINFO *baseAreaTopLvInf,INT16 procNo){

	if (NULL == divQueue || NULL == beforeDivArea || NULL ==baseAreaTopLvInf ) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[RC_AreaDivTopSetQueue] Bad param."HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (0 > procNo  ) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[RC_AreaDivTopSetQueue] Bad param.procNo[%d]"HERE,procNo);
		return (e_SC_RESULT_BADPARAM);
	}
	E_SC_RESULT result = e_SC_RESULT_FAIL;
	INT32 intResult = 0;
	INT32 altX = 0;
	INT32 altY = 0;
	INT16 joinX = 0;			//分割管理用接続フラグX
	INT16 joinY = 0;			//分割管理用接続フラグY
	INT16 splitX = 0;			//分割管理用切断フラグX
	INT16 splitY = 0;			//分割管理用切断フラグX

	RC_AREASTATEINFO afterDivArea = {};
	RP_Memcpy(&afterDivArea, beforeDivArea, sizeof(RC_AREASTATEINFO));

	// 非分割エリア内相対位置の取得
	if (RP_LEVEL2 == topLevel ||RP_LEVEL1 == topLevel ) {
		//トップ内の相対位置を求める
		intResult = SC_MESH_GetAlterPos(baseAreaTopLvInf->areaTopLv.pclRect.parcelId, afterDivArea.pclRect.parcelId, topLevel, &altX,
				&altY);
		if (0 != intResult) {
			SC_LOG_ErrorPrint(SC_TAG_RC, " [RC_AreaDivTopSetQueue]GetAlterPos error. "HERE);
			return (e_SC_RESULT_FAIL);
		}
	} else {
		//レベル3以上はまだ想定していない
		SC_LOG_ErrorPrint(SC_TAG_RC, "[RC_AreaDivTopSetQueue] error.topLevel [%d]"HERE, topLevel);
		return (e_SC_RESULT_FAIL);
	}

	//管理テーブル用接続フラグのセット
	result = setDivJoinflg(&afterDivArea, &joinX, &joinY);
	if (e_SC_RESULT_SUCCESS != result) {
		SC_LOG_ErrorPrint(SC_TAG_RC, " [RC_AreaDivTopSetQueue]setDivJoinflg error. %x"HERE, result);
		return (result);
	}
#if 0
	//管理テーブル用切断フラグのセット
	//切断フラグは旧処理使用のためコメントアウト
	result = setDivSplitflg(afterDivArea.joinLeft, afterDivArea.joinRight, afterDivArea.joinTop, afterDivArea.joinBottom, &splitX,
			&splitY);
	if (e_SC_RESULT_SUCCESS != result) {
		SC_LOG_ErrorPrint(SC_TAG_RC, " [RC_AreaDivTopSetQueue]setDivSplitflg error. %x"HERE, result);
		return (result);
	}
#endif
	intResult = areaEnqueue(divQueue,procNo, afterDivArea.pclRect, altX, altY, joinX, joinY, splitX, splitY);
	if (0 != intResult) {
		SC_LOG_ErrorPrint(SC_TAG_RC, " [RC_AreaDivTopSetQueue]areaEnqueue error. %d"HERE, intResult);
		result = e_SC_RESULT_FAIL;
	}
	return (result);
}


/**
 * @brief 行切り処理
 * @param beforeDivArea 	[I]分割対象エリア
 * @param alterPosX 		[I]出発地目的地横シフト量
 * @param BaseAreaTopLv 	[I]基準エリアトップレベル
 * @param ParcelDensity 	[I]対象パーセルトップ一枚密度
 * @param nBorInf			[I]四隅最寄パーセルまでの情報
 * @param area1 			[O]エリア分割結果1
 * @param area2 			[O]エリア分割結果2
 * @note 基準エリアを含むような枚数を取得し
 * 		 枚数を調整したあと、上下に分ける
 */
static E_SC_RESULT makeDivAreaHorizontal(INT16 topLevel, RC_AREASTATEINFO *beforeDivArea, INT32 alterPosX,
		RC_BASEAREATOPLVINFO *baseAreaTopLvInf, E_RC_DIVAREA_NBRINFO *oNBorInf, E_RC_DIVAREA_NBRINFO *dNBorInf, RC_AREASTATEINFO *area1,
		RC_AREASTATEINFO *area2) {

	E_SC_RESULT result = e_SC_RESULT_FAIL;

	if (NULL == beforeDivArea || NULL == baseAreaTopLvInf || NULL == oNBorInf || NULL == dNBorInf) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[makeDivAreaHorizontal] Bad param."HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == area1 || NULL == area2) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[makeDivAreaHorizontal] Bad param."HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (RC_DIVPCLRANGECNT_MIN > beforeDivArea->pclRect.xSize || RC_DIVPCLRANGECNT_MIN > beforeDivArea->pclRect.ySize) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[makeDivAreaHorizontal] Bad param."HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	//Y方向包含最低値初期化
	INT16 oContBaseY = 0;
	INT16 dContBaseY = 0;
	//Y枚数
	INT16 y1 = 0;
	INT16 y2 = 0;

	if (e_AREA_TYPE_OD == beforeDivArea->areaType) {
		//初回のみ、2点双方を含む
		if (0 <= alterPosX) {
			//出発地から見て目的地が右
			area1->primaryX = -1;
			area2->primaryX = -1;
		} else {
			//目的地が左
			area1->primaryX = 1;
			area2->primaryX = 1;
		}
		//O側基準エリアを包含するには最低何枚必要か
		oContBaseY = baseAreaContainCountHorizontal(topLevel, oNBorInf, &beforeDivArea->pclRect, &baseAreaTopLvInf->oBaseAreaTopLv);
		//D側基準エリアを包含するには最低何枚必要か
		dContBaseY = baseAreaContainCountHorizontal(topLevel, dNBorInf, &beforeDivArea->pclRect, &baseAreaTopLvInf->dBaseAreaTopLv);

		//枚数を決める
		result = decideDivAreaYSize(oContBaseY, dContBaseY, baseAreaTopLvInf->startParcelDensityTopLv,
				baseAreaTopLvInf->goalParcelDensityTopLv, beforeDivArea->pclRect.ySize, beforeDivArea->areaType, &y1, &y2);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[makeDivAreaHorizontal] decideDivAreaYSize error. %x"HERE, result);
			return (result);
		}

		//出発地最寄フラグ
		if (e_RC_CORNER_NEIGHBORFLG_BOTTOM == oNBorInf->neighborflgY) {
			//下が近い
			if (e_RC_CORNER_NEIGHBORFLG_BOTTOM == dNBorInf->neighborflgY) {
				//エラー
				SC_LOG_ErrorPrint(SC_TAG_RC, "[makeDivAreaHorizontal] error. "HERE);
				return (e_SC_RESULT_FAIL);
			}
			//格納優先度（上下）は下優先
			area1->primaryY = -1;
			area2->primaryY = -1;
			//エリア1の接続フラグ（上）を1
			area1->joinTop = 1;
			area1->joinBottom = beforeDivArea->joinBottom;
			//エリア2の接続フラグ（下）を1
			area2->joinTop = beforeDivArea->joinTop;
			area2->joinBottom = 1;

			//エリア2のラスト分割方向のみ登録
			area1->lastDivSideY = beforeDivArea->lastDivSideY;
			area2->lastDivSideY = e_RC_LASTDIVSIDEFLG_BOTTOM;

			area1->pclRect.parcelId = beforeDivArea->pclRect.parcelId;
			area1->pclRect.xSize = beforeDivArea->pclRect.xSize;
			area1->pclRect.ySize = y1;
			area1->areaType = e_AREA_TYPE_O;

			//エリア2はベースパーセルが移動する
			area2->pclRect.parcelId = SC_MESH_SftParcelId(beforeDivArea->pclRect.parcelId, 0, y1 - 1);
			area2->pclRect.xSize = beforeDivArea->pclRect.xSize;
			area2->pclRect.ySize = y2;
			area2->areaType = e_AREA_TYPE_D;
		} else if (e_RC_CORNER_NEIGHBORFLG_TOP == oNBorInf->neighborflgY) {
			//上が近い
			if (e_RC_CORNER_NEIGHBORFLG_TOP == dNBorInf->neighborflgY) {
				//エラー
				SC_LOG_ErrorPrint(SC_TAG_RC, "[makeDivAreaHorizontal] error. "HERE);
				return (e_SC_RESULT_FAIL);
			}
			//格納優先度（上下）は上優先
			area1->primaryY = 1;
			area2->primaryY = 1;
			//エリア1の接続フラグ（下）を1
			area1->joinTop = beforeDivArea->joinTop;
			area1->joinBottom = 1;
			//エリア2の接続フラグ（上）を1
			area2->joinTop = 1;
			area2->joinBottom = beforeDivArea->joinBottom;

			//エリア2のラスト分割方向のみ登録
			area1->lastDivSideY = beforeDivArea->lastDivSideY;
			area2->lastDivSideY = e_RC_LASTDIVSIDEFLG_TOP;

			//エリア1はベースパーセルが移動する
			area1->pclRect.parcelId = SC_MESH_SftParcelId(beforeDivArea->pclRect.parcelId, 0, y2 - 1);
			area1->pclRect.xSize = beforeDivArea->pclRect.xSize;
			area1->pclRect.ySize = y1;
			area1->areaType = e_AREA_TYPE_O;

			area2->pclRect.parcelId = beforeDivArea->pclRect.parcelId;
			area2->pclRect.xSize = beforeDivArea->pclRect.xSize;
			area2->pclRect.ySize = y2;
			area2->areaType = e_AREA_TYPE_D;
		} else {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[makeDivAreaHorizontal] error. "HERE);
			return (e_SC_RESULT_FAIL);
		}
		result = e_SC_RESULT_SUCCESS;
	} else if (e_AREA_TYPE_O == beforeDivArea->areaType) {

		//O側基準エリアを包含するには最低何枚必要か
		oContBaseY = baseAreaContainCountHorizontal(topLevel, oNBorInf, &beforeDivArea->pclRect, &baseAreaTopLvInf->oBaseAreaTopLv);

		//枚数を決める
		result = decideDivAreaYSize(oContBaseY, dContBaseY, baseAreaTopLvInf->startParcelDensityTopLv,
				baseAreaTopLvInf->goalParcelDensityTopLv, beforeDivArea->pclRect.ySize, beforeDivArea->areaType, &y1, &y2);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[makeDivAreaHorizontal]decideDivAreaYSize error. %x"HERE, result);
			return (result);
		}
		//出発地最寄フラグ
		if (e_RC_CORNER_NEIGHBORFLG_BOTTOM == oNBorInf->neighborflgY) {
			//下が近い
			//格納優先度（上下）は下優先
			area1->primaryY = -1;
			area2->primaryY = -1;
			//エリア1の接続フラグ（上）を1
			area1->joinTop = 1;
			area1->joinBottom = beforeDivArea->joinBottom;
			//エリア2の接続フラグ（下）を1
			area2->joinTop = beforeDivArea->joinTop;
			area2->joinBottom = 1;

			//エリア2のラスト分割方向のみ登録
			area1->lastDivSideY = beforeDivArea->lastDivSideY;
			area2->lastDivSideY = e_RC_LASTDIVSIDEFLG_BOTTOM;

			area1->pclRect.parcelId = beforeDivArea->pclRect.parcelId;
			area1->pclRect.xSize = beforeDivArea->pclRect.xSize;
			area1->pclRect.ySize = y1;
			area1->areaType = e_AREA_TYPE_O;

			//エリア2はベースパーセルが移動する
			area2->pclRect.parcelId = SC_MESH_SftParcelId(beforeDivArea->pclRect.parcelId, 0, y1 - 1);
			area2->pclRect.xSize = beforeDivArea->pclRect.xSize;
			area2->pclRect.ySize = y2;
			area2->areaType = e_AREA_TYPE_OTHER;
		} else if (e_RC_CORNER_NEIGHBORFLG_TOP == oNBorInf->neighborflgY) {
			//上が近い
			//格納優先度（上下）は上優先
			area1->primaryY = 1;
			area2->primaryY = 1;
			//エリア1の接続フラグ（下）を1
			area1->joinTop = beforeDivArea->joinTop;
			area1->joinBottom = 1;

			//エリア2の接続フラグ（上）を1
			area2->joinTop = 1;
			area2->joinBottom = beforeDivArea->joinBottom;

			//エリア2のラスト分割方向のみ登録
			area1->lastDivSideY = beforeDivArea->lastDivSideY;
			area2->lastDivSideY = e_RC_LASTDIVSIDEFLG_TOP;

			//エリア1はベースパーセルが移動する
			area1->pclRect.parcelId = SC_MESH_SftParcelId(beforeDivArea->pclRect.parcelId, 0, y2 - 1);
			area1->pclRect.xSize = beforeDivArea->pclRect.xSize;
			area1->pclRect.ySize = y1;
			area1->areaType = e_AREA_TYPE_O;

			area2->pclRect.parcelId = beforeDivArea->pclRect.parcelId;
			area2->pclRect.xSize = beforeDivArea->pclRect.xSize;
			area2->pclRect.ySize = y2;
			area2->areaType = e_AREA_TYPE_OTHER;
		} else {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[makeDivAreaHorizontal] error. "HERE);
			return (e_SC_RESULT_FAIL);
		}
		//格納優先度（左右）はそのまま
		area1->primaryX = beforeDivArea->primaryX;
		area2->primaryX = beforeDivArea->primaryX;
		result = (e_SC_RESULT_SUCCESS);

	} else if (e_AREA_TYPE_D == beforeDivArea->areaType) {
		//D側基準エリアを包含するには最低何枚必要か
		dContBaseY = baseAreaContainCountHorizontal(topLevel, dNBorInf, &beforeDivArea->pclRect, &baseAreaTopLvInf->dBaseAreaTopLv);

		//枚数を決める
		result = decideDivAreaYSize(oContBaseY, dContBaseY, baseAreaTopLvInf->startParcelDensityTopLv,
				baseAreaTopLvInf->goalParcelDensityTopLv, beforeDivArea->pclRect.ySize, beforeDivArea->areaType, &y1, &y2);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[makeDivAreaHorizontal] decideDivAreaYSize error. %x"HERE, result);
			return (result);
		}
		//目的地最寄までの縦シフト量
		if (e_RC_CORNER_NEIGHBORFLG_BOTTOM == dNBorInf->neighborflgY) {
			//下が近い
			//格納優先度（上下）は上優先
			area1->primaryY = 1;
			area2->primaryY = 1;
			//エリア1の接続フラグ（下）を1
			area1->joinTop = beforeDivArea->joinTop;
			area1->joinBottom = 1;

			//エリア2の接続フラグ（上）を1
			area2->joinTop = 1;
			area2->joinBottom = beforeDivArea->joinBottom;

			//エリア2のラスト分割方向のみ登録
			area1->lastDivSideY = beforeDivArea->lastDivSideY;
			area2->lastDivSideY = e_RC_LASTDIVSIDEFLG_TOP;

			//エリア1はベースパーセルが移動する
			area1->pclRect.parcelId = SC_MESH_SftParcelId(beforeDivArea->pclRect.parcelId, 0, y2 - 1);
			area1->pclRect.xSize = beforeDivArea->pclRect.xSize;
			area1->pclRect.ySize = y1;
			area1->areaType = e_AREA_TYPE_OTHER;

			area2->pclRect.parcelId = beforeDivArea->pclRect.parcelId;
			area2->pclRect.xSize = beforeDivArea->pclRect.xSize;
			area2->pclRect.ySize = y2;
			area2->areaType = e_AREA_TYPE_D;

		} else if (e_RC_CORNER_NEIGHBORFLG_TOP == dNBorInf->neighborflgY) {
			//上が近い
			//格納優先度（上下）は下優先
			area1->primaryY = -1;
			area2->primaryY = -1;
			//エリア1の接続フラグ（上）を1
			area1->joinTop = 1;
			area1->joinBottom = beforeDivArea->joinBottom;

			//エリア2の接続フラグ（下）を1
			area2->joinTop = beforeDivArea->joinTop;
			area2->joinBottom = 1;

			//エリア2のラスト分割方向のみ登録
			area1->lastDivSideY = beforeDivArea->lastDivSideY;
			area2->lastDivSideY = e_RC_LASTDIVSIDEFLG_BOTTOM;

			area1->pclRect.parcelId = beforeDivArea->pclRect.parcelId;
			area1->pclRect.xSize = beforeDivArea->pclRect.xSize;
			area1->pclRect.ySize = y1;
			area1->areaType = e_AREA_TYPE_OTHER;

			//エリア2はベースパーセルが移動する
			area2->pclRect.parcelId = SC_MESH_SftParcelId(beforeDivArea->pclRect.parcelId, 0, y1 - 1);
			area2->pclRect.xSize = beforeDivArea->pclRect.xSize;
			area2->pclRect.ySize = y2;
			area2->areaType = e_AREA_TYPE_D;
		} else {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[makeDivAreaHorizontal] error. "HERE);
			return (e_SC_RESULT_FAIL);
		}
		//格納優先度（左右）はそのまま
		area1->primaryX = beforeDivArea->primaryX;
		area2->primaryX = beforeDivArea->primaryX;
		result = (e_SC_RESULT_SUCCESS);

	} else if (e_AREA_TYPE_OTHER == beforeDivArea->areaType) {
		//枚数を決める
		result = decideDivAreaYSize(oContBaseY, dContBaseY, baseAreaTopLvInf->startParcelDensityTopLv,
				baseAreaTopLvInf->goalParcelDensityTopLv, beforeDivArea->pclRect.ySize, beforeDivArea->areaType, &y1, &y2);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[makeDivAreaHorizontal]decideDivAreaYSize error. %x"HERE, result);
			return (result);
		}
		if (-1 == beforeDivArea->primaryY) {
			//エリア1の接続フラグ（上）を1
			area1->joinTop = 1;
			area1->joinBottom = beforeDivArea->joinBottom;

			//エリア2の接続フラグ（下）を1
			area2->joinTop = beforeDivArea->joinTop;
			area2->joinBottom = 1;

			//エリア2のラスト分割方向のみ登録
			area1->lastDivSideY = beforeDivArea->lastDivSideY;
			area2->lastDivSideY = e_RC_LASTDIVSIDEFLG_BOTTOM;

			area1->pclRect.parcelId = beforeDivArea->pclRect.parcelId;
			area1->pclRect.xSize = beforeDivArea->pclRect.xSize;
			area1->pclRect.ySize = y1;
			area1->areaType = e_AREA_TYPE_OTHER;

			//エリア2はベースパーセルが移動する
			area2->pclRect.parcelId = SC_MESH_SftParcelId(beforeDivArea->pclRect.parcelId, 0, y1 - 1);
			area2->pclRect.xSize = beforeDivArea->pclRect.xSize;
			area2->pclRect.ySize = y2;
			area2->areaType = e_AREA_TYPE_OTHER;
		} else if (1 == beforeDivArea->primaryY) {
			//エリア1の接続フラグ（下）を1
			area1->joinTop = beforeDivArea->joinTop;
			area1->joinBottom = 1;

			//エリア2の接続フラグ（上）を1
			area2->joinTop = 1;
			area2->joinBottom = beforeDivArea->joinBottom;

			//エリア2のラスト分割方向のみ登録
			area1->lastDivSideY = beforeDivArea->lastDivSideY;
			area2->lastDivSideY = e_RC_LASTDIVSIDEFLG_TOP;

			//エリア1はベースパーセルが移動する
			area1->pclRect.parcelId = SC_MESH_SftParcelId(beforeDivArea->pclRect.parcelId, 0, y2 - 1);
			area1->pclRect.xSize = beforeDivArea->pclRect.xSize;
			area1->pclRect.ySize = y1;
			area1->areaType = e_AREA_TYPE_OTHER;

			area2->pclRect.parcelId = beforeDivArea->pclRect.parcelId;
			area2->pclRect.xSize = beforeDivArea->pclRect.xSize;
			area2->pclRect.ySize = y2;
			area2->areaType = e_AREA_TYPE_OTHER;
		} else {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[makeDivAreaHorizontal] error. "HERE);
			return (e_SC_RESULT_FAIL);
		}
		//格納優先度（左右）はそのまま
		area1->primaryX = beforeDivArea->primaryX;
		area2->primaryX = beforeDivArea->primaryX;
		//格納優先度（上下）もそのまま
		area1->primaryY = beforeDivArea->primaryY;
		area2->primaryY = beforeDivArea->primaryY;
		result = e_SC_RESULT_SUCCESS;

	} else {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[makeDivAreaHorizontal] error. %x"HERE, result);
		result = e_SC_RESULT_BADPARAM;
	}

	if (e_SC_RESULT_SUCCESS == result) {
		//左右の接続フラグはそのまま
		area1->joinLeft = beforeDivArea->joinLeft;
		area1->joinRight = beforeDivArea->joinRight;
		area2->joinLeft = beforeDivArea->joinLeft;
		area2->joinRight = beforeDivArea->joinRight;
		//ラスト分割方向左右もそのまま
		area1->lastDivSideX = beforeDivArea->lastDivSideX;
		area2->lastDivSideX = beforeDivArea->lastDivSideX;
	}
	return (result);
}
/**
 * @brief 列切り処理
 * @param beforeDivArea 		[I]分割対象エリア
 * @param alterPosY 			[I]出発地目的地縦シフト量
 * @param BaseAreaTopLv 		[I]基準エリアトップレベル
 * @param ParcelDensity 		[I]対象パーセルトップ一枚密度
 * @param nBorInf 				[I]四隅最寄パーセルまでの情報
 * @param area1 				[O]エリア分割結果1
 * @param area2 				[O]エリア分割結果2
 * @note 基準エリアを含むような枚数を取得し
 * 		 枚数を調整したあと、左右に分ける
 */
static E_SC_RESULT makeDivAreaVertical(INT16 topLevel, RC_AREASTATEINFO *beforeDivArea, INT32 alterPosY,
		RC_BASEAREATOPLVINFO *baseAreaTopLvInf, E_RC_DIVAREA_NBRINFO *oNBorInf, E_RC_DIVAREA_NBRINFO *dNBorInf, RC_AREASTATEINFO *area1,
		RC_AREASTATEINFO *area2) {

	E_SC_RESULT result = e_SC_RESULT_FAIL;

	if (NULL == beforeDivArea || NULL == baseAreaTopLvInf || NULL == oNBorInf || NULL == dNBorInf) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[makeDivAreaVertical] Bad param."HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == area1 || NULL == area2) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[makeDivAreaVertical] Bad param."HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (RC_DIVPCLRANGECNT_MIN > beforeDivArea->pclRect.xSize || RC_DIVPCLRANGECNT_MIN > beforeDivArea->pclRect.ySize) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[makeDivAreaVertical] error. %x"HERE, result);
		return (e_SC_RESULT_BADPARAM);
	}

	//X方向包含最低値初期化
	INT16 oContBaseX = 0;
	INT16 dContBaseX = 0;
	//X枚数
	INT16 x1 = 0;
	INT16 x2 = 0;

	if (e_AREA_TYPE_OD == beforeDivArea->areaType) {
		//初回のみ、2点双方を含む
		if (0 <= alterPosY) {
			//出発地から見て目的地が上
			area1->primaryY = -1;
			area2->primaryY = -1;
		} else {
			//目的地が下
			area1->primaryY = 1;
			area2->primaryY = 1;
		}

		//O側基準エリアを包含するには最低何枚必要か
		oContBaseX = baseAreaContainCountVertical(topLevel, oNBorInf, &beforeDivArea->pclRect, &baseAreaTopLvInf->oBaseAreaTopLv);
		//D側基準エリアを包含するには最低何枚必要か
		dContBaseX = baseAreaContainCountVertical(topLevel, dNBorInf, &beforeDivArea->pclRect, &baseAreaTopLvInf->dBaseAreaTopLv);

		//枚数を決める
		result = decideDivAreaXSize(oContBaseX, dContBaseX, baseAreaTopLvInf->startParcelDensityTopLv,
				baseAreaTopLvInf->goalParcelDensityTopLv, beforeDivArea->pclRect.xSize, beforeDivArea->areaType, &x1, &x2);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[makeDivAreaVertical]decideDivAreaXSize error. %x"HERE, result);
			return (result);
		}
		//出発地最寄フラグ
		if (e_RC_CORNER_NEIGHBORFLG_LEFT == oNBorInf->neighborflgX) {
			//左が近い
			if (e_RC_CORNER_NEIGHBORFLG_LEFT == dNBorInf->neighborflgX) {
				//エラー
				SC_LOG_ErrorPrint(SC_TAG_RC, "[makeDivAreaVertical] error. %x"HERE, result);
				return (e_SC_RESULT_FAIL);
			}
			//格納優先度（左右）は左優先
			area1->primaryX = -1;
			area2->primaryX = -1;
			//エリア1の接続フラグ（右）を1
			area1->joinLeft = beforeDivArea->joinLeft;
			area1->joinRight = 1;
			//エリア2の接続フラグ（左）を1
			area2->joinLeft = 1;
			area2->joinRight = beforeDivArea->joinRight;

			//エリア2のラスト分割方向のみ登録
			area1->lastDivSideX = beforeDivArea->lastDivSideX;
			area2->lastDivSideX = e_RC_LASTDIVSIDEFLG_LEFT;

			area1->pclRect.parcelId = beforeDivArea->pclRect.parcelId;
			area1->pclRect.xSize = x1;
			area1->pclRect.ySize = beforeDivArea->pclRect.ySize;
			area1->areaType = e_AREA_TYPE_O;

			//エリア2はベースパーセルが移動する
			area2->pclRect.parcelId = SC_MESH_SftParcelId(beforeDivArea->pclRect.parcelId, x1 - 1, 0);
			area2->pclRect.xSize = x2;
			area2->pclRect.ySize = beforeDivArea->pclRect.ySize;
			area2->areaType = e_AREA_TYPE_D;
		} else if (e_RC_CORNER_NEIGHBORFLG_RIGHT == oNBorInf->neighborflgX) {
			//右が近い
			if (e_RC_CORNER_NEIGHBORFLG_RIGHT == dNBorInf->neighborflgX) {
				//エラー
				SC_LOG_ErrorPrint(SC_TAG_RC, "[makeDivAreaVertical] error. %x"HERE, result);
				return (e_SC_RESULT_FAIL);
			}
			//格納優先度(左右）は右優先
			area1->primaryX = 1;
			area2->primaryX = 1;
			//エリア1の接続フラグ（左）を1
			area1->joinLeft = 1;
			area1->joinRight = beforeDivArea->joinRight;

			//エリア2の接続フラグ（右）を1
			area2->joinLeft = beforeDivArea->joinLeft;
			area2->joinRight = 1;

			//エリア2のラスト分割方向のみ登録
			area1->lastDivSideX = beforeDivArea->lastDivSideX;
			area2->lastDivSideX = e_RC_LASTDIVSIDEFLG_RIGHT;

			//エリア1はベースパーセルが移動する
			area1->pclRect.parcelId = SC_MESH_SftParcelId(beforeDivArea->pclRect.parcelId, x2 - 1, 0);
			area1->pclRect.xSize = x1;
			area1->pclRect.ySize = beforeDivArea->pclRect.ySize;
			area1->areaType = e_AREA_TYPE_O;

			area2->pclRect.parcelId = beforeDivArea->pclRect.parcelId;
			area2->pclRect.xSize = x2;
			area2->pclRect.ySize = beforeDivArea->pclRect.ySize;
			area2->areaType = e_AREA_TYPE_D;
		} else {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[makeDivAreaVertical] error. %x"HERE, result);
			return (e_SC_RESULT_FAIL);
		}
		result = e_SC_RESULT_SUCCESS;
	} else if (e_AREA_TYPE_O == beforeDivArea->areaType) {

		//O側基準エリアを包含するには最低何枚必要か
		oContBaseX = baseAreaContainCountVertical(topLevel, oNBorInf, &beforeDivArea->pclRect, &baseAreaTopLvInf->oBaseAreaTopLv);

		//枚数を決める
		result = decideDivAreaXSize(oContBaseX, dContBaseX, baseAreaTopLvInf->startParcelDensityTopLv,
				baseAreaTopLvInf->goalParcelDensityTopLv, beforeDivArea->pclRect.xSize, beforeDivArea->areaType, &x1, &x2);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[makeDivAreaVertical]decideDivAreaXSize error. %x"HERE, result);
			return (result);
		}
		//出発地最寄フラグ
		if (e_RC_CORNER_NEIGHBORFLG_LEFT == oNBorInf->neighborflgX) {
			//左が近い
			//格納優先度（左右）は左優先
			area1->primaryX = -1;
			area2->primaryX = -1;
			//エリア1の接続フラグ（右）を1
			area1->joinLeft = beforeDivArea->joinLeft;
			area1->joinRight = 1;
			//エリア2の接続フラグ（左）を1
			area2->joinLeft = 1;
			area2->joinRight = beforeDivArea->joinRight;

			//エリア2のラスト分割方向のみ登録
			area1->lastDivSideX = beforeDivArea->lastDivSideX;
			area2->lastDivSideX = e_RC_LASTDIVSIDEFLG_LEFT;

			area1->pclRect.parcelId = beforeDivArea->pclRect.parcelId;
			area1->pclRect.xSize = x1;
			area1->pclRect.ySize = beforeDivArea->pclRect.ySize;
			area1->areaType = e_AREA_TYPE_O;

			//エリア2はベースパーセルが移動する
			area2->pclRect.parcelId = SC_MESH_SftParcelId(beforeDivArea->pclRect.parcelId, x1 - 1, 0);
			area2->pclRect.xSize = x2;
			area2->pclRect.ySize = beforeDivArea->pclRect.ySize;
			area2->areaType = e_AREA_TYPE_OTHER;
		} else if (e_RC_CORNER_NEIGHBORFLG_RIGHT == oNBorInf->neighborflgX) {
			//右が近い
			//格納優先度(左右）は右優先
			area1->primaryX = 1;
			area2->primaryX = 1;
			//エリア1の接続フラグ（左）を1
			area1->joinLeft = 1;
			area1->joinRight = beforeDivArea->joinRight;

			//エリア2の接続フラグ（右）を1
			area2->joinLeft = beforeDivArea->joinLeft;
			area2->joinRight = 1;

			//エリア2のラスト分割方向のみ登録
			area1->lastDivSideX = beforeDivArea->lastDivSideX;
			area2->lastDivSideX = e_RC_LASTDIVSIDEFLG_RIGHT;

			//エリア1はベースパーセルが移動する
			area1->pclRect.parcelId = SC_MESH_SftParcelId(beforeDivArea->pclRect.parcelId, x2 - 1, 0);
			area1->pclRect.xSize = x1;
			area1->pclRect.ySize = beforeDivArea->pclRect.ySize;
			area1->areaType = e_AREA_TYPE_O;

			area2->pclRect.parcelId = beforeDivArea->pclRect.parcelId;
			area2->pclRect.xSize = x2;
			area2->pclRect.ySize = beforeDivArea->pclRect.ySize;
			area2->areaType = e_AREA_TYPE_OTHER;
		} else {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[makeDivAreaVertical] error. %x"HERE, result);
			return (e_SC_RESULT_FAIL);
		}
		//格納優先度（上下）はそのまま
		area1->primaryY = beforeDivArea->primaryY;
		area2->primaryY = beforeDivArea->primaryY;
		result = e_SC_RESULT_SUCCESS;

	} else if (e_AREA_TYPE_D == beforeDivArea->areaType) {
		//D側基準エリアを包含するには最低何枚必要か
		dContBaseX = baseAreaContainCountVertical(topLevel, dNBorInf, &beforeDivArea->pclRect, &baseAreaTopLvInf->dBaseAreaTopLv);

		//枚数を決める
		result = decideDivAreaXSize(oContBaseX, dContBaseX, baseAreaTopLvInf->startParcelDensityTopLv,
				baseAreaTopLvInf->goalParcelDensityTopLv, beforeDivArea->pclRect.xSize, beforeDivArea->areaType, &x1, &x2);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[makeDivAreaVertical]decideDivAreaXSize error. %x"HERE, result);
			return (result);
		}
		//目的地最寄フラグ
		if (e_RC_CORNER_NEIGHBORFLG_LEFT == dNBorInf->neighborflgX) {
			//左が近い
			//格納優先度（左右）は右優先
			area1->primaryX = 1;
			area2->primaryX = 1;
			//エリア1の接続フラグ（左）を1
			area1->joinLeft = 1;
			area1->joinRight = beforeDivArea->joinRight;

			//エリア2の接続フラグ（右）を1
			area2->joinLeft = beforeDivArea->joinLeft;
			area2->joinRight = 1;

			//エリア2のラスト分割方向のみ登録
			area1->lastDivSideX = beforeDivArea->lastDivSideX;
			area2->lastDivSideX = e_RC_LASTDIVSIDEFLG_RIGHT;

			//エリア1はベースパーセルが移動する
			area1->pclRect.parcelId = SC_MESH_SftParcelId(beforeDivArea->pclRect.parcelId, x2 - 1, 0);
			area1->pclRect.xSize = x1;
			area1->pclRect.ySize = beforeDivArea->pclRect.ySize;
			area1->areaType = e_AREA_TYPE_OTHER;

			area2->pclRect.parcelId = beforeDivArea->pclRect.parcelId;
			area2->pclRect.xSize = x2;
			area2->pclRect.ySize = beforeDivArea->pclRect.ySize;
			area2->areaType = e_AREA_TYPE_D;

		} else if (e_RC_CORNER_NEIGHBORFLG_RIGHT == dNBorInf->neighborflgX) {
			//右が近い
			//格納優先度（左右）は左優先
			area1->primaryX = -1;
			area2->primaryX = -1;
			//エリア1の接続フラグ（右）を1
			area1->joinLeft = beforeDivArea->joinLeft;
			area1->joinRight = 1;

			//エリア2の接続フラグ（左）を1
			area2->joinLeft = 1;
			area2->joinRight = beforeDivArea->joinRight;

			//エリア2のラスト分割方向のみ登録
			area1->lastDivSideX = beforeDivArea->lastDivSideX;
			area2->lastDivSideX = e_RC_LASTDIVSIDEFLG_LEFT;

			area1->pclRect.parcelId = beforeDivArea->pclRect.parcelId;
			area1->pclRect.xSize = x1;
			area1->pclRect.ySize = beforeDivArea->pclRect.ySize;
			area1->areaType = e_AREA_TYPE_OTHER;

			//エリア2はベースパーセルが移動する
			area2->pclRect.parcelId = SC_MESH_SftParcelId(beforeDivArea->pclRect.parcelId, x1 - 1, 0);
			area2->pclRect.xSize = x2;
			area2->pclRect.ySize = beforeDivArea->pclRect.ySize;
			area2->areaType = e_AREA_TYPE_D;
		} else {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[makeDivAreaVertical] error. %x"HERE, result);
			return (e_SC_RESULT_FAIL);
		}
		//格納優先度（上下）はそのまま
		area1->primaryY = beforeDivArea->primaryY;
		area2->primaryY = beforeDivArea->primaryY;
		result = e_SC_RESULT_SUCCESS;

	} else if (e_AREA_TYPE_OTHER == beforeDivArea->areaType) {
		//枚数を決める
		result = decideDivAreaXSize(oContBaseX, dContBaseX, baseAreaTopLvInf->startParcelDensityTopLv,
				baseAreaTopLvInf->goalParcelDensityTopLv, beforeDivArea->pclRect.xSize, beforeDivArea->areaType, &x1, &x2);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[makeDivAreaVertical]decideDivAreaXSize error. %x"HERE, result);
			return (result);
		}
		if (-1 == beforeDivArea->primaryX) {
			//エリア1の接続フラグ（右）を1
			area1->joinLeft = beforeDivArea->joinLeft;
			area1->joinRight = 1;
			//エリア2の接続フラグ（左）を1
			area2->joinLeft = 1;
			area2->joinRight = beforeDivArea->joinRight;

			//エリア2のラスト分割方向のみ登録
			area1->lastDivSideX = beforeDivArea->lastDivSideX;
			area2->lastDivSideX = e_RC_LASTDIVSIDEFLG_LEFT;

			area1->pclRect.parcelId = beforeDivArea->pclRect.parcelId;
			area1->pclRect.xSize = x1;
			area1->pclRect.ySize = beforeDivArea->pclRect.ySize;
			area1->areaType = e_AREA_TYPE_OTHER;

			//エリア2はベースパーセルが移動する
			area2->pclRect.parcelId = SC_MESH_SftParcelId(beforeDivArea->pclRect.parcelId, x1 - 1, 0);
			area2->pclRect.xSize = x2;
			area2->pclRect.ySize = beforeDivArea->pclRect.ySize;
			area2->areaType = e_AREA_TYPE_OTHER;
		} else if (1 == beforeDivArea->primaryX) {
			//エリア1の接続フラグ（左）を1
			area1->joinLeft = 1;
			area1->joinRight = beforeDivArea->joinRight;

			//エリア2の接続フラグ（右）を1
			area2->joinLeft = beforeDivArea->joinLeft;
			area2->joinRight = 1;

			//エリア2のラスト分割方向のみ登録
			area1->lastDivSideX = beforeDivArea->lastDivSideX;
			area2->lastDivSideX = e_RC_LASTDIVSIDEFLG_RIGHT;

			//エリア1はベースパーセルが移動する
			area1->pclRect.parcelId = SC_MESH_SftParcelId(beforeDivArea->pclRect.parcelId, x2 - 1, 0);
			area1->pclRect.xSize = x1;
			area1->pclRect.ySize = beforeDivArea->pclRect.ySize;
			area1->areaType = e_AREA_TYPE_OTHER;

			area2->pclRect.parcelId = beforeDivArea->pclRect.parcelId;
			area2->pclRect.xSize = x2;
			area2->pclRect.ySize = beforeDivArea->pclRect.ySize;
			area2->areaType = e_AREA_TYPE_OTHER;
		} else {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[makeDivAreaVertical] error. %x"HERE, result);
			return (e_SC_RESULT_FAIL);
		}
		//格納優先度（上下）はそのまま
		area1->primaryY = beforeDivArea->primaryY;
		area2->primaryY = beforeDivArea->primaryY;
		//格納優先度（左右）もそのまま
		area1->primaryX = beforeDivArea->primaryX;
		area2->primaryX = beforeDivArea->primaryX;
		result = e_SC_RESULT_SUCCESS;

	} else {
		result = e_SC_RESULT_BADPARAM;
		SC_LOG_ErrorPrint(SC_TAG_RC, "[makeDivAreaVertical] error. %x"HERE, result);
	}

	if (e_SC_RESULT_SUCCESS == result) {
		//上下の接続フラグはそのまま
		area1->joinTop = beforeDivArea->joinTop;
		area1->joinBottom = beforeDivArea->joinBottom;
		area2->joinTop = beforeDivArea->joinTop;
		area2->joinBottom = beforeDivArea->joinBottom;
		//ラスト分割方向上下もそのまま
		area1->lastDivSideY = beforeDivArea->lastDivSideY;
		area2->lastDivSideY = beforeDivArea->lastDivSideY;
	}
	return (result);
}

/**
 * @brief 基準エリアを包含しているか確認する
 * @param areaPclRect [I]対象エリアId,X,Y枚数
 * @param baseAreaTopLvPclRect [I]トップ基準エリア
 */
static Bool baseAreaContainCheck(SCRP_PCLRECT *areaPclRect, SCRP_PCLRECT *baseAreaTopLvPclRect,INT16 topLevel) {

	if (NULL == baseAreaTopLvPclRect || NULL == areaPclRect) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[baseAreaContainCheck] Bad param. "HERE);
		return (false);
	}
	if (0 >= baseAreaTopLvPclRect->xSize || areaPclRect->xSize < baseAreaTopLvPclRect->xSize) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[baseAreaContainCheck] Bad param. baseAreaxSize[%d]divareaxSize[%d]"HERE,baseAreaTopLvPclRect->xSize,areaPclRect->xSize);
		return (false);
	}
	if (0 >= baseAreaTopLvPclRect->ySize || areaPclRect->ySize < baseAreaTopLvPclRect->ySize) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[baseAreaContainCheck] Bad param. baseAreaySize[%d]divareaySize[%d]"HERE,baseAreaTopLvPclRect->ySize,areaPclRect->ySize);
		return (false);
	}
	UINT32 baseLB_parcelId = baseAreaTopLvPclRect->parcelId;
	UINT32 baseRT_parcelId = SC_MESH_SftParcelId(baseAreaTopLvPclRect->parcelId, baseAreaTopLvPclRect->xSize - 1,
			baseAreaTopLvPclRect->ySize - 1);

	INT32 xSft = 0;
	INT32 ySft = 0;
	INT32 intRet = 0;

	intRet = SC_MESH_GetAlterPos(areaPclRect->parcelId, baseLB_parcelId, topLevel, &xSft, &ySft);
	if(0!= intRet){
		SC_LOG_ErrorPrint(SC_TAG_RC, "[baseAreaContainCheck] GetAlterPos error. "HERE);
		return (false);
	}
	if (0 > xSft || 0 > ySft) {
		//基準の左下がマイナス方向なので範囲外
		SC_LOG_ErrorPrint(SC_TAG_RC, "[baseAreaContainCheck]  error. "HERE);
		return (false);
	} else if (areaPclRect->xSize <= xSft || areaPclRect->ySize <= ySft) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[baseAreaContainCheck]  error. "HERE);
		return (false);
	} else {
		xSft = 0;
		ySft = 0;
	}
	intRet = SC_MESH_GetAlterPos(areaPclRect->parcelId, baseRT_parcelId, topLevel, &xSft, &ySft);
	if(0!= intRet){
		SC_LOG_ErrorPrint(SC_TAG_RC, "[baseAreaContainCheck] GetAlterPos error. "HERE);
		return (false);
	}
	if (0 > xSft || 0 > ySft) {
		//基準の右上がマイナス方向なので範囲外
		SC_LOG_ErrorPrint(SC_TAG_RC, "[baseAreaContainCheck]  error. "HERE);
		return (false);
	} else if (areaPclRect->xSize <= xSft || areaPclRect->ySize <= ySft) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[baseAreaContainCheck]  error. "HERE);
		return (false);
	} else {
		//左下・右上ともにOK
	}
	return (true);
}
/**
 * @brief 四隅最寄から基準エリアを包含するには縦に何枚必要か返す
 * @param nBorInf 		[I]最寄までの情報
 * @param areaPclRect 	[I]対象エリアId,X,Y枚数
 * @param baseAreaTopLv [I]トップ基準エリア
 */
static INT16 baseAreaContainCountHorizontal(INT16 topLevel,E_RC_DIVAREA_NBRINFO *nBorInf, SCRP_PCLRECT *areaPclRect, SCRP_LEVELAREA *baseAreaTopLv) {

	if (NULL == baseAreaTopLv || NULL == areaPclRect || NULL == nBorInf) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[baseAreaContainCountHorizontal]Bad param. "HERE);
		return (-1);
	}
	//基準右上取得
	UINT32 baseRT_parcelId = SC_MESH_SftParcelId(baseAreaTopLv->pclRect.parcelId, baseAreaTopLv->pclRect.xSize - 1,baseAreaTopLv->pclRect.ySize - 1);

	INT32 xSft;
	INT32 ySft;
	//計算結果
	INT16 contBaseY;
	INT32 intRet = 0;

	intRet = SC_MESH_GetAlterPos(areaPclRect->parcelId, baseRT_parcelId, topLevel, &xSft, &ySft);
	if(0!= intRet){
		SC_LOG_ErrorPrint(SC_TAG_RC, "[baseAreaContainCountHorizontal] GetAlterPos error. "HERE);
		return (-1);
	}
	if (e_RC_CORNER_NEIGHBORFLG_BOTTOM == nBorInf->neighborflgY) {
		//下が最寄なので枚数はシフト量+1
		contBaseY = ySft + 1;
	} else if (e_RC_CORNER_NEIGHBORFLG_TOP == nBorInf->neighborflgY) {
		//上が最寄なので全体からシフト量+1 を引き基準のサイズを足したもの
		contBaseY = areaPclRect->ySize + (baseAreaTopLv->pclRect.ySize) - ySft - 1;
	} else {
		contBaseY = 0;
	}
	return (contBaseY);
}

/**
 * @brief 四隅最寄から基準エリアを包含するには横に何枚必要か返す
 * @param nBorInf 		[I]最寄までの情報
 * @param areaPclRect 	[I]対象エリアId,X,Y枚数
 * @param baseAreaTopLv [I]トップ基準エリア
 */
static INT16 baseAreaContainCountVertical(INT16 topLevel,E_RC_DIVAREA_NBRINFO *nBorInf, SCRP_PCLRECT *areaPclRect, SCRP_LEVELAREA *baseAreaTopLv) {

	if (NULL == baseAreaTopLv || NULL == areaPclRect || NULL == nBorInf) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[baseAreaContainCountVertical] Bad param. "HERE);
		return (-1);
	}
	//基準右上取得
	UINT32 baseRT_parcelId = SC_MESH_SftParcelId(baseAreaTopLv->pclRect.parcelId, baseAreaTopLv->pclRect.xSize - 1,
			baseAreaTopLv->pclRect.ySize - 1);

	INT32 xSft;
	INT32 ySft;
	//計算結果
	INT16 contBaseX;
	INT32 intRet = 0;
	intRet = SC_MESH_GetAlterPos(areaPclRect->parcelId, baseRT_parcelId, topLevel, &xSft, &ySft);
	if(0!= intRet){
		SC_LOG_ErrorPrint(SC_TAG_RC, "[baseAreaContainCountVertical] GetAlterPos error. "HERE);
		return (-1);
	}
	if (e_RC_CORNER_NEIGHBORFLG_LEFT == nBorInf->neighborflgX) {
		//左が最寄なので枚数はシフト量+1
		contBaseX = xSft + 1;
	} else if (e_RC_CORNER_NEIGHBORFLG_RIGHT == nBorInf->neighborflgX) {
		//右が最寄なので全体からシフト量+1 を引き基準のサイズを足したもの
		contBaseX = areaPclRect->xSize + (baseAreaTopLv->pclRect.xSize) - xSft - 1;
	} else {
		contBaseX = 0;
	}
	return (contBaseX);
}

/**
 * @brief 分割枚数Yの決定をする
 * @param oContBaseY 				[I]O基準エリア最寄からの包含Y枚数
 * @param dContBaseY 				[I]D基準エリア最寄からの包含Y枚数
 * @param parcelDensity 		[I]パーセル一枚密度
 * @param ySize 				[I]分割対象エリアのY枚数
 * @param areaType 				[I]エリアタイプ（OD双方、Oのみ、Dのみ、ODなし）
 * @param y1					[O]エリア1のY枚数
 * @param y2					[O]エリア2のY枚数
 */
static E_SC_RESULT decideDivAreaYSize(INT16 oContBaseY, INT16 dContBaseY, UINT32 startParcelDensity, UINT32 goalParcelDensity, UINT16 ySize,
		E_RC_AREATYPE areaType, INT16 *y1, INT16 *y2) {

	E_SC_RESULT result = e_SC_RESULT_FAIL;
	if (NULL == y1 || NULL == y2) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[decideDivAreaYSize]Bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	//初期化
	*y1 = 0;
	*y2 = 0;
	INT16 alterY = 0;
	//枚数調整用
	INT16 a = 0;
	INT16 b = 0;

	if (0 >= ySize) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[decideDivAreaYSize]Bad param.divareaYSize[%d]"HERE,ySize);
		return (e_SC_RESULT_BADPARAM);
	}

	if (e_AREA_TYPE_OD == areaType) {
		if (0 >= oContBaseY || 0 >= dContBaseY) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[decideDivAreaYSize]Bad param.oContBaseY[%d] dContBaseY[%d] "HERE,oContBaseY,dContBaseY);
			return (e_SC_RESULT_BADPARAM);
		}
		//全体からO基準とD基準を引くと余りが出る
		alterY = ySize - oContBaseY - dContBaseY;
		if (-2 >= alterY) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[decideDivAreaYSize] error. "HERE);
			return (e_SC_RESULT_FAIL);
		} else if (-1 == alterY) {
			*y1 = oContBaseY;
			*y2 = dContBaseY;
			result = e_SC_RESULT_SUCCESS;
		} else {
			//リンク数比較
			if(goalParcelDensity <= startParcelDensity) {
				//出発地の方がリンクが多い
				a = (alterY + 1) / 2;
				b = (alterY + 1) % 2;
				*y1 = oContBaseY + a;
				*y2 = dContBaseY + a + b;
				result = e_SC_RESULT_SUCCESS;
			} else {
				//目的地の方がリンクが多い
				a = (alterY + 1) / 2;
				b = (alterY + 1) % 2;
				*y1 = oContBaseY + a + b;
				*y2 = dContBaseY + a;
				result = e_SC_RESULT_SUCCESS;
			}
		}
	} else if (e_AREA_TYPE_O == areaType) {
		if (0 >= oContBaseY) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[decideDivAreaYSize] Bad param. oContBaseY[%d]"HERE,oContBaseY);
			return (e_SC_RESULT_BADPARAM);
		}
		alterY = ySize - oContBaseY;
		if (0 >= alterY) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[decideDivAreaYSize] error. ySize[%d],oContBaseY[%d]"HERE, ySize,oContBaseY);
			return (e_SC_RESULT_FAIL);
		} else if (1 == alterY) {
			*y1 = oContBaseY;
			*y2 = alterY + 1;
			result = e_SC_RESULT_SUCCESS;
		} else if (1 < alterY && alterY <= oContBaseY) {
			*y1 = oContBaseY + 1;
			*y2 = alterY;
			result = e_SC_RESULT_SUCCESS;

		} else {
			while (oContBaseY < RC_DIVPCLRANGECNT_MAX) {
				if (2 >= alterY) {
					break;
				}
				oContBaseY++;
				alterY--;
			}
			*y1 = oContBaseY;
			*y2 = alterY + 1;
			result = e_SC_RESULT_SUCCESS;
		}
	} else if (e_AREA_TYPE_D == areaType) {
		if (0 >= dContBaseY) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[decideDivAreaYSize]  dContBaseY[%d]"HERE,dContBaseY);
			return (e_SC_RESULT_BADPARAM);
		}
		alterY = ySize - dContBaseY;
		if (0 >= alterY) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[decideDivAreaYSize] error. ySize[%d],dContBaseY[%d]"HERE, ySize,dContBaseY);
			return (e_SC_RESULT_FAIL);

		} else if (1 == alterY) {
			*y1 = alterY + 1;
			*y2 = dContBaseY;
			result = e_SC_RESULT_SUCCESS;
		} else if (1 < alterY && alterY <= dContBaseY) {
			*y1 = alterY;
			*y2 = dContBaseY + 1;
			result = e_SC_RESULT_SUCCESS;
		} else {
			while (dContBaseY < RC_DIVPCLRANGECNT_MAX) {
				if (2 >= alterY) {
					break;
				}
				dContBaseY++;
				alterY--;
			}
			*y1 = alterY + 1;
			*y2 = dContBaseY;
			result = e_SC_RESULT_SUCCESS;
		}
	} else if (e_AREA_TYPE_OTHER == areaType) {
		if (RC_DIVPCLRANGECNT_MIN >= ySize) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[decideDivAreaYSize] Bad param. ySize[%d] <3"HERE,ySize);
			return (e_SC_RESULT_FAIL);
		} else if (RC_DIVPCLRANGECNT_MIN < ySize && ySize < RC_DIVPCLRANGECNT_MAX * 2) {
			//一辺の最大長の2倍内
			a = ySize / 2;
			b = ySize % 2;
			*y1 = a + 1;
			*y2 = a + b;
			result = e_SC_RESULT_SUCCESS;
		} else {
			//一辺最大長の2倍以上
			*y1 = RC_DIVPCLRANGECNT_MAX;
			*y2 = ySize - RC_DIVPCLRANGECNT_MAX + 1;
			result = e_SC_RESULT_SUCCESS;
		}
	} else {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[decideDivAreaYSize] Bad param.areaType[%d] "HERE,areaType);
		return (e_SC_RESULT_BADPARAM);
	}
	return (result);
}
/**
 * @brief 分割枚数Xの決定をする
 * @param oContBaseX 				[I]O基準エリア最寄からの包含X枚数
 * @param dContBaseX 				[I]D基準エリア最寄からの包含X枚数
 * @param parcelDensity 		[I]パーセル一枚密度
 * @param xSize 				[I]分割対象エリアのX枚数
 * @param areaType 				[I]エリアタイプ（OD双方、Oのみ、Dのみ、ODなし）
 * @param x1					[O]エリア1のX枚数
 * @param x2					[O]エリア2のX枚数
 */
static E_SC_RESULT decideDivAreaXSize(INT16 oContBaseX, INT16 dContBaseX, UINT32 startParcelDensity, UINT32 goalParcelDensity, UINT16 xSize,
		E_RC_AREATYPE areaType, INT16 *x1, INT16 *x2) {

	E_SC_RESULT result = e_SC_RESULT_FAIL;
	if (NULL == x1 || NULL == x2) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[decideDivAreaXSize]Bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	//初期化
	*x1 = 0;
	*x2 = 0;
	INT16 alterX =0;
	//枚数調整用
	INT16 a =0;
	INT16 b =0;

	if (0 >= xSize) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[decideDivAreaXSize]Bad param. divAreaxSize[%d]"HERE,xSize);
		return (e_SC_RESULT_BADPARAM);
	}

	if (e_AREA_TYPE_OD == areaType) {
		if (0 >= oContBaseX || 0 >= dContBaseX) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[decideDivAreaXSize] Bad param. oContBaseX[%d],dContBaseX[%d]"HERE, oContBaseX,dContBaseX);
			return (e_SC_RESULT_BADPARAM);
		}
		alterX = xSize - oContBaseX - dContBaseX;
		if (-2 >= alterX) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[decideDivAreaXSize] error. xSize[%d],oContBaseX[%d],dContBaseX[%d]"HERE,xSize, oContBaseX,dContBaseX);
			return (e_SC_RESULT_FAIL);
		} else if (-1 == alterX) {
			*x1 = oContBaseX;
			*x2 = dContBaseX;
			result = e_SC_RESULT_SUCCESS;
		} else {
			//リンク数比較
			if (goalParcelDensity <= startParcelDensity) {
				//出発地の方がリンクが多い
				a = (alterX + 1) / 2;
				b = (alterX + 1) % 2;
				*x1 = oContBaseX + a;
				*x2 = dContBaseX + a + b;
				result = e_SC_RESULT_SUCCESS;
			} else {
				//目的地の方がリンクが多い
				a = (alterX + 1) / 2;
				b = (alterX + 1) % 2;
				*x1 = oContBaseX + a + b;
				*x2 = dContBaseX + a;
				result = e_SC_RESULT_SUCCESS;
			}
		}
	} else if (e_AREA_TYPE_O == areaType) {
		if (0 >= oContBaseX ) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[decideDivAreaXSize] Badparam.oContBaseX[%d]"HERE,oContBaseX);
			return (e_SC_RESULT_BADPARAM);
		}
		alterX = xSize - oContBaseX;
		if (0 >= alterX) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[decideDivAreaXSize]  error. xSize[%d],oContBaseX[%d]"HERE, xSize,oContBaseX);
			return (e_SC_RESULT_FAIL);
		} else if (1 == alterX) {
			*x1 = oContBaseX;
			*x2 = alterX + 1;
			result = e_SC_RESULT_SUCCESS;
		} else if (1 < alterX && alterX <= oContBaseX) {
			*x1 = oContBaseX + 1;
			*x2 = alterX;
			result = e_SC_RESULT_SUCCESS;
		} else {
			while (oContBaseX < RC_DIVPCLRANGECNT_MAX) {
				if (2 >= alterX) {
					break;
				}
				oContBaseX++;
				alterX--;
			}
			*x1 = oContBaseX;
			*x2 = alterX + 1;
			result = e_SC_RESULT_SUCCESS;
		}
	} else if (e_AREA_TYPE_D == areaType) {
		if ( 0 >= dContBaseX) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[decideDivAreaXSize] Bad param. dContBaseX[%d]"HERE,dContBaseX);
			return (e_SC_RESULT_BADPARAM);
		}
		alterX = xSize - dContBaseX;
		if (0 >= alterX) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[decideDivAreaXSize] error.xSize[%d],dContBaseX[%d]"HERE, xSize,dContBaseX);
			return (e_SC_RESULT_FAIL);
		} else if (1 == alterX) {
			*x1 = alterX + 1;
			*x2 = dContBaseX;
			result = e_SC_RESULT_SUCCESS;
		} else if (1 < alterX && alterX <= dContBaseX) {
			*x1 = alterX;
			*x2 = dContBaseX + 1;
			result = e_SC_RESULT_SUCCESS;
		} else {
			while (dContBaseX < RC_DIVPCLRANGECNT_MAX) {
				if (2 >= alterX) {
					break;
				}
				dContBaseX++;
				alterX--;
			}
			*x1 = alterX + 1;
			*x2 = dContBaseX;
			result = e_SC_RESULT_SUCCESS;
		}
	} else if (e_AREA_TYPE_OTHER == areaType) {
		if (RC_DIVPCLRANGECNT_MIN >= xSize) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[decideDivAreaXSize]  error.divAreaxSize[%d] "HERE,xSize);
			return (e_SC_RESULT_FAIL);
		} else if (RC_DIVPCLRANGECNT_MIN < xSize && xSize < RC_DIVPCLRANGECNT_MAX * 2) {
			//一辺の最大長の2倍内
			a = xSize / 2;
			b = xSize % 2;
			*x1 = a + 1;
			*x2 = a + b;
			result = (e_SC_RESULT_SUCCESS);
		} else {
			//一辺最大長の2倍以上
			*x1 = RC_DIVPCLRANGECNT_MAX;
			*x2 = xSize - RC_DIVPCLRANGECNT_MAX + 1;
			result = (e_SC_RESULT_SUCCESS);
		}
	} else {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[decideDivAreaXSize] Bad param.areaType[%d]"HERE,areaType);
		return (e_SC_RESULT_BADPARAM);
	}
	return (result);
}

/**
 * @brief 分割エリアの接続方向フラグ4方向を2方向のフラグにまとめる
 * @param areaType 				[I]エリアタイプ（OD,Oのみ,Dのみ,ODなし）
 * @param joinLeft 				[I]接続方向_左
 * @param joinRight 			[I]接続方向_右
 * @param joinTop 				[I]接続方向_上
 * @param joinBottom 			[I]接続方向_下
 * @param primaryX 				[I]格納優先度_左右
 * @param primaryY 				[I]格納優先度_上下
 * @param joinX					[O]エリアの接続方向フラグ_左右
 * @param joinY 				[O]エリアの接続方向フラグ_上下
*/
static E_SC_RESULT setDivJoinflg(RC_AREASTATEINFO *afterDivArea, INT16 *joinX, INT16 *joinY) {

	if (NULL == afterDivArea || NULL == joinX || NULL == joinY) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[setDivJoinflg]Bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	if (e_AREA_TYPE_O == afterDivArea->areaType || e_AREA_TYPE_OD == afterDivArea->areaType) {
		//1番エリア、もしくは最初の分割が必要ないとき
		if (1 == afterDivArea->joinLeft && 1 == afterDivArea->joinRight) {
			//左にも右にも接続している
			*joinX = 2;
		} else if (0 == afterDivArea->joinLeft && 1 == afterDivArea->joinRight) {
			*joinX = 1;
		} else if (1 == afterDivArea->joinLeft && 0 == afterDivArea->joinRight) {
			*joinX = -1;
		} else if (0 == afterDivArea->joinLeft && 0 == afterDivArea->joinRight) {
			*joinX = 0;
		} else {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[setDivJoinflg] error.areaType[%d]joinLeft[%d]joinRight[%d] "HERE, afterDivArea->areaType,
					afterDivArea->joinLeft, afterDivArea->joinRight);
			return (e_SC_RESULT_FAIL);
		}
		if (1 == afterDivArea->joinBottom && 1 == afterDivArea->joinTop) {
			//下にも上にも接続している
			*joinY = 2;
		} else if (0 == afterDivArea->joinBottom && 1 == afterDivArea->joinTop) {
			*joinY = 1;
		} else if (1 == afterDivArea->joinBottom && 0 == afterDivArea->joinTop) {
			*joinY = -1;
		} else if (0 == afterDivArea->joinBottom && 0 == afterDivArea->joinTop) {
			*joinY = 0;
		} else {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[setDivJoinflg] error.areaType[%d]joinBottom[%d]joinTop[%d] "HERE, afterDivArea->areaType,
					afterDivArea->joinBottom, afterDivArea->joinTop);
			return (e_SC_RESULT_FAIL);
		}
	} else if (e_AREA_TYPE_D == afterDivArea->areaType) {
		//目的地エリアは最後なので接続フラグは0をセット
		*joinX = 0;
		*joinY = 0;
	} else if (e_AREA_TYPE_OTHER == afterDivArea->areaType) {
		/*
		 * 出発地でも目的地でもないエリアの接続フラグは、探索順に背かないようにする
		 */
		if (1 == afterDivArea->joinLeft && 1 == afterDivArea->joinRight) {
			//左にも右にも接続している
			if (e_RC_LASTDIVSIDEFLG_LEFT == afterDivArea->lastDivSideX) {
				//最後の分割方向が左、すなわち左側は必要ない
				*joinX = 1;
			} else if (e_RC_LASTDIVSIDEFLG_RIGHT == afterDivArea->lastDivSideX) {
				//最後の分割方向が右、すなわち右側は必要ない
				*joinX = -1;
			} else {
				//左右の分割方向は決まっていない、すなわち両方に進行できる
				*joinX = 2;
			}
		} else if (0 == afterDivArea->joinLeft && 1 == afterDivArea->joinRight) {
			if(e_RC_LASTDIVSIDEFLG_RIGHT == afterDivArea->lastDivSideX){
				//最後の分割方向が右、すなわち右側は必要ない
				*joinX =0;
			}else{
				*joinX = 1;
			}
		} else if (1 == afterDivArea->joinLeft && 0 == afterDivArea->joinRight) {
			if(e_RC_LASTDIVSIDEFLG_LEFT == afterDivArea->lastDivSideX){
				//最後の分割方向が左、すなわち左側は必要ない
				*joinX =0;
			}else{
				*joinX = -1;
			}
		} else if (0 == afterDivArea->joinLeft && 0 == afterDivArea->joinRight) {
			*joinX = 0;
		} else {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[setDivJoinflg] error.areaType[%d]joinLeft[%d]joinRight[%d] "HERE, afterDivArea->areaType,
					afterDivArea->joinLeft, afterDivArea->joinRight);
			return (e_SC_RESULT_FAIL);
		}
		if (1 == afterDivArea->joinBottom && 1 == afterDivArea->joinTop) {
			//下にも上にも接続している
			if (e_RC_LASTDIVSIDEFLG_BOTTOM == afterDivArea->lastDivSideY) {
				//最後の分割方向が下、すなわち下側は必要ない
				*joinY = 1;
			} else if (e_RC_LASTDIVSIDEFLG_TOP == afterDivArea->lastDivSideY) {
				//最後の分割方向が上、すなわち上側は必要ない
				*joinY = -1;
			} else {
				//上下の分割方向は決まっていない、すなわち両方に進行できる
				*joinY = 2;
			}
		} else if (0 == afterDivArea->joinBottom && 1 == afterDivArea->joinTop) {
			if(e_RC_LASTDIVSIDEFLG_TOP == afterDivArea->lastDivSideY){
				//最後の分割方向が上、すなわち上側は必要ない
				*joinY =0;
			}else{
				*joinY = 1;
			}
		} else if (1 == afterDivArea->joinBottom && 0 == afterDivArea->joinTop) {
			if(e_RC_LASTDIVSIDEFLG_BOTTOM == afterDivArea->lastDivSideY){
				//最後の分割方向が下、すなわち下側は必要ない
				*joinY =0;
			}else{
				*joinY = -1;
			}
		} else if (0 == afterDivArea->joinBottom && 0 == afterDivArea->joinTop) {
			*joinY = 0;
		} else {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[setDivJoinflg] error.areaType[%d]joinBottom[%d]joinTop[%d] "HERE, afterDivArea->areaType,
					afterDivArea->joinBottom, afterDivArea->joinTop);
			return (e_SC_RESULT_FAIL);
		}
	} else {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[setDivJoinflg] areaType[%d]. "HERE, afterDivArea->areaType);
		return (e_SC_RESULT_BADPARAM);
	}
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 分割エリアの切断フラグをセットする
 * @param joinLeft 				[I]接続方向_左
 * @param joinRight 			[I]接続方向_右
 * @param joinTop 				[I]接続方向_上
 * @param joinBottom 			[I]接続方向_下
 * @param splitX				[O]エリアの切断方向フラグ_左右
 * @param splitY 				[O]エリアの切断方向フラグ_上下
*/
static E_SC_RESULT setDivSplitflg(INT16 joinLeft, INT16 joinRight, INT16 joinTop, INT16 joinBottom, INT16 *splitX, INT16 *splitY) {

	if (NULL == splitX || NULL == splitY) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[setDivSplitflg]Bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (1 == joinLeft && 1 == joinRight) {
		//左にも右にも接続している
		*splitX = 0;
	} else if (0 == joinLeft && 1 == joinRight) {
		*splitX = -1;
	} else if (1 == joinLeft && 0 == joinRight) {
		*splitX = 1;
	} else if (0 == joinLeft && 0 == joinRight) {
		*splitX = 2;
	} else {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[setDivSplitflg] Bad param. joinLeft[%d]joinRight[%d]"HERE,joinLeft,joinRight);
		return (e_SC_RESULT_BADPARAM);
	}
	if (1 == joinBottom && 1 == joinTop) {
		//下にも上にも接続している
		*splitY = 0;
	} else if (0 == joinBottom && 1 == joinTop) {
		*splitY = -1;
	} else if (1 == joinBottom && 0 == joinTop) {
		*splitY = 1;
	} else if (0 == joinBottom && 0 == joinTop) {
		*splitY = 2;
	} else {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[setDivSplitflg] Bad param. joinBottom[%d]joinTop[%d]"HERE,joinBottom,joinTop);
		return (e_SC_RESULT_BADPARAM);
	}
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 四隅最寄までの移動枚数を詰める
 * @param topLevel 				[I]トップレベル
 * @param baseAreaRect 			[I]BaseAreaトップレベル矩形
 * @param pclRect 				[I]エリア左下Id,X,Y枚数
 * @param areaType 				[I]エリアタイプ（OD,Oのみ,Dのみ,ODなし）
 * @param nBorInf				[O]最寄情報
 * @note ただし、出発地は左、下優先、目的地は右、上優先
*/
static INT16 getTopAreaClosestCornerSft(
		INT16 topLevel,
		SCRP_PCLRECT *baseAreaRect,
		SCRP_PCLRECT *pclRect,
		E_RC_DIVAREA_NBRINFO *nBorInf,
		E_RC_AREATYPE areaType
		) {
	if (NULL == baseAreaRect || NULL == pclRect || NULL == nBorInf) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[getTopAreaClosestCornerSft] Bad param."HERE);
		return (-1);
	}
	if(baseAreaRect->xSize >= pclRect->xSize){
		SC_LOG_ErrorPrint(SC_TAG_RC, "baseArea-xSize[%d],pclRect->xSize[%d]"HERE,  baseAreaRect->xSize,pclRect->xSize);
		return(-1);
	}
	if(baseAreaRect->ySize >= pclRect->ySize){
		SC_LOG_ErrorPrint(SC_TAG_RC, "baseArea-ySize[%d],pclRect->ySize[%d]"HERE,  baseAreaRect->ySize,pclRect->ySize);
		return(-1);
	}
	//初期化
	nBorInf->neighborflgX = e_RC_CORNER_NEIGHBORFLG_DEFAULT;
	nBorInf->neighborflgY = e_RC_CORNER_NEIGHBORFLG_DEFAULT;
	nBorInf->closestCornerSftX = 0;
	nBorInf->closestCornerSftY = 0;

	UINT32 LB_parcelId = pclRect->parcelId;
	UINT32 RT_parcelId = SC_MESH_SftParcelId(pclRect->parcelId, pclRect->xSize - 1, pclRect->ySize - 1);
	UINT32 baseLB_parcelId = baseAreaRect->parcelId;
	UINT32 baseRT_parcelId = SC_MESH_SftParcelId(baseAreaRect->parcelId, baseAreaRect->xSize - 1, baseAreaRect->ySize - 1);
	INT16 xMin;
	INT16 yMin;

	INT32 xSftLB;
	INT32 ySftLB;
	INT32 xSftRT;
	INT32 ySftRT;

	INT32 intResult =0;

	intResult = SC_MESH_GetAlterPos(LB_parcelId, baseLB_parcelId, topLevel, &xSftLB, &ySftLB);
	if(0!= intResult){
		SC_LOG_ErrorPrint(SC_TAG_RC, " [getTopAreaClosestCornerSft]GetAlterPos error. intResult[%d] "HERE, intResult);
		return (-1);
	}
	if (0 > xSftLB || 0 > ySftLB) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[getTopAreaClosestCornerSft] fail.xSftLB[%d]ySftLB[%d]"HERE,xSftLB,ySftLB);
		return (-1);
	}
	//次に右上との比較
	intResult = SC_MESH_GetAlterPos(RT_parcelId, baseRT_parcelId, topLevel, &xSftRT, &ySftRT);
	if(0!= intResult){
		SC_LOG_ErrorPrint(SC_TAG_RC, " [getTopAreaClosestCornerSft]GetAlterPos error. intResult[%d] "HERE, intResult);
		return (-1);
	}
	if (0 < xSftRT || 0 < ySftRT) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[getTopAreaClosestCornerSft] fail.xSftRT[%d]ySftRT[%d]"HERE,xSftRT,ySftRT);
		return (-1);
	}

	if (e_AREA_TYPE_O == areaType) {
		xMin = xSftLB;
		yMin = ySftLB;

		if (abs(xSftRT) < xMin) {
			nBorInf->neighborflgX = e_RC_CORNER_NEIGHBORFLG_RIGHT;
			nBorInf->closestCornerSftX = abs(xSftRT);
		}else{
			nBorInf->neighborflgX = e_RC_CORNER_NEIGHBORFLG_LEFT;
			nBorInf->closestCornerSftX = xSftLB;
		}
		if (abs(ySftRT) < yMin) {
			nBorInf->neighborflgY =  e_RC_CORNER_NEIGHBORFLG_TOP;
			nBorInf->closestCornerSftY  = abs(ySftRT);
		} else {
			nBorInf->neighborflgY =  e_RC_CORNER_NEIGHBORFLG_BOTTOM;
			nBorInf->closestCornerSftY  = ySftLB;
		}
	} else if (e_AREA_TYPE_D == areaType) {
		xMin = abs(xSftRT);
		yMin = abs(ySftRT);
		if (xSftLB < xMin) {
			nBorInf->neighborflgX = e_RC_CORNER_NEIGHBORFLG_LEFT;
			nBorInf->closestCornerSftX = xSftLB;
		} else {
			nBorInf->neighborflgX = e_RC_CORNER_NEIGHBORFLG_RIGHT;
			nBorInf->closestCornerSftX = abs(xSftRT);
		}
		if (ySftLB < yMin) {
			nBorInf->neighborflgY =  e_RC_CORNER_NEIGHBORFLG_BOTTOM;
			nBorInf->closestCornerSftY  = ySftLB;
		} else {
			nBorInf->neighborflgY =  e_RC_CORNER_NEIGHBORFLG_TOP;
			nBorInf->closestCornerSftY  = abs(ySftRT);
		}
	} else {
		//エリアタイプが違う
		SC_LOG_ErrorPrint(SC_TAG_RC, "[getTopAreaClosestCornerSft] Bad param. AreaType[%d]"HERE,areaType);
		return (-1);
	}
	return (0);
}

/**
 * @brief エリアのjoinフラグをPCLSTATEテーブルに登録する
 * @param targetLevel 			[I]対象レベル
 * @param topLevel 				[I]トップレベル
 * @param pclRect 				[I]エリアId,X,Y枚数
 * @param joinX 				[I]接続情報_左右
 * @param joinY 				[I]接続情報_上下
 * @param pclState 				[O]SCRP_AREAPCLSTATEテーブル
*/
static E_SC_RESULT setAreaPCLStateJoin(INT16 targetLevel, INT16 topLevel,SCRP_PCLRECT *pclRect, INT16 joinX, INT16 joinY, SCRP_AREAPCLSTATE *pclState) {

	if (NULL == pclRect || NULL == pclState) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[setAreaPCLStateJoin] Bad param."HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if ( 0 >= pclRect->xSize || 0 >= pclRect->ySize ) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[setAreaPCLStateJoin] Bad param."HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if ( 3 <= targetLevel || 3 <= topLevel ) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[setAreaPCLStateJoin] Bad param."HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	UINT16 m, n;
	UINT16 idx = 0;
	SCRP_AREAPCLSTATE *pclStatePt = pclState;

	//現状、レベル1,2のみ通す
	if ( targetLevel != topLevel ) {
		if(RP_LEVEL2 == topLevel){
			//下位レベルの接続フラグはすべて0
			for (m = 0; m < pclRect->ySize; m++) {
				for (n = 0; n < pclRect->xSize; n++) {
					(pclStatePt+idx)->join_f = 0x00;
					idx++;
				}
			}
			return (e_SC_RESULT_SUCCESS);
		}else{
			SC_LOG_ErrorPrint(SC_TAG_RC, "[setAreaPCLStateJoin] Bad param.targetLevel[%d]topLevel[%d]"HERE,targetLevel,topLevel);
			return (e_SC_RESULT_BADPARAM);
		}
	} else {
		//条件は targetLevel == topLevelすなわち、分割を行ったレベル
		if (2 == joinX && 2 == joinY) {
			//上下左右にかぶりエリアあり
			//出発地のみ、かつ上下左右にかぶりなので可能性はかなり低い
			for (m = 0; m < pclRect->ySize; m++) {
				if (0 == m) {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (1 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (2 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (3 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (4 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (6 - 1));
						} else if (pclRect->xSize - 1 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (1 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (2 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (3 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (5 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (8 - 1));

						} else {
							(pclStatePt+idx)->join_f |= (0x1 << (1 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (2 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (3 - 1));
						}
						idx++;
					}
				} else if (pclRect->ySize - 1 == m) {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (1 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (4 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (6 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (7 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (8 - 1));
						} else if (pclRect->xSize - 1 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (3 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (5 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (6 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (7 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (8 - 1));
						} else {
							(pclStatePt+idx)->join_f |= (0x1 << (6 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (7 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (8 - 1));
						}
						idx++;
					}
				} else {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (1 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (4 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (6 - 1));
						} else if (pclRect->xSize - 1 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (3 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (5 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (8 - 1));
						} else {
						}
						idx++;
					}
				}
			}
		} else if (2 == joinX && 1 == joinY) {
			//上と左右にかぶりエリアあり
			for (m = 0; m < pclRect->ySize; m++) {
				if (0 == m) {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (4 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (6 - 1));
						} else if (pclRect->xSize - 1 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (5 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (8 - 1));
						} else {
						}
						idx++;
					}
				} else if (pclRect->ySize - 1 == m) {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (1 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (4 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (6 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (7 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (8 - 1));
						} else if (pclRect->xSize - 1 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (3 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (5 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (6 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (7 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (8 - 1));
						} else {
							(pclStatePt+idx)->join_f |= (0x1 << (6 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (7 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (8 - 1));
						}
						idx++;
					}
				} else {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (1 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (4 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (6 - 1));
						} else if (pclRect->xSize - 1 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (3 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (5 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (8 - 1));
						} else {
						}
						idx++;
					}
				}
			}
		} else if (2 == joinX && 0 == joinY) {
			//左右のみかぶり
			for (m = 0; m < pclRect->ySize; m++) {
				if (0 == m) {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (4 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (6 - 1));
						} else if (pclRect->xSize - 1 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (5 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (8 - 1));
						} else {
						}
						idx++;
					}
				} else if (pclRect->ySize - 1 == m) {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (1 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (4 - 1));
						} else if (pclRect->xSize - 1 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (3 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (5 - 1));
						} else {
						}
						idx++;
					}
				} else {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (1 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (4 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (6 - 1));
						} else if (pclRect->xSize - 1 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (3 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (5 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (8 - 1));
						} else {
						}
						idx++;
					}
				}
			}
		} else if (2 == joinX && -1 == joinY) {
			//下と左右にかぶりエリアあり
			for (m = 0; m < pclRect->ySize; m++) {
				if (0 == m) {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (1 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (2 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (3 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (4 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (6 - 1));
						} else if (pclRect->xSize - 1 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (1 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (2 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (3 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (5 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (8 - 1));
						} else {
							(pclStatePt+idx)->join_f |= (0x1 << (1 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (2 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (3 - 1));
						}
						idx++;
					}
				} else if (pclRect->ySize - 1 == m) {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (1 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (4 - 1));
						} else if (pclRect->xSize - 1 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (3 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (5 - 1));
						} else {
						}
						idx++;
					}
				} else {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (1 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (4 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (6 - 1));
						} else if (pclRect->xSize - 1 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (3 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (5 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (8 - 1));
						} else {
						}
						idx++;
					}
				}
			}
		} else if (1 == joinX && 2 == joinY) {
			//上下と右にかぶりエリアあり
			for (m = 0; m < pclRect->ySize; m++) {
				if (0 == m) {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (2 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (3 - 1));
						} else if (pclRect->xSize - 1 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (1 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (2 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (3 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (5 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (8 - 1));
						} else {
							(pclStatePt+idx)->join_f |= (0x1 << (1 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (2 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (3 - 1));
						}
						idx++;
					}
				} else if (pclRect->ySize - 1 == m) {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (7 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (8 - 1));
						} else if (pclRect->xSize - 1 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (3 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (5 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (6 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (7 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (8 - 1));
						} else {
							(pclStatePt+idx)->join_f |= (0x1 << (6 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (7 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (8 - 1));
						}
						idx++;
					}
				} else {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
						} else if (pclRect->xSize - 1 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (3 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (5 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (8 - 1));
						} else {
						}
						idx++;
					}
				}
			}
		} else if (1 == joinX && 1 == joinY) {
			//上と右にかぶりエリアあり
			for (m = 0; m < pclRect->ySize; m++) {
				if (0 == m) {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
						} else if (pclRect->xSize - 1 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (5 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (8 - 1));
						} else {
						}
						idx++;
					}
				} else if (pclRect->ySize - 1 == m) {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (7 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (8 - 1));
						} else if (pclRect->xSize - 1 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (3 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (5 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (6 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (7 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (8 - 1));
						} else {
							(pclStatePt+idx)->join_f |= (0x1 << (6 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (7 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (8 - 1));
						}
						idx++;
					}
				} else {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
						} else if (pclRect->xSize - 1 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (3 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (5 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (8 - 1));
						} else {
						}
						idx++;
					}
				}
			}
		} else if (1 == joinX && 0 == joinY) {
			//右にかぶりエリアあり
			for (m = 0; m < pclRect->ySize; m++) {
				if (0 == m) {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
						} else if (pclRect->xSize - 1 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (5 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (8 - 1));
						} else {
						}
						idx++;
					}
				} else if (pclRect->ySize - 1 == m) {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
						} else if (pclRect->xSize - 1 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (3 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (5 - 1));
						} else {
						}
						idx++;
					}
				} else {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
						} else if (pclRect->xSize - 1 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (3 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (5 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (8 - 1));
						} else {
						}
						idx++;
					}
				}
			}
		} else if (1 == joinX && -1 == joinY) {
			//下と右にかぶりエリアあり
			for (m = 0; m < pclRect->ySize; m++) {
				if (0 == m) {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt + idx)->join_f = 0x00;
						if (0 == n) {
							(pclStatePt + idx)->join_f |= (0x1 << (2 - 1));
							(pclStatePt + idx)->join_f |= (0x1 << (3 - 1));
						} else if (pclRect->xSize - 1 == n) {
							(pclStatePt + idx)->join_f |= (0x1 << (1 - 1));
							(pclStatePt + idx)->join_f |= (0x1 << (2 - 1));
							(pclStatePt + idx)->join_f |= (0x1 << (3 - 1));
							(pclStatePt + idx)->join_f |= (0x1 << (5 - 1));
							(pclStatePt + idx)->join_f |= (0x1 << (8 - 1));
						} else {
							(pclStatePt + idx)->join_f |= (0x1 << (1 - 1));
							(pclStatePt + idx)->join_f |= (0x1 << (2 - 1));
							(pclStatePt + idx)->join_f |= (0x1 << (3 - 1));
						}
						idx++;
					}
				} else if (pclRect->ySize - 1 == m) {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt + idx)->join_f = 0x00;
						if (0 == n) {
						} else if (pclRect->xSize - 1 == n) {
							(pclStatePt + idx)->join_f |= (0x1 << (3 - 1));
							(pclStatePt + idx)->join_f |= (0x1 << (5 - 1));
						} else {
						}
						idx++;
					}
				} else {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt + idx)->join_f = 0x00;
						if (0 == n) {
						} else if (pclRect->xSize - 1 == n) {
							(pclStatePt + idx)->join_f |= (0x1 << (3 - 1));
							(pclStatePt + idx)->join_f |= (0x1 << (5 - 1));
							(pclStatePt + idx)->join_f |= (0x1 << (8 - 1));
						} else {
						}
						idx++;
					}
				}
			}
		} else if (0 == joinX && 2 == joinY) {
			//上下にかぶりエリアあり
			for (m = 0; m < pclRect->ySize; m++) {
				if (0 == m) {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (2 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (3 - 1));
						} else if (pclRect->xSize - 1 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (1 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (2 - 1));
						} else {
							(pclStatePt+idx)->join_f |= (0x1 << (1 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (2 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (3 - 1));
						}
						idx++;
					}
				} else if (pclRect->ySize - 1 == m) {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (7 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (8 - 1));
						} else if (pclRect->xSize - 1 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (6 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (7 - 1));
						} else {
							(pclStatePt+idx)->join_f |= (0x1 << (6 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (7 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (8 - 1));
						}
						idx++;
					}
				} else {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						idx++;
					}
				}
			}
		} else if (0 == joinX && 1 == joinY) {
			//上にかぶりエリアあり
			for (m = 0; m < pclRect->ySize; m++) {
				if (0 == m) {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						idx++;
					}
				} else if (pclRect->ySize - 1 == m) {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (7 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (8 - 1));
						} else if (pclRect->xSize - 1 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (6 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (7 - 1));
						} else {
							(pclStatePt+idx)->join_f |= (0x1 << (6 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (7 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (8 - 1));
						}
						idx++;
					}
				} else {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						idx++;
					}
				}
			}
		} else if (0 == joinX && 0 == joinY) {
			//かぶりエリアなし（分割なし）
			for (m = 0; m < pclRect->ySize; m++) {
				for (n = 0; n < pclRect->xSize; n++) {
					(pclStatePt+idx)->join_f = 0x00;
					idx++;
				}
			}
		} else if (0 == joinX && -1 == joinY) {
			//下にかぶりエリアあり
			for (m = 0; m < pclRect->ySize; m++) {
				if (0 == m) {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (2 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (3 - 1));
						} else if (pclRect->xSize - 1 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (1 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (2 - 1));
						} else {
							(pclStatePt+idx)->join_f |= (0x1 << (1 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (2 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (3 - 1));
						}
						idx++;
					}
				} else if (pclRect->ySize - 1 == m) {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						idx++;
					}
				} else {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						idx++;
					}
				}
			}
		} else if (-1 == joinX && 2 == joinY) {
			//上下と左にかぶりエリアあり
			for (m = 0; m < pclRect->ySize; m++) {
				if (0 == m) {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (1 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (2 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (3 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (4 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (6 - 1));
						} else if (pclRect->xSize - 1 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (1 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (2 - 1));
						} else {
							(pclStatePt+idx)->join_f |= (0x1 << (1 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (2 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (3 - 1));
						}
						idx++;
					}
				} else if (pclRect->ySize - 1 == m) {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (1 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (4 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (6 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (7 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (8 - 1));
						} else if (pclRect->xSize - 1 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (6 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (7 - 1));
						} else {
							(pclStatePt+idx)->join_f |= (0x1 << (6 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (7 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (8 - 1));
						}
						idx++;
					}
				} else {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (1 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (4 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (6 - 1));
						}else{
						}
						idx++;
					}
				}
			}
		} else if (-1 == joinX && 1 == joinY) {
			//上と左にかぶりエリアあり
			for (m = 0; m < pclRect->ySize; m++) {
				if (0 == m) {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (4 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (6 - 1));
						} else{
						}
						idx++;
					}
				} else if (pclRect->ySize - 1 == m) {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (1 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (4 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (6 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (7 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (8 - 1));
						} else if (pclRect->xSize - 1 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (6 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (7 - 1));
						} else {
							(pclStatePt+idx)->join_f |= (0x1 << (6 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (7 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (8 - 1));
						}
						idx++;
					}
				} else {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (1 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (4 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (6 - 1));
						} else {
						}
						idx++;
					}
				}
			}
		} else if (-1 == joinX && 0 == joinY) {
			//左にかぶりエリアあり
			for (m = 0; m < pclRect->ySize; m++) {
				if (0 == m) {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (4 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (6 - 1));
						} else{
						}
						idx++;
					}
				} else if (pclRect->ySize - 1 == m) {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (1 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (4 - 1));
						} else{
						}
						idx++;
					}
				} else {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (1 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (4 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (6 - 1));
						} else{
						}
						idx++;
					}
				}
			}
		} else if (-1 == joinX && -1 == joinY) {
			//下と左にかぶりエリアあり
			for (m = 0; m < pclRect->ySize; m++) {
				if (0 == m) {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (1 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (2 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (3 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (4 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (6 - 1));
						} else if (pclRect->xSize - 1 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (1 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (2 - 1));
						} else {
							(pclStatePt+idx)->join_f |= (0x1 << (1 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (2 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (3 - 1));
						}
						idx++;
					}
				} else if (pclRect->ySize - 1 == m) {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (1 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (4 - 1));
						} else{
						}
						idx++;
					}
				} else {
					for (n = 0; n < pclRect->xSize; n++) {
						(pclStatePt+idx)->join_f = 0x00;
						if (0 == n) {
							(pclStatePt+idx)->join_f |= (0x1 << (1 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (4 - 1));
							(pclStatePt+idx)->join_f |= (0x1 << (6 - 1));
						} else{
						}
						idx++;
					}
				}
			}
		} else {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[setAreaPCLStateJoin] error.joinX[%d]joinY[%d] "HERE,joinX,joinY);
			return (e_SC_RESULT_FAIL);
		}
	}
	return (e_SC_RESULT_SUCCESS);
}
/**
 * @brief 「linkDensity」の設定（矩形以内）
 * @param [I]エリア矩形（Id,X枚数,Y枚数)
 * @param [O]PCLSTATE テーブル
 */
static E_SC_RESULT setAreaPCLStateLink(SCRP_PCLRECT *pclRect, SCRP_AREAPCLSTATE* pclStatePt){
	if (NULL == pclRect || NULL == pclStatePt) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[setAreaPCLStateLink] Bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (0 >= pclRect->xSize || 0 >= pclRect->ySize) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[setAreaPCLStateLink] Bad param. xSize[%d]ySize[%d]"HERE,pclRect->xSize,pclRect->ySize);
		return (e_SC_RESULT_BADPARAM);
	}
	E_SC_RESULT result = e_SC_RESULT_FAIL;
	T_DHC_ROAD_DENSITY densityAreaPtr = {};
	INT16 m,n =0;
	INT16 Idx = 0;
	UINT32 baseParcelId = pclRect->parcelId;
	UINT32 targetParcelId = pclRect->parcelId;
	for(m= 0;m<pclRect->ySize;m++){
		targetParcelId = SC_MESH_SftParcelId(baseParcelId , 0 , m);
		for(n =0;n<pclRect->xSize; n++){
			targetParcelId = SC_MESH_SftParcelId(baseParcelId , n ,m);

			result = RC_GetParcelDensity(&densityAreaPtr, targetParcelId, 1, 1);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_RC, "[setAreaPCLStateLink]getParcelDensity error. %x"HERE, result);
				return(result);
			}
			(pclStatePt+Idx)->linkDensity = densityAreaPtr.totalDensity;
			// 密度を詰めたので解放
			if (NULL != densityAreaPtr.data) {
				RP_MemFree(densityAreaPtr.data, e_MEM_TYPE_ROUTEPLAN);
				densityAreaPtr.data =NULL;
			}
			Idx++;
		}
	}
	return (e_SC_RESULT_SUCCESS);
}
/**
 * @brief 「切断有無フラグ」の設定（矩形以内）
 * @param [I]自パーセルに対する他方のパーセル相対位置（Ｘ方向）
 * @param [I]自パーセルに対する他方のパーセル相対位置（Ｙ方向）
 * @param [I]分割エリア内パーセル枚数（Ｘ方向）
 * @param [I]分割エリア内パーセル枚数（Ｙ方向）
 * @param [I]ダウンロードされているエリアID列
 * @param [I]ダウンロードされているエリアID列の有効テーブル数
 * @param [I]道路密度データ取得関数の出力値
 * @param [I]道路密度データの配列位置
 * @param [IO]PCLSTATE テーブル
 * @memo RC_AreaDiv.c#RC_AreaDiv_Set_DisConnect()の移植
 */
static E_SC_RESULT setAreaPCLStateSplit(INT32 ALT_X, INT32 ALT_Y, INT32 x_size, INT32 y_size, T_DHC_DOWNLOAD_AREA* downloadarea_ptr,
		T_DHC_ROAD_DENSITY* density_area_ptr, INT32 x_range, INT32 y_range, INT32 L_x, INT32 L_y, INT32 H_x, INT32 H_y,
		SCRP_AREAPCLSTATE* T_ParcelList_ptr) {

	if (NULL == downloadarea_ptr || NULL == density_area_ptr || NULL == T_ParcelList_ptr) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[setAreaPCLStateSplit] Bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (0 >= y_size || 0 >= x_size) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[setAreaPCLStateSplit] Bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (L_x > H_x || L_y > H_y) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[setAreaPCLStateSplit] Bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	SCRP_AREAPCLSTATE* w_ptr = T_ParcelList_ptr;
	INT32 m, n;

	//	ダウンロードされているかチェックする。
	for (m = 0; m < y_size; m++) {
		for (n = 0; n < x_size; n++) {
			// 上に接続
			result = checkAreaDownload(downloadarea_ptr, density_area_ptr, x_range, y_range, L_x, L_y, H_x, H_y, (ALT_X + n),
					(ALT_Y + m + 1));
			if (e_SC_RESULT_FAIL == result && y_size-1 != m) {
				w_ptr->split_f |= (0x1 << (7 - 1));
			}

			// 右上に接続
			result = checkAreaDownload(downloadarea_ptr, density_area_ptr, x_range, y_range, L_x, L_y, H_x, H_y, (ALT_X + n + 1),
					(ALT_Y + m + 1));
			if (e_SC_RESULT_FAIL == result && x_size-1 != n && y_size-1 != m) {
				w_ptr->split_f |= (0x1 << (8 - 1));
			}

			// 右  に接続
			result = checkAreaDownload(downloadarea_ptr, density_area_ptr, x_range, y_range, L_x, L_y, H_x, H_y, (ALT_X + n + 1),
					(ALT_Y + m));
			if (e_SC_RESULT_FAIL == result  && x_size-1 != n) {
				w_ptr->split_f |= (0x1 << (5 - 1));
			}

			// 右下に接続
			result = checkAreaDownload(downloadarea_ptr, density_area_ptr, x_range, y_range, L_x, L_y, H_x, H_y, (ALT_X + n + 1),
					(ALT_Y + m - 1));
			if (e_SC_RESULT_FAIL == result && x_size-1 != n && 0 != m) {
				w_ptr->split_f |= (0x1 << (3 - 1));
			}

			// 下に接続
			result = checkAreaDownload(downloadarea_ptr, density_area_ptr, x_range, y_range, L_x, L_y, H_x, H_y, (ALT_X + n),
					(ALT_Y + m - 1));
			if (e_SC_RESULT_FAIL == result && 0 != m) {
				w_ptr->split_f |= (0x1 << (2 - 1));
			}

			// 左下に接続
			result = checkAreaDownload(downloadarea_ptr, density_area_ptr, x_range, y_range, L_x, L_y, H_x, H_y, (ALT_X + n - 1),
					(ALT_Y + m - 1));
			if (e_SC_RESULT_FAIL == result && 0 != n && 0 != m) {
				w_ptr->split_f |= (0x1 << (1 - 1));
			}

			// 左  に接続
			result = checkAreaDownload(downloadarea_ptr, density_area_ptr, x_range, y_range, L_x, L_y, H_x, H_y, (ALT_X + n - 1),
					(ALT_Y + m));
			if (e_SC_RESULT_FAIL == result && 0 != n) {
				w_ptr->split_f |= (0x1 << (4 - 1));
			}

			// 左上に接続している
			result = checkAreaDownload(downloadarea_ptr, density_area_ptr, x_range, y_range, L_x, L_y, H_x, H_y, (ALT_X + n - 1),
					(ALT_Y + m + 1));
			if (e_SC_RESULT_FAIL == result && 0 != n && y_size-1 != m) {
				w_ptr->split_f |= (0x1 << (6 - 1));
			}
			w_ptr++;
		}
	}

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 「ダウンロード」のチェック
 * @param ダウンロードエリア
 * @param 道路密度データ取得関数の出力値
 * @param データ幅（非分割）
 * @param データ高（非分割）
 * @param 収納範囲をチェックする位置・最小（Ｘ方向）
 * @param 収納範囲をチェックする位置・最小（Ｙ方向）
 * @param 収納範囲をチェックする位置・最大（Ｘ方向）
 * @param 収納範囲をチェックする位置・最大（Ｙ方向）
 * @param Ｘ軸方向位置（非分割）
 * @param Ｙ軸方向位置（非分割）
 * @memo RC_AreaDiv.c#RC_AreaDiv_CheckDownload()の移植
 */
static E_SC_RESULT checkAreaDownload(T_DHC_DOWNLOAD_AREA* downloadarea_ptr, T_DHC_ROAD_DENSITY* density_area_ptr, INT32 x_range,
		INT32 y_range, INT32 L_x, INT32 L_y, INT32 H_x, INT32 H_y, INT32 x_pos, INT32 y_pos) {

	if (NULL == downloadarea_ptr || NULL == density_area_ptr) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[checkAreaDownload] Bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	if (x_pos < L_x || x_pos > H_x) {
		return (e_SC_RESULT_FAIL);
	}
	if (y_pos < L_y || y_pos > H_y) {
		return (e_SC_RESULT_FAIL);
	}

	INT32 tbl_no = y_pos * x_range + x_pos;
	if (tbl_no >= (x_range * y_range)) {
		return (e_SC_RESULT_FAIL);
	}
	if (tbl_no < 0) {
		return (e_SC_RESULT_FAIL);
	}

	//	ダウンロードされているかチェックする。
	INT32 k;
	for (k = 0; k < 8; k++) {
		if (density_area_ptr->data[tbl_no].areaId[k] == 0) {
			continue;
		}
		if (density_area_ptr->data[tbl_no].areaId[k] >= M_DHC_DOWNLOAD_AREA_MAX) {
			continue;
		}
		if (downloadarea_ptr->data[density_area_ptr->data[tbl_no].areaId[k] - 1].download_f == M_DHC_DOWNLOAD_AREA_ON) {
			return (e_SC_RESULT_SUCCESS);
		}
	}

	return (e_SC_RESULT_FAIL);
}

/**
 * @brief エリアをキューに追加する
 * @param procNo 				[I]分割処理終了数
 * @param pclRect 				[I]エリアId,X,Y枚数
 * @param altX,altY 			[I]非分割エリア相対位置
 * @param joinX,joinY			[I]エリアの接続方向フラグ
 * @param splitX,splitY 		[I]エリアの切断フラグ
*/
static INT16 areaEnqueue(RC_DIVAREA_QUEUE*divQueue,INT16 procNo, SCRP_PCLRECT pclRect, INT32 altX, INT32 altY, INT16 joinX, INT16 joinY, INT16 splitX, INT16 splitY) {

	if (NULL == divQueue) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[areaEnqueue] Bad param. "HERE);
		return (-1);
	}
	if( RC_DIVAREA_QUEUE_SIZE <=divQueue->count){
		SC_LOG_ErrorPrint(SC_TAG_RC, "[areaEnqueue] error. "HERE);
		return (-1);
	}

	divQueue->divQueuebuf[divQueue->rear].procNo = procNo;
	divQueue->divQueuebuf[divQueue->rear].pclRect.parcelId = pclRect.parcelId;
	divQueue->divQueuebuf[divQueue->rear].pclRect.xSize = pclRect.xSize;
	divQueue->divQueuebuf[divQueue->rear].pclRect.ySize = pclRect.ySize;
	divQueue->divQueuebuf[divQueue->rear].altX = altX;
	divQueue->divQueuebuf[divQueue->rear].altY = altY;
	divQueue->divQueuebuf[divQueue->rear].joinX = joinX;
	divQueue->divQueuebuf[divQueue->rear].joinY = joinY;
	divQueue->divQueuebuf[divQueue->rear].splitX = splitX;
	divQueue->divQueuebuf[divQueue->rear].splitY = splitY;
	divQueue->rear++;
	divQueue->count++;

	if(RC_DIVAREA_QUEUE_SIZE <= divQueue->rear){
		divQueue->rear = 0;
	}
	return (0);

}
/**
 * @brief エリアをキューから取り出す
 * @param procNo 				[O]分割処理終了数
 * @param pclRect 				[O]エリアId,X,Y枚数
 * @param altX,altY 			[O]非分割エリア相対位置
 * @param joinX,joinY			[O]エリアの接続方向フラグ
 * @param splitX,splitY 		[O]エリアの切断フラグ
*/
static INT16 areaDequeue(RC_DIVAREA_QUEUE*divQueue,INT16 *procNo, SCRP_PCLRECT *pclRect, INT32 *altX, INT32 *altY, INT16 *joinX, INT16 *joinY, INT16 *splitX,INT16 *splitY) {

	if (NULL == divQueue || NULL == procNo || NULL == pclRect || NULL == altX || NULL == altY || NULL == joinX
			|| NULL == joinY || NULL == splitX || NULL == splitY) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[areaDequeue] Bad param. "HERE);
		return (-1);
	}
	if (0 >= divQueue->count) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[areaDequeue]error. "HERE);
		return (-1);
	}

	*procNo = divQueue->divQueuebuf[divQueue->front].procNo;
	pclRect->parcelId =  divQueue->divQueuebuf[divQueue->front].pclRect.parcelId;
	pclRect->xSize =  divQueue->divQueuebuf[divQueue->front].pclRect.xSize;
	pclRect->ySize =  divQueue->divQueuebuf[divQueue->front].pclRect.ySize;
	*altX =  divQueue->divQueuebuf[divQueue->front].altX;
	*altY =  divQueue->divQueuebuf[divQueue->front].altY;
	*joinX =  divQueue->divQueuebuf[divQueue->front].joinX;
	*joinY =  divQueue->divQueuebuf[divQueue->front].joinY;
	*splitX =  divQueue->divQueuebuf[divQueue->front].splitX;
	*splitY =  divQueue->divQueuebuf[divQueue->front].splitY;
	divQueue->count--;
	divQueue->front++;
	if(RC_DIVAREA_QUEUE_SIZE <= divQueue->front){
		divQueue->front = 0;
	}
	return (0);
}
/**
 * @brief キューを生成する
 * @param キューアドレス格納
 */
static E_SC_RESULT makeAreaStackQueue(RC_DIVAREA_QUEUE* aQueue) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aQueue) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[makeAreaStackQueue] Bad param. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	aQueue->front = RC_DIVAREA_QUEUE_FRONT_INIT;
	aQueue->rear = RC_DIVAREA_QUEUE_REAR_INIT;
	aQueue->count = RC_DIVAREA_QUEUE_COUNT_INIT;
	aQueue->divQueuebuf = RP_MemAlloc(sizeof(RC_DIVAREA_QUEUEBUFF) * RC_DIVAREA_QUEUE_SIZE, e_MEM_TYPE_ROUTEPLAN);
	if (NULL == aQueue->divQueuebuf) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[makeAreaStackQueue]RP_MemAlloc error."HERE);
		return (e_SC_RESULT_MALLOC_ERR);
	}
	RP_Memset0(aQueue->divQueuebuf, sizeof(RC_DIVAREA_QUEUEBUFF) * RC_DIVAREA_QUEUE_SIZE);
	return (e_SC_RESULT_SUCCESS);
}
