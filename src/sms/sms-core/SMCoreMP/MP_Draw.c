/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#include "SMCoreMPInternal.h"

// 表示情報
static T_VIEW_INFO g_vi;

static E_SC_RESULT MP_DRAW_Zoom(INT32 maps);
static E_SC_RESULT MP_DRAW_GetMapRange(INT32 mapRange[]);
static E_SC_RESULT MP_DRAW_GetSeaFlag(INT32* pSeaFlag);
static E_SC_RESULT MP_DRAW_GetDownLoadArea(T_DHC_DOWNLOAD_AREA_NAME *downloadAreaName);


void MP_InitViewInfo(void)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SMCARSTATE	car_state;

	// 表示情報初期化
	memset(&g_vi, 0, sizeof(T_VIEW_INFO));

	// 車両状態初期化
	SC_MNG_GetCarState(&car_state, e_SC_CARLOCATION_REAL);
	car_state.dir = MP_ANGLE_90;
	//car_state.onRoad = true;
	SC_MNG_SetCarState(&car_state, e_SC_CARLOCATION_REAL);

	// 倍率
	g_vi.scale = 1.0f;

	// 収録範囲取得
	ret = MP_DRAW_GetMapRange(g_vi.mapRange);
	if (e_SC_RESULT_SUCCESS != ret) {
		g_vi.mapRange[0] = MP_DEFAULT_MAP_RANGE_T_LAT;
		g_vi.mapRange[1] = MP_DEFAULT_MAP_RANGE_B_LAT;
		g_vi.mapRange[2] = MP_DEFAULT_MAP_RANGE_L_LON;
		g_vi.mapRange[3] = MP_DEFAULT_MAP_RANGE_R_LON;
	}

	// 収録されていないパーセルの背景描画方法取得
	ret = MP_DRAW_GetSeaFlag(&g_vi.sea_flg);
	if (e_SC_RESULT_SUCCESS != ret) {
		g_vi.sea_flg = MP_SURFACE_TYPE_SEA;
	}

	// ダウンロードエリア取得
	ret = MP_DRAW_GetDownLoadArea(&g_vi.downLoadAreaName);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"MP_DRAW_GetDownLoadArea, ret=%d, " HERE, ret);
	}
}

T_VIEW_INFO* MP_GetViewInfo(void)
{
	return (&g_vi);
}

void MP_DRAW_SetVeiwInfo(INT32 maps)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	DOUBLE x = 0.0;
	DOUBLE y = 0.0;
	SMGEOCOORD geoCoord;
	INT32 rotate = 0;
	INT32 dispRang = 0;
	INT32 zoom = 0;

	// ビューポート取得
	ret = SC_MNG_GetMapViewPort(maps, &g_vi.rect);
	if(e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_MNG_GetMapViewPort, ret=%d, " HERE, ret);
	}
	g_vi.origin_x = g_vi.rect.right - g_vi.rect.left;
	g_vi.origin_y = g_vi.rect.bottom - g_vi.rect.top;
	g_vi.origin_half_x = g_vi.origin_x/2;
	g_vi.origin_half_y = g_vi.origin_y/2;

	// 表示縮尺取得
	ret = SC_MNG_GetScaleLevel(maps, &g_vi.scale_level, &g_vi.scale, &zoom);
	if(e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_MNG_GetScaleLevel, ret=%d, " HERE, ret);
	}

	// 画面領域設定
	dispRang = sqrt(g_vi.origin_half_x*g_vi.origin_half_x + g_vi.origin_half_y*g_vi.origin_half_y);
	//dispRang = g_vi.origin_half_x > g_vi.origin_half_y ? g_vi.origin_half_x : g_vi.origin_half_y;
