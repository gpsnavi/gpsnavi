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

#define	CC_ROOMSRCH_XML_NODE_COUNT				"count"
#define	CC_ROOMSRCH_XML_NODE_ROOM_LIST			"room_list"
#define	CC_ROOMSRCH_XML_NODE_ARRAY_ITEM			"array_item"
#define	CC_ROOMSRCH_XML_NODE_ROOM_NO			"room_no"
#define	CC_ROOMSRCH_XML_NODE_ROOM_NAME			"room_name"
#define	CC_ROOMSRCH_XML_NODE_JOINED				"joined"
#define	CC_ROOMSRCH_XML_NODE_UNREAD				"unread"
#define	CC_ROOMSRCH_XML_NODE_LAST_UPDATE		"last_update"
#define	CC_ROOMSRCH_XML_NODE_USER_LIST			"user_list"
#define	CC_ROOMSRCH_XML_NODE_GUID				"guid"
#define	CC_ROOMSRCH_XML_NODE_USER_NAME			"user_name"
#define	CC_ROOMSRCH_XML_NODE_LAST_ACTION		"last_action"
#define	CC_ROOMSRCH_XML_NODE_AVATAR_URL_SMALL	"avatar_url_small"
#define	CC_ROOMSRCH_XML_NODE_AVATAR_URL_MIDIUM	"avatar_url_medium"
#define	CC_ROOMSRCH_XML_NODE_AVATAR_URL_LARGE	"avatar_url_large"

#define	CC_ROOMSRCH_XML_DATA_APISTATUS_SIZE		CC_CMN_XML_RES_STS_CODE_SIZE
#define	CC_ROOMSRCH_XML_DATA_ROOMNO_SIZE		SCC_CHAT_MAXCHAR_ROOMNO
#define	CC_ROOMSRCH_XML_DATA_ROOMNAME_SIZE		SCC_CHAT_MAXCHAR_ROOMNAME
#define	CC_ROOMSRCH_XML_DATA_JOINED_SIZE		(5+1)
#define	CC_ROOMSRCH_XML_DATA_UNREAD_SIZE		(5+1)
#define	CC_ROOMSRCH_XML_DATA_LASTUPDATE_SIZE	SCC_CHAT_MAXCHAR_LASTDATE
#define	CC_ROOMSRCH_XML_DATA_GUID_SIZE			CC_CMN_GUID_STR_SIZE
#define	CC_ROOMSRCH_XML_DATA_USERNAME_SIZE		SCC_CHAT_MAXCHAR_USERNAME
#define	CC_ROOMSRCH_XML_DATA_LASTACTION_SIZE	SCC_CHAT_MAXCHAR_LASTDATE
#define	CC_ROOMSRCH_XML_DATA_AVTURLSML_SIZE		SCC_MAX_URL
#define	CC_ROOMSRCH_XML_DATA_AVTURLMID_SIZE		SCC_MAX_URL
#define	CC_ROOMSRCH_XML_DATA_AVTURLLRG_SIZE		SCC_MAX_URL

#define CC_ROOM_SRCH_SEND_BODY_SIZE	512

// ルーム検索レスポンスXML情報
typedef struct _ROOMINFO {
	INT32			roomListNum;
	SMROOMINFO		*roomList;
	INT32			*status;
	Char			*apiStatus;
} ROOMINFO;

// ルーム検索XMLパーサ
typedef struct _ROOMSRCH_PARSER {
	INT32			state;
	Char			*buf;
	ROOMINFO		roomInfo;
	INT32			roomNum;
} ROOMSRCH_PARSER;

