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

#define	CC_MAPPINGIDSRCH_SEND_BODY_SIZE						1024

#define	CC_MAPPINGIDSRCH_XML_NODE_LAT						"lat"
#define	CC_MAPPINGIDSRCH_XML_NODE_LON						"lon"
#define	CC_MAPPINGIDSRCH_XML_NODE_PID						"pid"
#define	CC_MAPPINGIDSRCH_XML_NODE_X							"x"
#define	CC_MAPPINGIDSRCH_XML_NODE_Y							"y"
#define	CC_MAPPINGIDSRCH_XML_NODE_MAPPING_ID				"mpng_id"
#define	CC_MAPPINGIDSRCH_XML_NODE_GENRE						"genre"
#define	CC_MAPPINGIDSRCH_XML_NODE_USER						"user"
#define	CC_MAPPINGIDSRCH_XML_NODE_AVATAR_URL_SMALL			"avatar_url_small"
#define	CC_MAPPINGIDSRCH_XML_NODE_AVATAR_URL_MEDIUM			"avatar_url_medium"
#define	CC_MAPPINGIDSRCH_XML_NODE_AVATAR_URL_LARGE			"avatar_url_large"
#define	CC_MAPPINGIDSRCH_XML_NODE_TEXT						"text"
#define	CC_MAPPINGIDSRCH_XML_NODE_PICT						"pict"
#define	CC_MAPPINGIDSRCH_XML_NODE_MODE						"mode"
#define	CC_MAPPINGIDSRCH_XML_NODE_MPNG_DATETM				"mpng_datetm"
#define	CC_MAPPINGIDSRCH_XML_NODE_RGST_DATETM				"rgst_datetm"
#define	CC_MAPPINGIDSRCH_XML_NODE_ALT_DATETM				"alt_datetm"
#define	CC_MAPPINGIDSRCH_XML_NODE_LAST_DATETM				"last_datetm"
#define	CC_MAPPINGIDSRCH_XML_NODE_LIKE_CNT					"like_cnt"
#define	CC_MAPPINGIDSRCH_XML_NODE_LIKE_FLG					"like_flg"
#define	CC_MAPPINGIDSRCH_XML_NODE_RATINGNUM_YES				"ratingnum_yes"
#define	CC_MAPPINGIDSRCH_XML_NODE_RATINGNUM_NO				"ratingnum_no"
#define	CC_MAPPINGIDSRCH_XML_NODE_RATING_OWN				"rating_own"
#define	CC_MAPPINGIDSRCH_XML_NODE_GROUP_NAME				"group_name"
#define	CC_MAPPINGIDSRCH_XML_NODE_COMMENT_CNT				"comment_cnt"
#define	CC_MAPPINGIDSRCH_XML_NODE_ACCESS					"access"
#define	CC_MAPPINGIDSRCH_XML_NODE_CAN_DELETE				"can_delete"
#define	CC_MAPPINGIDSRCH_XML_NODE_OWN_FLG					"own_flg"
#define	CC_MAPPINGIDSRCH_XML_NODE_USER_GUID					"user_guid"
#define	CC_MAPPINGIDSRCH_XML_NODE_GROUP_ID					"group_id"

#define	CC_MAPPINGIDSRCH_RES_FILE							"mappingid.srch"

