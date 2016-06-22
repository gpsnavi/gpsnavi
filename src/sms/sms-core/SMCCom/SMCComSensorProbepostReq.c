/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#include "SMCComInternal.h"

#define CC_SENSORPROBEPOST_BACKET_NAME_SIZE		(SCC_AWS_BACKETNAME_SIZE + 1)
#define CC_SENSORPROBEPOST_SIGNATURE_SIZE			(SC_CAL_SIGNATURE_SIZE + 1)
#define CC_SENSORPROBEPOST_EXPIRES_SIZE			11
#define CC_SENSORPROBEPOST_ACCESS_KEY_SIZE		(SCC_AWS_ACCESSKEY_SIZE + 1)
#define CC_SENSORPROBEPOST_SECRET_KEY_SIZE		(SCC_AWS_SECRETKEY_SIZE + 1)

#define CC_SENSORPROBEPOST_REQ_SEND_BODY_SIZE	512

#define CC_SENSORPROBEPOST_XML_NODE_BACKET_NAME	"bucket_nm"
#define CC_SENSORPROBEPOST_XML_NODE_SIGNATURE	"signature"
#define CC_SENSORPROBEPOST_XML_NODE_EXPIRES		"expires"
#define CC_SENSORPROBEPOST_XML_NODE_ACCESS_KEY	"access_key"
#define CC_SENSORPROBEPOST_XML_NODE_SECRET_KEY	"secret_key"

// センサプローブUP用認証取得レスポンスXML情報
typedef struct _SENSORPROBEPOSTINFO {
	SMAWSINFO		*aws;
	INT32			*status;
	Char			*apiStatus;
} SENSORPROBEPOSTINFO;

// センサプローブUP用認証取得XMLパーサ
typedef struct _SENSORPROBEPOSTINFO_PARSER {
	INT32			state;
	Char			*buf;
	SENSORPROBEPOSTINFO	probeInfo;
} SENSORPROBEPOSTINFO_PARSER;

enum SensorProbePostStatus {
	CC_SENSORPROBEPOST_NODE_NONE = 0,
	CC_SENSORPROBEPOST_NODE_XML,
	CC_SENSORPROBEPOST_NODE_XML_CHILD,
	CC_SENSORPROBEPOST_NODE_API_STATUS,
	CC_SENSORPROBEPOST_NODE_USER_BACKET_NAME,
	CC_SENSORPROBEPOST_NODE_USER_SIGNATURE,
	CC_SENSORPROBEPOST_NODE_USER_EXPIRES,
	CC_SENSORPROBEPOST_NODE_USER_ACCESS_KEY,
	CC_SENSORPROBEPOST_NODE_USER_SECRET_KEY
};

//------------------------------------------------
// 変数定義
//------------------------------------------------
static INT32	CB_Result;				// 処理結果 (コールバック関数用)

//------------------------------------------------
// 関数定義
//------------------------------------------------
static void CC_SensorProbePostReq_CreateUri(const T_CC_CMN_SMS_API_PRM *param, Char *uri);
static void CC_SensorProbePostReq_CreateBody(const T_CC_CMN_SMS_API_PRM *parm, Char *body);
static E_SC_RESULT CC_SensorProbePostReq_AnalyzeHttpResp(const Char *body, E_CONTEXT_TYPE contextType, SMAWSINFO *aws, const SMCALOPT *opt, Char *apiStatus);
static E_SC_RESULT CC_SensorProbePostReq_XmlParse(const Char* xml, T_CC_CMN_SMS_RESPONSE_INFO* resp_inf, SMAWSINFO *aws, const SMCALOPT *opt);
static void XMLCALL CC_SensorProbePostReq_StartElement(void *userData, const char *name, const char **atts);
static void XMLCALL CC_SensorProbePostReq_EndElement(void *userData, const char *name);
static void XMLCALL CC_SensorProbePostReq_CharacterData(void *userData, const XML_Char *data, INT32 len);

