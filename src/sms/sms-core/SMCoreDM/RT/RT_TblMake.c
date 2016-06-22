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
/* File：RT_TblMake.c                                                                            */
/* Info：誘導テーブル作成                                                                        */
/*-----------------------------------------------------------------------------------------------*/

#include "sms-core/SMCoreDM/SMCoreDMInternal.h"

static E_SC_RESULT RT_TBL_SetRoutePoint(RT_TBL_MAIN_t *, SC_RP_SectInfo *, SC_RP_FormInfo *, SC_RP_LinkInfo	*);
static E_SC_RESULT RT_TBL_SetGuideSharp(RT_TBL_MAIN_t *, RT_TBL_GUIDELINK_t *, RT_CROSSINFO_t *);
static E_SC_RESULT RT_TBL_SetGuideCrs(RT_TBL_MAIN_t *, RT_TBL_ROUTELINK_t *, RT_TBL_GUIDELINK_t *, RT_CROSSINFO_t *);
static E_SC_RESULT RT_TBL_RevCrossInfo(RT_TBL_MAIN_t *, RT_TBL_ROUTELINK_t *, RT_LINK_t *, RT_CROSSINFO_t *);
static E_SC_RESULT RT_TBL_RevGuideCrs(RT_TBL_MAIN_t *);
static UINT16      RT_TBL_GetGuideDir(RT_CROSSINFO_t *);
static E_SC_RESULT RT_TBL_SetRaExitNo(RT_TBL_MAIN_t *);
static E_SC_RESULT RT_TBL_RevHwyOut(RT_TBL_MAIN_t *);

static E_SC_RESULT RT_TBL_SetGuideCrossNo(RT_TBL_MAIN_t *);

/**
 * @brief	経路リンク情報設定
 * @param	[I]誘導テーブル
 */
E_SC_RESULT RT_TBL_SetRouteLink(RT_TBL_MAIN_t *guidetbl_p, SC_RP_RouteMng *mst_route_p)
{
	E_SC_RESULT				ret = e_SC_RESULT_SUCCESS;
	SC_RP_FormInfo			*mst_form_p;
	SC_RP_LinkInfo			*mst_link_p;
	SC_RP_ParcelInfo		*mst_parcel_p;
	SC_RP_SectInfo			*mst_sect_p;
	RT_TBL_ROUTELINK_t		*route_link_p;
	UINT16					ilp;
	UINT16					jlp;
	UINT16					klp;
	UINT32					total_dist;
	UINT32					total_time;

	SC_LOG_DebugPrint(SC_TAG_RT, SC_LOG_START);

	if (NULL == guidetbl_p) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 区間情報先頭アドレス取得
	mst_sect_p = mst_route_p->sectInfo;

	// 領域先頭アドレス取得
	route_link_p = guidetbl_p->route_link_p;

	// 累計距離/時間初期化
	total_dist = 0;
	total_time = 0;

	// 区間情報数分ループ
	for (ilp = 0 ; ilp < mst_route_p->sectVol ; ilp++, mst_sect_p++) {

		// 区間先頭パーセル情報アドレス取得
		mst_parcel_p = mst_route_p->parcelInfo + mst_sect_p->parcelIdx;

		// 区間先頭リンク情報アドレス取得
		mst_link_p = mst_route_p->linkInfo + mst_sect_p->linkIdx;

		// 区間先頭形状点情報アドレス取得
		mst_form_p = mst_route_p->formInfo + mst_sect_p->formIdx;

		// パーセル情報数分ループ
		for (jlp = 0 ; jlp < mst_sect_p->parcelVol ; jlp++, mst_parcel_p++) {

			// リンク情報数分ループ
			for (klp = 0 ; klp < mst_parcel_p->linkVol ; klp++, mst_link_p++) {

				// レベル２経路は断裂として非採用
				if (MAP_LEVEL2 == mst_link_p->level) {
					if (0 < guidetbl_p->route_link_vol) {
						// 断裂案内を行うためにSPLITフラグを強制的に立てる
						(route_link_p - 1)->split_f = SC_RP_SPLIT_LINK;
					}
					return (e_SC_RESULT_SUCCESS);
				}

				route_link_p->parcel_id      = mst_parcel_p->parcelId;		// パーセルID
				route_link_p->term_f         = mst_link_p->termFlag;		// 経路端点フラグ
				route_link_p->split_f        = mst_link_p->splitFlag;		// 経路断裂フラグ
				route_link_p->route_point_no = ilp;							// 地点情報オフセット
				route_link_p->link_id        = mst_link_p->linkId;			// リンクID
				route_link_p->link_dir       = mst_link_p->orFlag;			// リンク方向
				route_link_p->road_class     = mst_link_p->roadKind; 		// 道路種別
				route_link_p->link_type      = mst_link_p->linkKind; 		// リンク種別
				route_link_p->dist           = mst_link_p->dist;	 		// リンク長
				route_link_p->time           = mst_link_p->travelTime;		// 旅行時間
				route_link_p->total_dist     = total_dist;		 			// ここまでの距離
				route_link_p->total_time     = total_time;					// ここまでの旅行時間
				route_link_p->link_offset    = ALLF32;						// 誘導リンク情報へのオフセット
				route_link_p->link_vol       = 0;							// 誘導リンク情報数

				// 累計距離加算
				total_dist += route_link_p->dist;
				total_time += route_link_p->time;

				// 経路地点情報設定
				if (mst_link_p->termFlag) {
					ret = RT_TBL_SetRoutePoint(guidetbl_p, mst_sect_p, mst_form_p, mst_link_p);
					if (e_SC_RESULT_SUCCESS != ret) {
						SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
						return (e_SC_RESULT_SUCCESS);
					}
				}

				// 経路リンク情報更新
				route_link_p++;

				// 経路リンク情報数更新
				guidetbl_p->route_link_vol++;

			}
		}
	}

	SC_LOG_DebugPrint(SC_TAG_RT, SC_LOG_END);

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief	誘導リンク情報設定
 * @param	[I]誘導テーブル
 */
E_SC_RESULT RT_TBL_SetGuideLink(RT_TBL_MAIN_t *guidetbl_p, UINT32	make_dist)
{
	E_SC_RESULT					ret = e_SC_RESULT_SUCCESS;
	RT_TBL_ROUTELINK_t			*route_link_p;
	RT_TBL_GUIDELINK_t			*guide_link_p;
	static RT_CROSSINFO_t		crs;
	RT_MAPREQ_t					reqtbl;
	T_DHC_REQ_PARCEL 			mapReqPcl;
	RT_LINK_t					link_info;
	UINT32						total_dist = 0;
	//UINT16						route_point_no;
	UINT16						ilp;
	//UINT16						jlp;

	SC_LOG_DebugPrint(SC_TAG_RT, SC_LOG_START);

	if (NULL == guidetbl_p) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 完成済み
	if (guidetbl_p->make_no == guidetbl_p->route_link_vol) {
		return (ret);
	}

	// 地図要求テーブル初期化
	memset(&reqtbl, 0x00, sizeof(RT_MAPREQ_t));

	// 経路リンク情報先頭アドレス取得
	route_link_p = guidetbl_p->route_link_p + guidetbl_p->make_no;

	for (ilp = guidetbl_p->make_no ; ilp < guidetbl_p->route_link_vol ; ilp++, route_link_p++){

		// 指定距離分誘導リンク展開した
		if (make_dist < total_dist ) {
			// ラウンドアバウト内でない場合
			if (SC_MA_LINK_TYPE1_ROUNDABOUT != route_link_p->link_type) {
				break;
			}
		}

		// 誘導リンク展開距離加算
		total_dist += route_link_p->dist;

		// 地図未要求の場合
		if (route_link_p->parcel_id != reqtbl.data[4].parcel_id) {

			// 地図データ解放
			ret = RT_MAP_DataFree(&reqtbl);
			if (e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
				return (e_SC_RESULT_FAIL);
			}

			// 地図データ要求テーブル作成
			ret = RT_MAP_SetReqTbl(route_link_p->parcel_id, &mapReqPcl, &reqtbl);
			if (e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
				return (e_SC_RESULT_FAIL);
			}

			// 地図データ要求
			ret = RT_MAP_DataRead(&mapReqPcl, &reqtbl);
			if (e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
				return (e_SC_RESULT_FAIL);
			}
		}

		// 進入リンク情報取得
		link_info.parcel_id = route_link_p->parcel_id;
		link_info.link_id   = route_link_p->link_id;
		link_info.link_dir  = route_link_p->link_dir;

		// 地図データから交差点リンク情報取得
		ret = RT_MAP_GetCrossInfo(&reqtbl,&link_info, &crs);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
			return (e_SC_RESULT_FAIL);
		}

		// 退出リンク情報取得
		if ((guidetbl_p->route_link_vol - ilp) > 1) {
			link_info.parcel_id = (route_link_p + 1)->parcel_id;
			link_info.link_id   = (route_link_p + 1)->link_id;
			link_info.link_dir  = (route_link_p + 1)->link_dir;
			// 交差点リンク情報補正
			ret = RT_TBL_RevCrossInfo(guidetbl_p, route_link_p, &link_info, &crs);
		} else {
			// 交差点リンク情報補正
			ret = RT_TBL_RevCrossInfo(guidetbl_p, route_link_p, NULL, &crs);
		}
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
			return (e_SC_RESULT_FAIL);
		}

		// 更新対象の経路リンク情報取得
		route_link_p->link_offset = guidetbl_p->guide_link_vol;		// リンク情報オフセット
		route_link_p->link_vol    = 1;								// リンク情報数

		// 更新対象のリンク情報取得
		guide_link_p = (guidetbl_p->guide_link_p + route_link_p->link_offset);
		guide_link_p->parcel_id = crs.link[0].id.parcel_id;			// パーセルＩＤ
		guide_link_p->link_id   = crs.link[0].id.link_id;			// リンクＩＤ
		guide_link_p->dist      = crs.link[0].dist;					// リンク長
//		guide_link_p->time      = crs.link[0].time;					// 旅行時間
		guide_link_p->time      = route_link_p->time;				// 旅行時間 TODO

		// 誘導リンク形状点情報へのオフセット値初期化
		guide_link_p->sharp_offset = ALLF32;

		// 誘導リンク形状点情報設定
		ret = RT_TBL_SetGuideSharp(guidetbl_p, guide_link_p, &crs);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
			return (e_SC_RESULT_FAIL);
		}

		// 交差点情報番号初期化
		guide_link_p->crs_offset = ALLF16;

		// 交差点情報設定
		ret = RT_TBL_SetGuideCrs(guidetbl_p, route_link_p, guide_link_p, &crs);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
			return (e_SC_RESULT_FAIL);
		}

		// リンク情報数カウントＵＰ
		guidetbl_p->guide_link_vol++;

	}

	// 今回作成した経路リンク番号を設定
	guidetbl_p->make_no = ilp;

	// 地図データ解放 (後始末)
	ret = RT_MAP_DataFree(&reqtbl);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// ラウンドアバウト出口番号設定
	ret = RT_TBL_SetRaExitNo(guidetbl_p);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 高速出口補正
	ret = RT_TBL_RevHwyOut(guidetbl_p);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 交差点補正 (地図デジタイズ不正対応)
	ret = RT_TBL_RevGuideCrs(guidetbl_p);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 誘導交差点番号の設定
	ret = RT_TBL_SetGuideCrossNo(guidetbl_p);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	SC_LOG_DebugPrint(SC_TAG_RT, SC_LOG_END);
	return (ret);

}