#define	CC_MAPPINGIDSRCH_XML_DATA_APISTATUS_SIZE			CC_CMN_XML_RES_STS_CODE_SIZE
#define	CC_MAPPINGIDSRCH_XML_DATA_LAT_SIZE					20
#define	CC_MAPPINGIDSRCH_XML_DATA_LON_SIZE					20
#define	CC_MAPPINGIDSRCH_XML_DATA_PID_SIZE					11
#define	CC_MAPPINGIDSRCH_XML_DATA_X_SIZE					11
#define	CC_MAPPINGIDSRCH_XML_DATA_Y_SIZE					11
#define	CC_MAPPINGIDSRCH_XML_DATA_MAPPINGID_SIZE			CC_CMN_MAPPING_ID_SIZE
#define	CC_MAPPINGIDSRCH_XML_DATA_GENRE_SIZE				11
#define	CC_MAPPINGIDSRCH_XML_DATA_USER_SIZE					SCC_MAX_USERNAME
#define	CC_MAPPINGIDSRCH_XML_DATA_AVATAR_URL_SMALL_SIZE		SCC_MAX_URL
#define	CC_MAPPINGIDSRCH_XML_DATA_AVATAR_URL_MEDIUM_SIZE	SCC_MAX_URL
#define	CC_MAPPINGIDSRCH_XML_DATA_AVATAR_URL_LARGE_SIZE		SCC_MAX_URL
#define	CC_MAPPINGIDSRCH_XML_DATA_TEXT_SIZE					SCC_MAX_TEXT
#define	CC_MAPPINGIDSRCH_XML_DATA_PICT_SIZE					SCC_MAX_URL
#define	CC_MAPPINGIDSRCH_XML_DATA_MODE_SIZE					2
#define	CC_MAPPINGIDSRCH_XML_DATA_MPNGDATETM_SIZE			CC_CMN_MAPPING_DATE_SIZE
#define	CC_MAPPINGIDSRCH_XML_DATA_RGSTDATETM_SIZE			CC_CMN_MAPPING_DATE_SIZE
#define	CC_MAPPINGIDSRCH_XML_DATA_ALTDATETM_SIZE			CC_CMN_MAPPING_DATE_SIZE
#define	CC_MAPPINGIDSRCH_XML_DATA_LASTDATETM_SIZE			CC_CMN_MAPPING_DATE_SIZE
#define	CC_MAPPINGIDSRCH_XML_DATA_LIKECNT_SIZE				11
#define	CC_MAPPINGIDSRCH_XML_DATA_LIKEFLG_SIZE				2
#define	CC_MAPPINGIDSRCH_XML_DATA_RATINGNUM_YES_SIZE		11
#define	CC_MAPPINGIDSRCH_XML_DATA_RATINGNUM_NO_SIZE			11
#define	CC_MAPPINGIDSRCH_XML_DATA_RATING_OWN_SIZE			3
#define	CC_MAPPINGIDSRCH_XML_DATA_GROUPNAME_SIZE			CC_CMN_MAPPING_GROUPNAME_SIZE
#define	CC_MAPPINGIDSRCH_XML_DATA_COMMENTCNT_SIZE			11
#define	CC_MAPPINGIDSRCH_XML_DATA_ACCESS_SIZE				CC_CMN_MAPPING_ACCESS_SIZE
#define	CC_MAPPINGIDSRCH_XML_DATA_CAN_DELETE_SIZE			2
#define	CC_MAPPINGIDSRCH_XML_DATA_OWN_FLG_SIZE				3
#define	CC_MAPPINGIDSRCH_XML_DATA_GUID_SIZE					CC_CMN_GUID_STR_SIZE
#define	CC_MAPPINGIDSRCH_XML_DATA_GROUPID_SIZE				CC_CMN_MAPPING_GROUPID_SIZE

// マッピングID検索レスポンスXML情報
typedef struct _MAPPINGINFO {
	SMMAPPINGIDSRCHRES	*mappingInfo;
	INT32				*status;
	Char				*apiStatus;
} MAPPINGINFO;

// MAPPINGID検索XMLパーサ
typedef struct _MAPPINGIDSRCH_PARSER {
	INT32			state;
	Char			*buf;
	MAPPINGINFO		mappingInfo;
} MAPPINGIDSRCH_PARSER;

// mpng/idsrch/
enum MappingIdSrchStatus {
	CC_MAPPINGIDSRCH_NODE_NONE = 0,
	CC_MAPPINGIDSRCH_NODE_XML,
	CC_MAPPINGIDSRCH_NODE_XML_CHILD,
	CC_MAPPINGIDSRCH_NODE_API_STATUS,
	CC_MAPPINGIDSRCH_NODE_LAT,
	CC_MAPPINGIDSRCH_NODE_LON,
	CC_MAPPINGIDSRCH_NODE_PID,
	CC_MAPPINGIDSRCH_NODE_X,
	CC_MAPPINGIDSRCH_NODE_Y,
	CC_MAPPINGIDSRCH_NODE_MAPPING_ID,
	CC_MAPPINGIDSRCH_NODE_GENRE,
	CC_MAPPINGIDSRCH_NODE_USER,
	CC_MAPPINGIDSRCH_NODE_AVATAR_URL_SMALL,
	CC_MAPPINGIDSRCH_NODE_AVATAR_URL_MEDIUM,
	CC_MAPPINGIDSRCH_NODE_AVATAR_URL_LARGE,
	CC_MAPPINGIDSRCH_NODE_TEXT,
	CC_MAPPINGIDSRCH_NODE_PICT,
	CC_MAPPINGIDSRCH_NODE_MODE,
	CC_MAPPINGIDSRCH_NODE_MPNG_DATETM,
	CC_MAPPINGIDSRCH_NODE_RGST_DATETM,
	CC_MAPPINGIDSRCH_NODE_ALT_DATETM,
	CC_MAPPINGIDSRCH_NODE_LAST_DATETM,
	CC_MAPPINGIDSRCH_NODE_LIKE_CNT,
	CC_MAPPINGIDSRCH_NODE_LIKE_FLG,
	CC_MAPPINGIDSRCH_NODE_RATINGNUM_YES,
	CC_MAPPINGIDSRCH_NODE_RATINGNUM_NO,
	CC_MAPPINGIDSRCH_NODE_RATING_OWN,
	CC_MAPPINGIDSRCH_NODE_GROUP_NAME,
	CC_MAPPINGIDSRCH_NODE_COMMENT_CNT,
	CC_MAPPINGIDSRCH_NODE_ACCESS,
	CC_MAPPINGIDSRCH_NODE_CAN_DELETE,
	CC_MAPPINGIDSRCH_NODE_OWN_FLG,
	CC_MAPPINGIDSRCH_NODE_USER_GUID,
	CC_MAPPINGIDSRCH_NODE_GROUP_ID
};


