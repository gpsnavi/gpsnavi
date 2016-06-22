/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

/*
 * RG_GuideVoiceBuild_en.c
 *
 *  Created on: 2014/11/18
 *      Author: masutani
 */

#include "sms-core/SMCoreDM/SMCoreDMInternal.h"

// 経由地番号
const static INT32 m_WayPointList[] = {
		VOICE_TEXT_EN_201,
		VOICE_TEXT_EN_202,
		VOICE_TEXT_EN_203,
		VOICE_TEXT_EN_204,
		VOICE_TEXT_EN_205};
// ラウンドアバウト出口番号
const static INT32 m_RaundBairdList[] = {
		0,
		VOICE_TEXT_EN_206,
		VOICE_TEXT_EN_207,
		VOICE_TEXT_EN_208,
		VOICE_TEXT_EN_209,
		VOICE_TEXT_EN_210,
		VOICE_TEXT_EN_211,
		VOICE_TEXT_EN_212,
		VOICE_TEXT_EN_213,
		VOICE_TEXT_EN_214,
		VOICE_TEXT_EN_215,
		VOICE_TEXT_EN_216,
		VOICE_TEXT_EN_217,
		VOICE_TEXT_EN_218,
		VOICE_TEXT_EN_219,
		VOICE_TEXT_EN_220,
		VOICE_TEXT_EN_221};

/**
 * @brief	音声構築
 */
E_SC_RESULT RG_CTL_EN_SetGuideVoice_Normal(RG_CTL_MAIN_t *guidectl_p) {
	SC_LOG_DebugPrint(SC_TAG_RG, SC_LOG_START);

	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	E_RG_VCESTS vcests = e_RG_VCESTS_INVALID;
	RG_CTL_TRACK_t *new_p;
	RG_CTL_TRACK_t *old_p;
	UINT32 remain_dist;

	RT_VOICE_t dist_list = {};
	RT_VOICE_t turn_list = {};

	if (NULL == guidectl_p) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 前回・今回追跡情報取得
	new_p = &(guidectl_p->track[TRACK_NEW]);
	old_p = &(guidectl_p->track[TRACK_OLD]);

	// 今回最寄交差点なし
	if (!new_p->crs_vol) {
		return (e_SC_RESULT_SUCCESS);
	}

	// 残距離取得
	remain_dist = new_p->crs[0].remain_dist;

	// 交差点までの残距離が10km以上
	if (COMVCE_TIMING_FLW_CHK(remain_dist)) {
		// NOP
		vcests = e_RG_VCESTS_FLW;
	}
	// 交差点までの残距離が第１発話範囲内
	else if (GENVCE_TIMING_FST_EN_CHK(remain_dist)) {
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_EN_505);		// In about
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_EN_416);		// one quarter mile
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_305);			//,
		vcests = e_RG_VCESTS_FST;
	}
	// 交差点までの残距離が第２発話範囲内
	else if (GENVCE_TIMING_SCD_EN_CHK(remain_dist)) {
		vcests = e_RG_VCESTS_SCD;
	}
	// 交差点までの残距離が第３発話範囲内
	else if (GENVCE_TIMING_LST_EN_CHK(remain_dist)) {
		// NOP
		vcests = e_RG_VCESTS_LST;
	}

	// 前回最寄交差点と今回最寄交差点が同じ
	if (new_p->crs[0].crs_no == old_p->crs[0].crs_no) {
		// 前回発話状態が今回発話状態以上
		if (vcests <= old_p->voice.status) {
			// 文字列は格納せず、ステータスのみ設定
			new_p->voice.status = old_p->voice.status;
			return (e_SC_RESULT_SUCCESS);
		}
	}

	// 方向取得
	switch (new_p->crs[0].turn_dir) {
	case TURN_ST: RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_EN_401); break;
	case TURN_UT: RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_EN_402); break;
	case TURN_FR: RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_EN_403); break;
	case TURN_R:  RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_EN_404); break;
	case TURN_BR: RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_EN_405); break;
	case TURN_FL: RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_EN_406); break;
	case TURN_L:  RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_EN_407); break;
	case TURN_BL: RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_EN_408); break;
	default: return (e_SC_RESULT_SUCCESS);
	}

	// 音声構築
	switch (vcests) {
	case e_RG_VCESTS_FLW:

		RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &dist_list);
		RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_107);		//Continue on the current route.
		new_p->voice.status = vcests;
		break;

	case e_RG_VCESTS_FST:
	case e_RG_VCESTS_SCD:

		RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &dist_list);
		RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &turn_list);
		new_p->voice.status = vcests;

		// 次案内交差点あり
		if (2 == new_p->crs_vol) {
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_305);		//,
			ret = RG_CTL_EN_SetGuideVoice_Complex(guidectl_p);				// 複合
			if (e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
				return (e_SC_RESULT_FAIL);
			}
		} else {
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_306);		// .
		}
		break;

	case e_RG_VCESTS_LST:
		// 開始後の場合のみ
		if (ALLF16 == old_p->crs[0].crs_no) {
			RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &turn_list);
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_506);	//ahead.
			new_p->voice.status = vcests;
		}
		break;

	default:
		break;
	}

	SC_LOG_DebugPrint(SC_TAG_RG, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}


