/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#include "SMCComCMNInternal.h"

/**
 * @struct MEM_CELL
 * @brief メモリセル構造体の定義.
 *
 * メモリセル構造体は以下のような構造であり、未使用領域の場合に限り
 * 片方向のリンクを構成する。
 *
 *  ┌───────┐  +-->┌───────┐  +-->┌───────┐  +--> …
 *  │ size         │  |   │ size         │      │ size         │  |
 *  ├───────┤  |   ├───────┤  |   ├───────┤  |
 *  │ next         │--+   │ next         │--+   │ next         │--+
 *  └───────┘      └───────┘      └───────┘
 *  │              │      │              │      │              │
 *  │実メモリ      │      │実メモリ      │      │実メモリ      │
 *  │   領域       │      │   領域       │      │   領域       │
 *  │              │      │              │      │              │
 *  └───────┘      └───────┘      └───────┘
 *
 * 使用済み領域の場合には、nextの領域も含めて
 * データ(実メモリ)領域として用いる。
 */
typedef struct _MEM_CELL {
	SIZE				size;				// 領域有効サイズ
	struct _MEM_CELL	*next;				// 次の領域のポインタ
} MEM_CELL;


/**
 * @struct MEM_ALLOCAREAINF
 * @brief メモリマネージャ管理構造体
 * @note  メモリマネージャが管理上必要とする情報を記録する。
 *        実際には必要な情報種別分だけ情報が存在する形となる。
 */
typedef struct _MEM_ALLOCAREAINF {
	void				*top;				// 管理領域の先頭アドレス
	SIZE				freeAreaTotalSize;	// 管理領域のトータルサイズ
	MEM_CELL			*topFreeList;		// フリーツリーの先頭アドレス
	MEM_CELL			*latestFree;		// 前回のフリーツリーキャッシュ
} MEM_ALLOCAREAINF;

// 共通の関数マクロ項目
#define MEM_HDR_SIZE		offsetof(MEM_CELL, next)					// メモリセルヘッダサイズ
#define OFFSET(cp, size)	((void *)((Char*)(cp) + (size)))			// オフセット位置の計算
#define OBJMEMCELL(obj)		((MEM_CELL*)((Char*)(obj) - MEM_HDR_SIZE))	// オブジェクトからセル先頭位置を計算
#define MEMCELLOBJ(cell)	((void *)((Char*)(cell) + MEM_HDR_SIZE))	// セル先頭位置からオブジェクト位置計算

// メモリ管理構造体配列の実態(静的メモリ)
static MEM_ALLOCAREAINF memMngList[CC_MEM_TYPE_END];

// Mutex
static SCC_MUTEX mutexTable[CC_MEM_TYPE_END] = {SCC_MUTEX_INITIALIZER};

// DEBUG用
static SIZE freeAreaSize[CC_MEM_TYPE_END];
static SIZE useAreaSize[CC_MEM_TYPE_END];

/**
 * @brief メモリマネージャ管理に領域を割り当てる
 * @param[in] size 割り当て領域のサイズ
 * @param[in] type メモリ割り当て領域種別
 * @return 処理結果(E_SC_RESULT)
 * @note 実際に確保する領域は、(指定サイズ + 管理領域のサイズ)の合計サイズとなる。
 */
