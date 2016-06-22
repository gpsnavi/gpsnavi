/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef RP_NETWORK_H_
#define RP_NETWORK_H_

#define SCRP_LINKINFO_MAX			(0x000FFFFF)		/* リンク数最大 */
#define SCRP_PCLINFO_MAX			(0x000000FF)		/* パーセル数最大 */

#define SCRP_NBROWN					(0)					/* 区間管理近傍情報：出発地側 */
#define SCRP_NBRDST					(1)					/* 区間管理近傍情報：目的地側 */
#define SCRP_LINKODR				(0)					/* order */
#define SCRP_LINKRVS				(1)					/* reverse */

#define SCRP_HEAP_SIZE				(20000)				/* ヒープサイズ */
#define SCRP_HEAP_UV				(0x7FFFFFFF)		/* 未訪問(Unvisited) 20140724 ヒープ4Byte拡張 */
#define SCRP_HEAP_V					(0xFFFFFFFF)		/* 訪問済み(Visited) */
#define SCRP_HEAP_NU				(0xCFFFFFFF)		/* 非使用(No use) */

#define RC_CAND_STLINK				(ALL_F32)			/* 候補開始リンク判定値 */
#define RC_CAND_INIT				(0x7FFFFFFF)		/* 候補リンク初期値 */
#define RC_CAND_SIZE				(20000)				/* 候補リンクデータ数 */
#define RC_CAND_STLINK_SIZE			(2000)				/* 候補開始リンクデータ数 */
#define RC_CAND_AREA_MAX			(3)					/* 候補管理テーブル数 */
#define RC_CAND_IDX_LV1TOP			(0)					/* LV1TOP候補格納位置 */
#define RC_CAND_IDX_LV1O			(0)					/* LV1O側候補格納位置 */
#define RC_CAND_IDX_LV2TOP			(1)					/* LV2TOP候補格納位置 */
#define RC_CAND_IDX_LV1D			(2)					/* LV1D側候補格納位置 */

// SCRP_PCLINFO：flag
#define RCPI_DEST					(0x0001)			/* 目的地パーセルフラグ */
#define RCPI_OUTERMOST				(0x0002)			/* 最外殻パーセルフラグ */
#define RCPI_DOWNCNCT				(0x0004)			/* 下位接続該当パーセルフラグ */

#define RCPI_GET_DESTFLG(flag)		(flag & RCPI_DEST)				/* 目的地パーセルフラグ取得 */
#define RCPI_GET_OUTERMOST(flag)	(flag & RCPI_OUTERMOST)			/* 最外殻パーセルフラグ */
#define RCPI_GET_DOWNCNCT(flag)		(flag & RCPI_DOWNCNCT)			/* 下位接続該当パーセルフラグ */

// SCRP_NETDATA：flag
#define RCND_LINKOR					(0x0001)			/* 方向フラグ 0:順 1:逆 */
#define RCND_CONNECTLINK			(0x0002)			/* 接続情報フラグ */
#define RCND_STARTLINK				(0x0004)			/* 探索開始リンク */
#define RCND_TIMEREGIN				(0x0008)			/* 時間規制内フラグ 未使用 */
#define RCND_TIMEREGOUT				(0x0010)			/* 時間規制外フラグ 未使用 */
#define RCND_SEASONREGIN			(0x0020)			/* 季節規制内フラグ 未使用 */
#define RCND_REGISTEDCAND			(0x0040)			/* 候補経路登録済みフラグ */
#define RCND_CANDSPLIT				(0x0080)			/* 断裂地点候補(目的地候補) */
#define RCND_CANDJOIN				(0x0100)			/* 接続地点候補 */
#define RCND_DESTLINK				(0x0200)			/* 目的地近傍フラグ */
#define RCND_AREAENDLINK			(0x0400)			/* 探索エリア外殻フラグ */
#define RCND_EXCLUDELINK			(0x0800)			/* 探索対象外フラグ */
#define RCND_UPLEVEL				(0x1000)			/* 上位接続フラグ */
#define RCND_DOWNLEVEL				(0x2000)			/* 下位接続フラグ */

