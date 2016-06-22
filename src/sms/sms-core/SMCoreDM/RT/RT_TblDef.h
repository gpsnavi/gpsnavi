/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef RT_TBLDEF_H_
#define RT_TBLDEF_H_

#define		PUBLIC_INI			-1						// 公開テーブル番号初期値
#define		PUBLIC_NO1			0						// 公開テーブル番号１
#define		PUBLIC_NO2			1						// 公開テーブル番号２
#define		PUBLIC_MAX			2						// 公開テーブル数
#define		MAKETBL_DIV			20000					// 誘導テーブル作成距離：10km TODO
#define		MAKETBL_ALL			0xFFFFFFFF				// 誘導テーブル作成距離：無制限
#define		CRSREV_DIST			10						// 交差点補正対象距離
#define		CRSVOL_MAX			1000					// 初回誘導交差点情報数
#define		SHARPVOL_MAX		100000					// 初回リンク形状点情報数

#define		CRSTYPE_NORMAL		1						// 一般道交差点
#define		CRSTYPE_HWYIN		2						// 高速入口
#define		CRSTYPE_HWYOT		3						// 高速出口
#define		CRSTYPE_HWYJCT		4						// 高速分岐
#define		CRSTYPE_WAYPT		5						// 経由地
#define		CRSTYPE_DEST		6						// 目的地
#define		CRSTYPE_SPLIT		7						// 経路断裂点
#define		CRSTYPE_NOTCRS		8						// 非誘導交差点
#define		CRSTYPE_RA			9						// ＲＡ

#define		TURN_UT				1						// Ｕターン
#define		TURN_BR				2						// 右後
#define		TURN_R				3						// 右
#define		TURN_FR				4						// 右前
#define		TURN_ST				5						// 直進
#define		TURN_FL				6						// 左前
#define		TURN_L				7						// 左
#define		TURN_BL				8						// 左後
#define		TURN_RR				10						// 相対右
#define		TURN_RL				11						// 相対左
#define		TURN_RA				12						// ＲＡ
#define		TURN_EX				99						// 例外

#define		STYPE_START			1						// 出発地
#define		STYPE_WAYPT			2						// 経由地
#define		STYPE_DEST			3						// 目的地
#define		STYPE_SPLIT			4						// 経路断裂点
#define		STYPE_GDCRS			5						// 誘導交差点


// 角度番号(360度を16分割した番号)取得
#define 	RT_GET_ANGLE_NO(a)			(a / 23)

// 角度→案内方向変換テーブル
extern UINT16	RT_TURNDIR_TBL[16];

// 交差点情報テーブル
typedef struct {
	UINT16						crs_no;					// 交差点番号
	UINT16						crs_type;				// 交差点種別
	UINT8						ra_exit_no:4;			// ＲＡ出口番号
	UINT8						tl_f:4;					// 信号機フラグ
	UINT8						dir;					// 案内方向
	UINT16						reserve;				// アライメント調整
	RT_NAME_t					crs_name;				// 交差点名称
} RT_TBL_GUIDECRS_t;

// リンク情報テーブル
typedef struct {
	UINT32						parcel_id;				// パーセルＩＤ
	UINT32						link_id;				// リンクＩＤ
	UINT32						dist;					// リンク長
	UINT32						time;					// 旅行時間
	UINT32						sharp_offset;			// リンク形状点情報オフセット
	UINT16						crs_offset;				// 誘導交差点情報オフセット
	UINT16						sharp_vol;				// リンク形状点情報数
} RT_TBL_GUIDELINK_t;

// 経路リンク情報テーブル
typedef struct {
	UINT32						parcel_id;				// パーセルＩＤ
	UINT32						link_id;				// リンクＩＤ
	UINT32						dist;					// 実距離
	UINT32						total_dist;				// ここまでの実距離
	UINT32						time;					// 旅行時間
	UINT32						total_time;				// ここまでの旅行時間
	UINT32						link_offset;			// リンク情報オフセット
	UINT16						link_vol;				// リンク情報数
	UINT16						route_point_no;			// 経路地点情報オフセット
	UINT8						road_class;				// 道路種別
	UINT8						link_type;				// リンク種別
	UINT8						link_dir;				// 経路進行方向に対する地図データリンク方向
	UINT8						level:3;				// レベル
	UINT8						split_f:1;				// 経路断裂フラグ
	UINT8						term_f:2;				// 経路端点フラグ
	UINT8 						reserve:2;				// リザーブ
} RT_TBL_ROUTELINK_t;

// 地点情報テーブル
typedef struct {
	RT_POSITION_t				posi;					// 地点座標情報
	RT_LINKPOINT_t				s_point;				// 区間始点リンク形状点座標
	RT_LINKPOINT_t				e_point;				// 区間終点リンク形状点座標
	UINT32						sect_no;				// 区間番号
	UINT32						sect_dist;				// 区間距離
	UINT32						sect_time;				// 区間所要時間
} RT_TBL_ROUTEPOINT_t;

// 誘導情報テーブル
typedef struct {
	RT_ROUTEID_t				rt_id;					// 経路ＩＤ情報
	RT_TBL_ROUTEPOINT_t			*route_point_p;			// 経路地点情報
	RT_TBL_ROUTELINK_t			*route_link_p;			// 経路リンク情報
	RT_TBL_GUIDELINK_t			*guide_link_p;			// 誘導リンク情報
	RT_TBL_GUIDECRS_t			*guide_crs_p;			// 誘導交差点情報
	RT_XY_t						*guide_sharp_p;			// 誘導リンク形状点情報
	UINT32						guide_sharp_vol;		// 誘導リンク形状点情報数
	UINT16						route_point_vol;		// 経路地点情報数
	UINT16						route_link_vol;			// 経路リンク情報数
	UINT16						guide_link_vol;			// 誘導リンク情報数
	UINT16						guide_crs_vol;			// 誘導交差点情報数
	UINT16						guide_crs_real_vol;		// 誘導交差点情報数（非誘導抜き）
	UINT16						make_no;				// 作成済み経路リンク
} RT_TBL_MAIN_t;

// 誘導情報世代管理テーブル
typedef struct {
	RT_TBL_MAIN_t				guide_tbl[PUBLIC_MAX];
	INT16						public_no;
} RT_TBL_MGR_t;

E_SC_RESULT 	RT_TBL_Init();
E_SC_RESULT 	RT_TBL_MakeGuideTbl(UINT32);
E_SC_RESULT 	RT_TBL_AddGuideTbl(UINT32);
E_SC_RESULT		RT_TBL_FreeGuideTbl();
E_SC_RESULT 	RT_TBL_SetRouteLink(RT_TBL_MAIN_t *, SC_RP_RouteMng *);
E_SC_RESULT 	RT_TBL_SetGuideLink(RT_TBL_MAIN_t *, UINT32);
RT_TBL_MAIN_t 	*RT_TBL_LockGuideTbl();
E_SC_RESULT		RT_TBL_UnLockGuideTbl();
INT32 			RT_TBL_GetSituationType(UINT16);
UINT8 			RG_TBL_GetRelativeGuideDir(RT_TBL_GUIDECRS_t *);

#endif /* RT_TBLDEF_H_ */

