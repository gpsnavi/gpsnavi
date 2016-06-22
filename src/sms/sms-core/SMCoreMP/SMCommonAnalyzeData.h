/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#pragma once

#define INVALID_32	0xFFFFFFFF
#define INVALID_16	0xFFFF
#define INVALID_8	0xFF

//-----------------------------------------------------------------------------
// マクロ
//-----------------------------------------------------------------------------
#define GetUINT32(p_data, ofs)			(*(UINT32*)((char*)p_data+ofs))
#define GetINT32(p_data, ofs)			(*(INT32*)((char*)p_data+ofs))
#define GetUINT16(p_data, ofs)			(*(UINT16*)((char*)p_data+ofs))
#define GetINT16(p_data, ofs)			(*(INT16*)((char*)p_data+ofs))
#define GetUINT8(p_data, ofs)			(*(UINT8*)((char*)p_data+ofs))
#define GetINT8(p_data, ofs)			(*(INT8*)((char*)p_data+ofs))


//-----------------------------------------------------------------------------
// 共通
//-----------------------------------------------------------------------------
// パーマネントID
typedef union _BA_PERMANENT_ID {
	struct{
		UINT32	n					: 20;	// bit 19～ 0 - ID
		UINT32	b					:  3;	// bit 22～20 - 情報
		UINT32	c					:  1;	// bit 23     - 情報種別(0:リンクID,1接続ID)
		UINT32	u					:  1;	// bit 24     - 差分更新情報(0:差分更新されていない情報,1:差分更新された情報)
		UINT32	d					:  2;	// bit 26～25 - 方向(0:方向無,1:始点側,2:終点側,3:リザーブ)
		UINT32	p					:  4;	// bit 30～27 - 収録位置(自身は5)
		UINT32	s					:  1;	// bit 31     - 有効無効フラグ(0:有効,1:無効)
	} b;
	UINT32 d;
} BA_PERMANENT_ID;

// ボリューム情報	4byte
typedef union _BA_VOLUM_INFO {
	struct{
		UINT32	noncomp_size		: 29;	// bit 28～ 0 - バイナリデータ圧縮前サイズ
		UINT32	comp_form			:  3;	// bit 31～29 - バイナリデータ圧縮方式
	} b;
	UINT32 d;
} BA_VOLUM_INFO;

// リンク基本情報1	4byte
typedef union _BA_LINK_BASE_INFO1 {
	struct{
		UINT32	bypass_flg			:  1;	// bit  0     - バイパスフラグ
		UINT32	toll_flg			:  1;	// bit  1     - 有料道路フラグ
		UINT32	ipd_flg				:  1;	// bit  2     - 格上げIPDフラグ
		UINT32	plan_road			:  1;	// bit  3     - 計画道路
		UINT32	uturn_link			:  1;	// bit  4     - Ｕターン専用リンク
		UINT32	under_road_link		:  1;	// bit  5     - 地下車道リンク
		UINT32	high_level_link		:  1;	// bit  6     - 高架リンク
		UINT32	bridge_link			:  1;	// bit  7     - 橋リンク
		UINT32	tunnel_link			:  1;	// bit  8     - トンネルリンク
		UINT32	median_flg			:  2;	// bit 10～ 9 - 分離帯有無フラグ
		UINT32	infra_link_flg		:  1;	// bit 11     - インフラリンク該当フラグ
		UINT32	dts_flg				:  1;	// bit 12     - DST対象フラグ
		UINT32	pass_flg			:  1;	// bit 13     - 通行フラグ
		UINT32	one_way_flg			:  2;	// bit 15～14 - 一方通行コード
		UINT32	link_kind4			:  2;	// bit 17～16 - リンク種別４
		UINT32	link_kind3			:  3;	// bit 20～18 - リンク種別３
		UINT32	link_kind2			:  3;	// bit 23～21 - リンク種別２
		UINT32	link_kind1			:  4;	// bit 27～24 - リンク種別１
		UINT32	road_kind			:  4;	// bit 31～28 - 道路種別
	} b;
	UINT32 d;
} BA_LINK_BASE_INFO1;

