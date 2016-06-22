/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

//------------------------------------------------------------------------------------------------------------------------
//
//	SMCComDaialogStatusReq：USerpolicy.req処理
//
//------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------
// インクルード
//---------------------------------------------------------------------------------
#include "SMCComInternal.h"

#define	CC_DAIALOGSTATUS_XML_NODE_MESSAGE_ID			"message_id"
#define	CC_DAIALOGSTATUS_XML_NODE_CACHED_FLG			"cached_flg"
#define	CC_DAIALOGSTATUS_XML_NODE_CACHED_FLG_SIZE		1

#define	CC_DAIALOGSTATUS_CACHED_FLG_ENABLE				'1'
#define	CC_DAIALOGSTATUS_CACHED_FLG_DISABLE				'0'

#define CC_DAIALOGSTATUS_SEND_BODY_SIZE					512

/**
* @brief 通知メッセージ情報
*/
typedef struct _T_CC_DAIALOGSTATUSREQ_INFO {
	INT32			*status;
	Char			*apiStatus;
	Char			*message;
	Char			*messageId;
	Bool			*cachedFlg;
} T_CC_DAIALOGSTATUSREQ_INFO;

/**
* @brief 通知メッセージ取得パーサ
*/
typedef struct _T_CC_DAIALOGSTATUSREQ_PARSER {
	INT32			state;
	Char			*buf;
	T_CC_DAIALOGSTATUSREQ_INFO	daialogStatus;
} T_CC_DAIALOGSTATUSREQ_PARSER;

enum DaialogStatus {
	CC_DAIALOGSTATUS_NODE_NONE = 0,
	CC_DAIALOGSTATUS_NODE_XML,
	CC_DAIALOGSTATUS_NODE_XML_CHILD,
	CC_DAIALOGSTATUS_NODE_API_STATUS,
	CC_DAIALOGSTATUS_NODE_MESSAGE_ID,
	CC_DAIALOGSTATUS_NODE_MESSAGE,
	CC_DAIALOGSTATUS_NODE_CACHED_FAG
};

//---------------------------------------------------------------------------------
// 変数定義
//---------------------------------------------------------------------------------
static INT32	CB_Result;				// 処理結果 (コールバック関数用)

//------------------------------------------------
// 関数定義
//------------------------------------------------
static void CC_DaialogStatusReq_CreateUri(const T_CC_CMN_SMS_API_PRM* param, Char* pUri);
static void CC_DaialogStatusReq_CreateBody(const T_CC_CMN_SMS_API_PRM *param, const Char *lang, const Char *appVer, Char *body);
static E_SC_RESULT CC_DaialogStatusReq_Analy(const Char *body, E_CONTEXT_TYPE contextType, SMCALOPT *opt, T_CC_DAIALOGSTATUS_INFO *dlgStatusInfo);
static E_SC_RESULT CC_DaialogStatusReq_XmlParse(const Char* xml, T_CC_CMN_SMS_RESPONSE_INFO* resp_inf, T_CC_DAIALOGSTATUS_INFO *dlgStatusInfo, const SMCALOPT *opt);
static void XMLCALL CC_DaialogStatusReq_StartElement(void *userData, const char *name, const char **atts);
static void XMLCALL CC_DaialogStatusReq_EndElement(void *userData, const char *name);
static void XMLCALL CC_DaialogStatusReq_CharacterData(void *userData, const XML_Char *data, INT32 len);

