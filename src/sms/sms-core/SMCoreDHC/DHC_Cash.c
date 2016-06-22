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
 * 地図データ読み込み兼キャッシュ機能
 */

#include "SMCoreDHCInternal.h"

#define DHC_DEBUG_ON
#undef DHC_DEBUG_ON
#define MUTEX_USE

#ifdef DHC_DEBUG_ON
#define DHC_LOG_DebugPrintStart()	SC_LOG_DebugPrint(SC_TAG_DHC, SC_LOG_START)
#define DHC_LOG_DebugPrintEnd()		SC_LOG_DebugPrint(SC_TAG_DHC, SC_LOG_END)
#else
#define DHC_LOG_DebugPrintStart()
#define DHC_LOG_DebugPrintEnd()
#endif

/*-------------------------------------------------------------------
 * 定数定義
 *-------------------------------------------------------------------*/
// レベル４パーセル内における各レベルのパーセル数
#define SC_L1_PARCEL_CNT	(64)								// レベル１パーセル数(横)
#define SC_L2_PARCEL_CNT	(16)								// レベル２パーセル数(横)
#define SC_L3_PARCEL_CNT	(4)									// レベル３パーセル数(横)
// 圧縮方式
typedef enum {
	UN_COMP = 0,												// 圧縮：非圧縮
	ZIP_COMP,													// 圧縮：ZIP
	LZ4_COMP													// 圧縮：LZ4
} DHC_COMP;

/*-------------------------------------------------------------------
 * 変数定義
 *-------------------------------------------------------------------*/
static SC_MUTEX mutex = SC_MUTEX_INITIALIZER;
static T_DHC_MAPDATA_MNG m_HdlDataMng = {};
static T_DHC_BINARY m_sperBin = {};
static T_DHC_BINARY m_tmpBin = {};
extern UINT32 g_MaxCashSize;
extern UINT32 g_MaxReadSize;

/*-------------------------------------------------------------------
 * プロトタイプ
 *-------------------------------------------------------------------*/
T_DHC_BINARY* DHC_ReadDataGet(UINT32 aKind, T_DHC_READ_LIST* aRead);
T_DHC_READ_LIST* DHC_ReadListSearch(T_DHC_REQ_INFO* aReq);
T_DHC_CASH_LIST* DHC_CashListSearch(T_DHC_REQ_INFO* aReq);
T_DHC_READ_LIST* DHC_UnUseReadListDelete();
INT32 DHC_UseReadListDelete(T_DHC_READ_LIST* aRead);
INT32 DHC_UseCashListInsert(T_DHC_REQ_INFO* aReq, T_DHC_READ_LIST* aRead);
INT32 DHC_UseCashListDeletePoint(T_DHC_CASH_LIST* aCash);
INT32 DHC_UseCashListDeleteSize(UINT32 size);
E_DHC_CASH_RESULT DHC_GetPclDataBin(T_DHC_REQ_PARCEL_INFO* aReqInfo, T_DHC_BINARY* aBinInfo);
INT32 InflateBinary(UINT8* compbuf, UINT32 compsize, UINT8* uncompbuf, UINT32 uncompsize);
void DHC_GetPclDataBinInfo(UINT8 kind, UINT8* bufferAddr, UINT32* size, UINT8** addr);
void* DHC_GetDataBin(UINT8 kind, T_DHC_RES_PARCEL_BIN *resPcl);

/**
 * @brief	キャッシュ管理初期処理
 * @return	正常終了：e_SC_RESULT_SUCCESS
 * @return	異常終了：e_SC_RESULT_MALLOC_ERR以外
 */
E_SC_RESULT SC_DHC_CashInit()
{
	DHC_LOG_DebugPrintStart();

	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	UINT32 i;
	T_DHC_READ_LIST* readList;
	T_DHC_READ_LIST* readPrev;
	T_DHC_READ_LIST* readNext;
	T_DHC_CASH_LIST* cashList;
	T_DHC_CASH_LIST* cashPrev;
	T_DHC_CASH_LIST* cashNext;

	// 一時領域を確保
	m_sperBin.pBufferAddr = SC_MEM_Alloc(MEM_SPSIZE_MAX, e_MEM_TYPE_CASH);
	m_sperBin.bufferSize = MEM_SPSIZE_MAX;
	m_sperBin.binDataSize = 0;
	m_sperBin.user = 0;
	if (NULL == m_sperBin.pBufferAddr) {
		return (e_SC_RESULT_MALLOC_ERR);
	}
	m_tmpBin.pBufferAddr = SC_MEM_Alloc(MEM_SIZE_MAX, e_MEM_TYPE_CASH);
	m_tmpBin.bufferSize = MEM_SIZE_MAX;
	m_tmpBin.binDataSize = 0;
	m_tmpBin.user = 0;
	if (NULL == m_tmpBin.pBufferAddr) {
		return (e_SC_RESULT_MALLOC_ERR);
	}

#ifdef MUTEX_USE
	// Mutex生成
	ret = SC_CreateMutex(&mutex);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_DHC, "SC_CreateMutext error, " HERE);
		return (ret);
	}
#endif
	// MemMgr初期処理(リングバッファが領域確保)
	ret = DHC_MemMgrInit();
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_DHC, "DHC_MemMgrInit error, " HERE);
		return (ret);
	}

	// リングバッファと別の領域確保
	SC_LOG_DebugPrint(SC_TAG_DHC, "Cash CashList MemAlloc size[%d]"HERE, (sizeof(T_DHC_CASH_LIST) * MAX_CASH_CNT));
	SC_LOG_DebugPrint(SC_TAG_DHC, "Cash ReadList MemAlloc size[%d]"HERE, (sizeof(T_DHC_READ_LIST) * MAX_READ_CNT));
	m_HdlDataMng.readData.list = (T_DHC_READ_LIST*) SC_MEM_Alloc(
			(sizeof(T_DHC_READ_LIST) * MAX_READ_CNT),e_MEM_TYPE_CASH);
	m_HdlDataMng.cashData.list = (T_DHC_CASH_LIST*) SC_MEM_Alloc(
			(sizeof(T_DHC_CASH_LIST) * MAX_CASH_CNT),e_MEM_TYPE_CASH);
	if ((NULL == m_HdlDataMng.readData.list) || (NULL == m_HdlDataMng.cashData.list)) {
		SC_LOG_ErrorPrint(SC_TAG_DHC, "CashList or ReadList Alloc Error, " HERE);
		return (e_SC_RESULT_MALLOC_ERR);
	}

	// ■ Read
	readList = m_HdlDataMng.readData.list;
	readPrev = NULL;
	readNext = readList + 1;

	m_HdlDataMng.readData.firstRead = NULL;
	m_HdlDataMng.readData.lastRead = NULL;
	m_HdlDataMng.readData.firstEmp = readList;
	m_HdlDataMng.readData.lastEmp = NULL;
	m_HdlDataMng.readData.nowSize = 0;
	m_HdlDataMng.readData.maxSize = g_MaxReadSize;

	for (i = 0; i < (MAX_READ_CNT - 1); i++) {
		readList->prev = readPrev;
		readList->next = readNext;
		// 初期化
		DHC_ReadListInit(readList);
		readPrev = readList;
		readList = readList + 1;
		readNext = readList + 1;
	}
	// 最後の項目だけ別処理
	readList->prev = readPrev;
	readList->next = NULL;
	DHC_ReadListInit(readList);
	m_HdlDataMng.readData.lastEmp = readList;

	// ■ Cash
	cashList = m_HdlDataMng.cashData.list;
	cashPrev = NULL;
	cashNext = cashList + 1;

	m_HdlDataMng.cashData.firstCash = NULL;
	m_HdlDataMng.cashData.lastCash = NULL;
	m_HdlDataMng.cashData.firstEmp = cashList;
	m_HdlDataMng.cashData.lastEmp = NULL;
	m_HdlDataMng.cashData.nowSize = 0;
	m_HdlDataMng.cashData.maxSize = g_MaxCashSize;

	for (i = 0; i < (MAX_CASH_CNT - 1); i++) {
		cashList->prev = cashPrev;
		cashList->next = cashNext;
		// 初期化
		DHC_CashInfoInit(cashList);
		cashPrev = cashList;
		cashList = cashList + 1;
		cashNext = cashList + 1;
	}
	// 最後の項目だけ別処理
	cashList->prev = cashPrev;
	cashList->next = NULL;
	DHC_CashInfoInit(cashList);
	m_HdlDataMng.cashData.lastEmp = cashList;

	DHC_LOG_DebugPrintEnd();
	return (ret);
}

/**
 * @brief	キャッシュ管理終了処理
 * @return	正常終了：e_SC_RESULT_SUCCESS
 * @return	異常終了：e_SC_RESULT_MALLOC_ERR以外
 */
