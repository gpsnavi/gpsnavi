/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 * Copyright (c) 2016  Aisin AW, Ltd
 *
 * This program is licensed under GPL version 2 license.
 * See the LICENSE file distributed with this source file.
 */

/*
 * glview_wayland.c
 *
 *  Created on: 2015/11/05
 *      Author:t.aikawa
 */


#define GLV_WAYLAND_INPUT
#define GLV_WAYLAND_TOUCH

#include <stdio.h>
#ifdef GLV_WAYLAND_INPUT
#include <linux/input.h> // for BTN_LEFT
#endif /* GLV_WAYLAND_INPUT */

#include <wayland-client.h>
#include <wayland-egl.h>

#define	IVISHELL	(1)
//#define IVI_SURFACE_ID (0x1302)
#include "ivi-application-client-protocol.h"

#include <poll.h>
#include <errno.h>
#include <string.h>

extern "C" {
#include "glview.h"
#include "glview_local.h"
}

#include <libwindowmanager.h>
#include <libhomescreen.hpp>
#include "NaviTrace.h"

#ifdef GLV_WAYLAND_INPUT
extern GLVContext _glv_parent_context;
// input devices
struct wl_seat *seat;
struct wl_pointer *pointer;
struct wl_keyboard *keyboard;
#ifdef GLV_WAYLAND_TOUCH
static struct wl_touch *touch = NULL;
struct touch_event_data
{
	int event_type;	/* 0: map surface	*/
					/* 1: hmi surface	*/
	int x;
	int y;
};
#endif /* GLV_WAYLAND_TOUCH */
#endif /* GLV_WAYLAND_INPUT */

//static void draw(void *data, struct wl_callback *callback, uint32_t time);

#ifdef GLV_WAYLAND_INPUT
static void mouse_event(GLVMOUSEEVENT_t *glv_mouse_enent);

GLVINPUTFUNC_t	glv_input_func = {};

// ------------------------------------------------------------------------------------
//windowmanager
LibWindowmanager	*g_wm;
LibHomeScreen	*g_hs;
uint32_t g_id_ivisurf = 0;
bool gIsDraw;
long g_port = 0;
std::string g_token;
const char *g_app_name = "Navigation";

extern "C" {
int canUpdate(void)
{
	return 1;
//	if (gIsDraw == true) return 1;
//	return 0;
}

void setPort(long port)
{
	g_port = port;
	fprintf(stderr,"NAVI port = %d\n", g_port);
}

void setToken(char *tkn)
{
	g_token = std::string(tkn);
	fprintf(stderr,"NAVI token = %s\n", g_token.c_str());
}
}
int init_wm(LibWindowmanager *wm)
{
	int result = -1;

	if (g_wm->init(g_port, g_token.c_str()) != 0) {
        	return -1;
	}

	json_object *obj = json_object_new_object();
	json_object_object_add(obj, g_wm->kKeyDrawingName, json_object_new_string(g_app_name));
	
	result = g_wm->requestSurface(obj);
	if (result < 0) {
		fprintf(stderr,"wm request surface failed \n");
		return -1;
	}

	g_id_ivisurf = result;

	g_wm->set_event_handler(LibWindowmanager::Event_Active, [wm](json_object *object) {
		const char *label = json_object_get_string(
			json_object_object_get(object, g_wm->kKeyDrawingName));
		fprintf(stderr,"Surface %s got activated! \n", label);
	});

	g_wm->set_event_handler(LibWindowmanager::Event_Inactive, [wm](json_object *object) {
		const char *label = json_object_get_string(
			json_object_object_get(object, g_wm->kKeyDrawingName));
		fprintf(stderr,"Surface %s got inactivated! \n", label);
	});

	g_wm->set_event_handler(LibWindowmanager::Event_Visible, [wm](json_object *object) {
		const char *label = json_object_get_string(
			json_object_object_get(object, g_wm->kKeyDrawingName));
		fprintf(stderr,"Surface %s got visibled! \n", label);
	});

	g_wm->set_event_handler(LibWindowmanager::Event_Invisible, [wm](json_object *object) {
		const char *label = json_object_get_string(
			json_object_object_get(object, g_wm->kKeyDrawingName));
		fprintf(stderr,"Surface %s got invisibled! \n", label);
		gIsDraw = false;
	});

	g_wm->set_event_handler(LibWindowmanager::Event_SyncDraw, [wm](json_object *object) {
		const char *label = json_object_get_string(
			json_object_object_get(object, g_wm->kKeyDrawingName));
		const char *area = json_object_get_string(
			json_object_object_get(object, g_wm->kKeyDrawingArea));

		fprintf(stderr,"Surface %s got syncDraw! Area: %s. \n", label, area);
		if ((g_wm->kStrLayoutNormal + "." + g_wm->kStrAreaFull) == std::string(area)) {
			fprintf(stderr,"Layout:%s x:%d y:%d w:%d h:%d \n", area, 0, 0, 1080, 1488);
			//wl_egl_window_resize(gWindow->native, 1080, 1488, 0, 0);
			//gWindow->geometry.width = 1080;
			//gWindow->geometry.height = 1488;
		}
		else if ((g_wm->kStrLayoutSplit + "." + g_wm->kStrAreaMain)	== std::string(area) ||
				 (g_wm->kStrLayoutSplit + "." + g_wm->kStrAreaSub) == std::string(area)) {
			fprintf(stderr,"Layout:%s x:%d y:%d w:%d h:%d \n", area, 0, 0, 1080, 744);
			//wl_egl_window_resize(gWindow->native, 1080, 744, 0, 0);
			//gWindow->geometry.width = 1080;
			//gWindow->geometry.height = 744;
		}

		//if (!gWindow->fullscreen)
		//	gWindow->window_size = gWindow->geometry;
		gIsDraw = true;
		json_object *obj = json_object_new_object();
		json_object_object_add(obj, g_wm->kKeyDrawingName, json_object_new_string(g_app_name));

        g_wm->endDraw(obj);
    });

	g_wm->set_event_handler(LibWindowmanager::Event_FlushDraw, [wm](json_object *object) {
		const char *label = json_object_get_string(
			json_object_object_get(object, g_wm->kKeyDrawingName));
		fprintf(stderr,"Surface %s got flushdraw! \n", label);
	});

/*
	do
	{
        	surfaceIdStr = getenv("QT_IVI_SURFACE_ID");
	} while (surfaceIdStr == NULL);

	g_id_ivisurf = atoi(surfaceIdStr);
*/
	fprintf(stderr,"IVI_SURFACE_ID: %d \n", g_id_ivisurf);

	return 0;
}