E_SC_RESULT SCC_MEM_Initialize(SIZE size, E_SCC_MEM_TYPE type)
{
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	Char *ptr  = NULL;
	MEM_ALLOCAREAINF *inf = NULL;
	SIZE memSize = (size + MEM_HDR_SIZE);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	// パラメータチェック
	if (sizeof(MEM_CELL) > memSize) {
		// 最低限セルサイズがない場合はエラーとする
		SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[size], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (CC_MEM_TYPE_END == type) {
		SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[type], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	inf = &memMngList[type];
	if (NULL != inf->top) {
		// 二重初期化はエラーだが、処理は継続する
		return (e_SC_RESULT_SUCCESS);
	}

	// Mutex生成
	ret = SCC_CreateMutex(&mutexTable[type]);
	if (e_SC_RESULT_SUCCESS != ret) {
		SCC_LOG_ErrorPrint(SC_TAG_CC, "SCC_CreateMutext error, " HERE);
		return (ret);
	}

	// メモリ確保(管理領域サイズ含む)
	ptr = (Char*)malloc(memSize);
	if (NULL == ptr) {
		SCC_LOG_ErrorPrint(SC_TAG_CC, "malloc error, " HERE);
		return (e_SC_RESULT_MALLOC_ERR);
	}
	// 0クリア
	memset(ptr, 0, memSize);

	// メモリ管理構造体を初期化する
	inf->top = ptr;
	inf->topFreeList = (MEM_CELL*)ptr;
	inf->freeAreaTotalSize = memSize;
	inf->latestFree = NULL;

	// FreeListメンバ(第一セル)に正しい値を設定
	inf->topFreeList->size = memSize;	// 管理領域サイズは残りサイズに含んでOK
	inf->topFreeList->next = NULL;		// 次の領域のポインタなし

	freeAreaSize[type] = memSize;
	useAreaSize[type] = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief メモリマネージャ管理の領域を解放する
 * @param[in] type メモリ割り当て領域種別
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SCC_MEM_Finalize(E_SCC_MEM_TYPE type)
{
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	MEM_ALLOCAREAINF *inf = NULL;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// パラメータチェック
		if (CC_MEM_TYPE_END == type) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[type], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		inf = &memMngList[type];
		if (NULL != inf->top) {
			// メモリ解放
			free(inf->top);

			// メモリ管理構造体を終了状態に戻す
			inf->top = NULL;
			inf->topFreeList = NULL;
			inf->freeAreaTotalSize = 0;
			inf->latestFree = NULL;
		}

		// Mutex破棄
		ret = SCC_DestroyMutex(&mutexTable[type]);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "SCC_DestroyMutext error, " HERE);
			break;
		}
	} while (0);

	freeAreaSize[type] = 0;
	useAreaSize[type] = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief メモリ領域を割り当てる
 * @param[in] size:割り当てたい要求サイズ
 *			仮に要求サイズが0でも最低限のブロックサイズは割り当てる
 * @param[in] type:メモリ割り当て領域種別
 * @note 実際に確保する領域は、(指定サイズ + 管理領域のサイズ(4～8バイト))の合計サイズとなる。
 */
void *SCC_MEM_Alloc(SIZE size, E_SCC_MEM_TYPE type)
{
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	SCC_MUTEX	*mutex = NULL;
	MEM_CELL *next = NULL;			// 次の領域のポインタ */
	MEM_CELL *cur = NULL;			// 現在領域のポインタ */
	MEM_CELL **curPtr = NULL;		// 上記curへのポインタ */
	MEM_ALLOCAREAINF *inf = NULL;
	SIZE cellSize = size;			// セルに割り当て予定のサイズ

	// パラメータチェック
	if (0 == size) {
		SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[size], " HERE);
		return (NULL);
	}
	if (CC_MEM_TYPE_END == type) {
		SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[type], " HERE);
		return (NULL);
	}

	inf = &memMngList[type];
	if (NULL == inf->top) {
		// 該当するメモリバンク未割当なので0を返却する
		SCC_LOG_ErrorPrint(SC_TAG_CC, "UnInitialize, " HERE);
		return (NULL);
	}

	// 排他制御開始
	mutex = &mutexTable[type];
	ret = SCC_LockMutex(mutex);
	if (ret != e_SC_RESULT_SUCCESS) {
		SCC_LOG_ErrorPrint(SC_TAG_CC, "SCC_LockMutext error, " HERE);
		return (NULL);
	}

	// ヘッダサイズ分を取る
	cellSize += MEM_HDR_SIZE;

	/* 最低限4バイト境界にアラインメント
	 * 特にブロック境界にはアラインしていないので、
	 * 必要に応じて適切な境界にアラインすること
	 */
	cellSize = (cellSize + sizeof(INT32) - 1) & ~(sizeof(INT32) - 1);

	// ファーストフィットとFreeList形式を使用する
	curPtr = &inf->topFreeList;
	cur = *curPtr;

	// フィットするセルを探索
	while (NULL != cur) {
		if (cellSize <= cur->size) {
			// 割当可能セル発見
			break;
		}

		// 次セルへ移動
		curPtr = &cur->next;
		cur = *curPtr;
	}

	if (NULL == cur) {
		// 空き領域が見つからなかった場合
		SCC_LOG_ErrorPrint(SC_TAG_CC, "alloc err. memory full, type=%d size=%d, " HERE, type, size);
		// 排他制御終了
		ret = SCC_UnLockMutex(mutex);
		if (ret != e_SC_RESULT_SUCCESS) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "SCC_UnLockMutex error, " HERE);
		}
		return (NULL);
	}

	if (cur->size >= cellSize + sizeof(MEM_CELL)) {
		// 必要な領域は得られたが、大きいので分割する
		next = (MEM_CELL*)OFFSET(cur, cellSize);
		next->size = cur->size - cellSize;
		next->next = cur->next;
		cur->size = cellSize;
		cur->next = next;
	}

	// ツリーに次の領域のポインタを設定する
	*curPtr = cur->next;

	// 割り当て領域がフリーツリーキャッシュだったら、キャッシュクリア
	if (cur == inf->latestFree) {
		inf->latestFree = NULL;
	}

	freeAreaSize[type] -= cellSize;
	useAreaSize[type]  += cellSize;
	SCC_LOG_DebugPrint(SC_TAG_CC, "[SMCCom]block%d free=%u, use=%u, " HERE, (type + 1), freeAreaSize[type], useAreaSize[type]);

	// 排他制御終了
	ret = SCC_UnLockMutex(&mutexTable[type]);
	if (ret != e_SC_RESULT_SUCCESS) {
		SCC_LOG_ErrorPrint(SC_TAG_CC, "SCC_UnLockMutex error, " HERE);
	}

	return MEMCELLOBJ(cur);
}

