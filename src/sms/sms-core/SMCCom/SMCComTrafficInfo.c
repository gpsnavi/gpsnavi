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

#define		CC_TRAFFICINFO_DEBUG_MODE					0	// デバッグモード（0：通常モード、1：デバッグモード１、2：デバッグモード２）

// デバグモードでは、交通情報のAPIはコールしない
#ifdef	CC_TRAFFICINFO_DEBUG_MODE
#if (1 == CC_TRAFFICINFO_DEBUG_MODE)
#define	CC_TRAFFICINFO_DEBUG_MODE_1		// 交通情報APIはコールしないで、ローカルのレスポンスファイルをリードして処理する（zlib解凍も行う）
										// ※レスポンスファイルは、"…/Android/data/jp.co.hitachi.smsfv.aa/files/Data/Temp/trafficinfo.res"を用意しておくこと
										//   レスポンスファイルは、API仕様を基に作成すること
#endif
#if (2 == CC_TRAFFICINFO_DEBUG_MODE)
#define	CC_TRAFFICINFO_DEBUG_MODE_2		// 交通情報本体データファイルのパスのみ返却する（zlib解凍など処理は行わない）
#endif
										// ※交通情報本体データファイル（バイナリ）は、"…/Android/data/jp.co.hitachi.smsfv.aa/files/Data/Temp/trafficinfo.bin"
#endif

#define	CC_TRAFFICINFO_SEND_BODY_SIZE					1024

#define	CC_TRAFFICINFO_BUFF_SIZE						(1024 * 1024)
#define	CC_TRAFFICINFO_RES_FILE							"trafficinfo.res"
#define	CC_TRAFFICINFO_ZLIB_FILE						"trafficinfo.z"
#define	CC_TRAFFICINFO_BIN_FILE							"trafficinfo.bin"
#define	CC_TRAFFICINFO_FILE_HEAD_SIZE					(4 + 4 + 4)

// データサイズ
#define	CC_TRAFFICINFO_BIN_SIZE_ERROR_CD				4
#define	CC_TRAFFICINFO_BIN_SIZE_UNCOMPRESS_SIZE			4
#define	CC_TRAFFICINFO_BIN_SIZE_COMPRESS_SIZE			4

// オフセット
#define	CC_TRAFFICINFO_BIN_OFFSET_ERROR_CD				0
#define	CC_TRAFFICINFO_BIN_OFFSET_UNCOMPRESS_SIZE		(CC_TRAFFICINFO_BIN_OFFSET_ERROR_CD + CC_TRAFFICINFO_BIN_SIZE_ERROR_CD)
#define	CC_TRAFFICINFO_BIN_OFFSET_COMPRESS_SIZE			(CC_TRAFFICINFO_BIN_OFFSET_UNCOMPRESS_SIZE + CC_TRAFFICINFO_BIN_SIZE_UNCOMPRESS_SIZE)
#define	CC_TRAFFICINFO_BIN_OFFSET_DATA					(CC_TRAFFICINFO_BIN_OFFSET_COMPRESS_SIZE + CC_TRAFFICINFO_BIN_SIZE_COMPRESS_SIZE)

//------------------------------------------------
// 変数定義
//------------------------------------------------


//------------------------------------------------
// 関数定義
//------------------------------------------------
static void CC_TrafficInfo_CreateUri(const T_CC_CMN_SMS_API_PRM *param, Char *uri);
static void CC_TrafficInfo_CreateBody(const T_CC_CMN_SMS_API_PRM *parm, const SMTRAFFICSRCH *trafficSrch, Char *body);
static E_SC_RESULT CC_TrafficInfo_AnalyzeHttpResp(const Char *body, E_CONTEXT_TYPE contextType, SMTRAFFICINFO *trafficInfo, SMCALOPT *opt);

