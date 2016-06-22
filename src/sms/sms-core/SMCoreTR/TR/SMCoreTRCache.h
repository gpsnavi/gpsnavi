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
 * SMCoreTRCache.h
 *
 *  Created on: 2016/01/04
 *      Author: n.kanagawa
 */
#ifndef SMCORETRCACHE_H_
#define SMCORETRCACHE_H_


//-----------------------------------
// I/F定義
//-----------------------------------
E_SC_RESULT SC_TR_CacheTblInitialize();
E_SC_RESULT SC_TR_CacheTblFinalize();

Bool SC_TR_IsCache(UINT32 parcelId);
void SC_TR_SetUser(const UINT16 user, const UINT32 parcelId, const Bool onOff);
void SC_TR_ReleaseUser(const UINT16 user);
Bool SC_TR_InsertData(const UINT16 user, const UINT32 parcelId, TR_DATA_t* pData);
Bool SC_TR_UpdateData(const UINT16 user, const UINT32 parcelId, TR_DATA_t* pData);
Bool SC_TR_ReplaceData(const UINT16 user, const UINT32 parcelId, TR_DATA_t* pData);
void SC_TR_DeleteData(const UINT16 user, const UINT32 parcelId);
void SC_TR_DeleteNoUserData();

void SC_TR_SetMainParcelID(UINT16 user, UINT32 parcelId);
UINT32 SC_TR_GetMainParcelID(const UINT16 user);

void SC_TR_CacheDebugPrint();

#endif /* SMCORETRCACHE_H_ */
