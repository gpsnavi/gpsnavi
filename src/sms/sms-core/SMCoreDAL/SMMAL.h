/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef _SMMAL_H_
#define _SMMAL_H_

/* 地図データハンドル */
typedef UINT8* MAL_HDL;

#define MA_ALLF32 0xFFFFFFFF
#define MA_ALLF16 0xFFFF
#define MA_ALLF8 0xFF

/*-------------------------------------------------------------------
 * 定数定義
 *-------------------------------------------------------------------*/
// ##BASE_LINK_INFO Road種別
#define _ROAD_0								(0)
#define _ROAD_1								(1)
#define _ROAD_2								(2)
#define _ROAD_3								(3)
#define _ROAD_4								(4)
#define _ROAD_5								(5)
#define _ROAD_6								(6)
#define _ROAD_7								(7)
#define _ROAD_8								(8)
#define _ROAD_9								(9)
#define _ROAD_10							(10)
#define _ROAD_11							(11)
#define _ROAD_12							(12)
#define _ROAD_13							(13)
#define _ROAD_14							(14)
#define _ROAD_15							(15)
// ##BASE_LINK_INFO Link種別
#define _LINK_0								(0)
#define _LINK_1								(1)
#define _LINK_2								(2)
#define _LINK_3								(3)
#define _LINK_4								(4)
#define _LINK_5								(5)
#define _LINK_6								(6)
#define _LINK_7								(7)
#define _LINK_8								(8)
#define _LINK_9								(9)
#define _LINK_10							(10)
#define _LINK_11							(11)
#define _LINK_12							(12)
#define _LINK_13							(13)
#define _LINK_14							(14)
#define _LINK_15							(15)
// JPN（TODO 海外）
#define SC_MA_ROAD_UNKNOWN					_ROAD_3				// 不明
#define SC_MA_ROAD_TYPE_HIGHWAY_1			_ROAD_0				// 都市間高速
#define SC_MA_ROAD_TYPE_HIGHWAY_2			_ROAD_1				// 都市内高速
#define SC_MA_ROAD_TYPE_TOLLWAY				_ROAD_2				// 有料道路
//#define RP_ROAD_							_ROAD_				// Reserve
#define SC_MA_ROAD_TYPE_NATIONAL			_ROAD_4				// 国道
#define SC_MA_ROAD_TYPE_PREFECTURE			_ROAD_5				// 県道
#define SC_MA_ROAD_TYPE_MAINRURAL			_ROAD_6				// 主要地方道
#define SC_MA_ROAD_TYPE_LOCAL_1				_ROAD_7				// 一般道１（13m～）
#define SC_MA_ROAD_TYPE_LOCAL_2				_ROAD_8				// 一般道２（5.5m～13m）
#define SC_MA_ROAD_TYPE_LOCAL_3				_ROAD_9				// 一般道３（3m～5.5m）
#define SC_MA_ROAD_TYPE_NARROW_1			_ROAD_10			// 細道路１（3m～5.5m）
#define SC_MA_ROAD_TYPE_NARROW_2			_ROAD_11			// 細道路２（3m～5.5m）
#define SC_MA_ROAD_TYPE_ATHER				_ROAD_12			// 他
#define SC_MA_ROAD_TYPE_FERRY				_ROAD_13			// フェリー
#define SC_MA_ROAD_TYPE_CARTRAIN			_ROAD_14			// カートレイン
#define SC_MA_ROAD_TYPE_FOOT				_ROAD_15			// 歩道
// JPN（TODO 海外）
/* 【TYPE1】 0～12 */
#define SC_MA_LINK_TYPE1_NORMAL				_LINK_0				// 上下線非分離（本線）
#define SC_MA_LINK_TYPE1_SEPARATE			_LINK_1				// 上下線分離（本線）
//#define SC_MA_LINK_TYPE1_					_LINK_2				// 連結路１
//#define SC_MA_LINK_TYPE1_					_LINK_3				// 連結路２
//#define SC_MA_LINK_TYPE1_					_LINK_4				// 本線と同一路線の側道
//#define SC_MA_LINK_TYPE1_FRONTAG			_LINK_5				// Frontage Road
#define SC_MA_LINK_TYPE1_SAPA				_LINK_6				// SAPA等側線
#define SC_MA_LINK_TYPE1_ROUNDABOUT			_LINK_7				// ロータリー
//#define SC_MA_LINK_TYPE1_CIRCLE			_LINK_8				// Circle
//#define SC_MA_LINK_TYPE1_INSITE			_LINK_9				// 敷地内リンク
#define SC_MA_LINK_TYPE1_SMARTIC			_LINK_10			// スマートICリンク
#define SC_MA_LINK_TYPE1_PARKING			_LINK_11			// 駐車場接続リンク
//#define SC_MA_LINK_TYPE1_SLOPE			_LINK_12			// スロープリンク
/* 【TYPE2】 0～3 */
//#define SC_MA_LINK_TYPE2_					_LINK_0				//
#define SC_MA_LINK_TYPE2_CLOSS				_LINK_1				// 交差点内リンク
//#define SC_MA_LINK_TYPE2_MANEUVA			_LINK_2				// Maneuva
//#define SC_MA_LINK_TYPE2_INDESCRIBABLE	_LINK_3				// Indescribable
/* 【TYPE3】 0～7 */
/* 【TYPE4】 0～3 */