//	g_vi.dispRengXMin = 0;
//	g_vi.dispRengXMax = pVi->origin_x;
//	g_vi.dispRengYMin = 0;
//	g_vi.dispRengYMax = pVi->origin_y;
	FLOAT scaleRange = 1.0f/g_vi.scale;
	g_vi.dispRengXMin = -dispRang*scaleRange;
	g_vi.dispRengXMax = dispRang*scaleRange;
	g_vi.dispRengYMin = -dispRang*scaleRange;
	g_vi.dispRengYMax = dispRang*scaleRange;

	// 地図表示モードを取得
	ret = SC_MNG_GetDispMode(maps, &g_vi.disp_mode);
	if(e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_MNG_GetDispMode, ret=%d, " HERE, ret);
	}

	// 地図中心の地理座標を取得
	ret = SC_MNG_GetMapCursorCoord(maps, &g_vi.geo_coord);
	if(e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_MNG_GetMapCursorCoord, ret=%d, " HERE, ret);
	}

	// ドライバーモード判定
	ret = SC_MNG_GetDriverMode(maps, &g_vi.driver_mode);
	if(e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_MNG_GetDriverMode, ret=%d, " HERE, ret);
	}

	// 車両状態情報
	ret = SC_MNG_GetCarState(&g_vi.car_state, e_SC_CARLOCATION_NOW);
	if(e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_MNG_GetCarState, ret=%d, " HERE, ret);
	}
//	if(g_vi.car_state.onRoad) {
		g_vi.car_angle = MP_ANGLE_360 - g_vi.car_state.dir;
		g_vi.car_angle += MP_ANGLE_90;
		if(MP_ANGLE_360 <= g_vi.car_angle) {
			g_vi.car_angle -= MP_ANGLE_360;
		}
//	}

	// ルート探索有無取得
	ret = SC_MNG_GetExistRoute(&g_vi.route);
	if(e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_MNG_GetExistRoute, ret=%d, " HERE, ret);
	}

	// スクロールモード判定
	ret = SC_MNG_GetScrollMode(&g_vi.scroll_mode);
	if(e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_MNG_GetScrollMode, ret=%d, " HERE, ret);
	}

	// フリーズームモード判定
	ret = SC_MNG_GetZoomMode(&g_vi.zoom_mode);
	if(e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_MNG_GetZoomMode, ret=%d, " HERE, ret);
	}

	// 回転角度
	ret = SC_MNG_GetMapRotate(maps, &rotate);
	if(e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_MNG_GetMapRotate, ret=%d, " HERE, ret);
	}
	g_vi.rotate = (FLOAT)rotate;

	// 表示中心位置設定
	if(g_vi.driver_mode) {
		// ドライバーモード
		g_vi.center_point.la = g_vi.car_state.coord.latitude/1024.0/3600.0;
		g_vi.center_point.lo = g_vi.car_state.coord.longitude/1024.0/3600.0;
//		if(g_vi.car_state.onRoad) {
			if(0 == g_vi.disp_mode) {
				// ヘディングアップ
				g_vi.rotate = g_vi.car_state.dir;
				g_vi.rotate -= MP_ANGLE_90;
				if(g_vi.rotate < MP_ANGLE_0) {
					g_vi.rotate += MP_ANGLE_360;
				}
			}
//		}
	} else {
		// ドライバーモードではない
		g_vi.center_point.la = g_vi.geo_coord.latitude/3600.0/1024.0;
		g_vi.center_point.lo = g_vi.geo_coord.longitude/3600.0/1024.0;
	}

	// ズーム
	ret = MP_DRAW_Zoom(maps);
	if(e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"MP_DRAW_Zoom, ret=%d, " HERE, ret);
	}

	// 中心ピクセル座標計算
	MESHC_ChgLatLonToTilePixelCoord(g_vi.center_point.la, g_vi.center_point.lo, g_vi.zoom_level, &x, &y);
	g_vi.pixelCoord.x = (INT32)x;
	g_vi.pixelCoord.y = (INT32)y;

