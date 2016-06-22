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
 * RP_AreaDiv.c
 *
 *  Created on: 2015/11/06
 *      Author: masutani
 */

#include "sms-core/SMCoreRP/SMCoreRPInternal.h"

#ifdef	_WIN32
T_DHC_DOWNLOAD_AREA *RC_GetDownLoad_Area_Debug();
#endif
T_DHC_DOWNLOAD_AREA *RC_GetDownLoad_Area();

static int log_dsp_f = 0;
static Bool checkCoverArea(UINT32 aLb, UINT32 aRt, UINT32 aTarget);
static Bool checkCoverAreaNbrLinks(UINT32 aLb, UINT32 aRt, SCRP_NEIGHBORINFO* aNbr);

#define		DIV_NETWORK_PARCEL_CNT				15			//	ブロック方向の分割するパーセル数
#define		DIV_NETWORK_MAX_PARCEL_RANGE_CNT	11			//	分割探索エリアの一辺の最大パーセル数
#define		DIV_NETWORK_MAX_PARCEL_CNT			(DIV_NETWORK_MAX_PARCEL_RANGE_CNT * DIV_NETWORK_MAX_PARCEL_RANGE_CNT)			//	分割探索エリアの最大パーセル数
#define		DIV_NETWORK_MAX_LINK_CNT			200000		//	分割探索エリアの最大リンク本数

//#define		DIV_NETWORK_PARCEL_CNT		3			//	ブロック方向の分割するパーセル数
//#define		DIV_NETWORK_MAX_PARCEL_CNT	6			//	分割探索エリアの最大パーセル数
//#define		DIV_NETWORK_MAX_LINK_CNT	(50	* 256)	//	分割探索エリアの最大リンク本数
/**
 * @brief	分割エリアの設定
 * @param	[O]レベル管理テーブル
 */
void RC_AreaDivFree(SCRP_LEVELTBL* aLevel) {

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aLevel) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[RC_AreaDiv] Bad param."HERE);
		return;
	}

	if (NULL != aLevel->pclState && 0 != aLevel->pclStateVol) {
		RP_MemFree(aLevel->pclState, e_MEM_TYPE_ROUTEPLAN);
	}
	aLevel->pclState = NULL;
	aLevel->pclStateVol = 0;

	if (NULL != aLevel->divInfo && 0 != aLevel->divInfoVol) {
		RP_MemFree(aLevel->divInfo, e_MEM_TYPE_ROUTEPLAN);
	}
	aLevel->divInfo = NULL;
	aLevel->divInfoVol = 0;

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
}
void RC_AreaDivFree_old(SCRP_LEVELTBL_OLD* aLevel) {

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if (NULL == aLevel) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[RC_AreaDiv] Bad param."HERE);
		return;
	}

	if (NULL != aLevel->topArea.parcelTop_p && 0 != aLevel->topArea.parcelVol) {
		RP_MemFree(aLevel->topArea.parcelTop_p, e_MEM_TYPE_ROUTEPLAN);
	}
	aLevel->topArea.parcelTop_p = NULL;
	aLevel->topArea.parcelVol = 0;

	if (NULL != aLevel->topArea.divArea_p && 0 != aLevel->topArea.divAreaVol) {
		RP_MemFree(aLevel->topArea.divArea_p, e_MEM_TYPE_ROUTEPLAN);
	}
	aLevel->topArea.divArea_p = NULL;
	aLevel->topArea.divAreaVol = 0;

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
}

/**
 * @brief	分割エリアの設定
 * @param	[O]レベル管理テーブル
 * @param	[I]O側近傍情報
 * @param	[I]D側近傍情報
 */
E_SC_RESULT RC_AreaDiv(SCRP_LEVELTBL_OLD* aLevel, SCRP_NEIGHBORINFO* aONbor, SCRP_NEIGHBORINFO* aDNbor, T_DHC_ROAD_DENSITY*	density_area_ptr)
{
	//	出発地から目的地への方向を求める。

	//	パーセル相対位置の算出
	INT32	target_level = 1;	// 	変換するﾚﾍﾞﾙ
	UINT32	O_parcelID;			// 	変換するﾊﾟｰｾﾙID
	DOUBLE	O_x;				// 	変換する正規化Ｘ座標
	DOUBLE	O_y;				// 	変換する正規化Ｙ座標
	UINT32	D_parcelID;			// 	変換するﾊﾟｰｾﾙID
	DOUBLE	D_x;				// 	変換する正規化Ｘ座標
	DOUBLE	D_y;				// 	変換する正規化Ｙ座標
	INT32 	alter_pos_x;		//	Ｘ方向の相対位置
	INT32 	alter_pos_y;		//	Ｙ方向の相対位置

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	if ((NULL == aLevel) || (NULL == aONbor) || (NULL == aDNbor) || (NULL == density_area_ptr)) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[RC_AreaDiv] Bad param."HERE);
		return (e_SC_RESULT_BADPARAM);
	}

#if		0
	if(log_dsp_f)
		SC_LOG_DebugPrint(SC_TAG_RC, "RC_AreaDiv   parcelID...0x%08x    xy(%d,%d)  aLevel...%x\n"
				,aLevel->topArea.baseParcelId
				,aLevel->topArea.x
				,aLevel->topArea.y,
				 aLevel
				);
#endif

	if(e_SC_RESULT_FAIL == RC_AreaDiv_Change_ParcelID_Pos(
															target_level,					// 変換するﾚﾍﾞﾙ
															aONbor->point.parcelId,			// 変換するﾊﾟｰｾﾙID
															(DOUBLE)aONbor->point.x,		// 変換する正規化Ｘ座標
															(DOUBLE)aONbor->point.y,		// 変換する正規化Ｙ座標
															&O_parcelID,					// 変換するﾊﾟｰｾﾙID
															&O_x,							// 変換する正規化Ｘ座標
															&O_y							// 変換する正規化Ｙ座標
													)
	)
	{
		SC_LOG_ErrorPrint(SC_TAG_RC, "[RC_AreaDiv] e_SC_RESULT_FAIL == RC_AreaDiv_Change_ParcelID_Pos"HERE);
		return (e_SC_RESULT_FAIL);
	}


	if(e_SC_RESULT_FAIL == RC_AreaDiv_Change_ParcelID_Pos(
															target_level,					// 変換するﾚﾍﾞﾙ
															aDNbor->point.parcelId,			// 変換するﾊﾟｰｾﾙID
															(DOUBLE)aDNbor->point.x,		// 変換する正規化Ｘ座標
															(DOUBLE)aDNbor->point.y,		// 変換する正規化Ｙ座標
															&D_parcelID,					// 変換するﾊﾟｰｾﾙID
															&D_x,							// 変換する正規化Ｘ座標
															&D_y							// 変換する正規化Ｙ座標
													)
	)
	{
		SC_LOG_ErrorPrint(SC_TAG_RC, "[RC_AreaDiv] e_SC_RESULT_FAIL == RC_AreaDiv_Change_ParcelID_Pos"HERE);
		return (e_SC_RESULT_FAIL);
	}


	if( -1 == SC_MESH_GetAlterPos(
										O_parcelID,
										D_parcelID,
										target_level,
										&alter_pos_x,
										&alter_pos_y
									)
	)
	{
		SC_LOG_ErrorPrint(SC_TAG_RC, "[RC_AreaDiv] -1 == SC_MESH_GetAlterPos"HERE);
		return (e_SC_RESULT_FAIL);
	}


	INT32 s_alter_pos_x;	//	Ｘ方向の相対位置（開始）
	INT32 s_alter_pos_y;	//	Ｙ方向の相対位置（開始）
	INT32 e_alter_pos_x;	//	Ｘ方向の相対位置（終了）
	INT32 e_alter_pos_y;	//	Ｙ方向の相対位置（終了）


	INT32 delta_x;	//	増分値（Ｘ方向）
	INT32 delta_y;	//	増分値（Ｙ方向）

	if(alter_pos_x < 0)	{
		delta_x = -1;							//	進行方向
		s_alter_pos_x = aLevel->topArea.pclRect.xSize - 1;	//	開始位置
		e_alter_pos_x = 0;						//	終点位置
	}
	else	{
		delta_x =  1;							//	進行方向
		s_alter_pos_x = 0;						//	開始位置
		e_alter_pos_x = aLevel->topArea.pclRect.xSize - 1;	//	終点位置
	}

	if(alter_pos_y < 0)	{
		delta_y = -1;							//	進行方向
		s_alter_pos_y = aLevel->topArea.pclRect.ySize - 1;	//	開始位置
		e_alter_pos_y =0;						//	終点位置

	}
	else	{
		delta_y =  1;							//	進行方向
		s_alter_pos_y = 0;						//	開始位置
		e_alter_pos_y = aLevel->topArea.pclRect.ySize - 1;	//	終点位置
	}


	//	トップレベルのエリア範囲より、道路密度データ取得。
#if		0
	//if(log_dsp_f)
	{
		int  x, y, k;
		SC_LOG_DebugPrint(SC_TAG_RC, "@@@@@@@@@@@@@@　　　道路密度データ\n" );
		for( y=0;y<  density_area_ptr->y;y++)	{
			SC_LOG_DebugPrint(SC_TAG_RC, "@@@@@   y...%d. \n" , y);
			for( x=0;x<  density_area_ptr->x;x++)	{
				int  tbl_no;
				int  link_cnt;
				tbl_no 		= density_area_ptr->x * y + x;
				link_cnt 	= density_area_ptr->data[tbl_no].density;
				char  text[256];
				strcpy(text, "id...");
				for( k=0;k<8;k++)	{
					if(density_area_ptr->data[tbl_no].areaId[k] == 0)	continue;

					char  www[64];
					sprintf(www,"%d ", density_area_ptr->data[tbl_no].areaId[k]);
					strcat(text, www);
				}
				SC_LOG_DebugPrint(SC_TAG_RC, "@@   x...%d.  tbl_no...%d  link_cnt...%d  %s \n" , x, tbl_no, link_cnt, text);
			}
		}
		SC_LOG_DebugPrint(SC_TAG_RC, "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n" );


		SC_LOG_DebugPrint(SC_TAG_RC,"@@@@  delta_x/_y...%d / %d  alter_pos_x/_y...%d / %d  s_alter_pos_x/_y...%d / %d  e_alter_pos_x/_y...%d / %d\n",
			delta_x, delta_y,
			alter_pos_x, alter_pos_y,
			s_alter_pos_x, s_alter_pos_y,
			e_alter_pos_x, e_alter_pos_y
		);
	}
#endif

	//■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
	//	探索エリアの矩形が縦長の時、Ｙ方向を最初に40（DIV_NETWORK_PARCEL_CNT）を引き、39(40-1)で割り、ブロック数を得る。
	//	それ以外　　　　　　　の時、Ｘ方向を最初に40（DIV_NETWORK_PARCEL_CNT）を引き、39(40-1)で割り、ブロック数を得る。
	//	余りが出た時、商（ブロック数）を+1する。

	INT32 range_x, range_y;	//	範囲
	range_x = abs(e_alter_pos_x - s_alter_pos_x) + 1;
	range_y = abs(e_alter_pos_y - s_alter_pos_y) + 1;

	E_SC_RESULT  rc;

	if(range_y < range_x)	{	//	探索エリアが横長の時
		rc = 	RC_Div_Search_Area
				(
					0,									//	処理タイプ
														//		０：分割探索エリア（T_DivAreaInfo、T_ParcelList）を生成
														//		１：分割探索エリア（T_DivAreaInfo、T_ParcelList）に設定
					aLevel,								//	トップレベル探索エリア
					aLevel->topArea.pclRect.parcelId,	//	非分割エリア左下絶対パーセルＩＤ（相対位置0:0の時）
					O_parcelID,							//	出発地点の絶対パーセルＩＤ（レベル1）
					D_parcelID,							//	目的地点の絶対パーセルＩＤ（レベル1）
					DIV_NETWORK_MAX_PARCEL_RANGE_CNT,	//	分割探索エリアの一辺の最大パーセル数
					DIV_NETWORK_PARCEL_CNT,				//	ブロックサイズ（DIV_NETWORK_PARCEL_CNT）
					DIV_NETWORK_MAX_PARCEL_CNT,			//	分割探索エリアの最大パーセル数
					DIV_NETWORK_MAX_LINK_CNT,			//	分割探索エリアの最大リンク本数

					s_alter_pos_y,						//	ブロック開始・相対位置
					e_alter_pos_y,						//	ブロック終了・相対位置
					delta_y,							//	ブロック増減値
					range_y,							//	ブロック方向の範囲

					s_alter_pos_x,						//	階層開始・相対位置
					e_alter_pos_x,						//	階層終了・相対位置
					delta_x,							//	階層増減値
					range_x,							//	階層の範囲

					0,									//	矩形の形状	０：横長	１：縦長
					density_area_ptr,					//	道路密度データ取得関数の出力値
					aONbor,								//	出発地点
					aDNbor,								//	目的地点
					0,									//	分割エリア数（処理タイプ：１の時、有効）
					0									//	パーセル数（処理タイプ：１の時、有効）


				);
	}
	else					{	//	探索エリアが横長または正方形の時
		rc =	RC_Div_Search_Area
				(
					0,									//	処理タイプ
														//		０：分割探索エリア（T_DivAreaInfo、T_ParcelList）を生成
														//		１：分割探索エリア（T_DivAreaInfo、T_ParcelList）に設定
					aLevel,								//	トップレベル探索エリア
					aLevel->topArea.pclRect.parcelId,	//	非分割エリア左下絶対パーセルＩＤ（相対位置0:0の時）
					O_parcelID,							//	出発地点の絶対パーセルＩＤ（レベル1）
					D_parcelID,							//	目的地点の絶対パーセルＩＤ（レベル1）
					DIV_NETWORK_MAX_PARCEL_RANGE_CNT,	//	分割探索エリアの一辺の最大パーセル数(DIV_NETWORK_MAX_PARCEL_RANGE_CNT)
					DIV_NETWORK_PARCEL_CNT,				//	ブロックサイズ（DIV_NETWORK_PARCEL_CNT）
					DIV_NETWORK_MAX_PARCEL_CNT,			//	分割探索エリアの最大パーセル数
					DIV_NETWORK_MAX_LINK_CNT,			//	分割探索エリアの最大リンク本数

					s_alter_pos_x,						//	ブロック開始・相対位置
					e_alter_pos_x,						//	ブロック終了・相対位置
					delta_x,							//	ブロック増減値
					range_x,							//	ブロック方向の範囲

					s_alter_pos_y,						//	階層開始・相対位置
					e_alter_pos_y,						//	階層終了・相対位置
					delta_y,							//	階層増減値
					range_y,							//	階層の範囲

					1,									//	矩形の形状	０：横長	１：縦長
					density_area_ptr,					//	道路密度データ取得関数の出力値
					aONbor,								//	出発地点
					aDNbor,								//	目的地点
					0,									//	分割エリア数（処理タイプ：１の時、有効）
					0									//	パーセル数（処理タイプ：１の時、有効）

				);
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);

	return (rc);


}
/**
 * @brief	分割探索エリアの生成
 * @param	[I]検索範囲ﾙ
 * @param	[I]変換する正規化座標
 * @param	[O]変換後の正規化座標
 */
