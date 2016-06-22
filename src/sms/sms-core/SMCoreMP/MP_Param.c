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

// *****************************************************************************
// 縮尺
// *****************************************************************************
// スケールパラメータ
static SCALE_PARAM_t gScaleParam[MP_SCALE_LEVEL_CNT] = {
	// ズームレベル		// パーセルレベル	// ズーム許可フラグ		// スケール	// zoomIN/OUT
	// ------------------------------------------------------------------------------------
	{19,				1,					6,						1.0f,		2.0f,	1.0f},	// 00	12m
	{19,				1,					1,						0.5f,		1.0f,	0.5f},	// 01 	25m未使用
	{19,				1,					1,						0.5f,		1.0f,	0.5f},	// 02 	25m
	{19,				1,					2,						0.25f,		0.5f,	2.0f},	// 03 	50m
	{16,				2,					2,						1.0f,		2.0f,	1.0f},	// 04 	100m
	{16,				2,					4,						0.5f,		1.0f,	2.0f},	// 05 	200m
	{14,				3,					1,						1.0f,		2.0f,	1.0f},	// 06 	500m
	{14,				3,					3,						0.5f,		1.0f,	2.0f},	// 07 	1km
	{12,				4,					1,						1.0f,		2.0f,	1.0f},	// 08 	2.5km
	{12,				4,					1,						0.5f,		1.0f,	2.0f},	// 09 	5km
	{10,				5,					1,						1.0f,		2.0f,	1.0f},	// 10 	10km
	{10,				5,					1,						0.25f,		1.0f,	4.0f},	// 11 	25km
	{ 6,				6,					1,						1.0f,		4.0f,	1.0f},	// 12 	100km
	{ 6,				6,					1,						0.25f,		4.0f,	1.0f}	// 13 	250km
};

// *****************************************************************************
// 道路パラメータ[表示縮尺][道路種別]
// *****************************************************************************
// 道路カラー[道路種別]
static MP_COLOR_t gRoadColor[MP_ROAD_KIND_CODE_MAX] = {
	{	SET_RGBA(100,127,210,255),	SET_RGBA( 28, 80,147,255)	},	// 00
	{	SET_RGBA(100,127,210,255),	SET_RGBA( 28, 80,147,255)	},	// 01
	{	SET_RGBA(100,127,210,255),	SET_RGBA( 28, 80,147,255)	},	// 02
	{	SET_RGBA(  0,  0,  0,255),	SET_RGBA(170,170,170,255)	},	// 03
	{	SET_RGBA(255,130,171,255),	SET_RGBA(222, 80,101,255)	},	// 04
	{	SET_RGBA( 85,170, 85,255),	SET_RGBA( 48,123, 96,255)	},	// 05
	{	SET_RGBA(255,222,173,255),	SET_RGBA(150,150,150,255)	},	// 06
	{	SET_RGBA(255,255,255,255),	SET_RGBA(150,150,150,255)	},	// 07
	{	SET_RGBA(255,255,255,255),	SET_RGBA(150,150,150,255)	},	// 08
	{	SET_RGBA(255,255,255,255),	SET_RGBA(150,150,150,255)	},	// 09
	{	SET_RGBA(255,255,255,255),	SET_RGBA(150,150,150,255)	},	// 10
	{	SET_RGBA(255,255,255,255),	SET_RGBA(150,150,150,255)	},	// 11
	{	SET_RGBA(150,150,150,255),	SET_RGBA(150,150,150,255)	},	// 12
	{	SET_RGBA(  0,  0,  0,255),	SET_RGBA(150,150,150,255)	},	// 13
	{	SET_RGBA(  0,  0,  0,255),	SET_RGBA(150,150,150,255)	},	// 14
	{	SET_RGBA(150,150,150,255),	SET_RGBA(150,150,150,255)	},	// 15
};

