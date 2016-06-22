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

#define	CC_PROBEREQ_SEND_BODY_SIZE					1024

#define	CC_PROBEREQ_XML_NODE_PROBE_LIST				"probe"
#define	CC_PROBEREQ_XML_NODE_ITEM					"item"
#define	CC_PROBEREQ_XML_NODE_GPS_LAT				"gpsLatitude"
#define	CC_PROBEREQ_XML_NODE_GPS_LON				"gpsLongitude"
#define	CC_PROBEREQ_XML_NODE_MAP_LAT				"mapmatchingLatitude"
#define	CC_PROBEREQ_XML_NODE_MAP_LON				"mapmatchingLongitude"
#define	CC_PROBEREQ_XML_NODE_PARCEL_ID				"parcelid"
#define	CC_PROBEREQ_XML_NODE_TIME					"time"

#define	CC_PROBEREQ_RES_ZLIB_FILE					"probeInfo.z"
#define	CC_PROBEREQ_RES_XML_FILE					"probeInfo.xml"

#define	CC_PROBEREQ_XML_DATA_APISTATUS_SIZE			CC_CMN_XML_RES_STS_CODE_SIZE
#define	CC_PROBEREQ_XML_DATA_COUNT_SIZE				11
#define	CC_PROBEREQ_XML_DATA_GPS_LAT_SIZE			11
#define	CC_PROBEREQ_XML_DATA_GPS_LON_SIZE			11
#define	CC_PROBEREQ_XML_DATA_MAP_LAT_SIZE			11
#define	CC_PROBEREQ_XML_DATA_MAP_LON_SIZE			11
#define	CC_PROBEREQ_XML_DATA_PARCEL_ID_SIZE			11
#define	CC_PROBEREQ_XML_DATA_DATETM_SIZE			26

// プローブ情報検索レスポンスXML情報
typedef struct _PROBEINFO {
	INT32				probeListNum;
	SMPROBEINFO	*probeList;
	INT32				*status;
	Char				*apiStatus;
} PROBEINFO;

// プローブ情報XMLパーサ
typedef struct _PROBE_PARSER {
	INT32			state;
	Char			*buf;
	PROBEINFO		probeInfo;
} PROBE_PARSER;

// traffic/probereq/
enum ProbeReqStatus {
	CC_PROBEREQ_NODE_NONE = 0,
	CC_PROBEREQ_NODE_XML,
	CC_PROBEREQ_NODE_XML_CHILD,
	CC_PROBEREQ_NODE_API_STATUS,
	CC_PROBEREQ_NODE_PROBE_LIST,
	CC_PROBEREQ_NODE_PROBE_LIST_CHILD,
	CC_PROBEREQ_NODE_ITEM,
	CC_PROBEREQ_NODE_ITEM_CHILD,
	CC_PROBEREQ_NODE_GPS_LAT,
	CC_PROBEREQ_NODE_GPS_LON,
	CC_PROBEREQ_NODE_MAP_LAT,
	CC_PROBEREQ_NODE_MAP_LON,
	CC_PROBEREQ_NODE_PARCEL_ID,
	CC_PROBEREQ_NODE_TIME
};


//------------------------------------------------
// 変数定義
//------------------------------------------------
static INT32	CB_Result;				// 処理結果 (コールバック関数用)
static Bool		CB_IsXmlFile;

//------------------------------------------------
// 関数定義
//------------------------------------------------
static void CC_ProbeReq_CreateUri(const T_CC_CMN_SMS_API_PRM *param, Char *uri);
static void CC_ProbeReq_CreateBody(const T_CC_CMN_SMS_API_PRM *parm, const SMPROBESRCH *probeSrch, Char *body);
static E_SC_RESULT CC_ProbeReq_AnalyzeHttpResp(const Char *body, E_CONTEXT_TYPE contextType, SMPROBEINFO *probeInfo, SMPROBEINFONUM *probeInfoNum, SMCALOPT *opt, Char *apiStatus);
static E_SC_RESULT CC_ProbeReq_XmlParse(const Char* xml, T_CC_CMN_SMS_RESPONSE_INFO* resp_inf, SMPROBEINFO *probeInfo, SMPROBEINFONUM *probeInfoNum, SMCALOPT *opt);
static void XMLCALL CC_ProbeReq_StartElement(void *userData, const char *name, const char **atts);
static void XMLCALL CC_ProbeReq_EndElement(void *userData, const char *name);
static void XMLCALL CC_ProbeReq_CharacterData(void *userData, const XML_Char *data, INT32 len);