int
init_hs(LibHomeScreen* hs){
	if(g_hs->init(g_port, g_token)!=0)
	{
		fprintf(stderr,"homescreen init failed. \n");
		return -1;
	}

	g_hs->set_event_handler(LibHomeScreen::Event_TapShortcut, [](json_object *object){
		const char *application_name = json_object_get_string(
			json_object_object_get(object, "application_name"));
		fprintf(stderr,"Event_TapShortcut application_name = %s \n", application_name);
		if(strcmp(application_name, g_app_name) == 0)
		{
			fprintf(stderr,"try to activesurface %s \n", g_app_name);
			json_object *obj = json_object_new_object();
			json_object_object_add(obj, g_wm->kKeyDrawingName, json_object_new_string(g_app_name));
			json_object_object_add(obj, g_wm->kKeyDrawingArea, json_object_new_string("normal.full"));
			gIsDraw = false;
			g_wm->activateSurface(obj);
		}
	});

	g_hs->set_event_handler(LibHomeScreen::Event_OnScreenMessage, [](json_object *object){
		const char *display_message = json_object_get_string(
			json_object_object_get(object, "display_message"));
        fprintf(stderr,"Event_OnScreenMessage display_message = %s \n", display_message);
	});

	return 0;
}


// ------------------------------------------------------------------------------------
// keyboard

static void
keyboard_handle_keymap(void *data, struct wl_keyboard *keyboard,
                       uint32_t format, int fd, uint32_t size)
{
}

static void
keyboard_handle_enter(void *data, struct wl_keyboard *keyboard,
                      uint32_t serial, struct wl_surface *surface,
                      struct wl_array *keys)
{
    //fprintf(stderr, "Keyboard gained focus\n");
}

static void
keyboard_handle_leave(void *data, struct wl_keyboard *keyboard,
                      uint32_t serial, struct wl_surface *surface)
{
    //fprintf(stderr, "Keyboard lost focus\n");
}

static void
keyboard_handle_key(void *data, struct wl_keyboard *keyboard,
                    uint32_t serial, uint32_t time, uint32_t key,
                    uint32_t state)
{
    if(state == WL_KEYBOARD_KEY_STATE_PRESSED){
    	if(glv_input_func.keyboard_key != 0) (*glv_input_func.keyboard_key)(key,GLV_KEYBOARD_KEY_STATE_PRESSED);
    }else if(state == WL_KEYBOARD_KEY_STATE_RELEASED){
    	if(glv_input_func.keyboard_key != 0) (*glv_input_func.keyboard_key)(key,GLV_KEYBOARD_KEY_STATE_RELEASED);
    }
}

static void
keyboard_handle_modifiers(void *data, struct wl_keyboard *keyboard,
                          uint32_t serial, uint32_t mods_depressed,
                          uint32_t mods_latched, uint32_t mods_locked,
                          uint32_t group)
{
	//fprintf(stderr, "Modifiers depressed %d, latched %d, locked %d, group %d\n",
	//    mods_depressed, mods_latched, mods_locked, group);
}

