/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef _SMPAL_H_
#define _SMPAL_H_

// デフォルトユーザID
#define	SMPAL_DEFAULT_USER_ID			(char*)"sms_free_user_id"

#define SM_GC_POI_POINT_TBL_MAX_COUNT 50		// 地点りテーブル最大収納数
#define SM_GC_POI_HISTORY_TBL_MAX_COUNT 50		// 履歴テーブル最大収納数
#define SM_GC_POI_FAVORITE_TBL_MAX_COUNT 50		// お気に入りテーブル最大収納数

/**
 * 以降C++のみ
 */
#ifdef __cplusplus
//-----------------------------------
// 構造体定義
//-----------------------------------

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// 登録地テーブル
typedef struct {
	INT32 data_type;			//  １．GEM/GEMSPOT/地点    0:GEM   1:GEMSPOT   2:地点   -1:無効
	std::string userid;			//  2.ユーザＩＤ
	std::string datetime;		//  3.年月日時分秒。書式：yyyyMMddHHmmss
	std::string gemid;			//  4.ＧＥＭＩＤ
	std::string gemspotid;		//  5.サーバで管理しているＧＥＭＳＰＯＴＩＤ
	std::string pos_name;		//  6.登録地名
	INT32 lat;					//  7.緯度。単位：1/1024秒
	INT32 log;					//  8.経度。単位：1/1024秒
	INT32 len;					//  9.中心位置からの距離・閾値（ｍ）
} SM_GC_POI_POINT_TBL;
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// 履歴テーブル
typedef struct {
	std::string userid;			//  1.当レコードを登録した時のログＩＮユーザＩＤ
	std::string datetime;		//  2.年月日時分秒。書式：yyyyMMddHHmmss
	std::string gemid;			//  3.当レコードを登録した時のＧＥＭＩＤ
	std::string gemspotid;		//  4.サーバで管理しているＧＥＭＳＰＯＴＩＤ
	INT32 lat;					//  5.緯度。単位：1/1024秒
	INT32 log;					//  6.経度。単位：1/1024秒
} SM_GC_POI_HISTORY_TBL;
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//	お気に入りテーブル
typedef struct {
	std::string userid;			//  1.当レコードを登録した時のログＩＮユーザＩＤ
	std::string dbtime;			//  2.ＤＢ年月日時分秒。      書式：yyyyMMddHHmmss
	std::string contentstime;	//  3.コンテンツ年月日時分秒。書式：yyyyMMddHHmmss
	std::string ctgry_code;		//  4.カテゴリコード。ＴＲ４では、”１”（＝”ＧＥＭ”）固定
	std::string id;				//  5.ＩＤ
	std::string url;			//  6.ＵＲＬ
	std::string name;			//  7.氏名
	std::string pos_name;		//  8.場所
	std::string contents;		//  9.内容
	UINT8* binary_data;			//  10.バイナリデータ(BMP/PNG/JPG)
	INT32 binary_data_len;		//  11.バイナリデータ(BMP/PNG/JPG)の長さ
	INT32 lat;					//  12.緯度。単位：1/1024秒
	INT32 log;					//  13.経度。単位：1/1024秒
	INT32 len;					//  14.中心位置からの距離。入力時は参照しない。。単位：ｍ
} SM_GC_POI_FAVORITE_TBL;

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// 検索条件（地点・お気に入り共通）
typedef struct {
	INT32 data_type;			//  1.データの分類    0:GEM   1:GEMSPOT   2:地点   3:お気に入り  -1:無効
	std::string high_datetime;	//  2.年月日時分秒（Ｍａｘ）。書式：yyyyMMddHHmmss
	std::string low_datetime;	//  3.年月日時分秒（Ｍｉｎ）。 書式：yyyyMMddHHmmss
	std::string userid;			//  4.ユーザＩＤ
	INT32 c_lat;				//  5.緯度（中心）。単位：1/1024秒
	INT32 c_log;				//  6.経度（中心）。単位：1/1024秒
	INT32 len;					//  7.中心位置からの距離・閾値（ｍ）
} SC_POI_SEARCH_COND_TBL;

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// 検索条件（履歴）
typedef struct {
	std::string userid;			//  1.ユーザＩＤ
} SC_POI_SEARCH_COND_2_TBL;
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//	qsortワークエリア
typedef struct {
	INT32 len;					// 中心位置からの距離。。単位：ｍ
	INT32 id;					// 識別子
} SM_GC_POI_QSORT_WORK_TBL;

#endif // __cplusplus

#endif
