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

SMMarkAnalyze::SMMarkAnalyze(void)
{
//	m_pMarkHdlList = NULL;
	Finalize();
}

SMMarkAnalyze::~SMMarkAnalyze(void)
{
	Finalize();
}

E_SC_RESULT SMMarkAnalyze::Initialize(const UINT32 ParcelID, const char* p_Mark)
{
	// パラメータチェック
	if(INVALID_32 == ParcelID) {
		return (e_SC_RESULT_BADPARAM);
	}
	if(NULL == p_Mark) {
		return (e_SC_RESULT_BADPARAM);
	}

	m_ParcelID = ParcelID;
	m_pMark = (char*)p_Mark;

	// 初期化
	InitMark();

	return (e_SC_RESULT_SUCCESS);
}

E_SC_RESULT SMMarkAnalyze::Finalize(void)
{
	m_ParcelID = INVALID_32;
	m_pMark = NULL;

	m_mark_cnt = 0;

//	if(NULL != m_pMarkHdlList) {
//		delete [] m_pMarkHdlList;
//		m_pMarkHdlList = NULL;
//	}

	return (e_SC_RESULT_SUCCESS);
}

E_SC_RESULT SMMarkAnalyze::InitMark(void)
{
	//UINT32 i;
//	UINT32 mark_elm_size;

	// 記号背景要素数
	m_mark_cnt = GetUINT32(m_pMark, 4);

	m_FirstMarkHdl = m_pMark + 8;

//	m_pMarkHdlList = new MARK_HDL [m_mark_cnt];
//	if(NULL == m_pMarkHdlList) {
//		return e_SC_RESULT_FAIL;
//	}
//
//	// 背景要素数分ループ
//	for(i=0; i<m_mark_cnt; i++) {
//		// データサイズ（4バイト単位）
//		mark_elm_size = GET_MARK_ELM_SIZE(h_mark) * 4;
//
//		m_pMarkHdlList[i] = h_mark;
//
//		// 次へ
//		h_mark += mark_elm_size;
//	}

	return (e_SC_RESULT_SUCCESS);
}

UINT32 SMMarkAnalyze::GetMarkCnt(void)
{
	return (m_mark_cnt);
}

/*MARK_HDL SMMarkAnalyze::GetMarkHdl(UINT32 MarkID)
{
	MARK_HDL h_mark;
	UINT16 index;

	if(NULL == m_pMarkHdlList) {
		return NULL;
	}

	index = SearchMarkIndex(MarkID);
	if(INVALID_16 == index) {
		return NULL;
	}

	h_mark = GetMarkHdl(index);
	if(NULL == h_mark) {
		return NULL;
	}

	return h_mark;
}

MARK_HDL SMMarkAnalyze::GetMarkHdl(UINT16 Index)
{
	return m_pMarkHdlList[Index];
}*/

MARK_HDL SMMarkAnalyze::FirstMarkHdl(void)
{
	MARK_HDL h_mark = m_FirstMarkHdl;
	return (h_mark);
}

MARK_HDL SMMarkAnalyze::NextMarkHdl(MARK_HDL markHdl)
{
	INT32 size;
	MARK_HDL h_mark;

	size = GET_MARK_ELM_SIZE(markHdl) * 4;
	h_mark = markHdl + size;

	return (h_mark);
}

/*UINT16 SMMarkAnalyze::SearchMarkIndex(UINT32 MarkID)
{
	UINT16 ResultIndex = INVALID_16;
	MARK_HDL h_mark;
	UINT16 i;

	for(i=0; i<m_mark_cnt; i++) {
		h_mark = m_pMarkHdlList[i];
		if(MarkID == GET_MARK_ID(h_mark)) {
			ResultIndex = i;
			break;
		}
	}

	return ResultIndex;
}*/
