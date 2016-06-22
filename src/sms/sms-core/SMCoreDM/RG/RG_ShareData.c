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

/**
 * @brief	公開用誘導情報初期化
 * @param	[I]誘導制御テーブル
 */
E_SC_RESULT RG_CTL_SH_InitShareData()
{

	E_SC_RESULT				ret = e_SC_RESULT_SUCCESS;
	SMREALTIMEGUIDEDATA		realtimeinfo;
	SMVOICETTS				voicetts;

	SC_LOG_DebugPrint(SC_TAG_RG, SC_LOG_START);

	// 常駐メモリへの設定
	ret = SC_MNG_SetDeviationStatus(e_DEVIATION_STATUS_OFF);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 公開用テーブル初期化
	memset(&realtimeinfo, 0x00, sizeof(SMREALTIMEGUIDEDATA));

	// 常駐メモリへの設定
	ret = SC_MNG_SetRealTimeInfo(&realtimeinfo);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 公開用テーブル初期化
	memset(&voicetts, 0x00, sizeof(SMVOICETTS));

	// 常駐メモリへの設定
	ret = SC_MNG_SetVoiceTTS(&voicetts);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	SC_LOG_DebugPrint(SC_TAG_RG, SC_LOG_END);

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief	公開用経路逸脱状態設定
 * @param	[I]誘導制御テーブル
 */
E_SC_RESULT RG_CTL_SH_SetDeviationInfo(RG_CTL_MAIN_t *guidectl_p)
{

	E_SC_RESULT				ret = e_SC_RESULT_SUCCESS;
	E_DEVIATION_STATUS		status = false;
	//INT32					count  = 0;

	if (NULL == guidectl_p) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 逸脱回数が規定値を超えていた場合
	if (DEVCNT_MAX <= guidectl_p->deviation_cnt) {
		status = true;
	}

	// 常駐メモリへの設定
	ret = SC_MNG_SetDeviationStatus(status);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief	公開用目的地到着状態設定
 * @param	[I]誘導制御テーブル
 */
E_SC_RESULT RG_CTL_SH_SetPassedInfo(RG_CTL_MAIN_t *guidectl_p)
{

	E_SC_RESULT				ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_RPPOINT		point = {};
	INT32					pass;
	INT32					ilp;

	if (NULL == guidectl_p) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 経路地点情報取得
	ret = SC_MNG_GetAllRPPlace(&(point.point[0]), &point.pointNum);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 目的地到着している場合
	if (guidectl_p->arrival_f) {
		pass = point.pointNum;
	} else {
		// 最寄リンクなし
		if (ALLF16 == guidectl_p->track[TRACK_NEW].link.route_link_no) {
			return (e_SC_RESULT_SUCCESS);
		}

		// 最寄リンクの区間番号取得
		pass = guidectl_p->track[TRACK_NEW].link.sect_no + 1;
		if (point.pointNum < pass) {
			SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
			return (e_SC_RESULT_FAIL);
		}
	}

	// 最寄区間より手前の地点を通過済みに設定 (0は出発地)
	for (ilp = 0 ; ilp < pass ; ilp++) {
		point.point[ilp].isPassed = true;
	}

	// 経路地点情報設定
	ret = SC_MNG_SetAllRPPlace(&(point.point[0]), point.pointNum);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief	公開用誘導情報設定
 * @param	[I]誘導制御テーブル
 */
E_SC_RESULT RG_CTL_SH_SetRealTimeInfo(RG_CTL_MAIN_t *guidectl_p)
{

	E_SC_RESULT				ret = e_SC_RESULT_SUCCESS;
	SMREALTIMEGUIDEDATA		realtimeinfo = {};
	RG_CTL_TRACK_t			*track_p;
	DOUBLE					x,y,lat,lon;

	if (NULL == guidectl_p) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 今回追跡情報取得
	track_p = &(guidectl_p->track[TRACK_NEW]);

	// 交差点情報があれば
	if (track_p->crs_vol > 0) {

		// 道なり示唆
		if (COMVCE_TIMING_FLW_CHK(track_p->crs[0].remain_dist)) {
			realtimeinfo.turnDir = TURN_ST;
			realtimeinfo.remainDistToNextPlace = track_p->remain_dist;			// 残距離 (目的地)
			realtimeinfo.remainTimeToNextPlace = track_p->remain_time;			// 残時間（目的地）
			realtimeinfo.passedDistance        = track_p->passed_dist;			// 出発地からの移動距離
			realtimeinfo.valid = true;											// 誘導情報有効
			realtimeinfo.aheadPoint = true;										// 道なり有効
			realtimeinfo.roadSituation = RT_TBL_GetSituationType(track_p->crs[0].crs_type); // 案内種別
			realtimeinfo.bypass = track_p->crs[0].crs_no;						// 交差点番号
		}
		else {
			if (CRSTYPE_WAYPT == track_p->crs[0].crs_type) {
				realtimeinfo.turnDir = 22;										// 経由地
				realtimeinfo.nextBypassIndex = track_p->link.sect_no + 1;		// 経由地番号
			}else if (CRSTYPE_DEST == track_p->crs[0].crs_type) {
				realtimeinfo.turnDir = 23;										// 目的地
			}else if (CRSTYPE_SPLIT == track_p->crs[0].crs_type) {
				realtimeinfo.turnDir = TURN_ST;									// 経路断裂点
			}else {
				realtimeinfo.turnDir = track_p->crs[0].turn_dir;				// 方向
			}

			// 交差点緯度経度
			x = track_p->crs[0].crs_point.x;
			y = track_p->crs[0].crs_point.y;
			if(0 != SC_MESH_ChgParcelIDToTitude(1, track_p->crs[0].crs_point.parcel_id, x, y, &lat, &lon)){
				SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
				return (e_SC_RESULT_FAIL);
			}
			realtimeinfo.coord.latitude  = (INT32)(lat * 1024);
			realtimeinfo.coord.longitude = (INT32)(lon * 1024);

			realtimeinfo.remainDistToNextTurn  = track_p->crs[0].remain_dist;	// 残距離 (交差点)
			realtimeinfo.remainDistToNextPlace = track_p->remain_dist;			// 残距離 (目的地)
			realtimeinfo.remainTimeToNextPlace = track_p->remain_time;			// 残時間（目的地）
			realtimeinfo.passedDistance        = track_p->passed_dist;			// 出発地からの移動距離
			realtimeinfo.destination           = true;							// 目的地

			// 交差点名称あり＆絶対方向時のみ
			if (track_p->crs[0].tl_f) {
				realtimeinfo.showTrafficLight = true;							// 信号機あり
			}
			// 交差点名称
			if (track_p->crs[0].crs_name.len > 0) {
				strcpy(&(realtimeinfo.nextBroadString[0]), &(track_p->crs[0].crs_name.name[0]));
			}
			realtimeinfo.valid = true;											// 誘導情報有効
			realtimeinfo.aheadPoint = false;									// 道なり無効
			realtimeinfo.roadSituation = RT_TBL_GetSituationType(track_p->crs[0].crs_type); // 案内種別
			realtimeinfo.bypass = track_p->crs[0].crs_no;						// 交差点番号
		}
	} else {
		realtimeinfo.turnDir = 5;
		realtimeinfo.valid   = false;											// 誘導情報無効
		realtimeinfo.aheadPoint = false;										// 道なり無効
		realtimeinfo.roadSituation = STYPE_GDCRS;								// 案内種別
	}

	// 共有メモリへの設定
	ret = SC_MNG_SetRealTimeInfo(&realtimeinfo);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	return (e_SC_RESULT_SUCCESS);

}


/**
 * @brief	公開用誘導音声ＴＴＳ情報設定
 * @param	[I]誘導制御テーブル
 */
E_SC_RESULT RG_CTL_SH_SetVoiceTTS(RG_CTL_MAIN_t *guidectl_p)
{
	E_SC_RESULT				ret = e_SC_RESULT_SUCCESS;
	SMVOICETTS				voicetts = {};
	RG_CTL_TRACK_t			*track_p;

	if (NULL == guidectl_p) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 今回追跡情報取得
	track_p = &(guidectl_p->track[TRACK_NEW]);

	// 交差点情報があれば
	if (track_p->crs_vol > 0 || true == guidectl_p->arrival_f) {
		// 発話すべきＴＴＳ文字列があれば
		if (track_p->voice.tts.voice.current > 0) {
			// 文言ID配列をコピー
			memcpy(&voicetts.tts, &track_p->voice.tts.voice, sizeof(RT_VOICE_t));
			voicetts.priority = 5;													// 発話優先度
		}
	}

	// 共有メモリへの設定
	ret = SC_MNG_SetVoiceTTS(&voicetts);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	return (e_SC_RESULT_SUCCESS);

}

/**
 * @brief	公開用誘導音声ＴＴＳ情報設定
 * @param	[I]誘導制御テーブル
 */
E_SC_RESULT RG_CTL_SH_SetTurnList(RT_LST_MAIN_t *turnlist_p)
{
	E_SC_RESULT				ret = e_SC_RESULT_SUCCESS;
	SMTURNLIST				turnList;

	if (NULL == turnlist_p) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	memset(&turnList, 0x00, sizeof(SMTURNLIST));

	turnList.turnNum    = turnlist_p->turn_vol;			// 交差点情報数
	turnList.turnInfo   = &(turnlist_p->turn_info[0]);	// 交差点情報先頭アドレス

	// 共有メモリへの設定
	ret = SC_MNG_SetTurnList(&turnList);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	return (e_SC_RESULT_SUCCESS);

}

/**
 * @brief	公開用誘導交差点拡大図有無情報設定
 * @param	[I]誘導制御テーブル
 */
E_SC_RESULT RG_CTL_SH_SetDynamicGraphicStat(RG_CTL_MAIN_t *guidectl_p){

	E_SC_RESULT				ret = e_SC_RESULT_SUCCESS;
	//SMBITMAPINFO			bitmapinfo;
	RG_CTL_TRACK_t			*track_p;

	if (NULL == guidectl_p) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 今回追跡情報取得
	track_p = &(guidectl_p->track[TRACK_NEW]);

	// 交差点情報があれば
	if (track_p->crs_vol > 0) {
		// 誘導地点が経由地か目的地
		if (CRSTYPE_WAYPT == track_p->crs[0].crs_type || CRSTYPE_DEST == track_p->crs[0].crs_type) {
			ret = SC_MNG_SetDynamicGraphicStatus(e_DYNAMIC_GRAPHIC_NON);
		}
		// 残距離が３００ｍ以下
		else if(track_p->crs[0].remain_dist < 300){
			ret = SC_MNG_SetDynamicGraphicStatus(e_DYNAMIC_GRAPHIC_ON);
		}
		else {
			ret = SC_MNG_SetDynamicGraphicStatus(e_DYNAMIC_GRAPHIC_NON);
		}
	}
	else {
		ret = SC_MNG_SetDynamicGraphicStatus(e_DYNAMIC_GRAPHIC_NON);

	}
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}
	else {
		return (e_SC_RESULT_SUCCESS);
	}
}
