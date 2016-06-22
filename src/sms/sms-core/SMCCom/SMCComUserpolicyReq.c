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
//	SMCCom_UserpolicyReq：USerpolicy.req処理
//
//------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------
//インクルード
//---------------------------------------------------------------------------------
#include "SMCComInternal.h"

#define	CC_USERPOLICY_RES_FILE				"userpolicy.res"

//---------------------------------------------------------------------------------
//プロトタイプ宣言
//---------------------------------------------------------------------------------
//***   コールバック関数   ***
void XMLCALL CC_UserpolicyReq_StartElement(void *userData, const char *name, const char **atts);
void XMLCALL CC_UserpolicyReq_EndElement(void *userData, const char *name);
void XMLCALL CC_UserpolicyReq_CharacterData(void *userData, const XML_Char *data, INT32 len);
//***   内部関数   ***
E_SC_RESULT CC_UserpolicyReq_CreateUri(const T_CC_CMN_SMS_API_PRM* pPrm, const Char *lang, Char* pUri, INT32 UriMax);	//URI生成
E_SC_RESULT CC_UserpolicyReq_Analy(T_CC_CMN_SMS_API_PRM* pPrm, const Char *lang, const UINT8* pBuf, INT32 BufSz, Char* pApiSts, E_CONTEXT_TYPE ContextType, const SMCALOPT *opt);	//レスポンス解析
E_SC_RESULT CC_UserpolicyReq_XmlParse(const Char *lang, const Char* pXml, T_CC_CMN_SMS_RESPONSE_INFO* pResp_inf_t, Char* pVer, Char* pLang, const SMCALOPT *opt);	//Userpolicy.req応答メッセージ解析


//---------------------------------------------------------------------------------
//変数定義
//---------------------------------------------------------------------------------
static INT32	CB_Result;				// 処理結果 (コールバック関数用)
static Bool		CB_IsXmlFile;
static FILE		*fp_user_policy;

//---------------------------------------------------------------------------------
//外部関数
//---------------------------------------------------------------------------------
//************************************************
//Userpolicy.req送信・受信処理
//************************************************
/**
 * @brief Userpololicy.req処理
 * @param[in] pPrm SMS API関連パラメタテーブルポインタ
 * @param[in] pRecvBuf 受信バッファポインタ
 * @param[in] RecvBuf_sz 受信バッファサイズ
 * @param[out] pApiSts ユーザメッセージ(地図センタからのユーザ向けエラー文言)
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_OK以外 (コール関数の戻り値)
 */
