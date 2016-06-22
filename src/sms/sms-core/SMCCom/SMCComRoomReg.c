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

#define	CC_ROOMREG_XML_NODE_ROOM_NO			"room_no"
#define	CC_ROOMREG_XML_NODE_ROOM_NAME		"room_name"

#define	CC_ROOMREG_XML_DATA_APISTATUS_SIZE		CC_CMN_XML_RES_STS_CODE_SIZE
#define	CC_ROOMREG_XML_DATA_ROOMNO_SIZE			SCC_CHAT_MAXCHAR_ROOMNO

#define CC_ROOM_REG_SEND_BODY_SIZE	2176

// ルーム作成レスポンスXML情報
typedef struct _ROOMREGINFO {
	Char			*roomNo;
	INT32			*status;
	Char			*apiStatus;
} ROOMREGINFO;

// ルーム作成XMLパーサ
typedef struct _ROOMREG_PARSER {
	INT32			state;
	Char			*buf;
	ROOMREGINFO		roomRegInfo;
} ROOMREG_PARSER;

// Room.reg
enum RoomRegStatus {
	CC_ROOMREG_NODE_NONE = 0,
	CC_ROOMREG_NODE_ELGG,
	CC_ROOMREG_NODE_ELGG_CHILD,
	CC_ROOMREG_NODE_STATUS,
	CC_ROOMREG_NODE_RESULT,
	CC_ROOMREG_NODE_RESULT_CHILD,
	CC_ROOMREG_NODE_API_STATUS,
	CC_ROOMREG_NODE_ROOM_NO,
	CC_ROOMREG_NODE_ROOM_NAME
};


//------------------------------------------------
// 変数定義
//------------------------------------------------
static INT32	CB_Result;				// 処理結果 (コールバック関数用)
static Bool		CB_IsXmlFile;

//------------------------------------------------
// 関数定義
//------------------------------------------------
static E_SC_RESULT CC_RoomReg_CreateUri(const T_CC_CMN_SMS_API_PRM* pPrm, Char* pUri, INT32 UriMax);	//URI生成
static E_SC_RESULT CC_RoomReg_CreateBody(const T_CC_CMN_SMS_API_PRM* pPrm, const SMCREATEROOMINF *ChatRoomInf, Char* pBody, INT32 bodyMax);	//body生成

static E_SC_RESULT CC_RoomReg_AnalyzeHttpResp(const Char *body, E_CONTEXT_TYPE contextType, Char *roomNo, SMCALOPT *opt, Char *apiStatus);
static E_SC_RESULT CC_RoomReg_XmlParse(const Char* xml, T_CC_CMN_SMS_RESPONSE_INFO* resp_inf, Char *roomNo, SMCALOPT *opt);
static void XMLCALL CC_RoomReg_StartElement(void *userData, const char *name, const char **atts);
static void XMLCALL CC_RoomReg_EndElement(void *userData, const char *name);
static void XMLCALL CC_RoomReg_CharacterData(void *userData, const XML_Char *data, INT32 len);

/**
 * @brief ルーム作成
 * @param [IN]  smcal       SMCAL
 * @param [IN]  parm        APIパラメータ
 * @param [IN]  ChatRoomInf ルーム作成情報
 * @param [OUT] roomNo      ルームNo
 * @param [IN]  recv        センタ受信データ
 * @param [OUT] apiStatus   APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_RoomReg_SendRecv(SMCAL *smcal,
								const T_CC_CMN_SMS_API_PRM *parm,
								const SMCREATEROOMINF *ChatRoomInf,
								Char *roomNo,
								Char *recv,
								INT32 recvBufSize,
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
		if (CC_ISNULL(ChatRoomInf)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[ChatRoomInf], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(CC_ISINVALIDSTR(ChatRoomInf->userList)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[userList], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(roomNo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[roomNo], " HERE);
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

		// メモリ確保
		data = (Char*)SCC_MALLOC(CC_ROOM_REG_SEND_BODY_SIZE);
		if (NULL == data) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC() error " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		///URI生成
		ret = CC_RoomReg_CreateUri(parm, uri, CC_CMN_URI_STR_MAX_LEN);
		if(CC_CMN_RESULT_OK != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_RoomReg_CreateUri() error " HERE);
			break;
		}

		///body生成
		ret = CC_RoomReg_CreateBody(parm, ChatRoomInf, data, CC_ROOM_REG_SEND_BODY_SIZE);
		if(CC_CMN_RESULT_OK != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_RoomReg_CreateBody() error " HERE);
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
		ret = CC_RoomReg_AnalyzeHttpResp(body, contextType, roomNo, &opt, apiStatus);
		if(e_SC_RESULT_SUCCESS != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_RoomReg_AnalyzeHttpResp error, " HERE);
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
 * @brief Room.reg用URI生成
 * @param[in] pPrm SMS API関連パラメタテーブルポインタ
 * @param[out] pUri 生成URI出力先
 * @param[in] UriMax 生成URI出力先最大サイズ(byte)
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_PROC_ERR
*/
E_SC_RESULT CC_RoomReg_CreateUri(const T_CC_CMN_SMS_API_PRM* pPrm,
									Char* pUri,
									INT32 UriMax)
{
	INT32 rslt = 0;
	E_SC_RESULT ret = CC_CMN_RESULT_OK;

	rslt = snprintf(								//成功時には、書き込まれたbyte数を返す(EOSは含まない)
				(char*)pUri,
				UriMax,
				"%s\?method=Room.reg",
				pPrm->ApiPrmNavi.common_uri			//「http://～/distribution/」までのURI部分
				);
	if((rslt < 0) || (UriMax <= rslt)){				//負なら明らかにエラー、EOS終端できなかった場合もエラー
		SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_RoomReg_CreateUri() error len[%d] " HERE, rslt);
		ret = CC_CMN_RESULT_PROC_ERR;
	}

	return (ret);
}

