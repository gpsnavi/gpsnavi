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
 * MP_DrawMap.c
 *
 *  Created on:
 *      Author:n.kanagawa
 */

#include "SMCoreMPInternal.h"

#define	MP_CHACK_ZOOM_FLG(_setZoomFlg, _zoomFlg)		(((_zoomFlg & (0x0020 >> (_setZoomFlg-1)))>0) ? true : false)


// パーセルキャッシュ
static T_PARCEL_DATA g_Data[MP_CACHE_PARCEL_CNT];
// 道路用バッファ
static T_POINT gFloatBuf[MP_LINK_POINT_CNT];
static T_POINT gPolyLineData[MP_POLYLINE_CNT];
// 背景用バッファ
static T_POINT gTrianglePoint[MP_BKGD_POLYGON_POINT_CNT*10];
static T_Color gTrianglePointColor[MP_BKGD_POLYGON_POINT_CNT*10];
static T_POINT gContour[MP_BKGD_POLYGON_POINT_CNT];

static INT32 mp_GetDrawParcel(const DOUBLE lat, const DOUBLE lon, const INT32 level, UINT32* pOutParcelList);
static E_SC_RESULT mp_SelectParcelList(const UINT32* pParcelList, const UINT32 parcelCnt, T_PARCEL_DATA* pParcelDataList[]);
static void mp_ChangeFromLatLonToDisp(const DOUBLE Lat, const DOUBLE Lon, FLOAT& x, FLOAT& y);
static E_SC_RESULT mp_CreateParcel(T_PARCEL_DATA *pParcelData, UINT32 parcelID);
static E_SC_RESULT mp_DeleteParcel(T_PARCEL_DATA *pParcelData);
#if 0
static Bool mp_CheckDrawParcel(T_PARCEL_DATA* pData);
#endif
static void mp_CreateParcelInfo(T_PARCEL_DATA *pParcelData);
static void mp_ChangeXYToDisp(T_PARCEL_DATA* p_Data, const INT16 inX, const INT16 inY, T_POINT& outPoint);
static bool mp_DispLineCheck(const T_PARCEL_DATA* p_Data, const T_POINT* pS, const T_POINT* pE);

static bool mp_DrawRoadData(T_PARCEL_DATA* p_Data);
static bool mp_DrawBkgdData(T_PARCEL_DATA* p_Data);
static bool mp_DrawNameData(T_PARCEL_DATA* p_Data);
static bool mp_DrawMarkData(T_PARCEL_DATA* p_Data);
static bool mp_DrawRoadNameData(T_PARCEL_DATA* p_Data);
static bool mp_DrawRoadNoData(T_PARCEL_DATA* p_Data);
static bool mp_DrawBkgdColor(T_PARCEL_DATA* p_Data);
static bool mp_DrawTrfData(T_PARCEL_DATA* p_Data);

static UINT32 mp_SetRoadNameData(T_PARCEL_DATA* p_Data, RDSP_HDL rdspHdl, UINT16 roadKind, T_POINT* pPoint, INT32 pointCnt, UINT32 beforRouteID);
static T_BITMAPFONT* mp_GetRoadNameBitmapFont(MP_FontMng *pFontMng, UINT32 offset, char* pStr, FLOAT fontSize, Bool turnOver, RGBACOLOR color, RGBACOLOR outLineColor);
static bool mp_DispPointCheck(T_PARCEL_DATA* p_Data, T_POINT *pPoint, INT32 cnt);
static INT32 mp_GetBkgdPoint(T_PARCEL_DATA* p_Data, BKGD_OBJ_HDL hObj, T_POINT *pPoint);
static bool mp_SetBkgdVBOPolygon(T_PARCEL_DATA* p_Data, UINT16 bkgdKindCodeType);
static bool mp_DrawBkgdVBOPolygon(T_PARCEL_DATA* p_Data, UINT16 bkgdKindCodeType);
static bool mp_DrawBkgdLine(T_PARCEL_DATA* p_Data, UINT16 bkgdKindCodeType);
static FLOAT mp_GetRealAngle(FLOAT angle, FLOAT yRatio);
static INT32 mp_GetRoadPoint(T_PARCEL_DATA* p_Data, RDSP_HDL hRdsp, T_POINT *pPoint);

#if 0	// デバッグ
static bool mp_DrawRoadNoMesh(T_PARCEL_DATA* p_Data);
#endif

E_SC_RESULT DrawMapInitialize(void)
{
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	memset(g_Data, 0, sizeof(g_Data));

	return (ret);
}

E_SC_RESULT DrawMapFinalize(Bool vboNoDeleteFlg)
{
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	INT32 i;

	if (vboNoDeleteFlg) {
		for (i=0; i<MP_CACHE_PARCEL_CNT; i++) {
			if (g_Data[i].parcel_id != 0) {
				memset(g_Data[i].bkgdVBO, 0, sizeof(g_Data[i].bkgdVBO));
			}
		}
	}

	// データ削除
	for (i=0; i<MP_CACHE_PARCEL_CNT; i++) {
		if (g_Data[i].parcel_id != 0) {
			mp_DeleteParcel(&g_Data[i]);
		}
	}
	memset(g_Data, 0, sizeof(g_Data));

	return (ret);
}

E_SC_RESULT DrawMap(void)
{
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	T_VIEW_INFO* pVi = NULL;
	UINT32 parcelIDList[MP_CACHE_PARCEL_CNT];
	UINT32 parcelIDListOK[MP_CACHE_PARCEL_CNT];
	T_PARCEL_DATA* data[MP_CACHE_PARCEL_CNT];
	INT32 drawParcelCnt = 0;
	INT32 drawParcelCntOK = 0;
	INT32 i = 0;

	pVi = MP_GetViewInfo();

	// 描画パーセルリスト取得
	drawParcelCnt = mp_GetDrawParcel(
			pVi->center_point.la * (DOUBLE)MP_SEC_3600,
			pVi->center_point.lo * (DOUBLE)MP_SEC_3600,
			pVi->level,
			parcelIDList);
	if (0 == drawParcelCnt) {
		SC_LOG_InfoPrint(SC_TAG_MP, "mp_GetDrawParcel draw pclcnt(0), " HERE);
		return (ret);
	}

	// 描画不要パーセル排除
	if (pVi->level == MP_LEVEL_6) {
		parcelIDListOK[drawParcelCntOK] = parcelIDList[0];
		drawParcelCntOK++;
	} else {
		for(i=0; i<drawParcelCnt; i++) {
			PARCEL_INFO_t parcelInfo;
			MP_DRAW_GetParcelInfo(parcelIDList[i], &parcelInfo);
			if (MP_DRAW_CheckDrawParcel(&parcelInfo)) {
				parcelIDListOK[drawParcelCntOK] = parcelIDList[i];
				drawParcelCntOK++;
			}
		}
	}

	// パーセルデータ取得
	ret = mp_SelectParcelList(parcelIDListOK, drawParcelCntOK, data);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_MP, "mp_SelectParcelList error(0x%08x), " HERE, ret);
		return (ret);
	}

	// 中心パーセルの地図未ダウンロードチェック
	if (0 < drawParcelCntOK) {
		if (NULL != data[0]->Basis.p_data) {
			// パーセルが存在する場合
			if (NULL == data[0]->pRdBaseVer) {
				// 地図未ダウンロードの場合
				// 未ダウンロードエリア情報設定
				pVi->noDLFlg = true;
				pVi->noDLAreaCnt = data[0]->areaInfo.cnt;
				memcpy(pVi->noDLAreaNo, data[0]->areaInfo.no, MP_AREA_INFO_CNT_MAX);
			}
		}
	}

	// 描画
	for (i=0; i<drawParcelCntOK; i++) {
		MP_GL_PushMatrix();
		MP_GL_Translatef(
				(FLOAT)(data[i]->ParcelInfo.pixX - pVi->pixelCoord.x),
				(FLOAT)(data[i]->ParcelInfo.pixY - pVi->pixelCoord.y),
				0.0);

		// 背景ベース描画
		if (!mp_DrawBkgdColor(data[i])) {
			SC_LOG_ErrorPrint(SC_TAG_MP, "mp_DrawBkgdColor pcl_id(0x%08x), " HERE, data[i]->parcel_id);
		}

		// 背景
		if (!mp_DrawBkgdData(data[i])) {
			SC_LOG_ErrorPrint(SC_TAG_MP, "mp_DrawBkgdData pcl_id(0x%08x), " HERE, data[i]->parcel_id);
		}

		MP_GL_PopMatrix();
	}

	// 描画
	for (i=0; i<drawParcelCntOK; i++) {
		MP_GL_PushMatrix();
		MP_GL_Translatef(
				(FLOAT)(data[i]->ParcelInfo.pixX - pVi->pixelCoord.x),
				(FLOAT)(data[i]->ParcelInfo.pixY - pVi->pixelCoord.y),
				0.0);

		// 道路
		if (!mp_DrawRoadData(data[i])) {
			SC_LOG_ErrorPrint(SC_TAG_MP, "mp_DrawRoadData pcl_id(0x%08x), " HERE, data[i]->parcel_id);
		}

		MP_GL_PopMatrix();
	}

	// 交通情報描画
	if (pVi->trfInfo.disp && (MAP_LEVEL1 == pVi->level)) {
		// 交通情報キャッシュロック
		SC_TR_LockCacheTbl();

		for (i=0; i<drawParcelCntOK; i++) {
			MP_GL_PushMatrix();
			MP_GL_Translatef(
					(FLOAT)(data[i]->ParcelInfo.pixX - pVi->pixelCoord.x),
					(FLOAT)(data[i]->ParcelInfo.pixY - pVi->pixelCoord.y),
					0.0);

			// 交通情報
			if (!mp_DrawTrfData(data[i])) {
				SC_LOG_ErrorPrint(SC_TAG_MP, "mp_DrawTRF pcl_id(0x%08x), " HERE, data[i]->parcel_id);
			}

			MP_GL_PopMatrix();
		}

		// 交通情報キャッシュロック解除
		SC_TR_UnLockCacheTbl();
	}

	// ルート有無チェック
	if (pVi->route) {
		// 経路描画
		ret = MP_DRAW_Route();
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_MP, "MP_DRAW_RoutePoint error(0x%08x), " HERE, ret);
		}
	}

	MP_GL_BeginBlend();

	// 描画
	for (i=0; i<drawParcelCntOK; i++) {
		MP_GL_PushMatrix();
		MP_GL_Translatef(
				(FLOAT)(data[i]->ParcelInfo.pixX - pVi->pixelCoord.x),
				(FLOAT)(data[i]->ParcelInfo.pixY - pVi->pixelCoord.y),
				0.0);

		// 路線番号衝突判定分割数初期化
		if (NULL != data[i]->pRoadNoCollisionMesh) {
			if (!data[i]->pRoadNoCollisionMesh->setDivCnt(MP_MAP_SIZE, ParamRoadName_DivCnt(pVi->scale))) {
				SC_LOG_ErrorPrint(SC_TAG_MP, "setDivCnt pcl_id(0x%08x), " HERE, data[i]->parcel_id);
			}
		}

		// 記号背景
		if (!mp_DrawMarkData(data[i])) {
			SC_LOG_ErrorPrint(SC_TAG_MP, "mp_DrawMarkData pcl_id(0x%08x), " HERE, data[i]->parcel_id);
		}

		// 道路名称(路線名称)
		if (!mp_DrawRoadNameData(data[i])) {
			SC_LOG_ErrorPrint(SC_TAG_MP, "mp_DrawRoadNameData pcl_id(0x%08x), " HERE, data[i]->parcel_id);
		}

		// 名称
		if (!mp_DrawNameData(data[i])) {
			SC_LOG_ErrorPrint(SC_TAG_MP, "mp_DrawNameData pcl_id(0x%08x), " HERE, data[i]->parcel_id);
		}

		// 道路名称(路線番号)
		if (!mp_DrawRoadNoData(data[i])) {
			SC_LOG_ErrorPrint(SC_TAG_MP, "mp_DrawRoadNoData pcl_id(0x%08x), " HERE, data[i]->parcel_id);
		}

#if 0	// デバッグ
		mp_DrawRoadNoMesh(data[i]);
#endif

		MP_GL_PopMatrix();
	}

	MP_GL_EndBlend();

	return (ret);
}

