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
 * @file	MeshCalc.c
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

#define	FMESH_WIDTH				28800	// １次メッシュ経度幅(1/8秒)
#define	FMESH_HEIGHT			19200	// １次メッシュ緯度幅(1/8秒)
#define	SMESH_WIDTH				3600	// ２次メッシュ経度幅(1/8秒)
#define	SMESH_HEIGHT			2400	// ２次メッシュ緯度幅(1/8秒)
#define	MESH25_WIDTH			900		// ２．５次メッシュ経度幅(1/8秒)
#define	MESH25_HEIGHT			600		// ２．５次メッシュ経度幅(1/8秒)
#define	MESH35_WIDTH			225		// ３．５次メッシュ経度幅(1/8秒)
#define	MESH35_HEIGHT			150		// ３．５次メッシュ経度幅(1/8秒)

// 基準緯度経度
//#define	MAP_BASE_LATITUDE		0.0
//#define	MAP_BASE_LONGITUDE		360000.0
#define	MAP_BASE_LATITUDE		76800.0
#define	MAP_BASE_LONGITUDE		417600.0
// 緯度経度幅
#define	MAP_LATITUDE_RANGE		153600.0
#define	MAP_LONGITUDE_RANGE		230400.0

// 円周率
#define PII						3.141592653589793238462


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


#define LOG2   0.693147180559945309417  //log(2)


static double gZoomLevelPow[20] = {
	0,
	2,
	4,
	8,
	16,
	32,
	64,
	128,
	256,
	512,
	1024,
	2048,
	4096,
	8192,
	16384,
	32768,
	65536,
	131072,
	262144,
	524288
};

static double log_fast(double x);

//-----------------------------------------------------------------------------
// 関数
//-----------------------------------------------------------------------------
void ParcelDivide(const UINT32 parcelID, T_DIVIDE_PARCEL *pDivPcl)
{
	INT32 d1, d2;

	pDivPcl->a  = _GET_A(WORD0(parcelID));		//   1次メッシュ緯度
	pDivPcl->b  = _GET_B(WORD0(parcelID));		//   2次メッシュ緯度
	pDivPcl->c  = _GET_C(WORD0(parcelID));		// 2.5次メッシュ緯度
	d1			= _GET_D1(WORD0(parcelID));		//   1次メッシュ経度1
	d2 			= _GET_D2(WORD1(parcelID));		//   1次メッシュ経度2
	pDivPcl->e  = _GET_E(WORD1(parcelID));		//   2次メッシュ経度
	pDivPcl->f  = _GET_F(WORD1(parcelID));		// 2.5次メッシュ経度
	pDivPcl->d	= (d1 << 9) | d2;				//   1次メッシュ経度
}

UINT32 ParcelCombine(T_DIVIDE_PARCEL *pDivPcl)
{
	UINT32 ParcelID = 0;
	INT32 d1;
	INT32 d2;

	d1 = (pDivPcl->d & 0x200) >> 9;
	d2 = (pDivPcl->d & 0x1FF);

	ParcelID += (d1			<< 31);		//   1次メッシュ経度1
	ParcelID += (pDivPcl->a	<< 23);		//   1次メッシュ緯度
	ParcelID += (pDivPcl->b	<< 20);		//   2次メッシュ緯度
	ParcelID += (pDivPcl->c	<< 16);		// 2.5次メッシュ緯度
	ParcelID += (d2			<< 7);		//   1次メッシュ経度2
	ParcelID += (pDivPcl->e	<< 4);		//   2次メッシュ経度
	ParcelID += (pDivPcl->f		);		// 2.5次メッシュ経度

	return (ParcelID);
}

INT32 MESHC_GetLevel(const UINT32 parcelID)
{
	INT32 level = 0;
	UINT32 c = _GET_C(WORD0(parcelID));
	UINT32 f = _GET_F(WORD1(parcelID));

	if( (c==0xf) && (f==MAP_LEVEL6) ) {
		level = MAP_LEVEL6;
	} else if( (c==0xf) && (f==MAP_LEVEL5) ) {
		level = MAP_LEVEL5;
	} else if( (c==0xf) && (f==MAP_LEVEL4) ) {
		level = MAP_LEVEL4;
	} else if( (c==0xf) && (f==MAP_LEVEL3) ) {
		level = MAP_LEVEL3;
	} else if( (c==0xf) && (f==MAP_LEVEL2) ) {
		level = MAP_LEVEL2;
	} else {
		level = MAP_LEVEL1;
	}

	return (level);
}

