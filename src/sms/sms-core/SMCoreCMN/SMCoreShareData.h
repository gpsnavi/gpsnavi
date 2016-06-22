/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCORE_SHARE_DATA_H
#define SMCORE_SHARE_DATA_H

//-----------------------------------
// 定数定義
//-----------------------------------
#define	SC_SHARE_MAPS_NUM				3		// 表示操作対象地図
												//  メイン地図(1)、補助地図(2)、スモール地図(4)
#define	SC_SHARE_CLSCD_NUM				12		// ランドマーク種別コード
												//  買い物(1)、飲食(2)、ファースト·フード (3)、駐車場(4)、ガソリンスタンド(5)、銀行(6)、
												//  レンタカー(7)、カー備品(8)、駅(9)、住宿(10)、カーディーラー(11)、トイレ(12)
#define	SC_SHARE_POINT_NUM				SC_CORE_RP_PLACE_MAX	// 地点情報数
#define	SC_SHARE_CARSTATE_NUM			2		// 自車位置（実ロケーション，シミュレートロケーション）
#define	SC_SHARE_ICON_NUM				2048	// アイコン情報数

// 表示操作対象地図
#define	SC_SHARE_MAPS_MAIN				1		// メイン地図
#define	SC_SHARE_MAPS_APPEND			2		// 補助地図
#define	SC_SHARE_MAPS_SMALL				4		// スモール地図


//-----------------------------------
// 構造体定義
//-----------------------------------
#if 0
// 常駐メモリデータ入出力構造体
typedef struct _SC_DH_SHARE_DATA {
	E_SC_DH_SHARE_DATA_ID	dataId;			// [I]データ識別ID
	void					*data;			// [I/O]入力データ格納領域のポインタ
} SC_DH_SHARE_DATA;
#endif

// 地図表示縮尺(スケール)
typedef struct _SC_DH_SHARE_SCALE {
	INT32					maps;			// [I]表示操作対象地図
	INT32					scaleLevel;		// [I/O]地図表示縮尺(スケール)
	FLOAT					scaleRange;		// [I/O]拡大率
	INT32					zoomLevel;		// [I/O]ズームレベル
} SC_DH_SHARE_SCALE;

// 地図上文言拡大表示
typedef struct _SC_DH_SHARE_BIGTEXTATTR {
	INT32					maps;			// [I]表示操作対象地図
	Bool					bigText;		// [I/O]地図上文言拡大表示
	Char					reserve[3];		// 予約
} SC_DH_SHARE_BIGTEXTATTR;

// 地図上表示アイコン拡大表示
typedef struct _SC_DH_SHARE_BIGICONATTR {
	INT32					maps;			// [I]表示操作対象地図
	Bool					bigIcon;		// [I/O]地図上表示アイコン拡大表示
	Char					reserve[3];		// 予約
} SC_DH_SHARE_BIGICONATTR;

// ランドマーク表示
typedef struct _SC_DH_SHARE_LANDMARK {
	INT32					maps;			// [I]表示操作対象地図
	Bool					isShow;			// [I/O]ランドマーク表示
	Char					reserve[3];		// 予約
} SC_DH_SHARE_LANDMARK;

// ランドマーク表示属性
typedef struct _SC_DH_SHARE_LANDMARKATTR {
	INT32					maps;			// [I]表示操作対象地図
	INT32					classCode;		// [I]ランドマーク種別コード
	Bool					isShow;			// [I/O]ランドマーク表示属性
	Char					reserve[3];		// 予約
} SC_DH_SHARE_LANDMARKATTR;

// 地図表示モード
typedef struct _SC_DH_SHARE_DISPMODE {
	INT32					maps;			// [I]表示操作対象地図
	INT32					dispMode;		// [I/O]地図表示モード
} SC_DH_SHARE_DISPMODE;

// 地図ドライバモード
typedef struct _SC_DH_SHARE_DRIVERMODE {
	INT32					maps;			// [I]表示操作対象地図
	Bool					driverMode;		// [I/O]地図ドライバモード
	Char					reserve[3];		// 予約
} SC_DH_SHARE_DRIVERMODE;

