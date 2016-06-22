/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#include "sms-core/SMCoreSD/SMCoreSDInternal.h"

/**
 * @brief	受信メッセージ振り分け処理
 * @param	[I]受信メッセージ
 */
void SC_SDU_MsgAnalyze(const pthread_msq_msg_t *msg)
{
	//E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_SDU, SC_LOG_START);

	switch (msg->data[SC_MSG_MSG_ID]) {
	case e_SC_MSGID_REQ_SDU_UPLOAD:		// アップロード要求
		// アップロード
		SC_SDU_UploadReq(msg);
		break;

	case e_SC_MSGID_REQ_SDU_STOP:		// アップロード終了
		// アップロード終了
		SC_SDU_UploadStopReq(msg);
		break;

	case e_SC_MSGID_EVT_FINISH:			// 終了通知
		SC_LOG_DebugPrint(SC_TAG_SDU, "recv msgid finish, " HERE);
		break;

	default:							// 不正なメッセージ種別ID
		SC_LOG_ErrorPrint(SC_TAG_SDU, "recv msgid error, " HERE);
		break;
	}

	SC_LOG_DebugPrint(SC_TAG_SDU, SC_LOG_END);
}