#define RCND_GET_ORIDX(flag)				(flag & RCND_LINKOR)			/* 方向フラグ取得 */
#define RCND_GET_CONNECTLINK(flag)			(flag & RCND_CONNECTLINK)		/* 接続情報フラグ取得 */
#define RCND_GET_REGISTEDCANDFLG(flag)		(flag & RCND_REGISTEDCAND)		/* 候補経路登録フラグ */
#define RCND_GET_STARTLINKFLG(flag)			(flag & RCND_STARTLINK)			/* 開始リンクフラグ */
#define RCND_GET_DESTLINKFLG(flag)			(flag & RCND_DESTLINK)			/* 目的地リンクフラグ */
#define RCND_GET_CANDJOINFLG(flag)			(flag & RCND_CANDJOIN)			/* 接続地点候補フラグ */
#define RCND_GET_CANDSPLITFLG(flag)			(flag & RCND_CANDSPLIT)			/* 断裂地点候補(目的地候補)フラグ */
#define RCND_GET_AREAENDFLG(flag)			(flag & RCND_AREAENDLINK)		/* 探索エリア外殻フラグ */
#define RCND_GET_EXCLUDELINK(flag)			(flag & RCND_EXCLUDELINK)		/* 探索対象外フラグ */
#define RCND_GET_UPLEVELFLG(flag)			(flag & RCND_UPLEVEL)			/* 上位レベル接続フラグ */
#define RCND_GET_DOWNLEVELFLG(flag)			(flag & RCND_DOWNLEVEL)			/* 下位レベル接続フラグ */
#define RCND_GET_CANDFLGS(flag)				(flag & (RCND_DESTLINK | RCND_CANDJOIN | RCND_CANDSPLIT | RCND_AREAENDLINK))	/* 候補系フラグ取得 */
#define RCND_MASK_STARTFLG(flag)			(flag & ~RCND_STARTLINK)		/* 開始リンクフラグ以外マスク */

// SCRP_NETDATA：costSum 高速道路通過フラグ TODO
#define RCND_GET_COST(cost)					(cost & 0x7FFFFFFF)				/* コスト取得 */
#define RCND_GET_COSTWHFLG(cost)			(cost & 0x80000000)				/* 高速通過フラグ */

// ネットワークインデックス関連
#define RCID_NIDX_INVALID			(ALL_F32)					/* ネットワークインデックス無効値 */
#define RCID_GET_LINKIDX(d)			( (d)        & 0x000FFFFF)
#define RCID_GET_PCLIDX(d)			(((d) >> 24) & 0x000000FF)
#define RCID_GET_ORIDX(d)			(((d) >> 20) & 0x00000001)
#define RCID_MAKE_RCID(a,b,c)		(((a) & 0x000FFFFF) | (((b) << 24) & 0xFF000000) | (((c) << 20) & 0x00100000))
#define RCID_MASK_LINKIDX(a)		((a) & 0x000FFFFF)
#define RCID_MASK_PCLIDX(a)			(((a) << 24) & 0xFF000000)
#define RCID_MASK_ORIDX(a)			(((a) << 20) & 0x00100000)

/* 探索ネットワーク操作 */

// ネットワークインデックス → パーセル情報
#define RCNET_GET_PCLINFO(net, index)					(((net)->parcelInfo) + RCID_GET_PCLIDX(index))
// ネットワークインデックス → リンク情報
#define RCNET_GET_LINKINFO(net, index)					(((net)->linkTable) + (RCID_GET_LINKIDX(index)))
// ネットワークインデックス → ネットワーク情報
#define RCNET_GET_NETDATA(net, index)					(RCNET_GET_LINKINFO((net), (index))->linkNet + RCID_GET_ORIDX(index))

// 次接続 → 接続先パーセル情報
#define RCNET_GET_NEXTPCLINFO(net, link)				(RCNET_GET_PCLINFO(net, (link)->nextLink))
// 次接続 → 接続先リンク情報
#define RCNET_GET_NEXTLINKINFO(net, link)				(RCNET_GET_LINKINFO(net, (link)->nextLink))
// 次接続 → 接続先リンク方向
#define RCNET_GET_NEXTORIDX(link)						(RCID_GET_ORIDX((link)->nextLink))

