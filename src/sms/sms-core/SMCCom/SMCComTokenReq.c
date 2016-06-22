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
//	SMCCom_TokenReq：Token.req処理
//
//------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------
//インクルード
//---------------------------------------------------------------------------------
#include "SMCComInternal.h"

//---------------------------------------------------------------------------------
//プロトタイプ宣言
//---------------------------------------------------------------------------------
//***   コールバック関数   ***
void XMLCALL CC_TokenReq_StartElement(void *userData, const char *name, const char **atts);
void XMLCALL CC_TokenReq_EndElement(void *userData, const char *name);
void XMLCALL CC_TokenReq_CharacterData(void *userData, const XML_Char *data, INT32 len);
//***   内部関数   ***
E_SC_RESULT CC_TokenReq_CreateUri(const T_CC_CMN_SMS_API_PRM* pPrm, Char* pUri, INT32 UriMax);	//URI生成
E_SC_RESULT CC_TokenReq_Analy(T_CC_CMN_SMS_API_PRM* pPrm, const UINT8* pBuf, INT32 BufSz, Char* pApiSts, E_CONTEXT_TYPE ContextType);	//レスポンス解析
E_SC_RESULT CC_TokenReq_XmlParse(const Char* pXml, T_CC_CMN_SMS_RESPONSE_INFO* pResp_inf_t, Char* pToken);	//Token.req応答メッセージ解析
E_SC_RESULT CC_TokenReq_CreateTermSig(T_CC_CMN_SMS_API_PRM* pPrm);		//Term_sig生成


//---------------------------------------------------------------------------------
//変数定義
//---------------------------------------------------------------------------------
static INT32	CB_Result;				// 処理結果 (コールバック関数用)


//---------------------------------------------------------------------------------
//外部関数
//---------------------------------------------------------------------------------
//************************************************
//Token.req送信・受信処理
//************************************************
/**
 * @brief Token.req処理
 * @param[in] pPrm SMS API関連パラメタテーブルポインタ
 * @param[in] pRecvBuf 受信バッファポインタ
 * @param[in] RecvBuf_sz 受信バッファサイズ
 * @param[out] pUsrMsg ユーザメッセージ(地図センタからのユーザ向けエラー文言)
 * @param[out] pApiSts APIステータスコード
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_OK以外 (コール関数の戻り値)
 */
