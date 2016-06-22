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

#if 1	// TR4限定
#define SCC_AWS_POLICY_DOC_SIZE		256
#endif	// TR4限定

//------------------------------------------------
// 関数定義
//------------------------------------------------
static time_t SCC_AWS_GetExpires();
static void SCC_AWS_GetExpiresStr(LONG expires, Char *gmt);
static void SCC_AWS_GetBacketNameVersionDirName(Char *backetName, Char *verDirName);

/**
 * @brief AWSアクセス情報取得
 * @param [IN] smcal        SMCAL
 * @param [IN] parm         APIパラメータ
 * @param [IN] backet       バケット種別
 * @param [IN] filePath     ファイルパス
 * @param [IN] aws          AWS情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GetAWSInfo(SMCAL *smcal,
						  T_CC_CMN_SMS_API_PRM *parm,
						  E_HTTP_METHOD method,
						  E_AWS_BUCKET backet,
						  const Char *filePath,
						  SMAWSINFO *aws)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	E_SC_CAL_RESULT	calRet = e_SC_CAL_RESULT_SUCCESS;
	Char	*recvBuf = NULL;
	Char	apiStatus[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	INT32	num = 0;
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
#if 1	// TR4限定
	UChar	*policy = NULL;
	//Char	signature[SC_CAL_SIGNATURE_SIZE] = {};
#endif	// TR4限定

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISNULL(smcal)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[smcal], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(parm)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[parm], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISINVALIDSTR(filePath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[filePath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(aws)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[aws], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// メモリ確保
		recvBuf = (Char*)SCC_MALLOC(CC_CMN_RECIVE_BUFFER_SIZE);
		if (NULL == recvBuf) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, parm->ApiPrmMups.user_sig);

		if (e_HTTP_METHOD_POST == method) {
			// POST
			if (e_AWS_BACKET_PROBE == backet) {
				for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
					// 初期化
					memset(recvBuf, 0, CC_CMN_RECIVE_BUFFER_SIZE);
					memset(aws, 0, sizeof(SMAWSINFO));

					// AWSアクセス情報取得
					ret = CC_ProbePostReq_SendRecv(smcal, parm, aws, recvBuf, CC_CMN_RECIVE_BUFFER_SIZE, apiStatus);
					if (e_SC_RESULT_SUCCESS != ret) {
						SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ProbePostReq_SendRecv error " HERE);
						SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s " HERE, apiStatus);
						if (0 == strcmp("SMSAPIE003", apiStatus)) {
							// tokenエラー
							apiStatus[0] = EOS;

							// 再認証(必要な場合のみ再認証する)
							ret2 = CC_ReAuthreq(smcal, userSig, parm, recvBuf, CC_CMN_RECIVE_BUFFER_SIZE, apiStatus, false);
							if (e_SC_RESULT_SUCCESS != ret2) {
								SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ReAuthreq error, " HERE);
								break;
							}
							continue;
						}
						break;
					}
					break;
				}
				if (e_SC_RESULT_SUCCESS != ret) {
					break;
				}
				// 有効期限
				aws->expires = (LONG)SCC_AWS_GetExpires();
				// ポリシー取得
				ret = SCC_AWS_GetPolicy(smcal, aws->backetName, filePath, aws->expires, &policy);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_AWS_GetPolicy error " HERE);
					break;
				}
				strncpy(aws->policy, policy, (sizeof(aws->policy) - 1));
			} else if (e_AWS_BACKET_DRIVE == backet) {
				for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
					// 初期化
					memset(recvBuf, 0, CC_CMN_RECIVE_BUFFER_SIZE);
					memset(aws, 0, sizeof(SMAWSINFO));

					// AWSアクセス情報取得
					ret = CC_SensorProbePostReq_SendRecv(smcal, parm, aws, recvBuf, CC_CMN_RECIVE_BUFFER_SIZE, apiStatus);
					if (e_SC_RESULT_SUCCESS != ret) {
						SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SensorProbePostReq_SendRecv error " HERE);
						SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s " HERE, apiStatus);
						if (0 == strcmp("SMSAPIE003", apiStatus)) {
							// tokenエラー
							apiStatus[0] = EOS;

							// 再認証(必要な場合のみ再認証する)
							ret2 = CC_ReAuthreq(smcal, userSig, parm, recvBuf, CC_CMN_RECIVE_BUFFER_SIZE, apiStatus, false);
							if (e_SC_RESULT_SUCCESS != ret2) {
								SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ReAuthreq error, " HERE);
								break;
							}
							continue;
						}
						break;
					}
					break;
				}
				if (e_SC_RESULT_SUCCESS != ret) {
					break;
				}
				// 有効期限
				aws->expires = (LONG)SCC_AWS_GetExpires();
				// ポリシー取得
				ret = SCC_AWS_GetPolicy(smcal, aws->backetName, filePath, aws->expires, &policy);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_AWS_GetPolicy error " HERE);
					break;
				}
				strncpy(aws->policy, policy, (sizeof(aws->policy) - 1));
			} else {
				// POST
				// 現状、未サポート
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"no support, " HERE);
				ret = e_SC_RESULT_BADPARAM;
				break;
			}
		} else {
			// GET
			if (e_AWS_BACKET_PROBE == backet) {
				// PROBE
				// 現状、未サポート
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"no support, " HERE);
				ret = e_SC_RESULT_BADPARAM;
				break;
			} else if (e_AWS_BACKET_DRIVE == backet) {
				// 運転特性診断
				// 現状、未サポート
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"no support, " HERE);
				ret = e_SC_RESULT_BADPARAM;
				break;
			} else {
				// MAP
				for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
					// 初期化
					memset(recvBuf, 0, CC_CMN_RECIVE_BUFFER_SIZE);
					memset(aws, 0, sizeof(SMAWSINFO));

					// AWSアクセス情報取得
					ret = CC_MapdatabktReq_SendRecv(smcal, parm, aws, recvBuf, CC_CMN_RECIVE_BUFFER_SIZE, apiStatus);
					if (e_SC_RESULT_SUCCESS != ret) {
						SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MapdatabktReq_SendRecv error " HERE);
						SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s " HERE, apiStatus);
						if (0 == strcmp("SMSAPIE003", apiStatus)) {
							// tokenエラー
							apiStatus[0] = EOS;

							// 再認証(必要な場合のみ再認証する)
							ret2 = CC_ReAuthreq(smcal, userSig, parm, recvBuf, CC_CMN_RECIVE_BUFFER_SIZE, apiStatus, false);
							if (e_SC_RESULT_SUCCESS != ret2) {
								SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ReAuthreq error, " HERE);
								break;
							}
							continue;
						}
						break;
					}
					// バケット名とバージョンディレクトリ名取得
					SCC_AWS_GetBacketNameVersionDirName(aws->backetName, aws->versionDirName);
					break;
				}
				if (e_SC_RESULT_SUCCESS != ret) {
					break;
				}
				// 有効期限
				aws->expires = (LONG)SCC_AWS_GetExpires();
			}
			// ポリシー生成
			if (EOS != aws->versionDirName[0]) {
				sprintf((char*)aws->policy, "GET\n\n\n%ld\n/%s/%s/%s", aws->expires, aws->backetName, aws->versionDirName, filePath);
			} else {
				sprintf((char*)aws->policy, "GET\n\n\n%ld\n/%s/%s", aws->expires, aws->backetName, filePath);
			}
		}

		// シグネチャ取得
		calRet = SC_CAL_GetAWSSignature(smcal, aws->secretKey, aws->policy, aws->signature);
		if (e_SC_CAL_RESULT_SUCCESS != calRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "SC_CAL_GetAWSSignature error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}
	} while (0);

	// メモリ解放
	if (NULL != recvBuf) {
		SCC_FREE(recvBuf);
	}
	if (NULL != policy) {
		free(policy);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief AWSバケット名取得
 * @param [IN]  smcal       SMCAL
 * @param [IN]  parm        APIパラメータ
 * @param [IN]  backet      バケット種別
 * @param [OUT] backetName  バケット名
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GetAWSBucketName(SMCAL *smcal,
								T_CC_CMN_SMS_API_PRM *parm,
								E_HTTP_METHOD method,
								E_AWS_BUCKET backet,
								Char *backetName)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	//E_SC_CAL_RESULT	calRet = e_SC_CAL_RESULT_SUCCESS;
	Char	*recvBuf = NULL;
	Char	apiStatus[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	INT32	num = 0;
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMAWSINFO	*aws = NULL;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISNULL(smcal)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[smcal], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(parm)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[parm], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(backetName)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[backetName], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// メモリ確保
		recvBuf = (Char*)SCC_MALLOC(CC_CMN_RECIVE_BUFFER_SIZE);
		if (NULL == recvBuf) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		aws = (SMAWSINFO*)SCC_MALLOC(sizeof(SMAWSINFO));
		if (NULL == aws) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// 初期化
		memset(recvBuf, 0, CC_CMN_RECIVE_BUFFER_SIZE);
		*backetName = EOS;

		// ユーザーアクセスキー退避
		strcpy(userSig, parm->ApiPrmMups.user_sig);

		if (e_HTTP_METHOD_POST == method) {
			// POST
			if (e_AWS_BACKET_PROBE == backet) {
				// 処理なし
			} else {
				// POST
				// 現状、未サポート
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"no support, " HERE);
				ret = e_SC_RESULT_BADPARAM;
				break;
			}
		} else {
			// GET
			if (e_AWS_BACKET_PROBE == backet) {
				// PROBE
				// 現状、未サポート
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"no support, " HERE);
				ret = e_SC_RESULT_BADPARAM;
				break;
			} else {
				// MAP
				for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
					// AWSアクセス情報取得
					ret = CC_MapdatabktReq_SendRecv(smcal, parm, aws, recvBuf, CC_CMN_RECIVE_BUFFER_SIZE, apiStatus);
					if (e_SC_RESULT_SUCCESS != ret) {
						SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MapdatabktReq_SendRecv error " HERE);
						SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s " HERE, apiStatus);
						if (0 == strcmp("SMSAPIE003", apiStatus)) {
							// tokenエラー
							apiStatus[0] = EOS;

							// 再認証(必要な場合のみ再認証する)
							ret2 = CC_ReAuthreq(smcal, userSig, parm, recvBuf, CC_CMN_RECIVE_BUFFER_SIZE, apiStatus, false);
							if (e_SC_RESULT_SUCCESS != ret2) {
								SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ReAuthreq error, " HERE);
								break;
							}
							continue;
						}
						break;
					}
					strcpy((char*)backetName, (char*)aws->backetName);
					break;
				}
			}
		}
	} while (0);

	// メモリ解放
	if (NULL != recvBuf) {
		SCC_FREE(recvBuf);
	}
	if (NULL != aws) {
		SCC_FREE(aws);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

#if 1	// TR4限定
/**
* @brief ポリシー取得
* @param[in]  smcal     SMCAL
* @param[in]  backet    バケット名
* @param[in]  filePath  AWSファイルパス
* @param[out] policy    ポリシー
* @return 処理結果(E_SC_RESULT)
*/
E_SC_RESULT SCC_AWS_GetPolicy(SMCAL *smcal,
							  const Char *backet,
							  const Char *filePath,
							  LONG expires,
							  UChar **policy)
{
	E_SC_RESULT	ret = e_SC_CAL_RESULT_SUCCESS;
	E_SC_CAL_RESULT	calRet = e_SC_CAL_RESULT_SUCCESS;
	Char	gmt[20] = {};
	Char	*policyDoc = NULL;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// メモリ確保
		policyDoc = (Char*)SCC_MALLOC(SCC_AWS_POLICY_DOC_SIZE + strlen(backet) + strlen(filePath));
		if (NULL == policyDoc) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// 有効期限取得
		SCC_AWS_GetExpiresStr(expires, gmt);

		sprintf((char*)policyDoc,
				"{\"expiration\": \"%sZ\", \"conditions\": [{\"bucket\": \"%s\"}, [\"starts-with\", \"$key\", \"%s\"], {\"acl\": \"private\"}, [\"starts-with\", \"$Content-Type\", \"\"]]}",
				gmt,
				backet,
				filePath);

		// BASE64エンコード
		calRet = SC_CAL_Base64Encode(smcal, policyDoc, strlen(policyDoc), policy);
		if (e_SC_CAL_RESULT_SUCCESS != calRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "SC_CAL_Base64Encode error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
	} while (0);

	// メモリ解放
	if (NULL != policyDoc) {
		SCC_FREE(policyDoc);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
* @brief 有効期限(世界標準UNIX時刻)取得処理
* @return 有効期限
*/
time_t SCC_AWS_GetExpires()
{
	time_t	nowTime = {0};
	time_t	expires = {0};

	// 現在時刻取得
	time(&nowTime);
	// 60分後の時刻取得
	expires = nowTime + (60 * 60);

	return (expires);
}

/**
* @brief 有効期限(世界標準時刻)取得処理
* @param[out] gmt  有効期限(世界標準時刻)
*/
void SCC_AWS_GetExpiresStr(LONG expires, Char *gmt)
{
	struct tm	gt = {0};

	// tm構造体の取得
	gmtime_r((time_t*)&expires, &gt);
	strftime((char*)gmt, 20, "%Y-%m-%dT%H:%M:%S", &gt);
}
#endif

/**
 * @brief バケット名とバージョンディレクトリ名取得
 * @param[in] mapBacketName     バケット名/バージョンディレクトリ名
 * @param[out] backetName       バケット名
 * @param[out] versionDirName   バージョンディレクトリ名
 */

void SCC_AWS_GetBacketNameVersionDirName(Char *backetName, Char *versionDirName)
{
	Char	*chr = NULL;

	// バケット名とバージョンディレクトリ名に分ける
	chr = strchr(backetName, '/');
	if (NULL != chr) {
		// バケット名
		*chr = EOS;
		chr++;
		// バージョンディレクトリ名
		strcpy(versionDirName, chr);
	}
}

// AWSアクセス情報取得
E_SC_RESULT SCC_GetAWSInfo(SMCAL *smcal,
						   E_HTTP_METHOD method,
						   E_AWS_BUCKET backet,
						   const Char *filePath,
						   SMAWSINFO *aws)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	// AWSアクセス情報取得
	ret = CC_GetAWSBucketInfo(smcal, method, backet, filePath, aws);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}
