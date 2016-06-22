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
 * SMCoreTRData.h
 *
 *  Created on: 2016/01/04
 *      Author: n.kanagawa
 */
#ifndef SMCORETRDATA_H_
#define SMCORETRDATA_H_


//-----------------------------------------------------------------------------
// マクロ
//-----------------------------------------------------------------------------
#if 0
// little
#define TR_GetUINT32(p_data, ofs)		(*(UINT32*)((char*)p_data+ofs))
#define TR_GetUINT16(p_data, ofs)		(*(UINT16*)((char*)p_data+ofs))
#define TR_GetUINT8(p_data, ofs)		(*(UINT8*)((char*)p_data+ofs))
#else
// big
#define CONVERT_ENDIAN_16(d) \
        (((d)<<8) | (((d)>>8) & 0x00ff))
#define CONVERT_ENDIAN_32(d) \
        (((d)<<24) | (((d)<<8) & 0x00ff0000) | (((d)>>8) & 0x0000ff00) | (((d)>>24) & 0x000000ff))
#define TR_GetUINT32(p_data, ofs)		CONVERT_ENDIAN_32(*(UINT32*)((char*)p_data+ofs))
#define TR_GetUINT16(p_data, ofs)		CONVERT_ENDIAN_16(*(UINT16*)((char*)p_data+ofs))
#define TR_GetUINT8(p_data, ofs)		(*(UINT8*)((char*)p_data+ofs))
#endif

#define TR_DATA_HEAD_SIZE			16									// 交通情報バイナリデータヘッダサイズ
#define TR_PCL_HEAD_SIZE			10									// パーセルヘッダサイズ
#define TR_CGN_HEAD_SIZE			10									// 渋滞情報ヘッダサイズ
#define TR_RDKD_HEAD_SIZE			22									// 道路種別ごと交通情報ヘッダサイズ

// 交通情報バイナリデータ-ヘッダ
#define TR_DATA_KIND(_p)			TR_GetUINT8(_p, 0)					// データ種別
#define TR_MAP_VER(_p)				TR_GetUINT32(_p, 2)					// 地図バージョン
#define TR_FMT_VER(_p)				TR_GetUINT16(_p, 6)					// データフォーマットバージョン
#define TR_PCL_CNT(_p)				TR_GetUINT32(_p, 8)					// パーセルID数
#define TR_DATA_SIZE(_p)			TR_GetUINT32(_p, 12)				// データサイズ
#define TR_PCL_ADDR(_p)				(((char*)_p)+16)					// パーセルの並び先頭アドレス

// パーセル毎データ-ヘッダ
#define TR_PCL_SIZE(_p)				TR_GetUINT32(_p, 0)					// パーセル毎データサイズ
#define TR_PCL_ID(_p)				TR_GetUINT32(_p, 4)					// パーセルID
#define TR_YN_FLG(_p)				TR_GetUINT16(_p, 8)					// データ有無フラグ

// パーセル毎データ-渋滞情報ヘッダ
#define TR_CGN_SIZE(_p)				TR_GetUINT32(_p, 0)					// 渋滞情報データサイズ
#define TR_EXPIRY_DATE(_p)			TR_GetUINT32(_p, 4)					// 有効期限
#define TR_ROADKIND_CNT(_p)			TR_GetUINT8(_p, 9)					// パーセルID・道路種別ごとの渋滞情報数

// 渋滞情報(道路種別毎)-ヘッダ
#define TR_RDK_SIZE(_p)				TR_GetUINT32(_p, 0)					// パーセルID・道路種別内 バイト数
#define TR_RKPCL_ID(_p)				TR_GetUINT32(_p, 4)					// パーセルID
#define TR_ROAD_KIND(_p)			((TR_GetUINT16(_p, 8)&0xF000)>>12)	// 道路種別
#define TR_CRE_DATE(_p)				TR_GetUINT32(_p, 10)				// 交通情報作成日時分(UTC時刻)
#define TR_INFO_CNT(_p)				TR_GetUINT32(_p, 14)				// パーセルID・道路種別内 情報数
#define TR_INFO_OFS(_p)				TR_GetUINT32(_p, 18)				// 情報1までのオフセット

