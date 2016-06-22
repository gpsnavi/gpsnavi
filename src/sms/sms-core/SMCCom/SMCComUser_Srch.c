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

#define	CC_USER_SRCH_XML_NODE_COUNT				"count"
#define	CC_USER_SRCH_XML_NODE_USER_LIST			"user_list"
#define	CC_USER_SRCH_XML_NODE_ITEM				"item"
#define	CC_USER_SRCH_XML_NODE_GUID				"guid"
#define	CC_USER_SRCH_XML_NODE_USER_NAME			"user_name"
#define	CC_USER_SRCH_XML_NODE_LAST_ACTION		"last_action"
#define	CC_USER_SRCH_XML_NODE_AVATAR_URL_SMALL	"avatar_url_small"
#define	CC_USER_SRCH_XML_NODE_AVATAR_URL_MEDIUM	"avatar_url_medium"
#define	CC_USER_SRCH_XML_NODE_AVATAR_URL_LARGE	"avatar_url_large"

#define	CC_USER_SRCH_XML_DATA_APISTATUS_SIZE	CC_CMN_XML_RES_STS_CODE_SIZE
#define	CC_USER_SRCH_XML_DATA_COUNT_SIZE		11
#define	CC_USER_SRCH_XML_DATA_GUID_SIZE			CC_CMN_GUID_STR_SIZE
#define	CC_USER_SRCH_XML_DATA_USERNAME_SIZE		SCC_MAX_USERNAME
#define	CC_USER_SRCH_XML_DATA_LASTACTION_SIZE	11
#define	CC_USER_SRCH_XML_DATA_AVTURLSML_SIZE	SCC_MAX_URL
#define	CC_USER_SRCH_XML_DATA_AVTURLMID_SIZE	SCC_MAX_URL
#define	CC_USER_SRCH_XML_DATA_AVTURLLRG_SIZE	SCC_MAX_URL

#define CC_USER_SRCH_SEND_BODY_SIZE	1024

// ユーザ検索レスポンスXML情報
typedef struct _USERINFO {
	INT32			userListNum;
	SMUSERINFO		*userList;
	Char			*userNum;
	Char			*apiStatus;
} USERINFO;

// ユーザ検索XMLパーサ
typedef struct _USER_SRCH_PARSER {
	INT32			state;
	Char			*buf;
	USERINFO		userInfo;
	INT32			userInfoMaxNum;
} USER_SRCH_PARSER;

// user/srch/
enum RoomSrchStatus {
	CC_USER_SRCH_NODE_NONE = 0,
	CC_USER_SRCH_NODE_XML,
	CC_USER_SRCH_NODE_XML_CHILD,
	CC_USER_SRCH_NODE_API_STATUS,
	CC_USER_SRCH_NODE_COUNT,
	CC_USER_SRCH_NODE_USER_LIST,
	CC_USER_SRCH_NODE_USER_LIST_CHILD,
	CC_USER_SRCH_NODE_ITEM,
	CC_USER_SRCH_NODE_ITEM_CHILD,
	CC_USER_SRCH_NODE_GUID,
	CC_USER_SRCH_NODE_USER_NAME,
	CC_USER_SRCH_NODE_LAST_ACTION,
	CC_USER_SRCH_NODE_AVATAR_URL_SMALL,
	CC_USER_SRCH_NODE_AVATAR_URL_MEDIUM,
	CC_USER_SRCH_NODE_AVATAR_URL_LARGE
};


//------------------------------------------------
// 変数定義
//------------------------------------------------
static INT32	CB_Result;				// 処理結果 (コールバック関数用)
static Bool		CB_IsXmlFile;

//------------------------------------------------
// 関数定義
//------------------------------------------------
static E_SC_RESULT CC_User_Srch_CreateUri(const T_CC_CMN_SMS_API_PRM* pPrm, Char* pUri, INT32 UriMax);	//URI生成
static E_SC_RESULT CC_User_Srch_CreateBody(const T_CC_CMN_SMS_API_PRM* pPrm, const SMUSERSRCH *userSearch, Char* pBody);	//body生成

