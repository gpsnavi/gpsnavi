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
 * SMCoreShareData.c
 *
 *  Created on: 2015/11/05
 *      Author:
 */

#include "SMCoreCMNInternal.h"

#define SC_SHARE_DEBUG									// デバッグログ出力
#undef  SC_SHARE_DEBUG									// デバッグログ抑止

#define SC_SHARE_FILE_EXTENSION_OLD			".old"		// ファイル拡張子(.old)
#define SC_SHARE_FILE_EXTENSION_NEW			".new"		// ファイル拡張子(.new)

//-----------------------------------
// 列挙型定義
//-----------------------------------
// 表示操作対象地図(内部用インデックス)
typedef enum _E_SC_SHARE_IDX_MAPS {
	e_SC_SHARE_IDX_MAPS_MAIN = 0,			// メイン地図
	e_SC_SHARE_IDX_MAPS_APPEND,				// 補助地図
	e_SC_SHARE_IDX_MAPS_SMALL				// スモール地図
} E_SC_SHARE_IDX_MAPS;

//-----------------------------------
// 構造体定義
//-----------------------------------
// ランドマーク表示設定データ
typedef struct _SC_SHARE_LANDMARK {
	Bool				landmark[SC_SHARE_CLSCD_NUM];			// ランドマーク表示設定
} SC_SHARE_LANDMARK;

// 常駐メモリ格納データ定義
typedef struct _SC_SHARE_DATA {
	INT32					scaleLevel[SC_SHARE_MAPS_NUM];			// 地図表示縮尺(スケール)
	INT32					zoomLevel[SC_SHARE_MAPS_NUM];			// ズームレベル
	FLOAT					scaleRange[SC_SHARE_MAPS_NUM];			// 表示倍率
	Bool					bigTextAttr[SC_SHARE_MAPS_NUM];			// 地図上文言拡大表示
	Bool					bigIconAttr[SC_SHARE_MAPS_NUM];			// 地図上表示アイコン拡大表示
	Bool					showLandmark[SC_SHARE_MAPS_NUM];		// ランドマーク表示設定
	Bool					driverMode[SC_SHARE_MAPS_NUM]; 			// 地図ドライバモード
	Bool					cursor[SC_SHARE_MAPS_NUM];				// 地図中心カーソルの表示ステータス
	Bool					scrollMode[SC_SHARE_MAPS_NUM];			// 地図スクロールモード
	SC_SHARE_LANDMARK		showLandmarkAttr[SC_SHARE_MAPS_NUM];	// ランドマーク表示属性設定
	INT32					dispMode[SC_SHARE_MAPS_NUM];			// 地図表示モード
	SMCARSTATE				carState[SC_SHARE_CARSTATE_NUM];		// 車両状態情報
	FLOAT					degreeToward[SC_SHARE_MAPS_NUM];		// 移動角度
	INT32					pixelStep[SC_SHARE_MAPS_NUM];			// 移動長さ
	FLOAT					rate[SC_SHARE_MAPS_NUM];				// 地図フリーズームの拡大比例
	SMGEOCOORD				coord[SC_SHARE_MAPS_NUM];				// 経緯度
	SMRECT					rect[SC_SHARE_MAPS_NUM];				// 矩形
	INT32					rotate[SC_SHARE_MAPS_NUM];				// 地図の回転角度
	FLOAT					scale[SC_SHARE_MAPS_NUM];				// スケール
	INT32					width;									// 解像度(X座標)
	INT32					height;									// 解像度(Y座標)
	INT32					pointNum;								// 地点情報（出発地、経由地、目的地）数
	SMRPPOINT				point[SC_SHARE_POINT_NUM];				// 地点情報（出発地、経由地、目的地）
	Bool					isExistRoute;							// 現在ルートの探索結果の有無
	Bool					isPlanning;								// 探索中かどうか
	SMRPOPTION				rpOption;								// 探索条件
	SMRPTIPINFO				tipInfo;								// 探索詳細エラー情報
	SMSIMULATE				simulate;								// シミュレート情報
	SMREPLAN				replan;									// リルート条件閾値
	SMREALTIMEGUIDEDATA		guideData;								// リアルタイム案内情報
	E_GUIDE_STATUS			guide_status;							// 誘導状態
	SMVOICETTS				voiceTTS;								// 音声ＴＴＳ
	SMTURNLIST				turnList;								// ターンリスト情報数
	SMROUTEHEADINFO			routehead;								// 経路ヘッダ情報
	SMSCDYNAMICGRAPHISIZE	graphisize;								// 交差点拡大図サイズ
	E_DYNAMICGRAPHIC_STATUS	graphistatus;							// 交差点拡大図有無状態
	Char					pathIconDir[SC_MAX_PATH];				// ユーザ定義アイコンのリソースファイルのパス
	Char					pathIconInfo[SC_MAX_PATH];				// 設定ファイルの格納パスのパス
	INT32					iconNum;								// ユーザ定義ダイナミックアイコンデータ数
	SMMAPDYNUDI				iconInfo[SC_SHARE_ICON_NUM];			// ユーザ定義ダイナミックアイコンデータ
	Bool					iconDisp[SC_SHARE_ICON_NUM];			// ユーザ定義ダイナミックアイコンデータの表示/非表示
	Bool					isDemoMode;								// デモモード中かどうか
	E_DEVIATION_STATUS		deviation_status;						// 経路逸脱状態
	SMBITMAPINFO			dynamicbitmap;							// 拡大図情報
	SMRTCOSTINFO			routeCostInfo;							// 経路コスト情報
	INT32					vehicleType;							// 車両タイプ
	Bool					isSaveTracks;							// 走行軌跡を保存するかどうか
	Bool					isOnSurface;							// デバッグ情報を描画するかどうか
	Bool					isEcho;									// 位置情報共有するかどうか
	SMGENREDATA				genreData;								// ジャンルデータ
	Bool					zoomMode[SC_SHARE_MAPS_NUM];			// 地図ズームモード
	INT32					region;									// リージョン
	INT32					language;								// 言語
	SMMAPPINGALERT			alert;									// マッピングアラート情報
	SMTRAFFIC				traffic;								// 交通情報
} SC_SHARE_DATA;
// バックアップファイルヘッダ用
typedef struct _SC_SHARE_SCSD_HEAD {
	INT32					timeOfs;								// タイムスタンプOFS
	INT32					timeSize;								// タイムスタンプサイズ
	INT32					mapverOfs;								// 地図VerOFS
	INT32					mapverSize;								// 地図Verサイズ
	INT32					apiverOfs;								// アプリVerOFS
	INT32					apiverSize;								// アプリVerサイズ
	INT32					dataOfs;								// 復帰データOFS
	INT32					dataSize;								// 復帰データサイズ
} SC_SHARE_SCSD_HEAD;

typedef struct _SC_ROUTEBACKUP {									// 経路バックアップファイル
	UINT8 checksum;													// チェックサム
	UINT8 reserve[3];												//
	INT32 region;													// リージョン
	E_GUIDE_STATUS guide_status;									// 誘導状態
	INT32 pointNum;													// 地点情報数
	SMRPPOINT point[SC_SHARE_POINT_NUM];							// 地点情報（出発地、経由地、目的地）
} SC_ROUTEBACKUP;

//-----------------------------------
// 変数定義
//-----------------------------------
static SC_SHARE_DATA	*shareData;
static Char	confDirPath[SC_MAX_PATH];
static Char oldFilePath[SC_MAX_PATH];
static Char newFilePath[SC_MAX_PATH];

//-----------------------------------
// 関数定義
//-----------------------------------
static E_SC_SHARE_IDX_MAPS GetMapsIdx(INT32 maps);
static INT32 SC_SHARE_WriteFile(const Char *filePath, const void *data, INT32 dataSize);
static INT32 SC_SHARE_ReadFile(const Char *filePath, void *data, INT32 dataSize);
static E_SC_RESULT SC_SHARE_MemRestor(SC_SHARE_DATA* aBin);
static E_SC_RESULT SC_SHARE_DeleteFile(const Char *filePath);

/**
 * @brief 常駐メモリを割り当てる
 * @param[in] ptr      常駐メモリのポインタ
 * @param[in] confPath 設定ファイル格納ディレクトリのフルパス(NULL可)
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_Initialize(void *ptr, const Char *confPath)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	Char	filePath[SC_MAX_PATH] = {};

	SC_LOG_DebugPrint(SC_TAG_SHARE, SC_LOG_START);

	do {
		// 初期化
		confDirPath[0] = EOS;
		shareData = NULL;

		// パラメータチェック
		if (NULL == ptr) {
			SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[ptr], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 常駐メモリのポインタ設定
		shareData = (SC_SHARE_DATA*)ptr;

		if (NULL != confPath) {
			strcpy(confDirPath, confPath);
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_SHARE, SC_LOG_END);

	return (ret);
}

/**
 * @brief 常駐メモリを解放する
 */
void SC_SHARE_Finalize()
{
	Char	filePath[SC_MAX_PATH] = {};

	SC_LOG_DebugPrint(SC_TAG_SHARE, SC_LOG_START);

	// 常駐メモリのポインタにNULL設定
	shareData = NULL;

	SC_LOG_DebugPrint(SC_TAG_SHARE, SC_LOG_END);
}

