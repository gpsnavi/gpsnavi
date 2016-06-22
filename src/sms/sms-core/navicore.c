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
 * navicore.c
 *
 *  Created on: 2016/03/14
 *      Author:
 */

#include <smsallinclude.h>	//TODO smsallinclude.hを削除するときに変更する
#include <navicore.h>


// (デバッグ用)ログファイルのパス
#define NC_LOG_FILE_PATH	"Log/SMCore.log"

static Bool mIsInitialized = false;
static INT32 mDrivingDiagnosingStatus = 0;


INT32 NC_Initialize(INT32 iwidth, INT32 iheight, const Char* strRootPath, const Char* strMapPath, const Char* strLocatorPath) {
	INT32 result = NC_SUCCESS;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	E_PAL_RESULT pal_res = e_PAL_RESULT_SUCCESS;
	SMCARSTATE carState = {};
	Char logDirPath[SC_MAX_PATH] = {};
	Char configDirPath[SC_MAX_PATH] = {};

	printf("\n");
	printf("navi-core version %s(" __DATE__ ") , api version %s\n",CORE_VERSION,API_VERSION);
	printf("Copyright (c) 2016  Hitachi, Ltd.\n");
	printf("This program is dual licensed under GPL version 2 or a commercial license.\n");
	printf("\n");

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	do {

		// パラメータチェック
		if (NULL == strRootPath) {
			LOG_PRINT_ERROR((char*)SC_TAG_NC, "param error[strRootPath]");
			result = NC_PARAM_ERROR;
			break;
		}
		if (NULL == strMapPath) {
			LOG_PRINT_ERROR((char*)SC_TAG_NC, "param error[strMapPath]");
			result = NC_PARAM_ERROR;
			break;
		}
		if (NULL == strLocatorPath) {
			LOG_PRINT_ERROR((char*)SC_TAG_NC, "param error[strLocatorPath]");
			result = NC_PARAM_ERROR;
			break;
		}

		// 各パス設定
		sprintf((char*) logDirPath, "%s%s", strRootPath, NC_LOG_FILE_PATH);
		sprintf((char*) configDirPath, "%sConfig/", strMapPath);

		// ログ初期化
		ret = SC_LOG_Initialize(SC_LOG_TYPE_BOTH, SC_LOG_LV_DEBUG, (Char*) logDirPath);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint((char*) SC_TAG_NC, "SC_LOG_Initialize error, " HERE);
			result = NC_ERROR;
			break;
		}

		// ＰＯＩDB初期化
		pal_res = SC_POI_Initialize((char*) strMapPath);
		if (e_PAL_RESULT_SUCCESS != pal_res) {
			SC_LOG_ErrorPrint((char*) SC_TAG_NC, "SC_POI_Initialize error, " HERE);
			result = NC_ERROR;
			break;
		}

		// コア初期化
		ret = SC_MNG_Initialize(strRootPath, configDirPath, strMapPath);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "SC_MNG_Initialize error(0x%08x), " HERE, ret);
			result = NC_ERROR;
			break;
		}

		// 解像度設定
		ret = SC_MNG_SetResolution((INT32) iwidth, (INT32) iheight);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "SC_MNG_SetResolution error(0x%08x), " HERE, ret);
			result = NC_ERROR;
			break;
		}

		// 車両状態情報を取得
		ret = SC_MNG_GetCarState(&carState, e_SC_CARLOCATION_REAL);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "SC_MNG_GetCarState error(0x%08x), " HERE, ret);
			result = NC_ERROR;
			break;
		}
#if 0	// kanagawa
		// ロケータ初期化
		ret = LC_InitLocator(strLocatorPath, &carState);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "LC_InitLocator error(0x%08x), " HERE, ret);
			result = NC_ERROR;
			break;
		}
#endif	// kanagawa
	} while (0);

	// 運転特性診断停止中
	mDrivingDiagnosingStatus = 0;

	// NaviCore初期化完了
	mIsInitialized = true;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);

	return (result);
}

Bool NC_IsInitialized() {
	return (mIsInitialized);
}

void NC_Finalize() {
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	// エラーでも処理は継続する

	// コア終了化
	ret = SC_MNG_Finalize();
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint((char*) SC_TAG_NC, "SC_MNG_Finalize error(0x%08x), " HERE, ret);
	}

	// ログ終了化
	SC_LOG_Finalize();

	mIsInitialized = false;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);

	return;
}

INT32 NC_LC_SetLocationUpdateCB(NC_LOCATORCBFUNCPTR pfunc) {
	INT32 result = NC_SUCCESS;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == pfunc) {
			SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "pfunc NULL, " HERE);
			result = NC_ERROR;
			break;
		}

		// ロケーション更新CB登録
		ret = LC_SetLocationUpdateCallback(pfunc);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "LC_SetLocationUpdateCallback error(0x%08x), " HERE, ret);
			result = NC_ERROR;
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);

	return (result);
}

