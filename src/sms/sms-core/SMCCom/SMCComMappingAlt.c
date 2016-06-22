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

// マッピング更新レスポンスXML情報
typedef struct _MAPPINGINFO {
	Char			*apiStatus;
} MAPPINGINFO;

// マッピング更新XMLパーサ
typedef struct _MAPPINGALT_PARSER {
	INT32			state;
	Char			*buf;
	MAPPINGINFO		mappingInfo;
} MAPPINGALT_PARSER;

// mapping/alt
enum MappingAltStatus {
	CC_MAPPINGALT_NODE_NONE = 0,
	CC_MAPPINGALT_NODE_XML,
	CC_MAPPINGALT_NODE_XML_CHILD,
	CC_MAPPINGALT_NODE_STATUS,
	CC_MAPPINGALT_NODE_API_STATUS,
	CC_MAPPINGALT_NODE_MAPPING_ID
};

typedef struct _BODY_BUF {
	Char	lat[24];
	Char	lon[24];
	Char	pid[14];
	Char	x[12];
	Char	y[12];
	Char	mode[2];
	Char	accessFlg[2];
	Char	pictureDel[2];
	Char	cause[2];
	Char	time[12];
	Char	reserve[2];
} BODY_BUF;

//------------------------------------------------
// 変数定義
//------------------------------------------------
static INT32	CB_Result;				// 処理結果 (コールバック関数用)

//------------------------------------------------
// 関数定義
//------------------------------------------------
static void CC_MappingAlt_CreateUri(const T_CC_CMN_SMS_API_PRM *param, Char *uri);
static E_SC_RESULT CC_MappingAlt_CreatePostParam(const T_CC_CMN_SMS_API_PRM *parm, const SMMAPPINGINFO *mapping, SMCALPOSTPARAM *postParam, INT32 *postParamNum, BODY_BUF *bodyBuf);
static E_SC_RESULT CC_MappingAlt_AnalyzeHttpResp(const Char *body, E_CONTEXT_TYPE contextType, SMMAPPINGINFO *mapping, SMCALOPT *opt, Char *apiStatus);
static E_SC_RESULT CC_MappingAlt_XmlParse(const Char* xml, T_CC_CMN_SMS_RESPONSE_INFO* resp_inf, SMMAPPINGINFO* mapping, SMCALOPT *opt);
static void XMLCALL CC_MappingAlt_StartElement(void *userData, const char *name, const char **atts);
static void XMLCALL CC_MappingAlt_EndElement(void *userData, const char *name);
static void XMLCALL CC_MappingAlt_CharacterData(void *userData, const XML_Char *data, INT32 len);