# if 0
	// ユーザ定義アイコンのリソースと設定ファイルの格納パスを取得
	ret = SC_MNG_GetUDIResourcePath(&g_vi.path_icon_dir, &g_vi.path_icon_info);
	if(e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_MNG_GetUDIResourcePath, ret=%d, " HERE, ret);
	}
#endif

	// ユーザ定義ダイナミックアイコンデータの表示/非表示取得
	ret = SC_MNG_GetDynamicUDIDisplay(g_vi.disp_info, &g_vi.disp_num);
	if(e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_MNG_GetDynamicUDIDisplay, ret=%d, " HERE, ret);
	}

	// ユーザ定義ダイナミックアイコンデータ取得
	ret = SC_MNG_GetIconInfo(g_vi.icon_info, &g_vi.icon_num);
	if(e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_MNG_GetIconInfo, ret=%d, " HERE, ret);
	}

/*	// ペアリング情報取得
	ret = SC_MNG_GetPairingPosInfo(g_vi.pairing_pos, &g_vi.pairing_pos_num);
	if(e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_MNG_GetPairingPosInfo, ret=%d, " HERE, ret);
	}*/

	// 表示角度
	g_vi.disp_angle = 0.0f;
	if (g_vi.driver_mode) {	// ドライバーモード
		if (0 == g_vi.disp_mode) {	// ヘディングアップ
			g_vi.disp_angle = g_vi.car_angle;
		} else {
			g_vi.disp_angle = MP_ANGLE_360 - g_vi.rotate;
		}
	} else {
		g_vi.disp_angle = MP_ANGLE_360 - g_vi.rotate;
	}

	// 中心位置共有メモリに登録
	if(g_vi.driver_mode) {
		geoCoord.latitude	= g_vi.center_point.la * 3600.0 * 1024.0;
		geoCoord.longitude	= g_vi.center_point.lo * 3600.0 * 1024.0;
		ret = SC_MNG_SetMapCursorCoord(maps, &geoCoord);
		if(e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_MNG_SetMapCursorCoord, ret=%d, " HERE, ret);
		}
	}

	// 回転角度設定
	if(g_vi.driver_mode) {
		if(0 == g_vi.disp_mode) {
			ret = SC_MNG_SetMapRotate(maps, g_vi.rotate);
			if (e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_MNG_SetMapRotate, ret=%d, " HERE, ret);
			}
		}
	}

	// デバッグON/OFF取得
	ret = SC_MNG_GetDebugInfoOnSurface(&g_vi.debug_flg);
	if(e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_MNG_GetDebugInfoOnSurface, ret=%d, " HERE, ret);
	}

	// マッピングアラート情報取得
	ret = SC_MNG_GetMappingAlert(&g_vi.mapping_alert);
	if(e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_MNG_GetMappingAlert, ret=%d, " HERE, ret);
	}
	if (g_vi.mapping_alert.x == 0 && g_vi.mapping_alert.y == 0) {
		g_vi.mapping_alert_disp = false;
	} else {
		g_vi.mapping_alert_disp = true;
	}

	// 交通情報設定取得
	ret = SC_MNG_GetTrafficInfo(&g_vi.trfInfo);
	if(e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_MNG_GetTrafficInfo, ret=%d, " HERE, ret);
	}

	// 地図未ダウンロードフラグ初期化
	g_vi.noDLFlg = false;
	// 地図未ダウンロードエリア数初期化
	g_vi.noDLAreaCnt = 0;