// リンク基本情報2	4byte
typedef union _BA_LINK_BASE_INFO2 {
	struct{
		UINT32	country_code		:  3;	// bit  2～ 0 - 走りやすさマップ(走りやすさ)
		UINT32	lane_cnt			:  2;	// bit  4～ 3 - 走りやすさマップ(車線数)
		UINT32	other_regulation	:  1;	// bit  5     - その他規制エリアリンク
		UINT32	military_area		:  1;	// bit  6     - 軍事エリアリンク
		UINT32	freeze				:  1;	// bit  7     - 凍結リンク
		UINT32	flooded				:  1;	// bit  8     - 冠水リンク
		UINT32	school_zone			:  1;	// bit  9     - スクールゾーン
		UINT32	function_class		:  3;	// bit 12～10 - ファンクションクラス
		UINT32	width_code			:  3;	// bit 15～13 - 道路幅員コード
		UINT32	link_length			: 12;	// bit 27～16 - リンク長
		UINT32	link_length_unit	:  3;	// bit 30～28 - リンク長の単位フラグ
		UINT32	reserve				:  1;	// bit 31     - リザーブ
	} b;
	UINT32 d;
} BA_LINK_BASE_INFO2;


//-----------------------------------------------------------------------------
// 道路形状バイナリ
//-----------------------------------------------------------------------------
// XX情報1	4byte
typedef union _BA_XX_INFO {
	struct{
		UINT32	reserve				: 25;	// bit 24～ 0 - リザーブ
		UINT32	area_class_flg		:  2;	// bit 26～25 - 地域クラス情報有無フラグ
		UINT32	route_info_flg		:  1;	// bit 27     - 道路路線情報有無フラグ
		UINT32	higher_link_cnt		:  4;	// bit 31～28 - 上位レベルリンク情報数
	} b;
	UINT32 d;
} BA_XX_INFO;


//-----------------------------------------------------------------------------
// 道路ネットワークバイナリ
//-----------------------------------------------------------------------------
// リンク拡張1	4byte
typedef union _BA_LKEX_INFO1 {
	struct{
		UINT32	up_lvl_link_cnt		:  4;	// bit  3～ 0 - 上位レベルリンク情報数
		UINT32	down_lvl_link_flg	:  1;	// bit  4     - 下位レベルリンク情報有無フラグ
		UINT32	reserve				:  1;	// bit  5     - リザーブ
		UINT32	limit_speed			:  8;	// bit 13～ 6 - 制限速度
		UINT32	limit_speed_flg		:  1;	// bit 14     - 制限速度の単位フラグ
		UINT32	limit_speed_type	:  1;	// bit 15     - 制限速度タイプ
		UINT32	route_dir			:  3;	// bit 18～16 - 路線方向
		UINT32	reg_flg_e			:  1;	// bit 19     - 交通規制調査フラグ（終点）
		UINT32	reg_flg_s			:  1;	// bit 20     - 交通規制調査フラグ（始点）
		UINT32	slope				:  3;	// bit 23～21 - 勾配
		UINT32	tz_diff_flg			:  2;	// bit 25～24 - TZ分差フラグ
		UINT32	tz_time_diff		:  5;	// bit 30～26 - TZ時間差値
		UINT32	tz_dir_flg			:  1;	// bit 31     - TZ方向フラグ
	} b;
	UINT32 d;
} BA_LKEX_INFO1;

// リンク拡張2	4byte
typedef union _BA_LKEX_INFO2 {
	struct{
		UINT32	reserve				: 22;	// bit 21～ 0 - リザーブ
		UINT32	car_pool_flg		:  1;	// bit 22     - カープールレーン利用情報有無フラグ
		UINT32	lane_cnt_flg		:  1;	// bit 23     - レーン数有無フラグ
		UINT32	up_lvl_e			:  4;	// bit 27～24 - 上位レベル存在情報（終点）
		UINT32	up_lvl_s			:  4;	// bit 31～28 - 上位レベル存在情報（始点）
	} b;
	UINT32 d;
} BA_LKEX_INFO2;

