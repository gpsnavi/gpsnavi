
/*
 * font.cpp
 *
 *  Created on: 2015/11/05
 *      Author:t.aikawa
 */

#include "font.h"

#define MP_FREETYPE_OPENGL_BITMAP

#ifdef MP_FREETYPE_OPENGL_BITMAP
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#else
#endif /* MP_FREETYPE_OPENGL_BITMAP */

static unsigned int gColor = 0xFF444444;
static unsigned int gOutLineColor = 0xFFFFFFFF;
static unsigned int gBkgdColor = 0x00000000;

void initializeBitmapFont(void)
{
	setColor(0xFF444444);
	setOutLineColor(0xFFFFFFFF);
	setBkgdColor(0x00000000);
}

void setColor(unsigned int color)
{
	gColor = color;
}

void setOutLineColor(unsigned int color)
{
	gOutLineColor = color;
}

void setBkgdColor(unsigned int color)
{
	gBkgdColor = color;
}

bool isPowerOf2(int val) {
    return val > 0 && (val & (val - 1)) == 0;
}

int chgPowerOf2(int val) {
    int ret = 0;

    if (val <= 0) {
        return 0;
    }

    if (!isPowerOf2(val)) {
        for (int shift=2; shift<31; shift++) {
            // 32bit符号付整数なので30回シフト=2^31まで

            ret = 1 << shift;	// 1を左シフトして2のべき乗を生成
            if (ret > val) {
                return ret;
            }
        }
    } else {
        ret = val;
    }

    return ret;
}

int UTF8toUTF32(unsigned char *str, int *bytesInSequence)
{
	unsigned char c1, c2, c3, c4, c5, c6;

    *bytesInSequence = 1;
    if (!str)
    {

        return 0;
    }

    //0xxxxxxx (ASCII)      7bit
    c1 = str[0];
    if ((c1 & 0x80) == 0x00)
    {
        return c1;
    }

    //10xxxxxx              high-order byte
    if ((c1 & 0xc0) == 0x80)
    {
        return 0;
    }

    //0xFE or 0xFF          BOM (not utf-8)
    if (c1 == 0xfe || c1 == 0xFF )
    {
        return 0;
    }

    //110AAAAA 10BBBBBB     5+6bit=11bit
    c2 = str[1];
    if (((c1 & 0xe0) == 0xc0) &&
        ((c2 & 0xc0) == 0x80))
    {
        *bytesInSequence = 2;


        return ((c1 & 0x1f) << 6) | (c2 & 0x3f);
    }

    //1110AAAA 10BBBBBB 10CCCCCC        4+6*2bit=16bit
    c3 = str[2];
    if (((c1 & 0xf0) == 0xe0) &&
        ((c2 & 0xc0) == 0x80) &&
        ((c3 & 0xc0) == 0x80))
    {
        *bytesInSequence = 3;
        return ((c1 & 0x0f) << 12) | ((c2 & 0x3f) << 6) | (c3 & 0x3f);
    }

    //1111 0AAA 10BBBBBB 10CCCCCC 10DDDDDD      3+6*3bit=21bit
    c4 = str[3];
    if (((c1 & 0xf8) == 0xf0) &&
        ((c2 & 0xc0) == 0x80) &&
        ((c3 & 0xc0) == 0x80) &&
        ((c4 & 0xc0) == 0x80))
    {
        *bytesInSequence = 4;
        return ((c1 & 0x07) << 18) | ((c2 & 0x3f) << 12) | ((c3 & 0x3f) << 6) | (c4 & 0x3f);
    }

    //1111 00AA 10BBBBBB 10CCCCCC 10DDDDDD 10EEEEEE     2+6*4bit=26bit
    c5 = str[4];
    if (((c1 & 0xfc) == 0xf0) &&
        ((c2 & 0xc0) == 0x80) &&
        ((c3 & 0xc0) == 0x80) &&
        ((c4 & 0xc0) == 0x80) &&
        ((c5 & 0xc0) == 0x80))
    {
        *bytesInSequence = 4;
        return ((c1 & 0x03) << 24) | ((c2 & 0x3f) << 18) | ((c3 & 0x3f) << 12) | ((c4 & 0x3f) << 6) | (c5 & 0x3f);
    }

    //1111 000A 10BBBBBB 10CCCCCC 10DDDDDD 10EEEEEE 10FFFFFF        1+6*5bit=31bit
    c6 = str[5];
    if (((c1 & 0xfe) == 0xf0) &&
        ((c2 & 0xc0) == 0x80) &&
        ((c3 & 0xc0) == 0x80) &&
        ((c4 & 0xc0) == 0x80) &&
        ((c5 & 0xc0) == 0x80) &&
        ((c6 & 0xc0) == 0x80))
    {
        *bytesInSequence = 4;
        return ((c1 & 0x01) << 30) | ((c2 & 0x3f) << 24) | ((c3 & 0x3f) << 18) | ((c4 & 0x3f) << 12) | ((c5 & 0x3f) << 6) | (c6 & 0x3f);
    }

    return 0;
}

