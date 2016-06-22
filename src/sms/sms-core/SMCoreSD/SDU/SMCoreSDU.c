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

#define SC_SDU_PHYDDATA_MAXNUM		2000
#define SC_SDU_JSON_FILE_NAME		"phyd.json"
#define SC_SDU_LOG_FILE_PATH		"/Log/SensorProbe/SMCAL_sensorprobe.log"	// (デバッグ用)ログファイルのパス
#define SC_SDU_URI_MAX_LEN			(1024 + 1)
#define SC_SDU_RECIVE_BUFFER_SIZE	(1024 * 1024 * 1)
#define SC_SDU_AWS_DIRNAME_PROBE	"sensorprobe/"
#define SC_SDU_AWS_KEY				"\"key\""
#define SC_SDU_AWS_KEY_PROBE		"sensorprobe/${filename}"
#define SC_SDU_AWS_ACCESS_KEY_ID	"\"AWSAccessKeyId\""
#define SC_SDU_AWS_ACL				"\"acl\""
#define SC_SDU_AWS_ACL_PARIVATE		"private"
#define SC_SDU_AWS_POLICY			"\"policy\""
#define SC_SDU_AWS_SIGNATURE		"\"signature\""
#define SC_SDU_CONTENT_TYPE			"\"Content-Type\""
#define SC_SDU_CONTENT_TYPE_BIN		"application/binary"
#define SC_SDU_FILE					"\"file\"; filename=\"%s\"\r\nContent-Type: application/gzip"
#define SC_SDU_AWS_KEY_PROBE_LEN	23
#define SC_SDU_AWS_ACCESS_KEY_LEN	20
#define SC_SDU_AWS_ACL_PARIVATE_LEN	7
#define SC_SDU_CONTENT_TYPE_BIN_LEN	18
#define SC_SDU_FILE_LEN				51
#define SC_SDU_URI_STR_MAX_LEN		(1024+1)
#define SC_SDU_CAL_TIMEOUT			30

//#define SC_SDU_UPLOAD_LOG_ENABLE		// アップロードログ出力する

static SMSDSTATUS	sdStatus;
static Char		jsonFilePath[SC_MAX_PATH];
static Char		uploadFilePath[SC_MAX_PATH];
static SCC_AUTHINFO authInfo;
static SMCAL	smcal;
static SMCALOPT	smcalOpt;
static Char		*recvBuf;

static E_SC_RESULT SC_SDU_Upload(const Char *startTime, Bool isTerminal);

/**
 * @brief 初期化処理
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SDU_Init()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_CAL_RESULT	calRet = e_SC_CAL_RESULT_SUCCESS;
	const Char* dataPath = NULL;
	Char	*filePath = NULL;

	SC_LOG_DebugPrint(SC_TAG_SDU, SC_LOG_START);

	// 初期化
	sdStatus = SMSDSTATUS_STOP;
	memset(&smcal, 0, sizeof(SMCAL));
	memset(&smcalOpt, 0, sizeof(SMCALOPT));
	smcalOpt.isAws = true;
	smcalOpt.timeout = SC_SDU_CAL_TIMEOUT;
	recvBuf = NULL;
	memset(&authInfo, 0, sizeof(SCC_AUTHINFO));
	jsonFilePath[0] = EOS;
	uploadFilePath[0] = EOS;

	do {
		if (!SCC_IsLogined()) {
			break;
		}

		// フォルダパス取得
		dataPath = SC_MNG_GetApRootDirPath();

		// jsonファイル
		sprintf((char*)jsonFilePath, "%s/%s%s", dataPath, SC_SD_PROBE_DIR, SC_SDU_JSON_FILE_NAME);
		// jsonファイルをgzip圧縮したファイル
		sprintf((char*)uploadFilePath, "%s/%s%s.gz", dataPath, SC_SD_PROBE_DIR, SC_SDU_JSON_FILE_NAME);

		// メモリ確保
		recvBuf = (Char*)SC_MEM_Alloc(SC_SDU_RECIVE_BUFFER_SIZE, e_MEM_TYPE_SD);
		if (NULL == recvBuf) {
			SC_LOG_ErrorPrint(SC_TAG_SDU, (Char*)"SC_MEM_Alloc error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		filePath = (Char*)SC_MEM_Alloc(SC_MAX_PATH, e_MEM_TYPE_SD);
		if (NULL == filePath) {
			SC_LOG_ErrorPrint(SC_TAG_SDU, (Char*)"SC_MEM_Alloc error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// ログフォルダ
		sprintf((char*)filePath, "%s%s", dataPath, SC_SDU_LOG_FILE_PATH);
		// センタ通信ライブラリ初期化
#ifdef SC_SDU_UPLOAD_LOG_ENABLE
		calRet = SC_CAL_Initialize(&smcal, filePath);
#else
		calRet = SC_CAL_Initialize(&smcal, NULL);
#endif
		if (e_SC_CAL_RESULT_SUCCESS != calRet) {
			SC_LOG_ErrorPrint(SC_TAG_SDU, (Char*)"SC_CAL_Initialize error, " HERE);
			ret = SC_SD_ConvertResult(calRet);
			break;
		}

		// 認証情報取得
		ret = SCC_GetAuthInfo(&authInfo);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_SDU, (Char*)"SCC_GetAuthInfo error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_SDU, SC_LOG_END);

	return (ret);
}

/**
 * @brief 終了化処理
 */
