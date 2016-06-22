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
 * SMCoreTRCache.c
 *
 *  Created on: 2016/01/04
 *      Author: n.kanagawa
 */
#include "sms-core/SMCoreTR/SMCoreTRInternal.h"


// 交通情報パーセル情報
typedef struct {
	TR_BIN_YN_FLG			ynFlg;						// データ有無フラグ
	TR_CONGESTION_INFO_t	congestion;					// 渋滞情報
	//TODO												// SA/PA情報
	//TODO												// 駐車場情報
	//TODO												// 事象・規制情報
} TR_INFO_t;

// 交通情報パーセルテーブル
typedef struct {
	UINT16					user;						// ユーザ
	UINT32					pid;						// レベル1パーセルID
	TR_DATA_t				data;						// パーセルデータ
	TR_INFO_t				info;						// 情報
} TR_PARCEL_DATA_t;

// 交通情報キャッシュテーブル
typedef struct {
	UINT32					carposPid;					// 自車位置パーセルID
	UINT32					scrollPid;					// スクロール地点パーセルID
	TR_PARCEL_DATA_t		parcel[TR_AREA_MGR_MAX];	// パーセルデータリスト
} TR_CACHE_t;

// 交通情報キャッシュテーブル
static TR_CACHE_t	mTRCacheTbl = {};
// Mutex
static SC_MUTEX		mMutex = SC_MUTEX_INITIALIZER;

// プロトタイプ
static TR_PARCEL_DATA_t *searchParcel(UINT32 parcelId);
static TR_PARCEL_DATA_t *searchEmpty();
static E_SC_RESULT analyzeParcel(char *pData, TR_INFO_t *pInfo);
static INT32 analyzeCongestion(char *pData, TR_CONGESTION_INFO_t *pInfo);
static Bool isExpiryDate(const struct tm *expiryDate);


/**
 * @brief	交通情報管理テーブル初期化
 */
E_SC_RESULT SC_TR_CacheTblInitialize()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_START);

	// 交通情報キャッシュテーブル初期化
	memset((void*)&mTRCacheTbl, 0, sizeof(mTRCacheTbl));

	// Mutex生成
	ret = SC_CreateMutex(&mMutex);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_TR, "[TBL] SC_CreateMutext error, " HERE);
	}

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_END);

	return (ret);
}

/**
 * @brief	交通情報管理終了
 */
E_SC_RESULT SC_TR_CacheTblFinalize()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_START);

	// データ解放
	SC_TR_ReleaseUser(TR_USERTYPE_ALL);
	SC_TR_DeleteNoUserData();

	// 交通情報管理テーブル初期化
	memset((void*)&mTRCacheTbl, 0, sizeof(mTRCacheTbl));

	// Mutex解放
	ret = SC_DestroyMutex(&mMutex);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_TR, "[TBL] SC_DestroyMutex error, " HERE);
	}

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_END);

	return (ret);
}

/**
 * @brief	ロック
 */
Bool SC_TR_LockCacheTbl()
{
	Bool ret = true;

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_START);

	// 排他開始
	if (e_SC_RESULT_SUCCESS != SC_LockMutex(&mMutex)) {
		SC_LOG_ErrorPrint(SC_TAG_TR, "[TBL] SC_LockMutext error, " HERE);
		ret = false;
	}

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_END);

	return (ret);

}

/**
 * @brief	アンロック
 */
Bool SC_TR_UnLockCacheTbl()
{
	Bool ret = true;

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_START);

	// 排他終了
	if (e_SC_RESULT_SUCCESS != SC_UnLockMutex(&mMutex)) {
		SC_LOG_ErrorPrint(SC_TAG_TR, "[TBL] SC_UnLockMutext error, " HERE);
		ret = false;
	}

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_END);

	return (ret);
}

/**
 * @brief	キャッシュチェック
 * @param	[I]parcelId		パーセルID
 * @return	有：true/無：false
 */
Bool SC_TR_IsCache(UINT32 parcelId)
{
	if(NULL == searchParcel(parcelId)) {
		return (false);
	}
	return (true);
}

