/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#include "SMCoreMPInternal.h"

//------------------------------------------------------------------------------
// 定数
//------------------------------------------------------------------------------
#define MP_GL_ACCY				1.0E-15
#define MP_GL_BUF_SIZE			10000
#define MP_GL_LINE_OFF_SIZE		4


//------------------------------------------------------------------------------
// マクロ
//------------------------------------------------------------------------------
#define MP_GL_HALF(_data)		(FLOAT)((_data)*0.5f)


//------------------------------------------------------------------------------
// 静的変数
//------------------------------------------------------------------------------
// 汎用バッファ
static T_POINT gPointBuf[MP_GL_BUF_SIZE];


//------------------------------------------------------------------------------
// 関数
//------------------------------------------------------------------------------
/**
 * @brief		平方根(ルート)計算
 * @return		結果
 */
FLOAT MP_GL_SqrtF(const FLOAT x)
{
#if 1
	return (sqrtf(x));
#else
	FLOAT xHalf = 0.5f * x;
	INT32 tmp = 0x5F3759DF - ( *(INT32*)&x >> 1 ); //initial guess
	FLOAT xRes  = *(FLOAT*)&tmp;

	xRes *= ( 1.5f - ( xHalf * xRes * xRes ) );
	return (xRes * x);
#endif
}

/**
 * @brief		2点から指定した距離にオフセットした4点算出
 * @param[in]	pV0 座標1
 * @param[in]	pV1 座標2
 * @param[in]	dist 幅(片側)
 * @param[out]	pPos 変換座標
 * @return		結果(成功:0, 失敗:-1)
 */
INT32 MP_GL_LineOff(const T_POINT *pV0, const T_POINT *pV1, FLOAT dist, T_POINT* pPos)
{
	FLOAT	xlk;
	FLOAT	ylk;
	FLOAT	rsq;
	FLOAT	rinv;
	FLOAT	a;
	FLOAT	b;

	xlk = pV1->x - pV0->x;
	ylk = pV1->y - pV0->y;
	rsq = (xlk * xlk) + (ylk * ylk);

	if (rsq < MP_GL_ACCY) {
		return (-1);
	} else {
		rinv = 1.0f / MP_GL_SqrtF(rsq);

		a = -ylk * rinv * dist;
		b =  xlk * rinv * dist;
	}

	pPos[0].x = pV0->x + a;
	pPos[0].y = pV0->y + b;
	pPos[1].x = pV0->x - a;
	pPos[1].y = pV0->y - b;
	pPos[2].x = pV1->x + a;
	pPos[2].y = pV1->y + b;
	pPos[3].x = pV1->x - a;
	pPos[3].y = pV1->y - b;

	return (0);
}

/**
 * @brief		縮退三角形連結
 */
void MP_GL_DegenerateTriangle(const T_POINT* pPos, INT32 pointCnt, FLOAT width, T_POINT* pOutBuf, INT32* pIndex)
{
	INT32 idx = *pIndex;
	INT32 ret = 0;
	INT32 i = 0;
	//FLOAT w = width * 0.5f;

	for (i=0; i<pointCnt-1; i++) {
		if (0 == i && 0 != idx) {
			// 座標の並びが先頭　且つ　データの並びが先頭以外
			ret = MP_GL_LineOff(&pPos[i], &pPos[i+1], width, &pOutBuf[idx+1]);
			if (-1 == ret) {
				continue;
			}

			// 先頭と同じ座標を先頭に格納
			pOutBuf[idx] = pOutBuf[idx+1];

			idx += 5;
		} else {
			ret = MP_GL_LineOff(&pPos[i], &pPos[i+1], width, &pOutBuf[idx]);
			if (-1 == ret) {
				continue;
			}

			idx += 4;
		}
	}

	if (idx != *pIndex) {
		// リンクの最後の座標を最後尾に設定
		pOutBuf[idx] = pOutBuf[idx-1];
		idx++;

		*pIndex = idx;
	}
}

/**
 * @brief		2点間の距離算出
 * @param[in]	pV0 座標1
 * @param[in]	pV1 座標2
 * @return		距離
 */
