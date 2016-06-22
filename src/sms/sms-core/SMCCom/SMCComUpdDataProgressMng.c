/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#include "SMCComInternal.h"

#define	CC_UPDSTATUSINFO_FILE				"updStatusInfo.bin"

// 進捗通知処理種別
typedef enum _CC_PROGRESS_KIND {
	CC_PROGRESS_KIND_DL = 0,
	CC_PROGRESS_KIND_TARGZ,
	CC_PROGRESS_KIND_DB
} CC_PROGRESS_KIND;

static SMMAPDLCBFNC		smMapDLCBFnc;
static SMPROGRESSINFO	smProgressInfo;
static SMDLPROGRESS		smDlProgress;
static SMMAPUPDSTATUS	smUpdStatus;
static Char				stausFilePath[SCC_MAX_PATH];

static void CC_UpdData_ProgressCallback(UINT32 num, CC_PROGRESS_KIND kind);

/**
 * @brief 進捗状況初期化
 * @return 処理結果(E_SC_RESULT)
 */
void CC_UpdDataProgressMng_Initialize()
{
	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	// 初期化
	memset(&smProgressInfo, 0, sizeof(SMPROGRESSINFO));
	smMapDLCBFnc.cancel   = NULL;
	smMapDLCBFnc.progress = NULL;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}

/**
 * @brief 進捗状況終了化
 * @param[in]
 */
void CC_UpdDataProgressMng_Finalize()
{
	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}

/**
 * @brief コールバック関数設定
 * @param[in] callbackFnc   コールバック関数
 */
void CC_UpdDataProgressMng_SetCallbackFnc(const SMMAPDLCBFNC *callbackFnc)
{
	if (!CC_ISNULL(callbackFnc)) {
		memcpy(&smMapDLCBFnc, callbackFnc, sizeof(SMMAPDLCBFNC));
	}
}

/**
 * @brief ダウンロード進捗通知コールバック関数
 * @param[in] num   進捗[byte]
 */
void CC_UpdData_ProgressCallbackDL(UINT32 num)
{
	CC_UpdData_ProgressCallback(num, CC_PROGRESS_KIND_DL);
}

/**
 * @brief tar.gz解凍進捗通知コールバック関数
 * @param[in] num   進捗[byte]
 */
void CC_UpdData_ProgressCallbackTargz(UINT32 num)
{
	CC_UpdData_ProgressCallback(num, CC_PROGRESS_KIND_TARGZ);
}

/**
 * @brief DB更新進捗通知コールバック関数
 * @param[in] num   進捗[byte]
 */
void CC_UpdData_ProgressCallbackDB(UINT32 num)
{
	CC_UpdData_ProgressCallback(num, CC_PROGRESS_KIND_DB);
}

/**
 * @brief 進捗状況クリア
 */
void CC_UpdDataProgressMng_ClearProgress()
{
	memset(&smProgressInfo, 0, sizeof(SMPROGRESSINFO));
}

/**
 * @brief 進捗状況設定
 * @param[in] progressInfo  進捗状況
 */
void CC_UpdDataProgressMng_SetProgress(const SMPROGRESSINFO *progressInfo)
{
	if (!CC_ISNULL(progressInfo)) {
		memcpy(&smProgressInfo, progressInfo, sizeof(SMPROGRESSINFO));
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"doneSize=%lld, " HERE, smProgressInfo.doneSize);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"totalSize=%lld, " HERE, smProgressInfo.totalSize);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"completeCount=%u, " HERE, smProgressInfo.completeCount);
		SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"totalCount=%u, " HERE, smProgressInfo.totalCount);
	}
}

/**
 * @brief 進捗状況取得
 * @param[out] progressInfo 進捗状況
 */
void CC_UpdDataProgressMng_GetProgress(SMPROGRESSINFO *progressInfo)
{
	if (!CC_ISNULL(progressInfo)) {
		memcpy(progressInfo, &smProgressInfo, sizeof(SMPROGRESSINFO));
	}
}

/**
 * @brief 更新状況クリア
 */