E_SC_RESULT  RC_Div_Search_Area
		(
			INT32  					process_f,					//	処理タイプ
																//		０：分割探索エリア（T_DivAreaInfo、T_ParcelList）を生成
																//		１：分割探索エリア（T_DivAreaInfo、T_ParcelList）に設定
			SCRP_LEVELTBL_OLD* 		aLevel,						//	トップレベル探索エリア
			INT32  					baseParcelId,				//	非分割エリア左下絶対パーセルＩＤ（相対位置0:0の時）
			UINT32					O_parcelID,					//	出発地点の絶対パーセルＩＤ（レベル1）
			UINT32					D_parcelID,					//	目的地点の絶対パーセルＩＤ（レベル1）
			INT32					parcel_range_cnt_per_block,	//	分割探索エリアの一辺の最大パーセル数(DIV_NETWORK_MAX_PARCEL_RANGE_CNT)
			INT32  					parcel_cnt_per_block,		//	ブロック当たりのパーセル読み込み数（DIV_NETWORK_PARCEL_CNT）
			INT32  					max_parcel_cnt,				//	分割探索エリアの最大パーセル数
			INT32  					max_link_cnt,				//	分割探索エリアの最大リンク本数

			INT32  					block_start,				//	ブロック開始・相対位置
			INT32  					block_end,					//	ブロック終了・相対位置
			INT32  					block_delt,					//	ブロック増減値
			INT32  					block_range,				//	ブロック方向の範囲

			INT32  					layer_start,				//	階層開始・相対位置
			INT32  					layer_end,					//	階層終了・相対位置
			INT32  					layer_delt,					//	階層増減値
			INT32  					layer_range,				//	階層の範囲

			INT32 					block_f,					//	矩形の形状	０：横長	１：縦長
			T_DHC_ROAD_DENSITY*  	density_area_ptr,			//	道路密度データ取得関数の出力値
			SCRP_NEIGHBORINFO* 			aONbor,						//	出発地点
			SCRP_NEIGHBORINFO* 			aDNbor,						//	目的地点
			INT32  					divarea_cnt,				//	分割エリア数（処理タイプ：１の時、有効）
			INT32  					parcel_cnt					//	パーセル数（処理タイプ：１の時、有効）
		)
{

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);


	T_DHC_DOWNLOAD_AREA*  	downloadarea_ptr;	//	ダウンロードエリア
	T_RC_DivAreaInf*		T_RC_DivAreaInf_ptr;	//	「分割探索エリア管理情報」


	INT32  i, j;

#if		0
	//if(process_f == 1)	log_dsp_f = 1;		//	デバグ用

	//if(log_dsp_f)
	{
		SC_LOG_DebugPrint(SC_TAG_RC,"@@@@  START  start_block_no... %d  start_layer_no...%d  min_layer_range...%d  block_f...%d  divarea_cnt...%d\n",
							start_block_no,
							start_layer_no,
							min_layer_range,
							block_f,
							divarea_cnt
					);
		SC_LOG_DebugPrint(SC_TAG_RC,"@@@@  START  block_start... %d  block_end...%d  block_delt...%d  block_range...%d\n",
									block_start,	block_end,	block_delt,		block_range		);

		SC_LOG_DebugPrint(SC_TAG_RC,"@@@@  START  layer_start... %d  layer_end...%d  layer_delt...%d  layer_range...%d\n",
									layer_start,	layer_end,	layer_delt,		layer_range		);
	}
#endif


	if(process_f==1)	{	//	処理タイプ
							//		０：分割探索エリア（T_DivAreaInfo、T_ParcelList）を生成
							//		１：分割探索エリア（T_DivAreaInfo、T_ParcelList）に設定
		if(divarea_cnt == 0)	return (e_SC_RESULT_MALLOC_ERR);

		//	トップレベルのT_AreaInfoテーブルに設定する。
		aLevel->topArea.divArea_p = (T_DivAreaInfo*) RP_MemAlloc(sizeof(T_DivAreaInfo) * divarea_cnt, e_MEM_TYPE_ROUTEPLAN);

		if (NULL == aLevel->topArea.divArea_p) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[AreaDiv] RP_MemAlloc T_DivAreaInfo  error."HERE);
			return (e_SC_RESULT_MALLOC_ERR);
		}
		RP_Memset0(aLevel->topArea.divArea_p, sizeof(T_DivAreaInfo) * divarea_cnt);
		aLevel->topArea.divAreaVol = divarea_cnt;

		aLevel->topArea.parcelVol = parcel_cnt;
		aLevel->topArea.parcelTop_p = (SCRP_AREAPCLSTATE*)RP_MemAlloc(sizeof(SCRP_AREAPCLSTATE) * parcel_cnt, e_MEM_TYPE_ROUTEPLAN);
		aLevel->topArea.divArea_p->parcel_p = aLevel->topArea.parcelTop_p;

		if (NULL == aLevel->topArea.divArea_p->parcel_p) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[AreaDiv] RP_MemAlloc T_ParcelList  error."HERE);
			RC_AreaDivFree_old(aLevel);
			return (e_SC_RESULT_MALLOC_ERR);
		}
		RP_Memset0(aLevel->topArea.parcelTop_p, sizeof(SCRP_AREAPCLSTATE) * parcel_cnt);

		//	「分割探索エリア管理情報」の確保
		T_RC_DivAreaInf_ptr = (T_RC_DivAreaInf*) RP_MemAlloc(sizeof(T_RC_DivAreaInf) * divarea_cnt, e_MEM_TYPE_ROUTEPLAN);
		if (NULL == T_RC_DivAreaInf_ptr) {
			SC_LOG_ErrorPrint(SC_TAG_RC, "[AreaDiv] RP_MemAlloc T_RC_DivAreaInf_ptr  error."HERE);
			RC_AreaDivFree_old(aLevel);
			return (e_SC_RESULT_MALLOC_ERR);
		}
		RP_Memset0(T_RC_DivAreaInf_ptr, sizeof(T_RC_DivAreaInf) * divarea_cnt);
		//	「分割探索エリア管理情報」の初期化
		for(i=0;i<divarea_cnt;i++)	{
			(T_RC_DivAreaInf_ptr+i)->proc_f = -1;
		}

		//	ダウンロードエリアＩＤの取得
		downloadarea_ptr = RC_GetDownLoad_Area();

#ifdef	_WIN32
		downloadarea_ptr = RC_GetDownLoad_Area_Debug();	//	デバグ用
#endif


