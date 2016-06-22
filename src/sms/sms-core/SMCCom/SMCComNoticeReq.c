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

#define CC_NOTICE_REQ_SEND_BODY_SIZE		256

#define	CC_NOTICEREQ_XML_NODE_NOTICE_ID		"notice_id"
#define	CC_NOTICEREQ_XML_NODE_NOTICE		"notice"

#define	CC_NOTICEREQ_XML_DATA_NOTICE_SIZE	CC_CMN_NOTICE_SIZE

#define	CC_NOTICEREQ_RES_FILE				"notice.info"

// お知らせ情報レスポンスXML情報
typedef struct _NOTICE {
	Char			*apiStatus;
	Char			*notice;
} NOTICE;

// お知らせ情報XMLパーサ
typedef struct _NOTICE_PARSER {
	INT32			state;
	Char			*buf;
	NOTICE			notice;
} NOTICE_PARSER;

enum NOTICEStatus {
	CC_NOTICE_NODE_NONE = 0,
	CC_NOTICE_NODE_XML,
	CC_NOTICE_NODE_XML_CHILD,
	CC_NOTICE_NODE_API_STATUS,
	CC_NOTICE_NODE_NOTICE_ID,
	CC_NOTICE_NODE_NOTICE
};

//------------------------------------------------
// 変数定義
//------------------------------------------------
static INT32	CB_Result;				// 処理結果 (コールバック関数用)
static Bool		CB_IsXmlFile;

//------------------------------------------------
// 関数定義
//------------------------------------------------
static void CC_NoticeReq_CreateUri(const T_CC_CMN_SMS_API_PRM *param, Char *uri);
static void CC_NoticeReq_CreateBody(const T_CC_CMN_SMS_API_PRM *param, INT32 noticeId, Char *body);
static E_SC_RESULT CC_NoticeReq_AnalyzeHttpResp(const Char *body, E_CONTEXT_TYPE contextType, Char *notice, const SMCALOPT *opt, Char *apiStatus);
static E_SC_RESULT CC_NoticeReq_XmlParse(const Char* xml, T_CC_CMN_SMS_RESPONSE_INFO *resp_inf, Char *notice, const SMCALOPT *opt);
static void XMLCALL CC_NoticeReq_StartElement(void *userData, const char *name, const char **atts);
static void XMLCALL CC_NoticeReq_EndElement(void *userData, const char *name);
static void XMLCALL CC_NoticeReq_CharacterData(void *userData, const XML_Char *data, INT32 len);

/**
 * @brief お知らせ情報取得
 * @param [IN] smcal        SMCAL
 * @param [IN] parm         APIパラメータ
 * @param [IN] lang         言語
 * @param [IN] notice       お知らせ情報
 * @param [IN] recv         センタ受信データ
 * @param [OUT] apiStatus   APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_NoticeReq_SendRecv(SMCAL *smcal,
								  const T_CC_CMN_SMS_API_PRM *parm,
								  INT32 noticeId,
								  Char *notice,
								  Char *recv,
								  INT32 recvBufSize,
								  Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_CAL_RESULT	calRet = e_SC_CAL_RESULT_SUCCESS;
	Char	*uri = NULL;
	Char	*data = NULL;
	Char	*body = NULL;
	UINT32	bodySize = 0;
	E_CONTEXT_TYPE	contextType = E_TEXT_XML;
	SMCALOPT	opt = {};
	UINT32	recvSize = 0;
	INT32	status = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// 初期化
		opt.isResOutputFile = true;
		opt.cancel = SCC_IsCancel;
		CC_GetTempDirPath(opt.resFilePath);
		strcat(opt.resFilePath, CC_NOTICEREQ_RES_FILE);
#ifdef SC_CMN_BASIC_AUTH_SMS
		// BASIC認証
		opt.isBasicAuth = true;
		strcpy(opt.basic.basicAuthId, SC_CMN_BASIC_AUTH_ID);
		strcpy(opt.basic.basicAuthPwd, SC_CMN_BASIC_AUTH_PWD);
#endif
		*apiStatus = EOS;

		// メモリ確保
		uri = (Char*)SCC_MALLOC(CC_CMN_URI_STR_MAX_LEN);
		if (NULL == uri) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		data = (Char*)SCC_MALLOC(CC_NOTICE_REQ_SEND_BODY_SIZE);
		if (NULL == data) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = CC_CMN_RESULT_MALLOC_ERR;
			break;
		}

		// URI生成
		CC_NoticeReq_CreateUri(parm, uri);

		// body生成
		CC_NoticeReq_CreateBody(parm, noticeId, data);

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
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SC_CAL_AnalyzeResponseStatusStatus error, " HERE);
				ret = ConvertResult(calRet);
			}
			break;
		}

		// レスポンス解析
		ret = CC_NoticeReq_AnalyzeHttpResp(body, contextType, notice, &opt, apiStatus);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_NoticeReq_AnalyzeHttpResp error, " HERE);
			break;
		}
	} while (0);

	// メモリ解放
	if (NULL != uri) {
		SCC_FREE(uri);
	}

	// XMLファイル削除
	remove(opt.resFilePath);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief URL生成
 * @param [IN]  parm        APIパラメータ
 * @param [OUT] uri         URL
 * @return 処理結果(E_SC_RESULT)
 */
