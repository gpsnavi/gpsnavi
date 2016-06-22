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
 * HMI_Icon.c
 *
 *  Created on: 2015/11/05
 *      Author:t.aikawa
 */

#include <stdio.h>
#include <string.h>

#include "navicore.h"

#include "SMCoreMP/MP_Def.h"
#include "SMCoreMP/MP_GL.h"
#include "SMCoreMP/MP_Texture.h"

//#include "navi.h"
#include "HMI_Icon.h"
#include "image.h"

#define NC_SUCCESS				(0)
#define NC_ERROR				(-1)
#define NC_PARAM_ERROR			(-2)

typedef struct _ICON_TEXTURE {
	INT32 id;		// テクスチャID
	INT32 w;		// 幅
	INT32 h;		// 高さ
	UINT8 type;		// タイプ 0:bmp,1:png、
	UINT8 lvlimit;	// 最上位の表示スケール
	UINT8 spot_x;	// ホットスポットのX方向オフセット
	UINT8 spot_y;	// ホットスポットのY方向オフセット
} T_ICON_TEXTURE;

#define HMI_ICON_CNT_MAX		50
#define	 SC_MAX_PATH			512		// パス最大長

static INT32 g_IconCnt = 0;
static T_ICON_TEXTURE g_IconTexture[HMI_ICON_CNT_MAX];

typedef void*	MP_PING_HDL;
MP_PING_HDL MP_PNG_CreatePngForFile(char* pPath, INT32* pWidth, INT32* pHeight);
MP_PING_HDL MP_PNG_CreatePngForImage(char* pData, INT32* pWidth, INT32* pHeight,INT32 size);
void MP_PNG_DeletePng(MP_PING_HDL pngHdl);

static FILE* wfp = NULL;
static FILE* ifp = NULL;

static int hmi_create_image_source = 0;

extern int				hmi_image_num;
extern T_ICON_IMAGE		hmi_image[];

void  hmiMP_ICON_CreateResImage(void){
	hmi_create_image_source = 1;
}

UINT32 hmiMP_TEXTURE_Load(char* fileName, INT32* pWidth, INT32* pHeight)
{
	MP_PING_HDL pngHDL;
	UINT8* pPngData = NULL;
	UINT32 textureID;

	*pWidth = 0;
	*pHeight = 0;

	if(NULL == fileName) {
		printf("hmiMP_TEXTURE_Load:fileName NULL\n");
		return (0);
	}

	pngHDL = MP_PNG_CreatePngForFile(fileName, pWidth, pHeight);
	if (NULL == pngHDL) {
		printf("hmiMP_TEXTURE_Load:MP_PNG_CreatePngForFile NULL (%s)\n",fileName);
		return (0);
	}

	pPngData = (UINT8*)pngHDL;

	textureID = MP_GL_GenTextures(pPngData, *pWidth, *pHeight);

	MP_PNG_DeletePng(pngHDL);

	return ((UINT32)textureID);
}

UINT32 hmiMP_TEXTURE_Memory(char* data, INT32* pWidth, INT32* pHeight,INT32 size)
{
	MP_PING_HDL pngHDL;
	UINT8* pPngData = NULL;
	UINT32 textureID;

	*pWidth = 0;
	*pHeight = 0;

	if(NULL == data) {
		printf("hmiMP_TEXTURE_Load:data pointer NULL\n");
		return (0);
	}

	pngHDL = MP_PNG_CreatePngForImage(data, pWidth, pHeight, size);
	if (NULL == pngHDL) {
		printf("hmiMP_TEXTURE_Memory:MP_PNG_CreatePngForImage NULL\n");
		return (0);
	}

	pPngData = (UINT8*)pngHDL;

	textureID = MP_GL_GenTextures(pPngData, *pWidth, *pHeight);

	MP_PNG_DeletePng(pngHDL);

	return ((UINT32)textureID);
}