/**
 * @brief プローブ情報取得
 * @param [IN]  smcal           SMCAL
 * @param [IN]  parm            APIパラメータ
 * @param [OUT] trafficSrch     交通情報検索条件
 * @param [OUT] trafficInfo     交通情報検索結果
 * @param [IN]  recv            センタ受信データ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_TrafficInfo_SendRecv(SMCAL *smcal,
									const T_CC_CMN_SMS_API_PRM *parm,
									const SMTRAFFICSRCH *trafficSrch,
									SMTRAFFICINFO *trafficInfo,
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
		opt.isResOutputFile = true;
		opt.cancel = SCC_IsCancel;
		CC_GetTempDirPath(opt.resFilePath);
		strcat(opt.resFilePath, CC_TRAFFICINFO_RES_FILE);
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
		data = (Char*)SCC_MALLOC(CC_TRAFFICINFO_SEND_BODY_SIZE);
		if (NULL == data) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// URI生成
		CC_TrafficInfo_CreateUri(parm, uri);

		// body生成
		CC_TrafficInfo_CreateBody(parm, trafficSrch, data);

#if ((!defined(CC_TRAFFICINFO_DEBUG_MODE_1)) && (!defined(CC_TRAFFICINFO_DEBUG_MODE_2)))	// デバッグ時は通信しない
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
#endif																						// デバッグ時は通信しない

		// レスポンス解析
		ret = CC_TrafficInfo_AnalyzeHttpResp(body, contextType, trafficInfo, &opt);
		if(e_SC_RESULT_SUCCESS != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_TrafficInfo_AnalyzeHttpResp error, " HERE);
			break;
		}
	} while (0);

#ifndef CC_TRAFFICINFO_DEBUG_MODE_1
	// レスポンスファイル削除
	remove(opt.resFilePath);
#endif

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
void CC_TrafficInfo_CreateUri(const T_CC_CMN_SMS_API_PRM *parm, Char *uri)
{
	sprintf((char*)uri,
			"%straffic/req/",
			parm->ApiPrmNavi.sms_sp_uri
	);
}

/**
 * @brief body生成
 * @param [IN] parm         APIパラメータ
 * @param [IN] trafficSrch  交通情報検索条件
 * @param [OUT] body        body
 */
void CC_TrafficInfo_CreateBody(const T_CC_CMN_SMS_API_PRM *parm, const SMTRAFFICSRCH *trafficSrch, Char *body)
{
	INT32	num = 0;

	sprintf((char*)body,
			"term_id=%s&term_sig=%s&guid=%s&user_sig=%s&link_lv=%u&source_id=%d&rgn=%d&time=%u&jam=%d&sapa=%d&park=%d&reg=%d&app_ver=%s&map_version=%s",
			parm->ApiPrmMups.new_term_id,
			parm->ApiPrmMups.term_sig,
			parm->ApiPrmMups.guid,
			parm->ApiPrmMups.user_sig,
			trafficSrch->linkLv,
			trafficSrch->srcId,
			trafficSrch->rgn,
			trafficSrch->time,
			trafficSrch->jam,
			trafficSrch->sapa,
			trafficSrch->park,
			trafficSrch->reg,
			parm->ApiPrmNavi.appVer,
			trafficSrch->mapVer
	);

	// パーセルID
	sprintf((char*)&body[strlen((char*)body)],
			"&pid_request_num=%d",
			trafficSrch->parcelIdNum
	);
	for (num = 0; num < trafficSrch->parcelIdNum; num++) {
		if (0 == num) {
			sprintf((char*)&body[strlen((char*)body)],
					"&pid=%x",
					trafficSrch->parcelId[num]
			);
		} else {
			sprintf((char*)&body[strlen((char*)body)],
					",%x",
					trafficSrch->parcelId[num]
			);
		}
	}

	// 道路種別
	if (0 < trafficSrch->roadKindNum) {
		sprintf((char*)&body[strlen((char*)body)],
				"&road_kind_num=%d",
				trafficSrch->roadKindNum
		);
		for (num = 0; num < trafficSrch->roadKindNum; num++) {
			if (0 == num) {
				sprintf((char*)&body[strlen((char*)body)],
						"&road_kind=%d",
						trafficSrch->roadKind[num]
				);
			} else {
				sprintf((char*)&body[strlen((char*)body)],
						",%d",
						trafficSrch->roadKind[num]
				);
			}
		}
	} else {
		sprintf((char*)&body[strlen((char*)body)],
				"&road_kind_num=&road_kind="
		);
	}
}