static void mp_ChangeFromLatLonToDisp(const DOUBLE Lat, const DOUBLE Lon, FLOAT& x, FLOAT& y)
{
	T_VIEW_INFO* pVi;
	DOUBLE pixelCoord_x;
	DOUBLE pixelCoord_y;

	pVi = MP_GetViewInfo();

	// タイルピクセル座標取得
	MESHC_ChgLatLonToTilePixelCoord(
			Lat / MP_SEC_3600,
			Lon / MP_SEC_3600,
			pVi->zoom_level, &pixelCoord_x, &pixelCoord_y);
	x = pixelCoord_x;
	y = pixelCoord_y;
}

static void mp_ChangeXYToDisp(T_PARCEL_DATA* p_Data, const INT16 inX, const INT16 inY, T_POINT& outPoint)
{
	DOUBLE lat;
	DOUBLE lon;
	T_POINT buf;

	if (MP_LEVEL_5 <= p_Data->level) {
		MESHC_ChgParcelIDToLatLon(p_Data->level, p_Data->parcel_id, (DOUBLE)inX, (DOUBLE)inY, &lat, &lon);
		mp_ChangeFromLatLonToDisp(lat, lon, buf.x, buf.y);

		outPoint.x = buf.x - p_Data->ParcelInfo.pixX;
		outPoint.y = buf.y - p_Data->ParcelInfo.pixY;
	} else {
		outPoint.x = (inX * p_Data->ParcelInfo.keisuuX);
		outPoint.y = (inY * p_Data->ParcelInfo.keisuuY);
	}
}

static bool mp_DispLineCheck(const T_PARCEL_DATA* p_Data, const T_POINT* pS, const T_POINT* pE)
{
	bool noFlg = true;

	if (pS->x > p_Data->ParcelInfo.maxX && pE->x < p_Data->ParcelInfo.minX) {
		noFlg = false;
	}

	if (noFlg) {
		if (pE->x > p_Data->ParcelInfo.maxX && pS->x < p_Data->ParcelInfo.minX) {
			noFlg = false;
		}
	}

	if (noFlg) {
		if (pS->y > p_Data->ParcelInfo.maxY && pE->y < p_Data->ParcelInfo.minY) {
			noFlg = false;
		}
	}

	if (noFlg) {
		if (pE->y > p_Data->ParcelInfo.maxY && pS->y < p_Data->ParcelInfo.minY) {
			noFlg = false;
		}
	}

	return (noFlg);
}

// 読み込むパーセルリスト計算
static INT32 mp_GetDrawParcel(const DOUBLE lat, const DOUBLE lon, const INT32 level, UINT32* pOutParcelList)
{
	UINT32 myParcelID = MP_INVALID_VALUE_32;
	DOUBLE myX = 0.0;
	DOUBLE myY = 0.0;
	INT32 parcelCnt = 0;
	T_VIEW_INFO* pVi;

	pVi = MP_GetViewInfo();

	// 緯度経度からパーセルID正規化座標取得
	MESHC_ChgLatLonToParcelID(lat, lon, level, &myParcelID, &myX, &myY);

	// 中心パーセル
	pOutParcelList[0] = myParcelID;

	// レベルによって読み込むパーセル数判定
	if (level <= MP_LEVEL_4) {
		//parcelCnt = MP_READ_PARCEL_CNT_4;
		parcelCnt = MP_READ_PARCEL_CNT_9;
	} else if (level == MP_LEVEL_5){
		//parcelCnt = MP_READ_PARCEL_CNT_9;
		parcelCnt = MP_READ_PARCEL_CNT_25;
	} else {
		pOutParcelList[0] = 0x100F0806;
		parcelCnt = MP_READ_PARCEL_CNT_1;
	}

	// 周辺パーセルID取得
	if (MP_READ_PARCEL_CNT_1 == parcelCnt) {

	}
	else if (MP_READ_PARCEL_CNT_4 == parcelCnt) {
		// 周辺4枚
		if (myX >= MP_MAP_SIZE_HALF && myY >= MP_MAP_SIZE_HALF) {
			// 右上
			// 上、右上、右
			pOutParcelList[1] = MESHC_GetNextParcelID(myParcelID, DIR_TOP);
			pOutParcelList[2] = MESHC_GetNextParcelID(myParcelID, DIR_R_TOP);
			pOutParcelList[3] = MESHC_GetNextParcelID(myParcelID, DIR_R);
		}
		else if (myX >= MP_MAP_SIZE_HALF && myY < MP_MAP_SIZE_HALF) {
			// 右下
			// 右、右下、下
			pOutParcelList[1] = MESHC_GetNextParcelID(myParcelID, DIR_R);
			pOutParcelList[2] = MESHC_GetNextParcelID(myParcelID, DIR_R_DOWN);
			pOutParcelList[3] = MESHC_GetNextParcelID(myParcelID, DIR_DOWN);
		}
		else if (myX < MP_MAP_SIZE_HALF && myY < MP_MAP_SIZE_HALF) {
			// 左下
			// 下、左下、左
			pOutParcelList[1] = MESHC_GetNextParcelID(myParcelID, DIR_DOWN);
			pOutParcelList[2] = MESHC_GetNextParcelID(myParcelID, DIR_L_DOWN);
			pOutParcelList[3] = MESHC_GetNextParcelID(myParcelID, DIR_L);
		}
		else if (myX < MP_MAP_SIZE_HALF && myY >= MP_MAP_SIZE_HALF) {
			// 左上
			// 左、左上、上
			pOutParcelList[1] = MESHC_GetNextParcelID(myParcelID, DIR_L);
			pOutParcelList[2] = MESHC_GetNextParcelID(myParcelID, DIR_L_TOP);
			pOutParcelList[3] = MESHC_GetNextParcelID(myParcelID, DIR_TOP);
		}
	}
	else if (MP_READ_PARCEL_CNT_9 == parcelCnt) {
		// 周辺9枚
		for (INT32 dir=DIR_TOP; dir<=DIR_L_TOP; dir++) {
			pOutParcelList[dir+1] = MESHC_GetNextParcelID(myParcelID, dir);
		}
	}
	else {
		// 周辺25枚
		for (INT32 dir=DIR_TOP; dir<=DIR_L_TOP; dir++) {
			pOutParcelList[dir+1] = MESHC_GetNextParcelID(myParcelID, dir);
		}
		pOutParcelList[ 9] = MESHC_SftParcelID(myParcelID, -2,  2);
		pOutParcelList[10] = MESHC_SftParcelID(myParcelID, -1,  2);
		pOutParcelList[11] = MESHC_SftParcelID(myParcelID,  0,  2);
		pOutParcelList[12] = MESHC_SftParcelID(myParcelID,  1,  2);
		pOutParcelList[13] = MESHC_SftParcelID(myParcelID,  2,  2);

		pOutParcelList[14] = MESHC_SftParcelID(myParcelID, -2, -2);
		pOutParcelList[15] = MESHC_SftParcelID(myParcelID, -1, -2);
		pOutParcelList[16] = MESHC_SftParcelID(myParcelID,  0, -2);
		pOutParcelList[17] = MESHC_SftParcelID(myParcelID,  1, -2);
		pOutParcelList[18] = MESHC_SftParcelID(myParcelID,  2, -2);

		pOutParcelList[19] = MESHC_SftParcelID(myParcelID, -2,  1);
		pOutParcelList[20] = MESHC_SftParcelID(myParcelID, -2,  0);
		pOutParcelList[21] = MESHC_SftParcelID(myParcelID, -2, -1);

		pOutParcelList[22] = MESHC_SftParcelID(myParcelID,  2,  1);
		pOutParcelList[23] = MESHC_SftParcelID(myParcelID,  2,  0);
		pOutParcelList[24] = MESHC_SftParcelID(myParcelID,  2, -1);
	}

	return (parcelCnt);
}

