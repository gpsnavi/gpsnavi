/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 * Copyright (c) 2016  Aisin AW, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

/*
 * glview.h
 *
 *  Created on: 2015/11/05
 *      Author:t.aikawa
 */

#ifndef GLVIEW_H
#define GLVIEW_H

#define	 NAVI_OPENGLES2

#ifdef  NAVI_OPENGLES2
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#define _GLES_2
#include "SMCoreMP/es1emu/MP_Es1Emulation.h"
#else
#include <GLES/gl.h>
#include <GLES/glext.h>
#endif /* NAVI_OPENGLES2 */
#include <EGL/egl.h>
#include "pthread_msq.h"
#include "pthread_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GLV_OPENGL_ES1_API	(1)
#define GLV_OPENGL_ES2_API	(2)

#define GLV_DEBUG_OFF	(0)
#define GLV_DEBUG_ON	(1)

#define GLV_KEYBOARD_KEY_STATE_RELEASED		(0)
#define GLV_KEYBOARD_KEY_STATE_PRESSED		(1)

#define GLV_ERROR	PTHREAD_TIMER_ERROR
#define GLV_OK		PTHREAD_TIMER_OK

#define GLV_TIMER_ONLY_ONCE		PTHREAD_TIMER_ONLY_ONCE
#define GLV_TIMER_REPEAT		PTHREAD_TIMER_REPEAT

#define GLV_GESTURE_EVENT_DOWN			(1)
#define GLV_GESTURE_EVENT_SINGLE_UP		(2)
#define GLV_GESTURE_EVENT_SCROLL		(3)
#define GLV_GESTURE_EVENT_FLING			(4)
#define GLV_GESTURE_EVENT_SCROLL_STOP	(5)

//-----------------------------------
// 型定義
//-----------------------------------
typedef	void	*GLVDisplay;		//
typedef	void	*GLVWindow;			//
typedef	void	*GLVContext;		//

typedef unsigned long glvTime;

//-----------------------------------
// 関数ポインタ定義
//-----------------------------------
typedef struct _glveventfunc {
	int (*init)(GLVContext glv_ctx,int maps);
	int (*reshape)(GLVContext glv_ctx,int maps,int width, int height);
	int (*redraw)(GLVContext glv_ctx,int maps);
	int (*update)(GLVContext glv_ctx,int maps);
	int (*timer)(GLVContext glv_ctx,int maps,int group,int id);
	int (*gesture)(GLVContext glv_ctx,int maps,int eventType,int x,int y,int distance_x,int distance_y,int velocity_x,int velocity_y);
} GLVEVENTFUNC_t;

typedef struct _glvinputfunc {
	int (*keyboard_key)(unsigned int key,unsigned int state);
	int (*touch_down)(int pointer_sx,int pointer_sy);
	int (*touch_up)(int pointer_sx,int pointer_sy);
} GLVINPUTFUNC_t;

extern GLVINPUTFUNC_t	glv_input_func;

//-----------------------------------
void		glvSetDebugFlag(int flag);
GLVDisplay	glvOpenDisplay(char *dpyName);
int			glvCloseDisplay(GLVDisplay glv_dpy);
EGLNativeDisplayType glvGetNativeDisplay(GLVDisplay glv_dpy);
EGLDisplay	glvGetEGLDisplay(GLVDisplay glv_dpy);
EGLConfig	glvGetEGLConfig(GLVDisplay glv_dpy);
EGLint		glvGetEGLVisualid(GLVDisplay glv_dpy);

GLVWindow glvCreateNativeWindow(GLVDisplay glv_dpy,int x, int y, int width, int height,GLVWindow glv_win_parent);

void glvDestroyNativeWindow(GLVWindow glv_win);

GLVContext	glvCreateSurfaceView(GLVWindow glv_win,int maps,GLVEVENTFUNC_t *eventFunc);

void glvSwapBuffers(GLVContext glv_c);

int glvOnReShape(GLVContext glv_ctx,int width, int height);
int glvOnReDraw(GLVContext glv_ctx);
int glvOnUpdate(GLVContext glv_c);
int glvOnGesture(GLVContext glv_c,int eventType,int x,int y,int distance_x,int distance_y,int velocity_x,int velocity_y);
int glvOnActivate(GLVContext glv_c);

int glvCheckTimer(GLVContext glv_c,int id,int count);
void glvInitTimer(void);
int glvCreateTimer(GLVContext glv_c,int group,int id,int type,int mTime);
int glvStartTimer(GLVContext glv_c,int id);
int glvStopTimer(GLVContext glv_c,int id);

void glvEventLoop(GLVDisplay glv_dpy);
void glvActivateSurface();

#ifdef __cplusplus
}
#endif
#endif /* GLVIEW_H */

