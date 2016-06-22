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

#define	CC_GEMSPOTKWDSRCH_SEND_BODY_SIZE			2048

#define	CC_GEMSPOTKWDSRCH_XML_NODE_ARRAY_ITEM		"array_item"
#define	CC_GEMSPOTKWDSRCH_XML_NODE_ALL_CNT			"all_cnt"
#define	CC_GEMSPOTKWDSRCH_XML_NODE_RET_CNT			"ret_cnt"
#define	CC_GEMSPOTKWDSRCH_XML_NODE_SPOT_ID			"spot_id"
#define	CC_GEMSPOTKWDSRCH_XML_NODE_SPOT_NAME		"spot_name"
#define	CC_GEMSPOTKWDSRCH_XML_NODE_SPOT_LAT			"spot_lat"
#define	CC_GEMSPOTKWDSRCH_XML_NODE_SPOT_LON			"spot_lng"
#define	CC_GEMSPOTKWDSRCH_XML_NODE_CLS1_ID			"genre_cls1"
#define	CC_GEMSPOTKWDSRCH_XML_NODE_CLS1_NAME		"genre_cls1_name"
#define	CC_GEMSPOTKWDSRCH_XML_NODE_CLS2_ID			"genre_cls2"
#define	CC_GEMSPOTKWDSRCH_XML_NODE_CLS2_NAME		"genre_cls2_name"

#define	CC_GEMSPOTKWDSRCH_RES_FILE					"gemspotkwd.srch"

#define	CC_GEMSPOTKWDSRCH_XML_DATA_APISTATUS_SIZE	CC_CMN_XML_RES_STS_CODE_SIZE
#define	CC_GEMSPOTKWDSRCH_XML_DATA_ALL_CNT_SIZE		11
#define	CC_GEMSPOTKWDSRCH_XML_DATA_SPOT_ID_SIZE		CC_CMN_GEMSPOT_SPOT_ID
#define	CC_GEMSPOTKWDSRCH_XML_DATA_SPOT_NAME_SIZE	CC_CMN_GEMSPOT_SPOT_NAME
#define	CC_GEMSPOTKWDSRCH_XML_DATA_SPOT_LAT_SIZE	20
#define	CC_GEMSPOTKWDSRCH_XML_DATA_SPOT_LON_SIZE	20
#define	CC_GEMSPOTKWDSRCH_XML_DATA_CLS1_ID_SIZE		5
#define	CC_GEMSPOTKWDSRCH_XML_DATA_CLS1_NAME_SIZE	CC_CMN_GEMSPOT_CLS1_NAME
#define	CC_GEMSPOTKWDSRCH_XML_DATA_CLS2_ID_SIZE		5
#define	CC_GEMSPOTKWDSRCH_XML_DATA_CLS2_NAME_SIZE	CC_CMN_GEMSPOT_CLS2_NAME

// GEM SPOT検索レスポンスXML情報
typedef struct _GEMSPOT {
	INT32			gemListNum;
	INT64			allNum;
	SMGEMSPOT		*gemList;
	INT32			*status;
	Char			*apiStatus;
} GEMSPOT;

// GEM SPOT検索XMLパーサ
typedef struct _GEMSPOTKWDSRCH_PARSER {
	INT32			state;
	Char			*buf;
	GEMSPOT			gemSpot;
	INT32			gemSpotMaxNum;
} GEMSPOTKWDSRCH_PARSER;

// Gemspot.keyword.srch
enum GemSpotKwdSrchStates {
	CC_GEMSPOTKWDSRCH_NODE_NONE = 0,
	CC_GEMSPOTKWDSRCH_NODE_ELGG,
	CC_GEMSPOTKWDSRCH_NODE_ELGG_CHILD,
	CC_GEMSPOTKWDSRCH_NODE_STATUS,
	CC_GEMSPOTKWDSRCH_NODE_RESULT,
	CC_GEMSPOTKWDSRCH_NODE_RESULT_CHILD,
	CC_GEMSPOTKWDSRCH_NODE_API_STATUS,
	CC_GEMSPOTKWDSRCH_NODE_ARRAY_ITEM,
	CC_GEMSPOTKWDSRCH_NODE_ARRAY_ITEM_CHILD,
	CC_GEMSPOTKWDSRCH_NODE_ALL_CNT,
	CC_GEMSPOTKWDSRCH_NODE_RET_CNT,
	CC_GEMSPOTKWDSRCH_NODE_SPOT_ID,
	CC_GEMSPOTKWDSRCH_NODE_SPOT_NAME,
	CC_GEMSPOTKWDSRCH_NODE_SPOT_LAT,
	CC_GEMSPOTKWDSRCH_NODE_SPOT_LON,
	CC_GEMSPOTKWDSRCH_NODE_CLS1_ID,
	CC_GEMSPOTKWDSRCH_NODE_CLS1_NAME,
	CC_GEMSPOTKWDSRCH_NODE_CLS2_ID,
	CC_GEMSPOTKWDSRCH_NODE_CLS2_NAME
};


