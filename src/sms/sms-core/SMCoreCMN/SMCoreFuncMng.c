/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

/*
 * SMCoreFuncMng.c
 *
 *  Created on: 2015/11/05
 *      Author:
 */

#include "SMCoreCMNInternal.h"

#define	SC_THREAD_NUM			17						// スレッド数 TODO (仮)
#define	SC_DEFAULT_ROOT_DIRPATH	"/sdcard/jp.co.hitachi.smsfv.aa/Data"
#define	SC_DEFAULT_MAP_DIRPATH	"/sdcard/jp.co.hitachi.smsfv.aa/Map"

//-----------------------------------
// 変数定義
//-----------------------------------
// API同期
#ifdef __SMS_APPLE__
SC_SEMAPHORE	*syncAPISem;
SC_SEMAPHORE	*syncSDSem;
Char* syncAPI_SemName = "/syncAPI";
Char* syncSD_SemName = "/syncSD";
#else
SC_SEMAPHORE	syncAPISem;
SC_SEMAPHORE	syncSDSem;
#endif /* __SMS_APPLE__ */
static E_SC_RESULT apiResult;
static E_SC_RESULT sdResult;
static Char mapDirPath[SC_MAX_PATH];
static Char configRootPath[SC_MAX_PATH];
static Char apRootDirPath[SC_MAX_PATH];
static Char sdTripId[SC_SD_TRIP_ID_SIZE];

//-----------------------------------
// 関数定義
//-----------------------------------
static E_SC_RESULT SC_MNG_MemInitialize();
static E_SC_RESULT SC_MNG_MemFinalize();
E_SC_RESULT SC_MNG_CreateMsgQueue();
E_SC_RESULT SC_MNG_DestroyMsgQueue();
static E_SC_RESULT SC_MNG_SubFuncInitialize();
static E_SC_RESULT SC_MNG_SubFuncFinalize();
static E_SC_RESULT SC_MNG_ThreadInitialize();
static E_SC_RESULT SC_MNG_ThreadFinalize();
static void SC_MNG_InitPath(const Char *rootDir, const Char *confDir, const Char *mapDir);

//-----------------------------------
// 構造体定義
//-----------------------------------
// スレッド情報
typedef struct _SC_THREAD_INFO {
	SC_THREAD_ID		threadId;				// スレッドID
	SC_THREAD_MAIN_FUNC	Main;					// スレッドメイン関数ポインタ
} SC_THREAD_INFO;
// スレッド情報テーブル
SC_THREAD_INFO	threadInfTbl[SC_THREAD_NUM] = {
	{0,		(SC_THREAD_MAIN_FUNC)SC_FM_ThreadMain},	// Func.Mng.
	{0,		(SC_THREAD_MAIN_FUNC)SC_MP_ThreadMain},	// 地図描画
	{0,		(SC_THREAD_MAIN_FUNC)SC_RM_ThreadMain},	// 経路探索(RM)
	{0,		(SC_THREAD_MAIN_FUNC)SC_RC_ThreadMain},	// 経路探索(RC)
	{0,		(SC_THREAD_MAIN_FUNC)SC_RG_ThreadMain},	// 経路誘導(RG)
	{0,		(SC_THREAD_MAIN_FUNC)SC_RT_ThreadMain},	// 経路誘導(RT)
	{0,		(SC_THREAD_MAIN_FUNC)SC_DH_ThreadMain},	// データハンドラ
	{0,		(SC_THREAD_MAIN_FUNC)NULL},				// データハンドラキャンセル
	{0,		(SC_THREAD_MAIN_FUNC)SC_PM_ThreadMain},	// プローブアップロード(メイン)
	{0,		(SC_THREAD_MAIN_FUNC)SC_PU_ThreadMain},	// プローブアップロード(アップロード)
	{0,		(SC_THREAD_MAIN_FUNC)SC_PT_ThreadMain},	// プローブアップロード(タイマ)
	{0,		(SC_THREAD_MAIN_FUNC)SC_SDD_ThreadMain},// 運転特性診断センサデータ(データ取得)
	{0,		(SC_THREAD_MAIN_FUNC)SC_SDM_ThreadMain},// 運転特性診断センサデータ(メイン)
	{0,		(SC_THREAD_MAIN_FUNC)SC_SDU_ThreadMain},// 運転特性診断センサデータ(アップロード)
	{0,		(SC_THREAD_MAIN_FUNC)SC_SDT_ThreadMain},// 運転特性診断センサデータ(タイマ)
	{0,		(SC_THREAD_MAIN_FUNC)SC_TR_ThreadMain},	// 交通情報(TR)
	{0,		(SC_THREAD_MAIN_FUNC)SC_TRT_ThreadMain}	// 交通情報(タイマ)
};

// 初期化／終了化関数ポインタ
typedef struct _SC_INIT_FILAL_FUNC {
	SC_INITIALIZE_FUNC	Initialize;				// 初期化関数ポインタ
	SC_FINALIZE_FUNC	Finalize;				// 終了化関数ポインタ
} SC_INIT_FILAL_FUNC;
// 初期化／終了化関数ポインタテーブル
SC_INIT_FILAL_FUNC	initFinalFuncTbl[SC_THREAD_NUM] = {
	{(SC_INITIALIZE_FUNC)SC_FM_Initialize,		(SC_FINALIZE_FUNC)SC_FM_Finalize},	// Func.Mng.
	{(SC_INITIALIZE_FUNC)SC_MP_Initialize,		(SC_FINALIZE_FUNC)SC_MP_Finalize},	// 地図描画
	{(SC_INITIALIZE_FUNC)SC_RM_Initialize,		(SC_FINALIZE_FUNC)SC_RM_Finalize},	// 経路探索(RM)
	{(SC_INITIALIZE_FUNC)SC_RC_Initialize,		(SC_FINALIZE_FUNC)SC_RC_Finalize},	// 経路探索(RC)
	{(SC_INITIALIZE_FUNC)SC_RG_Initialize,		(SC_FINALIZE_FUNC)SC_RG_Finalize},	// 経路探索(RG)
	{(SC_INITIALIZE_FUNC)SC_RT_Initialize,		(SC_FINALIZE_FUNC)SC_RT_Finalize},	// 経路探索(RG)
	{(SC_INITIALIZE_FUNC)SC_DH_Initialize,		(SC_FINALIZE_FUNC)SC_DH_Finalize},	// データハンドラ
	{(SC_INITIALIZE_FUNC)NULL,					(SC_FINALIZE_FUNC)NULL},			// データハンドラキャンセル
	{(SC_INITIALIZE_FUNC)SC_PM_Initialize,		(SC_FINALIZE_FUNC)SC_PM_Finalize},	// プローブアップロード(メイン)
	{(SC_INITIALIZE_FUNC)SC_PU_Initialize,		(SC_FINALIZE_FUNC)SC_PU_Finalize},	// プローブアップロード(アップロード)
	{(SC_INITIALIZE_FUNC)SC_PT_Initialize,		(SC_FINALIZE_FUNC)SC_PT_Finalize},	// プローブアップロード(タイマ)
	{(SC_INITIALIZE_FUNC)SC_SDD_Initialize,		(SC_FINALIZE_FUNC)SC_SDD_Finalize},	// 運転特性診断センサデータ(データ取得)
	{(SC_INITIALIZE_FUNC)SC_SDM_Initialize,		(SC_FINALIZE_FUNC)SC_SDM_Finalize},	// 運転特性診断センサデータ(メイン)
	{(SC_INITIALIZE_FUNC)SC_SDU_Initialize,		(SC_FINALIZE_FUNC)SC_SDU_Finalize},	// 運転特性診断センサデータ(アップロード)
	{(SC_INITIALIZE_FUNC)SC_SDT_Initialize,		(SC_FINALIZE_FUNC)SC_SDT_Finalize},	// 運転特性診断センサデータ(タイマ)
	{(SC_INITIALIZE_FUNC)SC_TR_Initialize,		(SC_FINALIZE_FUNC)SC_TR_Finalize},	// 運交通情報(TR)
	{(SC_INITIALIZE_FUNC)SC_TRT_Initialize,		(SC_FINALIZE_FUNC)SC_TRT_Finalize}	// 運交通情報(タイマ)
};

/**
 * @brief コアの初期化処理を行う
 * @param[in] rootDir   ルートディレクトリのフルパス
 * @param[in] confDir   ナビコアの設定ファイル格納ディレクトリのフルパス
 * @param[in] mapDir    地図ディレクトリのフルパス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_Initialize(const Char *rootDir, const Char *confDir, const Char *mapDir)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT restorRet = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == rootDir) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[rootDir], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == confDir) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[confDir], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == mapDir) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[mapDir], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// パス初期化
		SC_MNG_InitPath(rootDir, confDir, mapDir);

		// メモリ管理初期化
		ret = SC_MNG_MemInitialize();
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MNG_MemInitialize error, " HERE);
			break;
		}

		// 常駐メモリとキャッシュメモリに領域を割り当てる
		// DALを初期化する
		ret = SC_DH_MemInitialize(mapDirPath, confDir);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_Initialize error, " HERE);
			break;
		}

		// 共有メモリのリストア
		restorRet = SC_DH_LoadShareData();
		// 設定ファイル読み込み(設定ファイル固定値のもの)
		ret = SC_MNG_LoadSecureConfig(confDir);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MNG_LoadSecureConfig error, " HERE);
			break;
		}
		if(e_SC_RESULT_SUCCESS != restorRet){
			// リストア失敗->設定ファイルを読み込む
			SC_LOG_DebugPrint(SC_TAG_CORE, "SC_DH_LoadShareData fail(%x), ", restorRet);
			ret = SC_MNG_LoadConfig(confDir);
			if (e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MNG_LoadConfig error, " HERE);
				break;
			}
		}

		// セマフォ生成
#ifdef __SMS_APPLE__
		ret = SC_CreateSemaphore(&syncAPISem, syncAPI_SemName, 0);
#else
		ret = SC_CreateSemaphore(&syncAPISem, 0);
#endif /* __SMS_APPLE__ */
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_CreateSemaphore error, " HERE);
			break;
		}
#ifdef __SMS_APPLE__
 		ret = SC_CreateSemaphore(&syncSDSem, syncSD_SemName, 0);
#else
		ret = SC_CreateSemaphore(&syncSDSem, 0);
#endif /* __SMS_APPLE__ */
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_CreateSemaphore error, " HERE);
			break;
		}

		// 各スレッド初期化
		ret = SC_MNG_SubFuncInitialize();
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MNG_SubFuncInitialize error, " HERE);
			break;
		}

		// メッセージキュー生成
		ret = SC_MNG_CreateMsgQueue();
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MNG_CreateMsgQueue error, " HERE);
			break;
		}

		// スレッド生成
		ret = SC_MNG_ThreadInitialize();
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MNG_CreateThread error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief コアの終了化処理を行う
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_Finalize()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;

	//SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);
	LOG_PRINT_START((char*)SC_TAG_CORE);

	// エラーでも処理は継続する

	// 変数初期化
	mapDirPath[0] = EOS;
	configRootPath[0] = EOS;
	apRootDirPath[0] = EOS;

	// 終了化
	ret = SC_DH_ConfigFinalize();
	if (e_SC_RESULT_SUCCESS != ret) {
		//SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_ConfigFinalize error, " HERE);
		LOG_PRINT_ERROR((char*)SC_TAG_CORE, "SC_DH_ConfigFinalize error, " HERE);
	}

	// スレッド終了
	ret2 = SC_MNG_ThreadFinalize();
	if (e_SC_RESULT_SUCCESS != ret2) {
		//SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DestroySemaphore error, " HERE);
		LOG_PRINT_ERROR((char*)SC_TAG_CORE, "SC_DestroySemaphore error, " HERE);
		if (e_SC_RESULT_SUCCESS == ret) {
			ret = ret2;
		}
	}

	// セマフォ破棄
#ifdef __SMS_APPLE__
	SC_DestroySemaphore(syncAPISem, syncAPI_SemName);
	SC_DestroySemaphore(syncSDSem, syncSD_SemName);
#else
	SC_DestroySemaphore(&syncAPISem);
	SC_DestroySemaphore(&syncSDSem);
