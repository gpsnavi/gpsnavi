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

#define	CC_CHATHIS_XML_NODE_ROOM_LIST			"room_list"

#define	CC_CHATHIS_XML_DATA_APISTATUS_SIZE		CC_CMN_XML_RES_STS_CODE_SIZE

#define CC_USER_SRCH_SEND_BODY_SIZE	512

// チャット情報取得レスポンスXML情報
typedef struct _CHATINFO {
	INT32			*status;
	Char			*apiStatus;
} CHATINFO;

// チャット情報取得XMLパーサ
typedef struct _CHATHIS_PARSER {
	INT32			state;
	Char			*buf;
	CHATINFO		chatInfo;
} CHATHIS_PARSER;

// chat.his
enum ChatHisStatus {
	CC_CHATHIS_NODE_NONE = 0,
	CC_CHATHIS_NODE_ELGG,
	CC_CHATHIS_NODE_ELGG_CHILD,
	CC_CHATHIS_NODE_STATUS,
	CC_CHATHIS_NODE_RESULT,
	CC_CHATHIS_NODE_RESULT_CHILD,
	CC_CHATHIS_NODE_API_STATUS,
	CC_CHATHIS_NODE_ROOM_LIST
};


//------------------------------------------------
// 変数定義
//------------------------------------------------
static INT32	CB_Result;				// 処理結果 (コールバック関数用)
static Bool		CB_IsXmlFile;

//------------------------------------------------
// 関数定義
//------------------------------------------------
static E_SC_RESULT CC_ChatHis_CreateUri(const T_CC_CMN_SMS_API_PRM* pPrm, Char* pUri, INT32 UriMax);	//URI生成
static E_SC_RESULT CC_ChatHis_CreateBody(const T_CC_CMN_SMS_API_PRM* pPrm, const SMCHATGETQUALI *getQuali, Char* pBody, INT32 bodyMax);	//body生成

static E_SC_RESULT CC_ChatHis_AnalyzeHttpResp(const Char *body, E_CONTEXT_TYPE contextType, SMCALOPT *opt, Char *apiStatus);
static E_SC_RESULT CC_ChatHis_XmlParse(const Char* xml, T_CC_CMN_SMS_RESPONSE_INFO* resp_inf, SMCALOPT *opt);
static void XMLCALL CC_ChatHis_StartElement(void *userData, const char *name, const char **atts);
static void XMLCALL CC_ChatHis_EndElement(void *userData, const char *name);
static void XMLCALL CC_ChatHis_CharacterData(void *userData, const XML_Char *data, INT32 len);

