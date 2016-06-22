/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

/*-------------------------------------------------------------------
 * File：SMMAL.c
 * Info：RMRC共通ライブラリ
 *-------------------------------------------------------------------*/
#include "SMCoreDALInternal.h"

static UINT16 MA_BinSearchNwLinkID(MAL_HDL aRoad, UINT32 aLinkId);
static UINT16 MA_BinSearchNwConnectID(MAL_HDL aRoad, UINT32 aLinkId);

/**
 * @brief ネットワークデータ索引からリンクID検索
 * @param 道路ネットワークバイナリ
 * @param 検索対象リンクID
 * @param リンクID検索：SC_MA_BINSRC_TYPE_LINK
 *        接続ID検索  ：SC_MA_BINSRC_TYPE_CNCT
 * @return MA_ALLF16以外：検索結果インデックス
 * @note aLinkIdはパーマネントID部でマスクして検索を行う。
 */
UINT16 SC_MA_BinSearchNwRecord(MAL_HDL aRoad, UINT32 aLinkId, INT32 aType) {
	UINT16 resultIdx = MA_ALLF16;

	if (NULL == aRoad) {
		return (MA_ALLF16);
	}

	switch (aType) {
	case SC_MA_BINSRC_TYPE_LINK:
		resultIdx = MA_BinSearchNwLinkID(aRoad, aLinkId);
		break;
	case SC_MA_BINSRC_TYPE_CNCT:
		resultIdx = MA_BinSearchNwConnectID(aRoad, aLinkId);
		break;
	default:
		return (resultIdx);
	}

	// 0 無効値
	if (0 == resultIdx || MA_ALLF16 == resultIdx) {
		return (MA_ALLF16);
	}
	return (resultIdx);
}

/**
 * @brief リンクID索引バイナリ検索
 * @param aRoad 道路ネットワーク
 * @param aLinkId 検索ID
 * @return MA_ALLF16以外：検索結果リンクINDEX(1始まり)
 */
static UINT16 MA_BinSearchNwLinkID(MAL_HDL aRoad, UINT32 aLinkId) {

	if (NULL == aRoad) {
		return (MA_ALLF16);
	}

	MAL_HDL pNet = SC_MA_A_NWBIN_GET_NWRCD_LINK(aRoad);			// リンクID先頭
	MAL_HDL pNetIdx = SC_MA_A_NWBIN_GET_IDXLINK(aRoad);			// リンクID索引先頭
	MAL_HDL pLink = NULL;
	UINT32 size = SC_MA_D_NWRCD_IDXLINK_GET_VOL(pNetIdx);		// リンクID索引数
	INT32 ans = 0;
	INT16 left = 0;
	INT16 right = 0;
	INT16 mid = 0;
	UINT16 resultIdx = 0;

	if (0 == size) {
		return (MA_ALLF16);
	}

	right = size - 1;
	while (left <= right) {
		mid = (left + right) / 2;
		resultIdx = read2byte(pNetIdx + 8 + mid * 2);
		pLink = SC_MA_A_NWRCD_LINK_GET_RECORD(pNet, (resultIdx - 1));
		ans = SC_MA_D_NWID_GET_PNT_ID(SC_MA_D_NWRCD_LINK_GET_ID(pLink)) - SC_MA_D_NWID_GET_PNT_ID(aLinkId);
		if (0 == ans) {
			return (resultIdx);
		} else if (ans < 0) {
			left = mid + 1;
		} else {
			right = mid - 1;
		}
	}
	return (MA_ALLF16);
}

/**
 * @brief 接続ID索引バイナリ検索
 * @param aRoad 道路ネットワーク
 * @param aLinkId 検索ID
 * @return MA_ALLF16以外：検索結果リンクINDEX(1始まり)
 */