#if		0
		if(log_dsp_f)	{
			SC_LOG_DebugPrint(SC_TAG_RC,"@@@@  START  ダウンロードエリアＩＤの表示");
			for(i=0;i<M_DHC_DOWNLOAD_AREA_MAX;i++)	{
				if(downloadarea_ptr->data[i].download_f == M_DHC_DOWNLOAD_AREA_ON)	{
					SC_LOG_DebugPrint(SC_TAG_RC,"@@@@  START  down_load_area_no...%d", i + 1);
				}
			}
			SC_LOG_DebugPrint(SC_TAG_RC,"@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
		}
#endif

	}

	INT32  T_DivAreaInfo_cnt 	= 0;		//	T_DivAreaInfoテーブルのカウント
	INT32  T_ParcelList_cnt 	= 0;		//	T_ParcelList テーブルのカウント

	INT32  start_block_no = block_start;	//	ブロック方向先頭のテーブルＮＯ
	INT32  block_read_total_cnt = 0;		//	ブロック方向に読み込んだパーセル数（合計）

	INT32  start_layer_no = layer_start;	//	階層方向先頭のテーブルＮＯ
	INT32  layer_read_total_cnt = 0;		//	階層方向に読み込んだパーセル数（合計）
	INT32  min_layer_range = layer_range;	//	最小の階層数

	INT32  o_alt_x = 0;		//	出発地点からの相対位置（ブロック方向）
	INT32  o_alt_y = 0;		// 	出発地点からの相対位置（レイヤー方向）

	while(1)	{
		INT32  last_f = 0;			//	最終ブロックの時、ＯＮ
		INT32  max_read_cnt = 0;	//	ブロック方向に読み込めるパーセル数
		INT32  w_layer_range;		//	読み込んだ層の数

		//	ブロック方向に読み込めるパーセル数の算出
		if(block_read_total_cnt + parcel_cnt_per_block >= block_range)	{
			max_read_cnt = block_range - block_read_total_cnt;
			last_f = 1;
		}
		else	{
			max_read_cnt = parcel_cnt_per_block;
		}
		//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		//		各ブロックの処理
		INT32	read_parcel_cnt = 0;	//	ブロック内で読み込んだパーセル数（総計）
		INT32	read_link_cnt 	= 0;	//	ブロック内で読み込んだリンク本数（総計）

		//	各ブロックでの階層の積み上げを行う。
		//	条件に合う範囲内で、最小の階層を採用する。
		w_layer_range = 0;
		for( i=layer_read_total_cnt; i<layer_range; i++)	{	//	階層方向に読む
			INT32  read_layer_no = layer_start + i * layer_delt;		//	階層    方向に読み込むテーブルＮＯ
			INT32  read_block_no = start_block_no;						//	ブロック方向に読み込むテーブルＮＯ
			INT32  chk_f = 0;		//	条件でＢｒｅａｋした時の判定フラグ
			w_layer_range++;	//	読み込んだ層の数

			for( j=0; j<max_read_cnt; j++)	{	//	読み込めるパーセル数だけブロック方向に読む
				INT32  tbl_no;	//	道路密度データの配列位置
				INT32  link_cnt;

				if(block_f)	{	//	矩形の形状	０：横長	１：縦長
					tbl_no = read_layer_no * block_range + read_block_no;
				}
				else	{
					tbl_no = read_block_no * layer_range + read_layer_no;
				}

				if((density_area_ptr->x * density_area_ptr->y) <= tbl_no)	{	//	テーブルサイズを超えた？
					continue;
				}

				link_cnt = density_area_ptr->data[tbl_no].density * 256;
				read_link_cnt += link_cnt;

				read_parcel_cnt++;

				//	読み込みリンク本数または、パーセル数が上限値を超えた時、該当ブロックの処理を終える。
				if(read_link_cnt > max_link_cnt  ||  read_parcel_cnt > max_parcel_cnt)	{
					read_parcel_cnt--;
					chk_f = 1;
					break;
				}

				read_block_no += block_delt;	//	ブロック方向にシフト
			}
			if(chk_f)	{
				w_layer_range--;	//	該当層でのブロック方向のパーセル情報読み込み時、条件を満たさなかったので、一階層減らす。
				break;
			}
		}
		if(w_layer_range < min_layer_range)	{
			min_layer_range = w_layer_range;
//			if(log_dsp_f)	SC_LOG_DebugPrint(SC_TAG_RC,"@@@@  i...%d  j... %d  min_layer_range...%d",	i, j, min_layer_range	);
		}

		INT32	x_size;				// 	分割エリア内パーセル枚数（Ｘ方向）
		INT32	y_size;				// 	分割エリア内パーセル枚数（Ｙ方向）


		if(block_f)	{	//	矩形の形状	０：横長	１：縦長
			x_size = max_read_cnt;		// Ｘ方向枚数
			y_size = min_layer_range;	// Ｙ方向枚数
		}
		else	{
			x_size = min_layer_range;	// Ｘ方向枚数
			y_size = max_read_cnt;		// Ｙ方向枚数
		}

		T_ParcelList_cnt += x_size * y_size;

		if(process_f==1)	{	//	処理タイプ
								//		０：分割探索エリア（T_DivAreaInfo、T_ParcelList）を生成
								//		１：分割探索エリア（T_DivAreaInfo、T_ParcelList）に設定

			INT32	alt_x;				// 	非分割エリア内の相対位置（Ｘ方向）
			INT32	alt_y;				// 	非分割エリア内の相対位置（Ｙ方向）
			INT32  	workBaseParcelId;	//	分割エリア左下絶対パーセルＩＤ


			if(block_delt < 0)	{
				alt_x = start_block_no - max_read_cnt + 1;
			}
			else	{
				alt_x = start_block_no;
			}
			if(layer_delt < 0)	{
				alt_y = start_layer_no - min_layer_range + 1;
			}
			else	{
				alt_y = start_layer_no;
			}
			if(block_f)	{	//	矩形の形状	０：横長	１：縦長
				workBaseParcelId = SC_MESH_SftParcelId(aLevel->topArea.pclRect.parcelId, alt_x, alt_y); 	// パーセルID
			}
			else	{
				workBaseParcelId = SC_MESH_SftParcelId(aLevel->topArea.pclRect.parcelId, alt_y, alt_x); 	// パーセルID
			}

			(T_RC_DivAreaInf_ptr+T_DivAreaInfo_cnt)->alt_x_for_o_pos 	= o_alt_x;			// 	出発地点からの相対位置（ブロック方向）
			(T_RC_DivAreaInf_ptr+T_DivAreaInfo_cnt)->alt_y_for_o_pos 	= o_alt_y;			// 	出発地点からの相対位置（レイヤー方向）
			(T_RC_DivAreaInf_ptr+T_DivAreaInfo_cnt)->baseParcelId 		= workBaseParcelId;	//	分割エリア左下絶対パーセルＩＤ
			if(block_f)	{	//	矩形の形状	０：横長	１：縦長
				(T_RC_DivAreaInf_ptr+T_DivAreaInfo_cnt)->alt_x 			= alt_x;			// 	非分割エリア内の相対位置（Ｘ方向）
				(T_RC_DivAreaInf_ptr+T_DivAreaInfo_cnt)->alt_y 			= alt_y;			// 	非分割エリア内の相対位置（Ｙ方向）
			}
			else	{
				(T_RC_DivAreaInf_ptr+T_DivAreaInfo_cnt)->alt_x 			= alt_y;			// 	非分割エリア内の相対位置（Ｘ方向）
				(T_RC_DivAreaInf_ptr+T_DivAreaInfo_cnt)->alt_y 			= alt_x;			// 	非分割エリア内の相対位置（Ｙ方向）
			}

			(T_RC_DivAreaInf_ptr+T_DivAreaInfo_cnt)->x_size 			= x_size;			// 	分割エリア内パーセル枚数（Ｘ方向）
			(T_RC_DivAreaInf_ptr+T_DivAreaInfo_cnt)->y_size 			= y_size;			// 	分割エリア内パーセル枚数（Ｙ方向）
			(T_RC_DivAreaInf_ptr+T_DivAreaInfo_cnt)->proc_f 			= 0;				//	処理済みフラグ	０：未処理	１：処理中	２：処理済み

			(T_RC_DivAreaInf_ptr+T_DivAreaInfo_cnt)->proc_no 			= -1;				//	処理順番（１～）
			(T_RC_DivAreaInf_ptr+T_DivAreaInfo_cnt)->link_cnt			= 0;				//	リンク本数

		}

		T_DivAreaInfo_cnt++;	//	T_DivAreaInfoテーブルのカウントアップ

		//	最終ブロックの処理後、各ブロックでの最小の階層を選択する。
		if(last_f)	{
			if(layer_read_total_cnt + min_layer_range >= layer_range)	{	//	最後の層まで処理したら、抜ける。
//				if(log_dsp_f)	SC_LOG_DebugPrint(SC_TAG_RC,"@@@@  最終ブロックの処理  最後の層まで処理したら、抜ける。  layer_read_total_cnt... %d  + min_layer_range...%d > layer_range...%d",	layer_read_total_cnt, min_layer_range, layer_range	);

				break;
			}

			if(min_layer_range > 1)	{
				min_layer_range--;	//	前回層と重複させる。
			}

			layer_read_total_cnt += min_layer_range;

			if(layer_read_total_cnt >= layer_range)	{	//	最後の層まで処理したら、抜ける。
				break;
			}

			//	ブロック方向は、頭から処理する。ただし前回ブロックと重複するようにする。
			start_block_no = block_start;

			block_read_total_cnt = 0;

			o_alt_y++;
			o_alt_x = 0;
			//	階層方向は、層を更新する。
			start_layer_no += (min_layer_range) * layer_delt;

			min_layer_range = layer_range;

#if		0
			SC_LOG_DebugPrint("SC_TAG_RC","@@@@  次の階層の読み込み開始   start_block_no... %d  block_read_total_cnt...%d  start_layer_no...%d  layer_read_total_cnt...%d  min_layer_range...%d \n",
												start_block_no,
												block_read_total_cnt,
												start_layer_no,
												layer_read_total_cnt,
												min_layer_range
										);
#endif

		}
		else	{		//	次のブロックの読み込み準備
			o_alt_x++;
			max_read_cnt--;	//	重複させる。

			block_read_total_cnt 	+= max_read_cnt;
			start_block_no 			+= max_read_cnt * block_delt;

		}
	}


	if(process_f==0)	{	//	処理タイプ
							//		０：分割探索エリア（T_DivAreaInfo、T_ParcelList）を生成
							//		１：分割探索エリア（T_DivAreaInfo、T_ParcelList）に設定
		E_SC_RESULT  rc = 	RC_Div_Search_Area
							(
								1,							//	処理タイプ
															//		０：分割探索エリア（T_DivAreaInfo、T_ParcelList）を生成
															//		１：分割探索エリア（T_DivAreaInfo、T_ParcelList）に設定
								aLevel,						//	トップレベル探索エリア
								baseParcelId,				//	非分割エリア左下絶対パーセルＩＤ（相対位置0:0の時）
								O_parcelID,					//	出発地点の絶対パーセルＩＤ（レベル1）
								D_parcelID,					//	目的地点の絶対パーセルＩＤ（レベル1）
								parcel_range_cnt_per_block,	//	分割探索エリアの一辺の最大パーセル数(DIV_NETWORK_MAX_PARCEL_RANGE_CNT)
								parcel_cnt_per_block,		//	ブロック当たりのパーセル読み込み数（DIV_NETWORK_PARCEL_CNT）
								max_parcel_cnt,				//	分割探索エリアの最大パーセル数
								max_link_cnt,				//	分割探索エリアの最大リンク本数

								block_start,				//	ブロック開始・相対位置
								block_end,					//	ブロック終了・相対位置
								block_delt,					//	ブロック増減値
								block_range,				//	ブロック方向の範囲

								layer_start,				//	階層開始・相対位置
								layer_end,					//	階層終了・相対位置
								layer_delt,					//	階層増減値
								layer_range,				//	階層の範囲

								block_f,					//	矩形の形状	０：横長	１：縦長
								density_area_ptr,			//	道路密度データ取得関数の出力値
								aONbor,						//	出発地点
								aDNbor,						//	目的地点
								T_DivAreaInfo_cnt,			//	分割エリア数（処理タイプ：１の時、有効）
								T_ParcelList_cnt			//	パーセル数（処理タイプ：１の時、有効）
							);

		return (rc);
	}

	//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	//	以下は、処理タイプ：１の時の処理
	//	処理タイプ
	//		０：分割探索エリア（T_DivAreaInfo、T_ParcelList）を生成
	//		１：分割探索エリア（T_DivAreaInfo、T_ParcelList）に設定
	E_SC_RESULT  rc =	 RC_Div_Set_Out_Search_Area
						(
							process_f,					//	処理タイプ
														//		０：分割探索エリア（T_DivAreaInfo、T_ParcelList）を生成
														//		１：分割探索エリア（T_DivAreaInfo、T_ParcelList）に設定
							aLevel,						//	トップレベル探索エリア
							baseParcelId,				//	非分割エリア左下絶対パーセルＩＤ（相対位置0:0の時）
							O_parcelID,					//	出発地点の絶対パーセルＩＤ（レベル1）
							D_parcelID,					//	目的地点の絶対パーセルＩＤ（レベル1）
							parcel_range_cnt_per_block,	//	分割探索エリアの一辺の最大パーセル数(DIV_NETWORK_MAX_PARCEL_RANGE_CNT)
							parcel_cnt_per_block,		//	ブロック当たりのパーセル読み込み数（DIV_NETWORK_PARCEL_CNT）
							max_parcel_cnt,				//	分割探索エリアの最大パーセル数
							max_link_cnt,				//	分割探索エリアの最大リンク本数

							block_start,				//	ブロック開始・相対位置
							block_end,					//	ブロック終了・相対位置
							block_delt,					//	ブロック増減値
							block_range,				//	ブロック方向の範囲

							layer_start,				//	階層開始・相対位置
							layer_end,					//	階層終了・相対位置
							layer_delt,					//	階層増減値
							layer_range,				//	階層の範囲

							block_f,					//	矩形の形状	０：横長	１：縦長
							density_area_ptr,			//	道路密度データ取得関数の出力値
							aONbor,						//	出発地点
							aDNbor,						//	目的地点
							divarea_cnt,				//	分割エリア数（処理タイプ：１の時、有効）
							downloadarea_ptr,			//	ダウンロードエリア
							T_RC_DivAreaInf_ptr			//	「分割探索エリア管理情報」
						);
	//	「分割探索エリア管理情報」の解放
	if(rc != e_SC_RESULT_SUCCESS)
		RC_AreaDivFree_old(aLevel);

	RP_MemFree(T_RC_DivAreaInf_ptr, e_MEM_TYPE_ROUTEPLAN);

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);

	return (rc);

}


/**
 * @brief	分割探索エリアの出力設定（処理タイプ：１の時の処理）
 * @param	[I]検索範囲ﾙ
 * @param	[I]変換する正規化座標
 * @param	[O]変換後の正規化座標
 */
