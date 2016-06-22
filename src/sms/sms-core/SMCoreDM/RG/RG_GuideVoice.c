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
 * @brief	音声構築
 */
E_SC_RESULT RG_CTL_SetGuideVoice(RT_TBL_MAIN_t *guidetbl_p, RG_CTL_MAIN_t *guidectl_p)
{
	SC_LOG_DebugPrint(SC_TAG_RG, SC_LOG_START);

	E_SC_RESULT				ret = e_SC_RESULT_SUCCESS;
	RG_CTL_TRACK_t			*new_p;
	INT32					language = SYS_LANGUAGE_INIT;

	if (NULL == guidetbl_p || NULL == guidectl_p) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 今回追跡情報取得
	new_p = &(guidectl_p->track[TRACK_NEW]);

	// 言語切り替え
	ret = SC_MNG_GetLanguage(&language);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] SC_MNG_GetLanguage " HERE);
		return (e_SC_RESULT_FAIL);
	}

	if (new_p->crs_vol > 0) {
		// JP
		if (SYS_LANGUAGE_JP == language) {
			// 交差点種別ごとの音声構築
			switch (new_p->crs[0].crs_type) {
			case CRSTYPE_NORMAL:
				ret = RG_CTL_JP_SetGuideVoice_Normal(guidectl_p);		// 一般交差点
				break;
			case CRSTYPE_HWYIN:
				ret = RG_CTL_JP_SetGuideVoice_HwyIn(guidectl_p);		// 高速入口
				break;
			case CRSTYPE_HWYOT:
				ret = RG_CTL_JP_SetGuideVoice_HwyOut(guidectl_p);		// 高速出口
				break;
			case CRSTYPE_HWYJCT:
				ret = RG_CTL_JP_SetGuideVoice_HwyJct(guidectl_p);		// 高速分岐
				break;
			case CRSTYPE_WAYPT:
				ret = RG_CTL_JP_SetGuideVoice_Waypt(guidectl_p);		// 経由地
				break;
			case CRSTYPE_DEST:
				ret = RG_CTL_JP_SetGuideVoice_Dest(guidectl_p);			// 目的地
				break;
			case CRSTYPE_SPLIT:
				ret = RG_CTL_JP_SetGuideVoice_Split(guidectl_p);		// 経路断裂点
				break;
			case CRSTYPE_RA:
				ret = RG_CTL_JP_SetGuideVoice_Ra(guidectl_p);			// ラウンドアバウト
				break;
			default:
				return (e_SC_RESULT_SUCCESS);
			}
		}
		// EN
		else if(SYS_LANGUAGE_EN == language){
			// 交差点種別ごとの音声構築
			switch (new_p->crs[0].crs_type) {
			case CRSTYPE_NORMAL:
				ret = RG_CTL_EN_SetGuideVoice_Normal(guidectl_p);		// 一般交差点
				break;
			case CRSTYPE_HWYIN:
				ret = RG_CTL_EN_SetGuideVoice_HwyIn(guidectl_p);		// 高速入口
				break;
			case CRSTYPE_HWYOT:
				ret = RG_CTL_EN_SetGuideVoice_HwyOut(guidectl_p);		// 高速出口
				break;
			case CRSTYPE_HWYJCT:
				ret = RG_CTL_EN_SetGuideVoice_HwyJct(guidectl_p);		// 高速分岐
				break;
			case CRSTYPE_WAYPT:
				ret = RG_CTL_EN_SetGuideVoice_Waypt(guidectl_p);		// 経由地
				break;
			case CRSTYPE_DEST:
				ret = RG_CTL_EN_SetGuideVoice_Dest(guidectl_p);			// 目的地
				break;
			case CRSTYPE_SPLIT:
				ret = RG_CTL_EN_SetGuideVoice_Split(guidectl_p);		// 経路断裂点
				break;
			case CRSTYPE_RA:
				ret = RG_CTL_EN_SetGuideVoice_Ra(guidectl_p);			// ラウンドアバウト
				break;
			default:
				return (e_SC_RESULT_SUCCESS);
			}
		}
		// unknown
		else {
			SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
			return (e_SC_RESULT_FAIL);
		}

		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
			return (e_SC_RESULT_FAIL);
		}

		// 再探索でない場合のみ
		if (e_SC_ROUTETYPE_REROUTE != guidetbl_p->rt_id.type) {
			// 開始案内
			SC_LOG_InfoPrint(SC_TAG_RG, "RG_CTL_SetGuideVoice_Start");
			if (SYS_LANGUAGE_JP == language) {
				ret = RG_CTL_JP_SetGuideVoice_Start(guidectl_p);
			} else {
				ret = RG_CTL_EN_SetGuideVoice_Start(guidectl_p);
			}
			if (e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
				return (e_SC_RESULT_FAIL);
			}
		}
	} else {
		SC_LOG_InfoPrint(SC_TAG_RG, "RG_CTL_SetGuideVoice_Dest");
		if (SYS_LANGUAGE_JP == language) {
			ret = RG_CTL_JP_SetGuideVoice_Dest(guidectl_p);			// 目的地 (逸脱)
		} else {
			ret = RG_CTL_EN_SetGuideVoice_Dest(guidectl_p);			// 目的地 (逸脱)
		}
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] ERROR " HERE);
			return (e_SC_RESULT_FAIL);
		}
	}

	if (new_p->voice.tts.voice.current) {
		SC_LOG_InfoPrint(SC_TAG_RG, "[音声発話♪]");
	}

	SC_LOG_DebugPrint(SC_TAG_RG, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}


