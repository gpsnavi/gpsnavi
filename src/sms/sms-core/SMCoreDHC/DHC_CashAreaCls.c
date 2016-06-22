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
 * 地域クラスデータ読み込み
 */

#include "SMCoreDHCInternal.h"


/*-------------------------------------------------------------------
 * 定数定義
 *-------------------------------------------------------------------*/
// 4 bytes alignment
#define DHC_ALIGNMENT4(_x)			(((_x)+3) & ~3)


/*-------------------------------------------------------------------
 * 構造体定義
 *-------------------------------------------------------------------*/
// 座標
typedef struct _T_POINT {
	INT16 x;
	INT16 y;
} T_POINT;

// バイナリヘッダ	2byte
typedef union _BIT_BINARY_HEAD {
	struct{
		UINT32	reserve				: 15;	// bit  0～14 - リザーブ
		UINT32	rec_flg				:  1;	// bit 15     - 背景地域クラスデータレコード有無フラグ
	} b;
	UINT16 d;
} BIT_BINARY_HEAD;

// 図形タイプ等	2byte
typedef union _BIT_SHAPE_TYPE {
	struct{
		UINT32	area1_pos_flg		:  1;	// bit  0     - 地域クラス1情報 収録位置情報
		UINT32	area1_flg			:  1;	// bit  1     - 地域クラス1情報 情報存在数
		UINT32	area2_pos_flg		:  1;	// bit  2     - 地域クラス2情報 収録位置情報
		UINT32	area2_flg			:  1;	// bit  3     - 地域クラス2情報 情報存在数
		UINT32	area3_pos_flg		:  1;	// bit  4     - 地域クラス3情報 収録位置情報
		UINT32	area3_flg			:  1;	// bit  5     - 地域クラス3情報 情報存在数
		UINT32	area4_pos_flg		:  1;	// bit  6     - 地域クラス4情報 収録位置情報
		UINT32	area4_flg			:  1;	// bit  7     - 地域クラス4情報 情報存在数
		UINT32	area5_pos_flg		:  1;	// bit  8     - 地域クラス5情報 収録位置情報
		UINT32	area5_flg			:  1;	// bit  9     - 地域クラス5情報 情報存在数
		UINT32	area6_pos_flg		:  1;	// bit 10     - 地域クラス6情報 収録位置情報
		UINT32	area6_flg			:  1;	// bit 11     - 地域クラス6情報 情報存在数
		UINT32	reserve				:  2;	// bit 12～13 - リザーブ
		UINT32	shape_type			:  2;	// bit 14～15 - 図形タイプ
	} b;
	UINT16 d;
} BIT_SHAPE_TYPE;

// 図形情報	2byte
typedef union _BIT_SHAPE_INFO {
	struct{
		UINT32	reserve2			:  8;	// bit  7～ 0 - 予約2
		UINT32	primitive_kind		:  4;	// bit 11～ 8 - 描画プリミティブ種別
		UINT32	reserve				:  2;	// bit 13～12 - 予約1
		UINT32	express_info		:  1;	// bit 14     - 表現付加情報
		UINT32	data_form			:  1;	// bit 15     - データ形式
	} b;
	UINT16 d;
} BIT_SHAPE_INFO;


/*-------------------------------------------------------------------
 * プロトタイプ
 *-------------------------------------------------------------------*/
E_DHC_CASH_RESULT DHC_GetAreaClsCode(char* pBin, T_DHC_AREA_CLS_CODE* pAreaClsCode);
void DHC_GetDefAreaClsCode(char* pBin, SMAREACLSCODE* pAreaClsCode);
Bool DHC_SearchAreaClsCode(char* pBin, T_DHC_AREA_CLS_CODE* pAreaClsCode);
void DHC_GetAreaClsCodeOfBkgd(char* pBkgd, SMAREACLSCODE* pAreaClsCode);
Bool DHC_JudgeInOut(char* pShape, INT16 searchX, INT16 searchY);
Bool DHC_IsCrossX(T_POINT p, T_POINT p1, T_POINT p2);
//Bool DHC_InPolygon(T_POINT *pPos, T_POINT *pPoly, INT32 polyCnt);


/**
 * @brief	地域クラスコード取得
 * @param	[I/O]pAreaClsCode:地域クラスコード
 */
