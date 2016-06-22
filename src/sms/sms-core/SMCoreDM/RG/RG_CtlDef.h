/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef RG_CTLDEF_H_
#define RG_CTLDEF_H_

#define DEVCNT_MAX				3						// 逸脱回数
#define DEVARRIVAL_DIST			50						// 逸脱到着判定距離
#define TRACK_NEW				0						// 今回経路追跡情報インデックス
#define	TRACK_OLD				1						// 前回経路追跡情報インデックス
#define TRACK_MAX				2						// 経路追跡情報最大値
#define NEARCRS_MAX				2						// 交差点情報最大値

// [共通] 発話タイミング
#define COMVCE_TIMING_APP		2000					// 接近案内発話タイミング (m)
#define COMVCE_TIMING_FLW		10000					// 通常交差点 道なり発話タイミング (m)
#define COMVCE_TIMING_ARV		30						// 到着発話タイミング (m)
// [共通] 発話タイミング判定
#define COMVCE_TIMING_APP_CHK(dist)		(COMVCE_TIMING_APP >= dist)
#define COMVCE_TIMING_FLW_CHK(dist)		(COMVCE_TIMING_FLW < dist)
#define COMVCE_TIMING_ARV_CHK(dist)		(COMVCE_TIMING_ARV > dist)

// [国内] 発話タイミング
#define	GENVCE_TIMING_JP_FST	300						// (JP) 通常交差点 第１発話タイミング (300m)
#define	GENVCE_TIMING_JP_SCD	100						// (JP) 通常交差点 第２発話タイミング (100m)
#define GENVCE_TIMING_JP_LST	75						// (JP) 通常交差点 第３発話タイミング ( 75m)
#define	HWYVCE_TIMING_JP_FST	1000					// (JP) 高速分岐点 第１発話タイミング (1000m)
#define	HWYVCE_TIMING_JP_SCD	500						// (JP) 高速分岐点 第２発話タイミング (500m)
#define HWYVCE_TIMING_JP_LST	450						// (JP) 高速分岐点 第３発話タイミング (450m)
// [国内] 発話タイミング判定
#define GENVCE_TIMING_FST_JP_CHK(dist)	(((GENVCE_TIMING_JP_FST + 25) >= dist)&&((GENVCE_TIMING_JP_FST - 25) <= dist))
#define GENVCE_TIMING_SCD_JP_CHK(dist)	(((GENVCE_TIMING_JP_SCD + 25) >= dist)&&((GENVCE_TIMING_JP_SCD - 25) <= dist))
#define GENVCE_TIMING_LST_JP_CHK(dist)	(GENVCE_TIMING_JP_LST > dist)
#define HWYVCE_TIMING_FST_JP_CHK(dist)	(((HWYVCE_TIMING_JP_FST + 50) >= dist)&&((HWYVCE_TIMING_JP_FST - 50) <= dist))
#define HWYVCE_TIMING_SCD_JP_CHK(dist)	(((HWYVCE_TIMING_JP_SCD + 50) >= dist)&&((HWYVCE_TIMING_JP_SCD - 50) <= dist))
#define HWYVCE_TIMING_LST_JP_CHK(dist)	(HWYVCE_TIMING_JP_LST > dist)

// [海外] 発話タイミング
#define	GENVCE_TIMING_EN_FST	400						// (EN) 通常交差点 第１発話タイミング ( 1/4mile)
#define	GENVCE_TIMING_EN_SCD	100						// (EN) 通常交差点 第２発話タイミング (1/10mile)
#define GENVCE_TIMING_EN_LST	75						// (EN) 通常交差点 第３発話タイミング (mile)
// [海外] 発話タイミング判定
#define GENVCE_TIMING_FST_EN_CHK(dist)	(((GENVCE_TIMING_EN_FST + 25) >= dist)&&((GENVCE_TIMING_EN_FST - 25) <= dist))
#define GENVCE_TIMING_SCD_EN_CHK(dist)	(((GENVCE_TIMING_EN_SCD + 25) >= dist)&&((GENVCE_TIMING_EN_SCD - 25) <= dist))
#define GENVCE_TIMING_LST_EN_CHK(dist)	(GENVCE_TIMING_EN_LST > dist)