//************************************************
//body生成
//************************************************
/**
 * @brief Room.reg用body生成
 * @param[in] pPrm SMS API関連パラメタテーブルポインタ
 * @param[in] ChatRoomInf Room.regパラメタテーブルポインタ
 * @param[out] pBody 生成body出力先
 * @param[in] bodyMax 生成body出力先最大サイズ(byte)
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_PROC_ERR
 * @note term_sigは直前のToken.Reqのレスポンス処理時に生成した端末アクセスキーを使用
*/
E_SC_RESULT CC_RoomReg_CreateBody(const T_CC_CMN_SMS_API_PRM* pPrm,
									const SMCREATEROOMINF *ChatRoomInf,
									Char* pBody,
									INT32 bodyMax)
{
	INT32 rslt = 0;
	E_SC_RESULT ret = CC_CMN_RESULT_OK;
	rslt = snprintf(								//成功時には、書き込まれたbyte数を返す(EOSは含まない)
				(char*)pBody,
				bodyMax,
				"term_id=%s&term_sig=%s&guid=%s&user_sig=%s&members=%s",
				pPrm->ApiPrmMups.new_term_id,		//端末ID (センター採番新規端末ID)
				pPrm->ApiPrmMups.term_sig,			//端末アクセスキー
				pPrm->ApiPrmMups.guid,				//ユーザID
				pPrm->ApiPrmMups.user_sig,			//ユーザアクセスキー
				ChatRoomInf->userList				//所属ユーザーIDリスト
				);
	if((rslt < 0) || (bodyMax <= rslt)){				//負なら明らかにエラー、EOS終端できなかった場合もエラー
		SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_RoomReg_CreateBody() error len[%d] " HERE, rslt);
		ret = CC_CMN_RESULT_PROC_ERR;
	}

	return (ret);
}