E_DHC_CASH_RESULT SC_DHC_GetAreaClsCode(T_DHC_AREA_CLS_CODE* pAreaClsCode)
{

	E_DHC_CASH_RESULT result = e_DHC_RESULT_CASH_SUCCESS;
	T_DHC_REQ_PARCEL mapReqPcl = {};
	T_DHC_RES_DATA mapResData = {};

	SC_LOG_DebugPrint(SC_TAG_DHC, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == pAreaClsCode) {
			SC_LOG_ErrorPrint(SC_TAG_DHC, "param. error " HERE);
			result = e_DHC_RESULT_CASH_FAIL;
			break;
		}

		// 地図要求情報設定
		mapReqPcl.user = SC_DHC_USER_DH;
		mapReqPcl.parcelNum = 1;
		mapReqPcl.parcelInfo[0].parcelId = pAreaClsCode->parcelId;
		mapReqPcl.parcelInfo[0].mapKind = SC_DHC_GetKindMask(e_DATA_KIND_BKGD_AREA_CLS);

		// 地図要求
		result = SC_DHC_MapRead(&mapReqPcl, &mapResData);
		if (e_DHC_RESULT_CASH_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_DHC, "SC_DHC_MapRead. error " HERE);
			break;
		}

		if (NULL == mapResData.parcelBin[0].binBkgdAreaCls) {
			SC_LOG_DebugPrint(SC_TAG_DHC, "SC_DHC_GetAreaClsCode. nodata " HERE);
			result = e_DHC_RESULT_CASH_SUCCESS;
			break;
		}

		// 地域クラスコード取得
		result = DHC_GetAreaClsCode(mapResData.parcelBin[0].binBkgdAreaCls, pAreaClsCode);
		if (e_DHC_RESULT_CASH_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_DHC, "DHC_GetAreaClsCode. error " HERE);
			break;
		}

	} while (0);

	// 地図解放
	if (NULL != mapResData.parcelBin[0].binBkgdAreaCls) {
		SC_DHC_MapFree(&mapReqPcl);
	}

	SC_LOG_DebugPrint(SC_TAG_DHC, SC_LOG_END);

	return (result);
}

/**
 * @brief	地域クラスコード取得
 * @param	[I]pBin:地域クラス背景先頭アドレス
 * @param	[I]pAreaClsCode:地域クラスコード
 */
E_DHC_CASH_RESULT DHC_GetAreaClsCode(char* pBin, T_DHC_AREA_CLS_CODE* pAreaClsCode)
{
	char* pPos = NULL;
	//char* pBkgd = NULL;
	//INT32 i = 0;
	//Bool find = false;
	BIT_BINARY_HEAD binHead;


	// バイナリデータ先頭(+ボリューム情報)
	pPos = pBin + 4;

	// バイナリヘッダ
	binHead.d = read2byte(pPos);
	pPos += 2;

	// リザーブ
	pPos += 2;

	// デフォルト地域クラス取得
	DHC_GetDefAreaClsCode(pPos, &pAreaClsCode->areaClsCode);
	pPos += read2byte(pPos) * 4;

	// 背景地域クラスデータレコード有無フラグチェック
	if (!binHead.b.rec_flg) {
		// 背景地域クラスデータレコードが存在しない
		return (e_DHC_RESULT_CASH_SUCCESS);
	}

	// 地域クラス取得検索
	DHC_SearchAreaClsCode(pPos, pAreaClsCode);

	return (e_DHC_RESULT_CASH_SUCCESS);
}

/**
 * @brief	デフォルト地域クラスコード取得
 * @param	[I]pBin:デフォルト地域クラスコード先頭アドレス
 * @param	[I]pAreaClsCode:デフォルト地域クラスコード
 */
void DHC_GetDefAreaClsCode(char* pBin, SMAREACLSCODE* pAreaClsCode)
{
	char* pPos = NULL;
	UINT32 dataSize = 0;
	UINT16 areaClsFlg = 0;
	INT32 i = 0;

	pPos = pBin;

	// データサイズ
	dataSize = read2byte(pPos) * 4;
	pPos += 2;

	// 地域クラス情報有無フラグ
	areaClsFlg = read2byte(pPos) & 0x001F;
	pPos += 2;

	// 地域クラス名称ID(6～2)
	// 上位ビットから順に地域クラス2,3,4,5,6
	for (i=0; i<5; i++) {
		if (areaClsFlg & (0x0001<<i)) {
			// 地域クラス名称IDがある場合
			pAreaClsCode->code[e_AREA_CLS6-i] = read2byte(pPos);
			pPos += 2;
		}
	}

	// 地域クラス1名称ID
	pAreaClsCode->code[e_AREA_CLS1] = read2byte(pPos);
}

