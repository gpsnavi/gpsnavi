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

MP_FontMng::MP_FontMng(UINT32 dataCntMax, bool isSearch)
{
	mDataCntMax = dataCntMax;
	mIsSearch = isSearch;
	mpDataArray = NULL;

	if (mIsSearch) {
		mDataCntMax = 0;
	} else {
		if (0 < mDataCntMax) {
			mpDataArray = new T_BITMAPFONT[mDataCntMax];
			if (NULL != mpDataArray) {
				memset(mpDataArray, 0x00, sizeof(T_BITMAPFONT)*mDataCntMax);
			} else {
				mDataCntMax = 0;
			}
		}
	}

	mDataMap.clear();
}

MP_FontMng::~MP_FontMng(void)
{
	if (mIsSearch) {
		MAP_t::iterator itr;
		for (itr = mDataMap.begin(); itr!=mDataMap.end(); itr++) {
			if (NULL != itr->second.pData) {
				MP_FONT_DeleteBitmapFont((UChar*)itr->second.pData);
			}
		}
		mDataMap.clear();
	}
	else {
		if (NULL != mpDataArray) {
			for (INT32 i=0; i<mDataCntMax; i++) {
				if (mpDataArray[i].readFlg) {
					MP_FONT_DeleteBitmapFont(mpDataArray[i].pData);
				}
			}

			delete [] mpDataArray;
			mpDataArray = NULL;
		}
	}
}

T_BITMAPFONT* MP_FontMng::setMap(const UINT32 key, T_BITMAPFONT& bitmapFont)
{
	std::pair< MAP_t::iterator, bool > result;

	result = mDataMap.insert(MAP_t::value_type(key, bitmapFont));
	if (!result.second) {
		return (NULL);
	}
	return (&result.first->second);
}

T_BITMAPFONT* MP_FontMng::getMap(const UINT32 key)
{
	MAP_t::iterator itr;

	itr = mDataMap.find(key);
	if (itr != mDataMap.end()) {
		return (&itr->second);
	}
	return (NULL);
}

T_BITMAPFONT* MP_FontMng::setArray(const UINT32 key, T_BITMAPFONT& bitmapFont)
{
	T_BITMAPFONT *pBitmapFont = NULL;

	if (key < mDataCntMax) {
		pBitmapFont = &mpDataArray[key];

		// 値を設定
		pBitmapFont->readFlg	= bitmapFont.readFlg;
		pBitmapFont->pData		= bitmapFont.pData;
		pBitmapFont->width		= bitmapFont.width;
		pBitmapFont->height		= bitmapFont.height;
		pBitmapFont->strWidth	= bitmapFont.strWidth;
		pBitmapFont->strHeight	= bitmapFont.strHeight;
		pBitmapFont->rotation	= bitmapFont.rotation;
	}

	return (pBitmapFont);
}

T_BITMAPFONT* MP_FontMng::getArray(const UINT32 key)
{
	if (key < mDataCntMax) {
		return (&mpDataArray[key]);
	}
	return (NULL);
}


T_BITMAPFONT* MP_FontMng::set(const UINT32 key, T_FONT_INFO& fontInfo)
{
	E_SC_RESULT ret;
	T_BITMAPFONT bitmapFont;
	T_BITMAPFONT* pBitmapFont = NULL;
#if 0 // kana暫定
	fontInfo.fontSize = 24.0;
#endif
	// 色設定
	MP_FONT_SetColorRGBA(GET_R(fontInfo.color), GET_G(fontInfo.color), GET_B(fontInfo.color), GET_A(fontInfo.color));
	MP_FONT_SetOutLineColorRGBA(GET_R(fontInfo.outLineColor), GET_G(fontInfo.outLineColor), GET_B(fontInfo.outLineColor), GET_A(fontInfo.outLineColor));
	MP_FONT_SetBkgdColorRGBA(GET_R(fontInfo.bkgdColor), GET_G(fontInfo.bkgdColor), GET_B(fontInfo.bkgdColor), GET_A(fontInfo.bkgdColor));

	// ビットマップフォント生成
	ret = MP_FONT_CreateBitmapFont(
					fontInfo.pStr,
					fontInfo.fontSize,
					fontInfo.outLineFlg,
					&bitmapFont.pData,
					&bitmapFont.width,
					&bitmapFont.height,
					&bitmapFont.strWidth,
					&bitmapFont.strHeight,
					fontInfo.rotation,
					fontInfo.lineBreak);

	MP_FONT_Initialize();

	if (e_SC_RESULT_SUCCESS != ret) {
		return (NULL);
	}
	bitmapFont.readFlg = true;
	bitmapFont.rotation = fontInfo.rotation;

	if (mIsSearch) {
		pBitmapFont = setMap(key, bitmapFont);
	} else {
		pBitmapFont = setArray(key, bitmapFont);
	}

	if (NULL == pBitmapFont) {
		MP_FONT_DeleteBitmapFont(bitmapFont.pData);
	}

	return (pBitmapFont);
}

T_BITMAPFONT* MP_FontMng::get(const UINT32 key)
{
	if (mIsSearch) {
		return (getMap(key));
	}
	return (getArray(key));
}
