/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

/**
 * @file	SMCoreMeshCalc.h
 * @brief	メッシュ計算ライブラリヘッダ
 *
 * @author	n.kanagawa
 * @date
 */


#ifndef _SMCOREMESHCALC_H_
#define _SMCOREMESHCALC_H_

//-----------------------------------------------------------------------------
// 外部関数
//-----------------------------------------------------------------------------
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


#endif // _SMCOREMESHCALC_H_
