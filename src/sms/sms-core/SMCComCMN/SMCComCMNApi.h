/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOMCMNAPI_H_
#define SMCCOMCMNAPI_H_

E_SC_RESULT SCC_CreateMutex(SCC_MUTEX *mutex);
E_SC_RESULT SCC_DestroyMutex(SCC_MUTEX *mutex);
E_SC_RESULT SCC_LockMutex(SCC_MUTEX *mutex);
E_SC_RESULT SCC_UnLockMutex(SCC_MUTEX *mutex);
E_SC_RESULT SCC_CreateSemaphore(SCC_SEMAPHORE *sem, UINT32 val);
E_SC_RESULT SCC_DestroySemaphore(SCC_SEMAPHORE *sem);
E_SC_RESULT SCC_LockSemaphore(SCC_SEMAPHORE *sem);
E_SC_RESULT SCC_LockTimeSemaphore(SCC_SEMAPHORE *sem, UINT32 sec);
E_SC_RESULT SCC_UnLockSemaphore(SCC_SEMAPHORE *sem);

#ifdef __SMS_APPLE__
void SCC_SetRootDirPath(const Char *rootDir);
#else
void SCC_SetRootDirPath(const Char *dirPath);
#endif /* __SMS_APPLE__ */
const Char *SCC_GetRootDirPath();
void SCC_SetDataDirPath(const Char *dirPath);
const Char *SCC_GetDataDirPath();
void SCC_SetConfigDirPath(const Char *dirPath);
const Char *SCC_GetConfigDirPath();
Char *SCC_ToLowerString(Char *str);
Char *SCC_ToLowerStringLen(Char *str, UINT32 len);

void SCC_Cancel();
void SCC_CancelClear();
Bool SCC_IsCancel();
void SCC_Cancel_Polling();
void SCC_CancelClear_Polling();
Bool SCC_IsCancel_Polling();
void SCC_Login();
void SCC_Logout();

E_SC_RESULT SCC_MoveFile(const Char *srcPath, const Char *dstPath);
E_SC_RESULT SCC_CopyFile(const Char *srcPath, const Char *dstPath, Bool isRemove);
E_SC_RESULT SCC_CopyFile2(const Char *srcPath, const Char *dstPath, Bool isRemove);
E_SC_RESULT SCC_CopyDir(const Char *srcDirPath, const Char *dstDirPath, Bool isRemove);
E_SC_RESULT SCC_GetImageMIMEType(const Char *filePath, CC_IMAGE_MIMETYPE *mimeType);
E_SC_RESULT CC_ChgHexString(const UChar *str, UINT32 strLen, UChar *hexStr);
Bool CC_CheckFileSize(const Char *filePath, UINT32 fileSize);
E_SC_RESULT SCC_SetConnectInfo( T_CC_CMN_CONNECT_INFO conn );

const char *SCC_GetPortalAPI();
const char *SCC_GetSmsSpAPI();
const char *SCC_GetSmsAuthAPI();
const char *SCC_GetMUPSAPI();
const char *SCC_GetKeyword1();
const char *SCC_GetKeyword2();

E_SC_RESULT CC_CmnDL_SplitAppVersion(const Char *strAppVer, CC_APP_VERSION *appVersion);
E_SC_RESULT CC_CmnDL_CheckFile(const Char *filePath, const UChar *md5, UINT32 fileSize);

E_SC_RESULT CC_UnTgz(const Char *filePath, const Char *dstPath, const SMPROGRESSCBFNC *cbFnc);
INT64 CC_GetFreeDiskSize(const Char *dirPath);

INT32 CC_ConvertStsStrToNum(const Char *apStatus);		//文字列⇒数値変換
E_SC_RESULT CC_MakeDir(const Char *dirPath);			//ディレクトリ作成
E_SC_RESULT CC_DeleteDir(const Char *dirPath);			//ディレクトリ削除
E_SC_RESULT CC_DeleteFile(const Char *filePath);		//ファイル削除
Bool CC_CheckFileExist(const Char *filePath);			//ファイルの存在チェック

// プローブUP完了通知
E_SC_RESULT SCC_NotifyProbePostComp(SMCAL *smcal, const Char *fileName, const Char *bucketName);
// ログイン有無取得
Bool SCC_IsLogined();
// 認証情報取得
E_SC_RESULT SCC_GetAuthInfo(SCC_AUTHINFO *authInfo);
// 交通情報取得
E_SC_RESULT SCC_GetTrafficInfo(const SMTRAFFICSRCH *trafficSrch, SMTRAFFICINFO *trafficInfo);

#endif /* SMCCOMCMNAPI_H_ */
