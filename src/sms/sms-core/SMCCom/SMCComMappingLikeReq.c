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

#define	CC_MAPPINGLIKEREQ_SEND_BODY_SIZE				1024

#define	CC_MAPPINGLIKEREQ_XML_NODE_LIKE_LIST			"like_list"
#define	CC_MAPPINGLIKEREQ_XML_NODE_ITEM					"item"
#define	CC_MAPPINGLIKEREQ_XML_NODE_LIKE_ID				"like_id"
#define	CC_MAPPINGLIKEREQ_XML_NODE_GUID					"guid"
#define	CC_MAPPINGLIKEREQ_XML_NODE_USER_NAME			"user_name"
#define	CC_MAPPINGLIKEREQ_XML_NODE_AVATAR_URL_SMALL		"avatar_url_small"
#define	CC_MAPPINGLIKEREQ_XML_NODE_AVATAR_URL_MEDIUM	"avatar_url_medium"
#define	CC_MAPPINGLIKEREQ_XML_NODE_AVATAR_URL_LARGE		"avatar_url_large"
#define	CC_MAPPINGLIKEREQ_XML_NODE_RGST_DATETM			"rgst_datetm"

#define	CC_MAPPINGLIKEREQ_RES_FILE						"mappinglike.res"

#define	CC_MAPPINGLIKEREQ_XML_DATA_APISTATUS_SIZE		CC_CMN_XML_RES_STS_CODE_SIZE
#define	CC_MAPPINGLIKEREQ_XML_DATA_LIKEID_SIZE			11
#define	CC_MAPPINGLIKEREQ_XML_DATA_GUID_SIZE			CC_CMN_GUID_STR_SIZE
#define	CC_MAPPINGLIKEREQ_XML_DATA_USER_SIZE			SCC_MAX_USERNAME
#define	CC_MAPPINGLIKEREQ_XML_DATA_AVATAR_URL_SIZE		CC_CMN_MAPPING_URL_SIZE
#define	CC_MAPPINGLIKEREQ_XML_DATA_RGSTDATETM_SIZE		CC_CMN_MAPPING_DATE_SIZE
#define	CC_MAPPINGLIKEREQ_XML_DATA_ALTDATETM_SIZE		CC_CMN_MAPPING_DATE_SIZE

// マッピングにLIKEを付けたユーザ取得レスポンスXML情報
typedef struct _MAPPINGLIKEINFO {
	INT32				likeListNum;
	SMMAPPINGLIKEINFO	*likeList;
	Char				*apiStatus;
} MAPPINGLIKEINFO;

// マッピングにLIKEを付けたユーザ取得XMLパーサ
typedef struct _MAPPINGLIKEREQ_PARSER {
	INT32				state;
	Char				*buf;
	MAPPINGLIKEINFO		likeInfo;
	INT32				startPos;
	INT32				likeInfoMaxNum;
	LONG				likeInfoNum;
} MAPPINGLIKEREQ_PARSER;

// mpnglike/req/
enum MappingLikeReqStatus {
	CC_MAPPINGLIKEREQ_NODE_NONE = 0,
	CC_MAPPINGLIKEREQ_NODE_XML,
	CC_MAPPINGLIKEREQ_NODE_XML_CHILD,
	CC_MAPPINGLIKEREQ_NODE_API_STATUS,
	CC_MAPPINGLIKEREQ_NODE_LIKE_LIST,
	CC_MAPPINGLIKEREQ_NODE_LIKE_LIST_CHILD,
	CC_MAPPINGLIKEREQ_NODE_ITEM,
	CC_MAPPINGLIKEREQ_NODE_ITEM_CHILD,
	CC_MAPPINGLIKEREQ_NODE_LIKE_ID,
	CC_MAPPINGLIKEREQ_NODE_GUID,
	CC_MAPPINGLIKEREQ_NODE_USER_NAME,
	CC_MAPPINGLIKEREQ_NODE_AVATAR_URL_SMALL,
	CC_MAPPINGLIKEREQ_NODE_AVATAR_URL_MEDIUM,
	CC_MAPPINGLIKEREQ_NODE_AVATAR_URL_LARGE,
	CC_MAPPINGLIKEREQ_NODE_RGST_DATETM
};


//------------------------------------------------
// 変数定義
//------------------------------------------------
static INT32	CB_Result;				// 処理結果 (コールバック関数用)
static Bool		CB_IsXmlFile;