// ##ID 値
// ##ID マスク
#define SC_MA_NWID_SUB_DELETE				(0x80000000)		// 補足情報 削除フラグ
#define SC_MA_NWID_SUB_CNCTDIR				(0x78000000)		// 補足情報 情報種別/図郭接続方向
#define SC_MA_NWID_SUB_CNCTSIDE				(0x06000000)		// 補足情報 接続先リンク方向マスク
#define SC_MA_NWID_SUB_DIFFUPDATE			(0x01000000)		// 補足情報 差分更新状況フラグ
#define SC_MA_NWID_PNT_TYPE					(0x00800000)		// パーマネントID タイプ
#define SC_MA_NWID_PNT_SRC					(0x00700000)		// パーマネントID 情報ソース
#define SC_MA_NWID_PNT_ID					(0x000FFFFF)		// パーマネントID 番号
#define SC_MA_NWID_SUB_CNCTSIDE_ODR			(0x02000000)		// 接続先リンク方向紫檀
#define SC_MA_NWID_SUB_CNCTSIDE_RVS			(0x04000000)		// 接続先リンク方向終端

#define SC_MA_NWID_IS_CNCTSIDE_ODR(a)		((a) & 0x02000000)
#define SC_MA_NWID_IS_PNT_TYPE_LINK(a)		(0 == ((a) & SC_MA_NWID_PNT_TYPE))
#define SC_MA_NWID_IS_PNT_TYPE_CNCT(a)		(SC_MA_NWID_PNT_TYPE == ((a) & SC_MA_NWID_PNT_TYPE))

// ##BASE_LINK_INFO マスク
#define SC_MA_BASE_LINK_ROAD_TYPE			(0xF0000000)		// 道路種別
#define SC_MA_BASE_LINK_LINK_TYPE1			(0x0F000000)		// リンク種別１
#define SC_MA_BASE_LINK_LINK_TYPE2			(0x00E00000)		// リンク種別２
#define SC_MA_BASE_LINK_LINK_TYPE3			(0x001C0000)		// リンク種別３
#define SC_MA_BASE_LINK_LINK_TYPE4			(0x00030000)		// リンク種別４

// NETWORK_RECORD#LINK or #CONNECTION 値
#define SC_MA_NWRCD_CNCT_INDEX_OUTPCL		(0x0000)			// 外部パーセルへの接続を示す
#define SC_MA_NWRCD_LINK_NO_EXDATA			(MA_ALLF32)			// 拡張データ無しを示す
// NETWORK_RECORD#LINK or #CONNECTION マスク
#define SC_MA_NWRCD_LINK_INFO_KIND			(0x8000)
#define SC_MA_NWRCD_LINK_INFO_VALUABLE		(0x4000)
#define SC_MA_NWRCD_LINK_INFO_UPDATE		(0x3000)
#define SC_MA_NWRCD_LINK_INFO_SORCE			(0x0C00)
#define SC_MA_NWRCD_LINK_INFO_STTYPE		(0x0080)
#define SC_MA_NWRCD_LINK_INFO_EDTYPE		(0x0020)

#define SC_MA_NWRCD_CNCT_INFO_KIND			(0x8000)			// データ種別
#define SC_MA_NWRCD_CNCT_INFO_VALUABLE		(0x4000)			// データ有効無効
#define SC_MA_NWRCD_CNCT_INFO_STTYPE		(0x0080)			// 接続リンク／接続ID
#define SC_MA_NWRCD_CNCT_INFO_INOUT			(0x0010)			// 接続先情報内外フラグ
#define SC_MA_NWRCD_CNCT_INFO_DIRECTION		(0x000F)			// 接続方向

// リンク拡張データNETWORK_RECORD#EXTENSION_LINK_RECORD#EXTENSION

// リンク種別
#define SC_MA_BINSRC_TYPE_LINK				(1)					// リンクデータ
#define SC_MA_BINSRC_TYPE_CNCT				(2)					// 接続データ

// 実長
#define SC_MA_REAL_MUL						(10000)

#define SC_MA_REAL_DENSITY					(256)				// 密度データ乗算値

/*-------------------------------------------------------------------
 * マクロ定義
 *-------------------------------------------------------------------*/

/*-------------------------------------------------------------------
 * 便利系
 *-------------------------------------------------------------------*/
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
#define SC_MA_road1byte(p, d)		{(d) = read1byte(p); (p) += 1;}
#define SC_MA_road2byte(p, d)		{(d) = read2byte(p); (p) += 2;}
#define SC_MA_road4byte(p, d)		{(d) = read4byte(p); (p) += 4;}

/*-------------------------------------------------------------------
 * 形状情報バイナリ
 * ●ROAD_SHAPE
 *-------------------------------------------------------------------*/
// オフセット:ROAD_SHAPE
#define _RSHP_BINARY				(4)
// オフセット:ROAD_SHAPE#BINARY
#define _RSHP_DIR					(8)
// #ROAD_SHAPE -> #ROAD_SHAPE#BINARY 取得
#define SC_MA_A_RSHP_GET_BINARY(p)						((p) + _RSHP_BINARY)

// #ROAD_SHAPE -> #ROAD_SHAPE#DIR Recordオフセット取得
#define SC_MA_A_SHBIN_GET_DIR(p)						((p) + _RSHP_DIR)

// 形状ﾃﾞｰﾀﾚｺｰﾄﾞの先頭位置（道路種別15） オフセット取得
#define SC_MA_D_SHBIN_GET_RECORD15_OFS(p)				(read4byte((SC_MA_A_SHBIN_GET_DIR(p) + 0)))
// 形状ﾃﾞｰﾀﾚｺｰﾄﾞの先頭位置（道路種別15）
#define SC_MA_A_SHBIN_GET_RECORD15(p)					(SC_MA_A_RSHP_GET_BINARY(p) + (SC_MA_D_SHBIN_GET_RECORD15_OFS(p) * 4))