UINT32 MESHC_GetUpperParcelID(const UINT32 parcelID)
{
	INT32 Level = 0;
	UINT32 out_ParcelId = 0xffffffff;
	T_DIVIDE_PARCEL DivPar;

	Level = MESHC_GetLevel(parcelID);

	// パーセル分割
	ParcelDivide(parcelID, &DivPar);

	// 一つ上のレベルのパーセル設定
	switch(Level)
	{
	case 1:
		DivPar.c	= 0x0F;							// 2.5次メッシュ緯度を予約にする
		DivPar.f	= MAP_LEVEL2;					// 2.5次メッシュ経度をLv2の予約にする
		break;
	case 2:
		DivPar.b	/= 4;							//   2次メッシュ緯度を1次メッシュ緯度の2分割にする(0or1のみ)
		DivPar.e	/= 4;							//   2次メッシュ経度を1次メッシュ経度の2分割にする(0or1のみ)
		DivPar.c	= 0x0F;							// 2.5次メッシュ緯度を予約にする
		DivPar.f	= MAP_LEVEL3;					// 2.5次メッシュ経度をLv3の予約にする
		break;
	case 3:
		DivPar.a	= (DivPar.a / 2) * 2;			//   1次メッシュ経度  1次2枚の西側
		DivPar.d	= (DivPar.d / 2) * 2;			//   1次メッシュ緯度2 1次2枚の南側
		DivPar.b	= 0x00;							//   2次メッシュ緯度を未使用にする
		DivPar.e	= 0x00;							//   2次メッシュ経度を未使用にする
		DivPar.c	= 0x0F;							// 2.5次メッシュ緯度を予約にする
		DivPar.f	= MAP_LEVEL4;					// 2.5次メッシュ経度をLv4の予約にする
		break;
	case 4:
		DivPar.a	= (((DivPar.a - BASE_LAT) / 8) * 8)+BASE_LAT;//   1次メッシュ経度  1次8枚の西側
		DivPar.d	= (((DivPar.d - BASE_LON) / 8) * 8)+BASE_LON;//   1次メッシュ緯度2 1次8枚の南側
		DivPar.b	= 0x00;							//   2次メッシュ緯度を未使用にする
		DivPar.e	= 0x00;							//   2次メッシュ経度を未使用にする
		DivPar.c	= 0x0F;							// 2.5次メッシュ緯度を予約にする
		DivPar.f	= MAP_LEVEL5;					// 2.5次メッシュ経度をLv5の予約にする
		break;
	case 5:
		//DivPar.a	= (((DivPar.a - BASE_LAT) / 32) * 32)+BASE_LAT;	//   1次メッシュ経度  1次32枚の西側
		//DivPar.d	= (((DivPar.d - BASE_LON) / 32) * 32)+BASE_LON;	//   1次メッシュ緯度2 1次32枚の南側
		DivPar.a	= (((DivPar.a - BASE_LAT) / 40) * 40)+BASE_LAT;	//   1次メッシュ経度  1次32枚の西側
		DivPar.d	= (((DivPar.d - BASE_LON) / 40) * 40)+BASE_LON;	//   1次メッシュ緯度2 1次32枚の南側
		DivPar.b	= 0x00;							//   2次メッシュ緯度を未使用にする
		DivPar.e	= 0x00;							//   2次メッシュ経度を未使用にする
		DivPar.c	= 0x0F;							// 2.5次メッシュ緯度を予約にする
		DivPar.f	= MAP_LEVEL6;					// 2.5次メッシュ経度をLv5の予約にする
		break;
	default:
		return (out_ParcelId);
	}

	// パーセル組み立て
	out_ParcelId = ParcelCombine(&DivPar);

	return (out_ParcelId);
}

