/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef RP_LIB_H_
#define RP_LIB_H_

/**
 * 変数定義
 */
#if 0
static T_DHC_REQ_PARCEL sRPMapReqTab = {};		/* 探索内で唯一の地図リクエスト用変数 */
static T_DHC_RES_DATA sRPMapResTab = {};			/* 探索内で唯一の地図応答用変数 */
#endif

/*-------------------------------------------------------------------
 * 便利系
 *-------------------------------------------------------------------*/
#define swap(type,a,b)		{type tmp=a;a=b;b=tmp;}

/*-------------------------------------------------------------------
 * 構造体
 *-------------------------------------------------------------------*/
typedef struct _MapShapeDir {/*----------------------// 形状先頭位置(#DIR) --------------------------*/
	UINT32 shapeRecord15Ofs;						// 道路種別15
	UINT32 shapeRecord14Ofs;						// 道路種別14
	UINT32 shapeRecord13Ofs;						// 道路種別13
	UINT32 shapeRecord12Ofs;						// 道路種別12
	UINT32 shapeRecord11Ofs;						// 道路種別11
	UINT32 shapeRecord10Ofs;						// 道路種別10
	UINT32 shapeRecord9Ofs;							// 道路種別9
	UINT32 shapeRecord8Ofs;							// 道路種別8
	UINT32 shapeRecord7Ofs;							// 道路種別7
	UINT32 shapeRecord6Ofs;							// 道路種別6
	UINT32 shapeRecord5Ofs;							// 道路種別5
	UINT32 shapeRecord4Ofs;							// 道路種別4
	UINT32 shapeRecord3Ofs;							// 道路種別3
	UINT32 shapeRecord2Ofs;							// 道路種別2
	UINT32 shapeRecord1Ofs;							// 道路種別1
	UINT32 shapeRecord0Ofs;							// 道路種別0
	UINT32 indexLinkOfs;							// リンク索引
	UINT32 indexUpperLink2Ofs;						// 上位リンク索引レベル2
	UINT32 indexUpperLink3Ofs;						// 上位リンク索引レベル3
	UINT32 indexUpperLink4Ofs;						// 上位リンク索引レベル4
	UINT32 indexUpperLink5Ofs;						// 上位リンク索引レベル5
	UINT32 indexUpperLink6Ofs;						// 上位リンク索引レベル6
	UINT32 indexUpperLinkRecordOfs;					// 上位当該自リンクID
} T_MapShapeDir;
typedef struct _MapShapeIndexRecord {/*---------------// 形状ID索引レコード(#INDEX_LINKID_RECORD)-----*/
	UINT32 dataSize;								// データサイズ
	UINT32 linkVol;									// リンク数
	// 以下可変長
} T_MapShapeIndexRecord;
typedef struct _MapBaseLinkInfo {/*-----------------// リンク基本情報(##BASE_LINK_INFO)--------------*/
	struct {
		UINT32 bypass :1;							// 1	バイパスフラグ
		UINT32 toll :1;								// 1	有料道路フラグ
		UINT32 ipd :1;								// 1	格上げIPDフラグ
		UINT32 plan :1;								// 1	計画道路
		UINT32 uturn :1;							// 1	Ｕターン専用リンク
		UINT32 stairs :1;							// 1	階段リンク
		UINT32 elevate :1;							// 1	高架リンク
		UINT32 bridge :1;							// 1	橋リンク
		UINT32 tunnel :1;							// 1	トンネルリンク
		UINT32 split :2;							// 2	分離帯有無フラグ
		UINT32 infla :1;							// 1	インフラリンク該当フラグ
		UINT32 dst :1;								// 1	DST対象フラグ
		UINT32 rightLeft :1;						// 1	通行フラグ
		UINT32 oneway :2;							// 2	一方通行コード
		UINT32 linkKind4 :2;						// 2	リンク種別4
		UINT32 linkKind3 :3;						// 3	リンク種別3
		UINT32 linkKind2 :3;						// 3	リンク種別2
		UINT32 linkKind1 :4;						// 4	リンク種別1
		UINT32 roadKind :4;							// 4	道路種別
	} b_code;
	struct {
		UINT32 easy :3;								// 3	走りやすさ
		UINT32 lanes :2;							// 2	車線数
		UINT32 anyReg :1;							// 1	その他規制エリアリンク
		UINT32 military :1;							// 1	軍事エリアリンク
		UINT32 frozen :1;							// 1	凍結リンク
		UINT32 flooding :1;							// 1	冠水リンク
		UINT32 school :1;							// 1	スクールゾーン
		UINT32 funcClass :3;						// 3	ファンクションクラス
		UINT32 width :3;							// 3	道路幅員コード
		UINT32 linkLength :12;						// 12	リンク長
		UINT32 linkLengthUnit :3;					// 3	リンク長の単位フラグ
		UINT32 reserved :1;							// 1	リザーブ
	} b_data;
} T_MapBaseLinkInfo;
typedef struct _MapShapeRecord {/*-------------------// 形状（#SHAPE） --------------------------*/
	UINT16 dataSize;								// データサイズ
	UINT16 updateId;								// 更新通番
	UINT32 linkId;									// リンクID
	T_MapBaseLinkInfo linkBaseInfo;					// リンク基本情報
	struct {
		UINT32 reserved :25;						// 25	Reserved
		UINT32 area :2;								// 2	地域クラス有無フラグ
		UINT32 road :1;								// 1	路線情報有無フラグ
		UINT32 upLevelLinkVol :4;					// 4	上位レベルリンク数
	} shapeInfo;
	UINT16 pointVol;								// 形状点数
	UINT16 viewFlag;								// 表示フラグ
	// 以降可変長
} T_MapShapeRecord;
typedef struct _MapNWConnection {/*------------------// 接続情報(#CONNECTION) -------------------*/
	UINT32 sameStId;								// 同一接続Id(始点)
	UINT32 sameEdId;								// 同一接続Id(終点)
	UINT16 sameStIdx;								// 同一接続Index(始点)
	UINT16 sameEdIdx;								// 同一接続Index(終点)
	UINT32 id;										// リンクID
	UINT32 exOfs;									// 拡張オフセット
	UINT16 coordX;									// 座標(始点)
	UINT16 coordY;									// 座標(終点)
	UINT16 country;									// 国
	UINT16 reserved;								// リザーブ
	// 以下パディング
} T_MapNWConnection;
typedef struct _MapNWLink {/*------------------------// リンク情報(#LINK) -----------------------*/
	UINT32 sameStId;								// 同一接続Id(始点)
	UINT32 sameEdId;								// 同一接続Id(終点)
	UINT16 sameStIdx;								// 同一接続Index(始点)
	UINT16 sameEdIdx;								// 同一接続Index(終点)
	UINT32 id;										// リンクID
	UINT32 feature;									// その他特徴量
	UINT16 coordXSt;								// 座標X(始点)
	UINT16 coordYSt;								// 座標Y(終点)
	UINT16 coordXEd;								// 座標X(始点)
	UINT16 coordYEd;								// 座標Y(終点)
	struct {
		UINT32 travelTime :12;
		UINT32 travelUnit :2;
		UINT32 mander :2;
		UINT32 dirRev :8;
		UINT32 dirOdr :8;
	} dir;
	UINT32 exOrShape;								// 形状オフセット
	T_MapBaseLinkInfo linkBaseInfo;					// リンク基本情報
} T_MapNWLink;

#endif /* RP_LIB_H_ */