static E_SC_RESULT CC_User_Srch_AnalyzeHttpResp(const Char *body, E_CONTEXT_TYPE contextType, INT32 limit, SMUSERINFO *userInfo, INT32 *userInfoNum, Char *userNum, SMCALOPT *opt, Char *apiStatus);
static E_SC_RESULT CC_User_Srch_XmlParse(const Char* xml, T_CC_CMN_SMS_RESPONSE_INFO* resp_inf, INT32 limit, SMUSERINFO *userInfo, INT32 *userInfoNum, Char *userNum, SMCALOPT *opt);
static void XMLCALL CC_User_Srch_StartElement(void *userData, const char *name, const char **atts);
static void XMLCALL CC_User_Srch_EndElement(void *userData, const char *name);
static void XMLCALL CC_User_Srch_CharacterData(void *userData, const XML_Char *data, INT32 len);

/**
 * @brief ユーザ検索
 * @param [IN]  smcal       SMCAL
 * @param [IN]  parm        APIパラメータ
 * @param [IN]  userSearch   検索条件
 * @param [OUT] userInfo     ユーザ基本情報
 * @param [IN]  recv        センタ受信データ
 * @param [OUT] apiStatus   APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_User_Srch_SendRecv(SMCAL *smcal,
								 const T_CC_CMN_SMS_API_PRM *parm,
								 const SMUSERSRCH *userSearch,
								 SMUSERINFO *userInfo,
								 INT32 *userInfoNum,
								 Char *userNum,
								 const Char *fileName,
								 Char *recv,
								 UINT32 recvBufSize,
								 Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_CAL_RESULT	calRet = e_SC_CAL_RESULT_SUCCESS;
	static Char	uri[CC_CMN_URI_STR_MAX_LEN] = {};
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
		memset(&opt, 0, sizeof(SMCALOPT));
		opt.isResOutputFile = true;
		CC_GetTempDirPath(opt.resFilePath);
		strcat(opt.resFilePath, fileName);
		opt.cancel = SCC_IsCancel;
#ifdef SC_CMN_BASIC_AUTH_SMS
		// BASIC認証
		opt.isBasicAuth = true;
		strcpy(opt.basic.basicAuthId, SC_CMN_BASIC_AUTH_ID);
		strcpy(opt.basic.basicAuthPwd, SC_CMN_BASIC_AUTH_PWD);
#endif
		*apiStatus = EOS;

		// メモリ確保
		data = (Char*)SCC_MALLOC(CC_USER_SRCH_SEND_BODY_SIZE);
		if (NULL == data) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC() error " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		///URI生成
		ret = CC_User_Srch_CreateUri(parm, uri, CC_CMN_URI_STR_MAX_LEN);
		if(CC_CMN_RESULT_OK != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_User_Srch_CreateUri() error " HERE);
			break;
		}

		///body生成
		ret = CC_User_Srch_CreateBody(parm, userSearch, data);
		if(CC_CMN_RESULT_OK != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_User_Srch_CreateBody() error " HERE);
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
		ret = CC_User_Srch_AnalyzeHttpResp(body, contextType, userSearch->limit, userInfo, userInfoNum, userNum, &opt, apiStatus);
		if(e_SC_RESULT_SUCCESS != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_User_Srch_AnalyzeHttpResp error, " HERE);
			break;
		}
	} while (0);

	if (NULL != data) {
		SCC_FREE(data);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//URI生成
//************************************************
/**
 * @brief User.srch用URI生成
 * @param[in] pPrm SMS API関連パラメタテーブルポインタ
 * @param[out] pUri 生成URI出力先
 * @param[in] UriMax 生成URI出力先最大サイズ(byte)
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_PROC_ERR
*/
E_SC_RESULT CC_User_Srch_CreateUri(const T_CC_CMN_SMS_API_PRM* pPrm,
									Char* pUri,
									INT32 UriMax)
{
	INT32 rslt = 0;
	E_SC_RESULT ret = CC_CMN_RESULT_OK;

	rslt = snprintf(								//成功時には、書き込まれたbyte数を返す(EOSは含まない)
				(char*)pUri,
				UriMax,
				"%suser/srch/",
				pPrm->ApiPrmNavi.sms_sp_uri
				);
	if((rslt < 0) || (UriMax <= rslt)){				//負なら明らかにエラー、EOS終端できなかった場合もエラー
		SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_User_Srch_CreateUri() error len[%d] " HERE, rslt);
		ret = CC_CMN_RESULT_PROC_ERR;
	}

	return (ret);
}