// gem.srch
enum RoomSrchStatus {
	CC_ROOMSRCH_NODE_NONE = 0,
	CC_ROOMSRCH_NODE_ELGG,
	CC_ROOMSRCH_NODE_ELGG_CHILD,
	CC_ROOMSRCH_NODE_STATUS,
	CC_ROOMSRCH_NODE_RESULT,
	CC_ROOMSRCH_NODE_RESULT_CHILD,
	CC_ROOMSRCH_NODE_API_STATUS,
	CC_ROOMSRCH_NODE_COUNT,
	CC_ROOMSRCH_NODE_ROOM_LIST,
	CC_ROOMSRCH_NODE_ROOM_LIST_CHILD,
	CC_ROOMSRCH_NODE_ARRAY_ITEM,
	CC_ROOMSRCH_NODE_ARRAY_ITEM_CHILD,
	CC_ROOMSRCH_NODE_ROOM_NO,
	CC_ROOMSRCH_NODE_ROOM_NAME,
	CC_ROOMSRCH_NODE_JOINED,
	CC_ROOMSRCH_NODE_UNREAD,
	CC_ROOMSRCH_NODE_LAST_UPDATE,
	CC_ROOMSRCH_NODE_USER_LIST,
	CC_ROOMSRCH_NODE_USER_LIST_CHILD,
	CC_ROOMSRCH_NODE_ARRAY_ITEM_2,
	CC_ROOMSRCH_NODE_ARRAY_ITEM_2_CHILD,
	CC_ROOMSRCH_NODE_GUID,
	CC_ROOMSRCH_NODE_USER_NAME,
	CC_ROOMSRCH_NODE_LAST_ACTION,
	CC_ROOMSRCH_NODE_AVATAR_URL_SMALL,
	CC_ROOMSRCH_NODE_AVATAR_URL_MIDIUM,
	CC_ROOMSRCH_NODE_AVATAR_URL_LARGE
};


//------------------------------------------------
// 変数定義
//------------------------------------------------
static INT32	CB_Result;				// 処理結果 (コールバック関数用)
static Bool		CB_IsXmlFile;

//------------------------------------------------
// 関数定義
//------------------------------------------------
static E_SC_RESULT CC_RoomSrch_CreateUri(const T_CC_CMN_SMS_API_PRM* pPrm, Char* pUri, INT32 UriMax);	//URI生成
static E_SC_RESULT CC_RoomSrch_CreateBody(const T_CC_CMN_SMS_API_PRM* pPrm, const SMROOMSRCHQUALI *SrchQuali, Char* pBody, INT32 bodyMax);	//body生成

static E_SC_RESULT CC_RoomSrch_AnalyzeHttpResp(const Char *body, E_CONTEXT_TYPE contextType, SMROOMINFO *roomInf, INT32 *roomNum, SMCALOPT *opt, Char *apiStatus);
static E_SC_RESULT CC_RoomSrch_XmlParse(const Char* xml, T_CC_CMN_SMS_RESPONSE_INFO* resp_inf, SMROOMINFO *roomI, INT32 *roomNum, SMCALOPT *opt);
static void XMLCALL CC_RoomSrch_StartElement(void *userData, const char *name, const char **atts);
static void XMLCALL CC_RoomSrch_EndElement(void *userData, const char *name);
static void XMLCALL CC_RoomSrch_CharacterData(void *userData, const XML_Char *data, INT32 len);

