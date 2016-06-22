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
 * button.c
 *
 *  Created on: 2015/11/05
 *      Author:t.aikawa
 */

#include <stdlib.h>
#include <stdio.h>

#include "navicore.h"

#include "SMCoreMP/MP_Def.h"
#include "SMCoreMP/MP_GL.h"

#include "glview.h"
#include "navi.h"
#include "HMI_Icon.h"

extern GLVContext glv_hmi_context;
extern GLVContext glv_map_context;

static int hmi_move_with_car=-1;
static int old_hmi_set_pin=-1;
static int hmi_set_pin=0;
static int hmi_compass=0;
static int hmi_fource_update_flag = 0;

#define DEMO_BUTTON_OWNPOSI			(0)
#define DEMO_BUTTON_ROUTE			(1)
#define DEMO_BUTTON_COMPASS			(2)
#define DEMO_BUTTON_GUIDE_START		(3)
#define DEMO_BUTTON_GUIDE_END		(4)
#define DEMO_BUTTON_SCALE_UP		(5)
#define DEMO_BUTTON_SCALE_DOWN		(6)
#define DEMO_BUTTON_MAX				(7)

typedef struct _demo_button {
	int			type;
	int			x1,y1;
	int			x2,y2;
	int			w,h;
	int			offset;
	int			visible;
	int			on;
	float		scale;
} DEMO_BUTTON_t;

static DEMO_BUTTON_t	demo_button[DEMO_BUTTON_MAX];
static int current_push_buttonId=-1;
static int old_push_buttonId=-1;
static int current_push_buttonOn=0;
static int old_push_buttonOn=0;


void sample_hmi_set_fource_update_mode(void)
{
	hmi_fource_update_flag = 1;
}

static void sample_hmi_init_button(int WinWidth,int WinHeight)
{
	DEMO_BUTTON_t	*button;

	button = &demo_button[DEMO_BUTTON_OWNPOSI];
	button->type = DEMO_BUTTON_OWNPOSI;
	button->scale = 0.8f;
	button->offset = 10;
	button->h	= 128 * button->scale;
	button->w	= 128 * button->scale;
	button->x1	= WinWidth  - button->w - 20;
	button->y1	= WinHeight - button->h - 30 - 20;
	button->x2	= button->x1 + button->w;
	button->y2	= button->y1 + button->h;
	button->visible = 0;
	button->on = 0;

	button = &demo_button[DEMO_BUTTON_ROUTE];
	button->type = DEMO_BUTTON_ROUTE;
	button->scale = 0.8f;
	button->offset = 10;
	button->h	= 128 * button->scale;
	button->w	= 128 * button->scale;
	button->x1	= WinWidth  - button->w - 20;
	button->y1	= 30;
	button->x2	= button->x1 + button->w;
	button->y2	= button->y1 + button->h;
	button->visible = 0;
	button->on = 0;

	button = &demo_button[DEMO_BUTTON_COMPASS];
	button->type = DEMO_BUTTON_COMPASS;
	button->scale = 0.8f;
	button->offset = 0;
	button->h	= 128 * button->scale;
	button->w	= 128 * button->scale;
	button->x1	= 10;
	button->y1	= 10;
	button->x2	= button->x1 + button->w;
	button->y2	= button->y1 + button->h;
	button->visible = 1;
	button->on = 0;

	button = &demo_button[DEMO_BUTTON_GUIDE_START];
	button->type = DEMO_BUTTON_GUIDE_START;
	button->scale = 0.8f;
	button->offset = 10;
	button->h	= 128 * button->scale;
	button->w	= 128 * button->scale;
	button->x1	= WinWidth  - button->w - 20;
	button->y1	= 30;
	button->x2	= button->x1 + button->w;
	button->y2	= button->y1 + button->h;
	button->visible = 0;
	button->on = 0;

	button = &demo_button[DEMO_BUTTON_GUIDE_END];
	button->type = DEMO_BUTTON_GUIDE_END;
	button->scale = 0.8f;
	button->offset = 10;
	button->h	= 128 * button->scale;
	button->w	= 128 * button->scale;
	button->x1	= WinWidth  - button->w - 20;
	button->y1	= 30;
	button->x2	= button->x1 + button->w;
	button->y2	= button->y1 + button->h;
	button->visible = 0;
	button->on = 0;

	button = &demo_button[DEMO_BUTTON_SCALE_UP];
	button->type = DEMO_BUTTON_SCALE_UP;
	button->scale = 0.8f;
	button->offset = 10;
	button->h	= 128 * button->scale;
	button->w	= 128 * button->scale;
	button->x1	= 30;
	button->y1	= WinHeight - button->h - 30 - 30 - button->h - 20;
	button->x2	= button->x1 + button->w;
	button->y2	= button->y1 + button->h;
	button->visible = 1;
	button->on = 0;

	button = &demo_button[DEMO_BUTTON_SCALE_DOWN];
	button->type = DEMO_BUTTON_SCALE_DOWN;
	button->scale = 0.8f;
	button->offset = 10;
	button->h	= 128 * button->scale;
	button->w	= 128 * button->scale;
	button->x1	= 30;
	button->y1	= WinHeight - button->h - 30 - 20;
	button->x2	= button->x1 + button->w;
	button->y2	= button->y1 + button->h;
	button->visible = 1;
	button->on = 0;

}