void SC_SDU_Final()
{
	SC_LOG_DebugPrint(SC_TAG_SDU, SC_LOG_START);

	// センタ通信ライブラリ終了化
	SC_CAL_Finalize(&smcal);

	// メモリ解放
	if (NULL != recvBuf) {
		SC_MEM_Free(recvBuf, e_MEM_TYPE_SD);
		recvBuf = NULL;
	}

	// ファイル削除
	if (EOS != jsonFilePath[0]) {
		remove(jsonFilePath);
	}
	if (EOS != uploadFilePath[0]) {
		remove(uploadFilePath);
	}

	// 初期化
	sdStatus = SMSDSTATUS_STOP;
	jsonFilePath[0] = EOS;
	uploadFilePath[0] = EOS;
	memset(&authInfo, 0, sizeof(SCC_AUTHINFO));

	SC_LOG_DebugPrint(SC_TAG_SDU, SC_LOG_END);
}

/**
 * @brief 運転特性診断開始要求
 * @param [in]msg   受信メッセージ
 */
void SC_SDU_StartReq(const pthread_msq_msg_t *msg)
{
	sdStatus = SMSDSTATUS_START;
}

/**
 * @brief 運転特性診断停止要求
 * @param [in]msg   受信メッセージ
 */
void SC_SDU_StopReq(const pthread_msq_msg_t *msg)
{
	sdStatus = SMSDSTATUS_STOP;
}

/**
 * @brief アップロード要求
 * @param [in]msg   受信メッセージ
 */
void SC_SDU_UploadReq(const pthread_msq_msg_t *msg)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	const Char	*time = NULL;

	SC_LOG_DebugPrint(SC_TAG_SDU, SC_LOG_START);

	// 診断中
	if (SMSDSTATUS_START == sdStatus) {
		// 運転特性診断用データ取得開始日
		time = (const Char*)msg->data[SC_MSG_REQ_SD_STARTTIME];
		// アップロード
		ret = SC_SDU_Upload(time, false);
	}

	// アップロード結果送信
	SC_SDU_SendUploadResult(ret);

	SC_LOG_DebugPrint(SC_TAG_SDU, SC_LOG_END);
}

/**
 * @brief アップロード終了要求
 * @param [in]msg   受信メッセージ
 */
void SC_SDU_UploadStopReq(const pthread_msq_msg_t *msg)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	const Char	*time = NULL;

	SC_LOG_DebugPrint(SC_TAG_SDU, SC_LOG_START);

	// 運転特性診断用データ取得開始日
	time = (const Char*)msg->data[SC_MSG_REQ_SD_STARTTIME];
	// アップロード
	ret = SC_SDU_Upload(time, true);

	// アップロード結果送信
	SC_SDU_SendUploadStopResult(e_SC_RESULT_SUCCESS, time);

	SC_LOG_DebugPrint(SC_TAG_SDU, SC_LOG_END);
}

