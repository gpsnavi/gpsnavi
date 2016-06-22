/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCORE_DEF_H
#define SMCORE_DEF_H


// ナビコアテンポラリディレクトリ
#define		SC_SHAREDATA_FILE_NAME		"scsd.bin"								// 常駐メモリデータ格納ファイル
#define		SC_BACKUP_FILE_NAME			"route_backup.dat"						// 経路バックアップファイル

// DBパス
#define		SC_DB_PATH					"Polaris.db"
#define		SC_LOG_DIR					"/Log/"
#define		SC_TEMP_DIR					"/Temp/"

#ifdef __SMS_APPLE__
#define		SC_MAX_PATH					512										// パス最大長
#else
#define		SC_MAX_PATH					260										// パス最大長
#endif /* __SMS_APPLE__ */
#define		EOS							'\0'									// NULL終端文字

#define		SC_MAP_SCALE_LEVEL_MIN		0	// 地図表示縮尺最大値
#define		SC_MAP_SCALE_LEVEL_MAX		12	// 地図表示縮尺最小値

#define		SC_PAIRING_NUM				5	// ペアリング数

#define		SC_PROBE_DATA_MAX_NUM		(60 * 60 * 24)							// Probeを端末に貯る最大件数

//-----------------------------------
// 共用体定義
//-----------------------------------
// 探索条件
typedef enum _E_ROUTE {
	e_ROUTE_SINGLE = 0,				// 単経路探索
	e_ROUTE_REPLAN,					// 再探索
	e_ROUTE_SINGLE_LV1				// 単経路探索(LV1のみ)
} E_ROUTE;

// 誘導状態
typedef enum _E_GUIDE_STATUS {
	e_GUIDE_STATUS_STOP = 0,		// 経路誘導停止
	e_GUIDE_STATUS_START,			// 経路誘導開始
	e_GUIDE_STATUS_RUN,				// 経路誘導実行
	e_GUIDE_STATUS_PAUSE,			// 経路誘導一時停止
	e_GUIDE_STATUS_RESUME,			// 経路誘導再開
} E_GUIDE_STATUS;

// 経路逸脱状態
typedef enum _E_DEVIATION_STATUS {
	e_DEVIATION_STATUS_OFF = 0,		// 非逸脱状態
	e_DEVIATION_STATUS_ON			// 逸脱状態
} E_DEVIATION_STATUS;

// 交差点拡大図状態
typedef enum _E_DYNAMICGRAPHIC_STATUS {
	e_DYNAMIC_GRAPHIC_NON = 0,		// 交差点拡大図無
	e_DYNAMIC_GRAPHIC_ON			// 交差点拡大図有
} E_DYNAMICGRAPHIC_STATUS;

#endif // #ifndef SMCORE_DEF_H
