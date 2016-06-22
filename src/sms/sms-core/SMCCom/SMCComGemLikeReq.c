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

#define	CC_GEMLIKEREQ_SEND_BODY_SIZE				1024

#define	CC_GEMLIKEREQ_XML_NODE_LIKE_LIST			"like_list"
#define	CC_GEMLIKEREQ_XML_NODE_ITEM					"item"
#define	CC_GEMLIKEREQ_XML_NODE_LIKE_ID				"like_id"
#define	CC_GEMLIKEREQ_XML_NODE_GUID					"guid"
#define	CC_GEMLIKEREQ_XML_NODE_USER_NAME			"user_name"
#define	CC_GEMLIKEREQ_XML_NODE_AVATAR_URL_SMALL		"avatar_url_small"
#define	CC_GEMLIKEREQ_XML_NODE_AVATAR_URL_MEDIUM	"avatar_url_medium"
#define	CC_GEMLIKEREQ_XML_NODE_AVATAR_URL_LARGE		"avatar_url_large"
#define	CC_GEMLIKEREQ_XML_NODE_PROFILE_URL			"profile_url"
#define	CC_GEMLIKEREQ_XML_NODE_RGST_DATETM			"rgst_datetm"

#define	CC_GEMLIKEREQ_RES_FILE						"gemlike.req"

#define	CC_GEMLIKEREQ_XML_DATA_APISTATUS_SIZE		CC_CMN_XML_RES_STS_CODE_SIZE
#define	CC_GEMLIKEREQ_XML_DATA_LIKEID_SIZE			11
#define	CC_GEMLIKEREQ_XML_DATA_GUID_SIZE			CC_CMN_GUID_STR_SIZE
#define	CC_GEMLIKEREQ_XML_DATA_USER_SIZE			SCC_MAX_USERNAME
#define	CC_GEMLIKEREQ_XML_DATA_AVATAR_URL_SIZE		SCC_MAX_URL
#define	CC_GEMLIKEREQ_XML_DATA_PROFILE_URL_SIZE		SCC_MAX_URL
#define	CC_GEMLIKEREQ_XML_DATA_COMMENT_SIZE			SCC_CMN_GEM_COMMENT
#define	CC_GEMLIKEREQ_XML_DATA_COMMENT_URL_SIZE		SCC_MAX_URL
#define	CC_GEMLIKEREQ_XML_DATA_RGSTDATETM_SIZE		SCC_MAX_GEM_DATETM
#define	CC_GEMLIKEREQ_XML_DATA_ALTDATETM_SIZE		SCC_MAX_GEM_DATETM

// GEM Like取得レスポンスXML情報
typedef struct _GEMLIKEINFO {
	INT32				likeListNum;
	SMGEMLIKEINFO		*likeList;
	INT32				*status;
	Char				*apiStatus;
} GEMLIKEINFO;

// GEM Like取得XMLパーサ
typedef struct _GEMLIKEREQ_PARSER {
	INT32				state;
	Char				*buf;
	GEMLIKEINFO			likeInfo;
	INT32				likeInfoMaxNum;
} GEMLIKEREQ_PARSER;

// gemlike/req/
enum GemLikeReqStatus {
	CC_GEMLIKEREQ_NODE_NONE = 0,
	CC_GEMLIKEREQ_NODE_XML,
	CC_GEMLIKEREQ_NODE_XML_CHILD,
	CC_GEMLIKEREQ_NODE_API_STATUS,
	CC_GEMLIKEREQ_NODE_LIKE_LIST,
	CC_GEMLIKEREQ_NODE_LIKE_LIST_CHILD,
	CC_GEMLIKEREQ_NODE_ITEM,
	CC_GEMLIKEREQ_NODE_ITEM_CHILD,
	CC_GEMLIKEREQ_NODE_LIKE_ID,
	CC_GEMLIKEREQ_NODE_GUID,
	CC_GEMLIKEREQ_NODE_USER_NAME,
	CC_GEMLIKEREQ_NODE_AVATAR_URL_SMALL,
	CC_GEMLIKEREQ_NODE_AVATAR_URL_MEDIUM,
	CC_GEMLIKEREQ_NODE_AVATAR_URL_LARGE,
	CC_GEMLIKEREQ_NODE_PROFILE_URL,
	CC_GEMLIKEREQ_NODE_RGST_DATETM
};


