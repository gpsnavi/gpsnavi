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

#define CC_PKGCOMMON_SEND_BODY_SIZE		256

//------------------------------------------------
// 変数定義
//------------------------------------------------

//------------------------------------------------
// 関数定義
//------------------------------------------------
static void CC_PkgCommon_CreateUri(const T_CC_CMN_SMS_API_PRM *param, const SMPKGCOMMONPARAM *apiParam, Char *uri);
static void CC_PkgCommon_CreateBody(const T_CC_CMN_SMS_API_PRM *parm, const SMPKGCOMMONPARAM *apiParam, const Char *appVer, Char *body);

/**
 * @brief パッケージ汎用API
 * @param [IN] smcal        SMCAL
 * @param [IN] parm         APIパラメータ
 * @param [IN] apiParam     ポータルAPIパラメータ
 * @param [OUT]jsonFilePath ポータルAPIのレスポンスを格納したJSONファイルのパス
 * @param [IN] recv         センタ受信データバッファ
 * @param [IN] recv         センタ受信データバッファサイズ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_PkgCommon_SendRecv(SMCAL *smcal,
								  const T_CC_CMN_SMS_API_PRM *parm,
								  const SMPKGCOMMONPARAM *apiParam,
								  const SMPKGWHITELIST *whiteList,
								  UINT32 whiteListNum,
								  Char *jsonFilePath,
								  Char *recv,
								  INT32 recvBufSize)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_CAL_RESULT	calRet = e_SC_CAL_RESULT_SUCCESS;
	Char	*uri = NULL;
	Char	*body = NULL;
	UINT32	bodySize = 0;
	E_CONTEXT_TYPE	contextType = E_TEXT_XML;
	SMCALOPT	opt = {};
	UINT32	recvSize = 0;
	Char	*data = NULL;
	INT32	status = 0;
	Char	*apiName = NULL;
	INT32	i = 0;
	INT32	apiNameLen = 0;
	INT32	jsonFilePathLen = 0;
	UINT32	num = 0;
	Bool	isAccept = false;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// 初期化
		opt.cancel = SCC_IsCancel;
		opt.isResOutputFile = true;
		CC_GetTempDirPath(jsonFilePath);
		jsonFilePathLen = strlen((char*)jsonFilePath);
		// メモリ確保
		apiNameLen = strlen((char*)apiParam->apiName);
		apiName = (Char*)SCC_MALLOC(apiNameLen + 1);
		if (NULL == apiName) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		for (i = 0; i < apiNameLen; i++) {
			if ('/' == apiParam->apiName[i]) {
				jsonFilePath[jsonFilePathLen + i] = '_';
			} else {
				jsonFilePath[jsonFilePathLen + i] = apiParam->apiName[i];
			}
		}
		memcpy(&jsonFilePath[jsonFilePathLen + apiNameLen], ".json", 6);
		strcpy(opt.resFilePath, jsonFilePath);
		opt.isResAcceptJson = true;
#ifdef SC_CMN_BASIC_AUTH_SMS
		// BASIC認証
		opt.isBasicAuth = true;
		strcpy(opt.basic.basicAuthId, SC_CMN_BASIC_AUTH_ID);
		strcpy(opt.basic.basicAuthPwd, SC_CMN_BASIC_AUTH_PWD);
#endif

#ifdef CC_CMN_PKGCMN_DOCHECK	// パッケージをチェックする
		strcpy(apiName, apiParam->apiName);
		if ('/' == apiName[strlen((char*)apiName) - 1]) {
			apiName[strlen((char*)apiName) - 1] = EOS;
		}

		// 使用可能なAPIかチェック
		for (num = 0; num < whiteListNum; num++) {
			if (0 == strcmp((char*)apiName, (char*)whiteList[num].apiName)) {
				// 許可されているAPI
				isAccept = true;
				break;
			}
		}
		if (!isAccept) {
			// 許可されていないAPI
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"unauthorized api(%s), " HERE, apiParam->apiName);
			ret = e_SC_RESULT_UNAUTHORIZED_API;
			break;
		}
#endif							// パッケージをチェックする

		// メモリ確保
		uri = (Char*)SCC_MALLOC(CC_CMN_URI_STR_MAX_LEN);
		if (NULL == uri) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		if (CC_PKGCOMMON_METHOD_POST == apiParam->method) {
			data = (Char*)SCC_MALLOC(CC_PKGCOMMON_SEND_BODY_SIZE + strlen((char*)apiParam->param));
			if (NULL == data) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
				ret = e_SC_RESULT_MALLOC_ERR;
				break;
			}
		}

		// URI生成
		CC_PkgCommon_CreateUri(parm, apiParam, uri);

		if (CC_PKGCOMMON_METHOD_GET == apiParam->method) {
			// GET
			///HTTPデータ送受信
			calRet = SC_CAL_GetRequest(smcal, uri, recv, recvBufSize, &recvSize, &opt);
			if (e_SC_CAL_RESULT_SUCCESS != calRet) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SC_CAL_GetRequest error, " HERE);
				ret = ConvertResult(calRet);
				break;
			}
		} else {
			// POST
			// body生成
			CC_PkgCommon_CreateBody(parm, apiParam, parm->ApiPrmNavi.appVer, data);

			///HTTPデータ送受信
			calRet = SC_CAL_PostRequest(smcal, uri, data, strlen(data), recv, recvBufSize, &recvSize, &opt);
			if (e_SC_CAL_RESULT_SUCCESS != calRet) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SC_CAL_PostRequest error, " HERE);
				ret = ConvertResult(calRet);
				break;
			}
		}

		// HTTPデータ解析
		calRet = SC_CAL_AnalyzeResponseStatus(smcal, recv, recvSize, (const Char**)&body, &bodySize, &contextType, &status);
		if (e_SC_CAL_RESULT_SUCCESS != calRet) {
			if (CC_CMN_SERVER_STOP == status) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"server stop..., " HERE);
				ret = e_SC_RESULT_SERVER_STOP;
			} else {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SC_CAL_AnalyzeResponse error, " HERE);
				ret = ConvertResult(calRet);
			}
			break;
		}
	} while (0);

	// メモリ解放
	if (NULL != apiName) {
		SCC_FREE(apiName);
	}
	if (NULL != uri) {
		SCC_FREE(uri);
	}
	if (NULL != data) {
		SCC_FREE(data);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief URL生成
 * @param [IN]  parm        APIパラメータ
 * @param [OUT] uri         URL
 * @return 処理結果(E_SC_RESULT)
 */