E_SC_RESULT CC_TokenReq_SendRecv(SMCAL* smcal,
									T_CC_CMN_SMS_API_PRM* pPrm,
									Char* pRecvBuf,
									UINT32 RecvBuf_sz,
									Char* pApiSts)
{
	E_SC_RESULT ret = CC_CMN_RESULT_OK;
	E_SC_CAL_RESULT	cal_ret = CC_CMN_RESULT_SMCAL_OK;
	//INT32 resp_sz = 0;							// httpレスポンスサイズ(ヘッダ含む)
	Char *p_body = NULL;						// httpレスポンスボディ部へのポインタ
	INT32 body_sz = 0;							// httpレスポンスボディ部サイズ
	E_CONTEXT_TYPE contextType;					// Content-Type
	Char *uri = NULL;							// URI生成用文字バッファ
	UINT32 recv_sz = 0;
	SMCALOPT	opt = {};
	INT32	status = 0;

	do {

		// 初期化
		opt.cancel = SCC_IsCancel;
#ifdef SC_CMN_BASIC_AUTH_MUPS
		// BASIC認証
		opt.isBasicAuth = true;
		strcpy(opt.basic.basicAuthId, SC_CMN_BASIC_AUTH_ID);
		strcpy(opt.basic.basicAuthPwd, SC_CMN_BASIC_AUTH_PWD);
#endif

		// メモリ確保
		uri = (Char*)SCC_MALLOC(CC_CMN_URI_STR_MAX_LEN);
		if (NULL == uri) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[TokenReq] CC_TokenReq_SendRecv() SCC_MALLOC() error, " HERE);
			ret = CC_CMN_RESULT_MALLOC_ERR;
			break;
		}

		///URI生成
		ret = CC_TokenReq_CreateUri(pPrm, uri, CC_CMN_URI_STR_MAX_LEN);
		if(CC_CMN_RESULT_OK != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[TokenReq] CC_TokenReq_SendRecv() CreateUriTokenReq() error " HERE);
			break;
		}

		///HTTPデータ送受信
		cal_ret = SC_CAL_GetRequest(smcal, uri, pRecvBuf, RecvBuf_sz, &recv_sz, &opt);
		if(CC_CMN_RESULT_SMCAL_OK != cal_ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[TokenReq] CC_TokenReq_SendRecv() SC_CAL_GetRequest() error：%x " HERE, cal_ret);
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
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[TokenReq] CC_TokenReq_SendRecv() SC_CAL_AnalyzeResponseStatus() error：%x " HERE, cal_ret);
				ret = CC_CMN_RESULT_COMM_ERR;				// ひとまず通信エラーにする
			}
			break;
		}

		///CICデータ解析
		ret = CC_TokenReq_Analy(pPrm, (const UINT8 *)p_body, body_sz, pApiSts, contextType);
		if(CC_CMN_RESULT_OK != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[TokenReq] CC_TokenReq_SendRecv() CC_TokenReq_Analy() error " HERE);
			break;
		}

	} while (0);

	if (NULL != uri) {
		SCC_FREE(uri);
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
void XMLCALL CC_TokenReq_StartElement(void *userData, const char *name, const char **atts)
{
	T_CC_TOKENREQ_PARSER	*pParser = (T_CC_TOKENREQ_PARSER*)userData;	// パーサ

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
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[TokenReq] CC_TokenReq_StartElement() prm error[userData], " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			break;
		}
		if (NULL == name) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[TokenReq] CC_TokenReq_StartElement() prm error[name], " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			break;
		}

		// 初期化
		memset(pParser->buf, 0, (CC_CMN_XML_BUF_SIZE + 1));

		if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_CIC)) {
			// <cic_response>
			pParser->state = NODE_NONE;
		}

		// <cic_response>
		if (NODE_NONE == pParser->state) {
			if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_STATUS)) {
				// <status>
				pParser->state = NODE_STATUS;
			} else if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_TOKEN)) {
				// <token>
				pParser->state = NODE_TOKEN;
			}
		} else {
			// 上記以外
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[TokenReq] CC_TokenReq_StartElement() format error, " HERE);
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
void XMLCALL CC_TokenReq_EndElement(void *userData, const char *name)
{
	T_CC_TOKENREQ_PARSER	*pParser = (T_CC_TOKENREQ_PARSER*)userData;	// パーサ
	Char	*chr = NULL;

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
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[TokenReq] CC_TokenReq_EndElement() prm error[userData], " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			break;
		}
		if (NULL == name) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[TokenReq] CC_TokenReq_EndElement() prm error[name], " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			break;
		}

		if (NODE_STATUS == pParser->state) {
			// <status>
			strcpy((char*)pParser->tokenInf.pApiSts, (char*)pParser->buf);
			pParser->state = NODE_NONE;
		} else if (NODE_TOKEN == pParser->state) {
			// <token>
			chr = strchr(pParser->buf, ':');
			if (NULL != chr) {
				*chr = EOS;
				if ((CC_CMN_TOKEN_STR_SIZE - 1) == strlen((char*)pParser->buf)) {
					strcpy((char*)pParser->tokenInf.pToken, (char*)pParser->buf);
				}
			}
			pParser->state = NODE_NONE;
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
void XMLCALL CC_TokenReq_CharacterData(void *userData, const XML_Char *data, INT32 len)
{
	T_CC_TOKENREQ_PARSER	*pParser = (T_CC_TOKENREQ_PARSER*)userData;	// パーサ
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
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[TokenReq] CC_TokenReq_CharacterData() prm error[userData], " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			break;
		}
		if (NULL == data) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[TokenReq] CC_TokenReq_CharacterData() prm error[data], " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			break;
		}

		// データをバッファにコピー
		bufLen = strlen((char*)pParser->buf);

		if (NODE_STATUS == pParser->state) {
			// <status>
			if (CC_CMN_XML_RES_MUPS_STS_SIZE > (bufLen + len)) {
				memcpy(&pParser->buf[bufLen], data, len);
				pParser->buf[bufLen + len] = EOS;
			}
		} else if (NODE_TOKEN == pParser->state) {
			// <token>
			if (CC_CMN_XML_TOKEN_SIZE > (bufLen + len)) {
				memcpy(&pParser->buf[bufLen], data, len);
				pParser->buf[bufLen + len] = EOS;
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
 * @brief Token.req用URI生成
 * @param[in] pPrm SMS API関連パラメタテーブルポインタ
 * @param[out] pUri 生成URI出力先
 * @param[in] UriMax 生成URI出力先最大サイズ(byte)
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:DAL_DATA_ERROR
 */
E_SC_RESULT CC_TokenReq_CreateUri(const T_CC_CMN_SMS_API_PRM* pPrm,
									Char* pUri,
									INT32 UriMax)
{
	INT32 rslt = 0;
	E_SC_RESULT ret = CC_CMN_RESULT_OK;

	// 端末未登録の場合 (センタ採番新規端末IDの有無で判定)
	if(0 == pPrm->ApiPrmMups.new_term_id[0]){
		rslt = snprintf(
				//成功時には、書き込まれたbyte数を返す(EOSは含まない)
				(char*) pUri, UriMax, "%s\?method=Token.req&term_id=%s",
				pPrm->ApiPrmMups.common_uri,		//「http://～/distribution/」までのURI部分
				pPrm->ApiPrmNavi.reg_term_id		//登録用端末ID
				);
	}
	// 端末登録済みの場合
	else {
		rslt = snprintf(
				//成功時には、書き込まれたbyte数を返す(EOSは含まない)
				(char*) pUri, UriMax, "%s\?method=Token.req&term_id=%s",
				pPrm->ApiPrmMups.common_uri,		//「http://～/distribution/」までのURI部分
				pPrm->ApiPrmMups.new_term_id		//センタ採番新規端末ID
				);
	}

	if ((rslt < 0) || (UriMax <= rslt)) {			//負なら明らかにエラー、EOS終端できなかった場合もエラー
		SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[TokenReq] CC_TokenReq_CreateUri() error len[%d] " HERE, rslt);
		ret = CC_CMN_RESULT_PROC_ERR;
	}

	return (ret);
}

//************************************************
//レスポンス解析
//************************************************
/**
 * @brief Token.req応答解析
 * @param[in] pPrm SMS API関連パラメタテーブルポインタ
 * @param[in] pBuf HTTP受信データ格納先
 * @param[in] pBufSz HTTP受信データサイズ
 * @param[out] pApiSts APIステータスコード
 * @param[in] ContextType Content-Type(0:XML、1:バイナリ)
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:DAL_DATA_ERROR
 * @note センタアクセスキー生成も行う
 */
E_SC_RESULT CC_TokenReq_Analy(T_CC_CMN_SMS_API_PRM* pPrm,
								const UINT8* pBuf,
								INT32 BufSz,
								Char* pApiSts,
								E_CONTEXT_TYPE ContextType)
{
	E_SC_RESULT ret = CC_CMN_RESULT_OK;
	T_CC_CMN_SMS_RESPONSE_INFO rsp_inf = {};

	USE_IT(BufSz);

	do {

		if (E_TEXT_XML != ContextType) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[TokenReq] CC_TokenReq_Analy() Content-Type error " HERE);
			ret = CC_CMN_RESULT_PROC_ERR;
			break;
		}

		///XML解析部をコール
		rsp_inf.apiSts = pApiSts;
		ret = CC_TokenReq_XmlParse((const Char*)pBuf, &rsp_inf, pPrm->ApiPrmMups.token);
		if (CC_CMN_RESULT_OK != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[TokenReq] CC_TokenReq_Analy() CC_TokenReq_XmlParse() error " HERE);
			break;
		}

		//正常系のXMLとして解析できなかった場合
		if (CC_CMN_XML_CIC_RES_STS_OK != rsp_inf.sts) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[TokenReq] CC_TokenReq_Analy() not nomar xml error " HERE);
			ret = e_SC_RESULT_SMS_API_ERR;
			break;
		}

		///取得したトークンからセンタアクセスキーを生成
		///@todo センタ提供ライブラリの正式番がリリースされたらtermsig取得は修正が必要

		//Term_sig生成
		ret = CC_TokenReq_CreateTermSig(pPrm);

#ifdef UPDDAL_DEMO		// UPDDAL_DEMO
		memcpy((char*)mPrm.mApiPrmCic.term_sig, (const char*)(mpConfigData->privateConfig.termSig), DAL_DAT_TERM_SIG_STR_SIZE-1);
		pPrm->ApiPrmMups.term_sig[CC_CMN_TERM_SIG_STR_SIZE-1]=EOS;//構造体側でEOS終端されない対策
#endif

#ifdef CC_DEBUG_MODE		//*******    デバッグ用表示   *******************************************************************
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[TokenReq] CC_TokenReq_Analy() token = %s ", pPrm->ApiPrmMups.token);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[TokenReq] CC_TokenReq_Analy() status = %d ", rsp_inf.sts);
#endif

	} while (0);

	return (ret);
}