typedef enum {
	e_RG_VCESTS_INVALID = 0,							// 初期状態
	e_RG_VCESTS_APP,									// 接近案内発話状態
	e_RG_VCESTS_FLW,									// 暫く道なり発話状態
	e_RG_VCESTS_FST,									// 第１発話状態
	e_RG_VCESTS_SCD,									// 第２発話状態
	e_RG_VCESTS_LST,									// 第３発話状態
	e_RG_VCESTS_ARV,									// 到着発話状態
} E_RG_VCESTS;

typedef struct {
	E_RG_VCESTS					status;					// 発話種別
	RT_NAME_t					tts;					// ＴＴＳ文字列
} RG_CTL_VOICE_t;

// 自車最寄リンク情報テーブル
typedef struct {
	UINT16						sect_no;				// 経路区間番号
	UINT16						route_link_no;			// 経路リンク番号
	UINT16						guide_link_no;			// リンクオフセット
	UINT16						point_no;				// 形状点オフセット
	UINT32						draw_len;				// リンク始点からの塗りつぶし長
} RG_CTL_NEARLINK_t;

// 誘導交差点情報テーブル
typedef struct {
	UINT16						crs_no;					// 交差点番号
	UINT16						crs_type;				// 交差点種別
	UINT8						ra_exit_no:4;			// ラウンドアバウト出口番号
	UINT8						tl_f:4;					// 信号機フラグ
	UINT8						turn_dir;				// 案内方向
	UINT32						remain_dist;			// 交差点までの残距離
	RT_NAME_t					crs_name;				// 交差点名称
	RT_POSITION_t				crs_point;				// 交差点座標
} RG_CTL_NEARCRS_t;

// 経路追跡情報テーブル
typedef struct {
	INT32						car_dir;				// 自車方位
	RT_POSITION_t				car_posi;				// 自車座標
	UINT32						remain_dist;			// 目的地までの残距離
	UINT32						remain_time;			// 目的地までの残旅行時間
	UINT32						passed_dist;			// 出発地からの移動距離
	UINT32						track_dist;				// 追跡距離
	RG_CTL_NEARLINK_t			link;					// 最寄リンク情報
	RG_CTL_NEARCRS_t			crs[NEARCRS_MAX];		// 最寄交差点情報
	RG_CTL_VOICE_t				voice;					// 音声発話情報
	UINT16						crs_vol;				// 最寄交差点情報数
} RG_CTL_TRACK_t;

// 誘導制御テーブル
typedef struct {
	RG_CTL_TRACK_t				track[TRACK_MAX];		// 交差点追跡情報 (0:前回/1:今回)
	Bool						simulate_f;				// シミュレーションフラグ
	Bool						arrival_f;				// 目的地到着フラグ
	UINT32						deviation_cnt;			// 逸脱回数
	UINT32						passed_dist;			// 最終順走時における出発地からの移動距離
} RG_CTL_MAIN_t;

