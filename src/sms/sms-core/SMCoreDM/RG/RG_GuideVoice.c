/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 * Copyright (c) 2016  Aisin AW, Ltd
 *
 * This program is licensed under GPL version 2 license.
 * See the LICENSE file distributed with this source file.
 */

/*-----------------------------------------------------------------------------------------------*/
/* File：RG_GuideControl.c                                                                       */
/* Info：誘導制御メイン                                                                          */
/*-----------------------------------------------------------------------------------------------*/

#include "sms-core/SMCoreDM/SMCoreDMInternal.h"

typedef struct _tts_text_tbl 
{
	INT32	code;
	char	*text;
} t_tts_text_tbl;

t_tts_text_tbl g_TTS_TEXT_TBL[] = 
{
{	VOICE_TEXT_301	,	VOICE_TEXT2_301	}	,
{	VOICE_TEXT_302	,	VOICE_TEXT2_302	}	,
{	VOICE_TEXT_303	,	VOICE_TEXT2_303	}	,
{	VOICE_TEXT_304	,	VOICE_TEXT2_304	}	,
{	VOICE_TEXT_305	,	VOICE_TEXT2_305	}	,
{	VOICE_TEXT_306	,	VOICE_TEXT2_306	}	,
{	VOICE_TEXT_JP_101	,	VOICE_TEXT2_JP_101	}	,
{	VOICE_TEXT_JP_102	,	VOICE_TEXT2_JP_102	}	,
{	VOICE_TEXT_JP_103	,	VOICE_TEXT2_JP_103	}	,
{	VOICE_TEXT_JP_104	,	VOICE_TEXT2_JP_104	}	,
{	VOICE_TEXT_JP_105	,	VOICE_TEXT2_JP_105	}	,
{	VOICE_TEXT_JP_106	,	VOICE_TEXT2_JP_106	}	,
{	VOICE_TEXT_JP_201	,	VOICE_TEXT2_JP_201	}	,
{	VOICE_TEXT_JP_202	,	VOICE_TEXT2_JP_202	}	,
{	VOICE_TEXT_JP_203	,	VOICE_TEXT2_JP_203	}	,
{	VOICE_TEXT_JP_204	,	VOICE_TEXT2_JP_204	}	,
{	VOICE_TEXT_JP_205	,	VOICE_TEXT2_JP_205	}	,
{	VOICE_TEXT_JP_206	,	VOICE_TEXT2_JP_206	}	,
{	VOICE_TEXT_JP_207	,	VOICE_TEXT2_JP_207	}	,
{	VOICE_TEXT_JP_208	,	VOICE_TEXT2_JP_208	}	,
{	VOICE_TEXT_JP_209	,	VOICE_TEXT2_JP_209	}	,
{	VOICE_TEXT_JP_210	,	VOICE_TEXT2_JP_210	}	,
{	VOICE_TEXT_JP_211	,	VOICE_TEXT2_JP_211	}	,
{	VOICE_TEXT_JP_212	,	VOICE_TEXT2_JP_212	}	,
{	VOICE_TEXT_JP_213	,	VOICE_TEXT2_JP_213	}	,
{	VOICE_TEXT_JP_214	,	VOICE_TEXT2_JP_214	}	,
{	VOICE_TEXT_JP_215	,	VOICE_TEXT2_JP_215	}	,
{	VOICE_TEXT_JP_216	,	VOICE_TEXT2_JP_216	}	,
{	VOICE_TEXT_JP_217	,	VOICE_TEXT2_JP_217	}	,
{	VOICE_TEXT_JP_218	,	VOICE_TEXT2_JP_218	}	,
{	VOICE_TEXT_JP_219	,	VOICE_TEXT2_JP_219	}	,
{	VOICE_TEXT_JP_220	,	VOICE_TEXT2_JP_220	}	,
{	VOICE_TEXT_JP_221	,	VOICE_TEXT2_JP_221	}	,
{	VOICE_TEXT_JP_401	,	VOICE_TEXT2_JP_401	}	,
{	VOICE_TEXT_JP_402	,	VOICE_TEXT2_JP_402	}	,
{	VOICE_TEXT_JP_403	,	VOICE_TEXT2_JP_403	}	,
{	VOICE_TEXT_JP_404	,	VOICE_TEXT2_JP_404	}	,
{	VOICE_TEXT_JP_405	,	VOICE_TEXT2_JP_405	}	,
{	VOICE_TEXT_JP_406	,	VOICE_TEXT2_JP_406	}	,
{	VOICE_TEXT_JP_407	,	VOICE_TEXT2_JP_407	}	,
{	VOICE_TEXT_JP_408	,	VOICE_TEXT2_JP_408	}	,
{	VOICE_TEXT_JP_409	,	VOICE_TEXT2_JP_409	}	,
{	VOICE_TEXT_JP_410	,	VOICE_TEXT2_JP_410	}	,
{	VOICE_TEXT_JP_411	,	VOICE_TEXT2_JP_411	}	,
{	VOICE_TEXT_JP_412	,	VOICE_TEXT2_JP_412	}	,
{	VOICE_TEXT_JP_413	,	VOICE_TEXT2_JP_413	}	,
{	VOICE_TEXT_JP_414	,	VOICE_TEXT2_JP_414	}	,
{	VOICE_TEXT_JP_415	,	VOICE_TEXT2_JP_415	}	,
{	VOICE_TEXT_JP_416	,	VOICE_TEXT2_JP_416	}	,
{	VOICE_TEXT_JP_417	,	VOICE_TEXT2_JP_417	}	,
{	VOICE_TEXT_JP_418	,	VOICE_TEXT2_JP_418	}	,
{	VOICE_TEXT_JP_419	,	VOICE_TEXT2_JP_419	}	,
{	VOICE_TEXT_JP_420	,	VOICE_TEXT2_JP_420	}	,
{	VOICE_TEXT_JP_501	,	VOICE_TEXT2_JP_501	}	,
{	VOICE_TEXT_JP_502	,	VOICE_TEXT2_JP_502	}	,
{	VOICE_TEXT_JP_503	,	VOICE_TEXT2_JP_503	}	,
{	VOICE_TEXT_JP_504	,	VOICE_TEXT2_JP_504	}	,
{	VOICE_TEXT_JP_505	,	VOICE_TEXT2_JP_505	}	,
{	VOICE_TEXT_JP_506	,	VOICE_TEXT2_JP_506	}	,
{	VOICE_TEXT_JP_507	,	VOICE_TEXT2_JP_507	}	,
{	VOICE_TEXT_JP_508	,	VOICE_TEXT2_JP_508	}	,
{	VOICE_TEXT_JP_509	,	VOICE_TEXT2_JP_509	}	,
{	VOICE_TEXT_JP_510	,	VOICE_TEXT2_JP_510	}	,
{	VOICE_TEXT_JP_511	,	VOICE_TEXT2_JP_511	}	,
{	VOICE_TEXT_JP_512	,	VOICE_TEXT2_JP_512	}	,
{	VOICE_TEXT_JP_513	,	VOICE_TEXT2_JP_513	}	,
{	VOICE_TEXT_JP_514	,	VOICE_TEXT2_JP_514	}	,
{	VOICE_TEXT_JP_515	,	VOICE_TEXT2_JP_515	}	,
{	VOICE_TEXT_EN_101	,	VOICE_TEXT2_EN_101	}	,
{	VOICE_TEXT_EN_102	,	VOICE_TEXT2_EN_102	}	,
{	VOICE_TEXT_EN_103	,	VOICE_TEXT2_EN_103	}	,
{	VOICE_TEXT_EN_104	,	VOICE_TEXT2_EN_104	}	,
{	VOICE_TEXT_EN_105	,	VOICE_TEXT2_EN_105	}	,
{	VOICE_TEXT_EN_106	,	VOICE_TEXT2_EN_106	}	,
{	VOICE_TEXT_EN_107	,	VOICE_TEXT2_EN_107	}	,
{	VOICE_TEXT_EN_201	,	VOICE_TEXT2_EN_201	}	,
{	VOICE_TEXT_EN_202	,	VOICE_TEXT2_EN_202	}	,
{	VOICE_TEXT_EN_203	,	VOICE_TEXT2_EN_203	}	,
{	VOICE_TEXT_EN_204	,	VOICE_TEXT2_EN_204	}	,
{	VOICE_TEXT_EN_205	,	VOICE_TEXT2_EN_205	}	,
{	VOICE_TEXT_EN_206	,	VOICE_TEXT2_EN_206	}	,
{	VOICE_TEXT_EN_207	,	VOICE_TEXT2_EN_207	}	,
{	VOICE_TEXT_EN_208	,	VOICE_TEXT2_EN_208	}	,
{	VOICE_TEXT_EN_209	,	VOICE_TEXT2_EN_209	}	,
{	VOICE_TEXT_EN_210	,	VOICE_TEXT2_EN_210	}	,
{	VOICE_TEXT_EN_211	,	VOICE_TEXT2_EN_211	}	,
{	VOICE_TEXT_EN_212	,	VOICE_TEXT2_EN_212	}	,
{	VOICE_TEXT_EN_213	,	VOICE_TEXT2_EN_213	}	,
{	VOICE_TEXT_EN_214	,	VOICE_TEXT2_EN_214	}	,
{	VOICE_TEXT_EN_215	,	VOICE_TEXT2_EN_215	}	,
{	VOICE_TEXT_EN_216	,	VOICE_TEXT2_EN_216	}	,
{	VOICE_TEXT_EN_217	,	VOICE_TEXT2_EN_217	}	,
{	VOICE_TEXT_EN_218	,	VOICE_TEXT2_EN_218	}	,
{	VOICE_TEXT_EN_219	,	VOICE_TEXT2_EN_219	}	,
{	VOICE_TEXT_EN_220	,	VOICE_TEXT2_EN_220	}	,
{	VOICE_TEXT_EN_221	,	VOICE_TEXT2_EN_221	}	,
{	VOICE_TEXT_EN_401	,	VOICE_TEXT2_EN_401	}	,
{	VOICE_TEXT_EN_402	,	VOICE_TEXT2_EN_402	}	,
{	VOICE_TEXT_EN_403	,	VOICE_TEXT2_EN_403	}	,
{	VOICE_TEXT_EN_404	,	VOICE_TEXT2_EN_404	}	,
{	VOICE_TEXT_EN_405	,	VOICE_TEXT2_EN_405	}	,
{	VOICE_TEXT_EN_406	,	VOICE_TEXT2_EN_406	}	,
{	VOICE_TEXT_EN_407	,	VOICE_TEXT2_EN_407	}	,
{	VOICE_TEXT_EN_408	,	VOICE_TEXT2_EN_408	}	,
{	VOICE_TEXT_EN_409	,	VOICE_TEXT2_EN_409	}	,
{	VOICE_TEXT_EN_410	,	VOICE_TEXT2_EN_410	}	,
{	VOICE_TEXT_EN_411	,	VOICE_TEXT2_EN_411	}	,
{	VOICE_TEXT_EN_412	,	VOICE_TEXT2_EN_412	}	,
{	VOICE_TEXT_EN_413	,	VOICE_TEXT2_EN_413	}	,
{	VOICE_TEXT_EN_414	,	VOICE_TEXT2_EN_414	}	,
{	VOICE_TEXT_EN_415	,	VOICE_TEXT2_EN_415	}	,
{	VOICE_TEXT_EN_416	,	VOICE_TEXT2_EN_416	}	,
{	VOICE_TEXT_EN_501	,	VOICE_TEXT2_EN_501	}	,
{	VOICE_TEXT_EN_502	,	VOICE_TEXT2_EN_502	}	,
{	VOICE_TEXT_EN_503	,	VOICE_TEXT2_EN_503	}	,
{	VOICE_TEXT_EN_504	,	VOICE_TEXT2_EN_504	}	,
{	VOICE_TEXT_EN_505	,	VOICE_TEXT2_EN_505	}	,
{	VOICE_TEXT_EN_506	,	VOICE_TEXT2_EN_506	}	,
{	VOICE_TEXT_EN_507	,	VOICE_TEXT2_EN_507	}	,
{	VOICE_TEXT_EN_508	,	VOICE_TEXT2_EN_508	}	,
{	VOICE_TEXT_EN_509	,	VOICE_TEXT2_EN_509	}	,
{	VOICE_TEXT_EN_510	,	VOICE_TEXT2_EN_510	}	,
{	VOICE_TEXT_EN_511	,	VOICE_TEXT2_EN_511	}	,
};

