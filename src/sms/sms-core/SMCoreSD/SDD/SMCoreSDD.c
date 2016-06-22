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
static Char		startTime[20];
static Char		mapDirPath[SC_MAX_PATH];
static SMSDSTATUS	sdStatus;

//---------------------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------------------

//---------------------------------------------------------------------------------
// 外部関数
//---------------------------------------------------------------------------------
/**
 * @brief 初期化
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SDD_Init()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	const Char* mapPath = NULL;

	SC_LOG_DebugPrint(SC_TAG_SDD, SC_LOG_START);

	// 初期化
	startTime[0] = EOS;
	sdStatus = SMSDSTATUS_STOP;
	mapDirPath[0] = EOS;

	if (SCC_IsLogined) {
		// フォルダパス取得
		mapPath  = SC_MNG_GetMapDirPath();

		// Mapフォルダ
		strcpy(mapDirPath, mapPath);
	} else {
		ret = e_SC_RESULT_FAIL;
	}

	SC_LOG_DebugPrint(SC_TAG_SDD, SC_LOG_END);

	return (ret);
}

/**
 * @brief 終了化
 */
void SC_SDD_Final()
{
	//const Char* mapPath = NULL;

	SC_LOG_DebugPrint(SC_TAG_SDD, SC_LOG_START);

	// 初期化
	startTime[0] = EOS;
	mapDirPath[0] = EOS;
	sdStatus = SMSDSTATUS_STOP;

	SC_LOG_DebugPrint(SC_TAG_SDD, SC_LOG_END);
}

/**
 * @brief	運転特性診断開始
 * @param	[I]受信メッセージ
 */
void SC_SDD_StartReq(const pthread_msq_msg_t *msg)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	pthread_msq_msg_t	sendMsg = {};
	Char	*time = NULL;

	SC_LOG_DebugPrint(SC_TAG_SDD, SC_LOG_START);

	// 診断開始時刻取得
	time = (Char*)msg->data[SC_MSG_REQ_SD_STARTTIME];
	if (NULL != time) {
		strcpy((char*)startTime, (char*)time);
		// ロケータ(センサデータ取得)初期化
		if (!SC_SensorData_Initialize()) {
			// 失敗
			ret = e_SC_RESULT_FAIL;
		} else {
			// 診断中
			sdStatus = SMSDSTATUS_START;
		}
	} else {
		ret = e_SC_RESULT_FAIL;
	}

	// 運転特性診断開始応答メッセージ生成
	sendMsg.data[SC_MSG_MSG_ID] = e_SC_MSGID_RES_SDD_START;
	sendMsg.data[SC_MSG_RES_SD_RESULT] = ret;

	// 送信メッセージをログ出力
	SC_LOG_DebugPrint(SC_TAG_SDD,
			"sendMsg=0x%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x, " HERE,
			sendMsg.data[0],  sendMsg.data[1],  sendMsg.data[2],  sendMsg.data[3],  sendMsg.data[4],
			sendMsg.data[5],  sendMsg.data[6],  sendMsg.data[7],  sendMsg.data[8],  sendMsg.data[9]);

	// 運転特性診断メインスレッドに処理結果の応答メッセージ送信
	if (PTHREAD_MSQ_OK != pthread_msq_msg_send(SC_CORE_MSQID_SDM, &sendMsg, SC_CORE_MSQID_SDD)) {
		SC_LOG_ErrorPrint(SC_TAG_SDD, "pthread_msq_msg_send error, " HERE);
		// メッセージの送信に失敗したら関数コール
		SC_SDM_StartRes(&sendMsg);
	}

	SC_LOG_DebugPrint(SC_TAG_SDD, SC_LOG_END);
}

/**
 * @brief	運転特性診断停止
 * @param	[I]受信メッセージ
 */
void SC_SDD_StopReq(const pthread_msq_msg_t *msg)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	pthread_msq_msg_t	sendMsg = {};

	SC_LOG_DebugPrint(SC_TAG_SDD, SC_LOG_START);

	if (SMSDSTATUS_START == sdStatus) {
		// ロケータ(センサデータ取得)終了化
		if (!SC_SensorData_Finalize()) {
			// 失敗
			ret = e_SC_RESULT_FAIL;
		}
		// 診断停止
		sdStatus = SMSDSTATUS_STOP;
	}

	// 運転特性診断終了応答メッセージ生成
	sendMsg.data[SC_MSG_MSG_ID] = e_SC_MSGID_RES_SDD_STOP;
	sendMsg.data[SC_MSG_RES_SD_RESULT] = ret;

	// 送信メッセージをログ出力
	SC_LOG_DebugPrint(SC_TAG_SDD,
			"sendMsg=0x%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x, " HERE,
			sendMsg.data[0],  sendMsg.data[1],  sendMsg.data[2],  sendMsg.data[3],  sendMsg.data[4],
			sendMsg.data[5],  sendMsg.data[6],  sendMsg.data[7],  sendMsg.data[8],  sendMsg.data[9]);

	// 運転特性診断メインスレッドに処理結果の応答メッセージ送信
	if (PTHREAD_MSQ_OK != pthread_msq_msg_send(SC_CORE_MSQID_SDM, &sendMsg, SC_CORE_MSQID_SDD)) {
		SC_LOG_ErrorPrint(SC_TAG_SDD, "pthread_msq_msg_send error, " HERE);
		// メッセージの送信に失敗したら関数コール
		SC_SDM_StopRes(&sendMsg);
	}

	SC_LOG_DebugPrint(SC_TAG_SDD, SC_LOG_END);
}