/**
 * @brief	経路地点情報設定
 * @param	[I]誘導テーブル
 */
static E_SC_RESULT RT_TBL_SetRoutePoint(RT_TBL_MAIN_t *guidetbl_p, SC_RP_SectInfo *mst_sect_p, SC_RP_FormInfo *mst_form_p, SC_RP_LinkInfo *mst_link_p)
{

	RT_TBL_ROUTEPOINT_t		*route_point_p;
	UINT16					ilp;

	SC_LOG_DebugPrint(SC_TAG_RT, SC_LOG_START);

	if (NULL == guidetbl_p || NULL == mst_sect_p || NULL == mst_form_p || NULL == mst_link_p) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	if (NULL == guidetbl_p->route_point_p) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 地点情報先頭アドレス取得
	route_point_p = (guidetbl_p->route_point_p + guidetbl_p->route_point_vol);

	// 区間始点リンクの場合
	if (mst_link_p->termFlag & SC_RP_TERM_IS_FIRST) {
		// 始点リンク形状点座標情報設定
		for (ilp = 0 ; ilp < mst_link_p->formVol ; ilp++) {
			route_point_p->s_point.pos[ilp].x = (mst_form_p + mst_link_p->formIdx + ilp)->x;
			route_point_p->s_point.pos[ilp].y = (mst_form_p + mst_link_p->formIdx + ilp)->y;
		}
		route_point_p->s_point.vol = mst_link_p->formVol;
	}

	// 区間終点リンクの場合
	if (mst_link_p->termFlag & SC_RP_TERM_IS_LAST) {
		// 終点リンク形状点座標情報設定
		for (ilp = 0 ; ilp < mst_link_p->formVol ; ilp++) {
			route_point_p->e_point.pos[ilp].x = (mst_form_p + mst_link_p->formIdx + ilp)->x;
			route_point_p->e_point.pos[ilp].y = (mst_form_p + mst_link_p->formIdx + ilp)->y;
		}
		route_point_p->e_point.vol = mst_link_p->formVol;

		// 区間番号設定
		route_point_p->sect_no = mst_sect_p->sectNumber;

		// 区間距離/時間設定
		route_point_p->sect_dist = mst_sect_p->sectDist;
		route_point_p->sect_time = mst_sect_p->sectTime;

		// 地点座標情報設定
		route_point_p->posi.parcel_id = mst_sect_p->parcelId;
		route_point_p->posi.x = (DOUBLE)mst_sect_p->x;
		route_point_p->posi.y = (DOUBLE)mst_sect_p->y;

		// 経路地点情報数更新
		guidetbl_p->route_point_vol++;
	}

	SC_LOG_DebugPrint(SC_TAG_RT, SC_LOG_END);

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief	誘導リンク形状点情報設定
 */
static E_SC_RESULT RT_TBL_SetGuideSharp(RT_TBL_MAIN_t *guidetbl_p, RT_TBL_GUIDELINK_t *guide_link_p, RT_CROSSINFO_t *crs_p)
{

	RT_XY_t			*guide_sharp_p;
	UINT32			ilp;
	UINT16			point_vol;

	SC_LOG_DebugPrint(SC_TAG_RT, SC_LOG_START);

	if (NULL == guidetbl_p || NULL == guide_link_p || NULL == crs_p) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 入力リンク形状点数取得
	point_vol = crs_p->link[0].point.vol;

	// ＭＡＸチェック
	if (SHARPVOL_MAX <= (guidetbl_p->guide_sharp_vol + point_vol)) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 設定先交差点情報取得
	guide_sharp_p = (guidetbl_p->guide_sharp_p + guidetbl_p->guide_sharp_vol);

	// 誘導リンク情報の誘導リンク形状点情報へのオフセット設定
	guide_link_p->sharp_offset = guidetbl_p->guide_sharp_vol;

	// 誘導リンク情報の誘導リンク形状点情報数設定
	guide_link_p->sharp_vol = point_vol;

	// 誘導リンク形状点数分ループ
	for (ilp = 0 ; ilp < point_vol ; ilp++) {
		// 誘導リンク形状点情報へ設定
		*(guide_sharp_p + ilp) = crs_p->link[0].point.pos[ilp];
	}

	// 誘導リンク形状点情報数カウントアップ
	guidetbl_p->guide_sharp_vol += point_vol;

	SC_LOG_DebugPrint(SC_TAG_RT, SC_LOG_END);

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief	誘導交差点情報設定
 */
static E_SC_RESULT RT_TBL_SetGuideCrs(RT_TBL_MAIN_t *guidetbl_p, RT_TBL_ROUTELINK_t *route_link_p, RT_TBL_GUIDELINK_t *guide_link_p, RT_CROSSINFO_t *crs_p)
{

	RT_TBL_GUIDECRS_t			*guide_crs_p;
	UINT16						dir;
	UINT16						ilp;
	UINT32						point_no = 0;

	SC_LOG_DebugPrint(SC_TAG_RT, SC_LOG_START);

	if (NULL == guidetbl_p || NULL == route_link_p || NULL == guide_link_p || NULL == crs_p) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// ＭＡＸチェック
	if (CRSVOL_MAX <= guidetbl_p->guide_crs_vol) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 設定先交差点情報取得
	guide_crs_p = (guidetbl_p->guide_crs_p + guidetbl_p->guide_crs_vol);

	// 経路断裂点
	if (route_link_p->split_f & SC_RP_SPLIT_LINK) {
		guide_link_p->crs_offset   = guidetbl_p->guide_crs_vol;			// 交差点オフセット
		guide_crs_p->dir           = TURN_ST;							// 方向
		guide_crs_p->crs_type      = CRSTYPE_SPLIT;

		// 交差点情報数カウントＵＰ
		guidetbl_p->guide_crs_vol++;
	}
	// レベル２（想定しないデータ）
	else if (MAP_LEVEL2 == route_link_p->level) {
		guide_link_p->crs_offset   = guidetbl_p->guide_crs_vol;			// 交差点オフセット
		guide_crs_p->dir           = TURN_ST;							// 方向
		guide_crs_p->crs_type      = CRSTYPE_SPLIT;

		// 交差点情報数カウントＵＰ
		guidetbl_p->guide_crs_vol++;
	}
	// 地点進入
	else if (route_link_p->term_f & SC_RP_TERM_IS_LAST) {
		guide_link_p->crs_offset   = guidetbl_p->guide_crs_vol;			// 交差点オフセット
		guide_crs_p->dir           = TURN_ST;							// 方向

		// 目的地
		if ((route_link_p->route_point_no + 1) == guidetbl_p->route_point_vol) {
			guide_crs_p->crs_type = CRSTYPE_DEST;
		} else {
			// 地点番号取得
			point_no = (guidetbl_p->route_point_p + route_link_p->route_point_no)->sect_no + 1;
			guide_crs_p->crs_type = CRSTYPE_WAYPT;
		}

		// 交差点情報数カウントＵＰ
		guidetbl_p->guide_crs_vol++;
	}
	// 高速入口
	else if ((crs_p->link[0].road_type > SC_MA_ROAD_TYPE_TOLLWAY) && (crs_p->link[1].road_type <= SC_MA_ROAD_TYPE_TOLLWAY)){
		// ２差路の場合は直進とする
		if (2 >= crs_p->link_vol) {
			dir = TURN_ST;
		} else {
			dir = RT_TBL_GetGuideDir(crs_p);
			if (ALLF16 == dir) {
				return (e_SC_RESULT_FAIL);
			} else if (TURN_EX == dir) {
				return (e_SC_RESULT_SUCCESS);		// 例外時は非案内地点とする
			}
		}

		guide_crs_p->crs_type      = CRSTYPE_HWYIN;						// 高速入口
		guide_crs_p->dir           = dir;								// 案内方向
		guide_crs_p->tl_f          = crs_p->tl_f;						// 信号機
		guide_crs_p->crs_name      = crs_p->crs_name;					// 交差点名称
		guide_link_p->crs_offset   = guidetbl_p->guide_crs_vol;			// 交差点オフセット

		// 交差点情報数カウントＵＰ
		guidetbl_p->guide_crs_vol++;
	}
	// 高速出口
	else if ((crs_p->link[0].road_type <= SC_MA_ROAD_TYPE_TOLLWAY) && (crs_p->link[1].road_type > SC_MA_ROAD_TYPE_TOLLWAY)){
		// ２差路の場合は直進とする
		if (2 >= crs_p->link_vol) {
			dir = TURN_ST;
		} else {
			dir = RT_TBL_GetGuideDir(crs_p);
			if (ALLF16 == dir) {
				return (e_SC_RESULT_FAIL);
			} else if (TURN_EX == dir) {
				return (e_SC_RESULT_SUCCESS);		// 例外時は非案内地点とする
			}
		}

		guide_crs_p->crs_type      = CRSTYPE_HWYOT;						// 高速出口 TODO
		guide_crs_p->dir           = dir;								// 案内方向
		guide_crs_p->tl_f          = crs_p->tl_f;						// 信号機
		guide_crs_p->crs_name      = crs_p->crs_name;					// 交差点名称
		guide_link_p->crs_offset   = guidetbl_p->guide_crs_vol;			// 交差点オフセット

		// 交差点情報数カウントＵＰ
		guidetbl_p->guide_crs_vol++;
	}
	// ＲＡ入口
	else if ((crs_p->link[0].link_kind != SC_MA_LINK_TYPE1_ROUNDABOUT) && (crs_p->link[1].link_kind == SC_MA_LINK_TYPE1_ROUNDABOUT)){
		guide_crs_p->crs_type      = CRSTYPE_RA;						// ＲＡ
		guide_crs_p->dir           = TURN_RA;							// 案内方向

		guide_crs_p->tl_f          = crs_p->tl_f;						// 信号機
		guide_crs_p->crs_name      = crs_p->crs_name;					// 交差点名称
		guide_link_p->crs_offset   = guidetbl_p->guide_crs_vol;			// 交差点オフセット

		// 交差点情報数カウントＵＰ
		guidetbl_p->guide_crs_vol++;
	}
	// ＲＡ出口
	else if ((crs_p->link[0].link_kind == SC_MA_LINK_TYPE1_ROUNDABOUT) && (crs_p->link[1].link_kind != SC_MA_LINK_TYPE1_ROUNDABOUT)){
		// ２差路の場合は直進とする
		if (2 >= crs_p->link_vol) {
			dir = TURN_ST;
		} else {
			dir = RT_TBL_GetGuideDir(crs_p);
			if (ALLF16 == dir) {
				return (e_SC_RESULT_FAIL);
			} else if (TURN_EX == dir) {
				return (e_SC_RESULT_SUCCESS);		// 例外時は非案内地点とする
			}
		}

		guide_crs_p->crs_type      = CRSTYPE_NORMAL;					// 一般道交差点
		guide_crs_p->dir           = dir;								// 案内方向
		guide_crs_p->tl_f          = crs_p->tl_f;						// 信号機
		guide_crs_p->crs_name      = crs_p->crs_name;					// 交差点名称
		guide_link_p->crs_offset   = guidetbl_p->guide_crs_vol;			// 交差点オフセット

		// 交差点情報数カウントＵＰ
		guidetbl_p->guide_crs_vol++;
	}
	// ＲＡ内 (出口番号勘定用)
	else if ((crs_p->link[0].link_kind == SC_MA_LINK_TYPE1_ROUNDABOUT) && (crs_p->link[1].link_kind == SC_MA_LINK_TYPE1_ROUNDABOUT)){
		// ２差路の場合は交差点情報作成せず
		if (2 >= crs_p->link_vol) {
			return (e_SC_RESULT_SUCCESS);
		}

		// 非経路通行規制判定（退出可能な非経路が存在しない場合は交差点情報作成せず)
		for (ilp = 2 ; ilp < crs_p->link_vol ; ilp++) {
			// 退出方向＝リンク順方向
			if (0 == crs_p->link[ilp].id.link_dir) {
				// 通行可能
				if ((0 == crs_p->link[ilp].onway_code) || (1 == crs_p->link[ilp].onway_code)) {
					 break;
				}
			// 退出方向＝リンク逆方向
			} else {
				// 通行可能
				if ((0 == crs_p->link[ilp].onway_code) || (2 == crs_p->link[ilp].onway_code)) {
					 break;
				}
			}
		}
		if (ilp >= crs_p->link_vol) {
			return (e_SC_RESULT_SUCCESS);
		}

		guide_crs_p->crs_type      = CRSTYPE_NOTCRS;					// 非誘導交差点
		guide_crs_p->dir           = TURN_ST;							// 案内方向
		guide_crs_p->tl_f          = crs_p->tl_f;						// 信号機
		guide_crs_p->crs_name      = crs_p->crs_name;					// 交差点名称
		guide_link_p->crs_offset   = guidetbl_p->guide_crs_vol;			// 交差点オフセット

		// 交差点情報数カウントＵＰ
		guidetbl_p->guide_crs_vol++;
	}
	// 高速上分岐／一般道交差点
	else {
		// ２差路の場合は交差点情報作成せず
		if (2 >= crs_p->link_vol) {
			return (e_SC_RESULT_SUCCESS);
		}

		// 交差点案内方向取得
		dir = RT_TBL_GetGuideDir(crs_p);
		if (ALLF16 == dir) {
			return (e_SC_RESULT_FAIL);
		} else if (TURN_EX == dir) {
			return (e_SC_RESULT_SUCCESS);		// 例外時は非案内地点とする
		}

		// 非直進判断の妥当性チェック
		// 非経路通行規制判定（退出可能な非経路が存在しない場合は直進扱いとする)
		for (ilp = 2 ; ilp < crs_p->link_vol ; ilp++) {
			// 退出方向＝リンク順方向
			if (0 == crs_p->link[ilp].id.link_dir) {
				// 通行可能
				if ((0 == crs_p->link[ilp].onway_code) || (1 == crs_p->link[ilp].onway_code)) {
					 break;
				}
			// 退出方向＝リンク逆方向
			} else {
				// 通行可能
				if ((0 == crs_p->link[ilp].onway_code) || (2 == crs_p->link[ilp].onway_code)) {
					 break;
				}
			}
		}
		if (ilp >= crs_p->link_vol) {
			dir = TURN_ST;
		}

		// とりあえず直進以外の場合
		if (TURN_ST != dir) {
			// 高速上分岐
			if (crs_p->link[0].road_type <= SC_MA_ROAD_TYPE_TOLLWAY) {
				guide_crs_p->crs_type      = CRSTYPE_HWYJCT;				// 高速上分岐
				guide_crs_p->dir           = dir;							// 案内方向
				guide_crs_p->tl_f          = crs_p->tl_f;					// 信号機
				guide_crs_p->crs_name      = crs_p->crs_name;				// 交差点名称
				guide_link_p->crs_offset   = guidetbl_p->guide_crs_vol;		// 交差点オフセット

				// 交差点情報数カウントＵＰ
				guidetbl_p->guide_crs_vol++;
			}
			// 一般道交差点
			else {
				guide_crs_p->crs_type      = CRSTYPE_NORMAL;				// 一般道交差点 TODO
				guide_crs_p->dir           = dir;							// 案内方向
				guide_crs_p->tl_f          = crs_p->tl_f;					// 信号機
				guide_crs_p->crs_name      = crs_p->crs_name;				// 交差点名称
				guide_link_p->crs_offset   = guidetbl_p->guide_crs_vol;		// 交差点オフセット

				// 交差点情報数カウントＵＰ
				guidetbl_p->guide_crs_vol++;
			}
		}
	}

	SC_LOG_DebugPrint(SC_TAG_RT, SC_LOG_END);

	return (e_SC_RESULT_SUCCESS);

}

/**
 * @brief	誘導交差点情報設定
 */
static UINT16 RT_TBL_GetGuideDir(RT_CROSSINFO_t *crs_p)
{

	INT32						angle;
	DOUBLE						l_real_x;
	DOUBLE						l_real_y;
	DOUBLE						c_x,c_y,i_x,i_y,o_x,o_y;
	UINT16						dir;
	UINT16						ilp;
	UINT16						case_f = 0;
	UINT32						base_id;
	INT32						xsft,ysft;
	INT32						angle_wk;

	SC_LOG_DebugPrint(SC_TAG_RT, SC_LOG_START);

	if (NULL == crs_p) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (ALLF16);
	}

	l_real_x = RT_real_x;
	l_real_y = RT_real_y;

	// 進入パーセルを中心とした３×３左下パーセル算出
	base_id = SC_MESH_SftParcelId(crs_p->link[0].id.parcel_id, -1, -1);

	// 進入座標取得
	i_x  = (DOUBLE)(crs_p->link[0].point.pos[ crs_p->link[0].point.vol - 2 ].x + 4096);	// 左下パーセルからの相対値
	i_x *= l_real_x;																	// 実長変換
	i_y  = (DOUBLE)(crs_p->link[0].point.pos[ crs_p->link[0].point.vol - 2 ].y + 4096);	// 左下パーセルからの相対値
	i_y *= l_real_y;																	// 実長変換

	// 中心座標取得
	c_x  = (DOUBLE)(crs_p->link[0].point.pos[ crs_p->link[0].point.vol - 1 ].x + 4096);	// 左下パーセルからの相対値
	c_x *= l_real_x;																	// 実長変換
	c_y  = (DOUBLE)(crs_p->link[0].point.pos[ crs_p->link[0].point.vol - 1 ].y + 4096);	// 左下パーセルからの相対値
	c_y *= l_real_y;																	// 実長変換

	// 退出座標取得
	SC_MESH_GetAlterPos(base_id, crs_p->link[1].id.parcel_id, 1, (INT32 *)&xsft, (INT32 *)&ysft);
	o_x  = (DOUBLE)(crs_p->link[1].point.pos[1].x + (4096 * xsft));						// 左下パーセルからの相対値
	o_x *= l_real_x;																	// 実長変換
	o_y  = (DOUBLE)(crs_p->link[1].point.pos[1].y + (4096 * ysft));						// 左下パーセルからの相対値
	o_y *= l_real_y;																	// 実長変換

	// 進入座標と中心座標、もしくは中心座標と退出座標が同一の場合
	if ((CompareDouble(i_x, c_x) && CompareDouble(i_y, c_y))
	||  (CompareDouble(o_x, c_x) && CompareDouble(o_y, c_y))) {
		return (TURN_EX);		// TODO
	}

	// 角度計算
	angle = RT_LIB_GetCrossAngle(i_x, i_y, c_x, c_y, o_x, o_y);
	if ((0 > angle) || (360 < angle)) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (ALLF16);
	}

	// 案内方向取得
	dir = RT_TURNDIR_TBL[ RT_GET_ANGLE_NO(angle) ];

#if 0
	switch(dir){
	case TURN_ST:	SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] angle = %d (直進)" ,angle);break;
	case TURN_UT:	SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] angle = %d (直戻)" ,angle);break;
	case TURN_FR:	SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] angle = %d (右前)" ,angle);break;
	case TURN_R:	SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] angle = %d (右)"   ,angle);break;
	case TURN_BR:	SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] angle = %d (右後)" ,angle);break;
	case TURN_FL:	SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] angle = %d (左前)" ,angle);break;
	case TURN_L:	SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] angle = %d (左)"   ,angle);break;
	case TURN_BL:	SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] angle = %d (左後)" ,angle);break;
	}