#endif /* __SMS_APPLE__ */

	// 各機能終了化
	ret2 = SC_MNG_SubFuncFinalize();
	if (e_SC_RESULT_SUCCESS != ret2) {
		//SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MNG_SubFuncFinalize error, " HERE);
		LOG_PRINT_ERROR((char*)SC_TAG_CORE, "SC_MNG_SubFuncFinalize error, " HERE);
		if (e_SC_RESULT_SUCCESS == ret) {
			ret = ret2;
		}
	}

	// メッセージキュー破壊
	ret2 = SC_MNG_DestroyMsgQueue();
	if (e_SC_RESULT_SUCCESS != ret2) {
		//SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MNG_DestroyMsgQueue error, " HERE);
		LOG_PRINT_ERROR((char*)SC_TAG_CORE, "SC_MNG_DestroyMsgQueue error, " HERE);
		if (e_SC_RESULT_SUCCESS == ret) {
			ret = ret2;
		}
	}

	// 常駐メモリとキャッシュメモリ領域を解放する
	// DALを終了化する
	ret2 = SC_DH_MemFinalize();
	if (e_SC_RESULT_SUCCESS != ret2) {
		//SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_Finalize error, " HERE);
		LOG_PRINT_ERROR((char*)SC_TAG_CORE, "SC_DH_Finalize error, " HERE);
		if (e_SC_RESULT_SUCCESS == ret) {
			ret = ret2;
		}
	}

	// メモリ管理終了化
	ret2 = SC_MNG_MemFinalize();
	if (e_SC_RESULT_SUCCESS != ret2) {
		//SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MNG_MemFinalize error, " HERE);
		LOG_PRINT_ERROR((char*)SC_TAG_CORE, "SC_MNG_MemFinalize error, " HERE);
		if (e_SC_RESULT_SUCCESS == ret) {
			ret = ret2;
		}
	}

	// ログ終了化
	ret2 = SC_LOG_Finalize();
	if (e_SC_RESULT_SUCCESS != ret2) {
		//SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_LOG_Finalize error, " HERE);
		LOG_PRINT_ERROR((char*)SC_TAG_CORE, "SC_LOG_Finalize error, " HERE);
		if (e_SC_RESULT_SUCCESS == ret) {
			ret = ret2;
		}
	}

	//SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);
	LOG_PRINT_END((char*)SC_TAG_CORE);

	return (ret);
}

