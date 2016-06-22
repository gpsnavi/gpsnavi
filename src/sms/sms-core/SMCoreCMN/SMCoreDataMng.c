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
 * SMCoreDataMng.c
 *
 *  Created on: 2014/02/27
 *      Author: 70251034
 */

#include "SMCoreCMNInternal.h"

/*-------------------------------------------------------------------
 * 定数定義
 *-------------------------------------------------------------------*/
#define DM_CANCEL_USE
#define DM_CANCEL_MAX			(20)			// キャンセル蓄積最大
#define DM_MSG_SEND_RETRYCOUNT	(3)				// msg送信リトライ
/*-------------------------------------------------------------------
 * 構造体定義
 *-------------------------------------------------------------------*/
typedef struct _SC_DM_USER_MNG {
	UINT32 reqId;								// 地図要求ID
	UINT32 finishCnt;							// 読み込み済み地図
	UINT32 cancelCode[DM_CANCEL_MAX];			// キャンセルコード
	UINT32 cancelVol;							// キャンセル数
} SC_DM_USER_MNG;
/*-------------------------------------------------------------------
 * 変数宣言
 *-------------------------------------------------------------------*/
static SC_MUTEX m_Mutex[e_SC_DH_USER_ID_END] = {};
static SC_DM_USER_MNG m_UserMng[e_SC_DH_USER_ID_END] = {};
static SC_DM_MAPREQ m_SaveReq[SC_DM_MAPREQ_MAX];
/*-------------------------------------------------------------------
 * マクロ定義
 *-------------------------------------------------------------------*/
#define DH_USER_CHECK(a)		((e_SC_USER_MP <= (a)) && (e_SC_DH_USER_ID_END > (a)))
#define DH_MUTEX_LOCK(mutex)														\
	{																				\
		E_SC_RESULT mRet = e_SC_RESULT_SUCCESS;										\
		mRet = SC_LockMutex(mutex);													\
		if (mRet != e_SC_RESULT_SUCCESS) {											\
			SC_LOG_ErrorPrint(SC_TAG_DH, "[RtMng] SC_LockMutext error, " HERE);		\
			return (mRet);															\
		}																			\
	}
#define DH_MUTEX_UNLOCK(mutex)														\
	{																				\
		E_SC_RESULT mRet = e_SC_RESULT_SUCCESS;										\
		mRet = SC_UnLockMutex(mutex);												\
		if (mRet != e_SC_RESULT_SUCCESS) {											\
			SC_LOG_ErrorPrint(SC_TAG_DH, "[RtMng] SC_UnLockMutext error, " HERE);	\
			return (mRet);															\
		}																			\
	}
#define DM_CANCELCODE_DELETE(code, vol)												\
	{																				\
		UINT32 tmp[DM_CANCEL_MAX] = {};											\
		memcpy(tmp, &code[vol], sizeof(UINT32) * (DM_CANCEL_MAX - vol));			\
		memcpy(code, tmp, sizeof(UINT32) * DM_CANCEL_MAX);							\
	}
#define DM_SET_DHC_USER_ID(a, b)													\
	{																				\
		switch (a) {																\
		case e_SC_USER_MP:															\
			b = SC_DHC_USER_MAP;													\
			break;																	\
		case e_SC_USER_RP:															\
			b = SC_DHC_USER_RP;														\
			break;																	\
		case e_SC_USER_LC:															\
			b = SC_DHC_USER_LC;														\
			break;																	\
		case e_SC_USER_DM:															\
			b = SC_DHC_USER_RG;														\
			break;																	\
		case e_SC_USER_PI:															\
			b = SC_DHC_USER_PI;														\
			break;																	\
		}																			\
	}
/*-------------------------------------------------------------------
 * 宣言
 *-------------------------------------------------------------------*/
static E_SC_RESULT SC_DM_InitFinishCnt(E_SC_USER_ID aUser, UINT16 aReqId);
static E_SC_RESULT SC_DM_SetFinishCnt(E_SC_USER_ID aUser, UINT16 aCnt);
static Bool SC_DM_CancelRead();
static Bool SC_DM_CancelJudge(UINT32 aUser, UINT32 aReqId);
static E_SC_RESULT SC_DM_MsgSend(pthread_msq_id_t* aReciver, pthread_msq_msg_t* aMsg, pthread_msq_id_t* aSender);