#endif

	// 直進以外と判断
	if (TURN_ST != dir) { return (dir); }


	// 以下、直進補正
	for (ilp = 2 ; ilp < crs_p->link_vol ; ilp++) {
		// 歩道とかはスキップ
		if (SC_MA_ROAD_TYPE_ATHER <= crs_p->link[ilp].road_type) {
			continue;
		}

		// 退出方向＝リンク順方向
		if (0 == crs_p->link[ilp].id.link_dir) {
			// 通行不可
			if ((0 != crs_p->link[ilp].onway_code) && (1 != crs_p->link[ilp].onway_code)) {
				 continue;
			}
		// 退出方向＝リンク逆方向
		} else {
			// 通行不可
			if ((0 != crs_p->link[ilp].onway_code) && (2 != crs_p->link[ilp].onway_code)) {
				continue;
			}
		}

		// 退出座標取得
		SC_MESH_GetAlterPos(base_id, crs_p->link[ilp].id.parcel_id, 1, (INT32 *)&xsft, (INT32 *)&ysft);
		o_x  = (DOUBLE)(crs_p->link[ilp].point.pos[1].x + (4096 * xsft));			// 左下パーセルからの相対値
		o_x *= l_real_x;															// 実長変換
		o_y  = (DOUBLE)(crs_p->link[ilp].point.pos[1].y + (4096 * ysft));			// 左下パーセルからの相対値
		o_y *= l_real_y;															// 実長変換

		// 進入座標と中心座標、もしくは中心座標と退出座標が同一の場合
		if ((CompareDouble(i_x, c_x) && CompareDouble(i_y, c_y))
		||  (CompareDouble(o_x, c_x) && CompareDouble(o_y, c_y))) {
			continue;	// 進入or退出リンク長０の場合は、非経路と判定しない為、スキップ
		}

		// 角度計算
		angle_wk = RT_LIB_GetCrossAngle(i_x, i_y, c_x, c_y, o_x, o_y);
		if ((0 > angle_wk) || (360 < angle_wk)) {
			SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
			return (ALLF16);
		}

		// 非経路にも直進あり
		if (TURN_ST == RT_TURNDIR_TBL[ RT_GET_ANGLE_NO(angle_wk) ]){
			// 進入・退出は道路種別・リンク種別ともに一致
			if ((crs_p->link[0].road_type == crs_p->link[1].road_type) &&
				(crs_p->link[0].link_kind == crs_p->link[1].link_kind) ) {
				// 進入・非経路は道路種別かリンク種別が不一致
				if ((crs_p->link[0].road_type != crs_p->link[ilp].road_type) ||
					(crs_p->link[0].link_kind != crs_p->link[ilp].link_kind) ) {
					case_f = 0;		// 退出が道なりと判断
				} else {
					case_f = 2;		// 道なり方向は角度差で決める
				}
			}
			else {
				// 進入・非経路は道路種別かリンク種別が不一致
				if ((crs_p->link[0].road_type != crs_p->link[ilp].road_type) ||
					(crs_p->link[0].link_kind != crs_p->link[ilp].link_kind) ) {
					case_f = 2;		// 道なり方向は角度差で決める
				} else {
					case_f = 1;		// 非経路が道なりと判断
				}
			}

			switch (case_f) {
			case 0:
				break;
			case 1:
				// 角度同値の場合は変更せず
				if (angle != angle_wk) {
					if (angle < angle_wk) {
						dir = TURN_FR;	// 非経路から見て右寄り
					} else {
						dir = TURN_FL;	// 非経路から見て左寄り
					}
				}
				break;
			case 2:
				// 角度同値の場合は変更せず
				if (angle != angle_wk) {
					// 非経路の方が、退出より直進
					if ((abs(180 - angle)) > (abs(180 - angle_wk))) {
						// 退出は右寄り
						if (180 >= angle) {
							dir = TURN_FR;	// 非経路から見て右寄り
						// 退出は左寄り
						} else {
							dir = TURN_FL;	// 非経路から見て左寄り
						}
					}
				}
				break;
			default:
				break;
			}
		}
	}

	SC_LOG_DebugPrint(SC_TAG_RT, SC_LOG_END);

	return (dir);

}


