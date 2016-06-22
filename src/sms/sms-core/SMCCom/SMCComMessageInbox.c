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

#define	CC_PERSONALINFO_READ						"read"
#define	CC_PERSONALINFO_UNREAD						"unread"

#define	CC_PERSONALINFO_SEND_BODY_SIZE				1024

#define	CC_PERSONALINFO_XML_NODE_MSG_LIST			"msg_list"
#define	CC_PERSONALINFO_XML_NODE_ITEM				"item"
#define	CC_PERSONALINFO_XML_NODE_COUNT				"count"
#define	CC_PERSONALINFO_XML_NODE_MSG_GUID			"msg_guid"
#define	CC_PERSONALINFO_XML_NODE_GUID				"guid"
#define	CC_PERSONALINFO_XML_NODE_USER_NAME			"user_name"
#define	CC_PERSONALINFO_XML_NODE_AVATAR_URL_SMALL	"avatar_url_small"
#define	CC_PERSONALINFO_XML_NODE_AVATAR_URL_MEDIUM	"avatar_url_medium"
#define	CC_PERSONALINFO_XML_NODE_AVATAR_URL_LARGE	"avatar_url_large"
#define	CC_PERSONALINFO_XML_NODE_MSG_TITLE			"msg_title"
#define	CC_PERSONALINFO_XML_NODE_MSG_DETAIL			"msg_detail"
#define	CC_PERSONALINFO_XML_NODE_MSG_TIME			"msg_time"
#define	CC_PERSONALINFO_XML_NODE_STATUS				"status"
#define	CC_PERSONALINFO_XML_NODE_INDEX				"index"
#define	CC_PERSONALINFO_XML_NODE_MAPPING_ID			"mpng_id"

#define	CC_PERSONALINFO_RES_FILE					"message.inbox"

#define	CC_PERSONALINFO_XML_DATA_APISTATUS_SIZE		CC_CMN_XML_RES_STS_CODE_SIZE
#define	CC_PERSONALINFO_XML_DATA_COUNT_SIZE		11
#define	CC_PERSONALINFO_XML_DATA_MSG_GUID_SIZE		11
#define	CC_PERSONALINFO_XML_DATA_GUID_SIZE			11
#define	CC_PERSONALINFO_XML_DATA_USER_NAME_SIZE		SCC_MAX_USERNAME
#define	CC_PERSONALINFO_XML_DATA_AVATAR_URL_SIZE	CC_CMN_AVATART_STR_SIZE
#define	CC_PERSONALINFO_XML_DATA_MSG_TITLE_SIZE		SCC_CMN_PRSNLINFO_MSG_TITLE
#define	CC_PERSONALINFO_XML_DATA_MSG_DETAIL_SIZE	SCC_CMN_PRSNLINFO_MSG_DETAIL
#define	CC_PERSONALINFO_XML_DATA_MSG_TIME_SIZE		11
#define	CC_PERSONALINFO_XML_DATA_STATUS_SIZE		7
#define	CC_PERSONALINFO_XML_DATA_INDEX_SIZE			11
#define	CC_PERSONALINFO_XML_DATA_MAPPING_ID_SIZE	CC_CMN_MAPPING_ID_SIZE

// パーソナルお知らせ情報取得レスポンスXML情報
typedef struct _PERSONALINFO {
	INT32			personalInfoNum;
	SMPERSONALINFO	*personalInfo;
	Char			*apiStatus;
	Char			*personalNum;
} PERSONALINFO;

// パーソナルお知らせ情報取得XMLパーサ
typedef struct _PERSONALINFO_PARSER {
	INT32			state;
	Char			*buf;
	PERSONALINFO	personalInfo;
	INT32			personalInfoMaxNum;
} PERSONALINFO_PARSER;

