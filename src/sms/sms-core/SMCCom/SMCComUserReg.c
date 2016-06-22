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
//	SMCCom_UserReg：User.reg処理
//
//------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------
//インクルード
//---------------------------------------------------------------------------------
#include "SMCComInternal.h"

#define CC_USE_REG_SEND_BODY_SIZE	4096

//---------------------------------------------------------------------------------
//プロトタイプ宣言
//---------------------------------------------------------------------------------
//***   コールバック関数   ***
void XMLCALL CC_UserReg_StartElement(void *userData, const char *name, const char **atts);
void XMLCALL CC_UserReg_EndElement(void *userData, const char *name);
void XMLCALL CC_UserReg_CharacterData(void *userData, const XML_Char *data, INT32 len);
//***   内部関数   ***
E_SC_RESULT CC_UserReg_CreateUri(const T_CC_CMN_SMS_API_PRM* pPrm, Char* pUri, INT32 UriMax);	//URI生成
E_SC_RESULT CC_UserReg_CreateBody(const T_CC_CMN_SMS_API_PRM* pPrm, Char* pBody);	//body生成
E_SC_RESULT CC_UserReg_Analy(T_CC_CMN_SMS_API_PRM* pPrm, const UINT8* apBuf, INT32 aBufSz, Char* apApiSts, E_CONTEXT_TYPE aContextType);	//レスポンス解析
E_SC_RESULT CC_UserReg_XmlParse(const Char* apXml, T_CC_CMN_SMS_RESPONSE_INFO* p_resp_inf, Char* pActSts);	//User.reg応答メッセージ解析	// TR3.5 アクティベーション対応


//---------------------------------------------------------------------------------
//変数定義
//---------------------------------------------------------------------------------
static INT32	CB_Result;				// 処理結果 (コールバック関数用)


//---------------------------------------------------------------------------------
//外部関数
//---------------------------------------------------------------------------------
//************************************************
//User.reg送信・受信処理
//************************************************
/**
 * @brief User.reg処理
 * @param[in] pPrm SMS API関連パラメタテーブルポインタ
 * @param[in] pRecvBuf 受信バッファポインタ
 * @param[in] RecvBuf_sz 受信バッファサイズ
 * @param[out] pApiSts APIステータスコード
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_OK以外 (コール関数の戻り値)
 */
