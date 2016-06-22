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

static E_SC_RESULT RG_CTL_BkupGuideTrackTbl(RG_CTL_MAIN_t *);
static E_SC_RESULT RG_CTL_TblAddReqCheck(RT_TBL_MAIN_t *, RG_CTL_MAIN_t *);
static E_SC_RESULT RG_CTL_SetArrivalStatus(RT_TBL_MAIN_t *, RG_CTL_MAIN_t *);

// 誘導制御テーブル
RG_CTL_MAIN_t		RG_G_CtlTbl;

/**
 * @brief	誘導制御メイン
 * @param	[I]なし
 */
E_SC_RESULT RG_CTL_GuideMain()
{
	E_SC_RESULT				ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT				jdg = e_SC_RESULT_SUCCESS;
	E_SC_SIMSTATE 			status;
	RT_TBL_MAIN_t			*guidetbl_p;
	RG_CTL_MAIN_t			*guidectl_p = NULL;
	SMCARSTATE				carState;
	DOUBLE					lat,lon;
	DOUBLE					x,y;
	UINT32					parcel_id;

	SC_LOG_DebugPrint(SC_TAG_RG, SC_LOG_START);

	// 誘導テーブル参照開始
	guidetbl_p = RT_TBL_LockGuideTbl();
	if (NULL == guidetbl_p) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	do {
		// 自車位置情報取得
		memset(&carState, 0, sizeof(SMCARSTATE));
		ret = SC_MNG_GetCarState(&carState, e_SC_CARLOCATION_NOW);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
			break;
		}

		// 緯度経度→座標変換
		lat = (DOUBLE) carState.coord.latitude / 1024;
		lon = (DOUBLE) carState.coord.longitude / 1024;
		SC_Lib_ChangeTitude2PID(lat, lon, 1, &parcel_id, &x, &y);

		// 誘導制御テーブル取得
		guidectl_p = RG_CTL_GetCtlTbl();
		if (NULL == guidectl_p) {
			SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 前回と同一座標
		if ((guidectl_p->track[TRACK_NEW].car_posi.parcel_id == parcel_id) &&
			(CompareDouble(guidectl_p->track[TRACK_NEW].car_posi.x, x)) &&
			(CompareDouble(guidectl_p->track[TRACK_NEW].car_posi.y, y)) ) {
			ret = e_SC_RESULT_SUCCESS;
			break;
		}

		// 前回誘導追跡情報保管
		ret = RG_CTL_BkupGuideTrackTbl(guidectl_p);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
			break;
		}

		// 自車位置座標設定
		guidectl_p->track[TRACK_NEW].car_posi.parcel_id = parcel_id;
		guidectl_p->track[TRACK_NEW].car_posi.x = x;
		guidectl_p->track[TRACK_NEW].car_posi.y = y;

		// 自車方位設定
		guidectl_p->track[TRACK_NEW].car_dir = carState.dir;

		// シミュレーション判定
		ret = SC_MNG_GetSimulationStatus(&status);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		}
		if(e_SIMULATE_STATE_STRAT == status){
			guidectl_p->simulate_f = true;
		}

		//-------------------------------------------------------------------------
		// 誘導計算
		//-------------------------------------------------------------------------
		// 前回目的地到着の場合
		if (guidectl_p->arrival_f) {
			ret = e_SC_RESULT_SUCCESS;
			SC_LOG_InfoPrint(SC_TAG_RG, "[CTL] ARRIVAL ON %d" HERE);
			break;
		}

		// 自車最寄誘導リンク情報取得
		ret = RG_CTL_GetNearLink(guidetbl_p, guidectl_p);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
			break;
		}

		// 自車最寄誘導交差点情報取得
		ret = RG_CTL_GetNearCross(guidetbl_p, guidectl_p);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
			break;
		}

		// 経路逸脱回数設定
		ret = RG_CTL_SetDevCount(guidectl_p, &carState);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
			break;
		}

		// 目的地到着状態設定
		ret = RG_CTL_SetArrivalStatus(guidetbl_p, guidectl_p);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
			break;
		}
#if 0	// kana暫定
		// 音声発話ＴＴＳ設定
		ret = RG_CTL_SetGuideVoice(guidetbl_p, guidectl_p);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
			break;
		}
