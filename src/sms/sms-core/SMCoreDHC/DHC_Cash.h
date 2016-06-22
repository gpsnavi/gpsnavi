/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef DHC_CASH_H_
#define DHC_CASH_H_


#define DHC_INVALID_DATA			(0xFFFF)			// 先頭アドレス無効値(データなし)
/*-------------------------------------------------------------------
 * マクロ定義
 *-------------------------------------------------------------------*/
/**
 * テーブル初期化
 */
#define DHC_ReadListInit(aReadList)									\
	{																\
		(aReadList)->parcelId = 0;									\
		DHC_BinInfoInit(&((aReadList)->stPclBin.stBinBkgd));		\
		DHC_BinInfoInit(&((aReadList)->stPclBin.stBinBkgdName));	\
		DHC_BinInfoInit(&((aReadList)->stPclBin.stBinCharStr));		\
		DHC_BinInfoInit(&((aReadList)->stPclBin.stBinGuide));		\
		DHC_BinInfoInit(&((aReadList)->stPclBin.stBinName));		\
		DHC_BinInfoInit(&((aReadList)->stPclBin.stBinRoad));		\
		DHC_BinInfoInit(&((aReadList)->stPclBin.stBinRoadName));	\
		DHC_BinInfoInit(&((aReadList)->stPclBin.stBinShape));		\
		DHC_BinInfoInit(&((aReadList)->stPclBin.stBinDensity));		\
		DHC_BinInfoInit(&((aReadList)->stPclBin.stBinMark));		\
		DHC_BinInfoInit(&((aReadList)->stPclBin.stBinParcelBasis));	\
		DHC_BinInfoInit(&((aReadList)->stPclBin.stBinRoadBaseVer));	\
		DHC_BinInfoInit(&((aReadList)->stPclBin.stBinBkgdAreaCls));	\
	}
#define DHC_BinInfoInit(aBinInfo)									\
	{																\
		(aBinInfo)->binDataSize = 0;								\
		(aBinInfo)->bufferSize = 0;									\
		(aBinInfo)->user = 0;										\
		(aBinInfo)->pBufferAddr = NULL;								\
	}
#define DHC_CashInfoInit(aCash)										\
	{																\
		(aCash)->parcelId = 0;										\
		(aCash)->dataKind = 0;										\
		(aCash)->binDataSize = 0;									\
		(aCash)->bufferSize = 0;									\
		(aCash)->pBufferAddr = NULL;								\
	}
/**
 * @brief	Cash 未使用リストInsert
 * @param	T_DHC_CASH_LIST*
 */
#define DHC_UnUseCashListPut(aCash)									\
	{																\
		T_DHC_CASH* cash = &(m_HdlDataMng.cashData);				\
																	\
		if (NULL == cash->firstEmp) {								\
			(aCash)->prev = NULL;									\
			(aCash)->next = NULL;									\
			cash->firstEmp = (aCash);								\
			cash->lastEmp = (aCash);								\
		} else {													\
			(aCash)->prev = cash->lastEmp;							\
			(aCash)->next = NULL;									\
			cash->lastEmp->next = (aCash);							\
			cash->lastEmp = (aCash);								\
		}															\
	}

/**
 * @brief	Read 使用中リストInsert
 * @param	T_DHC_READ_LIST*
 */
#define DHC_UseReadListPut(aRead)									\
	{																\
		T_DHC_READ* read = &(m_HdlDataMng.readData);				\
																	\
		if (NULL == read->firstRead) {								\
			(aRead)->prev = NULL;									\
			(aRead)->next = NULL;									\
			read->firstRead = (aRead);								\
			read->lastRead = (aRead);								\
		} else {													\
			(aRead)->prev = read->lastRead;							\
			(aRead)->next = NULL;									\
			read->lastRead->next = (aRead);							\
			read->lastRead = (aRead);								\
		}															\
	}

/*-------------------------------------------------------------------
 * 便利系
 *-------------------------------------------------------------------*/
// バイト別読み
#define read1byte(p)		*((UINT8*)(p))
#define read2byte(p)		*((UINT16*)(p))
#define read4byte(p)		*((UINT32*)(p))
/*-------------------------------------------------------------------
 * 道路密度バイナリ
 * ●ROAD_DENSITY
 *-------------------------------------------------------------------*/
// オフセット:ROAD_DENSITY
#define _RDEN_BINARY				(4)				// BINARY

// オフセット:ROAD_DENSITY#BINARY
#define _RDEN_L4_DENSITY			(6)				// レベル４密度