E_SC_RESULT CC_UserpolicyReq_SendRecv(SMCAL* smcal,
										T_CC_CMN_SMS_API_PRM* pPrm,
										const Char *lang,
										Char* pRecvBuf,
										UINT32 RecvBuf_sz,
										Char* pApiSts)
{
	E_SC_RESULT ret = CC_CMN_RESULT_OK;
	E_SC_CAL_RESULT	cal_ret = CC_CMN_RESULT_SMCAL_OK;
	//INT32 resp_sz = 0;							// httpレスポンスサイズ(ヘッダ含む)
	Char *p_body = NULL;						// httpレスポンスボディ部へのポインタ
	INT32 body_sz = 0;							// httpレスポンスボディ部サイズ
	E_CONTEXT_TYPE contextType = E_TEXT_XML;	// Content-Type
	Char *uri = NULL;							// URI生成用文字バッファ
	UINT32 recv_sz = 0;
	SMCALOPT opt = {};
	INT32	status = 0;

	// 初期化
	fp_user_policy = NULL;

	do {
		// 初期化
		opt.cancel = SCC_IsCancel;
		opt.isResOutputFile = true;
		CC_GetTempDirPath(opt.resFilePath);
		strcat(opt.resFilePath, CC_USERPOLICY_RES_FILE);
#ifdef SC_CMN_BASIC_AUTH_SMS
		// BASIC認証
		opt.isBasicAuth = true;
		strcpy(opt.basic.basicAuthId, SC_CMN_BASIC_AUTH_ID);
		strcpy(opt.basic.basicAuthPwd, SC_CMN_BASIC_AUTH_PWD);
#endif

		// メモリ確保
		uri = (Char*)SCC_MALLOC(CC_CMN_URI_STR_MAX_LEN);
		if (NULL == uri) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserpolicyReq] CC_UserpolicyReq_SendRecv() SCC_MALLOC() error " HERE);
			ret = CC_CMN_RESULT_MALLOC_ERR;
			break;
		}

		///URI生成
		ret = CC_UserpolicyReq_CreateUri(pPrm, lang, uri, CC_CMN_URI_STR_MAX_LEN);
		if(CC_CMN_RESULT_OK != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserpolicyReq] CC_UserpolicyReq_SendRecv() CC_UserpolicyReq_CreateUri() error " HERE);
			break;
		}

		///HTTPデータ送受信
		cal_ret = SC_CAL_GetRequest(smcal, uri, pRecvBuf, RecvBuf_sz, &recv_sz, &opt);
		if(CC_CMN_RESULT_SMCAL_OK != cal_ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserpolicynReq] CC_Userpolicy_SendRecv() SC_CAL_GetRequest() error：%x " HERE, cal_ret);
			ret = CC_CMN_RESULT_COMM_ERR;				// ひとまず通信エラーにする
			break;
		}

		///HTTPデータ解析
		cal_ret = SC_CAL_AnalyzeResponseStatus(smcal, pRecvBuf, recv_sz, (const Char**)&p_body, &body_sz, &contextType, &status);
		if(CC_CMN_RESULT_SMCAL_OK != cal_ret){
			if (CC_CMN_SERVER_STOP == status) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"server stop..., " HERE);
				ret = e_SC_RESULT_SERVER_STOP;
			} else {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserpolicyReq] CC_UserpolicyReq_SendRecv() SC_CAL_AnalyzeResponseStatus() error：%x " HERE, cal_ret);
				ret = CC_CMN_RESULT_COMM_ERR;				// ひとまず通信エラーにする
			}
			break;
		}

		///CICデータ解析
		ret = CC_UserpolicyReq_Analy(pPrm, lang, (const UINT8 *)p_body, body_sz, pApiSts, contextType, &opt);
		if(CC_CMN_RESULT_OK != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserpolicyReq] CC_UserpolicyReq_SendRecv() CC_UserpolicyReq_Analy() error " HERE);
			break;
		}

	} while (0);

	if (NULL != uri) {
		SCC_FREE(uri);
	}
	if (NULL != fp_user_policy) {
		fclose(fp_user_policy);
	}

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
 * @param [IN/OUT] userData ユーザデータ
 * @param [IN] name     タグ名
 * @param [OUT] atts    属性(未使用)
 * @return 処理結果(E_SC_RESULT)
 */
