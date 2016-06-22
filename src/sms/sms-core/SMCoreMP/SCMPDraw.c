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

#if 0
static E_SC_RESULT SCMP_DRAW_Cursor(void);
#endif
static E_SC_RESULT SCMP_DRAW_CarMark(void);
static E_SC_RESULT SCMP_DRAW_DynamicUDI(void);
static E_SC_RESULT SCMP_DRAW_GetFitScreenScaleLevel(DOUBLE centerLat, DOUBLE centerLon, DOUBLE Lat, DOUBLE Lon, INT32 range, UINT16 *pScaleLevel, FLOAT *pScaleRange);
static E_SC_RESULT SCMP_DRAW_GetRouteFarLatLon(DOUBLE centerLat, DOUBLE centerLon, DOUBLE *pFarLat, DOUBLE *pFarLon);
static E_SC_RESULT SCMP_DRAW_MappingAlert(void);

// 初期化
E_SC_RESULT SC_DRAW_Initialize(void)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	LOG_PRINT_START((char*)SC_TAG_MP);

	// パラメータ初期化
	Param_Initialize();

	// 表示情報初期化
	MP_InitViewInfo();

	// 地図描画初期化
	DrawMapInitialize();

	LOG_PRINT_END((char*)SC_TAG_MP);

	return (ret);
}

// 終了
E_SC_RESULT SC_DRAW_Finalize(void)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	LOG_PRINT_START((char*)SC_TAG_MP);

	// アイコンテクスチャ解放
	ret = MP_ICON_Finalize();
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"MP_ICON_Finalize, ret=%d, " HERE, ret);
	}

	// 地図描画終了
	DrawMapFinalize(false);

	LOG_PRINT_END((char*)SC_TAG_MP);

	return (ret);
}

// リソース初期化
E_SC_RESULT SC_DRAW_InitResource(INT32 maps)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	LOG_PRINT_START((char*)SC_TAG_MP);

#ifdef _GLES_2
//	MP_InitEs1Emu();	// kana暫定
#endif	// _GLES_2

	// デバッグ情報初期化
	MP_DEBUG_Initialize();

	// アイコン初期化
	{
		Char path_icon_dir[PATH_SIZE];	// アイコンリソースパス
		Char path_icon_info[PATH_SIZE];	// アイコン設定ファイルパス
		memset(path_icon_dir, 0, sizeof(path_icon_dir));
		memset(path_icon_info, 0, sizeof(path_icon_info));

		// ユーザ定義アイコンのリソースと設定ファイルの格納パスを取得
		ret = SC_MNG_GetUDIResourcePath(path_icon_dir, path_icon_info);
		SC_LOG_DebugPrint(SC_TAG_MP, (const Char*)"★%s,%s", path_icon_dir, path_icon_info);
		if(e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_MNG_GetUDIResourcePath, ret=%d, " HERE, ret);
		}

		// アイコン初期化
		ret = MP_ICON_Initialize(path_icon_dir, path_icon_info);
		if(e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"MP_ICON_Initialize, ret=%d, " HERE, ret);
			MP_DEBUG_SetErrInfo(ERR_TYPE_ICON_INITIALIZE);
		}

		// アイコン無でも動くように暫定
		ret = e_SC_RESULT_SUCCESS;
	}

	// 地図描画初期化
	DrawMapFinalize(true);
	DrawMapInitialize();

	LOG_PRINT_END((char*)SC_TAG_MP);

	return (ret);
}

// ビューポート設定
E_SC_RESULT SC_DRAW_SetViewport(INT32 maps, const SMRECT* rect)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	LOG_PRINT_START((char*)SC_TAG_MP);
	SC_LOG_DebugPrint(SC_TAG_MP, (const Char*)"★SC_DRAW_SetViewport %d,%d,%d,%d", rect->left, rect->top, rect->right, rect->bottom);

	// opengl初期化
	MP_GL_Init();

	// ウィンドウ全体をビューポートにする
	MP_GL_Viewport(rect->left, rect->top, rect->right, rect->bottom);

	LOG_PRINT_END((char*)SC_TAG_MP);

	return (ret);
}

// 再描画
E_SC_RESULT SC_DRAW_Refresh(INT32 maps)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	//SMGEOCOORD geoCoord;
	T_VIEW_INFO* p_vi;

	LOG_PRINT_START((char*)SC_TAG_MP);