/**
 * @brief ユーザ検索
 * @param [IN]  smcal       SMCAL
 * @param [IN]  parm        APIパラメータ
 * @param [IN]  SrchQuali   検索条件
 * @param [OUT] BaseInf     ユーザ基本情報
 * @param [IN]  recv        センタ受信データ
 * @param [OUT] apiStatus   APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_ChatHis_SendRecv(SMCAL *smcal,
								const T_CC_CMN_SMS_API_PRM *parm,
								const SMCHATGETQUALI *getQuali,
								const Char *fileName,
								Char *filePath,
								Char *recv,
								UINT32 recvBufSize,
								Char *apiStatus,
								Bool isPolling)
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
		// パラメータチェック
		if (CC_ISNULL(smcal)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[smcal], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(parm)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[parm], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(getQuali)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[getQuali], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if((CC_ISINVALIDSTR(getQuali->transaction)) && (0 < getQuali->getNum)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[transaction & getNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISINVALIDSTR(fileName)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[fileName], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(filePath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[filePath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(recv)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[recv], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 初期化
		memset(&opt, 0, sizeof(SMCALOPT));
		opt.isResOutputFile = true;
		CC_GetTempDirPath(opt.resFilePath);
		strcat(opt.resFilePath, fileName);
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
		*apiStatus = EOS;

		// メモリ確保
		data = (Char*)SCC_MALLOC(CC_USER_SRCH_SEND_BODY_SIZE);
		if (NULL == data) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC() error " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		///URI生成
		ret = CC_ChatHis_CreateUri(parm, uri, CC_CMN_URI_STR_MAX_LEN);
		if(CC_CMN_RESULT_OK != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ChatHis_CreateUri() error " HERE);
			break;
		}

		///body生成
		ret = CC_ChatHis_CreateBody(parm, getQuali, data, CC_USER_SRCH_SEND_BODY_SIZE);
		if(CC_CMN_RESULT_OK != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ChatHis_CreateBody() error " HERE);
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
		ret = CC_ChatHis_AnalyzeHttpResp(body, contextType, &opt, apiStatus);
		if(e_SC_RESULT_SUCCESS != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ChatHis_AnalyzeHttpResp error, " HERE);
			break;
		}
		else{
			// チャット情報xmlファイルパスコピー
			strcpy(filePath, opt.resFilePath);
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
 * @brief Chat.his用URI生成
 * @param[in] pPrm SMS API関連パラメタテーブルポインタ
 * @param[out] pUri 生成URI出力先
 * @param[in] UriMax 生成URI出力先最大サイズ(byte)
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_PROC_ERR
*/
E_SC_RESULT CC_ChatHis_CreateUri(const T_CC_CMN_SMS_API_PRM* pPrm,
									Char* pUri,
									INT32 UriMax)
{
	INT32 rslt = 0;
	E_SC_RESULT ret = CC_CMN_RESULT_OK;

	rslt = snprintf(								//成功時には、書き込まれたbyte数を返す(EOSは含まない)
				(char*)pUri,
				UriMax,
				"%s\?method=Chat.his",
				pPrm->ApiPrmNavi.common_uri			//「http://～/distribution/」までのURI部分
				);
	if((rslt < 0) || (UriMax <= rslt)){				//負なら明らかにエラー、EOS終端できなかった場合もエラー
		SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ChatHis_CreateUri() error len[%d] " HERE, rslt);
		ret = CC_CMN_RESULT_PROC_ERR;
	}

	return (ret);
}

//************************************************
//body生成
//************************************************
/**
 * @brief Chat.his用body生成
 * @param[in] pPrm SMS API関連パラメタテーブルポインタ
 * @param[in] getQuali Chat.hisパラメタテーブルポインタ
 * @param[out] pBody 生成body出力先
 * @param[in] bodyMax 生成body出力先最大サイズ(byte)
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_PROC_ERR
 * @note term_sigは直前のToken.Reqのレスポンス処理時に生成した端末アクセスキーを使用
*/
E_SC_RESULT CC_ChatHis_CreateBody(const T_CC_CMN_SMS_API_PRM* pPrm,
									const SMCHATGETQUALI *getQuali,
									Char* pBody,
									INT32 bodyMax)
{
	INT32 rslt = 0;
	E_SC_RESULT ret = CC_CMN_RESULT_OK;
	rslt = snprintf(								//成功時には、書き込まれたbyte数を返す(EOSは含まない)
				(char*)pBody,
				bodyMax,
				"term_id=%s&term_sig=%s&guid=%s&user_sig=%s",
				pPrm->ApiPrmMups.new_term_id,		//端末ID (センター採番新規端末ID)
				pPrm->ApiPrmMups.term_sig,			//端末アクセスキー
				pPrm->ApiPrmMups.guid,				//ユーザID
				pPrm->ApiPrmMups.user_sig			//ユーザアクセスキー
				);
	if((rslt < 0) || (bodyMax <= rslt)){				//負なら明らかにエラー、EOS終端できなかった場合もエラー
		SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ChatHis_CreateBody() error len[%d] " HERE, rslt);
		ret = CC_CMN_RESULT_PROC_ERR;
	}

	// ルームNo指定有の場合
	if((NULL != getQuali->roomNo) && (EOS != *(getQuali->roomNo))){
		rslt += snprintf(								//成功時には、書き込まれたbyte数を返す(EOSは含まない)
					(char*)(pBody + rslt),
					(bodyMax - rslt),
					"&room_no=%s",
					getQuali->roomNo					//ルームNo
					);
	}
	if((rslt < 0) || (bodyMax <= rslt)){				//負なら明らかにエラー、EOS終端できなかった場合もエラー
		SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ChatHis_CreateBody() error len[%d] " HERE, rslt);
		ret = CC_CMN_RESULT_PROC_ERR;
	}

	// 取得メッセージ数有の場合
	if(0 != getQuali->getNum){
		rslt += snprintf(								//成功時には、書き込まれたbyte数を返す(EOSは含まない)
					(char*)(pBody + rslt),
					(bodyMax - rslt),
					"&chat_count=%d",
					getQuali->getNum					//取得メッセージ数
					);
	}
	if((rslt < 0) || (bodyMax <= rslt)){				//負なら明らかにエラー、EOS終端できなかった場合もエラー
		SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ChatHis_CreateBody() error len[%d] " HERE, rslt);
		ret = CC_CMN_RESULT_PROC_ERR;
	}

	// トランザクション指定有の場合
	if((NULL != getQuali->transaction) && (EOS != *(getQuali->transaction))){
		rslt += snprintf(								//成功時には、書き込まれたbyte数を返す(EOSは含まない)
					(char*)(pBody + rslt),
					(bodyMax - rslt),
					"&transaction=%s",
					getQuali->transaction				//トランザクション条件
					);
	}
	if((rslt < 0) || (bodyMax <= rslt)){				//負なら明らかにエラー、EOS終端できなかった場合もエラー
		SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ChatHis_CreateBody() error len[%d] " HERE, rslt);
		ret = CC_CMN_RESULT_PROC_ERR;
	}

	return (ret);
}