// リンク索引の先頭位置 オフセット取得
#define SC_MA_D_SHBIN_GET_INDEX_LINK_OFS(p)				(read4byte((SC_MA_A_SHBIN_GET_DIR(p) + 64)))
// リンク索引の先頭位置
#define SC_MA_A_SHBIN_GET_INDEX_LINK(p)					(SC_MA_A_RSHP_GET_BINARY(p) + (SC_MA_D_SHBIN_GET_INDEX_LINK_OFS(p) * 4))

// 上位リンク索引の先頭位置（レベル２） オフセット取得
#define SC_MA_D_SHBIN_GET_LV2UPPER_IDXLINK_OFS(p)		(read4byte((SC_MA_A_SHBIN_GET_DIR(p) + 68)))
// 上位リンク索引の先頭位置（レベル２）
#define SC_MA_A_SHBIN_GET_LV2UPPER_IDXLINK(p)			(SC_MA_A_RSHP_GET_BINARY(p) + (SC_MA_D_SHBIN_GET_LV2UPPER_IDXLINK_OFS(p) * 4))
// 上位リンクID索引：リンク数取得
#define SC_MA_GET_SHBIN_IDXUPLINK_LINKVOL(p)			(read4byte((p) + 4))
// 上位リンクID索引：リンクID取得
#define SC_MA_GET_SHBIN_IDXUPLINK_ID(p, idx)			(read4byte((p) + 8 + ((idx) * 4)))
// 上位リンクID索引：上位該当リンクIDインデックス取得
#define SC_MA_GET_SHBIN_IDXUPLINK_UPIDX(p, idx)			(read2byte((p) + 8 + (SC_MA_GET_SHBIN_IDXUPLINK_LINKVOL(p) * 4) + ((idx) * 2)))
// 上位リンクID索引：リンク数取得
#define SC_MA_GET_SHBIN_IDXUPLINK_UPVOL(p, idx)			(read2byte((p) + 8 + (SC_MA_GET_SHBIN_IDXUPLINK_LINKVOL(p) * 4) + (SC_MA_GET_SHBIN_IDXUPLINK_LINKVOL(p) * 2) + ((idx) * 2)))

// 上位該当自リンクIDの先頭位置 オフセット取得
#define SC_MA_D_SHBIN_GET_UPPER_LINK_OFS(p)				(read4byte((SC_MA_A_SHBIN_GET_DIR(p) + 88)))
// 上位該当自リンクIDの先頭位置
#define SC_MA_A_SHBIN_GET_UPPER_LINK(p)					(SC_MA_A_RSHP_GET_BINARY(p) + (SC_MA_D_SHBIN_GET_UPPER_LINK_OFS(p) * 4))
// 上位該当自リンクID：リンクID取得
#define SC_MA_GET_SHBIN_UPLINK_ID(p, idx)				(read4byte((p) + 8 + ((idx) * 4)))

// #ROAD_SHAPE#BINARY#SHAPE_RECORD -> オフセット指定でレコード取得
#define SC_MA_A_SHRCD_GET_RECORD(p, idx)				((p) + (idx * 4))

// #ROAD_SHAPE#BINARY#SHAPE_RECORD -> 次レコード取得
#define SC_MA_A_SHRCD_GET_NEXT_RECORD(p)				((p) + (read2byte(p) * 4))
// #ROAD_SHAPE#BINARY#SHAPE_RECORD -> リンクID取得
#define SC_MA_D_SHRCD_GET_LINKID(p)						(read4byte((p) + 4))
// #ROAD_SHAPE#BINARY#SHAPE_RECORD -> 図形データ取得
#define SC_MA_A_SHRCD_GET_XY(p)							((p) + 20)
#define SC_MA_D_SHRCD_GET_XYVOL(p)						(read2byte(SC_MA_A_SHRCD_GET_XY(p)))
#define SC_MA_D_SHRCD_GET_XY_X(p, idx)					(read2byte((SC_MA_A_SHRCD_GET_XY(p) + 4) + ((idx) * 4)))
#define SC_MA_D_SHRCD_GET_XY_Y(p, idx)					(read2byte((SC_MA_A_SHRCD_GET_XY(p) + 4) + ((idx) * 4) + 2))
// 上位レベルリンク情報数
#define SC_MA_D_SHRCD_GET_UPLINKVOL(p)					((read4byte((p) + 16) >> 28) & 0x0000000F)
// 上位レベルリンク情報
#define SC_MA_A_SHRCD_GET_UPLINK(p)						(((p) + 24) + (SC_MA_D_SHRCD_GET_XYVOL(p) * 4))
// リンク長
#define SC_MA_A_SHRCD_GET_LINKDIST(p)					((read4byte((p) + 12) >> 16) & 0x00007FFF)

/*-------------------------------------------------------------------
 * 道路ネットワークバイナリ
 * ●ROAD_NETWORK
 *-------------------------------------------------------------------*/
// オフセット:ROAD_NETWORK
#define _RNET_BINARY				(4)				// BINARY
// オフセット:ROAD_NETWORK#BINARY
#define _RNET_DIR					(8)				// DIR
// オフセット:ROAD_NETWORK#BINARY#NETWORK_LINK_RECORD
#define _LINK_DATA_SIZE				(0)				// データサイズ
#define _LINK_RECORD_VOL			(4)				// レコード数
#define _LINK_RECORD				(8)				// リンクデータ
// オフセット:ROAD_NETWORK#BINARY#NETWORK_CONNECTION_RECORD
#define _CNCT_DATA_SIZE				(0)				// データサイズ
#define _CNCT_RECORD_VOL			(4)				// レコード数
#define _CNCT_RECORD				(8)				// 接続データ
// オフセット:ROAD_NETWORK#BINARY#EXTENSION_LINK_RECORD
#define _LINKEX_DATA_SIZE			(0)				// データサイズ
#define _LINKEX_RECORD_VOL			(4)				// レコード数
#define _LINKEX_RECORD				(8)				// リンクデータ

