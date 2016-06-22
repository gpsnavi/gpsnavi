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

SMBkgdAnalyze::SMBkgdAnalyze(void)
{
//	m_pBkgdHdlList = NULL;
	Finalize();
}

SMBkgdAnalyze::~SMBkgdAnalyze(void)
{
	Finalize();
}

E_SC_RESULT SMBkgdAnalyze::Initialize(const UINT32 ParcelID, const char* p_Bkgd)
{
	// パラメータチェック
	if(INVALID_32 == ParcelID) {
		return (e_SC_RESULT_BADPARAM);
	}
	if(NULL == p_Bkgd) {
		return (e_SC_RESULT_BADPARAM);
	}

	m_ParcelID = ParcelID;
	m_pBkgd = (char*)p_Bkgd;

	// 初期化
	InitBkgd();

	return (e_SC_RESULT_SUCCESS);
}

E_SC_RESULT SMBkgdAnalyze::Finalize(void)
{
	m_ParcelID = INVALID_32;
	m_pBkgd = NULL;

	m_bkgd_cnt = 0;

	m_FirstBkgdHdl = NULL;
//	if(NULL != m_pBkgdHdlList) {
//		delete [] m_pBkgdHdlList;
//		m_pBkgdHdlList = NULL;
//	}

	return (e_SC_RESULT_SUCCESS);
}

E_SC_RESULT SMBkgdAnalyze::InitBkgd(void)
{
	BKGD_HDL h_bkgd = NULL;
	BKGD_OBJ_HDL h_obj = NULL;
	UINT16 tmpTypeCode = INVALID_16;
	BA_BKGD_INFO2 info2;
	UINT32 i;
	//UINT32 bkgd_elm_size;

	// 背景要素数
	m_bkgd_cnt = GetUINT32(m_pBkgd, 4);

	m_FirstBkgdHdl = m_pBkgd + 8;
#if 0
	m_pBkgdHdlList = new BKGD_HDL [m_bkgd_cnt];
	if(NULL == m_pBkgdHdlList) {
		return e_SC_RESULT_FAIL;
	}

	// 背景要素数分ループ
	for(i=0; i<m_bkgd_cnt; i++) {
		// データサイズ（4バイト単位）
		bkgd_elm_size = GET_BKGD_ELM_SIZE(h_bkgd) * 4;

		m_pBkgdHdlList[i] = h_bkgd;

		// 次へ
		h_bkgd += bkgd_elm_size;
	}
#endif

	memset(m_TypeInfo, 0, sizeof(m_TypeInfo));

	h_bkgd = FirstBkgdHdl();

	// 背景要素数分ループ
	for(i=0; i<m_bkgd_cnt; i++) {
		// 現背景要素の先頭オブジェクトハンドル取得
		h_obj = GET_BKGDOBJ_FIRST_HDL(h_bkgd);

		info2.d = GET_BKGDOBJ_SORT_ID(h_obj);
		if(tmpTypeCode != info2.b.type_cd) {
			// 1つ前の背景要素と異なる場合
			m_TypeInfo[info2.b.type_cd].h_bkgd = h_bkgd;
			tmpTypeCode = info2.b.type_cd;
		}
		m_TypeInfo[info2.b.type_cd].cnt++;

		// 次へ
		h_bkgd = GET_BKGD_NEXT_HDL(h_bkgd);
	}

	return (e_SC_RESULT_SUCCESS);
}

UINT32 SMBkgdAnalyze::GetBkgdCnt(void)
{
	return (m_bkgd_cnt);
}
#if 0
BKGD_HDL SMBkgdAnalyze::GetBkgdHdl(UINT32 BkgdID)
{
	BKGD_HDL h_bkgd;
	UINT16 index;

	if(NULL == m_pBkgdHdlList) {
		return NULL;
	}

	index = SearchBkgdIndex(BkgdID);
	if(INVALID_16 == index) {
		return NULL;
	}

	h_bkgd = GetBkgdHdl(index);
	if(NULL == h_bkgd) {
		return NULL;
	}

	return h_bkgd;
}

BKGD_HDL SMBkgdAnalyze::GetBkgdHdl(UINT16 Index)
{
	return m_pBkgdHdlList[Index];
}
#endif
BKGD_HDL  SMBkgdAnalyze::FirstBkgdHdl(void)
{
	BKGD_HDL h_bkgd = m_FirstBkgdHdl;
	return (h_bkgd);
}

BKGD_HDL  SMBkgdAnalyze::NextBkgdHdl(BKGD_HDL bkgdHdl)
{
	INT32 size;
	BKGD_HDL h_bkgd;

	size = GET_BKGD_ELM_SIZE(bkgdHdl) * 4;
	h_bkgd = bkgdHdl + size;

	return (h_bkgd);
}
#if 0
UINT16 SMBkgdAnalyze::SearchBkgdIndex(UINT32 BkgdID)
{
	UINT16 ResultIndex = INVALID_16;
	BKGD_HDL h_bkgd;
	UINT16 i;

	for(i=0; i<m_bkgd_cnt; i++) {
		h_bkgd = m_pBkgdHdlList[i];
		if(BkgdID == GET_BKGD_ID(h_bkgd)) {
			ResultIndex = i;
			break;
		}
	}

	return ResultIndex;
}
#endif

BKGD_HDL SMBkgdAnalyze::GetFirstBkgdHdl(UINT32 bkgdKindCodeType, UINT32* pCnt)
{
	*pCnt = m_TypeInfo[bkgdKindCodeType].cnt;
	return (m_TypeInfo[bkgdKindCodeType].h_bkgd);
}