/**
 * @brief	ユーザ設定
 * @param	[I]user			ユーザ
 * @param	[I]parcelId		パーセルID
 * @param	[I]onOff		ON：true/OFF：false
 * @return	有：true/無：false
 */
void SC_TR_SetUser(const UINT16 user, const UINT32 parcelId, const Bool onOff)
{
	TR_PARCEL_DATA_t *pParcelTbl = NULL;

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_START);

	pParcelTbl = searchParcel(parcelId);
	if (NULL != pParcelTbl) {
		if (onOff) {
			// ユーザON設定
			pParcelTbl->user |= user;
		} else {
			// ユーザOFF設定
			pParcelTbl->user &= ~user;
		}
	}

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_END);
}

/**
 * @brief	ユーザ解除
 * @param	[I]user			ユーザ
 * @return	有：true/無：false
 */
void SC_TR_ReleaseUser(const UINT16 user)
{
	INT32	i=0;

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_START);

	// ユーザOFF設定
	for (i=0; i<TR_AREA_MGR_MAX; i++) {
		if (0 != mTRCacheTbl.parcel[i].pid) {
			mTRCacheTbl.parcel[i].user &= ~user;
		}
	}

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_END);
}

/**
 * @brief	データ挿入
 * @param	[I]user			ユーザー
 * @param	[I]parcelId		パーセルID
 * @param	[I]pData		データ
 * @return	成功：true/失敗：false
 */
Bool SC_TR_InsertData(const UINT16 user, const UINT32 parcelId, TR_DATA_t* pData)
{
	Bool				ret = true;
	TR_PARCEL_DATA_t	*pParcelTbl = NULL;

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_START);

	pParcelTbl = searchEmpty();
	if (NULL == pParcelTbl) {
		SC_LOG_ErrorPrint(SC_TAG_TR, (Char*)"searchEmpty no Empty");
		ret = false;
	} else {
		pParcelTbl->user = user;
		pParcelTbl->pid = parcelId;
		pParcelTbl->data.pData = pData->pData;
		pParcelTbl->data.size = pData->size;

		pData->pData = NULL;

		// パーセル解析
		if (e_SC_RESULT_SUCCESS != analyzeParcel(pParcelTbl->data.pData, &pParcelTbl->info)) {
			SC_LOG_ErrorPrint(SC_TAG_TR, (Char*)"analyzeParcel error");
		}
	}

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_END);

	return (ret);
}

/**
 * @brief	データ更新
 * @param	[I]user			ユーザー
 * @param	[I]parcelId		パーセルID
 * @param	[I]pData		データ
 * @return	成功：true/失敗：false
 */
Bool SC_TR_UpdateData(const UINT16 user, const UINT32 parcelId, TR_DATA_t* pData)
{
	Bool				ret = true;
	TR_PARCEL_DATA_t	*pParcelTbl = NULL;

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_START);

	pParcelTbl = searchParcel(parcelId);
	if (NULL == pParcelTbl) {
		ret = false;
	} else {
		// 更新前データ解放
		SC_TR_Free(pParcelTbl->data.pData);

		// 更新後データ設定
		pParcelTbl->user |= user;
		pParcelTbl->data.pData = pData->pData;
		pParcelTbl->data.size = pData->size;

		pData->pData = NULL;

		// パーセル解析
		if (e_SC_RESULT_SUCCESS != analyzeParcel(pParcelTbl->data.pData, &pParcelTbl->info)) {
			SC_LOG_ErrorPrint(SC_TAG_TR, (Char*)"analyzeParcel error");
		}
	}

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_END);

	return (ret);
}

/**
 * @brief	データ更新挿入
 * @param	[I]user			ユーザー
 * @param	[I]parcelId		パーセルID
 * @param	[I]pData		データ
 * @return	成功：true/失敗：false
 */
Bool SC_TR_ReplaceData(const UINT16 user, const UINT32 parcelId, TR_DATA_t* pData)
{
	Bool ret = true;

	// Update
	ret = SC_TR_UpdateData(user, parcelId, pData);
	if (!ret) {
		// Insert
		ret = SC_TR_InsertData(user, parcelId, pData);
	}

	return (ret);
}