// 道路パラメータ[表示縮尺][道路種別]
static MP_PARAM_t gRoadParam[MP_SCALE_LEVEL_CNT][MP_ROAD_KIND_CODE_MAX] = {
	{	// 0
		{	SET_LINE_PARAM(24,30,1,1,1)	},	// 00
		{	SET_LINE_PARAM(24,30,1,1,1)	},	// 01
		{	SET_LINE_PARAM(24,30,1,1,1)	},	// 02
		{	SET_LINE_PARAM( 1, 1,0,0,0)	},	// 03
		{	SET_LINE_PARAM(24,30,1,1,1)	},	// 04
		{	SET_LINE_PARAM(24,30,1,1,1)	},	// 05
		{	SET_LINE_PARAM(24,30,1,1,1)	},	// 06
		{	SET_LINE_PARAM(24,30,1,1,1)	},	// 07
		{	SET_LINE_PARAM(20,26,1,1,1)	},	// 08
		{	SET_LINE_PARAM(18,24,1,1,1)	},	// 09
		{	SET_LINE_PARAM(14,20,1,1,1)	},	// 10
		{	SET_LINE_PARAM(14,20,1,1,1)	},	// 11
		{	SET_LINE_PARAM( 4, 4,0,0,1)	},	// 12
		{	SET_LINE_PARAM( 4, 4,0,0,1)	},	// 13
		{	SET_LINE_PARAM( 4, 4,0,0,1)	},	// 14
		{	SET_LINE_PARAM( 4, 4,0,0,1)	},	// 15
	},
	{	// 1
		{	SET_LINE_PARAM(20,26,1,1,1)	},	// 00
		{	SET_LINE_PARAM(20,26,1,1,1)	},	// 01
		{	SET_LINE_PARAM(20,26,1,1,1)	},	// 02
		{	SET_LINE_PARAM( 1, 1,0,0,0)	},	// 03
		{	SET_LINE_PARAM(20,26,1,1,1)	},	// 04
		{	SET_LINE_PARAM(20,26,1,1,1)	},	// 05
		{	SET_LINE_PARAM(20,26,1,1,1)	},	// 06
		{	SET_LINE_PARAM(18,24,1,1,1)	},	// 07
		{	SET_LINE_PARAM(14,18,1,1,1)	},	// 08
		{	SET_LINE_PARAM(12,16,1,1,1)	},	// 09
		{	SET_LINE_PARAM(10,14,1,1,1)	},	// 10
		{	SET_LINE_PARAM(10,14,1,1,1)	},	// 11
		{	SET_LINE_PARAM( 4, 4,0,0,1)	},	// 12
		{	SET_LINE_PARAM( 4, 4,0,0,1)	},	// 13
		{	SET_LINE_PARAM( 4, 4,0,0,1)	},	// 14
		{	SET_LINE_PARAM( 4, 4,0,0,1)	},	// 15
	},
	{	// 2
		{	SET_LINE_PARAM(20,26,1,1,1)	},	// 00
		{	SET_LINE_PARAM(20,26,1,1,1)	},	// 01
		{	SET_LINE_PARAM(20,26,1,1,1)	},	// 02
		{	SET_LINE_PARAM( 1, 1,0,0,0)	},	// 03
		{	SET_LINE_PARAM(20,26,1,1,1)	},	// 04
		{	SET_LINE_PARAM(20,26,1,1,1)	},	// 05
		{	SET_LINE_PARAM(20,26,1,1,1)	},	// 06
		{	SET_LINE_PARAM(18,24,1,1,1)	},	// 07
		{	SET_LINE_PARAM(14,18,1,1,1)	},	// 08
		{	SET_LINE_PARAM(12,16,1,1,1)	},	// 09
		{	SET_LINE_PARAM(10,14,1,1,1)	},	// 10
		{	SET_LINE_PARAM(10,14,1,1,1)	},	// 11
		{	SET_LINE_PARAM( 4, 4,0,0,1)	},	// 12
		{	SET_LINE_PARAM( 4, 4,0,0,1)	},	// 13
		{	SET_LINE_PARAM( 4, 4,0,0,1)	},	// 14
		{	SET_LINE_PARAM( 4, 4,0,0,1)	},	// 15
	},
	{	// 3
		{	SET_LINE_PARAM(16,22,1,1,1)	},	// 00
		{	SET_LINE_PARAM(16,22,1,1,1)	},	// 01
		{	SET_LINE_PARAM(16,22,1,1,1)	},	// 02
		{	SET_LINE_PARAM( 1, 1,0,0,0)	},	// 03
		{	SET_LINE_PARAM(16,22,1,1,1)	},	// 04
		{	SET_LINE_PARAM(16,22,1,1,1)	},	// 05
		{	SET_LINE_PARAM(16,22,1,1,1)	},	// 06
		{	SET_LINE_PARAM(14,18,1,1,1)	},	// 07
		{	SET_LINE_PARAM(10,14,1,1,1)	},	// 08
		{	SET_LINE_PARAM( 8,12,1,1,1)	},	// 09
		{	SET_LINE_PARAM( 6,10,1,1,1)	},	// 10
		{	SET_LINE_PARAM( 6,10,1,1,1)	},	// 11
		{	SET_LINE_PARAM( 4, 4,0,0,1)	},	// 12
		{	SET_LINE_PARAM( 4, 4,0,0,1)	},	// 13
		{	SET_LINE_PARAM( 4, 4,0,0,1)	},	// 14
		{	SET_LINE_PARAM( 4, 4,0,0,1)	},	// 15
	},
	{	// 4
		{	SET_LINE_PARAM(12,18,1,1,1)	},	// 00
		{	SET_LINE_PARAM(12,18,1,1,1)	},	// 01
		{	SET_LINE_PARAM(12,18,1,1,1)	},	// 02
		{	SET_LINE_PARAM( 1, 1,0,0,0)	},	// 03
		{	SET_LINE_PARAM(12,18,1,1,1)	},	// 04
		{	SET_LINE_PARAM(12,18,1,1,1)	},	// 05
		{	SET_LINE_PARAM(10,14,1,0,1)	},	// 06
		{	SET_LINE_PARAM(10,14,1,0,1)	},	// 07
		{	SET_LINE_PARAM( 6,10,1,0,1)	},	// 08
		{	SET_LINE_PARAM( 6,10,1,0,1)	},	// 09
		{	SET_LINE_PARAM( 4, 6,1,0,1)	},	// 10
		{	SET_LINE_PARAM( 4, 6,1,0,1)	},	// 11
		{	SET_LINE_PARAM( 2, 2,0,0,1)	},	// 12
		{	SET_LINE_PARAM( 2, 2,0,0,1)	},	// 13
		{	SET_LINE_PARAM( 2, 2,0,0,1)	},	// 14
		{	SET_LINE_PARAM( 2, 2,0,0,1)	},	// 15
	},
	{	// 5
		{	SET_LINE_PARAM(10,14,1,1,1)	},	// 00
		{	SET_LINE_PARAM(10,14,1,1,1)	},	// 01
		{	SET_LINE_PARAM(10,14,1,1,1)	},	// 02
		{	SET_LINE_PARAM( 1, 1,0,0,0)	},	// 03
		{	SET_LINE_PARAM( 8,12,1,1,1)	},	// 04
		{	SET_LINE_PARAM( 8,12,1,1,1)	},	// 05
		{	SET_LINE_PARAM( 6,10,1,0,1)	},	// 06
		{	SET_LINE_PARAM( 6,10,1,0,1)	},	// 07
		{	SET_LINE_PARAM( 3, 3,0,0,1)	},	// 08
		{	SET_LINE_PARAM( 2, 2,0,0,1)	},	// 09
		{	SET_LINE_PARAM( 2, 2,0,0,1)	},	// 10
		{	SET_LINE_PARAM( 2, 2,0,0,1)	},	// 11
		{	SET_LINE_PARAM( 2, 2,0,0,1)	},	// 12
		{	SET_LINE_PARAM( 2, 2,0,0,1)	},	// 13
		{	SET_LINE_PARAM( 2, 2,0,0,1)	},	// 14
		{	SET_LINE_PARAM( 2, 2,0,0,1)	},	// 15
	},
	{	// 6
		{	SET_LINE_PARAM( 8,12,1,1,1)	},	// 00
		{	SET_LINE_PARAM( 8,12,1,1,1)	},	// 01
		{	SET_LINE_PARAM( 8,12,1,1,1)	},	// 02
		{	SET_LINE_PARAM( 1, 1,0,0,0)	},	// 03
		{	SET_LINE_PARAM( 6,10,1,0,1)	},	// 04
		{	SET_LINE_PARAM( 6,10,1,0,1)	},	// 05
		{	SET_LINE_PARAM( 4, 8,1,0,1)	},	// 06
		{	SET_LINE_PARAM( 4, 8,1,0,1)	},	// 07
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 08
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 09
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 10
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 11
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 12
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 13
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 14
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 15
	},
	{	// 7
		{	SET_LINE_PARAM( 8,10,1,1,1)	},	// 00
		{	SET_LINE_PARAM( 8,10,1,1,1)	},	// 01
		{	SET_LINE_PARAM( 8,10,1,1,1)	},	// 02
		{	SET_LINE_PARAM( 1, 1,0,0,0)	},	// 03
		{	SET_LINE_PARAM( 6, 8,1,0,1)	},	// 04
		{	SET_LINE_PARAM( 6, 8,1,0,1)	},	// 05
		{	SET_LINE_PARAM( 4, 6,1,0,1)	},	// 06
		{	SET_LINE_PARAM( 2, 2,0,0,1)	},	// 07
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 08
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 09
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 10
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 11
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 12
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 13
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 14
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 15
	},
	{	// 8
		{	SET_LINE_PARAM( 6, 8,1,0,1)	},	// 00
		{	SET_LINE_PARAM( 6, 8,1,0,1)	},	// 01
		{	SET_LINE_PARAM( 6, 8,1,0,1)	},	// 02
		{	SET_LINE_PARAM( 1, 1,0,0,0)	},	// 03
		{	SET_LINE_PARAM( 4, 6,1,0,1)	},	// 04
		{	SET_LINE_PARAM( 4, 6,1,0,1)	},	// 05
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 06
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 07
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 08
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 09
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 10
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 11
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 12
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 13
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 14
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 15
	},
	{	// 9
		{	SET_LINE_PARAM( 4, 6,1,0,1)	},	// 00
		{	SET_LINE_PARAM( 4, 6,1,0,1)	},	// 01
		{	SET_LINE_PARAM( 4, 6,1,0,1)	},	// 02
		{	SET_LINE_PARAM( 1, 1,0,0,0)	},	// 03
		{	SET_LINE_PARAM( 4, 4,0,0,1)	},	// 04
		{	SET_LINE_PARAM( 3, 3,0,0,1)	},	// 05
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 06
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 07
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 08
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 09
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 10
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 11
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 12
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 13
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 14
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 15
	},
	{	// 10
		{	SET_LINE_PARAM( 4, 4,0,0,1)	},	// 00
		{	SET_LINE_PARAM( 4, 4,0,0,1)	},	// 01
		{	SET_LINE_PARAM( 4, 4,0,0,1)	},	// 02
		{	SET_LINE_PARAM( 1, 1,0,0,0)	},	// 03
		{	SET_LINE_PARAM( 3, 3,0,0,1)	},	// 04
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 05
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 06
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 07
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 08
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 09
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 10
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 11
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 12
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 13
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 14
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 15
	},
	{	// 11
		{	SET_LINE_PARAM( 3, 3,0,0,1)	},	// 00
		{	SET_LINE_PARAM( 3, 3,0,0,1)	},	// 01
		{	SET_LINE_PARAM( 3, 3,0,0,1)	},	// 02
		{	SET_LINE_PARAM( 1, 1,0,0,0)	},	// 03
		{	SET_LINE_PARAM( 2, 2,0,0,1)	},	// 04
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 05
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 06
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 07
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 08
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 09
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 10
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 11
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 12
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 13
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 14
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 15
	},
	{	// 12
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 00
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 01
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 02
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 03
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 04
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 05
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 06
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 07
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 08
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 09
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 10
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 11
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 12
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 13
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 14
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 15
	},
	{	// 13
		{	SET_LINE_PARAM( 2, 2,0,0,1)	},	// 00
		{	SET_LINE_PARAM( 2, 2,0,0,1)	},	// 01
		{	SET_LINE_PARAM( 2, 2,0,0,1)	},	// 02
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 03
		{	SET_LINE_PARAM( 2, 2,0,0,1)	},	// 04
		{	SET_LINE_PARAM( 2, 2,0,0,1)	},	// 05
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 06
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 07
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 08
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 09
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 10
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 11
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 12
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 13
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 14
		{	SET_LINE_PARAM( 1, 1,0,0,1)	},	// 15
	}
};