/**
 * @brief センサプローブUP用認証取得
 * @param [IN] smcal        SMCAL
 * @param [IN] parm         APIパラメータ
 * @param [IN] aws          AWS情報
 * @param [IN] recv         センタ受信データ
 * @param [OUT] apiStatus   APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_SensorProbePostReq_SendRecv(SMCAL *smcal,
										   const T_CC_CMN_SMS_API_PRM *parm,
										   SMAWSINFO *aws,
										   Char *recv,
										   INT32 recvBufSize,
										   Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_CAL_RESULT	calRet = e_SC_CAL_RESULT_SUCCESS;
	Char	*uri = NULL;
	Char	*body = NULL;
	UINT32	bodySize = 0;
	Char* data = NULL;
	E_CONTEXT_TYPE	contextType = E_TEXT_XML;
	SMCALOPT	opt = {};
	UINT32	recvSize = 0;
	INT32	status = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// 初期化
		*apiStatus = EOS;
		opt.cancel = SCC_IsCancel;
#ifdef SC_CMN_BASIC_AUTH_SMS
		// BASIC認証
		opt.isBasicAuth = true;
		strcpy(opt.basic.basicAuthId, SC_CMN_BASIC_AUTH_ID);
		strcpy(opt.basic.basicAuthPwd, SC_CMN_BASIC_AUTH_PWD);
#endif

		// メモリ確保
		uri = (Char*)SCC_MALLOC(CC_CMN_URI_STR_MAX_LEN);
		if (NULL == uri) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		data = (Char*)SCC_MALLOC(CC_SENSORPROBEPOST_REQ_SEND_BODY_SIZE);
		if (NULL == data) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// URI生成
		CC_SensorProbePostReq_CreateUri(parm, uri);

		// body生成
		CC_SensorProbePostReq_CreateBody(parm, data);

		// HTTPデータ送受信
		calRet = SC_CAL_PostRequest(smcal, uri, data, strlen(data), recv, recvBufSize, &recvSize, &opt);
		if (e_SC_CAL_RESULT_SUCCESS != calRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SC_CAL_PostRequest error, " HERE);
			ret = ConvertResult(calRet);
			break;
		}

		// HTTPデータ解析
		calRet = SC_CAL_AnalyzeResponseStatus(smcal, recv, recvSize, (const Char**)&body, &bodySize, &contextType, &status);
		if (e_SC_CAL_RESULT_SUCCESS != calRet) {
			if (CC_CMN_SERVER_STOP == status) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"server stop..., " HERE);
				ret = e_SC_RESULT_SERVER_STOP;
			} else {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SC_CAL_AnalyzeResponseStatus error, " HERE);
				ret = ConvertResult(calRet);
			}
			break;
		}

		// レスポンス解析
		ret = CC_SensorProbePostReq_AnalyzeHttpResp(body, contextType, aws, &opt, apiStatus);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SensorProbePostReq_AnalyzeHttpResp error, " HERE);
			break;
		}
	} while (0);

	// メモリ解放
	if (NULL != uri) {
		SCC_FREE(uri);
	}
	if (NULL != data) {
		SCC_FREE(data);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}


/**
 * @brief URL生成
 * @param [IN]  parm        APIパラメータ
 * @param [OUT] uri         URL
 * @return 処理結果(E_SC_RESULT)
 */
void CC_SensorProbePostReq_CreateUri(const T_CC_CMN_SMS_API_PRM *parm, Char *uri)
{
	sprintf((char*)uri,
			"%ssensorprobepost/req",
			parm->ApiPrmNavi.sms_sp_uri
	);
}

/**
 * @brief body生成
 * @param [IN]  parm        APIパラメータ
 * @param [OUT] body        body
 */
void CC_SensorProbePostReq_CreateBody(const T_CC_CMN_SMS_API_PRM *parm,
									  Char *body)
{
	sprintf((char*)body,
			"term_id=%s&term_sig=%s&guid=%s&user_sig=%s",
			parm->ApiPrmMups.new_term_id,
			parm->ApiPrmMups.term_sig,
			parm->ApiPrmMups.guid,
			parm->ApiPrmMups.user_sig
	);
}