// 地図中心カーソルの表示ステータス
typedef struct _SC_DH_SHARE_SHOWCURSOR {
	INT32					maps;			// [I]表示操作対象地図
	Bool					isShow;			// [I/O]地図中心カーソルの表示
	Char					reserve[3];		// 予約
} SC_DH_SHARE_SHOWCURSOR;

// 車両状態情報
typedef struct _SC_DH_SHARE_CARSTATE {
	SMCARSTATE				carState;		// [I/O]車両状態情報
} SC_DH_SHARE_CARSTATE;

// 地図スクロールモード
typedef struct _SC_DH_SHARE_SCROLLMODE {
	INT32					maps;			// [I]表示操作対象地図
	Bool					mode;			// [I/O]地図スクロールモード
	Char					reserve[3];		// 予約
} SC_DH_SHARE_SCROLLMODE;

// 地図の移動情報(移動角度と移動長さ)
typedef struct _SC_DH_SHARE_MOVEMAPDIR {
	INT32					maps;			// [I]表示操作対象地図
	FLOAT					degreeToward;	// [I/O]移動角度
	INT32					pixelStep;		// [I/O]移動長さ
} SC_DH_SHARE_MOVEMAPDIR;

// フリーズーム時の地図拡大比例
typedef struct _SC_DH_SHARE_STEPRATE {
	INT32					maps;			// [I]表示操作対象地図
	FLOAT					rate;			// [I/O]フリーズーム時の地図拡大比例
} SC_DH_SHARE_STEPRATE;

// 地図中心の地理座標
typedef struct _SC_DH_SHARE_GEOCOORD {
	INT32					maps;			// [I]表示操作対象地図
	SMGEOCOORD				coord;			// [I/O]地図中心の地理座標
} SC_DH_SHARE_GEOCOORD;

// 地図のビューポート
typedef struct _SC_DH_SHARE_RECT {
	INT32					maps;			// [I]表示操作対象地図
	SMRECT					rect;			// [I/O]地図のビューポート
} SC_DH_SHARE_RECT;

// 地図の回転角度
typedef struct _SC_DH_SHARE_ROTATE {
	INT32					maps;			// [I]表示操作対象地図
	INT32					rotate;			// [I/O]地図の回転角度
} SC_DH_SHARE_ROTATE;

// 地図のスケール
typedef struct _SC_DH_SHARE_GEODISTANCE {
	INT32					maps;			// [I]表示操作対象地図
	INT32					unitPixel;		// [I]単位ドット数
	FLOAT					scale;			// [I/O]スケール
} SC_DH_SHARE_GEODISTANCE;

// 解像度
typedef struct _SC_DH_SHARE_RESOLUTION {
	INT32					width;			// [I/O]解像度(X座標)
	INT32					height;			// [I/O]解像度(Y座標)
} SC_DH_SHARE_RESOLUTION;

// 地点情報（出発地、経由地、目的地）
typedef struct _SC_DH_SHARE_RPPOINT {
	INT32					pointNum;						// [I/O]地点情報（出発地、経由地、目的地）数
	SMRPPOINT				point[SC_CORE_RP_PLACE_MAX];	// [I/O]地点情報（出発地、経由地、目的地）
} SC_DH_SHARE_RPPOINT;

// 探索結果の有無
typedef struct _SC_DH_SHARE_EXISTROUTE {
	Bool					isExistRoute;	// [I/O]探索結果の有無
	Char					reserve[3];		// 予約
} SC_DH_SHARE_EXISTROUTE;

// 探索条件の有無
typedef struct _SC_DH_SHARE_PLANNING {
	Bool					isPlanning;		// [I/O]探索条件の有無
	Char					reserve[3];		// 予約
} SC_DH_SHARE_PLANNING;

// 探索条件
typedef struct _SC_DH_SHARE_RPOPTION {
	SMRPOPTION				option;			// [I/O]探索条件
} SC_DH_SHARE_RPOPTION;

// 探索詳細エラー情報
typedef struct _SC_DH_SHARE_RPTIPINFO {
	SMRPTIPINFO				tipInfo;		// [I/O]探索詳細エラー情報
} SC_DH_SHARE_RPTIPINFO;

