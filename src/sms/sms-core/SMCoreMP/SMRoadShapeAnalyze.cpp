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

//-----------------------------------------------------------------------------
// 定数
//-----------------------------------------------------------------------------
#define ROAD_KIND_CNT_MAX	16


//-----------------------------------------------------------------------------
// 関数
//-----------------------------------------------------------------------------
SMRoadShapeAnalyze::SMRoadShapeAnalyze()
{
//	m_pRdSpHdlList = NULL;
	Finalize();
}

SMRoadShapeAnalyze::~SMRoadShapeAnalyze()
{
	Finalize();
}

E_SC_RESULT SMRoadShapeAnalyze::Initialize(const UINT32 ParcelID, const char* p_RoadShape)
{
	// パラメータチェック
	if(INVALID_32 == ParcelID) {
		return (e_SC_RESULT_BADPARAM);
	}
	if(NULL == p_RoadShape) {
		return (e_SC_RESULT_BADPARAM);
	}

	m_ParcelID = ParcelID;
	m_pRoadShape = (char*)p_RoadShape;

	// 初期化
	InitRoadShape();

	return (e_SC_RESULT_SUCCESS);
}

E_SC_RESULT SMRoadShapeAnalyze::Finalize(void)
{
	m_ParcelID = INVALID_32;
	m_pRoadShape = NULL;

	m_pShapeRec = NULL;

//	if(NULL != m_pRdSpHdlList) {
//		delete [] m_pRdSpHdlList;
//		m_pRdSpHdlList = NULL;
//	}

	return (e_SC_RESULT_SUCCESS);
}

UINT32 SMRoadShapeAnalyze::GetLinkCnt(void)
{
	return (m_LinkCnt);
}

RDSP_HDL SMRoadShapeAnalyze::GetRoadShapeHdl_ByLinkID(UINT32 LinkID)
{
	RDSP_HDL h_rdsp;

	if(NULL == m_pShapeIndex) {
		return (NULL);
	}

	h_rdsp = SearchShapeHdl(LinkID);
	return (h_rdsp);
}

RDSP_HDL SMRoadShapeAnalyze::GetRoadShapeHdl_ByOffSet(UINT32 OffSet)
{
	RDSP_HDL h_road_shape = NULL;

	h_road_shape = m_pShapeRec + OffSet;

	return (h_road_shape);
}


//RDSP_HDL SMRoadShapeAnalyze::GetRoadShapeHdl(UINT16 Index)
//{
//	return *(&m_pRdSpHdlList[Index]);
//}

RDSP_HDL SMRoadShapeAnalyze::SearchShapeHdl(UINT32 LinkID)
{
	RDSP_HDL ResultIndex = NULL;
	INT32 lowid;
	INT32 midid;
	INT32 highid;
	UINT32 target = LinkID;

	// 添字の範囲を初期化
	highid	= m_LinkCnt-1;
	lowid	= 0;

	// 値が見つかるまで繰り返す
	while (lowid <= highid) {
		midid = (lowid + highid) / 2;

		// オフセットスからリンクID取得
		UINT32 offset = m_pShapeIndex[midid]*4;
		RDSP_HDL h_rdsp = m_pShapeRec + offset;
		UINT32 buf_link_id = GET_LKSP_LINK_ID(h_rdsp);

		if(buf_link_id == target) {
			// 見つかった
			ResultIndex = h_rdsp;
			break;
		}
		else if(buf_link_id < target) {
			lowid = midid + 1;
		}
		else {
			highid = midid - 1;
		}
	}

	return (ResultIndex);
}

