/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCOREFUNCMNG_H_
#define SMCOREFUNCMNG_H_

#include "SMCoreCMNInternal.h"

#ifdef __SMS_APPLE__
extern SC_SEMAPHORE	*syncAPISem;
extern SC_SEMAPHORE	*syncSDSem;
#else
extern SC_SEMAPHORE	syncAPISem;
extern SC_SEMAPHORE	syncSDSem;
#endif /* __SMS_APPLE__ */

//-----------------------------------
// 関数ポインタ定義
//-----------------------------------
typedef	E_SC_RESULT (*SC_INITIALIZE_FUNC)();		// 初期化関数ポインタ
typedef	E_SC_RESULT (*SC_FINALIZE_FUNC)();			// 終了化関数ポインタ

#ifdef __cplusplus
extern "C" {
#endif
//-----------------------------------
// 外部I/F定義
//-----------------------------------
E_SC_RESULT SC_MNG_Initialize(const Char *rootDirPath, const Char *confDirPath, const Char *mapDirPath);
E_SC_RESULT SC_MNG_Finalize();
E_SC_RESULT SC_MNG_SetScaleLevel(INT32 maps, INT32 scaleLevel, FLOAT scaleRange, INT32 zoomLevel);
E_SC_RESULT SC_MNG_GetScaleLevel(INT32 maps, INT32 *scaleLevel, FLOAT *scaleRange, INT32 *zoomLevel);
E_SC_RESULT SC_MNG_GetBigTextAttr(INT32 maps, Bool *isBigText);
E_SC_RESULT SC_MNG_SetBigTextAttr(INT32 maps, Bool isBigText);
E_SC_RESULT SC_MNG_GetBigIconAttr(INT32 maps, Bool *isBigIcon);
E_SC_RESULT SC_MNG_SetBigIconAttr(INT32 maps, Bool isBigIcon);
E_SC_RESULT SC_MNG_GetLandmark(INT32 maps, Bool *isShow);
E_SC_RESULT SC_MNG_SetLandmark(INT32 maps, Bool isShow);
E_SC_RESULT SC_MNG_GetLandmarkAttr(INT32 maps, INT32 classCode, Bool *isShow);
E_SC_RESULT SC_MNG_SetLandmarkAttr(INT32 maps, INT32 classCode, Bool isShow);
E_SC_RESULT SC_MNG_GetDispMode(INT32 maps, INT32 *isDispMode);
E_SC_RESULT SC_MNG_SetDispMode(INT32 maps, INT32 isDispMode);
E_SC_RESULT SC_MNG_GetDriverMode(INT32 maps, Bool *isDriverMode);
E_SC_RESULT SC_MNG_SetDriverMode(INT32 maps, Bool isDriverMode);
E_SC_RESULT SC_MNG_GetShowCursor(INT32 maps, Bool *isShow);
E_SC_RESULT SC_MNG_SetShowCursor(INT32 maps, Bool isShow);
E_SC_RESULT SC_MNG_GetCarState(SMCARSTATE *carState, INT32 mode);
E_SC_RESULT SC_MNG_SetCarState(const SMCARSTATE *carState, INT32 mode);
E_SC_RESULT SC_MNG_GetScrollMode(Bool *mode);
E_SC_RESULT SC_MNG_SetScrollMode(Bool mode);
E_SC_RESULT SC_MNG_GetMoveMapDir(INT32 maps, FLOAT *degreeToward, INT32 *pixelStep);
E_SC_RESULT SC_MNG_SetMoveMapDir(INT32 maps, FLOAT degreeToward, INT32 pixelStep);
E_SC_RESULT SC_MNG_GetZoomStepRate(INT32 maps, FLOAT *stepRate);
E_SC_RESULT SC_MNG_SetZoomStepRate(INT32 maps, FLOAT stepRate);
E_SC_RESULT SC_MNG_GetMapCursorCoord(INT32 maps, SMGEOCOORD *geoCoord);
E_SC_RESULT SC_MNG_SetMapCursorCoord(INT32 maps, const SMGEOCOORD *geoCoord);
E_SC_RESULT SC_MNG_InitResource(INT32 maps);
E_SC_RESULT SC_MNG_GetMapViewPort(INT32 maps, SMRECT *rect);
E_SC_RESULT SC_MNG_SetMapViewPort(INT32 maps, const SMRECT *rect);
E_SC_RESULT SC_MNG_GetMapRotate(INT32 maps, INT32 *rotate);
E_SC_RESULT SC_MNG_SetMapRotate(INT32 maps, INT32 rotate);
E_SC_RESULT SC_MNG_GetGeoDistance(INT32 maps, INT32 unitPixel, FLOAT *scale);
E_SC_RESULT SC_MNG_SetGeoDistance(INT32 maps, INT32 unitPixel, FLOAT scale);
E_SC_RESULT SC_MNG_GetResolution(INT32 *width, INT32 *height);
E_SC_RESULT SC_MNG_SetResolution(INT32 width, INT32 height);
E_SC_RESULT SC_MNG_GetAllRPPlace(SMRPPOINT *allRPPoint, INT32 *allRPPointNum);
E_SC_RESULT SC_MNG_SetAllRPPlace(const SMRPPOINT *allRPPoint, INT32 allRPPointNum);
E_SC_RESULT SC_MNG_GetExistRoute(Bool *isExistRoute);
E_SC_RESULT SC_MNG_SetExistRoute(Bool isExistRoute);
E_SC_RESULT SC_MNG_GetPlanning(Bool *isPlanning);
E_SC_RESULT SC_MNG_SetPlanning(Bool isPlanning);
E_SC_RESULT SC_MNG_GetRPOption(SMRPOPTION *option);
E_SC_RESULT SC_MNG_SetRPOption(const SMRPOPTION *option);
E_SC_RESULT SC_MNG_GetRPTip(SMRPTIPINFO *tipInfo);
E_SC_RESULT SC_MNG_SetRPTip(const SMRPTIPINFO *tipInfo);
E_SC_RESULT SC_MNG_DoRoute(E_ROUTE route);
E_SC_RESULT SC_MNG_RePlan(E_ROUTE route);
E_SC_RESULT SC_MNG_DoGuide(E_GUIDE_STATUS event);
E_SC_RESULT SC_MNG_CancelPlanningRoute();
E_SC_RESULT SC_MNG_DeleteRouteResult();
E_SC_RESULT SC_MNG_RefreshMap(INT32 maps);
E_SC_RESULT SC_MNG_GetRealTimeInfo(SMREALTIMEGUIDEDATA *guideData);
E_SC_RESULT SC_MNG_SetRealTimeInfo(const SMREALTIMEGUIDEDATA *guideData);
E_SC_RESULT SC_MNG_GetGuideStatus(E_GUIDE_STATUS *status);
E_SC_RESULT SC_MNG_SetGuideStatus(E_GUIDE_STATUS status);
E_SC_RESULT SC_MNG_GetDeviationStatus(E_DEVIATION_STATUS *status);
E_SC_RESULT SC_MNG_SetDeviationStatus(E_DEVIATION_STATUS status);
E_SC_RESULT SC_MNG_GetVoiceTTS(SMVOICETTS *voiceTTS);
E_SC_RESULT SC_MNG_SetVoiceTTS(const SMVOICETTS *voiceTTS);
E_SC_RESULT SC_MNG_SetSimulationSpeed(INT32 speed);
E_SC_RESULT SC_MNG_GetSimulationSpeed(INT32 *speed);
E_SC_RESULT SC_MNG_DoSimulation(E_SC_SIMSTATE event);
E_SC_RESULT SC_MNG_GetSimulate(E_SC_SIMULATE *event);
E_SC_RESULT SC_MNG_GetSimulationStatus(E_SC_SIMSTATE *event);
E_SC_RESULT SC_MNG_SetSimulate(E_SC_SIMULATE event);
E_SC_RESULT SC_MNG_SetSimulationStatus(E_SC_SIMSTATE event);
E_SC_RESULT SC_MNG_DoTurnList();
E_SC_RESULT SC_MNG_GetTurnList(SMTURNLIST *turnList);
E_SC_RESULT SC_MNG_SetTurnList(const SMTURNLIST *turnList);
E_SC_RESULT SC_MNG_SaveShareData();
E_SC_RESULT SC_MNG_GetRouteLength(INT32 *length);
E_SC_RESULT SC_MNG_SetRouteLength(INT32 length);
E_SC_RESULT SC_MNG_GetRouteAveTime(INT32 *avetime);
E_SC_RESULT SC_MNG_SetRouteAveTime(INT32 avetime);
E_SC_RESULT SC_MNG_GetRouteHwayLength(INT32 *hwaylength);
E_SC_RESULT SC_MNG_SetRouteHwayLength(INT32 hwaylength);
E_SC_RESULT SC_MNG_GetRouteTollLength(INT32 *Tolllength);
E_SC_RESULT SC_MNG_SetRouteTollLength(INT32 Tolllength);
E_SC_RESULT SC_MNG_GetRouteTollFee(INT32 *Tollfee);
E_SC_RESULT SC_MNG_SetRouteTollFee(INT32 Tollfee);
E_SC_RESULT SC_MNG_GetDynamicGraphicSize(INT32 *Width,INT32 *Height);
E_SC_RESULT SC_MNG_SetDynamicGraphicSize(INT32 Width,INT32 Height);
E_SC_RESULT SC_MNG_GetDynamicGraphicStatus(E_DYNAMICGRAPHIC_STATUS *status);
E_SC_RESULT SC_MNG_SetDynamicGraphicStatus(E_DYNAMICGRAPHIC_STATUS status);
E_SC_RESULT SC_MNG_GetDynamicGraphicBitmap(SMBITMAPINFO *bitmapinfo);
E_SC_RESULT SC_MNG_SetDynamicGraphicBitmap(SMBITMAPINFO *bitmapinfo);
E_SC_RESULT SC_MNG_GetUDIResourcePath(Char *pathIconDir, Char *pathIconInfo);
E_SC_RESULT SC_MNG_SetUDIResourcePath(const Char *pathIconDir, const Char *pathIconInfo);
E_SC_RESULT SC_MNG_GetIconInfo(SMMAPDYNUDI *iconInfo, INT32 *iconNum);
E_SC_RESULT SC_MNG_SetIconInfo(const SMMAPDYNUDI *iconInfo, INT32 iconNum);
E_SC_RESULT SC_MNG_GetDynamicUDIDisplay(Bool *dispInfo, INT32 *dispNum);
E_SC_RESULT SC_MNG_SetDynamicUDIDisplay(const Bool *dispInfo, INT32 dispNum);
E_SC_RESULT SC_MNG_GetDemoMode(Bool *isDemoMode);
E_SC_RESULT SC_MNG_SetDemoMode(Bool isDemoMode);
E_SC_RESULT SC_MNG_OverviewMap(INT32 maps, INT32 overviewObj, SMRECT *rect);
E_SC_RESULT SC_MNG_GetZoomMode(Bool *mode);
E_SC_RESULT SC_MNG_SetZoomMode(Bool mode);
E_SC_RESULT SC_MNG_GetGuideDataVol(UINT16 *, UINT16);
E_SC_RESULT SC_MNG_GetGuideData(SMGUIDEDATA *, UINT16);

E_SC_RESULT SC_MNG_GetRouteCostInfo(SMRTCOSTINFO *guideData);
E_SC_RESULT SC_MNG_SetRouteCostInfo(const SMRTCOSTINFO *guideData);

E_SC_RESULT RG_NMG_GetDynamicGraphicBitmap(SMBITMAPINFO	*bitmapinfo);

E_SC_RESULT SC_MNG_SetVehicleType(INT32 vehicleType);
E_SC_RESULT SC_MNG_GetVehicleType(INT32 *vehicleType);
E_SC_RESULT SC_MNG_SetSaveTracksFlag(Bool isSaveTracks );
E_SC_RESULT SC_MNG_GetSaveTracksFlag(Bool *isSaveTracks );
E_SC_RESULT SC_MNG_SetDebugInfoOnSurface(Bool isOnSurface);
E_SC_RESULT SC_MNG_GetDebugInfoOnSurface(Bool *isOnSurface);
E_SC_RESULT SC_MNG_SetEchoFlag(Bool isEcho);
E_SC_RESULT SC_MNG_GetEchoFlag(Bool *isEcho);

E_SC_RESULT SC_MNG_GetGenreData(SMGENREDATA *genreData);
E_SC_RESULT SC_MNG_SetGenreData(const SMGENREDATA *genreData);

E_SC_RESULT SC_MNG_SetRegion(INT32);
E_SC_RESULT SC_MNG_GetRegion(INT32*);
E_SC_RESULT SC_MNG_SetLanguage(INT32);
E_SC_RESULT SC_MNG_GetLanguage(INT32*);

SC_SEMAPHORE *SC_MNG_GetSemId();
SC_SEMAPHORE *SC_MNG_GetSDSemId();

E_SC_RESULT SC_MNG_RouteBackup();
E_SC_RESULT SC_MNG_RouteBackupDelete();
E_SC_RESULT SC_MNG_GetRouteBackup(SMROUTEBACKUP*);

E_SC_RESULT SC_MNG_StartDrivingDiagnosing(Char *tripId);
E_SC_RESULT SC_MNG_StopDrivingDiagnosing(Char *tripId);
E_SC_RESULT SC_MNG_GetDrivingDiagnosingTripID(Char *tripId);

void SC_MNG_SetResult(E_SC_RESULT result);
void SC_MNG_SetSDResult(E_SC_RESULT result);
void SC_MNG_SetTripId(Char *tripId);
void SC_MNG_DynamicGraphicBitmapFree(Char *bitmap);

const Char *SC_MNG_GetMapDirPath();
const Char *SC_MNG_GetConfDirPath();
const Char *SC_MNG_GetApRootDirPath();

E_SC_RESULT SC_MNG_GetAreaClsCode(DOUBLE lat, DOUBLE lon, SMAREACLSCODE *areaClsCode);

E_SC_RESULT SC_MNG_SetMappingAlert(SMMAPPINGALERT *mappingAlert);
E_SC_RESULT SC_MNG_GetMappingAlert(SMMAPPINGALERT *mappingAlert);

E_SC_RESULT SC_MNG_SetTrafficInfo(SMTRAFFIC *trfInfo);
E_SC_RESULT SC_MNG_GetTrafficInfo(SMTRAFFIC *trfInfo);
E_SC_RESULT SC_MNG_RefreshTraffic(INT32 mode);

#ifdef __cplusplus
}
#endif

#endif // #ifndef SMCOREFUNCMNG_H_
