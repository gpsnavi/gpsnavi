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
//	SMCCom_AgreeAuthReq：Auth.Req処理
//
//------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------
//インクルード
//---------------------------------------------------------------------------------
#include "SMCComInternal.h"

#define CC_AGREEAUTH_REQ_SEND_BODY_SIZE		3072

enum AgreeAuthreqRespStatus {
	NODE_AUTHREQ_NONE = 0,
	NODE_AUTHREQ_XML,
	NODE_AUTHREQ_XML_CHILD,
	NODE_AUTHREQ_API_STATUS,
	NODE_AUTHREQ_GUID,
	NODE_AUTHREQ_USER_SIG,
	NODE_AUTHREQ_ACT_STATUS,
	NODE_AUTHREQ_POLICY_LATE_FLG,
	NODE_AUTHREQ_LANG,
	NODE_AUTHREQ_DIALOG_FLG
};

//---------------------------------------------------------------------------------
//プロトタイプ宣言
//---------------------------------------------------------------------------------
//***   コールバック関数   ***
void XMLCALL CC_AgreeAuthReq_StartElement(void *userData, const char *name, const char **atts);
void XMLCALL CC_AgreeAuthReq_EndElement(void *userData, const char *name);
void XMLCALL CC_AgreeAuthReq_CharacterData(void *userData, const XML_Char *data, INT32 len);
//***   内部関数   ***
E_SC_RESULT CC_AgreeAuthReq_CreateUri(const T_CC_CMN_SMS_API_PRM* pPrm, Char* pUri, INT32 UriMax);	//URI生成
E_SC_RESULT CC_AgreeAuthReq_CreateBody(const T_CC_CMN_SMS_API_PRM* pPrm, Char* pBody);	//body生成
E_SC_RESULT CC_AgreeAuthReq_Analy(T_CC_CMN_SMS_API_PRM* pPrm, const UINT8* apBuf, INT32 aBufSz, Char* apApiSts, E_CONTEXT_TYPE aContextType);	//レスポンス解析
E_SC_RESULT CC_AgreeAuthReq_XmlParse(const Char* apXml, T_CC_CMN_SMS_RESPONSE_INFO* p_resp_inf, Char* pUserSig, Char* pGuid, Char* pActStatus, Char* pPolicyLateFlg, INT32* pRatingDialog);	//Auth.req応答メッセージ解析	// TR3.5 アクティベーション対応


//---------------------------------------------------------------------------------
//変数定義
//---------------------------------------------------------------------------------
static INT32	CB_Result;				// 処理結果 (コールバック関数用)


//---------------------------------------------------------------------------------
//外部関数
//---------------------------------------------------------------------------------
//************************************************
//Auth.Req送信・受信処理
//************************************************
/**
 * @brief Auth.Req処理
 * @param[in] pPrm SMS API関連パラメタテーブルポインタ
 * @param[in] pRecvBuf 受信バッファポインタ
 * @param[in] RecvBuf_sz 受信バッファサイズ
 * @param[out] pApiSts APIステータスコード
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_OK以外 (コール関数の戻り値)
 */