/**
 * @brief	音声構築
 */
E_SC_RESULT RG_CTL_EN_SetGuideVoice_HwyIn(RG_CTL_MAIN_t *guidectl_p) {
	SC_LOG_DebugPrint(SC_TAG_RG, SC_LOG_START);

	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	E_RG_VCESTS vcests = e_RG_VCESTS_INVALID;
	RG_CTL_TRACK_t *new_p;
	RG_CTL_TRACK_t *old_p;
	UINT32 remain_dist;

	RT_VOICE_t dist_list = {};
	RT_VOICE_t turn_list = {};

	if (NULL == guidectl_p) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 前回・今回追跡情報取得
	new_p = &(guidectl_p->track[TRACK_NEW]);
	old_p = &(guidectl_p->track[TRACK_OLD]);

	// 今回最寄交差点なし
	if (!new_p->crs_vol) {
		return (e_SC_RESULT_SUCCESS);
	}

	// 残距離取得
	remain_dist = new_p->crs[0].remain_dist;

	// 交差点までの残距離が第１発話範囲内
	if (GENVCE_TIMING_FST_EN_CHK(remain_dist)) {

		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_EN_505);		// In about
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_EN_416);		// one quarter mile
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_305);			//,
		vcests = e_RG_VCESTS_FST;
	}
	// 交差点までの残距離が第２発話範囲内
	else if (GENVCE_TIMING_SCD_EN_CHK(remain_dist)) {
		vcests = e_RG_VCESTS_SCD;
	}
	// 交差点までの残距離が第３発話範囲内
	else if (GENVCE_TIMING_LST_EN_CHK(remain_dist)) {
		// NOP
		vcests = e_RG_VCESTS_LST;
	}

	// 前回最寄交差点と今回最寄交差点が同じ
	if (new_p->crs[0].crs_no == old_p->crs[0].crs_no) {
		// 前回発話状態が今回発話状態以上
		if (vcests <= old_p->voice.status) {
			// 文字列は格納せず、ステータスのみ設定
			new_p->voice.status = old_p->voice.status;
			return (e_SC_RESULT_SUCCESS);
		}
	}

	// 方向取得
	switch (new_p->crs[0].turn_dir) {
	case TURN_RR:
		RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_EN_411);		//on your right
		break;
	case TURN_RL:
		RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_EN_412);		//on your left
		break;
	case TURN_ST:
		break;
	default:
		return (e_SC_RESULT_SUCCESS);
	}

	// 音声構築
	switch (vcests) {
	case e_RG_VCESTS_FLW:
		RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &dist_list);
		RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_107);		// Continue on the current route.
		new_p->voice.status = vcests;
		break;

	case e_RG_VCESTS_FST:
	case e_RG_VCESTS_SCD:
		RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &dist_list);
		RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_503);		//Highway entrance
		RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_305);			//,
		RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &turn_list);
		new_p->voice.status = vcests;

		// 次案内交差点あり
		if (2 == new_p->crs_vol) {
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_305);		//,
			ret = RG_CTL_EN_SetGuideVoice_Complex(guidectl_p);				// 複合
			if (e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
				return (e_SC_RESULT_FAIL);
			}
		} else {
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_306);		//.
		}
		break;

	case e_RG_VCESTS_LST:
		// 開始後の場合のみ
		if (ALLF16 == old_p->crs[0].crs_no) {
			if (TURN_ST == new_p->crs[0].turn_dir) {
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_503);	//Highway entrance
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_305);		//,
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_506);	//ahead.
			} else {
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_503);	//Highway entrance
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_305);		//,
				RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &turn_list);
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_306);		//.
			}
			new_p->voice.status = vcests;
		}
		break;

	default:
		break;
	}
	SC_LOG_DebugPrint(SC_TAG_RG, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);

}