//------------------------------------------------
// 変数定義
//------------------------------------------------
static INT32	CB_Result;				// 処理結果 (コールバック関数用)
static Bool		CB_IsXmlFile;

//------------------------------------------------
// 関数定義
//------------------------------------------------
static void CC_MappingIdSrch_CreateUri(const T_CC_CMN_SMS_API_PRM *param, Char *uri);
static void CC_MappingIdSrch_CreateBody(const T_CC_CMN_SMS_API_PRM *parm, const Char *mappingId, Char *body);
static E_SC_RESULT CC_MappingIdSrch_AnalyzeHttpResp(const Char *body, E_CONTEXT_TYPE contextType, SMMAPPINGIDSRCHRES *mappingInfo, SMCALOPT *opt, Char *apiStatus);
static E_SC_RESULT CC_MappingIdSrch_XmlParse(const Char* xml, T_CC_CMN_SMS_RESPONSE_INFO* resp_inf, SMMAPPINGIDSRCHRES *mappingInfo, SMCALOPT *opt);
static void XMLCALL CC_MappingIdSrch_StartElement(void *userData, const char *name, const char **atts);
static void XMLCALL CC_MappingIdSrch_EndElement(void *userData, const char *name);
static void XMLCALL CC_MappingIdSrch_CharacterData(void *userData, const XML_Char *data, INT32 len);

/**
 * @brief マッピングID検索
 * @param [IN]  smcal           SMCAL
 * @param [IN]  parm            APIパラメータ
 * @param [OUT] mappingId       マッピングID
 * @param [OUT] mappingInfo     マッピング検索結果
 * @param [IN]  recv            センタ受信データ
 * @param [OUT] apiStatus       APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_MappingIdSrch_SendRecv(SMCAL *smcal,
									  const T_CC_CMN_SMS_API_PRM *parm,
									  const Char *mappingId,
									  SMMAPPINGIDSRCHRES *mappingInfo,
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
		strcat(opt.resFilePath, CC_MAPPINGIDSRCH_RES_FILE);
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
		data = (Char*)SCC_MALLOC(CC_MAPPINGIDSRCH_SEND_BODY_SIZE);
		if (NULL == data) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// URI生成
		CC_MappingIdSrch_CreateUri(parm, uri);

		// body生成
		CC_MappingIdSrch_CreateBody(parm, mappingId, data);

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
		ret = CC_MappingIdSrch_AnalyzeHttpResp(body,
												 contextType,
												 mappingInfo,
												 &opt,
												 apiStatus);
		if(e_SC_RESULT_SUCCESS != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MappingIdSrch_AnalyzeHttpResp error, " HERE);
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
void CC_MappingIdSrch_CreateUri(const T_CC_CMN_SMS_API_PRM *parm, Char *uri)
{
	sprintf((char*)uri,
			"%smpng/idsrch/",
			parm->ApiPrmNavi.sms_sp_uri
	);
}

/**
 * @brief body生成
 * @param [IN] parm         APIパラメータ
 * @param [IN] mappingId    マッピングID
 * @param [OUT] body        body
 */
