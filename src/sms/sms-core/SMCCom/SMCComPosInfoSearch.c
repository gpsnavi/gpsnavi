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

#define CC_POSINFO_SRCH_SEND_BODY_SIZE			512

#define	CC_POSINFO_XML_NODE_USER_LIST			"user_list"
#define	CC_POSINFO_XML_NODE_ARRAY_ITEM			"array_item"
#define	CC_POSINFO_XML_NODE_GUID				"guid"
#define	CC_POSINFO_XML_NODE_USER_NAME			"user_name"
#define	CC_POSINFO_XML_NODE_LAST_ACTION			"last_action"
#define	CC_POSINFO_XML_NODE_LAT					"lat"
#define	CC_POSINFO_XML_NODE_LON					"lon"

#define	CC_POSINFO_XML_DATA_GUID_SIZE			CC_CMN_GUID_STR_SIZE
#define	CC_POSINFO_XML_DATA_USER_NAME_SIZE		CC_CMN_USERNAME_STR_SIZE
#define	CC_POSINFO_XML_DATA_LAST_ACTION_SIZE	CC_MAX_DATE
#define	CC_POSINFO_XML_DATA_LAT_SIZE			20
#define	CC_POSINFO_XML_DATA_LON_SIZE			20

// 位置情報取得レスポンスXML情報
typedef struct _POSINFO {
	INT32			posInfoListNum;
	SMPOSITIONINFO	*posInfo;
	INT32			*status;
	Char			*apiStatus;
} POSINFO;

// 位置情報取得XMLパーサ
typedef struct _POSINFO_PARSER {
	INT32			state;
	Char			*buf;
	POSINFO			posInfo;
	INT32			posInfoNum;
} POSINFO_PARSER;

enum PosInfoStatus {
	CC_POSINFO_NODE_NONE = 0,
	CC_POSINFO_NODE_ELGG,
	CC_POSINFO_NODE_ELGG_CHILD,
	CC_POSINFO_NODE_STATUS,
	CC_POSINFO_NODE_RESULT,
	CC_POSINFO_NODE_RESULT_CHILD,
	CC_POSINFO_NODE_API_STATUS,
	CC_POSINFO_NODE_USER_LIST,
	CC_POSINFO_NODE_USER_LIST_CHILD,
	CC_POSINFO_NODE_ARRAY_ITEM,
	CC_POSINFO_NODE_ARRAY_ITEM_CHILD,
	CC_POSINFO_NODE_GUID,
	CC_POSINFO_NODE_USER_NAME,
	CC_POSINFO_NODE_LAST_ACTION,
	CC_POSINFO_NODE_LAT,
	CC_POSINFO_NODE_LON
};

//------------------------------------------------
// 変数定義
//------------------------------------------------
static INT32	CB_Result;				// 処理結果 (コールバック関数用)
static Bool		CB_IsXmlFile;

//------------------------------------------------
// 関数定義
//------------------------------------------------
static void CC_PosInfoSearch_CreateUri(const T_CC_CMN_SMS_API_PRM *param, Char *uri);
static void CC_PosInfoSearch_CreateBody(const T_CC_CMN_SMS_API_PRM *parm, const Char *roomNo, const SMPOSITIONINFO *posInfo, Char *body);
static E_SC_RESULT CC_PosInfoSearch_AnalyzeHttpResp(const Char *body, E_CONTEXT_TYPE contextType, SMPOSITIONINFO *posInfo, INT32 *posInfoNum, SMCALOPT *opt, Char *apiStatus);
static E_SC_RESULT CC_PosInfoSearch_XmlParse(const Char* xml, T_CC_CMN_SMS_RESPONSE_INFO* resp_inf, SMPOSITIONINFO *posInfo, INT32 *posInfoNum, SMCALOPT *opt);
static void XMLCALL CC_PosInfoSearch_StartElement(void *userData, const char *name, const char **atts);
static void XMLCALL CC_PosInfoSearch_EndElement(void *userData, const char *name);
static void XMLCALL CC_PosInfoSearch_CharacterData(void *userData, const XML_Char *data, INT32 len);