E_SC_RESULT  RC_Div_Set_Out_Search_Area
			(
				INT32  					process_f,					//	処理タイプ
																	//		０：分割探索エリア（T_DivAreaInfo、T_ParcelList）を生成
																	//		１：分割探索エリア（T_DivAreaInfo、T_ParcelList）に設定
				SCRP_LEVELTBL_OLD* 		aLevel,						//	トップレベル探索エリア
				INT32  					baseParcelId,				//	非分割エリア左下絶対パーセルＩＤ（相対位置0:0の時）
				UINT32					O_parcelID,					//	出発地点の絶対パーセルＩＤ（レベル1）
				UINT32					D_parcelID,					//	目的地点の絶対パーセルＩＤ（レベル1）
				INT32					parcel_range_cnt_per_block,	//	分割探索エリアの一辺の最大パーセル数(DIV_NETWORK_MAX_PARCEL_RANGE_CNT)
				INT32  					parcel_cnt_per_block,		//	ブロック当たりのパーセル読み込み数（DIV_NETWORK_PARCEL_CNT）
				INT32  					max_parcel_cnt,				//	分割探索エリアの最大パーセル数
				INT32  					max_link_cnt,				//	分割探索エリアの最大リンク本数

				INT32  					block_start,				//	ブロック開始・相対位置
				INT32  					block_end,					//	ブロック終了・相対位置
				INT32  					block_delt,					//	ブロック増減値
				INT32  					block_range,				//	ブロック方向の範囲

				INT32  					layer_start,				//	階層開始・相対位置
				INT32  					layer_end,					//	階層終了・相対位置
				INT32  					layer_delt,					//	階層増減値
				INT32  					layer_range,				//	階層の範囲

				INT32 					block_f,					//	矩形の形状	０：横長	１：縦長
				T_DHC_ROAD_DENSITY*  	density_area_ptr,			//	道路密度データ取得関数の出力値
				SCRP_NEIGHBORINFO* 			aONbor,						//	出発地点
				SCRP_NEIGHBORINFO* 			aDNbor,						//	目的地点
				INT32  					divarea_cnt,				//	分割エリア数（処理タイプ：１の時、有効）
				T_DHC_DOWNLOAD_AREA*  	downloadarea_ptr,			//	ダウンロードエリア
				T_RC_DivAreaInf*		T_RC_DivAreaInf_ptr			//	「分割探索エリア管理情報」
			)
{

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	INT32  i, j, k;

	//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	//		「パーセル情報」のエリア確保
	SCRP_AREAPCLSTATE*	parcellist_ptr 	= NULL;			//	T_ParcelList テーブルを押さえる。
	INT32  parcel_cnt;
	parcel_cnt = 0;
	parcellist_ptr 	= aLevel->topArea.divArea_p->parcel_p 	+ parcel_cnt;

	for(i=0;i<divarea_cnt;i++)	{
		INT32	alt_x 			= (T_RC_DivAreaInf_ptr+i)->alt_x;				// 	非分割エリア内の相対位置（Ｘ方向）
		INT32	alt_y 			= (T_RC_DivAreaInf_ptr+i)->alt_y;				// 	非分割エリア内の相対位置（Ｙ方向）
		INT32	x_size 			= (T_RC_DivAreaInf_ptr+i)->x_size;				// 	分割エリア内パーセル枚数（Ｘ方向）
		INT32	y_size 			= (T_RC_DivAreaInf_ptr+i)->y_size;				// 	分割エリア内パーセル枚数（Ｙ方向）

		(T_RC_DivAreaInf_ptr+i)->parcel_p = aLevel->topArea.divArea_p->parcel_p 	+ parcel_cnt;		// パーセル情報先頭ポインタ

		for(j=0;j<y_size;j++)	{
			for(k=0;k<x_size;k++)	{
				INT32  tbl_no;	//	道路密度データの配列位置
				INT32  link_cnt;
				INT32  range;


				if(block_f)	{	//	矩形の形状	０：横長	１：縦長
					range = block_range;
				}
				else	{
					range = layer_range;
				}
				tbl_no = (alt_y + j) * range + (alt_x + k);

				if((density_area_ptr->x * density_area_ptr->y) <= tbl_no)	{	//	テーブルサイズを超えた？
					SC_LOG_ErrorPrint(SC_TAG_RC,"@@@@@@@@@  テーブルサイズを超えた？？？？？  j...%d  k... %d  read_block_no...%d  read_layer_no...%d  tbl_no...%d  x...%d  y...%d"HERE,
							j, k, (alt_y + j), (alt_x + k),tbl_no, density_area_ptr->x, density_area_ptr->y	);
					continue;
				}

				link_cnt = density_area_ptr->data[tbl_no].density;
				//	ここでT_ParcelList情報を設定する。
				parcellist_ptr 	= aLevel->topArea.divArea_p->parcel_p 	+ parcel_cnt;

				parcellist_ptr->linkDensity = (UINT8)link_cnt;
				parcellist_ptr->join_f		= 0x00;
				parcellist_ptr->split_f		= 0x00;

				(T_RC_DivAreaInf_ptr+i)->link_cnt += (parcellist_ptr->linkDensity * 256);

//					if(log_dsp_f)	SC_LOG_DebugPrint(SC_TAG_RC,"@@@@@@@@@  j...%d  k... %d  read_block_no...%d  read_layer_no...%d  tbl_no...%d  link_cnt...%d linkDensity...%d  parcellist_ptr...%x\n",
//												j, k, (alt_y + j), (alt_x + k),tbl_no, link_cnt, parcellist_ptr->linkDensity, parcellist_ptr	);

				parcel_cnt++;				//	T_ParcelListテーブルのカウントアップ

			}
		}

	}


	//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	//		分割探索エリア管理情報」の処理優先度を決める
	//	パーセル相対位置の算出
	INT32	target_level = 1;	// 変換するﾚﾍﾞﾙ
	INT32 	O_alter_pos_x;		//	Ｘ方向の相対位置（出発地点）
	INT32 	O_alter_pos_y;		//	Ｙ方向の相対位置（出発地点）
	INT32 	D_alter_pos_x;		//	Ｘ方向の相対位置（目的地点）
	INT32 	D_alter_pos_y;		//	Ｙ方向の相対位置（目的地点）

	if( -1 == SC_MESH_GetAlterPos(
									baseParcelId,
									O_parcelID,
									target_level,
									&O_alter_pos_x,
									&O_alter_pos_y
								)
	)
	{
		return (e_SC_RESULT_FAIL);
	}

	if( -1 == SC_MESH_GetAlterPos(
									baseParcelId,
									D_parcelID,
									target_level,
									&D_alter_pos_x,
									&D_alter_pos_y
								)
	)
	{
		return (e_SC_RESULT_FAIL);
	}


	T_RC_DivAreaInf** divarea_ptr_ptr = (T_RC_DivAreaInf**)RP_MemAlloc(sizeof(T_RC_DivAreaInf*) * divarea_cnt, e_MEM_TYPE_ROUTEPLAN);

	if (NULL == divarea_ptr_ptr) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[AreaDiv] RP_MemAlloc divarea_ptr_ptr  error."HERE);
		return (e_SC_RESULT_MALLOC_ERR);
	}

	for(i=0;i<divarea_cnt;i++)	{
		*(divarea_ptr_ptr + i) = NULL;
	}
	//	分割探索エリアを処理順に並べ替える。
	if(e_SC_RESULT_FAIL == RC_AreaDiv_Set_OutWorkArea_by_Connection_f(
																		O_alter_pos_x,			//	Ｘ方向の相対位置（出発地点）
																		O_alter_pos_y,			//	Ｙ方向の相対位置（出発地点）
																		D_alter_pos_x,			//	Ｘ方向の相対位置（目的地点）
																		D_alter_pos_y,			//	Ｙ方向の相対位置（目的地点）
																		T_RC_DivAreaInf_ptr,	//	「分割探索エリア管理情報」
																		divarea_cnt,			//	「分割探索エリア管理情報」数
																		divarea_ptr_ptr			//	「分割探索エリア管理情報」アドレス格納テーブル
																	)
	)
	{
		//	「分割探索エリア管理情報」の解放
		RP_MemFree(divarea_ptr_ptr, e_MEM_TYPE_ROUTEPLAN);
		return (e_SC_RESULT_FAIL);
	}

#if		0
	for(i=0;i<divarea_cnt;i++)	{
		if(*(divarea_ptr_ptr+i) == NULL)	{
			SC_LOG_DebugPrint(SC_TAG_RC,(Char*)"@@@@   divarea_ptr_ptr == NULL   i...%d  \n",	i	);
		}
		else	{
			SC_LOG_DebugPrint(SC_TAG_RC,"@@@@   %d.  alt_x_for_o_pos...%d  alt_y_for_o_pos...%d  baseParcelId...%08x  alt_x...%d  alt_y...%d  parcel cnt...%d(x_size...%d  y_size...%d) proc_f...%d  link_cnt...%d \n",
								i,
								(*(divarea_ptr_ptr+i))->alt_x_for_o_pos,	// 	出発地点からの相対位置（ブロック方向）
								(*(divarea_ptr_ptr+i))->alt_y_for_o_pos,	// 	出発地点からの相対位置（レイヤー方向）
								(*(divarea_ptr_ptr+i))->baseParcelId,		//	分割エリア左下絶対パーセルＩＤ
								(*(divarea_ptr_ptr+i))->alt_x,				// 	非分割エリア内の相対位置（Ｘ方向）
								(*(divarea_ptr_ptr+i))->alt_y,				// 	非分割エリア内の相対位置（Ｙ方向）
								(*(divarea_ptr_ptr+i))->x_size * (*(divarea_ptr_ptr+i))->y_size,
								(*(divarea_ptr_ptr+i))->x_size,				// 	分割エリア内パーセル枚数（Ｘ方向）
								(*(divarea_ptr_ptr+i))->y_size,				// 	分割エリア内パーセル枚数（Ｙ方向）
								(*(divarea_ptr_ptr+i))->proc_f,				//	処理済みフラグ
								(*(divarea_ptr_ptr+i))->link_cnt
						);

		}
	}