/**
 * @brief	データ削除
 * @param	[I]user			ユーザー
 * @param	[I]parcelId		パーセルID(レベル1)
 */
void SC_TR_DeleteData(const UINT16 user, const UINT32 parcelId)
{
	TR_PARCEL_DATA_t *pParcelTbl = NULL;

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_START);

	do {
		pParcelTbl = searchParcel(parcelId);
		if (NULL == pParcelTbl) {
			break;
		}

		if (TR_USERTYPE_NONE != pParcelTbl->user) {
			break;
		}

		// データ解放
		SC_TR_Free(pParcelTbl->data.pData);
		pParcelTbl->data.pData = NULL;
		pParcelTbl->data.size = 0;
		pParcelTbl->user = 0;
		pParcelTbl->pid = 0;
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_END);
}

/**
 * @brief	ユーザ無データ削除
 */
void SC_TR_DeleteNoUserData()
{
	INT32 i=0;

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_START);

	for (i=0; i<TR_AREA_MGR_MAX; i++) {
		if (TR_USERTYPE_NONE == mTRCacheTbl.parcel[i].user) {
			SC_TR_Free(mTRCacheTbl.parcel[i].data.pData);
			mTRCacheTbl.parcel[i].data.pData = NULL;
			mTRCacheTbl.parcel[i].data.size = 0;
			mTRCacheTbl.parcel[i].user = 0;
			mTRCacheTbl.parcel[i].pid = 0;
		}
	}

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_END);
}

/**
 * @brief	メインパーセルID(レベル1)設定
 * @param	[I]user			ユーザー
 * @param	[I]parcelId		パーセルID(レベル1)
 */
void SC_TR_SetMainParcelID(const UINT16 user, const UINT32 parcelId)
{
	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_START);

	if (TR_USERTYPE_CARPOS == user) {
		mTRCacheTbl.carposPid = parcelId;
	} else if (TR_USERTYPE_SCROLL == user) {
		mTRCacheTbl.scrollPid = parcelId;
	} else {
		SC_LOG_ErrorPrint(SC_TAG_TR, (Char*)"unknown user %04x", user);
	}

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_END);
}

/**
 * @brief	メインパーセルID取得
 * @param	[I]user			ユーザー
 * @return	成功：parcelId/失敗：0
 */
UINT32 SC_TR_GetMainParcelID(const UINT16 user)
{
	UINT32 parcelId = 0;

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_START);

	if (TR_USERTYPE_CARPOS == user) {
		parcelId = mTRCacheTbl.carposPid;
	} else if (TR_USERTYPE_SCROLL == user) {
		parcelId = mTRCacheTbl.scrollPid;
	} else {
		SC_LOG_ErrorPrint(SC_TAG_TR, (Char*)"unknown user %04x", user);
	}

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_END);

	return (parcelId);
}

/**
 * @brief	渋滞情報取得
 * @param	[I]parcelId			パーセルID
 * @return	成功：渋滞情報/失敗：NULL
 */
TR_CONGESTION_INFO_t *SC_TR_GetCongestion(UINT32 parcelId)
{
	TR_CONGESTION_INFO_t	*pCongestionInfo = NULL;
	TR_PARCEL_DATA_t		*pParcel;

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_START);

	pParcel = searchParcel(parcelId);
	if (NULL != pParcel) {
		// 有効期限チェック
		if (isExpiryDate(&pParcel->info.congestion.expiryDate)) {
			// 期限内
			if (pParcel->info.ynFlg.b.congestion) {
				pCongestionInfo = &pParcel->info.congestion;
			}
		} else {
			// 期限切れ
			SC_LOG_DebugPrint(SC_TAG_TR, (Char*)"checkExpiryDate false");
		}
	}

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_END);

	return (pCongestionInfo);
}

/**
 * @brief	パーセル検索
 * @param	[I]parcelId		パーセルID
 * @return	成功：テーブル/失敗：NULL
 */
static TR_PARCEL_DATA_t *searchParcel(UINT32 parcelId)
{
	TR_PARCEL_DATA_t	*pParcelTbl = NULL;
	INT32				i=0;

	for (i=0; i<TR_AREA_MGR_MAX; i++) {
		if (parcelId == mTRCacheTbl.parcel[i].pid) {
			pParcelTbl = (&mTRCacheTbl.parcel[i]);
			break;
		}
	}

	return (pParcelTbl);
}