/**
 * @brief レスポンス解析
 * @param [IN]  body            xmlデータ
 * @param [IN]  contextType     コンテキスト
 * @param [OUT] trafficInfo     交通情報
 * @param [IN]  opt             オプション情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_TrafficInfo_AnalyzeHttpResp(const Char *body,
										   E_CONTEXT_TYPE contextType,
										   SMTRAFFICINFO *trafficInfo,
										   SMCALOPT *opt)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	FILE	*apiResFile = NULL;
	FILE	*trafficFile = NULL;
	Char	*trafficFilePath = NULL;
	UChar	*buff = NULL;
	UChar	wk[4] = {};
	INT32	len = 0;
	INT32	len2 = 0;
	struct stat	st = {};
	INT32	errorCode = 0;
	UINT32	uncompressSize = 0;
	UINT32	compressSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
#ifndef CC_TRAFFICINFO_DEBUG_MODE_2
		// メモリ確保
		trafficFilePath = (Char*)SCC_MALLOC(SCC_MAX_PATH);
		if (NULL == trafficFilePath) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		*trafficFilePath = EOS;
		buff = (Char*)SCC_MALLOC(CC_TRAFFICINFO_BUFF_SIZE);
		if (NULL == buff) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		*buff = EOS;

		// ファイルサイズ取得
		if (0 != stat((char*)opt->resFilePath, &st)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "stat error(%d) [%s], " HERE, errno, opt->resFilePath);
			ret = e_SC_RESULT_FILE_ACCESSERR;
			break;
		}

		// 交通情報バイナリファイルオープン
		apiResFile = fopen(opt->resFilePath, "rb");
		if (NULL == apiResFile) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"fopen error(%d) [%s], " HERE, errno, opt->resFilePath);
			ret = e_SC_RESULT_FILE_OPENERR;
			break;
		}

		// 交通情報本体ファイルパス設定
		CC_GetTempDirPath(trafficFilePath);
		strcat(trafficFilePath, CC_TRAFFICINFO_ZLIB_FILE);
		// 交通情報本体ファイルオープン
		trafficFile = fopen(trafficFilePath, "wb");
		if (NULL == trafficFile) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"fopen error(%d) [%s], " HERE, errno, trafficFilePath);
			ret = e_SC_RESULT_FILE_OPENERR;
			break;
		}

		// 交通情報ヘッダをリード
		len = fread(buff, 1, CC_TRAFFICINFO_FILE_HEAD_SIZE, apiResFile);
		if (CC_TRAFFICINFO_FILE_HEAD_SIZE != len) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"fread error(%d) [%s], " HERE, errno, trafficFilePath);
			ret = e_SC_RESULT_FILE_ACCESSERR;
			break;
		}

		// リトルエンディアン環境の場合は、ビッグからリトルにエンディアン変換する
		// エラーコード
		memcpy(wk, &buff[CC_TRAFFICINFO_BIN_OFFSET_ERROR_CD], CC_TRAFFICINFO_BIN_SIZE_ERROR_CD);
		errorCode = CONVERT_BIG_ENDIAN_INT32(*(INT32*)wk);
		// 圧縮前データ長
		memcpy(wk, &buff[CC_TRAFFICINFO_BIN_OFFSET_UNCOMPRESS_SIZE], CC_TRAFFICINFO_BIN_SIZE_UNCOMPRESS_SIZE);
		uncompressSize = (UINT32)CONVERT_BIG_ENDIAN_INT32(*(INT32*)wk);
		// 圧縮後データ長
		memcpy(wk, &buff[CC_TRAFFICINFO_BIN_OFFSET_COMPRESS_SIZE], CC_TRAFFICINFO_BIN_SIZE_COMPRESS_SIZE);
		compressSize = (UINT32)CONVERT_BIG_ENDIAN_INT32(*(INT32*)wk);

		// エラーコードチェック
		if (0 != errorCode) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"trafficinfo api error(%d), " HERE, errorCode);
			ret = e_SC_RESULT_SMS_API_ERR;
			break;
		}
		// 圧縮後ファイルサイズチェック
		if ((st.st_size - CC_TRAFFICINFO_BIN_OFFSET_DATA) != compressSize) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"compresssize error, " HERE);
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"compresssize=%u, " HERE, (st.st_size - CC_TRAFFICINFO_BIN_OFFSET_DATA));
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"compresssize=%u, " HERE, compressSize);
			ret = e_SC_RESULT_SMS_API_ERR;
			break;
		}

		if ((0 < uncompressSize) && (0 < compressSize)) {
			// 交通情報のヘッダ部を取り除いて、データ本体だけを別ファイルに出力する
			// EOFまで繰り返す
			while (0 == feof(apiResFile)) {
				// ファイルリード
				len = fread(buff, 1, CC_TRAFFICINFO_BUFF_SIZE, apiResFile);
				if (0 >= len) {
					if (0 == feof(apiResFile)) {
						SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"fread error(%d) [%s], " HERE, errno, opt->resFilePath);
						ret = e_SC_RESULT_FILE_ACCESSERR;
					}
					break;
				}

				// ファイルライト
				len2 = fwrite(buff, 1, len, trafficFile);
				if (len != len2) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"fwrite error(%d), " HERE, errno);
					ret = e_SC_RESULT_FILE_ACCESSERR;
					break;
				}
			}
			if (e_SC_RESULT_SUCCESS != ret) {
				break;
			}

			// ファイルクローズ
			fclose(apiResFile);
			apiResFile = NULL;
			fclose(trafficFile);
			trafficFile = NULL;

			// 交通情報本体ファイルパス設定
			CC_GetTempDirPath(trafficInfo->filePath);
			strcat(trafficInfo->filePath, CC_TRAFFICINFO_BIN_FILE);
			// zlib解凍
			ret = SMZLB_ZlibDeCompress(trafficFilePath, trafficInfo->filePath);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SMZLB_ZlibDeCompress error, " HERE);
				break;
			}
			// ファイルサイズ取得
			if (0 != stat((char*)trafficInfo->filePath, &st)) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, "stat error(%d) [%s], " HERE, errno, trafficInfo->filePath);
				ret = e_SC_RESULT_FILE_ACCESSERR;
				break;
			}
			// 解凍後ファイルサイズチェック
			if (st.st_size != uncompressSize) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"uncompresssize error, " HERE);
				ret = e_SC_RESULT_SMS_API_ERR;
				break;
			}
		}
#else
		// デバッグモード CC_TRAFFICINFO_DEBUG_MODE_2 の場合の処理
		// 交通情報本体ファイルパス設定
		CC_GetTempDirPath(trafficInfo->filePath);
		strcat(trafficInfo->filePath, CC_TRAFFICINFO_BIN_FILE);
		// ファイルサイズ取得
		if (0 != stat((char*)trafficInfo->filePath, &st)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "stat error(%d) [%s], " HERE, errno, trafficInfo->filePath);
			ret = e_SC_RESULT_FILE_ACCESSERR;
			break;
		}
		uncompressSize = st.st_size;
#endif
		trafficInfo->fileSize = uncompressSize;
	} while (0);

	// ファイルクローズ
	if (NULL != apiResFile) {
		// 交通情報バイナリファイル
		fclose(apiResFile);
	}
	if (NULL != trafficFile) {
		// 交通情報本体ファイル
		fclose(trafficFile);
	}

	if (NULL != trafficFilePath) {
		if (EOS != *trafficFilePath) {
			// ファイル削除
			remove(trafficFilePath);
		}
		// メモリ解放
		SCC_FREE(trafficFilePath);
	}
	if (NULL != buff) {
		// メモリ解放
		SCC_FREE(buff);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}