#endif
		// 誘導テーブル追加要求チェック
		ret = RG_CTL_TblAddReqCheck(guidetbl_p, guidectl_p);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
			break;
		}

		//-------------------------------------------------------------------------
		// 共有メモリへの誘導情報設定
		//-------------------------------------------------------------------------
		// 経路逸脱状態設定
		ret = RG_CTL_SH_SetDeviationInfo(guidectl_p);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
			break;
		}

		// 経由・目的地通過情報設定
		ret = RG_CTL_SH_SetPassedInfo(guidectl_p);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
			break;
		}

		// ターンバイターン情報設定
		ret = RG_CTL_SH_SetRealTimeInfo(guidectl_p);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
			break;
		}

		// 誘導音声ＴＴＳ設定
		ret = RG_CTL_SH_SetVoiceTTS(guidectl_p);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
			break;
		}

		// 交差点拡大図設定
		ret = RG_CTL_SH_SetDynamicGraphicStat(guidectl_p);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
			break;
		}

	} while(0);

	// 誘導テーブル参照終了
	jdg = RT_TBL_UnLockGuideTbl();
	if (e_SC_RESULT_SUCCESS != jdg) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}


#if 1
	if (NULL != guidectl_p) {
		SC_LOG_DebugPrint(SC_TAG_RG, "GUIDE DATA --> POSI[%08x:%f:%f ARRW:%d] LINK[OFS:%d:%d:%d] CRS[DIR:%d: DIST:%d]" ,
		parcel_id, guidectl_p->track[TRACK_NEW].car_posi.x, guidectl_p->track[TRACK_NEW].car_posi.y, guidectl_p->track[TRACK_NEW].car_dir, guidectl_p->track[TRACK_NEW].link.route_link_no,
		guidectl_p->track[TRACK_NEW].link.guide_link_no, guidectl_p->track[TRACK_NEW].link.point_no, guidectl_p->track[TRACK_NEW].crs[0].turn_dir, guidectl_p->track[TRACK_NEW].crs[0].remain_dist);
	}
#endif

	SC_LOG_DebugPrint(SC_TAG_RG, SC_LOG_END);

	return (ret);
}

/**
 * @brief	誘導制御テーブル初期化
 * @param	[I]誘導テーブル
 */