/**
 * @brief 常駐メモリのデータをファイルから読み込む
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_LoadShareData()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	int		readRet = 0;
	char	filePath[SC_MAX_PATH];
	struct tm* timeTab;
	SC_SHARE_SCSD_HEAD scsdHead;
	SC_SHARE_DATA* wk = NULL;
	Char* mapver = NULL;
	Char* mapverBk = NULL;
	Char* apiverBk = NULL;
	INT32 mapverSize = 0;
	INT32 bufSize = 0;
	UINT8* buf = NULL;

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	do {
		// メモリ初期化
		SC_SHARE_MemInit();

		if (EOS == confDirPath[0]) {
			// データなし
			ret = e_SC_RESULT_NODATA;
			break;
		}

		// 常駐メモリの設定データを格納しているバックアップファイルのパス生成
		if ('/' == confDirPath[strlen(confDirPath) - 1]) {
			sprintf((char*)filePath, "%s%s", confDirPath, SC_SHAREDATA_FILE_NAME);
		} else {
			sprintf((char*)filePath, "%s/%s", confDirPath, SC_SHAREDATA_FILE_NAME);
		}

		// ヘッダ読み込み
		memset(&scsdHead, 0, sizeof(SC_SHARE_SCSD_HEAD));
		// 常駐メモリのデータ読み込み
		readRet = SC_SHARE_ReadFile(filePath, &scsdHead, sizeof(SC_SHARE_SCSD_HEAD));
		if (-1 == readRet) {
			SC_LOG_ErrorPrint(SC_TAG_SHARE, "SC_SHARE_ReadFile error, " HERE);
			ret = e_SC_RESULT_FILE_ACCESSERR;
			break;
		} else if (0 == readRet) {
			// データなし
			ret = e_SC_RESULT_NODATA;
			break;
		} else if (readRet < sizeof(SC_SHARE_SCSD_HEAD)) {
			// データ破損
			ret = e_SC_RESULT_NODATA;
			break;
		}
		bufSize = scsdHead.timeSize + scsdHead.mapverSize + scsdHead.apiverSize + scsdHead.dataSize;
		if (0 == bufSize) {
			ret = e_SC_RESULT_NODATA;
			break;
		}
		bufSize += sizeof(SC_SHARE_SCSD_HEAD);
		// バックアップファイル復帰領域確保
		buf = SC_MALLOC(bufSize);
		if (NULL == buf) {
			ret = e_SC_RESULT_MALLOC_ERR;
			SC_LOG_ErrorPrint(SC_TAG_SHARE, "SC_MALLOC error, " HERE);
			break;
		}
		// バックアップファイル読み込み
		readRet = SC_SHARE_ReadFile(filePath, buf, bufSize);
		if (-1 == readRet) {
			SC_LOG_ErrorPrint(SC_TAG_SHARE, "SC_SHARE_ReadFile error, " HERE);
			ret = e_SC_RESULT_FILE_ACCESSERR;
			break;
		} else if (0 == readRet) {
			// データなし
			ret = e_SC_RESULT_NODATA;
			break;
		} else if (readRet < bufSize) {
			// データ破損
			ret = e_SC_RESULT_NODATA;
			break;
		}
		// 地図Ver取得
		ret = SC_DA_GetSystemMapVerNoDataSize(&mapverSize);
		if (SC_DA_RES_SUCCESS == ret) {
			mapver = SC_MALLOC(sizeof(Char) * mapverSize);
			if (NULL != mapver) {
				ret = SC_DA_GetSystemMapVerNoData(mapver);
			} else {
				ret = SC_DA_RES_FAIL;
			}
		}
		if (SC_DA_RES_SUCCESS != ret) {
			ret = e_SC_RESULT_RDB_ACCESSERR;
			SC_LOG_ErrorPrint(SC_TAG_CORE, (Char*) "SC_DA_GetSystemMapVerNoData error(0x%08x), " HERE, ret);
			break;
		}
		// 日時
		timeTab = (struct tm*) (buf + scsdHead.timeOfs);
		// 地図Verチェック
		mapverBk = (Char*) (buf + scsdHead.mapverOfs);
		if (memcmp(mapver, mapverBk, mapverSize)) {
			SC_LOG_DebugPrint(SC_TAG_DH, " mapver unmatch [%s]!=[%s]", mapver, mapverBk);
			break;
		}
		// AppVerチェック
		apiverBk = (Char*) (buf + scsdHead.apiverOfs);
		if (memcmp(API_VERSION, apiverBk, sizeof(API_VERSION))) {
			SC_LOG_DebugPrint(SC_TAG_DH, " apiver unmatch [%s]!=[%s]", API_VERSION, apiverBk);
			break;
		}
		// Sizeチェック
		if (scsdHead.dataSize != sizeof(SC_SHARE_DATA)) {
			SC_LOG_DebugPrint(SC_TAG_DH, " size unmatch [%d]!=[%d]", scsdHead.dataSize, sizeof(SC_SHARE_DATA));
			break;
		}
		// リストア
		wk = (SC_SHARE_DATA*) (buf + scsdHead.dataOfs);
		SC_SHARE_MemRestor(wk);

		// log
		SC_LOG_InfoPrint(SC_TAG_DH, "*----Restor scsd.bin [%d %2d/%2d %02d:%02d.%02d]----*", 1900 + timeTab->tm_year,
				timeTab->tm_mon + 1, timeTab->tm_mday, timeTab->tm_hour, timeTab->tm_min, timeTab->tm_sec);
		SC_LOG_InfoPrint(SC_TAG_DH, "* map version [%s]", mapver);
		SC_LOG_InfoPrint(SC_TAG_DH, "* api version [%s]", API_VERSION);
		SC_LOG_InfoPrint(SC_TAG_DH, "* location    [%d,%d]", shareData->carState[0].coord.latitude,
				shareData->carState[0].coord.longitude);
		SC_LOG_InfoPrint(SC_TAG_DH, "*---------------------------------------------*");
	} while (0);

	if (NULL != buf) {
		SC_FREE(buf);
	}
	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}

/**
 * @brief 常駐メモリのデータをファイルに書き込む
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SaveShareData()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	char filePath[SC_MAX_PATH];

	struct tm* timeTab;
	SC_SHARE_DATA* wk = NULL;
	SC_SHARE_SCSD_HEAD scsdHead;
	time_t timer;
	INT32 bufSize = 0;
	INT32 mapverSize = 0;
	Char* mapver = NULL;
	UINT8* buf = NULL;

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	do {
		if (EOS != confDirPath[0]) {
			// 常駐メモリの設定データを格納しているバックアップファイルのパス生成
			if ('/' == confDirPath[strlen(confDirPath) - 1]) {
				sprintf((char*) filePath, "%s%s", confDirPath, SC_SHAREDATA_FILE_NAME);
			} else {
				sprintf((char*) filePath, "%s/%s", confDirPath, SC_SHAREDATA_FILE_NAME);
			}

			// 地図Ver取得
			ret = SC_DA_GetSystemMapVerNoDataSize(&mapverSize);
			if (SC_DA_RES_SUCCESS == ret) {
				mapver = SC_MALLOC(sizeof(Char) * mapverSize);
				if (NULL != mapver) {
					ret = SC_DA_GetSystemMapVerNoData(mapver);
				} else {
					ret = SC_DA_RES_FAIL;
				}
			}
			if (SC_DA_RES_SUCCESS != ret) {
				ret = e_SC_RESULT_RDB_ACCESSERR;
				SC_LOG_ErrorPrint(SC_TAG_CORE, (Char*) "SC_DA_GetSystemMapVerNoData error(0x%08x), " HERE, ret);
				break;
			}
			// 日時取得
			timer = time(NULL);
			timeTab = localtime(&timer);
			// ヘッダ情報作成
			scsdHead.timeOfs = sizeof(SC_SHARE_SCSD_HEAD);
			scsdHead.timeSize = sizeof(struct tm);
			scsdHead.mapverOfs = scsdHead.timeOfs + scsdHead.timeSize;
			scsdHead.mapverSize = mapverSize;
			scsdHead.apiverOfs = scsdHead.mapverOfs + scsdHead.mapverSize;
			scsdHead.apiverSize = sizeof(API_VERSION);
			scsdHead.dataOfs = scsdHead.apiverOfs + scsdHead.apiverSize;
			scsdHead.dataSize = sizeof(SC_SHARE_DATA);
			// トータルサイズ
			bufSize = (scsdHead.timeSize + scsdHead.mapverSize + scsdHead.apiverSize
					+ scsdHead.dataSize) + sizeof(SC_SHARE_SCSD_HEAD);
			// ファイル出力用領域取得
			buf = SC_MALLOC(bufSize);
			if (NULL == buf) {
				ret = e_SC_RESULT_MALLOC_ERR;
				SC_LOG_ErrorPrint(SC_TAG_CORE, (Char*) "SC_MALLOC error" HERE);
				break;
			}
			memset(buf, 0, bufSize);
			// ヘッダ書込み
			memcpy(buf, &scsdHead, sizeof(SC_SHARE_SCSD_HEAD));
			// 日時
			memcpy((buf + scsdHead.timeOfs), timeTab, scsdHead.timeSize);
			// 地図Ver
			memcpy((buf + scsdHead.mapverOfs), mapver, scsdHead.mapverSize);
			// ApiVer
			memcpy((buf + scsdHead.apiverOfs), API_VERSION, scsdHead.apiverSize);
			// 共有メモリ情報
			wk = (SC_SHARE_DATA*) (buf + scsdHead.dataOfs);
			// 書込み
			memcpy(wk, shareData, scsdHead.dataSize);
			// 常駐メモリのデータ書き込み
			if (-1 == SC_SHARE_WriteFile(filePath, buf, bufSize)) {
				SC_LOG_ErrorPrint(SC_TAG_SHARE, "SC_SHARE_WriteFile error, " HERE);
			}
			// log
			SC_LOG_InfoPrint(SC_TAG_DH, "*----Backup scsd.bin [%d %2d/%2d %02d:%02d.%02d]----*", 1900 + timeTab->tm_year,
					timeTab->tm_mon + 1, timeTab->tm_mday, timeTab->tm_hour, timeTab->tm_min, timeTab->tm_sec);
			SC_LOG_InfoPrint(SC_TAG_DH, "* map version [%s]", mapver);
			SC_LOG_InfoPrint(SC_TAG_DH, "* api version [%s]", API_VERSION);
			SC_LOG_InfoPrint(SC_TAG_DH, "*---------------------------------------------*");
		}
	} while (0);

	// 作業領域開放
	if (NULL != buf) {
		SC_FREE(buf);
	}
	if (NULL != mapver) {
		SC_FREE(mapver);
	}
	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}

/**
 * @brief 地図表示縮尺(スケール)取得
 * @param[out] scale 表示縮尺(スケール)構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetScaleLevel(SC_DH_SHARE_SCALE *scale)
{
	// パラメータチェック
	if (NULL == scale) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[scale], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 地図表示縮尺(スケール)取得
	scale->scaleLevel = shareData->scaleLevel[GetMapsIdx(scale->maps)];
	scale->scaleRange = shareData->scaleRange[GetMapsIdx(scale->maps)];
	scale->zoomLevel= shareData->zoomLevel[GetMapsIdx(scale->maps)];

#ifdef SC_SHARE_DEBUG
	SC_LOG_DebugPrint(SC_TAG_DH, "地図表示縮尺(スケール)取得           maps=%d scaleLevel=%d scaleRange=%f zoomLevel=%d, " HERE, scale->maps,
			shareData->scaleLevel[GetMapsIdx(scale->maps)], shareData->scaleRange[GetMapsIdx(scale->maps)],
			shareData->zoomLevel[GetMapsIdx(scale->maps)]);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 地図表示縮尺(スケール)設定
 * @param[in] scale 表示縮尺(スケール)構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetScaleLevel(const SC_DH_SHARE_SCALE *scale)
{
	// パラメータチェック
	if (NULL == scale) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[scale], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 地図表示縮尺(スケール)設定
	shareData->scaleLevel[GetMapsIdx(scale->maps)] = scale->scaleLevel;
	shareData->scaleRange[GetMapsIdx(scale->maps)] = scale->scaleRange;
	shareData->zoomLevel[GetMapsIdx(scale->maps)] = scale->zoomLevel;

#ifdef SC_SHARE_DEBUG
	SC_LOG_DebugPrint(SC_TAG_DH, "地図表示縮尺(スケール)設定           maps=%d scaleLevel=%d scaleRange=%f zoomLevel, " HERE, scale->maps,
			shareData->scaleLevel[GetMapsIdx(scale->maps)], shareData->scaleRange[GetMapsIdx(scale->maps)],
			shareData->zoomLevel[GetMapsIdx(scale->maps)]);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 地図上文言拡大表示取得
 * @param[out] bigText 文言拡大表示構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetBigTextAttr(SC_DH_SHARE_BIGTEXTATTR *bigText)
{
	// パラメータチェック
	if (NULL == bigText) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[bigText], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 地図上文言拡大表示取得
	bigText->bigText = shareData->bigTextAttr[GetMapsIdx(bigText->maps)];

#ifdef SC_SHARE_DEBUG
	// 地図上文言拡大表示
	SC_LOG_DebugPrint(SC_TAG_DH,
			"地図上文言拡大表示取得               maps=%d bigTextAttr=%d, " HERE,
			bigText->maps, shareData->bigTextAttr[GetMapsIdx(bigText->maps)]);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 地図上文言拡大表示設定
 * @param[in] bigText 拡大表示構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetBigTextAttr(const SC_DH_SHARE_BIGTEXTATTR *bigText)
{
	// パラメータチェック
	if (NULL == bigText) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[bigText], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 地図上文言拡大表示設定
	shareData->bigTextAttr[GetMapsIdx(bigText->maps)] = bigText->bigText;

#ifdef SC_SHARE_DEBUG
	// 地図上文言拡大表示
	SC_LOG_DebugPrint(SC_TAG_DH,
			"地図上文言拡大表示設定               maps=%d bigTextAttr=%d, " HERE,
			bigText->maps, shareData->bigTextAttr[GetMapsIdx(bigText->maps)]);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 表示アイコン拡大表示取得
 * @param[out] bigIcon 拡大表示構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetBigIconAttr(SC_DH_SHARE_BIGICONATTR *bigIcon)
{
	// パラメータチェック
	if (NULL == bigIcon) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[bigIcon], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 表示アイコン拡大表示取得
	bigIcon->bigIcon = shareData->bigIconAttr[GetMapsIdx(bigIcon->maps)];

#ifdef SC_SHARE_DEBUG
	// 地図上文言拡大表示
	SC_LOG_DebugPrint(SC_TAG_DH,
			"地図上表示アイコン拡大表示取得       maps=%d bigIconAttr=%d, " HERE,
			bigIcon->maps, shareData->bigIconAttr[GetMapsIdx(bigIcon->maps)]);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 表示アイコン拡大表示設定
 * @param[in] isBigIcon 拡大表示構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetBigIconAttr(const SC_DH_SHARE_BIGICONATTR *bigIcon)
{
	// パラメータチェック
	if (NULL == bigIcon) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[bigIcon], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 表示アイコン拡大表示設定
	shareData->bigIconAttr[GetMapsIdx(bigIcon->maps)] = bigIcon->bigIcon;

#ifdef SC_SHARE_DEBUG
	// 地図上文言拡大表示
	SC_LOG_DebugPrint(SC_TAG_DH,
			"地図上表示アイコン拡大表示設定       maps=%d bigIconAttr=%d, " HERE,
			bigIcon->maps, shareData->bigIconAttr[GetMapsIdx(bigIcon->maps)]);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief ランドマーク表示取得
 * @param[out] isShow ランドマーク表示構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetLandmark(SC_DH_SHARE_LANDMARK *landmark)
{
	// パラメータチェック
	if (NULL == landmark) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[landmark], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// ランドマーク表示取得
	landmark->isShow = shareData->showLandmark[GetMapsIdx(landmark->maps)];

#ifdef SC_SHARE_DEBUG
	// ランドマーク表示取得
	SC_LOG_DebugPrint(SC_TAG_DH,
			"ランドマーク表示取得                 maps=%d showLandmark=%d, " HERE,
			landmark->maps, shareData->showLandmark[GetMapsIdx(landmark->maps)]);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief ランドマーク表示設定
 * @param[in] isShow ランドマーク表示構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetLandmark(const SC_DH_SHARE_LANDMARK *landmark)
{
	// パラメータチェック
	if (NULL == landmark) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[landmark], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// ランドマーク表示設定
	shareData->showLandmark[GetMapsIdx(landmark->maps)] = landmark->isShow;

#ifdef SC_SHARE_DEBUG
	// ランドマーク表示設定
	SC_LOG_DebugPrint(SC_TAG_DH,
			"ランドマーク表示設定                 maps=%d showLandmark=%d, " HERE,
			landmark->maps, shareData->showLandmark[GetMapsIdx(landmark->maps)]);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief ランドマーク表示属性取得
 * @param[out] isShow     ランドマーク表示属性構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetLandmarkAttr(SC_DH_SHARE_LANDMARKATTR *landmark)
{
	// パラメータチェック
	if (NULL == landmark) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[landmark], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// ランドマーク表示取得
	landmark->isShow = shareData->showLandmarkAttr[GetMapsIdx(landmark->maps)].landmark[landmark->classCode];

#ifdef SC_SHARE_DEBUG
	// ランドマーク表示属性取得
	SC_LOG_DebugPrint(SC_TAG_DH,
			"ランドマーク表示属性取得             maps=%d classCode=%d showLandmarkAttr=%d, " HERE,
			landmark->maps, landmark->classCode, shareData->showLandmarkAttr[GetMapsIdx(landmark->maps)].landmark[landmark->classCode]);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief ランドマーク表示属性設定
 * @param[in] isShow     ランドマーク表示属性構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetLandmarkAttr(const SC_DH_SHARE_LANDMARKATTR *landmark)
{
	// パラメータチェック
	if (NULL == landmark) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[landmark], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// ランドマーク表示設定
	shareData->showLandmarkAttr[GetMapsIdx(landmark->maps)].landmark[landmark->classCode] = landmark->isShow;

#ifdef SC_SHARE_DEBUG
	// ランドマーク表示属性設定
	SC_LOG_DebugPrint(SC_TAG_DH,
			"ランドマーク表示属性設定             maps=%d classCode=%d showLandmarkAttr=%d, " HERE,
			landmark->maps, landmark->classCode, shareData->showLandmarkAttr[GetMapsIdx(landmark->maps)].landmark[landmark->classCode]);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 地図表示モード取得
 * @param[out] mode 地図表示モード構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetDispMode(SC_DH_SHARE_DISPMODE *mode)
{
	// パラメータチェック
	if (NULL == mode) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[mode], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 地図表示モード取得
	mode->dispMode = shareData->dispMode[GetMapsIdx(mode->maps)];

#ifdef SC_SHARE_DEBUG
	// 地図表示モード
	SC_LOG_DebugPrint(SC_TAG_DH,
			"地図表示モード取得                   maps=%d dispMode=%d, " HERE,
			mode->maps, shareData->dispMode[GetMapsIdx(mode->maps)]);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 地図表示モード設定
 * @param[in] mode 地図表示モード構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetDispMode(const SC_DH_SHARE_DISPMODE *mode)
{
	// パラメータチェック
	if (NULL == mode) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[mode], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 地図表示モード設定
	shareData->dispMode[GetMapsIdx(mode->maps)] = mode->dispMode;

#ifdef SC_SHARE_DEBUG
	// 地図表示モード
	SC_LOG_DebugPrint(SC_TAG_DH,
			"地図表示モード設定                   maps=%d dispMode=%d, " HERE,
			mode->maps, shareData->dispMode[GetMapsIdx(mode->maps)]);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 地図ドライバモード取得
 * @param[out] mode ドライバモード構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetDriverMode(SC_DH_SHARE_DRIVERMODE *mode)
{
	// パラメータチェック
	if (NULL == mode) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[mode], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 地図ドライバモード取得
	mode->driverMode = shareData->driverMode[GetMapsIdx(mode->maps)];

#ifdef SC_SHARE_DEBUG
	// 地図ドライバモード
	SC_LOG_DebugPrint(SC_TAG_DH,
			"地図ドライバモード取得               maps=%d driverMode=%d, " HERE,
			mode->maps, shareData->driverMode[GetMapsIdx(mode->maps)]);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 地図ドライバモード設定
 * @param[in] mode ドライバモード構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetDriverMode(const SC_DH_SHARE_DRIVERMODE *mode)
{
	// パラメータチェック
	if (NULL == mode) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[mode], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 地図ドライバモード設定
	shareData->driverMode[GetMapsIdx(mode->maps)] = mode->driverMode;

#ifdef SC_SHARE_DEBUG
	// 地図ドライバモード
	SC_LOG_DebugPrint(SC_TAG_DH,
			"地図ドライバモード設定               maps=%d driverMode=%d, " HERE,
			mode->maps, shareData->driverMode[GetMapsIdx(mode->maps)]);
#endif

	return (e_SC_RESULT_SUCCESS);
}

//
/**
 * @brief 地図中心カーソルの表示ステータス取得
 * @param[out] cursor 地図中心カーソルステータス構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetShowCursor(SC_DH_SHARE_SHOWCURSOR *cursor)
{
	// パラメータチェック
	if (NULL == cursor) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[cursor], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 地図中心カーソルの表示ステータス取得
	cursor->isShow = shareData->cursor[GetMapsIdx(cursor->maps)];

#ifdef SC_SHARE_DEBUG
	// 地図中心カーソルの表示ステータス
	SC_LOG_DebugPrint(SC_TAG_DH,
			"地図中心カーソルの表示ステータス取得 maps=%d isShow=%d, " HERE,
			cursor->maps, shareData->cursor[GetMapsIdx(cursor->maps)]);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 地図中心カーソルの表示ステータス設定
 * @param[in] cursor 地図中心カーソルステータス構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetShowCursor(const SC_DH_SHARE_SHOWCURSOR *cursor)
{
	// パラメータチェック
	if (NULL == cursor) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[cursor], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == cursor) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 地図中心カーソルの表示ステータス設定
	shareData->cursor[GetMapsIdx(cursor->maps)] = cursor->isShow;

#ifdef SC_SHARE_DEBUG
	// 地図中心カーソルの表示ステータス
	SC_LOG_DebugPrint(SC_TAG_DH,
			"地図中心カーソルの表示ステータス設定 maps=%d isShow=%d, " HERE,
			cursor->maps, shareData->cursor[GetMapsIdx(cursor->maps)]);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 車両状態情報取得
 * @param[out] carState 車両状態情報構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetCarState(SC_DH_SHARE_CARSTATE *carState)
{
	INT32 mode = 0;

	// パラメータチェック
	if (NULL == carState) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[carState], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 車両状態情報取得
	if ((shareData->simulate.simulate == e_SIMULATE_UNAVAILABLE)
	||  (shareData->simulate.state == e_SIMULATE_STATE_STOP)) {
		// 実ロケ (非シミュ環境 or シミュ停止中)
		mode = 0;
	} else {
		// シミュロケ
		mode = 1;
	}
	memcpy(&carState->carState, &shareData->carState[mode], sizeof(SMCARSTATE));

#ifdef SC_SHARE_DEBUG
	// 車両状態情報
	SC_LOG_DebugPrint(SC_TAG_DH,
			"車両状態情報取得                     longitude=%ld latitude=%ld speed=%f dir=%d onRoad=%d isRouteSelected=%d roadClass=%d linkId=%d, mode %d" HERE,
			shareData->carState[mode].coord.longitude, shareData->carState[mode].coord.latitude,
			shareData->carState[mode].speed, shareData->carState[mode].dir, shareData->carState[mode].onRoad,
			shareData->carState[mode].isRouteSelected, shareData->carState[mode].roadClass,
			shareData->carState[mode].linkId, mode);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 車両状態情報設定
 * @param[in] carState  車両状態情報構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetCarState(const SC_DH_SHARE_CARSTATE *carState)
{
	INT32 mode = 0;

	// パラメータチェック
	if (NULL == carState) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[carState], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 車両情報設定
	if ((shareData->simulate.simulate == e_SIMULATE_UNAVAILABLE)
	||  (shareData->simulate.state == e_SIMULATE_STATE_STOP)) {
		// 実ロケ (非シミュ環境 or シミュ停止中)
		mode = 0;
	} else {
		// シミュロケ
		mode = 1;
	}
	memcpy(&shareData[mode].carState, &carState->carState, sizeof(SMCARSTATE));

#ifdef SC_SHARE_DEBUG
	// 車両状態情報
	SC_LOG_DebugPrint(SC_TAG_DH,
			"車両状態情報設定                     longitude=%ld latitude=%ld speed=%f dir=%d onRoad=%d isRouteSelected=%d roadClass=%d linkId=%d, mode %d" HERE,
			shareData->carState[mode].coord.longitude, shareData->carState[mode].coord.latitude,
			shareData->carState[mode].speed, shareData->carState[mode].dir, shareData->carState[mode].onRoad,
			shareData->carState[mode].isRouteSelected, shareData->carState[mode].roadClass,
			shareData->carState[mode].linkId, mode);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 地図スクロールモード取得
 * @param[out] mode スクロールモード構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetScrollMode(SC_DH_SHARE_SCROLLMODE *mode)
{
	// パラメータチェック
	if (NULL == mode) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[mode], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 地図スクロールモード取得
	mode->mode = shareData->scrollMode[GetMapsIdx(mode->maps)];

#ifdef SC_SHARE_DEBUG
	// 地図スクロールモード
	SC_LOG_DebugPrint(SC_TAG_DH,
			"地図スクロールモード取得             maps=%d scrollMode=%d, " HERE,
			mode->maps, shareData->scrollMode[GetMapsIdx(mode->maps)]);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 地図スクロールモード設定
 * @param[in] mode  スクロールモード構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetScrollMode(const SC_DH_SHARE_SCROLLMODE *mode)
{
	// パラメータチェック
	if (NULL == mode) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[mode], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 地図スクロールモード設定
	shareData->scrollMode[GetMapsIdx(mode->maps)] = mode->mode;

#ifdef SC_SHARE_DEBUG
	// 地図スクロールモード
	SC_LOG_DebugPrint(SC_TAG_DH,
			"地図スクロールモード設定             maps=%d scrollMode=%d, " HERE,
			mode->maps, shareData->scrollMode[GetMapsIdx(mode->maps)]);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 地図の移動情報(移動角度と移動長さ)取得
 * @param[out] dir 地図の移動情報(移動角度と移動長さ)構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetMoveMapDir(SC_DH_SHARE_MOVEMAPDIR *dir)
{
	// パラメータチェック
	if (NULL == dir) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[dir], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 移動角度取得
	dir->degreeToward = shareData->degreeToward[GetMapsIdx(dir->maps)];
	// 移動長さ取得
	dir->pixelStep    = shareData->pixelStep[GetMapsIdx(dir->maps)];

#ifdef SC_SHARE_DEBUG
	// 移動角度
	SC_LOG_DebugPrint(SC_TAG_DH,
			"移動角度設定                         maps=%d degreeToward=%f pixelStep=%d, " HERE,
			dir->maps, shareData->degreeToward[GetMapsIdx(dir->maps)], shareData->pixelStep[GetMapsIdx(dir->maps)]);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 地図の移動情報(移動角度と移動長さ)設定
 * @param[in] dir  地図の移動情報(移動角度と移動長さ)構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetMoveMapDir(const SC_DH_SHARE_MOVEMAPDIR *dir)
{
	// パラメータチェック
	if (NULL == dir) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[dir], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 移動角度設定
	shareData->degreeToward[GetMapsIdx(dir->maps)] = dir->degreeToward;
	// 移動長さ設定
	shareData->pixelStep[GetMapsIdx(dir->maps)]    = dir->pixelStep;

#ifdef SC_SHARE_DEBUG
	// 移動角度
	SC_LOG_DebugPrint(SC_TAG_DH,
			"移動角度設定                         maps=%d degreeToward=%f pixelStep=%d, " HERE,
			dir->maps, shareData->degreeToward[GetMapsIdx(dir->maps)], shareData->pixelStep[GetMapsIdx(dir->maps)]);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief フリーズーム時の地図拡大比例取得
 * @param[out] rate 地図フリーズームの拡大比例構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetZoomStepRate(SC_DH_SHARE_STEPRATE *rate)
{
	// パラメータチェック
	if (NULL == rate) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[rate], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// フリーズーム時の地図拡大比例取得
	rate->rate = shareData->rate[GetMapsIdx(rate->maps)];

#ifdef SC_SHARE_DEBUG
	// 地図フリーズームの拡大比例
	SC_LOG_DebugPrint(SC_TAG_DH,
			"地図フリーズームの拡大比例取得       maps=%d rate=%f, " HERE,
			rate->maps, shareData->rate[GetMapsIdx(rate->maps)]);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief フリーズーム時の地図拡大比例設定
 * @param[in] rate 地図フリーズームの拡大比例構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetZoomStepRate(const SC_DH_SHARE_STEPRATE *rate)
{
	// パラメータチェック
	if (NULL == rate) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[rate], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// フリーズーム時の地図拡大比例設定
	shareData->rate[GetMapsIdx(rate->maps)] = rate->rate;

#ifdef SC_SHARE_DEBUG
	// 地図フリーズームの拡大比例
	SC_LOG_DebugPrint(SC_TAG_DH,
			"地図フリーズームの拡大比例設定       maps=%d rate=%f, " HERE,
			rate->maps, shareData->rate[GetMapsIdx(rate->maps)]);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 地図中心の地理座標取得
 * @param[out] coord 経緯度構造体のポインタ
 *                   地図ビューポートの左上（left,top）と
 *                   右下 (right,bottom)のスクリーン座標
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetMapCursorCoord(SC_DH_SHARE_GEOCOORD *coord)
{
	// パラメータチェック
	if (NULL == coord) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[coord], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 地図中心の地理座標取得
	memcpy(&coord->coord, &shareData->coord[GetMapsIdx(coord->maps)], sizeof(SMGEOCOORD));

#ifdef SC_SHARE_DEBUG
	// 経緯度
	SC_LOG_DebugPrint(SC_TAG_DH,
			"経緯度取得                           maps=%d longitude=%ld latitude=%ld, " HERE,
			coord->maps,
			shareData->coord[GetMapsIdx(coord->maps)].longitude, shareData->coord[GetMapsIdx(coord->maps)].latitude);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 地図中心の地理座標設定
 * @param[in] coord  経緯度構造体のポインタ
 *                   地図ビューポートの左上（left,top）と
 *                   右下 (right,bottom)のスクリーン座標
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetMapCursorCoord(const SC_DH_SHARE_GEOCOORD *coord)
{
	// パラメータチェック
	if (NULL == coord) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[coord], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 地図中心の地理座標設定
	memcpy(&shareData->coord[GetMapsIdx(coord->maps)], &coord->coord, sizeof(SMGEOCOORD));

#ifdef SC_SHARE_DEBUG
	// 経緯度
	SC_LOG_DebugPrint(SC_TAG_DH,
			"経緯度設定                           maps=%d longitude=%ld latitude=%ld, " HERE,
			coord->maps,
			shareData->coord[GetMapsIdx(coord->maps)].longitude, shareData->coord[GetMapsIdx(coord->maps)].latitude);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 地図のビューポート取得
 * @param[out] rect 矩形構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetMapViewport(SC_DH_SHARE_RECT *rect)
{
	// パラメータチェック
	if (NULL == rect) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[rect], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 地図のビューポート取得
	memcpy(&rect->rect, &shareData->rect[GetMapsIdx(rect->maps)], sizeof(SMRECT));

#ifdef SC_SHARE_DEBUG
	// 地図のビューポート
	SC_LOG_DebugPrint(SC_TAG_DH,
			"地図のビューポート取得               maps=%d left=%d right=%d top=%d bottom=%d, " HERE,
			rect->maps, shareData->rect[GetMapsIdx(rect->maps)].left, shareData->rect[GetMapsIdx(rect->maps)].right,
			shareData->rect[GetMapsIdx(rect->maps)].top, shareData->rect[GetMapsIdx(rect->maps)].bottom);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 地図のビューポート設定
 * @param[out] rect 矩形構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetMapViewport(const SC_DH_SHARE_RECT *rect)
{
	// パラメータチェック
	if (NULL == rect) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[rect], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 地図のビューポート設定
	memcpy(&shareData->rect[GetMapsIdx(rect->maps)], &rect->rect, sizeof(SMRECT));

#ifdef SC_SHARE_DEBUG
	// 地図のビューポート
	SC_LOG_DebugPrint(SC_TAG_DH,
			"地図のビューポート設定               maps=%d left=%d right=%d top=%d bottom=%d, " HERE,
			rect->maps, shareData->rect[GetMapsIdx(rect->maps)].left, shareData->rect[GetMapsIdx(rect->maps)].right,
			shareData->rect[GetMapsIdx(rect->maps)].top, shareData->rect[GetMapsIdx(rect->maps)].bottom);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 地図の回転角度取得
 * @param[out] rotate 地図の回転角度構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetMapRotate(SC_DH_SHARE_ROTATE *rotate)
{
	// パラメータチェック
	if (NULL == rotate) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[rotate], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 地図の回転角度取得
	rotate->rotate = shareData->rotate[GetMapsIdx(rotate->maps)];

#ifdef SC_SHARE_DEBUG
	// 地図の回転角度
	SC_LOG_DebugPrint(SC_TAG_DH,
			"地図の回転角度取得                   maps=%d rotate=%d, " HERE,
			rotate->maps, shareData->rotate[GetMapsIdx(rotate->maps)]);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 地図の回転角度設定
 * @param[in] rotate 地図の回転角度構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetMapRotate(const SC_DH_SHARE_ROTATE *rotate)
{
	// パラメータチェック
	if (NULL == rotate) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[rotate], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 地図の回転角度設定
	shareData->rotate[GetMapsIdx(rotate->maps)] = rotate->rotate;

#ifdef SC_SHARE_DEBUG
	// 地図の回転角度
	SC_LOG_DebugPrint(SC_TAG_DH,
			"地図の回転角度設定                   maps=%d rotate=%d, " HERE,
			rotate->maps, shareData->rotate[GetMapsIdx(rotate->maps)]);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 地図のスケール取得
 * @param[out] distance 地図スケール構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetGeoDistance(SC_DH_SHARE_GEODISTANCE *distance)
{
	// パラメータチェック
	if (NULL == distance) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[distance], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// TODO 仕様不明のため、unitPixel=65(HMIで設定している値)の場合のみ取得する
	if (65 == distance->unitPixel) {
		// 地図のスケール取得
		distance->scale = shareData->scale[GetMapsIdx(distance->maps)];
	}

#ifdef SC_SHARE_DEBUG
	// スケール
	SC_LOG_DebugPrint(SC_TAG_DH,
			"スケール取得                         maps=%d scale=%f unitPixel=%d, " HERE,
			distance->maps, shareData->scale[GetMapsIdx(distance->maps)], distance->unitPixel);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 地図のスケール設定
 * @param[in] distance 地図スケール構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetGeoDistance(const SC_DH_SHARE_GEODISTANCE *distance)
{
	// パラメータチェック
	if (NULL == distance) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[distance], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// TODO 仕様不明のため、unitPixel=65(HMIで設定している値)の場合のみ設定する
	if (65 == distance->unitPixel) {
		// 地図のスケール設定
		shareData->scale[GetMapsIdx(distance->maps)] = distance->scale;
	}

#ifdef SC_SHARE_DEBUG
	// スケール
	SC_LOG_DebugPrint(SC_TAG_DH,
			"スケール設定                         maps=%d scale=%f unitPixel=%d, " HERE,
			distance->maps, shareData->scale[GetMapsIdx(distance->maps)], distance->unitPixel);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 解像度取得
 * @param[out] distance 解像度構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetResolution(SC_DH_SHARE_RESOLUTION *resolution)
{
	// パラメータチェック
	if (NULL == resolution) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[resolution], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 解像度(X座標)取得
	resolution->width  = shareData->width;
	// 解像度(Y座標)取得
	resolution->height = shareData->height;

#ifdef SC_SHARE_DEBUG
	// 解像度
	SC_LOG_DebugPrint(SC_TAG_DH,
			"解像度取得                           width=%d height=%d, " HERE,
			shareData->width, shareData->height);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 解像度設定
 * @param[in] resolution 解像度構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetResolution(const SC_DH_SHARE_RESOLUTION *resolution)
{
	// パラメータチェック
	if (NULL == resolution) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[resolution], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 解像度(X座標)設定
	shareData->width  = resolution->width;
	// 解像度(Y座標)設定
	shareData->height = resolution->height;

#ifdef SC_SHARE_DEBUG
	// 解像度
	SC_LOG_DebugPrint(SC_TAG_DH,
			"解像度設定                           width=%d height=%d, " HERE,
			shareData->width, shareData->height);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 地点情報（出発地、経由地、目的地）取得
 * @param[out] point    地点情報（出発地、経由地、目的地）構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetAllRPPlace(SC_DH_SHARE_RPPOINT *point)
{
#ifdef SC_SHARE_DEBUG
	INT32	num = 0;
#endif

	// パラメータチェック
	if (NULL == point) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[point], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 地点情報（出発地、経由地、目的地）取得
	point->pointNum = shareData->pointNum;
	if (0 < shareData->pointNum) {
		memcpy(point->point, shareData->point, (sizeof(SMRPPOINT) * shareData->pointNum));
	}

#ifdef SC_SHARE_DEBUG
	SC_LOG_DebugPrint(SC_TAG_DH,
			"地点情報（出発地、経由地、目的地）   pointNum=%d, " HERE,
			shareData->pointNum);

	for (num = 0; num < point->pointNum; num++) {
		SC_LOG_DebugPrint(SC_TAG_DH,
				"地点情報（出発地、経由地、目的地）%d  longitude=%ld latitude=%ld nodeName=%s placeType=%d cond=%d isPassed=%d rpPointIndex=%ld, " HERE,
				(num + 1), shareData->point[num].coord.longitude, shareData->point[num].coord.latitude,
				shareData->point[num].nodeName, shareData->point[num].rpPointType, shareData->point[num].cond,
				shareData->point[num].isPassed, shareData->point[num].rpPointIndex);
	}
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 地点情報（出発地、経由地、目的地）設定
 * @param[in] point      地点情報（出発地、経由地、目的地）構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetAllRPPlace(const SC_DH_SHARE_RPPOINT *point)
{
#ifdef SC_SHARE_DEBUG
	INT32	num = 0;
#endif

	// パラメータチェック
	if (NULL == point) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[point], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (COUNTOF(shareData->point) < point->pointNum) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[point->pointNum], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 地点情報（出発地、経由地、目的地）設定
	shareData->pointNum  = point->pointNum;
	memset(&shareData->point[0], 0, sizeof(shareData->point));
	if (0 < point->pointNum) {
		memcpy(&shareData->point[0], &point->point[0], (sizeof(SMRPPOINT) * point->pointNum));
	}

#ifdef SC_SHARE_DEBUG
	// 地点情報（出発地、経由地、目的地）
	SC_LOG_DebugPrint(SC_TAG_DH,
			"地点情報（出発地、経由地、目的地）   pointNum=%d, " HERE,
			shareData->pointNum);

	for (num = 0; num < point->pointNum; num++) {
		SC_LOG_DebugPrint(SC_TAG_DH,
				"地点情報（出発地、経由地、目的地）%d  longitude=%ld latitude=%ld nodeName=%s rpPointType=%d cond=%d isPassed=%d rpPointIndex=%ld placeType=%d, " HERE,
				(num + 1), shareData->point[num].coord.longitude, shareData->point[num].coord.latitude,
				shareData->point[num].nodeName, shareData->point[num].rpPointType, shareData->point[num].cond,
				shareData->point[num].isPassed, shareData->point[num].rpPointIndex, shareData->point[num].placeType);
		SC_LOG_DebugPrint(SC_TAG_DH, "[施設情報]\nlongitude=%ld\nlatitude=%ld\nname=%s\ndataId=%s\ndataType=%d",
				shareData->point[num].poiPlace.geo.longitude, shareData->point[num].poiPlace.geo.latitude,
				shareData->point[num].poiPlace.name, shareData->point[num].poiPlace.dataId,
				shareData->point[num].poiPlace.dataType);
	}
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 探索結果の有無取得
 * @param[out] exist    探索結果の有無構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetExistRoute(SC_DH_SHARE_EXISTROUTE *exist)
{
	// パラメータチェック
	if (NULL == exist) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[exist], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 探索結果の有無取得
	exist->isExistRoute  = shareData->isExistRoute;

#ifdef SC_SHARE_DEBUG
	// 探索結果の有無
	SC_LOG_DebugPrint(SC_TAG_DH,
			"探索結果の有無取得                   isExistRoute=%d, " HERE,
			shareData->isExistRoute);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 探索結果の有無設定
 * @param[in] exist     探索結果の有無構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetExistRoute(const SC_DH_SHARE_EXISTROUTE *exist)
{
	// パラメータチェック
	if (NULL == exist) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[exist], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 探索結果の有無設定
	shareData->isExistRoute = exist->isExistRoute;

#ifdef SC_SHARE_DEBUG
	// 探索結果の有無
	SC_LOG_DebugPrint(SC_TAG_DH,
			"探索結果の有無設定                   isExistRoute=%d, " HERE,
			shareData->isExistRoute);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 探索中かどうか取得
 * @param[out] plan     探索中かどうか構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetPlanning(SC_DH_SHARE_PLANNING *plan)
{
	// パラメータチェック
	if (NULL == plan) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[plan], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 探索中かどうか取得
	plan->isPlanning = shareData->isPlanning;

#ifdef SC_SHARE_DEBUG
	// 探索中かどうか
	SC_LOG_DebugPrint(SC_TAG_DH,
			"探索中かどうか取得                   isPlanning=%d, " HERE,
			shareData->isPlanning);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 探索中かどうかの有無設定
 * @param[in] resolution 探索中かどうか構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetPlanning(const SC_DH_SHARE_PLANNING *plan)
{
	// パラメータチェック
	if (NULL == plan) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[plan], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 探索中かどうかの有無設定
	shareData->isPlanning = plan->isPlanning;

#ifdef SC_SHARE_DEBUG
	// 探索中かどうか
	SC_LOG_DebugPrint(SC_TAG_DH,
			"探索中かどうか設定                   isPlanning=%d, " HERE,
			shareData->isPlanning);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 探索条件取得
 * @param[out] option   探索条件構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetRPOption(SC_DH_SHARE_RPOPTION *option)
{
	// パラメータチェック
	if (NULL == option) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[option], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 探索条件取得
	memcpy(&option->option, &shareData->rpOption, sizeof(SMRPOPTION));

#ifdef SC_SHARE_DEBUG
	// 探索条件
	SC_LOG_DebugPrint(SC_TAG_DH,
			"探索条件の取得                       rpCond=%d appendCond=%ld regulationType=%d vehicleType=%d tollType=%d, " HERE,
			shareData->rpOption.rpCond, shareData->rpOption.appendCond, shareData->rpOption.regulationType,
			shareData->rpOption.vehicleType, shareData->rpOption.tollType);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 探索条件設定
 * @param[in] option    探索条件構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetRPOption(const SC_DH_SHARE_RPOPTION *option)
{
	// パラメータチェック
	if (NULL == option) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[option], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 探索条件設定
	memcpy(&shareData->rpOption, &option->option, sizeof(SMRPOPTION));
	// 探索条件あり
	shareData->isPlanning = true;

#ifdef SC_SHARE_DEBUG
	// 探索条件
	SC_LOG_DebugPrint(SC_TAG_DH,
			"探索条件の設定                       rpCond=%d appendCond=%ld regulationType=%d vehicleType=%d tollType=%d, " HERE,
			shareData->rpOption.rpCond, shareData->rpOption.appendCond, shareData->rpOption.regulationType,
			shareData->rpOption.vehicleType, shareData->rpOption.tollType);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 探索詳細エラー情報取得
 * @param[out] tipInfo  探索詳細エラー情報構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetRPTip(SC_DH_SHARE_RPTIPINFO *tipInfo)
{
	// パラメータチェック
	if (NULL == tipInfo) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[tipInfo], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 探索詳細エラー情報取得
	memcpy(&tipInfo->tipInfo, &shareData->tipInfo, sizeof(SC_DH_SHARE_RPTIPINFO));

#ifdef SC_SHARE_DEBUG
	// 探索詳細エラー情報
	SC_LOG_DebugPrint(SC_TAG_DH,
			"探索詳細エラー情報取得               tipClass=%d tipIndex=%d isRePlan=%d appendOption=%ld, " HERE,
			shareData->tipInfo.tipClass, shareData->tipInfo.tipIndex,
			shareData->tipInfo.isRePlan, shareData->tipInfo.appendOption);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 探索詳細エラー情報設定
 * @param[in] tipInfo   探索詳細エラー情報構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetRPTip(const SC_DH_SHARE_RPTIPINFO *tipInfo)
{
	// パラメータチェック
	if (NULL == tipInfo) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[tipInfo], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 探索詳細エラー情報設定
	memcpy(&shareData->tipInfo, &tipInfo->tipInfo, sizeof(SC_DH_SHARE_RPTIPINFO));

#ifdef SC_SHARE_DEBUG
	// 探索詳細エラー情報
	SC_LOG_DebugPrint(SC_TAG_DH,
			"探索詳細エラー情報設定               tipClass=%d tipIndex=%d isRePlan=%d appendOption=%ld, " HERE,
			shareData->tipInfo.tipClass, shareData->tipInfo.tipIndex,
			shareData->tipInfo.isRePlan, shareData->tipInfo.appendOption);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 探索結果削除
 * @param[in] param ダミーパラメータ(NULL)
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_DeleteRouteResult(void *param)
{
	USE_IT(param);

	// TODO 探索結果削除

	// 現在ルートの探索結果の有無
	shareData->isExistRoute = false;

	// 地点情報（出発地、経由地、目的地）削除
	if (0 < shareData->pointNum) {
		memset(&shareData->point[0], 0, (sizeof(SMRPPOINT) * shareData->pointNum));
	}
	shareData->pointNum  = 0;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief シミュレータ環境かどうか取得
 * @param[out] sim      シミュレータ環境構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetSimulate(SC_DH_SHARE_SIMULATE *sim)
{
	// パラメータチェック
	if (NULL == sim) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[sim], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// シミュレータ環境かどうか取得
	sim->sumilate = shareData->simulate.simulate;

#ifdef SC_SHARE_DEBUG
	// シミュレータ環境かどうか
	SC_LOG_DebugPrint(SC_TAG_DH,
			"シミュレータ環境取得                 simulate=%d, " HERE,
			shareData->simulate.simulate);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief シミュレータ環境かどうか設定
 * @param[in] sim       シミュレータ環境構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetSimulate(const SC_DH_SHARE_SIMULATE *sim)
{
	// パラメータチェック
	if (NULL == sim) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[sim], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// シミュレータ環境かどうか設定
	shareData->simulate.simulate = sim->sumilate;

#ifdef SC_SHARE_DEBUG
	// シミュレータ環境かどうか
	SC_LOG_DebugPrint(SC_TAG_DH,
			"シミュレータ環境設定                 simulate=%d, " HERE,
			shareData->simulate.simulate);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief シミュレータ速度取得
 * @param[out] simSpeed シミュレータ速度構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetSimulateSpeed(SC_DH_SHARE_SIMULATE_SPEED *simSpeed)
{
	// パラメータチェック
	if (NULL == simSpeed) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[simSpeed], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// シミュレータ速度取得
	simSpeed->speed = shareData->simulate.speed;

#ifdef SC_SHARE_DEBUG
	// シミュレータ速度
	SC_LOG_DebugPrint(SC_TAG_DH,
			"シミュレータ速度取得                 speed=%d, " HERE,
			shareData->simulate.speed);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief シミュレータ速度設定
 * @param[in] simSpeed  シミュレータ速度構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetSimulateSpeed(const SC_DH_SHARE_SIMULATE_SPEED *simSpeed)
{
	// パラメータチェック
	if (NULL == simSpeed) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[simSpeed], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 探索詳細エラー情報設定
	if ((0 > simSpeed->speed) || (100 < simSpeed->speed)) {
		shareData->simulate.speed = 100;
	} else if (0 == simSpeed->speed) {
		shareData->simulate.speed = 1;
	} else {
		shareData->simulate.speed = simSpeed->speed;
	}

#ifdef SC_SHARE_DEBUG
	// 探索詳細エラー情報
	SC_LOG_DebugPrint(SC_TAG_DH,
			"シミュレータ速度設定                 speed=%d, " HERE,
			shareData->simulate.speed);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief シミュレータ状態取得
 * @param[out] simState シミュレータ状態構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetSimulateState(SC_DH_SHARE_SIMULATE_STATE *simState)
{
	// パラメータチェック
	if (NULL == simState) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[simState], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// シミュレータ状態取得
	simState->state = shareData->simulate.state;

#ifdef SC_SHARE_DEBUG
	// シミュレータ状態
	SC_LOG_DebugPrint(SC_TAG_DH,
			"シミュレータ状態取得                 state=%d, " HERE,
			shareData->simulate.state);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief シミュレータ状態設定
 * @param[in] simState  シミュレータ状態構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetSimulateState(const SC_DH_SHARE_SIMULATE_STATE *simState)
{
	// パラメータチェック
	if (NULL == simState) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[simState], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// シミュレータ状態設定
	shareData->simulate.state = simState->state;

#ifdef SC_SHARE_DEBUG
	// シミュレータ状態
	SC_LOG_DebugPrint(SC_TAG_DH,
			"シミュレータ状態設定                 state=%d, " HERE,
			shareData->simulate.state);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief リルート条件閾値取得
 * @param[out] replan   リルート条件閾値構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetReplan(SC_DH_SHARE_REPLAN *replan)
{
	// パラメータチェック
	if (NULL == replan) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[replan], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// リルート条件閾値取得
	memcpy(&replan->replan, &shareData->replan, sizeof(SMREPLAN));

#ifdef SC_SHARE_DEBUG
	// リルート条件閾値
	SC_LOG_DebugPrint(SC_TAG_DH,
			"リルート条件閾値取得                 distance=%d angle=%d, " HERE,
			shareData->replan.distance, shareData->replan.angle);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief リルート条件閾値設定
 * @param[in] replan    リルート条件閾値構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetReplan(const SC_DH_SHARE_REPLAN *replan)
{
	// パラメータチェック
	if (NULL == replan) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[replan], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// リルート条件閾値設定
	memcpy(&shareData->replan, &replan->replan, sizeof(SMREPLAN));

#ifdef SC_SHARE_DEBUG
	// リルート条件閾値
	SC_LOG_DebugPrint(SC_TAG_DH,
			"リルート条件閾値設定                 distance=%d angle=%d, " HERE,
			shareData->replan.distance, shareData->replan.angle);
#endif

	return (e_SC_RESULT_SUCCESS);
}


/**
 * @brief リアルタイム案内情報取得
 * @param[out] guide    リアルタイム案内情報構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetRealTimeInfo(SC_DH_SHARE_GUIDE_DATA *guide)
{
	// パラメータチェック
	if (NULL == guide) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[guide], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// リアルタイム案内情報取得
	memcpy(&guide->guideData, &shareData->guideData, sizeof(SMREALTIMEGUIDEDATA));

#ifdef SC_SHARE_DEBUG
	INT32	num = 0;
	// リアルタイム案内情報
	SC_LOG_DebugPrint(SC_TAG_DH,
			"リアルタイム案内情報取得             turnDir=%d nextTurn=%ld trafficLight=%d rtToNextPlace=%ld destination=%d bypass=%d rdToNextPlace=%ld nextBroadString=%s roadLaneNum=%d, " HERE,
			shareData->guideData.turnDir, shareData->guideData.remainDistToNextTurn,
			shareData->guideData.showTrafficLight, shareData->guideData.remainTimeToNextPlace,
			shareData->guideData.destination, shareData->guideData.bypass,
			shareData->guideData.remainDistToNextPlace, shareData->guideData.nextBroadString,
			shareData->guideData.roadLaneNum);
	for (num = 0; num < shareData->guideData.roadLaneNum; num++) {
		SC_LOG_DebugPrint(SC_TAG_DH,
				"リアルタイム案内情報取得 車線%d      nextBroadString=%s laneHightLight=%d advisableLaneFlag=%d, " HERE,
				(num + 1), shareData->guideData.roadLane[num].laneFlag,
				shareData->guideData.roadLane[num].laneHightLight, shareData->guideData.roadLane[num].advisableLaneFlag);
	}
	SC_LOG_DebugPrint(SC_TAG_DH,
			"リアルタイム案内情報取得             passedDistance=%ld aheadPoint=%d valid=%d graphMaxShowDist=%d roadType=%d roadSituation=%d nextBypassIndex, " HERE,
			shareData->guideData.passedDistance, shareData->guideData.aheadPoint,
			shareData->guideData.valid, shareData->guideData.graphMaxShowDist,
			shareData->guideData.roadType, shareData->guideData.roadSituation,
			shareData->guideData.nextBypassIndex);
	for (num = 0; num < shareData->guideData.roadLaneAtGuidePointNum; num++) {
		SC_LOG_DebugPrint(SC_TAG_DH,
				"リアルタイム案内情報取得 案内点%d    nextBroadString=%s laneHightLight=%d advisableLaneFlag=%d, " HERE,
				(num + 1), shareData->guideData.roadLaneAtGuidePoint[num].laneFlag,
				shareData->guideData.roadLaneAtGuidePoint[num].laneHightLight, shareData->guideData.roadLaneAtGuidePoint[num].advisableLaneFlag);
	}
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief リアルタイム案内情報設定
 * @param[in] guide     リアルタイム案内情報構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetRealTimeInfo(const SC_DH_SHARE_GUIDE_DATA *guide)
{
	//INT32	num = 0;
	// パラメータチェック
	if (NULL == guide) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[guide], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// リアルタイム案内情報設定
	memcpy(&shareData->guideData, &guide->guideData, sizeof(SMREALTIMEGUIDEDATA));

#ifdef SC_SHARE_DEBUG
	// リアルタイム案内情報
	SC_LOG_DebugPrint(SC_TAG_DH,
			"リアルタイム案内情報設定             turnDir=%d nextTurn=%ld trafficLight=%d rtToNextPlace=%ld destination=%d bypass=%d rdToNextPlace=%ld nextBroadString=%s roadLaneNum=%d, " HERE,
			shareData->guideData.turnDir, shareData->guideData.remainDistToNextTurn,
			shareData->guideData.showTrafficLight, shareData->guideData.remainTimeToNextPlace,
			shareData->guideData.destination, shareData->guideData.bypass,
			shareData->guideData.remainDistToNextPlace, shareData->guideData.nextBroadString,
			shareData->guideData.roadLaneNum);
	for (num = 0; num < shareData->guideData.roadLaneNum; num++) {
		SC_LOG_DebugPrint(SC_TAG_DH,
				"リアルタイム案内情報設定 車線%d      nextBroadString=%s laneHightLight=%d advisableLaneFlag=%d, " HERE,
				(num + 1), shareData->guideData.roadLane[num].laneFlag,
				shareData->guideData.roadLane[num].laneHightLight, shareData->guideData.roadLane[num].advisableLaneFlag);
	}
	SC_LOG_DebugPrint(SC_TAG_DH,
			"リアルタイム案内情報設定             passedDistance=%ld aheadPoint=%d valid=%d graphMaxShowDist=%d roadType=%d roadSituation=%d nextBypassIndex, " HERE,
			shareData->guideData.passedDistance, shareData->guideData.aheadPoint,
			shareData->guideData.valid, shareData->guideData.graphMaxShowDist,
			shareData->guideData.roadType, shareData->guideData.roadSituation,
			shareData->guideData.nextBypassIndex);
	for (num = 0; num < shareData->guideData.roadLaneAtGuidePointNum; num++) {
		SC_LOG_DebugPrint(SC_TAG_DH,
				"リアルタイム案内情報設定 案内点%d    nextBroadString=%s laneHightLight=%d advisableLaneFlag=%d, " HERE,
				(num + 1), shareData->guideData.roadLaneAtGuidePoint[num].laneFlag,
				shareData->guideData.roadLaneAtGuidePoint[num].laneHightLight, shareData->guideData.roadLaneAtGuidePoint[num].advisableLaneFlag);
	}
#endif

	return (e_SC_RESULT_SUCCESS);
}


/**
 * @brief 誘導状態取得
 * @param[out] status   誘導状態構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetGuideStatus(SC_DH_SHARE_GUIDE_STATUS *status)
{
	// パラメータチェック
	if (NULL == status) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[status], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 誘導状態取得
	status->guide_status = shareData->guide_status;

#ifdef SC_SHARE_DEBUG
	// 誘導状態
	SC_LOG_DebugPrint(SC_TAG_DH,
			"誘導状態取得                         guide_status=%d, " HERE,
			shareData->guide_status);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 誘導状態設定
 * @param[in] status    誘導状態構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetGuideStatus(const SC_DH_SHARE_GUIDE_STATUS *status)
{
	// パラメータチェック
	if (NULL == status) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[status], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 誘導状態設定
	shareData->guide_status = status->guide_status;

#ifdef SC_SHARE_DEBUG
	// 誘導状態
	SC_LOG_DebugPrint(SC_TAG_DH,
			"誘導状態設定                         guide_status=%d, " HERE,
			shareData->guide_status);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 経路逸脱状態取得
 * @param[out] status   経路逸脱状態構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetDeviationStatus(SC_DH_SHARE_DEVIATION_STATUS *status)
{
	// パラメータチェック
	if (NULL == status) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[status], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 誘導状態取得
	status->deviation_status = shareData->deviation_status;

#ifdef SC_SHARE_DEBUG
	// 誘導中車両状態
	SC_LOG_DebugPrint(SC_TAG_DH,
			"経路逸脱状態取得                   deviation_status=%d, " HERE,
			shareData->deviation_status);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 経路逸脱状態設定
 * @param[in] status    経路逸脱状態構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetDeviationStatus(const SC_DH_SHARE_DEVIATION_STATUS *status)
{
	// パラメータチェック
	if (NULL == status) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[status], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 誘導中車両状態設定
	shareData->deviation_status = status->deviation_status;

#ifdef SC_SHARE_DEBUG
	// 誘導状態
	SC_LOG_DebugPrint(SC_TAG_DH,
			"経路逸脱状態設定                   deviation_status=%d, " HERE,
			shareData->deviation_status);
#endif

	return (e_SC_RESULT_SUCCESS);
}
/**
 * @brief 音声ＴＴＳ情報取得
 * @param[out] voice    音声ＴＴＳ情報構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetVoiceTTS(SC_DH_SHARE_VOICE_TTS *voice)
{
	// パラメータチェック
	if (NULL == voice) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[voice], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// リアルタイム案内情報取得
	memcpy(&voice->voiceTTS, &shareData->voiceTTS, sizeof(SMVOICETTS));


	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 音声ＴＴＳ情報設定
 * @param[in] voice     音声ＴＴＳ情報構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetVoiceTTS(const SC_DH_SHARE_VOICE_TTS *voice)
{
	// パラメータチェック
	if (NULL == voice) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[voice], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// リアルタイム案内情報設定
	memcpy(&shareData->voiceTTS, &voice->voiceTTS, sizeof(SMVOICETTS));

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief ターンリスト情報取得
 * @param[out] turnList    ターンリスト情報構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetTurnListInfo(SC_DH_SHARE_TURN_LIST *turnList)
{
	// パラメータチェック
	if (NULL == turnList) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[turnList], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// ターンリスト情報取得
	memcpy(&turnList->turnList, &shareData->turnList, sizeof(SMTURNLIST));

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief ターンリスト情報取得
 * @param[in] turnList    ターンリスト情報構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetTurnListInfo(const SC_DH_SHARE_TURN_LIST *turnList)
{
	// パラメータチェック
	if (NULL == turnList) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[point], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// ターンリスト情報設定
	memcpy(&shareData->turnList, &turnList->turnList, sizeof(SMTURNLIST));

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 経路総距離情報取得
 * @param[out] rotue    経路総距離テーブルポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetRouteLength(SC_DH_SHARE_ROUTE_LENGTH *rotue)
{
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	// パラメータチェック
	if (NULL == rotue) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[rotue], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 経路総距離情報収録
	rotue->length = shareData->routehead.length;

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 経路総距離情報設定
 * @param[out] routelength  	経路総距離テーブルポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetRouteLength(const SC_DH_SHARE_ROUTE_LENGTH* rtLength)
{
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	// パラメータチェック
	if (NULL == rtLength) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[status], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	// 誘導状態
	SC_LOG_DebugPrint(SC_TAG_DH,"shareData->routehead.length = %d, " HERE,shareData->routehead.length);
	SC_LOG_DebugPrint(SC_TAG_DH, "length = %d, " HERE, rtLength->length);

	// 経路総距離情報取得
	shareData->routehead.length = rtLength->length;

#ifdef SC_SHARE_DEBUG
	// 誘導状態
	SC_LOG_DebugPrint(SC_TAG_DH,
			"経路総距離情報設定                         route_length =%d, " HERE,
			shareData->routehead.length);
#endif
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 経路所要時間情報取得
 * @param[out] rotue    経路所要時間テーブルポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetRouteAveTimne(SC_DH_SHARE_ROUTE_AVETIME *rotue)
{

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	// パラメータチェック
	if (NULL == rotue) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[rotue], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 経路所要時間情報取得
	rotue->avetime = shareData->routehead.avetime;

#ifdef SC_SHARE_DEBUG
	// 誘導状態
	SC_LOG_DebugPrint(SC_TAG_DH,
			"経路所要時間情報設定                         route_avetime =%d, " HERE,
			shareData->routehead.avetime);
#endif

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 経路所要時間情報設定
 * @param[out] rotue  	経路所要時間テーブルポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetRouteAveTimne(const SC_DH_SHARE_ROUTE_AVETIME* rtAvetime)
{
	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_START);

	// パラメータチェック
	if (NULL == rtAvetime) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[status], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	// 経路所要時間情報設定
	shareData->routehead.avetime = rtAvetime->avetime;

#ifdef SC_SHARE_DEBUG
	// 誘導状態
	SC_LOG_DebugPrint(SC_TAG_DH,
			"経路所要時間情報設定                         route_avetime =%d, " HERE,
			shareData->routehead.avetime);
#endif

	SC_LOG_DebugPrint(SC_TAG_RC, SC_LOG_END);

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 経路高速道路総距離取得
 * @param[out] rotue    経路高速距離テーブルポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetRouteHwayLength(SC_DH_SHARE_ROUTE_HWAYLENGTH *rotue)
{
	// パラメータチェック
	if (NULL == rotue) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[rotue], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 経路高速道路総距離情報取得
	rotue->hwaylength = shareData->routehead.hwaylength;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 経路高速道路総距離設定
 * @param[out] rotue  	経路高速距離テーブルポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetRouteHwayLength(const SC_DH_SHARE_ROUTE_HWAYLENGTH* rtHWLength)
{
	// パラメータチェック
	if (NULL == rtHWLength) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[status], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	// 経路高速道路総距離設定
	shareData->routehead.hwaylength = rtHWLength->hwaylength;

#ifdef SC_SHARE_DEBUG
	// 誘導状態
	SC_LOG_DebugPrint(SC_TAG_DH,
			"経路高速道路総距離設定                         route_hwaylength =%d, " HERE,
			shareData->routehead.hwaylength);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 経路有料道路総距離取得
 * @param[out] rotue    経路有料距離テーブルポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetRouteTollLength(SC_DH_SHARE_ROUTE_TOLLLENGTH *rotue)
{
	// パラメータチェック
	if (NULL == rotue) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[rotue], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 経路高速道路総距離情報取得
	rotue->tolllength= shareData->routehead.tolllength;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 経路有料道路総距離設定
 * @param[out] rotue  	経路有料距離テーブルポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetRouteTollLength(INT32 tolllength)
{
	// ターンリスト情報取得
	shareData->routehead.tolllength= tolllength;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 経路有料道路料金取得
 * @param[out] rotue    経路有料料金テーブルポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetRouteTollFee(SC_DH_SHARE_ROUTE_TOLLFEE *rotue)
{
	// パラメータチェック
	if (NULL == rotue) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[rotue], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 経路高速道路総距離情報取得
	rotue->tollfee= shareData->routehead.tollfee;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 経路有料道路料金設定
 * @param[out] rotue  	経路有料料金テーブルポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetRouteTollFee(INT32 tollfee)
{
	// ターンリスト情報取得
	shareData->routehead.tollfee= tollfee;

	return (e_SC_RESULT_SUCCESS);
}
/**
 * @brief 表示操作対象地図取得
 * @param[in] maps 表示操作対象地図
 * @return E_SC_SHARE_IDX_MAPS 表示操作対象地図(内部用インデックス)
 */
