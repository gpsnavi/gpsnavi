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
 * navi.h
 *
 *  Created on: 2015/11/05
 *      Author:t.aikawa
 */

#ifndef NAVI_H
#define NAVI_H


#ifdef __cplusplus
extern "C" {
#endif

// navi.c
void naviGetResolution(int *w,int *h);
void naviGetRegion(int *r);
void set_map_draw_japan(int japan);
int sample_hmi_keyboard_handle_key(unsigned int key,unsigned int state);

extern int hmiMAP_MAX_SCALE;
extern int main_window_mapScale;

// route.c
int  sample_calc_demo_route(void);
void sample_clear_demo_route_icon(void);
void sample_init_demo_route_icon(void);
void sample_set_demo_icon_pin_flag(SMGEOCOORD	*geoCood);

// button.c
void sample_hmi_request_mapDraw(void);
void sample_hmi_set_fource_update_mode(void);
void sample_hmi_draw_compass(FLOAT rotate);
void sample_hmi_request_update(void);
void sample_hmi_set_pin_mode(int pin);
int  sample_hmi_get_pin_mode(void);
int  sample_hmi_button_down(int pointer_sx,int pointer_sy);
int  sample_hmi_button_up(int pointer_sx,int pointer_sy);

// guide.h
void sample_createGuideThread(void);
void sample_guide_request_start(void);
void sample_guide_request_end(void);

#ifdef __cplusplus
}
#endif

#endif // NAVI_H