#ifdef _COP_MP_DEBUG
	clock_t t1, t2;
	DOUBLE fTime;
	t1 = clock();
#endif // _COP_MP_DEBUG

	// 共有メモリ読み込み
	MP_DRAW_SetVeiwInfo(maps);

	// 表示情報取得
	p_vi = MP_GetViewInfo();

	// 背景色を指定
	RGBACOLOR color;
	if (p_vi->level == MP_LEVEL_6) {
		color = MP_RGBA_BLACK;
	} else {
		color = ParamBkgd_BaseColor(p_vi->sea_flg/*MP_SURFACE_TYPE_SEA*/);
	}
	MP_GL_ClearColor(color);

	// プロジェクション行列の設定
	MP_GL_MatrixProjection();

	// 座標体系設定
	MP_GL_Orthof(0, p_vi->rect.right, p_vi->rect.bottom, 0, -1, 1);

	MP_GL_MatrixModelView();

	MP_GL_PushMatrix();

	// 縮尺、回転
	MP_GL_Translatef(p_vi->origin_half_x, p_vi->origin_half_y, 0.0f);
	MP_GL_Scalef(p_vi->scale, p_vi->scale, p_vi->scale);
	MP_GL_Rotatef(p_vi->rotate, 0.0f, 0.0f, 1.0f);

	// 地図描画
	DrawMap();

	// ダイナミックアイコン描画
	SCMP_DRAW_DynamicUDI();

	// マッピングアラート表示
	SCMP_DRAW_MappingAlert();

	// 自車マーク描画
	SCMP_DRAW_CarMark();

	MP_GL_PopMatrix();

#if 0
#if 1 // kana暫定
	{
		NCDRAWENDINFO draw_info;
		void sample_hmi_draw_compass(FLOAT rotate);
		void sample_hmi_request_update(void);

		sample_hmi_draw_compass(p_vi->rotate);

		sample_hmi_request_update();
	}
#endif // kanagawa
#else
	{
		// 暫定		AIKAWA.AIKAWA
		E_SC_RESULT MP_HMI_runMapDrawEndCB(NCDRAWENDINFO *pInfo);
		NCDRAWENDINFO draw_info;
		draw_info.maps		= NC_MP_MAP_MAIN;
		draw_info.rotate	= p_vi->rotate;
		MP_HMI_runMapDrawEndCB(&draw_info);
	}
#endif

	MP_GL_Flush();

#ifdef _COP_MP_DEBUG
	t2 = clock();
	fTime = (DOUBLE)(t2 - t1)/CLOCKS_PER_SEC;
	SC_LOG_DebugPrint(SC_TAG_MP, (Char*)"★★fTime %f, ", fTime);
#endif // _COP_MP_DEBUG

	// デバッグ情報表示
	if (p_vi->debug_flg) {
		MP_DEBUG_Set1FrameDrawTime(fTime);
		MP_DEBUG_DispInfo();
		MP_GL_Flush();
	}

	// 警告表示
	MP_DEBUG_DispWarning();

	LOG_PRINT_END((char*)SC_TAG_MP);

	return (ret);
}

//
E_SC_RESULT SC_DRAW_SetZoomStepRate(INT32 maps)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	FLOAT stepRate = 1.0f;
	INT32 scaleLevel = 0;
	FLOAT scaleRange = 0.0f;
	INT32 zoomLevel = 0;

	LOG_PRINT_START((char*)SC_TAG_MP);

	do {

		// フリーズーム時の地図拡大比例取得
		ret = SC_MNG_GetZoomStepRate(maps, &stepRate);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_MNG_GetZoomStepRate, ret=%d, " HERE, ret);
			break;
		}

		// スケール取得
		ret = SC_MNG_GetScaleLevel(maps, &scaleLevel, &scaleRange, &zoomLevel);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_MNG_GetScaleLevel, ret=%d, " HERE, ret);
			break;
		}

		if (stepRate < 1.0f) {
			// 拡大
			SC_LOG_DebugPrint(SC_TAG_MP, "ZoomIN%f), " HERE, stepRate);
			scaleRange = scaleRange + (1.0f - stepRate)*(scaleRange);
			if (scaleRange >= ParamScale_ZoomIn(scaleLevel)) {
				if (scaleLevel != MP_SCALE_LEVEL_MIN) {
					scaleLevel--;
					if (1 == scaleLevel)	scaleLevel--;
					scaleRange = ParamScale_Scale(scaleLevel);
				} else {
					scaleRange = ParamScale_ZoomIn(scaleLevel);
				}
			}
			SC_MNG_SetScaleLevel(maps, scaleLevel, scaleRange, ParamScale_ZoomLevel(scaleLevel));
		}
		else if (stepRate > 1.0f) {
			// 縮小
			SC_LOG_DebugPrint(SC_TAG_MP, "ZoomOUT(%f), " HERE, stepRate);
			scaleRange = scaleRange - (stepRate - 1.0f)*(scaleRange);
			if (scaleRange < ParamScale_Scale(scaleLevel)) {
				scaleRange = ParamScale_ZoomOut(scaleLevel);
				if (scaleLevel != MP_SCALE_LEVEL_MAX-1) {
					scaleLevel++;
					if (1 == scaleLevel)	scaleLevel++;
				}
			}
			SC_MNG_SetScaleLevel(maps, scaleLevel, scaleRange, ParamScale_ZoomLevel(scaleLevel));
		}

	} while (0);

	LOG_PRINT_END((char*)SC_TAG_MP);

	return (ret);
}