// *****************************************************************************
// 背景パラメータ
// *****************************************************************************
// 背景カラー[背景種別コード(分類ｺｰﾄﾞ)]
static MP_COLOR_t gBkgdKindCodeTypeColor[MP_BKGD_KIND_CODE_TYPE_MAX] = {
	{	SET_RGBA(225,225,215,255),	SET_RGBA(170,170,170,255)	},	// 00地表面
	{	SET_RGBA( 99,124, 53,255),	SET_RGBA(170,170,170,255)	},	// 01等高線
	{	SET_RGBA(165,165, 16,255),	SET_RGBA(170,170,170,255)	},	// 02大字界面
	{	SET_RGBA(202,223,170,230),	SET_RGBA(170,170,170,255)	},	// 03敷地
	{	SET_RGBA(161,196,253,230),	SET_RGBA(170,170,170,255)	},	// 04水系
	{	SET_RGBA(165,165, 16,255),	SET_RGBA(170,170,170,255)	},	// 05駐車場
	{	SET_RGBA(140,140,140,255),	SET_RGBA(170,170,170,255)	},	// 06道路
	{	SET_RGBA(165,165, 16,255),	SET_RGBA(170,170,170,255)	},	// 07橋桁
	{	SET_RGBA(  0,  0,  0,255),	SET_RGBA(170,170,170,255)	},	// 08道路(線)
	{	SET_RGBA(  0,  0,  0,255),	SET_RGBA(170,170,170,255)	},	// 09路線(線)
	{	SET_RGBA(  0,  0,  0,255),	SET_RGBA(170,170,170,255)	},	// 10境界(線)
	{	SET_RGBA(200,200,200,230),	SET_RGBA(170,170,170,255)	}	// 11建物
};