E_SC_SHARE_IDX_MAPS GetMapsIdx(INT32 maps)
{
	E_SC_SHARE_IDX_MAPS	idx = e_SC_SHARE_IDX_MAPS_MAIN;

	switch (maps) {
	// メイン地図
	case SC_SHARE_MAPS_MAIN:
		idx = e_SC_SHARE_IDX_MAPS_MAIN;
		break;
	// 補助地図
	case SC_SHARE_MAPS_APPEND:
		idx = e_SC_SHARE_IDX_MAPS_APPEND;
		break;
	// スモール地図
	case SC_SHARE_MAPS_SMALL:
		idx = e_SC_SHARE_IDX_MAPS_SMALL;
		break;
	// 無効値
	default:
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "parame error[maps], " HERE);
		idx = e_SC_SHARE_IDX_MAPS_MAIN;
		break;
	}

	return (idx);
}

/**
 * @brief 車両状態情報取得(実ロケーション)
 * @param[out] carState 車両状態情報構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetCarStateReal(SC_DH_SHARE_CARSTATE *carState)
{
	// パラメータチェック
	if (NULL == carState) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[carState], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 車両状態情報取得
	memcpy(&carState->carState, &shareData->carState[0], sizeof(SMCARSTATE));

#ifdef SC_SHARE_DEBUG
	// 車両状態情報
	SC_LOG_DebugPrint(SC_TAG_DH,
			"車両状態情報取得                     longitude=%ld latitude=%ld speed=%f dir=%d onRoad=%d isRouteSelected=%d roadClass=%d linkId=%d, " HERE,
			shareData->carState[0].coord.longitude, shareData->carState[0].coord.latitude, shareData->carState[0].speed,
			shareData->carState[0].dir, shareData->carState[0].onRoad, shareData->carState[0].isRouteSelected,
			shareData->carState[0].roadClass, shareData->carState[0].linkId);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 車両状態情報設定(実ロケーション)
 * @param[in] carState  車両状態情報構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetCarStateReal(const SC_DH_SHARE_CARSTATE *carState)
{
	// パラメータチェック
	if (NULL == carState) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[carState], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 車両情報設定
	memcpy(&shareData->carState[0], &carState->carState, sizeof(SMCARSTATE));

#ifdef SC_SHARE_DEBUG
	// 車両状態情報
	SC_LOG_DebugPrint(SC_TAG_DH,
			"車両状態情報設定                     longitude=%ld latitude=%ld speed=%f dir=%d onRoad=%d isRouteSelected=%d roadClass=%d linkId=%d, " HERE,
			shareData->carState[0].coord.longitude, shareData->carState[0].coord.latitude, shareData->carState[0].speed,
			shareData->carState[0].dir, shareData->carState[0].onRoad, shareData->carState[0].isRouteSelected,
			shareData->carState[0].roadClass, shareData->carState[0].linkId);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 車両状態情報取得(シミュレータロケーション)
 * @param[out] carState 車両状態情報構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetCarStateSimu(SC_DH_SHARE_CARSTATE *carState)
{
	// パラメータチェック
	if (NULL == carState) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[carState], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 車両状態情報取得
	memcpy(&carState->carState, &shareData->carState[1], sizeof(SMCARSTATE));

#ifdef SC_SHARE_DEBUG
	// 車両状態情報
	SC_LOG_DebugPrint(SC_TAG_DH,
			"車両状態情報取得                     longitude=%ld latitude=%ld speed=%f dir=%d onRoad=%d isRouteSelected=%d roadClass=%d linkId=%d, " HERE,
			shareData->carState[1].coord.longitude, shareData->carState[1].coord.latitude, shareData->carState[1].speed,
			shareData->carState[1].dir, shareData->carState[1].onRoad, shareData->carState[1].isRouteSelected,
			shareData->carState[1].roadClass, shareData->carState[1].linkId);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 車両状態情報設定(シミュレータロケーション)
 * @param[in] carState  車両状態情報構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetCarStateSimu(const SC_DH_SHARE_CARSTATE *carState)
{
	// パラメータチェック
	if (NULL == carState) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[carState], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 車両情報設定
	memcpy(&shareData->carState[1], &carState->carState, sizeof(SMCARSTATE));

#ifdef SC_SHARE_DEBUG
	// 車両状態情報
	SC_LOG_DebugPrint(SC_TAG_DH,
			"車両状態情報設定                     longitude=%ld latitude=%ld speed=%f dir=%d onRoad=%d isRouteSelected=%d roadClass=%d linkId=%d, " HERE,
			shareData->carState[1].coord.longitude, shareData->carState[1].coord.latitude, shareData->carState[1].speed,
			shareData->carState[1].dir, shareData->carState[1].onRoad, shareData->carState[1].isRouteSelected,
			shareData->carState[1].roadClass, shareData->carState[1].linkId);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 交差点拡大図サイズ設定
 * @param[in] graphisize  縦横サイズ構造体
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetDynamicGraphiSize(const SC_DH_SHARE_DYNAMICGRAPHISIZE *graphisize)
{
	// パラメータチェック
	if (NULL == graphisize) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[grapjisize], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 交差点拡大図情報設定
	shareData->graphisize.width  = graphisize->width;
	shareData->graphisize.height = graphisize->height;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 交差点拡大図サイズ取得
 * @param[in] graphisize  縦横サイズ構造体
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetDynamicGraphiSize(SC_DH_SHARE_DYNAMICGRAPHISIZE *graphisize)
{
	// パラメータチェック
	if (NULL == graphisize) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[grapjisize], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 共有メモリから交差点拡大図情報を取得
	graphisize->width  = shareData->graphisize.width;
	graphisize->height = shareData->graphisize.height;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 交差点拡大図有無設定
 * @param[in] graphisize  縦横サイズ構造体
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetDynamicGraphiStat(const SC_DH_SHARE_DYNAMICGRAPHICSTATUS *status)
{

	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 交差点拡大図情報設定
	shareData->graphistatus  = status->graphic_stat;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 交差点拡大図有無取得
 * @param[in] graphisize  縦横サイズ構造体
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetDynamicGraphiStat(SC_DH_SHARE_DYNAMICGRAPHICSTATUS *status)
{

	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 交差点拡大図有無取得
	status->graphic_stat = shareData->graphistatus;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 交差点拡大図情報設定
 * @param[in] graphisize  縦横サイズ構造体
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetDynamicGraphiBitmap(const SC_DH_SHARE_DYNAMICGRAPHIBITMAP *bitmap)
{

	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 交差点拡大図情報設定
	shareData->dynamicbitmap.bitmap     = bitmap->bitmapinfo.bitmap;
	shareData->dynamicbitmap.bitmapsize = bitmap->bitmapinfo.bitmapsize;
	shareData->dynamicbitmap.bufsize    = bitmap->bitmapinfo.bufsize;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 交差点拡大図情報取得
 * @param[in] graphisize  縦横サイズ構造体
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetDynamicGraphiBitmap(SC_DH_SHARE_DYNAMICGRAPHIBITMAP *bitmap)
{

	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 交差点拡大図情報取得
	bitmap->bitmapinfo.bitmap     = shareData->dynamicbitmap.bitmap;
	bitmap->bitmapinfo.bitmapsize = shareData->dynamicbitmap.bitmapsize;
	bitmap->bitmapinfo.bufsize    = shareData->dynamicbitmap.bufsize;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief ユーザ定義アイコンのリソースと設定ファイルの格納パス取得
 * @param[out] resource ユーザ定義アイコンのリソースと設定ファイルの格納パス構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetUDIResourcePath(SC_DH_SHARE_ICON_RESOURCE_PATH *resource)
{
	// パラメータチェック
	if (NULL == resource) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[resource], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// ユーザ定義アイコンのリソースと設定ファイルの格納パス取得
	strcpy(resource->pathIconDir,  shareData->pathIconDir);
	strcpy(resource->pathIconInfo, shareData->pathIconInfo);

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief ユーザ定義アイコンのリソースと設定ファイルの格納パス設定
 * @param[in] resource  ユーザ定義アイコンのリソースと設定ファイルの格納パス構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetUDIResourcePath(const SC_DH_SHARE_ICON_RESOURCE_PATH *resource)
{
	// パラメータチェック
	if (NULL == resource) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[resource], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// ユーザ定義アイコンのリソースと設定ファイルの格納パス設定
	strcpy(shareData->pathIconDir,  resource->pathIconDir);
	strcpy(shareData->pathIconInfo, resource->pathIconInfo);

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief ユーザ定義ダイナミックアイコンデータ取得
 * @param[out] iconInfo ユーザ定義ダイナミックアイコンデータ構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetIconInfo(SC_DH_SHARE_MAPDYNUDI *resource)
{
	// パラメータチェック
	if (NULL == resource) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[resource], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// アイコンデータ取得
	if (0 < shareData->iconNum) {
		memcpy(resource->iconInfo, shareData->iconInfo, sizeof(SMMAPDYNUDI) * shareData->iconNum);
	}
	resource->iconNum = shareData->iconNum;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief ユーザ定義ダイナミックアイコンデータ設定
 * @param[in] iconInfo  ユーザ定義ダイナミックアイコンデータ構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetIconInfo(const SC_DH_SHARE_MAPDYNUDI *iconInfo)
{
	// パラメータチェック
	if (NULL == iconInfo) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[iconInfo], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// アイコンデータ設定
	if (0 < iconInfo->iconNum) {
		memcpy(shareData->iconInfo, iconInfo->iconInfo, sizeof(SMMAPDYNUDI) * iconInfo->iconNum);
	} else {
		memset(shareData->iconInfo, 0, sizeof(shareData->iconInfo));
	}
	shareData->iconNum = iconInfo->iconNum;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief ユーザ定義ダイナミックアイコンデータの表示/非表示取得
 * @param[out] dispInfo ユーザ定義ダイナミックアイコンデータ構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetDynamicUDIDisplay(SC_DH_SHARE_MAPDYNUDI *dispInfo)
{
	// パラメータチェック
	if (NULL == dispInfo) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[dispInfo], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// ユーザ定義ダイナミックアイコンデータの表示/非表示取得
	if (0 < shareData->iconNum) {
		memcpy(dispInfo->iconDisp, shareData->iconDisp, sizeof(shareData->iconDisp));
	}
	dispInfo->iconNum = shareData->iconNum;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief ユーザ定義ダイナミックアイコンデータの表示/非表示設定
 * @param[in] dispInfo  ユーザ定義ダイナミックアイコンデータ構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetDynamicUDIDisplay(const SC_DH_SHARE_MAPDYNUDI *dispInfo)
{
	// パラメータチェック
	if (NULL == dispInfo) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[dispInfo], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// ユーザ定義ダイナミックアイコンデータの表示/非表示取得
	if (0 < dispInfo->iconNum) {
		memcpy(shareData->iconDisp, dispInfo->iconDisp, sizeof(shareData->iconDisp));
	} else {
		memset(shareData->iconDisp, 0, sizeof(shareData->iconDisp));
	}
	shareData->iconNum = dispInfo->iconNum;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief デモモード取得
 * @param[out] demo    デモモード構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetDemoMode(SC_DH_SHARE_DEMOMODE *demo)
{
	// パラメータチェック
	if (NULL == demo) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[demo], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// デモモード取得
	demo->isDemoMode  = shareData->isDemoMode;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief デモモード設定
 * @param[in] demo     デモモード構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetDemoMode(const SC_DH_SHARE_DEMOMODE *demo)
{
	// パラメータチェック
	if (NULL == demo) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[demo], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// デモモード設定
	shareData->isDemoMode = demo->isDemoMode;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 経路コスト情報取得
 * @param[out] rtcost    経路コスト情報構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetRouteCostInfo(SC_DH_SHARE_ROUTECOSTINFO *rtcost)
{
	// パラメータチェック
	if (NULL == rtcost) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[rtcost], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 経路コスト情報取得
	memcpy(&rtcost->rtCostInfo, &shareData->routeCostInfo, sizeof(SMRTCOSTINFO));

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 経路コスト情報設定
 * @param[in] rtcost    経路コスト情報構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetRouteCostInfo(const SC_DH_SHARE_ROUTECOSTINFO *rtcost)
{
	//INT32	num = 0;
	// パラメータチェック
	if (NULL == rtcost) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[rtcost], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// リアルタイム案内情報設定
	memcpy(&shareData->routeCostInfo, &rtcost->rtCostInfo, sizeof(SMRTCOSTINFO));

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 車両タイプ取得
 * @param[out] vehicleType    SC_DH_SHARE_VEHICLETYPEのポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetVehicleType(SC_DH_SHARE_VEHICLETYPE *vehicleType)
{
	// パラメータチェック
	if (NULL == vehicleType) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[vehicleType], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 車両タイプ取得
	vehicleType->vehicleType = shareData->vehicleType;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 車両タイプ設定
 * @param[in] vehicleType    SC_DH_SHARE_VEHICLETYPEのポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetVehicleType(const SC_DH_SHARE_VEHICLETYPE *vehicleType)
{
	//INT32	num = 0;
	// パラメータチェック
	if (NULL == vehicleType) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[vehicleType], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 車両タイプ設定
	shareData->vehicleType = vehicleType->vehicleType;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 走行軌跡を保存するかどうか取得
 * @param[out] isSavetracks    SC_DH_SHARE_SAVETRACKSのポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetSaveTracksFlag(SC_DH_SHARE_SAVETRACKS *isSavetracks)
{
	// パラメータチェック
	if (NULL == isSavetracks) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[isSavetracks], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 走行軌跡を保存するかどうか取得
	isSavetracks->isSaveTracks = shareData->isSaveTracks;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 走行軌跡を保存するかどうか設定
 * @param[in] isSavetracks    SC_DH_SHARE_SAVETRACKSのポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetSaveTracksFlag(const SC_DH_SHARE_SAVETRACKS *isSavetracks)
{
	//INT32	num = 0;
	// パラメータチェック
	if (NULL == isSavetracks) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[isSavetracks], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 走行軌跡を保存するかどうか設定
	shareData->isSaveTracks = isSavetracks->isSaveTracks;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief デバッグ情報を描画するかどうか取得
 * @param[out] isOnSurface    SC_DH_SHARE_ONSURFACEのポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetDebugInfoOnSurface(SC_DH_SHARE_ONSURFACE *isOnSurface)
{
	// パラメータチェック
	if (NULL == isOnSurface) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[isOnSurface], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 走行軌跡を保存するかどうか取得
	isOnSurface->isOnSurface = shareData->isOnSurface;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief デバッグ情報を描画するかどうか設定
 * @param[in] isOnSurface    SC_DH_SHARE_ONSURFACEのポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetDebugInfoOnSurface(const SC_DH_SHARE_ONSURFACE *isOnSurface)
{
	//INT32	num = 0;
	// パラメータチェック
	if (NULL == isOnSurface) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[isOnSurface], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 走行軌跡を保存するかどうか設定
	shareData->isOnSurface = isOnSurface->isOnSurface;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 位置情報共有するかどうか取得
 * @param[out] isEcho    SC_DH_SHARE_ECHOのポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetEchoFlag(SC_DH_SHARE_ECHO *isEcho)
{
	// パラメータチェック
	if (NULL == isEcho) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[isEcho], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 位置情報共有するかどうか取得
	isEcho->isEcho = shareData->isEcho;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 位置情報共有するかどうか設定
 * @param[in] isEcho    SC_DH_SHARE_ECHOのポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetEchoFlag(const SC_DH_SHARE_ECHO *isEcho)
{
	//INT32	num = 0;
	// パラメータチェック
	if (NULL == isEcho) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[isEcho], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 位置情報共有するかどうか設定
	shareData->isEcho = isEcho->isEcho;

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief ジャンルデータ取得
 * @param[out] genredata    ジャンルデータ構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetGenreData(SC_DH_SHARE_GENREDATA *genredata)
{
	// パラメータチェック
	if (NULL == genredata) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[genredata], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// ジャンルデータ取得
	memcpy(&genredata->genreData, &shareData->genreData, sizeof(SMGENREDATA));

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief ジャンルデータ取得
 * @param[out] genredata    ジャンルデータ構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetGenreData(const SC_DH_SHARE_GENREDATA *genredata)
{
	// パラメータチェック
	if (NULL == genredata) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[genredata], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// ジャンルデータ設定
	memcpy(&shareData->genreData, &genredata->genreData, sizeof(SMGENREDATA));

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 地図ズームモード取得
 * @param[out] mode ズームモード構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetZoomMode(SC_DH_SHARE_ZOOMMODE *mode)
{
	// パラメータチェック
	if (NULL == mode) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[mode], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 地図ズームモード取得
	mode->mode = shareData->zoomMode[GetMapsIdx(mode->maps)];

#ifdef SC_SHARE_DEBUG
	// 地図ズームモード
	SC_LOG_DebugPrint(SC_TAG_DH,
			"地図ズームモード取得             maps=%d zoomMode=%d, " HERE,
			mode->maps, shareData->zoomMode[GetMapsIdx(mode->maps)]);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 地図ズームモード設定
 * @param[in] mode  ズームモード構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetZoomMode(const SC_DH_SHARE_ZOOMMODE *mode)
{
	// パラメータチェック
	if (NULL == mode) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[mode], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 地図ズームモード設定
	shareData->zoomMode[GetMapsIdx(mode->maps)] = mode->mode;

#ifdef SC_SHARE_DEBUG
	// 地図ズームモード
	SC_LOG_DebugPrint(SC_TAG_DH,
			"地図ズームモード設定             maps=%d zoomMode=%d, " HERE,
			mode->maps, shareData->zoomMode[GetMapsIdx(mode->maps)]);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief リージョン設定
 * @param[in] region  リージョン
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetRegion(const SC_DH_SHARE_REGION *region) {
	// パラメータチェック
	if (NULL == region) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[mode], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// リージョン設定
	shareData->region = region->region;

#ifdef SC_SHARE_DEBUG
	SC_LOG_DebugPrint(SC_TAG_DH, "リージョン設定             region=%d, " HERE, region->region);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief リージョン取得
 * @param[out] region  リージョン
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetRegion(SC_DH_SHARE_REGION *region) {
	// パラメータチェック
	if (NULL == region) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[mode], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// リージョン設定
	region->region = shareData->region;

#ifdef SC_SHARE_DEBUG
	SC_LOG_DebugPrint(SC_TAG_DH, "リージョン取得             region=%d, " HERE, region->region);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 言語設定
 * @param[in] language  言語
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetLanguage(const SC_DH_SHARE_LANGUAGE *language) {
	// パラメータチェック
	if (NULL == language) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[mode], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 言語設定
	shareData->language = language->language;

#ifdef SC_SHARE_DEBUG
	SC_LOG_DebugPrint(SC_TAG_DH, "言語設定             language=%d, " HERE, language->language);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 言語取得
 * @param[out] language  言語
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetLanguage(SC_DH_SHARE_LANGUAGE *language) {
	// パラメータチェック
	if (NULL == language) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[mode], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == shareData) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 言語設定
	language->language = shareData->language;

#ifdef SC_SHARE_DEBUG
	SC_LOG_DebugPrint(SC_TAG_DH, "言語取得             language=%d, " HERE, language->language);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief マッピングアラート情報設定
 * @param[in] mappingAlert   マッピングアラート情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetMappingAlert(const SC_DH_SHARE_MAPPINGALERT *mappingAlert)
{
	// パラメータチェック
	if (NULL == mappingAlert) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error, " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	shareData->alert = mappingAlert->alert;

#ifdef SC_SHARE_DEBUG
	SC_LOG_DebugPrint(SC_TAG_DH, "アラート表示情報設定	lat=%d lon=%d, " HERE,
			mappingAlert->alert.posi.latitude, mappingAlert->alert.posi.longitude);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief マッピングアラート情報取得
 * @param[out] mappingAlert  マッピングアラート情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetMappingAlert(SC_DH_SHARE_MAPPINGALERT *mappingAlert)
{
	// パラメータチェック
	if (NULL == mappingAlert) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error, " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	mappingAlert->alert = shareData->alert;

#ifdef SC_SHARE_DEBUG
	SC_LOG_DebugPrint(SC_TAG_DH, "アラート表示情報取得	lat=%d lon=%d, " HERE,
			mappingAlert->alert.posi.latitude, mappingAlert->alert.posi.longitude);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 交通情報設定
 * @param[in] traffic   交通情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_SetTraffic(const SC_DH_SHARE_TRAFFIC *traffic)
{
	// パラメータチェック
	if (NULL == traffic) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error, " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	shareData->traffic = traffic->traffic;

#ifdef SC_SHARE_DEBUG
	SC_LOG_DebugPrint(SC_TAG_DH, "交通情報設定, " HERE);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 交通情報取得
 * @param[out] traffic  交通情報
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_SHARE_GetTraffic(SC_DH_SHARE_TRAFFIC *traffic)
{
	// パラメータチェック
	if (NULL == traffic) {
		SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error, " HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	traffic->traffic = shareData->traffic;

#ifdef SC_SHARE_DEBUG
	SC_LOG_DebugPrint(SC_TAG_DH, "交通情報取得	lat=%d lon=%d, " HERE);
#endif

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief	共有メモリリストア
 * @param	[I]復帰データ
 */