static E_SC_RESULT mp_CreateParcel(T_PARCEL_DATA *pParcelData, UINT32 parcelID)
{
	T_DHC_REQ_PARCEL mapReqPcl;
	T_DHC_RES_DATA mapResData = {};
	Bool pclDataFlg = false;
	E_DHC_CASH_RESULT ret = e_DHC_RESULT_CASH_SUCCESS;
	//char* p_road_shape = NULL;
	//char* p_bkgd = NULL;
	//char* p_name = NULL;
	//char* p_mark = NULL;
	//char* p_road_name = NULL;
	//char* p_basis = NULL;


	// 初期化
	pParcelData->parcel_id = parcelID;
	pParcelData->level = MESHC_GetLevel(parcelID);
	pParcelData->RoadShape.p_data = NULL;
	pParcelData->Bkgd.p_data = NULL;
	pParcelData->Name.p_data = NULL;
	pParcelData->Mark.p_data = NULL;
	pParcelData->RoadName.p_data = NULL;
	pParcelData->Basis.p_data = NULL;
	pParcelData->pNameFontMng = NULL;
	pParcelData->pRoadNameFontMng = NULL;
	pParcelData->pRdBaseVer = NULL;
	pParcelData->pRoadNoCollisionMesh = NULL;

	pParcelData->bkgdBaseFlg = MP_SURFACE_TYPE_SEA;

	// 地図要求情報
	mapReqPcl.user = SC_DHC_USER_MAP;
	mapReqPcl.parcelNum = 1;
	mapReqPcl.parcelInfo[0].parcelId = parcelID;
	mapReqPcl.parcelInfo[0].mapKind = SC_DHC_GetKindMask(e_DATA_KIND_PARCEL_BASIS)
									| SC_DHC_GetKindMask(e_DATA_KIND_SHAPE)
									| SC_DHC_GetKindMask(e_DATA_KIND_BKGD)
									| SC_DHC_GetKindMask(e_DATA_KIND_NAME)
									| SC_DHC_GetKindMask(e_DATA_KIND_MARK)
									| SC_DHC_GetKindMask(e_DATA_KIND_ROAD_NAME)
									| SC_DHC_GetKindMask(e_DATA_KIND_ROAD_BASE_VERSION);

	ret = SC_DHC_MapRead(&mapReqPcl, &mapResData);
	if (e_DHC_RESULT_CASH_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_MP, "SC_DHC_MapRead 0x%08x", ret);
	}

	// パーセル基本情報バイナリ
	if (NULL != mapResData.parcelBin[0].binParcelBasis) {
		pParcelData->Basis.p_data = (UINT8*)mapResData.parcelBin[0].binParcelBasis;

		PCLB_HDL pclbHdl = (PCLB_HDL)pParcelData->Basis.p_data;

		// 海陸フラグ取得
		pParcelData->bkgdBaseFlg = GET_PCLB_SEA_FLG(pclbHdl);

		// エリア情報取得
		UINT8 areaCnt = GET_PCLB_AREAREC_CNT(pclbHdl);
		AREA_HDL areaHdl = GET_PCLB_AREA_HDL(pclbHdl);
		for (INT32 i=0; i<areaCnt*4; i++) {
			UINT8 areaNo = GET_AREA_AREA_NO(areaHdl, i);
			if (0 == areaNo) { // 0は無効値
				continue;
			}
			//SC_LOG_InfoPrint(SC_TAG_MP, (Char*)"★%08x, %d, no[0x%02x]", parcelID, pParcelData->areaInfo.cnt, areaNo);
			pParcelData->areaInfo.no[pParcelData->areaInfo.cnt] = areaNo;
			pParcelData->areaInfo.cnt++;
		}

		pclDataFlg = true;
	}

	// 道路形状バイナリ
	if (NULL != mapResData.parcelBin[0].binShape) {
		pParcelData->RoadShape.p_data = (UINT8*)mapResData.parcelBin[0].binShape;
		pParcelData->p_RoadShapeAnalyze = new SMRoadShapeAnalyze;
		pParcelData->p_RoadShapeAnalyze->Initialize(pParcelData->parcel_id, (const char*)pParcelData->RoadShape.p_data);
		pclDataFlg = true;
	}

	// 背景バイナリ
	if (NULL != mapResData.parcelBin[0].binBkgd) {
		pParcelData->Bkgd.p_data = (UINT8*)mapResData.parcelBin[0].binBkgd;
		pParcelData->p_BkgdAnalyze = new SMBkgdAnalyze;
		pParcelData->p_BkgdAnalyze->Initialize(pParcelData->parcel_id, (const char*)pParcelData->Bkgd.p_data);
		pclDataFlg = true;
	}

	// 名称バイナリ
	if (NULL != mapResData.parcelBin[0].binName) {
		pParcelData->Name.p_data = (UINT8*)mapResData.parcelBin[0].binName;
		pParcelData->p_NameAnalyze = new SMNameAnalyze;
		pParcelData->p_NameAnalyze->Initialize(pParcelData->parcel_id, (const char*)pParcelData->Name.p_data);
		pclDataFlg = true;
	}

	// 記号背景バイナリ
	if (NULL != mapResData.parcelBin[0].binMark) {
		pParcelData->Mark.p_data = (UINT8*)mapResData.parcelBin[0].binMark;
		pParcelData->p_MarkAnalyze = new SMMarkAnalyze;
		pParcelData->p_MarkAnalyze->Initialize(pParcelData->parcel_id, (const char*)pParcelData->Mark.p_data);
		pclDataFlg = true;
	}

	// 道路名称バイナリ
	if (NULL != mapResData.parcelBin[0].binRoadName) {
		pParcelData->RoadName.p_data = (UINT8*)mapResData.parcelBin[0].binRoadName;
		pParcelData->p_RoadNameAnalyze = new SMRoadNameAnalyze;
		pParcelData->p_RoadNameAnalyze->Initialize(pParcelData->parcel_id, (const char*)pParcelData->RoadName.p_data);
		pclDataFlg = true;
	}

	// 道路系ベースバージョン
	if (NULL != mapResData.parcelBin[0].iRoadBaseVersion) {
		pParcelData->pRdBaseVer = (UINT32*)mapResData.parcelBin[0].iRoadBaseVersion;
		pclDataFlg = true;
	}

	if (pclDataFlg == false) {
		// NOデータ
		SC_LOG_ErrorPrint(SC_TAG_MP, "SC_DHC_MapRead NoData %d", ret);
		return (e_SC_RESULT_NODATA);
	}

	// ビットマップフォント初期化
	if (NULL != pParcelData->p_NameAnalyze) {
		pParcelData->pNameFontMng = new MP_FontMng(pParcelData->p_NameAnalyze->GetNameCnt(), false);
		if (NULL == pParcelData->pNameFontMng) {
			SC_LOG_ErrorPrint(SC_TAG_MP, "pNameFontMng NULL");
		}
	}

	// 道路名称ビットマップフォント管理初期化
	pParcelData->pRoadNameFontMng = new MP_FontMng(0, true);
	if (NULL == pParcelData->pRoadNameFontMng) {
		SC_LOG_ErrorPrint(SC_TAG_MP, "pRoadNameFontMng NULL");
	}

	// 路線番号衝突判定初期化
	pParcelData->pRoadNoCollisionMesh = new MP_CollisionMesh;
	if (NULL == pParcelData->pRoadNoCollisionMesh) {
		SC_LOG_ErrorPrint(SC_TAG_MP, "pRoadNoCollisionMesh NULL");
	}

	return (e_SC_RESULT_SUCCESS);
}

static void mp_CreateParcelInfo(T_PARCEL_DATA *pParcelData)
{
	T_VIEW_INFO* pVi;

	pVi = MP_GetViewInfo();

	MP_DRAW_GetParcelInfo(pParcelData->parcel_id, &pParcelData->ParcelInfo);
}

static E_SC_RESULT mp_DeleteParcel(T_PARCEL_DATA *pParcelData)
{
	T_DHC_REQ_PARCEL mapReqPcl;
	INT32 i = 0;
	INT32 j = 0;

	if (NULL != pParcelData->p_RoadShapeAnalyze) {
		delete pParcelData->p_RoadShapeAnalyze;
		pParcelData->p_RoadShapeAnalyze = NULL;
	}
	if (NULL != pParcelData->p_BkgdAnalyze) {
		delete pParcelData->p_BkgdAnalyze;
		pParcelData->p_BkgdAnalyze = NULL;
	}
	if (NULL != pParcelData->p_NameAnalyze) {
		delete pParcelData->p_NameAnalyze;
		pParcelData->p_NameAnalyze = NULL;
	}
	if (NULL != pParcelData->p_MarkAnalyze) {
		delete pParcelData->p_MarkAnalyze;
		pParcelData->p_MarkAnalyze = NULL;
	}
	if (NULL != pParcelData->p_RoadNameAnalyze) {
		delete pParcelData->p_RoadNameAnalyze;
		pParcelData->p_RoadNameAnalyze = NULL;
	}
	//if (NULL != pParcelData->p_BasisAnalyze) {
	//	delete pParcelData->p_BasisAnalyze;
	//	pParcelData->p_BasisAnalyze = NULL;
	//}

	mapReqPcl.user = SC_DHC_USER_MAP;
	mapReqPcl.parcelNum = 1;
	mapReqPcl.parcelInfo[0].parcelId = pParcelData->parcel_id;
	mapReqPcl.parcelInfo[0].mapKind = 0;

	// 道路形状バイナリ解放
	if (NULL != pParcelData->RoadShape.p_data) {
		mapReqPcl.parcelInfo[0].mapKind = mapReqPcl.parcelInfo[0].mapKind | SC_DHC_GetKindMask(e_DATA_KIND_SHAPE);
		pParcelData->RoadShape.p_data = NULL;
	}

	// 背景バイナリ解放
	if (NULL != pParcelData->Bkgd.p_data) {
		mapReqPcl.parcelInfo[0].mapKind = mapReqPcl.parcelInfo[0].mapKind | SC_DHC_GetKindMask(e_DATA_KIND_BKGD);
		pParcelData->Bkgd.p_data = NULL;
	}

	// 名称バイナリ解放
	if (NULL != pParcelData->Name.p_data) {
		mapReqPcl.parcelInfo[0].mapKind = mapReqPcl.parcelInfo[0].mapKind | SC_DHC_GetKindMask(e_DATA_KIND_NAME);
		pParcelData->Name.p_data = NULL;
	}

	// 記号背景バイナリ解放
	if (NULL != pParcelData->Mark.p_data) {
		mapReqPcl.parcelInfo[0].mapKind = mapReqPcl.parcelInfo[0].mapKind | SC_DHC_GetKindMask(e_DATA_KIND_MARK);
		pParcelData->Mark.p_data = NULL;
	}

	// 道路名称バイナリ解放
	if (NULL != pParcelData->RoadName.p_data) {
		mapReqPcl.parcelInfo[0].mapKind = mapReqPcl.parcelInfo[0].mapKind | SC_DHC_GetKindMask(e_DATA_KIND_ROAD_NAME);
		pParcelData->RoadName.p_data = NULL;
	}

	// パーセル基本情報バイナリ解放
	if (NULL != pParcelData->Basis.p_data) {
		mapReqPcl.parcelInfo[0].mapKind = mapReqPcl.parcelInfo[0].mapKind | SC_DHC_GetKindMask(e_DATA_KIND_PARCEL_BASIS);
		pParcelData->Basis.p_data = NULL;
	}

	// パーセル道路系ベースバージョン解放
	if (NULL != pParcelData->pRdBaseVer) {
		mapReqPcl.parcelInfo[0].mapKind = mapReqPcl.parcelInfo[0].mapKind | SC_DHC_GetKindMask(e_DATA_KIND_ROAD_BASE_VERSION);
		pParcelData->pRdBaseVer = NULL;
	}

	// 地図データ解放
	if (0 < mapReqPcl.parcelInfo[0].mapKind) {
		SC_DHC_MapFree(&mapReqPcl);
	}

	// 名称ビットマップフォント管理解放
	if (NULL != pParcelData->pNameFontMng) {
		delete pParcelData->pNameFontMng;
		pParcelData->pNameFontMng = NULL;
	}

	// 道路名称ビットマップフォント管理解放
	if (NULL != pParcelData->pRoadNameFontMng) {
		delete pParcelData->pRoadNameFontMng;
		pParcelData->pRoadNameFontMng = NULL;
	}

	// 背景VBO解放
	for (i=0; i<MP_BKGD_KIND_CODE_TYPE_MAX; i++) {
		for (j=0; j<pParcelData->bkgdVBO[i].vboCnt; j++) {
			if (0 != pParcelData->bkgdVBO[i].vbo[j].vboID) {
				MP_GL_DeleteVBO(&pParcelData->bkgdVBO[i].vbo[j]);
			}
		}
	}

	// 路線番号衝突判定解放
	if (NULL != pParcelData->pRoadNoCollisionMesh) {
		delete pParcelData->pRoadNoCollisionMesh;
		pParcelData->pRoadNoCollisionMesh = NULL;
	}

	// レコード初期化
	memset(pParcelData, 0, sizeof(T_PARCEL_DATA));

	return (e_SC_RESULT_SUCCESS);
}

static E_SC_RESULT mp_SelectParcelList(const UINT32* pParcelList, const UINT32 parcelCnt, T_PARCEL_DATA* pParcelDataList[])
{
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	INT32 i = 0;
	INT32 j = 0;
	Bool fflg = false;
	INT32 No = 0;
#if 1
	// 不要なデータ削除
	// キャッシュ数分ループ
	for (i=0; i<MP_CACHE_PARCEL_CNT; i++) {
		fflg = false;

		// 要求パーセル数分ループ
		for (j=0; j<parcelCnt; j++) {
			if (g_Data[i].parcel_id == pParcelList[j]) {
				// キャッシュ有
				fflg = true;
				break;
			}
		}

		// キャッシュ有無チェック
		if (fflg == false) {
			// キャッシュ無の場合

			// パーセル解放
			if(g_Data[i].parcel_id != 0) {
				mp_DeleteParcel(&g_Data[i]);
			}
		}
	}

	// 要求パーセル数分ループ
	for (i=0; i<parcelCnt; i++) {
		fflg = false;
		No = 0;
		for (j=0; j<MP_CACHE_PARCEL_CNT; j++) {
			if (g_Data[j].parcel_id == 0) {
				No = j;
			}
			else if (g_Data[j].parcel_id == pParcelList[i]) {
				fflg = true;
				No = j;
				break;
			}
		}

		if (fflg == false) {
			mp_CreateParcel(&g_Data[No], pParcelList[i]);
		}
		mp_CreateParcelInfo(&g_Data[No]);

		pParcelDataList[i] = &g_Data[No];
	}
#else
	for (i=0; i<MP_CACHE_PARCEL_CNT; i++) {
		if(g_Data[i].parcel_id != 0) {
			//mp_DeleteParcel(&g_Data[i]);
		}
	}
	for (i=0; i<parcelCnt; i++) {
		mp_CreateParcel(&g_Data[i], pParcelList[i]);
		pParcelDataList[i] = &g_Data[i];
	}
#endif
	return (ret);
}