#if 0
static E_SC_RESULT SCMP_DRAW_Cursor(void)
{
#if 1
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	static T_POINT vertices[] = {
		{0.0f,	6.0f},
		{0.0f,	30.0f}
	};
	INT32 i;
	T_VIEW_INFO* p_vi;

	p_vi = MP_GetViewInfo();

	// ドライバーモード判定
	if (p_vi->driver_mode) {
		return (e_SC_RESULT_SUCCESS);
	}

	MP_GL_PushMatrix();
	MP_GL_LoadIdentity();
	MP_GL_Translatef(p_vi->origin_half_x, p_vi->origin_half_y, 0.0f);

	MP_GL_BeginBlend();
	MP_GL_ColorRGBATrans(MP_RGBA_BLUE, 0.6f);

	for (i=0; i<4; i++) {
		MP_GL_Rotatef(90.0f, 0.0f, 0.0f, 1.0f);
		MP_GL_DrawLines(vertices, 2, 10.0f);
	}

	MP_GL_EndBlend();
	MP_GL_PopMatrix();
#else
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	T_VIEW_INFO* p_vi;

	MP_GL_BeginBlend();

	p_vi = MP_GetViewInfo();

	// アイコン表示
	ret = MP_ICON_Draw(p_vi->origin_half_x, p_vi->origin_half_y, 0.0f, 1.0f, ParamIcon_IconID(MP_ICON_TYPE_CURSOR));
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"MP_ICON_Draw, ret=%d, " HERE, ret);
	}

	MP_GL_EndBlend();
#endif
	return (ret);
}
#endif

static E_SC_RESULT SCMP_DRAW_CarMark(void)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	T_VIEW_INFO* p_vi;
	DOUBLE lat;
	DOUBLE lon;
	FLOAT car_point_x;
	FLOAT car_point_y;

	p_vi = MP_GetViewInfo();

	MP_GL_BeginBlend();

	p_vi = MP_GetViewInfo();

	// 自車位置緯度経度取得
	lat = p_vi->car_state.coord.latitude/1024.0/3600.0;
	lon = p_vi->car_state.coord.longitude/1024.0/3600.0;

	// ピクセル座標算出
	MP_DRAW_GetPixelPos(lat, lon, &car_point_x, &car_point_y);

	// アイコン表示
	ret = MP_ICON_Draw(car_point_x, car_point_y, p_vi->car_angle, (1.0f/p_vi->scale), ParamIcon_IconID(MP_ICON_TYPE_CAR));
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"MP_ICON_Draw, ret=%d, " HERE, ret);
	}

	MP_GL_EndBlend();

	return (ret);
}