/**
 * @brief 表示縮尺を取得する
 * @param[in]  maps       表示操作対象地図
 * @param[out] scaleLevel 地図表示縮尺(スケール)
 * @param[out] zoomLevel ズームレベル
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetScaleLevel(INT32 maps, INT32 *scaleLevel, FLOAT *scaleRange, INT32 *zoomLevel)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_SCALE	scale = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == scaleLevel) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[scaleLevel], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_SCALELEVEL;
		scale.maps = maps;
		data.data = (void*)&scale;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_GetShareData error, " HERE);
			break;
		}
		*scaleLevel = scale.scaleLevel;
		*scaleRange = scale.scaleRange;
		*zoomLevel = scale.zoomLevel;
	} while(0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 表示縮尺を設定する
 * @param[in] maps       表示操作対象地図
 * @param[in] scaleLevel 地図表示縮尺(スケール)
 * @param[in] zoomLevel ズームレベル
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetScaleLevel(INT32 maps, INT32 scaleLevel, FLOAT scaleRange, INT32 zoomLevel)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_SCALE	scale = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_SCALELEVEL;
		scale.maps = maps;
		scale.scaleLevel = scaleLevel;
		scale.scaleRange = scaleRange;
		scale.zoomLevel = zoomLevel;
		data.data = (void*)&scale;
		SC_LOG_InfoPrint(SC_TAG_CORE, "SC_MNG_SetScaleLevel scale=%d scaleRange=%f zoom=%d, " HERE, scaleLevel, scaleRange,
				zoomLevel);

		// 常駐メモリデータを設定する
		ret = SC_DH_SetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 地図上文言拡大表示を取得する
 * @param[in]  maps      表示操作対象地図
 * @param[out] isBigText 地図上文言拡大表示・非表示(true:表示する、false:表示しない)
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetBigTextAttr(INT32 maps, Bool *isBigText)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_BIGTEXTATTR	text = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == isBigText) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[isBigText], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_BIGTEXTATTR;
		text.maps = maps;
		data.data = (void*)&text;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_GetShareData error, " HERE);
			break;
		}
		*isBigText = text.bigText;
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 地図上文言拡大表示を設定する
 * @param[in] maps      表示操作対象地図
 * @param[in] isBigText 地図上文言拡大表示・非表示(true:表示する、false:表示しない)
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetBigTextAttr(INT32 maps, Bool isBigText)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_BIGTEXTATTR	text = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	// 常駐メモリデータ入出力構造体に値を設定する
	data.dataId = e_SC_DH_SHARE_BIGTEXTATTR;
	text.maps = maps;
	text.bigText = isBigText;
	data.data = (void*)&text;

	// 常駐メモリデータを設定する
	ret = SC_DH_SetShareData(&data);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
	}

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 地図上表示アイコン拡大表示を取得する
 * @param[in]  maps      表示操作対象地図
 * @param[out] isBigIcon 地図上表示アイコン拡大表示・非表示(true:表示する、false:表示しない)
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetBigIconAttr(INT32 maps, Bool *isBigIcon)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_BIGICONATTR	icon = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == isBigIcon) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[isBigIcon], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_BIGICONATTR;
		icon.maps = maps;
		data.data = (void*)&icon;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_GetShareData error, " HERE);
			break;
		}
		*isBigIcon = icon.bigIcon;
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 地図上表示アイコン拡大表示を設定する
 * @param[in] maps      表示操作対象地図
 * @param[in] isBigIcon 地図上表示アイコン拡大表示・非表示(true:表示する、false:表示しない)
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetBigIconAttr(INT32 maps, Bool isBigIcon)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_BIGICONATTR	icon = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	// 常駐メモリデータ入出力構造体に値を設定する
	data.dataId = e_SC_DH_SHARE_BIGICONATTR;
	icon.maps = maps;
	icon.bigIcon = isBigIcon;
	data.data = (void*)&icon;

	// 常駐メモリデータを設定する
	ret = SC_DH_SetShareData(&data);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
	}

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief ランドマーク表示設定を取得する
 * @param[in]  maps   表示操作対象地図
 * @param[out] isShow ランドマーク表示・非表示(true:表示する、false:表示しない)
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetLandmark(INT32 maps, Bool *isShow)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_LANDMARK	landmark = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == isShow) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[isShow], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_SHOWLANDMARK;
		landmark.maps = maps;
		data.data = (void*)&landmark;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_GetShareData error, " HERE);
			break;
		}
		*isShow = landmark.isShow;
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief ランドマーク表示設定を設定する
 * @param[in] maps   表示操作対象地図
 * @param[in] isShow ランドマーク表示・非表示(true:表示する、false:表示しない)
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetLandmark(INT32 maps, Bool isShow)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_LANDMARK	landmark = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	// 常駐メモリデータ入出力構造体に値を設定する
	data.dataId = e_SC_DH_SHARE_SHOWLANDMARK;
	landmark.maps = maps;
	landmark.isShow = isShow;
	data.data = (void*)&landmark;

	// 常駐メモリデータを設定する
	ret = SC_DH_SetShareData(&data);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
	}

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief ランドマーク表示属性設定を取得する
 * @param[in]  maps      表示操作対象地図
 * @param[in]  classCode ランドマーク種別コード
 * @param[out] isShow    ランドマーク表示属性表示・非表示(true:表示する、false:表示しない)
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetLandmarkAttr(INT32 maps, INT32 classCode, Bool *isShow)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_LANDMARKATTR	landmark = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == isShow) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[isShow], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_SHOWLANDMARKATTR;
		landmark.maps = maps;
		landmark.classCode = classCode;
		data.data = (void*)&landmark;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_GetShareData error, " HERE);
			break;
		}
		*isShow = landmark.isShow;
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief ランドマーク表示属性設定を設定する
 * @param[in] maps      表示操作対象地図
 * @param[in] classCode ランドマーク種別コード
 * @param[in] isShow    ランドマーク表示属性表示・非表示(true:表示する、false:表示しない)
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetLandmarkAttr(INT32 maps, INT32 classCode, Bool isShow)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_LANDMARKATTR	landmark = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	// 常駐メモリデータ入出力構造体に値を設定する
	data.dataId = e_SC_DH_SHARE_SHOWLANDMARKATTR;
	landmark.maps = maps;
	landmark.classCode = classCode;
	landmark.isShow = isShow;
	data.data = (void*)&landmark;

	// 常駐メモリデータを設定する
	ret = SC_DH_SetShareData(&data);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
	}

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 地図表示モードを取得する
 * @param[in]  maps       表示操作対象地図
 * @param[out] isDispMode 地図表示モード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetDispMode(INT32 maps, INT32 *isDispMode)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_DISPMODE	mode = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == isDispMode) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[isDispMode], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_DISPMODE;
		mode.maps = maps;
		data.data = (void*)&mode;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_GetShareData error, " HERE);
			break;
		}
		*isDispMode = mode.dispMode;
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 地図表示モードを設定する
 * @param[in] maps       表示操作対象地図
 * @param[in] isDispMode isDispMode 地図表示モード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetDispMode(INT32 maps, INT32 isDispMode)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_DISPMODE	mode = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	// 常駐メモリデータ入出力構造体に値を設定する
	data.dataId = e_SC_DH_SHARE_DISPMODE;
	mode.maps = maps;
	mode.dispMode = isDispMode;
	data.data = (void*)&mode;

	// 常駐メモリデータを設定する
	ret = SC_DH_SetShareData(&data);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
	}

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 地図ドライバモードを取得する
 * @param[in]  maps         表示操作対象地図
 * @param[out] isDriverMode 地図ドライバモード(true:ドライバーモード、false:ドライバーモードではない)
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetDriverMode(INT32 maps, Bool *isDriverMode)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_DRIVERMODE	mode = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == isDriverMode) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[isDriverMode], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_DRIVERMODE;
		mode.maps = maps;
		data.data = (void*)&mode;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_GetShareData error, " HERE);
			break;
		}
		*isDriverMode = mode.driverMode;
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 地図ドライバモードを設定する
 * @param[in] maps         表示操作対象地図
 * @param[in] isDriverMode 地図ドライバモード(true:ドライバーモード、false:ドライバーモードではない)
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetDriverMode(INT32 maps, Bool isDriverMode)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_DRIVERMODE	mode = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	// 常駐メモリデータ入出力構造体に値を設定する
	data.dataId = e_SC_DH_SHARE_DRIVERMODE;
	mode.maps = maps;
	mode.driverMode = isDriverMode;
	data.data = (void*)&mode;

	// 常駐メモリデータを設定する
	ret = SC_DH_SetShareData(&data);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
	}

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 地図中心カーソルの表示ステータスを取得する
 * @param[in]  maps   表示操作対象地図
 * @param[out] isShow 地図中心カーソルの表示・非表示(true:表示する、false:表示しない)
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetShowCursor(INT32 maps, Bool *isShow)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_SHOWCURSOR	cursor = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == isShow) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[isShow], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_SHOWCURSOR;
		cursor.maps = maps;
		data.data = (void*)&cursor;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_GetShareData error, " HERE);
			break;
		}
		*isShow = cursor.isShow;
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 地図中心カーソルの表示ステータスを設定する
 * @param[in] maps   表示操作対象地図
 * @param[in] isShow 地図中心カーソルの表示・非表示(true:表示する、false:表示しない)
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetShowCursor(INT32 maps, Bool isShow)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_SHOWCURSOR	cursor = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	// 常駐メモリデータ入出力構造体に値を設定する
	data.dataId = e_SC_DH_SHARE_SHOWCURSOR;
	cursor.maps = maps;
	cursor.isShow = isShow;
	data.data = (void*)&cursor;

	// 常駐メモリデータを設定する
	ret = SC_DH_SetShareData(&data);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
	}

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 車両状態情報取得を取得する
 * @param[out] carState 車両状態情報構造体のポインタ
 * @param[in]  mode     情報の取得元
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetCarState(SMCARSTATE *carState, INT32 mode)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_CARSTATE	state = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == carState) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[carState], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		if (e_SC_CARLOCATION_REAL == mode) {
			data.dataId = e_SC_DH_SHARE_CARSTATE_REAL;
		} else if (e_SC_CARLOCATION_SIMU == mode) {
			data.dataId = e_SC_DH_SHARE_CARSTATE_SIMU;
		} else {
			data.dataId = e_SC_DH_SHARE_CARSTATE;
		}
		data.data = (void*)&state;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_GetShareData error, " HERE);
			break;
		}
		memcpy(carState, &state.carState, sizeof(SMCARSTATE));
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 車両状態情報を設定する
 * @param[in] carState  車両状態情報構造体のポインタ
 * @param[in] mode      情報の設定先
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetCarState(const SMCARSTATE *carState, INT32 mode)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_CARSTATE	state = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == carState) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[carState], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		if (e_SC_CARLOCATION_REAL == mode) {
			data.dataId = e_SC_DH_SHARE_CARSTATE_REAL;
		} else if (e_SC_CARLOCATION_SIMU == mode) {
			data.dataId = e_SC_DH_SHARE_CARSTATE_SIMU;
		} else {
			data.dataId = e_SC_DH_SHARE_CARSTATE;
		}
		memcpy(&state.carState, carState, sizeof(SMCARSTATE));
		data.data = (void*)&state;

		// 常駐メモリデータを設定する
		ret = SC_DH_SetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 地図スクロールモードを取得する
 * @param[out] scrollMode スクロールモード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetScrollMode(Bool *scrollMode)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_SCROLLMODE	mode = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == scrollMode) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[scrollMode], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_SCROLLMODE;
		data.data = (void*)&mode;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_GetShareData error, " HERE);
			break;
		}
		*scrollMode = mode.mode;
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 地図スクロールモードを設定する
 * @param[in] scrollMode  スクロールモード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetScrollMode(Bool scrollMode)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_SCROLLMODE	mode = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);


	// 常駐メモリデータ入出力構造体に値を設定する
	data.dataId = e_SC_DH_SHARE_SCROLLMODE;
	mode.mode = scrollMode;
	data.data = (void*)&mode;

	// 常駐メモリデータを設定する
	ret = SC_DH_SetShareData(&data);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
	}

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 地図の移動情報(移動角度と移動長さ)を取得する
 * @param[in]  maps         地図種類
 * @param[out] degreeToward 移動角度
 * @param[out] pixelStep    移動長さ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetMoveMapDir(INT32 maps, FLOAT *degreeToward, INT32 *pixelStep)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_MOVEMAPDIR	dir = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == degreeToward) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[degreeToward], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == pixelStep) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[pixelStep], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_MOVEMAPDIR;
		dir.maps = maps;
		data.data = (void*)&dir;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_GetShareData error, " HERE);
			break;
		}
		*degreeToward = dir.degreeToward;
		*pixelStep    = dir.pixelStep;
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 地図の移動情報(移動角度と移動長さ)を設定する
 * @param[in] maps         地図種類
 * @param[in] degreeToward 移動角度
 * @param[in] pixelStep    移動長さ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetMoveMapDir(INT32 maps, FLOAT degreeToward, INT32 pixelStep)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_MOVEMAPDIR	dir = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	// 常駐メモリデータ入出力構造体に値を設定する
	data.dataId = e_SC_DH_SHARE_MOVEMAPDIR;
	dir.degreeToward = degreeToward;
	dir.pixelStep    = pixelStep;
	data.data = (void*)&dir;

	// 常駐メモリデータを設定する
	ret = SC_DH_SetShareData(&data);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
	} else {

		ret = SC_DRAW_MoveMapDir(maps);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DRAW_MoveMapDir error, %x" HERE, ret);
		}
	}

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief フリーズーム時の地図拡大比例を取得する
 * @param[in]  maps     表示操作対象地図
 * @param[out] stepRate フリーズーム時の地図拡大比例
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetZoomStepRate(INT32 maps, FLOAT *stepRate)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_STEPRATE	rate = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == stepRate) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[stepRate], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_ZOOMSTEPRATE;
		rate.maps = maps;
		data.data = (void*)&rate;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_GetShareData error, " HERE);
			break;
		}
		*stepRate = rate.rate;
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief フリーズーム時の地図拡大比例を設定する
 * @param[in] maps      表示操作対象地図
 * @param[in] stepRate  フリーズーム時の地図拡大比例
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetZoomStepRate(INT32 maps, FLOAT stepRate)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_STEPRATE	rate = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);


	// 常駐メモリデータ入出力構造体に値を設定する
	data.dataId = e_SC_DH_SHARE_ZOOMSTEPRATE;
	rate.maps = maps;
	rate.rate = stepRate;
	data.data = (void*)&rate;

	// 常駐メモリデータを設定する
	ret = SC_DH_SetShareData(&data);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
	} else {

		ret = SC_DRAW_SetZoomStepRate(maps);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DRAW_SetZoomStepRate error, %x" HERE, ret);
		}
	}

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 地図中心の地理座標を取得する
 * @param[in]  maps     表示操作対象地図
 * @param[out] geoCoord 地図中心の地理座標
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetMapCursorCoord(INT32 maps, SMGEOCOORD *geoCoord)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_GEOCOORD	coord = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == geoCoord) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[geoCoord], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_MAPCURSORCOORD;
		coord.maps = maps;
		data.data = (void*)&coord;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_GetShareData error, " HERE);
			break;
		}
		memcpy(geoCoord, &coord.coord, sizeof(SMGEOCOORD));

	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 地図中心の地理座標を設定する
 * @param[in] maps      表示操作対象地図
 * @param[in] geoCoord  地図中心の地理座標
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetMapCursorCoord(INT32 maps, const SMGEOCOORD *geoCoord)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_GEOCOORD	coord = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == geoCoord) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[geoCoord], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_MAPCURSORCOORD;
		coord.maps = maps;
		memcpy(&coord.coord, geoCoord, sizeof(SMGEOCOORD));
		data.data = (void*)&coord;

		// 常駐メモリデータを設定する
		ret = SC_DH_SetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 地図リソースを初期化する
 * @param[in] maps  表示操作対象地図
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_InitResource(INT32 maps)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// 地図リソース初期化
		ret = SC_DRAW_InitResource(maps);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, (Char*)"SC_DRAW_InitResource error(0x%08x), " HERE, ret);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 地図のビューポートを取得する
 * @param[in]  maps 表示操作対象地図
 * @param[out] rect 地図のビューポート
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetMapViewPort(INT32 maps, SMRECT *rect)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_RECT	rct = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == rect) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[rect], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_MAPVIEWPORT;
		rct.maps = maps;
		data.data = (void*)&rct;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_GetShareData error, " HERE);
			break;
		}
		memcpy(rect, &rct.rect, sizeof(SMRECT));
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 地図のビューポートを設定する
 * @param[in] maps  表示操作対象地図
 * @param[in] rect  地図のビューポート
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetMapViewPort(INT32 maps, const SMRECT *rect)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_RECT	rct = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == rect) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[rect], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_MAPVIEWPORT;
		rct.maps = maps;
		memcpy(&rct.rect, rect, sizeof(SMRECT));
		data.data = (void*)&rct;

		if (rct.rect.right > 1 && rct.rect.bottom > 1) {
			// 常駐メモリデータを設定する
			ret = SC_DH_SetShareData(&data);
			if (e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
				break;
			}
		}

		// 描画ビューポート設定
		ret = SC_DRAW_SetViewport(maps, rect);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, (Char*)"SC_DRAW_SetViewport error(0x%08x), " HERE, ret);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 地図の回転角度を取得する
 * @param[in]  maps 表示操作対象地図
 * @param[out] rect 地図の回転角度
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetMapRotate(INT32 maps, INT32 *rotate)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_ROTATE	rtt = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == rotate) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[rotate], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_MAPROTATE;
		rtt.maps = maps;
		data.data = (void*)&rtt;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_GetShareData error, " HERE);
			break;
		}
		*rotate = rtt.rotate;
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 地図の回転角度を設定する
 * @param[in] maps  表示操作対象地図
 * @param[in] rect  地図の回転角度
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetMapRotate(INT32 maps, INT32 rotate)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_ROTATE	rtt = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	// 常駐メモリデータ入出力構造体に値を設定する
	data.dataId = e_SC_DH_SHARE_MAPROTATE;
	rtt.maps = maps;
	rtt.rotate = rotate;
	data.data = (void*)&rtt;

	// 常駐メモリデータを設定する
	ret = SC_DH_SetShareData(&data);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
	}

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 地図のスケールを取得する
 * @param[in]  maps      表示操作対象地図
 * @param[in]  unitPixel 単位ピクセル
 * @param[out] scale 単位ピクセル
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetGeoDistance(INT32 maps, INT32 unitPixel, FLOAT *scale)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_GEODISTANCE	dis = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == scale) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[scale], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_GEODISTANCE;
		dis.maps = maps;
		dis.unitPixel = unitPixel;
		data.data = (void*)&dis;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_GetShareData error, " HERE);
			break;
		}
		*scale = dis.scale;
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 地図のスケールを設定する
 * @param[in] maps       表示操作対象地図
 * @param[in] unitPixel  単位ピクセル
 * @param[in] scale      スケール
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetGeoDistance(INT32 maps, INT32 unitPixel, FLOAT scale)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_GEODISTANCE	dis = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	// 常駐メモリデータ入出力構造体に値を設定する
	data.dataId = e_SC_DH_SHARE_GEODISTANCE;
	dis.maps = maps;
	dis.unitPixel = unitPixel;
	dis.scale = scale;
	data.data = (void*)&dis;

	// 常駐メモリデータを設定する
	ret = SC_DH_SetShareData(&data);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
	}

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 地図の解像度を取得する
 * @param[out] width  解像度(X座標)
 * @param[out] height 解像度(Y座標)
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetResolution(INT32 *width, INT32 *height)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_RESOLUTION	rsn = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == width) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[width], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == height) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[height], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_RESOLUTION;
		data.data = (void*)&rsn;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_GetShareData error, " HERE);
			break;
		}
		*width  = rsn.width;
		*height = rsn.height;
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 地図の解像度を設定する
 * @param[in] width  解像度(X座標)
 * @param[in] height 解像度(Y座標)
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetResolution(INT32 width, INT32 height)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_RESOLUTION	rsn = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	// 常駐メモリデータ入出力構造体に値を設定する
	data.dataId = e_SC_DH_SHARE_RESOLUTION;
	rsn.width  = width;
	rsn.height = height;
	data.data = (void*)&rsn;

	// 常駐メモリデータを設定する
	ret = SC_DH_SetShareData(&data);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
	}

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 地点情報（出発地、経由地、目的地）を取得する
 * @param[out] allRPPoint    地点情報（出発地、経由地、目的地）
 * @param[out] allRPPointNum 地点情報（出発地、経由地、目的地）数
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetAllRPPlace(SMRPPOINT *allRPPoint, INT32 *allRPPointNum)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_RPPOINT	point = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == allRPPoint) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[allRPPoint], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == allRPPointNum) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[allRPPointNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_RPPOINT;
		data.data = (void*)&point;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_GetShareData error, " HERE);
			break;
		}
		*allRPPointNum = point.pointNum;
		if (0 < point.pointNum) {
			memcpy(allRPPoint, point.point, (sizeof(SMRPPOINT) * point.pointNum));
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 地点情報（出発地、経由地、目的地）を設定する
 * @param[in] allRPPoint    地点情報（出発地、経由地、目的地）
 * @param[in] allRPPointNum 地点情報（出発地、経由地、目的地）数
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetAllRPPlace(const SMRPPOINT *allRPPoint, INT32 allRPPointNum)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_RPPOINT	point = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if ((0 != allRPPointNum) && (NULL == allRPPoint)) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[allRPPoint], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((0 > allRPPointNum) || COUNTOF(point.point) < allRPPointNum) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[allRPPointNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_RPPOINT;
		point.pointNum = allRPPointNum;
		if (0 < allRPPointNum) {
			memcpy(point.point, allRPPoint, (sizeof(SMRPPOINT) * allRPPointNum));
		}
		data.data = (void*)&point;

		// 常駐メモリデータを設定する
		ret = SC_DH_SetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 現在ルートの探索結果の有無を取得する
 * @param[out] isExistRoute 探索結果の有無
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetExistRoute(Bool *isExistRoute)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_EXISTROUTE	exist = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == isExistRoute) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[isExistRoute], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_EXISTROUTE;
		data.data = (void*)&exist;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_GetShareData error, " HERE);
			break;
		}
		*isExistRoute = exist.isExistRoute;
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 現在ルートの探索結果の有無を設定する
 * @param[in] isExistRoute  探索結果の有無
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetExistRoute(Bool isExistRoute)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_EXISTROUTE	exist = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_EXISTROUTE;
		exist.isExistRoute = isExistRoute;
		data.data = (void*)&exist;

		// 常駐メモリデータを設定する
		ret = SC_DH_SetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 探索中かどうかを取得する
 * @param[out] isPlanning   探索中かどうか(探索中：true、探索完了または探索未実行：false)
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetPlanning(Bool *isPlanning)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_PLANNING	plan = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == isPlanning) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[isPlanning], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_PLANNING;
		data.data = (void*)&plan;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_GetShareData error, " HERE);
			break;
		}
		*isPlanning = plan.isPlanning;
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 探索中かどうかを設定する
 * @param[in] isPlanning    探索中かどうか(探索中：true、探索完了または探索未実行：false)
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetPlanning(Bool isPlanning)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_PLANNING	plan = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_PLANNING;
		plan.isPlanning = isPlanning;
		data.data = (void*)&plan;

		// 常駐メモリデータを設定する
		ret = SC_DH_SetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 探索条件を取得する
 * @param[out] option    探索条件
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetRPOption(SMRPOPTION *option)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_RPOPTION	op = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == option) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[option], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_RPOPTION;
		data.data = (void*)&op;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_GetShareData error, " HERE);
			break;
		}
		memcpy(option, &op.option, sizeof(SMRPOPTION));
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 探索条件を設定する
 * @param[in] option     探索条件
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetRPOption(const SMRPOPTION *option)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_RPOPTION	op = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == option) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[option], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_RPOPTION;
		memcpy(&op.option, option, sizeof(SMRPOPTION));
		data.data = (void*)&op;

		// 常駐メモリデータを設定する
		ret = SC_DH_SetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 探索のエラー情報を取得する
 * @param[out] tipInfo  探索のエラー情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetRPTip(SMRPTIPINFO *tipInfo)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_RPTIPINFO	tip = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == tipInfo) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[tipInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_RPTIP;
		data.data = (void*)&tip;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_GetShareData error, " HERE);
			break;
		}
		memcpy(tipInfo, &tip.tipInfo, sizeof(SMRPTIPINFO));
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 探索のエラー情報を設定する
 * @param[in] tipInfo   探索のエラー情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetRPTip(const SMRPTIPINFO *tipInfo)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_RPTIPINFO	tip = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == tipInfo) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[tipInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_RPTIP;
		memcpy(&tip.tipInfo, tipInfo, sizeof(SMRPTIPINFO));
		data.data = (void*)&tip;

		// 常駐メモリデータを設定する
		ret = SC_DH_SetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 探索をキャンセルする
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_CancelPlanningRoute()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	pthread_msq_msg_t msg = {};
	Bool	isPlanning = false;

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		/*** 探索探索中かどうか取得 ***/
		ret = SC_MNG_GetPlanning(&isPlanning);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, (Char*) "SC_MNG_GetPlanning error(0x%08x), " HERE, ret);
			ret = e_SC_RESULT_FAIL;
			break;
		}
		if (false == isPlanning) {
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 探索キャンセル要求メッセージ生成
		msg.data[SC_MSG_MSG_ID] = e_SC_MSGID_REQ_RM_CANCEL;		// メッセージID(探索キャンセル)

		// 送信メッセージをログ出力
		SC_LOG_DebugPrint(SC_TAG_CORE,
				"sendMsg=0x%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x, " HERE,
				msg.data[0], msg.data[1], msg.data[2], msg.data[3], msg.data[4],
				msg.data[5], msg.data[6], msg.data[7], msg.data[8], msg.data[9]);

		// 経路探索スレッドに探索キャンセル要求メッセージ送信
		if (PTHREAD_MSQ_OK != pthread_msq_msg_send(SC_CORE_MSQID_RM, &msg, SC_CORE_MSQID_FM)) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "pthread_msq_msg_send error, " HERE);
			ret = e_SC_RESULT_FAIL;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief カレント経路（地点及び結果を含む）を削除する
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_DeleteRouteResult()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	// 常駐メモリデータ入出力構造体に値を設定する
	data.dataId = e_SC_DH_SHARE_DELROUTERESULT;
	data.data = (void*)NULL;

	// 常駐メモリデータを設定する
	ret = SC_DH_SetShareData(&data);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
	}
	// 確定経路の削除を行う(カレントに初期値設定)
	//SC_RP_RouteSetId(SC_RP_RTIDINIT, SC_RP_RTTYPEINIT);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 地図を再描画する
 * @param[in] maps  表示操作対象地図
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_RefreshMap(INT32 maps)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
#if 0
	pthread_msq_msg_t msg = {};
