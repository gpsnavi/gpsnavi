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

//---------------------------------------------------------------------------------
// 変数定義
//---------------------------------------------------------------------------------
static Char	mapFilePath[SC_MAX_PATH];
static Char	startTime[20];
static Bool	isStoped[2];
static SMSDSTATUS	sdStatus;

//---------------------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------------------

//---------------------------------------------------------------------------------
// 外部関数
//---------------------------------------------------------------------------------
/**
 * @brief 初期化
 * @param	[in]mapFilePath  地図ファイルパス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SDM_Init(const Char *mapPath)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	Bool		exist = false;
	INT32		num = 0;
	Char		*str = NULL;

	SC_LOG_DebugPrint(SC_TAG_SDM, SC_LOG_START);

	// 初期化
	startTime[0] = EOS;
	memset(isStoped, false, sizeof(isStoped));
	sdStatus = SMSDSTATUS_STOP;
	CC_SD_CLEAR_UPLOADERRORNUM();

	do {
		if (!SCC_IsLogined) {
			break;
		}

		// DBファイルパス作成
		strcpy(mapFilePath, mapPath);
		str = strstr(mapFilePath, SC_DB_PATH);
		if (NULL != str) {
			*str = EOS;
		}
		if ('/' == mapFilePath[strlen(mapFilePath) - 1]) {
			sprintf(&mapFilePath[strlen(mapFilePath)], "%s", SC_SD_DB_FILE_NAME);
		} else {
			sprintf(&mapFilePath[strlen(mapFilePath)], "/%s", SC_SD_DB_FILE_NAME);
		}

		// DB初期化（DBオープン）
		ret = SC_SDDAL_Initialize(mapFilePath);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_SDM, "db open error[%s], " HERE, mapFilePath);
			break;
		}

		// テーブル有無チェック
		ret = SC_SDDAL_CheckTabeleExist(SC_SDDAL_TABLENAME_PHYD_SENSOR_DATA, &exist);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_SDM, "table exist check error, " HERE);
			break;
		}
		if (exist) {
			// テーブルが存在する
			// データ件数を取得する
			ret = SC_SDDAL_GetPhydSensorDataCount(&num);
			if (e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_ErrorPrint(SC_TAG_SDM, "select count error, " HERE);
				break;
			}
			if (0 < num) {
				// データを削除する
				ret = SC_SDDAL_DelPhydSensorData(NULL, 0, true);
				if (e_SC_RESULT_SUCCESS != ret) {
					SC_LOG_ErrorPrint(SC_TAG_SDM, "delete data error, " HERE);
					break;
				}
			}
		} else {
			// テーブルが存在しない
			// テーブルを作成する
			ret = SC_SDDAL_CreateTabelePhydSensorData(true);
			if (e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_ErrorPrint(SC_TAG_SDM, "create table error, " HERE);
				break;
			}
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_SDM, SC_LOG_END);

	return (ret);
}

/**
 * @brief 終了化
 */
void SC_SDM_Final()
{
	SC_LOG_DebugPrint(SC_TAG_SDM, SC_LOG_START);

	// DB終了化（DBクローズ）
	SC_SensorData_Finalize();

	// 初期化
	mapFilePath[0] = EOS;
	startTime[0] = EOS;
	memset(isStoped, false, sizeof(isStoped));
	sdStatus = SMSDSTATUS_STOP;
	CC_SD_CLEAR_UPLOADERRORNUM();

	SC_LOG_DebugPrint(SC_TAG_SDM, SC_LOG_END);
}

/**
 * @brief 運転特性診断開始
 * @param [in]msg   受信メッセージ
 */