static const struct wl_keyboard_listener keyboard_listener = {
    keyboard_handle_keymap,
    keyboard_handle_enter,
    keyboard_handle_leave,
    keyboard_handle_key,
    keyboard_handle_modifiers,
};
// ------------------------------------------------------------------------------------
// pointer
static uint32_t pointer_left=WL_POINTER_BUTTON_STATE_RELEASED;
static int pointer_sx=0;
static int pointer_sy=0;

int sample_hmi_button_down(int pointer_sx,int pointer_sy);
int sample_hmi_button_up(int pointer_sx,int pointer_sy);

static void
pointer_handle_enter(void *data, struct wl_pointer *pointer,
                     uint32_t serial, struct wl_surface *surface,
                     wl_fixed_t sx, wl_fixed_t sy)
{
    //fprintf(stderr, "Pointer entered surface %p at %d %d\n", surface, sx, sy);
}

static void
pointer_handle_leave(void *data, struct wl_pointer *pointer,
                     uint32_t serial, struct wl_surface *surface)
{
    //fprintf(stderr, "Pointer left surface %p\n", surface);
}

static void
pointer_handle_motion(void *data, struct wl_pointer *pointer,
                      uint32_t time, wl_fixed_t sx, wl_fixed_t sy)
{
	GLVMOUSEEVENT_t	glv_mouse_enent;
    //printf("Pointer moved at %d %d\n", sx, sy);
    pointer_sx = wl_fixed_to_int(sx);
    pointer_sy = wl_fixed_to_int(sy);
    if(pointer_left == WL_POINTER_BUTTON_STATE_PRESSED){
   	 glv_mouse_enent.type = GLV_MOUSE_EVENT_MOTION;
   	 glv_mouse_enent.time = time;
   	 glv_mouse_enent.x    = (int)pointer_sx;
   	 glv_mouse_enent.y    = (int)pointer_sy;
   	 mouse_event(&glv_mouse_enent);
    }
}

static void
pointer_handle_button(void *data, struct wl_pointer *wl_pointer,
                      uint32_t serial, uint32_t time, uint32_t button,
                      uint32_t state)
{
	int rc;
	GLVMOUSEEVENT_t	glv_mouse_enent;
    //printf("Pointer button\n");
    if(button == BTN_LEFT){
    	if(state == WL_POINTER_BUTTON_STATE_PRESSED){
    		rc = 0;
    		if(glv_input_func.touch_down != 0) rc = (*glv_input_func.touch_down)(pointer_sx,pointer_sy);
    		if(rc == 1){
    		}else{
				pointer_left = WL_POINTER_BUTTON_STATE_PRESSED;
				glv_mouse_enent.type = GLV_MOUSE_EVENT_PRESS;
				glv_mouse_enent.time = time;
				glv_mouse_enent.x    = (int)pointer_sx;
				glv_mouse_enent.y    = (int)pointer_sy;
				mouse_event(&glv_mouse_enent);
    		}
    	}else{
    		rc = 0;
    		if(glv_input_func.touch_up != 0) rc = (*glv_input_func.touch_up)(pointer_sx,pointer_sy);
    		if(rc == 1){
    		}else{
				pointer_left=WL_POINTER_BUTTON_STATE_RELEASED;
				glv_mouse_enent.type = GLV_MOUSE_EVENT_RELEASE;
				glv_mouse_enent.time = time;
				glv_mouse_enent.x    = (int)pointer_sx;
				glv_mouse_enent.y    = (int)pointer_sy;
				mouse_event(&glv_mouse_enent);
    		}
    	}
    }
#if 1
    if (button == BTN_RIGHT && state == WL_POINTER_BUTTON_STATE_PRESSED){
    	wl_shell_surface_move(((GLVCONTEXT_t *)_glv_parent_context)->glv_win->wl_window.shell_surface,seat, serial);
    }
#endif
}

static void
pointer_handle_axis(void *data, struct wl_pointer *wl_pointer,
                    uint32_t time, uint32_t axis, wl_fixed_t value)
{
	//printf("Pointer handle axis\n");
}

static const struct wl_pointer_listener pointer_listener = {
    pointer_handle_enter,
    pointer_handle_leave,
    pointer_handle_motion,
    pointer_handle_button,
    pointer_handle_axis,
};