#endif

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

#if 0
	// 地図描画要求メッセージ生成
	msg.data[SC_MSG_MSG_ID] = e_SC_MSGID_REQ_REFRESH_MAP;	// メッセージID(地図描画要求)
	msg.data[SC_MSG_REQ_RFRSH_MPS] = maps;					// 表示操作対象地図

	// 送信メッセージをログ出力
	SC_LOG_DebugPrint(SC_TAG_CORE,
			"sendMsg=0x%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x, " HERE,
			msg.data[0],  msg.data[1],  msg.data[2],  msg.data[3],  msg.data[4],
			msg.data[5],  msg.data[6],  msg.data[7],  msg.data[8],  msg.data[9]);

	// 描画スレッドに地図描画要求メッセージ送信
	if (PTHREAD_MSQ_OK != pthread_msq_msg_send(SC_CORE_MSQID_MP, &msg, SC_CORE_MSQID_FM)) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "pthread_msq_msg_send error, " HERE);
		ret = e_SC_RESULT_FAIL;
	}
#else
	// 再描画
	ret = SC_DRAW_Refresh(maps);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DRAW_Refresh error, " HERE);
	}
#endif

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 経路探索する
 * @param[in] route 探索条件(単経路探索、複数経路探索)
 * @param[in] type  探索条件種別
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_DoRoute(E_ROUTE route)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	pthread_msq_msg_t msg = {};
	//SMRPTIPINFO	tipInfo = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		if (e_ROUTE_SINGLE == route) {
			// 単経路探索要求メッセージ生成
			msg.data[SC_MSG_MSG_ID] = e_SC_MSGID_REQ_RM_RTSINGLE;	// メッセージID(単経路探索要求)
			msg.data[SC_MSG_REQ_RT_USELEVEL] = 2;					// TODO レベル定義
		} else if (e_ROUTE_SINGLE_LV1 == route) {
			msg.data[SC_MSG_MSG_ID] = e_SC_MSGID_REQ_RM_RTSINGLE;	// メッセージID(単経路探索要求)
			msg.data[SC_MSG_REQ_RT_USELEVEL] = 1;					// TODO レベル定義
		} else {
			// 複数経路探索要求メッセージ生成
			msg.data[SC_MSG_MSG_ID] = e_SC_MSGID_REQ_RM_RTMULTI;	// メッセージID(複数経路探索要求)
		}

		// 送信メッセージをログ出力
		SC_LOG_DebugPrint(SC_TAG_CORE,
				"sendMsg=0x%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x, " HERE,
				msg.data[0],  msg.data[1],  msg.data[2],  msg.data[3],  msg.data[4],
				msg.data[5],  msg.data[6],  msg.data[7],  msg.data[8],  msg.data[9]);

		// 経路探索スレッドに経路探索要求メッセージ送信
		if (PTHREAD_MSQ_OK != pthread_msq_msg_send(SC_CORE_MSQID_RM, &msg, SC_CORE_MSQID_FM)) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "pthread_msq_msg_send error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// セマフォロック
#ifdef __SMS_APPLE__
		ret = SC_LockSemaphore(syncAPISem);
#else
		ret = SC_LockSemaphore(&syncAPISem);
#endif /* __SMS_APPLE__ */
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_LockSemaphore error, " HERE);
			break;
		}

		// 処理結果取得
		ret = apiResult;

	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 再探索する
 * @param[in] route 探索条件(単経路探索、複数経路探索)
 * @param[in] type  探索条件種別
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_RePlan(E_ROUTE route)
{
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	pthread_msq_msg_t msg = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// 再探索要求メッセージ生成
		msg.data[SC_MSG_MSG_ID] = e_SC_MSGID_REQ_RM_REROUTE;

		// 送信メッセージをログ出力
		SC_LOG_DebugPrint(SC_TAG_CORE,
				"sendMsg=0x%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x, " HERE,
				msg.data[0],  msg.data[1],  msg.data[2],  msg.data[3],  msg.data[4],
				msg.data[5],  msg.data[6],  msg.data[7],  msg.data[8],  msg.data[9]);

		// メッセージ送信 FM->RM
		if (PTHREAD_MSQ_OK != pthread_msq_msg_send(SC_CORE_MSQID_RM, &msg, SC_CORE_MSQID_FM)) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "pthread_msq_msg_send error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// セマフォロック
#ifdef __SMS_APPLE__
		ret = SC_LockSemaphore(syncAPISem);
#else
		ret = SC_LockSemaphore(&syncAPISem);
#endif /* __SMS_APPLE__ */
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_LockSemaphore error, " HERE);
			break;
		}

		// 処理結果取得
		ret = apiResult;

		/** 探索結果の有無フラグ変更なし **/
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 経路誘導する
 * @param[in] event 誘導状態
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_DoGuide(E_GUIDE_STATUS event)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	pthread_msq_msg_t msg = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		if (e_GUIDE_STATUS_START == event) {
			// 経路誘導開始要求メッセージ生成
			msg.data[SC_MSG_MSG_ID] = e_SC_MSGID_REQ_RG_GUIDESTART;
		} else if (e_GUIDE_STATUS_RUN == event) {
			// 経路誘導実行要求メッセージ生成
			msg.data[SC_MSG_MSG_ID] = e_SC_MSGID_REQ_RG_GUIDERUN;
		} else if (e_GUIDE_STATUS_STOP == event) {
			// 経路誘導終了要求メッセージ生成
			msg.data[SC_MSG_MSG_ID] = e_SC_MSGID_REQ_RG_GUIDESTOP;
		} else {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "pthread_msq_msg_send error, " HERE);
			ret = e_SC_RESULT_FAIL;
		}

		// 送信メッセージをログ出力
		SC_LOG_DebugPrint(SC_TAG_CORE,
				"sendMsg=0x%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x, " HERE,
				msg.data[0],  msg.data[1],  msg.data[2],  msg.data[3],  msg.data[4],
				msg.data[5],  msg.data[6],  msg.data[7],  msg.data[8],  msg.data[9]);

		// 経路誘導スレッドに経路誘導要求メッセージ送信
		if (PTHREAD_MSQ_OK != pthread_msq_msg_send(SC_CORE_MSQID_RG, &msg, SC_CORE_MSQID_FM)) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "pthread_msq_msg_send error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// セマフォロック
#ifdef __SMS_APPLE__
		ret = SC_LockSemaphore(syncAPISem);
#else
		ret = SC_LockSemaphore(&syncAPISem);
#endif /* __SMS_APPLE__ */
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_LockSemaphore error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 処理結果取得
		if (e_SC_RESULT_SUCCESS != apiResult) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "guide error, " HERE);
			ret = e_SC_RESULT_FAIL;
		}

	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief リアルタイム案内情報を取得する
 * @param[out] guideData    リアルタイム案内情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetRealTimeInfo(SMREALTIMEGUIDEDATA *guideData)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_GUIDE_DATA	guide = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == guideData) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[guideData], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_GUIDEREALTIMEINFO;
		data.data = (void*)&guide;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
		memcpy(guideData, &guide.guideData, sizeof(SMREALTIMEGUIDEDATA));
	} while (0);

//	SC_LOG_InfoPrint(SC_TAG_CORE, "guide data = dir[%d] / dist[%d]" HERE, guideData->turnDir, guideData->remainDistToNextTurn);
	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief リアルタイム案内情報を設定する
 * @param[in] guideData     リアルタイム案内情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetRealTimeInfo(const SMREALTIMEGUIDEDATA *guideData)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_GUIDE_DATA	guide = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == guideData) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[guideData], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_GUIDEREALTIMEINFO;
		memcpy(&guide.guideData, guideData, sizeof(SMREALTIMEGUIDEDATA));
		data.data = (void*)&guide;

		// 常駐メモリデータを設定する
		ret = SC_DH_SetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 誘導状態を取得する
 * @param[out] status   誘導状態
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetGuideStatus(E_GUIDE_STATUS *status)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_GUIDE_STATUS	sts = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == status) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[status], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_GUIDESTATUS;
		data.data = (void*)&sts;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
		*status = sts.guide_status;
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 誘導状態を設定する
 * @param[in] status    誘導状態
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetGuideStatus(E_GUIDE_STATUS status)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_GUIDE_STATUS	sts = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_GUIDESTATUS;
		sts.guide_status = status;
		data.data = (void*)&sts;

		// 常駐メモリデータを設定する
		ret = SC_DH_SetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 音声ＴＴＳ情報を取得する
 * @param[out] voiceTTS    音声ＴＴＳ情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetVoiceTTS(SMVOICETTS *voiceTTS)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_VOICE_TTS	voice = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == voiceTTS) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[voiceTTS], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_VOICETTS;
		data.data = (void*)&voice;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
		memcpy(voiceTTS, &voice.voiceTTS, sizeof(SMVOICETTS));
	} while (0);

//	SC_LOG_InfoPrint(SC_TAG_CORE, "voice tts = %s (%d [byte])" HERE, voiceTTS->tts, voiceTTS->len);
	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 音声ＴＴＳ情報を設定する
 * @param[in] voiceTTS     音声ＴＴＳ情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetVoiceTTS(const SMVOICETTS *voiceTTS)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_VOICE_TTS	voice = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == voiceTTS) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[voiceTTS], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_VOICETTS;
		memcpy(&voice.voiceTTS, voiceTTS, sizeof(SMVOICETTS));
		data.data = (void*)&voice;

		// 常駐メモリデータを設定する
		ret = SC_DH_SetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief シュミレーションを行う
 * @param[in] event 誘導状態
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_DoSimulation(E_SC_SIMSTATE event)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	pthread_msq_msg_t msg = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		if (e_SIMULATE_STATE_READY == event) {
			// シュミレーション開始要求メッセージ生成
			msg.data[SC_MSG_MSG_ID] = e_SC_MSGID_REQ_RG_SIMSTART;
		} else if (e_SIMULATE_STATE_STRAT == event) {
			// シュミレーション実行要求メッセージ生成
			msg.data[SC_MSG_MSG_ID] = e_SC_MSGID_REQ_RG_SIMRUN;
		} else if (e_SIMULATE_STATE_STOP == event) {
			// シュミレーション終了要求メッセージ生成
			msg.data[SC_MSG_MSG_ID] = e_SC_MSGID_REQ_RG_SIMEXIT;
		} else {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "pthread_msq_msg_send error, " HERE);
			ret = e_SC_RESULT_FAIL;
		}

		// 送信メッセージをログ出力
		SC_LOG_DebugPrint(SC_TAG_CORE,
				"sendMsg=0x%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x, " HERE,
				msg.data[0],  msg.data[1],  msg.data[2],  msg.data[3],  msg.data[4],
				msg.data[5],  msg.data[6],  msg.data[7],  msg.data[8],  msg.data[9]);

		// 経路誘導スレッドに経路誘導要求メッセージ送信
		if (PTHREAD_MSQ_OK != pthread_msq_msg_send(SC_CORE_MSQID_RG, &msg, SC_CORE_MSQID_FM)) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "pthread_msq_msg_send error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// セマフォロック
#ifdef __SMS_APPLE__
		ret = SC_LockSemaphore(syncAPISem);
#else
		ret = SC_LockSemaphore(&syncAPISem);