INT32 NC_MP_SetBitmapFontCB(NC_BITMAPFONTFUNCPTR pfunc) {
	INT32 result = NC_SUCCESS;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == pfunc) {
			SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "pfunc NULL, " HERE);
			result = NC_ERROR;
			break;
		}

		// ビットマップフォント生成処理CB登録
		ret = SC_DRAW_SetBitmapFontCB(pfunc);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "SC_DRAW_SetBitmapFontCB error(0x%08x), " HERE, ret);
			result = NC_ERROR;
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);

	return (result);
}

INT32 NC_MP_SetImageReadForFileCB(NC_IMAGEFUNCPTR pfunc) {
	INT32 result = NC_SUCCESS;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == pfunc) {
			SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "pfunc NULL, " HERE);
			result = NC_ERROR;
			break;
		}

		// 画像リード処理CB登録
		ret = SC_DRAW_SetImageReadForFileCB(pfunc);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "NC_MP_SetImageReadForFileCB error(0x%08x), " HERE, ret);
			result = NC_ERROR;
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);

	return (result);
}

INT32 NC_MP_SetImageReadForImageCB(NC_IMAGEFUNCPTR pfunc) {
	INT32 result = NC_SUCCESS;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == pfunc) {
			SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "pfunc NULL, " HERE);
			result = NC_ERROR;
			break;
		}

		// 画像リード処理CB登録
		ret = SC_DRAW_SetImageReadForImageCB(pfunc);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "NC_MP_SetImageReadForImageCB error(0x%08x), " HERE, ret);
			result = NC_ERROR;
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);

	return (result);
}

INT32 NC_MP_InitResource(INT32 iMaps) {
	INT32 result = NC_SUCCESS;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	ret = SC_MNG_InitResource(iMaps);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "SC_MNG_InitResource error(0x%08x), " HERE, ret);
		result = NC_ERROR;
	}

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);

	return (result);
}

INT32 NC_MP_SetMapScaleLevel(INT32 iMaps, INT32 iScale) {
	INT32 result = NC_SUCCESS;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	// パラメタチェック
	if (iScale < 0 || (SC_MAP_SCALE_LEVEL_MAX + 1) < (INT32) iScale) {
		SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "param error." HERE);
		return (NC_PARAM_ERROR);
	}

	/*** 表示縮尺設定 ***/
	ret = SC_MNG_SetScaleLevel(iMaps,
			iScale,
			SC_DRAW_GetScaleRange(iScale),
			SC_DRAW_GetZoomLevel(iScale));
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "SC_MNG_SetScaleLevel error(0x%08x), " HERE, ret);
		result = NC_ERROR;
	}

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);

	return (result);
}

INT32 NC_MP_GetMapScaleLevel(INT32 iMaps) {
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	INT32 scale = 0;
	FLOAT scaleRange = 0.0f;
	INT32 zoom = 0;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	/*** 表示縮尺取得 ***/
	ret = SC_MNG_GetScaleLevel((INT32) iMaps, &scale, &scaleRange, &zoom);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "SC_MNG_GetScaleLevel error(0x%08x), " HERE, ret);
		scale = NC_ERROR;
	}

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);

	return (scale);
}

INT32 NC_MP_SetMapDispMode(INT32 iMaps, INT32 iDispMode) {
	INT32	result = NC_SUCCESS;
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	do {
		// パラメータチェック
		if ((SC_MDM_HEADUP > iDispMode) || (SC_MDM_BIRDVIEW < iDispMode)) {
			SC_LOG_ErrorPrint(SC_TAG_NC, (Char*)"param error[iDispMode], " HERE);
			result = NC_PARAM_ERROR;
			break;
		}

		/*** 地図表示モード設定 ***/
		ret = SC_MNG_SetDispMode(iMaps, iDispMode);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_NC, (Char*)"SC_MNG_SetDispMode error(0x%08x), " HERE, ret);
			result = NC_ERROR;
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);

	return (result);
}

INT32 NC_MP_GetMapDispMode(INT32 iMaps) {
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	INT32 dispMode = 0;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	/*** 地図表示モード取得 ***/
	ret = SC_MNG_GetDispMode((INT32) iMaps, &dispMode);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "SC_MNG_GetDispMode error(0x%08x), " HERE, ret);
		dispMode = NC_ERROR;
	}

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);

	return (dispMode);
}