/**
 * @brief	交差点リンク情報補正
 */
static E_SC_RESULT RT_TBL_RevCrossInfo(RT_TBL_MAIN_t *guidetbl_p, RT_TBL_ROUTELINK_t *route_link_p, RT_LINK_t *link_p, RT_CROSSINFO_t *crs_p)
{
	//E_SC_RESULT					ret = e_SC_RESULT_SUCCESS;
	RT_LINKINFO_t				linkinfo;
	INT16						ilp;
	INT16						jlp;
	INT16						cnt;

	SC_LOG_DebugPrint(SC_TAG_RT, SC_LOG_START);

	if (NULL == guidetbl_p || NULL == route_link_p || NULL == crs_p) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 経路始終点リンク形状点補正
	if (route_link_p->term_f & SC_RP_TERM_IS_FIRST) {
		crs_p->link[ 0 ].point = (guidetbl_p->route_point_p + route_link_p->route_point_no)->s_point;
		crs_p->link[ 0 ].dist  = route_link_p->dist;
		crs_p->link[ 0 ].time  = route_link_p->time;
	}
	if (route_link_p->term_f & SC_RP_TERM_IS_LAST) {
		crs_p->link[ 0 ].point = (guidetbl_p->route_point_p + route_link_p->route_point_no)->e_point;
		crs_p->link[ 0 ].dist  = route_link_p->dist;
		crs_p->link[ 0 ].time  = route_link_p->time;
	}

	// リンク方向と形状点座標の補正
	for (ilp = 0 ; ilp < crs_p->link_vol ; ilp++) {
		// 交差点進入リンクの場合
		if (0 == ilp) {
			// 交差点流入方向が逆方向
			if (1 == crs_p->link[ ilp ].id.link_dir) {
				// 補正前リンク情報を退避
				linkinfo = crs_p->link[ ilp ];

				// 経路リンク方向に沿った形状点座標の並びに補正
				for (cnt = 0, jlp = crs_p->link[ ilp ].point.vol - 1 ; jlp >= 0 ; jlp--, cnt++) {
					linkinfo.point.pos[ cnt ].x = crs_p->link[ ilp ].point.pos[ jlp ].x;
					linkinfo.point.pos[ cnt ].y = crs_p->link[ ilp ].point.pos[ jlp ].y;
				}

				// 補正した退避リンク情報を再設定
				crs_p->link[ ilp ] = linkinfo;
			}
		}
		// 交差点退出リンクの場合
		else {
			// 補正前リンク情報を退避
			linkinfo = crs_p->link[ ilp ];

			// 交差点流入方向を経路リンク方向（交差点流出方向）に変換
			linkinfo.id.link_dir = (~linkinfo.id.link_dir) & 0x0001;

			// 交差点流入方向が順方向
			if (0 == crs_p->link[ ilp ].id.link_dir) {
				// 経路リンク方向に沿った形状点座標の並びに補正
				for (cnt = 0, jlp = crs_p->link[ ilp ].point.vol - 1 ; jlp >= 0 ; jlp--, cnt++) {
					linkinfo.point.pos[ cnt ].x = crs_p->link[ ilp ].point.pos[ jlp ].x;
					linkinfo.point.pos[ cnt ].y = crs_p->link[ ilp ].point.pos[ jlp ].y;
				}
			}

			// 補正した退避リンク情報を再設定
			crs_p->link[ ilp ] = linkinfo;
		}
	}

	// 退出リンクなし＝経路終点
	if (NULL == link_p) {
		// 交差点情報を作成する必要がない為、進入リンク以外の情報は破棄
		crs_p->link_vol = 1;
		return (e_SC_RESULT_SUCCESS);
	}

	// 交差点進入リンク＝退出リンク (前区間終点リンク＝次区間始点リンク)
	if ((crs_p->link[ 0 ].id.parcel_id == link_p->parcel_id) &&
		(crs_p->link[ 0 ].id.link_id   == link_p->link_id)   ){
		// 交差点情報を作成する必要がない為、進入リンク以外の情報は破棄
		crs_p->link_vol = 1;
		return (e_SC_RESULT_SUCCESS);
	}

	// 配列１のリンク情報＝経路退出リンク
	if ((crs_p->link[ 1 ].id.parcel_id == link_p->parcel_id) &&
		(crs_p->link[ 1 ].id.link_id   == link_p->link_id)   ){
		return (e_SC_RESULT_SUCCESS);
	}

	// 配列１のリンク情報を退避
	linkinfo = crs_p->link[ 1 ];

	// 経路退出リンクを検索
	for (ilp = 2 ; ilp < crs_p->link_vol ; ilp++) {
		// 発見
		if ((crs_p->link[ ilp ].id.parcel_id == link_p->parcel_id) &&
			(crs_p->link[ ilp ].id.link_id   == link_p->link_id)   ){
			// リンク情報の入れ替え
			crs_p->link[ 1 ]   = crs_p->link[ ilp ];
			crs_p->link[ ilp ] = linkinfo;
			break;
		}
	}
	if (ilp >= crs_p->link_vol) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	SC_LOG_DebugPrint(SC_TAG_RT, SC_LOG_END);

	return (e_SC_RESULT_SUCCESS);
}


