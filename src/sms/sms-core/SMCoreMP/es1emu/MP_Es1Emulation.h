/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef _MP_ES1EMULATION_H
#define _MP_ES1EMULATION_H

#ifdef _GLES_2

// シェーダ関連
// シェーダ
enum {
	PROGRAM_VERTEX_ARRAY = 0,
//	PROGRAM_COLOR_ARRAY,
//	PROGRAM_TEXTURE_ARRAY,
	PROGRAM_MAX
};

/**
 * @brief	初期化
 */
int MP_InitEs1Emu();

/**
 * @brief	プログラム設定
 */
void MP_UseProgram(int programType);
void MP_LoadMatrix();

/**
 * opengl es 1 emulation
 */
#ifndef GL_API
#define GL_API
#endif // GL_API
#ifndef GL_APIENTRY
#define GL_APIENTRY
#endif // GL_APIENTRY

/* MatrixMode */
#define GL_MODELVIEW                      0x1700
#define GL_PROJECTION                     0x1701

#define GL_VERTEX_ARRAY                   0x8074
#define GL_NORMAL_ARRAY                   0x8075
#define GL_COLOR_ARRAY                    0x8076
#define GL_TEXTURE_COORD_ARRAY            0x8078

GL_API void GL_APIENTRY glEnableClientState (GLenum array);
GL_API void GL_APIENTRY glDisableClientState (GLenum array);
GL_API void GL_APIENTRY glColor4f (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
GL_API void GL_APIENTRY glPopMatrix (void);
GL_API void GL_APIENTRY glPushMatrix (void);
GL_API void GL_APIENTRY glMatrixMode (GLenum mode);
GL_API void GL_APIENTRY glLoadIdentity (void);
GL_API void GL_APIENTRY glOrthof (GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar);
GL_API void GL_APIENTRY glRotatef (GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
GL_API void GL_APIENTRY glScalef (GLfloat x, GLfloat y, GLfloat z);
GL_API void GL_APIENTRY glTranslatef (GLfloat x, GLfloat y, GLfloat z);

GL_API void GL_APIENTRY glVertexPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GL_API void GL_APIENTRY glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GL_API void GL_APIENTRY glTexCoordPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);

#endif // _GLES_2

#endif	// _MP_ES1EMULATION_H