#if 0
static Bool mp_CheckDrawParcel(T_PARCEL_DATA* pData)
{
	return (MP_DRAW_CheckDrawParcel(&pData->ParcelInfo));
}
#endif

static bool mp_DrawRoadData(T_PARCEL_DATA* p_Data)
{
	T_VIEW_INFO* pVi = NULL;

	if (p_Data->p_RoadShapeAnalyze == NULL) {
		return (true);
	}

	pVi = MP_GetViewInfo();

	FLOAT keisuuX = p_Data->ParcelInfo.keisuuX;
	FLOAT keisuuY = p_Data->ParcelInfo.keisuuY;

	// 表示対象道路名称初期化
	p_Data->roadNameInfoCnt = 0;

	for (UINT32 loop = 0; loop < 2; loop++) {
		// 道路種別数分ループ
		for (INT16 roadkind=15; roadkind >= 0; roadkind--) {
			FLOAT line_size = 0.0f;
			FLOAT line_size_half = 0.0f;
			UINT32 roadKindCnt = 0;

			// 道路形状ハンドル取得
			RDSP_HDL h_rdsp = p_Data->p_RoadShapeAnalyze->GetRoadShapeHdlOfRoadKind(roadkind, roadKindCnt);
			if(NULL == h_rdsp) {
				continue;
			}

			Bool edge_flg = false;
			RGBACOLOR road_color;
			DRAWPARAM param = ParamRoad_Param(pVi->scale_level, roadkind);
			if (0 == loop) {
				line_size = GET_WIDTH_OUT(param);
				road_color = ParamRoad_OLineColor(roadkind);

				// 縁描画判定
				if (!GET_OUTFLG(param)) {
					// 縁無し
					continue;
				}
			}
			else {
				line_size = GET_WIDTH(param);
				if (0 == line_size) {
					continue;
				}
				road_color = ParamRoad_LineColor(roadkind);
			}
			line_size /= pVi->scale;
			line_size_half = line_size * 0.5f;

			edge_flg = GET_EDGE(param);

			MP_GL_ColorRGBA(road_color);

			UINT32 route_id_tmp = 0;

			INT32 polyLineDataIdx = 0;

			// 道路種別毎のリンク数分ループ
			for (UINT16 i=0; i<roadKindCnt; i++) {
				// 道路形状ハンドル取得
				if (0 != i) {
					h_rdsp = GET_LKSP_NEXT_HDL(h_rdsp);
				}

				// リンク基本情報1取得
				BA_LINK_BASE_INFO1 base_info1;
				base_info1.d = GET_LKSP_LINK_BASE_INFO1(h_rdsp);

//				// リンク基本情報1取得
//				BA_LINK_BASE_INFO2 base_info2;
//				base_info2.d = GET_LKSP_LINK_BASE_INFO2(h_rdsp);

				if (base_info1.b.tunnel_link) {
					// トンネルリンク
					if (0 == loop) {
						continue;
					}
				}

				// 点数取得
				UINT16 point_cnt = GET_LKSP_POINT_CNT(h_rdsp);

				INT32 draw_point_cnt = 0;
				T_POINT tmp;
				bool no_flg = true;
				UINT16* pShape = GET_LKSP_POINT_P(h_rdsp);
				for (UINT32 j = 0; j < point_cnt; j++) {
					//UINT16 x = GET_LKSP_POINT_X(h_rdsp, j);
					//UINT16 y = GET_LKSP_POINT_Y(h_rdsp, j);
					UINT16 x = *(pShape);
					UINT16 y = *(pShape+1);

					if (MP_LEVEL_5 <= p_Data->level) {
						DOUBLE lat, lon;
						FLOAT buf[2];
						MESHC_ChgParcelIDToLatLon(p_Data->level, p_Data->parcel_id, x, y, &lat, &lon);
						mp_ChangeFromLatLonToDisp(lat, lon, buf[0], buf[1]);
						tmp.x = buf[0]-p_Data->ParcelInfo.pixX;
						tmp.y = buf[1]-p_Data->ParcelInfo.pixY;
					} else {
						tmp.x = (x * keisuuX);
						tmp.y = (y * keisuuY);
					}

					if (no_flg) {
						if (tmp.x >= p_Data->ParcelInfo.minX && tmp.x <= p_Data->ParcelInfo.maxX) {
							if (tmp.y >= p_Data->ParcelInfo.minY && tmp.y <= p_Data->ParcelInfo.maxY) {
								no_flg = false;
							}
						}
					}

					gFloatBuf[draw_point_cnt].x = tmp.x;
					gFloatBuf[draw_point_cnt].y = tmp.y;

					draw_point_cnt++;
					pShape += 2;
				}

				if (no_flg) {
					if (pVi->level <= MP_LEVEL_2) {
						no_flg = mp_DispLineCheck(p_Data, &gFloatBuf[0], &gFloatBuf[draw_point_cnt-1]);
					}
				}

				if (no_flg) {
					continue;
				}

				if (base_info1.b.tunnel_link) {
					// トンネルの場合
					// それまでのデータがあれば描画
					if (polyLineDataIdx > 0) {
						MP_GL_DrawPolyLine(gPolyLineData, polyLineDataIdx);
					}
					polyLineDataIdx = 0;

					MP_GL_ColorRGBATrans(road_color, 0.5f);
					MP_GL_BeginBlend();

					MP_GL_DrawLines(gFloatBuf, draw_point_cnt, line_size);

					MP_GL_EndBlend();
					MP_GL_ColorRGBA(road_color);
				}
				else {
					// トンネル以外
					// ポリラインデータ設定
					MP_GL_DegenerateTriangle(gFloatBuf, draw_point_cnt, line_size_half, gPolyLineData, &polyLineDataIdx);

					// 端描画
					if (edge_flg && loop == 1) {
						if (!pVi->scroll_mode && !pVi->zoom_mode) {
							MP_GL_DrawCircleFillEx(&gFloatBuf[0], line_size_half);
							MP_GL_DrawCircleFillEx(&gFloatBuf[draw_point_cnt-1], line_size_half);
						}
					}

					if (polyLineDataIdx > MP_VERTEX_BUFFER_MAX) {
						// ポリライン描画
						MP_GL_DrawPolyLine(gPolyLineData, polyLineDataIdx);
						polyLineDataIdx = 0;
					}
				}

				//*********************************************
				// 道路名称情報取得
				//*********************************************
				if (loop != 0) {
					if (MP_LEVEL_1 == p_Data->level && MP_LINK_KIND1_NONSEPARATE_MAIN >= base_info1.b.link_kind1) {
						route_id_tmp = mp_SetRoadNameData(p_Data, h_rdsp, roadkind, gFloatBuf, draw_point_cnt, route_id_tmp);
					}
				}
			}

			// ポリライン描画
			if (polyLineDataIdx > 0) {
				MP_GL_DrawPolyLine(gPolyLineData, polyLineDataIdx);
			}
		}
	}

	return (true);
}

static bool mp_DrawTrfData(T_PARCEL_DATA* p_Data)
{
	T_VIEW_INFO				*pVi = NULL;
	TR_CONGESTION_INFO_t	*pCngsInfo = NULL;

	if (p_Data->p_RoadShapeAnalyze == NULL) {
		return (true);
	}

	pVi = MP_GetViewInfo();

	// 渋滞ラインフチ色設定
	MP_GL_Color4f(1.0f, 1.0f, 1.0f, 1.0f);

	// 交通情報データ取得
	pCngsInfo = SC_TR_GetCongestion(p_Data->parcel_id);
	if (NULL == pCngsInfo) {
		return (true);
	}

	// 道路種別数分ループ
	for (INT16 roadkind=MP_ROAD_KIND_CODE_12; roadkind >= MP_ROAD_KIND_CODE_0; roadkind--) {
		INT32 outLineIdx = 0;
		INT32 lineIdx = 0;

		// 情報数取得
		INT32 infoCnt = pCngsInfo->roadKind[roadkind].cnt;
		if (0 == infoCnt) {
			continue;
		}

		// 渋滞ライン太さ取得
		FLOAT lineWHalf = (ParamCongestion_LineSize(pVi->scale_level, roadkind)/pVi->scale) / 2.0f;
		FLOAT outLineWHalf = (ParamCongestion_OutLineSize(pVi->scale_level, roadkind)/pVi->scale) / 2.0f;

		// 指定道路種別の道路の太さ取得
		DRAWPARAM param = ParamRoad_Param(pVi->scale_level, roadkind);
		FLOAT lineSize = GET_WIDTH(param) / pVi->scale;
		if (0 == lineSize) {
			continue;
		}

		// 交通情報表示位置へのシフト量算出
		FLOAT shift = (lineSize/2.0f) + outLineWHalf;

		// 情報先頭アドレス取得
		char *pInfo = pCngsInfo->roadKind[roadkind].pInfo;

		// 情報数分ループ
		for (INT32 i=0; i<infoCnt; i++) {
			// 渋滞度
			UINT16 cngsLvl = TR_CNGS_LVL(pInfo);
			// 方向
			UINT8 dir = TR_LINK_DIR(pInfo);
			// 色
			RGBACOLOR cngsColor = ParamCongestion_Color(cngsLvl);
			if (0 == (UINT32)cngsColor) {
				pInfo += TR_INFO_SIZE(pInfo);
				continue;
			}
			// 矢印有無
			Bool arrow = false;
			if (MP_CNGS_LVL_2 == cngsLvl) {
				arrow = true;
			}

			// リンクIDに該当する道路形状ハンドル取得
			RDSP_HDL hRdsp = p_Data->p_RoadShapeAnalyze->GetRoadShapeHdl_ByLinkID(TR_LINK_ID(pInfo));
			if (NULL == hRdsp) {
				pInfo += TR_INFO_SIZE(pInfo);
				continue;
			}

			// 道路リンク座標取得
			INT32 pointCnt = mp_GetRoadPoint(p_Data, hRdsp, gFloatBuf);
			if (0 == pointCnt) {
				pInfo += TR_INFO_SIZE(pInfo);
				continue;
			}

			// 縮退三角形連結(フチ)
			MP_GL_DegenerateTriangleShift(gFloatBuf, pointCnt, outLineWHalf, shift, dir, arrow, gPolyLineData, &outLineIdx);

			// 縮退三角形連結(メイン)
			INT32 indexTmp = lineIdx;
			MP_GL_DegenerateTriangleShift(gFloatBuf, pointCnt, lineWHalf, shift, dir, arrow, gTrianglePoint, &lineIdx);
			INT32 getCnt = lineIdx - indexTmp;
			for (INT32 j=0; j<getCnt; j++) {
				gTrianglePointColor[indexTmp+j] = *(T_Color*)&cngsColor;
			}

			// ポリライン描画
			if (outLineIdx > MP_VERTEX_BUFFER_MAX) {
				MP_GL_DrawPolyLine(gPolyLineData, outLineIdx);
				outLineIdx = 0;
			}
			if (lineIdx > MP_VERTEX_BUFFER_MAX) {
				MP_GL_DrawPolyLineColor(gTrianglePoint, gTrianglePointColor, lineIdx);
				lineIdx = 0;
			}

			// 次へ
			pInfo += TR_INFO_SIZE(pInfo);
		}

		// ポリライン描画
		if (outLineIdx > MP_TRIANGLE_CNT) {
			MP_GL_DrawPolyLine(gPolyLineData, outLineIdx);
		}
		if (lineIdx > MP_TRIANGLE_CNT) {
			MP_GL_DrawPolyLineColor(gTrianglePoint, gTrianglePointColor, lineIdx);
		}
	}

	return (true);
}