INT32 NC_MP_SetMapMoveWithCar(INT32 iMaps, Bool bMoveWithCar) {
	INT32 result = NC_SUCCESS;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	// ドライバーモードかどうかは、共有資産として、どこかに覚えておく

	/*** ドライバーモード設定 ***/
	ret = SC_MNG_SetDriverMode(iMaps, bMoveWithCar);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "SC_MNG_SetDriverMode error(0x%08x), " HERE, ret);
		result = NC_ERROR;
	}

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);

	return (result);
}

INT32 NC_MP_GetMapMoveWithCar(INT32 iMaps) {
	INT32 result = 0;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	Bool bMoveWithCar = false;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	/*** ドライバーモード設定 ***/
	ret = SC_MNG_GetDriverMode((INT32) iMaps, &bMoveWithCar);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "SC_MNG_SetDriverMode error(0x%08x), " HERE, ret);
		result = NC_ERROR;
	} else {
		result = bMoveWithCar;
	}

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);

	return (result);
}

INT32 NC_MP_MoveMapDir(INT32 iMaps, FLOAT fDegreeToward, INT32 iPixelStep) {

	INT32 result = NC_SUCCESS;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	do {
		// パラメータチェック
		if (0 > iPixelStep) {
			SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "param error[iPixelStep](0x%08x), " HERE, ret);
			result = NC_PARAM_ERROR;
			break;
		}

		if (isnan(fDegreeToward)) {
			SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "param error[iPixelStep](0x%08x), " HERE, ret);
			result = NC_ERROR;
			break;
		}

		/*** 地図の移動情報(移動角度と移動長さ)設定 ***/
		ret = SC_MNG_SetMoveMapDir(iMaps, fDegreeToward, iPixelStep);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "SC_MNG_SetMoveMapDir error(0x%08x), " HERE, ret);
			result = NC_ERROR;
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);

	return (result);
}

INT32 NC_MP_SetMapViewport(INT32 iMaps, INT32 left, INT32 top, INT32 right, INT32 bottom) {
	INT32 result = NC_SUCCESS;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	SMRECT rect = {};
	INT32 width = 0;
	INT32 height = 0;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	do {
		/*** 解像度取得 ***/
		ret = SC_MNG_GetResolution((INT32*) &width, (INT32*) &height);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "SC_MNG_GetResolution error(0x%08x), " HERE, ret);
			result = NC_ERROR;
			break;
		}

		// パラメータチェック
		if ((0 > left)/* || (width < left)*/) {
			SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "param error[left], " HERE);
			result = NC_PARAM_ERROR;
			break;
		}
		if ((0 > top)/* || (height < top)*/) {
			SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "param error[top], " HERE);
			result = NC_PARAM_ERROR;
			break;
		}
		if ((0 > right)/* || (width < right)*/|| (left >= right)) {
			SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "param error[right], " HERE);
			result = NC_PARAM_ERROR;
			break;
		}
		if ((0 > bottom)/* || (height < bottom)*/|| (top >= bottom)) {
			SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "param error[bottom], " HERE);
			result = NC_PARAM_ERROR;
			break;
		}

		rect.left = left;
		rect.right = right;
		rect.top = top;
		rect.bottom = bottom;

		/*** 地図のビューポート設定 ***/
		ret = SC_MNG_SetMapViewPort(iMaps, (const SMRECT*) &rect);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "SC_MNG_SetMapViewPort error(0x%08x), " HERE, ret);
			result = NC_ERROR;
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);

	return (result);
}

INT32 NC_MP_SetMapRotate(INT32 iMaps, INT32 iDegree) {
	INT32 result = NC_SUCCESS;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	do {
		// パラメータチェック
		if ((0 > iDegree) || (iDegree > 359)) {
			SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "param error[%d], " HERE, iDegree);
			result = NC_PARAM_ERROR;
			break;
		}

		/*** 地図の回転角度設定 ***/
		ret = SC_MNG_SetMapRotate(iMaps, iDegree);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "SC_MNG_SetMapRotate error(0x%08x), " HERE, ret);
			result = NC_ERROR;
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);

	return (result);
}

INT32 NC_MP_RefreshMap(INT32 iMaps) {
	INT32 result = NC_SUCCESS;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	/*** 地図再描画 ***/
	ret = SC_MNG_RefreshMap(iMaps);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "SC_MNG_RefreshMap error(0x%08x), " HERE, ret);
		result = NC_ERROR;
	}

	// 常駐メモリのデータ出力(デバッグ用)
	//SC_SHARE_OutputAllData();

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);

	return (result);
}