E_DHC_CASH_RESULT SC_DHC_CashFinalize()
{

	SC_LOG_DebugPrint(SC_TAG_DHC, SC_LOG_START);

#ifdef MUTEX_USE
	// Mutex生成
	SC_DestroyMutex(&mutex);
#endif

#if _SCDHC_SAVE_USESIZE
	// メモリブロック使用最大数ダンプ
	DHC_ShowMemoryUseVol();
#endif

#if 0	// メモリ開放はMemMng任せ
	// キャッシュ用リスト破棄
	SC_MEM_Free(m_HdlDataMng.readData.list);
	SC_MEM_Free(m_HdlDataMng.cashData.list);

	// 地図読み一時領域破棄
	SC_MEM_Free(m_sperBin.pBufferAddr);

	// MemMgr終了
	MemMgr_Finalize();
#endif

	SC_LOG_DebugPrint(SC_TAG_DHC, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief	地図開放要求
 * @param	[I]要求情報
 */
E_DHC_CASH_RESULT SC_DHC_MapFree(T_DHC_REQ_PARCEL *aReqPcl)
{
	T_DHC_REQ_INFO aReq = {};
	T_DHC_READ* read;
	T_DHC_READ_LIST* readList;
	T_DHC_BINARY* binInfo;
	E_DHC_CASH_RESULT ans = e_DHC_RESULT_CASH_SUCCESS;
	UINT32 i;
	UINT32 kind;
	UINT32 mask;

	DHC_LOG_DebugPrintStart();

	if (NULL == aReqPcl) {
		return (e_DHC_RESULT_CASH_FAIL);
	}

#ifdef MUTEX_USE
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	// 排他制御開始
	ret = SC_LockMutex(&mutex);
	if (ret != e_SC_RESULT_SUCCESS) {
		SC_LOG_ErrorPrint(SC_TAG_MEM, "SC_LockMutext error, " HERE);
		return (e_DHC_RESULT_CASH_FAIL);
	}
#endif

	aReq.user = aReqPcl->user;

	read = &(m_HdlDataMng.readData);

	for (i = 0; i < aReqPcl->parcelNum; i++) {
		aReq.parcelId = aReqPcl->parcelInfo[i].parcelId;

		// 読み込み中テーブルからパーセルID検索
		readList = DHC_ReadListSearch(&aReq);
		if (NULL == readList) {
			SC_LOG_ErrorPrint(SC_TAG_DHC, "Unknown Parcel[0x%08x], " HERE, aReq.parcelId);
			continue;
		}

		for (kind = 0; kind < e_DATA_KIND_END; kind++) {

			mask = SC_DHC_GetKindMask(kind);
			if (mask == 0 || mask != (aReqPcl->parcelInfo[i].mapKind & mask)) {
				continue;
			}

			// データ種別を設定
			aReq.mapKind = kind;

			binInfo = DHC_ReadDataGet(aReq.mapKind, readList);
			if (NULL == binInfo) {
				SC_LOG_ErrorPrint(SC_TAG_DHC, "Unknown Kind[%d], " HERE, aReq.mapKind);
				continue;
			}
			// user削除
			binInfo->user = (binInfo->user & ~(aReq.user));
			if (0 == binInfo->user) {
				// データ参照ユーザ無し
				if (NULL != binInfo->pBufferAddr) {
					// キャッシュへの移行是非
					if ((aReq.mapKind == e_DATA_KIND_ROAD)
					|| (aReq.mapKind == e_DATA_KIND_SHAPE)
					|| (aReq.mapKind == e_DATA_KIND_GUIDE)
					|| (aReq.mapKind == e_DATA_KIND_BKGD)
					|| (aReq.mapKind == e_DATA_KIND_NAME)
					|| (aReq.mapKind == e_DATA_KIND_DENSITY)
					|| (aReq.mapKind == e_DATA_KIND_MARK)
					|| (aReq.mapKind == e_DATA_KIND_ROAD_NAME)
					|| (aReq.mapKind == e_DATA_KIND_PARCEL_BASIS)
					|| (aReq.mapKind == e_DATA_KIND_ROAD_BASE_VERSION)
					|| (aReq.mapKind == e_DATA_KIND_BKGD_AREA_CLS)) {
						// キャッシュ追加→地図開放→終了
						ans = DHC_UseCashListInsert(&aReq, readList);
						if (e_DHC_RESULT_CASH_SUCCESS != ans) {
							// キャッシュへの追加失敗
							DHC_ReleaseBinMemory(binInfo->pBufferAddr);
							SC_LOG_ErrorPrint(SC_TAG_DHC, "Cash Insert Failed, " HERE);
						}
					} else {
						// 地図開放→終了
						DHC_ReleaseBinMemory(binInfo->pBufferAddr);
					}
					if (0 > (read->nowSize - binInfo->bufferSize)) {
						// ログ出力のみ
						read->nowSize = 0;
						SC_LOG_DebugPrint(SC_TAG_DHC, "Read Size Under Zero, " HERE);
					} else {
						read->nowSize -= binInfo->bufferSize;
					}
				} else {
					SC_LOG_ErrorPrint(SC_TAG_DHC, "cash buffer is null. id=%x kind=%d user=%d " HERE, aReq.parcelId,
							aReq.mapKind, aReq.user);
				}
				// 情報初期化
				DHC_BinInfoInit(binInfo);
				// 他のデータが読み込まれていなければ空リストとして削除
				UINT16 user = 0;
				user |= readList->stPclBin.stBinRoad.user;
				user |= readList->stPclBin.stBinBkgd.user;
				user |= readList->stPclBin.stBinName.user;
				user |= readList->stPclBin.stBinRoadName.user;
				user |= readList->stPclBin.stBinBkgdName.user;
				user |= readList->stPclBin.stBinGuide.user;
				user |= readList->stPclBin.stBinCharStr.user;
				user |= readList->stPclBin.stBinShape.user;
				user |= readList->stPclBin.stBinDensity.user;
				user |= readList->stPclBin.stBinMark.user;
				user |= readList->stPclBin.stBinParcelBasis.user;
				user |= readList->stPclBin.stBinRoadBaseVer.user;
				user |= readList->stPclBin.stBinBkgdAreaCls.user;
				if (0 == user) {
					// 読み込み使用リストから削除→空リストへ追加
					DHC_UseReadListDelete(readList);
				}
			}
		}
	}

#ifdef MUTEX_USE
	// 排他制御終了
	ret = SC_UnLockMutex(&mutex);
	if (ret != e_SC_RESULT_SUCCESS) {
		SC_LOG_ErrorPrint(SC_TAG_MEM, "SC_UnLockMutext error, " HERE);
		return (e_DHC_RESULT_CASH_FAIL);
	}
#endif

	DHC_LOG_DebugPrintEnd();
	return (e_DHC_RESULT_CASH_SUCCESS);
}

/**
 * @brief	地図全開放要求
 * @param	[I]要求情報
 * @param	[I]地図開放方法 ユーザ指定：SC_DHC_MAPFREE_USER
 * 			                種別指定  ：SC_DHC_MAPFREE_KIND
 */
E_DHC_CASH_RESULT SC_DHC_MapFreeEx(T_DHC_REQ_INFO aReq, INT32 aType)
{
	T_DHC_READ* read;
	T_DHC_READ_LIST* readList;
	T_DHC_READ_LIST* nextRead;
	T_DHC_BINARY* binInfo;
	T_DHC_REQ_INFO wkReq;
	E_DHC_CASH_RESULT ans = e_DHC_RESULT_CASH_SUCCESS;
	UINT32 kindCnt;
	INT32 kind[e_DATA_KIND_END] = { e_DATA_KIND_ROAD, e_DATA_KIND_SHAPE, e_DATA_KIND_GUIDE, e_DATA_KIND_BKGD,
			e_DATA_KIND_NAME, e_DATA_KIND_ROAD_NAME, e_DATA_KIND_BKGD_NAME, e_DATA_KIND_CHARSTR, e_DATA_KIND_DENSITY,
			e_DATA_KIND_MARK, e_DATA_KIND_PARCEL_BASIS, e_DATA_KIND_ROAD_BASE_VERSION, e_DATA_KIND_BKGD_AREA_CLS };
	UINT32 i;

	DHC_LOG_DebugPrintStart();

#ifdef MUTEX_USE
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	// 排他制御開始
	ret = SC_LockMutex(&mutex);
	if (ret != e_SC_RESULT_SUCCESS) {
		SC_LOG_ErrorPrint(SC_TAG_MEM, "SC_LockMutext error, " HERE);
		return (e_DHC_RESULT_CASH_FAIL);
	}
#endif

	do {
		// 最新情報取得
		read = &(m_HdlDataMng.readData);
		readList = m_HdlDataMng.readData.firstRead;
		nextRead = readList;
		wkReq.user = aReq.user;

		// 開放方法に合わせて設定
		if (SC_DHC_MAPFREE_USER == aType) {
			kindCnt = e_DATA_KIND_END;
		} else if (SC_DHC_MAPFREE_KIND == aType) {
			kindCnt = 1;
			kind[0] = aReq.mapKind;
		} else {
			ans = e_DHC_RESULT_CASH_FAIL;
			break;
		}
		// 全リスト確認
		while (nextRead) {
			readList = nextRead;
			for (i = 0; i < kindCnt; i++) {
				wkReq.mapKind = kind[i];
				binInfo = DHC_ReadDataGet(kind[i], readList);
				if (NULL == binInfo) {
					continue;
				}
				// ユーザ登録なければスルー
				if (0 == (binInfo->user & wkReq.user)) {
					continue;
				}
				// user削除
				binInfo->user = (binInfo->user & ~(wkReq.user));
				if (binInfo->user) {
					continue;
				}

				if (NULL != binInfo->pBufferAddr) {
					// キャッシュへの移行是非
					if ((kind[i] == e_DATA_KIND_ROAD)
					|| (kind[i] == e_DATA_KIND_SHAPE)
					|| (kind[i] == e_DATA_KIND_GUIDE)
					|| (kind[i] == e_DATA_KIND_BKGD)
					|| (kind[i] == e_DATA_KIND_NAME)
					|| (kind[i] == e_DATA_KIND_DENSITY)
					|| (kind[i] == e_DATA_KIND_MARK)
					|| (kind[i] == e_DATA_KIND_ROAD_NAME)
					|| (kind[i] == e_DATA_KIND_PARCEL_BASIS)
					|| (kind[i] == e_DATA_KIND_ROAD_BASE_VERSION)
					|| (kind[i] == e_DATA_KIND_BKGD_AREA_CLS)) {
						// キャッシュ追加→地図開放→終了
						ans = DHC_UseCashListInsert(&wkReq, readList);
						if (e_DHC_RESULT_CASH_SUCCESS != ans) {
							// キャッシュへの追加失敗
							DHC_ReleaseBinMemory(binInfo->pBufferAddr);
							SC_LOG_ErrorPrint(SC_TAG_DHC, "Cash Insert Failed, " HERE);
						}
					} else {
						// 地図開放→終了
						DHC_ReleaseBinMemory(binInfo->pBufferAddr);
					}
					if (0 > (read->nowSize - binInfo->bufferSize)) {
						// ログ出力のみ
						read->nowSize = 0;
						SC_LOG_DebugPrint(SC_TAG_RM, "Read Size Under Zero, " HERE);
					} else {
						read->nowSize -= binInfo->bufferSize;
					}
				} else {
					SC_LOG_ErrorPrint(SC_TAG_DHC, "cash buffer is null. id=%x kind=%d user=%d " HERE, readList->parcelId,
							kind[i], aReq.user);
				}
				// 情報初期化
				DHC_BinInfoInit(binInfo);
			}
			// Next
			nextRead = readList->next;

			// 他のデータが読み込まれていなければ空リストとして削除
			UINT16 user = 0;
			user |= readList->stPclBin.stBinRoad.user;
			user |= readList->stPclBin.stBinBkgd.user;
			user |= readList->stPclBin.stBinName.user;
			user |= readList->stPclBin.stBinRoadName.user;
			user |= readList->stPclBin.stBinBkgdName.user;
			user |= readList->stPclBin.stBinGuide.user;
			user |= readList->stPclBin.stBinCharStr.user;
			user |= readList->stPclBin.stBinShape.user;
			user |= readList->stPclBin.stBinDensity.user;
			user |= readList->stPclBin.stBinMark.user;
			user |= readList->stPclBin.stBinParcelBasis.user;
			user |= readList->stPclBin.stBinRoadBaseVer.user;
			user |= readList->stPclBin.stBinBkgdAreaCls.user;
			if (0 == user) {
				// 読み込み使用リストから削除→からリストへ追加
				DHC_UseReadListDelete(readList);
			}
		}
	} while (0);

#ifdef MUTEX_USE
	// 排他制御終了
	ret = SC_UnLockMutex(&mutex);
	if (ret != e_SC_RESULT_SUCCESS) {
		SC_LOG_ErrorPrint(SC_TAG_MEM, "SC_UnLockMutext error, " HERE);
		return (e_DHC_RESULT_CASH_FAIL);
	}
#endif
	DHC_LOG_DebugPrintEnd();
	return (ans);
}

/**
 * @brief	地図アドレス取得
 * @param	[I]要求情報
 * @param	[O]地図結果
 * @memo	読み込み済みの地図管理テーブルから地図データを検索する。
 */
E_DHC_CASH_RESULT SC_DHC_MapWatch(T_DHC_REQ_INFO aReq, void** aBin)
{
	T_DHC_READ_LIST* readList;
	T_DHC_READ_LIST* nextRead;
	T_DHC_BINARY* binInfo;
	E_DHC_CASH_RESULT ans = e_DHC_RESULT_CASH_SUCCESS;

	DHC_LOG_DebugPrintStart();

#ifdef MUTEX_USE
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	// 排他制御開始
	ret = SC_LockMutex(&mutex);
	if (ret != e_SC_RESULT_SUCCESS) {
		SC_LOG_ErrorPrint(SC_TAG_MEM, "SC_LockMutext error, " HERE);
		return (e_DHC_RESULT_CASH_FAIL);
	}
#endif

	do {
		// 最新情報取得
		readList = m_HdlDataMng.readData.firstRead;
		nextRead = readList;
		*aBin = NULL;

		// 全リスト確認
		while (nextRead) {
			readList = nextRead;
			if (readList->parcelId == aReq.parcelId) {
				binInfo = DHC_ReadDataGet(aReq.mapKind, readList);
				if (NULL != binInfo) {
					// データありでもユーザ無しは提供しない
					if (aReq.user & binInfo->user) {
						// 結果格納
						*aBin = binInfo->pBufferAddr;
						break;
					}
				}
			}
			// Next
			nextRead = readList->next;
		}
	} while (0);

#ifdef MUTEX_USE
	// 排他制御終了
	ret = SC_UnLockMutex(&mutex);
	if (ret != e_SC_RESULT_SUCCESS) {
		SC_LOG_ErrorPrint(SC_TAG_MEM, "SC_UnLockMutext error, " HERE);
		return (e_DHC_RESULT_CASH_FAIL);
	}
#endif
	DHC_LOG_DebugPrintEnd();
	return (ans);
}

/**
 * @brief	地図読み要求
 * @param	[I]要求情報
 * @param	[O]地図データアドレス情報
 */
E_DHC_CASH_RESULT SC_DHC_MapRead(T_DHC_REQ_PARCEL *aReqPcl, T_DHC_RES_DATA *aResData)
{
	T_DHC_REQ_INFO aReq;
	T_DHC_READ* read;
	//T_DHC_READ_LIST addList;
	T_DHC_READ_LIST* readList;
	T_DHC_CASH_LIST* cashList;
	T_DHC_BINARY* binInfo;
	//UINT8* result;
	INT32 ret;
	void** outAddr;
	T_DHC_REQ_PARCEL_INFO workReqInfo;
	UINT32 i;
	UINT32 kind;
	UINT32 mask;
	UINT8* workAddr;

	DHC_LOG_DebugPrintStart();

	if (NULL == aReqPcl) {
		return (e_DHC_RESULT_CASH_FAIL);
	}

#ifdef MUTEX_USE
	E_SC_RESULT sret = e_SC_RESULT_SUCCESS;
	// 排他制御開始
	sret = SC_LockMutex(&mutex);
	if (sret != e_SC_RESULT_SUCCESS) {
		SC_LOG_ErrorPrint(SC_TAG_MEM, "SC_LockMutext error, " HERE);
		return (e_DHC_RESULT_CASH_FAIL);
	}
#endif

	// 返却テーブル初期化
	memset(aResData, 0x00, sizeof(T_DHC_RES_DATA));

	aReq.user = aReqPcl->user;
	aResData->parcelNum = aReqPcl->parcelNum;

	read = &(m_HdlDataMng.readData);

	for (i = 0; i < aReqPcl->parcelNum; i++) {
		aReq.parcelId = aReqPcl->parcelInfo[i].parcelId;

		workReqInfo.parcelId = aReqPcl->parcelInfo[i].parcelId;
		workReqInfo.mapKind = aReqPcl->parcelInfo[i].mapKind;

		aResData->parcelBin[i].parcelId = aReqPcl->parcelInfo[i].parcelId;

		for (kind = 0; kind < e_DATA_KIND_END; kind++) {

			mask = SC_DHC_GetKindMask(kind);
			if (mask == 0 || mask != (workReqInfo.mapKind & mask)) {
				continue;
			}

			outAddr = DHC_GetDataBin(kind, &aResData->parcelBin[i]);

			// データ種別を設定
			aReq.mapKind = kind;

			// 読み込み中テーブルからパーセルID検索
			readList = DHC_ReadListSearch(&aReq);

			if (NULL != readList) {
				binInfo = DHC_ReadDataGet(aReq.mapKind, readList);
				if (NULL == binInfo) {
					SC_LOG_ErrorPrint(SC_TAG_DHC, "Unknown Kind[%d], " HERE, aReq.mapKind);
					continue;
				}
				if (NULL != binInfo->pBufferAddr) {
					if (aReq.user & binInfo->user) {
						// 既読
						*outAddr = binInfo->pBufferAddr;
					} else {
						// ユーザ情報追加
						binInfo->user |= aReq.user;
						*outAddr = binInfo->pBufferAddr;
					}

					// フラグを落とす
					workReqInfo.mapKind = (workReqInfo.mapKind & ~mask);
					continue;
				}
			}
			// キャッシュに登録されない種別は検索しない
			if ((aReq.mapKind == e_DATA_KIND_ROAD) || (aReq.mapKind == e_DATA_KIND_SHAPE)
			||  (aReq.mapKind == e_DATA_KIND_GUIDE)
			||  (aReq.mapKind == e_DATA_KIND_BKGD)
			||  (aReq.mapKind == e_DATA_KIND_NAME)
			||  (aReq.mapKind == e_DATA_KIND_DENSITY)
			||  (aReq.mapKind == e_DATA_KIND_MARK)
			||  (aReq.mapKind == e_DATA_KIND_ROAD_NAME)
			||  (aReq.mapKind == e_DATA_KIND_PARCEL_BASIS)
			||  (aReq.mapKind == e_DATA_KIND_ROAD_BASE_VERSION)
			||  (aReq.mapKind == e_DATA_KIND_BKGD_AREA_CLS)) {
				// キャッシュ検索
				cashList = DHC_CashListSearch(&aReq);
			} else {
				cashList = NULL;
			}
			// キャッシュ復帰
			if (NULL != cashList) {
				// サイズ確認
				if (read->maxSize < (read->nowSize + cashList->bufferSize)) {
					//読みサイズOver
					SC_LOG_ErrorPrint(SC_TAG_DHC, "Read Size Over Max, " HERE);
					continue;
				}
				// ①空リストDelete
				if (NULL == readList) {
					if (NULL == read->lastEmp) {
						// 空リスト無し
						// 読み込み枚数Over
						SC_LOG_ErrorPrint(SC_TAG_DHC, "Read Empty List Non, " HERE);
						continue;
					}
					readList = DHC_UnUseReadListDelete();
					binInfo = DHC_ReadDataGet(aReq.mapKind, readList);
					// ②使用リストPut
					DHC_UseReadListPut(readList);
					readList->parcelId = cashList->parcelId;		// パーセルID
				}

				binInfo->user |= aReq.user;							// ユーザ
				binInfo->binDataSize = cashList->binDataSize;		// データサイズ
				binInfo->bufferSize = cashList->bufferSize;			// バッファサイズ
				binInfo->pBufferAddr = cashList->pBufferAddr;		// データアドレス
				//キャッシュから削除
				ret = DHC_UseCashListDeletePoint(cashList);
				if (e_DHC_RESULT_CASH_SUCCESS != ret) {
					// erro 不整合 削除はできているはず
					// ログ出力のみ
					SC_LOG_DebugPrint(SC_TAG_DHC, "Cash Size UnMatch, " HERE);
				}
				*outAddr = binInfo->pBufferAddr;

				// フラグを落とす
				workReqInfo.mapKind = (workReqInfo.mapKind & ~mask);
				// トータルサイズ加算
				read->nowSize += binInfo->bufferSize;
			}
		}

		// 地図読み
		if (workReqInfo.mapKind > 0) {

			UINT32 size = 0;

			// 一時領域へ地図データを読み込む
			ret = DHC_GetPclDataBin(&workReqInfo, &m_sperBin);
			if (e_DHC_RESULT_CASH_SUCCESS != ret) {
				SC_LOG_ErrorPrint(SC_TAG_DHC, "DHC_GetPclDataBin. error ret=[0x%08x]" HERE, ret);
				if (e_DHC_RESULT_CASH_UNKNOWN_PCL == ret) {
					SC_LOG_ErrorPrint(SC_TAG_DHC, "Unknown Parcel[0x%08x], " HERE, workReqInfo.parcelId);
				}
				continue;
			}

			// データ種別ループ
			for (kind = 0; kind < e_DATA_KIND_END; kind++) {

				mask = SC_DHC_GetKindMask(kind);
				if (mask == 0 || mask != (workReqInfo.mapKind & mask)) {
					continue;
				}

				outAddr = DHC_GetDataBin(kind, &aResData->parcelBin[i]);

				// データ種別を設定
				aReq.mapKind = kind;

				// 読み込み中テーブルからパーセルID検索
				readList = DHC_ReadListSearch(&aReq);
				if (NULL != readList) {
					binInfo = DHC_ReadDataGet(aReq.mapKind, readList);
				}

				// 該当するデータ種別のデータサイズと格納先アドレスを取得
				DHC_GetPclDataBinInfo(kind, m_sperBin.pBufferAddr, &size, &workAddr);
				if (size == 0) {
					continue;
				}

				if (read->maxSize < (read->nowSize + size)) {
					// error
					//読みサイズOver
					SC_LOG_ErrorPrint(SC_TAG_DHC, "Read Size Over Max. " HERE);
					SC_LOG_DebugPrint(SC_TAG_DHC, " max[%d] read[%d] new[%d]" HERE, read->maxSize, read->nowSize, size);
					continue;
				}
				// ①空リストDelete
				if (NULL == readList) {
					if (NULL == read->lastEmp) {
						// 空リスト無し
						// 読み込み枚数Over
						SC_LOG_ErrorPrint(SC_TAG_DHC, "Read Empty List Non, " HERE);
						continue;
					}
					readList = DHC_UnUseReadListDelete();
					binInfo = DHC_ReadDataGet(aReq.mapKind, readList);
					// ②使用リストPut
					DHC_UseReadListPut(readList);
					readList->parcelId = aReq.parcelId;					// パーセルID
				}

				// 地図取得準備
				binInfo->user = aReq.user;													// ユーザ
				binInfo->binDataSize = size;												// サイズ
				binInfo->pBufferAddr = DHC_GetBinMemory(size, &(binInfo->bufferSize));	// Mem 領域確保 + バッファサイズ
				if (NULL == binInfo->pBufferAddr) {
					// リングバッファ取得失敗
					// 注：本来読みサイズMAXで弾いているはず
					// TODO リストを元に戻す
					SC_LOG_DebugPrint(SC_TAG_DHC, "Get Bin size[%d] binBufferSize[%d]" HERE, size, binInfo->bufferSize);
					SC_LOG_ErrorPrint(SC_TAG_DHC, "Get Bin Memory Failed, " HERE);
					continue;
				}
				// 一時領域のデータをリングバッファへコピー
				memcpy(binInfo->pBufferAddr, workAddr, size);
				*outAddr = binInfo->pBufferAddr;

				// トータルサイズ加算
				read->nowSize += binInfo->bufferSize;
			}
		}
	}


#ifdef MUTEX_USE
	// 排他制御終了
	sret = SC_UnLockMutex(&mutex);
	if (sret != e_SC_RESULT_SUCCESS) {
		SC_LOG_ErrorPrint(SC_TAG_MEM, "SC_UnLockMutext error, " HERE);
		return (e_DHC_RESULT_CASH_FAIL);
	}
#endif

	DHC_LOG_DebugPrintEnd();
	return (e_DHC_RESULT_CASH_SUCCESS);
}

/**
 * @brief	使用中からパーセルID検索
 * @memo	不明種別は来ない前提
 */
T_DHC_BINARY* DHC_ReadDataGet(UINT32 aKind, T_DHC_READ_LIST* aRead)
{
	T_DHC_BINARY* binInfo = NULL;

	// パラメータチェック
	if (NULL == aRead) {
		return (binInfo);
	}

	switch (aKind) {
	case e_DATA_KIND_ROAD:
		binInfo = &(aRead->stPclBin.stBinRoad);
		break;
	case e_DATA_KIND_SHAPE:
		binInfo = &(aRead->stPclBin.stBinShape);
		break;
	case e_DATA_KIND_GUIDE:
		binInfo = &(aRead->stPclBin.stBinGuide);
		break;
	case e_DATA_KIND_BKGD:
		binInfo = &(aRead->stPclBin.stBinBkgd);
		break;
	case e_DATA_KIND_NAME:
		binInfo = &(aRead->stPclBin.stBinName);
		break;
	case e_DATA_KIND_ROAD_NAME:
		binInfo = &(aRead->stPclBin.stBinRoadName);
		break;
	case e_DATA_KIND_BKGD_NAME:
		binInfo = &(aRead->stPclBin.stBinBkgdName);
		break;
	case e_DATA_KIND_CHARSTR:
		binInfo = &(aRead->stPclBin.stBinCharStr);
		break;
	case e_DATA_KIND_DENSITY:
		binInfo = &(aRead->stPclBin.stBinDensity);
		break;
	case e_DATA_KIND_MARK:
		binInfo = &(aRead->stPclBin.stBinMark);
		break;
	case e_DATA_KIND_PARCEL_BASIS:
		binInfo = &(aRead->stPclBin.stBinParcelBasis);
		break;
	case e_DATA_KIND_ROAD_BASE_VERSION:
		binInfo = &(aRead->stPclBin.stBinRoadBaseVer);
		break;
	case e_DATA_KIND_BKGD_AREA_CLS:
		binInfo = &(aRead->stPclBin.stBinBkgdAreaCls);
		break;
	default:
		// 不明種別は来ない前提の為、何もしない
		break;
	}
	return (binInfo);
}

/**
 * @brief	使用中からパーセルID検索
 * @param	[I]要求情報
 */
T_DHC_READ_LIST* DHC_ReadListSearch(T_DHC_REQ_INFO* aReq)
{
	T_DHC_READ_LIST* read = m_HdlDataMng.readData.firstRead;
	T_DHC_READ_LIST* nextRead = read;
	//T_DHC_BINARY* binInfo;
	//UINT32 i;

	while (nextRead) {
		read = nextRead;
		if (aReq->parcelId == read->parcelId) {
			return (read);
		}
		nextRead = read->next;
	}
	return (NULL);
}

/**
 * @brief	キャッシュから地図データ検索
 * @param	[I]要求情報
 */
T_DHC_CASH_LIST* DHC_CashListSearch(T_DHC_REQ_INFO* aReq)
{
	T_DHC_CASH_LIST* cash = m_HdlDataMng.cashData.firstCash;
	T_DHC_CASH_LIST* nextCash = cash;

	while (nextCash) {
		cash = nextCash;
		if ((aReq->parcelId == cash->parcelId) && (aReq->mapKind == cash->dataKind)) {
			// 双方一致
			return (cash);
		}
		nextCash = cash->next;
	}
	return (NULL);
}

/**
 * @brief	Read
 * @brief	空リストから読み込み用テーブル取得
 * @param	[O]空リスト
 */
T_DHC_READ_LIST* DHC_UnUseReadListDelete()
{
	T_DHC_READ* read = &(m_HdlDataMng.readData);
	T_DHC_READ_LIST* last = read->lastEmp;

	// 先頭
	if (NULL == last->prev) {
		read->lastEmp = NULL;
		read->firstEmp = NULL;
	}
	// 末端
	else {
		read->lastEmp = last->prev;
		read->lastEmp->next = NULL;
	}
	return (last);
}

/**
 * @brief	Read
 * @brief	指定使用リスト削除および空リスト末尾へ追加
 * @param	[I]aRead:対象の読み中項目
 */
INT32 DHC_UseReadListDelete(T_DHC_READ_LIST* aRead)
{
	T_DHC_READ* read = &(m_HdlDataMng.readData);
	T_DHC_READ_LIST* prev = aRead->prev;
	T_DHC_READ_LIST* next = aRead->next;

	// 末端 && 先頭
	if ((NULL == next) && (NULL == prev)) {
		read->lastRead = NULL;
		read->firstRead = NULL;
	}
	// 末端
	else if (NULL == next) {
		// 使用リスト更新
		prev->next = NULL;
		read->lastRead = prev;
	}
	// 先頭
	else if (NULL == prev) {
		read->firstRead = aRead->next;
		read->firstRead->prev = NULL;
	}
	// 中間
	else {
		prev->next = next;
		next->prev = prev;
	}
	// 未使用リスト更新
	if (NULL == read->firstEmp) {
		aRead->prev = NULL;
		aRead->next = NULL;
		read->firstEmp = aRead;
		read->lastEmp = aRead;
	} else {
		aRead->prev = read->lastEmp;
		aRead->next = NULL;
		read->lastEmp->next = aRead;
		read->lastEmp = aRead;
	}
	// 初期化
	DHC_ReadListInit(aRead);

	return (e_DHC_RESULT_CASH_SUCCESS);
}

/**
 * @brief	Cash
 * @brief	未使用リスト末端から使用リストを追加し読み込みデータの管理情報を追加
 * @param	[I]要求情報
 * @param	[I]読み中リスト
 * @memo	ReadからCashへデータを移す場合に使用。
 */
INT32 DHC_UseCashListInsert(T_DHC_REQ_INFO* aReq, T_DHC_READ_LIST* aRead)
{
	T_DHC_CASH* cash = &(m_HdlDataMng.cashData);
	T_DHC_CASH_LIST* newUse;
	//T_DHC_CASH_LIST* prev;
	//T_DHC_CASH_LIST* next;
	T_DHC_BINARY* binInfo;
	INT32 ret = 0;

	binInfo = DHC_ReadDataGet(aReq->mapKind, aRead);
	if (NULL == binInfo) {
		// 不明地図種別
		return (e_DHC_RESULT_CASH_UNKNOWN_KIND);
	}

	if ((cash->nowSize + binInfo->bufferSize) > cash->maxSize) {
		// 指定サイズの空き作成
		ret = DHC_UseCashListDeleteSize(binInfo->bufferSize);
		if (e_DHC_RESULT_CASH_SUCCESS == ret) {
			// バグ（サイズ異常 テーブル内不整合）
			// TODO ログ出力のみ
		}
	}

	// UnUseCash空きリスト無し
	if (NULL == cash->lastEmp) {
		// 本来はキャッシュリストサイズよりかもキャッシュ可能サイズは小さくするのであり得ないはず？
		if (NULL == cash->firstCash) {
			// バグ（Use，UnUse 双方リストなし）
			return (e_DHC_RESULT_CASH_UNMATCH);
		}
		// 領域開放してからキャッシュ削除
		DHC_ReleaseBinMemory(cash->firstCash->pBufferAddr);
		// 先頭のデータを削除
		ret = DHC_UseCashListDeletePoint(cash->firstCash);
		if (e_DHC_RESULT_CASH_SUCCESS == ret) {
			// バグ（サイズ異常）
			// TODO ログ出力のみ
		}
	}

	// ①空リスト末端から取得
	newUse = cash->lastEmp;
	// 末端 && 先頭
	if (NULL == cash->lastEmp->prev) {
		cash->firstEmp = NULL;
		cash->lastEmp = NULL;
	}
	// 末端
	else {
		cash->lastEmp = newUse->prev;
		cash->lastEmp->next = NULL;
	}

	// ②使用リスト末端へ張り替え
	if (NULL == cash->firstCash) {
		// 使用中リスト初回登録
		newUse->prev = NULL;
		newUse->next = NULL;
		cash->firstCash = newUse;
		cash->lastCash = newUse;
	} else {
		newUse->prev = cash->lastCash;
		newUse->next = NULL;
		cash->lastCash->next = newUse;
		cash->lastCash = newUse;
	}

	// 管理用情報を移す
	cash->lastCash->dataKind = aReq->mapKind;
	cash->lastCash->parcelId = aRead->parcelId;
	cash->lastCash->bufferSize = binInfo->bufferSize;
	cash->lastCash->binDataSize = binInfo->binDataSize;
	cash->lastCash->pBufferAddr = binInfo->pBufferAddr;

	return (e_DHC_RESULT_CASH_SUCCESS);
}

/**
 * @brief	Cash
 * @brief	指定使用リスト削除及び空リスト末尾追加
 * @param	[I]aCash:対象のキャッシュ項目
 * @memo	地図の開放はコール前に行うこと
 */
INT32 DHC_UseCashListDeletePoint(T_DHC_CASH_LIST* aCash)
{
	T_DHC_CASH* cash = &(m_HdlDataMng.cashData);
	T_DHC_CASH_LIST* prev = aCash->prev;
	T_DHC_CASH_LIST* next = aCash->next;
	UINT32 size = aCash->bufferSize;

	// 末端 && 先頭
	if ((NULL == next) && (NULL == prev)) {
		cash->lastCash = NULL;
		cash->firstCash = NULL;
	}
	// 末端
	else if (NULL == next) {
		// 使用リスト更新
		prev->next = NULL;
		cash->lastCash = prev;
	}
	// 先頭
	else if (NULL == prev) {
		cash->firstCash = aCash->next;
		cash->firstCash->prev = NULL;
	}
	// 中間
	else {
		prev->next = next;
		next->prev = prev;
	}
	// 未使用リスト更新
	DHC_UnUseCashListPut(aCash);
	// 初期化
	DHC_CashInfoInit(aCash);

	// 念のためチェック
	if (0 > (cash->nowSize - size)) {
		// error TODO ログ出力のみ
		cash->nowSize = 0;
		return (e_DHC_RESULT_CASH_CASHSIZEMISSMATCH);
	} else {
		cash->nowSize -= size;
	}
	return (e_DHC_RESULT_CASH_SUCCESS);
}

/**
 * @brief	Cash
 * @brief	サイズ指定リスト削除
 * @param	[I]確保サイズ
 * @memo	ReadからCashへのデータ移動時に使用。
 */
INT32 DHC_UseCashListDeleteSize(UINT32 size)
{
	T_DHC_CASH* cash = &(m_HdlDataMng.cashData);
	T_DHC_CASH_LIST* del;

	if (cash->maxSize < size) {
		// error
		return (e_DHC_RESULT_CASH_CASHSIZE);
	}
	while ((cash->maxSize - cash->nowSize) < size) {
		if (NULL == cash->firstCash) {
			// error
			// 恐らくバグ
			return (e_DHC_RESULT_CASH_UNMATCH);
		}
		del = cash->firstCash;
		cash->firstCash = del->next;
		if (NULL == cash->firstCash) {
			// Cash全項目削除
			cash->lastCash = NULL;
		} else {
			cash->firstCash->prev = NULL;
		}

		// 削除したデータを空リスト末端へ追加
		DHC_UnUseCashListPut(del);
		// Mem 領域開放
		DHC_ReleaseBinMemory(del->pBufferAddr);
		// 初期化
		DHC_CashInfoInit(del);

		// サイズ引く
		if (0 > (cash->nowSize - del->bufferSize)) {
			// error TODO ログ出力のみ
			cash->nowSize = 0;
			return (e_DHC_RESULT_CASH_CASHSIZEMISSMATCH);
		} else {
			cash->nowSize -= del->bufferSize;
		}
	}
	return (e_DHC_RESULT_CASH_SUCCESS);
}

/**
 * @brief	サイズ取得
 * @param	[I]aReq:要求内容
 * @param	[O]aSize:サイズ格納先
 * @return	応答ステータス
 * @memo	20140120 外部公開
 */
E_DHC_CASH_RESULT SC_DHC_GetPclDataSize(T_DHC_REQ_INFO* aReq, UINT32* aSize)
{
	T_DAL_PID pstPid;
	SC_DA_RESULT dalRet = SC_DA_RES_FAIL;
	E_DHC_CASH_RESULT ret = e_DHC_RESULT_CASH_SUCCESS;

	if (NULL == aReq) {
		return (e_DHC_RESULT_CASH_FAIL);
	}

	pstPid.parcelId = aReq->parcelId;
	pstPid.divideId = 0;

	// サイズ取得
	switch (aReq->mapKind) {
	case e_DATA_KIND_ROAD:
		dalRet = SC_DA_GetPclRoadDataSize(&pstPid, aSize);
		break;
	case e_DATA_KIND_SHAPE:
		dalRet = SC_DA_GetPclShapeDataSize(&pstPid, aSize);
		break;
	case e_DATA_KIND_GUIDE:
		dalRet = SC_DA_GetPclGuideDataSize(&pstPid, aSize);
		break;
	case e_DATA_KIND_BKGD:
		dalRet = SC_DA_GetPclBkgdDataSize(&pstPid, aSize);
		break;
	case e_DATA_KIND_NAME:
		dalRet = SC_DA_GetPclNameDataSize(&pstPid, aSize);
		break;
	case e_DATA_KIND_ROAD_NAME:
		dalRet = SC_DA_GetPclRoadNameDataSize(&pstPid, aSize);
		break;
	case e_DATA_KIND_BKGD_NAME:
		//dalRet = SC_DA_GetPclxxxDataSize(&pstPid, aSize);
		break;
	case e_DATA_KIND_CHARSTR:
		//dalRet = SC_DA_GetPclxxxDataSize(&pstPid, aSize);
		break;
	case e_DATA_KIND_DENSITY:
		dalRet = SC_DA_GetPclDensityDataSize(&pstPid, aSize);
		break;
	case e_DATA_KIND_MARK:
		dalRet = SC_DA_GetPclMarkDataSize(&pstPid, aSize);
		break;
	case e_DATA_KIND_PARCEL_BASIS:
		dalRet = SC_DA_GetPclBasisDataSize(&pstPid, aSize);
		break;
	case e_DATA_KIND_BKGD_AREA_CLS:
		dalRet = SC_DA_GetPclBkgdAreaClsDataSize(&pstPid, aSize);
		break;
	default:
		// 不明種別は来ない前提の為、何もしない
		break;
	}
	if (SC_DA_RES_SUCCESS != dalRet) {
		ret = e_DHC_RESULT_CASH_FAIL;
	}
	return (ret);
}

/**
 * @brief	地図取得
 * @param	[I]aReqInfo:要求内容
 * @param	[O]aBinInfo:データ格納先
 */
E_DHC_CASH_RESULT DHC_GetPclDataBin(T_DHC_REQ_PARCEL_INFO* aReqInfo, T_DHC_BINARY* aBinInfo)
{

	T_DAL_PID pstPid;
	T_DAL_BINARY dalBin;
	SC_DA_RESULT dalRet = SC_DA_RES_FAIL;
	E_DHC_CASH_RESULT ret = e_DHC_RESULT_CASH_SUCCESS;
	T_DHC_VOLUM_INFO val;
	INT32 inflatesize = 0;
	UINT32 size;
	UINT8 kind;
	UINT8* workAddr;
	UINT8* outAddr;

	// パラメータチェック
	if ((NULL == aReqInfo) || (NULL == aBinInfo)) {
		return (e_DHC_RESULT_CASH_FAIL);
	}
	pstPid.parcelId = aReqInfo->parcelId;
	pstPid.divideId = 0;

	dalBin.bufferSize = m_tmpBin.bufferSize;
	dalBin.pBufferAddr = m_tmpBin.pBufferAddr;
	dalBin.binDataSize = 0;

	// データ取得
	dalRet = SC_DA_GetPclData(&pstPid, aReqInfo->mapKind, &dalBin);
	if (SC_DA_RES_NODATA == dalRet) {
		return (e_DHC_RESULT_CASH_UNKNOWN_PCL);
	}
	else if (SC_DA_RES_SUCCESS != dalRet) {
		return (e_DHC_RESULT_CASH_FAIL);
	}

	// データサイズチェック
	if (0 == dalBin.binDataSize) {
		return (e_DHC_RESULT_CASH_UNKNOWN_PCL);
	}


	outAddr = aBinInfo->pBufferAddr;
	aBinInfo->binDataSize = 0;

	// 解凍処理
	for (kind = 0; kind < e_DATA_KIND_END; kind++) {

		// 該当するデータ種別のデータサイズと格納先アドレスを取得
		DHC_GetPclDataBinInfo(kind, m_tmpBin.pBufferAddr, &size, &workAddr);

		if (0 < size) {
			if	((kind == e_DATA_KIND_ROAD)
				||	(kind == e_DATA_KIND_SHAPE)
				||	(kind == e_DATA_KIND_GUIDE)
				||	(kind == e_DATA_KIND_BKGD)
				||	(kind == e_DATA_KIND_NAME)
				||	(kind == e_DATA_KIND_DENSITY)
				||	(kind == e_DATA_KIND_MARK)
				||	(kind == e_DATA_KIND_ROAD_NAME)
				||	(kind == e_DATA_KIND_BKGD_AREA_CLS)) {

				// ボリューム情報
				val.d = read4byte(workAddr);
				// 圧縮方式のチェック
				if ((UN_COMP != val.b.comp_form) && (ZIP_COMP != val.b.comp_form)) {
					return (e_DHC_RESULT_CASH_UNMATCH);
				}

				// バッファをオーバーしないかチェック
				aBinInfo->binDataSize = aBinInfo->binDataSize + (val.b.noncomp_size*4) + sizeof(size);
				if (aBinInfo->bufferSize < aBinInfo->binDataSize) {
					ret = e_DHC_RESULT_CASH_FAIL;
					break;
				}

				// 圧縮方式によって分岐
				switch (val.b.comp_form) {
				case ZIP_COMP:

					// データ解凍&格納
					inflatesize = InflateBinary((UINT8*)workAddr + sizeof(val.d), size - sizeof(val.d), (UINT8*)outAddr + sizeof(size) + sizeof(val.d), (val.b.noncomp_size*4));
					if (inflatesize != (val.b.noncomp_size*4)) {
						ret = e_DHC_RESULT_CASH_FAIL;
						break;
					}
					// ボリューム情報部分をコピー
					memcpy(outAddr + sizeof(size), workAddr, sizeof(val.d));

					// 解凍後サイズを設定
					size = inflatesize + sizeof(val.d);
					break;
				default:
					// データ格納
					memcpy(outAddr + sizeof(size), workAddr, size);
				}
			}
			else if ((kind == e_DATA_KIND_PARCEL_BASIS)
					|| (kind == e_DATA_KIND_ROAD_BASE_VERSION)) {
				// バッファをオーバーしないかチェック
				aBinInfo->binDataSize = aBinInfo->binDataSize + size + sizeof(size);
				if (aBinInfo->bufferSize < aBinInfo->binDataSize) {
					ret = e_DHC_RESULT_CASH_FAIL;
					break;
				}
				// データ格納
				memcpy(outAddr + sizeof(size), workAddr, size);
			}
		}
		else {
			// バッファをオーバーしないかチェック
			aBinInfo->binDataSize = aBinInfo->binDataSize + sizeof(size);
			if (aBinInfo->bufferSize < aBinInfo->binDataSize) {
				ret = e_DHC_RESULT_CASH_FAIL;
				break;
			}
		}

		// データサイズ格納
		*((UINT32*)outAddr) = size;
		outAddr = outAddr + sizeof(size) + size;

	}
	return (ret);
}

/**
 * @brief	道路密度データ取得
 * @param	[I/O]aDenInfo:道路密度データ
 */
E_DHC_CASH_RESULT SC_DHC_GetRoadDensity(T_DHC_ROAD_DENSITY* aDenInfo)
{

	//void *data = NULL;
	T_DHC_REQ_PARCEL mapReqPcl;
	T_DHC_RES_DATA mapResData = {};
	T_DHC_ROAD_DENSITY_DATA* denData;
	E_DHC_CASH_RESULT ret;

	UINT32	i;
	UINT32	j;
	UINT16	xlp;
	UINT16	ylp;

	INT32	targetLevel;
	UINT32	targetParcelId;
	UINT32	leftParcelId;
	UINT32	areaParcelVol;
	UINT32	level4ParcelId;
	UINT32	level4ParcelVol;
	UINT32	underLevelParcelId;

	UINT32	denIndex;
	INT32	xSftW, ySftW;

	SC_LOG_DebugPrint(SC_TAG_DHC, SC_LOG_START);

	// パラメータチェック
	if (NULL == aDenInfo) {
		return (e_DHC_RESULT_CASH_FAIL);
	}

	denData = aDenInfo->data;

	// 対象となるベースのパーセルIDの、レベルを取得
	targetLevel = SC_MESH_GetLevel(aDenInfo->baseParcelId);

	// 対象エリアのパーセル数
	areaParcelVol = aDenInfo->x * aDenInfo->y;

	// 出力I/Fの初期化
	aDenInfo->totalDensity = 0;

	for(i=0; i<areaParcelVol; i++){
		// 密度
		denData[i].density = 0;

		// エリア
		for(j=0; j<SC_DHC_CROSS_AREA_VOL; j++){
			denData[i].areaId[j] = 0;
		}
	}


	// 対象エリア（x * y範囲）の、個々のパーセルID
	// および対応するレベル４パーセルIDを取得
	T_DHC_AREA_PARCEL_LIST* areaList = (T_DHC_AREA_PARCEL_LIST*) SC_MEM_Alloc(
		(sizeof(T_DHC_AREA_PARCEL_LIST) * areaParcelVol),e_MEM_TYPE_CASH);
	if (NULL == areaList) {
		SC_LOG_ErrorPrint(SC_TAG_DHC, "[Density] Mem Alloc error."HERE);
		return (e_DHC_RESULT_CASH_FAIL);
	}

	leftParcelId = aDenInfo->baseParcelId;
	targetParcelId = leftParcelId;
	i=0;

	for(ylp=0; ylp<aDenInfo->y; ylp++){
		if (ylp != 0) {
			// 対象エリア、一つ上のパーセルIDを取得（初回はスキップ）
			targetParcelId = SC_MESH_GetNextParcelID(leftParcelId, DIR_TOP);
			leftParcelId = targetParcelId;
		}

		for(xlp=0; xlp<aDenInfo->x; xlp++){
			if (xlp != 0) {
				// 対象エリア、一つ右のパーセルIDを取得（初回はスキップ）
				targetParcelId = SC_MESH_GetNextParcelID(targetParcelId, DIR_R);
			}

			// レベル４パーセルIDを取得（レベル４まで繰り返し）
			level4ParcelId = targetParcelId;
			for (j=targetLevel; j<4; j++) {
				level4ParcelId = SC_MESH_GetUpperParcelID(level4ParcelId);
			}

			// 対象エリアのパーセルIDを格納
			areaList[i].parcelId = targetParcelId;

			// 対応したレベル４パーセルIDを格納
			areaList[i].level4ParcelId = level4ParcelId;

			i++;
		}
	}


	// 対象エリアの範囲を包含するレベル４パーセルIDを取得
	SC_MESH_GetAlterPos(areaList[0].level4ParcelId, areaList[areaParcelVol-1].level4ParcelId, 4, &xSftW, &ySftW);
	xSftW += 1;
	ySftW += 1;

	level4ParcelVol = xSftW * ySftW;
	T_DHC_LEVEL4_PARCEL_LIST* level4List = (T_DHC_LEVEL4_PARCEL_LIST*) SC_MEM_Alloc(
		(sizeof(T_DHC_LEVEL4_PARCEL_LIST) * level4ParcelVol),e_MEM_TYPE_CASH);
	if (NULL == level4List) {
		SC_LOG_ErrorPrint(SC_TAG_DHC, "[Density] Mem Alloc error."HERE);
		// ワークテーブル解放
		SC_MEM_Free(areaList, e_MEM_TYPE_CASH);
		return (e_DHC_RESULT_CASH_FAIL);
	}

	targetParcelId = areaList[0].level4ParcelId;
	leftParcelId = targetParcelId;

	i=0;

	// 地図(密度)要求データ初期化
	mapReqPcl.user = SC_DHC_USER_DH;
	mapReqPcl.parcelNum = 0;

	for(ylp=0; ylp<ySftW; ylp++){
		if (ylp != 0) {
			// 一つ上のパーセルIDを取得（初回はスキップ）
			targetParcelId = SC_MESH_GetNextParcelID(leftParcelId, DIR_TOP);
			leftParcelId = targetParcelId;
		}

		for(xlp=0; xlp<xSftW; xlp++){
			if (xlp != 0) {
				// 一つ右のパーセルIDを取得（初回はスキップ）
				targetParcelId = SC_MESH_GetNextParcelID(targetParcelId, DIR_R);
			}

			// 指定されたレベルのパーセルID(左下)を取得
			// レベル４が指定されている場合は、自パーセルIDを設定
			if (4 == targetLevel) {
				underLevelParcelId = targetParcelId;
			} else {
				underLevelParcelId = SC_MESH_GetUnderLevelParcelID(targetParcelId, targetLevel);
			}

			// レベル４パーセルIDを格納
			level4List[i].parcelId = targetParcelId;
			// 指定されたレベルのパーセルID(左下)を格納
			level4List[i].underLevelParcelId = underLevelParcelId;

			// 道路密度バイナリデータ取得条件設定
			mapReqPcl.parcelInfo[i].parcelId = targetParcelId;
			mapReqPcl.parcelInfo[i].mapKind = SC_DHC_GetKindMask(e_DATA_KIND_DENSITY);
			mapReqPcl.parcelNum++;
			i++;
		}
	}

	// 道路密度バイナリデータ取得
	ret = SC_DHC_MapRead(&mapReqPcl, &mapResData);
	if (e_DHC_RESULT_CASH_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_DHC, "[Density] SC_DHC_MapRead. ret(0x%x)" HERE, ret);
		// ワークテーブル解放
		SC_MEM_Free(areaList, e_MEM_TYPE_CASH);
		SC_MEM_Free(level4List, e_MEM_TYPE_CASH);
		return (e_DHC_RESULT_CASH_FAIL);
	}

	// 地図(密度)データのポインタを格納
	for(i=0; i<mapResData.parcelNum; i++){
		level4List[i].density_p = mapResData.parcelBin[i].binDensity;
	}

	// 対象エリア（x * y範囲）の、個々のパーセルの位相位置(Index)を求め
	// 地図(密度)データから密度を取得
	for(i=0; i<areaParcelVol; i++){

		for(j=0; j<level4ParcelVol; j++){

			if(areaList[i].level4ParcelId == level4List[j].parcelId){
				if(level4List[j].density_p != NULL){
					// 左下から対象エリアの指定パーセルまでの相対位置を取得
					SC_MESH_GetAlterPos(level4List[j].underLevelParcelId, areaList[i].parcelId, targetLevel, &xSftW, &ySftW);

					switch (targetLevel) {
					case 1:	// レベル１
						// インデックス
						denIndex = ((SC_L1_PARCEL_CNT * ySftW) + (xSftW));

						// 密度データ
						if(D_DENBIN_GET_LV1_OFS(level4List[j].density_p) != DHC_INVALID_DATA){
							denData[i].density = D_DENRCD1_DENSITY(level4List[j].density_p, denIndex);
						}

						// エリアデータ
						if(D_DENBIN_GET_AREA_LV1_OFS(level4List[j].density_p) != DHC_INVALID_DATA){
							memcpy(denData[i].areaId, A_AREARCD1_AREA(level4List[j].density_p, denIndex), SC_DHC_CROSS_AREA_VOL);
						}
						break;
					case 2:	// レベル２
						// インデックス
						denIndex = ((SC_L2_PARCEL_CNT * ySftW) + (xSftW));

						// 密度データ
						if (D_DENBIN_GET_LV2_OFS(level4List[j].density_p) != DHC_INVALID_DATA) {
							denData[i].density = D_DENRCD2_DENSITY(level4List[j].density_p, denIndex);
						}

						// エリアデータ
						if (D_DENBIN_GET_AREA_LV2_OFS(level4List[j].density_p) != DHC_INVALID_DATA) {
							memcpy(denData[i].areaId, A_AREARCD2_AREA(level4List[j].density_p, denIndex), SC_DHC_CROSS_AREA_VOL);
						}
						break;
					case 3:	// レベル３
						// インデックス
						denIndex = ((SC_L3_PARCEL_CNT * ySftW) + (xSftW));

						// 密度データ
						if(D_DENBIN_GET_LV3_OFS(level4List[j].density_p) != DHC_INVALID_DATA){
							denData[i].density = D_DENRCD3_DENSITY(level4List[j].density_p, denIndex);
						}
						break;
					case 4:	// レベル４
						// 密度データ
						denData[i].density = D_DENBIN_GET_RDEN4(level4List[j].density_p);
						break;
					default:
						// 1～4以外のレベルは来ない前提の為、何もしない
						break;
					}
				}

				// 総密度
				aDenInfo->totalDensity += denData[i].density;
				break;
			}
		}
	}

	// 地図(密度)データの解放準備
	mapReqPcl.parcelNum = 0;
	for (i=0; i<level4ParcelVol; i++) {

		// データの取得が出来たパーセルのみ解放を行う
		if (level4List[i].density_p != NULL) {
			mapReqPcl.parcelInfo[mapReqPcl.parcelNum].parcelId = level4List[i].parcelId;
			mapReqPcl.parcelInfo[mapReqPcl.parcelNum].mapKind = SC_DHC_GetKindMask(e_DATA_KIND_DENSITY);
			mapReqPcl.parcelNum++;
		}
	}

	// 解放処理
	ret = SC_DHC_MapFree(&mapReqPcl);
	if (e_DHC_RESULT_CASH_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_DHC, "[Density] SC_DHC_MapFree. ret(0x%x)"HERE, ret);
		// ワークテーブル解放
		SC_MEM_Free(areaList, e_MEM_TYPE_CASH);
		SC_MEM_Free(level4List, e_MEM_TYPE_CASH);
		return (e_DHC_RESULT_CASH_FAIL);
	}

	// ワークテーブル解放
	SC_MEM_Free(areaList, e_MEM_TYPE_CASH);
	SC_MEM_Free(level4List, e_MEM_TYPE_CASH);

	SC_LOG_DebugPrint(SC_TAG_DHC, SC_LOG_END);
	return (e_DHC_RESULT_CASH_SUCCESS);
}

