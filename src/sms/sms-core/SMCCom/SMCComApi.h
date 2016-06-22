/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOMAPI_H_
#define SMCCOMAPI_H_

void CC_GetTempDirPath(Char *dirPath);
E_SC_RESULT CC_Read_application_ini(SMRGNSETTING *rgn_setting, Bool isCreate, Bool *restored);	//application.iniファイルリード
E_SC_RESULT CC_Write_application_ini(const SMRGNSETTING *rgn_setting);			//application.iniファイルライト
INT32 CC_Convert_Rgn_to_ISO3166(const Char *cRegionCode);						//仕向け地コード→ISO-3166コード変換
Char* CC_Convert_ISO3166_to_Rgn(const INT32 iRegionCode);						//ISO-3166コード→仕向け地コード変換

#ifdef __SMS_APPLE__
E_SC_RESULT CC_InitializeCmnCtrl(const Char* pTerm_id, const Char* pItem_id, const Char* logDir, const Char* rootDir, const Char* dataDir, const Char* configDir, const Char* oldDataDir);	// 通信制御初期化処理
#else
E_SC_RESULT CC_InitializeCmnCtrl(const Char* pTerm_id, const Char* pItem_id, const Char* logDir, const Char* dataDir, const Char* configDir, const Char* oldDataDir);	// 通信制御初期化処理
#endif /* __SMS_APPLE__ */
void CC_FinalizeCmnCtrl();				// 通信制御終了処理
E_SC_RESULT CC_RegistrationDevice(Char* papiStatus);		// Androidナビ端末登録処理
E_SC_RESULT CC_RegistrationUser(const Char* pUsername, const Char* pMail, const Char* pId, const Char* pPassword, const Char* pLang, Char* pApiStatus);	// ユーザー登録処理
E_SC_RESULT CC_AuthenticationUser(const Char* pId, const Char* pPassword, const Char* pAppVer, Char* pGuid, Char* pLang, Char* pPkgFlg, Char* papiStatus);	// ユーザー認証処理
E_SC_RESULT CC_AuthenticationUserInternal(SMCAL* pCal, const Char* pId, const Char* pPassword, Char* pGuid, Char* pLang, Char* pPkgFlg, Char* recvBuf, UINT32 recvBuf_sz, Char* pApiStatus, Bool isPolling);	// ユーザー認証処理
E_SC_RESULT CC_GemRegenerate(SMGEMREG *gem, Char *apiStatus);
E_SC_RESULT CC_GemSearch(const SMGEMSEARCH *gemSearch, SMGEMINFO *gemInfo, INT32 *gemInfoNum, INT32 *lastFlg, Char *apiStatus);
E_SC_RESULT CC_GemTimelineSearch(const SMGEMTIMELINESEARCH *gemSearch, SMGEMTIMELINEINFO *gemInfo, INT32 *gemInfoNum, INT32 *lastFlg, Char *apiStatus);
E_SC_RESULT CC_GemIdSearch(const Char *gemId, SMGEMINFO *gemInfo, Char *apiStatus);
E_SC_RESULT CC_GemUpdate(SMGEMUPD *gem, Char *apiStatus);
E_SC_RESULT CC_GemDelete(SMGEMDEL *gem, Char *apiStatus);
E_SC_RESULT CC_GemLikeReg(const Char *gemId, LONG *likeCnt, Char *apiStatus);
E_SC_RESULT CC_GemLikeCncl(const Char *gemId, LONG *likeCnt, Char *apiStatus);
E_SC_RESULT CC_SearchUser(const SMUSERSRCHQUALI *SrchQuali, SMUSERBASEINFO *BaseInf, INT32 *userNum, Char *pApiSts);	// ユーザ検索
E_SC_RESULT CC_SearchUser_Polling(const SMUSERSRCHQUALI *SrchQuali, SMUSERBASEINFO *BaseInf, INT32 *userNum, Char *pApiSts);	// ユーザ検索
E_SC_RESULT CC_SearchRoom(const SMROOMSRCHQUALI *SrchQuali, SMROOMINFO *roomInf, INT32 *roomNum, Char *pApiSts);	// ルーム検索
E_SC_RESULT CC_SearchRoom_Polling(const SMROOMSRCHQUALI *SrchQuali, SMROOMINFO *roomInf, INT32 *roomNum, Char *pApiSts);	// ルーム検索
E_SC_RESULT CC_SendChatMsg(const SMCHATMESSAGE *ChatMsg, Char *pApiSts);	// チャット発信
E_SC_RESULT CC_GetChatMsg(const SMCHATGETQUALI *GetQuali, Char *pFilePath, Char *pApiSts);	// チャット情報取得
E_SC_RESULT CC_GetChatMsg_Polling(const SMCHATGETQUALI *GetQuali, Char *pFilePath, Char *pApiSts);	// チャット情報取得
E_SC_RESULT CC_CreateChatRoom(const SMCREATEROOMINF *ChatRoomInf, Char *roomNo, Char *pApiSts);	// ルーム作成
E_SC_RESULT CC_ChangeRoomName(const SMCHANGEROOMNAME *ChangeRoomName, Char *pApiSts);	// ルーム名変更
E_SC_RESULT CC_SearchRoomMember(const Char *roomNo, SMMEMBERINFO *MemberInf, INT32 *userNum, Char *pApiSts);	// ルームメンバ検索
E_SC_RESULT CC_SearchUnreadMsgRoom(SMUNREADMSGROOM *unrdMsgRm, INT32 *getnum, Char *pApiSts);	// 未読メッセージ有ルーム検索
E_SC_RESULT CC_SearchUnreadMsgRoom_Polling(SMUNREADMSGROOM *unrdMsgRm, INT32 *getnum, Char *pApiSts);	// 未読メッセージ有ルーム検索
E_SC_RESULT CC_GetUserSig(Char *user_sig);	// User_sig取得
E_SC_RESULT CC_TremsApprovalInfo(SMTERMSINFO *termsInfo, Bool isDownload, const Char *lang);
E_SC_RESULT CC_GetUserInfo(Char *userName, Char *id, Char *password);
E_SC_RESULT CC_GetUserAttrInfo(const Char *srchGuid, SMUSERPROFILE *userInfo, Char *apiStatus);
E_SC_RESULT CC_GetUserAttrInfo_Polling(const Char *srchGuid, SMUSERPROFILE *userInfo, Char *apiStatus);
E_SC_RESULT CC_UpdatePosInfo(const SMPOSITIONINFO *pos, Char *apiStatus);
E_SC_RESULT CC_UpdatePosInfo_Polling(const SMPOSITIONINFO *pos, Char *apiStatus);
E_SC_RESULT CC_SearchPosInfo(const Char *roomNo, SMPOSITIONINFO *posInfo, INT32 *posInfoNum, Char *apiStatus);
E_SC_RESULT CC_SearchPosInfo_Polling(const Char *roomNo, SMPOSITIONINFO *posInfo, INT32 *posInfoNum, Char *apiStatus);
E_SC_RESULT CC_SaveAppconf(const Char *appconf, INT32 appconfSize, Char *apiStatus);
E_SC_RESULT CC_GetAppconf(Char *appconf, INT32 *appconfSize, Char *apiStatus);
E_SC_RESULT CC_SetComBackupData(INT32 type, const Char *bkData, INT32 bkDataSize);
E_SC_RESULT CC_GetComBackupData(INT32 type, Char *bkData, INT32 *bkDataSize);
E_SC_RESULT CC_GemspotKwdSearch(const SMGEMSPOTKWDSEARCH *gemSpotSerch, SMGEMSPOT *gemInfo, INT32 *gemInfoNum, INT64 *gemAllNum, Char *apiStatus);
E_SC_RESULT CC_GemspotIdSearch(INT64 gemSpotId, SMGEMSPOTINFO *gemSpotInfo, Char *apiStatus);
E_SC_RESULT CC_NotifyReg(const SMGCMPUSHINFO *pushInfo, Char *apiStatus);
E_SC_RESULT CC_PrjNoReq(Char *projectNo, Char *apiStatus);
E_SC_RESULT CC_ReAuthreq(SMCAL *cal, const Char *userSig, T_CC_CMN_SMS_API_PRM *parm, Char *recvBuf, UINT32 recvBufSize, Char *apiSts, Bool isPolling);
E_SC_RESULT CC_GetPersonalInfo(INT32 offset, INT32 limit, INT32 status, SMPERSONALINFO *personalInfo, INT32 *personalInfoNum, Char *personalNum, Char *apiStatus);
E_SC_RESULT CC_UpdatePersonalInfo(INT32 *msgGuidList, INT32 msgGuidListNum, INT32 type, Char *apiStatus);
E_SC_RESULT CC_GroupSearch(INT32 type, const Char *groupName, INT32 offset, INT32 limit, SMGROUP *groupInfo, INT32 *groupInfoNum, Char *groupNum, Char *apiStatus);
E_SC_RESULT CC_RegGemComment(const Char *gemId, const Char *comment, Char *apiStatus);
E_SC_RESULT CC_UpdGemComment(const Char *gemId, INT32 commentId, const Char *comment, Char *apiStatus);
E_SC_RESULT CC_DelGemComment(const Char *gemId, INT32 commentId, Char *apiStatus);
E_SC_RESULT CC_GetGemComment(const Char *gemId, INT32 limit, SMGEMCOMMENTINFO *gemInfo, INT32 *gemInfoNum, Char *apiStatus);
E_SC_RESULT CC_GetGemLike(const Char *gemId, INT32 limit, SMGEMLIKEINFO *likeInfo, INT32 *likeInfoNum, Char *apiStatus);
E_SC_RESULT CC_GetNoticeList(const Char *lang, INT32 type, INT32 limit, SMNOTICEINFO *noticeInfo, INT32 *noticeInfoNum, Char *noticeNum, Char *apiStatus);
E_SC_RESULT CC_GetNotice(INT32 noticeId, Char *notice, Char *apiStatus);
E_SC_RESULT CC_AgreeAuthenticationUser(const Char* pId, const Char* pPassword, Char* pGuid, Char* pLang, Char* pApiSts);
E_SC_RESULT CC_UserSearch(const SMUSERSRCH *userSearch, SMUSERINFO *userInfo, INT32 *userInfoNum, Char *userNum, Char *pApiSts);	// ユーザ検索
E_SC_RESULT CC_GetAuthInfo(Char **termId, Char **termSig, Char **guid, Char **userSig);
E_SC_RESULT CC_GetAuthSessionCookie(Bool isReAuth, Char **sessionCookie);
INT32 CC_GetShowRatingReqDialog();
E_SC_RESULT CC_RegRating(INT32 rating, Char *apiStatus);
void CC_SetShowRatingReqDialog(INT32 isShowRatingReqDialog);
E_SC_RESULT CC_GetDaialogStatus(const Char *lang, const Char *appVer, T_CC_DAIALOGSTATUS_INFO *dlgStatusInfo);
E_SC_RESULT CC_GetAWSBucketInfo(SMCAL *smcal, E_HTTP_METHOD method, E_AWS_BUCKET backet, const Char *filePath, SMAWSINFO *aws);
E_SC_RESULT CC_GetDriveCheck(const Char *tripId, Char *filePath);
E_SC_RESULT CC_GetDriveCheckList(const Char *tripId, INT32 limit, Char *filePath);
E_SC_RESULT CC_UpdateDriveCheckStar(const Char *tripId, INT32 starNum, Char *filePath);
E_SC_RESULT CC_RegMapping(SMMAPPINGINFO *mappingInfo, Char *apiStatus);
E_SC_RESULT CC_UpdMapping(SMMAPPINGINFO *mappingInfo, Char *apiStatus);
E_SC_RESULT CC_DelMapping(const Char *mappingId, Char *apiStatus);
E_SC_RESULT CC_SearchMapping(const SMMAPPINGSRCH *mappingSrch, SMMAPPINGSRCHRES *mappingInfo, INT32 *mappingInfoNum, LONG *allNum, INT32 *lastFlg, Char *apiStatus);
E_SC_RESULT CC_SearchMappingId(const Char *mappingId, SMMAPPINGIDSRCHRES *mappingInfo, Char *apiStatus);
E_SC_RESULT CC_GetMappingHistory(const Char *mappingId, INT32 startPos, INT32 maxCnt, SMMAPPINGHISTORY *mappingInfo, INT32 *mappingInfoNum, LONG *allNum, Char *apiStatus);
E_SC_RESULT CC_RegMappingComment(const Char *mappingId, const Char *comment, Char *apiStatus);
E_SC_RESULT CC_UpdMappingComment(const Char *mappingId, INT32 commentId, const Char *comment, Char *apiStatus);
E_SC_RESULT CC_DelMappingComment(const Char *mappingId, INT32 commentId, Char *apiStatus);
E_SC_RESULT CC_GetMappingComment(const Char *mappingId, INT32 startPos, INT32 maxCnt, SMMAPPINGCOMMENTINFO *commentInfo, INT32 *commentInfoNum, LONG *allNum, Char *apiStatus);
E_SC_RESULT CC_RegMappingLike(const Char *mappingId, LONG *likeCnt, Char *apiStatus);
E_SC_RESULT CC_DelMappingLike(const Char *mappingId, LONG *likeCnt, Char *apiStatus);
E_SC_RESULT CC_GetMappingLike(const Char *mappingId, INT32 startPos, INT32 maxCnt, SMMAPPINGLIKEINFO *likeInfo, INT32 *likeInfoNum, LONG *allNum, Char *apiStatus);
E_SC_RESULT CC_RegMappingRate(const Char *mappingId, INT32 ratingOwn, Char *rating, Char *apiStatus);
E_SC_RESULT CC_GetMappingRate(const Char *mappingId, INT32 rateType, INT32 startPos, INT32 maxCnt, SMMAPPINGRATEINFO *rateInfo, INT32 *rateInfoNum, LONG *allNum, Char *apiStatus);
E_SC_RESULT CC_GetParkingrate(INT32 companyId, INT32 storeId, SMSTOREPARKINGINFO *parking, Char *apiStatus);
E_SC_RESULT CC_RegAccessCode(const Char *accessCode, const Char *dirPath, SMPACKAGEGROUPINFO *pkgInfo, Char *apiStatus);
E_SC_RESULT CC_ReleaseAccessCode(const Char *accessCode, Char *apiStatus);
E_SC_RESULT CC_GetPackageList(const Char *dirPath, Bool isDownLoad, SMPACKAGEGROUPINFO *pkgInfo, INT32 *pkgInfoNum, Char *apiStatus);
E_SC_RESULT CC_GetPackageListMgr(const Char *dirPath, SMPACKAGEGROUPINFO *pkgInfo);
E_SC_RESULT CC_LockPackage(const Char *packagePath);
E_SC_RESULT CC_UnLockPackage(SMPACKAGEINFO *pkgInfo, const Char *packagePath);
E_SC_RESULT CC_UnLockPackageMgr(SMPACKAGEINFO *pkgInfo, const Char *packagePath);
E_SC_RESULT CC_UpdatePassword(const Char *password, const Char *newPassword, Char *apiStatus);
E_SC_RESULT CC_GetPassword(Char *password);
E_SC_RESULT CC_GetProbeInfo(const SMPROBESRCH *probeSrch, SMPROBEINFO *probeInfo, SMPROBEINFONUM *probeInfoNum, Char *apiStatus);

