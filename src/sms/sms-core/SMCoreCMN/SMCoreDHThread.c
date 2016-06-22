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
 * SMCoreDHThread.c
 *
 *  Created on: 2015/11/05
 *      Author:
 */

#include "SMCoreCMNInternal.h"

//-----------------------------------
// 関数定義
//-----------------------------------
#if 0
static E_SC_RESULT SC_DH_GetPclBinData(pthread_msq_msg_t *rcvMsg, INT32 mapKind);
static E_SC_RESULT SC_DH_CashFreePclBinData(pthread_msq_msg_t *rcvMsg, INT32 mapKind);
#endif

// メッセージ処理関数のポインタ
typedef E_SC_RESULT (*SC_DH_MsgFunc)(pthread_msq_msg_t *rcvMsg);

#define DH_USER_CHECK(user)			((e_SC_USER_MP <= (user)) && (e_SC_USER_END > (user)))
//-----------------------------------
// 構造体定義
//-----------------------------------

// メッセージテーブル
typedef struct _SC_DH_MSG_DATAINFO {
	INT32				reqType;
	SC_DH_MsgFunc		func;
} SC_DH_MSG_DATAINFO;

// メッセージ受信後のメッセージ処理関数
const static SC_DH_MSG_DATAINFO	msgFunc[e_SC_DM_MAPREQCODE_END] = {
#if 1
	{e_SC_DM_RESTYPE_ALL, 		(SC_DH_MsgFunc)SC_DM_MapRead_MsgRes},
	{e_SC_DM_RESTYPE_ONE, 		(SC_DH_MsgFunc)SC_DM_MapRead_MsgRes},
	{e_SC_DM_RESTYPE_CANCEL, 	(SC_DH_MsgFunc)NULL},
	{e_SC_DM_RESTYPE_ERROR, 	(SC_DH_MsgFunc)NULL},
	{e_SC_DM_RESTYPE_FREE, 		(SC_DH_MsgFunc)NULL},
	{e_SC_DM_RESTYPE_SEM, 		(SC_DH_MsgFunc)SC_DM_MapRead_SemRes},
	{e_SC_DM_FREETYPE_USER,		(SC_DH_MsgFunc)SC_DM_MapFree_NoRes},
	{e_SC_DM_FREETYPE_KIND, 	(SC_DH_MsgFunc)SC_DM_MapFree_NoRes},
	{e_SC_DM_FREETYPE_DITAIL, 	(SC_DH_MsgFunc)SC_DM_MapFree_MsgRes},
#else
	// データ取得,					// キャッシュ解放
	{e_DATA_KIND_ROAD, 			(SC_DH_MsgFunc)SC_DH_GetPclBinData,		(SC_DH_MsgFunc)SC_DH_CashFreePclBinData},		// パーセル道路データ
	{e_DATA_KIND_BKGD, 			(SC_DH_MsgFunc)SC_DH_GetPclBinData,		(SC_DH_MsgFunc)SC_DH_CashFreePclBinData},		// パーセル背景データ
	{e_DATA_KIND_NAME,			(SC_DH_MsgFunc)NULL,					(SC_DH_MsgFunc)NULL},							// 名称データ
	{e_DATA_KIND_ROAD_NAME,		(SC_DH_MsgFunc)NULL,					(SC_DH_MsgFunc)NULL},							// 道路名称データ
	{e_DATA_KIND_BKGD_NAME,		(SC_DH_MsgFunc)NULL,					(SC_DH_MsgFunc)NULL},							// 道路名称データ
	{e_DATA_KIND_GUIDE,			(SC_DH_MsgFunc)NULL,					(SC_DH_MsgFunc)NULL},							// 誘導データ
	{e_DATA_KIND_CHARSTR,		(SC_DH_MsgFunc)NULL,					(SC_DH_MsgFunc)NULL},							// 文言データ
	{e_DATA_KIND_SHAPE,			(SC_DH_MsgFunc)SC_DH_GetPclBinData,		(SC_DH_MsgFunc)SC_DH_CashFreePclBinData}		// 形状データ
#endif
};