#endif

	//		「接続有無フラグ」の設定
	RC_AreaDiv_Set_Connect_Flag_f(
									block_delt,				//	ブロック増減値
									layer_delt,				//	階層増減値
									block_f,				//	矩形の形状	０：横長	１：縦長
									divarea_cnt,			//	「分割探索エリア管理情報」数
									divarea_ptr_ptr			//	「分割探索エリア管理情報」アドレス格納テーブル
			);

	T_RC_DivAreaInf** out_divarea_ptr_ptr = (T_RC_DivAreaInf**) RP_MemAlloc(sizeof(T_RC_DivAreaInf*) * divarea_cnt, e_MEM_TYPE_ROUTEPLAN);
	RP_Memset0(out_divarea_ptr_ptr, sizeof(T_RC_DivAreaInf*) * divarea_cnt);

	if (NULL == out_divarea_ptr_ptr) {
		SC_LOG_ErrorPrint(SC_TAG_RC, "[AreaDiv] RP_MemAlloc out_divarea_ptr_ptr  error."HERE);
		RP_MemFree(divarea_ptr_ptr, e_MEM_TYPE_ROUTEPLAN);
		return (e_SC_RESULT_MALLOC_ERR);
	}

	int  out_divarea_cnt = 0;
	for(i=0;i<divarea_cnt;i++)	{
		if(*(divarea_ptr_ptr+i) == NULL)	{
			//SC_LOG_DebugPrint(SC_TAG_RC,(Char*)"@@@@   divarea_ptr_ptr == NULL   i...%d  \n",	i	);
			continue;
		}
		*(out_divarea_ptr_ptr+out_divarea_cnt) = *(divarea_ptr_ptr+i);
		out_divarea_cnt++;
	}

	//out_divarea_cnt = 1;

	aLevel->topArea.divAreaVol = out_divarea_cnt;

	//	ブロック数が１の時、領域を調整する。
	if(out_divarea_cnt == 1)	{
		E_SC_RESULT ret = RC_AreaDiv_Create_1_Block_Search_Area(
														aLevel,						//	トップレベル探索エリア
														baseParcelId,				//	非分割エリア左下絶対パーセルＩＤ（相対位置0:0の時）
														density_area_ptr,			//	道路密度データ取得関数の出力値
														aONbor,						//	出発地点
														aDNbor,						//	目的地点
														parcel_range_cnt_per_block,	//	分割探索エリアの一辺の最大パーセル数(DIV_NETWORK_MAX_PARCEL_RANGE_CNT)
														parcel_cnt_per_block,		//	ブロック当たりのパーセル読み込み数（DIV_NETWORK_PARCEL_CNT）
														max_parcel_cnt,				//	分割探索エリアの最大パーセル数
														max_link_cnt,				//	分割探索エリアの最大リンク本数
														O_alter_pos_x,				//	非分割エリ内Ｘ方向の相対位置（出発地点）
														O_alter_pos_y,				//	非分割エリ内Ｙ方向の相対位置（出発地点）
														D_alter_pos_x,				//	非分割エリ内Ｘ方向の相対位置（目的地点）
														D_alter_pos_y,				//	非分割エリ内Ｙ方向の相対位置（目的地点）
														(*(out_divarea_ptr_ptr+0))	//	分割探索エリア管理情報
													);
		if (ret != e_SC_RESULT_SUCCESS) {
			// 生成エリアがODを含んでいない為処理を続行する
			SC_LOG_DebugPrint(SC_TAG_RC, "Convert area cover od parcel." HERE);
#if 0
			SC_LOG_DebugPrint(SC_TAG_RC, (Char*) "+++++  e_SC_RESULT_SUCCESS != RC_AreaDiv_Create_1_Block_Search_Area  "HERE);
			//	「分割探索エリア管理情報」の解放
			RP_MemFree(divarea_ptr_ptr, e_MEM_TYPE_ROUTEPLAN);
			RP_MemFree(out_divarea_ptr_ptr, e_MEM_TYPE_ROUTEPLAN);
			RC_AreaDivFree_old(aLevel);

			return (e_SC_RESULT_FAIL);
#endif
		}
	}



	//	出力エリアに設定する
	T_DivAreaInfo* 	divArea_p		= aLevel->topArea.divArea_p;		//	T_DivAreaInfoテーブルを押さえる。

	for(i=0;i<out_divarea_cnt;i++)	{
		INT32  	workBaseParcelId	= (*(out_divarea_ptr_ptr+i))->baseParcelId;	//	分割エリア左下絶対パーセルＩＤ
		INT32	x_size 				= (*(out_divarea_ptr_ptr+i))->x_size;		// 	分割エリア内パーセル枚数（Ｘ方向）
		INT32	y_size 				= (*(out_divarea_ptr_ptr+i))->y_size;		// 	分割エリア内パーセル枚数（Ｙ方向）


		divArea_p->pclRect.parcelId = workBaseParcelId;						//	分割エリア左下絶対パーセルＩＤ
		divArea_p->pclRect.xSize 	= x_size;								// 	Ｘ方向枚数
		divArea_p->pclRect.ySize 	= y_size;								// 	Ｙ方向枚数
		divArea_p->parcelVol 	= divArea_p->pclRect.xSize * divArea_p->pclRect.ySize;	// 	パーセル情報数
		divArea_p->parcel_p		= (*(out_divarea_ptr_ptr+i))->parcel_p;		// 	パーセル情報先頭ポインタ

		//	「切断有無フラグ」の設定
		INT32  x_range, y_range;

		if(block_f)	{	//	矩形の形状	０：横長	１：縦長
			x_range = block_range;
			y_range = layer_range;
		}
		else	{
			x_range = layer_range;
			y_range = block_range;
		}

		if(out_divarea_cnt == 1)	{
			RC_AreaDiv_Set_DisConnect(
										(*(out_divarea_ptr_ptr+i))->alt_x,			// 	非分割エリア内の相対位置（Ｘ方向）
										(*(out_divarea_ptr_ptr+i))->alt_y,			// 	非分割エリア内の相対位置（Ｙ方向）
										x_size,										// 	分割エリア内パーセル枚数（Ｘ方向）
										y_size,										// 	分割エリア内パーセル枚数（Ｙ方向）
										downloadarea_ptr,							//	ダウンロードエリア
										density_area_ptr,							//	道路密度データ取得関数の出力値
										x_range,									//	データ幅
										y_range,									//	データ高
										(*(out_divarea_ptr_ptr+i))->alt_x,			//	収納範囲をチェックする位置・最小（Ｘ方向）
										(*(out_divarea_ptr_ptr+i))->alt_y,			//	収納範囲をチェックする位置・最小（Ｙ方向）
										(*(out_divarea_ptr_ptr+i))->alt_x + x_size,	//	収納範囲をチェックする位置・最大（Ｘ方向）
										(*(out_divarea_ptr_ptr+i))->alt_y + y_size,	//	収納範囲をチェックする位置・最大（Ｙ方向）
										(*(out_divarea_ptr_ptr+i))->parcel_p		//	T_ParcelList テーブル
									);
		}
		else	{
			RC_AreaDiv_Set_DisConnect(
										(*(out_divarea_ptr_ptr+i))->alt_x,			// 	非分割エリア内の相対位置（Ｘ方向）
										(*(out_divarea_ptr_ptr+i))->alt_y,			// 	非分割エリア内の相対位置（Ｙ方向）
										x_size,										// 	分割エリア内パーセル枚数（Ｘ方向）
										y_size,										// 	分割エリア内パーセル枚数（Ｙ方向）
										downloadarea_ptr,							//	ダウンロードエリア
										density_area_ptr,							//	道路密度データ取得関数の出力値
										x_range,									//	データ幅
										y_range,									//	データ高
										0,											//	収納範囲をチェックする位置・最小（Ｘ方向）
										0,											//	収納範囲をチェックする位置・最小（Ｙ方向）
										x_range - 1,								//	収納範囲をチェックする位置・最大（Ｘ方向）
										y_range - 1,								//	収納範囲をチェックする位置・最大（Ｙ方向）
										(*(out_divarea_ptr_ptr+i))->parcel_p		//	T_ParcelList テーブル
									);
		}



#if		0
		//if(log_dsp_f)
		{
			SCRP_AREAPCLSTATE*	parcellist_ptr = divArea_p->parcel_p;
			SC_LOG_DebugPrint(SC_TAG_RC,"@@@@   出力  %d.  alt_x_for_o_pos...%d  alt_y_for_o_pos...%d  baseParcelId...%08x  alt_x...%d  alt_y...%d  parcel cnt...%d(x_size...%d  y_size...%d) proc_f...%d  link_cnt...%d \n",
								i,
								(*(out_divarea_ptr_ptr+i))->alt_x_for_o_pos,	// 	出発地点からの相対位置（ブロック方向）
								(*(out_divarea_ptr_ptr+i))->alt_y_for_o_pos,	// 	出発地点からの相対位置（レイヤー方向）
								workBaseParcelId,								//	分割エリア左下絶対パーセルＩＤ
								(*(out_divarea_ptr_ptr+i))->alt_x,				// 	非分割エリア内の相対位置（Ｘ方向）
								(*(out_divarea_ptr_ptr+i))->alt_y,				// 	非分割エリア内の相対位置（Ｙ方向）
								x_size * y_size,
								x_size,											// 	分割エリア内パーセル枚数（Ｘ方向）
								y_size,											// 	分割エリア内パーセル枚数（Ｙ方向）
								(*(out_divarea_ptr_ptr+i))->proc_f,				//	処理済みフラグ
								(*(out_divarea_ptr_ptr+i))->link_cnt
						);


			INT32  m, n;
			for(m=0;m<y_size;m++)	{
				for(n=0;n<x_size;n++)	{
					INT32  tbl_no;	//	道路密度データの配列位置
					INT32  link_cnt;
					INT32  range;


					if(block_f)	{	//	矩形の形状	０：横長	１：縦長
						range = block_range;
					}
					else	{
						range = layer_range;
					}
					tbl_no = ((*(out_divarea_ptr_ptr+i))->alt_y + m) * range + ((*(out_divarea_ptr_ptr+i))->alt_x + n);

					if((density_area_ptr->x * density_area_ptr->y) <= tbl_no)	{	//	テーブルサイズを超えた？
//						if(log_dsp_f)	SC_LOG_DebugPrint(SC_TAG_RC,"@@@@@@@@@  テーブルサイズを超えた？？？？？  m...%d  n... %d  read_block_no...%d  read_layer_no...%d  tbl_no...%d  x...%d  y...%d",
//								m, n, (alt_y + m), (alt_x + n),tbl_no, density_area_ptr->x, density_area_ptr->y	);
						continue;
					}

					char  join_f_text[1024];
					char  split_f_text[1024];

					strcpy(join_f_text, "");
					strcpy(split_f_text, "");

					if((parcellist_ptr->join_f >> (1 - 1)) & 0x1)	strcat(join_f_text, 	"  L_D");
					else											strcat(join_f_text, 	"     ");

					if((parcellist_ptr->join_f >> (2 - 1)) & 0x1)	strcat(join_f_text, 	"    D");
					else											strcat(join_f_text, 	"     ");

					if((parcellist_ptr->join_f >> (3 - 1)) & 0x1)	strcat(join_f_text, 	"  R_D");
					else											strcat(join_f_text, 	"     ");

					if((parcellist_ptr->join_f >> (4 - 1)) & 0x1)	strcat(join_f_text, 	"  L  ");
					else											strcat(join_f_text, 	"     ");

					if((parcellist_ptr->join_f >> (5 - 1)) & 0x1)	strcat(join_f_text, 	"  R  ");
					else											strcat(join_f_text, 	"     ");

					if((parcellist_ptr->join_f >> (6 - 1)) & 0x1)	strcat(join_f_text, 	"  L_U");
					else											strcat(join_f_text, 	"     ");

					if((parcellist_ptr->join_f >> (7 - 1)) & 0x1)	strcat(join_f_text, 	"    U");
					else											strcat(join_f_text, 	"     ");

					if((parcellist_ptr->join_f >> (8 - 1)) & 0x1)	strcat(join_f_text, 	"  R_U");
					else											strcat(join_f_text, 	"     ");


					if((parcellist_ptr->split_f >> (1 - 1)) & 0x1)	strcat(split_f_text, 	"  L_D");
					else											strcat(split_f_text, 	"     ");

					if((parcellist_ptr->split_f >> (2 - 1)) & 0x1)	strcat(split_f_text, 	"    D");
					else											strcat(split_f_text, 	"     ");

					if((parcellist_ptr->split_f >> (3 - 1)) & 0x1)	strcat(split_f_text, 	"  R_D");
					else											strcat(split_f_text, 	"     ");

					if((parcellist_ptr->split_f >> (4 - 1)) & 0x1)	strcat(split_f_text, 	"  L  ");
					else											strcat(split_f_text, 	"     ");

					if((parcellist_ptr->split_f >> (5 - 1)) & 0x1)	strcat(split_f_text, 	"  R  ");
					else											strcat(split_f_text, 	"     ");

					if((parcellist_ptr->split_f >> (6 - 1)) & 0x1)	strcat(split_f_text, 	"  L_U");
					else											strcat(split_f_text, 	"     ");

					if((parcellist_ptr->split_f >> (7 - 1)) & 0x1)	strcat(split_f_text, 	"    U");
					else											strcat(split_f_text, 	"     ");

					if((parcellist_ptr->split_f >> (8 - 1)) & 0x1)	strcat(split_f_text, 	"  R_U");
					else											strcat(split_f_text, 	"     ");


					SC_LOG_DebugPrint(SC_TAG_RC,"@@@@@@@@@  m(y)...%d  n(x)... %d  join_f...0x%02x( %s )  split_f...0x%02x( %s )   read_block_no...%d  read_layer_no...%d  tbl_no...%d  linkDensity...%d  parcellist_ptr...%x \n",
												m, n,
												parcellist_ptr->join_f, join_f_text,
												parcellist_ptr->split_f, split_f_text,
												(*(out_divarea_ptr_ptr+i))->alt_y + m,
												(*(out_divarea_ptr_ptr+i))->alt_x + n,
												tbl_no,
												parcellist_ptr->linkDensity, parcellist_ptr	);

					parcellist_ptr++;				//	T_ParcelListテーブルのカウントアップ

				}
			}
		}
#endif

		divArea_p++;
	}

	//	「分割探索エリア管理情報」の解放
	RP_MemFree(divarea_ptr_ptr, 		e_MEM_TYPE_ROUTEPLAN);
	RP_MemFree(out_divarea_ptr_ptr, 	e_MEM_TYPE_ROUTEPLAN);

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);


	return (e_SC_RESULT_SUCCESS);

}


/**
 * @brief	出力用和0区エリアへの設定
 * @param	[I]Ｘ方向の相対位置（出発地点）
 * @param	[I]Ｙ方向の相対位置（出発地点）
 * @param	[I]Ｘ方向の相対位置（目的地点）
 * @param	[I]Ｙ方向の相対位置（目的地点）
 *
 * @param	[I]「分割探索エリア管理情報」
 * @param	[I]「分割探索エリア管理情報」数
 * @param	[IO]「分割探索エリア管理情報」アドレス格納テーブル
 */
