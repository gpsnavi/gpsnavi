/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#include "sms-core/SMCoreSD/SMCoreSDInternal.h"

#define SC_SDU_STRING_BUFF_SIZE			(1024)

#define SC_SDU_JSON_KEY_FILE_VERSION	"\"fileVersion\""
#define SC_SDU_JSON_KEY_GUID			"\"userGUID\""
#define SC_SDU_JSON_KEY_DEVICE_ID		"\"deviceID\""
#define SC_SDU_JSON_KEY_TRIP_ID			"\"tripID\""
#define SC_SDU_JSON_KEY_TERMINAL		"\"terminal\""
#define SC_SDU_JSON_KEY_DRIVE_DATA		"\"driveData\""
#define SC_SDU_JSON_KEY_DATA_TIME		"\"dataTime\""
#define SC_SDU_JSON_KEY_GPS_LAT			"\"gpsPosLat\""
#define SC_SDU_JSON_KEY_GPS_LON			"\"gpsPosLng\""
#define SC_SDU_JSON_KEY_NET_LAT			"\"netPosLat\""
#define SC_SDU_JSON_KEY_NET_LON			"\"netPosLng\""
#define SC_SDU_JSON_KEY_SENSOR_DATA		"\"senserData\""
#define SC_SDU_JSON_KEY_MS				"\"sensedMs\""
#define SC_SDU_JSON_KEY_ROW_ACC_X		"\"rawAccX\""
#define SC_SDU_JSON_KEY_ROW_ACC_Y		"\"rawAccY\""
#define SC_SDU_JSON_KEY_ROW_ACC_Z		"\"rawAccZ\""
#define SC_SDU_JSON_KEY_COR_ACC_X		"\"corAccX\""
#define SC_SDU_JSON_KEY_COR_ACC_Y		"\"corAccY\""
#define SC_SDU_JSON_KEY_COR_ACC_Z		"\"corAccZ\""
#define SC_SDU_JSON_KEY_ORT_X			"\"orientationX\""
#define SC_SDU_JSON_KEY_ORT_Y			"\"orientationY\""
#define SC_SDU_JSON_KEY_ORT_Z			"\"orientationZ\""

#define SC_SDU_FILE_VERSION				"DD001-001"
#define SC_SDU_TERMINAL					"1"