/**
 * @brief 位置情報を共有する
 * @param [IN] smcal        SMCAL
 * @param [IN] parm         APIパラメータ
 * @param [IN] posInfo      位置情報
 * @param [IN] recv         センタ受信データ
 * @param [OUT] apiStatus   APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_PosInfoSearch_SendRecv(SMCAL *smcal,
									  const T_CC_CMN_SMS_API_PRM *parm,
									  const Char *roomNo,
									  SMPOSITIONINFO *posInfo,
									  INT32 *posInfoNum,
									  const Char *fileName,
									  Char *recv,
									  UINT32 recvBufSize,
									  Char *apiStatus,
									  Bool isPolling)
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
		if (isPolling) {
			opt.cancel = SCC_IsCancel_Polling;
		} else {
			opt.cancel = SCC_IsCancel;
		}
		CC_GetTempDirPath(opt.resFilePath);
		strcat(opt.resFilePath, fileName);
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
		data = (Char*)SCC_MALLOC(CC_POSINFO_SRCH_SEND_BODY_SIZE);
		if (NULL == data) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// URI生成
		CC_PosInfoSearch_CreateUri(parm, uri);

		// body生成
		CC_PosInfoSearch_CreateBody(parm, roomNo, posInfo, data);

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
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SC_CAL_AnalyzeResponseStatus error, " HERE);
				ret = ConvertResult(calRet);
			}
			break;
		}

		// レスポンス解析
		ret = CC_PosInfoSearch_AnalyzeHttpResp(body, contextType, posInfo, posInfoNum, &opt, apiStatus);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_PosInfoSearch_AnalyzeHttpResp error, " HERE);
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
void CC_PosInfoSearch_CreateUri(const T_CC_CMN_SMS_API_PRM *parm, Char *uri)
{
	sprintf((char*)uri,
			"%s\?method=Pinfo.srch",
			parm->ApiPrmNavi.common_uri
	);
}

/**
 * @brief body生成
 * @param [IN]  parm        APIパラメータ
 * @param [IN]  roomNo      ルームNo
 * @param [IN]  posInfo     位置情報
 * @param [OUT] body        body
 */
void CC_PosInfoSearch_CreateBody(const T_CC_CMN_SMS_API_PRM *parm,
								 const Char *roomNo,
								 const SMPOSITIONINFO *posInfo,
								 Char *body)
{
	sprintf((char*)body,
			"term_id=%s&term_sig=%s&guid=%s&user_sig=%s&room_no=%s",
			parm->ApiPrmMups.new_term_id,
			parm->ApiPrmMups.term_sig,
			parm->ApiPrmMups.guid,
			parm->ApiPrmMups.user_sig,
			roomNo
	);
}