INT32 NC_MP_SetUDIResource(const Char *strPathIconDir, const Char *strPathIconInfo) {
	INT32 result = NC_SUCCESS;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	do {
		if (NULL == strPathIconDir) {
			SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "param error[strPathIconDir], " HERE);
			result = NC_PARAM_ERROR;
			break;
		}
		if (NULL == strPathIconInfo) {
			SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "param error[strPathIconInfo], " HERE);
			result = NC_PARAM_ERROR;
			break;
		}

		/*** ユーザ定義アイコンのリソースと設定ファイルの格納パス設定 ***/
		ret = SC_MNG_SetUDIResourcePath(strPathIconDir, strPathIconInfo);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "SC_MNG_SetUDIResourcePath error(0x%08x), " HERE, ret);
			result = NC_ERROR;
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);

	return (result);
}

INT32 NC_DM_GetCarMoveStatus() {
	INT32 result = CMS_MOVE;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	E_GUIDE_STATUS guideStatus = e_GUIDE_STATUS_STOP;
	SC_DH_SHARE_RPPOINT point = {};

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	// 誘導状態取得
	ret = SC_MNG_GetGuideStatus(&guideStatus);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_GetGuideStatus error(0x%08x), " HERE, ret);
		return (result);
	}

	// 経路地点情報取得
	ret = SC_MNG_GetAllRPPlace(&(point.point[0]), &point.pointNum);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_GetAllRPPlace error(0x%08x), " HERE, ret);
		return (result);
	}

	switch (guideStatus) {
	case e_GUIDE_STATUS_START:
	case e_GUIDE_STATUS_RUN:
	case e_GUIDE_STATUS_RESUME:
//	case e_GUIDE_STATUS_SIMSTART:
//	case e_GUIDE_STATUS_SIMRUM:
//	case e_GUIDE_STATUS_SIMRESUME:

// 目的地未通過
		if (false == point.point[point.pointNum - 1].isPassed) {
			result = CMS_MOVE;		// CMS_MOVE
		} else {
			result = CMS_ARRIVE;	// CMS_ARRIVE
		}
		break;
	case e_GUIDE_STATUS_STOP:
	case e_GUIDE_STATUS_PAUSE:
//	case e_GUIDE_STATUS_SIMEXIT:
//	case e_GUIDE_STATUS_SIMPAUSE:
	default:
		result = CMS_STOP;			// CMS_STOP
		break;
	}

//	SC_LOG_InfoPrint(SC_TAG_RG, (Char*)"SC_DM_GetCarMoveStatus = %d (%d)" HERE, result, status);
	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);

	return (result);

}

Bool NC_DM_IsExistRoute() {
	Bool isExistRoute = false;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	/*** 探索結果有無取得 ***/
	ret = SC_MNG_GetExistRoute((Bool*) &isExistRoute);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_GetExistRoute error(0x%08x), " HERE, ret);
		isExistRoute = false;
	}

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);
	return (isExistRoute);
}

INT32 NC_DM_GetRouteLength() {
	INT32 route_length = 0;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	/*** 探索結果有無取得 ***/
	ret = SC_MNG_GetRouteLength(&route_length);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_GetRouteLength error(0x%08x), " HERE, ret);
		route_length = 0;
	}

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);

	return (route_length);
}

INT32 NC_DM_GetRouteAveTime() {
	INT32 route_avetime = 0;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	/*** 探索結果有無取得 ***/
	ret = SC_MNG_GetRouteAveTime(&route_avetime);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_GetRouteAveTime error(0x%08x), " HERE, ret);
		route_avetime = 0;
	}

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);

	return (route_avetime);
}

INT32 NC_DM_GetRouteHighwayLength() {
	INT32 route_hwaylength = 0;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	/*** 探索結果有無取得 ***/
	ret = SC_MNG_GetRouteHwayLength(&route_hwaylength);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_GetRouteHwayLength error(0x%08x), " HERE, ret);
		route_hwaylength = 0;
	}

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);

	return (route_hwaylength);
}

INT32 NC_DM_GetRouteTollwayLength() {
	INT32 route_tolllength = 0;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	/*** 探索結果有無取得 ***/
	ret = SC_MNG_GetRouteTollLength(&route_tolllength);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_GetRouteTollLength error(0x%08x), " HERE, ret);
		route_tolllength = 0;
	}

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);

	return (route_tolllength);
}

INT32 NC_DM_GetRouteTollFee() {
	INT32 route_tollfee = 0;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	/*** 探索結果有無取得 ***/
	ret = SC_MNG_GetRouteTollFee(&route_tollfee);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_GetRouteTollFee error(0x%08x), " HERE, ret);
		route_tollfee = 0;
	}

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);

	return (route_tollfee);
}

Bool NC_RP_IsPlanning() {
	Bool isPlanning = false;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	/*** 探索探索中かどうか取得 ***/
	ret = SC_MNG_GetPlanning((Bool*) &isPlanning);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_GetPlanning error(0x%08x), " HERE, ret);
		isPlanning = false;
	}

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);

	return (isPlanning);
}

