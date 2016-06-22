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
 * SMCoreDataHandler.c
 *
 *  Created on: 2015/11/07
 *      Author:
 */

#include "SMCoreCMNInternal.h"

// 常駐メモリのデータ識別IDチェック
#define	SC_DH_CHECK_SHAREDATA_ID(id)	((e_SC_DH_SHARE_SCALELEVEL <= id) && (e_SC_DH_SHARE_DATA_ID_END > id))

//-----------------------------------
// 構造体定義
//-----------------------------------
// 常駐メモリデータ情報構造体
typedef struct _SC_DH_SHARE_DATA_INF {
	INT32	scaleLevel;		// 地図縮尺
	INT32	dispMode;		// 地図表示モード
	INT32	rpPOption;		// 探索条件
} SC_DH_SHARE_DATA_INF;

// 関数ポインタ
typedef E_SC_RESULT (*SC_DH_Func)(void *param);
typedef struct _SC_SHARE_FUNC_TBL {
	SC_DH_Func	getFunc;
	SC_DH_Func	setFunc;
} SC_SHARE_FUNC_TBL;

//-----------------------------------
// テーブル定義
//-----------------------------------
#define	SC_SHARE_FUNC_TBL_NUM		e_SC_DH_SHARE_DATA_ID_END		// 常駐メモリのデータ識別ID数
// 設定・取得関数ポインタテーブル
const static SC_SHARE_FUNC_TBL funcTbl[SC_SHARE_FUNC_TBL_NUM] = {
	{(SC_DH_Func)SC_SHARE_GetScaleLevel,		(SC_DH_Func)SC_SHARE_SetScaleLevel},		// 地図表示縮尺(スケール)
	{(SC_DH_Func)SC_SHARE_GetBigTextAttr,		(SC_DH_Func)SC_SHARE_SetBigTextAttr},		// 地図上文言拡大表示
	{(SC_DH_Func)SC_SHARE_GetBigIconAttr,		(SC_DH_Func)SC_SHARE_SetBigIconAttr},		// 地図上表示アイコン拡大表示
	{(SC_DH_Func)SC_SHARE_GetLandmark,			(SC_DH_Func)SC_SHARE_SetLandmark},			// ランドマーク表示設定
	{(SC_DH_Func)SC_SHARE_GetLandmarkAttr,		(SC_DH_Func)SC_SHARE_SetLandmarkAttr},		// ランドマーク表示属性設定
	{(SC_DH_Func)SC_SHARE_GetDispMode,			(SC_DH_Func)SC_SHARE_SetDispMode},			// 地図表示モード
	{(SC_DH_Func)SC_SHARE_GetDriverMode,		(SC_DH_Func)SC_SHARE_SetDriverMode},		// 地図ドライバモード
	{(SC_DH_Func)SC_SHARE_GetShowCursor,		(SC_DH_Func)SC_SHARE_SetShowCursor},		// 地図中心カーソルの表示ステータス
	{(SC_DH_Func)SC_SHARE_GetCarState,			(SC_DH_Func)SC_SHARE_SetCarState},			// 車両状態情報
	{(SC_DH_Func)SC_SHARE_GetScrollMode,		(SC_DH_Func)SC_SHARE_SetScrollMode},		// 地図スクロールモード
	{(SC_DH_Func)SC_SHARE_GetMoveMapDir,		(SC_DH_Func)SC_SHARE_SetMoveMapDir},		// スクロール長
	{(SC_DH_Func)SC_SHARE_GetZoomStepRate,		(SC_DH_Func)SC_SHARE_SetZoomStepRate},		// フリーズーム時の地図拡大比例
	{(SC_DH_Func)SC_SHARE_GetMapCursorCoord,	(SC_DH_Func)SC_SHARE_SetMapCursorCoord},	// 地図中心の地理座標
	{(SC_DH_Func)SC_SHARE_GetMapViewport,		(SC_DH_Func)SC_SHARE_SetMapViewport},		// 地図のビューポート
	{(SC_DH_Func)SC_SHARE_GetMapRotate,			(SC_DH_Func)SC_SHARE_SetMapRotate},			// 地図の回転角度
	{(SC_DH_Func)SC_SHARE_GetGeoDistance,		(SC_DH_Func)SC_SHARE_SetGeoDistance},		// 地図のスケール
	{(SC_DH_Func)SC_SHARE_GetResolution,		(SC_DH_Func)SC_SHARE_SetResolution},		// 解像度
	{(SC_DH_Func)SC_SHARE_GetAllRPPlace,		(SC_DH_Func)SC_SHARE_SetAllRPPlace},		// 地点情報（出発地、経由地、目的地）
	{(SC_DH_Func)SC_SHARE_GetExistRoute,		(SC_DH_Func)SC_SHARE_SetExistRoute},		// 探索結果の有無
	{(SC_DH_Func)SC_SHARE_GetPlanning,			(SC_DH_Func)SC_SHARE_SetPlanning},			// 探索中かどうか
	{(SC_DH_Func)SC_SHARE_GetRPOption,			(SC_DH_Func)SC_SHARE_SetRPOption},			// 探索条件
	{(SC_DH_Func)SC_SHARE_GetRPTip,				(SC_DH_Func)SC_SHARE_SetRPTip},				// 探索のエラー情報
	{(SC_DH_Func)NULL,							(SC_DH_Func)SC_SHARE_DeleteRouteResult},	// 探索結果削除
	{(SC_DH_Func)SC_SHARE_GetSimulate,			(SC_DH_Func)SC_SHARE_SetSimulate},			// シミュレータ
	{(SC_DH_Func)SC_SHARE_GetSimulateSpeed,		(SC_DH_Func)SC_SHARE_SetSimulateSpeed},		// シミュレータ速度
	{(SC_DH_Func)SC_SHARE_GetSimulateState,		(SC_DH_Func)SC_SHARE_SetSimulateState},		// シミュレータ状態
	{(SC_DH_Func)SC_SHARE_GetReplan,			(SC_DH_Func)SC_SHARE_SetReplan},			// リルート条件閾値
	{(SC_DH_Func)SC_SHARE_GetRealTimeInfo,		(SC_DH_Func)SC_SHARE_SetRealTimeInfo},		// リアルタイム案内情報
	{(SC_DH_Func)SC_SHARE_GetGuideStatus,		(SC_DH_Func)SC_SHARE_SetGuideStatus},		// 誘導状態
	{(SC_DH_Func)SC_SHARE_GetVoiceTTS,			(SC_DH_Func)SC_SHARE_SetVoiceTTS},			// 音声ＴＴＳ情報
	{(SC_DH_Func)SC_SHARE_GetTurnListInfo,		(SC_DH_Func)SC_SHARE_SetTurnListInfo},		// ターンリスト情報
	{(SC_DH_Func)SC_SHARE_GetRouteLength,		(SC_DH_Func)SC_SHARE_SetRouteLength},		// 経路総距離
	{(SC_DH_Func)SC_SHARE_GetRouteAveTimne,		(SC_DH_Func)SC_SHARE_SetRouteAveTimne},		// 経路所要時間
	{(SC_DH_Func)SC_SHARE_GetRouteHwayLength,	(SC_DH_Func)SC_SHARE_SetRouteHwayLength},	// 経路高速距離
	{(SC_DH_Func)SC_SHARE_GetRouteTollLength,	(SC_DH_Func)SC_SHARE_SetRouteTollLength},	// 経路有料距離
	{(SC_DH_Func)SC_SHARE_GetRouteTollFee,		(SC_DH_Func)SC_SHARE_SetRouteTollFee},		// 経路料金
	{(SC_DH_Func)SC_SHARE_GetCarStateReal,		(SC_DH_Func)SC_SHARE_SetCarStateReal},		// 車両状態情報(実ロケーション)
	{(SC_DH_Func)SC_SHARE_GetCarStateSimu,		(SC_DH_Func)SC_SHARE_SetCarStateSimu},		// 車両状態情報(シミュレータロケーション)
	{(SC_DH_Func)SC_SHARE_GetUDIResourcePath,	(SC_DH_Func)SC_SHARE_SetUDIResourcePath},	// ユーザ定義アイコンのリソースと設定ファイルの格納パス
	{(SC_DH_Func)SC_SHARE_GetIconInfo,			(SC_DH_Func)SC_SHARE_SetIconInfo},			// ユーザ定義ダイナミックアイコンデータ
	{(SC_DH_Func)SC_SHARE_GetDynamicUDIDisplay,	(SC_DH_Func)SC_SHARE_SetDynamicUDIDisplay},	// ユーザ定義ダイナミックアイコンデータの表示/非表示
	{(SC_DH_Func)SC_SHARE_GetDemoMode,			(SC_DH_Func)SC_SHARE_SetDemoMode},			// 探索結果の有無
	{(SC_DH_Func)SC_SHARE_GetDeviationStatus,	(SC_DH_Func)SC_SHARE_SetDeviationStatus},	// 経路逸脱状態
	{(SC_DH_Func)SC_SHARE_GetDynamicGraphiSize,	(SC_DH_Func)SC_SHARE_SetDynamicGraphiSize},	// 交差点拡大図サイズ
	{(SC_DH_Func)SC_SHARE_GetDynamicGraphiStat,	(SC_DH_Func)SC_SHARE_SetDynamicGraphiStat},	// 交差点拡大図有無ステータス
	{(SC_DH_Func)SC_SHARE_GetDynamicGraphiBitmap,(SC_DH_Func)SC_SHARE_SetDynamicGraphiBitmap},// 交差点拡大図情報																			// 交差点拡大図
	{(SC_DH_Func)SC_SHARE_GetRouteCostInfo,		(SC_DH_Func)SC_SHARE_SetRouteCostInfo},		// 経路コスト情報
	{(SC_DH_Func)SC_SHARE_GetVehicleType,		(SC_DH_Func)SC_SHARE_SetVehicleType},		// 車両タイプ
	{(SC_DH_Func)SC_SHARE_GetSaveTracksFlag,	(SC_DH_Func)SC_SHARE_SetSaveTracksFlag},	// 走行軌跡を保存するかどうか
	{(SC_DH_Func)SC_SHARE_GetDebugInfoOnSurface,(SC_DH_Func)SC_SHARE_SetDebugInfoOnSurface},// デバッグ情報を描画するかどうか
	{(SC_DH_Func)SC_SHARE_GetEchoFlag,			(SC_DH_Func)SC_SHARE_SetEchoFlag},			// 位置情報共有するかどうか
	{(SC_DH_Func)SC_SHARE_GetGenreData,			(SC_DH_Func)SC_SHARE_SetGenreData},			// ジャンルデータ
	{(SC_DH_Func)SC_SHARE_GetZoomMode,			(SC_DH_Func)SC_SHARE_SetZoomMode},			// 地図ズームモード
	{(SC_DH_Func)SC_SHARE_GetRegion,			(SC_DH_Func)SC_SHARE_SetRegion},			// リージョン
	{(SC_DH_Func)SC_SHARE_GetLanguage,			(SC_DH_Func)SC_SHARE_SetLanguage},			// 言語
	{(SC_DH_Func)SC_SHARE_GetMappingAlert,		(SC_DH_Func)SC_SHARE_SetMappingAlert},		// アラート表示情報
	{(SC_DH_Func)SC_SHARE_GetTraffic,			(SC_DH_Func)SC_SHARE_SetTraffic},			// 交通情報
};

