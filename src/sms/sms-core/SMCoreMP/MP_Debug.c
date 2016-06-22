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

#define MP_DEBUG_FONT_SIZE			24
#define MP_DEBUG_FONT_SIZE_WARNING	30

#define MP_DEBUG_LEFT		0
#define MP_DEBUG_CENTER		1

#define MP_DEBUG_INFO		0
#define MP_DEBUG_WARNING	1

static INT32 mp_DEBUG_DrawString(char* pStr, INT32 x, INT32 y, INT32 pos, INT32 infoType);

static DOUBLE	gFDS = 0.0;
static Bool		gErrInfo[ERR_TYPE_MAX] = {};

static char gErrStr[ERR_TYPE_MAX][PATH_SIZE] = {
	"アイコン生成に失敗しました"
};

static char gOnOff[2][8] = {
	"OFF",
	"ON"
};

void MP_DEBUG_Initialize(void)
{
	gFDS = 0.0;
	memset(gErrInfo, 0, sizeof(gErrInfo));
}

void MP_DEBUG_Set1FrameDrawTime(DOUBLE sec)
{
	gFDS = sec;
}

void MP_DEBUG_SetErrInfo(UINT32 errType)
{
	if (errType >= ERR_TYPE_MAX) {
		return;
	}
	gErrInfo[errType] = true;
}

void MP_DEBUG_DispInfo(void)
{
	T_VIEW_INFO* pVi;
	static char str[256];
	INT32	x_ofs = 10;
	INT32	y_ofs = 300;
	UINT32	myParcelID;
	DOUBLE	myX;
	DOUBLE	myY;
	UINT16	zoomFlg;
	//INT32	i = 0;

	MP_GL_BeginBlend();

	pVi = MP_GetViewInfo();

	// fps
	sprintf(str, "fps[%d, (%f)]", (INT32)(1.0/gFDS), (FLOAT)gFDS);
	mp_DEBUG_DrawString(str, x_ofs, y_ofs, MP_DEBUG_LEFT, MP_DEBUG_INFO);
	y_ofs += MP_DEBUG_FONT_SIZE;

	// 画面サイズ
	sprintf(str, "画面サイズ[%d, %d]", pVi->origin_x, pVi->origin_y);
	mp_DEBUG_DrawString(str, x_ofs, y_ofs, MP_DEBUG_LEFT, MP_DEBUG_INFO);
	y_ofs += MP_DEBUG_FONT_SIZE;

	// パーセルID
	MESHC_ChgLatLonToParcelID(pVi->center_point.la*3600.0, pVi->center_point.lo*3600.0, pVi->level, &myParcelID, &myX, &myY);
	sprintf(str, "パーセルID[%08x, %d, %d]", myParcelID, (INT32)myX, (INT32)myY);
	mp_DEBUG_DrawString(str, x_ofs, y_ofs, MP_DEBUG_LEFT, MP_DEBUG_INFO);
	y_ofs += MP_DEBUG_FONT_SIZE;

	// ズーム許可フラグ
	zoomFlg = ParamScale_ZoomFlg(pVi->scale_level);
	sprintf(str, "ズーム許可フラグ[%d]", zoomFlg);
	mp_DEBUG_DrawString(str, x_ofs, y_ofs, MP_DEBUG_LEFT, MP_DEBUG_INFO);
	y_ofs += MP_DEBUG_FONT_SIZE;

	// ズームレベル
	sprintf(str, "ズームレベル[%d]", pVi->zoom_level);
	mp_DEBUG_DrawString(str, x_ofs, y_ofs, MP_DEBUG_LEFT, MP_DEBUG_INFO);
	y_ofs += MP_DEBUG_FONT_SIZE;

	// 表示縮尺
	sprintf(str, "縮尺[%d, %f]", pVi->scale_level, pVi->scale);
	mp_DEBUG_DrawString(str, x_ofs, y_ofs, MP_DEBUG_LEFT, MP_DEBUG_INFO);
	y_ofs += MP_DEBUG_FONT_SIZE;

	// 回転角度
	sprintf(str, "回転角度[%d]", (INT32)pVi->rotate);
	mp_DEBUG_DrawString(str, x_ofs, y_ofs, MP_DEBUG_LEFT, MP_DEBUG_INFO);
	y_ofs += MP_DEBUG_FONT_SIZE;

	// カーソル位置
	sprintf(str, "カーソル位置[%f, %f]", (FLOAT)pVi->center_point.la, (FLOAT)pVi->center_point.lo);
	mp_DEBUG_DrawString(str, x_ofs, y_ofs, MP_DEBUG_LEFT, MP_DEBUG_INFO);
	y_ofs += MP_DEBUG_FONT_SIZE;

	// 自車位置
	sprintf(str, "自車位置[%f, %f]", (FLOAT)pVi->car_state.coord.latitude/3600.0f/1024.0f, (FLOAT)pVi->car_state.coord.longitude/3600.0f/1024.0f);
	mp_DEBUG_DrawString(str, x_ofs, y_ofs, MP_DEBUG_LEFT, MP_DEBUG_INFO);
	y_ofs += MP_DEBUG_FONT_SIZE;

	// 自車角度
	sprintf(str, "自車角度[%d(N0°CW), %d(E0°CCW:locator)]", (INT32)pVi->car_angle, pVi->car_state.dir);
	mp_DEBUG_DrawString(str, x_ofs, y_ofs, MP_DEBUG_LEFT, MP_DEBUG_INFO);
	y_ofs += MP_DEBUG_FONT_SIZE;

	// マップマッチング 速度
	sprintf(str, "MapMatching[%s], 速度[%dkm/h]", gOnOff[(INT32)pVi->car_state.onRoad], (INT32)((pVi->car_state.speed*3600.0f)/1000.0f));
	mp_DEBUG_DrawString(str, x_ofs, y_ofs, MP_DEBUG_LEFT, MP_DEBUG_INFO);
	y_ofs += MP_DEBUG_FONT_SIZE;

	// スクロールモード
	sprintf(str, "スクロールモード[%s]", gOnOff[(INT32)pVi->scroll_mode]);
	mp_DEBUG_DrawString(str, x_ofs, y_ofs, MP_DEBUG_LEFT, MP_DEBUG_INFO);
	y_ofs += MP_DEBUG_FONT_SIZE;

	// フリーズームモード
	sprintf(str, "フリーズームモード[%s]", gOnOff[(INT32)pVi->zoom_mode]);
	mp_DEBUG_DrawString(str, x_ofs, y_ofs, MP_DEBUG_LEFT, MP_DEBUG_INFO);
	y_ofs += MP_DEBUG_FONT_SIZE;


	MP_GL_EndBlend();
}