int createBitmapFont(char* str, float fontSize, int outLineFlg, unsigned char **ppBitMap, int* pWidth, int* pHeight, int* pStrWidth, int* pStrHeight, float rotation, int lineBreak)
{
	int buffer_width;
	int buffer_height;
	int strWidth;
	int strHeight;
	int width;
	int height;
	int n,i;
	static bool initFlag=0;
	static FT_Library library;
	static FT_Face face;
	FT_Error err;
	extern char navi_config_map_font_file[];

	*ppBitMap = NULL;
#define MAX_CHARS	(50)
	int	utf32_string[MAX_CHARS],utf32,bytesInSequence;

	n=0;
	int str_size = strlen(str);
	for(i=0;i<str_size;i+=bytesInSequence){

		utf32 = UTF8toUTF32((unsigned char *)str,&bytesInSequence);
		if(utf32 != 0){
			utf32_string[n++] = utf32;
			if(n >= MAX_CHARS){
				break;
			}
		}
		str += bytesInSequence;
	}
	if(n == 0){
		/*  */
		return(0);
	}
	//printf("usf32 size = %d\n",n);

    int bg_a = (gBkgdColor & 0xFF000000) >> 24;
    int bg_r = (gBkgdColor & 0x00FF0000) >> 16;
    int bg_g = (gBkgdColor & 0x0000FF00) >> 8;
    int bg_b = gBkgdColor & 0x000000FF;

    int fc_a = (gColor & 0xFF000000) >> 24;
    int fc_r = (gColor & 0x00FF0000) >> 16;
    int fc_g = (gColor & 0x0000FF00) >> 8;
    int fc_b = gColor & 0x000000FF;

#define FONT_MARGIN_WIDTH		(6)
#define FONT_MARGIN_HEIGHT		(6)

	buffer_width  = chgPowerOf2((fontSize+FONT_MARGIN_WIDTH) * n);
	buffer_height = fontSize + FONT_MARGIN_HEIGHT;

	int bufferSize = buffer_width*buffer_height;
	unsigned char *buffer = (unsigned char*)calloc(bufferSize,1);

	if(initFlag == 0){
		err = FT_Init_FreeType(&library);
		if (err) {printf("createBitmapFont:FT_Init_FreeType error\n"); }

#if 0
		err = FT_New_Face(library, "/usr/share/fonts/truetype/fonts-japanese-gothic.ttf", 0, &face);
#else
		err = FT_New_Face(library, navi_config_map_font_file, 0, &face);
#endif
		if (err) { printf("createBitmapFont:FT_New_Face error\n"); }
		initFlag = 1;
	}

	err = FT_Set_Pixel_Sizes(face, fontSize, fontSize);
	if (err) { printf("createBitmapFont:FT_Set_Pixel_Sizes\n"); }

	int xOffset = 0;
	for(i=0;i<n;i++){
		if(outLineFlg == 0){
		/* モノクロビットマップ */
			err = FT_Load_Char(face, utf32_string[i], 0);
			if (err) { printf("createBitmapFont:FT_Load_Char\n"); }

			int	baseline = (face->height + face->descender) * face->size->metrics.y_ppem / face->units_per_EM;
			baseline += (FONT_MARGIN_HEIGHT/2);

			//printf("1 baseline = %d\n",baseline);

			err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_MONO);
			if (err) { printf("createBitmapFont:FT_Render_Glyph\n"); }

			FT_Bitmap *bm = &face->glyph->bitmap;
			FT_GlyphSlot g = face->glyph;
			int row, col, bit, c,index;

			//printf("bitmap_left  = %d , bitmap_top  = %d\n",g->bitmap_left,g->bitmap_top);
			//printf("advance.x = %d , advance.y = %d\n",(int)g->advance.x,(int)g->advance.y);
			//printf("bitmap.width = %d , bitmap.rows = %d\n",bm->width,bm->rows);
			//printf("pitch = %d\n",bm->pitch);

			/* モノクロビットマップの場合 */
			for (row = 0; row < (int)bm->rows; row ++) {
			    for (col = 0; col < bm->pitch; col ++) {
				c = bm->buffer[bm->pitch * row + col];
				for (bit = 7; bit >= 0; bit --) {
				    if (((c >> bit) & 1) == 1){
					index = g->bitmap_left + (buffer_width * (row + baseline - g->bitmap_top)) + (col * 8 + 7 - bit)+ xOffset;
					if((index < 0) || (index >= bufferSize)){
						printf("createBitmapFont: buffer size over %d\n",index);
					}else{
						*(buffer + index) = 0xff;
					}
				    }
				}
			    }
			}
			xOffset += (g->bitmap_left + bm->width);
		}else{
		/* アンチエイリアスフォント */
#if 1
			/* ノーマルフォント */
			err = FT_Load_Char(face, utf32_string[i],0);
			if (err) { printf("FT_Load_Char\n"); }
#else
			/* ボールド */
			err = FT_Load_Char(face, utf32_string[i], FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP);
			if (err) { printf("FT_Load_Char\n"); }
			if(face->glyph->format != FT_GLYPH_FORMAT_OUTLINE){
			    printf("createBitmapFont: FT_Load_Char format != FT_GLYPH_FORMAT_OUTLINE\n"); // エラー！ アウトラインでなければならない
			}
			int strength = 1 << 6;    // 適当な太さ
			FT_Outline_Embolden(&face->glyph->outline, strength);
#endif

#if 1
			int	baseline = (face->height + face->descender) * face->size->metrics.y_ppem / face->units_per_EM;
#else
			int	baseline = (face->ascender) * face->size->metrics.y_ppem / face->units_per_EM;
#endif
			baseline += (FONT_MARGIN_HEIGHT/2);

			//printf("2 baseline = %d\n",baseline);

			err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
			if (err) { printf("createBitmapFont:FT_Render_Glyph error\n"); }

			FT_Bitmap *bm = &face->glyph->bitmap;
			FT_GlyphSlot g = face->glyph;
			int row, col, index;
			unsigned char c;

			//printf("bitmap_left  = %d , bitmap_top  = %d\n",g->bitmap_left,g->bitmap_top);
			//printf("advance.x = %d , advance.y = %d\n",(int)g->advance.x,(int)g->advance.y);
			//printf("bitmap.width = %d , bitmap.rows = %d\n",bm->width,bm->rows);
			//printf("pitch = %d\n",bm->pitch);

			for (row = 0; row < (int)bm->rows; row ++) {
			    for (col = 0; col < bm->pitch; col ++) {
				c = (unsigned char)bm->buffer[bm->pitch * row + col];
				if(c > 0){
					index = g->bitmap_left + (buffer_width * (row + baseline - g->bitmap_top)) + col + xOffset;
					if((index < 0) || (index >= bufferSize)){
						printf("createBitmapFont: buffer size over %d\n",index);
					}else{
						*(buffer + index) = c;
					}
				}
			    }
			}
			xOffset += (g->bitmap_left + bm->width);
		}
	}
	//printf("xOffset = %d\n",xOffset);

    strWidth = xOffset;
    strHeight = buffer_height;

    width = chgPowerOf2(strWidth);
    height = chgPowerOf2(strHeight);

	int fontBufferSize = width*height;
	unsigned char *fontBuffer = (unsigned char*)calloc(fontBufferSize,4);

    //背景色を描画
    for(int h = 0; h<strHeight; h++){
        for(int w = 0; w<strWidth; w++){
            int out = w * 4 + (width * 4)*h;
            int in  = w     + (buffer_width)*h;
            int alpha;

            alpha = buffer[in];
            if(alpha != 0x00){
                fontBuffer[out] = bg_r;
                fontBuffer[out+1] = bg_g;
                fontBuffer[out+2] = bg_b;
                fontBuffer[out+3] = bg_a;


            	alpha = (alpha * fc_a) / 255;

            	fontBuffer[out]   = (((fc_r - bg_r) * alpha) / 255) + bg_r;
            	fontBuffer[out+1] = (((fc_g - bg_g) * alpha) / 255) + bg_g;
            	fontBuffer[out+2] = (((fc_b - bg_b) * alpha) / 255) + bg_b;
                fontBuffer[out+3] = (((alpha - bg_a) * alpha) / 255) + bg_a;
            }else{
                fontBuffer[out] = bg_r;
                fontBuffer[out+1] = bg_g;
                fontBuffer[out+2] = bg_b;
                fontBuffer[out+3] = bg_a;
            }
        }
    }

	free(buffer);

	*ppBitMap = fontBuffer;

	*pWidth = width;
	*pHeight =height;
	*pStrWidth = strWidth;
	*pStrHeight =strHeight;

    return 0;
}

int deleteBitmapFont(unsigned char *pBitMap)
{
	if(NULL == pBitMap) {
		return -1;
	}

	free(pBitMap);

	return 0;
}
