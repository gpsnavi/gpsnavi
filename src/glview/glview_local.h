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
 * glview_local.h
 *
 *  Created on: 2015/11/05
 *      Author:t.aikawa
 */

#ifndef GLVIEW_LOCAL_H
#define GLVIEW_LOCAL_H

#define GLV_MOUSE_EVENT_PRESS			(1)
#define GLV_MOUSE_EVENT_RELEASE			(2)
#define GLV_MOUSE_EVENT_MOTION			(3)

typedef struct _wldisplay {
   struct wl_display		*display;
   struct wl_compositor		*compositor;
   struct wl_subcompositor	*subcompositor;
   struct wl_shell			*shell;
} WLDISPLAY_t;

typedef struct _wlwindow {
	struct wl_surface		*parent;
	struct wl_surface		*surface;
	struct wl_subsurface	*subsurface;
	struct wl_shell_surface	*shell_surface;
	struct wl_callback		*callback;
	int x, y;
	int width, height;
} WLWINDOW_t;

typedef struct _glvdisplay {
	const char				*display_name;
	EGLNativeDisplayType	native_dpy;
	EGLDisplay				egl_dpy;
	EGLConfig				config;
	EGLint					vid;
	WLDISPLAY_t				wl_dpy;
} GLVDISPLAY_t;

typedef struct _glvnativeWindow {
	GLVDISPLAY_t		*glv_dpy;
	EGLNativeWindowType	egl_window;
	WLWINDOW_t			wl_window;
}GLVWINDOW_t;

typedef struct _glvContext {
	pthread_t			threadId;
	GLVWINDOW_t			*glv_win;
	int					maps;
	int					api;
	EGLSurface			egl_surf;
	EGLContext			egl_ctx;
	GLVEVENTFUNC_t		eventFunc;
	pthread_msq_id_t	queue;
} GLVCONTEXT_t;

typedef struct _glvMouseEvent {
	int			type;
	glvTime		time;
	int			x;
	int			y;
} GLVMOUSEEVENT_t;

void _glvInitNativeDisplay(GLVDISPLAY_t *glv_dpy);
void _glvOpenNativeDisplay(GLVDISPLAY_t *glv_dpy);
void _glvCloseNativeDisplay(GLVDISPLAY_t *glv_dpy);

GLVWindow _glvCreateNativeWindow(GLVDISPLAY_t *glv_dpy,int x, int y, int width, int height,GLVWindow glv_win_parent);
void _glvDestroyNativeWindow(GLVWindow glv_win);

#endif /* GLVIEW_LOCAL_H */

