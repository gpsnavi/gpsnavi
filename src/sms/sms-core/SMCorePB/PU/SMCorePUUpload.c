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

#define SC_PU_LOG_FILE_PATH			"/Log/SMCAL_probe.log"	// (デバッグ用)ログファイルのパス
#define SC_PU_URI_MAX_LEN			(1024 + 1)
#define SC_PU_RECIVE_BUFFER_SIZE	(1024 * 1024 * 2)	// TODO

#define SC_PU_AWS_DIRNAME_PROBE		"probe/"	// Probe
#define SC_PU_AWS_KEY				"\"key\""
#define SC_PU_AWS_KEY_PROBE			"probe/${filename}"
#define SC_PU_AWS_ACCESS_KEY_ID		"\"AWSAccessKeyId\""
#define SC_PU_AWS_ACL				"\"acl\""
#define SC_PU_AWS_ACL_PARIVATE		"private"
#define SC_PU_AWS_POLICY			"\"policy\""
#define SC_PU_AWS_SIGNATURE			"\"signature\""
#define SC_PU_CONTENT_TYPE			"\"Content-Type\""
#define SC_PU_CONTENT_TYPE_BIN		"application/binary"
#define SC_PU_FILE					"\"file\"; filename=\"%s\"\r\nContent-Type: application/octet-stream"
#define SC_PU_AWS_KEY_PROBE_LEN		17
#define SC_PU_AWS_ACCESS_KEY_LEN	20
#define SC_PU_AWS_ACL_PARIVATE_LEN	7
#define SC_PU_CONTENT_TYPE_BIN_LEN	18
#define SC_PU_FILE_LEN				59
#define SC_PU_LOGINID_MINSIZE		4
#define SC_PU_LOGINID_MAXSIZE		128
#define SC_PU_POSDATA_MINSIZE		(SC_PU_LOGINID_MINSIZE + 14)
#define SC_PU_POSDATA_MAXSIZE		(SC_PU_LOGINID_MAXSIZE + 14)
#define SC_PU_URI_STR_MAX_LEN		(1024+1)

static SMCAL	smcal;
static SMCALOPT	smcalOpt;
static Char		*recvBuf;

static void SC_PU_UploadProbe(pthread_msq_msg_t *msg);
static Char *SC_PU_GetSequenceNo(Char *data);

// 初期化処理
E_SC_RESULT SC_PU_Upload_Initialize()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_CAL_RESULT	calRet = e_SC_CAL_RESULT_SUCCESS;
	Char	logFilePath[SC_MAX_PATH] = {};

	SC_LOG_DebugPrint(SC_TAG_PU, SC_LOG_START);

	// 初期化
	memset(&smcal, 0, sizeof(SMCAL));
	memset(&smcalOpt, 0, sizeof(SMCALOPT));
	smcalOpt.isAws = true;
	recvBuf = NULL;

	do {
		sprintf(logFilePath, "%s%s", SC_MNG_GetApRootDirPath(), SC_PU_LOG_FILE_PATH);

		// センタ通信ライブラリ初期化
		calRet = SC_CAL_Initialize(&smcal, logFilePath);
		if (e_SC_CAL_RESULT_SUCCESS != calRet) {
			SC_LOG_ErrorPrint(SC_TAG_PU, (Char*)"SC_CAL_Initialize err, " HERE);
			ret = ConvertResult(calRet);
			break;
		}

		// メモリ確保
		recvBuf = (Char*)SC_MEM_Alloc((SC_PU_RECIVE_BUFFER_SIZE), e_MEM_TYPE_DYNAMIC);
		if (NULL == recvBuf) {
			SC_LOG_ErrorPrint(SC_TAG_PU, (Char*)"SC_MEM_Alloc err, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_PU, SC_LOG_END);

	return (ret);
}

// 終了化処理
E_SC_RESULT SC_PU_Upload_Finalize()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_PU, SC_LOG_START);

	// センタ通信ライブラリ終了化
	SC_CAL_Finalize(&smcal);

	// メモリ解放
	if (NULL != recvBuf) {
		SC_MEM_Free(recvBuf, e_MEM_TYPE_DYNAMIC);
		recvBuf = NULL;
	}

	SC_LOG_DebugPrint(SC_TAG_PU, SC_LOG_END);

	return (ret);
}

// データアップロード
void SC_PU_Upload(pthread_msq_msg_t *msg)
{
//	SC_LOG_DebugPrint(SC_TAG_PU, SC_LOG_START);

	// Probeデータアップロード
	SC_PU_UploadProbe(msg);

//	SC_LOG_DebugPrint(SC_TAG_PU, SC_LOG_END);
}

