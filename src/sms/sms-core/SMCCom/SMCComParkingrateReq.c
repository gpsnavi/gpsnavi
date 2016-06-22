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

#define CC_PARKINGSEND_BODY_SIZE	1024

#define	CC_PARKINGXML_NODE_STORE_ID				"store_id"
#define	CC_PARKINGXML_NODE_STORE_NAME			"store_name"
#define	CC_PARKINGXML_NODE_PARKING_CNT			"parking_cnt"
#define	CC_PARKINGXML_NODE_PARKING_LIST			"parking_list"
#define	CC_PARKINGXML_NODE_ITEM					"item"
#define	CC_PARKINGXML_NODE_PARKING_ID			"parking_id"
#define	CC_PARKINGXML_NODE_PARKING_NAME			"parking_name"
#define	CC_PARKINGXML_NODE_ENTRANCE_NAME		"entrance_name"
#define	CC_PARKINGXML_NODE_ENTRANCE_LAT			"entrance_lat"
#define	CC_PARKINGXML_NODE_ENTRANCE_LON			"entrance_lon"
#define	CC_PARKINGXML_NODE_EMPTY_RATE			"empty_rate"
#define	CC_PARKINGXML_NODE_EMPTY_RATE_GETDATETM	"empty_rate_getdatetm"
#define	CC_PARKINGXML_NODE_PRIORITY				"priority"

#define	CC_PARKING_RES_FILE						"parkingentrance.res"

#define	CC_PARKINGXML_DATA_APISTATUS_SIZE		CC_CMN_XML_RES_STS_CODE_SIZE
#define	CC_PARKINGXML_DATA_STORE_ID_SIZE		11
#define	CC_PARKINGXML_DATA_STORE_NAME_SIZE		CC_CMN_PARKING_STORENAME
#define	CC_PARKINGXML_DATA_PARKING_CNT_SIZE		11
#define	CC_PARKINGXML_DATA_PARKING_ID_SIZE		11
#define	CC_PARKINGXML_DATA_PARKING_NAME_SIZE	CC_CMN_PARKING_PARKINGNAME
#define	CC_PARKINGXML_DATA_ENTRANCE_NAME_SIZE	CC_CMN_PARKING_ENTRANCENAME
#define	CC_PARKINGXML_DATA_ENTRANCE_LAT_SIZE	20
#define	CC_PARKINGXML_DATA_ENTRANCE_LON_SIZE	20
#define	CC_PARKINGXML_DATA_EMPTY_RATE_SIZE		3
#define	CC_PARKINGXML_DATA_EMPTY_RATE_GETDATETM_SIZE	11
#define	CC_PARKINGXML_DATA_PRIORITY_SIZE		11

// 駐車場情報取得レスポンスXML情報
typedef struct _PARKINGINFO {
} PARKINGINFO;

// 駐車場情報取得XMLパーサ
typedef struct _PARKINGPARSER {
	INT32			state;
	Char			*buf;
	SMSTOREPARKINGINFO	*parkingInfo;
	Char				*apiStatus;
} PARKINGPARSER;

// user/srch/
enum RoomSrchStatus {
	CC_PARKINGNODE_NONE = 0,
	CC_PARKINGNODE_XML,
	CC_PARKINGNODE_XML_CHILD,
	CC_PARKINGNODE_API_STATUS,
	CC_PARKINGNODE_STORE_ID,
	CC_PARKINGNODE_STORE_NAME,
	CC_PARKINGNODE_PARKING_CNT,
	CC_PARKINGNODE_PARKING_LIST,
	CC_PARKINGNODE_PARKING_LIST_CHILD,
	CC_PARKINGNODE_ITEM,
	CC_PARKINGNODE_ITEM_CHILD,
	CC_PARKINGNODE_PARKING_ID,
	CC_PARKINGNODE_PARKING_NAME,
	CC_PARKINGNODE_ENTRANCE_NAME,
	CC_PARKINGNODE_ENTRANCE_LAT,
	CC_PARKINGNODE_ENTRANCE_LON,
	CC_PARKINGNODE_EMPTY_RATE,
	CC_PARKINGNODE_EMPTY_RATE_GETDATETM,
	CC_PARKINGNODE_PRIORITY
};