//************************************************
//body生成
//************************************************
/**
 * @brief user/srch/用body生成
 * @param[in] pPrm SMS API関連パラメタテーブルポインタ
 * @param[in] roomSearch Room.srchパラメタテーブルポインタ
 * @param[out] pBody 生成body出力先
 * @return 処理結果(E_SC_RESULT)
 * @note term_sigは直前のToken.Reqのレスポンス処理時に生成した端末アクセスキーを使用
*/
E_SC_RESULT CC_User_Srch_CreateBody(const T_CC_CMN_SMS_API_PRM* pPrm,
									const SMUSERSRCH *userSearch,
									Char* pBody)
{
	E_SC_RESULT ret = CC_CMN_RESULT_OK;
	Char	*userName = NULL;
	INT32	userNameLen = 0;

	do {
		sprintf((char*)pBody,
				"term_id=%s&term_sig=%s&guid=%s&user_sig=%s&offset=%d&limit=%d",
				pPrm->ApiPrmMups.new_term_id,		//端末ID (センター採番新規端末ID)
				pPrm->ApiPrmMups.term_sig,			//端末アクセスキー
				pPrm->ApiPrmMups.guid,				//ユーザID
				pPrm->ApiPrmMups.user_sig,			//ユーザアクセスキー
				userSearch->offset,					//取得データの開始位置
				userSearch->limit					//取得件数
				);

		// 種別指定有の場合
		if((NULL != userSearch->pOnly) && (EOS != *(userSearch->pOnly))){
			sprintf((char*)&pBody[strlen((char*)pBody)],
					"&only=%s",
					userSearch->pOnly					//種別指定
					);
		} else {
			sprintf((char*)&pBody[strlen((char*)pBody)],
					"&only="
					);
		}

		// ユーザ名指定有の場合
		if((NULL != userSearch->pUserName) && (EOS != *(userSearch->pUserName))){
			// メモリ確保
			userNameLen = (SCC_CHAT_MAXCHAR_USERNAME * 3);
			userName = (Char*)SCC_MALLOC(userNameLen);
			if (NULL == userName) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_User_Srch_CreateBody() SCC_MALLOC error " HERE);
				ret = CC_CMN_RESULT_MALLOC_ERR;
				break;
			}
			memset(userName, 0, userNameLen);

			// URLエンコード
			SC_CAL_UrlEncode(userSearch->pUserName, userNameLen, userName);

			sprintf((char*)&pBody[strlen((char*)pBody)],
					"&user_name=%s",
					userName							//ユーザ名
					);
		} else {
			sprintf((char*)&pBody[strlen((char*)pBody)],
					"&user_name="
					);
		}

		// ソート条件の指定有の場合
		if((NULL != userSearch->pOrder) && (EOS != *(userSearch->pOrder))){
			sprintf((char*)&pBody[strlen((char*)pBody)],
					"&order=%s",
					userSearch->pOrder					//ソート条件
					);
		} else {
			sprintf((char*)&pBody[strlen((char*)pBody)],
					"&order="
					);
		}
	} while (0);

	// メモリ解放
	if (NULL != userName) {
		SCC_FREE(userName);
	}

	return (ret);
}