FLOAT MP_GL_Distance(const T_POINT *v0, const T_POINT *v1)
{
	FLOAT x = 0.0f;
	FLOAT y = 0.0f;
	FLOAT distance = 0.0f;

	x = v1->x - v0->x;
	y = v1->y - v0->y;

	// 距離rを求める
	distance = MP_GL_SqrtF((x*x) + (y*y));

	// 絶対値
	distance = fabs(distance);

	return (distance);
}

/**
 * @brief		2点間の角度算出
 * 				(座標1から座標2への角度)　右0°で反時計回り
 * @param[in]	pV0 座標1
 * @param[in]	pV1 座標2
 * @return		角度
 */
FLOAT MP_GL_Degree(const T_POINT *v0, const T_POINT *v1)
{
	FLOAT rad = 0.0f;
	FLOAT deg = 0.0f;

	// 2点間から表示角度算出
	rad = atan2f((v1->y - v0->y), (v1->x - v0->x));

	// 0から2π(0度から360度)を求める為、マイナス値補正
	if(rad < 0.0f) {
		rad = rad + (2.0f * M_PI);
	}

	// ラジアンから度に変換
	deg = rad * 180.0f / M_PI;

	return (deg);
}

/**
 * @brief		指定した頂点配列を描画
 * @param[in]	mode 描画モード
 * @param[in]	pPos 頂点座標
 * @param[in]	cnt 頂点座標数
 */
void MP_GL_Draw(const INT32 mode, const T_POINT* pPos, INT32 cnt)
{
	glVertexPointer(2, GL_FLOAT, 0, pPos);
#ifdef _GLES_2
//	MP_UseProgram(PROGRAM_VERTEX_ARRAY);
	MP_LoadMatrix();
#endif
	glDrawArrays(mode, 0, cnt);
}

/**
 * @brief		指定した頂点配列をLINE_STRIPで描画
 * @param[in]	pPos 頂点座標
 * @param[in]	cnt 頂点座標数
 * @param[in]	width 幅
 */
void MP_GL_DrawLineStrip(const T_POINT* pPos, INT32 cnt, FLOAT width)
{
	glLineWidth(width);
	MP_GL_Draw(GL_LINE_STRIP, pPos, cnt);
}

/**
 * @brief		線を描画
 * @param[in]	pPos 頂点座標
 * @param[in]	cnt 頂点座標数
 * @param[in]	width 太さ
 */
void MP_GL_DrawLines(const T_POINT* pPos, INT32 cnt, FLOAT width)
{
	INT32 idx = 0;
	INT32 ret = 0;
	INT32 i = 0;
	FLOAT w = 0.0f;

	if (MP_GL_BUF_SIZE < (cnt-1)*MP_GL_LINE_OFF_SIZE) {
		// 描画しない
		return;
	}

	// 太さの半分
	w = MP_GL_HALF(width);

	// 点数分ループ
	for (i=0; i<cnt-1; i++) {
		ret = MP_GL_LineOff(&pPos[i], &pPos[i+1], w, &gPointBuf[idx]);
		if (-1 == ret) {
			continue;
		}
		idx += MP_GL_LINE_OFF_SIZE;
	}

	MP_GL_Draw(GL_TRIANGLE_STRIP, gPointBuf, idx);
}

/**
 * @brief		線を描画
 * @param[in]	pPos 頂点座標(ポリゴン化済み)
 * @param[in]	cnt 頂点座標数
 */
void MP_GL_DrawPolyLine(const T_POINT* pPos, INT32 cnt)
{
	MP_GL_Draw(GL_TRIANGLE_STRIP, pPos, cnt);
}

/**
 * @brief		破線を描画
 * @param[in]	pPos 頂点座標
 * @param[in]	cnt 頂点座標数
 * @param[in]	width 太さ
 * @param[in]	dot 破線間隔
 */