/**
 * @brief	地図データ解凍処理
 * @param	[I]p_compbuf     :解凍前データポインタ
 * @param	[I]compsize      :解凍前データのサイズ
 * @param	[I/O]p_uncompbuf :解凍後データ格納ポインタ
 * @param	[I]uncompsize    :解凍後データのサイズ
 */
INT32 InflateBinary(UINT8* p_compbuf, UINT32 compsize, UINT8* p_uncompbuf, UINT32 uncompsize)
{
	DHC_LOG_DebugPrintStart();

    /* ===== 変数宣言・初期化 ===== */
	z_stream	st_zlib;
	INT32 zlib_rc = Z_OK;
	INT32 size = 0;

	/* ===== 引数チェック ===== */
    if ((NULL == p_uncompbuf) || (NULL == p_compbuf)) {
        return (size);
    }
    if ((0 == compsize) || (0 == uncompsize)) {
        return (size);
    }

    /* ===== ZLIB初期化処理 ===== */
	st_zlib.zalloc = Z_NULL;
	st_zlib.zfree = Z_NULL;
	st_zlib.opaque = Z_NULL;
	// 初期化
    st_zlib.next_in = Z_NULL;
    st_zlib.avail_in = 0;
    if ((zlib_rc = inflateInit(&st_zlib)) != Z_OK) {
		return (size);
	}

	/* ===== 解凍 ===== */
	st_zlib.next_in = p_compbuf;			// 入力ポインタ
	st_zlib.avail_in = compsize;			// 入力バッファの有効バイト数
	st_zlib.next_out = p_uncompbuf;			// 出力ポインタ
	st_zlib.avail_out = uncompsize;			// 出力バッファ残量
	// zlibで圧縮したデータを解凍する
	if (Z_STREAM_END != inflate(&st_zlib, Z_FINISH)) {
		return (0);
	}
	size = st_zlib.total_out;

    /* ===== ZLIB終了処理 ===== */
	inflateEnd(&st_zlib);

	DHC_LOG_DebugPrintEnd();
	return (size);
}

