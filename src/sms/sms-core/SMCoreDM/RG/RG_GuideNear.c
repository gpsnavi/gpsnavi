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
/* File：RG_NearGuideLinkGet.c                                                                   */
/* Info：最寄誘導リンク取得                                                                      */
/*-----------------------------------------------------------------------------------------------*/

#include "sms-core/SMCoreDM/SMCoreDMInternal.h"


static E_SC_RESULT RG_CTL_GetClipArea(RT_POSITION_t* posi_p, UINT16 size, RT_CLIPAREA_t *clip_area_p);

/**
 * @brief	最寄誘導リンク取得
 * @param	[I]なし
 */
E_SC_RESULT RG_CTL_GetNearLink(RT_TBL_MAIN_t *guidetbl_p, RG_CTL_MAIN_t *guidectl_p)
{

	E_SC_RESULT					ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_RPPOINT			point = {};
	RT_CLIPAREA_t				clip_area;
	RG_CTL_NEARLINK_t			link;
	RT_TBL_ROUTELINK_t			*route_link_p;
	RT_TBL_GUIDELINK_t			*guide_link_p;
	RT_XY_t						*guide_sharp_p;
	RT_POSITION_t				*posi_p;

	UINT32						total_len = 0;
	UINT32						min_v_len = ALLF32;
	UINT16						min_x,max_x;
	UINT16						min_y,max_y;
	UINT16						base_x,base_y;
	UINT16						x,y;
	UINT16						ilp,jlp,klp;
	UINT16						sect_no = 0;

	DOUBLE						cx;
	DOUBLE						cy;
	DOUBLE						sx;
	DOUBLE						sy;
	DOUBLE						ex;
	DOUBLE						ey;
	INT8 						rtn;
	DOUBLE 						dx;
	DOUBLE 						dy;
	DOUBLE 						l_len;
	DOUBLE						v_len;
	DOUBLE 						ratio;
	DOUBLE 						cxx;
	DOUBLE 						cyy;
	DOUBLE 						angle;
	DOUBLE 						len_v_dir;
	DOUBLE 						len_l_dir;
	DOUBLE						rx;
	DOUBLE						ry;
	INT32						car_dir;
	INT32						jdg_dir;



	if (NULL == guidetbl_p || NULL == guidectl_p) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 最寄リンク情報初期化
	memset(&link, ALLF8, sizeof(RG_CTL_NEARLINK_t));

	// 矩形エリア情報初期化
	memset(&clip_area, 0x00, sizeof(RT_CLIPAREA_t));

	// 自車を中心とした矩形エリア取得
	ret = RG_CTL_GetClipArea(&(guidectl_p->track[TRACK_NEW].car_posi), 40, &clip_area);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	rx = RT_real_x;
	ry = RT_real_y;

	// 自車位置座標の実長を算出
	cx  = (DOUBLE)clip_area.ct_pos.x;
	cx *= rx;					// 実長
	cy  = (DOUBLE)clip_area.ct_pos.y;
	cy *= ry;					// 実長

	car_dir = guidectl_p->track[TRACK_NEW].car_dir;

	ilp = jlp = 0;
	// 前回近傍リンクあり
	if ( guidectl_p->track[TRACK_OLD].link.route_link_no != ALLF16) {
		// シミュレーション判定
		if (guidectl_p->simulate_f) {
			// 前回近傍リンク含め先のリンクしか採択対象としない (大橋ＪＣＴ対応)
			ilp = guidectl_p->track[TRACK_OLD].link.route_link_no;
		}
	}

	// 経路地点情報取得
	ret = SC_MNG_GetAllRPPlace(&(point.point[0]), &point.pointNum);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 経路リンク情報先頭アドレス取得
	route_link_p = (guidetbl_p->route_link_p + ilp);

	// 経路リンク数分ループ
	for (; ilp < guidetbl_p->route_link_vol ; ilp++, route_link_p++) {

		// 区間番号取得
		sect_no = (guidetbl_p->route_point_p + route_link_p->route_point_no)->sect_no;

		// 通過済み区間のリンクは採択しない
		if (point.point[sect_no + 1].isPassed){
			continue;
		}

		// 対応する誘導リンク情報が作成されていない
		if (ALLF32 == route_link_p->link_offset) {
			break;
		}

		// 経路リンク構成リンク情報先頭アドレス取得
		guide_link_p = (guidetbl_p->guide_link_p + route_link_p->link_offset);

		// 経路リンク構成リンク情報数分ループ
		for (jlp = 0 ; jlp < route_link_p->link_vol ; jlp++, guide_link_p++) {
			// 矩形エリアが存在するパーセルＩＤかどうか判定
			for (klp = 0 ; klp < clip_area.parcel_vol ; klp++) {
				if (guide_link_p->parcel_id == clip_area.parcel[klp].id) {
					break;
				}
			}
			if (klp >= clip_area.parcel_vol) {
				continue;
			}
			// 当該パーセルの基準座標を取得
			base_x = clip_area.parcel[klp].base_x;
			base_y = clip_area.parcel[klp].base_y;

			// 形状点情報取得
			guide_sharp_p = (guidetbl_p->guide_sharp_p + guide_link_p->sharp_offset);

			// 当該リンク始点座標取得
			min_x = (guide_sharp_p->x + base_x);
			max_x = (guide_sharp_p->x + base_x);
			min_y = (guide_sharp_p->y + base_y);
			max_y = (guide_sharp_p->y + base_y);

			// 当該リンクを包含する矩形を算出
			for (klp = 1 ; klp < guide_link_p->sharp_vol ; klp++) {
				x = ((guide_sharp_p + klp)->x + base_x);
				y = ((guide_sharp_p + klp)->y + base_y);

				// 右上左下の更新
				if (max_x < x) {		max_x = x;	}
				else if (min_x > x) {	min_x = x;	}
				if (max_y < y) {		max_y = y;	}
				else if (min_y > y) {	min_y = y;	}
			}

			// 自車を中心とする矩形エリアと、リンクを包含する矩形が重なる場合
			if ((clip_area.lb_pos.x > max_x) || (clip_area.lb_pos.y > max_y)  ||
				(min_x > clip_area.rt_pos.x) || (min_y > clip_area.rt_pos.y)) {
				continue;
			}

			// サブリンク合算長
			total_len = 0;

			// サブリンク始点座標の実長を算出
			sx  = (DOUBLE)(guide_sharp_p->x + base_x);
			sx *= rx;						// 実長
			sy  = (DOUBLE)(guide_sharp_p->y + base_y);
			sy *= ry;						// 実長

			// サブリンク数分ループ
			for (klp = 1 ; klp < guide_link_p->sharp_vol ; klp++) {
				// サブリンク終点座標の実長を算出
				ex = (DOUBLE)((guide_sharp_p + klp)->x + base_x);
				ex *= rx;					// 実長
				ey = (DOUBLE)((guide_sharp_p + klp)->y + base_y);
				ey *= ry;					// 実長

				angle = 0.0;

				// 垂線長算出
				RT_LIB_VerticalPoint(cx, cy, sx, sy, ex, ey,
				&rtn, &dx, &dy,	&l_len,	&v_len,	&ratio,	&cxx, &cyy,	&angle,	&len_v_dir,	&len_l_dir);
				if ((0 > rtn) || (0 > v_len)) {
					;
				}
				else{
					// 垂線長１０ｍ以下
					if (10 >= v_len) {
						jdg_dir = abs(car_dir - (INT32)angle);

						// 自車の進行方向と±30°以内
						if ((0 <= jdg_dir && jdg_dir < 30) || (330 < jdg_dir && jdg_dir <= 360)) {
							// 垂線距離が短ければ書き換え→登録処理
							if (min_v_len > (UINT32)v_len) {
								link.sect_no        = sect_no;									// 経路区間番号
								link.route_link_no  = ilp;										// 経路リンク番号
								link.guide_link_no  = jlp;										// リンクオフセット
								link.point_no       = klp - 1;									// 形状点オフセット
								link.draw_len       = total_len + ((UINT32)(l_len * ratio));	// リンク始点からの塗りつぶし長

								// 最近傍リンク更新
								min_v_len = (UINT32)v_len;
							}
						}
					}

					// リンクのトータル距離を加算
					total_len += (UINT32)l_len;
				}
				// 次サブリンク始点座標更新
				sx = ex;
				sy = ey;
			}
		}
	}

	// 近傍リンクが発見できている場合
	if (ALLF32 != min_v_len) {
		guidectl_p->track[TRACK_NEW].link = link;		// 近傍リンク
	}
	else {
		// 前回は順走していた
		if (guidectl_p->track[TRACK_OLD].link.route_link_no != ALLF16) {

			// 自車位置緯度経度算出
			posi_p = &(guidectl_p->track[TRACK_NEW].car_posi);
			if(0 != SC_MESH_ChgParcelIDToTitude(1, posi_p->parcel_id, posi_p->x, posi_p->y, &sy, &sx)){
				SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
				return (e_SC_RESULT_FAIL);
			}

			// 前回最寄リンクの進行方向端点緯度経度算出
			route_link_p   = (guidetbl_p->route_link_p + guidectl_p->track[TRACK_OLD].link.route_link_no);
			guide_link_p   = (guidetbl_p->guide_link_p + route_link_p->link_offset + guidectl_p->track[TRACK_OLD].link.guide_link_no );
			guide_sharp_p  = (guidetbl_p->guide_sharp_p + guide_link_p->sharp_offset);
			guide_sharp_p += (guide_link_p->sharp_vol - 1);
			if(0 != SC_MESH_ChgParcelIDToTitude(1, guide_link_p->parcel_id, guide_sharp_p->x, guide_sharp_p->y, &ey, &ex)){
				SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
				return (e_SC_RESULT_FAIL);
			}

			sx /= 3600;
			sy /= 3600;
			ex /= 3600;
			ey /= 3600;

			// 直線距離算出
			v_len = sc_MESH_GetRealLen(sy, sx, ey, ex);

			// 前回最寄交差点通過中と判断
			if (10 > v_len) {
				guidectl_p->track[TRACK_NEW].link = guidectl_p->track[TRACK_OLD].link;
			}
		}
	}

	return (e_SC_RESULT_SUCCESS);

}