void MP_GL_DrawDotLines(const T_POINT* pPos, INT32 cnt, FLOAT width, FLOAT dot)
{
	T_POINT		vv[2];			// 破線線分
	T_POINT		v0;				// 始点
	T_POINT		v1;				// 終点
	T_POINT		dv;				// 単位ベクトル
	FLOAT		len = 0.0f;		// 2点間の距離
	FLOAT		vpos = 0.0f;
	FLOAT		dLen = 0.0f;
	Bool		drawF = true;
	FLOAT		chk_len = 0.0f;
	FLOAT		w = 0.0f;
	INT32		index = 0;
	INT32		i = 0;

	// 太さの半分
	w = width * 0.5f;

	for (i=0; i < cnt-1; i++) {
		v0 = pPos[i];
		v1 = pPos[i + 1];

		dv.x = v1.x - v0.x;
		dv.y = v1.y - v0.y;

		// 2点間の距離
		len = fabs(MP_GL_SqrtF((dv.x)*(dv.x) + (dv.y)*(dv.y)));
		if (len < MP_GL_ACCY) {
			continue;
		}

		// 2点間単位ベクトル計算
		dv.x *= 1.0f/len;
		dv.y *= 1.0f/len;

		vpos = 0.0f;
		dLen = 0.0f;

		while (len > MP_GL_ACCY) {
			if (chk_len >= dot - MP_GL_ACCY) {
				drawF = (drawF) ? false : true;
				chk_len = 0.0f;
			}

			if (chk_len + len <= dot) {
				dLen = len - chk_len;
				if (dLen < 0.0f) dLen = len;
				if (drawF) {
					vv[0].x = (dv.x * vpos) + v0.x;
					vv[0].y = (dv.y * vpos) + v0.y;
					vv[1].x = (dv.x * (vpos + dLen)) + v0.x;
					vv[1].y = (dv.y * (vpos + dLen)) + v0.y;

					MP_GL_DegenerateTriangle(vv, 2, w, gPointBuf, &index);
				}
				chk_len += dLen;
				len -= dLen;
				break;
			}

			dLen = dot - chk_len;
			if (drawF) {
				vv[0].x = (dv.x * vpos) + v0.x;
				vv[0].y = (dv.y * vpos) + v0.y;
				vv[1].x = (dv.x * (vpos + dLen)) + v0.x;
				vv[1].y = (dv.y * (vpos + dLen)) + v0.y;

				MP_GL_DegenerateTriangle(vv, 2, w, gPointBuf, &index);
			}
			chk_len += dLen;
			len  -= dLen;

			vpos += dLen;
		}
	}

	if (index >= MP_TRIANGLE_CNT) {
		MP_GL_Draw(GL_TRIANGLE_STRIP, gPointBuf, index);
	}
}

/**
 * @brief		円を描画
 * @param[in]	center 中心位置
 * @param[in]	radius 半径
 * @param[in]	divCnt 分割数
 */
void MP_GL_DrawCircle(const T_POINT* pCenter, FLOAT radius, INT32 divCnt, FLOAT width)
{
	static T_POINT buf[360+2];
	FLOAT		dPos = 0.0f;
	FLOAT		dd   = (M_PI * 2.0f) / (FLOAT)divCnt;
	T_POINT		v0;
	INT32		i = 0;
	FLOAT		w = 0.0f;

	// 太さの半分
	w = width * 0.5f;

	for (i=0; i <= divCnt; i++) {
		v0.x = cos(dPos) * radius;
		v0.y = sin(dPos) * radius;
		buf[i].x = v0.x + pCenter->x;
		buf[i].y = v0.y + pCenter->y;
		dPos += dd;
	}
	MP_GL_DrawLines(buf, divCnt+1, w);
}

/**
 * @brief		円(塗りつぶし)を描画
 * @param[in]	center 中心位置
 * @param[in]	radius 半径
 * @param[in]	divCnt 分割数
 */
void MP_GL_DrawCircleFill(const T_POINT* pCenter, FLOAT radius, INT32 divCnt)
{
	static T_POINT buf[360+2];
	FLOAT dPos = 0.0f;
	INT32 i = 0;
	FLOAT dd = (M_PI * 2.0f) / (FLOAT)divCnt;

	buf[0].x = pCenter->x;
	buf[0].y = pCenter->y;

	for (i=0; i<=divCnt; i++) {
		buf[i+1].x = pCenter->x + cos(dPos) * radius;
		buf[i+1].y = pCenter->y + sin(dPos) * radius;
		dPos += dd;
	}

	MP_GL_Draw(GL_TRIANGLE_FAN, buf, divCnt+2);
}

/**
 * @brief		円(塗りつぶし)を描画 バッファリング
 * @param[in]	center 中心位置
 * @param[in]	radius 半径
 */