void MP_DEBUG_DispWarning(void)
{
	T_VIEW_INFO* pVi;
	static char str[MP_STRING_MAX_LEN];
	INT32	x_ofs = 0;
	INT32	y_ofs = 0;
	INT32	i = 0;
	//Char	*pName = NULL;


	MP_GL_BeginBlend();

	pVi = MP_GetViewInfo();

	x_ofs = pVi->origin_half_x;
	y_ofs = (pVi->origin_half_y/2) + 20;

	// *********************************
	// 地図未ダウンロード文言表示
	// *********************************
	if (pVi->noDLFlg) {
		sprintf(str, "地図データがありません");
		y_ofs += mp_DEBUG_DrawString(str, x_ofs, y_ofs, MP_DEBUG_CENTER, MP_DEBUG_WARNING);
		sprintf(str, "下記の%s地図データを", (pVi->noDLAreaCnt>=2 ? "いずれかの" : ""));
		y_ofs += mp_DEBUG_DrawString(str, x_ofs, y_ofs, MP_DEBUG_CENTER, MP_DEBUG_WARNING);
		sprintf(str, "設定からダウンロードしてください");
		y_ofs += mp_DEBUG_DrawString(str, x_ofs, y_ofs, MP_DEBUG_CENTER, MP_DEBUG_WARNING);

		for (i=0; i<pVi->noDLAreaCnt; i++) {
			if (i%2 == 0) {	// 偶数の場合：初期化
				memset(str, 0x00, sizeof(str));
			} else {		// 奇数の場合：","設定
				strcat(str, ", ");
			}
			strcat(str, pVi->downLoadAreaName.data[pVi->noDLAreaNo[i]-1].areaName);

			if ((i%2 != 0) || (i == pVi->noDLAreaCnt-1)) {	// 奇数or最後の場合：文字列出力
				y_ofs += mp_DEBUG_DrawString(str, x_ofs, y_ofs, MP_DEBUG_CENTER, MP_DEBUG_WARNING);
			}
		}
	}

	// *********************************
	// エラー表示
	// *********************************
	for (i=0; i<ERR_TYPE_MAX; i++) {
		if(true == gErrInfo[i]) {
			sprintf(str, "エラー[%d]:%s", i, gErrStr[i]);
			y_ofs += mp_DEBUG_DrawString(str, x_ofs, y_ofs, MP_DEBUG_CENTER, MP_DEBUG_WARNING);
		}
	}

	MP_GL_EndBlend();
}

static INT32 mp_DEBUG_DrawString(char* pStr, INT32 x, INT32 y, INT32 pos, INT32 infoType)
{
	unsigned char* pBitMap = NULL;
	INT32 width;
	INT32 height;
	INT32 strWidth;
	INT32 strHeight;
	FLOAT spotX = 0.0f;
	FLOAT spotY = 0.0f;
	UINT32 textureID;
	FLOAT fontSize = (FLOAT)MP_DEBUG_FONT_SIZE;
	INT32 outLineFlg = 0;


	MP_FONT_Initialize();

	// 色設定
	if (MP_DEBUG_INFO == infoType) {
		fontSize = MP_DEBUG_FONT_SIZE;
		MP_FONT_SetColorRGBA(0, 0, 255, 255);
		outLineFlg = 1;
	} else if(MP_DEBUG_WARNING == infoType) {
		fontSize = MP_DEBUG_FONT_SIZE_WARNING;
		MP_FONT_SetColorRGBA(255, 0, 0, 255);
		//MP_FONT_SetBkgdColorRGBA(255, 255, 255, 200);
		outLineFlg = 0;
	}

	// ビットマップフォント生成
	MP_FONT_CreateBitmapFont(pStr, fontSize, outLineFlg, &pBitMap, &width, &height, &strWidth, &strHeight, 0.0f, false);
//	SC_LOG_DebugPrint(SC_TAG_MP, (Char*)"■%s,%d,%d", pStr, width, height);

	// 表示位置
	if (MP_DEBUG_LEFT == pos) {
		spotX = 0.0f;
		spotY = 0.0f;
	} else if (MP_DEBUG_CENTER == pos) {
		spotX = (FLOAT)strWidth/2;
		spotY = 0.0f;
	}

	if (NULL != pBitMap) {
		// テクスチャ生成
		textureID = MP_TEXTURE_LoadByteArray((char*)pBitMap, width, height);
		if (0 != textureID) {
			// 描画
			MP_TEXTURE_DrawRect(
				x, y,
				(FLOAT)width,
				(FLOAT)height,
				spotX, spotY,
				0.0f,
				textureID);

			// テクスチャ解放
			MP_TEXTURE_Delete(&textureID);
		}

		// ビットマップフォント解放
		MP_FONT_DeleteBitmapFont(pBitMap);
	}

	return (strHeight);
}
