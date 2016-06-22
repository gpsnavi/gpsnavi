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

UINT32 MP_TEXTURE_Load(char* fileName, INT32* pWidth, INT32* pHeight)
{
	MP_PING_HDL pngHDL;
	UINT8* pPngData = NULL;
	UINT32 textureID;

	*pWidth = 0;
	*pHeight = 0;

	if(NULL == fileName) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"fileName NULL, " HERE);
		return (0);
	}

	pngHDL = MP_PNG_CreatePngForFile(fileName, pWidth, pHeight);
	if (NULL == pngHDL) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"MP_PNG_CreatePng NULL (%s), " HERE, fileName);
		return (0);
	}

	pPngData = (UINT8*)pngHDL;

	textureID = MP_GL_GenTextures(pPngData, *pWidth, *pHeight);

	MP_PNG_DeletePng(pngHDL);

	return ((UINT32)textureID);
}

UINT32 MP_TEXTURE_LoadByteArray(char* pByteArray, INT32 width, INT32 height)
{
	UINT32 textureID;

	if(NULL == pByteArray) {
		return (0);
	}

	textureID = MP_GL_GenTextures(pByteArray, width, height);

	return ((UINT32)textureID);
}

void MP_TEXTURE_Delete(UINT32* p_TextureID)
{
	MP_GL_DeleteTextures(p_TextureID);
}

void MP_TEXTURE_DrawRect(FLOAT x, FLOAT y, FLOAT width, FLOAT height, FLOAT spot_x, FLOAT spot_y, FLOAT rotation, UINT32 textureId)
{
	T_POINT squares[4] = {
		0,		0,
		width,	0,
		0,		height,
		width,	height
	};

	MP_GL_PushMatrix();

	// 指定座標に移動
	MP_GL_Translatef(x, y, 0.0);
	MP_GL_Rotatef(rotation, 0.0, 0.0, 1.0);

	// スポットにオフセット
	if(0 != spot_x || 0 != spot_y) {
		MP_GL_Translatef(-spot_x, -spot_y, 0.0);
	}

	MP_GL_DrawTextures(textureId, squares);

	MP_GL_PopMatrix();
}