/**
 * @brief	地域クラスコード取得
 * @param	[I]pBin:背景データ先頭アドレス
 * @param	[O]pAreaClsCode:地域クラスコード
 */
Bool DHC_SearchAreaClsCode(char* pBin, T_DHC_AREA_CLS_CODE* pAreaClsCode)
{
	char* pBkgd = NULL;
	UINT16 cnt = 0;
	INT32 i = 0;
	BIT_SHAPE_TYPE shapeType;
	Bool Judg = false;

	// 背景要素数
	cnt = read2byte(pBin + 2);

	// 背景レコード先頭アドレス
	pBkgd = pBin + 4;

	// 背景要素数分ループ
	for (i=0; i<cnt; i++) {

		// 図形タイプ等
		shapeType.d = read2byte(pBkgd + 2);
		if (shapeType.b.shape_type != 2) {
			// 2:面データ（図形データ有り)以外は処理しない
			continue;
		}

		// ポリゴン内外判定実施
		Judg = DHC_JudgeInOut((pBkgd + 8), pAreaClsCode->x, pAreaClsCode->y);
		if (Judg) {
			// 地域クラスコード取得
			DHC_GetAreaClsCodeOfBkgd(pBkgd, &pAreaClsCode->areaClsCode);
			return (true);
		}

		// 次へ
		pBkgd += (read2byte(pBkgd) * 4);
	}

	return (false);
}

/**
 * @brief	背景レコードから地域クラスコード取得
 * @param	[I]pBkgd:背景レコード先頭アドレス
 * @param	[I]pAreaClsCode:地域クラスコード
 */
void DHC_GetAreaClsCodeOfBkgd(char* pBkgd, SMAREACLSCODE* pAreaClsCode)
{
	char* pShape = NULL;
	BIT_SHAPE_TYPE shapeType;
	BIT_SHAPE_INFO shapeInfo;
	UINT16 pntCnt = 0;
	UINT16 shapeSize = 0;
	char* pPos = NULL;


	// 図形タイプ等取得
	shapeType.d = read2byte(pBkgd + 2);

	// 図形データ先頭アドレス
	pShape = pBkgd + 8;
	// 点数
	pntCnt = read2byte(pShape);
	// データ形式等
	shapeInfo.d = read2byte(pShape+2);
	// 地域クラス情報先頭アドレス取得
	shapeSize = 4 + ((shapeInfo.b.data_form == 0) ? DHC_ALIGNMENT4((4+(pntCnt-1)*2)) : (pntCnt*4));
	pPos = pShape + shapeSize;

	// 地域クラス情報取得
	if (shapeType.b.area6_flg == 1 && shapeType.b.area6_pos_flg == 0) {
		pAreaClsCode->code[e_AREA_CLS6] = read2byte(pPos);
		pPos += 2;
	}
	if (shapeType.b.area5_flg == 1 && shapeType.b.area5_pos_flg == 0) {
		pAreaClsCode->code[e_AREA_CLS5] = read2byte(pPos);
		pPos += 2;
	}
	if (shapeType.b.area4_flg == 1 && shapeType.b.area4_pos_flg == 0) {
		pAreaClsCode->code[e_AREA_CLS4] = read2byte(pPos);
		pPos += 2;
	}
	if (shapeType.b.area3_flg == 1 && shapeType.b.area3_pos_flg == 0) {
		pAreaClsCode->code[e_AREA_CLS3] = read2byte(pPos);
		pPos += 2;
	}
	if (shapeType.b.area2_flg == 1 && shapeType.b.area2_pos_flg == 0) {
		pAreaClsCode->code[e_AREA_CLS2] = read2byte(pPos);
		pPos += 2;
	}
	if (shapeType.b.area1_flg == 1 && shapeType.b.area1_pos_flg == 0) {
		pAreaClsCode->code[e_AREA_CLS1] = read2byte(pPos);
		pPos += 2;
	}
}

/**
 * @brief	図形データから指定座標を内外判
 * @param	[I]pBkgd:背景レコード先頭アドレス
 * @param	[I]searchX:X座標
 * @param	[I]searchY:Y座標
 */
