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
 * RG_GuideVoiceBuild_jp.c
 *
 *  Created on: 2014/11/18
 *      Author: masutani
 */

#include "sms-core/SMCoreDM/SMCoreDMInternal.h"

// 経由地番号
const static INT32 m_WayPointList[] = {
		VOICE_TEXT_JP_201,
		VOICE_TEXT_JP_202,
		VOICE_TEXT_JP_203,
		VOICE_TEXT_JP_204,
		VOICE_TEXT_JP_205};
// ラウンドアバウト出口番号
const static INT32 m_RaundBairdList[] = {
		0,
		VOICE_TEXT_JP_206,
		VOICE_TEXT_JP_207,
		VOICE_TEXT_JP_208,
		VOICE_TEXT_JP_209,
		VOICE_TEXT_JP_210,
		VOICE_TEXT_JP_211,
		VOICE_TEXT_JP_212,
		VOICE_TEXT_JP_213,
		VOICE_TEXT_JP_214,
		VOICE_TEXT_JP_215,
		VOICE_TEXT_JP_216,
		VOICE_TEXT_JP_217,
		VOICE_TEXT_JP_218,
		VOICE_TEXT_JP_219,
		VOICE_TEXT_JP_220,
		VOICE_TEXT_JP_221};

/**
 * @brief	音声構築
 */
