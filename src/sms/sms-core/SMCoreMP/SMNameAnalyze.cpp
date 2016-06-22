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

SMNameAnalyze::SMNameAnalyze(void)
{
//	m_pNameHdlList = NULL;
	Finalize();
}

SMNameAnalyze::~SMNameAnalyze(void)
{
	Finalize();
}

E_SC_RESULT SMNameAnalyze::Initialize(const UINT32 ParcelID, const char* p_Name)
{
	// パラメータチェック
	if(INVALID_32 == ParcelID) {
		return (e_SC_RESULT_BADPARAM);
	}
	if(NULL == p_Name) {
		return (e_SC_RESULT_BADPARAM);
	}

	m_ParcelID = ParcelID;
	m_pName = (char*)p_Name;

	// 初期化
	InitName();

	return (e_SC_RESULT_SUCCESS);
}

E_SC_RESULT SMNameAnalyze::Finalize(void)
{
	m_ParcelID = INVALID_32;
	m_pName = NULL;

	m_name_cnt = 0;

	m_FirstNameHdl = NULL;

//	if(NULL != m_pNameHdlList) {
//		delete [] m_pNameHdlList;
//		m_pNameHdlList = NULL;
//	}

	return (e_SC_RESULT_SUCCESS);
}

E_SC_RESULT SMNameAnalyze::InitName(void)
{
	//NAME_HDL h_name;
	//UINT32 i;
	//UINT32 disp_name_size;

	// 記号背景要素数
	m_name_cnt = GetUINT32(m_pName, 4);

	m_FirstNameHdl = m_pName + 8;
#if 0
	m_pNameHdlList = new NAME_HDL [m_name_cnt];
	if(NULL == m_pNameHdlList) {
		return e_SC_RESULT_FAIL;
	}

	// 名称要素数分ループ
	for(i=0; i<m_name_cnt; i++) {
		// 表示名称サイズ
		disp_name_size = GET_NMLG_SIZE(h_name) * 4;

		m_pNameHdlList[i] = h_name;

		// 次へ
		h_name += disp_name_size;
	}
#endif

	return (e_SC_RESULT_SUCCESS);
}

UINT32 SMNameAnalyze::GetNameCnt(void)
{
	return (m_name_cnt);
}
#if 0
NAME_HDL SMNameAnalyze::GetNameHdl(UINT32 NameID)
{
	NAME_HDL h_name;
	UINT16 index;

	if(NULL == m_pNameHdlList) {
		return NULL;
	}

	index = SearchNameIndex(NameID);
	if(INVALID_16 == index) {
		return NULL;
	}

	h_name = GetNameHdl(index);
	if(NULL == h_name) {
		return NULL;
	}

	return h_name;
}

NAME_HDL SMNameAnalyze::GetNameHdl(UINT16 Index)
{
	return m_pNameHdlList[Index];
}
#endif

NAME_HDL SMNameAnalyze::FirstNameHdl(void)
{
	NAME_HDL h_name = m_FirstNameHdl;
	return (h_name);
}

NAME_HDL SMNameAnalyze::NextNameHdl(NAME_HDL nameHdl)
{
	INT32 size;
	NAME_HDL h_name;

	size = GET_NAME_SIZE(nameHdl) * 4;
	h_name = nameHdl + size;

	return (h_name);
}
#if 0
UINT16 SMNameAnalyze::SearchNameIndex(UINT32 NameID)
{
	UINT16 ResultIndex = INVALID_16;
	NAME_HDL h_name;
	UINT16 i;

	for(i=0; i<m_name_cnt; i++) {
		h_name = m_pNameHdlList[i];
		if(NameID == GET_NAME_ID(h_name)) {
			ResultIndex = i;
			break;
		}
	}

	return ResultIndex;
}
#endif
NMLG_HDL SMNameAnalyze::GetNmLgHdl(NAME_HDL hName, UINT32 No)
{
	NMLG_HDL h_nmlg;

	UINT32 lang_cnt = GET_NAME_LANG_CNT(hName);

	if(No > lang_cnt) {
		return (NULL);
	}

	h_nmlg = hName + 12;

	for(UINT32 i=0; i<No; i++) {
		h_nmlg += GET_NMLG_SIZE(h_nmlg);
	}

	return (h_nmlg);
}