//-----------------------------------
// 変数定義
//-----------------------------------
static void *shareMem;
static SC_MUTEX shareMutex = SC_MUTEX_INITIALIZER;

//-----------------------------------
// 関数定義
//-----------------------------------
static E_SC_RESULT SC_DH_DalInitialize(const Char *dirPath);
static E_SC_RESULT SC_DH_DalFinalize();
static E_SC_RESULT SC_DH_GetShareDataDispatch(SC_DH_SHARE_DATA *data);
static E_SC_RESULT SC_DH_SetShareDataDispatch(SC_DH_SHARE_DATA *data);

/**
 * @brief 常駐メモリを割り当てる
 * @param[in] mapDirPath  地図データ格納ディレクトリのフルパス
 * @param[in] confDirPath ナビコアの設定ファイル格納ディレクトリのフルパス(NULL可)
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_DH_MemInitialize(const Char *mapDirPath, const Char *confDirPath)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	do {
		// 常駐メモリ割り当て
		shareMem = SC_MEM_Alloc(SC_MEM_SIZE_SHARE, e_MEM_TYPE_SHARE);
		if (NULL == shareMem) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_MEM_Alloc(e_MEM_SHARE) error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		memset(shareMem, 0, SC_MEM_SIZE_SHARE);
		// 常駐メモリ初期化
		ret = SC_SHARE_Initialize(shareMem, confDirPath);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_SHARE_Initialize error, " HERE);
			break;
		}
#if 0
		// 常駐メモリのデータをファイルから読み込む
		ret = SC_DH_LoadShareData();
		if ((e_SC_RESULT_SUCCESS != ret) && (e_SC_RESULT_NODATA != ret)) {
			SC_LOG_ErrorPrint(SC_TAG_SHARE, "SC_SHARE_LoadShareData error, " HERE);
			break;
		}
#endif

		// キャッシュメモリの初期化
		ret = SC_DHC_CashInit();
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_DHC_CashInit error, " HERE);
			break;
		}

		// Mutex生成
		ret = SC_CreateMutex(&shareMutex);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_CreateMutext(e_MEM_SHARE) error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// DALの初期化APIを呼び出す
		ret = SC_DH_DalInitialize(mapDirPath);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_DH_DalInitialize error, " HERE);
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}

/**
 * @brief 常駐メモリを解放する
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_DH_MemFinalize()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	// エラーでも処理は継続する

	// 地図キャッシュ処理終了
	if (e_SC_RESULT_SUCCESS != SC_DHC_CashFinalize()) {
		SC_LOG_ErrorPrint(SC_TAG_DH, "SC_DHC_CashFinalize() error, " HERE);
	}

	if (NULL != shareMem) {
		// 常駐メモリ解放
		SC_MEM_Free(shareMem, e_MEM_TYPE_SHARE);
		shareMem = NULL;
	}

	// Mutex破棄
	ret = SC_DestroyMutex(&shareMutex);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_DH, "SC_DestroyMutext(e_MEM_SHARE) error, " HERE);
	}

	// DALの終了化APIを呼び出す
	ret2 = SC_DH_DalFinalize();
	if (e_SC_RESULT_SUCCESS != ret2) {
		SC_LOG_ErrorPrint(SC_TAG_DH, "SC_DH_DalFinalize error, " HERE);
		if (e_SC_RESULT_SUCCESS == ret) {
			ret = ret2;
		}
	}

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}

/**
 * @brief 常駐メモリデータを取得する
 * @param[in/out] shareData 常駐メモリデータ入出力構造体
 * @return 処理結果(E_SC_RESULT)
 * @
 */