static E_SC_RESULT SCMP_DRAW_DynamicUDI(void)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32 i;
	//DOUBLE lat;
	//DOUBLE lon;
	T_VIEW_INFO* p_vi;
	FLOAT pt[2];

	p_vi = MP_GetViewInfo();

	MP_GL_BeginBlend();

	// ダイナミックアイコン数分ループ
	//for (i=0; i<p_vi->icon_num; i++) {
	for (i=p_vi->icon_num-1; i>=0; i--) {

		// 表示有無チェック
		if (!p_vi->disp_info[i]) {
			continue;
		}

		// 表示座標取得
		MP_DRAW_GetPixelPos(
				((DOUBLE)p_vi->icon_info[i].Latitude)/1024.0/3600.0,
				((DOUBLE)p_vi->icon_info[i].Longititude)/1024.0/3600.0,
				&pt[0], &pt[1]);
		if (p_vi->dispRengXMin > pt[0] || p_vi->dispRengXMax < pt[0]) {
			continue;
		}
		if (p_vi->dispRengYMin > pt[1] || p_vi->dispRengYMax < pt[1]) {
			continue;
		}

		// アイコン表示
		ret = MP_ICON_Draw(pt[0], pt[1], p_vi->disp_angle, 1.0f/p_vi->scale, p_vi->icon_info[i].IconID);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"MP_ICON_Draw, ret=%d, " HERE, ret);
		}
	}

	MP_GL_EndBlend();

	return (ret);
}

E_SC_RESULT SC_DRAW_MoveMapDir(INT32 maps)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	DOUBLE x = 0.0;
	DOUBLE y = 0.0;
	DOUBLE radian = 0.0;
//	FLOAT chg_scale = 0.0f;
	FLOAT degree_toward = 0;
	INT32 pixel_step = 0;
	SMGEOCOORD geo_coord;
	IDO_KEIDO_t center_point;
	//PIXEL_COORD_t pixelCoord;
	INT32 scaleLevel = 0;
	FLOAT scaleRange = 0.0f;
	INT32 zoom_level = 0;
	INT32 rotate = 0;
	DOUBLE pcX = 0.0;
	DOUBLE pcY = 0.0;

	LOG_PRINT_START((char*)SC_TAG_MP);

	do {

		// 表示縮尺取得
		ret = SC_MNG_GetScaleLevel(maps, &scaleLevel, &scaleRange, &zoom_level);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_MNG_GetScaleLevel, ret=%d, " HERE, ret);
			break;
		}
		zoom_level = ParamScale_ZoomLevel(scaleLevel);
//		chg_scale = 1.0f / scaleRange;

		// 回転角度取得
		ret = SC_MNG_GetMapRotate(maps, &rotate);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_MNG_GetMapRotate, ret=%d, " HERE, ret);
			break;
		}

		// 地図の移動情報取得
		ret = SC_MNG_GetMoveMapDir(maps, &degree_toward, &pixel_step);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_MNG_GetMoveMapDir, ret=%d, " HERE, ret);
			break;
		}

		// 地図中心の地理座標を取得
		ret = SC_MNG_GetMapCursorCoord(maps, &geo_coord);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_MNG_GetMapCursorCoord, ret=%d, " HERE, ret);
			break;
		}
		center_point.la = (DOUBLE)geo_coord.latitude/3600.0/1024.0;
		center_point.lo = (DOUBLE)geo_coord.longitude/3600.0/1024.0;

		// 中心ピクセル座標計算
		MESHC_ChgLatLonToTilePixelCoord(center_point.la, center_point.lo, zoom_level, &pcX, &pcY);
		//pixelCoord.x = (INT32)pcX;
		//pixelCoord.y = (INT32)pcY;

		// 角度変換
		degree_toward -= (FLOAT)MP_ANGLE_90;
		if (degree_toward < (FLOAT)MP_ANGLE_0) {
			degree_toward += (FLOAT)MP_ANGLE_360;
		}
		degree_toward -= (FLOAT)rotate;
		if (degree_toward < (FLOAT)MP_ANGLE_0) {
			degree_toward += (FLOAT)MP_ANGLE_360;
		}

		radian = (DOUBLE)degree_toward * MP_PI / 180.0;

		// 座標変換
		x = ( (DOUBLE)pixel_step * cos( radian ) );
		y = ( (DOUBLE)pixel_step * sin( radian ) );

//		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"★★★★%f,%f, ", x,y);

		// ピクセル座標と移動座標から中心緯度経度算出
//		if (x != 0 || y != 0) {
			x = round(/*pixelCoord.x*/pcX + (x / scaleRange));
			y = round(/*pixelCoord.y*/pcY + (y / scaleRange));
			MESHC_ChgTilePixelCoordToLatLon((INT32)x, (INT32)y, zoom_level, &center_point.la, &center_point.lo);