#ifdef GLV_WAYLAND_TOUCH
static void
touch_handle_down(void *data, struct wl_touch *touch, uint32_t serial,
				  uint32_t time, struct wl_surface *surface, int32_t id,
				  wl_fixed_t x_w, wl_fixed_t y_w)
{
	int rc;
	/* This does not support multi-touch, since implementation is for testing */
	if (id > 0) {
		//printf("Receive touch down event with touch id(%d), "
		//	   "but this is not handled\n", id);
		return;
	}
	struct touch_event_data *touch_event = (struct touch_event_data *)data;
	touch_event->x = wl_fixed_to_int(x_w);
	touch_event->y = wl_fixed_to_int(y_w);

    pointer_sx = touch_event->x;
    pointer_sy = touch_event->y;

    rc = 0;
    if(glv_input_func.touch_down != 0) rc = (*glv_input_func.touch_down)(pointer_sx,pointer_sy);
    if(rc == 1){
		touch_event->event_type = 1; /* button */
	}else{
		touch_event->event_type = 0; /* main */

		GLVMOUSEEVENT_t glv_touch_event;

		glv_touch_event.type = GLV_MOUSE_EVENT_PRESS;
		glv_touch_event.time = time;
		glv_touch_event.x    = touch_event->x;
		glv_touch_event.y    = touch_event->y;

		mouse_event(&glv_touch_event);
	}
}

static void
touch_handle_up(void *data, struct wl_touch *touch, uint32_t serial,
				uint32_t time, int32_t id)
{
	if (id > 0) {
		return;
	}
	struct touch_event_data *touch_event = (struct touch_event_data *)data;

	/* map surface event */
	if (touch_event->event_type == 1){
		if(glv_input_func.touch_up != 0) (*glv_input_func.touch_up)(pointer_sx,pointer_sy);
	}else if (touch_event->event_type == 0){
    		GLVMOUSEEVENT_t glv_touch_event;
			glv_touch_event.type = GLV_MOUSE_EVENT_RELEASE;
			glv_touch_event.time = time;
			glv_touch_event.x    = touch_event->x;
			glv_touch_event.y    = touch_event->y;

			mouse_event(&glv_touch_event);
	}

	/* reset */
	touch_event->event_type = 0;
}

static void
touch_handle_motion(void *data, struct wl_touch *touch, uint32_t time,
					int32_t id, wl_fixed_t x_w, wl_fixed_t y_w)
{
	if (id > 0) {
		return;
	}
	struct touch_event_data *touch_event = (struct touch_event_data *)data;
	touch_event->x = wl_fixed_to_int(x_w);
	touch_event->y = wl_fixed_to_int(y_w);
    pointer_sx = touch_event->x;
    pointer_sy = touch_event->y;

	if (touch_event->event_type != 0) {
		return; /* hmi surface event */
	}

	GLVMOUSEEVENT_t glv_touch_event;

	glv_touch_event.type = GLV_MOUSE_EVENT_MOTION;
	glv_touch_event.time = time;
	glv_touch_event.x    = touch_event->x;
	glv_touch_event.y    = touch_event->y;

	mouse_event(&glv_touch_event);
}

static void
touch_handle_frame(void *data, struct wl_touch *touch)
{
	/* no op */
}

static void
touch_handle_cancel(void *data, struct wl_touch *touch)
{
	/* no op */
}

static const struct wl_touch_listener touch_listener = {
	touch_handle_down,
	touch_handle_up,
	touch_handle_motion,
	touch_handle_frame,
	touch_handle_cancel
};
#endif /* GLV_WAYLAND_TOUCH */

//static void
//seat_handle_capabilities(void *data, struct wl_seat *seat,
//                         enum wl_seat_capability caps)
static void
seat_handle_capabilities(void *data, struct wl_seat *seat,
                         uint32_t caps)
{
    if (caps & WL_SEAT_CAPABILITY_POINTER) {
		//printf("Display has a pointer\n");
    }

    if (caps & WL_SEAT_CAPABILITY_KEYBOARD) {
		//printf("Display has a keyboard\n");
    }

    if (caps & WL_SEAT_CAPABILITY_TOUCH) {
		//printf("Display has a touch screen\n");
    }

    if ((caps & WL_SEAT_CAPABILITY_POINTER) && !pointer) {
	pointer = wl_seat_get_pointer(seat);
	wl_pointer_add_listener(pointer, &pointer_listener, NULL);
    } else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && pointer) {
	wl_pointer_destroy(pointer);
	pointer = NULL;
    }

    if (caps & WL_SEAT_CAPABILITY_KEYBOARD) {
		keyboard = wl_seat_get_keyboard(seat);
		if (keyboard != NULL)
		{
			wl_keyboard_add_listener(keyboard, &keyboard_listener, NULL);
		}
    } else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD)) {
		if (keyboard != NULL)
		{
			wl_keyboard_destroy(keyboard);
			keyboard = NULL;
		}
    }