// シミュレータ環境
typedef struct _SC_DH_SHARE_SIMULATE {
	E_SC_SIMULATE			sumilate;		// [I/O]シミュレータ環境かどうか
} SC_DH_SHARE_SIMULATE;
// シミュレータ速度
typedef struct _SC_DH_SHARE_SIMULATE_SPEED {
	INT32					speed;			// [I/O]シミュレータ速度
} SC_DH_SHARE_SIMULATE_SPEED;
// シミュレータ状態
typedef struct _SC_DH_SHARE_SIMULATE_STATE {
	E_SC_SIMSTATE			state;			// [I/O]シミュレータ状態
} SC_DH_SHARE_SIMULATE_STATE;
// リルート条件閾値
typedef struct _SC_DH_SHARE_REPLAN {
	SMREPLAN				replan;			// [I/O]リルート条件閾値
} SC_DH_SHARE_REPLAN;
// リアルタイム案内情報
typedef struct _SC_DH_SHARE_GUIDE_DATA {
	SMREALTIMEGUIDEDATA		guideData;		// [I/O]リアルタイム案内情報
} SC_DH_SHARE_GUIDE_DATA;
// 音声ＴＴＳ情報
typedef struct _SC_DH_SHARE_VOICE_TTS {
	SMVOICETTS				voiceTTS;		// [I/O]音声ＴＴＳ情報
} SC_DH_SHARE_VOICE_TTS;
// ターンリスト情報
typedef struct _SC_DH_SHARE_TURN_LIST {
	SMTURNLIST				turnList;		// [I/O]ターンリスト情報
} SC_DH_SHARE_TURN_LIST;
// 誘導状態
typedef struct _SC_DH_SHARE_GUIDE_STATUS {
	E_GUIDE_STATUS			guide_status;	// [I/O]誘導状態
} SC_DH_SHARE_GUIDE_STATUS;
// 経路逸脱状態
typedef struct _SC_DH_SHARE_DEVIATION_STATUS {
	E_DEVIATION_STATUS	deviation_status;			// [I/O]経路逸脱状態
} SC_DH_SHARE_DEVIATION_STATUS;

typedef struct _SC_DH_SHARE_ROUTE_LENGTH {
	INT32					length;					// [I/O]経路総距離
} SC_DH_SHARE_ROUTE_LENGTH;
typedef struct _SC_DH_SHARE_ROUTE_AVETIME {
	INT32					avetime;				// [I/O]経路所要時間
} SC_DH_SHARE_ROUTE_AVETIME;
typedef struct _SC_DH_SHARE_ROUTE_HWAYLENGTH {
	INT32					hwaylength;				// [I/O]経路高速総距離
} SC_DH_SHARE_ROUTE_HWAYLENGTH;
typedef struct _SC_DH_SHARE_ROUTE_TOLLLENGTH {
	INT32					tolllength;				// [I/O]経路有料総距離
} SC_DH_SHARE_ROUTE_TOLLLENGTH;
typedef struct _SC_DH_SHARE_ROUTE_TOLLFEE {
	INT32					tollfee;				// [I/O]経路料金
} SC_DH_SHARE_ROUTE_TOLLFEE;
// ユーザ定義アイコンのリソースと設定ファイルの格納パス
typedef struct _SC_DH_SHARE_ICON_RESOURCE_PATH {
	Char					pathIconDir[SC_MAX_PATH];	// [I/O]ユーザ定義アイコンのリソースファイルのパス
	Char					pathIconInfo[SC_MAX_PATH];	// [I/O]設定ファイルの格納パスのパス
} SC_DH_SHARE_ICON_RESOURCE_PATH;
// ユーザ定義ダイナミックアイコンデータ
typedef struct _SC_DH_SHARE_MAPDYNUDI {
	INT32					iconNum;						// [I/O]ユーザ定義ダイナミックアイコンデータ数
	SMMAPDYNUDI				iconInfo[SC_SHARE_ICON_NUM];	// [I/O]ユーザ定義ダイナミックアイコンデータ
	Bool					iconDisp[SC_SHARE_ICON_NUM];	// [I/O]ユーザ定義ダイナミックアイコンデータの表示/非表示
} SC_DH_SHARE_MAPDYNUDI;