//------------------------------------------------
// 変数定義
//------------------------------------------------
static INT32	CB_Result;				// 処理結果 (コールバック関数用)
static Bool		CB_IsXmlFile;

//------------------------------------------------
// 関数定義
//------------------------------------------------
static void CC_ParkingrateReq_CreateUri(const T_CC_CMN_SMS_API_PRM* param, Char* uri);	//URI生成
static void CC_ParkingrateReq_CreateBody(const T_CC_CMN_SMS_API_PRM* param, INT32 companyId, INT32 storeId, Char* body);	//body生成
static E_SC_RESULT CC_ParkingrateReq_AnalyzeHttpResp(const Char *body, E_CONTEXT_TYPE contextType, SMSTOREPARKINGINFO *parkingInfo, const SMCALOPT *opt, Char *apiStatus);
static E_SC_RESULT CC_ParkingrateReq_XmlParse(const Char* xml, T_CC_CMN_SMS_RESPONSE_INFO* resp_inf, SMSTOREPARKINGINFO *parkingInfo, const SMCALOPT *opt);
static void XMLCALL CC_ParkingrateReq_StartElement(void *userData, const char *name, const char **atts);
static void XMLCALL CC_ParkingrateReq_EndElement(void *userData, const char *name);
static void XMLCALL CC_ParkingrateReq_CharacterData(void *userData, const XML_Char *data, INT32 len);

/**
 * @brief 駐車場情報取得
 * @param [in]  smcal       SMCAL
 * @param [in]  parm        APIパラメータ
 * @param [in]  companyId   企業ID
 * @param [in]  storeId     店舗ID
 * @param [out] parkingInfo 駐車場情報
 * @param [in]  recv        センタ受信データ格納バッファ
 * @param [in]  recvBufSize センタ受信データ格納バッファサイズ
 * @param [out] apiStatus   APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_ParkingrateReq_SendRecv(SMCAL *smcal,
									   const T_CC_CMN_SMS_API_PRM *parm,
									   INT32 companyId,
									   INT32 storeId,
									   SMSTOREPARKINGINFO *parkingInfo,
									   Char *recv,
									   UINT32 recvBufSize,
									   Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_CAL_RESULT	calRet = e_SC_CAL_RESULT_SUCCESS;
	static Char	uri[CC_CMN_URI_STR_MAX_LEN] = {};
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
		memset(&opt, 0, sizeof(SMCALOPT));
		opt.isResOutputFile = true;
		CC_GetTempDirPath(opt.resFilePath);
		strcat(opt.resFilePath, CC_PARKING_RES_FILE);
		opt.cancel = SCC_IsCancel;
#ifdef SC_CMN_BASIC_AUTH_SMS
		// BASIC認証
		opt.isBasicAuth = true;
		strcpy(opt.basic.basicAuthId, SC_CMN_BASIC_AUTH_ID);
		strcpy(opt.basic.basicAuthPwd, SC_CMN_BASIC_AUTH_PWD);
#endif
		*apiStatus = EOS;

		// メモリ確保
		data = (Char*)SCC_MALLOC(CC_PARKINGSEND_BODY_SIZE);
		if (NULL == data) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC() error " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// URI生成
		CC_ParkingrateReq_CreateUri(parm, uri);

		// body生成
		CC_ParkingrateReq_CreateBody(parm, companyId, storeId, data);

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

		// レスポンス解析
		ret = CC_ParkingrateReq_AnalyzeHttpResp(body, contextType, parkingInfo, &opt, apiStatus);
		if(e_SC_RESULT_SUCCESS != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ParkingrateReq_AnalyzeHttpResp error, " HERE);
			break;
		}
	} while (0);

	// XMLファイル削除
	remove(opt.resFilePath);

	if (NULL != data) {
		SCC_FREE(data);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//URI生成
//************************************************
/**
 * @brief parkingentrance/req用URI生成
 * @param[in]  param    SMS API関連パラメタテーブルポインタ
 * @param[out] uri      生成URI出力先
*/
void CC_ParkingrateReq_CreateUri(const T_CC_CMN_SMS_API_PRM *param, Char *uri)
{
	sprintf((char*)uri,
			"%sparkingrate/req/",
			param->ApiPrmNavi.sms_sp_uri);
}

