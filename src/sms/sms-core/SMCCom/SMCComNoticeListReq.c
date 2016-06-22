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

#define	CC_NOTICELISTREQ_SEND_BODY_SIZE				1024

#define	CC_NOTICELISTREQ_XML_NODE_COUNT_UNREAD		"count_unread"
#define	CC_NOTICELISTREQ_XML_NODE_NOTICE_LIST		"notice_list"
#define	CC_NOTICELISTREQ_XML_NODE_ITEM				"item"
#define	CC_NOTICELISTREQ_XML_NODE_NOTICE_ID			"notice_id"
#define	CC_NOTICELISTREQ_XML_NODE_NOTICE			"notice"
#define	CC_NOTICELISTREQ_XML_NODE_UNREAD_FLG		"unread_flg"

#define	CC_NOTICELISTREQ_RES_FILE					"noticelist.req"

#define	CC_NOTICELISTREQ_XML_DATA_APISTATUS_SIZE	CC_CMN_XML_RES_STS_CODE_SIZE
#define	CC_NOTICELISTREQ_XML_DATA_COUNT_SIZE		11
#define	CC_NOTICELISTREQ_XML_DATA_NOTICEID_SIZE		11
#define	CC_NOTICELISTREQ_XML_DATA_NOTICE_SIZE		SCC_CMN_NOTICE_LIST_STR_SIZE
#define	CC_NOTICELISTREQ_XML_DATA_UNREAD_FLG_SIZE	2


// お知らせ情報一覧取得レスポンスXML情報
typedef struct _NOTICELISTINFO {
	INT32				noticeListNum;
	SMNOTICEINFO		*noticeList;
	Char				*noticeNum;
	Char				*apiStatus;
} NOTICELISTINFO;

// お知らせ情報一覧取得XMLパーサ
typedef struct _NOTICELISTREQ_PARSER {
	INT32				state;
	Char				*buf;
	NOTICELISTINFO		noticeInfo;
	INT32				noticeListMaxNum;
} NOTICELISTREQ_PARSER;

// noticelist/req/
enum NoticeListReqStatus {
	CC_NOTICELISTREQ_NODE_NONE = 0,
	CC_NOTICELISTREQ_NODE_XML,
	CC_NOTICELISTREQ_NODE_XML_CHILD,
	CC_NOTICELISTREQ_NODE_API_STATUS,
	CC_NOTICELISTREQ_NODE_COUNT_UNREAD,
	CC_NOTICELISTREQ_NODE_NOTICE_LIST,
	CC_NOTICELISTREQ_NODE_NOTICE_LIST_CHILD,
	CC_NOTICELISTREQ_NODE_ITEM,
	CC_NOTICELISTREQ_NODE_ITEM_CHILD,
	CC_NOTICELISTREQ_NODE_NOTICE_ID,
	CC_NOTICELISTREQ_NODE_NOTICE,
	CC_NOTICELISTREQ_NODE_UNREAD_FLG
};


//------------------------------------------------
// 変数定義
//------------------------------------------------
static INT32	CB_Result;				// 処理結果 (コールバック関数用)
static Bool		CB_IsXmlFile;

//------------------------------------------------
// 関数定義
//------------------------------------------------
static void CC_NoticeListReq_CreateUri(const T_CC_CMN_SMS_API_PRM *param, Char *uri);
static void CC_NoticeListReq_CreateBody(const T_CC_CMN_SMS_API_PRM *parm, const Char *lang, INT32 type, Char *body);
static E_SC_RESULT CC_NoticeListReq_AnalyzeHttpResp(const Char *body, E_CONTEXT_TYPE contextType, INT32 limit, SMNOTICEINFO *noticeInfo, INT32 *noticeInfoNum, Char *noticeNum, SMCALOPT *opt, Char *apiStatus);
static E_SC_RESULT CC_NoticeListReq_XmlParse(const Char* xml, T_CC_CMN_SMS_RESPONSE_INFO* resp_inf, INT32 limit, SMNOTICEINFO* noticeInfo, INT32 *noticeInfoNum, Char *noticeNum, SMCALOPT *opt);
static void XMLCALL CC_NoticeListReq_StartElement(void *userData, const char *name, const char **atts);
static void XMLCALL CC_NoticeListReq_EndElement(void *userData, const char *name);
static void XMLCALL CC_NoticeListReq_CharacterData(void *userData, const XML_Char *data, INT32 len);

