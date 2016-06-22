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
 * navicore.h
 *
 *  Created on: 2016/03/14
 *      Author:
 */

#ifndef NAVICORE_H_
#define NAVICORE_H_

#include "navicoredef.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief SMCoreを初期化する
 * @param[in]	iwidth			解像度（横）
 * @param[in]	iheight			解像度（縦）
 * @param[in]	strRootPath		rootパス。絶対パス指定。
 * @param[in]	strMapPath		Map/XXXのパス。絶対パス指定。
 * @param[in]	strLocatorPath	ロケータ設定ファイルのパス。絶対パス指定。
 * @return 処理結果(0：正常、-1：異常)、-2:エラー)
 */
INT32 NC_Initialize(
		INT32 iwidth,
		INT32 iheight,
		const Char* strRootPath,
		const Char* strMapPath,
		const Char* strLocatorPath
);

/**
 * @brief NaviCoreが初期化済かどうかを確認する。
 * @return 処理結果(true：初期化済み、false：未初期化)
 */
Bool NC_IsInitialized();

/**
 * @brief リソースをリリースする
 */
void NC_Finalize();

/**
 * @brief ロケーション更新CB登録
 * @param[in]	pfunc		CB関数ポインタ
 * @return 処理結果(正常：0、異常：-1、-2:エラー)
 */
INT32 NC_LC_SetLocationUpdateCB(NC_LOCATORCBFUNCPTR pfunc);

/**
 * @brief ビットマップフォント生成CB登録
 * @param[in]	pfunc		CB関数ポインタ
 * @return 処理結果(正常：0、異常：-1、-2:エラー)
 */
INT32 NC_MP_SetBitmapFontCB(NC_BITMAPFONTFUNCPTR pfunc);

/**
 * @brief 画像読み込みCB登録
 * @param[in]	pfunc		CB関数ポインタ
 * @return 処理結果(正常：0、異常：-1、-2:エラー)
 */
INT32 NC_MP_SetImageReadForFileCB(NC_IMAGEFUNCPTR pfunc);

/**
 * @brief 画像読み込みCB登録
 * @param[in]	pfunc		CB関数ポインタ
 * @return 処理結果(正常：0、異常：-1、-2:エラー)
 */
INT32 NC_MP_SetImageReadForImageCB(NC_IMAGEFUNCPTR pfunc);

/**
 * @brief 描画リソースを初期化する
 * @param[in]	iMaps	地図種類
 * @return 処理結果(正常：0、異常：-1、-2:エラー)
 */
INT32 NC_MP_InitResource(INT32 iMaps);


/**
 * @brief 地図縮尺を設定する
 * @param[in] iMaps             地図種類
 * @param[in] iScale            地図縮尺
 * @return 処理結果(正常：0、異常：-1)
 * @warning 本APIを呼び出した後、NC_MP_RefreshMapを呼び出して地図状態を設定する。
 */
INT32 NC_MP_SetMapScaleLevel(INT32 iMaps, INT32 iScale);

/**
 * @brief 地図の縮尺を取得する
 * @param[in] iMaps             地図種類
 * @return 処理結果(正常：地図縮尺(>=0)、異常：-1)
 * @warning NC_MP_RefreshMapを呼び出した後、本APIを呼び出して最新地図状態を取得する。
 */
INT32 NC_MP_GetMapScaleLevel(INT32 iMaps);

/**
 * @brief 地図表示モードを設定する
 * @param[in] iMaps             地図種類
 * @param[in] iDispMode         地図表示モード
 * @return 処理結果(正常：0、異常：-1、-2:地図表示モードエラー)
 * @warning 本APIを呼び出した後、NC_MP_RefreshMapを呼び出して地図状態を設定する。
 */
INT32 NC_MP_SetMapDispMode(INT32 iMaps, INT32 iDispMode);

/**
 * @brief 地図表示モードを取得する
 * @param[in] iMaps             地図種類
 * @return 処理結果(正常：地図表示モード
 *                        ヘッディングアップモード(SC_MDM_HEADUP=0)
 *                        ノースアップモード(SC_MDM_NORTHUP=1)
 *                        バードビューモード(SC_MDM_BIRDVIEW=2)
 *                  異常：-1)
 * @warning NC_MP_RefreshMapを呼び出した後、本APIを呼び出して最新地図状態を取得する。
 */
INT32 NC_MP_GetMapDispMode(INT32 iMaps);

