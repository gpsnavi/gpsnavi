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

#define	CC_GEMTIMELINESRCH_SEND_BODY_SIZE				2048

#define	CC_GEMTIMELINESRCH_XML_NODE_GEM_LIST			"gem_list"
#define	CC_GEMTIMELINESRCH_XML_NODE_ITEM				"item"
#define	CC_GEMTIMELINESRCH_XML_NODE_LAT					"lat"
#define	CC_GEMTIMELINESRCH_XML_NODE_LON					"lon"
#define	CC_GEMTIMELINESRCH_XML_NODE_ALT					"alt"
#define	CC_GEMTIMELINESRCH_XML_NODE_GEM_ID				"gem_id"
#define	CC_GEMTIMELINESRCH_XML_NODE_GEM_URL				"gem_url"
#define	CC_GEMTIMELINESRCH_XML_NODE_GEM_PICT			"gem_pict"
#define	CC_GEMTIMELINESRCH_XML_NODE_USER				"user"
#define	CC_GEMTIMELINESRCH_XML_NODE_TEXT				"text"
#define	CC_GEMTIMELINESRCH_XML_NODE_GEM_DATETM			"gem_datetm"
#define	CC_GEMTIMELINESRCH_XML_NODE_RGST_DATETM			"rgst_datetm"
#define	CC_GEMTIMELINESRCH_XML_NODE_ALT_DATETM			"alt_datetm"
#define	CC_GEMTIMELINESRCH_XML_NODE_LAST_DATETM			"last_datetm"
#define	CC_GEMTIMELINESRCH_XML_NODE_LIKE_CNT			"like_cnt"
#define	CC_GEMTIMELINESRCH_XML_NODE_LIKE_FLG			"like_flg"
#define	CC_GEMTIMELINESRCH_XML_NODE_GROUP_ID			"group_id"
#define	CC_GEMTIMELINESRCH_XML_NODE_GROUP_NAME			"group_name"
#define	CC_GEMTIMELINESRCH_XML_NODE_COMMENT_CNT			"comment_cnt"
#define	CC_GEMTIMELINESRCH_XML_NODE_ACCESS				"access"
#define	CC_GEMTIMELINESRCH_XML_NODE_LAST_FLG			"last_flg"
#define	CC_GEMTIMELINESRCH_XML_NODE_GEM_OWNER			"gem_owner"
#define	CC_GEMTIMELINESRCH_XML_NODE_CAN_DELETE			"can_delete"
#define	CC_GEMTIMELINESRCH_XML_NODE_USER_GUID			"user_guid"

#define	CC_GEMTIMELINESRCH_RES_FILE						"mytimeline.srch"

#define	CC_GEMTIMELINESRCH_XML_DATA_LAT_SIZE			20
#define	CC_GEMTIMELINESRCH_XML_DATA_LON_SIZE			20
#define	CC_GEMTIMELINESRCH_XML_DATA_ALT_SIZE			20
#define	CC_GEMTIMELINESRCH_XML_DATA_GEMID_SIZE			SCC_MAX_ID
#define	CC_GEMTIMELINESRCH_XML_DATA_GEMURL_SIZE			SCC_MAX_URL
#define	CC_GEMTIMELINESRCH_XML_DATA_APISTATUS_SIZE		CC_CMN_XML_RES_STS_CODE_SIZE
#define	CC_GEMTIMELINESRCH_XML_DATA_USER_SIZE			SCC_MAX_USERNAME
#define	CC_GEMTIMELINESRCH_XML_DATA_TEXT_SIZE			SCC_MAX_TEXT
#define	CC_GEMTIMELINESRCH_XML_DATA_GEMPICT_SIZE		SCC_MAX_GEM_PICTPATH
#define	CC_GEMTIMELINESRCH_XML_DATA_GEMDATETM_SIZE		SCC_MAX_GEM_DATETM
#define	CC_GEMTIMELINESRCH_XML_DATA_RGSTDATETM_SIZE		SCC_MAX_GEM_DATETM
#define	CC_GEMTIMELINESRCH_XML_DATA_ALTDATETM_SIZE		SCC_MAX_GEM_DATETM
#define	CC_GEMTIMELINESRCH_XML_DATA_LASTDATETM_SIZE		SCC_MAX_GEM_DATETM
#define	CC_GEMTIMELINESRCH_XML_DATA_LIKECNT_SIZE		SCC_MAX_LIKE_CNT
#define	CC_GEMTIMELINESRCH_XML_DATA_LIKEFLG_SIZE		2
#define	CC_GEMTIMELINESRCH_XML_DATA_GROUPID_SIZE		SCC_MAX_GROUPID
#define	CC_GEMTIMELINESRCH_XML_DATA_GROUPNAME_SIZE		SCC_MAX_GROUPNAME
#define	CC_GEMTIMELINESRCH_XML_DATA_COMMENTCNT_SIZE		11
#define	CC_GEMTIMELINESRCH_XML_DATA_ACCESS_SIZE			30
#define	CC_GEMTIMELINESRCH_XML_DATA_LASTFLG_SIZE		2
#define	CC_GEMTIMELINESRCH_XML_DATA_GEM_OWNER_SIZE		2
#define	CC_GEMTIMELINESRCH_XML_DATA_CAN_DELETE_SIZE		2
#define	CC_GEMTIMELINESRCH_XML_DATA_GUID_SIZE			CC_CMN_GUID_STR_SIZE