E_SC_RESULT CC_UserReg_SendRecv(SMCAL* smcal,
								T_CC_CMN_SMS_API_PRM* pPrm,
								Char* pRecvBuf,
								UINT32 RecvBuf_sz,
								Char* pApiSts)
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
	SMCALOPT opt = {};
	INT32	status = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {

		// 初期化
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
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserReg] CC_UserReg_SendRecv() SCC_MALLOC() error " HERE);
			ret = CC_CMN_RESULT_MALLOC_ERR;
			break;
		}
		data = (Char*)SCC_MALLOC(CC_USE_REG_SEND_BODY_SIZE);
		if (NULL == data) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserReg] CC_UserReg_SendRecv() SCC_MALLOC() error " HERE);
			ret = CC_CMN_RESULT_MALLOC_ERR;
			break;
		}

		///URI生成
		ret = CC_UserReg_CreateUri(pPrm, uri, CC_CMN_URI_STR_MAX_LEN);
		if(CC_CMN_RESULT_OK != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserReg] CC_UserReg_SendRecv() CC_UserReg_CreateUri() error " HERE);
			break;
		}

		///body生成
		ret = CC_UserReg_CreateBody(pPrm, data);
		if(CC_CMN_RESULT_OK != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserReg] CC_UserReg_SendRecv() CC_UserReg_CreateUri() error " HERE);
			break;
		}

		///HTTPデータ送受信
		cal_ret = SC_CAL_PostRequest(smcal, uri, data, strlen(data), pRecvBuf, RecvBuf_sz, &recv_sz, &opt);
		if(CC_CMN_RESULT_SMCAL_OK != cal_ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserReg] CC_UserReg_SendRecv() SC_CAL_PostRequest() error：%x " HERE, cal_ret);
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
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserReg] CC_UserReg_SendRecv() SC_CAL_AnalyzeResponseStatus() error：%x " HERE, cal_ret);
				ret = CC_CMN_RESULT_COMM_ERR;				// ひとまず通信エラーにする
			}
			break;
		}

		///CICデータ解析
		ret = CC_UserReg_Analy(pPrm, (const UINT8 *)p_body, body_sz, pApiSts, contextType);
		if(CC_CMN_RESULT_OK != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserReg] CC_UserReg_SendRecv() CC_UserReg_Analy() error " HERE);
			break;
		}

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
void XMLCALL CC_UserReg_StartElement(void *userData, const char *name, const char **atts)
{
	T_CC_USERREG_PARSER	*pParser = (T_CC_USERREG_PARSER*)userData;	// パーサ

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == userData) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserReg] CC_UserReg_StartElement() prm error[userData], " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			break;
		}
		if (NULL == name) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserReg] CC_UserReg_StartElement() prm error[name], " HERE);
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
			} else if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_ACT_STATUS)) {
				// <act_status>
				pParser->state = NODE_ACT_STATUS;
#if 0															// TR3.5 アクティベーション対応
			} else if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_GUID)) {
				// <guid>
				pParser->state = NODE_GUID;
			} else if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_USER_SIG)) {
				// <user_sig>
				pParser->state = NODE_USER_SIG;
#endif															// TR3.5 アクティベーション対応
			} else {
				pParser->state = NODE_RESULT_CHILD;
			}
		} else {
			// 上記以外
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserReg] CC_UserReg_StartElement() format error, " HERE);
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
void XMLCALL CC_UserReg_EndElement(void *userData, const char *name)
{
	T_CC_USERREG_PARSER	*pParser = (T_CC_USERREG_PARSER*)userData;	// パーサ

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		if (CC_CMN_RESULT_OK != CB_Result) {
			break;
		}

		// パラメータチェック
		if (NULL == userData) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserReg] CC_UserReg_EndElement() prm error[userData], " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			break;
		}
		if (NULL == name) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserReg] CC_UserReg_EndElement() prm error[name], " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			break;
		}

		if (NODE_STATUS == pParser->state) {
			// <status>
			*(pParser->userRegInf.pStatus) = atoi((char*)pParser->buf);
			pParser->state = NODE_ELGG;
		} else if (NODE_RESULT == pParser->state) {
			// <result>
			pParser->state = NODE_ELGG;
		} else if (NODE_API_STATUS == pParser->state) {
			// <api_status>
			strcpy((char*)pParser->userRegInf.pApiStatus, (char*)pParser->buf);
			pParser->state = NODE_RESULT;
		} else if (NODE_ACT_STATUS == pParser->state) {
			// <act_status>
			strcpy((char*)pParser->userRegInf.pActStatus, (char*)pParser->buf);		// TR3.5 アクティベーション対応
			pParser->state = NODE_RESULT;
#if 0															// TR3.5 アクティベーション対応
		} else if (NODE_GUID == pParser->state) {
			// <guid>
			strcpy((char*)pParser->userRegInf.pGuid, (char*)pParser->buf);
			pParser->state = NODE_RESULT;
		} else if (NODE_USER_SIG == pParser->state) {
			// <user_sig>
			strcpy((char*)pParser->userRegInf.pUserSig, (char*)pParser->buf);
			pParser->state = NODE_RESULT;
#endif															// TR3.5 アクティベーション対応
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
void XMLCALL CC_UserReg_CharacterData(void *userData, const XML_Char *data, INT32 len)
{
	T_CC_USERREG_PARSER	*pParser = (T_CC_USERREG_PARSER*)userData;	// パーサ
	//char buf[CC_CMN_XML_PARSE_DATA_SIZE + 1] = {};
	INT32	bufLen = 0;

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		if (CC_CMN_RESULT_OK != CB_Result) {
			break;
		}

		// パラメータチェック
		if (NULL == userData) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserReg] CC_UserReg_CharacterData() prm error[userData], " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			break;
		}
		if (NULL == data) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserReg] CC_UserReg_CharacterData() prm error[data], " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			break;
		}

		// データをバッファにコピー
		bufLen = strlen((char*)pParser->buf);

		if (NODE_STATUS == pParser->state) {
			// <status>
			if (CC_CMN_XML_RES_STS_SIZE > (bufLen + len)) {
				memcpy(&pParser->buf[bufLen], data, len);
				pParser->buf[bufLen + len] = EOS;
			}
		} else if (NODE_API_STATUS == pParser->state) {
			// <api_status>
			if (CC_CMN_XML_RES_STS_CODE_SIZE > (bufLen + len)) {
				memcpy(&pParser->buf[bufLen], data, len);
				pParser->buf[bufLen + len] = EOS;
			}
		} else if (NODE_ACT_STATUS == pParser->state) {
			// <act_status>
			if (CC_CMN_ACT_STATUS_STR_SIZE > (bufLen + len)) {		// TR3.5 アクティベーション対応
				memcpy(&pParser->buf[bufLen], data, len);
				pParser->buf[bufLen + len] = EOS;
			}
#if 0															// TR3.5 アクティベーション対応
		} else if (NODE_GUID == pParser->state) {
			// <guid>
			if (CC_CMN_GUID_STR_SIZE > (bufLen + len)) {
				memcpy(&pParser->buf[bufLen], data, len);
				pParser->buf[bufLen + len] = EOS;
			}
		} else if (NODE_USER_SIG == pParser->state) {
			// <user_sig>
			if (CC_CMN_USER_SIG_STR_SIZE > (bufLen + len)) {
				memcpy(&pParser->buf[bufLen], data, len);
				pParser->buf[bufLen + len] = EOS;
			}
#endif															// TR3.5 アクティベーション対応
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
 * @brief User.reg用URI生成
 * @param[in] pPrm SMS API関連パラメタテーブルポインタ
 * @param[out] pUri 生成URI出力先
 * @param[in] UriMax 生成URI出力先最大サイズ(byte)
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_PROC_ERR
 * @note term_sigは直前のToken.Reqのレスポンス処理時に生成した端末アクセスキーを使用
*/
E_SC_RESULT CC_UserReg_CreateUri(const T_CC_CMN_SMS_API_PRM* pPrm,
									Char* pUri,
									INT32 UriMax)
{
	INT32 rslt = 0;
	E_SC_RESULT ret = CC_CMN_RESULT_OK;

	rslt = snprintf(								//成功時には、書き込まれたbyte数を返す(EOSは含まない)
				(char*)pUri,
				UriMax,
				"%s\?method=User.reg",
				pPrm->ApiPrmNavi.common_uri			//「http://～/distribution/」までのURI部分
				);
	if((rslt < 0) || (UriMax <= rslt)){				//負なら明らかにエラー、EOS終端できなかった場合もエラー
		SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserReg] CC_UserReg_CreateUri() error len[%d] " HERE, rslt);
		ret = CC_CMN_RESULT_PROC_ERR;
	}

	return (ret);
}

