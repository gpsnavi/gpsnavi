/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#pragma once

#define MP_BITSET

//-----------------------------------------------------------------------------
// クラス
//-----------------------------------------------------------------------------
/**
 * @brief  メッシュ衝突クラス
 */
class MP_CollisionMesh
{
private:
	static const int MP_DIV_MAX	= 32;	// 管理情報最大値

	// 分割最大値
	UINT32 mDivCntMax;
	// 分割数
	UINT32 mDivCnt;
	// 計算値
	FLOAT mCalcValue;
	// エリア管理テーブル
#ifndef MP_BITSET
	Bool mArea[MP_DIV_MAX][MP_DIV_MAX];
#else
	std::bitset<MP_DIV_MAX> mBitArea[MP_DIV_MAX];
#endif

	inline void reset(void) {
#ifndef MP_BITSET
		memset(mArea, 0x00, sizeof(mArea));
#else
		for (UINT16 x=0; x<MP_DIV_MAX; x++) {	mBitArea[x].reset();	}
#endif
	}

	inline bool set(UINT16 x, UINT16 y, bool val) {
#ifndef MP_BITSET
		mArea[x][y] = val;
#else
		mBitArea[x].set(y, val);
#endif
		return (val);
	}

	inline bool get(UINT16 x, UINT16 y) {
#ifndef MP_BITSET
		return (bool)mArea[x][y];
#else
		return (mBitArea[x].test(y));
#endif
	}

public:
	/**
	 * @brief コンストラクタ
	 */
	MP_CollisionMesh(void);

	/**
	 * @brief デストラクタ
	 */
	~MP_CollisionMesh(void);

	/**
	 * @brief 分割情報設定
	 */
	bool setDivCnt(UINT32 divCntMax, UINT32 divCnt);

	/**
	 * @brief 衝突設定
	 */
	bool setCollision(UINT16 x, UINT16 y);
};