// 背景カラー[背景種別コード(種別ｺｰﾄﾞ)]
static MP_COLOR_t gBkgdKindCodeKindColor[MP_BKGD_KIND_CODE_KIND_MAX];

// 背景パラメータ[表示縮尺(1つのみ)][背景種別]
static MP_PARAM_t gBkgdParam[1][MP_BKGD_KIND_CODE_TYPE_MAX] = {
	{	// 線・面
		{	SET_POLYGON_PARAM( 1, 0)	},	// 00地表面
		{	SET_POLYGON_PARAM( 1, 0)	},	// 01等高線
		{	SET_POLYGON_PARAM( 1, 0)	},	// 02大字界面
		{	SET_POLYGON_PARAM( 1, 0)	},	// 03敷地
		{	SET_POLYGON_PARAM( 1, 0)	},	// 04水系
		{	SET_POLYGON_PARAM( 1, 0)	},	// 05駐車場
		{	SET_POLYGON_PARAM( 1, 0)	},	// 06道路
		{	SET_POLYGON_PARAM( 1, 0)	},	// 07橋桁
		{	SET_POLYGON_PARAM( 1, 0)	},	// 08道路(線)
		{	SET_POLYGON_PARAM( 1, 0)	},	// 09路線(線)
		{	SET_POLYGON_PARAM( 1, 0)	},	// 10境界(線)
		{	SET_POLYGON_PARAM( 1, 0)	}	// 11建物
	}
};

static MP_PARAM_t gBkgdLineParam[MP_SCALE_LEVEL_CNT][MP_BKGD_KIND_CODE_KIND_MAX];


// *****************************************************************************
// 名称パラメータ
// *****************************************************************************
// フォントパラメータ[分類ｺｰﾄﾞ]
static MP_FONT_t gNameFont[MP_NAME_KIND_CODE_KIND_MAX];


// *****************************************************************************
// 記号背景パラメータ
// *****************************************************************************
// 地図記号[種別ｺｰﾄﾞ]
static UINT32 gMarkMapSymbol[MP_MARK_KIND_CODE_KIND_MAX];


// *****************************************************************************
// 道路名称パラメータ
// *****************************************************************************
// 道路名称カラー[道路種別]
static MP_COLOR_t gRoadNameColor[MP_ROAD_KIND_CODE_MAX] = {
	{	SET_RGBA( 28, 80,147,255),	SET_RGBA(255,255,255,255)	},	// 00
	{	SET_RGBA( 28, 80,147,255),	SET_RGBA(255,255,255,255)	},	// 01
	{	SET_RGBA( 28, 80,147,255),	SET_RGBA(255,255,255,255)	},	// 02
	{	SET_RGBA(  0,  0,  0,255),	SET_RGBA(255,255,255,255)	},	// 03
	{	SET_RGBA(222, 80,101,255),	SET_RGBA(255,255,255,255)	},	// 04
	{	SET_RGBA( 48,123, 96,255),	SET_RGBA(255,255,255,255)	},	// 05
	{	SET_RGBA(210,105, 30,255),	SET_RGBA(255,255,255,255)	},	// 06
	{	SET_RGBA(210,105, 30,255),	SET_RGBA(255,255,255,255)	},	// 07
	{	SET_RGBA(210,105, 30,255),	SET_RGBA(255,255,255,255)	},	// 08
	{	SET_RGBA(210,105, 30,255),	SET_RGBA(255,255,255,255)	},	// 09
	{	SET_RGBA(210,105, 30,255),	SET_RGBA(255,255,255,255)	},	// 10
	{	SET_RGBA(210,105, 30,255),	SET_RGBA(255,255,255,255)	},	// 11
	{	SET_RGBA(210,105, 30,255),	SET_RGBA(255,255,255,255)	},	// 12
	{	SET_RGBA(210,105, 30,255),	SET_RGBA(255,255,255,255)	},	// 13
	{	SET_RGBA(210,105, 30,255),	SET_RGBA(255,255,255,255)	},	// 14
	{	SET_RGBA(210,105, 30,255),	SET_RGBA(255,255,255,255)	},	// 15
};

#if 0
// 道路名称パラメータ[表示縮尺(1つのみ)][背景種別]
static MP_PARAM_t gRoadNameParam[1][MP_ROAD_KIND_CODE_MAX] = {
	{
		{	SET_NAME_PARAM(24, 0)	},	// 00
		{	SET_NAME_PARAM(24, 0)	},	// 01
		{	SET_NAME_PARAM(24, 0)	},	// 02
		{	SET_NAME_PARAM(24, 0)	},	// 03
		{	SET_NAME_PARAM(24, 0)	},	// 04
		{	SET_NAME_PARAM(24, 0)	},	// 05
		{	SET_NAME_PARAM(24, 0)	},	// 06
		{	SET_NAME_PARAM(24, 0)	},	// 07
		{	SET_NAME_PARAM(24, 0)	},	// 08
		{	SET_NAME_PARAM(24, 0)	},	// 09
		{	SET_NAME_PARAM(24, 0)	},	// 10
		{	SET_NAME_PARAM(24, 0)	},	// 11
		{	SET_NAME_PARAM(24, 0)	},	// 12
		{	SET_NAME_PARAM(24, 0)	},	// 13
		{	SET_NAME_PARAM(24, 0)	},	// 14
		{	SET_NAME_PARAM(24, 0)	}	// 15
	}
};
#endif


// *****************************************************************************
// 経路パラメータ
// *****************************************************************************
static UINT32 gRouteParam[MP_SCALE_LEVEL_CNT] = {
	20,	// 00	12m
	18,	// 01 	25m未使用
	18,	// 02 	25m
	16,	// 03 	50m
	14,	// 04 	100m
	12,	// 05 	200m
	10,	// 06 	500m
	10,	// 07 	1km
	10,	// 08 	2.5km
	10,	// 09 	5km
	10,	// 10 	10km
	8,	// 11 	25km
	8,	// 12 	100km
	8	// 13 	250km
};

