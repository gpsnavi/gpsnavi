/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SC_SD_CMN_H
#define SC_SD_CMN_H

#define SC_SD_PROBE_DIR						"Probe/"
#define SC_SD_DB_FILE_NAME					"SensorData.db"

#define SC_SD_TRIP_ID_SIZE					16

#define CC_SD_UPLOAD_TIMER					60		// アップロードタイマ
#define CC_SD_UPLOAD_NO_STOP				0		// アップロード停止しない
#define CC_SD_UPLOAD_STOP					1		// アップロード停止する
#define CC_SD_UPLOAD_ERROR_MAXNUM			5		// 許容するアップロードエラー最大回数

#define	CC_SD_CLEAR_UPLOADERRORNUM()		((uploadErrorNum) = 0)	// アップロードエラー回数クリア
#define	CC_SD_GET_UPLOADERRORNUM()			(uploadErrorNum)		// アップロードエラー回数取得
#define	CC_SD_COUTUP_UPLOADERRORNUM()		((uploadErrorNum)++)	// アップロードエラー回数カウントアップ

typedef enum _SMSDSTATUS {
	SMSDSTATUS_STOP = 0,		// 診断停止中
	SMSDSTATUS_START			// 診断中
} SMSDSTATUS;

typedef struct _SMPHYDDATA {
	INT32	id;					// ID(ロケータから計測データ取得時は設定しない)
	Char	time[20];			// 計測時刻(UTC)
	DOUBLE	latNetwork;			// 緯度(ネットワーク)
	DOUBLE	lonNetwork;			// 経度(ネットワーク)
	DOUBLE	latGPS;				// 緯度(GPS)
	DOUBLE	lonGPS;				// 経度(GPS)
	DOUBLE	acc[3];				// 加速度センサの値(X軸、Y軸、Z軸)[m/s^2]
	DOUBLE	orientation[3];		// 姿勢(azimuth(Z軸)、pitch(X軸)、roll(Y軸))[deg]
	DOUBLE	magneticField[3];	// 磁気センサの値(X軸、Y軸、Z軸)[uT]
	DOUBLE	gyroscope[3];		// ジャイロの値(X軸、Y軸、Z軸)[rad/sec]
	DOUBLE	gravity[3];			// 重力センサの値(X軸、Y軸、Z軸)[m/s^2]
} SMPHYDDATA;

extern UINT32	uploadErrorNum;

#endif // #ifndef SC_SD_CMN_H