#define TTSMAX	(2048)
/**
 * @brief	TTS文字列作成
 */
E_SC_RESULT RG_CTL_CreateVoiceText(RT_NAME_t *in, INT32 language)
{
	int i = 0,j = 0;
	int code,num;
	int len = 0;
	char tts_voice[TTSMAX];
	
	memset(tts_voice,0,TTSMAX);
	
	if (language == SYS_LANGUAGE_EN)
	{
		strncat(tts_voice, "flite \"", (TTSMAX - len - 1));
	}
	else
	{
		strncat(tts_voice, "jtalk \"", (TTSMAX - len - 1));
	}
	
	while(1)
	{
		len = strlen(tts_voice);
		if (len > (TTSMAX - 10)) break;
		
		code = in->voice.voice_list[j];
		
		if (code == 0) break;
		j++;
		
		num = -1;
		for(i=0;i < (sizeof(g_TTS_TEXT_TBL)/sizeof(t_tts_text_tbl));i++)
		{
			if (g_TTS_TEXT_TBL[i].code == code)
			{
				num = i;
				break;
			}
		}
		
		if (num >= 0)
		{
			strncat(tts_voice,g_TTS_TEXT_TBL[num].text, (TTSMAX - len - 1));
		}
	}
	
	len = strlen(tts_voice);
	
	if (len > (TTSMAX - 10))
	{
		//TTSバッファが限界なので再生しない
	}
	else
	{
		strncat(tts_voice, "\" & ", (TTSMAX - len - 1));
		
		system(tts_voice);
	}
	
	return (e_SC_RESULT_SUCCESS);
}

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

	if (language != SYS_LANGUAGE_JP)
	{
		// default language : US
		language = SYS_LANGUAGE_EN;
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
		RG_CTL_CreateVoiceText(&new_p->voice.tts, language);
		SC_LOG_InfoPrint(SC_TAG_RG, "[音声発話♪]");
	}

	SC_LOG_DebugPrint(SC_TAG_RG, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}