E_SC_RESULT 	RC_AreaDiv_Set_OutWorkArea_by_Connection_f(
															INT32 					O_alter_pos_x,			//	Ｘ方向の相対位置（出発地点）
															INT32 					O_alter_pos_y,			//	Ｙ方向の相対位置（出発地点）
															INT32 					D_alter_pos_x,			//	Ｘ方向の相対位置（目的地点）
															INT32 					D_alter_pos_y,			//	Ｙ方向の相対位置（目的地点）
															T_RC_DivAreaInf*		T_RC_DivAreaInf_ptr,	//	「分割探索エリア管理情報」
															INT32					divarea_cnt,			//	「分割探索エリア管理情報」数
															T_RC_DivAreaInf** 		divarea_ptr_ptr			//	「分割探索エリア管理情報」アドレス格納テーブル
		)
{
	INT32	i, j;
	INT32  start_no = 0;

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	T_RC_DivAreaInf* w_RC_DivAreaInf_ptr = NULL;
	//	出発地点を含むブロックを検索
	INT32 O_T_RC_DivAreaInf_no = -1;


	for(i=0;i<divarea_cnt;i++)	{
		INT32	alt_x 			= (T_RC_DivAreaInf_ptr+i)->alt_x;				// 	非分割エリア内の相対位置（Ｘ方向）
		INT32	alt_y 			= (T_RC_DivAreaInf_ptr+i)->alt_y;				// 	非分割エリア内の相対位置（Ｙ方向）
		INT32	x_size 			= (T_RC_DivAreaInf_ptr+i)->x_size;				// 	分割エリア内パーセル枚数（Ｘ方向）
		INT32	y_size 			= (T_RC_DivAreaInf_ptr+i)->y_size;				// 	分割エリア内パーセル枚数（Ｙ方向）

		if(	O_alter_pos_x >= alt_x   &&  O_alter_pos_x < alt_x + x_size  &&
			O_alter_pos_y >= alt_y   &&  O_alter_pos_y < alt_y + y_size	)
		{
			if(O_T_RC_DivAreaInf_no == -1)	{	//	未登録の場合
				O_T_RC_DivAreaInf_no =  i;
			}
			else	{	//	既に候補ありの場合、出発地より一番遠いブロックを選択する。
				if(	((T_RC_DivAreaInf_ptr+i	)->alt_x_for_o_pos 	* (T_RC_DivAreaInf_ptr+i)->alt_x_for_o_pos	) +
					((T_RC_DivAreaInf_ptr+i	)->alt_y_for_o_pos 	* (T_RC_DivAreaInf_ptr+i)->alt_y_for_o_pos	)
					>
					((T_RC_DivAreaInf_ptr+O_T_RC_DivAreaInf_no	)->alt_x_for_o_pos 	* (T_RC_DivAreaInf_ptr+O_T_RC_DivAreaInf_no)->alt_x_for_o_pos	) +
					((T_RC_DivAreaInf_ptr+O_T_RC_DivAreaInf_no	)->alt_y_for_o_pos 	* (T_RC_DivAreaInf_ptr+O_T_RC_DivAreaInf_no)->alt_y_for_o_pos	)
				)
				{
					O_T_RC_DivAreaInf_no =  i;

				}
			}

		}

	}

	if(O_T_RC_DivAreaInf_no == -1)	{
		SC_LOG_DebugPrint(SC_TAG_RC,"@@@@   出発地点なし  alt_x...%d  _y...%d \n",	O_alter_pos_x, O_alter_pos_y	);
		return (e_SC_RESULT_FAIL);
	}

	w_RC_DivAreaInf_ptr = T_RC_DivAreaInf_ptr + O_T_RC_DivAreaInf_no;

	w_RC_DivAreaInf_ptr->proc_f 	= 1;		//	処理済みフラグ	０：未処理	１：処理中	２：処理済み
	w_RC_DivAreaInf_ptr->proc_no 	= start_no;	//	処理順番（０～）
	divarea_ptr_ptr[start_no] = w_RC_DivAreaInf_ptr;
	start_no++;



	//	目的地点を含むブロックを検索
	INT32 D_T_RC_DivAreaInf_no = -1;

	for(i=0;i<divarea_cnt;i++)	{
		INT32	alt_x 			= (T_RC_DivAreaInf_ptr+i)->alt_x;				// 	非分割エリア内の相対位置（Ｘ方向）
		INT32	alt_y 			= (T_RC_DivAreaInf_ptr+i)->alt_y;				// 	非分割エリア内の相対位置（Ｙ方向）
		INT32	x_size 			= (T_RC_DivAreaInf_ptr+i)->x_size;				// 	分割エリア内パーセル枚数（Ｘ方向）
		INT32	y_size 			= (T_RC_DivAreaInf_ptr+i)->y_size;				// 	分割エリア内パーセル枚数（Ｙ方向）

		if(	D_alter_pos_x >= alt_x   &&  D_alter_pos_x < alt_x + x_size  &&
			D_alter_pos_y >= alt_y   &&  D_alter_pos_y < alt_y + y_size	)
		{
			if(D_T_RC_DivAreaInf_no == -1)	{	//	未登録の場合
				D_T_RC_DivAreaInf_no =  i;
			}
			else	{	//	既に候補ありの場合、出発地より一番遠いブロックを選択する。
				if(	((T_RC_DivAreaInf_ptr+i	)->alt_x_for_o_pos 	* (T_RC_DivAreaInf_ptr+i)->alt_x_for_o_pos	) +
					((T_RC_DivAreaInf_ptr+i	)->alt_y_for_o_pos 	* (T_RC_DivAreaInf_ptr+i)->alt_y_for_o_pos	)
					>
					((T_RC_DivAreaInf_ptr+D_T_RC_DivAreaInf_no	)->alt_x_for_o_pos 	* (T_RC_DivAreaInf_ptr+D_T_RC_DivAreaInf_no)->alt_x_for_o_pos	) +
					((T_RC_DivAreaInf_ptr+D_T_RC_DivAreaInf_no	)->alt_y_for_o_pos 	* (T_RC_DivAreaInf_ptr+D_T_RC_DivAreaInf_no)->alt_y_for_o_pos	)
				)
				{
					D_T_RC_DivAreaInf_no =  i;

				}
			}

		}
	}

	if(D_T_RC_DivAreaInf_no == -1)	{
		SC_LOG_DebugPrint(SC_TAG_RC,"@@@@   目的地点なし    alt_x...%d  _y...%d \n",	D_alter_pos_x, D_alter_pos_y	);
		return (e_SC_RESULT_FAIL);
	}

	w_RC_DivAreaInf_ptr = T_RC_DivAreaInf_ptr + D_T_RC_DivAreaInf_no;
	w_RC_DivAreaInf_ptr->proc_f 	= 2;				//	処理済みフラグ	０：未処理	１：処理中	２：処理済み
	//	出発地点を含むブロックと目的地点を含むブロックが違う時
	if(O_T_RC_DivAreaInf_no != D_T_RC_DivAreaInf_no)	{
		w_RC_DivAreaInf_ptr->proc_no 	= divarea_cnt - 1;	//	処理順番（０～）
		divarea_ptr_ptr[divarea_cnt - 1] = w_RC_DivAreaInf_ptr;
	}


	//		隣接するパーセルを追跡する。
	while(1)	{
		INT32  chk_f = 0;
		for(i=0;i<divarea_cnt;i++)	{
			if((T_RC_DivAreaInf_ptr+i)->proc_f != 1)	continue;	//	処理済みフラグ	０：未処理	１：処理中	２：処理済み

			for(j=0;j<divarea_cnt;j++)	{
				if((T_RC_DivAreaInf_ptr+j)->proc_f != 0)	continue;	//	処理済みフラグ	０：未処理	１：処理中	２：処理済み

				chk_f = 1;
				//	隣接するパーセルを探し、処理順番を付加する。
				//	相対位置を求める。
				INT32  alt_x = (T_RC_DivAreaInf_ptr+j)->alt_x_for_o_pos - (T_RC_DivAreaInf_ptr+i)->alt_x_for_o_pos;
				INT32  alt_y = (T_RC_DivAreaInf_ptr+j)->alt_y_for_o_pos - (T_RC_DivAreaInf_ptr+i)->alt_y_for_o_pos;

				if(	abs(alt_x) > 1 )	continue;
				if( abs(alt_y) > 1 )	continue;
				if( alt_x == 0  &&  alt_y == 0)	{
					SC_LOG_DebugPrint(SC_TAG_RC,"@@@@   同じ位置にいる？？？    i...%d  j...%d \n",	i,	j	);
					continue;
				}
				if(j == D_T_RC_DivAreaInf_no)	{	//	隣接するブロックが目的地だった時
					(T_RC_DivAreaInf_ptr+j)->proc_f 	= 2;		//	処理済みフラグ	０：未処理	１：処理中	２：処理済み
				}
				else	{
					(T_RC_DivAreaInf_ptr+j)->proc_f 	= 1;		//	処理済みフラグ	０：未処理	１：処理中	２：処理済み
				}

				(T_RC_DivAreaInf_ptr+j)->proc_no 	= start_no;	//	処理順番（０～）
				divarea_ptr_ptr[start_no] = T_RC_DivAreaInf_ptr+j;

				start_no++;

			}
			(T_RC_DivAreaInf_ptr+i)->proc_f 	= 2;		//	処理済みフラグ	０：未処理	１：処理中	２：処理済み

		}
		if(chk_f == 0)	//	処理対象が存在しない時
			break;

	}


	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);

	return (e_SC_RESULT_SUCCESS);

}
/**
 * @brief	出発地点・目的地点を含む１ブロックの分割探索エリアの設定
 * @param	[IO] トップレベル探索エリア
 * @param	[I] 非分割エリア左下絶対パーセルＩＤ（相対位置0:0の時）
 * @param	[I] 分割探索エリアの一辺の最大パーセル数(DIV_NETWORK_MAX_PARCEL_RANGE_CNT)
 * @param	[I] ブロック当たりのパーセル読み込み数（DIV_NETWORK_PARCEL_CNT）
 * @param	[I] 分割探索エリアの最大パーセル数
 * @param	[I] 分割探索エリアの最大リンク本数
 * @param	[I] Ｘ方向の相対位置（出発地点）
 * @param	[I] Ｙ方向の相対位置（出発地点）
 * @param	[I] Ｘ方向の相対位置（目的地点）
 * @param	[I] Ｙ方向の相対位置（目的地点）
 * @return e_SC_RESULT_SUCCESS エリア生成完了
 *         e_SC_RESULT_FAIL 生成エリアが不正
 */
E_SC_RESULT 	RC_AreaDiv_Create_1_Block_Search_Area(
					SCRP_LEVELTBL_OLD* 		aLevel,						//	トップレベル探索エリア
					INT32  					baseParcelId,				//	非分割エリア左下絶対パーセルＩＤ（相対位置0:0の時）
					T_DHC_ROAD_DENSITY*  	density_area_ptr,			//	道路密度データ取得関数の出力値
					SCRP_NEIGHBORINFO* 			aONbor,						//	出発地点
					SCRP_NEIGHBORINFO* 			aDNbor,						//	目的地点
					INT32					parcel_range_cnt_per_block,	//	分割探索エリアの一辺の最大パーセル数(DIV_NETWORK_MAX_PARCEL_RANGE_CNT)
					INT32  					parcel_cnt_per_block,		//	ブロック当たりのパーセル読み込み数（DIV_NETWORK_PARCEL_CNT）
					INT32  					max_parcel_cnt,				//	分割探索エリアの最大パーセル数
					INT32  					max_link_cnt,				//	分割探索エリアの最大リンク本数
					INT32 					O_alter_pos_x,				//	非分割エリ内Ｘ方向の相対位置（出発地点）
					INT32 					O_alter_pos_y,				//	非分割エリ内Ｙ方向の相対位置（出発地点）
					INT32 					D_alter_pos_x,				//	非分割エリ内Ｘ方向の相対位置（目的地点）
					INT32 					D_alter_pos_y,				//	非分割エリ内Ｙ方向の相対位置（目的地点）
					T_RC_DivAreaInf*		T_RC_DivAreaInf_ptr			//	「分割探索エリア管理情報」

				)
{
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);
	INT32  s_x, e_x, s_y, e_y;

	if (O_alter_pos_x > D_alter_pos_x) {
		s_x = D_alter_pos_x;
		e_x = O_alter_pos_x;
	} else {
		s_x = O_alter_pos_x;
		e_x = D_alter_pos_x;
	}
	if (O_alter_pos_y > D_alter_pos_y) {
		s_y = D_alter_pos_y;
		e_y = O_alter_pos_y;
	} else {
		s_y = O_alter_pos_y;
		e_y = D_alter_pos_y;
	}

	INT32 delta = 0;
	INT32 x, y;
	//INT32 w_x = 0;
	//INT32 w_y = 0;
	INT32 total_lnk_cnt = 0;
	INT32 total_parcel_cnt = 0;
	INT32 w_s_x = 0;
	INT32 w_s_y = 0;
	INT32 w_e_x = 0;
	INT32 w_e_y = 0;
	INT32 range_out_x = 0;
	INT32 range_out_y = 0;

	while (1) {
		total_lnk_cnt = 0;
		total_parcel_cnt = 0;

		//	出発地点＆目的地点を含む矩形領域を±１ずつ拡張して、上限値（リンク本数、パーセル数）をチェックする。

		if (range_out_x == 0) {
			w_s_x = s_x - delta;
			w_e_x = e_x + delta;
			if ((w_e_x - w_s_x + 1) > parcel_range_cnt_per_block) {
				range_out_x = 1;
				w_s_x++;
				w_e_x--;
			}
			if (w_s_x < 0)
				w_s_x = 0;
			if (w_e_x >= aLevel->topArea.pclRect.xSize)
				w_e_x = aLevel->topArea.pclRect.xSize - 1;
		}

		if (range_out_y == 0) {
			w_s_y = s_y - delta;
			w_e_y = e_y + delta;
			if ((w_e_y - w_s_y + 1) > parcel_range_cnt_per_block) {
				range_out_y = 1;
				w_s_y++;
				w_e_y--;
			}
			if (w_s_y < 0)
				w_s_y = 0;
			if (w_e_y >= aLevel->topArea.pclRect.ySize)
				w_e_y = aLevel->topArea.pclRect.ySize - 1;
		}

		if (range_out_x == 1 && range_out_y == 1)
			break;

		for (x = w_s_x; x <= w_e_x; x++) {
			for (y = w_s_y; y <= w_e_y; y++) {
				INT32 tbl_no;	//	道路密度データの配列位置

				tbl_no = aLevel->topArea.pclRect.xSize * y + x;
				total_lnk_cnt += density_area_ptr->data[tbl_no].density * 256;

				total_parcel_cnt++;
			}
		}

		if (total_lnk_cnt > max_link_cnt) {
			w_s_x++;
			w_e_x--;
			w_s_y++;
			w_e_y--;
			break;
		}
		if (total_parcel_cnt > max_parcel_cnt) {
			w_s_x++;
			w_e_x--;
			w_s_y++;
			w_e_y--;
			break;
		}
		if (delta > parcel_range_cnt_per_block / 2)
			break;

		delta++;
	}

	INT32 range_x = w_e_x - w_s_x + 1;
	INT32 range_y = w_e_y - w_s_y + 1;
	// 左下パーセルID算出
	INT32 wbaseParcelId = SC_MESH_SftParcelId(aLevel->topArea.pclRect.parcelId, w_s_x, w_s_y);

#if 1 // 2015/08/19
	// 右上パーセル取得（rangeは枚数の為 -1）
	UINT32 rt = SC_MESH_SftParcelId(wbaseParcelId, range_x - 1, range_y - 1);

	// 出発地側、目的地側確認 不正エリアの場合e_SC_RESULT_BADPARAM通知
	if (!checkCoverAreaNbrLinks(wbaseParcelId, rt, aONbor)) {
		SC_LOG_DebugPrint(SC_TAG_RC, "mono area is not cover oSide...");
		return (e_SC_RESULT_FAIL);
	}
	if (!checkCoverAreaNbrLinks(wbaseParcelId, rt, aDNbor)) {
		SC_LOG_DebugPrint(SC_TAG_RC, "mono area is not cover dSide...");
		return (e_SC_RESULT_FAIL);
	}
#endif

	// トップレベルのT_AreaInfoテーブルに設定する。
	//INT32 divarea_cnt = 1;
	//INT32 T_ParcelList_cnt = range_x * range_y;

	T_RC_DivAreaInf_ptr->alt_x_for_o_pos = 0;			// 出発地点からの相対位置（ブロック方向）
	T_RC_DivAreaInf_ptr->alt_y_for_o_pos = 0;			// 出発地点からの相対位置（レイヤー方向）
	T_RC_DivAreaInf_ptr->baseParcelId = wbaseParcelId;	// 分割エリア左下絶対パーセルＩＤ

	T_RC_DivAreaInf_ptr->alt_x = w_s_x;					// 非分割エリア内の相対位置（Ｘ方向）
	T_RC_DivAreaInf_ptr->alt_y = w_s_y;					// 非分割エリア内の相対位置（Ｙ方向）

	T_RC_DivAreaInf_ptr->x_size = range_x;				// 分割エリア内パーセル枚数（Ｘ方向）
	T_RC_DivAreaInf_ptr->y_size = range_y;				// 分割エリア内パーセル枚数（Ｙ方向）
	T_RC_DivAreaInf_ptr->proc_f = 0;					// 処理済みフラグ ０：未処理 １：処理中 ２：処理済み

	T_RC_DivAreaInf_ptr->proc_no = 1;					// 処理順番（１～）
	T_RC_DivAreaInf_ptr->link_cnt = 0;					// リンク本数

	T_RC_DivAreaInf_ptr->parcel_p = aLevel->topArea.divArea_p->parcel_p;		// パーセル情報先頭ポインタ

	INT32 parcel_cnt = 0;
	SCRP_AREAPCLSTATE* parcellist_ptr = NULL;

	INT32 j, k;
	for (j = 0; j < range_y; j++) {
		for (k = 0; k < range_x; k++) {
			INT32 tbl_no;	//	道路密度データの配列位置
			INT32 link_cnt;
			//INT32 range;

			tbl_no = (w_s_y + j) * range_x + (w_s_x + k);

			if ((density_area_ptr->x * density_area_ptr->y) <= tbl_no) {	//	テーブルサイズを超えた？
				SC_LOG_ErrorPrint(SC_TAG_RC, "table size over flow. j=%d k=%d x=%d y=%d"HERE, j, k, tbl_no, density_area_ptr->x,
						density_area_ptr->y);
				continue;
			}

			link_cnt = density_area_ptr->data[tbl_no].density;
			//	ここでT_ParcelList情報を設定する。
			parcellist_ptr = aLevel->topArea.divArea_p->parcel_p + parcel_cnt;

			parcellist_ptr->linkDensity = (UINT8) link_cnt;
			parcellist_ptr->join_f = 0x00;
			parcellist_ptr->split_f = 0x00;

			T_RC_DivAreaInf_ptr->link_cnt += (parcellist_ptr->linkDensity * 256);

			// T_ParcelListテーブルのカウントアップ
			parcel_cnt++;
		}
	}

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief	「接続有無フラグ」の設定
 * @param	[I] ブロック増減値
 * @param	[I] 階層増減値
 * @param	[I] 矩形の形状	０：横長	１：縦長
 * @param	[I]「分割探索エリア管理情報」数
 * @param	[IO]「分割探索エリア管理情報」アドレス格納テーブル
 */
