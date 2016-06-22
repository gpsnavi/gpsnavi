/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

/**
 * @file	SMCoreMeshCalc.c
 * @brief	メッシュ計算ライブラリ
 *
 * @author	n.kanagawa
 * @date
 */

#include "SMCoreCMNInternal.h"

//-----------------------------------------------------------------------------
// 定数
//-----------------------------------------------------------------------------
// ■基準パーセル(レベル6)を固定にしてる
#define BASE_LAT				18
#define BASE_LON				4
#define LV6_PARCEL_ID			0x90F0207

#define	FMESH_WIDTH				28800	// １次メッシュ経度幅(1/8秒)
#define	FMESH_HEIGHT			19200	// １次メッシュ緯度幅(1/8秒)
#define	SMESH_WIDTH				3600	// ２次メッシュ経度幅(1/8秒)
#define	SMESH_HEIGHT			2400	// ２次メッシュ緯度幅(1/8秒)
#define	MESH25_WIDTH			900		// ２．５次メッシュ経度幅(1/8秒)
#define	MESH25_HEIGHT			600		// ２．５次メッシュ経度幅(1/8秒)
#define	MESH35_WIDTH			225		// ３．５次メッシュ経度幅(1/8秒)
#define	MESH35_HEIGHT			150		// ３．５次メッシュ経度幅(1/8秒)


//-----------------------------------------------------------------------------
// 構造体
//-----------------------------------------------------------------------------
#if 0
typedef struct _DIVIDE_PARCEL
{
	INT32 d;	//   1次メッシュ経度
	INT32 a;	//   1次メッシュ緯度
	INT32 b;	//   2次メッシュ緯度
	INT32 c;	// 2.5次メッシュ緯度
//	INT32 d1;	//   1次メッシュ経度1
//	INT32 d2;	//   1次メッシュ経度2
	INT32 e;	//   2次メッシュ経度
	INT32 f;	// 2.5次メッシュ経度

	//UINT32 Combine(void);
} T_DIVIDE_PARCEL;
#endif

//-----------------------------------------------------------------------------
// マクロ
//-----------------------------------------------------------------------------
// ■パーセルID算出用
#define WORD0(id_)		(((id_) >> 16) & 0xffff)	// 0WORD獲得
#define WORD1(id_)		((id_) & 0xffff)			// 1WORD獲得
#define _GET_A(word0_)	(((word0_) >> 7) & 0xff)	// 領域A獲得
#define _GET_B(word0_)	(((word0_) >> 4) & 0x07)	// 領域B獲得
#define _GET_C(word0_)	((word0_) & 0xf)			// 領域C獲得
#define _GET_D1(word0_)	((word0_) >> 15)			// 領域D1獲得
#define _GET_D2(word1_)	((word1_) >> 7 & 0x1FF)		// 領域D2獲得
#define _GET_E(word1_)	(((word1_) >> 4) & 0x07)	// 領域E獲得
#define _GET_F(word1_)	((word1_) & 0xf)			// 領域F獲得

// ■分割数に合わせて値の計算
#define	convValidCode( value, next, limit )		\
	next = 0;									\
	if( 0 < (value) ){							\
		while( (limit) <= (value) ){			\
			(value) -= (limit);					\
			(next)  += 1;						\
		}										\
	}											\
	else{										\
		while( 0 > (value) ){					\
			(value) += (limit);					\
			(next)  -= 1;						\
		}										\
	}


//-----------------------------------------------------------------------------
// 内部関数
//-----------------------------------------------------------------------------
static void sc_MESH_ParcelDivide(const UINT32 ParcelId, T_DIVIDE_PARCEL* p_DivPcl);
static UINT32 sc_MESH_ParcelCombine(T_DIVIDE_PARCEL* p_DivPcl);


//-----------------------------------------------------------------------------
// 関数
//-----------------------------------------------------------------------------

/**
 * @brief		パーセルIDからのレベル算出処理
 *
 * @param		[in] ParcelId パーセルID
 * @return		レベル(1～6)
 */
INT32 SC_MESH_GetLevel(UINT32 ParcelId)
{
	INT32 level = 0;
	UINT32 c = _GET_C(WORD0(ParcelId));
	UINT32 f = _GET_F(WORD1(ParcelId));

	if( (c==0xf) && (f==0x6) ) {
		level = 6;
	} else if( (c==0xf) && (f==0x5) ) {
		level = 5;
	} else if( (c==0xf) && (f==0x4) ) {
		level = 4;
	} else if( (c==0xf) && (f==0x3) ) {
		level = 3;
	} else if( (c==0xf) && (f==0x2) ) {
		level = 2;
	} else {
		level = 1;
	}

	return (level);
}

/**
 * @brief      上位パーセルID算出処理
 *
 * @param [in] ParcelId 自レベルパーセルID
 * @return     1レベル下のパーセルID
 */