// オフセット:ROAD_DENSITY#BINARY
#define _RDEN_DIR					(8)				// DIR

// オフセット:ROAD_DENSITY#BINARY#DIR
#define _DENDIR_LV3					(0)				// レベル３密度情報の先頭位置
#define _DENDIR_LV2					(2)				// レベル２密度情報の先頭位置
#define _DENDIR_LV1					(4)				// レベル１密度情報の先頭位置
#define _DENDIR_AREA_LV3			(8)				// レベル３エリア情報の先頭位置
#define _DENDIR_AREA_LV2			(10)			// レベル２エリア情報の先頭位置
#define _DENDIR_AREA_LV1			(12)			// レベル１エリア情報の先頭位置

// オフセット:ROAD_DENSITY#BINARY#ROAD_DENSITY1～3
#define _DEN_DATA_SIZE				(0)				// データサイズ
#define _DEN_RECORD_VOL				(2)				// 密度情報数
#define _DEN_RECORD					(4)				// 密度データ

// オフセット:ROAD_DENSITY#BINARY#AREA_INFO1
#define _AREA_DATA_SIZE				(0)				// データサイズ
#define _AREA_RECORD_VOL			(2)				// エリア情報数
#define _AREA_RECORD				(4)				// エリアデータ

/*-------------------------------------------------------------------
 * 道路データ：取得系マクロ
 * UINT8* 渡し
 *-------------------------------------------------------------------*/
// ROAD_DENSITY#BINARY#DIR  オフセット取得
#define D_DENBIN_GET_LV3_OFS(p)				(read2byte((p) + _RDEN_DIR + _DENDIR_LV3))
#define D_DENBIN_GET_LV2_OFS(p)				(read2byte((p) + _RDEN_DIR + _DENDIR_LV2))
#define D_DENBIN_GET_LV1_OFS(p)				(read2byte((p) + _RDEN_DIR + _DENDIR_LV1))
#define D_DENBIN_GET_AREA_LV3_OFS(p)		(read2byte((p) + _RDEN_DIR + _DENDIR_AREA_LV3))
#define D_DENBIN_GET_AREA_LV2_OFS(p)		(read2byte((p) + _RDEN_DIR + _DENDIR_AREA_LV2))
#define D_DENBIN_GET_AREA_LV1_OFS(p)		(read2byte((p) + _RDEN_DIR + _DENDIR_AREA_LV1))

// ROAD_DENSITY#BINARY
#define D_DENBIN_GET_RDEN4(p)				(read2byte((p) + _RDEN_L4_DENSITY))
// ROAD_DENSITY#BINARY -> #DIR
#define A_DENBIN_GET_DENDIR(p)				((p) + _RDEN_DIR)
// ROAD_DENSITY#BINARY -> #ROAD_DENSITY3
#define A_DENBIN_GET_RDEN3(p)				((p) + _RDEN_BINARY + (D_DENBIN_GET_LV3_OFS(p) * 4))
// ROAD_DENSITY#BINARY -> #ROAD_DENSITY2
#define A_DENBIN_GET_RDEN2(p)				((p) + _RDEN_BINARY + (D_DENBIN_GET_LV2_OFS(p) * 4))
// ROAD_DENSITY#BINARY -> #ROAD_DENSITY1
#define A_DENBIN_GET_RDEN1(p)				((p) + _RDEN_BINARY + (D_DENBIN_GET_LV1_OFS(p) * 4))
// ROAD_DENSITY#BINARY -> #AREA_INFO1
#define A_DENBIN_GET_AREA1(p)				((p) + _RDEN_BINARY + (D_DENBIN_GET_AREA_LV1_OFS(p) * 4))
// ROAD_DENSITY#BINARY -> #AREA_INFO2
#define A_DENBIN_GET_AREA2(p)				((p) + _RDEN_BINARY + (D_DENBIN_GET_AREA_LV2_OFS(p) * 4))

// レベル３密度情報
// ROAD_DENSITY#BINARY#ROAD_DENSITY3	データサイズ
#define D_DENBIN_GET_RDEN3_RCDSIZE(p)		(read2byte(A_DENBIN_GET_RDEN3(p) + _DEN_DATA_SIZE))
// ROAD_DENSITY#BINARY#ROAD_DENSITY3	密度情報数
#define D_DENBIN_GET_RDEN3_RCDVOL(p)		(A_DENBIN_GET_RDEN3(p) + _DEN_RECORD_VOL)
// ROAD_DENSITY#BINARY#ROAD_DENSITY3	密度データ先頭取得
#define A_DENBIN_GET_DENRCD3(p)				(A_DENBIN_GET_RDEN3(p) + _DEN_RECORD)
// ROAD_DENSITY#BINARY#ROAD_DENSITY3	index指定密度データ取得
#define D_DENRCD3_DENSITY(p,idx)			(read1byte(A_DENBIN_GET_DENRCD3(p) + (idx)))

