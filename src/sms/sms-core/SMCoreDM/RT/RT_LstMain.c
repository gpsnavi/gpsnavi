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
/* File：RT_TblMain.c                                                                            */
/* Info：誘導テーブル作成制御                                                                    */
/*-----------------------------------------------------------------------------------------------*/

#include "sms-core/SMCoreDM/SMCoreDMInternal.h"

// 誘導テーブル
static RT_LST_MAIN_t	RT_G_TurnList;

static E_SC_RESULT 	RT_LST_InitTurnList(RT_LST_MAIN_t *);

/**
 * @brief	ターンリスト作成メイン
 * @param	[I]なし
 */
E_SC_RESULT RT_LST_MakeTurnList()
{
	E_SC_RESULT			ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT			jdg = e_SC_RESULT_SUCCESS;
	RT_LST_MAIN_t		*turnlist_p;
	RT_TBL_MAIN_t		*guidetbl_p;
	UINT32				rt_id;
	UINT32				rt_mode;

	SC_LOG_DebugPrint(SC_TAG_RT, SC_LOG_START);

	// ターンリスト情報取得
	turnlist_p = RT_LST_GetTurnList();
	if (NULL == turnlist_p) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[LST] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 確定推奨経路ＩＤ取得
	ret = SC_RP_GetCurrentRouteId(&rt_id, &rt_mode);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 経路IDで作成済み判定
	if ((rt_id == turnlist_p->rt_id.id) && (rt_mode == turnlist_p->rt_id.mode)) {
		SC_LOG_DebugPrint(SC_TAG_RT, "[TBL] TURNLIST ALREADY EXISTS !! " HERE);
		return (e_SC_RESULT_SUCCESS);
	}

	// ターンリスト情報初期化
	ret = RT_LST_InitTurnList(turnlist_p);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[LST] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 誘導テーブル作成
	ret = RT_TBL_MakeGuideTbl(MAKETBL_ALL);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_DebugPrint(SC_TAG_RT, "[call] RT_GuideTblMake => FAIL !!"HERE);
		return (e_SC_RESULT_FAIL);
	}

	do {
		// 誘導テーブル参照開始
		guidetbl_p = RT_TBL_LockGuideTbl();
		if (NULL == guidetbl_p) {
			SC_LOG_ErrorPrint(SC_TAG_RT, "[LST] ERROR " HERE);
			jdg = e_SC_RESULT_FAIL;
			break;
		}

		// ターンリスト情報設定
		ret = RT_LST_SetTurnList(guidetbl_p, turnlist_p);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_DebugPrint(SC_TAG_RT, "[call] RT_GuideTblMake => FAIL !!"HERE);
			jdg = e_SC_RESULT_FAIL;
		}

		// 誘導テーブル参照終了
		ret = RT_TBL_UnLockGuideTbl();
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_RG, "[LST] ERROR " HERE);
			jdg = e_SC_RESULT_FAIL;
		}

	} while(0);

	// 誘導テーブル解放
	ret = RT_TBL_FreeGuideTbl();
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[LST] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	SC_LOG_DebugPrint(SC_TAG_RT, SC_LOG_END);

	return (jdg);
}

/**
 * @brief	ターンリスト情報取得
 * @param	[I]なし
 */
RT_LST_MAIN_t *RT_LST_GetTurnList()
{
	return (&RT_G_TurnList);
}

/**
 * @brief	ターンリスト情報初期化
 * @param	[I]ターンリスト情報
 */
static E_SC_RESULT RT_LST_InitTurnList(RT_LST_MAIN_t *turnlist_p)
{
	if (NULL == turnlist_p) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 初期化 TODO
	memset(turnlist_p, 0x00, sizeof(RT_LST_MAIN_t));
	memset(&turnlist_p->rt_id, 0xFF, sizeof(RT_ROUTEID_t));

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief	ターンリスト情報初期化
 * @param	[I]ターンリスト情報
 */
E_SC_RESULT RT_LST_Init()
{

	E_SC_RESULT			ret = e_SC_RESULT_SUCCESS;

	// ターンリスト情報初期化
	ret = RT_LST_InitTurnList(&RT_G_TurnList);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[LST] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	return (e_SC_RESULT_SUCCESS);
}