// GEMタイムライン検索レスポンスXML情報
typedef struct _GEMINFO {
	INT32			gemListNum;
	SMGEMTIMELINEINFO	*gemList;
	INT32			*status;
	Char			*apiStatus;
	INT32			*lastFlg;
} GEMINFO;

// GEMタイムライン検索XMLパーサ
typedef struct _GEMTIMELINESRCH_PARSER {
	INT32			state;
	Char			*buf;
	GEMINFO			gemInfo;
	INT32			gemInfoMaxNum;
} GEMTIMELINESRCH_PARSER;

// mytimeline/srch/
enum GemTimelineSrchStatus {
	CC_GEMTIMELINESRCH_NODE_NONE = 0,
	CC_GEMTIMELINESRCH_NODE_XML,
	CC_GEMTIMELINESRCH_NODE_XML_CHILD,
	CC_GEMTIMELINESRCH_NODE_API_STATUS,
	CC_GEMTIMELINESRCH_NODE_CNT,
	CC_GEMTIMELINESRCH_NODE_LAST_FLG,
	CC_GEMTIMELINESRCH_NODE_GEM_LIST,
	CC_GEMTIMELINESRCH_NODE_GEM_LIST_CHILD,
	CC_GEMTIMELINESRCH_NODE_ITEM,
	CC_GEMTIMELINESRCH_NODE_ITEM_CHILD,
	CC_GEMTIMELINESRCH_NODE_LAT,
	CC_GEMTIMELINESRCH_NODE_LON,
	CC_GEMTIMELINESRCH_NODE_ALT,
	CC_GEMTIMELINESRCH_NODE_GEM_TYPE,
	CC_GEMTIMELINESRCH_NODE_GEM_ID,
	CC_GEMTIMELINESRCH_NODE_GEM_URL,
	CC_GEMTIMELINESRCH_NODE_GEM_PICT,
	CC_GEMTIMELINESRCH_NODE_USER,
	CC_GEMTIMELINESRCH_NODE_TEXT,
	CC_GEMTIMELINESRCH_NODE_GEM_DATETM,
	CC_GEMTIMELINESRCH_NODE_RGST_DATETM,
	CC_GEMTIMELINESRCH_NODE_ALT_DATETM,
	CC_GEMTIMELINESRCH_NODE_LAST_DATETM,
	CC_GEMTIMELINESRCH_NODE_LIKE_CNT,
	CC_GEMTIMELINESRCH_NODE_LIKE_FLG,
	CC_GEMTIMELINESRCH_NODE_GROUP_ID,
	CC_GEMTIMELINESRCH_NODE_GROUP_NAME,
	CC_GEMTIMELINESRCH_NODE_COMMENT_CNT,
	CC_GEMTIMELINESRCH_NODE_ACCESS,
	CC_GEMTIMELINESRCH_NODE_GEM_OWNER,
	CC_GEMTIMELINESRCH_NODE_CAN_DELETE,
	CC_GEMTIMELINESRCH_NODE_USER_GUID
};


//------------------------------------------------
// 変数定義
//------------------------------------------------
static INT32	CB_Result;				// 処理結果 (コールバック関数用)
static Bool		CB_IsXmlFile;