// データサイズ:ROAD_NETWORK#BINARY#NETWORK_RECORD#LINK
#define SC_MA_NWRCD_LINK_DATASIZE	(44)
// データサイズ:ROAD_NETWORK#BINARY#NETWORK_RECORD#CONNECTION
#define SC_MA_NWRCD_CNCT_DATASIZE	(28)

/*-------------------------------------------------------------------
 * 誘導データバイナリ
 * ●GUIDE
 *-------------------------------------------------------------------*/
// オフセット:GUIDE
#define _GUIDE_BINARY				(4)				// BINARY
// オフセット:GUIDE#BINARY
#define _GUIDE_DIR					(8)				// DIR
// オフセット:GUIDE#BINARY#DIR
#define _GDDIR_GDRCD				(0)				// 誘導データへのオフセット
#define _GDDIR_CNRCD				(4)				// 交差点名称データへのオフセット
#define _GDDIR_IDXLINKRCD			(16)			// リンクID索引レコードへのオフセット

#define D_GDBIN_GET_GDRCD_OFS(p)			(read4byte(((p) + _GUIDE_DIR + _GDDIR_GDRCD)))
#define D_GDBIN_GET_CNRCD_OFS(p)			(read4byte(((p) + _GUIDE_DIR + _GDDIR_CNRCD)))
#define D_GDBIN_GET_IDXLINK_OFS(p)			(read4byte(((p) + _GUIDE_DIR + _GDDIR_IDXLINKRCD)))

// 誘導レコード
#define SC_MA_A_GDBIN_GET_GDRCD(p)			((p) + _GUIDE_BINARY + (D_GDBIN_GET_GDRCD_OFS(p) * 4))	// 誘導レコード先頭
#define SC_MA_D_GDRCD_GET_SIZE(p)			(read4byte((p) + 0))									// サイズ
#define SC_MA_D_GDRCD_GET_VOL(p)			(read4byte((p) + 4))									// リンク数

// 誘導リンクレコード
#define SC_MA_A_GDRCD_GET_GDLINK(p,ofs)		((p) + ofs * 4)											// 指定誘導リンク先頭
#define SC_MA_D_GDLINK_GET_SETFLG(p)		(read4byte((p) + 2))									// 収録情報フラグ
#define SC_MA_D_GDLINK_GET_LINKID(p)		(read4byte((p) + 4))									// リンクID

// リンク誘導・ノード誘導レコード
#define SC_MA_A_GDLINK_GET_GDLKINFO(p)		((p) + 8)												// リンク誘導情報
#define SC_MA_D_GDLKINFO_GET_SIZE(p)		(read2byte((p) + 0))									// リンク誘導情報サイズ
#define SC_MA_D_GDNDINFO_GET_SIZE(p)		(read2byte((p) + 0))									// ノード誘導情報サイズ
#define SC_MA_D_GDNDINFO_GET_OFSFLG(p)		(read2byte((p) + 2))									// ノード誘導情報内の各種誘導情報オフセット有無フラグ
#define SC_MA_A_GDNDINFO_GET_OFS(p)			((p) + 4)												// 各種誘導情報先頭

// ノード属性情報レコード
#define SC_MA_M_GDNDINFO_OFSFLG_NDRCD		0x8000													// ノード属性情報
#define SC_MA_D_NDRCD_GET_PTKIND(p)			(read2byte((p) + 8))									// 地点種別
#define SC_MA_D_NDRCD_GET_CNRCD_OFS(p)		(read4byte((p) + 16))									// 交差点名称オフセット

// 交差点名称レコード
#define SC_MA_A_GDBIN_GET_CNRCD(p)			((p) + _GUIDE_BINARY + (D_GDBIN_GET_CNRCD_OFS(p) * 4))	// 交差点名称レコード先頭
#define SC_MA_D_CNRCD_GET_SIZE(p)			(read2byte((p) + 0))									// サイズ
#define SC_MA_D_CNRCD_GET_VOL(p)			(read2byte((p) + 2))									// 交差点名称数
#define SC_MA_D_CNRCD_GET_NAMESIZE(p)		(read2byte((p) + 10))									// 交差点名称数
#define SC_MA_A_CNRCD_GET_NAME(p)			((p) + 12)												// 交差点名称

// リンクＩＤ索引レコード
#define SC_MA_A_GDBIN_GET_IDXLINK(p)		((p) + _GUIDE_BINARY + (D_GDBIN_GET_IDXLINK_OFS(p) * 4))// 索引レコード先頭
#define SC_MA_D_IDXLINK_GET_SIZE(p)			(read4byte((p) + 0))									// サイズ
#define SC_MA_D_IDXLINK_GET_VOL(p)			(read4byte((p) + 4))									// リンク数
#define SC_MA_A_IDXLINK_GET_RECORD(p)		((p) + 8)												// 索引先頭

/*-------------------------------------------------------------------
 * 道路データ：取得系マクロ
 * MAL_HDL 渡し
 *-------------------------------------------------------------------*/