#if 0
	// 指定表示位置の中心緯度経度取得
	if(g_vi.driver_mode/* || g_vi.scroll_mode*/) {
		//SMGEOCOORD geoCoord;
		SC_DRAW_ScreenToGeoCode(maps,
				g_vi.rect.right-((g_vi.rect.right+g_vi.rect.left)/2),
				g_vi.rect.bottom-((g_vi.rect.bottom+1000/*g_vi.rect.top*/)/2),
				&g_vi.geo_coord);
		if (!g_vi.scroll_mode) {
		SC_MNG_SetMapCursorCoord(maps, &g_vi.geo_coord);
		}

		g_vi.center_point.la = g_vi.geo_coord.latitude/3600.0/1024.0;
		g_vi.center_point.lo = g_vi.geo_coord.longitude/3600.0/1024.0;

		// 中心ピクセル座標計算
		MESHC_ChgLatLonToTilePixelCoord(g_vi.center_point.la, g_vi.center_point.lo, g_vi.zoom_level, &x, &y);
		g_vi.pixelCoord.x = (INT32)x;
		g_vi.pixelCoord.y = (INT32)y;
	}
#endif
}

E_SC_RESULT MP_DRAW_Zoom(INT32 maps)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	// ズームレベル取得
	g_vi.zoom_level = ParamScale_ZoomLevel(g_vi.scale_level);

	// パーセルレベル取得
	g_vi.level = ParamScale_ParcelLevel(g_vi.scale_level);

	return (ret);
}

Bool MP_DRAW_GetPixelPos(DOUBLE lat, DOUBLE lon, FLOAT* p_x, FLOAT* p_y)
{
	// XX.XXXX形式からピクセル座標算出
	DOUBLE pixelCoord_x;
	DOUBLE pixelCoord_y;

	MESHC_ChgLatLonToTilePixelCoord(lat, lon, g_vi.zoom_level, &pixelCoord_x, &pixelCoord_y);
	*p_x = (FLOAT)((pixelCoord_x - (DOUBLE)g_vi.pixelCoord.x));
	*p_y = (FLOAT)((pixelCoord_y - (DOUBLE)g_vi.pixelCoord.y));

	return (true);
}

void MP_DRAW_ChangeFromDispToLatLon(const INT32 x, const INT32 y, DOUBLE *pLat, DOUBLE *pLon)
{
	T_VIEW_INFO* pVi;

	pVi = MP_GetViewInfo();

	// ★座標
	INT32 center_pixel_x = pVi->pixelCoord.x;
	INT32 center_pixel_y = pVi->pixelCoord.y;
	MESHC_ChgTilePixelCoordToLatLon(
		center_pixel_x + (x),
		center_pixel_y + (y),
		pVi->zoom_level,
		pLat,
		pLon);
	*pLat *= (DOUBLE)MP_SEC_3600;
	*pLon *= (DOUBLE)MP_SEC_3600;
}

void MP_DRAW_GetParcelInfo(const UINT32 parcelID, PARCEL_INFO_t* pParcelInfo)
{
	T_VIEW_INFO* pVi;

	pVi = MP_GetViewInfo();

	pParcelInfo->parcel_id = parcelID;
	pParcelInfo->level = MESHC_GetLevel(parcelID);

	DOUBLE minlat;
	DOUBLE minlon;
	DOUBLE maxlat;
	DOUBLE maxlon;
	MESHC_ChgParcelIDToLatLon(pParcelInfo->level, pParcelInfo->parcel_id,           0,           0, &minlat, &minlon);
	MESHC_ChgParcelIDToLatLon(pParcelInfo->level, pParcelInfo->parcel_id, MP_MAP_SIZE, MP_MAP_SIZE, &maxlat, &maxlon);

	DOUBLE pixX[2];
	DOUBLE pixY[2];
	MESHC_ChgLatLonToTilePixelCoord(minlat/MP_SEC_3600, minlon/MP_SEC_3600, pVi->zoom_level, &pixX[0], &pixY[0]);
	MESHC_ChgLatLonToTilePixelCoord(maxlat/MP_SEC_3600, maxlon/MP_SEC_3600, pVi->zoom_level, &pixX[1], &pixY[1]);

	// パーセルの緯度経度ピクセル座標幅
	DOUBLE xSabun = pixX[1] - pixX[0];
	DOUBLE ySabun = pixY[1] - pixY[0];

	// パーセル左下ピクセル座標
	pParcelInfo->pixX = pixX[0];
	pParcelInfo->pixY = pixY[0];
	// パーセル正規化座標係数
	pParcelInfo->keisuuX = xSabun / (DOUBLE)MP_MAP_SIZE;
	pParcelInfo->keisuuY = ySabun / (DOUBLE)MP_MAP_SIZE;

	// パーセル内描画領域
	pParcelInfo->minX = pVi->dispRengXMin - ((pParcelInfo->pixX - pVi->pixelCoord.x));
	pParcelInfo->minY = pVi->dispRengYMin - ((pParcelInfo->pixY - pVi->pixelCoord.y));
	pParcelInfo->maxX = pParcelInfo->minX + (pVi->dispRengXMax - pVi->dispRengXMin);
	pParcelInfo->maxY = pParcelInfo->minY + (pVi->dispRengYMax - pVi->dispRengYMin);

	pParcelInfo->yRatio = (fabs(ySabun)/fabs(xSabun));
}

