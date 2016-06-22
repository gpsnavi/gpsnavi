/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef RT_LSTDEF_H_
#define RT_LSTDEF_H_

#define 	LIST_ICON_START			1				// 出発地
#define 	LIST_ICON_WAYPT			2				// 経由地
#define 	LIST_ICON_DEST			3				// 目的地
#define		LIST_ICON_TURNL			4				// 左
#define		LIST_ICON_TURNFL		5				// 左前
#define		LIST_ICON_TURNBL		6				// 左後
#define		LIST_ICON_TURNR			7				// 右
#define		LIST_ICON_TURNFR		8				// 右前
#define		LIST_ICON_TURNBR		9				// 右後
#define		LIST_ICON_TURNUT		10				// Ｕターン
#define		LIST_ICON_HWOUT			11				// 高速出口
#define		LIST_ICON_HWIN			12				// 高速入口
#define		LIST_ICON_CHWOUT		13				// 都市高速出口
#define		LIST_ICON_CHWIN			14				// 都市高速入口
#define		LIST_ICON_RA			18				// ＲＡ
#define		LIST_ICON_JCT			20				// 分岐
#define		LIST_ICON_TURNST		22				// 直進

// ターンリスト情報
typedef struct {
	RT_ROUTEID_t			rt_id;					// 経路ID情報
	UINT32					turn_vol;				// 交差点情報数
	SMTURNINFO				turn_info[1000];		// 交差点情報
} RT_LST_MAIN_t;

E_SC_RESULT 	RT_LST_Init();
E_SC_RESULT 	RT_LST_MakeTurnList();
RT_LST_MAIN_t 	*RT_LST_GetTurnList();
E_SC_RESULT		RT_LST_SetTurnList(RT_TBL_MAIN_t *, RT_LST_MAIN_t *);
#endif /* RT_LSTDEF_H_ */