/**
 * @brief	地図管理初期化
 * @memo	DHThread初期化時に呼び出す。
 */
E_SC_RESULT SC_DM_DataMngInit()
{
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	UINT8 i;

	// Mutex生成
	for (i = 0; i < e_SC_DH_USER_ID_END; i++) {
		ret = SC_CreateMutex(&m_Mutex[i]);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_CreateMutext error, " HERE);
			return (e_SC_RESULT_FAIL);
		}
	}
	return (ret);
}

/**
 * @brief	地図管理終了処理
 */
E_SC_RESULT SC_DM_DataMngFinalize()
{
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	UINT8 i;

	// Mutex破棄
	for (i = 0; i < e_SC_DH_USER_ID_END; i++) {
		ret = SC_DestroyMutex(&m_Mutex[i]);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_CreateMutext error, " HERE);
			return (e_SC_RESULT_FAIL);
		}
	}
	return (ret);
}

/**
 * @brief	管理情報初期化
 * @param	[I]ユーザID
 * @param	[O]要求ID
 * @memo	地図読み込み済み件数及び要求IDの初期化処理。
 * @memo	新規地図要求・地図削除要求いずれかの要求時に初期化。
 */
static E_SC_RESULT SC_DM_InitFinishCnt(E_SC_USER_ID aUser, UINT16 aReqId)
{
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	// [排他制御開始]
	DH_MUTEX_LOCK(&m_Mutex[aUser]);

	m_UserMng[aUser].reqId = aReqId;
	m_UserMng[aUser].finishCnt = 0;

	// [排他制御終了]
	DH_MUTEX_UNLOCK(&m_Mutex[aUser]);

	return (ret);
}

/**
 * @brief	管理情報更新
 * @param	[I]ユーザID
 * @param	[I]地図読み込み済み件数
 */
static E_SC_RESULT SC_DM_SetFinishCnt(E_SC_USER_ID aUser, UINT16 aCnt)
{
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	// [排他制御開始]
	DH_MUTEX_LOCK(&m_Mutex[aUser]);

	m_UserMng[aUser].finishCnt = aCnt;

	// [排他制御終了]
	DH_MUTEX_UNLOCK(&m_Mutex[aUser]);

	return (ret);
}

/**
 * @brief	地図読み込み数取得
 * @param	[I]ユーザID
 * @param	[I]要求ID
 * @param	[O]地図読み込み件数格納アドレス
 */
E_SC_RESULT SC_DM_GetFinishCnt(E_SC_USER_ID aUser, UINT32 aReqId, UINT32* aCnt)
{
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	// [排他制御開始]
	DH_MUTEX_LOCK(&m_Mutex[aUser]);

	if (aReqId == m_UserMng[aUser].reqId) {
		*aCnt = m_UserMng[aUser].finishCnt;
	} else {
		ret = e_SC_RESULT_BADPARAM;
	}

	// [排他制御終了]
	DH_MUTEX_UNLOCK(&m_Mutex[aUser]);

	return (ret);
}

/**
 * @brief	キャンセル
 * @param	[I]受信メッセージ構造体のポインタ
 * @memo	キャンセルキューからMSGを取り出す
 */
static Bool SC_DM_CancelRead()
{
	Bool ret = false;
	UINT32 user;
	pthread_msq_msg_t rmsg = {};
	pthread_msq_id_t* queue = SC_CORE_MSQID_DC;
	UINT32 reqId;

	// メッセージ受信
	while (PTHREAD_MSQ_OK == pthread_msq_msg_receive_try(queue, &rmsg)) {
		// キャンセルなら処理
		if (e_SC_MSGID_REQ_MAP_CANCEL != rmsg.data[SC_MSG_MSG_ID]) {
			continue;
		}
		// Cancel登録
		reqId = rmsg.data[SC_MSG_REQ_MAP_CAN_REQID];
		user = rmsg.data[SC_MSG_REQ_MAP_CAN_USER_ID];
		if ((0 <= user) && (user < e_SC_DH_USER_ID_END)) {
			if (DM_CANCEL_MAX <= m_UserMng[user].cancelVol) {
				// 1個削除
				DM_CANCELCODE_DELETE(m_UserMng[user].cancelCode, 1);
				m_UserMng[user].cancelVol--;
			}
			// Cancel要求ID格納
			m_UserMng[user].cancelCode[m_UserMng[user].cancelVol] = reqId;
			// カウント
			m_UserMng[user].cancelVol++;
			// キャンセル受信あり
			ret = true;
		}
	}

	return (ret);
}