/**
 * @brief メモリ領域を解放する
 * @param[in] ptr  解放したいメモリ領域の先頭ポインタ
 * @param[in] type メモリ割り当て領域種別
 */
void SCC_MEM_Free(void *ptr, E_SCC_MEM_TYPE type)
{
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	MEM_CELL	*prevFree = NULL;		// 前スロット要素
	MEM_CELL	*nextFree = NULL;		// 次スロット要素
	MEM_CELL	*cur = NULL;			// 現在スロット位置
	MEM_ALLOCAREAINF *inf = NULL;
	SIZE		cellSize = 0;

	// パラメータチェック
	if (NULL == ptr) {
		SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[ptr], " HERE);
		return;
	}
	if (CC_MEM_TYPE_END == type) {
		SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[type], " HERE);
		return;
	}

	inf = &memMngList[type];
	if (NULL == inf->top) {
		// 該当タイプの領域全体が解放されている場合
		return;
	}

	// 排他制御開始
	ret = SCC_LockMutex(&mutexTable[type]);
	if (ret != e_SC_RESULT_SUCCESS) {
		SCC_LOG_ErrorPrint(SC_TAG_CC, "SCC_LockMutext error, " HERE);
		return;
	}

	// 解放対象のスロットアドレスを取得
	cur = OBJMEMCELL(ptr);

	cellSize = cur->size;

	/* 探索開始位置の設定
	 *   前回の解放時に使った際のポインタ位置を有効ならば使ってしまう。
	 *   そうすることによりフリーツリー探索を行う無駄な時間を少し省ける。
	 */
	nextFree = inf->topFreeList;
	if (NULL != inf->latestFree && inf->latestFree < cur) {
		// 前回キャッシュ位置が妥当な場合は使う
		nextFree = inf->latestFree;
	}

	while (NULL != nextFree && nextFree < cur) {
		// 次空き要素があり、次空き要素が未だcurを超過していない場合
		prevFree = nextFree;
		nextFree = nextFree->next;
	}

	// 次要素との後処理(連結処理)
	if (NULL != nextFree) {
		// 次空き要素が存在する場合
		if (OFFSET(cur, cur->size) == nextFree) {
			// 次空き要素と隣接する場合は連結する
			cur->size += nextFree->size;
			cur->next = nextFree->next;
		} else {
			// 次空き要素と隣接しない場合
			cur->next = nextFree;
		}
	} else {
		// 次空き要素が存在しない場合
		cur->next = NULL;
	}

	// キャッシュ設定
	inf->latestFree = cur;

	// 前要素との後処理(連結処理)
	if (NULL != prevFree) {
		if (OFFSET(prevFree, prevFree->size) == cur) {
			// 前の隣接空き要素と連結する
			prevFree->size += cur->size;
			prevFree->next = cur->next;
			// キャッシュはひとつ前に設定
			inf->latestFree = prevFree;
		} else {
			prevFree->next = cur;
		}
	} else {
		// 前空き要素がない場合、curはFreeListの先頭となる
		inf->topFreeList = cur;
		// キャッシュは不要なのでNULL設定 */
		inf->latestFree = NULL;
	}

	freeAreaSize[type] += cellSize;
	useAreaSize[type]  -= cellSize;
	SCC_LOG_DebugPrint(SC_TAG_CC, "[SMCCom]block%d free=%u, use=%u, " HERE, (type + 1), freeAreaSize[type], useAreaSize[type]);

	// 排他制御終了
	ret = SCC_UnLockMutex(&mutexTable[type]);
	if (ret != e_SC_RESULT_SUCCESS) {
		SCC_LOG_ErrorPrint(SC_TAG_CC, "SCC_UnLockMutex error, " HERE);
	}
}

