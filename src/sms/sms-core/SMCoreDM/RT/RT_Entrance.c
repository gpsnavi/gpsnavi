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
/* File：RT_Entrance.c                                                                           */
/* Info：メッセージ受け口                                                                        */
/*-----------------------------------------------------------------------------------------------*/

#include "sms-core/SMCoreDM/SMCoreDMInternal.h"

/*-----------------------------------------------------------------------------------------------*/
/* 宣言                                                                                          */
/*-----------------------------------------------------------------------------------------------*/
static void RT_RGMsg_Send(E_SC_MSG_ID, E_SC_RESULT);


/**
 * @brief	送信元別メッセージ振り分け処理
 * @param	[I]受信メッセージ
 */
void SC_RT_MsgAnalyze(pthread_msq_msg_t* aMsg)
{
	E_SC_RESULT			ret  = e_SC_RESULT_SUCCESS;
	//E_SC_POSINFO_KIND	type = e_SC_POSINFO_KIND_DST;

	SC_LOG_DebugPrint(SC_TAG_RT, SC_LOG_START);

	// FMからの送信メッセージ
	if (SC_CORE_MSQID_RG == pthread_msq_msg_issender(aMsg)) {
		switch (aMsg->data[SC_MSG_MSG_ID]) {
		case e_SC_MSGID_REQ_RT_GUIDEMAKE:		// 経路誘導情報作成
			SC_LOG_InfoPrint(SC_TAG_RT, "[Msg] e_SC_MSGID_REQ_RT_GUIDEMAKE => GUIDE MAKE !!");

			// 誘導テーブル作成
			ret = RT_TBL_MakeGuideTbl(MAKETBL_DIV);
			if (e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_DebugPrint(SC_TAG_RT, "[call] RT_MakeGuideTbl => FAIL !!"HERE);
				// 誘導テーブル解放
				ret = RT_TBL_FreeGuideTbl();
				if (e_SC_RESULT_SUCCESS != ret) {
					SC_LOG_DebugPrint(SC_TAG_RT, "[call] RT_FreeGuideTbl => FAIL !!"HERE);
				}
			}

			// 経路誘導開始応答
			RT_RGMsg_Send(e_SC_MSGID_RES_RT_GUIDEMAKE, e_SC_RESULT_SUCCESS);
			break;

		case e_SC_MSGID_REQ_RT_GUIDEADD:		// 経路誘導情報追加
			SC_LOG_InfoPrint(SC_TAG_RT, "[Msg] e_SC_MSGID_REQ_RT_GUIDEADD => GUIDE ADD !!");

			// 誘導テーブル作成
			ret = RT_TBL_AddGuideTbl(MAKETBL_DIV);
			if (e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_DebugPrint(SC_TAG_RT, "[call] RT_TBL_AddGuideTbl => FAIL !!"HERE);
				// 誘導テーブル解放
				ret = RT_TBL_FreeGuideTbl();
				if (e_SC_RESULT_SUCCESS != ret) {
					SC_LOG_DebugPrint(SC_TAG_RT, "[call] RT_FreeGuideTbl => FAIL !!"HERE);
				}
			}

			break;

		case e_SC_MSGID_REQ_RT_GUIDEFREE:		// 経路誘導情報解放
			SC_LOG_InfoPrint(SC_TAG_RT, "[Msg] e_SC_MSGID_REQ_RT_GUIDEFREE  =>  GUIDE FREE !!");

			// 誘導テーブル解放
			ret = RT_TBL_FreeGuideTbl();
			if (e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_DebugPrint(SC_TAG_RT, "[call] RT_FreeGuideTbl => FAIL !!"HERE);
			}

			// 経路誘導実行応答
			RT_RGMsg_Send(e_SC_MSGID_RES_RT_GUIDEFREE, e_SC_RESULT_SUCCESS);
			break;

		case e_SC_MSGID_REQ_RT_LISTMAKE:		// ターンリスト作成
			SC_LOG_InfoPrint(SC_TAG_RT, "[Msg] e_SC_MSGID_REQ_RT_LISTMAKE  =>  LIST MAKE !!");

			// ターンリスト作成
			ret = RT_LST_MakeTurnList();
			if (e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_DebugPrint(SC_TAG_RT, "[call] RT_LST_MakeTurnList => FAIL !!"HERE);
				// 誘導テーブル解放
				ret = RT_TBL_FreeGuideTbl();
				if (e_SC_RESULT_SUCCESS != ret) {
					SC_LOG_DebugPrint(SC_TAG_RT, "[call] RT_FreeGuideTbl => FAIL !!"HERE);
				}
			}

			// 経路誘導実行応答
			RT_RGMsg_Send(e_SC_MSGID_RES_RT_LISTMAKE, e_SC_RESULT_SUCCESS);
			break;

		default:
			SC_LOG_ErrorPrint(SC_TAG_RT, "[Msg] UNKNOWN MESSAGE. " HERE);
			break;
		}

	} else {
		// Unknown Sender
		SC_LOG_ErrorPrint(SC_TAG_RT, "[CTL] UNKNOWN MESSAG. " HERE);
	}

	SC_LOG_DebugPrint(SC_TAG_RT, SC_LOG_END);
}

/**
 * @brief	RGへの要求メッセージ送信
 * @param	[I]送信メッセージ
 * @memo	RG->FM
 */
static void RT_RGMsg_Send(E_SC_MSG_ID msg_type, E_SC_RESULT result)
{
	pthread_msq_msg_t sendMsg = {};
	pthread_msq_id_t* queue;

	// メッセージ設定
	sendMsg.data[SC_MSG_MSG_ID] = msg_type;
	sendMsg.data[SC_MSG_RES_RT_RESULT] = result;

	// キュー設定
	queue = ((pthread_msq_id_t*) SC_CORE_MSQID_RG);

	// メッセージ送信
	pthread_msq_msg_send(queue, &sendMsg, SC_CORE_MSQID_RT);
}