// デモモード
typedef struct _SC_DH_SHARE_DEMOMODE {
	Bool					isDemoMode;	// [I/O]デモモード
} SC_DH_SHARE_DEMOMODE;

// 拡大図縦横サイズ
typedef struct _SC_DH_SHARE_DYNAMICGRAPHISIZE {
	INT32					width;					// [I/O]Ｘ方向サイズ
	INT32					height;					// [I/O]Ｙ方向サイズ
} SC_DH_SHARE_DYNAMICGRAPHISIZE;
// 交差点拡大図有無
typedef struct _SC_DH_SHARE_DYNAMICGRAPHISTATUS {
	E_DYNAMICGRAPHIC_STATUS	graphic_stat;			// [I/O]交差点拡大図有無
} SC_DH_SHARE_DYNAMICGRAPHICSTATUS;
// 交差点拡大図
typedef struct _SC_DH_SHARE_DYNAMICGRAPHIBITMAP{
	SMBITMAPINFO			bitmapinfo;				// [I/O]交差点拡大図情報
} SC_DH_SHARE_DYNAMICGRAPHIBITMAP;

// 経路コスト情報
typedef struct _SC_DH_SHARE_ROUTECOSTINFO {
	SMRTCOSTINFO			rtCostInfo;
} SC_DH_SHARE_ROUTECOSTINFO;

// 車両タイプ
typedef struct _SC_DH_SHARE_VEHICLETYPE {
	INT32					vehicleType;	// [I/O]車両タイプ
} SC_DH_SHARE_VEHICLETYPE;

// 走行ログの保存 する/しない
typedef struct _SC_DH_SHARE_SAVETRACKS {
	Bool					isSaveTracks;	// [I/O]走行ログの保存 する/しない
	Char					reserve[3];		// 予約
} SC_DH_SHARE_SAVETRACKS;

// デバッグ情報の描画 する/しない
typedef struct _SC_DH_SHARE_ONSURFACE {
	Bool					isOnSurface;	// [I/O]デバッグ情報の描画 する/しない
	Char					reserve[3];		// 予約
} SC_DH_SHARE_ONSURFACE;

// 位置情報共有 する/しない
typedef struct _SC_DH_SHARE_ECHO {
	Bool					isEcho;			// [I/O]位置情報共有 する/しない
	Char					reserve[3];		// 予約
} SC_DH_SHARE_ECHO;

// ジャンルデータ
typedef struct _SC_DH_SHARE_GENREDATA {
	SMGENREDATA				genreData;
} SC_DH_SHARE_GENREDATA;

// 地図ズームモード
typedef struct _SC_DH_SHARE_ZOOMMODE {
	INT32					maps;			// [I]表示操作対象地図
	Bool					mode;			// [I/O]地図ズームモード
	Char					reserve[3];		// 予約
} SC_DH_SHARE_ZOOMMODE;

// リージョン設定
typedef struct _SC_DH_SHARE_REGION {
	INT32					region;			// リージョン
} SC_DH_SHARE_REGION;

// 言語設定
typedef struct _SC_DH_SHARE_LANGUAGE {
	INT32					language;		// 言語
} SC_DH_SHARE_LANGUAGE;

// マッピングアラート情報
typedef struct _SC_DH_SHARE_MAPPINGALERT {
	SMMAPPINGALERT			alert;			// [I/O]マッピングアラート情報
} SC_DH_SHARE_MAPPINGALERT;

// 交通情報
typedef struct _SC_DH_SHARE_TRAFFIC {
	SMTRAFFIC				traffic;			// [I/O]交通情報
} SC_DH_SHARE_TRAFFIC;

//-----------------------------------
// 外部I/F定義
//-----------------------------------
E_SC_RESULT SC_SHARE_Initialize(void *ptr, const Char *confPath);
void SC_SHARE_Finalize();
E_SC_RESULT SC_SHARE_MemInit();
E_SC_RESULT SC_SHARE_LoadShareData();
E_SC_RESULT SC_SHARE_SaveShareData();

