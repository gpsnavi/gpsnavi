/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#include "sms-core/SMCoreMP/SMCoreMPInternal.h"
#include "MP_Shader.h"

#ifdef _GLES_2


//------------------------------------------------------------------------------
// 内部関数プロトタイプ
//------------------------------------------------------------------------------
/**
 * @brief		シェーダプログラムコンパイル
 * @param[in]	type GL_VERTEX_SHADER:頂点シェーダ / GL_FRAGMENT_SHADER:フラグメントシェーダ
 * @param[in]	pProgram プログラム文字列
 * @return		コンパイル結果生成されるシェーダオブジェクト / 失敗時は0を返却
 */
static UINT32 MP_CompileShaderProgram(INT32 type, const char *pProgram);


//------------------------------------------------------------------------------
// 関数
//------------------------------------------------------------------------------
static UINT32 MP_CompileShaderProgram(INT32 type, const char *pProgram)
{
	GLuint shaderId;
	GLint shaderCompiled;
	const GLchar* sources[2] = {
			(const GLchar*)GLSL_VERSION,
			(const GLchar*)pProgram
		};

	// シェーダオブジェクトの作成
	shaderId = glCreateShader((GLenum)type);

	// ソースコードをシェーダオブジェクトに変換
	glShaderSource(shaderId, 2, (const char**)sources, NULL);

	// コンパイル
	glCompileShader(shaderId);

	// コンパイルエラーチェック
	glGetShaderiv(shaderId, GL_COMPILE_STATUS, &shaderCompiled);
	if (!shaderCompiled) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"MP_CompileShaderProgram err");
		return (0);
	}

	return (shaderId);
}

UINT32 MP_CreateProgram(const char* pVShader, const char* pFShader)
{
	GLuint vshaderId;
	GLuint fshaderId;

	SC_LOG_DebugPrint(SC_TAG_MP, SC_LOG_START);

	// 頂点シェーダコンパイル
	vshaderId = MP_CompileShaderProgram(GL_VERTEX_SHADER, pVShader);
	if (vshaderId == 0) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"MP_CompileShaderProgram err");
		return (0);
	}

	// フラグメントシェーダコンパイル
	fshaderId = MP_CompileShaderProgram(GL_FRAGMENT_SHADER, pFShader);
	if (fshaderId == 0) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"MP_CompileShaderProgram err");
		return (0);
	}

	// シェーダプログラム作成
	GLuint programId = glCreateProgram();

	// シェーダプログラムにシェーダオブジェクト登録
	glAttachShader(programId, fshaderId);
	glAttachShader(programId, vshaderId);

	// シェーダオブジェクトは不要な為削除
	glDeleteShader(vshaderId);
	glDeleteShader(fshaderId);

	SC_LOG_DebugPrint(SC_TAG_MP, (Char*)"pId=%d, vId=%d, fId=%d", programId, fshaderId, vshaderId);
	SC_LOG_DebugPrint(SC_TAG_MP, SC_LOG_END);

	return (programId);
}

UINT32 MP_LinkShaderProgram(const UINT32 programId)
{
	GLint bLinked;

	SC_LOG_DebugPrint(SC_TAG_MP, SC_LOG_START);

	// プログラムをリンク
	glLinkProgram(programId);

	// リンクエラー判定
	glGetProgramiv(programId, GL_LINK_STATUS, &bLinked);
	if (!bLinked) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"MP_LinkShaderProgram err");
		return (0);
	}

	SC_LOG_DebugPrint(SC_TAG_MP, SC_LOG_END);

	return (programId);
}

void MP_DeleteProgram(const UINT32 programId)
{
	SC_LOG_DebugPrint(SC_TAG_MP, SC_LOG_START);

	glUseProgram(0);
	glDeleteProgram(programId);

	SC_LOG_DebugPrint(SC_TAG_MP, (Char*)"pId=%d", programId);
	SC_LOG_DebugPrint(SC_TAG_MP, SC_LOG_END);
}

#endif // _GLES_2