static UINT16 MA_BinSearchNwConnectID(MAL_HDL aRoad, UINT32 aLinkId) {

	if (NULL == aRoad) {
		return (MA_ALLF16);
	}

	MAL_HDL pConnectIdx = SC_MA_A_NWBIN_GET_IDXCNCT(aRoad);			// 接続ID索引
	MAL_HDL pConnectTop = SC_MA_A_NWBIN_GET_NWRCD_CNCT(aRoad);		// 接続ID先頭
	MAL_HDL pLink = NULL;
	UINT32 size = SC_MA_D_NWRCD_IDXCNCT_GET_VOL(pConnectIdx);		// 接続ID索引数
	INT16 left = 0;
	INT16 right = 0;
	INT16 mid = 0;
	INT32 ans = 0;
	UINT16 resultIdx = 0;

	if (0 == size) {
		return (MA_ALLF16);
	}

	right = size - 1;
	while (left <= right) {
		mid = (left + right) / 2;
		resultIdx = read2byte(pConnectIdx + 8 + mid * 2);
		pLink = SC_MA_A_NWRCD_CNCT_GET_RECORD(pConnectTop, (resultIdx - 1));
		ans = SC_MA_D_NWID_GET_PNT_ID(SC_MA_D_NWRCD_CNCT_GET_ID(pLink)) - SC_MA_D_NWID_GET_PNT_ID(aLinkId);
		if (0 == ans) {
			return (resultIdx);
		} else if (ans < 0) {
			left = mid + 1;
		} else {
			right = mid - 1;
		}
	}
	return (MA_ALLF16);
}

/**
 * @brief 形状データリンクID検索
 * @param 形状データ先頭
 * @param 検索ID
 * @param 検索結果オフセット値(*4済み)
 */
