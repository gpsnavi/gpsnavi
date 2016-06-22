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

#define	CC_CHATMSG_MSGTYPE_MSG					"msg"
#define	CC_CHATMSG_MSGTYPE_PIC					"pic"
#define	CC_CHATMSG_MSG_SIZE						1536

#define	CC_CHATMSG_XML_DATA_APISTATUS_SIZE		CC_CMN_XML_RES_STS_CODE_SIZE

// チャット発信レスポンスXML情報
typedef struct _CHATMSGINFO {
	INT32			*status;
	Char			*apiStatus;
} CHATMSGINFO;

// チャット発信XMLパーサ
typedef struct _CHATMSG_PARSER {
	INT32			state;
	Char			*buf;
	CHATMSGINFO		chatMsgInfo;
} CHATMSG_PARSER;

// chat.msg
enum ChatMsgStatus {
	CC_CHATMSG_NODE_NONE = 0,
	CC_CHATMSG_NODE_ELGG,
	CC_CHATMSG_NODE_ELGG_CHILD,
	CC_CHATMSG_NODE_STATUS,
	CC_CHATMSG_NODE_RESULT,
	CC_CHATMSG_NODE_RESULT_CHILD,
	CC_CHATMSG_NODE_API_STATUS
};

typedef struct _CHATMSG_BODY_BUF {
	Char	termId[CC_CMN_TARGETID_STR_SIZE];
	Char	termSig[CC_CMN_TERM_SIG_STR_SIZE];
	Char	guid[CC_CMN_GUID_STR_SIZE];
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE];
	Char	roomNo[SCC_CHAT_MAXCHAR_ROOMNO];
	Char	message[SCC_CHAT_MAXCHAR_MESSAGE];
	Char	msgType[SCC_CHAT_MAXCHAR_MSGTYPE];
	Char	gemId[SCC_CHAT_MAXCHAR_GEMID];
} CHATMSG_BODY_BUF;

//------------------------------------------------
// 変数定義
//------------------------------------------------
static INT32	CB_Result;				// 処理結果 (コールバック関数用)
static Bool		CB_IsXmlFile;

//------------------------------------------------
// 関数定義
//------------------------------------------------
static E_SC_RESULT CC_ChatMsg_CreateUri(const T_CC_CMN_SMS_API_PRM* pPrm, Char* pUri, INT32 UriMax);	//URI生成
static E_SC_RESULT CC_ChatMsg_CreatePostParam(const T_CC_CMN_SMS_API_PRM *parm, const SMCHATMESSAGE *chatMsg, SMCALPOSTPARAM *postParam, INT32 *postParamNum, CHATMSG_BODY_BUF *bodyBuf);

static E_SC_RESULT CC_ChatMsg_AnalyzeHttpResp(const Char *body, E_CONTEXT_TYPE contextType, SMCALOPT *opt, Char *apiStatus);
static E_SC_RESULT CC_ChatMsg_XmlParse(const Char* xml, T_CC_CMN_SMS_RESPONSE_INFO* resp_inf, SMCALOPT *opt);
static void XMLCALL CC_ChatMsg_StartElement(void *userData, const char *name, const char **atts);
static void XMLCALL CC_ChatMsg_EndElement(void *userData, const char *name);
static void XMLCALL CC_ChatMsg_CharacterData(void *userData, const XML_Char *data, INT32 len);

