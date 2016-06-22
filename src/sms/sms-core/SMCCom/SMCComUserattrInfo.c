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

#define CC_USERATTR_INFO_SEND_BODY_SIZE			512

#define	CC_USERINFO_XML_NODE_USER_NAME			"user_name"
#define	CC_USERINFO_XML_NODE_PROFILE			"profile"
#define	CC_USERINFO_XML_NODE_COMMENT			"comment"
#define	CC_USERINFO_XML_NODE_ADDRESS			"address"
#define	CC_USERINFO_XML_NODE_HOBBY				"hobby"
#define	CC_USERINFO_XML_NODE_SPECIAL_SKILL		"special_skill"
#define	CC_USERINFO_XML_NODE_MAIL				"mail"
#define	CC_USERINFO_XML_NODE_AVATAR				"avatar"

#define	CC_USERINFO_XML_DATA_APISTATUS_SIZE		CC_CMN_XML_RES_STS_CODE_SIZE
#define	CC_USERINFO_XML_DATA_USER_NAME_SIZE		CC_CMN_USERNAME_STR_SIZE
#define	CC_USERINFO_XML_DATA_PROFILE_SIZE		CC_CMN_PROFILE_STR_SIZE
#define	CC_USERINFO_XML_DATA_COMMENT_SIZE		CC_CMN_COMMENT_STR_SIZE
#define	CC_USERINFO_XML_DATA_ADDRESS_SIZE		CC_CMN_ADDRESS_STR_SIZE
#define	CC_USERINFO_XML_DATA_HOBBY_SIZE			CC_CMN_HOBBY_STR_SIZE
#define	CC_USERINFO_XML_DATA_SPECIAL_SKILL_SIZE	CC_CMN_SKIL_STR_SIZE
#define	CC_USERINFO_XML_DATA_MAIL_SIZE			CC_CMN_MALEADDR_STR_SIZE
#define	CC_USERINFO_XML_DATA_AVATAR_SIZE		CC_CMN_AVATART_STR_SIZE


// ユーザー属性照会レスポンスXML情報
typedef struct _USERINFO {
	SMUSERPROFILE	*userInfo;
	INT32			*status;
	Char			*apiStatus;
} USERINFO;

// ユーザー属性照会XMLパーサ
typedef struct _USERINFO_PARSER {
	INT32			state;
	Char			*buf;
	USERINFO		userInfo;
} USERINFO_PARSER;

// Userattr.info
enum UserStatus {
	CC_USERINFO_NODE_NONE = 0,
	CC_USERINFO_NODE_ELGG,
	CC_USERINFO_NODE_ELGG_CHILD,
	CC_USERINFO_NODE_STATUS,
	CC_USERINFO_NODE_RESULT,
	CC_USERINFO_NODE_RESULT_CHILD,
	CC_USERINFO_NODE_API_STATUS,
	CC_USERINFO_NODE_USER_NAME,
	CC_USERINFO_NODE_PROFILE,
	CC_USERINFO_NODE_COMMENT,
	CC_USERINFO_NODE_ADDRESS,
	CC_USERINFO_NODE_HOBBY,
	CC_USERINFO_NODE_SPECIAL_SKILL,
	CC_USERINFO_NODE_MAIL,
	CC_USERINFO_NODE_AVATAR
};


//------------------------------------------------
// 変数定義
//------------------------------------------------
static INT32	CB_Result;				// 処理結果 (コールバック関数用)

//------------------------------------------------
// 関数定義
//------------------------------------------------
static void CC_UserattrInfo_CreateUri(const T_CC_CMN_SMS_API_PRM *parm, const Char *srchGuid, Char *uri);
static void CC_UserattrInfo_CreateBody(const T_CC_CMN_SMS_API_PRM *parm, const Char *srchGuid, Char *body);
static E_SC_RESULT CC_UserattrInfo_AnalyzeHttpResp(const Char *body, E_CONTEXT_TYPE contextType, SMUSERPROFILE *userInfo, SMCALOPT *opt, Char *apiStatus);
static E_SC_RESULT CC_UserattrInfo_XmlParse(const Char *xml, T_CC_CMN_SMS_RESPONSE_INFO *resp_inf, SMUSERPROFILE *userInfo, SMCALOPT *opt);
static void XMLCALL CC_UserattrInfo_StartElement(void *userData, const char *name, const char **atts);
static void XMLCALL CC_UserattrInfo_EndElement(void *userData, const char *name);
static void XMLCALL CC_UserattrInfo_CharacterData(void *userData, const XML_Char *data, INT32 len);