/**
 * @brief レスポンス解析
 * @param [IN] body         xmlデータ
 * @param [IN] contextType  コンテキスト
 * @param [IN] opt          オプション情報
 * @param [OUT] apiStatus   APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_ChatHis_AnalyzeHttpResp(const Char *body, E_CONTEXT_TYPE contextType, SMCALOPT *opt, Char *apiStatus)
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
		ret = CC_ChatHis_XmlParse((const Char*)body, &rsp_inf, opt);
		if(e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ChatHis_XmlParse error, " HERE);
			return (e_SC_RESULT_SMS_API_ERR);
		}

		// 正常系のXMLとして解析できなかった場合
		if (CC_CMN_XML_CIC_RES_STS_OK != rsp_inf.sts) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"status error, " HERE);
			return (e_SC_RESULT_SMS_API_ERR);
		}
		if (!CHECK_API_STATUS2(rsp_inf.apiSts)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"api status error, " HERE);
			return (e_SC_RESULT_SMS_API_ERR);
		}
	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief Chat.his応答メッセージ解析
 * @param [IN] xml      XMLファイルのフルパス
 * @param [IN] resp_inf CICレスポンス情報
 * @param [OUT] gem     GEM情報格納領域
 * @param [IN/OUT] gem  GEM情報格納領域数/GEM情報数
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_ChatHis_XmlParse(const Char* xml, T_CC_CMN_SMS_RESPONSE_INFO* resp_inf, SMCALOPT *opt)
{
	E_SC_RESULT		ret = e_SC_RESULT_SUCCESS;
	CHATHIS_PARSER	chathisParser = {};
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
//		memset(baseI, 0, (sizeof(SMUSERBASEINFO) * (*userNum)));
		chathisParser.buf = SCC_MALLOC(CC_CMN_XML_BUF_SIZE + 1);
		if (NULL == chathisParser.buf) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			CB_Result = e_SC_RESULT_MALLOC_ERR;
			ret = CB_Result;
			break;
		}
		chathisParser.chatInfo.status = &resp_inf->sts;
		chathisParser.chatInfo.apiStatus = &resp_inf->apiSts[0];
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
		XML_SetUserData(parser, &chathisParser);
		XML_SetElementHandler(parser, CC_ChatHis_StartElement, CC_ChatHis_EndElement);
		XML_SetCharacterDataHandler(parser, CC_ChatHis_CharacterData);

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
//		if (e_SC_RESULT_SUCCESS == CB_Result) {
//			*userNum = usersrchParser.userInfo.userListNum;
//		}
	} while (0);

	if (NULL != chathisParser.buf) {
		SCC_FREE(chathisParser.buf);
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
void XMLCALL CC_ChatHis_StartElement(void *userData, const char *name, const char **atts)
{
	CHATHIS_PARSER *parser = (CHATHIS_PARSER*)userData;

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

		if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_ELGG)) {
			// <elgg>
			parser->state = CC_CHATHIS_NODE_ELGG;
		}

		// <elgg>
		if (CC_CHATHIS_NODE_ELGG == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_STATUS)) {
				// <status>
				parser->state = CC_CHATHIS_NODE_STATUS;
			} else if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_RESULT)) {
				// <result>
				parser->state = CC_CHATHIS_NODE_RESULT;
			} else if (0 != strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_ELGG)) {
				parser->state = CC_CHATHIS_NODE_ELGG_CHILD;
			}
		} else if (CC_CHATHIS_NODE_RESULT == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_API_STATUS)) {
				// <api_status>
				parser->state = CC_CHATHIS_NODE_API_STATUS;
			} else if (0 == strcmp((char*)name, (char*)CC_CHATHIS_XML_NODE_ROOM_LIST)) {
				// <room_list>
				parser->state = CC_CHATHIS_NODE_ROOM_LIST;
			} else {
				parser->state = CC_CHATHIS_NODE_RESULT_CHILD;
			}
		} else if (CC_CHATHIS_NODE_ROOM_LIST == parser->state) {
			// 処理無し
		} else {
			// 上記以外
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"format error  state=%d, " HERE, parser->state);
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
void XMLCALL CC_ChatHis_EndElement(void *userData, const char *name)
{
	CHATHIS_PARSER *parser = (CHATHIS_PARSER*)userData;

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
		if (NULL == name) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[name], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}

		if (CC_CHATHIS_NODE_STATUS == parser->state) {
			// <status>
			*(parser->chatInfo.status) = atoi((char*)parser->buf);
			parser->state = CC_CHATHIS_NODE_ELGG;
		} else if (CC_CHATHIS_NODE_RESULT == parser->state) {
			// <result>
			parser->state = CC_CHATHIS_NODE_ELGG;
		} else if (CC_CHATHIS_NODE_API_STATUS == parser->state) {
			// <api_status>
			strcpy((char*)parser->chatInfo.apiStatus, (char*)parser->buf);
			parser->state = CC_CHATHIS_NODE_RESULT;
		} else if (CC_CHATHIS_NODE_ROOM_LIST == parser->state) {
			// <room_list>
			if(0 == strcmp((char*)name, (char*)CC_CHATHIS_XML_NODE_ROOM_LIST)){	//当該終了タグがroom_listの場合
				parser->state = CC_CHATHIS_NODE_RESULT;
			}
		} else if (CC_CHATHIS_NODE_ELGG_CHILD == parser->state) {
			parser->state = CC_CHATHIS_NODE_ELGG;
		} else if (CC_CHATHIS_NODE_RESULT_CHILD == parser->state) {
			if(0 == strcmp((char*)name, (char*)CC_CHATHIS_XML_NODE_ROOM_LIST)){	//当該終了タグがroom_listの場合
				parser->state = CC_CHATHIS_NODE_RESULT;
			} else {
				parser->state = CC_CHATHIS_NODE_RESULT;
			}
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
void XMLCALL CC_ChatHis_CharacterData(void *userData, const XML_Char *data, INT32 len)
{
	CHATHIS_PARSER *parser = (CHATHIS_PARSER*)userData;
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

		if (CC_CHATHIS_NODE_STATUS == parser->state) {
			// <status>
			if (CC_CMN_XML_RES_STS_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_CHATHIS_NODE_API_STATUS == parser->state) {
			// <api_status>
			if (CC_CHATHIS_XML_DATA_APISTATUS_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_CHATHIS_NODE_ROOM_LIST == parser->state) {
			// <room_list>
			// 処理無し
		}
	} while (0);

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}