// 地域クラス情報フラグ	4byte

// リンク方向	4byte
typedef union _BA_LINK_DIR {
	struct{
		UINT32	ave_travel_time_flg	: 12;	// bit 11～ 0 - 平均旅行時間
		UINT32	ave_travel_time		:  2;	// bit 13～12 - 平均旅行時間の単位フラグ
		UINT32	dakou				:  2;	// bit 15～14 - 蛇行道路
		UINT32	dir_s				:  8;	// bit 23～16 - リンク方向（終点）
		UINT32	dir_e				:  8;	// bit 31～24 - リンク方向（始点）
	} b;
	UINT32 d;
} BA_LINK_DIR;


//-----------------------------------------------------------------------------
// 背景データカラム
//-----------------------------------------------------------------------------
// 背景情報1	2byte
typedef union _BA_BKGD_INFO1 {
	struct{
		UINT32	reserve				:  6;	// bit  5～ 0 - 最大階数
		UINT32	exadd_flg			:  1;	// bit  6     - 付加情報有無フラグ
		UINT32	obj3d_flg			:  1;	// bit  7     - 3Dオブジェクト有無フラグ
		UINT32	figure_type			:  2;	// bit  9～ 8 - 図形タイプ
		UINT32	zoom_flg			:  6;	// bit 15～10 - ズーム許可フラグ
	} b;
	UINT16 d;
} BA_BKGD_INFO1;

// 背景情報2	4byte
typedef union _BA_BKGD_INFO2 {
	struct{
		UINT32	kind_cd				:  8;	// bit  7～ 0 - 背景種別コード（種別ｺｰﾄﾞ）
		UINT32	type_cd				:  8;	// bit 15～ 8 - 背景種別コード（分類ｺｰﾄﾞ）
		UINT32	sort_id				: 16;	// bit 31～16 - ソートID
	} b;
	UINT32 d;
} BA_BKGD_INFO2;

// 背景座標情報	2byte
typedef union _BA_BKGD_POINT_INFO {
	struct{
		UINT32	reserve2			:  8;	// bit  7～ 0 - 予約2
		UINT32	primitive_kind		:  4;	// bit 11～ 8 - 描画プリミティブ種別
		UINT32	reserve				:  2;	// bit 13～12 - 予約1
		UINT32	express_info		:  1;	// bit 14     - 表現付加情報
		UINT32	data_form			:  1;	// bit 15     - データ形式
	} b;
	UINT16 d;
} BA_BKGD_POINT_INFO;


//-----------------------------------------------------------------------------
// 記号背景データカラム
//-----------------------------------------------------------------------------
// 記号背景情報1	4byte
typedef union _BA_MARK_INFO1 {
	struct{
		UINT32	kind_cd				: 16;	// bit 15～ 0 - 背景種別コード（種別ｺｰﾄﾞ）
		UINT32	type_cd				: 16;	// bit 31～16 - 背景種別コード（分類ｺｰﾄﾞ）
	} b;
	UINT32 d;
} BA_MARK_INFO1;

// 記号背景情報2	2byte
typedef union _BA_MARK_INFO2 {
	struct{
		UINT32	reserve				:  8;	// bit  7～ 0 - リザーブ
		UINT32	figure_type			:  2;	// bit  9～ 8 - 図形タイプ
		UINT32	zoom_flg			:  6;	// bit 15～10 - ズーム許可フラグ
	} b;
	UINT16 d;
} BA_MARK_INFO2;