E_SC_RESULT CC_AgreeAuthReq_SendRecv(SMCAL* smcal,
									 T_CC_CMN_SMS_API_PRM* pPrm,
									 Char* pRecvBuf,
									 UINT32 RecvBuf_sz,
									 Char* pApiSts,
									 Bool isPolling)
{
	E_SC_RESULT ret = CC_CMN_RESULT_OK;		// 戻り値(処理結果)
	E_SC_CAL_RESULT	cal_ret = CC_CMN_RESULT_SMCAL_OK;	// HTTPデータ系処理結果
	Char* data = NULL;
	//INT32 resp_sz = 0;						// httpレスポンスサイズ(ヘッダ含む)
	Char* p_body = NULL;					// httpレスポンスボディ部へのポインタ
	INT32 body_sz = 0;						// httpレスポンスボディ部サイズ
	E_CONTEXT_TYPE contextType = E_TEXT_XML;// Content-Type
	Char *uri = NULL;						// URI生成用文字バッファ
	UINT32 recv_sz = 0;
	SMCALOPT	opt = {};
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
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[AgreeAuthReq] CC_AgreeAuthReq_SendRecv() SCC_MALLOC() error " HERE);
			ret = CC_CMN_RESULT_MALLOC_ERR;
			break;
		}
		data = (Char*)SCC_MALLOC(CC_AGREEAUTH_REQ_SEND_BODY_SIZE);
		if (NULL == data) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[AgreeAuthReq] CC_AgreeAuthReq_SendRecv() SCC_MALLOC() error " HERE);
			ret = CC_CMN_RESULT_MALLOC_ERR;
			break;
		}

		///URI生成
		ret = CC_AgreeAuthReq_CreateUri(pPrm, uri, CC_CMN_URI_STR_MAX_LEN);
		if(CC_CMN_RESULT_OK != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[AgreeAuthReq] CC_AgreeAuthReq_SendRecv() CC_AgreeAuthReq_CreateUri() error " HERE);
			break;
		}

		///body生成
		CC_AgreeAuthReq_CreateBody(pPrm, data);
		if(CC_CMN_RESULT_OK != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[AgreeAuthReq] CC_AgreeAuthReq_SendRecv() CC_AgreeAuthReq_CreateBody() error " HERE);
			break;
		}

		///HTTPデータ送受信
		cal_ret = SC_CAL_PostRequest(smcal, uri, data, strlen(data), pRecvBuf, RecvBuf_sz, &recv_sz, &opt);
		if(CC_CMN_RESULT_SMCAL_OK != cal_ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[AgreeAuthReq] CC_AgreeAuthReq_SendRecv() SC_CAL_PostRequest() error：%x " HERE, cal_ret);
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
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[AgreeAuthReq] CC_AgreeAuthReq_SendRecv() SC_CAL_AnalyzeResponseStatus() error：%x " HERE, cal_ret);
				ret = CC_CMN_RESULT_COMM_ERR;				// ひとまず通信エラーにする
			}
			break;
		}

		///CICデータ解析
		ret = CC_AgreeAuthReq_Analy(pPrm, (const UINT8 *)p_body, body_sz, pApiSts, contextType);
		if(CC_CMN_RESULT_OK != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[AgreeAuthReq] CC_AgreeAuthReq_SendRecv() CC_AgreeAuthReq_Analy() error " HERE);
			break;
		}

		// COOKIE取得(戻り値はチェックしない)
		SC_CAL_GetCookie(smcal, pRecvBuf, recv_sz, pPrm->ApiPrmUser.cookie);
	} while (0);

	if (NULL != uri) {
		SCC_FREE(uri);
	}
	if (NULL != data) {
		SCC_FREE(data);
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
 * @param [IN/OUT] userData ユーザデータ
 * @param [IN] name     タグ名
 * @param [OUT] atts    属性(未使用)
 * @return 処理結果(E_SC_RESULT)
 */
void XMLCALL CC_AgreeAuthReq_StartElement(void *userData, const char *name, const char **atts)
{
	T_CC_AGREEAUTHREQ_PARSER	*pParser = (T_CC_AGREEAUTHREQ_PARSER*)userData;	// パーサ

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
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[AgreeAuthReq] CC_AgreeAuthReq_StartElement() prm error[userData], " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			break;
		}
		if (NULL == name) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[AgreeAuthReq] CC_AgreeAuthReq_StartElement() prm error[name], " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			break;
		}

		// 初期化
		memset(pParser->buf, 0, (CC_CMN_XML_BUF_SIZE + 1));

		if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_XML)) {
			// <xml>
			pParser->state = NODE_AUTHREQ_XML;
		}

		// <xml>
		if (NODE_AUTHREQ_XML == pParser->state) {
			if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_API_STATUS)) {
				// <api_status>
				pParser->state = NODE_AUTHREQ_API_STATUS;
			} else if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_USER_SIG)) {
				// <user_sig>
				pParser->state = NODE_AUTHREQ_USER_SIG;
			} else if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_GUID)) {
				// <guid>
				pParser->state = NODE_AUTHREQ_GUID;
			} else if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_ACT_STATUS)) {		// TR3.5 アクティベーション対応
				// <act_status>
				pParser->state = NODE_AUTHREQ_ACT_STATUS;
			} else if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_POLICY_LATE_FLG)) {
				// <policy_late_flg>
				pParser->state = NODE_AUTHREQ_POLICY_LATE_FLG;
			} else if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_DIALOG_FLG)) {
				// <dialog_flg>
				pParser->state = NODE_AUTHREQ_DIALOG_FLG;
			} else if (0 != strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_XML)) {
				pParser->state = NODE_AUTHREQ_XML_CHILD;
			}
		} else {
			// 上記以外
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[AgreeAuthReq] CC_AgreeAuthReq_StartElement() format error, " HERE);
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
void XMLCALL CC_AgreeAuthReq_EndElement(void *userData, const char *name)
{
	T_CC_AGREEAUTHREQ_PARSER	*pParser = (T_CC_AGREEAUTHREQ_PARSER*)userData;	// パーサ

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
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[AgreeAuthReq] CC_AgreeAuthReq_EndElement() prm error[userData], " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			break;
		}
		if (NULL == name) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[AgreeAuthReq] CC_AgreeAuthReq_EndElement() prm error[name], " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			break;
		}

		if (NODE_AUTHREQ_API_STATUS == pParser->state) {
			// <api_status>
			strcpy((char*)pParser->authReqInf.pApiStatus, (char*)pParser->buf);
			pParser->state = NODE_AUTHREQ_XML;
		} else if (NODE_AUTHREQ_USER_SIG == pParser->state) {
			// <user_sig>
			strcpy((char*)pParser->authReqInf.pUserSig, (char*)pParser->buf);
			pParser->state = NODE_AUTHREQ_XML;
		} else if (NODE_AUTHREQ_GUID == pParser->state) {
			// <guid>
			strcpy((char*)pParser->authReqInf.pGuid, (char*)pParser->buf);
			pParser->state = NODE_AUTHREQ_XML;
		} else if (NODE_AUTHREQ_ACT_STATUS == pParser->state) {								// TR3.5 アクティベーション対応
			// <act_status>
			strcpy((char*)pParser->authReqInf.pActStatus, (char*)pParser->buf);
			pParser->state = NODE_AUTHREQ_XML;
		} else if (NODE_AUTHREQ_POLICY_LATE_FLG == pParser->state) {								// TR3.5 アクティベーション対応
			// <policy_late_flg>
			strcpy((char*)pParser->authReqInf.pPolicyLateFlg, (char*)pParser->buf);
			pParser->state = NODE_AUTHREQ_XML;
		} else if (NODE_AUTHREQ_DIALOG_FLG == pParser->state) {
			// <dialog_flg>
			if (SCC_CMN_RATING_DIALOG_SHOW == atoi((char*)pParser->buf)) {
				*(pParser->authReqInf.pRatingDialog) = SCC_CMN_RATING_DIALOG_SHOW_INTERNAL;
			} else {
				*(pParser->authReqInf.pRatingDialog) = SCC_CMN_RATING_DIALOG_NONSHOW_INTERNAL;
			}
			pParser->state = NODE_AUTHREQ_XML;
		} else if (NODE_AUTHREQ_XML_CHILD == pParser->state) {
			pParser->state = NODE_AUTHREQ_XML;
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
void XMLCALL CC_AgreeAuthReq_CharacterData(void *userData, const XML_Char *data, INT32 len)
{
	T_CC_AGREEAUTHREQ_PARSER	*pParser = (T_CC_AGREEAUTHREQ_PARSER*)userData;	// パーサ
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
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[AgreeAuthReq] CC_AgreeAuthReq_CharacterData() prm error[userData], " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			break;
		}
		if (NULL == data) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[AgreeAuthReq] CC_AgreeAuthReq_CharacterData() prm error[data], " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			break;
		}

		// データをバッファにコピー
		bufLen = strlen((char*)pParser->buf);

		if (NODE_AUTHREQ_API_STATUS == pParser->state) {
			// <api_status>
			if (CC_CMN_XML_RES_STS_CODE_SIZE > (bufLen + len)) {
				memcpy(&pParser->buf[bufLen], data, len);
				pParser->buf[bufLen + len] = EOS;
			}
		} else if (NODE_AUTHREQ_USER_SIG == pParser->state) {
			// <user_sig>
			if (CC_CMN_USER_SIG_STR_SIZE > (bufLen + len)) {
				memcpy(&pParser->buf[bufLen], data, len);
				pParser->buf[bufLen + len] = EOS;
			}
		} else if (NODE_AUTHREQ_GUID == pParser->state) {
			// <guid>
			if (CC_CMN_GUID_STR_SIZE > (bufLen + len)) {
				memcpy(&pParser->buf[bufLen], data, len);
				pParser->buf[bufLen + len] = EOS;
			}
		} else if (NODE_AUTHREQ_ACT_STATUS == pParser->state) {					// TR3.5 アクティベーション対応
			// <act_status>
			if (CC_CMN_ACT_STATUS_STR_SIZE > (bufLen + len)) {
				memcpy(&pParser->buf[bufLen], data, len);
				pParser->buf[bufLen + len] = EOS;
			}
		} else if (NODE_AUTHREQ_POLICY_LATE_FLG == pParser->state) {
			// <policy_late_flg>
			if (CC_CMN_POLICY_LATE_FLG_STR_SIZE > (bufLen + len)) {
				memcpy(&pParser->buf[bufLen], data, len);
				pParser->buf[bufLen + len] = EOS;
			}
		} else if (NODE_AUTHREQ_DIALOG_FLG == pParser->state) {
			// <dialog_flg>
			if (CC_CMN_RATING_DIALOG_STR_SIZE > (bufLen + len)) {
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
 * @brief Auth.Req用URI生成
 * @param[in] pPrm SMS API関連パラメタテーブルポインタ
 * @param[out] pUri 生成URI出力先
 * @param[in] UriMax 生成URI出力先最大サイズ(byte)
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_PROC_ERR
 * @note term_sigは直前のToken.Reqのレスポンス処理時に生成した端末アクセスキーを使用
*/
E_SC_RESULT CC_AgreeAuthReq_CreateUri(const T_CC_CMN_SMS_API_PRM* pPrm,
									  Char* pUri,
									  INT32 UriMax)
{
	INT32 rslt = 0;
	E_SC_RESULT ret = CC_CMN_RESULT_OK;

	rslt = snprintf(								//成功時には、書き込まれたbyte数を返す(EOSは含まない)
				(char*)pUri,
				UriMax,
				"%sagreeauth/req/",
				pPrm->ApiPrmNavi.sms_sp_uri
				);
	if((rslt < 0) || (UriMax <= rslt)){				//負なら明らかにエラー、EOS終端できなかった場合もエラー
		SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[AgreeAuthReq] CC_AgreeAuthReq_CreateUri() error len[%d] " HERE, rslt);
		ret = CC_CMN_RESULT_PROC_ERR;
	}

	return (ret);
}

//************************************************
//body生成
//************************************************
/**
 * @brief Auth.Req用body生成
 * @param[in] pPrm SMS API関連パラメタテーブルポインタ
 * @param[out] pBody 生成URI出力先
 * @param[in] bodyMax 生成URI出力先最大サイズ(byte)
 * @return 処理結果(E_SC_RESULT)
 * @note term_sigは直前のToken.Reqのレスポンス処理時に生成した端末アクセスキーを使用
*/
E_SC_RESULT CC_AgreeAuthReq_CreateBody(const T_CC_CMN_SMS_API_PRM* pPrm,
										Char* pBody)
{
	E_SC_RESULT ret = CC_CMN_RESULT_OK;
	Char	*loginId = NULL;
	Char	*password = NULL;
	INT32	loginIdLen = 0;
	INT32	passwordLen = 0;

	do {
		// メモリ確保
		loginIdLen = (CC_CMN_LOGINID_STR_SIZE * 3);
		loginId = (Char*)SCC_MALLOC(loginIdLen);
		if (NULL == loginId) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserReg] SCC_MALLOC error " HERE);
			ret = CC_CMN_RESULT_MALLOC_ERR;
			break;
		}
		memset(loginId, 0, loginIdLen);

		passwordLen = (CC_CMN_PASSWORD_STR_SIZE * 3);
		password = (Char*)SCC_MALLOC(passwordLen);
		if (NULL == password) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserReg] SCC_MALLOC error " HERE);
			ret = CC_CMN_RESULT_MALLOC_ERR;
			break;
		}
		memset(password, 0, passwordLen);

		// URLエンコード
		SC_CAL_UrlEncode(pPrm->ApiPrmUser.login_id, loginIdLen, loginId);
		SC_CAL_UrlEncode(pPrm->ApiPrmUser.password, passwordLen, password);

		sprintf((char*)pBody,
				"language=%s&agree_version=%s&term_id=%s&term_sig=%s&login_id=%s&passwd=%s&app_ver=%s",
				pPrm->ApiPrmUser.policylang,
				pPrm->ApiPrmUser.policyver,
				pPrm->ApiPrmMups.new_term_id,		//端末ID (センター採番新規端末ID)
				pPrm->ApiPrmMups.term_sig,			//端末アクセスキー
				loginId,							//ログインID
				password,							//パスワード
				pPrm->ApiPrmNavi.appVer				//端末アプリVer
				);
	} while (0);

	// メモリ解放
	if (NULL != loginId) {
		SCC_FREE(loginId);
	}
	if (NULL != password) {
		SCC_FREE(password);
	}

	return (ret);
}