UINT32 SC_MESH_GetUpperParcelID(const UINT32 ParcelId)
{
	INT32 Level = 0;
	UINT32 out_ParcelId = 0xffffffff;
	T_DIVIDE_PARCEL DivPar;

	Level = SC_MESH_GetLevel(ParcelId);

	// パーセル分割
	sc_MESH_ParcelDivide(ParcelId, &DivPar);

	// 一つ上のレベルのパーセル設定
	switch(Level)
	{
	case 1:
		DivPar.c	= 0x0F;							// 2.5次メッシュ緯度を予約にする
		DivPar.f	= 0x02;							// 2.5次メッシュ経度をLv2の予約にする
		break;
	case 2:
		DivPar.b	/= 4;							//   2次メッシュ緯度を1次メッシュ緯度の2分割にする(0or1のみ)
		DivPar.e	/= 4;							//   2次メッシュ経度を1次メッシュ経度の2分割にする(0or1のみ)
		DivPar.c	= 0x0F;							// 2.5次メッシュ緯度を予約にする
		DivPar.f	= 0x03;							// 2.5次メッシュ経度をLv3の予約にする
		break;
	case 3:
		DivPar.a	= (DivPar.a / 2) * 2;			//   1次メッシュ経度  1次2枚の西側
		DivPar.d	= (DivPar.d / 2) * 2;			//   1次メッシュ緯度2 1次2枚の南側
		DivPar.b	= 0x00;							//   2次メッシュ緯度を未使用にする
		DivPar.e	= 0x00;							//   2次メッシュ経度を未使用にする
		DivPar.c	= 0x0F;							// 2.5次メッシュ緯度を予約にする
		DivPar.f	= 0x04;							// 2.5次メッシュ経度をLv4の予約にする
		break;
	case 4:
		DivPar.a	= (((DivPar.a - BASE_LAT) / 8) * 8)+BASE_LAT;//   1次メッシュ経度  1次8枚の西側
		DivPar.d	= (((DivPar.d - BASE_LON) / 8) * 8)+BASE_LON;//   1次メッシュ緯度2 1次8枚の南側
		DivPar.b	= 0x00;							//   2次メッシュ緯度を未使用にする
		DivPar.e	= 0x00;							//   2次メッシュ経度を未使用にする
		DivPar.c	= 0x0F;							// 2.5次メッシュ緯度を予約にする
		DivPar.f	= 0x05;							// 2.5次メッシュ経度をLv5の予約にする
		break;
	case 5:
		DivPar.a	= (((DivPar.a - BASE_LAT) / 32) * 32)+BASE_LAT;	//   1次メッシュ経度  1次32枚の西側
		DivPar.d	= (((DivPar.d - BASE_LON) / 32) * 32)+BASE_LON;	//   1次メッシュ緯度2 1次32枚の南側
		DivPar.b	= 0x00;							//   2次メッシュ緯度を未使用にする
		DivPar.e	= 0x00;							//   2次メッシュ経度を未使用にする
		DivPar.c	= 0x0F;							// 2.5次メッシュ緯度を予約にする
		DivPar.f	= 0x06;							// 2.5次メッシュ経度をLv5の予約にする
		break;
	default:
		return (out_ParcelId);
	}

	// パーセル組み立て
	out_ParcelId = sc_MESH_ParcelCombine(&DivPar);

	return (out_ParcelId);
}

/**
 * @brief		下位パーセルID算出処理
 *
 * @param		[in] ParcelId 自レベルパーセルID
 * @param		[out] p_UnderParcelList 下位レベルパーセルIDリスト
 * @return		INT32	16
 */
INT32 SC_MESH_GetUnderParcelID(const UINT32 ParcelId, T_UNDER_PARCEL_LIST* p_UnderParcelList)
{
	INT32 x = 0;
	INT32 y = 0;
	INT32 pos = 1;
	INT32 loop = 16;
	INT32 i = 0;
	INT32 level;
	T_DIVIDE_PARCEL mesh;

	memset(p_UnderParcelList, 0, sizeof(T_UNDER_PARCEL_LIST));

	// 自レベル算出
	level = SC_MESH_GetLevel(ParcelId);
	if(1 >= level) {
		return (0);
	}

	// 下位パーセルID算出
	for(i=0; i<loop; i++) {

		sc_MESH_ParcelDivide(ParcelId, &mesh);

		switch(level)
		{
		case 2:		// レベル2の場合
			// 縦横位置を更新
			if( (4 == i) || (8 == i) || (12 == i) ){
				x = 0;	// 横位置を0に戻す
				y++;	// 縦位置を+1する
			}

			// 2.5次緯度メッシュコード算出
			mesh.c = 0xC | y;

			// 2.5次経度メッシュコード算出
			mesh.f = 0xC | x;
			x++;	// 横位置を+1する

			break;

		case 3:		// レベル3の場合
			// 縦横位置を更新
			if( (4 == i) || (8 == i) || (12 == i) ){
				x = 0;	// 横位置を0に戻す
				y++;	// 縦位置を+1する
			}

			// 2次緯度メッシュコード算出
			if( 1 == mesh.b ){
				mesh.b += y+3;
			}
			else{
				mesh.b += y;
			}

			// 2次経度メッシュコード算出
			if( 1 == mesh.e ){
				mesh.e += x+3;
			}
			else{
				mesh.e += x;
			}
			x++;	// 横位置を+1する

			// 予約コード格納
			mesh.c = 0xf;
			mesh.f = 2;

			break;

		case 4:		// レベル4の場合
			// 1次緯度メッシュコード算出
			if( 8 <= i ){
				mesh.a++;
			}

			// 1次経度メッシュコード算出
			if( ((4 <= i) && (7 >= i)) || (12 <= i) ){
				mesh.d++;
			}

			// 縦横位置を更新
			if( 0 == (i%4) ){
				pos = 1;
			}
			// 1次メッシュ1/4緯度メッシュコード算出
			if( 2 < pos ){
				mesh.b = 1;
			}
			else{
				mesh.b = 0;
			}
			// 1次メッシュ1/4経度メッシュコード算出
			if( 0 == (pos%2) ){
				mesh.e = 1;
			}
			else{
				mesh.e = 0;
			}
			pos++;

			// 予約コード格納
			mesh.c = 0xf;
			mesh.f = 3;
			break;

		case 5:		// レベル5の場合
			// 縦横位置を更新
			if( (4 == i) || (8 == i) || (12 == i) ){
				x = 0;	// 横位置を0に戻す
				y++;	// 縦位置を+1する
			}

			// 1次緯度メッシュコード算出
			mesh.a += y*4;
			// 1次経度メッシュコード算出
			mesh.d += x*4;

			x++;	// 横位置を+1する

			// 予約コード格納
			mesh.c = 0xf;
			mesh.f = 4;
			break;

		default:
			break;
		}

		p_UnderParcelList->ParcelId[i] = sc_MESH_ParcelCombine(&mesh);
	}

	return (i);
}