/**
 * @brief	センサデータ受信
 * @param	[I]受信メッセージ
 */
void SC_SDD_SensorDataRecv(const pthread_msq_msg_t *msg)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	//pthread_msq_msg_t	sendMsg = {};
	SMPHYDDATA	*smPhydData = NULL;
	INT32		num = 0;
	//INT32		i = 0;
	//DBOBJECT	*db = NULL;

	SC_LOG_DebugPrint(SC_TAG_SDD, SC_LOG_START);

	smPhydData = (SMPHYDDATA*)msg->data[SC_MSG_EVT_SD_DATA];
	num = msg->data[SC_MSG_EVT_SD_DATA_NUM];

	// 診断中
	if (SMSDSTATUS_START == sdStatus) {
#if 0	// デバッグ用
		SC_LOG_DebugPrint(SC_TAG_SDD, "sensorData num=%d, " HERE, num);
		for (i = 0; i < num; i++) {
			SC_LOG_DebugPrint(SC_TAG_SDD, "sensorData[%d], " HERE, i);
			SC_LOG_DebugPrint(SC_TAG_SDD, "time=%s, " HERE, smPhydData[i].time);
			SC_LOG_DebugPrint(SC_TAG_SDD, "latNetwork=%f, " HERE, smPhydData[i].latNetwork);
			SC_LOG_DebugPrint(SC_TAG_SDD, "lonNetwork=%f, " HERE, smPhydData[i].lonNetwork);
			SC_LOG_DebugPrint(SC_TAG_SDD, "latGPS=%f, " HERE, smPhydData[i].latGPS);
			SC_LOG_DebugPrint(SC_TAG_SDD, "lonGPS=%f, " HERE, smPhydData[i].lonGPS);
			SC_LOG_DebugPrint(SC_TAG_SDD, "acc[0]=%f, " HERE, smPhydData[i].acc[0]);
			SC_LOG_DebugPrint(SC_TAG_SDD, "acc[1]=%f, " HERE, smPhydData[i].acc[1]);
			SC_LOG_DebugPrint(SC_TAG_SDD, "acc[2]=%f, " HERE, smPhydData[i].acc[2]);
			SC_LOG_DebugPrint(SC_TAG_SDD, "orientation[0]=%f, " HERE, smPhydData[i].orientation[0]);
			SC_LOG_DebugPrint(SC_TAG_SDD, "orientation[1]=%f, " HERE, smPhydData[i].orientation[1]);
			SC_LOG_DebugPrint(SC_TAG_SDD, "orientation[2]=%f, " HERE, smPhydData[i].orientation[2]);
			SC_LOG_DebugPrint(SC_TAG_SDD, "magneticField[0]=%f, " HERE, smPhydData[i].magneticField[0]);
			SC_LOG_DebugPrint(SC_TAG_SDD, "magneticField[1]=%f, " HERE, smPhydData[i].magneticField[1]);
			SC_LOG_DebugPrint(SC_TAG_SDD, "magneticField[2]=%f, " HERE, smPhydData[i].magneticField[2]);
			SC_LOG_DebugPrint(SC_TAG_SDD, "gyroscope[0]=%f, " HERE, smPhydData[i].gyroscope[0]);
			SC_LOG_DebugPrint(SC_TAG_SDD, "gyroscope[1]=%f, " HERE, smPhydData[i].gyroscope[1]);
			SC_LOG_DebugPrint(SC_TAG_SDD, "gyroscope[2]=%f, " HERE, smPhydData[i].gyroscope[2]);
			SC_LOG_DebugPrint(SC_TAG_SDD, "gravity[0]=%f, " HERE, smPhydData[i].gravity[0]);
			SC_LOG_DebugPrint(SC_TAG_SDD, "gravity[1]=%f, " HERE, smPhydData[i].gravity[1]);
			SC_LOG_DebugPrint(SC_TAG_SDD, "gravity[2]=%f, " HERE, smPhydData[i].gravity[2]);
		}
#endif	// デバッグ用

		if (CC_SD_UPLOAD_ERROR_MAXNUM > CC_SD_GET_UPLOADERRORNUM()) {
			// DBに追加する
			ret = SC_SDDAL_AddPhydSensorData(startTime, smPhydData, num, true);
			if (e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_ErrorPrint(SC_TAG_SDD, "SC_SDDAL_AddPhydSensorData error, " HERE);
			}
		}
	}

	if (NULL != smPhydData) {
		free(smPhydData);
		smPhydData = NULL;
	}

	SC_LOG_DebugPrint(SC_TAG_SDD, SC_LOG_END);
}
