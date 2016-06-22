/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef _MP_FONT_H
#define _MP_FONT_H

#ifdef __cplusplus
extern "C" {
#endif

void MP_FONT_Initialize(void);

E_SC_RESULT MP_FONT_SetBitmapFontCB(NC_BITMAPFONTFUNCPTR pfunc);

E_SC_RESULT MP_FONT_CreateBitmapFont(char* str, FLOAT fontSize, INT32 outLineFlg, UChar **ppBitMap, INT32* pWidth, INT32* pHeight, INT32* pStrWidth, INT32* pStrHeight, FLOAT rotation, Bool lineBreak);
E_SC_RESULT MP_FONT_DeleteBitmapFont(UChar *pBitMap);

void MP_FONT_SetColorRGBA(UINT8 r, UINT8 g, UINT8 b, UINT8 a);
void MP_FONT_SetColor(UINT32 color);
UINT32 MP_FONT_GetColor(void);

void MP_FONT_SetOutLineColorRGBA(UINT8 r, UINT8 g, UINT8 b, UINT8 a);
void MP_FONT_SetOutLineColor(UINT32 color);
UINT32 MP_FONT_GetOutLineColor(void);

void MP_FONT_SetBkgdColorRGBA(UINT8 r, UINT8 g, UINT8 b, UINT8 a);
void MP_FONT_SetBkgdColor(UINT32 color);
UINT32 MP_FONT_GetBkgdColor(void);

#ifdef __cplusplus
}
#endif

#endif // _MP_FONT_H