/**
 * @brief	音声構築
 */
E_SC_RESULT RG_CTL_EN_SetGuideVoice_HwyOut(RG_CTL_MAIN_t *guidectl_p) {
	SC_LOG_DebugPrint(SC_TAG_RG, SC_LOG_START);

	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	E_RG_VCESTS vcests = e_RG_VCESTS_INVALID;
	RG_CTL_TRACK_t *new_p;
	RG_CTL_TRACK_t *old_p;
	UINT32 remain_dist;

	RT_VOICE_t dist_list = {};
	RT_VOICE_t turn_list = {};

	if (NULL == guidectl_p) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 前回・今回追跡情報取得
	new_p = &(guidectl_p->track[TRACK_NEW]);
	old_p = &(guidectl_p->track[TRACK_OLD]);

	// 今回最寄交差点なし
	if (!new_p->crs_vol) {
		return (e_SC_RESULT_SUCCESS);
	}

	// 残距離取得
	remain_dist = new_p->crs[0].remain_dist;

	// 交差点までの残距離が第１発話範囲内
	if (GENVCE_TIMING_FST_EN_CHK(remain_dist)) {
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_EN_505);		//In about
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_EN_416);		//one quarter mile
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_305);			//,
		vcests = e_RG_VCESTS_FST;
	}
	// 交差点までの残距離が第２発話範囲内
	else if (GENVCE_TIMING_SCD_EN_CHK(remain_dist)) {
		vcests = e_RG_VCESTS_SCD;
	}
	// 交差点までの残距離が第３発話範囲内
	else if (GENVCE_TIMING_LST_EN_CHK(remain_dist)) {
		// NOP
		vcests = e_RG_VCESTS_LST;
	}

	// 前回最寄交差点と今回最寄交差点が同じ
	if (new_p->crs[0].crs_no == old_p->crs[0].crs_no) {
		// 前回発話状態が今回発話状態以上
		if (vcests <= old_p->voice.status) {
			// 文字列は格納せず、ステータスのみ設定
			new_p->voice.status = old_p->voice.status;
			return (e_SC_RESULT_SUCCESS);
		}
	}

	// 方向取得
	switch (new_p->crs[0].turn_dir) {
	case TURN_RR:
		RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_EN_411);		//on your right
		break;
	case TURN_RL:
		RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_EN_412);		//on your left
		break;
	case TURN_ST:
		break;
	default:
		return (e_SC_RESULT_SUCCESS);
	}

	// 音声構築
	switch (vcests) {
	case e_RG_VCESTS_FLW:
		RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &dist_list);
		RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_107);		// Continue on the current route.
		new_p->voice.status = vcests;
		break;

	case e_RG_VCESTS_FST:
	case e_RG_VCESTS_SCD:
		RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &dist_list);
		RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_504);		//Highway exit
		RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_305);			//,
		RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &turn_list);

		new_p->voice.status = vcests;

		// 次案内交差点あり
		if (2 == new_p->crs_vol) {
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_305);		//,
			ret = RG_CTL_EN_SetGuideVoice_Complex(guidectl_p);				// 複合
			if (e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
				return (e_SC_RESULT_FAIL);
			}
		} else {
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_306);		//.
		}
		break;

	case e_RG_VCESTS_LST:
		// 開始後の場合のみ
		if (ALLF16 == old_p->crs[0].crs_no) {
			if (TURN_ST == new_p->crs[0].turn_dir) {
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_504);	//Highway exit
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_305);		//,
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_506);	//ahead.
			} else {
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_504);	//Highway exit
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_305);		//,
				RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &turn_list);		//[方向C]
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_306);		//.
			}
			new_p->voice.status = vcests;
		}
		break;

	default:
		break;
	}
	SC_LOG_DebugPrint(SC_TAG_RG, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);

}


