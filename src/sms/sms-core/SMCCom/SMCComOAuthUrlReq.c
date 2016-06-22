/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

//------------------------------------------------------------------------------------------------------------------------
//
//	SMCComOAuthUrlReq：auth/req/処理
//
//------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------
//インクルード
//---------------------------------------------------------------------------------
#include "SMCComInternal.h"

#define CC_OAUTHURLREQ_SEND_BODY_SIZE	1024

#define	CC_OAUTHURLREQ_XML_NODE_URL						"oauth_url"
#define	CC_OAUTHURLREQ_XML_NODE_SESSION					"oauth_session"

#define	CC_OAUTHURLREQ_XML_DATA_URL_SIZE				CC_CMN_OAUTH_URL_SIZE
#define	CC_OAUTHURLREQ_XML_DATA_SESSION_SIZE			CC_CMN_OAUTH_SESSION_ID_SIZE

#define	CC_OAUTHURLREQ_PHP_SESSION_ID					"PHPSESSID="

// OAuth連携用URL取得レスポンスXML情報
typedef struct _OAUTHURLINFO {
	Char			*url;
	Char			*sessionId;
	INT32			*status;
	Char			*apiStatus;
} OAUTHURLINFO;

// OAuth連携用URL取得XMLパーサ
typedef struct _OAUTHURL_PARSER {
	INT32			state;
	Char			*buf;
	OAUTHURLINFO	urlInfo;
} OAUTHURL_PARSER;

enum OAuthUrlReqRespStatus {
	CC_OAUTHURLREQ_NODE_NONE = 0,
	CC_OAUTHURLREQ_NODE_XML,
	CC_OAUTHURLREQ_NODE_XML_CHILD,
	CC_OAUTHURLREQ_NODE_API_STATUS,
	CC_OAUTHURLREQ_NODE_OAUTH_URL,
	CC_OAUTHURLREQ_NODE_OAUTH_SESSION
};

//---------------------------------------------------------------------------------
//プロトタイプ宣言
//---------------------------------------------------------------------------------
static void CC_OAuthUrlReq_CreateUri(const T_CC_CMN_SMS_API_PRM *param, Char *uri);
static void CC_OAuthUrlReq_CreateBody(const T_CC_CMN_SMS_API_PRM *parm, const Char *serviceId, Char *body);
static E_SC_RESULT CC_OAuthUrlReq_AnalyzeHttpResp(const Char *body, E_CONTEXT_TYPE contextType, Char *url, Char *sessionId, SMCALOPT *opt, Char *apiStatus);
static E_SC_RESULT CC_OAuthUrlReq_XmlParse(const Char *xml, T_CC_CMN_SMS_RESPONSE_INFO *resp_inf, Char *url, Char *sessionId, SMCALOPT *opt);
static void CC_OAuthUrlReq_StartElement(void *userData, const char *name, const char **atts);
static void CC_OAuthUrlReq_EndElement(void *userData, const char *name);
static void CC_OAuthUrlReq_CharacterData(void *userData, const XML_Char *data, INT32 len);


//---------------------------------------------------------------------------------
//変数定義
//---------------------------------------------------------------------------------
static INT32	CB_Result;				// 処理結果 (コールバック関数用)


/**
 * @brief OAuth認証用URL取得
 * @param [IN] smcal        SMCAL
 * @param [IN] parm         APIパラメータ
 * @param [IN] serviceId    サービスID
 * @param [OUT] url         OAuth連携用URL
 * @param [OUT] sessionId   PHPセッションID
 * @param [IN] recv         センタ受信データ
 * @param [IN] recvBufSize  センタ受信データサイズ
 * @param [OUT] apiStatus   APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_OAuthUrlReq_SendRecv(SMCAL *smcal,
		  	  	  	  	  	  	    const T_CC_CMN_SMS_API_PRM *parm,
		  	  	  	  	  	  	    const Char *serviceId,
		  	  	  	  	  	  	    Char *url,
		  	  	  	  	  	  	    Char *sessionId,
		  	  	  	  	  	  	    Char *recv,
									INT32 recvBufSize,
									Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_CAL_RESULT	calRet = e_SC_CAL_RESULT_SUCCESS;
	Char	*uri = NULL;
	Char	*body = NULL;
	UINT32	bodySize = 0;
	E_CONTEXT_TYPE	contextType = E_TEXT_XML;
	SMCALOPT	opt = {};
	UINT32	recvSize = 0;
	Char	*data = NULL;
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
		data = (Char*)SCC_MALLOC(CC_OAUTHURLREQ_SEND_BODY_SIZE);
		if (NULL == data) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// URI生成
		CC_OAuthUrlReq_CreateUri(parm, uri);

		// body生成
		CC_OAuthUrlReq_CreateBody(parm, serviceId, data);

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
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SC_CAL_AnalyzeResponse error, " HERE);
				ret = ConvertResult(calRet);
			}
			break;
		}

		// レスポンス解析
		ret = CC_OAuthUrlReq_AnalyzeHttpResp(body, contextType, url, sessionId, &opt, apiStatus);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_OAuthUrlReq_AnalyzeHttpResp error, " HERE);
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
*/
void CC_OAuthUrlReq_CreateUri(const T_CC_CMN_SMS_API_PRM *param, Char *uri)
{
	sprintf((char*)uri,
			"%surlreq.php",
			CC_CMN_XML_BASIC_CENTER_SMS_AUTH_URI
	);
}