#ifdef GLV_WAYLAND_TOUCH
	if ((caps & WL_SEAT_CAPABILITY_TOUCH) && !touch) {
		touch = wl_seat_get_touch(seat);
		if (touch != NULL)
		{
			wl_touch_add_listener(touch, &touch_listener, (struct touch_event_data *)data);
		}
	} else if (!(caps & WL_SEAT_CAPABILITY_TOUCH) && touch) {
		if (touch != NULL)
		{
			wl_touch_destroy(touch);
			touch = NULL;
		}
	}
#endif /* GLV_WAYLAND_TOUCH */
}

static const struct wl_seat_listener seat_listener = {
    seat_handle_capabilities,
	NULL	//
};

static void
handle_ping(void *data, struct wl_shell_surface *shell_surface,
	    uint32_t serial)
{
	wl_shell_surface_pong(shell_surface, serial);
	//fprintf(stderr, "Pinged and ponged\n");
}

static void
handle_configure(void *data, struct wl_shell_surface *shell_surface,
		 uint32_t edges, int32_t width, int32_t height)
{
}

static void
handle_popup_done(void *data, struct wl_shell_surface *shell_surface)
{
}

static const struct wl_shell_surface_listener shell_surface_listener = {
	handle_ping,
	handle_configure,
	handle_popup_done
};
// ------------------------------------------------------------------------------------
#endif /* GLV_WAYLAND_INPUT */

// ----------------------------------------------------------------------------
extern "C" void _glvInitNativeDisplay(GLVDISPLAY_t *glv_dpy)
{
	   // non
}

static void
registry_handle_global(void *data, struct wl_registry *registry, uint32_t id,
                       const char *interface, uint32_t version)
{
   WLDISPLAY_t *d = (WLDISPLAY_t *)data;

   if (strcmp(interface, "wl_compositor") == 0) {
      d->compositor = (struct wl_compositor *)wl_registry_bind(registry, id, (const struct wl_interface *)&wl_compositor_interface, 1);
   } else if (strcmp(interface, "wl_shell") == 0) {
      d->shell = (struct wl_shell *)wl_registry_bind(registry, id, (const struct wl_interface *)&wl_shell_interface, 1);
   }
#ifdef GLV_WAYLAND_INPUT
   else if (strcmp(interface, "wl_seat") == 0) {
#ifdef GLV_WAYLAND_TOUCH
		struct touch_event_data *touch_event = (touch_event_data *)calloc(1, sizeof *touch_event);
		seat = (struct wl_seat *)wl_registry_bind(registry, id, (const struct wl_interface *)&wl_seat_interface, 1);
		wl_seat_add_listener(seat, &seat_listener, touch_event);
#else
		seat = (struct wl_seat *)wl_registry_bind(registry, id, (const struct wl_interface *)&wl_seat_interface, 1);
		wl_seat_add_listener(seat, &seat_listener, NULL);
#endif /* !GLV_WAYLAND_TOUCH */
   }
#endif /* GLV_WAYLAND_INPUT */
   else if (strcmp(interface, "wl_subcompositor") == 0) {
   		d->subcompositor = (struct wl_subcompositor *)wl_registry_bind(registry, id, (const struct wl_interface *)&wl_subcompositor_interface, 1);
	}
	else if (strcmp(interface, "ivi_application") == 0)
	{
		d->ivi_application =
			(struct ivi_application *)wl_registry_bind(registry, id,
					 (const struct wl_interface *)&ivi_application_interface, 1);
	}
}

static void
registry_handle_global_remove(void *data, struct wl_registry *registry,
                              uint32_t name)
{
}

static const struct wl_registry_listener registry_listener = {
   registry_handle_global,
   registry_handle_global_remove
};

static void
sync_callback(void *data, struct wl_callback *callback, uint32_t serial)
{
   int *done = (int*)data;

   *done = 1;
   wl_callback_destroy(callback);
}

static const struct wl_callback_listener sync_listener = {
   sync_callback
};

// -----------------------------------------------------------------------
static void
surface_enter(void *data,
	      struct wl_surface *wl_surface, struct wl_output *wl_output)
{
}
static void
surface_leave(void *data,
	      struct wl_surface *wl_surface, struct wl_output *output)
{
}

static const struct wl_surface_listener surface_listener = {
	surface_enter,
	surface_leave
};
// -----------------------------------------------------------------------

static int wayland_roundtrip(struct wl_display *display)
{
   struct wl_callback *callback;
   int done = 0, ret = 0;

   callback = wl_display_sync(display);
   wl_callback_add_listener(callback, &sync_listener, &done);
   while (ret != -1 && !done)
      ret = wl_display_dispatch(display);

   if (!done)
      wl_callback_destroy(callback);

   return ret;
}