INT32 hmiMP_ICON_Load(char* p_Path, char* p_InfoPath)
{
	INT32	ret = NC_SUCCESS;
	INT32 i = 0;
	INT32 icon_cnt = 0;
	FILE* fp = NULL;
	char icon_path[SC_MAX_PATH];
	INT32 width = 0;
	INT32 height = 0;

	// 初期化
	memset(g_IconTexture, 0, sizeof(g_IconTexture));

	if(NULL == p_Path) {
		printf("hmiMP_ICON_Load: p_Path NULL\n");
		return NC_PARAM_ERROR;
	}
	if(NULL == p_InfoPath) {
		printf("hmiMP_ICON_Load: p_InfoPath NULL\n");
		return NC_PARAM_ERROR;
	}

	// アイコン情報解析
	fp = fopen(p_InfoPath, "rb");
	if(NULL == fp) {
		printf("hmiMP_ICON_Load: fp NULL\n");
		return NC_PARAM_ERROR;
	}
	fseek(fp, 0, SEEK_END);
	icon_cnt = (ftell(fp) / 4);
	fseek(fp, 0, SEEK_SET);

	// ------------------------------------------------------------
	if(hmi_create_image_source == 1){
		wfp = fopen("image.c", "wb");
		if(NULL == wfp) {
			printf("hmiMP_ICON_Load: wfp can not create.\n");
			return NC_PARAM_ERROR;
		}
		printf("create hmi image(image.c)\n");
		fprintf(wfp,
				"\n\n\n"
				"#include \"image.h\"\n"
				"\n"
				"int		hmi_image_num = %d;\n"
				"T_ICON_IMAGE	hmi_image[] = {\n",icon_cnt
				);
	}
	// ------------------------------------------------------------

	for(i=0; i<icon_cnt; i++) {
		// アイコン情報
		fread(&g_IconTexture[i].type, 1, 1, fp);
		fread(&g_IconTexture[i].lvlimit, 1, 1, fp);
		fread(&g_IconTexture[i].spot_x, 1, 1, fp);
		fread(&g_IconTexture[i].spot_y, 1, 1, fp);

		width = 0;
		height = 0;

		if (0 == g_IconTexture[i].type) {

			if(wfp != NULL){
				fprintf(wfp,
						"	{\n"
						"		.n = 0,\n"
						"		.info = {\n"
						"		},\n"
						"		.image = NULL,\n"
						"	},\n"
						);
			}

			continue;
		}

		// アイコン
		if(i > 0) {

			sprintf(icon_path, "%s/%d.png", p_Path, i);
#if 0
			g_IconTexture[i].id = MP_TEXTURE_Load(icon_path, &width, &height);
#else
			g_IconTexture[i].id = hmiMP_TEXTURE_Load(icon_path, &width, &height);
#endif
			if(0 == g_IconTexture[i].id) {
				printf("hmiMP_ICON_Load: MP_TEXTURE_Load icon_path(%s)\n",icon_path);
				ret = NC_PARAM_ERROR;
				continue;
			}

			g_IconTexture[i].w = width;
			g_IconTexture[i].h = height;

#if 0
			printf("★MP_TEXTURE_Load %s,%d,%d,%d,%d,%d,%d,%d\n",
				icon_path,
				g_IconTexture[i].id,
				g_IconTexture[i].w,
				g_IconTexture[i].h,
				g_IconTexture[i].type,
				g_IconTexture[i].lvlimit,
				g_IconTexture[i].spot_x,
				g_IconTexture[i].spot_y
				);
#endif
			// ------------------------------------------------------------
			if(wfp != NULL){
				int n;
				int	ch;
				unsigned fileSize;

				ifp = fopen(icon_path, "rb");
				if(ifp==NULL){
					continue;
				}
				fseek(ifp,0,SEEK_END);
				fileSize=ftell(ifp);
				fseek(ifp,0,SEEK_SET);

				fprintf(wfp,
						"	{\n"
						"	//	%d.png\n"
						"		.n = %d,\n"
						"		.info = {\n"
						"		.w = %d,\n"
						"		.h = %d,\n"
						"		.type = %d,\n"
						"		.lvlimit = %d,\n"
						"		.spot_x = %d,\n"
						"		.spot_y = %d,\n"
						"		},\n"
						"		.size = %d,\n",
						i,i,
						g_IconTexture[i].w,
						g_IconTexture[i].h,
						g_IconTexture[i].type,
						g_IconTexture[i].lvlimit,
						g_IconTexture[i].spot_x,
						g_IconTexture[i].spot_y,
						fileSize
						);
				n = 0;
				fprintf(wfp,"		.image =");
				while((ch = fgetc(ifp)) != EOF){
					if(n == 0){
						fprintf(wfp,"\n\"");
					}
					fprintf(wfp,"\\x%02x",ch);
					n++;
					if(n == 32){
						fprintf(wfp,"\"");
						n = 0;
					}
				}
				if(n != 0){
					fprintf(wfp,"\"");
				}
				fprintf(wfp,",\n");
				fprintf(wfp,"	},\n");

				fclose(ifp);
			}
			// ------------------------------------------------------------
		}
	}

	fclose(fp);

	// ------------------------------------------------------------
	if(wfp != NULL){
		fprintf(wfp,"};\n\n");
		fclose(wfp);
	}
	// ------------------------------------------------------------

	g_IconCnt = icon_cnt;

	return ret;
}

