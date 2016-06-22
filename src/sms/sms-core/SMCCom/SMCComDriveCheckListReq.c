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

#define CC_DRIVECHECKLIST_REQ_SEND_BODY_SIZE	512
#define	CC_DRIVECHECKLIST_REQ_FILE				"drivechecklist.res"

//------------------------------------------------
// 変数定義
//------------------------------------------------

//------------------------------------------------
// 関数定義
//------------------------------------------------
static void CC_DriveCheckListReq_CreateUri(const T_CC_CMN_SMS_API_PRM *param, Char *uri);
static void CC_DriveCheckListReq_CreateBody(const T_CC_CMN_SMS_API_PRM *parm, const Char* tripId, INT32 limit, const Char *appVer, Char *body);

/**
 * @brief 運転特性診断結果を取得する
 * @param [IN] smcal        SMCAL
 * @param [IN] parm         APIパラメータ
 * @param [IN] tripId       トリップID
 * @param [IN] limit        取得件数
 * @param [OUT] filePath    センタから受信したレスポンスを格納したファイルのフルパス
 * @param [IN] recv         センタ受信データバッファ
 * @param [IN] recv         センタ受信データバッファサイズ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_DriveCheckListReq_SendRecv(SMCAL *smcal,
										  const T_CC_CMN_SMS_API_PRM *parm,
										  const Char *tripId,
										  INT32 limit,
										  Char *filePath,
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

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// 初期化
		opt.cancel = SCC_IsCancel;
		opt.isResOutputFile = true;
		CC_GetTempDirPath(filePath);
		strcat(filePath, CC_DRIVECHECKLIST_REQ_FILE);
		strcpy(opt.resFilePath, filePath);
		opt.isResAcceptJson = true;
#ifdef SC_CMN_BASIC_AUTH_SMS
		// BASIC認証
		opt.isBasicAuth = true;
		strcpy(opt.basic.basicAuthId, SC_CMN_BASIC_AUTH_ID);
		strcpy(opt.basic.basicAuthPwd, SC_CMN_BASIC_AUTH_PWD);
#endif

		// メモリ確保
		uri = (Char*)SCC_MALLOC(CC_CMN_URI_STR_MAX_LEN);
		if (NULL == uri) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		data = (Char*)SCC_MALLOC(CC_DRIVECHECKLIST_REQ_SEND_BODY_SIZE);
		if (NULL == data) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// URI生成
		CC_DriveCheckListReq_CreateUri(parm, uri);

		// body生成
		CC_DriveCheckListReq_CreateBody(parm, tripId, limit, parm->ApiPrmNavi.appVer, data);

		// HTTPデータ送受信
		calRet = SC_CAL_PostRequest(smcal, uri, data, strlen(data), recv, recvBufSize, &recvSize, &opt);
		if (e_SC_CAL_RESULT_SUCCESS != calRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SC_CAL_PostRequest error, " HERE);
			ret = ConvertResult(calRet);
			break;
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
void CC_DriveCheckListReq_CreateUri(const T_CC_CMN_SMS_API_PRM *parm, Char *uri)
{
	sprintf((char*)uri,
			"%sdrivechecklist/req",
			parm->ApiPrmNavi.sms_sp_uri
	);
}

/**
 * @brief POSTパラメータ生成
 * @param [IN]  parm        APIパラメータ
 * @param [IN]  tripId      トリップID
 * @param [IN]  limit       取得件数
 * @param [IN]  appVer      端末アプリVer
 * @param [OUT] body        body
 */
void CC_DriveCheckListReq_CreateBody(const T_CC_CMN_SMS_API_PRM *parm,
									 const Char *tripId,
									 INT32 limit,
									 const Char *appVer,
									 Char *body)
{
	sprintf((char*)body,
			"term_id=%s&term_sig=%s&guid=%s&user_sig=%s&limit=%d&app_ver=%s",
			parm->ApiPrmMups.new_term_id,
			parm->ApiPrmMups.term_sig,
			parm->ApiPrmMups.guid,
			parm->ApiPrmMups.user_sig,
			limit,
			appVer
	);

	// トリップIDの指定がある場合
	if ((NULL != tripId) && (EOS != tripId[0])) {
		sprintf((char*)&body[strlen((char*)body)],
				"&offset=%s",
				tripId
		);
	}

}
