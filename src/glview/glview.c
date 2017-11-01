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
 * glview.c
 *
 *  Created on: 2015/11/05
 *      Author:t.aikawa
 */

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "glview.h"
#include "glview_local.h"

#define GLV_ON_RESHAPE	(1)
#define GLV_ON_REDRAW	(2)
#define GLV_ON_UPDATE	(3)
#define GLV_ON_TIMER	(4)
#define GLV_ON_GESTURE	(5)

GLVContext _glv_parent_context=0;

#define GLV_DEBUG	if(glv_debug_flag == GLV_DEBUG_ON)

static int glv_debug_flag = GLV_DEBUG_OFF;

void glvSetDebugFlag(int flag){
	glv_debug_flag = flag;
}

int canUpdate(void);

int glvCheckTimer(GLVContext glv_c,int id,int count);
/* ---------------------------------------------------------- */
GLVDisplay glvOpenDisplay(char *dpyName)
{
	EGLDisplay egl_dpy;
	GLVDISPLAY_t *glv_dpy;
	EGLint egl_major, egl_minor;
	EGLBoolean rc;
	EGLint num_configs;
	static const EGLint attribs[] = {
#ifdef NAVI_OPENGLES2
	  EGL_RENDERABLE_TYPE,EGL_OPENGL_ES2_BIT,
#endif
	  EGL_RED_SIZE, 1,
	  EGL_GREEN_SIZE, 1,
	  EGL_BLUE_SIZE, 1,
	  EGL_ALPHA_SIZE, 1,		// append 2015.12.10 by T.Aikawa
	  EGL_DEPTH_SIZE, 1,
	  EGL_NONE
	};

	glv_dpy =(GLVDISPLAY_t *)malloc(sizeof(GLVDISPLAY_t));
	if(!glv_dpy){
		return(0);
	}
	glv_dpy->display_name = dpyName;

	_glvInitNativeDisplay(glv_dpy);

	_glvOpenNativeDisplay(glv_dpy);

	egl_dpy = eglGetDisplay(glv_dpy->native_dpy);
	if(!egl_dpy){
		free(glv_dpy);
		return(0);
	}

	rc = eglInitialize(egl_dpy, &egl_major, &egl_minor);
	if(!rc){
		eglTerminate(egl_dpy);
		free(glv_dpy);
		return(0);
	}

	printf("EGL_VERSION    = %s\n", eglQueryString(egl_dpy, EGL_VERSION));
	printf("EGL_VENDOR     = %s\n", eglQueryString(egl_dpy, EGL_VENDOR));
	//printf("EGL_EXTENSIONS = %s\n", eglQueryString(egl_dpy, EGL_EXTENSIONS));
	printf("EGL_CLIENT_APIS= %s\n", eglQueryString(egl_dpy, EGL_CLIENT_APIS));

	if (!eglChooseConfig(egl_dpy,attribs,&glv_dpy->config,1,&num_configs)) {
		eglTerminate(egl_dpy);
		free(glv_dpy);
		return(0);
	}

#if 0
	if (!eglGetConfigAttrib(egl_dpy,glv_dpy->config,EGL_NATIVE_VISUAL_ID,&glv_dpy->vid)) {
		eglTerminate(egl_dpy);
		free(glv_dpy);
		return(0);
	}
#endif

	glv_dpy->egl_dpy = egl_dpy;

	return ((GLVDisplay)glv_dpy);
}

int glvCloseDisplay(GLVDisplay glv_dpy)
{
	eglTerminate(((GLVDISPLAY_t*)glv_dpy)->egl_dpy);
	_glvCloseNativeDisplay((GLVDISPLAY_t*)glv_dpy);
	free(glv_dpy);
	return(GLV_OK);
}

GLVWindow glvCreateNativeWindow(GLVDisplay glv_dpy,
              int x, int y, int width, int height,
			  GLVWindow glv_window_parent)
{
	GLVWindow glv_window;
	glv_window = _glvCreateNativeWindow((GLVDISPLAY_t*)glv_dpy, x, y, width, height,glv_window_parent);
	return(glv_window);
}

void glvDestroyNativeWindow(GLVWindow glv_win)
{
	_glvDestroyNativeWindow(glv_win);
}

EGLNativeDisplayType glvGetNativeDisplay(GLVDisplay glv_dpy)
{
	return (((GLVDISPLAY_t*)glv_dpy)->native_dpy);
}

EGLDisplay glvGetEGLDisplay(GLVDisplay glv_dpy)
{
	return ((EGLDisplay)((GLVDISPLAY_t*)glv_dpy)->egl_dpy);
}