//-----------------------------------------------------------------------------
// 表示名称データカラム
//-----------------------------------------------------------------------------
// 名称情報1	4byte
typedef union _BA_NAME_INFO1 {
	struct{
		UINT32	reserve				:  7;	// bit  6～ 0 - リザーブ
		UINT32	angle				:  9;	// bit 15～ 7 - 角度
		UINT32	teisu				:  4;	// bit 19～16 - 乗算定数
		UINT32	type				:  3;	// bit 22～20 - 文字情報タイプ
		UINT32	dir					:  1;	// bit 23     - 文字列方向
		UINT32	size				:  2;	// bit 25～24 - 文字サイズ
		UINT32	zoom_flg			:  6;	// bit 31～26 - ズーム許可フラグ
/*		UINT32	zoom_flg			:  6;	// bit 31～26 - ズーム許可フラグ
		UINT32	size				:  2;	// bit 25～24 - 文字サイズ
		UINT32	dir					:  1;	// bit 23     - 文字列方向
		UINT32	type				:  3;	// bit 22～20 - 文字情報タイプ
		UINT32	teisu				:  4;	// bit 19～16 - 乗算定数
		UINT32	angle				:  9;	// bit 15～ 7 - 角度
		UINT32	reserve				:  7;	// bit  6～ 0 - リザーブ*/
	} b;
	UINT32 d;
} BA_NAME_INFO1;


//-----------------------------------------------------------------------------
// 道路名称データカラム
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// 誘導データカラム
//-----------------------------------------------------------------------------
// 各種誘導データオフセット有無フラグ	2byte
typedef union _BA_GUIDE_OFS_FLG {
	struct{
		UINT32	reserve				: 13;	// bit 12～ 0 - リザーブ
		UINT32	speed				:  1;	// bit 13     - スピード取り締まり機オフセット有無
		UINT32	speed_limit			:  1;	// bit 14     - スピードリミットオフセット有無
		UINT32	exit				:  1;	// bit 15     - EXIT情報オフセット有無
	} b;
	UINT16 d;
} BA_GUIDE_OFS_FLG;

// 各種誘導データオフセット有無フラグ	2byte
typedef union _BA_GUIDE_NODE_OFS_FLG {
	struct{
		UINT32	reserve				:  7;	// bit  6～ 0 - リザーブ
		UINT32	area_info			:  1;	// bit  7     - エリア案内情報　
		UINT32	rane_info			:  1;	// bit  8     - 専用レーン誘導情報　
		UINT32	guide_info			:  1;	// bit  9     - 案内情報
		UINT32	closs_shape			:  1;	// bit 10     - 交差点詳細図
		UINT32	dest				:  1;	// bit 11     - 行き先看板
		UINT32	traffic_sign		:  1;	// bit 12     - 交通標識
		UINT32	road_struct			:  1;	// bit 13     - 道路構造物
		UINT32	etc_rane			:  1;	// bit 14     - ETCレーン
		UINT32	node_attr			:  1;	// bit 15     - ノード属性
	} b;
	UINT16 d;
} BA_GUIDE_NODE_OFS_FLG;

// 地点種別	2byte
typedef union _BA_GUIDE_SPOT_KIND {
	struct{
		UINT32	reserve				:  9;	// bit  8～ 0 - リザーブ
		UINT32	institution_flg		:  1;	// bit  9     - 建造物・施設 有無　
		UINT32	cross_name_flg		:  1;	// bit 10     - 交差点名称 有無
		UINT32	signal				:  1;	// bit 11     - 信号機
		UINT32	spot_kind			:  4;	// bit 15～12 - 地点種別
	} b;
	UINT16 d;
} BA_GUIDE_SPOT_KIND;

// 交差点名称有無情報	2byte
typedef union _BA_GUIDE_CROSS_NAME_INFO {
	struct{
		UINT32	reserve				: 14;	// bit 13～ 0 - リザーブ
		UINT32	voice_flg			:  1;	// bit 14     - 音声データ数/音声データ 有無
		UINT32	cross_name_yomi_flg	:  1;	// bit 15     - 交差点名称読み 有無
	} b;
	UINT16 d;
} BA_GUIDE_CROSS_NAME_INFO;
