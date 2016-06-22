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
// 型
//-----------------------------------------------------------------------------
typedef char* NAME_HDL;		// 名称ハンドル
typedef char* NMLG_HDL;		// 言語名称ハンドル

//-----------------------------------------------------------------------------
// マクロ
//-----------------------------------------------------------------------------
// 表示名称
#define GET_NAME_SIZE(_p)				GetUINT16(_p, 0)		// 表示名称サイズ
#define GET_NAME_LANG_CNT(_p)			GetUINT16(_p, 2)		// 言語情報数
#define GET_NAME_NAME_KIND(_p)			GetUINT32(_p, 4)		// 名称種別
#define GET_NAME_ID(_p)					GetUINT32(_p, 8)		// 表示名称ID

#define GET_NMLG_SIZE(_p)				GetUINT16(_p, 0)		// 言語名称サイズ
#define GET_NMLG_LANG_KIND(_p)			GetUINT8(_p, 2)			// 言語情報
#define GET_NMLG_INFO1(_p)				GetUINT32(_p, 4)		// 名称情報1
#define GET_NMLG_X(_p)					GetUINT16(_p, 8)		// X座標
#define GET_NMLG_Y(_p)					GetUINT16(_p, 10)		// Y座標
#define GET_NMLG_OFS_X(_p)				GetINT8(_p, 12)			// X座標オフセット
#define GET_NMLG_OFS_Y(_p)				GetINT8(_p, 13)			// Y座標オフセット
#define GET_NMLG_STR_SIZE(_p)			GetUINT16(_p, 14)		// 文字列サイズ
#define GET_NMLG_STR(_p)				(char*)(&_p[16])		// 文字列

#define GET_NAME_NEXT_HDL(_p)			(NAME_HDL)(_p + (GET_NAME_SIZE(_p)*4))	// 次の名称ハンドル

//-----------------------------------------------------------------------------
// クラス
//-----------------------------------------------------------------------------
/**
 * @brief  名称解析クラス
 */
class SMNameAnalyze
{
private:
	UINT32		m_ParcelID;		// パーセルID
	char*		m_pName;		// 名称データカラム

	UINT32		m_name_cnt;		// 表示名称要素数

	NAME_HDL	m_FirstNameHdl;	// 先頭名称ハンドル
//	NAME_HDL*	m_pNameHdlList;	// 表示名称ハンドルリスト

	E_SC_RESULT InitName(void);
//	UINT16 SearchNameIndex(UINT32 NameID);
public:
	/**
	 * @brief コンストラクタ
	 */
	SMNameAnalyze(void);
	/**
	 * @brief デストラクタ
	 */
	~SMNameAnalyze(void);

	/**
	 * @brief 初期化
	 * @param[in] ParcelID パーセルID
	 * @param[in] p_Name 名称データカラム
	 * @return 処理結果
	 */
	E_SC_RESULT Initialize(const UINT32 ParcelID, const char* p_Name);

	/**
	 * @brief 終了処理
	 * @return 処理結果
	 */
	E_SC_RESULT Finalize(void);

	/**
	 * @brief 名称要素数取得
	 * @return 記号背景要素数
	 */
	UINT32 GetNameCnt(void);
#if 0
	/**
	 * @brief 名称ハンドル取得
 	 * @param[in] NameID 表示名称ID
	 * @return 名称ハンドル
	 */
	NAME_HDL GetNameHdl(UINT32 NameID);

	/**
	 * @brief 名称ハンドル取得
 	 * @param[in] Index インデックス
	 * @return 名称ハンドル
	 */
	NAME_HDL GetNameHdl(UINT16 Index);
#endif
	/**
	 * @brief 先頭名称ハンドル取得
	 * @return 名称ハンドル
	 */
	NAME_HDL FirstNameHdl(void);

	/**
	 * @brief 次名称ハンドル取得
	 * @param[in] NAME_HDL 名称ハンドル
	 * @return 名称ハンドル
	 */
	NAME_HDL NextNameHdl(NAME_HDL nameHdl);

	/**
	 * @brief 言語名称ハンドル取得
 	 * @param[in] hName 名称ハンドル
 	 * @param[in] No 何番目
	 * @return 言語名称ハンドル
	 */
	NMLG_HDL GetNmLgHdl(NAME_HDL hName, UINT32 No);
};
