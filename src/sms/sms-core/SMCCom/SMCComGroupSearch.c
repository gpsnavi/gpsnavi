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

#define	CC_GROUPSRCH_SEND_BODY_SIZE						2048

#define	CC_GROUPSRCH_XML_NODE_COUNT						"count"
#define	CC_GROUPSRCH_XML_NODE_GROUP_LIST				"group_list"
#define	CC_GROUPSRCH_XML_NODE_ITEM						"item"
#define	CC_GROUPSRCH_XML_NODE_GROUP_ID					"group_id"
#define	CC_GROUPSRCH_XML_NODE_NAME						"name"
#define	CC_GROUPSRCH_XML_NODE_DISCRIPTION_BRIEF			"discription_brief"
#define	CC_GROUPSRCH_XML_NODE_ACCESS					"access"
#define	CC_GROUPSRCH_XML_NODE_PICT_URL_SMALL			"pict_url_small"
#define	CC_GROUPSRCH_XML_NODE_PICT_URL_MEDIUM			"pict_url_medium"
#define	CC_GROUPSRCH_XML_NODE_PICT_URL_LARGE			"pict_url_large"
#define	CC_GROUPSRCH_XML_NODE_MEMBER_NUM				"member_num"
#define	CC_GROUPSRCH_XML_NODE_INDEX						"index"

#define	CC_GROUPSRCH_RES_FILE							"group.srch"

#define	CC_GROUPSRCH_XML_DATA_COUNT_SIZE				11
#define	CC_GROUPSRCH_XML_DATA_APISTATUS_SIZE			CC_CMN_XML_RES_STS_CODE_SIZE
#define	CC_GROUPSRCH_XML_DATA_GROUP_ID_SIZE				SCC_CMN_GROUP_ID
#define	CC_GROUPSRCH_XML_DATA_NAME_SIZE					SCC_CMN_GROUP_NAME
#define	CC_GROUPSRCH_XML_DATA_DISCRIPTION_BRIEF_SIZE	SCC_CMN_DISCRIPTION_BRIEF
#define	CC_GROUPSRCH_XML_DATA_ACCESS_SIZE				30
#define	CC_GROUPSRCH_XML_DATA_PICT_URL_SMALL_SIZE		SCC_MAX_URL
#define	CC_GROUPSRCH_XML_DATA_PICT_URL_MEDIUM_SIZE		SCC_MAX_URL
#define	CC_GROUPSRCH_XML_DATA_PICT_URL_LARGE_SIZE		SCC_MAX_URL
#define	CC_GROUPSRCH_XML_DATA_MEMBER_NUM_SIZE			11
#define	CC_GROUPSRCH_XML_DATA_INDEX_SIZE				2

// サークル検索レスポンスXML情報
typedef struct _GEMINFO {
	INT32			groupListNum;
	SMGROUP	*groupList;
	INT32			*status;
	Char			*apiStatus;
	Char			*groupNum;
} GROUPINFO;

// サークル検索XMLパーサ
typedef struct _GROUPSRCH_PARSER {
	INT32			state;
	Char			*buf;
	GROUPINFO		groupInfo;
	INT32			groupInfoMaxNum;
} GROUPSRCH_PARSER;

// group/srch
enum GroupSrchStatus {
	CC_GROUPSRCH_NODE_NONE = 0,
	CC_GROUPSRCH_NODE_XML,
	CC_GROUPSRCH_NODE_XML_CHILD,
	CC_GROUPSRCH_NODE_STATUS,
	CC_GROUPSRCH_NODE_API_STATUS,
	CC_GROUPSRCH_NODE_CNT,
	CC_GROUPSRCH_NODE_GROUP_LIST,
	CC_GROUPSRCH_NODE_GROUP_LIST_CHILD,
	CC_GROUPSRCH_NODE_ITEM,
	CC_GROUPSRCH_NODE_ITEM_CHILD,
	CC_GROUPSRCH_NODE_INDEX,
	CC_GROUPSRCH_NODE_GROUP_ID,
	CC_GROUPSRCH_NODE_NAME,
	CC_GROUPSRCH_NODE_DISCRIPTION_BRIEF,
	CC_GROUPSRCH_NODE_ACCESS,
	CC_GROUPSRCH_NODE_PICT_URL_SMALL,
	CC_GROUPSRCH_NODE_PICT_URL_MEDIUM,
	CC_GROUPSRCH_NODE_PICT_URL_LARGE,
	CC_GROUPSRCH_NODE_MEMBER_NUM
};