// message.inbox
enum MessageInboxStatus {
	CC_PERSONALINFO_NODE_NONE = 0,
	CC_PERSONALINFO_NODE_XML,
	CC_PERSONALINFO_NODE_XML_CHILD,
	CC_PERSONALINFO_NODE_API_STATUS,
	CC_PERSONALINFO_NODE_MSG_LIST,
	CC_PERSONALINFO_NODE_MSG_LIST_CHILD,
	CC_PERSONALINFO_NODE_ITEM,
	CC_PERSONALINFO_NODE_ITEM_CHILD,
	CC_PERSONALINFO_NODE_COUNT,
	CC_PERSONALINFO_NODE_MSG_GUID,
	CC_PERSONALINFO_NODE_GUID,
	CC_PERSONALINFO_NODE_USER_NAME,
	CC_PERSONALINFO_NODE_AVATAR_URL_SMALL,
	CC_PERSONALINFO_NODE_AVATAR_URL_MEDIUM,
	CC_PERSONALINFO_NODE_AVATAR_URL_LARGE,
	CC_PERSONALINFO_NODE_MSG_TITLE,
	CC_PERSONALINFO_NODE_MSG_DETAIL,
	CC_PERSONALINFO_NODE_MSG_TIME,
	CC_PERSONALINFO_NODE_MSG_STATUS,
	CC_PERSONALINFO_NODE_INDEX,
	CC_PERSONALINFO_NODE_MAPPING_ID
};

//------------------------------------------------
// 変数定義
//------------------------------------------------
static INT32	CB_Result;				// 処理結果 (コールバック関数用)
static Bool		CB_IsXmlFile;

//------------------------------------------------
// 関数定義
//------------------------------------------------
static void CC_MessageInbox_CreateUri(const T_CC_CMN_SMS_API_PRM *param, Char *uri);
static void CC_MessageInbox_CreateBody(const T_CC_CMN_SMS_API_PRM *parm, INT32 offset, INT32 limit, INT32 status, Char *body);
static E_SC_RESULT CC_MessageInbox_AnalyzeHttpResp(const Char *body, E_CONTEXT_TYPE contextType, INT32 limit, SMPERSONALINFO *personalInfo, INT32 *personalInfoNum, Char *personalNum, SMCALOPT *opt, Char *apiStatus);
static E_SC_RESULT CC_MessageInbox_XmlParse(const Char* xml, T_CC_CMN_SMS_RESPONSE_INFO* resp_inf, INT32 limit, SMPERSONALINFO *personalInfo, INT32 *personalInfoNum, Char *personalNum, SMCALOPT *opt);
static void XMLCALL CC_MessageInbox_StartElement(void *userData, const char *name, const char **atts);
static void XMLCALL CC_MessageInbox_EndElement(void *userData, const char *name);
static void XMLCALL CC_MessageInbox_CharacterData(void *userData, const XML_Char *data, INT32 len);

/**
 * @brief パーソナルお知らせ情報取得
 * @param [IN]  smcal           SMCAL
 * @param [IN]  parm            APIパラメータ
 * @param [IN]  offset          取得データの開始位置
 * @param [IN]  limit           最大取得件数
 * @param [IN]  msgStatus       ステータス
 * @param [IN]  personalInfo    パーソナルお知らせ情報
 * @param [IN]  personalInfoNum パーソナルお知らせ情報数
 * @param [IN]  recv            センタ受信データ
 * @param [OUT] apiStatus       APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_MessageInbox_SendRecv(SMCAL *smcal,
									 const T_CC_CMN_SMS_API_PRM *parm,
									 INT32 offset,
									 INT32 limit,
									 INT32 msgStatus,
									 SMPERSONALINFO *personalInfo,
									 INT32 *personalInfoNum,
									 Char *personalNum,
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
		opt.isResOutputFile = true;
		opt.cancel = SCC_IsCancel;
		CC_GetTempDirPath(opt.resFilePath);
		strcat(opt.resFilePath, CC_PERSONALINFO_RES_FILE);
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
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		data = (Char*)SCC_MALLOC(CC_PERSONALINFO_SEND_BODY_SIZE);
		if (NULL == data) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// URI生成
		CC_MessageInbox_CreateUri(parm, uri);

		// body生成
		CC_MessageInbox_CreateBody(parm, offset, limit, msgStatus, data);

		// HTTPデータ送受信
		calRet = SC_CAL_PostRequest(smcal, uri, data, strlen(data), recv, recvBufSize, &recvSize, &opt);
		if(e_SC_CAL_RESULT_SUCCESS != calRet){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SC_CAL_PostRequest error, " HERE);
			ret = ConvertResult(calRet);
			break;
		}

		// HTTPデータ解析
		calRet = SC_CAL_AnalyzeResponseStatus(smcal, recv, recvSize, (const Char**)&body, &bodySize, &contextType, &status);
		if(e_SC_CAL_RESULT_SUCCESS != calRet){
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
		ret = CC_MessageInbox_AnalyzeHttpResp(body, contextType, limit, personalInfo, personalInfoNum, personalNum, &opt, apiStatus);
		if(e_SC_RESULT_SUCCESS != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MessageInbox_AnalyzeHttpResp error, " HERE);
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
void CC_MessageInbox_CreateUri(const T_CC_CMN_SMS_API_PRM *parm, Char *uri)
{
	sprintf((char*)uri,
			"%smessage/inbox/",
			parm->ApiPrmNavi.sms_sp_uri
	);
}

/**
 * @brief body生成
 * @param [IN]  parm    APIパラメータ
 * @param [IN]  offset          取得データの開始位置
 * @param [IN]  limit           最大取得件数
 * @param [IN]  status          ステータス
 * @param [OUT] body     body
 * @return 処理結果(E_SC_RESULT)
 */