static INT32 mp_GetRoadPoint(T_PARCEL_DATA* p_Data, RDSP_HDL hRdsp, T_POINT *pPoint)
{
	T_VIEW_INFO* pVi = NULL;
	INT32 drawPointCnt = 0;
	T_POINT tmp;
	bool no_flg = true;
	DOUBLE lat;
	DOUBLE lon;

	pVi = MP_GetViewInfo();

	FLOAT keisuuX = p_Data->ParcelInfo.keisuuX;
	FLOAT keisuuY = p_Data->ParcelInfo.keisuuY;

	// 点数取得
	UINT16 pointCnt = GET_LKSP_POINT_CNT(hRdsp);

	// 形状先頭アドレス取得
	UINT16* pShape = GET_LKSP_POINT_P(hRdsp);

	for (UINT32 i=0; i<pointCnt; i++) {
		UINT16 x = *(pShape);
		UINT16 y = *(pShape+1);

		if (MP_LEVEL_5 <= p_Data->level) {
			MESHC_ChgParcelIDToLatLon(p_Data->level, p_Data->parcel_id, x, y, &lat, &lon);
			mp_ChangeFromLatLonToDisp(lat, lon, tmp.x, tmp.y);
			tmp.x = tmp.x - p_Data->ParcelInfo.pixX;
			tmp.y = tmp.y - p_Data->ParcelInfo.pixY;
		} else {
			tmp.x = (x * keisuuX);
			tmp.y = (y * keisuuY);
		}

		if (no_flg) {
			if (tmp.x >= p_Data->ParcelInfo.minX && tmp.x <= p_Data->ParcelInfo.maxX) {
				if (tmp.y >= p_Data->ParcelInfo.minY && tmp.y <= p_Data->ParcelInfo.maxY) {
					no_flg = false;
				}
			}
		}

		pPoint[drawPointCnt].x = tmp.x;
		pPoint[drawPointCnt].y = tmp.y;

		drawPointCnt++;
		pShape += 2;
	}

	if (no_flg) {
		if (pVi->level <= MP_LEVEL_2) {
			no_flg = mp_DispLineCheck(p_Data, &pPoint[0], &pPoint[drawPointCnt-1]);
		}
	}

	if (no_flg) {
		drawPointCnt = 0;
	}

	return (drawPointCnt);
}

static UINT32 mp_SetRoadNameData(T_PARCEL_DATA* p_Data, RDSP_HDL rdspHdl, UINT16 roadKind, T_POINT* pPoint, INT32 pointCnt, UINT32 beforRouteID)
{
	UINT32 route_id = beforRouteID;
	T_VIEW_INFO* pVi = NULL;

	pVi = MP_GetViewInfo();

	do {
		if (pointCnt <= 2 || pointCnt >= 7) {
			break;
		}

		BA_XX_INFO XX_info;
		XX_info.d = GET_LKSP_XX_INFO(rdspHdl);

		if (!XX_info.b.route_info_flg) {
			break;
		}

		ROUT_HDL h_rout = p_Data->p_RoadShapeAnalyze->GetRouteHdl(rdspHdl);
		if (NULL == h_rout) {
			break;
		}

		UINT16 route_cnt = GET_ROUT_CNT(h_rout);
		if (0 == route_cnt) {
			break;
		}

		route_id = GET_ROUT_ID(h_rout, 0);
		//if (beforRouteID == route_id) {
		//	break;
		//}

		UINT32 route_name_ofs = GET_ROUT_OFS(h_rout, 0);
		if (MP_INVALID_VALUE_32 == route_name_ofs) {
			break;
		}

		if (p_Data->roadNameInfoCnt < MP_ROAD_NAME_CNT_MAX) {
			// 角度算出
			FLOAT angle = MP_GL_Degree(&pPoint[0], &pPoint[1]);

			// 画面回転を加味した角度算出
			FLOAT dispAngle = angle + pVi->rotate;
			if (dispAngle >= MP_ANGLE_360) {
				dispAngle -= MP_ANGLE_360;
			}

			p_Data->roadNameInfo[p_Data->roadNameInfoCnt].pos.x = gFloatBuf[0].x;
			p_Data->roadNameInfo[p_Data->roadNameInfoCnt].pos.y = gFloatBuf[0].y;
			p_Data->roadNameInfo[p_Data->roadNameInfoCnt].endPos.x = gFloatBuf[pointCnt/2].x;
			p_Data->roadNameInfo[p_Data->roadNameInfoCnt].endPos.y = gFloatBuf[pointCnt/2].y;
			p_Data->roadNameInfo[p_Data->roadNameInfoCnt].angle = angle;
			p_Data->roadNameInfo[p_Data->roadNameInfoCnt].ofs = route_name_ofs * 4;
			p_Data->roadNameInfo[p_Data->roadNameInfoCnt].roadKind = roadKind;
			p_Data->roadNameInfo[p_Data->roadNameInfoCnt].turnOver =  MP_GL_IS_TURNOVER(dispAngle);
			p_Data->roadNameInfoCnt++;
		}

	} while (0);

	return (route_id);
}

static bool mp_DrawBkgdData(T_PARCEL_DATA* p_Data)
{
	T_VIEW_INFO* pVi = NULL;

	static const UINT16 linePolygonFlg[MP_BKGD_KIND_CODE_TYPE_MAX] = {
		MP_SHAPE_TYPE_POLYGON,						// 地表面(面)
		MP_SHAPE_TYPE_POLYGON,						// 等高線(面)
		MP_SHAPE_TYPE_POLYGON,						// 大字界(面)
		MP_SHAPE_TYPE_POLYGON,						// 敷地(面)
		MP_SHAPE_TYPE_POLYGON | MP_SHAPE_TYPE_LINE,	// 水系(面,線)
		MP_SHAPE_TYPE_POLYGON,						// 駐車場(面)
		MP_SHAPE_TYPE_POLYGON,						// 橋桁(面)
		MP_SHAPE_TYPE_POLYGON,						// 道路(面)
		MP_SHAPE_TYPE_LINE,							// 道路(線)
		MP_SHAPE_TYPE_LINE,							// 路線(線)
		MP_SHAPE_TYPE_LINE,							// 境界(線)
		MP_SHAPE_TYPE_POLYGON | MP_SHAPE_TYPE_LINE	// 建物(面,線)
	};

	if (p_Data->p_BkgdAnalyze == NULL) {
		return (true);
	}

	pVi = MP_GetViewInfo();

	// 背景種別コード（分類ｺｰﾄﾞ）分ループ
	for (UINT16 bkgdKindCodeType = 0; bkgdKindCodeType < MP_BKGD_KIND_CODE_TYPE_MAX; bkgdKindCodeType++) {
		// 面描画
		if (linePolygonFlg[bkgdKindCodeType]&MP_SHAPE_TYPE_POLYGON) {
			if ((MP_BKGD_KIND_CODE_TYPE_SITE == bkgdKindCodeType && pVi->scroll_mode) ||
				(MP_BKGD_KIND_CODE_TYPE_BUILDING == bkgdKindCodeType && pVi->scroll_mode)) {
			} else {
				mp_SetBkgdVBOPolygon(p_Data, bkgdKindCodeType);
			}

			if (MP_BKGD_KIND_CODE_TYPE_SITE == bkgdKindCodeType) {
				MP_GL_BeginBlend();
			}
			mp_DrawBkgdVBOPolygon(p_Data, bkgdKindCodeType);
			if (MP_BKGD_KIND_CODE_TYPE_SITE == bkgdKindCodeType) {
				MP_GL_EndBlend();
			}
		}

		// 線描画
		if (linePolygonFlg[bkgdKindCodeType]&MP_SHAPE_TYPE_LINE) {
			mp_DrawBkgdLine(p_Data, bkgdKindCodeType);
		}
	}

	return (true);
}

static bool mp_SetBkgdVBOPolygon(T_PARCEL_DATA* p_Data, UINT16 bkgdKindCodeType)
{
	T_VBO_INFO				*pVBOInfo = NULL;	// VBO情報
	T_VBO_INFO_LIST			*pVBOList = NULL;	// VBO情報リスト
	BKGD_HDL				h_bkgd = NULL;		// 背景ハンドル
	BKGD_OBJ_HDL			h_obj = NULL;		// 背景オブジェクトハンドル
	UINT32					bkgd_cnt = 0;		// 背景要素数
	UINT16					obj_cnt = 0;		// オブジェクト数
	RGBACOLOR				bkgd_color;			// 背景カラー
	BA_BKGD_INFO1			info1;				// 背景情報1
	BA_BKGD_INFO2			info2;				// 背景情報2


	// VBO情報リスト取得
	pVBOList = &p_Data->bkgdVBO[bkgdKindCodeType];

	// VBO設定済みかチェック
	if (pVBOList->setFlg) {
		return (true);
	}
	pVBOList->setFlg = true;

	pVBOInfo = &pVBOList->vbo[0];

	// 背景ハンドル取得
#if 1	// ★★★地表・水系暫定★★★
	if (MP_BKGD_KIND_CODE_TYPE_SURFACE == bkgdKindCodeType ||
		MP_BKGD_KIND_CODE_TYPE_WATER == bkgdKindCodeType) {
		bkgd_cnt = p_Data->p_BkgdAnalyze->GetBkgdCnt();
		h_bkgd = p_Data->p_BkgdAnalyze->FirstBkgdHdl();
	} else {
		h_bkgd = p_Data->p_BkgdAnalyze->GetFirstBkgdHdl(bkgdKindCodeType, &bkgd_cnt);
	}
#else
	h_bkgd = p_Data->p_BkgdAnalyze->GetFirstBkgdHdl(bkgdKindCodeType, &bkgd_cnt);
#endif	// ★★★地表・水系暫定★★★

	// 背景要素数分ループ
	for (UINT32 i = 0; i < bkgd_cnt; i++) {
		if (0 == i) {
		} else {
			h_bkgd = GET_BKGD_NEXT_HDL(h_bkgd);
		}

		// オブジェクト数取得
		obj_cnt = GET_BKGD_OBJ_CNT(h_bkgd);

		// オブジェクト数分ループ
		for (UINT16 j=0; j<obj_cnt; j++) {
			if (0 == j) {
				h_obj = GET_BKGDOBJ_FIRST_HDL(h_bkgd);
			} else {
				h_obj = GET_BKGDOBJ_NEXT_HDL(h_obj);
			}

			// 背景情報1,2取得
			info1.d = GET_BKGDOBJ_INFO(h_obj);
			info2.d = GET_BKGDOBJ_SORT_ID(h_obj);

#if 1	// ★★★地表・水系暫定★★★
			if (0 == j) {
				if (info2.b.type_cd != bkgdKindCodeType) {
					break;
				}
			}
#endif	// ★★★地表・水系暫定★★★

			INT32 contour_cnt = 0;

			if (MP_SHAPE_TYPE_POLYGON == info1.b.figure_type) {
				// 面データ
				contour_cnt = mp_GetBkgdPoint(p_Data, h_obj, gContour);
			}
			else if(MP_SHAPE_TYPE_NOPOLYGON == info1.b.figure_type) {
				// 面データ(図形データ無し)
				mp_ChangeXYToDisp(p_Data,           0,           0, gContour[0]);	// 左下
				mp_ChangeXYToDisp(p_Data,           0, MP_MAP_SIZE, gContour[1]);	// 左上
				mp_ChangeXYToDisp(p_Data, MP_MAP_SIZE, MP_MAP_SIZE, gContour[2]);	// 右上
				mp_ChangeXYToDisp(p_Data, MP_MAP_SIZE,           0, gContour[3]);	// 右下
				contour_cnt = 4;
			}
			else {
				// その他
				continue;
			}

			// 背景カラー取得
			bkgd_color = ParamBkgd_Color(info2.b.type_cd, info2.b.kind_cd);

			if (contour_cnt >= MP_TRIANGLE_CNT) {
				INT32 triangleCnt = 0;

				// 三角形分割したポリゴン座標取得
				if (!Triangulation((T_POLYGON_POINT*)gContour, contour_cnt, (T_TRI_POINT*)&gTrianglePoint[pVBOInfo->pointCnt], &triangleCnt)) {
					continue;
				}

				// 頂点カラー設定
				for (INT32 l=0; l<triangleCnt; l++) {
					memcpy(&gTrianglePointColor[pVBOInfo->pointCnt+l].r, &bkgd_color, 4);
				}

				pVBOInfo->pointCnt += triangleCnt;
			}
		}

		if (pVBOInfo->pointCnt > MP_VERTEX_BUFFER_MAX) {
			// VBO設定
			MP_GL_SetVBO(pVBOInfo, &gTrianglePoint[0], &gTrianglePointColor[0]);
			pVBOList->vboCnt++;
			if (pVBOList->vboCnt == MP_VBO_CNT_MAX) {
				// 管理数がMAXに達した場合はそれ以上処理しない
				return (true);
			}
			pVBOInfo = &pVBOList->vbo[pVBOList->vboCnt];
		}
	}

	if (pVBOInfo->pointCnt >= MP_TRIANGLE_CNT) {
		// VBO設定
		MP_GL_SetVBO(pVBOInfo, &gTrianglePoint[0], &gTrianglePointColor[0]);
		pVBOList->vboCnt++;
	}

	return (true);
}

