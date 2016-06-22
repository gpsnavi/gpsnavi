/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef RT_LIBDEF_H_
#define RT_LIBDEF_H_

#define ALLF8					0xFF
#define	ALLF16					0xFFFF
#define ALLF32					0xFFFFFFFF

extern DOUBLE 					RT_real_x;				// 実長変換係数(x)
extern DOUBLE 					RT_real_y;				// 実長変換係数(y)

// ラジアンを度に変換する．
#define RT_M_RADTODEG(radian)    ((180 / M_PI) * (radian))

// 度をラジアンに変換する．
#define RT_M_DEGTORAD(degree)    ((M_PI / 180) * (degree))

// 経路ＩＤ情報
typedef struct {
	UINT32						id;						// 経路ＩＤ
	UINT32						mode;					// 探索モード種別
	E_SC_ROUTETYPE				type;					// 探索種別
} RT_ROUTEID_t;

// 名称テーブル
typedef struct {
	UINT16						len;
	UINT8						name[SC_BROADSTRING_LEN];	// 交差点名称

	RT_VOICE_t					voice;
} RT_NAME_t;

// ＴＴＳ発話テーブル
typedef struct {
	UINT16						len;
	UINT8						tts[SC_VOICE_TTS_LEN];		// ＴＴＳ
} RT_VOICE_TTS_t;

// リンク情報
typedef struct {
	UINT32						parcel_id;			// パーセルID
	UINT32						link_id;			// リンクID
	UINT16						link_dir;			// リンク方向
} RT_LINK_t;

// ＸＹ座標情報
typedef struct {
	UINT16						x;					// Ｘ座標
	UINT16						y;					// Ｙ座標
} RT_XY_t;

// リンク形状点情報
typedef struct {
	UINT16						vol;				// 形状点数
	RT_XY_t						pos[500];			// 形状点座標
} RT_LINKPOINT_t;

// 矩形エリア情報
typedef struct {
	struct {
		UINT32					id;					// パーセルＩＤ
		UINT16					base_x;				// 左下基準Ｘ座標
		UINT16					base_y;				// 左下基準Ｙ座標
	} parcel[4];									// 包含パーセル情報
	UINT16						parcel_vol;			// 包含パーセル情報数
	RT_XY_t						lb_pos;				// 矩形左下座標
	RT_XY_t						rt_pos;				// 矩形右上座標
	RT_XY_t						ct_pos;				// 矩形中心座標
} RT_CLIPAREA_t;

// 位置情報
typedef struct {
	UINT32						parcel_id;			// パーセルＩＤ
	DOUBLE						x;					// Ｘ座標
	DOUBLE						y;					// Ｙ座標
} RT_POSITION_t;

// ２次元ベクトル
typedef struct {
	DOUBLE x;
	DOUBLE y;
} RT_VECTOR2_t;

INT32 RT_LIB_GetCrossAngle(DOUBLE i_x, DOUBLE i_y, DOUBLE c_x, DOUBLE c_y, DOUBLE o_x, DOUBLE o_y);
INT32 RT_LIB_GetCrossAngle2(DOUBLE ii_x, DOUBLE ii_y, DOUBLE io_x, DOUBLE io_y, DOUBLE oi_x, DOUBLE oi_y, DOUBLE oo_x, DOUBLE oo_y);
INT32 RT_LIB_VerticalPoint(DOUBLE cx, DOUBLE cy, DOUBLE sx,	DOUBLE sy, DOUBLE inEx,	DOUBLE inEy, INT8* ret,	DOUBLE* dx,	DOUBLE* dy,	DOUBLE* l_len,
	DOUBLE* v_len, DOUBLE* ratio, DOUBLE* cxx, DOUBLE* cyy, DOUBLE* angle, DOUBLE* len_v_dir, DOUBLE* len_l_dir );

E_SC_RESULT RG_CTL_JointVoiceList(RT_VOICE_t*, RT_VOICE_t*);
E_SC_RESULT RG_CTL_InsertVoiceList(RT_VOICE_t* dst, UINT16 val, UINT8 index);

#endif /* RT_LIBDEF_H_ */