/**
 * @brief レスポンス解析
 * @param [IN] body         xmlデータ
 * @param [IN] contextType  コンテキスト
 * @param [OUT] posInfo     位置情報
 * @param [IN] opt          オプション情報
 * @param [OUT] apiStatus   APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_PosInfoSearch_AnalyzeHttpResp(const Char *body,
											 E_CONTEXT_TYPE contextType,
											 SMPOSITIONINFO *posInfo,
											 INT32 *posInfoNum,
											 SMCALOPT *opt,
											 Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	T_CC_CMN_SMS_RESPONSE_INFO	rsp_inf;
	//Char	apiSts[CC_CMN_XML_RES_STS_CODE_SIZE] = {};

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		if (E_TEXT_XML != contextType) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"Content-Type error, " HERE);
			ret = e_SC_RESULT_SMS_API_ERR;
			break;
		}

		// XML解析
		rsp_inf.apiSts = apiStatus;

		ret = CC_PosInfoSearch_XmlParse((const Char*)body, &rsp_inf, posInfo, posInfoNum, opt);
		if(e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_PosInfoSearch_XmlParse error, " HERE);
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
 * @brief Pinfo.alt応答メッセージ解析
 * @param [IN] xml      XMLファイルのフルパス
 * @param [IN] resp_inf CICレスポンス情報
 * @param [OUT] posInfo 位置情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_PosInfoSearch_XmlParse(const Char* xml,
									  T_CC_CMN_SMS_RESPONSE_INFO* resp_inf,
									  SMPOSITIONINFO *posInfo,
									  INT32 *posInfoNum,
									  SMCALOPT *opt)
{
	E_SC_RESULT		ret = e_SC_RESULT_SUCCESS;
	POSINFO_PARSER	posinfoParser = {};
	Char buf[CC_CMN_XML_PARSE_DATA_SIZE + 1] = {};
	XML_Parser parser = NULL;
	INT32	done = 0;
	INT32	len = 0;
	INT32	parsedLen = 0;
	FILE	*fp = NULL;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// 初期化
		resp_inf->sts = 0;
		posinfoParser.buf = (Char*)SCC_MALLOC(CC_CMN_XML_BUF_SIZE + 1);
		if (NULL == posinfoParser.buf) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			CB_Result = CC_CMN_RESULT_MALLOC_ERR;
			ret = CB_Result;
			break;
		}
		posinfoParser.posInfo.posInfoListNum = 0;
		posinfoParser.posInfo.status = &resp_inf->sts;
		posinfoParser.posInfo.apiStatus = &resp_inf->apiSts[0];
		posinfoParser.posInfo.posInfo = posInfo;
		posinfoParser.posInfoNum = *posInfoNum;
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
		XML_SetUserData(parser, &posinfoParser);
		XML_SetElementHandler(parser, CC_PosInfoSearch_StartElement, CC_PosInfoSearch_EndElement);
		XML_SetCharacterDataHandler(parser, CC_PosInfoSearch_CharacterData);

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
			*posInfoNum = posinfoParser.posInfo.posInfoListNum;
		}
	} while (0);

	if (NULL != posinfoParser.buf) {
		SCC_FREE(posinfoParser.buf);
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
void XMLCALL CC_PosInfoSearch_StartElement(void *userData, const char *name, const char **atts)
{
	POSINFO_PARSER *parser = (POSINFO_PARSER*)userData;

	//SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

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
			parser->state = CC_POSINFO_NODE_ELGG;
		}

		// <elgg>
		if (CC_POSINFO_NODE_ELGG == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_STATUS)) {
				// <status>
				parser->state = CC_POSINFO_NODE_STATUS;
			} else if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_RESULT)) {
				// <result>
				parser->state = CC_POSINFO_NODE_RESULT;
			} else if (0 != strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_ELGG)) {
				parser->state = CC_POSINFO_NODE_ELGG_CHILD;
			}
		} else if (CC_POSINFO_NODE_RESULT == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_API_STATUS)) {
				// <api_status>
				parser->state = CC_POSINFO_NODE_API_STATUS;
			} else if (0 == strcmp((char*)name, (char*)CC_POSINFO_XML_NODE_USER_LIST)) {
				// <user_list>
				parser->state = CC_POSINFO_NODE_USER_LIST;
			} else {
				parser->state = CC_POSINFO_NODE_RESULT_CHILD;
			}
		} else if (CC_POSINFO_NODE_USER_LIST == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_POSINFO_XML_NODE_ARRAY_ITEM)) {
				// <array_item>
				parser->state = CC_POSINFO_NODE_ARRAY_ITEM;
			} else {
				parser->state = CC_POSINFO_NODE_USER_LIST_CHILD;
			}
		} else if (CC_POSINFO_NODE_ARRAY_ITEM == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_POSINFO_XML_NODE_GUID)) {
				// <guid>
				parser->state = CC_POSINFO_NODE_GUID;
			} else if (0 == strcmp((char*)name, (char*)CC_POSINFO_XML_NODE_USER_NAME)) {
				// <user_name>
				parser->state = CC_POSINFO_NODE_USER_NAME;
			} else if (0 == strcmp((char*)name, (char*)CC_POSINFO_XML_NODE_LAST_ACTION)) {
				// <last_action>
				parser->state = CC_POSINFO_NODE_LAST_ACTION;
			} else if (0 == strcmp((char*)name, (char*)CC_POSINFO_XML_NODE_LAT)) {
				// <lat>
				parser->state = CC_POSINFO_NODE_LAT;
			} else if (0 == strcmp((char*)name, (char*)CC_POSINFO_XML_NODE_LON)) {
				// <lon>
				parser->state = CC_POSINFO_NODE_LON;
			} else {
				parser->state = CC_POSINFO_NODE_ARRAY_ITEM_CHILD;
			}
		} else {
			// 上記以外
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"format error[%s], " HERE, name);
			CB_Result = e_SC_RESULT_SMS_API_ERR;
			break;
		}
	} while (0);

	//SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}

/**
 * @brief タグ解析終了
 * @param [IN/OUT] userData ユーザデータ
 * @param [IN] name     タグ名
 */
