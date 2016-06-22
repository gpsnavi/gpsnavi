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
 * SMCoreCMNApi.h
 *
 *  Created on: 2015/11/05
 *      Author: masutani
 */

#ifndef SMCORECMNAPI_H_
#define SMCORECMNAPI_H_

#ifdef __cplusplus
extern "C" {
#endif
/**
 * SMCoreCmn
 */
E_SC_RESULT SC_CreateMutex(SC_MUTEX *mutex);
E_SC_RESULT SC_DestroyMutex(SC_MUTEX *mutex);
E_SC_RESULT SC_LockMutex(SC_MUTEX *mutex);
E_SC_RESULT SC_UnLockMutex(SC_MUTEX *mutex);
#ifdef __SMS_APPLE__
E_SC_RESULT SC_CreateSemaphore(SC_SEMAPHORE **sem, Char *sem_name, UINT32 val);
E_SC_RESULT SC_DestroySemaphore(SC_SEMAPHORE *sem, Char *sem_name);
#else
E_SC_RESULT SC_CreateSemaphore(SC_SEMAPHORE *sem, UINT32 val);
E_SC_RESULT SC_DestroySemaphore(SC_SEMAPHORE *sem);
#endif /* __SMS_APPLE__ */
E_SC_RESULT SC_LockSemaphore(SC_SEMAPHORE *sem);
E_SC_RESULT SC_LockTimeSemaphore(SC_SEMAPHORE *sem, UINT32 sec);
E_SC_RESULT SC_UnLockSemaphore(SC_SEMAPHORE *sem);
INT32 SC_Trim(Char *str);

E_SC_RESULT SC_CheckSumCalc(UINT8*, UINT32, UINT8*);
Bool SC_CheckSumJudge(UINT8*, UINT32, UINT8);

E_SC_RESULT SC_SetBasicKeyword(Char *str1, Char *str2);
E_SC_RESULT SC_GetBasicKeyword(Char *str1, Char *str2);
E_SC_RESULT SC_MakeDir(const Char *dirPath);
Bool IsLittleEndian();
Bool CompareDouble(DOUBLE param1, DOUBLE param2);

E_SC_RESULT ConvertResult(E_SC_CAL_RESULT calRet);		//戻り値変換（E_SC_CAL_RESULT → E_SC_RESULT）

/**
 * SMCoreLog
 */
E_SC_RESULT SC_LOG_Initialize(SC_LOG_TYPE type, SC_LOG_LV lvMin, const Char *filePath);
E_SC_RESULT SC_LOG_Print(SC_LOG_LV lv, const char *tag, const char *fmt, ...);
E_SC_RESULT SC_LOG_DebugPrint(const char *tag, const char *fmt, ...);
E_SC_RESULT SC_LOG_InfoPrint(const char *tag, const char *fmt, ...);
E_SC_RESULT SC_LOG_WarnPrint(const char *tag, const char *fmt, ...);
E_SC_RESULT SC_LOG_ErrorPrint(const char *tag, const char *fmt, ...);
E_SC_RESULT SC_LOG_Finalize();

/**
 * SMCoreMem
 */
E_SC_RESULT SC_MEM_Initialize(SIZE size, E_SC_MEM_TYPE type);
E_SC_RESULT SC_MEM_Finalize(E_SC_MEM_TYPE type);
void *SC_MEM_Alloc(SIZE size, E_SC_MEM_TYPE type);
void SC_MEM_Free(void *ptr, E_SC_MEM_TYPE type);
void SC_MEM_FreeAll(E_SC_MEM_TYPE type);
void SC_MEM_Dump();
void SC_MEM_Dump_Type(E_SC_MEM_TYPE type);

/**
 * MeshCalc
 */
/**
 * @brief	パーセルIDを分割
 * @param	[in] parcelID パーセルID
 * @param	[out] pDivPcl 分割パーセル
 * @return	void
 */
void ParcelDivide(const UINT32 parcelID, T_DIVIDE_PARCEL *pDivPcl);

/**
 * @brief	分割パーセルを結合
 * @param	[in] pDivPcl 分割パーセル
 * @return	成功：パーセルID、失敗：0
 */
UINT32 ParcelCombine(T_DIVIDE_PARCEL *pDivPcl);

/**
 * @brief	パーセルIDからレベル取得
 * @param	[in] parcelID パーセルID
 * @return	成功：レベル(1～6)、失敗：0
 */
INT32 MESHC_GetLevel(const UINT32 parcelID);

/**
 * @brief	上位パーセルID取得
 * @param	[in] parcelID 自レベルパーセルID
 * @return	成功：上位レベルパーセルID、失敗：無効値
 */
UINT32 MESHC_GetUpperParcelID(const UINT32 parcelID);

/**
 * @brief	下位パーセルID取得
 * @param	[in] parcelID 自レベルパーセルID
 * @param	[out] pUnderParcelList 下位レベルパーセルIDリスト
 * @return	成功：取得パーセル数(16)、失敗：0
 */
INT32 MESHC_GetUnderParcelID(const UINT32 parcelID, T_UNDER_PARCEL_LIST *pUnderParcelList);

/**
 * @brief	任意量シフトしたパーセルID取得
 * @param	[in] parcelID 基準パーセルID
 * @param	[in] xVol x方向シフト量
 * @param	[in] yYol y方向シフト量
 * @return	成功：隣接パーセルID、失敗：無効値
 */
UINT32 MESHC_SftParcelID(const UINT32 parcelID, const INT16 xVol, const INT16 yVol);

/**
 * @brief	隣接パーセルID取得
 * @param	[in] ParcelId パーセルID
 * @param	[in] dir 隣接方向
 * @return	成功：隣接パーセルID、失敗：無効値
 */
UINT32 MESHC_GetNextParcelID(const UINT32 parcelID, const UINT8 dir);

/**
 * @brief	パーセルID(正規化座標)から緯度・経度(秒)に変換する
 * @param	[in] level 変換するﾚﾍﾞﾙ
 * @param	[in] parcel_id 変換するﾊﾟｰｾﾙID
 * @param	[in] x 変換する正規化X座標
 * @param	[in] y 変換する正規化Y座標
 * @param	[out] latitude 変換後の緯度（秒）
 * @param	[out] longitude 変換後の経度（秒）
 * @return	成功：0、失敗：-1
 */
INT32 MESHC_ChgParcelIDToLatLon(const INT32 level, const UINT32 parcelID, const DOUBLE x, const DOUBLE y, DOUBLE *pLatitude,
		DOUBLE *pLongitude);

/**
 * @brief	緯度・経度(秒)からパーセルID(正規化座標)に変換する
 * @param	[in] latitude 変換する緯度（秒）
 * @param	[in] longitude 変換する経度（秒）
 * @param	[in] level 変換後のレベル
 * @param	[out] parcel_id 変換後のパーセルID
 * @param	[out] x 変換後の正規化Ｘ座標
 * @param	[out] y 変換後の正規化Ｙ座標
 * @return	成功：0、失敗：-1
 */
INT32 MESHC_ChgLatLonToParcelID(const DOUBLE latitude, const DOUBLE longitude, const INT32 level, UINT32 *pParcelID, DOUBLE *pX,
		DOUBLE *pY);

/**
 * @brief	パーセル相対位置の算出
 * @param	[in] base_parcel 基準となるパーセルID
 * @param	[in] target_parcel 対象パーセルID
 * @param	[in] level パーセルのレベル
 * @param	[out] alt_x パーセルの相対位置（Ｘ方向）
 * @param	[out] alt_y パーセルの相対位置（Ｙ方向）
 * @return	成功：0、失敗：-1
 */
INT32 MESHC_GetParcelAlterPos(const UINT32 baseParcel, const UINT32 targetParcel, const INT32 level, INT32 *pAltX, INT32 *pAltY);

/**
 * @brief	実長取得
 * @param	[in] latitude 緯度（秒）
 * @param	[in] longitude 経度（秒）
 * @param	[out] len ｍ/秒(DOUBLE len[2]のポインタ)
 * @return	void
 */
void MESHC_GetRealSize(const DOUBLE latitude, const DOUBLE longitude, DOUBLE *pLen);

/**
 * @brief	実長取得
 * @param	[in] slatitude 始点緯度（時）
 * @param	[in] slongitude 始点経度（時）
 * @param	[in] elatitude 終点緯度（時）
 * @param	[in] elongitude 終点経度（時）
 * @return	実長 ｍ/時
 */
DOUBLE MESHC_GetRealLen(const DOUBLE sLatitude, const DOUBLE sLongitude, const DOUBLE eLatitude, const DOUBLE eLongitude);

/**
 * @brief	緯度経度、ズームレベルからピクセル座標に変換する
 * @param	[in] lat 緯度（DEC）
 * @param	[in] lon 経度（DEC）
 * @param	[in] zoomLevel ズームレベル
 * @param	[out] pPixelCoordX ピクセル座標X
 * @param	[out] pPixelCoordY ピクセル座標Y
 * @return	void
 */
void MESHC_ChgLatLonToTilePixelCoord(const DOUBLE latitude, const DOUBLE longitude, const INT32 zoomLevel, DOUBLE *pPixelCoordX,
		DOUBLE *pPixelCoordY);

/**
 * @brief	緯度経度からワールド座標に変換する
 * @param	[in] lat 緯度（DEC）
 * @param	[in] lon 経度（DEC）
 * @param	[out] pWorldCoordX ワールド座標X
 * @param	[out] pWorldCoordY ワールド座標Y
 * @return	void
 */
void MESHC_ChgLatLonToTileWorldCoord(const DOUBLE latitude, const DOUBLE longitude, DOUBLE *pWorldCoordX, DOUBLE *pWorldCoordY);

/**
 * @brief	ピクセル座標、ズームレベルから緯度経度に変換する
 * @param	[in] pixelCoordX ピクセル座標X
 * @param	[in] pixelCoordY ピクセル座標Y
 * @param	[in] zoomLevel ズームレベル
 * @param	[out] pLat 緯度（DEC）
 * @param	[out] pLon 経度（DEC）
 * @return	void
 */
void MESHC_ChgTilePixelCoordToLatLon(const INT32 pixelCoordX, const INT32 pixelCoordY, const INT32 zoomLevel, DOUBLE *pLatitude,
		DOUBLE *pLongitude);

/**
 * SMCoreMeshCalc
 */
INT32	SC_MESH_GetLevel(UINT32 ParcelId);
UINT32	SC_MESH_GetUpperParcelID(const UINT32 ParcelId);
INT32	SC_MESH_GetUnderParcelID(const UINT32 ParcelId, T_UNDER_PARCEL_LIST* p_UnderParcelList);
UINT32	SC_MESH_SftParcelId(UINT32 ParcelId, INT16 x_vol, INT16 y_vol);
UINT32	SC_MESH_GetNextParcelID(UINT32 ParcelId, UINT8 Dir);
UINT32	SC_MESH_GetParcelID(INT32 lvl, INT32 pid_x, INT32 pid_y);
INT32	SC_MESH_ChgParcelIDToTitude(INT32 level, UINT32 parcel_id, DOUBLE x, DOUBLE y, DOUBLE *latitude, DOUBLE *longitude);
void	SC_MESH_GetRealSize(DOUBLE latitude, DOUBLE longitude, DOUBLE* len);
INT32	SC_MESH_GetAlterPos(UINT32 base_parcel, UINT32 target_parcel, INT32 level, INT32 *alt_x, INT32 *alt_y);
INT32	SC_Lib_ChangeTitude2PID(DOUBLE latitude, DOUBLE longitude, INT32 level, UINT32 *parcel_id, DOUBLE *x, DOUBLE *y);
DOUBLE sc_MESH_GetRealLen(DOUBLE slatitude, DOUBLE slongitude, DOUBLE elatitude, DOUBLE elongitude);
UINT32	SC_MESH_GetUnderLevelParcelID(const UINT32 baseParcelId, INT32 outLevel);

/**
 * SMCoreDataHandler
 */
E_SC_RESULT SC_DH_MemInitialize(const Char *mapDirPath, const Char *confPath);
E_SC_RESULT SC_DH_MemFinalize();
E_SC_RESULT SC_DH_GetShareData(SC_DH_SHARE_DATA *data);
E_SC_RESULT SC_DH_SetShareData(SC_DH_SHARE_DATA *data);
E_SC_RESULT SC_DH_SaveShareData();
E_SC_RESULT SC_DH_LoadShareData();
E_SC_RESULT SC_DH_RouteBackup();
E_SC_RESULT SC_DH_RouteBackupDelete();
E_SC_RESULT SC_DH_GetRouteBackup(SMROUTEBACKUP*);

/**
 * SMCoreMsg
 */
void SC_MSG_MsgDispatch(pthread_msq_msg_t *msg, const pthread_msq_id_t *msq);

/**
 * SMCoreFMThread
 */
E_SC_RESULT SC_FM_Initialize();
E_SC_RESULT SC_FM_Finalize();
void *SC_FM_ThreadMain(void *param);

#ifdef __cplusplus
}
#endif

#endif /* SMCORECMNAPI_H_ */