/**
 * @brief チャット発信
 * @param [IN]  smcal       SMCAL
 * @param [IN]  parm        APIパラメータ
 * @param [IN]  chatMsg   	チャットメッセージ情報
 * @param [IN]  recv        センタ受信データ
 * @param [OUT] apiStatus   APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_ChatMsg_SendRecv(SMCAL *smcal,
								const T_CC_CMN_SMS_API_PRM *parm,
								const SMCHATMESSAGE *chatMsg,
								Char *recv,
								INT32 recvBufSize,
								Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_CAL_RESULT	calRet = e_SC_CAL_RESULT_SUCCESS;
	static Char	uri[CC_CMN_URI_STR_MAX_LEN] = {};
	Char *body = NULL;
	UINT32 bodySize = 0;
	E_CONTEXT_TYPE contextType = E_TEXT_XML;
	SMCALOPT opt = {};
	UINT32 recvSize = 0;
	SMCALPOSTPARAM postParam[8] = {};
	INT32 postParamNum = 0;
	CHATMSG_BODY_BUF bodyBuf = {};
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
		if (CC_ISNULL(chatMsg)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[chatMsg], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(CC_ISINVALIDSTR(chatMsg->roomNo)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[chatMsg->roomNo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if((CC_ISINVALIDSTR(chatMsg->msgType)) || ((0 != strcmp(CC_CHATMSG_MSGTYPE_MSG, chatMsg->msgType)) && (0 != strcmp(CC_CHATMSG_MSGTYPE_PIC, chatMsg->msgType)))){
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[chatMsg->msgType], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(0 == strcmp(CC_CHATMSG_MSGTYPE_MSG, chatMsg->msgType)){
			if(((NULL == chatMsg->message) || (EOS == chatMsg->message[0]))){
				SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[chatMsg->message], " HERE);
				ret = e_SC_RESULT_BADPARAM;
				break;
			}
			else if(CC_CHATMSG_MSG_SIZE < strlen(chatMsg->message)){
				SCC_LOG_ErrorPrint(SC_TAG_CC, "param error(max size over)[chatMsg->message], " HERE);
				ret = e_SC_RESULT_BADPARAM;
				break;
			}
		}
		if((0 == strcmp(CC_CHATMSG_MSGTYPE_PIC, chatMsg->msgType)) && (CC_ISINVALIDSTR(chatMsg->picFilePath))){
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[chatMsg->picFilePath], " HERE);
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
		*apiStatus = EOS;
		opt.cancel = SCC_IsCancel;
#ifdef SC_CMN_BASIC_AUTH_SMS
		// BASIC認証
		opt.isBasicAuth = true;
		strcpy(opt.basic.basicAuthId, SC_CMN_BASIC_AUTH_ID);
		strcpy(opt.basic.basicAuthPwd, SC_CMN_BASIC_AUTH_PWD);
#endif

		///URI生成
		ret = CC_ChatMsg_CreateUri(parm, uri, CC_CMN_URI_STR_MAX_LEN);
		if(CC_CMN_RESULT_OK != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ChatMsg_CreateUri() error " HERE);
			break;
		}

		// POSTパラメータ生成
		ret = CC_ChatMsg_CreatePostParam(parm, chatMsg, postParam, &postParamNum, &bodyBuf);
		if(CC_CMN_RESULT_OK != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ChatMsg_CreatePostParam() error " HERE);
			break;
		}

		// HTTPデータ送受信
		calRet = SC_CAL_PostRequest_Multipart(smcal, uri, postParam, postParamNum, recv, recvBufSize, &recvSize, &opt);
		if(e_SC_CAL_RESULT_SUCCESS != calRet){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SC_CAL_PostRequest_Multipart error, " HERE);
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
		ret = CC_ChatMsg_AnalyzeHttpResp(body, contextType, &opt, apiStatus);
		if(e_SC_RESULT_SUCCESS != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ChatMsg_AnalyzeHttpResp error, " HERE);
			break;
		}

	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//URI生成
//************************************************
/**
 * @brief Chat.msg用URI生成
 * @param[in] pPrm SMS API関連パラメタテーブルポインタ
 * @param[out] pUri 生成URI出力先
 * @param[in] UriMax 生成URI出力先最大サイズ(byte)
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_PROC_ERR
*/
E_SC_RESULT CC_ChatMsg_CreateUri(const T_CC_CMN_SMS_API_PRM* pPrm,
									Char* pUri,
									INT32 UriMax)
{
	INT32 rslt = 0;
	E_SC_RESULT ret = CC_CMN_RESULT_OK;

	rslt = snprintf(								//成功時には、書き込まれたbyte数を返す(EOSは含まない)
				(char*)pUri,
				UriMax,
				"%s\?method=Chat.msg",
				pPrm->ApiPrmNavi.common_uri			//「http://～/distribution/」までのURI部分
				);
	if((rslt < 0) || (UriMax <= rslt)){				//負なら明らかにエラー、EOS終端できなかった場合もエラー
		SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ChatMsg_CreateUri() error len[%d] " HERE, rslt);
		ret = CC_CMN_RESULT_PROC_ERR;
	}

	return (ret);
}