/**
 * @brief ユーザー属性照会
 * @param [IN] smcal        SMCAL
 * @param [IN/OUT] parm     APIパラメータ
 * @param [IN] srchGuid     取得対象ユーザーID
 * @param [OUT] userInfo    ユーザ属性
 * @param [IN] recv         センタ受信データ
 * @param [IN] recvBufSize  センタ受信データバッファサイズ
 * @param [OUT] apiStatus   APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_UserattrInfo_SendRecv(SMCAL *smcal,
									 const T_CC_CMN_SMS_API_PRM *parm,
									 const Char *srchGuid,
									 SMUSERPROFILE *userInfo,
									 Char *recv,
									 UINT32 recvBufSize,
									 Char *apiStatus,
									 Bool isPolling)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_CAL_RESULT	calRet = e_SC_CAL_RESULT_SUCCESS;
	Char	*uri = NULL;
	Char	*data = NULL;
	Char	*body = NULL;
	UINT32	bodySize = 0;
	E_CONTEXT_TYPE	contextType = E_TEXT_XML;
	SMCALOPT	opt = {};
	UINT32	recvSize = 0;
	const Char	*guid = NULL;
	INT32	status = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// 初期化
		if (isPolling) {
			opt.cancel = SCC_IsCancel_Polling;
		} else {
			opt.cancel = SCC_IsCancel;
		}
#ifdef SC_CMN_BASIC_AUTH_SMS
		// BASIC認証
		opt.isBasicAuth = true;
		strcpy(opt.basic.basicAuthId, SC_CMN_BASIC_AUTH_ID);
		strcpy(opt.basic.basicAuthPwd, SC_CMN_BASIC_AUTH_PWD);
#endif

		// メモリ確保
		uri = (Char*)SCC_MALLOC(CC_CMN_URI_STR_MAX_LEN);
		if (NULL == uri) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC() error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		data = (Char*)SCC_MALLOC(CC_USERATTR_INFO_SEND_BODY_SIZE);
		if (NULL == data) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = CC_CMN_RESULT_MALLOC_ERR;
			break;
		}

		// URI生成
		if ((NULL != srchGuid) && (EOS != *srchGuid)) {
			guid = srchGuid;
		} else {
			guid = parm->ApiPrmMups.guid;
		}
		CC_UserattrInfo_CreateUri(parm, guid, uri);

		// body生成
		CC_UserattrInfo_CreateBody(parm, guid, data);

		// HTTPデータ送受信
//		calRet = SC_CAL_GetRequest(smcal, uri, recv, recvBufSize, &recvSize, &opt);
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
		ret = CC_UserattrInfo_AnalyzeHttpResp(body, contextType, userInfo, &opt, apiStatus);
		if(e_SC_RESULT_SUCCESS != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_UserattrInfo_AnalyzeHttpResp error, " HERE);
			break;
		}
	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief URL生成
 * @param [IN]  parm        APIパラメータ
 * @param [IN]  srchGuid    取得対象ユーザーID
 * @param [OUT] uri         URL
 * @param [IN]  opt         オプション情報
 * @param [OUT] apiStatus   APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
void CC_UserattrInfo_CreateUri(const T_CC_CMN_SMS_API_PRM *parm, const Char *srchGuid, Char *uri)
{
//	sprintf((char*)uri,
//			"%s\?method=Userattr.info&term_id=%s&term_sig=%s&guid=%s&user_sig=%s&srch_guid=%s",
//			parm->ApiPrmNavi.common_uri,
//			parm->ApiPrmMups.new_term_id,
//			parm->ApiPrmMups.term_sig,
//			parm->ApiPrmMups.guid,
//			parm->ApiPrmMups.user_sig,
//			srchGuid
//	);
	sprintf((char*)uri,
			"%s\?method=Userattr.info",
			parm->ApiPrmNavi.common_uri
	);
}

/**
 * @brief Userattr.info用body生成
 * @param[in]  param    APIパラメータ
 * @param[in]  srchGuid 取得対象ユーザーID
 * @param[out] body     body
*/
void CC_UserattrInfo_CreateBody(const T_CC_CMN_SMS_API_PRM *parm,
								const Char *srchGuid,
								Char *body)
{
	sprintf((char*)body,
			"term_id=%s&term_sig=%s&guid=%s&user_sig=%s&srch_guid=%s",
			parm->ApiPrmMups.new_term_id,
			parm->ApiPrmMups.term_sig,
			parm->ApiPrmMups.guid,
			parm->ApiPrmMups.user_sig,
			srchGuid
	);
}