/**
 * @brief レスポンス解析
 * @param [IN] body         xmlデータ
 * @param [IN] contextType  コンテキスト
 * @param [IN] opt          オプション情報
 * @param [OUT] apiStatus   APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_SensorProbePostReq_AnalyzeHttpResp(const Char *body,
												  E_CONTEXT_TYPE contextType,
												  SMAWSINFO *aws,
												  const SMCALOPT *opt,
												  Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	T_CC_CMN_SMS_RESPONSE_INFO	rsp_inf;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		if (E_TEXT_XML != contextType) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"Content-Type error, " HERE);
			ret = e_SC_RESULT_SMS_API_ERR;
			break;
		}

		// XML解析
		rsp_inf.apiSts = apiStatus;

		ret = CC_SensorProbePostReq_XmlParse((const Char*)body, &rsp_inf, aws, opt);
		if(e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SensorProbePostReq_XmlParse error, " HERE);
			ret = e_SC_RESULT_SMS_API_ERR;
			break;
		}

		// 正常系のXMLとして解析できなかった場合
		if (CC_CMN_XML_CIC_RES_STS_OK != rsp_inf.sts) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"status error, " HERE);
			ret = e_SC_RESULT_SMS_API_ERR;
			break;
		}
		if (!CHECK_API_STATUS(rsp_inf.apiSts)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"api status error, " HERE);
			ret = e_SC_RESULT_SMS_API_ERR;
			break;
		}
	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief Probepost.req応答メッセージ解析
 * @param [IN] xml      XMLファイルのフルパス
 * @param [IN] resp_inf レスポンス情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_SensorProbePostReq_XmlParse(const Char* xml,
										   T_CC_CMN_SMS_RESPONSE_INFO* resp_inf,
										   SMAWSINFO *aws,
										   const SMCALOPT *opt)
{
	E_SC_RESULT		ret = e_SC_RESULT_SUCCESS;
	SENSORPROBEPOSTINFO_PARSER	probepostParser = {};
	Char buf[CC_CMN_XML_PARSE_DATA_SIZE + 1] = {};
	XML_Parser parser = NULL;
	INT32	done = 0;
	INT32	len = 0;
	INT32	parsedLen = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// 初期化
		resp_inf->sts = 0;
		probepostParser.buf = (Char*)SCC_MALLOC(CC_CMN_XML_BUF_SIZE + 1);
		if (NULL == probepostParser.buf) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			CB_Result = CC_CMN_RESULT_MALLOC_ERR;
			ret = CB_Result;
			break;
		}
		probepostParser.probeInfo.status = &resp_inf->sts;
		probepostParser.probeInfo.apiStatus = &resp_inf->apiSts[0];
		probepostParser.probeInfo.aws = aws;
		CB_Result = CC_CMN_RESULT_OK;

		// XMLパーサ生成
		parser = XML_ParserCreate(NULL);
		if (NULL == parser) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"XML_ParserCreate error, " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			ret = CB_Result;
			break;
		}

		// コールバック関数設定
		XML_SetUserData(parser, &probepostParser);
		XML_SetElementHandler(parser, CC_SensorProbePostReq_StartElement, CC_SensorProbePostReq_EndElement);
		XML_SetCharacterDataHandler(parser, CC_SensorProbePostReq_CharacterData);

		while (!done) {
			if (CC_ISCANCEL()) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"cancel, " HERE);
				CB_Result = e_SC_RESULT_CANCEL;
				ret = CB_Result;
				break;
			}

			strncpy((char*)buf, &xml[parsedLen], (sizeof(buf) - 1));
			len = (INT32)strlen(buf);
			parsedLen += len;
			if (strlen(xml) <= parsedLen) {
				done = 1;
			} else {
				done = 0;
			}

			// XML解析
			if ((XML_STATUS_ERROR == XML_Parse(parser, (const char*)buf, len, done)) || (e_SC_RESULT_SUCCESS != CB_Result)) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"XML_Parse error(0x%08x), " HERE, CB_Result);
				CB_Result = e_SC_RESULT_SMS_API_ERR;
				ret = CB_Result;
				break;
			}

			if (!done) {
				// バッファクリア
				memset(buf, 0, (sizeof(buf) - 1));
			}
		}
	} while (0);

	if (NULL != probepostParser.buf) {
		SCC_FREE(probepostParser.buf);
	}

	if (NULL != parser) {
		XML_ParserFree(parser);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief タグ解析開始
 * @param [IN/OUT] userData ユーザデータ
 * @param [IN] name     タグ名
 * @param [OUT] atts    属性(未使用)
 */