/**
 * @brief マッピング更新
 * @param [IN] smcal        SMCAL
 * @param [IN] parm         APIパラメータ
 * @param [IN/OUT] mapping  マッピング情報
 * @param [IN] recv         センタ受信データ
 * @param [OUT] apiStatus   APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_MappingAlt_SendRecv(SMCAL *smcal,
								   const T_CC_CMN_SMS_API_PRM *parm,
								   SMMAPPINGINFO *mapping,
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
	SMCALPOSTPARAM	postParam[17] = {};
	INT32	postParamNum = 0;
	BODY_BUF bodyBuf = {};
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
			SCC_LOG_ErrorPrint(SC_TAG_CC, "SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// URI生成
		CC_MappingAlt_CreateUri(parm, uri);

		// POSTパラメータ生成
		ret = CC_MappingAlt_CreatePostParam(parm, mapping, postParam, &postParamNum, &bodyBuf);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MappingAlt_CreatePostParam error, " HERE);
			break;
		}

		// HTTPデータ送受信
		calRet = SC_CAL_PostRequest_Multipart(smcal, uri, postParam, postParamNum, recv, recvBufSize, &recvSize, &opt);
		if (e_SC_CAL_RESULT_SUCCESS != calRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SC_CAL_PostRequest_Multipart error, " HERE);
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
		ret = CC_MappingAlt_AnalyzeHttpResp(body, contextType, mapping, &opt, apiStatus);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MappingAlt_AnalyzeHttpResp error, " HERE);
			break;
		}
	} while (0);

	// メモリ解放
	if (NULL != uri) {
		SCC_FREE(uri);
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
void CC_MappingAlt_CreateUri(const T_CC_CMN_SMS_API_PRM *parm, Char *uri)
{
	sprintf((char*)uri,
			"%smpng/alt/",
			parm->ApiPrmNavi.sms_sp_uri
	);
}

/**
 * @brief POSTパラメータ生成(Multipart)
 * @param [IN]  parm            APIパラメータ
 * @param [IN/OUT] mapping      マッピング情報
 * @param [OUT] postParam       POSTパラメータ
 * @param [OUT] postParamNum    POSTパラメータ数
 * @param [OUT] bodyBuf         body
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_MappingAlt_CreatePostParam(const T_CC_CMN_SMS_API_PRM *parm,
										  const SMMAPPINGINFO *mapping,
										  SMCALPOSTPARAM *postParam,
										  INT32 *postParamNum,
										  BODY_BUF *bodyBuf)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	struct stat	st = {};
	//time_t	nowTime = {0};
	CC_IMAGE_MIMETYPE	mimeType = CC_IMAGE_MIMETYPE_NONE;

	do {
		postParam[num].type = SMCALPOSTPARAM_DATATYPE_TEXT;
		postParam[num].name = "\"term_id\"";
		postParam[num].len = strlen(parm->ApiPrmMups.new_term_id);
		postParam[num].data = (Char*)parm->ApiPrmMups.new_term_id;
		num++;

		postParam[num].type = SMCALPOSTPARAM_DATATYPE_TEXT;
		postParam[num].name = "\"term_sig\"";
		postParam[num].len = strlen(parm->ApiPrmMups.term_sig);
		postParam[num].data = (Char*)parm->ApiPrmMups.term_sig;
		num++;

		postParam[num].type = SMCALPOSTPARAM_DATATYPE_TEXT;
		postParam[num].name = "\"guid\"";
		postParam[num].len = strlen(parm->ApiPrmMups.guid);
		postParam[num].data = (Char*)parm->ApiPrmMups.guid;
		num++;

		postParam[num].type = SMCALPOSTPARAM_DATATYPE_TEXT;
		postParam[num].name = "\"user_sig\"";
		postParam[num].len = strlen(parm->ApiPrmMups.user_sig);
		postParam[num].data = (Char*)parm->ApiPrmMups.user_sig;
		num++;

		postParam[num].type = SMCALPOSTPARAM_DATATYPE_TEXT;
		postParam[num].name = "\"mpng_id\"";
		postParam[num].len = strlen(mapping->mappingId);
		postParam[num].data = (Char*)mapping->mappingId;
		num++;

		postParam[num].type = SMCALPOSTPARAM_DATATYPE_TEXT;
		postParam[num].name = "\"lat\"";
		sprintf(bodyBuf->lat, "%f", mapping->lat);
		postParam[num].len = strlen(bodyBuf->lat);
		postParam[num].data = bodyBuf->lat;
		num++;

		postParam[num].type = SMCALPOSTPARAM_DATATYPE_TEXT;
		postParam[num].name = "\"lon\"";
		sprintf(bodyBuf->lon, "%f", mapping->lon);
		postParam[num].len = strlen(bodyBuf->lon);
		postParam[num].data = bodyBuf->lon;
		num++;

		postParam[num].type = SMCALPOSTPARAM_DATATYPE_TEXT;
		postParam[num].name = "\"pid\"";
		sprintf(bodyBuf->pid, "%ld", mapping->parcelId);
		postParam[num].len = strlen(bodyBuf->pid);
		postParam[num].data = bodyBuf->pid;
		num++;

		postParam[num].type = SMCALPOSTPARAM_DATATYPE_TEXT;
		postParam[num].name = "\"x\"";
		sprintf(bodyBuf->x, "%d", mapping->x);
		postParam[num].len = strlen(bodyBuf->x);
		postParam[num].data = bodyBuf->x;
		num++;

		postParam[num].type = SMCALPOSTPARAM_DATATYPE_TEXT;
		postParam[num].name = "\"y\"";
		sprintf(bodyBuf->y, "%d", mapping->y);
		postParam[num].len = strlen(bodyBuf->y);
		postParam[num].data = bodyBuf->y;
		num++;

		if ((NULL != mapping->text) && (EOS != mapping->text[0])) {
			postParam[num].type = SMCALPOSTPARAM_DATATYPE_TEXT;
			postParam[num].name = "\"text\"";
			postParam[num].len = strlen(mapping->text);
			postParam[num].data = (Char*)mapping->text;
			num++;
		}

		if ((NULL != mapping->pic) && (EOS != mapping->pic[0])) {
			postParam[num].type = SMCALPOSTPARAM_DATATYPE_FILE;
			// MIME TYPE取得
			ret = SCC_GetImageMIMEType(mapping->pic, &mimeType);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_GetImageMIMEType error, " HERE);
				ret = e_SC_RESULT_BADPARAM;
				break;
			}
			if (CC_IMAGE_MIMETYPE_JPG == mimeType) {
				postParam[num].name = "\"pic\"; filename=\"pic.jpeg\"\r\nContent-Type: image/jpeg";
			} else if (CC_IMAGE_MIMETYPE_GIF == mimeType) {
				postParam[num].name = "\"pic\"; filename=\"pic.gif\"\r\nContent-Type: image/gif";
			} else if (CC_IMAGE_MIMETYPE_PNG == mimeType) {
				postParam[num].name = "\"pic\"; filename=\"pic.png\"\r\nContent-Type: image/png";
			} else {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"pic file format error, " HERE);
				ret = e_SC_RESULT_BADPARAM;
				break;
			}
			// ファイルサイズ取得
			if (0 != stat((char*)mapping->pic, &st)) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, "stat error[%s] (0x%08x), " HERE, mapping->pic, errno);
				ret = e_SC_RESULT_FILE_ACCESSERR;
				break;
			}
			postParam[num].len = (UINT32)st.st_size;
			postParam[num].data = (Char*)mapping->pic;
			num++;
		}

		postParam[num].type = SMCALPOSTPARAM_DATATYPE_TEXT;
		postParam[num].name = "\"mode\"";
		sprintf(bodyBuf->mode, "%d", mapping->mode);
		postParam[num].len = strlen(bodyBuf->mode);
		postParam[num].data = bodyBuf->mode;
		num++;

		postParam[num].type = SMCALPOSTPARAM_DATATYPE_TEXT;
		postParam[num].name = "\"access_flg\"";
		sprintf(bodyBuf->accessFlg, "%d", mapping->accessFlg);
		postParam[num].len = strlen(bodyBuf->accessFlg);
		postParam[num].data = bodyBuf->accessFlg;
		num++;

		postParam[num].type = SMCALPOSTPARAM_DATATYPE_TEXT;
		postParam[num].name = "\"picture_del\"";
		sprintf(bodyBuf->pictureDel, "%d", mapping->pictureDel);
		postParam[num].len = strlen(bodyBuf->pictureDel);
		postParam[num].data = bodyBuf->pictureDel;
		num++;

		postParam[num].type = SMCALPOSTPARAM_DATATYPE_TEXT;
		postParam[num].name = "\"cause\"";
		sprintf(bodyBuf->cause, "%d", mapping->cause);
		postParam[num].len = strlen(bodyBuf->cause);
		postParam[num].data = bodyBuf->cause;
		num++;

		postParam[num].type = SMCALPOSTPARAM_DATATYPE_TEXT;
		postParam[num].name = "\"app_ver\"";
		postParam[num].len = strlen(parm->ApiPrmNavi.appVer);
		postParam[num].data = (Char*)parm->ApiPrmNavi.appVer;
		num++;

		*postParamNum = num;
	} while (0);

	return (ret);
}

/**
 * @brief レスポンス解析
 * @param [IN] body         xmlデータ
 * @param [IN] contextType  コンテキスト
 * @param [IN/OUT] mapping  マッピング情報
 * @param [IN] opt          オプション情報
 * @param [OUT] apiStatus   APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_MappingAlt_AnalyzeHttpResp(const Char *body,
										  E_CONTEXT_TYPE contextType,
										  SMMAPPINGINFO *mapping,
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

		ret = CC_MappingAlt_XmlParse((const Char*)body, &rsp_inf, mapping, opt);
		if(e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MappingAlt_XmlParse error, " HERE);
			ret = e_SC_RESULT_SMS_API_ERR;
			break;
		}

		// 正常系のXMLとして解析できなかった場合
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
 * @brief mpng/alt/応答メッセージ解析
 * @param [IN] xml      XMLファイルのフルパス
 * @param [IN] resp_inf CICレスポンス情報
 * @param [OUT] mapping マッピング情報格納領域
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_MappingAlt_XmlParse(const Char* xml,
								   T_CC_CMN_SMS_RESPONSE_INFO* resp_inf,
								   SMMAPPINGINFO* mapping,
								   SMCALOPT *opt)
{
	E_SC_RESULT		ret = e_SC_RESULT_SUCCESS;
	MAPPINGALT_PARSER	mappingaltParser = {};
	Char buf[CC_CMN_XML_PARSE_DATA_SIZE + 1] = {};
	XML_Parser parser = NULL;
	INT32	done = 0;
	INT32	len = 0;
	INT32	parsedLen = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// 初期化
		resp_inf->sts = 0;
		mappingaltParser.buf = (Char*)SCC_MALLOC(CC_CMN_XML_BUF_SIZE + 1);
		if (NULL == mappingaltParser.buf) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			CB_Result = e_SC_RESULT_MALLOC_ERR;
			ret = CB_Result;
			break;
		}
		mappingaltParser.mappingInfo.apiStatus = &resp_inf->apiSts[0];
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
		XML_SetUserData(parser, &mappingaltParser);
		XML_SetElementHandler(parser, CC_MappingAlt_StartElement, CC_MappingAlt_EndElement);
		XML_SetCharacterDataHandler(parser, CC_MappingAlt_CharacterData);

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

	if (NULL != mappingaltParser.buf) {
		SCC_FREE(mappingaltParser.buf);
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
void XMLCALL CC_MappingAlt_StartElement(void *userData, const char *name, const char **atts)
{
	MAPPINGALT_PARSER *parser = (MAPPINGALT_PARSER*)userData;

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
			parser->state = CC_MAPPINGALT_NODE_XML;
		}

		// <xml>
		if (CC_MAPPINGALT_NODE_XML == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_API_STATUS)) {
				// <api_status>
				parser->state = CC_MAPPINGALT_NODE_API_STATUS;
			} else if (0 != strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_XML)) {
				parser->state = CC_MAPPINGALT_NODE_XML_CHILD;
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
void XMLCALL CC_MappingAlt_EndElement(void *userData, const char *name)
{
	MAPPINGALT_PARSER *parser = (MAPPINGALT_PARSER*)userData;

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

		if (CC_MAPPINGALT_NODE_API_STATUS == parser->state) {
			// <api_status>
			strcpy((char*)parser->mappingInfo.apiStatus, (char*)parser->buf);
			parser->state = CC_MAPPINGALT_NODE_XML;
		} else if (CC_MAPPINGALT_NODE_XML_CHILD == parser->state) {
			 parser->state = CC_MAPPINGALT_NODE_XML;
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
void XMLCALL CC_MappingAlt_CharacterData(void *userData, const XML_Char *data, INT32 len)
{
	MAPPINGALT_PARSER *parser = (MAPPINGALT_PARSER*)userData;
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

		if (CC_MAPPINGALT_NODE_API_STATUS == parser->state) {
			// <api_status>
			if (CC_CMN_XML_RES_STS_CODE_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		}
	} while (0);

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}