E_SC_RESULT RG_CTL_InitCtlTbl()
{
	RG_CTL_MAIN_t			*guidectl_p;
	UINT16					ilp;

	SC_LOG_DebugPrint(SC_TAG_RG, SC_LOG_START);

	// 誘導制御テーブル取得
	guidectl_p = RG_CTL_GetCtlTbl();
	if (NULL == guidectl_p) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 初期化 TODO
	memset(guidectl_p, 0x00, sizeof(RG_CTL_MAIN_t));

	// 最寄交差点情報初期化
	for (ilp = 0 ; ilp < NEARCRS_MAX ; ilp++) {
		guidectl_p->track[TRACK_NEW].crs[ilp].crs_no = ALLF16;
		guidectl_p->track[TRACK_OLD].crs[ilp].crs_no = ALLF16;
	}

	// 最寄リンク情報初期化
	memset(&guidectl_p->track[TRACK_NEW].link, ALLF8, sizeof(RG_CTL_NEARLINK_t));
	memset(&guidectl_p->track[TRACK_OLD].link, ALLF8, sizeof(RG_CTL_NEARLINK_t));

	SC_LOG_DebugPrint(SC_TAG_RG, SC_LOG_END);

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief	誘導制御テーブル取得
 * @param	[I]なし
 */
RG_CTL_MAIN_t *RG_CTL_GetCtlTbl()
{
	return (&RG_G_CtlTbl);
}

/**
 * @brief	誘導制御テーブル初期化
 * @param	[I]誘導テーブル
 */
static E_SC_RESULT RG_CTL_BkupGuideTrackTbl(RG_CTL_MAIN_t *guidectl_p)
{
	UINT16		ilp;

	if (NULL == guidectl_p) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 順走していた場合のみ
	if (ALLF16 != guidectl_p->track[TRACK_NEW].link.route_link_no) {
		// 前回追跡情報を配列１に退避
		guidectl_p->track[TRACK_OLD] = guidectl_p->track[TRACK_NEW];
	}

	// 今回追跡情報を格納する配列０を初期化
	memset(&(guidectl_p->track[TRACK_NEW]), 0x00, sizeof(RG_CTL_TRACK_t));

	// 最寄交差点情報初期化
	for (ilp = 0 ; ilp < NEARCRS_MAX ; ilp++) {
		guidectl_p->track[TRACK_NEW].crs[ilp].crs_no = ALLF16;
	}

	// 最寄リンク情報初期化
	memset(&(guidectl_p->track[TRACK_NEW].link), ALLF8, sizeof(RG_CTL_NEARLINK_t));

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief	誘導テーブル誘導制御テーブル初期化
 * @param	[I]誘導テーブル
 */
static E_SC_RESULT RG_CTL_TblAddReqCheck(RT_TBL_MAIN_t *guidetbl_p, RG_CTL_MAIN_t *guidectl_p)
{

	E_SC_RESULT				ret = e_SC_RESULT_SUCCESS;
	UINT32					old_track_dist = 0;
	UINT32					new_track_dist = 0;

	if (NULL == guidetbl_p || NULL == guidectl_p) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 誘導テーブルが未完成の場合
	if (guidetbl_p->route_link_vol > guidetbl_p->make_no) {

		// -----------------------------------------------------------------
		// 最寄リンクなし
		// 延長により最寄リンクが取得できる可能性がある為、制限無しで要求する
		// -----------------------------------------------------------------
		if (ALLF16 == guidectl_p->track[TRACK_NEW].link.route_link_no) {
			// 誘導テーブル追加要求
			RG_RTMsg_Send(e_SC_MSGID_REQ_RT_GUIDEADD);
		}
		// -----------------------------------------------------------------
		// 最寄リンクあり
		// 前回未要求時のみ要求する
		// -----------------------------------------------------------------
		else {
			old_track_dist = guidectl_p->track[TRACK_OLD].track_dist;
			new_track_dist = guidectl_p->track[TRACK_NEW].track_dist;

			// 今回追跡距離が規定値以下の場合
			if (MAKETBL_DIV > new_track_dist) {

				// 前回追跡距離が０もしくは、規定値以上だった場合
				if ((0 == old_track_dist) || (MAKETBL_DIV <= old_track_dist)) {
					// 誘導テーブル追加要求 (必要以上の要求はしない)
					RG_RTMsg_Send(e_SC_MSGID_REQ_RT_GUIDEADD);
				}
			}
		}
	}

	return (ret);

}

/**
 * @brief	目的地到着状態設定
 */
static E_SC_RESULT RG_CTL_SetArrivalStatus(RT_TBL_MAIN_t *guidetbl_p, RG_CTL_MAIN_t *guidectl_p)
{
	RG_CTL_TRACK_t			*new_p;
	RT_POSITION_t			*posi_p;
	DOUBLE					slat;
	DOUBLE					slon;
	DOUBLE					elat;
	DOUBLE					elon;
	DOUBLE					dist;

	if (NULL == guidetbl_p || NULL == guidectl_p) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 今回追跡情報取得
	new_p = &(guidectl_p->track[TRACK_NEW]);

	// 最寄リンクあり
	if (ALLF16 != new_p->link.route_link_no) {
		if (0 < new_p->crs_vol) {
			// 最寄交差点が目的地
			if (CRSTYPE_DEST == new_p->crs[0].crs_type) {

				// 到着案内範囲内
				if (COMVCE_TIMING_ARV_CHK(new_p->crs[0].remain_dist)) {
					// 目的地到着状態設定
					guidectl_p->arrival_f = true;
				}
			// 最寄交差点が経路断裂点
			} else if (CRSTYPE_SPLIT == new_p->crs[0].crs_type) {
				// シミュレーション中
				if (guidectl_p->simulate_f) {
					if (COMVCE_TIMING_ARV_CHK(new_p->crs[0].remain_dist)) {
						// 目的地到着状態設定
						guidectl_p->arrival_f = true;
					}
				}
			}
		}
	}
	else {
		// 自車位置緯度経度算出
		posi_p = &(new_p->car_posi);
		if(0 != SC_MESH_ChgParcelIDToTitude(1, posi_p->parcel_id, posi_p->x, posi_p->y, &slat, &slon)){
			SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
			return (e_SC_RESULT_FAIL);
		}

		// 目的地緯度経度算出
		posi_p = &((guidetbl_p->route_point_p + guidetbl_p->route_point_vol - 1)->posi);
		if(0 != SC_MESH_ChgParcelIDToTitude(1, posi_p->parcel_id, posi_p->x, posi_p->y, &elat, &elon)){
			SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
			return (e_SC_RESULT_FAIL);
		}

		slat /= 3600;
		slon /= 3600;
		elat /= 3600;
		elon /= 3600;

		// 直線距離算出
		dist = sc_MESH_GetRealLen(slat, slon, elat, elon);

		// 逸脱到着判定距離以内
		if (DEVARRIVAL_DIST > dist) {
			// 目的地到着状態設定
			guidectl_p->arrival_f = true;
		}
	}

	return (e_SC_RESULT_SUCCESS);
}