//------------------------------------------------
// 変数定義
//------------------------------------------------
static INT32	CB_Result;				// 処理結果 (コールバック関数用)
static Bool		CB_IsXmlFile;

//------------------------------------------------
// 関数定義
//------------------------------------------------
static void CC_GemspotKwdSearch_CreateUri(const T_CC_CMN_SMS_API_PRM *param, Char *uri);
static E_SC_RESULT CC_GemspotKwdSearch_CreateBody(const T_CC_CMN_SMS_API_PRM *parm, const SMGEMSPOTKWDSEARCH *gemSpotSerch, Char *body);
static E_SC_RESULT CC_GemspotKwdSearch_AnalyzeHttpResp(const Char *body, E_CONTEXT_TYPE contextType, INT32 gemSpotMaxNum, SMGEMSPOT *gemSpot, INT32 *gemSpotNum, INT64 *gemSpotAllNum, SMCALOPT *opt, Char *apiStatus);
static E_SC_RESULT CC_GemspotKwdSearch_XmlParse(const Char* xml, T_CC_CMN_SMS_RESPONSE_INFO* resp_inf, INT32 gemSpotMaxNum, SMGEMSPOT* gem, INT32 *gemNum, INT64 *gemSpotAllNum, SMCALOPT *opt);
static void XMLCALL CC_GemspotKwdSearch_StartElement(void *userData, const char *name, const char **atts);
static void XMLCALL CC_GemspotKwdSearch_EndElement(void *userData, const char *name);
static void XMLCALL CC_GemspotKwdSearch_CharacterData(void *userData, const XML_Char *data, INT32 len);