INT32 NC_RP_CancelPlanningRoute() {
	INT32 result = NC_SUCCESS;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	/*** 探索キャンセル ***/
	ret = SC_MNG_CancelPlanningRoute();
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_CancelPlanningRoute error(0x%08x), " HERE, ret);
		result = NC_ERROR;
	}

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);

	return (result);
}

Bool NC_RP_PlanSingleRoute(SMRPPOINT *newPoint, INT32 newPointNum) {
	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	Bool bResult = false;
	E_SC_RESULT result = e_SC_RESULT_SUCCESS;
	E_SC_RESULT saveState = e_SC_RESULT_SUCCESS;

	SMRPPOINT savePoint[SC_SHARE_POINT_NUM] = {};
	SMRPTIPINFO tip = {};
	INT32 savePointNum = 0;

	// 探索失敗登録用デフォルト
	tip.tipClass = EC_NOROUTE;

	do {
		// パラメータチェック
		if (NULL == newPoint) {
			SC_LOG_ErrorPrint(SC_TAG_NC, "newPoint NULL, " HERE);
			SC_MNG_SetRPTip(&tip);
			break;
		}
		//地点数チェック
		if (newPointNum < 2 || SC_CORE_RP_PLACE_MAX < newPointNum) {
			SC_LOG_ErrorPrint(SC_TAG_NC, "reRP list num over, " HERE);
			tip.tipClass = EC_WAYPOINT_MAX;
			SC_MNG_SetRPTip(&tip);
			break;
		}
		// 探索中設定:true
		result = SC_MNG_SetPlanning(true);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_SetPlanning error(0x%08x), " HERE, result);
			SC_MNG_SetRPTip(&tip);
			break;
		}

		// 現在の条件保持
		saveState = SC_MNG_GetAllRPPlace(&savePoint[0], &savePointNum);
		if (e_SC_RESULT_SUCCESS != saveState) {
			SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_GetAllRPPlace error(0x%08x), " HERE, saveState);
		}

		// 出発地、経由地、目的地設定
		result = SC_MNG_SetAllRPPlace(newPoint, newPointNum);
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_SetAllRPPlace error(0x%08x), " HERE, result);
			SC_MNG_SetRPTip(&tip);
			break;
		}

		// 経路探索
		result = SC_MNG_DoRoute(e_ROUTE_SINGLE);
		if (e_SC_RESULT_SUCCESS == result) {
			bResult = true;
		} else {
			SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_DoRoute error(0x%08x), " HERE, result);
			if (e_SC_RESULT_SUCCESS != saveState) {
				break;
			}
			// 探索失敗時は元の探索条件を復帰
			result = SC_MNG_SetAllRPPlace(&savePoint[0], savePointNum);
			if (e_SC_RESULT_SUCCESS != result) {
				SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_SetAllRPPlace error(0x%08x), " HERE, result);
				break;
			}
		}
	} while (0);

	// 探索中設定:false
	result = SC_MNG_SetPlanning(false);
	if (e_SC_RESULT_SUCCESS != result) {
		SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_SetPlanning error(0x%08x), " HERE, result);
	}

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);

	return (bResult);
}

Bool NC_RP_DeleteRouteResult() {
	Bool result = true;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	/*** 経路削除 ***/
	ret = SC_MNG_DeleteRouteResult();
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_DeleteRouteResult error(0x%08x), " HERE, ret);
		result = false;
	}

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);
	return (result);
}

void NC_Guide_StartGuide() {
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	do {
		// 経路誘導開始
		ret = SC_MNG_DoGuide(e_GUIDE_STATUS_START);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_DoGuide (START) error(0x%08x), " HERE, ret);
			break;
		}

		/*** 誘導状態設定 ***/
		ret = SC_MNG_SetGuideStatus(e_GUIDE_STATUS_START);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_SetGuideStatus error(0x%08x), " HERE, ret);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);
	return;
}

INT32 NC_Guide_RunGuide() {
	INT32 result = NC_SUCCESS;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	do {
		// 経路誘導実行
		ret = SC_MNG_DoGuide(e_GUIDE_STATUS_RUN);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_DoGuide (RUN) error(0x%08x), " HERE, ret);
			result = NC_ERROR;
			break;
		}

		/*** 誘導状態設定 ***/
		ret = SC_MNG_SetGuideStatus(e_GUIDE_STATUS_RUN);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_SetGuideStatus error(0x%08x), " HERE, ret);
			result = NC_ERROR;
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);
	return (result);
}