/**
 * @brief		隣接したメッシュIDを求める
 *
 * @param		[in] ParcelId パーセルID
 * @param		[in] dir 隣接方向
 * @return		隣接パーセルID
 */
UINT32 SC_MESH_GetNextParcelID(UINT32 ParcelId, UINT8 dir)
{
	UINT32 out_ParcelId = 0xFFFFFFFF;

	switch( dir )
	{
	case DIR_TOP:		out_ParcelId = SC_MESH_SftParcelId(ParcelId,  0,  1);	break;
	case DIR_R_TOP:		out_ParcelId = SC_MESH_SftParcelId(ParcelId,  1,  1);	break;
	case DIR_R:			out_ParcelId = SC_MESH_SftParcelId(ParcelId,  1,  0);	break;
	case DIR_R_DOWN:	out_ParcelId = SC_MESH_SftParcelId(ParcelId,  1, -1);	break;
	case DIR_DOWN:		out_ParcelId = SC_MESH_SftParcelId(ParcelId,  0, -1);	break;
	case DIR_L_DOWN:	out_ParcelId = SC_MESH_SftParcelId(ParcelId, -1, -1);	break;
	case DIR_L:			out_ParcelId = SC_MESH_SftParcelId(ParcelId, -1,  0);	break;
	case DIR_L_TOP:		out_ParcelId = SC_MESH_SftParcelId(ParcelId, -1,  1);	break;
	default:															break;
	}
	return (out_ParcelId);
}

/**
 * @brief		任意量シフトしたパーセルIDを求める
 *
 * @param		[in] ParcelId 基準パーセルID
 * @param		[in] x_vol x方向シフト量
 * @param		[in] y_vol y方向シフト量
 * @return		パーセルID
 */
UINT32 SC_MESH_SftParcelId(UINT32 ParcelId, INT16 x_vol, INT16 y_vol)
{

	UINT32	out_ParcelId = 0xFFFFFFFF;
	INT32	level;
	INT32	lat_next;	// 緯度方向の相対コード
	INT32	lon_next;	// 経度方向の相対コード
	T_DIVIDE_PARCEL mesh;

	// レベル取得
	level = SC_MESH_GetLevel(ParcelId);

	// メッシュコードを分解して取得
	sc_MESH_ParcelDivide(ParcelId, &mesh);

	// シフト処理
	switch(level)
	{
	case 6:
		mesh.a = mesh.a + (y_vol * 32);	// 1次32枚
		mesh.d = mesh.d + (x_vol * 32);	// 1次32枚
		break;

	case 5:
		mesh.a = mesh.a + (y_vol * 8);	// 1次8枚
		mesh.d = mesh.d + (x_vol * 8);	// 1次8枚
		break;

	case 4:
		mesh.a = mesh.a + (y_vol * 2);	// 1次2枚
		mesh.d = mesh.d + (x_vol * 2);	// 1次2枚
		break;

	case 3:
		// 緯度方向算出
		mesh.b += y_vol;
		convValidCode(mesh.b, lat_next, 2);
		mesh.a += lat_next;

		// 経度方向算出
		mesh.e += x_vol;
		convValidCode(mesh.e, lon_next, 2);
		mesh.d += lon_next;
		break;

	case 2:
		// 緯度方向算出
		mesh.b += y_vol;
		convValidCode(mesh.b, lat_next, 8);
		mesh.a += lat_next;

		// 経度方向算出
		mesh.e += x_vol;
		convValidCode(mesh.e, lon_next, 8);
		mesh.d += lon_next;
		break;

	case 1:
		// 緯度方向算出
		mesh.c &= 0x03; // 分割ビットOFF
		mesh.c += y_vol;
		convValidCode(mesh.c, lat_next, 4);
		mesh.b += lat_next;
		convValidCode(mesh.b, lat_next, 8);
		mesh.a += lat_next;
		mesh.c |= 0x0c; // 分割ビットON

		// 経度方向算出
		mesh.f &= 0x03; // 分割ビットOFF
		mesh.f += x_vol;
		convValidCode(mesh.f, lon_next, 4);
		mesh.e += lon_next;
		convValidCode(mesh.e, lon_next, 8);
		mesh.d += lon_next;
		mesh.f |= 0x0c; // 分割ビットON
		break;

	default:
		break;
	}

	out_ParcelId = sc_MESH_ParcelCombine(&mesh);

	return (out_ParcelId);
}

/**
 * @brief		緯度経度(1/8)からパーセルID算出
 *
 * @param		[in] lvl レベル
 * @param		[in] pid_x 経度1/8秒
 * @param		[in] pid_y 緯度1/8秒
 * @return		パーセルID
 */
