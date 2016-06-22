/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#include "sms-core/SMCorePB/SMCorePBInternal.h"

/**
 * @brief	受信メッセージ振り分け処理
 * @param	[I]受信メッセージ
 */
void SC_PU_MsgAnalyze(pthread_msq_msg_t *msg)
{
	//E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_PU, SC_LOG_START);

	switch (msg->data[SC_MSG_MSG_ID]) {
	case e_SC_MSGID_REQ_PU_UPLOAD:		// アップロード要求
		// アップロード
		SC_PU_Upload(msg);
		break;
	case e_SC_MSGID_EVT_FINISH:			// 終了通知
		SC_LOG_DebugPrint(SC_TAG_PU, "recv msgid finish, " HERE);
		break;
	default:							// 不正なメッセージ種別ID
		SC_LOG_ErrorPrint(SC_TAG_PU, "recv msgid error, " HERE);
		break;
	}

	SC_LOG_DebugPrint(SC_TAG_PU, SC_LOG_END);
}