/**
 * @brief	音声構築
 */
E_SC_RESULT RG_CTL_EN_SetGuideVoice_HwyJct(RG_CTL_MAIN_t *guidectl_p) {
	SC_LOG_DebugPrint(SC_TAG_RG, SC_LOG_START);

	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	E_RG_VCESTS vcests = e_RG_VCESTS_INVALID;
	RG_CTL_TRACK_t *new_p;
	RG_CTL_TRACK_t *old_p;
	UINT32 remain_dist;

	RT_VOICE_t dist_list = {};
	RT_VOICE_t turn_list = {};

	if (NULL == guidectl_p) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 前回・今回追跡情報取得
	new_p = &(guidectl_p->track[TRACK_NEW]);
	old_p = &(guidectl_p->track[TRACK_OLD]);

	// 今回最寄交差点なし
	if (!new_p->crs_vol) {
		return (e_SC_RESULT_SUCCESS);
	}

	// 残距離取得
	remain_dist = new_p->crs[0].remain_dist;

	// 交差点までの残距離が第１発話範囲内
	if (GENVCE_TIMING_FST_EN_CHK(remain_dist)) {
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_EN_505);		//In about
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_EN_416);		//one quarter mile
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_305);			//,
		vcests = e_RG_VCESTS_FST;
	}
	// 交差点までの残距離が第２発話範囲内
	else if (GENVCE_TIMING_SCD_EN_CHK(remain_dist)) {
		vcests = e_RG_VCESTS_SCD;
	}
	// 交差点までの残距離が第３発話範囲内
	else if (GENVCE_TIMING_LST_EN_CHK(remain_dist)) {
		// NOP
		vcests = e_RG_VCESTS_LST;
	}

	// 前回最寄交差点と今回最寄交差点が同じ
	if (new_p->crs[0].crs_no == old_p->crs[0].crs_no) {
		// 前回発話状態が今回発話状態以上
		if (vcests <= old_p->voice.status) {
			// 文字列は格納せず、ステータスのみ設定
			new_p->voice.status = old_p->voice.status;
			return (e_SC_RESULT_SUCCESS);
		}
	}

	// 方向取得
	switch (new_p->crs[0].turn_dir) {
	case TURN_RR: RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_EN_409); break;	//keep right
	case TURN_RL: RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_EN_410); break;	//keep left
	default: return (e_SC_RESULT_SUCCESS);
	}

	// 音声構築
	switch (vcests) {
	case e_RG_VCESTS_FLW:
		RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &dist_list);
		RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_107);		// Continue on the current route.
		new_p->voice.status = vcests;
		break;

	case e_RG_VCESTS_FST:
	case e_RG_VCESTS_SCD:
		RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &dist_list);
		RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &turn_list);
		new_p->voice.status = vcests;

		// 次案内交差点あり
		if (2 == new_p->crs_vol) {
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_305);		//,
			ret = RG_CTL_EN_SetGuideVoice_Complex(guidectl_p);				// 複合
			if (e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
				return (e_SC_RESULT_FAIL);
			}
		} else {
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_306);		//.
		}
		break;

	case e_RG_VCESTS_LST:
		// 開始後の場合のみ
		if (ALLF16 == old_p->crs[0].crs_no) {
			RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &turn_list);
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_306);		//.
			new_p->voice.status = vcests;
		}
		break;

	default:
		break;
	}
	SC_LOG_DebugPrint(SC_TAG_RG, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);

}