#endif /* __SMS_APPLE__ */
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_LockSemaphore error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 処理結果取得
		if (e_SC_RESULT_SUCCESS != apiResult) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "simulation error, " HERE);
			ret = e_SC_RESULT_FAIL;
		}

	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief シュミレーションを取得する
 * @param[out] status    シュミレーション環境
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetSimulate(E_SC_SIMULATE *status)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_SIMULATE	sts = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == status) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[status], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体にを設定する
		data.dataId = e_SC_DH_SHARE_SIMULATE;
		data.data = (void*)&sts;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
		*status = sts.sumilate;
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief シュミレーションを取得する
 * @param[out] status    シュミレーション状態
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetSimulationStatus(E_SC_SIMSTATE *status)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_SIMULATE_STATE	sts = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == status) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[status], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体にを設定する
		data.dataId = e_SC_DH_SHARE_SIMULATESTATE;
		data.data = (void*)&sts;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
		*status = sts.state;
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}


/**
 * @brief シュミレーション速度を取得する
 * @param[in] status     シュミレーション状態
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetSimulationSpeed(INT32 *speed)
{
	E_SC_RESULT					ret  = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA			data = {};
	SC_DH_SHARE_SIMULATE_SPEED	sts  = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == speed) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[speed], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体にを設定する
		data.dataId = e_SC_DH_SHARE_SIMULATESPEED;
		data.data = (void*)&sts;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
		*speed = sts.speed;
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}


/**
 * @brief シュミレーション環境を登録する
 * @param[in] status     シュミレーション状態
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetSimulate(E_SC_SIMULATE status)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_SIMULATE	sts = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_SIMULATE;
		sts.sumilate = status;
		data.data = (void*)&sts;

		// 常駐メモリデータを設定する
		ret = SC_DH_SetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief シュミレーション状態を登録する
 * @param[in] status     シュミレーション状態
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetSimulationStatus(E_SC_SIMSTATE status)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_SIMULATE_STATE	sts = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	SC_LOG_InfoPrint(SC_TAG_RG, "SC_MNG_SetSimulationStatus status %d",status);
	do {
		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_SIMULATESTATE;
		sts.state = status;
		data.data = (void*)&sts;

		// 常駐メモリデータを設定する
		ret = SC_DH_SetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}


/**
 * @brief シュミレーション速度を登録する
 * @param[in] status     シュミレーション状態
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetSimulationSpeed(INT32 speed)
{
	E_SC_RESULT					ret  = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA			data = {};
	SC_DH_SHARE_SIMULATE_SPEED	sts  = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_SIMULATESPEED;
		sts.speed = speed;
		data.data = (void*)&sts;

		// 常駐メモリデータを設定する
		ret = SC_DH_SetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return(ret);
}

/**
 * @brief ターンリスト作成する
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_DoTurnList()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	pthread_msq_msg_t msg = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// 要求メッセージ設定
		msg.data[SC_MSG_MSG_ID] = e_SC_MSGID_REQ_RG_TURNLIST;

		// 経路誘導スレッドに経路誘導要求メッセージ送信
		if (PTHREAD_MSQ_OK != pthread_msq_msg_send(SC_CORE_MSQID_RG, &msg, SC_CORE_MSQID_FM)) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "pthread_msq_msg_send error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// セマフォロック
#ifdef __SMS_APPLE__
		ret = SC_LockSemaphore(syncAPISem);
#else
		ret = SC_LockSemaphore(&syncAPISem);
#endif /* __SMS_APPLE__ */
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_LockSemaphore error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 処理結果取得
		if (e_SC_RESULT_SUCCESS != apiResult) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "turnList error, " HERE);
			ret = e_SC_RESULT_FAIL;
		}

	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief ターンリスト情報を取得する
 * @param[out] turnList    ターンリスト情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetTurnList(SMTURNLIST *turnList)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA		data = {};
	SC_DH_SHARE_TURN_LIST	turn = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == turnList) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[turnList], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_TURNLIST;
		data.data = (void*)&turn;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
		memcpy(turnList, &turn.turnList, sizeof(SMTURNLIST));
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief ターンリスト情報を設定する
 * @param[in] turnList     リアルタイム案内情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetTurnList(const SMTURNLIST *turnList)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA		data = {};
	SC_DH_SHARE_TURN_LIST	turn = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == turnList) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[turnList], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_TURNLIST;
		memcpy(&turn.turnList, turnList, sizeof(SMTURNLIST));
		data.data = (void*)&turn;

		// 常駐メモリデータを設定する
		ret = SC_DH_SetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief バックアップ処理
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SaveShareData()
{
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	// 共有メモリバックアップ
	ret = SC_DH_SaveShareData();
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SaveShareData error, " HERE);
	}

	return (ret);
}

/**
 * @brief 経路総距離値を取得する
 * @param[in] length     経路距離値設定先アドレス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetRouteLength(INT32 *length)
{
	E_SC_RESULT					ret  = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA			data = {};
	SC_DH_SHARE_ROUTE_LENGTH	sts  = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == length) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[length], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体にを設定する
		data.dataId = e_SC_DH_SHARE_ROUTELENGTH;
		data.data = (void*)&sts;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
		*length = sts.length;
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 経路総距離値を登録する
 * @param[in] length     登録経路総距離値
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetRouteLength(INT32 length)
{
	E_SC_RESULT					ret  = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA			data = {};
	SC_DH_SHARE_ROUTE_LENGTH	sts  = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_ROUTELENGTH;
		sts.length = length;
		data.data = (void*)&sts;

		// 常駐メモリデータを設定する
		ret = SC_DH_SetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 経路所要時間を取得する
 * @param[in] length     経路距離値設定先アドレス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetRouteAveTime(INT32 *avetime)
{
	E_SC_RESULT					ret  = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA			data = {};
	SC_DH_SHARE_ROUTE_AVETIME	sts  = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == avetime) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[length], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体にを設定する
		data.dataId = e_SC_DH_SHARE_ROUTEAVETIME;
		data.data = (void*)&sts;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
		*avetime = sts.avetime;
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 経路所要時間を登録する
 * @param[in] length     経路所要時間値
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetRouteAveTime(INT32 avetime)
{
	E_SC_RESULT					ret  = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA			data = {};
	SC_DH_SHARE_ROUTE_AVETIME	sts  = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_ROUTEAVETIME;
		sts.avetime  = avetime;
		data.data = (void*)&sts;

		// 常駐メモリデータを設定する
		ret = SC_DH_SetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 経路高速道路距離値を取得する
 * @param[in] hwaylength     経路高速道路距離値設定先アドレス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetRouteHwayLength(INT32 *hwaylength)
{
	E_SC_RESULT						ret  = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA				data = {};
	SC_DH_SHARE_ROUTE_HWAYLENGTH	sts  = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == hwaylength) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[length], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体にを設定する
		data.dataId = e_SC_DH_SHARE_ROUTEHWAYLENGTH;
		data.data = (void*)&sts;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
		*hwaylength = sts.hwaylength;
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief経路高速道路距離値を登録する
 * @param[in] length     経路高速道路距離値
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetRouteHwayLength(INT32 hwaylength)
{
	E_SC_RESULT						ret  = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA				data = {};
	SC_DH_SHARE_ROUTE_HWAYLENGTH	sts  = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_ROUTEHWAYLENGTH;
		sts.hwaylength  = hwaylength;
		data.data = (void*)&sts;

		// 常駐メモリデータを設定する
		ret = SC_DH_SetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 経路有料道路距離値を取得する
 * @param[in] hwaylength     経路有料道路距離値設定先アドレス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetRouteTollLength(INT32 *tolllength)
{
	E_SC_RESULT						ret  = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA				data = {};
	SC_DH_SHARE_ROUTE_TOLLLENGTH	sts  = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == tolllength) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[length], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体にを設定する
		data.dataId = e_SC_DH_SHARE_ROUTETOLLLENGTH;
		data.data = (void*)&sts;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
		*tolllength = sts.tolllength;
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 経路有料道路距離値を登録する
 * @param[in] length     経路有料道路距離値
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetRouteTollLength(INT32 tolllength)
{
	E_SC_RESULT						ret  = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA				data = {};
	SC_DH_SHARE_ROUTE_TOLLLENGTH	sts  = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_ROUTETOLLLENGTH;
		sts.tolllength  = tolllength;
		data.data = (void*)&sts;

		// 常駐メモリデータを設定する
		ret = SC_DH_SetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 経路有料道路料金を取得する
 * @param[in] hwaylength     経路有料道路料金値設定先アドレス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetRouteTollFee(INT32 *tollfee)
{
	E_SC_RESULT					ret  = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA			data = {};
	SC_DH_SHARE_ROUTE_TOLLFEE	sts  = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == tollfee) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[length], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体にを設定する
		data.dataId = e_SC_DH_SHARE_ROUTETOLLFEE;
		data.data = (void*)&sts;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
		*tollfee = sts.tollfee;
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 経路有料道路距離値を登録する
 * @param[in] length     経路有料道路距離値
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetRouteTollFee(INT32 tollfee)
{
	E_SC_RESULT					ret  = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA			data = {};
	SC_DH_SHARE_ROUTE_TOLLFEE	sts  = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_ROUTETOLLFEE;
		sts.tollfee  = tollfee;
		data.data = (void*)&sts;

		// 常駐メモリデータを設定する
		ret = SC_DH_SetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 交差点拡大図のサイズを登録する
 * @param[in] Width：Ｘ方向サイズ Height：Ｙ方向サイズ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetDynamicGraphicSize(INT32 Width,INT32 Height)
{
	E_SC_RESULT						ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA				data = {};
	SC_DH_SHARE_DYNAMICGRAPHISIZE	sts;

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	SC_LOG_DebugPrint(SC_TAG_CORE, (Char*)"SC_MNG_SetDynamicGraphicSize Width %d Height %d " HERE, Width,Height);

	do {
		if(Width < 0 || Height < 0){
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		sts.width  = Width;
		sts.height = Height;

		data.dataId = e_SC_DH_SHARE_DYNAMICGRAPHISIZE;
		data.data = (void*)&sts;

		// 常駐メモリデータを設定する
		ret = SC_DH_SetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}


/**
 * @brief 交差点拡大図のサイズを取得する
 * @param[in] Width：Ｘ方向サイズ Height：Ｙ方向サイズ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetDynamicGraphicSize(INT32 *Width,INT32 *Height)
{
	E_SC_RESULT						ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA				data = {};
	SC_DH_SHARE_DYNAMICGRAPHISIZE	sts;

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		if(Width == NULL || Height == NULL){
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		data.dataId = e_SC_DH_SHARE_DYNAMICGRAPHISIZE;
		data.data = (void*)&sts;

		// 常駐メモリデータを設定する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}

		*Width  = sts.width;
		*Height = sts.height;

	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 交差点拡大図の有無を取得
 * @param[in/out] *status：ステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT  SC_MNG_GetDynamicGraphicStatus(E_DYNAMICGRAPHIC_STATUS *status)
{
	E_SC_RESULT							ret  = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA					data = {};
	SC_DH_SHARE_DYNAMICGRAPHICSTATUS	sts  = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		if(NULL == status){
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		data.dataId = e_SC_DH_SHARE_DYNAMICGRAPHISTATUS;
		data.data = (void*)&sts;

		// 常駐メモリデータを設定する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}

		*status = sts.graphic_stat;
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 交差点拡大図の有無を登録
 * @param[in/out] status：ステータス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT  SC_MNG_SetDynamicGraphicStatus(E_DYNAMICGRAPHIC_STATUS status)
{
	E_SC_RESULT							ret  = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA					data = {};
	SC_DH_SHARE_DYNAMICGRAPHICSTATUS	sts  = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// 拡大図表示処理が実装されるまではe_DYNAMIC_GRAPHIC_NONを登録
#if 0
		sts.graphic_stat = status;
#else
		sts.graphic_stat = e_DYNAMIC_GRAPHIC_NON;
#endif
		data.dataId      = e_SC_DH_SHARE_DYNAMICGRAPHISTATUS;
		data.data        = (void*)&sts;

		// 常駐メモリデータを設定する
		ret = SC_DH_SetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}

	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

#if 0
/**
 * @brief 交差点拡大図登録
 * @param[in] bitmapinfoe：ビットマップ情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT  SC_MNG_SetDynamicGraphicBitmap(SMBITMAPINFO *bitmapinfo)
{
	E_SC_RESULT							ret  = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA					data = {};
	SC_DH_SHARE_DYNAMICGRAPHIBITMAP		sts  = {};
	UINT32								width;
	UINT32								height;

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		sts.bitmapinfo.bitmap     = bitmapinfo->bitmap;
		sts.bitmapinfo.bitmapsize = bitmapinfo->bitmapsize;
		sts.bitmapinfo.bufsize    = bitmapinfo->bufsize;

		data.dataId      = e_SC_DH_SHARE_DYNAMICGRAPHIBITMAP;
		data.data        = (void*)&sts;

		// 常駐メモリデータを設定する
		ret = SC_DH_SetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 交差点拡大図取得
 * @param[out] bitmapinfoe：ビットマップ情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT  SC_MNG_GetDynamicGraphicBitmap(SMBITMAPINFO	*bitmapinfo)
{
	E_SC_RESULT							ret  = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA					data = {};
	SC_DH_SHARE_DYNAMICGRAPHIBITMAP		sts  = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		data.dataId      = e_SC_DH_SHARE_DYNAMICGRAPHIBITMAP;
		data.data        = (void*)&sts;

		// 常駐メモリデータを設定する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}

		bitmapinfo->bitmap     = sts.bitmapinfo.bitmap;
		bitmapinfo->bitmapsize = sts.bitmapinfo.bitmapsize;
		bitmapinfo->bufsize    = sts.bitmapinfo.bufsize;


	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}
#endif
/**
 * @brief 交差点拡大図バッファ解放
 * @param[in] bitmap：ビットマップバッファ
 * @return
 */