//			SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"★★★★%f,%f, ", x,y);
//		}

		MP_DRAW_CheckMapRange(&center_point.la, &center_point.lo);

		// 地図中心の地理座標を登録
		geo_coord.latitude	= (LONG)(center_point.la * 3600.0 * 1024.0);
		geo_coord.longitude = (LONG)(center_point.lo * 3600.0 * 1024.0);
		ret = SC_MNG_SetMapCursorCoord(maps, &geo_coord);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_MNG_SetMapCursorCoord, ret=%d, " HERE, ret);
			break;
		}

	} while (0);

	LOG_PRINT_END((char*)SC_TAG_MP);

	return (ret);
}

E_SC_RESULT SC_DRAW_OverviewMap(INT32 maps, INT32 overviewObj, SMRECT* pRect)
{
	E_SC_RESULT		ret = e_SC_RESULT_SUCCESS;
	Bool			routeFlg = false;
	SMRPPOINT		point[SC_CORE_RP_PLACE_MAX] = {};
	INT32			pointNum = 0;
	INT32			i = 0;
	DOUBLE			centerLat = 0.0;
	DOUBLE			centerLon = 0.0;
	DOUBLE			farLat = 0.0;
	DOUBLE			farLon = 0.0;
	DOUBLE			lat = 0.0;
	DOUBLE			lon = 0.0;
	DOUBLE			startLat = 0.0;
	DOUBLE			startLon = 0.0;
	DOUBLE			endLat = 0.0;
	DOUBLE			endLon = 0.0;
	UINT16			scaleLevel = 0;
	FLOAT			scaleRange = 0.0f;
	SMGEOCOORD		geoCoord = {};
	SMRECT			viewPortRect = {};
	INT32			w = 0;
	INT32			h = 0;
	INT32			range = 0;

	LOG_PRINT_START((char*)SC_TAG_MP);

	do {

		// 画面半径計算
		w = (pRect->right - pRect->left);
		h = (pRect->bottom - pRect->top);
		range = w<h ? w/2 : h/2;
		range -= range * 0.05;			// 5%の余裕を持たせる

		// ルート探索有無取得
		ret = SC_MNG_GetExistRoute(&routeFlg);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_MNG_GetExistRoute, ret=%d, " HERE, ret);
			break;
		}

		if (!routeFlg) {
			break;
		}

		// 地点緯度経度取得
		ret = SC_MNG_GetAllRPPlace(point, &pointNum);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, (Char*)"SC_MNG_GetAllRPPlace error(0x%08x), " HERE, ret);
			break;
		}

		// 目的地と出発値から中間点の緯度経度算出
		for (i=0; i<pointNum; i++) {
			lat = point[i].coord.latitude / 1024.0 / MP_SEC_3600;
			lon = point[i].coord.longitude / 1024.0 / MP_SEC_3600;
			if (LST_START == point[i].rpPointType) {
				// 出発地
				startLat = point[i].coord.latitude / 1024.0 / MP_SEC_3600;
				startLon = point[i].coord.longitude / 1024.0 / MP_SEC_3600;
			}
			else if (LST_DEST == point[i].rpPointType) {
				// 目的地
				endLat = point[i].coord.latitude / 1024.0 / MP_SEC_3600;
				endLon = point[i].coord.longitude / 1024.0 / MP_SEC_3600;
			}
		};
		centerLat = (startLat + endLat)/2.0;
		centerLon = (startLon + endLon)/2.0;

		// 地図中心座標設定
		geoCoord.latitude = centerLat * 1024.0 * MP_SEC_3600;
		geoCoord.longitude = centerLon * 1024.0 * MP_SEC_3600;
		ret = SC_MNG_SetMapCursorCoord(maps, &geoCoord);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, (Char*)"SC_MNG_SetMapCursorCoord error(0x%08x), " HERE, ret);
			break;
		}

		// 中心から経路上一番遠い緯度経度取得
		ret = SCMP_DRAW_GetRouteFarLatLon(centerLat, centerLon, &farLat, &farLon);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, (Char*)"SCMP_DRAW_GetFitScreenScaleLevelRoute(0x%08x), " HERE, ret);
			break;
		}

		// 全経路が表示できる地図表示縮尺(スケール)、表示倍率取得
		ret = SCMP_DRAW_GetFitScreenScaleLevel(centerLat, centerLon, farLat, farLon, range, &scaleLevel, &scaleRange);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, (Char*)"SCMP_DRAW_GetFitScreenScaleLevel(0x%08x), " HERE, ret);
			break;
		}


		// 縮尺設定
		ret = SC_MNG_SetScaleLevel(maps, (INT32)scaleLevel, scaleRange, ParamScale_ZoomLevel(scaleLevel));
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, (Char*)"SC_MNG_SetScaleLevel error(0x%08x), " HERE, ret);
			break;
		}

		// 指定表示位置へ移動
		{
			// ビューポート取得
			ret =  SC_MNG_GetMapViewPort(maps, &viewPortRect);
			if (e_SC_RESULT_SUCCESS  != ret) {
				SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_MNG_GetMapViewPort, ret=%d, " HERE, ret);
				return (false);
			}

			// 指定表示位置の中心緯度経度取得
			ret = SC_DRAW_ScreenToGeoCode(maps, viewPortRect.right-((pRect->right+pRect->left)/2), viewPortRect.bottom-((pRect->bottom+pRect->top)/2), &geoCoord);
			if (e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_ErrorPrint(SC_TAG_CORE, (Char*)"SC_DRAW_ScreenToGeoCode error(0x%08x), " HERE, ret);
				break;
			}

			// 中心位置設定
			ret = SC_MNG_SetMapCursorCoord(maps, &geoCoord);
			if (e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_ErrorPrint(SC_TAG_CORE, (Char*)"SC_MNG_SetMapCursorCoord error(0x%08x), " HERE, ret);
				break;
			}
		}

	} while (0);

	LOG_PRINT_END((char*)SC_TAG_MP);

	return (ret);
}

