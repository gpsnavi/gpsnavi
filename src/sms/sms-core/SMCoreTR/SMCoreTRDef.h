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
 * SMCoreTRDef.h
 *
 *  Created on: 2016/01/04
 *      Author: n.kanagawa
 */
#ifndef SMCORETRDEF_H_
#define SMCORETRDEF_H_


#define TR_ROAD_KIND_MAX		16					// 道路種別最大値

// 道路種別データ
typedef struct {
	INT32				cnt;						// 情報数
	char				*pInfo;						// 情報先頭アドレス
} TR_ROADKIND_INFO_t;

// 渋滞情報
typedef struct {
	struct tm			expiryDate;					// 有効期限
	TR_ROADKIND_INFO_t	roadKind[TR_ROAD_KIND_MAX];	// 道路種別先頭アドレス
} TR_CONGESTION_INFO_t;

// SA/PA情報
// 駐車場情報
// 事象・規制情報

#endif /* SMCORETRDEF_H_ */
