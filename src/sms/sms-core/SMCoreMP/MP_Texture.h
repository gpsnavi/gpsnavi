/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef _MP_TEXTURE_H
#define _MP_TEXTURE_H

#ifdef __cplusplus
extern "C" {
#endif

UINT32 MP_TEXTURE_Load(char* fileName, INT32* pWidth, INT32* pHeight);
UINT32 MP_TEXTURE_LoadByteArray(char* pByteArray, INT32 width, INT32 height);
void MP_TEXTURE_Delete(UINT32* p_TextureID);
void MP_TEXTURE_DrawRect(FLOAT x, FLOAT y, FLOAT width, FLOAT height, FLOAT spot_x, FLOAT spot_y, FLOAT rotation, UINT32 textureId);

#ifdef __cplusplus
}
#endif

#endif	// _MP_TEXTURE_H