static bool mp_DrawBkgdLine(T_PARCEL_DATA* p_Data, UINT16 bkgdKindCodeType)
{
	BKGD_HDL				h_bkgd = NULL;		// 背景ハンドル
	BKGD_OBJ_HDL			h_obj = NULL;		// 背景オブジェクトハンドル
	UINT32					bkgd_cnt = 0;		// 背景要素数
	UINT16					obj_cnt = 0;		// オブジェクト数
	RGBACOLOR				bkgd_color;			// 背景カラー
	BA_BKGD_INFO1			info1;				// 背景情報1
	BA_BKGD_INFO2			info2;				// 背景情報2
	T_VIEW_INFO				*pVi = NULL;

	pVi = MP_GetViewInfo();

	// 指定背景種別(分類ｺｰﾄﾞ)の背景ハンドル取得
#if 1	// ★★★水系暫定★★★
	if (MP_BKGD_KIND_CODE_TYPE_WATER == bkgdKindCodeType) {
		bkgd_cnt = p_Data->p_BkgdAnalyze->GetBkgdCnt();
		h_bkgd = p_Data->p_BkgdAnalyze->FirstBkgdHdl();
	} else {
		h_bkgd = p_Data->p_BkgdAnalyze->GetFirstBkgdHdl(bkgdKindCodeType, &bkgd_cnt);
	}
#else
	h_bkgd = p_Data->p_BkgdAnalyze->GetFirstBkgdHdl(bkgdKindCodeType, &bkgd_cnt);
#endif	// ★★★水系暫定★★★

	// 背景要素数分ループ
	for (UINT32 i=0; i<bkgd_cnt; i++) {
		if (0 == i) {
		} else {
			h_bkgd = GET_BKGD_NEXT_HDL(h_bkgd);
		}

		// オブジェクト数取得
		obj_cnt = GET_BKGD_OBJ_CNT(h_bkgd);

		// オブジェクト数分ループ
		for (UINT16 j=0; j<obj_cnt; j++) {
			if (0 == j) {
				h_obj = GET_BKGDOBJ_FIRST_HDL(h_bkgd);
			} else {
				h_obj = GET_BKGDOBJ_NEXT_HDL(h_obj);
			}

			// 背景情報1,2取得
			info1.d = GET_BKGDOBJ_INFO(h_obj);
			info2.d = GET_BKGDOBJ_SORT_ID(h_obj);

#if 1	// ★★★水系暫定★★★
			if (MP_BKGD_KIND_CODE_TYPE_WATER == bkgdKindCodeType) {
				if (0 == j) {
					if (info2.b.type_cd != bkgdKindCodeType) {
						break;
					}
				}
			}
#endif	// ★★★水系暫定★★★

			if (MP_SHAPE_TYPE_LINE != info1.b.figure_type) {
				continue;
			}

			// 座標取得
			INT32 contour_cnt = mp_GetBkgdPoint(p_Data, h_obj, gContour);

			// 描画判定
			if (mp_DispPointCheck(p_Data, gContour, contour_cnt)) {
				continue;
			}

			// 背景カラー取得
			bkgd_color = ParamBkgd_Color(info2.b.type_cd, info2.b.kind_cd);
			MP_GL_ColorRGBA(bkgd_color);
#if 0
			// ライン描画
			MP_GL_DrawLineStrip(gContour, contour_cnt, 2.0f);
#else
			// 背景線パラメータ取得
			DRAWPARAM param = ParamBkgdLine_Param(pVi->scale_level, info2.b.type_cd, info2.b.kind_cd);
			FLOAT line_size = (FLOAT)GET_WIDTH(param);
			FLOAT oline_size = (FLOAT)GET_WIDTH_OUT(param);
			FLOAT dotLength = (FLOAT)GET_DOT(param);

			if (MP_LINE_NMR == GET_LINE(param)) {
				// ライン描画
				MP_GL_DrawLineStrip(gContour, contour_cnt, line_size);
			}
			else if (MP_LINE_DOT == GET_LINE(param)) {
				// 破線描画
				// 破線ベース描画
				MP_GL_DrawLines(gContour, contour_cnt, oline_size/pVi->scale);

				// 破線描画
				MP_GL_ColorRGBA(MP_RGBA_WHITE);
				MP_GL_DrawDotLines(gContour, contour_cnt, line_size/pVi->scale, dotLength);
			}
			else {
				// 何もしない
			}
#endif
		}
	}

	return (true);
}

static bool mp_DispPointCheck(T_PARCEL_DATA* p_Data, T_POINT *pPoint, INT32 cnt)
{
	bool no_flg = true;
	INT32 i = 0;
	INT32 maxIdxX = 0;
	INT32 minIdxX = 0;
	INT32 maxIdxY = 0;
	INT32 minIdxY = 0;

	for (i=0; i<cnt; i++) {
		if (pPoint[i].x >= p_Data->ParcelInfo.minX && pPoint[i].x <= p_Data->ParcelInfo.maxX) {
			if (pPoint[i].y >= p_Data->ParcelInfo.minY && pPoint[i].y <= p_Data->ParcelInfo.maxY) {
				no_flg = false;
				break;
			}
		}

		// 最大最小座標取得
		if (pPoint[maxIdxX].x < pPoint[i].x) {
			maxIdxX = i;
		}
		if (pPoint[minIdxX].x > pPoint[i].x) {
			minIdxX = i;
		}
		if (pPoint[maxIdxY].y < pPoint[i].y) {
			maxIdxY = i;
		}
		if (pPoint[minIdxY].y > pPoint[i].y) {
			minIdxY = i;
		}
	}

	if (no_flg) {
		no_flg = mp_DispLineCheck(p_Data, &pPoint[minIdxX], &pPoint[maxIdxX]);
	}
	if (no_flg) {
		no_flg = mp_DispLineCheck(p_Data, &pPoint[minIdxY], &pPoint[maxIdxY]);
	}

	return (no_flg);
}

static INT32 mp_GetBkgdPoint(T_PARCEL_DATA* p_Data, BKGD_OBJ_HDL hObj, T_POINT *pPoint)
{
	UINT16				point_cnt = 0;	// 背景座標点数
	BA_BKGD_POINT_INFO	point_info;		// 背景座標情報
	INT16				x = 0;			// 座標X
	INT16				y = 0;			// 座標Y
	INT8				*pOfs = NULL;	// オフセット座標アドレス
	INT32				contour_cnt = 0;// 座標点数

	// 背景座標点数取得
	point_cnt = GET_BKGDOBJ_POINT_CNT(hObj);

	// 背景座標情報取得
	point_info.d = GET_BKGDOBJ_POINT_INFO(hObj);

	if (MP_1BYTE == point_info.b.data_form) {
		// 1バイトデータ
		x = GET_BKGDOBJ_POINT_X(hObj);
		y = GET_BKGDOBJ_POINT_Y(hObj);

		pOfs = GET_BKGDOBJ_OFS_P(hObj);
		for (UINT16 k = 0; k < point_cnt; k++) {
			if (k != 0) {
				x += *(pOfs);
				y += *(pOfs+1);
				pOfs += 2;
			}

			mp_ChangeXYToDisp(p_Data, x, y, pPoint[contour_cnt]);
			contour_cnt++;
		}
	}
	else {
		// 2バイトデータ
		for (UINT16 k=0; k<point_cnt; k++) {
			//UINT16 penup_flg = GET_BKGDOBJ_PENUPFLG(hObj, k);
			x = GET_BKGDOBJ_POINT_X2(hObj, k);
			y = GET_BKGDOBJ_POINT_Y2(hObj, k);

			mp_ChangeXYToDisp(p_Data, x, y, pPoint[contour_cnt]);
			contour_cnt++;
		}
	}

	return (contour_cnt);
}

static bool mp_DrawBkgdVBOPolygon(T_PARCEL_DATA* p_Data, UINT16 bkgdKindCodeType)
{
	T_VBO_INFO_LIST* pVBOList = NULL;

	pVBOList = &p_Data->bkgdVBO[bkgdKindCodeType];

	if (!pVBOList->setFlg) {
		return (true);
	}

	for (INT32 i=0; i<pVBOList->vboCnt; i++) {
		MP_GL_DrawVBO(&pVBOList->vbo[i]);
	}

	return (true);
}

