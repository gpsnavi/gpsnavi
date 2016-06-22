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
#include "MP_Es1Emulation.h"
#include "MP_Shader.h"
#include "MP_Matrix.h"

#ifdef _GLES_2

//------------------------------------------------------------------------------
// 定数
//------------------------------------------------------------------------------
#if 0
/**
 * @brief		シェーダー：色をグローバル指定
 */
#define VSHADER_VERTEX_ARRAY  "\
	precision lowp float;\
	attribute vec4 a_pos;\
	uniform mat4 u_pmvMatrix;\
	void main(void)\
	{\
		gl_Position = u_pmvMatrix * a_pos;\
	}"

#define FSHADER_VERTEX_ARRAY  "\
	precision lowp float;\
	uniform vec4 u_color;\
	void main (void)\
	{\
		gl_FragColor = u_color;\
	}"


/**
 * @brief		シェーダー：色を個別指定
 */
#define VSHADER_COLOR_ARRAY  "\
	precision lowp float;\
	attribute vec4 a_pos;\
	attribute vec4 a_color;\
	uniform mat4 u_pmvMatrix;\
	varying vec4 v_color;\
	void main(void)\
	{\
		gl_Position = u_pmvMatrix * a_pos;\
		v_color = a_color;\
	}"

#define FSHADER_COLOR_ARRAY  "\
	precision lowp float;\
	varying vec4 v_color;\
	void main (void)\
	{\
		gl_FragColor = v_color;\
	}"


/**
 * @brief		シェーダー：テクスチャ用
 */
#define VSHADER_TEXTURE_ARRAY  "\
	precision lowp float;\
	attribute vec4 a_pos;\
	attribute vec2 a_texture;\
	uniform mat4 u_pmvMatrix;\
	varying vec2 v_texcoord;\
	void main(void)\
	{\
		gl_Position = u_pmvMatrix * a_pos;\
		v_texcoord = a_texture;\
	}"

#define FSHADER_TEXTURE_ARRAY  "\
	precision lowp float;\
	varying vec2 v_texcoord;\
	uniform sampler2D texture0;\
	void main (void)\
	{\
		gl_FragColor = texture2D(texture0, v_texcoord);\
	}"
#else
#define VSHADER_VERTEX_ARRAY  "\
	precision mediump float;\
	attribute vec4 a_pos;\
	attribute vec4 a_color;\
	attribute vec2 a_texture;\
	uniform mat4 u_pmvMatrix;\
	uniform vec4 u_color;\
	uniform bool u_color_arrey;\
	uniform bool u_texture_arrey;\
	varying vec4 v_color;\
	varying vec2 v_texcoord;\
	void main (void) \
	{\
		gl_Position = u_pmvMatrix * a_pos;\
		if (u_texture_arrey) {\
			v_texcoord = a_texture;\
		} else {\
			if (u_color_arrey) {\
				v_color = a_color;\
			} else {\
				v_color = u_color;\
			}\
		}\
	}"

#define FSHADER_VERTEX_ARRAY  "\
	precision mediump float;\
	uniform bool u_texture_arrey;\
	uniform sampler2D texture0;\
	varying vec2 v_texcoord;\
	varying vec4 v_color;\
	void main (void)\
	{\
		if(u_texture_arrey) {\
			gl_FragColor = texture2D(texture0, v_texcoord);\
		} else {\
			gl_FragColor = v_color;\
		}\
	}"
#endif
// スタックサイズ
#define STACK_SIZE					(10)

// attributeインデックス
#define ATTR_LOC_POS				(0)		// attribute vec4 a_pos;
#define ATTR_LOC_COLOR				(1)		// attribute vec4 a_color;
#define ATTR_LOC_TEXTURE			(2)		// attribute vec2 a_texture;


//------------------------------------------------------------------------------
// 構造体
//------------------------------------------------------------------------------
// シェーダ情報
typedef struct {
	GLuint		programId;		// プログラムID

	// attributeロケーション
	// リンク前に"attributeインデックス"に関連付け済み

	// uniformロケーション
	GLint		uPMVMatrix;		// プロジェクション*モデルビューマトリクス
	GLint		uColor;			// グローバルカラー
	GLint		uTexture0;		// テクスチャー
	GLint		uColorArrey;
	GLint		uTextureArrey;
} PROGRAM_INFO;

// カラー情報
typedef struct {
	FLOAT		r;
	FLOAT		g;
	FLOAT		b;
	FLOAT		a;
} COLOR_INFO;

// マトリクススタック
typedef struct {
	MPMatrix	mat[STACK_SIZE];			// スタック
	INT32		sp;							// スタックポインタ
	MPMatrix	cur;						// カレント
} MATRIX_FILO;