// 探索履歴 → パーセル情報
#define RCNET_GET_HISTPCLINFO(net, link)				(RCNET_GET_PCLINFO(net, (link)->inLinkHist))
// 探索履歴 → リンク情報
#define RCNET_GET_HISTLINKINFO(net, link)				(RCNET_GET_LINKINFO(net, (link)->inLinkHist))
// 探索履歴 → ネットワーク情報
#define RCNET_GET_HISTNETDATA(net, link)				(RCNET_GET_HISTLINKINFO(net, link)->linkNet + RCNET_GET_HISTORIDX(link))
// 探索履歴 → 侵入リンク方向
#define RCNET_GET_HISTORIDX(link)						(RCID_GET_ORIDX((link)->inLinkHist))

// ヒープ用
#define RCNET_GET_HEAPNETDATA(net, heap)				(RCNET_GET_NETDATA(net, heap))

/*-------------------------------------------------------------------
 * 構造体定義
 *-------------------------------------------------------------------*/
// SCRC_NETDATA#flag 分解用
typedef union {
	struct {
		UINT16 reserved :5;				// 		リザーブ
		UINT16 calcTarget :1;			// 11	計算対象
		UINT16 destNbr :1;				// 10	目的近傍
		UINT16 candJoin :1;				// 9	分割候補
		UINT16 candSplit :1;			// 8	断裂候補
		UINT16 candYet :1;				// 7	候補経路登録済み
		UINT16 seasonRegIn :1;			// 6	季節規制内フラグ（季節規制外は格納しない）
		UINT16 timeRegOut :1;			// 5	時間規制外フラグ（誘導用）
		UINT16 timeRegIn :1;			// 4	時間規制内フラグ
		UINT16 startLink :1;			// 3	開始リンクフラグ
		UINT16 connectLink :1;			// 2	接続リンクフラグ
		UINT16 orFlag :1;				// 1	方向フラグ
	} b_flag;
	UINT16 flag;
} U_LINK_NET_FLAG;

// 近傍リンク情報
typedef struct _SCRP_NEIGHBORLINK {
	UINT32 parcelId;					// パーセルID
	UINT16 dataIndex;					// データINDEX(0始まり)
	UINT32 linkId;						// リンクID
	UINT8 roadKind;						// 道路種別
	UINT8 linkKind;						// リンク種別
	struct {
		UINT16 reserved :13;			// フラグ
		UINT16 noUse :1;				// 使用不可フラグ(現状未使用)
		UINT16 followFlag :1;			// 逆走(or不明):0 順走:1
		UINT16 orFlag :1;				// リンク方向 順:0 逆:1
	};
	UINT32 linkDist;					// リンク長
	DOUBLE remainDist;					// リンク内分距離
	UINT16 subVol;						// 形状数
	UINT16 subIndex;					// 垂線サブリンクIndex
	UINT16 subDist;						// 垂線サブリンク距離
	DOUBLE subRemainDist;				// 垂線サブリンク内分距離
	DOUBLE ratio;						// 垂線サブリンク内分率
	DOUBLE angle;						// 垂線サブリンク角度
	UINT16 x;							// 垂線足座標X
	UINT16 y;							// 垂線足座標Y
	DOUBLE leavDist;					// 基準点からの距離
	UINT32 cost;						// 初期コスト
} SCRP_NEIGHBORLINK;

// 近傍情報
typedef struct _SCRP_NEIGHBORINFO {
	SCRP_POINT point;					// 近傍座標
	UINT16 nbrLinkVol;					// 近傍サイズ
	SCRP_NEIGHBORLINK* neighborLink;	// 近傍リンクアドレス
	SCRP_PCLRECT nbrArea;				// 近傍エリア
} SCRP_NEIGHBORINFO;

// エリア内パーセル状態
typedef struct _SCRP_AREAPCLSTATE {
	UINT8 linkDensity;					// パーセル内リンク密度（1/256単位）
	UINT8 reserve;						// バウンダリ調整領域
	UINT8 join_f;						// 接続有無フラグ
	UINT8 split_f;						// 切断有無フラグ
} SCRP_AREAPCLSTATE;

// 分割エリア情報
typedef struct _SCRP_DIVAREA {
	SCRP_PCLRECT pclRect;				// 矩形情報
	UINT16 pclIdx;						// パーセル情報インデックス
	UINT16 pclVol;						// パーセル情報数
} SCRP_DIVAREA;