// 先頭位置(オフセット取得) ROAD_NETWORK#BINARY#DIR
#define SC_MA_D_NWBIN_GET_NWLINK_OFS(p)		(read4byte(((p) + _RNET_DIR + 4)))		// リンクデータレコードの先頭位置
#define SC_MA_D_NWBIN_GET_NWCNCT_OFS(p)		(read4byte(((p) + _RNET_DIR + 8)))		// 接続データレコードの先頭位置
#define SC_MA_D_NWBIN_GET_NWLINKEX_OFS(p)	(read4byte(((p) + _RNET_DIR + 12)))		// リンク拡張データレコードの先頭位置
#define SC_MA_D_NWBIN_GET_NWCNCTEX_OFS(p)	(read4byte(((p) + _RNET_DIR + 18)))		// 接続ID拡張データレコードの先頭位置
#define SC_MA_D_NWBIN_GET_LINKREG_OFS(p)	(read4byte(((p) + _RNET_DIR + 20)))		// リンク規制情報データレコードの先頭位置
#define SC_MA_D_NWBIN_GET_IDXLINK_OFS(p)	(read4byte(((p) + _RNET_DIR + 24)))		// リンク索引レコードの先頭位置
#define SC_MA_D_NWBIN_GET_IDXCNCT_OFS(p)	(read4byte(((p) + _RNET_DIR + 28)))		// 接続ID索引レコード先頭位置

/* 先頭位置
 * ROAD_NETWORK#BINARY -> #DIR */
#define SC_MA_A_NWBIN_GET_NWDIR(p)			((p) + _RNET_DIR)
/* リンクデータレコード
 * ROAD_NETWORK#BINARY -> #NETWORK_LINK_RECORD */
#define SC_MA_A_NWBIN_GET_NWLINK(p)			((p) + _RNET_BINARY + (SC_MA_D_NWBIN_GET_NWLINK_OFS(p) * 4))
/* 接続データレコード
 * ROAD_NETWORK#BINARY -> #NETWORK_CONNECTION_RECORD */
#define SC_MA_A_NWBIN_GET_NWCNCT(p)			((p) + _RNET_BINARY + (SC_MA_D_NWBIN_GET_NWCNCT_OFS(p) * 4))
/* リンク拡張データレコード
 * ROAD_NETWORK#BINARY -> #EXTENSION_LINK_RECORD */
#define SC_MA_A_NWBIN_GET_NWLINKEX(p)		((p) + _RNET_BINARY + (SC_MA_D_NWBIN_GET_NWLINKEX_OFS(p) * 4))
/* リンクID索引レコード
 * ROAD_NETWORK#BINARY -> #INDEX_LINK_RECORD */
#define SC_MA_A_NWBIN_GET_IDXLINK(p)		((p) + _RNET_BINARY + (SC_MA_D_NWBIN_GET_IDXLINK_OFS(p) * 4))
/* 接続ID索引レコード
 * ROAD_NETWORK#BINARY -> #INDEX_CONNECTIONID_RECORD */
#define SC_MA_A_NWBIN_GET_IDXCNCT(p)		((p) + _RNET_BINARY + (SC_MA_D_NWBIN_GET_IDXCNCT_OFS(p) * 4))

// ROAD_NETWORK#BINARY#NETWORK_LINK_RECORD  レコードサイズ
#define SC_MA_D_NWBIN_GET_LINK_RECORDSIZE(p)	(read4byte(((SC_MA_A_NWBIN_GET_NWLINK(p)) + _LINK_DATA_SIZE)))
// ROAD_NETWORK#BINARY#NETWORK_LINK_RECORD  レコード数取得
#define SC_MA_D_NWBIN_GET_LINK_RECORDVOL(p)		(read4byte(((SC_MA_A_NWBIN_GET_NWLINK(p)) + _LINK_RECORD_VOL)))
// ROAD_NETWORK#BINARY#NETWORK_LINK_RECORD  リンクデータ先頭取得
#define SC_MA_A_NWBIN_GET_NWRCD_LINK(p)			((SC_MA_A_NWBIN_GET_NWLINK(p)) + _LINK_RECORD)
// ROAD_NETWORK#BINARY#NETWORK_LINK_RECORD -> #LINK  index指定リンクデータ取得
#define SC_MA_A_NWRCD_LINK_GET_RECORD(p,idx)	((p) + (SC_MA_NWRCD_LINK_DATASIZE * (idx)))

// ROAD_NETWORK#BINARY#NETWORK_CONNECTION_RECORD  レコードサイズ
#define SC_MA_D_NWBIN_GET_CNCT_RECORDSIZE(p)	(read4byte(((SC_MA_A_NWBIN_GET_NWCNCT(p)) + _CNCT_DATA_SIZE)))
// ROAD_NETWORK#BINARY#NETWORK_CONNECTION_RECORD  レコード数取得
#define SC_MA_D_NWBIN_GET_CNCT_RECORDVOL(p)		(read4byte(((SC_MA_A_NWBIN_GET_NWCNCT(p)) + _CNCT_RECORD_VOL)))
// ROAD_NETWORK#BINARY#NETWORK_CONNECTION_RECORD  接続データ先頭取得
#define SC_MA_A_NWBIN_GET_NWRCD_CNCT(p)			((SC_MA_A_NWBIN_GET_NWCNCT(p)) + _CNCT_RECORD)
// ROAD_NETWORK#BINARY#NETWORK_CONNECTION_RECORD -> #CONNECTION  index指定リンクデータ取得
#define SC_MA_A_NWRCD_CNCT_GET_RECORD(p,idx)	((p) + (SC_MA_NWRCD_CNCT_DATASIZE * idx))