void SC_MNG_DynamicGraphicBitmapFree(Char *bitmap){
	if(NULL != bitmap){
		free(bitmap);
	}
	return;
}


/**
 * @brief ユーザ定義アイコンのリソースと設定ファイルの格納パスを取得する
 * @param[out] pathIconDir  ユーザ定義アイコンのリソースパス
 * @param[out] pathIconInfo 設定ファイルのパス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetUDIResourcePath(Char *pathIconDir, Char *pathIconInfo)
{
	E_SC_RESULT				ret  = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA		data = {};
	SC_DH_SHARE_ICON_RESOURCE_PATH	path = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == pathIconDir) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[pathIconDir], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == pathIconInfo) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[pathIconInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体にを設定する
		data.dataId = e_SC_DH_SHARE_ICONRESOURCEPATH;
		data.data = (void*)&path;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
		strcpy(pathIconDir,  path.pathIconDir);
		strcpy(pathIconInfo, path.pathIconInfo);
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief ユーザ定義アイコンのリソースと設定ファイルの格納パスを設定する
 * @param[in] pathIconDir   ユーザ定義アイコンのリソースパス
 * @param[in] pathIconInfo  設定ファイルのパス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetUDIResourcePath(const Char *pathIconDir, const Char *pathIconInfo)
{
	E_SC_RESULT				ret  = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA		data = {};
	SC_DH_SHARE_ICON_RESOURCE_PATH	path = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == pathIconDir) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[pathIconDir], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == pathIconInfo) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[pathIconInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_ICONRESOURCEPATH;
		strcpy(path.pathIconDir,  pathIconDir);
		strcpy(path.pathIconInfo, pathIconInfo);
		data.data = (void*)&path;

		// 常駐メモリデータを設定する
		ret = SC_DH_SetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief ユーザ定義ダイナミックアイコンデータを取得する
 * @param[out] iconInfo アイコン情報
 * @param[out] iconNum  アイコン情報数
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetIconInfo(SMMAPDYNUDI *iconInfo, INT32 *iconNum)
{
	E_SC_RESULT				ret  = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA		data = {};
	SC_DH_SHARE_MAPDYNUDI	icon = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == iconNum) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[iconNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((0 != *iconNum) && (NULL == iconInfo)) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[iconInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体にを設定する
		data.dataId = e_SC_DH_SHARE_ICONINFO;
		data.data = (void*)&icon;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
		if (0 < icon.iconNum) {
			memcpy(iconInfo, icon.iconInfo, (sizeof(SMMAPDYNUDI) * icon.iconNum));
		}
		*iconNum = icon.iconNum;
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief ユーザ定義ダイナミックアイコンデータを設定する
 * @param[in] iconInfo  アイコン情報
 * @param[in] iconNum   アイコン情報数
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetIconInfo(const SMMAPDYNUDI *iconInfo, INT32 iconNum)
{
	E_SC_RESULT				ret  = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA		data = {};
	SC_DH_SHARE_MAPDYNUDI	icon = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if ((0 != iconNum) && (NULL == iconInfo)) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[iconInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (0 > iconNum) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[iconNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_ICONINFO;
		if(0 < iconNum) {
			memcpy(icon.iconInfo, iconInfo, (sizeof(SMMAPDYNUDI) * iconNum));
		}
		icon.iconNum = iconNum;
		data.data = (void*)&icon;

		// 常駐メモリデータを設定する
		ret = SC_DH_SetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}


/**
 * @brief ユーザ定義ダイナミックアイコンデータの表示/非表示を取得する
 * @param[out] dispInfo アイコン情報
 * @param[out] dispNum  アイコン情報数
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetDynamicUDIDisplay(Bool *dispInfo, INT32 *dispNum)
{
	E_SC_RESULT				ret  = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA		data = {};
	SC_DH_SHARE_MAPDYNUDI	icon = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == dispNum) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[dispNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((0 != *dispNum) && (NULL == dispInfo)) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[dispInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体にを設定する
		data.dataId = e_SC_DH_SHARE_ICONDISP;
		data.data = (void*)&icon;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
		if (0 < icon.iconNum) {
			memcpy(dispInfo, icon.iconDisp, (sizeof(Bool) * icon.iconNum));
		}
		*dispNum = icon.iconNum;
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief ユーザ定義ダイナミックアイコンデータの表示/非表示を設定する
 * @param[in] dispInfo  アイコン情報
 * @param[in] dispNum   アイコン情報数
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetDynamicUDIDisplay(const Bool *dispInfo, INT32 dispNum)
{
	E_SC_RESULT				ret  = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA		data = {};
	SC_DH_SHARE_MAPDYNUDI	icon = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if ((0 != dispNum) && (NULL == dispInfo)) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[dispInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (0 > dispNum) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[dispNum], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_ICONDISP;
		if(0 < dispNum) {
			memcpy(icon.iconDisp, dispInfo, (sizeof(Bool) * dispNum));
		}
		icon.iconNum = dispNum;
		data.data = (void*)&icon;

		// 常駐メモリデータを設定する
		ret = SC_DH_SetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief デモモードを取得する
 * @param[out] isDemoMode デモモード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetDemoMode(Bool *isDemoMode)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_DEMOMODE	demo = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == isDemoMode) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[isDemoMode], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_DEMOMODE;
		data.data = (void*)&demo;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_GetShareData error, " HERE);
			break;
		}
		*isDemoMode = demo.isDemoMode;
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief デモモードを設定する
 * @param[in] isDemoMode  デモモード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetDemoMode(Bool isDemoMode)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_DEMOMODE	demo = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_DEMOMODE;
		demo.isDemoMode = isDemoMode;
		data.data = (void*)&demo;

		// 常駐メモリデータを設定する
		ret = SC_DH_SetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief maps引数で指定した地図画面の地図を全景表示する。
 * @param[in] maps        表示操作対象地図
 * @param[in] overviewObj 全ルート表示
 * @param[in] rect        表示位置
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_OverviewMap(INT32 maps, INT32 overviewObj, SMRECT *rect)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	ret = SC_DRAW_OverviewMap(maps, overviewObj, rect);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, (Char*)"SC_DRAW_OverviewMap, " HERE);
	}

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 経路逸脱状態を取得する
 * @param[out] status   経路逸脱状態
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetDeviationStatus(E_DEVIATION_STATUS *status)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_DEVIATION_STATUS	sts = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == status) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[status], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_DEVIATIONSTATUS;
		data.data = (void*)&sts;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
		*status = sts.deviation_status;
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 経路逸脱状態を設定する
 * @param[in] status    経路逸脱状態
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetDeviationStatus(E_DEVIATION_STATUS status)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_DEVIATION_STATUS	sts = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_DEVIATIONSTATUS;
		sts.deviation_status = status;
		data.data = (void*)&sts;

		// 常駐メモリデータを設定する
		ret = SC_DH_SetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 経路コスト情報を取得する
 * @param[out] routeCostInfo    経路コスト情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetRouteCostInfo(SMRTCOSTINFO *routeCostInfo)
{
	E_SC_RESULT					ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA			data = {};
	SC_DH_SHARE_ROUTECOSTINFO	costInfo = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == routeCostInfo) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[routeCostInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_ROUTECOSTINFO;
		data.data = (void*)&costInfo;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
		memcpy(routeCostInfo, &costInfo.rtCostInfo, sizeof(SMRTCOSTINFO));
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 経路コスト情報を設定する
 * @param[in] routeCostInfo    経路コスト情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetRouteCostInfo(const SMRTCOSTINFO *routeCostInfo)
{
	E_SC_RESULT					ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA			data = {};
	SC_DH_SHARE_ROUTECOSTINFO	costInfo = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == routeCostInfo) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[routeCostInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_ROUTECOSTINFO;
		memcpy(&costInfo.rtCostInfo, routeCostInfo, sizeof(SMRTCOSTINFO));
		data.data = (void*)&costInfo;

		// 常駐メモリデータを設定する
		ret = SC_DH_SetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief ジャンルデータを取得する
 * @param[out] genreData    ジャンルデータ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetGenreData(SMGENREDATA *genreData)
{
	E_SC_RESULT					ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA			data = {};
	SC_DH_SHARE_GENREDATA		genre = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == genreData) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[genreData], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_GENREDATA;
		data.data = (void*)&genre;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
		memcpy(genreData, &genre.genreData, sizeof(SMGENREDATA));
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief ジャンルデータを設定する
 * @param[out] genreData    ジャンルデータ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetGenreData(const SMGENREDATA *genreData)
{
	E_SC_RESULT					ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA			data = {};
	SC_DH_SHARE_GENREDATA		genre = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == genreData) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[genreData], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_GENREDATA;
		memcpy(&genre.genreData, genreData, sizeof(SMGENREDATA));
		data.data = (void*)&genre;

		// 常駐メモリデータを設定する
		ret = SC_DH_SetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief セマフォIDを取得する
 * @return セマフォID
 */
SC_SEMAPHORE *SC_MNG_GetSemId()
{
#ifdef __SMS_APPLE__
	return (syncAPISem);
#else
	return (&syncAPISem);
#endif /* __SMS_APPLE__ */
}

/**
 * @brief 運転特性診断セマフォIDを取得する
 * @return セマフォID
 */
SC_SEMAPHORE *SC_MNG_GetSDSemId()
{
#ifdef __SMS_APPLE__
	return (syncSDSem);
#else
	return (&syncSDSem);
#endif /* __SMS_APPLE__ */
}

/**
 * @brief API処理結果を設定する
 * @param[in] result    処理結果
 */
void SC_MNG_SetResult(E_SC_RESULT result)
{
	apiResult = result;
}

/**
 * @brief 運転特性診断処理結果を設定する
 * @param[in] result    処理結果
 */
void SC_MNG_SetSDResult(E_SC_RESULT result)
{
	sdResult = result;
}


/**
 * @brief トリップIDを設定する
 * @param[in] tripId    トリップID
 */
void SC_MNG_SetTripId(Char *tripId)
{
	strcpy(sdTripId, tripId);
}

/**
 * @brief 地図データ格納ディレクトリのフルパスを取得する
 * @return 地図データ格納ディレクトリのフルパス
 */
const Char *SC_MNG_GetMapDirPath()
{
	return (mapDirPath);
}

/**
 * @brief 設定ファイルディレクトリ(Config)のフルパスを取得する
 * @return 設定ファイルディレクトリ(Config)のフルパス
 */
const Char *SC_MNG_GetConfDirPath()
{
	return (configRootPath);
}

/**
 * @brief APのルートディレクトリ(/jp.co.hitachi.smsfv.aa/Data)を取得する
 * @return APのルートディレクトリ(/jp.co.hitachi.smsfv.aa/Data)のフルパス
 */
const Char *SC_MNG_GetApRootDirPath()
{
	return ((const Char*)apRootDirPath);
}