extern "C" void _glvOpenNativeDisplay(GLVDISPLAY_t *glv_dpy)
{
	   struct wl_registry *registry;

	   glv_dpy->native_dpy = (EGLNativeDisplayType)wl_display_connect(NULL);
	   glv_dpy->wl_dpy.display = (struct wl_display*)glv_dpy->native_dpy;

	   if (!glv_dpy->native_dpy){
	      fprintf(stderr,"failed to initialize native display\n");
	   }
	   glv_dpy->wl_dpy.compositor = 0;
	   glv_dpy->wl_dpy.shell = 0;

	   registry = wl_display_get_registry((struct wl_display*)glv_dpy->native_dpy);
	   wl_registry_add_listener(registry, &registry_listener, &glv_dpy->wl_dpy);
	   wayland_roundtrip((struct wl_display*)glv_dpy->native_dpy);
	   wl_registry_destroy(registry);
}

extern "C" void _glvCloseNativeDisplay(GLVDISPLAY_t *glv_dpy)
{
	   wl_display_flush((struct wl_display*)glv_dpy->native_dpy);
	   wl_display_disconnect((struct wl_display*)glv_dpy->native_dpy);
}

extern "C" GLVWindow _glvCreateNativeWindow(GLVDISPLAY_t *glv_dpy,
              int x, int y, int width, int height,
			  GLVWindow glv_window_parent)
{
	struct wl_surface		*parent;
	struct wl_surface		*surface;
	struct wl_subsurface	*subsurface  = NULL;
	struct wl_shell_surface	*shell_surface = NULL;
	struct ivi_surface		*ivi_surface  = NULL;
	
	struct wl_egl_window	*native;
	struct wl_region		*region;
	WLDISPLAY_t	*wl_dpy;
	GLVWINDOW_t	*glv_window;

	if(glv_window_parent != NULL){
		parent = ((GLVWINDOW_t *)glv_window_parent)->wl_window.surface;
	}else{
		parent = 0;
	}
	subsurface    = 0;
	shell_surface = 0;

	wl_dpy = &glv_dpy->wl_dpy;

	surface = wl_compositor_create_surface(wl_dpy->compositor);

	if(parent == 0){
		// -----------------------------------------------------------------------------------------------------------------
		/* map */
		region = wl_compositor_create_region(wl_dpy->compositor);
		wl_region_add(region, 0, 0, width, height);
		wl_surface_set_opaque_region(surface, region);
		wl_region_destroy(region);

		native = wl_egl_window_create(surface, width, height);

#ifdef IVISHELL
		{
			g_wm = new LibWindowmanager();
			g_hs = new LibHomeScreen();			

			if(init_wm(g_wm)!=0){
				fprintf(stderr, "Init LibWindowmanager\n");
				abort();
			}

			if(init_hs(g_hs)!=0){
				fprintf(stderr, "Init LibHomeScreen\n");
				abort();
			}

			ivi_surface = ivi_application_surface_create(wl_dpy->ivi_application, g_id_ivisurf, surface);
		
			if (ivi_surface == NULL) {
				fprintf(stderr, "Failed to create ivi_client_surface\n");
				abort();
			}
		}
#else
		shell_surface = wl_shell_get_shell_surface(wl_dpy->shell,surface);
		wl_shell_surface_set_toplevel(shell_surface);

#ifdef GLV_WAYLAND_INPUT
		wl_shell_surface_add_listener(shell_surface,&shell_surface_listener, NULL);
#endif /* GLV_WAYLAND_INPUT */
#endif	//#ifdef IVISHELL

		// -----------------------------------------------------------------------------------------------------------------
	}else{
		// -----------------------------------------------------------------------------------------------------------------
		/* hmi */
		wl_surface_add_listener(surface,&surface_listener, NULL);

		subsurface = wl_subcompositor_get_subsurface(wl_dpy->subcompositor,surface,parent);
		wl_subsurface_set_desync(subsurface);

		native = wl_egl_window_create(surface, width, height);

		wl_subsurface_set_position(subsurface, x, y);
		// -----------------------------------------------------------------------------------------------------------------
	}
	//*winRet = native;

	glv_window = (GLVWINDOW_t *)malloc(sizeof(GLVWINDOW_t));
	if(!glv_window){
		return(0);
	}

	glv_window->glv_dpy                 = glv_dpy;
	glv_window->egl_window              = native;
	glv_window->wl_window.parent        = parent;
	glv_window->wl_window.surface       = surface;
	glv_window->wl_window.subsurface    = subsurface;
	glv_window->wl_window.shell_surface = shell_surface;
	glv_window->wl_window.ivi_surface   = ivi_surface;
	glv_window->wl_window.callback      = 0;
	glv_window->wl_window.x             = x;
	glv_window->wl_window.y             = y;
	glv_window->wl_window.width         = width;
	glv_window->wl_window.height        = height;

   return((GLVWindow)glv_window);
}

