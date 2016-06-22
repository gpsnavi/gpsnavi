/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#include "SMCoreCMNInternal.h"

//-----------------------------------
// 変数定義
//-----------------------------------
// メッセージキュー
#ifdef __SMS_APPLE__
SC_CORE_MSQ msgQueue[SC_QUEUE_NUM] = {
		{ 50, PTHREAD_MSQ_ID_INITIALIZER("/SC_FM_Send","/SC_FM_Receive") },	// Func.Mng.メッセージキュー
		{ 50, PTHREAD_MSQ_ID_INITIALIZER("/SC_MP_Send","/SC_MP_Receive") },	// 地図描画メッセージキュー
		{ 50, PTHREAD_MSQ_ID_INITIALIZER("/SC_RM_Send","/SC_RM_Receive") },	// 経路探索(RM)メッセージキュー
		{ 50, PTHREAD_MSQ_ID_INITIALIZER("/SC_RC_Send","/SC_RC_Receive") },	// 経路探索(RC)メッセージキュー
		{ 50, PTHREAD_MSQ_ID_INITIALIZER("/SC_RG_Send","/SC_RG_Receive") },	// 経路誘導(RG)メッセージキュー
		{ 50, PTHREAD_MSQ_ID_INITIALIZER("/SC_RT_Send","/SC_RT_Receive") },	// 経路誘導(RT)メッセージキュー
		{ 50, PTHREAD_MSQ_ID_INITIALIZER("/SC_DH_Send","/SC_DH_Receive") },	// Data Handlerメッセージキュー
		{ 50, PTHREAD_MSQ_ID_INITIALIZER("/SC_DHC_Send","/SC_DHC_Receive") },	// Data HandlerキャンセルメッセージキューID
		{ 50, PTHREAD_MSQ_ID_INITIALIZER("/SC_PM_Send","/SC_PM_Receive") },	// プローブアップロード メッセージキューID
		{ 50, PTHREAD_MSQ_ID_INITIALIZER("/SC_PU_Send","/SC_PU_Receive") },	// プローブアップロード メッセージキューID
		{ 50, PTHREAD_MSQ_ID_INITIALIZER("/SC_PT_Send","/SC_PT_Receive") },	// タイマ(プローブアップロード用) メッセージキューID
		{ 50, PTHREAD_MSQ_ID_INITIALIZER("/SC_SDD_Send","/SC_SDD_Receive") },	// 運転特性診断(データ取得)メッセージキューID
		{ 50, PTHREAD_MSQ_ID_INITIALIZER("/SC_SDM_Send","/SC_SDM_Receive") },	// 運転特性診断(メイン)メッセージキューID
		{ 50, PTHREAD_MSQ_ID_INITIALIZER("/SC_SDU_Send","/SC_SDU_Receive") },	// 運転特性診断(アップロード)メッセージキューID
		{ 50, PTHREAD_MSQ_ID_INITIALIZER("/SC_SDT_Send","/SC_SDT_Receive") },	// 運転特性診断(タイマ)メッセージキューID
		{ 50, PTHREAD_MSQ_ID_INITIALIZER("/SC_TR_Send","/SC_TR_Receive") },		// 交通情報(TR)メッセージキューID
		{ 50, PTHREAD_MSQ_ID_INITIALIZER("/SC_TRT_Send","/SC_TRT_Receive") },	// 交通情報(TRT)メッセージキューID
};
#else
SC_CORE_MSQ msgQueue[SC_QUEUE_NUM] = {
	{50,	PTHREAD_MSQ_ID_INITIALIZER},	// Func.Mng.メッセージキュー
	{50,	PTHREAD_MSQ_ID_INITIALIZER},	// 地図描画メッセージキュー
	{50,	PTHREAD_MSQ_ID_INITIALIZER},	// 経路探索(RM)メッセージキュー
	{50,	PTHREAD_MSQ_ID_INITIALIZER},	// 経路探索(RC)メッセージキュー
	{50,	PTHREAD_MSQ_ID_INITIALIZER},	// 経路誘導(RG)メッセージキュー
	{50,	PTHREAD_MSQ_ID_INITIALIZER},	// 経路誘導(RT)メッセージキュー
	{50,	PTHREAD_MSQ_ID_INITIALIZER},	// Data Handlerメッセージキュー
	{50,	PTHREAD_MSQ_ID_INITIALIZER},	// Data HandlerキャンセルメッセージキューID
	{50,	PTHREAD_MSQ_ID_INITIALIZER},	// プローブアップロード メッセージキューID
	{50,	PTHREAD_MSQ_ID_INITIALIZER},	// プローブアップロード メッセージキューID
	{50,	PTHREAD_MSQ_ID_INITIALIZER},	// タイマ(プローブアップロード用) メッセージキューID
	{50,	PTHREAD_MSQ_ID_INITIALIZER},	// 運転特性診断(データ取得)メッセージキューID
	{50,	PTHREAD_MSQ_ID_INITIALIZER},	// 運転特性診断(メイン)メッセージキューID
	{50,	PTHREAD_MSQ_ID_INITIALIZER},	// 運転特性診断(アップロード)メッセージキューID
	{50,	PTHREAD_MSQ_ID_INITIALIZER},	// 運転特性診断(タイマ)メッセージキューID
	{50,	PTHREAD_MSQ_ID_INITIALIZER},	// 交通情報(TR)メッセージキューID
	{50,	PTHREAD_MSQ_ID_INITIALIZER},	// 交通情報(TRT)メッセージキューID
};
#endif /* __SMS_APPLE__ */