void MP_GL_DrawCircleFillEx(const T_POINT* pCenter, FLOAT radius)
{
	static T_POINT g_point[16];
	static Bool gPolycircle = false;

	static T_POINT point[16];
	FLOAT dPos = 0.0f;
	INT32 divCnt = 8;
	INT32 i = 0;

	if (gPolycircle == false) {
		FLOAT dd = (M_PI * 2.0f) / (FLOAT)divCnt;
		for (i=0; i<=divCnt; i++) {
			g_point[i].x = cos(dPos);
			g_point[i].y = sin(dPos);
			dPos += dd;
		}
		gPolycircle = true;
	}

	point[0].x = pCenter[0].x;
	point[0].y = pCenter[0].y;

	for (i=0; i<=divCnt; i++) {
		point[i+1].x = point[0].x + (g_point[i].x * radius);
		point[i+1].y = point[0].y + (g_point[i].y * radius);
	}

	MP_GL_Draw(GL_TRIANGLE_FAN, point, divCnt+2);
}

/**
 * @brief		四角描画
 * @param[in]	x X座標
 * @param[in]	y Y座標
 * @param[in]	width 幅
 * @param[in]	height 高さ
 */
void MP_GL_DrawSquares(FLOAT x, FLOAT y, FLOAT width, FLOAT height)
{
	static T_POINT squares[4];

	squares[0].x = x;
	squares[0].y = y;
	squares[1].x = x;
	squares[1].y = height;
	squares[2].x = width;
	squares[2].y = y;
	squares[3].x = width;
	squares[3].y = height;

	MP_GL_Draw(GL_TRIANGLE_STRIP, squares, 4);
}

/**
 * @brief		2点から指定した距離にオフセットした4点算出(片側へシフト)
 * @param[in]	pV0 座標1
 * @param[in]	pV1 座標2
 * @param[in]	dist 幅(片側)
 * @param[in]	shift シフト量(-:左/+:右)
 * @param[out]	pPos 変換座標
 * @return		結果(成功:0, 失敗:-1)
 */
INT32 MP_GL_LineOffShift(const T_POINT *pV0, const T_POINT *pV1, FLOAT dist, FLOAT shift, T_POINT* pPos)
{
	FLOAT	xlk;
	FLOAT	ylk;
	FLOAT	rsq;
	FLOAT	rinv;
	FLOAT	a;
	FLOAT	b;
	FLOAT	aSft;
	FLOAT	bSft;

	xlk = pV1->x - pV0->x;
	ylk = pV1->y - pV0->y;
	rsq = (xlk * xlk) + (ylk * ylk);

	if (rsq < MP_GL_ACCY) {
		return (-1);
	} else {
		rinv = 1.0f / MP_GL_SqrtF(rsq);

		a = -ylk * rinv * dist;
		b =  xlk * rinv * dist;
		aSft = -ylk * rinv * (shift);
		bSft =  xlk * rinv * (shift);
	}

	pPos[0].x = pV0->x + aSft + a;
	pPos[0].y = pV0->y + bSft + b;
	pPos[1].x = pV0->x + aSft - a;
	pPos[1].y = pV0->y + bSft - b;
	pPos[2].x = pV1->x + aSft + a;
	pPos[2].y = pV1->y + bSft + b;
	pPos[3].x = pV1->x + aSft - a;
	pPos[3].y = pV1->y + bSft - b;

	return (0);
}

/**
 * @brief		縮退三角形連結(シフト)
 */