UINT32 SC_MESH_GetParcelID(INT32 lvl, INT32 pid_x, INT32 pid_y)
{
	INT32			parcel_id;		// パーセルID

	// 緯度(単位：2/3度)の設定
	parcel_id = (pid_y / FMESH_HEIGHT) << 23;

	// 1次メッシュ内での相対緯度の設定
	if( lvl == 3 ) {
		// レベル3は1次メッシュを1/2×1/2に分割した際の相対位置を設定
		// (レベル3は2次メッシュ4×4枚分のため(2次メッシュ緯度幅×4)で割る)
		parcel_id |= ((pid_y % FMESH_HEIGHT) / (SMESH_HEIGHT * 4)) << 20;
	}
	else if( lvl < 3 ) {
		parcel_id |= (pid_y % FMESH_HEIGHT) / SMESH_HEIGHT << 20;
	}

	// 2次メッシュ内での相対緯度の設定(レベル1以上は下で経度と一緒に設定)
	if( lvl == 1 ) {
		parcel_id |= (0x000c0000 | ((pid_y % SMESH_HEIGHT) / MESH25_HEIGHT << 16));
	}

//	if( lvl == -1 ) {
//		parcel_id |= (pid_y % SMESH_HEIGHT) / MESH35_HEIGHT << 16;
//	}

	// 経度(単位：度)の設定
	parcel_id |= (pid_x / FMESH_WIDTH - 100) << 7;

	// 1次メッシュ内での相対経度の設定
	if( lvl == 3 ) {
		// レベル3は1次メッシュを1/2×1/2に分割した際の相対位置を設定
		// (レベル3は2次メッシュ4×4枚分のため(2次メッシュ経度幅×4)で割る)
		parcel_id |= ((pid_x % FMESH_WIDTH) / (SMESH_WIDTH * 4 )) << 4;
	}
	else if( lvl < 3 ) {
		parcel_id |= (pid_x % FMESH_WIDTH) / SMESH_WIDTH << 4;
	}

	// 2次メッシュ内での相対経度の設定
	if( lvl == 1 ) {
		parcel_id |= (0x0000000c | ((pid_x % SMESH_WIDTH) / MESH25_WIDTH));
	}

//	if( lvl == -1 ) {
//		parcel_id |= (pid_x % SMESH_WIDTH) / MESH35_WIDTH;
//	}

	if( lvl > 1 ) {
		parcel_id |= (0x000f0000 | (lvl+1));
	}

	return( parcel_id );
}





/**
 * @brief		ＰＩＤ座標系を緯度・経度に変換する
 *
 * @param		[in] lvl 変換するﾚﾍﾞﾙ
 * @param		[in] parcel_id 変換するﾊﾟｰｾﾙID
 * @param		[in] x 変換する正規化X座標
 * @param		[in] y 変換する正規化Y座標
 * @param		[out] latitude 変換後の緯度（秒）
 * @param		[out] longitude 変換後の経度（秒）
 * @return		パーセルID
 */