/**
 * @brief	音声構築
 */
E_SC_RESULT RG_CTL_EN_SetGuideVoice_Waypt(RG_CTL_MAIN_t *guidectl_p) {
	SC_LOG_DebugPrint(SC_TAG_RG, SC_LOG_START);

	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	E_RG_VCESTS vcests = e_RG_VCESTS_INVALID;
	RG_CTL_TRACK_t *new_p;
	RG_CTL_TRACK_t *old_p;
	UINT32 remain_dist;

	if (NULL == guidectl_p) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 前回・今回追跡情報取得
	new_p = &(guidectl_p->track[TRACK_NEW]);
	old_p = &(guidectl_p->track[TRACK_OLD]);

	// 今回最寄交差点なし
	if (!new_p->crs_vol) {
		return (e_SC_RESULT_SUCCESS);
	}

	// 残距離取得
	remain_dist = new_p->crs[0].remain_dist;

	// 交差点までの残距離が第３発話範囲内
	if (COMVCE_TIMING_ARV_CHK(remain_dist)) {
		// NOP
		vcests = e_RG_VCESTS_ARV;
	}

	// 前回最寄交差点と今回最寄交差点が同じ
	if (new_p->crs[0].crs_no == old_p->crs[0].crs_no) {
		// 前回発話状態が今回発話状態以上
		if (vcests <= old_p->voice.status) {
			// 文字列は格納せず、ステータスのみ設定
			new_p->voice.status = old_p->voice.status;
			return (e_SC_RESULT_SUCCESS);
		}
	}

	// 音声構築
	switch (vcests) {
	case e_RG_VCESTS_ARV:
		if (SC_CORE_RP_PLACE_MAX - 3 < new_p->link.sect_no) {
			SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] waypoint no error." HERE);
			break;
		}
		RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_502);		//You have reached your waypoint
		RT_SET_VOICE_LIST(&new_p->voice.tts.voice, m_WayPointList[new_p->link.sect_no]);	// waypoint no
		new_p->voice.status = vcests;

		// 次案内交差点あり
		if (2 == new_p->crs_vol) {
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_305);		//,
			ret = RG_CTL_EN_SetGuideVoice_Complex(guidectl_p);				// 複合
			if (e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
				return (e_SC_RESULT_FAIL);
			}
		} else {
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_306);		//.
		}
		break;

	default:
		break;
	}
	SC_LOG_DebugPrint(SC_TAG_RG, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);

}

/**
 * @brief	音声構築
 */
E_SC_RESULT RG_CTL_EN_SetGuideVoice_Dest(RG_CTL_MAIN_t *guidectl_p) {
	SC_LOG_DebugPrint(SC_TAG_RG, SC_LOG_START);

	//E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	RG_CTL_TRACK_t *new_p;

	if (NULL == guidectl_p) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 今回追跡情報取得
	new_p = &(guidectl_p->track[TRACK_NEW]);

	if (guidectl_p->arrival_f && new_p->voice.status != e_RG_VCESTS_ARV) {
		RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_104);		//You have reached your destination. Ending route guidance.
		new_p->voice.status = e_RG_VCESTS_ARV;
	}

	SC_LOG_DebugPrint(SC_TAG_RG, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);

}

/**
 * @brief	音声構築
 */
