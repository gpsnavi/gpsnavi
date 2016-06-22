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
 * SMCoreTRDispatch.c
 *
 *  Created on: 2016/01/04
 *      Author: n.kanagawa
 */
#include "sms-core/SMCoreTR/SMCoreTRInternal.h"


/**
 * @brief	送信元別メッセージ振り分け処理
 * @param	[I]受信メッセージ
 */
void SC_TR_MsgAnalyze(pthread_msq_msg_t* aMsg)
{
	E_SC_RESULT			ret = e_SC_RESULT_SUCCESS;
	E_SC_MSG_ID			msgType;
	E_SC_TR_EVENT		evtType;
	UINT32				parcelId = 0;
	SMTRAFFIC			trfInfo = {};

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == aMsg) {
			SC_LOG_ErrorPrint(SC_TAG_TR, "param error, " HERE);
			break;
		}

		// 共有メモリ取得
		ret = SC_MNG_GetTrafficInfo(&trfInfo);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_TR, "SC_MNG_GetTrafficInfo error, " HERE);
			break;
		}
		if (!trfInfo.disp) {
			// 表示OFFの場合処理しない
			break;
		}

		// メッセージ種別取得
		msgType = (E_SC_MSG_ID)aMsg->data[SC_MSG_MSG_ID];
		// イベント種別取得
		evtType = (E_SC_TR_EVENT)aMsg->data[SC_MSG_REQ_TR_EVT];

		// メッセージ振り分け
		switch (msgType) {
		case e_SC_MSGID_REQ_TR_UPDATE:	// リクエスト
			switch(evtType) {
			case e_SC_TR_EVENT_CARPOS:	// 自車周辺更新
				SC_LOG_InfoPrint(SC_TAG_TR, "[Msg] e_SC_MSGID_REQ_TR_UPDATE:e_SC_TR_EVENT_CARPOS");
				SC_TR_CacheDiffUpdate(TR_USERTYPE_CARPOS, parcelId);
				break;

			case e_SC_TR_EVENT_SCROLL:	// スクロール地点周辺更新
				SC_LOG_InfoPrint(SC_TAG_TR, "[Msg] e_SC_MSGID_REQ_TR_UPDATE:e_SC_TR_EVENT_SCROLL");
				SC_TR_CacheDiffUpdate(TR_USERTYPE_SCROLL, parcelId);
				break;
#if 0
			case e_SC_TR_EVENT_MANUAL:	// 手動更新
				SC_LOG_InfoPrint(SC_TAG_TR, "[Msg] e_SC_MSGID_REQ_TR_UPDATE:e_SC_TR_EVENT_MANUAL");
				SC_TR_CacheAllUpdate(TR_USERTYPE_CARPOS);
				break;

			case e_SC_TR_EVENT_TIMER:	// タイマー更新
				SC_LOG_InfoPrint(SC_TAG_TR, "[Msg] e_SC_MSGID_REQ_TR_UPDATE:e_SC_TR_EVENT_TIMER");
				SC_TR_CacheAllUpdate(TR_USERTYPE_CARPOS);
				SC_TRT_SendStratTimerMsg(SC_CORE_MSQID_TR, (INT32)trfInfo.updateTime);
				break;
#endif
			default:
				SC_LOG_InfoPrint(SC_TAG_TR, "[Msg] e_SC_MSGID_REQ_TR_UPDATE:none");
				break;
			}
			break;

		case e_SC_MSGID_EVT_TIMEOUT:	// 監視タイマT.O.
			SC_LOG_InfoPrint(SC_TAG_TR, "[Msg] e_SC_MSGID_EVT_TIMEOUT:");
			SC_TR_CacheAllUpdate(TR_USERTYPE_CARPOS);
			SC_TRT_SendStratTimerMsg(SC_CORE_MSQID_TR, (INT32)trfInfo.updateTime);
			break;

		case e_SC_MSGID_EVT_FINISH:		// 終了通知
			SC_LOG_InfoPrint(SC_TAG_TR, "recv msgid finish, " HERE);
			break;

		default:
			SC_LOG_DebugPrint(SC_TAG_TR, "[Msg] none");
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_END);
}