// レベルエリア情報
typedef struct _SCRP_LEVELAREA {
	SCRP_PCLRECT pclRect;				// 矩形情報
	UINT16 divIdx;						// 分割情報インデックス
	UINT16 divVol;						// 分割情報数
} SCRP_LEVELAREA;

// 探索レベルテーブル
typedef struct _SCRP_LEVELTBL {
	UINT8 topLevel;						// トップレベル
	SCRP_LEVELAREA areaTable[3]; 		// 探索レベルテーブル
										// LEVEL1 TOP -> [0]:1-top   [1]:non   [2]:non
										// LEVEL2 TOP -> [0]:1-Oside [1]:2-top [2]:1-Dside
	UINT16 divInfoVol;					// エリア情報数
	SCRP_DIVAREA* divInfo;				// エリア情報先頭ポインタ
	UINT16 pclStateVol;					// パーセルリスト数
	SCRP_AREAPCLSTATE* pclState;		// パーセルリスト先頭ポインタ
} SCRP_LEVELTBL;

// ダイクストラヒープ
typedef struct _SCRP_DIJKSTRAHEAP {
	UINT32* heap;						// ヒープ
	UINT16 heapEnd;						// ヒープ末端
	UINT16 heapSize;					// ヒープテーブルサイズ
} SCRP_DIJKSTRAHEAP;

// 探索情報テーブル：ネットワークデータ
typedef struct _SCRP_NETDATA {
	UINT32 nextLink;					// 接続リンクへのネットワークインデックス
	UINT32 inLinkHist;					// 計算結果侵入リンクネットワークインデックス
	UINT16 flag;						// フラグ情報
	UINT32 costSum;						// コスト
	UINT32 heap;						// ヒープ 20140724 4Byte拡張
} SCRP_NETDATA;

// 探索情報テーブル：リンク
typedef struct _SCRP_LINKINFO {
	UINT16 detaIndex;					// データINDEX(0始まり)
#if 0 // TODO 追加予定
	UINT16 regOfs;						// 規制情報オフセット
#endif
	SCRP_NETDATA linkNet[2];			// リンク情報([0]順 [1]逆)
} SCRP_LINKINFO;

// 探索情報テーブル：パーセルデータ
typedef struct _SCRP_PCLINFO {
	UINT8* mapNetworkBin;				// 地図アドレス：道路
	UINT8* mapNetworkLinkBin;			// 地図アドレス：道路->リンク先頭
	UINT8* mapNetworkCnctBin;			// 地図アドレス：道路->接続先頭
	UINT8* mapNetworkLinkExBin;			// 地図アドレス：道路->リンク拡張先頭
	UINT16 index;						// 自身Index
	UINT32 parcelId;					// パーセルID
	UINT32 linkIdx;						// リンク情報テーブルインデックス
	UINT16 linkVol;						// リンク数
	UINT16 linkIdVol;					// リンクID数
	UINT8 areaSplit;					// 断裂情報
	UINT8 areaJoin;						// 分割情報
	UINT8 flag;							// フラグ情報
} SCRP_PCLINFO;

/* 20151215 masu
 * 旧エリア情報生成処理を使用する場合の為の構造体定義です。
 */
#define SCRP_OLDAREAMAKE_
#ifdef SCRP_OLDAREAMAKE_
typedef struct {						// --------------------------*/
	SCRP_PCLRECT pclRect;				// 矩形情報
	UINT32 parcelVol;					// パーセル情報数
	SCRP_AREAPCLSTATE * parcel_p;		// パーセル情報先頭ポインタ
} T_DivAreaInfo;
typedef struct {						// エリア情報 ---------------*/
	SCRP_PCLRECT pclRect;				// 矩形情報
	UINT32 divAreaVol;					// エリア情報数
	T_DivAreaInfo * divArea_p;			// エリア情報先頭ポインタ
	UINT32 parcelVol;					// パーセルリスト総数
	SCRP_AREAPCLSTATE* parcelTop_p;		// パーセルリスト先頭ポインタ
} T_AreaInfo;
typedef struct _SCRP_LEVELTBL_OLD {		// --------------------------*/
	UINT8 topLevel;						// トップレベル
	T_AreaInfo topArea;					// トップエリア
} SCRP_LEVELTBL_OLD;
#endif /* SCRP_OLDAREAMAKE_ */

#endif /* RP_NETWORK_H_ */