void SC_SDM_StartReq(const pthread_msq_msg_t *msg)
{
	pthread_msq_msg_t	sendMsg = {};
	struct timeval	tv = {};
	struct tm	*utcTm = NULL;

	SC_LOG_DebugPrint(SC_TAG_SDM, SC_LOG_START);

	// 初期化
	memset(isStoped, false, sizeof(isStoped));

	// 診断中
	sdStatus = SMSDSTATUS_START;

	// 現在時刻取得
	gettimeofday(&tv, NULL);
	// UTCに変換
	utcTm = gmtime((time_t*)&tv.tv_sec);
	// YYYYMMDDhhmmssSSS形式
	sprintf((char*)startTime,
			"%04d%02d%02d%02d%02d%02d%03d",
			(utcTm->tm_year + 1900),
			(utcTm->tm_mon + 1),
			utcTm->tm_mday,
			utcTm->tm_hour,
			utcTm->tm_min,
			utcTm->tm_sec,
			(int)(tv.tv_usec / 1000));

	// 運転特性診断開始要求メッセージ生成
	sendMsg.data[SC_MSG_MSG_ID] = e_SC_MSGID_REQ_SDD_START;
	sendMsg.data[SC_MSG_REQ_SD_STARTTIME] = (uintptr_t)startTime;

	// 送信メッセージをログ出力
	SC_LOG_DebugPrint(SC_TAG_SDM,
			"sendMsg=0x%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x, " HERE,
			sendMsg.data[0],  sendMsg.data[1],  sendMsg.data[2],  sendMsg.data[3],  sendMsg.data[4],
			sendMsg.data[5],  sendMsg.data[6],  sendMsg.data[7],  sendMsg.data[8],  sendMsg.data[9]);

	// 運転特性診断開始要求メッセージ送信
	if (PTHREAD_MSQ_OK != pthread_msq_msg_send(SC_CORE_MSQID_SDD, &sendMsg, SC_CORE_MSQID_SDM)) {
		SC_LOG_ErrorPrint(SC_TAG_SDM, "pthread_msq_msg_send error, " HERE);
		SC_SDD_StartReq(&sendMsg);
	}

	// タイマ開始要求メッセージ生成
	sendMsg.data[SC_MSG_MSG_ID] = e_SC_MSGID_REQ_SDT_START;
	sendMsg.data[SC_MSG_REQ_SD_STARTTIME] = (INT32)CC_SD_UPLOAD_TIMER;

	// 送信メッセージをログ出力
	SC_LOG_DebugPrint(SC_TAG_SDM,
			"sendMsg=0x%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x, " HERE,
			sendMsg.data[0],  sendMsg.data[1],  sendMsg.data[2],  sendMsg.data[3],  sendMsg.data[4],
			sendMsg.data[5],  sendMsg.data[6],  sendMsg.data[7],  sendMsg.data[8],  sendMsg.data[9]);

	// タイマ開始要求メッセージ送信
	if (PTHREAD_MSQ_OK != pthread_msq_msg_send(SC_CORE_MSQID_SDT, &sendMsg, SC_CORE_MSQID_SDM)) {
		SC_LOG_ErrorPrint(SC_TAG_SDM, "pthread_msq_msg_send error, " HERE);
		SC_SDT_StartTimer(&sendMsg);
	}

	// 運転特性診断開始
	SC_SDU_StartReq(&sendMsg);

	SC_LOG_DebugPrint(SC_TAG_SDM, SC_LOG_END);
}

/**
 * @brief 運転特性診断開始応答
 * @param [in]msg   受信メッセージ
 */