INT32 SC_MESH_ChgParcelIDToTitude(
							INT32		level,      // 変換するﾚﾍﾞﾙ
							UINT32		parcel_id,  // 変換するﾊﾟｰｾﾙID
							DOUBLE		x,          // 変換する正規化Ｘ座標
							DOUBLE		y,          // 変換する正規化Ｙ座標
							DOUBLE		*latitude,  // 変換後の緯度（秒）
							DOUBLE		*longitude  // 変換後の経度（秒）
		)
{

	//********************************************************************************************
	//		１次メッシュ：	経度・・・３６００秒	緯度・・・２４００秒
	//		２次メッシュ：	経度・・・　４５０秒	緯度・・・　３００秒
	//							メッシュ当り			　経度（秒）    緯度（秒）
	//		レベル－１：	１次メッシュ　＊　1/(8*4*4)			28.125       18.75
	//		レベル　０：	１次メッシュ　＊　1/(8*4)		　 112.5         75
	//		レベル　１：	１次メッシュ　＊　1/(8)			　 450          300
	//		レベル　２：	１次メッシュ　＊　1/(2)		      1800         1200
	//		レベル　３：	１次メッシュ　＊　2			　    7200         4800
	//		レベル　４：	１次メッシュ　＊　(2*4)		     28800        19200
	//		レベル　５：	１次メッシュ　＊　(2*4*4)	    115200        76800
	//		レベル　６：	１次パーセル　＊　(2*4*16)	    115200 * 4    76800 * 4
	//********************************************************************************************
	INT32  w_h;
	INT32  w_l;

	switch(level)
	{
/*	case	MAP_LEVELM1 :
		*latitude  =	(
							(double)(((int)(parcel_id >> 23)) & 0xff)			+
							(double)((parcel_id >> 20) & 0x00000007)/8			+
							(double)((parcel_id >> 16) & 0x0000000f)/128		+
							(double)y/(128 * MAP_SIZE)
						) * 2400;
		*longitude  =	(
							(double)((int)((parcel_id  & 0x80000000) | ((parcel_id << 15) & 0x7fc00000)) >> 22)	+
							100															+
							(double)((parcel_id >>  4) & 0x00000007)/8					+
							(double)((parcel_id      ) & 0x0000000f)/128				+
							(double)x/(128 * MAP_SIZE)
						) * 3600;
		break;*/
	case 1:
		*latitude  =	(
							(double)(((int)(parcel_id >> 23)) & 0xff)			+
							(double)((parcel_id >> 20) & 0x00000007)/8			+
							(double)((parcel_id >> 16) & 0x00000003)/32			+
							(double)y/(32 * MAP_SIZE)
						) * 2400;
		*longitude  =	(
							(double)((int)((parcel_id  & 0x80000000) | ((parcel_id << 15) & 0x7fc00000)) >> 22)	+
							100															+
							(double)((parcel_id >>  4) & 0x00000007)/8					+
							(double)((parcel_id      ) & 0x00000003)/32					+
							(double)x/(32 * MAP_SIZE)
						) * 3600;
		break;
	case 2:
		*latitude  =	(
							(double)(((int)(parcel_id >> 23)) & 0xff)			+
							(double)((parcel_id >> 20) & 0x00000007)/8			+
							(double)y/(8 * MAP_SIZE)
						) * 2400;
		*longitude  =	(
							(double)((int)((parcel_id  & 0x80000000) | ((parcel_id << 15) & 0x7fc00000)) >> 22)	+
							100															+
							(double)((parcel_id >>  4) & 0x00000007)/8					+
							(double)x/(8 * MAP_SIZE)
						) * 3600;
		break;
	case 3:
		*latitude  =	(
							(double)(((int)(parcel_id >> 23)) & 0xff)			+
							(double)((parcel_id >> 20) & 0x00000001)/2			+
							(double)y/(2 * MAP_SIZE)
						) * 2400;
		*longitude  =	(
							(double)((int)((parcel_id  & 0x80000000) | ((parcel_id << 15) & 0x7fc00000)) >> 22)	+
							100															+
							(double)((parcel_id >>  4) & 0x00000001)/2					+
							(double)x/(2 * MAP_SIZE)
						) * 3600;
		break;
	case 4:
		*latitude  =	(
							(double)(((int)(parcel_id >> 23)) & 0xff)			+
							(double)y*2/MAP_SIZE
						) * 2400;
		*longitude  =	(
							(double)((int)((parcel_id  & 0x80000000) | ((parcel_id << 15) & 0x7fc00000)) >> 22)	+
							100																			+
							(double)x*2/MAP_SIZE
						) * 3600;
		break;
	case 5:
		*latitude  =	(
							(double)(((int)(parcel_id >> 23)) & 0xff)			+
							(double)y*8/MAP_SIZE
						) * 2400;
		*longitude  =	(
							(double)((int)((parcel_id  & 0x80000000) | ((parcel_id << 15) & 0x7fc00000)) >> 22)	+
							100																			+
							(double)x*8/MAP_SIZE
						) * 3600;
		break;
	case 6 :
		w_h = (parcel_id >> 23) & 0xff;
		w_l = (((parcel_id  & 0x80000000) | ((parcel_id << 15) & 0x7fc00000)) >> 22)	+	100;
		*latitude  =	(
							(double)(((int)(parcel_id >> 23)) & 0xff)			+
							(double)y*32/MAP_SIZE
						) * 2400;
		*longitude  =	(
							(double)((int)((parcel_id  & 0x80000000) | ((parcel_id << 15) & 0x7fc00000)) >> 22)	+
							100																			+
							(double)x*32/MAP_SIZE
						) * 3600;
		break;
	default:
				return(-1);
	}



	return(0);
}

/**
 * @brief		実長取得
 */
void SC_MESH_GetRealSize(
		DOUBLE	latitude,	//	緯度（秒）
		DOUBLE	longitude,  //	経度（秒）
		DOUBLE*	len			//	ｍ／秒
		)
{

	DOUBLE	f1;		// ２地点の緯度(°)
	DOUBLE	g1;		// ２地点の経度（東経基準）(°)
	DOUBLE	s;		// ２地点間の地表面距離(m)

	f1 = latitude/3600.0;
	g1 = longitude/3600.0;

	// 経度
	s = sc_MESH_GetRealLen(f1, g1, f1, g1 + 1.0/8.0);
	*len	= s / 3600. * 8.0;

	// 緯度
	s = sc_MESH_GetRealLen(f1, g1, f1 + 1.0/12.0, g1);
	*(len+1)=s / 3600. * 12.0;
}







/**
 * @brief      パーセルID分解
 *
 * @param [in] ParcelId パーセルID
 * @return     void
 */
static void sc_MESH_ParcelDivide(const UINT32 ParcelId, T_DIVIDE_PARCEL* p_DivPcl)
{
	INT32 d1, d2;
	p_DivPcl->a  = _GET_A(WORD0(ParcelId));		//   1次メッシュ緯度
	p_DivPcl->b  = _GET_B(WORD0(ParcelId));		//   2次メッシュ緯度
	p_DivPcl->c  = _GET_C(WORD0(ParcelId));		// 2.5次メッシュ緯度
	d1			 = _GET_D1(WORD0(ParcelId));		//   1次メッシュ経度1
	d2 			 = _GET_D2(WORD1(ParcelId));		//   1次メッシュ経度2
	p_DivPcl->e  = _GET_E(WORD1(ParcelId));		//   2次メッシュ経度
	p_DivPcl->f  = _GET_F(WORD1(ParcelId));		// 2.5次メッシュ経度

	p_DivPcl->d = (d1 << 9) | d2;					//   1次メッシュ経度

}

/**
 * @brief      パーセルID結合
 *
 * @return     パーセルID
 */