// OpenGL ES パラメータ
typedef struct {
	PROGRAM_INFO	program[PROGRAM_MAX];		// プログラム情報
	COLOR_INFO		color;						// グローバルカラー情報
	INT32			mode;						// マトリクスモード
	MATRIX_FILO		filo[2];					// スタック [0]:モデルビュー、[1]プロジェクション
	MPMatrix		*pMat;						// カレントマトリクス
} ES1PARAMS;


//------------------------------------------------------------------------------
// 静的変数
//------------------------------------------------------------------------------
// スレッド固有バッファのキー
static pthread_key_t buffer_key;
// 1 回限りのキーの初期化
static pthread_once_t buffer_key_once = PTHREAD_ONCE_INIT;


//------------------------------------------------------------------------------
// 内部関数プロトタイプ
//------------------------------------------------------------------------------
static void buffer_destroy(void *buf);
static void buffer_key_alloc();
static void threadBuffer_alloc(INT32 size);
static void initParams();
static ES1PARAMS *getParams();


//------------------------------------------------------------------------------
// 関数
//------------------------------------------------------------------------------
// スレッド固有のバッファを解放する
static void buffer_destroy(void *buf)
{
	free(buf);
}

// キーを確保する */
static void buffer_key_alloc()
{
	pthread_key_create(&buffer_key, buffer_destroy);
}

// スレッド固有のバッファを確保する
static void threadBuffer_alloc(INT32 size)
{
	pthread_once(&buffer_key_once, buffer_key_alloc);
	pthread_setspecific(buffer_key, malloc(size));
}

static void initParams()
{
	SC_LOG_DebugPrint(SC_TAG_MP, SC_LOG_START);

#ifdef _GL_PTHREAD_SAFE
	// スレッド固有のバッファを確保
	threadBuffer_alloc(sizeof(ES1PARAMS));
#endif	// _GL_PTHREAD_SAFE

	ES1PARAMS* param = getParams();

	// 初期化
	memset(param, 0, sizeof(ES1PARAMS));

	SC_LOG_DebugPrint(SC_TAG_MP, SC_LOG_END);
}

static ES1PARAMS *getParams()
{
#ifdef _GL_PTHREAD_SAFE
	// スレッド固有バッファから取得
	return ((ES1PARAMS*)pthread_getspecific(buffer_key));
#else
	static ES1PARAMS mParams;
	return (&mParams);
#endif	// _GL_PTHREAD_SAFE
}