EGLConfig glvGetEGLConfig(GLVDisplay glv_dpy)
{
	return ((EGLDisplay)((GLVDISPLAY_t*)glv_dpy)->config);
}

EGLint glvGetEGLVisualid(GLVDisplay glv_dpy)
{
	return (((GLVDISPLAY_t*)glv_dpy)->vid);
}

void glvSwapBuffers(GLVContext glv_c)
{
	GLVCONTEXT_t *glv_context;

	glv_context = (GLVCONTEXT_t*)glv_c;


	//if (canUpdate() == 1)
	{
		eglSwapBuffers(glv_context->glv_win->glv_dpy->egl_dpy, glv_context->egl_surf);
	}
}

void *glvSurfaceViewMsgHandler(GLVCONTEXT_t *glv_context)
{
	int	rc = 0;
	static int drawCount = 0;
	pthread_msq_msg_t	rmsg = {};

	while (1) {
		// メッセージ初期化
		memset(&rmsg, 0, sizeof(pthread_msq_msg_t));

		// メッセージ受信
		rc = pthread_msq_msg_receive(&glv_context->queue, &rmsg);
		if (PTHREAD_MSQ_OK != rc) {

			continue;
		}
#if 0
		GLV_DEBUG {
			pthread_msq_id_t *queue;
			queue = &glv_context->queue;
			printf("queue : %d %d %d\n",
			queue->maxMsgQueueNum,
			queue->fifoIndex,
			queue->queueNum);
		}
#endif
		switch(rmsg.data[0]){
		case GLV_ON_RESHAPE:
			GLV_DEBUG printf("GLV_ON_RESHAPE(%d,%d)\n",(int)rmsg.data[2],(int)rmsg.data[3]);
			/* 描画サイズ変更 */
			if(glv_context->eventFunc.reshape != NULL){
				int rc;
				rc = (glv_context->eventFunc.reshape)(glv_context,glv_context->maps,rmsg.data[2],rmsg.data[3]);
				if(rc != GLV_OK){
					fprintf(stderr,"glv_context->eventFunc.reshape error\n");
				}
			}
			break;
		case GLV_ON_REDRAW:
			drawCount++;
			GLV_DEBUG printf("GLV_ON_REDRAW   count = %d\n",drawCount);
			/* 描画 */
			if(glv_context->eventFunc.redraw != NULL){
				int rc;
				rc = (glv_context->eventFunc.redraw)(glv_context,glv_context->maps);
				if(rc != GLV_OK){
					fprintf(stderr,"glv_context->eventFunc.redraw error\n");
				}
			}
			//if (canUpdate() == 1)
			{
				eglSwapBuffers(glv_context->glv_win->glv_dpy->egl_dpy, glv_context->egl_surf);
			}
			break;
		case GLV_ON_UPDATE:
			GLV_DEBUG printf("GLV_ON_UPDATE   count = %d\n",drawCount);
			/* 描画 */
			if(glv_context->eventFunc.update != NULL){
				int rc;
				rc = (glv_context->eventFunc.update)(glv_context,glv_context->maps);
				if(rc != GLV_OK){
					fprintf(stderr,"glv_context->eventFunc.update error\n");
				}
			}
			break;
		case GLV_ON_TIMER:
			GLV_DEBUG printf("GLV_ON_TIMER\n");
			if(glvCheckTimer(glv_context,rmsg.data[3],rmsg.data[4]) == GLV_OK){
				//printf("GLV_ON_TIMER id = %d OK\n",rmsg.data[3]);
				if(glv_context->eventFunc.timer != NULL){
					int rc;
					rc = (glv_context->eventFunc.timer)(glv_context,glv_context->maps,rmsg.data[2],rmsg.data[3]);
					if(rc != GLV_OK){
						fprintf(stderr,"glv_context->eventFunc.timer error\n");
					}
				}
			}else{
				//printf("GLV_ON_TIMER id = %d IGNORE\n",rmsg.data[3]);
			}
			break;
		case GLV_ON_GESTURE:
			GLV_DEBUG printf("GLV_ON_GESTURE\n");
			if(glv_context->eventFunc.gesture != NULL){
				int rc;
				rc = (glv_context->eventFunc.gesture)(glv_context,glv_context->maps,
						rmsg.data[2],rmsg.data[3],rmsg.data[4],rmsg.data[5],rmsg.data[6],rmsg.data[7],rmsg.data[8]);
				if(rc != GLV_OK){
					fprintf(stderr,"glv_context->eventFunc.gesture error\n");
				}
			}
			break;
		default:
			break;
		}
	}
	return NULL;
}

