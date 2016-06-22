/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef RC_COSTCALC_H_
#define RC_COSTCALC_H_

// 道路種別関連
#define TRI_PART_SIZE		17				/* 角度分解 */
#define ROADTYPE_SIZE		16				/* 道路種別MAX */
#define LINKTYPE_SIZE		13				/* リンク種別MAX */

// 角度コストテーブル
#define AGL_STRAIGHT		(0)				/* 直進コスト */
#define AGL_UTURN			(12000)			/* Uターンコスト */
#define LR0					(AGL_UTURN)		/* 349.0～ 11.5  Uターン */
#define R1					(6000)			/*  11.5～ 34.0 */
#define R2					(5500)			/*  34.0～ 56.5 */
#define R3					(5000)			/*  56.5～ 79.0 */
#define R4					(4000)			/*  79.0～101.5 右折 */
#define R5					(3500)			/* 101.5～124.0 */
#define R6					(500)			/* 124.0～146.5 */
#define R7					(300)			/* 146.5～169.0 */
#define LR8					(AGL_STRAIGHT)	/* 169.0～191.5 直進 */
#define L1					(200)			/* 191.5～214.0 */
#define L2					(300)			/* 214.0～236.5 */
#define L3					(1500)			/* 236.5～259.0 */
#define L4					(3000)			/* 259.0～281.5 左折 */
#define L5					(3000)			/* 281.5～304.0 */
#define L6					(3500)			/* 304.0～326.5 */
#define L7					(4000)			/* 326.5～349.0 */

// 特殊なリンクに対するコスト これらのリンクを通す場合別コストを用意する
#define ROAD4_INDEX			(3)				/* 道路種別4 */
#define ROAD12_INDEX		(11)			/* 道路種別12 */
#define ROAD15_INDEX		(14)			/* 道路種別15 */
#define ROAD3_BASECOST		(720)			/* 道路種別3(他)基準コスト */
#define ROAD12_BASECOST		(430)			/* 道路種別12(自転車)基準コスト */
#define ROAD15_BASECOST		(720)			/* 道路種別15(歩道)基準コスト */

#define ON_TOLL_COST		(12000)			/* 一般優先用：高速乗りコスト */

// 北０時計回り→東０度半時計回り変換マクロ
#define RP_ANGLE_N2E_CNV(angle)		(359 < (360 - (UINT32)(angle) + 90) ? ((360 - (UINT32)(angle) + 90) - 360) : (360 - (UINT32)(angle) + 90))
// 角度を探索用に1/17へ正規化
#define AGL_CALC_ANGLE(a)			(((a) + 11) / 22.5)

// 計算対象リンク情報
typedef struct _SCRC_TARGETLINKINFO {
	SCRP_PCLINFO* pclInfo;						// コスト計算
	SCRP_LINKINFO* linkTable;					// リンクテーブル
	SCRP_NETDATA* linkNet;						// ネットワークテーブル
	UINT32 linkIndex;							// リンク番地
} SCRC_TARGETLINKINFO;

// 交差点リンク情報
typedef struct _SCRC_CROSSLINKTBL {
	UINT16 listVol;								// リンク数
	SCRC_TARGETLINKINFO linkList[RP_CLOSS_MAX];	// 計算対象リンク情報
	UINT32 baseCost;							// ベースリンクコスト
	SCRC_TARGETLINKINFO baseLink;				// ベースリンク情報
} SCRC_CROSSLINKTBL;

// コスト結果群
typedef struct _SCRC_RESULTCOSTS {
	UINT32 travelCost;							// 旅行コスト
	UINT32 crossCost;							// 交差コスト
	UINT32 advanceCost;							// その他コスト
	UINT32 totalCost;							// 累計コスト
} SCRC_RESULTCOSTS;

// 侵入脱出計算テーブル
typedef struct _SCRC_IOCALCTBL {
	SCRC_TARGETLINKINFO* inLink;				// 侵入リンク情報
	SCRC_TARGETLINKINFO* outLink;				// 退出リンク情報
	SCRC_RESULTCOSTS costTable;					// コストテーブル
	UINT8* inLinkBin;							// 侵入リンクバイナリアドレス
	UINT8* outLinkBin;							// 退出リンクバイナリアドレス
	U_LINK_NET_FLAG reg;						// 規制フラグ
} SCRC_IOCALCTBL;

#endif /* RC_COSTCALC_H_ */