/**
 * @brief 初期化処理を行う
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_DH_Initialize()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	// DataMng初期化
	ret = SC_DM_DataMngInit();
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_DH, "SC_DM_DataMngInit() error, " HERE);
	}

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}

/**
 * @brief 終了化処理を行う
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_DH_Finalize()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	// DataMng終了
	ret = SC_DM_DataMngFinalize();
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_DH, "SC_DM_DataMngFinalize() error, " HERE);
	}

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}

/**
 * @brief スレッドメイン関数
 * @param[in] param スレッドメイン関数引数構造体のポインタ
 * @return NULL
 */
void *SC_DH_ThreadMain(void *param)
{
	//E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	rc = 0;
	pthread_msq_msg_t	rmsg = {};

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	while (true != SC_Thread_GetIsFinish()) {
		// メッセージ初期化
		memset(&rmsg, 0, sizeof(pthread_msq_msg_t));

		// メッセージ受信
		rc = pthread_msq_msg_receive(SC_CORE_MSQID_DH, &rmsg);
		if (PTHREAD_MSQ_OK != rc) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "pthread_msq_msg_receive error(0x%08x), " HERE, rc);
			continue;
		}

		// 受信メッセージをログ出力
		SC_LOG_DebugPrint(SC_TAG_DH,
				"recvMsg=0x%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x, " HERE,
				rmsg.data[0],  rmsg.data[1],  rmsg.data[2],  rmsg.data[3],  rmsg.data[4],
				rmsg.data[5],  rmsg.data[6],  rmsg.data[7],  rmsg.data[8],  rmsg.data[9]);


		// メッセージ種別IDチェック
		if (true != SC_MSG_CHECK_MSG_ID(&rmsg)) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "recv msgid error, " HERE);
			continue;
		}

		// メッセージディスパッチ
		SC_MSG_MsgDispatch(&rmsg, SC_CORE_MSQID_DH);
	}

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (NULL);
}

/**
 * @brief メッセージディスパッチ
 * @param[in] rcvMsg 受信メッセージ構造体のポインタ
 */
void SC_DH_MsgDispatch(pthread_msq_msg_t *rcvMsg)
{
	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);
	INT32	type = 0;

#if 0
	// 地図データ種別IDチェック
	if (true != SC_MSG_CHECK_MAP_DATA_ID(rcvMsg)) {
		SC_LOG_ErrorPrint(SC_TAG_DH, "recv mapid error, " HERE);
	}
#endif

	switch (rcvMsg->data[SC_MSG_MSG_ID]) {
	case e_SC_MSGID_REQ_MAP_DATA:		// 地図データ取得要求
		// 地図データ応答種別
		type = rcvMsg->data[SC_MSG_REQ_MAP_READ_RESTYPE];
		if (NULL != msgFunc[type].func) {
			// 地図データ取得関数呼び出し
			msgFunc[type].func(rcvMsg);
		}
		break;
	case e_SC_MSGID_REQ_MAP_CASH_FREE:	// 地図データキャッシュ解放
		// 地図データ種別ID取得
		type = rcvMsg->data[SC_MSG_REQ_MAP_FREE_FREETYPE];
		if (NULL != msgFunc[type].func) {
			// 地図データキャッシュ解放関数呼び出し
			msgFunc[type].func(rcvMsg);
		}
		break;
	case e_SC_MSGID_EVT_FINISH:			// 終了通知
		SC_LOG_DebugPrint(SC_TAG_DH, "recv msgid finish, " HERE);
		break;
	default:							// 不正なメッセージ種別ID
		SC_LOG_ErrorPrint(SC_TAG_DH, "recv msgid error, " HERE);
		break;
	}

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);
}
#if 0
/**
 * @brief パーセルバイナリデータ取得
 * @param[in] rcvMsg  受信メッセージ構造体のポインタ
 * @param[in] mapKind 地図データ種別
 */