E_SC_RESULT SC_DH_GetShareData(SC_DH_SHARE_DATA *shareData)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	Bool	isMutexLocked = false;

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == shareData) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "param error[shareData], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == shareData->data) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "param error[shareData->data], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// Mutexロック
		ret = SC_LockMutex(&shareMutex);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_LockMutext error, " HERE);
			break;
		}
		isMutexLocked = true;

		// データ識別IDで指定されたデータを常駐メモリから取得する
		ret = SC_DH_GetShareDataDispatch(shareData);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_DH_GetShareDataDispatch error, " HERE);
			break;
		}
	} while (0);

	if (true == isMutexLocked) {
		// Mutexアンロック
		ret2 = SC_UnLockMutex(&shareMutex);
		if (e_SC_RESULT_SUCCESS != ret2) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_UnLockMutext error, " HERE);
			if (e_SC_RESULT_SUCCESS == ret) {
				ret = ret2;
			}
		}
	}

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}

/**
 * @brief 常駐メモリデータを設定する
 * @param[in] data 常駐メモリデータ入出力構造体
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_DH_SetShareData(SC_DH_SHARE_DATA *shareData)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	Bool	isMutexLocked = false;

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == shareData) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "param error[shareData], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// Mutexロック
		ret = SC_LockMutex(&shareMutex);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_LockMutext error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}
		isMutexLocked = true;

		// データ識別IDで指定されたデータを常駐メモリに設定する
		ret = SC_DH_SetShareDataDispatch(shareData);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_DH_SetShareDataDispatch error, " HERE);
			break;
		}
#if 0
		// 常駐メモリのデータをファイルに保存する
		ret = SC_DH_SaveShareData();
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_DH_SaveShareData error, " HERE);
			break;
		}
#endif
	} while (0);

	if (true == isMutexLocked) {
		// Mutexアンロック
		ret2 = SC_UnLockMutex(&shareMutex);
		if (e_SC_RESULT_SUCCESS != ret2) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_UnLockMutext error, " HERE);
			if (e_SC_RESULT_SUCCESS == ret) {
				ret = ret2;
			}
		}
	}

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}

/**
 * @brief DALを初期化する
 * @param[in] dirPath 地図格納フォルダのフルパス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_DH_DalInitialize(const Char *dirPath)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DA_RESULT dal_res = SC_DA_RES_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	// DAL初期化
	dal_res = SC_DA_Initialize(dirPath);
	if (SC_DA_RES_SUCCESS != dal_res) {
		SC_LOG_ErrorPrint(SC_TAG_DH, "SC_DA_Initialize(0x%08x), " HERE, dal_res);
		ret = e_SC_RESULT_RDB_ACCESSERR;
	}

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}

/**
 * @brief DALを終了化する
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_DH_DalFinalize()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_DA_RESULT dal_res = SC_DA_RES_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	// DAL終了化
	dal_res = SC_DA_Finalize();
	if (SC_DA_RES_SUCCESS != dal_res) {
		SC_LOG_ErrorPrint(SC_TAG_DH, "SC_DA_Finalize(0x%08x), " HERE, dal_res);
		ret = e_SC_RESULT_RDB_ACCESSERR;
	}

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}

/**
 * @brief 常駐メモリデータを取得する
 * @param[in] data 常駐メモリデータ入出力構造体
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_DH_GetShareDataDispatch(SC_DH_SHARE_DATA *data)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	// パラメータチェック
	if (true != SC_DH_CHECK_SHAREDATA_ID(data->dataId)) {
		SC_LOG_ErrorPrint(SC_TAG_DH, "parame error[data->dataId], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 常駐メモリのデータ取得
	ret = funcTbl[data->dataId].getFunc(data->data);

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}

/**
 * @brief 常駐メモリデータを設定する
 * @param[in] data 常駐メモリデータ入出力構造体
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_DH_SetShareDataDispatch(SC_DH_SHARE_DATA *data)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	// パラメータチェック
	if (true != SC_DH_CHECK_SHAREDATA_ID(data->dataId)) {
		SC_LOG_ErrorPrint(SC_TAG_DH, "parame error[data->dataId], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 常駐メモリのデータ設定
	ret = funcTbl[data->dataId].setFunc(data->data);

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}

/**
 * @brief
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_DH_SaveShareData()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	Bool	isMutexLocked = false;

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	do {
		// Mutexロック
		ret = SC_LockMutex(&shareMutex);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_LockMutext error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}
		isMutexLocked = true;

		// バックアップファイルから共有メモリ復帰
		ret = SC_SHARE_SaveShareData();
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_SHARE_SaveShareData error, " HERE);
			break;
		}
	} while (0);

	if (true == isMutexLocked) {
		// Mutexアンロック
		ret2 = SC_UnLockMutex(&shareMutex);
		if (e_SC_RESULT_SUCCESS != ret2) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_UnLockMutext error, " HERE);
			if (e_SC_RESULT_SUCCESS == ret) {
				ret = ret2;
			}
		}
	}

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}

/**
 * @brief
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_DH_LoadShareData()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT	ret2 = e_SC_RESULT_SUCCESS;
	Bool	isMutexLocked = false;

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	do {
		// Mutexロック
		ret = SC_LockMutex(&shareMutex);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_LockMutext error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}
		isMutexLocked = true;

		// 共有メモリをファイル出力
		ret = SC_SHARE_LoadShareData();
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_SHARE_LoadShareData error, " HERE);
			break;
		}
	} while (0);

	if (true == isMutexLocked) {
		// Mutexアンロック
		ret2 = SC_UnLockMutex(&shareMutex);
		if (e_SC_RESULT_SUCCESS != ret2) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_UnLockMutext error, " HERE);
			if (e_SC_RESULT_SUCCESS == ret) {
				ret = ret2;
			}
		}
	}

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}

/**
 * @brief
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_DH_RouteBackup() {
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT ret2 = e_SC_RESULT_SUCCESS;
	Bool isMutexLocked = false;

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	do {
		// Mutexロック
		ret = SC_LockMutex(&shareMutex);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_LockMutext error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}
		isMutexLocked = true;

		// 経路バックアップ作成
		ret = SC_RouteBackup();
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_RouteBackup error, " HERE);
			break;
		}
	} while (0);

	if (true == isMutexLocked) {
		// Mutexアンロック
		ret2 = SC_UnLockMutex(&shareMutex);
		if (e_SC_RESULT_SUCCESS != ret2) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_UnLockMutext error, " HERE);
			if (e_SC_RESULT_SUCCESS == ret) {
				ret = ret2;
			}
		}
	}

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}

/**
 * @brief
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_DH_RouteBackupDelete() {
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT ret2 = e_SC_RESULT_SUCCESS;
	Bool isMutexLocked = false;

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	do {
		// Mutexロック
		ret = SC_LockMutex(&shareMutex);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_LockMutext error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}
		isMutexLocked = true;

		// 経路バックアップファイル削除
		ret = SC_RouteBackupDelete();
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_RouteBackupDelete error, " HERE);
			break;
		}
	} while (0);

	if (true == isMutexLocked) {
		// Mutexアンロック
		ret2 = SC_UnLockMutex(&shareMutex);
		if (e_SC_RESULT_SUCCESS != ret2) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_UnLockMutext error, " HERE);
			if (e_SC_RESULT_SUCCESS == ret) {
				ret = ret2;
			}
		}
	}

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}

/**
 * @brief
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_DH_GetRouteBackup(SMROUTEBACKUP* backup) {
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	E_SC_RESULT ret2 = e_SC_RESULT_SUCCESS;
	Bool isMutexLocked = false;
	//INT32 result = 0;

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	do {
		// Mutexロック
		ret = SC_LockMutex(&shareMutex);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_LockMutext error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}
		isMutexLocked = true;

		// 経路バックアップファイル読み込み
		ret = SC_GetRouteBackup(backup);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_GetRouteBackup error, " HERE);
			break;
		}
	} while (0);

	if (true == isMutexLocked) {
		// Mutexアンロック
		ret2 = SC_UnLockMutex(&shareMutex);
		if (e_SC_RESULT_SUCCESS != ret2) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "SC_UnLockMutext error, " HERE);
			if (e_SC_RESULT_SUCCESS == ret) {
				ret = ret2;
			}
		}
	}

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}