//************************************************
//Token.req応答メッセージ解析
//************************************************
/**
 * @brief Token.req応答メッセージ解析
 * @param　pXml XMLファイルのフルパス
 * @param  pResp_inf_t SMSレスポンス情報
 * @param　pToken token文字列
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:DAL_DATA_ERROR
 */
E_SC_RESULT CC_TokenReq_XmlParse(const Char* pXml,
									T_CC_CMN_SMS_RESPONSE_INFO* pResp_inf_t,
									Char* pToken)
{
	E_SC_RESULT ret = CC_CMN_RESULT_OK;			// 戻り値
	T_CC_TOKENREQ_PARSER token_parser_t = {};	// Tokenパーサ
	Char buf[CC_CMN_XML_PARSE_DATA_SIZE + 1] = {};
	XML_Parser parser = NULL;
	INT32	done = 0;
	INT32	len = 0;
	INT32	parsedLen = 0;

	do {

		// パラメータチェック
		if (NULL == pXml) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[TokenReq] CC_TokenReq_XmlParse() prm error[pXml] " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			ret = CB_Result;
			break;
		}
		if ((NULL == pResp_inf_t) || (NULL == pResp_inf_t->apiSts)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[TokenReq] CC_TokenReq_XmlParse() prm error[pResp_inf_t] " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			ret = CB_Result;
			break;
		}
		if (NULL == pToken) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[TokenReq] CC_TokenReq_XmlParse() prm error[pToken] " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			ret = CB_Result;
			break;
		}

		// 初期化
		memset(pToken, 0, CC_CMN_TOKEN_STR_SIZE);
		pResp_inf_t->sts = 0;
		*(pResp_inf_t->apiSts) = EOS;
		CB_Result = CC_CMN_RESULT_OK;
		token_parser_t.buf = (Char*)SCC_MALLOC(CC_CMN_XML_BUF_SIZE + 1);
		if (NULL == token_parser_t.buf) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[TokenReq] CC_TokenReq_XmlParse() SCC_MALLOC error, " HERE);
			CB_Result = CC_CMN_RESULT_MALLOC_ERR;
			ret = CB_Result;
			break;
		}
		// XMLパーサ生成
		parser = XML_ParserCreate(NULL);
		if (NULL == parser) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[TokenReq] CC_TokenReq_XmlParse() XML_ParserCreate() error, " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			ret = CB_Result;
			break;
		}

		token_parser_t.tokenInf.pStatus = &pResp_inf_t->sts;
		token_parser_t.tokenInf.pApiSts = pResp_inf_t->apiSts;
		token_parser_t.tokenInf.pToken = pToken;

		// コールバック関数設定
		XML_SetUserData(parser, &token_parser_t);
		XML_SetElementHandler(parser, CC_TokenReq_StartElement, CC_TokenReq_EndElement);
		XML_SetCharacterDataHandler(parser, CC_TokenReq_CharacterData);

		while (!done) {
			if (CC_ISCANCEL()) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"cancel, " HERE);
				CB_Result = e_SC_RESULT_CANCEL;
				ret = CB_Result;
				break;
			}

			strncpy((char*)buf, &pXml[parsedLen], (sizeof(buf) - 1));
			len = (INT32)strlen(buf);
			parsedLen += len;
			if (strlen(pXml) <= parsedLen) {
				done = 1;
			} else {
				done = 0;
			}

			// XML解析
			if ((XML_STATUS_ERROR == XML_Parse(parser, (const char*)buf, len, done)) || (CC_CMN_RESULT_OK != CB_Result)) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[TokenReq] CC_TokenReq_XmlParse() XML_Parse() error(0x%08x), " HERE, CB_Result);
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

	if (NULL != token_parser_t.buf) {
		SCC_FREE(token_parser_t.buf);
	}

	if (NULL != parser) {
		XML_ParserFree(parser);
	}

	return (ret);
}