// *****************************************************************************
// アイコンパラメータ
// *****************************************************************************
// アイコンタイプ
static UINT32 gIconType[MP_ICON_TYPE_MAX];


// *****************************************************************************
// IF
// *****************************************************************************
static void ParamScale_Initialize(void);
static void ParamRoad_Initialize(void);
static void ParamBkgd_Initialize(void);
static void ParamName_Initialize(void);
static void ParamName_Set(UINT32 nameKind, RGBACOLOR color, RGBACOLOR outLineColor, RGBACOLOR bkgdColor, UINT32 size, UINT8 outLineFlg, UINT8 offset);
static void ParamMark_Initialize(void);
static void ParamIcon_Initialize(void);
static void ParamRoute_Initialize(void);

// *****************************************************************************
// 初期化
Bool Param_Initialize(void)
{
	ParamScale_Initialize();
	ParamRoad_Initialize();
	ParamBkgd_Initialize();
	ParamName_Initialize();
	ParamMark_Initialize();
	ParamIcon_Initialize();
	ParamRoute_Initialize();

	return (true);
}

// *****************************************************************************
// 縮尺
static void ParamScale_Initialize(void)
{
}

INT8 ParamScale_ZoomLevel(UINT16 scale)
{
	return (gScaleParam[scale].zoomLevel);
}

INT8 ParamScale_ParcelLevel(UINT16 scale)
{
	return (gScaleParam[scale].parcelLevel);
}

UINT16 ParamScale_ZoomFlg(UINT16 scale)
{
	return (gScaleParam[scale].zoomFlg);
}

UINT16 ParamScale_ZoomToScale(INT8 zoomLevel)
{
	INT32	i = 0;
	UINT16	scale = MP_SCALE_LEVEL_MAX;

	for (i=0; i<MP_SCALE_LEVEL_CNT; i++) {
		if (1 == i) {
			continue;
		}
		if (zoomLevel == gScaleParam[i].zoomLevel) {
			scale = i;
			break;
		}
	}

	return (scale);
}

FLOAT ParamScale_Scale(UINT16 scale)
{
	return (gScaleParam[scale].scale);
}

FLOAT ParamScale_ZoomIn(UINT16 scale)
{
	return (gScaleParam[scale].zoomIn);
}

FLOAT ParamScale_ZoomOut(UINT16 scale)
{
	return (gScaleParam[scale].zoomOut);
}

// *****************************************************************************
// 道路
static void ParamRoad_Initialize(void)
{
}

RGBACOLOR ParamRoad_LineColor(UINT16 roadKind)
{
	return (gRoadColor[roadKind].color);
}

RGBACOLOR ParamRoad_OLineColor(UINT16 roadKind)
{
	return (gRoadColor[roadKind].colorOut);
}

DRAWPARAM ParamRoad_Param(UINT16 scale, UINT16 roadKind)
{
	return (gRoadParam[scale][roadKind].param);
}