//************************************************
//body生成
//************************************************
/**
 * @brief parkingentrance/req用body生成
 * @param[in] param     SMS API関連パラメタテーブルポインタ
 * @param[in] companyId 企業ID
 * @param[in] storeId   店舗ID
 * @param[out] body     生成body出力先
*/
void CC_ParkingrateReq_CreateBody(const T_CC_CMN_SMS_API_PRM* param,
								  INT32 companyId,
								  INT32 storeId,
								  Char* body)
{
	sprintf((char*)body,
			"term_id=%s&term_sig=%s&guid=%s&user_sig=%s&company_id=%d&store_id=%d&app_ver=%s",
			param->ApiPrmMups.new_term_id,		// 端末ID (センター採番新規端末ID)
			param->ApiPrmMups.term_sig,			// 端末アクセスキー
			param->ApiPrmMups.guid,				// ユーザID
			param->ApiPrmMups.user_sig,			// ユーザアクセスキー
			companyId,							// 企業ID
			storeId,							// 店舗ID
			param->ApiPrmNavi.appVer			// アプリバージョン
	);
}

/**
 * @brief レスポンス解析
 * @param [in] body         xmlデータ
 * @param [in] contextType  コンテキスト
 * @param [out] parkingInfo 駐車場情報
 * @param [in] opt          オプション情報
 * @param [out] apiStatus   APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_ParkingrateReq_AnalyzeHttpResp(const Char *body,
											  E_CONTEXT_TYPE contextType,
											  SMSTOREPARKINGINFO *parkingInfo,
											  const SMCALOPT *opt,
											  Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	T_CC_CMN_SMS_RESPONSE_INFO	rsp_inf;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		if (E_TEXT_XML != contextType) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"Content-Type error, " HERE);
			ret = e_SC_RESULT_SMS_API_ERR;
			break;
		}

		// XML解析
		rsp_inf.apiSts = apiStatus;
		ret = CC_ParkingrateReq_XmlParse((const Char*)body, &rsp_inf, parkingInfo, opt);
		if(e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ParkingrateReq_XmlParse error, " HERE);
			return (e_SC_RESULT_SMS_API_ERR);
		}

		if ((!CHECK_API_STATUS(rsp_inf.apiSts)) && (!CHECK_API_STATUS2(rsp_inf.apiSts))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"api status error, " HERE);
			return (e_SC_RESULT_SMS_API_ERR);
		}
	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief parkingentrance/req応答メッセージ解析
 * @param [in]  xml         XMLファイルのフルパス
 * @param [in]  resp_inf    レスポンス情報
 * @param [out] parkingInfo ユーザ情報格納領域
 * @param [in] opt          オプション情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_ParkingrateReq_XmlParse(const Char* xml,
									   T_CC_CMN_SMS_RESPONSE_INFO* resp_inf,
									   SMSTOREPARKINGINFO* parkingInfo,
									   const SMCALOPT *opt)
{
	E_SC_RESULT		ret = e_SC_RESULT_SUCCESS;
	PARKINGPARSER	parkingParser = {};
	Char buf[CC_CMN_XML_PARSE_DATA_SIZE + 1] = {};
	XML_Parser parser = NULL;
	INT32	done = 0;
	INT32	len = 0;
	INT32	parsedLen = 0;
	FILE	*fp = NULL;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if ((NULL == opt) || (true != opt->isResOutputFile)) {
			if (NULL == xml) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[xml], " HERE);
				CB_Result = e_SC_RESULT_FAIL;
				ret = CB_Result;
				break;
			}
		}

		// 初期化
		resp_inf->sts = 0;
		memset(parkingInfo, 0, (sizeof(SMSTOREPARKINGINFO)));
		parkingParser.buf = (Char*)SCC_MALLOC(CC_CMN_XML_BUF_SIZE + 1);
		if (NULL == parkingParser.buf) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			CB_Result = e_SC_RESULT_MALLOC_ERR;
			ret = CB_Result;
			break;
		}
		parkingParser.parkingInfo = parkingInfo;
		parkingParser.apiStatus = &resp_inf->apiSts[0];
		CB_Result = e_SC_RESULT_SUCCESS;

		// XMLパーサ生成
		parser = XML_ParserCreate(NULL);
		if (NULL == parser) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"XML_ParserCreate error, " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			ret = CB_Result;
			break;
		}

		// コールバック関数設定
		XML_SetUserData(parser, &parkingParser);
		XML_SetElementHandler(parser, CC_ParkingrateReq_StartElement, CC_ParkingrateReq_EndElement);
		XML_SetCharacterDataHandler(parser, CC_ParkingrateReq_CharacterData);

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
			if (!CB_IsXmlFile) {
				strncpy((char*)buf, &xml[parsedLen], (sizeof(buf) - 1));
				len = (INT32)strlen(buf);
				parsedLen += len;
				if (strlen(xml) <= parsedLen) {
					done = 1;
				} else {
					done = 0;
				}
			} else {
				len = (INT32)fread(buf, 1, (sizeof(buf) - 1), fp);
				done = (len < (sizeof(buf) - 1));
			}

			// XML解析
			if ((XML_STATUS_ERROR == XML_Parse(parser, (const char*)buf, len, done)) || (e_SC_RESULT_SUCCESS != CB_Result)) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"XML_Parse error(0x%08x), " HERE, CB_Result);
				CB_Result = e_SC_RESULT_SMS_API_ERR;
				ret = CB_Result;
				break;
			}

			if (!done) {
				// バッファクリア
				memset(buf, 0, (sizeof(buf) - 1));
			}
		}
	} while (0);

	if (NULL != parkingParser.buf) {
		SCC_FREE(parkingParser.buf);
	}
	if (NULL != fp) {
		fclose(fp);
	}

	if (NULL != parser) {
		XML_ParserFree(parser);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief タグ解析開始
 * @param [in/out] userData ユーザデータ
 * @param [in] name     タグ名
 * @param [out] atts    属性(未使用)
 */
