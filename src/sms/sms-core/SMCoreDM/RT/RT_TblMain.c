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

// 実長ＸＹ
DOUBLE	RT_real_x = 0.0;			// 実長変換係数(x)
DOUBLE 	RT_real_y = 0.0;			// 実長変換係数(y)

// 角度→案内方向変換テーブル
UINT16	RT_TURNDIR_TBL[16] = {};

// 誘導情報テーブル
static RT_TBL_MGR_t	RT_G_TblMgr;

// ミューテックス
static SC_MUTEX			m_Mutex = SC_MUTEX_INITIALIZER;		// Mutex

static E_SC_RESULT RT_TBL_InitGuideTbl(RT_TBL_MAIN_t *);
static E_SC_RESULT RT_TBL_AllocGuideTbl(RT_TBL_MAIN_t *, SC_RP_RouteMng *);
static E_SC_RESULT RT_TBL_SetParam();

/**
 * @brief	誘導テーブル作成メイン
 * @param	[I]なし
 */
E_SC_RESULT RT_TBL_MakeGuideTbl(UINT32 make_dist)
{
	E_SC_RESULT			ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT			jdg = e_SC_RESULT_SUCCESS;
	RT_TBL_MAIN_t		*guidetbl_p = NULL;
	SC_RP_RouteMng		*mst_route_p;
	UINT32				rt_id;
	UINT32				rt_mode;
	UINT32				dist;
	INT16				ilp;
	clock_t 			clc[2] = {};

	SC_LOG_DebugPrint(SC_TAG_RT, SC_LOG_START);

	clc[0] = clock();

	// リージョン別パラメータ設定
	ret = RT_TBL_SetParam();
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 排他開始
	ret = SC_LockMutex(&m_Mutex);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	do {
		// 確定推奨経路ＩＤ取得
		ret = SC_RP_GetCurrentRouteId(&rt_id, &rt_mode);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
			jdg = e_SC_RESULT_FAIL;
			break;
		}

		// 推奨経路取得
		ret = SC_RP_ReadRouteEntry(rt_id, rt_mode, SC_RP_USER_RG, &mst_route_p);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
			jdg = e_SC_RESULT_FAIL;
			break;
		}

		do {
			// 推奨経路情報ポインタチェック
			if (NULL ==  mst_route_p->sectInfo || NULL == mst_route_p->parcelInfo ||
				NULL == mst_route_p->linkInfo  || NULL == mst_route_p->formInfo) {
				SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
				jdg = e_SC_RESULT_FAIL;
				break;
			}

			// 誘導テーブルが存在していた場合
			if (PUBLIC_INI != RT_G_TblMgr.public_no){
				SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
				jdg = e_SC_RESULT_FAIL;
				break;
			}

			// 誘導テーブルの初期化＆生成準備
			for (ilp = 0 ; ilp < PUBLIC_MAX ; ilp++) {
				// 誘導テーブル取得
				guidetbl_p = &(RT_G_TblMgr.guide_tbl[ilp]);

				// 誘導テーブル初期化
				ret = RT_TBL_InitGuideTbl(guidetbl_p);
				if (e_SC_RESULT_SUCCESS != ret) {
					SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
					jdg = e_SC_RESULT_FAIL;
					break;
				}

				// 誘導テーブル領域確保
				ret = RT_TBL_AllocGuideTbl(guidetbl_p, mst_route_p);
				if (e_SC_RESULT_SUCCESS != ret) {
					SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
					jdg = e_SC_RESULT_FAIL;
					break;
				}
			}
			if (e_SC_RESULT_FAIL == jdg) {
				break;
			}

			// 誘導テーブル公開番号をPUBLIC_NO1に設定
			RT_G_TblMgr.public_no = PUBLIC_NO1;

			// 誘導テーブル取得
			guidetbl_p = &(RT_G_TblMgr.guide_tbl[RT_G_TblMgr.public_no]);

			// 経路リンク情報設定
			ret = RT_TBL_SetRouteLink(guidetbl_p, mst_route_p);
			if (e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
				jdg = e_SC_RESULT_FAIL;
				break;
			}

			// 誘導リンク情報設定
			ret = RT_TBL_SetGuideLink(guidetbl_p, make_dist);
			if (e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
				jdg = e_SC_RESULT_FAIL;
				break;
			}

			// 経路ＩＤ情報設定
			guidetbl_p->rt_id.id   = rt_id;
			guidetbl_p->rt_id.mode = rt_mode;
			guidetbl_p->rt_id.type = mst_route_p->routeType;

		} while(0);

		// 推奨経路参照終了
		ret = SC_RP_ReadRouteExit(rt_id, rt_mode, SC_RP_USER_RG);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
			jdg = e_SC_RESULT_FAIL;
			break;
		}

	} while (0);

	// 排他終了
	ret = SC_UnLockMutex(&m_Mutex);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