extern "C" void _glvDestroyNativeWindow(GLVWindow glv_win)
{
	GLVWINDOW_t *glv_window=(GLVWINDOW_t *)glv_win;

	wl_egl_window_destroy((struct wl_egl_window *)glv_window->egl_window);

	if (glv_window->wl_window.shell_surface)
		wl_shell_surface_destroy(glv_window->wl_window.shell_surface);

	wl_surface_destroy(glv_window->wl_window.surface);

	if (glv_window->wl_window.subsurface)
		wl_subsurface_destroy(glv_window->wl_window.subsurface);

	if (glv_window->wl_window.callback)
		wl_callback_destroy(glv_window->wl_window.callback);

	free(glv_win);
}

#if 0
static const struct wl_callback_listener frame_listener = {
   draw
};
#endif

#if 0
static void
draw(void *data, struct wl_callback *callback, uint32_t time)
{
	GLVCONTEXT_t *glv_context = (GLVCONTEXT_t*)data;

   glvOnReDraw((GLVContext)glv_context);

   if (callback){
      wl_callback_destroy(callback);
   }
#if 0
   glv_context->glv_win->wl_window.callback = wl_surface_frame(glv_context->glv_win->wl_window.surface);
   wl_callback_add_listener(glv_context->glv_win->wl_window.callback, &frame_listener, glv_context);
#endif
}
#endif

void glvEventLoop(GLVDisplay glv_dpy)
{
	struct pollfd pollfd;
	struct wl_display		*display;
	int ret;

	display = ((GLVDISPLAY_t*)glv_dpy)->wl_dpy.display;

	pollfd.fd = wl_display_get_fd(display);
	pollfd.events = POLLIN;
	pollfd.revents = 0;

	while(1){
	  int redraw = 0;

      wl_display_dispatch_pending(display);

      ret = wl_display_flush(display);
      if (ret < 0 && errno == EAGAIN)
         pollfd.events |= POLLOUT;
      else if (ret < 0)
         break;

      if(poll(&pollfd, 1, (redraw ? 0 : -1)) == -1)
         break;

      if(pollfd.revents & (POLLERR | POLLHUP))
         break;

      if(pollfd.revents & POLLIN) {
         ret = wl_display_dispatch(display);
         if (ret == -1)
            break;
      }

      if(pollfd.revents & POLLOUT) {
         ret = wl_display_flush(display);
         if (ret == 0)
            pollfd.events &= ~POLLOUT;
         else if (ret == -1 && errno != EAGAIN)
            break;
      }

      if (redraw) {
         //glvOnReDraw(glv_c);
      }
   }
}

void glvActivateSurface()
{
#ifdef IVISHELL
	json_object *obj = json_object_new_object();
	json_object_object_add(obj, g_wm->kKeyDrawingName, json_object_new_string(g_app_name));
	json_object_object_add(obj, g_wm->kKeyDrawingArea, json_object_new_string("normal.full"));
	g_wm->activateSurface(obj);
#endif
}

