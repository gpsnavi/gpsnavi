/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

//------------------------------------------------------------------------------------------------------------------------
//
//	SMCCom_CmnCtrl：センタ通信制御
//
//------------------------------------------------------------------------------------------------------------------------
//#define MAP_DL_ENABLE

//---------------------------------------------------------------------------------
//インクルード
//---------------------------------------------------------------------------------
#include "SMCComInternal.h"

#define CC_USERSRCH_ONLY_FOLLOWER		"follower"
#define CC_USERSRCH_ONLY_FOLLOWEE		"followee"
#define CC_USERSRCH_ORDER_NEWEST		"newest"
#define CC_USERSRCH_ONLY_LAST_ACTION	"last_action"
#define CC_USERSRCH_ONLY_GUID			"guid"
#define CC_ROOMSRCH_ORDER_LAST_UPDATE	"last_update"
#define CC_ROOMSRCH_ORDER_ROOM_NAME		"room_name"
#define CC_ROOMSRCH_PRIORITY_UNREAD		"unread"

#define	CC_ROOMSRCH_RES_FILE			"room.srch"
#define	CC_ROOMSRCH_RES_FILE_POLLING	"polling_room.srch"
#define	CC_POSINFO_RES_FILE				"pinfo.srch"
#define	CC_POSINFO_RES_FILE_POLLING		"polling_pinfo.srch"
#define	CC_USERSRCH_RES_FILE			"user.srch"
#define	CC_USERSRCH_RES_FILE_POLLING	"polling_user.srch"
#define	CC_CHATHIS_RES_FILE				"chat_his.xml"
#define	CC_CHATHIS_RES_FILE_POLLING		"polling_chat_his.xml"
#define	CC_SRCHUSER_RES_FILE			"user.info"

#define	CC_LOG_FILE						"SCCom.log"

//---------------------------------------------------------------------------------
//プロトタイプ宣言
//---------------------------------------------------------------------------------
E_SC_RESULT CC_SMS_API_initSetPrm(const Char* pTerm_id, const Char* pItem_id);				//SMS API関連パラメタテーブル初期設定処理
E_SC_RESULT CC_SaveUserInfo(const Char* pUsername, const Char* pMail, const Char* pId, const Char* pPassword, const Char* pLang);	//ユーザー情報退避処理
void CC_SaveUserInfo_Auth(const Char* pId, const Char* pPassword);	//ユーザー情報退避処理(認証用)
E_SC_RESULT CC_Control_Token_RegistDevice(SMCAL* pCal, T_CC_CMN_SMS_API_PRM* pPrm, Char* pRecvBuf, UINT32 RecvBuf_sz, Char* pApiSts, Bool isPolling);	//トークン／端末登録制御処理
E_SC_RESULT CC_ReAuthenticationUser(SMCAL* pCal, const Char* pId, const Char* pPassword, Char* pRecv_Buf, UINT32 RecvBuf_sz, Char* pApiSts, Bool isPolling);	//ユーザー再認証処理
E_SC_RESULT CC_ReadUserInfoFile();												//ユーザ情報ファイル読出し
E_SC_RESULT CC_SaveUserInfoFile();												//ユーザ情報ファイル退避
void CC_ProgressCallback(const SMDLPROGRESS *progress);
E_SC_RESULT CC_ChangeMapDBPath(SMUPDINFO *mapUpdInfo);
E_SC_RESULT CC_MoveFileForDL(const Char *tempDirPath, const Char *oldDataDir);
E_SC_RESULT CC_UpdateData_ComebackCheck(SMCAL *cal, const SMMAPUPDSTATUS *updStatus, const Char *dlTempDirPath, Bool *errComeback);
E_SC_RESULT CC_GetComBuff(SMCAL **cal, Char **buff, INT32 *buffSize);
void CC_FreeComBuff(const SMCAL *cal);
Bool CC_GetPackageInfo(const Char *pkgFileName, SMPACKAGEINFO *pkgInfo);
Bool CC_PkgCmn_GetPackageInfo(const Char *pkgName, const Char *pkgVer, SMPACKAGEINFO *pkgInfo);

//---------------------------------------------------------------------------------
//変数定義
//---------------------------------------------------------------------------------
static T_CC_CMN_SMS_API_PRM	SmsApi_prm_t = {};		//SMS API関連パラメタ構造体
static T_CC_CMN_SMS_API_PRM	tmpSmsApi_prm_t = {};		//SMS API関連パラメタ構造体
static Char* pRecvBuf[CC_CMN_API_CONCURRENCY_MAXNUM] = {NULL};	//受信バッファ格納先ポインタ
#if 0	// アラート機能無効
static Char* pAlert_RecvBuf = NULL;					//アラート関連機能用受信バッファ格納先ポインタ
static Char* pAnalyze_RecvBuf = NULL;				//HIF画像解析用受信バッファ格納先ポインタ
#endif	// アラート機能無効
static Char* pGcm_RecvBuf = NULL;					//GCM用受信バッファ格納先ポインタ
static SMCAL smcal[CC_CMN_API_CONCURRENCY_MAXNUM];
static Bool apiConcurrencyList[CC_CMN_API_CONCURRENCY_MAXNUM];
#if 0	// アラート機能無効
static SMCAL smcal_alert;							//アラート関連機能用
static SMCAL smcal_analyze;							//HIF画像解析用
#endif	// アラート機能無効
static SMCAL smcal_gcm;								//GCM関連機能用
static Char tempDirPath[SCC_MAX_PATH] = {};
static SMDLPROGRESS smDlProgress;
static SCC_MUTEX mutexAuthObj;
static SCC_MUTEX *mutexAuth;
static SCC_MUTEX mutexDLObj;
static SCC_MUTEX *mutexDL;
static SCC_MUTEX mutexAPIMngObj;
static SCC_MUTEX *mutexAPIMng;
static Char bkLoginId[CC_CMN_LOGINID_STR_SIZE];
static Char bkPassword[CC_CMN_PASSWORD_STR_SIZE];
static SMPACKAGEINFO_INTERNAL	pkgList[CC_CMN_PKGMGR_GROUP_MAXNUM * CC_CMN_PKGMGR_PACKAGE_MAXNUM];
static INT32 pkgListNum;
static UINT32	pkgWhiteListNum;
static SMPKGWHITELIST	pkgWhiteList[CC_CMN_PKGCMN_WHITELIST_API_MAXNUM];

//---------------------------------------------------------------------------------
//構造体定義
//---------------------------------------------------------------------------------
/**
* @brief 仕向け地コードISO-3166対応表
*
* @note  仕向け地が追加になった場合は、CC_CMN_REGION_NUMとRgn_ISO3166_tを更新すること
*/
typedef struct _T_CC_CMN_SMS_RGNCD_ISO3166 {
	INT32	iso3166_cd;							// ISO-3166コード
	Char	rgn_cd[SCC_MAPDWL_MAXCHAR_RGNCODE];	// 仕向け地コード文字列
} T_CC_CMN_SMS_RGNCD_ISO3166;

#define	CC_CMN_REGION_NUM			(1)			// 端末サポート仕向け地数(Rgn_ISO3166_tに定義された仕向け地の数)

static T_CC_CMN_SMS_RGNCD_ISO3166 Rgn_ISO3166_t[CC_CMN_REGION_NUM] = {
												{ 392, SCC_AREA_CODE_JPN }	// 日本
};

//---------------------------------------------------------------------------------
//外部関数
//---------------------------------------------------------------------------------
//************************************************
//通信制御初期化処理
//************************************************
/**
 * @brief 通信制御初期化
 * @param[in] pTerm_id 登録用TermID
 * @param[in] pItem_id 初期商品ID
 * @param[in] logDir ログ出力先ディレクトリ
 * @param[in] dataDir Dataフォルダパス(～/jp.co.hitachi.smsfv.aa/Data)
 * @param[in] configDir Configディレクトリパス(～/jp.co.hitachi.smsfv.aa/Map/JPN/Config)
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_OK以外 (コール関数の戻り値)
 * @note 通信開始時にコールすること。
 */
#ifdef __SMS_APPLE__
E_SC_RESULT CC_InitializeCmnCtrl(const Char* pTerm_id,
								 const Char* pItem_id,
								 const Char* logDir,
                                 const Char* rootDir,
								 const Char* dataDir,
								 const Char* configDir,
                                 const Char* oldDataDir)
#else
E_SC_RESULT CC_InitializeCmnCtrl(const Char* pTerm_id,
								 const Char* pItem_id,
								 const Char* logDir,
								 const Char* dataDir,
								 const Char* configDir,
                                 const Char* oldDataDir)
#endif /* __SMS_APPLE__ */
{
	E_SC_RESULT ret = CC_CMN_RESULT_OK;
	E_SC_CAL_RESULT calRet = CC_CMN_RESULT_SMCAL_OK;
	UINT32	len = 0;
	//Char	*srcFilePath = NULL;
	//Char	*dstFilePath = NULL;
	INT32	num = 0;
	Char	*logFile[CC_CMN_API_CONCURRENCY_MAXNUM] = {};

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// 初期化
		SCC_SetRootDirPath("\0");
		SCC_SetDataDirPath("\0");
		SCC_SetConfigDirPath("\0");
		//SCC_Logout();
		memset(smcal, 0, sizeof(smcal));
#if 0	// アラート機能無効
		memset(&smcal_alert, 0, sizeof(SMCAL));
		memset(&smcal_analyze, 0, sizeof(SMCAL));
#endif	// アラート機能無効
		memset(&smcal_gcm, 0, sizeof(SMCAL));
		for (num = 0; num < CC_CMN_API_CONCURRENCY_MAXNUM; num++) {
			pRecvBuf[num] = NULL;
		}
#if 0	// アラート機能無効
		pAlert_RecvBuf = NULL;
		pAnalyze_RecvBuf = NULL;
#endif	// アラート機能無効

		pGcm_RecvBuf = NULL;
		mutexAuth = NULL;
		mutexDL   = NULL;
		mutexAPIMng = NULL;
		memset(apiConcurrencyList, 0, sizeof(apiConcurrencyList));
		memset(pkgList, 0, sizeof(pkgList));
		pkgListNum = 0;

		//-------------------------------------------
		// パラメータチェック
		//-------------------------------------------
		if (CC_ISINVALIDSTR(pTerm_id)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_InitializeCmnCtrl() param error[pTerm_id], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		len = strlen(pTerm_id);
		if (CC_CMN_TERMID_STR_SIZE < len) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_InitializeCmnCtrl() param max size over[pTerm_id], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISINVALIDSTR(pItem_id)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_InitializeCmnCtrl() param error[pItem_id], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		len = strlen(pItem_id);
		if (CC_CMN_INIT_ITEMID_STR_SIZE < len) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_InitializeCmnCtrl() param max size over[pItem_id], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISINVALIDSTR(dataDir)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_InitializeCmnCtrl() param error[dataDir], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		len = strlen(dataDir);
		if (SCC_MAX_PATH < len) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_InitializeCmnCtrl() param max size over[dataDir], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (!CC_ISINVALIDSTR(configDir)) {
			len = strlen(configDir);
			if (SCC_MAX_PATH < len) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_InitializeCmnCtrl() param max size over[configDir], " HERE);
				ret = CC_CMN_RESULT_PARAM_ERR;
				break;
			}
		}

		// メモリ管理初期化
		ret = SCC_MEM_Initialize(SCC_MEM_SIZE_DYNAMIC, CC_MEM_TYPE_DYNAMIC);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "SCC_MEM_Initialize(DYNAMIC) error, " HERE);
			break;
		}
		ret = SCC_MEM_Initialize(SCC_MEM_SIZE_DYNAMIC2, CC_MEM_TYPE_DYNAMIC2);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "SCC_MEM_Initialize(DYNAMIC2) error, " HERE);
			break;
		}

		// Mutex生成
		ret = SCC_CreateMutex(&mutexAuthObj);
		if (CC_CMN_RESULT_OK != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_InitializeCmnCtrl() SCC_CreateMutex() error, " HERE);
			break;
		}
		mutexAuth = &mutexAuthObj;

		// Mutex生成
		ret = SCC_CreateMutex(&mutexDLObj);
		if (CC_CMN_RESULT_OK != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_InitializeCmnCtrl() SCC_CreateMutex() error, " HERE);
			break;
		}
		mutexDL = &mutexDLObj;

		// Mutex生成
		ret = SCC_CreateMutex(&mutexAPIMngObj);
		if (CC_CMN_RESULT_OK != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_InitializeCmnCtrl() SCC_CreateMutex() error, " HERE);
			break;
		}
		mutexAPIMng = &mutexAPIMngObj;

		// 各種ディレクトリパスを内部メモリに保持する
#ifdef __SMS_APPLE__
		SCC_SetRootDirPath(rootDir);
#else
		SCC_SetRootDirPath(dataDir);
#endif /* __SMS_APPLE__ */
		SCC_SetDataDirPath(dataDir);
		SCC_SetConfigDirPath(configDir);

		//-------------------------------------------
		// 受信バッファ確保
		//-------------------------------------------
		for (num = 0; num < CC_CMN_API_CONCURRENCY_MAXNUM; num++) {
			pRecvBuf[num] = (Char*)SCC_MALLOC(CC_CMN_RECIVE_BUFFER_SIZE);
			if(NULL == pRecvBuf[num]){									// メモリ確保失敗
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_InitializeCmnCtrl() SCC_MALLOC() error, " HERE);
				ret = CC_CMN_RESULT_MALLOC_ERR;
				break;
			}
		}
		if (CC_CMN_RESULT_OK != ret) {
			break;
		}
#if 0	// アラート機能無効
		// アラート関連機能用
		pAlert_RecvBuf = (Char*)SCC_MALLOC(CC_CMN_ALERT_RECIVE_BUFFER_SIZE);
		if(NULL == pAlert_RecvBuf){								// メモリ確保失敗
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_InitializeCmnCtrl() SCC_MALLOC() error, " HERE);
			ret = CC_CMN_RESULT_MALLOC_ERR;
			break;
		}
		// HIF画像解析用
		pAnalyze_RecvBuf = (Char*)SCC_MALLOC(CC_CMN_ANALYZE_RECIVE_BUFFER_SIZE);
		if(NULL == pAnalyze_RecvBuf){								// メモリ確保失敗
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_InitializeCmnCtrl() SCC_MALLOC() error, " HERE);
			ret = CC_CMN_RESULT_MALLOC_ERR;
			break;
		}
#endif	// アラート機能無効
		// GCM用
		pGcm_RecvBuf = (Char*)SCC_MALLOC(CC_CMN_GCM_RECIVE_BUFFER_SIZE);
		if(NULL == pGcm_RecvBuf){								// メモリ確保失敗
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_InitializeCmnCtrl() SCC_MALLOC() error, " HERE);
			ret = CC_CMN_RESULT_MALLOC_ERR;
			break;
		}

		//-------------------------------------------
		// センタ通信ライブラリ初期化
		//-------------------------------------------
		// OpenSSL初期化
		SC_CAL_Initialize_OpenSSL();
		for (num = 0; num < CC_CMN_API_CONCURRENCY_MAXNUM; num++) {
			logFile[num] = (Char*)SCC_MALLOC(SCC_MAX_PATH);
			if (NULL == logFile[num]) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
				ret = e_SC_RESULT_MALLOC_ERR;
				break;
			}
			sprintf(logFile[num], "%s%02d/%s", logDir, (num + 1), CC_LOG_FILE);
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_InitializeCmnCtrl() logFile=%s, " HERE, logFile[num]);
			// SMCAL初期化
			calRet = SC_CAL_Initialize(&smcal[num], logFile[num]);
			if (CC_CMN_RESULT_SMCAL_OK != calRet) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_InitializeCmnCtrl() SC_CAL_Initialize() error, " HERE);
				ret = CC_CMN_RESULT_PROC_ERR;
				break;
			}
		}
		if (CC_CMN_RESULT_OK != ret) {
			break;
		}
#if 0	// アラート機能無効
		// アラート関連機能用
		calRet = SC_CAL_Initialize(&smcal_alert, NULL);
		if (CC_CMN_RESULT_SMCAL_OK != calRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_InitializeCmnCtrl() SC_CAL_Initialize() error, " HERE);
			ret = CC_CMN_RESULT_PROC_ERR;
			break;
		}
		// HIF画像解析用
		calRet = SC_CAL_Initialize(&smcal_analyze, NULL);
		if (CC_CMN_RESULT_SMCAL_OK != calRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_InitializeCmnCtrl() SC_CAL_Initialize() error, " HERE);
			ret = CC_CMN_RESULT_PROC_ERR;
			break;
		}