//------------------------------------------------
// 関数定義
//------------------------------------------------
static void CC_GemTimelineSearch_CreateUri(const T_CC_CMN_SMS_API_PRM *param, Char *uri);
static E_SC_RESULT CC_GemTimelineSearch_CreateBody(const T_CC_CMN_SMS_API_PRM *parm, const SMGEMTIMELINESEARCH *gemSerch, Char *body);
static E_SC_RESULT CC_GemTimelineSearch_AnalyzeHttpResp(const Char *body, E_CONTEXT_TYPE contextType, INT32 gemInfoMaxNum, SMGEMTIMELINEINFO *gemInfo, INT32 *gemInfoNum, INT32 *lastFlg, SMCALOPT *opt, Char *apiStatus);
static E_SC_RESULT CC_GemTimelineSearch_XmlParse(const Char* xml, T_CC_CMN_SMS_RESPONSE_INFO* resp_inf, INT32 gemInfoMaxNum, SMGEMTIMELINEINFO* gem, INT32 *gemNum, INT32 *lastFlg, SMCALOPT *opt);
static void XMLCALL CC_GemTimelineSearch_StartElement(void *userData, const char *name, const char **atts);
static void XMLCALL CC_GemTimelineSearch_EndElement(void *userData, const char *name);
static void XMLCALL CC_GemTimelineSearch_CharacterData(void *userData, const XML_Char *data, INT32 len);

