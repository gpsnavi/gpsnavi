/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#include "sms-core/SMCoreMP/SMCoreMPInternal.h"
#include "MP_Matrix.h"


// OpenGLの行列の並び(○の数字が順番)
//　　行→→→
//　　 0 1 2 3
//列 0①⑤⑨⑬
//↓ 1②⑥⑩⑭
//↓ 2③⑦⑪⑮
//↓ 3④⑧⑫⑯

//------------------------------------------------------------------------------
// 関数
//------------------------------------------------------------------------------
void MP_LoadIdentityMatrix(MPMatrix *mat)
{
	memset(mat, 0x00, sizeof(MPMatrix));
	mat->m[0][0] = 1.0f;
	mat->m[1][1] = 1.0f;
	mat->m[2][2] = 1.0f;
	mat->m[3][3] = 1.0f;
}

void MP_CopyMatrix(MPMatrix *dst, const MPMatrix *src)
{
	memcpy(dst, src, sizeof(MPMatrix));
}

void MP_MultMatrix(MPMatrix *dst, const MPMatrix *src1, const MPMatrix *src2)
{
	INT32 x;
	INT32 y;
	MPMatrix tmp;

	for (x=0; x<4; x++) {
		for (y=0; y<4; y++) {
			tmp.m[x][y] = (src1->m[0][y] * src2->m[x][0]) +
						  (src1->m[1][y] * src2->m[x][1]) +
						  (src1->m[2][y] * src2->m[x][2]) +
						  (src1->m[3][y] * src2->m[x][3]);
		}
	}

	memcpy(dst, &tmp, sizeof(MPMatrix));
}

void MP_RotateMatrix(MPMatrix *mat, const FLOAT angle, const FLOAT x, const FLOAT y, const FLOAT z)
{
	MPMatrix rotate;
	FLOAT rad = MP_DEG_TO_RAD(angle);
	FLOAT c = cosf(rad);
	FLOAT s = sinf(rad);

	MP_LoadIdentityMatrix(&rotate);

	// X軸回転
	if (x>0.0f) {
		rotate.m[1][1] = c;
		rotate.m[1][2] = s;
		rotate.m[2][1] = -s;
		rotate.m[2][2] = c;
	}
	// Y軸回転
	if (y>0.0f) {
		rotate.m[0][0] = c;
		rotate.m[0][2] = -s;
		rotate.m[2][0] = s;
		rotate.m[2][2] = c;
	}
	// Z軸回転
	if (z>0.0f) {
		rotate.m[0][0] = c;
		rotate.m[0][1] = s;
		rotate.m[1][0] = -s;
		rotate.m[1][1] = c;
	}

	MP_MultMatrix(mat, mat, &rotate);
}

void MP_TranslateMatrix(MPMatrix *mat, const FLOAT x, const FLOAT y, const FLOAT z)
{
	MPMatrix translate;

	MP_LoadIdentityMatrix(&translate);
	translate.m[3][0] = x;
	translate.m[3][1] = y;
	translate.m[3][2] = z;

	MP_MultMatrix(mat, mat, &translate);
}

void MP_ScaleMatrix(MPMatrix *mat, const FLOAT x, const FLOAT y, const FLOAT z)
{
	MPMatrix scale;

	MP_LoadIdentityMatrix(&scale);
	scale.m[0][0] = x;
	scale.m[1][1] = y;
	scale.m[2][2] = z;

	MP_MultMatrix(mat, mat, &scale);
}

void MP_OrthoMatrix(MPMatrix *mat, const FLOAT left, const FLOAT right, const FLOAT bottom, const FLOAT top, const FLOAT zNear, const FLOAT zFar)
{
	MPMatrix ortho;
	FLOAT dx = right - left;
	FLOAT dy = top - bottom;
	FLOAT dz = zFar - zNear;

	MP_LoadIdentityMatrix(&ortho);
	ortho.m[0][0] =  2.0f / dx;
	ortho.m[1][1] =  2.0f / dy;
	ortho.m[2][2] =- 2.0f / dz;
	ortho.m[3][0] =- (right + left) / dx;
	ortho.m[3][1] =- (top + bottom) / dy;
	ortho.m[3][2] =- (zFar + zNear) / dz;

	MP_MultMatrix(mat, mat, &ortho);
}