/**
 * @brief	ダウンロードエリア情報取得
 * @param	[O]ダウンロードエリア情報
 * @param	[I]データ種別
 */
E_DHC_CASH_RESULT SC_DHC_GetDownload_Area(T_DHC_DOWNLOAD_AREA *aDownloadArea, UINT8 kind)
{

	T_DAL_DLAREA downloadArea[M_DHC_DOWNLOAD_AREA_MAX];
	SC_DA_RESULT dalRet;
	UINT8* dataKind;
	UINT8 dataCnt = 0;
	UINT8 i;

	SC_LOG_DebugPrint(SC_TAG_DHC, SC_LOG_START);

	// パラメータチェック
	if (NULL == aDownloadArea) {
		return (e_DHC_RESULT_CASH_FAIL);
	}

	// 出力I/Fの初期化
	for(i=0; i<M_DHC_DOWNLOAD_AREA_MAX; i++){
		aDownloadArea->data[i].download_f = M_DHC_DOWNLOAD_AREA_OFF;
	}

	switch (kind) {
	case M_DHC_DOWNLOAD_AREA_PARCEL:	// パーセルデータ
		dataKind = M_DHC_DLAREA_KIND_PARCEL;
		break;
	case M_DHC_DOWNLOAD_AREA_TRAFFIC:	// 交通情報データ
		dataKind = M_DHC_DLAREA_KIND_TRAFFIC;
		break;
	default:
		SC_LOG_ErrorPrint(SC_TAG_DHC, "SC_DHC_GetDownload_Area. param err[kind]" HERE);
		return (e_DHC_RESULT_CASH_FAIL);
	}

	// ダウンロードエリア情報取得
	dalRet = SC_DA_GetDlAreaMapData(dataKind, downloadArea, &dataCnt);
	if (SC_DA_RES_SUCCESS != dalRet) {
		SC_LOG_ErrorPrint(SC_TAG_DHC, "SC_DA_GetDlAreaMapData. error " HERE);
		return (e_DHC_RESULT_CASH_FAIL);
	}

	// ダウンロードフラグの設定
	for(i=0; i<dataCnt; i++){
		aDownloadArea->data[downloadArea[i].id - 1].download_f = downloadArea[i].downloadFlag;
	}

	SC_LOG_DebugPrint(SC_TAG_DHC, SC_LOG_END);
	return (e_DHC_RESULT_CASH_SUCCESS);
}