void CC_MappingIdSrch_CreateBody(const T_CC_CMN_SMS_API_PRM *parm, const Char *mappingId, Char *body)
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
 * @param [IN]  body            xmlデータ
 * @param [IN]  contextType     コンテキスト
 * @param [OUT] mappingInfo     マッピングID検索結果
 * @param [IN]  opt             オプション情報
 * @param [OUT] apiStatus       APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_MappingIdSrch_AnalyzeHttpResp(const Char *body,
											 E_CONTEXT_TYPE contextType,
											 SMMAPPINGIDSRCHRES *mappingInfo,
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
		ret = CC_MappingIdSrch_XmlParse((const Char*)body, &rsp_inf, mappingInfo, opt);
		if(e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MappingIdSrch_XmlParse error, " HERE);
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
 * @brief mpng/idsrch/応答メッセージ解析
 * @param [IN] xml              XMLファイルのフルパス
 * @param [IN] resp_inf         CICレスポンス情報
 * @param [OUT] mappingInfo     マッピングID検索結果
 * @param [IN]  opt             オプション情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_MappingIdSrch_XmlParse(const Char* xml,
									  T_CC_CMN_SMS_RESPONSE_INFO* resp_inf,
									  SMMAPPINGIDSRCHRES* mappingInfo,
									  SMCALOPT *opt)
{
	E_SC_RESULT		ret = e_SC_RESULT_SUCCESS;
	MAPPINGIDSRCH_PARSER	mappingsrchParser = {};
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
		memset(mappingInfo, 0, sizeof(SMMAPPINGIDSRCHRES));
		mappingsrchParser.buf = (Char*)SCC_MALLOC(CC_CMN_XML_LARGE_BUF_SIZE + 1);
		if (NULL == mappingsrchParser.buf) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			CB_Result = e_SC_RESULT_MALLOC_ERR;
			ret = CB_Result;
			break;
		}
		mappingsrchParser.mappingInfo.mappingInfo = mappingInfo;
		mappingsrchParser.mappingInfo.apiStatus = &resp_inf->apiSts[0];
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
		XML_SetUserData(parser, &mappingsrchParser);
		XML_SetElementHandler(parser, CC_MappingIdSrch_StartElement, CC_MappingIdSrch_EndElement);
		XML_SetCharacterDataHandler(parser, CC_MappingIdSrch_CharacterData);

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

	if (NULL != mappingsrchParser.buf) {
		SCC_FREE(mappingsrchParser.buf);
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
void XMLCALL CC_MappingIdSrch_StartElement(void *userData, const char *name, const char **atts)
{
	MAPPINGIDSRCH_PARSER *parser = (MAPPINGIDSRCH_PARSER*)userData;

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
			parser->state = CC_MAPPINGIDSRCH_NODE_XML;
		}

		// <xml>
		if (CC_MAPPINGIDSRCH_NODE_XML == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_API_STATUS)) {
				// <api_status>
				parser->state = CC_MAPPINGIDSRCH_NODE_API_STATUS;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGIDSRCH_XML_NODE_LAT)) {
				// <lat>
				parser->state = CC_MAPPINGIDSRCH_NODE_LAT;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGIDSRCH_XML_NODE_LON)) {
				// <lon>
				parser->state = CC_MAPPINGIDSRCH_NODE_LON;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGIDSRCH_XML_NODE_PID)) {
				// <pid>
				parser->state = CC_MAPPINGIDSRCH_NODE_PID;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGIDSRCH_XML_NODE_X)) {
				// <x>
				parser->state = CC_MAPPINGIDSRCH_NODE_X;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGIDSRCH_XML_NODE_Y)) {
				// <Y>
				parser->state = CC_MAPPINGIDSRCH_NODE_Y;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGIDSRCH_XML_NODE_MAPPING_ID)) {
				// <mpng_id>
				parser->state = CC_MAPPINGIDSRCH_NODE_MAPPING_ID;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGIDSRCH_XML_NODE_GENRE)) {
				// <genre>
				parser->state = CC_MAPPINGIDSRCH_NODE_GENRE;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGIDSRCH_XML_NODE_USER)) {
				// <user>
				parser->state = CC_MAPPINGIDSRCH_NODE_USER;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGIDSRCH_XML_NODE_AVATAR_URL_SMALL)) {
				// <avatar_url_small>
				parser->state = CC_MAPPINGIDSRCH_NODE_AVATAR_URL_SMALL;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGIDSRCH_XML_NODE_AVATAR_URL_MEDIUM)) {
				// <avatar_url_medium>
				parser->state = CC_MAPPINGIDSRCH_NODE_AVATAR_URL_MEDIUM;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGIDSRCH_XML_NODE_AVATAR_URL_LARGE)) {
				// <avatar_url_large>
				parser->state = CC_MAPPINGIDSRCH_NODE_AVATAR_URL_LARGE;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGIDSRCH_XML_NODE_TEXT)) {
				// <text>
				parser->state = CC_MAPPINGIDSRCH_NODE_TEXT;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGIDSRCH_XML_NODE_PICT)) {
				// <pict>
				parser->state = CC_MAPPINGIDSRCH_NODE_PICT;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGIDSRCH_XML_NODE_MODE)) {
				// <mode>
				parser->state = CC_MAPPINGIDSRCH_NODE_MODE;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGIDSRCH_XML_NODE_MPNG_DATETM)) {
				// <mpng_datetm>
				parser->state = CC_MAPPINGIDSRCH_NODE_MPNG_DATETM;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGIDSRCH_XML_NODE_RGST_DATETM)) {
				// <rgst_datetm>
				parser->state = CC_MAPPINGIDSRCH_NODE_RGST_DATETM;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGIDSRCH_XML_NODE_ALT_DATETM)) {
				// <alt_datetm>
				parser->state = CC_MAPPINGIDSRCH_NODE_ALT_DATETM;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGIDSRCH_XML_NODE_LAST_DATETM)) {
				// <last_datetm>
				parser->state = CC_MAPPINGIDSRCH_NODE_LAST_DATETM;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGIDSRCH_XML_NODE_LIKE_CNT)) {
				// <like_cnt>
				parser->state = CC_MAPPINGIDSRCH_NODE_LIKE_CNT;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGIDSRCH_XML_NODE_LIKE_FLG)) {
				// <like_flg>
				parser->state = CC_MAPPINGIDSRCH_NODE_LIKE_FLG;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGIDSRCH_XML_NODE_RATINGNUM_YES)) {
				// <ratingnum_yes>
				parser->state = CC_MAPPINGIDSRCH_NODE_RATINGNUM_YES;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGIDSRCH_XML_NODE_RATINGNUM_NO)) {
				// <ratingnum_no>
				parser->state = CC_MAPPINGIDSRCH_NODE_RATINGNUM_NO;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGIDSRCH_XML_NODE_RATING_OWN)) {
				// <rating_own>
				parser->state = CC_MAPPINGIDSRCH_NODE_RATING_OWN;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGIDSRCH_XML_NODE_GROUP_NAME)) {
				// <group_name>
				parser->state = CC_MAPPINGIDSRCH_NODE_GROUP_NAME;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGIDSRCH_XML_NODE_COMMENT_CNT)) {
				// <comment_cnt>
				parser->state = CC_MAPPINGIDSRCH_NODE_COMMENT_CNT;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGIDSRCH_XML_NODE_ACCESS)) {
				// <access>
				parser->state = CC_MAPPINGIDSRCH_NODE_ACCESS;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGIDSRCH_XML_NODE_CAN_DELETE)) {
				// <can_delete>
				parser->state = CC_MAPPINGIDSRCH_NODE_CAN_DELETE;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGIDSRCH_XML_NODE_OWN_FLG)) {
				// <own_flg>
				parser->state = CC_MAPPINGIDSRCH_NODE_OWN_FLG;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGIDSRCH_XML_NODE_USER_GUID)) {
				// <user_guid>
				parser->state = CC_MAPPINGIDSRCH_NODE_USER_GUID;
			} else if (0 == strcmp((char*)name, (char*)CC_MAPPINGIDSRCH_XML_NODE_GROUP_ID)) {
				// <group_id>
				parser->state = CC_MAPPINGIDSRCH_NODE_GROUP_ID;
			} else if (0 != strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_XML)) {
				parser->state = CC_MAPPINGIDSRCH_NODE_XML_CHILD;
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
void XMLCALL CC_MappingIdSrch_EndElement(void *userData, const char *name)
{
	MAPPINGIDSRCH_PARSER *parser = (MAPPINGIDSRCH_PARSER*)userData;
	SMMAPPINGIDSRCHRES	*mapping = NULL;

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
		if (NULL == parser->mappingInfo.mappingInfo) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[parser->mappingInfo.mappingInfo], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}
		if (NULL == name) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[name], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}

		mapping = parser->mappingInfo.mappingInfo;

		if (CC_MAPPINGIDSRCH_NODE_API_STATUS == parser->state) {
			// <api_status>
			strcpy((char*)parser->mappingInfo.apiStatus, (char*)parser->buf);
			parser->state = CC_MAPPINGIDSRCH_NODE_XML;
		} else if (CC_MAPPINGIDSRCH_NODE_LAT == parser->state) {
			// <lat>
			mapping->lat = atof((char*)parser->buf);
			parser->state = CC_MAPPINGIDSRCH_NODE_XML;
		} else if (CC_MAPPINGIDSRCH_NODE_LON == parser->state) {
			// <lon>
			mapping->lon = atof((char*)parser->buf);
			parser->state = CC_MAPPINGIDSRCH_NODE_XML;
		} else if (CC_MAPPINGIDSRCH_NODE_PID == parser->state) {
			// <pid>
			mapping->parcelId = (UINT32)atol((char*)parser->buf);
			parser->state = CC_MAPPINGIDSRCH_NODE_XML;
		} else if (CC_MAPPINGIDSRCH_NODE_X == parser->state) {
			// <x>
			mapping->x = atoi((char*)parser->buf);
			parser->state = CC_MAPPINGIDSRCH_NODE_XML;
		} else if (CC_MAPPINGIDSRCH_NODE_Y == parser->state) {
			// <y>
			mapping->y = atoi((char*)parser->buf);
			parser->state = CC_MAPPINGIDSRCH_NODE_XML;
		} else if (CC_MAPPINGIDSRCH_NODE_MAPPING_ID == parser->state) {
			// <mpng_id>
			strcpy((char*)mapping->mappingId, (char*)parser->buf);
			parser->state = CC_MAPPINGIDSRCH_NODE_XML;
		} else if (CC_MAPPINGIDSRCH_NODE_GENRE == parser->state) {
			// <genre>
			mapping->genre = atoi((char*)parser->buf);
			parser->state = CC_MAPPINGIDSRCH_NODE_XML;
		} else if (CC_MAPPINGIDSRCH_NODE_USER == parser->state) {
			// <user>
			strcpy((char*)mapping->user, (char*)parser->buf);
			parser->state = CC_MAPPINGIDSRCH_NODE_XML;
		} else if (CC_MAPPINGIDSRCH_NODE_AVATAR_URL_SMALL == parser->state) {
			// <avatar_url_small>
			strcpy((char*)mapping->avtSmlUrl, (char*)parser->buf);
			parser->state = CC_MAPPINGIDSRCH_NODE_XML;
		} else if (CC_MAPPINGIDSRCH_NODE_AVATAR_URL_MEDIUM == parser->state) {
			// <avatar_url_medium>
			strcpy((char*)mapping->avtMidUrl, (char*)parser->buf);
			parser->state = CC_MAPPINGIDSRCH_NODE_XML;
		} else if (CC_MAPPINGIDSRCH_NODE_AVATAR_URL_LARGE == parser->state) {
			// <avatar_url_large>
			strcpy((char*)mapping->avtLrgUrl, (char*)parser->buf);
			parser->state = CC_MAPPINGIDSRCH_NODE_XML;
		} else if (CC_MAPPINGIDSRCH_NODE_TEXT == parser->state) {
			// <text>
			strcpy((char*)mapping->text, (char*)parser->buf);
			parser->state = CC_MAPPINGIDSRCH_NODE_XML;
		} else if (CC_MAPPINGIDSRCH_NODE_PICT == parser->state) {
			// <pict>
			strcpy((char*)mapping->pictUrl, (char*)parser->buf);
			parser->state = CC_MAPPINGIDSRCH_NODE_XML;
		} else if (CC_MAPPINGIDSRCH_NODE_MODE == parser->state) {
			// <mode>
			mapping->mode = atoi((char*)parser->buf);
			parser->state = CC_MAPPINGIDSRCH_NODE_XML;
		} else if (CC_MAPPINGIDSRCH_NODE_MPNG_DATETM == parser->state) {
			// <mpng_datetm>
			mapping->mappingDatetm = atoi((char*)parser->buf);
			parser->state = CC_MAPPINGIDSRCH_NODE_XML;
		} else if (CC_MAPPINGIDSRCH_NODE_RGST_DATETM == parser->state) {
			// <rgst_datetm>
			mapping->rgstDatetm = atoi((char*)parser->buf);
			parser->state = CC_MAPPINGIDSRCH_NODE_XML;
		} else if (CC_MAPPINGIDSRCH_NODE_ALT_DATETM == parser->state) {
			// <alt_datetm>
			mapping->altDatetm = atoi((char*)parser->buf);
			parser->state = CC_MAPPINGIDSRCH_NODE_XML;
		} else if (CC_MAPPINGIDSRCH_NODE_LAST_DATETM == parser->state) {
			// <last_datetm>
			mapping->lastDatetm = atoi((char*)parser->buf);
			parser->state = CC_MAPPINGIDSRCH_NODE_XML;
		} else if (CC_MAPPINGIDSRCH_NODE_LIKE_CNT == parser->state) {
			// <like_cnt>
			mapping->likeCnt = atol((char*)parser->buf);
			parser->state = CC_MAPPINGIDSRCH_NODE_XML;
		} else if (CC_MAPPINGIDSRCH_NODE_LIKE_FLG == parser->state) {
			// <like_flg>
			mapping->likeFlg = atoi((char*)parser->buf);
			parser->state = CC_MAPPINGIDSRCH_NODE_XML;
		} else if (CC_MAPPINGIDSRCH_NODE_RATINGNUM_YES == parser->state) {
			// <ratingnum_yes>
			mapping->ratingnumYes = atoi((char*)parser->buf);
			parser->state = CC_MAPPINGIDSRCH_NODE_XML;
		} else if (CC_MAPPINGIDSRCH_NODE_RATINGNUM_NO == parser->state) {
			// <ratingnum_no>
			mapping->ratingnumNo = atoi((char*)parser->buf);
			parser->state = CC_MAPPINGIDSRCH_NODE_XML;
		} else if (CC_MAPPINGIDSRCH_NODE_RATING_OWN == parser->state) {
			// <rating_own>
			mapping->ratingOwn = atoi((char*)parser->buf);
			parser->state = CC_MAPPINGIDSRCH_NODE_XML;
		} else if (CC_MAPPINGIDSRCH_NODE_GROUP_NAME == parser->state) {
			// <group_name>
			strcpy((char*)mapping->groupName, (char*)parser->buf);
			parser->state = CC_MAPPINGIDSRCH_NODE_XML;
		} else if (CC_MAPPINGIDSRCH_NODE_COMMENT_CNT == parser->state) {
			// <comment_cnt>
			mapping->commentCnt = atol((char*)parser->buf);
			parser->state = CC_MAPPINGIDSRCH_NODE_XML;
		} else if (CC_MAPPINGIDSRCH_NODE_ACCESS == parser->state) {
			// <access>
			strcpy((char*)mapping->access, (char*)parser->buf);
			parser->state = CC_MAPPINGIDSRCH_NODE_XML;
		} else if (CC_MAPPINGIDSRCH_NODE_CAN_DELETE == parser->state) {
			// <can_delete>
			mapping->canDelete = atoi((char*)parser->buf);
			parser->state = CC_MAPPINGIDSRCH_NODE_XML;
		} else if (CC_MAPPINGIDSRCH_NODE_OWN_FLG == parser->state) {
			// <own_flg>
			mapping->ownFlg = atoi((char*)parser->buf);
			parser->state = CC_MAPPINGIDSRCH_NODE_XML;
		} else if (CC_MAPPINGIDSRCH_NODE_USER_GUID == parser->state) {
			// <user_guid>
			strcpy((char*)mapping->guid, (char*)parser->buf);
			parser->state = CC_MAPPINGIDSRCH_NODE_XML;
		} else if (CC_MAPPINGIDSRCH_NODE_GROUP_ID == parser->state) {
			// <group_id>
			strcpy((char*)mapping->groupId, (char*)parser->buf);
			parser->state = CC_MAPPINGIDSRCH_NODE_XML;
		} else if (CC_MAPPINGIDSRCH_NODE_XML_CHILD == parser->state) {
			parser->state = CC_MAPPINGIDSRCH_NODE_XML;
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
void XMLCALL CC_MappingIdSrch_CharacterData(void *userData, const XML_Char *data, INT32 len)
{
	MAPPINGIDSRCH_PARSER *parser = (MAPPINGIDSRCH_PARSER*)userData;
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

		if (CC_MAPPINGIDSRCH_NODE_API_STATUS == parser->state) {
			// <api_status>
			if (CC_MAPPINGIDSRCH_XML_DATA_APISTATUS_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_MAPPINGIDSRCH_NODE_LAT == parser->state) {
			// <lat>
			if (CC_MAPPINGIDSRCH_XML_DATA_LAT_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_MAPPINGIDSRCH_NODE_LON == parser->state) {
			// <lon>
			if (CC_MAPPINGIDSRCH_XML_DATA_LON_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_MAPPINGIDSRCH_NODE_PID == parser->state) {
			// <pid>
			if (CC_MAPPINGIDSRCH_XML_DATA_PID_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_MAPPINGIDSRCH_NODE_X == parser->state) {
			// <x>
			if (CC_MAPPINGIDSRCH_XML_DATA_X_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_MAPPINGIDSRCH_NODE_Y == parser->state) {
			// <y>
			if (CC_MAPPINGIDSRCH_XML_DATA_Y_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_MAPPINGIDSRCH_NODE_MAPPING_ID == parser->state) {
			// <mpng_id>
			if (CC_MAPPINGIDSRCH_XML_DATA_MAPPINGID_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_MAPPINGIDSRCH_NODE_GENRE == parser->state) {
			// <genre>
			if (CC_MAPPINGIDSRCH_XML_DATA_GENRE_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_MAPPINGIDSRCH_NODE_USER == parser->state) {
			// <user>
			if (CC_MAPPINGIDSRCH_XML_DATA_USER_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_MAPPINGIDSRCH_NODE_AVATAR_URL_SMALL == parser->state) {
			// <avatar_url_small>
			if (CC_MAPPINGIDSRCH_XML_DATA_AVATAR_URL_SMALL_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_MAPPINGIDSRCH_NODE_AVATAR_URL_MEDIUM == parser->state) {
			// <avatar_url_medium>
			if (CC_MAPPINGIDSRCH_XML_DATA_AVATAR_URL_MEDIUM_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_MAPPINGIDSRCH_NODE_AVATAR_URL_LARGE == parser->state) {
			// <avatar_url_large>
			if (CC_MAPPINGIDSRCH_XML_DATA_AVATAR_URL_LARGE_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_MAPPINGIDSRCH_NODE_TEXT == parser->state) {
			// <text>
			if (CC_MAPPINGIDSRCH_XML_DATA_TEXT_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_MAPPINGIDSRCH_NODE_PICT == parser->state) {
			// <pict>
			if (CC_MAPPINGIDSRCH_XML_DATA_PICT_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_MAPPINGIDSRCH_NODE_MODE == parser->state) {
			// <mode>
			if (CC_MAPPINGIDSRCH_XML_DATA_MODE_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_MAPPINGIDSRCH_NODE_MPNG_DATETM == parser->state) {
			// <mpng_datetm>
			if (CC_MAPPINGIDSRCH_XML_DATA_MPNGDATETM_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_MAPPINGIDSRCH_NODE_RGST_DATETM == parser->state) {
			// <rgst_datetm>
			if (CC_MAPPINGIDSRCH_XML_DATA_RGSTDATETM_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_MAPPINGIDSRCH_NODE_ALT_DATETM == parser->state) {
			// <alt_datetm>
			if (CC_MAPPINGIDSRCH_XML_DATA_ALTDATETM_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_MAPPINGIDSRCH_NODE_LAST_DATETM == parser->state) {
			// <last_datetm>
			if (CC_MAPPINGIDSRCH_XML_DATA_LASTDATETM_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_MAPPINGIDSRCH_NODE_LIKE_CNT == parser->state) {
			// <like_cnt>
			if (CC_MAPPINGIDSRCH_XML_DATA_LIKECNT_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_MAPPINGIDSRCH_NODE_LIKE_FLG == parser->state) {
			// <like_flg>
			if (CC_MAPPINGIDSRCH_XML_DATA_LIKEFLG_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_MAPPINGIDSRCH_NODE_RATINGNUM_YES == parser->state) {
			// <ratingnum_yes>
			if (CC_MAPPINGIDSRCH_XML_DATA_RATINGNUM_YES_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_MAPPINGIDSRCH_NODE_RATINGNUM_NO == parser->state) {
			// <ratingnum_no>
			if (CC_MAPPINGIDSRCH_XML_DATA_RATINGNUM_NO_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_MAPPINGIDSRCH_NODE_RATING_OWN == parser->state) {
			// <rating_own>
			if (CC_MAPPINGIDSRCH_XML_DATA_RATING_OWN_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_MAPPINGIDSRCH_NODE_GROUP_NAME == parser->state) {
			// <group_name>
			if (CC_MAPPINGIDSRCH_XML_DATA_GROUPNAME_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_MAPPINGIDSRCH_NODE_COMMENT_CNT == parser->state) {
			// <comment_cnt>
			if (CC_MAPPINGIDSRCH_XML_DATA_COMMENTCNT_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_MAPPINGIDSRCH_NODE_ACCESS == parser->state) {
			// <access>
			if (CC_MAPPINGIDSRCH_XML_DATA_ACCESS_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_MAPPINGIDSRCH_NODE_CAN_DELETE == parser->state) {
			// <can_delete>
			if (CC_MAPPINGIDSRCH_XML_DATA_CAN_DELETE_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_MAPPINGIDSRCH_NODE_OWN_FLG == parser->state) {
			// <own_flg>
			if (CC_MAPPINGIDSRCH_XML_DATA_OWN_FLG_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_MAPPINGIDSRCH_NODE_USER_GUID == parser->state) {
			// <user_guid>
			if (CC_MAPPINGIDSRCH_XML_DATA_GUID_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_MAPPINGIDSRCH_NODE_GROUP_ID == parser->state) {
			// <group_id>
			if (CC_MAPPINGIDSRCH_XML_DATA_GROUPID_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		}
	} while (0);

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}