E_SC_RESULT RG_CTL_JP_SetGuideVoice_Normal(RG_CTL_MAIN_t *guidectl_p)
{
	SC_LOG_DebugPrint(SC_TAG_RG, SC_LOG_START);

	E_SC_RESULT				ret = e_SC_RESULT_SUCCESS;
	RG_CTL_TRACK_t			*new_p;
	RG_CTL_TRACK_t			*old_p;
	E_RG_VCESTS				vcests = e_RG_VCESTS_INVALID;
	UINT32					remain_dist;

	RT_VOICE_t dist_list = {};
	RT_VOICE_t turn_list = {};
	RT_VOICE_t crsn_list = {};

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
	if (COMVCE_TIMING_FLW_CHK(remain_dist)){
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_503);		//この先
		vcests = e_RG_VCESTS_FLW;
	}
	// 交差点までの残距離が第１発話範囲内
	else if (GENVCE_TIMING_FST_JP_CHK(remain_dist)) {
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_502);		// およそ
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_415);		// 300m
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_505);		// 先
		vcests = e_RG_VCESTS_FST;
	}
	// 交差点までの残距離が第２発話範囲内
	else if (GENVCE_TIMING_SCD_JP_CHK(remain_dist)) {
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_502);		// およそ
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_416);		// 100m
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_505);		// 先
		vcests = e_RG_VCESTS_SCD;
	}
	// 交差点までの残距離が第３発話範囲内
	else if (GENVCE_TIMING_LST_JP_CHK(remain_dist)) {
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_501);		//まもなく
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

	// 交差点名称取得
	if (new_p->crs[0].crs_name.len) {
		sprintf(crsn_list.valiavbleName, "%s", new_p->crs[0].crs_name.name);
		RT_SET_VOICE_LIST(&crsn_list, VOICE_TEXT_302);			//name
		RT_SET_VOICE_LIST(&crsn_list, VOICE_TEXT_JP_508);		//交差点を
	}

	// 方向取得
	switch (new_p->crs[0].turn_dir) {
	case TURN_ST:	RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_JP_401);	break;
	case TURN_UT:	RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_JP_402);	break;
	case TURN_FR:	RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_JP_403);	break;
	case TURN_R:	RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_JP_404);	break;
	case TURN_BR:	RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_JP_405);	break;
	case TURN_FL:	RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_JP_406);	break;
	case TURN_L:	RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_JP_407);	break;
	case TURN_BL:	RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_JP_408);	break;
	default:
		return (e_SC_RESULT_SUCCESS);
	}

	// 音声構築
	switch (vcests) {
	case e_RG_VCESTS_FLW:

		RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &dist_list);
		RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_303);			//、
		RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_515);		//暫く道なりです。

		new_p->voice.status = vcests;
		break;

	case e_RG_VCESTS_FST:
	case e_RG_VCESTS_SCD:

		RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &dist_list);
		RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_303);			//、
		RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &crsn_list);
		RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &turn_list);
		RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_510);		//です。

		new_p->voice.status = vcests;

		// 次案内交差点あり
		if (2 == new_p->crs_vol) {
			ret = RG_CTL_JP_SetGuideVoice_Complex(guidectl_p);		// 複合
			if (e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
				return (e_SC_RESULT_FAIL);
			}
		}
		break;

	case e_RG_VCESTS_LST:

		// 開始後の場合のみ
		if (ALLF16 == old_p->crs[0].crs_no) {

			RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &dist_list);
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_303);			//、
			RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &crsn_list);
			RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &turn_list);
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_510);		//です。

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
E_SC_RESULT RG_CTL_JP_SetGuideVoice_HwyIn(RG_CTL_MAIN_t *guidectl_p)
{
	E_SC_RESULT				ret = e_SC_RESULT_SUCCESS;
	RG_CTL_TRACK_t			*new_p;
	RG_CTL_TRACK_t			*old_p;
	E_RG_VCESTS				vcests = e_RG_VCESTS_INVALID;
	UINT32					remain_dist;

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
	if (GENVCE_TIMING_FST_JP_CHK(remain_dist)) {

		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_502);		// およそ
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_415);		// 300m
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_505);		// 先
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_303);			//、
		vcests = e_RG_VCESTS_FST;
	}
	// 交差点までの残距離が第２発話範囲内
	else if (GENVCE_TIMING_SCD_JP_CHK(remain_dist)) {

		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_502);		// およそ
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_416);		// 100m
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_505);		// 先
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_303);			//、
		vcests = e_RG_VCESTS_SCD;
	}
	// 交差点までの残距離が第３発話範囲内
	else if (GENVCE_TIMING_LST_JP_CHK(remain_dist)) {
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_501);		// まもなく
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
		RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_JP_404);	// 右方向
		RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_303);		//、
		break;
	case TURN_RL:
		RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_JP_407);	// 左方向
		RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_303);		//、
		break;
	case TURN_ST:
		break;
	default:
		return (e_SC_RESULT_SUCCESS);
	}

	// 音声構築
	switch (vcests) {
	case e_RG_VCESTS_FLW:
		RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &dist_list);			//[距離]
		RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_515);		// 暫く道なりです

		new_p->voice.status = vcests;
		break;

	case e_RG_VCESTS_FST:
	case e_RG_VCESTS_SCD:
		RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &dist_list);			//[距離]
		RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &turn_list);			//[方向C]
		RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_512);		//高速道路入口です。

		new_p->voice.status = vcests;

		// 次案内交差点あり
		if (2 == new_p->crs_vol) {
			ret = RG_CTL_JP_SetGuideVoice_Complex(guidectl_p);				// 複合
			if (e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
				return (e_SC_RESULT_FAIL);
			}
		}
		break;

	case e_RG_VCESTS_LST:
		// 開始後の場合のみ
		if (ALLF16 == old_p->crs[0].crs_no) {
			RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &dist_list);		//[距離]
			RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &turn_list);		//[方向C]
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_512);	//高速道路入口です。

			new_p->voice.status = vcests;
		}
		break;

	default:
		break;
	}
	return (e_SC_RESULT_SUCCESS);

}


/**
 * @brief	音声構築
 */