/**
 * @brief	ダウンロードエリア名称取得
 * @param	[O]ダウンロードエリア名称情報
 */
E_DHC_CASH_RESULT SC_DHC_GetDownload_AreaName(T_DHC_DOWNLOAD_AREA_NAME *aDownloadAreaName)
{

	T_DAL_DLAREA downloadArea[M_DHC_DOWNLOAD_AREA_MAX];
	SC_DA_RESULT dalRet;
	UINT8 dataCnt = 0;
	UINT8 i;

	SC_LOG_DebugPrint(SC_TAG_DHC, SC_LOG_START);

	// パラメータチェック
	if (NULL == aDownloadAreaName) {
		return (e_DHC_RESULT_CASH_FAIL);
	}

	// 出力I/Fの初期化
	memset(aDownloadAreaName, 0x00, sizeof(T_DHC_DOWNLOAD_AREA_NAME));

	// ダウンロードエリア情報取得
	dalRet = SC_DA_GetDlAreaMapData(M_DHC_DLAREA_KIND_PARCEL, downloadArea, &dataCnt);
	if (SC_DA_RES_SUCCESS != dalRet) {
		SC_LOG_ErrorPrint(SC_TAG_DHC, "SC_DA_GetDlAreaMapData. error " HERE);
		return (e_DHC_RESULT_CASH_FAIL);
	}

	// ダウンロード名称の設定
	for(i=0; i<dataCnt; i++){
		strcpy((char *)(aDownloadAreaName->data[downloadArea[i].id - 1].areaName),
			   (const char *)(downloadArea[i].areaName));
	}

#if 0
	for(i=0; i<M_DHC_DOWNLOAD_AREA_MAX; i++){
		SC_LOG_ErrorPrint(SC_TAG_DHC, "●id=%d name=%s", i+1, aDownloadAreaName->data[i].areaName);
	}
#endif

	SC_LOG_DebugPrint(SC_TAG_DHC, SC_LOG_END);
	return (e_DHC_RESULT_CASH_SUCCESS);
}