Bool DHC_JudgeInOut(char* pShape, INT16 searchX, INT16 searchY)
{
	char* pPos = NULL;
	UINT16 pntCnt;
	BIT_SHAPE_INFO shapeInfo;
	INT32 i = 0;
	INT32 crossCnt = 0;

	INT16 x = 0;
	INT16 y = 0;
	INT16 tmpX = 0;
	INT16 tmpY = 0;
	T_POINT inPoint;
	T_POINT p1;
	T_POINT p2;

	// 入力座標設定
	inPoint.x = searchX;
	inPoint.y = searchY;

	pPos = pShape;

	// 点数
	pntCnt = read2byte(pPos);
	pPos += 2;

	// データ形式等
	shapeInfo.d = read2byte(pPos);
	pPos += 2;

	if (shapeInfo.b.data_form == 0) {
		// 1byte
		// 始点座標
		x = read2byte(pPos);
		pPos += 2;
		y = read2byte(pPos);
		pPos += 2;

		// 始点座標保持
		tmpX = x;
		tmpY = y;

		// 座標点数文ループ
		for (i=0; i<pntCnt; i++) {
			if (i != 0) {
				x += *(INT8*)(pPos  );
				y += *(INT8*)(pPos+1);
				pPos += 2;
			}

			// 始点側座標設定
			p1.x = x;
			p1.y = y;

			// 終点側座標設定
			if (i == pntCnt-1) {
				p2.x = tmpX;
				p2.y = tmpY;
			} else {
				p2.x = x + *(INT8*)(pPos  );
				p2.y = y + *(INT8*)(pPos+1);
			}

			// 内外判定
			if (DHC_IsCrossX(inPoint, p1, p2)) {
				crossCnt++;
			}
		}
	}
	else {
		// 2byte
		// 座標点数文ループ
		for (i=0; i<pntCnt; i++) {
			x = read2byte(pPos  ) & 0x1FFF;
			y = read2byte(pPos+2) & 0x1FFF;
			pPos += 4;

			// 始点座標保持
			if (i == 0) {
				tmpX = x;
				tmpY = y;
			}

			// 始点側座標設定
			p1.x = x;
			p1.y = y;

			// 終点側座標設定
			if (i == pntCnt-1) {
				p2.x = tmpX;
				p2.y = tmpY;
			} else {
				p2.x = read2byte(pPos  ) & 0x1FFF;
				p2.y = read2byte(pPos+2) & 0x1FFF;
			}

			// 内外判定
			if (DHC_IsCrossX(inPoint, p1, p2)) {
				crossCnt++;
			}
		}
	}

	// 内外判定
	if (crossCnt%2 == 0) {
		// 偶数：外
		return (false);
	} else {
		// 奇数：内
		return (true);
	}
}

/**
 * @brief	指定座標(p)が線分(p1,p2)とX軸と平行なプラス方向に交差するか判定
 * @param	[I]p:判定座標
 * @param	[I]p1:始点
 * @param	[I]p2:終点
 */
Bool DHC_IsCrossX(T_POINT p, T_POINT p1, T_POINT p2)
{
	// p1とp2比較しyが大きい方がp2となるように設定
	if (p1.y > p2.y) {
		T_POINT tmpP = p1;
		p1 = p2;
		p2 = tmpP;
	}

	// Y軸方向に範囲内かチェック
	if (p1.y <= p.y && p.y < p2.y) {
		// X軸に水平な半直線との交点比較
		if (((p1.x * (p2.y - p.y) + p2.x * (p.y - p1.y)) / (p2.y - p1.y)) > p.x) {
			return (true);
		}
	}

	return (false);
}

/**
 * @brief	ポリゴン内外判定
 * @param	[I]pPos:判定座標
 * @param	[I]pPoly:座標配列
 * @param	[I]polyCnt:座標数
 */
/*Bool DHC_InPolygon(T_POINT *pPos, T_POINT *pPoly, INT32 polyCnt)
{
	Bool bResult = true;	// 内:true, 外:false
	INT32 i = 0;
	INT32 cnt = 0;

	for (i=0; i<polyCnt; i++) {
		if (DHC_IsCrossX(*pPos, pPoly[i], pPoly[(i+1)%polyCnt])) {
			cnt++;
		}
	}

	// 内外判定
	if (cnt%2 == 0) {	// 偶数：外
		bResult = false;
	} else {			// 奇数：内
		bResult = true;
	}

	return bResult;
}*/