/**
 * @brief アップロード
 * @param [in]startTime     運転特性診断用データ取得開始日時(YYYYMMDDhhmmssSSS形式)
 * @param [in]isTerminal	運転特性診断終了フラグ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SDU_Upload(const Char *startTime, Bool isTerminal)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_CAL_RESULT	calRet = e_SC_CAL_RESULT_SUCCESS;
	INT32	zlbRet = SMZLB_OK;
	INT32	id = 1;
	SMPHYDDATA	*data = NULL;
	INT32	num = 0;
	INT32	dataNum = 0;
	Bool	lastFlg = false;
	//Bool	terminal = false;
	Char	*url = NULL;
	//INT32	dataLen = 0;
	Char	*fileName = NULL;
	Char	*res = NULL;
	Char	*body = NULL;
	UINT32	bodyLen = 0;
	UINT32	resLen = 0;
	E_CONTEXT_TYPE	contextType = E_APP_BIN;
	//UChar	*policy = NULL;
	//Char	signature[SC_CAL_SIGNATURE_SIZE] = {};
	SMCALPOSTPARAM	param[7] = {};
	SMAWSINFO	aws = {};
	struct stat	st = {};
	struct timeval	tv = {};
	struct tm	*utcTm = NULL;
	Char	nowTime[16] = {};

	SC_LOG_DebugPrint(SC_TAG_SDU, SC_LOG_START);

	// 初期化
	res = recvBuf;
	smcalOpt.isResOutputFile = false;
	smcalOpt.isAws = true;

	do {
		// パラメータチェック
		if (NULL == startTime) {
			SC_LOG_ErrorPrint(SC_TAG_SDU, "param error[startTime] error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// メモリ確保
		data = (SMPHYDDATA*)SC_MEM_Alloc((sizeof(SMPHYDDATA) * SC_SDU_PHYDDATA_MAXNUM), e_MEM_TYPE_SD);
		if (NULL == data) {
			SC_LOG_ErrorPrint(SC_TAG_SDU, "SC_MEM_Alloc error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		fileName = (Char*)SC_MEM_Alloc(SC_MAX_PATH, e_MEM_TYPE_DYNAMIC);
		if (NULL == fileName) {
			SC_LOG_ErrorPrint(SC_TAG_SDU, "malloc error, " HERE);
			calRet = e_SC_CAL_RESULT_MALLOC_ERR;
			break;
		}
		*fileName = EOS;
		url = (Char*)SC_MEM_Alloc(SC_SDU_URI_STR_MAX_LEN, e_MEM_TYPE_SD);
		if (NULL == url) {
			SC_LOG_ErrorPrint(SC_TAG_SDU, "malloc error, " HERE);
			ret = e_SC_CAL_RESULT_MALLOC_ERR;
			break;
		}

		// センサデータ取得
		ret = SC_SDDAL_GetPhydSensorDataList(SC_SDU_PHYDDATA_MAXNUM, startTime, id, data, &dataNum, &lastFlg);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_SDU, "SC_SDDAL_GetPhydSensorDataList error, " HERE);
			break;
		}
		if ((!isTerminal) && (0 == dataNum)) {
			// データがないのでアップロードしない
			break;
		}

		// リストの末尾からデータの計測時刻を秒単位で比較して、
		// 秒数が変わったら、そこより(末尾に向かって)後ろのデータは
		// アップロード対象から除く(データ件数を減らす)
		for (num = (dataNum - 1); 0 < num; num--) {
			if (0 != memcmp(data[dataNum - 1].time, data[num - 1].time, 14)) {
				dataNum = num;
				break;
			}
		}

		// 現在時刻取得
		gettimeofday(&tv, NULL);
		// UTCに変換
		utcTm = gmtime((time_t*)&tv.tv_sec);
		// YYYYMMDDhhmmssSSS形式
		sprintf((char*)nowTime,
				"%04d%02d%02d%02d%02d%02d",
				(utcTm->tm_year + 1900),
				(utcTm->tm_mon + 1),
				utcTm->tm_mday,
				utcTm->tm_hour,
				utcTm->tm_min,
				utcTm->tm_sec);

		// ファイル名生成(<UserGUID>_<deviceID>_<DataTime>_<TripID>.dat.gz)
		sprintf((char*)fileName, "%s_%s_%s_%.8s-%.6s.dat.gz", authInfo.guid, authInfo.term_id, nowTime, startTime, &startTime[8]);

		// jsonファイル生成
		ret = SC_SDU_CreateJsonFile(&authInfo, startTime, data, dataNum, jsonFilePath, isTerminal);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_SDU, "SC_SDU_CreateJsonFile error, " HERE);
			break;
		}

		// gzip圧縮
		zlbRet = SMZLB_GzCompress(jsonFilePath, uploadFilePath);
		if (SMZLB_OK != zlbRet) {
			SC_LOG_ErrorPrint(SC_TAG_SDU, "SMZLB_GzCompress error, " HERE);
			SC_LOG_ErrorPrint(SC_TAG_SDU, "jsonFilePath=%s, uploadFilePath=%s, " HERE, jsonFilePath, uploadFilePath);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// AWSアクセス情報取得
		ret = SCC_GetAWSInfo(&smcal, e_HTTP_METHOD_POST, e_AWS_BACKET_DRIVE, SC_SDU_AWS_DIRNAME_PROBE, &aws);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_SDU, (Char*)"SCC_GetAWSInfo error, " HERE);
			break;
		}

		// アップロードするファイル情報
		param[0].type = SMCALPOSTPARAM_DATATYPE_TEXT;
		param[0].name = SC_SDU_AWS_KEY;
		param[0].len = SC_SDU_AWS_KEY_PROBE_LEN;
		param[0].data = SC_SDU_AWS_KEY_PROBE;

		param[1].type = SMCALPOSTPARAM_DATATYPE_TEXT;
		param[1].name = SC_SDU_AWS_ACCESS_KEY_ID;
		param[1].len = SC_SDU_AWS_ACCESS_KEY_LEN;
		param[1].data = aws.accessKey;

		param[2].type = SMCALPOSTPARAM_DATATYPE_TEXT;
		param[2].name = SC_SDU_AWS_ACL;
		param[2].len = SC_SDU_AWS_ACL_PARIVATE_LEN;
		param[2].data = SC_SDU_AWS_ACL_PARIVATE;

		param[3].type = SMCALPOSTPARAM_DATATYPE_TEXT;
		param[3].name = SC_SDU_AWS_POLICY;
		param[3].len = strlen((char*)aws.policy);
		param[3].data = aws.policy;

		param[4].type = SMCALPOSTPARAM_DATATYPE_TEXT;
		param[4].name = SC_SDU_AWS_SIGNATURE;
		param[4].len = strlen(aws.signature);
		param[4].data = aws.signature;

		param[5].type = SMCALPOSTPARAM_DATATYPE_TEXT;
		param[5].name = SC_SDU_CONTENT_TYPE;
		param[5].len = SC_SDU_CONTENT_TYPE_BIN_LEN;
		param[5].data = SC_SDU_CONTENT_TYPE_BIN;

		param[6].type = SMCALPOSTPARAM_DATATYPE_FILE;
		param[6].name = (Char*)SC_MEM_Alloc((SC_SDU_FILE_LEN + strlen(fileName) + 1), e_MEM_TYPE_SD);
		if (NULL == param[6].name) {
			SC_LOG_ErrorPrint(SC_TAG_SDU, "SC_MEM_Alloc error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		sprintf((char*)param[6].name, SC_SDU_FILE, fileName);
		// ファイルサイズ取得
		if (0 != stat((char*)uploadFilePath, &st)) {
			SC_LOG_ErrorPrint(SC_TAG_SDU, "stat error[%s] (0x%08x), " HERE, uploadFilePath, errno);
			ret = e_SC_RESULT_FILE_ACCESSERR;
			break;
		}
		param[6].len = (UINT32)st.st_size;
		param[6].data = uploadFilePath;

		// URL生成
		sprintf(url, "https://%s.s3.amazonaws.com/", aws.backetName);

		// アップロード
		calRet = SC_CAL_PostRequest_Multipart(&smcal, url, param, 7, res, SC_SDU_RECIVE_BUFFER_SIZE, &resLen, &smcalOpt);
		if (e_SC_CAL_RESULT_SUCCESS != calRet) {
			SC_LOG_ErrorPrint(SC_TAG_SDU, "SC_CAL_PostRequest_Multipart error, " HERE);
			ret = ConvertResult(calRet);
			break;
		}

		// HTTPデータ解析
		calRet = SC_CAL_AnalyzeResponse(&smcal, res, resLen, (const Char**)&body, &bodyLen, &contextType);
		if (e_SC_CAL_RESULT_SUCCESS != calRet){
			SC_LOG_ErrorPrint(SC_TAG_SDU, (Char*)"SC_CAL_AnalyzeResponse error, " HERE);
			ret = ConvertResult(calRet);
			break;
		}

		// センサデータ削除
		ret = SC_SDDAL_DelPhydSensorData(startTime, data[dataNum - 1].id, true);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_SDU, "SC_SDDAL_DelPhydSensorData error, " HERE);
			break;
		}
	} while (0);

	if ((NULL != fileName) && (EOS != *fileName)) {
		// ファイル削除
		remove(fileName);
	}

	if (e_SC_RESULT_SUCCESS != ret) {
		// アップロードエラー回数をカウントアップ
		CC_SD_COUTUP_UPLOADERRORNUM();
	} else {
		// アップロードエラー回数をクリア
		CC_SD_CLEAR_UPLOADERRORNUM();
	}

	// メモリ解放
	if (NULL != data) {
		SC_MEM_Free(data, e_MEM_TYPE_SD);
	}
	if (NULL != fileName) {
		SC_MEM_Free(fileName, e_MEM_TYPE_SD);
	}
	if (NULL != param[6].name) {
		SC_MEM_Free(param[6].name, e_MEM_TYPE_DYNAMIC);
	}
	if (NULL != url) {
		SC_MEM_Free(url, e_MEM_TYPE_SD);
	}

	SC_LOG_DebugPrint(SC_TAG_SDU, SC_LOG_END);

	return (ret);
}

/**
 * @brief アップロード結果送信
 * @param [in]result    アップロード結果
 */