int MP_InitEs1Emu()
{
	GLuint shaderProg;
	Bool ret = true;

	SC_LOG_DebugPrint(SC_TAG_MP, SC_LOG_START);

	// パラメータ初期化
	initParams();

	ES1PARAMS *param = getParams();

	do {
#if 0
		// SHADER_VERTEX_ARRAY
		{
			// シェーダプログラム生成
			shaderProg = MP_CreateProgram(VSHADER_VERTEX_ARRAY, FSHADER_VERTEX_ARRAY);
			if (shaderProg == 0) {
				ret = false;
				break;
			}

			// AttribLocationのindexを明示的に指定(リンク前にglBindAttribLocation())
			glBindAttribLocation(shaderProg, ATTR_LOC_POS, "a_pos");

			// シェーダプログラムリンク
			shaderProg = MP_LinkShaderProgram(shaderProg);
			if (shaderProg == 0) {
				ret = false;
				break;
			}

			param->program[PROGRAM_VERTEX_ARRAY].programId = shaderProg;
			param->program[PROGRAM_VERTEX_ARRAY].uPMVMatrix = glGetUniformLocation(shaderProg, "u_pmvMatrix");
			param->program[PROGRAM_VERTEX_ARRAY].uColor = glGetUniformLocation(shaderProg, "u_color");
		}

		// SHADER_COLOR_ARRAY
		{
			// シェーダプログラム生成
			shaderProg = MP_CreateProgram(VSHADER_COLOR_ARRAY, FSHADER_COLOR_ARRAY);
			if (shaderProg == 0) {
				ret = false;
				break;
			}

			// AttribLocationのindexを明示的に指定(リンク前にglBindAttribLocation())
			glBindAttribLocation(shaderProg, ATTR_LOC_POS, "a_pos");
			glBindAttribLocation(shaderProg, ATTR_LOC_COLOR, "a_color");

			// シェーダプログラムリンク
			shaderProg = MP_LinkShaderProgram(shaderProg);
			if (shaderProg == 0) {
				ret = false;
				break;
			}

			param->program[PROGRAM_COLOR_ARRAY].programId = shaderProg;
			param->program[PROGRAM_COLOR_ARRAY].uPMVMatrix = glGetUniformLocation(shaderProg, "u_pmvMatrix");
		}

		// SHADER_TEXTURE_ARRAY
		{
			// シェーダプログラム生成
			shaderProg = MP_CreateProgram(VSHADER_TEXTURE_ARRAY, FSHADER_TEXTURE_ARRAY);
			if (shaderProg == 0) {
				ret = false;
				break;
			}
			// AttribLocationのindexを明示的に指定(リンク前にglBindAttribLocation())
			glBindAttribLocation(shaderProg, ATTR_LOC_POS, "a_pos");
			glBindAttribLocation(shaderProg, ATTR_LOC_TEXTURE, "a_texture");

			// シェーダプログラムリンク
			shaderProg = MP_LinkShaderProgram(shaderProg);
			if (shaderProg == 0) {
				ret = false;
				break;
			}

			param->program[PROGRAM_TEXTURE_ARRAY].programId = shaderProg;
			param->program[PROGRAM_TEXTURE_ARRAY].uPMVMatrix = glGetUniformLocation(shaderProg, "u_pmvMatrix");
			param->program[PROGRAM_TEXTURE_ARRAY].uTexture0 = glGetUniformLocation(shaderProg, "texture0");
		}
#else
		// SHADER_VERTEX_ARRAY
		{
			// シェーダプログラム生成
			shaderProg = MP_CreateProgram(VSHADER_VERTEX_ARRAY, FSHADER_VERTEX_ARRAY);
			if (shaderProg == 0) {
				ret = false;
				break;
			}

			// AttribLocationのindexを明示的に指定(リンク前にglBindAttribLocation())
			glBindAttribLocation(shaderProg, ATTR_LOC_POS, "a_pos");
			glBindAttribLocation(shaderProg, ATTR_LOC_COLOR, "a_color");
			glBindAttribLocation(shaderProg, ATTR_LOC_TEXTURE, "a_texture");

			// シェーダプログラムリンク
			shaderProg = MP_LinkShaderProgram(shaderProg);
			if (shaderProg == 0) {
				ret = false;
				break;
			}

			param->program[PROGRAM_VERTEX_ARRAY].programId = shaderProg;
			param->program[PROGRAM_VERTEX_ARRAY].uPMVMatrix = glGetUniformLocation(shaderProg, "u_pmvMatrix");
			param->program[PROGRAM_VERTEX_ARRAY].uColor = glGetUniformLocation(shaderProg, "u_color");
			param->program[PROGRAM_VERTEX_ARRAY].uTexture0 = glGetUniformLocation(shaderProg, "texture0");
			param->program[PROGRAM_VERTEX_ARRAY].uColorArrey = glGetUniformLocation(shaderProg, "u_color_arrey");
			param->program[PROGRAM_VERTEX_ARRAY].uTextureArrey = glGetUniformLocation(shaderProg, "u_texture_arrey");
		}

		MP_UseProgram(0);
#endif
	} while (0);

	if (!ret) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"MP_GL_CreateShader err");
	}

	SC_LOG_DebugPrint(SC_TAG_MP, SC_LOG_END);

	return (ret);
}

void MP_UseProgram(int programType)
{
	ES1PARAMS *param = getParams();

	PROGRAM_INFO* program = &param->program[programType];

	// シェーダプログラムを選択
	glUseProgram(program->programId);
#if 0
	// プロジェクション*モデルビュー
	MPMatrix mat;
	MP_MultMatrix(&mat, &param->filo[1].cur, &param->filo[0].cur);

	// マトリクスUniform転送
	glUniformMatrix4fv(program->uPMVMatrix, 1, GL_FALSE, (GLfloat*)&mat);

	// シェーダ毎のパラメータ転送

	switch (programType)
	{
	case PROGRAM_VERTEX_ARRAY:
		glUniform4fv(program->uColor, 1, (GLfloat*)&param->color);
		glEnableVertexAttribArray(ATTR_LOC_POS);
		break;
	case PROGRAM_COLOR_ARRAY:
		glEnableVertexAttribArray(ATTR_LOC_POS);
		glEnableVertexAttribArray(ATTR_LOC_COLOR);
		break;
	case PROGRAM_TEXTURE_ARRAY:
		glEnableVertexAttribArray(ATTR_LOC_POS);
		glEnableVertexAttribArray(ATTR_LOC_TEXTURE);
		glUniform1i(program->uTexture0, 0);
		break;
	default:
		break;
	}
#else
	glUniform1i(program->uTextureArrey, 0);
	glUniform1i(program->uColorArrey, 0);
	glUniform1i(program->uTexture0, 0);
	glEnableVertexAttribArray(ATTR_LOC_POS);
#endif
}

void MP_LoadMatrix()
{
	ES1PARAMS *param = getParams();
	PROGRAM_INFO* program = &param->program[0];

	MPMatrix mat;
	MP_MultMatrix(&mat, &param->filo[1].cur, &param->filo[0].cur);

	// マトリクスUniform転送
	glUniformMatrix4fv(program->uPMVMatrix, 1, GL_FALSE, (GLfloat*)&mat);

	//glUniform4fv(program->uColor, 1, (GLfloat*)&param->color);
}