//************************************************
//body生成
//************************************************
/**
 * @brief User.reg用body生成
 * @param[in] pPrm SMS API関連パラメタテーブルポインタ
 * @param[out] pBody 生成body出力先
 * @return 処理結果(E_SC_RESULT)
 * @note term_sigは直前のToken.Reqのレスポンス処理時に生成した端末アクセスキーを使用
*/
E_SC_RESULT CC_UserReg_CreateBody(const T_CC_CMN_SMS_API_PRM* pPrm,
									Char* pBody)
{
	E_SC_RESULT ret = CC_CMN_RESULT_OK;
	Char	*userName = NULL;
	Char	*loginId = NULL;
	Char	*password = NULL;
	Char	*mail = NULL;
	INT32	userNameLen = 0;
	INT32	loginIdLen = 0;
	INT32	passwordLen = 0;
	INT32	mailLen = 0;

	do {
		// メモリ確保
		userNameLen = (CC_CMN_USERNAME_STR_SIZE * 3);
		userName = (Char*)SCC_MALLOC(userNameLen);
		if (NULL == userName) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserReg] SCC_MALLOC error " HERE);
			ret = CC_CMN_RESULT_MALLOC_ERR;
			break;
		}
		memset(userName, 0, userNameLen);

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

		mailLen = (CC_CMN_MALEADDR_STR_SIZE * 3);
		mail = (Char*)SCC_MALLOC(mailLen);
		if (NULL == mail) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserReg] SCC_MALLOC error " HERE);
			ret = CC_CMN_RESULT_MALLOC_ERR;
			break;
		}
		memset(mail, 0, mailLen);

		// URLエンコード
		SC_CAL_UrlEncode(pPrm->ApiPrmUser.user_name, userNameLen, userName);
		SC_CAL_UrlEncode(pPrm->ApiPrmUser.login_id, loginIdLen, loginId);
		SC_CAL_UrlEncode(pPrm->ApiPrmUser.password, passwordLen, password);
		SC_CAL_UrlEncode(pPrm->ApiPrmUser.mail_addr, mailLen, mail);

		sprintf((char*)pBody,
				"term_id=%s&term_sig=%s&user_name=%s&login_id=%s&passwd=%s&passwd_cnf=%s&mail_addr=%s&lang=%s&policyver=%s&policylang=%s&app_ver=%s",
				pPrm->ApiPrmMups.new_term_id,		//端末ID (センター採番新規端末ID)
				pPrm->ApiPrmMups.term_sig,			//端末アクセスキー
				userName,							//ユーザー名
				loginId,							//ログインID
				password,							//パスワード
				password,							//パスワードの再確認入力項目
				mail,								//メールアドレス
				pPrm->ApiPrmUser.lang,				//言語設定
				pPrm->ApiPrmUser.policyver,			//利用規約Ver
				pPrm->ApiPrmUser.policylang,		//利用規約言語タイプ
				pPrm->ApiPrmNavi.appVer				//端末アプリVer
				);
	} while (0);

	// メモリ解放
	if (NULL != userName) {
		SCC_FREE(userName);
	}
	if (NULL != password) {
		SCC_FREE(password);
	}
	if (NULL != mail) {
		SCC_FREE(mail);
	}

	return (ret);
}