/**
 * @brief	誘導交差点補正
 * @param	[I]誘導テーブル
 */
static E_SC_RESULT RT_TBL_RevGuideCrs(RT_TBL_MAIN_t *guidetbl_p)
{
	RT_TBL_GUIDELINK_t			*guide_link_p;
	RT_TBL_GUIDECRS_t			*guide_crs_p;
	RT_XY_t						*guide_sharp_p;
	DOUBLE						l_real_x;
	DOUBLE						l_real_y;
	DOUBLE						ic_x,ic_y,oc_x,oc_y,ii_x,ii_y,oo_x,oo_y;
	UINT32						base_id;
	INT32						xsft,ysft;
	INT32						angle;
	INT16						ilp,jlp;
	INT16						i_no,o_no;
	UINT16						dir;
	UINT16						sharp_vol;

	if (NULL == guidetbl_p) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 誘導交差点２点(１点は目的地のはず)以上が判定条件
	if (1 > guidetbl_p->guide_crs_vol) {
		return (e_SC_RESULT_SUCCESS);
	}

	l_real_x = RT_real_x;
	l_real_y = RT_real_y;

	// 先頭誘導リンク情報取得
	guide_link_p = guidetbl_p->guide_link_p;

	// 誘導リンク情報数分ループ
	for (ilp = 0; ilp < guidetbl_p->guide_link_vol - 1; ilp++) {
		// 交差点情報なし
		if (ALLF16 == (guide_link_p + ilp)->crs_offset) {
			continue;
		}

		// 一般誘導交差点でない
		guide_crs_p = (guidetbl_p->guide_crs_p + (guide_link_p + ilp)->crs_offset);
		if (CRSTYPE_NORMAL != guide_crs_p->crs_type) {
			continue;
		}

		i_no = ilp;

		// 交差点進入リンクは10m以下
		if (CRSREV_DIST > (guide_link_p + ilp)->dist) {
			i_no = 0;								// 下限設定

			// 10m以上のリンクを経路始点方向にサーチ
			for (jlp = ilp - 1 ; jlp > 0 ; jlp--) {
				if (CRSREV_DIST <= (guide_link_p + jlp)->dist) {
					i_no = jlp;
					break;
				}
			}
		}

		o_no = (ilp+1);

		// 交差点退出リンクは10m以下
		if (CRSREV_DIST > (guide_link_p + (ilp+1))->dist) {
			o_no = guidetbl_p->guide_link_vol - 1;	// 上限設定

			// 10m以上のリンクを経路終点方向にサーチ
			for (jlp = ilp + 1 ; jlp < guidetbl_p->guide_link_vol ; jlp++) {
				if (CRSREV_DIST <= (guide_link_p + jlp)->dist) {
					o_no = jlp;
					break;
				}
			}
		}

//		SC_LOG_ErrorPrint(SC_TAG_RT, "i = %d->%d o = %d->%d", ilp, i_no, (ilp+1), o_no);

		// 角度補正の必要あり
		if ((i_no != ilp)||(o_no != (ilp+1))) {
			// 進入リンクの形状点情報取得
			guide_sharp_p = (guidetbl_p->guide_sharp_p + (guide_link_p + i_no)->sharp_offset);
			sharp_vol = (guide_link_p + i_no)->sharp_vol;

			// 進入パーセルを中心とした３×３左下パーセル算出
			base_id = SC_MESH_SftParcelId((guide_link_p + i_no)->parcel_id, -1, -1);

			// 進入座標取得
			ii_x  = (DOUBLE)(guide_sharp_p[sharp_vol - 2].x + 4096);	// 左下パーセルからの相対値
			ii_x *= l_real_x;											// 実長変換
			ii_y  = (DOUBLE)(guide_sharp_p[sharp_vol - 2].y + 4096);	// 左下パーセルからの相対値
			ii_y *= l_real_y;											// 実長変換

			// 中心座標取得
			ic_x  = (DOUBLE)(guide_sharp_p[sharp_vol - 1].x + 4096);	// 左下パーセルからの相対値
			ic_x *= l_real_x;											// 実長変換
			ic_y  = (DOUBLE)(guide_sharp_p[sharp_vol - 1].y + 4096);	// 左下パーセルからの相対値
			ic_y *= l_real_y;											// 実長変換

			// 退出リンクの形状点情報取得
			guide_sharp_p = (guidetbl_p->guide_sharp_p + (guide_link_p + o_no)->sharp_offset);
			sharp_vol = (guide_link_p + o_no)->sharp_vol;

			// 中心座標取得
			SC_MESH_GetAlterPos(base_id, (guide_link_p + o_no)->parcel_id, 1, (INT32 *)&xsft, (INT32 *)&ysft);
			oc_x  = (DOUBLE)(guide_sharp_p[0].x + (4096 * xsft));		// 左下パーセルからの相対値
			oc_x *= l_real_x;											// 実長変換
			oc_y  = (DOUBLE)(guide_sharp_p[0].y + (4096 * ysft));		// 左下パーセルからの相対値
			oc_y *= l_real_y;											// 実長変換

			// 退出座標取得
			oo_x  = (DOUBLE)(guide_sharp_p[1].x + (4096 * xsft));		// 左下パーセルからの相対値
			oo_x *= l_real_x;											// 実長変換
			oo_y  = (DOUBLE)(guide_sharp_p[1].y + (4096 * ysft));		// 左下パーセルからの相対値
			oo_y *= l_real_y;											// 実長変換

			// 進入座標と中心座標、もしくは中心座標と退出座標が同一の場合
			if ((CompareDouble(ii_x, ic_x) && CompareDouble(ii_y, ic_y))
			||  (CompareDouble(oo_x, oc_x) && CompareDouble(oo_y, oc_y))) {
				dir = TURN_ST;
			} else {
				// 角度計算 (進入リンクと退出リンクが接続されていない場合の角度計算)
				angle = RT_LIB_GetCrossAngle2(ii_x, ii_y, ic_x, ic_y, oc_x, oc_y, oo_x, oo_y);
				if ((0 > angle) || (360 < angle)) {
					SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
					return (e_SC_RESULT_FAIL);
				}

				// 案内方向取得
				dir = RT_TURNDIR_TBL[ RT_GET_ANGLE_NO(angle) ];

				// ターンバイターン案内の表示不正を防ぐため、退出リンクが存在する方向をチェック
				// TODO 海外対応時には自車の車線位置によって確認方向の追加が必要
				// 角度再計算 (進入リンクから見て退出リンクが存在する角度を計算)
				angle = RT_LIB_GetCrossAngle(ii_x, ii_y, ic_x, ic_y, oc_x, oc_y);
				if ((0 > angle) || (360 < angle)) {
					SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
					return (e_SC_RESULT_FAIL);
				}

				// 左後方向かつ、第一交差点時に右折(退出リンクへの角度が180度未満)している場合はUターンで固定
				if (TURN_BL == dir && 180 > angle) {
					dir = TURN_UT;
				}
				// Uターンかつ、第一交差点時に左折(退出リンクへの角度が180度超過)した場合は表示不正のため補正を行わない
				else if(TURN_UT == dir && 180 < angle){
					continue;
				}
			}

			// 補正基点交差点情報更新
			if (TURN_ST == dir) {
				// 第１交差点
				guide_crs_p = (guidetbl_p->guide_crs_p + (guide_link_p + ilp)->crs_offset);
				guide_crs_p->crs_type = CRSTYPE_NOTCRS;					// 非誘導交差点
				guide_crs_p->dir      = TURN_ST;						// 案内方向
			} else {
				// 第１交差点
				guide_crs_p = (guidetbl_p->guide_crs_p + (guide_link_p + ilp)->crs_offset);
				guide_crs_p->crs_type = CRSTYPE_NORMAL;					// 案内交差点
				guide_crs_p->dir      = dir;							// 案内方向
			}

			// 中継点交差点情報更新
			for (jlp = (ilp + 1) ; jlp < o_no; jlp++) {
				if (ALLF16 != (guide_link_p + jlp)->crs_offset) {
					guide_crs_p = (guidetbl_p->guide_crs_p + (guide_link_p + jlp)->crs_offset);
					guide_crs_p->crs_type = CRSTYPE_NOTCRS;				// 非誘導交差点
					guide_crs_p->dir      = TURN_ST;					// 案内方向
				}
			}
		}

	}

	return (e_SC_RESULT_SUCCESS);

}