static bool mp_DrawNameData(T_PARCEL_DATA* p_Data)
{
	T_VIEW_INFO* pVi = NULL;
	//E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	if (p_Data->p_NameAnalyze == NULL) {
		return (true);
	}
	if (p_Data->pNameFontMng == NULL) {
		return (true);
	}

	pVi = MP_GetViewInfo();

	//FLOAT keisuuX = p_Data->ParcelInfo.keisuuX;
	//FLOAT keisuuY = p_Data->ParcelInfo.keisuuY;
	UINT16 zoomFlg = ParamScale_ZoomFlg(pVi->scale_level);
	FLOAT angle = pVi->disp_angle;
	static char str_utf8[256];
	NAME_HDL h_name;

	// 名称数取得
	UINT32 name_cnt = p_Data->p_NameAnalyze->GetNameCnt();

	// 名称数分ループ
	for (UINT16 i=0; i<name_cnt; i++) {
		if (0 == i) {
			h_name = p_Data->p_NameAnalyze->FirstNameHdl();
		} else {
			h_name = GET_NAME_NEXT_HDL(h_name);
		}

		NMLG_HDL h_nmlg = p_Data->p_NameAnalyze->GetNmLgHdl(h_name, 0);
		if (NULL == h_nmlg) {
			continue;
		}

		BA_NAME_INFO1 info1;
		info1.d = GET_NMLG_INFO1(h_nmlg);

		// ズーム許可フラグチェック
		if (!MP_CHACK_ZOOM_FLG(zoomFlg, info1.b.zoom_flg)) {
			continue;
		}

		UINT16 nameType = (GET_NAME_NAME_KIND(h_name) & 0xFFFF0000) >> 16;
		UINT16 nameKind = (GET_NAME_NAME_KIND(h_name) & 0x0000FFFF);

		INT16 x = GET_NMLG_X(h_nmlg);
		INT16 y = GET_NMLG_Y(h_nmlg);
// 指示ポイントオフセットは使用しない
//		x += GET_NMLG_OFS_X(h_nmlg);
//		y += GET_NMLG_OFS_Y(h_nmlg);

		UINT16 utf8_size = GET_NMLG_STR_SIZE(h_nmlg);
		if (0 == utf8_size) {
			continue;
		}

		// 路線番号衝突判定設定
		p_Data->pRoadNoCollisionMesh->setCollision(x, y);

		T_POINT tmp;
		mp_ChangeXYToDisp(p_Data, x, y, tmp);
		if (tmp.x >= p_Data->ParcelInfo.minX && tmp.x <= p_Data->ParcelInfo.maxX) {
			if (tmp.y >= p_Data->ParcelInfo.minY && tmp.y <= p_Data->ParcelInfo.maxY) {
			} else {
				continue;
			}
		} else {
			continue;
		}

		UINT32 textureID = 0;
		FLOAT scale = 1.0f/pVi->scale;
		T_BITMAPFONT* pBitmapFont = NULL;

		pBitmapFont = p_Data->pNameFontMng->get(i);
		if (NULL == pBitmapFont) {
			continue;
		}
		if (pBitmapFont->readFlg) {
			// 取得済み
		}
		else {
			// 未取得
			if (pVi->scroll_mode || pVi->zoom_mode) {
				// スクロールorフリーズーム中は処理しない
				continue;
			}

			// 文字列取得(UTF8)
			char* str_ptr = GET_NMLG_STR(h_nmlg);
			strncpy(str_utf8, str_ptr, utf8_size);
			str_utf8[utf8_size] = '\0';

			// 設定値取得
			T_FONT_INFO fontInfo;
			fontInfo.pStr			= str_utf8;
			fontInfo.color			= ParamName_Color(nameType, nameKind);
			fontInfo.outLineColor	= ParamName_OutLineColor(nameType, nameKind);
			fontInfo.bkgdColor		= ParamName_BkgdColor(nameType, nameKind);
			fontInfo.fontSize		= ParamName_FontSize(pVi->scale_level, nameType, nameKind);
			fontInfo.outLineFlg		= ParamName_OutLine(nameType, nameKind);
			fontInfo.rotation		= 0.0f;
			fontInfo.lineBreak		= true;

			pBitmapFont = p_Data->pNameFontMng->set(i, fontInfo);
		}

		// テキストテクスチャ設定
		if (NULL != pBitmapFont) {
			textureID = MP_TEXTURE_LoadByteArray((char*)pBitmapFont->pData, pBitmapFont->width, pBitmapFont->height);
			if (0 == textureID) {
				continue;
			}

			FLOAT spotX = 0.0f;
			FLOAT spotY = 0.0f;
			if (MP_STR_TYPE_CENTER == info1.b.type) {
				// 重心ポイント
				// 表示位置補正
				spotX = ((FLOAT)pBitmapFont->strWidth / 2) * scale;
				spotY = ((FLOAT)pBitmapFont->strHeight / 2) * scale;
			} else if (MP_STR_TYPE_POINT == info1.b.type) {
				// 指示ポイント
				// 表示位置補正
				// オフセット取得
				INT32 offset = ParamName_Offset(nameType, nameKind);
				switch (offset)
				{
				case MP_OFFSET_TOP:
					spotX = ((FLOAT)pBitmapFont->strWidth / 2) * scale;
					spotY = (FLOAT)(MP_DEFAULT_ICON_HALF_SIZE + pBitmapFont->strHeight) * scale;
					break;
				case MP_OFFSET_BOTTOM:
					spotX = ((FLOAT)pBitmapFont->strWidth / 2) * scale;
					spotY = (FLOAT)(-MP_DEFAULT_ICON_HALF_SIZE) * scale;
					break;
				case MP_OFFSET_RIGHT:
					spotX = (FLOAT)(-MP_DEFAULT_ICON_HALF_SIZE) * scale;
					spotY = ((FLOAT)pBitmapFont->strHeight / 2) * scale;
					break;
				case MP_OFFSET_LEFT:
					spotX = (FLOAT)(MP_DEFAULT_ICON_HALF_SIZE + pBitmapFont->strWidth) * scale;
					spotY = ((FLOAT)pBitmapFont->strHeight / 2) * scale;
					break;
				default:	// MP_OFFSET_CENTER
					spotX = ((FLOAT)pBitmapFont->strWidth / 2) * scale;
					spotY = ((FLOAT)pBitmapFont->strHeight / 2) * scale;
					break;
				}
			}

			MP_TEXTURE_DrawRect(
						tmp.x,
						tmp.y,
						(FLOAT)pBitmapFont->width * scale,
						(FLOAT)pBitmapFont->height * scale,
						spotX,
						spotY,
						angle,
						textureID);

			MP_TEXTURE_Delete(&textureID);
		}
	}

	return (true);
}

static bool mp_DrawMarkData(T_PARCEL_DATA* p_Data)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	T_VIEW_INFO* pVi = NULL;

	if (p_Data->p_MarkAnalyze == NULL) {
		return (true);
	}

	pVi = MP_GetViewInfo();

	//FLOAT keisuuX = p_Data->ParcelInfo.keisuuX;
	//FLOAT keisuuY = p_Data->ParcelInfo.keisuuY;
	UINT16 zoomFlg = ParamScale_ZoomFlg(pVi->scale_level);
	//FLOAT angle = pVi->disp_angle;
	MARK_HDL h_mark;

	// 記号背景数取得
	UINT32 mark_cnt = p_Data->p_MarkAnalyze->GetMarkCnt();

	// 記号背景数分ループ
	for (UINT16 i=0; i<mark_cnt; i++) {
		if (0 == i) {
			h_mark = p_Data->p_MarkAnalyze->FirstMarkHdl();
		} else {
			h_mark = GET_MARK_NEXT_HDL(h_mark);
		}

		BA_MARK_INFO1 info1;
		info1.d = GET_MARK_INFO1(h_mark);

		BA_MARK_INFO2 info2;
		info2.d = GET_MARK_INFO2(h_mark);

		// ズーム許可フラグチェック
		if (!MP_CHACK_ZOOM_FLG(zoomFlg, info2.b.zoom_flg)) {
			continue;
		}

		UINT16 x = GET_MARK_X(h_mark);
		UINT16 y = GET_MARK_Y(h_mark);

		T_POINT tmp;
		mp_ChangeXYToDisp(p_Data, x, y, tmp);
		if (tmp.x >= p_Data->ParcelInfo.minX && tmp.x <= p_Data->ParcelInfo.maxX) {
			if (tmp.y >= p_Data->ParcelInfo.minY && tmp.y <= p_Data->ParcelInfo.maxY) {
			} else {
				continue;
			}
		} else {
			continue;
		}

		// 表示角度
		FLOAT angle;
		if (MP_INVALID_VALUE_16 == GET_MARK_ANGLE(h_mark)) {
			angle = pVi->disp_angle;
		} else if (info1.b.kind_cd != 170) {	// 暫定で種別ｺｰﾄﾞ170以外は角度を見ない
			angle = pVi->disp_angle;
		} else {
			// 10倍の値で格納されているので10で割る
			angle = (FLOAT)GET_MARK_ANGLE(h_mark) / 10.0f;
			angle = mp_GetRealAngle(angle, p_Data->ParcelInfo.yRatio);
		}

		// アイコンID取得
		UINT32 iconID = ParamMark_MapSymbol(info1.b.type_cd, info1.b.kind_cd);
		if (iconID == MP_INVALID_VALUE_32) {
			continue;
		}

		// アイコン表示
		ret = MP_ICON_Draw(tmp.x, tmp.y, angle, 1.0f/pVi->scale, iconID);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_MP, "MP_ICON_Draw, ret=%d, " HERE, ret);
		}
	}

	return (true);
}

static bool mp_DrawRoadNameData(T_PARCEL_DATA* p_Data)
{
	UINT32 route_size = 0;
	char* p_route_str = NULL;
	UINT32 textureID = 0;
	T_VIEW_INFO* pVi = NULL;
	INT32 i = 0;
	RGBACOLOR color;
	RGBACOLOR outColor;
	FLOAT fontSize = 0.0f;
	FLOAT scale = 0.0f;
	//E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	FLOAT angle = 0.0f;

	if (NULL == p_Data->p_RoadNameAnalyze) {
		return (true);
	}

	pVi = MP_GetViewInfo();

	// 拡大率取得
	scale = 1.0f/pVi->scale;

	for (i=0; i<p_Data->roadNameInfoCnt; i++) {

		angle = p_Data->roadNameInfo[i].angle;

		// 道路名称取得
		p_route_str = p_Data->p_RoadNameAnalyze->GetRouteNameFromOffset(p_Data->roadNameInfo[i].ofs, route_size);
		if (route_size == 0) {
			continue;
		}

		// 色取得
		color = ParamRoadName_Color(p_Data->roadNameInfo[i].roadKind);
		outColor = ParamRoadName_OutLineColor(p_Data->roadNameInfo[i].roadKind);
		// フォントサイズ取得
		fontSize = ParamRoadName_FontSize(p_Data->roadNameInfo[i].roadKind);

		T_BITMAPFONT* pBitmapFont = mp_GetRoadNameBitmapFont(p_Data->pRoadNameFontMng, p_Data->roadNameInfo[i].ofs, p_route_str, fontSize, false, color, outColor);
		if (NULL == pBitmapFont) {
			continue;
		}

		// 表示リンクより道路名称が長い場合、道路名称は非表示
		FLOAT link_length = MP_GL_Distance(&p_Data->roadNameInfo[i].pos, &p_Data->roadNameInfo[i].endPos);
		if (((FLOAT)pBitmapFont->strWidth * scale) > link_length) {
			continue;
		}

		// 描画重複設定
		p_Data->pRoadNoCollisionMesh->setCollision(
						p_Data->roadNameInfo[i].pos.x * (1.0f/p_Data->ParcelInfo.keisuuX),
						p_Data->roadNameInfo[i].pos.y * (1.0f/p_Data->ParcelInfo.keisuuY));

		// テクスチャ生成
		textureID = MP_TEXTURE_LoadByteArray((char*)pBitmapFont->pData, pBitmapFont->width, pBitmapFont->height);
		if (textureID) {
			FLOAT spotX = 0.0f;
			// 文字列を反転する場合
			// 始点(スポット)を文字列の終端位置に設定し、180°回転することで文字列を反転させる
			if (p_Data->roadNameInfo[i].turnOver) {
				spotX = (FLOAT)pBitmapFont->strWidth * scale;
				angle += 180.0f;
			}

			// 描画
			MP_TEXTURE_DrawRect(
					p_Data->roadNameInfo[i].pos.x,
					p_Data->roadNameInfo[i].pos.y,
					(FLOAT)pBitmapFont->width * scale,
					(FLOAT)pBitmapFont->height * scale,
					spotX,//0.0f,
					(FLOAT)(pBitmapFont->strHeight * scale) * 0.5f,
					angle,
					textureID);

			// テクスチャ解放
			MP_TEXTURE_Delete(&textureID);
		}
	}

	return (true);
}