/**
 * @brief	キャンセル
 * @param	[I]受信メッセージ構造体のポインタ
 * @memo	キャンセルキューからMSGを取り出す
 */
static Bool SC_DM_CancelJudge(UINT32 aUser, UINT32 aReqId)
{
	Bool ret = false;
	UINT32 i;

	// メッセージ受信
	for (i = 0; i < m_UserMng[aUser].cancelVol; i++) {
		if (aReqId != m_UserMng[aUser].cancelCode[i]) {
			continue;
		}
		// 現状の位置以降のデータのみ残す
		DM_CANCELCODE_DELETE(m_UserMng[aUser].cancelCode, (m_UserMng[aUser].cancelVol - i));
		m_UserMng[aUser].cancelVol -= (i + 1);

		// 結果
		ret = true;
		break;
	}

	return (ret);
}

/**
 * @brief	MSG送信
 * @param	[I]送信者
 * @param	[I]受信者
 * @param	[I]MSG
 * @memo	リトライあり
 */
static E_SC_RESULT SC_DM_MsgSend(pthread_msq_id_t* aReciver, pthread_msq_msg_t* aMsg, pthread_msq_id_t* aSender)
{

	UINT8 retry = 0;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	while (DM_MSG_SEND_RETRYCOUNT > retry) {
		// メッセージ送信
		if (PTHREAD_MSQ_OK == pthread_msq_msg_send(aReciver, aMsg, aSender)) {
			break;
		}
		// リトライカウントアップ
		retry++;
		SC_LOG_ErrorPrint(SC_TAG_RC, "pthread_msq_msg_send failed. retry(%d)"HERE, retry);
	}
	if (DM_MSG_SEND_RETRYCOUNT == retry) {
		ret = e_SC_RESULT_FAIL;
	}
	return (ret);
}

/**
 * @brief	地図アドレス取得IF
 * @param	[I]ユーザID
 * @param	[I]要求テーブル
 * @param	[O]地図アドレス取得結果格納アドレス
 * @memo	SC_DHC_MapWatchラッパー
 */
E_SC_RESULT SC_DM_MapWatch(E_SC_USER_ID aUser, SC_DM_MAPREQ* aReq, void** aBin)
{
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	E_DHC_CASH_RESULT casRet = e_DHC_RESULT_CASH_SUCCESS;
	T_DHC_REQ_INFO req;

	if ((NULL == aReq) || (NULL == aBin)) {
		return (e_SC_RESULT_BADPARAM);
	}
	req.mapKind = aReq->kind;
	req.parcelId = aReq->pclId;
	req.user = aUser;

	//casRet = SC_DHC_MapWatch(req, aBin);
	if (casRet != e_DHC_RESULT_CASH_SUCCESS) {
		ret = e_SC_RESULT_FAIL;
	}

	return (ret);
}

/**
 * @brief	地図読み込み（MSG応答用）
 * @param	[I]受信メッセージ構造体のポインタ
 * @memo	地図読み込み処理後MSG送信者へMSG応答を行う
 */
