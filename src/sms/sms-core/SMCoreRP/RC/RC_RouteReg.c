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
 * RP_RouteReg.c
 *
 *  Created on: 2015/11/06
 *      Author: masutani
 */

#include "sms-core/SMCoreRP/SMCoreRPInternal.h"

#define RC_REG_INFO_SIZE				5		/* 規制情報数サイズ */
#define RC_REG_LINK_SIZE				11		/* 規制リンクリストサイズ */

/*-------------------------------------------------------------------
 * 型定義
 *-------------------------------------------------------------------*/
typedef UINT16 (*F_RegJudge)(SCRP_NETCONTROLER*, SCRC_IOCALCTBL*);

// 2015/07/31 ライカムパッチ
typedef struct {
	UINT32 parcelId;					// パーセルID
	UINT32 linkId;						// 規制リンクID
} ROUTE_REG_LINK;

typedef struct {
	UINT32 listIdx;						// リンクリストIndex
	UINT32 linkVol;						// リンク数
} ROUTE_REG_INFO;

typedef struct {
	ROUTE_REG_INFO regInfo[RC_REG_INFO_SIZE];		// 規制情報
	UINT32 regInfoVol;								// 規制情報数
	ROUTE_REG_LINK linkList[RC_REG_LINK_SIZE];		// 規制リンクリスト
	UINT32 linkListVol;								// 規制リンクリスト数
} ROUTE_PATCH_LIST;

static ROUTE_PATCH_LIST mRaikamPatch = {};

/*-------------------------------------------------------------------
 * 宣言
 *-------------------------------------------------------------------*/
static UINT16 RC_JudgeTimeReg_Dummy(SCRP_NETCONTROLER* aNetTab, SCRC_IOCALCTBL* aInoutData);
static UINT16 RC_JudgeTimeReg_Use(SCRP_NETCONTROLER* aNetTab, SCRC_IOCALCTBL* aInoutData);
static UINT16 RC_JudgeTimeReg_Unuse(SCRP_NETCONTROLER* aNetTab, SCRC_IOCALCTBL* aInoutData);
static UINT16 RC_JudgeTimeReg_Ignore(SCRP_NETCONTROLER* aNetTab, SCRC_IOCALCTBL* aInoutData);
static UINT16 RC_JudgeSeasonReg_Dummy(SCRP_NETCONTROLER* aNetTab, SCRC_IOCALCTBL* aInoutData);
static UINT16 RC_JudgeSeasonReg_Use(SCRP_NETCONTROLER* aNetTab, SCRC_IOCALCTBL* aInoutData);
static UINT16 RC_JudgeSeasonReg_Unuse(SCRP_NETCONTROLER* aNetTab, SCRC_IOCALCTBL* aInoutData);
static UINT16 RC_JudgeSeasonReg_Ignore(SCRP_NETCONTROLER* aNetTab, SCRC_IOCALCTBL* aInoutData);

// 2015/07/31 ライカムパッチ
typedef Bool (*F_PatchRegJudge)(SCRP_NETCONTROLER* aNetTab, SCRC_TARGETLINKINFO *aInLink, SCRC_TARGETLINKINFO *aOutLink, ROUTE_REG_INFO *aRegInfo);
static F_PatchRegJudge mPatchReg = NULL;
static void setPatchRegRaikam();
static Bool isPatchRegRaikam(SCRP_NETCONTROLER* aNetTab, SCRC_TARGETLINKINFO *aInLink, SCRC_TARGETLINKINFO *aOutLink, ROUTE_REG_INFO *aRegInfo);

static F_RegJudge m_TimeRegJudge;
static F_RegJudge m_SeasonRegJudge;

/**
 * @brief	規制判定用関数設定
 * @param	[I]探索設定情報
 */
