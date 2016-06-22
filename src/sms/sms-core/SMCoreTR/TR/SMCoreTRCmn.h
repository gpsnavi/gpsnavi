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
 * SMCoreTRCmn.h
 *
 *  Created on: 2016/01/04
 *      Author: n.kanagawa
 */
#ifndef SMCORETRCMN_H_
#define SMCORETRCMN_H_

// ユーザタイプ
#define TR_USERTYPE_NONE		0x0000					// ユーザ無
#define TR_USERTYPE_CARPOS		0x0001					// 自車周辺
#define TR_USERTYPE_SCROLL		0x0002					// スクロール地点周辺
#define TR_USERTYPE_ALL			(TR_USERTYPE_CARPOS|TR_USERTYPE_SCROLL)

#if 1
#define TR_LEVEL				1						// 処理レベル
#define TR_AREA_MAX				9						// エリア構成パーセル数
#else
#define TR_LEVEL				2						// 処理レベル
#define TR_AREA_MAX				64						// エリア構成パーセル数(16*4枚)
#endif
#define TR_AREA_MGR_MAX			(TR_AREA_MAX * 2)		// パーセル最大保有数
//#define TR_AREA_LEVEL_MAX		1						// パーセル最大レベル数

// 更新タイマ
//#define TR_UPDATE_TIMER			60/**5*/				// 更新タイマ値

// ON/OFF
#define TR_ON					true
#define TR_OFF					false

// 提供情報源ID
#define TR_PROBE				0						// プローブ
#define TR_VICS					1						// VICS
#define TR_PB_VICS				2						// プローブ＋VICS

// データ
typedef struct {
	UINT32				size;		// データサイズ
	char*				pData;		// データアドレス
} TR_DATA_t;

// パーセルリスト
typedef struct {
	UINT32				pcl[TR_AREA_MAX];	// パーセルID
	INT32				cnt;				// パーセル数
} TR_PARCEL_LIST_t;

//-----------------------------------
// I/F定義
//-----------------------------------
time_t SC_TR_NowUTC();
struct tm *SC_TR_NowTM();
void SC_TR_ChgTM(TR_BIN_TIME binTime, struct tm *pTm);
Bool SC_TR_CompTM(const struct tm *a, const struct tm *b);

#endif /* SMCORETRCMN_H_ */
