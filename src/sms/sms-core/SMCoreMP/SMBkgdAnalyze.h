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
typedef char* BKGD_HDL;			// 背景ハンドル
typedef char* BKGD_OBJ_HDL;		// 背景オブジェクトハンドル


//-----------------------------------------------------------------------------
// マクロ
//-----------------------------------------------------------------------------
// 背景情報
#define GET_BKGD_ELM_SIZE(_p)			GetUINT16(_p, 0)		// 背景要素サイズ
#define GET_BKGD_OBJ_CNT(_p)			GetUINT16(_p, 2)		// オブジェクト数

#define GET_BKGDOBJ_SIZE(_p)			GetUINT16(_p, 0)		// オブジェクトサイズ
#define GET_BKGDOBJ_INFO(_p)			GetUINT16(_p, 2)		// ズーム許可フラグ～
#define GET_BKGDOBJ_SORT_ID(_p)			GetUINT32(_p, 4)		// ソートID～
#define GET_BKGDOBJ_ID(_p)				GetUINT32(_p, 8)		// 背景ID

#define GET_BKGDOBJ_POINT_CNT(_p)		GetUINT16(_p, 12)		// 点数
#define GET_BKGDOBJ_POINT_INFO(_p)		GetUINT16(_p, 14)		// データ形式
#define GET_BKGDOBJ_POINT_X(_p)			GetUINT16(_p, 16)		// 始点座標X
#define GET_BKGDOBJ_POINT_Y(_p)			GetUINT16(_p, 18)		// 始点座標Y
#define GET_BKGDOBJ_OFS_X(_p,_i)		GetINT8(_p, (20+(2*(_i))))		// オフセットX
#define GET_BKGDOBJ_OFS_Y(_p,_i)		GetINT8(_p, (20+(2*(_i))+1))	// オフセットY

#define GET_BKGDOBJ_PENUPFLG(_p,_i)		((GetUINT16(_p, (16+(4*(_i)))) & 0x8000) >> 15)	// ペンアップフラグ
#define GET_BKGDOBJ_POINT_X2(_p,_i)		(GetUINT16(_p, (16+(4*(_i)))) & 0x1FFF)			// 絶対座標X
#define GET_BKGDOBJ_POINT_Y2(_p,_i)		(GetUINT16(_p, (16+(4*(_i))+2)) & 0x1FFF)		// 絶対座標Y

#define GET_BKGDOBJ_SSIZE(_p)			((GET_BKGD_POINT_CNT(_p)*2)+8)
#define GET_BKGDOBJD_3DOBJ(_p)			GetUINT32((_p+GET_BKGD_SSIZE(_p)), 0)	// 3DオブジェクトID
#define GET_BKGDOBJ_ADD_INFO(_p)		GetUINT32((_p+GET_BKGD_SSIZE(_p)), 4)	// 付加情報

#define GET_BKGD_NEXT_HDL(_p)			(BKGD_HDL)(_p + (GET_BKGD_ELM_SIZE(_p)*4))	// 次の背景ハンドル

#define GET_BKGDOBJ_FIRST_HDL(_p)		(BKGD_OBJ_HDL)(_p+4)							// 先頭背景オブジェクトハンドル
#define GET_BKGDOBJ_NEXT_HDL(_p)		(BKGD_OBJ_HDL)(_p + (GET_BKGDOBJ_SIZE(_p)*4))	// 次の背景オブジェクトハンドル

#define GET_BKGDOBJ_OFS_P(_p)			(INT8*)(_p+20)							// オフセットポインタ
#define GET_BKGDOBJ_POINT_P(_p)			(UINT16*)(_p+16)							// 絶対座標ポインタ


//-----------------------------------------------------------------------------
// 構造体
//-----------------------------------------------------------------------------
/**
 * @brief  背景背景種別コード位置情報
 */
typedef struct _BKGD_KIND_CODE_POS_INFO {
	BKGD_HDL	h_bkgd;
	UINT32		cnt;
} T_BKGD_KIND_CODE_POS_INFO;

//-----------------------------------------------------------------------------
// クラス
//-----------------------------------------------------------------------------
/**
 * @brief  背景解析クラス
 */
class SMBkgdAnalyze
{
private:
	UINT32		m_ParcelID;		// パーセルID
	char*		m_pBkgd;		// 背景データカラム

	UINT32		m_bkgd_cnt;		// 背景要素数

	BKGD_HDL	m_FirstBkgdHdl;	// 先頭背景ハンドル
//	BKGD_HDL*	m_pBkgdHdlList;	// 背景ハンドルリスト

	T_BKGD_KIND_CODE_POS_INFO 	m_TypeInfo[MP_BKGD_KIND_CODE_TYPE_MAX];

	E_SC_RESULT InitBkgd(void);
//	UINT16 SearchBkgdIndex(UINT32 BkgdID);
public:
	/**
	 * @brief コンストラクタ
	 */
	SMBkgdAnalyze(void);
	/**
	 * @brief デストラクタ
	 */
	~SMBkgdAnalyze(void);

	/**
	 * @brief 初期化
	 * @param[in] ParcelID パーセルID
	 * @param[in] p_Bkgd 背景データカラム
	 * @return 処理結果
	 */
	E_SC_RESULT Initialize(const UINT32 ParcelID, const char* p_Bkgd);

	/**
	 * @brief 終了処理
	 * @return 処理結果
	 */
	E_SC_RESULT Finalize(void);

	/**
	 * @brief 背景要素数取得
	 * @return 背景要素数
	 */
	UINT32 GetBkgdCnt(void);

	/**
	 * @brief 先頭背景ハンドル取得
	 * @return 背景ハンドル
	 */
	BKGD_HDL FirstBkgdHdl(void);

	/**
	 * @brief 次背景ハンドル取得
	 * @param[in] 背景ハンドル
	 * @return 次背景ハンドル
	 */
	BKGD_HDL NextBkgdHdl(BKGD_HDL bkgdHdl);

	/**
	 * @brief 指定背景種別(分類ｺｰﾄﾞ)の背景ハンドル取得
	 * @param[in] bkgdKindCodeType 取得する背景種別コード(分類ｺｰﾄﾞ)
	 * @param[out] pCnt 分類ｺｰﾄﾞ毎の背景要素数
	 * @return 先頭背景ハンドル
	 */
	BKGD_HDL GetFirstBkgdHdl(UINT32 bkgdKindCodeType, UINT32* pCnt);
};