//-----------------------------------
// ローカル関数
//-----------------------------------
/**
 * @brief メモリ管理の初期化処理を行う
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_MemInitialize()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// メモリ管理初期化
		// 動的確保メモリ
		ret = SC_MEM_Initialize(SC_MEM_SIZE_DYNAMIC, e_MEM_TYPE_DYNAMIC);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MEM_Initialize(DYNAMIC) error, " HERE);
			break;
		}
		// 常駐メモリ
		ret = SC_MEM_Initialize(SC_MEM_SIZE_SHARE, e_MEM_TYPE_SHARE);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MEM_Initialize(SHARE) error, " HERE);
			break;
		}
		// キャッシュメモリ
		ret = SC_MEM_Initialize(SC_MEM_SIZE_CASH, e_MEM_TYPE_CASH);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MEM_Initialize(CASH) error, " HERE);
			break;
		}
		// 探索メモリ
		ret = SC_MEM_Initialize(SC_MEM_SIZE_ROUTEPLAN, e_MEM_TYPE_ROUTEPLAN);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MEM_Initialize(ROUTEPLAN) error, " HERE);
			break;
		}
		// 候補経路メモリ
		ret = SC_MEM_Initialize(SC_MEM_SIZE_ROUTECAND, e_MEM_TYPE_ROUTECAND);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MEM_Initialize(ROUTEPLAN) error, " HERE);
			break;
		}
		// 経路メモリ
		ret = SC_MEM_Initialize(SC_MEM_SIZE_ROUTEMNG, e_MEM_TYPE_ROUTEMNG);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MEM_Initialize(ROUTEMNG) error, " HERE);
			break;
		}
		// 誘導テーブルメモリ
		ret = SC_MEM_Initialize(SC_MEM_SIZE_GUIDETBL, e_MEM_TYPE_GUIDETBL);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MEM_Initialize(GUIDETBL) error, " HERE);
			break;
		}
		// ジャンルデータルメモリ
		ret = SC_MEM_Initialize(SC_MEM_SIZE_GENREDATA, e_MEM_TYPE_GENREDATA);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MEM_Initialize(GENREDATA) error, " HERE);
			break;
		}
		// 運転特性診断メモリ
		ret = SC_MEM_Initialize(SC_MEM_SIZE_SD, e_MEM_TYPE_SD);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MEM_Initialize(SD) error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief メモリ管理の終了化処理を行う
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_MemFinalize()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	// メモリ管理終了化
	ret = SC_MEM_Finalize(e_MEM_TYPE_DYNAMIC);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MEM_Finalize(DYNAMIC) error, " HERE);
	}

	// 常駐メモリ
	ret2 = SC_MEM_Finalize(e_MEM_TYPE_SHARE);
	if (e_SC_RESULT_SUCCESS != ret2) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MEM_Finalize(SHARE) error, " HERE);
		if (e_SC_RESULT_SUCCESS == ret) {
			ret = ret2;
		}
	}

	// キャッシュメモリ
	ret2 = SC_MEM_Finalize(e_MEM_TYPE_CASH);
	if (e_SC_RESULT_SUCCESS != ret2) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MEM_Finalize(CASH) error, " HERE);
		if (e_SC_RESULT_SUCCESS == ret) {
			ret = ret2;
		}
	}

	// 探索メモリ
	ret2 = SC_MEM_Finalize(e_MEM_TYPE_ROUTEPLAN);
	if (e_SC_RESULT_SUCCESS != ret2) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MEM_Finalize(ROUTEPLAN) error, " HERE);
		if (e_SC_RESULT_SUCCESS == ret) {
			ret = ret2;
		}
	}

	// 候補経路メモリ
	ret2 = SC_MEM_Finalize(e_MEM_TYPE_ROUTECAND);
	if (e_SC_RESULT_SUCCESS != ret2) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MEM_Finalize(ROUTECAND) error, " HERE);
		if (e_SC_RESULT_SUCCESS == ret) {
			ret = ret2;
		}
	}

	// 経路メモリ
	ret2 = SC_MEM_Finalize(e_MEM_TYPE_ROUTEMNG);
	if (e_SC_RESULT_SUCCESS != ret2) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MEM_Finalize(ROUTEMNG) error, " HERE);
		if (e_SC_RESULT_SUCCESS == ret) {
			ret = ret2;
		}
	}

	// 誘導テーブルメモリ
	ret2 = SC_MEM_Finalize(e_MEM_TYPE_GUIDETBL);
	if (e_SC_RESULT_SUCCESS != ret2) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MEM_Finalize(GUIDETBL) error, " HERE);
		if (e_SC_RESULT_SUCCESS == ret) {
			ret = ret2;
		}
	}

	// ジャンルデータメモリ
	ret2 = SC_MEM_Finalize(e_MEM_TYPE_GENREDATA);
	if (e_SC_RESULT_SUCCESS != ret2) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MEM_Finalize(GENREDATA) error, " HERE);
		if (e_SC_RESULT_SUCCESS == ret) {
			ret = ret2;
		}
	}

	// 運転特性診断メモリ
	ret2 = SC_MEM_Finalize(e_MEM_TYPE_SD);
	if (e_SC_RESULT_SUCCESS != ret2) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MEM_Finalize(SD) error, " HERE);
		if (e_SC_RESULT_SUCCESS == ret) {
			ret = ret2;
		}
	}

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief メッセージキューを生成する
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_CreateMsgQueue()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	num = 0;

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	for (num = 0; num < SC_QUEUE_NUM; num++) {
		// メッセージキュー生成
		if (0 != pthread_msq_create(&msgQueue[num].msgQueue, msgQueue[num].msgQueueSize)) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "pthread_msq_create error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}
	}

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief メッセージキューを破壊する
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_DestroyMsgQueue()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	num = 0;

	for (num = 0; num < SC_QUEUE_NUM; num++) {
		// メッセージキュー破壊
		if (0 != pthread_msq_destroy(&msgQueue[num].msgQueue)) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "pthread_msq_destroy error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}
	}

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 各スレッドを初期化する
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SubFuncInitialize()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	num = 0;

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	for (num = 0; num < SC_THREAD_NUM; num++) {
		if (NULL == initFinalFuncTbl[num].Initialize) {
			// NULLの場合はInitialize関数を呼び出さない
			continue;
		}
		ret = initFinalFuncTbl[num].Initialize();
		if (e_SC_RESULT_SUCCESS != ret) {
			break;
		}
	}

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 各機能を終了化する
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SubFuncFinalize()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	num = 0;

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	for (num = 0; num < SC_THREAD_NUM; num++) {
		if (NULL == initFinalFuncTbl[num].Finalize) {
			// NULLの場合はFinalize関数を呼び出さない
			continue;
		}
		ret = initFinalFuncTbl[num].Finalize();
		if (e_SC_RESULT_SUCCESS != ret) {
			break;
		}
	}

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief スレッドを生成する
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_ThreadInitialize()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	INT32	num = 0;

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// スレッド初期化
		ret = SC_Thread_Initialize();
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_Thread_Initialize error, " HERE);
			break;
		}

		for (num = 0; num < SC_THREAD_NUM; num++) {
			if (NULL == threadInfTbl[num].Main) {
				continue;
			}
			// スレッド生成
			ret = SC_Thread_Create(&threadInfTbl[num].threadId, threadInfTbl[num].Main, NULL);
			if (e_SC_RESULT_SUCCESS != ret) {
				SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_Thread_Create error, " HERE);
				break;
			}
		}
		if (e_SC_RESULT_SUCCESS != ret) {
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief スレッドを終了する
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_ThreadFinalize()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	INT32	num = 0;
	pthread_msq_msg_t msg = {};

	//SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);
	LOG_PRINT_START((char*)SC_TAG_CORE);

	// エラーでも処理は継続する

	// スレッド終了フラグ設定
	SC_Thread_SetIsFinish(true);

	// 終了メッセージ送信
	msg.data[0] = e_SC_MSGID_EVT_FINISH;

	// 送信メッセージをログ出力
	SC_LOG_DebugPrint(SC_TAG_CORE,
			"sendMsg=0x%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x, " HERE,
			msg.data[0],  msg.data[1],  msg.data[2],  msg.data[3],  msg.data[4],
			msg.data[5],  msg.data[6],  msg.data[7],  msg.data[8],  msg.data[9]);

	for (num = 0; num < SC_THREAD_NUM; num++) {
		if (PTHREAD_MSQ_OK != pthread_msq_msg_send(SC_CORE_MSQID(num), &msg, SC_CORE_MSQID_FM)) {
			//SC_LOG_ErrorPrint(SC_TAG_CORE, "pthread_msq_msg_send error, " HERE);
			LOG_PRINT_ERROR((char*)SC_TAG_CORE, "pthread_msq_msg_send error, " HERE);
			if (e_SC_RESULT_SUCCESS == ret) {
				ret = e_SC_RESULT_FAIL;
			}
		}
	}

	// スレッド終了待ち
	for (num = 0; num < SC_THREAD_NUM; num++) {
		if (NULL == threadInfTbl[num].Main) {
			continue;
		}
		ret2 = SC_Thread_Join(threadInfTbl[num].threadId);
		if (e_SC_RESULT_SUCCESS != ret2) {
			//SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_Thread_Join error, " HERE);
			LOG_PRINT_ERROR((char*)SC_TAG_CORE, "SC_Thread_Join error, " HERE);
			if (e_SC_RESULT_SUCCESS == ret) {
				ret = ret2;
			}
		}
	}

	// スレッド終了化
	ret2 = SC_Thread_Finalize();
	if (e_SC_RESULT_SUCCESS != ret2) {
		//SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_Thread_Initialize error, " HERE);
		LOG_PRINT_ERROR((char*)SC_TAG_CORE, "SC_Thread_Initialize error, " HERE);
		if (e_SC_RESULT_SUCCESS == ret) {
			ret = ret2;
		}
	}

	//SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);
	LOG_PRINT_END((char*)SC_TAG_CORE);

	return (ret);
}

// パス初期化
static void SC_MNG_InitPath(const Char *rootDir, const Char *confDir, const Char*mapDir)
{
	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	// jp.co.hitachi.smsfv.aa/Dataディレクトリパス
	strcpy((char*)apRootDirPath, (char*)rootDir);
	if ('/' == apRootDirPath[strlen((char*)apRootDirPath) - 1]) {
		apRootDirPath[strlen((char*)apRootDirPath) - 1] = '\0';
	}

	// Configディレクトリパス
	strcpy((char*)configRootPath, (char*)confDir);
	if ('/' == configRootPath[strlen((char*)configRootPath) - 1]) {
		configRootPath[strlen((char*)configRootPath) - 1] = '\0';
	}

	// jp.co.hitachi.smsfv.aa/Mapディレクトリパス
	strcpy((char*)mapDirPath, (char*)mapDir);
	if ('/' != mapDirPath[strlen((char*)mapDirPath) - 1]) {
		strcat(mapDirPath, "/");
	}
	strcat(mapDirPath, SC_DB_PATH);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);
}



// 交差点拡大図表示確認用サンプル関数
// 本関数は、交差点拡大図の領域にビットマップ画像が表示されるか確認
// の為のサンプルであり、実際に交差点拡大図を描画するには別処理が必要

E_SC_RESULT  RG_NMG_GetDynamicGraphicBitmap(SMBITMAPINFO	*bitmapinfo)
{
	//E_SC_RESULT							ret = e_SC_RESULT_SUCCESS;
	Char								*buffer;
	INT32								size;
	INT32								readsize;
	INT32								width;
	INT32								height;
	//INT32								ilp;
	//Char								tmp;

	static char filename[] = "/mnt/sdcard/AndroidNaviSys/MAPPY/Log/sample.bmp";
	FILE	*fp;

	// 拡大図サイズ取得
	SC_MNG_GetDynamicGraphicSize(&width,&height);
	size = (width*height*2);

	// 描画領域未設定
	if(0 == size){
		return (e_SC_RESULT_FAIL);
	}

	// 拡大図展開用バッファ確保
	buffer = (char *)malloc(size);
	if(NULL == buffer){
		return (e_SC_RESULT_FAIL);
	}

	// 以下サンプルとしてビットマップをファイルから取得
	fp = fopen((char *)filename,"rb");
	if(fp == NULL){
		SC_LOG_ErrorPrint(SC_TAG_CORE, "%s file open failed" HERE,filename);
		return (e_SC_RESULT_FAIL);
	}

	// ビットマップバッファのサイズ取得
	readsize = (INT32)fread((char *)buffer, (size_t)sizeof(char), (size_t)(size), (FILE*)fp);

	if (0 != fclose(fp)) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "%s file close failed" HERE, filename);
		return (e_SC_RESULT_FAIL);
	}
	// 共有メモリ登録用テーブルに情報設定
	bitmapinfo->bitmap     = buffer;
	bitmapinfo->bitmapsize = readsize;
	bitmapinfo->bufsize    = size;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief プローブ情報の 車両タイプを設定 する。
 * @param[in] vehicleType            1：自動車、2：バイク(単車)、3：自転車、4：徒歩
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetVehicleType(INT32 vehicleType)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_VEHICLETYPE	vtpe = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	// 常駐メモリデータ入出力構造体に値を設定する
	data.dataId = e_SC_DH_SHARE_VEHICLETYPE;
	vtpe.vehicleType = vehicleType;
	data.data = (void*)&vtpe;

	// 常駐メモリデータを設定する
	ret = SC_DH_SetShareData(&data);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
	}

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief プローブ情報の 車両タイプを取得 する。
 * @param[out] vehicleType            1：自動車、2：バイク(単車)、3：自転車、4：徒歩
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetVehicleType(INT32 *vehicleType)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_VEHICLETYPE	vtpe = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == vehicleType) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[vehicleType], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_VEHICLETYPE;
		data.data = (void*)&vtpe;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_GetShareData error, " HERE);
			break;
		}
		*vehicleType = vtpe.vehicleType;
	} while(0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);
	return (ret);
}

/**
 * @brief プローブ情報の 保存フラグを ON/OFF する。
 * @param[in] flag            true : フラグON、false : フラグOFF
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetSaveTracksFlag(Bool isSaveTracks )
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_SAVETRACKS	savetracks = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	// 常駐メモリデータ入出力構造体に値を設定する
	data.dataId = e_SC_DH_SHARE_ISSAVETRACKS;
	savetracks.isSaveTracks = isSaveTracks;
	data.data = (void*)&savetracks;

	// 常駐メモリデータを設定する
	ret = SC_DH_SetShareData(&data);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
	}

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief プローブ情報の 保存フラグを 取得する。
 * @param[out] flag            true : フラグON、false : フラグOFF
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetSaveTracksFlag(Bool *flag)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_SAVETRACKS	savetracks = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == flag) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[flag], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_ISSAVETRACKS;
		data.data = (void*)&savetracks;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_GetShareData error, " HERE);
			break;
		}
		*flag = savetracks.isSaveTracks;
	} while(0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);
	return (ret);
}

/**
 * @brief デバッグ情報表示フラグを ON/OFF する
 * @param[in] flag            true : フラグON、false : フラグOFF
 */