void 				RG_RTMsg_Send(E_SC_MSG_ID);
E_SC_RESULT 		RG_CTL_GuideMain();
E_SC_RESULT 		RG_CTL_InitCtlTbl();
RG_CTL_MAIN_t*		RG_CTL_GetCtlTbl();
E_SC_RESULT 		RG_CTL_SimReady();
E_SC_RESULT 		RG_CTL_RunSimulation();
E_SC_RESULT 		RG_CTL_ExitSimulation();
E_SC_RESULT 		RG_CTL_SH_InitShareData();
E_SC_RESULT			RG_CTL_SH_SetDeviationInfo(RG_CTL_MAIN_t *);
E_SC_RESULT 		RG_CTL_SH_SetPassedInfo(RG_CTL_MAIN_t *);
E_SC_RESULT 		RG_CTL_SH_SetRealTimeInfo(RG_CTL_MAIN_t *);
E_SC_RESULT 		RG_CTL_SH_SetVoiceTTS(RG_CTL_MAIN_t *);
E_SC_RESULT 		RG_CTL_SH_SetTurnList(RT_LST_MAIN_t *);
E_SC_RESULT 		RG_CTL_SetDevCount(RG_CTL_MAIN_t *,  SMCARSTATE *);
E_SC_RESULT 		RG_CTL_GetNearCross(RT_TBL_MAIN_t *, RG_CTL_MAIN_t *);
E_SC_RESULT 		RG_CTL_GetNearLink(RT_TBL_MAIN_t *, RG_CTL_MAIN_t *);
E_SC_RESULT 		RG_CTL_SetGuideVoice(RT_TBL_MAIN_t *, RG_CTL_MAIN_t *);
E_SC_RESULT 		RG_CTL_SH_SetDynamicGraphicBitmap(RG_CTL_MAIN_t *);
E_SC_RESULT 		RG_CTL_SH_SetDynamicGraphicStat(RG_CTL_MAIN_t *);
E_SC_RESULT 		RG_CTL_GetDynamicGraphicBitmap(SMBITMAPINFO	*bitmapinfo);

E_SC_RESULT			RG_CTL_JP_SetGuideVoice_Normal(RG_CTL_MAIN_t *);
E_SC_RESULT			RG_CTL_JP_SetGuideVoice_HwyIn(RG_CTL_MAIN_t *);
E_SC_RESULT			RG_CTL_JP_SetGuideVoice_HwyOut(RG_CTL_MAIN_t *);
E_SC_RESULT			RG_CTL_JP_SetGuideVoice_HwyJct(RG_CTL_MAIN_t *);
E_SC_RESULT			RG_CTL_JP_SetGuideVoice_Waypt(RG_CTL_MAIN_t *);
E_SC_RESULT			RG_CTL_JP_SetGuideVoice_Dest(RG_CTL_MAIN_t *);
E_SC_RESULT			RG_CTL_JP_SetGuideVoice_Split(RG_CTL_MAIN_t *);
E_SC_RESULT			RG_CTL_JP_SetGuideVoice_Complex(RG_CTL_MAIN_t *);
E_SC_RESULT			RG_CTL_JP_SetGuideVoice_Start(RG_CTL_MAIN_t *);
E_SC_RESULT			RG_CTL_JP_SetGuideVoice_Ra(RG_CTL_MAIN_t *);

E_SC_RESULT			RG_CTL_EN_SetGuideVoice_Normal(RG_CTL_MAIN_t *);
E_SC_RESULT			RG_CTL_EN_SetGuideVoice_HwyIn(RG_CTL_MAIN_t *);
E_SC_RESULT			RG_CTL_EN_SetGuideVoice_HwyOut(RG_CTL_MAIN_t *);
E_SC_RESULT			RG_CTL_EN_SetGuideVoice_HwyJct(RG_CTL_MAIN_t *);
E_SC_RESULT			RG_CTL_EN_SetGuideVoice_Waypt(RG_CTL_MAIN_t *);
E_SC_RESULT			RG_CTL_EN_SetGuideVoice_Dest(RG_CTL_MAIN_t *);
E_SC_RESULT			RG_CTL_EN_SetGuideVoice_Split(RG_CTL_MAIN_t *);
E_SC_RESULT			RG_CTL_EN_SetGuideVoice_Complex(RG_CTL_MAIN_t *);
E_SC_RESULT			RG_CTL_EN_SetGuideVoice_Start(RG_CTL_MAIN_t *);
E_SC_RESULT			RG_CTL_EN_SetGuideVoice_Ra(RG_CTL_MAIN_t *);

#endif /* RG_CTLDEF_H_ */
