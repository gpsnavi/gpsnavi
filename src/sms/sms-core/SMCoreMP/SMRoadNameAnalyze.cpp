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

SMRoadNameAnalyze::SMRoadNameAnalyze(void)
{
	Finalize();
}

SMRoadNameAnalyze::~SMRoadNameAnalyze(void)
{
	Finalize();
}

E_SC_RESULT SMRoadNameAnalyze::Initialize(const UINT32 ParcelID, const char* p_RdNm)
{
	// パラメータチェック
	if(INVALID_32 == ParcelID) {
		return (e_SC_RESULT_BADPARAM);
	}
	if(NULL == p_RdNm) {
		return (e_SC_RESULT_BADPARAM);
	}

	m_ParcelID = ParcelID;
	m_pRdNm = (char*)p_RdNm;

	// 初期化
	InitRdNm();

	return (e_SC_RESULT_SUCCESS);
}

E_SC_RESULT SMRoadNameAnalyze::Finalize(void)
{
	m_ParcelID = INVALID_32;
	m_pRdNm = NULL;

	m_rdnm_cnt = 0;

	return (e_SC_RESULT_SUCCESS);
}

E_SC_RESULT SMRoadNameAnalyze::InitRdNm(void)
{
	RDNM_HDL h_rdnm;
	//UINT32 i;
	//UINT32 disp_rdnm_size;

	// 要素数
	m_rdnm_cnt = GetUINT32(m_pRdNm, 4);

	h_rdnm = m_pRdNm + 8;

	return (e_SC_RESULT_SUCCESS);
}

UINT32 SMRoadNameAnalyze::GetRdNmCnt(void)
{
	return (m_rdnm_cnt);
}

RDNM_HDL SMRoadNameAnalyze::GetRdNmHdlFromOffset(UINT32 Offset)
{
	return ((RDNM_HDL)&m_pRdNm[Offset]);
}

RNLG_HDL SMRoadNameAnalyze::GetRNLgHdl(RDNM_HDL hRdNm, UINT32 No)
{
	RNLG_HDL h_rnlg;

	UINT32 lang_cnt = GET_RDNM_LANG_CNT(hRdNm);

	if(No > lang_cnt) {
		return (NULL);
	}

	h_rnlg = hRdNm + 8;

	for(UINT32 i=0; i<No; i++) {
		h_rnlg += GET_RNLG_SIZE(h_rnlg);
	}

	return (h_rnlg);
}

char* SMRoadNameAnalyze::GetRouteNo(RDNM_HDL hRdNm, UINT32& Size)
{
	UINT32 ofs = 4/*8*/;

	Size = GetUINT16(hRdNm, ofs);
	char* p_str = (char*)(&hRdNm[6]);
	return (p_str);
}

char* SMRoadNameAnalyze::GetRouteName(RDNM_HDL hRdNm, UINT32& Size)
{
	UINT32 ofs = 4/*8*/;

	UINT32 routeno_size = GetUINT16(hRdNm, ofs);
	if(routeno_size % 2) {
		routeno_size +=1;
	}
	Size = GetUINT16(hRdNm, (ofs+2+routeno_size));
	char* p_str = (char*)(&hRdNm[(ofs+2+routeno_size+2)]);
	return (p_str);
}

char* SMRoadNameAnalyze::GetRouteNameYomi(RDNM_HDL hRdNm, UINT32& Size)
{
	UINT32 ofs = 4/*8*/;

	UINT32 routeno_size = GetUINT16(hRdNm, ofs);
	if(routeno_size % 2) {
		routeno_size +=1;
	}
	UINT32 routename_size = GetUINT16(hRdNm, (ofs+2+routeno_size));
	if(routename_size % 2) {
		routename_size +=1;
	}
	Size = GetUINT16(hRdNm, (ofs+2+routeno_size+2+routename_size));
	char* p_str = (char*)(&hRdNm[(ofs+2+routeno_size+2+routename_size+2)]);
	return (p_str);
}

char* SMRoadNameAnalyze::GetStrFromOffset(UINT16 type, MP_LANG_CODE_e langCode, UINT32 offset, UINT32& Size)
{
	static char str_utf8[256];
	char* p_route_str= NULL;

	Size = 0;

	// 道路名称ハンドル取得
	RDNM_HDL h_rdnm = GetRdNmHdlFromOffset(offset);
	if (NULL == h_rdnm) {
		return (NULL);
	}

	UINT16 langCnt = GET_RDNM_LANG_CNT(h_rdnm);
	if (0 == langCnt) {
		return (NULL);
	}

	for (UINT16 i=0; i<langCnt; i++) {
		RNLG_HDL h_rnlg = GetRNLgHdl(h_rdnm, i);
		if (NULL == h_rnlg) {
			return (NULL);
		}

		// 言語種別チェック
		if (langCode != GET_RNLG_LANG_KIND(h_rnlg)) {
			continue;
		}

		if (ROAD_NAME_TYPE_NAME == type) {
			p_route_str = GetRouteName(h_rnlg, Size);
			if (0 == Size) {
				return (NULL);
			}
		}
		else if (ROAD_NAME_TYPE_NO == type) {
			p_route_str = GetRouteNo(h_rnlg, Size);
			if (0 == Size) {
				return (NULL);
			}

			// ★暫定
			// 5文字より多い場合は処理しない
			if (5 < Size) {
				Size = 0;
				return (NULL);
			}
		}

		// 文字列取得(UTF8) ::削除
		Size -= 2;
		strncpy(str_utf8, &p_route_str[1], Size);
		str_utf8[Size] = '\0';

		return (str_utf8);
	}

	return (NULL);
}

char* SMRoadNameAnalyze::GetRouteNameFromOffset(UINT32 offset, UINT32& Size)
{
	return (GetStrFromOffset(ROAD_NAME_TYPE_NAME, MP_LANG_CODE_JPN, offset, Size));
}

char* SMRoadNameAnalyze::GetRouteNoFromOffset(UINT32 offset, UINT32& Size)
{
	return (GetStrFromOffset(ROAD_NAME_TYPE_NO, MP_LANG_CODE_JPN, offset, Size));
}