E_SC_RESULT SC_DH_GetPclBinData(pthread_msq_msg_t *rcvMsg, INT32 mapKind)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_DHC_CASH_RESULT	cashRet = e_DHC_RESULT_CASH_SUCCESS;
	pthread_msq_msg_t	sndMsg = {};
	T_DHC_REQ_INFO	req = {};
	void	*data = NULL;
	pthread_msq_id_t	*sender = NULL;

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	// メッセージ送信元取得
	sender = (pthread_msq_id_t*)pthread_msq_msg_issender(rcvMsg);

	// 要求データ設定
	req.user     = (UINT16)rcvMsg->data[SC_MSG_USER_ID];			// ユーザ
	req.parcelId = rcvMsg->data[SC_MSG_RQ_MAP_KEY1];				// パーセルID
	req.mapKind  = mapKind;											// 地図データ種別

	// 地図読み要求
	cashRet = SC_DHC_MapRead(req, &data);
	if (e_DHC_RESULT_CASH_SUCCESS != cashRet) {
		SC_LOG_ErrorPrint(SC_TAG_DH, "SC_DHC_MapRead error(0x%08x), " HERE, cashRet);
		ret = e_SC_RESULT_MAP_GETERR;
	}

	// 送信メッセージ生成
	sndMsg.data[SC_MSG_MSG_ID] = e_SC_MSGID_RES_MAP_DATA;			// メッセージ種別ID（地図データ取得応答）
	sndMsg.data[SC_MSG_MAP_ID] = rcvMsg->data[SC_MSG_MAP_ID];		// 地図データ種別ID
	sndMsg.data[SC_MSG_RES_MAP_RESULT] = (int)ret;					// 処理結果
	if (e_SC_RESULT_SUCCESS == ret) {
		sndMsg.data[SC_MSG_RES_MAP_DTADR] = (int)data;				// バイナリデータ格納バッファの先頭アドレス
	}

	// 送信メッセージをログ出力
	SC_LOG_DebugPrint(SC_TAG_DH,
			"sendMsg=0x%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x, " HERE,
			sndMsg.data[0],  sndMsg.data[1],  sndMsg.data[2],  sndMsg.data[3],  sndMsg.data[4],
			sndMsg.data[5],  sndMsg.data[6],  sndMsg.data[7],  sndMsg.data[8],  sndMsg.data[9]);

	// メッセージ送信
	if (PTHREAD_MSQ_OK != pthread_msq_msg_send(sender, &sndMsg, SC_CORE_MSQID_DH)) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "pthread_msq_msg_send error, " HERE);
		if (e_SC_RESULT_SUCCESS == ret) {
			ret = e_SC_RESULT_FAIL;
		}
	}

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}

/**
 * @brief パーセルバイナリデータキャッシュ解放
 * @param[in] rcvMsg  受信メッセージ構造体のポインタ
 * @param[in] mapKind 地図データ種別
 */
E_SC_RESULT SC_DH_CashFreePclBinData(pthread_msq_msg_t *rcvMsg, INT32 mapKind)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_DHC_CASH_RESULT	cashRet = e_DHC_RESULT_CASH_SUCCESS;
	T_DHC_REQ_INFO	req = {};

	// 要求データ設定
	req.user     = (UINT16)rcvMsg->data[SC_MSG_USER_ID];			// ユーザ
	req.parcelId = rcvMsg->data[SC_MSG_RQ_MAP_KEY1];				// パーセルID
	req.mapKind  = mapKind;											// 地図データ種別

	// 地図開放要求
	cashRet = SC_DHC_MapFree(req);
	if (e_DHC_RESULT_CASH_SUCCESS != cashRet) {
		SC_LOG_ErrorPrint(SC_TAG_DH, "SC_DHC_MapFree error(0x%08x), " HERE, cashRet);
		ret = e_SC_RESULT_FAIL;
	}

	return (ret);
}
#endif