void XMLCALL CC_UserpolicyReq_StartElement(void *userData, const char *name, const char **atts)
{
	T_CC_USERPOLICYREQ_PARSER	*pParser = (T_CC_USERPOLICYREQ_PARSER*)userData;	// パーサ
	Char pFilePath[SCC_MAX_PATH] = {};

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
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserpolicyReq] CC_UserpolicyReq_StartElement() prm error[userData], " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			break;
		}
		if (NULL == name) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserpolicyReq] CC_UserpolicyReq_StartElement() prm error[name], " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			break;
		}

		// 初期化
		memset(pParser->buf, 0, (CC_CMN_XML_BUF_SIZE + 1));

		if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_ELGG)) {
			// <elgg>
			pParser->state = NODE_ELGG;
		}

		// <elgg>
		if (NODE_ELGG == pParser->state) {
			if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_STATUS)) {
				// <status>
				pParser->state = NODE_STATUS;
			} else if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_RESULT)) {
				// <result>
				pParser->state = NODE_RESULT;
			} else if (0 != strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_ELGG)) {
				pParser->state = NODE_ELGG_CHILD;
			}
		} else if (NODE_RESULT == pParser->state) {
			if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_API_STATUS)) {
				// <api_status>
				pParser->state = NODE_API_STATUS;
			} else if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_POLICY_VER)) {
				// <policy_ver>
				pParser->state = NODE_POLICY_VER;
			} else if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_POLICY_LANG)) {
				// <policy_lang>
				pParser->state = NODE_POLICY_LANG;
			} else if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_POLICY_CONTENT)) {
				// <policy_content>
				CC_GetTempDirPath(pFilePath);
				sprintf(&pFilePath[strlen(pFilePath)], "%s_%s", pParser->userpolicyInf.lang, CC_CMN_USERPOLICY_FILE_NAME);
				fp_user_policy = fopen(pFilePath, "w");
				if(NULL == fp_user_policy){
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserpolicyReq] CC_UserpolicyReq_StartElement() fopen() error " HERE);
					CB_Result = CC_CMN_RESULT_FILE_OPEN_ERR;
					break;
				}
				pParser->state = NODE_POLICY_CONTENT;
			} else {
				pParser->state = NODE_RESULT_CHILD;
			}
		} else {
			// 上記以外
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserpolicyReq] CC_UserpolicyReq_StartElement() format error, " HERE);
			CB_Result = CC_CMN_RESULT_COMM_ERR;
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
 * @param [IN/OUT] userData ユーザデータ
 * @param [IN] name     タグ名
 */
void XMLCALL CC_UserpolicyReq_EndElement(void *userData, const char *name)
{
	T_CC_USERPOLICYREQ_PARSER	*pParser = (T_CC_USERPOLICYREQ_PARSER*)userData;	// パーサ

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
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserpolicyReq] CC_UserpolicyReq_EndElement() prm error[userData], " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			break;
		}
		if (NULL == name) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserpolicyReq] CC_UserpolicyReq_EndElement() prm error[name], " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			break;
		}

		if (NODE_STATUS == pParser->state) {
			// <status>
			*(pParser->userpolicyInf.pStatus) = atoi((char*)pParser->buf);
			pParser->state = NODE_ELGG;
		} else if (NODE_RESULT == pParser->state) {
			// <result>
			pParser->state = NODE_ELGG;
		} else if (NODE_API_STATUS == pParser->state) {
			// <api_status>
			strcpy((char*)pParser->userpolicyInf.pApiStatus, (char*)pParser->buf);
			pParser->state = NODE_RESULT;
		} else if (NODE_POLICY_VER == pParser->state) {
			// <policy_ver>
			strcpy((char*)pParser->userpolicyInf.pVer, (char*)pParser->buf);
			pParser->state = NODE_RESULT;
		} else if (NODE_POLICY_LANG == pParser->state) {
			// <policy_lang>
			strcpy((char*)pParser->userpolicyInf.pLang, (char*)pParser->buf);
			pParser->state = NODE_RESULT;
		} else if (NODE_POLICY_CONTENT == pParser->state) {
			// <policy_content>
			pParser->state = NODE_RESULT;
		} else if (NODE_ELGG_CHILD == pParser->state) {
			pParser->state = NODE_ELGG;
		} else if (NODE_RESULT_CHILD == pParser->state) {
			pParser->state = NODE_RESULT;
		}
	} while (0);

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}

//************************************************
//CharacterData処理
//************************************************
/**
 * @brief 解析データ
 * @param [IN] userData ユーザデータ
 * @param [IN] data     データ
 * @param [IN] len      データ長
 * @return 処理結果(E_SC_RESULT)
 */
