/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCOREMSQ_H_
#define SMCOREMSQ_H_

//-----------------------------------
// 共用体定義
//-----------------------------------
// メッセージキューID定義
typedef enum _E_SC_MSQ_ID {
	e_SC_MSQID_FM = 0,					// Func.Mng.メッセージキューID
	e_SC_MSQID_MP,						// 地図描画メッセージキューID
	e_SC_MSQID_RM,						// 経路探索(RM)メッセージキューID
	e_SC_MSQID_RC,						// 経路探索(RC)メッセージキューID
	e_SC_MSQID_RG,						// 経路誘導(RG)メッセージキューID
	e_SC_MSQID_RT,						// 経路誘導(RT)メッセージキューID
	e_SC_MSQID_DH,						// Data HandlerメッセージキューID
	e_SC_MSQID_DC,						// Data HandlerキャンセルメッセージキューID
	e_SC_MSQID_PM,						// プローブアップロード(メイン)メッセージキューID
	e_SC_MSQID_PU,						// プローブアップロード(アップロード)メッセージキューID
	e_SC_MSQID_PT,						// プローブアップロード(タイマ)メッセージキューID
	e_SC_MSQID_SDD,						// 運転特性診断(データ取得)メッセージキューID
	e_SC_MSQID_SDM,						// 運転特性診断(メイン)メッセージキューID
	e_SC_MSQID_SDU,						// 運転特性診断(アップロード)メッセージキューID
	e_SC_MSQID_SDT,						// 運転特性診断(タイマ)メッセージキューID
	e_SC_MSQID_TR,						// 交通情報(TR)メッセージキューID
	e_SC_MSQID_TRT,						// 交通情報(タイマ)メッセージキューID

	e_SC_MSQID_END
} E_SC_MSQ_ID;

//-----------------------------------
// マクロ定義
//-----------------------------------
#define	SC_QUEUE_NUM		e_SC_MSQID_END					// メッセージキュー数

#define	SC_CORE_MSQID(idx)	(&msgQueue[idx].msgQueue)			// メッセージキューID取得
#define	SC_CORE_MSQID_FM	(&msgQueue[e_SC_MSQID_FM].msgQueue)	// Func.Mng.メッセージキューID
#define SC_CORE_MSQID_MP	(&msgQueue[e_SC_MSQID_MP].msgQueue)	// 地図描画メッセージキューID
#define SC_CORE_MSQID_RM	(&msgQueue[e_SC_MSQID_RM].msgQueue)	// 経路探索(RM)メッセージキューID
#define SC_CORE_MSQID_RC	(&msgQueue[e_SC_MSQID_RC].msgQueue)	// 経路探索(RC)メッセージキューID
#define SC_CORE_MSQID_RG	(&msgQueue[e_SC_MSQID_RG].msgQueue)	// 経路誘導(RG)メッセージキューID
#define SC_CORE_MSQID_RT	(&msgQueue[e_SC_MSQID_RT].msgQueue)	// 経路誘導(RG)メッセージキューID
#define SC_CORE_MSQID_DH	(&msgQueue[e_SC_MSQID_DH].msgQueue)	// Data HandlerメッセージキューID
#define SC_CORE_MSQID_DC	(&msgQueue[e_SC_MSQID_DC].msgQueue)	// Data HandlerキャンセルメッセージキューID
#define SC_CORE_MSQID_PM	(&msgQueue[e_SC_MSQID_PM].msgQueue)	// プローブアップロード メッセージキューID
#define SC_CORE_MSQID_PU	(&msgQueue[e_SC_MSQID_PU].msgQueue)	// プローブアップロード メッセージキューID
#define SC_CORE_MSQID_PT	(&msgQueue[e_SC_MSQID_PT].msgQueue)	// タイマ(プローブアップロード用) メッセージキューID
#define SC_CORE_MSQID_SDD	(&msgQueue[e_SC_MSQID_SDD].msgQueue)// 運転特性診断(データ取得)メッセージキューID
#define SC_CORE_MSQID_SDM	(&msgQueue[e_SC_MSQID_SDM].msgQueue)// 運転特性診断(メイン)メッセージキューID
#define SC_CORE_MSQID_SDU	(&msgQueue[e_SC_MSQID_SDU].msgQueue)// 運転特性診断(アップロード)メッセージキューID
#define SC_CORE_MSQID_SDT	(&msgQueue[e_SC_MSQID_SDT].msgQueue)// 運転特性診断(タイマ)メッセージキューID
#define SC_CORE_MSQID_TR	(&msgQueue[e_SC_MSQID_TR].msgQueue)	// 交通情報(TR)メッセージキューID
#define SC_CORE_MSQID_TRT	(&msgQueue[e_SC_MSQID_TRT].msgQueue)// 交通情報(タイマ)メッセージキューID


//-----------------------------------
// 構造体定義
//-----------------------------------
typedef struct _SC_CORE_MSQ {
	INT32				msgQueueSize;
	pthread_msq_id_t	msgQueue;
} SC_CORE_MSQ;

//-----------------------------------
// 外部変数定義
//-----------------------------------
// メッセージキュー
extern SC_CORE_MSQ msgQueue[SC_QUEUE_NUM];

#endif // #ifndef SMCORE_MSQ_H_