void CC_MessageInbox_CreateBody(const T_CC_CMN_SMS_API_PRM *parm, INT32 offset, INT32 limit, INT32 status, Char *body)
{
	sprintf((char*)body,
			"term_id=%s&term_sig=%s&guid=%s&user_sig=%s&offset=%d&limit=%d",
			parm->ApiPrmMups.new_term_id,
			parm->ApiPrmMups.term_sig,
			parm->ApiPrmMups.guid,
			parm->ApiPrmMups.user_sig,
			offset,
			limit
	);

	if (SCC_CMN_PRSNLINFO_STS_READ == status) {
		// 既読のみ
		sprintf((char*)&body[strlen((char*)body)],
				"&status=%s",
				CC_PERSONALINFO_READ
		);
	} else if (SCC_CMN_PRSNLINFO_STS_UNREAD == status) {
		// 未読のみ
		sprintf((char*)&body[strlen((char*)body)],
				"&status=%s",
				CC_PERSONALINFO_UNREAD
		);
	} else {
		// 既読＋未読
		sprintf((char*)&body[strlen((char*)body)],
				"&status="
		);
	}
}

/**
 * @brief レスポンス解析
 * @param [IN]  body        xmlデータ
 * @param [IN]  contextType コンテキスト
 * @param [OUT] personalInfo     パーソナルお知らせ情報取得結果
 * @param [IN]  opt         オプション情報
 * @param [OUT] apiStatus   APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_MessageInbox_AnalyzeHttpResp(const Char *body,
										    E_CONTEXT_TYPE contextType,
										    INT32 limit,
										    SMPERSONALINFO *personalInfo,
										    INT32 *personalInfoNum,
										    Char *personalNum,
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
		ret = CC_MessageInbox_XmlParse((const Char*)body, &rsp_inf, limit, personalInfo, personalInfoNum, personalNum, opt);
		if(e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MessageInbox_XmlParse error, " HERE);
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
 * @brief message/inbox/応答メッセージ解析
 * @param [IN] xml      XMLファイルのフルパス
 * @param [IN] resp_inf CICレスポンス情報
 * @param [IN] personalInfo     パーソナルお知らせ情報
 * @param [IN] personalInfoNum  パーソナルお知らせ情報数
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_MessageInbox_XmlParse(const Char* xml,
									 T_CC_CMN_SMS_RESPONSE_INFO* resp_inf,
									 INT32 limit,
									 SMPERSONALINFO *personalInfo,
									 INT32 *personalInfoNum,
									 Char *personalNum,
									 SMCALOPT *opt)
{
	E_SC_RESULT		ret = e_SC_RESULT_SUCCESS;
	PERSONALINFO_PARSER	personalInfoParser = {};
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
		personalInfoParser.buf = (Char*)SCC_MALLOC(CC_CMN_XML_BUF_SIZE + 1);
		if (NULL == personalInfoParser.buf) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			CB_Result = e_SC_RESULT_MALLOC_ERR;
			ret = CB_Result;
			break;
		}
		personalInfoParser.personalInfoMaxNum = limit;
		personalInfoParser.personalInfo.personalInfoNum = 0;
		personalInfoParser.personalInfo.personalInfo = personalInfo;
		personalInfoParser.personalInfo.apiStatus = &resp_inf->apiSts[0];
		personalInfoParser.personalInfo.personalNum = personalNum;
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
		XML_SetUserData(parser, &personalInfoParser);
		XML_SetElementHandler(parser, CC_MessageInbox_StartElement, CC_MessageInbox_EndElement);
		XML_SetCharacterDataHandler(parser, CC_MessageInbox_CharacterData);

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
		if (e_SC_RESULT_SUCCESS == CB_Result) {
			*personalInfoNum = personalInfoParser.personalInfo.personalInfoNum;
		}
	} while (0);

	if (NULL != personalInfoParser.buf) {
		SCC_FREE(personalInfoParser.buf);
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
void XMLCALL CC_MessageInbox_StartElement(void *userData, const char *name, const char **atts)
{
	PERSONALINFO_PARSER *parser = (PERSONALINFO_PARSER*)userData;

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		if (CC_CMN_RESULT_OK != CB_Result) {
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
			parser->state = CC_PERSONALINFO_NODE_XML;
		}

		// <xml>
		if (CC_PERSONALINFO_NODE_XML == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_API_STATUS)) {
				// <api_status>
				parser->state = CC_PERSONALINFO_NODE_API_STATUS;
			} else if (0 == strcmp((char*)name, (char*)CC_PERSONALINFO_XML_NODE_COUNT)) {
				// <count>
				parser->state = CC_PERSONALINFO_NODE_COUNT;
			} else if (0 == strcmp((char*)name, (char*)CC_PERSONALINFO_XML_NODE_MSG_LIST)) {
				// <msg_list>
				parser->state = CC_PERSONALINFO_NODE_MSG_LIST;
			} else if (0 != strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_XML)) {
				parser->state = CC_PERSONALINFO_NODE_XML_CHILD;
			}
		} else if (CC_PERSONALINFO_NODE_MSG_LIST == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_PERSONALINFO_XML_NODE_ITEM)) {
				// <item>
				if (parser->personalInfoMaxNum <= parser->personalInfo.personalInfoNum) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"item maxnum over, " HERE);
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"maxNum=%d, num=%d, " HERE, parser->personalInfoMaxNum, parser->personalInfo.personalInfoNum);
					CB_Result = e_SC_RESULT_SMS_API_ERR;
					break;
				}
				parser->state = CC_PERSONALINFO_NODE_ITEM;
			} else {
				parser->state = CC_PERSONALINFO_NODE_MSG_LIST_CHILD;
			}
		} else if (CC_PERSONALINFO_NODE_ITEM == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_PERSONALINFO_XML_NODE_MSG_GUID)) {
				// <msg_guid>
				parser->state = CC_PERSONALINFO_NODE_MSG_GUID;
			} else if (0 == strcmp((char*)name, (char*)CC_PERSONALINFO_XML_NODE_GUID)) {
				// <guid>
				parser->state = CC_PERSONALINFO_NODE_GUID;
			} else if (0 == strcmp((char*)name, (char*)CC_PERSONALINFO_XML_NODE_USER_NAME)) {
				// <user_name>
				parser->state = CC_PERSONALINFO_NODE_USER_NAME;
			} else if (0 == strcmp((char*)name, (char*)CC_PERSONALINFO_XML_NODE_AVATAR_URL_SMALL)) {
				// <avatar_url_small>
				parser->state = CC_PERSONALINFO_NODE_AVATAR_URL_SMALL;
			} else if (0 == strcmp((char*)name, (char*)CC_PERSONALINFO_XML_NODE_AVATAR_URL_MEDIUM)) {
				// <avatar_url_medium>
				parser->state = CC_PERSONALINFO_NODE_AVATAR_URL_MEDIUM;
			} else if (0 == strcmp((char*)name, (char*)CC_PERSONALINFO_XML_NODE_AVATAR_URL_LARGE)) {
				// <avatar_url_large>
				parser->state = CC_PERSONALINFO_NODE_AVATAR_URL_LARGE;
			} else if (0 == strcmp((char*)name, (char*)CC_PERSONALINFO_XML_NODE_MSG_TITLE)) {
				// <msg_title>
				parser->state = CC_PERSONALINFO_NODE_MSG_TITLE;
			} else if (0 == strcmp((char*)name, (char*)CC_PERSONALINFO_XML_NODE_MSG_DETAIL)) {
				// <msg_detail>
				parser->state = CC_PERSONALINFO_NODE_MSG_DETAIL;
			} else if (0 == strcmp((char*)name, (char*)CC_PERSONALINFO_XML_NODE_MSG_TIME)) {
				// <msg_time>
				parser->state = CC_PERSONALINFO_NODE_MSG_TIME;
			} else if (0 == strcmp((char*)name, (char*)CC_PERSONALINFO_XML_NODE_STATUS)) {
				// <status>
				parser->state = CC_PERSONALINFO_NODE_MSG_STATUS;
			} else if (0 == strcmp((char*)name, (char*)CC_PERSONALINFO_XML_NODE_INDEX)) {
				// <index>
				parser->state = CC_PERSONALINFO_NODE_INDEX;
			} else if (0 == strcmp((char*)name, (char*)CC_PERSONALINFO_XML_NODE_MAPPING_ID)) {
				// <mapping_id>
				parser->state = CC_PERSONALINFO_NODE_MAPPING_ID;
			} else {
				parser->state = CC_PERSONALINFO_NODE_ITEM_CHILD;
			}
		} else {
			// 上記以外
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"format error[%s][%d], " HERE, name, parser->state);
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
void XMLCALL CC_MessageInbox_EndElement(void *userData, const char *name)
{
	PERSONALINFO_PARSER *parser = (PERSONALINFO_PARSER*)userData;
	SMPERSONALINFO	*personalInfo = NULL;

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		if (CC_CMN_RESULT_OK != CB_Result) {
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
		if (NULL == parser->personalInfo.personalInfo) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[parser->personalInfo.personalInfo], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}
		if (NULL == name) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[name], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}

		personalInfo = &parser->personalInfo.personalInfo[parser->personalInfo.personalInfoNum];

		if (CC_PERSONALINFO_NODE_API_STATUS == parser->state) {
			// <api_status>
			strcpy((char*)parser->personalInfo.apiStatus, (char*)parser->buf);
			parser->state = CC_PERSONALINFO_NODE_XML;
		} else if (CC_PERSONALINFO_NODE_COUNT == parser->state) {
			// <count>
			strcpy((char*)parser->personalInfo.personalNum, (char*)parser->buf);
			parser->state = CC_PERSONALINFO_NODE_XML;
		} else if (CC_PERSONALINFO_NODE_MSG_LIST == parser->state) {
			// <msg_list>
			parser->state = CC_PERSONALINFO_NODE_XML;
		} else if (CC_PERSONALINFO_NODE_ITEM == parser->state) {
			// <item>
			parser->state = CC_PERSONALINFO_NODE_MSG_LIST;
			parser->personalInfo.personalInfoNum++;
		} else if (CC_PERSONALINFO_NODE_MSG_GUID == parser->state) {
			// <msg_guid>
			personalInfo->msgGuid = atoi((char*)parser->buf);
			parser->state = CC_PERSONALINFO_NODE_ITEM;
		} else if (CC_PERSONALINFO_NODE_GUID == parser->state) {
			// <guid>
			strcpy((char*)personalInfo->guid, (char*)parser->buf);
			parser->state = CC_PERSONALINFO_NODE_ITEM;
		} else if (CC_PERSONALINFO_NODE_USER_NAME == parser->state) {
			// <user_name>
			strcpy((char*)personalInfo->userName, (char*)parser->buf);
			parser->state = CC_PERSONALINFO_NODE_ITEM;
		} else if (CC_PERSONALINFO_NODE_AVATAR_URL_SMALL == parser->state) {
			// <avatar_url_small>
			strcpy((char*)personalInfo->avtSmlURL, (char*)parser->buf);
			parser->state = CC_PERSONALINFO_NODE_ITEM;
		} else if (CC_PERSONALINFO_NODE_AVATAR_URL_MEDIUM == parser->state) {
			// <avatar_url_medium>
			strcpy((char*)personalInfo->avtMidURL, (char*)parser->buf);
			parser->state = CC_PERSONALINFO_NODE_ITEM;
		} else if (CC_PERSONALINFO_NODE_AVATAR_URL_LARGE == parser->state) {
			// <avatar_url_large>
			strcpy((char*)personalInfo->avtLrgURL, (char*)parser->buf);
			parser->state = CC_PERSONALINFO_NODE_ITEM;
		} else if (CC_PERSONALINFO_NODE_MSG_TITLE == parser->state) {
			// <msg_title>
			strcpy((char*)personalInfo->msgTitle, (char*)parser->buf);
			parser->state = CC_PERSONALINFO_NODE_ITEM;
		} else if (CC_PERSONALINFO_NODE_MSG_DETAIL == parser->state) {
			// <msg_detail>
			strcpy((char*)personalInfo->msgDetail, (char*)parser->buf);
			parser->state = CC_PERSONALINFO_NODE_ITEM;
		} else if (CC_PERSONALINFO_NODE_MSG_TIME == parser->state) {
			// <msg_time>
			personalInfo->msgTime = atoi(parser->buf);
			parser->state = CC_PERSONALINFO_NODE_ITEM;
		} else if (CC_PERSONALINFO_NODE_MSG_STATUS == parser->state) {
			// <status>
			if (0 == strcmp(CC_PERSONALINFO_READ, parser->buf)) {
				// 既読
				personalInfo->status = SCC_CMN_PRSNLINFO_STS_READ;
			} else {
				// 未読
				personalInfo->status = SCC_CMN_PRSNLINFO_STS_UNREAD;
			}
			parser->state = CC_PERSONALINFO_NODE_ITEM;
		} else if (CC_PERSONALINFO_NODE_INDEX == parser->state) {
			// <index>
			personalInfo->index = atoi((char*)parser->buf);
			parser->state = CC_PERSONALINFO_NODE_ITEM;
		} else if (CC_PERSONALINFO_NODE_MAPPING_ID == parser->state) {
			// <mapping_id>
			strcpy((char*)personalInfo->mappingId, (char*)parser->buf);
			parser->state = CC_PERSONALINFO_NODE_ITEM;
		} else if (CC_PERSONALINFO_NODE_XML_CHILD == parser->state) {
			parser->state = CC_PERSONALINFO_NODE_XML;
		} else if (CC_PERSONALINFO_NODE_MSG_LIST_CHILD == parser->state) {
			parser->state = CC_PERSONALINFO_NODE_MSG_LIST;
		} else if (CC_PERSONALINFO_NODE_ITEM_CHILD == parser->state) {
			parser->state = CC_PERSONALINFO_NODE_ITEM;
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
void XMLCALL CC_MessageInbox_CharacterData(void *userData, const XML_Char *data, INT32 len)
{
	PERSONALINFO_PARSER *parser = (PERSONALINFO_PARSER*)userData;
	//char buf[CC_CMN_XML_PARSE_DATA_SIZE + 1] = {};
	INT32	bufLen = 0;

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		if (CC_CMN_RESULT_OK != CB_Result) {
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

		if (CC_PERSONALINFO_NODE_API_STATUS == parser->state) {
			// <api_status>
			if (CC_PERSONALINFO_XML_DATA_APISTATUS_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_PERSONALINFO_NODE_COUNT == parser->state) {
			// <count>
			if (CC_PERSONALINFO_XML_DATA_COUNT_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_PERSONALINFO_NODE_MSG_GUID == parser->state) {
			// <msg_guid>
			if (CC_PERSONALINFO_XML_DATA_MSG_GUID_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_PERSONALINFO_NODE_GUID == parser->state) {
			// <guid>
			if (CC_PERSONALINFO_XML_DATA_GUID_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_PERSONALINFO_NODE_USER_NAME == parser->state) {
			// <user_name>
			if (CC_PERSONALINFO_XML_DATA_USER_NAME_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_PERSONALINFO_NODE_AVATAR_URL_SMALL == parser->state) {
			// <avatar_url_small>
			if (CC_PERSONALINFO_XML_DATA_AVATAR_URL_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_PERSONALINFO_NODE_AVATAR_URL_MEDIUM == parser->state) {
			// <avatar_url_medium>
			if (CC_PERSONALINFO_XML_DATA_AVATAR_URL_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_PERSONALINFO_NODE_AVATAR_URL_LARGE == parser->state) {
			// <avatar_url_large>
			if (CC_PERSONALINFO_XML_DATA_AVATAR_URL_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_PERSONALINFO_NODE_MSG_TITLE == parser->state) {
			// <msg_title>
			if (CC_PERSONALINFO_XML_DATA_MSG_TITLE_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_PERSONALINFO_NODE_MSG_DETAIL == parser->state) {
			// <msg_detail>
			if (CC_PERSONALINFO_XML_DATA_MSG_DETAIL_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_PERSONALINFO_NODE_MSG_TIME == parser->state) {
			// <msg_time>
			if (CC_PERSONALINFO_XML_DATA_MSG_TIME_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_PERSONALINFO_NODE_MSG_STATUS == parser->state) {
			// <status>
			if (CC_PERSONALINFO_XML_DATA_STATUS_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_PERSONALINFO_NODE_INDEX == parser->state) {
			// <index>
			if (CC_PERSONALINFO_XML_DATA_INDEX_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_PERSONALINFO_NODE_MAPPING_ID == parser->state) {
			// <mapping_id>
			if (CC_PERSONALINFO_XML_DATA_MAPPING_ID_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		}
	} while (0);

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}