Bool MP_DRAW_CheckDrawParcel(PARCEL_INFO_t* pParcelInfo)
{
	T_VIEW_INFO* pVi;
	UINT32 parcelID;
	INT32 level;
	DOUBLE parcel_lat[4];	// パーセル四隅の緯度
	DOUBLE parcel_lon[4];	// パーセル四隅の経度
	DOUBLE screen_lat[4];	// 表示画面四隅の緯度
	DOUBLE screen_lon[4];	// 表示画面四隅の経度

	pVi = MP_GetViewInfo();

	parcelID = pParcelInfo->parcel_id;
	level = pParcelInfo->level;

	// パーセルの緯度経度取得
	MESHC_ChgParcelIDToLatLon(level, parcelID,           0,           0, &parcel_lat[MP_DIR_L_B], &parcel_lon[MP_DIR_L_B]);	// 左下
	MESHC_ChgParcelIDToLatLon(level, parcelID, MP_MAP_SIZE,           0, &parcel_lat[MP_DIR_R_B], &parcel_lon[MP_DIR_R_B]);	// 右下
	MESHC_ChgParcelIDToLatLon(level, parcelID,           0, MP_MAP_SIZE, &parcel_lat[MP_DIR_L_T], &parcel_lon[MP_DIR_L_T]);	// 左上
	MESHC_ChgParcelIDToLatLon(level, parcelID, MP_MAP_SIZE, MP_MAP_SIZE, &parcel_lat[MP_DIR_R_T], &parcel_lon[MP_DIR_R_T]);	// 右上

	// 表示画面四隅の緯度経度取得
	MP_DRAW_ChangeFromDispToLatLon(pVi->dispRengXMin, pVi->dispRengYMax, &screen_lat[MP_DIR_L_B], &screen_lon[MP_DIR_L_B]);	// 左下
	MP_DRAW_ChangeFromDispToLatLon(pVi->dispRengXMax, pVi->dispRengYMax, &screen_lat[MP_DIR_R_B], &screen_lon[MP_DIR_R_B]);	// 右下
	MP_DRAW_ChangeFromDispToLatLon(pVi->dispRengXMin, pVi->dispRengYMin, &screen_lat[MP_DIR_L_T], &screen_lon[MP_DIR_L_T]);	// 左上
	MP_DRAW_ChangeFromDispToLatLon(pVi->dispRengXMax, pVi->dispRengYMin, &screen_lat[MP_DIR_R_T], &screen_lon[MP_DIR_R_T]);	// 右上

	// 表示画面左下緯度よりパーセル右上緯度が小さい
	// 表示画面左下経度よりパーセル右上経度が小さい
	if (screen_lat[MP_DIR_L_B] > parcel_lat[MP_DIR_R_T] ||
	    screen_lon[MP_DIR_L_B] > parcel_lon[MP_DIR_R_T]) {
		return (false);
	}

	// 表示画面右下緯度よりパーセル左上緯度が小さい
	// 表示画面右下経度よりパーセル左上経度が大きい
	if (screen_lat[MP_DIR_R_B] > parcel_lat[MP_DIR_L_T] ||
	    screen_lon[MP_DIR_R_B] < parcel_lon[MP_DIR_L_T]) {
		return (false);
	}

	// 表示画面左上緯度よりパーセル右下緯度が大さい
	// 表示画面左上経度よりパーセル右下経度が小さい
	if (screen_lat[MP_DIR_L_T] < parcel_lat[MP_DIR_R_B] ||
	    screen_lon[MP_DIR_L_T] > parcel_lon[MP_DIR_R_B]) {
		return (false);
	}

	// 表示画面右上緯度よりパーセル左下緯度が大さい
	// 表示画面右上経度よりパーセル左下経度が大さい
	if (screen_lat[MP_DIR_R_T] < parcel_lat[MP_DIR_L_B] ||
	    screen_lon[MP_DIR_R_T] < parcel_lon[MP_DIR_L_B]) {
		return (false);
	}

	return (true);
}

