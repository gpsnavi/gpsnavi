/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef RT_MAPDEF_H_
#define RT_MAPDEF_H_


/*-------------------------------------------------------------------
 * 定数定義
 *-------------------------------------------------------------------*/

/*-------------------------------------------------------------------
 * マクロ定義
 *-------------------------------------------------------------------*/

/*-------------------------------------------------------------------
 * 便利系
 *-------------------------------------------------------------------*/
#define swap(type,a,b)		{type tmp=a;a=b;b=tmp;}
#define bigEndian16(a)		(((a << 8) | (a >> 8)) & 0x00FF)
#define bigEndian32(a)		(((a << 24) & 0xFF000000) | ((a << 8) & 0x00FF0000) | ((a >> 8) & 0x0000FF00) | ((a >> 24) & 0x000000FF))
// 進み
#define move1byte(p)		((p) += 1)
#define move2byte(p)		((p) += 2)
#define move4byte(p)		((p) += 4)
// バイト別読み
#define read1byte(p)		*((UINT8*)(p))
#define read2byte(p)		*((UINT16*)(p))
#define read4byte(p)		*((UINT32*)(p))
// 読み進み
#define road1byte(p, d)		{(d) = read1byte(p); (p) += 1;}
#define road2byte(p, d)		{(d) = read2byte(p); (p) += 2;}
#define road4byte(p, d)		{(d) = read4byte(p); (p) += 4;}

/*-------------------------------------------------------------------
 * 構造体
 *-------------------------------------------------------------------*/
// 地図データテーブル
typedef struct{
	UINT32						parcel_id;				// パーセルＩＤ
	UINT8						*road_p;				// 道路ＮＷデータ
	UINT8						*sharp_p;				// 道路形状データ
	UINT8						*guide_p;				// 誘導データ
	UINT8						*basis_p;				// パーセル基本情報
} RT_MAPDATA_t;

// 地図データ要求テーブル
typedef struct{
	UINT16						data_vol;				// 要求パーセル数
	RT_MAPDATA_t				data[9];				// 地図データテーブル
} RT_MAPREQ_t;

// リンク情報テーブル
typedef struct {
	UINT16						road_type;				// 道路種別
	UINT16						link_kind;				// リンク種別
	UINT16						onway_code;				// 一方通行
	UINT32						dist;					// リンク長
	UINT32						time;
	RT_LINK_t					id;						// パーセル・リンクＩＤ
	RT_LINKPOINT_t				point;					// 形状点情報
} RT_LINKINFO_t;

// 交差点情報
typedef struct {
	UINT16						tl_f;					// 信号機フラグ
	UINT16						link_vol;				// 差路数
	RT_LINKINFO_t				link[16];				// リンク情報テーブル
	RT_NAME_t					crs_name;				// 交差点名称
} RT_CROSSINFO_t;

typedef struct {
	UINT8						*road_addr;				// 道路ＮＷデータアドレス
	UINT8						*sharp_addr;			// 道路形状データアドレス
	UINT32						parcel_id;				// パーセルＩＤ
	UINT16						cnct_side;				// リンク始終点フラグ
} RT_MAPADDR_t;

E_SC_RESULT RT_MAP_SetReqTbl(UINT32 , T_DHC_REQ_PARCEL *, RT_MAPREQ_t *);
E_SC_RESULT RT_MAP_DataRead(T_DHC_REQ_PARCEL *, RT_MAPREQ_t *);
E_SC_RESULT RT_MAP_DataFree(RT_MAPREQ_t *);
E_SC_RESULT RT_MAP_GetCrossInfo(RT_MAPREQ_t *, RT_LINK_t *, RT_CROSSINFO_t *);

#endif /* RT_MAPDEF_H_ */