void CC_NoticeReq_CreateUri(const T_CC_CMN_SMS_API_PRM *parm, Char *uri)
{
	sprintf((char*)uri,
			"%snotice/req/",
			parm->ApiPrmNavi.sms_sp_uri
	);
}

/**
 * @brief notice/req/用body生成
 * @param[in]  param    APIパラメータ
 * @param[in]  noticeId お知らせ情報ID
 * @param[out] body     body
*/
void CC_NoticeReq_CreateBody(const T_CC_CMN_SMS_API_PRM *param,
							 INT32 noticeId,
							 Char *body)
{
	sprintf((char*)body,
			"term_id=%s&term_sig=%s&guid=%s&user_sig=%s&notice_id=%d",
			param->ApiPrmMups.new_term_id,
			param->ApiPrmMups.term_sig,
			param->ApiPrmMups.guid,
			param->ApiPrmMups.user_sig,
			noticeId
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
E_SC_RESULT CC_NoticeReq_AnalyzeHttpResp(const Char *body,
										 E_CONTEXT_TYPE contextType,
										 Char *notice,
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

		ret = CC_NoticeReq_XmlParse((const Char*)body, &rsp_inf, notice, opt);
		if(e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_NoticeReq_XmlParse error, " HERE);
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
 * @brief notice/req/応答メッセージ解析
 * @param [IN] xml      XMLファイルのフルパス
 * @param [IN] resp_inf レスポンス情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_NoticeReq_XmlParse(const Char *xml,
								  T_CC_CMN_SMS_RESPONSE_INFO* resp_inf,
								  Char *notice,
								  const SMCALOPT *opt)
{
	E_SC_RESULT		ret = e_SC_RESULT_SUCCESS;
	NOTICE_PARSER	noticeParser = {};
	Char buf[CC_CMN_XML_PARSE_DATA_SIZE + 1] = {};
	XML_Parser parser = NULL;
	INT32	done = 0;
	INT32	len = 0;
	INT32	parsedLen = 0;
	FILE	*fp = NULL;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if ((NULL == opt) || (true != opt->isResOutputFile)) {
			if (NULL == xml) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[xml], " HERE);
				CB_Result = e_SC_RESULT_FAIL;
				ret = CB_Result;
				break;
			}
		}

		// 初期化
		resp_inf->sts = 0;
		noticeParser.buf = (Char*)SCC_MALLOC(CC_CMN_XML_LARGE_BUF_SIZE + 1);
		if (NULL == noticeParser.buf) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			CB_Result = CC_CMN_RESULT_MALLOC_ERR;
			ret = CB_Result;
			break;
		}
		noticeParser.notice.apiStatus = &resp_inf->apiSts[0];
		noticeParser.notice.notice = notice;
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
		XML_SetUserData(parser, &noticeParser);
		XML_SetElementHandler(parser, CC_NoticeReq_StartElement, CC_NoticeReq_EndElement);
		XML_SetCharacterDataHandler(parser, CC_NoticeReq_CharacterData);

		if ((NULL == opt) || (true != opt->isResOutputFile)) {
			CB_IsXmlFile = false;
		} else {
			CB_IsXmlFile = true;
			fp = fopen((char*)opt->resFilePath, "r");
			if (NULL == fp) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"file open error(%d)[%s], " HERE, errno, opt->resFilePath);
				CB_Result = e_SC_RESULT_FILE_ACCESSERR;
				ret = CB_Result;
				break;
			}
		}

		while (!done) {
			if (CC_ISCANCEL()) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"cancel, " HERE);
				CB_Result = e_SC_RESULT_CANCEL;
				ret = CB_Result;
				break;
			}

			if (!CB_IsXmlFile) {
				strncpy((char*)buf, &xml[parsedLen], (sizeof(buf) - 1));
				len = (INT32)strlen(buf);
				parsedLen += len;
				if (strlen(xml) <= parsedLen) {
					done = 1;
				} else {
					done = 0;
				}
			} else {
				len = (INT32)fread(buf, 1, (sizeof(buf) - 1), fp);
				done = (len < (sizeof(buf) - 1));
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

	if (NULL != noticeParser.buf) {
		SCC_FREE(noticeParser.buf);
	}
	if (NULL != fp) {
		fclose(fp);
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
void XMLCALL CC_NoticeReq_StartElement(void *userData, const char *name, const char **atts)
{
	NOTICE_PARSER *parser = (NOTICE_PARSER*)userData;

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
			// <XML>
			parser->state = CC_NOTICE_NODE_XML;
		}

		// <XML>
		if (CC_NOTICE_NODE_XML == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_API_STATUS)) {
				// <api_status>
				parser->state = CC_NOTICE_NODE_API_STATUS;
			} else if (0 == strcmp((char*)name, (char*)CC_NOTICEREQ_XML_NODE_NOTICE_ID)) {
				// <notice_id>
				parser->state = CC_NOTICE_NODE_NOTICE_ID;
			} else if (0 == strcmp((char*)name, (char*)CC_NOTICEREQ_XML_NODE_NOTICE)) {
				// <notice>
				parser->state = CC_NOTICE_NODE_NOTICE;
			} else if (0 != strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_XML)) {
				parser->state = CC_NOTICE_NODE_XML_CHILD;
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
void XMLCALL CC_NoticeReq_EndElement(void *userData, const char *name)
{
	NOTICE_PARSER *parser = (NOTICE_PARSER*)userData;

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

		if (CC_NOTICE_NODE_API_STATUS == parser->state) {
			// <api_status>
			strcpy((char*)parser->notice.apiStatus, (char*)parser->buf);
			parser->state = CC_NOTICE_NODE_XML;
		} else if (CC_NOTICE_NODE_NOTICE_ID == parser->state) {
			// <notice_id>
			parser->state = CC_NOTICE_NODE_XML;
		} else if (CC_NOTICE_NODE_NOTICE == parser->state) {
			// <notice>
			strcpy((char*)parser->notice.notice, (char*)parser->buf);
			parser->state = CC_NOTICE_NODE_XML;
		} else if (CC_NOTICE_NODE_XML_CHILD == parser->state) {
			 parser->state = CC_NOTICE_NODE_XML;
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
void XMLCALL CC_NoticeReq_CharacterData(void *userData, const XML_Char *data, INT32 len)
{
	NOTICE_PARSER *parser = (NOTICE_PARSER*)userData;
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

		if (CC_NOTICE_NODE_API_STATUS == parser->state) {
			// <api_status>
			if (CC_CMN_XML_RES_STS_CODE_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_NOTICE_NODE_NOTICE_ID == parser->state) {
			// <notice_id>
		} else if (CC_NOTICE_NODE_NOTICE == parser->state) {
			// <notice>
			if (CC_NOTICEREQ_XML_DATA_NOTICE_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		}
	} while (0);

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}