E_SC_RESULT RG_CTL_EN_SetGuideVoice_Split(RG_CTL_MAIN_t *guidectl_p) {
	SC_LOG_DebugPrint(SC_TAG_RG, SC_LOG_START);

	//E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	E_RG_VCESTS vcests = e_RG_VCESTS_INVALID;
	RG_CTL_TRACK_t *new_p;
	RG_CTL_TRACK_t *old_p;
	UINT32 remain_dist;

	if (NULL == guidectl_p) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 前回・今回追跡情報取得
	new_p = &(guidectl_p->track[TRACK_NEW]);
	old_p = &(guidectl_p->track[TRACK_OLD]);

	// 今回最寄交差点なし
	if (!new_p->crs_vol) {
		return (e_SC_RESULT_SUCCESS);
	}

	// 残距離取得
	remain_dist = new_p->crs[0].remain_dist;

	if (COMVCE_TIMING_APP_CHK(remain_dist)) {
		vcests = e_RG_VCESTS_APP;
	}

	// 前回最寄交差点と今回最寄交差点が同じ
	if (new_p->crs[0].crs_no == old_p->crs[0].crs_no) {
		// 前回発話状態が今回発話状態以上
		if (vcests <= old_p->voice.status) {
			// 文字列は格納せず、ステータスのみ設定
			new_p->voice.status = old_p->voice.status;
			return (e_SC_RESULT_SUCCESS);
		}
	}

	// 音声構築
	switch (vcests) {
	case e_RG_VCESTS_APP:
		RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_106);		// No route guidance from here. Follow actual traffic regulations.
		new_p->voice.status = vcests;
		break;
	default:
		break;
	}
	SC_LOG_DebugPrint(SC_TAG_RG, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);

}

/**
 * @brief	音声構築
 */