/**
 * @brief メモリ領域を解放する
 * @param[in] type メモリ割り当て領域種別
 */
void SCC_MEM_FreeAll(E_SCC_MEM_TYPE type)
{
	MEM_ALLOCAREAINF *inf = NULL;

	if (CC_MEM_TYPE_END == type) {
		SCC_LOG_ErrorPrint(SC_TAG_CC, "param error[type], " HERE);
		return;
	}
	inf = &memMngList[type];
	if (NULL == inf->top) {
		// 該当タイプの領域全体が解放されている場合
		return;
	}

	// メモリ管理構造体を初期化する
	inf->topFreeList = (MEM_CELL*) inf->top;
	inf->latestFree = NULL;

	// FreeListメンバ(第一セル)に正しい値を設定
	inf->topFreeList->size = inf->freeAreaTotalSize;	// 管理領域サイズは残りサイズに含んでOK
	inf->topFreeList->next = NULL;						// 次の領域のポインタなし

}

/**
 * デバッグ用ダンプ (フリーリストダンプ)
 */
void SCC_MEM_Dump() {

	INT32 i, e;
	MEM_ALLOCAREAINF *inf = NULL;
	MEM_CELL *cur = NULL;			// 現在領域のポインタ */
	MEM_CELL **curPtr = NULL;		// 上記curへのポインタ */

	for (i = 0; i < CC_MEM_TYPE_END; i++) {
		SCC_LOG_InfoPrint(SC_TAG_CC, "MEM_TYPE %d, " HERE, i);
		inf = &memMngList[i];
		if (NULL == inf->top) {
			SCC_LOG_InfoPrint(SC_TAG_CC, "Not Use Memory, " HERE);
			continue;
		}
		SCC_LOG_InfoPrint(SC_TAG_CC, "totalsize:%9d, addr:%p, " HERE, inf->freeAreaTotalSize, inf->top);
		cur = inf->topFreeList;
		e = 0;
		while (cur != NULL ) {
			SCC_LOG_InfoPrint(SC_TAG_CC, "%d,   size:%9d, addr:%p " HERE, e, cur->size, cur->next);
			// 次セルへ移動
			curPtr = &cur->next;
			cur = *curPtr;
			e++;
		}
	}
}

/**
 * デバッグ用ダンプ (フリーリストダンプ)
 */
void SCC_MEM_Dump_Type(E_SCC_MEM_TYPE type) {

	//INT32 i;
	INT32 e;
	MEM_ALLOCAREAINF *inf = NULL;
	MEM_CELL *cur = NULL;			// 現在領域のポインタ */
	MEM_CELL **curPtr = NULL;		// 上記curへのポインタ */

	SCC_LOG_InfoPrint(SC_TAG_CC, "MEM_TYPE %d, " HERE, type);
	inf = &memMngList[type];
	if (NULL == inf->top) {
		SCC_LOG_InfoPrint(SC_TAG_CC, "Not Use Memory" HERE);
		return;
	}
	SCC_LOG_InfoPrint(SC_TAG_CC, "totalsize:%9d, addr:%p, " HERE, inf->freeAreaTotalSize, inf->top);
	cur = inf->topFreeList;
	e = 0;
	while (cur != NULL ) {
		SCC_LOG_InfoPrint(SC_TAG_CC, "%d,   size:%9d, addr:%p, " HERE, e, cur->size, cur->next);
		// 次セルへ移動
		curPtr = &cur->next;
		cur = *curPtr;
		e++;
	}
}