E_SC_RESULT SC_MA_BinSearchShapeIndex(MAL_HDL aShape, UINT32 key, UINT32* aOffset) {

	MAL_HDL shapeRec = NULL;
	MAL_HDL index = NULL;
	MAL_HDL indexLink = NULL;
	UINT32 size = 0;
	INT32 left = 0;
	INT32 right = 0;
	INT32 mid = 0;
	INT32 ans = 0;
	UINT32 ofs = 0;
	UINT32 resultOfs = MA_ALLF32;

	// パラメータチェック
	if (NULL == aShape || NULL == aOffset) {
		SC_LOG_ErrorPrint(SC_TAG_DAL, "param error. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	if (MA_ALLF32 == SC_MA_D_SHBIN_GET_INDEX_LINK_OFS(aShape) ) {
		SC_LOG_ErrorPrint(SC_TAG_DAL, "param error. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 形状
	shapeRec = SC_MA_GetMapSharpRecord(aShape);
	if (NULL == shapeRec) {
		SC_LOG_ErrorPrint(SC_TAG_DAL, "param error. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	// 索引
	index = SC_MA_A_SHBIN_GET_INDEX_LINK(aShape);
	indexLink = index + 8;
	// 索引数
	size = read4byte(index + 4);
	right = size - 1;

	while (left <= right) {
		mid = (left + right) / 2;

		// ID比較
		ofs = read4byte(indexLink + (mid * 4));
		ans = (SC_MA_D_NWID_GET_PNT_ID(SC_MA_D_SHRCD_GET_LINKID(shapeRec + ofs * 4))) - SC_MA_D_NWID_GET_PNT_ID(key);

		if (0 == ans) {
			resultOfs = ofs * 4;
			break;
		} else if (ans < 0) {
			left = mid + 1;
		} else {
			right = mid - 1;
		}
	}
	if (MA_ALLF32 != resultOfs) {
		*aOffset = resultOfs;
	} else {
		*aOffset = resultOfs;
		return (e_SC_RESULT_FAIL);
	}

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 形状データ上位リンクID検索
 * @param 形状データ先頭
 * @param 検索ID
 * @param [O]検索結果インデックス値
 */
E_SC_RESULT SC_MA_BinSearchShapeUpperIndex(UINT8* aShape, UINT32 aKey, UINT32* aIndex) {

	if (NULL == aShape || NULL == aIndex) {
		SC_LOG_ErrorPrint(SC_TAG_DAL, "param error. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	MAL_HDL indexUpper = NULL;
	MAL_HDL indexUpperLink = NULL;
	UINT32 resultIdx = MA_ALLF32;
	INT32 left = 0;
	INT32 right = 0;
	INT32 mid = 0;
	INT32 ans = 0;

	// 上位リンクID索引レコードがない場合
	if (MA_ALLF32 == SC_MA_D_SHBIN_GET_LV2UPPER_IDXLINK_OFS(aShape) ) {
		SC_LOG_ErrorPrint(SC_TAG_DAL, "param error. "HERE);
		return (e_SC_RESULT_BADPARAM);
	}

	// 上位リンクID索引レコード
	indexUpper = SC_MA_A_SHBIN_GET_LV2UPPER_IDXLINK(aShape);
	indexUpperLink = indexUpper;

	// リンク数-1が右端
	right = read4byte(indexUpper + 4) - 1;
	left = 0;

	while (left <= right) {
		mid = (left + right) / 2;

		// ID比較
		ans = (SC_MA_D_NWID_GET_PNT_ID(SC_MA_GET_SHBIN_IDXUPLINK_ID(indexUpperLink, mid))) - SC_MA_D_NWID_GET_PNT_ID(aKey);

		if (0 == ans) {
			resultIdx = mid;
			break;
		} else if (ans < 0) {
			left = mid + 1;
		} else {
			right = mid - 1;
		}
	}
	if (MA_ALLF32 != resultIdx) {
		*aIndex = resultIdx;
	} else {
		*aIndex = resultIdx;
		return (e_SC_RESULT_FAIL);
	}

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief	形状情報
 * @param	[I]地図アドレス
 * @return	形状レコード先頭
 */
MAL_HDL SC_MA_GetMapSharpRecord(MAL_HDL aBin)
{
	MAL_HDL bin;
	UINT8 rlp;
	UINT32 ofs = MA_ALLF32;

	// パラメータチェック
	if (NULL == aBin) {
		return (NULL);
	}
	bin = SC_MA_A_SHBIN_GET_DIR(aBin);
	for (rlp = 0; rlp < 16; rlp++) {
		ofs = read4byte(bin);
		move4byte(bin);
		if (MA_ALLF32 != ofs) {
			break;
		}
	}
	if (MA_ALLF32 == ofs) {
		return (NULL);
	}
	// バイナリ先頭取得
	return (SC_MA_A_RSHP_GET_BINARY(aBin) + (ofs * 4));
}

/**
 * @brief	誘導データ索引からリンクID検索
 * @param	[I]検索対象リンクID
 * @param	[I]誘導バイナリデータ先頭
 * @return	ALLF32		オフセット未発見
 * @return	ALLF32以外	オフセット
 */
UINT32 SC_MA_GetGuideLinkOffset(UINT32 link_id, UINT8 *data_p)
{

	UINT8			*binlink;
	UINT8			*stRecord;
	UINT16			vol;
	UINT32			ans;

	if (NULL == data_p){
		return (MA_ALLF32);
	}

	// リンクID索引レコード先頭アドレス取得
	binlink = SC_MA_A_GDBIN_GET_IDXLINK(data_p);

	// リンクID索引レコード数取得
	vol = SC_MA_D_IDXLINK_GET_VOL(binlink);

	// リンクID索引データ先頭アドレス取得
	stRecord = SC_MA_A_IDXLINK_GET_RECORD(binlink);

	// リンクオフセット検索
	ans = MA_BinSearchOffset((const UINT32*) stRecord, vol, link_id, SC_MA_A_GDBIN_GET_GDRCD(data_p));
	if (MA_ALLF32 == ans) {
		return (MA_ALLF32);
	}

	return (ans);
}

/*
 * @brief	Index bin
 * @param	検索先頭
 * @param	検索テーブル数
 * @param	キー
 * @param	データ先頭
 * @return	ALLF32：検索失敗
 * @return	ALLF32以外：インデックス値（１始まり）
 */
UINT32 MA_BinSearchOffset(const UINT32* p, UINT32 count, UINT32 key, UINT8* aLinkBin)
{
	INT16 left = 0;
	INT16 right;
	INT16 mid;
	UINT32 ofs;
	INT32 ans;
	MAL_HDL linkBin;				// リンクデータ

	// パラメータチェック
	if ((NULL == p) || (NULL == aLinkBin)) {
		return (MA_ALLF32);
	}
	// 索引データなし
	if (0 > count) {
		return (MA_ALLF32);
	}
	right = count - 1;

	while (left <= right) {
		mid = (left + right) / 2;
		ofs = *(p + mid);

		linkBin = SC_MA_A_GDRCD_GET_GDLINK(aLinkBin, ofs);
		ans = (SC_MA_NWID_PNT_ID & SC_MA_D_GDLINK_GET_LINKID(linkBin) ) - (key & SC_MA_NWID_PNT_ID);

		if (0 == ans) {
			return (ofs);
		} else if (ans < 0) {
			left = mid + 1;
		} else {
			right = mid - 1;
		}
	}

	return (MA_ALLF32);
}

