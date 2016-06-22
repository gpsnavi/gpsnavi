/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef _MP_MATRIX_H
#define _MP_MATRIX_H


//------------------------------------------------------------------------------
// 構造体
//------------------------------------------------------------------------------
/**
 * @brief  マトリクス
 */
typedef struct {
	FLOAT   m[4][4];
} MPMatrix;


//------------------------------------------------------------------------------
// プロトタイプ宣言
//------------------------------------------------------------------------------
/**
 * @brief		単位行列をロード
 * @param[out]	mat マトリクス
 */
void MP_LoadIdentityMatrix(MPMatrix *mat);

/**
 * @brief		行列をコピー
 * @param[out]	dst コピー先マトリクス
 * @param[in]	src コピー元マトリクス
 */
void MP_CopyMatrix(MPMatrix *dst, const MPMatrix *src);

/**
 * @brief		現在の行列と指定した行列との積を算出
 * @param[out]	dst src1とsrc2の積
 * @param[in]	src1 現在マトリクス
 * @param[in]	src2 指定マトリクス
 */
void MP_MultMatrix(MPMatrix *dst, const MPMatrix *src1, const MPMatrix *src2);

/**
 * @brief		現在の行列を回転した行列を算出
 * @param[io]	mat 現在のマトリクス→回転後のマトリクス
 * @param[in]	angle 角度
 * @param[in]	x X方向
 * @param[in]	y Y方向
 * @param[in]	z Z方向
 */
void MP_RotateMatrix(MPMatrix *mat, const FLOAT angle, const FLOAT x, const FLOAT y, const FLOAT z);

/**
 * @brief		現在の行列を平行移動した行列を算出
 * @param[io]	mat 現在のマトリクス→平行移動後のマトリクス
 * @param[in]	x X方向
 * @param[in]	y Y方向
 * @param[in]	z Z方向
 */
void MP_TranslateMatrix(MPMatrix *mat, const FLOAT x, const FLOAT y, const FLOAT z);

/**
 * @brief		現在の行列を拡大縮小した行列を算出
 * @param[io]	mat 現在のマトリクス→拡大縮小後のマトリクス
 * @param[in]	x X方向拡大率
 * @param[in]	y Y方向拡大率
 * @param[in]	z Z方向拡大率
 */
void MP_ScaleMatrix(MPMatrix *mat, const FLOAT x, const FLOAT y, const FLOAT z);

/**
 * @brief		平行投影変換行列を算出
 * @param[io]	mat 現在のマトリクス→平行投影変換後のマトリクス
 * @param[in]	left Xの左端
 * @param[in]	right Xの右端
 * @param[in]	bottom Yの下端
 * @param[in]	top Yの上端
 * @param[in]	zNear Zの手前
 * @param[in]	zFar Zの奥
 */
void MP_OrthoMatrix(MPMatrix *mat, const FLOAT left, const FLOAT right, const FLOAT bottom, const FLOAT top, const FLOAT zNear, const FLOAT zFar);


#endif	// _MP_MATRIX_H
