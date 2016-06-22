/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#include "SMCoreMPInternal.h"

MP_CollisionMesh::MP_CollisionMesh(void)
:mDivCntMax(0),mDivCnt(0),mCalcValue(0.0f)
{
	reset();
}

MP_CollisionMesh::~MP_CollisionMesh(void)
{
}

bool MP_CollisionMesh::setDivCnt(UINT32 divCntMax, UINT32 divCnt)
{
	if (divCnt>divCntMax || divCnt>MP_DIV_MAX) {
		return (false);
	}

	mDivCntMax = divCntMax;
	mDivCnt = divCnt;

	mCalcValue = (FLOAT)mDivCnt / (FLOAT)(mDivCntMax+1);

	// モザイク状にON/OFF設定
	for (INT32 x=0; x<mDivCnt; x++) {
		for (INT32 y=0; y<mDivCnt; y++) {
			if (0 == (x+y)%2) {
				set(x, y, true);
			} else {
				set(x, y, false);
			}
		}
	}

	return (true);
}

bool MP_CollisionMesh::setCollision(UINT16 x, UINT16 y)
{
	UINT16 indexX = 0;
	UINT16 indexY = 0;

//	if ((x > mDivCntMax) || (y > mDivCntMax)) {
//		return (false);
//	}

	indexX = (UINT16)((FLOAT)x * mCalcValue);
	indexY = (UINT16)((FLOAT)y * mCalcValue);

	if (get(indexX, indexY)) {
		return (false);
	} else {
		return (set(indexX, indexY, true));
	}
}

