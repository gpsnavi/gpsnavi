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
 * image.h
 *
 *  Created on: 2016/05/10
 *      Author:t.aikawa
 */

#include "navicore.h"

typedef struct _ICON_IMAGE{
	int n;
	struct {
		INT32 w;		// 幅
		INT32 h;		// 高さ
		UINT8 type;		// タイプ 0:bmp,1:png、
		UINT8 lvlimit;	// 最上位の表示スケール
		UINT8 spot_x;	// ホットスポットのX方向オフセット
		UINT8 spot_y;	// ホットスポットのY方向オフセット
	}info;
	int		size;
	char	*image;
} T_ICON_IMAGE;
