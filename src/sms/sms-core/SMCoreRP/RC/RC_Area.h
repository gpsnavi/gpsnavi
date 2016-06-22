/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef RC_AREA_H_
#define RC_AREA_H_

#define RC_DIVAREA_QUEUE_SIZE			(256)			/* 分割エリア生成用スタックキューサイズ */
														/* 250km*250kmのエリアをLv1で処理、1万枚/{RC_DIVLINKCNT_MAX(40万本)/都内最大密度(26*256)} = 166分割なので256とする*/
#define RC_DIVAREA_QUEUE_FRONT_INIT		(0)
#define RC_DIVAREA_QUEUE_REAR_INIT		(0)
#define RC_DIVAREA_QUEUE_COUNT_INIT		(0)
#define RC_DIVAREA_STACK_MAX			(50)			/* 分割エリア生成用スタック最大 */
														/* 一辺2^n正方形のエリアを半分ずつ4*4まで切ったときは瞬間最大値2*(n-2)+1*/
														/* 250km*250kmのエリアを一辺100枚、2^7に見立てると11、余裕を持たせて50*/
#define RC_DIVAREA_STACK_IDX_INIT		(-1)			/* スタックインデックス初期値 */
#define RC_DIVPCLRANGECNT_MIN			(3)				/* 分割探索エリアの一辺の最少パーセル数 */
#define RC_DIVPCLRANGECNT_MAX			(11)			/* 分割探索エリアの一辺の最大パーセル数 */
#define RC_DIVPARCELCNT_MAX				(121)			/* 分割探索エリアの最大パーセル数 */
#define RC_DIVLINKCNT_MAX				(400000)		/* 分割探索エリアの最大リンク本数 ※レベルで処理を分けていません */

/*
 * 列挙型定義
 */
typedef enum _E_RC_AREATYPE {
	e_AREA_TYPE_OD = 0,					// OD双方を含む
	e_AREA_TYPE_O,						// Oのみ含む
	e_AREA_TYPE_D,						// Dのみ含む
	e_AREA_TYPE_OTHER,					// OもDも含まないエリア
	e_AREA_TYPE_DEFAULT					// 初期化用
} E_RC_AREATYPE;
typedef enum _E_RC_DIVAREA_CORNERNBRFLG {
	e_RC_CORNER_NEIGHBORFLG_LEFT = -1,		//左側に近い
	e_RC_CORNER_NEIGHBORFLG_DEFAULT = 0,	//デフォルト値
	e_RC_CORNER_NEIGHBORFLG_RIGHT = 1,		//右側に近い
	e_RC_CORNER_NEIGHBORFLG_BOTTOM = -1,	//下側に近い
	e_RC_CORNER_NEIGHBORFLG_TOP = 1,		//上側に近い
} E_RC_DIVAREA_CORNERNBRFLG;

typedef enum _E_RC_DIVAREA_LASTDIVSIDEFLG_X {
	e_RC_LASTDIVSIDEFLG_LEFT = -1,			//最後の分割方向が左
	e_RC_LASTDIVSIDEFLG_XDEFAULT = 0,		//デフォルト値
	e_RC_LASTDIVSIDEFLG_RIGHT = 1,			//最後の分割方向が右
} E_RC_DIVAREA_LASTDIVSIDEFLG_X;
typedef enum _E_RC_DIVAREA_LASTDIVSIDEFLG_Y {
	e_RC_LASTDIVSIDEFLG_BOTTOM = -1,		//最後の分割方向が下
	e_RC_LASTDIVSIDEFLG_YDEFAULT = 0,		//デフォルト値
	e_RC_LASTDIVSIDEFLG_TOP = 1,			//最後の分割方向が上
} E_RC_DIVAREA_LASTDIVSIDEFLG_Y;


