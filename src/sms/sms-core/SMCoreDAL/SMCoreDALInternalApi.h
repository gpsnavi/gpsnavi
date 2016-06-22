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
 * SMCoreDALInternalApi.h
 *
 *  Created on: 2016/02/04
 *      Author: masutani
 */

#ifndef SMCOREDALINTERNALAPI_H_
#define SMCOREDALINTERNALAPI_H_

#ifdef __cplusplus
extern "C" {
#endif
/**
 * SMDALSystemInfo
 */
SC_DA_RESULT SC_DA_GetSystemInformationData(sqlite3* sqliteObj, char strBinData[], char sql_query[]);
SC_DA_RESULT SC_DA_LoadSystemInfo(sqlite3* sqliteObj, char* query, T_DAL_DBSYSTEMINFO* aInfo);
SC_DA_RESULT SC_DA_GetSystemInformationDataSize(sqlite3* sqliteObj, INT32 *pBinSize, char sql_query[]);
/**
 * SMDALParcel
 */
SC_DA_RESULT SC_DA_LoadPclData(sqlite3* sqliteObj, T_DAL_PID *pstPid, UINT32 mapKind, T_DAL_BINARY *pstBinData);
SC_DA_RESULT SC_DA_GetPclDataSize(sqlite3* sqliteObj, T_DAL_PID *pstPid, UINT32 *pBinSize, UINT16 type);
/**
 * SMDALDownloadAreaMap
 */
SC_DA_RESULT SC_DA_LoadDownloadAreaMapData(sqlite3* sqliteObj, UINT8 *kind, T_DAL_DLAREA *downloadArea, UINT8 *dataCnt);
/**
 * SMDALAreaCls
 */
SC_DA_RESULT SC_DA_LoadAreaCls(sqlite3* sqliteObj, UINT16 getLanCd, E_SC_AREA_CLS getAreaClsCode, SMAREACLSCODE* pAreaClsCode,
		T_DAL_AREA_CLS* pAreaCls);
#ifdef __cplusplus
}
#endif

#endif /* SMCOREDALINTERNALAPI_H_ */
