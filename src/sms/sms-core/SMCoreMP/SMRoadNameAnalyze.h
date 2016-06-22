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
typedef char* RDNM_HDL;		// 道路名称ハンドル
typedef char* RNLG_HDL;		// 言語名称ハンドル

//-----------------------------------------------------------------------------
// マクロ
//-----------------------------------------------------------------------------
// 表示名称
#define GET_RDNM_SIZE(_p)				GetUINT16(_p, 0)		// 道路名称サイズ
#define GET_RDNM_LANG_CNT(_p)			GetUINT16(_p, 2)		// 言語名称数
#define GET_RDNM_ID(_p)					GetUINT32(_p, 4)		// 道路路線情報ID

#define GET_RNLG_SIZE(_p)				GetUINT16(_p, 0)		// 名称サイズ
#define GET_RNLG_LANG_KIND(_p)			GetUINT8(_p, 2)			// 言語種別
#define GET_RNLG_GUIDE_VOICE_ID(_p)		GetUINT32(_p, 4)		// 誘導音声ID
/*#define GET_RNLG_ROUTE_NO_SIZE(_p)		GetUINT16(_p, 8)		// 路線番号サイズ
#define GET_RNLG_ROUTE_NO_STR(_p)		(char*)(&_p[10])		// 路線番号文字列
#define GET_RNLG_ROUTE_NAME_SIZE(_p)	GetUINT16(_p, (8+2+GET_RNLG_ROUTE_NO_SIZE(_p)))		// 路線名称文字列サイズ
#define GET_RNLG_ROUTE_NAME_STR(_p)		GetUINT8(_p, 8)			// 路線名称文字列
#define GET_RNLG_ROUTE_YOMI_SIZE(_p)	GetUINT16(_p, 8)		// 路線名称文字列読みサイズ
#define GET_RNLG_ROUTE_YOMI_STR(_p)		GetUINT8(_p, 8)			// 路線名称文字列読み*/


//-----------------------------------------------------------------------------
// クラス
//-----------------------------------------------------------------------------
/**
 * @brief  道路名称解析クラス
 */
class SMRoadNameAnalyze
{
private:
	// 道路名称タイプ
	static const int ROAD_NAME_TYPE_NAME	= 0;	// 路線名称
	static const int ROAD_NAME_TYPE_NO		= 1;	// 路線番号

	UINT32		m_ParcelID;		// パーセルID
	char*		m_pRdNm;		// 道路名称データカラム

	UINT32		m_rdnm_cnt;		// 道路名称要素数

//	RDNM_HDL*	m_pRdNmHdlList;	// 道路名称ハンドルリスト

	E_SC_RESULT InitRdNm(void);

	/**
	 * @brief 文字列取得
 	 * @param[in] type 道路名称タイプ
  	 * @param[in] langCode 言語種別
 	 * @param[in] offset オフセット
 	 * @param[out] Size サイズ
	 * @return 文字列
	 */
	char* GetStrFromOffset(UINT16 type, MP_LANG_CODE_e langCode, UINT32 offset, UINT32& Size);
public:
	/**
	 * @brief コンストラクタ
	 */
	SMRoadNameAnalyze(void);
	/**
	 * @brief デストラクタ
	 */
	~SMRoadNameAnalyze(void);

	/**
	 * @brief 初期化
	 * @param[in] ParcelID パーセルID
	 * @param[in] p_RdNm 道路名称データカラム
	 * @return 処理結果
	 */
	E_SC_RESULT Initialize(const UINT32 ParcelID, const char* p_RdNm);

	/**
	 * @brief 終了処理
	 * @return 処理結果
	 */
	E_SC_RESULT Finalize(void);

	/**
	 * @brief 道路名称要素数取得
	 * @return 道路名称要素数
	 */
	UINT32 GetRdNmCnt(void);

	/**
	 * @brief 道路名称ハンドル取得
 	 * @param[in] RdNmID 道路名称ID
	 * @return 道路名称ハンドル
	 */
	RDNM_HDL GetRdNmHdlFromOffset(UINT32 Offset);

	/**
	 * @brief 言語名称ハンドル取得
 	 * @param[in] hRdNm 名道路称ハンドル
 	 * @param[in] No 何番目
	 * @return 言語名称ハンドル
	 */
	RNLG_HDL GetRNLgHdl(RDNM_HDL hRdNm, UINT32 No);

	/**
	 * @brief 路線番号文字列取得
 	 * @param[in] hRdNm 道路名称ハンドル
 	 * @param[in] Size 何番目
	 * @return 路線番号文字列
	 */
	static char* GetRouteNo(RDNM_HDL hRdNm, UINT32& Size);

	/**
	 * @brief 路線名称文字列取得
 	 * @param[in] hRdNm 道路名称ハンドル
 	 * @param[in] Size 何番目
	 * @return 路線名称文字列
	 */
	static char* GetRouteName(RDNM_HDL hRdNm, UINT32& Size);

	/**
	 * @brief路線名称文字列読み取得
 	 * @param[in] hRdNm 道路名称ハンドル
 	 * @param[in] Size 何番目
	 * @return 路線名称文字列読み
	 */
	static char* GetRouteNameYomi(RDNM_HDL hRdNm, UINT32& Size);

	/**
	 * @brief 路線名称文字列取得
 	 * @param[in] offset オフセット
 	 * @param[out] Size サイズ
	 * @return 路線名称
	 */
	char* GetRouteNameFromOffset(UINT32 offset, UINT32& Size);

	/**
	 * @brief 路線番号文字列取得
 	 * @param[in] offset オフセット
 	 * @param[out] Size サイズ
	 * @return 路線番号
	 */
	char* GetRouteNoFromOffset(UINT32 offset, UINT32& Size);
};
