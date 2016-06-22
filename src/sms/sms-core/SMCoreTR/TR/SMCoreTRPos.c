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
 * SMCoreTRPos.c
 *
 *  Created on: 2016/01/04
 *      Author: n.kanagawa
 */
#include "sms-core/SMCoreTR/SMCoreTRInternal.h"


/**
 * @brief	カレント緯度経度取得
 * @param	[I]user			ユーザ
 * @param	[O]lat			緯度
 * @param	[O]lon			経度
 * @return	パーセルID
 */
E_SC_RESULT SC_TR_GetCurrentPos(const UINT16 user, DOUBLE *pLat, DOUBLE *pLon)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SMCARSTATE	carState = {};
	SMGEOCOORD	geoCoord = {};
	//UINT32		parcelId = 0;
	DOUBLE		lat = 0.0;
	DOUBLE		lon = 0.0;
	INT32		maps = 1;

	do {
		if (TR_USERTYPE_CARPOS == user) {
			// 自車位置
			// 車両状態取得
			ret = SC_MNG_GetCarState(&carState, e_SC_CARLOCATION_NOW);
			if(e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_ErrorPrint(SC_TAG_TR, (Char*)"SC_MNG_GetCarState, ret=%d, " HERE, ret);
				break;
			}

			// パーセルID算出
			lat	= carState.coord.latitude/1024.0;		// 緯度位置座標
			lon	= carState.coord.longitude/1024.0;		// 経度位置座標

		} else if (TR_USERTYPE_SCROLL == user) {
			// カーソル位置
			// カーソル座標を取得
			ret = SC_MNG_GetMapCursorCoord(maps, &geoCoord);
			if(e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_ErrorPrint(SC_TAG_TR, (Char*)"SC_MNG_GetMapCursorCoord, ret=%d, " HERE, ret);
				break;
			}

			// パーセルID算出
			lat	= geoCoord.latitude/1024.0;		// 緯度位置座標
			lon	= geoCoord.longitude/1024.0;	// 経度位置座標
		} else {
			ret = e_SC_RESULT_FAIL;
			break;
		}

		*pLat = lat;
		*pLon = lon;

	} while (0);

	return (ret);
}

/**
 * @brief	カレントパーセルID取得
 * @param	[I]user			ユーザー
 * @return	パーセルID
 */
UINT32 SC_TR_GetCurrentPosParcelID(const UINT16 user)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	UINT32		parcelId = 0;
	DOUBLE		x = 0;
	DOUBLE		y = 0;
	DOUBLE		lat = 0.0;
	DOUBLE		lon = 0.0;
	INT32		level = 1;

	ret = SC_TR_GetCurrentPos(user, &lat, &lon);
	if(e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_TR, (Char*)"SC_TR_GetCurrentPos, ret=%d, " HERE, ret);
		return (parcelId);
	}

	// 緯度経度からパーセルID算出
	MESHC_ChgLatLonToParcelID(lat, lon, level, &parcelId, &x, &y);

	return (parcelId);
}

/**
 * @brief	周辺パーセルリスト取得
 * @param	[I]parcelId		パーセルID(レベル1)
 * @param	[O]pList		パーセルリスト
 */
void SC_TR_GetAreaParcelList(const UINT32 parcelId, TR_PARCEL_LIST_t *pList)
{
	INT16	x = 0;
	INT16	y = 0;

	// 初期化
	pList->cnt = 0;

	for (x=-1; x<=1; x++) {
		for (y=-1; y<=1; y++) {
			pList->pcl[pList->cnt] = SC_MESH_SftParcelId(parcelId, x, y);
			pList->cnt++;
		}
	}
}

/**
 * @brief	レベル2パーセルリスト取得
 * @param	[I]pLvl1PclList	レベル1パーセルリスト
 * @param	[O]pList		レベル2パーセルリスト
 */
void SC_TR_GetLevel2ParcelList(const TR_PARCEL_LIST_t *pLvl1PclList, TR_PARCEL_LIST_t *pList)
{
	INT32	i = 0;
	INT32	j = 0;
	UINT32	tmpParcelId = 0;
	Bool	sameFlg = false;

	// 初期化
	pList->cnt = 0;

	for (i=0; i<pLvl1PclList->cnt; i++) {
		// 上記パーセルID取得
		tmpParcelId = SC_MESH_GetUpperParcelID(pLvl1PclList->pcl[i]);

		// 重複チェック
		sameFlg = false;
		for (j=0; j<pList->cnt; j++) {
			if (tmpParcelId == pList->pcl[j]) {
				sameFlg = true;
				break;
			}
		}

		if (!sameFlg) {
			// 重複なしの場合リストにパーセルID格納
			pList->pcl[pList->cnt] = tmpParcelId;
			pList->cnt++;
		}
	}
}