void XMLCALL CC_UserpolicyReq_CharacterData(void *userData, const XML_Char *data, INT32 len)
{
	T_CC_USERPOLICYREQ_PARSER	*pParser = (T_CC_USERPOLICYREQ_PARSER*)userData;	// パーサ
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
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserpolicyReq] CC_UserpolicyReq_CharacterData() prm error[userData], " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			break;
		}
		if (NULL == data) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserpolicyReq] CC_UserpolicyReq_CharacterData() prm error[data], " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			break;
		}

		// データをバッファにコピー
		bufLen = strlen((char*)pParser->buf);

		if (NODE_STATUS == pParser->state) {
			// <status>
			if (CC_CMN_XML_RES_STS_SIZE >(bufLen + len)) {
				memcpy(&pParser->buf[bufLen], data, len);
				pParser->buf[bufLen + len] = EOS;
			}
		} else if (NODE_API_STATUS == pParser->state) {
			// <api_status>
			if (CC_CMN_XML_RES_STS_CODE_SIZE > (bufLen + len)) {
				memcpy(&pParser->buf[bufLen], data, len);
				pParser->buf[bufLen + len] = EOS;
			}
		} else if (NODE_POLICY_VER == pParser->state) {
			// <policy_ver>
			if (CC_CMN_POLICYVER_STR_SIZE > (bufLen + len)) {
				memcpy(&pParser->buf[bufLen], data, len);
				pParser->buf[bufLen + len] = EOS;
			}
		} else if (NODE_POLICY_LANG == pParser->state) {
			// <policy_lang>
			if (CC_CMN_POLICYLANG_STR_SIZE > (bufLen + len)) {
				memcpy(&pParser->buf[bufLen], data, len);
				pParser->buf[bufLen + len] = EOS;
			}
		} else if (NODE_POLICY_CONTENT == pParser->state) {
			// <policy_content>
			// ファイル書き込み
			if (len != fwrite(data, 1, len, fp_user_policy)) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserpolicyReq] CC_UserpolicyReq_CharacterData() fwrite error, " HERE);
				CB_Result = CC_CMN_RESULT_PROC_ERR;
				break;
			}
		}
	} while (0);

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}

//---------------------------------------------------------------------------------
//内部関数
//---------------------------------------------------------------------------------
//************************************************
//URI生成
//************************************************
/**
 * @brief Userpolicy.req用URI生成
 * @param[in] pPrm SMS API関連パラメタテーブルポインタ
 * @param[out] pUri 生成URI出力先
 * @param[in] UriMax 生成URI出力先最大サイズ(byte)
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:DAL_DATA_ERROR
 */
E_SC_RESULT CC_UserpolicyReq_CreateUri(const T_CC_CMN_SMS_API_PRM* pPrm,
									const Char *lang,
									Char* pUri,
									INT32 UriMax)
{
	INT32 rslt = 0;
	E_SC_RESULT ret = CC_CMN_RESULT_OK;

	rslt = snprintf(												//成功時には、書き込まれたbyte数を返す(EOSは含まない)
				(char*)pUri,
				UriMax,
				"%s\?method=Userpolicy.req&lang=%s",
				pPrm->ApiPrmNavi.common_uri,						//「http://～/distribution/」までのURI部分
				lang												//言語
				);
	if ((rslt < 0) || (UriMax <= rslt)) {							//負なら明らかにエラー、EOS終端できなかった場合もエラー
		SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserpolicyReq] CC_UserpolicyReq_CreateUri() error len[%d] " HERE, rslt);
		ret = CC_CMN_RESULT_PROC_ERR;
	}

	return (ret);
}

//************************************************
//レスポンス解析
//************************************************
/**
 * @brief Userpolicy.req応答解析
 * @param[in] pPrm SMS API関連パラメタテーブルポインタ
 * @param[in] pBuf HTTP受信データ格納先
 * @param[in] pBufSz HTTP受信データサイズ
 * @param[out] pApiSts ステータスコード
 * @param[in] ContextType Content-Type(0:XML、1:バイナリ)
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:DAL_DATA_ERROR
 * @note センタアクセスキー生成も行う
 */