E_SC_RESULT CC_ChatMsg_CreatePostParam(const T_CC_CMN_SMS_API_PRM *parm,
									  const SMCHATMESSAGE *chatMsg,
									  SMCALPOSTPARAM *postParam,
									  INT32 *postParamNum,
									  CHATMSG_BODY_BUF *bodyBuf)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	struct stat	st = {};
	CC_IMAGE_MIMETYPE	mimeType = CC_IMAGE_MIMETYPE_NONE;

	do {
		postParam[num].type = SMCALPOSTPARAM_DATATYPE_TEXT;
		postParam[num].name = "\"term_id\"";
		strcpy(bodyBuf->termId, parm->ApiPrmMups.new_term_id);
		postParam[num].len = strlen(bodyBuf->termId);
		postParam[num].data = bodyBuf->termId;
		num++;

		postParam[num].type = SMCALPOSTPARAM_DATATYPE_TEXT;
		postParam[num].name = "\"term_sig\"";
		strcpy(bodyBuf->termSig, parm->ApiPrmMups.term_sig);
		postParam[num].len = strlen(bodyBuf->termSig);
		postParam[num].data = bodyBuf->termSig;
		num++;

		postParam[num].type = SMCALPOSTPARAM_DATATYPE_TEXT;
		postParam[num].name = "\"guid\"";
		strcpy(bodyBuf->guid, parm->ApiPrmMups.guid);
		postParam[num].len = strlen(bodyBuf->guid);
		postParam[num].data = bodyBuf->guid;
		num++;

		postParam[num].type = SMCALPOSTPARAM_DATATYPE_TEXT;
		postParam[num].name = "\"user_sig\"";
		strcpy(bodyBuf->userSig, parm->ApiPrmMups.user_sig);
		postParam[num].len = strlen(bodyBuf->userSig);
		postParam[num].data = bodyBuf->userSig;
		num++;

		postParam[num].type = SMCALPOSTPARAM_DATATYPE_TEXT;
		postParam[num].name = "\"room_no\"";
		strcpy(bodyBuf->roomNo, chatMsg->roomNo);
		postParam[num].len = strlen(bodyBuf->roomNo);
		postParam[num].data = bodyBuf->roomNo;
		num++;

		if(0 == strcmp(CC_CHATMSG_MSGTYPE_MSG, chatMsg->msgType)){
			postParam[num].type = SMCALPOSTPARAM_DATATYPE_TEXT;
			postParam[num].name = "\"message\"";
			strcpy(bodyBuf->message, chatMsg->message);
			postParam[num].len = strlen(bodyBuf->message);
			postParam[num].data = bodyBuf->message;
			num++;
		}
		else if(0 == strcmp(CC_CHATMSG_MSGTYPE_PIC, chatMsg->msgType)){
			postParam[num].type = SMCALPOSTPARAM_DATATYPE_FILE;
			// MIME TYPE取得
			ret = SCC_GetImageMIMEType(chatMsg->picFilePath, &mimeType);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_GetImageMIMEType error, " HERE);
				ret = e_SC_RESULT_BADPARAM;
				break;
			}
			if (CC_IMAGE_MIMETYPE_JPG == mimeType) {
				postParam[num].name = "\"message\"; filename=\"pic.jpeg\"\r\nContent-Type: image/jpeg";
			} else if (CC_IMAGE_MIMETYPE_GIF == mimeType) {
				postParam[num].name = "\"message\"; filename=\"pic.gif\"\r\nContent-Type: image/gif";
			} else if (CC_IMAGE_MIMETYPE_PNG == mimeType) {
				postParam[num].name = "\"message\"; filename=\"pic.png\"\r\nContent-Type: image/png";
			} else {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"pic file format error, " HERE);
				ret = e_SC_RESULT_BADPARAM;
				break;
			}
			// ファイルサイズ取得
			if (0 != stat((char*)chatMsg->picFilePath, &st)) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, "stat error[%s] (0x%08x), " HERE, chatMsg->picFilePath, errno);
				ret = e_SC_RESULT_FILE_ACCESSERR;
				break;
			}
			postParam[num].len = (UINT32)st.st_size;
			postParam[num].data = (Char*)chatMsg->picFilePath;
			num++;
		}

		postParam[num].type = SMCALPOSTPARAM_DATATYPE_TEXT;
		postParam[num].name = "\"message_type\"";
		strcpy(bodyBuf->msgType, chatMsg->msgType);
		postParam[num].len = strlen(bodyBuf->msgType);
		postParam[num].data = bodyBuf->msgType;
		num++;

		if((NULL != chatMsg->gemId) && (EOS != chatMsg->gemId[0])){
			postParam[num].type = SMCALPOSTPARAM_DATATYPE_TEXT;
			postParam[num].name = "\"gem_id\"";
			strcpy(bodyBuf->gemId, chatMsg->gemId);
			postParam[num].len = strlen(bodyBuf->gemId);
			postParam[num].data = bodyBuf->gemId;
			num++;
		}

		*postParamNum = num;
	} while (0);

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
E_SC_RESULT CC_ChatMsg_AnalyzeHttpResp(const Char *body, E_CONTEXT_TYPE contextType, SMCALOPT *opt, Char *apiStatus)
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
		ret = CC_ChatMsg_XmlParse((const Char*)body, &rsp_inf, opt);
		if(e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ChatMsg_XmlParse error, " HERE);
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
E_SC_RESULT CC_ChatMsg_XmlParse(const Char* xml, T_CC_CMN_SMS_RESPONSE_INFO* resp_inf, SMCALOPT *opt)
{
	E_SC_RESULT		ret = e_SC_RESULT_SUCCESS;
	CHATMSG_PARSER	chatMsgParser = {};
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
		chatMsgParser.buf = SCC_MALLOC(CC_CMN_XML_BUF_SIZE + 1);
		if (NULL == chatMsgParser.buf) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			CB_Result = e_SC_RESULT_MALLOC_ERR;
			ret = CB_Result;
			break;
		}
		chatMsgParser.chatMsgInfo.status = &resp_inf->sts;
		chatMsgParser.chatMsgInfo.apiStatus = &resp_inf->apiSts[0];
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
		XML_SetUserData(parser, &chatMsgParser);
		XML_SetElementHandler(parser, CC_ChatMsg_StartElement, CC_ChatMsg_EndElement);
		XML_SetCharacterDataHandler(parser, CC_ChatMsg_CharacterData);

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

	} while (0);

	if (NULL != chatMsgParser.buf) {
		SCC_FREE(chatMsgParser.buf);
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
void XMLCALL CC_ChatMsg_StartElement(void *userData, const char *name, const char **atts)
{
	CHATMSG_PARSER *parser = (CHATMSG_PARSER*)userData;

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
			parser->state = CC_CHATMSG_NODE_ELGG;
		}

		// <elgg>
		if (CC_CHATMSG_NODE_ELGG == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_STATUS)) {
				// <status>
				parser->state = CC_CHATMSG_NODE_STATUS;
			} else if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_RESULT)) {
				// <result>
				parser->state = CC_CHATMSG_NODE_RESULT;
			} else if (0 != strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_ELGG)) {
				parser->state = CC_CHATMSG_NODE_ELGG_CHILD;
			}
		} else if (CC_CHATMSG_NODE_RESULT == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_API_STATUS)) {
				// <api_status>
				parser->state = CC_CHATMSG_NODE_API_STATUS;
			} else {
				parser->state = CC_CHATMSG_NODE_RESULT_CHILD;
			}
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
void XMLCALL CC_ChatMsg_EndElement(void *userData, const char *name)
{
	CHATMSG_PARSER *parser = (CHATMSG_PARSER*)userData;

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

		if (CC_CHATMSG_NODE_STATUS == parser->state) {
			// <status>
			*(parser->chatMsgInfo.status) = atoi((char*)parser->buf);
			parser->state = CC_CHATMSG_NODE_ELGG;
		} else if (CC_CHATMSG_NODE_RESULT == parser->state) {
			// <result>
			parser->state = CC_CHATMSG_NODE_ELGG;
		} else if (CC_CHATMSG_NODE_API_STATUS == parser->state) {
			// <api_status>
			strcpy((char*)parser->chatMsgInfo.apiStatus, (char*)parser->buf);
			parser->state = CC_CHATMSG_NODE_RESULT;
		} else if (CC_CHATMSG_NODE_ELGG_CHILD == parser->state) {
			parser->state = CC_CHATMSG_NODE_ELGG;
		} else if (CC_CHATMSG_NODE_RESULT_CHILD == parser->state) {
			parser->state = CC_CHATMSG_NODE_RESULT;
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
void XMLCALL CC_ChatMsg_CharacterData(void *userData, const XML_Char *data, INT32 len)
{
	CHATMSG_PARSER *parser = (CHATMSG_PARSER*)userData;
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

		if (CC_CHATMSG_NODE_STATUS == parser->state) {
			// <status>
			if (CC_CMN_XML_RES_STS_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_CHATMSG_NODE_API_STATUS == parser->state) {
			// <api_status>
			if (CC_CHATMSG_XML_DATA_APISTATUS_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		}
	} while (0);

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}
