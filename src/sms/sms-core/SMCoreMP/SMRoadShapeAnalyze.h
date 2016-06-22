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
typedef char* RDSP_HDL;		// 道路形状ハンドル
typedef char* UPLV_HDL;		// 上位レベルリンク情報ハンドル
typedef char* ROUT_HDL;		// 道路路線情報ハンドル
typedef char* AREA_HDL;		// 地域クラス情報ハンドル


//-----------------------------------------------------------------------------
// マクロ
//-----------------------------------------------------------------------------
// リンク形状
#define GET_LKSP_DATA_SIZE(_p)			GetUINT16(_p, 0)		// データサイズ
#define GET_LKSP_UPDATE_NO(_p)			GetUINT16(_p, 2)		// 更新通番
#define GET_LKSP_LINK_ID(_p)			GetUINT32(_p, 4)		// リンクID
#define GET_LKSP_LINK_BASE_INFO1(_p)	GetUINT32(_p, 8)		// リンク基本情報1
#define GET_LKSP_LINK_BASE_INFO2(_p)	GetUINT32(_p, 12)		// リンク基本情報2
#define GET_LKSP_XX_INFO(_p)			GetUINT32(_p, 16)		// XX情報
#define GET_LKSP_POINT_CNT(_p)			GetUINT16(_p, 20)		// 点数
#define GET_LKSP_DISP_FLG(_p)			GetUINT16(_p, 22)		// 表示フラグ
#define GET_LKSP_POINT_X(_p,_i)			GetUINT16(_p, (24+(4*(_i))))	// 点X
#define GET_LKSP_POINT_Y(_p,_i)			GetUINT16(_p, (24+(4*(_i))+2))	// 点Y

#define GET_LKSP_POINT_P(_p)			(UINT16*)(_p+24)	// 形状ポインタ

#define GET_LKSP_NEXT_HDL(_p)			(RDSP_HDL)(_p + (GET_LKSP_DATA_SIZE(_p)*4))	// 次の道路形状ハンドル

// 上位レベルリンク情報
#define GET_UPLV_LINK_ID(_p,_i)			GetUINT32(_p, (4*(_i)))		// 上位レベルリンク情報-リンクID

// 道路路線情報
#define GET_ROUT_CNT(_p)				GetUINT16(_p, 0)		// 道路路線情報数
#define GET_ROUT_ID(_p,_i)				GetUINT32(_p, (4+(8*(_i))))		// 道路路線情報ID
#define GET_ROUT_OFS(_p,_i)				GetUINT32(_p, (4+(8*(_i))+4))	// 道路路線名称オフセット１

// 地域クラス情報
#define GET_AREA_CLS3(_p)				GetUINT16(_p, 0)		// 地域クラス3名称ID
#define GET_AREA_CLS2(_p)				GetUINT16(_p, 2)		// 地域クラス2名称ID


//-----------------------------------------------------------------------------
// 構造体
//-----------------------------------------------------------------------------
// 道路種別形状情報
typedef struct _ROAD_KIND_SHAPE_INFO {
	RDSP_HDL	pAddr;
	INT32		cnt;
} T_ROAD_KIND_SHAPE_INFO;


//-----------------------------------------------------------------------------
// クラス
//-----------------------------------------------------------------------------
/**
 * @brief  道路解析クラス
 */
class SMRoadShapeAnalyze
{
private:
	UINT32		m_ParcelID;		// パーセルID
	char*		m_pRoadShape;	// 道路形状バイナリ

	// 道路形状バイナリ
	UINT32		m_LinkCnt;		// リンク数(リンク形状)
	char*		m_pShapeRec;	// 形状データレコード先頭アドレス
	UINT32*		m_pShapeIndex;	// 形状データリンク索引情報のソート済み索引先頭アドレス
	//RDSP_HDL*	m_pRdSpHdlList;	// 道路形状ハンドルリスト
	T_ROAD_KIND_SHAPE_INFO m_ShapeInfo[16];	// 道路種別形状情報

	E_SC_RESULT InitRoadShape(void);
	RDSP_HDL SearchShapeHdl(UINT32 LinkID);
public:
	/**
	 * @brief コンストラクタ
	 */
	SMRoadShapeAnalyze();
	/**
	 * @brief デストラクタ
	 */
	~SMRoadShapeAnalyze();

	/**
	 * @brief 初期化
	 * @param[in] ParcelID パーセルID
 	 * @param[in] p_RoadShape 道路形状バイナリ
	 * @return 処理結果
	 */
	E_SC_RESULT Initialize(const UINT32 ParcelID, const char* p_RoadShape);

	/**
	 * @brief 終了処理
	 * @return 処理結果
	 */
	E_SC_RESULT Finalize(void);

	/**
	 * @brief リンク数取得
	 * @return 処理結果
	 */
	UINT32 GetLinkCnt(void);

	/**
	 * @brief 道路形状ハンドル取得
 	 * @param[in] LinkID リンクID
	 * @return 道路形状ハンドル
	 */
	RDSP_HDL GetRoadShapeHdl_ByLinkID(UINT32 LinkID);

	/**
	 * @brief 道路形状ハンドル取得
 	 * @param[in] OffSet 道路形状オフセット
	 * @return 道路形状ハンドル
	 */
	RDSP_HDL GetRoadShapeHdl_ByOffSet(UINT32 OffSet);

//	/**
//	 * @brief 道路形状ハンドル取得
//	 * @param[in] Index インデックス
//	 * @return 道路形状ハンドル
//	 */
//	RDSP_HDL GetRoadShapeHdl(UINT16 Index);

	/**
	 * @brief 上位レベルリンク情報ハンドル取得
 	 * @param[in] rdspHdl 道路形状ハンドル
	 * @return 上位レベルリンク情報ハンドル
	 */
	UPLV_HDL GetUpLevelLinkHdl(RDSP_HDL rdspHdl);

	/**
	 * @brief 道路路線情報ハンドル取得
 	 * @param[in] rdspHdl 道路形状ハンドル
	 * @return 道路路線情報ハンドル
	 */
	ROUT_HDL GetRouteHdl(RDSP_HDL rdspHdl);

	/**
	 * @brief 地域クラス情報ハンドル取得
 	 * @param[in] rdspHdl 道路形状ハンドル
	 * @return 地域クラス情報ハンドル
	 */
	AREA_HDL GetAreaHdl(RDSP_HDL rdspHdl);

	/**
	 * @brief 道路種別毎の道路形状ハンドル取得
 	 * @param[in] roadKind 道路種別
 	 * @param[in] cnt レコード数
	 * @return 道路形状ハンドル
	 */
	RDSP_HDL GetRoadShapeHdlOfRoadKind(UINT16 roadKind, UINT32& cnt);

	/**
	 * @brief 次の道路形状ハンドル取得
 	 * @param[in] rdspHdl 道路形状ハンドル
	 * @return 次の道路形状ハンドル
	 */
	RDSP_HDL NextRoadShapeHdl(RDSP_HDL rdspHdl);
};