/**
 * @brief ルーム検索
 * @param [IN] smcal           SMCAL
 * @param [IN]  parm           APIパラメータ
 * @param [IN/OUT] roomSearch  ルーム検索情報
 * @param [IN]  recv           センタ受信データ
 * @param [OUT] apiStatus      APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_RoomSrch_SendRecv(SMCAL *smcal,
								 const T_CC_CMN_SMS_API_PRM *parm,
								 const SMROOMSRCHQUALI *SrchQuali,
								 SMROOMINFO *roomInf,
								 INT32 *roomNum,
								 const Char *fileName,
								 Char *recv,
								 UINT32 recvBufSize,
								 Char *apiStatus,
								 Bool isPolling)
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
		// パラメータチェック
		if (CC_ISNULL(smcal)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[smcal], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(parm)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[parm], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(SrchQuali)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[SrchQuali], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(roomInf)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[roomInf], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((CC_ISNULL(roomNum)) || (0 >= *roomNum)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[roomNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(recv)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[recv], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISINVALIDSTR(fileName)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[fileName], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 初期化
		memset(&opt, 0, sizeof(SMCALOPT));
		opt.isResOutputFile = true;
		CC_GetTempDirPath(opt.resFilePath);
		strcat(opt.resFilePath, fileName);
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
		*apiStatus = EOS;

		// メモリ確保
		data = (Char*)SCC_MALLOC(CC_ROOM_SRCH_SEND_BODY_SIZE);
		if (NULL == data) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC() error " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		///URI生成
		ret = CC_RoomSrch_CreateUri(parm, uri, CC_CMN_URI_STR_MAX_LEN);
		if(CC_CMN_RESULT_OK != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_RoomSrch_CreateUri() error " HERE);
			break;
		}

		///body生成
		ret = CC_RoomSrch_CreateBody(parm, SrchQuali, data, CC_ROOM_SRCH_SEND_BODY_SIZE);
		if(CC_CMN_RESULT_OK != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_RoomSrch_CreateBody() error " HERE);
			break;
		}

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
		ret = CC_RoomSrch_AnalyzeHttpResp(body, contextType, roomInf, roomNum, &opt, apiStatus);
		if(e_SC_RESULT_SUCCESS != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_RoomSrch_AnalyzeHttpResp error, " HERE);
			break;
		}
	} while (0);

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
 * @brief Room.srch用URI生成
 * @param[in] pPrm SMS API関連パラメタテーブルポインタ
 * @param[out] pUri 生成URI出力先
 * @param[in] UriMax 生成URI出力先最大サイズ(byte)
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_PROC_ERR
*/
E_SC_RESULT CC_RoomSrch_CreateUri(const T_CC_CMN_SMS_API_PRM* pPrm,
									Char* pUri,
									INT32 UriMax)
{
	INT32 rslt = 0;
	E_SC_RESULT ret = CC_CMN_RESULT_OK;

	rslt = snprintf(								//成功時には、書き込まれたbyte数を返す(EOSは含まない)
				(char*)pUri,
				UriMax,
				"%s\?method=Room.srch",
				pPrm->ApiPrmNavi.common_uri			//「http://～/distribution/」までのURI部分
				);
	if((rslt < 0) || (UriMax <= rslt)){				//負なら明らかにエラー、EOS終端できなかった場合もエラー
		SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_RoomSrch_CreateUri() error len[%d] " HERE, rslt);
		ret = CC_CMN_RESULT_PROC_ERR;
	}

	return (ret);
}

//************************************************
//body生成
//************************************************
/**
 * @brief Room.srch用body生成
 * @param[in] pPrm SMS API関連パラメタテーブルポインタ
 * @param[in] roomSearch Room.srchパラメタテーブルポインタ
 * @param[out] pBody 生成body出力先
 * @param[in] bodyMax 生成body出力先最大サイズ(byte)
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_PROC_ERR
 * @note term_sigは直前のToken.Reqのレスポンス処理時に生成した端末アクセスキーを使用
*/
E_SC_RESULT CC_RoomSrch_CreateBody(const T_CC_CMN_SMS_API_PRM* pPrm,
									const SMROOMSRCHQUALI *SrchQuali,
									Char* pBody,
									INT32 bodyMax)
{
	INT32 rslt = 0;
	E_SC_RESULT ret = CC_CMN_RESULT_OK;
	rslt = snprintf(								//成功時には、書き込まれたbyte数を返す(EOSは含まない)
				(char*)pBody,
				bodyMax,
				"term_id=%s&term_sig=%s&guid=%s&user_sig=%s&offset=%d&limit=%d",
				pPrm->ApiPrmMups.new_term_id,		//端末ID (センター採番新規端末ID)
				pPrm->ApiPrmMups.term_sig,			//端末アクセスキー
				pPrm->ApiPrmMups.guid,				//ユーザID
				pPrm->ApiPrmMups.user_sig,			//ユーザアクセスキー
				SrchQuali->offset,					//取得データの開始位置
				SrchQuali->limit					//取得件数
				);
	if((rslt < 0) || (bodyMax <= rslt)){				//負なら明らかにエラー、EOS終端できなかった場合もエラー
		SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_RoomSrch_CreateBody() error len[%d] " HERE, rslt);
		ret = CC_CMN_RESULT_PROC_ERR;
	}

	// ソート条件の指定有の場合
	if((NULL != SrchQuali->pOrder) && (EOS != *(SrchQuali->pOrder))){
		rslt += snprintf(								//成功時には、書き込まれたbyte数を返す(EOSは含まない)
					(char*)(pBody + rslt),
					(bodyMax - rslt),
					"&order=%s",
					SrchQuali->pOrder					//ソート条件
					);
	}
	if((rslt < 0) || (bodyMax <= rslt)){				//負なら明らかにエラー、EOS終端できなかった場合もエラー
		SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_RoomSrch_CreateBody() error len[%d] " HERE, rslt);
		ret = CC_CMN_RESULT_PROC_ERR;
	}

	// 未読優先指定有の場合
	if((NULL != SrchQuali->pPriority) && (EOS != *(SrchQuali->pPriority))){
		rslt += snprintf(								//成功時には、書き込まれたbyte数を返す(EOSは含まない)
					(char*)(pBody + rslt),
					(bodyMax - rslt),
					"&priority=%s",
					SrchQuali->pPriority				//未読優先指定
					);
	}
	if((rslt < 0) || (bodyMax <= rslt)){				//負なら明らかにエラー、EOS終端できなかった場合もエラー
		SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_RoomSrch_CreateBody() error len[%d] " HERE, rslt);
		ret = CC_CMN_RESULT_PROC_ERR;
	}

	return (ret);
}