/**
 * @brief レスポンス解析
 * @param [IN] body         xmlデータ
 * @param [IN] contextType  コンテキスト
 * @param [OUT] roomNo      ルームNo
 * @param [IN] opt          オプション情報
 * @param [OUT] apiStatus   APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_RoomReg_AnalyzeHttpResp(const Char *body, E_CONTEXT_TYPE contextType, Char *roomNo, SMCALOPT *opt, Char *apiStatus)
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
		ret = CC_RoomReg_XmlParse((const Char*)body, &rsp_inf, roomNo, opt);
		if(e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_RoomReg_XmlParse error, " HERE);
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
 * @brief Room.reg応答メッセージ解析
 * @param [IN] xml      XMLファイルのフルパス
 * @param [IN] resp_inf CICレスポンス情報
 * @param [OUT] roomNo      ルームNo
 * @param [OUT] gem     GEM情報格納領域
 * @param [IN/OUT] gem  GEM情報格納領域数/GEM情報数
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_RoomReg_XmlParse(const Char* xml, T_CC_CMN_SMS_RESPONSE_INFO* resp_inf, Char *roomNo, SMCALOPT *opt)
{
	E_SC_RESULT		ret = e_SC_RESULT_SUCCESS;
	ROOMREG_PARSER	roomregParser = {};
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
		roomregParser.buf = SCC_MALLOC(CC_CMN_XML_BUF_SIZE + 1);
		if (NULL == roomregParser.buf) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			CB_Result = e_SC_RESULT_MALLOC_ERR;
			ret = CB_Result;
			break;
		}
		roomregParser.roomRegInfo.roomNo = roomNo;
		roomregParser.roomRegInfo.status = &resp_inf->sts;
		roomregParser.roomRegInfo.apiStatus = &resp_inf->apiSts[0];
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
		XML_SetUserData(parser, &roomregParser);
		XML_SetElementHandler(parser, CC_RoomReg_StartElement, CC_RoomReg_EndElement);
		XML_SetCharacterDataHandler(parser, CC_RoomReg_CharacterData);

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

	if (NULL != roomregParser.buf) {
		SCC_FREE(roomregParser.buf);
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
void XMLCALL CC_RoomReg_StartElement(void *userData, const char *name, const char **atts)
{
	ROOMREG_PARSER *parser = (ROOMREG_PARSER*)userData;

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
			parser->state = CC_ROOMREG_NODE_ELGG;
		}

		// <elgg>
		if (CC_ROOMREG_NODE_ELGG == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_STATUS)) {
				// <status>
				parser->state = CC_ROOMREG_NODE_STATUS;
			} else if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_RESULT)) {
				// <result>
				parser->state = CC_ROOMREG_NODE_RESULT;
			} else if (0 != strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_ELGG)) {
				parser->state = CC_ROOMREG_NODE_ELGG_CHILD;
			}
		} else if (CC_ROOMREG_NODE_RESULT == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_API_STATUS)) {
				// <api_status>
				parser->state = CC_ROOMREG_NODE_API_STATUS;
			} else if (0 == strcmp((char*)name, (char*)CC_ROOMREG_XML_NODE_ROOM_NO)) {
				// <room_no>
				parser->state = CC_ROOMREG_NODE_ROOM_NO;
			} else if (0 == strcmp((char*)name, (char*)CC_ROOMREG_XML_NODE_ROOM_NAME)) {
				// <room_name>
				parser->state = CC_ROOMREG_NODE_ROOM_NAME;
			} else {
				parser->state = CC_ROOMREG_NODE_RESULT_CHILD;
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
void XMLCALL CC_RoomReg_EndElement(void *userData, const char *name)
{
	ROOMREG_PARSER *parser = (ROOMREG_PARSER*)userData;

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

		if (CC_ROOMREG_NODE_STATUS == parser->state) {
			// <status>
			*(parser->roomRegInfo.status) = atoi((char*)parser->buf);
			parser->state = CC_ROOMREG_NODE_ELGG;
		} else if (CC_ROOMREG_NODE_RESULT == parser->state) {
			// <result>
			parser->state = CC_ROOMREG_NODE_ELGG;
		} else if (CC_ROOMREG_NODE_API_STATUS == parser->state) {
			// <api_status>
			strcpy((char*)parser->roomRegInfo.apiStatus, (char*)parser->buf);
			parser->state = CC_ROOMREG_NODE_RESULT;
		} else if (CC_ROOMREG_NODE_ROOM_NO == parser->state) {
			// <room_no>
			strcpy((char*)parser->roomRegInfo.roomNo, (char*)parser->buf);
			parser->state = CC_ROOMREG_NODE_RESULT;
		} else if (CC_ROOMREG_NODE_ROOM_NAME == parser->state) {
			// <room_name>
			parser->state = CC_ROOMREG_NODE_RESULT;
		} else if (CC_ROOMREG_NODE_ELGG_CHILD == parser->state) {
			parser->state = CC_ROOMREG_NODE_ELGG;
		} else if (CC_ROOMREG_NODE_RESULT_CHILD == parser->state) {
			parser->state = CC_ROOMREG_NODE_RESULT;
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
void XMLCALL CC_RoomReg_CharacterData(void *userData, const XML_Char *data, INT32 len)
{
	ROOMREG_PARSER *parser = (ROOMREG_PARSER*)userData;
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

		if (CC_ROOMREG_NODE_STATUS == parser->state) {
			// <status>
			if (CC_CMN_XML_RES_STS_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_ROOMREG_NODE_API_STATUS == parser->state) {
			// <api_status>
			if (CC_ROOMREG_XML_DATA_APISTATUS_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_ROOMREG_NODE_ROOM_NO == parser->state) {
			// <room_no>
			if (CC_ROOMREG_XML_DATA_ROOMNO_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_ROOMREG_NODE_ROOM_NAME == parser->state) {
			// <room_name>
			// 処理無し
		}
	} while (0);

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}
