/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef _MP_SHADER_H
#define _MP_SHADER_H

#ifdef _GLES_2

//------------------------------------------------------------------------------
// 定数
//------------------------------------------------------------------------------
#define GLSL_VERSION	"#version 100\n"	// OpenGL ES 2.0 GLSL 1.0


//------------------------------------------------------------------------------
// プロトタイプ宣言
//------------------------------------------------------------------------------
/**
 * @brief		シェーダプログラム生成
 * @param[in]	pVShader 頂点シェーダプログラム
 * @param[in]	pFShader フラグメントシェーダプログラム
 * @return		プログラムID / 失敗時は0を返却
 */
UINT32 MP_CreateProgram(const char* pVShader, const char* pFShader);

/**
 * @brief		シェーダプログラムリンク
 * @param[in]	programId プログラムID
 * @return		シェーダオブジェクト / 失敗時は0を返却
 */
UINT32 MP_LinkShaderProgram(const UINT32 programId);

/**
 * @brief		シェーダプログラム解放
 * @param[in]	programId プログラムID
 */
void MP_DeleteProgram(const UINT32 programId);

#endif // _GLES_2

#endif	// _MP_SHADER_H
