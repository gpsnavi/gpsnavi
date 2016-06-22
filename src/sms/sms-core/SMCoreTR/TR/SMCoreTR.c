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
 * SMCoreTR.c
 *
 *  Created on: 2016/01/04
 *      Author: n.kanagawa
 */
#include "sms-core/SMCoreTR/SMCoreTRInternal.h"


// プロトタイプ
static void checkCacheParcel(const TR_PARCEL_LIST_t *pList, TR_PARCEL_LIST_t *pYes, TR_PARCEL_LIST_t *pNo);
static void redrawReq();


/**
 * @brief	自車位置更新時メッセージ送信
 * 			※ロケータの位置変化コールバックからのコールを想定
 */
void SC_TR_SendCarposUpdateMsg()
{
	//E_SC_RESULT			ret = e_SC_RESULT_SUCCESS;
	pthread_msq_msg_t	msg = {};

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_START);

	do {
		// ログインチェック
		if (!SCC_IsLogined()) {
			SC_LOG_ErrorPrint(SC_TAG_TR, (Char*)"SCC_IsLogined, no login, " HERE);
			break;
		}

		msg.data[SC_MSG_MSG_ID] = e_SC_MSGID_REQ_TR_UPDATE;
		msg.data[SC_MSG_REQ_TR_EVT] = e_SC_TR_EVENT_CARPOS;

		// メッセージ送信
		if (PTHREAD_MSQ_OK != pthread_msq_msg_send(SC_CORE_MSQID_TR, &msg, SC_CORE_MSQID_TR)) {
			SC_LOG_ErrorPrint(SC_TAG_TR, (Char*)"pthread_msq_msg_send error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_END);
}

/**
 * @brief	スクロール位置更新時メッセージ送信
 * 			※HMI(JNI)からのコールを想定
 */
void SC_TR_SendScrollUpdateMsg()
{
	//E_SC_RESULT			ret = e_SC_RESULT_SUCCESS;
	pthread_msq_msg_t	msg = {};

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_START);

	do {
		// ログインチェック
		if (!SCC_IsLogined()) {
			SC_LOG_ErrorPrint(SC_TAG_TR, (Char*)"SCC_IsLogined, no login, " HERE);
			break;
		}

		msg.data[SC_MSG_MSG_ID] = e_SC_MSGID_REQ_TR_UPDATE;
		msg.data[SC_MSG_REQ_TR_EVT] = e_SC_TR_EVENT_SCROLL;

		// メッセージ送信
		if (PTHREAD_MSQ_OK != pthread_msq_msg_send(SC_CORE_MSQID_TR, &msg, SC_CORE_MSQID_TR)) {
			SC_LOG_ErrorPrint(SC_TAG_TR, (Char*)"pthread_msq_msg_send error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_END);
}

/**
 * @brief	自車位置手動更新時メッセージ送信
 * 			※HMI(JNI)からのコールを想定
 */
/*void SC_TR_SendManualUpdateMsg()
{
	E_SC_RESULT			ret = e_SC_RESULT_SUCCESS;
	pthread_msq_msg_t	msg = {};

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_START);

	do {
		// ログインチェック
		if (!SCC_IsLogined()) {
			SC_LOG_ErrorPrint(SC_TAG_TR, (Char*)"SCC_IsLogined, no login, " HERE);
			break;
		}

		msg.data[SC_MSG_MSG_ID] = e_SC_MSGID_REQ_TR_UPDATE;
		msg.data[SC_MSG_REQ_TR_EVT] = e_SC_TR_EVENT_MANUAL;

		// メッセージ送信
		if (PTHREAD_MSQ_OK != pthread_msq_msg_send(SC_CORE_MSQID_TR, &msg, SC_CORE_MSQID_TR)) {
			SC_LOG_ErrorPrint(SC_TAG_TR, (Char*)"pthread_msq_msg_send error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_END);
}*/

/**
 * @brief	自車位置タイマー更新時メッセージ送信
 * 			※タイマスレッドからのコールを想定
 */
/*void SC_TR_SendTimerUpdateMsg()
{
	E_SC_RESULT			ret = e_SC_RESULT_SUCCESS;
	pthread_msq_msg_t	msg = {};

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_START);

	do {
		// ログインチェック
		if (!SCC_IsLogined()) {
			SC_LOG_ErrorPrint(SC_TAG_TR, (Char*)"SCC_IsLogined, no login, " HERE);
			break;
		}

		msg.data[SC_MSG_MSG_ID] = e_SC_MSGID_REQ_TR_UPDATE;
		msg.data[SC_MSG_REQ_TR_EVT] = e_SC_TR_EVENT_TIMER;
		msg.data[SC_MSG_REQ_TR_TIMER] = TR_UPDATE_TIMER;

		// メッセージ送信
		if (PTHREAD_MSQ_OK != pthread_msq_msg_send(SC_CORE_MSQID_TR, &msg, SC_CORE_MSQID_TR)) {
			SC_LOG_ErrorPrint(SC_TAG_TR, (Char*)"pthread_msq_msg_send error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_END);
}*/

/**
 * @brief	キャッシュデータ差分更新
 * @param	[I]user			ユーザー
 * @param	[I]parcelId		パーセルID
 */
void SC_TR_CacheDiffUpdate(const UINT16 user, UINT32 parcelId)
{
	E_SC_RESULT			ret = e_SC_RESULT_SUCCESS;
	INT32				i = 0;
	TR_PARCEL_LIST_t	parcelList = {};
	TR_PARCEL_LIST_t	noCache = {};
	TR_PARCEL_LIST_t	yesCache = {};
	TR_TRAFFIC_LIST_t	trafficList = {};

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_START);

	do {
		// カレントパーセルID取得 ※キューにたまっている古い可能性のあるパーセルを処理しない為、ここでパーセルIDを再取得
		parcelId = SC_TR_GetCurrentPosParcelID(user);

		// 前回の要求と同じパーセルIDの場合処理しない
		if (parcelId == SC_TR_GetMainParcelID(user)) {
			break;
		}

		// 周辺パーセル取得
		SC_TR_GetAreaParcelList(parcelId, &parcelList);

		// 周辺パーセルリストでキャッシュをチェック
		checkCacheParcel(&parcelList, &yesCache, &noCache);
		if (0 == noCache.cnt) {
			break;
		}

		// 交通情報取得
		ret = SC_TR_ComTraffic(&noCache, &trafficList);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_TR, (Char*)"SC_TR_ComTraffic error, " HERE);
			break;
		}

		// 排他開始
		if (!SC_TR_LockCacheTbl()) {
			SC_LOG_ErrorPrint(SC_TAG_TR, (Char*)"SC_TR_CacheLock error, " HERE);
			break;
		}

		// キャッシュから不要データを削除
		SC_TR_ReleaseUser(user);
		for (i=0; i<yesCache.cnt; i++) {
			SC_TR_SetUser(user, yesCache.pcl[i], TR_ON);
		}
		SC_TR_DeleteNoUserData();

		// データ設定
		for (i=0; i<trafficList.cnt; i++) {
			if (!SC_TR_ReplaceData(user, trafficList.trf[i].pid, &trafficList.trf[i].data)) {
				SC_LOG_ErrorPrint(SC_TAG_TR, (Char*)"SC_TR_ReplaceData error, " HERE);
			}
		}

		// メインパーセル更新
		SC_TR_SetMainParcelID(user, parcelId);

		// 排他終了
		if (!SC_TR_UnLockCacheTbl()) {
			SC_LOG_ErrorPrint(SC_TAG_TR, (Char*)"SC_TR_CacheUnLock error, " HERE);
			break;
		}
	} while (0);

	// キャッシュ登録できなかったデータの解放
	for (i=0; i<trafficList.cnt; i++) {
		if (NULL != trafficList.trf[i].data.pData) {
			SC_TR_Free(trafficList.trf[i].data.pData);
		}
	}

	// 再描画要求
	redrawReq();

#ifdef _TR_DEBUG
	SC_TR_CacheDebugPrint();
#endif

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_END);
}