void NC_Guide_StopGuide() {
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	do {
		// 経路誘導終了
		ret = SC_MNG_DoGuide(e_GUIDE_STATUS_STOP);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_DoGuide (STOP) error(0x%08x), " HERE, ret);
			break;
		}

		/*** 誘導状態設定 ***/
		ret = SC_MNG_SetGuideStatus(e_GUIDE_STATUS_STOP);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_SetGuideStatus error(0x%08x), " HERE, ret);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);
	return;
}

void NC_Guide_PauseGuide() {
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	do {
		/*** 誘導状態設定 ***/
		ret = SC_MNG_SetGuideStatus(e_GUIDE_STATUS_PAUSE);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_SetGuideStatus error(0x%08x), " HERE, ret);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);
	return;
}

void NC_Guide_ResumeGuide() {
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	do {
		/*** 誘導状態設定 ***/
		ret = SC_MNG_SetGuideStatus(e_GUIDE_STATUS_RESUME);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_SetGuideStatus error(0x%08x), " HERE, ret);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);
	return;
}

Bool NC_Guide_IsGuiding() {
	Bool status = false;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	E_GUIDE_STATUS guideStatus = e_GUIDE_STATUS_STOP;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	/*** 誘導状態取得 ***/
	ret = SC_MNG_GetGuideStatus(&guideStatus);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_GetGuideStatus error(0x%08x), " HERE, ret);
	} else {
		if (e_GUIDE_STATUS_START == guideStatus || e_GUIDE_STATUS_RUN == guideStatus || e_GUIDE_STATUS_RESUME == guideStatus) {
			// 誘導中
			status = true;
		}
	}

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);
	return (status);
}

Bool NC_Guide_IsGuideStop() {
	Bool status = false;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	E_GUIDE_STATUS guideStatus = e_GUIDE_STATUS_STOP;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	/*** 誘導状態取得 ***/
	ret = SC_MNG_GetGuideStatus(&guideStatus);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_GetGuideStatus error(0x%08x), " HERE, ret);
	} else {
		if (e_GUIDE_STATUS_STOP == guideStatus) {
			// 誘導中
			status = true;
		}
	}

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);
	return (status);
}

Bool NC_Guide_IsGuidePause() {
	Bool status = false;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	E_GUIDE_STATUS guideStatus = e_GUIDE_STATUS_STOP;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	/*** 誘導状態取得 ***/
	ret = SC_MNG_GetGuideStatus(&guideStatus);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_GetGuideStatus error(0x%08x), " HERE, ret);
	} else {
		if (e_GUIDE_STATUS_PAUSE == guideStatus) {
			// 誘導中
			status = true;
		}
	}

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);
	return (status);
}

INT32 NC_Guide_GetRealTimeInfo(SMREALTIMEGUIDEDATA *guide) {
	INT32 result = NC_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	// パラメータチェック
	if (NULL == guide) {
		SC_LOG_ErrorPrint(SC_TAG_NC, "guide NULL, " HERE);
		result = NC_ERROR;
		return (result);
	}

	/*** リアルタイムの案内情報取得 ***/
	E_SC_RESULT ret = SC_MNG_GetRealTimeInfo(guide);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_GetRealTimeInfo error(0x%08x), " HERE, ret);
		result = NC_ERROR;
	}

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);
	return (result);
}

INT32 NC_Simulation_StartSimulation() {

	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	//E_SC_SIMSTATE status;
	//E_SC_SIMULATE simulate;
	INT32 result = NC_RESULT_SIM_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);
	do {

		// シミュレーション開始
		ret = SC_MNG_DoSimulation(e_SIMULATE_STATE_READY);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_DoSimulation (START) error(0x%08x), " HERE, ret);
			break;
		}

		/*** シミュレーション環境設定 ***/
		ret = SC_MNG_SetSimulate(e_SIMULATE_AVAILABLE);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_SetSimulate error(0x%08x), " HERE, ret);
			break;
		}

		/*** シミュレーション状態設定 ***/
		ret = SC_MNG_SetSimulationStatus(e_SIMULATE_STATE_STRAT);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_SetSimulationStatus error(0x%08x), " HERE, ret);
			break;
		}
	} while (0);

	// 開始準備成功
	if (e_SC_RESULT_SUCCESS != ret) {
		result = NC_RESULT_SIM_FAILD;
	} else {
		result = NC_RESULT_SIM_SUCCESS;
	}

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);
	return (result);

}

void NC_Simulation_PauseSimulation() {

	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	//INT32 result = NC_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	/*** シミュレーション状態を一時停止に設定。これ以外は何もしない ***/
	ret = SC_MNG_SetSimulationStatus(e_SIMULATE_STATE_SUSPEND);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_SetSimulationStatus error(0x%08x), " HERE, ret);
	}

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);
	return;
}