E_SC_RESULT SMRoadShapeAnalyze::InitRoadShape(void)
{
	UINT32 i;
//	UINT32 j;
	//UINT32 rd_sp_index = 0;

	char* p_road;
	BA_VOLUM_INFO vol_info;
	char* p_binary;
	UINT16 all_shape_cnt;
	UINT32 road_type_ofs[ROAD_KIND_CNT_MAX];
	char* p_shape_data;
	UINT32 shape_data_size;
	UINT32 shape_data_cnt;
	RDSP_HDL h_rdsp;
//	UINT32 shape_size;

	if(NULL == m_pRoadShape) {
		return (e_SC_RESULT_SUCCESS);
	}

	// ■形状データレコード先頭アドレス
	m_pShapeRec = m_pRoadShape + 4 + 2 + 2 + 92;

	// 道路データ先頭アドレス
	p_road = m_pRoadShape;

	// ボリューム情報
	vol_info.d = GetUINT32(p_road, 0);

	// バイナリデータ先頭アドレス
	p_binary = &p_road[4];

	// リンク数取得
	UINT32 ofs = GetUINT32(&p_binary[4], 64);
	char* p_link_index = p_binary + ofs * 4;
	m_LinkCnt = GetUINT32(p_link_index, 4);

	// ■リンク索引レコードの先頭位置
	// リンク索引情報のソート済み索引まで移動
	m_pShapeIndex = (UINT32*)(&p_link_index[8]);

	// ■道路形状初期化
//	m_pRdSpHdlList = new RDSP_HDL [m_LinkCnt];
//	if(NULL == m_pRdSpHdlList) {
//		return e_SC_RESULT_FAIL;
//	}
//	memset(m_pRdSpHdlList, 0, sizeof(RDSP_HDL)*m_LinkCnt);

	// 全形状データレコード数
	all_shape_cnt = GetUINT16(p_binary, 0);

	// 先頭位置情報(16*4 + 4 + 4*5 + 4)
	for(i=0; i<ROAD_KIND_CNT_MAX; i++) {
		road_type_ofs[i] = GetUINT32(&p_binary[4], i*4);
	}

	memset(&m_ShapeInfo, 0, sizeof(m_ShapeInfo));

	for(i=0; i<ROAD_KIND_CNT_MAX; i++) {
		// 形状データレコード先頭アドレス
		if(INVALID_32 == road_type_ofs[i]) {
			continue;
		}
		p_shape_data = p_binary + road_type_ofs[i] * 4;

		// データサイズ（4バイト単位）
		shape_data_size = GetUINT32(p_shape_data, 0) * 4;

		// 形状レコード数
		shape_data_cnt = GetUINT32(p_shape_data, 4);

		// 形状1先頭アドレス(道路形状ハンドル)
		h_rdsp = &p_shape_data[8];

		m_ShapeInfo[15-i].pAddr = h_rdsp;
		m_ShapeInfo[15-i].cnt = shape_data_cnt;

//		// 形状レコード数分ループ
//		for(j=0; j<shape_data_cnt; j++) {
//			// データサイズ（4バイト単位）
//			shape_size = GET_LKSP_DATA_SIZE(h_rdsp) * 4;
//
//			m_pRdSpHdlList[rd_sp_index] = h_rdsp;
//			rd_sp_index++;
//
//			// 次の形状へ
//			h_rdsp += shape_size;
//
//		}
	}

	return (e_SC_RESULT_SUCCESS);
}

UPLV_HDL SMRoadShapeAnalyze::GetUpLevelLinkHdl(RDSP_HDL rdspHdl)
{
	UPLV_HDL h_uplv = rdspHdl + 20;
	h_uplv += (4+GET_LKSP_POINT_CNT(rdspHdl)*4);	// 図形データサイズ

	return (h_uplv);
}

ROUT_HDL SMRoadShapeAnalyze::GetRouteHdl(RDSP_HDL rdspHdl)
{
	ROUT_HDL h_rout;
	UPLV_HDL h_uplv = (ROUT_HDL)GetUpLevelLinkHdl(rdspHdl);

	BA_XX_INFO xx_info;
	xx_info.d = GET_LKSP_XX_INFO(rdspHdl);
	h_rout = h_uplv + xx_info.b.higher_link_cnt*4;	// 上位レベルリンク情報サイズ

	return (h_rout);
}

AREA_HDL SMRoadShapeAnalyze::GetAreaHdl(RDSP_HDL rdspHdl)
{
	AREA_HDL h_area;
	ROUT_HDL h_rout = (AREA_HDL)GetRouteHdl(rdspHdl);

	h_area = h_rout + (4+GET_ROUT_CNT(h_rout)*8);	// 道路路線情報サイズ

	return (h_area);
}

RDSP_HDL SMRoadShapeAnalyze::GetRoadShapeHdlOfRoadKind(UINT16 roadKind, UINT32& cnt)
{
	if(roadKind >= 16) {
		cnt = 0;
		return (NULL);
	}

	cnt = m_ShapeInfo[roadKind].cnt;
	return (m_ShapeInfo[roadKind].pAddr);
}

RDSP_HDL SMRoadShapeAnalyze::NextRoadShapeHdl(RDSP_HDL rdspHdl)
{
	UINT32 size = GET_LKSP_DATA_SIZE(rdspHdl);
	RDSP_HDL h_rdsp = rdspHdl + (size*4);
	return (h_rdsp);
}