/**
 * @brief お知らせ情報一覧取得
 * @param [IN]  smcal       SMCAL
 * @param [IN]  parm        APIパラメータ
 * @param [IN]  lang        言語
 * @param [IN]  type        取得種別
 * @param [IN]  limit       取得最大件数
 * @param [OUT] noticeInfo  お知らせ情報一覧
 * @param [OUT] noticeInfoNum  お知らせ情報一覧数
 * @param [IN]  recv        センタ受信データ
 * @param [OUT] apiStatus   APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_NoticeListReq_SendRecv(SMCAL *smcal,
									  const T_CC_CMN_SMS_API_PRM *parm,
									  const Char *lang,
									  INT32 type,
									  INT32 limit,
									  SMNOTICEINFO *noticeInfo,
									  INT32 *noticeInfoNum,
									  Char *noticeNum,
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
		strcat(opt.resFilePath, CC_NOTICELISTREQ_RES_FILE);
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
		data = (Char*)SCC_MALLOC(CC_NOTICELISTREQ_SEND_BODY_SIZE);
		if (NULL == data) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// URI生成
		CC_NoticeListReq_CreateUri(parm, uri);

		// body生成
		CC_NoticeListReq_CreateBody(parm, lang, type, data);

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
		ret = CC_NoticeListReq_AnalyzeHttpResp(body, contextType, limit, noticeInfo, noticeInfoNum, noticeNum, &opt, apiStatus);
		if(e_SC_RESULT_SUCCESS != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_NoticeListReq_AnalyzeHttpResp error, " HERE);
			break;
		}
	} while (0);

	// XMLファイル削除
	remove(opt.resFilePath);

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
void CC_NoticeListReq_CreateUri(const T_CC_CMN_SMS_API_PRM *parm, Char *uri)
{
	sprintf((char*)uri,
			"%snoticelist/req/",
			parm->ApiPrmNavi.sms_sp_uri
	);
}

/**
 * @brief body生成
 * @param [IN]  parm     APIパラメータ
 * @param [IN]  lang     言語
 * @param [IN]  type     取得種別
 * @param [OUT] body     body
 * @return 処理結果(E_SC_RESULT)
 */
void CC_NoticeListReq_CreateBody(const T_CC_CMN_SMS_API_PRM *parm, const Char *lang, INT32 type, Char *body)
{
	sprintf((char*)body,
			"term_id=%s&term_sig=%s&guid=%s&user_sig=%s&notice_target=%s&lang=%s&type=%d",
			parm->ApiPrmMups.new_term_id,
			parm->ApiPrmMups.term_sig,
			parm->ApiPrmMups.guid,
			parm->ApiPrmMups.user_sig,
			SCC_CMN_NOTICE_TARGET,
			lang,
			type
	);
}

