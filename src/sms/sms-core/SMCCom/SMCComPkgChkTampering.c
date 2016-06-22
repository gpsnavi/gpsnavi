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

#define	CC_PKG_CHKTAMPERING_FILE_NAME					"pkg_file_list.xml"		// 改ざんチェックリストファイル名

#define	CC_PKG_CHKTAMPERING_XML_NODE_PKG_LIST			"pkg_list"
#define	CC_PKG_CHKTAMPERING_XML_NODE_PKG_LIST_ITEM		"item"
#define	CC_PKG_CHKTAMPERING_XML_NODE_VERSION			"version"
#define	CC_PKG_CHKTAMPERING_XML_NODE_FILE_NUM			"pkg_file_num"
#define	CC_PKG_CHKTAMPERING_XML_NODE_FILE_LIST			"pkg_file_list"
#define	CC_PKG_CHKTAMPERING_XML_NODE_FILE_LIST_ITEM		"item"
#define	CC_PKG_CHKTAMPERING_XML_NODE_FILE_NAME			"pkg_file_name"
#define	CC_PKG_CHKTAMPERING_XML_NODE_FILE_MD5			"pkg_file_md5"

#define	CC_PKG_CHKTAMPERING_XML_DATA_VERSION_SIZE		CC_CMN_PKGMGR_VERSION
#define	CC_PKG_CHKTAMPERING_XML_DATA_FILE_NUM_SIZE		11
#define	CC_PKG_CHKTAMPERING_XML_DATA_FILE_NAME_SIZE		SCC_MAX_PATH
#define	CC_PKG_CHKTAMPERING_XML_DATA_FILE_MD5_SIZE		32

// 改ざんチェックリストXMLパーサ
typedef struct _CHKTAMPERING_PARSER {
	INT32			state;
	Char			*buf;
	const Char		*chkVersion;
	Char			version[CC_CMN_PKGMGR_VERSION];
	INT32			*chkTamperingListNum;
	SMCHKTAMPERING	*chkTamperingList;
	INT32			*tmpChkTamperingListNum;
	SMCHKTAMPERING	*tmpChkTamperingList;
} CHKTAMPERING_PARSER;

enum PkgChkTamperingStatus {
	CC_PKG_CHKTAMPERING_NODE_NONE = 0,
	CC_PKG_CHKTAMPERING_NODE_XML,
	CC_PKG_CHKTAMPERING_NODE_XML_CHILD,
	CC_PKG_CHKTAMPERING_NODE_PKG_LIST,
	CC_PKG_CHKTAMPERING_NODE_PKG_LIST_CHILD,
	CC_PKG_CHKTAMPERING_NODE_PKG_LIST_ITEM,
	CC_PKG_CHKTAMPERING_NODE_PKG_LIST_ITEM_CHILD,
	CC_PKG_CHKTAMPERING_NODE_VERSION,
	CC_PKG_CHKTAMPERING_NODE_FILE_NUM,
	CC_PKG_CHKTAMPERING_NODE_FILE_LIST,
	CC_PKG_CHKTAMPERING_NODE_FILE_LIST_CHILD,
	CC_PKG_CHKTAMPERING_NODE_FILE_LIST_ITEM,
	CC_PKG_CHKTAMPERING_NODE_FILE_LIST_ITEM_CHILD,
	CC_PKG_CHKTAMPERING_NODE_FILE_NAME,
	CC_PKG_CHKTAMPERING_NODE_FILE_MD5
};

//------------------------------------------------
// 変数定義
//------------------------------------------------
static INT32	CB_Result;				// 処理結果 (コールバック関数用)

//------------------------------------------------
// 関数定義
//------------------------------------------------
static E_SC_RESULT CC_Pkg_ChkTampering_XmlParse(const Char *xmlFilePath, const Char *version, SMCHKTAMPERING *pkgChkTamperingList, INT32 *pkgChkTamperingListNum);
static void XMLCALL CC_Pkg_ChkTampering_StartElement(void *userData, const char *name, const char **atts);
static void XMLCALL CC_Pkg_ChkTampering_EndElement(void *userData, const char *name);
static void XMLCALL CC_Pkg_ChkTampering_CharacterData(void *userData, const XML_Char *data, INT32 len);
static E_SC_RESULT CC_Pkg_ChkTamperingMd5(const Char *pkgDirPath, SMCHKTAMPERING *pkgChkTamperingList, INT32 pkgChkTamperingListNum);