//------------------------------------------------
// 変数定義
//------------------------------------------------
static INT32	CB_Result;				// 処理結果 (コールバック関数用)
static Bool		CB_IsXmlFile;

//------------------------------------------------
// 関数定義
//------------------------------------------------
static void CC_GemLikeReq_CreateUri(const T_CC_CMN_SMS_API_PRM *param, Char *uri);
static void CC_GemLikeReq_CreateBody(const T_CC_CMN_SMS_API_PRM *parm, const Char *gemId, Char *body);
static E_SC_RESULT CC_GemLikeReq_AnalyzeHttpResp(const Char *body, E_CONTEXT_TYPE contextType, INT32 limit, SMGEMLIKEINFO *likeInfo, INT32 *likeInfoNum, SMCALOPT *opt, Char *apiStatus);
static E_SC_RESULT CC_GemLikeReq_XmlParse(const Char* xml, T_CC_CMN_SMS_RESPONSE_INFO* resp_inf, INT32 limit, SMGEMLIKEINFO* like, INT32 *likeNum, SMCALOPT *opt);
static void XMLCALL CC_GemLikeReq_StartElement(void *userData, const char *name, const char **atts);
static void XMLCALL CC_GemLikeReq_EndElement(void *userData, const char *name);
static void XMLCALL CC_GemLikeReq_CharacterData(void *userData, const XML_Char *data, INT32 len);

/**
 * @brief GEM Like取得
 * @param [IN]  smcal       SMCAL
 * @param [IN]  parm        APIパラメータ
 * @param [IN]  gemId       GEM ID
 * @param [IN]  limit       取得最大件数
 * @param [OUT] likeInfo    GEM Like
 * @param [OUT] likeInfoNum GEM Like数
 * @param [IN]  recv        センタ受信データ
 * @param [OUT] apiStatus   APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GemLikeReq_SendRecv(SMCAL *smcal,
									  const T_CC_CMN_SMS_API_PRM *parm,
									  const Char *gemId,
									  INT32 limit,
									  SMGEMLIKEINFO *likeInfo,
									  INT32 *likeInfoNum,
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
		strcat(opt.resFilePath, CC_GEMLIKEREQ_RES_FILE);
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
		data = (Char*)SCC_MALLOC(CC_GEMLIKEREQ_SEND_BODY_SIZE);
		if (NULL == data) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// URI生成
		CC_GemLikeReq_CreateUri(parm, uri);

		// body生成
		CC_GemLikeReq_CreateBody(parm, gemId, data);

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
		ret = CC_GemLikeReq_AnalyzeHttpResp(body, contextType, limit, likeInfo, likeInfoNum, &opt, apiStatus);
		if(e_SC_RESULT_SUCCESS != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GemLikeReq_AnalyzeHttpResp error, " HERE);
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
void CC_GemLikeReq_CreateUri(const T_CC_CMN_SMS_API_PRM *parm, Char *uri)
{
	sprintf((char*)uri,
			"%sgemlike/req/",
			parm->ApiPrmNavi.sms_sp_uri
	);
}

/**
 * @brief body生成
 * @param [IN]  parm     APIパラメータ
 * @param [IN]  gemId    GEM ID
 * @param [OUT] body     body
 * @return 処理結果(E_SC_RESULT)
 */
void CC_GemLikeReq_CreateBody(const T_CC_CMN_SMS_API_PRM *parm, const Char *gemId, Char *body)
{
	sprintf((char*)body,
			"term_id=%s&term_sig=%s&guid=%s&user_sig=%s&gem_id=%s",
			parm->ApiPrmMups.new_term_id,
			parm->ApiPrmMups.term_sig,
			parm->ApiPrmMups.guid,
			parm->ApiPrmMups.user_sig,
			gemId
	);
}