static UINT32 sc_MESH_ParcelCombine(T_DIVIDE_PARCEL* p_DivPcl)
{
	UINT32 ParcelID = 0;
	INT32 d1;
	INT32 d2;

	d1 = (p_DivPcl->d & 0x200) >> 9;
	d2 = p_DivPcl->d & 0x1FF;

	ParcelID += (d1				<< 31);		//   1次メッシュ経度1
	ParcelID += (p_DivPcl->a	<< 23);		//   1次メッシュ緯度
	ParcelID += (p_DivPcl->b	<< 20);		//   2次メッシュ緯度
	ParcelID += (p_DivPcl->c	<< 16);		// 2.5次メッシュ緯度
	ParcelID += (d2				<< 7);		//   1次メッシュ経度2
	ParcelID += (p_DivPcl->e	<< 4);		//   2次メッシュ経度
	ParcelID += p_DivPcl->f;				// 2.5次メッシュ経度

	return (ParcelID);
}

double sc_MESH_GetRealLen(
							double		slatitude,	//	時（秒でないよ）
							double		slongitude,
							double		elatitude,
							double		elongitude
						)
{

#define PII 3.141592653589793238462        /* 円周率 */


	DOUBLE	f1,f2;						//	２地点の緯度(°)
	DOUBLE	fr1,fr2;					//	２地点の緯度(rad)
	DOUBLE	g1,g2;						//	２地点の経度（東経基準）(°)
	DOUBLE	gr1,gr2;					//	２地点の経度（東経基準）(rad)
	DOUBLE	h1,h2;						//	標高(m)
	DOUBLE	a=6378136.0;				//	赤道半径(m)
	DOUBLE	e2=0.006694470;				//	地球の離心率の自乗
	DOUBLE	x1,y1,z1,x2,y2,z2;			//	２地点の直交座標値(m)
	DOUBLE	r;							//	２地点間の直距離(m)
	DOUBLE	s;							//	２地点間の地表面距離(m)
	DOUBLE	w;							//	２地点間の半射程角(°) (中心角の１／２)
	DOUBLE	wr;							//	２地点間の半射程角(rad)
	DOUBLE	rad;						//	度→ラジアン変換係数
	DOUBLE	N1,N2;						//	緯度補正した地球の半径(m)

	//***********************************************	経度

	f1 = slatitude;
	g1 = slongitude;

	f2 = elatitude;
	g2 = elongitude;

    rad=PII/180.0;

	h1=h2=0.0;                                       /* ここでは、標高を無視 */

    if(g1<0) g1=360.0+g1;

    fr1=f1*rad; gr1=g1*rad;

    if(g2<0) g2=360.0+g2;

    fr2=f2*rad; gr2=g2*rad;

	N1=a/(sqrt(1.0-e2*sin(fr1)*sin(fr1)));

    x1=(N1+h1)*cos(fr1)*cos(gr1);

    y1=(N1+h1)*cos(fr1)*sin(gr1);

    z1=(N1*(1.0-e2)+h1)*sin(fr1);

	N2=a/(sqrt(1.0-e2*sin(fr2)*sin(fr2)));

    x2=(N2+h2)*cos(fr2)*cos(gr2);

    y2=(N2+h2)*cos(fr2)*sin(gr2);

    z2=(N2*(1.0-e2)+h2)*sin(fr2);

	r=sqrt((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2)+(z1-z2)*(z1-z2));		//	直距離

    wr=asin(r/2/a);													//	半射程角(rad)

    w=wr/rad;														//	半射程角(°)

    s=a*2*wr;														//	地表面距離

	return(s);

}
















//***************************************************************************************************************************
//                パーセル相対位置の算出
//***************************************************************************************************************************
INT32 SC_MESH_GetAlterPos(
		UINT32 base_parcel,			//	基準となるﾊﾟｰｾﾙID
		UINT32 target_parcel,		//	対象ﾊﾟｰｾﾙID
		INT32 level,				//	ﾒｯｼｭのﾚﾍﾞﾙ
		INT32 *alt_x,				//	ﾒｯｼｭの相対位置（Ｘ方向）
		INT32 *alt_y				//	ﾒｯｼｭの相対位置（Ｙ方向）
		)
{
	DOUBLE		base_latitude;		//	変換後の緯度（秒）
	DOUBLE		base_longitude;		//	変換後の経度（秒）
	DOUBLE		target_latitude;	//	変換後の緯度（秒）
	DOUBLE		target_longitude;	//	変換後の経度（秒）


	SC_MESH_ChgParcelIDToTitude(
							level,				 //	変換するﾚﾍﾞﾙ
							base_parcel,		//	変換するﾊﾟｰｾﾙID
							0,					//	変換する正規化Ｘ座標
							0,					//	変換する正規化Ｙ座標
							&base_latitude,		//	変換後の緯度（秒）
							&base_longitude		//	変換後の経度（秒）
	);
	SC_MESH_ChgParcelIDToTitude(
							level,				 //	変換するﾚﾍﾞﾙ
							target_parcel,		//	変換するﾊﾟｰｾﾙID
							0,					//	変換する正規化Ｘ座標
							0,					//	変換する正規化Ｙ座標
							&target_latitude,	//	変換後の緯度（秒）
							&target_longitude	//	変換後の経度（秒）
	);


	DOUBLE parcel_lat;		//	パーセル緯度単位（秒）
	DOUBLE parcel_lon;		//	パーセル経度単位（秒）

	parcel_lat = 2400.;
	parcel_lon = 3600.;

	switch (level) {
/*	case MAP_LEVELM1:
		parcel_lat /= (2 * 4 * 4 * 4);
		parcel_lon /= (2 * 4 * 4 * 4);
		break;
*/
	case 1:
		parcel_lat /= (2 * 4 * 4);
		parcel_lon /= (2 * 4 * 4);
		break;

	case 2:
		parcel_lat /= (2 * 4);
		parcel_lon /= (2 * 4);
		break;

	case 3:
		parcel_lat /= 2;
		parcel_lon /= 2;
		break;

	case 4:
		parcel_lat *= 2;
		parcel_lon *= 2;
		break;

	case 5:
		parcel_lat *= (2 * 4);
		parcel_lon *= (2 * 4);
		break;

	case 6:
		parcel_lat *= (2 * 4 * 4);
		parcel_lon *= (2 * 4 * 4);
		break;
/*
	case 6:
		parcel_lat = MAP_latitude_range;
		parcel_lon = MAP_longitude_range;
		break;
*/
	default:
		return (-1);
		break;
	}

	*alt_x = (INT32) ((target_longitude - base_longitude) / parcel_lon);	//	ﾒｯｼｭの相対位置（Ｘ方向）
	*alt_y = (INT32) ((target_latitude - base_latitude) / parcel_lat);		//	ﾒｯｼｭの相対位置（Ｙ方向）

//	if((target_longitude - 	base_longitude) != *alt_x * parcel_lon)
//		printf("????????????????????????\n");
//
//	if((target_latitude  - 	base_latitude) != *alt_y * parcel_lat)
//		printf("!!!!!!!!!!!!!!!!!!!!!!!!\n");

	return (0);
}