//------------------------------------------------
// 変数定義
//------------------------------------------------
static INT32	CB_Result;				// 処理結果 (コールバック関数用)
static Bool		CB_IsXmlFile;

//------------------------------------------------
// 関数定義
//------------------------------------------------
static void CC_GroupSearch_CreateUri(const T_CC_CMN_SMS_API_PRM *param, Char *uri);
static E_SC_RESULT CC_GroupSearch_CreateBody(const T_CC_CMN_SMS_API_PRM *parm, INT32 type, const Char *groupName, INT32 offset, INT32 limit, Char *body);
static E_SC_RESULT CC_GroupSearch_AnalyzeHttpResp(const Char *body, E_CONTEXT_TYPE contextType, INT32 groupInfoMaxNum, SMGROUP *groupInfo, INT32 *groupInfoNum, Char *groupNum, SMCALOPT *opt, Char *apiStatus);
static E_SC_RESULT CC_GroupSearch_XmlParse(const Char* xml, T_CC_CMN_SMS_RESPONSE_INFO* resp_inf, INT32 groupInfoMaxNum, SMGROUP* groupInfo, INT32 *groupInfoNum, Char *groupNum, SMCALOPT *opt);
static void XMLCALL CC_GroupSearch_StartElement(void *userData, const char *name, const char **atts);
static void XMLCALL CC_GroupSearch_EndElement(void *userData, const char *name);
static void XMLCALL CC_GroupSearch_CharacterData(void *userData, const XML_Char *data, INT32 len);