// 地図表示縮尺(スケール)
E_SC_RESULT SC_SHARE_GetScaleLevel(SC_DH_SHARE_SCALE *scale);
E_SC_RESULT SC_SHARE_SetScaleLevel(const SC_DH_SHARE_SCALE *scale);
// 地図上文言拡大表示
E_SC_RESULT SC_SHARE_GetBigTextAttr(SC_DH_SHARE_BIGTEXTATTR *bigText);
E_SC_RESULT SC_SHARE_SetBigTextAttr(const SC_DH_SHARE_BIGTEXTATTR *bigText);
// 地図上表示アイコン拡大表示
E_SC_RESULT SC_SHARE_GetBigIconAttr(SC_DH_SHARE_BIGICONATTR *bigIcon);
E_SC_RESULT SC_SHARE_SetBigIconAttr(const SC_DH_SHARE_BIGICONATTR *bigIcon);
// ランドマーク表示設定
E_SC_RESULT SC_SHARE_GetLandmark(SC_DH_SHARE_LANDMARK *landmark);
E_SC_RESULT SC_SHARE_SetLandmark(const SC_DH_SHARE_LANDMARK *landmark);
// ランドマーク表示属性設定
E_SC_RESULT SC_SHARE_GetLandmarkAttr(SC_DH_SHARE_LANDMARKATTR *landmark);
E_SC_RESULT SC_SHARE_SetLandmarkAttr(const SC_DH_SHARE_LANDMARKATTR *landmark);
// 地図表示モード
E_SC_RESULT SC_SHARE_GetDispMode(SC_DH_SHARE_DISPMODE *mode);
E_SC_RESULT SC_SHARE_SetDispMode(const SC_DH_SHARE_DISPMODE *mode);
// 地図ドライバモード
E_SC_RESULT SC_SHARE_GetDriverMode(SC_DH_SHARE_DRIVERMODE *mode);
E_SC_RESULT SC_SHARE_SetDriverMode(const SC_DH_SHARE_DRIVERMODE *mode);
// 地図中心カーソルの表示ステータス
E_SC_RESULT SC_SHARE_GetShowCursor(SC_DH_SHARE_SHOWCURSOR *cursor);
E_SC_RESULT SC_SHARE_SetShowCursor(const SC_DH_SHARE_SHOWCURSOR *cursor);
// 車両状態情報
E_SC_RESULT SC_SHARE_GetCarState(SC_DH_SHARE_CARSTATE *state);
E_SC_RESULT SC_SHARE_SetCarState(const SC_DH_SHARE_CARSTATE *state);
// 地図スクロールモード
E_SC_RESULT SC_SHARE_GetScrollMode(SC_DH_SHARE_SCROLLMODE *mode);
E_SC_RESULT SC_SHARE_SetScrollMode(const SC_DH_SHARE_SCROLLMODE *mode);
//地図の移動情報(移動角度と移動長さ)
E_SC_RESULT SC_SHARE_GetMoveMapDir(SC_DH_SHARE_MOVEMAPDIR *dir);
E_SC_RESULT SC_SHARE_SetMoveMapDir(const SC_DH_SHARE_MOVEMAPDIR *dir);
// フリーズーム時の地図拡大比例
E_SC_RESULT SC_SHARE_GetZoomStepRate(SC_DH_SHARE_STEPRATE *rate);
E_SC_RESULT SC_SHARE_SetZoomStepRate(const SC_DH_SHARE_STEPRATE *rate);
// 地図中心の地理座標
E_SC_RESULT SC_SHARE_GetMapCursorCoord(SC_DH_SHARE_GEOCOORD *coord);
E_SC_RESULT SC_SHARE_SetMapCursorCoord(const SC_DH_SHARE_GEOCOORD *coord);
// 地図のビューポート
E_SC_RESULT SC_SHARE_GetMapViewport(SC_DH_SHARE_RECT *rect);
E_SC_RESULT SC_SHARE_SetMapViewport(const SC_DH_SHARE_RECT *rect);
// 地図の回転角度
E_SC_RESULT SC_SHARE_GetMapRotate(SC_DH_SHARE_ROTATE *rotate);
E_SC_RESULT SC_SHARE_SetMapRotate(const SC_DH_SHARE_ROTATE *rotate);
// 地図のスケール
E_SC_RESULT SC_SHARE_GetGeoDistance(SC_DH_SHARE_GEODISTANCE *distance);
E_SC_RESULT SC_SHARE_SetGeoDistance(const SC_DH_SHARE_GEODISTANCE *distance);
// 解像度
E_SC_RESULT SC_SHARE_GetResolution(SC_DH_SHARE_RESOLUTION *resolution);
E_SC_RESULT SC_SHARE_SetResolution(const SC_DH_SHARE_RESOLUTION *resolution);
// 地点情報（出発地、経由地、目的地）
E_SC_RESULT SC_SHARE_GetAllRPPlace(SC_DH_SHARE_RPPOINT *point);
E_SC_RESULT SC_SHARE_SetAllRPPlace(const SC_DH_SHARE_RPPOINT *point);
// 探索結果の有無
E_SC_RESULT SC_SHARE_GetExistRoute(SC_DH_SHARE_EXISTROUTE *exist);
E_SC_RESULT SC_SHARE_SetExistRoute(const SC_DH_SHARE_EXISTROUTE *exist);
// 探索条件の有無
E_SC_RESULT SC_SHARE_GetPlanning(SC_DH_SHARE_PLANNING *plan);
E_SC_RESULT SC_SHARE_SetPlanning(const SC_DH_SHARE_PLANNING *plan);
// 探索条件
E_SC_RESULT SC_SHARE_GetRPOption(SC_DH_SHARE_RPOPTION *option);
E_SC_RESULT SC_SHARE_SetRPOption(const SC_DH_SHARE_RPOPTION *option);
// 探索詳細エラー情報
E_SC_RESULT SC_SHARE_GetRPTip(SC_DH_SHARE_RPTIPINFO *tipInfo);
E_SC_RESULT SC_SHARE_SetRPTip(const SC_DH_SHARE_RPTIPINFO *tipInfo);
// 探索結果削除
E_SC_RESULT SC_SHARE_DeleteRouteResult(void *param);
// シミュレータ環境
E_SC_RESULT SC_SHARE_GetSimulate(SC_DH_SHARE_SIMULATE *simu);
E_SC_RESULT SC_SHARE_SetSimulate(const SC_DH_SHARE_SIMULATE *simu);
// シミュレータ速度
E_SC_RESULT SC_SHARE_GetSimulateSpeed(SC_DH_SHARE_SIMULATE_SPEED *simuSpeed);
E_SC_RESULT SC_SHARE_SetSimulateSpeed(const SC_DH_SHARE_SIMULATE_SPEED *simuSpeed);
// シミュレータ状態
E_SC_RESULT SC_SHARE_GetSimulateState(SC_DH_SHARE_SIMULATE_STATE *simuState);
E_SC_RESULT SC_SHARE_SetSimulateState(const SC_DH_SHARE_SIMULATE_STATE *simuState);
// リルート条件閾値
E_SC_RESULT SC_SHARE_GetReplan(SC_DH_SHARE_REPLAN *replan);
E_SC_RESULT SC_SHARE_SetReplan(const SC_DH_SHARE_REPLAN *replan);
// リアルタイム案内情報
E_SC_RESULT SC_SHARE_GetRealTimeInfo(SC_DH_SHARE_GUIDE_DATA *guide);
E_SC_RESULT SC_SHARE_SetRealTimeInfo(const SC_DH_SHARE_GUIDE_DATA *guide);
// 誘導状態
E_SC_RESULT SC_SHARE_GetGuideStatus(SC_DH_SHARE_GUIDE_STATUS *status);
E_SC_RESULT SC_SHARE_SetGuideStatus(const SC_DH_SHARE_GUIDE_STATUS *status);
// 経路逸脱状態
E_SC_RESULT SC_SHARE_GetDeviationStatus(SC_DH_SHARE_DEVIATION_STATUS *status);
E_SC_RESULT SC_SHARE_SetDeviationStatus(const SC_DH_SHARE_DEVIATION_STATUS *status);
// 音声ＴＴＳ情報
E_SC_RESULT SC_SHARE_GetVoiceTTS(SC_DH_SHARE_VOICE_TTS *voice);
E_SC_RESULT SC_SHARE_SetVoiceTTS(const SC_DH_SHARE_VOICE_TTS *voice);
// ターンリスト情報
E_SC_RESULT SC_SHARE_GetTurnListInfo(SC_DH_SHARE_TURN_LIST *turnList);
E_SC_RESULT SC_SHARE_SetTurnListInfo(const SC_DH_SHARE_TURN_LIST *turnList);
// 経路総距離
E_SC_RESULT SC_SHARE_GetRouteLength(SC_DH_SHARE_ROUTE_LENGTH *route);
E_SC_RESULT SC_SHARE_SetRouteLength(const SC_DH_SHARE_ROUTE_LENGTH *route);
// 経路所要時間
E_SC_RESULT SC_SHARE_GetRouteAveTimne(SC_DH_SHARE_ROUTE_AVETIME *route);
E_SC_RESULT SC_SHARE_SetRouteAveTimne(const SC_DH_SHARE_ROUTE_AVETIME *route);
// 経路高速距離
E_SC_RESULT SC_SHARE_GetRouteHwayLength(SC_DH_SHARE_ROUTE_HWAYLENGTH *route);
E_SC_RESULT SC_SHARE_SetRouteHwayLength(const SC_DH_SHARE_ROUTE_HWAYLENGTH *route);
// 経路有料距離
E_SC_RESULT SC_SHARE_GetRouteTollLength(SC_DH_SHARE_ROUTE_TOLLLENGTH *rotue);
E_SC_RESULT SC_SHARE_SetRouteTollLength(INT32 tolllength);
// 経路料金
E_SC_RESULT SC_SHARE_GetRouteTollFee(SC_DH_SHARE_ROUTE_TOLLFEE *rotue);
E_SC_RESULT SC_SHARE_SetRouteTollFee(INT32 tollfee);
// 車両状態情報(実ロケーション)
E_SC_RESULT SC_SHARE_GetCarStateReal(SC_DH_SHARE_CARSTATE *state);
E_SC_RESULT SC_SHARE_SetCarStateReal(const SC_DH_SHARE_CARSTATE *state);
// 車両状態情報(シミュレータロケーション)
E_SC_RESULT SC_SHARE_GetCarStateSimu(SC_DH_SHARE_CARSTATE *state);
E_SC_RESULT SC_SHARE_SetCarStateSimu(const SC_DH_SHARE_CARSTATE *state);
// 交差点拡大図サイズ
E_SC_RESULT SC_SHARE_GetDynamicGraphiSize(SC_DH_SHARE_DYNAMICGRAPHISIZE *graphisize);
E_SC_RESULT SC_SHARE_SetDynamicGraphiSize(const SC_DH_SHARE_DYNAMICGRAPHISIZE *graphisize);
// 交差点拡大図有無情報
E_SC_RESULT SC_SHARE_GetDynamicGraphiStat(SC_DH_SHARE_DYNAMICGRAPHICSTATUS *status);
E_SC_RESULT SC_SHARE_SetDynamicGraphiStat(const SC_DH_SHARE_DYNAMICGRAPHICSTATUS *status);
// 交差点拡大図情報
E_SC_RESULT SC_SHARE_GetDynamicGraphiBitmap(SC_DH_SHARE_DYNAMICGRAPHIBITMAP *bitmap);
E_SC_RESULT SC_SHARE_SetDynamicGraphiBitmap(const SC_DH_SHARE_DYNAMICGRAPHIBITMAP *bitmap);
// ユーザ定義アイコンのリソースと設定ファイルの格納パス
E_SC_RESULT SC_SHARE_GetUDIResourcePath(SC_DH_SHARE_ICON_RESOURCE_PATH *resource);
E_SC_RESULT SC_SHARE_SetUDIResourcePath(const SC_DH_SHARE_ICON_RESOURCE_PATH *resource);
// ユーザ定義ダイナミックアイコンデータ
E_SC_RESULT SC_SHARE_GetIconInfo(SC_DH_SHARE_MAPDYNUDI *iconInfo);
E_SC_RESULT SC_SHARE_SetIconInfo(const SC_DH_SHARE_MAPDYNUDI *iconInfo);
// ユーザ定義ダイナミックアイコンデータの表示/非表示
E_SC_RESULT SC_SHARE_GetDynamicUDIDisplay(SC_DH_SHARE_MAPDYNUDI *dispInfo);
E_SC_RESULT SC_SHARE_SetDynamicUDIDisplay(const SC_DH_SHARE_MAPDYNUDI *dispInfo);
// デモモード
E_SC_RESULT SC_SHARE_GetDemoMode(SC_DH_SHARE_DEMOMODE *demo);
E_SC_RESULT SC_SHARE_SetDemoMode(const SC_DH_SHARE_DEMOMODE *demo);
// 経路コスト情報
E_SC_RESULT SC_SHARE_GetRouteCostInfo(SC_DH_SHARE_ROUTECOSTINFO *rtcost);
E_SC_RESULT SC_SHARE_SetRouteCostInfo(const SC_DH_SHARE_ROUTECOSTINFO *rtcost);