void GL_APIENTRY glEnableClientState (GLenum array)
{
	ES1PARAMS* param = getParams();
	PROGRAM_INFO* program = &param->program[0];

	switch (array)
	{
	case GL_COLOR_ARRAY:			/* カラー配列。glColorPointer() を参照 */
		glUniform1i(program->uColorArrey, 1);
		glEnableVertexAttribArray(ATTR_LOC_COLOR);
		break;
	case GL_NORMAL_ARRAY:			/* 法線配列。glNormalPointer() を参照 */
		break;
	case GL_TEXTURE_COORD_ARRAY:	/* テクスチャ座標配列。glTexCoordPointer() を参照 */
		glUniform1i(program->uTextureArrey, 1);
		glEnableVertexAttribArray(ATTR_LOC_TEXTURE);
		break;
	case GL_VERTEX_ARRAY:			/* 頂点配列。glVertexPointer() を参照 */
		glEnableVertexAttribArray(ATTR_LOC_POS);
		break;
	default:
		break;
	}
}
void GL_APIENTRY glDisableClientState (GLenum array)
{
	ES1PARAMS* param = getParams();
	PROGRAM_INFO* program = &param->program[0];

	switch (array)
	{
	case GL_COLOR_ARRAY:			/* カラー配列。glColorPointer() を参照 */
		glUniform1i(program->uColorArrey, 0);
		glDisableVertexAttribArray(ATTR_LOC_COLOR);
		break;
	case GL_NORMAL_ARRAY:			/* 法線配列。glNormalPointer() を参照 */
		break;
	case GL_TEXTURE_COORD_ARRAY:	/* テクスチャ座標配列。glTexCoordPointer() を参照 */
		glUniform1i(program->uTextureArrey, 0);
		glDisableVertexAttribArray(ATTR_LOC_TEXTURE);
		break;
	case GL_VERTEX_ARRAY:			/* 頂点配列。glVertexPointer() を参照 */
		glDisableVertexAttribArray(ATTR_LOC_POS);
		break;
	default:
		break;
	}
}

void GL_APIENTRY glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	ES1PARAMS* param = getParams();
	PROGRAM_INFO* program = &param->program[0];

	param->color.r = red;
	param->color.g = green;
	param->color.b = blue;
	param->color.a = alpha;

	glUniform4fv(program->uColor, 1, (GLfloat*)&param->color);
}

void GL_APIENTRY glPushMatrix(void)
{
	ES1PARAMS	*param = getParams();
	MATRIX_FILO	*filo = &param->filo[param->mode];

	if (filo->sp < STACK_SIZE) {
		memcpy(&filo->mat[(filo->sp)++], &filo->cur, sizeof(MPMatrix));
	}
}

void GL_APIENTRY glPopMatrix(void)
{
	ES1PARAMS	*param = getParams();
	MATRIX_FILO	*filo = &param->filo[param->mode];

	if (filo->sp > 0) {
		memcpy(&filo->cur, &filo->mat[--(filo->sp)], sizeof(MPMatrix));
	}
}

void GL_APIENTRY glMatrixMode(GLenum mode)
{
	ES1PARAMS *param = getParams();

	param->mode = (GL_MODELVIEW^mode);
	param->pMat = &param->filo[param->mode].cur;
}

void GL_APIENTRY glLoadIdentity(void)
{
	MP_LoadIdentityMatrix(getParams()->pMat);
}

void GL_APIENTRY glOrthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar)
{
	MP_OrthoMatrix(getParams()->pMat, left, right, bottom,top, zNear, zFar);
}

void GL_APIENTRY glRotatef (GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	MP_RotateMatrix(getParams()->pMat, angle, x, y, z);
}

void GL_APIENTRY glScalef (GLfloat x, GLfloat y, GLfloat z)
{
	MP_ScaleMatrix(getParams()->pMat, x, y, z);
}

void GL_APIENTRY glTranslatef (GLfloat x, GLfloat y, GLfloat z)
{
	MP_TranslateMatrix(getParams()->pMat, x, y, z);
}

void GL_APIENTRY glVertexPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	glVertexAttribPointer(ATTR_LOC_POS, size, type, GL_FALSE, sizeof(FLOAT)*size, pointer);
}

void GL_APIENTRY glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	glVertexAttribPointer(ATTR_LOC_COLOR, size, type, GL_TRUE, sizeof(BYTE)*size, pointer);
}

void GL_APIENTRY glTexCoordPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	glVertexAttribPointer(ATTR_LOC_TEXTURE, size, type, GL_FALSE, sizeof(FLOAT)*size, pointer);
}

#endif // _GLES_2