//************************************************
//レスポンス解析
//************************************************
/**
 * @brief Auth.Req応答解析
 * @param[in] pPrm SMS API関連パラメタテーブルポインタ
 * @param[in] apBuf HTTP受信データ格納先
 * @param[in] apBufSz HTTP受信データサイズ
 * @param[out] apApiSts APIステータスコード
 * @param[in] aContextType Content-Type(0:XML、1:バイナリ)
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:DAL_DATA_ERROR
 * @note ユーザーアクセスキー(user_sig)、GUID(guid)の退避も行う
 */
E_SC_RESULT CC_AgreeAuthReq_Analy(T_CC_CMN_SMS_API_PRM* pPrm,
								  const UINT8* apBuf,
								  INT32 aBufSz,
								  Char* apApiSts,
								  E_CONTEXT_TYPE aContextType)
{
	E_SC_RESULT ret = CC_CMN_RESULT_OK;
	T_CC_CMN_SMS_RESPONSE_INFO rsp_inf = {};
	//Char	apiSts[CC_CMN_XML_RES_STS_CODE_SIZE] = {};			// APIステータス

	USE_IT(aBufSz);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {

		if(E_TEXT_XML != aContextType){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[AgreeAuthReq] CC_AgreeAuthReq_Analy() Content-Type error " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}

		///XML解析部をコール
		rsp_inf.apiSts = apApiSts;
		ret = CC_AgreeAuthReq_XmlParse((const Char*)apBuf,
									   &rsp_inf,
									   pPrm->ApiPrmMups.user_sig,
									   pPrm->ApiPrmMups.guid,
									   pPrm->ApiPrmMups.act_status,
									   pPrm->ApiPrmUser.policyLateFlg,
									   &pPrm->ApiPrmUser.showRatingDialog);	// TR3.5 アクティベーション対応
		if(CC_CMN_RESULT_OK != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[AgreeAuthReq] CC_AgreeAuthReq_Analy() CC_AgreeAuthReq_XmlParse() error " HERE);
			break;
		}

		if (!CHECK_API_STATUS(rsp_inf.apiSts)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[AgreeAuthReq] CC_AgreeAuthReq_Analy() api status error, " HERE);
			// 規約の再同意が必要な場合
			if ((0 == strcmp("SMSAPIE072", rsp_inf.apiSts)) ||
				(SCC_CMN_POLICY_LATE_FLG_OLD == pPrm->ApiPrmUser.policyLateFlg[0])) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_ReAuthenticationUser() need policy agree, " HERE);
				ret = e_SC_RESULT_POLICY_AGREE;
			} else {
				ret = e_SC_RESULT_SMS_API_ERR;
			}
			break;
		}

#ifdef CC_DEBUG_MODE		//*******    デバッグ用表示   *******************************************************************
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[AgreeAuthReq] CC_AgreeAuthReq_Analy() api_status = %s ", rsp_inf.apiSts);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[AgreeAuthReq] CC_AgreeAuthReq_Analy() guid = %s ", pPrm->ApiPrmMups.guid);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[AgreeAuthReq] CC_AgreeAuthReq_Analy() user_sig = %s ", pPrm->ApiPrmMups.user_sig);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[AgreeAuthReq] CC_AgreeAuthReq_Analy() act_status = %s ", pPrm->ApiPrmMups.act_status);	// TR3.5 アクティベーション対応
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[AgreeAuthReq] CC_AgreeAuthReq_Analy() policy_late_flg = %s ", pPrm->ApiPrmUser.policyLateFlg);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[AgreeAuthReq] CC_AgreeAuthReq_Analy() showRatingDialog = %d ", pPrm->ApiPrmUser.showRatingDialog);
#endif

	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//Agreeauth.req応答メッセージ解析
