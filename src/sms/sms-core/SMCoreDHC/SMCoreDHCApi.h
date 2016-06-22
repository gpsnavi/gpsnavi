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
 * SMCoreDHCApi.h
 *
 *  Created on: 2015/11/05
 *      Author: masutani
 */

#ifndef SMCOREDHCAPI_H_
#define SMCOREDHCAPI_H_

#ifdef __cplusplus
extern "C" {
#endif
/**
 * DHC_Cash
 */
// 地図読みキャッシュ初期処理（バッファ確保含む）
E_SC_RESULT SC_DHC_CashInit();
// 地図読みキャッシュ終了処理
E_DHC_CASH_RESULT SC_DHC_CashFinalize();
// 地図読み
E_DHC_CASH_RESULT SC_DHC_MapRead(T_DHC_REQ_PARCEL *aReqPcl, T_DHC_RES_DATA *aResData);
// 地図開放（メモリの開放とは意味が違う）
E_DHC_CASH_RESULT SC_DHC_MapFree(T_DHC_REQ_PARCEL *aReqPcl);
// 地図開放拡張版（種別指定・ユーザ指定）
E_DHC_CASH_RESULT SC_DHC_MapFreeEx(T_DHC_REQ_INFO aReq, INT32 aType);
E_DHC_CASH_RESULT SC_DHC_MapWatch(T_DHC_REQ_INFO aReq, void** aBin);
// レコードサイズ取得
E_DHC_CASH_RESULT SC_DHC_GetPclDataSize(T_DHC_REQ_INFO* aReq, UINT32* aSize);
// 道路密度データ取得
E_DHC_CASH_RESULT SC_DHC_GetRoadDensity(T_DHC_ROAD_DENSITY* aDenInfo);
// ダウンロードエリア情報取得
E_DHC_CASH_RESULT SC_DHC_GetDownload_Area(T_DHC_DOWNLOAD_AREA *aDownloadArea, UINT8 kind);
// ダウンロードエリア名称取得
E_DHC_CASH_RESULT SC_DHC_GetDownload_AreaName(T_DHC_DOWNLOAD_AREA_NAME *aDownloadAreaName);
// 地図データ種別のマスクを取得
UINT32 SC_DHC_GetKindMask(UINT8 kind);
// 地域クラスコード取得
E_DHC_CASH_RESULT SC_DHC_GetAreaClsCode(T_DHC_AREA_CLS_CODE* pAreaClsCode);
#ifdef __cplusplus
}
#endif

#endif /* SMCOREDHCAPI_H_ */