/**
 * @brief プローブ情報検索
 * @param [IN]  smcal           SMCAL
 * @param [IN]  parm            APIパラメータ
 * @param [OUT] probeSrch       プローブ情報検索条件
 * @param [OUT] probeInfo       プローブ情報検索結果
 * @param [OUT] probeInfoNum    プローブ情報検索結果取得件数
 * @param [IN]  recv            センタ受信データ
 * @param [OUT] apiStatus       APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_ProbeReq_SendRecv(SMCAL *smcal,
								 const T_CC_CMN_SMS_API_PRM *parm,
								 const SMPROBESRCH *probeSrch,
								 SMPROBEINFO *probeInfo,
								 SMPROBEINFONUM *probeInfoNum,
								 Char *recv,
								 INT32 recvBufSize,
								 Char *apiStatus)
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
		strcat(opt.resFilePath, CC_PROBEREQ_RES_XML_FILE);
#ifdef SC_CMN_BASIC_AUTH_SMS
		// BASIC認証
		opt.isBasicAuth = true;
		strcpy(opt.basic.basicAuthId, SC_CMN_BASIC_AUTH_ID);
		strcpy(opt.basic.basicAuthPwd, SC_CMN_BASIC_AUTH_PWD);
#endif
		*apiStatus = EOS;

		// メモリ確保
		uri = (Char*)SCC_MALLOC(CC_CMN_URI_STR_MAX_LEN);
		if (NULL == uri) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		data = (Char*)SCC_MALLOC(CC_PROBEREQ_SEND_BODY_SIZE);
		if (NULL == data) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// URI生成
		CC_ProbeReq_CreateUri(parm, uri);

		// body生成
		CC_ProbeReq_CreateBody(parm, probeSrch, data);

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
		ret = CC_ProbeReq_AnalyzeHttpResp(body,
										  contextType,
										  probeInfo,
										  probeInfoNum,
										  &opt,
										  apiStatus);
		if(e_SC_RESULT_SUCCESS != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ProbeReq_AnalyzeHttpResp error, " HERE);
			break;
		}
	} while (0);

	// XMLファイル削除
	remove(opt.resFilePath);

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
void CC_ProbeReq_CreateUri(const T_CC_CMN_SMS_API_PRM *parm, Char *uri)
{
	sprintf((char*)uri,
			"%straffic/probereq/",
			parm->ApiPrmNavi.sms_sp_uri
	);
}

/**
 * @brief body生成
 * @param [IN] parm         APIパラメータ
 * @param [IN] probeSrch    プローブ情報検索条件
 * @param [OUT] body        body
 */
void CC_ProbeReq_CreateBody(const T_CC_CMN_SMS_API_PRM *parm, const SMPROBESRCH *probeSrch, Char *body)
{
	sprintf((char*)body,
			"term_id=%s&term_sig=%s&guid=%s&user_sig=%s&from_time=%s&to_time=%s&app_ver=%s",
			parm->ApiPrmMups.new_term_id,
			parm->ApiPrmMups.term_sig,
			parm->ApiPrmMups.guid,
			parm->ApiPrmMups.user_sig,
			probeSrch->fromTime,
			probeSrch->toTime,
			parm->ApiPrmNavi.appVer
	);
}