void MP_GL_DegenerateTriangleShift(const T_POINT* pPos, INT32 pointCnt, FLOAT width, FLOAT shift, UINT8 dir, Bool arrow, T_POINT* pOutBuf, INT32* pIndex)
{
	INT32 idx = *pIndex;
	INT32 ret = 0;
	INT32 i = 0;

	if (0 == dir) {
		// 順方向左にシフト
		for (i=0; i<pointCnt-1; i++) {
			if (0 == i && 0 != idx) {
				// 座標の並びが先頭　且つ　データの並びが先頭以外
				ret = MP_GL_LineOffShift(&pPos[i], &pPos[i+1], width, -shift, &pOutBuf[idx+1]);
				if (-1 == ret) {
					continue;
				}
				// 先頭と同じ座標を先頭に格納
				pOutBuf[idx] = pOutBuf[idx+1];
				idx += 5;
			} else {
				ret = MP_GL_LineOffShift(&pPos[i], &pPos[i+1], width, -shift, &pOutBuf[idx]);
				if (-1 == ret) {
					continue;
				}
				idx += 4;
			}
		}
	} else {
		// 逆方向左にシフト
		for (i=pointCnt-1; i>0; i--) {
			if ((pointCnt-1) == i && 0 != idx) {
				// 座標の並びが先頭　且つ　データの並びが先頭以外
				ret = MP_GL_LineOffShift(&pPos[i], &pPos[i-1], width, -shift, &pOutBuf[idx+1]);
				if (-1 == ret) {
					continue;
				}
				// 先頭と同じ座標を先頭に格納
				pOutBuf[idx] = pOutBuf[idx+1];
				idx += 5;
			} else {
				ret = MP_GL_LineOffShift(&pPos[i], &pPos[i-1], width, -shift, &pOutBuf[idx]);
				if (-1 == ret) {
					continue;
				}
				idx += 4;
			}
		}
	}

	if (idx != *pIndex) {
		// 矢印追加
		if (arrow) {
			idx += MP_GL_AddHalfArrow(&pOutBuf[idx-3], &pOutBuf[idx-1], shift*3, shift, &pOutBuf[idx]);
		}

		// リンクの最後の座標を最後尾に設定
		pOutBuf[idx] = pOutBuf[idx-1];
		idx++;

		*pIndex = idx;
	}
}

/**
 * @brief		指定した頂点配列を描画(色配列指定)
 * @param[in]	mode 描画モード
 * @param[in]	pPos 頂点座標
 * @param[in]	pColor 頂点に対応した色
 * @param[in]	cnt 頂点座標数
 */
void MP_GL_DrawColor(const INT32 mode, const T_POINT* pPos, const T_Color* pColor, INT32 cnt)
{
	glEnableClientState(GL_COLOR_ARRAY);

	glVertexPointer(2, GL_FLOAT, 0, pPos);
	glColorPointer(4, GL_UNSIGNED_BYTE, 0, pColor);
#ifdef _GLES_2
//	MP_UseProgram(PROGRAM_COLOR_ARRAY);
	MP_LoadMatrix();
#endif
	glDrawArrays(mode, 0, cnt);

	glDisableClientState(GL_COLOR_ARRAY);
}

/**
 * @brief		線を描画(オフセット指定)
 * @param[in]	pPos 頂点座標
 * @param[in]	cnt 頂点座標数
 * @param[in]	width 太さ
 * @param[in]	offSet オフセット量
 * @param[in]	dir 方向 0:順方向左側/1順方向右側
 */
/*void MP_GL_DrawLinesShift(const T_POINT* pPos, INT32 cnt, FLOAT width, FLOAT shift, INT16 dir)
{
	INT32 idx = 0;
	INT32 ret = 0;
	INT32 i = 0;
	FLOAT w = 0.0f;

	if (MP_GL_BUF_SIZE < (cnt-1)*MP_GL_LINE_OFF_SIZE) {
		// 描画しない
		return;
	}

	// 太さの半分
	w = MP_GL_HALF(width);

	// 点数分ループ
	if (0 == dir) {
		// 順方向
		for (i=0; i<cnt-1; i++) {
			ret = MP_GL_LineOffEx(&pPos[i], &pPos[i+1], w, -shift, &gPointBuf[idx]);
			if (-1 == ret) {
				continue;
			}
			idx += MP_GL_LINE_OFF_SIZE;
		}
	} else {
		// 逆方向
		for (i=cnt-1; i>0; i--) {
			ret = MP_GL_LineOffEx(&pPos[i], &pPos[i-1], w, -shift, &gPointBuf[idx]);
			if (-1 == ret) {
				continue;
			}
			idx += MP_GL_LINE_OFF_SIZE;
		}
	}

	MP_GL_Draw(GL_TRIANGLE_STRIP, gPointBuf, idx);
}*/

/**
 * @brief		線を描画(色配列指定)
 * @param[in]	pPos 頂点座標(ポリゴン化済み)
 * @param[in]	pColor 頂点に対応した色
 * @param[in]	cnt 頂点座標数
 */