void 	RC_AreaDiv_Set_Connect_Flag_f(
										INT32					block_delt,				//	ブロック増減値
										INT32					layer_delt,				//	階層増減値
										INT32					block_f,				//	矩形の形状	０：横長	１：縦長
										INT32					divarea_cnt,			//	「分割探索エリア管理情報」数
										T_RC_DivAreaInf** 		divarea_ptr_ptr			//	「分割探索エリア管理情報」アドレス格納テーブル
		)
{
	INT32  i, j;

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	for(i=0;i<divarea_cnt - 1;i++)	{	//	最後の分割エリア（＝目的地）については処理しない
		if(*(divarea_ptr_ptr+i) == NULL)	continue;
		INT32	x_size 			= (*(divarea_ptr_ptr+i))->x_size;				// 	分割エリア内パーセル枚数（Ｘ方向）
		INT32	y_size 			= (*(divarea_ptr_ptr+i))->y_size;				// 	分割エリア内パーセル枚数（Ｙ方向）
		for(j=i+1;j<divarea_cnt;j++)	{//	自「分割エリア」より若い分割エリアについては処理しない
			if(*(divarea_ptr_ptr+j) == NULL)	continue;
			//	隣接する分割エリアを探す。
			//	相対位置を求める。
			INT32  alt_x = (*(divarea_ptr_ptr+j))->alt_x_for_o_pos - (*(divarea_ptr_ptr+i))->alt_x_for_o_pos;
			INT32  alt_y = (*(divarea_ptr_ptr+j))->alt_y_for_o_pos - (*(divarea_ptr_ptr+i))->alt_y_for_o_pos;

			if(	abs(alt_x) > 1 )	continue;
			if( abs(alt_y) > 1 )	continue;
			if( alt_x == 0  &&  alt_y == 0)	{
				continue;
			}

			INT32  alt_X, alt_Y;
			if(block_f)	{	//	矩形の形状	０：横長	１：縦長
				alt_Y = alt_y * layer_delt;
				alt_X = alt_x * block_delt;
			}
			else	{
				alt_Y = alt_x * block_delt;
				alt_X = alt_y * layer_delt;
			}

			//	「接続有無フラグ」の設定
			RC_AreaDiv_Set_Connect_f(
										alt_X,								// 	自パーセルに対する他方のパーセル相対位置（Ｘ方向）
										alt_Y,								// 	自パーセルに対する他方のパーセル相対位置（Ｙ方向）
										x_size,								// 	分割エリア内パーセル枚数（Ｘ方向）
										y_size,								// 	分割エリア内パーセル枚数（Ｙ方向）
										(*(divarea_ptr_ptr+i))->parcel_p	//	T_ParcelList テーブル
									);


		}

	}


	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);

}



/**
 * @brief	「接続有無フラグ」の設定
 * @param	[I]自パーセルに対する他方のパーセル相対位置（Ｘ方向）
 * @param	[I]自パーセルに対する他方のパーセル相対位置（Ｙ方向）
 * @param	[I]	分割エリア内パーセル枚数（Ｘ方向）
 * @param	[I]	分割エリア内パーセル枚数（Ｙ方向）
 * @param	[IO]T_ParcelList テーブル
 */
void 	RC_AreaDiv_Set_Connect_f(
									INT32			alt_x,				// 	自パーセルに対する他方のパーセル相対位置（Ｘ方向）
									INT32			alt_y,				// 	自パーセルに対する他方のパーセル相対位置（Ｙ方向）
									INT32			x_size,				// 	分割エリア内パーセル枚数（Ｘ方向）
									INT32			y_size,				// 	分割エリア内パーセル枚数（Ｙ方向）
									SCRP_AREAPCLSTATE*	T_ParcelList_ptr	//	T_ParcelList テーブル
		)
{
	SCRP_AREAPCLSTATE*	w_ptr = T_ParcelList_ptr;
	INT32  m, n;


	//	「接続有無フラグ」の設定

	if(alt_x ==  0  &&  alt_y ==  1)	{	//	  上に接続している
		for(m=0;m<y_size;m++)	{
			for(n=0;n<x_size;n++)	{
				if(m != y_size - 1 )	{
					w_ptr++;
					continue;
				}
				if(n==0)	{
					w_ptr->join_f |= (0x1<<(7 - 1));
					w_ptr->join_f |= (0x1<<(8 - 1));
				}
				else	{
					if(n==x_size - 1)	{
						w_ptr->join_f |= (0x1<<(6 - 1));
						w_ptr->join_f |= (0x1<<(7 - 1));
					}	else	{
						w_ptr->join_f |= (0x1<<(6 - 1));
						w_ptr->join_f |= (0x1<<(7 - 1));
						w_ptr->join_f |= (0x1<<(8 - 1));
					}
				}
				w_ptr++;
			}
		}
	}
	if(alt_x ==  1  &&  alt_y ==  1)	{	//	右上に接続している
		for(m=0;m<y_size;m++)	{
			for(n=0;n<x_size;n++)	{
				if(m != y_size - 1 )	{
					w_ptr++;
					continue;
				}
				if(n != x_size - 1 )	{
					w_ptr++;
					continue;
				}
				w_ptr->join_f |= (0x1<<(8 - 1));
				w_ptr++;
			}
		}
	}
	if(alt_x ==  1  &&  alt_y ==  0)	{	//	右  に接続している
		for(m=0;m<y_size;m++)	{
			for(n=0;n<x_size;n++)	{
				if(n != x_size - 1  )	{
					w_ptr++;
					continue;
				}
				if(m==0)	{
					w_ptr->join_f |= (0x1<<(5 - 1));
					w_ptr->join_f |= (0x1<<(8 - 1));
				}
				else	{
					if(m == y_size -1)	{
						w_ptr->join_f |= (0x1<<(3 - 1));
						w_ptr->join_f |= (0x1<<(5 - 1));
					}	else	{
						w_ptr->join_f |= (0x1<<(3 - 1));
						w_ptr->join_f |= (0x1<<(5 - 1));
						w_ptr->join_f |= (0x1<<(8 - 1));
					}
				}
				w_ptr++;
			}
		}
	}
	if(alt_x ==  1  &&  alt_y == -1)	{	//	右下に接続している
		for(m=0;m<y_size;m++)	{
			for(n=0;n<x_size;n++)	{
				if(m != 0 )	{
					w_ptr++;
					continue;
				}
				if(n != x_size - 1 )	{
					w_ptr++;
					continue;
				}
				w_ptr->join_f |= (0x1<<(3 - 1));
				w_ptr++;
			}
		}
	}
	if(alt_x ==  0  &&  alt_y == -1)	{	//	  下に接続している
		for(m=0;m<y_size;m++)	{
			for(n=0;n<x_size;n++)	{
				if(m != 0 )	{
					w_ptr++;
					continue;
				}
				if(n==0)	{
					w_ptr->join_f |= (0x1<<(2 - 1));
					w_ptr->join_f |= (0x1<<(3 - 1));
				}
				else	{
					if(n==x_size - 1)	{
						w_ptr->join_f |= (0x1<<(1 - 1));
						w_ptr->join_f |= (0x1<<(2 - 1));
					}	else	{
						w_ptr->join_f |= (0x1<<(1 - 1));
						w_ptr->join_f |= (0x1<<(2 - 1));
						w_ptr->join_f |= (0x1<<(3 - 1));
					}
				}
				w_ptr++;
			}
		}
	}
	if(alt_x == -1  &&  alt_y == -1)	{	//	左下に接続している
		for(m=0;m<y_size;m++)	{
			for(n=0;n<x_size;n++)	{
				if(m != 0 )	{
					w_ptr++;
					continue;
				}
				if(n != 0 )	{
					w_ptr++;
					continue;
				}
				w_ptr->join_f |= (0x1<<(1 - 1));
				w_ptr++;
			}
		}
	}
	if(alt_x == -1  &&  alt_y ==  0)	{	//	左  に接続している
		for(m=0;m<y_size;m++)	{
			for(n=0;n<x_size;n++)	{
				if(n != 0 )	{
					w_ptr++;
					continue;
				}
				if(m==0)	{
					w_ptr->join_f |= (0x1<<(4 - 1));
					w_ptr->join_f |= (0x1<<(6 - 1));
				}
				else	{
					if(m == y_size -1)	{
						w_ptr->join_f |= (0x1<<(1 - 1));
						w_ptr->join_f |= (0x1<<(4 - 1));
					}	else	{
						w_ptr->join_f |= (0x1<<(1 - 1));
						w_ptr->join_f |= (0x1<<(4 - 1));
						w_ptr->join_f |= (0x1<<(6 - 1));
					}
				}
				w_ptr++;
			}
		}
	}
	if(alt_x == -1  &&  alt_y ==  1)	{	//	左上に接続している
		for(m=0;m<y_size;m++)	{
			for(n=0;n<x_size;n++)	{
				if(m != y_size - 1 )	{
					w_ptr++;
					continue;
				}
				if(n != 0 )	{
					w_ptr++;
					continue;
				}
				w_ptr->join_f |= (0x1<<(6 - 1));
				w_ptr++;
			}
		}
	}


}
/**
 * @brief	「切断有無フラグ」の設定（矩形以内）
 * @param	[I]自パーセルに対する他方のパーセル相対位置（Ｘ方向）
 * @param	[I]自パーセルに対する他方のパーセル相対位置（Ｙ方向）
 * @param	[I]	分割エリア内パーセル枚数（Ｘ方向）
 * @param	[I]	分割エリア内パーセル枚数（Ｙ方向）
 * @param	[I]	ダウンロードされているエリアID列
 * @param	[I]	ダウンロードされているエリアID列の有効テーブル数
 * @param	[I]	道路密度データ取得関数の出力値
 * @param	[I]	道路密度データの配列位置
 * @param	[IO]T_ParcelList テーブル
 */