E_SC_RESULT RG_CTL_JP_SetGuideVoice_HwyOut(RG_CTL_MAIN_t *guidectl_p)
{
	E_SC_RESULT				ret = e_SC_RESULT_SUCCESS;
	RG_CTL_TRACK_t			*new_p;
	RG_CTL_TRACK_t			*old_p;
	E_RG_VCESTS				vcests = e_RG_VCESTS_INVALID;
	UINT32					remain_dist;

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
	if (HWYVCE_TIMING_FST_JP_CHK(remain_dist)) {
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_502);		// およそ
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_419);		// 1km
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_505);		// 先
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_303);			//、

		vcests = e_RG_VCESTS_FST;
	}
	// 交差点までの残距離が第２発話範囲内
	else if (HWYVCE_TIMING_SCD_JP_CHK(remain_dist)) {
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_502);		// およそ
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_420);		// 500m
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_505);		// 先
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_303);			//、

		vcests = e_RG_VCESTS_SCD;
	}
	// 交差点までの残距離が第３発話範囲内
	else if (HWYVCE_TIMING_LST_JP_CHK(remain_dist)) {
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_501);		// まもなく

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
		RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_JP_404);		// 右方向
		RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_303);			//、
		break;
	case TURN_RL:
		RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_JP_407);		// 左方向
		RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_303);			//、
		break;
	case TURN_ST:
		break;
	default:
		return (e_SC_RESULT_SUCCESS);
	}

	// 音声構築
	switch (vcests) {
	case e_RG_VCESTS_FLW:
		RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &dist_list);			//[距離]
		RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_515);		// 暫く道なりです

		new_p->voice.status = vcests;
		break;

	case e_RG_VCESTS_FST:
	case e_RG_VCESTS_SCD:
		RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &dist_list);			//[距離]
		RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &turn_list);			//[方向C]
		RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_513);		//高速道路出口です。

		new_p->voice.status = vcests;

		// 次案内交差点あり
		if (2 == new_p->crs_vol) {
			ret = RG_CTL_JP_SetGuideVoice_Complex(guidectl_p);				// 複合
			if (e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
				return (e_SC_RESULT_FAIL);
			}
		}
		break;

	case e_RG_VCESTS_LST:
		// 開始後の場合のみ
		if (ALLF16 == old_p->crs[0].crs_no) {
			RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &dist_list);		//[距離]
			RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &turn_list);		//[方向C]
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_513);	//高速道路出口です。

			new_p->voice.status = vcests;
		}
		break;

	default:
		break;
	}
	return (e_SC_RESULT_SUCCESS);

}


/**
 * @brief	音声構築
 */
E_SC_RESULT RG_CTL_JP_SetGuideVoice_HwyJct(RG_CTL_MAIN_t *guidectl_p)
{
	E_SC_RESULT				ret = e_SC_RESULT_SUCCESS;
	RG_CTL_TRACK_t			*new_p;
	RG_CTL_TRACK_t			*old_p;
	E_RG_VCESTS				vcests = e_RG_VCESTS_INVALID;
	UINT32					remain_dist;

	RT_VOICE_t dist_list = {};
	RT_VOICE_t turn_list = {};
	RT_VOICE_t crsn_list = {};

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
	if (HWYVCE_TIMING_FST_JP_CHK(remain_dist)) {
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_502);		// およそ
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_419);		// 1km
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_505);		// 先
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_303);			//、

		vcests = e_RG_VCESTS_FST;
	}
	// 交差点までの残距離が第２発話範囲内
	else if (HWYVCE_TIMING_SCD_JP_CHK(remain_dist)) {
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_502);		// およそ
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_420);		// 500m
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_505);		// 先
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_303);			//、

		vcests = e_RG_VCESTS_SCD;
	}
	// 交差点までの残距離が第３発話範囲内
	else if (HWYVCE_TIMING_LST_JP_CHK(remain_dist)) {
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_501);		// まもなく
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_303);			//、

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

	// 交差点名称取得
	if (new_p->crs[0].crs_name.len) {
		sprintf(crsn_list.valiavbleName, "%s", new_p->crs[0].crs_name.name);
		RT_SET_VOICE_LIST(&crsn_list, VOICE_TEXT_302);			// name
		RT_SET_VOICE_LIST(&crsn_list, VOICE_TEXT_JP_506);		// を
	}

	// 方向取得
	switch (new_p->crs[0].turn_dir) {
	case TURN_RR:	RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_JP_404);		break;
	case TURN_RL:	RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_JP_407);		break;
	default:
		return (e_SC_RESULT_SUCCESS);
	}

	// 音声構築
	switch (vcests) {
	case e_RG_VCESTS_FLW:
		RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &dist_list);			//[距離]
		RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_515);		// 暫く道なりです

		new_p->voice.status = vcests;
		break;

	case e_RG_VCESTS_FST:
	case e_RG_VCESTS_SCD:
		RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &dist_list);			//[距離]
		RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &crsn_list);			//[JCT/IC]
		RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &turn_list);			//[方向B]
		RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_510);		//です。

		new_p->voice.status = vcests;

		// 次案内交差点あり
		if (2 == new_p->crs_vol) {
			ret = RG_CTL_JP_SetGuideVoice_Complex(guidectl_p);		// 複合
			if (e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
				return (e_SC_RESULT_FAIL);
			}
		}
		break;

	case e_RG_VCESTS_LST:
		// 開始後の場合のみ
		if (ALLF16 == old_p->crs[0].crs_no) {
			RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &dist_list);		//[距離]
			RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &crsn_list);		//[JCT/IC]
			RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &turn_list);		//[方向B]
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_510);	// です。

			new_p->voice.status = vcests;
		}
		break;

	default:
		break;
	}
	return (e_SC_RESULT_SUCCESS);

}