/*-------------------------------------------------------------------
 * デバッグ用
 *-------------------------------------------------------------------*/

void DHC_Debug_MemTableAllDump()
{
#ifdef	ANDROID
	__android_log_print(ANDROID_LOG_DEBUG, "CashHDL", "[CASH]%s():start", __FUNCTION__);
#endif	// ANDROID
	DHC_Debug_MemTableReadUseDump();
	DHC_Debug_MemTableReadUnUseDump();
	DHC_Debug_MemTableCashUseDump();
	DHC_Debug_MemTableCashUnUseDump();
}
void DHC_Debug_MemTableReadUnUseDump()
{
	T_DHC_READ* read = &(m_HdlDataMng.readData);
	T_DHC_READ_LIST* readList;
	T_DHC_READ_LIST* readNext;
	UINT32 i;

	// 読み込み中メモリ(未使用)
#ifdef	ANDROID
	__android_log_print(ANDROID_LOG_DEBUG, "CashHDL", "[CASH]%s():--------Read UnUse Dmp--------", __FUNCTION__);
#endif	// ANDROID
	i = 0;
	readList = read->firstEmp;
	readNext = readList;
	while (readNext) {
		readList = readNext;
		if (0 == readList->parcelId)
			break;
#ifdef	ANDROID
		__android_log_print(ANDROID_LOG_DEBUG, "CashHDL",
				"%2d |id=0x%08x user=%x,%x,%x,%x,%x,%x,%x pnt=%p,%p,%p,%p,%p,%p,%p, next=%p"
				,i
				,readList->parcelId
				,readList->stPclBin.stBinRoad.user
				,readList->stPclBin.stBinShape.user
				,readList->stPclBin.stBinGuide.user
				,readList->stPclBin.stBinBkgd.user
				,readList->stPclBin.stBinParcelBasis.user
				,readList->stPclBin.stBinRoadBaseVer.user
				,readList->stPclBin.stBinBkgdAreaCls.user
				,readList->stPclBin.stBinRoad.pBufferAddr
				,readList->stPclBin.stBinShape.pBufferAddr
				,readList->stPclBin.stBinGuide.pBufferAddr
				,readList->stPclBin.stBinBkgd.pBufferAddr
				,readList->stPclBin.stBinParcelBasis.pBufferAddr
				,readList->stPclBin.stBinRoadBaseVer.pBufferAddr
				,readList->stPclBin.stBinBkgdAreaCls.pBufferAddr
				,readList->next
				);
#endif	// ANDROID
		readNext = readList->next;
		i++;
	}
}
void DHC_Debug_MemTableReadUseDump()
{

	T_DHC_READ* read = &(m_HdlDataMng.readData);
	T_DHC_READ_LIST* readList;
	T_DHC_READ_LIST* readNext;
	UINT32 i;

	// 読み込み中メモリ(使用中)
#ifdef	ANDROID
	__android_log_print(ANDROID_LOG_DEBUG, "CashHDL", "[CASH]%s():--------Read Use Dmp--------", __FUNCTION__);
#endif	// ANDROID
	i = 0;
	readList = read->firstRead;
	readNext = readList;
	while (readNext) {
		readList = readNext;
		if (0 == readList->parcelId)
			break;
#ifdef	ANDROID
		__android_log_print(ANDROID_LOG_DEBUG, "CashHDL",
				"%3d |id=0x%08x user=%x,%x,%x,%x,%x,%x,%x pnt=%p,%p,%p,%p,%p,%p,%p, next=%p"
				,i
				,readList->parcelId
				,readList->stPclBin.stBinRoad.user
				,readList->stPclBin.stBinShape.user
				,readList->stPclBin.stBinGuide.user
				,readList->stPclBin.stBinBkgd.user
				,readList->stPclBin.stBinParcelBasis.user
				,readList->stPclBin.stBinRoadBaseVer.user
				,readList->stPclBin.stBinBkgdAreaCls.user
				,readList->stPclBin.stBinRoad.pBufferAddr
				,readList->stPclBin.stBinShape.pBufferAddr
				,readList->stPclBin.stBinGuide.pBufferAddr
				,readList->stPclBin.stBinBkgd.pBufferAddr
				,readList->stPclBin.stBinParcelBasis.pBufferAddr
				,readList->stPclBin.stBinRoadBaseVer.pBufferAddr
				,readList->stPclBin.stBinBkgdAreaCls.pBufferAddr
				,readList->next);
#endif	// ANDROID
		readNext = readList->next;
		i++;
	}
}
void DHC_Debug_MemTableCashUnUseDump()
{
	T_DHC_CASH* cash = &(m_HdlDataMng.cashData);
	T_DHC_CASH_LIST* cashList;
	T_DHC_CASH_LIST* cashNext;
	UINT32 i;
	// キャッシュメモリ(未使用)
#ifdef	ANDROID
	__android_log_print(ANDROID_LOG_DEBUG, "CashHDL", "[CASH]%s():--------Cash UnUse Dmp--------", __FUNCTION__);
#endif	// ANDROID
	i = 0;
	cashList = cash->firstEmp;
	cashNext = cashList;
	while (cashNext) {
		cashList = cashNext;
		if (0 == cashList->parcelId)
			break;
#ifdef	ANDROID
		__android_log_print(ANDROID_LOG_DEBUG, "CashHDL",
				"%3d |id=0x%08x kind=%d pnt=%p buff=%d size=%d next=%p"
				,i
				,cashList->parcelId
				,cashList->dataKind
				,cashList->pBufferAddr
				,cashList->bufferSize
				,cashList->binDataSize
				,cashList->next);
#endif	// ANDROID
		cashNext = cashList->next;
		i++;
	}
}
void DHC_Debug_MemTableCashUseDump()
{
	T_DHC_CASH* cash = &(m_HdlDataMng.cashData);
	T_DHC_CASH_LIST* cashList;
	T_DHC_CASH_LIST* cashNext;
	UINT32 i;

	// キャッシュメモリ(使用中)
#ifdef	ANDROID
	__android_log_print(ANDROID_LOG_DEBUG, "CashHDL", "[CASH]%s():--------Cash Use Dmp--------", __FUNCTION__);
#endif	// ANDROID
	i = 0;
	cashList = cash->firstCash;
	cashNext = cashList;
	while (cashNext) {
		cashList = cashNext;
		if (0 == cashList->parcelId)
			break;
#ifdef	ANDROID
		__android_log_print(ANDROID_LOG_DEBUG, "CashHDL",
				"%3d |id=0x%08x kind=%d pnt=%p buff=%8d size=%8d next=%p"
				,i
				,cashList->parcelId
				,cashList->dataKind
				,cashList->pBufferAddr
				,cashList->bufferSize
				,cashList->binDataSize
				,cashList->next);
#endif	// ANDROID
		cashNext = cashList->next;
		i++;
	}
}