void MP_GL_DrawPolyLineColor(const T_POINT* pPos, T_Color* pColor, INT32 cnt)
{
	MP_GL_DrawColor(GL_TRIANGLE_STRIP, pPos, pColor, cnt);
}

/**
 * @brief		半矢印座標追加
 * 				与えられた2点から半矢印座標を生成
 * @param[in]	pPos1 始点座標
 * @param[in]	pPos2 終点座標
 * @param[in]	length 矢印の長さ
 * @param[in]	width 傘の幅
 * @return		生成に成功した場合2
 */

INT32 MP_GL_AddHalfArrow(const T_POINT* pPos1, const T_POINT* pPos2, FLOAT length, FLOAT width, T_POINT* pOut)
{
	FLOAT rad;
	FLOAT sn;
	FLOAT cs;

	// 2点間の距離チェック
	if (length > MP_GL_Distance(pPos1, pPos2)/3.0f) {
		return (0);
	}

	rad = atan2f(pPos2->y-pPos1->y, pPos2->x-pPos1->x);
	sn = sin(rad);
	cs = cos(rad);

	pOut[0].x = pPos2->x - (length*cs);
	pOut[0].y = pPos2->y - (length*sn);
	pOut[1].x = pOut[0].x + (width*sn);
	pOut[1].y = pOut[0].y - (width*cs);

	return (2);
}

/**
 * @brief		VBO設定
 * @param[io]	pVbo VBO情報2
 * @param[in]	pPoint 頂点配列
 * @param[in]	pColor カラー配列
 * @return		結果(成功:true, 失敗:false)
 */
Bool MP_GL_SetVBO(T_VBO_INFO *pVbo, const T_POINT *pPoint, const T_Color *pColor)
{
	INT32 pointSize = sizeof(T_POINT) * pVbo->pointCnt;
	INT32 colorSize = sizeof(T_Color) * pVbo->pointCnt;

	pVbo->type = GL_TRIANGLES;

	glGenBuffers(1, &pVbo->vboID);
	glBindBuffer(GL_ARRAY_BUFFER, pVbo->vboID);

	glBufferData(GL_ARRAY_BUFFER, (pointSize + colorSize), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0,         pointSize, pPoint);	// 頂点
	glBufferSubData(GL_ARRAY_BUFFER, pointSize, colorSize, pColor);	// 色

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return (true);
}

/**
 * @brief		VBO削除
 * @param[io]	pVbo VBO情報2
 * @return		結果(成功:true, 失敗:false)
 */
Bool MP_GL_DeleteVBO(T_VBO_INFO *pVbo)
{
	glDeleteBuffers(1, &pVbo->vboID);
	pVbo->vboID = 0;
	pVbo->pointCnt = 0;
	pVbo->type = 0;

	return (true);
}

/**
 * @brief		VBO描画
 * @param[in]	pVbo VBO情報
 * @return		結果(成功:true, 失敗:false)
 */
Bool MP_GL_DrawVBO(const T_VBO_INFO *pVbo)
{
	if (0 == pVbo->vboID){
		return (false);
	}

	glEnableClientState(GL_COLOR_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, pVbo->vboID);

	glVertexPointer(2, GL_FLOAT, 0, 0);
	glColorPointer(4, GL_UNSIGNED_BYTE, 0, (void*)(sizeof(T_POINT) * pVbo->pointCnt));

#ifdef _GLES_2
//	MP_UseProgram(PROGRAM_COLOR_ARRAY);
	MP_LoadMatrix();
#endif
	glDrawArrays(pVbo->type, 0, pVbo->pointCnt);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glDisableClientState(GL_COLOR_ARRAY);

	return (true);
}

/**
 * @brief		テクスチャ設定
 */
UINT32 MP_GL_GenTextures(const UINT8* pByteArray, INT32 width, INT32 height)
{
	GLuint textureID;

	if(NULL == pByteArray) {
		return (0);
	}

	glGenTextures(1, &textureID);
#ifdef _GLES_2
	glActiveTexture(GL_TEXTURE0);
#endif
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
#ifdef _GLES_2
#else
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
#endif
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLint)width, (GLint)height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)pByteArray);

	return ((UINT32)textureID);
}

/**
 * @brief		テクスチャ解放
 */
void MP_GL_DeleteTextures(UINT32 *pTextureID)
{
	glDeleteTextures(1, (GLuint*)pTextureID);
	*pTextureID = 0;
}