//------------------------------------------------
// 関数定義
//------------------------------------------------
static void CC_MappingLikeReq_CreateUri(const T_CC_CMN_SMS_API_PRM *param, Char *uri);
static void CC_MappingLikeReq_CreateBody(const T_CC_CMN_SMS_API_PRM *parm, const Char *mappingId, Char *body);
static E_SC_RESULT CC_MappingLikeReq_AnalyzeHttpResp(const Char *body, E_CONTEXT_TYPE contextType, INT32 startPos, INT32 maxCnt, SMMAPPINGLIKEINFO *likeInfo, INT32 *likeInfoNum, LONG *allNum, SMCALOPT *opt, Char *apiStatus);
static E_SC_RESULT CC_MappingLikeReq_XmlParse(const Char* xml, T_CC_CMN_SMS_RESPONSE_INFO* resp_inf, INT32 startPos, INT32 maxCnt, SMMAPPINGLIKEINFO *likeInfo, INT32 *likeInfoNum, LONG *allNum, SMCALOPT *opt);
static void XMLCALL CC_MappingLikeReq_StartElement(void *userData, const char *name, const char **atts);
static void XMLCALL CC_MappingLikeReq_EndElement(void *userData, const char *name);
static void XMLCALL CC_MappingLikeReq_CharacterData(void *userData, const XML_Char *data, INT32 len);

/**
 * @brief マッピングLike取得
 * @param [IN]  smcal       SMCAL
 * @param [IN]  parm        APIパラメータ
 * @param [IN]  mappingId   マッピングID
 * @param [IN]  startPos    取得開始位置
 * @param [IN]  maxCnt      取得最大件数
 * @param [OUT] likeInfo    マッピングにLIKEを付けたユーザ情報
 * @param [OUT] likeInfoNum マッピングにLIKEを付けたユーザ情報取得数
 * @param [OUT] allNum      マッピングにLIKEを付けたユーザ情報総数
 * @param [IN]  recv        センタ受信データ
 * @param [OUT] apiStatus   APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_MappingLikeReq_SendRecv(SMCAL *smcal,
									   const T_CC_CMN_SMS_API_PRM *parm,
									   const Char *mappingId,
									   INT32 startPos,
									   INT32 maxCnt,
									   SMMAPPINGLIKEINFO *likeInfo,
									   INT32 *likeInfoNum,
									   LONG *allNum,
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
		strcat(opt.resFilePath, CC_MAPPINGLIKEREQ_RES_FILE);
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
		data = (Char*)SCC_MALLOC(CC_MAPPINGLIKEREQ_SEND_BODY_SIZE);
		if (NULL == data) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// URI生成
		CC_MappingLikeReq_CreateUri(parm, uri);

		// body生成
		CC_MappingLikeReq_CreateBody(parm, mappingId, data);

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
		ret = CC_MappingLikeReq_AnalyzeHttpResp(body, contextType, startPos, maxCnt, likeInfo, likeInfoNum, allNum, &opt, apiStatus);
		if(e_SC_RESULT_SUCCESS != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MappingLikeReq_AnalyzeHttpResp error, " HERE);
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
void CC_MappingLikeReq_CreateUri(const T_CC_CMN_SMS_API_PRM *parm, Char *uri)
{
	sprintf((char*)uri,
			"%smpnglike/req/",
			parm->ApiPrmNavi.sms_sp_uri
	);
}

/**
 * @brief body生成
 * @param [IN]  parm        APIパラメータ
 * @param [IN]  mappingId   マッピングID
 * @param [OUT] body        body
 * @return 処理結果(E_SC_RESULT)
 */
void CC_MappingLikeReq_CreateBody(const T_CC_CMN_SMS_API_PRM *parm, const Char *mappingId, Char *body)
{
	sprintf((char*)body,
			"term_id=%s&term_sig=%s&guid=%s&user_sig=%s&mpng_id=%s&app_ver=%s",
			parm->ApiPrmMups.new_term_id,
			parm->ApiPrmMups.term_sig,
			parm->ApiPrmMups.guid,
			parm->ApiPrmMups.user_sig,
			mappingId,
			parm->ApiPrmNavi.appVer
	);
}