void DHC_GetPclDataBinInfo(UINT8 kind, UINT8* bufferAddr, UINT32* size, UINT8** addr)
{
	UINT8 i;
	UINT8* workAddr;

	*size = 0;
	*addr = NULL;

	// パラメータチェック
	if (e_DATA_KIND_END <= kind || bufferAddr == NULL) {
		return;
	}

	workAddr = bufferAddr;

	for (i = 0; i <= kind; i++) {
		*size = (UINT32)read4byte(workAddr);
		*addr = workAddr + sizeof(*size);
		workAddr = *addr + (*size);
	}
}

UINT32 SC_DHC_GetKindMask(UINT8 kind) {

	UINT32 mask = 0;

	switch (kind) {
	case e_DATA_KIND_ROAD:
		mask = SC_DHC_KIND_ROAD;
		break;
	case e_DATA_KIND_SHAPE:
		mask = SC_DHC_KIND_SHAPE;
		break;
	case e_DATA_KIND_GUIDE:
		mask = SC_DHC_KIND_GUIDE;
		break;
	case e_DATA_KIND_BKGD:
		mask = SC_DHC_KIND_BKGD;
		break;
	case e_DATA_KIND_NAME:
		mask = SC_DHC_KIND_NAME;
		break;
	case e_DATA_KIND_ROAD_NAME:
		mask = SC_DHC_KIND_ROAD_NAME;
		break;
	case e_DATA_KIND_DENSITY:
		mask = SC_DHC_KIND_DENSITY;
		break;
	case e_DATA_KIND_MARK:
		mask = SC_DHC_KIND_MARK;
		break;
	case e_DATA_KIND_PARCEL_BASIS:
		mask = SC_DHC_KIND_PARCEL_BASIS;
		break;
	case e_DATA_KIND_ROAD_BASE_VERSION:
		mask = SC_DHC_KIND_ROAD_BASE_VER;
		break;
	case e_DATA_KIND_BKGD_AREA_CLS:
		mask = SC_DHC_KIND_BKGD_AREA_CLS;
		break;
	default:
		// 不明種別は来ない前提の為、何もしない
		break;
	}
	return (mask);
}

