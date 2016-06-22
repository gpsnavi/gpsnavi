/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef DHC_MEMMGR_H_
#define DHC_MEMMGR_H_

/*
 * 定数定義
 */

#define _SCDHC_SAVE_USESIZE			(0)						// メモリブロック使用最大値表示コンパイルスイッチ（チューニング用）

#define		DATA_MEM_SIZE			(1024 * 32)				// データ用メモリサイズ(32KB)
// ブロックサイズ
#define		MEM_SIZE_4				(4)						// 4B
#define		MEM_SIZE_15K			(1024 * 15)				// 15KB
#define		MEM_SIZE_100K			(1024 * 100)			// 100KB
#define		MEM_SIZE_200K			(1024 * 200)			// 200KB
#define		MEM_SIZE_400K			(1024 * 400)			// 400KB
#define		MEM_SIZE_500K			(1024 * 500)			// 500KB
#define		MEM_SIZE_1M				(1024 * 1024 * 1)		// 1MB
#define		MEM_SIZE_3M				(1024 * 1024 * 3)		// 3MB
// ブロック数
#define		MEM_VOL_4				128				// 4B
#define		MEM_VOL_15K				128				// 15KB
#define		MEM_VOL_100K			128				// 100KB
#define		MEM_VOL_200K			128				// 200KB
#define		MEM_VOL_400K			24				// 400KB
#define		MEM_VOL_500K			24				// 500KB
#define		MEM_VOL_1M				26				// 1MB
#define		MEM_VOL_3M				20				// 3MB
// 最大値
#define		MEM_SPSIZE_MAX			(1024 * 1024 * 4  + 1024 * 500)		// 地図解凍用メモリ領域4.5MB
#define		MEM_SIZE_MAX			MEM_SIZE_3M		// ブロックサイズMAX
#define		MEM_VOL_MAX				MEM_VOL_3M		// ブロック面数MAX
// バッファ管理情報数（実際の読み込み枚数に影響）
#define		MAX_CASH_CNT			(64)			// Cashリスト数
#define		MAX_READ_CNT			(256)			// Readリスト数
/*
 * 構造体定義
 */
// ★リングバッファ変更時はg_MemSizeInfoとE_CHASH_MEM_BLOCKを変更すること。
typedef enum {
	e_CHASH_MEM_BLOCK1 = 0,
	e_CHASH_MEM_BLOCK2,
	e_CHASH_MEM_BLOCK3,
	e_CHASH_MEM_BLOCK4,
	e_CHASH_MEM_BLOCK5,
	e_CHASH_MEM_BLOCK6,
	e_CHASH_MEM_BLOCK7,
	e_CHASH_MEM_BLOCK8,
	e_CHASH_MEM_END
} E_CHASH_MEM_BLOCK;
typedef struct {
	UINT32 size;
	UINT32 vol;
	UINT32 select;
}T_MemSizeInfo;

#endif /* DHC_MEMMGR_H_ */
