/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef _MP_GL_H
#define _MP_GL_H

#ifdef __cplusplus
extern "C" {
#endif


//------------------------------------------------------------------------------
// 構造体
//------------------------------------------------------------------------------
// 座標
typedef struct _POINT {
	FLOAT	x;	// X座標
	FLOAT	y;	// Y座標
} T_POINT;

// rgbaカラー
typedef struct _Color {
	UINT8	r;
	UINT8	g;
	UINT8	b;
	UINT8	a;
} T_Color;

// VBO情報
typedef struct _VBO_INFO {
	UINT32	vboID;			// VBO ID
	UINT32	type;			// 頂点タイプ(GL_TRIANGLES, GL_TRIANGLE_STRIP)
	INT32	pointCnt;		// 頂点座標数
} T_VBO_INFO;


//------------------------------------------------------------------------------
// マクロ
//------------------------------------------------------------------------------
// 右0度反時計回りで90度より大きく270度より小さいか判定
#define MP_GL_IS_TURNOVER(_deg)		(((_deg>90.0f)&&(270.0f>_deg)) ? true : false)


//------------------------------------------------------------------------------
// プロトタイプ宣言
//------------------------------------------------------------------------------
FLOAT MP_GL_SqrtF(const FLOAT x);
INT32 MP_GL_LineOff(const T_POINT *pV0, const T_POINT *pV1, FLOAT dist, T_POINT* pPos);
void MP_GL_DegenerateTriangle(const T_POINT* pPoint, INT32 pointCnt, FLOAT width, T_POINT* pOutBuf, INT32* pIndex);
FLOAT MP_GL_Distance(const T_POINT *v0, const T_POINT *v1);
FLOAT MP_GL_Degree(const T_POINT *v0, const T_POINT *v1);

void MP_GL_Draw(const INT32 mode, const T_POINT* pPos, INT32 cnt);
void MP_GL_DrawLineStrip(const T_POINT* pPos, INT32 cnt, FLOAT width);
void MP_GL_DrawLines(const T_POINT* pPos, INT32 cnt, FLOAT width);
void MP_GL_DrawPolyLine(const T_POINT* pPos, INT32 cnt);
void MP_GL_DrawDotLines(const T_POINT* pPos, INT32 cnt, FLOAT width, FLOAT dot);
void MP_GL_DrawCircle(const T_POINT* pCenter, FLOAT radius, INT32 divCnt, FLOAT width);
void MP_GL_DrawCircleFill(const T_POINT* pCenter, FLOAT radius, INT32 divCnt);
void MP_GL_DrawCircleFillEx(const T_POINT* pCenter, FLOAT radius);
void MP_GL_DrawSquares(FLOAT x, FLOAT y, FLOAT width, FLOAT height);

INT32 MP_GL_LineOffShift(const T_POINT *pV0, const T_POINT *pV1, FLOAT dist, FLOAT shift, T_POINT* pPos);
void MP_GL_DegenerateTriangleShift(const T_POINT* pPos, INT32 pointCnt, FLOAT width, FLOAT shift, UINT8 dir, Bool arrow, T_POINT* pOutBuf, INT32* pIndex);
void MP_GL_DrawColor(const INT32 mode, const T_POINT* pPos, const T_Color* pColor, INT32 cnt);
//void MP_GL_DrawLinesShift(const T_POINT* pPos, INT32 cnt, FLOAT width, FLOAT shift, INT16 dir);
void MP_GL_DrawPolyLineColor(const T_POINT* pPos, T_Color* pColor, INT32 cnt);
INT32 MP_GL_AddHalfArrow(const T_POINT* pPos1, const T_POINT* pPos2, FLOAT length, FLOAT width, T_POINT* pOut);

Bool MP_GL_SetVBO(T_VBO_INFO *pVbo, const T_POINT *pPoint, const T_Color *pColor);
Bool MP_GL_DeleteVBO(T_VBO_INFO *pVbo);
Bool MP_GL_DrawVBO(const T_VBO_INFO *pVbo);

UINT32 MP_GL_GenTextures(const UINT8* pByteArray, INT32 width, INT32 height);
void MP_GL_DeleteTextures(UINT32 *pTextureID);
void MP_GL_DrawTextures(UINT32 pTextureID, const T_POINT *pSquares);

void MP_GL_Init();
void MP_GL_LoadIdentity();
void MP_GL_Viewport(INT32 x, INT32 y, INT32 width, INT32 height);
void MP_GL_ClearColor(RGBACOLOR rgba);
void MP_GL_Color4f(FLOAT r, FLOAT g, FLOAT b, FLOAT a);
void MP_GL_ColorRGBA(RGBACOLOR rgba);
void MP_GL_ColorRGBATrans(RGBACOLOR rgba, FLOAT a);
void MP_GL_BeginBlend();
void MP_GL_EndBlend();
void MP_GL_PushMatrix();
void MP_GL_PopMatrix();
void MP_GL_Rotatef(FLOAT angle, FLOAT x, FLOAT y, FLOAT z);
void MP_GL_Translatef(FLOAT x, FLOAT y, FLOAT z);
void MP_GL_Scalef(FLOAT x, FLOAT y, FLOAT z);
void MP_GL_MatrixProjection();
void MP_GL_MatrixModelView();
void MP_GL_Flush();
void MP_GL_Orthof(FLOAT left, FLOAT right, FLOAT bottom, FLOAT top, FLOAT zNear, FLOAT zFar);


#ifdef __cplusplus
}
#endif

#endif	// _MP_GL_H