void XMLCALL CC_ParkingrateReq_StartElement(void *userData, const char *name, const char **atts)
{
	PARKINGPARSER *parser = (PARKINGPARSER*)userData;

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
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
			parser->state = CC_PARKINGNODE_XML;
		}

		// <xml>
		if (CC_PARKINGNODE_XML == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_API_STATUS)) {
				// <api_status>
				parser->state = CC_PARKINGNODE_API_STATUS;
			} else if (0 == strcmp((char*)name, (char*)CC_PARKINGXML_NODE_STORE_ID)) {
				// <store_id>
				parser->state = CC_PARKINGNODE_STORE_ID;
			} else if (0 == strcmp((char*)name, (char*)CC_PARKINGXML_NODE_STORE_NAME)) {
				// <store_name>
				parser->state = CC_PARKINGNODE_STORE_NAME;
			} else if (0 == strcmp((char*)name, (char*)CC_PARKINGXML_NODE_PARKING_CNT)) {
				// <parking_cnt>
				parser->state = CC_PARKINGNODE_PARKING_CNT;
			} else if (0 == strcmp((char*)name, (char*)CC_PARKINGXML_NODE_PARKING_LIST)) {
				// <parking_list>
				parser->state = CC_PARKINGNODE_PARKING_LIST;
			} else if (0 != strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_XML)) {
				parser->state = CC_PARKINGNODE_XML_CHILD;
			}
		} else if (CC_PARKINGNODE_PARKING_LIST == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_PARKINGXML_NODE_ITEM)) {
				// <item>
				if (CC_PARKINGINFO_MAXNUM <= parser->parkingInfo->parkingNum) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"item maxnum over, " HERE);
					CB_Result = e_SC_RESULT_SMS_API_ERR;
					break;
				}
				parser->state = CC_PARKINGNODE_ITEM;
			} else {
				parser->state = CC_PARKINGNODE_PARKING_LIST_CHILD;
			}
		} else if (CC_PARKINGNODE_ITEM == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_PARKINGXML_NODE_PARKING_ID)) {
				// <parking_id>
				parser->state = CC_PARKINGNODE_PARKING_ID;
			} else if (0 == strcmp((char*)name, (char*)CC_PARKINGXML_NODE_PARKING_NAME)) {
				// <parking_name>
				parser->state = CC_PARKINGNODE_PARKING_NAME;
			} else if (0 == strcmp((char*)name, (char*)CC_PARKINGXML_NODE_ENTRANCE_NAME)) {
				// <entrance_name>
				parser->state = CC_PARKINGNODE_ENTRANCE_NAME;
			} else if (0 == strcmp((char*)name, (char*)CC_PARKINGXML_NODE_ENTRANCE_LAT)) {
				// <entrance_lat>
				parser->state = CC_PARKINGNODE_ENTRANCE_LAT;
			} else if (0 == strcmp((char*)name, (char*)CC_PARKINGXML_NODE_ENTRANCE_LON)) {
				// <entrance_lon>
				parser->state = CC_PARKINGNODE_ENTRANCE_LON;
			} else if (0 == strcmp((char*)name, (char*)CC_PARKINGXML_NODE_EMPTY_RATE)) {
				// <empty_rate>
				parser->state = CC_PARKINGNODE_EMPTY_RATE;
			} else if (0 == strcmp((char*)name, (char*)CC_PARKINGXML_NODE_EMPTY_RATE_GETDATETM)) {
				// <empty_rate_getdatetm>
				parser->state = CC_PARKINGNODE_EMPTY_RATE_GETDATETM;
			} else if (0 == strcmp((char*)name, (char*)CC_PARKINGXML_NODE_PRIORITY)) {
				// <priority>
				parser->state = CC_PARKINGNODE_PRIORITY;
			} else {
				parser->state = CC_PARKINGNODE_ITEM_CHILD;
			}
		} else {
			// 上記以外
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"format error, " HERE);
			CB_Result = e_SC_RESULT_SMS_API_ERR;
			break;
		}
	} while (0);

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}