int sample_hmi_check_button_area(int x,int y,int on)
{
	DEMO_BUTTON_t	*button;
	int i;
	current_push_buttonId = -1;
	current_push_buttonOn = 0;
	for(i=0;i<DEMO_BUTTON_MAX;i++){
		button = &demo_button[i];
		if(button->visible  != 1) continue;
		if( (button->x1 < x ) && (button->x2 > x) &&
			(button->y1 < y ) && (button->y2 > y) 	){
			printf("check button = %d\n",button->type);
			current_push_buttonId = button->type;
			current_push_buttonOn = on;
			return(button->type);
		}
	}
	return(-1);
}
void sample_hmi_set_button_visible(int type,int visible)
{
	demo_button[type].visible = visible;
}

void sample_hmi_set_pin_mode(int pin)
{
	hmi_set_pin = pin;
}
int sample_hmi_get_pin_mode(void)
{
	return hmi_set_pin;
}
void hmiMP_GL_Init(void)
{
#ifdef NAVI_OPENGLES1
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	glDisable(GL_LIGHTING);
	glShadeModel(GL_FLAT);
#endif

	glDisable(GL_DITHER);		// ディザを無効化
	glDisable(GL_DEPTH_TEST);

	// 頂点配列の使用を許可
	glEnableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);

    glClearColor(0.0, 0.0, 0.0, 0.0);
	// 画面クリア
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

static void hmiMP_GL_DrawSquares(FLOAT x, FLOAT y, FLOAT width, FLOAT height)
{
	static T_POINT squares[4];

	squares[0].x = x;
	squares[0].y = y;
	squares[1].x = x;
	squares[1].y = y + height;
	squares[2].x = x + width;
	squares[2].y = y;
	squares[3].x = x + width;
	squares[3].y = y + height;

	MP_GL_Draw(GL_TRIANGLE_STRIP, squares, 4);
}

void sample_hmi_draw_compass(FLOAT rotate)
{
#if 0
	typedef enum _E_SC_RESULT {
		e_SC_RESULT_SUCCESS = 0x00000000,
	} E_SC_RESULT;

	E_SC_RESULT MP_ICON_Draw(FLOAT x, FLOAT y, FLOAT angle, FLOAT scale, INT32 iconID);
#endif

	MP_GL_PushMatrix();
	MP_GL_BeginBlend();

	{
		DEMO_BUTTON_t	*button;
		float x,y,w,h,offset,scale;

		button = &demo_button[DEMO_BUTTON_COMPASS];
		scale	= button->scale;
		offset	= button->offset;
		w		= button->w;
		h		= button->h;
		x		= button->x1;
		y		= button->y1;
		//hmiMP_GL_DrawSquares(x,y,w - offset,h - offset);
		if(hmi_compass == 0){
			mapMP_ICON_Draw(x + (w/2) - offset,y + (h/2) - offset,rotate,scale,89); // 回転用コンパス

		}else{
			mapMP_ICON_Draw(x + (w/2) - offset,y + (h/2) - offset,0.0f,scale,90); // 回転用コンパス
		}
	}

	MP_GL_EndBlend();
	MP_GL_PopMatrix();
}