INT32 NC_Simulation_ExitSimulation() {

	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	INT32 result = NC_RESULT_SIM_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);
	do {
		// シミュレーション終了
		ret = SC_MNG_DoSimulation(e_SIMULATE_STATE_STOP);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_DoSimulation (EXIT) error(0x%08x), " HERE, ret);
			break;
		}

		// デモ走行の走行進捗度０％に設定

#if 0
		/*** シミュレーション環境設定 ***/
		ret = SC_MNG_SetSimulate(e_SIMULATE_UNAVAILABLE);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_SetSimulate error(0x%08x), " HERE, ret);
			break;
		}
#endif
#if 0
		/*** シミュレーション状態設定 ***/
		ret = SC_MNG_SetSimulationStatus(e_SIMULATE_STATE_STOP);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_SetSimulationStatus error(0x%08x), " HERE, ret);
			break;
		}
#endif

	} while (0);

	// 終了処理成功
	if (e_SC_RESULT_SUCCESS != ret) {
		result = NC_RESULT_SIM_FAILD;
	} else {
		result = NC_RESULT_SIM_SUCCESS;
	}

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);
	return (result);
}

INT32 NC_Simulation_CalcNextPos() {

	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	E_SC_SIMSTATE status;
	//E_SC_SIMULATE simulate;
	E_GUIDE_STATUS guideStatus;
	INT32 result = NC_RESULT_SIM_FAILD;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);
	do {
#if 1
		// 暫定処理：状態取得を取得してシミュレーションの自車位置更新を行うか判断する
		ret = SC_MNG_GetGuideStatus(&guideStatus);
		// 誘導停止状態
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_GetGuideStatus error(0x%08x), " HERE, ret);
			break;
		} else {
			if (e_GUIDE_STATUS_STOP == guideStatus) {
				break;
			}
		}
#endif
		// シミュレーション状態取得
		ret = SC_MNG_GetSimulationStatus(&status);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_GetSimulationStatus error(0x%08x), " HERE, ret);
			break;
		} else {
			// シミュレーション実行状態以外
			if (e_SIMULATE_STATE_STRAT != status) {
				break;
			}

		}

		// シミュレーション実行
		ret = SC_MNG_DoSimulation(e_SIMULATE_STATE_STRAT);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_DoSimulation (RUNNING) error(0x%08x), " HERE, ret);
			break;
		}

		result = NC_RESULT_SIM_SUCCESS;
	} while (0);

	// 開始準備成功
	if (e_SC_RESULT_SUCCESS != ret) {
		result = NC_RESULT_SIM_FAILD;
	}
	return (result);
}

INT32 NC_Simulation_GetPosPercent() {

	INT32 result = NC_SUCCESS;
	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);
	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);
	return (result);
}

void NC_Simulation_SetSpeed(INT32 iSpeed) {

	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);
	SC_LOG_InfoPrint(SC_TAG_NC, "SIMULATION SPEED = %d", (INT32) iSpeed);

	//  シミュレーションステップ距離取得
	ret = SC_MNG_SetSimulationSpeed(iSpeed);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_GetSimulationSpeed error(0x%08x), " HERE, ret);
	}
	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);
	return;

}

Bool NC_Simulation_IsInSimu() {

	Bool result = false;
	E_SC_SIMSTATE status;
	E_SC_RESULT ret;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	// シミュレーション状態取得
	ret = SC_MNG_GetSimulationStatus(&status);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_GetSimulationStatus error(0x%08x), " HERE, ret);
	} else {
		// シミュレーション実行状態
		if (e_SIMULATE_STATE_STRAT == status) {
			result = true;
		} else {
			result = false;
		}
	}

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);
	return (result);
}

Bool NC_Simulation_IsSimuPause() {

	Bool result = false;
	E_SC_SIMSTATE status;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	// シミュレーション状態取得
	ret = SC_MNG_GetSimulationStatus(&status);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_GetSimulationStatus error(0x%08x), " HERE, ret);
	} else {
		// シミュレーション実行状態
		if (e_SIMULATE_STATE_SUSPEND == status) {
			result = true;
		} else {
			result = false;
		}
	}

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);
	return (result);
}

Bool NC_Simulation_IsSimuStop() {

	Bool result = false;
	E_SC_SIMSTATE status;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	// シミュレーション状態取得
	ret = SC_MNG_GetSimulationStatus(&status);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_GetSimulationStatus error(0x%08x), " HERE, ret);
	} else {
		// シミュレーション実行状態
		if (e_SIMULATE_STATE_STOP == status) {
			result = true;
		} else {
			result = false;
		}
	}

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);
	return (result);
}

INT32 NC_Simulation_GetSpeed() {

	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	INT32 speed;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	//  シミュレーションステップ距離取得
	ret = SC_MNG_GetSimulationSpeed(&speed);
	if (e_SC_RESULT_SUCCESS != ret) {
		speed = 0;
		SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_GetSimulationSpeed error(0x%08x), " HERE, ret);
	}

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);
	return (speed);
}

