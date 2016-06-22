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
 * SMCoreTRCom.h
 *
 *  Created on: 2016/01/04
 *      Author: n.kanagawa
 */
#ifndef SMCORETRCOM_H_
#define SMCORETRCOM_H_


// 交通情報データ
typedef struct {
	UINT32			pid;		// パーセルＩＤ
	TR_DATA_t		data;		// データ
} TR_TRAFFIC_t;

// 交通情報リスト
typedef struct {
	TR_TRAFFIC_t	trf[TR_AREA_MAX];
	UINT32			cnt;
} TR_TRAFFIC_LIST_t;


//-----------------------------------
// I/F定義
//-----------------------------------
E_SC_RESULT SC_TR_ComTraffic(const TR_PARCEL_LIST_t *pList, TR_TRAFFIC_LIST_t *pTraffic);

#endif /* SMCORETRCOM_H_ */