void* DHC_GetDataBin(UINT8 kind, T_DHC_RES_PARCEL_BIN *resPcl)
{
	void* binInfo = NULL;

	// パラメータチェック
	if (NULL == resPcl) {
		return (binInfo);
	}

	switch (kind) {
	case e_DATA_KIND_ROAD:
		binInfo = &(resPcl->binRoad);
		break;
	case e_DATA_KIND_SHAPE:
		binInfo = &(resPcl->binShape);
		break;
	case e_DATA_KIND_GUIDE:
		binInfo = &(resPcl->binGuide);
		break;
	case e_DATA_KIND_BKGD:
		binInfo = &(resPcl->binBkgd);
		break;
	case e_DATA_KIND_NAME:
		binInfo = &(resPcl->binName);
		break;
	case e_DATA_KIND_ROAD_NAME:
		binInfo = &(resPcl->binRoadName);
		break;
	case e_DATA_KIND_DENSITY:
		binInfo = &(resPcl->binDensity);
		break;
	case e_DATA_KIND_MARK:
		binInfo = &(resPcl->binMark);
		break;
	case e_DATA_KIND_PARCEL_BASIS:
		binInfo = &(resPcl->binParcelBasis);
		break;
	case e_DATA_KIND_ROAD_BASE_VERSION:
		binInfo = &(resPcl->iRoadBaseVersion);
		break;
	case e_DATA_KIND_BKGD_AREA_CLS:
		binInfo = &(resPcl->binBkgdAreaCls);
		break;
	default:
		// 不明種別は来ない前提の為、何もしない
		break;
	}
	return (binInfo);
}
