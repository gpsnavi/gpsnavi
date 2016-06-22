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
 * SMCoreRP.h
 *
 *  Created on: 2015/11/11
 *      Author: masutani
 */

#ifndef SMCORERP_H_
#define SMCORERP_H_

#define SC_RP_USER_MAP			(0x00000001)						// ユーザ：MAP
#define SC_RP_USER_RP			(0x00000002)						// ユーザ：RP
#define SC_RP_USER_RG			(0x00000004)						// ユーザ：RG
#define SC_RP_USER_LC			(0x00000008)						// ユーザ：LC

/**
 * RP_Route
 */
#define RC_ROUTE_VERSION		"1.0"				// 推奨経路Ver 20160128
#define SC_RP_ROUTEVER_SIZE		(32)				// 経路Verサイズ
#define SC_RP_MAPVER_SIZE		(128)				// 地図Verサイズ

#define SC_RP_SECT_MAX			(SC_CORE_RP_PLACE_MAX - 1)	// 最大区間数
#define SC_RP_TERM_IS_MID		(0)					// termフラグ 始終端以外
#define SC_RP_TERM_IS_FIRST		(1)					// 始端フラグ
#define SC_RP_TERM_IS_LAST		(2)					// 終端フラグ
#define SC_RP_DIR_IS_ODR		(0)					// リンク順方向
#define SC_RP_DIR_IS_RVS		(1)					// リンク逆方向
#define SC_RP_SPLIT_LINK		(1)					// 断裂
#define SC_RP_SPLIT_INDEX_INIT	(0xFFFF)			// 断裂INDEX無効値
#define SC_RP_RTIDINIT			(0xFFFFFFFF)		/* 経路ID無効値 */
#define SC_RP_RTTYPEINIT		(RPC_SIZE)			/* 経路種別無効値 */
/*-------------------------------------------------------------------
 * 経路タイプ定義
 *-------------------------------------------------------------------*/
typedef enum _E_SC_ROUTETYPE {
	e_SC_ROUTETYPE_SINGLE = 0x00000000,
	e_SC_ROUTETYPE_MULTI,
	e_SC_ROUTETYPE_REROUTE,
	e_SC_ROUTETYPE_END
} E_SC_ROUTETYPE;
/*-------------------------------------------------------------------
 * 構造体定義
 *-------------------------------------------------------------------*/
typedef struct {/*----------------------// 形状情報 -----------------*/
	UINT16 x;							// X
	UINT16 y;							// Y
} SC_RP_FormInfo;
typedef struct {/*----------------------// 規制情報 -----------------*/
	UINT32 regCode[4];					// 通行コード（地図フォーマットのまま格納）
} SC_RP_RegInfo;
typedef struct {/*----------------------// リンク情報 ---------------*/
	UINT32 linkId;						// リンクID
	UINT8 roadKind;						// 道路種別
	UINT8 linkKind;						// リンク種別
	UINT32 dist;						// リンク長
	UINT32 travelTime;					// 平均旅行時間
	struct {
		UINT16 reserve:9;
		UINT16 splitFlag:1;				// 断裂リンクフラグ
		UINT16 termFlag:2;				// 区間始終端リンクフラグ
		UINT16 orFlag:1;				// リンク方向 0:順 1:逆
		UINT16 level:3;					// レベル
	};									// フラグ情報
	UINT16 regIdx;						// 規制INDEX
	UINT16 regVol;						// 規制数
	UINT16 formIdx;						// 形状点INDEX
	UINT16 formVol;						// 形状点数
} SC_RP_LinkInfo;
typedef struct {/*----------------------// パーセル情報 -------------*/
	UINT32 parcelId;					// パーセルID
	UINT16 linkIdx;						// リンクINDEX
	UINT16 linkVol;						// リンク数
	UINT32 preDist;						// 当該パーセル前までのリンク長
	UINT32 preTime;						// 当該パーセル前までの旅行時間
	UINT8 level;						// レベル
	UINT8 reserve[3];
	UINT16 regIdx;						// 規制INDEX
	UINT16 regVol;						// 規制数
	UINT16 subLinkIdx;					// 形状点INDEX
	UINT16 subLinkVol;					// 形状点数
} SC_RP_ParcelInfo;
typedef struct {/*----------------------// 区間情報 -----------------*/
	UINT8 priority;						// 探索種別
	UINT8 sectNumber;					// 区間番号(0始まり)
	UINT32 sectDist;					// 区間距離
	UINT32 sectHWDist;					// 区間高速距離
	UINT16 parcelIdx;					// パーセルINDEX
	UINT16 parcelVol;					// パーセル情報数
	UINT16 linkIdx;						// リンクINDEX
	UINT16 linkVol;						// リンク情報数
	UINT16 formIdx;						// 形状INDEX
	UINT16 formVol;						// 形状情報数
	UINT32 sectTime;					// 区間所要時間
	UINT16 splitIdx;					// 区間内での断裂開始リンクINDEX(ALL_F無効)
	UINT32 parcelId;					// 区間目的地パーセル
	UINT16 x;							// 区間目的地座標X
	UINT16 y;							// 区間目的地座標Y
} SC_RP_SectInfo;
typedef struct {/*----------------------// 推奨経路マネージャ -------*/
	Char routeVer[SC_RP_ROUTEVER_SIZE];	// 推奨経路Ver
	Char mapVer[SC_RP_MAPVER_SIZE];		// 地図Ver
	E_SC_ROUTETYPE routeType;			// 経路タイプ
	SC_RP_SectInfo* sectInfo;			// 区間情報
	UINT8 sectVol;						// 区間数
	SC_RP_ParcelInfo* parcelInfo;		// パーセル情報
	UINT16 parcelVol;					// パーセル情報数
	SC_RP_LinkInfo* linkInfo;			// リンク情報
	UINT16 linkVol;						// リンク情報数
	SC_RP_FormInfo* formInfo;			// 形状点情報
	UINT16 formVol;						// 形状点情報数
	SC_RP_RegInfo* regInfo;				// 規制情報
	UINT16 regVol;						// 規制情報数
//	RouteSetting* RouteSetting;			// 探索条件
} SC_RP_RouteMng;


#endif /* SMCORERP_H_ */