/**
 * @brief レスポンス解析
 * @param [IN]  body            xmlデータ
 * @param [IN]  contextType     コンテキスト
 * @param [OUT] probeInfo       プローブ情報
 * @param [OUT] probeInfoNum    プローブ情報数
 * @param [IN]  opt             オプション情報
 * @param [OUT] apiStatus       APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_ProbeReq_AnalyzeHttpResp(const Char *body,
										E_CONTEXT_TYPE contextType,
										SMPROBEINFO *probeInfo,
										SMPROBEINFONUM *probeInfoNum,
										SMCALOPT *opt,
										Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	T_CC_CMN_SMS_RESPONSE_INFO	rsp_inf = {};

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// XML解析
		rsp_inf.apiSts = apiStatus;
		ret = CC_ProbeReq_XmlParse((const Char*)body, &rsp_inf, probeInfo, probeInfoNum, opt);
		if(e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ProbeReq_XmlParse error, " HERE);
			ret = e_SC_RESULT_SMS_API_ERR;
			break;
		}

		if ((!CHECK_API_STATUS(rsp_inf.apiSts)) && (!CHECK_API_STATUS2(rsp_inf.apiSts))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"api status error, " HERE);
			ret = e_SC_RESULT_SMS_API_ERR;
			break;
		}
	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief traffic/probereq/応答メッセージ解析
 * @param [IN] xml              XMLファイルのフルパス
 * @param [IN] resp_inf         レスポンス情報
 * @param [OUT] probeInfo       プローブ情報
 * @param [OUT] probeInfoNum    プローブ情報数
 * @param [IN]  opt             オプション情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_ProbeReq_XmlParse(const Char* xml,
								 T_CC_CMN_SMS_RESPONSE_INFO* resp_inf,
								 SMPROBEINFO *probeInfo,
								 SMPROBEINFONUM *probeInfoNum,
								 SMCALOPT *opt)
{
	E_SC_RESULT		ret = e_SC_RESULT_SUCCESS;
	PROBE_PARSER	probesrchParser = {};
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
		memset(probeInfo, 0, (sizeof(SMPROBEINFO) * CC_CMN_PROBEINFO_MAXNUM));
		memset(probeInfoNum, 0, sizeof(SMPROBEINFONUM));
		probesrchParser.buf = (Char*)SCC_MALLOC(CC_CMN_XML_LARGE_BUF_SIZE + 1);
		if (NULL == probesrchParser.buf) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			CB_Result = e_SC_RESULT_MALLOC_ERR;
			ret = CB_Result;
			break;
		}
		probesrchParser.probeInfo.probeListNum = 0;
		probesrchParser.probeInfo.probeList = probeInfo;
		probesrchParser.probeInfo.apiStatus = &resp_inf->apiSts[0];
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
		XML_SetUserData(parser, &probesrchParser);
		XML_SetElementHandler(parser, CC_ProbeReq_StartElement, CC_ProbeReq_EndElement);
		XML_SetCharacterDataHandler(parser, CC_ProbeReq_CharacterData);

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
		if (e_SC_RESULT_SUCCESS == CB_Result) {
			probeInfoNum->probeInfoNum = probesrchParser.probeInfo.probeListNum;
		}
	} while (0);

	if (NULL != probesrchParser.buf) {
		SCC_FREE(probesrchParser.buf);
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
 * @param [IN/OUT] userData ユーザデータ
 * @param [IN] name     タグ名
 * @param [OUT] atts    属性(未使用)
 */