// ROAD_NETWORK#BINARY#EXTENSION_LINK_RECORD レコードサイズ
#define SC_MA_A_NWBIN_GET_LINKEX_RECORDSIZE(p)	(read4byte(((SC_MA_A_NWBIN_GET_NWLINKEX(p)) + _LINKEX_DATA_SIZE)))
// ROAD_NETWORK#BINARY#EXTENSION_LINK_RECORD レコード数
#define SC_MA_A_NWBIN_GET_LINKEX_RECORDVOL(p)	(read4byte(((SC_MA_A_NWBIN_GET_NWLINKEX(p)) + _LINKEX_RECORD_VOL)))
// ROAD_NETWORK#BINARY#EXTENSION_LINK_RECORD レコード先頭
#define SC_MA_A_NWBIN_GET_NWRCD_LINKEX(p)		(SC_MA_A_NWBIN_GET_NWLINKEX(p) + _LINKEX_RECORD)
// ROAD_NETWORK#BINARY#EXTENSION_LINK_RECORD#EXTENSION index指定リンク拡張データ取得
#define SC_MA_A_NWRCD_LINKEX_GET_RECORD(p,ofs)	((p) + (ofs * 4))

// ROAD_NETWORK#BINARY#NETWORK_LINK_RECORD#LINK
#define SC_MA_NWRCD_LINK_MOVE_NEXT(p)			((p) = ((p) + SC_MA_NWRCD_LINK_DATASIZE))
#define SC_MA_NWRCD_LINK_GET_NEXT(p)			((MAL_HDL) p + SC_MA_NWRCD_LINK_DATASIZE))
#define SC_MA_D_NWRCD_LINK_GET_STID(p)			(read4byte((p) + 0))
#define SC_MA_D_NWRCD_LINK_GET_EDID(p)			(read4byte((p) + 4))
#define SC_MA_D_NWRCD_LINK_GET_STIDX(p)			(read2byte((p) + 8))
#define SC_MA_D_NWRCD_LINK_GET_EDIDX(p)			(read2byte((p) + 10))
#define SC_MA_D_NWRCD_LINK_GET_ID(p)			(read4byte((p) + 12))
#define SC_MA_D_NWRCD_LINK_GET_LIMIT(p)			(read4byte((p) + 16))
#define SC_MA_D_NWRCD_LINK_GET_ST_X(p)			(read2byte((p) + 20))
#define SC_MA_D_NWRCD_LINK_GET_ST_Y(p)			(read2byte((p) + 22))
#define SC_MA_D_NWRCD_LINK_GET_ED_X(p)			(read2byte((p) + 24))
#define SC_MA_D_NWRCD_LINK_GET_ED_Y(p)			(read2byte((p) + 26))
#define SC_MA_D_NWRCD_LINK_GET_STDIR(p)			((read4byte((MAL_HDL)(p) + 28) >> 24) & 0x000000FF)
#define SC_MA_D_NWRCD_LINK_GET_EDDIR(p)			((read4byte((MAL_HDL)(p) + 28) >> 16) & 0x000000FF)
#define SC_MA_D_NWRCD_LINK_GET_TRAVELTIME(p)	(read4byte((MAL_HDL)(p) + 28) & 0x00003FFF)
#define SC_MA_D_NWRCD_LINK_GET_LINKEXOFSFLG(p)	((read4byte((MAL_HDL)(p) + 32) >> 31) & 0x00000001)
#define SC_MA_D_NWRCD_LINK_GET_EXOFS(p)			(read4byte((MAL_HDL)(p) + 32) & 0x7FFFFFFF)
#define SC_MA_D_NWRCD_LINK_GET_FORMOFS(p)		(read4byte((MAL_HDL)(p) + 32))

// ROAD_NETWORK#BINARY#NETWORK_CONNECTION_RECORD#CONNECTION
#define SC_MA_NWRCD_CNCT_MOVE_NEXT(p)			((p) = ((p) + SC_MA_NWRCD_CNCT_DATASIZE))
#define SC_MA_D_NWRCD_CNCT_GET_STID(p)			(read4byte((p) + 0))
#define SC_MA_D_NWRCD_CNCT_GET_EDID(p)			(read4byte((p) + 4))
#define SC_MA_D_NWRCD_CNCT_GET_STIDX(p)			(read2byte((p) + 8))
#define SC_MA_D_NWRCD_CNCT_GET_EDIDX(p)			(read2byte((p) + 10))
#define SC_MA_D_NWRCD_CNCT_GET_ID(p)			(read4byte((p) + 12))
#define SC_MA_D_NWRCD_CNCT_GET_EXOFS(p)			(read4byte((p) + 16))