INT32 hmiMP_ICON_Initialize(void)
{
	INT32	ret = NC_SUCCESS;
	INT32	icon_cnt = 0;
	INT32	i = 0;
	INT32	n;
	INT32 width = 0;
	INT32 height = 0;

	// 初期化
	memset(g_IconTexture, 0, sizeof(g_IconTexture));

	extern int				hmi_image_num;
	extern T_ICON_IMAGE		hmi_image[];

	icon_cnt = hmi_image_num;

	for(i=0; i<icon_cnt; i++) {
		n = hmi_image[i].n;
		if(n != 0){
			g_IconTexture[n].w			= hmi_image[i].info.w;
			g_IconTexture[n].h			= hmi_image[i].info.h;
			g_IconTexture[n].type		= hmi_image[i].info.type;
			g_IconTexture[n].lvlimit	= hmi_image[i].info.lvlimit;
			g_IconTexture[n].spot_x		= hmi_image[i].info.spot_x;
			g_IconTexture[n].spot_y		= hmi_image[i].info.spot_y;

			g_IconTexture[n].id = hmiMP_TEXTURE_Memory(hmi_image[i].image, &width, &height,hmi_image[i].size);

			if(0 == g_IconTexture[n].id) {
				printf("hmiMP_ICON_Initialize: hmiMP_TEXTURE_Memory bad image(%d.png)\n",i);
				ret = NC_PARAM_ERROR;
				continue;
			}
		}
	}
	g_IconCnt = icon_cnt;

	return ret;

}

INT32 hmiMP_ICON_Finalize(void)
{
	INT32	ret = NC_SUCCESS;
	INT32 i=0;

	for(i=1; i<g_IconCnt; i++) {
		if(0 != g_IconTexture[i].id) {
			printf("hmiMP_ICON_Finalize: ★MP_TEXTURE_Delete %d\n", g_IconTexture[i].id);
			MP_TEXTURE_Delete((UINT32*)&g_IconTexture[i].id);
		}
		g_IconTexture[i].id = 0;
	}
	memset(g_IconTexture, 0, sizeof(g_IconTexture));
	g_IconCnt = 0;

	return ret;
}

INT32 hmiMP_ICON_Draw(FLOAT x, FLOAT y, FLOAT angle, FLOAT scale, INT32 iconID)
{
	INT32	ret = NC_SUCCESS;

	if (iconID >= g_IconCnt) {
		printf("hmiMP_ICON_Draw: (%d)\n",iconID);
		return NC_PARAM_ERROR;
	}
	if (0 == g_IconTexture[iconID].id) {
		printf("hmiMP_ICON_Draw: icon_id 0\n");
		return NC_PARAM_ERROR;
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

	return ret;
}

INT32 mapMP_ICON_Draw(FLOAT x, FLOAT y, FLOAT angle, FLOAT scale, INT32 iconID)
{
	typedef enum _E_SC_RESULT {
		e_SC_RESULT_SUCCESS = 0x00000000,
	} E_SC_RESULT;

	E_SC_RESULT MP_ICON_Draw(FLOAT x, FLOAT y, FLOAT angle, FLOAT scale, INT32 iconID);

	INT32	ret = NC_SUCCESS;

	MP_ICON_Draw(x,y,angle,scale,iconID);

	return (INT32)ret;
}