/**
 * @brief	空き検索
 * @return	成功：テーブル/失敗：NULL
 */
static TR_PARCEL_DATA_t *searchEmpty()
{
	return (searchParcel(0));
}

/**
 * @brief	パーセルデータ解析
 * @param	[I]pParcel		パーセルデータ先頭アドレス
 * @param	[O]pInfo		情報
 */
static E_SC_RESULT analyzeParcel(char *pParcel, TR_INFO_t *pInfo)
{
	E_SC_RESULT		ret = e_SC_RESULT_SUCCESS;
	char			*pAddr = NULL;
	INT32			dataSize = 0;

	// 初期化
	memset(pInfo, 0, sizeof(TR_INFO_t));

	if (NULL == pParcel) {
		// データ無の為何もしない
		return (ret);
	}

	// データ有無フラグ
	pInfo->ynFlg.d = TR_YN_FLG(pParcel);
	// データ部先頭アドレス
	pAddr = pParcel + TR_PCL_HEAD_SIZE;

	// 渋滞情報
	if (pInfo->ynFlg.b.congestion) {
		dataSize = analyzeCongestion(pAddr, &pInfo->congestion);
		pAddr += dataSize;
	}
	// SA/PA情報 TODO
	// 駐車場情報 TODO
	// 事象・規制情報 TODO

	return (ret);
}

/**
 * @brief	渋滞情報解析
 * @param	[I]pCongestion		渋滞情報先頭アドレス
 * @param	[O]pCongestionInfo	渋滞情報
 * @return	渋滞情報サイズ
 */
static INT32 analyzeCongestion(char *pCongestion, TR_CONGESTION_INFO_t *pCongestionInfo)
{
	INT32			dataSize = 0;
	char			*pRdkdAddr = NULL;
	UINT32			i = 0;
	UINT32			roadKindCnt = 0;
	UINT16			roadKind = 0;
	TR_BIN_TIME		expiryDate;

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_START);

	// 渋滞情報データサイズ
	dataSize = TR_CGN_SIZE(pCongestion);

	// 有効期限
	expiryDate.d = TR_EXPIRY_DATE(pCongestion);
	SC_TR_ChgTM(expiryDate, &pCongestionInfo->expiryDate);

	// 道路種別数
	roadKindCnt = TR_ROADKIND_CNT(pCongestion);

	// 道路種別先頭アドレス
	pRdkdAddr = pCongestion + TR_CGN_HEAD_SIZE;

#ifdef _TR_DEBUG
	SC_LOG_DebugPrint(SC_TAG_TR, (Char*)"■roadKindCnt[%d]", roadKindCnt);
#endif

	for (i=0; i<roadKindCnt; i++) {
		// 道路種別
		roadKind = TR_ROAD_KIND(pRdkdAddr);
		if (roadKind >= TR_ROAD_KIND_MAX) {
			// 種別不正
			SC_LOG_ErrorPrint(SC_TAG_TR, (Char*)"roadKind error %d", roadKind);
			break;
		}

		// 道路種別-情報の先頭アドレス設定
		pCongestionInfo->roadKind[roadKind].pInfo = pRdkdAddr + TR_RDKD_HEAD_SIZE + TR_INFO_OFS(pRdkdAddr);
		// 道路種別数設定
		pCongestionInfo->roadKind[roadKind].cnt = TR_INFO_CNT(pRdkdAddr);

#ifdef _TR_DEBUG
		SC_LOG_DebugPrint(SC_TAG_TR, (Char*)"■[%2d] pid[%08x], size[%5d], rdkind[%2d], pInfo[%p] cnt[%4d]",
				i+1,
				TR_RKPCL_ID(pRdkdAddr),
				TR_RDK_SIZE(pRdkdAddr),
				roadKind,
				pCongestionInfo->roadKind[roadKind].pInfo,
				pCongestionInfo->roadKind[roadKind].cnt);
#endif

		// 次の道路種別へ
		pRdkdAddr += TR_RDK_SIZE(pRdkdAddr);
	}

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_END);

	return (dataSize);
}