#if 1
	if (NULL != guidetbl_p) {
		clc[1] = clock();
		dist  = (guidetbl_p->route_link_p + guidetbl_p->route_link_vol - 1)->total_dist;
		dist += (guidetbl_p->route_link_p + guidetbl_p->route_link_vol - 1)->dist;

		SC_LOG_InfoPrint(SC_TAG_RT, "-----------------------------------------------------");
		SC_LOG_InfoPrint(SC_TAG_RT, "PUBLIC NO           : 0");
		SC_LOG_InfoPrint(SC_TAG_RT, "ROUTE LINK VOL      : %d", guidetbl_p->route_link_vol);
		SC_LOG_InfoPrint(SC_TAG_RT, "ROUTE POINT VOL     : %d", guidetbl_p->route_point_vol);
		SC_LOG_InfoPrint(SC_TAG_RT, "GUIDE LINK VOL      : %d", guidetbl_p->guide_link_vol);
		SC_LOG_InfoPrint(SC_TAG_RT, "GUIDE SHAP VOL      : %d", guidetbl_p->guide_sharp_vol);
		SC_LOG_InfoPrint(SC_TAG_RT, "GUIDE CRS VOL       : %d", guidetbl_p->guide_crs_vol);
		SC_LOG_InfoPrint(SC_TAG_RT, "GUIDE DISTANCE      : %d", dist);
		SC_LOG_InfoPrint(SC_TAG_RT, "GUIDE TBLMAKE TIME  : %f", ((clc[1] - clc[0]) / (DOUBLE) CLOCKS_PER_SEC));
		SC_LOG_InfoPrint(SC_TAG_RT, "-----------------------------------------------------");
	}
#endif
	SC_LOG_DebugPrint(SC_TAG_RT, SC_LOG_END);

	return (jdg);
}

/**
 * @brief	誘導テーブル追加メイン
 * @param	[I]なし
 */