E_SC_RESULT CC_UserpolicyReq_Analy(T_CC_CMN_SMS_API_PRM* pPrm,
								const Char *lang,
								const UINT8* pBuf,
								INT32 BufSz,
								Char* pApiSts,
								E_CONTEXT_TYPE ContextType,
								const SMCALOPT *opt)
{
	E_SC_RESULT ret = CC_CMN_RESULT_OK;
	T_CC_CMN_SMS_RESPONSE_INFO rsp_inf = {};
	Char	apiSts[CC_CMN_XML_RES_STS_CODE_SIZE] = {};

	USE_IT(BufSz);

	do {

		if (E_TEXT_XML != ContextType) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserpolicyReq] CC_UserpolicyReq_Analy() Content-Type error " HERE);
			ret = CC_CMN_RESULT_PROC_ERR;
			break;
		}

		///XML解析部をコール
		rsp_inf.apiSts = apiSts;
		ret = CC_UserpolicyReq_XmlParse(lang, (const Char*)pBuf, &rsp_inf, pPrm->ApiPrmUser.policyver, pPrm->ApiPrmUser.policylang, opt);
		if (CC_CMN_RESULT_OK != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserpolicyReq] CC_UserpolicyReq_Analy() CC_UserpolicyReq_XmlParse() error " HERE);
			break;
		}

		//正常系のXMLとして解析できなかった場合
		if ((CC_CMN_XML_CIC_RES_STS_OK != rsp_inf.sts) || (true != CHECK_API_STATUS(rsp_inf.apiSts))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserpolicyReq] CC_UserpolicyReq_Analy() not nomar xml error " HERE);
			ret = e_SC_RESULT_SMS_API_ERR;
			break;
		}

#ifdef CC_DEBUG_MODE		//*******    デバッグ用表示   *******************************************************************
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[UserpolicyReq] CC_UserpolicyReq_Analy() status = %d ", rsp_inf.sts);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[UserpolicyReq] CC_UserpolicyReq_Analy() api_status = %s ", rsp_inf.apiSts);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[UserpolicyReq] CC_UserpolicyReq_Analy() policyver_dl = %s ", pPrm->ApiPrmUser.policyver);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[UserpolicyReq] CC_UserpolicyReq_Analy() policylang_dl = %s ", pPrm->ApiPrmUser.policylang);
#endif

	} while (0);

	return (ret);
}

//************************************************
//Userpolicy.req応答メッセージ解析
//************************************************
/*
 * @brief Userpolicy.req応答メッセージ解析
 * @param　pXml XMLデータ
 * @param  pResp_inf_t SMSレスポンス情報
 * @param　pVer 規約のバージョン情報
 * @param　pPolicy 規約情報
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:DAL_DATA_ERROR
 */