/**
 * @brief	音声構築
 */
E_SC_RESULT RG_CTL_JP_SetGuideVoice_Waypt(RG_CTL_MAIN_t *guidectl_p)
{
	E_SC_RESULT				ret = e_SC_RESULT_SUCCESS;
	RG_CTL_TRACK_t			*new_p;
	RG_CTL_TRACK_t			*old_p;
	E_RG_VCESTS				vcests = e_RG_VCESTS_INVALID;
	UINT32					remain_dist;

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

	// 交差点までの残距離が第３発話範囲内
	if (COMVCE_TIMING_ARV_CHK(remain_dist)) {
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_501);		// まもなく
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
		RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &dist_list);
		RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_414);		// 経由地
		RT_SET_VOICE_LIST(&new_p->voice.tts.voice, m_WayPointList[new_p->link.sect_no]);	// n番目
		RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_511);		// 付近です。

		new_p->voice.status = vcests;

		// 次案内交差点あり
		if (2 == new_p->crs_vol) {
			ret = RG_CTL_JP_SetGuideVoice_Complex(guidectl_p);		// 複合
			if (e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
				return (e_SC_RESULT_FAIL);
			}
		}
		break;

	default:
		break;
	}
	return (e_SC_RESULT_SUCCESS);

}


/**
 * @brief	音声構築
 */
E_SC_RESULT RG_CTL_JP_SetGuideVoice_Dest(RG_CTL_MAIN_t *guidectl_p)
{
	//E_SC_RESULT				ret = e_SC_RESULT_SUCCESS;
	RG_CTL_TRACK_t			*new_p;

	if (NULL == guidectl_p) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 今回追跡情報取得
	new_p = &(guidectl_p->track[TRACK_NEW]);

	if (guidectl_p->arrival_f && new_p->voice.status != e_RG_VCESTS_ARV) {
		RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_104);		// まもなく、目的地付近です。ルートガイドを終了します。

		new_p->voice.status = e_RG_VCESTS_ARV;
	}

	return (e_SC_RESULT_SUCCESS);

}

/**
 * @brief	音声構築
 */
E_SC_RESULT RG_CTL_JP_SetGuideVoice_Split(RG_CTL_MAIN_t *guidectl_p)
{
	//E_SC_RESULT				ret = e_SC_RESULT_SUCCESS;
	RG_CTL_TRACK_t			*new_p;
	RG_CTL_TRACK_t			*old_p;
	E_RG_VCESTS				vcests = e_RG_VCESTS_INVALID;
	UINT32					remain_dist;

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
		RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_105);		// この先、経路が切れています。実際の交通規制に従って走行して下さい。

		new_p->voice.status = vcests;
		break;
	default:
		break;
	}
	return (e_SC_RESULT_SUCCESS);

}

/**
 * @brief	音声構築
 */
