/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SC_PM_DATA_H
#define SC_PM_DATA_H

//------------------------------------------------------------------------------------------------------------------------
//
//	SMCorePMDataヘッダ
//
//------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------
//インクルード
//---------------------------------------------------------------------------------

extern Bool isTimerStarted;

//---------------------------------------------------------------------------------
// 定数定義
//---------------------------------------------------------------------------------
#define	SC_PM_PROBE_MAX_PATH				260			/* ファイルパス文字列最大サイズ					*/

#define	SC_PM_PROBE_DATA_FILE_NAME			"SC_PM_ProbeData.bin"		/* プローブデータ蓄積ファイル名文字列				*/							// 2014.04.18

#define	SC_PM_PROBE_INFO_FILE_NAME_SZ		256			/* プローブ情報アップロードデータファイル名変数サイズ	*/

#define	SC_PM_PROBE_INFO_CUNT_MAX			60			/* プローブ情報アップロード基準プローブデータ数	*/
#define SC_PM_PROBE_INFO_UPLOAD_CUNT_MAX	99999		/* プローブ情報数カウンタ最大数					*/

#define	SC_PM_PROBE_HEAD_SIZE_DATA_SZ		4			/* プローブヘッダ データサイズエリアサイズ		*/
#define	SC_PM_PROBE_HEAD_SIZE_DATA_ID		2			/* プローブヘッダ データIDエリアサイズ			*/
#define	SC_PM_PROBE_HEAD_SIZE_SEQUENCE		7			/* プローブヘッダ シーケンス番号エリアサイズ	*/

#define	SC_PM_PROBE_HEAD_SIZE_DATA_VER		16			/* プローブ情報バイナリデータバージョン エリアサイズ	*/	// 可変長だが、ひとまず少し大きめに固定長とする
#define	SC_PM_PROBE_HEAD_SIZE_ANDROID_VER	32			/* Androidアプリケーションバージョン エリアサイズ		*/	// 可変長だが、ひとまず少し大きめに固定長とする
#define	SC_PM_PROBE_HEAD_SIZE_MAP_BUILD_NO	132			/* 地図ビルド番号										*/	// 可変長だが、ひとまず少し大きめに固定長とする
#define	SC_PM_PROBE_HEAD_SIZE_USER_ID		64			/* ユーザID エリアサイズ								*/	// 可変長だが、ひとまず少し大きめに固定長とする
#define	SC_PM_PROBE_HEAD_SIZE_DEVICE_ID		16			/* デバイスID エリアサイズ								*/	// 可変長だが、ひとまず少し大きめに固定長とする
#define	SC_PM_PROBE_HEAD_SIZE_SEQUENCE_NUM	16			/* シーケンス番号 エリアサイズ							*/	// 可変長だが、ひとまず少し大きめに固定長とする

#define	SC_PM_PROBE_STR_SIZE_INDE_CHAR		1			/* 改行文字サイズ	*/
#define	SC_PM_PROBE_STR_SIZE_END_CHAR		1			/* 終端文字サイズ	*/

#define	SC_PM_PROBE_HEAD_PROBE_ID			0x01		/* プローブID文字	*/
#define	SC_PM_PROBE_INFO_END_CHAR			0x00		/* 終端文字	*/

#define	SC_PM_PROBE_INFO_FILE_EXTENSION		""			/* プローブ情報アップロードデータファイル名拡張子	*/
#define	SC_PM_PROBE_INFO_FILE_CONNECT		"_"			/* プローブ情報アップロードデータファイル名連結文字	*/

#define	SC_PM_PROBE_INFO_DATA_ID		"PD"			/* プローブファイル 先頭識別子( プローブ情報バイナリ構造_00-00-R10 より追加 ) */
#define	SC_PM_PROBE_INFO_DATA_ID_SZ		2				/* プローブファイル 先頭識別子のサイズ = 2バイト */

// プローブ情報バイナリ構造_00-00-R10 より、プローブ情報バイナリデータバージョンの運用方法を取り決めたため、それに従う。
#define	SC_PM_PROBE_INFO_DATA_VERSION		"PD000-003"

#define SC_PM_PROBE_UPLOAD_TIMER			60
#define	SC_PM_USERINFO_FILE_NAME			"SMCC_UserInfo.txt"					// ユーザ情報ファイル名文字列											// 2014.04.18

#define SC_PM_POSINF_LOGINID_STR_SIZE		(128+1)		/* ログインID文字列サイズ		*/
#define SC_PM_POSINF_DATA_UNIT_SIZE			14			/* 位置情報共有データサイズ (login_idを除く)	*/
#define	SC_PM_POSINF_DATA_TIME_SIZE			6			/* 位置情報共有データ時刻部分サイズ	*/
#define	SC_PM_POSINF_DATA_POSI_SIZE			8			/* 位置情報共有データ位置情報部分サイズ	*/
#define	SC_PM_POSINF_FILE_EXTENSION			"Pos.csv"	/* 位置情報共有データファイル名固定部文字列	*/

//---------------------------------------------------------------------------------
// 構造体定義
//---------------------------------------------------------------------------------
/**
* @brief プローブヘッダ
*/
typedef struct _T_SC_PM_PROBE_INFO_HEAD{
	Char	data_ver[SC_PM_PROBE_HEAD_SIZE_DATA_VER];			/* データバージョン						*/	//ひとまず固定長にする
	UINT32	data_size;											/* データサイズ							*/
	UINT16	deta_id;											/* データID								*/
	Char	user_id[SC_PM_PROBE_HEAD_SIZE_USER_ID];				/* ユーザID								*/	//ひとまず固定長にする (ひとまず値は、ユーザ登録時のguidの値)
	Char	device_id[SC_PM_PROBE_HEAD_SIZE_DEVICE_ID];			/* デバイスID							*/	//ひとまず固定長にする (デバイスID取得時のdev_id)
	Char	sequence_num[SC_PM_PROBE_HEAD_SIZE_SEQUENCE_NUM];	/* シーケンス番号						*/	//ひとまず固定長にする
} T_SC_PM_PROBE_INFO_HEAD;