// 車両タイプ
E_SC_RESULT SC_SHARE_GetVehicleType(SC_DH_SHARE_VEHICLETYPE *vehicleType);
E_SC_RESULT SC_SHARE_SetVehicleType(const SC_DH_SHARE_VEHICLETYPE *vehicleType);

// 走行軌跡を保存するかどうか
E_SC_RESULT SC_SHARE_GetSaveTracksFlag(SC_DH_SHARE_SAVETRACKS *isSavetracks);
E_SC_RESULT SC_SHARE_SetSaveTracksFlag(const SC_DH_SHARE_SAVETRACKS *isSavetracks);

// デバッグ情報を描画するかどうか
E_SC_RESULT SC_SHARE_GetDebugInfoOnSurface(SC_DH_SHARE_ONSURFACE *isOnSurface);
E_SC_RESULT SC_SHARE_SetDebugInfoOnSurface(const SC_DH_SHARE_ONSURFACE *isOnSurface);

// 位置情報共有するかどうか
E_SC_RESULT SC_SHARE_GetEchoFlag(SC_DH_SHARE_ECHO *isEcho);
E_SC_RESULT SC_SHARE_SetEchoFlag(const SC_DH_SHARE_ECHO *isEcho);

// ジャンルデータ
E_SC_RESULT SC_SHARE_GetGenreData(SC_DH_SHARE_GENREDATA *rtcost);
E_SC_RESULT SC_SHARE_SetGenreData(const SC_DH_SHARE_GENREDATA *rtcost);