E_SC_RESULT CC_CheckMapUpdate(SMCHECKUPDINFO *chkUpdInf, Char *apiStatus);
E_SC_RESULT CC_GetRegionList(SMRGNINFO *rgnI, INT32 *rgnNum, Char *apiStatus);
E_SC_RESULT CC_GetAreaList(const Char *rgnCode, SMSECINFO *sectI, INT32 *sectNum, INT32 *areaNum, Char *apiStatus);
E_SC_RESULT CC_UpdateData(SMUPDINFO *mapUpdInfo, SMDLERRORINFO *errInfo, Char *apiStatus);
E_SC_RESULT CC_GetRegionSetting(SMRGNSETTING *rgn_setting);
E_SC_RESULT CC_ChangeNowRegion(const Char *rgnCode);
void CC_GetProgress(SMDLPROGRESS *dlProgress);

E_SC_RESULT CC_NotifyProbePostComp(SMCAL *smcal_prb, const Char *fileName, const Char *bucketName);	// プローブUP完了通知
E_SC_RESULT CC_GetTrafficInfo(const SMTRAFFICSRCH *trafficSrch, SMTRAFFICINFO *trafficInfo);
E_SC_RESULT CC_GetOAuthUrl(const Char *serviceId, Char *url, Char *sessionId, Char *apiStatus);
E_SC_RESULT CC_ReqPkgCommon(const SMPKGCOMMONPARAM *param, Char *jsonFilePath);
E_SC_RESULT CC_CheckTamperingPkg(const Char *pkgName, const Char *pkgVer, const Char *pkgDirPath);
E_SC_RESULT CC_GetPkgWhiteList(const Char *pkgName, const Char *pkgVer);

#endif /* SMCCOMAPI_H_ */