E_SC_RESULT RG_CTL_EN_SetGuideVoice_Complex(RG_CTL_MAIN_t *guidectl_p) {
	SC_LOG_DebugPrint(SC_TAG_RG, SC_LOG_START);

	//E_RG_VCESTS vcests = e_RG_VCESTS_INVALID;
	RG_CTL_TRACK_t *new_p;
	RG_CTL_TRACK_t *old_p;
	UINT32 remain_dist;

	RT_VOICE_t turn_list = {};
	//RT_VOICE_t crsn_list = {};

	if (NULL == guidectl_p) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 前回・今回追跡情報取得
	new_p = &(guidectl_p->track[TRACK_NEW]);
	old_p = &(guidectl_p->track[TRACK_OLD]);

	// 次交差点なし
	if (2 > new_p->crs_vol) {
		return (e_SC_RESULT_SUCCESS);
	}

	// 第１交差点ー第２交差点間離取得
	remain_dist = new_p->crs[1].remain_dist - new_p->crs[0].remain_dist;

	switch (new_p->crs[1].crs_type) {
	case CRSTYPE_NORMAL:
		if (GENVCE_TIMING_LST_EN_CHK(remain_dist)) {

			// 方向取得
			switch (new_p->crs[1].turn_dir) {
			case TURN_ST: RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_EN_401); break;		//go straight
			case TURN_UT: RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_EN_402); break;		//make a u-turn
			case TURN_FR: RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_EN_403); break;		//slight right turn
			case TURN_R:  RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_EN_404); break;		//right turn
			case TURN_BR: RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_EN_405); break;		//sharp right turn
			case TURN_FL: RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_EN_406); break;		//slight left turn
			case TURN_L:  RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_EN_407); break;		//left turn
			case TURN_BL: RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_EN_408); break;		//sharp left turn
			default: return (e_SC_RESULT_SUCCESS);
			}

			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_509);		//then
			RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &turn_list);
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_306);			//.
		}
		break;

	case CRSTYPE_HWYIN:
		if (GENVCE_TIMING_LST_EN_CHK(remain_dist)) {

			// 方向取得
			switch (new_p->crs[1].turn_dir) {
			case TURN_ST: break;
			case TURN_RR: RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_EN_411); break;		//on your right
			case TURN_RL: RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_EN_412); break;		//on your left
			default: return (e_SC_RESULT_SUCCESS);
			}

			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_509);		//then
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_503);		//Highway entrance
			RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &turn_list);
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_306);			//.
		}
		break;

	case CRSTYPE_HWYOT:
		if (GENVCE_TIMING_LST_EN_CHK(remain_dist)) {

			// 方向取得
			switch (new_p->crs[1].turn_dir) {
			case TURN_ST: break;
			case TURN_RR: RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_EN_411); break;		//on your right
			case TURN_RL: RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_EN_412); break;		//on your left
			default: return (e_SC_RESULT_SUCCESS);
			}

			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_509);		//then
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_504);		//Highway exit
			RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &turn_list);
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_306);			//.
		}
		break;

	case CRSTYPE_HWYJCT:
		if (GENVCE_TIMING_LST_EN_CHK(remain_dist)) {

			// 方向取得
			switch (new_p->crs[1].turn_dir) {
			case TURN_RR: RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_EN_409); break;		//keep right
			case TURN_RL: RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_EN_410); break;		//keep left
			default: return (e_SC_RESULT_SUCCESS);
			}

			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_509);		//then
			RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &turn_list);
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_306);			//.
		}
		break;

	case CRSTYPE_WAYPT:
		if (GENVCE_TIMING_LST_EN_CHK(remain_dist)) {
			INT16 sectno = 0;
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_509);		//then
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_502);		//You have reached your waypoint
			// n番目
			if (CRSTYPE_WAYPT == new_p->crs[0].crs_type) {
				sectno = new_p->link.sect_no + 1;
			} else {
				sectno = new_p->link.sect_no;
			}
			if (SC_CORE_RP_PLACE_MAX - 3 < sectno) {
				SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] waypoint no error." HERE);
				return (e_SC_RESULT_FAIL);
			}
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, m_WayPointList[sectno]);
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_306);			//.
		}
		break;

	case CRSTYPE_DEST:
		if (GENVCE_TIMING_LST_EN_CHK(remain_dist)) {
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_509);		//then
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_105);		//You have reached your destination.
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_306);			//.
		}
		break;

	case CRSTYPE_RA:
		if (GENVCE_TIMING_LST_EN_CHK(remain_dist)) {
			if (0 < new_p->crs[1].ra_exit_no) {
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_509);	//then
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_507);	//there is a roundabout
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_305);		//,
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_508);	//take the
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, m_RaundBairdList[new_p->crs[1].ra_exit_no]);		// n番目
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_510);	//exit
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_305);		//.
			}
			else {
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_509);	//then
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_507);	//there is a roundabout
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_305);		//.
			}
		}
		break;

	default:
		// 処理不要の為、何もしない
		break;
	}

	SC_LOG_DebugPrint(SC_TAG_RG, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);

}

/**
 * @brief	音声構築
 */
E_SC_RESULT RG_CTL_EN_SetGuideVoice_Start(RG_CTL_MAIN_t *guidectl_p) {
	SC_LOG_DebugPrint(SC_TAG_RG, SC_LOG_START);

	RG_CTL_TRACK_t *new_p;
	RG_CTL_TRACK_t *old_p;

	if (NULL == guidectl_p) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 前回・今回追跡情報取得
	new_p = &(guidectl_p->track[TRACK_NEW]);
	old_p = &(guidectl_p->track[TRACK_OLD]);

	// シミュレーション判定
	if (guidectl_p->simulate_f) {
		// シミュレーション時は開始案内しない。
		return (e_SC_RESULT_SUCCESS);
	}

	// 前回誘導情報なし＝今回初誘導
	if (ALLF16 == old_p->link.route_link_no) {

		// 定位置案内あり
		if (new_p->voice.tts.voice.current) {
			// 開始案内文字列＋案内文字列を設定（0番目に追加）
			RG_CTL_InsertVoiceList(&new_p->voice.tts.voice, VOICE_TEXT_EN_103, 0);	// Starting route guidance to the destination.
		} else {
			// 開始案内文字列を設定
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_103);			// Starting route guidance to the destination.
		}
	}

	SC_LOG_DebugPrint(SC_TAG_RG, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);

}

/**
 * @brief	音声構築
 */
