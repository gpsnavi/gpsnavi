/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_UPD_DATA_PROGRESS_MNG_H
#define SMCCOM_UPD_DATA_PROGRESS_MNG_H

//// 地図データ更新状況
//typedef enum _E_UPDDATA_PROGRESS_STATUS {
//	CC_UPDDATA_PROGRESS_STATUS_INIT = 0,	// 初期(未更新)
//	CC_UPDDATA_PROGRESS_STATUS_BASE,		// 広域地図更新
//	CC_UPDDATA_PROGRESS_STATUS_DATA,		// プロパティデータ更新
//	CC_UPDDATA_PROGRESS_STATUS_AREA,		// エリア更新
//	CC_UPDDATA_PROGRESS_STATUS_FINISH		// 更新完了
//} E_UPDDATA_PROGRESS_STATUS;

// 地図データ更新状況（詳細）
typedef enum _CC_MAPUPDSTATUS {
	CC_MAPUPDSTATUS_INIT = 0,									// 初期(未更新)=0
	CC_MAPUPDSTATUS_MAPDL,										// 広域地図ダウンロード=1
	CC_MAPUPDSTATUS_MAPUPDATE,									// 広域地図更新=2
	CC_MAPUPDSTATUS_MAPVERUPDATE,								// 広域地図バージョン更新=3
	CC_MAPUPDSTATUS_MAPUPDATED,									// 広域地図更新完了=4
	CC_MAPUPDSTATUS_DATADL,										// プロパティデータダウンロード=5
	CC_MAPUPDSTATUS_DATAUPDATE,									// プロパティデータ更新=6
	CC_MAPUPDSTATUS_DATAVERUPDATE,								// プロパティデータバージョン更新=7
	CC_MAPUPDSTATUS_DATAUPDATED,								// プロパティデータ更新完了=8
	CC_MAPUPDSTATUS_AREADL,										// エリア地図ダウンロード=9
	CC_MAPUPDSTATUS_AREATARGZ,									// エリア地図tar.gz解凍=10
	CC_MAPUPDSTATUS_AREAUPDATE,									// エリア地図更新=11
	CC_MAPUPDSTATUS_AREAVERUPDATE,								// エリア地図バージョン更新=12
	CC_MAPUPDSTATUS_AREACLSDL,									// 地域クラス地図ダウンロード=13
	CC_MAPUPDSTATUS_AREACLSTARGZ,								// 地域クラス地図tar.gz解凍=14
	CC_MAPUPDSTATUS_AREACLSUPDATE,								// 地域クラス地図更新=15
	CC_MAPUPDSTATUS_AREACLSVERUPDATE,							// 地域クラス地図バージョン更新=16
	CC_MAPUPDSTATUS_AREAUPDATED,								// エリア更新完了=17
	CC_MAPUPDSTATUS_UPDATED,									// 更新完了=18
} CC_MAPUPDSTATUS;

// 地図データ更新状況
typedef struct _SMMAPUPDSTATUS {
	CC_MAPUPDSTATUS	status;										// ステータス
	Char			regionCode[SCC_MAPDWL_MAXCHAR_RGNCODE];		// 仕向け地
	Bool			hasUpdate[SCC_MAPDWL_KIND_NUM];				// 更新有無
																// [0]:広域地図ダウンロード有無
																// [1]:プロパティファイルダウンロード有無
	INT32			version[SCC_MAPDWL_KIND_NUM];				// 更新データのバージョン
																// [0]:広域地図ダウンロード有無
																// [1]:プロパティファイルダウンロード有無
	UINT32			areaIdx;									// 更新中のDL地域コードリストのインデックス(未使用)
//	UINT32			completeCount;								// 処理完了数
//	UINT32			totalCount;									// 処理対象総数
	Char			backetName[SCC_AWS_BACKETNAME_SIZE + 1];	// AWS S3 地図データ配置先バケット名
	Char			reserve[3];
} SMMAPUPDSTATUS;

void CC_UpdDataProgressMng_Initialize();
void CC_UpdDataProgressMng_Finalize();
void CC_UpdDataProgressMng_SetCallbackFnc(const SMMAPDLCBFNC *callbackFnc);
void CC_UpdData_ProgressCallbackDL(UINT32 num);
void CC_UpdData_ProgressCallbackTargz(UINT32 num);
void CC_UpdData_ProgressCallbackDB(UINT32 num);
void CC_UpdDataProgressMng_ClearProgress();
void CC_UpdDataProgressMng_SetProgress(const SMPROGRESSINFO *progressInfo);
void CC_UpdDataProgressMng_GetProgress(SMPROGRESSINFO *progressInfo);
void CC_UpdDataProgressMng_ClearUpdStatus();
void CC_UpdDataProgressMng_SetUpdStatus(const SMMAPUPDSTATUS *status);
void CC_UpdDataProgressMng_GetUpdStatus(SMMAPUPDSTATUS *status);
void CC_UpdDataProgressMng_SetStatus(CC_MAPUPDSTATUS status);
CC_MAPUPDSTATUS CC_UpdDataProgressMng_GetStatus();
void CC_UpdDataProgressMng_SetAreaIdx(UINT32 areaIdx);
UINT32 CC_UpdDataProgressMng_GetAreaIdx();
//void CC_UpdDataProgressMng_SetCompleteCount(UINT32 completeCount);
//UINT32 CC_UpdDataProgressMng_GetCompleteCount();
//void CC_UpdDataProgressMng_SetTotalCount(UINT32 totalCount);
//UINT32 CC_UpdDataProgressMng_GetTotalCount();
void CC_UpdDataProgressMng_SetBacketName(const Char *backetName);
void CC_UpdDataProgressMng_GetBacketName(Char *backetName);
E_SC_RESULT CC_UpdDataProgressMng_LoadUpdStatus();
E_SC_RESULT CC_UpdDataProgressMng_SaveUpdStatus();

#endif // #ifndef SMCCOM_UPD_DATA_PROGRESS_MNG_H