/**
 * @brief タグ解析終了
 * @param [in/out] userData ユーザデータ
 * @param [in] name     タグ名
 */
void XMLCALL CC_ParkingrateReq_EndElement(void *userData, const char *name)
{
	PARKINGPARSER *parser = (PARKINGPARSER*)userData;
	SMPARKINGINFO	*parkingInfo = NULL;

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		if (e_SC_RESULT_SUCCESS != CB_Result) {
			break;
		}

		// パラメータチェック
		if (NULL == userData) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[userData], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}
		if (NULL == parser->buf) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[parser->buf], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}
		if (NULL == name) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[name], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}

		parkingInfo = &parser->parkingInfo->parking[parser->parkingInfo->parkingNum];

		if (CC_PARKINGNODE_API_STATUS == parser->state) {
			// <api_status>
			strcpy((char*)parser->apiStatus, (char*)parser->buf);
			parser->state = CC_PARKINGNODE_XML;
		} else if (CC_PARKINGNODE_STORE_ID == parser->state) {
			// <store_id>
			parser->parkingInfo->storeId = atoi((char*)parser->buf);
			parser->state = CC_PARKINGNODE_XML;
		} else if (CC_PARKINGNODE_STORE_NAME == parser->state) {
			// <store_name>
			strcpy((char*)parser->parkingInfo->storeName, (char*)parser->buf);
			parser->state = CC_PARKINGNODE_XML;
		} else if (CC_PARKINGNODE_PARKING_CNT == parser->state) {
			// <parking_cnt>
			parser->state = CC_PARKINGNODE_XML;
		} else if (CC_PARKINGNODE_PARKING_LIST == parser->state) {
			// <parking_list>
			parser->state = CC_PARKINGNODE_XML;
		} else if (CC_PARKINGNODE_ITEM == parser->state) {
			// <item>
			parser->state = CC_PARKINGNODE_PARKING_LIST;
			parser->parkingInfo->parkingNum++;
		} else if (CC_PARKINGNODE_PARKING_ID == parser->state) {
			// <parking_id>
			parkingInfo->parkingId = atoi(parser->buf);
			parser->state = CC_PARKINGNODE_ITEM;
		} else if (CC_PARKINGNODE_PARKING_NAME == parser->state) {
			// <parking_name>
			strcpy((char*)parkingInfo->parkingName, (char*)parser->buf);
			parser->state = CC_PARKINGNODE_ITEM;
		} else if (CC_PARKINGNODE_ENTRANCE_NAME == parser->state) {
			// <entrance_name>
			strcpy((char*)parkingInfo->entranceName, (char*)parser->buf);
			parser->state = CC_PARKINGNODE_ITEM;
		} else if (CC_PARKINGNODE_ENTRANCE_LAT == parser->state) {
			// <entrance_lat>
			parkingInfo->entranceLat = atof((char*)parser->buf);
			parser->state = CC_PARKINGNODE_ITEM;
		} else if (CC_PARKINGNODE_ENTRANCE_LON == parser->state) {
			// <entrance_lon>
			parkingInfo->entranceLon = atof((char*)parser->buf);
			parser->state = CC_PARKINGNODE_ITEM;
		} else if (CC_PARKINGNODE_EMPTY_RATE == parser->state) {
			// <empty_rate>
			parkingInfo->emptyRate = atoi((char*)parser->buf);
			parser->state = CC_PARKINGNODE_ITEM;
		} else if (CC_PARKINGNODE_EMPTY_RATE_GETDATETM == parser->state) {
			// <empty_rate_getdatetm>
			parkingInfo->emptyRateGetdatetm = atoi((char*)parser->buf);
			parser->state = CC_PARKINGNODE_ITEM;
		} else if (CC_PARKINGNODE_PRIORITY == parser->state) {
			// <priority>
			parkingInfo->priority = atoi((char*)parser->buf);
			parser->state = CC_PARKINGNODE_ITEM;
		} else if (CC_PARKINGNODE_XML_CHILD == parser->state) {
			parser->state = CC_PARKINGNODE_XML;
		} else if (CC_PARKINGNODE_PARKING_LIST_CHILD == parser->state) {
			parser->state = CC_PARKINGNODE_PARKING_LIST;
		} else if (CC_PARKINGNODE_ITEM_CHILD == parser->state) {
			parser->state = CC_PARKINGNODE_ITEM;
		}
	} while (0);

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}

