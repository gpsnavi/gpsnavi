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
typedef char* MARK_HDL;		// 記号背景ハンドル


//-----------------------------------------------------------------------------
// マクロ
//-----------------------------------------------------------------------------
// 記号背景情報
#define GET_MARK_ELM_SIZE(_p)			GetUINT16(_p, 0)		// 記号背景要素サイズ
#define GET_MARK_INFO1(_p)				GetUINT32(_p, 4)		// 記号背景情報1
#define GET_MARK_ID(_p)					GetUINT32(_p, 8)		// 記号背景ID
#define GET_MARK_IMAGE_KIND(_p)			GetUINT16(_p, 12)		// イメージ種別

#define GET_MARK_IMAGE_ID(_p)			0						// イメージID
#define GET_MARK_INFO2(_p)				GetUINT16(_p, 16)		// 記号背景情報2
#define GET_MARK_ANGLE(_p)				GetUINT16(_p, 18)		// 角度
#define GET_MARK_X(_p)					GetUINT16(_p, 20)		// X座標
#define GET_MARK_Y(_p)					GetUINT16(_p, 22)		// Y座標

#define GET_MARK_DATA_TITLE_SIZE(_p)	GetUINT16(_p, 24)		// データタイトルサイズ
#define GET_MARK_LANG_INFO_CNT(_p)		GetUINT16(_p, 26)		// 言語情報数

#define GET_MARK_TITLE_SIZE(_p)			GetUINT16(_p, 28)		// サイズ
#define GET_MARK_TITLE_LANGKIND(_p)		GetUINT16(_p, 30)		// 言語種別
#define GET_MARK_TITLE_STR(_p)			(char*)(&_p[31])		// データタイトル文字列

#define GET_MARK_NEXT_HDL(_p)			(MARK_HDL)(_p + (GET_MARK_ELM_SIZE(_p)*4))	// 次の記号背景ハンドル

//-----------------------------------------------------------------------------
// クラス
//-----------------------------------------------------------------------------
/**
 * @brief  記号背景解析クラス
 */
class SMMarkAnalyze
{
private:
	UINT32		m_ParcelID;		// パーセルID
	char*		m_pMark;		// 記号背景データカラム

	UINT32		m_mark_cnt;		// 記号背景要素数

	MARK_HDL	m_FirstMarkHdl;	// 先頭記号背景ハンドル
//	MARK_HDL*	m_pMarkHdlList;	// 記号背景ハンドルリスト

	E_SC_RESULT InitMark(void);
//	UINT16 SearchMarkIndex(UINT32 MarkID);
public:
	/**
	 * @brief コンストラクタ
	 */
	SMMarkAnalyze(void);
	/**
	 * @brief デストラクタ
	 */
	~SMMarkAnalyze(void);

	/**
	 * @brief 初期化
	 * @param[in] ParcelID パーセルID
	 * @param[in] p_Mark 記号背景データカラム
	 * @return 処理結果
	 */
	E_SC_RESULT Initialize(const UINT32 ParcelID, const char* p_Mark);

	/**
	 * @brief 終了処理
	 * @return 処理結果
	 */
	E_SC_RESULT Finalize(void);

	/**
	 * @brief 記号背景要素数取得
	 * @return 記号背景要素数
	 */
	UINT32 GetMarkCnt(void);

//	/**
//	 * @brief 記号背景ハンドル取得
// 	 * @param[in] BkgdID 背景ID
//	 * @return 背景ハンドル
//	 */
//	MARK_HDL GetMarkHdl(UINT32 MarkID);

//	/**
//	 * @brief 記号背景ハンドル取得
// 	 * @param[in] Index インデックス
//	 * @return 記号背景ハンドル
//	 */
//	MARK_HDL GetMarkHdl(UINT16 Index);

	/**
	 * @brief 先頭背景記号ハンドル取得
	 * @return 背景記号ハンドル
	 */
	MARK_HDL FirstMarkHdl(void);

	/**
	 * @brief 次背景記号ハンドル取得
	 * @param[in] MARK_HDL 背景記号ハンドル
	 * @return 背景記号ハンドル
	 */
	MARK_HDL NextMarkHdl(MARK_HDL markHdl);
};