/**
 * @brief レスポンス解析
 * @param [IN]  body        xmlデータ
 * @param [IN]  contextType コンテキスト
 * @param [IN]  startPos    取得開始位置
 * @param [IN]  maxCnt      取得最大件数
 * @param [OUT] likeInfo    マッピングにLIKEを付けたユーザ情報
 * @param [OUT] likeInfoNum マッピングにLIKEを付けたユーザ情報取得数
 * @param [OUT] allNum      マッピングにLIKEを付けたユーザ情報総数
 * @param [IN]  opt         オプション情報
 * @param [OUT] apiStatus   APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_MappingLikeReq_AnalyzeHttpResp(const Char *body,
											  E_CONTEXT_TYPE contextType,
											  INT32 startPos,
											  INT32 maxCnt,
											  SMMAPPINGLIKEINFO *likeInfo,
											  INT32 *likeInfoNum,
											  LONG *allNum,
											  SMCALOPT *opt,
											  Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	T_CC_CMN_SMS_RESPONSE_INFO	rsp_inf = {};

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		if (E_TEXT_XML != contextType) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"Content-Type error, " HERE);
			ret = e_SC_RESULT_SMS_API_ERR;
			break;
		}

		// XML解析
		rsp_inf.apiSts = apiStatus;
		ret = CC_MappingLikeReq_XmlParse((const Char*)body, &rsp_inf, startPos, maxCnt, likeInfo, likeInfoNum, allNum, opt);
		if(e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MappingLikeReq_XmlParse error, " HERE);
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
 * @brief mpnglike/req/応答メッセージ解析
 * @param [IN] xml          XMLファイルのフルパス
 * @param [IN] resp_inf     CICレスポンス情報
 * @param [IN]  startPos    取得開始位置
 * @param [IN]  maxCnt      取得最大件数
 * @param [OUT] likeInfo    マッピングにLIKEを付けたユーザ情報
 * @param [OUT] likeInfoNum マッピングにLIKEを付けたユーザ情報取得数
 * @param [OUT] allNum      マッピングにLIKEを付けたユーザ情報総数
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_MappingLikeReq_XmlParse(const Char* xml,
									   T_CC_CMN_SMS_RESPONSE_INFO* resp_inf,
									   INT32 startPos,
									   INT32 maxCnt,
									   SMMAPPINGLIKEINFO *likeInfo,
									   INT32 *likeInfoNum,
									   LONG *allNum,
									   SMCALOPT *opt)
{
	E_SC_RESULT		ret = e_SC_RESULT_SUCCESS;
	MAPPINGLIKEREQ_PARSER	likeParser = {};
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
		memset(likeInfo, 0, (sizeof(SMMAPPINGLIKEINFO) * (maxCnt)));
		likeParser.buf = (Char*)SCC_MALLOC(CC_CMN_XML_LARGE_BUF_SIZE + 1);
		if (NULL == likeParser.buf) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			CB_Result = e_SC_RESULT_MALLOC_ERR;
			ret = CB_Result;
			break;
		}
		likeParser.startPos = startPos;
		likeParser.likeInfoMaxNum = maxCnt;
		likeParser.likeInfoNum = -1;
		likeParser.likeInfo.likeListNum = 0;
		likeParser.likeInfo.likeList = likeInfo;
		likeParser.likeInfo.apiStatus = &resp_inf->apiSts[0];
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
		XML_SetUserData(parser, &likeParser);
		XML_SetElementHandler(parser, CC_MappingLikeReq_StartElement, CC_MappingLikeReq_EndElement);
		XML_SetCharacterDataHandler(parser, CC_MappingLikeReq_CharacterData);

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
			*likeInfoNum = likeParser.likeInfo.likeListNum;
			*allNum = likeParser.likeInfoNum + 1;
		}
	} while (0);

	if (NULL != likeParser.buf) {
		SCC_FREE(likeParser.buf);
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
void XMLCALL CC_MappingLikeReq_StartElement(void *userData, const char *name, const char **atts)
{
	MAPPINGLIKEREQ_PARSER *parser = (MAPPINGLIKEREQ_PARSER*)userData;

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
			parser->state = CC_MAPPINGLIKEREQ_NODE_XML;
		}

		// <xml>
		if (CC_MAPPINGLIKEREQ_NODE_XML == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_API_STATUS)) {
				// <api_status>
				parser->state = CC_MAPPINGLIKEREQ_NODE_API_STATUS;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGLIKEREQ_XML_NODE_LIKE_LIST)) {
				// <like_list>
				parser->state = CC_MAPPINGLIKEREQ_NODE_LIKE_LIST;
			} else if (0 != strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_XML)) {
				parser->state = CC_MAPPINGLIKEREQ_NODE_XML_CHILD;
			}
		} else if (CC_MAPPINGLIKEREQ_NODE_LIKE_LIST == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_MAPPINGLIKEREQ_XML_NODE_ITEM)) {
				// <item>
				parser->likeInfoNum++;
				parser->state = CC_MAPPINGLIKEREQ_NODE_ITEM;
			} else {
				parser->state = CC_MAPPINGLIKEREQ_NODE_LIKE_LIST_CHILD;
			}
		} else if (CC_MAPPINGLIKEREQ_NODE_ITEM == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_MAPPINGLIKEREQ_XML_NODE_LIKE_ID)) {
				// <like_id>
				parser->state = CC_MAPPINGLIKEREQ_NODE_LIKE_ID;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGLIKEREQ_XML_NODE_GUID)) {
				// <guid>
				parser->state = CC_MAPPINGLIKEREQ_NODE_GUID;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGLIKEREQ_XML_NODE_USER_NAME)) {
				// <user_name>
				parser->state = CC_MAPPINGLIKEREQ_NODE_USER_NAME;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGLIKEREQ_XML_NODE_AVATAR_URL_SMALL)) {
				// <avatar_url_small>
				parser->state = CC_MAPPINGLIKEREQ_NODE_AVATAR_URL_SMALL;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGLIKEREQ_XML_NODE_AVATAR_URL_MEDIUM)) {
				// <avatar_url_medium>
				parser->state = CC_MAPPINGLIKEREQ_NODE_AVATAR_URL_MEDIUM;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGLIKEREQ_XML_NODE_AVATAR_URL_LARGE)) {
				// <avatar_url_large>
				parser->state = CC_MAPPINGLIKEREQ_NODE_AVATAR_URL_LARGE;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGLIKEREQ_XML_NODE_RGST_DATETM)) {
				// <rgst_datetm>
				parser->state = CC_MAPPINGLIKEREQ_NODE_RGST_DATETM;
			} else {
				parser->state = CC_MAPPINGLIKEREQ_NODE_ITEM_CHILD;
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
void XMLCALL CC_MappingLikeReq_EndElement(void *userData, const char *name)
{
	MAPPINGLIKEREQ_PARSER	*parser = (MAPPINGLIKEREQ_PARSER*)userData;
	SMMAPPINGLIKEINFO	*likeInfo = NULL;

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

		likeInfo = &parser->likeInfo.likeList[parser->likeInfo.likeListNum];

		if (CC_MAPPINGLIKEREQ_NODE_API_STATUS == parser->state) {
			// <api_status>
			strcpy((char*)parser->likeInfo.apiStatus, (char*)parser->buf);
			parser->state = CC_MAPPINGLIKEREQ_NODE_XML;
		} else if (CC_MAPPINGLIKEREQ_NODE_LIKE_LIST == parser->state) {
			// <like_list>
			parser->state = CC_MAPPINGLIKEREQ_NODE_XML;
		} else if (CC_MAPPINGLIKEREQ_NODE_ITEM == parser->state) {
			// <item>
			parser->state = CC_MAPPINGLIKEREQ_NODE_LIKE_LIST;
			if (parser->startPos > parser->likeInfoNum) {
				// 取得対象外
				break;
			}
			if (parser->likeInfoMaxNum <= parser->likeInfo.likeListNum) {
				break;
			}
			parser->likeInfo.likeListNum++;
		} else if (CC_MAPPINGLIKEREQ_NODE_LIKE_ID == parser->state) {
			// <like_id>
			parser->state = CC_MAPPINGLIKEREQ_NODE_ITEM;
			if (parser->startPos > parser->likeInfoNum) {
				// 取得対象外
				break;
			}
			if (parser->likeInfoMaxNum <= parser->likeInfo.likeListNum) {
				break;
			}
			likeInfo->likeId = atoi((char*)parser->buf);
		} else if (CC_MAPPINGLIKEREQ_NODE_GUID == parser->state) {
			// <guid>
			parser->state = CC_MAPPINGLIKEREQ_NODE_ITEM;
			if (parser->startPos > parser->likeInfoNum) {
				// 取得対象外
				break;
			}
			if (parser->likeInfoMaxNum <= parser->likeInfo.likeListNum) {
				break;
			}
			strcpy((char*)likeInfo->guid, (char*)parser->buf);
		} else if (CC_MAPPINGLIKEREQ_NODE_USER_NAME == parser->state) {
			// <user_name>
			parser->state = CC_MAPPINGLIKEREQ_NODE_ITEM;
			if (parser->startPos > parser->likeInfoNum) {
				// 取得対象外
				break;
			}
			if (parser->likeInfoMaxNum <= parser->likeInfo.likeListNum) {
				break;
			}
			strcpy((char*)likeInfo->user, (char*)parser->buf);
		} else if (CC_MAPPINGLIKEREQ_NODE_AVATAR_URL_SMALL == parser->state) {
			// <avatar_url_small>
			parser->state = CC_MAPPINGLIKEREQ_NODE_ITEM;
			if (parser->startPos > parser->likeInfoNum) {
				// 取得対象外
				break;
			}
			if (parser->likeInfoMaxNum <= parser->likeInfo.likeListNum) {
				break;
			}
			strcpy((char*)likeInfo->avaSmlUrl, (char*)parser->buf);
		} else if (CC_MAPPINGLIKEREQ_NODE_AVATAR_URL_MEDIUM == parser->state) {
			// <avatar_url_medium>
			parser->state = CC_MAPPINGLIKEREQ_NODE_ITEM;
			if (parser->startPos > parser->likeInfoNum) {
				// 取得対象外
				break;
			}
			if (parser->likeInfoMaxNum <= parser->likeInfo.likeListNum) {
				break;
			}
			strcpy((char*)likeInfo->avaMidUrl, (char*)parser->buf);
		} else if (CC_MAPPINGLIKEREQ_NODE_AVATAR_URL_LARGE == parser->state) {
			// <avatar_url_large>
			parser->state = CC_MAPPINGLIKEREQ_NODE_ITEM;
			if (parser->startPos > parser->likeInfoNum) {
				// 取得対象外
				break;
			}
			if (parser->likeInfoMaxNum <= parser->likeInfo.likeListNum) {
				break;
			}
			strcpy((char*)likeInfo->avaLrgUrl, (char*)parser->buf);
		} else if (CC_MAPPINGLIKEREQ_NODE_RGST_DATETM == parser->state) {
			// <rgst_datetm>
			parser->state = CC_MAPPINGLIKEREQ_NODE_ITEM;
			if (parser->startPos > parser->likeInfoNum) {
				// 取得対象外
				break;
			}
			if (parser->likeInfoMaxNum <= parser->likeInfo.likeListNum) {
				break;
			}
			likeInfo->rgstDatetm = atoi((char*)parser->buf);
		} else if (CC_MAPPINGLIKEREQ_NODE_XML_CHILD == parser->state) {
			parser->state = CC_MAPPINGLIKEREQ_NODE_XML;
		} else if (CC_MAPPINGLIKEREQ_NODE_LIKE_LIST_CHILD == parser->state) {
			parser->state = CC_MAPPINGLIKEREQ_NODE_LIKE_LIST;
		} else if (CC_MAPPINGLIKEREQ_NODE_ITEM_CHILD == parser->state) {
			parser->state = CC_MAPPINGLIKEREQ_NODE_ITEM;
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
void XMLCALL CC_MappingLikeReq_CharacterData(void *userData, const XML_Char *data, INT32 len)
{
	MAPPINGLIKEREQ_PARSER *parser = (MAPPINGLIKEREQ_PARSER*)userData;
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

		if (CC_MAPPINGLIKEREQ_NODE_API_STATUS == parser->state) {
			// <api_status>
			if (CC_MAPPINGLIKEREQ_XML_DATA_APISTATUS_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		}

		if (parser->startPos > parser->likeInfoNum) {
			// 取得対象外
			break;
		}
		if (parser->likeInfoMaxNum <= parser->likeInfo.likeListNum) {
			break;
		}

		if (CC_MAPPINGLIKEREQ_NODE_ITEM == parser->state) {
			// <item>
		} else if (CC_MAPPINGLIKEREQ_NODE_LIKE_ID == parser->state) {
			// <like_id>
			if (CC_MAPPINGLIKEREQ_XML_DATA_LIKEID_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_MAPPINGLIKEREQ_NODE_GUID == parser->state) {
			// <guid>
			if (CC_MAPPINGLIKEREQ_XML_DATA_GUID_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_MAPPINGLIKEREQ_NODE_USER_NAME == parser->state) {
			// <user_name>
			if (CC_MAPPINGLIKEREQ_XML_DATA_USER_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_MAPPINGLIKEREQ_NODE_AVATAR_URL_SMALL == parser->state) {
			// <avatar_url_small>
			if (CC_MAPPINGLIKEREQ_XML_DATA_AVATAR_URL_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_MAPPINGLIKEREQ_NODE_AVATAR_URL_MEDIUM == parser->state) {
			// <avatar_url_medium>
			if (CC_MAPPINGLIKEREQ_XML_DATA_AVATAR_URL_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_MAPPINGLIKEREQ_NODE_AVATAR_URL_LARGE == parser->state) {
			// <avatar_url_large>
			if (CC_MAPPINGLIKEREQ_XML_DATA_AVATAR_URL_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_MAPPINGLIKEREQ_NODE_RGST_DATETM == parser->state) {
			// <rgst_datetm>
			if (CC_MAPPINGLIKEREQ_XML_DATA_RGSTDATETM_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		}
	} while (0);

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}
