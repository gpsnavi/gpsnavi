/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

/*-----------------------------------------------------------------------------------------------*/
/* File：RG_GuideApi.c                                                                           */
/* Info：誘導情報外部ＩＦ                                                                        */
/*-----------------------------------------------------------------------------------------------*/

#include "sms-core/SMCoreDM/SMCoreDMInternal.h"

/**
 * @brief	誘導情報リスト取得 外部ＩＦ
 * @param	[I]reqNo
 * @param	[O]SMREALTIMEGUIDEDATA
 *
 */
E_SC_RESULT RG_API_GetGuideDataVol(UINT16 *listVol_p, UINT16 reqNo)
{
	E_SC_RESULT				ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT				jdg = e_SC_RESULT_SUCCESS;
	RT_TBL_MAIN_t			*guidetbl_p;

	SC_LOG_DebugPrint(SC_TAG_RG, SC_LOG_START);

	if (NULL == listVol_p) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 誘導テーブル参照開始
	guidetbl_p = RT_TBL_LockGuideTbl();
	if (NULL == guidetbl_p) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	do {
		// 要求番号無効
		if (reqNo >= guidetbl_p->guide_crs_real_vol) {
			SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 要求番号からの残交差点数
		*listVol_p = guidetbl_p->guide_crs_real_vol - reqNo;

	} while(0);

	// 誘導テーブル参照終了
	jdg = RT_TBL_UnLockGuideTbl();
	if (e_SC_RESULT_SUCCESS != jdg) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	SC_LOG_DebugPrint(SC_TAG_RG, SC_LOG_END);

	return (ret);

}

/**
 * @brief	誘導情報リスト取得 外部ＩＦ
 * @param	[I]reqNo
 * @param	[O]SMGUIDEDATA
 *
 */
E_SC_RESULT RG_API_GetGuideData(SMGUIDEDATA *guideData_p, UINT16 reqNo)
{
	E_SC_RESULT				ret = e_SC_RESULT_FAIL;
	E_SC_RESULT				jdg = e_SC_RESULT_FAIL;
	RG_CTL_MAIN_t			*guidectl_p = NULL;
	RT_TBL_MAIN_t			*guidetbl_p;
	RT_TBL_ROUTELINK_t		*route_link_p;
	RT_TBL_GUIDELINK_t		*guide_link_p;
	RT_TBL_GUIDECRS_t		*guide_crs_p;
	RT_XY_t					*guide_sharp_p;
	DOUBLE					x,y,lat,lon;
	UINT32					total_dist = 0;
	UINT16					ilp,jlp;

	SC_LOG_DebugPrint(SC_TAG_RG, SC_LOG_START);

	if (NULL == guideData_p) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 誘導テーブル参照開始
	guidetbl_p = RT_TBL_LockGuideTbl();
	if (NULL == guidetbl_p) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	do {
		// 出力テーブルの初期化
		memset(guideData_p, 0x00, sizeof(SMGUIDEDATA));

		// 誘導制御テーブル取得
		guidectl_p = RG_CTL_GetCtlTbl();
		if (NULL == guidectl_p) {
			SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
			break;
		}
		// 要求番号無効
		if (reqNo >= guidetbl_p->guide_crs_real_vol) {
			break;
		}

		route_link_p = guidetbl_p->route_link_p;
		guide_link_p = guidetbl_p->guide_link_p;

		// 経路リンク数分ループ
		for (ilp = 0  ; ilp < guidetbl_p->make_no ; ilp++, route_link_p++) {

			// 経路リンク構成リンク情報数分ループ
			for (jlp = 0 ; jlp < route_link_p->link_vol ; jlp++, guide_link_p++) {

				// リンク長加算
				total_dist += guide_link_p->dist;

				// 交差点情報なし
				if (ALLF16 == guide_link_p->crs_offset) {
					continue;
				}

				// リンク終点の交差点情報取得
				guide_crs_p = (guidetbl_p->guide_crs_p + guide_link_p->crs_offset);

				// 非誘導交差点
				if (ALLF16 == guide_crs_p->crs_no) {
					continue;
				}

				// 要求番号ではない
				if (reqNo != guide_crs_p->crs_no) {
					continue;
				}

				// 通過済みの交差点が要求された
				if (total_dist < guidectl_p->passed_dist) {
					ilp = guidetbl_p->make_no;
					break;
				}
				total_dist -= guidectl_p->passed_dist;


				if (CRSTYPE_WAYPT == guide_crs_p->crs_type) {
					guideData_p->turnDir = 22;										// 経由地
					guideData_p->wayPointNo = (guidetbl_p->route_point_p + route_link_p->route_point_no)->sect_no + 1;
				}else if (CRSTYPE_DEST == guide_crs_p->crs_type) {
					guideData_p->turnDir = 23;										// 目的地
				}else if (CRSTYPE_SPLIT == guide_crs_p->crs_type) {
					guideData_p->turnDir = TURN_ST;									// 経路断裂点
				}else {
					guideData_p->turnDir = RG_TBL_GetRelativeGuideDir(guide_crs_p);	// 案内方向
				}

				// 交差点緯度経度
				// 誘導リンク形状点情報から当該リンクの端点情報を取得
				guide_sharp_p  = (guidetbl_p->guide_sharp_p + guide_link_p->sharp_offset);
				guide_sharp_p += (guide_link_p->sharp_vol - 1);

				// 交差点座標
				x = (DOUBLE)guide_sharp_p->x;
				y = (DOUBLE)guide_sharp_p->y;
				if(0 != SC_MESH_ChgParcelIDToTitude(1, guide_link_p->parcel_id, x, y, &lat, &lon)){
					SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
					ilp = guidetbl_p->make_no;
					break;
				}
				guideData_p->coord.latitude  = (INT32)(lat * 1024);
				guideData_p->coord.longitude = (INT32)(lon * 1024);
				guideData_p->remainDist      = total_dist;							// 残距離 (交差点)
				guideData_p->roadSituation   = RT_TBL_GetSituationType(guide_crs_p->crs_type); // 案内種別
				guideData_p->passNo          = guide_crs_p->crs_no;					 // 交差点番号

				// 交差点名称あり＆絶対方向時のみ
				if (guide_crs_p->tl_f) {
					guideData_p->showTrafficLight = true;							// 信号機あり
				}
				// 交差点名称
				if (guide_crs_p->crs_name.len > 0) {
					strcpy(&(guideData_p->name[0]), &(guide_crs_p->crs_name.name[0]));
				}

				ret = e_SC_RESULT_SUCCESS;
				ilp = guidetbl_p->make_no;
				break;

			}
		}

	} while(0);


	// 誘導テーブル参照終了
	jdg = RT_TBL_UnLockGuideTbl();
	if (e_SC_RESULT_SUCCESS != jdg) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		ret = e_SC_RESULT_FAIL;
	}

	SC_LOG_DebugPrint(SC_TAG_RG, SC_LOG_END);

	return (ret);

}