E_SC_RESULT RC_RegJudgeFuncSet(SCRP_SEARCHSETTING* aSetting) {
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	// 2015/07/31 ライカムパッチ
	setPatchRegRaikam();

	//時間規制
	switch (aSetting->b_setting.seasonReg) {
	case RP_SETTING_TIMEREG_ON:
		m_TimeRegJudge = RC_JudgeTimeReg_Use;
		break;
	case RP_SETTING_TIMEREG_OFF:
		m_TimeRegJudge = RC_JudgeTimeReg_Unuse;
		break;
	case RP_SETTING_TIMEREG_AVOID:
		m_TimeRegJudge = RC_JudgeTimeReg_Ignore;
		break;
	default:
		// ダミーを格納
		m_TimeRegJudge = RC_JudgeTimeReg_Dummy;
		SC_LOG_ErrorPrint(SC_TAG_RC, "[Reg] Unknown TimeRegSetting"HERE);
		SC_LOG_DebugPrint(SC_TAG_RC, "[Reg] Unknown TimeRegSetting setting[0x%04x]", (aSetting->b_setting.timeReg));
		return (e_SC_RESULT_BADPARAM);
	}
	//季節規制
	switch (aSetting->b_setting.seasonReg) {
	case RP_SETTING_SEASONREG_ON:
		m_SeasonRegJudge = RC_JudgeSeasonReg_Use;
		break;
	case RP_SETTING_SEASONREG_OFF:
		m_SeasonRegJudge = RC_JudgeSeasonReg_Unuse;
		break;
	case RP_SETTING_SEASONREG_AVOID:
		m_SeasonRegJudge = RC_JudgeSeasonReg_Ignore;
		break;
	default:
		// ダミーを格納
		m_SeasonRegJudge = RC_JudgeSeasonReg_Dummy;
		SC_LOG_ErrorPrint(SC_TAG_RC, "[Reg] Unknown SeasonRegSetting"HERE);
		SC_LOG_DebugPrint(SC_TAG_RC, "[Reg] Unknown SeasonRegSetting setting[0x%04x]", (aSetting->b_setting.seasonReg));
		return (e_SC_RESULT_BADPARAM);
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

UINT16 RC_JudgeTimeReg(SCRP_NETCONTROLER* aNetTab, SCRC_IOCALCTBL* aInoutData) {
	return (m_TimeRegJudge(aNetTab, aInoutData));
}
UINT16 RC_JudgeSeasonReg(SCRP_NETCONTROLER* aNetTab, SCRC_IOCALCTBL* aInoutData) {
	return (m_SeasonRegJudge(aNetTab, aInoutData));
}
static UINT16 RC_JudgeTimeReg_Dummy(SCRP_NETCONTROLER* aNetTab, SCRC_IOCALCTBL* aInoutData) {
	return (0);
}
static UINT16 RC_JudgeTimeReg_Use(SCRP_NETCONTROLER* aNetTab, SCRC_IOCALCTBL* aInoutData) {
	return (0);
}
static UINT16 RC_JudgeTimeReg_Unuse(SCRP_NETCONTROLER* aNetTab, SCRC_IOCALCTBL* aInoutData) {
	return (0);
}
static UINT16 RC_JudgeTimeReg_Ignore(SCRP_NETCONTROLER* aNetTab, SCRC_IOCALCTBL* aInoutData) {
	return (0);
}
static UINT16 RC_JudgeSeasonReg_Dummy(SCRP_NETCONTROLER* aNetTab, SCRC_IOCALCTBL* aInoutData) {
	return (0);
}
static UINT16 RC_JudgeSeasonReg_Use(SCRP_NETCONTROLER* aNetTab, SCRC_IOCALCTBL* aInoutData) {
	return (0);
}
static UINT16 RC_JudgeSeasonReg_Unuse(SCRP_NETCONTROLER* aNetTab, SCRC_IOCALCTBL* aInoutData) {
	return (0);
}
static UINT16 RC_JudgeSeasonReg_Ignore(SCRP_NETCONTROLER* aNetTab, SCRC_IOCALCTBL* aInoutData) {
	return (0);
}

/**
 * ライカムパッチとなるか判定
 * @return true:該当する
 *         false:該当しない
 * @memo 2015/07/31 ライカムパッチ
 * @memo 性能確保のため一部パラメタチェックを無効化 2016/01/08
 */
Bool RC_JudgePatchReg(SCRP_NETCONTROLER* aNetTab, SCRC_TARGETLINKINFO *aInLink, SCRC_TARGETLINKINFO *aOutLink) {

	if (NULL == mPatchReg) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[Reg] bad param"HERE);
		return (false);
	}
#if 0 // 2016/01/08
	if (NULL == aNetTab || NULL == aInLink || NULL == aOutLink) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[Reg] bad param"HERE);
		return (false);
	}
#endif

	Bool ret = false;
	UINT32 i;
	for (i = 0; i < mRaikamPatch.regInfoVol; i++) {
		if (mPatchReg(aNetTab, aInLink, aOutLink, &mRaikamPatch.regInfo[i])) {
			ret = true;
			break;
		}
	}
	return (ret);
}

/**
 * ライカムパッチ規制判定
 * @param aNetTab
 * @param aInLink
 * @param aOutLink
 * @param aRegInfo
 * @return true対象規制
 * @memo 2015/07/31 ライカムパッチ
 *       性能を考慮しパラメタチェックは無し。(RC_JudgePatchRegで実行済み)
 */
