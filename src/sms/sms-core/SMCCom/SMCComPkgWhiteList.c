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

#define	CC_PKG_WHITELIST_FILE_NAME						"api_list.xml"			// ホワイトリストファイル名

#define	CC_PKG_WHITELIST_XML_NODE_PKG_LIST				"pkg_list"
#define	CC_PKG_WHITELIST_XML_NODE_PKG_LIST_ITEM			"item"
#define	CC_PKG_WHITELIST_XML_NODE_VERSION				"version"
#define	CC_PKG_WHITELIST_XML_NODE_API_NUM				"api_num"
#define	CC_PKG_WHITELIST_XML_NODE_API_LIST				"api_list"
#define	CC_PKG_WHITELIST_XML_NODE_API_LIST_ITEM			"item"
#define	CC_PKG_WHITELIST_XML_NODE_API_NAME				"api_name"
#define	CC_PKG_WHITELIST_XML_NODE_RES_TYPE				"res_type"

#define	CC_PKG_WHITELIST_XML_DATA_VERSION_SIZE			CC_CMN_PKGMGR_VERSION
#define	CC_PKG_WHITELIST_XML_DATA_API_NUM_SIZE			11
#define	CC_PKG_WHITELIST_XML_DATA_API_NAME_SIZE			CC_CMN_PKGCMN_WHITELIST_API_NAME_SIZE
#define	CC_PKG_WHITELIST_XML_DATA_RES_TYPE_SIZE			6

// ホワイトリストXMLパーサ
typedef struct _PKGWHITELIST_PARSER {
	INT32			state;
	Char			*buf;
	const Char		*chkVersion;
	Char			version[CC_CMN_PKGMGR_VERSION];
	SMPKGWHITELIST	*pkgWhiteList;
	INT32			*pkgWhiteListNum;
	SMPKGWHITELIST	*tmpPkgWhiteList;
	INT32			*tmpPkgWhiteListNum;
} PKGWHITELIST_PARSER;

enum PkgWhiteListStatus {
	CC_PKG_WHITELIST_NODE_NONE = 0,
	CC_PKG_WHITELIST_NODE_XML,
	CC_PKG_WHITELIST_NODE_XML_CHILD,
	CC_PKG_WHITELIST_NODE_PKG_LIST,
	CC_PKG_WHITELIST_NODE_PKG_LIST_CHILD,
	CC_PKG_WHITELIST_NODE_PKG_LIST_ITEM,
	CC_PKG_WHITELIST_NODE_PKG_LIST_ITEM_CHILD,
	CC_PKG_WHITELIST_NODE_VERSION,
	CC_PKG_WHITELIST_NODE_API_NUM,
	CC_PKG_WHITELIST_NODE_API_LIST,
	CC_PKG_WHITELIST_NODE_API_LIST_CHILD,
	CC_PKG_WHITELIST_NODE_API_LIST_ITEM,
	CC_PKG_WHITELIST_NODE_API_LIST_ITEM_CHILD,
	CC_PKG_WHITELIST_NODE_API_NAME,
	CC_PKG_WHITELIST_NODE_RES_TYPE
};

//------------------------------------------------
// 変数定義
//------------------------------------------------
static INT32	CB_Result;				// 処理結果 (コールバック関数用)

//------------------------------------------------
// 関数定義
//------------------------------------------------
static E_SC_RESULT CC_Pkg_WhiteList_XmlParse(const Char *xmlFilePath, const Char *version, SMPKGWHITELIST *pkgWhiteList, INT32 *pkgWhiteListNum);
static void XMLCALL CC_Pkg_WhiteList_StartElement(void *userData, const char *name, const char **atts);
static void XMLCALL CC_Pkg_WhiteList_EndElement(void *userData, const char *name);
static void XMLCALL CC_Pkg_WhiteList_CharacterData(void *userData, const XML_Char *data, INT32 len);