/**
 * @brief レスポンス解析
 * @param [IN]  body        xmlデータ
 * @param [IN]  contextType コンテキスト
 * @param [IN]  limit       取得最大件数
 * @param [OUT] noticeInfo     お知らせ情報一覧取得結果
 * @param [OUT] noticeInfoNum  お知らせ情報一覧取得結果件数
 * @param [IN]  opt         オプション情報
 * @param [OUT] apiStatus   APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_NoticeListReq_AnalyzeHttpResp(const Char *body,
											 E_CONTEXT_TYPE contextType,
											 INT32 limit,
											 SMNOTICEINFO *noticeInfo,
											 INT32 *noticeInfoNum,
											 Char *noticeNum,
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
		ret = CC_NoticeListReq_XmlParse((const Char*)body, &rsp_inf, limit, noticeInfo, noticeInfoNum, noticeNum, opt);
		if(e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_NoticeListReq_XmlParse error, " HERE);
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
 * @brief noticelist/req/応答メッセージ解析
 * @param [IN] xml      XMLファイルのフルパス
 * @param [IN] resp_inf CICレスポンス情報
 * @param [OUT] noticeInfo     お知らせ情報格納領域
 * @param [IN/OUT] noticeInfo  お知らせ情報格納領域数/お知らせ情報数
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_NoticeListReq_XmlParse(const Char* xml,
								  T_CC_CMN_SMS_RESPONSE_INFO* resp_inf,
								  INT32 limit,
								  SMNOTICEINFO* noticeInfo,
								  INT32 *noticeInfoNum,
								  Char *noticeNum,
								  SMCALOPT *opt)
{
	E_SC_RESULT		ret = e_SC_RESULT_SUCCESS;
	NOTICELISTREQ_PARSER	noticeListParser = {};
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
		memset(noticeInfo, 0, (sizeof(SMNOTICEINFO) * (limit)));
		noticeListParser.buf = (Char*)SCC_MALLOC(CC_CMN_XML_LARGE_BUF_SIZE + 1);
		if (NULL == noticeListParser.buf) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			CB_Result = e_SC_RESULT_MALLOC_ERR;
			ret = CB_Result;
			break;
		}
		noticeListParser.noticeListMaxNum = limit;
		noticeListParser.noticeInfo.noticeListNum = 0;
		noticeListParser.noticeInfo.noticeList = noticeInfo;
		noticeListParser.noticeInfo.apiStatus = &resp_inf->apiSts[0];
		noticeListParser.noticeInfo.noticeNum = noticeNum;
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
		XML_SetUserData(parser, &noticeListParser);
		XML_SetElementHandler(parser, CC_NoticeListReq_StartElement, CC_NoticeListReq_EndElement);
		XML_SetCharacterDataHandler(parser, CC_NoticeListReq_CharacterData);

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
			*noticeInfoNum = noticeListParser.noticeInfo.noticeListNum;
		}
	} while (0);

	if (NULL != noticeListParser.buf) {
		SCC_FREE(noticeListParser.buf);
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
void XMLCALL CC_NoticeListReq_StartElement(void *userData, const char *name, const char **atts)
{
	NOTICELISTREQ_PARSER *parser = (NOTICELISTREQ_PARSER*)userData;

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
			parser->state = CC_NOTICELISTREQ_NODE_XML;
		}

		// <xml>
		if (CC_NOTICELISTREQ_NODE_XML == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_API_STATUS)) {
				// <api_status>
				parser->state = CC_NOTICELISTREQ_NODE_API_STATUS;
			} else if (0 == strcmp((char*)name, (char*)CC_NOTICELISTREQ_XML_NODE_COUNT_UNREAD)) {
				// <count_unread>
				parser->state = CC_NOTICELISTREQ_NODE_COUNT_UNREAD;
			} else if (0 == strcmp((char*)name, (char*)CC_NOTICELISTREQ_XML_NODE_NOTICE_LIST)) {
				// <notice_list>
				parser->state = CC_NOTICELISTREQ_NODE_NOTICE_LIST;
			} else if (0 != strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_XML)) {
				parser->state = CC_NOTICELISTREQ_NODE_XML_CHILD;
			}
		} else if (CC_NOTICELISTREQ_NODE_NOTICE_LIST == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_NOTICELISTREQ_XML_NODE_ITEM)) {
				// <item>
				if (parser->noticeListMaxNum <= parser->noticeInfo.noticeListNum) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"item maxnum over, " HERE);
					CB_Result = e_SC_RESULT_SMS_API_ERR;
					break;
				}
				parser->state = CC_NOTICELISTREQ_NODE_ITEM;
			} else {
				parser->state = CC_NOTICELISTREQ_NODE_NOTICE_LIST_CHILD;
			}
		} else if (CC_NOTICELISTREQ_NODE_ITEM == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_NOTICELISTREQ_XML_NODE_NOTICE_ID)) {
				// <notice_id>
				parser->state = CC_NOTICELISTREQ_NODE_NOTICE_ID;
			} else if (0 == strcmp((char*)name, (char*)CC_NOTICELISTREQ_XML_NODE_NOTICE)) {
				// <notice>
				parser->state = CC_NOTICELISTREQ_NODE_NOTICE;
			} else if (0 == strcmp((char*)name, (char*)CC_NOTICELISTREQ_XML_NODE_UNREAD_FLG)) {
				// <unread_flg>
				parser->state = CC_NOTICELISTREQ_NODE_UNREAD_FLG;
			} else {
				parser->state = CC_NOTICELISTREQ_NODE_ITEM_CHILD;
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
void XMLCALL CC_NoticeListReq_EndElement(void *userData, const char *name)
{
	NOTICELISTREQ_PARSER *parser = (NOTICELISTREQ_PARSER*)userData;
	SMNOTICEINFO	*noticeInfo = NULL;

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
		if (NULL == parser->noticeInfo.noticeList) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[parser->noticeInfo.noticeList], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}
		if (NULL == name) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[name], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}

		noticeInfo = &parser->noticeInfo.noticeList[parser->noticeInfo.noticeListNum];

		if (CC_NOTICELISTREQ_NODE_API_STATUS == parser->state) {
			// <api_status>
			strcpy((char*)parser->noticeInfo.apiStatus, (char*)parser->buf);
			parser->state = CC_NOTICELISTREQ_NODE_XML;
		} else if (CC_NOTICELISTREQ_NODE_COUNT_UNREAD == parser->state) {
			// <count_unread>
			strcpy((char*)parser->noticeInfo.noticeNum, (char*)parser->buf);
			parser->state = CC_NOTICELISTREQ_NODE_XML;
		} else if (CC_NOTICELISTREQ_NODE_NOTICE_LIST == parser->state) {
			// <notice_list>
			parser->state = CC_NOTICELISTREQ_NODE_XML;
		} else if (CC_NOTICELISTREQ_NODE_ITEM == parser->state) {
			// <item>
			parser->state = CC_NOTICELISTREQ_NODE_NOTICE_LIST;
			parser->noticeInfo.noticeListNum++;
		} else if (CC_NOTICELISTREQ_NODE_NOTICE_ID == parser->state) {
			// <notice_id>
			noticeInfo->noticeId = atoi((char*)parser->buf);
			parser->state = CC_NOTICELISTREQ_NODE_ITEM;
		} else if (CC_NOTICELISTREQ_NODE_NOTICE == parser->state) {
			// <notice>
			strcpy((char*)noticeInfo->notice, (char*)parser->buf);
			parser->state = CC_NOTICELISTREQ_NODE_ITEM;
		} else if (CC_NOTICELISTREQ_NODE_UNREAD_FLG == parser->state) {
			// <unread_flg>
			noticeInfo->unreadFlg =atoi((char*)parser->buf);
			parser->state = CC_NOTICELISTREQ_NODE_ITEM;
		} else if (CC_NOTICELISTREQ_NODE_XML_CHILD == parser->state) {
			parser->state = CC_NOTICELISTREQ_NODE_XML;
		} else if (CC_NOTICELISTREQ_NODE_NOTICE_LIST_CHILD == parser->state) {
			parser->state = CC_NOTICELISTREQ_NODE_NOTICE_LIST;
		} else if (CC_NOTICELISTREQ_NODE_ITEM_CHILD == parser->state) {
			parser->state = CC_NOTICELISTREQ_NODE_ITEM;
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
void XMLCALL CC_NoticeListReq_CharacterData(void *userData, const XML_Char *data, INT32 len)
{
	NOTICELISTREQ_PARSER *parser = (NOTICELISTREQ_PARSER*)userData;
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

		if (CC_NOTICELISTREQ_NODE_API_STATUS == parser->state) {
			// <api_status>
			if (CC_NOTICELISTREQ_XML_DATA_APISTATUS_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_NOTICELISTREQ_NODE_COUNT_UNREAD == parser->state) {
			// <count_unread>
			if (CC_NOTICELISTREQ_XML_DATA_COUNT_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_NOTICELISTREQ_NODE_ITEM == parser->state) {
			// <item>
		} else if (CC_NOTICELISTREQ_NODE_NOTICE_ID == parser->state) {
			// <notice_id>
			if (CC_NOTICELISTREQ_XML_DATA_NOTICEID_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_NOTICELISTREQ_NODE_NOTICE == parser->state) {
			// <notice>
			if (CC_NOTICELISTREQ_XML_DATA_NOTICE_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_NOTICELISTREQ_NODE_UNREAD_FLG == parser->state) {
			// <unread_flg>
			if (CC_NOTICELISTREQ_XML_DATA_UNREAD_FLG_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		}
	} while (0);

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}
