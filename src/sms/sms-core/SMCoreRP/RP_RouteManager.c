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
 * RP_RouteManager.c
 *
 *  Created on: 2014/02/17
 *      Author: 70251034
 */

#include "SMCoreRPInternal.h"

/*-------------------------------------------------------------------
 * 定数定義
 *-------------------------------------------------------------------*/
#define SCRM_RTBUFMAX			(4)						// バッファ数(確定経路,非確定経路,予備,予備)
#define SCRM_RTTYPEMAX			(RPC_SIZE)				// 経路種別数
/*-------------------------------------------------------------------
 * マクロ定義
 *-------------------------------------------------------------------*/
#define RM_MUTEX_LOCK(mutex)															\
	{																					\
		E_SC_RESULT ret = e_SC_RESULT_SUCCESS;											\
		ret = SC_LockMutex(&mutex);														\
		if (ret != e_SC_RESULT_SUCCESS) {												\
			SC_LOG_ErrorPrint(SC_TAG_RM, "SC_LockMutext error. [0x%08x] "HERE, ret);	\
			return (ret);																\
		}																				\
	}
#define RM_MUTEX_UNLOCK(mutex)															\
	{																					\
		E_SC_RESULT ret = e_SC_RESULT_SUCCESS;											\
		ret = SC_UnLockMutex(&mutex);													\
		if (ret != e_SC_RESULT_SUCCESS) {												\
			SC_LOG_ErrorPrint(SC_TAG_RM, "SC_UnLockMutext error. [0x%08x] "HERE, ret);	\
			return (ret);																\
		}																				\
	}
/*-------------------------------------------------------------------
 * 構造体定義
 *-------------------------------------------------------------------*/
typedef struct _SCRM_ROUTEBUFFERMNG {		// 公開経路管理
	UINT32 rtId;							// 経路ID
	struct {
		Bool setFlag;						// サイズ
		UINT32 user;						// ユーザ
		SC_RP_RouteMng* rt;					// データ
	} rtCont[SCRM_RTTYPEMAX];
} SCRM_ROUTEBUFFERMNG;
typedef struct _SCRM_ROUTEBUFFER_PRE {		// 書込み待機経路
	UINT32 rtId;							// 経路ID
	UINT32 rtType;							// 経路種別
	SC_RP_RouteMng* rt;						// データ
} SCRM_ROUTEBUFFER_PRE;
/*-------------------------------------------------------------------
 * 変数宣言
 *-------------------------------------------------------------------*/
static SC_MUTEX m_Mutex = SC_MUTEX_INITIALIZER;					// Mutex
static SCRM_ROUTEBUFFERMNG m_RtMng[SCRM_RTBUFMAX];				// 適当にバッファ持つ
static SCRM_ROUTEBUFFER_PRE m_RtMngPre;							// 待機経路
static UINT32 m_ReleaseRtId;									// 確定経路ID
static UINT32 m_ReleaseRtType;									// 確定経路種別
/*-------------------------------------------------------------------
 * 宣言
 *-------------------------------------------------------------------*/
static E_SC_RESULT RM_VacantIdxSearch(UINT32* aIdx);
static E_SC_RESULT RM_RouteIdSearch(UINT32 aRtId, UINT32* aIdx);
static E_SC_RESULT RM_DeleteReleaseRoute();
static void RM_RouteFree(SC_RP_RouteMng* aRt);

/**
 * @brief	管理テーブル空き検索
 * @param	[O]管理テーブルIDX
 */
static E_SC_RESULT RM_VacantIdxSearch(UINT32* aIdx) {
	UINT32 i;

	for (i = 0; i < SCRM_RTBUFMAX; i++) {
		// 経路無し
		if (SC_RP_RTIDINIT == m_RtMng[i].rtId) {
			*aIdx = i;
			return (e_SC_RESULT_SUCCESS);
		}
	}
	// 見つからない
	return (e_SC_RESULT_FAIL);
}

/**
 * @brief	経路ID検索
 * @param	[I]検索ID
 * @param	[O]管理テーブルIDX
 */