/**
 * @brief レスポンス解析
 * @param [IN] body         xmlデータ
 * @param [IN] contextType  コンテキスト
 * @param [IN] userInfo     ユーザ属性
 * @param [IN] opt          オプション情報
 * @param [OUT] apiStatus   APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_UserattrInfo_AnalyzeHttpResp(const Char *body,
											E_CONTEXT_TYPE contextType,
											SMUSERPROFILE *userInfo,
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
		ret = CC_UserattrInfo_XmlParse((const Char*)body, &rsp_inf, userInfo, opt);
		if(e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_UserattrInfo_XmlParse error, " HERE);
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
 * @brief Userattr.info応答メッセージ解析
 * @param [IN] xml          XMLファイルのフルパス
 * @param [IN] resp_inf     CICレスポンス情報
 * @param [OUT] userInfo    ユーザ属性情報格納領域
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_UserattrInfo_XmlParse(const Char *xml, T_CC_CMN_SMS_RESPONSE_INFO *resp_inf, SMUSERPROFILE *userInfo, SMCALOPT *opt)
{
	E_SC_RESULT		ret = e_SC_RESULT_SUCCESS;
	USERINFO_PARSER	userInfoParser = {};
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
		*(resp_inf->apiSts) = EOS;
		memset(userInfo, 0, sizeof(SMUSERPROFILE));
		userInfoParser.buf = (Char*)SCC_MALLOC(CC_CMN_XML_LARGE_BUF_SIZE + 1);
		if (NULL == userInfoParser.buf) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			CB_Result = e_SC_RESULT_MALLOC_ERR;
			ret = CB_Result;
			break;
		}
		userInfoParser.userInfo.userInfo = userInfo;
		userInfoParser.userInfo.status = &resp_inf->sts;
		userInfoParser.userInfo.apiStatus = &resp_inf->apiSts[0];
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
		XML_SetUserData(parser, &userInfoParser);
		XML_SetElementHandler(parser, CC_UserattrInfo_StartElement, CC_UserattrInfo_EndElement);
		XML_SetCharacterDataHandler(parser, CC_UserattrInfo_CharacterData);

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

	if (NULL != userInfoParser.buf) {
		SCC_FREE(userInfoParser.buf);
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
void XMLCALL CC_UserattrInfo_StartElement(void *userData, const char *name, const char **atts)
{
	USERINFO_PARSER *parser = (USERINFO_PARSER*)userData;

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
			parser->state = CC_USERINFO_NODE_ELGG;
		}

		// <elgg>
		if (CC_USERINFO_NODE_ELGG == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_STATUS)) {
				// <status>
				parser->state = CC_USERINFO_NODE_STATUS;
			} else if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_RESULT)) {
				// <result>
				parser->state = CC_USERINFO_NODE_RESULT;
			} else if (0 != strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_ELGG)) {
				parser->state = CC_USERINFO_NODE_ELGG_CHILD;
			}
		} else if (CC_USERINFO_NODE_RESULT == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_API_STATUS)) {
				// <api_status>
				parser->state = CC_USERINFO_NODE_API_STATUS;
			} else if (0 == strcmp((char*)name, (char*)CC_USERINFO_XML_NODE_USER_NAME)) {
				// <user_name>
				parser->state = CC_USERINFO_NODE_USER_NAME;
			} else if (0 == strcmp((char*)name, (char*)CC_USERINFO_XML_NODE_PROFILE)) {
				// <profile>
				parser->state = CC_USERINFO_NODE_PROFILE;
			} else if (0 == strcmp((char*)name, (char*)CC_USERINFO_XML_NODE_COMMENT)) {
				// <comment>
				parser->state = CC_USERINFO_NODE_COMMENT;
			} else if (0 == strcmp((char*)name, (char*)CC_USERINFO_XML_NODE_ADDRESS)) {
				// <address>
				parser->state = CC_USERINFO_NODE_ADDRESS;
			} else if (0 == strcmp((char*)name, (char*)CC_USERINFO_XML_NODE_HOBBY)) {
				// <hobby>
				parser->state = CC_USERINFO_NODE_HOBBY;
			} else if (0 == strcmp((char*)name, (char*)CC_USERINFO_XML_NODE_SPECIAL_SKILL)) {
				// <special_skill>
				parser->state = CC_USERINFO_NODE_SPECIAL_SKILL;
			} else if (0 == strcmp((char*)name, (char*)CC_USERINFO_XML_NODE_MAIL)) {
				// <mail>
				parser->state = CC_USERINFO_NODE_MAIL;
			} else if (0 == strcmp((char*)name, (char*)CC_USERINFO_XML_NODE_AVATAR)) {
				// <avatar>
				parser->state = CC_USERINFO_NODE_AVATAR;
			} else {
				parser->state = CC_USERINFO_NODE_RESULT_CHILD;
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
void XMLCALL CC_UserattrInfo_EndElement(void *userData, const char *name)
{
	USERINFO_PARSER *parser = (USERINFO_PARSER*)userData;

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
		if (NULL == name) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[name], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}

		if (CC_USERINFO_NODE_STATUS == parser->state) {
			// <status>
			*(parser->userInfo.status) = atoi((char*)parser->buf);
			parser->state = CC_USERINFO_NODE_ELGG;
		} else if (CC_USERINFO_NODE_RESULT == parser->state) {
			// <result>
			parser->state = CC_USERINFO_NODE_ELGG;
		} else if (CC_USERINFO_NODE_API_STATUS == parser->state) {
			// <api_status>
			strcpy((char*)parser->userInfo.apiStatus, (char*)parser->buf);
			parser->state = CC_USERINFO_NODE_RESULT;
		} else if (CC_USERINFO_NODE_USER_NAME == parser->state) {
			// <user_name>
			strcpy((char*)parser->userInfo.userInfo->userName, (char*)parser->buf);
			parser->state = CC_USERINFO_NODE_RESULT;
		} else if (CC_USERINFO_NODE_PROFILE == parser->state) {
			// <profile>
			strcpy((char*)parser->userInfo.userInfo->profile, (char*)parser->buf);
			parser->state = CC_USERINFO_NODE_RESULT;
		} else if (CC_USERINFO_NODE_COMMENT == parser->state) {
			// <comment>
			strcpy((char*)parser->userInfo.userInfo->comment, (char*)parser->buf);
			parser->state = CC_USERINFO_NODE_RESULT;
		} else if (CC_USERINFO_NODE_ADDRESS == parser->state) {
			// <address>
			strcpy((char*)parser->userInfo.userInfo->address, (char*)parser->buf);
			parser->state = CC_USERINFO_NODE_RESULT;
		} else if (CC_USERINFO_NODE_HOBBY == parser->state) {
			// <hobby>
			strcpy((char*)parser->userInfo.userInfo->hobby, (char*)parser->buf);
			parser->state = CC_USERINFO_NODE_RESULT;
		} else if (CC_USERINFO_NODE_SPECIAL_SKILL == parser->state) {
			// <special_skill>
			strcpy((char*)parser->userInfo.userInfo->specialSkill, (char*)parser->buf);
			parser->state = CC_USERINFO_NODE_RESULT;
		} else if (CC_USERINFO_NODE_MAIL == parser->state) {
			// <mail>
			strcpy((char*)parser->userInfo.userInfo->mail, (char*)parser->buf);
			parser->state = CC_USERINFO_NODE_RESULT;
		} else if (CC_USERINFO_NODE_AVATAR == parser->state) {
			// <avatar>
			strcpy((char*)parser->userInfo.userInfo->avatar, (char*)parser->buf);
			parser->state = CC_USERINFO_NODE_RESULT;
		} else if (CC_USERINFO_NODE_ELGG_CHILD == parser->state) {
			parser->state = CC_USERINFO_NODE_ELGG;
		} else if (CC_USERINFO_NODE_RESULT_CHILD == parser->state) {
			parser->state = CC_USERINFO_NODE_RESULT;
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
void XMLCALL CC_UserattrInfo_CharacterData(void *userData, const XML_Char *data, INT32 len)
{
	USERINFO_PARSER *parser = (USERINFO_PARSER*)userData;
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

		if (CC_USERINFO_NODE_STATUS == parser->state) {
			// <status>
			if (CC_CMN_XML_RES_STS_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_USERINFO_NODE_API_STATUS == parser->state) {
			// <api_status>
			if (CC_USERINFO_XML_DATA_APISTATUS_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_USERINFO_NODE_USER_NAME == parser->state) {
			// <user_name>
			if (CC_USERINFO_XML_DATA_USER_NAME_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_USERINFO_NODE_PROFILE == parser->state) {
			// <profile>
			if (CC_USERINFO_XML_DATA_PROFILE_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_USERINFO_NODE_COMMENT == parser->state) {
			// <comment>
			if (CC_USERINFO_XML_DATA_COMMENT_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_USERINFO_NODE_ADDRESS == parser->state) {
			// <address>
			if (CC_USERINFO_XML_DATA_ADDRESS_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_USERINFO_NODE_HOBBY == parser->state) {
			// <hobby>
			if (CC_USERINFO_XML_DATA_HOBBY_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_USERINFO_NODE_SPECIAL_SKILL == parser->state) {
			// <special_skill>
			if (CC_USERINFO_XML_DATA_SPECIAL_SKILL_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_USERINFO_NODE_MAIL == parser->state) {
			// <mail>
			if (CC_USERINFO_XML_DATA_MAIL_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_USERINFO_NODE_AVATAR == parser->state) {
			// <avatar>
			if (CC_USERINFO_XML_DATA_AVATAR_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		}
	} while (0);

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}