/**
* @brief 位置情報
*/
typedef struct _T_SC_PM_PROBE_INFO_DATA_POS{
	INT32	latitude;									/* 緯度				*/
	INT32	longitude;									/* 経度				*/
} T_SC_PM_PROBE_INFO_DATA_POS;

/**
* @brief プローブデータ
*/
typedef struct _T_SC_PM_PROBE_INFO_DATA{
	Bool	mapMatchingFlg;								/* マップマッチングフラグ	*/
	Bool	linkDirectionFlg;							/* 方向フラグ				*/
	Bool	accelerometerFlg;							/* センサ加速度有効フラグ	*/
	Bool	gyroscopeFlg;								/* ジャイロ有効フラグ		*/
	Bool	magneticFieldFlg;							/* 磁気有効フラグ			*/
	Bool	orientationFlg;								/* 方位角有効フラグ 		*/
	Bool	puressureFlg;								/* 気圧有効フラグ			*/
	Bool	lightFlg;									/* 照度有効フラグ			*/
	Bool	gpsFlg;										/* GPS情報有効フラグ		*/
	Bool	dummy1[3];									/* ぱでぃんぐ				*/

	UINT8	year;										/* 年						*/
	UINT8	month;										/* 月						*/
	UINT8	day;										/* 日						*/
	UINT8	hour;										/* 時						*/
	UINT8	minute;										/* 分						*/
	UINT8	second;										/* 秒						*/
	UINT16	dummy2;										/* ぱでぃんぐ				*/
	INT32	gps_latitude;								/* GPS位置情報				*/
	INT32	gps_longitude;								/* GPS位置情報				*/
	INT32	map_latitude;								/* マップマッチング位置情報	*/
	INT32	map_longitude;								/* マップマッチング位置情報	*/
	INT16	direction;									/* 方向						*/
	UINT16	dummy3;										/* ぱでぃんぐ				*/
	UINT32	speed;										/* 速度						*/
	UINT8	save_flg;									/* 保存フラグ				*/
	UINT8	transfar_type;								/* 車両タイプ				*/
	UINT8	echo_flag;									/* 位置情報共有フラグ		*/
	UINT8	guideStatus;								/* 経路誘導利用ステータス	*/
	INT32	accelerometer[3];							/* センサ加速度				*/
	INT32	gyroscope[3];								/* ジャイロスコープ			*/
	INT32	puressure;									/* 圧力センサ(気圧)			*/
	INT32	light;										/* 照度センサ				*/
	INT32	gpsAltitude;								/* GPS高度					*/
	INT16	gpsAngle;									/* GPS方位					*/
	INT16	gpsSpeed;									/* GPS速度					*/
	INT32	gpsAccuracy;								/* GPS精度					*/
	UINT32	parcelId;									/* マップマッチングしたリンクのパーセルID */
	ULONG	linkId;										/* マップマッチングしたリンクのリンクID	*/
	INT32	magneticField[3];							/* 磁気						*/
	INT32	orientation[3];								/* 方位角					*/
} T_SC_PM_PROBE_INFO_DATA;

/**
* @brief プローブ情報
*/
typedef struct _T_SC_PM_PROBE_INFO{
//	T_SC_PM_PROBE_INFO_HEAD		head;					/* プローブヘッダ		*/
	T_SC_PM_PROBE_INFO_DATA		data;					/* プローブデータ		*/
//	T_SC_PM_PROBE_PHDY_DATA		phdy;					/* PHDYデータ			*/
} T_SC_PM_PROBE_INFO;

/**
* @brief 位置情報共有機能用データ
*/
typedef struct _T_SC_PM_SHARE_PPOSITION_INFO{
	Char	login_id[SC_PM_POSINF_LOGINID_STR_SIZE];	/* login_id					*/
	UINT8	year;										/* 年						*/
	UINT8	month;										/* 月						*/
	UINT8	day;										/* 日						*/
	UINT8	hour;										/* 時						*/
	UINT8	minute;										/* 分						*/
	UINT8	second;										/* 秒						*/
	UINT16	dummy;										/* ぱでぃんぐ				*/
	INT32	map_latitude;								/* マップマッチング位置情報	*/
	INT32	map_longitude;								/* マップマッチング位置情報	*/
} T_SC_PM_SHARE_PPOSITION_INFO;


#if 1		//*************************************************************************************   デバッグ用処理 (SMCore.cppから直接コールするため)
#ifdef __cplusplus
extern "C" {
#endif
#endif		//*************************************************************************************   デバッグ用処理 (SMCore.cppから直接コールするため)


//---------------------------------------------------------------------------------
// 外部I/F定義
//---------------------------------------------------------------------------------
E_SC_RESULT SC_PM_ProbeInfo_Initial();									// プローブ情報初期化
E_SC_RESULT SC_PM_CreateProbeData(const pthread_msq_msg_t *msg);		// プローブ情報作成・送信依頼処理
void SC_PM_NoticeUploadFinish(const pthread_msq_msg_t *msg);

#if 1		//*************************************************************************************   デバッグ用処理 (SMCore.cppから直接コールするため)
#ifdef __cplusplus
}
#endif
#endif		//*************************************************************************************   デバッグ用処理 (SMCore.cppから直接コールするため)


#endif // #ifndef SC_PM_PROBE_DATA_H