// Probeデータアップロード
void SC_PU_UploadProbe(pthread_msq_msg_t *msg)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_CAL_RESULT	calRet = e_SC_CAL_RESULT_SUCCESS;
	Char	*url = NULL;
	Char	*data = NULL;
	INT32	dataLen = 0;
	Char	*fileName = NULL;
	Char	*prbFileName = NULL;
	Char	*res = NULL;
	Char	*body = NULL;
	UINT32	bodyLen = 0;
	UINT32	resLen = 0;
	E_CONTEXT_TYPE	contextType = E_APP_BIN;
	Char	*seqNo = NULL;
	//UChar	*policy = NULL;
	//Char	signature[SC_CAL_SIGNATURE_SIZE] = {};
	SMCALPOSTPARAM	param[7] = {};
	SMAWSINFO	aws = {};

	SC_LOG_DebugPrint(SC_TAG_PU, SC_LOG_START);

	// 初期化
	res = recvBuf;
	smcalOpt.isResOutputFile = false;
	smcalOpt.isAws = true;

	do {
		// Probeデータ取得
		data = (Char*)msg->data[SC_MSG_REQ_PRBDT_MEM_ADDR];
		dataLen = msg->data[SC_MSG_REQ_PRBDT_MEM_SIZE];
		fileName = (Char*)msg->data[SC_MSG_REQ_PRBDT_FILE_NAME];
//		apiParam = (T_CC_CMN_SMS_API_PRM*)msg->data[SC_MSG_REQ_API_PARAM];

		if (NULL == data) {
			SC_LOG_ErrorPrint(SC_TAG_PU, "msg->data[%d] error, " HERE, (INT32)SC_MSG_REQ_PRBDT_MEM_ADDR);
			calRet = e_SC_CAL_RESULT_BADPARAM;
			break;
		}
		if (NULL == fileName) {
			SC_LOG_ErrorPrint(SC_TAG_PU, "msg->data[%d] error, " HERE, (INT32)SC_MSG_REQ_PRBDT_FILE_NAME);
			calRet = e_SC_CAL_RESULT_BADPARAM;
			break;
		}

		// メモリ確保
		url = (Char*)SC_MEM_Alloc(SC_PU_URI_STR_MAX_LEN, e_MEM_TYPE_DYNAMIC);
		if (NULL == url) {
			SC_LOG_ErrorPrint(SC_TAG_PU, "malloc err, " HERE);
			calRet = e_SC_CAL_RESULT_MALLOC_ERR;
			break;
		}
		prbFileName = (Char*)SC_MEM_Alloc(SC_MAX_PATH, e_MEM_TYPE_DYNAMIC);
		if (NULL == prbFileName) {
			SC_LOG_ErrorPrint(SC_TAG_PU, "malloc error, " HERE);
			calRet = e_SC_CAL_RESULT_MALLOC_ERR;
			break;
		}

		// プローブ情報バイナリからシーケンス番号
		seqNo = SC_PU_GetSequenceNo(data);
		if (NULL == seqNo) {
			SC_LOG_ErrorPrint(SC_TAG_PU, "SC_PU_GetSequenceNo error, " HERE);
			calRet = e_SC_CAL_RESULT_BADPARAM;
			break;
		}

		// AWSアクセス情報取得
		ret = SCC_GetAWSInfo(&smcal, e_HTTP_METHOD_POST, e_AWS_BACKET_PROBE, SC_PU_AWS_DIRNAME_PROBE, &aws);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_PU, (Char*)"SCC_GetAWSInfo error, " HERE);
			calRet = e_SC_CAL_RESULT_TCP_COMMUNICATION_ERR;
			break;
		}

		// アップロードするファイル情報
		param[0].type = SMCALPOSTPARAM_DATATYPE_TEXT;
		param[0].name = SC_PU_AWS_KEY;
		param[0].len = SC_PU_AWS_KEY_PROBE_LEN;
		param[0].data = SC_PU_AWS_KEY_PROBE;

		param[1].type = SMCALPOSTPARAM_DATATYPE_TEXT;
		param[1].name = SC_PU_AWS_ACCESS_KEY_ID;
		param[1].len = SC_PU_AWS_ACCESS_KEY_LEN;
		param[1].data = aws.accessKey;

		param[2].type = SMCALPOSTPARAM_DATATYPE_TEXT;
		param[2].name = SC_PU_AWS_ACL;
		param[2].len = SC_PU_AWS_ACL_PARIVATE_LEN;
		param[2].data = SC_PU_AWS_ACL_PARIVATE;

		param[3].type = SMCALPOSTPARAM_DATATYPE_TEXT;
		param[3].name = SC_PU_AWS_POLICY;
		param[3].len = strlen((char*)aws.policy);
		param[3].data = aws.policy;

		param[4].type = SMCALPOSTPARAM_DATATYPE_TEXT;
		param[4].name = SC_PU_AWS_SIGNATURE;
		param[4].len = strlen(aws.signature);
		param[4].data = aws.signature;

		param[5].type = SMCALPOSTPARAM_DATATYPE_TEXT;
		param[5].name = SC_PU_CONTENT_TYPE;
		param[5].len = SC_PU_CONTENT_TYPE_BIN_LEN;
		param[5].data = SC_PU_CONTENT_TYPE_BIN;

		param[6].type = SMCALPOSTPARAM_DATATYPE_TEXT;
		param[6].name = (Char*)SC_MEM_Alloc((SC_PU_FILE_LEN + strlen(fileName) + 1), e_MEM_TYPE_DYNAMIC);
		sprintf((char*)param[6].name, SC_PU_FILE, fileName);
		param[6].len = dataLen;
		param[6].data = data;

		// URL生成
		sprintf(url, "https://%s.s3.amazonaws.com/", aws.backetName);

		// Probeデータアップロード
		calRet = SC_CAL_PostRequest_Multipart(&smcal, url, param, 7, res, SC_PU_RECIVE_BUFFER_SIZE, &resLen, &smcalOpt);
		if (e_SC_CAL_RESULT_SUCCESS != calRet) {
			SC_LOG_ErrorPrint(SC_TAG_PU, "SC_CAL_PostRequest_Multipart error, " HERE);
			break;
		}

		// HTTPデータ解析
		calRet = SC_CAL_AnalyzeResponse(&smcal, res, resLen, (const Char**)&body, &bodyLen, &contextType);
		if (e_SC_CAL_RESULT_SUCCESS != calRet){
			SC_LOG_ErrorPrint(SC_TAG_PU, (Char*)"SC_CAL_AnalyzeResponse error, " HERE);
			break;
		}

		// プローブUP完了通知のファイル名
		sprintf(prbFileName, "%s%s", SC_PU_AWS_DIRNAME_PROBE, fileName);

		// プローブUP完了通知
		ret = SCC_NotifyProbePostComp(&smcal, (const Char*)prbFileName, (const Char*)aws.backetName);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_PU, (Char*)"SCC_NotifyProbePostComp error, " HERE);
			calRet = e_SC_CAL_RESULT_TCP_COMMUNICATION_ERR;
			break;
		}
	} while (0);

	// メモリ解放
	if (NULL != data) {
		SC_MEM_Free(data, e_MEM_TYPE_DYNAMIC);
	}
	if (NULL != fileName) {
		SC_MEM_Free(fileName, e_MEM_TYPE_DYNAMIC);
	}
	if (NULL != param[6].name) {
		SC_MEM_Free(param[6].name, e_MEM_TYPE_DYNAMIC);
	}
	if (NULL != url) {
		SC_MEM_Free(url, e_MEM_TYPE_DYNAMIC);
	}
	if (NULL != prbFileName) {
		SC_MEM_Free(prbFileName, e_MEM_TYPE_DYNAMIC);
	}

	// 戻り値変換（E_SC_CAL_RESULT → E_SC_RESULT）
	ret = ConvertResult(calRet);
	// Probeデータアップロード結果送信
	SC_PU_SendUploadResult(ret);

	SC_LOG_DebugPrint(SC_TAG_PU, SC_LOG_END);
}