// *****************************************************************************
// 背景
static void ParamBkgd_Initialize(void)
{
	INT32 i=0;
	INT32 j=0;

	// 背景カラー****************************************
	// 初期化
	memset(gBkgdKindCodeKindColor, 0, sizeof(gBkgdKindCodeKindColor));

	// 地表面 - 面
	gBkgdKindCodeKindColor[1].color = gBkgdKindCodeTypeColor[MP_BKGD_KIND_CODE_TYPE_SURFACE].color;	// １：陸
	gBkgdKindCodeKindColor[2].color = gBkgdKindCodeTypeColor[MP_BKGD_KIND_CODE_TYPE_WATER].color;	// １：海
	gBkgdKindCodeKindColor[3].color = gBkgdKindCodeTypeColor[MP_BKGD_KIND_CODE_TYPE_SURFACE].color;	// ２：陸
	gBkgdKindCodeKindColor[4].color = gBkgdKindCodeTypeColor[MP_BKGD_KIND_CODE_TYPE_WATER].color;	// ２：海

	// 敷地
	gBkgdKindCodeKindColor[44].color = SET_RGBA(246,174,170,100);	// 軍事基地
	gBkgdKindCodeKindColor[49].color = SET_RGBA(170,200,140,255);	// 運動施設
	gBkgdKindCodeKindColor[50].color = SET_RGBA(210,210,210,255);	// 工業施設
	gBkgdKindCodeKindColor[56].color = SET_RGBA(230,230,220,255);	// 空港
	gBkgdKindCodeKindColor[58].color = SET_RGBA(240,185,185,200);	// 鉄道敷地領域
	gBkgdKindCodeKindColor[60].color = SET_RGBA(240,130,130,200);	// ショッピングセンター

	// 路線
	gBkgdKindCodeKindColor[170].color = SET_RGBA(120,120,120,255);	// ＪＲ線：在来線
	gBkgdKindCodeKindColor[171].color = SET_RGBA(120,120,120,255);	// ＪＲ線：在来線（トンネル）
	gBkgdKindCodeKindColor[172].color = SET_RGBA(120,120,120,255);	// ＪＲ線：新幹線(本線、幹線)
	gBkgdKindCodeKindColor[173].color = SET_RGBA(120,120,120,255);	// ＪＲ線：新幹線(それ以外)
	gBkgdKindCodeKindColor[174].color = SET_RGBA(120,120,120,255);	// ＪＲ線：新幹線(本線、幹線）（トンネル）
	gBkgdKindCodeKindColor[175].color = SET_RGBA(120,120,120,255);	// ＪＲ線：新幹線(それ以外）（トンネル）
	gBkgdKindCodeKindColor[176].color = SET_RGBA(120,120,120,255);	// 私鉄：
	gBkgdKindCodeKindColor[177].color = SET_RGBA(120,120,120,255);	// 私鉄：（トンネル）
	gBkgdKindCodeKindColor[178].color = SET_RGBA(120,120,120,255);	// モノレール，新都市交通
	gBkgdKindCodeKindColor[179].color = SET_RGBA(120,120,120,255);	// 地下鉄（地表部）
	gBkgdKindCodeKindColor[180].color = SET_RGBA(120,120,120,255);	// 路面電車
	gBkgdKindCodeKindColor[181].color = SET_RGBA(120,120,120,255);	// ロープウェイ
	gBkgdKindCodeKindColor[182].color = SET_RGBA(120,120,120,255);	// ケーブルカー
	gBkgdKindCodeKindColor[183].color = SET_RGBA(120,120,120,255);	// フェリー航路（長距離）
	gBkgdKindCodeKindColor[184].color = SET_RGBA(120,120,120,255);	// フェリー航路（中距離）

	// 建物
	gBkgdKindCodeKindColor[202].color = SET_RGBA(240,185,185,230);	// 駅
	gBkgdKindCodeKindColor[203].color = SET_RGBA(240,185,185,230);	// 駅


	// 背景線パラメータ****************************************
	// 初期化
	for (i=0; i<MP_SCALE_LEVEL_CNT; i++) {
		for (j=0; j<MP_BKGD_KIND_CODE_KIND_MAX; j++) {
			gBkgdLineParam[i][j].param = SET_LINE_PARAM(2,0,MP_OFF,MP_OFF,MP_LINE_NMR);
		}
	}

	// 路線
	for (i=0; i<MP_SCALE_LEVEL_CNT; i++) {
		DRAWPARAM railroad_maram = 0;

		switch (i)
		{
		case 0:		railroad_maram = SET_DOT_LINE_PARAM(8,10, MP_ON,MP_OFF,MP_LINE_DOT,50);	break;
		case 1:		railroad_maram = SET_DOT_LINE_PARAM(8,10, MP_ON,MP_OFF,MP_LINE_DOT,60);	break;
		case 2:		railroad_maram = SET_DOT_LINE_PARAM(6, 8, MP_ON,MP_OFF,MP_LINE_DOT,60);	break;
		case 3:		railroad_maram = SET_DOT_LINE_PARAM(4, 6, MP_ON,MP_OFF,MP_LINE_DOT,70);	break;
		case 4:		railroad_maram = SET_DOT_LINE_PARAM(2, 4, MP_ON,MP_OFF,MP_LINE_DOT,10);	break;
		case 5:		railroad_maram = SET_DOT_LINE_PARAM(2, 4, MP_ON,MP_OFF,MP_LINE_DOT,20);	break;
		case 6:		railroad_maram = SET_DOT_LINE_PARAM(2, 0,MP_OFF,MP_OFF,MP_LINE_NMR, 0);	break;
		case 7:		railroad_maram = SET_DOT_LINE_PARAM(2, 0,MP_OFF,MP_OFF,MP_LINE_NMR, 0);	break;
		case 8:		railroad_maram = SET_DOT_LINE_PARAM(2, 0,MP_OFF,MP_OFF,MP_LINE_NMR, 0);	break;
		case 9:		railroad_maram = SET_DOT_LINE_PARAM(2, 0,MP_OFF,MP_OFF,MP_LINE_NMR, 0);	break;
		case 10:	railroad_maram = SET_DOT_LINE_PARAM(2, 0,MP_OFF,MP_OFF,MP_LINE_NMR, 0);	break;
		case 11:	railroad_maram = SET_DOT_LINE_PARAM(2, 0,MP_OFF,MP_OFF,MP_LINE_NMR, 0);	break;
		case 12:	railroad_maram = SET_DOT_LINE_PARAM(2, 0,MP_OFF,MP_OFF,MP_LINE_NMR, 0);	break;
		case 13:	railroad_maram = SET_DOT_LINE_PARAM(2, 0,MP_OFF,MP_OFF,MP_LINE_NMR, 0);	break;
		}

		gBkgdLineParam[i][170].param = railroad_maram;
		gBkgdLineParam[i][171].param = railroad_maram;
		gBkgdLineParam[i][172].param = railroad_maram;
		gBkgdLineParam[i][173].param = railroad_maram;
		gBkgdLineParam[i][174].param = railroad_maram;
		gBkgdLineParam[i][175].param = railroad_maram;
		gBkgdLineParam[i][176].param = railroad_maram;
		gBkgdLineParam[i][177].param = railroad_maram;
	}
}

RGBACOLOR ParamBkgd_BaseColor(UINT16 type)
{
	if (MP_SURFACE_TYPE_LAND == type) {
		// 陸
		return (gBkgdKindCodeTypeColor[MP_BKGD_KIND_CODE_TYPE_SURFACE].color);
	}
	else if (MP_SURFACE_TYPE_SEA == type) {
		// 海
		return (gBkgdKindCodeTypeColor[MP_BKGD_KIND_CODE_TYPE_WATER].color);
	}
	else if (MP_SURFACE_TYPE_NO_DL == type) {
		// 地図未ダウンロード
		return (SET_RGBA(230,230,220,255));
	}

	return (gBkgdKindCodeTypeColor[MP_BKGD_KIND_CODE_TYPE_WATER].color);
}

RGBACOLOR ParamBkgd_Color(UINT16 bkgdType, UINT16 bkgdKind)
{
	if (0 != (UINT32)gBkgdKindCodeKindColor[bkgdKind].color) {
		return (gBkgdKindCodeKindColor[bkgdKind].color);
	}

	return (gBkgdKindCodeTypeColor[bkgdType].color);
}

DRAWPARAM ParamBkgd_Param(UINT16 scale, UINT16 bkgdType)
{
	return (gBkgdParam[0][bkgdType].param);
}

DRAWPARAM ParamBkgdLine_Param(UINT16 scale, UINT16 bkgdType, UINT16 bkgdKind)
{
	return (gBkgdLineParam[scale][bkgdKind].param);
}

Bool ParamBkgd_NonDisplay(UINT16 scale, UINT16 bkgdType, UINT16 bkgdKind, UINT32 figureType)
{
/*	// 水系のライン非表示
	if (MP_BKGD_KIND_CODE_TYPE_WATER == bkgdType && figureType == MP_SHAPE_TYPE_LINE) {
		return (true);
	}*/

	return (false);
}