void CC_PkgCommon_CreateUri(const T_CC_CMN_SMS_API_PRM *parm,
							const SMPKGCOMMONPARAM *apiParam,
							Char *uri)
{
	if (CC_PKGCOMMON_FRAMEWORK_ELLG == apiParam->framework) {
		sprintf((char*)uri,
				"%s?method=%s",
				parm->ApiPrmNavi.common_uri,
				apiParam->apiName
		);
	} else {
		sprintf((char*)uri,
				"%s%s",
				parm->ApiPrmNavi.sms_sp_uri,
				apiParam->apiName
		);
	}
}

/**
 * @brief POSTパラメータ生成
 * @param [IN]  parm        APIパラメータ
 * @param [IN]  apiParam    ポータルAPIパラメータ
 * @param [IN]  appVer      端末アプリVer
 * @param [OUT] body        body
 */
void CC_PkgCommon_CreateBody(const T_CC_CMN_SMS_API_PRM *parm,
							 const SMPKGCOMMONPARAM *apiParam,
							 const Char *appVer,
							 Char *body)
{
	if ((NULL == apiParam->param) || (EOS == *apiParam->param)) {
		sprintf((char*)body,
				"term_id=%s&term_sig=%s&guid=%s&user_sig=%s&app_ver=%s",
				parm->ApiPrmMups.new_term_id,
				parm->ApiPrmMups.term_sig,
				parm->ApiPrmMups.guid,
				parm->ApiPrmMups.user_sig,
				appVer
		);
	} else {
		sprintf((char*)body,
				"term_id=%s&term_sig=%s&guid=%s&user_sig=%s&%s&app_ver=%s",
				parm->ApiPrmMups.new_term_id,
				parm->ApiPrmMups.term_sig,
				parm->ApiPrmMups.guid,
				parm->ApiPrmMups.user_sig,
				apiParam->param,
				appVer
		);
	}
}