static E_SC_RESULT SC_SHARE_MemRestor(SC_SHARE_DATA* aBin)
{
	//INT32 i;
	//INT32 l;
	SC_LOG_DebugPrint(SC_TAG_SHARE, SC_LOG_START);

	// チェック
	if ((NULL == shareData) || (NULL == aBin)) {
		return (e_SC_RESULT_FAIL);
	}
	// 地図表示
	memcpy(&shareData->scaleLevel[0], &aBin->scaleLevel[0], sizeof(INT32) * SC_SHARE_MAPS_NUM);
	memcpy(&shareData->scaleRange[0], &aBin->scaleRange[0], sizeof(FLOAT) * SC_SHARE_MAPS_NUM);
	memcpy(&shareData->zoomLevel[0], &aBin->zoomLevel[0], sizeof(INT32) * SC_SHARE_MAPS_NUM);
	memcpy(&shareData->bigTextAttr[0], &aBin->bigTextAttr[0], sizeof(Bool) * SC_SHARE_MAPS_NUM);
	memcpy(&shareData->bigIconAttr[0], &aBin->bigIconAttr[0], sizeof(Bool) * SC_SHARE_MAPS_NUM);
	memcpy(&shareData->showLandmark[0], &aBin->showLandmark[0], sizeof(Bool) * SC_SHARE_MAPS_NUM);
	memcpy(&shareData->dispMode[0], &aBin->dispMode[0], sizeof(INT32) * SC_SHARE_MAPS_NUM);
	// 車両情報
	memcpy(&shareData->carState[0].coord, &aBin->carState[0].coord, sizeof(SMGEOCOORD));
	memcpy(&shareData->carState[1].coord, &aBin->carState[1].coord, sizeof(SMGEOCOORD));
	// 探索条件
	memcpy(&shareData->rpOption, &aBin->rpOption, sizeof(SMRPOPTION));
	// リルート条件閾値
	memcpy(&shareData->replan, &aBin->replan, sizeof(SMREPLAN));

	SC_LOG_DebugPrint(SC_TAG_SHARE, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief	共有メモリの全初期化
 * @memo	共有メモリの全初期化を行う。
 * @memo	バックアップリストアやINIファイル読み込み前に行うこと。
 */
E_SC_RESULT SC_SHARE_MemInit()
{
	INT32 i;
	INT32 l;
	SC_LOG_DebugPrint(SC_TAG_SHARE, SC_LOG_START);

	// チェック
	if (NULL == shareData) {
		return (e_SC_RESULT_FAIL);
	}
	// 0クリア
	memset(shareData, 0, sizeof(shareData));

	// 地図表示関連
	for (i = 0; i < SC_SHARE_MAPS_NUM; i++) {
		shareData->scaleLevel[i] = 0;							// 地図表示縮尺
		shareData->scaleRange[i] = 1.0f;						// 地図表示縮尺
		shareData->zoomLevel[i] = 0;							// ズームレベル
		shareData->bigTextAttr[i] = false;						// 地図上文言拡大表示
		shareData->bigIconAttr[i] = false;						// 地図上表示アイコン拡大表示
		shareData->showLandmark[i] = false;						// ランドマーク表示
		shareData->driverMode[i] = false;						// 地図ドライバモード
		shareData->cursor[i] = false;							// 地図中心カーソル表示ステータス
		shareData->scrollMode[i] = false;						// 地図スクロールモード
		shareData->zoomMode[i] = false;							// 地図ズームモード
		for (l = 0; l < SC_SHARE_CLSCD_NUM; l++) {
			shareData->showLandmarkAttr[i].landmark[l] = false;	// ランドマーク表示属性設定
		}
		shareData->dispMode[i] = SC_MDM_HEADUP;					// 地図表示モード
	}
	// 車両情報
	for (i = 0; i < SC_SHARE_CARSTATE_NUM; i++) {
		shareData->carState[i].coord.latitude = 131535846;		// 経緯度位置座標(ini，scsdいずれも無かった場合の為に一応格納) TODO 地図から取得
		shareData->carState[i].coord.longitude = 515233692;		// 経緯度位置座標(ini，scsdいずれも無かった場合の為に一応格納) TODO 地図から取得
		shareData->carState[i].onRoad = false;					// 車両位置は道路上か否か
		shareData->carState[i].isRouteSelected = false;			// 車両位置は経路上か否か
	}
	shareData->isExistRoute = false;							// 現在ルートの探索結果の有無
	shareData->isPlanning = false;								// 探索中かどうか
	shareData->guide_status = e_GUIDE_STATUS_STOP;				// 誘導状態
	// 探索
	shareData->rpOption.rpCond = RPC_HIGHWAY;					// 探索条件
	shareData->rpOption.appendCond = 0;							// 付加条件
	shareData->rpOption.regulationType = RTU_NONE;				// 規制情報
	shareData->rpOption.vehicleType = CTY_NORMAL_CAR;			// 車両タイプ
	// シミュレート情報
	shareData->simulate.simulate = e_SIMULATE_UNAVAILABLE;		// シミュレート環境
	shareData->simulate.state = e_SIMULATE_STATE_STOP;			// シミュレート状態
	// リルート閾値
	shareData->replan.angle = 40.;								// 自車が探索ルートから離れた距離[m]
	shareData->replan.distance = 100.;							// 自車角度と最寄の探索ルートのリンクとの角度差[度]
	shareData->isDemoMode = false;								// デモモード中かどうか

	shareData->region = SYS_REGION_INIT;						// リージョン
	shareData->language = SYS_LANGUAGE_INIT;					// 言語

	SC_LOG_DebugPrint(SC_TAG_SHARE, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief	経路バックアップ処理
 */
E_SC_RESULT SC_RouteBackup() {

	SC_ROUTEBACKUP route_backup = {};
	Char dirPath[SC_MAX_PATH] = {};
	Char* rootPath = NULL;
	//UINT8* buf;
	INT32 bufSize = 0;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	do {
		if (NULL == shareData) {
			SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 保存データをテーブルに展開
		route_backup.region = shareData->region;
		route_backup.guide_status = shareData->guide_status;
		route_backup.pointNum = shareData->pointNum;
		memcpy(route_backup.point, shareData->point, sizeof(SMRPPOINT) * SC_SHARE_POINT_NUM);

		// バックアップファイルサイズ取得
		bufSize = 0;
		bufSize += sizeof(route_backup);

		// チェックサム作成
		UINT8* data = (UINT8*) &route_backup;
		if (e_SC_RESULT_SUCCESS != SC_CheckSumCalc(data + 4, (sizeof(SC_ROUTEBACKUP) - 4), &route_backup.checksum)) {
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// ファイルパス生成
		rootPath = (Char*) SC_MNG_GetApRootDirPath();
		if ('/' == rootPath[strlen(rootPath) - 1]) {
			sprintf((char*) dirPath, "%s%s%s", rootPath, &SC_TEMP_DIR[1], SC_BACKUP_FILE_NAME);
		} else {
			sprintf((char*) dirPath, "%s%s%s", rootPath, SC_TEMP_DIR, SC_BACKUP_FILE_NAME);
		}

		// ファイル書き出し
		if (-1 == SC_SHARE_WriteFile(dirPath, &route_backup, bufSize)) {
			SC_LOG_ErrorPrint(SC_TAG_SHARE, "SC_SHARE_WriteFile error, " HERE);
			ret = e_SC_RESULT_FILE_ACCESSERR;
			break;
		}
	} while (0);

#ifdef SC_SHARE_DEBUG
	SC_LOG_DebugPrint(SC_TAG_DH, "経路バックアップ作成             result=%x, " HERE, ret);
#endif
	return (ret);
}

/**
 * @brief	経路バックアップ削除処理
 */
E_SC_RESULT SC_RouteBackupDelete() {

	Char dirPath[SC_MAX_PATH] = {};
	//SC_ROUTEBACKUP route_backup = {};
	Char* rootPath = NULL;
	INT32 readRet = 0;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	do {
		if (NULL == shareData) {
			SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// ファイルパス生成
		rootPath = (Char*) SC_MNG_GetApRootDirPath();
		if ('/' == rootPath[strlen(rootPath) - 1]) {
			sprintf((char*) dirPath, "%s%s%s", rootPath, &SC_TEMP_DIR[1], SC_BACKUP_FILE_NAME);
		}else{
			sprintf((char*) dirPath, "%s%s%s", rootPath, SC_TEMP_DIR, SC_BACKUP_FILE_NAME);
		}

		// 経路バックアップファイル削除
		readRet = SC_SHARE_DeleteFile(dirPath);
		if (e_SC_RESULT_SUCCESS != readRet) {
			SC_LOG_ErrorPrint(SC_TAG_SHARE, "SC_SHARE_DeleteFile error, " HERE);
			ret = readRet;
			break;
		}
	} while (0);

#ifdef SC_SHARE_DEBUG
	SC_LOG_DebugPrint(SC_TAG_DH, "経路バックアップ削除             result=%x, " HERE, ret);
#endif
	return (ret);
}

/**
 * @brief	経路バックアップ取得処理
 */
E_SC_RESULT SC_GetRouteBackup(SMROUTEBACKUP* backup) {

	Char dirPath[SC_MAX_PATH] = {};
	SC_ROUTEBACKUP route_backup = {};
	Char* rootPath = NULL;
	INT32 readRet = 0;
	//INT32 result = 0;
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;

	do {
		if (NULL == shareData) {
			SC_LOG_ErrorPrint(SC_TAG_SHARE, "uninitialized, " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == backup) {
			SC_LOG_ErrorPrint(SC_TAG_SHARE, "badparam, " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// ファイルパス生成
		rootPath = (Char*) SC_MNG_GetApRootDirPath();
		if ('/' == rootPath[strlen(rootPath) - 1]) {
			sprintf((char*) dirPath, "%s%s%s", rootPath, &SC_TEMP_DIR[1], SC_BACKUP_FILE_NAME);
		}else{
			sprintf((char*) dirPath, "%s%s%s", rootPath, SC_TEMP_DIR, SC_BACKUP_FILE_NAME);
		}

		// 経路バックアップファイル読み込み
		readRet = SC_SHARE_ReadFile(dirPath, &route_backup, sizeof(SC_ROUTEBACKUP));
		if (-1 == readRet) {
			SC_LOG_ErrorPrint(SC_TAG_SHARE, "SC_SHARE_ReadFile error, " HERE);
			ret = e_SC_RESULT_FILE_ACCESSERR;
			break;
		} else if (0 == readRet) {
			SC_LOG_ErrorPrint(SC_TAG_SHARE, "SC_SHARE_ReadFile nodata, " HERE);
			ret = e_SC_RESULT_NODATA;
			break;
		} else if (readRet != sizeof(SC_ROUTEBACKUP)) {
			SC_LOG_ErrorPrint(SC_TAG_SHARE, "SC_SHARE_ReadFile filesize unmatch, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// チェックサム確認
		UINT8 *data = (UINT8*) &route_backup;
		if (!SC_CheckSumJudge(data + 4, sizeof(SC_ROUTEBACKUP) - 4, route_backup.checksum)) {
			SC_LOG_ErrorPrint(SC_TAG_SHARE, "SC_CheckSumJudge sumcheck error, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 地点数確認
		if (SC_SHARE_POINT_NUM < route_backup.pointNum) {
			SC_LOG_ErrorPrint(SC_TAG_SHARE, "backup route point over flow, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 保存データをテーブルに展開
		memset(backup, 0, sizeof(SMROUTEBACKUP));
		backup->guide_status = route_backup.guide_status;
		backup->pointNum = route_backup.pointNum;
		backup->region = route_backup.region;
		memcpy(backup->point, route_backup.point, sizeof(SMRPPOINT) * SC_SHARE_POINT_NUM);

	} while (0);

#ifdef SC_SHARE_DEBUG
	SC_LOG_DebugPrint(SC_TAG_DH, "経路バックアップ取得             result=%x, " HERE, ret);
#if 0
	// log
	{
		if (backup) {
			SC_LOG_DebugPrint(SC_TAG_DH, "経路バックアップファイルパス=%s", dirPath);
			SC_LOG_DebugPrint(SC_TAG_DH, "リージョン=%d", backup->region);
			SC_LOG_DebugPrint(SC_TAG_DH, "ガイド状態=%d", backup->guide_status);
			SC_LOG_DebugPrint(SC_TAG_DH, "地点数=%d", backup->pointNum);
			if (backup->pointNum < SC_SHARE_POINT_NUM) {
				int i = 0;
				for (i = 0; i < backup->pointNum; i++) {
					SC_LOG_DebugPrint(SC_TAG_DH, "[%d]", i);
					SC_LOG_DebugPrint(SC_TAG_DH, "地点名=%s", backup->point[i].nodeName);
					SC_LOG_DebugPrint(SC_TAG_DH, "地点=%d,%d", backup->point[i].coord.latitude, backup->point[i].coord.longitude);
					SC_LOG_DebugPrint(SC_TAG_DH, "条件=%d", backup->point[i].cond);
					SC_LOG_DebugPrint(SC_TAG_DH, "通過=%d", backup->point[i].isPassed);
					SC_LOG_DebugPrint(SC_TAG_DH, "タイプ=%d", backup->point[i].placeType);
					SC_LOG_DebugPrint(SC_TAG_DH, "地点インデックス=%d", backup->point[i].rpPointIndex);
				}
			}
		}
	}
#endif
#endif
	return (ret);
}

/**
 * @brief ファイル書き込み
 * @param[in] filePath データを書き込むファイルのフルパス
 * @param[in] data     書き込むデータ
 * @param[in] dataSize 書き込むデータ長
 * @memo ファイルへの書き込みを(1)～(2)の条件のいずれかで行う。
 *       ■指定されたファイルが存在する場合
 *         ①指定されたファイル名＋拡張子「.new」ファイルを新規作成する。
 *         ②①で作成したファイルにデータを書き込む。
 *         ③既存の指定されたファイルを指定されたファイル名＋拡張子「.old」にリネームする。
 *         ④①で新規作成したファイルの拡張子「.new」を取る(拡張子「.new」なしにリネームする)。
 *         ⑤③でリネームした既存ファイルを削除する。
 *       ■指定されたファイルが存在しない場合
 *         ①指定されたファイル名＋拡張子「.old」ファイルを新規作成する。
 *         ②①で作成したファイルにデータを書き込む。
 *         ③①で作成したファイルの拡張子「.old」を取る(拡張子「.old」なしにリネームする)。
 * @return 正常：書き込んだデータサイズ、異常：-1
 */
INT32 SC_SHARE_WriteFile(const Char *filePath, const void *data, INT32 dataSize)
{
	INT32	ret = -1;
	struct stat	st = {};
	INT32	fd = -1;
	INT32	size = -1;

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == filePath) {
			SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[filePath], " HERE);
			break;
		}
		if (NULL == data) {
			SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[data], " HERE);
			break;
		}

		// 既存ファイルのリネーム後のファイル名設定
		snprintf((char*)oldFilePath, (sizeof(oldFilePath) - 1), "%s%s", filePath, SC_SHARE_FILE_EXTENSION_OLD);
		// 新規作成するファイル名設定
		snprintf((char*)newFilePath, (sizeof(newFilePath) - 1), "%s%s", filePath, SC_SHARE_FILE_EXTENSION_NEW);

		// 指定されたファイル存在チェック
		if (0 == stat((char*)filePath, &st)) {
			// 指定されたファイルが存在する場合
			// 新規作成するファイル(拡張子「.new」)オープン
#ifdef __SMS_APPLE__
			fd = open((char*)newFilePath, (O_CREAT|O_WRONLY|O_TRUNC), (S_IRUSR|S_IWUSR));
#else
			fd = open((char*)newFilePath, (O_CREAT|O_WRONLY|O_TRUNC), 0666);
#endif /* __SMS_APPLE__ */
			if (-1 == fd) {
				SC_LOG_ErrorPrint(SC_TAG_SHARE, "file open(.new) error(0x%08x), " HERE, errno);
				break;
			}

			// データ書き込み
			size = write(fd, data, dataSize);
			if (-1 == size) {
				SC_LOG_ErrorPrint(SC_TAG_SHARE, "file write(.new) error(0x%08x), " HERE, errno);
				break;
			}

			// ファイルクローズ
			close(fd);
			fd = -1;

			// 既存のファイル名に拡張子「.old」を付加したファイル名にリネーム
			if (-1 == rename((char*)filePath, (char*)oldFilePath)) {
				SC_LOG_ErrorPrint(SC_TAG_SHARE, "file rename(=>.old) error(0x%08x), " HERE, errno);
				break;
			}

			// 拡張子「.new」を取ったファイル名にリネーム
			if (-1 == rename((char*)newFilePath, (char*)filePath)) {
				SC_LOG_ErrorPrint(SC_TAG_SHARE, "file rename(.new=>) error(0x%08x), " HERE, errno);
				break;
			}

			// 拡張子「.old」ファイル削除
			if (-1 == unlink((char*)oldFilePath)) {
				SC_LOG_ErrorPrint(SC_TAG_SHARE, "file unlink(.old) error(0x%08x), " HERE, errno);
				break;
			}
		} else {
			// 指定されたファイルが存在しない場合
			// 拡張子「.old」ファイルをオープン(作成する)
			fd = open((char*)oldFilePath, (O_CREAT|O_WRONLY|O_TRUNC), 0666);
			if (-1 == fd) {
				SC_LOG_ErrorPrint(SC_TAG_SHARE, "file open(.old) error(0x%08x), " HERE, errno);
				break;
			}

			// データ書き込み
			size = write(fd, data, dataSize);
			if (-1 == size) {
				SC_LOG_ErrorPrint(SC_TAG_SHARE, "file write(.old) error(0x%08x), " HERE, errno);
				break;
			}

			// ファイルクローズ
			close(fd);
			fd = -1;

			// 拡張子「.old」を取ったファイル名にリネーム
			if (-1 == rename((char*)oldFilePath, (char*)filePath)) {
				SC_LOG_ErrorPrint(SC_TAG_SHARE, "file rename(.old=>) error(0x%08x), " HERE, errno);
				break;
			}
		}
		ret = size;
	} while (0);

	if (-1 != fd) {
		// ファイルクローズ
		close(fd);
	}

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}

/**
 * @brief ファイル読み込み
 * @param[in]  filePath データを読み込むファイルのフルパス
 * @param[out] data     読み込んだデータ
 * @param[in]  dataSize 読み込むデータ長
 * @note ファイルへの読み込みを（１）～（５）の条件のいずれかで行う。
 *       （１）指定されたファイル名＋拡張子「.new」の新規ファイルが存在する場合
 *         ①指定されたファイル名＋拡張子「.new」のファイルを削除する。
 *         ②指定されたファイルを読み込む。
 *       （２）指定されたファイル名＋拡張子「.old」ファイルと拡張子「.new」ファイルが存在する場合
 *         ①指定されたファイル名＋拡張子「.new」ファイルを指定されたファイル名にリネームする。
 *         ②指定されたファイル名＋拡張子「.old」ファイルを削除する。
 *         ③指定されたファイルを読み込む。
 *       （３）指定されたファイル名＋拡張子「.old」ファイルと指定されたファイルが存在する場合
 *         ①指定されたファイル名＋拡張子「.old」ファイルを削除する。
 *         ②指定されたファイルを読み込む。
 *       （４）指定されたファイルが存在する場合
 *         ①指定されたファイルを読み込む。
 *       （５）指定されたファイルが存在しない場合
 *         ①既存の指定されたファイル名＋拡張子「.old」ファイルを削除する。
 *         ②データの読み込みなし。
 * @return 正常：読み込んだデータサイズ、異常：-1、データなし：0
 */
INT32 SC_SHARE_ReadFile(const Char *filePath, void *data, INT32 dataSize)
{
	INT32	ret = -1;
	struct stat	st = {};
	INT32	fd = -1;
	INT32	size = -1;

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == filePath) {
			SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[filePath], " HERE);
			break;
		}

		// パラメータチェック
		if (NULL == data) {
			SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[data], " HERE);
			break;
		}

		// 既存ファイルのリネーム後のファイル名設定
		snprintf((char*)oldFilePath, (sizeof(oldFilePath) - 1), "%s%s", filePath, SC_SHARE_FILE_EXTENSION_OLD);
		// 新規作成するファイル名設定
		snprintf((char*)newFilePath, (sizeof(newFilePath) - 1), "%s%s", filePath, SC_SHARE_FILE_EXTENSION_NEW);

		// 指定されたファイル存在チェック
		if (0 == stat((char*)filePath, &st)) {
			// 指定されたファイルが存在する場合
			// 指定されたファイル名＋拡張子「.new」ファイルを削除する
			unlink((char*)newFilePath);
		} else {
			// 指定されたファイルが存在しない場合
			// 指定されたファイル名＋拡張子「.new」ファイルするかチェック
			if (0 == stat((char*)newFilePath, &st)) {
				// 指定されたファイル名＋拡張子「.new」ファイルが存在する
				// 指定されたファイル名＋拡張子「.new」ファイルを指定されたファイル名にリネーム
				if (-1 == rename((char*)newFilePath, (char*)filePath)) {
					SC_LOG_ErrorPrint(SC_TAG_SHARE, "file rename(.new=>) error(0x%08x), " HERE, errno);
					break;
				}

				// 指定されたファイル名＋拡張子「.old」ファイルを削除
				if (-1 == unlink((char*)oldFilePath)) {
					SC_LOG_ErrorPrint(SC_TAG_SHARE, "file unlink(.old) error(0x%08x), " HERE, errno);
					break;
				}
			} else {
				// 指定されたファイル名＋拡張子「.new」ファイルが存在しない場合
				// 指定されたファイル名＋拡張子「.old」ファイル削除（エラーは無視する）
				unlink((char*)oldFilePath);
				*(Char*)data = EOS;
				ret = 0;
				break;
			}
		}

		// 指定されたファイルをオープン
		fd = open((char*)filePath, O_RDONLY);
		if (-1 == fd) {
			SC_LOG_ErrorPrint(SC_TAG_SHARE, "file open error(0x%08x), " HERE, errno);
			break;
		}

		// ファイルを読み込む
		size = read(fd, data, dataSize);
		if (-1 == size) {
			SC_LOG_ErrorPrint(SC_TAG_SHARE, "file read error(0x%08x), " HERE, errno);
			break;
		}
		ret = size;
	} while (0);

	if (-1 != fd) {
		// ファイルクローズ
		close(fd);
	}

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}

/**
 * @brief ファイル削除
 * @param[in]  削除するファイルパス
 * @note ファイルの削除を行う。
 *       （１）指定されたファイルが存在する場合
 *         ①指定されたファイル名のファイルを削除する。
 *       （２）指定されたファイル名＋拡張子「.new」のファイルが存在する場合
 *         ①指定されたファイル名＋拡張子「.new」のファイルを削除する。
 *       （３）指定されたファイル名＋拡張子「.old」のファイルが存在する場合
 *         ①指定されたファイル名＋拡張子「.old」ファイルを削除する。
 * @return 正常：e_SC_RESULT_SUCCESS、異常：e_SC_RESULT_SUCCESS以外
 */
E_SC_RESULT SC_SHARE_DeleteFile(const Char *filePath) {
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	struct stat st = {};

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == filePath) {
			SC_LOG_ErrorPrint(SC_TAG_SHARE, "param error[filePath], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 既存ファイルのファイル名取得
		snprintf((char*) oldFilePath, (sizeof(oldFilePath) - 1), "%s%s", filePath, SC_SHARE_FILE_EXTENSION_OLD);
		snprintf((char*) newFilePath, (sizeof(newFilePath) - 1), "%s%s", filePath, SC_SHARE_FILE_EXTENSION_NEW);

		// 指定されたファイル削除
		if (0 == stat((char*) filePath, &st)) {
			if (-1 == unlink((char*) filePath)) {
				SC_LOG_ErrorPrint(SC_TAG_SHARE, "file unlink error(0x%08x), " HERE, errno);
				ret = e_SC_RESULT_FAIL;
			}
		}
		if (0 == stat((char*) newFilePath, &st)) {
			if (-1 == unlink((char*) newFilePath)) {
				SC_LOG_ErrorPrint(SC_TAG_SHARE, "file unlink(.new) error(0x%08x), " HERE, errno);
			}
		}
		if (0 == stat((char*) oldFilePath, &st)) {
			if (-1 == unlink((char*) oldFilePath)) {
				SC_LOG_ErrorPrint(SC_TAG_SHARE, "file unlink(.old) error(0x%08x), " HERE, errno);
			}
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}


/**
 * @brief 常駐メモリに設定されているデータをすべてログ出力する
 */
void SC_SHARE_OutputAllData()
{
	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

#ifdef SC_SHARE_DEBUG
	INT32	num = 0;

	// 地図表示縮尺(スケール)
	SC_LOG_DebugPrint(SC_TAG_DH,
			"■地図表示縮尺(スケール)           maps=1 scaleLevel=%d, maps=2 scaleLevel=%d, maps=4 scaleLevel=%d, " HERE,
			shareData->scaleLevel[0], shareData->scaleLevel[1], shareData->scaleLevel[2]);

	// 地図上文言拡大表示
	SC_LOG_DebugPrint(SC_TAG_DH,
			"■地図上文言拡大表示               maps=1 bigTextAttr=%d, maps=2 bigTextAttr=%d, maps=4 bigTextAttr=%d, " HERE,
			shareData->bigTextAttr[0], shareData->bigTextAttr[1], shareData->bigTextAttr[2]);

	// 地図上文言拡大表示
	SC_LOG_DebugPrint(SC_TAG_DH,
			"■地図上表示アイコン拡大表示       maps=1 bigIconAttr=%d, maps=2 bigIconAttr=%d, maps=4 bigIconAttr=%d, " HERE,
			shareData->bigIconAttr[0], shareData->bigIconAttr[1], shareData->bigIconAttr[2]);

	// ランドマーク表示設定
	SC_LOG_DebugPrint(SC_TAG_DH,
			"■ランドマーク表示                 maps=1 showLandmark=%d, maps=2 showLandmark=%d, maps=4 showLandmark=%d, " HERE,
			shareData->showLandmark[0], shareData->showLandmark[1], shareData->showLandmark[2]);

	// ランドマーク表示属性設定
	SC_LOG_DebugPrint(SC_TAG_DH,
			"■ランドマーク表示属性１           maps=1 showLandmarkAttr=%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d, " HERE,
			shareData->showLandmarkAttr[0].landmark[0],
			shareData->showLandmarkAttr[0].landmark[1],
			shareData->showLandmarkAttr[0].landmark[2],
			shareData->showLandmarkAttr[0].landmark[3],
			shareData->showLandmarkAttr[0].landmark[4],
			shareData->showLandmarkAttr[0].landmark[5],
			shareData->showLandmarkAttr[0].landmark[6],
			shareData->showLandmarkAttr[0].landmark[7],
			shareData->showLandmarkAttr[0].landmark[8],
			shareData->showLandmarkAttr[0].landmark[9],
			shareData->showLandmarkAttr[0].landmark[10],
			shareData->showLandmarkAttr[0].landmark[11]);
	SC_LOG_DebugPrint(SC_TAG_DH,
			"■ランドマーク表示属性２           maps=2 showLandmarkAttr=%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d, " HERE,
			shareData->showLandmarkAttr[1].landmark[0],
			shareData->showLandmarkAttr[1].landmark[1],
			shareData->showLandmarkAttr[1].landmark[2],
			shareData->showLandmarkAttr[1].landmark[3],
			shareData->showLandmarkAttr[1].landmark[4],
			shareData->showLandmarkAttr[1].landmark[5],
			shareData->showLandmarkAttr[1].landmark[6],
			shareData->showLandmarkAttr[1].landmark[7],
			shareData->showLandmarkAttr[1].landmark[8],
			shareData->showLandmarkAttr[1].landmark[9],
			shareData->showLandmarkAttr[1].landmark[10],
			shareData->showLandmarkAttr[1].landmark[11]);
	SC_LOG_DebugPrint(SC_TAG_DH,
			"■ランドマーク表示属性３           maps=4 showLandmarkAttr=%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d, " HERE,
			shareData->showLandmarkAttr[2].landmark[0],
			shareData->showLandmarkAttr[2].landmark[1],
			shareData->showLandmarkAttr[2].landmark[2],
			shareData->showLandmarkAttr[2].landmark[3],
			shareData->showLandmarkAttr[2].landmark[4],
			shareData->showLandmarkAttr[2].landmark[5],
			shareData->showLandmarkAttr[2].landmark[6],
			shareData->showLandmarkAttr[2].landmark[7],
			shareData->showLandmarkAttr[2].landmark[8],
			shareData->showLandmarkAttr[2].landmark[9],
			shareData->showLandmarkAttr[2].landmark[10],
			shareData->showLandmarkAttr[2].landmark[11]);

	// 地図表示モード
	SC_LOG_DebugPrint(SC_TAG_DH,
			"■地図表示モード                   maps=1 dispMode=%d, maps=2 dispMode=%d, maps=4 dispMode=%d, " HERE,
			shareData->dispMode[0], shareData->dispMode[1], shareData->dispMode[2]);

	// 地図ドライバモード
	SC_LOG_DebugPrint(SC_TAG_DH,
			"■地図ドライバモード               maps=1 driverMode=%d, maps=2 driverMode=%d, maps=4 driverMode=%d, " HERE,
			shareData->driverMode[0], shareData->driverMode[1], shareData->driverMode[2]);

	// 地図中心カーソルの表示ステータス
	SC_LOG_DebugPrint(SC_TAG_DH,
			"■地図中心カーソルの表示ステータス maps=1 cursor=%d, maps=2 cursor=%d, maps=4 cursor=%d, " HERE,
			shareData->cursor[0], shareData->cursor[1], shareData->cursor[2]);

	// 車両状態情報
	SC_LOG_DebugPrint(SC_TAG_DH,
			"■車両状態情報                     longitude=%ld latitude=%ld speed=%f dir=%d onRoad=%d isRouteSelected=%d roadClass=%d linkId=%d, " HERE,
			shareData->carState[0].coord.longitude, shareData->carState[0].coord.latitude, shareData->carState[0].speed,
			shareData->carState[0].dir, shareData->carState[0].onRoad, shareData->carState[0].isRouteSelected,
			shareData->carState[0].roadClass, shareData->carState[0].linkId);
	// 車両状態情報
	SC_LOG_DebugPrint(SC_TAG_DH,
			"■車両状態情報                     longitude=%ld latitude=%ld speed=%f dir=%d onRoad=%d isRouteSelected=%d roadClass=%d linkId=%d, " HERE,
			shareData->carState[1].coord.longitude, shareData->carState[1].coord.latitude, shareData->carState[1].speed,
			shareData->carState[1].dir, shareData->carState[1].onRoad, shareData->carState[1].isRouteSelected,
			shareData->carState[1].roadClass, shareData->carState[1].linkId);

	// 地図スクロールモード
	SC_LOG_DebugPrint(SC_TAG_DH,
			"■地図スクロールモード             maps=1 scrollMode=%d, maps=2 scrollMode=%d, maps=4 scrollMode=%d, " HERE,
			shareData->scrollMode[0], shareData->scrollMode[1], shareData->scrollMode[2]);

	// 地図ズームモード
	SC_LOG_DebugPrint(SC_TAG_DH,
			"■地図ズームモード             maps=1 zoomMode=%d, maps=2 zoomMode=%d, maps=4 zoomMode=%d, " HERE,
			shareData->zoomMode[0], shareData->zoomMode[1], shareData->zoomMode[2]);

	// 移動角度
	SC_LOG_DebugPrint(SC_TAG_DH,
			"■移動角度                         maps=1 degreeToward=%d, maps=2 degreeToward=%d, maps=4 degreeToward=%d, " HERE,
			shareData->degreeToward[0], shareData->degreeToward[1], shareData->degreeToward[2]);

	// 移動長さ
	SC_LOG_DebugPrint(SC_TAG_DH,
			"■移動長さ                         maps=1 pixelStep=%d, maps=2 pixelStep=%d, maps=4 pixelStep=%d, " HERE,
			shareData->pixelStep[0], shareData->pixelStep[1], shareData->pixelStep[2]);

	// 地図フリーズームの拡大比例
	SC_LOG_DebugPrint(SC_TAG_DH,
			"■地図フリーズームの拡大比例       maps=1 pixelStep=%f, maps=2 pixelStep=%f, maps=4 pixelStep=%f, " HERE,
			shareData->rate[0], shareData->rate[1], shareData->rate[2]);

	// 経緯度
	SC_LOG_DebugPrint(SC_TAG_DH,
			"■経緯度                           maps=1 longitude=%ld latitude=%ld, maps=2 longitude=%ld latitude=%ld, maps=4 longitude=%ld latitude=%ld, " HERE,
			shareData->coord[0].longitude, shareData->coord[0].latitude,
			shareData->coord[1].longitude, shareData->coord[1].latitude,
			shareData->coord[2].longitude, shareData->coord[2].latitude);

	// 地図のビューポート
	SC_LOG_DebugPrint(SC_TAG_DH,
			"■地図のビューポート1              maps=1 left=%d right=%d top=%d bottom=%d, " HERE,
			shareData->rect[0].left, shareData->rect[0].right, shareData->rect[0].top, shareData->rect[0].bottom);
	SC_LOG_DebugPrint(SC_TAG_DH,
			"■地図のビューポート2              maps=2 left=%d right=%d top=%d bottom=%d, " HERE,
			shareData->rect[1].left, shareData->rect[1].right, shareData->rect[1].top, shareData->rect[1].bottom);
	SC_LOG_DebugPrint(SC_TAG_DH,
			"■地図のビューポート3              maps=4 left=%d right=%d top=%d bottom=%d, " HERE,
			shareData->rect[2].left, shareData->rect[2].right, shareData->rect[2].top, shareData->rect[2].bottom);

	// 地図の回転角度
	SC_LOG_DebugPrint(SC_TAG_DH,
			"■地図の回転角度                   maps=1 rotate=%d, maps=2 rotate=%d, maps=4 rotate=%d, " HERE,
			shareData->rotate[0], shareData->rotate[1], shareData->rotate[2]);

	// スケール
	SC_LOG_DebugPrint(SC_TAG_DH,
			"■スケール                         maps=1 scale=%f, maps=2 scale=%f, maps=4 scale=%f, " HERE,
			shareData->scale[0], shareData->scale[1], shareData->scale[2]);

	// 解像度
	SC_LOG_DebugPrint(SC_TAG_DH,
			"■解像度                           width=%d height=%d, " HERE,
			shareData->width, shareData->height);
	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	// 地点情報（出発地、経由地、目的地）
	for (num = 0; num < shareData->pointNum; num++) {
		SC_LOG_DebugPrint(SC_TAG_DH,
				"■地点情報%d                       longitude=%ld latitude=%ld nodeName=%s placeType=%d cond=%d isPassed=%d rpPointIndex=%ld, " HERE,
				(num + 1), shareData->point[num].coord.longitude, shareData->point[num].coord.latitude,
				shareData->point[num].nodeName, shareData->point[num].rpPointType, shareData->point[num].cond,
				shareData->point[num].isPassed, shareData->point[num].rpPointIndex);
	}

	// 探索結果の有無
	SC_LOG_DebugPrint(SC_TAG_DH,
			"■探索結果の有無                   isExistRoute=%d, " HERE,
			shareData->isExistRoute);

	// 探索中かどうか
	SC_LOG_DebugPrint(SC_TAG_DH,
			"■探索中かどうか                   isPlanning=%d, " HERE,
			shareData->isPlanning);

	// 探索条件
	SC_LOG_DebugPrint(SC_TAG_DH,
			"■探索条件                         rpCond=%d height=%ld regType=%d vehicleType=%d tollType=%d, " HERE,
			shareData->rpOption.rpCond, shareData->rpOption.appendCond,
			shareData->rpOption.regulationType, shareData->rpOption.vehicleType,
			shareData->rpOption.tollType);

	// 探索詳細エラー情報
	SC_LOG_DebugPrint(SC_TAG_DH,
			"■探索詳細エラー情報               tipClass=%d tipIndex=%d isRePlan=%d appendOption=%ld, " HERE,
			shareData->tipInfo.tipClass, shareData->tipInfo.tipIndex,
			shareData->tipInfo.isRePlan, shareData->tipInfo.appendOption);

	// シミュレータ環境かどうか
	SC_LOG_DebugPrint(SC_TAG_DH,
			"■シミュレータ環境取得             simulate=%d, " HERE,
			shareData->simulate.simulate);
	// シミュレータ速度
	SC_LOG_DebugPrint(SC_TAG_DH,
			"■シミュレータ速度取得             speed=%d, " HERE,
			shareData->simulate.speed);
	// シミュレータ状態
	SC_LOG_DebugPrint(SC_TAG_DH,
			"■シミュレータ状態取得             state=%d, " HERE,
			shareData->simulate.state);

	// リアルタイム案内情報
	SC_LOG_DebugPrint(SC_TAG_DH,
			"■リアルタイム案内情報設定         turnDir=%d nextTurn=%ld trafficLight=%d rtToNextPlace=%ld destination=%d bypass=%d rdToNextPlace=%ld nextBroadString=%s roadLaneNum=%d, " HERE,
			shareData->guideData.turnDir, shareData->guideData.remainDistToNextTurn,
			shareData->guideData.showTrafficLight, shareData->guideData.remainTimeToNextPlace,
			shareData->guideData.destination, shareData->guideData.bypass,
			shareData->guideData.remainDistToNextPlace, shareData->guideData.nextBroadString,
			shareData->guideData.roadLaneNum);
	for (num = 0; num < shareData->guideData.roadLaneNum; num++) {
		SC_LOG_DebugPrint(SC_TAG_DH,
				"■リアルタイム案内情報設定 車線%d   nextBroadString=%s laneHightLight=%d advisableLaneFlag=%d, " HERE,
					(num + 1), shareData->guideData.roadLane[num].laneFlag,
					shareData->guideData.roadLane[num].laneHightLight, shareData->guideData.roadLane[num].advisableLaneFlag);
	}
	SC_LOG_DebugPrint(SC_TAG_DH,
			"■リアルタイム案内情報設定          passedDistance=%ld aheadPoint=%d valid=%d graphMaxShowDist=%d roadType=%d roadSituation=%d nextBypassIndex, " HERE,
			shareData->guideData.passedDistance, shareData->guideData.aheadPoint,
			shareData->guideData.valid, shareData->guideData.graphMaxShowDist,
			shareData->guideData.roadType, shareData->guideData.roadSituation,
			shareData->guideData.nextBypassIndex);
	for (num = 0; num < shareData->guideData.roadLaneAtGuidePointNum; num++) {
		SC_LOG_DebugPrint(SC_TAG_DH,
				"■リアルタイム案内情報設定 案内点%d nextBroadString=%s laneHightLight=%d advisableLaneFlag=%d, " HERE,
				(num + 1), shareData->guideData.roadLaneAtGuidePoint[num].laneFlag,
				shareData->guideData.roadLaneAtGuidePoint[num].laneHightLight, shareData->guideData.roadLaneAtGuidePoint[num].advisableLaneFlag);
	}
#endif	// #ifdef SC_SHARE_DEBUG

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);
}