//分割エリアスタック管理
typedef struct _RC_DIVAREA_STACK {
	SCRP_PCLRECT pclRect;
	E_RC_AREATYPE areaType;
	INT16 joinLeft;						// 接続有無フラグ
	INT16 joinRight;
	INT16 joinTop;
	INT16 joinBottom;
	INT16 primaryX;						// 格納優先度（左右）
	INT16 primaryY;						// 格納優先度（上下）
	E_RC_DIVAREA_LASTDIVSIDEFLG_X lastDivSideX;	//ラスト分割方向X
	E_RC_DIVAREA_LASTDIVSIDEFLG_Y lastDivSideY;	//ラスト分割方向Y
} RC_DIVAREA_STACK;

//分割エリア管理
typedef struct _RC_DIVAREA_QUEUEBUFF {
	INT16 procNo;						// 処理順番
	SCRP_PCLRECT pclRect;
	INT32 altX;							// 非分割エリア内相対位置X
	INT32 altY;							// 非分割エリア内相対位置Y
	INT16 joinX;						// 接続有無フラグ
	INT16 joinY;
	INT16 splitX;						// 切断有無フラグ
	INT16 splitY;
} RC_DIVAREA_QUEUEBUFF;

//分割エリア管理
typedef struct _RC_DIVAREA_QUEUE {
	INT16 front;						// 先頭位置
	INT16 rear;							// 現在位置
	INT16 count;						// 合計
	RC_DIVAREA_QUEUEBUFF *divQueuebuf;
} RC_DIVAREA_QUEUE;

// スタック格納用 処理対象エリア情報
typedef struct _RC_AREASTATEINFO {
	E_RC_AREATYPE areaType;				// エリア種別
	SCRP_PCLRECT pclRect;				// 矩形情報
	INT16 joinLeft;						// 接続情報_左
	INT16 joinRight;					// 接続情報_右
	INT16 joinTop;						// 接続情報_上
	INT16 joinBottom;					// 接続情報_下
	INT16 primaryX;						// 格納優先度_左右
	INT16 primaryY;						// 格納優先度_上下
	E_RC_DIVAREA_LASTDIVSIDEFLG_X lastDivSideX;	//ラスト分割方向X
	E_RC_DIVAREA_LASTDIVSIDEFLG_Y lastDivSideY;	//ラスト分割方向Y
} RC_AREASTATEINFO;

typedef struct _RC_BASEAREATOPLVINFO {
	SCRP_LEVELAREA oBaseAreaTopLv;		//topLv_O基準エリア
	SCRP_LEVELAREA dBaseAreaTopLv;		//topLv_D基準エリア
	SCRP_LEVELAREA areaTopLv;
	T_AreaInfo oBaseArea;				//Lv1_O基準エリア
	T_AreaInfo dBaseArea;				//Lv1_D基準エリア
	UINT32 startParcelIdTopLv;			//topLv_OパーセルID
	UINT32 goalParcelIdTopLv;			//topLv_DパーセルID
	UINT32 startParcelDensityTopLv;		//topLv_O密度
	UINT32 goalParcelDensityTopLv;		//topLv_D密度
	UINT16 totalPclVolTopLv;			//topレベル合計枚数
} RC_BASEAREATOPLVINFO;

#if 1 /* 旧AreaDiv用 */
// 分割探索エリア管理情報
typedef struct {
	INT32 alt_x_for_o_pos;				// 出発地点からの相対位置（ブロック方向）
	INT32 alt_y_for_o_pos;				// 出発地点からの相対位置（レイヤー方向）
	INT32 baseParcelId;					// 分割エリア左下絶対パーセルＩＤ
	INT32 alt_x;						// 非分割エリア内の相対位置（Ｘ方向）
	INT32 alt_y;						// 非分割エリア内の相対位置（Ｙ方向）
	INT32 x_size;						// 分割エリア内パーセル枚数（Ｘ方向）
	INT32 y_size;						// 分割エリア内パーセル枚数（Ｙ方向）
	SCRP_AREAPCLSTATE* parcel_p;		// パーセル情報先頭ポインタ
	INT32 proc_f;						// 処理済みフラグ ０：未処理 １：処理中 ２：処理済み
	INT32 proc_no;						// 処理順番（０～）
	INT32 link_cnt;						// リンク本数
} T_RC_DivAreaInf;
#endif

#endif /* RC_AREA_H_ */