// レベル２密度情報
// ROAD_DENSITY#BINARY#ROAD_DENSITY2	データサイズ
#define D_DENBIN_GET_RDEN2_RCDSIZE(p)		(read2byte(A_DENBIN_GET_RDEN2(p) + _DEN_DATA_SIZE))
// ROAD_DENSITY#BINARY#ROAD_DENSITY2	密度情報数
#define D_DENBIN_GET_RDEN2_RCDVOL(p)		(A_DENBIN_GET_RDEN2(p) + _DEN_RECORD_VOL)
// ROAD_DENSITY#BINARY#ROAD_DENSITY2	密度データ先頭取得
#define A_DENBIN_GET_DENRCD2(p)				(A_DENBIN_GET_RDEN2(p) + _DEN_RECORD)
// ROAD_DENSITY#BINARY#ROAD_DENSITY2	index指定密度データ取得
#define D_DENRCD2_DENSITY(p,idx)			(read1byte(A_DENBIN_GET_DENRCD2(p) + (idx)))

// レベル１密度情報
// ROAD_DENSITY#BINARY#ROAD_DENSITY1	データサイズ
#define D_DENBIN_GET_RDEN1_RCDSIZE(p)		(read2byte(A_DENBIN_GET_RDEN1(p) + _DEN_DATA_SIZE))
// ROAD_DENSITY#BINARY#ROAD_DENSITY1	密度情報数
#define D_DENBIN_GET_RDEN1_RCDVOL(p)		(A_DENBIN_GET_RDEN1(p) + _DEN_RECORD_VOL)
// ROAD_DENSITY#BINARY#ROAD_DENSITY1	密度データ先頭取得
#define A_DENBIN_GET_DENRCD1(p)				(A_DENBIN_GET_RDEN1(p) + _DEN_RECORD)
// ROAD_DENSITY#BINARY#ROAD_DENSITY1	index指定密度データ取得
#define D_DENRCD1_DENSITY(p,idx)			(read1byte(A_DENBIN_GET_DENRCD1(p) + (idx)))

// レベル1エリア情報
// ROAD_DENSITY#BINARY#AREA_INFO1		データサイズ
#define D_DENBIN_GET_AREA1_RCDSIZE(p)		(read2byte(A_DENBIN_GET_AREA1(p) + _AREA_DATA_SIZE))
// ROAD_DENSITY#BINARY#AREA_INFO1		エリア情報数
#define D_DENBIN_GET_AREA1_RCDVOL(p)		(A_DENBIN_GET_AREA1(p) + _AREA_RECORD_VOL)
// ROAD_DENSITY#BINARY#AREA_INFO1		エリアデータ先頭取得
#define A_DENBIN_GET_AREARCD1(p)			(A_DENBIN_GET_AREA1(p) + _AREA_RECORD)
// ROAD_DENSITY#BINARY#AREA_INFO1		index指定エリア情報の先頭位置を取得
#define A_AREARCD1_AREA(p,idx)				(A_DENBIN_GET_AREARCD1(p) + (SC_DHC_CROSS_AREA_VOL * idx))

// レベル2エリア情報
// ROAD_DENSITY#BINARY#AREA_INFO1		データサイズ
#define D_DENBIN_GET_AREA2_RCDSIZE(p)		(read2byte(A_DENBIN_GET_AREA2(p) + _AREA_DATA_SIZE))
// ROAD_DENSITY#BINARY#AREA_INFO1		エリア情報数
#define D_DENBIN_GET_AREA2_RCDVOL(p)		(A_DENBIN_GET_AREA2(p) + _AREA_RECORD_VOL)
// ROAD_DENSITY#BINARY#AREA_INFO1		エリアデータ先頭取得
#define A_DENBIN_GET_AREARCD2(p)			(A_DENBIN_GET_AREA2(p) + _AREA_RECORD)
// ROAD_DENSITY#BINARY#AREA_INFO1		index指定エリア情報の先頭位置を取得
#define A_AREARCD2_AREA(p,idx)				(A_DENBIN_GET_AREARCD2(p) + (SC_DHC_CROSS_AREA_VOL * idx))

/*-------------------------------------------------------------------
 * 構造体定義
 *-------------------------------------------------------------------*/