void SC_SDM_StartRes(const pthread_msq_msg_t *msg)
{
	pthread_msq_msg_t	sendMsg = {};
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	Char	*tripId = NULL;

	SC_LOG_DebugPrint(SC_TAG_SDM, SC_LOG_START);

	// 運転特性診断開始応答メッセージ生成
	sendMsg.data[SC_MSG_MSG_ID] = e_SC_MSGID_RES_SDM_START;
	ret = msg->data[SC_MSG_RES_SD_RESULT];
	if (e_SC_RESULT_SUCCESS == ret) {
		// メモリ確保
		tripId = (Char*)SC_MEM_Alloc(SC_SD_TRIP_ID_SIZE, e_MEM_TYPE_SD);
		if (NULL == tripId) {
			SC_LOG_ErrorPrint(SC_TAG_SDU, (Char*)"SC_MEM_Alloc error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
		} else {
			// トリップID
			sprintf((char*)tripId, "%.8s-%.6s", startTime, &startTime[8]);
		}
	}
	sendMsg.data[SC_MSG_RES_SD_RESULT] = ret;
	sendMsg.data[SC_MSG_RES_SD_TRIP_ID] = (uintptr_t)tripId;

	// 送信メッセージをログ出力
	SC_LOG_DebugPrint(SC_TAG_SDM,
			"sendMsg=0x%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x, " HERE,
			sendMsg.data[0],  sendMsg.data[1],  sendMsg.data[2],  sendMsg.data[3],  sendMsg.data[4],
			sendMsg.data[5],  sendMsg.data[6],  sendMsg.data[7],  sendMsg.data[8],  sendMsg.data[9]);

	// FMに処理結果の応答メッセージ送信
	if (PTHREAD_MSQ_OK != pthread_msq_msg_send(SC_CORE_MSQID_FM, &sendMsg, SC_CORE_MSQID_SDM)) {
		SC_LOG_ErrorPrint(SC_TAG_SDM, "pthread_msq_msg_send error, " HERE);
		// メッセージの送信に失敗したら関数コール
		SC_FM_ResStartDrive(&sendMsg);
	}

	SC_LOG_DebugPrint(SC_TAG_SDM, SC_LOG_END);
}

/**
 * @brief 運転特性診断停止
 * @param [in]msg   受信メッセージ
 */
void SC_SDM_StopReq(const pthread_msq_msg_t *msg)
{
	pthread_msq_msg_t	sendMsg = {};

	SC_LOG_DebugPrint(SC_TAG_SDM, SC_LOG_START);

	// 診断停止
	sdStatus = SMSDSTATUS_STOP;

	// 運転特性診断停止要求メッセージ生成
	sendMsg.data[SC_MSG_MSG_ID] = e_SC_MSGID_REQ_SDD_STOP;

	// 送信メッセージをログ出力
	SC_LOG_DebugPrint(SC_TAG_SDM,
			"sendMsg=0x%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x, " HERE,
			sendMsg.data[0],  sendMsg.data[1],  sendMsg.data[2],  sendMsg.data[3],  sendMsg.data[4],
			sendMsg.data[5],  sendMsg.data[6],  sendMsg.data[7],  sendMsg.data[8],  sendMsg.data[9]);

	// 運転特性診断停止要求メッセージ送信
	if (PTHREAD_MSQ_OK != pthread_msq_msg_send(SC_CORE_MSQID_SDD, &sendMsg, SC_CORE_MSQID_SDM)) {
		SC_LOG_ErrorPrint(SC_TAG_SDM, "pthread_msq_msg_send error, " HERE);
		SC_SDD_StopReq(&sendMsg);
	}

	// タイマ停止
	SC_SDT_StopTimer();

	// アップロード終了要求メッセージ生成
	sendMsg.data[SC_MSG_MSG_ID] = e_SC_MSGID_REQ_SDU_STOP;
	sendMsg.data[SC_MSG_REQ_SD_STARTTIME] = (INT32)startTime;

	// 送信メッセージをログ出力
	SC_LOG_DebugPrint(SC_TAG_SDM,
			"sendMsg=0x%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x, " HERE,
			sendMsg.data[0],  sendMsg.data[1],  sendMsg.data[2],  sendMsg.data[3],  sendMsg.data[4],
			sendMsg.data[5],  sendMsg.data[6],  sendMsg.data[7],  sendMsg.data[8],  sendMsg.data[9]);

	// アップロード終了要求メッセージ送信
	if (PTHREAD_MSQ_OK != pthread_msq_msg_send(SC_CORE_MSQID_SDU, &sendMsg, SC_CORE_MSQID_SDM)) {
		SC_LOG_ErrorPrint(SC_TAG_SDM, "pthread_msq_msg_send error, " HERE);
		SC_SDU_UploadStopReq(&sendMsg);
	}

	// 運転特性診断停止
	SC_SDU_StopReq(&sendMsg);

	SC_LOG_DebugPrint(SC_TAG_SDM, SC_LOG_END);
}

/**
 * @brief 運転特性診断停止応答
 * @param [in]msg   受信メッセージ
 */
void SC_SDM_StopRes(const pthread_msq_msg_t *msg)
{
	pthread_msq_msg_t	sendMsg = {};

	SC_LOG_DebugPrint(SC_TAG_SDM, SC_LOG_START);

	if (e_SC_MSGID_RES_SDD_STOP == msg->data[SC_MSG_MSG_ID]) {
		isStoped[0] = true;
	} else if (e_SC_MSGID_RES_SDU_STOP == msg->data[SC_MSG_MSG_ID]) {
		isStoped[1] = true;
	}

	if ((isStoped[0]) && (isStoped[1])) {
		// 運転特性診断停止応答メッセージ生成
		sendMsg.data[SC_MSG_MSG_ID] = e_SC_MSGID_RES_SDM_STOP;
		sendMsg.data[SC_MSG_RES_SD_RESULT] = msg->data[SC_MSG_RES_SD_RESULT];
		sendMsg.data[SC_MSG_RES_SD_TRIP_ID] = msg->data[SC_MSG_RES_SD_TRIP_ID];

		// 送信メッセージをログ出力
		SC_LOG_DebugPrint(SC_TAG_SDM,
				"sendMsg=0x%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x, " HERE,
				sendMsg.data[0],  sendMsg.data[1],  sendMsg.data[2],  sendMsg.data[3],  sendMsg.data[4],
				sendMsg.data[5],  sendMsg.data[6],  sendMsg.data[7],  sendMsg.data[8],  sendMsg.data[9]);

		// FMに処理結果の応答メッセージ送信
		if (PTHREAD_MSQ_OK != pthread_msq_msg_send(SC_CORE_MSQID_FM, &sendMsg, SC_CORE_MSQID_SDM)) {
			SC_LOG_ErrorPrint(SC_TAG_SDM, "pthread_msq_msg_send error, " HERE);
			// メッセージの送信に失敗したら関数コール
			SC_FM_ResStartDrive(&sendMsg);
		}
		memset(isStoped, false, sizeof(isStoped));
	}

	SC_LOG_DebugPrint(SC_TAG_SDM, SC_LOG_END);
}

/**
 * @brief アップロード要求
 * @param [in]msg   受信メッセージ
 */
void SC_SDM_UploadReq(const pthread_msq_msg_t *msg)
{
	pthread_msq_msg_t	sendMsg = {};

	SC_LOG_DebugPrint(SC_TAG_SDM, SC_LOG_START);

	// タイムアウト通知応答
	SC_SDT_TimeoutRes();

	// 診断中
	if (SMSDSTATUS_START == sdStatus) {
		// アップロード要求メッセージ生成
		sendMsg.data[SC_MSG_MSG_ID] = e_SC_MSGID_REQ_SDU_UPLOAD;
		sendMsg.data[SC_MSG_REQ_SD_STARTTIME] = (INT32)startTime;

		// 送信メッセージをログ出力
		SC_LOG_DebugPrint(SC_TAG_SDM,
				"sendMsg=0x%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x, " HERE,
				sendMsg.data[0],  sendMsg.data[1],  sendMsg.data[2],  sendMsg.data[3],  sendMsg.data[4],
				sendMsg.data[5],  sendMsg.data[6],  sendMsg.data[7],  sendMsg.data[8],  sendMsg.data[9]);

		// 運転特性診断アップロードスレッドにアップロード要求メッセージ送信
		if (PTHREAD_MSQ_OK != pthread_msq_msg_send(SC_CORE_MSQID_SDU, &sendMsg, SC_CORE_MSQID_SDM)) {
			SC_LOG_ErrorPrint(SC_TAG_SDM, "pthread_msq_msg_send error, " HERE);
			// メッセージの送信に失敗したら関数コール
			SC_SDU_UploadReq(&sendMsg);
		}
	}

	SC_LOG_DebugPrint(SC_TAG_SDM, SC_LOG_END);
}