/**
 * @brief ホワイトリスト取得
 * @param [IN] smcal        SMCAL
 * @param [IN] parm         APIパラメータ
 * @param [IN] pkgInfo      パッケージ情報
 * @param [OUT] pkgWhiteList    ホワイトリスト
 * @param [OUT] pkgWhiteListNum ホワイトリスト数
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_Pkg_GetPkgWhiteList(SMCAL *smcal,
								   T_CC_CMN_SMS_API_PRM *parm,
								   SMPACKAGEINFO *pkgInfo,
								   SMPKGWHITELIST *pkgWhiteList,
								   INT32 *pkgWhiteListNum)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	//E_SC_CAL_RESULT	calRet = e_SC_CAL_RESULT_SUCCESS;
	Char	*tempDirPath = NULL;
	Char	*xmlFilePath = NULL;
	INT32	whiteListNum = -1;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// 初期化
		*pkgWhiteListNum = 0;
		memset(pkgWhiteList, 0, (sizeof(SMPKGWHITELIST) * CC_CMN_PKGCMN_WHITELIST_API_MAXNUM));

		// メモリ確保
		tempDirPath = (Char*)SCC_MALLOC(SCC_MAX_PATH);
		if (NULL == tempDirPath) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		// テンポラリディレクトリのパス取得
		CC_GetTempDirPath(tempDirPath);

		// ダウンロードするホワイトリストファイル名を設定
		strcpy((char*)pkgInfo->packageFileName, CC_PKG_WHITELIST_FILE_NAME);

		// メモリ確保
		xmlFilePath = (Char*)SCC_MALLOC(SCC_MAX_PATH);
		if (NULL == xmlFilePath) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// ダウンロードしたホワイトリストファイルのパスを設定
		sprintf(xmlFilePath, "%s%s", tempDirPath, CC_PKG_WHITELIST_FILE_NAME);

		// ホワイトリストをダウンロード
		ret = CC_DownloadPackage(smcal, parm, PKGDLDATATYPE_OTHER, pkgInfo, tempDirPath);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_DownloadPackage error, " HERE);
			break;
		}

		// XML解析
		ret = CC_Pkg_WhiteList_XmlParse(xmlFilePath, pkgInfo->version, pkgWhiteList, &whiteListNum);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_Pkg_WhiteList_XmlParse error, " HERE);
			break;
		}

		if (0 > pkgWhiteListNum) {
			// 対象バージョンのパッケージ情報がない => パッケージの更新必須
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"package need update, " HERE);
			ret = e_SC_RESULT_NEDD_PKG_UPDATE;
			break;
		} else if (0 == pkgWhiteListNum) {
			// チェック対象なし（チェック不要）
			break;
		} else {
			*pkgWhiteListNum = whiteListNum;
		}
	} while (0);

	// メモリ解放
	if (NULL != tempDirPath) {
		SCC_FREE(tempDirPath);
	}
	if (NULL != xmlFilePath) {
		// XMLファイル削除
		remove(xmlFilePath);

		// メモリ解放
		SCC_FREE(xmlFilePath);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief XML解析
 * @param [IN] xmlFilePath      XMLファイルのフルパス
 * @param [IN] version          バージョン
 * @param [OUT] pkgWhiteList    ホワイトリスト
 * @param [OUT] pkgWhiteListNum ホワイトリスト数
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_Pkg_WhiteList_XmlParse(const Char *xmlFilePath,
									  const Char *version,
									  SMPKGWHITELIST *pkgWhiteList,
									  INT32 *pkgWhiteListNum)
{
	E_SC_RESULT		ret = e_SC_RESULT_SUCCESS;
	PKGWHITELIST_PARSER	listParser = {};
	Char buf[CC_CMN_XML_PARSE_DATA_SIZE + 1] = {};
	XML_Parser parser = NULL;
	INT32	done = 0;
	INT32	len = 0;
	//INT32	parsedLen = 0;
	FILE	*fp = NULL;
	INT32	tmpPkgWhiteListNum = 0;
	SMPKGWHITELIST	*tmpPkgWhiteList = NULL;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// 初期化
		// メモリ確保
		listParser.buf = (Char*)SCC_MALLOC(CC_CMN_XML_LARGE_BUF_SIZE + 1);
		if (NULL == listParser.buf) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			CB_Result = e_SC_RESULT_MALLOC_ERR;
			ret = CB_Result;
			break;
		}
		// メモリ確保
		tmpPkgWhiteList = (SMPKGWHITELIST*)SCC_MALLOC(sizeof(SMPKGWHITELIST) * CC_CMN_PKGCMN_WHITELIST_API_MAXNUM);
		if (NULL == tmpPkgWhiteList) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			CB_Result = e_SC_RESULT_MALLOC_ERR;
			ret = CB_Result;
			break;
		}
		memset(tmpPkgWhiteList, 0, (sizeof(SMPKGWHITELIST) * CC_CMN_PKGCMN_WHITELIST_API_MAXNUM));
		listParser.chkVersion = version;
		listParser.version[0] = EOS;
		listParser.pkgWhiteList = pkgWhiteList;
		listParser.pkgWhiteListNum = pkgWhiteListNum;
		listParser.tmpPkgWhiteList = tmpPkgWhiteList;
		listParser.tmpPkgWhiteListNum = &tmpPkgWhiteListNum;

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
		XML_SetUserData(parser, &listParser);
		XML_SetElementHandler(parser, CC_Pkg_WhiteList_StartElement, CC_Pkg_WhiteList_EndElement);
		XML_SetCharacterDataHandler(parser, CC_Pkg_WhiteList_CharacterData);

		fp = fopen((char*)xmlFilePath, "r");
		if (NULL == fp) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"file open error(%d)[%s], " HERE, errno, xmlFilePath);
			CB_Result = e_SC_RESULT_FILE_ACCESSERR;
			ret = CB_Result;
			break;
		}

		while (!done) {
			if (CC_ISCANCEL()) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"cancel, " HERE);
				CB_Result = e_SC_RESULT_CANCEL;
				ret = CB_Result;
				break;
			}

			len = (INT32)fread(buf, 1, (sizeof(buf) - 1), fp);
			done = (len < (sizeof(buf) - 1));

			// XML解析
			if ((XML_STATUS_ERROR == XML_Parse(parser, (const char*)buf, len, done)) || (e_SC_RESULT_SUCCESS != CB_Result)) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"XML_Parse error(0x%08x), " HERE, CB_Result);
				CB_Result = e_SC_RESULT_DATA_ERR;
				ret = CB_Result;
				break;
			}

			if (!done) {
				// バッファクリア
				memset(buf, 0, (sizeof(buf) - 1));
			}
		}
	} while (0);

	// メモリ解放
	if (NULL != listParser.buf) {
		SCC_FREE(listParser.buf);
	}
	if (NULL != tmpPkgWhiteList) {
		SCC_FREE(tmpPkgWhiteList);
	}

	// ファイルクローズ
	if (NULL != fp) {
		fclose(fp);
	}

	// パーサ解放
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
void XMLCALL CC_Pkg_WhiteList_StartElement(void *userData, const char *name, const char **atts)
{
	PKGWHITELIST_PARSER *parser = (PKGWHITELIST_PARSER*)userData;

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

		if (0 < *parser->pkgWhiteListNum) {
			// 必要なデータは取得済み
			break;
		}

		// 初期化
		memset(parser->buf, 0, (CC_CMN_XML_BUF_SIZE + 1));

		if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_XML)) {
			// <xml>
			parser->state = CC_PKG_WHITELIST_NODE_XML;
		}

		// <xml>
		if (CC_PKG_WHITELIST_NODE_XML == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_PKG_WHITELIST_XML_NODE_PKG_LIST)) {
				// <pkg_list>
				parser->state = CC_PKG_WHITELIST_NODE_PKG_LIST;
			} else if (0 != strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_XML)) {
				parser->state = CC_PKG_WHITELIST_NODE_XML_CHILD;
			}
		} else if (CC_PKG_WHITELIST_NODE_PKG_LIST == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_PKG_WHITELIST_XML_NODE_API_LIST_ITEM)) {
				// <item>
				*parser->tmpPkgWhiteListNum = 0;
				memset(parser->tmpPkgWhiteList, 0, (sizeof(SMPKGWHITELIST) * CC_CMN_PKGCMN_WHITELIST_API_MAXNUM));
				parser->state = CC_PKG_WHITELIST_NODE_PKG_LIST_ITEM;
			} else {
				parser->state = CC_PKG_WHITELIST_NODE_PKG_LIST_CHILD;
			}
		} else if (CC_PKG_WHITELIST_NODE_PKG_LIST_ITEM == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_PKG_WHITELIST_XML_NODE_VERSION)) {
				// <version>
				parser->state = CC_PKG_WHITELIST_NODE_VERSION;
			} else 	if (0 == strcmp((char*)name, (char*)CC_PKG_WHITELIST_XML_NODE_API_NUM)) {
				// <api_num>
				parser->state = CC_PKG_WHITELIST_NODE_API_NUM;
			} else if (0 == strcmp((char*)name, (char*)CC_PKG_WHITELIST_XML_NODE_API_LIST)) {
				// <api_list>
				parser->state = CC_PKG_WHITELIST_NODE_API_LIST;
			} else if (0 != strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_XML)) {
				parser->state = CC_PKG_WHITELIST_NODE_PKG_LIST_ITEM_CHILD;
			}
		} else if (CC_PKG_WHITELIST_NODE_API_LIST == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_PKG_WHITELIST_XML_NODE_API_LIST_ITEM)) {
				// <item>
				if (CC_CMN_PKGCMN_WHITELIST_API_MAXNUM <= *parser->tmpPkgWhiteListNum) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"item maxnum over, " HERE);
					CB_Result = e_SC_RESULT_DATA_ERR;
					break;
				}
				parser->state = CC_PKG_WHITELIST_NODE_API_LIST_ITEM;
			} else {
				parser->state = CC_PKG_WHITELIST_NODE_API_LIST_CHILD;
			}
		} else if (CC_PKG_WHITELIST_NODE_API_LIST_ITEM == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_PKG_WHITELIST_XML_NODE_API_NAME)) {
				// <api_name>
				parser->state = CC_PKG_WHITELIST_NODE_API_NAME;
			} else if (0 == strcmp((char*)name, (char*)CC_PKG_WHITELIST_XML_NODE_RES_TYPE)) {
				// <res_type>
				parser->state = CC_PKG_WHITELIST_NODE_RES_TYPE;
			} else {
				parser->state = CC_PKG_WHITELIST_NODE_API_LIST_ITEM_CHILD;
			}
		} else {
			// 上記以外
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"format error, " HERE);
			CB_Result = e_SC_RESULT_DATA_ERR;
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
void XMLCALL CC_Pkg_WhiteList_EndElement(void *userData, const char *name)
{
	PKGWHITELIST_PARSER *parser = (PKGWHITELIST_PARSER*)userData;
	SMPKGWHITELIST	*list = NULL;

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
		if (NULL == name) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[name], " HERE);
			CB_Result = e_SC_RESULT_FAIL;
			break;
		}

		if (0 < *parser->pkgWhiteListNum) {
			// 必要なデータは取得済み
			break;
		}

		list = &parser->tmpPkgWhiteList[*parser->tmpPkgWhiteListNum];
		if (CC_PKG_WHITELIST_NODE_PKG_LIST == parser->state) {
			// <pkg_list>
			parser->state = CC_PKG_WHITELIST_NODE_XML;
		} else if (CC_PKG_WHITELIST_NODE_PKG_LIST_ITEM == parser->state) {
			// <item>
			if (0 == strcmp((char*)parser->chkVersion, (char*)parser->version)) {
				*parser->pkgWhiteListNum = *parser->tmpPkgWhiteListNum;
				memcpy(parser->pkgWhiteList, parser->tmpPkgWhiteList, (sizeof(SMPKGWHITELIST) * CC_CMN_PKGCMN_WHITELIST_API_MAXNUM));
			}
			parser->state = CC_PKG_WHITELIST_NODE_PKG_LIST;
		} else if (CC_PKG_WHITELIST_NODE_VERSION == parser->state) {
			// <version>
			strcpy((char*)parser->version, (char*)parser->buf);
			parser->state = CC_PKG_WHITELIST_NODE_PKG_LIST_ITEM;
		} else 	if (CC_PKG_WHITELIST_NODE_API_NUM == parser->state) {
			// <api_num>
			parser->state = CC_PKG_WHITELIST_NODE_PKG_LIST_ITEM;
		} else if (CC_PKG_WHITELIST_NODE_API_LIST == parser->state) {
			// <api_list>
			parser->state = CC_PKG_WHITELIST_NODE_PKG_LIST_ITEM;
		} else if (CC_PKG_WHITELIST_NODE_API_LIST_ITEM == parser->state) {
			// <item>
			parser->state = CC_PKG_WHITELIST_NODE_API_LIST;
			if (CC_CMN_PKGCMN_WHITELIST_API_MAXNUM > *parser->tmpPkgWhiteListNum) {
				(*parser->tmpPkgWhiteListNum)++;
			}
		} else if (CC_PKG_WHITELIST_NODE_API_NAME == parser->state) {
			// <api_name>
			if (CC_CMN_PKGCMN_WHITELIST_API_MAXNUM > *parser->tmpPkgWhiteListNum) {
				strcpy((char*)list->apiName, (char*)parser->buf);
				if ('/' == list->apiName[strlen((char*)list->apiName) - 1]) {
					list->apiName[strlen((char*)list->apiName) - 1] = EOS;
				}
			}
			parser->state = CC_PKG_WHITELIST_NODE_API_LIST_ITEM;
		} else if (CC_PKG_WHITELIST_NODE_RES_TYPE == parser->state) {
			// <res_type>
			if (CC_CMN_PKGCMN_WHITELIST_API_MAXNUM > *parser->tmpPkgWhiteListNum) {
				list->resType = atoi((char*)parser->buf);
			}
			parser->state = CC_PKG_WHITELIST_NODE_API_LIST_ITEM;
		} else if (CC_PKG_WHITELIST_NODE_XML_CHILD == parser->state) {
			parser->state = CC_PKG_WHITELIST_NODE_XML;
		} else if (CC_PKG_WHITELIST_NODE_PKG_LIST_CHILD == parser->state) {
			parser->state = CC_PKG_WHITELIST_NODE_PKG_LIST;
		} else if (CC_PKG_WHITELIST_NODE_PKG_LIST_ITEM_CHILD == parser->state) {
			parser->state = CC_PKG_WHITELIST_NODE_PKG_LIST_ITEM;
		} else if (CC_PKG_WHITELIST_NODE_API_LIST_CHILD == parser->state) {
			parser->state = CC_PKG_WHITELIST_NODE_API_LIST;
		} else if (CC_PKG_WHITELIST_NODE_API_LIST_ITEM_CHILD == parser->state) {
			parser->state = CC_PKG_WHITELIST_NODE_API_LIST_ITEM;
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
void XMLCALL CC_Pkg_WhiteList_CharacterData(void *userData, const XML_Char *data, INT32 len)
{
	PKGWHITELIST_PARSER *parser = (PKGWHITELIST_PARSER*)userData;
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

		if (0 < *parser->pkgWhiteListNum) {
			// 必要なデータは取得済み
			break;
		}

		// データをバッファにコピー
		bufLen = strlen((char*)parser->buf);

		if (CC_PKG_WHITELIST_NODE_PKG_LIST == parser->state) {
			// <pkg_list>
		} else if (CC_PKG_WHITELIST_NODE_PKG_LIST_ITEM == parser->state) {
			// <item>
		} else if (CC_PKG_WHITELIST_NODE_VERSION == parser->state) {
			// <version>
			if (CC_PKG_WHITELIST_XML_DATA_VERSION_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_PKG_WHITELIST_NODE_API_NUM == parser->state) {
			// <api_num>
			if (CC_PKG_WHITELIST_XML_DATA_API_NUM_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_PKG_WHITELIST_NODE_API_LIST == parser->state) {
			// <api_list>
		} else if (CC_PKG_WHITELIST_NODE_API_LIST_ITEM == parser->state) {
			// <item>
		} else if (CC_PKG_WHITELIST_NODE_API_NAME == parser->state) {
			// <api_name>
			if (CC_PKG_WHITELIST_XML_DATA_API_NAME_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_PKG_WHITELIST_NODE_RES_TYPE == parser->state) {
			// <res_type>
			if (CC_PKG_WHITELIST_XML_DATA_RES_TYPE_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		}
	} while (0);

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}