void CC_UpdDataProgressMng_ClearUpdStatus()
{
	memset(&smUpdStatus, 0, sizeof(SMMAPUPDSTATUS));
	remove(stausFilePath);
}

/**
 * @brief 更新状況設定
 * @param[in] status    更新状況
 */
void CC_UpdDataProgressMng_SetUpdStatus(const SMMAPUPDSTATUS *status)
{
	if (!CC_ISNULL(status)) {
		memcpy(&smUpdStatus, status, sizeof(SMMAPUPDSTATUS));
	}
}

/**
 * @brief 更新状況取得
 * @param[out] status   更新状況
 */
void CC_UpdDataProgressMng_GetUpdStatus(SMMAPUPDSTATUS *status)
{
	if (!CC_ISNULL(status)) {
		memcpy(status, &smUpdStatus, sizeof(SMMAPUPDSTATUS));
	}
}

/**
 * @brief 更新状況読み込み
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_UpdDataProgressMng_LoadUpdStatus()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	FILE	*fp = NULL;
	struct	stat st = {};
	INT32	readSize = 0;

	do {
		sprintf(stausFilePath, "%s%s%s", SCC_GetRootDirPath(), CC_CMN_TEMP_DIR_PATH, CC_UPDSTATUSINFO_FILE);
		if (0 == stat(stausFilePath, &st)) {
			// ファイルオープン
			fp = fopen(stausFilePath, "rb+");
			if (NULL == fp) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"fopen error, " HERE);
				ret = e_SC_RESULT_FILE_OPENERR;
				break;
			}

			// TODO:ファイルから進捗状況取得
			readSize = fread(&smUpdStatus, 1, sizeof(SMMAPUPDSTATUS), fp);
			if (sizeof(SMMAPUPDSTATUS) != readSize) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"fread error, " HERE);
				ret = e_SC_RESULT_FILE_ACCESSERR;
				break;
			}
			//SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"completeCount=%u, " HERE, smUpdStatus.completeCount);
			//SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"totalCount=%u, " HERE, smUpdStatus.totalCount);
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"hasUpdate[0]=%d, " HERE, smUpdStatus.hasUpdate[0]);
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"hasUpdate[1]=%d, " HERE, smUpdStatus.hasUpdate[1]);
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"version[0]=%d, " HERE, smUpdStatus.version[0]);
			SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"version[1]=%d, " HERE, smUpdStatus.version[1]);
		}
	} while (0);

	if (NULL != fp) {
		fclose(fp);
	}

	return (ret);
}

/**
 * @brief 更新状況保存
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_UpdDataProgressMng_SaveUpdStatus()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	FILE	*fp = NULL;
	INT32	writeSize = 0;

	do {
		fp = fopen(stausFilePath, "wb");
		if (NULL == fp) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"fopen error, " HERE);
			ret = e_SC_RESULT_FILE_OPENERR;
			break;
		}
		writeSize = fwrite(&smUpdStatus, 1, sizeof(SMMAPUPDSTATUS), fp);
		if (sizeof(SMMAPUPDSTATUS) != writeSize) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"fwrite error, " HERE);
			ret = e_SC_RESULT_FILE_ACCESSERR;
			break;
		}
	} while (0);

	if (NULL != fp) {
		fclose(fp);
	}

	return (ret);
}

/**
 * @brief ステータス設定
 * @param[in] status    ステータス
 */
void CC_UpdDataProgressMng_SetStatus(CC_MAPUPDSTATUS status)
{
	smUpdStatus.status = status;
}

/**
 * @brief ステータス取得
 * @return ステータス
 */
CC_MAPUPDSTATUS CC_UpdDataProgressMng_GetStatus()
{
	return (smUpdStatus.status);
}

/**
 * @brief 処理完了数設定
 * @param[in] areaIdx   更新中のDL地域コードリストのインデックス
 */
void CC_UpdDataProgressMng_SetAreaIdx(UINT32 areaIdx)
{
	smUpdStatus.areaIdx = areaIdx;
}

/**
 * @brief 更新中のDL地域コードリストのインデックス取得
 * @return 更新中のDL地域コードリストのインデックス
 */