#endif	// アラート機能無効
		// GCM関連機能用
		calRet = SC_CAL_Initialize(&smcal_gcm, NULL);
		if (CC_CMN_RESULT_SMCAL_OK != calRet) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_InitializeCmnCtrl() SC_CAL_Initialize() error, " HERE);
			ret = CC_CMN_RESULT_PROC_ERR;
			break;
		}

		//-------------------------------------------
		// テンポラリフォルダ作成
		//-------------------------------------------
		strcpy((char*)tempDirPath, (const char*)dataDir);
		if ('/' == tempDirPath[strlen((const char*)tempDirPath) - 1]) {
			sprintf(&tempDirPath[strlen((const char*)tempDirPath)], "%s", CC_CMN_TEMP_DIR_PATH);
		} else {
			sprintf(&tempDirPath[strlen((const char*)tempDirPath)], "/%s", CC_CMN_TEMP_DIR_PATH);
		}
		CC_MakeDir(tempDirPath);

		if ((!CC_ISINVALIDSTR(configDir)) && (!CC_ISINVALIDSTR(oldDataDir))) {
			// フォルダ未定時のDataフォルダ内の必要なファイルを、フォルダ決定時のDataフォルダに移動する
			ret = CC_MoveFileForDL(tempDirPath, oldDataDir);
			if (e_SC_RESULT_SUCCESS != ret) {
				break;
			}
		}

		//-------------------------------------------
		// SMS API関連パラメタテーブル初期設定
		//-------------------------------------------
		ret = CC_SMS_API_initSetPrm(pTerm_id, pItem_id);
		if(e_SC_RESULT_SUCCESS != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_InitializeCmnCtrl() CC_SMS_API_initSetPrm() error, " HERE);
			break;
		}

		// 地図ダウンロード進捗状況初期化
		CC_UpdDataProgressMng_Initialize();
	} while (0);

	// メモリ解放
	for (num = 0; num < CC_CMN_API_CONCURRENCY_MAXNUM; num++) {
		if (NULL != logFile[num]) {
			SCC_FREE(logFile[num]);
		}
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//通信制御終了処理
//************************************************
/**
 * @brief 通信制御終了
 * @note 通信終了時にコールすること。
 */
void CC_FinalizeCmnCtrl()
{
	E_SC_RESULT ret = CC_CMN_RESULT_OK;
	INT32	num = 0;

	LOG_PRINT_START((char*)SC_TAG_CC);

	//SCC_Logout();

	if (NULL != mutexAuth) {
		// Mutex破棄
		ret = SCC_DestroyMutex(mutexAuth);
		if (CC_CMN_RESULT_OK != ret) {
			LOG_PRINT_ERROR(SC_TAG_CC, (Char*)"[CmnCtrl] CC_FinalizeCmnCtrl() SCC_DestroyMutex() error, " HERE);
		}
		mutexAuth = NULL;
	}
	if (NULL != mutexDL) {
		// Mutex破棄
		ret = SCC_DestroyMutex(mutexDL);
		if (CC_CMN_RESULT_OK != ret) {
			LOG_PRINT_ERROR(SC_TAG_CC, (Char*)"[CmnCtrl] CC_FinalizeCmnCtrl() SCC_DestroyMutex() error, " HERE);
		}
		mutexDL = NULL;
	}
	if (NULL != mutexAPIMng) {
		// Mutex破棄
		ret = SCC_DestroyMutex(mutexAPIMng);
		if (CC_CMN_RESULT_OK != ret) {
			LOG_PRINT_ERROR(SC_TAG_CC, (Char*)"[CmnCtrl] CC_FinalizeCmnCtrl() SCC_DestroyMutex() error, " HERE);
		}
		mutexAPIMng = NULL;
	}

	//-------------------------------------------
	// 受信バッファ解放
	//-------------------------------------------
	for (num = 0; num < CC_CMN_API_CONCURRENCY_MAXNUM; num ++) {
		if(!CC_ISNULL(pRecvBuf[num])){
			SCC_FREE((void*)pRecvBuf[num]);				// 受信バッファ解放
		}
	}
#if	0	// アラート機能無効
	if (!CC_ISNULL(pAlert_RecvBuf)) {
		SCC_FREE((void*)pAlert_RecvBuf);		// アラート関連機能受信バッファ解放
	}
	if (!CC_ISNULL(pAnalyze_RecvBuf)) {
		SCC_FREE((void*)pAnalyze_RecvBuf);		// HIF画像解析受信バッファ解放
	}
#endif	// アラート機能無効
	if (!CC_ISNULL(pGcm_RecvBuf)) {
		SCC_FREE((void*)pGcm_RecvBuf);			// GCM受信バッファ解放
	}

	//-------------------------------------------
	// センタ通信ライブラリ終了化
	//-------------------------------------------
	for (num = 0; num < CC_CMN_API_CONCURRENCY_MAXNUM; num++) {
		SC_CAL_Finalize(&smcal[num]);
	}
#if	0	// アラート機能無効
	// アラート関連機能用
	SC_CAL_Finalize(&smcal_alert);
	// HIF画像解析用
	SC_CAL_Finalize(&smcal_analyze);
#endif	// アラート機能無効
	// GCM関連機能用
	SC_CAL_Finalize(&smcal_gcm);

	SCC_SetRootDirPath("\0");
	SCC_SetDataDirPath("\0");
	SCC_SetConfigDirPath("\0");

	// メモリ管理終了化
	ret = SCC_MEM_Finalize(CC_MEM_TYPE_DYNAMIC);
	if (e_SC_RESULT_SUCCESS != ret) {
		LOG_PRINT_ERROR(SC_TAG_CC, "SCC_MEM_Finalize(DYNAMIC) error, " HERE);
	}
	ret = SCC_MEM_Finalize(CC_MEM_TYPE_DYNAMIC2);
	if (e_SC_RESULT_SUCCESS != ret) {
		LOG_PRINT_ERROR(SC_TAG_CC, "SCC_MEM_Finalize(DYNAMIC2) error, " HERE);
	}

	LOG_PRINT_END((char*)SC_TAG_CC);
}

//************************************************
//Androidナビ端末登録処理
//************************************************
/**
 * @brief Androidナビ端末登録
 * @param[out] pApiSts ステータスコード
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_OK以外 (コール関数の戻り値)
 */
E_SC_RESULT CC_RegistrationDevice(Char* pApiSts)
{
	//INT32 optinfnum;
	E_SC_RESULT ret = CC_CMN_RESULT_OK;
	SMCAL	*cal = NULL;
	//Char	*recvBuf = NULL;
	//INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

#if 0	//******************************************************************************************************************************************
	do{

		//-------------------------------------------
		// パラメータチェック
		//-------------------------------------------
		if (CC_ISNULL(pApiSts)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_RegistrationDevice() param error[pApiSts], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_RegistrationDevice() CC_GetComBuff error, " HERE);
			break;
		}

		memset(recvBuf, 0, recvBufSize);			// 受信バッファクリア

		//-------------------------------------------
		//Token.req送信・受信処理
		//-------------------------------------------
		ret = CC_TokenReq_SendRecv(cal, &SmsApi_prm_t, recvBuf, recvBufSize, pApiSts);
		if(CC_CMN_RESULT_OK != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_RegistrationDevice() CC_TokenReq_SendRecv() error, " HERE);
			break;
		}

		memset(recvBuf, 0, recvBufSize);			// 受信バッファクリア

		//-------------------------------------------
		//Deviceid.req送信・受信処理
		//-------------------------------------------
		ret = CC_DeviceidReq_SendRecv(cal, &SmsApi_prm_t, recvBuf, recvBufSize, pApiSts);
		if(CC_CMN_RESULT_OK != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_RegistrationDevice() CC_DeviceidReq_SendRecv() error, " HERE);
			break;
		}

		memset(recvBuf, 0, recvBufSize);			// 受信バッファクリア

		//-------------------------------------------
		//Term.reg送信・受信処理
		//-------------------------------------------
		ret = CC_TermReg_SendRecv(cal, &SmsApi_prm_t, recvBuf, recvBufSize, pApiSts);
		if(CC_CMN_RESULT_OK != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_RegistrationDevice() CC_TermReg_SendRecv() error, " HERE);
			break;
		}

	}while(0);
#endif	//******************************************************************************************************************************************

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//ユーザー登録処理
//************************************************
/**
 * @brief ユーザー登録
 * @param[in] pUsername ユーザー名文字列ポインタ
 * @param[in] pMail メールアドレス文字列ポインタ
 * @param[in] pId ログインID文字列ポインタ
 * @param[in] pPassword パスワード文字列ポインタ
 * @param[in] pLang 言語設定文字列ポインタ
 * @param[out] pApiSts APIステータスコード
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_OK以外 (コール関数の戻り値)
 * @note TR3.5のアクティベーション対応で、out引数のGuidを削除
 */
E_SC_RESULT CC_RegistrationUser(const Char* pUsername,
								const Char* pMail,
								const Char* pId,
								const Char* pPassword,
								const Char* pLang,
								Char* pApiSts)
{
	//INT32 optinfnum;
	E_SC_RESULT ret = CC_CMN_RESULT_OK;
	UINT32	len = 0;
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do{

		//-------------------------------------------
		// パラメータチェック
		//-------------------------------------------
		if (CC_ISINVALIDSTR(pUsername)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_RegistrationUser() param error[pUsername], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		len = strlen((char*)pUsername);
		if (CC_CMN_USERNAME_STR_SIZE <= len) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_RegistrationUser() param max size over[strUsername], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISINVALIDSTR(pMail)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_RegistrationUser() param error[pMail], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		len = strlen((char*)pMail);
		if (CC_CMN_MALEADDR_STR_SIZE <= len) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_RegistrationUser() param max size over[pMail], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISINVALIDSTR(pId)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_RegistrationUser() param error[pId], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		len = strlen((char*)pId);
		if (CC_CMN_LOGINID_STR_SIZE <= len) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_RegistrationUser() param max size over[pId], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISINVALIDSTR(pPassword)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_RegistrationUser() param error[pPassword], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		len = strlen((char*)pPassword);
		if (CC_CMN_PASSWORD_STR_SIZE <= len) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_RegistrationUser() param max size over[pPassword], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISINVALIDSTR(pLang)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_RegistrationUser() param error[pLang], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		len = strlen((char*)pLang);
		if (CC_CMN_LANG_STR_SIZE <= len) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_RegistrationUser() param max size over[pLang], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISNULL(pApiSts)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_RegistrationUser() param error[pApiSts], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}

		// 初期化
		*pApiSts = EOS;

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_RegistrationUser() CC_GetComBuff error, " HERE);
			break;
		}

		memset(recvBuf, 0, recvBufSize);			// 受信バッファクリア

		//-------------------------------------------
		//ユーザー情報退避
		//-------------------------------------------
		ret = CC_SaveUserInfo(pUsername, pMail, pId, pPassword, pLang);
		if(CC_CMN_RESULT_OK != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_RegistrationUser() CC_SaveUserInfo() error, " HERE);
			break;
		}

		memset(recvBuf, 0, recvBufSize);			// 受信バッファクリア

		//-------------------------------------------
		//トークン／端末登録制御処理
		//-------------------------------------------
		ret = CC_Control_Token_RegistDevice(cal, &SmsApi_prm_t, recvBuf,  recvBufSize, pApiSts, false);
		if(CC_CMN_RESULT_OK != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_RegistrationUser() CC_Control_Token_RegistDevice() error, " HERE);
			break;
		}

		memset(recvBuf, 0, recvBufSize);			// 受信バッファクリア

		//-------------------------------------------
		///User.reg送信・受信処理
		//-------------------------------------------
		ret = CC_UserReg_SendRecv(cal, &SmsApi_prm_t, recvBuf, recvBufSize, pApiSts);
		if(CC_CMN_RESULT_OK != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_RegistrationUser() CC_UserReg_SendRecv() error, " HERE);
			break;
		}

		// ユーザ情報ファイル退避
		CC_SaveUserInfoFile();

	}while(0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//↓Activate.checkが無くなったため削除 2014.02.12
#if 0
//************************************************
//ユーザーアクティベーション処理
//************************************************
/**
 * @brief ユーザーアクティベーション
 * @param[out] pUsrMsg ユーザメッセージ(地図センタからのユーザ向けエラー文言)
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_OK以外 (コール関数の戻り値)
 * @pre パラメタチェックは上位層で既に行われているものとする。
 * @pre apTelInf,apTelOutは構造体のメンバも正しく設定済みであること。
 * @note DAL_VER.cppを使用しているため事前にセッタをコール済みであること。
 * @warning CDAL_Errorの初期化及び設定を本関数内で行う。
 */
E_SC_RESULT CC_CheckActivateUser(Char* pApiSts)
{
	INT32 optinfnum;
	E_SC_RESULT ret = CC_CMN_RESULT_OK;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do{
		//-------------------------------------------
		///Activate.check送信・受信処理
		//-------------------------------------------
		ret = CC_ActivateCheck_SendRecv(pApiSts);
		if(CC_CMN_RESULT_OK != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl]CC_CheckActivateUser() CC_ActivateCheck_SendRecv() error, " HERE);
			break;
		}
	}while(0);

	SCC_LOG_DebugPrint(DALCMN_LOGLV_NMR, (Char*)"[CmnCtrl]CC_CheckActivateUser() finish, " HERE);

	return (ret);
}
#endif	//#if 0
//↑Activate.checkが無くなったため削除 2014.02.12

//************************************************
//ユーザー認証処理
//************************************************
E_SC_RESULT CC_AuthenticationUser(const Char* pId,
								  const Char* pPassword,
								  const Char* pAppVer,
								  Char* pGuid,
								  Char* pLang,
								  Char* pPkgFlg,
								  Char* pApiSts)
{
	E_SC_RESULT ret = CC_CMN_RESULT_OK;
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;
	Char	appVer[32] = {};
	Char	*ptr = NULL;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISINVALIDSTR(pAppVer)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_AuthenticationUser() param error[pAppVer], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}

		// 端末アプリVer
		strcpy((char*)appVer, (char*)pAppVer);
		ptr = strrchr(appVer, '.');
		if (NULL == ptr) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_AuthenticationUser() param error[pAppVer], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		strncpy(SmsApi_prm_t.ApiPrmNavi.appVer, (ptr + 1), 8);
		strncpy(&SmsApi_prm_t.ApiPrmNavi.appVer[8], (ptr + 10), 2);
		SmsApi_prm_t.ApiPrmNavi.appVer[10] = EOS;

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (CC_CMN_RESULT_OK == ret) {
			// ユーザー認証
			ret = CC_AuthenticationUserInternal(cal, pId, pPassword, pGuid, pLang, pPkgFlg, recvBuf, recvBufSize, pApiSts, false);

			// 通信に必要なバッファ解放
			CC_FreeComBuff(cal);
		} else {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_AuthenticationUser() CC_GetComBuff error, " HERE);
		}
	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief ユーザー認証
 * @param[in] pId ログインID文字列ポインタ
 * @param[in] pPassword パスワード文字列ポインタ
 * @param[out] pGuidg  GUID
 * @param[out] pLang 現在ユーザが設定している言語設定
 * @param[out] pPkgFlg パッケージフラグ
 * @param[in] recvBuf 受信バッファ
 * @param[in] recvBuf_sz 受信バッファサイズ
 * @param[out] pApiSts APIステータスコード
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_OK以外 (コール関数の戻り値)
 */
E_SC_RESULT CC_AuthenticationUserInternal(SMCAL* pCal,
										  const Char* pId,
										  const Char* pPassword,
										  Char* pGuid,
										  Char* pLang,
										  Char* pPkgFlg,
										  Char* recvBuf,
										  UINT32 recvBuf_sz,
										  Char* pApiSts,
										  Bool isPolling)
{
	//INT32 optinfnum;
	E_SC_RESULT ret = CC_CMN_RESULT_OK;
	Char login_id[CC_CMN_LOGINID_STR_SIZE] = {};
	Char password[CC_CMN_PASSWORD_STR_SIZE] = {};
	UINT32	len = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do{

		//-------------------------------------------
		// パラメータチェック
		//-------------------------------------------
		if (CC_ISINVALIDSTR(pId)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_AuthenticationUser() param error[pId], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		len = strlen((char*)pId);
		if (CC_CMN_LOGINID_STR_SIZE <= len) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_AuthenticationUser() param max size over[pId], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISINVALIDSTR(pPassword)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_AuthenticationUser() param error[pPassword], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		len = strlen((char*)pPassword);
		if (CC_CMN_PASSWORD_STR_SIZE <= len) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_AuthenticationUser() param max size over[pPassword], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISNULL(pGuid)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_AuthenticationUser() param error[pGuid], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISNULL(pLang)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_AuthenticationUser() param error[pLang], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISNULL(pPkgFlg)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_AuthenticationUser() param error[pPkgFlg], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISNULL(pApiSts)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_AuthenticationUser() param error[pApiSts], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}

		//-------------------------------------------
		//ユーザ認証期限切れ時の再ユーザ認証時用の対策処理
		//-------------------------------------------
		strcpy(login_id, pId);				// ログインIDコピー
		strcpy(password, pPassword);		// パスワードコピー

		// 初期化
		*pApiSts = EOS;

		//-------------------------------------------
		//ユーザー情報退避(認証用)
		//-------------------------------------------
		CC_SaveUserInfo_Auth(login_id, password);

		memset(recvBuf, 0, recvBuf_sz);			// 受信バッファクリア

		//-------------------------------------------
		//トークン／端末登録制御処理
		//-------------------------------------------
		ret = CC_Control_Token_RegistDevice(pCal, &SmsApi_prm_t, recvBuf, recvBuf_sz, pApiSts, isPolling);
		if(CC_CMN_RESULT_OK != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_AuthenticationUser() CC_Control_Token_RegistDevice() error, " HERE);
			break;
		}

		memset(recvBuf, 0, recvBuf_sz);			// 受信バッファクリア
		SmsApi_prm_t.ApiPrmUser.policyLateFlg[0] = SCC_CMN_POLICY_LATE_FLG_NEW;

		//-------------------------------------------
		///Auth.req送信・受信処理
		//-------------------------------------------
		ret = CC_AuthReq_SendRecv(pCal, &SmsApi_prm_t, recvBuf, recvBuf_sz, pApiSts, isPolling);
		if(CC_CMN_RESULT_OK != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_AuthenticationUser() CC_AuthReq_SendRecv() error, " HERE);
			// OAuth連携のトークエラーの場合、戻り値を変更する
			if ((0 == strcmp(CC_CMN_API_OAUTH_TOKEN_INVALID, pApiSts)) ||
				(0 == strcmp(CC_CMN_API_OAUTH_TOKEN_EXPIRATION, pApiSts))) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_AuthenticationUser() oauth token error, " HERE);
				ret = e_SC_RESULT_OAUTH_TOKEN_ERROR;
			}
			break;
		}

		// アクティベーション状態が"未"の場合								// TR3.5 アクティベーション対応
		if(0 != strcmp(SmsApi_prm_t.ApiPrmMups.act_status, CC_CMN_ACTIVATION_OK)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_AuthenticationUser() no activationed, " HERE);
			ret = e_SC_RESULT_NONACTIVATION;
			break;
		}

		// 言語
		strcpy(pLang, SmsApi_prm_t.ApiPrmUser.lang);
		// パッケージフラグ
		sprintf(pPkgFlg, "%d", SmsApi_prm_t.ApiPrmUser.packageFlg);

		// 規約の再同意が必要な場合
		if (SCC_CMN_POLICY_LATE_FLG_NEW != SmsApi_prm_t.ApiPrmUser.policyLateFlg[0]) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_AuthenticationUser() need policy agree, " HERE);
			ret = e_SC_RESULT_POLICY_AGREE;
			break;
		}

		// ログイン状態にする
		SCC_Login();

		// GUID
		strcpy(pGuid, SmsApi_prm_t.ApiPrmMups.guid);

		// ユーザ情報ファイル退避
		CC_SaveUserInfoFile();

	}while(0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief 利用規約再同意と認証要求
 * @param[in] pId ログインID文字列ポインタ
 * @param[in] pPassword パスワード文字列ポインタ
 * @param[out] pGuidg  GUID
 * @param[out] pLang 現在ユーザが設定している言語設定
 * @param[out] pApiSts APIステータスコード
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_OK以外 (コール関数の戻り値)
 */
E_SC_RESULT CC_AgreeAuthenticationUser(const Char* pId,
									   const Char* pPassword,
									   Char* pGuid,
									   Char* pLang,
									   Char* pApiSts)

{
	//INT32 optinfnum;
	E_SC_RESULT ret = CC_CMN_RESULT_OK;
	Char login_id[CC_CMN_LOGINID_STR_SIZE] = {};
	Char password[CC_CMN_PASSWORD_STR_SIZE] = {};
	UINT32	len = 0;
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do{

		//-------------------------------------------
		// パラメータチェック
		//-------------------------------------------
		if (CC_ISINVALIDSTR(pId)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_AgreeAuthenticationUser() param error[pId], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		len = strlen((char*)pId);
		if (CC_CMN_LOGINID_STR_SIZE <= len) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_AgreeAuthenticationUser() param max size over[pId], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISINVALIDSTR(pPassword)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_AgreeAuthenticationUser() param error[pPassword], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		len = strlen((char*)pPassword);
		if (CC_CMN_PASSWORD_STR_SIZE <= len) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_AgreeAuthenticationUser() param max size over[pPassword], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISNULL(pLang)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_AgreeAuthenticationUser() param error[pLang], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISNULL(pApiSts)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_AgreeAuthenticationUser() param error[pApiSts], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		//-------------------------------------------
		//ユーザ認証期限切れ時の再ユーザ認証時用の対策処理
		//-------------------------------------------
		strcpy(login_id, pId);				// ログインIDコピー
		strcpy(password, pPassword);		// パスワードコピー

		// 初期化
		*pApiSts = EOS;

		//-------------------------------------------
		//ユーザー情報退避(認証用)
		//-------------------------------------------
		CC_SaveUserInfo_Auth(login_id, password);

		memset(recvBuf, 0, recvBufSize);			// 受信バッファクリア

		//-------------------------------------------
		//トークン／端末登録制御処理
		//-------------------------------------------
		ret = CC_Control_Token_RegistDevice(cal, &SmsApi_prm_t, recvBuf, recvBufSize, pApiSts, false);
		if(CC_CMN_RESULT_OK != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_AgreeAuthenticationUser() CC_Control_Token_RegistDevice() error, " HERE);
			break;
		}

		memset(recvBuf, 0, recvBufSize);			// 受信バッファクリア
		SmsApi_prm_t.ApiPrmUser.policyLateFlg[0] = SCC_CMN_POLICY_LATE_FLG_NEW;

		//-------------------------------------------
		///AgreeAuth.req送信・受信処理
		//-------------------------------------------
		ret = CC_AgreeAuthReq_SendRecv(cal, &SmsApi_prm_t, recvBuf, recvBufSize, pApiSts, false);
		if(CC_CMN_RESULT_OK != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_AgreeAuthenticationUser() CC_AuthReq_SendRecv() error, " HERE);
			break;
		}

		// アクティベーション状態が"未"の場合
		if(0 != strcmp(SmsApi_prm_t.ApiPrmMups.act_status, CC_CMN_ACTIVATION_OK)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_AgreeAuthenticationUser() no activationed, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 規約の再同意が必要な場合
		if (SCC_CMN_POLICY_LATE_FLG_NEW != SmsApi_prm_t.ApiPrmUser.policyLateFlg[0]) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_AgreeAuthenticationUser() need policy agree, " HERE);
			ret = e_SC_RESULT_POLICY_AGREE;
			break;
		}

		// ログイン状態にする
		SCC_Login();

		// GUID
		strcpy(pGuid, SmsApi_prm_t.ApiPrmMups.guid);
		// 言語
		strcpy(pLang, SmsApi_prm_t.ApiPrmUser.lang);

		// ユーザ情報ファイル退避
		CC_SaveUserInfoFile();

	}while(0);

	if (NULL != cal) {
		// 通信に必要なバッファ解放
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//GEM登録処理
//************************************************
/**
 * @brief GEM登録
 * @param[in/out] gem   GEM登録情報
 * @param[out] apiSts   APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GemRegenerate(SMGEMREG *gem, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISNULL(gem)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gem], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if ((!CC_ISINVALIDSTR(gem->text)) && (SCC_MAX_TEXT <= strlen(gem->text))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gem->text], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if ((!CC_ISINVALIDSTR(gem->pic)) && (SCC_MAX_PATH <= strlen(gem->pic))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gem->pic], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if ((!CC_ISINVALIDSTR(gem->circleGuid)) && (SCC_MAX_CIRCLE_GUID <= strlen(gem->circleGuid))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gem->circleGuid], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if ((CC_CMN_GEM_ACCESS_FLG_PRIVATE != gem->accessFlg) && (CC_CMN_GEM_ACCESS_FLG_PUBLIC != gem->accessFlg)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[accessFlg], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[pApiSts], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			gem->gemId[0] = EOS;
			gem->gemUrl[0] = EOS;
			*apiStatus = EOS;
			memset(recvBuf, 0, recvBufSize);

			// GEM登録
			ret = CC_GemReg_SendRecv(cal, &SmsApi_prm_t, gem, recvBuf, recvBufSize, apiStatus);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GemReg_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//GEM検索処理
//************************************************
/**
 * @brief GEM検索
 * @param[in/out] gem       GEM検索条件/検索結果
 * @param[out] gemNum       GEM検索条件/検索結果格納領域数/GEM情報数
 * @param[out]lastFlg       検索結果が最終位置か
 * @param[out]apiSts        APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GemSearch(const SMGEMSEARCH *gemSearch,
						 SMGEMINFO *gemInfo,
						 INT32 *gemInfoNum,
						 INT32 *lastFlg,
						 Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISNULL(gemSearch)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemSearch], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if ((!CC_ISINVALIDSTR(gemSearch->id)) && (SCC_MAX_ID <= strlen(gemSearch->id))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemSearch->id], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if ((!CC_ISINVALIDSTR(gemSearch->keyword)) && (SCC_MAX_KEYWORD <= strlen(gemSearch->keyword))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemSearch->keyword], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if ((0 >= gemSearch->maxCnt) || (SCC_GEM_MAXNUM < gemSearch->maxCnt)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemSearch->maxCnt], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if ((CC_CMN_GEM_ACCESS_FLG_PRIVATE != gemSearch->accessFlg) && (CC_CMN_GEM_ACCESS_FLG_PUBLIC != gemSearch->accessFlg)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemSearch->accessFlg], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if ((CC_CMN_GEM_SORT_NEW > gemSearch->sort) || (CC_CMN_GEM_SORT_DISTANCE < gemSearch->sort)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemSearch->sort], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (0 > gemSearch->radius) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemSearch->radius], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISNULL(gemInfo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemInfo], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISNULL(gemInfoNum)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemInfoNum], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISNULL(lastFlg)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[lastFlg], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			*gemInfoNum = 0;
			memset(gemInfo, 0, (sizeof(SMGEMINFO) * gemSearch->maxCnt));
			*apiStatus = EOS;
			memset(recvBuf, 0, recvBufSize);
			*lastFlg = 1;

			// GEM検索
			ret = CC_GemSearch_SendRecv(cal, &SmsApi_prm_t, gemSearch, gemInfo, gemInfoNum, lastFlg, recvBuf, recvBufSize, apiStatus);
			if (e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GemSearch_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief GEMタイムライン検索
 * @param[in/out] gem       GEMタイムライン検索条件/検索結果
 * @param[out] gemNum       GEMタイムライン検索条件/検索結果格納領域数/GEM情報数
 * @param[out]lastFlg       検索結果が最終位置か
 * @param[out]apiSts        APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GemTimelineSearch(const SMGEMTIMELINESEARCH *gemSearch,
								 SMGEMTIMELINEINFO *gemInfo,
								 INT32 *gemInfoNum,
								 INT32 *lastFlg,
								 Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISNULL(gemSearch)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemSearch], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if ((!CC_ISINVALIDSTR(gemSearch->id)) && (SCC_MAX_ID <= strlen(gemSearch->id))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemSearch->id], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if ((!CC_ISINVALIDSTR(gemSearch->keyword)) && (SCC_MAX_KEYWORD <= strlen(gemSearch->keyword))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemSearch->keyword], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if ((0 >= gemSearch->maxCnt) || (SCC_GEMTIMELINE_MAXNUM < gemSearch->maxCnt)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemSearch->maxCnt], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if ((CC_CMN_GEM_SORT_NEW > gemSearch->sort) || (CC_CMN_GEM_SORT_LIKE < gemSearch->sort)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemSearch->sort], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISNULL(gemInfo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemInfo], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISNULL(gemInfoNum)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemInfoNum], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISNULL(lastFlg)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[lastFlg], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			*gemInfoNum = 0;
			memset(gemInfo, 0, (sizeof(SMGEMINFO) * gemSearch->maxCnt));
			*apiStatus = EOS;
			memset(recvBuf, 0, recvBufSize);
			*lastFlg = 1;

			// GEMタイムライン検索
			ret = CC_GemTimelineSearch_SendRecv(cal,
												&SmsApi_prm_t,
												gemSearch,
												gemInfo,
												gemInfoNum,
												lastFlg,
												recvBuf,
												recvBufSize,
												apiStatus);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GemTimelineSearch_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief GEMID検索
 * @param[in]  gemId    GEMID
 * @param[out] gemInfo  GEM検索結果格納領域
 * @param[out] apiSts   APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GemIdSearch(const Char *gemId, SMGEMINFO *gemInfo, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISINVALIDSTR(gemId)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemId], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (SCC_MAX_ID <= strlen(gemId)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemId], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISNULL(gemInfo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemInfo], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			*apiStatus = EOS;
			memset(gemInfo, 0, sizeof(SMGEMINFO));
			memset(recvBuf, 0, recvBufSize);

			// GEM検索
			ret = CC_GemIdSearch_SendRecv(cal, &SmsApi_prm_t, gemId, gemInfo, recvBuf, recvBufSize, apiStatus);
			if (e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GemIdSearch_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//GEM更新処理
//************************************************
/**
 * @brief GEM更新
 * @param[in/out] gem   GEM更新情報
 * @param[out] apiSts   APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GemUpdate(SMGEMUPD *gem, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISNULL(gem)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gem], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISINVALIDSTR(gem->gemId) || (SCC_MAX_ID <= strlen(gem->gemId))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gem->gemId], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if ((CC_CMN_GEM_ACCESS_FLG_PRIVATE != gem->accessFlg) && (CC_CMN_GEM_ACCESS_FLG_PUBLIC != gem->accessFlg)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[gem->accessFlg], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if ((CC_CMN_GEM_PICTURE_NODEL != gem->pictureDel) && (CC_CMN_GEM_PICTURE_DEL != gem->pictureDel)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gem->picture_del], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[pApiSts], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			*apiStatus = EOS;
			memset(recvBuf, 0, recvBufSize);

			// GEM更新
			ret = CC_GemAlt_SendRecv(cal, &SmsApi_prm_t, gem, recvBuf, recvBufSize, apiStatus);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GemAlt_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//GEM削除処理
//************************************************
/**
 * @brief GEM削除
 * @param[in/out] gem   GEM削除情報
 * @param[out] apiSts   APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GemDelete(SMGEMDEL *gem, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISNULL(gem)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gem], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISINVALIDSTR(gem->gemId) || (SCC_MAX_ID <= strlen(gem->gemId))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gem->gemId], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[pApiSts], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			*apiStatus = EOS;
			memset(recvBuf, 0, recvBufSize);

			// GEM削除
			ret = CC_GemDel_SendRecv(cal, &SmsApi_prm_t, gem, recvBuf, recvBufSize, apiStatus);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GemDel_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//GEMにLikeをつける処理
//************************************************
E_SC_RESULT CC_GemLikeReg(const Char *gemId, LONG *likeCnt, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISINVALIDSTR(gemId)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemId], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (SCC_MAX_ID <= strlen(gemId)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemId], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISNULL(likeCnt)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[likeCnt], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			*apiStatus = EOS;
			memset(recvBuf, 0, recvBufSize);

			// GEMにLikeする
			ret = CC_GemLikeReg_SendRecv(cal, &SmsApi_prm_t, gemId, likeCnt, recvBuf, recvBufSize, apiStatus);
			if(CC_CMN_RESULT_OK != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GemLikeReg_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//GEMのLikeを取り消す処理
//************************************************
E_SC_RESULT CC_GemLikeCncl(const Char *gemId, LONG *likeCnt, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISINVALIDSTR(gemId)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemId], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (SCC_MAX_ID <= strlen(gemId)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemId], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISNULL(likeCnt)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[likeCnt], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			*apiStatus = EOS;
			memset(recvBuf, 0, recvBufSize);

			// GEMのLikeを取り消す
			ret = CC_GemLikeCncl_SendRecv(cal, &SmsApi_prm_t, gemId, likeCnt, recvBuf, recvBufSize, apiStatus);
			if(CC_CMN_RESULT_OK != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GemLikeCncl_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//規約取得処理
//************************************************
/**
 * @brief 規約取得
 * @param[out] termsInfo 規約情報構造体ポインタ
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_OK以外 (コール関数の戻り値)
 */
E_SC_RESULT CC_TremsApprovalInfo(SMTERMSINFO *termsInfo, Bool isDownload, const Char *lang)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	Char apiSts[CC_CMN_USER_MESSAGE_AREA_SIZE] = {};
	//INT16 result = 0;										// 結果
	Char pUserInfo[SCC_MAX_PATH] = {};
	struct stat st = {};
	int stat_ret = 0;
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {

		//-------------------------------------------
		// パラメータチェック
		//-------------------------------------------
		if (CC_ISNULL(termsInfo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_TremsApprovalInfo() param error[termsInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(lang)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_TremsApprovalInfo() param error[lang], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_CMN_LANG_STR_SIZE <= strlen((char*)lang)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_TremsApprovalInfo() param max size over[lang], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		//-------------------------------------------
		// 規約情報引数初期化
		//-------------------------------------------
		//termsInfo->isApproved = false;												// 規約承認済みフラグ：未承認
		memset(termsInfo->termsFileName, 0, SCC_MAX_PATH);							// 規約文章ファイルパス"0"クリア

		memset(recvBuf, 0, recvBufSize);								// 受信バッファクリア

		//-------------------------------------------
		// ユーザ規約承認状況チェック
		//-------------------------------------------
		// ユーザ規約文章ファイルフルパス文字列作成
		sprintf(pUserInfo, "%s%s_%s", tempDirPath, lang, CC_CMN_USERPOLICY_FILE_NAME);

		// ユーザ規約文章ファイル状態取得
		stat_ret = stat((const char*)pUserInfo, &st);

		// 規約ファイルのダウンロード指定あり または ユーザ規約文章ファイルが存在しない
		if ((isDownload) || (0 != stat_ret)) {
			// 通信に必要なバッファ取得
			ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
				break;
			}

			memset(recvBuf, 0, recvBufSize);

			//-------------------------------------------
			// Userpolicy.req送信・受信処理 (規約文章取得 & ファイル出力)
			//-------------------------------------------
			ret = CC_UserpolicyReq_SendRecv(cal, &SmsApi_prm_t, lang, recvBuf, recvBufSize, apiSts);
			if(CC_CMN_RESULT_OK != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_TremsApprovalInfo() CC_UserpolicyReq_SendRecv() error, " HERE);
				break;
			}
		}

		// ユーザ規約文章ファイル状態取得 (上で規約文章取得した時のための再チェック)
		stat_ret = stat((const char*)pUserInfo, &st);
		if(0 != stat_ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_TremsApprovalInfo() UserPolicy file error, " HERE);
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_TremsApprovalInfo() path=%s, " HERE, pUserInfo);
			ret = e_SC_RESULT_FILE_ACCESSERR;
			break;
		}

		// ユーザ規約文章ファイルフルパス設定
		strcpy((char*)termsInfo->termsFileName, (const char*)pUserInfo);

		if (isDownload) {
			strcpy(termsInfo->termsLang, SmsApi_prm_t.ApiPrmUser.policylang);
			strcpy(termsInfo->termsVersion, SmsApi_prm_t.ApiPrmUser.policyver);
		} else {
			termsInfo->termsLang[0] = EOS;
			termsInfo->termsVersion[0] = EOS;
		}
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//ユーザ情報取得処理
//************************************************
/**
 * @brief ユーザ情報取得
 * @param[out] userName ユーザ名
 * @param[out] id ユーザID
 * @param[out] password パスワード
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_OK以外 (コール関数の戻り値)
 */
E_SC_RESULT CC_GetUserInfo(Char *userName, Char *id, Char *password)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	//Char	pUserInfo[SCC_MAX_PATH];

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		if (CC_ISNULL(userName)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetUserInfo() param error[userName], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(id)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetUserInfo() param error[id], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(password)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetUserInfo() param error[password], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

#if 0	//******************************************************************************************************************   デバッグ用処理
		CC_ReadUserInfoFile();								// ユーザ情報ファイル読出し
#endif	//******************************************************************************************************************   デバッグ用処理

		// ログインID
		memset(id, 0, CC_CMN_LOGINID_STR_SIZE);
		strcpy((char*)id, (const char*)SmsApi_prm_t.ApiPrmUser.login_id);

		// パスワード
		memset(password, 0, CC_CMN_PASSWORD_STR_SIZE);
		strcpy((char*)password, (const char*)SmsApi_prm_t.ApiPrmUser.password);

#if 0	//******************************************************************************************************************   デバッグ用処理
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_GetUserInfo() login_id = %s ", id);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_GetUserInfo() password = %s ", password);
#endif	//******************************************************************************************************************   デバッグ用処理

	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//ユーザ属性取得処理
//************************************************
/**
 * @brief ユーザ属性取得
 * @param[in]  srchGuid 取得対象ユーザーID
 * @param[out] userInfo ユーザ属性
 * @param[out] apiStatus   APIステータス
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_OK以外 (コール関数の戻り値)
 */
E_SC_RESULT CC_GetUserAttrInfo(const Char *srchGuid, SMUSERPROFILE *userInfo, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		if ((!CC_ISINVALIDSTR(srchGuid)) && (CC_CMN_GUID_STR_SIZE <= strlen(srchGuid))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[srchGuid], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(userInfo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[userInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			*apiStatus = EOS;
			memset(userInfo, 0, sizeof(SMUSERPROFILE));
			memset(recvBuf, 0, recvBufSize);

			// ユーザー属性照会
			ret = CC_UserattrInfo_SendRecv(cal, &SmsApi_prm_t, srchGuid, userInfo, recvBuf, recvBufSize, apiStatus, false);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_UserattrInfo_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief ユーザ属性取得(ポーリング用)
 * @param[in]  srchGuid 取得対象ユーザーID
 * @param[out] userInfo ユーザ属性
 * @param[out] apiStatus   APIステータス
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_OK以外 (コール関数の戻り値)
 */
E_SC_RESULT CC_GetUserAttrInfo_Polling(const Char *srchGuid, SMUSERPROFILE *userInfo, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		if ((!CC_ISINVALIDSTR(srchGuid)) && (CC_CMN_GUID_STR_SIZE <= strlen(srchGuid))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[srchGuid], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(userInfo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[userInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			*apiStatus = EOS;
			memset(userInfo, 0, sizeof(SMUSERPROFILE));
			memset(recvBuf, 0, recvBufSize);

			// ユーザー属性照会
			ret = CC_UserattrInfo_SendRecv(cal, &SmsApi_prm_t, srchGuid, userInfo, recvBuf, recvBufSize, apiStatus, true);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_UserattrInfo_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, true);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//ユーザ検索処理
//************************************************
/**
 * @brief ユーザ検索
 * @param[in] SrchQuali 取得データの開始位置(1件目を0とする)
 * @param[out] BaseInf ユーザ基本情報構造体ポインタ
 * @param[in/out] userNum 取得ユーザ情報数(現状inは100とする)
 * @param[out] pApiSts APIステータスコード
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_OK以外 (コール関数の戻り値)
 */
E_SC_RESULT CC_SearchUser(const SMUSERSRCHQUALI *SrchQuali,
							SMUSERBASEINFO *BaseInf,
							INT32 *userNum,
							Char *pApiSts)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメタチェック
		if(CC_ISNULL(SrchQuali)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchUser() param error[SrchQuali], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if((!CC_ISINVALIDSTR(SrchQuali->pOnly)) &&
		   (0 != strcmp(CC_USERSRCH_ONLY_FOLLOWER, SrchQuali->pOnly)) &&
		   (0 != strcmp(CC_USERSRCH_ONLY_FOLLOWEE, SrchQuali->pOnly))){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchUser() param error[SrchQuali->pOnly], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(0 > SrchQuali->offset){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchUser() param error[SrchQuali->offset], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if((0 >= SrchQuali->limit) || (SCC_CHAT_MAXUSER_NUM < SrchQuali->limit)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchUser() param error[SrchQuali->limit], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if((!CC_ISINVALIDSTR(SrchQuali->pOrder)) &&
		   (0 != strcmp(CC_USERSRCH_ORDER_NEWEST, SrchQuali->pOrder)) &&
		   (0 != strcmp(CC_USERSRCH_ONLY_LAST_ACTION, SrchQuali->pOrder)) &&
		   (0 != strcmp(CC_USERSRCH_ONLY_GUID, SrchQuali->pOrder))){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchUser() param error[SrchQuali->pOrder], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(CC_ISNULL(BaseInf)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchUser() param error[BaseInf], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(CC_ISNULL(userNum)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchUser() param error[userNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(CC_ISNULL(pApiSts)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchUser() param error[pApiSts], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchUser() not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchUser() CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			memset(recvBuf, 0, recvBufSize);

			// ユーザ検索
			ret = CC_UserSrch_SendRecv(cal, &SmsApi_prm_t, SrchQuali, BaseInf, userNum, CC_USERSRCH_RES_FILE, recvBuf, recvBufSize, pApiSts, false);
			if(e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_UserSrch_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiSts=%s, " HERE, pApiSts);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, pApiSts)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief ユーザ検索(ポーリング用)
 * @param[in] SrchQuali 取得データの開始位置(1件目を0とする)
 * @param[out] BaseInf ユーザ基本情報構造体ポインタ
 * @param[in/out] userNum 取得ユーザ情報数(現状inは100とする)
 * @param[out] pApiSts APIステータスコード
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_OK以外 (コール関数の戻り値)
 */
E_SC_RESULT CC_SearchUser_Polling(const SMUSERSRCHQUALI *SrchQuali,
								  SMUSERBASEINFO *BaseInf,
								  INT32 *userNum,
								  Char *pApiSts)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメタチェック
		if(CC_ISNULL(SrchQuali)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchUser_Polling() param error[SrchQuali], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if((!CC_ISINVALIDSTR(SrchQuali->pOnly)) &&
		   (0 != strcmp(CC_USERSRCH_ONLY_FOLLOWER, SrchQuali->pOnly)) &&
		   (0 != strcmp(CC_USERSRCH_ONLY_FOLLOWEE, SrchQuali->pOnly))){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchUser_Polling() param error[SrchQuali->pOnly], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(0 > SrchQuali->offset){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchUser_Polling() param error[SrchQuali->offset], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if((0 >= SrchQuali->limit) || (SCC_CHAT_MAXUSER_NUM < SrchQuali->limit)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchUser_Polling() param error[SrchQuali->limit], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if((!CC_ISINVALIDSTR(SrchQuali->pOrder)) &&
		   (0 != strcmp(CC_USERSRCH_ORDER_NEWEST, SrchQuali->pOrder)) &&
		   (0 != strcmp(CC_USERSRCH_ONLY_LAST_ACTION, SrchQuali->pOrder)) &&
		   (0 != strcmp(CC_USERSRCH_ONLY_GUID, SrchQuali->pOrder))){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchUser_Polling() param error[SrchQuali->pOrder], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(CC_ISNULL(BaseInf)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchUser_Polling() param error[BaseInf], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(CC_ISNULL(userNum)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchUser_Polling() param error[userNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(CC_ISNULL(pApiSts)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchUser_Polling() param error[pApiSts], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchUser_Polling() not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchUser_Polling() CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			memset(recvBuf, 0, recvBufSize);

			// ユーザ検索
			ret = CC_UserSrch_SendRecv(cal, &SmsApi_prm_t, SrchQuali, BaseInf, userNum, CC_USERSRCH_RES_FILE_POLLING, recvBuf, recvBufSize, pApiSts, true);
			if(e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_UserSrch_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiSts=%s, " HERE, pApiSts);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, pApiSts)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, true);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//ルーム検索処理
//************************************************
/**
 * @brief ルーム検索
 * @param[in] SrchQuali ルーム検索条件
 * @param[out] roomInf ルーム情報構造体ポインタ
 * @param[in/out] roomNum 取得ルーム情報数(現状inは100とする)
 * @param[out] pApiSts APIステータスコード
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_OK以外 (コール関数の戻り値)
 */
E_SC_RESULT CC_SearchRoom(const SMROOMSRCHQUALI *SrchQuali,
							SMROOMINFO *roomInf,
							INT32 *roomNum,
							Char *pApiSts)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメタチェック
		if(CC_ISNULL(SrchQuali)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchRoom() param error[SrchQuali], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(0 > SrchQuali->offset){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchRoom() param error[SrchQuali->offset], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if((0 >= SrchQuali->limit) || (SCC_CHAT_MAXROOM_NUM < SrchQuali->limit)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchRoom() param error[SrchQuali->limit], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if((!CC_ISINVALIDSTR(SrchQuali->pOrder)) &&
		   (0 != strcmp(CC_ROOMSRCH_ORDER_LAST_UPDATE, SrchQuali->pOrder)) &&
		   (0 != strcmp(CC_ROOMSRCH_ORDER_ROOM_NAME, SrchQuali->pOrder))){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchRoom() param error[SrchQuali->pOrder], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if((!CC_ISINVALIDSTR(SrchQuali->pPriority)) &&
		   (0 != strcmp(CC_ROOMSRCH_PRIORITY_UNREAD, SrchQuali->pPriority))){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchRoom() param error[SrchQuali->pPriority], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(CC_ISNULL(roomInf)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchRoom() param error[roomInf], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(CC_ISNULL(roomNum)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchRoom() param error[roomNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(CC_ISNULL(pApiSts)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchRoom() param error[pApiSts], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchRoom() not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchRoom() CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			memset(recvBuf, 0, recvBufSize);

			// ルーム検索
			ret = CC_RoomSrch_SendRecv(cal, &SmsApi_prm_t, SrchQuali, roomInf, roomNum, CC_ROOMSRCH_RES_FILE, recvBuf, recvBufSize, pApiSts, false);
			if(e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_RoomSrch_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"ApiSts=%s, " HERE, pApiSts);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, pApiSts)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief ルーム検索(ポーリング用)
 * @param[in] SrchQuali ルーム検索条件
 * @param[out] roomInf ルーム情報構造体ポインタ
 * @param[in/out] roomNum 取得ルーム情報数(現状inは100とする)
 * @param[out] pApiSts APIステータスコード
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_OK以外 (コール関数の戻り値)
 */
E_SC_RESULT CC_SearchRoom_Polling(const SMROOMSRCHQUALI *SrchQuali,
								  SMROOMINFO *roomInf,
								  INT32 *roomNum,
								  Char *pApiSts)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメタチェック
		if(CC_ISNULL(SrchQuali)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchRoom_Polling() param error[SrchQuali], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(0 > SrchQuali->offset){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchRoom_Polling() param error[SrchQuali->offset], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if((0 >= SrchQuali->limit) || (SCC_CHAT_MAXROOM_NUM < SrchQuali->limit)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchRoom_Polling() param error[SrchQuali->limit], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if((!CC_ISINVALIDSTR(SrchQuali->pOrder)) &&
		   (0 != strcmp(CC_ROOMSRCH_ORDER_LAST_UPDATE, SrchQuali->pOrder)) &&
		   (0 != strcmp(CC_ROOMSRCH_ORDER_ROOM_NAME, SrchQuali->pOrder))){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchRoom_Polling() param error[SrchQuali->pOrder], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if((!CC_ISINVALIDSTR(SrchQuali->pPriority)) &&
		   (0 != strcmp(CC_ROOMSRCH_PRIORITY_UNREAD, SrchQuali->pPriority))){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchRoom_Polling() param error[SrchQuali->pPriority], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(CC_ISNULL(roomInf)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchRoom_Polling() param error[roomInf], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(CC_ISNULL(roomNum)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchRoom_Polling() param error[roomNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(CC_ISNULL(pApiSts)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchRoom_Polling() param error[pApiSts], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchRoom_Polling() CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			memset(recvBuf, 0, recvBufSize);

			// ルーム検索
			ret = CC_RoomSrch_SendRecv(cal,
									   &SmsApi_prm_t,
									   SrchQuali,
									   roomInf,
									   roomNum,
									   CC_ROOMSRCH_RES_FILE_POLLING,
									   recvBuf,
									   recvBufSize,
									   pApiSts,
									   true);
			if(e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_RoomSrch_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"ApiSts=%s, " HERE, pApiSts);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, pApiSts)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, true);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//チャット発言処理
//************************************************
/**
 * @brief チャット発言
 * @param[in] ChatMsg チャットメッセージ情報
 * @param[out] pApiSts APIステータスコード
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_OK以外 (コール関数の戻り値)
 */
E_SC_RESULT CC_SendChatMsg(const SMCHATMESSAGE *ChatMsg,
							Char *pApiSts)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメタチェック
		if(CC_ISNULL(ChatMsg)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SendChatMsg() param error[ChatMsg], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(CC_ISNULL(pApiSts)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SendChatMsg() param error[pApiSts], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			memset(recvBuf, 0, recvBufSize);

			// ユーザ検索
			ret = CC_ChatMsg_SendRecv(cal, &SmsApi_prm_t, ChatMsg, recvBuf, recvBufSize, pApiSts);
			if(e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ChatMsg_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiSts=%s, " HERE, pApiSts);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, pApiSts)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//チャット情報取得処理
//************************************************
/**
 * @brief チャット情報取得
 * @param[in]  GetQuali チャット情報取得条件
 * @param[out] pFilePath チャット情報xmlファイルパス
 * @param[out] pApiSts APIステータスコード
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_OK以外 (コール関数の戻り値)
 */
E_SC_RESULT CC_GetChatMsg(const SMCHATGETQUALI *GetQuali,
							Char *pFilePath,
							Char *pApiSts)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメタチェック
		if(CC_ISNULL(GetQuali)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetChatMsg() param error[GetQuali], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(CC_ISNULL(pFilePath)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetChatMsg() param error[pFilePath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(CC_ISNULL(pApiSts)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetChatMsg() param error[pApiSts], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			memset(recvBuf, 0, recvBufSize);

			// チャット取得
			ret = CC_ChatHis_SendRecv(cal,
									  &SmsApi_prm_t,
									  GetQuali,
									  CC_CHATHIS_RES_FILE,
									  pFilePath,
									  recvBuf,
									  recvBufSize,
									  pApiSts,
									  false);
			if(e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ChatHis_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiSts=%s, " HERE, pApiSts);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, pApiSts)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief チャット情報取得(ポーリング)
 * @param[in]  GetQuali チャット情報取得条件
 * @param[out] pFilePath チャット情報xmlファイルパス
 * @param[out] pApiSts APIステータスコード
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_OK以外 (コール関数の戻り値)
 */
E_SC_RESULT CC_GetChatMsg_Polling(const SMCHATGETQUALI *GetQuali,
								  Char *pFilePath,
								  Char *pApiSts)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメタチェック
		if(CC_ISNULL(GetQuali)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetChatMsg_Polling() param error[GetQuali], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(CC_ISNULL(pFilePath)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetChatMsg_Polling() param error[pFilePath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(CC_ISNULL(pApiSts)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetChatMsg_Polling() param error[pApiSts], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetChatMsg_Polling() not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetChatMsg_Polling() CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			memset(recvBuf, 0, recvBufSize);

			// チャット取得
			ret = CC_ChatHis_SendRecv(cal,
									  &SmsApi_prm_t,
									  GetQuali,
									  CC_CHATHIS_RES_FILE_POLLING,
									  pFilePath,
									  recvBuf,
									  recvBufSize,
									  pApiSts,
									  true);
			if(e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ChatHis_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiSts=%s, " HERE, pApiSts);
				if (0 == strcmp("SMSAPICE019", pApiSts)) {
					// ユーザに紐づくチャットルームが存在しなくてもエラーにしない。
					ret = e_SC_RESULT_SUCCESS;
					break;
				} else if (0 == strcmp(CC_CMN_API_AUTH_ERR, pApiSts)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, true);
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

	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//ルーム作成処理
//************************************************
/**
 * @brief ルーム作成
 * @param[in]  ChatRoomInf ルーム作成情報
 * @param[out] roomNo ルームNo
 * @param[out] pApiSts APIステータスコード
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_OK以外 (コール関数の戻り値)
 */
E_SC_RESULT CC_CreateChatRoom(const SMCREATEROOMINF *ChatRoomInf,
								Char *roomNo,

								Char *pApiSts)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメタチェック
		if(CC_ISNULL(ChatRoomInf)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_CreateChatRoom() param error[ChatRoomInf], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(CC_ISNULL(roomNo)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_CreateChatRoom() param error[roomNo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(CC_ISNULL(pApiSts)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_CreateChatRoom() param error[pApiSts], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_CreateChatRoom() not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_CreateChatRoom() CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			memset(recvBuf, 0, recvBufSize);

			// ルーム作成
			ret = CC_RoomReg_SendRecv(cal, &SmsApi_prm_t, ChatRoomInf, roomNo, recvBuf, recvBufSize, pApiSts);
			if(e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_RoomReg_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiSts=%s, " HERE, pApiSts);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, pApiSts)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//ルーム名変更処理
//************************************************
/**
 * @brief ルーム名変更
 * @param[in]  ChangeRoomName ルーム名変更情報
 * @param[out] pApiSts APIステータスコード
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_OK以外 (コール関数の戻り値)
 */
E_SC_RESULT CC_ChangeRoomName(const SMCHANGEROOMNAME *ChangeRoomName,
								Char *pApiSts)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメタチェック
		if(CC_ISNULL(ChangeRoomName)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_CreateChatRoom() param error[ChangeRoomName], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(CC_ISNULL(pApiSts)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_CreateChatRoom() param error[pApiSts], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_CreateChatRoom() not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_CreateChatRoom() CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			memset(recvBuf, 0, recvBufSize);

			// ルーム名変更
			ret = CC_RnameAlt_SendRecv(cal, &SmsApi_prm_t, ChangeRoomName, recvBuf, recvBufSize, pApiSts);
			if(e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_RoomReg_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiSts=%s, " HERE, pApiSts);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, pApiSts)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//ルームメンバ検索処理
//************************************************
/**
 * @brief ルームメンバ検索
 * @param[in] roomNo      ルームNo
 * @param[out] MemberInf  メンバ情報構造体ポインタ
 * @param[in/out] userNum 取得ユーザ情報数(現状inは100とする)
 * @param[out] pApiSts    APIステータスコード
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_OK以外 (コール関数の戻り値)
 */
E_SC_RESULT CC_SearchRoomMember(const Char *roomNo,
								SMMEMBERINFO *MemberInf,
								INT32 *userNum,
								Char *pApiSts)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメタチェック
		if(CC_ISINVALIDSTR(roomNo)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchRoomMember() param error[roomNo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(CC_ISNULL(MemberInf)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchRoomMember() param error[MemberInf], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(CC_ISNULL(userNum)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchRoomMember() param error[userNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(CC_ISNULL(pApiSts)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchRoomMember() param error[pApiSts], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchRoomMember() not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchRoomMember() CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			memset(recvBuf, 0, recvBufSize);

			// ルームメンバ検索
			ret = CC_MemberSrch_SendRecv(cal, &SmsApi_prm_t, roomNo, MemberInf, userNum, recvBuf, recvBufSize, pApiSts);
			if(e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MemberSrch_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiSts=%s, " HERE, pApiSts);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, pApiSts)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//未読メッセージ有ルーム検索処理
//************************************************
/**
 * @brief 未読メッセージ有ルーム検索
 * @param[out] unrdMsgRm  未読メッセージ有ルーム情報構造体ポインタ
 * @param[in/out] getnum 取得情報数
 * @param[out] pApiSts    APIステータスコード
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_OK以外 (コール関数の戻り値)
 */
E_SC_RESULT CC_SearchUnreadMsgRoom(SMUNREADMSGROOM *unrdMsgRm,
									INT32 *getnum,
									Char *pApiSts)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメタチェック
		if(CC_ISNULL(unrdMsgRm)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchUnreadMsgRoom() param error[unrdMsgRm], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(CC_ISNULL(getnum)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchUnreadMsgRoom() param error[getnum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(CC_ISNULL(pApiSts)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchUnreadMsgRoom() param error[pApiSts], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchUnreadMsgRoom() not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchUnreadMsgRoom() CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			memset(recvBuf, 0, recvBufSize);

			// 未読メッセージ有ルーム検索
			ret = CC_UnreadSrch_SendRecv(cal, &SmsApi_prm_t, unrdMsgRm, getnum, recvBuf, recvBufSize, pApiSts, false);
			if(e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_UnreadSrch_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiSts=%s, " HERE, pApiSts);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, pApiSts)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//未読メッセージ有ルーム検索処理 (ポーリング版)
//************************************************
/**
 * @brief 未読メッセージ有ルーム検索 (ポーリング版)
 * @param[out] unrdMsgRm  未読メッセージ有ルーム情報構造体ポインタ
 * @param[in/out] getnum 取得情報数
 * @param[out] pApiSts    APIステータスコード
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_OK以外 (コール関数の戻り値)
 */
E_SC_RESULT CC_SearchUnreadMsgRoom_Polling(SMUNREADMSGROOM *unrdMsgRm,
									INT32 *getnum,
									Char *pApiSts)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメタチェック
		if(CC_ISNULL(unrdMsgRm)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchUnreadMsgRoom() param error[unrdMsgRm], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(CC_ISNULL(getnum)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchUnreadMsgRoom() param error[getnum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(CC_ISNULL(pApiSts)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchUnreadMsgRoom() param error[pApiSts], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchUnreadMsgRoom() not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SearchUnreadMsgRoom() CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			memset(recvBuf, 0, recvBufSize);

			// 未読メッセージ有ルーム検索
			ret = CC_UnreadSrch_SendRecv(cal, &SmsApi_prm_t, unrdMsgRm, getnum, recvBuf, recvBufSize, pApiSts, true);
			if(e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_UnreadSrch_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiSts=%s, " HERE, pApiSts);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, pApiSts)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, true);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//User_sig取得処理
//************************************************
/**
 * @brief User_sig取得
 * @param[out] user_sig
 * @retval 正常終了  :CC_CMN_RESULT_OK
 */
E_SC_RESULT CC_GetUserSig(Char *user_sig)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	//Char	pUserInfo[SCC_MAX_PATH];

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		if (CC_ISNULL(user_sig)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetUserSig() param error[user_sig], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetUserSig() not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

#if 0	//******************************************************************************************************************   デバッグ用処理
		CC_ReadUserInfoFile();								// ユーザ情報ファイル読出し
#endif	//******************************************************************************************************************   デバッグ用処理

		memset(user_sig, 0, CC_CMN_USER_SIG_STR_SIZE);
		strcpy((char*)user_sig, (const char*)SmsApi_prm_t.ApiPrmMups.user_sig);

#if 0	//******************************************************************************************************************   デバッグ用処理
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_GetUserSig() user_sig = %s ", user_sig);
#endif	//******************************************************************************************************************   デバッグ用処理

	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//位置情報更新
//************************************************
/**
 * @brief 位置情報更新
 * @param[in]  pos              位置情報
 * @param[out] apiStatus        APIステータス
 * @retval OK : e_SC_RESULT_SUCCESS
 * @retval NG : e_SC_RESULT_SUCCESS以外
 */
E_SC_RESULT CC_UpdatePosInfo(const SMPOSITIONINFO *pos, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		if (CC_ISNULL(pos)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[pos], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((CC_ISINVALIDSTR(pos->roomNo)) || (CC_MAX_ROOMNO <= strlen(pos->roomNo))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[pos->roomNo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((CC_CMN_POSITION_FLG_PRIVATE != pos->positionFlg) &&
			(CC_CMN_POSITION_FLG_PUBLIC  != pos->positionFlg)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[pos->positionFlg], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((CC_CMN_POSITION_FLG_PUBLIC == pos->positionFlg) && (!pos->latFlg)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[pos->latFlg], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((CC_CMN_POSITION_FLG_PUBLIC == pos->positionFlg) && (!pos->lonFlg)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[pos->lonFlg], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus] " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			*apiStatus = EOS;
			memset(recvBuf, 0, recvBufSize);

			// 位置情報更新
			ret = CC_PosInfoUpdate_SendRecv(cal, &SmsApi_prm_t, pos, recvBuf, recvBufSize, apiStatus, false);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_PosInfoUpdate_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief 位置情報更新(ポーリング)
 * @param[in]  pos              位置情報
 * @param[out] apiStatus        APIステータス
 * @retval OK : e_SC_RESULT_SUCCESS
 * @retval NG : e_SC_RESULT_SUCCESS以外
 */
E_SC_RESULT CC_UpdatePosInfo_Polling(const SMPOSITIONINFO *pos, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	//Char	guid[CC_CMN_GUID_STR_SIZE] = {};
	//Char	act_sts[CC_CMN_ACT_STATUS_STR_SIZE] = {};			// TR3.5 アクティベーション対応
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		if (CC_ISNULL(pos)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[pos], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((CC_ISINVALIDSTR(pos->roomNo)) || (CC_MAX_ROOMNO <= strlen(pos->roomNo))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[pos->roomNo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((CC_CMN_POSITION_FLG_PRIVATE != pos->positionFlg) &&
			(CC_CMN_POSITION_FLG_PUBLIC  != pos->positionFlg)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[pos->positionFlg], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((CC_CMN_POSITION_FLG_PUBLIC == pos->positionFlg) && (!pos->latFlg)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[pos->latFlg], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((CC_CMN_POSITION_FLG_PUBLIC == pos->positionFlg) && (!pos->lonFlg)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[pos->lonFlg], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus] " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			*apiStatus = EOS;
			memset(recvBuf, 0, recvBufSize);

			// 位置情報更新
			ret = CC_PosInfoUpdate_SendRecv(cal,
											&SmsApi_prm_t,
											pos,
											recvBuf,
											recvBufSize,
											apiStatus,
											true);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_PosInfoUpdate_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, true);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//位置情報取得
//************************************************
/**
 * @brief 位置情報取得
 * @param[in] roomNo            ルームNo
 * @param[in/out] posInfo       位置情報
 * @param[in/out] posInfoNum    位置情報数
 * @param[out] apiSts           APIステータス
 * @retval OK : e_SC_RESULT_SUCCESS
 * @retval NG : e_SC_RESULT_SUCCESS以外
 */
E_SC_RESULT CC_SearchPosInfo(const Char *roomNo, SMPOSITIONINFO *posInfo, INT32 *posInfoNum, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	//Char	guid[CC_CMN_GUID_STR_SIZE] = {};
	//INT32	posNum = 0;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		if (CC_ISINVALIDSTR(roomNo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[roomNo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_MAX_ROOMNO <= strlen(roomNo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[roomNo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(posInfo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[posInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(posInfoNum)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[posInfoNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			*apiStatus = EOS;
			*posInfoNum = 0;
			memset(posInfo, 0, (sizeof(SMPOSITIONINFO) * CC_MAX_POSINFO_NUM));
			memset(recvBuf, 0, recvBufSize);

			// 位置情報取得
			ret = CC_PosInfoSearch_SendRecv(cal,
											&SmsApi_prm_t,
											roomNo,
											posInfo,
											posInfoNum,
											CC_POSINFO_RES_FILE,
											recvBuf,
											recvBufSize,
											apiStatus,
											false);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_PosInfoSearch_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief 位置情報取得(ポーリング)
 * @param[in] roomNo            ルームNo
 * @param[in/out] posInfo       位置情報
 * @param[in/out] posInfoNum    位置情報数
 * @param[out] apiSts           APIステータス
 * @retval OK : e_SC_RESULT_SUCCESS
 * @retval NG : e_SC_RESULT_SUCCESS以外
 */
E_SC_RESULT CC_SearchPosInfo_Polling(const Char *roomNo, SMPOSITIONINFO *posInfo, INT32 *posInfoNum, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	//Char	guid[CC_CMN_GUID_STR_SIZE] = {};
	//Char	act_sts[CC_CMN_ACT_STATUS_STR_SIZE] = {};			// TR3.5 アクティベーション対応
	//INT32	posNum = 0;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		if (CC_ISINVALIDSTR(roomNo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[roomNo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_MAX_ROOMNO <= strlen(roomNo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[roomNo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(posInfo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[posInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(posInfoNum)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[posInfoNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			*apiStatus = EOS;
			*posInfoNum = 0;
			memset(posInfo, 0, (sizeof(SMPOSITIONINFO) * CC_MAX_POSINFO_NUM));
			memset(recvBuf, 0, recvBufSize);

			// 位置情報取得
			ret = CC_PosInfoSearch_SendRecv(cal,
											&SmsApi_prm_t,
											roomNo,
											posInfo,
											posInfoNum,
											CC_POSINFO_RES_FILE_POLLING,
											recvBuf,
											recvBufSize,
											apiStatus,
											true);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_PosInfoSearch_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, true);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//アプリ設定保存/取得
//************************************************
/**
 * @brief アプリ設定保存
 * @param[in] appconf      アプリ設定
 * @param[in] appconfSize  アプリ設定サイズ
 * @param[out] apiSts       APIステータス
 * @retval OK : e_SC_RESULT_SUCCESS
 * @retval NG : e_SC_RESULT_SUCCESS以外
 */
E_SC_RESULT CC_SaveAppconf(const Char *appconf, INT32 appconfSize, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		if (CC_ISNULL(appconf)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[appconf], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (0 > appconfSize) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[appconfSize], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			*apiStatus = EOS;
			memset(recvBuf, 0, recvBufSize);

			// アプリ設定保存
			ret = CC_AppconfReg_SendRecv(cal, &SmsApi_prm_t, appconf, appconfSize, recvBuf, recvBufSize, apiStatus);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_AppconfReg_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief アプリ設定取得
 * @param[in]  appconf      アプリ設定
 * @param[in]  appconfSize  アプリ設定サイズ
 * @param[out] apiSts       APIステータス
 * @retval OK : e_SC_RESULT_SUCCESS
 * @retval NG : e_SC_RESULT_SUCCESS以外
 */
E_SC_RESULT CC_GetAppconf(Char *appconf, INT32 *appconfSize, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		if (CC_ISNULL(appconf)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[appconf], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(appconfSize)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[appconfSize], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			*apiStatus = EOS;
			*appconf= EOS;
			*appconfSize = 0;
			memset(recvBuf, 0, recvBufSize);

			// アプリ設定取得
			ret = CC_AppconfReq_SendRecv(cal, &SmsApi_prm_t, appconf, recvBuf, recvBufSize, apiStatus);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_AppconfReq_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
		*appconfSize = strlen(appconf);
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//センタ連携のバックアップデータ設定/取得
//************************************************
/**
 * @brief バックアップデータ設定
 * @param[in]  type         バックアップデータ種別
 * @param[out] bkData       バックアップデータ
 * @param[out] bkDataSize   バックアップデータサイズ
 * @retval OK : e_SC_RESULT_SUCCESS
 * @retval NG : e_SC_RESULT_SUCCESS以外
 */
E_SC_RESULT CC_SetComBackupData(INT32 type, const Char *bkData, INT32 bkDataSize)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		if (0 != type) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[type], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(bkData)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[bkData], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// バックアップデータ設定
		ret = CC_SetBackupData(type, bkData, bkDataSize, &SmsApi_prm_t);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SetBackupData error, " HERE);
			break;
		}

		// ユーザ情報ファイル退避
		CC_SaveUserInfoFile();
	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief バックアップデータ取得
 * @param[in]  type         バックアップデータ種別
 * @param[out] bkData       バックアップデータ
 * @param[out] bkDataSize   バックアップデータサイズ
 * @retval OK : e_SC_RESULT_SUCCESS
 * @retval NG : e_SC_RESULT_SUCCESS以外
 */
E_SC_RESULT CC_GetComBackupData(INT32 type, Char *bkData, INT32 *bkDataSize)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		if (0 != type) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[type], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(bkData)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[bkData], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(bkDataSize)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[bkDataSize], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 初期化
		*bkData = EOS;
		bkDataSize = 0;

		// バックアップデータ取得
		ret = CC_GetBackupData(type, bkData, bkDataSize, &SmsApi_prm_t);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetBackupData error, " HERE);
			break;
		}
	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//更新データ有無チェック
//************************************************
/**
 * @brief 更新データ有無チェック
 * @param[in/out] chkUpdInf		更新データチェック情報格納ポインタ
 * @param[out] apiStatus		APIステータス格納ポインタ
 * @retval OK : e_SC_RESULT_SUCCESS
 * @retval NG : e_SC_RESULT_SUCCESS以外
 * @note 広域地図データ、プロパティファイルのサーバと端末内のデータバージョンを比較し、
 *       それぞれのダウンロード可否を返す
 */
E_SC_RESULT CC_CheckMapUpdate(SMCHECKUPDINFO *chkUpdInf, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	Char	*dlTempDirPath = NULL;
	SMMAPDLCBFNC	callbackFnc = {};
	//INT32	num = 0;
	SMMAPUPDSTATUS	*updStatus = NULL;
	//Char	backetName[SCC_AWS_BACKETNAME_SIZE + 1] = {};
	Bool	errComeback = false;
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメタチェック
		if(CC_ISNULL(chkUpdInf)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_CheckMapUpdate() param error[chkUpdInf], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_CheckMapUpdate() param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 初期化
		memset(chkUpdInf->hasUpdate, false, sizeof(chkUpdInf->hasUpdate));

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// メモリ確保
		dlTempDirPath = (Char*)SCC_MALLOC(SCC_MAX_PATH);
		if (NULL == dlTempDirPath) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		updStatus = (SMMAPUPDSTATUS*)SCC_MALLOC(sizeof(SMMAPUPDSTATUS));
		if (NULL == updStatus) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// ダウンロード用テンポラリフォルダのパス生成
		strcpy((char*)dlTempDirPath, (char*)SCC_GetRootDirPath());
		strcat(dlTempDirPath, CC_CMN_TEMP_DIR_PATH);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"dlTempDirPath = %s, " HERE, dlTempDirPath);

		// コールバック関数のポインタ設定
		callbackFnc.cancel = (SCC_CANCEL_FNC)SCC_IsCancel;
		callbackFnc.progress = NULL;

		// 異常復帰かチェックする
		do {
			if (CC_ISEOS(chkUpdInf->regionCode)) {
				// 仕向け地指定なし
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"no regionCode, " HERE);
				break;
			}

			// 進捗状況取得
			CC_UpdDataProgressMng_GetUpdStatus(updStatus);
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"status=%d, " HERE, updStatus->status);
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"regionCode=%s, " HERE, updStatus->regionCode);
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"hasUpdate[0]=%d, " HERE, updStatus->hasUpdate[0]);
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"hasUpdate[1]=%d, " HERE, updStatus->hasUpdate[1]);
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"version[0]=%d, " HERE, updStatus->version[0]);
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"version[1]=%d, " HERE, updStatus->version[1]);
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"areaIdx=%d, " HERE, updStatus->areaIdx);
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"backetName=%s, " HERE, updStatus->backetName);

			// 更新中だった仕向け地と指定された仕向け地が一致するかチェック
			if (0 != strcmp(chkUpdInf->regionCode, updStatus->regionCode)) {
				// 一致しない場合、進捗状況をすべて破棄して最初からやり直す
				// 進捗クリア
				CC_UpdDataProgressMng_ClearUpdStatus();
				// ダウンロード用テンポラリフォルダ削除(戻り値はチェックしない)
				CC_DeleteDir(dlTempDirPath);
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"CC_UpdDataProgressMng_ClearUpdStatus, " HERE);
				// ダウンロード用テンポラリフォルダ作成
				ret = CC_MakeDir(dlTempDirPath);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MakeDir error[%s], " HERE, dlTempDirPath);
				}
				break;
			}
			// 進捗状況チェック
			if ((CC_MAPUPDSTATUS_MAPUPDATE     > updStatus->status) ||
				(CC_MAPUPDSTATUS_DATAVERUPDATE < updStatus->status)) {
				// 広域地図の更新前 または プロパティデータ更新が完了している
				// 進捗クリア
				CC_UpdDataProgressMng_ClearUpdStatus();
//				// ダウンロード用テンポラリフォルダ削除(戻り値はチェックしない)
//				CC_DeleteDir(dlTempDirPath);
//				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"CC_UpdDataProgressMng_ClearUpdStatus, " HERE);
//				// ダウンロード用テンポラリフォルダ作成
//				ret = CC_MakeDir(dlTempDirPath);
//				if (e_SC_RESULT_SUCCESS != ret) {
//					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MakeDir error[%s], " HERE, dlTempDirPath);
//				}
				break;
			}

			// 異常復帰する
			errComeback = true;
			memcpy(chkUpdInf->hasUpdate, updStatus->hasUpdate, sizeof(chkUpdInf->hasUpdate));
			memcpy(chkUpdInf->version, updStatus->version, sizeof(chkUpdInf->version));
			if ((chkUpdInf->hasUpdate[SCC_MAPDWL_KIND_MAP]) &&
				(CC_MAPUPDSTATUS_MAPVERUPDATE < updStatus->status)) {
				chkUpdInf->hasUpdate[SCC_MAPDWL_KIND_MAP] = false;
			}
			if ((chkUpdInf->hasUpdate[SCC_MAPDWL_KIND_DATA]) &&
				(CC_MAPUPDSTATUS_DATAVERUPDATE < updStatus->status)) {
				chkUpdInf->hasUpdate[SCC_MAPDWL_KIND_DATA] = false;
			}
		} while (0);
		if (e_SC_RESULT_SUCCESS != ret) {
			break;
		}

		if (errComeback) {
			// エラー復帰チェック
			ret = CC_UpdateData_ComebackCheck(cal, updStatus, dlTempDirPath, &errComeback);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_UpdateData_ComebackCheck error, " HERE);
				break;
			}
		}
		if (!errComeback) {
			// エラー復帰ではない
			// ダウンロード用テンポラリフォルダ作成
			ret = CC_MakeDir(dlTempDirPath);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MakeDir error[%s], " HERE, dlTempDirPath);
				break;
			}
		}

		// 更新データの有無チェック
		ret = CC_CheckUpdate(cal, &SmsApi_prm_t, chkUpdInf, dlTempDirPath, &callbackFnc, errComeback);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_CheckUPDate error, " HERE);
			break;
		}

		// TODO:TR5.0暫定　広域地図の更新がある場合は、エリア地図の更新は必須とする
		if (chkUpdInf->hasUpdate[SCC_MAPDWL_KIND_MAP]) {
			// 広域地図の更新あり
			CC_SetIsRegionMapUpdate(true);
		} else {
			// 広域地図の更新なし
			CC_SetIsRegionMapUpdate(false);
		}
		// TODO:TR5.0暫定　広域地図の更新がある場合は、エリア地図の更新は必須とする
	} while (0);

	// メモリ解放
	if (NULL != dlTempDirPath) {
		SCC_FREE(dlTempDirPath);
	}
	if (NULL != updStatus) {
		SCC_FREE(updStatus);
	}

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	if (SCC_IsCancel()) {
		ret = e_SC_RESULT_CANCEL;
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//リージョン情報リスト取得
//************************************************
/**
 * @brief リージョン情報リスト取得
 * @param[out] rgnI リージョン情報構造体ポインタ
 * @param[in/out] rgnNum    リージョン情報数
 * @param[out] apiStatus  APIステータス
 * @retval OK : e_SC_RESULT_SUCCESS
 * @retval NG : e_SC_RESULT_SUCCESS以外
 * @note センタと端末の地図データVerを比較判定し、必要ならダウンロード＆解凍する
 */
E_SC_RESULT CC_GetRegionList(SMRGNINFO *rgnI, INT32 *rgnNum, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	Char	*dlTempDirPath = NULL;
	SMMAPDLCBFNC	callbackFnc = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメタチェック
		if(CC_ISNULL(rgnI)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetRegionList() param error[rgnI], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(CC_ISNULL(rgnNum)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetRegionList() param error[rgnNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(CC_ISNULL(apiStatus)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetRegionList() param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// メモリ確保
		dlTempDirPath = (Char*)SCC_MALLOC(SCC_MAX_PATH);
		if (NULL == dlTempDirPath) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// ダウンロード用テンポラリフォルダのパス生成
		strcpy((char*)dlTempDirPath, (char*)SCC_GetRootDirPath());
		strcat(dlTempDirPath, CC_CMN_TEMP_DIR_PATH);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"dlTempDirPath = %s, " HERE, dlTempDirPath);

		// ダウンロード用テンポラリフォルダ作成
		ret = CC_MakeDir(dlTempDirPath);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MakeDir error[%s], " HERE, dlTempDirPath);
			break;
		}

		// コールバック関数のポインタ設定
		callbackFnc.cancel = (SCC_CANCEL_FNC)SCC_IsCancel;
		callbackFnc.progress = NULL;

		// リージョン情報リスト取得
		ret = CC_GetRgnInfoList(cal, &SmsApi_prm_t, rgnI, rgnNum, dlTempDirPath, &callbackFnc);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetRgnInfoList error, " HERE);
			break;
		}

	} while (0);

	if (NULL != dlTempDirPath) {
		// メモリ解放
		SCC_FREE(dlTempDirPath);
	}

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	if (SCC_IsCancel()) {
		ret = e_SC_RESULT_CANCEL;
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//地域情報リスト取得
//************************************************
/**
 * @brief 地域情報リスト取得
 * @param[in] rgnCode 仕向け地コード
 * @param[out] sectI 地方情報構造体ポインタ
 * @param[in/out] sectNum    地方情報数
 * @param[in] sectNum    地域情報最大数
 * @param[out] apiStatus  APIステータス
 * @retval OK : e_SC_RESULT_SUCCESS
 * @retval NG : e_SC_RESULT_SUCCESS以外
 * @note センタと端末の地図データVerを比較判定
 */
E_SC_RESULT CC_GetAreaList(const Char *rgnCode, SMSECINFO *sectI, INT32 *sectNum, INT32 *areaNum, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	Char	*dlTempDirPath = NULL;
	SMMAPDLCBFNC	callbackFnc = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメタチェック
		if(CC_ISINVALIDSTR(rgnCode)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetAreaList() param error[rgnCode], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(sectI)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetAreaList() param error[sectI], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((CC_ISNULL(sectNum)) || (0 >= *sectNum)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetAreaList() param error[sectNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((CC_ISNULL(areaNum)) || (0 >= *areaNum)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetAreaList() param error[areaNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetAreaList() param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// メモリ確保
		dlTempDirPath = (Char*)SCC_MALLOC(SCC_MAX_PATH);
		if (NULL == dlTempDirPath) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// ダウンロード用テンポラリフォルダのパス生成
		strcpy((char*)dlTempDirPath, (char*)SCC_GetRootDirPath());
		strcat(dlTempDirPath, CC_CMN_TEMP_DIR_PATH);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"dlTempDirPath = %s, " HERE, dlTempDirPath);

		// ダウンロード用テンポラリフォルダ作成
		ret = CC_MakeDir(dlTempDirPath);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MakeDir error[%s], " HERE, dlTempDirPath);
			break;
		}

		// コールバック関数のポインタ設定
		callbackFnc.cancel = (SCC_CANCEL_FNC)SCC_IsCancel;
		callbackFnc.progress = NULL;

		// 地域情報リスト取得
		ret = CC_GetAreaInfoList(cal, &SmsApi_prm_t, rgnCode, sectI, sectNum, areaNum, dlTempDirPath, &callbackFnc);
		if ((e_SC_RESULT_SUCCESS != ret) && (e_SC_RESULT_NOT_FOUND_MAP != ret)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetAreaInfoList error, " HERE);
			break;
		}


//*****************************************************************
#if 0
INT32 sect_cnt, area_cnt;
SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"sectNum:%d, " HERE, *sectNum);
for(sect_cnt = 0; *sectNum > sect_cnt; sect_cnt++){
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"SectCD:%d  SectNM:%s, " HERE, sectI[sect_cnt].sectionCode, sectI[sect_cnt].sectionName);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"areaNum:%d, " HERE, sectI[sect_cnt].area_num);
	for(area_cnt = 0; sectI[sect_cnt].area_num > area_cnt; area_cnt++){
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"    AreaCD:%d  AreaNM:%s, " HERE, sectI[sect_cnt].areaInfo[area_cnt].areaCode, sectI[sect_cnt].areaInfo[area_cnt].areaName);
	}
}
#endif
//*****************************************************************


	} while (0);

	if (NULL != dlTempDirPath) {
		// メモリ解放
		SCC_FREE(dlTempDirPath);
	}

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	if (SCC_IsCancel()) {
		ret = e_SC_RESULT_CANCEL;
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//地図データダウンロード
//************************************************
/**
 * @brief データ更新
 * @param[in/out] mapUpdInfo    データ更新情報
 * @retval OK : e_SC_RESULT_SUCCESS
 * @retval NG : e_SC_RESULT_SUCCESS以外
 */
E_SC_RESULT CC_UpdateData(SMUPDINFO *mapUpdInfo, SMDLERRORINFO *errInfo, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	Char	*dlTempDirPath = NULL;
	INT32	len = 0;
	SMMAPDLCBFNC	callbackFnc = {};
	SCC_UPDATEDATA	*updData = NULL;
	SCC_UPDATEDATA	*updAppData = NULL;
	INT32	num = 0;
	UINT32	dataNum = 0;
	UINT32	dataNumProgress = 0;
	SMCHECKUPDINFO	*chkUpdInf = NULL;
	SMMAPUPDSTATUS	*updStatus = NULL;
	Bool	errComeback = false;
	//Char	*filePath = NULL;
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISNULL(errInfo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[errInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(mapUpdInfo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mapUpdInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((!mapUpdInfo->rgnMapUpdate) && (0 == mapUpdInfo->updateAreaNum)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mapUpdInfo->updateAreaNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		errInfo->dlErrInfo = CC_DLERROR_NG;

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// 地図分割ダウンロードパラメタ内地図DB格納パス変更
		ret = CC_ChangeMapDBPath(mapUpdInfo);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ChangeMapDBPath error, " HERE);
			break;
		}

		// メモリ確保
		dlTempDirPath = (Char*)SCC_MALLOC(SCC_MAX_PATH);
		if (NULL == dlTempDirPath) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		chkUpdInf = (SMCHECKUPDINFO*)SCC_MALLOC(sizeof(SMCHECKUPDINFO));
		if (NULL == chkUpdInf) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		updStatus = (SMMAPUPDSTATUS*)SCC_MALLOC(sizeof(SMMAPUPDSTATUS));
		if (NULL == updStatus) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// 進捗状況設定
		memset(&smDlProgress, 0, sizeof(SMDLPROGRESS));
		strcpy(smDlProgress.msg, SCC_CMN_PROGRESS_MSG_CHECK);

		// コールバック関数のポインタ設定
		callbackFnc.cancel   = (SCC_CANCEL_FNC)SCC_IsCancel;
		callbackFnc.progress = NULL;

		// 更新状況を読み込む
		ret = CC_UpdDataProgressMng_LoadUpdStatus();
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_UpdDataProgressMng_LoadUpdStatus error, " HERE);
			break;
		}

		// 進捗情報取得
		CC_UpdDataProgressMng_GetUpdStatus(updStatus);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"status=%d, " HERE, updStatus->status);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"regionCode=%s, " HERE, updStatus->regionCode);
		//SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"completeCount=%u, " HERE, updStatus->completeCount);
		//SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"totalCount=%u, " HERE, updStatus->totalCount);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"backetName=%s, " HERE, updStatus->backetName);

		// ダウンロード用テンポラリフォルダのパス生成
		strcpy((char*)dlTempDirPath, (char*)mapUpdInfo->installDirPath);
		len = strlen((char*)dlTempDirPath);
		if ('/' != dlTempDirPath[len - 1]) {
			strcat(dlTempDirPath, "/");
		}
		strcat(dlTempDirPath, CC_CMN_TEMP_DIR_PATH);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"dlTempDirPath=%s, " HERE, dlTempDirPath);

		// 進捗状況チェック
		if ((CC_MAPUPDSTATUS_MAPDL <= updStatus->status) && (CC_MAPUPDSTATUS_DATAVERUPDATE >= updStatus->status)) {
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"error comeback update, " HERE);
			// 広域地図ダウンロード ～ プロパティデータ更新の間
			memcpy(chkUpdInf->hasUpdate, updStatus->hasUpdate, sizeof(chkUpdInf->hasUpdate));
			// hasUpdateの更新
			if (chkUpdInf->hasUpdate[SCC_MAPDWL_KIND_MAP]) {
				if (CC_MAPUPDSTATUS_MAPUPDATED <= updStatus->status) {
					// 更新済みの場合は、更新対象から外す
					chkUpdInf->hasUpdate[SCC_MAPDWL_KIND_MAP] = false;
				}
			}
			if (chkUpdInf->hasUpdate[SCC_MAPDWL_KIND_DATA]) {
				if (CC_MAPUPDSTATUS_DATAUPDATED <= updStatus->status) {
					// 更新済みの場合は、更新対象から外す
					chkUpdInf->hasUpdate[SCC_MAPDWL_KIND_DATA] = false;
				}
			}
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"hasUpdate[0]=%d, " HERE, chkUpdInf->hasUpdate[SCC_MAPDWL_KIND_MAP]);
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"hasUpdate[1]=%d, " HERE, chkUpdInf->hasUpdate[SCC_MAPDWL_KIND_DATA]);
			memcpy(chkUpdInf->version, updStatus->version, sizeof(chkUpdInf->version));
			chkUpdInf->updateStatus = SCC_MAPDWL_CHECK_NO_DATA;
			errComeback = true;
		} else {
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"normal update, " HERE);
			memset(updStatus, 0, sizeof(SMMAPUPDSTATUS));
			// 進捗クリア
			CC_UpdDataProgressMng_ClearUpdStatus();
			// ダウンロード用テンポラリフォルダ削除(戻り値はチェックしない)
			CC_DeleteDir(dlTempDirPath);
			// ダウンロード用テンポラリフォルダ作成
			ret = CC_MakeDir(dlTempDirPath);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MakeDir error[%s], " HERE, dlTempDirPath);
				break;
			}
		}

		// エラー復帰チェック
		ret = CC_UpdateData_ComebackCheck(cal, updStatus, dlTempDirPath, &errComeback);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_UpdateData_ComebackCheck error, " HERE);
			break;
		}

		// 仕向け地コード
		strcpy(chkUpdInf->regionCode, mapUpdInfo->regionCode);
		strcpy(updStatus->regionCode, mapUpdInfo->regionCode);
		// 更新データ有無チェック
		ret = CC_CheckUpdate(cal, &SmsApi_prm_t, chkUpdInf, dlTempDirPath, &callbackFnc, errComeback);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_CheckUpdate error, " HERE);
			break;
		}
		memcpy(updStatus->hasUpdate, chkUpdInf->hasUpdate, sizeof(chkUpdInf->hasUpdate));
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"updateStatus=%d, " HERE, chkUpdInf->updateStatus);
		// チェックが完了したので、デフォルトをCC_DLERROR_OKにする
		errInfo->dlErrInfo = CC_DLERROR_OK;

		// PARCELとAREA_CLS
		dataNum = (mapUpdInfo->updateAreaNum * 2);
		dataNumProgress = mapUpdInfo->updateAreaNum;
		for (num = 0; num < SCC_MAPDWL_KIND_NUM; num++) {
			if ((mapUpdInfo->rgnMapUpdate) && (updStatus->hasUpdate[num])) {
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"hasUpdate[%d]=%d, " HERE, num, updStatus->hasUpdate[num]);
				// 更新データ数(更新対象のファイル数)
				dataNum++;
#ifdef __SMS_APPLE__
#else
				dataNumProgress++;
#endif /* __SMS_APPLE__ */
			}
		}
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"dataNum=%u, " HERE, dataNum);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"dataNumProgress=%u, " HERE, dataNumProgress);
		smDlProgress.totalCount = dataNumProgress;

		// メモリ確保
		updData = (SCC_UPDATEDATA*)SCC_MALLOC(sizeof(SCC_UPDATEDATA) * dataNum);
		if (NULL == updData) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			if ((updStatus->hasUpdate[SCC_MAPDWL_KIND_MAP]) || (updStatus->hasUpdate[SCC_MAPDWL_KIND_DATA])) {
				errInfo->dlErrInfo = CC_DLERROR_NG;
			}
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		memset(updData, 0, (sizeof(SCC_UPDATEDATA) * dataNum));
		updAppData = (SCC_UPDATEDATA*)SCC_MALLOC(sizeof(SCC_UPDATEDATA));
		if (NULL == updAppData) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			if ((updStatus->hasUpdate[SCC_MAPDWL_KIND_MAP]) || (updStatus->hasUpdate[SCC_MAPDWL_KIND_DATA])) {
				errInfo->dlErrInfo = CC_DLERROR_NG;
			}
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		memset(updAppData, 0, sizeof(SCC_UPDATEDATA));

		// ダウンロード情報取得
		ret = CC_GetUpdDataInfo(cal, &SmsApi_prm_t, mapUpdInfo, updStatus->hasUpdate, dlTempDirPath, updData, updAppData);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetUpdAreaInfo error, " HERE);
			if ((updStatus->hasUpdate[SCC_MAPDWL_KIND_MAP]) || (updStatus->hasUpdate[SCC_MAPDWL_KIND_DATA])) {
				errInfo->dlErrInfo = CC_DLERROR_NG;
			}
			break;
		}
		if (!errComeback) {
			num = 0;
			if (updStatus->hasUpdate[SCC_MAPDWL_KIND_DATA]) {
				updStatus->version[SCC_MAPDWL_KIND_DATA] = updData[num].dataVersion;
				num++;
			}
			if (updStatus->hasUpdate[SCC_MAPDWL_KIND_MAP]) {
				updStatus->version[SCC_MAPDWL_KIND_MAP] = updData[num].dataVersion;
				num++;
			}
			updAppData->dataVersion;
		} else {

		}
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"status=%d, " HERE, updStatus->status);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"regionCode=%s, " HERE, updStatus->regionCode);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"backetName=%s, " HERE, updStatus->backetName);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"hasUpdate[0]=%d, " HERE, updStatus->hasUpdate[SCC_MAPDWL_KIND_MAP]);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"hasUpdate[1]=%d, " HERE, updStatus->hasUpdate[SCC_MAPDWL_KIND_DATA]);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"rgnMapUpdate=%d, " HERE, mapUpdInfo->rgnMapUpdate);

		// キャンセルチェック
		if (SCC_IsCancel()) {
			SCC_LOG_WarnPrint(SC_TAG_CC, (Char*)"cancel, " HERE);
			ret = e_SC_RESULT_CANCEL;
			if ((updStatus->hasUpdate[SCC_MAPDWL_KIND_MAP]) || (updStatus->hasUpdate[SCC_MAPDWL_KIND_DATA])) {
				errInfo->dlErrInfo = CC_DLERROR_NG;
			}
			break;
		}

		// コールバック関数のポインタ設定
		callbackFnc.cancel   = (SCC_CANCEL_FNC)SCC_IsCancel;
		callbackFnc.progress = (SCC_PROGRESSINFO_FNC)CC_ProgressCallback;
		CC_UpdDataProgressMng_SetCallbackFnc(&callbackFnc);

		// 更新状況設定
		CC_UpdDataProgressMng_SetUpdStatus(updStatus);
		// 更新状況保存
		ret = CC_UpdDataProgressMng_SaveUpdStatus();
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_UpdDataProgressMng_SaveUpdStatus error, " HERE);
			if ((updStatus->hasUpdate[SCC_MAPDWL_KIND_MAP]) || (updStatus->hasUpdate[SCC_MAPDWL_KIND_DATA])) {
				errInfo->dlErrInfo = CC_DLERROR_NG;
			}
			break;
		}
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"dlTempDirPath=%s, " HERE, dlTempDirPath);

		// ダウンロードと更新
		ret = CC_UpdDataReq(cal, &SmsApi_prm_t, mapUpdInfo, updAppData->dataVersion, dlTempDirPath, updData, dataNum, dataNumProgress, updStatus, &callbackFnc);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_UpdDataReq error, " HERE);
			if ((updStatus->hasUpdate[SCC_MAPDWL_KIND_MAP]) || (updStatus->hasUpdate[SCC_MAPDWL_KIND_DATA])) {
				if (CC_MAPUPDSTATUS_DATAUPDATED > updStatus->status) {
					errInfo->dlErrInfo = CC_DLERROR_NG;
				}
			}
			break;
		}

		// 更新完了
		CC_UpdDataProgressMng_SetStatus(CC_MAPUPDSTATUS_UPDATED);
		// 進捗情報保存(処理は完了したので戻り値はチェックしない)
		CC_UpdDataProgressMng_SaveUpdStatus();
		// ダウンロード用テンポラリフォルダ削除(戻り値はチェックしない)
		CC_DeleteDir(dlTempDirPath);
	} while (0);

	// メモリ解放
	if (NULL != dlTempDirPath) {
		SCC_FREE(dlTempDirPath);
	}
	if (NULL != updData) {
		SCC_FREE(updData);
	}
	if (NULL != updAppData) {
		SCC_FREE(updAppData);
	}
	if (NULL != chkUpdInf) {
		SCC_FREE(chkUpdInf);
	}
	if (NULL != updStatus) {
		SCC_FREE(updStatus);
	}

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	if (SCC_IsCancel()) {
		ret = e_SC_RESULT_CANCEL;
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief エラー復帰チェック
 * @param[in] updStatus     地図データ更新状況
 * @param[in] dlTempDirPath ダウンロード用テンポラリフォルダのパス
 * @param[out] errComeback  異常復帰
 * @retval OK : e_SC_RESULT_SUCCESS
 * @retval NG : e_SC_RESULT_SUCCESS以外
 */
E_SC_RESULT CC_UpdateData_ComebackCheck(SMCAL *cal, const SMMAPUPDSTATUS *updStatus, const Char *dlTempDirPath, Bool *errComeback)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	Char	backetName[SCC_AWS_BACKETNAME_SIZE + 1] = {};
	Bool	backetNameCheck = false;
	Char	*filePath = NULL;
	Char	*saveFilePath = NULL;
	SMPROGRESSCBFNC	callbackFnc = {};
	FILE	*fp = NULL;
	FILE	*fp2 = NULL;
	//struct	stat st = {};
	Char	ver[SCC_VERSION_SIZE] = {};
	Char	ver2[SCC_VERSION_SIZE] = {};

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"errComeback=%d, " HERE, *errComeback);

	do {
		if (!*errComeback) {
			break;
		}

		// 進捗状況チェック
		if (CC_MAPUPDSTATUS_MAPDL == updStatus->status) {
			backetNameCheck = true;
		} else if ((CC_MAPUPDSTATUS_MAPUPDATE <= updStatus->status) &&
			(CC_MAPUPDSTATUS_DATAVERUPDATE >= updStatus->status) &&
			(updStatus->hasUpdate[SCC_MAPDWL_KIND_MAP])) {
			backetNameCheck = true;
		}

		// バケット名チェック
		if (backetNameCheck) {
			// AWSアクセス情報取得
			ret = CC_GetAWSBucketName(cal, &SmsApi_prm_t, e_HTTP_METHOD_GET, e_AWS_BACKET_MAP, backetName);
			if(e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetAWSBucketName error, " HERE);
				break;
			}

			// バケット名が一致するか比較する
			if (0 != strcmp(backetName, updStatus->backetName)) {
				// バケット名が不一致
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"backet name unmatch[aws=%s, lc=%s], " HERE, backetName, updStatus->backetName);
				// 更新途中の場合は、すべて破棄して最初からやり直す
				*errComeback = false;
				break;
			}
		}

		// メモリ確保
		filePath = (Char*)SCC_MALLOC(SCC_MAX_PATH);
		if (NULL == filePath) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		saveFilePath = (Char*)SCC_MALLOC(SCC_MAX_PATH);
		if (NULL == saveFilePath) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// コールバック関数のポインタ設定
		callbackFnc.cancel   = (SCC_CANCEL_FNC)SCC_IsCancel;
		callbackFnc.progress = NULL;

		// 仕向け地コード_dataVersion.txtファイルからバージョンを取得する
		sprintf(filePath, "%s%s_%s", dlTempDirPath, updStatus->regionCode, SCC_CMN_DATAVERSION);
		// ファイルオープン
		fp = fopen(filePath, "r");
		if (NULL == fp) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"fopen error[file=%s], " HERE, filePath);
			// 更新途中の場合は、すべて破棄して最初からやり直す
			*errComeback = false;
			break;
		}
		// ファイルリード
		if (sizeof(ver) != fread(ver, 1, sizeof(ver), fp)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"fread error[file=%s], " HERE, filePath);
			// 更新途中の場合は、すべて破棄して最初からやり直す
			*errComeback = false;
			break;
		}

		// ダウンロードデータのAWS上のパス生成
		sprintf(filePath, "%s%s/%s", SCC_CMN_AWS_DIR_APPDATA, updStatus->regionCode, SCC_CMN_DATAVERSION);
		// ダウンロードデータの保存先(端末)のパス生成
		sprintf(saveFilePath, "%s%s_%s", dlTempDirPath, updStatus->regionCode, SCC_CMN_DATAVERSION_NEW);

		// dataVersion.txtをダウンロードする
		ret = CC_Download(cal, &SmsApi_prm_t, filePath, saveFilePath, &callbackFnc, 0, 0);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_Download error[dl=%s], [save=%s], " HERE, filePath, saveFilePath);
			break;
		}
		// ファイルオープン
		fp2 = fopen(saveFilePath, "r");
		if (NULL == fp2) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"fopen error[file=%s], " HERE, saveFilePath);
			ret = e_SC_RESULT_FILE_OPENERR;
			break;
		}
		if (sizeof(ver2) != fread(ver2, 1, sizeof(ver2), fp2)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"fread error[file=%s], " HERE, filePath);
			ret = e_SC_RESULT_FILE_ACCESSERR;
			break;
		}

		// バージョンチェック
		if (0 != memcmp(ver, ver2, sizeof(ver))) {
			// 更新途中の場合は、すべて破棄して最初からやり直す
			*errComeback = false;
		}
	} while (0);

	// ファイルクローズ
	if (NULL != fp) {
		fclose(fp);
	}
	if (NULL != fp2) {
		fclose(fp2);
	}

	// メモリ解放
	if (NULL != filePath) {
		SCC_FREE(filePath);
	}
	if (NULL != saveFilePath) {
		SCC_FREE(saveFilePath);
	}

	if (e_SC_RESULT_SUCCESS == ret) {
		if (!*errComeback) {
			// 進捗クリア
			CC_UpdDataProgressMng_ClearUpdStatus();
		}
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}


/**
 * @brief 進捗状況取得
 * @param[out] dlProgressNum       ダウンロード済みサイズ(分子)
 * @param[out] dlProgressAllNum    ダウンロード対象総サイズ(分母)
 * @param[out] unzipProgressNum    tar.gz解凍済みサイズ(分子)
 * @param[out] unzipProgressAllNum tar.gz解凍対象総サイズ(分母)
 */
void CC_GetProgress(SMDLPROGRESS *dlProgress)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	do {
		// Mutexロック
		ret = SCC_LockMutex(mutexDL);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_LockMutex error, " HERE);
			break;
		}

		if (!CC_ISNULL(dlProgress)) {
			memcpy(dlProgress, &smDlProgress, sizeof(SMDLPROGRESS));
		}

		// Mutexアンロック
		ret = SCC_UnLockMutex(mutexDL);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_UnLockMutex error, " HERE);
			break;
		}
	} while (0);
}

//************************************************
//仕向け設定情報取得
//************************************************
/**
 * @brief 仕向け設定情報取得
 * @param[out] rgn_setting 仕向け設定情報
 * @retval OK : e_SC_RESULT_SUCCESS
 * @retval NG : e_SC_RESULT_SUCCESS以外
 */
E_SC_RESULT CC_GetRegionSetting(SMRGNSETTING *rgn_setting)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	Bool		restored = false;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメタチェック
		if (CC_ISNULL(rgn_setting)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetRegionSetting() param error[rgn_setting], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// application.iniファイルリード
		ret = CC_Read_application_ini(rgn_setting, false, &restored);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetRegionSetting() CC_Read_application_ini() error, " HERE);
			break;
		}
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"folder_num=%d, " HERE, rgn_setting->folder_num);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"Region=%s, " HERE, rgn_setting->dt_Folder[0].Region);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"Region=%s, " HERE, rgn_setting->dt_Folder[0].folder_Path);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"now_Region=%s, " HERE, rgn_setting->now_Region);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"restored=%d, " HERE, restored);

	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//仕向け地変更
//************************************************
/**
 * @brief 仕向け地変更
 * @param[in] rgnCode 仕向け地コード
 */
E_SC_RESULT CC_ChangeNowRegion(const Char *rgnCode)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SMRGNSETTING rgn_setting = {};
	Bool	restored = false;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメタチェック
		if (CC_ISINVALIDSTR(rgnCode)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ChangeNowRegion() param error[rgnCode], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// application.iniファイルリード
		ret = CC_Read_application_ini(&rgn_setting, true, &restored);
		if(e_SC_RESULT_SUCCESS != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ChangeNowRegion() CC_Read_application_ini error, " HERE);
			break;
		}

		// 仕向け地書換え
		memset(rgn_setting.now_Region, 0, SCC_MAPDWL_MAXCHAR_RGNCODE);
		strcpy(rgn_setting.now_Region, (const char*)rgnCode);

		// application.iniファイルライト
		ret = CC_Write_application_ini(&rgn_setting);
		if(e_SC_RESULT_SUCCESS != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ChangeNowRegion() CC_Write_application_ini error, " HERE);
			break;
		}
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"folder_num=%d, " HERE, rgn_setting.folder_num);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"Region=%s, " HERE, rgn_setting.dt_Folder[0].Region);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"Region=%s, " HERE, rgn_setting.dt_Folder[0].folder_Path);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"now_Region=%s, " HERE, rgn_setting.now_Region);

	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//プローブUP完了通知
//************************************************
/**
 * @brief プローブUP完了通知
 * @param[in] smcal_prb  SMCAL
 * @param[in] fileName   プローブデータファイル名
 * @param[in] bucketName アップロードバケット名
 * @retval OK : e_SC_RESULT_SUCCESS
 * @retval NG : e_SC_RESULT_SUCCESS以外
 */
E_SC_RESULT CC_NotifyProbePostComp(SMCAL *smcal_prb, const Char *fileName, const Char *bucketName)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	Char	*recvBuf = NULL;
	Char	apiSts[CC_CMN_XML_RES_STS_CODE_SIZE] = {};
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		if (CC_ISNULL(smcal_prb)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[smcal_prb], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISINVALIDSTR(fileName)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[fileName] " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISINVALIDSTR(bucketName)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[bucketName] " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// メモリ確保
		recvBuf = (Char*)SCC_MALLOC(CC_CMN_PROBE_RECIVE_BUFFER_SIZE);
		if (NULL == recvBuf) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			apiSts[0] = EOS;
			memset(recvBuf, 0, CC_CMN_PROBE_RECIVE_BUFFER_SIZE);

			// プローブUP完了通知
			ret = CC_ProbePostComp_SendRecv(smcal_prb,
											&SmsApi_prm_t,
											fileName,
											bucketName,
											recvBuf,
											CC_CMN_PROBE_RECIVE_BUFFER_SIZE,
											apiSts);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ProbePostComp_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiSts);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiSts)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, CC_CMN_PROBE_RECIVE_BUFFER_SIZE);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(smcal_prb, userSig, &SmsApi_prm_t, recvBuf, CC_CMN_PROBE_RECIVE_BUFFER_SIZE, apiSts2, false);
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
	} while (0);

	// メモリ解放
	if (NULL != recvBuf) {
		SCC_FREE(recvBuf);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief キーワードでGEM SPOT検索
 * @param[in] gemSpotSerch    GEM SPOT ID
 * @param[in] gemSpot         GEM SPOT情報リスト
 * @param[in] gemSpot         GEM SPOT情報数
 * @param[in] gemAllNum       GEM SPOT情報全件数
 * @param[in] apiStatus       APIステータスコード
 * @retval OK : e_SC_RESULT_SUCCESS
 * @retval NG : e_SC_RESULT_SUCCESS以外
 */
E_SC_RESULT CC_GemspotKwdSearch(const SMGEMSPOTKWDSEARCH *gemSpotSerch,
								SMGEMSPOT *gemSpot,
								INT32 *gemSpotNum,
								INT64 *gemAllNum,
								Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISNULL(gemSpotSerch)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemSpotSerch], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((!CC_ISINVALIDSTR(gemSpotSerch->keyword)) && (SCC_MAX_KEYWORD <= strlen(gemSpotSerch->keyword))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemSpotSerch->keyword], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((0 >= gemSpotSerch->maxCnt) || (CC_CMN_GEMSPOT_MAXNUM < gemSpotSerch->maxCnt)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemSpotSerch->maxCnt], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(gemSpot)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemSpot], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(gemSpotNum)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemSpotNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(gemAllNum)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemAllNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			*apiStatus = EOS;
			memset(gemSpot, 0, (sizeof(SMGEMSPOT) * gemSpotSerch->maxCnt));
			memset(recvBuf, 0, recvBufSize);

			// キーワードでGEM SPOT情報検索
			ret = CC_GemspotKwdSearch_SendRecv(cal,
											   &SmsApi_prm_t,
											   gemSpotSerch,
											   gemSpot,
											   gemSpotNum,
											   gemAllNum,
											   recvBuf,
											   recvBufSize,
											   apiStatus);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GemspotKwdSearch_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief GEM SPOT情報取得
 * @param[in] gemSpotId    GEM SPOT ID
 * @param[in] gemSpot      GEM SPOT情報
 * @param[in] apiStatus    APIステータスコード
 * @retval OK : e_SC_RESULT_SUCCESS
 * @retval NG : e_SC_RESULT_SUCCESS以外
 */
E_SC_RESULT CC_GemspotIdSearch(INT64 gemSpotId,
							   SMGEMSPOTINFO *gemSpotInfo,
							   Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISNULL(gemSpotInfo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemSpotInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			*apiStatus = EOS;
			memset(gemSpotInfo, 0, sizeof(SMGEMSPOTINFO));
			memset(recvBuf, 0, recvBufSize);

			// GEM SPOT情報取得
			ret = CC_GemspotIdSearch_SendRecv(cal,
											  &SmsApi_prm_t,
											  gemSpotId,
											  gemSpotInfo,
											  recvBuf,
											  recvBufSize,
											  apiStatus);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GemspotIdSearch_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief GCMプッシュ通知先の登録・削除
 * @param[in] pushInfo     GCMプッシュ登録情報
 * @param[in] apiStatus    APIステータスコード
 * @retval OK : e_SC_RESULT_SUCCESS
 * @retval NG : e_SC_RESULT_SUCCESS以外
 */
E_SC_RESULT CC_NotifyReg(const SMGCMPUSHINFO *pushInfo, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISNULL(pushInfo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemSpotInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((CC_CMN_GCM_OPERATION_DEL != pushInfo->operation) &&
			(CC_CMN_GCM_OPERATION_REG != pushInfo->operation)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[pushInfo->operation], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_CMN_GCM_NOTIFY_TYPE_CHAT_UNREAD != pushInfo->notifyType) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[pushInfo->notifyType], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_CMN_GCM_PLATFORM_TYPE_ANDROID != pushInfo->platformType) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[pushInfo->platformType], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISINVALIDSTR(pushInfo->deviceToken)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[pushInfo->deviceToken], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_CMN_GCM_REGISTRATION_ID <= strlen(pushInfo->deviceToken)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[pushInfo->deviceToken], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			*apiStatus = EOS;
			memset(pGcm_RecvBuf, 0, CC_CMN_GCM_RECIVE_BUFFER_SIZE);

			// GCMプッシュ通知先の登録・削除
			ret = CC_NotifyReg_SendRecv(&smcal_gcm,
										&SmsApi_prm_t,
										pushInfo,
										pGcm_RecvBuf,
										CC_CMN_GCM_RECIVE_BUFFER_SIZE,
										apiStatus);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_NotifyReg_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(pGcm_RecvBuf, 0, CC_CMN_GCM_RECIVE_BUFFER_SIZE);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(&smcal_gcm, userSig, &SmsApi_prm_t, pGcm_RecvBuf, CC_CMN_RECIVE_BUFFER_SIZE, apiSts2, false);
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
	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief Google開発者プロジェクトNo取得
 * @param[in] projectNo    Google開発者プロジェクトNo
 * @param[in] apiStatus    APIステータスコード
 * @retval OK : e_SC_RESULT_SUCCESS
 * @retval NG : e_SC_RESULT_SUCCESS以外
 */
E_SC_RESULT CC_PrjNoReq(Char *projectNo, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	//Char	*recvBuf = NULL;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISNULL(projectNo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[projectNo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			*apiStatus = EOS;
			*projectNo = EOS;
			memset(pGcm_RecvBuf, 0, CC_CMN_GCM_RECIVE_BUFFER_SIZE);

			// Google開発者プロジェクトNo取得
			ret = CC_PrjNoReq_SendRecv(&smcal_gcm,
									   &SmsApi_prm_t,
									   projectNo,
									   pGcm_RecvBuf,
									   CC_CMN_GCM_RECIVE_BUFFER_SIZE,
									   apiStatus);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_PrjNoReq_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(pGcm_RecvBuf, 0, CC_CMN_GCM_RECIVE_BUFFER_SIZE);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(&smcal_gcm, userSig, &SmsApi_prm_t, pGcm_RecvBuf, CC_CMN_GCM_RECIVE_BUFFER_SIZE, apiSts2, false);
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
	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief パーソナルお知らせ情報取得
 * @param[in] offset            取得データの開始位置
 * @param[in] limit             最大取得件数
 * @param[in] status            ステータス
 * @param[in] personalInfo      パーソナルお知らせ情報
 * @param[in] personalInfoNum   パーソナルお知らせ情報数
 * @param[in] apiStatus         APIステータスコード
 * @retval OK : e_SC_RESULT_SUCCESS
 * @retval NG : e_SC_RESULT_SUCCESS以外
 */
E_SC_RESULT CC_GetPersonalInfo(INT32 offset,
							   INT32 limit,
							   INT32 status,
							   SMPERSONALINFO *personalInfo,
							   INT32 *personalInfoNum,
							   Char *personalNum,
							   Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (0 > offset) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[offset], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((0 >= limit) || (SCC_CMN_PRSNLINFO_MAXNUM < limit)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[limit], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((SCC_CMN_PRSNLINFO_STS_ALL    != status) &&
			(SCC_CMN_PRSNLINFO_STS_READ   != status) &&
			(SCC_CMN_PRSNLINFO_STS_UNREAD != status)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[status], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(personalInfo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[personalInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(personalInfoNum)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[personalInfoNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(personalNum)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[personalNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			*apiStatus = EOS;
			memset(personalInfo, 0, (sizeof(SMPERSONALINFO) * limit));
			memset(recvBuf, 0, recvBufSize);

			// パーソナルお知らせ情報取得
			ret = CC_MessageInbox_SendRecv(cal,
										   &SmsApi_prm_t,
										   offset,
										   limit,
										   status,
										   personalInfo,
										   personalInfoNum,
										   personalNum,
										   recvBuf,
										   recvBufSize,
										   apiStatus);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MessageInbox_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief パーソナルお知らせ情報更新
 * @param[in] msgGuidList       お知らせIDリスト
 * @param[in] msgGuidListNum    お知らせIDリスト数
 * @param[in] type              更新種別
 * @param[in] apiStatus         APIステータスコード
 * @retval OK : e_SC_RESULT_SUCCESS
 * @retval NG : e_SC_RESULT_SUCCESS以外
 */
E_SC_RESULT CC_UpdatePersonalInfo(INT32 *msgGuidList,
								  INT32 msgGuidListNum,
								  INT32 type,
								  Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISNULL(msgGuidList)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[msgGuidList], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((0 >= msgGuidListNum) || (SCC_CMN_PRSNLINFO_MAXNUM < msgGuidListNum)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[msgGuidListNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((SCC_CMN_PRSNLINFO_TYPE_UPD_READ != type) && (SCC_CMN_PRSNLINFO_TYPE_DEL_INFO != type)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[type], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiSts], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			*apiStatus = EOS;
			memset(recvBuf, 0, recvBufSize);

			// パーソナルお知らせ情報更新
			ret = CC_MessageProcess_SendRecv(cal,
											 &SmsApi_prm_t,
											 msgGuidList,
											 msgGuidListNum,
											 type,
											 recvBuf,
											 recvBufSize,
											 apiStatus);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MessageProcess_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//サークル検索処理
//************************************************
/**
 * @brief サークル検索
 * @param[in]  type         種別
 * @param[in]  groupName    サークル名
 * @param[in]  offset       オフセット
 * @param[in]  limit        取得最大件数
 * @param[out] groupInfo    サークル情報
 * @param[out] groupInfoNum サークル情報数
 * @param[out]apiStatus APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GroupSearch(INT32 type,
						   const Char *groupName,
						   INT32 offset,
						   INT32 limit,
						   SMGROUP *groupInfo,
						   INT32 *groupInfoNum,
						   Char *groupNum,
						   Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if ((SCC_CMN_GROUP_TYPE_ALL    != type) &&
			(SCC_CMN_GROUP_TYPE_OWNER  != type) &&
			(SCC_CMN_GROUP_TYPE_MEMBER != type)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[type], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (0 > offset) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[offset], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((0 >= limit) || (SCC_CMN_GROUP_SEARCH_MAXNUM < limit)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[limit], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == groupNum) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[groupNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == groupInfo) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[groupInfoList], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == apiStatus) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			*groupInfoNum = 0;
			memset(groupInfo, 0, (sizeof(SMGROUP) * limit));
			*apiStatus = EOS;
			memset(recvBuf, 0, recvBufSize);

			// サークル検索
			ret = CC_GroupSearch_SendRecv(cal,
										  &SmsApi_prm_t,
										  type,
										  groupName,
										  offset,
										  limit,
										  groupInfo,
										  groupInfoNum,
										  groupNum,
										  recvBuf,
										  recvBufSize,
										  apiStatus);
			if (e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GroupSearch_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//GEMコメント処理
//************************************************
/**
 * @brief GEMコメント投稿
 * @param[in]  gemId        GEM ID
 * @param[in]  comment      コメント
 * @param[out] apiStatus    APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_RegGemComment(const Char *gemId, const Char *comment, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISINVALIDSTR(gemId)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemId], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISINVALIDSTR(comment)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[comment], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (SCC_CMN_GEM_COMMENT <= strlen((char*)comment)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[comment], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			*apiStatus = EOS;
			memset(recvBuf, 0, recvBufSize);

			// GEMコメント投稿
			ret = CC_GemCommentReg_SendRecv(cal,
											&SmsApi_prm_t,
											gemId,
											comment,
											recvBuf,
											recvBufSize,
											apiStatus);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GemCommentReg_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief GEMコメント変更
 * @param[in]  gemId        GEM ID
 * @param[in]  commentId    コメントID
 * @param[in]  comment      コメント
 * @param[out] apiStatus    APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_UpdGemComment(const Char *gemId, INT32 commentId, const Char *comment, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISINVALIDSTR(gemId)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemId], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISINVALIDSTR(comment)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[comment], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (SCC_CMN_GEM_COMMENT <= strlen((char*)comment)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[comment], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			*apiStatus = EOS;
			memset(recvBuf, 0, recvBufSize);

			// GEMコメント変更
			ret = CC_GemCommentAlt_SendRecv(cal,
											&SmsApi_prm_t,
											gemId,
											commentId,
											comment,
											recvBuf,
											recvBufSize,
											apiStatus);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GemCommentAlt_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief GEMコメント削除
 * @param[in]  gemId        GEM ID
 * @param[in]  commentId    コメントID
 * @param[out] apiStatus    APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_DelGemComment(const Char *gemId, INT32 commentId, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISINVALIDSTR(gemId)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemId], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			*apiStatus = EOS;
			memset(recvBuf, 0, recvBufSize);

			// GEMコメント削除
			ret = CC_GemCommentDel_SendRecv(cal,
											&SmsApi_prm_t,
											gemId,
											commentId,
											recvBuf,
											recvBufSize,
											apiStatus);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GemCommentDel_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief GEMコメント取得
 * @param[in]  gemId        GEM ID
 * @param[in]  limit        取得最大件数
 * @param[out] gemInfo      コメントID
 * @param[out] gemInfoNum   コメントID
 * @param[out] apiStatus    APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GetGemComment(const Char *gemId,
							 INT32 limit,
							 SMGEMCOMMENTINFO *gemInfo,
							 INT32 *gemInfoNum,
							 Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISINVALIDSTR(gemId)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemId], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((0 >= limit) || (SCC_CMN_GEM_COMMENTINFO_MAXNUM < limit)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[limit], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(gemInfo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(gemInfoNum)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemInfoNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			*gemInfoNum = 0;
			memset(gemInfo, 0, (sizeof(SMGEMCOMMENTINFO) * limit));
			*apiStatus = EOS;
			memset(recvBuf, 0, recvBufSize);

			// GEMコメント取得
			ret = CC_GemCommentReq_SendRecv(cal,
											&SmsApi_prm_t,
											gemId,
											limit,
											gemInfo,
											gemInfoNum,
											recvBuf,
											recvBufSize,
											apiStatus);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GemCommentReq_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief GEMにLikeをつけたユーザ取得
 * @param[in]  gemId        GEM ID
 * @param[in]  limit        取得最大件数
 * @param[out] likeInfo     GEMにLikeをつけたユーザ
 * @param[out] likeInfoNum  GEMにLikeをつけたユーザ数
 * @param[out] apiStatus    APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GetGemLike(const Char *gemId, INT32 limit, SMGEMLIKEINFO *likeInfo, INT32 *likeInfoNum, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISINVALIDSTR(gemId)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemId], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((0 >= limit) || (SCC_CMN_GEM_LIKEINFO_MAXNUM < limit)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[limit], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(likeInfo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[likeInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(likeInfoNum)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[likeInfoNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			*likeInfoNum = 0;
			memset(likeInfo, 0, (sizeof(SMGEMLIKEINFO) * limit));
			*apiStatus = EOS;
			memset(recvBuf, 0, recvBufSize);

			// GEMにLikeをつけたユーザ取得
			ret = CC_GemLikeReq_SendRecv(cal,
										 &SmsApi_prm_t,
										 gemId,
										 limit,
										 likeInfo,
										 likeInfoNum,
										 recvBuf,
										 recvBufSize,
										 apiStatus);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GemLikeReq_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief お知らせ情報一覧取得
 * @param[in]  lang         言語
 * @param[in]  type         取得種別
 * @param[in]  limit        取得最大件数
 * @param[out] noticeInfo   お知らせ情報一覧
 * @param[out] noticeInfoNum お知らせ情報一覧数
 * @param[out] apiStatus    APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GetNoticeList(const Char *lang,
							 INT32 type,
							 INT32 limit,
							 SMNOTICEINFO *noticeInfo,
							 INT32 *noticeInfoNum,
							 Char *noticeNum,
							 Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISINVALIDSTR(lang)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[lang], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((SCC_CMN_NOTICE_TYPE_LISTNUM != type) && (SCC_CMN_NOTICE_TYPE_LIST != type)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[type], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((0 >= limit) || (SCC_CMN_NOTICE_LIST_MAXNUM < limit)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[limit], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(noticeInfo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[noticeInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(noticeInfoNum)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[noticeInfoNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(noticeNum)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[noticeNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			*noticeInfoNum = 0;
			memset(noticeInfo, 0, (sizeof(SMNOTICEINFO) * limit));
			*apiStatus = EOS;
			memset(recvBuf, 0, recvBufSize);

			// お知らせ情報一覧取得
			ret = CC_NoticeListReq_SendRecv(cal,
											&SmsApi_prm_t,
											lang,
											type,
											limit,
											noticeInfo,
											noticeInfoNum,
											noticeNum,
											recvBuf,
											recvBufSize,
											apiStatus);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_NoticeListReq_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief お知らせ情報取得
 * @param[in]  noticeId     お知らせ情報ID
 * @param[out] notice       お知らせ情報
 * @param[out] apiStatus    APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GetNotice(INT32 noticeId,
						 Char *notice,
						 Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISNULL(notice)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[notice], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			memset(notice, 0, SCC_CMN_NOTICE_STR_SIZE);
			*apiStatus = EOS;
			memset(recvBuf, 0, recvBufSize);

			// お知らせ情報取得
			ret = CC_NoticeReq_SendRecv(cal,
										&SmsApi_prm_t,
										noticeId,
										notice,
										recvBuf,
										recvBufSize,
										apiStatus);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_NoticeReq_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief ユーザ検索
 * @param[in]  userSearch   検索条件
 * @param[out] userInfo     ユーザ情報
 * @param[out] apiStatus    APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_UserSearch(const SMUSERSRCH *userSearch,
							SMUSERINFO *userInfo,
							INT32 *userInfoNum,
							Char *userNum,
							Char *pApiSts)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメタチェック
		if(CC_ISNULL(userSearch)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[userSearch], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if((!CC_ISINVALIDSTR(userSearch->pOnly)) &&
		   (0 != strcmp(CC_USERSRCH_ONLY_FOLLOWER, userSearch->pOnly)) &&
		   (0 != strcmp(CC_USERSRCH_ONLY_FOLLOWEE, userSearch->pOnly))){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[userSearch->pOnly], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(0 > userSearch->offset){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[userSearch->offset], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if((0 >= userSearch->limit) || (SCC_USERSEARCH_MAXNUM < userSearch->limit)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[userSearch->limit], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if((!CC_ISINVALIDSTR(userSearch->pOrder)) &&
		   (0 != strcmp(CC_USERSRCH_ORDER_NEWEST, userSearch->pOrder)) &&
		   (0 != strcmp(CC_USERSRCH_ONLY_LAST_ACTION, userSearch->pOrder)) &&
		   (0 != strcmp(CC_USERSRCH_ONLY_GUID, userSearch->pOrder))){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[userSearch->pOrder], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(CC_ISNULL(userInfo)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[userInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(CC_ISNULL(userInfoNum)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[userInfoNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(CC_ISNULL(userNum)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[userNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if(CC_ISNULL(pApiSts)){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[pApiSts], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			memset(recvBuf, 0, recvBufSize);

			// ユーザ検索
			ret = CC_User_Srch_SendRecv(cal,
										&SmsApi_prm_t,
										userSearch,
										userInfo,
										userInfoNum,
										userNum,
										CC_SRCHUSER_RES_FILE,
										recvBuf,
										recvBufSize,
										pApiSts);
			if(e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_User_Srch_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiSts=%s, " HERE, pApiSts);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, pApiSts)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//認証情報取得処理
//************************************************
/**
 * @brief 認証情報取得
 * @param[out] termId
 * @param[out] termsig
 * @param[out] guid
 * @param[out] userSig
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GetAuthInfo(Char **termId, Char **termSig, Char **guid, Char **userSig)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	//Char	pUserInfo[SCC_MAX_PATH];

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISNULL(termId)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[termId], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(termSig)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[termSig], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(guid)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[guid], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(userSig)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[userSig], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		*termId  = (Char*)SmsApi_prm_t.ApiPrmMups.new_term_id;
		*termSig = (Char*)SmsApi_prm_t.ApiPrmMups.term_sig;
		*guid    = (Char*)SmsApi_prm_t.ApiPrmMups.guid;
		*userSig = (Char*)SmsApi_prm_t.ApiPrmMups.user_sig;
	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//認証セッションCookie取得処理
//************************************************
/**
 * @brief 認証セッションCookie取得
 * @param[in]  isReAuth         再認証するか否か(true:する、false:しない)
 * @param[out] sessionCookie    セッションCookie
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GetAuthSessionCookie(Bool isReAuth, Char **sessionCookie)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	//Char	pUserInfo[SCC_MAX_PATH] = {};
	Char	apiSts[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISNULL(sessionCookie)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[sessionCookie], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		if (isReAuth) {
			// 再認証する
			// ユーザーアクセスキー退避
			strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

			// 通信に必要なバッファ取得
			ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
				break;
			}

			// 再認証する
			ret = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts, false);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ReAuthreq error, " HERE);
				break;
			}
		}

		// セッションCookie取得
		*sessionCookie = (Char*)SmsApi_prm_t.ApiPrmUser.cookie;
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//アプリ評価機能
//************************************************
/**
 * @brief アプリ評価依頼ダイアログの表示有無を取得する
 * @return アプリ評価依頼ダイアログの表示有無( 0：表示する、1：表示しない)
 */
INT32 CC_GetShowRatingReqDialog()
{
	INT32	ret = SCC_CMN_RATING_DIALOG_NONSHOW;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	// ログイン済みの場合のみ
	if (CC_ISLOGINED()) {
		// アプリ評価依頼ダイアログの表示有無取得
		if (SCC_CMN_RATING_DIALOG_SHOW_INTERNAL == SmsApi_prm_t.ApiPrmUser.showRatingDialog) {
			ret = SCC_CMN_RATING_DIALOG_SHOW;
		} else {
			ret = SCC_CMN_RATING_DIALOG_NONSHOW;
		}
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief アプリ評価依頼ダイアログの表示有無を設定する
 * @return アプリ評価依頼ダイアログの表示有無( 0：表示する、1：表示しない)
 */
void CC_SetShowRatingReqDialog(INT32 isShowRatingReqDialog)
{
	if (SCC_CMN_RATING_DIALOG_SHOW == SmsApi_prm_t.ApiPrmUser.showRatingDialog) {
		// 表示する
		SmsApi_prm_t.ApiPrmUser.showRatingDialog = SCC_CMN_RATING_DIALOG_SHOW;
	} else {
		// 表示しない
		SmsApi_prm_t.ApiPrmUser.showRatingDialog = SCC_CMN_RATING_DIALOG_NONSHOW;
	}
}

/**
 * @brief アプリ評価状態を登録する
 * @param[in] rating        評価ステータス
 * @param[out] apiStatus    APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_RegRating(INT32 rating, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if ((SCC_CMN_RATING_DO       != rating) &&
			(SCC_CMN_RATING_FINISHED != rating) &&
			(SCC_CMN_RATING_LATER    != rating)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[rating], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			memset(recvBuf, 0, recvBufSize);

			// アプリ評価状態登録
			ret = CC_RatingReg_SendRecv(cal,
										&SmsApi_prm_t,
										rating,
										recvBuf,
										recvBufSize,
										apiStatus);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_RatingReg_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiSts=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief 通知メッセージを取得する
 * @param[in] lang          言語
 * @param[in] appVer        アプリバージョン
 * @param[out] dlgStatusInfo    通知メッセージ情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GetDaialogStatus(const Char *lang,
								const Char *appVer,
								T_CC_DAIALOGSTATUS_INFO *dlgStatusInfo)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;
	Char	ver[CC_CMN_APPVER_STR_SIZE] = {};
	Char	*ptr = NULL;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISNULL(lang)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[lang], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(appVer)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[appVer], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(dlgStatusInfo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[dlgStatusInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}
		memset(recvBuf, 0, recvBufSize);

		//-------------------------------------------
		//トークン／端末登録制御処理
		//-------------------------------------------
		ret = CC_Control_Token_RegistDevice(cal, &SmsApi_prm_t, recvBuf,  recvBufSize, dlgStatusInfo->apiStatus, false);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_Control_Token_RegistDevice error, " HERE);
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, dlgStatusInfo->apiStatus);
			break;
		}

		// 端末アプリVer
		ptr = strrchr(appVer, '.');
		if (NULL == ptr) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[appVer], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		strncpy(ver, (ptr + 1), 8);
		strncpy(&ver[8], (ptr + 10), 2);
		ver[10] = EOS;
		strcpy(SmsApi_prm_t.ApiPrmNavi.appVer, ver);

		memset(recvBuf, 0, recvBufSize);
		// 通知メッセージ取得
		ret = CC_DaialogStatusReq_SendRecv(cal,
										   &SmsApi_prm_t,
										   lang,
										   ver,
										   recvBuf,
										   recvBufSize,
										   dlgStatusInfo);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_DaialogStatusReq_SendRecv error, " HERE);
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, dlgStatusInfo->apiStatus);
			break;
		}
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}


/**
 * @brief AWSアクセス情報取得
 * @param [IN] smcal        SMCAL
 * @param [IN] backet       バケット種別
 * @param [IN] filePath     ファイルパス
 * @param [IN] aws          AWS情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GetAWSBucketInfo(SMCAL *smcal,
								E_HTTP_METHOD method,
								E_AWS_BUCKET backet,
								const Char *filePath,
								SMAWSINFO *aws)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISNULL(smcal)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[smcal], " HERE);
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

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// AWSアクセス情報取得
		ret = CC_GetAWSInfo(smcal, &SmsApi_prm_t, method, backet, filePath, aws);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetAWSInfo error, " HERE);
			break;
		}
	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief 運転特性診断結果を取得する
 * @param [IN]  tripId      トリップID
 * @param [OUT] filePath    センタから受信したレスポンスを格納したファイルのフルパス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GetDriveCheck(const Char *tripId, Char *filePath)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISNULL(tripId)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[tripId], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(filePath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[filePath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// 運転特性診断結果を取得する
		ret = CC_DriveCheckReq_SendRecv(cal,
										&SmsApi_prm_t,
										tripId,
										filePath,
										recvBuf,
										recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_DriveCheckReq_SendRecv error, " HERE);
			break;
		}
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief 運転特性診断結果一覧を取得する
 * @param [IN]  tripId      トリップID
 * @param [IN]  limit       取得件数
 * @param [OUT] filePath    センタから受信したレスポンスを格納したファイルのフルパス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GetDriveCheckList(const Char *tripId, INT32 limit, Char *filePath)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if ((0 >= limit) || (CC_DRIVECHECK_LIMIT < limit)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[limit], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(filePath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[filePath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// 運転特性診断結果一覧を取得する
		ret = CC_DriveCheckListReq_SendRecv(cal,
											&SmsApi_prm_t,
											tripId,
											limit,
											filePath,
											recvBuf,
											recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_DriveCheckListReq_SendRecv error, " HERE);
			break;
		}
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief ５星アンケートを更新する
 * @param [IN]  tripId      トリップID
 * @param [IN]  starNum     ユーザーが回答した星ポイント数(0 ～ 5)
 * @param [OUT] filePath    センタから受信したレスポンスを格納したファイルのフルパス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_UpdateDriveCheckStar(const Char *tripId, INT32 starNum, Char *filePath)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISNULL(tripId)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[tripId], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((CC_CMN_DRIVE_CHECK_START_MIN > starNum) ||
			(CC_CMN_DRIVE_CHECK_START_MAX < starNum)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[starNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(filePath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[filePath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ５星アンケートを更新する
		ret = CC_StarSrchAlt_SendRecv(cal,
									  &SmsApi_prm_t,
									  tripId,
									  starNum,
									  filePath,
									  recvBuf,
									  recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_StarsrchAlt_SendRecv error, " HERE);
			break;
		}
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief 駐車場情報を取得する
 * @param [in]  companyId   企業ID
 * @param [in]  storeId     店舗ID
 * @param [out] parking     駐車場情報
 * @param [out] apiStatus   APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GetParkingrate(INT32 companyId,
							  INT32 storeId,
							  SMSTOREPARKINGINFO *parking,
							  Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISNULL(parking)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[parking], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			memset(recvBuf, 0, recvBufSize);

			// 駐車場情報を取得する
			ret = CC_ParkingrateReq_SendRecv(cal,
												 &SmsApi_prm_t,
												 companyId,
												 storeId,
												 parking,
												 recvBuf,
												 recvBufSize,
												 apiStatus);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ParkingrateReq_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiSts=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief マッピング登録
 * @param [in]  mappingInfo マッピング情報
 * @param [out] apiStatus   APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_RegMapping(SMMAPPINGINFO *mappingInfo, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISNULL(mappingInfo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		if ((CC_CMN_MAPPING_GENRE_GEM > mappingInfo->genre) ||
			(CC_CMN_MAPPING_GENRE_TUBUYAKI < mappingInfo->genre)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingInfo->genre], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		} else if (CC_CMN_MAPPING_GENRE_TUBUYAKI == mappingInfo->genre) {
			// ジャンルがつぶやきの場合は、テキスト必須
			if (CC_ISINVALIDSTR(mappingInfo->text)) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingInfo->text], " HERE);
				ret = e_SC_RESULT_BADPARAM;
				break;
			}
		} else if (CC_CMN_MAPPING_GENRE_GEM == mappingInfo->genre) {
			// ジャンルがGEMの場合はテキストor画像ファイルのどちらか必須
			if ((CC_ISINVALIDSTR(mappingInfo->text)) && (CC_ISINVALIDSTR(mappingInfo->pic))) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingInfo->text or mappingInfo->pic], " HERE);
				ret = e_SC_RESULT_BADPARAM;
				break;
			}
		}
		if ((!CC_ISINVALIDSTR(mappingInfo->text)) && (CC_CMN_MAPPING_TEXT_SIZE <= strlen(mappingInfo->text))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingInfo->text], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((!CC_ISINVALIDSTR(mappingInfo->pic)) && (SCC_MAX_PATH <= strlen(mappingInfo->pic))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingInfo->pic], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((!CC_ISINVALIDSTR(mappingInfo->circleGuid)) &&
			(CC_CMN_MAPPING_CIRCLE_GUID_SIZE <= strlen(mappingInfo->circleGuid))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingInfo->circleGuid], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((CC_CMN_MAPPING_MODE_NONE > mappingInfo->mode) || (CC_CMN_MAPPING_MODE_ANGER < mappingInfo->mode)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingInfo->mode], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((CC_CMN_MAPPING_ACCESS_FLG_PRIVATE != mappingInfo->accessFlg) &&
			(CC_CMN_MAPPING_ACCESS_FLG_PUBLIC != mappingInfo->accessFlg)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[smMappingInfo->accessFlg], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			memset(recvBuf, 0, recvBufSize);

			// マッピング登録
			ret = CC_MappingReg_SendRecv(cal,
										 &SmsApi_prm_t,
										 mappingInfo,
										 recvBuf,
										 recvBufSize,
										 apiStatus);
			if(e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MappingReg_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiSts=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief マッピング更新
 * @param [in]  mappingInfo マッピング情報
 * @param [out] apiStatus   APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_UpdMapping(SMMAPPINGINFO *mappingInfo, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISNULL(mappingInfo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((CC_ISINVALIDSTR(mappingInfo->mappingId)) ||
			(CC_CMN_MAPPING_ID_SIZE <= strlen((char*)mappingInfo->mappingId))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingInfo->mappingId], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((CC_CMN_MAPPING_GENRE_GEM > mappingInfo->genre) ||
			(CC_CMN_MAPPING_GENRE_DISASTER < mappingInfo->genre)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingInfo->genre], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		} else if (CC_CMN_MAPPING_GENRE_TUBUYAKI == mappingInfo->genre) {
			// ジャンルがつぶやきの場合は、テキスト必須
			if (CC_ISINVALIDSTR(mappingInfo->text)) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingInfo->text], " HERE);
				ret = e_SC_RESULT_BADPARAM;
				break;
			}
		} else if (CC_CMN_MAPPING_GENRE_GEM == mappingInfo->genre) {
			// ジャンルがGEMの場合はテキストor画像ファイルのどちらか必須
			if (mappingInfo->pictureUpd) {
				// 画像変更あり
				if ((CC_ISINVALIDSTR(mappingInfo->text)) && (CC_ISINVALIDSTR(mappingInfo->pic))) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingInfo->text or mappingInfo->pic], " HERE);
					ret = e_SC_RESULT_BADPARAM;
					break;
				}
			}
		}
		if ((!CC_ISINVALIDSTR(mappingInfo->text)) && (CC_CMN_MAPPING_TEXT_SIZE <= strlen(mappingInfo->text))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingInfo->text], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((!CC_ISINVALIDSTR(mappingInfo->pic)) && (SCC_MAX_PATH <= strlen(mappingInfo->pic))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingInfo->pic], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((!CC_ISINVALIDSTR(mappingInfo->circleGuid)) &&
			(CC_CMN_MAPPING_CIRCLE_GUID_SIZE <= strlen(mappingInfo->circleGuid))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingInfo->circleGuid], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((CC_CMN_MAPPING_ACCESS_FLG_PRIVATE != mappingInfo->accessFlg) &&
			(CC_CMN_MAPPING_ACCESS_FLG_PUBLIC != mappingInfo->accessFlg)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[smMappingInfo->accessFlg], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((CC_CMN_MAPPING_PICTURE_NODEL != mappingInfo->pictureDel) &&
			(CC_CMN_MAPPING_PICTURE_DEL   != mappingInfo->pictureDel)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingInfo->pictureDel], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((CC_CMN_MAPPING_GENRE_GEM == mappingInfo->genre) ||
			(CC_CMN_MAPPING_GENRE_TUBUYAKI == mappingInfo->genre)) {
			if (0 > mappingInfo->cause) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingInfo->cause], " HERE);
				ret = e_SC_RESULT_BADPARAM;
				break;
			}
		} else {
			// ジャンルがGEM、つぶやき以外の場合は必須
			if (0 >= mappingInfo->cause) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingInfo->cause], " HERE);
				ret = e_SC_RESULT_BADPARAM;
				break;
			}
		}

		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			memset(recvBuf, 0, recvBufSize);

			// マッピング更新
			ret = CC_MappingAlt_SendRecv(cal,
										 &SmsApi_prm_t,
										 mappingInfo,
										 recvBuf,
										 recvBufSize,
										 apiStatus);
			if(e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MappingAlt_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiSts=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief マッピング削除
 * @param [in]  mappingId   マッピングID
 * @param [out] apiStatus   APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_DelMapping(const Char *mappingId, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if ((CC_ISINVALIDSTR(mappingId)) || (CC_CMN_MAPPING_ID_SIZE <= strlen((char*)mappingId))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingId], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			memset(recvBuf, 0, recvBufSize);

			// マッピング削除
			ret = CC_MappingDel_SendRecv(cal,
										 &SmsApi_prm_t,
										 mappingId,
										 recvBuf,
										 recvBufSize,
										 apiStatus);
			if(e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MappingDel_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiSts=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief マッピング検索
 * @param[in]  mappingSrch      マッピング検索条件
 * @param[out] mappingInfo      マッピング検索結果
 * @param[out] mappingInfoNum   マッピング検索結果取得数
 * @param[out] allNum           マッピング検索結果総数
 * @param[out] lastFlg          最終位置フラグ
 * @param[out] apiSts           APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_SearchMapping(const SMMAPPINGSRCH *mappingSrch,
							 SMMAPPINGSRCHRES *mappingInfo,
							 INT32 *mappingInfoNum,
							 LONG *allNum,
							 INT32 *lastFlg,
							 Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISNULL(mappingSrch)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingSrch], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((!CC_ISINVALIDSTR(mappingSrch->mappingId)) &&
			(CC_CMN_MAPPING_ID_SIZE <= strlen(mappingSrch->mappingId))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[gemSearch->mappingId], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((0 > mappingSrch->maxCnt) || (CC_CMN_MAPPING_SEARCH_MAXNUM < mappingSrch->maxCnt)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingSrch->maxCnt], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		for (num = 0; num < mappingSrch->genreNum; num++) {
			if ((CC_CMN_MAPPING_GENRE_GEM > mappingSrch->genre[num]) ||
				(CC_CMN_MAPPING_GENRE_DISASTER < mappingSrch->genre[num])) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingSrch->genre[%d]], " HERE, num);
				ret = e_SC_RESULT_BADPARAM;
				break;
			}
		}
		if ((CC_CMN_MAPPING_ACCESS_FLG_NONE != mappingSrch->accessFlg) &&
			(CC_CMN_MAPPING_ACCESS_FLG_OWN  != mappingSrch->accessFlg)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingSrch->accessFlg], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((CC_CMN_MAPPING_FOLLOWER_FLG_NONE != mappingSrch->followerFlg) &&
			(CC_CMN_MAPPING_FOLLOWER_FLG_FOLLOW != mappingSrch->followerFlg)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingSrch->followerFlg], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((NULL != mappingSrch->groupId) && ((EOS != *mappingSrch->groupId))) {
			if (CC_CMN_MAPPING_CIRCLE_GUIDS_SIZE <= strlen((char*)mappingSrch->groupId)) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingSrch->groupId], " HERE);
				ret = e_SC_RESULT_BADPARAM;
				break;
			}
		}
		if ((!CC_ISINVALIDSTR(mappingSrch->keyword)) &&
			(CC_CMN_MAPPING_KEYWORD_SIZE <= strlen((char*)mappingSrch->keyword))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingSrch->keyword], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_CMN_MAPPING_SRCH_RATING_NONE > mappingSrch->rating) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingSrch->rating], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((CC_CMN_MAPPING_SORT_NEW != mappingSrch->sort) &&
			(CC_CMN_MAPPING_SORT_LIKE != mappingSrch->sort)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingSrch->sort], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(mappingInfo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(mappingInfoNum)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingInfoNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(allNum)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[allNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(lastFlg)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[lastFlg], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			*mappingInfoNum = 0;
			memset(mappingInfo, 0, (sizeof(SMMAPPINGSRCHRES) * mappingSrch->maxCnt));
			*apiStatus = EOS;
			memset(recvBuf, 0, recvBufSize);
			*lastFlg = 1;

			// マッピング検索
			ret = CC_MappingSrch_SendRecv(cal,
										  &SmsApi_prm_t,
										  mappingSrch,
										  mappingInfo,
										  mappingInfoNum,
										  allNum,
										  lastFlg,
										  recvBuf,
										  recvBufSize,
										  apiStatus);
			if (e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MappingSrch_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief マッピングID検索
 * @param[in]  mappingId        マッピングID
 * @param[out] mappingInfo      マッピング検索結果
 * @param[out] apiSts           APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_SearchMappingId(const Char *mappingId,
							   SMMAPPINGIDSRCHRES *mappingInfo,
							   Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if ((CC_ISINVALIDSTR(mappingId)) || (CC_CMN_MAPPING_ID_SIZE <= strlen(mappingId))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingId], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(mappingInfo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			memset(mappingInfo, 0, sizeof(SMMAPPINGIDSRCHRES));
			*apiStatus = EOS;
			memset(recvBuf, 0, recvBufSize);

			// マッピングID検索
			ret = CC_MappingIdSrch_SendRecv(cal,
											&SmsApi_prm_t,
											mappingId,
											mappingInfo,
											recvBuf,
											recvBufSize,
											apiStatus);
			if (e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MappingIdSrch_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief マッピング編集履歴取得
 * @param[in]  mappingId        マッピングID
 * @param[in]  startPos         マッピングID
 * @param[in]  maxCnt           マッピングID
 * @param[out] mappingInfo      マッピング編集履歴情報
 * @param[out] mappingInfoNum   マッピング編集履歴情報取得数
 * @param[out] allNum           マッピング編集履歴情報総数
 * @param[out] apiSts           APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GetMappingHistory(const Char *mappingId,
								 INT32 startPos,
								 INT32 maxCnt,
								 SMMAPPINGHISTORY *mappingInfo,
								 INT32 *mappingInfoNum,
								 LONG *allNum,
								 Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if ((CC_ISINVALIDSTR(mappingId)) || (CC_CMN_MAPPING_ID_SIZE <= strlen((char*)mappingId))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingId], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (0 > startPos) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[startPos], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((0 >= maxCnt) || (CC_CMN_MAPPING_HISTORY_MAXNUM < maxCnt)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[maxCnt], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(mappingInfo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(mappingInfoNum)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingInfoNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(allNum)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[allNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			memset(mappingInfo, 0, sizeof(SMMAPPINGHISTORY));
			*apiStatus = EOS;
			memset(recvBuf, 0, recvBufSize);

			// マッピング編集履歴取得
			ret = CC_MappingHistoryReq_SendRecv(cal,
												&SmsApi_prm_t,
												mappingId,
												startPos,
												maxCnt,
												mappingInfo,
												mappingInfoNum,
												allNum,
												recvBuf,
												recvBufSize,
												apiStatus);
			if (e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MappingHistoryReq_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief マッピングコメント投稿
 * @param[in]  mappingId    マッピングID
 * @param[in]  comment      コメント
 * @param[out] apiSts       APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_RegMappingComment(const Char *mappingId,
								 const Char *comment,
								 Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if ((CC_ISINVALIDSTR(mappingId)) || (CC_CMN_MAPPING_ID_SIZE <= strlen(mappingId))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingId], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((CC_ISINVALIDSTR(comment)) || (CC_CMN_MAPPING_COMMENT_SIZE <= strlen(comment))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[comment], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			memset(recvBuf, 0, recvBufSize);

			// マッピングコメント登録
			ret = CC_MappingCommentReg_SendRecv(cal,
												&SmsApi_prm_t,
												mappingId,
												comment,
												recvBuf,
												recvBufSize,
												apiStatus);
			if(e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MappingCommentReg_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiSts=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief マッピングコメント更新
 * @param[in]  mappingId    マッピングID
 * @param[in]  commentId    コメントID
 * @param[in]  comment      コメント
 * @param[out] apiSts       APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_UpdMappingComment(const Char *mappingId,
								 INT32 commentId,
								 const Char *comment,
								 Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if ((CC_ISINVALIDSTR(mappingId)) || (CC_CMN_MAPPING_ID_SIZE <= strlen(mappingId))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingId], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((CC_ISINVALIDSTR(comment)) || (CC_CMN_MAPPING_COMMENT_SIZE <= strlen(comment))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[comment], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			memset(recvBuf, 0, recvBufSize);

			// マッピングコメント更新
			ret = CC_MappingCommentAlt_SendRecv(cal,
												&SmsApi_prm_t,
												mappingId,
												commentId,
												comment,
												recvBuf,
												recvBufSize,
												apiStatus);
			if(e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MappingCommentAlt_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiSts=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief マッピングコメント削除
 * @param[in]  mappingId    マッピングID
 * @param[in]  commentId    コメントID
 * @param[in]  comment      コメント
 * @param[out] apiSts       APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_DelMappingComment(const Char *mappingId,
								 INT32 commentId,
								 Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if ((CC_ISINVALIDSTR(mappingId)) || (CC_CMN_MAPPING_ID_SIZE <= strlen(mappingId))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingId], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			memset(recvBuf, 0, recvBufSize);

			// マッピングコメント削除
			ret = CC_MappingCommentDel_SendRecv(cal,
												&SmsApi_prm_t,
												mappingId,
												commentId,
												recvBuf,
												recvBufSize,
												apiStatus);
			if(e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MappingCommentDel_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiSts=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief マッピングコメント取得
 * @param[in]  mappingId    マッピングID
 * @param[in]  startPos     マッピングID
 * @param[in]  maxCnt       マッピングID
 * @param[out] commentInfo  マッピングコメント情報
 * @param[out] commentInfoNum   マッピングコメント情報取得数
 * @param[out] allNum       マッピングコメント情報総数
 * @param[out] apiSts       APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GetMappingComment(const Char *mappingId,
								 INT32 startPos,
								 INT32 maxCnt,
								 SMMAPPINGCOMMENTINFO *commentInfo,
								 INT32 *commentInfoNum,
								 LONG *allNum,
								 Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if ((CC_ISINVALIDSTR(mappingId)) || (CC_CMN_MAPPING_ID_SIZE <= strlen(mappingId))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingId], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (0 > startPos) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[startPos], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((0 >= maxCnt) || (CC_CMN_MAPPING_HISTORY_MAXNUM < maxCnt)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[maxCnt], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(commentInfo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[commentInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(commentInfoNum)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[commentInfoNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(allNum)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[allNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			memset(commentInfo, 0, sizeof(SMMAPPINGCOMMENTINFO));
			*apiStatus = EOS;
			memset(recvBuf, 0, recvBufSize);

			// マッピングコメント取得
			ret = CC_MappingCommentReq_SendRecv(cal,
												&SmsApi_prm_t,
												mappingId,
												startPos,
												maxCnt,
												commentInfo,
												commentInfoNum,
												allNum,
												recvBuf,
												recvBufSize,
												apiStatus);
			if (e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MappingCommentReq_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief マッピングにLIKEを付ける
 * @param[in]  mappingId    マッピングID
 * @param[out] likeCnt      LIKE数
 * @param[out] apiSts       APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_RegMappingLike(const Char *mappingId, LONG *likeCnt, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if ((CC_ISINVALIDSTR(mappingId)) || (CC_CMN_MAPPING_ID_SIZE <= strlen(mappingId))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingId], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(likeCnt)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[likeCnt], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			*apiStatus = EOS;
			memset(recvBuf, 0, recvBufSize);

			// マッピングにLIKEを付ける
			ret = CC_MappingLikeReg_SendRecv(cal,
											 &SmsApi_prm_t,
											 mappingId,
											 likeCnt,
											 recvBuf,
											 recvBufSize,
											 apiStatus);
			if (e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MappingLikeReg_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief マッピングに付けたLIKEを取り消す
 * @param[in]  mappingId    マッピングID
 * @param[out] likeCnt      LIKE数
 * @param[out] apiSts       APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_DelMappingLike(const Char *mappingId, LONG *likeCnt, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if ((CC_ISINVALIDSTR(mappingId)) || (CC_CMN_MAPPING_ID_SIZE <= strlen(mappingId))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingId], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(likeCnt)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[likeCnt], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			*apiStatus = EOS;
			memset(recvBuf, 0, recvBufSize);

			// マッピングにつけたLIKEを取り消す
			ret = CC_MappingLikeDel_SendRecv(cal,
											 &SmsApi_prm_t,
											 mappingId,
											 likeCnt,
											 recvBuf,
											 recvBufSize,
											 apiStatus);
			if (e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MappingLikeDel_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief マッピングにLikeをつけたユーザを取得する
 * @param[in]  mappingId    マッピングID
 * @param[in]  startPos     取得開始位置
 * @param[in]  maxCnt       取得最大件数
 * @param[out] likeInfo     マッピングににLikeをつけたユーザ情報
 * @param[out] likeInfoNum  マッピングににLikeをつけたユーザ情報取得数
 * @param[out] allNum       マッピングににLikeをつけたユーザ情報総数
 * @param[out] apiStatus    APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GetMappingLike(const Char *mappingId,
							  INT32 startPos,
							  INT32 maxCnt,
							  SMMAPPINGLIKEINFO *likeInfo,
							  INT32 *likeInfoNum,
							  LONG *allNum,
							  Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if ((CC_ISINVALIDSTR(mappingId)) || (CC_CMN_MAPPING_ID_SIZE <= strlen(mappingId))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingId], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (0 > startPos) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[startPos], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((0 >= maxCnt) || (CC_CMN_MAPPING_LIKE_MAXNUM < maxCnt)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[maxCnt], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(likeInfo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[likeInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(likeInfoNum)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[likeInfoNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(allNum)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[allNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			*likeInfoNum = 0;
			memset(likeInfo, 0, (sizeof(SMMAPPINGLIKEINFO) * maxCnt));
			*apiStatus = EOS;
			memset(recvBuf, 0, recvBufSize);

			// マッピングにLikeをつけたユーザを取得する
			ret = CC_MappingLikeReq_SendRecv(cal,
											 &SmsApi_prm_t,
											 mappingId,
											 startPos,
											 maxCnt,
											 likeInfo,
											 likeInfoNum,
											 allNum,
											 recvBuf,
											 recvBufSize,
											 apiStatus);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MappingLikeReq_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief マッピングに評価をつける
 * @param[in]  mappingId    マッピングID
 * @param[in]  ratingOwn    本人評価値
 * @param[out] rating       評価値
 * @param[out] apiStatus    APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_RegMappingRate(const Char *mappingId, INT32 ratingOwn, Char *rating, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if ((CC_ISINVALIDSTR(mappingId)) || (CC_CMN_MAPPING_ID_SIZE <= strlen(mappingId))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingId], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((CC_CMN_MAPPING_RATE_NO > ratingOwn) || (CC_CMN_MAPPING_RATE_YES < ratingOwn)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[ratingOwn], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(rating)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[rating], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			*apiStatus = EOS;
			memset(recvBuf, 0, recvBufSize);

			// マッピングに評価をつける
			ret = CC_MappingRateReg_SendRecv(cal,
											 &SmsApi_prm_t,
											 mappingId,
											 ratingOwn,
											 rating,
											 recvBuf,
											 recvBufSize,
											 apiStatus);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MappingRateReg_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief マッピングに評価をつけたユーザを取得する
 * @param[in]  mappingId    マッピングID
 * @param[in]  ratingType   評価種別
 * @param[in]  startPos     取得開始位置
 * @param[in]  maxCnt       取得最大件数
 * @param[out] likeInfo     マッピングににLikeをつけたユーザ情報
 * @param[out] likeInfoNum  マッピングににLikeをつけたユーザ情報取得数
 * @param[out] allNum       マッピングににLikeをつけたユーザ情報総数
 * @param[out] apiStatus    APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GetMappingRate(const Char *mappingId,
							  INT32 ratingType,
							  INT32 startPos,
							  INT32 maxCnt,
							  SMMAPPINGRATEINFO *rateInfo,
							  INT32 *rateInfoNum,
							  LONG *allNum,
							  Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if ((CC_ISINVALIDSTR(mappingId)) || (CC_CMN_MAPPING_ID_SIZE <= strlen(mappingId))) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[mappingId], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((CC_CMN_MAPPING_RATE_NO > ratingType) && (CC_CMN_MAPPING_RATE_YES < ratingType)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[rateType], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (0 > startPos) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[startPos], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((0 >= maxCnt) || (CC_CMN_MAPPING_RATE_MAXNUM < maxCnt)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[maxCnt], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(rateInfo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[rateInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(rateInfoNum)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[rateInfoNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(allNum)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[allNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			*rateInfoNum = 0;
			memset(rateInfo, 0, (sizeof(SMMAPPINGRATEINFO) * maxCnt));
			*apiStatus = EOS;
			memset(recvBuf, 0, recvBufSize);

			// マッピングに評価をつけたユーザを取得する
			ret = CC_MappingRateReq_SendRecv(cal,
											 &SmsApi_prm_t,
											 mappingId,
											 ratingType,
											 startPos,
											 maxCnt,
											 rateInfo,
											 rateInfoNum,
											 allNum,
											 recvBuf,
											 recvBufSize,
											 apiStatus);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MappingRateReq_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief アクセスコードを登録する
 * @param [in]  accessCode  アクセスコード
 * @param [in]  dirPath     アイコンファイル格納ディレクトリのフルパス
 * @param [out] pkgInfo     パッケージ一覧
 * @param [out] apiStatus   APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_RegAccessCode(const Char *accessCode,
							 const Char *dirPath,
							 SMPACKAGEGROUPINFO *pkgInfo,
							 Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	INT32	num2 = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISINVALIDSTR(accessCode)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[accessCode], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_CMN_PKGMGR_ACCESSCODE < strlen(accessCode)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[accessCode], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISINVALIDSTR(dirPath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[dirPath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_CMN_PKGMGR_PATH <= strlen(dirPath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[accessCode], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(pkgInfo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[pkgInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			memset(recvBuf, 0, recvBufSize);

			// アクセスコードを登録する
			ret = CC_PkgCodeReg_SendRecv(cal,
										 &SmsApi_prm_t,
										 accessCode,
										 dirPath,
										 pkgInfo,
										 recvBuf,
										 recvBufSize,
										 apiStatus);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_PkgCodeReg_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiSts=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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

		for (num = 0; ((pkgListNum < (CC_CMN_PKGMGR_GROUP_MAXNUM * CC_CMN_PKGMGR_PACKAGE_MAXNUM)) && (num < pkgInfo->pkgInfoNum)); num++) {
			// 空きエリアを検索する
			for (num2 = 0; num2 < (CC_CMN_PKGMGR_GROUP_MAXNUM * CC_CMN_PKGMGR_PACKAGE_MAXNUM); num2++) {
				if (EOS == pkgList[num2].packageFileName[0]) {
					break;
				}
			}
			if ((CC_CMN_PKGMGR_GROUP_MAXNUM * CC_CMN_PKGMGR_PACKAGE_MAXNUM) == num2) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"pkgList full, " HERE);
				ret = e_SC_RESULT_FAIL;
				break;
			}
//			// グループID
//			strcpy((char*)pkgList[num2].groupId, (char*)pkgInfo->groupId);
//			// パッケージID
//			strcpy((char*)pkgList[num2].packageId, (char*)pkgInfo->pkgInfo[num].packageId);
			// アクセスコード
			strcpy((char*)pkgList[num2].accessCode, (char*)accessCode);
			// アイコンファイル
			strcpy((char*)pkgList[num2].iconFileName, (char*)pkgInfo->pkgInfo[num].iconFileName);
			// パッケージファイル
			strcpy((char*)pkgList[num2].packageFileName, (char*)pkgInfo->pkgInfo[num].packageFileName);
			// パッケージバージョン
			strcpy((char*)pkgList[num2].packageVer, (char*)pkgInfo->pkgInfo[num].version);

			// S3にアクセスするための情報を内部バッファにコピーする
			// バケット名
			strcpy((char*)pkgList[num2].backetName, (char*)pkgInfo->pkgInfo[num].backetName);
			// アクセスキー
			strcpy((char*)pkgList[num2].accessKey,  (char*)pkgInfo->pkgInfo[num].accessKey);
			// シークレットキー
			strcpy((char*)pkgList[num2].secretKey,  (char*)pkgInfo->pkgInfo[num].secretKey);
			pkgListNum++;
		}
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief アクセスコードを解除する
 * @param [in]  accessCode  アクセスコード
 * @param [out] apiStatus   APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_ReleaseAccessCode(const Char *accessCode, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISINVALIDSTR(accessCode)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[accessCode], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_CMN_PKGMGR_ACCESSCODE < strlen(accessCode)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[accessCode], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			memset(recvBuf, 0, recvBufSize);

			// アクセスコードを解除する
			ret = CC_PkgCodeDel_SendRecv(cal,
										 &SmsApi_prm_t,
										 accessCode,
										 recvBuf,
										 recvBufSize,
										 apiStatus);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_PkgCodeDel_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiSts=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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

		for (num = 0; num < (CC_CMN_PKGMGR_GROUP_MAXNUM * CC_CMN_PKGMGR_PACKAGE_MAXNUM); num++) {
			if ((EOS != pkgList[num].accessCode) &&
				(0 == strcmp((char*)pkgList[num].accessCode, (char*)accessCode))) {
				// アクセスコードが一致するパッケージ情報を削除する
				memset(&pkgList[num], 0, sizeof(SMPACKAGEINFO_INTERNAL));
				pkgListNum--;
			}
		}
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief パッケージ一覧を取得する(通常モード)
 * @param [in]  strDirPath  パッケージファイルとアイコンファイルの格納ディレクトリのフルパス
 * @param [out] pkgInfo     パッケージ一覧
 * @param [out] pkgInfoNum  パッケージ数
 * @param [out] apiStatus   APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GetPackageList(const Char *dirPath,
							  Bool isDownload,
							  SMPACKAGEGROUPINFO *pkgInfo,
							  INT32 *pkgInfoNum,
							  Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	INT32	num2 = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;
	Bool	isDL = isDownload;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISINVALIDSTR(dirPath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[dirPath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_CMN_PKGMGR_PATH <= strlen(dirPath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[accessCode], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(pkgInfo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[pkgInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(pkgInfo->pkgInfo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[pkgInfo->pkgInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(pkgInfoNum)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[pkgInfoNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		if (!isDL) {
			if (0 == pkgListNum) {
				isDL = true;
			}
		}

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			memset(recvBuf, 0, recvBufSize);
			*pkgInfoNum = 0;

			// パッケージ一覧を取得する
			ret = CC_PkgReq_SendRecv(cal,
									 &SmsApi_prm_t,
									 dirPath,
									 isDL,
									 pkgInfo,
									 pkgInfoNum,
									 recvBuf,
									 recvBufSize,
									 apiStatus);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_PkgReq_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiSts=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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

		// パッケージ情報クリア
		memset(pkgList, 0, sizeof(pkgList));
		for (num = 0, pkgListNum = 0; num < *pkgInfoNum; num++) {
			for (num2 = 0; num2 < pkgInfo[num].pkgInfoNum; num2++) {
//				// グループID
//				strcpy((char*)pkgList[pkgListNum].groupId, (char*)pkgInfo[num].groupId);
//				// パッケージID
//				strcpy((char*)pkgList[pkgListNum].packageId, (char*)pkgInfo[num].pkgInfo[num2].packageId);
				// アイコンファイル
				strcpy((char*)pkgList[pkgListNum].iconFileName, (char*)pkgInfo[num].pkgInfo[num2].iconFileName);
				// パッケージファイル
				strcpy((char*)pkgList[pkgListNum].packageFileName, (char*)pkgInfo[num].pkgInfo[num2].packageFileName);
				// パッケージバージョン
				strcpy((char*)pkgList[pkgListNum].packageVer, (char*)pkgInfo[num].pkgInfo[num2].version);

				// S3にアクセスするための情報を内部バッファにコピーする
				// バケット名
				strcpy((char*)pkgList[pkgListNum].backetName, (char*)pkgInfo[num].pkgInfo[num2].backetName);
				// アクセスキー
				strcpy((char*)pkgList[pkgListNum].accessKey,  (char*)pkgInfo[num].pkgInfo[num2].accessKey);
				// シークレットキー
				strcpy((char*)pkgList[pkgListNum].secretKey,  (char*)pkgInfo[num].pkgInfo[num2].secretKey);
				pkgListNum++;
			}
		}
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief パッケージ一覧を取得する(管理者モード)
 * @param [in]  strDirPath  パッケージファイルとアイコンファイルの格納ディレクトリのフルパス
 * @param [out] pkgInfo     パッケージ一覧
 * @param [out] pkgInfoNum  パッケージ数
 * @param [out] apiStatus   APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GetPackageListMgr(const Char *dirPath, SMPACKAGEGROUPINFO *pkgInfo)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISINVALIDSTR(dirPath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[dirPath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_CMN_PKGMGR_PATH <= strlen(dirPath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[accessCode], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(pkgInfo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[pkgInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(pkgInfo->pkgInfo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[pkgInfo->pkgInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 初期化
		pkgInfo->pkgInfoNum = 0;

		// パッケージ一覧取得
		ret = CC_PkgList(dirPath, pkgInfo->pkgInfo, &pkgInfo->pkgInfoNum);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_PkgList error, " HERE);
			break;
		}
	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief パッケージをロック（アンインストール）する
 * @param [in]  packagePath パッケージフォルダのフルパス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_LockPackage(const Char *packagePath)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISINVALIDSTR(packagePath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[packagePath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_CMN_PKGMGR_PATH <= strlen(packagePath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[packagePath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// ディレクトリ削除
		ret = CC_DeleteDir(packagePath);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_DeleteDir error[path=%s], " HERE, packagePath);
			break;
		}
	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief パッケージをアンロック（インストール）する(通常モード)
 * @param [in]  pkgInfo     パッケージ情報
 * @param [in]  packagePath パッケージフォルダのフルパス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_UnLockPackage(SMPACKAGEINFO *pkgInfo, const Char *packagePath)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;
	Char	*dlDirPath = NULL;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISNULL(pkgInfo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[pkgInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISINVALIDSTR(packagePath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[packagePath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_CMN_PKGMGR_PATH <= strlen(packagePath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[packagePath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// パッケージ情報取得
		if (!CC_GetPackageInfo(pkgInfo->packageFileName, pkgInfo)) {
			ret = e_SC_RESULT_FAIL;
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"package info not found, " HERE);
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// メモリ確保
		dlDirPath = (Char*)SCC_MALLOC(SCC_MAX_PATH);
		if (NULL == dlDirPath) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		if ('/' != packagePath[strlen(packagePath) - 1]) {
			sprintf((char*)dlDirPath, "%s/", packagePath);
		} else {
			sprintf((char*)dlDirPath, "%s", packagePath);
		}

		// ディレクトリ作成
		ret = CC_MakeDir(dlDirPath);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MakeDir error[%s], " HERE, dlDirPath);
			break;
		}

		if (!CC_ISINVALIDSTR(pkgInfo->iconFileName)) {
			// アイコンファイルダウンロード
			ret = CC_DownloadPackage(cal, &SmsApi_prm_t, PKGDLDATATYPE_ICON, pkgInfo, dlDirPath);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_PkgUnLock error, " HERE);
				// エラーでも処理を継続する
				ret = e_SC_RESULT_SUCCESS;
			}
		}

		// パッケージファイルダウンロード
		ret = CC_DownloadPackage(cal, &SmsApi_prm_t, PKGDLDATATYPE_PKG, pkgInfo, dlDirPath);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_PkgUnLock error, " HERE);
			break;
		}
	} while (0);

	// メモリ解放
	if (NULL != dlDirPath) {
		SCC_FREE(dlDirPath);
	}

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief パッケージをアンロック（コピー）する(管理者モード)
 * @param [in]  pkgInfo     パッケージ情報
 * @param [in]  packagePath パッケージフォルダのフルパス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_UnLockPackageMgr(SMPACKAGEINFO *pkgInfo, const Char *packagePath)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	Char	*srcDirPath = NULL;
	Char	*dstDirPath = NULL;
	Char	*chr = NULL;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISNULL(pkgInfo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[pkgInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISINVALIDSTR(packagePath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[packagePath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_CMN_PKGMGR_PATH <= strlen(packagePath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[packagePath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// メモリ確保
		srcDirPath = (Char*)SCC_MALLOC(SCC_MAX_PATH);
		if (NULL == srcDirPath) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		dstDirPath = (Char*)SCC_MALLOC(SCC_MAX_PATH);
		if (NULL == dstDirPath) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// コピー元ディレクトリ名取得
		strcpy((char*)srcDirPath, (char*)pkgInfo->packageFileName);
		if ('/' == srcDirPath[strlen(srcDirPath) - 1]) {
			srcDirPath[strlen(srcDirPath) - 1] = EOS;
		}
		chr = strrchr(srcDirPath, '/');
		if (NULL == chr) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[pkgInfo->packageFileName], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		chr++;

		// ディレクトリパス生成
		if ('/' == packagePath[strlen(packagePath) - 1]) {
			sprintf((char*)dstDirPath, "%s%s/", packagePath, chr);
		} else {
			sprintf((char*)dstDirPath, "%s/%s/", packagePath, chr);
		}
		strcat(srcDirPath, "/");

		// ディレクトリ作成
		ret = CC_MakeDir(dstDirPath);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_MakeDir error[%s], " HERE, dstDirPath);
			break;
		}

		// パッケージアンロック（コピー）
		ret = SCC_CopyDir(srcDirPath, dstDirPath, false);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_PkgUnLock error, " HERE);
			break;
		}
	} while (0);

	// メモリ解放
	if (NULL != srcDirPath) {
		SCC_FREE(srcDirPath);
	}
	if (NULL != dstDirPath) {
		SCC_FREE(dstDirPath);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief パスワードを変更する
 * @param [in]  password    現在のパスワード
 * @param [in]  newPassword 新パスワード
 * @param [out] apiStatus   APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_UpdatePassword(const Char *password,
							  const Char *newPassword,
							  Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	//INT32	num2 = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;
	Bool	mutexLock = false;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISINVALIDSTR(password)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[password], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_CMN_PASSWORD_STR_SIZE <= strlen(password)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[password], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISINVALIDSTR(newPassword)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[newPassword], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_CMN_PASSWORD_STR_SIZE <= strlen(newPassword)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[newPassword], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			memset(recvBuf, 0, recvBufSize);

			if (!mutexLock) {
				// Mutexロック
				ret = SCC_LockMutex(mutexAuth);
				if (e_SC_RESULT_SUCCESS != ret) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_LockMutex error, " HERE);
					break;
				}
				mutexLock = true;
			}

			// パスワードを変更する
			ret = CC_PasswdAlt_SendRecv(cal, &SmsApi_prm_t, password, newPassword, recvBuf, recvBufSize, apiStatus);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_PasswdAlt_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiSts=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// Mutexアンロック
					ret2 = SCC_UnLockMutex(mutexAuth);
					if (e_SC_RESULT_SUCCESS != ret2) {
						SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_UnLockMutex error, " HERE);
						break;
					}
					mutexLock = false;

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
		// パスワードを新パスワードに上書きする
		strcpy(SmsApi_prm_t.ApiPrmUser.password, newPassword);
	} while (0);

	if (mutexLock) {
		// Mutexアンロック
		ret2 = SCC_UnLockMutex(mutexAuth);
		if (e_SC_RESULT_SUCCESS != ret2) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_UnLockMutex error, " HERE);
		}
	}

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief パスワードを取得する
 * @param [in]  password    パスワード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GetPassword(Char *password)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISNULL(password)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[password], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// パスワード取得
		strcpy(password, SmsApi_prm_t.ApiPrmUser.password);
	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief プローブ情報取得
 * @param[in]  probeSrch        プローブ情報検索条件
 * @param[out] probeInfo        プローブ情報
 * @param[out] probeInfoNum     プローブ情報数
 * @param[out] apiSts           APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GetProbeInfo(const SMPROBESRCH *probeSrch,
							SMPROBEINFO *probeInfo,
							SMPROBEINFONUM *probeInfoNum,
							Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISNULL(probeSrch)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[probeSrch], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISINVALIDSTR(probeSrch->fromTime)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[probeSrch->fromTime], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_CMN_PROBEINFO_TIME != strlen((char*)probeSrch->fromTime)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[probeSrch->fromTime], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISINVALIDSTR(probeSrch->toTime)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[probeSrch->toTime], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_CMN_PROBEINFO_TIME != strlen((char*)probeSrch->toTime)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[probeSrch->toTime], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (0 < strcmp((char*)probeSrch->fromTime, (char*)probeSrch->toTime)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[probeSrch->fromTime > probeSrch->toTime], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(probeInfo)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[probeInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(probeInfoNum)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[probeInfoNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			memset(probeInfo, 0, (sizeof(SMPROBEINFO) * CC_CMN_PROBEINFO_MAXNUM));
			memset(probeInfoNum, 0, sizeof(SMPROBEINFONUM));
			*apiStatus = EOS;
			memset(recvBuf, 0, recvBufSize);

			// プローブ情報取得
			ret = CC_ProbeReq_SendRecv(cal,
									   &SmsApi_prm_t,
									   probeSrch,
									   probeInfo,
									   probeInfoNum,
									   recvBuf,
									   recvBufSize,
									   apiStatus);
			if (e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ProbeReq_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(recvBuf, 0, recvBufSize);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiSts2, false);
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
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief 交通情報取得
 * @param[in]  trafficSrch      交通情報検索条件
 * @param[out] trafficInfo      交通情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GetTrafficInfo(const SMTRAFFICSRCH *trafficSrch, SMTRAFFICINFO *trafficInfo)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiStatus[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == trafficSrch) {
			ret = e_SC_RESULT_BADPARAM;
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[trafficSrch], " HERE);
			break;
		}
		if ((0 == trafficSrch->parcelIdNum) || (CC_CMN_TRAFFICINFO_PARCEL_MAXNUM < trafficSrch->parcelIdNum)) {
			ret = e_SC_RESULT_BADPARAM;
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[trafficSrch->parcelIdNum], " HERE);
			break;
		}
		if (CC_CMN_TRAFFICINFO_ROAD_KIND_MAXNUM < trafficSrch->roadKindNum) {
			ret = e_SC_RESULT_BADPARAM;
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[trafficSrch->roadKindNum], " HERE);
			break;
		}
		for (num = 0; num < trafficSrch->roadKindNum; num++) {
			if (CC_CMN_TRAFFICINFO_ROAD_KIND_MAX < (INT32)trafficSrch->roadKind[num]) {
				ret = e_SC_RESULT_BADPARAM;
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[trafficSrch->roadKind[%d]], " HERE, num);
				break;
			}
		}
		if (e_SC_RESULT_SUCCESS != ret) {
			break;
		}
		if (NULL == trafficInfo) {
			ret = e_SC_RESULT_BADPARAM;
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[trafficInfo], " HERE);
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			memset(trafficInfo, 0, sizeof(SMTRAFFICINFO));
			memset(recvBuf, 0, recvBufSize);

			// 交通情報取得
			ret = CC_TrafficInfo_SendRecv(cal,
										  &SmsApi_prm_t,
										  trafficSrch,
										  trafficInfo,
										  recvBuf,
										  recvBufSize);
			if (e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_TrafficInfo_SendRecv error, " HERE);
				// 初期化
				memset(recvBuf, 0, recvBufSize);
				*apiStatus = EOS;

				// 再認証(必要な場合のみ再認証する)
				ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiStatus, false);
				if (e_SC_RESULT_SUCCESS != ret2) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ReAuthreq error, " HERE);
					break;
				}
				continue;
			}
			break;
		}
		if (e_SC_RESULT_SUCCESS != ret) {
			break;
		}
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief OAuth連携用URLを取得する
 * @param[in] serviceId     サービスID
 * @param[out] url          OAuth連携用URL
 * @param[out] sessionId    PHPセッションID
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GetOAuthUrl(const Char *serviceId, Char *url, Char *sessionId, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	//INT32	num = 0;
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISINVALIDSTR(serviceId)) {
			ret = e_SC_RESULT_BADPARAM;
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[serviceId], " HERE);
			break;
		}
		if (CC_CMN_OAUTH_SERVICE_ID != strlen(serviceId)) {
			ret = e_SC_RESULT_BADPARAM;
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[serviceId], " HERE);
			break;
		}
		if (CC_ISNULL(url)) {
			ret = e_SC_RESULT_BADPARAM;
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[url], " HERE);
			break;
		}
		if (CC_ISNULL(sessionId)) {
			ret = e_SC_RESULT_BADPARAM;
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[sessionId], " HERE);
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			ret = e_SC_RESULT_BADPARAM;
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[apiStatus], " HERE);
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// 初期化
		url[0] = EOS;
		sessionId[0] = EOS;
		memset(recvBuf, 0, recvBufSize);

		// OAuth連携用URL取得
		ret = CC_OAuthUrlReq_SendRecv(cal,
									  &SmsApi_prm_t,
									  serviceId,
									  url,
									  sessionId,
									  recvBuf,
									  recvBufSize,
									  apiStatus);
		if (e_SC_RESULT_SUCCESS != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_OAuthUrlReq_SendRecv error, " HERE);
			break;
		}
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief パッケージ汎用API
 * @param[in] param         APIのパラメータ
 * @param[out] jsonFilePath ポータルAPIのレスポンスを格納したJSONファイルのパス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_ReqPkgCommon(const SMPKGCOMMONPARAM *param, Char *jsonFilePath)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiStatus[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISNULL(param)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[param], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((CC_PKGCOMMON_FRAMEWORK_ELLG != param->framework) &&
			(CC_PKGCOMMON_FRAMEWORK_LARAVEL != param->framework)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[param->framework], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((CC_PKGCOMMON_METHOD_GET != param->method) &&
			(CC_PKGCOMMON_METHOD_POST != param->method)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[param->method], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		if (CC_ISNULL(jsonFilePath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[jsonFilePath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

#ifdef CC_CMN_PKGCMN_DOCHECK	// パッケージをチェックする
		// パッケージ使用可能リスト取得
		if (0 == pkgWhiteListNum) {
			ret = e_SC_RESULT_FAIL;
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"package white list nothing, " HERE);
			break;
		}
#endif							// パッケージをチェックする

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			*jsonFilePath = EOS;
			memset(recvBuf, 0, recvBufSize);

			// パッケージ汎用API
			ret = CC_PkgCommon_SendRecv(cal,
										&SmsApi_prm_t,
										param,
										pkgWhiteList,
										pkgWhiteListNum,
										jsonFilePath,
										recvBuf,
										recvBufSize);
			if (e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_PkgCommon_SendRecv error, " HERE);
				// 初期化
				memset(recvBuf, 0, recvBufSize);
				*apiStatus = EOS;

				// 再認証(必要な場合のみ再認証する)
				ret2 = CC_ReAuthreq(cal, userSig, &SmsApi_prm_t, recvBuf, recvBufSize, apiStatus, false);
				if (e_SC_RESULT_SUCCESS != ret2) {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ReAuthreq error, " HERE);
					break;
				}
				continue;
			}
			break;
		}
		if (e_SC_RESULT_SUCCESS != ret) {
			break;
		}
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief パケージ改ざんチェック
 * @param[in] pkgName       パッケージ名
 * @param[in] pkgVer        パッケージバージョン
 * @param[in] pkgDirPath    パッケージディレクトリのフルパス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_CheckTamperingPkg(const Char *pkgName, const Char *pkgVer, const Char *pkgDirPath)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
#ifdef CC_CMN_PKGCMN_DOCHECK	// パッケージをチェックする
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;
	SMPACKAGEINFO	*pkgInfo = NULL;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISINVALIDSTR(pkgName)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[pkgName], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISINVALIDSTR(pkgVer)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[pkgVer], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISINVALIDSTR(pkgDirPath)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[pkgDirPath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		// メモリ確保
		pkgInfo = (SMPACKAGEINFO*)SCC_MALLOC(sizeof(SMPACKAGEINFO));
		if (NULL == pkgInfo) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// パッケージ情報取得
		if (!CC_PkgCmn_GetPackageInfo(pkgName, pkgVer, pkgInfo)) {
			ret = e_SC_RESULT_FAIL;
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_PkgCmn_GetPackageInfo error, " HERE);
			break;
		}

		// 改ざんチェック
		ret = CC_Pkg_ChkTampering(cal, &SmsApi_prm_t, pkgInfo, pkgDirPath);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_Pkg_ChkTampering error, " HERE);
			break;
		}
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	// メモリ解放
	if(NULL != pkgInfo){
		SCC_FREE(pkgInfo);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
#endif							// 改ざんチェックする

	return (ret);
}

/**
 * @brief ホワイトリスト取得
 * @param[in] pkgName       パッケージ名
 * @param[in] pkgVer        パッケージバージョン
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GetPkgWhiteList(const Char *pkgName, const Char *pkgVer)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
#ifdef CC_CMN_PKGCMN_DOCHECK	// パッケージをチェックする
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};
	SMCAL	*cal = NULL;
	Char	*recvBuf = NULL;
	INT32	recvBufSize = 0;
	SMPACKAGEINFO	*pkgInfo = NULL;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISINVALIDSTR(pkgName)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[pkgName], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISINVALIDSTR(pkgVer)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[pkgVer], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 通信に必要なバッファ取得
		ret = CC_GetComBuff(&cal, &recvBuf, &recvBufSize);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_GetComBuff error, " HERE);
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		// メモリ確保
		pkgInfo = (SMPACKAGEINFO*)SCC_MALLOC(sizeof(SMPACKAGEINFO));
		if (NULL == pkgInfo) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// パッケージ情報取得
		if (!CC_PkgCmn_GetPackageInfo(pkgName, pkgVer, pkgInfo)) {
			ret = e_SC_RESULT_FAIL;
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_PkgCmn_GetPackageInfo error, " HERE);
			break;
		}

		// ホワイトリスト取得
		ret = CC_Pkg_GetPkgWhiteList(cal, &SmsApi_prm_t, pkgInfo, pkgWhiteList, &pkgWhiteListNum);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_Pkg_GetPkgWhiteList error, " HERE);
			break;
		}
	} while (0);

	// 通信に必要なバッファ解放
	if (NULL != cal) {
		CC_FreeComBuff(cal);
	}

	// メモリ解放
	if(NULL != pkgInfo){
		SCC_FREE(pkgInfo);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
#endif							// パッケージをチェックする

	return (ret);
}





//---------------------------------------------------------------------------------
//内部関数
//---------------------------------------------------------------------------------
//************************************************
//SMS API関連パラメタテーブル初期設定処理
//************************************************
/**
 * @brief SMS API関連パラメタテーブル初期設定
 * @param[in] pTerm_id 登録用TermID文字列ポインタ
 * @param[in] pItem_id 初期商品ID文字列ポインタ
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @pre 起動後、最初に１回コールする
 */
E_SC_RESULT CC_SMS_API_initSetPrm(const Char* pTerm_id,
									const Char* pItem_id)
{
	E_SC_RESULT ret = CC_CMN_RESULT_OK;

	do{

		// ユーザ情報ファイル読出し
		CC_ReadUserInfoFile();

		memset(SmsApi_prm_t.ApiPrmNavi.common_uri, 0, CC_CMN_COMMON_URI_MAX_STR_SIZE);
		strcpy(SmsApi_prm_t.ApiPrmNavi.common_uri, CC_CMN_XML_BASIC_CENTER_URI);			// センタURI(各APIで共通の前半部分)設定

		memset(SmsApi_prm_t.ApiPrmNavi.sms_sp_uri, 0, CC_CMN_COMMON_URI_MAX_STR_SIZE);
		strcpy(SmsApi_prm_t.ApiPrmNavi.sms_sp_uri, CC_CMN_XML_BASIC_CENTER_SMS_SP_URI);		// センタURI(各APIで共通の前半部分)設定

		memset(SmsApi_prm_t.ApiPrmMups.common_uri, 0, CC_CMN_COMMON_URI_MAX_STR_SIZE);
		strcpy(SmsApi_prm_t.ApiPrmMups.common_uri, CC_CMN_XML_BASIC_MUPS_CENTER_URI);		// MUPS系API用センタURI(各APIで共通の前半部分)設定

		memset(SmsApi_prm_t.ApiPrmNavi.reg_term_id, 0, CC_CMN_TERMID_STR_SIZE);
		strcpy(SmsApi_prm_t.ApiPrmNavi.reg_term_id, pTerm_id);								// 登録用端末ID設定

		memset(SmsApi_prm_t.ApiPrmNavi.init_item_id, 0, CC_CMN_INIT_ITEMID_STR_SIZE);
		strcpy(SmsApi_prm_t.ApiPrmNavi.init_item_id, pItem_id);								// 初期商品ID設定

//		memset(SmsApi_prm_t.ApiPrmMups.new_term_id, 0, CC_CMN_TARGETID_STR_SIZE);
//		strcpy(SmsApi_prm_t.ApiPrmMups.new_term_id, SmsApi_prm_t.ApiPrmNavi.reg_term_id);	// センター採番新規端末IDに登録用端末IDを設定 (初回のToken.reqの端末IDは登録用端末IDのため)

//		memset(SmsApi_prm_t.ApiPrmUser.lang, 0, CC_CMN_LANG_STR_SIZE);
//		strcpy(SmsApi_prm_t.ApiPrmUser.lang, pLang);										// 言語設定

		memcpy(SmsApi_prm_t.ApiPrmUser.login_id, bkLoginId, sizeof(bkLoginId));
		SmsApi_prm_t.ApiPrmUser.login_id[sizeof(bkLoginId) - 1] = EOS;
		memcpy(SmsApi_prm_t.ApiPrmUser.password, bkPassword, sizeof(bkPassword));
		SmsApi_prm_t.ApiPrmUser.password[sizeof(bkPassword) - 1] = EOS;

		// ユーザ情報ファイル退避
		CC_SaveUserInfoFile();

	}while(0);

	return (ret);
}

//************************************************
//ユーザー情報退避処理
//************************************************
/**
 * @brief ユーザー情報退避
 * @param[in] pUsername		ユーザー名文字列ポインタ
 * @param[in] pMail			メールアドレス文字列ポインタ
 * @param[in] pId			ログインID文字列ポインタ
 * @param[in] pPassword		パスワード文字列ポインタ
 * @param[in] pLang			言語設定文字列ポインタ
 * @retval 正常終了  :CC_CMN_RESULT_OK
 */
E_SC_RESULT CC_SaveUserInfo(const Char* pUsername,
							const Char* pMail,
							const Char* pId,
							const Char* pPassword,
							const Char* pLang)
{
	E_SC_RESULT ret = CC_CMN_RESULT_OK;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do{

		memset(SmsApi_prm_t.ApiPrmUser.user_name, 0, CC_CMN_USERNAME_STR_SIZE);
		strcpy(SmsApi_prm_t.ApiPrmUser.user_name, pUsername);		// ユーザー名

		memset(SmsApi_prm_t.ApiPrmUser.mail_addr, 0, CC_CMN_MALEADDR_STR_SIZE);
		strcpy(SmsApi_prm_t.ApiPrmUser.mail_addr, pMail);			// メールアドレス

		memset(SmsApi_prm_t.ApiPrmUser.login_id, 0, CC_CMN_LOGINID_STR_SIZE);
		strcpy(SmsApi_prm_t.ApiPrmUser.login_id, pId);				// ログインID

		memset(SmsApi_prm_t.ApiPrmUser.password, 0, CC_CMN_PASSWORD_STR_SIZE);
		strcpy(SmsApi_prm_t.ApiPrmUser.password, pPassword);		// パスワード

		memset(SmsApi_prm_t.ApiPrmUser.lang, 0, CC_CMN_LANG_STR_SIZE);
		strcpy(SmsApi_prm_t.ApiPrmUser.lang, pLang);				// 言語設定

		// ユーザ情報ファイル退避
		CC_SaveUserInfoFile();

	}while(0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//ユーザー情報退避処理(ユーザー認証用)
//************************************************
/**
 * @brief ユーザー情報退避
 * @param[in] pId			ログインID文字列ポインタ
 * @param[in] pPassword		パスワード文字列ポインタ
 * @retval 正常終了  :CC_CMN_RESULT_OK
 */
void CC_SaveUserInfo_Auth(const Char* pId,
						  const Char* pPassword)
{
	memset(SmsApi_prm_t.ApiPrmUser.login_id, 0, CC_CMN_LOGINID_STR_SIZE);
	strcpy(SmsApi_prm_t.ApiPrmUser.login_id, pId);				// ログインID
	strcpy(bkLoginId, pId);

	memset(SmsApi_prm_t.ApiPrmUser.password, 0, CC_CMN_PASSWORD_STR_SIZE);
	strcpy(SmsApi_prm_t.ApiPrmUser.password, pPassword);		// パスワード
	strcpy(bkPassword, pPassword);

	// ここではファイルに保存しない
	// ユーザ情報ファイル退避
	//CC_SaveUserInfoFile();
}

//************************************************
//トークン／端末登録制御処理
//************************************************
/**
 * @brief トークン／端末登録制御
 * @param[in] pCal センタ通信ライブラリパラメタテーブルポインタ
 * @param[in] pPrm SMS API関連パラメタテーブルポインタ
 * @param[in] pRecv_Buf 受信バッファポインタ
 * @param[in] RecvBuf_sz 受信バッファサイズ
 * @param[out] pApiSts APIステータスコード
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @note Token.req送受信後、センタ採番新規端末IDの有無をチェックして、無かった場合(端末未登録)に
 *       Deviceid.reqを送受信して端末登録を行う。
 */
E_SC_RESULT CC_Control_Token_RegistDevice(SMCAL* pCal,
								T_CC_CMN_SMS_API_PRM* pPrm,
								Char* pRecv_Buf,
								UINT32 RecvBuf_sz,
								Char* pApiSts,
								Bool isPolling)
{
	E_SC_RESULT ret = CC_CMN_RESULT_OK;

	do{
//***************** ↓ MUPSサーバ停止による暫定対策   MUPSサーバ復活後削除すること  2014.07.17  ***************************************************************
#if 1
		strcpy(pPrm->ApiPrmMups.new_term_id, "SM0000000000");
		strcpy(pPrm->ApiPrmMups.term_sig, "1234567890123456");
#else
//***************** ↑ MUPSサーバ停止による暫定対策   MUPSサーバ復活後削除すること  2014.07.17  ***************************************************************
		memset(pRecv_Buf, 0, RecvBuf_sz);			// 受信バッファクリア

		// 端末未登録の場合 (センタ採番新規端末IDの有無で判定)
		if(0 == pPrm->ApiPrmMups.new_term_id[0]) {

			memset(pRecv_Buf, 0, RecvBuf_sz);			// 受信バッファクリア

			//-------------------------------------------
			//Deviceid.req送信・受信処理
			//-------------------------------------------
			ret = CC_DeviceidReq_SendRecv(pCal, pPrm, pRecv_Buf, RecvBuf_sz, pApiSts, isPolling);
			if(CC_CMN_RESULT_OK != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_Control_Token_RegistDevice() CC_DeviceidReq_SendRecv() error, " HERE);
				break;
			}

			memset(pRecv_Buf, 0, RecvBuf_sz);			// 受信バッファクリア

			//-------------------------------------------
			//Token.req送信・受信処理
			//-------------------------------------------
			ret = CC_TokenReq_SendRecv(pCal, pPrm, pRecv_Buf, RecvBuf_sz, pApiSts, isPolling);
			if(CC_CMN_RESULT_OK != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_Control_Token_RegistDevice() CC_TokenReq_SendRecv() error, " HERE);
				break;
			}

		} else {

			//-------------------------------------------
			//Token.req送信・受信処理
			//-------------------------------------------
			ret = CC_TokenReq_SendRecv(pCal, pPrm, pRecv_Buf, RecvBuf_sz, pApiSts);
			if(CC_CMN_RESULT_OK != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_Control_Token_RegistDevice() CC_TokenReq_SendRecv() error, " HERE);
				break;
			}

		}
//***************** ↓ MUPSサーバ停止による暫定対策   MUPSサーバ復活後削除すること  2014.07.17  ***************************************************************
#endif
//***************** ↑ MUPSサーバ停止による暫定対策   MUPSサーバ復活後削除すること  2014.07.17  ***************************************************************

	}while(0);

	return (ret);
}

//************************************************
//ユーザー再認証処理
//************************************************
/**
 * @brief ユーザー認証 (チャット履歴情報取得専用)
 * @param[in] pId ログインID文字列ポインタ
 * @param[in] pPassword パスワード文字列ポインタ
 * @param[out] pApiSts APIステータスコード
 * @retval 正常終了  :CC_CMN_RESULT_OK
 * @retval エラー終了:CC_CMN_RESULT_OK以外 (コール関数の戻り値)
 */
E_SC_RESULT CC_ReAuthenticationUser(SMCAL* pCal,
									const Char* pId,
									const Char* pPassword,
									Char* pRecv_Buf,
									UINT32 RecvBuf_sz,
									Char* pApiSts,
									Bool isPolling)
{
	//INT32 optinfnum;
	E_SC_RESULT ret = CC_CMN_RESULT_OK;
	Char login_id[CC_CMN_LOGINID_STR_SIZE] = {};
	Char password[CC_CMN_PASSWORD_STR_SIZE] = {};

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do{

		//-------------------------------------------
		// パラメータチェック
		//-------------------------------------------
		if (CC_ISINVALIDSTR(pId)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_ReAuthenticationUser() param error[pId], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISINVALIDSTR(pPassword)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_ReAuthenticationUser() param error[pPassword], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISNULL(pRecv_Buf)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_ReAuthenticationUser() param error[pRecv_Buf], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (0 == RecvBuf_sz) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_ReAuthenticationUser() param error[RecvBuf_sz], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISNULL(pApiSts)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_ReAuthenticationUser() param error[pApiSts], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}

		//-------------------------------------------
		//ユーザ認証期限切れ時の再ユーザ認証時用の対策処理
		//-------------------------------------------
		strcpy(login_id, pId);				// ログインIDコピー
		strcpy(password, pPassword);		// パスワードコピー

		//-------------------------------------------
		//ユーザー情報退避(認証用)
		//-------------------------------------------
		CC_SaveUserInfo_Auth(login_id, password);

		//-------------------------------------------
		//トークン／端末登録制御処理
		//-------------------------------------------
		ret = CC_Control_Token_RegistDevice(pCal, &SmsApi_prm_t, pRecv_Buf,  RecvBuf_sz, pApiSts, isPolling);
		if(CC_CMN_RESULT_OK != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_ReAuthenticationUser() CC_Control_Token_RegistDevice() error, " HERE);
			break;
		}

		memset(pRecv_Buf, 0, RecvBuf_sz);			// 受信バッファクリア

		//-------------------------------------------
		///Auth.req送信・受信処理
		//-------------------------------------------
		ret = CC_AuthReq_SendRecv(pCal, &SmsApi_prm_t, pRecv_Buf, RecvBuf_sz, pApiSts, isPolling);
		if(CC_CMN_RESULT_OK != ret){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_ReAuthenticationUser() CC_AuthReq_SendRecv() error, " HERE);
			break;
		}
		SCC_Login();

		// ユーザ情報ファイル退避
		CC_SaveUserInfoFile();

	}while(0);


	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//application.iniファイルリード
//************************************************
/**
 * @brief application.iniファイルリード
 * @param[out] rgn_setting 仕向け設定情報構造体ポインタ
 * @retval OK : e_SC_RESULT_SUCCESS
 * @retval NG : e_SC_RESULT_SUCCESS以外
 * @note application.iniを読込んで、仕向け設定情報構造体にセットして返す。
 *       application.iniが無い場合(初期状態)は、ファイルを作成する。
 */
E_SC_RESULT CC_Read_application_ini(SMRGNSETTING *rgn_setting, Bool isCreate, Bool *restored)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	Char	*filePath = NULL;
	Char	*dbFilePath = NULL;
	FILE	*fp = NULL;
	INT32	result = 0;
	struct	stat st = {};
	INT32	stat_ret = 0;
	INT32	num = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメタチェック
		if (CC_ISNULL(rgn_setting)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_Read_application_ini() param error[rgn_setting], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(restored)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_Read_application_ini() param error[restored], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 初期化
		memset(rgn_setting, 0, sizeof(SMRGNSETTING));
		*restored = false;

		if (!CC_ISEOS(SCC_GetConfigDirPath())) {
			// PATH用メモリ確保
			filePath = (Char*)SCC_MALLOC(SCC_MAX_PATH);
			if (NULL == filePath) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_Read_application_ini() SCC_MALLOC() error, " HERE);
				ret = e_SC_RESULT_MALLOC_ERR;
				break;
			}
			dbFilePath = (Char*)SCC_MALLOC(SCC_MAX_PATH);
			if (NULL == dbFilePath) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_Read_application_ini() SCC_MALLOC() error, " HERE);
				ret = e_SC_RESULT_MALLOC_ERR;
				break;
			}

			//-------------------------------------------
			// Configフォルダ作成
			//-------------------------------------------
			// Configパス
			strcpy((char*)filePath, SCC_GetConfigDirPath());
			stat_ret = stat((const char*)filePath, &st);
			// Configフォルダが無い場合
			if(0 != stat_ret){
				CC_MakeDir(filePath);							// Configフォルダ作成
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"CC_Read_application_ini() Config folder create!, " HERE);
			}

			// application.iniファイルフルパス
			strcat((char*)filePath, SCC_MAPDWL_APPLICATION_INI_FILE_NAME);
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"CC_Read_application_ini() filePath = %s, " HERE, filePath);

			// ファイルオープン
			fp = fopen(filePath, "rb");
			if(NULL != fp){
				// application.iniファイルから、仕向け設定情報構造体読出し
				result = fread(rgn_setting, 1, sizeof(SMRGNSETTING), fp);
				if(sizeof(SMRGNSETTING) == result){
					break;
				} else {
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_Read_application_ini() fread() read_size = %d error, " HERE, result);
					fclose(fp);
					fp = NULL;
				}
			}
			rgn_setting->folder_num = 0;
			for (num = 0; num < CC_CMN_REGION_NUM; num++) {
				sprintf(dbFilePath, "%s%s%s/%s", SCC_GetRootDirPath(), CC_CMN_MAP_DIR_PATH, Rgn_ISO3166_t[num].rgn_cd, SCC_CMN_DB_FILE_MAP);
				stat_ret = stat((const char*)dbFilePath, &st);
				if (0 == stat_ret) {
					strcpy(rgn_setting->dt_Folder[rgn_setting->folder_num].Region, Rgn_ISO3166_t[num].rgn_cd);
					sprintf(rgn_setting->dt_Folder[rgn_setting->folder_num].folder_Path, "%s%s%s",
							SCC_GetRootDirPath(), CC_CMN_MAP_DIR_PATH, Rgn_ISO3166_t[num].rgn_cd);
					rgn_setting->folder_num++;
				}
			}
			if ((isCreate) && (0 < rgn_setting->folder_num)) {
				strcpy(rgn_setting->now_Region, rgn_setting->dt_Folder[0].Region);
				// application.iniファイルが無い場合(初期状態)、作成する
				fp = fopen(filePath, "wb");
				if(NULL == fp){
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_Read_application_ini() fopen() error, " HERE);
					ret = e_SC_RESULT_FILE_OPENERR;
					break;
				}
				// application.iniファイル書込み
				result = fwrite(rgn_setting, sizeof(SMRGNSETTING), 1, fp);
				if(1 != result){
					SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_Read_application_ini() fwrite() error, " HERE);
					ret = e_SC_RESULT_FILE_ACCESSERR;
					break;
				}
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"CC_Read_application_ini() application.ini file create!, " HERE);
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"folder_num=%d, " HERE, rgn_setting->folder_num);
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"Region=%s, " HERE, rgn_setting->dt_Folder[0].Region);
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"Region=%s, " HERE, rgn_setting->dt_Folder[0].folder_Path);
				SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"now_Region=%s, " HERE, rgn_setting->now_Region);
				*restored = true;
			}
		}

	} while (0);

	// ファイルクローズ
	if(NULL != fp){
		fclose(fp);
	}

	// メモリ解放
	if(NULL != filePath){
		SCC_FREE(filePath);
	}
	if(NULL != dbFilePath){
		SCC_FREE(dbFilePath);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//application.iniファイルライト
//************************************************
/**
 * @brief application.iniファイルライト
 * @param[in] rgn_setting 仕向け設定情報構造体ポインタ
 * @retval OK : e_SC_RESULT_SUCCESS
 * @retval NG : e_SC_RESULT_SUCCESS以外
 * @note 引数のrgn_settingの内容をapplication.iniに書込む。
 */
E_SC_RESULT CC_Write_application_ini(const SMRGNSETTING *rgn_setting)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	Char	*filePath = NULL;
	FILE	*fp = NULL;
	INT32	result = 0;
	struct	stat st = {};
	int		stat_ret = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメタチェック
		if (CC_ISNULL(rgn_setting)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_Write_application_ini() param error[rgn_setting], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// PATH用メモリ確保
		filePath = (Char*)SCC_MALLOC(SCC_MAX_PATH);
		if (NULL == filePath) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_Write_application_ini() SCC_MALLOC() error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		//-------------------------------------------
		// Configフォルダ作成
		//-------------------------------------------
		// Configパス
		strcpy((char*)filePath, SCC_GetConfigDirPath());
		stat_ret = stat((const char*)filePath, &st);
		// Configフォルダが無い場合
		if(0 != stat_ret){
			CC_MakeDir(filePath);							// Configフォルダ作成
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"CC_Write_application_ini() Config folder create!, " HERE);
		}

		// application.iniファイルフルパス
		strcat((char*)filePath, SCC_MAPDWL_APPLICATION_INI_FILE_NAME);

		// ファイルオープン
		fp = fopen(filePath, "wb");
		if(NULL == fp){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_Write_application_ini() fopen() error, " HERE);
			ret = e_SC_RESULT_FILE_OPENERR;
			break;
		}

		// 仕向け設定情報書込み
		result = fwrite(rgn_setting, sizeof(SMRGNSETTING), 1, fp);
		if(1 != result){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_Write_application_ini() fwrite() error, " HERE);
			ret = e_SC_RESULT_FILE_ACCESSERR;
			break;
		}

	} while (0);

	// ファイルクローズ
	if(NULL != fp){
		fclose(fp);
	}

	// メモリ解放
	if(NULL != filePath){
		SCC_FREE(filePath);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//仕向け地コード→ISO-3166コード変換
//************************************************
/**
 * @brief 仕向け地コード→ISO-3166コード変換
 * @param[in] cRegionCode   仕向け地コード(アプリ定義文字列)
 * @retval OK : 対応するISO-3166コード
 * @retval NG : 0
 */
INT32 CC_Convert_Rgn_to_ISO3166(const Char *cRegionCode)
{
	INT32 ret = 0;
	INT32 cnt = 0;

	for(cnt = 0; CC_CMN_REGION_NUM > cnt; cnt++){
		if(0 == strcmp(cRegionCode, Rgn_ISO3166_t[cnt].rgn_cd)){
			ret = Rgn_ISO3166_t[cnt].iso3166_cd;
		}
	}

	if(0 == ret){
		SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"Undefine Region Code error [RegionCode:%s], " HERE, cRegionCode);
	}

	return (ret);
}

//************************************************
//ISO-3166コード→仕向け地コード変換
//************************************************
/**
 * @brief 仕向け地コード→ISO-3166コード変換
 * @param[in] iRegionCode   仕向け地コード(ISO-3166コード)
 * @retval OK : 対応する仕向け地コード文字列ポインタ
 * @retval NG : NULL
 */
Char* CC_Convert_ISO3166_to_Rgn(const INT32 iRegionCode)
{
	Char *ret = NULL;
	INT32 cnt = 0;

	for(cnt = 0; CC_CMN_REGION_NUM > cnt; cnt++){
		if(Rgn_ISO3166_t[cnt].iso3166_cd == iRegionCode){
			ret = Rgn_ISO3166_t[cnt].rgn_cd;
		}
	}

	if(NULL == ret){
		SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"Undefine Region Code error [RegionCode:%d], " HERE, iRegionCode);
	}

	return (ret);
}



//---------------------------------------------------------------------------------
//準内部関数 (SMCCom内で共通使用)
//---------------------------------------------------------------------------------
void CC_GetTempDirPath(Char *dirPath)
{
	if (NULL != dirPath) {
		strcpy(dirPath, tempDirPath);
	}
}

//************************************************
//ユーザ情報ファイル読出し
//************************************************
/**
 * @brief ユーザ情報ファイル読出し
 * @retval OK : e_SC_RESULT_SUCCESS
 * @retval NG : e_SC_RESULT_FILE_OPENERR
 * @note ユーザ情報ファイルから、各種API送信時に使用する情報を読出す
 */
E_SC_RESULT CC_ReadUserInfoFile()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	FILE* fp_user_inf = NULL;			// ファイルポインタ
	Char pUserInfo[SCC_MAX_PATH];
	INT32 result = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// ユーザ情報ファイルフルパス文字列作成
		sprintf(pUserInfo, "%s%s", tempDirPath, CC_CMN_USERINFO_FILE_NAME);

		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_ReadUserInfoFile() filePath = %s, " HERE, pUserInfo);

		// ファイルオープン
		fp_user_inf = fopen(pUserInfo, "rb");
		if(NULL == fp_user_inf){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ReadUserInfoFile() fopen() error, " HERE);
			ret = e_SC_RESULT_FILE_OPENERR;
			break;
		}

		// ユーザ情報ファイルから、SMS API関連パラメタ構造体読出し
		result = fread(&SmsApi_prm_t, 1, sizeof(T_CC_CMN_SMS_API_PRM), fp_user_inf);
		if(0 >= result){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ReadUserInfoFile() fread() error, " HERE);
			ret = e_SC_RESULT_FILE_ACCESSERR;
			break;
		}
	} while (0);

	// ファイルクローズ
	if(NULL != fp_user_inf){
		fclose(fp_user_inf);
	}
#if 0	//******************************************************************************************************************   デバッグ用処理
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Read reg_term_id = %s ", SmsApi_prm_t.ApiPrmNavi.reg_term_id);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Read common_uri = %s ", SmsApi_prm_t.ApiPrmNavi.common_uri);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Read sms_sp_uri = %s ", SmsApi_prm_t.ApiPrmNavi.sms_sp_uri);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Read alert_uri = %s ", SmsApi_prm_t.ApiPrmNavi.alert_uri);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Read analyze_uri = %s ", SmsApi_prm_t.ApiPrmNavi.analyze_uri);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Read init_item_id = %s ", SmsApi_prm_t.ApiPrmNavi.init_item_id);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Read policyver_dev = %s ", SmsApi_prm_t.ApiPrmNavi.policyver_dev);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Read new_term_id = %s ", SmsApi_prm_t.ApiPrmMups.new_term_id);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Save user_sig = %s ", SmsApi_prm_t.ApiPrmMups.user_sig);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Read guid = %s ", SmsApi_prm_t.ApiPrmMups.guid);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Read policyver_dl = %s ", SmsApi_prm_t.ApiPrmMups.policyver_dl);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Read user_name = %s ", SmsApi_prm_t.ApiPrmUser.user_name);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Read mail_addr = %s ", SmsApi_prm_t.ApiPrmUser.mail_addr);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Read login_id = %s ", SmsApi_prm_t.ApiPrmUser.login_id);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Read password = %s ", SmsApi_prm_t.ApiPrmUser.password);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Read lang = %s ", SmsApi_prm_t.ApiPrmUser.lang);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Read policyver = %s ", SmsApi_prm_t.ApiPrmUser.policyver);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Read policylang = %s ", SmsApi_prm_t.ApiPrmUser.policylang);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Read approval = %d ", SmsApi_prm_t.ApiPrmUser.approval);
#endif	//******************************************************************************************************************   デバッグ用処理

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//ユーザ情報ファイル退避
//************************************************
/**
 * @brief ユーザ情報ファイル退避
 * @retval OK : CC_CMN_RESULT_OK
 * @retval NG : e_SC_RESULT_FILE_OPENERR
 * @note 各種API送信時に使用する情報をユーザ情報ファイルに退避する
 * @note SMS API関連パラメタ構造体内の値が変化するたびに、本関数をコールすること
 */
E_SC_RESULT CC_SaveUserInfoFile()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	FILE* fp_user_inf = NULL;			// ファイルポインタ
	Char pUserInfo[SCC_MAX_PATH];
	INT32 result = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// ユーザ情報ファイルフルパス文字列作成
		sprintf(pUserInfo, "%s%s", tempDirPath, CC_CMN_USERINFO_FILE_NAME);

		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] CC_SaveUserInfoFile() filePath = %s, " HERE, pUserInfo);

		// ファイルオープン
		fp_user_inf = fopen(pUserInfo, "wb");
		if(NULL == fp_user_inf){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SaveUserInfoFile() fopen() error, " HERE);
			ret = e_SC_RESULT_FILE_OPENERR;
			break;
		}

		// ログインID、パスワード、メールアドレス、ユーザ名を出力しないようにする
		memcpy(&tmpSmsApi_prm_t, &SmsApi_prm_t, sizeof(T_CC_CMN_SMS_API_PRM));
		memset(tmpSmsApi_prm_t.ApiPrmUser.login_id, 0, sizeof(tmpSmsApi_prm_t.ApiPrmUser.login_id));
		memset(tmpSmsApi_prm_t.ApiPrmUser.password, 0, sizeof(tmpSmsApi_prm_t.ApiPrmUser.password));
		memset(tmpSmsApi_prm_t.ApiPrmUser.mail_addr, 0, sizeof(tmpSmsApi_prm_t.ApiPrmUser.mail_addr));
		memset(tmpSmsApi_prm_t.ApiPrmUser.user_name, 0, sizeof(tmpSmsApi_prm_t.ApiPrmUser.user_name));

		// URLを出力しないようにする
		memset(tmpSmsApi_prm_t.ApiPrmMups.common_uri, 0, sizeof(tmpSmsApi_prm_t.ApiPrmMups.common_uri));
		memset(tmpSmsApi_prm_t.ApiPrmNavi.sms_sp_uri, 0, sizeof(tmpSmsApi_prm_t.ApiPrmNavi.sms_sp_uri));

		// SMS API関連パラメタ構造体ごとファイルに書込み
		result = fwrite(&tmpSmsApi_prm_t, sizeof(T_CC_CMN_SMS_API_PRM), 1, fp_user_inf);
		if(1 != result){
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SaveUserInfoFile() fwrite() error, " HERE);
			ret = e_SC_RESULT_FILE_ACCESSERR;
			break;
		}
	} while (0);

	// ファイルクローズ
	if(NULL != fp_user_inf){
		fclose(fp_user_inf);
	}

#if 0	//******************************************************************************************************************   デバッグ用処理
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Save reg_term_id = %s ", SmsApi_prm_t.ApiPrmNavi.reg_term_id);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Save common_uri = %s ", SmsApi_prm_t.ApiPrmNavi.common_uri);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Save alert_uri = %s ", SmsApi_prm_t.ApiPrmNavi.alert_uri);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Save analyze_uri = %s ", SmsApi_prm_t.ApiPrmNavi.analyze_uri);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Save init_item_id = %s ", SmsApi_prm_t.ApiPrmNavi.init_item_id);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Save policyver_dev = %s ", SmsApi_prm_t.ApiPrmNavi.policyver_dev);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Save new_term_id = %s ", SmsApi_prm_t.ApiPrmMups.new_term_id);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Save user_sig = %s ", SmsApi_prm_t.ApiPrmMups.user_sig);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Save guid = %s ", SmsApi_prm_t.ApiPrmMups.guid);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Save policyver_dl = %s ", SmsApi_prm_t.ApiPrmMups.policyver_dl);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Save user_name = %s ", SmsApi_prm_t.ApiPrmUser.user_name);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Save mail_addr = %s ", SmsApi_prm_t.ApiPrmUser.mail_addr);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Save login_id = %s ", SmsApi_prm_t.ApiPrmUser.login_id);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Save password = %s ", SmsApi_prm_t.ApiPrmUser.password);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Save lang = %s ", SmsApi_prm_t.ApiPrmUser.lang);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Save policyver = %s ", SmsApi_prm_t.ApiPrmUser.policyver);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Save policylang = %s ", SmsApi_prm_t.ApiPrmUser.policylang);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"[CmnCtrl] User Info File Save approval = %d ", SmsApi_prm_t.ApiPrmUser.approval);
#endif	//******************************************************************************************************************   デバッグ用処理

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief ダウンロード進捗通知コールバック関数
 * @param[in] num   ダウンロード進捗
 */
void CC_ProgressCallback(const SMDLPROGRESS *progress)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	do {
		// Mutexロック
		ret = SCC_LockMutex(mutexDL);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_LockMutex error, " HERE);
			break;
		}

		if (!CC_ISNULL(progress)) {
			memcpy(&smDlProgress, progress, sizeof(SMDLPROGRESS));
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"msg=%s, " HERE, smDlProgress.msg);
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"doneSize=%u, totalSize=%u, completeCount=%u, totalCount=%u, " HERE,
							   smDlProgress.doneSize, smDlProgress.totalSize,
							   smDlProgress.completeCount, smDlProgress.totalCount);
		}

		// Mutexアンロック
		ret = SCC_UnLockMutex(mutexDL);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_UnLockMutex error, " HERE);
			break;
		}
	} while (0);
}

/**
 * @brief 地図分割ダウンロードパラメタ内地図DB格納パス変更関数
 * @param[in/out] mapUpdInfo   データ更新情報
 * @note 当該仕向けの地図DBダウンロードが初回と２回目以降で、HMIから渡されるパラメタ内の
 *       地図DB格納先のパスが若干変わるため、２回目以降の場合に初回時のパス(ルートパス)に変更する。
 *       また、プロパティファイルのインポート先パスも作成してパラメタ内に設定する。
 */
E_SC_RESULT CC_ChangeMapDBPath(SMUPDINFO *mapUpdInfo)
{
	SMRGNSETTING *rgn_setting = NULL;
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	UINT16 cnt = 0;
	Char	*c_ret = NULL;
	size_t	len = 0;
	Bool	restored = false;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		//*******    プロパティファイルインポート先パス作成
		// プロパティファイルインポート用パス
		strcpy((char*)mapUpdInfo->propertyPath, (char*)SCC_GetRootDirPath());
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"Property file import folder Path = %s, " HERE, mapUpdInfo->propertyPath);

		//*******    地図DB格納パス変更
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"MapDB Folder Path from HMI parm = %s, " HERE, mapUpdInfo->installDirPath);

		// メモリ確保
		rgn_setting = (SMRGNSETTING*)SCC_MALLOC(sizeof(SMRGNSETTING));
		if (NULL == rgn_setting) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// application.iniファイルリード
		ret = CC_Read_application_ini(rgn_setting, true, &restored);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_Read_application_ini() error, " HERE);
			ret = e_SC_RESULT_FILE_ACCESSERR;
			break;
		}

		// 地図DB未ダウンロードの場合（初期状態）
		if(0 == rgn_setting->folder_num){
			break;								// パスの変更なしで処理終了
		}
		// １回でも地図DBをダウンロードしてある場合
		else{
			for(cnt = 0; rgn_setting->folder_num > cnt; cnt++){
				// 当該仕向けの地図DBが既に有る場合
				if((0 == strcmp(rgn_setting->dt_Folder[cnt].Region, mapUpdInfo->regionCode)) &&
					(0 == strcmp(rgn_setting->dt_Folder[cnt].folder_Path, mapUpdInfo->installDirPath))){

					// ルートパスの次のフォルダ名の先頭をサーチ
					c_ret = strstr(mapUpdInfo->installDirPath, CC_CMN_MAP_DIR_NAME);

					// サーチ成功の場合
					if(NULL != c_ret){
						// パラメタ内のパスを初回時のパス(ルートパス)に変更する
						len = strlen(mapUpdInfo->installDirPath);
						len = len - (c_ret - mapUpdInfo->installDirPath);
						memset(c_ret, 0, len);

						SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"Change MapDB Folder Path = %s, " HERE, mapUpdInfo->installDirPath);

					}
					else{
						// ありえないはずなので、エラーログ出力
						SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ChangeMapDBPath() illegal error, " HERE);
						ret = e_SC_RESULT_MAP_UPDATE_ERR;
					}
					break;
				}
			}
			if (e_SC_RESULT_SUCCESS != ret) {
				break;
			}
		}
	} while(0);

	// メモリ解放
	if (NULL != rgn_setting) {
		SCC_FREE(rgn_setting);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

#if 0	// アラート機能無効
//************************************************
//アラート登録処理
//************************************************
/**
 * @brief アラート登録
 * @param[in]  alertList   アラート登録情報
 * @param[out] alert       アラート登録レスポンス情報
 * @param[out] userMsg     メッセージ
 * @param[out] apiStatus   APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_RegenerateAlert(SMALERTLIST *alertList, SMALERTREG *alert, Char *userMsg, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISNULL(alertList)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[alertList], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISNULL(alert)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[alert], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISNULL(userMsg)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[userMsg], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[pApiSts], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_TCP_CONNECT_ERROR;
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			memset(pAlert_RecvBuf, 0, CC_CMN_ALERT_RECIVE_BUFFER_SIZE);

			// 初期化
			*userMsg = EOS;
			*apiStatus = EOS;

			// アラート登録
			ret = CC_AlertReg_SendRecv(&smcal_alert, &SmsApi_prm_t, alertList, alert, (Char*)pAlert_RecvBuf, CC_CMN_ALERT_RECIVE_BUFFER_SIZE, userMsg, apiStatus);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_AlertReg_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(pAlert_RecvBuf, 0, CC_CMN_ALERT_RECIVE_BUFFER_SIZE);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(&smcal_alert, userSig, &SmsApi_prm_t, pAlert_RecvBuf, CC_CMN_ALERT_RECIVE_BUFFER_SIZE, apiSts2, false);
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
	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//アラート検索処理
//************************************************
/**
 * @brief アラート検索
 * @param[in]  alertCond   アラート検索条件
 * @param[out] alertList   アラート検索結果
 * @param[out] userMsg     メッセージ
 * @param[out] apiStatus   APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_SearchAlert(SMALERTCOND *alertCond, SMALERTLIST *alertList, Char *userMsg, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	alertInfoNum = 0;
	INT32	lastFlg = 0;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISNULL(alertCond)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[alertCond], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if ((alertCond->parcelNum < SCC_MIN_PARCEL_NUM) && (SCC_MAX_PARCEL_NUM < alertCond->parcelNum)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[alertCond->parcelNum], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISNULL(alertList)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[alertList], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISNULL(userMsg)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[userMsg], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[pApiSts], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_TCP_CONNECT_ERROR;
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);
		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			// 初期化
			*userMsg = EOS;
			*apiStatus = EOS;

			memset(pAlert_RecvBuf, 0, CC_CMN_ALERT_RECIVE_BUFFER_SIZE);

			// アラート検索
			ret = CC_AlertSearch_SendRecv(&smcal_alert, &SmsApi_prm_t, alertCond, alertList, &alertInfoNum, &lastFlg, (Char*)pAlert_RecvBuf, CC_CMN_ALERT_RECIVE_BUFFER_SIZE, userMsg, apiStatus);
			if (e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_AlertSearch_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(pAlert_RecvBuf, 0, CC_CMN_ALERT_RECIVE_BUFFER_SIZE);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(&smcal_alert, userSig, &SmsApi_prm_t, pAlert_RecvBuf, CC_CMN_ALERT_RECIVE_BUFFER_SIZE, apiSts2, false);
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
	} while (0);

	alertList->alertNum = alertInfoNum;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//アラート削除処理
//************************************************
/**
 * @brief アラート削除
 * @param[in]  alertList   アラート削除情報
 * @param[out] userMsg     メッセージ
 * @param[out] apiStatus   APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_DeleteAlert(SMALERTLIST *alertList, Char *userMsg, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userMsg2[CC_CMN_USER_MESSAGE_AREA_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISNULL(alertList)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[alertList], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISNULL(userMsg)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[userMsg], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[pApiSts], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_TCP_CONNECT_ERROR;
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			memset(pAlert_RecvBuf, 0, CC_CMN_ALERT_RECIVE_BUFFER_SIZE);

			// 初期化
			*userMsg = EOS;
			*apiStatus = EOS;

			// アラート削除
			ret = CC_AlertDel_SendRecv(&smcal_alert, &SmsApi_prm_t, alertList, (Char*)pAlert_RecvBuf, CC_CMN_ALERT_RECIVE_BUFFER_SIZE, userMsg, apiStatus);
			if(e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_AlertReg_SendRecv error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"apiStatus=%s, " HERE, apiStatus);
				if (0 == strcmp(CC_CMN_API_AUTH_ERR, apiStatus)) {
					// tokenエラー
					apiSts2[0] = EOS;
					memset(pAlert_RecvBuf, 0, CC_CMN_ALERT_RECIVE_BUFFER_SIZE);

					// 再認証(必要な場合のみ再認証する)
					ret2 = CC_ReAuthreq(&smcal_alert, userSig, &SmsApi_prm_t, pAlert_RecvBuf, CC_CMN_ALERT_RECIVE_BUFFER_SIZE, apiSts2, false);
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
	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

//************************************************
//画像解析要求処理
//************************************************
/**
 * @brief 画像解析要求
 * @param[in/OUT] analyze  解析要求情報
 * @param[out] userMsg     メッセージ
 * @param[out] apiStatus   APIステータスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_AnalyzePicture(SMANALYZEINFO *analyze, Char *userMsg, Char *apiStatus)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	Char	apiSts2[CC_CMN_XML_RES_STS_CODE_SIZE] ={};
	Char	userSig[CC_CMN_USER_SIG_STR_SIZE] = {};

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_ISNULL(analyze)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[analyze], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISNULL(userMsg)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[userMsg], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}
		if (CC_ISNULL(apiStatus)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[pApiSts], " HERE);
			ret = CC_CMN_RESULT_PARAM_ERR;
			break;
		}

		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_TCP_CONNECT_ERROR;
			break;
		}

		// ユーザーアクセスキー退避
		strcpy(userSig, SmsApi_prm_t.ApiPrmMups.user_sig);

		for (num = 0; num <= CC_CMN_SEND_RETRY_CNT; num++) {
			memset(pAnalyze_RecvBuf, 0, CC_CMN_ANALYZE_RECIVE_BUFFER_SIZE);

			// 初期化
			*userMsg = EOS;
			*apiStatus = EOS;

			// 画像解析要求
			ret = CC_PictAnalyze_SendRecv(&smcal_analyze, &SmsApi_prm_t, analyze, pAnalyze_RecvBuf, CC_CMN_ANALYZE_RECIVE_BUFFER_SIZE, userMsg, apiStatus);
			if(e_SC_RESULT_SUCCESS != ret){
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_AnalyzePicture_SendRecv error, " HERE);
				break;
			}
			break;
		}
		if (e_SC_RESULT_SUCCESS != ret) {
			break;
		}
	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}
#endif	// アラート機能無効

/**
 * @brief ファイルを移動する
 * @param[in] tempDirPath
 * @param[in] oldDataDir
 * @retval 正常終了  :e_SC_RESULT_SUCCESS
 * @retval エラー終了:e_SC_RESULT_SUCCESS以外 (コール関数の戻り値)
 */
E_SC_RESULT CC_MoveFileForDL(const Char *tempDirPath, const Char *oldDataDir)
{
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT ret2 = e_SC_RESULT_SUCCESS;
	Char	*srcFilePath = NULL;
	Char	*dstFilePath = NULL;
	Char	*srcStr = NULL;
	Char	*dstStr = NULL;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// メモリ確保
		srcFilePath = (Char*)SCC_MALLOC(SCC_MAX_PATH);
		if (NULL == srcFilePath) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		dstFilePath = (Char*)SCC_MALLOC(SCC_MAX_PATH);
		if (NULL == dstFilePath) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_MALLOC error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}

		// SMCC_UserInfo.txt
		// コピー元ファイルパス生成
		if ('/' == oldDataDir[strlen((const char*)oldDataDir) - 1]) {
			sprintf(srcFilePath, "%s%s%s", oldDataDir, CC_CMN_TEMP_DIR_PATH, CC_CMN_USERINFO_FILE_NAME);
		} else {
			sprintf(srcFilePath, "%s/%s%s", oldDataDir, CC_CMN_TEMP_DIR_PATH, CC_CMN_USERINFO_FILE_NAME);
		}
		// コピー先ファイルパス生成
		sprintf(dstFilePath, "%s%s", tempDirPath, CC_CMN_USERINFO_FILE_NAME);

		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"srcFilePath=%s, " HERE, srcFilePath);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"dstFilePath=%s, " HERE, dstFilePath);
		// ファイルコピー
		ret2 = SCC_CopyFile(srcFilePath, dstFilePath, true);
		if (e_SC_RESULT_SUCCESS != ret2) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_CopyFile error, " HERE);
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"srcFilePath=%s, " HERE, srcFilePath);
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"dstFilePath=%s, " HERE, dstFilePath);
			// エラーでも処理は継続する
			if (e_SC_RESULT_SUCCESS == ret) {
				ret = ret2;
			}
		}

		// dataVersionInfo.db
		strcpy(srcFilePath, oldDataDir);
		srcStr = strstr(srcFilePath, CC_CMN_DATA_DIR_NAME);
		strcpy(dstFilePath, tempDirPath);
		dstStr = strstr(dstFilePath, CC_CMN_DATA_DIR_NAME);
		if ((NULL != srcStr) && (NULL != dstStr)) {
			*srcStr = EOS;
			*dstStr = EOS;

			// ディレクトリ作成
			sprintf(&dstFilePath[strlen((char*)dstFilePath)], "%s", CC_CMN_TEMP_DIR_PATH);
			CC_MakeDir(dstFilePath);

			// コピー元ファイルパス生成
			sprintf(&srcFilePath[strlen((char*)srcFilePath)], "%s%s", CC_CMN_TEMP_DIR_PATH, SCC_CMN_DB_FILE_DATAVERSIONINFO);
			// コピー先ファイルパス生成
			sprintf(&dstFilePath[strlen((char*)dstFilePath)], "%s", SCC_CMN_DB_FILE_DATAVERSIONINFO);

			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"srcFilePath=%s, " HERE, srcFilePath);
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"dstFilePath=%s, " HERE, dstFilePath);

			// ファイルコピー
			ret2 = SCC_CopyFile(srcFilePath, dstFilePath, true);
			if (e_SC_RESULT_SUCCESS != ret2) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_CopyFile error, " HERE);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"srcFilePath=%s, " HERE, srcFilePath);
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"dstFilePath=%s, " HERE, dstFilePath);
				// エラーでも処理は継続する
				if (e_SC_RESULT_SUCCESS == ret) {
					ret = ret2;
				}
			}
		}

	} while (0);

	// メモリ解放
	if (NULL != srcFilePath) {
		SCC_FREE(srcFilePath);
	}
	if (NULL != dstFilePath) {
		SCC_FREE(dstFilePath);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief 再認証する
 * @param[in]  userSig
 * @param[out] apiSts
 * @retval 正常終了  :e_SC_RESULT_SUCCESS
 * @retval エラー終了:e_SC_RESULT_SUCCESS以外 (コール関数の戻り値)
 */
E_SC_RESULT CC_ReAuthreq(SMCAL *cal,
						 const Char *userSig,
						 T_CC_CMN_SMS_API_PRM *parm,
						 Char *recvBuf,
						 UINT32 recvBufSize,
						 Char *apiSts,
						 Bool isPolling)
{
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT ret2 = e_SC_RESULT_SUCCESS;
	Bool	mutexLock = false;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// 未ログインエラー
		if (!CC_ISLOGINED()) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"not login error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// Mutexロック
		ret = SCC_LockMutex(mutexAuth);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_LockMutex error, " HERE);
			break;
		}
		mutexLock = true;

		if (0 == strcmp(userSig, parm->ApiPrmMups.user_sig)) {
			// 初期化
			memset(recvBuf, 0, recvBufSize);
			// 再認証
			ret = CC_ReAuthenticationUser(cal,
										  parm->ApiPrmUser.login_id,
										  parm->ApiPrmUser.password,
										  recvBuf,
										  recvBufSize,
										  apiSts,
										  isPolling);
			if (e_SC_RESULT_SUCCESS != ret) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_ReAuthenticationUser error, " HERE);
				break;
			}
		}
	} while (0);

	if (mutexLock) {
		// Mutexアンロック
		ret2 = SCC_UnLockMutex(mutexAuth);
		if (e_SC_RESULT_SUCCESS != ret2) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"SCC_UnLockMutex error, " HERE);
			if (e_SC_RESULT_SUCCESS == ret) {
				ret = ret2;
			}
		}
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}


/**
 * @brief 通信に必要なバッファ取得
 * @param[in] cal       SMCALのポインタ
 * @param[in] buff      バッファのポインタ
 * @param[in] buffSize  バッファサイズのポインタ
 * @retval 正常終了  :e_SC_RESULT_SUCCESS
 * @retval エラー終了:e_SC_RESULT_SUCCESS以外
 */
E_SC_RESULT CC_GetComBuff(SMCAL **cal, Char **buff, INT32 *buffSize)
{
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;

	do {
		if (CC_ISNULL(cal)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[cal], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(buff)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[buff], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (CC_ISNULL(buffSize)) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"param error[buffSize], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 初期化
		*cal = NULL;
		*buff = NULL;
		*buffSize = 0;

		// 排他制御開始
		ret = SCC_LockMutex(mutexAPIMng);
		if (ret != e_SC_RESULT_SUCCESS) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "SCC_LockMutext error, " HERE);
			break;
		}

		ret = e_SC_RESULT_FAIL;
		for (num = 0; num < CC_CMN_API_CONCURRENCY_MAXNUM; num++) {
			if (!apiConcurrencyList[num]) {
				apiConcurrencyList[num] = true;
				*cal = &smcal[num];
				*buff = pRecvBuf[num];
				*buffSize = CC_CMN_RECIVE_BUFFER_SIZE;
				ret = e_SC_RESULT_SUCCESS;
				break;
			}
		}

		// 排他制御終了
		ret2 = SCC_UnLockMutex(mutexAPIMng);
		if (ret2 != e_SC_RESULT_SUCCESS) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "SCC_UnLockMutex error, " HERE);
			if (ret == e_SC_RESULT_SUCCESS) {
				ret = ret2;
			}
			break;
		}
	} while (0);

	return (ret);
}

/**
 * @brief 通信に必要なバッファ解放
 * @param[in] cal   SMCALのポインタ
 */
void CC_FreeComBuff(const SMCAL *cal)
{
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	INT32	num = 0;

	do {
		if (NULL == cal) {
			break;
		}

		// 排他制御開始
		ret = SCC_LockMutex(mutexAPIMng);
		if (ret != e_SC_RESULT_SUCCESS) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "SCC_LockMutext error, " HERE);
			break;
		}

		for (num = 0; num < CC_CMN_API_CONCURRENCY_MAXNUM; num++) {
			if (cal == &smcal[num]) {
				apiConcurrencyList[num] = false;
				break;
			}
		}

		// 排他制御終了
		ret = SCC_UnLockMutex(mutexAPIMng);
		if (ret != e_SC_RESULT_SUCCESS) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "SCC_UnLockMutex error, " HERE);
			break;
		}
	} while (0);
}

/**
 * @brief パッケージ情報を取得する
 * @param[in] pkgFileName   パッケージファイル名
 * @param[in] pkgInfo       パッケージ情報
 * @retval 成功：true
 * @retval 失敗：false
 */
Bool CC_GetPackageInfo(const Char *pkgFileName, SMPACKAGEINFO *pkgInfo)
{
	Bool	ret = false;
	INT32	num = 0;

	for (num = 0; num < (CC_CMN_PKGMGR_GROUP_MAXNUM * CC_CMN_PKGMGR_PACKAGE_MAXNUM); num++) {
		if (EOS == pkgList[num].packageFileName[0]) {
			continue;
		}
		if (0 == strcmp(pkgList[num].packageFileName, pkgFileName)) {
			strcpy((char*)pkgInfo->backetName, (char*)pkgList[num].backetName);
			strcpy((char*)pkgInfo->accessKey, (char*)pkgList[num].accessKey);
			strcpy((char*)pkgInfo->secretKey, (char*)pkgList[num].secretKey);
			ret = true;
			break;
		}
	}

	return (ret);
}

/**
 * @brief パッケージ情報を取得する
 * @param[in] pkgFileName   パッケージ名(フォルダ名)
 * @param[in] pkgInfo       パッケージ情報
 * @retval 成功：true
 * @retval 失敗：false
 */
Bool CC_PkgCmn_GetPackageInfo(const Char *pkgName, const Char *pkgVer, SMPACKAGEINFO *pkgInfo)
{
	Bool	ret = false;
	INT32	num = 0;
	Char	pkgDirName[CC_CMN_PKGMGR_PCKAGE] = {};

	for (num = 0; num < (CC_CMN_PKGMGR_GROUP_MAXNUM * CC_CMN_PKGMGR_PACKAGE_MAXNUM); num++) {
		if (EOS == pkgList[num].packageFileName[0]) {
			continue;
		}
		strcpy(pkgDirName, pkgList[num].packageFileName);
		if (4 <= strlen((char*)pkgDirName)) {
			if (0 == strcmp((char*)SCC_ToLowerString(&pkgDirName[strlen((char*)pkgDirName) - 4]), ".zip")) {
				pkgDirName[strlen((char*)pkgDirName) - 4] = EOS;
			}
		}
		if (0 == strcmp((char*)pkgDirName, (char*)pkgName)) {
			strcpy((char*)pkgInfo->version, (char*)pkgVer);
			strcpy((char*)pkgInfo->backetName, (char*)pkgList[num].backetName);
			strcpy((char*)pkgInfo->accessKey, (char*)pkgList[num].accessKey);
			strcpy((char*)pkgInfo->secretKey, (char*)pkgList[num].secretKey);
			ret = true;
			break;
		}
	}

	return (ret);
}