void *glvSurfaceViewProc(void *param)
{
	GLVCONTEXT_t *glv_context;
	EGLDisplay egl_dpy;
	EGLConfig config;
	EGLenum api;
	EGLNativeWindowType win;
	EGLSurface egl_surf;
	EGLContext egl_ctx;
	static int instanceCount=0;

	glv_context = (GLVCONTEXT_t*)param;
	egl_dpy = glv_context->glv_win->glv_dpy->egl_dpy;
	config  = glv_context->glv_win->glv_dpy->config;
	win = glv_context->glv_win->egl_window;

#if 0
	switch(glv_context->api){
		case GLV_OPENGL_ES1_API:
			api = EGL_OPENGL_ES_API;
			break;
		case GLV_OPENGL_ES2_API:
			api = EGL_OPENGL_ES_API;
			break;
		default:
			api = EGL_OPENGL_ES_API;
			break;
	}

	eglBindAPI(api);

	egl_ctx = eglCreateContext(egl_dpy, config, EGL_NO_CONTEXT, NULL );
	if (!egl_ctx) {
	  printf("glvSurfaceViewProc:Error: eglCreateContext failed\n");
	  exit(-1);
	}
#else
	switch(glv_context->api){
		case GLV_OPENGL_ES1_API:
			api = 1;
			break;
		case GLV_OPENGL_ES2_API:
			api = 2;
			break;
		default:
			api = 2;
			break;
	}
	//// egl-contexts collect all state descriptions needed required for operation
	EGLint ctxattr[] = {
	  EGL_CONTEXT_CLIENT_VERSION, 2,	/* 1:opengles1.1 2:opengles2.0 */
	  EGL_NONE
	};
	ctxattr[1] = api;
	egl_ctx = eglCreateContext ( egl_dpy, config, EGL_NO_CONTEXT, ctxattr );
	if(egl_ctx == EGL_NO_CONTEXT ) {
	   fprintf(stderr,"glvSurfaceViewProc:Unable to create EGL context (eglError: %d)\n", eglGetError());
	   exit(-1);
	}
#endif
	glv_context->egl_ctx = egl_ctx;

	egl_surf = eglCreateWindowSurface(egl_dpy, config, win, NULL);

   if (!egl_surf) {
      fprintf(stderr,"glvSurfaceViewProc:Error: eglCreateWindowSurface failed\n");
      exit(-1);
   }
   glv_context->egl_surf = egl_surf;

   if (!eglMakeCurrent(egl_dpy, egl_surf, egl_surf, egl_ctx)) {
      fprintf(stderr,"glvSurfaceViewProc:Error: eglMakeCurrent() failed\n");
      exit(-1);
   }
   if(instanceCount == 0){
		printf("GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
		printf("GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
		//printf("GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
		//printf("GL_EXTENSIONS = %s\n", (char *) glGetString(GL_EXTENSIONS));
   }
   instanceCount++;

#ifdef NAVI_OPENGLES2
   MP_InitEs1Emu();
#endif


   /* 初期化 */
   if(glv_context->eventFunc.init != NULL){
	   int rc;
	   rc = (glv_context->eventFunc.init)(glv_context,glv_context->maps);
	   if(rc != GLV_OK){
		   fprintf(stderr,"glv_context->eventFunc.init error\n");
	   }
   }

   glvSurfaceViewMsgHandler(glv_context);

   eglDestroyContext(egl_dpy, egl_ctx);
   eglDestroySurface(egl_dpy, egl_surf);

   return(NULL);
}

GLVContext glvCreateSurfaceView(GLVWindow glv_win,int maps,GLVEVENTFUNC_t *eventFunc)
{
	int			pret = 0;
	int			api;
	pthread_t 	threadId;
	GLVCONTEXT_t *glv_context;
	pthread_msq_id_t queue = PTHREAD_MSQ_ID_INITIALIZER;

	glv_context =(GLVCONTEXT_t *)malloc(sizeof(GLVCONTEXT_t));
	if(!glv_context){
		return(0);
	}

#ifdef NAVI_OPENGLES2
   api = GLV_OPENGL_ES2_API;
#else
   api = GLV_OPENGL_ES1_API;
#endif

	glv_context->glv_win   = (GLVWINDOW_t *)glv_win;
	glv_context->api       = api;
	glv_context->maps      = maps;

	memcpy(&glv_context->queue,&queue,sizeof(pthread_msq_id_t));
	memcpy(&glv_context->eventFunc,eventFunc,sizeof(GLVEVENTFUNC_t));

	// メッセージキュー生成
	if (0 != pthread_msq_create(&glv_context->queue, 50)) {
		fprintf(stderr,"glvSurfaceViewProc:Error: pthread_msq_create() failed\n");
		exit(-1);
	}
	// スレッド生成
	pret = pthread_create(&threadId, NULL, glvSurfaceViewProc, (void *)glv_context);

	if (0 != pret) {
		free(glv_context);
		return(0);
	}

	glv_context->threadId = threadId;

	if(glv_context->glv_win->wl_window.parent == 0){
		_glv_parent_context = glv_context;
	}

	return (glv_context);
}

int glvOnReShape(GLVContext glv_c,int width, int height)
{
	GLVCONTEXT_t *glv_context;
	pthread_msq_msg_t smsg;

	glv_context = (GLVCONTEXT_t*)glv_c;

	smsg.data[0] = GLV_ON_RESHAPE;
	smsg.data[1] = glv_context->maps;
	smsg.data[2] = width;
	smsg.data[3] = height;
	GLV_DEBUG printf("glvOnReShape  RESHAPE %d,%d\n",width,height);
	pthread_msq_msg_send(&glv_context->queue,&smsg,0);
	return (GLV_OK);
}

int glvOnReDraw(GLVContext glv_c)
{
	GLVCONTEXT_t *glv_context;
	pthread_msq_msg_t smsg;

	glv_context = (GLVCONTEXT_t*)glv_c;

	smsg.data[0] = GLV_ON_REDRAW;
	smsg.data[1] = glv_context->maps;
	smsg.data[2] = 0;
	smsg.data[3] = 0;
	//GLV_DEBUG printf("glvOnReDraw \n");
	pthread_msq_msg_send(&glv_context->queue,&smsg,0);
	return (GLV_OK);
}

int glvOnUpdate(GLVContext glv_c)
{
	GLVCONTEXT_t *glv_context;
	pthread_msq_msg_t smsg;

	glv_context = (GLVCONTEXT_t*)glv_c;

	smsg.data[0] = GLV_ON_UPDATE;
	smsg.data[1] = glv_context->maps;
	smsg.data[2] = 0;
	smsg.data[3] = 0;
	//GLV_DEBUG printf("glvOnUpdate \n");
	pthread_msq_msg_send(&glv_context->queue,&smsg,0);
	return (GLV_OK);
}

int glvOnGesture(GLVContext glv_c,int eventType,int x,int y,int distance_x,int distance_y,int velocity_x,int velocity_y)
{
	GLVCONTEXT_t *glv_context;
	pthread_msq_msg_t smsg;

	glv_context = (GLVCONTEXT_t*)glv_c;

	smsg.data[0] = GLV_ON_GESTURE;
	smsg.data[1] = glv_context->maps;
	smsg.data[2] = eventType;
	smsg.data[3] = x;
	smsg.data[4] = y;
	smsg.data[5] = distance_x;
	smsg.data[6] = distance_y;
	smsg.data[7] = velocity_x;
	smsg.data[8] = velocity_y;
	//GLV_DEBUG printf("glvOnGesture \n");
	pthread_msq_msg_send(&glv_context->queue,&smsg,0);
	return (GLV_OK);
}


int glvCheckTimer(GLVContext glv_c,int id,int count)
{
	GLVCONTEXT_t *glv_context;
	glv_context = (GLVCONTEXT_t*)glv_c;
	int rc;

	rc = pthreadCheckTimer(glv_context->threadId,id,count);
	return(rc);
}
void glvInitTimer(void)
{
	pthreadInitTimer();
}
int glvCreateTimer(GLVContext glv_c,int group,int id,int type,int mTime)
{
	GLVCONTEXT_t *glv_context;
	glv_context = (GLVCONTEXT_t*)glv_c;
	int rc;

	rc = pthreadCreateTimer(glv_context->threadId,&glv_context->queue,GLV_ON_TIMER,group,id,type,mTime);

	return(rc);
}
int glvStartTimer(GLVContext glv_c,int id)
{
	GLVCONTEXT_t *glv_context;
	glv_context = (GLVCONTEXT_t*)glv_c;
	int rc;
	rc = pthreadStartTimer(glv_context->threadId,id);
	return(rc);
}
int glvStopTimer(GLVContext glv_c,int id)
{
	GLVCONTEXT_t *glv_context;
	glv_context = (GLVCONTEXT_t*)glv_c;
	int rc;
	rc = pthreadStopTimer(glv_context->threadId,id);
	return(rc);
}

