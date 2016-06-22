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
 * SMCoreTRPos.h
 *
 *  Created on: 2016/01/04
 *      Author: n.kanagawa
 */
#ifndef SMCORETRPOS_H_
#define SMCORETRPOS_H_

//-----------------------------------
// I/F定義
//-----------------------------------
E_SC_RESULT SC_TR_GetCurrentPos(const UINT16 user, DOUBLE *pLat, DOUBLE *pLon);
UINT32 SC_TR_GetCurrentPosParcelID(const UINT16 user);
void SC_TR_GetAreaParcelList(const UINT32 parcelId, TR_PARCEL_LIST_t *pList);
void SC_TR_GetLevel2ParcelList(const TR_PARCEL_LIST_t *pLvl1PclList, TR_PARCEL_LIST_t *pList);

#endif /* SMCORETRPOS_H_ */