void XMLCALL CC_ProbeReq_StartElement(void *userData, const char *name, const char **atts)
{
	PROBE_PARSER *parser = (PROBE_PARSER*)userData;

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		if (e_SC_RESULT_SUCCESS != CB_Result) {
			break;
		}
		if (CC_ISCANCEL()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"cancel, " HERE);
			CB_Result = e_SC_RESULT_CANCEL;
			break;
		}

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
			parser->state = CC_PROBEREQ_NODE_XML;
		}

		// <xml>
		if (CC_PROBEREQ_NODE_XML == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_API_STATUS)) {
				// <api_status>
				parser->state = CC_PROBEREQ_NODE_API_STATUS;
			} else if (0 == strcmp((char*)name, (char*)CC_PROBEREQ_XML_NODE_PROBE_LIST)) {
				// <probe>
				parser->state = CC_PROBEREQ_NODE_PROBE_LIST;
			} else if (0 != strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_XML)) {
				parser->state = CC_PROBEREQ_NODE_XML_CHILD;
			}
		} else if (CC_PROBEREQ_NODE_PROBE_LIST == parser->state) {
			if (0 == strncmp((char*)name, (char*)CC_PROBEREQ_XML_NODE_ITEM, strlen((char*)CC_PROBEREQ_XML_NODE_ITEM))) {
				// <item>
				if (CC_CMN_PROBEINFO_MAXNUM <= parser->probeInfo.probeListNum) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"item maxnum over, " HERE);
					CB_Result = e_SC_RESULT_SMS_API_ERR;
					break;
				}
				parser->state = CC_PROBEREQ_NODE_ITEM;
			} else {
				parser->state = CC_PROBEREQ_NODE_PROBE_LIST_CHILD;
			}
		} else if (CC_PROBEREQ_NODE_ITEM == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_PROBEREQ_XML_NODE_GPS_LAT)) {
				// <gpsLatitude>
				parser->state = CC_PROBEREQ_NODE_GPS_LAT;
			} else if (0 == strcmp((char*)name, (char*)CC_PROBEREQ_XML_NODE_GPS_LON)) {
				// <gpsLongitude>
				parser->state = CC_PROBEREQ_NODE_GPS_LON;
			} else if (0 == strcmp((char*)name, (char*)CC_PROBEREQ_XML_NODE_MAP_LAT)) {
				// <mapmatchingLatitude>
				parser->state = CC_PROBEREQ_NODE_MAP_LAT;
			} else if (0 == strcmp((char*)name, (char*)CC_PROBEREQ_XML_NODE_MAP_LON)) {
				// <mapmatchingLongitude>
				parser->state = CC_PROBEREQ_NODE_MAP_LON;
			} else if (0 == strcmp((char*)name, (char*)CC_PROBEREQ_XML_NODE_PARCEL_ID)) {
				// <parcel_id>
				parser->state = CC_PROBEREQ_NODE_PARCEL_ID;
			} else if (0 == strcmp((char*)name, (char*)CC_PROBEREQ_XML_NODE_TIME)) {
				// <time>
				parser->state = CC_PROBEREQ_NODE_TIME;
			} else {
				parser->state = CC_PROBEREQ_NODE_ITEM_CHILD;
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
 * @param [IN/OUT] userData ユーザデータ
 * @param [IN] name     タグ名
 */
void XMLCALL CC_ProbeReq_EndElement(void *userData, const char *name)
{
	PROBE_PARSER *parser = (PROBE_PARSER*)userData;
	SMPROBEINFO	*probe = NULL;

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		if (e_SC_RESULT_SUCCESS != CB_Result) {
			break;
		}
		if (CC_ISCANCEL()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"cancel, " HERE);
			CB_Result = e_SC_RESULT_CANCEL;
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
		if (NULL == parser->probeInfo.probeList) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[parser->probeInfo.probeList], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}
		if (NULL == name) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[name], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}

		probe = &parser->probeInfo.probeList[parser->probeInfo.probeListNum];

		if (CC_PROBEREQ_NODE_API_STATUS == parser->state) {
			// <api_status>
			strcpy((char*)parser->probeInfo.apiStatus, (char*)parser->buf);
			parser->state = CC_PROBEREQ_NODE_XML;
		} else if (CC_PROBEREQ_NODE_PROBE_LIST == parser->state) {
			// <probe>
			parser->state = CC_PROBEREQ_NODE_XML;
		} else if (CC_PROBEREQ_NODE_ITEM == parser->state) {
			// <item>
			parser->state = CC_PROBEREQ_NODE_PROBE_LIST;
			parser->probeInfo.probeListNum++;
		} else if (CC_PROBEREQ_NODE_GPS_LAT == parser->state) {
			// <gpsLatitude>
			probe->gpsLat = ((DOUBLE)atoi((char*)parser->buf)) / ((DOUBLE)(1024 * 3600));
			probe->gpsLatFlg = true;
			parser->state = CC_PROBEREQ_NODE_ITEM;
		} else if (CC_PROBEREQ_NODE_GPS_LON == parser->state) {
			// <gpsLongitude>
			probe->gpsLon = ((DOUBLE)atoi((char*)parser->buf)) / ((DOUBLE)(1024 * 3600));
			probe->gpsLonFlg = true;
			parser->state = CC_PROBEREQ_NODE_ITEM;
		} else if (CC_PROBEREQ_NODE_MAP_LAT == parser->state) {
			// <mapmatchingLatitude>
			probe->mapLat = ((DOUBLE)atoi((char*)parser->buf)) / ((DOUBLE)(1024 * 3600));
			probe->mapLatFlg = true;
			parser->state = CC_PROBEREQ_NODE_ITEM;
		} else if (CC_PROBEREQ_NODE_MAP_LON == parser->state) {
			// <mapmatchingLongitude>
			probe->mapLon = ((DOUBLE)atoi((char*)parser->buf)) / ((DOUBLE)(1024 * 3600));
			probe->mapLonFlg = true;
			parser->state = CC_PROBEREQ_NODE_ITEM;
		} else if (CC_PROBEREQ_NODE_PARCEL_ID == parser->state) {
			// <parcel_id>
			probe->parcelId = atol((char*)parser->buf);
			parser->state = CC_PROBEREQ_NODE_ITEM;
		} else if (CC_PROBEREQ_NODE_TIME == parser->state) {
			// <time>
			memcpy(probe->time, parser->buf, 4);
			memcpy(&probe->time[4], &parser->buf[5], 2);
			memcpy(&probe->time[6], &parser->buf[8], 2);
			memcpy(&probe->time[8], &parser->buf[11], 2);
			memcpy(&probe->time[10], &parser->buf[14], 2);
			memcpy(&probe->time[12], &parser->buf[17], 2);
			probe->time[14] = EOS;
			parser->state = CC_PROBEREQ_NODE_ITEM;
		} else if (CC_PROBEREQ_NODE_XML_CHILD == parser->state) {
			parser->state = CC_PROBEREQ_NODE_XML;
		} else if (CC_PROBEREQ_NODE_PROBE_LIST_CHILD == parser->state) {
			parser->state = CC_PROBEREQ_NODE_PROBE_LIST;
		} else if (CC_PROBEREQ_NODE_ITEM_CHILD == parser->state) {
			parser->state = CC_PROBEREQ_NODE_ITEM;
		}
	} while (0);

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}

