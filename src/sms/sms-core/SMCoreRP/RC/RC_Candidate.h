/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef RC_CANDIDATE_H_
#define RC_CANDIDATE_H_

// SCRC_NETDATA -> SCRC_CANDDATA 情報移行マクロ（）
#define RC_SET_FLAG_NW2CAND(a,d)					\
		{											\
			d = a;									\
		}

// 候補開始リンク
typedef struct _SCRC_CANDSTARTLINK {
	UINT32 candIdx;						// 候補リンクインデックス
	UINT32 netIdx;						// SCRP_NETDATAへのインデックス
	UINT16 dataIdx;						// 地図ネットワークデータインデックス
	UINT32 formOfs;						// 形状データオフセット(4byte)
	UINT32 parcelId;					// パーセルID
	UINT32 linkId;						// リンクID
	UINT8 roadKind;						// 道路種別        TODO できればサイズ縮小のため削除
	UINT8 linkKind;						// リンク種別      TODO できればサイズ縮小のため削除
	UINT16 flag;						// フラグ情報（SCRP_NETDATA#flagと同じ情報を格納）
	UINT32 cost;						// 累計コスト
	INT8 connectLevel;					// 上位or下位レベルを格納する 0:接続なし
	UINT8 reserve[3];					// リザーブ
	struct {							// 下位接続の場合    | 上位接続の場合
		UINT32 parcelId;				// 下位パーセルID    | 上位パーセルID
		UINT32 linkId;					// 下位リンクID      | 上位リンクID
		UINT8 pclSftX;					// 下位パーセル位置X | reserve
		UINT8 pclSftY;					// 下位パーセル位置Y | reserve
	} st;
#if 0
	struct {
		UINT32 linkId;					// 下位開始リンクID
		UINT8 pclSftX;					// 下位開始リンクパーセル位置X
		UINT8 pclSftY;					// 下位開始リンクパーセル位置Y
	} ed;
#endif
} SCRP_CANDSTARTLINK;

// 候補リンク
typedef struct _SCRC_CANDDATA {
	UINT32 next;						// 次接続候補リンクインデックス
	UINT16 dataIdx;						// 地図ネットワークデータインデックス
	UINT32 formOfs;						// 形状データオフセット(4byte)
	UINT32 parcelId;					// パーセルID
	UINT32 linkId;						// リンクID
	UINT16 flag;						// フラグ情報（探索情報テーブルをそのまま格納）
	UINT32 cost;						// 累計コスト
} SCRP_CANDDATA;

// 候補管理テーブル
typedef struct _SCRC_CANDTBLINFO {
	UINT32 candSize;					// 候補リンク数
	UINT32 candIdx;						// 候補リンクインデックス
	UINT32 stLinkSize;					// 候補開始リンクテーブル数
	UINT32 stLinkIdx;					// 候補開始リンクインデックス
} SCRP_CANDTBLINFO;

/* 候補経路管理
 *
 * candTblInfo[3]
 * レベル１トップ  [0] レベル１トップ
 *                 [1] reserve
 *                 [2] reserve
 * レベル２トップ  [0] レベル１O側
 *                 [1] レベル２トップ
 *                 [2] レベル１D側
 */
typedef struct _SCRC_CANDMANAGER {

	// 最終候補経路情報
	SCRP_CANDTBLINFO candTblInfo[RC_CAND_AREA_MAX];	// 候補管理テーブル
	SCRP_CANDDATA* cand;				// 候補リンク先頭
	UINT32 candSize;					// 候補リンクサイズ
	UINT32 candCurrent;					// 候補リンクカレントインデックス
	SCRP_CANDSTARTLINK* stLink;			// 候補開始リンク先頭
	UINT16 stLinkSize;					// 候補開始リンクサイズ
	UINT16 stLinkCurrent;				// 候補開始リンクカレントインデックス

	// 分割情報
	SCRP_CANDTBLINFO* splitCandTblInfo;	// 分割候補管理テーブル
	UINT16 splitCandTblInfoVol;			// 分割エリア数
	SCRP_CANDDATA* splitCand;			// 候補リンク先頭
	UINT32 splitCandSize;				// 候補リンクサイズ
	UINT32 splitCandCurrent;			// 候補リンクカレントインデックス
	SCRP_CANDSTARTLINK* splitStLink;	// 候補開始リンク先頭
	UINT16 splitStLinkSize;				// 候補開始リンクサイズ
	UINT16 splitStLinkCurrent;			// 候補開始リンクカレントインデックス
} SCRP_CANDMANAGER;

#endif /* RC_CANDIDATE_H_ */