Bool ParamBkgd_NonDisplayMove(UINT16 scale, UINT16 bkgdType, UINT16 bkgdKind)
{
/*	// スケール5以上の建物
	if (scale >= 5) {
		if (MP_BKGD_KIND_CODE_TYPE_BUILDING == bkgdType) {
			return (true);
		}
	}*/

	return (false);
}

// *****************************************************************************
// 名称
static void ParamName_Initialize(void)
{
	UINT32 cnt;
	UINT32 i;

//	memset(gNameFont, MP_INVALID_VALUE_8, sizeof(gNameFont));

	cnt = sizeof(gNameFont)/sizeof(MP_FONT_t);
	for(i=0; i<cnt; i++) {
		ParamName_Set(i, SET_RGBA(10,10,10,255), SET_RGBA(255,255,255,255), SET_RGBA(0,0,0,0), 26, 1, MP_OFFSET_BOTTOM);
	}

	// 行政界名称
	ParamName_Set(1, SET_RGBA(10,10,10,255), SET_RGBA(255,255,255,255), SET_RGBA(0,0,0,0), 34, 1, MP_OFFSET_CENTER);
	ParamName_Set(2, SET_RGBA(10,10,10,255), SET_RGBA(255,255,255,255), SET_RGBA(0,0,0,0), 32, 1, MP_OFFSET_CENTER);
	ParamName_Set(3, SET_RGBA(10,10,10,255), SET_RGBA(255,255,255,255), SET_RGBA(0,0,0,0), 30, 1, MP_OFFSET_CENTER);

	// 高速関連
	for(i=1161; i<=1167; i++) {
		ParamName_Set(i, SET_RGBA(255,255,255,255), SET_RGBA(255,255,255,255), SET_RGBA(80,150,50,220), 30, 0, MP_OFFSET_CENTER);
	}

	// 交差点名称
	ParamName_Set(203, SET_RGBA(28,80,230,255), SET_RGBA(0,0,255,255), SET_RGBA(255,255,255,220), 30, 0, MP_OFFSET_BOTTOM);
	ParamName_Set(1169, SET_RGBA(28,80,230,255), SET_RGBA(0,0,255,255), SET_RGBA(255,255,255,220), 30, 0, MP_OFFSET_BOTTOM);

	// 駅
	ParamName_Set(1283, SET_RGBA(0,114,168,255), SET_RGBA(255,255,255,255), SET_RGBA(0,0,0,0), 30, 1, MP_OFFSET_BOTTOM);
}

static void ParamName_Set(UINT32 nameKind, RGBACOLOR color, RGBACOLOR outLineColor, RGBACOLOR bkgdColor, UINT32 size, UINT8 outLineFlg, UINT8 offset)
{
	gNameFont[nameKind].color = color;
	gNameFont[nameKind].outLineColor = outLineColor;
	gNameFont[nameKind].bkgdColor = bkgdColor;
	gNameFont[nameKind].size = size;
	gNameFont[nameKind].outLineFlg = outLineFlg;
	gNameFont[nameKind].offset = offset;
}

RGBACOLOR ParamName_Color(UINT16 nameType, UINT16 nameKind)
{
	return (gNameFont[nameKind].color);
}

RGBACOLOR ParamName_OutLineColor(UINT16 nameType, UINT16 nameKind)
{
	return (gNameFont[nameKind].outLineColor);
}

RGBACOLOR ParamName_BkgdColor(UINT16 nameType, UINT16 nameKind)
{
	return (gNameFont[nameKind].bkgdColor);
}

FLOAT ParamName_FontSize(UINT16 scale, UINT16 nameType, UINT16 nameKind)
{
	return ((FLOAT)gNameFont[nameKind].size);
}

INT32 ParamName_OutLine(UINT16 nameType, UINT16 nameKind)
{
	return ((INT32)gNameFont[nameKind].outLineFlg);
}

INT32 ParamName_Offset(UINT16 nameType, UINT16 nameKind)
{
	return ((INT32)gNameFont[nameKind].offset);
}

// *****************************************************************************
// 記号背景
static void ParamMark_Initialize(void)
{
	memset(gMarkMapSymbol, MP_INVALID_VALUE_8, sizeof(gMarkMapSymbol));

	// 【地図記号】
	gMarkMapSymbol[1] = 30;			// 山
	gMarkMapSymbol[10] = 31;		// ゴルフ場 [Golf Course]
	gMarkMapSymbol[23] = 32;		// 警察署 [Police Station]
	gMarkMapSymbol[25] = 33;		// 裁判所 [Court House]
	gMarkMapSymbol[26] = 34;		// 消防署
	gMarkMapSymbol[27] = 35;		// 郵便局
	gMarkMapSymbol[41] = 36;		// 小学校、中学校
	gMarkMapSymbol[43] = 37;		// 大学 [University/College]
	gMarkMapSymbol[50] = 38;		// 病院
	gMarkMapSymbol[60] = 39;		// 神社
	gMarkMapSymbol[61] = 40;		// 寺院
	gMarkMapSymbol[64] = 41;		// 城跡
	gMarkMapSymbol[67] = 42;		// 遺跡
	gMarkMapSymbol[68] = 75;		// バス停
	gMarkMapSymbol[75] = 44;		// ヘリポート
	gMarkMapSymbol[81] = 45;		// ショッピングセンター [Shopping Center]
	gMarkMapSymbol[87] = 46;		// 博物館
	gMarkMapSymbol[91] = 47;		// キャンプ場
	gMarkMapSymbol[92] = 48;		// 動物園
	gMarkMapSymbol[96] = 49;		// 図書館
	gMarkMapSymbol[102] = 50;		// ポスト
	gMarkMapSymbol[103] = 51;		// 公衆電話
	gMarkMapSymbol[104] = 52;		// 喫茶店
	gMarkMapSymbol[105] = 53;		// リサイクリングボックス
	gMarkMapSymbol[106] = 54;		// スーパー
	gMarkMapSymbol[107] = 55;		// 工具店
	gMarkMapSymbol[108] = 56;		// コンビニエンスストア
	gMarkMapSymbol[109] = 57;		// ミニ・ランドアバウト、ロータリー
	gMarkMapSymbol[110] = 58;		// 蛇口や他の飲用水源の位置を表示するためのもの
	gMarkMapSymbol[111] = 59;		// ATM
	gMarkMapSymbol[112] = 60;		// レストラン、飲食店
	gMarkMapSymbol[113] = 61;		// 美容院
	gMarkMapSymbol[114] = 62;		// 公衆トイレ
	gMarkMapSymbol[115] = 63;		// 薬局、ドラッグストア
	gMarkMapSymbol[116] = 64;		// パン屋
	//gMarkMapSymbol[117] = 65;		// 横断歩道
	gMarkMapSymbol[118] = 66;		// 展望台など
	gMarkMapSymbol[119] = 67;		// 居酒屋、パブなど
	gMarkMapSymbol[120] = 68;		// その他の【ホテル】
	gMarkMapSymbol[121] = 69;		// その他の【ＧＳ（ガソリンスタンド）】
	gMarkMapSymbol[122] = 70;		// その他の【カーショップ（カー用品店）】
	gMarkMapSymbol[123] = 71;		// その他の【ファーストフード】
	gMarkMapSymbol[124] = 72;		// その他の【銀行】
	gMarkMapSymbol[130] = 73;		// 屋外駐車場 [Open Parking Area]
	gMarkMapSymbol[141] = 74;		// 信号機
	gMarkMapSymbol[149] = 57;		// 駅前ロータリー

	gMarkMapSymbol[170] = 83;		// 一方通行

	// 【その他】
	gMarkMapSymbol[5508] = 76;		// 駅
}