/**
 * @brief	有効期限チェック
 * @param	[I]expiryDate		有効期限
 * @return	期限内:true/期限切れ:false
 */
static Bool isExpiryDate(const struct tm *expiryDate)
{
	Bool	ret = true;
	struct tm *pNowTm = NULL;

	pNowTm = SC_TR_NowTM();
	if (NULL == pNowTm) {
		ret = false;
	} else {
		if (SC_TR_CompTM(pNowTm, expiryDate)) {
			ret = false;
		}
	}

	return (ret);
}

#if 0
static char* reqParcel(UINT32 parcelId)
{
	E_DHC_CASH_RESULT ret = e_DHC_RESULT_CASH_SUCCESS;
	T_DHC_REQ_PARCEL mapReqPcl;
	T_DHC_RES_DATA mapResData = {};
	char* pData = NULL;

	mapReqPcl.user = SC_DHC_USER_PI;
	mapReqPcl.parcelNum = 1;
	mapReqPcl.parcelInfo[0].parcelId = parcelId;
	mapReqPcl.parcelInfo[0].mapKind = SC_DHC_GetKindMask(e_DATA_KIND_SHAPE);

	ret = SC_DHC_MapRead(&mapReqPcl, &mapResData);
	if (e_DHC_RESULT_CASH_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_TR, (Char*)"SC_DHC_MapRead 0x%08x", ret);
	}

	if (NULL != mapResData.parcelBin[0].binShape) {
		pData = (UINT8*)mapResData.parcelBin[0].binShape;
	}

	return (pData);
}

static void freeParcel(UINT32 parcelId)
{
	T_DHC_REQ_PARCEL mapReqPcl;

	mapReqPcl.user = SC_DHC_USER_PI;
	mapReqPcl.parcelNum = 1;
	mapReqPcl.parcelInfo[0].parcelId = parcelId;
	mapReqPcl.parcelInfo[0].mapKind = 0;

	// 道路形状バイナリ解放
	mapReqPcl.parcelInfo[0].mapKind = SC_DHC_GetKindMask(e_DATA_KIND_SHAPE);

	// 地図データ解放
	SC_DHC_MapFree(&mapReqPcl);

	return;
}

// 交通情報
typedef struct {
	char*	p;
	UINT16	cngsLvl;
	UINT16	cnt;
} TR_SHAPE_t;

static void getShape(UINT32 parcelId, TR_CONGESTION_INFO_t *pCngsInfo)
{
	char* pData;
	INT32 i = 0;

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_START);

	// 地図取得
	pData = reqParcel(parcelId);
	if (NULL == pData) {
		return;
	}

	// バイナリデータ先頭アドレス
	char* p_Binary = pData + 4;

	// バイナリデータレコード数
	char* m_pShapeRec = p_Binary + 2 + 2 + 92;

	// 先頭位置
	UINT32 *pTop = (UINT32*)((char*)p_Binary + 4);
	// 索引データ先頭
	UINT32 *pIndex = (UINT32*)((char*)p_Binary + ((pTop[16])*4));

	UINT32 detaSize = *pIndex;
	UINT32 detaCnt = *(pIndex+1);
	pIndex = pIndex + 2;