//********************************************************************************
//		１次メッシュ：	経度・・・３６００秒	緯度・・・２４００秒
//		２次メッシュ：	経度・・・　４５０秒	緯度・・・　３００秒
//							メッシュ当り			　経度（秒）    緯度（秒）
//		レベル－１：	１次メッシュ　＊　1/(8*4*4)			28.125       18.75
//		レベル　０：	１次メッシュ　＊　1/(8*4)		　 112.5         75
//		レベル　１：	１次メッシュ　＊　1/(8)			　 450          300
//		レベル　２：	１次メッシュ　＊　1/(2)		      1800         1200
//		レベル　３：	１次メッシュ　＊　2			　    7200         4800
//		レベル　４：	１次メッシュ　＊　(2*4)		     28800        19200
//		レベル　５：	１次メッシュ　＊　(2*4*4)	    115200        76800
//***************************************************************************************************************************
//					緯度・経度をＰＩＤ座標系に変換する
//***************************************************************************************************************************
INT32 SC_Lib_ChangeTitude2PID(DOUBLE latitude,		// 変換する緯度（秒）
		DOUBLE longitude,		// 変換する経度（秒）
		INT32 level,      	// 変換後のﾚﾍﾞﾙ
		UINT32 *parcel_id,		// 変換後のﾊﾟｰｾﾙID
		DOUBLE *x,         	// 変換後の正規化Ｘ座標
		DOUBLE *y				// 変換後の正規化Ｙ座標
		)
{
	// TODO 収録範囲考慮
	//if(latitude  > (MAP_BASE_latitude  + MAP_latitude_range )  ||  latitude  < (MAP_BASE_latitude ))	return(-1);
	//if(longitude > (MAP_BASE_longitude + MAP_longitude_range)  ||  longitude < (MAP_BASE_longitude))	return(-1);

	INT32 param1, param2, param3, param4, param5, param6, param7;

	// 基準点よりの位置を求める。
	DOUBLE len_latitude;		// 緯度（秒）
	DOUBLE len_longitude;		// 経度（秒）
	//len_latitude	= latitude/*  - MAP_BASE_latitude*/;
	//len_longitude	= longitude/* - MAP_BASE_longitude*/;
	len_latitude = latitude;
	len_longitude = longitude;

	if (len_longitude < 0) {
		param1 = 1;
	} else {
		param1 = 0;
	}

	DOUBLE parcel_lat;		// パーセル緯度単位（秒）
	DOUBLE parcel_lon;		// パーセル経度単位（秒）

	parcel_lat = 2400.;
	parcel_lon = 3600.;

	if (level == 3) {
		parcel_lat *= 2;
		parcel_lon *= 2;
	}
	if (level == 4) {
		parcel_lat *= (2 * 4);
		parcel_lon *= (2 * 4);
	}
	if (level == 5) {
		parcel_lat *= (2 * 4 * 4);
		parcel_lon *= (2 * 4 * 4);
	}
	if (level == 6) {
		//parcel_lat = MAP_latitude_range;
		//parcel_lon = MAP_longitude_range;
	}

	INT32 alt_y;	// １次メッシュ単位での収納範囲内での相対位置
	INT32 alt_x;

	alt_y = (INT32) (len_latitude / parcel_lat);
	alt_x = (INT32) (len_longitude / parcel_lon);

	if ((len_latitude - alt_y * parcel_lat) < 0) {
		alt_y--;
	}
	if ((len_longitude - alt_x * parcel_lon) < 0) {
		alt_x--;
	}

	DOUBLE w_latitude;			// １次メッシュ左下緯度（秒）
	DOUBLE w_longitude;			// １次メッシュ左下経度（秒）
	//w_latitude = MAP_BASE_latitude + alt_y * parcel_lat;			// １次メッシュ左下緯度
	//w_longitude = MAP_BASE_longitude + alt_x * parcel_lon;		// １次メッシュ左下経度
	w_latitude = alt_y * parcel_lat;	// １次メッシュ左下緯度
	w_longitude = alt_x * parcel_lon;	// １次メッシュ左下経度

	param2 = (INT32) (w_latitude / 2400.);
	param5 = (INT32) ((w_longitude - 100 * 3600) / 3600.);

	// １次メッシュ分を除いた残りの緯度経度
	len_latitude = len_latitude - alt_y * parcel_lat;
	len_longitude = len_longitude - alt_x * parcel_lon;

	param4 = 0xF;
	param7 = level + 1;

	if (level > 2) {
		param3 = 0;
		param6 = 0;
	} else {
		if (level == 2) {
			parcel_lat = 2400. / 2;
			parcel_lon = 3600. / 2;
			param3 = (INT32) (len_latitude / parcel_lat);
			param6 = (INT32) (len_longitude / parcel_lon);

			len_latitude = len_latitude - parcel_lat * param3;
			len_longitude = len_longitude - parcel_lon * param6;
		} else {
			parcel_lat = 2400. / 8;
			parcel_lon = 3600. / 8;
			param3 = (INT32) (len_latitude / parcel_lat);
			param6 = (INT32) (len_longitude / parcel_lon);

			len_latitude = len_latitude - parcel_lat * param3;
			len_longitude = len_longitude - parcel_lon * param6;

			if (level < 2) {
				if (level == 2) {
					parcel_lat = 2400. / 128;
					parcel_lon = 3600. / 128;
				} else {	// = MAP_LEVEL0
					parcel_lat = 2400. / 32;
					parcel_lon = 3600. / 32;
				}
				param4 = (INT32) (len_latitude / parcel_lat);
				param7 = (INT32) (len_longitude / parcel_lon);
				len_latitude = len_latitude - parcel_lat * param4;
				len_longitude = len_longitude - parcel_lon * param7;
				if (level == 1) {
					param4 |= 0xc;
					param7 |= 0xc;
				}
			}
		}
	}
	// パーセル内の正規化座標を算出
	DOUBLE w_parcel_y = len_latitude / parcel_lat * MAP_SIZE;		// パーセル緯度単位（秒）
	DOUBLE w_parcel_x = len_longitude / parcel_lon * MAP_SIZE;		// パーセル経度単位（秒）

	UINT32 w_parcel_id =
			  ((param1 << 31) & 0x80000000)
			| ((param2 << 23) & 0x7f800000)
			| ((param3 << 20) & 0x00700000)
			| ((param4 << 16) & 0x000f0000)
			| ((param5 <<  7) & 0x0000ff80)
			| ((param6 <<  4) & 0x00000070)
			| ((param7 <<  0) & 0x0000000f);

	*parcel_id = w_parcel_id;
	*y = w_parcel_y;
	*x = w_parcel_x;

	return (0);
}