/**
 * @brief レスポンス解析
 * @param [IN] body         xmlデータ
 * @param [IN] contextType  コンテキスト
 * @param [IN/OUT] roomSearch GEM検索条件/結果
 * @param [IN] opt          オプション情報
 * @param [OUT] apiStatus   APIステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_RoomSrch_AnalyzeHttpResp(const Char *body, E_CONTEXT_TYPE contextType, SMROOMINFO *roomInf, INT32 *roomNum, SMCALOPT *opt, Char *apiStatus)
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
		ret = CC_RoomSrch_XmlParse((const Char*)body, &rsp_inf, roomInf, roomNum, opt);
		if(e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_RoomSrch_XmlParse error, " HERE);
			return (e_SC_RESULT_SMS_API_ERR);
		}

		// 正常系のXMLとして解析できなかった場合
		if (CC_CMN_XML_CIC_RES_STS_OK != rsp_inf.sts) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"status error, " HERE);
			return (e_SC_RESULT_SMS_API_ERR);
		}
//		if (!CHECK_API_STATUS(rsp_inf.apiSts)) {
		if (!CHECK_API_STATUS2(rsp_inf.apiSts)) {
			// 『チャットルーム（Sqliteファイル）が存在しない』は、エラーとしない
			if (0 != strcmp("SMSAPICE009", rsp_inf.apiSts)) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"api status error, " HERE);
				return (e_SC_RESULT_SMS_API_ERR);
			}
		}
	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief Gem.srch応答メッセージ解析
 * @param [IN] xml      XMLファイルのフルパス
 * @param [IN] resp_inf CICレスポンス情報
 * @param [OUT] gem     GEM情報格納領域
 * @param [IN/OUT] gem  GEM情報格納領域数/GEM情報数
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_RoomSrch_XmlParse(const Char* xml, T_CC_CMN_SMS_RESPONSE_INFO* resp_inf, SMROOMINFO* roomI, INT32 *roomNum, SMCALOPT *opt)
{
	E_SC_RESULT		ret = e_SC_RESULT_SUCCESS;
	ROOMSRCH_PARSER	roomsrchParser = {};
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
		memset(roomI, 0, (sizeof(SMROOMINFO) * (*roomNum)));
		roomsrchParser.buf = (Char*)SCC_MALLOC(CC_CMN_XML_LARGE_BUF_SIZE + 1);
		if (NULL == roomsrchParser.buf) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			CB_Result = e_SC_RESULT_MALLOC_ERR;
			ret = CB_Result;
			break;
		}
		roomsrchParser.roomInfo.roomListNum = 0;
		roomsrchParser.roomInfo.roomList = roomI;
		roomsrchParser.roomInfo.status = &resp_inf->sts;
		roomsrchParser.roomInfo.apiStatus = &resp_inf->apiSts[0];
		roomsrchParser.roomNum = *roomNum;
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
		XML_SetUserData(parser, &roomsrchParser);
		XML_SetElementHandler(parser, CC_RoomSrch_StartElement, CC_RoomSrch_EndElement);
		XML_SetCharacterDataHandler(parser, CC_RoomSrch_CharacterData);

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
		if (e_SC_RESULT_SUCCESS == CB_Result) {
			*roomNum = roomsrchParser.roomInfo.roomListNum;
		}
	} while (0);

	if (NULL != roomsrchParser.buf) {
		SCC_FREE(roomsrchParser.buf);
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
void XMLCALL CC_RoomSrch_StartElement(void *userData, const char *name, const char **atts)
{
	ROOMSRCH_PARSER *parser = (ROOMSRCH_PARSER*)userData;

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

		if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_ELGG)) {
			// <elgg>
			parser->state = CC_ROOMSRCH_NODE_ELGG;
		}

		// <elgg>
		if (CC_ROOMSRCH_NODE_ELGG == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_STATUS)) {
				// <status>
				parser->state = CC_ROOMSRCH_NODE_STATUS;
			} else if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_RESULT)) {
				// <result>
				parser->state = CC_ROOMSRCH_NODE_RESULT;
			} else if (0 != strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_ELGG)) {
				parser->state = CC_ROOMSRCH_NODE_ELGG_CHILD;
			}
		} else if (CC_ROOMSRCH_NODE_RESULT == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_API_STATUS)) {
				// <api_status>
				parser->state = CC_ROOMSRCH_NODE_API_STATUS;
			} else if (0 == strcmp((char*)name, (char*)CC_ROOMSRCH_XML_NODE_COUNT)) {
				// <count>
				parser->state = CC_ROOMSRCH_NODE_COUNT;
			} else if (0 == strcmp((char*)name, (char*)CC_ROOMSRCH_XML_NODE_ROOM_LIST)) {
				// <room_list>
				parser->state = CC_ROOMSRCH_NODE_ROOM_LIST;
			} else {
				parser->state = CC_ROOMSRCH_NODE_RESULT_CHILD;
			}
		} else if (CC_ROOMSRCH_NODE_ROOM_LIST == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_ROOMSRCH_XML_NODE_ARRAY_ITEM)) {
				// <array_item>
				parser->state = CC_ROOMSRCH_NODE_ARRAY_ITEM;
			} else {
				parser->state = CC_ROOMSRCH_NODE_ROOM_LIST_CHILD;
			}
		} else if (CC_ROOMSRCH_NODE_ARRAY_ITEM == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_ROOMSRCH_XML_NODE_ROOM_NO)) {
				// <room_no>
				parser->state = CC_ROOMSRCH_NODE_ROOM_NO;
			} else if (0 == strcmp((char*)name, (char*)CC_ROOMSRCH_XML_NODE_ROOM_NAME)) {
				// <room_name>
				parser->state = CC_ROOMSRCH_NODE_ROOM_NAME;
			} else if (0 == strcmp((char*)name, (char*)CC_ROOMSRCH_XML_NODE_JOINED)) {
				// <joined>
				parser->state = CC_ROOMSRCH_NODE_JOINED;
			} else if (0 == strcmp((char*)name, (char*)CC_ROOMSRCH_XML_NODE_UNREAD)) {
				// <unread>
				parser->state = CC_ROOMSRCH_NODE_UNREAD;
			} else if (0 == strcmp((char*)name, (char*)CC_ROOMSRCH_XML_NODE_LAST_UPDATE)) {
				// <last_update>
				parser->state = CC_ROOMSRCH_NODE_LAST_UPDATE;
			} else if (0 == strcmp((char*)name, (char*)CC_ROOMSRCH_XML_NODE_USER_LIST)) {
				// <user_list>
				parser->state = CC_ROOMSRCH_NODE_USER_LIST;
			} else {
				parser->state = CC_ROOMSRCH_NODE_ARRAY_ITEM_CHILD;
			}
		} else if (CC_ROOMSRCH_NODE_USER_LIST == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_ROOMSRCH_XML_NODE_ARRAY_ITEM)) {
				// <array_item>
				parser->state = CC_ROOMSRCH_NODE_ARRAY_ITEM_2;
			} else {
				parser->state = CC_ROOMSRCH_NODE_USER_LIST_CHILD;
			}
		} else if (CC_ROOMSRCH_NODE_ARRAY_ITEM_2 == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_ROOMSRCH_XML_NODE_GUID)) {
				// <guid>
				parser->state = CC_ROOMSRCH_NODE_GUID;
			} else if (0 == strcmp((char*)name, (char*)CC_ROOMSRCH_XML_NODE_USER_NAME)) {
				// <user_name>
				parser->state = CC_ROOMSRCH_NODE_USER_NAME;
			} else if (0 == strcmp((char*)name, (char*)CC_ROOMSRCH_XML_NODE_LAST_ACTION)) {
				// <last_action>
				parser->state = CC_ROOMSRCH_NODE_LAST_ACTION;
			} else if (0 == strcmp((char*)name, (char*)CC_ROOMSRCH_XML_NODE_AVATAR_URL_SMALL)) {
				// <avatar_url_small>
				parser->state = CC_ROOMSRCH_NODE_AVATAR_URL_SMALL;
			} else if (0 == strcmp((char*)name, (char*)CC_ROOMSRCH_XML_NODE_AVATAR_URL_MIDIUM)) {
				// <avatar_url_midium>
				parser->state = CC_ROOMSRCH_NODE_AVATAR_URL_MIDIUM;
			} else if (0 == strcmp((char*)name, (char*)CC_ROOMSRCH_XML_NODE_AVATAR_URL_LARGE)) {
				// <avatar_url_large>
				parser->state = CC_ROOMSRCH_NODE_AVATAR_URL_LARGE;
			} else {
				parser->state = CC_ROOMSRCH_NODE_ARRAY_ITEM_2_CHILD;
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
void XMLCALL CC_RoomSrch_EndElement(void *userData, const char *name)
{
	ROOMSRCH_PARSER *parser = (ROOMSRCH_PARSER*)userData;
	SMROOMINFO	*room = NULL;
	Bool	b_bool = false;

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
		if (NULL == parser->roomInfo.roomList) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[parser->roomInfo.roomList], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}
		if (NULL == name) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[name], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}

		room = &parser->roomInfo.roomList[parser->roomInfo.roomListNum];

		if (CC_ROOMSRCH_NODE_STATUS == parser->state) {
			// <status>
			*(parser->roomInfo.status) = atoi((char*)parser->buf);
			parser->state = CC_ROOMSRCH_NODE_ELGG;
		} else if (CC_ROOMSRCH_NODE_RESULT == parser->state) {
			// <result>
			parser->state = CC_ROOMSRCH_NODE_ELGG;
		} else if (CC_ROOMSRCH_NODE_API_STATUS == parser->state) {
			// <api_status>
			strcpy((char*)parser->roomInfo.apiStatus, (char*)parser->buf);
			parser->state = CC_ROOMSRCH_NODE_RESULT;
		} else if (CC_ROOMSRCH_NODE_COUNT == parser->state) {
			// <count>
			parser->state = CC_ROOMSRCH_NODE_RESULT;
		} else if (CC_ROOMSRCH_NODE_ROOM_LIST == parser->state) {
			// <room_list>
			parser->state = CC_ROOMSRCH_NODE_RESULT;
		} else if (CC_ROOMSRCH_NODE_ARRAY_ITEM == parser->state) {
			// <array_item>
			parser->state = CC_ROOMSRCH_NODE_ROOM_LIST;
			parser->roomInfo.roomListNum++;
		} else if (CC_ROOMSRCH_NODE_ROOM_NO == parser->state) {
			// <room_no>
			strcpy((char*)room->roomNo, (char*)parser->buf);
			parser->state = CC_ROOMSRCH_NODE_ARRAY_ITEM;
		} else if (CC_ROOMSRCH_NODE_ROOM_NAME == parser->state) {
			// <room_name>
			strcpy((char*)room->roomName, (char*)parser->buf);
			parser->state = CC_ROOMSRCH_NODE_ARRAY_ITEM;
		} else if (CC_ROOMSRCH_NODE_JOINED == parser->state) {
			// <joined>
			if(0 == strcmp("true", (const char*)parser->buf)){
				b_bool = true;
			}else{
				b_bool = false;
			}
			room->joined = b_bool;
			parser->state = CC_ROOMSRCH_NODE_ARRAY_ITEM;
		} else if (CC_ROOMSRCH_NODE_UNREAD == parser->state) {
			// <unread>
			if(0 == strcmp("true", (const char*)parser->buf)){
				b_bool = true;
			}else{
				b_bool = false;
			}
			room->unRead = b_bool;
			parser->state = CC_ROOMSRCH_NODE_ARRAY_ITEM;
		} else if (CC_ROOMSRCH_NODE_LAST_UPDATE == parser->state) {
			// <last_update>
			strcpy((char*)room->lastDatetm, (char*)parser->buf);
			parser->state = CC_ROOMSRCH_NODE_ARRAY_ITEM;
		} else if (CC_ROOMSRCH_NODE_USER_LIST == parser->state) {
			// <user_list>
			parser->state = CC_ROOMSRCH_NODE_ARRAY_ITEM;
		} else if (CC_ROOMSRCH_NODE_ARRAY_ITEM_2 == parser->state) {
			// <array_item>
			parser->state = CC_ROOMSRCH_NODE_USER_LIST;
			room->userCnt++;
		} else if (CC_ROOMSRCH_NODE_GUID == parser->state) {
			// <guid>
			strcpy((char*)room->userBaseInf[room->userCnt].guid, (char*)parser->buf);
			parser->state = CC_ROOMSRCH_NODE_ARRAY_ITEM_2;
		} else if (CC_ROOMSRCH_NODE_USER_NAME == parser->state) {
			// <user_name>
			strcpy((char*)room->userBaseInf[room->userCnt].userName, (char*)parser->buf);
			parser->state = CC_ROOMSRCH_NODE_ARRAY_ITEM_2;
		} else if (CC_ROOMSRCH_NODE_LAST_ACTION == parser->state) {
			// <last_action>
			strcpy((char*)room->userBaseInf[room->userCnt].lastDate, (char*)parser->buf);
			parser->state = CC_ROOMSRCH_NODE_ARRAY_ITEM_2;
		} else if (CC_ROOMSRCH_NODE_AVATAR_URL_SMALL == parser->state) {
			// <avatar_url_small>
			strcpy((char*)room->userBaseInf[room->userCnt].avtSmlURL, (char*)parser->buf);
			parser->state = CC_ROOMSRCH_NODE_ARRAY_ITEM_2;
		} else if (CC_ROOMSRCH_NODE_AVATAR_URL_MIDIUM == parser->state) {
			// <avatar_url_midium>
			strcpy((char*)room->userBaseInf[room->userCnt].avtMidURL, (char*)parser->buf);
			parser->state = CC_ROOMSRCH_NODE_ARRAY_ITEM_2;
		} else if (CC_ROOMSRCH_NODE_AVATAR_URL_LARGE == parser->state) {
			// <avatar_url_large>
			strcpy((char*)room->userBaseInf[room->userCnt].avtLrgURL, (char*)parser->buf);
			parser->state = CC_ROOMSRCH_NODE_ARRAY_ITEM_2;
		} else if (CC_ROOMSRCH_NODE_ELGG_CHILD == parser->state) {
			parser->state = CC_ROOMSRCH_NODE_ELGG;
		} else if (CC_ROOMSRCH_NODE_RESULT_CHILD == parser->state) {
			parser->state = CC_ROOMSRCH_NODE_RESULT;
		} else if (CC_ROOMSRCH_NODE_ROOM_LIST_CHILD == parser->state) {
			parser->state = CC_ROOMSRCH_NODE_ROOM_LIST;
		} else if (CC_ROOMSRCH_NODE_ARRAY_ITEM_CHILD == parser->state) {
			parser->state = CC_ROOMSRCH_NODE_ARRAY_ITEM;
		} else if (CC_ROOMSRCH_NODE_USER_LIST_CHILD == parser->state) {
			parser->state = CC_ROOMSRCH_NODE_USER_LIST;
		} else if (CC_ROOMSRCH_NODE_ARRAY_ITEM_2_CHILD == parser->state) {
			parser->state = CC_ROOMSRCH_NODE_ARRAY_ITEM_2;
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
void XMLCALL CC_RoomSrch_CharacterData(void *userData, const XML_Char *data, INT32 len)
{
	ROOMSRCH_PARSER *parser = (ROOMSRCH_PARSER*)userData;
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

		if (CC_ROOMSRCH_NODE_STATUS == parser->state) {
			// <status>
			if (CC_CMN_XML_RES_STS_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_ROOMSRCH_NODE_API_STATUS == parser->state) {
			// <api_status>
			if (CC_ROOMSRCH_XML_DATA_APISTATUS_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_ROOMSRCH_NODE_COUNT == parser->state) {
			// <count>
		} else if (CC_ROOMSRCH_NODE_ROOM_LIST == parser->state) {
			// <room_list>
		} else if (CC_ROOMSRCH_NODE_ARRAY_ITEM == parser->state) {
			// <array_item>
		} else if (CC_ROOMSRCH_NODE_ROOM_NO == parser->state) {
			// <room_no>
			if (CC_ROOMSRCH_XML_DATA_ROOMNO_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_ROOMSRCH_NODE_ROOM_NAME == parser->state) {
			// <room_name>
			if (CC_ROOMSRCH_XML_DATA_ROOMNAME_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_ROOMSRCH_NODE_JOINED == parser->state) {
			// <joined>
			if (CC_ROOMSRCH_XML_DATA_JOINED_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_ROOMSRCH_NODE_UNREAD == parser->state) {
			// <unread>
			if (CC_ROOMSRCH_XML_DATA_UNREAD_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_ROOMSRCH_NODE_LAST_UPDATE == parser->state) {
			// <last_update>
			if (CC_ROOMSRCH_XML_DATA_LASTUPDATE_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_ROOMSRCH_NODE_USER_LIST == parser->state) {
			// <user_list>
		} else if (CC_ROOMSRCH_NODE_ARRAY_ITEM_2 == parser->state) {
			// <array_item>
		} else if (CC_ROOMSRCH_NODE_GUID == parser->state) {
			// <guid>
			if (CC_ROOMSRCH_XML_DATA_GUID_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_ROOMSRCH_NODE_USER_NAME == parser->state) {
			// <user_name>
			if (CC_ROOMSRCH_XML_DATA_USERNAME_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_ROOMSRCH_NODE_LAST_ACTION == parser->state) {
			// <last_action>
			if (CC_ROOMSRCH_XML_DATA_LASTACTION_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_ROOMSRCH_NODE_AVATAR_URL_SMALL == parser->state) {
			// <avatar_url_small>
			if (CC_ROOMSRCH_XML_DATA_AVTURLSML_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_ROOMSRCH_NODE_AVATAR_URL_MIDIUM == parser->state) {
			// <avatar_url_midium>
			if (CC_ROOMSRCH_XML_DATA_AVTURLMID_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_ROOMSRCH_NODE_AVATAR_URL_LARGE == parser->state) {
			// <avatar_url_large>
			if (CC_ROOMSRCH_XML_DATA_AVTURLLRG_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		}
	} while (0);

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}
