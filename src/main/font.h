
/*
 * font.h
 *
 *  Created on: 2015/11/05
 *      Author:t.aikawa
 */

#ifndef _FONT_H
#define _FONT_H

#ifdef __cplusplus
extern "C" {
#endif

void initializeBitmapFont(void);
int createBitmapFont(char* str, float fontSize, int outLineFlg, unsigned char **ppBitMap, int* pWidth, int* pHeight, int* pStrWidth, int* pStrHeight, float rotation, int lineBreak);
int deleteBitmapFont(unsigned char *pBitMap);

void setColor(unsigned int color);
void setOutLineColor(unsigned int color);
void setBkgdColor(unsigned int color);

#ifdef __cplusplus
}
#endif

#endif // _FONT_H