static E_SC_RESULT SCMP_DRAW_GetRouteFarLatLon(DOUBLE centerLat, DOUBLE centerLon, DOUBLE *pFarLat, DOUBLE *pFarLon)
{
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	INT32 i = 0;
	INT32 j = 0;
	INT32 k = 0;
	INT32 e = 0;
	SC_RP_LinkInfo* p_link_info = NULL;
	SC_RP_FormInfo* p_form_info = NULL;
	SC_RP_SectInfo* p_sect_info = NULL;
	SC_RP_ParcelInfo* p_parcel_info = NULL;
	SC_RP_RouteMng* p_mng = NULL;
	UINT32 parcel_id = MP_INVALID_VALUE_32;
	DOUBLE lat = 0.0;
	DOUBLE lon = 0.0;
	FLOAT x = 0.0f;
	FLOAT y = 0.0f;
	UINT32 route_id = 0;
	UINT32 route_type = 0;
	DOUBLE farLat = 0.0;
	DOUBLE farLon = 0.0;

	do {
		// 探索関数から経路取得
		ret = SC_RP_GetCurrentRouteId(&route_id, &route_type);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_LC, (Char*)"SC_RP_GetCurrentRouteId, ret=%d, " HERE, ret);
			break;
		}
		ret = SC_RP_ReadRouteEntry(route_id, route_type, SC_RP_USER_MAP, &p_mng);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_LC, (Char*)"SC_RP_ReadRouteEntry, ret=%d, " HERE, ret);
			break;
		}

		// 経路情報各先頭アドレス取得
		p_sect_info = p_mng->sectInfo;		// 区間情報先頭アドレス
		p_parcel_info = p_mng->parcelInfo;	// パーセル情報先頭アドレス
		p_link_info = p_mng->linkInfo;		// リンク情報先頭アドレス
		p_form_info = p_mng->formInfo;		// 形状点情報先頭アドレス

		// 区間数分ループ
		for (e = 0; e < p_mng->sectVol; e++, p_sect_info++) {
			p_parcel_info = p_mng->parcelInfo + p_sect_info->parcelIdx;
			p_link_info = p_mng->linkInfo + p_sect_info->linkIdx;
			p_form_info = p_mng->formInfo + p_sect_info->formIdx;

			// パーセル情報数分ループ
			for (k = 0; k < p_sect_info->parcelVol; k++, p_parcel_info++) {

				// パーセルID
				parcel_id = p_parcel_info->parcelId;

				// リンク数分ループ
				for (i = p_parcel_info->linkIdx; i < (p_parcel_info->linkIdx + p_parcel_info->linkVol); i++) {

					// 形状点数分ループ
					for (j = 0; j < p_link_info[i].formVol; j++) {
						x = p_form_info[p_link_info[i].formIdx + j].x;
						y = p_form_info[p_link_info[i].formIdx + j].y;

						// パーセルIDから緯度経度秒計算
						MESHC_ChgParcelIDToLatLon(MESHC_GetLevel(parcel_id), parcel_id, x, y, &lat, &lon);

						// 緯度最大値比較
						if (fabs(farLat) < fabs(centerLat-(lat/3600.0))) {
							farLat = centerLat-(lat/3600.0);
						}
						// 経度最大値比較
						if (fabs(farLon) < fabs(centerLon-(lon/3600.0))) {
							farLon = centerLon-(lon/3600.0);
						}
					}
				}
			}
		}

	} while (0);

	// 探索関数から経路解放
	ret = SC_RP_ReadRouteExit(route_id, route_type, SC_RP_USER_MAP);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_LC, (Char*)"SC_RP_ReadRouteExit, ret=%d, " HERE, ret);
	}

	*pFarLat = centerLat + farLat;
	*pFarLon = centerLon + farLon;

	return (ret);
}