// キャッシュリスト
typedef struct _DHC_CASH_LIST {
	UINT32 parcelId;					// パーセルID
	INT32 dataKind;						// データ種別
	UINT32 bufferSize;					// バッファサイズ
	UINT8* pBufferAddr;					// バッファアドレス
	UINT32 binDataSize;					// バイナリサイズ
	struct _DHC_CASH_LIST* prev;		// 前リスト
	struct _DHC_CASH_LIST* next;		// 後リスト
} T_DHC_CASH_LIST;
// キャッシュリスト管理
typedef struct {
	T_DHC_CASH_LIST* firstCash;			// キャッシュ先頭
	T_DHC_CASH_LIST* lastCash;			// キャッシュ末端
	T_DHC_CASH_LIST* firstEmp;			// 空リスト先頭
	T_DHC_CASH_LIST* lastEmp;			// 空リスト末端
	T_DHC_CASH_LIST* list;				// リストテーブル領域アドレス
	UINT32 nowSize;						// キャッシュサイズ
	UINT32 maxSize;						// キャッシュ最大サイズ
} T_DHC_CASH;
// バイナリデータ
typedef struct {
	UINT32 bufferSize;					// バッファサイズ
	UINT8* pBufferAddr;					// バッファアドレス
	UINT32 binDataSize;					// バイナリデータ
	UINT16 user;						// 使用ユーザ
} T_DHC_BINARY;
// パーセル内バイナリデータ管理
typedef struct {
	T_DHC_BINARY stBinRoad;				// 道路データ
	T_DHC_BINARY stBinBkgd;				// 背景データ
	T_DHC_BINARY stBinName;				// 名称データ
	T_DHC_BINARY stBinRoadName;			// 道路名称データ
	T_DHC_BINARY stBinBkgdName;			// 背景名称データ
	T_DHC_BINARY stBinGuide;			// 誘導データ
	T_DHC_BINARY stBinCharStr;			// 文言データ
	T_DHC_BINARY stBinShape;			// 形状データ
	T_DHC_BINARY stBinDensity;			// 密度データ
	T_DHC_BINARY stBinMark;				// 記号背景データ
	T_DHC_BINARY stBinParcelBasis;		// パーセル基本情報データ
	T_DHC_BINARY stBinRoadBaseVer;		// 道路系ベースバージョンデータ
	T_DHC_BINARY stBinBkgdAreaCls;		// 地域クラス背景データ
} T_DHC_PARCEL_BIN;
// 読み込み中パーセルデータ管理
typedef struct _DHC_READ_LIST {
	UINT32 parcelId;					// パーセルID
	T_DHC_PARCEL_BIN stPclBin;			// パーセル内バイナリデータ管理
	struct _DHC_READ_LIST* prev;		// 前リスト
	struct _DHC_READ_LIST* next;		// 後リスト
} T_DHC_READ_LIST;
// 読み込み中リスト管理
typedef struct {
	T_DHC_READ_LIST* firstRead;			// 読み込み先頭
	T_DHC_READ_LIST* lastRead;			// 読み込み末端
	T_DHC_READ_LIST* firstEmp;			// 空リスト先頭
	T_DHC_READ_LIST* lastEmp;			// 空リスト末端
	T_DHC_READ_LIST* list;				// リストテーブル領域アドレス
	UINT32 nowSize;						// 読み込みサイズ
	UINT32 maxSize;						// 読み込み最大サイズ
} T_DHC_READ;
// データマネージャ
typedef struct {
	T_DHC_READ readData;				// 読み込み中データ
	T_DHC_CASH cashData;				// キャッシュデータ
} T_DHC_MAPDATA_MNG;
typedef struct {
	UINT32 parcelId;					// パーセルID
	UINT32 level4ParcelId;				// レベル４のパーセルID
} T_DHC_AREA_PARCEL_LIST;
typedef struct {
	UINT32 parcelId;					// パーセルID
	UINT32 underLevelParcelId;			// 下位レベルのパーセルID
	UINT8* density_p;					// 密度データ
} T_DHC_LEVEL4_PARCEL_LIST;
// ボリューム情報	4byte
typedef union {
	struct{
		UINT32	noncomp_size	: 29;	// bit 28～ 0 - バイナリデータ圧縮前サイズ
		UINT32	comp_form		:  3;	// bit 31～29 - バイナリデータ圧縮方式
	} b;
	UINT32 d;
} T_DHC_VOLUM_INFO;

#endif /* DHC_CASH_H_ */