//************************************************
//Term_sig生成
//************************************************
/**
 * @brief Term_sig生成
 * @param [IN/OUT] pPrm SMS API関連パラメタテーブルポインタ
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @note MD5SUM関数を通す前のベース文字列は、「"term_id"+Term_ID+"token"+Token」
 */
E_SC_RESULT CC_TokenReq_CreateTermSig(T_CC_CMN_SMS_API_PRM* pPrm)
{
	E_SC_RESULT ret = CC_CMN_RESULT_OK;
	T_CC_CMN_MD5_CTX ctx = {};
	Char base_string[8 + CC_CMN_TARGETID_STR_SIZE + 6 + CC_CMN_TOKEN_STR_SIZE] = {};
	UChar md5[CC_CMN_TERM_SIG_STR_SIZE] = {};

	// MD5SUM初期化
	CC_MD5_Init(&ctx);

	// Term_sigベース文字列作成
	// 端末未登録の場合 (センタ採番新規端末IDの有無で判定)
	if(0 == pPrm->ApiPrmMups.new_term_id[0]){
		sprintf(base_string, "%s%s%s%s", "term_id", pPrm->ApiPrmNavi.reg_term_id, "token", pPrm->ApiPrmMups.token);
	}
	// 端末登録済みの場合
	else{
		sprintf(base_string, "%s%s%s%s", "term_id", pPrm->ApiPrmMups.new_term_id, "token", pPrm->ApiPrmMups.token);
	}

#if 1	//******************************************************************************************************************   デバッグ用処理
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[TokenReq] CC_TokenReq_CreateTermSig() base_string=%s, " HERE, base_string);
#endif	//******************************************************************************************************************   デバッグ用処理

	// MD5SUMダイジェスト更新
	CC_MD5_Update(&ctx, (UINT8*)base_string, strlen(base_string));

	// MD5SUM終了化
	CC_MD5_Final(md5, &ctx);

	// Term_sig生成(md5から下位64BIT抽出→16HEXキャラクタ変換なので、後ろ8byteを16HEXキャラクタで文字列作成)
	sprintf(pPrm->ApiPrmMups.term_sig, "%02x%02x%02x%02x%02x%02x%02x%02x",
			md5[8], md5[9], md5[10], md5[11], md5[12], md5[13], md5[14], md5[15]);

#if 1	//******************************************************************************************************************   デバッグ用処理
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[TokenReq] CC_TokenReq_CreateTermSig() Term_sig=%02x%02x%02x%02x%02x%02x%02x%02x, " HERE,
						md5[8], md5[9], md5[10], md5[11], md5[12], md5[13], md5[14], md5[15]);
#endif	//******************************************************************************************************************   デバッグ用処理

	return (ret);
}