//************************************************
//レスポンス解析
//************************************************
/**
 * @brief User.reg応答解析
 * @param[in] pPrm SMS API関連パラメタテーブルポインタ
 * @param[in] apBuf HTTP受信データ格納先
 * @param[in] apBufSz HTTP受信データサイズ
 * @param[out] apApiSts APIステータスコード
 * @param[in] aContextType Content-Type(0:XML、1:バイナリ)
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:DAL_DATA_ERROR
 */
E_SC_RESULT CC_UserReg_Analy(T_CC_CMN_SMS_API_PRM* pPrm,
								const UINT8* apBuf,
								INT32 aBufSz,
								Char* apApiSts,
								E_CONTEXT_TYPE aContextType)
{
	E_SC_RESULT ret = CC_CMN_RESULT_OK;
	T_CC_CMN_SMS_RESPONSE_INFO rsp_inf = {};

	USE_IT(aBufSz);

	do {

		if(E_TEXT_XML != aContextType){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserReg] CC_UserReg_Analy() Content-Type error " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}

		///XML解析部をコール
		rsp_inf.apiSts = apApiSts;
		ret = CC_UserReg_XmlParse((const Char*)apBuf, &rsp_inf, pPrm->ApiPrmMups.act_status);		// TR3.5 アクティベーション対応
		if(CC_CMN_RESULT_OK != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserReg] CC_UserReg_Analy() CC_UserReg_XmlParse() error " HERE);
			break;
		}

		//正常系のXMLとして解析できなかった場合
		if((CC_CMN_XML_CIC_RES_STS_OK != rsp_inf.sts) || (true != CHECK_API_STATUS(rsp_inf.apiSts))){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserReg] CC_UserReg_Analy() not nomar xml error " HERE);
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserReg] error status=%d " HERE, rsp_inf.sts);
			ret = e_SC_RESULT_SMS_API_ERR;
			break;
		}

