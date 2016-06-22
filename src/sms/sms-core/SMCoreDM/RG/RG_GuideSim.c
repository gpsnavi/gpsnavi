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
/* File：RG_GuideControl.c                                                                       */
/* Info：誘導制御メイン                                                                          */
/*-----------------------------------------------------------------------------------------------*/

#include "sms-core/SMCoreDM/SMCoreDMInternal.h"

static E_SC_RESULT RG_CTL_SimStartInfoSet(RT_TBL_MAIN_t *);
static E_SC_RESULT RG_CTL_SimMoveNewPoint(RT_TBL_MAIN_t	*, RG_CTL_TRACK_t *);

// シュミレーション開始時地点情報バックアップ
SC_DH_SHARE_RPPOINT		SimStartRPPlace;

/**
 * @brief	シュミレーション開始準備
 * @param	[I]なし
 */
E_SC_RESULT RG_CTL_SimReady()
{
	E_SC_RESULT				ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT				jdg = e_SC_RESULT_SUCCESS;
	SMCARSTATE				carState;
	RT_TBL_MAIN_t			*guidetbl_p;
	INT32					uiSpeed;
	INT32					ilp;
	Bool					mode = false;

	static SC_DH_SHARE_RPPOINT	rpplace;

	// 初期化
	memset(&SimStartRPPlace, 0x00, sizeof(SC_DH_SHARE_RPPOINT));
	memset(&rpplace, 0x00, sizeof(SC_DH_SHARE_RPPOINT));

	// 経路地点情報取得
	ret = SC_MNG_GetAllRPPlace(&(SimStartRPPlace.point[0]), &SimStartRPPlace.pointNum);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 通過済みの経由・目的地を全て「未通過」にして共有メモリ更新
	memcpy(&rpplace, &SimStartRPPlace, sizeof(SC_DH_SHARE_RPPOINT));
	for (ilp = 0 ; ilp < rpplace.pointNum ; ilp++) {
		rpplace.point[ilp].isPassed = false;
	}
	ret = SC_MNG_SetAllRPPlace(&(rpplace.point[0]), rpplace.pointNum);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// シミュレーション終了位置取得
	ret = SC_MNG_GetCarState(&carState, e_SC_CARLOCATION_REAL);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}
	// 実位置を書き換え
	ret = SC_MNG_SetCarState(&carState, e_SC_CARLOCATION_SIMU);
	if (e_SC_RESULT_SUCCESS != ret) {
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
		// シュミレーション開始位置情報設定
		ret = RG_CTL_SimStartInfoSet(guidetbl_p);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_RG, (Char*)"SC_MNG_GetCarState error(0x%08x), " HERE, ret);
			break;
		}

		// デモモード取得
		ret = SC_MNG_GetDemoMode(&mode);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_RG, (Char*)"SC_MNG_GetDemoMode error(0x%08x), " HERE, ret);
		}
		if(false == mode) {
			uiSpeed = 7;		// デモモードＯＦＦ時
		}else{
			uiSpeed = 4;		// デモモードＯＮ時
		}

		//  シミュレーションステップ距離取得
		ret = SC_MNG_SetSimulationSpeed(uiSpeed);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_RG, (Char*)"SC_MNG_SetSimulationSpeed error(0x%08x), " HERE, ret);
		}

	} while(0);

	// 誘導テーブル参照終了
	jdg = RT_TBL_UnLockGuideTbl();
	if (e_SC_RESULT_SUCCESS != jdg) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	return (ret);
}

/**
 * @brief	シュミレーション開始位置情報登録
 * @param	[I]誘導テーブル
 */