void SC_SDU_SendUploadResult(E_SC_RESULT result)
{
	pthread_msq_msg_t msg = {};

	SC_LOG_DebugPrint(SC_TAG_SDU, SC_LOG_START);

	// アップロード完了応答メッセージ生成
	msg.data[SC_MSG_MSG_ID] = e_SC_MSGID_RES_SDU_UPLOAD;
	msg.data[SC_MSG_RES_SD_RESULT] = (INT32)result;

	// 送信メッセージをログ出力
	SC_LOG_DebugPrint(SC_TAG_SDU,
			"sendMsg=0x%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x, " HERE,
			msg.data[0],  msg.data[1],  msg.data[2],  msg.data[3],  msg.data[4],
			msg.data[5],  msg.data[6],  msg.data[7],  msg.data[8],  msg.data[9]);

	// アップロード結果メッセージ送信
	if (PTHREAD_MSQ_OK != pthread_msq_msg_send(SC_CORE_MSQID_SDM, &msg, SC_CORE_MSQID_SDU)) {
		SC_LOG_ErrorPrint(SC_TAG_SDU, "pthread_msq_msg_send error, " HERE);
	}

	SC_LOG_DebugPrint(SC_TAG_SDU, SC_LOG_END);
}

/**
 * @brief アップロード終了結果送信
 * @param [in]result    アップロード結果
 * @param [in]startTime 運転特性診断用データ取得開始日
 */