UINT32 ParamMark_MapSymbol(UINT16 markType, UINT16 markKind)
{
	if(markKind > MP_MARK_KIND_CODE_KIND_MAX) {
		return (MP_INVALID_VALUE_32);
	}

	return (gMarkMapSymbol[markKind]);
}

// *****************************************************************************
// 道路名称
RGBACOLOR ParamRoadName_Color(UINT16 roadKind)
{
	return (gRoadNameColor[roadKind].color);
}

RGBACOLOR ParamRoadName_OutLineColor(UINT16 roadKind)
{
	return (gRoadNameColor[roadKind].colorOut);
}


FLOAT ParamRoadName_FontSize(UINT16 roadKind)
{
	return (26.0f);
}

FLOAT ParamRoadName_RoadNoFontSize(UINT16 roadKind)
{
	return (26.0f);
}

UINT32 ParamRoadName_DivCnt(FLOAT scale)
{
	UINT32 divCnt = 0;

	if (scale<= 0.5f) {
		divCnt = 8;
	}
	else if (scale<= 1.0f) {
		divCnt = 12;
	}
	else if (scale> 1.0f) {
		divCnt = 24;
	}

	return (divCnt);
}

UINT32 ParamRoadName_IconID(UINT16 roadKind)
{
	UINT32 iconID = 0;

	switch (roadKind)
	{
	case MP_ROAD_KIND_CODE_0:
	case MP_ROAD_KIND_CODE_1:
	case MP_ROAD_KIND_CODE_2:
		iconID = 84;
		break;
	case MP_ROAD_KIND_CODE_4:
		iconID = 85;
		break;
	case MP_ROAD_KIND_CODE_5:
		iconID = 86;
		break;
	default:
		iconID = 0;
		break;
	}

	return (iconID);
}

// *****************************************************************************
// 経路
static void ParamRoute_Initialize(void)
{
}

RGBACOLOR ParamRoute_Color(void)
{
	return (SET_RGBA(0,0,255,255));
}

RGBACOLOR ParamRoute_SplitLinkColor(void)
{
	return (SET_RGBA(0,0xcc,0xcc,0xcc)); // cian
}

UINT32 ParamRoute_Size(UINT16 scale)
{
	return (gRouteParam[scale]);
}

UINT32 ParamRoute_SplitLinkSize(UINT16 scale)
{
	return (gRouteParam[scale]);
}

// *****************************************************************************
// 基本リソース(アイコン)
static void ParamIcon_Initialize(void)
{
	gIconType[MP_ICON_TYPE_CAR]		= 20;
	gIconType[MP_ICON_TYPE_START]	= 21;
	gIconType[MP_ICON_TYPE_DEST]	= 22;
	gIconType[MP_ICON_TYPE_TRANSIT]	= 23;
	gIconType[MP_ICON_TYPE_CURSOR]	= 1;
}

UINT32 ParamIcon_IconID(MP_ICON_TYPE_e iconType)
{
	return (gIconType[iconType]);
}

// *****************************************************************************
// 渋滞情報
// 渋滞情報色
RGBACOLOR ParamCongestion_Color(UINT16 cngsLvl)
{
	RGBACOLOR color;
	switch (cngsLvl)
	{
	case MP_CNGS_LVL_0:		// 渋滞なし
		color = SET_RGBA(130,205,80,255);
		break;
	case MP_CNGS_LVL_1:		// 混雑
		color = SET_RGBA(240,125,0,255);
		break;
	case MP_CNGS_LVL_2:		// 渋滞:
		color = SET_RGBA(230,0,0,255);
		break;
//	case MP_CNGS_LVL_3:		// 大渋滞:
//		color = SET_RGBA(170,0,50,255);
//		break;
	default:				// その他
		color = SET_RGBA(0,0,0,0);
		break;
	}

	return (color);
}

// 交通情報サイズ
FLOAT ParamCongestion_LineSize(UINT16 scaleLevel, UINT16 roadKind)
{
	FLOAT w = 6.0f;
	if (MP_ROAD_KIND_CODE_5 >= roadKind) {
		w = 8.0f;
	}
	return (w);
}

// 交通情報フチサイズ
FLOAT ParamCongestion_OutLineSize(UINT16 scaleLevel, UINT16 roadKind)
{
	FLOAT w = 10.0f;
	if (MP_ROAD_KIND_CODE_5 >= roadKind) {
		w = 12.0f;
	}
	return (w);
}