E_SC_RESULT RT_TBL_AddGuideTbl(UINT32 make_dist)
{
	E_SC_RESULT			ret = e_SC_RESULT_SUCCESS;
	RT_TBL_MAIN_t		*public_p;
	RT_TBL_MAIN_t		*private_p;

	UINT32				dist;
	INT16				p_no;
	clock_t 			clc[2] = {};

	SC_LOG_DebugPrint(SC_TAG_RT, SC_LOG_START);

	clc[0] = clock();

	// 誘導テーブル公開番号取得
	p_no = RT_G_TblMgr.public_no;

	if (PUBLIC_NO1 == p_no) {
		p_no = PUBLIC_NO2;
		public_p  = &(RT_G_TblMgr.guide_tbl[PUBLIC_NO1]);
		private_p = &(RT_G_TblMgr.guide_tbl[PUBLIC_NO2]);
	}
	else if (PUBLIC_NO2 == p_no) {
		p_no = PUBLIC_NO1;
		public_p  = &(RT_G_TblMgr.guide_tbl[PUBLIC_NO2]);
		private_p = &(RT_G_TblMgr.guide_tbl[PUBLIC_NO1]);
	}
	else {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 非公開領域←公開領域
	memcpy(private_p->route_link_p, public_p->route_link_p, sizeof(RT_TBL_ROUTELINK_t) * public_p->route_link_vol);
	private_p->route_link_vol = public_p->route_link_vol;
	memcpy(private_p->route_point_p, public_p->route_point_p, sizeof(RT_TBL_ROUTEPOINT_t) * public_p->route_point_vol);
	private_p->route_point_vol = public_p->route_point_vol;
	memcpy(private_p->guide_link_p, public_p->guide_link_p, sizeof(RT_TBL_GUIDELINK_t) * public_p->guide_link_vol);
	private_p->guide_link_vol = public_p->guide_link_vol;
	memcpy(private_p->guide_crs_p, public_p->guide_crs_p, sizeof(RT_TBL_GUIDECRS_t) * public_p->guide_crs_vol);
	private_p->guide_crs_vol = public_p->guide_crs_vol;
	memcpy(private_p->guide_sharp_p, public_p->guide_sharp_p, sizeof(RT_XY_t) * public_p->guide_sharp_vol);
	private_p->guide_sharp_vol = public_p->guide_sharp_vol;
	private_p->rt_id   = public_p->rt_id;
	private_p->make_no = public_p->make_no;

	// 誘導リンク情報設定
	ret = RT_TBL_SetGuideLink(private_p, make_dist);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 排他開始
	ret = SC_LockMutex(&m_Mutex);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 誘導テーブル公開番号切り替え
	RT_G_TblMgr.public_no = p_no;

	// 排他終了
	ret = SC_UnLockMutex(&m_Mutex);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

#if 1
	clc[1] = clock();
	dist  = (private_p->route_link_p + private_p->route_link_vol - 1)->total_dist;
	dist += (private_p->route_link_p + private_p->route_link_vol - 1)->dist;

	SC_LOG_InfoPrint(SC_TAG_RT, "-----------------------------------------------------");
	SC_LOG_InfoPrint(SC_TAG_RT, "PUBLIC NO           : %d", p_no);
	SC_LOG_InfoPrint(SC_TAG_RT, "ROUTE LINK VOL      : %d", private_p->route_link_vol);
	SC_LOG_InfoPrint(SC_TAG_RT, "ROUTE POINT VOL     : %d", private_p->route_point_vol);
	SC_LOG_InfoPrint(SC_TAG_RT, "GUIDE LINK VOL      : %d", private_p->guide_link_vol);
	SC_LOG_InfoPrint(SC_TAG_RT, "GUIDE SHAP VOL      : %d", private_p->guide_sharp_vol);
	SC_LOG_InfoPrint(SC_TAG_RT, "GUIDE CRS VOL       : %d", private_p->guide_crs_vol);
	SC_LOG_InfoPrint(SC_TAG_RT, "GUIDE DISTANCE      : %d", dist);
	SC_LOG_InfoPrint(SC_TAG_RT, "GUIDE TBLMAKE TIME  : %f", ((clc[1] - clc[0]) / (DOUBLE) CLOCKS_PER_SEC));
	SC_LOG_InfoPrint(SC_TAG_RT, "-----------------------------------------------------");
	SC_LOG_DebugPrint(SC_TAG_RT, SC_LOG_END);
#endif

	return (ret);
}

/**
 * @brief	誘導テーブル作成メイン
 * @param	[I]なし
 */
E_SC_RESULT RT_TBL_FreeGuideTbl()
{

	E_SC_RESULT			ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT			jdg = e_SC_RESULT_SUCCESS;
	RT_TBL_MAIN_t		*guidetbl_p;
	INT16				ilp;

	SC_LOG_DebugPrint(SC_TAG_RT, SC_LOG_START);

	// 排他開始
	ret = SC_LockMutex(&m_Mutex);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	do {
		// 誘導テーブル非公開に設定
		RT_G_TblMgr.public_no = PUBLIC_INI;

		// 準備
		for (ilp = 0 ; ilp < PUBLIC_MAX ; ilp++) {
			// 誘導テーブル取得
			guidetbl_p = &(RT_G_TblMgr.guide_tbl[ilp]);

			// 領域解放
			if (NULL != guidetbl_p->route_link_p) {
				SC_MEM_Free(guidetbl_p->route_link_p, e_MEM_TYPE_GUIDETBL);		// 経路リンク
				guidetbl_p->route_link_p = NULL;
			}
			if (NULL != guidetbl_p->guide_link_p) {
				SC_MEM_Free(guidetbl_p->guide_link_p, e_MEM_TYPE_GUIDETBL);		// 誘導リンク
				guidetbl_p->guide_link_p = NULL;
			}
			if (NULL != guidetbl_p->route_point_p) {
				SC_MEM_Free(guidetbl_p->route_point_p, e_MEM_TYPE_GUIDETBL);	// 地点情報
				guidetbl_p->route_point_p = NULL;
			}
			if (NULL != guidetbl_p->guide_crs_p) {
				SC_MEM_Free(guidetbl_p->guide_crs_p, e_MEM_TYPE_GUIDETBL);		// 誘導交差点情報
				guidetbl_p->guide_crs_p = NULL;
			}
			if (NULL != guidetbl_p->guide_sharp_p) {
				SC_MEM_Free(guidetbl_p->guide_sharp_p, e_MEM_TYPE_GUIDETBL);	// 誘導リンク形状情報
				guidetbl_p->guide_sharp_p = NULL;
			}

			// 誘導テーブル初期化
			ret = RT_TBL_InitGuideTbl(guidetbl_p);
			if (e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
				jdg = e_SC_RESULT_FAIL;
				break;
			}
		}
		if (e_SC_RESULT_FAIL == jdg) {
			break;
		}

	} while(0);

	// 排他終了
	ret = SC_UnLockMutex(&m_Mutex);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	SC_LOG_DebugPrint(SC_TAG_RT, SC_LOG_END);

	return (jdg);

}

/**
 * @brief	誘導テーブル初期化
 * @param	[I]誘導テーブル
 */
static E_SC_RESULT RT_TBL_InitGuideTbl(RT_TBL_MAIN_t *guidetbl_p)
{

	SC_LOG_DebugPrint(SC_TAG_RT, SC_LOG_START);

	if (NULL == guidetbl_p) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 初期化 TODO
	memset(guidetbl_p, 0x00, sizeof(RT_TBL_MAIN_t));
	memset(&guidetbl_p->rt_id, 0xFF, sizeof(RT_ROUTEID_t));

	SC_LOG_DebugPrint(SC_TAG_RT, SC_LOG_END);

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief	誘導テーブル作成メイン
 * @param	[I]なし
 */
static E_SC_RESULT RT_TBL_AllocGuideTbl(RT_TBL_MAIN_t *guidetbl_p, SC_RP_RouteMng *mst_route_p)
{

	UINT32			size = 0;

	SC_LOG_DebugPrint(SC_TAG_RT, SC_LOG_START);

	if (NULL == guidetbl_p || NULL == mst_route_p) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 推奨経路構成リンク本数から必要領域サイズ取得
	size = sizeof(RT_TBL_ROUTELINK_t) * mst_route_p->linkVol;

	// 領域確保
	guidetbl_p->route_link_p = SC_MEM_Alloc(size, e_MEM_TYPE_GUIDETBL);
	if (NULL == guidetbl_p->route_link_p) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 領域初期化
	memset(guidetbl_p->route_link_p, 0x00, size);
	SC_LOG_InfoPrint(SC_TAG_RT, "RT_TBL_ROUTELINK_t  ALLOC SIZE : %d", size);

	// 推奨経路構成リンク本数から必要領域サイズ取得
	size = sizeof(RT_TBL_GUIDELINK_t) * mst_route_p->linkVol;

	// 領域確保
	guidetbl_p->guide_link_p = SC_MEM_Alloc(size, e_MEM_TYPE_GUIDETBL);
	if (NULL == guidetbl_p->guide_link_p) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 領域初期化
	memset(guidetbl_p->guide_link_p, 0x00, size);
	SC_LOG_InfoPrint(SC_TAG_RT, "RT_TBL_GUIDELINK_t  ALLOC SIZE : %d", size);

	// 推奨経路構成区間数から必要領域サイズ取得
	size = sizeof(RT_TBL_ROUTEPOINT_t) * mst_route_p->sectVol;

	// 領域確保
	guidetbl_p->route_point_p = SC_MEM_Alloc(size, e_MEM_TYPE_GUIDETBL);
	if (NULL == guidetbl_p->route_point_p) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 領域初期化
	memset(guidetbl_p->route_point_p, 0x00, size);
	SC_LOG_InfoPrint(SC_TAG_RT, "RT_TBL_ROUTEPOINT_t ALLOC SIZE : %d", size);

	// 誘導交差点情報から必要領域サイズ取得
	size = sizeof(RT_TBL_GUIDECRS_t) * CRSVOL_MAX;

	// 領域確保
	guidetbl_p->guide_crs_p = SC_MEM_Alloc(size, e_MEM_TYPE_GUIDETBL);
	if (NULL == guidetbl_p->guide_crs_p) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 領域初期化
	memset(guidetbl_p->guide_crs_p, 0x00, size);
	SC_LOG_InfoPrint(SC_TAG_RT, "RT_TBL_GUIDECRS_t   ALLOC SIZE : %d", size);

	// 誘導リンク形状点情報から必要領域サイズ取得
	size = sizeof(RT_XY_t) * SHARPVOL_MAX;

	// 領域確保
	guidetbl_p->guide_sharp_p = SC_MEM_Alloc(size, e_MEM_TYPE_GUIDETBL);
	if (NULL == guidetbl_p->guide_sharp_p) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 領域初期化
	memset(guidetbl_p->guide_sharp_p, 0x00, size);
	SC_LOG_InfoPrint(SC_TAG_RT, "RT_XY_t             ALLOC SIZE : %d", size);

	SC_LOG_DebugPrint(SC_TAG_RT, SC_LOG_END);

	return (e_SC_RESULT_SUCCESS);

}

/**
 * @brief	誘導テーブル取得
 * @param	[I]なし
 */
RT_TBL_MAIN_t *RT_TBL_LockGuideTbl()
{

	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	// 排他開始
	ret = SC_LockMutex(&m_Mutex);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] SC_LockMutext error, " HERE);
		return (NULL);
	}
	// 非公開中
	if (PUBLIC_INI == RT_G_TblMgr.public_no) {
		ret = RT_TBL_UnLockGuideTbl();
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] SC_UnLockMutext error, " HERE);
		}
		return (NULL);
	}

	return (&(RT_G_TblMgr.guide_tbl[RT_G_TblMgr.public_no]));
}