void SC_SDU_SendUploadStopResult(E_SC_RESULT result, const Char *startTime)
{
	E_SC_RESULT	ret = result;
	pthread_msq_msg_t msg = {};
	Char	*tripId = NULL;

	SC_LOG_DebugPrint(SC_TAG_SDU, SC_LOG_START);

	if (NULL != startTime) {
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

	// アップロード終了応答メッセージ生成
	msg.data[SC_MSG_MSG_ID] = e_SC_MSGID_RES_SDU_STOP;
	msg.data[SC_MSG_RES_SD_RESULT] = (INT32)result;
	msg.data[SC_MSG_RES_SD_TRIP_ID] = (uintptr_t)tripId;

	// 送信メッセージをログ出力
	SC_LOG_DebugPrint(SC_TAG_SDU,
			"sendMsg=0x%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x, " HERE,
			msg.data[0],  msg.data[1],  msg.data[2],  msg.data[3],  msg.data[4],
			msg.data[5],  msg.data[6],  msg.data[7],  msg.data[8],  msg.data[9]);

	// アップロード結果メッセージ送信
	if (PTHREAD_MSQ_OK != pthread_msq_msg_send(SC_CORE_MSQID_SDM, &msg, SC_CORE_MSQID_SDU)) {
		SC_LOG_ErrorPrint(SC_TAG_SDU, "pthread_msq_msg_send error, " HERE);
		SC_SDM_StopRes(&msg);
	}

	SC_LOG_DebugPrint(SC_TAG_SDU, SC_LOG_END);
}