/**
 * @brief パッケージ改ざんチェック
 * @param [IN] smcal        SMCAL
 * @param [IN] parm         APIパラメータ
 * @param [IN] pkgInfo      パッケージ情報
 * @param [IN] pkgDirPath   パッケージディレクトリのフルパス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_Pkg_ChkTampering(SMCAL *smcal,
								T_CC_CMN_SMS_API_PRM *parm,
								const SMPACKAGEINFO *pkgInfo,
								const Char *pkgDirPath)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	//E_SC_CAL_RESULT	calRet = e_SC_CAL_RESULT_SUCCESS;
	Char	*tempDirPath = NULL;
	Char	*xmlFilePath = NULL;
	INT32	pkgChkTamperingListNum = -1;
	SMCHKTAMPERING	*pkgChkTamperingList =  NULL;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// メモリ確保
		pkgChkTamperingList = (SMCHKTAMPERING*)SCC_MALLOC(sizeof(SMCHKTAMPERING) * CC_CMN_PKGCMN_CHKTAMPERINGLIST_FILE_MAXNUM);
		if (NULL == pkgChkTamperingList) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		memset(pkgChkTamperingList, 0, (sizeof(SMCHKTAMPERING) * CC_CMN_PKGCMN_CHKTAMPERINGLIST_FILE_MAXNUM));

		tempDirPath = (Char*)SCC_MALLOC(SCC_MAX_PATH);
		if (NULL == tempDirPath) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		// テンポラリディレクトリのパス取得
		CC_GetTempDirPath(tempDirPath);

		// ダウンロードする改ざんチェックリストファイル名を設定
		strcpy((char*)pkgInfo->packageFileName, CC_PKG_CHKTAMPERING_FILE_NAME);

		// メモリ確保
		xmlFilePath = (Char*)SCC_MALLOC(SCC_MAX_PATH);
		if (NULL == xmlFilePath) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// ダウンロードした改ざんチェックリストファイルのパスを設定
		sprintf(xmlFilePath, "%s%s", tempDirPath, CC_PKG_CHKTAMPERING_FILE_NAME);

		// 改ざんリストダウンロード
		ret = CC_DownloadPackage(smcal, parm, PKGDLDATATYPE_OTHER, pkgInfo, tempDirPath);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_DownloadPackage error, " HERE);
			break;
		}

		// XML解析
		ret = CC_Pkg_ChkTampering_XmlParse(xmlFilePath, pkgInfo->version, pkgChkTamperingList, &pkgChkTamperingListNum);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_Pkg_ChkTampering_XmlParse error, " HERE);
			break;
		}

		if (0 > pkgChkTamperingListNum) {
			// 対象バージョンのパッケージ情報がない => パッケージの更新必須
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"package need update, " HERE);
			ret = e_SC_RESULT_NEDD_PKG_UPDATE;
			break;
		} else if (0 == pkgChkTamperingListNum) {
			// チェック対象なし（チェック不要）
			break;
		}

		// 改ざんチェック
		ret = CC_Pkg_ChkTamperingMd5(pkgDirPath, pkgChkTamperingList, pkgChkTamperingListNum);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_Pkg_ChkTamperingMd5 error, " HERE);
			break;
		}
	} while (0);

	// メモリ解放
	if (NULL != pkgChkTamperingList) {
		SCC_FREE(pkgChkTamperingList);
	}
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
 * @param [IN] xmlFilePath              XMLファイルのフルパス
 * @param [IN] version                  バージョン
 * @param [IN] pkgChkTamperingList      改ざんチェックリスト
 * @param [IN] pkgChkTamperingListNum   改ざんチェックリスト数
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_Pkg_ChkTampering_XmlParse(const Char *xmlFilePath,
										 const Char *version,
										 SMCHKTAMPERING *pkgChkTamperingList,
										 INT32 *pkgChkTamperingListNum)
{
	E_SC_RESULT		ret = e_SC_RESULT_SUCCESS;
	CHKTAMPERING_PARSER	listParser = {};
	Char buf[CC_CMN_XML_PARSE_DATA_SIZE + 1] = {};
	XML_Parser parser = NULL;
	INT32	done = 0;
	INT32	len = 0;
	//INT32	parsedLen = 0;
	FILE	*fp = NULL;
	INT32	tmpPkgChkTamperingListNum = 0;
	SMCHKTAMPERING	*tmpPkgChkTamperingList =  NULL;

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
		tmpPkgChkTamperingList = (SMCHKTAMPERING*)SCC_MALLOC(sizeof(SMCHKTAMPERING) * CC_CMN_PKGCMN_CHKTAMPERINGLIST_FILE_MAXNUM);
		if (NULL == tmpPkgChkTamperingList) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		memset(tmpPkgChkTamperingList, 0, (sizeof(SMCHKTAMPERING) * CC_CMN_PKGCMN_CHKTAMPERINGLIST_FILE_MAXNUM));
		listParser.chkVersion = version;
		listParser.version[0] = EOS;
		listParser.chkTamperingList = pkgChkTamperingList;
		listParser.chkTamperingListNum = pkgChkTamperingListNum;
		listParser.tmpChkTamperingList = tmpPkgChkTamperingList;
		listParser.tmpChkTamperingListNum = &tmpPkgChkTamperingListNum;

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
		XML_SetElementHandler(parser, CC_Pkg_ChkTampering_StartElement, CC_Pkg_ChkTampering_EndElement);
		XML_SetCharacterDataHandler(parser, CC_Pkg_ChkTampering_CharacterData);

		// XMLファイルオープン
		fp = fopen((char*)xmlFilePath, "r");
		if (NULL == fp) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"file open error(%d)[%s], " HERE, errno, xmlFilePath);
			CB_Result = e_SC_RESULT_FILE_OPENERR;
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
	if (NULL != tmpPkgChkTamperingList) {
		SCC_FREE(tmpPkgChkTamperingList);
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
void XMLCALL CC_Pkg_ChkTampering_StartElement(void *userData, const char *name, const char **atts)
{
	CHKTAMPERING_PARSER *parser = (CHKTAMPERING_PARSER*)userData;

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

		if (0 < *parser->chkTamperingListNum) {
			// 必要なデータは取得済み
			break;
		}

		// 初期化
		memset(parser->buf, 0, (CC_CMN_XML_BUF_SIZE + 1));

		if (0 == strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_XML)) {
			// <xml>
			parser->state = CC_PKG_CHKTAMPERING_NODE_XML;
		}

		// <xml>
		if (CC_PKG_CHKTAMPERING_NODE_XML == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_PKG_CHKTAMPERING_XML_NODE_PKG_LIST)) {
				// <pkg_list>
				parser->state = CC_PKG_CHKTAMPERING_NODE_PKG_LIST;
			} else if (0 != strcmp((char*)name, (char*)CC_CMN_XML_NODE_NAME_XML)) {
				parser->state = CC_PKG_CHKTAMPERING_NODE_XML_CHILD;
			}
		} else if (CC_PKG_CHKTAMPERING_NODE_PKG_LIST == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_PKG_CHKTAMPERING_XML_NODE_FILE_LIST_ITEM)) {
				// <item>
				*parser->tmpChkTamperingListNum = 0;
				memset(parser->tmpChkTamperingList, 0, (sizeof(SMCHKTAMPERING) * CC_CMN_PKGCMN_CHKTAMPERINGLIST_FILE_MAXNUM));
				parser->state = CC_PKG_CHKTAMPERING_NODE_PKG_LIST_ITEM;
			} else {
				parser->state = CC_PKG_CHKTAMPERING_NODE_PKG_LIST_CHILD;
			}
		} else if (CC_PKG_CHKTAMPERING_NODE_PKG_LIST_ITEM == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_PKG_CHKTAMPERING_XML_NODE_VERSION)) {
				// <version>
				parser->state = CC_PKG_CHKTAMPERING_NODE_VERSION;
			} else if (0 == strcmp((char*)name, (char*)CC_PKG_CHKTAMPERING_XML_NODE_FILE_NUM)) {
				// <pkg_file_num>
				parser->state = CC_PKG_CHKTAMPERING_NODE_FILE_NUM;
			} else if (0 == strcmp((char*)name, (char*)CC_PKG_CHKTAMPERING_XML_NODE_FILE_LIST)) {
				// <pkg_file_list>
				parser->state = CC_PKG_CHKTAMPERING_NODE_FILE_LIST;
			} else {
				parser->state = CC_PKG_CHKTAMPERING_NODE_PKG_LIST_ITEM_CHILD;
			}
		} else if (CC_PKG_CHKTAMPERING_NODE_FILE_LIST == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_PKG_CHKTAMPERING_XML_NODE_FILE_LIST_ITEM)) {
				// <item>
				if (CC_CMN_PKGCMN_CHKTAMPERINGLIST_FILE_MAXNUM <= *parser->tmpChkTamperingListNum) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"item maxnum over, " HERE);
					CB_Result = e_SC_RESULT_DATA_ERR;
					break;
				}
				parser->state = CC_PKG_CHKTAMPERING_NODE_FILE_LIST_ITEM;
			} else {
				parser->state = CC_PKG_CHKTAMPERING_NODE_FILE_LIST_CHILD;
			}
		} else if (CC_PKG_CHKTAMPERING_NODE_FILE_LIST_ITEM == parser->state) {
			if (0 == strcmp((char*)name, (char*)CC_PKG_CHKTAMPERING_XML_NODE_FILE_NAME)) {
				// <pkg_file_name>
				parser->state = CC_PKG_CHKTAMPERING_NODE_FILE_NAME;
			} else if (0 == strcmp((char*)name, (char*)CC_PKG_CHKTAMPERING_XML_NODE_FILE_MD5)) {
				// <pkg_file_md5>
				parser->state = CC_PKG_CHKTAMPERING_NODE_FILE_MD5;
			} else {
				parser->state = CC_PKG_CHKTAMPERING_NODE_FILE_LIST_ITEM_CHILD;
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
void XMLCALL CC_Pkg_ChkTampering_EndElement(void *userData, const char *name)
{
	CHKTAMPERING_PARSER *parser = (CHKTAMPERING_PARSER*)userData;
	SMCHKTAMPERING	*list = NULL;

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

		if (0 < *parser->chkTamperingListNum) {
			// 必要なデータは取得済み
			break;
		}

		list = &parser->tmpChkTamperingList[*parser->tmpChkTamperingListNum];

		if (CC_PKG_CHKTAMPERING_NODE_PKG_LIST == parser->state) {
			// <pkg_list>
			parser->state = CC_PKG_CHKTAMPERING_NODE_XML;
		} else if (CC_PKG_CHKTAMPERING_NODE_PKG_LIST_ITEM == parser->state) {
			// <item>
			if (0 == strcmp((char*)parser->chkVersion, (char*)parser->version)) {
				*parser->chkTamperingListNum = *parser->tmpChkTamperingListNum;
				memcpy(parser->chkTamperingList, parser->tmpChkTamperingList, (sizeof(SMCHKTAMPERING) * CC_CMN_PKGCMN_CHKTAMPERINGLIST_FILE_MAXNUM));
			}
			parser->state = CC_PKG_CHKTAMPERING_NODE_PKG_LIST;
		} else if (CC_PKG_CHKTAMPERING_NODE_VERSION == parser->state) {
			// <version>
			strcpy((char*)parser->version, (char*)parser->buf);
			parser->state = CC_PKG_CHKTAMPERING_NODE_PKG_LIST_ITEM;
		} else if (CC_PKG_CHKTAMPERING_NODE_FILE_NUM == parser->state) {
			// <pkg_file_num>
			parser->state = CC_PKG_CHKTAMPERING_NODE_PKG_LIST_ITEM;
		} else if (CC_PKG_CHKTAMPERING_NODE_FILE_LIST == parser->state) {
			// <pkg_file_list>
			parser->state = CC_PKG_CHKTAMPERING_NODE_PKG_LIST_ITEM;
		} else if (CC_PKG_CHKTAMPERING_NODE_FILE_LIST_ITEM == parser->state) {
			// <item>
			parser->state = CC_PKG_CHKTAMPERING_NODE_FILE_LIST;
			if (CC_CMN_PKGCMN_CHKTAMPERINGLIST_FILE_MAXNUM > *parser->tmpChkTamperingListNum) {
				(*parser->tmpChkTamperingListNum)++;
			}
		} else if (CC_PKG_CHKTAMPERING_NODE_FILE_NAME == parser->state) {
			// <pkg_file_name>
			if (CC_CMN_PKGCMN_CHKTAMPERINGLIST_FILE_MAXNUM > *parser->tmpChkTamperingListNum) {
				strcpy((char*)list->fileName, (char*)parser->buf);
			}
			parser->state = CC_PKG_CHKTAMPERING_NODE_FILE_LIST_ITEM;
		} else if (CC_PKG_CHKTAMPERING_NODE_FILE_MD5 == parser->state) {
			// <pkg_file_md5>
			if (CC_CMN_PKGCMN_CHKTAMPERINGLIST_FILE_MAXNUM > *parser->tmpChkTamperingListNum) {
				// 16進数文字列をバイトの16進数コードの文字列に変換
				if (e_SC_RESULT_SUCCESS != CC_ChgHexString(parser->buf, strlen((char*)parser->buf), list->md5)) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ChgHexString error, " HERE);
					CB_Result = e_SC_RESULT_FAIL;
					break;
				}
			}
			parser->state = CC_PKG_CHKTAMPERING_NODE_FILE_LIST_ITEM;
		} else if (CC_PKG_CHKTAMPERING_NODE_XML_CHILD == parser->state) {
			parser->state = CC_PKG_CHKTAMPERING_NODE_XML;
		} else if (CC_PKG_CHKTAMPERING_NODE_PKG_LIST_CHILD == parser->state) {
			parser->state = CC_PKG_CHKTAMPERING_NODE_PKG_LIST;
		} else if (CC_PKG_CHKTAMPERING_NODE_PKG_LIST_ITEM_CHILD == parser->state) {
			parser->state = CC_PKG_CHKTAMPERING_NODE_PKG_LIST_ITEM;
		} else if (CC_PKG_CHKTAMPERING_NODE_FILE_LIST_CHILD == parser->state) {
			parser->state = CC_PKG_CHKTAMPERING_NODE_FILE_LIST;
		} else if (CC_PKG_CHKTAMPERING_NODE_FILE_LIST_ITEM_CHILD == parser->state) {
			parser->state = CC_PKG_CHKTAMPERING_NODE_FILE_LIST_ITEM;
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
void XMLCALL CC_Pkg_ChkTampering_CharacterData(void *userData, const XML_Char *data, INT32 len)
{
	CHKTAMPERING_PARSER *parser = (CHKTAMPERING_PARSER*)userData;
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

		if (0 < *parser->chkTamperingListNum) {
			// 必要なデータは取得済み
			break;
		}

		// データをバッファにコピー
		bufLen = strlen((char*)parser->buf);

		if (CC_PKG_CHKTAMPERING_NODE_PKG_LIST == parser->state) {
			// <pkg_list>
		} else if (CC_PKG_CHKTAMPERING_NODE_PKG_LIST_ITEM == parser->state) {
			// <item>
		} else if (CC_PKG_CHKTAMPERING_NODE_VERSION == parser->state) {
			// <version>
			if (CC_PKG_CHKTAMPERING_XML_DATA_VERSION_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_PKG_CHKTAMPERING_NODE_FILE_NUM == parser->state) {
			// <api_num>
			if (CC_PKG_CHKTAMPERING_XML_DATA_FILE_NUM_SIZE > (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_PKG_CHKTAMPERING_NODE_FILE_LIST == parser->state) {
			// <api_list>
		} else if (CC_PKG_CHKTAMPERING_NODE_FILE_LIST_ITEM == parser->state) {
			// <item>
		} else if (CC_PKG_CHKTAMPERING_NODE_FILE_NAME == parser->state) {
			// <pkg_file_name>
			if (CC_PKG_CHKTAMPERING_XML_DATA_FILE_NAME_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		} else if (CC_PKG_CHKTAMPERING_NODE_FILE_MD5 == parser->state) {
			// <pkg_file_md5>
			if (CC_PKG_CHKTAMPERING_XML_DATA_FILE_MD5_SIZE >= (bufLen + len)) {
				memcpy(&parser->buf[bufLen], data, len);
				parser->buf[bufLen + len] = EOS;
			}
		}
	} while (0);

//	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}

/**
 * @brief パッケージ改ざんチェック(MD5)
 * @param [IN] pkgDirPath   パッケージディレクトリのフルパス
 * @param [IN] pkgChkTamperingList      改ざんチェックリスト
 * @param [IN] pkgChkTamperingListNum   改ざんチェックリスト数
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_Pkg_ChkTamperingMd5(const Char *pkgDirPath,
								   SMCHKTAMPERING *pkgChkTamperingList,
								   INT32 pkgChkTamperingListNum)
{
	E_SC_RESULT	ret = e_SC_RESULT_DATA_ERR;
	Char		*filePath = NULL;
	Char		*fileName = NULL;
	INT32		num = 0;


	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// メモリ確保
		filePath = (Char*)SCC_MALLOC(SCC_MAX_PATH);
		if (NULL == filePath) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// パッケージディレクトリのフルパスをコピー
		strcpy(filePath, pkgDirPath);
		if ('/' != filePath[strlen((char*)filePath) - 1]) {
			strcat(filePath, "/");
		}
		// パッケージファイル名の格納位置の先頭ポインタ
		fileName = filePath + strlen(filePath);

		for (num = 0; num < pkgChkTamperingListNum; num++) {
			// パッケージファイル名コピー
			strcpy(fileName, pkgChkTamperingList[num].fileName);

			// MD5チェック(ファイルサイズはチェックしないので0固定)
			ret = CC_CmnDL_CheckFile(filePath, pkgChkTamperingList[num].md5, 0);
			if (e_SC_RESULT_SUCCESS != ret) {
				ret = e_SC_RESULT_DATA_ERR;
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_CmnDL_CheckFile error[%s], " HERE, fileName);
				break;
			}
		}
		if (e_SC_RESULT_SUCCESS != ret) {
			break;
		}
	} while (0);

	// メモリ解放
	if (NULL != filePath) {
		SCC_FREE(filePath);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}