E_SC_RESULT RG_CTL_JP_SetGuideVoice_Complex(RG_CTL_MAIN_t *guidectl_p)
{
	RG_CTL_TRACK_t			*new_p;
	RG_CTL_TRACK_t			*old_p;
	//E_RG_VCESTS				vcests = e_RG_VCESTS_INVALID;
	UINT32					remain_dist;

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
		if (GENVCE_TIMING_LST_JP_CHK(remain_dist)) {

			// 方向取得
			switch (new_p->crs[1].turn_dir) {
			case TURN_ST:	RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_JP_401);	break;		// "直進方向"
			case TURN_UT:	RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_JP_402);	break;		// "Ｕターン"
			case TURN_FR:	RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_JP_403);	break;		// "斜め右方向"
			case TURN_R:	RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_JP_404);	break;		// "右方向"
			case TURN_BR:	RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_JP_405);	break;		// "大きく右方向"
			case TURN_FL:	RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_JP_406);	break;		// "斜め左方向"
			case TURN_L:	RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_JP_407);	break;		// "左方向"
			case TURN_BL:	RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_JP_408);	break;		// "大きく左方向"
			default: return (e_SC_RESULT_SUCCESS);
			}

			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_504);		// その先
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_303);			// 、
			RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &turn_list);
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_510);		// です。
		}
		break;

	case CRSTYPE_HWYIN:
		if (GENVCE_TIMING_LST_JP_CHK(remain_dist)) {

			// 方向取得
			switch (new_p->crs[1].turn_dir) {
			case TURN_ST:	break;		// "直進方向"
			case TURN_RR:	RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_JP_404);	break;		// "右方向"
			case TURN_RL:	RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_JP_407);	break;		// "左方向"
			default: return (e_SC_RESULT_SUCCESS);
			}

			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_504);		// その先
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_303);			// 、
			RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &turn_list);
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_303);			// 、
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_512);		// 高速道路入口です。
		}
		break;

	case CRSTYPE_HWYOT:
		if (HWYVCE_TIMING_LST_JP_CHK(remain_dist)) {

			// 方向取得
			switch (new_p->crs[1].turn_dir) {
			case TURN_ST:	break;		// "直進方向"
			case TURN_RR:	RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_JP_404);	break;		// "右方向"
			case TURN_RL:	RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_JP_407);	break;		// "左方向"
			default: return (e_SC_RESULT_SUCCESS);
			}

			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_504);		// その先
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_303);			// 、
			RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &turn_list);
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_303);			// 、
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_513);		// 高速道路出口です。
		}
		break;

	case CRSTYPE_HWYJCT:
		if (HWYVCE_TIMING_LST_JP_CHK(remain_dist)) {

			// 方向取得
			switch (new_p->crs[1].turn_dir) {
			case TURN_RR:	RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_JP_404);	break;		// "右方向"
			case TURN_RL:	RT_SET_VOICE_LIST(&turn_list, VOICE_TEXT_JP_407);	break;		// "左方向"
			default: return (e_SC_RESULT_SUCCESS);
			}

			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_504);		// その先
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_303);			// 、
			RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &turn_list);
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_510);		// です。
		}
		break;

	case CRSTYPE_WAYPT:
		if (GENVCE_TIMING_LST_JP_CHK(remain_dist)) {
			INT16 sectno = 0;
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_504);		// その先
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_303);			// 、
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_414);		// 経由地
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
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_511);		// 付近です。
		}
		break;

	case CRSTYPE_DEST:
		if (GENVCE_TIMING_LST_JP_CHK(remain_dist)) {
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_504);		// その先
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_303);			// 、
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_413);		// 目的地
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_511);		// 付近です。
		}
		break;

	case CRSTYPE_RA:
		if (GENVCE_TIMING_LST_JP_CHK(remain_dist)) {
			if (0 < new_p->crs[1].ra_exit_no) {
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_504);		// その先
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_303);			// 、
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_102);		// ラウンドアバウトがあります。
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_509);		// 出口は手前から
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, m_RaundBairdList[new_p->crs[1].ra_exit_no]);		// n番目
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_510);		// です。
			}
			else {
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_504);		// その先
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_303);			// 、
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_102);		// ラウンドアバウトがあります。
			}
		}
		break;

	default:
		// 処理不要の為、何もしない
		break;
	}

	return (e_SC_RESULT_SUCCESS);

}