static E_SC_RESULT RG_CTL_SimStartInfoSet(RT_TBL_MAIN_t *guidetbl_p)
{
	RT_XY_t					s_pos;
	RT_XY_t					e_pos;
	UINT32					parcel_id;
	UINT32					link_id;
	UINT32					ilp;
	SMCARSTATE				carState;
	DOUBLE					latitude;					//	変換後の緯度（秒）
	DOUBLE					longitude;					//	変換後の経度（秒）
	DOUBLE					angle;
	E_SC_RESULT				ret;
	DOUBLE					real_sx;
	DOUBLE					real_sy;
	DOUBLE					real_ex;
	DOUBLE					real_ey;

	// シュミレーション開始区間の開始始終点リンク情報取得
	if (0 == guidetbl_p->route_link_vol) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 始終点リンクのＩＤ、始点、次点座標取得
	parcel_id = guidetbl_p->guide_link_p->parcel_id;
	link_id   = guidetbl_p->guide_link_p->link_id;
	s_pos     = *(guidetbl_p->guide_sharp_p);

	// 経路始点座標と異なる座標を経路始点以降から検索
	for(ilp = 1 ; guidetbl_p->guide_sharp_vol ; ilp++) {
		e_pos = *(guidetbl_p->guide_sharp_p + ilp);

		// 経路始点と同一座標はスキップ
		if ((s_pos.x == e_pos.x) && (s_pos.y == e_pos.y)) {
			continue;
		}
		break;
	}
	// 発見できなかった場合
	if(ilp >= guidetbl_p->guide_sharp_vol){
		SC_LOG_ErrorPrint(SC_TAG_RG, (Char*)"RG_CTL_SimStartInfoSet error " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 進入座標取得
	real_sx = (DOUBLE)s_pos.x * RT_real_x;
	real_sy = (DOUBLE)s_pos.y * RT_real_y;

	// 退出座標取得
	real_ex = (DOUBLE)e_pos.x * RT_real_x;
	real_ey = (DOUBLE)e_pos.y * RT_real_y;

	// 直線の傾き度計算
	angle = atan2((real_ey - real_sy), (real_ex - real_sx));
	if (angle < 0.0) {
		angle = angle + RT_M_DEGTORAD(360.0);
	}
	// ラジアン→度
	angle = RT_M_RADTODEG(angle);

	// 開始点を実長変換
	// 始点を緯度経度変換
	if(0 != SC_MESH_ChgParcelIDToTitude(1, parcel_id, (DOUBLE)s_pos.x, (DOUBLE)s_pos.y, &latitude, &longitude)){
		SC_LOG_ErrorPrint(SC_TAG_RG, (Char*)"SC_MESH_ChgParcelIDToTitude error " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 車両状態情報を設定
	memset(&carState, 0, sizeof(SMCARSTATE));

	ret = SC_MNG_GetCarState(&carState, e_SC_CARLOCATION_SIMU);
	if(e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_LC, (Char*)"SC_MNG_GetCarState, ret=%d" HERE, ret);
	}

	// s_pos，傾きとその他リンク情報を自車位置情報に登録
	carState.dir = (INT32)angle;
	carState.coord.latitude  = (INT32)(latitude * 1024);
	carState.coord.longitude = (INT32)(longitude * 1024);
	carState.linkId          = link_id;
	carState.parcelId        = parcel_id;
	carState.onRoad			 = true;
	carState.isRouteSelected = true;

	// 共有メモリへ登録
	ret = SC_MNG_SetCarState(&carState, e_SC_CARLOCATION_SIMU);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "SC_DH_SetShareData error, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 自車位置更新描画要求
	LC_LocationUpdateCallback();

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief	シュミレーション実行
 * @param	[I]なし
 */
E_SC_RESULT RG_CTL_RunSimulation()
{
	E_SC_RESULT				ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT				jdg = e_SC_RESULT_SUCCESS;
	RT_TBL_MAIN_t			*guidetbl_p;
	RG_CTL_MAIN_t			*guidectl_p;
	RG_CTL_TRACK_t			*newTrack_p;
	//DOUBLE					lat,lon;
	//DOUBLE					x,y;
	//UINT32					parcel_id;

	// 誘導テーブル参照開始
	guidetbl_p = RT_TBL_LockGuideTbl();
	if (NULL == guidetbl_p) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	do {
		// 誘導制御テーブル取得
		guidectl_p = RG_CTL_GetCtlTbl();
		if (NULL == guidectl_p) {
			SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 前回誘導追跡情報取得
		newTrack_p = &guidectl_p->track[TRACK_NEW];
		if(ALLF16 != newTrack_p->link.route_link_no){
			// 移動先のリンク情報とリンク上の座標を求める
			ret = RG_CTL_SimMoveNewPoint(guidetbl_p,newTrack_p);
		}
		else{
			SC_LOG_DebugPrint(SC_TAG_RG, ">>>>>>>>>>>>>>>> newTrack_p DataNon <<<<<<<<<<<<<<<<<<");
		}

	} while(0);

	// 誘導テーブル参照終了
	jdg = RT_TBL_UnLockGuideTbl();
	if (e_SC_RESULT_SUCCESS != jdg) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	return (ret);
}

/**
 * @brief	シュミレーション移動先情報作成
 * @param	[I]車両状態情報 誘導情報テーブル 経路追跡情報テーブル
 */
static E_SC_RESULT RG_CTL_SimMoveNewPoint(	RT_TBL_MAIN_t		*guidetbl_p,
											RG_CTL_TRACK_t		*newTrack_p)
{
	E_SC_RESULT				ret = e_SC_RESULT_SUCCESS;
	DOUBLE					lat = 0.0;
	DOUBLE					lon = 0.0;
	DOUBLE					real_sx,real_sy;
	DOUBLE					real_ex,real_ey;
	DOUBLE					subLen;
	DOUBLE					mvLen;
	DOUBLE					totalLen;
	//DOUBLE					radian;
	DOUBLE					vectorx;
	DOUBLE					vectory;
	DOUBLE					par;
	DOUBLE					diff_x;
	DOUBLE					diff_y;
	DOUBLE					angle;
	DOUBLE					l_real_x;
	DOUBLE					l_real_y;
	DOUBLE					sx,sy;
	DOUBLE					ex,ey;

	UINT32					parcel_id;
	INT32					speed;
	INT32					ilp,jlp,klp;
	INT32					start_point_no;
	INT32					start_guide_link_no;
	INT32					start_route_link_no;
	INT32					direction;
	Bool					end_f = false;
	SMCARSTATE				newcarState;
	RT_TBL_GUIDELINK_t		*guidLink_p;
	RT_TBL_ROUTELINK_t		*routeLink_p;
	RT_XY_t					*guidSharp_p;

	// 自車位置取得
	parcel_id = newTrack_p->car_posi.parcel_id;
	sx        = newTrack_p->car_posi.x;
	sy        = newTrack_p->car_posi.y;
	direction = newTrack_p->car_dir;

	// 前回経路リンクテーブル取得
	routeLink_p  = (guidetbl_p->route_link_p + newTrack_p->link.route_link_no);
	// 前回誘導リンクテーブル取得
	guidLink_p   = (guidetbl_p->guide_link_p + routeLink_p->link_offset);

	// 前回誘導リンク形状点テーブル取得
	guidSharp_p = (guidetbl_p->guide_sharp_p + guidLink_p->sharp_offset);

	// 自車位置の存在サブリンク始点の次の座標取得
	ex = (DOUBLE)(guidSharp_p + newTrack_p->link.point_no+1)->x;
	ey = (DOUBLE)(guidSharp_p + newTrack_p->link.point_no+1)->y ;

	// 自車位置と近傍リンクにずれが発生していた場合
	if (parcel_id != guidLink_p->parcel_id) {
		sx = ex;
		sy = ey;
	}

	// 実長変換
	l_real_x = RT_real_x;
	l_real_y = RT_real_y;

	real_sx = sx * l_real_x;
	real_sy = sy * l_real_y;
	real_ex = ex * l_real_x;
	real_ey = ey * l_real_y;

	// 前回誘導地点から誘導サブリンク終点までの距離計算
	subLen = sqrt((real_sx - real_ex) * (real_sx - real_ex) + (real_sy - real_ey) * (real_sy - real_ey));

	// シュミレーション移動距離取得
	ret = SC_MNG_GetSimulationSpeed(&speed);
	if (e_SC_RESULT_SUCCESS != ret) {
		speed = 0;
		SC_LOG_ErrorPrint(SC_TAG_RG, (Char*)"SC_MNG_GetSimulationSpeed error(0x%08x), " HERE, ret);
	}

	if(speed <= 3){
		speed = 4;
	}

	mvLen = (DOUBLE)speed;

	// 直近交差点が存在する場合は、移動量を抑える
	if (newTrack_p->crs_vol) {
		if ((newTrack_p->crs[0].crs_type == CRSTYPE_HWYOT) || (newTrack_p->crs[0].crs_type == CRSTYPE_HWYJCT)) {
			if (HWYVCE_TIMING_JP_FST > newTrack_p->crs[0].remain_dist) {
				if (mvLen > 20) {
					mvLen = 20;
				}
			}
		} else {
			if (GENVCE_TIMING_JP_FST > newTrack_p->crs[0].remain_dist) {
				if (mvLen > 10) {
					mvLen = 10;
				}
			}
		}
	}

	// 前回誘導地点から誘導サブリン終点までの距離が移動距離より短い
	if(subLen <= mvLen){
		totalLen = subLen;
		// 誘導対象の経路リンク情報テーブル取得
		start_route_link_no = newTrack_p->link.route_link_no;
		routeLink_p         = (guidetbl_p->route_link_p + start_route_link_no);
		// 経路リンク情報テーブルから誘導リンク情報テーブル取得
		start_guide_link_no = newTrack_p->link.guide_link_no;
		// 誘導リンク情報テーブルから誘導先の点座標オフセット取得
		start_point_no      = newTrack_p->link.point_no + 1;
		// 移動距離を超えるまでサブリンクを移動
		for(klp = start_route_link_no;klp < guidetbl_p->route_link_vol;klp++){
			guidLink_p = (guidetbl_p->guide_link_p + routeLink_p->link_offset + start_guide_link_no);

			for(ilp = start_guide_link_no; ilp < routeLink_p->link_vol;ilp++){

				for(jlp = start_point_no;jlp < guidLink_p->sharp_vol-1;jlp++){

					guidSharp_p = (guidetbl_p->guide_sharp_p + guidLink_p->sharp_offset);

					// 終点情報取得(実長変換）
					sx = (DOUBLE)(guidSharp_p + jlp)->x;
					sy = (DOUBLE)(guidSharp_p + jlp)->y;
					ex = (DOUBLE)(guidSharp_p + jlp + 1)->x;
					ey = (DOUBLE)(guidSharp_p + jlp + 1)->y;

					// 実長変換
					real_sx = sx * l_real_x;
					real_sy = sy * l_real_y;
					real_ex = ex * l_real_x;
					real_ey = ey * l_real_y;

					// 同一座標の場合はリンク長も０なので自車スキップ位置には成り得ない為、スキップ
					if ((real_ey - real_sy)==0 && (real_ex - real_sx)==0) {
						continue;
					}

					// 前回誘導地点から誘導サブリンク終点までの距離
					subLen = sqrt((real_sx - real_ex) * (real_sx - real_ex) + (real_sy - real_ey) * (real_sy - real_ey));

					// 直線の傾き度計算
					angle = atan2((real_ey - real_sy), (real_ex - real_sx));
					if (angle < 0.0) {
						angle = angle + RT_M_DEGTORAD(360.0);
					}
					// ラジアン→度
					direction = (INT32)RT_M_RADTODEG(angle);

					// ここまでのサブリンク長合計値が移動距離を超えた
					if((totalLen + subLen) > mvLen){
						// 移動距離を現在のサブリンク始点からの距離に変換
						mvLen -= totalLen;
						// サブリンク長合計値に最終サブリンク長加算
						totalLen += subLen;
						break;
					}
					else {
						// サブリンク長合計値に求めたサブリンク長を加算
						totalLen += subLen;
					}
				}
				// ここまでのサブリンク長合計値が移動距離を超えた
				if(totalLen >= mvLen){
					break;
				}
				// start_point_noを０で初期化
				start_point_no = 0;
				// 次のguidLink_pへ
				guidLink_p++;
			}
			// ここまでのサブリンク長合計値が移動距離を超えた
			if(totalLen >= mvLen){
				break;
			}
			// 誘導テーブル開始位置
			start_guide_link_no = 0;
			routeLink_p++;
		}

		// 経路残距離 < 移動量
		if (totalLen < mvLen) {
			end_f = true;
		}
	}

	// 車両状態情報を設定
	memset(&newcarState, 0, sizeof(SMCARSTATE));

	// 経路終点到着
	if (end_f) {
		routeLink_p = (guidetbl_p->route_link_p + (guidetbl_p->route_link_vol-1));
		guidLink_p =  (guidetbl_p->guide_link_p + (guidetbl_p->guide_link_vol-1));

		// 経路終端座標取得
		ex = (DOUBLE)(guidetbl_p->guide_sharp_p + (guidetbl_p->guide_sharp_vol-1))->x;
		ey = (DOUBLE)(guidetbl_p->guide_sharp_p + (guidetbl_p->guide_sharp_vol-1))->y ;

		if (guidetbl_p->guide_sharp_vol < 2) {
			;
		} else {
			sx = (DOUBLE)(guidetbl_p->guide_sharp_p + (guidetbl_p->guide_sharp_vol-2))->x;
			sy = (DOUBLE)(guidetbl_p->guide_sharp_p + (guidetbl_p->guide_sharp_vol-2))->y ;

			// 実長変換
			real_sx = sx * l_real_x;
			real_sy = sy * l_real_y;
			real_ex = ex * l_real_x;
			real_ey = ey * l_real_y;

			if ((real_ey - real_sy)==0 && (real_ex - real_sx)==0) {
				;
			} else {
				// 直線の傾き度計算
				angle = atan2((real_ey - real_sy), (real_ex - real_sx));
				if (angle < 0.0) {
					angle = angle + RT_M_DEGTORAD(360.0);
				}
				// ラジアン→度
				direction = (INT32)RT_M_RADTODEG(angle);
			}
		}

		// 緯度経度変換
		SC_MESH_ChgParcelIDToTitude(1,guidLink_p->parcel_id,ex,ey,&lat,&lon);

		//傾きとその他リンク情報を自車位置情報に登録
		newcarState.dir = direction;
		newcarState.coord.latitude  = (INT32)(lat * 1024);
		newcarState.coord.longitude = (INT32)(lon * 1024);
		newcarState.linkId          = guidLink_p->link_id;
		newcarState.parcelId        = guidLink_p->parcel_id;
		newcarState.onRoad			= true;
		newcarState.isRouteSelected	= true;

	} else {
		// 移動先までの内分立計算
		par = mvLen / subLen;

		// Ｘ,Ｙ方向差分
		diff_x = real_ex - real_sx;
		diff_y = real_ey - real_sy;

		// Ｘ，Ｙ移動量距離計算
		vectorx =  diff_x * par;
		vectory =  diff_y * par;

		// Ｘ，Ｙ移動先座標取得
		real_sx += vectorx;
		real_sy += vectory;

		// 実長→正規化座標
		real_sx /= l_real_x;
		real_sy /= l_real_y;

		// 緯度経度変換
		SC_MESH_ChgParcelIDToTitude(1,guidLink_p->parcel_id,real_sx,real_sy,&lat,&lon);

		//傾きとその他リンク情報を自車位置情報に登録
		newcarState.dir = direction;
		newcarState.coord.latitude  = (INT32)(lat * 1024);
		newcarState.coord.longitude = (INT32)(lon * 1024);
		newcarState.linkId          = guidLink_p->link_id;
		newcarState.parcelId        = guidLink_p->parcel_id;
		newcarState.onRoad			= true;
		newcarState.isRouteSelected	= true;
	}

	// 共有メモリへ登録
	ret = SC_MNG_SetCarState(&newcarState, e_SC_CARLOCATION_SIMU);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "SC_DH_SetShareData error, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 自車位置更新描画要求
	LC_LocationUpdateCallback();

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief	シュミレーション実行
 * @param	[I]なし
 */
E_SC_RESULT RG_CTL_ExitSimulation()
{
	E_SC_RESULT				ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_RPPOINT		point = {};
	SMCARSTATE				carState;
	Bool					mode = false;

	// 経路地点情報取得
	ret = SC_MNG_GetAllRPPlace(&(point.point[0]), &point.pointNum);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}
	// 目的地到着時
	if (true == point.point[point.pointNum - 1].isPassed) {
		sleep(2);	// ２秒待つ
	}

	// デモモード取得
	ret = SC_MNG_GetDemoMode(&mode);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RG, (Char*)"SC_MNG_GetDemoMode error(0x%08x), " HERE, ret);
	}
	if(true == mode) {	// ON
		// シミュレーション終了位置取得
		ret = SC_MNG_GetCarState(&carState, e_SC_CARLOCATION_SIMU);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
			return (e_SC_RESULT_FAIL);
		}
		// 実位置を書き換え
		ret = SC_MNG_SetCarState(&carState, e_SC_CARLOCATION_REAL);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
			return (e_SC_RESULT_FAIL);
		}
	} else {
		// シミュレーション開始前の地点情報（通過済み状態）を復帰
		if (0 < SimStartRPPlace.pointNum) {
			ret = SC_MNG_SetAllRPPlace(&(SimStartRPPlace.point[0]), SimStartRPPlace.pointNum);
			if (e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
				return (e_SC_RESULT_FAIL);
			}
		}
	}

	/*** シュミレーション状態設定 ***/
	ret = SC_MNG_SetSimulationStatus(e_SIMULATE_STATE_STOP);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RG, (Char*)"SC_MNG_SetSimulationStatus error(0x%08x), " HERE, ret);
		return (e_SC_RESULT_FAIL);
	}

	// 自車位置更新描画要求
	LC_LocationUpdateCallback();

	return (e_SC_RESULT_SUCCESS);
}