/**
 * @brief body生成
 * @param[in] parm      SMS APIパラメータ
 * @param[in] serviceId サービスID
 * @param [OUT] body    body
*/
void CC_OAuthUrlReq_CreateBody(const T_CC_CMN_SMS_API_PRM *parm, const Char *serviceId, Char *body)
{
	sprintf((char*)body,
			"term_id=%s&term_sig=%s&svc_id=%s",
			parm->ApiPrmMups.new_term_id,
			parm->ApiPrmMups.term_sig,
			serviceId
	);
}

/**
 * @brief レスポンス解析
 * @param [IN] body         xmlデータ
 * @param [IN] contextType  コンテキスト
 * @param [OUT] url         OAuth認証用URL
 * @param [OUT] sessionId   PHPセッションID
 * @param [IN] opt          オプション情報
 * @param [OUT] apiStatus   APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_OAuthUrlReq_AnalyzeHttpResp(const Char *body,
										   E_CONTEXT_TYPE contextType,
										   Char *url,
										   Char *sessionId,
										   SMCALOPT *opt,
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

		ret = CC_OAuthUrlReq_XmlParse((const Char*)body, &rsp_inf, url, sessionId, opt);
		if(e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_OAuthUrlReq_XmlParse error, " HERE);
			ret = e_SC_RESULT_SMS_API_ERR;
			break;
		}

		// 正常系のXMLとして解析できなかった場合
		if (CC_CMN_XML_CIC_RES_STS_OK != rsp_inf.sts) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"status error, " HERE);
			ret = e_SC_RESULT_SMS_API_ERR;
			break;
		}
		if ((!CHECK_API_STATUS(rsp_inf.apiSts)) && (!CHECK_API_STATUS2(rsp_inf.apiSts))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"api status error, " HERE);
			ret = e_SC_RESULT_SMS_API_ERR;
			break;
		}
	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief urlreq.php応答メッセージ解析
 * @param [IN] xml          XMLファイルのフルパス
 * @param [IN] resp_inf     CICレスポンス情報
 * @param [OUT] url         OAuth認証用URL
 * @param [OUT] sessionId   PHPセッションID
 * @param [OUT] opt         オプション情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_OAuthUrlReq_XmlParse(const Char *xml,
									T_CC_CMN_SMS_RESPONSE_INFO *resp_inf,
									Char *url,
									Char *sessionId,
									SMCALOPT *opt)
{
	E_SC_RESULT		ret = e_SC_RESULT_SUCCESS;
	OAUTHURL_PARSER	oauthParser = {};
	Char buf[CC_CMN_XML_PARSE_DATA_SIZE + 1] = {};
	XML_Parser parser = NULL;
	INT32	done = 0;
	INT32	len = 0;
	INT32	parsedLen = 0;
	Char	*ptr = NULL;
	Char	*ptr2 = NULL;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// 初期化
		resp_inf->sts = 0;
		oauthParser.buf = (Char*)SCC_MALLOC(CC_CMN_XML_LARGE_BUF_SIZE + 1);
		if (NULL == oauthParser.buf) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			CB_Result = e_SC_RESULT_MALLOC_ERR;
			ret = CB_Result;
			break;
		}
		oauthParser.urlInfo.status = &resp_inf->sts;
		oauthParser.urlInfo.apiStatus = &resp_inf->apiSts[0];
		oauthParser.urlInfo.url = url;
		oauthParser.urlInfo.sessionId = sessionId;
		CB_Result = e_SC_RESULT_SUCCESS;

		// XMLパーサ生成
		parser = XML_ParserCreate(NULL);
		if (NULL == parser) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"XML_ParserCreate error, " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			ret = CB_Result;
			break;
		}

		// コールバック関数設定
		XML_SetUserData(parser, &oauthParser);
		XML_SetElementHandler(parser, CC_OAuthUrlReq_StartElement, CC_OAuthUrlReq_EndElement);
		XML_SetCharacterDataHandler(parser, CC_OAuthUrlReq_CharacterData);

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
		if(e_SC_RESULT_SUCCESS != ret) {
			break;
		}

		// URLからPHPセッションIDを抜き出す
		if (EOS == *sessionId) {
			ptr = (Char*)strstr((char*)url, (char*)CC_OAUTHURLREQ_PHP_SESSION_ID);
			if (NULL == ptr) {
				break;
			}
			ptr += strlen((char*)CC_OAUTHURLREQ_PHP_SESSION_ID);
			ptr2 = (Char*)strchr(ptr, '&');
			if (NULL != ptr2) {
				len = (INT32)(ptr2 - ptr);
				ptr = ptr2;
			} else {
				len = strlen((char*)ptr);
			}
			if (CC_OAUTHURLREQ_XML_DATA_SESSION_SIZE > len) {
				memcpy(sessionId, ptr, len);
				sessionId[len] = EOS;
			}
		}
	} while (0);

	if (NULL != oauthParser.buf) {
		SCC_FREE(oauthParser.buf);
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
void XMLCALL CC_OAuthUrlReq_StartElement(void *userData, const char *name, const char **atts)
{
	OAUTHURL_PARSER *parser = (OAUTHURL_PARSER*)userData;

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
			parser->state = CC_OAUTHURLREQ_NODE_XML;
		}

		// <xml>
		if (CC_OAUTHURLREQ_NODE_XML == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_API_STATUS)) {
				// <api_status>
				parser->state = CC_OAUTHURLREQ_NODE_API_STATUS;
			} else if (0 == strcmp((char*)name, (char*)CC_OAUTHURLREQ_XML_NODE_URL)) {
				// <oauth_url>
				parser->state = CC_OAUTHURLREQ_NODE_OAUTH_URL;
			} else if (0 == strcmp((char*)name, (char*)CC_OAUTHURLREQ_XML_NODE_SESSION)) {
				// <oauth_session>
				parser->state = CC_OAUTHURLREQ_NODE_OAUTH_SESSION;
			} else if (0 != strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_XML)) {
				parser->state = CC_OAUTHURLREQ_NODE_XML_CHILD;
			}
		} else {
			// 上記以外
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"format error, " HERE);
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
void XMLCALL CC_OAuthUrlReq_EndElement(void *userData, const char *name)
{
	OAUTHURL_PARSER *parser = (OAUTHURL_PARSER*)userData;

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

		if (CC_OAUTHURLREQ_NODE_API_STATUS == parser->state) {
			// <api_status>
			strcpy((char*)parser->urlInfo.apiStatus, (char*)parser->buf);
			parser->state = CC_OAUTHURLREQ_NODE_XML;
		} else if (CC_OAUTHURLREQ_NODE_OAUTH_URL == parser->state) {
			// <oauth_url>
			strcpy((char*)parser->urlInfo.url, (char*)parser->buf);
			parser->state = CC_OAUTHURLREQ_NODE_XML;
		} else if (CC_OAUTHURLREQ_NODE_OAUTH_SESSION == parser->state) {
			// <oauth_session>
			strcpy((char*)parser->urlInfo.sessionId, (char*)parser->buf);
			parser->state = CC_OAUTHURLREQ_NODE_XML;
		} else if (CC_OAUTHURLREQ_NODE_XML_CHILD == parser->state) {
			 parser->state = CC_OAUTHURLREQ_NODE_XML;
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
void XMLCALL CC_OAuthUrlReq_CharacterData(void *userData, const XML_Char *data, INT32 len)
{
	OAUTHURL_PARSER *parser = (OAUTHURL_PARSER*)userData;
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

		if (CC_OAUTHURLREQ_NODE_API_STATUS == parser->state) {
			// <api_status>
			if (CC_CMN_XML_RES_STS_CODE_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_OAUTHURLREQ_NODE_OAUTH_URL == parser->state) {
			// <oauth_url>
			if (CC_OAUTHURLREQ_XML_DATA_URL_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_OAUTHURLREQ_NODE_OAUTH_SESSION == parser->state) {
			// <oauth_session>
			if (CC_OAUTHURLREQ_XML_DATA_SESSION_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		}
	} while (0);

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}
