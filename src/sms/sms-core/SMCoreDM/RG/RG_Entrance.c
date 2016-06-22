/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

/*-----------------------------------------------------------------------------------------------*/
/* File：RG_Entrance.c                                                                           */
/* Info：メッセージ受け口                                                                        */
/*-----------------------------------------------------------------------------------------------*/

#include "sms-core/SMCoreDM/SMCoreDMInternal.h"

/*-----------------------------------------------------------------------------------------------*/
/* 宣言                                                                                          */
/*-----------------------------------------------------------------------------------------------*/
static void RG_FMMsg_Send(E_SC_MSG_ID, E_SC_RESULT);

/**
 * @brief	送信元別メッセージ振り分け処理
 * @param	[I]受信メッセージ
 */
void SC_RG_MsgAnalyze(pthread_msq_msg_t* aMsg)
{
	E_SC_RESULT			ret  = e_SC_RESULT_SUCCESS;
	//E_SC_POSINFO_KIND	type = e_SC_POSINFO_KIND_DST;
	static INT32		simstart_f = 0;

	SC_LOG_DebugPrint(SC_TAG_RG, SC_LOG_START);

	// FMからの送信メッセージ
	if (SC_CORE_MSQID_FM == pthread_msq_msg_issender(aMsg)) {
		switch (aMsg->data[SC_MSG_MSG_ID]) {
		case e_SC_MSGID_REQ_RG_GUIDESTART:		// 経路誘導開始
			SC_LOG_InfoPrint(SC_TAG_RG, "[Msg] e_SC_MSGID_REQ_RG_GUIDESTART => GUIDE START !!");

			// 誘導制御テーブル初期化
			RG_CTL_InitCtlTbl();

			// 各種誘導情報共有メモリ初期化
			RG_CTL_SH_InitShareData();

			// 経路誘導情報作成要求
			RG_RTMsg_Send(e_SC_MSGID_REQ_RT_GUIDEMAKE);
			break;

		case e_SC_MSGID_REQ_RG_GUIDERUN:		// 経路誘導実行
//			SC_LOG_InfoPrint(SC_TAG_RG, "[Msg] e_SC_MSGID_REQ_RG_GUIDERUN  =>  GUIDE RUN !!");

			// 誘導制御メイン
			ret = RG_CTL_GuideMain();
			if (e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_DebugPrint(SC_TAG_RG, "[call] GuideTblTrack => FAIL !!"HERE);
			}

			// 経路誘導実行応答
			RG_FMMsg_Send(e_SC_MSGID_RES_RG_GUIDERUN, e_SC_RESULT_SUCCESS);
			break;

		case e_SC_MSGID_REQ_RG_GUIDESTOP:		// 経路誘導終了
			SC_LOG_InfoPrint(SC_TAG_RG, "[Msg] e_SC_MSGID_REQ_RG_GUIDESTOP =>  GUIDE STOP !!");

			// 経路誘導情報解放要求
			RG_RTMsg_Send(e_SC_MSGID_REQ_RT_GUIDEFREE);
			break;

		case e_SC_MSGID_REQ_RG_SIMSTART:		// シュミレーション開始
			SC_LOG_InfoPrint(SC_TAG_RG, "[Msg] e_SC_MSGID_REQ_RG_SIMSTART => SIM START !!");

			// ここではフラグを立てるだけ。誘導情報作成要求はしない。
			simstart_f = 1;

			// 経路誘導開始応答
			RG_FMMsg_Send(e_SC_MSGID_RES_RG_SIMSTART, e_SC_RESULT_SUCCESS);
			break;

		case e_SC_MSGID_REQ_RG_SIMRUN:			// シュミレーション実行
//			SC_LOG_InfoPrint(SC_TAG_RG, "[Msg] e_SC_MSGID_REQ_RG_SIMRUN  =>  SIM RUN !!");

			if (2 == simstart_f) {
				// シュミレーション実行
				ret = RG_CTL_RunSimulation();
				if (e_SC_RESULT_SUCCESS != ret) {
					SC_LOG_DebugPrint(SC_TAG_RG, "[call] RG_CTL_RunSimulation => FAIL !!"HERE);
				}
			}

			 // 経路誘導実行応答
			RG_FMMsg_Send(e_SC_MSGID_RES_RG_SIMRUN, e_SC_RESULT_SUCCESS);
			break;

		case e_SC_MSGID_REQ_RG_SIMEXIT:			// シュミレーション終了
			SC_LOG_InfoPrint(SC_TAG_RG, "[Msg] e_SC_MSGID_REQ_RG_SIMEXIT =>  SIM EXIT !!");

			ret = RG_CTL_ExitSimulation();
			if (e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_DebugPrint(SC_TAG_RG, "[call] RG_CTL_SetShareData => FAIL !!"HERE);
			}

			simstart_f = 0;

			// 経路誘導終了応答
			RG_FMMsg_Send(e_SC_MSGID_RES_RG_SIMEXIT, e_SC_RESULT_SUCCESS);
			break;

		case e_SC_MSGID_REQ_RG_TURNLIST:		// ターンリスト作成
			SC_LOG_InfoPrint(SC_TAG_RG, "[Msg] e_SC_MSGID_REQ_RG_TURNLIST => TURNLIST MAKE !!");

			// ターンリスト作成要求
			RG_RTMsg_Send(e_SC_MSGID_REQ_RT_LISTMAKE);
			break;

		default:
			SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] UNKNOWN MESSAGE. " HERE);
			break;
		}

	// RTからの送信メッセージ
	} else if (SC_CORE_MSQID_RT == pthread_msq_msg_issender(aMsg)) {
		switch (aMsg->data[SC_MSG_MSG_ID]) {
		case e_SC_MSGID_RES_RT_GUIDEMAKE:		// 経路誘導情報作成応答
			SC_LOG_InfoPrint(SC_TAG_RG, "[Msg] e_SC_MSGID_RES_RT_GUIDEMAKE  =>  GUIDE MAKE !!");

			if (1 == simstart_f) {
				// シュミレーション準備
				ret = RG_CTL_SimReady();
				if (e_SC_RESULT_SUCCESS != ret) {
					SC_LOG_DebugPrint(SC_TAG_RG, "[call] RG_CTL_SimReady => FAIL !!"HERE);
				}
				simstart_f = 2;
			}

			// 経路誘導開始応答
			RG_FMMsg_Send(e_SC_MSGID_RES_RG_GUIDESTART, e_SC_RESULT_SUCCESS);
			break;

		case e_SC_MSGID_RES_RT_GUIDEFREE:		// 経路誘導情報解放応答
			SC_LOG_InfoPrint(SC_TAG_RG, "[Msg] e_SC_MSGID_RES_RT_GUIDEFREE  =>  GUIDE FREE !!");

			// 経路誘導終了応答
			RG_FMMsg_Send(e_SC_MSGID_RES_RG_GUIDESTOP, e_SC_RESULT_SUCCESS);
			break;

		case e_SC_MSGID_RES_RT_LISTMAKE:		// ターンリスト作成応答
			SC_LOG_InfoPrint(SC_TAG_RG, "[Msg] e_SC_MSGID_RES_RT_LISTMAKE  =>  LIST MAKE !!");

			// ターンリスト作成要求応答
			RG_FMMsg_Send(e_SC_MSGID_RES_RG_TURNLIST, e_SC_RESULT_SUCCESS);
			break;

		default:
			SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] UNKNOWN MESSAGE. " HERE);
			break;
		}

	} else {
		// Unknown Sender
		SC_LOG_ErrorPrint(SC_TAG_RG, "[CTL] UNKNOWN MESSAG. " HERE);
	}

	SC_LOG_DebugPrint(SC_TAG_RG, SC_LOG_END);
}

