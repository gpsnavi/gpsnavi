/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCOREMEM_H_
#define SMCOREMEM_H_

//-----------------------------------
// メモリサイズ定義
//-----------------------------------
#define	SC_MEM_SIZE_DYNAMIC		(1024 * 1024 * 3)	// 動的確保メモリサイズ
#define	SC_MEM_SIZE_SHARE		(1024 * 1024 * 2)	// 常駐メモリサイズ
#define	SC_MEM_SIZE_ROUTEPLAN	(1024 * 1024 * 25)	// 探索メモリサイズ
#define	SC_MEM_SIZE_ROUTECAND	(1024 * 1024 * 10)	// 候補経路メモリサイズ
#define	SC_MEM_SIZE_ROUTEMNG	(1024 * 1024)		// 経路メモリサイズ
#define SC_MEM_SIZE_GUIDETBL	(1024 * 1024 * 4)	// 誘導テーブルメモリサイズ
#define SC_MEM_SIZE_GENREDATA	(1024 * 500)		// ジャンルデータメモリサイズ
#define SC_MEM_SIZE_CASH		(1024 * 1024 * 160)	// キャッシュメモリサイズ
#define	SC_MEM_SIZE_SD			(1024 * 1024 * 2)	// 運転特性診断用動的確保メモリサイズ

//-----------------------------------
// 列挙型定義
//-----------------------------------
/**
 * @brief 動的メモリ利用のカテゴリをあらわす。
 */
typedef enum _E_SC_MEM_TYPE {
    e_MEM_TYPE_DYNAMIC = 0,		// 動的メモリ
    e_MEM_TYPE_SHARE,			// 動的メモリ(常駐メモリ)
    e_MEM_TYPE_ROUTEPLAN,		// 動的メモリ(探索)
    e_MEM_TYPE_ROUTECAND,		// 動的メモリ(探索)
    e_MEM_TYPE_ROUTEMNG,		// 動的メモリ(推奨経路)
    e_MEM_TYPE_GUIDETBL,		// 動的メモリ(誘導テーブル)
    e_MEM_TYPE_GENREDATA,		// 動的メモリ(ジャンルデータ)
    e_MEM_TYPE_CASH,			// キャッシュメモリ
    e_MEM_TYPE_SD,				// 動的メモリ(運転特性診断)
    e_MEM_TYPE_END 				// 終端マーク/
} E_SC_MEM_TYPE;

#endif // #ifndef SMCOREMEM_H_