/**
 * @brief		下位パーセルID算出処理
 *
 * @param		[in] ParcelId 自レベルパーセルID
 * @param		[in] outLevel 対象となる下位のパーセルのレベル
 * @return		指定されたレベルの左下に位置するパーセルID
 */
UINT32 SC_MESH_GetUnderLevelParcelID(const UINT32 baseParcelId, INT32 outLevel)
{
	INT32 x = 0;
	INT32 y = 0;
	INT32 pos = 1;
	INT32 loop = 16;
	INT32 i = 0;
	INT32 level;
	T_DIVIDE_PARCEL mesh;
	UINT32 ParcelId;

	// 自レベル算出
	level = SC_MESH_GetLevel(baseParcelId);
	if(1 >= level) {
		return (0);
	}

	if (level <= outLevel) {
		return (0);
	}

	ParcelId = baseParcelId;

	// 下位パーセルID算出
	while (level > outLevel) {

		sc_MESH_ParcelDivide(ParcelId, &mesh);

		switch(level)
		{
		case 2:		// レベル2の場合
			// 2.5次緯度メッシュコード算出
			mesh.c = 0xC | y;

			// 2.5次経度メッシュコード算出
			mesh.f = 0xC | x;

			break;

		case 3:		// レベル3の場合
			// 2次緯度メッシュコード算出
			if( 1 == mesh.b ){
				mesh.b += y+3;
			}
			else{
				mesh.b += y;
			}

			// 2次経度メッシュコード算出
			if( 1 == mesh.e ){
				mesh.e += x+3;
			}
			else{
				mesh.e += x;
			}

			// 予約コード格納
			mesh.c = 0xf;
			mesh.f = 2;

			break;

		case 4:		// レベル4の場合
			// 1次緯度メッシュコード算出
			if( 8 <= i ){
				mesh.a++;
			}

			// 1次経度メッシュコード算出
			if( ((4 <= i) && (7 >= i)) || (12 <= i) ){
				mesh.d++;
			}

			// 縦横位置を更新
			if( 0 == (i%4) ){
				pos = 1;
			}
			// 1次メッシュ1/4緯度メッシュコード算出
			if( 2 < pos ){
				mesh.b = 1;
			}
			else{
				mesh.b = 0;
			}
			// 1次メッシュ1/4経度メッシュコード算出
			if( 0 == (pos%2) ){
				mesh.e = 1;
			}
			else{
				mesh.e = 0;
			}
			pos++;

			// 予約コード格納
			mesh.c = 0xf;
			mesh.f = 3;
			break;

		default:
			break;
		}

		ParcelId = sc_MESH_ParcelCombine(&mesh);
		level -= 1;
	}
	return (ParcelId);
}