//************************************************
/**
 * @brief Agreeauth.Req応答メッセージ解析
 * @param[in] apXml XMLデータ
 * @param[in] p_resp_inf CICレスポンス情報
 * @param[out] pUserSig user_sig文字列
 * @param[out] pGuid guid文字列
 * @param[out] pActStatus アクティベーション状態	// TR3.5 アクティベーション対応
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_PROC_ERR
 */
E_SC_RESULT CC_AgreeAuthReq_XmlParse(const Char* apXml,
									 T_CC_CMN_SMS_RESPONSE_INFO* p_resp_inf,
									 Char* pUserSig,
									 Char* pGuid,
									 Char* pActStatus,
									 Char* pPolicyLateFlg,
									 INT32* pRatingDialog)
{
	E_SC_RESULT ret = CC_CMN_RESULT_OK;		// 戻り値
	T_CC_AGREEAUTHREQ_PARSER authreqParser = {};		// パーサ
	Char buf[CC_CMN_XML_PARSE_DATA_SIZE + 1] = {};
	XML_Parser parser = NULL;
	INT32	done = 0;
	INT32	len = 0;
	INT32	parsedLen = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {

		// パラメータチェック
		if (NULL == apXml) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[AgreeAuthReq] CC_AgreeAuthReq_XmlParse() prm error[apXml] " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			ret = CB_Result;
			break;
		}
		if (NULL == p_resp_inf || NULL == p_resp_inf->apiSts) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[AgreeAuthReq] CC_AgreeAuthReq_XmlParse() prm error[p_resp_inf] " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			ret = CB_Result;
			break;
		}
		if (NULL == pUserSig) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[AgreeAuthReq] CC_AgreeAuthReq_XmlParse() prm error[pUserSig] " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			ret = CB_Result;
			break;
		}
		if (NULL == pGuid) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[AgreeAuthReq] CC_AgreeAuthReq_XmlParse() prm error[pGuid] " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			ret = CB_Result;
			break;
		}
		if (NULL == pActStatus) {					// TR3.5 アクティベーション対応
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[AgreeAuthReq] CC_AgreeAuthReq_XmlParse() prm error[pActStatus] " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			ret = CB_Result;
			break;
		}
		if (NULL == pPolicyLateFlg) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[AgreeAuthReq] CC_AgreeAuthReq_XmlParse() prm error[pPolicyLateFlg] " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			ret = CB_Result;
			break;
		}
		if (NULL == pRatingDialog) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[AgreeAuthReq] CC_AgreeAuthReq_XmlParse() prm error[pRatingDialog] " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			ret = CB_Result;
			break;
		}

		// 初期化
		memset(pUserSig, 0, CC_CMN_USER_SIG_STR_SIZE);
		memset(pGuid, 0, CC_CMN_GUID_STR_SIZE);
		memset(pActStatus, 0, CC_CMN_ACT_STATUS_STR_SIZE);				// TR3.5 アクティベーション対応
		memset(pPolicyLateFlg, 0, CC_CMN_POLICY_LATE_FLG_STR_SIZE);
		*pRatingDialog = SCC_CMN_RATING_DIALOG_NONSHOW_INTERNAL;
		p_resp_inf->sts = 0;
		*(p_resp_inf->apiSts) = EOS;
		CB_Result = CC_CMN_RESULT_OK;
		authreqParser.buf = (Char*)SCC_MALLOC(CC_CMN_XML_BUF_SIZE + 1);
		if (NULL == authreqParser.buf) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[AgreeAuthReq] CC_AgreeAuthReq_XmlParse() SCC_MALLOC error, " HERE);
			CB_Result = CC_CMN_RESULT_MALLOC_ERR;
			ret = CB_Result;
			break;
		}
		// XMLパーサ生成
		parser = XML_ParserCreate(NULL);
		if (NULL == parser) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[AgreeAuthReq] CC_AgreeAuthReq_XmlParse() XML_ParserCreate() error, " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			ret = CB_Result;
			break;
		}

		authreqParser.authReqInf.pApiStatus = &p_resp_inf->apiSts[0];
		authreqParser.authReqInf.pUserSig = pUserSig;
		authreqParser.authReqInf.pGuid = pGuid;
		authreqParser.authReqInf.pActStatus = pActStatus;				// TR3.5 アクティベーション対応
		authreqParser.authReqInf.pPolicyLateFlg = pPolicyLateFlg;
		authreqParser.authReqInf.pRatingDialog = pRatingDialog;

		// コールバック関数設定
		XML_SetUserData(parser, &authreqParser);
		XML_SetElementHandler(parser, CC_AgreeAuthReq_StartElement, CC_AgreeAuthReq_EndElement);
		XML_SetCharacterDataHandler(parser, CC_AgreeAuthReq_CharacterData);

		while (!done) {
			if (CC_ISCANCEL()) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"cancel, " HERE);
				CB_Result = e_SC_RESULT_CANCEL;
				ret = CB_Result;
				break;
			}

			strncpy((char*)buf, &apXml[parsedLen], (sizeof(buf) - 1));
			len = (INT32)strlen(buf);
			parsedLen += len;
			if (strlen(apXml) <= parsedLen) {
				done = 1;
			} else {
				done = 0;
			}

			// XML解析
			if ((XML_STATUS_ERROR == XML_Parse(parser, (const char*)buf, len, done)) || (CC_CMN_RESULT_OK != CB_Result)) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[AgreeAuthReq] CC_AgreeAuthReq_XmlParse() XML_Parse() error(0x%08x), " HERE, CB_Result);
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

	if (NULL != authreqParser.buf) {
		SCC_FREE(authreqParser.buf);
	}

	if (NULL != parser) {
		XML_ParserFree(parser);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}
