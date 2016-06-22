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
/* File：RT_LstMake.c                                                                            */
/* Info：誘導テーブル作成                                                                        */
/*-----------------------------------------------------------------------------------------------*/

#include "sms-core/SMCoreDM/SMCoreDMInternal.h"

static INT32 RT_LST_GetTurnDir(RT_TBL_GUIDECRS_t *);
static INT32 RT_LST_GetIconType(RT_TBL_GUIDECRS_t *);

/**
 * @brief	誘導テーブル作成メイン
 * @param	[I]なし
 */
E_SC_RESULT RT_LST_SetTurnList(RT_TBL_MAIN_t *guidetbl_p, RT_LST_MAIN_t *turnlist_p)
{
	E_SC_RESULT				ret = e_SC_RESULT_SUCCESS;
	RT_TBL_ROUTELINK_t		*route_link_p;
	RT_TBL_GUIDELINK_t		*guide_link_p;
	RT_TBL_GUIDECRS_t		*guide_crs_p;
	RT_XY_t					*guide_sharp_p;
	UINT32					total_dist;
	UINT32					point_no;
	UINT16					ilp,jlp,cnt = 0;
	DOUBLE					x,y;
	DOUBLE					latitude;
	DOUBLE					longitude;

	SC_LOG_DebugPrint(SC_TAG_RT, SC_LOG_START);

	if (NULL == guidetbl_p || NULL == turnlist_p) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[LST] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 最寄経路リンク情報アドレス取得
	route_link_p = guidetbl_p->route_link_p;

	// 最寄リンク終点までのリンク長設定
	total_dist = 0;

	// 出発地リスト登録
	turnlist_p->turn_info[ 0 ].linkId        = ALLF32;			// 進入リンクID
	turnlist_p->turn_info[ 0 ].roadDir       = 5;				// 案内方向
	turnlist_p->turn_info[ 0 ].turnListIcon  = LIST_ICON_START;	// アイコン種別
	turnlist_p->turn_info[ 0 ].roadSituation = STYPE_START;		// 案内種別

	// 交差点緯度経度
	x = (DOUBLE)guidetbl_p->guide_sharp_p->x;
	y = (DOUBLE)guidetbl_p->guide_sharp_p->y;

	if(0 != SC_MESH_ChgParcelIDToTitude(1, guidetbl_p->guide_link_p->parcel_id, x, y, &latitude, &longitude)){
		SC_LOG_ErrorPrint(SC_TAG_RT, "[LST] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	turnlist_p->turn_info[ 0 ].rdPos.latitude  = (INT32)(latitude * 1024);
	turnlist_p->turn_info[ 0 ].rdPos.longitude = (INT32)(longitude * 1024);

	// リスト数カウントアップ
	cnt = 1;

	// 経路リンク数分ループ
	for (ilp = 0; ilp < guidetbl_p->route_link_vol ; ilp++, route_link_p++) {

		// 対応する誘導リンク情報が作成されていない
		if (ALLF32 == route_link_p->link_offset) {
			break;
		}

		// 経路リンク構成リンク情報アドレス取得
		guide_link_p = (guidetbl_p->guide_link_p + route_link_p->link_offset);

		// 経路リンク構成リンク情報数分ループ
		for (jlp = 0 ; jlp < route_link_p->link_vol ; jlp++, guide_link_p++) {

			// リンク長加算
			total_dist += guide_link_p->dist;

			// 交差点情報なし
			if (ALLF16 == guide_link_p->crs_offset) {
				continue;
			}

			// 誘導交差点情報アドレス取得
			guide_crs_p = (guidetbl_p->guide_crs_p + guide_link_p->crs_offset);

			// 非誘導交差点
			if (CRSTYPE_NOTCRS == guide_crs_p->crs_type) {
				continue;
			}

			turnlist_p->turn_info[ cnt ].roadType      = (INT32)route_link_p->road_class;			// 進入道路種別
			turnlist_p->turn_info[ cnt ].linkId        = guide_link_p->link_id;						// 進入リンクID
			turnlist_p->turn_info[ cnt ].lengthToStart = total_dist;								// 出発地からの距離
			turnlist_p->turn_info[ cnt ].roadDir       = RT_LST_GetTurnDir(guide_crs_p);			// 案内方向
			turnlist_p->turn_info[ cnt ].turnListIcon  = RT_LST_GetIconType(guide_crs_p);			// アイコン種別
			turnlist_p->turn_info[ cnt ].roadSituation = RT_TBL_GetSituationType(guide_crs_p->crs_type);// 案内種別

			// アイコン種別が経由地の場合
			if (LIST_ICON_WAYPT == turnlist_p->turn_info[ cnt ].turnListIcon) {
				point_no = (guidetbl_p->route_point_p + route_link_p->route_point_no)->sect_no + 1;
				turnlist_p->turn_info[ cnt ].byPassIndex = (BYTE)point_no;							// 経由地番号
			}

			// 交差点名称ありの場合
			if (guide_crs_p->crs_name.len > 0) {
				strcpy(&(turnlist_p->turn_info[ cnt ].name[0]), &(guide_crs_p->crs_name.name[0]));	// 誘導地名称
			}

			// 誘導リンク形状点情報の当該リンクの端点情報取得
			guide_sharp_p  = (guidetbl_p->guide_sharp_p + guide_link_p->sharp_offset);
			guide_sharp_p += (guide_link_p->sharp_vol - 1);

			// 交差点緯度経度
			x = (DOUBLE)guide_sharp_p->x;
			y = (DOUBLE)guide_sharp_p->y;

			if(0 != SC_MESH_ChgParcelIDToTitude(1, guide_link_p->parcel_id, x, y, &latitude, &longitude)){
				SC_LOG_ErrorPrint(SC_TAG_RT, "[LST] ERROR " HERE);
				return (e_SC_RESULT_FAIL);
			}

			turnlist_p->turn_info[ cnt ].rdPos.latitude  = (INT32)(latitude * 1024);
			turnlist_p->turn_info[ cnt ].rdPos.longitude = (INT32)(longitude * 1024);

			// リスト数カウントアップ
			cnt++;
		}
	}

	// ターンリスト数設定
	turnlist_p->turn_vol = cnt;
	// 経路ＩＤ情報設定
	turnlist_p->rt_id = guidetbl_p->rt_id;

	// 共有メモリへターンリスト情報設定
	ret = RG_CTL_SH_SetTurnList(turnlist_p);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[LST] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	SC_LOG_DebugPrint(SC_TAG_RT, SC_LOG_END);

	return (ret);

}

/**
 * @brief	案内方向取得
 * @param	[I]誘導テーブル交差点情報
 */
static INT32 RT_LST_GetTurnDir(RT_TBL_GUIDECRS_t *guide_crs_p)
{
	INT32	dir;

	if (NULL == guide_crs_p) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	dir = RG_TBL_GetRelativeGuideDir(guide_crs_p);

	switch (dir) {
	case TURN_ST:
	case TURN_UT:
	case TURN_FR:
	case TURN_R:
	case TURN_BR:
	case TURN_FL:
	case TURN_L:
	case TURN_BL:
	case TURN_RA:
		break;
	case TURN_RR:
		dir = TURN_FR;
		break;
	case TURN_RL:
		dir = TURN_FL;
		break;
	default:
		dir = TURN_ST;
	}

	return (dir);
}

/**
 * @brief	リストアイコン種別取得
 * @param	[I]誘導テーブル交差点情報
 */
static INT32 RT_LST_GetIconType(RT_TBL_GUIDECRS_t *guide_crs_p)
{
	INT32	type;

	if (NULL == guide_crs_p) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	switch (guide_crs_p->crs_type) {
	case CRSTYPE_NORMAL:
		switch (guide_crs_p->dir) {
		case TURN_ST:
			type = LIST_ICON_TURNST;
			break;
		case TURN_UT:
			type = LIST_ICON_TURNUT;
			break;
		case TURN_FR:
			type = LIST_ICON_TURNFR;
			break;
		case TURN_R:
			type = LIST_ICON_TURNR;
			break;
		case TURN_BR:
			type = LIST_ICON_TURNBR;
			break;
		case TURN_FL:
			type = LIST_ICON_TURNFL;
			break;
		case TURN_L:
			type = LIST_ICON_TURNL;
			break;
		case TURN_BL:
			type = LIST_ICON_TURNBL;
			break;
		default:
			type = LIST_ICON_TURNST;
		}
		break;

	case CRSTYPE_HWYIN:
		type = LIST_ICON_HWIN;
		break;

	case CRSTYPE_HWYOT:
		type = LIST_ICON_HWOUT;
		break;

	case CRSTYPE_HWYJCT:
		type = LIST_ICON_JCT;
		break;

	case CRSTYPE_WAYPT:
		type = LIST_ICON_WAYPT;
		break;

	case CRSTYPE_DEST:
		type = LIST_ICON_DEST;
		break;

	case CRSTYPE_SPLIT:
		type = LIST_ICON_TURNST;
		break;

	case CRSTYPE_RA:
		type = LIST_ICON_RA;
		break;

	default:
		type = LIST_ICON_TURNST;
	}
	return (type);
}
