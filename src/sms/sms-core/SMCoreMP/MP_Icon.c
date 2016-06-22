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

typedef struct _ICON_TEXTURE {
	INT32 id;		// テクスチャID
	INT32 w;		// 幅
	INT32 h;		// 高さ
	UINT8 type;		// タイプ 0:bmp,1:png、
	UINT8 lvlimit;	// 最上位の表示スケール
	UINT8 spot_x;	// ホットスポットのX方向オフセット
	UINT8 spot_y;	// ホットスポットのY方向オフセット
} T_ICON_TEXTURE;

static INT32 g_IconCnt = 0;
static T_ICON_TEXTURE g_IconTexture[ICON_CNT_MAX];

E_SC_RESULT MP_ICON_Initialize(char* p_Path, char* p_InfoPath)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32 i = 0;
	INT32 icon_cnt = 0;
	FILE* fp = NULL;
	char icon_path[SC_MAX_PATH];
	INT32 width = 0;
	INT32 height = 0;

	// 初期化
	memset(g_IconTexture, 0, sizeof(g_IconTexture));

	if(NULL == p_Path) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"MP_ICON_Initialize p_Path NULL, " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if(NULL == p_InfoPath) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"MP_ICON_Initialize p_InfoPath NULL, " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// アイコン情報解析
	fp = fopen(p_InfoPath, "rb");
	if(NULL == fp) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"MP_ICON_Initialize fp NULL, " HERE);
		return (e_SC_RESULT_FILE_OPENERR);
	}
	fseek(fp, 0, SEEK_END);
	icon_cnt = (ftell(fp) / 4);
	fseek(fp, 0, SEEK_SET);

	for(i=0; i<icon_cnt; i++) {
		// アイコン情報
		fread(&g_IconTexture[i].type, 1, 1, fp);
		fread(&g_IconTexture[i].lvlimit, 1, 1, fp);
		fread(&g_IconTexture[i].spot_x, 1, 1, fp);
		fread(&g_IconTexture[i].spot_y, 1, 1, fp);

		width = 0;
		height = 0;

		if (0 == g_IconTexture[i].type) {
			continue;
		}

		// アイコン
		if(i > 0) {

			sprintf(icon_path, "%s/%d.png", p_Path, i);
			g_IconTexture[i].id = MP_TEXTURE_Load(icon_path, &width, &height);
			if(0 == g_IconTexture[i].id) {
				SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"MP_TEXTURE_Load icon_path(%s), " HERE, icon_path);
				ret = e_SC_RESULT_FAIL;
				continue;
			}

			g_IconTexture[i].w = width;
			g_IconTexture[i].h = height;

			SC_LOG_InfoPrint(SC_TAG_MP, (const Char*)"★MP_TEXTURE_Load %s,%d,%d,%d,%d,%d,%d,%d",
				icon_path,
				g_IconTexture[i].id,
				g_IconTexture[i].w,
				g_IconTexture[i].h,
				g_IconTexture[i].type,
				g_IconTexture[i].lvlimit,
				g_IconTexture[i].spot_x,
				g_IconTexture[i].spot_y
				);
		}
	}

	fclose(fp);

	g_IconCnt = icon_cnt;

	return (ret);
}

E_SC_RESULT MP_ICON_Finalize(void)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32 i=0;

	for(i=1; i<g_IconCnt; i++) {
		if(0 != g_IconTexture[i].id) {
			SC_LOG_InfoPrint(SC_TAG_MP, (const Char*)"★MP_TEXTURE_Delete %d", g_IconTexture[i].id);
			MP_TEXTURE_Delete(&g_IconTexture[i].id);
		}
		g_IconTexture[i].id = 0;
	}
	memset(g_IconTexture, 0, sizeof(g_IconTexture));
	g_IconCnt = 0;

	return (ret);
}

E_SC_RESULT MP_ICON_Draw(FLOAT x, FLOAT y, FLOAT angle, FLOAT scale, INT32 iconID)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	if (iconID >= g_IconCnt) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"MP_ICON_Draw (%d), " HERE, iconID);
		return (e_SC_RESULT_FAIL);
	}
	if (0 == g_IconTexture[iconID].id) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"MP_ICON_Draw icon_id 0, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	MP_TEXTURE_DrawRect(
			x,
			y,
			g_IconTexture[iconID].w * scale,
			g_IconTexture[iconID].h * scale,
			g_IconTexture[iconID].spot_x * scale,
			g_IconTexture[iconID].spot_y * scale,
			angle,
			g_IconTexture[iconID].id);

	return (ret);
}