Bool MP_DRAW_CheckMapRange(DOUBLE* latitude, DOUBLE* longitude)
{
	Bool ret = true;
	INT32 lat = (INT32)(*latitude * 3600.0);
	INT32 lon = (INT32)(*longitude * 3600.0);

	if (lat > (g_vi.mapRange[0])) {
		lat = g_vi.mapRange[0];
		ret = false;
	}
	else if(lat < (g_vi.mapRange[1])) {
		lat = (g_vi.mapRange[1]);
		ret = false;
	}

	if (lon > (g_vi.mapRange[3])) {
		lon = g_vi.mapRange[3];
		ret = false;
	}
	else if (lon < (g_vi.mapRange[2])) {
		lon = g_vi.mapRange[2];
		ret = false;
	}

	if (!ret) {
		*latitude = (DOUBLE)lat/3600.0;
		*longitude = (DOUBLE)lon/3600.0;
	}

	return (true);
}

static E_SC_RESULT MP_DRAW_GetMapRange(INT32 mapRange[])
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	char range[32];
	INT32 i = 0;
	INT32 dataPos = 0;
	INT32 rangeCnt = 0;
	INT32 rangeSize = 0;
	char spliter = ':';

	ret = SC_DA_GetSystemMapRangeData(range);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_DA_GetSystemMapRangeData, ret=%d, " HERE, ret);
		return (ret);
	}

	rangeSize = strlen(range);
	for (i=0; i<rangeSize; i++) {
		if ((range[i] == spliter) || (i == rangeSize-1)) {
			if ((range[i] == spliter)) {
				range[i] = '\0';
			}
			mapRange[rangeCnt] = atoi(&range[dataPos]) * 60;
			rangeCnt++;
			dataPos = i+1;
		}
	}

	return (ret);
}

static E_SC_RESULT MP_DRAW_GetSeaFlag(INT32* pSeaFlag)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	char sea_flg[8];

	ret = SC_DA_GetSystemSeaFlagData(sea_flg);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_DA_GetSystemSeaFlag, ret=%d, " HERE, ret);
		return (ret);
	}

	*pSeaFlag = atoi(sea_flg);

	return (ret);
}

static E_SC_RESULT MP_DRAW_GetDownLoadArea(T_DHC_DOWNLOAD_AREA_NAME *downloadAreaName)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_DHC_CASH_RESULT dhcCashRet = e_DHC_RESULT_CASH_SUCCESS;

	dhcCashRet = SC_DHC_GetDownload_AreaName(downloadAreaName);
	if (e_DHC_RESULT_CASH_SUCCESS != dhcCashRet) {
		ret = e_SC_RESULT_FAIL;
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_DHC_GetDownload_AreaName, ret=%d, " HERE, dhcCashRet);
	}

	return (ret);
}