/**
 * @brief		テクスチャ描画
 */
void MP_GL_DrawTextures(UINT32 pTextureID, const T_POINT *pSquares)
{
#if 1
	static T_POINT textureCoords[4] = {
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f
	};

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glEnable(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, pTextureID);

	glVertexPointer(2, GL_FLOAT, 0, pSquares);
	glTexCoordPointer(2, GL_FLOAT, 0, textureCoords);

#ifdef _GLES_2
//	MP_UseProgram(PROGRAM_TEXTURE_ARRAY);
	MP_LoadMatrix();
#endif
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisable(GL_TEXTURE_2D);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#endif
}

/**
 * @brief		初期化
 */
void MP_GL_Init()
{
#ifdef _GLES_2
	glDisable(GL_DITHER);
	glDisable(GL_DEPTH_TEST);
#else
	glDisable(GL_LIGHTING);
	// ディザを無効化
	glDisable(GL_DITHER);

	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);

	glDisable(GL_DEPTH_TEST);

	glShadeModel(GL_FLAT);
#endif
	// 頂点配列の使用を許可
	glEnableClientState(GL_VERTEX_ARRAY);

	// 画面クリア
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

/**
 * @brief		LoadIdentity
 */
void MP_GL_LoadIdentity()
{
	glLoadIdentity();
}

/**
 * @brief		Viewport設定
 */
void MP_GL_Viewport(INT32 x, INT32 y, INT32 width, INT32 height)
{
	glViewport(x, y, width, height);
}

/**
 * @brief		ClearColor
 */
void MP_GL_ClearColor(RGBACOLOR rgba)
{
	glClearColor(GET_FR(rgba), GET_FG(rgba), GET_FB(rgba), GET_FA(rgba));
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

/**
 * @brief		色設定
 */
void MP_GL_Color4f(FLOAT r, FLOAT g, FLOAT b, FLOAT a)
{
	glColor4f(r, g, b, a);
}

/**
 * @brief		色設定
 * @param[in]	rgba rgba
 */
void MP_GL_ColorRGBA(RGBACOLOR rgba)
{
	glColor4f(GET_FR(rgba), GET_FG(rgba), GET_FB(rgba), GET_FA(rgba));
}

/**
 * @brief		色設定(透過色置き換え)
 * @param[in]	rgba rgba
 */
void MP_GL_ColorRGBATrans(RGBACOLOR rgba, FLOAT a)
{
	glColor4f(GET_FR(rgba), GET_FG(rgba), GET_FB(rgba), a);
}

/**
 * @brief		BLEND開始
 */
void MP_GL_BeginBlend()
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

/**
 * @brief		BLEND終了
 */
void MP_GL_EndBlend()
{
	glDisable(GL_BLEND);
}

/**
 * @brief		PushMatrix
 */
void MP_GL_PushMatrix()
{
	glPushMatrix();
}

/**
 * @brief		PopMatrix
 */
void MP_GL_PopMatrix()
{
	glPopMatrix();
}

/**
 * @brief		Rotatef
 */
void MP_GL_Rotatef(FLOAT angle, FLOAT x, FLOAT y, FLOAT z)
{
	glRotatef(angle, x, y, z);
}

/**
 * @brief		Translatef
 */
void MP_GL_Translatef(FLOAT x, FLOAT y, FLOAT z)
{
	glTranslatef(x, y, z);
}

/**
 * @brief		Scalef
 */
void MP_GL_Scalef(FLOAT x, FLOAT y, FLOAT z)
{
	glScalef(x, y, z);
}

/**
 * @brief		PROJECTIONモード
 */
void MP_GL_MatrixProjection()
{
	// プロジェクション行列の設定
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
}

/**
 * @brief		MODELVIEWモード
 */
void MP_GL_MatrixModelView()
{
	// モデルビュー行列の設定
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

/**
 * @brief		Flush
 */
void MP_GL_Flush()
{
	glFlush();
}

/**
 * @brief		Orthof
 */
void MP_GL_Orthof(FLOAT left, FLOAT right, FLOAT bottom, FLOAT top, FLOAT zNear, FLOAT zFar)
{
	glOrthof(left, right, bottom, top, zNear, zFar);
}