/**
 * @brief 地図をドライバーモードに設定する
 * @param[in] iMaps             地図種類
 * @param[in] bMoveWithCar      ドライバーモードか否か
 * @return 処理結果(正常：0、異常：-1)
 * @warning 本APIを呼び出した後、NC_MP_RefreshMapを呼び出して地図状態を設定する。
 */
INT32 NC_MP_SetMapMoveWithCar(INT32 iMaps, Bool bMoveWithCar);

/**
 * @brief 地図をドライバーモードに取得する
 * @param[in] iMaps             地図種類
 * @return 処理結果(1：ドライバーモード、0：ドライバーモードではない。-1：iMap引数範囲外エラー)
 * @warning 本APIを呼び出した後、NC_MP_RefreshMapを呼び出して地図状態を設定する。
 */
INT32 NC_MP_GetMapMoveWithCar(INT32 iMaps);

/**
 * @brief 指定した長さと角度で、地図をスクロールする
 * @param[in] iMaps             地図種類
 * @param[in] fDegreeToward     移動角度について
 *                              単位：度
 *                              値範囲：0～359
 *                              正方向：アップ
 *                              プラス値：時計回り
 *                              なお、値範囲を超える場合は、自動的に値範囲内の角度を変換する
 *                              （例：３６１度→１度）
 * @param[in] iPixelStep        移動長さ
 *                              （範囲：0～地図viewport範囲の対角線の長さ）
 * @return 処理結果(正常：0、異常：-1、移動長さ異常：-2)
 * @warning 本APIを呼び出した後、NC_MP_RefreshMapを呼び出して地図状態を設定する。
 */
INT32 NC_MP_MoveMapDir(INT32 iMaps, FLOAT fDegreeToward, INT32 iPixelStep);

/**
 * @brief 地図のビューポートを設定する
 * @param[in]	iMaps	地図種類
 * @param[in]	lLeft	地図ビューポートの左上スクリーンX座標
 * 						値範囲：0～デバイスX軸の解像度
 * @param[in]	lTop	地図ビューポートの左上スクリーンY座標
 * 						値範囲：0～デバイスY軸の解像度
 * @param[in]	lRight	地図ビューポートの右下スクリーンX座標
 * 						値範囲：0～デバイスX軸の解像度、かつlLeftより大きい
 * @param[in]	lBottom	地図ビューポートの右下スクリーンY座標
 * 						値範囲：0～デバイスY軸の解像度、かつlTopより大きい
 * @return 処理結果(正常：0、異常：-1、-2:ビューポート設定エラー)
 * @warning 本APIを呼び出した後、NC_MP_RefreshMapを呼び出して地図状態を設定する。
 */
INT32 NC_MP_SetMapViewport(INT32 iMaps, INT32 left, INT32 top, INT32 right, INT32 bottom);

/**
 * @brief 地図の回転角度を設定する
 * @param[in] iMaps             地図種類
 * @param[in] iDegree           地図の回転角度 地図の回転角度(0～359)
 * @return 処理結果(正常：0、異常：-1)
 */
INT32 NC_MP_SetMapRotate(INT32 iMaps, INT32 iDegree);

/**
 * @brief 地図をリフレッシュする
 * @param[in] iMaps             地図種類
 * @return 処理結果(正常：0、異常：-1)
 */
INT32 NC_MP_RefreshMap(INT32 iMaps);

/**
 * @brief ユーザ定義アイコンのリソースと、設定ファイルの格納パスを設定する。
 * @param[in] strPathIconDir ユーザ定義アイコンリソースの格納パス。NULLにするのは不可。
 * @param[in] strPathIconInfo ユーザ定義アイコンの設定情報の格納パス。NULLにするのは不可。
 * @return 処理結果(正常：0、異常：-1、-2:エラー)
 * @warning 本APIを呼び出した後、NC_MP_RefreshMapを呼び出して地図状態を設定する。
 */
INT32 NC_MP_SetUDIResource(const Char *strPathIconDir, const Char *strPathIconInfo);


/**
 * @brief 車両目的地到着状態を取得する
 * @return 0:CMS_STOP, 1:CMS_MOVE, 2:CMS_ARRIVE
 */
INT32 NC_DM_GetCarMoveStatus();

/**
 * @brief 現在ルートの探索結果の有無を判定する
 * @return 探索結果の有無(あり：true、なし：false)
 */
Bool NC_DM_IsExistRoute();