//---------------------------------------------------------------------------------
// 外部関数
//---------------------------------------------------------------------------------
//************************************************
// daialog_status/req送信・受信処理
//************************************************
/**
 * @brief daialog_status/req処理
 * @param[in] param         SMS API関連パラメタテーブルポインタ
 * @param[in] lang          言語
 * @param[in] appVer        アプリバージョン
 * @param[in] recvBuf       受信バッファポインタ
 * @param[in] recvBufSize   受信バッファサイズ
 * @param[out] dlgStatusInfo    通知メッセージ情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_DaialogStatusReq_SendRecv(SMCAL *smcal,
										 T_CC_CMN_SMS_API_PRM *param,
										 const Char *lang,
										 const Char *appVer,
										 Char *recvBuf,
										 UINT32 recvBufSize,
										 T_CC_DAIALOGSTATUS_INFO *dlgStatusInfo)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_CAL_RESULT	calRet = e_SC_CAL_RESULT_SUCCESS;
	Char		*body = NULL;							// httpレスポンスボディ部へのポインタ
	INT32		bodySize = 0;							// httpレスポンスボディ部サイズ
	E_CONTEXT_TYPE	contextType = E_TEXT_XML;			// Content-Type
	Char		*uri = NULL;							// URI生成用文字バッファ
	UINT32		recvSize = 0;
	SMCALOPT	opt = {};
	Char		*data = NULL;
	INT32		status = 0;

	do {
		// 初期化
		memset(dlgStatusInfo, 0, sizeof(T_CC_DAIALOGSTATUS_INFO));
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
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		data = (Char*)SCC_MALLOC(CC_DAIALOGSTATUS_SEND_BODY_SIZE);
		if (NULL == data) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// URI生成
		CC_DaialogStatusReq_CreateUri(param, uri);

		// body生成
		CC_DaialogStatusReq_CreateBody(param, lang, appVer, data);

		// HTTPデータ送受信
		calRet = SC_CAL_PostRequest(smcal, uri, data, strlen(data), recvBuf, recvBufSize, &recvSize, &opt);
		if (e_SC_CAL_RESULT_SUCCESS != calRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SC_CAL_PostRequest error：%x " HERE, calRet);
			ret = ConvertResult(calRet);
			break;
		}

		// HTTPデータ解析
		calRet = SC_CAL_AnalyzeResponseStatus(smcal, recvBuf, recvSize, (const Char**)&body, &bodySize, &contextType, &status);
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
		ret = CC_DaialogStatusReq_Analy(body, contextType, &opt, dlgStatusInfo);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_DaialogStatusReq_Analy error, " HERE);
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

	return (ret);
}

//---------------------------------------------------------------------------------
// 内部関数
//---------------------------------------------------------------------------------
//************************************************
// RI生成
//************************************************
/**
 * @brief daialog_status/req用URI生成
 * @param[in] param     SMS API関連パラメタテーブルポインタ
 * @param[out] uri      生成URI出力先
 */
void CC_DaialogStatusReq_CreateUri(const T_CC_CMN_SMS_API_PRM* param,
								   Char* uri)
{
	sprintf((char*)uri,
			"%sdaialog_status/req",
			param->ApiPrmNavi.sms_sp_uri
	);
}

/**
 * @brief body生成
 * @param [in]  param   APIパラメータ
 * @param [in]  lang    言語
 * @param [in]  appVer  アプリバージョン
 * @param [out] body    body
 */
void CC_DaialogStatusReq_CreateBody(const T_CC_CMN_SMS_API_PRM *param,
									const Char *lang,
									const Char *appVer,
									Char *body)
{
	sprintf((char*)body,
			"term_id=%s&term_sig=%s&app_ver=%s&lang=%s",
			param->ApiPrmMups.new_term_id,
			param->ApiPrmMups.term_sig,
			appVer,
			lang
	);
}