/**
 * @brief GEMタイムライン検索
 * @param [IN]  smcal       SMCAL
 * @param [IN]  parm        APIパラメータ
 * @param [OUT] gemSerch    GEMタイムライン検索情報
 * @param [OUT] gemInfo     GEMタイムライン検索結果
 * @param [OUT] gemInfoNum  GEMタイムライン検索結果件数
 * @param [OUT] lastFlg     GEMタイムライン検索結果が最終位置か
 * @param [IN]  recv        センタ受信データ
 * @param [OUT] apiStatus   APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GemTimelineSearch_SendRecv(SMCAL *smcal,
										  const T_CC_CMN_SMS_API_PRM *parm,
										  const SMGEMTIMELINESEARCH *gemSerch,
										  SMGEMTIMELINEINFO *gemInfo,
										  INT32 *gemInfoNum,
										  INT32 *lastFlg,
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
		strcat(opt.resFilePath, CC_GEMTIMELINESRCH_RES_FILE);
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
		data = (Char*)SCC_MALLOC(CC_GEMTIMELINESRCH_SEND_BODY_SIZE);
		if (NULL == data) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// URI生成
		CC_GemTimelineSearch_CreateUri(parm, uri);

		// body生成
		ret = CC_GemTimelineSearch_CreateBody(parm, gemSerch, data);
		if(e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GemTimelineSearch_CreateBody error, " HERE);
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
		ret = CC_GemTimelineSearch_AnalyzeHttpResp(body, contextType, gemSerch->maxCnt, gemInfo, gemInfoNum, lastFlg, &opt, apiStatus);
		if(e_SC_RESULT_SUCCESS != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GemTimelineSearch_AnalyzeHttpResp error, " HERE);
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
void CC_GemTimelineSearch_CreateUri(const T_CC_CMN_SMS_API_PRM *parm, Char *uri)
{
	sprintf((char*)uri,
			"%smytimeline/srch/",
			parm->ApiPrmNavi.sms_sp_uri
	);
}

/**
 * @brief body生成
 * @param [IN]  parm        APIパラメータ
 * @param [IN/OUT] gemSerch GEMタイムライン検索条件/結果
 * @param [OUT] body        body
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GemTimelineSearch_CreateBody(const T_CC_CMN_SMS_API_PRM *parm, const SMGEMTIMELINESEARCH *gemSerch, Char *body)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	Char	*keyword = NULL;
	INT32	keywordLen = 0;

	do {
		sprintf((char*)body,
				"term_id=%s&term_sig=%s&guid=%s&user_sig=%s&max_cnt=%d&sort=%d",
				parm->ApiPrmMups.new_term_id,
				parm->ApiPrmMups.term_sig,
				parm->ApiPrmMups.guid,
				parm->ApiPrmMups.user_sig,
				gemSerch->maxCnt,
				gemSerch->sort
		);
		if ((NULL != gemSerch->keyword) && (EOS != gemSerch->keyword[0])) {
			// メモリ確保
			keywordLen = (SCC_MAX_KEYWORD * 3);
			keyword = (Char*)SCC_MALLOC(keywordLen);
			if (NULL == keyword) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserReg] SCC_MALLOC error " HERE);
				ret = e_SC_RESULT_MALLOC_ERR;
				break;
			}
			memset(keyword, 0, keywordLen);

			// URLエンコード
			SC_CAL_UrlEncode(gemSerch->keyword, keywordLen, keyword);

			sprintf((char*)&body[strlen((char*)body)],
					"&keyword=%s",
					keyword
			);
		} else {
			sprintf((char*)&body[strlen((char*)body)],
					"&keyword="
			);
		}
		if ((NULL != gemSerch->id) && (EOS != gemSerch->id[0])) {
			sprintf((char*)&body[strlen((char*)body)],
					"&gem_id=%s",
					gemSerch->id
			);
		} else {
			sprintf((char*)&body[strlen((char*)body)],
					"&gem_id="
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
 * @param [OUT] gemInfo     GEMタイムライン検索結果
 * @param [OUT] gemInfoNum  GEMタイムライン検索結果件数
 * @param [OUT] lastFlg     GEMタイムライン検索結果が最終位置か
 * @param [IN]  opt         オプション情報
 * @param [OUT] apiStatus   APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GemTimelineSearch_AnalyzeHttpResp(const Char *body,
												 E_CONTEXT_TYPE contextType,
												 INT32 gemInfoMaxNum,
												 SMGEMTIMELINEINFO *gemInfo,
												 INT32 *gemInfoNum,
												 INT32 *lastFlg,
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
		ret = CC_GemTimelineSearch_XmlParse((const Char*)body, &rsp_inf, gemInfoMaxNum, gemInfo, gemInfoNum, lastFlg, opt);
		if(e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GemTimelineSearch_XmlParse error, " HERE);
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
 * @brief gem/srch/応答メッセージ解析
 * @param [IN] xml      XMLファイルのフルパス
 * @param [IN] resp_inf CICレスポンス情報
 * @param [OUT] gem     GEM情報格納領域
 * @param [IN/OUT] gem  GEM情報格納領域数/GEM情報数
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GemTimelineSearch_XmlParse(const Char* xml,
										  T_CC_CMN_SMS_RESPONSE_INFO* resp_inf,
										  INT32 gemInfoMaxNum,
										  SMGEMTIMELINEINFO* gem,
										  INT32 *gemNum,
										  INT32 *lastFlg,
										  SMCALOPT *opt)
{
	E_SC_RESULT		ret = e_SC_RESULT_SUCCESS;
	GEMTIMELINESRCH_PARSER	gemsrchParser = {};
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
		memset(gem, 0, (sizeof(SMGEMTIMELINEINFO) * (gemInfoMaxNum)));
		gemsrchParser.buf = (Char*)SCC_MALLOC(CC_CMN_XML_LARGE_BUF_SIZE + 1);
		if (NULL == gemsrchParser.buf) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			CB_Result = e_SC_RESULT_MALLOC_ERR;
			ret = CB_Result;
			break;
		}
		gemsrchParser.gemInfoMaxNum = gemInfoMaxNum;
		gemsrchParser.gemInfo.gemListNum = 0;
		gemsrchParser.gemInfo.gemList = gem;
		gemsrchParser.gemInfo.apiStatus = &resp_inf->apiSts[0];
		gemsrchParser.gemInfo.lastFlg = lastFlg;
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
		XML_SetElementHandler(parser, CC_GemTimelineSearch_StartElement, CC_GemTimelineSearch_EndElement);
		XML_SetCharacterDataHandler(parser, CC_GemTimelineSearch_CharacterData);

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
			*gemNum = gemsrchParser.gemInfo.gemListNum;
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
void XMLCALL CC_GemTimelineSearch_StartElement(void *userData, const char *name, const char **atts)
{
	GEMTIMELINESRCH_PARSER *parser = (GEMTIMELINESRCH_PARSER*)userData;

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
			parser->state = CC_GEMTIMELINESRCH_NODE_XML;
		}

		// <xml>
		if (CC_GEMTIMELINESRCH_NODE_XML == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_API_STATUS)) {
				// <api_status>
				parser->state = CC_GEMTIMELINESRCH_NODE_API_STATUS;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMTIMELINESRCH_XML_NODE_LAST_FLG)) {
				// <last_flg>
				parser->state = CC_GEMTIMELINESRCH_NODE_LAST_FLG;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMTIMELINESRCH_XML_NODE_GEM_LIST)) {
				// <gem_list>
				parser->state = CC_GEMTIMELINESRCH_NODE_GEM_LIST;
			} else if (0 != strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_XML)) {
				parser->state = CC_GEMTIMELINESRCH_NODE_XML_CHILD;
			}
		} else if (CC_GEMTIMELINESRCH_NODE_GEM_LIST == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_GEMTIMELINESRCH_XML_NODE_ITEM)) {
				// <item>
				if (parser->gemInfoMaxNum <= parser->gemInfo.gemListNum) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"item maxnum over, " HERE);
					CB_Result = e_SC_RESULT_SMS_API_ERR;
					break;
				}
				parser->state = CC_GEMTIMELINESRCH_NODE_ITEM;
			} else {
				parser->state = CC_GEMTIMELINESRCH_NODE_GEM_LIST_CHILD;
			}
		} else if (CC_GEMTIMELINESRCH_NODE_ITEM == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_GEMTIMELINESRCH_XML_NODE_LAT)) {
				// <lat>
				parser->state = CC_GEMTIMELINESRCH_NODE_LAT;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMTIMELINESRCH_XML_NODE_LON)) {
				// <lon>
				parser->state = CC_GEMTIMELINESRCH_NODE_LON;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMTIMELINESRCH_XML_NODE_ALT)) {
				// <alt>
				parser->state = CC_GEMTIMELINESRCH_NODE_ALT;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMTIMELINESRCH_XML_NODE_GEM_ID)) {
				// <gem_id>
				parser->state = CC_GEMTIMELINESRCH_NODE_GEM_ID;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMTIMELINESRCH_XML_NODE_GEM_URL)) {
				// <gem_url>
				parser->state = CC_GEMTIMELINESRCH_NODE_GEM_URL;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMTIMELINESRCH_XML_NODE_GEM_PICT)) {
				// <gem_pict>
				parser->state = CC_GEMTIMELINESRCH_NODE_GEM_PICT;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMTIMELINESRCH_XML_NODE_USER)) {
				// <user>
				parser->state = CC_GEMTIMELINESRCH_NODE_USER;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMTIMELINESRCH_XML_NODE_TEXT)) {
				// <text>
				parser->state = CC_GEMTIMELINESRCH_NODE_TEXT;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMTIMELINESRCH_XML_NODE_GEM_DATETM)) {
				// <gem_datetm>
				parser->state = CC_GEMTIMELINESRCH_NODE_GEM_DATETM;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMTIMELINESRCH_XML_NODE_RGST_DATETM)) {
				// <rgst_datetm>
				parser->state = CC_GEMTIMELINESRCH_NODE_RGST_DATETM;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMTIMELINESRCH_XML_NODE_ALT_DATETM)) {
				// <alt_datetm>
				parser->state = CC_GEMTIMELINESRCH_NODE_ALT_DATETM;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMTIMELINESRCH_XML_NODE_LAST_DATETM)) {
				// <last_datetm>
				parser->state = CC_GEMTIMELINESRCH_NODE_LAST_DATETM;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMTIMELINESRCH_XML_NODE_LIKE_CNT)) {
				// <like_cnt>
				parser->state = CC_GEMTIMELINESRCH_NODE_LIKE_CNT;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMTIMELINESRCH_XML_NODE_LIKE_FLG)) {
				// <like_flg>
				parser->state = CC_GEMTIMELINESRCH_NODE_LIKE_FLG;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMTIMELINESRCH_XML_NODE_GROUP_ID)) {
				// <group_id>
				parser->state = CC_GEMTIMELINESRCH_NODE_GROUP_ID;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMTIMELINESRCH_XML_NODE_GROUP_NAME)) {
				// <group_name>
				parser->state = CC_GEMTIMELINESRCH_NODE_GROUP_NAME;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMTIMELINESRCH_XML_NODE_COMMENT_CNT)) {
				// <comment_cnt>
				parser->state = CC_GEMTIMELINESRCH_NODE_COMMENT_CNT;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMTIMELINESRCH_XML_NODE_ACCESS)) {
				// <access>
				parser->state = CC_GEMTIMELINESRCH_NODE_ACCESS;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMTIMELINESRCH_XML_NODE_GEM_OWNER)) {
				// <gem_owner>
				parser->state = CC_GEMTIMELINESRCH_NODE_GEM_OWNER;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMTIMELINESRCH_XML_NODE_CAN_DELETE)) {
				// <can_delete>
				parser->state = CC_GEMTIMELINESRCH_NODE_CAN_DELETE;
			} else if (0 == strcmp((char*)name, (char*)CC_GEMTIMELINESRCH_XML_NODE_USER_GUID)) {
				// <user_guid>
				parser->state = CC_GEMTIMELINESRCH_NODE_USER_GUID;
			} else {
				parser->state = CC_GEMTIMELINESRCH_NODE_ITEM_CHILD;
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
void XMLCALL CC_GemTimelineSearch_EndElement(void *userData, const char *name)
{
	GEMTIMELINESRCH_PARSER *parser = (GEMTIMELINESRCH_PARSER*)userData;
	SMGEMTIMELINEINFO	*gem = NULL;

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
		if (NULL == parser->gemInfo.gemList) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[parser->gemInfo.gemList], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}
		if (NULL == name) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[name], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}

		gem = &parser->gemInfo.gemList[parser->gemInfo.gemListNum];

		if (CC_GEMTIMELINESRCH_NODE_API_STATUS == parser->state) {
			// <api_status>
			strcpy((char*)parser->gemInfo.apiStatus, (char*)parser->buf);
			parser->state = CC_GEMTIMELINESRCH_NODE_XML;
		} else if (CC_GEMTIMELINESRCH_NODE_LAST_FLG == parser->state) {
			// <last_flg>
			*(parser->gemInfo.lastFlg) = atoi((char*)parser->buf);
			parser->state = CC_GEMTIMELINESRCH_NODE_XML;
		} else if (CC_GEMTIMELINESRCH_NODE_GEM_LIST == parser->state) {
			// <gem_list>
			parser->state = CC_GEMTIMELINESRCH_NODE_XML;
		} else if (CC_GEMTIMELINESRCH_NODE_ITEM == parser->state) {
			// <item>
			parser->state = CC_GEMTIMELINESRCH_NODE_GEM_LIST;
			parser->gemInfo.gemListNum++;
		} else if (CC_GEMTIMELINESRCH_NODE_LAT == parser->state) {
			// <lat>
			gem->lat = atof((char*)parser->buf);
			parser->state = CC_GEMTIMELINESRCH_NODE_ITEM;
		} else if (CC_GEMTIMELINESRCH_NODE_LON == parser->state) {
			// <lon>
			gem->lon = atof((char*)parser->buf);
			parser->state = CC_GEMTIMELINESRCH_NODE_ITEM;
		} else if (CC_GEMTIMELINESRCH_NODE_ALT == parser->state) {
			// <alt>
			gem->alt = atof((char*)parser->buf);
			parser->state = CC_GEMTIMELINESRCH_NODE_ITEM;
		} else if (CC_GEMTIMELINESRCH_NODE_GEM_ID == parser->state) {
			// <gem_id>
			strcpy((char*)gem->id, (char*)parser->buf);
			parser->state = CC_GEMTIMELINESRCH_NODE_ITEM;
		} else if (CC_GEMTIMELINESRCH_NODE_GEM_URL == parser->state) {
			// <gem_url>
			strcpy((char*)gem->url, (char*)parser->buf);
			parser->state = CC_GEMTIMELINESRCH_NODE_ITEM;
		} else if (CC_GEMTIMELINESRCH_NODE_USER == parser->state) {
			// <user>
			strcpy((char*)gem->user, (char*)parser->buf);
			parser->state = CC_GEMTIMELINESRCH_NODE_ITEM;
		} else if (CC_GEMTIMELINESRCH_NODE_TEXT == parser->state) {
			// <text>
			strcpy((char*)gem->text, (char*)parser->buf);
			parser->state = CC_GEMTIMELINESRCH_NODE_ITEM;
		} else if (CC_GEMTIMELINESRCH_NODE_GEM_PICT == parser->state) {
			// <gem_pict>
			strcpy((char*)gem->gemPict, (char*)parser->buf);
			parser->state = CC_GEMTIMELINESRCH_NODE_ITEM;
		} else if (CC_GEMTIMELINESRCH_NODE_GEM_DATETM == parser->state) {
			// <gem_datetm>
			strcpy((char*)gem->gemDatetm, (char*)parser->buf);
			parser->state = CC_GEMTIMELINESRCH_NODE_ITEM;
		} else if (CC_GEMTIMELINESRCH_NODE_RGST_DATETM == parser->state) {
			// <rgst_datetm>
			strcpy((char*)gem->rgstDatetm, (char*)parser->buf);
			parser->state = CC_GEMTIMELINESRCH_NODE_ITEM;
		} else if (CC_GEMTIMELINESRCH_NODE_ALT_DATETM == parser->state) {
			// <alt_datetm>
			strcpy((char*)gem->altDatetm, (char*)parser->buf);
			parser->state = CC_GEMTIMELINESRCH_NODE_ITEM;
		} else if (CC_GEMTIMELINESRCH_NODE_LAST_DATETM == parser->state) {
			// <last_datetm>
			strcpy((char*)gem->lastDatetm, (char*)parser->buf);
			parser->state = CC_GEMTIMELINESRCH_NODE_ITEM;
		} else if (CC_GEMTIMELINESRCH_NODE_LIKE_CNT == parser->state) {
			// <like_cnt>
			gem->likeCnt = atol((char*)parser->buf);
			parser->state = CC_GEMTIMELINESRCH_NODE_ITEM;
		} else if (CC_GEMTIMELINESRCH_NODE_LIKE_FLG == parser->state) {
			// <like_flg>
			gem->likeFlg = atoi((char*)parser->buf);
			parser->state = CC_GEMTIMELINESRCH_NODE_ITEM;
		} else if (CC_GEMTIMELINESRCH_NODE_GROUP_ID == parser->state) {
			// <group_id>
			strcpy((char*)gem->groupId, (char*)parser->buf);
			parser->state = CC_GEMTIMELINESRCH_NODE_ITEM;
		} else if (CC_GEMTIMELINESRCH_NODE_GROUP_NAME == parser->state) {
			// <group_name>
			strcpy((char*)gem->groupName, (char*)parser->buf);
			parser->state = CC_GEMTIMELINESRCH_NODE_ITEM;
		} else if (CC_GEMTIMELINESRCH_NODE_COMMENT_CNT == parser->state) {
			// <comment_cnt>
			gem->commentCnt = atol((char*)parser->buf);
			parser->state = CC_GEMTIMELINESRCH_NODE_ITEM;
		} else if (CC_GEMTIMELINESRCH_NODE_ACCESS == parser->state) {
			// <access>
			strcpy((char*)gem->access, (char*)parser->buf);
			parser->state = CC_GEMTIMELINESRCH_NODE_ITEM;
		} else if (CC_GEMTIMELINESRCH_NODE_GEM_OWNER == parser->state) {
			// <gem_owner>
			gem->gemOwner = atoi((char*)parser->buf);
			parser->state = CC_GEMTIMELINESRCH_NODE_ITEM;
		} else if (CC_GEMTIMELINESRCH_NODE_CAN_DELETE == parser->state) {
			// <can_delete>
			gem->canDelete = atoi((char*)parser->buf);
			parser->state = CC_GEMTIMELINESRCH_NODE_ITEM;
		} else if (CC_GEMTIMELINESRCH_NODE_USER_GUID == parser->state) {
			// <user_guid>
			strcpy((char*)gem->guid, (char*)parser->buf);
			parser->state = CC_GEMTIMELINESRCH_NODE_ITEM;
		} else if (CC_GEMTIMELINESRCH_NODE_XML_CHILD == parser->state) {
			parser->state = CC_GEMTIMELINESRCH_NODE_XML;
		} else if (CC_GEMTIMELINESRCH_NODE_GEM_LIST_CHILD == parser->state) {
			parser->state = CC_GEMTIMELINESRCH_NODE_GEM_LIST;
		} else if (CC_GEMTIMELINESRCH_NODE_ITEM_CHILD == parser->state) {
			parser->state = CC_GEMTIMELINESRCH_NODE_ITEM;
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
void XMLCALL CC_GemTimelineSearch_CharacterData(void *userData, const XML_Char *data, INT32 len)
{
	GEMTIMELINESRCH_PARSER *parser = (GEMTIMELINESRCH_PARSER*)userData;
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

		if (CC_GEMTIMELINESRCH_NODE_API_STATUS == parser->state) {
			// <api_status>
			if (CC_GEMTIMELINESRCH_XML_DATA_APISTATUS_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMTIMELINESRCH_NODE_LAST_FLG == parser->state) {
			// <last_flg>
			if (CC_GEMTIMELINESRCH_XML_DATA_LASTFLG_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMTIMELINESRCH_NODE_ITEM == parser->state) {
			// <item>
		} else if (CC_GEMTIMELINESRCH_NODE_LAT == parser->state) {
			// <lat>
			if (CC_GEMTIMELINESRCH_XML_DATA_LAT_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMTIMELINESRCH_NODE_LON == parser->state) {
			// <lon>
			if (CC_GEMTIMELINESRCH_XML_DATA_LON_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMTIMELINESRCH_NODE_ALT == parser->state) {
			// <alt>
			if (CC_GEMTIMELINESRCH_XML_DATA_ALT_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMTIMELINESRCH_NODE_GEM_ID == parser->state) {
			// <gem_id>
			if (CC_GEMTIMELINESRCH_XML_DATA_GEMID_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMTIMELINESRCH_NODE_GEM_URL == parser->state) {
			// <gem_url>
			if (CC_GEMTIMELINESRCH_XML_DATA_GEMURL_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMTIMELINESRCH_NODE_USER == parser->state) {
			// <user>
			if (CC_GEMTIMELINESRCH_XML_DATA_USER_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMTIMELINESRCH_NODE_TEXT == parser->state) {
			// <text>
			if (CC_GEMTIMELINESRCH_XML_DATA_TEXT_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMTIMELINESRCH_NODE_GEM_PICT == parser->state) {
			// <gem_pict>
			if (CC_GEMTIMELINESRCH_XML_DATA_GEMPICT_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMTIMELINESRCH_NODE_GEM_DATETM == parser->state) {
			// <gem_datetm>
			if (CC_GEMTIMELINESRCH_XML_DATA_GEMDATETM_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMTIMELINESRCH_NODE_RGST_DATETM == parser->state) {
			// <rgst_datetm>
			if (CC_GEMTIMELINESRCH_XML_DATA_RGSTDATETM_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMTIMELINESRCH_NODE_ALT_DATETM == parser->state) {
			// <alt_datetm>
			if (CC_GEMTIMELINESRCH_XML_DATA_ALTDATETM_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMTIMELINESRCH_NODE_LAST_DATETM == parser->state) {
			// <last_datetm>
			if (CC_GEMTIMELINESRCH_XML_DATA_LASTDATETM_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMTIMELINESRCH_NODE_LIKE_CNT == parser->state) {
			// <like_cnt>
			if (CC_GEMTIMELINESRCH_XML_DATA_LIKECNT_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMTIMELINESRCH_NODE_LIKE_FLG == parser->state) {
			// <like_flg>
			if (CC_GEMTIMELINESRCH_XML_DATA_LIKEFLG_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMTIMELINESRCH_NODE_GROUP_ID == parser->state) {
			// <group_id>
			if (CC_GEMTIMELINESRCH_XML_DATA_GROUPID_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMTIMELINESRCH_NODE_GROUP_NAME == parser->state) {
			// <group_name>
			if (CC_GEMTIMELINESRCH_XML_DATA_GROUPNAME_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMTIMELINESRCH_NODE_COMMENT_CNT == parser->state) {
			// <comment_cnt>
			if (CC_GEMTIMELINESRCH_XML_DATA_COMMENTCNT_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMTIMELINESRCH_NODE_ACCESS == parser->state) {
			// <access>
			if (CC_GEMTIMELINESRCH_XML_DATA_ACCESS_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMTIMELINESRCH_NODE_GEM_OWNER == parser->state) {
			// <gem_owner>
			if (CC_GEMTIMELINESRCH_XML_DATA_GEM_OWNER_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMTIMELINESRCH_NODE_CAN_DELETE == parser->state) {
			// <can_delete>
			if (CC_GEMTIMELINESRCH_XML_DATA_CAN_DELETE_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GEMTIMELINESRCH_NODE_USER_GUID == parser->state) {
			// <user_guid>
			if (CC_GEMTIMELINESRCH_XML_DATA_GUID_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		}
	} while (0);

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}
