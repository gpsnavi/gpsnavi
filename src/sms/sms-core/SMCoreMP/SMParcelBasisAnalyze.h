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

//-----------------------------------------------------------------------------
// 型
//-----------------------------------------------------------------------------
typedef char* PCLB_HDL;		// パーセル基本情報ハンドル
typedef char* AREA_HDL;		// エリア情報ハンドル

//-----------------------------------------------------------------------------
// マクロ
//-----------------------------------------------------------------------------
#define GET_PCLB_SIZE(_p)				GetUINT16(_p, 0)		// データサイズ
#define GET_PCLB_SEA_FLG(_p)			GetUINT8(_p, 2)			// 背景海フラグ
#define GET_PCLB_AREAREC_CNT(_p)		GetUINT8(_p, 3)			// エリア情報レコード数
#define GET_PCLB_REAL_LENGTH_T(_p)		GetUINT32(_p, 4)		// 実長データ(X方向上辺)
#define GET_PCLB_REAL_LENGTH_B(_p)		GetUINT32(_p, 8)		// 実長データ(X方向下辺)
#define GET_PCLB_REAL_LENGTH_L(_p)		GetUINT32(_p, 12)		// 実長データ(Y方向左辺)
#define GET_PCLB_REAL_LENGTH_R(_p)		GetUINT32(_p, 16)		// 実長データ(Y方向右辺)
#define GET_PCLB_COUNTRY_CODE_CNT(_p)	GetUINT16(_p, 20)		// 国識別コード数
#define GET_PCLB_COUNTRY_CODE(_p,_n)	GetUINT16(_p, 20+(_n*2))// 国識別コード

#define GET_PCLB_AREA_HDL(_p)			(_p + 20 + MP_ALIGNMENT4(2 + (GET_PCLB_COUNTRY_CODE_CNT(_p)*2)))
																// エリア情報ハンドル

#define GET_AREA_AREA_NO(_p,_n)			GetUINT8(_p, (_n))		// エリア番号