#ifdef CC_DEBUG_MODE		//*******    デバッグ用表示   *******************************************************************
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[UserReg] CC_UserReg_Analy() status = %d ", rsp_inf.sts);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[UserReg] CC_UserReg_Analy() api_status = %s ", rsp_inf.apiSts);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[UserReg] CC_UserReg_Analy() act_status = %s ", pPrm->ApiPrmMups.act_status);		// TR3.5 アクティベーション対応
#endif

	} while (0);

	return (ret);
}

//************************************************
//User.reg応答メッセージ解析
//************************************************
/**
 * @brief User.reg応答メッセージ解析
 * @param[in] apXml XMLファイルのフルパス
 * @param[in] p_resp_inf CICレスポンス情報
 * @param[out] pActSts アクティベーション状態
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_PROC_ERR
 */
E_SC_RESULT CC_UserReg_XmlParse(const Char* apXml,
								T_CC_CMN_SMS_RESPONSE_INFO* p_resp_inf,
								Char* pActSts)
{
	E_SC_RESULT ret = CC_CMN_RESULT_OK;		// 戻り値
	T_CC_USERREG_PARSER userregParser = {};// パーサ
	Char buf[CC_CMN_XML_PARSE_DATA_SIZE + 1] = {};
	XML_Parser parser = NULL;
	INT32	done = 0;
	INT32	len = 0;
	INT32	parsedLen = 0;

	do {

		// パラメータチェック
		if (NULL == apXml) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserReg] CC_UserReg_XmlParse() prm error[apXml] " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			ret = CB_Result;
			break;
		}
		if (NULL == p_resp_inf || NULL == p_resp_inf->apiSts) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserReg] CC_UserReg_XmlParse() prm error[p_resp_inf] " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			ret = CB_Result;
			break;
		}
		if (NULL == pActSts) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserReg] CC_UserReg_XmlParse() prm error[pActSts] " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			ret = CB_Result;
			break;
		}

		// 初期化
		memset(pActSts, 0, CC_CMN_ACT_STATUS_STR_SIZE);			// TR3.5 アクティベーション対応
		p_resp_inf->sts = 0;
		*(p_resp_inf->apiSts) = EOS;
		CB_Result = CC_CMN_RESULT_OK;
		userregParser.buf = (Char*)SCC_MALLOC(CC_CMN_XML_BUF_SIZE + 1);
		if (NULL == userregParser.buf) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserReg] CC_UserReg_XmlParse() SCC_MALLOC error, " HERE);
			CB_Result = CC_CMN_RESULT_MALLOC_ERR;
			ret = CB_Result;
			break;
		}
		// XMLパーサ生成
		parser = XML_ParserCreate(NULL);
		if (NULL == parser) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserReg] CC_UserReg_XmlParse() XML_ParserCreate() error, " HERE);
			CB_Result = CC_CMN_RESULT_PROC_ERR;
			ret = CB_Result;
			break;
		}

		userregParser.userRegInf.pStatus = &p_resp_inf->sts;
		userregParser.userRegInf.pApiStatus = &p_resp_inf->apiSts[0];
		userregParser.userRegInf.pActStatus = pActSts;			// TR3.5 アクティベーション対応

		// コールバック関数設定
		XML_SetUserData(parser, &userregParser);
		XML_SetElementHandler(parser, CC_UserReg_StartElement, CC_UserReg_EndElement);
		XML_SetCharacterDataHandler(parser, CC_UserReg_CharacterData);

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
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[UserReg] CC_UserReg_XmlParse() XML_Parse() error(0x%08x), " HERE, CB_Result);
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

	if (NULL != userregParser.buf) {
		SCC_FREE(userregParser.buf);
	}

	if (NULL != parser) {
		XML_ParserFree(parser);
	}

	return (ret);
}