/**
 * @brief 解析データ
 * @param [IN] userData ユーザデータ
 * @param [IN] data     データ
 * @param [IN] len      データ長
 */
void XMLCALL CC_ProbeReq_CharacterData(void *userData, const XML_Char *data, INT32 len)
{
	PROBE_PARSER *parser = (PROBE_PARSER*)userData;
	//char buf[CC_CMN_XML_PARSE_DATA_SIZE + 1] = {};
	INT32	bufLen = 0;

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		if (e_SC_RESULT_SUCCESS != CB_Result) {
			break;
		}
		if (CC_ISCANCEL()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"cancel, " HERE);
			CB_Result = e_SC_RESULT_CANCEL;
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

		if (CC_PROBEREQ_NODE_API_STATUS == parser->state) {
			// <api_status>
			if (CC_PROBEREQ_XML_DATA_APISTATUS_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_PROBEREQ_NODE_PROBE_LIST == parser->state) {
			// <probe>
		} else if (CC_PROBEREQ_NODE_ITEM == parser->state) {
			// <item>
		} else if (CC_PROBEREQ_NODE_GPS_LAT == parser->state) {
			// <gpsLatitude>
			if (CC_PROBEREQ_XML_DATA_GPS_LAT_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_PROBEREQ_NODE_GPS_LON == parser->state) {
			// <gpsLongitude>
			if (CC_PROBEREQ_XML_DATA_GPS_LON_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_PROBEREQ_NODE_MAP_LAT == parser->state) {
			// <mapmatchingLatitude>
			if (CC_PROBEREQ_XML_DATA_MAP_LAT_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_PROBEREQ_NODE_MAP_LON == parser->state) {
			// <mapmatchingLongitude>
			if (CC_PROBEREQ_XML_DATA_MAP_LON_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_PROBEREQ_NODE_PARCEL_ID == parser->state) {
			// <parcel_id>
			if (CC_PROBEREQ_XML_DATA_PARCEL_ID_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_PROBEREQ_NODE_TIME == parser->state) {
			// <time>
			if (CC_PROBEREQ_XML_DATA_DATETM_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			} else {
			}
		}
	} while (0);

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}