//************************************************
// レスポンス解析
//************************************************
/**
 * @brief daialog_status/req応答解析
 * @brief レスポンス解析
 * @param[in] body          xmlデータ
 * @param[in] contextType   コンテキスト
 * @param[in] opt           オプション情報
 * @param[out] dlgStatusInfo    通知メッセージ情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_DaialogStatusReq_Analy(const Char *body,
									  E_CONTEXT_TYPE contextType,
									  SMCALOPT *opt,
									  T_CC_DAIALOGSTATUS_INFO *dlgStatusInfo)
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
		rsp_inf.apiSts = dlgStatusInfo->apiStatus;

		ret = CC_DaialogStatusReq_XmlParse((const Char*)body, &rsp_inf, dlgStatusInfo, opt);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_DaialogStatusReq_XmlParse error, " HERE);
			ret = e_SC_RESULT_SMS_API_ERR;
			break;
		}

		// 正常系のXMLとして解析できなかった場合
		if (!CHECK_API_STATUS(rsp_inf.apiSts)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"api status error, " HERE);
			ret = e_SC_RESULT_SMS_API_ERR;
			break;
		}
	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
// daialog_status/req応答メッセージ解析
//************************************************
/*
 * @brief daialog_status/req応答メッセージ解析
 * @param[in] xml           xmlデータ
 * @param[out] resp_inf     レスポンス情報
 * @param[out] dlgStatusInfo    通知メッセージ情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_DaialogStatusReq_XmlParse(const Char* xml,
										 T_CC_CMN_SMS_RESPONSE_INFO* resp_inf,
										 T_CC_DAIALOGSTATUS_INFO *dlgStatusInfo,
										 const SMCALOPT *opt)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;					// 戻り値
	T_CC_DAIALOGSTATUSREQ_PARSER	dialogStatusParser = {};
	Char		buf[CC_CMN_XML_PARSE_DATA_SIZE + 1] = {};
	XML_Parser	parser = NULL;
	INT32		done = 0;
	INT32		len = 0;
	INT32		parsedLen = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// 初期化
		resp_inf->sts = 0;
		*(resp_inf->apiSts) = EOS;
		// メモリ確保
		dialogStatusParser.buf = (Char*)SCC_MALLOC(CC_CMN_XML_BUF_SIZE + 1);
		if (NULL == dialogStatusParser.buf) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			CB_Result = e_SC_RESULT_MALLOC_ERR;
			ret = CB_Result;
			break;
		}
		dialogStatusParser.daialogStatus.status = &resp_inf->sts;
		dialogStatusParser.daialogStatus.apiStatus = &resp_inf->apiSts[0];
		dialogStatusParser.daialogStatus.message = dlgStatusInfo->message;
		dialogStatusParser.daialogStatus.messageId = dlgStatusInfo->messageId;
		dialogStatusParser.daialogStatus.cachedFlg = &dlgStatusInfo->cachedFlg;
		CB_Result = e_SC_RESULT_SUCCESS;

		// XMLパーサ生成
		parser = XML_ParserCreate(NULL);
		if (NULL == parser) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"XML_ParserCreate() error, " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			ret = CB_Result;
			break;
		}

		// コールバック関数設定
		XML_SetUserData(parser, &dialogStatusParser);
		XML_SetElementHandler(parser, CC_DaialogStatusReq_StartElement, CC_DaialogStatusReq_EndElement);
		XML_SetCharacterDataHandler(parser, CC_DaialogStatusReq_CharacterData);

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
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"XML_Parse() error(0x%08x), " HERE, CB_Result);
				CB_Result = e_SC_RESULT_TCP_COMMUNICATION_ERR;
				ret = CB_Result;
				break;
			}

			if (!done) {
				// バッファクリア
				memset(buf, 0, (sizeof(buf) - 1));
			}
		}
	} while (0);

	if (NULL != dialogStatusParser.buf) {
		SCC_FREE(dialogStatusParser.buf);
	}

	if (NULL != parser) {
		XML_ParserFree(parser);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//---------------------------------------------------------------------------------
//コールバック関数
//---------------------------------------------------------------------------------
//************************************************
//XML解析開始
//************************************************
/**
 * @brief タグ解析開始
 * @param [in/out] userData ユーザデータ
 * @param [in] name     タグ名
 * @param [out] atts    属性(未使用)
 * @return 処理結果(E_SC_RESULT)
 */
void XMLCALL CC_DaialogStatusReq_StartElement(void *userData, const char *name, const char **atts)
{
	T_CC_DAIALOGSTATUSREQ_PARSER	*parser = (T_CC_DAIALOGSTATUSREQ_PARSER*)userData;	// パーサ

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
			parser->state = CC_DAIALOGSTATUS_NODE_XML;
		}

		// <xml>
		if (CC_DAIALOGSTATUS_NODE_XML == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_API_STATUS)) {
				// <api_status>
				parser->state = CC_DAIALOGSTATUS_NODE_API_STATUS;
			} else if (0 == strcmp((char*)name, (char*)CC_DAIALOGSTATUS_XML_NODE_MESSAGE_ID)) {
				// <message_id>
				parser->state = CC_DAIALOGSTATUS_NODE_MESSAGE_ID;
			} else if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_MESSAGE)) {
				// <message>
				parser->state = CC_DAIALOGSTATUS_NODE_MESSAGE;
			} else if (0 == strcmp((char*)name, (char*)CC_DAIALOGSTATUS_XML_NODE_CACHED_FLG)) {
				// <cached_flg>
				parser->state = CC_DAIALOGSTATUS_NODE_CACHED_FAG;
			} else if (0 != strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_XML)) {
				parser->state = CC_DAIALOGSTATUS_NODE_XML_CHILD;
			}
		} else {
			// 上記以外
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"format error, " HERE);
			CB_Result = e_SC_RESULT_TCP_COMMUNICATION_ERR;
			break;
		}
	} while (0);

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}