void XMLCALL CC_SensorProbePostReq_StartElement(void *userData, const char *name, const char **atts)
{
	SENSORPROBEPOSTINFO_PARSER *parser = (SENSORPROBEPOSTINFO_PARSER*)userData;

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		if (e_SC_RESULT_SUCCESS != CB_Result) {
			break;
		}
		if (CC_ISCANCEL()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"cancel, " HERE);
			CB_Result = e_SC_RESULT_CANCEL;
			break;
		}

		// パラメータチェック
		if (NULL == userData) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[userData], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}
		if (NULL == name) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[name], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}

		// 初期化
		memset(parser->buf, 0, (CC_CMN_XML_BUF_SIZE + 1));

		if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_XML)) {
			// <xml>
			parser->state = CC_SENSORPROBEPOST_NODE_XML;
		}

		// <xml>
		if (CC_SENSORPROBEPOST_NODE_XML == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_API_STATUS)) {
				// <api_status>
				parser->state = CC_SENSORPROBEPOST_NODE_API_STATUS;
			} else if (0 == strcmp((char*)name, (char*)CC_SENSORPROBEPOST_XML_NODE_BACKET_NAME)) {
				// <bucket_nm>
				parser->state = CC_SENSORPROBEPOST_NODE_USER_BACKET_NAME;
			} else if (0 == strcmp((char*)name, (char*)CC_SENSORPROBEPOST_XML_NODE_SIGNATURE)) {
				// <signature>
				parser->state = CC_SENSORPROBEPOST_NODE_USER_SIGNATURE;
			} else if (0 == strcmp((char*)name, (char*)CC_SENSORPROBEPOST_XML_NODE_EXPIRES)) {
				// <expires>
				parser->state = CC_SENSORPROBEPOST_NODE_USER_EXPIRES;
			} else if (0 == strcmp((char*)name, (char*)CC_SENSORPROBEPOST_XML_NODE_ACCESS_KEY)) {
				// <access_key>
				parser->state = CC_SENSORPROBEPOST_NODE_USER_ACCESS_KEY;
			} else if (0 == strcmp((char*)name, (char*)CC_SENSORPROBEPOST_XML_NODE_SECRET_KEY)) {
				// <secret_key>
				parser->state = CC_SENSORPROBEPOST_NODE_USER_SECRET_KEY;
			} else if (0 != strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_XML)) {
				parser->state = CC_SENSORPROBEPOST_NODE_XML_CHILD;
			}
		} else {
			// 上記以外
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"format error(name=%s), state=%d, " HERE, name, parser->state);
			CB_Result = e_SC_RESULT_SMS_API_ERR;
			break;
		}
	} while (0);

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}

/**
 * @brief タグ解析終了
 * @param [IN/OUT] userData ユーザデータ
 * @param [IN] name     タグ名
 */
