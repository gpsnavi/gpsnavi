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
 * RP_LinkLevelConvert.h
 *
 *  Created on: 2016/01/25
 *      Author: masutani
 */

#ifndef RP_LINKLEVELCONVERT_H_
#define RP_LINKLEVELCONVERT_H_

typedef struct _SCRP_LVCHANGE_RES {
	UINT32 parcelId;						// パーセルID
	UINT32 linkId;							// リンクID
	UINT32 formOfs;							// 形状オフセット
} SCRP_LVCHANGE_RES;
typedef struct _SCRP_LVCHANGE_PARAM {
	UINT32 parcelId;						// パーセルID
	UINT32* linkId;							// Lv2リンクID
	UINT32 linkIdVol;						// Lv2リンクID数
	SCRP_LVCHANGE_RES* resLinkInfo;			// 応答テーブルアドレス
	UINT32 resLinkInfoVol;					// 応答テーブル格納数
} SCRP_LVCHANGE_TBL;

#endif /* RP_LINKLEVELCONVERT_H_ */