static E_SC_RESULT SCMP_DRAW_GetFitScreenScaleLevel(DOUBLE centerLat, DOUBLE centerLon, DOUBLE Lat, DOUBLE Lon, INT32 range, UINT16 *pScaleLevel, FLOAT *pScaleRange)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	DOUBLE		pcCenterX = 0.0;
	DOUBLE		pcCenterY = 0.0;
	DOUBLE		pcX = 0.0;
	DOUBLE		pcY = 0.0;
	UINT16		zoomLevel = 0;
	UINT16		scaleLevel = MP_SCALE_LEVEL_MIN;
	FLOAT		zoomSta = 1.0f;
	FLOAT		zoomEnd = 1.0f;
	FLOAT		scaleRange = 0.0f;
	//Bool		flg = false;
	FLOAT		distance = 0.0f;
	FLOAT		accu = 0.01;

	// 画面に入りきるズームレベル計算
	for (scaleLevel=MP_SCALE_LEVEL_MIN; scaleLevel<MP_SCALE_LEVEL_MAX-1; scaleLevel++) {
		// ズームレベル取得
		zoomLevel = ParamScale_ZoomLevel(scaleLevel);

		// 画面中心ピクセルコード
		MESHC_ChgLatLonToTilePixelCoord(centerLat, centerLon, zoomLevel, &pcCenterX, &pcCenterY);

		// 対象ピクセルコード
		MESHC_ChgLatLonToTilePixelCoord(Lat, Lon, zoomLevel, &pcX, &pcY);

		// 画面に収まるか判定
		distance = fabsf(sqrtf(((pcCenterX-pcX)*(pcCenterX-pcX)) + ((pcCenterY-pcY)*(pcCenterY-pcY))));
		if (distance < (range * (1.0f/ParamScale_Scale(scaleLevel)))) {
			break;
		}
	}

	zoomEnd = ParamScale_Scale(scaleLevel);
	zoomSta = ParamScale_ZoomIn(scaleLevel) - accu;
	for (scaleRange=zoomSta; scaleRange>zoomEnd; scaleRange-=accu) {
		if (distance < (range * (1.0f/scaleRange))) {
			break;
		}
	}

	*pScaleRange = scaleRange;
	*pScaleLevel = scaleLevel;

	return (ret);
}

FLOAT SC_DRAW_GetScaleRange(INT32 scaleLevel)
{
	return (ParamScale_Scale((UINT16)scaleLevel));
}

INT8 SC_DRAW_GetZoomLevel(INT32 scaleLevel)
{
	return (ParamScale_ZoomLevel((UINT16)scaleLevel));
}