// ----------------------------------------------------------------------------
static void mouse_event(GLVMOUSEEVENT_t *glv_mouse_enent)
{
#define MOUSE_NON		(0)
#define MOUSE_PRESS		(1)
#define MOUSE_RELEASE	(2)
#define MOUSE_MOVE		(3)
#define MOUSE_FLICK		(4)
#define MOUSE_SWIPE		(5)

#define MOUSE_TOUCH_SIZE	(20)	/* [dot] */
#define MOUSE_MOVE_SIZE		(200)	/* [dot] */
#define MOUSE_FLICK_SIZE	(100)	/* [dot] */
#define MOUSE_MOVE_TIME		(200)	/* [msec] */
#define MOUSE_FLICK_TIME	(300)	/* [msec] */

	glvTime current_time;
	static glvTime pressTime,releaseTime,moveTime;
	glvTime elapsedTime;
	static int s_x,s_y,e_x,e_y,m_x,m_y,old_m_x,old_m_y;
	static int stat = MOUSE_NON;
	int x,y;
	int distance_x,distance_y;
	int velocity_x,velocity_y;
	int   flag;
	flag = 0;

	if(_glv_parent_context == 0){
		return;
	}

	GLVContext glv_c = _glv_parent_context;

	switch(glv_mouse_enent->type)
	 {
	   case    GLV_MOUSE_EVENT_PRESS:
						   current_time = glv_mouse_enent->time;
						   pressTime = current_time;
						   moveTime = pressTime;
						   s_x = glv_mouse_enent->x;
						   s_y = glv_mouse_enent->y;
						   x = s_x;
						   y = s_y;
						   old_m_x = x;
						   old_m_y = y;
						   stat = MOUSE_PRESS;
						   distance_x = 0;
						   distance_y = 0;
						   velocity_x = 0;
						   velocity_y = 0;
						   flag = 1;
						   break;
	   case  GLV_MOUSE_EVENT_RELEASE:
						   current_time = glv_mouse_enent->time;
						   releaseTime = current_time;
						   e_x = glv_mouse_enent->x;
						   e_y = glv_mouse_enent->y;

						   x = e_x;
						   y = e_y;
						   distance_x = x - old_m_x;
						   distance_y = y - old_m_y;
						   elapsedTime = releaseTime - pressTime;
						   if(elapsedTime <= MOUSE_FLICK_TIME){
							   if((abs(e_x - s_x) > MOUSE_FLICK_SIZE) || (abs(e_y - s_y) > MOUSE_FLICK_SIZE)){
									/* 短い時間で大きく移動した */
								   velocity_x = distance_x * 1000 / (int)elapsedTime;   /* pixel / sec */
								   velocity_y = distance_y * 1000 / (int)elapsedTime;   /* pixel / sec */
									stat = MOUSE_FLICK;
									flag = 1;
								}
						   }
						  if(flag == 1) break;
						   if(stat == MOUSE_MOVE){
								/* ゆっくり移動した */
							    stat = MOUSE_SWIPE;
								flag = 1;
						   }else{
							    stat = MOUSE_RELEASE;
								flag = 1;
						   }
						   break;
	   case   GLV_MOUSE_EVENT_MOTION:
		   // printf("M\n");
						   current_time = glv_mouse_enent->time;
						   m_x = glv_mouse_enent->x;
						   m_y = glv_mouse_enent->y;

						   x = m_x;
						   y = m_y;
						   if(stat == MOUSE_PRESS){
							   elapsedTime = current_time - pressTime;
							   if(elapsedTime > MOUSE_FLICK_TIME){
								   if((abs(m_x - s_x) > MOUSE_TOUCH_SIZE) || (abs(m_y - s_y) > MOUSE_TOUCH_SIZE)){
									   stat = MOUSE_MOVE;
								   }
							   }
						   }
						   if(stat == MOUSE_MOVE){
							   elapsedTime = current_time - moveTime;
							   if(elapsedTime > MOUSE_MOVE_TIME){
								   /* 時間が経過した */
								   distance_x = x - old_m_x;
								   distance_y = y - old_m_y;
								   old_m_x = x;
								   old_m_y = y;
								   flag = 1;
							   }else if((abs(m_x - old_m_x) > MOUSE_MOVE_SIZE) || (abs(m_y - old_m_y) > MOUSE_MOVE_SIZE)){
								   /* 直前の位置から移動した */
								   distance_x = x - old_m_x;
								   distance_y = y - old_m_y;
								   old_m_x = x;
								   old_m_y = y;
								   flag = 1;
							   }
							   moveTime = current_time;
						   }
						   break;
	 }
	if(flag == 0) return;

	switch(stat)
	 {
	   case    MOUSE_NON:
		   break;
	   case    MOUSE_PRESS:
		   //printf("[press(%d,%d)]\n",x,y);
			glvOnGesture(glv_c,GLV_GESTURE_EVENT_DOWN,x,y,0,0,0,0);
		   break;
	   case    MOUSE_RELEASE:
			stat = MOUSE_NON;
		   //printf("[release(%d,%d) vector:%d %d]\n",x,y,distance_x,distance_y);
			glvOnGesture(glv_c,GLV_GESTURE_EVENT_SINGLE_UP,x,y,distance_x,distance_y,0,0);
		   break;
	   case    MOUSE_MOVE:
		   //printf("[move(%d,%d) vector:%d %d]\n",x,y,distance_x,distance_y);
			glvOnGesture(glv_c,GLV_GESTURE_EVENT_SCROLL,x,y,distance_x,distance_y,0,0);
		   break;
	   case    MOUSE_FLICK:
			stat = MOUSE_NON;
		   //printf("[flick(%d,%d) vector:%d %d velocity:%d %d time:%d]\n",x,y,distance_x,distance_y,velocity_x,velocity_y,elapsedTime);
			glvOnGesture(glv_c,GLV_GESTURE_EVENT_FLING,x,y,distance_x,distance_y,velocity_x,velocity_y);
		   break;
	   case    MOUSE_SWIPE:
			stat = MOUSE_NON;
		   //printf("[swipe(%d,%d) vector:%d %d]\n",x,y,distance_x,distance_y);
			glvOnGesture(glv_c,GLV_GESTURE_EVENT_SCROLL_STOP,x,y,distance_x,distance_y,0,0);
		   break;
	   default:
		   break;
	 }
}