void XMLCALL CC_PosInfoSearch_EndElement(void *userData, const char *name)
{
	POSINFO_PARSER *parser = (POSINFO_PARSER*)userData;
	SMPOSITIONINFO	*pos = NULL;

	//SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

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
		if (NULL == name) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[name], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}

		if (CC_MAX_POSINFO_NUM <= parser->posInfo.posInfoListNum) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"parser->posInfo.posInfoListNum max over, " HERE);
			CB_Result = e_SC_RESULT_SMS_API_ERR;
			break;
		}
		pos = &parser->posInfo.posInfo[parser->posInfo.posInfoListNum];

		if (CC_POSINFO_NODE_STATUS == parser->state) {
			// <status>
			*(parser->posInfo.status) = atoi((char*)parser->buf);
			parser->state = CC_POSINFO_NODE_ELGG;
		} else if (CC_POSINFO_NODE_RESULT == parser->state) {
			// <result>
			parser->state = CC_POSINFO_NODE_ELGG;
		} else if (CC_POSINFO_NODE_API_STATUS == parser->state) {
			// <api_status>
			strcpy((char*)parser->posInfo.apiStatus, (char*)parser->buf);
			parser->state = CC_POSINFO_NODE_RESULT;
		} else if (CC_POSINFO_NODE_USER_LIST == parser->state) {
			// <user_list>
			parser->state = CC_POSINFO_NODE_RESULT;
		} else if (CC_POSINFO_NODE_ARRAY_ITEM == parser->state) {
			// <array_item>
			parser->state = CC_POSINFO_NODE_USER_LIST;
			parser->posInfo.posInfoListNum++;
		} else if (CC_POSINFO_NODE_GUID == parser->state) {
			// <guid>
			strcpy((char*)pos->guid, (char*)parser->buf);
			parser->state = CC_POSINFO_NODE_ARRAY_ITEM;
		} else if (CC_POSINFO_NODE_USER_NAME == parser->state) {
			// <user_name>
			strcpy((char*)pos->userName, (char*)parser->buf);
			parser->state = CC_POSINFO_NODE_ARRAY_ITEM;
		} else if (CC_POSINFO_NODE_LAST_ACTION == parser->state) {
			// <last_action>
			strcpy((char*)pos->lastAction, (char*)parser->buf);
			parser->state = CC_POSINFO_NODE_ARRAY_ITEM;
		} else if (CC_POSINFO_NODE_LAT == parser->state) {
			// <lat>
			if (EOS != *(parser->buf)) {
				pos->latFlg = true;
				pos->lat = atof((char*)parser->buf);
			}
			parser->state = CC_POSINFO_NODE_ARRAY_ITEM;
		} else if (CC_POSINFO_NODE_LON == parser->state) {
			// <lon>
			if (EOS != *(parser->buf)) {
				pos->lonFlg = true;
				pos->lon = atof((char*)parser->buf);
			}
			parser->state = CC_POSINFO_NODE_ARRAY_ITEM;
		} else if (CC_POSINFO_NODE_ELGG_CHILD == parser->state) {
			 parser->state = CC_POSINFO_NODE_ELGG;
		} else if (CC_POSINFO_NODE_RESULT_CHILD == parser->state) {
			 parser->state = CC_POSINFO_NODE_RESULT;
		} else if (CC_POSINFO_NODE_USER_LIST_CHILD == parser->state) {
			parser->state = CC_POSINFO_NODE_USER_LIST;
		} else if (CC_POSINFO_NODE_ARRAY_ITEM_CHILD == parser->state) {
			parser->state = CC_POSINFO_NODE_ARRAY_ITEM;
		}
	} while (0);

	//SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}

/**
 * @brief 解析データ
 * @param [IN] userData ユーザデータ
 * @param [IN] data     データ
 * @param [IN] len      データ長
 */
void XMLCALL CC_PosInfoSearch_CharacterData(void *userData, const XML_Char *data, INT32 len)
{
	POSINFO_PARSER *parser = (POSINFO_PARSER*)userData;
	//char buf[CC_CMN_XML_PARSE_DATA_SIZE + 1] = {};
	INT32	bufLen = 0;

	//SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

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

		if (CC_POSINFO_NODE_STATUS == parser->state) {
			// <status>
			if (CC_CMN_XML_RES_STS_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_POSINFO_NODE_API_STATUS == parser->state) {
			// <api_status>
			if (CC_CMN_XML_RES_STS_CODE_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_POSINFO_NODE_USER_LIST == parser->state) {
			// <user_list>
		} else if (CC_POSINFO_NODE_ARRAY_ITEM == parser->state) {
			// <array_item>
		} else if (CC_POSINFO_NODE_GUID == parser->state) {
			// <guid>
			if (CC_POSINFO_XML_DATA_GUID_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_POSINFO_NODE_USER_NAME == parser->state) {
			// <user_name>
			if (CC_POSINFO_XML_DATA_USER_NAME_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_POSINFO_NODE_LAST_ACTION == parser->state) {
			// <last_action>
			if (CC_POSINFO_XML_DATA_LAST_ACTION_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_POSINFO_NODE_LAT == parser->state) {
			// <lat>
			if (CC_POSINFO_XML_DATA_LAT_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_POSINFO_NODE_LON == parser->state) {
			// <lon>
			if (CC_POSINFO_XML_DATA_LON_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		}
	} while (0);

	//SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}