E_SC_RESULT SC_DRAW_ScreenToGeoCode(INT32 maps, INT32 screenX, INT32 screenY, SMGEOCOORD* pGeoCoord)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	DOUBLE lat = 0.0;
	DOUBLE lon = 0.0;
	SMGEOCOORD geo_coord;
	INT32 scaleLevel = 0;
	FLOAT scaleRange = 0.0f;
	INT32 zoom_level = 0;
	INT32 rotate = 0;
	DOUBLE centerPixelX = 0.0;
	DOUBLE centerPixelY = 0.0;
	SMRECT rect;
	DOUBLE radian = 0.0;
	INT32 posTmpX = 0;
	INT32 posTmpY = 0;
	INT32 posX = 0;
	INT32 posY = 0;

	LOG_PRINT_START((char*)SC_TAG_MP);

	do {
		// ビューポート取得
		ret = SC_MNG_GetMapViewPort(maps, &rect);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_MNG_GetMapViewPort, ret=%d, " HERE, ret);
			break;
		}

		// 表示縮尺取得
		ret = SC_MNG_GetScaleLevel(maps, &scaleLevel, &scaleRange, &zoom_level);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_MNG_GetScaleLevel, ret=%d, " HERE, ret);
			break;
		}

		// ズームレベル取得
		zoom_level = ParamScale_ZoomLevel(scaleLevel);
		//chg_scale = 1.0f / scaleRange;

		// 回転角度取得
		ret = SC_MNG_GetMapRotate(maps, &rotate);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_MNG_GetMapRotate, ret=%d, " HERE, ret);
			break;
		}

		// 地図中心ピクセル座標取得
		ret = SC_MNG_GetMapCursorCoord(maps, &geo_coord);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_MNG_GetMapCursorCoord, ret=%d, " HERE, ret);
			break;
		}
		MESHC_ChgLatLonToTilePixelCoord(
			geo_coord.latitude/3600.0/1024.0,
			geo_coord.longitude/3600.0/1024.0,
			zoom_level, &centerPixelX, &centerPixelY);

		// 角度を考慮した画面中心からの座標変換
		radian = (DOUBLE)(MP_ANGLE_360 - rotate) * MP_PI / 180.0;
		posTmpX = (screenX - ((rect.right-rect.left)/2));
		posTmpY = (screenY - ((rect.bottom-rect.top)/2));
		posX = posTmpX*cos(radian) - posTmpY*sin(radian);
		posY = posTmpX*sin(radian) + posTmpY*cos(radian);

		// 指定ピクセル座標の緯度経度取得
		MESHC_ChgTilePixelCoordToLatLon(
				centerPixelX + (posX / scaleRange),
				centerPixelY + (posY / scaleRange),
				zoom_level,
				&lat, &lon);

		pGeoCoord->latitude = (LONG)(lat * 3600.0 * 1024.0);
		pGeoCoord->longitude = (LONG)(lon * 3600.0 * 1024.0);

	} while (0);

	LOG_PRINT_END((char*)SC_TAG_MP);

	return (ret);
}

static E_SC_RESULT SCMP_DRAW_MappingAlert(void)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	T_VIEW_INFO* p_vi;
	T_POINT point[2];
	FLOAT lineWidth = 10.0f;

	p_vi = MP_GetViewInfo();

	// マッピングアラート表示チェック
	if (p_vi->mapping_alert_disp) {
		MP_GL_BeginBlend();

		// アラート地点座標取得
		MP_DRAW_GetPixelPos(
				((DOUBLE)p_vi->mapping_alert.posi.latitude)/1024.0/3600.0,
				((DOUBLE)p_vi->mapping_alert.posi.longitude)/1024.0/3600.0,
				&point[0].x, &point[0].y);

		// アラート拡大表示座標取得
		SMGEOCOORD alertDispPos;
		SC_DRAW_ScreenToGeoCode(1, p_vi->mapping_alert.x, p_vi->mapping_alert.y, &alertDispPos);
		MP_DRAW_GetPixelPos(
				((DOUBLE)alertDispPos.latitude)/1024.0/3600.0,
				((DOUBLE)alertDispPos.longitude)/1024.0/3600.0,
				&point[1].x, &point[1].y);

		// 描画
		MP_GL_ColorRGBA(MP_RGBA_BLACK);
		MP_GL_DrawCircle(point, 32.0f/p_vi->scale, 32, lineWidth/p_vi->scale);
		MP_GL_ColorRGBATrans(MP_RGBA_BLACK, 0.3f);
		MP_GL_DrawLines(point, 2, lineWidth/p_vi->scale);

		// アイコン表示
		ret = MP_ICON_Draw(point[0].x, point[0].y, p_vi->disp_angle, 1.0f/p_vi->scale, p_vi->mapping_alert.udi);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"MP_ICON_Draw, ret=%d, " HERE, ret);
		}

		MP_GL_EndBlend();
	}

	return (ret);
}

E_SC_RESULT SC_DRAW_SetBitmapFontCB(NC_BITMAPFONTFUNCPTR pfunc)
{
	return (MP_FONT_SetBitmapFontCB(pfunc));
}

E_SC_RESULT SC_DRAW_SetImageReadForFileCB(NC_IMAGEFUNCPTR pfunc)
{
	return (MP_PNG_SetImageReadForFileCB(pfunc));
}

E_SC_RESULT SC_DRAW_SetImageReadForImageCB(NC_IMAGEFUNCPTR pfunc)
{
	return (MP_PNG_SetImageReadForImageCB(pfunc));
}