INT32 MESHC_GetUnderParcelID(
		const UINT32		parcelID,
		T_UNDER_PARCEL_LIST	*pUnderParcelList
		)
{
	INT32 x = 0;
	INT32 y = 0;
	INT32 pos = 1;
	INT32 loop = UNDER_PARCEL_CNT;
	INT32 i = 0;
	INT32 level;
	T_DIVIDE_PARCEL mesh;

	memset(pUnderParcelList, 0, sizeof(T_UNDER_PARCEL_LIST));

	// 自レベル算出
	level = MESHC_GetLevel(parcelID);
	if(MAP_LEVEL1 >= level) {
		return (0);
	}

	// 下位パーセルID算出
	for(i=0; i<loop; i++) {

		ParcelDivide(parcelID, &mesh);

		switch(level)
		{
		case MAP_LEVEL2:		// レベル2の場合
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

		case MAP_LEVEL3:		// レベル3の場合
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

		case MAP_LEVEL4:		// レベル4の場合
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

		case MAP_LEVEL5:		// レベル5の場合
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

		pUnderParcelList->ParcelId[i] = ParcelCombine(&mesh);
	}

	return (i);
}

UINT32 MESHC_GetNextParcelID(
		const UINT32 parcelID,
		const UINT8 dir
		)
{
	UINT32 out_ParcelId = 0xFFFFFFFF;

	switch( dir )
	{
	case DIR_TOP:		out_ParcelId = MESHC_SftParcelID(parcelID,  0,  1);	break;
	case DIR_R_TOP:		out_ParcelId = MESHC_SftParcelID(parcelID,  1,  1);	break;
	case DIR_R:			out_ParcelId = MESHC_SftParcelID(parcelID,  1,  0);	break;
	case DIR_R_DOWN:	out_ParcelId = MESHC_SftParcelID(parcelID,  1, -1);	break;
	case DIR_DOWN:		out_ParcelId = MESHC_SftParcelID(parcelID,  0, -1);	break;
	case DIR_L_DOWN:	out_ParcelId = MESHC_SftParcelID(parcelID, -1, -1);	break;
	case DIR_L:			out_ParcelId = MESHC_SftParcelID(parcelID, -1,  0);	break;
	case DIR_L_TOP:		out_ParcelId = MESHC_SftParcelID(parcelID, -1,  1);	break;
	default:																break;
	}
	return (out_ParcelId);
}

UINT32 MESHC_SftParcelID(
		const UINT32	parcelID,
		const INT16		xVol,
		const INT16		yVol
		)
{

	UINT32	out_ParcelId = 0xFFFFFFFF;
	INT32	level;
	INT32	lat_next;	// 緯度方向の相対コード
	INT32	lon_next;	// 経度方向の相対コード
	T_DIVIDE_PARCEL mesh;

	// レベル取得
	level = MESHC_GetLevel(parcelID);

	// メッシュコードを分解して取得
	ParcelDivide(parcelID, &mesh);

	// シフト処理
	switch(level)
	{
	case MAP_LEVEL6:
		//mesh.a = mesh.a + (yVol * 32);	// 1次32枚
		//mesh.d = mesh.d + (xVol * 32);	// 1次32枚
		mesh.a = mesh.a + (yVol * 40);	// 1次40枚
		mesh.d = mesh.d + (xVol * 40);	// 1次40枚
		break;

	case MAP_LEVEL5:
		mesh.a = mesh.a + (yVol * 8);	// 1次8枚
		mesh.d = mesh.d + (xVol * 8);	// 1次8枚
		break;

	case MAP_LEVEL4:
		mesh.a = mesh.a + (yVol * 2);	// 1次2枚
		mesh.d = mesh.d + (xVol * 2);	// 1次2枚
		break;

	case MAP_LEVEL3:
		// 緯度方向算出
		mesh.b += yVol;
		convValidCode(mesh.b, lat_next, 2);
		mesh.a += lat_next;

		// 経度方向算出
		mesh.e += xVol;
		convValidCode(mesh.e, lon_next, 2);
		mesh.d += lon_next;
		break;

	case MAP_LEVEL2:
		// 緯度方向算出
		mesh.b += yVol;
		convValidCode(mesh.b, lat_next, 8);
		mesh.a += lat_next;

		// 経度方向算出
		mesh.e += xVol;
		convValidCode(mesh.e, lon_next, 8);
		mesh.d += lon_next;
		break;

	case MAP_LEVEL1:
		// 緯度方向算出
		mesh.c &= 0x03; // 分割ビットOFF
		mesh.c += yVol;
		convValidCode(mesh.c, lat_next, 4);
		mesh.b += lat_next;
		convValidCode(mesh.b, lat_next, 8);
		mesh.a += lat_next;
		mesh.c |= 0x0c; // 分割ビットON

		// 経度方向算出
		mesh.f &= 0x03; // 分割ビットOFF
		mesh.f += xVol;
		convValidCode(mesh.f, lon_next, 4);
		mesh.e += lon_next;
		convValidCode(mesh.e, lon_next, 8);
		mesh.d += lon_next;
		mesh.f |= 0x0c; // 分割ビットON
		break;

	default:
		break;
	}

	out_ParcelId = ParcelCombine(&mesh);

	return (out_ParcelId);
}

INT32 MESHC_ChgParcelIDToLatLon(
		const INT32		level,		// 変換するﾚﾍﾞﾙ
		const UINT32	parcelID,	// 変換するﾊﾟｰｾﾙID
		const DOUBLE	x,			// 変換する正規化Ｘ座標
		const DOUBLE	y,			// 変換する正規化Ｙ座標
		DOUBLE			*pLatitude,	// 変換後の緯度（秒）
		DOUBLE			*pLongitude	// 変換後の経度（秒）
		)
{

	//********************************************************************************************
	//		１次メッシュ：	経度・・・３６００秒	緯度・・・２４００秒
	//		２次メッシュ：	経度・・・　４５０秒	緯度・・・　３００秒
	//							メッシュ当り			　経度（秒）    緯度（秒）
	//		レベル　１：	１次メッシュ　＊　1/(8*4)		　 112.5         75
	//		レベル　２：	１次メッシュ　＊　1/(8)			　 450          300
	//		レベル　３：	１次メッシュ　＊　1/(2)		      1800         1200
	//		レベル　４：	１次メッシュ　＊　2			　    7200         4800
	//		レベル　５：	１次メッシュ　＊　(2*4)		     28800        19200
	//		レベル　６：	１次メッシュ　＊　(2*4*4)	    115200        76800
	//		レベル　６：	１次メッシュ　＊　(2*4*5)	    144000        96000
	//********************************************************************************************
	INT32  w_h;
	INT32  w_l;

	switch(level)
	{
	case MAP_LEVEL1:
		*pLatitude  =	(
							(DOUBLE)(((int)(parcelID >> 23)) & 0xff)			+
							(DOUBLE)((parcelID >> 20) & 0x00000007)/8			+
							(DOUBLE)((parcelID >> 16) & 0x00000003)/32			+
							(DOUBLE)y/(32 * MAP_SIZE)
						) * 2400;
		*pLongitude  =	(
							(DOUBLE)((int)((parcelID  & 0x80000000) | ((parcelID << 15) & 0x7fc00000)) >> 22)	+
							100															+
							(DOUBLE)((parcelID >>  4) & 0x00000007)/8					+
							(DOUBLE)((parcelID      ) & 0x00000003)/32					+
							(DOUBLE)x/(32 * MAP_SIZE)
						) * 3600;
		break;
	case MAP_LEVEL2:
		*pLatitude  =	(
							(DOUBLE)(((int)(parcelID >> 23)) & 0xff)			+
							(DOUBLE)((parcelID >> 20) & 0x00000007)/8			+
							(DOUBLE)y/(8 * MAP_SIZE)
						) * 2400;
		*pLongitude  =	(
							(DOUBLE)((int)((parcelID  & 0x80000000) | ((parcelID << 15) & 0x7fc00000)) >> 22)	+
							100															+
							(DOUBLE)((parcelID >>  4) & 0x00000007)/8					+
							(DOUBLE)x/(8 * MAP_SIZE)
						) * 3600;
		break;
	case MAP_LEVEL3:
		*pLatitude  =	(
							(DOUBLE)(((int)(parcelID >> 23)) & 0xff)			+
							(DOUBLE)((parcelID >> 20) & 0x00000001)/2			+
							(DOUBLE)y/(2 * MAP_SIZE)
						) * 2400;
		*pLongitude  =	(
							(DOUBLE)((int)((parcelID  & 0x80000000) | ((parcelID << 15) & 0x7fc00000)) >> 22)	+
							100															+
							(DOUBLE)((parcelID >>  4) & 0x00000001)/2					+
							(DOUBLE)x/(2 * MAP_SIZE)
						) * 3600;
		break;
	case MAP_LEVEL4:
		*pLatitude  =	(
							(DOUBLE)(((int)(parcelID >> 23)) & 0xff)			+
							(DOUBLE)y*2/MAP_SIZE
						) * 2400;
		*pLongitude  =	(
							(DOUBLE)((int)((parcelID  & 0x80000000) | ((parcelID << 15) & 0x7fc00000)) >> 22)	+
							100																			+
							(DOUBLE)x*2/MAP_SIZE
						) * 3600;
		break;
	case MAP_LEVEL5:
		*pLatitude  =	(
							(DOUBLE)(((int)(parcelID >> 23)) & 0xff)			+
							(DOUBLE)y*8/MAP_SIZE
						) * 2400;
		*pLongitude  =	(
							(DOUBLE)((int)((parcelID  & 0x80000000) | ((parcelID << 15) & 0x7fc00000)) >> 22)	+
							100																			+
							(DOUBLE)x*8/MAP_SIZE
						) * 3600;
		break;
	case MAP_LEVEL6:
		w_h = (parcelID >> 23) & 0xff;
		w_l = (((parcelID  & 0x80000000) | ((parcelID << 15) & 0x7fc00000)) >> 22)	+	100;
		*pLatitude  =	(
							(DOUBLE)(((int)(parcelID >> 23)) & 0xff)			+
							//(DOUBLE)y*32/MAP_SIZE
							(DOUBLE)y*40/MAP_SIZE
						) * 2400;
		*pLongitude  =	(
							(DOUBLE)((int)((parcelID  & 0x80000000) | ((parcelID << 15) & 0x7fc00000)) >> 22)	+
							100																			+
							//(DOUBLE)x*32/MAP_SIZE
							(DOUBLE)x*40/MAP_SIZE
						) * 3600;
		break;
	default:
		return(-1);
	}

	return(0);
}

INT32 MESHC_ChgLatLonToParcelID(
		const DOUBLE	latitude,	// 変換する緯度（秒）
		const DOUBLE	longitude,	// 変換する経度（秒）
		const INT32		level,		// 変換後のレベル
		UINT32			*pParcelID,	// 変換後のパーセルID
		DOUBLE			*pX,		// 変換後の正規化Ｘ座標
		DOUBLE			*pY			// 変換後の正規化Ｙ座標
		)
{
	//********************************************************************************************
	//		１次メッシュ：	経度・・・３６００秒	緯度・・・２４００秒
	//		２次メッシュ：	経度・・・　４５０秒	緯度・・・　３００秒
	//							メッシュ当り			　経度（秒）    緯度（秒）
	//		レベル　１：	１次メッシュ　＊　1/(8*4)		　 112.5         75
	//		レベル　２：	１次メッシュ　＊　1/(8)			　 450          300
	//		レベル　３：	１次メッシュ　＊　1/(2)		      1800         1200
	//		レベル　４：	１次メッシュ　＊　2			　    7200         4800
	//		レベル　５：	１次メッシュ　＊　(2*4)		     28800        19200
	//		レベル　６：	１次メッシュ　＊　(2*4*4)	    115200        76800
	//********************************************************************************************

	INT32 param1, param2, param3, param4, param5, param6, param7;
	DOUBLE	len_latitude;	// 緯度（秒）
	DOUBLE	len_longitude;	// 経度（秒）
	DOUBLE	parcel_lat;		// パーセル緯度単位（秒）
	DOUBLE	parcel_lon;		// パーセル経度単位（秒）
	INT32	alt_y;			// １次メッシュ単位での収納範囲内での相対位置
	INT32	alt_x;
	DOUBLE	w_latitude;		// １次メッシュ左下緯度（秒）
	DOUBLE	w_longitude;	// １次メッシュ左下経度（秒）
	DOUBLE	w_parcel_y;
	DOUBLE	w_parcel_x;
	UINT32	w_parcel_id;


	// TODO 収録範囲考慮
	//if(latitude  > (MAP_BASE_latitude  + MAP_latitude_range )  ||  latitude  < (MAP_BASE_latitude ))	return(-1);
	//if(longitude > (MAP_BASE_longitude + MAP_longitude_range)  ||  longitude < (MAP_BASE_longitude))	return(-1);

	// 基準点よりの位置を求める。
	len_latitude	= latitude  - MAP_BASE_LATITUDE;
	len_longitude	= longitude - MAP_BASE_LONGITUDE;

	if(len_longitude < 0) {
		param1 = 1;
	} else {
		param1 = 0;
	}

	parcel_lat = 2400.;
	parcel_lon = 3600.;

	if(level == MAP_LEVEL4)	{
		parcel_lat *= 2;
		parcel_lon *= 2;
	}
	if(level == MAP_LEVEL5)	{
		parcel_lat *= (2 * 4);
		parcel_lon *= (2 * 4);
	}
	if(level == MAP_LEVEL6)	{
		//parcel_lat *= (2 * 4 * 4);
		//parcel_lon *= (2 * 4 * 4);
		parcel_lat *= (2 * 4 * 5);
		parcel_lon *= (2 * 4 * 5);
	}

	alt_y = (INT32) (len_latitude / parcel_lat);
	alt_x = (INT32) (len_longitude / parcel_lon);

	if ((len_latitude - alt_y * parcel_lat) < 0) {
		alt_y--;
	}
	if ((len_longitude - alt_x * parcel_lon) < 0) {
		alt_x--;
	}

	w_latitude	= MAP_BASE_LATITUDE		+ alt_y * parcel_lat;	//	１次メッシュ左下緯度
	w_longitude = MAP_BASE_LONGITUDE	+ alt_x * parcel_lon;	//	１次メッシュ左下経度

	param2 = (INT32) (w_latitude / 2400.);
	param5 = (INT32) ((w_longitude - 100 * 3600) / 3600.);

	// １次メッシュ分を除いた残りの緯度経度
	len_latitude = len_latitude - alt_y * parcel_lat;
	len_longitude = len_longitude - alt_x * parcel_lon;

	param4 = 0xF;
	param7 = level/* + 1*/;

	if (level > MAP_LEVEL3) {
		param3 = 0;
		param6 = 0;
	} else {
		if (level == MAP_LEVEL3) {
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

			if (level == MAP_LEVEL1) {
				parcel_lat = 2400. / 32;
				parcel_lon = 3600. / 32;

				param4 = (INT32) (len_latitude / parcel_lat);
				param7 = (INT32) (len_longitude / parcel_lon);
				len_latitude = len_latitude - parcel_lat * param4;
				len_longitude = len_longitude - parcel_lon * param7;

				param4 |= 0xc;
				param7 |= 0xc;
			}
		}
	}
	// パーセル内の正規化座標を算出
	w_parcel_y = len_latitude / parcel_lat * MAP_SIZE;		// パーセル緯度単位（秒）
	w_parcel_x = len_longitude / parcel_lon * MAP_SIZE;		// パーセル経度単位（秒）

	w_parcel_id =
			  ((param1 << 31) & 0x80000000)
			| ((param2 << 23) & 0x7f800000)
			| ((param3 << 20) & 0x00700000)
			| ((param4 << 16) & 0x000f0000)
			| ((param5 <<  7) & 0x0000ff80)
			| ((param6 <<  4) & 0x00000070)
			| ((param7 <<  0) & 0x0000000f);

	*pParcelID = w_parcel_id;
	*pY = w_parcel_y;
	*pX = w_parcel_x;

	return (0);
}

void MESHC_GetRealSize(
		const DOUBLE	latitude,
		const DOUBLE	longitude,
		DOUBLE			*pLen
		)
{

	DOUBLE	f1;		// ２地点の緯度(°)
	DOUBLE	g1;		// ２地点の経度（東経基準）(°)
	DOUBLE	s;		// ２地点間の地表面距離(m)

	f1 = latitude/3600.0;
	g1 = longitude/3600.0;

	// 経度
	s = MESHC_GetRealLen(f1, g1, f1, g1 + 1.0/8.0);
	*pLen	= s / 3600. * 8.0;

	// 緯度
	s = MESHC_GetRealLen(f1, g1, f1 + 1.0/12.0, g1);
	*(pLen+1)=s / 3600. * 12.0;
}

DOUBLE MESHC_GetRealLen(
		const DOUBLE	sLatitude,
		const DOUBLE	sLongitude,
		const DOUBLE	eLatitude,
		const DOUBLE	eLongitude
		)
{
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


	f1 = sLatitude;
	g1 = sLongitude;

	f2 = eLatitude;
	g2 = eLongitude;

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

INT32 MESHC_GetParcelAlterPos(
		const UINT32	baseParcel,
		const UINT32	targetParcel,
		const INT32		level,
		INT32			*pAltX,
		INT32			*pAltY
		)
{
	DOUBLE	base_latitude;		//	変換後の緯度（秒）
	DOUBLE	base_longitude;		//	変換後の経度（秒）
	DOUBLE	target_latitude;	//	変換後の緯度（秒）
	DOUBLE	target_longitude;	//	変換後の経度（秒）
	DOUBLE	parcel_lat;			//	パーセル緯度単位（秒）
	DOUBLE	parcel_lon;			//	パーセル経度単位（秒）


	MESHC_ChgParcelIDToLatLon(level, baseParcel, 0, 0, &base_latitude, &base_longitude);
	MESHC_ChgParcelIDToLatLon(level, targetParcel, 0, 0, &target_latitude, &target_longitude);

	parcel_lat = 2400.;
	parcel_lon = 3600.;

	switch (level)
	{
	case MAP_LEVEL1:
		parcel_lat /= (2 * 4 * 4);
		parcel_lon /= (2 * 4 * 4);
		break;

	case MAP_LEVEL2:
		parcel_lat /= (2 * 4);
		parcel_lon /= (2 * 4);
		break;

	case MAP_LEVEL3:
		parcel_lat /= 2;
		parcel_lon /= 2;
		break;

	case MAP_LEVEL4:
		parcel_lat *= 2;
		parcel_lon *= 2;
		break;

	case MAP_LEVEL5:
		parcel_lat *= (2 * 4);
		parcel_lon *= (2 * 4);
		break;

	case MAP_LEVEL6:
		//parcel_lat *= (2 * 4 * 4);
		//parcel_lon *= (2 * 4 * 4);
		parcel_lat *= (2 * 4 * 5);
		parcel_lon *= (2 * 4 * 5);
		break;

	default:
		return (-1);
		break;
	}

	*pAltX = (INT32) ((target_longitude - base_longitude) / parcel_lon);	//	ﾒｯｼｭの相対位置（Ｘ方向）
	*pAltY = (INT32) ((target_latitude - base_latitude) / parcel_lat);		//	ﾒｯｼｭの相対位置（Ｙ方向）

//	if((target_longitude - 	base_longitude) != *alt_x * parcel_lon)
//		printf("????????????????????????\n");
//
//	if((target_latitude  - 	base_latitude) != *alt_y * parcel_lat)
//		printf("!!!!!!!!!!!!!!!!!!!!!!!!\n");

	return (0);
}

void MESHC_ChgLatLonToTilePixelCoord(
		const DOUBLE	latitude,
		const DOUBLE	longitude,
		const INT32		zoomLevel,
		DOUBLE			*pPixelCoordX,
		DOUBLE			*pPixelCoordY
		)
{
	DOUBLE lat_rad;
	DOUBLE lng_rad;
	DOUBLE R;
	DOUBLE worldCoord_x;
	DOUBLE worldCoord_y;

	// ラジアン変換
	lat_rad = latitude * PII / 180;
	lng_rad = longitude * PII / 180;

	// この緯度経度の値に対応する世界座標
	R = 128 / PII;
	worldCoord_x = R * (lng_rad + PII);
	worldCoord_y = - R / 2 * /*log*/log_fast( (1 + sin(lat_rad)) / (1 - sin(lat_rad)) ) +128;

#ifdef _COP_WORLD_COORD_OFS
	worldCoord_x -=200;
	worldCoord_y -=100;
#endif // #ifdef _COP_WORLD_COORD_OFS

	// 世界座標からピクセル座標
	*pPixelCoordX = worldCoord_x * gZoomLevelPow[zoomLevel];
	*pPixelCoordY = worldCoord_y * gZoomLevelPow[zoomLevel];

	return;
}

void MESHC_ChgLatLonToTileWorldCoord(
		const DOUBLE	latitude,
		const DOUBLE	longitude,
		DOUBLE			*pWorldCoordX,
		DOUBLE			*pWorldCoordY
		)
{
	DOUBLE lat_rad;
	DOUBLE lng_rad;
	DOUBLE R;

	// ラジアン変換
	lat_rad = latitude * PII / 180;
	lng_rad = longitude * PII / 180;

	// この緯度経度の値に対応する世界座標
	R = 128 / PII;
	*pWorldCoordX = R * (lng_rad + PII);
	*pWorldCoordY = - R / 2 * /*log*/log_fast( (1 + sin(lat_rad)) / (1 - sin(lat_rad)) ) +128;

#ifdef _COP_WORLD_COORD_OFS
	*pWorldCoordX -= 200;
	*pWorldCoordY -= 100;
#endif // #ifdef _COP_WORLD_COORD_OFS

	return;
}

void MESHC_ChgTilePixelCoordToLatLon(
		const INT32		pixelCoordX,
		const INT32		pixelCoordY,
		const INT32		zoomLevel,
		DOUBLE			*pLatitude,
		DOUBLE			*pLongitude
		)
{
	DOUBLE lat_rad;
	DOUBLE lng_rad;
	DOUBLE R;

	DOUBLE worldCoord_x = pixelCoordX / gZoomLevelPow[zoomLevel];
	DOUBLE worldCoord_y = pixelCoordY / gZoomLevelPow[zoomLevel];

#ifdef _COP_WORLD_COORD_OFS
	worldCoord_x += 200.0;
	worldCoord_y += 100.0;
#endif // _COP_WORLD_COORD_OFS

    R = 128 / PII;
	lat_rad = atan(sinh( (128 - worldCoord_y) / R ));
	lng_rad = worldCoord_x / R - PII;

	*pLatitude = lat_rad * 180 / PII;
	*pLongitude = lng_rad * 180 / PII;

	return;
}

double log_fast(double x)
{
	static double table[17]={
		.0                       ,  // log( 16 /16)
		.0606246218164348425806  ,  // log( 17 /16)
		.1177830356563834545387  ,  // log( 18 /16)
		.17185025692665922234    ,  // log( 19 /16)
		.2231435513142097557662  ,  // log( 20 /16)
		.2719337154836417588316  ,  // log( 21 /16)
		.3184537311185346158102  ,  // log( 22 /16)
		.3629054936893684531378  ,  // log( 23 /16)
		.405465108108164381978   ,  // log( 24 /16)
		.4462871026284195115325  ,  // log( 25 /16)
		.4855078157817008078017  ,  // log( 26 /16)
		.5232481437645478365168  ,  // log( 27 /16)
		.5596157879354226862708  ,  // log( 28 /16)
		.5947071077466927895143  ,  // log( 29 /16)
		.6286086594223741377443  ,  // log( 30 /16)
		.6613984822453650082602  ,  // log( 31 /16)
		.6931471805599453094172  ,  // log( 32 /16)
	};
	unsigned long long w,kasuu16;
	int q;
	double y,h,z;
	w=*(unsigned long long*)&x;
	q=(((int)(w>>47)&0x1F)+1)>>1;
	kasuu16=(w & 0xFFFFFFFFFFFFFULL)^0x4030000000000000ULL;//仮数*16  16<=kasuu16<32
	h=*(double*)&kasuu16;
	z=(double)(q+16);
	h=(h-z)/(h+z);
	z=h*h;
	y=(2.0/9)*z+2.0/7;
	y=y*z+2.0/5;
	y=y*z+2.0/3;
	y=y*z+2.0;
	y=y*h;
	return (((int)(w>>52)-1023)*LOG2+table[q]+y);
}