/** リンク拡張 ROAD_NETWORK#BINARY#EXTENSION_LINK_RECORD#EXTENSION **/
// データサイズ取得
#define SC_MA_D_NWRCD_EXLINK_GET_SIZE(p)		(read2byte((p) + 0))
// 道路形状オフセット有無フラグ
#define SC_MA_D_NWRCD_EXLINK_GET_FLG_FORMOFS(p)	((read2byte((MAL_HDL)(p) + 2) >> 15) & 0x0001)
// 上位レベルリンク有無フラグ
#define SC_MA_D_NWRCD_EXLINK_GET_FLG_UPLV(p)	((read2byte((MAL_HDL)(p) + 2) >> 14) & 0x0001)
// 下位レベルリンク有無フラグ
#define SC_MA_D_NWRCD_EXLINK_GET_FLG_DOWNLV(p)	((read2byte((MAL_HDL)(p) + 2) >> 13) & 0x0001)
// 地域クラス収録有無フラグ
#define SC_MA_D_NWRCD_EXLINK_GET_FLG_AREACLS(p)	((read2byte((MAL_HDL)(p) + 2) >> 12) & 0x0001)
// リンクID取得
#define SC_MA_D_NWRCD_EXLINK_GET_ID(p)			(read4byte((MAL_HDL)(p) + 4))
// 形状オフセット取得
#define SC_MA_D_NWRCD_EXLINK_GET_FORMOFS(p)		(read4byte((MAL_HDL)(p) + 8))
// 上位レベル接続データ取得
#define SC_MA_D_NWRCD_EXLINK_GET_UPLVINFO(p)	(read4byte((MAL_HDL)(p) + 8 + SC_MA_D_NWRCD_EXLINK_GET_FLG_FORMOFS(p) * 4))
// 上位レベル接続データ数取得
#define SC_MA_D_NWRCD_EXLINK_GET_UPLVINFO_VOL(p)	((SC_MA_D_NWRCD_EXLINK_GET_UPLVINFO(p) >> 28) & 0x0000000F)
// 上位レベル始点側接続レベル取得
#define SC_MA_D_NWRCD_EXLINK_GET_UPLVINFO_STLV(p)	((SC_MA_D_NWRCD_EXLINK_GET_UPLVINFO(p) >> 20) & 0x0000000F)
// 上位レベル終点側接続レベル取得
#define SC_MA_D_NWRCD_EXLINK_GET_UPLVINFO_EDLV(p)	((SC_MA_D_NWRCD_EXLINK_GET_UPLVINFO(p) >> 16) & 0x0000000F)
// 上位レベルリンク収録情報アドレス取得
#define SC_MA_A_NWRCD_EXLINK_GET_UPLVLINK(p)	((MAL_HDL)(p) + 8 + SC_MA_D_NWRCD_EXLINK_GET_FLG_FORMOFS(p) * 4 + SC_MA_D_NWRCD_EXLINK_GET_FLG_AREACLS(p) * 4 + 4)
// 地域クラス収録情報取得
#define SC_MA_D_NWRCD_EXLINK_GET_AREACLS(p)		(read4byte((MAL_HDL)(p) + 8 + SC_MA_D_NWRCD_EXLINK_GET_FLG_FORMOFS(p) * 4 + SC_MA_D_NWRCD_EXLINK_GET_FLG_UPLV(p) * 4))
// 下位レベルリンク情報アドレス取得
#define SC_MA_A_NWRCD_EXLINK_GET_DOWNLVLINK(p)																					\
	(SC_MA_D_NWRCD_EXLINK_GET_FLG_UPLV(p) 																						\
			? (SC_MA_A_NWRCD_EXLINK_GET_UPLVLINK(p) + SC_MA_D_NWRCD_EXLINK_GET_UPLVINFO_VOL(p) * 4)								\
			: ((MAL_HDL)(p) + 8 + SC_MA_D_NWRCD_EXLINK_GET_FLG_FORMOFS(p) * 4 + SC_MA_D_NWRCD_EXLINK_GET_FLG_AREACLS(p) * 4))
// 下位接続始端リンクID取得
#define SC_MA_A_NWRCD_EXLINK_GET_DOWNLVSTLINK(p)			(read4byte(p))
// 下位接続始端リンク存在下位パーセルシフト量X取得
#define SC_MA_A_NWRCD_EXLINK_GET_DOWNLVSTLINK_SFTX(p)		(read2byte(p + 8) & 0x00FF)
// 下位接続始端リンク存在下位パーセルシフト量Y取得
#define SC_MA_A_NWRCD_EXLINK_GET_DOWNLVSTLINK_SFTY(p)		((read2byte(p + 8) >> 8) & 0x00FF)
// 下位接続終端リンクID取得
#define SC_MA_A_NWRCD_EXLINK_GET_DOWNLVEDLINK(p)			(read4byte(p + 4))
// 下位接続終端リンク存在下位パーセルシフト量X取得
#define SC_MA_A_NWRCD_EXLINK_GET_DOWNLVEDLINK_SFTX(p)		(read2byte(p + 10) & 0x00FF)
// 下位接続終端リンク存在下位パーセルシフト量Y取得
#define SC_MA_A_NWRCD_EXLINK_GET_DOWNLVEDLINK_SFTY(p)		((read2byte(p + 10) >> 8) & 0x00FF)

// リンクID索引レコード ROAD_NETWORK#BINARY#INDEX_LINKID_RECORD
#define SC_MA_D_NWRCD_IDXLINK_GET_SIZE(p)		(read4byte((p) + 0))		// 索引レコードサイズ
#define SC_MA_D_NWRCD_IDXLINK_GET_VOL(p)		(read4byte((p) + 4))		// 索引レコード数
#define SC_MA_A_NWRCD_IDXLINK_GET_RECORD(p)		((p) + 8)					// 索引レコード先頭

// 接続ID索引レコード ROAD_NETWORK#BINARY#INDEX_CONNECTIONID_RECORD
#define SC_MA_D_NWRCD_IDXCNCT_GET_SIZE(p)		(read4byte((p) + 0))		// 索引レコードサイズ
#define SC_MA_D_NWRCD_IDXCNCT_GET_VOL(p)		(read4byte((p) + 4))		// 索引レコード数
#define SC_MA_A_NWRCD_IDXCNCT_GET_RECORD(p)		((p) + 8)					// 索引レコード先頭