E_SC_RESULT SC_MNG_SetDebugInfoOnSurface(Bool isOnSurface)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_ONSURFACE	onsurface = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	// 常駐メモリデータ入出力構造体に値を設定する
	data.dataId = e_SC_DH_SHARE_ISONSURFACE;
	onsurface.isOnSurface = isOnSurface;
	data.data = (void*)&onsurface;

	// 常駐メモリデータを設定する
	ret = SC_DH_SetShareData(&data);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
	}

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief デバッグ情報表示フラグを 取得する。
 * @param[out] flag            true : フラグON、false : フラグOFF
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetDebugInfoOnSurface(Bool *flag)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_ONSURFACE	onsurface = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == flag) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[flag], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_ISONSURFACE;
		data.data = (void*)&onsurface;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_GetShareData error, " HERE);
			break;
		}
		*flag = onsurface.isOnSurface;
	} while(0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);
	return (ret);
}


/**
 * @brief 位置情報共有フラグを ON/OFF する
 * @param[in] isEcho            true : フラグON、false : フラグOFF
 */
E_SC_RESULT SC_MNG_SetEchoFlag(Bool isEcho)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_ECHO	echo = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	// 常駐メモリデータ入出力構造体に値を設定する
	data.dataId = e_SC_DH_SHARE_ISECHO;
	echo.isEcho = isEcho;
	data.data = (void*)&echo;

	// 常駐メモリデータを設定する
	ret = SC_DH_SetShareData(&data);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
	}

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 位置情報共有フラグを 取得する。
 * @param[out] isEcho            true : フラグON、false : フラグOFF
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetEchoFlag(Bool *isEcho)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_ECHO	echo = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == isEcho) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[isEcho], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_ISECHO;
		data.data = (void*)&echo;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_GetShareData error, " HERE);
			break;
		}
		*isEcho = echo.isEcho;
	} while(0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);
	return (ret);
}

/**
 * @brief 地図ズームモードを取得する
 * @param[out] zoomMode ズームモード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetZoomMode(Bool *zoomMode)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_ZOOMMODE	mode = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == zoomMode) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[scrollMode], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_ZOOMMODE;
		data.data = (void*)&mode;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_GetShareData error, " HERE);
			break;
		}
		*zoomMode = mode.mode;
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 地図ズームモードを設定する
 * @param[in] zoomMode  ズームモード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetZoomMode(Bool zoomMode)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA	data = {};
	SC_DH_SHARE_SCROLLMODE	mode = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);


	// 常駐メモリデータ入出力構造体に値を設定する
	data.dataId = e_SC_DH_SHARE_ZOOMMODE;
	mode.mode = zoomMode;
	data.data = (void*)&mode;

	// 常駐メモリデータを設定する
	ret = SC_DH_SetShareData(&data);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
	}

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief リージョンを設定する
 * @param[in] region  リージョン
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetRegion(INT32 aRegion) {

	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA data = {};
	SC_DH_SHARE_REGION region = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	// 常駐メモリデータ入出力構造体に値を設定する
	data.dataId = e_SC_DH_SHARE_REGION;
	region.region = aRegion;
	data.data = (void*) &region;

	// 常駐メモリデータを設定する
	ret = SC_DH_SetShareData(&data);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, (%x)" HERE, ret);
	}

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief リージョンを取得する
 * @param[out] aRegion リージョン
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetRegion(INT32* aRegion) {

	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA data = {};
	SC_DH_SHARE_REGION region = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == aRegion) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[scrollMode], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_REGION;
		data.data = (void*) &region;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_GetShareData error, (%x)" HERE, ret);
			break;
		}
		*aRegion = region.region;
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 言語を設定する
 * @param[in] aLanguage 言語
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetLanguage(INT32 aLanguage) {

	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA data = {};
	SC_DH_SHARE_LANGUAGE language = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	// 常駐メモリデータ入出力構造体に値を設定する
	data.dataId = e_SC_DH_SHARE_LANGUAGE;
	language.language = aLanguage;
	data.data = (void*) &language;

	// 常駐メモリデータを設定する
	ret = SC_DH_SetShareData(&data);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, (%x)" HERE, ret);
	}

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 言語を取得する
 * @param[out] aLanguage 言語
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetLanguage(INT32 *aLanguage) {

	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA data = {};
	SC_DH_SHARE_LANGUAGE language = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == aLanguage) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[scrollMode], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_LANGUAGE;
		data.data = (void*) &language;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_GetShareData error, (%x)" HERE, ret);
			break;
		}
		*aLanguage = language.language;
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 経路バックアップ処理
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_RouteBackup() {
	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	ret = SC_DH_RouteBackup();
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_RouteBackup error, " HERE);
	}

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);
	return (ret);
}

/**
 * @brief 経路バックアップ削除
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_RouteBackupDelete() {
	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	ret = SC_DH_RouteBackupDelete();
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_RouteBackupDelete error, " HERE);
	}

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);
	return (ret);
}

/**
 * @brief 経路バックアップ取得
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetRouteBackup(SMROUTEBACKUP* backup) {
	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	ret = SC_DH_GetRouteBackup(backup);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_GetRouteBackup error, " HERE);
	}

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);
	return (ret);
}

/**
 * @brief リアルタイム案内情報数を取得する
 * @param[out] guideData    リアルタイム案内情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetGuideDataVol(UINT16 *listVol, UINT16 reqNo)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// 誘導情報取得
		ret = RG_API_GetGuideDataVol(listVol, reqNo);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MNG_GetGuideListVol error, " HERE);
		}

	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief リアルタイム案内情報を取得する
 * @param[out] guideData    リアルタイム案内情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetGuideData(SMGUIDEDATA *guideData, UINT16 reqNo)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == guideData) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[guideData], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 誘導情報取得
		ret = RG_API_GetGuideData(guideData, reqNo);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_MNG_GetGuideList error, " HERE);
		}

	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 運転特性診断を開始する
 * @param[out] tripId   トリップID
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_StartDrivingDiagnosing(Char *tripId)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	pthread_msq_msg_t msg = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == tripId) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[tripId], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 初期化
		sdTripId[0] = EOS;

		msg.data[SC_MSG_MSG_ID] = e_SC_MSGID_REQ_SDM_START;

		// 送信メッセージをログ出力
		SC_LOG_DebugPrint(SC_TAG_CORE,
				"sendMsg=0x%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x, " HERE,
				msg.data[0],  msg.data[1],  msg.data[2],  msg.data[3],  msg.data[4],
				msg.data[5],  msg.data[6],  msg.data[7],  msg.data[8],  msg.data[9]);

		// 運転特性診断メインスレッドに運転特性診断開始要求メッセージ送信
		if (PTHREAD_MSQ_OK != pthread_msq_msg_send(SC_CORE_MSQID_SDM, &msg, SC_CORE_MSQID_FM)) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "pthread_msq_msg_send error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// セマフォロック
#ifdef __SMS_APPLE__
		ret = SC_LockSemaphore(syncSDSem);
#else
		ret = SC_LockSemaphore(&syncSDSem);
#endif /* __SMS_APPLE__ */
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_LockSemaphore error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 処理結果取得
		if (e_SC_RESULT_SUCCESS != sdResult) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "start error, " HERE);
			ret = e_SC_RESULT_FAIL;
		}

		// トリップIDコピー
		strcpy(tripId, sdTripId);
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 運転特性診断を停止する
 * @param[out] tripId   トリップID
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_StopDrivingDiagnosing(Char *tripId)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	pthread_msq_msg_t msg = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == tripId) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[tripId], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		msg.data[SC_MSG_MSG_ID] = e_SC_MSGID_REQ_SDM_STOP;

		// 送信メッセージをログ出力
		SC_LOG_DebugPrint(SC_TAG_CORE,
				"sendMsg=0x%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x, " HERE,
				msg.data[0],  msg.data[1],  msg.data[2],  msg.data[3],  msg.data[4],
				msg.data[5],  msg.data[6],  msg.data[7],  msg.data[8],  msg.data[9]);

		// 運転特性診断メインスレッドに運転特性診断停止要求メッセージ送信
		if (PTHREAD_MSQ_OK != pthread_msq_msg_send(SC_CORE_MSQID_SDM, &msg, SC_CORE_MSQID_FM)) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "pthread_msq_msg_send error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// セマフォロック
#ifdef __SMS_APPLE__
		ret = SC_LockSemaphore(syncSDSem);
#else
		ret = SC_LockSemaphore(&syncSDSem);
#endif /* __SMS_APPLE__ */
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_LockSemaphore error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 処理結果取得
		if (e_SC_RESULT_SUCCESS != sdResult) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "stop error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// トリップIDコピー
		strcpy(tripId, sdTripId);
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 運転特性診断トリップID取得
 * @param[in] tripId    トリップID
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetDrivingDiagnosingTripID(Char *tripId)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	// パラメータチェック
	if (NULL == tripId) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[tripId], " HERE);
		ret = e_SC_RESULT_BADPARAM;
	} else {
		// トリップIDコピー
		strcpy(tripId, sdTripId);
	}

	return (ret);
}

/**
 * @brief 地域クラスコード取得
 * @param[in] lat           緯度
 * @param[in] lon           経度
 * @param[out] pAreaClsCode 地域クラスコード
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetAreaClsCode(DOUBLE lat, DOUBLE lon, SMAREACLSCODE *pAreaClsCode)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_DHC_CASH_RESULT result = e_DHC_RESULT_CASH_SUCCESS;
	T_DHC_AREA_CLS_CODE dhcAareaClsCode = {};
	UINT32 parcelID;
	DOUBLE x = 0.0;
	DOUBLE y = 0.0;

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// 緯度経度からパーセルID,X,Yに変換
		MESHC_ChgLatLonToParcelID(lat, lon, 1, &parcelID, &x, &y);

		// パーセルID,X,Yから地域クラスコード取得
		dhcAareaClsCode.parcelId = parcelID;
		dhcAareaClsCode.x = (UINT16)x;
		dhcAareaClsCode.y = (UINT16)y;

		// 地域クラスコード取得
		result = SC_DHC_GetAreaClsCode(&dhcAareaClsCode);
		if (e_DHC_RESULT_CASH_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DHC_GetAreaClsCode error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		memcpy(pAreaClsCode, &dhcAareaClsCode.areaClsCode, sizeof(SMAREACLSCODE));

	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief マッピングアラート情報設定
 * @param[in] mappingAlert   マッピングアラート情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetMappingAlert(SMMAPPINGALERT *mappingAlert)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA			data = {};
	SC_DH_SHARE_MAPPINGALERT	alert = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == mappingAlert) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[mappingAlert], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_MAPPINGALERT;
		memcpy(&alert.alert, mappingAlert, sizeof(SMMAPPINGALERT));
		data.data = (void*)&alert;

		// 常駐メモリデータを設定する
		ret = SC_DH_SetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief マッピングアラート情報取得
 * @param[out] mappingAlert   マッピングアラート情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetMappingAlert(SMMAPPINGALERT *mappingAlert)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA			data = {};
	SC_DH_SHARE_MAPPINGALERT	alert = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == mappingAlert) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[mappingAlert], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_MAPPINGALERT;
		data.data = (void*)&alert;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_GetShareData error, " HERE);
			break;
		}
		memcpy(mappingAlert, &alert.alert, sizeof(SMMAPPINGALERT));
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 交通情報設定
 * @param[in] trfInfo   交通情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_SetTrafficInfo(SMTRAFFIC *trfInfo)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA		data = {};
	SC_DH_SHARE_TRAFFIC		traffic = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == trfInfo) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[trfInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_TRAFFIC;
		memcpy(&traffic.traffic, trfInfo, sizeof(SMTRAFFIC));
		data.data = (void*)&traffic;

		// 常駐メモリデータを設定する
		ret = SC_DH_SetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_SetShareData error, " HERE);
			break;
		}

		// 更新タイマ
		if (trfInfo->disp) {
			// 自動更新タイマ開始
			SC_TRT_SendStratTimerMsg(SC_CORE_MSQID_FM, 1);
		} else {
			// 自動更新タイマ停止
			SC_TRT_StopTimer();
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 交通情報取得
 * @param[out] trfInfo   交通情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_GetTrafficInfo(SMTRAFFIC *trfInfo)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DH_SHARE_DATA		data = {};
	SC_DH_SHARE_TRAFFIC		traffic = {};

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == trfInfo) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "param error[trfInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリデータ入出力構造体に値を設定する
		data.dataId = e_SC_DH_SHARE_TRAFFIC;
		data.data = (void*)&traffic;

		// 常駐メモリデータを取得する
		ret = SC_DH_GetShareData(&data);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_CORE, "SC_DH_GetShareData error, " HERE);
			break;
		}
		memcpy(trfInfo, &traffic.traffic, sizeof(SMTRAFFIC));
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 交通情報を更新する
 * @param[in] pos  更新位置
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_MNG_RefreshTraffic(INT32 mode)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_START);

	// メッセージ送信
	if (TRAFFIC_POS_CAR == mode) {
		SC_TR_SendCarposUpdateMsg();
	} else if (TRAFFIC_POS_SCROLL == mode) {
		SC_TR_SendScrollUpdateMsg();
	} else {
		// NOP
	}

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);

	return (ret);
}