/**
 * @brief GEM SPOT検索
 * @param [IN]  smcal       SMCAL
 * @param [IN]  parm        APIパラメータ
 * @param [OUT] gemSpotSerch    GEM SPOT検索情報
 * @param [OUT] gemSpot     GEM SPOT検索結果
 * @param [OUT] gemSpotNum  GEM SPOT検索結果件数
 * @param [IN]  recv        センタ受信データ
 * @param [IN]  recvBufSize センタ受信データバッファサイズ
 * @param [OUT] apiStatus   APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GemspotKwdSearch_SendRecv(SMCAL *smcal,
										 const T_CC_CMN_SMS_API_PRM *parm,
										 const SMGEMSPOTKWDSEARCH *gemSpotSerch,
										 SMGEMSPOT *gemSpot,
										 INT32 *gemSpotNum,
										 INT64 *gemSpotAllNum,
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
		strcat(opt.resFilePath, CC_GEMSPOTKWDSRCH_RES_FILE);
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
		data = (Char*)SCC_MALLOC(CC_GEMSPOTKWDSRCH_SEND_BODY_SIZE);
		if (NULL == data) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// URI生成
		CC_GemspotKwdSearch_CreateUri(parm, uri);

		// body生成
		ret = CC_GemspotKwdSearch_CreateBody(parm, gemSpotSerch, data);
		if(e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GemspotKwdSearch_CreateBody error, " HERE);
			break;
		}

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
		ret = CC_GemspotKwdSearch_AnalyzeHttpResp(body, contextType, gemSpotSerch->maxCnt, gemSpot, gemSpotNum, gemSpotAllNum, &opt, apiStatus);
		if(e_SC_RESULT_SUCCESS != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GemspotKwdSearch_AnalyzeHttpResp error, " HERE);
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
void CC_GemspotKwdSearch_CreateUri(const T_CC_CMN_SMS_API_PRM *parm, Char *uri)
{
	sprintf((char*)uri,
			"%s\?method=Gemspot.keyword.srch",
			parm->ApiPrmNavi.common_uri);
}

/**
 * @brief body生成
 * @param [IN]  parm        APIパラメータ
 * @param [IN/OUT] gemSpotSerch GEM SPOT検索条件/結果
 * @param [OUT] body        body
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GemspotKwdSearch_CreateBody(const T_CC_CMN_SMS_API_PRM *parm, const SMGEMSPOTKWDSEARCH *gemSpotSerch, Char *body)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	Char	*keyword = NULL;
	INT32	keywordLen = 0;

	do {
		sprintf((char*)body,
				"term_id=%s&term_sig=%s&guid=%s&user_sig=%s&start_position=%lld&max_cnt=%d",
				parm->ApiPrmMups.new_term_id,
				parm->ApiPrmMups.term_sig,
				parm->ApiPrmMups.guid,
				parm->ApiPrmMups.user_sig,
				gemSpotSerch->startPos,
				gemSpotSerch->maxCnt
		);
		if ((NULL != gemSpotSerch->keyword) && (EOS != gemSpotSerch->keyword[0])) {
			// メモリ確保
			keywordLen = (SCC_MAX_KEYWORD * 3);
			keyword = (Char*)SCC_MALLOC(keywordLen);
			if (NULL == keyword) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error " HERE);
				ret = e_SC_RESULT_MALLOC_ERR;
				break;
			}
			memset(keyword, 0, keywordLen);

			// URLエンコード
			SC_CAL_UrlEncode(gemSpotSerch->keyword, keywordLen, keyword);

			sprintf((char*)&body[strlen((char*)body)],
					"&keyword=%s",
					keyword
			);
		}
		if (0 < gemSpotSerch->radius) {
			sprintf((char*)&body[strlen((char*)body)],
					"&cent_lat=%f&cent_lng=%f&radius=%d&sort=%d",
					gemSpotSerch->lat,
					gemSpotSerch->lon,
					gemSpotSerch->radius,
					gemSpotSerch->sort
			);
		} else {
			sprintf((char*)&body[strlen((char*)body)],
					"&cent_lat=&cent_lng=&radius=&sort="
			);
		}
	} while (0);

	// メモリ解放
	if (NULL != keyword) {
		SCC_FREE(keyword);
	}

	return (ret);
}

/**
 * @brief レスポンス解析
 * @param [IN]  body        xmlデータ
 * @param [IN]  contextType コンテキスト
 * @param [OUT] gemSpot     GEM SPOT検索結果
 * @param [OUT] gemSpotNum  GEM SPOT検索結果件数
 * @param [IN]  opt         オプション情報
 * @param [OUT] apiStatus   APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GemspotKwdSearch_AnalyzeHttpResp(const Char *body,
												E_CONTEXT_TYPE contextType,
												INT32 gemSpotMaxNum,
												SMGEMSPOT *gemSpot,
												INT32 *gemSpotNum,
												INT64 *gemSpotAllNum,
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
		ret = CC_GemspotKwdSearch_XmlParse((const Char*)body, &rsp_inf, gemSpotMaxNum, gemSpot, gemSpotNum, gemSpotAllNum, opt);
		if(e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GemspotKwdSearch_XmlParse error, " HERE);
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
 * @brief Gemspot.keyword.srch応答メッセージ解析
 * @param [IN] xml      XMLファイルのフルパス
 * @param [IN] resp_inf CICレスポンス情報
 * @param [OUT] gem     GEM情報格納領域
 * @param [IN/OUT] gem  GEM情報格納領域数/GEM情報数
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GemspotKwdSearch_XmlParse(const Char* xml,
										 T_CC_CMN_SMS_RESPONSE_INFO* resp_inf,
										 INT32 gemSpotMaxNum,
										 SMGEMSPOT* gem,
										 INT32 *gemNum,
										 INT64 *gemSpotAllNum,
										 SMCALOPT *opt)
{
	E_SC_RESULT		ret = e_SC_RESULT_SUCCESS;
	GEMSPOTKWDSRCH_PARSER	gemspotParser = {};
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
		memset(gem, 0, (sizeof(SMGEMSPOT) * (gemSpotMaxNum)));
		gemspotParser.buf = (Char*)SCC_MALLOC(CC_CMN_XML_BUF_SIZE + 1);
		if (NULL == gemspotParser.buf) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			CB_Result = e_SC_RESULT_MALLOC_ERR;
			ret = CB_Result;
			break;
		}
		gemspotParser.gemSpotMaxNum = gemSpotMaxNum;
		gemspotParser.gemSpot.gemListNum = 0;
		gemspotParser.gemSpot.allNum = 0;
		gemspotParser.gemSpot.gemList = gem;
		gemspotParser.gemSpot.status = &resp_inf->sts;
		gemspotParser.gemSpot.apiStatus = &resp_inf->apiSts[0];
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
		XML_SetUserData(parser, &gemspotParser);
		XML_SetElementHandler(parser, CC_GemspotKwdSearch_StartElement, CC_GemspotKwdSearch_EndElement);
		XML_SetCharacterDataHandler(parser, CC_GemspotKwdSearch_CharacterData);

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
			*gemNum = gemspotParser.gemSpot.gemListNum;
			*gemSpotAllNum = gemspotParser.gemSpot.allNum;
		}
	} while (0);

	if (NULL != gemspotParser.buf) {
		SCC_FREE(gemspotParser.buf);
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
void XMLCALL CC_GemspotKwdSearch_StartElement(void *userData, const char *name, const char **atts)
{
	GEMSPOTKWDSRCH_PARSER *parser = (GEMSPOTKWDSRCH_PARSER*)userData;

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

		if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_ELGG)) {
			// <elgg>
			parser->state = CC_GEMSPOTKWDSRCH_NODE_ELGG;
		}

		// <elgg>
		if (CC_GEMSPOTKWDSRCH_NODE_ELGG == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_STATUS)) {
				// <status>
				parser->state = CC_GEMSPOTKWDSRCH_NODE_STATUS;
			} else if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_RESULT)) {
				// <result>
				parser->state = CC_GEMSPOTKWDSRCH_NODE_RESULT;
			} else if (0 != strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_ELGG)) {
				parser->state = CC_GEMSPOTKWDSRCH_NODE_ELGG_CHILD;
			}
		} else if (CC_GEMSPOTKWDSRCH_NODE_RESULT == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_API_STATUS)) {
				// <api_status>
				parser->state = CC_GEMSPOTKWDSRCH_NODE_API_STATUS;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMSPOTKWDSRCH_XML_NODE_ALL_CNT)) {
				// <all_cnt>
				parser->state = CC_GEMSPOTKWDSRCH_NODE_ALL_CNT;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMSPOTKWDSRCH_XML_NODE_RET_CNT)) {
				// <ret_cnt>
				parser->state = CC_GEMSPOTKWDSRCH_NODE_RET_CNT;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMSPOTKWDSRCH_XML_NODE_ARRAY_ITEM)) {
				// <array_item>
				if (parser->gemSpotMaxNum <= parser->gemSpot.gemListNum) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"array_item maxnum over, " HERE);
					CB_Result = e_SC_RESULT_SMS_API_ERR;
					break;
				}
				parser->state = CC_GEMSPOTKWDSRCH_NODE_ARRAY_ITEM;
			} else {
				parser->state = CC_GEMSPOTKWDSRCH_NODE_RESULT_CHILD;
			}
		} else if (CC_GEMSPOTKWDSRCH_NODE_ARRAY_ITEM == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_GEMSPOTKWDSRCH_XML_NODE_SPOT_ID)) {
				// <spot_id>
				parser->state = CC_GEMSPOTKWDSRCH_NODE_SPOT_ID;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMSPOTKWDSRCH_XML_NODE_SPOT_NAME)) {
				// <spot_name>
				parser->state = CC_GEMSPOTKWDSRCH_NODE_SPOT_NAME;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMSPOTKWDSRCH_XML_NODE_SPOT_LAT)) {
				// <spot_lat>
				parser->state = CC_GEMSPOTKWDSRCH_NODE_SPOT_LAT;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMSPOTKWDSRCH_XML_NODE_SPOT_LON)) {
				// <spot_lng>
				parser->state = CC_GEMSPOTKWDSRCH_NODE_SPOT_LON;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMSPOTKWDSRCH_XML_NODE_CLS1_ID)) {
				// <genre_cls1>
				parser->state = CC_GEMSPOTKWDSRCH_NODE_CLS1_ID;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMSPOTKWDSRCH_XML_NODE_CLS1_NAME)) {
				// <genre_cls1_name>
				parser->state = CC_GEMSPOTKWDSRCH_NODE_CLS1_NAME;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMSPOTKWDSRCH_XML_NODE_CLS2_ID)) {
				// <genre_cls2>
				parser->state = CC_GEMSPOTKWDSRCH_NODE_CLS2_ID;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMSPOTKWDSRCH_XML_NODE_CLS2_NAME)) {
				// <genre_cls2_name>
				parser->state = CC_GEMSPOTKWDSRCH_NODE_CLS2_NAME;
			} else {
				parser->state = CC_GEMSPOTKWDSRCH_NODE_ARRAY_ITEM_CHILD;
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
void XMLCALL CC_GemspotKwdSearch_EndElement(void *userData, const char *name)
{
	GEMSPOTKWDSRCH_PARSER *parser = (GEMSPOTKWDSRCH_PARSER*)userData;
	SMGEMSPOT	*gem = NULL;

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
		if (NULL == parser->gemSpot.gemList) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[parser->gemSpot.gemList], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}
		if (NULL == name) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[name], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}

		gem = &parser->gemSpot.gemList[parser->gemSpot.gemListNum];

		if (CC_GEMSPOTKWDSRCH_NODE_STATUS == parser->state) {
			// <status>
			*(parser->gemSpot.status) = atoi((char*)parser->buf);
			parser->state = CC_GEMSPOTKWDSRCH_NODE_ELGG;
		} else if (CC_GEMSPOTKWDSRCH_NODE_RESULT == parser->state) {
			// <result>
			parser->state = CC_GEMSPOTKWDSRCH_NODE_ELGG;
		} else if (CC_GEMSPOTKWDSRCH_NODE_API_STATUS == parser->state) {
			// <api_status>
			strcpy((char*)parser->gemSpot.apiStatus, (char*)parser->buf);
			parser->state = CC_GEMSPOTKWDSRCH_NODE_RESULT;
		} else if (CC_GEMSPOTKWDSRCH_NODE_ALL_CNT == parser->state) {
			// <all_cnt>
			parser->gemSpot.allNum = (INT64)atoll((char*)parser->buf);
			parser->state = CC_GEMSPOTKWDSRCH_NODE_RESULT;
		} else if (CC_GEMSPOTKWDSRCH_NODE_RET_CNT == parser->state) {
			// <ret_cnt>
			parser->state = CC_GEMSPOTKWDSRCH_NODE_RESULT;
		} else if (CC_GEMSPOTKWDSRCH_NODE_ARRAY_ITEM == parser->state) {
			// <array_item>
			parser->state = CC_GEMSPOTKWDSRCH_NODE_RESULT;
			parser->gemSpot.gemListNum++;
		} else if (CC_GEMSPOTKWDSRCH_NODE_SPOT_ID == parser->state) {
			// <spot_id>
			gem->spotId = (INT64)atoll((char*)parser->buf);
			parser->state = CC_GEMSPOTKWDSRCH_NODE_ARRAY_ITEM;
		} else if (CC_GEMSPOTKWDSRCH_NODE_SPOT_NAME == parser->state) {
			// <spot_name>
			strcpy((char*)gem->spotName, (char*)parser->buf);
			parser->state = CC_GEMSPOTKWDSRCH_NODE_ARRAY_ITEM;
		} else if (CC_GEMSPOTKWDSRCH_NODE_SPOT_LAT == parser->state) {
			// <spot_lat>
			gem->lat = atof((char*)parser->buf);
			parser->state = CC_GEMSPOTKWDSRCH_NODE_ARRAY_ITEM;
		} else if (CC_GEMSPOTKWDSRCH_NODE_SPOT_LON == parser->state) {
			// <spot_lng>
			gem->lon = atof((char*)parser->buf);
			parser->state = CC_GEMSPOTKWDSRCH_NODE_ARRAY_ITEM;
		} else if (CC_GEMSPOTKWDSRCH_NODE_CLS1_ID == parser->state) {
			// <genre_cls1>
			gem->cls1Id = atoi((char*)parser->buf);
			parser->state = CC_GEMSPOTKWDSRCH_NODE_ARRAY_ITEM;
		} else if (CC_GEMSPOTKWDSRCH_NODE_CLS1_NAME == parser->state) {
			// <genre_cls1_name>
			strcpy((char*)gem->cls1Name, (char*)parser->buf);
			parser->state = CC_GEMSPOTKWDSRCH_NODE_ARRAY_ITEM;
		} else if (CC_GEMSPOTKWDSRCH_NODE_CLS2_ID == parser->state) {
			// <genre_cls2>
			gem->cls2Id = atoi((char*)parser->buf);
			parser->state = CC_GEMSPOTKWDSRCH_NODE_ARRAY_ITEM;
		} else if (CC_GEMSPOTKWDSRCH_NODE_CLS2_NAME == parser->state) {
			// <genre_cls2_name>
			strcpy((char*)gem->cls2Name, (char*)parser->buf);
			parser->state = CC_GEMSPOTKWDSRCH_NODE_ARRAY_ITEM;
		} else if (CC_GEMSPOTKWDSRCH_NODE_ELGG_CHILD == parser->state) {
			parser->state = CC_GEMSPOTKWDSRCH_NODE_ELGG;
		} else if (CC_GEMSPOTKWDSRCH_NODE_RESULT_CHILD == parser->state) {
			parser->state = CC_GEMSPOTKWDSRCH_NODE_RESULT;
		} else if (CC_GEMSPOTKWDSRCH_NODE_ARRAY_ITEM_CHILD == parser->state) {
			parser->state = CC_GEMSPOTKWDSRCH_NODE_ARRAY_ITEM;
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
void XMLCALL CC_GemspotKwdSearch_CharacterData(void *userData, const XML_Char *data, INT32 len)
{
	GEMSPOTKWDSRCH_PARSER *parser = (GEMSPOTKWDSRCH_PARSER*)userData;
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

		if (CC_GEMSPOTKWDSRCH_NODE_STATUS == parser->state) {
			// <status>
			if (CC_CMN_XML_RES_STS_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMSPOTKWDSRCH_NODE_API_STATUS == parser->state) {
			// <api_status>
			if (CC_GEMSPOTKWDSRCH_XML_DATA_APISTATUS_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMSPOTKWDSRCH_NODE_ARRAY_ITEM == parser->state) {
			// <array_item>
		} else if (CC_GEMSPOTKWDSRCH_NODE_ALL_CNT == parser->state) {
			// <all_cnt>
			if (CC_GEMSPOTKWDSRCH_XML_DATA_ALL_CNT_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMSPOTKWDSRCH_NODE_RET_CNT == parser->state) {
			// <ret_cnt>
		} else if (CC_GEMSPOTKWDSRCH_NODE_SPOT_ID == parser->state) {
			// <spot_id>
			if (CC_GEMSPOTKWDSRCH_XML_DATA_SPOT_ID_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMSPOTKWDSRCH_NODE_SPOT_NAME == parser->state) {
			// <spot_name>
			if (CC_GEMSPOTKWDSRCH_XML_DATA_SPOT_NAME_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMSPOTKWDSRCH_NODE_SPOT_LAT == parser->state) {
			// <spot_lat>
			if (CC_GEMSPOTKWDSRCH_XML_DATA_SPOT_LAT_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMSPOTKWDSRCH_NODE_SPOT_LON == parser->state) {
			// <spot_lng>
			if (CC_GEMSPOTKWDSRCH_XML_DATA_SPOT_LON_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMSPOTKWDSRCH_NODE_CLS1_ID == parser->state) {
			// <genre_cls1>
			if (CC_GEMSPOTKWDSRCH_XML_DATA_CLS1_ID_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMSPOTKWDSRCH_NODE_CLS1_NAME == parser->state) {
			// <genre_cls1_name>
			if (CC_GEMSPOTKWDSRCH_XML_DATA_CLS1_NAME_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMSPOTKWDSRCH_NODE_CLS2_ID == parser->state) {
			// <genre_cls2>
			if (CC_GEMSPOTKWDSRCH_XML_DATA_CLS2_ID_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMSPOTKWDSRCH_NODE_CLS2_NAME == parser->state) {
			// <genre_cls2_name>
			if (CC_GEMSPOTKWDSRCH_XML_DATA_CLS2_NAME_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		}
	} while (0);

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}