/**
 * @brief	音声構築
 */
E_SC_RESULT RG_CTL_JP_SetGuideVoice_Start(RG_CTL_MAIN_t *guidectl_p)
{
	RG_CTL_TRACK_t			*new_p;
	RG_CTL_TRACK_t			*old_p;

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
			RG_CTL_InsertVoiceList(&new_p->voice.tts.voice, VOICE_TEXT_JP_103, 0);	// 目的地へのルートガイドを開始します。
		}
		else {
			// 開始案内文字列を設定
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_103);		// 目的地へのルートガイドを開始します。
		}
	}

	return (e_SC_RESULT_SUCCESS);

}

/**
 * @brief	音声構築
 */
E_SC_RESULT RG_CTL_JP_SetGuideVoice_Ra(RG_CTL_MAIN_t *guidectl_p)
{
	E_SC_RESULT				ret = e_SC_RESULT_SUCCESS;
	RG_CTL_TRACK_t			*new_p;
	RG_CTL_TRACK_t			*old_p;
	E_RG_VCESTS				vcests = e_RG_VCESTS_INVALID;
	UINT32					remain_dist;

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
	if (COMVCE_TIMING_FLW_CHK(remain_dist)){
		RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_503);		// この先
		vcests = e_RG_VCESTS_FLW;
	}
	// 交差点までの残距離が第１発話範囲内
	else if (GENVCE_TIMING_FST_JP_CHK(remain_dist)) {
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_502);		// およそ
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_415);		// 300m
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_505);		// 先

		vcests = e_RG_VCESTS_FST;
	}
	// 交差点までの残距離が第２発話範囲内
	else if (GENVCE_TIMING_SCD_JP_CHK(remain_dist)) {
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_502);		// およそ
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_416);		// 100m
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_505);		// 先

		vcests = e_RG_VCESTS_SCD;
	}
	// 交差点までの残距離が第３発話範囲内
	else if (GENVCE_TIMING_LST_JP_CHK(remain_dist)) {
		RT_SET_VOICE_LIST(&dist_list, VOICE_TEXT_JP_501);		// まもなく

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
		RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_303);			// 、
		RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_515);		// 暫く道なりです。

		new_p->voice.status = vcests;
		break;

	case e_RG_VCESTS_FST:
	case e_RG_VCESTS_SCD:

		if (0 < new_p->crs[0].ra_exit_no) {
			RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &dist_list);
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_303);			// 、
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_102);		// ラウンドアバウトがあります。
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_509);		// 出口は手前から
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, m_RaundBairdList[new_p->crs[0].ra_exit_no]);		// n番目
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_510);		// です。

			new_p->voice.status = vcests;
		}
		else {
			RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &dist_list);
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_303);			// 、
			RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_102);		// ラウンドアバウトがあります。

			new_p->voice.status = vcests;
		}

		// 次案内交差点あり
		if (2 == new_p->crs_vol) {
			ret = RG_CTL_JP_SetGuideVoice_Complex(guidectl_p);		// 複合
			if (e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
				return (e_SC_RESULT_FAIL);
			}
		}
		break;

	case e_RG_VCESTS_LST:
		// 開始後の場合のみ
		if (ALLF16 == old_p->crs[0].crs_no) {
			if (0 < new_p->crs[0].ra_exit_no) {
				RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &dist_list);
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_102);		// ラウンドアバウトがあります。
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_509);		// 出口は手前から
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, m_RaundBairdList[new_p->crs[0].ra_exit_no]);		// n番目
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_510);		// です。
			}
			else {
				RG_CTL_JointVoiceList(&new_p->voice.tts.voice, &dist_list);
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_303);			// 、
				RT_SET_VOICE_LIST(&new_p->voice.tts.voice, VOICE_TEXT_JP_102);		// ラウンドアバウトがあります。
			}
			new_p->voice.status = vcests;
		}
		break;

	default:
		break;
	}
	return (e_SC_RESULT_SUCCESS);

}