void sample_hmi_update(GLVContext glv_ctx)
{
	int updateFlag = 0;
	int new_car;

	/* ------------------------------------------ */
	new_car = NC_MP_GetMapMoveWithCar(NC_MP_MAP_MAIN);
	if(hmi_move_with_car != new_car){
		updateFlag = 1;
		hmi_move_with_car = new_car;
	}
	/* ------------------------------------------ */
	if(old_hmi_set_pin != hmi_set_pin){
		updateFlag = 1;
		old_hmi_set_pin = hmi_set_pin;
	}
	/* ------------------------------------------ */
	if(old_push_buttonId != current_push_buttonId){
		updateFlag = 1;
		old_push_buttonId = current_push_buttonId;
	}
	/* ------------------------------------------ */
	if(old_push_buttonOn != current_push_buttonOn){
		updateFlag = 1;
		old_push_buttonOn = current_push_buttonOn;
	}
	/* ------------------------------------------ */
	if(hmi_fource_update_flag == 1){	/* 強制更新 */
		hmi_fource_update_flag = 0;
		updateFlag = 1;
	}
	/* ------------------------------------------ */
	if(updateFlag == 0) return;

	printf("----------[HMI Update]--------------\n");
	sample_hmi_set_button_visible(DEMO_BUTTON_OWNPOSI,0);
	sample_hmi_set_button_visible(DEMO_BUTTON_ROUTE,0);
	sample_hmi_set_button_visible(DEMO_BUTTON_GUIDE_START,0);
	sample_hmi_set_button_visible(DEMO_BUTTON_GUIDE_END,0);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* ----------------------------------------------------------- */
    {
		MP_GL_PushMatrix();

		MP_GL_BeginBlend();

		glColor4f(0.0, 0.0, 0.0, 0.6);

		{
			DEMO_BUTTON_t	*button;
			float x,y,w,h,offset,scale;
			button = &demo_button[DEMO_BUTTON_SCALE_UP];
			scale	= button->scale;
			offset	= button->offset;
			w		= button->w;
			h		= button->h;
			x		= button->x1;
			y		= button->y1;
			hmiMP_GL_DrawSquares(x,y,w - offset,h - offset);
			if((current_push_buttonId == DEMO_BUTTON_SCALE_UP) && (old_push_buttonOn == 1)){
				hmiMP_ICON_Draw(x + (w/2) - offset,y + (h/2) - offset,0.0f,scale,26); //
			}else{
				hmiMP_ICON_Draw(x + (w/2) - offset,y + (h/2) - offset,0.0f,scale,25); //
			}
		}

		MP_GL_EndBlend();
		MP_GL_PopMatrix();
    }
    /* ----------------------------------------------------------- */
    {
		MP_GL_PushMatrix();

		MP_GL_BeginBlend();

		glColor4f(0.0, 0.0, 0.0, 0.6);

		{
			DEMO_BUTTON_t	*button;
			float x,y,w,h,offset,scale;
			button = &demo_button[DEMO_BUTTON_SCALE_DOWN];
			scale	= button->scale;
			offset	= button->offset;
			w		= button->w;
			h		= button->h;
			x		= button->x1;
			y		= button->y1;
			hmiMP_GL_DrawSquares(x,y,w - offset,h - offset);
			if((current_push_buttonId == DEMO_BUTTON_SCALE_DOWN) && (old_push_buttonOn == 1)){
				hmiMP_ICON_Draw(x + (w/2) - offset,y + (h/2) - offset,0.0f,scale,24); //
			}else{
				hmiMP_ICON_Draw(x + (w/2) - offset,y + (h/2) - offset,0.0f,scale,23); //
			}
		}

		MP_GL_EndBlend();
		MP_GL_PopMatrix();
    }
    /* ----------------------------------------------------------- */
    if(hmi_move_with_car == 0){
		MP_GL_PushMatrix();
		MP_GL_BeginBlend();

		glColor4f(0.0, 0.0, 0.0, 0.6);

		{
			DEMO_BUTTON_t	*button;
			float x,y,w,h,offset,scale;
			button = &demo_button[DEMO_BUTTON_OWNPOSI];
			scale	= button->scale;
			offset	= button->offset;
			w		= button->w;
			h		= button->h;
			x		= button->x1;
			y		= button->y1;
			hmiMP_GL_DrawSquares(x,y,w - offset,h - offset);
			if((current_push_buttonId == DEMO_BUTTON_OWNPOSI) && (old_push_buttonOn == 1)){
				hmiMP_ICON_Draw(x + (w/2) - offset,y + (h/2) - offset,0.0f,scale,30); //
			}else{
				hmiMP_ICON_Draw(x + (w/2) - offset,y + (h/2) - offset,0.0f,scale,29); //
			}
			sample_hmi_set_button_visible(DEMO_BUTTON_OWNPOSI,1);
		}

		MP_GL_EndBlend();
		MP_GL_PopMatrix();
    }
    /* ----------------------------------------------------------- */
    if(NC_DM_IsExistRoute() == 1){
    	/* 経路がある */
    	if(NC_Simulation_IsInSimu() == 1){
    		/* 終了ボタンを表示する */
    		MP_GL_PushMatrix();
    		MP_GL_BeginBlend();
    		glColor4f(0.0, 0.0, 0.0, 0.6);
    		{
    			DEMO_BUTTON_t	*button;
    			float x,y,w,h,offset,scale;
    			button = &demo_button[DEMO_BUTTON_GUIDE_END];
    			scale	= button->scale;
    			offset	= button->offset;
    			w		= button->w;
    			h		= button->h;
    			x		= button->x1;
    			y		= button->y1;
    			hmiMP_GL_DrawSquares(x,y,w - offset,h - offset);
    			if((current_push_buttonId == DEMO_BUTTON_GUIDE_END) && (old_push_buttonOn == 1)){
    				hmiMP_ICON_Draw(x + (w/2) - offset,y + (h/2) - offset,0.0f,scale,22); //
    			}else{
    				hmiMP_ICON_Draw(x + (w/2) - offset,y + (h/2) - offset,0.0f,scale,21); //
    			}
    			sample_hmi_set_button_visible(DEMO_BUTTON_GUIDE_END,1);
    		}

    		MP_GL_EndBlend();
    		MP_GL_PopMatrix();
    	}else{
    		/* 開始ボタンを表示する */
    		MP_GL_PushMatrix();
    		MP_GL_BeginBlend();
    		glColor4f(0.0, 0.0, 0.0, 0.6);
    		{
    			DEMO_BUTTON_t	*button;
    			float x,y,w,h,offset,scale;
    			button = &demo_button[DEMO_BUTTON_GUIDE_START];
    			scale	= button->scale;
    			offset	= button->offset;
    			w		= button->w;
    			h		= button->h;
    			x		= button->x1;
    			y		= button->y1;
    			hmiMP_GL_DrawSquares(x,y,w - offset,h - offset);
    			if((current_push_buttonId == DEMO_BUTTON_GUIDE_START) && (old_push_buttonOn == 1)){
    				hmiMP_ICON_Draw(x + (w/2) - offset,y + (h/2) - offset,0.0f,scale,27); //
    			}else{
    				hmiMP_ICON_Draw(x + (w/2) - offset,y + (h/2) - offset,0.0f,scale,28); //
    			}
    			sample_hmi_set_button_visible(DEMO_BUTTON_GUIDE_START,1);
    		}

    		MP_GL_EndBlend();
    		MP_GL_PopMatrix();
    	}
    }else
    if(hmi_set_pin == 1){
		MP_GL_PushMatrix();
		MP_GL_BeginBlend();

		glColor4f(0.0, 0.0, 0.0, 0.6);
		{
			DEMO_BUTTON_t	*button;
			float x,y,w,h,offset,scale;
			button = &demo_button[DEMO_BUTTON_ROUTE];
			scale	= button->scale;
			offset	= button->offset;
			w		= button->w;
			h		= button->h;
			x		= button->x1;
			y		= button->y1;
			hmiMP_GL_DrawSquares(x,y,w - offset,h - offset);
			if((current_push_buttonId == DEMO_BUTTON_ROUTE) && (old_push_buttonOn == 1)){
				hmiMP_ICON_Draw(x + (w/2) - offset,y + (h/2) - offset,0.0f,scale,28); //
			}else{
				hmiMP_ICON_Draw(x + (w/2) - offset,y + (h/2) - offset,0.0f,scale,27); //
			}
			sample_hmi_set_button_visible(DEMO_BUTTON_ROUTE,1);
		}

		MP_GL_EndBlend();
		MP_GL_PopMatrix();
    }
	glvSwapBuffers(glv_ctx);
}