/**
 * @brief レスポンス解析
 * @param [IN]  body        xmlデータ
 * @param [IN]  contextType コンテキスト
 * @param [IN]  limit       取得最大件数
 * @param [OUT] likeInfo     GEM Like取得結果
 * @param [OUT] likeInfoNum  GEM Like取得結果件数
 * @param [IN]  opt         オプション情報
 * @param [OUT] apiStatus   APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GemLikeReq_AnalyzeHttpResp(const Char *body,
										  E_CONTEXT_TYPE contextType,
										  INT32 limit,
										  SMGEMLIKEINFO *likeInfo,
										  INT32 *likeInfoNum,
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
		ret = CC_GemLikeReq_XmlParse((const Char*)body, &rsp_inf, limit, likeInfo, likeInfoNum, opt);
		if(e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GemLikeReq_XmlParse error, " HERE);
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
 * @brief gemlike/req/応答メッセージ解析
 * @param [IN] xml      XMLファイルのフルパス
 * @param [IN] resp_inf CICレスポンス情報
 * @param [OUT] like     GEM情報格納領域
 * @param [IN/OUT] like  GEM情報格納領域数/GEM情報数
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GemLikeReq_XmlParse(const Char* xml,
								   T_CC_CMN_SMS_RESPONSE_INFO* resp_inf,
								   INT32 limit,
								   SMGEMLIKEINFO* like,
								   INT32 *likeNum,
								   SMCALOPT *opt)
{
	E_SC_RESULT		ret = e_SC_RESULT_SUCCESS;
	GEMLIKEREQ_PARSER	gemsrchParser = {};
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
		memset(like, 0, (sizeof(SMGEMLIKEINFO) * (limit)));
		gemsrchParser.buf = (Char*)SCC_MALLOC(CC_CMN_XML_LARGE_BUF_SIZE + 1);
		if (NULL == gemsrchParser.buf) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			CB_Result = e_SC_RESULT_MALLOC_ERR;
			ret = CB_Result;
			break;
		}
		gemsrchParser.likeInfoMaxNum = limit;
		gemsrchParser.likeInfo.likeListNum = 0;
		gemsrchParser.likeInfo.likeList = like;
		gemsrchParser.likeInfo.apiStatus = &resp_inf->apiSts[0];
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
		XML_SetUserData(parser, &gemsrchParser);
		XML_SetElementHandler(parser, CC_GemLikeReq_StartElement, CC_GemLikeReq_EndElement);
		XML_SetCharacterDataHandler(parser, CC_GemLikeReq_CharacterData);

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
			*likeNum = gemsrchParser.likeInfo.likeListNum;
		}
	} while (0);

	if (NULL != gemsrchParser.buf) {
		SCC_FREE(gemsrchParser.buf);
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
void XMLCALL CC_GemLikeReq_StartElement(void *userData, const char *name, const char **atts)
{
	GEMLIKEREQ_PARSER *parser = (GEMLIKEREQ_PARSER*)userData;

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
			parser->state = CC_GEMLIKEREQ_NODE_XML;
		}

		// <xml>
		if (CC_GEMLIKEREQ_NODE_XML == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_API_STATUS)) {
				// <api_status>
				parser->state = CC_GEMLIKEREQ_NODE_API_STATUS;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMLIKEREQ_XML_NODE_LIKE_LIST)) {
				// <like_list>
				parser->state = CC_GEMLIKEREQ_NODE_LIKE_LIST;
			} else if (0 != strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_XML)) {
				parser->state = CC_GEMLIKEREQ_NODE_XML_CHILD;
			}
		} else if (CC_GEMLIKEREQ_NODE_LIKE_LIST == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_GEMLIKEREQ_XML_NODE_ITEM)) {
				// <item>
				if (parser->likeInfoMaxNum <= parser->likeInfo.likeListNum) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"item maxnum over, " HERE);
					CB_Result = e_SC_RESULT_SMS_API_ERR;
					break;
				}
				parser->state = CC_GEMLIKEREQ_NODE_ITEM;
			} else {
				parser->state = CC_GEMLIKEREQ_NODE_LIKE_LIST_CHILD;
			}
		} else if (CC_GEMLIKEREQ_NODE_ITEM == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_GEMLIKEREQ_XML_NODE_LIKE_ID)) {
				// <like_id>
				parser->state = CC_GEMLIKEREQ_NODE_LIKE_ID;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMLIKEREQ_XML_NODE_GUID)) {
				// <guid>
				parser->state = CC_GEMLIKEREQ_NODE_GUID;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMLIKEREQ_XML_NODE_USER_NAME)) {
				// <user_name>
				parser->state = CC_GEMLIKEREQ_NODE_USER_NAME;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMLIKEREQ_XML_NODE_AVATAR_URL_SMALL)) {
				// <avatar_url_small>
				parser->state = CC_GEMLIKEREQ_NODE_AVATAR_URL_SMALL;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMLIKEREQ_XML_NODE_AVATAR_URL_MEDIUM)) {
				// <avatar_url_medium>
				parser->state = CC_GEMLIKEREQ_NODE_AVATAR_URL_MEDIUM;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMLIKEREQ_XML_NODE_AVATAR_URL_LARGE)) {
				// <avatar_url_large>
				parser->state = CC_GEMLIKEREQ_NODE_AVATAR_URL_LARGE;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMLIKEREQ_XML_NODE_PROFILE_URL)) {
				// <profile_url>
				parser->state = CC_GEMLIKEREQ_NODE_PROFILE_URL;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMLIKEREQ_XML_NODE_RGST_DATETM)) {
				// <rgst_datetm>
				parser->state = CC_GEMLIKEREQ_NODE_RGST_DATETM;
			} else {
				parser->state = CC_GEMLIKEREQ_NODE_ITEM_CHILD;
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
void XMLCALL CC_GemLikeReq_EndElement(void *userData, const char *name)
{
	GEMLIKEREQ_PARSER *parser = (GEMLIKEREQ_PARSER*)userData;
	SMGEMLIKEINFO	*like = NULL;

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
		if (NULL == parser->likeInfo.likeList) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[parser->likeInfo.likeList], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}
		if (NULL == name) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[name], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}

		like = &parser->likeInfo.likeList[parser->likeInfo.likeListNum];

		if (CC_GEMLIKEREQ_NODE_API_STATUS == parser->state) {
			// <api_status>
			strcpy((char*)parser->likeInfo.apiStatus, (char*)parser->buf);
			parser->state = CC_GEMLIKEREQ_NODE_XML;
		} else if (CC_GEMLIKEREQ_NODE_LIKE_LIST == parser->state) {
			// <comment_list>
			parser->state = CC_GEMLIKEREQ_NODE_XML;
		} else if (CC_GEMLIKEREQ_NODE_ITEM == parser->state) {
			// <item>
			parser->state = CC_GEMLIKEREQ_NODE_LIKE_LIST;
			parser->likeInfo.likeListNum++;
		} else if (CC_GEMLIKEREQ_NODE_LIKE_ID == parser->state) {
			// <like_id>
			like->likeId = atoi((char*)parser->buf);
			parser->state = CC_GEMLIKEREQ_NODE_ITEM;
		} else if (CC_GEMLIKEREQ_NODE_GUID == parser->state) {
			// <guid>
			strcpy((char*)like->guid, (char*)parser->buf);
			parser->state = CC_GEMLIKEREQ_NODE_ITEM;
		} else if (CC_GEMLIKEREQ_NODE_USER_NAME == parser->state) {
			// <user_name>
			strcpy((char*)like->user, (char*)parser->buf);
			parser->state = CC_GEMLIKEREQ_NODE_ITEM;
		} else if (CC_GEMLIKEREQ_NODE_AVATAR_URL_SMALL == parser->state) {
			// <avatar_url_small>
			strcpy((char*)like->avaSmlUrl, (char*)parser->buf);
			parser->state = CC_GEMLIKEREQ_NODE_ITEM;
		} else if (CC_GEMLIKEREQ_NODE_AVATAR_URL_MEDIUM == parser->state) {
			// <avatar_url_medium>
			strcpy((char*)like->avaMidUrl, (char*)parser->buf);
			parser->state = CC_GEMLIKEREQ_NODE_ITEM;
		} else if (CC_GEMLIKEREQ_NODE_AVATAR_URL_LARGE == parser->state) {
			// <avatar_url_large>
			strcpy((char*)like->avaLrgUrl, (char*)parser->buf);
			parser->state = CC_GEMLIKEREQ_NODE_ITEM;
		} else if (CC_GEMLIKEREQ_NODE_PROFILE_URL == parser->state) {
			// <profile_url>
			strcpy((char*)like->profileUrl, (char*)parser->buf);
			parser->state = CC_GEMLIKEREQ_NODE_ITEM;
		} else if (CC_GEMLIKEREQ_NODE_RGST_DATETM == parser->state) {
			// <rgst_datetm>
			like->rgstDatetm = atoi((char*)parser->buf);
			parser->state = CC_GEMLIKEREQ_NODE_ITEM;
		} else if (CC_GEMLIKEREQ_NODE_XML_CHILD == parser->state) {
			parser->state = CC_GEMLIKEREQ_NODE_XML;
		} else if (CC_GEMLIKEREQ_NODE_LIKE_LIST_CHILD == parser->state) {
			parser->state = CC_GEMLIKEREQ_NODE_LIKE_LIST;
		} else if (CC_GEMLIKEREQ_NODE_ITEM_CHILD == parser->state) {
			parser->state = CC_GEMLIKEREQ_NODE_ITEM;
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
void XMLCALL CC_GemLikeReq_CharacterData(void *userData, const XML_Char *data, INT32 len)
{
	GEMLIKEREQ_PARSER *parser = (GEMLIKEREQ_PARSER*)userData;
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

		if (CC_GEMLIKEREQ_NODE_API_STATUS == parser->state) {
			// <api_status>
			if (CC_GEMLIKEREQ_XML_DATA_APISTATUS_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMLIKEREQ_NODE_ITEM == parser->state) {
			// <item>
		} else if (CC_GEMLIKEREQ_NODE_LIKE_ID == parser->state) {
			// <like_id>
			if (CC_GEMLIKEREQ_XML_DATA_LIKEID_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMLIKEREQ_NODE_GUID == parser->state) {
			// <guid>
			if (CC_GEMLIKEREQ_XML_DATA_GUID_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMLIKEREQ_NODE_USER_NAME == parser->state) {
			// <user_name>
			if (CC_GEMLIKEREQ_XML_DATA_USER_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMLIKEREQ_NODE_AVATAR_URL_SMALL == parser->state) {
			// <avatar_url_small>
			if (CC_GEMLIKEREQ_XML_DATA_AVATAR_URL_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMLIKEREQ_NODE_AVATAR_URL_MEDIUM == parser->state) {
			// <avatar_url_medium>
			if (CC_GEMLIKEREQ_XML_DATA_AVATAR_URL_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMLIKEREQ_NODE_AVATAR_URL_LARGE == parser->state) {
			// <avatar_url_large>
			if (CC_GEMLIKEREQ_XML_DATA_AVATAR_URL_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMLIKEREQ_NODE_PROFILE_URL == parser->state) {
			// <profile_url>
			if (CC_GEMLIKEREQ_XML_DATA_PROFILE_URL_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMLIKEREQ_NODE_RGST_DATETM == parser->state) {
			// <rgst_datetm>
			if (CC_GEMLIKEREQ_XML_DATA_RGSTDATETM_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		}
	} while (0);

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}