/**
 * @brief	指定ユーザーキャッシュデータ全更新
 * 			※キャッシュデータが存在しない場合は、再取得
 * @param	[I]user			ユーザー
 */
void SC_TR_CacheAllUpdate(const UINT16 user)
{
	E_SC_RESULT			ret = e_SC_RESULT_SUCCESS;
	INT32				i = 0;
	UINT32				parcelId = 0;
	TR_PARCEL_LIST_t	parcelList = {};
	TR_TRAFFIC_LIST_t	trafficList = {};

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_START);

	do {
		// カレントパーセルID取得
		parcelId = SC_TR_GetCurrentPosParcelID(user);
		// 周辺パーセル取得
		SC_TR_GetAreaParcelList(parcelId, &parcelList);
		if (0 == parcelList.cnt) {
			break;
		}

		// センター通信
		ret = SC_TR_ComTraffic(&parcelList, &trafficList);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_TR, (Char*)"SC_TR_ComTraffic error, " HERE);
			break;
		}

		// 排他開始
		if (!SC_TR_LockCacheTbl()) {
			SC_LOG_ErrorPrint(SC_TAG_TR, (Char*)"SC_TR_CacheLock error, " HERE);
			break;
		}

		// キャッシュから不要データを削除
		SC_TR_ReleaseUser(user);
		for (i=0; i<trafficList.cnt; i++) {
			SC_TR_SetUser(user, trafficList.trf[i].pid, TR_ON);
		}
		SC_TR_DeleteNoUserData();

		// センターから取得したデータを設定
		for (i=0; i<trafficList.cnt; i++) {
			if (!SC_TR_ReplaceData(user, trafficList.trf[i].pid, &trafficList.trf[i].data)) {
				SC_LOG_ErrorPrint(SC_TAG_TR, (Char*)"SC_TR_ReplaceData error, " HERE);
			}
		}

		// メインパーセル更新
		SC_TR_SetMainParcelID(user, parcelId);

		// 排他終了
		if (!SC_TR_UnLockCacheTbl()) {
			SC_LOG_ErrorPrint(SC_TAG_TR, (Char*)"SC_TR_CacheUnLock error, " HERE);
			break;
		}

	} while (0);

	// キャッシュ登録できなかったデータの解放
	for (i=0; i<trafficList.cnt; i++) {
		if (NULL != trafficList.trf[i].data.pData) {
			SC_TR_Free(trafficList.trf[i].data.pData);
		}
	}

	// 再描画要求
	redrawReq();

#ifdef _TR_DEBUG
	SC_TR_CacheDebugPrint();
#endif

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_END);
}

/**
 * @brief	キャッシュチェック
 * @param	[I]pList	パーセルリスト
 * @param	[O]pYes		キャッシュに存在するパーセルリスト
 * @param	[O]pNo		キャッシュに存在しないパーセルリスト
 */
static void checkCacheParcel(const TR_PARCEL_LIST_t *pList, TR_PARCEL_LIST_t *pYes, TR_PARCEL_LIST_t *pNo)
{
	INT32		i = 0;

	pYes->cnt = 0;
	pNo->cnt = 0;

	// 周辺パーセルリストをキャッシュ有無で振り分け
	for (i=0; i<pList->cnt; i++) {
		if (SC_TR_IsCache(pList->pcl[i])) {
			// 有
			pYes->pcl[pYes->cnt] = pList->pcl[i];
			pYes->cnt++;
		} else {
			// 無
			pNo->pcl[pNo->cnt] = pList->pcl[i];
			pNo->cnt++;
		}
	}
}

/**
 * @brief	再描画要求
 */
static void redrawReq()
{
	// HMIへ再描画要求
	LC_LocationUpdateCallback();
}