// Probeデータアップロード結果送信
void SC_PU_SendUploadResult(E_SC_RESULT result)
{
	pthread_msq_msg_t msg = {};

	SC_LOG_DebugPrint(SC_TAG_PU, SC_LOG_START);

	// アップロード終了応答メッセージ生成
	msg.data[SC_MSG_MSG_ID] = e_SC_MSGID_RES_PU_UPLOAD_FINISH;
	msg.data[SC_MSG_RES_UPLOAD_RESULT] = (INT32)result;

	// 送信メッセージをログ出力
	SC_LOG_DebugPrint(SC_TAG_PU,
			"sendMsg=0x%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x, " HERE,
			msg.data[0],  msg.data[1],  msg.data[2],  msg.data[3],  msg.data[4],
			msg.data[5],  msg.data[6],  msg.data[7],  msg.data[8],  msg.data[9]);

	// Probeデータアップロード結果メッセージ送信
	if (PTHREAD_MSQ_OK != pthread_msq_msg_send(SC_CORE_MSQID_PM, &msg, SC_CORE_MSQID_PU)) {
		SC_LOG_ErrorPrint(SC_TAG_PU, "pthread_msq_msg_send error, " HERE);
		SC_PM_NoticeUploadFinish(&msg);
	}

	SC_LOG_DebugPrint(SC_TAG_PU, SC_LOG_END);
}

/**
 * @brief プローブ情報バイナリからシーケンス番号を取得する
 * @param[in] data  プローブ情報バイナリ
 * @return シーケンス番号
 */
Char *SC_PU_GetSequenceNo(Char *data)
{
	Char	*seqNo = NULL;
	Char	*wk = NULL;
	Char	*chr = NULL;

	do {
		// パラメータチェック
		if (NULL == data) {
			break;
		}
		wk = data;

		// データバージョン
		chr = strchr(wk, EOS);
		if (NULL == chr) {
			break;
		}
		wk = chr + 1;

		// Androidアプリケーションバージョン
		chr = strchr(wk, EOS);
		if (NULL == chr) {
			break;
		}
		wk = chr + 1;

		// データサイズ
		wk += SC_PM_PROBE_HEAD_SIZE_DATA_SZ;

		// データID
		wk += SC_PM_PROBE_HEAD_SIZE_DATA_ID;

		// ユーザID
		chr = strchr(wk, EOS);
		if (NULL == chr) {
			break;
		}
		wk = chr + 1;

		// デバイスID
		chr = strchr(wk, EOS);
		if (NULL == chr) {
			break;
		}
		wk = chr + 1;

		// シーケンス番号
		seqNo = wk;
	} while (0);

	return (seqNo);
}