// 渋滞情報(道路種別毎)-情報
#define TR_LINK_ID(_p)				TR_GetUINT32(_p, 0)					// リンクID
#define TR_LINK_DIR(_p)				((TR_GetUINT8(_p, 4)&0x80)>>7)		// 方向
#define TR_LINK_CLASS(_p)			TR_GetUINT8(_p, 5)					// 集約リンク区分
//#define TR_LINK_DIR(_p)				((TR_GetUINT16(_p, 4)&0x8000)>>15)	// 方向
//#define TR_LINK_CLASS(_p)			((TR_GetUINT16(_p, 4)&0x00FF))		// 集約リンク区分
#define TR_BASE_INFO(_p)			TR_GetUINT16(_p, 6)					// 基本情報
#define TR_SEC_CNT(_p)				((TR_BASE_INFO(_p)&0xFF00)>>8)		// 渋滞部数
#define TR_CNGS_LVL(_p)				((TR_BASE_INFO(_p)&0x00E0)>>5)		// 渋滞度
#define TR_TRVL_FLG(_p)				((TR_BASE_INFO(_p)&0x0010)>>4)		// 旅行時間提供有無フラグ
#define TR_TRVL_LIND(_p)			((TR_BASE_INFO(_p)&0x000C)>>2)		// 旅行時間種別
#define TR_INFO_SIZE(_p)			(10 + (TR_LINK_CLASS(_p)*6) + (TR_TRVL_FLG(_p)*2) + (TR_SEC_CNT(_p)*2))

// 地図バージョン
typedef union _TR_BIN_MAP_VER {
	struct{
		// ★
		UINT32	Reserved			:  5;	// bit 31～27  - Reserved
	} b;
	UINT32 d;
} TR_BIN_MAP_VER;

// データフォーマットバージョン
typedef union _TR_BIN_FMT_VER {
	struct{
		UINT32	decimal				:  8;	// bit  7～ 0  - 少数部
		UINT32	integer				:  8;	// bit 15～ 8  - 整数部
	} b;
	UINT16 d;
} TR_BIN_FMT_VER;

// データ有無フラグ
typedef union _TR_BIN_YN_FLG {
	struct{
		UINT32	regulation			:  1;	// bit  0     - 事象・規制情報
		UINT32	parking				:  1;	// bit  1     - 駐車場情報
		UINT32	sapa				:  1;	// bit  2     - ＳＡ／ＰＡ情報
		UINT32	congestion			:  4;	// bit  3     - 渋滞情報
		UINT32	reserve				: 12;	// bit 15～ 4 - reserve
	} b;
	UINT16 d;
} TR_BIN_YN_FLG;

// 有効期限
// 交通情報作成日時分
typedef union _TR_BIN_TIME {
	struct{
		UINT32	mm					:  6;	// bit  5～ 0  - 0～59（分）
		UINT32	hh					:  5;	// bit 10～ 6  - 0～23（時　24時間表現）
		UINT32	day					:  5;	// bit 15～11  - 1～31（日）
		UINT32	manth				:  4;	// bit 19～16  - 1～12（月）
		UINT32	year				:  7;	// bit 26～20  - 0～99（年　西暦下2桁）
		UINT32	reserve				:  5;	// bit 31～27  - reserve
	} b;
	UINT32 d;
} TR_BIN_TIME;

// 基本情報
typedef union _TR_BIN_BASE_INFO {
	struct{
		UINT32	reserve				:  2;	// bit  1～ 0  - reserve
		UINT32	travel_kind			:  2;	// bit  3～ 2  - 旅行時間種別
		UINT32	travel_flg			:  1;	// bit  4      - 旅行時間提供有無フラグ
		UINT32	congestion_lvl		:  3;	// bit  7～ 5  - 渋滞度
		UINT32	sec_cnt				:  8;	// bit 15～ 8  - 渋滞部数
	} b;
	UINT16 d;
} TR_BIN_BASE_INFO;


#endif /* SMCORETRDATA_H_ */