/**
 * @brief	ラウンドアバウト出口番号設定
 * @param	[I]誘導テーブル
 */
static E_SC_RESULT RT_TBL_SetRaExitNo(RT_TBL_MAIN_t *guidetbl_p)
{
	RT_TBL_GUIDELINK_t			*guide_link_p;
	RT_TBL_GUIDECRS_t			*guide_crs_p;
	INT16						ilp;
	//INT16						jlp;
	UINT16						srch_no = ALLF16;
	UINT8						exit_no = 0;

	if (NULL == guidetbl_p) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 誘導交差点２点以上が判定条件
	if (2 > guidetbl_p->guide_crs_vol) {
		return (e_SC_RESULT_SUCCESS);
	}

	// 先頭誘導リンク情報取得
	guide_link_p = guidetbl_p->guide_link_p;

	// 誘導リンク情報数分ループ
	for (ilp = 0; ilp < guidetbl_p->guide_link_vol - 1; ilp++) {
		// 交差点情報なし
		if (ALLF16 == (guide_link_p + ilp)->crs_offset) {
			continue;
		}

		// 交差点情報取得
		guide_crs_p = (guidetbl_p->guide_crs_p + (guide_link_p + ilp)->crs_offset);

		if (ALLF16 == srch_no) {
			// ラウンドアバウト入口
			if ((CRSTYPE_RA == guide_crs_p->crs_type) && (0 == guide_crs_p->ra_exit_no)) {
				srch_no = (guide_link_p + ilp)->crs_offset;
			}
		}
		else {
			exit_no++;

			// ラウンドアバウト出口
			if (CRSTYPE_NOTCRS != guide_crs_p->crs_type) {
				// 出口は一般道
				if (CRSTYPE_NORMAL == guide_crs_p->crs_type) {
					// 入口で出口番号案内し、出口案内はしない
					guide_crs_p->crs_type = CRSTYPE_NOTCRS;
					guide_crs_p = (guidetbl_p->guide_crs_p + srch_no);
					guide_crs_p->ra_exit_no = exit_no;

					srch_no = ALLF16;
					exit_no = 0;
				}
				// 出口は上記以外
				else {
					// 入口で出口案内せず、出口案内はする
					srch_no = ALLF16;
					exit_no = 0;
				}
			}
		}
	}

	return (e_SC_RESULT_SUCCESS);

}