static E_SC_RESULT RG_CTL_GetClipArea(RT_POSITION_t* posi_p, UINT16 size, RT_CLIPAREA_t *clip_area_p)
{

	RT_POSITION_t	lb;
	RT_POSITION_t	rt;
	DOUBLE			move_x;
	DOUBLE			move_y;
	DOUBLE			xPnt;
	DOUBLE			yPnt;
	INT32			xSft;
	INT32			ySft;
	INT16			xlp;
	INT16			ylp;
	INT16			cnt;

	if (NULL == posi_p || NULL == clip_area_p) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 入力された距離を座標に変換
	move_x = (DOUBLE)size / RT_real_x;
	move_y = (DOUBLE)size / RT_real_y;

	// ベースを中心に矩形を作成
	if ((posi_p->x - move_x) < 0) {
		xSft = -1;
		xPnt = MAP_SIZE - (move_x - posi_p->x);
	} else {
		xSft = 0;
		xPnt = posi_p->x - move_x;
	}

	if ((posi_p->y - move_y) < 0) {
		ySft = -1;
		yPnt = MAP_SIZE - (move_y - posi_p->y);
	} else {
		ySft = 0;
		yPnt = posi_p->y - move_y;
	}

	// 矩形左下座標算出
	lb.parcel_id = SC_MESH_SftParcelId(posi_p->parcel_id, (INT16)xSft, (INT16)ySft);
	lb.x = xPnt;
	lb.y = yPnt;

	if ((posi_p->x + move_x) > MAP_SIZE) {
		xSft = 1;
		xPnt = (move_x + posi_p->x) - MAP_SIZE;
	} else {
		xSft = 0;
		xPnt = posi_p->x + move_x;
	}
	if ((posi_p->y + move_y) > MAP_SIZE) {
		ySft = 1;
		yPnt = (move_y + posi_p->y) - MAP_SIZE;
	} else {
		ySft = 0;
		yPnt = posi_p->y + move_y;
	}

	// 矩形右上座標算出
	rt.parcel_id = SC_MESH_SftParcelId(posi_p->parcel_id, (INT16)xSft, (INT16)ySft);
	rt.x = xPnt;
	rt.y = yPnt;


	// 矩形左下ー右上のシフト量取得
	SC_MESH_GetAlterPos(lb.parcel_id, rt.parcel_id, 1, (INT32 *)&xSft, (INT32 *)&ySft);

	// 矩形エリア座標情報設定
	clip_area_p->lb_pos.x = (UINT16)lb.x;							// 左下相対Ｘ座標
	clip_area_p->lb_pos.y = (UINT16)lb.y;							// 左下相対Ｙ座標
	clip_area_p->rt_pos.x = (UINT16)(rt.x + (xSft * MAP_SIZE));		// 右上相対Ｘ座標
	clip_area_p->rt_pos.y = (UINT16)(rt.y + (ySft * MAP_SIZE));		// 右上相対Ｙ座標

	// 矩形左下ー中心のシフト量取得
	SC_MESH_GetAlterPos(lb.parcel_id, posi_p->parcel_id, 1, (INT32 *)&xSft, (INT32 *)&ySft);

	// 矩形中心座標情報設定
	clip_area_p->ct_pos.x = (UINT16)(posi_p->x + (xSft * MAP_SIZE));// 中心相対Ｘ座標
	clip_area_p->ct_pos.y = (UINT16)(posi_p->y + (ySft * MAP_SIZE));// 中心相対Ｙ座標

	// 矩形包含パーセル設定
	for (cnt = 0, ylp = 0; ylp <= ySft; ylp++) {
		for (xlp = 0; xlp <= xSft; xlp++, cnt++) {
			clip_area_p->parcel[cnt].id = SC_MESH_SftParcelId(lb.parcel_id, xlp, ylp);
			clip_area_p->parcel[cnt].base_x = (xlp * MAP_SIZE);
			clip_area_p->parcel[cnt].base_y = (ylp * MAP_SIZE);
		}
	}

	// 矩形包含パーセル数設定
	clip_area_p->parcel_vol = cnt;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief	最寄誘導交差点取得
 * @param	[I]なし
 */
E_SC_RESULT RG_CTL_GetNearCross(RT_TBL_MAIN_t *guidetbl_p, RG_CTL_MAIN_t *guidectl_p)
{

	//E_SC_RESULT					ret = e_SC_RESULT_SUCCESS;
	RT_TBL_ROUTELINK_t			*route_link_p;
	RT_TBL_GUIDELINK_t			*guide_link_p;
	RT_TBL_GUIDECRS_t			*guide_crs_p;
	RT_XY_t						*guide_sharp_p;
	RG_CTL_TRACK_t				*new_p;
	DOUBLE						ratio = 0.0;
	UINT32						crs_remain_dist = 0;
	UINT32						total_dist  = 0;
	UINT32						total_time  = 0;
	UINT32						passed_dist = 0;
	UINT32						passed_time = 0;
	UINT16						ilp,jlp;
	UINT16						cnt;
	UINT16						crs_no = 0;

	if (NULL == guidetbl_p || NULL == guidectl_p) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 今回追跡情報取得
	new_p = &(guidectl_p->track[TRACK_NEW]);

	// 最寄リンクなし
	if (ALLF16 == new_p->link.route_link_no) {
		SC_LOG_DebugPrint(SC_TAG_RG, "[CTL] NEAR LINK NON " HERE);
		return (e_SC_RESULT_SUCCESS);
	}

	// 最寄経路リンク情報アドレス取得
	ilp = new_p->link.route_link_no;
	route_link_p = (guidetbl_p->route_link_p + ilp);

	// 最寄経路リンク構成リンク情報アドレス取得
	jlp = new_p->link.guide_link_no;
	guide_link_p = (guidetbl_p->guide_link_p + route_link_p->link_offset + jlp );

	// 最寄リンク終点までのリンク長設定
	if (guide_link_p->dist >= new_p->link.draw_len) {
		crs_remain_dist = guide_link_p->dist - new_p->link.draw_len;
	}else {
		crs_remain_dist = 0;
	}

	// 最寄交差点番号設定
	if (guidetbl_p->guide_crs_real_vol) {
		crs_no = guidetbl_p->guide_crs_real_vol - 1;
	}

	// 経路リンク数分ループ
	for (cnt = 0; ilp < guidetbl_p->make_no ; ilp++, route_link_p++, jlp=0) {

		// 経路リンク構成リンク情報数分ループ
		for (; jlp < route_link_p->link_vol ; jlp++, guide_link_p++, cnt++) {

			// 初回リンク以外はリンク長/旅行時間加算
			if (0 != cnt) {
				crs_remain_dist += guide_link_p->dist;
			}

			// 交差点情報なし
			if (ALLF16 == guide_link_p->crs_offset) {
				continue;
			}

			// 道なり距離以上
			if (COMVCE_TIMING_FLW_CHK(crs_remain_dist)) {
				if (crs_no > (guidetbl_p->guide_crs_p + guide_link_p->crs_offset)->crs_no) {
					// 最寄交差点番号更新
					crs_no = guidetbl_p->guide_crs_vol;
				}
				continue;
			}

			if (NEARCRS_MAX > new_p->crs_vol) {
				// リンク終点の交差点情報取得
				guide_crs_p = (guidetbl_p->guide_crs_p + guide_link_p->crs_offset);

				// 非誘導交差点
				if (CRSTYPE_NOTCRS == guide_crs_p->crs_type) {
					continue;
				}

				// 誘導交差点情報設定
				new_p->crs[ new_p->crs_vol ].crs_no      = (guidetbl_p->guide_crs_p + guide_link_p->crs_offset)->crs_no;		// 交差点番号
				new_p->crs[ new_p->crs_vol ].crs_type    = guide_crs_p->crs_type;			// 交差点種別
				new_p->crs[ new_p->crs_vol ].turn_dir    = RG_TBL_GetRelativeGuideDir(guide_crs_p);	// 案内方向
				new_p->crs[ new_p->crs_vol ].ra_exit_no  = guide_crs_p->ra_exit_no;			// ラウンドアバウト出口番号
				new_p->crs[ new_p->crs_vol ].tl_f        = guide_crs_p->tl_f;				// 信号機
				new_p->crs[ new_p->crs_vol ].crs_name    = guide_crs_p->crs_name;			// 交差点名称
				new_p->crs[ new_p->crs_vol ].remain_dist = crs_remain_dist;					// 残距離

				if ((CRSTYPE_WAYPT != guide_crs_p->crs_type) &&	(CRSTYPE_DEST != guide_crs_p->crs_type) && (CRSTYPE_SPLIT != guide_crs_p->crs_type) ){
					// 交差点幅を考慮した残距離に補正
					if (new_p->crs[ new_p->crs_vol ].remain_dist > 10) {
						new_p->crs[ new_p->crs_vol ].remain_dist -= 10;
					} else {
						new_p->crs[ new_p->crs_vol ].remain_dist = 0;
					}
				}

				// 誘導リンク形状点情報から当該リンクの端点情報を取得
				guide_sharp_p  = (guidetbl_p->guide_sharp_p + guide_link_p->sharp_offset);
				guide_sharp_p += (guide_link_p->sharp_vol - 1);

				// 交差点座標
				new_p->crs[ new_p->crs_vol ].crs_point.parcel_id = guide_link_p->parcel_id;
				new_p->crs[ new_p->crs_vol ].crs_point.x = (DOUBLE)guide_sharp_p->x;
				new_p->crs[ new_p->crs_vol ].crs_point.y = (DOUBLE)guide_sharp_p->y;

				new_p->crs_vol++;
			}
		}
	}

	// 誘導交差点発見できず or 道なり案内距離
	if (0 == new_p->crs_vol) {
		new_p->crs[0].crs_type    = CRSTYPE_NORMAL;
		new_p->crs[0].turn_dir    = TURN_ST;
		new_p->crs[0].remain_dist = ALLF32;
		new_p->crs[0].crs_no      = crs_no;
		new_p->crs_vol = 1;
	}

	// 経路総距離計算
	for (ilp = 0 ; ilp < guidetbl_p->route_point_vol ; ilp++) {
		total_dist += (guidetbl_p->route_point_p + ilp)->sect_dist;
		total_time += (guidetbl_p->route_point_p + ilp)->sect_time;
	}

	// 最寄経路リンクポインタ取得
	route_link_p = (guidetbl_p->route_link_p + new_p->link.route_link_no);
	// 最寄経路リンクの始点誘導リンクポインタ取得
	guide_link_p = (guidetbl_p->guide_link_p + route_link_p->link_offset);

	// 経路始点～最寄り経路リンク始点までの累計距離・時間設定
	passed_dist  =  route_link_p->total_dist;
	passed_time  =  route_link_p->total_time;

	// 最寄経路リンク始点～最寄誘導リンクの１本手前までの距離を合算
	for (ilp = 0 ; ilp < new_p->link.guide_link_no ; guide_link_p++, ilp++) {
		passed_dist += guide_link_p->dist;
		passed_time += guide_link_p->time;
	}
	// 最寄誘導リンク始点～垂線足までの塗りつぶし長加算
	passed_dist += new_p->link.draw_len;

	// 最寄誘導リンク始点～垂線足までの塗りつぶし時間加算
	ratio = (DOUBLE)((DOUBLE)new_p->link.draw_len / (DOUBLE)guide_link_p->dist);
	passed_time += (UINT32)((DOUBLE)guide_link_p->time * ratio);

	// 目的地までの残距離/時間
	new_p->remain_dist = total_dist - passed_dist;
	new_p->remain_time = total_time - passed_time;

	// 出発地からの移動距離 (経路総距離 - 目的地までの残距離)
	new_p->passed_dist = passed_dist;

	// 今回追跡距離
	new_p->track_dist = crs_remain_dist;

	// 最終順走時における出発地からの移動距離
	guidectl_p->passed_dist = passed_dist;

	return (e_SC_RESULT_SUCCESS);

}

/**
 * @brief	逸脱回数カウント設定
 * @param	[I]なし
 */
E_SC_RESULT RG_CTL_SetDevCount(RG_CTL_MAIN_t *guidectl_p,  SMCARSTATE *carstate_p)
{
	UINT32				dev_cnt = 0;

	if (NULL == guidectl_p || NULL == carstate_p) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 現逸脱カウント取得
	dev_cnt = guidectl_p->deviation_cnt;

	do {
		// シミュレーション判定
		if (guidectl_p->simulate_f) {
			break;
		}

		// 最寄りリンク判定
		if (ALLF16 != guidectl_p->track[TRACK_NEW].link.route_link_no) {
			dev_cnt = 0;		// 経路復帰
			break;
		}

		// 経路マッチング判定
		if (carstate_p->isRouteSelected) {
			dev_cnt = 0;		// 経路復帰
			break;
		}

		// 車速判定
		if (!carstate_p->speed) {
			break;
		}

		// 道路マッチング判定
		if (!carstate_p->onRoad) {
			break;
		}

		// 逸脱回数カウントアップ
		dev_cnt++;

	} while(0);

	// 逸脱回数設定
	if (DEVCNT_MAX <= dev_cnt) {
		guidectl_p->deviation_cnt = DEVCNT_MAX;
	} else {
		guidectl_p->deviation_cnt = dev_cnt;
	}

	SC_LOG_InfoPrint(SC_TAG_RG, "[CTL] speed = %f onr = %d isrt = %d link = %08x devcnt = %d", carstate_p->speed, carstate_p->onRoad, carstate_p->isRouteSelected, carstate_p->linkId, guidectl_p->deviation_cnt);

	return (e_SC_RESULT_SUCCESS);
}

