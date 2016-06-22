/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCOREDATAHANDLER_H_
#define SMCOREDATAHANDLER_H_

//-----------------------------------
// 列挙型定義
//-----------------------------------
// 常駐メモリのデータ識別ID
typedef enum _E_SC_DH_SHARE_DATA_ID {
	e_SC_DH_SHARE_SCALELEVEL = 0,			// 地図表示縮尺(スケール)
	e_SC_DH_SHARE_BIGTEXTATTR,				// 地図上文言拡大表示
	e_SC_DH_SHARE_BIGICONATTR,				// 地図上表示アイコン拡大表示
	e_SC_DH_SHARE_SHOWLANDMARK,				// ランドマーク表示設定
	e_SC_DH_SHARE_SHOWLANDMARKATTR,			// ランドマーク表示属性設定
	e_SC_DH_SHARE_DISPMODE, 				// 地図表示モード
	e_SC_DH_SHARE_DRIVERMODE, 				// 地図ドライバモード
	e_SC_DH_SHARE_SHOWCURSOR, 				// 地図中心カーソルの表示ステータス
	e_SC_DH_SHARE_CARSTATE,					// 車両状態情報
	e_SC_DH_SHARE_SCROLLMODE,				// 地図スクロールモード
	e_SC_DH_SHARE_MOVEMAPDIR,				// 地図の移動情報
	e_SC_DH_SHARE_ZOOMSTEPRATE,				// フリーズーム時の地図拡大比例
	e_SC_DH_SHARE_MAPCURSORCOORD,			// 地図中心の地理座標
	e_SC_DH_SHARE_MAPVIEWPORT,				// 地図のビューポート
	e_SC_DH_SHARE_MAPROTATE,				// 地図の回転角度
	e_SC_DH_SHARE_GEODISTANCE,				// 地図スケール
	e_SC_DH_SHARE_RESOLUTION,				// 解像度
	e_SC_DH_SHARE_RPPOINT,					// 地点情報（出発地、経由地、目的地）
	e_SC_DH_SHARE_EXISTROUTE,				// 探索結果の有無
	e_SC_DH_SHARE_PLANNING,					// 探索条件有無
	e_SC_DH_SHARE_RPOPTION,					// 探索条件
	e_SC_DH_SHARE_RPTIP,					// 探索のエラー情報
	e_SC_DH_SHARE_DELROUTERESULT,			// 探索結果削除
	e_SC_DH_SHARE_SIMULATE,					// シミュレータ環境かどうか
	e_SC_DH_SHARE_SIMULATESPEED,			// シミュレーション速度
	e_SC_DH_SHARE_SIMULATESTATE,			// シミュレーション状態
	e_SC_DH_SHARE_REPLAN,					// リルート条件閾値
	e_SC_DH_SHARE_GUIDEREALTIMEINFO,		// リアルタイム案内情報
	e_SC_DH_SHARE_GUIDESTATUS,				// 誘導状態
	e_SC_DH_SHARE_VOICETTS,					// 音声ＴＴＳ
	e_SC_DH_SHARE_TURNLIST,					// ターンリスト情報
	e_SC_DH_SHARE_ROUTELENGTH,				// 経路総距離
	e_SC_DH_SHARE_ROUTEAVETIME,				// 経路所要時間
	e_SC_DH_SHARE_ROUTEHWAYLENGTH,			// 経路高速道路総距離
	e_SC_DH_SHARE_ROUTETOLLLENGTH,			// 経路有料道路総距離
	e_SC_DH_SHARE_ROUTETOLLFEE,				// 経路料金
	e_SC_DH_SHARE_CARSTATE_REAL,			// 車両状態情報(実ロケーション)
	e_SC_DH_SHARE_CARSTATE_SIMU,			// 車両状態情報(シミュレータロケーション)
	e_SC_DH_SHARE_ICONRESOURCEPATH,			// アイコンリソースパス
	e_SC_DH_SHARE_ICONINFO,					// ユーザ定義ダイナミックアイコンデータ
	e_SC_DH_SHARE_ICONDISP,					// ユーザ定義ダイナミックアイコンデータ表示/非表示
	e_SC_DH_SHARE_DEMOMODE,					// デモモード
	e_SC_DH_SHARE_DEVIATIONSTATUS,			// 経路逸脱状態
	e_SC_DH_SHARE_DYNAMICGRAPHISIZE,		// 交差点拡大図サイズ
	e_SC_DH_SHARE_DYNAMICGRAPHISTATUS,		// 交差点拡大図有無情報
	e_SC_DH_SHARE_DYNAMICGRAPHIBITMAP,		// 交差点拡大図情報
	e_SC_DH_SHARE_ROUTECOSTINFO,			// 経路コスト情報
	e_SC_DH_SHARE_VEHICLETYPE,				// 車両タイプ
	e_SC_DH_SHARE_ISSAVETRACKS,				// 走行軌跡を保存するかどうか
	e_SC_DH_SHARE_ISONSURFACE,				// デバッグ情報を描画するかどうか
	e_SC_DH_SHARE_ISECHO,					// 位置情報共有するかどうか
	e_SC_DH_SHARE_GENREDATA,				// ジャンルデータ
	e_SC_DH_SHARE_ZOOMMODE,					// 地図ズームモード
	e_SC_DH_SHARE_REGION,					// リージョン
	e_SC_DH_SHARE_LANGUAGE,					// 言語
	e_SC_DH_SHARE_MAPPINGALERT,				// マッピングアラート情報
	e_SC_DH_SHARE_TRAFFIC,					// 交通情報

	e_SC_DH_SHARE_DATA_ID_END
} E_SC_DH_SHARE_DATA_ID;

//-----------------------------------
// 構造体定義
//-----------------------------------
// 常駐メモリデータ入出力構造体
typedef struct _SC_DH_SHARE_DATA {
	E_SC_DH_SHARE_DATA_ID	dataId;			// [I]データ識別ID
	void					*data;			// [I/O]入力データ格納領域のポインタ
} SC_DH_SHARE_DATA;

#endif // #ifndef SMCOREDATAHANDLER_H_
