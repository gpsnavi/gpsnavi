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

#define SCC_DL_URI_MAX_LEN					(2048 + 1)
#define SCC_DL_RECIVE_BUFFER_SIZE			(2048 + 1)
#define	SCC_DL_TIMEOUT						60
#define	SCC_DL_RETRY_NUM					3
#define	SCC_DL_PKG_URL_EXPIRES				(5 * 60)

static E_SC_RESULT CC_DownloadFile(SMCAL *smcal, T_CC_CMN_SMS_API_PRM *parm, const Char *awsFilePath, const Char *outFilePath, SMAWSINFO  *aws, const SMPROGRESSCBFNC *dlCBFnc, UINT32 resumeStartPos, UINT32 resumeEndPos);

/**
 * @brief データダウンロード(レジューム機能付き)
 * @param[in] smcal         センタアクセスライブラリ情報構造体
 * @param[in] awsFilePath   AWSからダウンロードするファイルパス
 * @param[in] outFilePath   ダウンロードしたデータの出力先ファイルのフルパス
 * @param[in] dlCBFnc       コールバック関数のポインタ
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_RESULT CC_Download(SMCAL *smcal,
						T_CC_CMN_SMS_API_PRM *parm,
						const Char *awsFilePath,
						const Char *outFilePath,
						const SMPROGRESSCBFNC *dlCBFnc,
						UINT32 resumeStartPos,
						UINT32 resumeEndPos)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_CAL_RESULT	calRet = e_SC_CAL_RESULT_SUCCESS;
	Char	*url = NULL;
	Char	*res = NULL;
	UINT32	resLen = 0;
	Char	*body = NULL;
	UINT32	bodyLen = 0;
	SMCALOPT	*opt = NULL;
	E_CONTEXT_TYPE	contextType = E_APP_BIN;
	Char	sigEncode[SC_CAL_SIGNATURE_SIZE * 2] = {};
	SMAWSINFO	aws = {};
	INT32	status = 0;
	INT32	num = 0;
	struct	stat st = {};
	//Char	apiSts[CC_CMN_XML_RES_STS_CODE_SIZE] ={};

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISINVALIDSTR(awsFilePath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param err[awsFilePath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISINVALIDSTR(outFilePath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param err[outDirPath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((0 != resumeStartPos) && (resumeStartPos >= resumeEndPos)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param err[resumeStartPos, resumeEndPos], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// メモリ確保
		url = (Char*)SCC_MALLOC(SCC_DL_URI_MAX_LEN);
		if (NULL == url) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "SCC_MALLOC err, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		res = (Char*)SCC_MALLOC(SCC_DL_RECIVE_BUFFER_SIZE);
		if (NULL == res) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "SCC_MALLOC err, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		opt = (SMCALOPT*)SCC_MALLOC(sizeof(SMCALOPT));
		if (NULL == opt) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "SCC_MALLOC err, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// 初期化
		memset(opt, 0, sizeof(SMCALOPT));
		opt->isAws = true;
		opt->isResOutputFile = true;
		opt->timeout = SCC_DL_TIMEOUT;
		opt->cancel = (SC_CAL_CANCEL_FNC)dlCBFnc->cancel;
		opt->progress = (SC_CAL_PROGRESS_FNC)dlCBFnc->progress;
		strcpy(opt->resFilePath, outFilePath);
		if ((0 < resumeStartPos) && (resumeStartPos < resumeEndPos)) {
			opt->isResume = true;
			opt->resumeStratPos = resumeStartPos;
			opt->resumeEndPos   = (resumeEndPos - 1);
		} else {
			// ダウンロードするファイルと同名のファイルは削除する
			remove(opt->resFilePath);
		}
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"isResume=%d, resumeStratPos=%u, resumeEndPos=%u, " HERE, opt->isResume, opt->resumeStratPos, opt->resumeEndPos);

		for (num = 0; num < SCC_DL_RETRY_NUM; num++) {
			// AWSアクセス情報取得
			ret = CC_GetAWSInfo(smcal, parm, e_HTTP_METHOD_GET, e_AWS_BACKET_MAP, awsFilePath, &aws);
			if(e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetAWSInfo error, " HERE);
				break;
			}

			// URLエンコード
			SC_CAL_UrlEncode(aws.signature, SC_CAL_SIGNATURE_SIZE, sigEncode);

			// URL生成
			if (EOS != aws.versionDirName[0]) {
				sprintf(url, "https://%s.s3.amazonaws.com/%s/%s?Signature=%s&Expires=%ld&AWSAccessKeyId=%s",
						aws.backetName, aws.versionDirName, awsFilePath, sigEncode, aws.expires, aws.accessKey);
			} else {
				sprintf(url, "https://%s.s3.amazonaws.com/%s?Signature=%s&Expires=%ld&AWSAccessKeyId=%s",
						aws.backetName, awsFilePath, sigEncode, aws.expires, aws.accessKey);
			}

			if ((0 < num) && (0 < resumeEndPos)) {
				// ファイルサイズ取得
				if (0 != stat((const char*)outFilePath, &st)) {
					opt->resumeStratPos = 0;
				} else {
					opt->resumeStratPos = (UINT32)st.st_size;
				}
				opt->resumeEndPos = resumeEndPos;
				if (opt->resumeStratPos < opt->resumeEndPos) {
					opt->isResume = true;
				}
			}
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"isResume=%d, resumeStratPos=%u, resumeEndPos=%u, " HERE, opt->isResume, opt->resumeStratPos, opt->resumeEndPos);

			// ダウンロード
			calRet = SC_CAL_GetRequest(smcal, url, res, SCC_DL_RECIVE_BUFFER_SIZE, &resLen, opt);
			if (e_SC_CAL_RESULT_CANCEL == calRet) {
				// キャンセル
				SCC_LOG_ErrorPrint(SC_TAG_CC, "SC_CAL_GetRequest cancel, " HERE);
				ret = ConvertResult(calRet);
				break;
			} else if (e_SC_CAL_RESULT_SUCCESS != ret) {
				// エラーの場合は、リトライする
				SCC_LOG_ErrorPrint(SC_TAG_CC, "SC_CAL_GetRequest error, " HERE);
				ret = ConvertResult(calRet);
				continue;
			}

			// HTTPデータ解析
			calRet = SC_CAL_AnalyzeResponseStatus(smcal, res, resLen, (const Char**)&body, &bodyLen, &contextType, &status);
			if (e_SC_CAL_RESULT_SUCCESS == calRet) {
				break;
			} else if (CC_CMN_SERVER_STOP == status) {
				// サーバ停止中
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"server stop..., " HERE);
				ret = e_SC_RESULT_SERVER_STOP;
				break;
			} else if (CC_CMN_NOT_FOUND == status) {
				// NOT FOUND
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not found..., " HERE);
				ret = e_SC_RESULT_TCP_COMMUNICATION_ERR;
				break;
			} else {
				// エラーの場合は、リトライする
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SC_CAL_AnalyzeResponseStatus error[status=%d], " HERE, status);
				ret = ConvertResult(calRet);
				opt->isResume = false;
				opt->resumeStratPos = 0;
				continue;
			}
		}
		if ((e_SC_CAL_RESULT_CANCEL != calRet) && (e_SC_RESULT_SUCCESS != ret)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "SC_CAL_GetRequest retry over, " HERE);
			break;
		}
	} while (0);

	if (NULL != url) {
		SCC_FREE(url);
	}
	if (NULL != res) {
		SCC_FREE(res);
	}
	if (NULL != opt) {
		SCC_FREE(opt);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief パッケージファイルダウンロード
 * @param [in] smcal        SMCAL
 * @param [in] parm         APIパラメータ
 * @param [in] dlDataType   ダウンロードデータタイプ
 * @param [in] pkgInfo      パッケージ一覧
 * @param [in] dlDirPath    ダウンロードしたファイルの格納ディレクトリのフルパス(パスの終端に'/'を付けること)
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_DownloadPackage(SMCAL *smcal,
							   T_CC_CMN_SMS_API_PRM *parm,
							   SMPKGDLDATATYPE dlDataType,
							   const SMPACKAGEINFO *pkgInfo,
							   const Char *dlDirPath)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	//E_SC_CAL_RESULT	calRet = e_SC_CAL_RESULT_SUCCESS;
	//INT32	num = 0;
	Char	*fileName = NULL;
	Char	*dlFilePath = NULL;
	SMAWSINFO	*aws = NULL;
	SMPROGRESSCBFNC	callbackFnc = {};
	time_t	nowTime = {0};
	//time_t	expires = {0};
	Char	*chr = NULL;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	// コールバック関数
	callbackFnc.cancel   = NULL;
	callbackFnc.progress = NULL;

	do {
		// メモリ確保
		aws = (SMAWSINFO*)SCC_MALLOC(sizeof(SMAWSINFO));
		if (NULL == aws) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		fileName = (Char*)SCC_MALLOC(CC_CMN_PKGMGR_PCKAGE);
		if (NULL == fileName) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		dlFilePath = (Char*)SCC_MALLOC(SCC_MAX_PATH);
		if (NULL == dlFilePath) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// S3にアクセスするために必要な情報設定
		strcpy((char*)aws->backetName, (char*)pkgInfo->backetName);		// バケット名
		chr = (Char*)strchr((char*)aws->backetName, '/');
		if (NULL == chr) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"backet name error[%s], " HERE, aws->backetName);
			ret = e_SC_RESULT_FAIL;
			break;
		}
		*chr = EOS;
		chr++;
		strcpy((char*)aws->accessKey, (char*)pkgInfo->accessKey);		// アクセスキー
		strcpy((char*)aws->secretKey, (char*)pkgInfo->secretKey);		// シークレットキー

		if (PKGDLDATATYPE_ICON == dlDataType) {
			// アイコン
			if (0 != strlen(chr)) {
				if ('/' == chr[strlen((char*)chr) - 1]) {
					sprintf((char*)fileName, "%s%s", chr, pkgInfo->iconFileName);
				} else {
					sprintf((char*)fileName, "%s/%s", chr, pkgInfo->iconFileName);
				}
			} else {
				sprintf((char*)fileName, "%s%s", SCC_CMN_AWS_DIR_PACKAGE, pkgInfo->iconFileName);
			}
			sprintf((char*)dlFilePath, "%s%s", dlDirPath, pkgInfo->iconFileName);
		} else {
			// パッケージ
			if (0 != strlen(chr)) {
				if ('/' == chr[strlen((char*)chr) - 1]) {
					sprintf((char*)fileName, "%s%s", chr, pkgInfo->packageFileName);
				} else {
					sprintf((char*)fileName, "%s/%s", chr, pkgInfo->packageFileName);
				}
			} else {
				sprintf((char*)fileName, "%s%s", SCC_CMN_AWS_DIR_PACKAGE, pkgInfo->packageFileName);
			}
			sprintf((char*)dlFilePath, "%s%s", dlDirPath, pkgInfo->packageFileName);
		}

		// 現在時刻取得
		time(&nowTime);
		aws->expires = nowTime + SCC_DL_PKG_URL_EXPIRES;				// 有効期限[秒]

		// ファイルダウンロード
		ret = CC_DownloadFile(smcal, parm, fileName, dlFilePath, aws, &callbackFnc, 0, 0);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_Download error[%s], " HERE, dlFilePath);
			break;
		}
	} while (0);


	// メモリ解放
	if (NULL != fileName) {
		SCC_FREE(fileName);
	}
	if (NULL != aws) {
		SCC_FREE(aws);
	}
	if (NULL != dlFilePath) {
		SCC_FREE(dlFilePath);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief AWS S3に配置したファイルダウンロード
 * @param[in] smcal             センタアクセスライブラリ情報構造体
 * @param[in] awsFilePath       AWSからダウンロードするファイルパス
 * @param[in] outFilePath       ダウンロードしたデータの出力先ファイルのフルパス
 *
 * @param[in] dlCBFnc           コールバック関数のポインタ
 * @param[in] resumeStartPos    コールバック関数のポインタ
 * @param[in] resumeEndPos      コールバック関数のポインタ
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_RESULT CC_DownloadFile(SMCAL *smcal,
							T_CC_CMN_SMS_API_PRM *parm,
							const Char *awsFilePath,
							const Char *outFilePath,
							SMAWSINFO  *aws,
							const SMPROGRESSCBFNC *dlCBFnc,
							UINT32 resumeStartPos,
							UINT32 resumeEndPos)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_CAL_RESULT	calRet = e_SC_CAL_RESULT_SUCCESS;
	Char	*url = NULL;
	Char	*res = NULL;
	UINT32	resLen = 0;
	Char	*body = NULL;
	UINT32	bodyLen = 0;
	SMCALOPT	*opt = NULL;
	E_CONTEXT_TYPE	contextType = E_APP_BIN;
	//UChar	*policy = NULL;
	Char	sigEncode[SC_CAL_SIGNATURE_SIZE * 2] = {};
	INT32	status = 0;
	INT32	num = 0;
	struct	stat st = {};
	//Char	apiSts[CC_CMN_XML_RES_STS_CODE_SIZE] ={};

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISINVALIDSTR(awsFilePath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param err[awsFilePath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISINVALIDSTR(outFilePath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param err[outDirPath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(aws)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param err[aws], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((0 != resumeStartPos) && (resumeStartPos >= resumeEndPos)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param err[resumeStartPos, resumeEndPos], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// メモリ確保
		url = (Char*)SCC_MALLOC(SCC_DL_URI_MAX_LEN);
		if (NULL == url) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "SCC_MALLOC err, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		res = (Char*)SCC_MALLOC(SCC_DL_RECIVE_BUFFER_SIZE);
		if (NULL == res) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "SCC_MALLOC err, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		opt = (SMCALOPT*)SCC_MALLOC(sizeof(SMCALOPT));
		if (NULL == opt) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "SCC_MALLOC err, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// 初期化
		memset(opt, 0, sizeof(SMCALOPT));
		opt->isAws = true;
		opt->isResOutputFile = true;
		opt->timeout = SCC_DL_TIMEOUT;
		opt->cancel = (SC_CAL_CANCEL_FNC)dlCBFnc->cancel;
		opt->progress = (SC_CAL_PROGRESS_FNC)dlCBFnc->progress;
		strcpy(opt->resFilePath, outFilePath);
		if ((0 < resumeStartPos) && (resumeStartPos < resumeEndPos)) {
			opt->isResume = true;
			opt->resumeStratPos = resumeStartPos;
			opt->resumeEndPos   = (resumeEndPos - 1);
		} else {
			// ダウンロードするファイルと同名のファイルは削除する
			remove(opt->resFilePath);
		}
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"isResume=%d, resumeStratPos=%u, resumeEndPos=%u, " HERE, opt->isResume, opt->resumeStratPos, opt->resumeEndPos);

		for (num = 0; num < SCC_DL_RETRY_NUM; num++) {
			// ポリシー生成
			sprintf((char*)aws->policy, "GET\n\n\n%ld\n/%s/%s", aws->expires, aws->backetName, awsFilePath);

			// シグネチャ取得
			calRet = SC_CAL_GetAWSSignature(smcal, aws->secretKey, aws->policy, aws->signature);
			if (e_SC_CAL_RESULT_SUCCESS != calRet) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, "SC_CAL_GetAWSSignature error, " HERE);
				ret = e_SC_RESULT_FAIL;
				break;
			}

			// URLエンコード
			SC_CAL_UrlEncode(aws->signature, SC_CAL_SIGNATURE_SIZE, sigEncode);

			// URL生成
			sprintf(url, "https://%s.s3.amazonaws.com/%s?Signature=%s&Expires=%ld&AWSAccessKeyId=%s",
					aws->backetName, awsFilePath, sigEncode, aws->expires, aws->accessKey);

			if ((0 < num) && (0 < resumeEndPos)) {
				// ファイルサイズ取得
				if (0 != stat((const char*)outFilePath, &st)) {
					opt->resumeStratPos = 0;
				} else {
					opt->resumeStratPos = (UINT32)st.st_size;
				}
				opt->resumeEndPos = resumeEndPos;
				if (opt->resumeStratPos < opt->resumeEndPos) {
					opt->isResume = true;
				}
			}
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"isResume=%d, resumeStratPos=%u, resumeEndPos=%u, " HERE, opt->isResume, opt->resumeStratPos, opt->resumeEndPos);

			// ダウンロード
			calRet = SC_CAL_GetRequest(smcal, url, res, SCC_DL_RECIVE_BUFFER_SIZE, &resLen, opt);
			if (e_SC_CAL_RESULT_CANCEL == calRet) {
				// キャンセル
				SCC_LOG_ErrorPrint(SC_TAG_CC, "SC_CAL_GetRequest cancel, " HERE);
				ret = ConvertResult(calRet);
				break;
			} else if (e_SC_CAL_RESULT_SUCCESS != ret) {
				// エラーの場合は、リトライする
				SCC_LOG_ErrorPrint(SC_TAG_CC, "SC_CAL_GetRequest error, " HERE);
				ret = ConvertResult(calRet);
				continue;
			}

			// HTTPデータ解析
			calRet = SC_CAL_AnalyzeResponseStatus(smcal, res, resLen, (const Char**)&body, &bodyLen, &contextType, &status);
			if (e_SC_CAL_RESULT_SUCCESS == calRet) {
				break;
			} else if (CC_CMN_SERVER_STOP == status) {
				// サーバ停止中
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"server stop..., " HERE);
				ret = e_SC_RESULT_SERVER_STOP;
				break;
			} else if (CC_CMN_NOT_FOUND == status) {
				// NOT FOUND
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not found..., " HERE);
				ret = e_SC_RESULT_TCP_COMMUNICATION_ERR;
				break;
			} else {
				// エラーの場合は、リトライする
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SC_CAL_AnalyzeResponseStatus error[status=%d], " HERE, status);
				ret = ConvertResult(calRet);
				opt->isResume = false;
				opt->resumeStratPos = 0;
				continue;
			}
		}
		if ((e_SC_CAL_RESULT_CANCEL != calRet) && (e_SC_RESULT_SUCCESS != ret)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "SC_CAL_GetRequest retry over, " HERE);
			break;
		}
	} while (0);

	if (NULL != url) {
		SCC_FREE(url);
	}
	if (NULL != res) {
		SCC_FREE(res);
	}
	if (NULL != opt) {
		SCC_FREE(opt);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}