void sample_hmi_init(GLVContext glv_ctx)
{
	int width,height;

	naviGetResolution(&width,&height);

	sample_hmi_init_button(width,height);

	// プロジェクション行列の設定
	MP_GL_MatrixProjection();

	// 座標体系設定
	MP_GL_Orthof(0, width, height, 0, -1, 1);

	MP_GL_MatrixModelView();

	sample_hmi_update(glv_ctx);
}

void sample_hmi_request_update(void)
{
	glvOnUpdate(glv_hmi_context);
}

void sample_hmi_request_mapDraw(void)
{
	glvOnReDraw(glv_map_context);
}

void sample_hmi_request_action(int buttonId)
{
	switch(buttonId){
	case DEMO_BUTTON_OWNPOSI:
		NC_MP_SetMapMoveWithCar(NC_MP_MAP_MAIN,1);
		glvOnReDraw(glv_map_context);
		break;
	case DEMO_BUTTON_ROUTE:
		sample_calc_demo_route();
		break;
	case DEMO_BUTTON_COMPASS:
		if(hmi_compass== 0){
			hmi_compass = 1;
		}else{
			hmi_compass = 0;
		}
		NC_MP_SetMapDispMode(NC_MP_MAP_MAIN,hmi_compass);
		if(hmi_compass == 1){
			NC_MP_SetMapRotate(NC_MP_MAP_MAIN,0);
		}
		glvOnReDraw(glv_map_context);
		break;
	case DEMO_BUTTON_GUIDE_START:
		sample_guide_request_start();
		break;
	case DEMO_BUTTON_GUIDE_END:
		sample_guide_request_end();
		break;
	case DEMO_BUTTON_SCALE_UP:
		   if(main_window_mapScale < hmiMAP_MAX_SCALE){
			   main_window_mapScale++;
				   if (main_window_mapScale == 1) {
					   main_window_mapScale++;
				   }
				   if(main_window_mapScale > hmiMAP_MAX_SCALE) main_window_mapScale = hmiMAP_MAX_SCALE;
				  NC_MP_SetMapScaleLevel(NC_MP_MAP_MAIN,main_window_mapScale);
					glvOnReDraw(glv_map_context);
		   }
		break;
	case DEMO_BUTTON_SCALE_DOWN:
  	   if(main_window_mapScale > 0){
  		   main_window_mapScale--;
				   if (main_window_mapScale == 1) {
					   main_window_mapScale--;
				   }
				   if(main_window_mapScale < 0) main_window_mapScale = 0;
				  NC_MP_SetMapScaleLevel(NC_MP_MAP_MAIN,main_window_mapScale);
				  glvOnReDraw(glv_map_context);
  	   }
		break;
	default:
		break;
	}
}

static int hmi_onmap_button=0;
static int hmi_onmap_buttonId=-1;

int sample_hmi_button_down(int pointer_sx,int pointer_sy)
{
	hmi_onmap_buttonId = sample_hmi_check_button_area(pointer_sx,pointer_sy,1);
	if(hmi_onmap_buttonId > -1){
		printf("sample_hmi_check_button_area (%d)\n",hmi_onmap_buttonId);
		hmi_onmap_button = 1;
		sample_hmi_request_update();
		return(1);
	}
	return(0);
}

int sample_hmi_button_up(int pointer_sx,int pointer_sy)
{
	if(hmi_onmap_button == 1){
		hmi_onmap_button = 0;
		hmi_onmap_buttonId = sample_hmi_check_button_area(pointer_sx,pointer_sy,0);
		sample_hmi_request_update();
		sample_hmi_request_action(hmi_onmap_buttonId);
		return(1);
	}
	return(0);
}