/*	for (i=0; i<detaCnt; i++) {
		SC_LOG_DebugPrint(SC_TAG_TR, "%d offset=%08x", i+1, pIndex[i]);
	}*/

	INT32 cnt = pCngsInfo->roadKind[0].cnt;
	char *pInfo = pCngsInfo->roadKind[0].pInfo;

	INT32 allPointCnt = 0;

	// 情報数分ループ
	for (i=0; i<cnt; i++) {
		// 渋滞度
		UINT16 cngsLvl = TR_CNGS_LVL(pInfo);
		if (0 == cngsLvl) {
			pInfo += TR_INFO_SIZE(pInfo);
			continue;
		}

		// リンクID
		UINT32 linkId = TR_LINK_ID(pInfo);

		char* aaa = searchShape(pIndex, detaCnt, m_pShapeRec, linkId);
		if (aaa == NULL) {
			SC_LOG_DebugPrint(SC_TAG_TR, "NULL");
		} else {
			SC_LOG_DebugPrint(SC_TAG_TR, "%d", *(UINT16*)(aaa+20));
			allPointCnt += *(UINT16*)(aaa+20);
		}

		// リンクIDで座標情報取得
		// 個数計算
		SC_LOG_DebugPrint(SC_TAG_TR, "%d linkid=%08x", i+1, linkId);

		// 次へ
		pInfo += TR_INFO_SIZE(pInfo);
	}

	SC_LOG_DebugPrint(SC_TAG_TR, "■%d", allPointCnt);
	// メモリ確保
	char* ppppp = SC_TR_Malloc(allPointCnt*sizeof(TR_SHAPE_t) + allPointCnt*4);

	TR_SHAPE_t* shapeHead = (TR_SHAPE_t*)ppppp;

	UINT32 ofs = allPointCnt*sizeof(TR_SHAPE_t);

	pInfo = pCngsInfo->roadKind[0].pInfo;
	// 座標コピー
	// 情報数分ループ
	for (i=0; i<cnt; i++) {
		// 渋滞度
		shapeHead[i].cngsLvl=TR_CNGS_LVL(pInfo);
		if (0 == shapeHead[i].cngsLvl) {
			pInfo += TR_INFO_SIZE(pInfo);
			continue;
		}
		// リンクID
		UINT32 linkId = TR_LINK_ID(pInfo);
		char* aaa = searchShape(pIndex, detaCnt, m_pShapeRec, linkId);
		shapeHead[i].cnt = *(UINT16*)(aaa+20);
/*		shapeHead[i].p = ppppp + ofs;
		memcpy(shapeHead[i].p, (aaa+24), shapeHead[i].cnt*4);
		ofs += (shapeHead[i].cnt * 4);

		SC_LOG_DebugPrint(SC_TAG_TR, "■%d %p %d", shapeHead[i].cnt, shapeHead[i].p, shapeHead[i].cngsLvl);
*/		// 次へ
		pInfo += TR_INFO_SIZE(pInfo);
	}

	freeParcel(parcelId);

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_END);
}

static char* searchShape(UINT32* pShapeIndex, UINT32 cnt, char* pShapeRec, UINT32 LinkID)
{
	char* ResultIndex = NULL;
	INT32 lowid;
	INT32 midid;
	INT32 highid;
	UINT32 target = LinkID;

	// 添字の範囲を初期化
	highid	= cnt-1;
	lowid	= 0;

	// 値が見つかるまで繰り返す
	while (lowid <= highid) {
		midid = (lowid + highid) / 2;

		// オフセットスからリンクID取得
		UINT32 offset = pShapeIndex[midid]*4;
		char* h_rdsp = pShapeRec + offset;
		UINT32 buf_link_id = *(UINT32*)((char*)h_rdsp + 4);

		if(buf_link_id == target) {
			// 見つかった
			ResultIndex = h_rdsp;
			break;
		}
		else if(buf_link_id < target) {
			lowid = midid + 1;
		}
		else {
			highid = midid - 1;
		}
	}

	return (ResultIndex);
}
#endif
/**
 * @brief	デバッグ出力
 */
void SC_TR_CacheDebugPrint()
{
	INT32 i = 0;

	SC_LOG_DebugPrint(SC_TAG_TR, "************************************************");

	SC_LOG_DebugPrint(SC_TAG_TR, "carposPid = %08x", mTRCacheTbl.carposPid);
	SC_LOG_DebugPrint(SC_TAG_TR, "scrollPid = %08x", mTRCacheTbl.scrollPid);

	for (i=0; i<TR_AREA_MGR_MAX; i++) {
		SC_LOG_DebugPrint(SC_TAG_TR, "%3d user[%04x], pid[%08x], pData[%p], size[%d]"
				,i+1
				,mTRCacheTbl.parcel[i].user
				,mTRCacheTbl.parcel[i].pid
				,mTRCacheTbl.parcel[i].data.pData
				,mTRCacheTbl.parcel[i].data.size);
	}

	SC_LOG_DebugPrint(SC_TAG_TR, "************************************************");
}