E_SC_RESULT RG_CTL_EN_SetGuideVoice_Ra(RG_CTL_MAIN_t *guidectl_p) {
	SC_LOG_DebugPrint(SC_TAG_RG, SC_LOG_START);

	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	E_RG_VCESTS vcests = e_RG_VCESTS_INVALID;
	RG_CTL_TRACK_t *new_p;
	RG_CTL_TRACK_t *old_p;
	UINT32 remain_dist;

	RT_VOICE_t dist_list = {};

	if (NULL == guidectl_p) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 前回・今回追跡情報取得
	new_p = &(guidectl_p->track[TRACK_NEW]);
	old_p = &(guidectl_p->track[TRACK_OLD]);

	// 今回最寄交差点なし
	if (!new_p->crs_vol) {
		return (e_SC_RESULT_SUCCESS);
	}

	// 残距離取得
	remain_dist = new_p->crs[0].remain_dist;

	// 交差点までの残距離が10km以上
	if (COMVCE_TIMING_FLW_CHK(remain_dist)) {
		// NOP
		vcests = e_RG_VCESTS_FLW;
	}
	// 交差点までの残距離が第１発話範囲内
	else if (GENVCE_TIMING_FST_EN_CHK(remain_dist)) {
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_EN_505);		//In about
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_EN_416);		//one quarter mile
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_305);			//,
		vcests = e_RG_VCESTS_FST;
	}
	// 交差点までの残距離が第２発話範囲内
	else if (GENVCE_TIMING_SCD_EN_CHK(remain_dist)) {
		vcests = e_RG_VCESTS_SCD;
	}
	// 交差点までの残距離が第３発話範囲内
	else if (GENVCE_TIMING_LST_EN_CHK(remain_dist)) {
		// NOP
		vcests = e_RG_VCESTS_LST;
	}

	// 前回最寄交差点と今回最寄交差点が同じ
	if (new_p->crs[0].crs_no == old_p->crs[0].crs_no) {
		// 前回発話状態が今回発話状態以上
		if (vcests <= old_p->voice.status) {
			// 文字列は格納せず、ステータスのみ設定
			new_p->voice.status = old_p->voice.status;
			return (e_SC_RESULT_SUCCESS);
		}
	}

	// 音声構築
	switch (vcests) {
	case e_RG_VCESTS_FLW:
		RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &dist_list);
		RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_107);		//Continue on the current route.
		new_p->voice.status = vcests;
		break;

	case e_RG_VCESTS_FST:
	case e_RG_VCESTS_SCD:

		if (0 < new_p->crs[0].ra_exit_no) {
			RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &dist_list);
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_507);	//there is a roundabout
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_305);		//,
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_508);	//take the
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, m_RaundBairdList[new_p->crs[0].ra_exit_no]);		// n番目
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_510);	//exit
			new_p->voice.status = vcests;
		} else {
			RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &dist_list);
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_507);	//there is a roundabout
			new_p->voice.status = vcests;
		}

		// 次案内交差点あり
		if (2 == new_p->crs_vol) {
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_305);		//,
			ret = RG_CTL_EN_SetGuideVoice_Complex(guidectl_p);				// 複合
			if (e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
				return (e_SC_RESULT_FAIL);
			}
		} else {
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_306);		//.
		}
		break;

	case e_RG_VCESTS_LST:
		// 開始後の場合のみ
		if (ALLF16 == old_p->crs[0].crs_no) {
			if (0 < new_p->crs[0].ra_exit_no) {
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_507);	//there is a roundabout
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_305);		//,
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_508);	//take the
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, m_RaundBairdList[new_p->crs[0].ra_exit_no]);		// n番目
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_510);	//exit
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_306);		//.
			} else {
				RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &dist_list);
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_EN_507);	//there is a roundabout
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_306);		//.
			}
			new_p->voice.status = vcests;
		}
		break;

	default:
		break;
	}
	SC_LOG_DebugPrint(SC_TAG_RG, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);

}