void NC_Simulation_ResumeSimulation() {

	//INT32 result = NC_SUCCESS;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	/*** シミュレーション状態をSTRTに設定。これ以外は何もしない ***/
	ret = SC_MNG_SetSimulationStatus(e_SIMULATE_STATE_STRAT);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_NC, "SC_MNG_SetSimulationStatus error(0x%08x), " HERE, ret);
	}

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);
	return;
}

// --------------------------------------------------------------------------------------------
// 車両状態情報
INT32 NC_DM_GetCarState(SMCARSTATE *carState, INT32 mode) {
	INT32 result = NC_SUCCESS;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == carState) {
			SC_LOG_ErrorPrint(SC_TAG_NC, "param error[carState], " HERE);
			result = NC_PARAM_ERROR;
			break;
		}

		/*** 車両状態情報取得 ***/
		ret = SC_MNG_GetCarState(carState, mode);

		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "SC_MNG_GetCarState error(0x%08x), " HERE, ret);
			result = NC_ERROR;
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);

	return (result);
}

//ユーザ定義ダイナミックアイコンデータを設定
INT32 NC_DM_SetIconInfo(const SMMAPDYNUDI *iconInfo, INT32 iconNum)
{
	INT32 result = NC_SUCCESS;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == iconInfo) {
			SC_LOG_ErrorPrint(SC_TAG_NC, "param error[iconInfo], " HERE);
			result = NC_PARAM_ERROR;
			break;
		}

		/*** ユーザ定義ダイナミックアイコンデータを設定 ***/
		ret = SC_MNG_SetIconInfo(iconInfo, iconNum);

		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "SC_MNG_SetIconInfo error(0x%08x), " HERE, ret);
			result = NC_ERROR;
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);

	return (result);
}

INT32 NC_DM_SetDynamicUDIDisplay(const Bool *dispInfo, INT32 dispNum)
{
	INT32 result = NC_SUCCESS;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == dispInfo) {
			SC_LOG_ErrorPrint(SC_TAG_NC, "param error[dispInfo], " HERE);
			result = NC_PARAM_ERROR;
			break;
		}

		/*** 車両状態情報取得 ***/
		ret = SC_MNG_SetDynamicUDIDisplay(dispInfo, dispNum);

		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "SC_MNG_SetDynamicUDIDisplay error(0x%08x), " HERE, ret);
			result = NC_ERROR;
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);

	return (result);
}

// 表示座標から緯度経度を求める
INT32 NC_MP_ScreenToGeoCode(INT32 maps, INT32 screenX, INT32 screenY, SMGEOCOORD* pGeoCoord)
{
	INT32 result = NC_SUCCESS;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	do {

		/*** 車両状態情報取得 ***/
		ret = SC_DRAW_ScreenToGeoCode(maps,screenX,screenY,pGeoCoord);

		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "SC_DRAW_ScreenToGeoCode error(0x%08x), " HERE, ret);
			result = NC_ERROR;
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);

	return (result);
}

static NC_DRAWENDINFOFUNCPTR drawendFuncForHMI = NULL;

E_SC_RESULT MP_HMI_runMapDrawEndCB(NCDRAWENDINFO *pInfo)
{
	INT32	cbRet;
	if(drawendFuncForHMI == NULL){
		return(e_SC_RESULT_NOENTRY);
	}
	cbRet = (drawendFuncForHMI)(pInfo);
	if(cbRet != 0){
		return(e_SC_RESULT_FAIL);
	}
	return (e_SC_RESULT_SUCCESS);
}

E_SC_RESULT MP_HMI_SetMapDrawEndCB(NC_DRAWENDINFOFUNCPTR pfunc)
{
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	drawendFuncForHMI = pfunc;
	return (ret);
}

E_SC_RESULT SC_HMI_SetMapDrawEndCB(NC_DRAWENDINFOFUNCPTR pfunc)
{
	return (MP_HMI_SetMapDrawEndCB(pfunc));
}

INT32 NC_MP_SetMapDrawEndCB(NC_DRAWENDINFOFUNCPTR pfunc) {
	INT32 result = NC_SUCCESS;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == pfunc) {
			SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "pfunc NULL, " HERE);
			result = NC_ERROR;
			break;
		}

		// 地図描画終了時CB登録
		ret = SC_HMI_SetMapDrawEndCB(pfunc);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_NC, (Char*) "SC_HMI_SetMapDrawEndCB error(0x%08x), " HERE, ret);
			result = NC_ERROR;
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_NC, SC_LOG_END);

	return (result);
}


