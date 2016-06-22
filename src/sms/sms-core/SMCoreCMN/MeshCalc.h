/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

/**
 * @file	MeshCalc.h
 * @brief	メッシュ計算ライブラリヘッダ
 *
 * @author	n.kanagawa
 * @date
 */


#ifndef _MESHCALC_H_
#define _MESHCALC_H_

//-----------------------------------------------------------------------------
// 定数
//-----------------------------------------------------------------------------
// 下位パーセル数
#define UNDER_PARCEL_CNT	16

// 方向
#define DIR_TOP				0		// 上
#define DIR_R_TOP			1		// 右上
#define DIR_R				2		// 右
#define DIR_R_DOWN			3		// 右下
#define DIR_DOWN			4		// 下
#define DIR_L_DOWN			5		// 左下
#define DIR_L				6		// 左
#define DIR_L_TOP			7		// 左上

// レベル
#define MAP_LEVEL1			1
#define MAP_LEVEL2			2
#define MAP_LEVEL3			3
#define MAP_LEVEL4			4
#define MAP_LEVEL5			5
#define MAP_LEVEL6			6

// 正規化座標サイズ
#define	MAP_SIZE			4096


//-----------------------------------------------------------------------------
// 構造体
//-----------------------------------------------------------------------------
/**
 * @struct T_UNDER_PARCEL_LIST
 * @brief 下位パーセルIDリスト管理構造体
 */
typedef struct _T_UNDER_PARCEL_LIST {
	UINT32 ParcelId[UNDER_PARCEL_CNT];
} T_UNDER_PARCEL_LIST;

/**
 * @struct T_DIVIDE_PARCEL
 * @brief 分割パーセル構造体
 */
typedef struct _DIVIDE_PARCEL
{
	INT32 d;	//   1次メッシュ経度
	INT32 a;	//   1次メッシュ緯度
	INT32 b;	//   2次メッシュ緯度
	INT32 c;	// 2.5次メッシュ緯度
//	INT32 d1;	//   1次メッシュ経度1
//	INT32 d2;	//   1次メッシュ経度2
	INT32 e;	//   2次メッシュ経度
	INT32 f;	// 2.5次メッシュ経度
} T_DIVIDE_PARCEL;

#endif // _MESHCALC_H_
