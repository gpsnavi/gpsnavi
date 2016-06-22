/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#pragma once

//-----------------------------------------------------------------------------
// 構造体
//-----------------------------------------------------------------------------
// ビットマップフォント
typedef struct _BITMAPFONT {
	Bool	readFlg;
	UINT8*	pData;
	INT32	width;
	INT32	height;
	INT32	strWidth;
	INT32	strHeight;
	FLOAT	rotation;
} T_BITMAPFONT;

// フォント情報
typedef struct _FONT_INFO {
	char		*pStr;			// 文字列ポインタ
	FLOAT		fontSize;		// サイズ
	INT32		outLineFlg;		// 縁の有無 1縁有、0縁無
	RGBACOLOR	color;			// 色
	RGBACOLOR	outLineColor;	// 縁色
	RGBACOLOR	bkgdColor;		// 背景色
	FLOAT		rotation;		// 回転角度
	Bool		lineBreak;		// 改行指定
} T_FONT_INFO;


//-----------------------------------------------------------------------------
// クラス
//-----------------------------------------------------------------------------
/**
 * @brief  フォント管理クラス
 */
class MP_FontMng
{
private:
	typedef std::map< UINT32, T_BITMAPFONT > MAP_t;

	// データ管理数最大値
	UINT32 mDataCntMax;

	// 検索ON/OFF
	bool mIsSearch;

	// データ
	T_BITMAPFONT*						mpDataArray;
	std::map< UINT32, T_BITMAPFONT >	mDataMap;

	/**
	 * @brief データ設定(map)
	 */
	inline T_BITMAPFONT* setMap(const UINT32 key, T_BITMAPFONT& bitmapFont);

	/**
	 * @brief データ取得(map)
	 */
	inline T_BITMAPFONT* getMap(const UINT32 key);

	/**
	 * @brief データ設定
	 */
	inline T_BITMAPFONT* setArray(const UINT32 key, T_BITMAPFONT& bitmapFont);

	/**
	 * @brief データ取得
	 */
	inline T_BITMAPFONT* getArray(const UINT32 key);

public:
	/**
	 * @brief コンストラクタ
	 */
	MP_FontMng(UINT32 dataCntMax, bool isSearch);

	/**
	 * @brief デストラクタ
	 */
	~MP_FontMng(void);

	/**
	 * @brief データ設定
	 */
	T_BITMAPFONT* set(const UINT32 key, T_FONT_INFO& fontInfo);

	/**
	 * @brief データ取得
	 */
	T_BITMAPFONT* get(const UINT32 index);
};