static Bool isPatchRegRaikam(SCRP_NETCONTROLER* aNetTab, SCRC_TARGETLINKINFO *aInLink, SCRC_TARGETLINKINFO *aOutLink, ROUTE_REG_INFO *aRegInfo) {
	Bool ret = false;
	void* bin = NULL;
	UINT32 linkId = 0;

	do {
		ROUTE_REG_LINK* linkList = &mRaikamPatch.linkList[aRegInfo->listIdx];

		// out 判定
		if (aOutLink->pclInfo->parcelId != linkList[0].parcelId) {
			break;
		}
		bin = SC_MA_A_NWRCD_LINK_GET_RECORD(aOutLink->pclInfo->mapNetworkLinkBin, aOutLink->linkTable->detaIndex);
		linkId = SC_MA_D_NWRCD_LINK_GET_ID(bin);
		if (linkId != linkList[0].linkId) {
			break;
		}

		// in 判定
		if (aInLink->pclInfo->parcelId != linkList[1].parcelId) {
			break;
		}
		bin = SC_MA_A_NWRCD_LINK_GET_RECORD(aInLink->pclInfo->mapNetworkLinkBin, aInLink->linkTable->detaIndex);
		linkId = SC_MA_D_NWRCD_LINK_GET_ID(bin);
		if (linkId != linkList[1].linkId) {
			break;
		}

		// preリンク判定
		SCRP_NETDATA* wkLink = aInLink->linkNet;
		UINT32 i;
		for (i = 2; i < aRegInfo->linkVol; i++) {
			if (ALL_F32 == wkLink->inLinkHist) {
				break;
			}
			SCRP_LINKINFO* preLink = RCNET_GET_HISTLINKINFO(aNetTab, wkLink);
			SCRP_PCLINFO* preInfo = RCNET_GET_HISTPCLINFO(aNetTab, wkLink);
			UINT32 or = RCNET_GET_HISTORIDX(wkLink);

			if (preInfo->parcelId != linkList[i].parcelId) {
				break;
			}
			bin = SC_MA_A_NWRCD_LINK_GET_RECORD(preInfo->mapNetworkLinkBin, preLink->detaIndex);
			linkId = SC_MA_D_NWRCD_LINK_GET_ID(bin);
			if (linkId != linkList[i].linkId) {
				break;
			}
			wkLink = &preLink->linkNet[or];
		}
		if (i < aRegInfo->linkVol) {
			break;
		}
		ret = true;
	} while (0);

	return (ret);
}

/**
 * ライカムパッチリンク情報設定
 * @memo 2015/07/31 ライカムパッチ
 */
static void setPatchRegRaikam() {
	UINT16 info = 0;
	UINT16 idx = 0;

	mRaikamPatch.regInfo[info].listIdx = idx;
	mRaikamPatch.regInfo[info++].linkVol = 3;
	mRaikamPatch.linkList[idx].parcelId = 0x13bf0ded;
	mRaikamPatch.linkList[idx++].linkId = 0x28000189;
	mRaikamPatch.linkList[idx].parcelId = 0x13bf0ded;
	mRaikamPatch.linkList[idx++].linkId = 0x2800007c;
	mRaikamPatch.linkList[idx].parcelId = 0x13bf0ded;
	mRaikamPatch.linkList[idx++].linkId = 0x2800007d;

	mRaikamPatch.regInfo[info].listIdx = idx;
	mRaikamPatch.regInfo[info++].linkVol = 2;
	mRaikamPatch.linkList[idx].parcelId = 0x13bf0ded;
	mRaikamPatch.linkList[idx++].linkId = 0x2800018a;
	mRaikamPatch.linkList[idx].parcelId = 0x13bf0ded;
	mRaikamPatch.linkList[idx++].linkId = 0x28000190;

	mRaikamPatch.regInfo[info].listIdx = idx;
	mRaikamPatch.regInfo[info++].linkVol = 2;
	mRaikamPatch.linkList[idx].parcelId = 0x13bf0ded;
	mRaikamPatch.linkList[idx++].linkId = 0x280001db;
	mRaikamPatch.linkList[idx].parcelId = 0x13bf0ded;
	mRaikamPatch.linkList[idx++].linkId = 0x280002ad;

	mRaikamPatch.regInfo[info].listIdx = idx;
	mRaikamPatch.regInfo[info++].linkVol = 2;
	mRaikamPatch.linkList[idx].parcelId = 0x13bf0ded;
	mRaikamPatch.linkList[idx++].linkId = 0x28000429;
	mRaikamPatch.linkList[idx].parcelId = 0x13bf0ded;
	mRaikamPatch.linkList[idx++].linkId = 0x2800043c;

	mRaikamPatch.regInfo[info].listIdx = idx;
	mRaikamPatch.regInfo[info++].linkVol = 2;
	mRaikamPatch.linkList[idx].parcelId = 0x13bf0ded;
	mRaikamPatch.linkList[idx++].linkId = 0x28000440;
	mRaikamPatch.linkList[idx].parcelId = 0x13bf0ded;
	mRaikamPatch.linkList[idx++].linkId = 0x28000432;

	mRaikamPatch.regInfoVol = info;
	mRaikamPatch.linkListVol = idx;

	// パッチ関数ポインタ
	mPatchReg = isPatchRegRaikam;
}