// 地図ズームモード
E_SC_RESULT SC_SHARE_GetZoomMode(SC_DH_SHARE_ZOOMMODE *mode);
E_SC_RESULT SC_SHARE_SetZoomMode(const SC_DH_SHARE_ZOOMMODE *mode);

// リージョン
E_SC_RESULT SC_SHARE_GetRegion(SC_DH_SHARE_REGION *region);
E_SC_RESULT SC_SHARE_SetRegion(const SC_DH_SHARE_REGION *region);

// 言語
E_SC_RESULT SC_SHARE_GetLanguage(SC_DH_SHARE_LANGUAGE *language);
E_SC_RESULT SC_SHARE_SetLanguage(const SC_DH_SHARE_LANGUAGE *language);

// 経路バックアップ
E_SC_RESULT SC_RouteBackup();
E_SC_RESULT SC_RouteBackupDelete();
E_SC_RESULT SC_GetRouteBackup(SMROUTEBACKUP*);

// マッピングアラート情報
E_SC_RESULT SC_SHARE_GetMappingAlert(SC_DH_SHARE_MAPPINGALERT *mappinAalert);
E_SC_RESULT SC_SHARE_SetMappingAlert(const SC_DH_SHARE_MAPPINGALERT *mappinAalert);

// 交通情報
E_SC_RESULT SC_SHARE_GetTraffic(SC_DH_SHARE_TRAFFIC *traffic);
E_SC_RESULT SC_SHARE_SetTraffic(const SC_DH_SHARE_TRAFFIC *traffic);

// 全データ出力（デバッグ用）
void SC_SHARE_OutputAllData();

#endif // #ifndef SMCORE_SHARE_DATA_H