static E_SC_RESULT RM_RouteIdSearch(UINT32 aRtId, UINT32* aIdx) {
	UINT32 i;

	for (i = 0; i < SCRM_RTBUFMAX; i++) {
		// 経路無し
		if (aRtId == m_RtMng[i].rtId) {
			*aIdx = i;
			return (e_SC_RESULT_SUCCESS);
		}
	}
	// 見つからない
	return (e_SC_RESULT_FAIL);
}

/**
 * @brief	確定経路削除処理
 */
static E_SC_RESULT RM_DeleteReleaseRoute() {
	UINT32 i;
	UINT32 e;
	UINT32 user = 0;

	// 確定経路なし
	if (SC_RP_RTIDINIT == m_ReleaseRtId) {
		SC_LOG_ErrorPrint(SC_TAG_RM, "Param error. "HERE);
		return (e_SC_RESULT_SUCCESS);
	}

	// 確定経路検索
	for (i = 0; i < SCRM_RTBUFMAX; i++) {
		if (m_ReleaseRtId == m_RtMng[i].rtId) {
			// ユーザ確認しながら削除
			user = 0;
			for (e = 0; e < SCRM_RTTYPEMAX; e++) {
				user |= m_RtMng[i].rtCont[e].user;
				if (user) {
					continue;
				}
				if (NULL != m_RtMng[i].rtCont[e].rt) {
					RM_RouteFree(m_RtMng[i].rtCont[e].rt);
				}
				m_RtMng[i].rtCont[e].rt = NULL;
				m_RtMng[i].rtCont[e].user = 0;
				m_RtMng[i].rtCont[e].setFlag = false;
			}
			// ユーザ全削除済み
			if (0 == user) {
				m_RtMng[i].rtId = SC_RP_RTIDINIT;
			}
			// ※書き換え用データも初期化
			if (NULL != m_RtMngPre.rt) {
				RM_RouteFree(m_RtMngPre.rt);
			}
			m_RtMngPre.rtId = SC_RP_RTIDINIT;
			m_RtMngPre.rtType = SC_RP_RTTYPEINIT;
			m_RtMngPre.rt = NULL;
		}
	}
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief	推奨経路開放処理
 * @param	[I/O]推奨経路管理
 */
static void RM_RouteFree(SC_RP_RouteMng* aRt) {

	if (NULL == aRt) {
		return;
	}

	if (NULL != aRt->parcelInfo) {
		RP_MemFree(aRt->parcelInfo, e_MEM_TYPE_ROUTEMNG);
		aRt->parcelInfo = NULL;
	}
	if (NULL != aRt->linkInfo) {
		RP_MemFree(aRt->linkInfo, e_MEM_TYPE_ROUTEMNG);
		aRt->linkInfo = NULL;
	}
	if (NULL != aRt->formInfo) {
		RP_MemFree(aRt->formInfo, e_MEM_TYPE_ROUTEMNG);
		aRt->formInfo = NULL;
	}
	if (NULL != aRt->regInfo) {
		RP_MemFree(aRt->regInfo, e_MEM_TYPE_ROUTEMNG);
		aRt->regInfo = NULL;
	}
	if (NULL != aRt->sectInfo) {
		RP_MemFree(aRt->sectInfo, e_MEM_TYPE_ROUTEMNG);
		aRt->sectInfo = NULL;
	}
	RP_MemFree(aRt, e_MEM_TYPE_ROUTEMNG);
}

/**
 * @brief	初期化処理
 */
E_SC_RESULT SC_RP_RouteManagerInit() {
	UINT32 i;
	E_SC_RESULT result = e_SC_RESULT_SUCCESS;

	// Mutex生成
	result = SC_CreateMutex(&m_Mutex);
	if (e_SC_RESULT_SUCCESS != result) {
		SC_LOG_ErrorPrint(SC_TAG_RM, "SC_CreateMutext error. [0x%08x] "HERE, result);
		return (e_SC_RESULT_FAIL);
	}

	// 初期化
	RP_Memset0(m_RtMng, sizeof(SCRM_ROUTEBUFFERMNG) * SCRM_RTBUFMAX);
	RP_Memset0(&m_RtMngPre, sizeof(SCRM_ROUTEBUFFER_PRE));

	m_ReleaseRtId = SC_RP_RTIDINIT;
	m_ReleaseRtType = SC_RP_RTTYPEINIT;

	for (i = 0; i < SCRM_RTBUFMAX; i++) {
		m_RtMng[i].rtId = SC_RP_RTIDINIT;
	}
	m_RtMngPre.rtId = SC_RP_RTIDINIT;
	m_RtMngPre.rtType = SCRM_RTTYPEMAX;

	return (result);
}

/**
 * @brief	経路参照終了
 * @param	[I]経路ID
 * @param	[I]経路種別
 * @param	[I]ユーザ
 * @return	e_SC_RESULT_SUCCESS 取得成功
 * @return	e_SC_RESULT_SUCCESS以外 取得失敗
 */
E_SC_RESULT SC_RP_ReadRouteExit(UINT32 aRtId, UINT32 aRtType, UINT32 aUser) {
	SC_LOG_DebugPrint(SC_TAG_RM, SC_LOG_START);
	SC_LOG_DebugPrint(SC_TAG_RM, "Route exit user user=0x%08x "HERE, aUser);

	UINT32 i;
	E_SC_RESULT result = e_SC_RESULT_FAIL;

	// チェック
	if ((SC_RP_RTIDINIT == aRtId) || (SCRM_RTTYPEMAX <= aRtType)) {
		SC_LOG_ErrorPrint(SC_TAG_RM, "Param error. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	// [排他制御開始]
	RM_MUTEX_LOCK(m_Mutex);

	for (i = 0; i < SCRM_RTBUFMAX; i++) {
		// ID
		if (aRtId != m_RtMng[i].rtId) {
			continue;
		}
		// ユーザ登録解除
		m_RtMng[i].rtCont[aRtType].user = ((~aUser) & m_RtMng[i].rtCont[aRtType].user);
		// 成功
		result = e_SC_RESULT_SUCCESS;

		// [※特殊処理※]
		// 参照ユーザ無し＆確定経路＆待機経路 の場合データ入れ替え作業実施
		if ((0 == m_RtMng[i].rtCont[aRtType].user) && (m_ReleaseRtId == aRtId) && (m_RtMngPre.rtId == aRtId)
				&& (aRtType == m_RtMngPre.rtType)) {

			// 旧経路開放
			if (NULL != m_RtMng[i].rtCont[aRtType].rt) {
				RM_RouteFree(m_RtMng[i].rtCont[aRtType].rt);
			}

			// 待機経路設定
			m_RtMng[i].rtCont[aRtType].rt = m_RtMngPre.rt;
			m_RtMng[i].rtCont[aRtType].setFlag = true;
			m_RtMng[i].rtCont[aRtType].user = 0;

			// 待機経路初期化
			m_RtMngPre.rtId = SC_RP_RTIDINIT;
			m_RtMngPre.rtType = SC_RP_RTTYPEINIT;
			m_RtMngPre.rt = NULL;
		}
		break;
	}
	// [排他制御終了]
	RM_MUTEX_UNLOCK(m_Mutex);

	SC_LOG_DebugPrint(SC_TAG_RM, SC_LOG_END);
	return (result);
}

/**
 * @brief	経路参照開始
 * @param	[I]経路ID
 * @param	[I]経路種別
 * @param	[I]ユーザ
 * @param	[O]データアドレス
 * @return	e_SC_RESULT_SUCCESS 取得成功
 * @return	e_SC_RESULT_SUCCESS以外 取得失敗
 * @memo	指定された経路ID・経路種別のアドレスを取得する。
 */
E_SC_RESULT SC_RP_ReadRouteEntry(UINT32 aRtId, UINT32 aRtType, UINT32 aUser, SC_RP_RouteMng** aRtBin) {
	SC_LOG_DebugPrint(SC_TAG_RM, SC_LOG_START);

	UINT32 i;
	E_SC_RESULT result = e_SC_RESULT_FAIL;
	SC_LOG_DebugPrint(SC_TAG_RM, "Route entry user user=0x%08x "HERE, aUser);

	// チェック
	if (NULL == aRtBin || SC_RP_RTIDINIT == aRtId || SCRM_RTTYPEMAX <= aRtType) {
		SC_LOG_ErrorPrint(SC_TAG_RM, "Param error. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	// [排他制御開始]
	RM_MUTEX_LOCK(m_Mutex);

	for (i = 0; i < SCRM_RTBUFMAX; i++) {
		// ID
		if (aRtId != m_RtMng[i].rtId) {
			continue;
		}
		// データ
		if (false == m_RtMng[i].rtCont[aRtType].setFlag) {
			result = e_SC_RESULT_FAIL;
			break;
		}
		// ユーザ登録
		m_RtMng[i].rtCont[aRtType].user |= aUser;
		// 結果格納
		*aRtBin = m_RtMng[i].rtCont[aRtType].rt;
		// 成功
		result = e_SC_RESULT_SUCCESS;
		break;
	}

	// [排他制御終了]
	RM_MUTEX_UNLOCK(m_Mutex);

	SC_LOG_DebugPrint(SC_TAG_RM, SC_LOG_END);
	return (result);
}

/**
 * @brief	経路登録処理
 * @param	[I]経路ID
 * @param	[I]経路種別
 * @param	[I]経路ポインタ
 * @param	[I]サイズ
 * @memo	同一経路IDが存在しない場合は新規登録
 * @memo	同一経路IDが存在している場合は更新登録とする
 */
E_SC_RESULT SC_RP_RouteAdd(UINT32 aRtId, UINT32 aRtType, SC_RP_RouteMng* aBuf) {
	SC_LOG_DebugPrint(SC_TAG_RM, SC_LOG_START);

	UINT32 vacant = ALL_F32;
	UINT32 setIdx = ALL_F32;
	E_SC_RESULT result = e_SC_RESULT_SUCCESS;

	// [排他制御開始]
	RM_MUTEX_LOCK(m_Mutex);

	// 同じIDいるか検索
	if (e_SC_RESULT_FAIL == RM_RouteIdSearch(aRtId, &setIdx)) {
		// 空きスペース取得
		if (e_SC_RESULT_FAIL == RM_VacantIdxSearch(&vacant)) {
			SC_LOG_ErrorPrint(SC_TAG_RM, "Free spase can't find. "HERE);
			// 空きスペースが無い＝未開放ユーザの可能性
			// TODO 1番地の領域を強制開放する
		} else {
			setIdx = vacant;
		}
	}

	if (ALL_F32 != setIdx) {
		// ID設定
		m_RtMng[setIdx].rtId = aRtId;
		// user,usingチェック
		if (0 == m_RtMng[setIdx].rtCont[aRtType].user) {
			if (NULL != m_RtMng[setIdx].rtCont[aRtType].rt) {
				//参照ユーザ無しはデータ削除
				RM_RouteFree(m_RtMng[setIdx].rtCont[aRtType].rt);
			}
			m_RtMng[setIdx].rtCont[aRtType].user = 0;
			m_RtMng[setIdx].rtCont[aRtType].setFlag = true;
			m_RtMng[setIdx].rtCont[aRtType].rt = aBuf;
		} else {
			// [異常ケース] 確定経路の場合上書き待機扱いデータとして保持
			//              使用中のユーザが参照終了後にデータを移動
			if (NULL != m_RtMngPre.rt) {
				RM_RouteFree(m_RtMngPre.rt);
			}
			m_RtMngPre.rtId = aRtId;
			m_RtMngPre.rtType = aRtType;
			m_RtMngPre.rt = aBuf;
		}
	}

	// [排他制御終了]
	RM_MUTEX_UNLOCK(m_Mutex);

	SC_LOG_DebugPrint(SC_TAG_RM, SC_LOG_END);
	return (result);
}

/**
 * @brief	経路削除処理
 * @param	[I]経路ID
 * @param	[I]経路種別
 */
E_SC_RESULT SC_RP_RouteDelete(UINT32 aRtId, UINT32 aRtType) {
	SC_LOG_DebugPrint(SC_TAG_RM, SC_LOG_START);

	UINT32 i,e;
	UINT32 user;
	Bool using;

	// 確定経路の場合削除しない
	// （確定経路を削除する場合にはSC_RP_RouteSetIdのRT_RTIDINIT指定を使用）
	if (aRtId == m_ReleaseRtId) {
		SC_LOG_ErrorPrint(SC_TAG_RM, "ParamId is ReleaseRtId. type=%d "HERE, aRtId);
		return (e_SC_RESULT_FAIL);
	}
	if (SCRM_RTTYPEMAX <= aRtType) {
		SC_LOG_ErrorPrint(SC_TAG_RM, "RtType unknown. type=%d "HERE, aRtType);
		return (e_SC_RESULT_BADPARAM);
	}
	// [排他制御開始]
	RM_MUTEX_LOCK(m_Mutex);

	// 経路検索
	for (i = 0; i < SCRM_RTBUFMAX; i++) {
		if (aRtId == m_RtMng[i].rtId) {
			// ユーザ確認
			user = m_RtMng[i].rtCont[aRtType].user;
			if (user) {
				SC_LOG_DebugPrint(SC_TAG_RM, "user is route watching. user=0x%08x "HERE, user);
				break;
			}
			if (false == m_RtMng[i].rtCont[aRtType].setFlag) {
				SC_LOG_DebugPrint(SC_TAG_RM, "this route can't found. user=0x%08x "HERE, user);
				break;
			}
			if (NULL != m_RtMng[i].rtCont[aRtType].rt) {
				RM_RouteFree(m_RtMng[i].rtCont[aRtType].rt);
			}
			m_RtMng[i].rtCont[aRtType].rt = NULL;
			m_RtMng[i].rtCont[aRtType].user = 0;
			m_RtMng[i].rtCont[aRtType].setFlag = false;

			// 全データ開放済みの場合エリアを空に
			using = false;
			for (e = 0; e < SCRM_RTTYPEMAX; e++) {
				if (true == m_RtMng[i].rtCont[e].setFlag) {
					using = true;
					break;
				}
			}
			// データ全削除済み
			if (false == using) {
				m_RtMng[i].rtId = SC_RP_RTIDINIT;
			}
			break;
		}
	}
	// [排他制御終了]
	RM_MUTEX_UNLOCK(m_Mutex);

	SC_LOG_DebugPrint(SC_TAG_RM, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief	確定経路設定処理
 * @param	[I]確定経路ID
 * @param	[I]確定経路種別
 * @memo	現在の確定経路を削除し新規に確定経路を設定する。
 * @memo	確定経路IDにRT_RTIDINITを指定することで確定経路の削除と同義とする。
 */
E_SC_RESULT SC_RP_RouteSetId(UINT32 aRtId, UINT32 aRtType) {
	// [排他制御開始]
	RM_MUTEX_LOCK(m_Mutex);

	// 確定ID,種別が同じ場合には何もしない
	if (m_ReleaseRtId != aRtId || m_ReleaseRtType != aRtType) {

		// 確定経路削除処理（参照ユーザありの場合Cleaning時に削除される）
		RM_DeleteReleaseRoute();

		// 確定経路ID登録
		m_ReleaseRtId = aRtId;

		// 経路種別
		m_ReleaseRtType = aRtType;
	}

	// [排他制御終了]
	RM_MUTEX_UNLOCK(m_Mutex);

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief	管理テーブル掃除
 * @memo	公開経路以外＆参照ユーザ無しデータを全開放
 * @memo	待機経路も確定IDと異なっている場合開放する
 */
E_SC_RESULT SC_RP_RouteCleaning() {
	SC_LOG_DebugPrint(SC_TAG_RM, SC_LOG_START);

	UINT32 i,e;
	UINT32 user;

	// [排他制御開始]
	RM_MUTEX_LOCK(m_Mutex);

	// 管理情報全舐め
	for (i = 0; i < SCRM_RTBUFMAX; i++) {
		// 確定経路or経路無し
		if (m_ReleaseRtId == m_RtMng[i].rtId) {
			continue;
		}
		// ユーザ確認しながら削除
		user = 0;
		for (e = 0; e < SCRM_RTTYPEMAX; e++) {
			user |= m_RtMng[i].rtCont[e].user;
			if (user) {
				continue;
			}
			if (NULL != m_RtMng[i].rtCont[e].rt) {
				RM_RouteFree(m_RtMng[i].rtCont[e].rt);
			}
			m_RtMng[i].rtCont[e].rt = NULL;
			m_RtMng[i].rtCont[e].user = 0;
			m_RtMng[i].rtCont[e].setFlag = false;
		}
		// ユーザ全削除済み
		if (0 == user) {
			m_RtMng[i].rtId = SC_RP_RTIDINIT;
		}
	}
	// 待機経路用(確定IDと不一致の場合開放しておく)
	if (m_ReleaseRtId != m_RtMngPre.rtId) {
		if (NULL != m_RtMngPre.rt) {
			RM_RouteFree(m_RtMngPre.rt);
		}
		m_RtMngPre.rtId = SC_RP_RTIDINIT;
		m_RtMngPre.rtType = SC_RP_RTTYPEINIT;
		m_RtMngPre.rt = NULL;
	}

	// [排他制御終了]
	RM_MUTEX_UNLOCK(m_Mutex);

	SC_LOG_DebugPrint(SC_TAG_RM, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief	現在の確定経路IDと種別を取得する
 * @param	[O]経路ID
 * @param	[O]経路種別
 */
E_SC_RESULT SC_RP_GetCurrentRouteId(UINT32* aRtId, UINT32* aRtType) {
	SC_LOG_DebugPrint(SC_TAG_RM, SC_LOG_START);
	E_SC_RESULT result = e_SC_RESULT_FAIL;

	if (NULL == aRtId || NULL == aRtType) {
		return (e_SC_RESULT_BADPARAM);
	}
	// [排他制御開始]
	RM_MUTEX_LOCK(m_Mutex);

	if (SC_RP_RTIDINIT != m_ReleaseRtId) {
		*aRtId = m_ReleaseRtId;
		*aRtType = m_ReleaseRtType;
		result = e_SC_RESULT_SUCCESS;
	}

	// [排他制御終了]
	RM_MUTEX_UNLOCK(m_Mutex);

	SC_LOG_DebugPrint(SC_TAG_RM, SC_LOG_END);
	return (result);
}

#if 0
/**
 * @brief	ダンプ用
 */
static void RM_RouteManagerDebug()
{
	UINT8 i;
	UINT8 e;
	Char typeName[8][18] = {
		{	"RPC_RECOMMENDATION"}, {"RPC_HIGHWAY       "}, {"RPC_NORMAL        "},
		{	"RPC_TIME          "}, {"RPC_DISTANCE      "}, {"RPC_WIDTH         "},
		{	"RPC_VICS          "}, {"RPC_SIZE          "}};

	for (i = 0; i < SCRM_RTBUFMAX; i++) {
		for (e = 0; e < SCRM_RTTYPEMAX; e++) {
			SC_LOG_DebugPrint(SC_TAG_RM, "No.%d --- RtId[0x%08x] RtType[%s] User[0x%08x] Using[%2d] buf[%p]"
					, i
					, m_RtMng[i].rtId
					, &typeName[e][0]
					, m_RtMng[i].rtCont[e].user
					, m_RtMng[i].rtCont[e].setFlag
					, m_RtMng[i].rtCont[e].rt
			);
		}
	}
}
#endif