//************************************************
//XML解析終了
//************************************************
/**
 * @brief タグ解析終了
 * @param [in/out] userData ユーザデータ
 * @param [in] name     タグ名
 */
void XMLCALL CC_DaialogStatusReq_EndElement(void *userData, const char *name)
{
	T_CC_DAIALOGSTATUSREQ_PARSER	*parser = (T_CC_DAIALOGSTATUSREQ_PARSER*)userData;	// パーサ

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

		if (CC_DAIALOGSTATUS_NODE_API_STATUS == parser->state) {
			// <api_status>
			strcpy((char*)parser->daialogStatus.apiStatus, (char*)parser->buf);
			parser->state = CC_DAIALOGSTATUS_NODE_XML;
		} else if (CC_DAIALOGSTATUS_NODE_MESSAGE_ID == parser->state) {
			// <message_id>
			strcpy((char*)parser->daialogStatus.messageId, (char*)parser->buf);
			parser->state = CC_DAIALOGSTATUS_NODE_XML;
		} else if (CC_DAIALOGSTATUS_NODE_MESSAGE == parser->state) {
			// <message>
			strcpy((char*)parser->daialogStatus.message, (char*)parser->buf);
			parser->state = CC_DAIALOGSTATUS_NODE_XML;
		} else if (CC_DAIALOGSTATUS_NODE_CACHED_FAG == parser->state) {
			// <cached_flg>
			if (CC_DAIALOGSTATUS_CACHED_FLG_ENABLE == *(parser->buf)) {
				*(parser->daialogStatus.cachedFlg) = true;
			} else {
				*(parser->daialogStatus.cachedFlg) = false;
			}
			parser->state = CC_DAIALOGSTATUS_NODE_XML;
		} else if (CC_DAIALOGSTATUS_NODE_XML_CHILD == parser->state) {
			parser->state = CC_DAIALOGSTATUS_NODE_XML;
		}
	} while (0);

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}

//************************************************
//CharacterData処理
//************************************************
/**
 * @brief 解析データ
 * @param [in] userData ユーザデータ
 * @param [in] data     データ
 * @param [in] len      データ長
 * @return 処理結果(E_SC_RESULT)
 */
void XMLCALL CC_DaialogStatusReq_CharacterData(void *userData, const XML_Char *data, INT32 len)
{
	T_CC_DAIALOGSTATUSREQ_PARSER	*parser = (T_CC_DAIALOGSTATUSREQ_PARSER*)userData;	// パーサ
	//Char		buf[CC_CMN_XML_PARSE_DATA_SIZE + 1] = {};
	INT32		bufLen = 0;

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
		if (NULL == data) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[data], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}

		// データをバッファにコピー
		bufLen = strlen((char*)parser->buf);

		if (CC_DAIALOGSTATUS_NODE_API_STATUS == parser->state) {
			// <api_status>
			if (CC_CMN_XML_RES_STS_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_DAIALOGSTATUS_NODE_MESSAGE_ID == parser->state) {
			// <message_id>
			if (CC_CMN_MESSAGE_ID_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_DAIALOGSTATUS_NODE_MESSAGE == parser->state) {
			// <message>
			if (CC_CMN_MESSAGE_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_DAIALOGSTATUS_NODE_CACHED_FAG == parser->state) {
			// <cached_flg>
			if (CC_DAIALOGSTATUS_XML_NODE_CACHED_FLG_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		}
	} while (0);

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}