/**
 * @brief	案内種別取得
 * @param	[I]誘導テーブル交差点情報
 */
INT32 RT_TBL_GetSituationType(UINT16 crs_type)
{
	INT32	type;

	switch (crs_type) {
	case CRSTYPE_NORMAL:
	case CRSTYPE_HWYIN:
	case CRSTYPE_HWYOT:
	case CRSTYPE_HWYJCT:
	case CRSTYPE_RA:
		type = STYPE_GDCRS;
		break;

	case CRSTYPE_WAYPT:
		type = STYPE_WAYPT;
		break;

	case CRSTYPE_DEST:
		type = STYPE_DEST;
		break;

	case CRSTYPE_SPLIT:
		type = STYPE_SPLIT;
		break;

	default:
		type = STYPE_GDCRS;
	}
	return (type);
}

/**
 * @brief	高速出口補正
 * @param	[I]誘導テーブル
 */
static E_SC_RESULT RT_TBL_RevHwyOut(RT_TBL_MAIN_t *guidetbl_p)
{
	RT_TBL_ROUTELINK_t			*route_link_p;
	RT_TBL_GUIDELINK_t			*guide_link_p;
	RT_TBL_GUIDECRS_t			*guide_crs_p;
	INT16						ilp,jlp;
	UINT16						srch_no = ALLF16;

	if (NULL == guidetbl_p) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 誘導交差点２点以上が判定条件
	if (2 > guidetbl_p->guide_crs_vol) {
		return (e_SC_RESULT_SUCCESS);
	}

	// 先頭経路リンク情報取得
	route_link_p = guidetbl_p->route_link_p;

	// 最終経路リンクから検索する
	for (ilp = (guidetbl_p->route_link_vol - 1) ; ilp >= 0 ; ilp--) {

		guide_link_p = guidetbl_p->guide_link_p + (route_link_p + ilp)->link_offset;
		for (jlp = ((route_link_p + ilp)->link_vol - 1) ; jlp >= 0 ; jlp--) {

			// 交差点情報なし
			if (ALLF16 == (guide_link_p + jlp)->crs_offset) {
				continue;
			}

			// 交差点情報取得
			guide_crs_p = (guidetbl_p->guide_crs_p + (guide_link_p + jlp)->crs_offset);

			// 高速出口未発見時
			if (ALLF16 == srch_no) {
				// 高速出口
				if (CRSTYPE_HWYOT == guide_crs_p->crs_type) {
					// 進入リンクは本線でない
					if ((SC_MA_LINK_TYPE1_NORMAL   != (route_link_p + ilp)->link_type)  &&
						(SC_MA_LINK_TYPE1_SEPARATE != (route_link_p + ilp)->link_type)) {
						srch_no = (guide_link_p + jlp)->crs_offset;
					}
				}
			}
			// 高速出口未発見中
			else {

				// 進入リンクは本線でない
				if ((SC_MA_LINK_TYPE1_NORMAL   != (route_link_p + ilp)->link_type)  &&
					(SC_MA_LINK_TYPE1_SEPARATE != (route_link_p + ilp)->link_type)) {

					// 経路始点
					if ((0 == ilp) && ((route_link_p + ilp)->term_f & SC_RP_TERM_IS_FIRST)) {
						// 高速出口→一般道交差点に補正
						guide_crs_p = (guidetbl_p->guide_crs_p + srch_no);

						if (TURN_ST != guide_crs_p->dir) {
							guide_crs_p->crs_type = CRSTYPE_NORMAL;
						} else {
							guide_crs_p->crs_type = CRSTYPE_NOTCRS;
						}
						srch_no = ALLF16;
					}
					continue;
				}

				// 高速分岐
				if (CRSTYPE_HWYJCT == guide_crs_p->crs_type) {
					// 高速分岐→高速出口に補正
					guide_crs_p->crs_type = CRSTYPE_HWYOT;

					// 高速出口→一般道交差点に補正
					guide_crs_p = (guidetbl_p->guide_crs_p + srch_no);

					if (TURN_ST != guide_crs_p->dir) {
						guide_crs_p->crs_type = CRSTYPE_NORMAL;
					} else {
						guide_crs_p->crs_type = CRSTYPE_NOTCRS;
					}
				}
				srch_no = ALLF16;
			}
		}
	}

	return (e_SC_RESULT_SUCCESS);

}