void 	RC_AreaDiv_Set_DisConnect(
										INT32					ALT_X,						// 	非分割エリア内の相対位置（Ｘ方向）
										INT32					ALT_Y,						// 	非分割エリア内の相対位置（Ｙ方向）
										INT32					x_size,						// 	分割エリア内パーセル枚数（Ｘ方向）
										INT32					y_size,						// 	分割エリア内パーセル枚数（Ｙ方向）
										T_DHC_DOWNLOAD_AREA*  	downloadarea_ptr,			//	ダウンロードエリア
										T_DHC_ROAD_DENSITY*  	density_area_ptr,			//	道路密度データ取得関数の出力値
										INT32  					x_range,					//	データ幅
										INT32  					y_range,					//	データ高
										INT32					L_x,						//	収納範囲をチェックする位置・最小（Ｘ方向）
										INT32					L_y,						//	収納範囲をチェックする位置・最小（Ｙ方向）
										INT32					H_x,						//	収納範囲をチェックする位置・最大（Ｘ方向）
										INT32					H_y,						//	収納範囲をチェックする位置・最大（Ｙ方向）
										SCRP_AREAPCLSTATE*			T_ParcelList_ptr			//	T_ParcelList テーブル
		)
{
	//	ダウンロードされているかチェックする。
	//INT32 OK_f = 0;
	//INT32  k;
	INT32  m, n;
	SCRP_AREAPCLSTATE*	w_ptr = T_ParcelList_ptr;
	//INT32  tbl_no;



	for(m=0;m<y_size;m++)	{
		for(n=0;n<x_size;n++)	{
			//	  上に接続
			if(e_SC_RESULT_FAIL == RC_AreaDiv_CheckDownload(
																downloadarea_ptr,			//	ダウンロードエリア
																density_area_ptr,			//	道路密度データ取得関数の出力値
																x_range,					//	データ幅（非分割）
																y_range,					//	データ高（非分割）
																L_x,						//	収納範囲をチェックする位置・最小（Ｘ方向）
																L_y,						//	収納範囲をチェックする位置・最小（Ｙ方向）
																H_x,						//	収納範囲をチェックする位置・最大（Ｘ方向）
																H_y,						//	収納範囲をチェックする位置・最大（Ｙ方向）
																(ALT_X + n),				//	Ｘ軸方向位置（非分割）
																(ALT_Y + m + 1)				//	Ｙ軸方向位置（非分割）
															)
			)
			{
				w_ptr->split_f |= (0x1<<(7 - 1));
			}

			//	右上に接続
			if(e_SC_RESULT_FAIL == RC_AreaDiv_CheckDownload(
																downloadarea_ptr,			//	ダウンロードエリア
																density_area_ptr,			//	道路密度データ取得関数の出力値
																x_range,					//	データ幅（非分割）
																y_range,					//	データ高（非分割）
																L_x,						//	収納範囲をチェックする位置・最小（Ｘ方向）
																L_y,						//	収納範囲をチェックする位置・最小（Ｙ方向）
																H_x,						//	収納範囲をチェックする位置・最大（Ｘ方向）
																H_y,						//	収納範囲をチェックする位置・最大（Ｙ方向）
																(ALT_X + n + 1),			//	Ｘ軸方向位置（非分割）
																(ALT_Y + m + 1)				//	Ｙ軸方向位置（非分割）
															)
			)
			{
				w_ptr->split_f |= (0x1<<(8 - 1));
			}

			//	右  に接続
			if(e_SC_RESULT_FAIL == RC_AreaDiv_CheckDownload(
																downloadarea_ptr,			//	ダウンロードエリア
																density_area_ptr,			//	道路密度データ取得関数の出力値
																x_range,					//	データ幅（非分割）
																y_range,					//	データ高（非分割）
																L_x,						//	収納範囲をチェックする位置・最小（Ｘ方向）
																L_y,						//	収納範囲をチェックする位置・最小（Ｙ方向）
																H_x,						//	収納範囲をチェックする位置・最大（Ｘ方向）
																H_y,						//	収納範囲をチェックする位置・最大（Ｙ方向）
																(ALT_X + n + 1),			//	Ｘ軸方向位置（非分割）
																(ALT_Y + m)					//	Ｙ軸方向位置（非分割）
														)
			)
			{
				w_ptr->split_f |= (0x1<<(5 - 1));
			}

			//	右下に接続
			if(e_SC_RESULT_FAIL == RC_AreaDiv_CheckDownload(
																downloadarea_ptr,			//	ダウンロードエリア
																density_area_ptr,			//	道路密度データ取得関数の出力値
																x_range,					//	データ幅（非分割）
																y_range,					//	データ高（非分割）
																L_x,						//	収納範囲をチェックする位置・最小（Ｘ方向）
																L_y,						//	収納範囲をチェックする位置・最小（Ｙ方向）
																H_x,						//	収納範囲をチェックする位置・最大（Ｘ方向）
																H_y,						//	収納範囲をチェックする位置・最大（Ｙ方向）
																(ALT_X + n + 1),			//	Ｘ軸方向位置（非分割）
																(ALT_Y + m - 1)				//	Ｙ軸方向位置（非分割）
														)
			)
			{
				w_ptr->split_f |= (0x1<<(3 - 1));
			}

			//	  下に接続
			if(e_SC_RESULT_FAIL == RC_AreaDiv_CheckDownload(
																downloadarea_ptr,			//	ダウンロードエリア
																density_area_ptr,			//	道路密度データ取得関数の出力値
																x_range,					//	データ幅（非分割）
																y_range,					//	データ高（非分割）
																L_x,						//	収納範囲をチェックする位置・最小（Ｘ方向）
																L_y,						//	収納範囲をチェックする位置・最小（Ｙ方向）
																H_x,						//	収納範囲をチェックする位置・最大（Ｘ方向）
																H_y,						//	収納範囲をチェックする位置・最大（Ｙ方向）
																(ALT_X + n),				//	Ｘ軸方向位置（非分割）
																(ALT_Y + m - 1)				//	Ｙ軸方向位置（非分割）
														)
			)
			{
				w_ptr->split_f |= (0x1<<(2 - 1));
			}

			//	左下に接続
			if(e_SC_RESULT_FAIL == RC_AreaDiv_CheckDownload(
																downloadarea_ptr,			//	ダウンロードエリア
																density_area_ptr,			//	道路密度データ取得関数の出力値
																x_range,					//	データ幅（非分割）
																y_range,					//	データ高（非分割）
																L_x,						//	収納範囲をチェックする位置・最小（Ｘ方向）
																L_y,						//	収納範囲をチェックする位置・最小（Ｙ方向）
																H_x,						//	収納範囲をチェックする位置・最大（Ｘ方向）
																H_y,						//	収納範囲をチェックする位置・最大（Ｙ方向）
																(ALT_X + n - 1),			//	Ｘ軸方向位置（非分割）
																(ALT_Y + m - 1)				//	Ｙ軸方向位置（非分割）
														)
			)
			{
				w_ptr->split_f |= (0x1<<(1 - 1));
			}

			//	左  に接続
			if(e_SC_RESULT_FAIL == RC_AreaDiv_CheckDownload(
																downloadarea_ptr,			//	ダウンロードエリア
																density_area_ptr,			//	道路密度データ取得関数の出力値
																x_range,					//	データ幅（非分割）
																y_range,					//	データ高（非分割）
																L_x,						//	収納範囲をチェックする位置・最小（Ｘ方向）
																L_y,						//	収納範囲をチェックする位置・最小（Ｙ方向）
																H_x,						//	収納範囲をチェックする位置・最大（Ｘ方向）
																H_y,						//	収納範囲をチェックする位置・最大（Ｙ方向）
																(ALT_X + n - 1),			//	Ｘ軸方向位置（非分割）
																(ALT_Y + m)					//	Ｙ軸方向位置（非分割）
														)
			)
			{
				w_ptr->split_f |= (0x1<<(4 - 1));
			}

			//	左上に接続している
			if(e_SC_RESULT_FAIL == RC_AreaDiv_CheckDownload(
																downloadarea_ptr,			//	ダウンロードエリア
																density_area_ptr,			//	道路密度データ取得関数の出力値
																x_range,					//	データ幅（非分割）
																y_range,					//	データ高（非分割）
																L_x,						//	収納範囲をチェックする位置・最小（Ｘ方向）
																L_y,						//	収納範囲をチェックする位置・最小（Ｙ方向）
																H_x,						//	収納範囲をチェックする位置・最大（Ｘ方向）
																H_y,						//	収納範囲をチェックする位置・最大（Ｙ方向）
																(ALT_X + n - 1),			//	Ｘ軸方向位置（非分割）
																(ALT_Y + m + 1)				//	Ｙ軸方向位置（非分割）
														)
			)
			{
				w_ptr->split_f |= (0x1<<(6 - 1));
			}



			w_ptr++;
		}
	}


}



/**
 * @brief	「ダウンロード」のチェック

 * @param	[I]	ダウンロードされているエリアID列
 * @param	[I]	ダウンロードされているエリアID列の有効テーブル数
 * @param	[I]	道路密度データ取得関数の出力値
 * @param	[I]	道路密度データの配列位置
 */
E_SC_RESULT 	RC_AreaDiv_CheckDownload(
										T_DHC_DOWNLOAD_AREA*  	downloadarea_ptr,			//	ダウンロードエリア
										T_DHC_ROAD_DENSITY*  	density_area_ptr,			//	道路密度データ取得関数の出力値
										INT32					x_range,					//	データ幅（非分割）
										INT32					y_range,					//	データ高（非分割）
										INT32					L_x,						//	収納範囲をチェックする位置・最小（Ｘ方向）
										INT32					L_y,						//	収納範囲をチェックする位置・最小（Ｙ方向）
										INT32					H_x,						//	収納範囲をチェックする位置・最大（Ｘ方向）
										INT32					H_y,						//	収納範囲をチェックする位置・最大（Ｙ方向）
										INT32					x_pos,						//	Ｘ軸方向位置（非分割）
										INT32					y_pos						//	Ｙ軸方向位置（非分割）

		)
{


	if(x_pos < L_x  || x_pos > H_x)	return (e_SC_RESULT_FAIL);
	if(y_pos < L_y  || y_pos > H_y)	return (e_SC_RESULT_FAIL);

	INT32	tbl_no = y_pos * x_range + x_pos;

	if(tbl_no >= (x_range * y_range)	)		return (e_SC_RESULT_FAIL);
	if(tbl_no < 0						)		return (e_SC_RESULT_FAIL);

	//	ダウンロードされているかチェックする。
	INT32  k;
	//INT32  m, n;

	for(k=0;k<8;k++)	{
		if(	density_area_ptr->data[tbl_no].areaId[k] == 0)	continue;
		if(	density_area_ptr->data[tbl_no].areaId[k] >= M_DHC_DOWNLOAD_AREA_MAX		)	{
			if(log_dsp_f)	SC_LOG_DebugPrint(SC_TAG_RC,"@@@@@@@@@  RC_AreaDiv_CheckDownload  areaId...%d  ", density_area_ptr->data[tbl_no].areaId[k]);

			continue;
		}
		if(downloadarea_ptr->data[density_area_ptr->data[tbl_no].areaId[k] - 1].download_f == M_DHC_DOWNLOAD_AREA_ON)	{
			return (e_SC_RESULT_SUCCESS);
		}
	}


	return (e_SC_RESULT_FAIL);


}

/**
 * @brief	正規化座標のレベル変換
 * @param	[I]変換するﾚﾍﾞﾙ
 * @param	[I]変換する正規化座標
 * @param	[O]変換後の正規化座標
 */
E_SC_RESULT 	RC_AreaDiv_Change_ParcelID_Pos(
												INT32	target_level,		// 変換するﾚﾍﾞﾙ
												UINT32	base_parcelID,		// 変換するﾊﾟｰｾﾙID
												DOUBLE	base_x,				// 変換する正規化Ｘ座標
												DOUBLE	base_y,				// 変換する正規化Ｙ座標
												UINT32*	target_parcelID,	// 変換後のﾊﾟｰｾﾙID
												DOUBLE*	target_x,			// 変換後の正規化Ｘ座標
												DOUBLE*	target_y			// 変換後の正規化Ｙ座標
											)
{
	DOUBLE		base_latitude;		//	変換後の緯度（秒）
	DOUBLE		base_longitude;		//	変換後の経度（秒）
	if(-1 == SC_MESH_ChgParcelIDToTitude(
										SC_MESH_GetLevel(base_parcelID),		// 変換するﾚﾍﾞﾙ
										base_parcelID,							// 変換するﾊﾟｰｾﾙID
										base_x,									// 変換する正規化Ｘ座標
										base_y,									// 変換する正規化Ｙ座標
										&base_latitude,							// 変換後の緯度（秒）
										&base_longitude							// 変換後の経度（秒）
									)
	)
		return (e_SC_RESULT_FAIL);

	if( -1 == SC_Lib_ChangeTitude2PID(
										base_latitude,		// 変換する緯度（秒）
										base_longitude,		// 変換する経度（秒）
										target_level,		// 変換後のレベル
										target_parcelID,	// 変換後のパーセルID
										target_x,			// 変換後の正規化Ｘ座標
										target_y			// 変換後の正規化Ｙ座標
									)
	)
		return (e_SC_RESULT_FAIL);


	return (e_SC_RESULT_SUCCESS);
}

/**
 * 全近傍リンクがエリアに包含されているかを判定する
 * @param aLb エリア左下パーセル
 * @param aRt エリア右上パーセル
 * @param aNbr 判定対象近傍情報
 * @return true:左下～右上までのエリア内に対象パーセルが含まれている
 */
static Bool checkCoverAreaNbrLinks(UINT32 aLb, UINT32 aRt, SCRP_NEIGHBORINFO* aNbr) {

	UINT32 i;

	if (NULL == aNbr || NULL == aNbr->neighborLink || 0 == aNbr->nbrLinkVol) {
		SC_LOG_DebugPrint(SC_TAG_RC, "bad param"HERE);
		return (false);
	}

	for (i = 0; i < aNbr->nbrLinkVol; i++) {
		if (!checkCoverArea(aLb, aRt, aNbr->neighborLink[i].parcelId)) {
			SC_LOG_DebugPrint(SC_TAG_RC, "mono area is not cover neighbor links..."HERE);
			return (false);
		}
	}

	return (true);
}

/**
 * パーセルエリア内包含判定
 * @param aLb エリア左下パーセル
 * @param aRt エリア右上パーセル
 * @param aTerget 判定対象パーセル
 * @return true:左下～右上までのエリア内に対象パーセルが含まれている
 */
static Bool checkCoverArea(UINT32 aLb, UINT32 aRt, UINT32 aTarget) {


	INT32 sft_x = 0;
	INT32 sft_y = 0;
	INT32 ret = -1;

	ret = SC_MESH_GetAlterPos(aLb, aTarget, 1, &sft_x, &sft_y);
	if (ret == -1 || 0 > sft_x || 0 > sft_y) {
		SC_LOG_DebugPrint(SC_TAG_RC, "target parcel is not cover area. %x ->%x (%d,%d)"HERE, aLb, aTarget, sft_x, sft_y);
		return (false);
	}
	ret = SC_MESH_GetAlterPos(aRt, aTarget, 1, &sft_x, &sft_y);
	if (ret == -1 || 0 < sft_x || 0 < sft_y) {
		SC_LOG_DebugPrint(SC_TAG_RC, "target parcel is not cover area. %x ->%x (%d,%d)"HERE, aLb, aTarget, sft_x, sft_y);
		return (false);
	}


	return (true);
}