/**
 * @brief 解析データ
 * @param [in] userData ユーザデータ
 * @param [in] data     データ
 * @param [in] len      データ長
 */
void XMLCALL CC_ParkingrateReq_CharacterData(void *userData, const XML_Char *data, INT32 len)
{
	PARKINGPARSER *parser = (PARKINGPARSER*)userData;
	//char buf[CC_CMN_XML_PARSE_DATA_SIZE + 1] = {};
	INT32	bufLen = 0;

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		if (e_SC_RESULT_SUCCESS != CB_Result) {
			break;
		}

		// パラメータチェック
		if (NULL == userData) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[userData], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}
		if (NULL == parser->buf) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[parser->buf], " HERE);
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

		if (CC_PARKINGNODE_API_STATUS == parser->state) {
			// <api_status>
			if (CC_PARKINGXML_DATA_APISTATUS_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_PARKINGNODE_STORE_ID == parser->state) {
			// <store_id>
			if (CC_PARKINGXML_DATA_STORE_ID_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_PARKINGNODE_STORE_NAME == parser->state) {
			// <store_name>
			if (CC_PARKINGXML_DATA_STORE_NAME_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_PARKINGNODE_PARKING_CNT == parser->state) {
			// <parking_cnt>
			if (CC_PARKINGXML_DATA_PARKING_CNT_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_PARKINGNODE_PARKING_LIST == parser->state) {
			// <parking_list>
		} else if (CC_PARKINGNODE_ITEM == parser->state) {
			// <item>
		} else if (CC_PARKINGNODE_PARKING_ID == parser->state) {
			// <parking_id>
			if (CC_PARKINGXML_DATA_PARKING_ID_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_PARKINGNODE_PARKING_NAME == parser->state) {
			// <parking_name>
			if (CC_PARKINGXML_DATA_PARKING_NAME_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_PARKINGNODE_ENTRANCE_NAME == parser->state) {
			// <entrance_name>
			if (CC_PARKINGXML_DATA_ENTRANCE_NAME_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_PARKINGNODE_ENTRANCE_LAT == parser->state) {
			// <entrance_lat>
			if (CC_PARKINGXML_DATA_ENTRANCE_LAT_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_PARKINGNODE_ENTRANCE_LON == parser->state) {
			// <entrance_lon>
			if (CC_PARKINGXML_DATA_ENTRANCE_LON_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_PARKINGNODE_EMPTY_RATE == parser->state) {
			// <empty_rate>
			if (CC_PARKINGXML_DATA_EMPTY_RATE_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_PARKINGNODE_EMPTY_RATE_GETDATETM == parser->state) {
			// <empty_rate_getdatetm>
			if (CC_PARKINGXML_DATA_EMPTY_RATE_GETDATETM_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_PARKINGNODE_PRIORITY == parser->state) {
			// <priority>
			if (CC_PARKINGXML_DATA_PRIORITY_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		}
	} while (0);

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}