/**
 * @brief 相対方向取得
 */
UINT8 RG_TBL_GetRelativeGuideDir(RT_TBL_GUIDECRS_t *guide_crs_p)
{

	UINT8	dir = TURN_ST;

	if (NULL == guide_crs_p) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (dir);
	}

	dir = guide_crs_p->dir;

	switch (guide_crs_p->crs_type) {
	case CRSTYPE_HWYIN:
	case CRSTYPE_HWYOT:
	case CRSTYPE_HWYJCT:
		// 案内方向の相対化
		switch (dir) {
		case TURN_UT:
		case TURN_BR:
		case TURN_R:
		case TURN_FR:
			dir = TURN_RR;
			break;

		case TURN_FL:
		case TURN_L:
		case TURN_BL:
			dir = TURN_RL;
			break;

		default:
			dir = TURN_ST;
			break;
		}
		break;
	default:
		break;
	}

	return (dir);

}

/**
 * @brief	誘導交差点補正
 * @param	[I]誘導テーブル
 */
static E_SC_RESULT RT_TBL_SetGuideCrossNo(RT_TBL_MAIN_t *guidetbl_p)
{
	RT_TBL_GUIDECRS_t		*guide_crs_p;
	UINT16					ilp;
	UINT16					crs_no = 0;

	if (NULL == guidetbl_p) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 交差点情報数分ループ
	for (ilp = 0 ; ilp < guidetbl_p->guide_crs_vol ; ilp++) {
		guide_crs_p = guidetbl_p->guide_crs_p + ilp;

		// 非誘導交差点でなければ番号を設定
		if (CRSTYPE_NOTCRS != guide_crs_p->crs_type) {
			guide_crs_p->crs_no = crs_no;
			crs_no++;
		} else {
			guide_crs_p->crs_no = ALLF16;
		}
	}

	// 非誘導交差点を除いた交差点情報数設定
	guidetbl_p->guide_crs_real_vol = crs_no;

	return (e_SC_RESULT_SUCCESS);
}