UINT32 CC_UpdDataProgressMng_GetAreaIdx()
{
	return (smUpdStatus.areaIdx);
}

///**
// * @brief 処理完了数設定
// * @param[in] completeCount 処理完了数
// */
//void CC_UpdDataProgressMng_SetCompleteCount(UINT32 completeCount)
//{
//	smUpdStatus.completeCount = completeCount;
//}
//
///**
// * @brief 処理完了数取得
// * @return 処理完了数
// */
//UINT32 CC_UpdDataProgressMng_GetCompleteCount()
//{
//	return smUpdStatus.completeCount;
//}
//
///**
// * @brief 処理対象総数設定
// * @param[in] totalCount    処理対象総数
// */
//void CC_UpdDataProgressMng_SetTotalCount(UINT32 totalCount)
//{
//	smUpdStatus.totalCount = totalCount;
//}
//
///**
// * @brief 処理対象総数取得
// * @return 処理対象総数
// */
//UINT32 CC_UpdDataProgressMng_GetTotalCount()
//{
//	return smUpdStatus.totalCount;
//}

void CC_UpdDataProgressMng_SetBacketName(const Char *backetName)
{
	if (!CC_ISNULL(backetName)) {
		strcpy(smUpdStatus.backetName, backetName);
	}
}

void CC_UpdDataProgressMng_GetBacketName(Char *backetName)
{
	if (!CC_ISNULL(backetName)) {
		strcpy(backetName, smUpdStatus.backetName);
	}
}

/**
 * @brief 進捗通知コールバック関数を呼び出す
 * @param[in] num   進捗状況(前回の進捗通知からの差分。累計ではない。)
 * @param[in] kind  処理種別
 */
void CC_UpdData_ProgressCallback(UINT32 num, CC_PROGRESS_KIND kind)
{
	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"num=%u, kind=%d, " HERE, num, kind);

	smProgressInfo.doneSize += num;
	if (smProgressInfo.doneSize > smProgressInfo.totalSize) {
		// 処理完了数が処理対象総数を超えた場合、処理完了数に処理対象総数を設定する
		SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"doneSize(%lld) < totalSize(%lld)" HERE, smProgressInfo.doneSize, smProgressInfo.totalSize);
		smProgressInfo.doneSize = smProgressInfo.totalSize;
	}
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"msg=%s, " HERE, smProgressInfo.msg);
	SCC_LOG_DebugPrint(SC_TAG_CC, (Char*)"doneSize=%lld, totalSize=%lld, completeCount=%u, totalCount=%u, " HERE,
					   smProgressInfo.doneSize, smProgressInfo.totalSize,
					   smProgressInfo.completeCount, smProgressInfo.totalCount);

	if (!CC_ISNULL(smMapDLCBFnc.progress)) {
		// 進捗通知する情報設定
		strcpy(smDlProgress.msg, smProgressInfo.msg);
		smDlProgress.completeCount = smProgressInfo.completeCount;
		smDlProgress.totalCount    = smProgressInfo.totalCount;
		if (CC_PROGRESS_KIND_DL == kind) {
			// ダウンロード
			smDlProgress.doneSize  = (UINT32)(smProgressInfo.doneSize  / SCC_SIZE_KB);
			smDlProgress.totalSize = (UINT32)(smProgressInfo.totalSize / SCC_SIZE_KB);
		} else if (CC_PROGRESS_KIND_TARGZ == kind) {
			// tar.gz解凍
			smDlProgress.doneSize  = (UINT32)(smProgressInfo.doneSize  / SCC_SIZE_KB);
			smDlProgress.totalSize = (UINT32)(smProgressInfo.totalSize / SCC_SIZE_KB);
		} else {
			// DB更新
			smDlProgress.doneSize  = (UINT32)smProgressInfo.doneSize;
			smDlProgress.totalSize = (UINT32)smProgressInfo.totalSize;
		}
		// 進捗通知
		smMapDLCBFnc.progress(&smDlProgress);
	}

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);
}