static bool mp_DrawRoadNoData(T_PARCEL_DATA* p_Data)
{
	T_VIEW_INFO* pVi = NULL;

	char* pRouteStr = NULL;
	UINT32 textureID = 0;
	FLOAT fontSize = 0.0f;
	FLOAT scale = 0.0f;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	T_BITMAPFONT* pBitmapFont = NULL;
	T_POINT dispPoint;

	if (NULL == p_Data->p_RoadShapeAnalyze) {
		return (true);
	}
	if (NULL == p_Data->p_RoadNameAnalyze) {
		return (true);
	}

	pVi = MP_GetViewInfo();

	// 拡大率取得
	scale = 1.0f/pVi->scale;

	// 取得対象道路種別設定
	UINT16 startRoadKind = MP_ROAD_KIND_CODE_0;
	UINT16 endRoadKind = MP_ROAD_KIND_CODE_5;
	if (MP_LEVEL_4 <= p_Data->level) {
		startRoadKind = MP_ROAD_KIND_CODE_4;
	}

	// 道路種別数分ループ
	for (UINT16 roadkind=startRoadKind; roadkind<=endRoadKind; roadkind++) {

		UINT32 roadKindCnt = 0;

		// 道路形状ハンドル取得
		RDSP_HDL h_rdsp = p_Data->p_RoadShapeAnalyze->GetRoadShapeHdlOfRoadKind(roadkind, roadKindCnt);
		if(NULL == h_rdsp) {
			continue;
		}

		// 道路種別毎のリンク数分ループ
		for (UINT32 i=0; i<roadKindCnt; i++) {
			// 道路形状ハンドル取得
			if (0 != i) {
				h_rdsp = GET_LKSP_NEXT_HDL(h_rdsp);
			}

			// リンク基本情報1取得
			BA_LINK_BASE_INFO1 base_info1;
			base_info1.d = GET_LKSP_LINK_BASE_INFO1(h_rdsp);
			// XX情報
			BA_XX_INFO XX_info;
			XX_info.d = GET_LKSP_XX_INFO(h_rdsp);

			// リンク種別1チェック
			if(MP_LINK_KIND1_NONSEPARATE_MAIN < base_info1.b.link_kind1) {
				continue;
			}
			// 道路路線情報有無フラグチェック
			if (!XX_info.b.route_info_flg) {
				continue;
			}

			// 座標点取得
			UINT16 pointCnt = GET_LKSP_POINT_CNT(h_rdsp);
			if (pointCnt <= 2) {	continue;	}	// 2点以下は表示対象外
			UINT16 x = GET_LKSP_POINT_X(h_rdsp, pointCnt/2);
			UINT16 y = GET_LKSP_POINT_Y(h_rdsp, pointCnt/2);

			// 座標変換
			mp_ChangeXYToDisp(p_Data, x, y, dispPoint);
			if (dispPoint.x >= p_Data->ParcelInfo.minX && dispPoint.x <= p_Data->ParcelInfo.maxX) {
				if (dispPoint.y >= p_Data->ParcelInfo.minY && dispPoint.y <= p_Data->ParcelInfo.maxY) {
				} else {
					continue;
				}
			} else {
				continue;
			}

			// 道路路線情報ハンドル
			ROUT_HDL h_rout = p_Data->p_RoadShapeAnalyze->GetRouteHdl(h_rdsp);
			if (NULL == h_rout) {
				continue;
			}

			// 道路路線情報数
			UINT16 route_cnt = GET_ROUT_CNT(h_rout);
			if (0 == route_cnt) {
				continue;
			}

			UINT32 routeSize = 0;
			UINT32 routeNameOfs = MP_INVALID_VALUE_32;
			for (INT32 j=0; j<route_cnt; j++) {
				routeNameOfs = GET_ROUT_OFS(h_rout, j);
				if (MP_INVALID_VALUE_32 == routeNameOfs) {
					continue;
				}

				pRouteStr = p_Data->p_RoadNameAnalyze->GetRouteNoFromOffset(routeNameOfs*4, routeSize);
				if (0 < routeSize) {
					break;
				}
			}

			if (0 < routeSize) {
				// フォントサイズ取得
				fontSize = ParamRoadName_RoadNoFontSize(roadkind);

				// 描画重複判定
				if (!p_Data->pRoadNoCollisionMesh->setCollision(x, y)) {
					continue;
				}

				// ビットマップフォント取得
				pBitmapFont = mp_GetRoadNameBitmapFont(p_Data->pRoadNameFontMng, routeNameOfs*4, pRouteStr, fontSize, false, MP_RGBA_WHITE, MP_RGBA_NON);
				if (NULL == pBitmapFont) {
					continue;
				}

				// アイコン描画
				ret = MP_ICON_Draw(dispPoint.x, dispPoint.y, pVi->disp_angle, scale, ParamRoadName_IconID(roadkind));
				if (e_SC_RESULT_SUCCESS != ret) {
					SC_LOG_ErrorPrint(SC_TAG_MP, "MP_ICON_Draw, ret=%d, " HERE, ret);
					continue;
				}

				// 路線番号描画
				// テクスチャ生成
				textureID = MP_TEXTURE_LoadByteArray((char*)pBitmapFont->pData, pBitmapFont->width, pBitmapFont->height);
				if (textureID) {
					// 描画
					MP_TEXTURE_DrawRect(
							dispPoint.x,
							dispPoint.y,
							(FLOAT)pBitmapFont->width * scale,
							(FLOAT)pBitmapFont->height * scale,
							(FLOAT)(pBitmapFont->strWidth * scale) * 0.5f,
							(FLOAT)(pBitmapFont->strHeight * scale) * 0.5f,
							pVi->disp_angle,
							textureID);

					// テクスチャ解放
					MP_TEXTURE_Delete(&textureID);
				}
			}
		}
	}

	return (true);
}

static bool mp_DrawBkgdColor(T_PARCEL_DATA* p_Data)
{
	T_VIEW_INFO* pVi = NULL;
	UINT16 seaLandFlg;

	pVi = MP_GetViewInfo();

	// パーセル基本情報有無
	if (NULL == p_Data->Basis.p_data) {
		return (true);
	}

	if (NULL == p_Data->pRdBaseVer) {
		// 地図未ダウンロード
		seaLandFlg = MP_SURFACE_TYPE_NO_DL;
	} else {
		// 地図ダウンロード済み
		// 海陸判定
		seaLandFlg = (UINT16)p_Data->bkgdBaseFlg;
		if (pVi->sea_flg == seaLandFlg) {
			if (p_Data->level != MP_LEVEL_6) {
				return (true);
			}
		}
	}

	// 色設定
	RGBACOLOR color = ParamBkgd_BaseColor(seaLandFlg);
	MP_GL_ColorRGBA(color);

	// 海陸描画
	MP_GL_DrawSquares(0.0f, 0.0f, (MP_MAP_SIZE * p_Data->ParcelInfo.keisuuX), (MP_MAP_SIZE * p_Data->ParcelInfo.keisuuY));

	return (true);
}

static T_BITMAPFONT* mp_GetRoadNameBitmapFont(MP_FontMng *pFontMng, UINT32 offset, char* pStr, FLOAT fontSize, Bool turnOver, RGBACOLOR color, RGBACOLOR outLineColor)
{
	//E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	T_BITMAPFONT *pBitmapFont;
	T_VIEW_INFO* pVi = NULL;
	static char tmpStr[MP_ROAD_NAME_MAX_SIZE+4];

	if (NULL == pFontMng) {
		return (NULL);
	}

	// キー生成
	UINT32 key = offset + (turnOver ? 1 : 0);

	// ビットマップフォント検索
	pBitmapFont = pFontMng->get(key);
	if (NULL != pBitmapFont) {
		return (pBitmapFont);
	}

	pVi = MP_GetViewInfo();

	// スクロールモードorフリーズームモード判定
	if (pVi->scroll_mode || pVi->zoom_mode) {
		return (NULL);
	}

	// フォント情報設定
	T_FONT_INFO fontInfo;
	sprintf(tmpStr, "  %s  ", pStr);
	fontInfo.pStr			= tmpStr;
	fontInfo.fontSize		= fontSize;
	fontInfo.color			= color;
	fontInfo.outLineColor	= outLineColor;
	fontInfo.bkgdColor		= MP_RGBA_NON;
	fontInfo.outLineFlg		= 1;
	if (outLineColor == 0) fontInfo.outLineFlg = 0;
	fontInfo.rotation		= (turnOver ? 180.0f : 0.0f);
	fontInfo.lineBreak		= false;

	// 追加
	pBitmapFont = pFontMng->set(key, fontInfo);

	return (pBitmapFont);
}

static FLOAT mp_GetRealAngle(FLOAT angle, FLOAT yRatio)
{
	FLOAT rAngle = MP_ANGLE_0;
	FLOAT dirX = 0.0f;
	FLOAT dirY = 0.0f;

	// 東方向0°反時計回りで90°以上270°未満の場合はX方向を-にする
	dirX = 1.0f;
	if (angle >= MP_ANGLE_90 && angle < MP_ANGLE_270) {
		dirX *= -1.0f;
	}

	// X方向を1とした時のY方向の値に比率計算
	dirY = dirX * tan(MP_DEG_TO_RAD(angle)) * yRatio;
	// 角度算出
	rAngle = MP_RAD_TO_DEG(atan2(dirY, dirX));
	// 角度を東方向0°反時計回り → 北方向0°時計回りに変換
	rAngle = MP_E0CCW_TO_N0CW(rAngle);

	return (rAngle);
}

#if 0	// デバッグ
static bool mp_DrawRoadNoMesh(T_PARCEL_DATA* p_Data)
{
	T_VIEW_INFO* pVi = NULL;
	T_POINT point[2];

	pVi = MP_GetViewInfo();

	FLOAT divCnt = (FLOAT)MP_MAP_SIZE/(FLOAT)ParamRoadName_DivCnt(pVi->scale);

	// 横線
	for(FLOAT i=0; i<MP_MAP_SIZE; i+=divCnt) {
		mp_ChangeXYToDisp(p_Data,           0, i, point[0]);
		mp_ChangeXYToDisp(p_Data, MP_MAP_SIZE, i, point[1]);
		MP_GL_DrawLines(point, 2, 2.0f*(1.0f/pVi->scale));
	}
	// 縦線
	for(FLOAT i=0; i<MP_MAP_SIZE; i+=divCnt) {
		mp_ChangeXYToDisp(p_Data,    i,           0, point[0]);
		mp_ChangeXYToDisp(p_Data,    i, MP_MAP_SIZE, point[1]);
		MP_GL_DrawLines(point, 2, 2.0f*(1.0f/pVi->scale));
	}
}
#endif