/**
 * @brief サークル検索
 * @param [IN]  smcal       SMCAL
 * @param [IN]  parm        APIパラメータ
 * @param [in]  type        種別
 * @param [in]  groupName   サークル名
 * @param [in]  offset      オフセット
 * @param [in]  limit       取得最大件数
 * @param [out] groupInfo   サークル情報
 * @param [out] groupInfoNum サークル情報数
 * @param [IN]  recv        センタ受信データ
 * @param [OUT] apiStatus   APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GroupSearch_SendRecv(SMCAL *smcal,
									const T_CC_CMN_SMS_API_PRM *parm,
									INT32 type,
									const Char *groupName,
									INT32 offset,
									INT32 limit,
									SMGROUP *groupInfo,
									INT32 *groupInfoNum,
									Char *groupNum,
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
		strcat(opt.resFilePath, CC_GROUPSRCH_RES_FILE);
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
		data = (Char*)SCC_MALLOC(CC_GROUPSRCH_SEND_BODY_SIZE);
		if (NULL == data) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// URI生成
		CC_GroupSearch_CreateUri(parm, uri);

		// body生成
		ret = CC_GroupSearch_CreateBody(parm, type, groupName, offset, limit, data);
		if(e_SC_RESULT_SUCCESS != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GroupSearch_CreateBody error, " HERE);
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
		ret = CC_GroupSearch_AnalyzeHttpResp(body, contextType, limit, groupInfo, groupInfoNum, groupNum, &opt, apiStatus);
		if(e_SC_RESULT_SUCCESS != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GroupSearch_AnalyzeHttpResp error, " HERE);
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
void CC_GroupSearch_CreateUri(const T_CC_CMN_SMS_API_PRM *parm, Char *uri)
{
	sprintf((char*)uri,
			"%sgroup/srch/",
			parm->ApiPrmNavi.sms_sp_uri);
}

/**
 * @brief body生成
 * @param [IN]  parm        APIパラメータ
 * @param [in]  type        種別
 * @param [in]  groupName   サークル名
 * @param [in]  offset      オフセット
 * @param [in]  limit       取得最大件数
 * @param [out] groupInfo   サークル情報
 * @param [out] groupInfoNum サークル情報数
 * @param [IN]  recv        センタ受信データ
 * @param [OUT] body        body
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GroupSearch_CreateBody(const T_CC_CMN_SMS_API_PRM *parm,
									  INT32 type,
									  const Char *groupName,
									  INT32 offset,
									  INT32 limit,
									  Char *body)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	Char	*grpName = NULL;
	INT32	grpNameLen = 0;

	const Char *typeList[3] = {
			SCC_CMN_GROUP_TYPE_STR_ALL,
			SCC_CMN_GROUP_TYPE_STR_OWNER,
			SCC_CMN_GROUP_TYPE_STR_MEMBER
	};

	do {
		sprintf((char*)body,
				"term_id=%s&term_sig=%s&guid=%s&user_sig=%s&type=%s&offset=%d&limit=%d",
				parm->ApiPrmMups.new_term_id,
				parm->ApiPrmMups.term_sig,
				parm->ApiPrmMups.guid,
				parm->ApiPrmMups.user_sig,
				typeList[type],
				offset,
				limit
		);

		if ((NULL != groupName) && (EOS != *groupName)) {
			// メモリ確保
			grpNameLen = (SCC_CMN_GROUP_NAME * 3);
			grpName = (Char*)SCC_MALLOC(grpNameLen);
			if (NULL == grpName) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error " HERE);
				ret = e_SC_RESULT_MALLOC_ERR;
				break;
			}
			memset(grpName, 0, grpNameLen);

			// URLエンコード
			SC_CAL_UrlEncode(groupName, grpNameLen, grpName);

			sprintf((char*)&body[strlen((char*)body)],
					"&group_name=%s",
					grpName
			);
		} else {
			sprintf((char*)&body[strlen((char*)body)],
					"&group_name="
			);
		}
	} while (0);

	return (ret);
}

/**
 * @brief レスポンス解析
 * @param [IN]  body        xmlデータ
 * @param [IN]  contextType コンテキスト
 * @param [OUT] groupInfo     サークル検索結果
 * @param [OUT] groupInfoNum  サークル検索結果件数
 * @param [IN]  opt         オプション情報
 * @param [OUT] apiStatus   APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GroupSearch_AnalyzeHttpResp(const Char *body,
										   E_CONTEXT_TYPE contextType,
										   INT32 groupInfoMaxNum,
										   SMGROUP *groupInfo,
										   INT32 *groupInfoNum,
										   Char *groupNum,
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
		ret = CC_GroupSearch_XmlParse((const Char*)body, &rsp_inf, groupInfoMaxNum, groupInfo, groupInfoNum, groupNum, opt);
		if(e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GroupSearch_XmlParse error, " HERE);
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
 * @brief group/srch応答メッセージ解析
 * @param [IN] xml      XMLファイルのフルパス
 * @param [IN] resp_inf CICレスポンス情報
 * @param [OUT] groupInfo     サークル情報格納領域
 * @param [IN/OUT] groupInfo  サークル情報格納領域数/サークル情報数
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GroupSearch_XmlParse(const Char* xml,
									T_CC_CMN_SMS_RESPONSE_INFO* resp_inf,
									INT32 groupInfoMaxNum,
									SMGROUP* groupInfo,
									INT32 *groupInfoNum,
									Char *groupNum,
									SMCALOPT *opt)
{
	E_SC_RESULT		ret = e_SC_RESULT_SUCCESS;
	GROUPSRCH_PARSER	groupsrchParser = {};
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
		memset(groupInfo, 0, (sizeof(SMGROUP) * (groupInfoMaxNum)));
		groupsrchParser.buf = (Char*)SCC_MALLOC(CC_CMN_XML_BUF_SIZE + 1);
		if (NULL == groupsrchParser.buf) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			CB_Result = e_SC_RESULT_MALLOC_ERR;
			ret = CB_Result;
			break;
		}
		groupsrchParser.groupInfoMaxNum = groupInfoMaxNum;
		groupsrchParser.groupInfo.groupListNum = 0;
		groupsrchParser.groupInfo.groupList = groupInfo;
		groupsrchParser.groupInfo.apiStatus = &resp_inf->apiSts[0];
		groupsrchParser.groupInfo.groupNum = groupNum;
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
		XML_SetUserData(parser, &groupsrchParser);
		XML_SetElementHandler(parser, CC_GroupSearch_StartElement, CC_GroupSearch_EndElement);
		XML_SetCharacterDataHandler(parser, CC_GroupSearch_CharacterData);

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
			*groupInfoNum = groupsrchParser.groupInfo.groupListNum;
		}
	} while (0);

	if (NULL != groupsrchParser.buf) {
		SCC_FREE(groupsrchParser.buf);
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
void XMLCALL CC_GroupSearch_StartElement(void *userData, const char *name, const char **atts)
{
	GROUPSRCH_PARSER *parser = (GROUPSRCH_PARSER*)userData;

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
			// <XML>
			parser->state = CC_GROUPSRCH_NODE_XML;
		}

		// <XML>
		if (CC_GROUPSRCH_NODE_XML == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_API_STATUS)) {
				// <api_status>
				parser->state = CC_GROUPSRCH_NODE_API_STATUS;
			} else if (0 == strcmp((char*)name, (char*)CC_GROUPSRCH_XML_NODE_COUNT)) {
				// <count>
				parser->state = CC_GROUPSRCH_NODE_CNT;
			} else if (0 == strcmp((char*)name, (char*)CC_GROUPSRCH_XML_NODE_GROUP_LIST)) {
				// <group_list>
				parser->state = CC_GROUPSRCH_NODE_GROUP_LIST;
			} else if (0 != strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_XML)) {
				parser->state = CC_GROUPSRCH_NODE_XML_CHILD;
			}
		} else if (CC_GROUPSRCH_NODE_GROUP_LIST == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_GROUPSRCH_XML_NODE_ITEM)) {
				// <item>
				if (parser->groupInfoMaxNum <= parser->groupInfo.groupListNum) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"item maxnum over, " HERE);
					CB_Result = e_SC_RESULT_SMS_API_ERR;
					break;
				}
				parser->state = CC_GROUPSRCH_NODE_ITEM;
			} else {
				parser->state = CC_GROUPSRCH_NODE_GROUP_LIST_CHILD;
			}
		} else if (CC_GROUPSRCH_NODE_ITEM == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_GROUPSRCH_XML_NODE_INDEX)) {
				// <index>
				parser->state = CC_GROUPSRCH_NODE_INDEX;
			} else if (0 == strcmp((char*)name, (char*)CC_GROUPSRCH_XML_NODE_GROUP_ID)) {
				// <group_id>
				parser->state = CC_GROUPSRCH_NODE_GROUP_ID;
			} else if (0 == strcmp((char*)name, (char*)CC_GROUPSRCH_XML_NODE_NAME)) {
				// <name>
				parser->state = CC_GROUPSRCH_NODE_NAME;
			} else if (0 == strcmp((char*)name, (char*)CC_GROUPSRCH_XML_NODE_DISCRIPTION_BRIEF)) {
				// <discription_brief>
				parser->state = CC_GROUPSRCH_NODE_DISCRIPTION_BRIEF;
			} else if (0 == strcmp((char*)name, (char*)CC_GROUPSRCH_XML_NODE_ACCESS)) {
				// <access>
				parser->state = CC_GROUPSRCH_NODE_ACCESS;
			} else if (0 == strcmp((char*)name, (char*)CC_GROUPSRCH_XML_NODE_PICT_URL_SMALL)) {
				// <pict_url_small>
				parser->state = CC_GROUPSRCH_NODE_PICT_URL_SMALL;
			} else if (0 == strcmp((char*)name, (char*)CC_GROUPSRCH_XML_NODE_PICT_URL_MEDIUM)) {
				// <pict_url_medium>
				parser->state = CC_GROUPSRCH_NODE_PICT_URL_MEDIUM;
			} else if (0 == strcmp((char*)name, (char*)CC_GROUPSRCH_XML_NODE_PICT_URL_LARGE)) {
				// <pict_url_large>
				parser->state = CC_GROUPSRCH_NODE_PICT_URL_LARGE;
			} else if (0 == strcmp((char*)name, (char*)CC_GROUPSRCH_XML_NODE_MEMBER_NUM)) {
				// <member_num>
				parser->state = CC_GROUPSRCH_NODE_MEMBER_NUM;
			} else {
				parser->state = CC_GROUPSRCH_NODE_ITEM_CHILD;
			}
		} else {
			// 上記以外
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"format error, name=%s, state=%d " HERE, name, parser->state);
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
void XMLCALL CC_GroupSearch_EndElement(void *userData, const char *name)
{
	GROUPSRCH_PARSER *parser = (GROUPSRCH_PARSER*)userData;
	SMGROUP	*groupInfo = NULL;

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
		if (NULL == parser->groupInfo.groupList) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[parser->groupInfo.groupInfoList], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}
		if (NULL == name) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[name], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}

		groupInfo = &parser->groupInfo.groupList[parser->groupInfo.groupListNum];

		if (CC_GROUPSRCH_NODE_API_STATUS == parser->state) {
			// <api_status>
			strcpy((char*)parser->groupInfo.apiStatus, (char*)parser->buf);
			parser->state = CC_GROUPSRCH_NODE_XML;
		} else if (CC_GROUPSRCH_NODE_CNT == parser->state) {
			// <count>
			strcpy((char*)parser->groupInfo.groupNum, (char*)parser->buf);
			parser->state = CC_GROUPSRCH_NODE_XML;
		} else if (CC_GROUPSRCH_NODE_GROUP_LIST == parser->state) {
			// <group_list>
			parser->state = CC_GROUPSRCH_NODE_XML;
		} else if (CC_GROUPSRCH_NODE_ITEM == parser->state) {
			// <item>
			parser->state = CC_GROUPSRCH_NODE_GROUP_LIST;
			parser->groupInfo.groupListNum++;
		} else if (CC_GROUPSRCH_NODE_INDEX == parser->state) {
			// <index>
			groupInfo->index = atoi((char*)parser->buf);
			parser->state = CC_GROUPSRCH_NODE_ITEM;
		} else if (CC_GROUPSRCH_NODE_GROUP_ID == parser->state) {
			// <group_id>
			strcpy((char*)groupInfo->groupId, (char*)parser->buf);
			parser->state = CC_GROUPSRCH_NODE_ITEM;
		} else if (CC_GROUPSRCH_NODE_NAME == parser->state) {
			// <name>
			strcpy((char*)groupInfo->name, (char*)parser->buf);
			parser->state = CC_GROUPSRCH_NODE_ITEM;
		} else if (CC_GROUPSRCH_NODE_DISCRIPTION_BRIEF == parser->state) {
			// <discription_brief>
			strcpy((char*)groupInfo->discriptionBrief, (char*)parser->buf);
			parser->state = CC_GROUPSRCH_NODE_ITEM;
		} else if (CC_GROUPSRCH_NODE_ACCESS == parser->state) {
			// <access>
			strcpy((char*)groupInfo->access, (char*)parser->buf);
			parser->state = CC_GROUPSRCH_NODE_ITEM;
		} else if (CC_GROUPSRCH_NODE_PICT_URL_SMALL == parser->state) {
			// <pict_url_small>
			strcpy((char*)groupInfo->pictSmlUrl, (char*)parser->buf);
			parser->state = CC_GROUPSRCH_NODE_ITEM;
		} else if (CC_GROUPSRCH_NODE_PICT_URL_MEDIUM == parser->state) {
			// <pict_url_medium>
			strcpy((char*)groupInfo->pictMidUrl, (char*)parser->buf);
			parser->state = CC_GROUPSRCH_NODE_ITEM;
		} else if (CC_GROUPSRCH_NODE_PICT_URL_LARGE == parser->state) {
			// <pict_url_large>
			strcpy((char*)groupInfo->pictLrgUrl, (char*)parser->buf);
			parser->state = CC_GROUPSRCH_NODE_ITEM;
		} else if (CC_GROUPSRCH_NODE_MEMBER_NUM == parser->state) {
			// <member_num>
			groupInfo->memberNum = atoi((char*)parser->buf);
			parser->state = CC_GROUPSRCH_NODE_ITEM;
		} else if (CC_GROUPSRCH_NODE_XML_CHILD == parser->state) {
			parser->state = CC_GROUPSRCH_NODE_XML;
		} else if (CC_GROUPSRCH_NODE_GROUP_LIST_CHILD == parser->state) {
			parser->state = CC_GROUPSRCH_NODE_GROUP_LIST;
		} else if (CC_GROUPSRCH_NODE_ITEM_CHILD == parser->state) {
			parser->state = CC_GROUPSRCH_NODE_ITEM;
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
void XMLCALL CC_GroupSearch_CharacterData(void *userData, const XML_Char *data, INT32 len)
{
	GROUPSRCH_PARSER *parser = (GROUPSRCH_PARSER*)userData;
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

		if (CC_GROUPSRCH_NODE_GROUP_LIST == parser->state) {
			// <group_list>
		} else if (CC_GROUPSRCH_NODE_API_STATUS == parser->state) {
			// <api_status>
			if (CC_GROUPSRCH_XML_DATA_APISTATUS_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GROUPSRCH_NODE_CNT == parser->state) {
			// <count>
			if (CC_GROUPSRCH_XML_DATA_COUNT_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GROUPSRCH_NODE_ITEM == parser->state) {
			// <item>
		} else if (CC_GROUPSRCH_NODE_INDEX == parser->state) {
			// <index>
			if (CC_GROUPSRCH_XML_DATA_INDEX_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GROUPSRCH_NODE_GROUP_ID == parser->state) {
			// <group_id>
			if (CC_GROUPSRCH_XML_DATA_GROUP_ID_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GROUPSRCH_NODE_NAME == parser->state) {
			// <name>
			if (CC_GROUPSRCH_XML_DATA_NAME_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GROUPSRCH_NODE_DISCRIPTION_BRIEF == parser->state) {
			// <discription_brief>
			if (CC_GROUPSRCH_XML_DATA_DISCRIPTION_BRIEF_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GROUPSRCH_NODE_ACCESS == parser->state) {
			// <access>
			if (CC_GROUPSRCH_XML_DATA_ACCESS_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GROUPSRCH_NODE_PICT_URL_SMALL == parser->state) {
			// <pict_url_small>
			if (CC_GROUPSRCH_XML_DATA_PICT_URL_SMALL_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GROUPSRCH_NODE_PICT_URL_MEDIUM == parser->state) {
			// <pict_url_medium>
			if (CC_GROUPSRCH_XML_DATA_PICT_URL_MEDIUM_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GROUPSRCH_NODE_PICT_URL_LARGE == parser->state) {
			// <pict_url_large>
			if (CC_GROUPSRCH_XML_DATA_PICT_URL_LARGE_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_GROUPSRCH_NODE_MEMBER_NUM == parser->state) {
			// <member_num>
			if (CC_GROUPSRCH_XML_DATA_MEMBER_NUM_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		}
	} while (0);

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}