/**
 * @brief	誘導テーブル取得
 * @param	[I]なし
 */
E_SC_RESULT RT_TBL_UnLockGuideTbl()
{

	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	// 排他終了
	ret = SC_UnLockMutex(&m_Mutex);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] SC_UnLockMutext error, " HERE);
	}

	return (ret);
}

/**
 * @brief	初期化処理
 */
E_SC_RESULT RT_TBL_Init()
{

	E_SC_RESULT			ret = e_SC_RESULT_SUCCESS;

	// Mutex生成
	ret = SC_CreateMutex(&m_Mutex);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RM, "[TBL] SC_CreateMutext error, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 誘導テーブル公開番号初期化
	RT_G_TblMgr.public_no = PUBLIC_INI;

	// 誘導テーブル初期化
	ret = RT_TBL_InitGuideTbl(&(RT_G_TblMgr.guide_tbl[PUBLIC_NO1]));
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}
	ret = RT_TBL_InitGuideTbl(&(RT_G_TblMgr.guide_tbl[PUBLIC_NO2]));
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief	パラメータ初期設定
 */
static E_SC_RESULT RT_TBL_SetParam()
{

	E_SC_RESULT				ret = e_SC_RESULT_SUCCESS;
	SMCARSTATE				carState  = {};
	RT_MAPREQ_t				reqtbl    = {};
	T_DHC_REQ_PARCEL		mapReqPcl = {};
	INT32					aRegion   = SYS_REGION_JPN;
	DOUBLE					lat,lon;
	DOUBLE					x,y;
	UINT32					parcel_id;

	// ---------------------------------------------------------------------
	// 角度テーブル設定
	// ---------------------------------------------------------------------
	ret = SC_MNG_GetRegion(&aRegion);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 国内
	if (SYS_REGION_JPN == aRegion) {
		RT_TURNDIR_TBL[0] = TURN_UT;		// [00]:000 - 022
		RT_TURNDIR_TBL[1] = TURN_BR;		// [01]:023 - 045
		RT_TURNDIR_TBL[2] = TURN_BR;		// [02]:046 - 068
		RT_TURNDIR_TBL[3] = TURN_R;			// [03]:069 - 091
		RT_TURNDIR_TBL[4] = TURN_R;			// [04]:092 - 114
		RT_TURNDIR_TBL[5] = TURN_FR;		// [05]:115 - 137
		RT_TURNDIR_TBL[6] = TURN_FR;		// [06]:138 - 160
		RT_TURNDIR_TBL[7] = TURN_ST;		// [07]:161 - 183
		RT_TURNDIR_TBL[8] = TURN_ST;		// [08]:184 - 206
		RT_TURNDIR_TBL[9] = TURN_FL;		// [09]:207 - 229
		RT_TURNDIR_TBL[10] = TURN_FL;		// [10]:230 - 252
		RT_TURNDIR_TBL[11] = TURN_L;		// [11]:253 - 275
		RT_TURNDIR_TBL[12] = TURN_L;		// [12]:276 - 298
		RT_TURNDIR_TBL[13] = TURN_BL;		// [13]:299 - 321
		RT_TURNDIR_TBL[14] = TURN_BL;		// [14]:322 - 344
		RT_TURNDIR_TBL[15] = TURN_BL; 		// [15]:345 - 359
	}else {
		RT_TURNDIR_TBL[0] = TURN_BR;		// [00]:000 - 022
		RT_TURNDIR_TBL[1] = TURN_BR;		// [01]:023 - 045
		RT_TURNDIR_TBL[2] = TURN_BR;		// [02]:046 - 068
		RT_TURNDIR_TBL[3] = TURN_R;			// [03]:069 - 091
		RT_TURNDIR_TBL[4] = TURN_R;			// [04]:092 - 114
		RT_TURNDIR_TBL[5] = TURN_FR;		// [05]:115 - 137
		RT_TURNDIR_TBL[6] = TURN_FR;		// [06]:138 - 160
		RT_TURNDIR_TBL[7] = TURN_ST;		// [07]:161 - 183
		RT_TURNDIR_TBL[8] = TURN_ST;		// [08]:184 - 206
		RT_TURNDIR_TBL[9] = TURN_FL;		// [09]:207 - 229
		RT_TURNDIR_TBL[10] = TURN_FL;		// [10]:230 - 252
		RT_TURNDIR_TBL[11] = TURN_L;		// [11]:253 - 275
		RT_TURNDIR_TBL[12] = TURN_L;		// [12]:276 - 298
		RT_TURNDIR_TBL[13] = TURN_BL;		// [13]:299 - 321
		RT_TURNDIR_TBL[14] = TURN_BL;		// [14]:322 - 344
		RT_TURNDIR_TBL[15] = TURN_UT; 		// [15]:345 - 359
	}

	// ---------------------------------------------------------------------
	// 実長設定
	// ---------------------------------------------------------------------
#if 1
	ret = SC_MNG_GetCarState(&carState, e_SC_CARLOCATION_NOW);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 緯度経度→座標変換
	lat = (DOUBLE) carState.coord.latitude / 1024;
	lon = (DOUBLE) carState.coord.longitude / 1024;
	SC_Lib_ChangeTitude2PID(lat, lon, 1, &parcel_id, &x, &y);

	reqtbl.data_vol = 1;
	reqtbl.data[0].parcel_id = parcel_id;

	mapReqPcl.parcelNum = 1;
	mapReqPcl.parcelInfo[0].parcelId = parcel_id;
	mapReqPcl.parcelInfo[0].mapKind = SC_DHC_GetKindMask(e_DATA_KIND_PARCEL_BASIS);

	// 地図データ要求
	ret = RT_MAP_DataRead(&mapReqPcl, &reqtbl);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}
	if (NULL == reqtbl.data[0].basis_p) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 実長設定
	RT_real_x = (DOUBLE)SC_MA_REAL_LEN(SC_MA_D_BASIS_GET_X_BOTTOM(reqtbl.data[0].basis_p));
	RT_real_y = (DOUBLE)SC_MA_REAL_LEN(SC_MA_D_BASIS_GET_Y_LEFT(reqtbl.data[0].basis_p));

	// 地図データ解放
	ret = RT_MAP_DataFree(&reqtbl);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}
#else
	RT_real_x = (DOUBLE)((DOUBLE)13824 / (DOUBLE)20000); 			// 仮：実長変換係数(x)
	RT_real_y = (DOUBLE)((DOUBLE)11288 / (DOUBLE)20000);			// 仮：実長変換係数(y)
#endif

	SC_LOG_ErrorPrint(SC_TAG_RT, "[TBL] RT_real_x=%f RT_real_y=%f ", RT_real_x, RT_real_y );

	return (e_SC_RESULT_SUCCESS);

}