E_SC_RESULT CC_UserpolicyReq_XmlParse(const Char *lang,
									const Char* pXml,
									T_CC_CMN_SMS_RESPONSE_INFO* pResp_inf_t,
									Char* pVer,
									Char* pLang,
									const SMCALOPT *opt)
{
	E_SC_RESULT ret = CC_CMN_RESULT_OK;					// 戻り値
	T_CC_USERPOLICYREQ_PARSER userpolicy_parser_t = {};// Userpolicyパーサ
	Char buf[CC_CMN_XML_PARSE_DATA_SIZE + 1] = {};
	XML_Parser parser = NULL;
	INT32	done = 0;
	INT32	len = 0;
	INT32	parsedLen = 0;
	FILE	*fp = NULL;

	do {

		// パラメータチェック
		if (NULL == pXml) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserpolicyReq] CC_UserpolicyReq_XmlParse() prm error[pXml] " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			ret = CB_Result;
			break;
		}
		if ((NULL == pResp_inf_t) || (NULL == pResp_inf_t->apiSts)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserpolicyReq] CC_UserpolicyReq_XmlParse() prm error[pResp_inf_t] " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			ret = CB_Result;
			break;
		}
		if (NULL == pVer) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserpolicyReq] CC_UserpolicyReq_XmlParse() prm error[pVer] " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			ret = CB_Result;
			break;
		}
		if (NULL == pLang) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserpolicyReq] CC_UserpolicyReq_XmlParse() prm error[pLang] " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			ret = CB_Result;
			break;
		}

		// パラメータチェック
		if ((NULL == opt) || (true != opt->isResOutputFile)) {
			if (NULL == pXml) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserpolicyReq] CC_UserpolicyReq_XmlParse() prm error[xml], " HERE);
				CB_Result = e_SC_RESULT_FAIL;
				ret = CB_Result;
				break;
			}
		}

		// 初期化
		memset(pVer, 0, CC_CMN_POLICYVER_STR_SIZE);
		pResp_inf_t->sts = 0;
		*(pResp_inf_t->apiSts) = EOS;
		CB_Result = CC_CMN_RESULT_OK;
		userpolicy_parser_t.buf = (Char*)SCC_MALLOC(CC_CMN_XML_BUF_SIZE + 1);
		if (NULL == userpolicy_parser_t.buf) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserpolicyReq] CC_UserpolicyReq_XmlParse() SCC_MALLOC error, " HERE);
			CB_Result = CC_CMN_RESULT_MALLOC_ERR;
			ret = CB_Result;
			break;
		}
		// XMLパーサ生成
		parser = XML_ParserCreate(NULL);
		if (NULL == parser) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserpolicyReq] CC_UserpolicyReq_XmlParse() XML_ParserCreate() error, " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			ret = CB_Result;
			break;
		}

		userpolicy_parser_t.userpolicyInf.pStatus = &pResp_inf_t->sts;
		userpolicy_parser_t.userpolicyInf.pApiStatus = &pResp_inf_t->apiSts[0];
		userpolicy_parser_t.userpolicyInf.pVer = pVer;
		userpolicy_parser_t.userpolicyInf.pLang = pLang;
		userpolicy_parser_t.userpolicyInf.pPolicy = NULL;
		userpolicy_parser_t.userpolicyInf.lang = lang;

		// コールバック関数設定
		XML_SetUserData(parser, &userpolicy_parser_t);
		XML_SetElementHandler(parser, CC_UserpolicyReq_StartElement, CC_UserpolicyReq_EndElement);
		XML_SetCharacterDataHandler(parser, CC_UserpolicyReq_CharacterData);

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
				strncpy((char*)buf, &pXml[parsedLen], (sizeof(buf) - 1));
				len = (INT32)strlen(buf);
				parsedLen += len;
				if (strlen(pXml) <= parsedLen) {
					done = 1;
				} else {
					done = 0;
				}
			} else {
				len = (INT32)fread(buf, 1, (sizeof(buf) - 1), fp);
				done = (len < (sizeof(buf) - 1));
			}

			// XML解析
			if ((XML_STATUS_ERROR == XML_Parse(parser, (const char*)buf, len, done)) || (CC_CMN_RESULT_OK != CB_Result)) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserpolicyReq] CC_UserpolicyReq_XmlParse() XML_Parse() error(0x%08x), " HERE, CB_Result);
				CB_Result = CC_CMN_RESULT_COMM_ERR;
				ret = CB_Result;
				break;
			}

			if (!done) {
				// バッファクリア
				memset(buf, 0, (sizeof(buf) - 1));
			}
		}

	}while(0);

	if (NULL != userpolicy_parser_t.buf) {
		SCC_FREE(userpolicy_parser_t.buf);
	}

	if (NULL != parser) {
		XML_ParserFree(parser);
	}

	if (NULL != fp) {
		fclose(fp);
	}

	return (ret);
}