/**
 * @brief レスポンス解析
 * @param [IN] body         xmlデータ
 * @param [IN] contextType  コンテキスト
 * @param [IN/OUT] userInfo ユーザ検索条件/結果
 * @param [IN] opt          オプション情報
 * @param [OUT] apiStatus   APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_User_Srch_AnalyzeHttpResp(const Char *body,
										 E_CONTEXT_TYPE contextType,
										 INT32 limit,
										 SMUSERINFO *userInfo,
										 INT32 *userInfoNum,
										 Char *userNum,
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
		ret = CC_User_Srch_XmlParse((const Char*)body, &rsp_inf, limit, userInfo, userInfoNum, userNum, opt);
		if(e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_User_Srch_XmlParse error, " HERE);
			return (e_SC_RESULT_SMS_API_ERR);
		}

		if ((!CHECK_API_STATUS(rsp_inf.apiSts)) && (!CHECK_API_STATUS2(rsp_inf.apiSts))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"api status error, " HERE);
			return (e_SC_RESULT_SMS_API_ERR);
		}
	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief user/srch/応答メッセージ解析
 * @param [IN] xml      XMLファイルのフルパス
 * @param [IN] resp_inf レスポンス情報
 * @param [OUT] userInfo     ユーザ情報格納領域
 * @param [IN/OUT] userNum   ユーザ情報格納領域数/ユーザ情報数
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_User_Srch_XmlParse(const Char* xml,
								  T_CC_CMN_SMS_RESPONSE_INFO* resp_inf,
								  INT32 limit,
								  SMUSERINFO* userInfo,
								  INT32 *userInfoNum,
								  Char *userNum,
								  SMCALOPT *opt)
{
	E_SC_RESULT		ret = e_SC_RESULT_SUCCESS;
	USER_SRCH_PARSER	usersrchParser = {};
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
		memset(userInfo, 0, (sizeof(SMUSERINFO) * limit));
		usersrchParser.buf = (Char*)SCC_MALLOC(CC_CMN_XML_BUF_SIZE + 1);
		if (NULL == usersrchParser.buf) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			CB_Result = e_SC_RESULT_MALLOC_ERR;
			ret = CB_Result;
			break;
		}
		usersrchParser.userInfoMaxNum = limit;
		usersrchParser.userInfo.userListNum = 0;
		usersrchParser.userInfo.userList = userInfo;
		usersrchParser.userInfo.apiStatus = &resp_inf->apiSts[0];
		usersrchParser.userInfo.userNum = userNum;
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
		XML_SetUserData(parser, &usersrchParser);
		XML_SetElementHandler(parser, CC_User_Srch_StartElement, CC_User_Srch_EndElement);
		XML_SetCharacterDataHandler(parser, CC_User_Srch_CharacterData);

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
			*userInfoNum = usersrchParser.userInfo.userListNum;
		}
	} while (0);

	if (NULL != usersrchParser.buf) {
		SCC_FREE(usersrchParser.buf);
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
void XMLCALL CC_User_Srch_StartElement(void *userData, const char *name, const char **atts)
{
	USER_SRCH_PARSER *parser = (USER_SRCH_PARSER*)userData;

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
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
			parser->state = CC_USER_SRCH_NODE_XML;
		}

		// <xml>
		if (CC_USER_SRCH_NODE_XML == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_API_STATUS)) {
				// <api_status>
				parser->state = CC_USER_SRCH_NODE_API_STATUS;
			} else if (0 == strcmp((char*)name, (char*)CC_USER_SRCH_XML_NODE_COUNT)) {
				// <count>
				parser->state = CC_USER_SRCH_NODE_COUNT;
			} else if (0 == strcmp((char*)name, (char*)CC_USER_SRCH_XML_NODE_USER_LIST)) {
				// <user_list>
				parser->state = CC_USER_SRCH_NODE_USER_LIST;
			} else if (0 != strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_XML)) {
				parser->state = CC_USER_SRCH_NODE_XML_CHILD;
			}
		} else if (CC_USER_SRCH_NODE_USER_LIST == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_USER_SRCH_XML_NODE_ITEM)) {
				// <item>
				if (parser->userInfoMaxNum <= parser->userInfo.userListNum) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"item maxnum over, " HERE);
					CB_Result = e_SC_RESULT_SMS_API_ERR;
					break;
				}
				parser->state = CC_USER_SRCH_NODE_ITEM;
			} else {
				parser->state = CC_USER_SRCH_NODE_USER_LIST_CHILD;
			}
		} else if (CC_USER_SRCH_NODE_ITEM == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_USER_SRCH_XML_NODE_GUID)) {
				// <guid>
				parser->state = CC_USER_SRCH_NODE_GUID;
			} else if (0 == strcmp((char*)name, (char*)CC_USER_SRCH_XML_NODE_USER_NAME)) {
				// <user_name>
				parser->state = CC_USER_SRCH_NODE_USER_NAME;
			} else if (0 == strcmp((char*)name, (char*)CC_USER_SRCH_XML_NODE_LAST_ACTION)) {
				// <last_action>
				parser->state = CC_USER_SRCH_NODE_LAST_ACTION;
			} else if (0 == strcmp((char*)name, (char*)CC_USER_SRCH_XML_NODE_AVATAR_URL_SMALL)) {
				// <avatar_url_small>
				parser->state = CC_USER_SRCH_NODE_AVATAR_URL_SMALL;
			} else if (0 == strcmp((char*)name, (char*)CC_USER_SRCH_XML_NODE_AVATAR_URL_MEDIUM)) {
				// <avatar_url_midium>
				parser->state = CC_USER_SRCH_NODE_AVATAR_URL_MEDIUM;
			} else if (0 == strcmp((char*)name, (char*)CC_USER_SRCH_XML_NODE_AVATAR_URL_LARGE)) {
				// <avatar_url_large>
				parser->state = CC_USER_SRCH_NODE_AVATAR_URL_LARGE;
			} else {
				parser->state = CC_USER_SRCH_NODE_ITEM_CHILD;
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
void XMLCALL CC_User_Srch_EndElement(void *userData, const char *name)
{
	USER_SRCH_PARSER *parser = (USER_SRCH_PARSER*)userData;
	SMUSERINFO	*userInfo = NULL;

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		if (e_SC_RESULT_SUCCESS != CB_Result) {
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
		if (NULL == parser->userInfo.userList) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[parser->userInfo.userList], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}
		if (NULL == name) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[name], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}

		userInfo = &parser->userInfo.userList[parser->userInfo.userListNum];

		if (CC_USER_SRCH_NODE_API_STATUS == parser->state) {
			// <api_status>
			strcpy((char*)parser->userInfo.apiStatus, (char*)parser->buf);
			parser->state = CC_USER_SRCH_NODE_XML;
		} else if (CC_USER_SRCH_NODE_COUNT == parser->state) {
			// <count>
			strcpy((char*)parser->userInfo.userNum, (char*)parser->buf);
			parser->state = CC_USER_SRCH_NODE_XML;
		} else if (CC_USER_SRCH_NODE_USER_LIST == parser->state) {
			// <user_list>
			parser->state = CC_USER_SRCH_NODE_XML;
		} else if (CC_USER_SRCH_NODE_ITEM == parser->state) {
			// <item>
			parser->state = CC_USER_SRCH_NODE_USER_LIST;
			parser->userInfo.userListNum++;
		} else if (CC_USER_SRCH_NODE_GUID == parser->state) {
			// <guid>
			strcpy((char*)userInfo->guid, (char*)parser->buf);
			parser->state = CC_USER_SRCH_NODE_ITEM;
		} else if (CC_USER_SRCH_NODE_USER_NAME == parser->state) {
			// <user_name>
			strcpy((char*)userInfo->userName, (char*)parser->buf);
			parser->state = CC_USER_SRCH_NODE_ITEM;
		} else if (CC_USER_SRCH_NODE_LAST_ACTION == parser->state) {
			// <last_action>
			strcpy((char*)userInfo->lastDate, (char*)parser->buf);
			parser->state = CC_USER_SRCH_NODE_ITEM;
		} else if (CC_USER_SRCH_NODE_AVATAR_URL_SMALL == parser->state) {
			// <avatar_url_small>
			strcpy((char*)userInfo->avtSmlURL, (char*)parser->buf);
			parser->state = CC_USER_SRCH_NODE_ITEM;
		} else if (CC_USER_SRCH_NODE_AVATAR_URL_MEDIUM == parser->state) {
			// <avatar_url_midium>
			strcpy((char*)userInfo->avtMidURL, (char*)parser->buf);
			parser->state = CC_USER_SRCH_NODE_ITEM;
		} else if (CC_USER_SRCH_NODE_AVATAR_URL_LARGE == parser->state) {
			// <avatar_url_large>
			strcpy((char*)userInfo->avtLrgURL, (char*)parser->buf);
			parser->state = CC_USER_SRCH_NODE_ITEM;
		} else if (CC_USER_SRCH_NODE_XML_CHILD == parser->state) {
			parser->state = CC_USER_SRCH_NODE_XML;
		} else if (CC_USER_SRCH_NODE_USER_LIST_CHILD == parser->state) {
			parser->state = CC_USER_SRCH_NODE_USER_LIST;
		} else if (CC_USER_SRCH_NODE_ITEM_CHILD == parser->state) {
			parser->state = CC_USER_SRCH_NODE_ITEM;
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
void XMLCALL CC_User_Srch_CharacterData(void *userData, const XML_Char *data, INT32 len)
{
	USER_SRCH_PARSER *parser = (USER_SRCH_PARSER*)userData;
	//char buf[CC_CMN_XML_PARSE_DATA_SIZE + 1] = {};
	INT32	bufLen = 0;

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		if (e_SC_RESULT_SUCCESS != CB_Result) {
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

		if (CC_USER_SRCH_NODE_API_STATUS == parser->state) {
			// <api_status>
			if (CC_USER_SRCH_XML_DATA_APISTATUS_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_USER_SRCH_NODE_COUNT == parser->state) {
			// <count>
			if (CC_USER_SRCH_XML_DATA_COUNT_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_USER_SRCH_NODE_USER_LIST == parser->state) {
			// <user_list>
		} else if (CC_USER_SRCH_NODE_ITEM == parser->state) {
			// <item>
		} else if (CC_USER_SRCH_NODE_GUID == parser->state) {
			// <guid>
			if (CC_USER_SRCH_XML_DATA_GUID_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_USER_SRCH_NODE_USER_NAME == parser->state) {
			// <user_name>
			if (CC_USER_SRCH_XML_DATA_USERNAME_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_USER_SRCH_NODE_LAST_ACTION == parser->state) {
			// <last_action>
			if (CC_USER_SRCH_XML_DATA_LASTACTION_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_USER_SRCH_NODE_AVATAR_URL_SMALL == parser->state) {
			// <avatar_url_small>
			if (CC_USER_SRCH_XML_DATA_AVTURLSML_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_USER_SRCH_NODE_AVATAR_URL_MEDIUM == parser->state) {
			// <avatar_url_midium>
			if (CC_USER_SRCH_XML_DATA_AVTURLMID_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_USER_SRCH_NODE_AVATAR_URL_LARGE == parser->state) {
			// <avatar_url_large>
			if (CC_USER_SRCH_XML_DATA_AVTURLLRG_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		}
	} while (0);

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}