/**
 * @brief	FMへの応答メッセージ送信
 * @param	[I]送信メッセージ
 * @memo	RG->FM
 */
static void RG_FMMsg_Send(E_SC_MSG_ID msg_type, E_SC_RESULT result)
{
	pthread_msq_msg_t sendMsg = {};
	pthread_msq_id_t* queue;

	// メッセージ設定
	sendMsg.data[SC_MSG_MSG_ID] = msg_type;
	sendMsg.data[SC_MSG_RES_RG_RESULT] = result;

	// キュー設定
	queue = ((pthread_msq_id_t*) SC_CORE_MSQID_FM);

	// メッセージ送信
	pthread_msq_msg_send(queue, &sendMsg, SC_CORE_MSQID_RG);
}

/**
 * @brief	RTへの要求メッセージ送信
 * @param	[I]送信メッセージ
 * @memo	RG->RT
 */
void RG_RTMsg_Send(E_SC_MSG_ID msg_type)
{
	pthread_msq_msg_t sendMsg = {};
	pthread_msq_id_t* queue;

	// メッセージ設定
	sendMsg.data[SC_MSG_MSG_ID] = msg_type;

	// キュー設定
	queue = ((pthread_msq_id_t*) SC_CORE_MSQID_RT);

	// メッセージ送信
	pthread_msq_msg_send(queue, &sendMsg, SC_CORE_MSQID_RG);
}