void XMLCALL CC_SensorProbePostReq_EndElement(void *userData, const char *name)
{
	SENSORPROBEPOSTINFO_PARSER *parser = (SENSORPROBEPOSTINFO_PARSER*)userData;

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		if (e_SC_RESULT_SUCCESS != CB_Result) {
			break;
		}
		if (CC_ISCANCEL()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"cancel, " HERE);
			CB_Result = e_SC_RESULT_CANCEL;
			break;
		}

		// パラメータチェック
		if (NULL == userData) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[userData], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}
		if (NULL == parser->buf) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[parser->buf], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}
		if (NULL == name) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[name], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}

		if (CC_SENSORPROBEPOST_NODE_API_STATUS == parser->state) {
			// <api_status>
			strcpy((char*)parser->probeInfo.apiStatus, (char*)parser->buf);
			parser->state = CC_SENSORPROBEPOST_NODE_XML;
		} else if (CC_SENSORPROBEPOST_NODE_USER_BACKET_NAME == parser->state) {
			// <bucket_nm>
			strcpy((char*)parser->probeInfo.aws->backetName, (char*)parser->buf);
			parser->state = CC_SENSORPROBEPOST_NODE_XML;
		} else if (CC_SENSORPROBEPOST_NODE_USER_SIGNATURE == parser->state) {
			// <signature>
			strcpy((char*)parser->probeInfo.aws->signature, (char*)parser->buf);
			parser->state = CC_SENSORPROBEPOST_NODE_XML;
		} else if (CC_SENSORPROBEPOST_NODE_USER_EXPIRES == parser->state) {
			// <expires>
			parser->probeInfo.aws->expires = atol((char*)parser->buf);
			parser->state = CC_SENSORPROBEPOST_NODE_XML;
		} else if (CC_SENSORPROBEPOST_NODE_USER_ACCESS_KEY == parser->state) {
			// <access_key>
			strcpy((char*)parser->probeInfo.aws->accessKey, (char*)parser->buf);
			parser->state = CC_SENSORPROBEPOST_NODE_XML;
		} else if (CC_SENSORPROBEPOST_NODE_USER_SECRET_KEY == parser->state) {
			// <secret_key>
			strcpy((char*)parser->probeInfo.aws->secretKey, (char*)parser->buf);
			parser->state = CC_SENSORPROBEPOST_NODE_XML;
		} else if (CC_SENSORPROBEPOST_NODE_XML_CHILD == parser->state) {
			 parser->state = CC_SENSORPROBEPOST_NODE_XML;
		}
	} while (0);

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}

/**
 * @brief 解析データ
 * @param [IN] userData ユーザデータ
 * @param [IN] data     データ
 * @param [IN] len      データ長
 */
void XMLCALL CC_SensorProbePostReq_CharacterData(void *userData, const XML_Char *data, INT32 len)
{
	SENSORPROBEPOSTINFO_PARSER *parser = (SENSORPROBEPOSTINFO_PARSER*)userData;
	//char buf[CC_CMN_XML_PARSE_DATA_SIZE + 1] = {};
	INT32	bufLen = 0;

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		if (e_SC_RESULT_SUCCESS != CB_Result) {
			break;
		}
		if (CC_ISCANCEL()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"cancel, " HERE);
			CB_Result = e_SC_RESULT_CANCEL;
			break;
		}

		// パラメータチェック
		if (NULL == userData) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[userData], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}
		if (NULL == parser->buf) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[parser->buf], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}
		if (NULL == data) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[data], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}

		// データをバッファにコピー
		bufLen = strlen((char*)parser->buf);

		if (CC_SENSORPROBEPOST_NODE_API_STATUS == parser->state) {
			// <api_status>
			if (CC_CMN_XML_RES_STS_CODE_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_SENSORPROBEPOST_NODE_USER_BACKET_NAME == parser->state) {
			// <bucket_nm>
			if (CC_SENSORPROBEPOST_BACKET_NAME_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_SENSORPROBEPOST_NODE_USER_SIGNATURE == parser->state) {
			// <signature>
			if (CC_SENSORPROBEPOST_SIGNATURE_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_SENSORPROBEPOST_NODE_USER_EXPIRES == parser->state) {
			// <expires>
			if (CC_SENSORPROBEPOST_EXPIRES_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_SENSORPROBEPOST_NODE_USER_ACCESS_KEY == parser->state) {
			// <access_key>
			if (CC_SENSORPROBEPOST_ACCESS_KEY_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_SENSORPROBEPOST_NODE_USER_SECRET_KEY == parser->state) {
			// <secret_key>
			if (CC_SENSORPROBEPOST_SECRET_KEY_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		}
	} while (0);

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}
