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
 * SMCoreTRCom.c
 *
 *  Created on: 2016/01/04
 *      Author: n.kanagawa
 */
#include "sms-core/SMCoreTR/SMCoreTRInternal.h"


// プロトタイプ
static void setTrafficSearchStruct(const TR_PARCEL_LIST_t *pList, SMTRAFFICSRCH *pTrafficSrch);
static E_SC_RESULT analyzeFile(char* pFile, TR_TRAFFIC_LIST_t *pTraffic);


/**
 * @brief	交通情報取得(センタ通信)
 * 			入力パーセル数と出力パーセル数は必ずしも一致しない
 * @param	[I]pParcelList		要求パーセルリスト
 * @param	[O]pTraffic			取得交通情報リスト
 */
E_SC_RESULT SC_TR_ComTraffic(const TR_PARCEL_LIST_t *pList, TR_TRAFFIC_LIST_t *pTraffic)
{
	E_SC_RESULT		ret = e_SC_RESULT_SUCCESS;
	SMTRAFFICSRCH	trafficSrch = {};
	SMTRAFFICINFO	trafficInfo = {};

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_START);

	do {
		// 初期化
		pTraffic->cnt = 0;

#ifdef _TR_DEBUG
		INT32 i = 0;
		for (i=0; i<pList->cnt; i++) {
			SC_LOG_DebugPrint(SC_TAG_TR, (Char*)"■Req Trafficinfo[%d] ParcelID=%08x",
					i+1, pList->pcl[i]);
		}
#endif

		// 交通情報検索条件の構造体設定
		setTrafficSearchStruct(pList, &trafficSrch);

		// 交通情報取得
		ret = SCC_GetTrafficInfo(&trafficSrch, &trafficInfo);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_TR, (Char*)"SCC_GetTrafficInfo error, " HERE);
			break;
		}
		if (0 == trafficInfo.fileSize) {
			ret = e_SC_RESULT_SUCCESS;
			break;
		}

		// ファイル解析
		ret = analyzeFile(trafficInfo.filePath, pTraffic);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_TR, (Char*)"analyzeFile error, " HERE);
			break;
		}

#ifdef _TR_DEBUG
		for (i=0; i<pTraffic->cnt; i++) {
			SC_LOG_DebugPrint(SC_TAG_TR, (Char*)"■Get TrafficInfo[%d] ParcelID=%08x, Size=%d",
					i+1, pTraffic->trf[i].pid, pTraffic->trf[i].data.size);
		}
#endif
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_END);

	return (ret);
}

/**
 * @brief	交通情報検索条件の構造体設定
 * @param	[I]pList			要求パーセルリスト
 * @param	[O]pTrafficSrch		交通情報検索条件の構造体
 */
static void setTrafficSearchStruct(const TR_PARCEL_LIST_t *pList, SMTRAFFICSRCH *pTrafficSrch)
{
	INT32 i = 0;

	// パーセルID数
	pTrafficSrch->parcelIdNum = pList->cnt;
	// パーセルID
	for (i=0; i<pList->cnt; i++) {
		pTrafficSrch->parcelId[i] = pList->pcl[i];
	}

	// 道路リンクレベル
	pTrafficSrch->linkLv = 1;

	// 道路種別要求数 ※0固定全て取得
	pTrafficSrch->roadKindNum = 0;


	// 提供情報源ID ※0：プローブ固定
	pTrafficSrch->srcId = 0;

	// リージョン ※0：日本固定
	pTrafficSrch->rgn = 0;

	// 対象日時（UNIX時間）
	pTrafficSrch->time = 0;

	// 渋滞情報
	pTrafficSrch->jam = true;
	// SA・PA情報
	pTrafficSrch->sapa = false;
	// 駐車場情報
	pTrafficSrch->park = false;
	// 事象・規制情報
	pTrafficSrch->reg = false;

	// 地図バージョン
	if (SC_DA_RES_SUCCESS != SC_DA_GetSystemMapVerNoData(&pTrafficSrch->mapVer[0])) {
		SC_LOG_ErrorPrint(SC_TAG_TR, "SC_DA_GetSystemMapVerNoData Error " HERE);
		pTrafficSrch->mapVer[0] = '\0';
	} else {
		if (SC_DA_RES_SUCCESS != SC_DA_GetSystemMapBuildNoData(&pTrafficSrch->mapVer[8])) {
			SC_LOG_ErrorPrint(SC_TAG_TR, "SC_DA_GetSystemMapBuildNoData Error " HERE);
		}
	}
}

/**
 * @brief	取得バイナリからパーセルデータ取得
 * @param	[I]pFile		ファイルパス
 * @param	[O]pTraffic		取得交通情報リスト
 */
static E_SC_RESULT analyzeFile(char* pFile, TR_TRAFFIC_LIST_t *pTraffic)
{
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	UINT32		parcelCnt = 0;
	UINT32		i = 0;
	UINT32		size = 0;
	FILE		*pF = NULL;
	//char*		pBuf = NULL;
	char		data[TR_DATA_HEAD_SIZE] = {};

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_START);

	// 初期化
	pTraffic->cnt = 0;

	// ファイルオープン
	pF = fopen(pFile, "rb");
	if (NULL == pF) {
		SC_LOG_ErrorPrint(SC_TAG_TR, (Char*)"fopen %s", pFile);
		return (e_SC_RESULT_FAIL);
	}

	// 交通情報バイナリデータヘッダ部読み込み
	fread(data, 1, TR_DATA_HEAD_SIZE, pF);
	// パーセルの並び先頭までシーク
	fseek(pF, TR_DATA_HEAD_SIZE, SEEK_SET);

	// パーセル数
	parcelCnt = TR_PCL_CNT(data);

	// パーセルID数分ループ
	for (i=0; i<parcelCnt; i++) {

		// パーセルデータサイズ取得
		fread(&size, sizeof(UINT32), 1, pF);
		size = CONVERT_ENDIAN_32(size);
		if (0 == size) {
			SC_LOG_ErrorPrint(SC_TAG_TR, (Char*)"parcel data size 0, " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}
		// カレントファイルポインタから戻す
		fseek(pF, -sizeof(UINT32), SEEK_CUR);

		// パーセルサイズ分メモリ確保
		char* pBuf = (char*)SC_TR_Malloc(size);
		if (NULL == pBuf) {
			SC_LOG_ErrorPrint(SC_TAG_TR, (Char*)"SC_TR_Malloc err %d, " HERE, size);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// 確保領域にデータ読み込み
		fread(pBuf, 1, size, pF);

		// データ設定
		pTraffic->trf[pTraffic->cnt].data.size = size;
		pTraffic->trf[pTraffic->cnt].data.pData = pBuf;
		pTraffic->trf[pTraffic->cnt].pid = TR_PCL_ID(pBuf);
		pTraffic->cnt++;
	}

	// ファイルクローズ
	if (NULL != pF) {
		fclose(pF);
	}

	SC_LOG_DebugPrint(SC_TAG_TR, SC_LOG_END);

	return (ret);
}