E_SC_RESULT SC_SDU_CreateJsonFile(const SCC_AUTHINFO *authInfo,
								  const Char *startTime,
								  const SMPHYDDATA *data,
								  INT32 dataNum,
								  const Char *filePath,
								  Bool isTerminal)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	Char	*json = NULL;
	Char	*ptr = NULL;
	Char	tripId[SC_SD_TRIP_ID_SIZE] = {};
	INT32	i = 0;
	//INT32	j = 0;
	Bool	isTimeChenged = false;
	Char	time[20] = {};
	Char	timeMS[4] = {};
	FILE	*fp = NULL;

	SC_LOG_DebugPrint(SC_TAG_SDU, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == authInfo) {
			SC_LOG_ErrorPrint(SC_TAG_SDU, "param error[authInfo], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((NULL == startTime) || (EOS == *startTime)) {
			SC_LOG_ErrorPrint(SC_TAG_SDU, "param error[startTime], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == data) {
			SC_LOG_ErrorPrint(SC_TAG_SDU, "param error[data], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if ((NULL == filePath) || (EOS == *filePath)) {
			SC_LOG_ErrorPrint(SC_TAG_SDU, "param error[filePath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// メモリ確保
		json = (char*)SC_MEM_Alloc(SC_SDU_STRING_BUFF_SIZE, e_MEM_TYPE_SD);
		if (NULL == json) {
			SC_LOG_ErrorPrint(SC_TAG_SDU, "SC_MEM_Alloc error, " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		ptr = json;

		// JSONファイル出力処理
		// ディレクトリ作成
		ret = SC_MakeDir(filePath);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_SDU, "SC_MakeDir error, " HERE);
			break;
		}

		// ファイルオープン
		fp = fopen(filePath, "w");
		if (NULL == fp) {
			SC_LOG_ErrorPrint(SC_TAG_SDU, "fopen error errno=%d, " HERE, errno);
			ret = e_SC_RESULT_FILE_OPENERR;
			break;
		}

		// トリップID
		sprintf((char*)tripId, "%.8s-%.6s", startTime, &startTime[8]);

		// ■プローブヘッダ
		// 開始
		sprintf((char*)ptr, "{");
		ptr++;

		// ファイルバージョン、ユーザGUID、端末ID、トリップID
		sprintf((char*)ptr, "%s:\"%s\",%s:\"%s\",%s:\"%s\",%s:\"%s\"",
				SC_SDU_JSON_KEY_FILE_VERSION,
				SC_SDU_FILE_VERSION,
				SC_SDU_JSON_KEY_GUID,
				authInfo->guid,
				SC_SDU_JSON_KEY_DEVICE_ID,
				authInfo->term_id,
				SC_SDU_JSON_KEY_TRIP_ID,
				tripId);
		ptr += strlen((char*)ptr);

		// 終端フラグ
		if (isTerminal) {
			sprintf((char*)ptr, ",%s:%s", SC_SDU_JSON_KEY_TERMINAL, SC_SDU_TERMINAL);
			ptr += strlen(ptr);
		}

		if (0 < dataNum) {
			//運転特性データ
			sprintf((char*)ptr, ",%s:[", SC_SDU_JSON_KEY_DRIVE_DATA);
			ptr += strlen(ptr);
		}

		// ファイル出力
		fprintf(fp, "%s", json);
		ptr = json;

		for (i = 0; i < dataNum; i++) {
			if (0 < i) {
				if (0 != memcmp(data[i - 1].time, data[i].time, 14)) {
					isTimeChenged = true;
					// 秒単位データ配列(終了)
					sprintf((char*)ptr, "]},");
					ptr += strlen((char*)ptr);
				} else {
					isTimeChenged = false;
				}
			} else {
				isTimeChenged = true;
			}

			// ミリ秒
			sprintf((char*)timeMS, "%.3s", &data[i].time[14]);
			if (isTimeChenged) {
				// 秒単位データ配列(開始)
				sprintf((char*)ptr, "{");
				ptr++;

				sprintf((char*)time,
						"%.4s/%.2s/%.2s %.2s:%.2s:%.2s",
						data[i].time,
						&data[i].time[4],
						&data[i].time[6],
						&data[i].time[8],
						&data[i].time[10],
						&data[i].time[12]);

				// データ時刻
				sprintf((char*)ptr, "%s:\"%s\"", SC_SDU_JSON_KEY_DATA_TIME, time);
				ptr += strlen((char*)ptr);

				// GPS位置情報(緯度)
				if (DBL_MAX != data[i].latGPS) {
					sprintf((char*)ptr, ",%s:%f", SC_SDU_JSON_KEY_GPS_LAT, data[i].latGPS);
					ptr += strlen((char*)ptr);
				}

				// GPS位置情報(経度)
				if (DBL_MAX != data[i].lonGPS) {
					sprintf((char*)ptr, ",%s:%f", SC_SDU_JSON_KEY_GPS_LON, data[i].lonGPS);
					ptr += strlen((char*)ptr);
				}

				// ネット位置情報(緯度)
				if (DBL_MAX != data[i].latNetwork) {
					sprintf((char*)ptr, ",%s:%f", SC_SDU_JSON_KEY_NET_LAT, data[i].latNetwork);
					ptr += strlen((char*)ptr);
				}

				// ネット位置情報(経度)
				if (DBL_MAX != data[i].lonNetwork) {
					sprintf((char*)ptr, ",%s:%f", SC_SDU_JSON_KEY_NET_LON, data[i].lonNetwork);
					ptr += strlen((char*)ptr);
				}

				// 計測データ配列(開始)
				sprintf((char*)ptr, ",%s:[", SC_SDU_JSON_KEY_SENSOR_DATA);
				ptr += strlen(ptr);
			} else {
				sprintf((char*)ptr, ",");
				ptr++;
			}

			// 計測データ(開始)
			sprintf((char*)ptr, "{");
			ptr += strlen((char*)ptr);

			// 計測時刻
			sprintf((char*)ptr, "%s:\"%s\"", SC_SDU_JSON_KEY_MS, timeMS);
			ptr += strlen(ptr);

			// X軸の加速度センサ情報
			if (DBL_MAX != data[i].acc[0]) {
				sprintf((char*)ptr, ",%s:%f", SC_SDU_JSON_KEY_ROW_ACC_X, data[i].acc[0]);
				ptr += strlen(ptr);
			}

			// Y軸の加速度センサ情報
			if (DBL_MAX != data[i].acc[1]) {
				sprintf((char*)ptr, ",%s:%f", SC_SDU_JSON_KEY_ROW_ACC_Y, data[i].acc[1]);
				ptr += strlen(ptr);
			}

			// Z軸の加速度センサ情報
			if (DBL_MAX != data[i].acc[2]) {
				sprintf((char*)ptr, ",%s:%f", SC_SDU_JSON_KEY_ROW_ACC_Z, data[i].acc[2]);
				ptr += strlen(ptr);
			}

			// X軸の重力の方向
			if (DBL_MAX != data[i].gravity[0]) {
				sprintf((char*)ptr, ",%s:%f",
						SC_SDU_JSON_KEY_COR_ACC_X,
						data[i].gravity[0]);
				ptr += strlen(ptr);
			}

			// Y軸の重力の方向
			if (DBL_MAX != data[i].gravity[1]) {
				sprintf((char*)ptr, ",%s:%f", SC_SDU_JSON_KEY_COR_ACC_Y, data[i].gravity[1]);
				ptr += strlen(ptr);
			}

			// Z軸の重力の方向
			if (DBL_MAX != data[i].gravity[2]) {
				sprintf((char*)ptr, ",%s:%f", SC_SDU_JSON_KEY_COR_ACC_Z, data[i].gravity[2]);
				ptr += strlen(ptr);
			}

			// X軸の方位情報
			if (DBL_MAX != data[i].orientation[1]) {
				sprintf((char*)ptr, ",%s:%f", SC_SDU_JSON_KEY_ORT_X, data[i].orientation[1]);
				ptr += strlen(ptr);
			}

			// Y軸の方位情報
			if (DBL_MAX != data[i].orientation[2]) {
				sprintf((char*)ptr, ",%s:%f", SC_SDU_JSON_KEY_ORT_Y, data[i].orientation[2]);
				ptr += strlen(ptr);
			}

			// Z軸の方位情報
			if (DBL_MAX != data[i].orientation[0]) {
				sprintf((char*)ptr, ",%s:%f", SC_SDU_JSON_KEY_ORT_Z, data[i].orientation[0]);
				ptr += strlen(ptr);
			}

			// 計測データ(終了)
			sprintf((char*)ptr, "}");
			ptr++;

			// ファイル出力
			fprintf(fp, "%s", json);
			ptr = json;
		}

		if (0 < dataNum) {
			// 秒単位データ配列(終了)
			sprintf((char*)ptr, "]}");
			ptr += strlen(ptr);

			// 運転特性診断データ(終了)
			sprintf(ptr, "]");
			ptr++;
		}

		// 終了
		sprintf(ptr, "}");

		// ファイル出力
		fprintf(fp, "%s", json);
	} while (0);

	if (NULL != fp) {
		// ファイルクローズ
		fclose(fp);
	}

	if (NULL != json) {
		SC_MEM_Free(json, e_MEM_TYPE_SD);
	}

	SC_LOG_DebugPrint(SC_TAG_SDU, SC_LOG_END);

	return (ret);
}