/**
 * @brief 現在ルートの総距離を取得する
 * @return 経路総距離
 */
INT32 NC_DM_GetRouteLength();

/**
 * @brief 現在ルートの所用時間を取得する
 * @return 経路所要時間
 */
INT32 NC_DM_GetRouteAveTime();

/**
 * @brief 現在ルートの高速道路距離を取得する
 * @return 経路高速道路距離
 */
INT32 NC_DM_GetRouteHighwayLength();

/**
 * @brief 現在ルートの有料道路距離を取得する
 * @return 経路有料道路距離
 */
INT32 NC_DM_GetRouteTollwayLength();

/**
 * @brief 現在ルートの料金を取得する
 * @return 経路有料金
 */
INT32 NC_DM_GetRouteTollFee();

/**
 * @brief NaviCoreがルート探索中かどうかを取得する
 * @return 探索中かどうか(探索中：true、探索完了または探索未実行：false)
 */
Bool NC_RP_IsPlanning();

/**
 * @brief 探索をキャンセルする
 */
INT32 NC_RP_CancelPlanningRoute();


/**
 * @brief 出発地、経由地、目的地を設定して、探索する
 * @param[in] newPoint             地点情報
 * @param[in] newPointNum          地点情報数数
 */
Bool NC_RP_PlanSingleRoute(SMRPPOINT *newPoint, INT32 newPointNum);

/**
 * @brief カレント経路（地点及び結果を含む）を削除する
 * @return 処理結果(正常：true、異常：false)
 */
Bool NC_RP_DeleteRouteResult();

/**
 * @brief 経路誘導を開始する
 */
void NC_Guide_StartGuide();

/**
 * @brief 経路誘導を実行する
 */
INT32 NC_Guide_RunGuide();

/**
 * @brief 経路誘導を終了する
 */
void NC_Guide_StopGuide();

/**
 * @brief 経路誘導を一時停止する
 */
void NC_Guide_PauseGuide();

/**
 * @brief 経路誘導を再開する
 */
void NC_Guide_ResumeGuide();

/**
 * @brief 誘導中判定
 */
Bool NC_Guide_IsGuiding();

/**
 * @brief 誘導停止判定
 */
Bool NC_Guide_IsGuideStop();

/**
 * @brief 誘導一時停止判定
 */
Bool NC_Guide_IsGuidePause();

/**
 * @brief リアルタイムの案内情報を取得する
 * @param[in] guide   リアルタイム案内情報
 */
INT32 NC_Guide_GetRealTimeInfo(SMREALTIMEGUIDEDATA *guide);

/**
 * @brief シミュレーションを開始する
 * @return 処理結果(正常：e_SC_RESULT_SIM_SUCCESS、異常：e_SC_RESULT_SIM_FAILD)
 */
INT32 NC_Simulation_StartSimulation();

/**
 * @brief シミュレーションを一時停止する
 */
void NC_Simulation_PauseSimulation();

/**
 * @brief シミュレーションを終了する
 * @return 処理結果(正常：e_SC_RESULT_SIM_SUCCESS、異常：e_SC_RESULT_SIM_FAILD)
 */
INT32 NC_Simulation_ExitSimulation();

/**
 * @brief GC_Simulation_SetSpeedで設定した距離だけ、シミュレーションでの車両位置を進める
 * @return 処理結果(正常：e_SC_RESULT_SIM_SUCCESS、異常：e_SC_RESULT_SIM_FAILD)
 */
INT32 NC_Simulation_CalcNextPos();

/**
 * @brief シミュレーションの進捗度を取得する
 */
INT32 NC_Simulation_GetPosPercent();

/**
 * @brief シミュレーションのステップ距離を設定する
 */
void NC_Simulation_SetSpeed(INT32 iSpeed);

/**
 * @brief シミュレーション中かどうかを取得する
 */
Bool NC_Simulation_IsInSimu();

/**
 * @brief シミュレーションが一時停止状態かどうかを取得する
 */
Bool NC_Simulation_IsSimuPause();

/**
 * @brief シミュレーションが停止状態かどうかを取得する
 */
Bool NC_Simulation_IsSimuStop();

/**
 * @brief シミュレーションステップ距離を取得する
 */
INT32 NC_Simulation_GetSpeed();

/**
 * @brief シミュレーションを再開する
 */
void NC_Simulation_ResumeSimulation();


#ifdef __cplusplus
}
#endif

#endif /* NAVICORE_H_ */