E_SC_RESULT SC_DM_MapRead_MsgRes(pthread_msq_msg_t *rcvMsg)
{
	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	pthread_msq_id_t *sender = NULL;
	pthread_msq_msg_t sndMsg = {};
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	E_DHC_CASH_RESULT cashRet = e_DHC_RESULT_CASH_SUCCESS;
	T_DHC_REQ_PARCEL mapReqPcl = {};
	T_DHC_RES_DATA mapResData = {};
	SC_DM_MAPREQ* req;
	UINT32 i;
	UINT32 e;
	UINT32 user;
	//UINT32 msgCnt = 0;
	UINT32 cnt = 0;
	Bool cancel = false;

	// メッセージ送信元取得
	sender = (pthread_msq_id_t*) pthread_msq_msg_issender(rcvMsg);

	do {
		user = rcvMsg->data[SC_MSG_REQ_MAP_READ_USER_ID];
		req = (SC_DM_MAPREQ*) rcvMsg->data[SC_MSG_REQ_MAP_READ_TBL];
		// ユーザ確認
		if (true != DH_USER_CHECK(user)) {
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		// パラメタ確認
		if (NULL == req) {
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((SC_DM_MAPREQ_MAX < rcvMsg->data[SC_MSG_REQ_MAP_READ_VOL]) || (0 >= rcvMsg->data[SC_MSG_REQ_MAP_READ_VOL])) {
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		// 管理情報初期化
		ret = SC_DM_InitFinishCnt(user, rcvMsg->data[SC_MSG_REQ_MAP_READ_REQID]);
		if (e_SC_RESULT_SUCCESS != ret) {
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

#ifdef DM_CANCEL_USE
		cancel = SC_DM_CancelJudge(user, rcvMsg->data[SC_MSG_REQ_MAP_READ_REQID]);
		if (cancel) {
			break;
		}
#endif
		// 要求をバックアップ
		memcpy(m_SaveReq, req, sizeof(SC_DM_MAPREQ) * rcvMsg->data[SC_MSG_REQ_MAP_READ_VOL]);
		// ユーザID取得
		//DM_SET_DHC_USER_ID(user, dhcReq.user);

		// 要求内容処理
		for (i = 0; i < rcvMsg->data[SC_MSG_REQ_MAP_READ_VOL]; i++) {
#ifdef DM_CANCEL_USE
			if (SC_DM_CancelRead()) {
				cancel = SC_DM_CancelJudge(user, rcvMsg->data[SC_MSG_REQ_MAP_READ_REQID]);
				if (cancel) {
					for (e = 0; e < i; e++) {
						mapReqPcl.parcelNum = 1;
						mapReqPcl.parcelInfo[0].parcelId = m_SaveReq[e].pclId;
						mapReqPcl.parcelInfo[0].mapKind = SC_DHC_GetKindMask(m_SaveReq[e].kind);
						cashRet = SC_DHC_MapFree(&mapReqPcl);
						if (e_DHC_RESULT_CASH_SUCCESS != cashRet) {
							SC_LOG_ErrorPrint(SC_TAG_DH, "SC_DHC_MapFree error(0x%08x), " HERE, cashRet);
						}
					}
					SC_LOG_DebugPrint(SC_TAG_DH, "map read cancel. data free. %d/%d", e,
							rcvMsg->data[SC_MSG_REQ_MAP_READ_VOL]);
					break;
				}
			}
#endif
			// 要求データ設定
			mapReqPcl.parcelNum = 1;
			mapReqPcl.parcelInfo[0].parcelId = m_SaveReq[i].pclId;
			mapReqPcl.parcelInfo[0].mapKind = SC_DHC_GetKindMask(m_SaveReq[i].kind);
			// 地図読み要求
			cashRet = SC_DHC_MapRead(&mapReqPcl, &mapResData);
			if (e_DHC_RESULT_CASH_SUCCESS != cashRet) {
				SC_LOG_ErrorPrint(SC_TAG_DH, "SC_DHC_MapRead error(0x%08x), " HERE, cashRet);
			}
#if 0
			if (e_DHC_RESULT_CASH_SUCCESS != cashRet) {
				SC_LOG_ErrorPrint(SC_TAG_DH, "SC_DHC_MapRead error(0x%08x), " HERE, cashRet);
				(req + i)->bin = (UINT8*) NULL;
			} else {
				(req + i)->bin = (UINT8*) data;
			}
#endif
			// 初回応答
			if ((0 == i) && (e_SC_DM_RESTYPE_ONE == rcvMsg->data[SC_MSG_REQ_MAP_READ_RESTYPE])) {
				// 送信メッセージ生成
				sndMsg.data[SC_MSG_MSG_ID] = e_SC_MSGID_RES_MAP_DATA;
				sndMsg.data[SC_MSG_RES_MAP_RESTYPE] = e_SC_DM_RESTYPE_ONE;
				sndMsg.data[SC_MSG_RES_MAP_REQID] = rcvMsg->data[SC_MSG_REQ_MAP_READ_REQID];
				sndMsg.data[SC_MSG_RES_MAP_TBL] = rcvMsg->data[SC_MSG_REQ_MAP_READ_TBL];
				// メッセージ送信
				SC_DM_MsgSend(sender, &sndMsg, SC_CORE_MSQID_DH);
			}
			// 管理情報更新
			SC_DM_SetFinishCnt(user, (i + 1));
		}
#ifdef DM_CANCEL_USE
		if (cancel) {
			// 管理情報初期化
			SC_DM_InitFinishCnt(user, 0);
			ret = e_SC_RESULT_CANCEL;
			break;
		}
#endif

		// 念のため件数確認
		SC_DM_GetFinishCnt(user, rcvMsg->data[SC_MSG_REQ_MAP_READ_REQID], &cnt);
		if (rcvMsg->data[SC_MSG_REQ_MAP_READ_VOL] != cnt) {
			// mutexによる設定エラー。再登録してみる
			SC_DM_SetFinishCnt(user, i);
		}
	} while (0);

	// 地図取得以前のエラー
	if (e_SC_RESULT_SUCCESS == ret) {
		sndMsg.data[SC_MSG_MSG_ID] = e_SC_MSGID_RES_MAP_DATA;
		sndMsg.data[SC_MSG_RES_MAP_RESTYPE] = e_SC_DM_RESTYPE_ALL;
		sndMsg.data[SC_MSG_RES_MAP_REQID] = rcvMsg->data[SC_MSG_REQ_MAP_READ_REQID];
		sndMsg.data[SC_MSG_RES_MAP_TBL] = rcvMsg->data[SC_MSG_REQ_MAP_READ_TBL];
		SC_LOG_DebugPrint(SC_TAG_DH, "DataMng reseive read finish.");
#ifdef DM_CANCEL_USE
	} else if (e_SC_RESULT_CANCEL == ret) {
		sndMsg.data[SC_MSG_MSG_ID] = e_SC_MSGID_RES_MAP_DATA;
		sndMsg.data[SC_MSG_RES_MAP_RESTYPE] = e_SC_DM_RESTYPE_CANCEL;
		sndMsg.data[SC_MSG_RES_MAP_REQID] = rcvMsg->data[SC_MSG_REQ_MAP_READ_REQID];
		sndMsg.data[SC_MSG_RES_MAP_TBL] = rcvMsg->data[SC_MSG_REQ_MAP_READ_TBL];
		SC_LOG_DebugPrint(SC_TAG_DH, "DataMng reseive cancel.");
#endif
	} else {
		sndMsg.data[SC_MSG_MSG_ID] = e_SC_MSGID_RES_MAP_DATA;
		sndMsg.data[SC_MSG_RES_MAP_RESTYPE] = e_SC_DM_RESTYPE_ERROR;
		sndMsg.data[SC_MSG_RES_MAP_REQID] = rcvMsg->data[SC_MSG_REQ_MAP_READ_REQID];
		sndMsg.data[SC_MSG_RES_MAP_TBL] = rcvMsg->data[SC_MSG_REQ_MAP_READ_TBL];
		SC_LOG_DebugPrint(SC_TAG_DH, "DataMng reseive error.");
	}
	// メッセージ送信
	SC_DM_MsgSend(sender, &sndMsg, SC_CORE_MSQID_DH);

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}

/**
 * @brief	地図読み込み（セマフォ応答用）
 * @param	[I]受信メッセージ構造体のポインタ
 * @memo	地図読み込み処理後セマフォアンロックを行う
 */
E_SC_RESULT SC_DM_MapRead_SemRes(pthread_msq_msg_t *rcvMsg)
{
	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	SC_SEMAPHORE* apiSem = NULL;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	E_DHC_CASH_RESULT cashRet = e_DHC_RESULT_CASH_SUCCESS;
	T_DHC_REQ_PARCEL mapReqPcl = {};
	T_DHC_RES_DATA mapResData = {};
	SC_DM_MAPREQ* req;
	UINT32 i;
	UINT32 user;

	do {
		apiSem = (SC_SEMAPHORE*) rcvMsg->data[SC_MSG_REQ_MAP_READ_SYNCSEM];
		user = rcvMsg->data[SC_MSG_REQ_MAP_READ_USER_ID];
		req = (SC_DM_MAPREQ*) rcvMsg->data[SC_MSG_REQ_MAP_READ_TBL];
		// ユーザ確認
		if (true != DH_USER_CHECK(user)) {
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		// パラメタ確認
		if ((NULL == apiSem) || (NULL == req)) {
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((SC_DM_MAPREQ_MAX < rcvMsg->data[SC_MSG_REQ_MAP_READ_VOL]) || (0 >= rcvMsg->data[SC_MSG_REQ_MAP_READ_VOL])) {
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		// 管理情報初期化
		ret = SC_DM_InitFinishCnt(user, rcvMsg->data[SC_MSG_REQ_MAP_READ_REQID]);
		if (e_SC_RESULT_SUCCESS != ret) {
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// ユーザID取得
		//DM_SET_DHC_USER_ID(user, dhcReq.user);

		// 要求内容処理
		for (i = 0; i < rcvMsg->data[SC_MSG_REQ_MAP_READ_VOL]; i++) {
			// 要求データ設定
			mapReqPcl.parcelNum = 1;
			mapReqPcl.parcelInfo[0].parcelId = (req + i)->pclId;
			mapReqPcl.parcelInfo[0].mapKind = SC_DHC_GetKindMask((req + i)->kind);
			// 地図読み要求
			cashRet = SC_DHC_MapRead(&mapReqPcl, &mapResData);
			if (e_DHC_RESULT_CASH_SUCCESS != cashRet) {
				SC_LOG_ErrorPrint(SC_TAG_DH, "SC_DHC_MapRead error(0x%08x), " HERE, cashRet);
			}
#if 0
			if (e_DHC_RESULT_CASH_SUCCESS != cashRet) {
				SC_LOG_ErrorPrint(SC_TAG_DH, "SC_DHC_MapRead error(0x%08x), " HERE, cashRet);
				(req + i)->bin = (UINT8*) NULL;
			} else {
				(req + i)->bin = (UINT8*) data;
			}
#endif
		}
		// 管理情報更新
		SC_DM_SetFinishCnt(user, i);
	} while (0);

	// セマフォ解除
	if (NULL != apiSem) {
		if (e_SC_RESULT_SUCCESS != SC_UnLockSemaphore(apiSem)) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "[MapMng] SC_LockSemaphore error, " HERE);
		}
	}
	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}

/**
 * @brief	地図開放
 * @param	[I]受信メッセージ構造体のポインタ
 */
E_SC_RESULT SC_DM_MapFree_NoRes(pthread_msq_msg_t *rcvMsg)
{
	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	E_DHC_CASH_RESULT cashRet = e_DHC_RESULT_CASH_SUCCESS;
	T_DHC_REQ_INFO dhcReq = {};
	UINT32 user;

	do {
		user = rcvMsg->data[SC_MSG_REQ_MAP_FREE_USER_ID];
		// ユーザ確認
		if (true != DH_USER_CHECK(user)) {
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		// 管理情報初期化
		ret = SC_DM_InitFinishCnt(user, rcvMsg->data[SC_MSG_REQ_MAP_FREE_REQID]);
		if (e_SC_RESULT_SUCCESS != ret) {
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 要求データ設定
		//DM_SET_DHC_USER_ID(user, dhcReq.user);
		dhcReq.parcelId = 0;
		dhcReq.mapKind = rcvMsg->data[SC_MSG_REQ_MAP_FREE_MAPKIND];

		// 地図開放要求
		switch (rcvMsg->data[SC_MSG_REQ_MAP_FREE_FREETYPE]) {
		case e_SC_DM_FREETYPE_USER:
			//cashRet = SC_DHC_MapFreeEx(dhcReq, SC_DHC_MAPFREE_USER);
			break;
		case e_SC_DM_FREETYPE_KIND:
			//cashRet = SC_DHC_MapFreeEx(dhcReq, SC_DHC_MAPFREE_KIND);
			break;
		default:
			cashRet = e_DHC_RESULT_CASH_FAIL;
			break;
		}
		if (e_DHC_RESULT_CASH_SUCCESS != cashRet) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_DHC_MapFreeEx error(0x%08x), " HERE, cashRet);
		}
	} while (0);
	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}

/**
 * @brief	地図開放
 * @param	[I]受信メッセージ構造体のポインタ
 * @memo	地図開放処理後MSG送信者へMSG応答を行う
 */
E_SC_RESULT SC_DM_MapFree_MsgRes(pthread_msq_msg_t *rcvMsg)
{
	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	pthread_msq_id_t *sender = NULL;
	pthread_msq_msg_t sndMsg = {};
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	E_DHC_CASH_RESULT cashRet = e_DHC_RESULT_CASH_SUCCESS;
	T_DHC_REQ_PARCEL mapReqPcl = {};
	SC_DM_MAPREQ* req;
	UINT32 user;
	UINT32 i;

	// メッセージ送信元取得
	sender = (pthread_msq_id_t*) pthread_msq_msg_issender(rcvMsg);

	do {
		user = rcvMsg->data[SC_MSG_REQ_MAP_FREE_USER_ID];
		req = (SC_DM_MAPREQ*) rcvMsg->data[SC_MSG_REQ_MAP_FREE_TBL];
		// ユーザ確認
		if (true != DH_USER_CHECK(user)) {
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		// パラメタ確認
		if ((NULL == req) || (0 >= rcvMsg->data[SC_MSG_REQ_MAP_FREE_VOL])) {
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		// 管理情報初期化
		ret = SC_DM_InitFinishCnt(user, rcvMsg->data[SC_MSG_REQ_MAP_FREE_REQID]);
		if (e_SC_RESULT_SUCCESS != ret) {
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 要求データ設定
		//DM_SET_DHC_USER_ID(user, dhcReq.user);
		for (i = 0; i < rcvMsg->data[SC_MSG_REQ_MAP_FREE_VOL]; i++) {
			mapReqPcl.parcelNum = 1;
			mapReqPcl.parcelInfo[0].parcelId = (req + i)->pclId;
			mapReqPcl.parcelInfo[0].mapKind = SC_DHC_GetKindMask((req + i)->kind);
			// 開放
			cashRet = SC_DHC_MapFree(&mapReqPcl);
			if (e_DHC_RESULT_CASH_SUCCESS != cashRet) {
				SC_LOG_ErrorPrint(SC_TAG_DH, "SC_DHC_MapFree error(0x%08x), " HERE, cashRet);
			}
		}
	} while (0);

	// 送信メッセージ生成
	if (e_SC_RESULT_SUCCESS == ret) {
		sndMsg.data[SC_MSG_MSG_ID] = e_SC_MSGID_RES_MAP_CASH_FREE;
		sndMsg.data[SC_MSG_RES_MAP_RESTYPE] = e_SC_DM_RESTYPE_FREE;
		sndMsg.data[SC_MSG_RES_MAP_REQID] = rcvMsg->data[SC_MSG_REQ_MAP_READ_REQID];
		sndMsg.data[SC_MSG_RES_MAP_TBL] = rcvMsg->data[SC_MSG_REQ_MAP_READ_TBL];
	} else {
		sndMsg.data[SC_MSG_MSG_ID] = e_SC_MSGID_RES_MAP_CASH_FREE;
		sndMsg.data[SC_MSG_RES_MAP_RESTYPE] = e_SC_DM_RESTYPE_ERROR;
		sndMsg.data[SC_MSG_RES_MAP_REQID] = rcvMsg->data[SC_MSG_REQ_MAP_READ_REQID];
		sndMsg.data[SC_MSG_RES_MAP_TBL] = rcvMsg->data[SC_MSG_REQ_MAP_READ_TBL];
	}
	// メッセージ送信
	SC_DM_MsgSend(sender, &sndMsg, SC_CORE_MSQID_DH);

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}