// ##ID
#define SC_MA_D_NWID_GET_SUB_DELETE(a)			(((a) >> 31) & 0x00000001)
#define SC_MA_D_NWID_GET_SUB_CNCTDIR(a)			(((a) >> 27) & 0x0000000F)
#define SC_MA_D_NWID_GET_SUB_CNCTSIDE(a)		(((a) >> 25) & 0x00000003)
#define SC_MA_D_NWID_GET_SUB_DIFFUPDATE(a)		(((a) >> 24) & 0x00000001)
#define SC_MA_D_NWID_GET_PNT_TYPE(a)			(((a) >> 23) & 0x00000001)
#define SC_MA_D_NWID_GET_PNT_SRC(a)				(((a) >> 20) & 0x00000007)
#define SC_MA_D_NWID_GET_PNT_ID(a)				(((a) & SC_MA_NWID_PNT_ID))
// ##BASE_LINK_INFO
#define SC_MA_D_NWRCD_LINK_GET_BASE1(p)			(read4byte((p) + 36))
#define SC_MA_D_NWRCD_LINK_GET_BASE2(p)			(read4byte((p) + 36 + 4))
#define SC_MA_D_BASE_LINK_GET_ROAD_TYPE(p)		(((SC_MA_D_NWRCD_LINK_GET_BASE1(p)) >> 28) & 0x0000000F)
#define SC_MA_D_BASE_LINK_GET_LINK1_TYPE(p)		(((SC_MA_D_NWRCD_LINK_GET_BASE1(p)) >> 24) & 0x0000000F)
#define SC_MA_D_BASE_LINK_GET_LINK2_TYPE(p)		(((SC_MA_D_NWRCD_LINK_GET_BASE1(p)) >> 21) & 0x00000007)
#define SC_MA_D_BASE_LINK_GET_LINK3_TYPE(p)		(((SC_MA_D_NWRCD_LINK_GET_BASE1(p)) >> 18) & 0x00000007)
#define SC_MA_D_BASE_LINK_GET_LINK4_TYPE(p)		(((SC_MA_D_NWRCD_LINK_GET_BASE1(p)) >> 16) & 0x00000003)
#define SC_MA_D_BASE_LINK_GET_ONEWAY(p)			(((SC_MA_D_NWRCD_LINK_GET_BASE1(p)) >> 14) & 0x00000003)
#define SC_MA_D_BASE_LINK_GET_TOLLFLAG(p)		(((SC_MA_D_NWRCD_LINK_GET_BASE1(p)) >>  1) & 0x00000001)
#define SC_MA_D_BASE_LINK_GET_BYPASS(p)			(((SC_MA_D_NWRCD_LINK_GET_BASE1(p))        & 0x00000001))
#define SC_MA_D_BASE_LINK_GET_LINKDIST(p)		(((SC_MA_D_NWRCD_LINK_GET_BASE2(p)) >> 16) & 0x00007FFF)
#define SC_MA_D_BASE_LINK_GET_LANE(p)			(((SC_MA_D_NWRCD_LINK_GET_BASE2(p)) >>  3) & 0x00000003)
#define SC_MA_D_BASE_LINK_GET_EASYRUN(p)		(((SC_MA_D_NWRCD_LINK_GET_BASE2(p))        & 0x00000007))

// PARCEL_BASIS  実長情報
#define SC_MA_D_BASIS_GET_X_TOP(p)				(read4byte((p) +  4))
#define SC_MA_D_BASIS_GET_X_BOTTOM(p)			(read4byte((p) +  8))
#define SC_MA_D_BASIS_GET_Y_LEFT(p)				(read4byte((p) + 12))
#define SC_MA_D_BASIS_GET_Y_RIGHT(p)			(read4byte((p) + 16))

// リンク方向の正規化を解除
#define SC_MA_DENORMALIZE(a)					((int)(((a) * 359) / 254))

// 実長算出
#define SC_MA_REAL_LEN(len)						((DOUBLE)len / (DOUBLE)SC_MA_REAL_MUL)

/* 旅行時間計算用 定数値 */
static const UINT32 sTravelTimeUnit[4] = {1, 2, 4, 40};
/* リンク長計算用 定数値 */
static const UINT32 sLinkDistUnit[5] = {1, 2, 4, 20, 250};

// 地図フォーマット準平均旅行時間算出
#define SC_MA_CALC_LINK_TRAVELTIME(src, ans)				\
{															\
	UINT32 unit = (((src) >> 12) & 0x00000003);				\
	ans = (((src) & 0x00000FFF) * sTravelTimeUnit[unit]);	\
}
// 地図フォーマット準平均旅行時間算出
#define SC_MA_GET_LINK_TRAVELTIME(src)			(((src) & 0x00000FFF) * sTravelTimeUnit[(((src) >> 12) & 0x00000003)])

// 地図フォーマット準平均旅行時間算出
#define SC_MA_CALC_LINK_TRAVELTIME_U(src, unit, ans)		\
{															\
	UINT8 u[4] = {1, 2, 4, 40};								\
	ans = (((src) & 0x00000FFF) * u[unit]);					\
}
// 地図フォーマット準拠距離算出
#define SC_MA_CALC_LINK_DIST(src, ans)						\
{															\
	UINT8 u[5] = {1, 2, 4, 20, 250};						\
	UINT8 unit = (((src) >> 12) & 0x00000007);				\
	ans = (src & 0x00000FFF) * u[unit];						\
}
// 地図フォーマット準拠距離算出
#define SC_MA_GET_LINK_DIST(src)				(((src) & 0x00000FFF) * sLinkDistUnit[(((src) >> 12) & 0x00000007)])

// 地図フォーマット準拠距離算出
#define SC_MA_CALC_LINK_DIST_U(src, unit, ans)				\
{															\
	UINT8 u[5] = {1, 2, 4, 20, 250};						\
	ans = (src & 0x00000FFF) * u[unit];						\
}

#endif

