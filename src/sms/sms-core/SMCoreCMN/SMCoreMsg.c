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
 * SMCoreMsg.c
 *
 *  Created on: 2015/11/05
 *      Author:
 */

#include "SMCoreCMNInternal.h"

// TODO 関数ポインタ追加


//-----------------------------------
// テーブル定義
//-----------------------------------
#define	SC_MSG_DATAINF_TBL_NUM			85		// メッセージテーブル数 TODO
// メッセージテーブル
// 【!! 注意 !!】
// ↓↓↓　データを追加・削除したらメッセージテーブル数も変更すること
const static SC_MSG_DATAINFO msgDataInfTbl[SC_MSG_DATAINF_TBL_NUM] = {
	// 【メッセージID】						【送信先メッセージキュー】	【メッセージ受信後の処理関数ポインタ】
	/***** 要求 *****/
	{e_SC_MSGID_REQ_MAP_DATA,				SC_CORE_MSQID_DH,			(SC_MsgFunc)SC_DH_MsgDispatch},		// 地図データ取得(DH)
	{e_SC_MSGID_REQ_MAP_CASH_FREE,			SC_CORE_MSQID_DH,			(SC_MsgFunc)SC_DH_MsgDispatch},		// 地図データキャッシュ解放(DH)
	{e_SC_MSGID_REQ_REFRESH_MAP,			SC_CORE_MSQID_MP,			(SC_MsgFunc)NULL},					// 地図描画(MP)
	{e_SC_MSGID_REQ_RM_INIT,				SC_CORE_MSQID_RM,			(SC_MsgFunc)SC_RM_MsgAnalyze},		// 探索初期化(RM)
	{e_SC_MSGID_REQ_RC_INIT,				SC_CORE_MSQID_RC,			(SC_MsgFunc)SC_RC_MsgAnalyze},		// 探索初期化(RC)
	{e_SC_MSGID_REQ_RM_RTSINGLE,			SC_CORE_MSQID_RM,			(SC_MsgFunc)SC_RM_MsgAnalyze},		// 単経路探索(RM)
	{e_SC_MSGID_REQ_RM_RTMULTI,				SC_CORE_MSQID_RM,			(SC_MsgFunc)SC_RM_MsgAnalyze},		// 複数経路探索(RM)
	{e_SC_MSGID_REQ_RM_REROUTE,				SC_CORE_MSQID_RM,			(SC_MsgFunc)SC_RM_MsgAnalyze},		// 再探索(RM)
	{e_SC_MSGID_REQ_RM_CANCEL,				SC_CORE_MSQID_RM,			(SC_MsgFunc)SC_RM_MsgAnalyze},		// 探索キャンセル(RM)
	{e_SC_MSGID_REQ_RC_RTSINGLE,			SC_CORE_MSQID_RC,			(SC_MsgFunc)SC_RC_MsgAnalyze},		// 単経路探索(RC)
	{e_SC_MSGID_REQ_RC_RTMULTI,				SC_CORE_MSQID_RC,			(SC_MsgFunc)SC_RC_MsgAnalyze},		// 複数経路探索(RC)
	{e_SC_MSGID_REQ_RC_REROUTE,				SC_CORE_MSQID_RC,			(SC_MsgFunc)SC_RC_MsgAnalyze},		// 再探索(RC)
	{e_SC_MSGID_REQ_VIEW_ROUTE,				SC_CORE_MSQID_MP,			(SC_MsgFunc)NULL},					// 経路描画(MP)
	{e_SC_MSGID_REQ_RG_GUIDESTART,			SC_CORE_MSQID_RG,			(SC_MsgFunc)SC_RG_MsgAnalyze},		// 経路誘導開始(RG)
	{e_SC_MSGID_REQ_RG_GUIDERUN,			SC_CORE_MSQID_RG,			(SC_MsgFunc)SC_RG_MsgAnalyze},		// 経路誘導実行(RG)
	{e_SC_MSGID_REQ_RG_GUIDESTOP,			SC_CORE_MSQID_RG,			(SC_MsgFunc)SC_RG_MsgAnalyze},		// 経路誘導終了(RG)
	{e_SC_MSGID_REQ_RG_SIMSTART,			SC_CORE_MSQID_RG,			(SC_MsgFunc)SC_RG_MsgAnalyze},		// シュミレーション開始(RG)
	{e_SC_MSGID_REQ_RG_SIMRUN,				SC_CORE_MSQID_RG,			(SC_MsgFunc)SC_RG_MsgAnalyze},		// シュミレーション実行(RG)
	{e_SC_MSGID_REQ_RG_SIMEXIT,				SC_CORE_MSQID_RG,			(SC_MsgFunc)SC_RG_MsgAnalyze},		// シュミレーション終了(RG)
	{e_SC_MSGID_REQ_PU_UPLOAD,				SC_CORE_MSQID_PU,			(SC_MsgFunc)SC_PU_MsgAnalyze},		// プローブアップロード(PU)
	{e_SC_MSGID_REQ_PT_START,				SC_CORE_MSQID_PT,			(SC_MsgFunc)SC_PT_MsgAnalyze},		// タイマ開始(PT)
	{e_SC_MSGID_REQ_RG_TURNLIST,			SC_CORE_MSQID_RG,			(SC_MsgFunc)SC_RG_MsgAnalyze},		// ターンリスト作成要求(RG)
	{e_SC_MSGID_REQ_RT_GUIDEMAKE,			SC_CORE_MSQID_RT,			(SC_MsgFunc)SC_RT_MsgAnalyze},		// 経路誘導情報作成要求(RT)
	{e_SC_MSGID_REQ_RT_GUIDEFREE,			SC_CORE_MSQID_RT,			(SC_MsgFunc)SC_RT_MsgAnalyze},		// 経路誘導情報解放要求(RT)
	{e_SC_MSGID_REQ_RT_LISTMAKE,			SC_CORE_MSQID_RT,			(SC_MsgFunc)SC_RT_MsgAnalyze},		// ターンリスト作成要求(RT)
	{e_SC_MSGID_REQ_RT_GUIDEADD,			SC_CORE_MSQID_RT,			(SC_MsgFunc)SC_RT_MsgAnalyze},		// 経路誘導情報追加要求(RT)
	{e_SC_MSGID_REQ_SDM_START,				SC_CORE_MSQID_SDM,			(SC_MsgFunc)SC_SDM_MsgAnalyze},		// 運転特性診断開始要求(FM->SDM)
	{e_SC_MSGID_REQ_SDD_START,				SC_CORE_MSQID_SDD,			(SC_MsgFunc)SC_SDD_MsgAnalyze},		// 運転特性診断開始要求(SDM->SDD)
	{e_SC_MSGID_REQ_SDT_START,				SC_CORE_MSQID_SDT,			(SC_MsgFunc)SC_SDT_MsgAnalyze},		// タイマ開始要求(SDM->SDT)
	{e_SC_MSGID_REQ_SDU_UPLOAD,				SC_CORE_MSQID_SDU,			(SC_MsgFunc)SC_SDU_MsgAnalyze},		// アップロード要求(SDM->SDU)
	{e_SC_MSGID_REQ_SDM_STOP,				SC_CORE_MSQID_SDM,			(SC_MsgFunc)SC_SDM_MsgAnalyze},		// 運転特性診断停止要求(FM->SDM)
	{e_SC_MSGID_REQ_SDD_STOP,				SC_CORE_MSQID_SDD,			(SC_MsgFunc)SC_SDD_MsgAnalyze},		// 運転特性診断停止要求(SDM->SDD)
	{e_SC_MSGID_REQ_SDU_STOP,				SC_CORE_MSQID_SDU,			(SC_MsgFunc)SC_SDU_MsgAnalyze},		// アップロード終了要求(SDM->SDU)
	{e_SC_MSGID_REQ_TR_UPDATE,				SC_CORE_MSQID_TR,			(SC_MsgFunc)SC_TR_MsgAnalyze},		// 交通情報更新要求(TR)
	{e_SC_MSGID_REQ_TRT_START,				SC_CORE_MSQID_TRT,			(SC_MsgFunc)SC_TRT_MsgAnalyze},		// タイマ開始要求(TR->TRT)

	/***** 応答 *****/
	{e_SC_MSGID_RES_MAP_DATA,				SC_CORE_MSQID_MP,			(SC_MsgFunc)NULL},	// 地図データ取得(MP)
	{e_SC_MSGID_RES_MAP_DATA,				SC_CORE_MSQID_RC,			(SC_MsgFunc)SC_RC_MsgAnalyze},		// 地図データ取得(RC)
	{e_SC_MSGID_RES_FM_RTSINGLE,			SC_CORE_MSQID_FM,			(SC_MsgFunc)SC_FM_ResRoute},		// 単経路探索応答(FM)
	{e_SC_MSGID_RES_FM_RTMULTI,				SC_CORE_MSQID_FM,			(SC_MsgFunc)SC_FM_ResRoute},		// 複数経路探索応答(FM)
	{e_SC_MSGID_RES_FM_REROUTE,				SC_CORE_MSQID_FM,			(SC_MsgFunc)SC_FM_ResReRoute},		// 再探索応答(FM)
	{e_SC_MSGID_RES_RC_RTSINGLE,			SC_CORE_MSQID_RM,			(SC_MsgFunc)SC_RM_MsgAnalyze},		// 単経路探索応答(RM)
	{e_SC_MSGID_RES_RC_RTMULTI,				SC_CORE_MSQID_RM,			(SC_MsgFunc)SC_RM_MsgAnalyze},		// 複数経路探索応答(RM)
	{e_SC_MSGID_RES_RC_REROUTE,				SC_CORE_MSQID_RM,			(SC_MsgFunc)SC_RM_MsgAnalyze},		// 再探索応答(RM)
	{e_SC_MSGID_RES_RG_GUIDESTART,			SC_CORE_MSQID_FM,			(SC_MsgFunc)SC_FM_ResGuide},		// 経路誘導開始応答(FM)
	{e_SC_MSGID_RES_RG_GUIDERUN,			SC_CORE_MSQID_FM,			(SC_MsgFunc)SC_FM_ResGuide},		// 経路誘導実行応答(FM)
	{e_SC_MSGID_RES_RG_GUIDESTOP,			SC_CORE_MSQID_FM,			(SC_MsgFunc)SC_FM_ResGuide},		// 経路誘導終了応答(FM)
	{e_SC_MSGID_RES_RG_SIMSTART,			SC_CORE_MSQID_FM,			(SC_MsgFunc)SC_FM_ResGuide},		// シュミレーション開始応答(FM)
	{e_SC_MSGID_RES_RG_SIMRUN,				SC_CORE_MSQID_FM,			(SC_MsgFunc)SC_FM_ResGuide},		// シュミレーション実行応答(FM)
	{e_SC_MSGID_RES_RG_SIMEXIT,				SC_CORE_MSQID_FM,			(SC_MsgFunc)SC_FM_ResGuide},		// シュミレーション終了応答(FM)
	{e_SC_MSGID_RES_PU_UPLOAD_FINISH,		SC_CORE_MSQID_PM,			(SC_MsgFunc)SC_PM_MsgAnalyze},		// アップロード終了応答(PU)
	{e_SC_MSGID_RES_RG_TURNLIST,			SC_CORE_MSQID_FM,			(SC_MsgFunc)SC_FM_ResGuide},		// ターンリスト作成応答(FM)
	{e_SC_MSGID_RES_RT_GUIDEMAKE,			SC_CORE_MSQID_RG,			(SC_MsgFunc)SC_RG_MsgAnalyze},		// 経路誘導情報作成要求(RG)
	{e_SC_MSGID_RES_RT_GUIDEFREE,			SC_CORE_MSQID_RG,			(SC_MsgFunc)SC_RG_MsgAnalyze},		// 経路誘導情報解放要求(RG)
	{e_SC_MSGID_RES_RT_LISTMAKE,			SC_CORE_MSQID_RG,			(SC_MsgFunc)SC_RG_MsgAnalyze},		// ターンリスト作成応答(RG)
	{e_SC_MSGID_RES_SDM_START,				SC_CORE_MSQID_FM,			(SC_MsgFunc)SC_FM_ResStartDrive},	// 運転特性診断開始応答(SDM->FM)
	{e_SC_MSGID_RES_SDD_START,				SC_CORE_MSQID_SDM,			(SC_MsgFunc)SC_SDM_MsgAnalyze},		// 運転特性診断開始応答(SDD->SDM)
	{e_SC_MSGID_RES_SDT_TIMERSTART,			SC_CORE_MSQID_SDM,			(SC_MsgFunc)SC_SDM_MsgAnalyze},		// タイマ開始応答(SDT->SDM)
	{e_SC_MSGID_RES_SDU_UPLOAD,				SC_CORE_MSQID_SDM,			(SC_MsgFunc)SC_SDM_MsgAnalyze},		// アップロード完了応答(SDU->SDM)
	{e_SC_MSGID_RES_SDM_STOP,				SC_CORE_MSQID_FM,			(SC_MsgFunc)SC_FM_ResStopDrive},	// 運転特性診断停止応答(SDM->FM)
	{e_SC_MSGID_RES_SDD_STOP,				SC_CORE_MSQID_SDM,			(SC_MsgFunc)SC_SDM_MsgAnalyze},		// 運転特性診断停止応答(SDD->SDM)
	{e_SC_MSGID_RES_SDU_STOP,				SC_CORE_MSQID_SDM,			(SC_MsgFunc)SC_SDM_MsgAnalyze},		// アップロード終了応答(SDU->SDM)

	/***** イベント／通知 *****/
	{e_SC_MSGID_EVT_CAR_STOP,				SC_CORE_MSQID_RM,			(SC_MsgFunc)SC_RM_MsgAnalyze},		// 自車位置停止(RM)
	{e_SC_MSGID_EVT_DEST_MOVE,				SC_CORE_MSQID_RM,			(SC_MsgFunc)SC_RM_MsgAnalyze},		// 目的地変更(RM)
	{e_SC_MSGID_EVT_ROUTESETTING_CHANGE,	SC_CORE_MSQID_RM,			(SC_MsgFunc)SC_RM_MsgAnalyze},		// 探索条件変更(RM)
	{e_SC_MSGID_EVT_ROUTE_DISTRIBUTE,		SC_CORE_MSQID_FM,			(SC_MsgFunc)SC_FM_ResRoute},		// 経路公開(FM)
	{e_SC_MSGID_EVT_TIMEOUT,				SC_CORE_MSQID_PM,			(SC_MsgFunc)SC_PM_MsgAnalyze},		// タイムアウト
	{e_SC_MSGID_EVT_TIMEOUT,				SC_CORE_MSQID_SDM,			(SC_MsgFunc)SC_SDM_MsgAnalyze},		// タイムアウト
	{e_SC_MSGID_EVT_SENSORDATA,				SC_CORE_MSQID_SDD,			(SC_MsgFunc)SC_SDD_MsgAnalyze},		// センサデータ通知
	{e_SC_MSGID_EVT_TIMEOUT,				SC_CORE_MSQID_TR,			(SC_MsgFunc)SC_TR_MsgAnalyze},		// タイムアウト
	{e_SC_MSGID_EVT_FINISH,					SC_CORE_MSQID_FM,			(SC_MsgFunc)NULL},					// 終了通知(FM)
	{e_SC_MSGID_EVT_FINISH,					SC_CORE_MSQID_MP,			(SC_MsgFunc)NULL},					// 終了通知(MP)
	{e_SC_MSGID_EVT_FINISH,					SC_CORE_MSQID_RM,			(SC_MsgFunc)NULL},					// 終了通知(RM)
	{e_SC_MSGID_EVT_FINISH,					SC_CORE_MSQID_RC,			(SC_MsgFunc)NULL},					// 終了通知(RC)
	{e_SC_MSGID_EVT_FINISH,					SC_CORE_MSQID_RG,			(SC_MsgFunc)NULL},					// 終了通知(RG)
	{e_SC_MSGID_EVT_FINISH,					SC_CORE_MSQID_RT,			(SC_MsgFunc)NULL},					// 終了通知(RT)
	{e_SC_MSGID_EVT_FINISH,					SC_CORE_MSQID_DH,			(SC_MsgFunc)NULL},					// 終了通知(DH)
	{e_SC_MSGID_EVT_FINISH,					SC_CORE_MSQID_PM,			(SC_MsgFunc)NULL},					// 終了通知(PM)
	{e_SC_MSGID_EVT_FINISH,					SC_CORE_MSQID_PU,			(SC_MsgFunc)NULL},					// 終了通知(PU)
	{e_SC_MSGID_EVT_FINISH,					SC_CORE_MSQID_PT,			(SC_MsgFunc)NULL},					// 終了通知(PT)
	{e_SC_MSGID_EVT_FINISH,					SC_CORE_MSQID_SDD,			(SC_MsgFunc)NULL},					// 終了通知(SDD)
	{e_SC_MSGID_EVT_FINISH,					SC_CORE_MSQID_SDM,			(SC_MsgFunc)NULL},					// 終了通知(SDM)
	{e_SC_MSGID_EVT_FINISH,					SC_CORE_MSQID_SDU,			(SC_MsgFunc)NULL},					// 終了通知(SDU)
	{e_SC_MSGID_EVT_FINISH,					SC_CORE_MSQID_SDT,			(SC_MsgFunc)NULL},					// 終了通知(SDT)
	{e_SC_MSGID_EVT_FINISH,					SC_CORE_MSQID_TR,			(SC_MsgFunc)NULL},					// 終了通知(TR)
	{e_SC_MSGID_EVT_FINISH,					SC_CORE_MSQID_TRT,			(SC_MsgFunc)NULL}					// 終了通知(TRT)
};

/**
 * @brief メッセージIDに応じたメッセージデータを取得する
 * @param[in] msg 受信メッセージ
 * @param[in] msq メッセージキュー(メッセージ受信キュー)
 */
void SC_MSG_MsgDispatch(pthread_msq_msg_t *msg, const pthread_msq_id_t *msq)
{
	const SC_MSG_DATAINFO	*msgData = NULL;
	INT32	num = 0;
	INT32	msgId = 0;

	// パラメータチェック
	if (NULL == msg) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[msgId], " HERE);
		return;
	}

	// メッセージ種別ID取得
	msgId = msg->data[SC_MSG_MSG_ID];

	for (num = 0; num < SC_MSG_DATAINF_TBL_NUM; num++) {
		if ((msgId == msgDataInfTbl[num].msgID) && (msq == msgDataInfTbl[num].msq)) {
			msgData = &msgDataInfTbl[num];
			break;
		}
	}
	if (NULL == msgData) {
		// 見つからなかった
		SC_LOG_ErrorPrint(SC_TAG_CORE, "not found msgID=%d, " HERE, msgId);
		return;
	}

	// メッセージを処理する関数を呼び出す
	if (NULL != msgData->Func) {
		msgData->Func(msg);
	}
}

