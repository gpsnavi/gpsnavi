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
 * navi.c
 *
 *  Created on: 2015/11/05
 *      Author:t.aikawa
 *  Modified on: 2016/09/26
 *      Author:clement.dransart@awtce.be
 */

/*
 * 		1st demo version 2015.11.17		Ver. 0.0.6
 * 		2nd oss  version 2016.06.12		Ver. 0.0.7
 */

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "navicore.h"
#include "glview.h"
#include "png.h"
#include "font.h"
#include "navi.h"
#include "HMI_Icon.h"

#include "server.h"

#define APP_NAME_TEXT		"navi - sample GPS Navigation Version 0.0.7 (build 031 " __DATE__ ")"

static 	char *dpyName = NULL;
GLVContext glv_map_context = 0;
GLVContext glv_hmi_context = 0;

GLVEVENTFUNC_t SurfaceViewEventFunc;
GLVEVENTFUNC_t hmi_SurfaceViewEventFunc;

int hmiMAP_MAX_SCALE;
int main_window_mapScale = 2;
int sample_hmi_load_image_file=0;

/* ----------------------------------------------------------------------- */
#define NAVI_HOME_PATH			"/home/"

#define NAVI_DATA_PATH			"navi_data/"

#define NAVI_CONFIG_PATH_JAPAN		NAVI_HOME_PATH NAVI_DATA_PATH "japan_TR9/"
#define NAVI_CONFIG_PATH_UK			NAVI_HOME_PATH NAVI_DATA_PATH "uk_TR6/"
#define NAVI_CONFIG_PATH_GERMANY	NAVI_HOME_PATH NAVI_DATA_PATH "germany_TR6/"
#define NAVI_CONFIG_PATH_NEVADA		NAVI_HOME_PATH NAVI_DATA_PATH "nevada_TR6/"

#define NAVI_AGL_DEFAULT_PATH_JAPAN	"/var/mapdata/navi_data/japan_TR9"
#define NAVI_AGL_DEFAULT_PATH_UK	"/var/mapdata/navi_data_UK/UnitedKingdom_TR9"

#define NAVI_REGION_OPTIONAL	(-1)
#define NAVI_REGION_JAPAN		(0)
#define NAVI_REGION_UK			(1)
#define NAVI_REGION_GERMANY		(2)
#define NAVI_REGION_NEVADA		(3)

#define NAVI_RESOLUTION_OPTIONAL	(-1)	// --width WIDTH --height HEIGHT
#define NAVI_RESOLUTION_FullHD		(0)	// Full-HD	(1080p)
#define NAVI_RESOLUTION_HD			(1)	// HD		( 720p)
#define NAVI_RESOLUTION_AGL_DEMO	(2)	// CES 2016 demo

char *navi_config_hmi_udi_data_path	= NAVI_HOME_PATH NAVI_DATA_PATH "HMI/";
char *navi_config_hmi_udi_info_file	= NAVI_HOME_PATH NAVI_DATA_PATH "HMI/udi_info";

#define NAVI_DATA_PATH_SIZE		(256)
char navi_config_path[NAVI_DATA_PATH_SIZE]={};
char navi_config_map_db_path[NAVI_DATA_PATH_SIZE]={};
char navi_config_user_data_path[NAVI_DATA_PATH_SIZE]={};
char navi_config_map_udi_data_path[NAVI_DATA_PATH_SIZE]={};
char navi_config_map_udi_info_file[NAVI_DATA_PATH_SIZE]={};
char navi_config_map_font_file[NAVI_DATA_PATH_SIZE]={};

//static int WinWidth = 1080, WinHeight = 1670;	// agl
//static int WinWidth = 1920, WinHeight = 1080;	// Full-HD	(1080p)
static int WinWidth = 1280, WinHeight = 720;	// HD		( 720p)

//static int resolution = NAVI_RESOLUTION_HD;
static int resolution = NAVI_RESOLUTION_AGL_DEMO;

static int region     = NAVI_REGION_JAPAN;
//static int region     = NAVI_REGION_UK;


#define SHM_FILE_MODE (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH)

void *g_GeocordSHM = NULL;
static int g_voicelanguage = 0;

int search_map_data(void)
{
	int ret = -1;
	struct stat sb;
	
	ret = stat(NAVI_AGL_DEFAULT_PATH_UK, &sb);
	if (ret == 0)
	{
		strcpy(navi_config_path, NAVI_AGL_DEFAULT_PATH_UK);
		if ((navi_config_path[strlen(navi_config_path) - 1] != '/') &&
				(sizeof(navi_config_path) > (strlen(navi_config_path) + 1))) {
			strcat(navi_config_path, "/");
		}
		region = NAVI_REGION_OPTIONAL;
		g_voicelanguage = 2;
		
		return 0;
	}
	
	ret = stat(NAVI_AGL_DEFAULT_PATH_JAPAN, &sb);
	if (ret == 0)
	{
		strcpy(navi_config_path, NAVI_AGL_DEFAULT_PATH_JAPAN);
		if ((navi_config_path[strlen(navi_config_path) - 1] != '/') &&
				(sizeof(navi_config_path) > (strlen(navi_config_path) + 1))) {
			strcat(navi_config_path, "/");
		}
		region = NAVI_REGION_OPTIONAL;
		g_voicelanguage = 1;
		
		return 0;
	}
	
	return -1;
}

int Create_GeocordSHM(void)
{
	int fd = 0;
	struct stat sstat;

	memset(&sstat,0,sizeof(sstat));
		
	fd = shm_open("/shm_navigation_geocord", (O_RDWR|O_CREAT), SHM_FILE_MODE);
	ftruncate(fd, 4096);
	fstat(fd, &sstat);
	
	g_GeocordSHM = mmap(NULL,sstat.st_size,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	
	close(fd);
	
	return 0;
}

void naviGetResolution(int *w,int *h)
{
	*w = WinWidth;
	*h = WinHeight;
}
void naviGetRegion(int *r)
{
	*r = region;
}
void naviStartUpRegion(int region)
{
	/* ---------------------------------------------------------------------------------- */
	switch(region){
	case NAVI_REGION_JAPAN:
		strcpy(navi_config_path,NAVI_CONFIG_PATH_JAPAN);
		break;
	case NAVI_REGION_UK:
		strcpy(navi_config_path,NAVI_CONFIG_PATH_UK);
		break;
	case NAVI_REGION_GERMANY:
		strcpy(navi_config_path,NAVI_CONFIG_PATH_GERMANY);
		break;
	case NAVI_REGION_NEVADA:
		strcpy(navi_config_path,NAVI_CONFIG_PATH_NEVADA);
		break;
	default:
		break;
	}

	strcat(navi_config_map_db_path			,navi_config_path); strcat(navi_config_map_db_path			,"Map/JPN/");
	strcat(navi_config_user_data_path		,navi_config_path); strcat(navi_config_user_data_path		,"Data");
	strcat(navi_config_map_udi_data_path	,navi_config_path); strcat(navi_config_map_udi_data_path	,"Data/MD/UDI/");
	strcat(navi_config_map_udi_info_file	,navi_config_path); strcat(navi_config_map_udi_info_file	,"Data/MD/UDI/udi_info");
	strcat(navi_config_map_font_file		,navi_config_path); strcat(navi_config_map_font_file		,"IPAfont00303/ipagp.ttf");

	fprintf(stdout,"navi_config_path             (%s)\n",navi_config_path);
	fprintf(stdout,"navi_config_map_db_path      (%s)\n",navi_config_map_db_path);
	fprintf(stdout,"navi_config_user_data_path   (%s)\n",navi_config_user_data_path);
	fprintf(stdout,"navi_config_map_udi_data_path(%s)\n",navi_config_map_udi_data_path);
	fprintf(stdout,"navi_config_map_udi_info_file(%s)\n",navi_config_map_udi_info_file);
	fprintf(stdout,"navi_config_map_font_file    (%s)\n",navi_config_map_font_file);
	/* ---------------------------------------------------------------------------------- */
}

void naviStartUpResolution(int resolution)
{
	/* ---------------------------------------------------------------------------------- */
	switch(resolution){
	case NAVI_RESOLUTION_FullHD:	// Full-HD	(1080p)
		WinWidth	= 1920;
		WinHeight	= 1080;
		break;
	case NAVI_RESOLUTION_HD:		// HD		( 720p)
		WinWidth	= 1280;
		WinHeight	= 720;
		break;
	case NAVI_RESOLUTION_AGL_DEMO:		// CES 2016 demo
		WinWidth	= 1080;
		WinHeight	= (1920 - 218 - 215);
		break;
	default:
		break;
	}
	//printf("window size %dx%d\n",WinWidth,WinHeight);
	/* ---------------------------------------------------------------------------------- */
}

INT32 BitmapFontCallBack(NCBITMAPFONTINFO* pInfo)
{
	int result = 0;

	initializeBitmapFont();
	setColor(pInfo->color);
	setOutLineColor(pInfo->outLineColor);
	setBkgdColor(pInfo->bkgdColor);

	result = createBitmapFont(
			pInfo->str, pInfo->fontSize, pInfo->outLineFlg,
			&pInfo->pBitMap,
			&pInfo->width, &pInfo->height,
			&pInfo->strWidth, &pInfo->strHeight,
			pInfo->rotation, true);

	if (result != 0) {
		return (0);
	}
	return (1);
}

INT32 ReadImageForFileCallBack(NCBITMAPINFO* pInfo)
{
	pInfo->pBitMap = (UChar *)readPngData(pInfo->path, &pInfo->width, &pInfo->height);
	if (NULL == pInfo->pBitMap) {
		return (0);
	}
	return (1);
}

INT32 SetImageForMemoryCallBack(NCBITMAPINFO* pInfo)
{
	pInfo->pBitMap = (UChar *)setPngData(pInfo->image, pInfo->dataSize, &pInfo->width, &pInfo->height);
	if (NULL == pInfo->pBitMap) {
		return (0);
	}
	return (1);
}

INT32 MapDrawEndCallBack(NCDRAWENDINFO* pInfo)
{
	sample_hmi_draw_compass(pInfo->rotate);
	sample_hmi_request_update();
	return (1);
}

#if 0
static void init(void)
{
	INT32 rc;

	NC_MP_SetBitmapFontCB(BitmapFontCallBack);
	NC_MP_SetImageReadForFileCB(ReadImageForFileCallBack);
	NC_MP_SetImageReadForImageCB(SetImageForMemoryCallBack);
	NC_MP_SetMapDrawEndCB(MapDrawEndCallBack);

	rc = NC_Initialize(WinWidth,WinHeight,navi_config_user_data_path,navi_config_map_db_path,"locatorPath");
	if(NC_SUCCESS != rc){
		fprintf(stderr,"NC_Initialize error.\n");
		fprintf(stderr," user data path(%s)\n",navi_config_user_data_path);
		fprintf(stderr," map  db   path(%s)\n",navi_config_map_db_path);
		exit(-1);
	}

	NC_MP_SetMapMoveWithCar(NC_MP_MAP_MAIN,1);
	NC_MP_SetMapScaleLevel(NC_MP_MAP_MAIN,main_window_mapScale);
}
#endif

int map_init(GLVContext glv_ctx,int maps)
{
	NC_MP_SetUDIResource((Char*)navi_config_map_udi_data_path,(Char*)navi_config_map_udi_info_file);

	NC_MP_InitResource(maps);

	NC_MP_SetMapViewport(maps,0,0,WinWidth, WinHeight);

	sample_init_demo_route_icon();

	return(GLV_OK);
}

int map_reshape(GLVContext glv_ctx,int maps,int width, int height)
{
	NC_MP_SetMapViewport(maps,0,0,width,height);
	return(GLV_OK);
}

int map_redraw(GLVContext glv_ctx,int maps)
{
	NC_MP_RefreshMap(maps);
	return(GLV_OK);
}

int hmi_init(GLVContext glv_ctx,int maps)
{
	void sample_hmi_init(GLVContext glv_ctx);
	void hmiMP_GL_Init(void);

    hmiMP_GL_Init();

    if(sample_hmi_load_image_file == 1){
    	//printf("load image file(%s).\n",navi_config_hmi_udi_data_path);
    	//	HMIで使用するイメージはイメージファイルを読み込んで使用する
    	hmiMP_ICON_Load(navi_config_hmi_udi_data_path, navi_config_hmi_udi_info_file);
    }else{
    	//	HMIで使用するイメージはプログラム中のイメージデータを使用する
    	hmiMP_ICON_Initialize();
    }

	sample_hmi_init(glv_ctx);

	return(GLV_OK);
}

int hmi_update(GLVContext glv_ctx,int maps)
{
	void sample_hmi_update(GLVContext glv_ctx);

	sample_hmi_update(glv_ctx);
	return(GLV_OK);
}

#define GESTURE_FLICK_TIMER_ID			(1000)
#define GESTURE_LONG_PRESS_TIMER_ID		(1001)

#define PI	(3.1415926)
static float calcAngleBaseS(float x, float y)
{
		double acos1 = acos(x / hypot(x, y));
		double radian = (y < 0) ? PI * 2 - acos1 : acos1;
		float angle = (float) ((radian * 180.0 / PI) + 90.0);
		//if(angle > 360.0) angle -= 360.0;
		return angle;
}

/** 初期移動速度割合（8%） */
static float MOVE_RATIO = 0.08f;
/** 移動到達地点倍率（1倍） */
static float GOLE_RATIO = 0.8f;
/** 到達地点補正閾値-短（900） */
static long GOLE_SHORT_CORRECT_LINE = 900;
/** 到達地点補正倍率-短（0.4倍） */
static float GOLE_SHORT_CORRECT_RATIO = 0.4f;
/** 到達地点補正閾値-中（1700） */
static int GOLE_MIDIUM_CORRECT_LINE = 1700;
/** 到達地点補正倍率-中（0.8倍） */
static float GOLE_MIDIUM_CORRECT_RATIO = 0.8f;

static float mAngle;
static float mSpeedRatio;
static double mBaseSpeed;
static double mBaseDistance;
static int mMoveDistance;
static int mGoleDistance;

static void run_scroll(GLVContext glv_ctx,int distanceX,int distanceY)
{
	float	angle;
	int		distance;

	NC_MP_SetMapMoveWithCar(NC_MP_MAP_MAIN,0);
	glvStopTimer(glv_ctx,GESTURE_FLICK_TIMER_ID);

	angle = calcAngleBaseS(-distanceX,-distanceY);
	distance = (int)round(hypot((double)distanceX, (double)distanceY));

	NC_MP_MoveMapDir(NC_MP_MAP_MAIN,angle ,distance);
	NC_MP_RefreshMap(NC_MP_MAP_MAIN);
	glvSwapBuffers(glv_ctx);
}

static void run_flick(GLVContext glv_ctx)
{
	mSpeedRatio = (float) mMoveDistance / (float) mGoleDistance;
	if (1 < mSpeedRatio) {
		glvStopTimer(glv_ctx,GESTURE_FLICK_TIMER_ID);
		return;
	}
	float ratio = (1.0f - mSpeedRatio * mSpeedRatio);
	int distance = (int) round(mBaseSpeed * ratio);

	NC_MP_MoveMapDir(NC_MP_MAP_MAIN,mAngle ,distance);
	NC_MP_RefreshMap(NC_MP_MAP_MAIN);
	glvSwapBuffers(glv_ctx);

	mMoveDistance += distance;

	if (1 > mBaseSpeed * ratio || 0 == distance) {
		//scrollEnd();
		glvStopTimer(glv_ctx,GESTURE_FLICK_TIMER_ID);
	}
}

static int map_long_press_x;
static int map_long_press_y;

int map_gesture(GLVContext glv_ctx,int maps,int eventType,int x,int y,int distanceX,int distanceY,int velocityX,int velocityY)
{
	glvStopTimer(glv_ctx,GESTURE_LONG_PRESS_TIMER_ID);
	switch(eventType)
	 {
	   case    GLV_GESTURE_EVENT_DOWN:
		   //printf("GESTURE:[down(%d,%d)]\n",x,y);
		   glvStartTimer(glv_ctx,GESTURE_LONG_PRESS_TIMER_ID);
		   map_long_press_x = x;
		   map_long_press_y = y;
		   break;
	   case    GLV_GESTURE_EVENT_SINGLE_UP:
		   //printf("GESTURE:[single up(%d,%d) vector:%d %d]\n",x,y,distanceX,distanceY);
		   break;
	   case    GLV_GESTURE_EVENT_SCROLL:
		   //printf("GESTURE:[scroll(%d,%d) vector:%d %d]\n",x,y,distanceX,distanceY);
		   run_scroll(glv_ctx,distanceX,distanceY);
		   break;
	   case    GLV_GESTURE_EVENT_FLING:
		   //printf("GESTURE:[flick(%d,%d) vector:%d %d velocity:%d %d]\n",x,y,distanceX,distanceY,velocityX,velocityY);
		   {
				mMoveDistance = 0;
				mBaseDistance = hypot((double)velocityX, (double)velocityY) * GOLE_RATIO;
				mGoleDistance = round(mBaseDistance);
				if (GOLE_SHORT_CORRECT_LINE > mGoleDistance) {
					mGoleDistance *= GOLE_SHORT_CORRECT_RATIO;
				} else if (GOLE_MIDIUM_CORRECT_LINE > mGoleDistance) {
					mGoleDistance *= GOLE_MIDIUM_CORRECT_RATIO;
				}

				mBaseSpeed = mGoleDistance * MOVE_RATIO;
				mAngle = calcAngleBaseS(-velocityX, -velocityY);
		   }
		   NC_MP_SetMapMoveWithCar(NC_MP_MAP_MAIN,0);
		   run_flick(glv_ctx);
		   glvStartTimer(glv_ctx,GESTURE_FLICK_TIMER_ID);
		   break;
	   case    GLV_GESTURE_EVENT_SCROLL_STOP:
		   //printf("GESTURE:[scroll end(%d,%d) vector:%d %d]\n",x,y,distanceX,distanceY);
		   run_scroll(glv_ctx,distanceX,distanceY);
		   break;
	   default:
		   break;
	 }
	return(GLV_OK);
}

int map_timer(GLVContext glv_ctx,int maps,int group,int id)
{
	if(id == GESTURE_FLICK_TIMER_ID){
		run_flick(glv_ctx);
	}else if(GESTURE_LONG_PRESS_TIMER_ID){
		SMGEOCOORD	geoCood;
		if(NC_Simulation_IsInSimu() != 0){
			/* シミュレーション中 */
			return(GLV_OK);
		}
		//printf("TIMER:long press\n");
		NC_MP_ScreenToGeoCode(NC_MP_MAP_MAIN,map_long_press_x, map_long_press_y, &geoCood);
		//printf("TIMER:latitude(%ld) , longitude(%ld)\n",geoCood.latitude,geoCood.longitude);
		sample_set_demo_icon_pin_flag(&geoCood);
		NC_MP_RefreshMap(NC_MP_MAP_MAIN);
		glvSwapBuffers(glv_ctx);
	}
	return(GLV_OK);
}

static void usage(void)
{
	printf("Usage:\n");
//	printf("  --map  [ uk | nevada ]      set the map region\n");
//	printf("           ~~                 default\n");
	printf("  --r    [ fhd | hd | agl ]   set the resolution\n");
	printf("                      ~~~     default\n");
	printf("  --width  WIDTH              width  of wayland surface\n");
	printf("  --height HEIGHT             height of wayland surface\n");
	printf("  --data   data_path          set the map data path\n");
	printf("  --help                      this help message\n");
}

extern void setPort(long port);
extern void setToken(char *tkn);


int main_arg(int argc, char *argv[])
{
	int width,height;
	int i,n;
	char *home;
	int ret = -1;
	
	setPort(strtol(argv[1], NULL, 10));
	setToken(argv[2]);

	ret = search_map_data();
	if (ret != 0)
	{
		home = getenv("NAVI_DATA_DIR");
		if(home != 0){
			strcpy(navi_config_path,home);
			if ((navi_config_path[strlen(navi_config_path) - 1] != '/') &&
					(sizeof(navi_config_path) > (strlen(navi_config_path) + 1))) {
				strcat(navi_config_path, "/");
			}
			region = NAVI_REGION_OPTIONAL;
		}
	}

	for(i = 3; i < argc; i++) {
		if(strcmp(argv[i], "-display") == 0) {
			dpyName = argv[i+1];
			i++;
		}
		else if(strcmp(argv[i], "-debug") == 0) {
			glvSetDebugFlag(GLV_DEBUG_ON);
		}
		else if (strcmp(argv[i], "--r") == 0) {
			if(strcmp(argv[i+1], "fhd") == 0) {
				resolution = NAVI_RESOLUTION_FullHD;
			}else if(strcmp(argv[i+1], "hd") == 0) {
				resolution = NAVI_RESOLUTION_HD;
			}else if(strcmp(argv[i+1], "agl") == 0) {
				resolution = NAVI_RESOLUTION_AGL_DEMO;
			}
			i++;
		}
		else if (strcmp(argv[i], "--width") == 0) {
			n = sscanf(argv[i+1],"%d",&width);
			if(n ==1){
				resolution = NAVI_RESOLUTION_OPTIONAL;
				WinWidth = width;
			}
			i++;
		}
		else if (strcmp(argv[i], "--height") == 0) {
			n = sscanf(argv[i+1],"%d",&height);
			if(n ==1){
				resolution = NAVI_RESOLUTION_OPTIONAL;
				WinHeight = height;
			}
			i++;
		}
		else if (strcmp(argv[i], "--map") == 0) {
			if(strcmp(argv[i+1], "japan") == 0) {
				region = NAVI_REGION_JAPAN;
			}else if(strcmp(argv[i+1], "uk") == 0) {
				region = NAVI_REGION_UK;
			}else if(strcmp(argv[i+1], "nevada") == 0) {
				region = NAVI_REGION_NEVADA;
			}else if(strcmp(argv[i+1], "germany") == 0) {
				region = NAVI_REGION_GERMANY;
			}
			i++;
		}
		else if (strcmp(argv[i], "--data") == 0) {
			strcpy(navi_config_path,argv[i+1]);
			if ((navi_config_path[strlen(navi_config_path) - 1] != '/') &&
					(sizeof(navi_config_path) > (strlen(navi_config_path) + 1))) {
				strcat(navi_config_path, "/");
			}
			region = NAVI_REGION_OPTIONAL;
			i++;
		}
		else if (strcmp(argv[i], "--help") == 0) {
			usage();
			return(-1);
		}
		else if(strcmp(argv[i], "-hmiimageout") == 0) {
			hmiMP_ICON_CreateResImage();
		}
		else if(strcmp(argv[i], "-hmiimageload") == 0) {
			sample_hmi_load_image_file = 1;
		}
		else {
			usage();
			return(-1);
		}
	}
	return(0);
}

int main(int argc, char *argv[])
{
	GLVDisplay	glv_dpy;
	GLVWindow	glv_map_window;
	GLVWindow	glv_hmi_window;
	int rc;


	fprintf(stderr,"start gps navi\n");	

	if (argc < 2)
	{
		fprintf(stderr,"Error:few args\n");	
	}

	rc = main_arg(argc,argv);
	if(0 != rc){
		return(-1);
	}

	fprintf(stderr,"%s\n",APP_NAME_TEXT);

	naviStartUpResolution(resolution);
	naviStartUpRegion(region);

	if(region == NAVI_REGION_JAPAN){
		set_map_draw_japan(1);
		hmiMAP_MAX_SCALE = 12;
	}else{
		set_map_draw_japan(0);
		hmiMAP_MAX_SCALE = 11;
	}

	glv_dpy = glvOpenDisplay(dpyName);
	if(!glv_dpy){
		printf("Error: glvOpenDisplay() failed\n");
		return(-1);
	}
	/* ----------------------------------------------------------------------------------------------- */
	NC_MP_SetBitmapFontCB(BitmapFontCallBack);
	NC_MP_SetImageReadForFileCB(ReadImageForFileCallBack);
	NC_MP_SetImageReadForImageCB(SetImageForMemoryCallBack);
	NC_MP_SetMapDrawEndCB(MapDrawEndCallBack);

	Create_GeocordSHM();

	rc = NC_Initialize(WinWidth,WinHeight,navi_config_user_data_path,navi_config_map_db_path,"locatorPath");
	if(NC_SUCCESS != rc){
		fprintf(stderr,"NC_Initialize error.\n");
		fprintf(stderr," user data path(%s)\n",navi_config_user_data_path);
		fprintf(stderr," map  db   path(%s)\n",navi_config_map_db_path);
		return(-1);
	}

	if (g_voicelanguage == 1) SC_MNG_SetLanguage(1);
	else if (g_voicelanguage == 2) SC_MNG_SetLanguage(2);
	

	NC_MP_SetMapMoveWithCar(NC_MP_MAP_MAIN,1);
	NC_MP_SetMapScaleLevel(NC_MP_MAP_MAIN,main_window_mapScale);
	/* ----------------------------------------------------------------------------------------------- */
	glv_input_func.keyboard_key = sample_hmi_keyboard_handle_key;
	glv_input_func.touch_down   = sample_hmi_button_down;
	glv_input_func.touch_up     = sample_hmi_button_up;

	glv_map_window = glvCreateNativeWindow(glv_dpy, 0, 0, WinWidth, WinHeight,NULL);
	glv_hmi_window = glvCreateNativeWindow(glv_dpy, 0, 0, WinWidth, WinHeight,glv_map_window);

	glvInitTimer();

	/* ----------------------------------------------------------------------------------------------- */
	SurfaceViewEventFunc.init		= map_init;
	SurfaceViewEventFunc.reshape	= map_reshape;
	SurfaceViewEventFunc.redraw		= map_redraw;
	SurfaceViewEventFunc.update		= NULL;
	SurfaceViewEventFunc.timer		= map_timer;
	SurfaceViewEventFunc.gesture	= map_gesture;

	glv_map_context = glvCreateSurfaceView(glv_map_window,NC_MP_MAP_MAIN,&SurfaceViewEventFunc);

	hmi_SurfaceViewEventFunc.init		= hmi_init;
	hmi_SurfaceViewEventFunc.reshape	= NULL;
	hmi_SurfaceViewEventFunc.redraw		= NULL;
	hmi_SurfaceViewEventFunc.update		= hmi_update;
	hmi_SurfaceViewEventFunc.timer		= NULL;
	hmi_SurfaceViewEventFunc.gesture	= NULL;

	glv_hmi_context = glvCreateSurfaceView(glv_hmi_window,NC_MP_MAP_MAIN,&hmi_SurfaceViewEventFunc);

	/* ----------------------------------------------------------------------------------------------- */
	glvCreateTimer(glv_map_context,1000,GESTURE_FLICK_TIMER_ID     ,GLV_TIMER_REPEAT   ,  50);
	glvCreateTimer(glv_map_context,1000,GESTURE_LONG_PRESS_TIMER_ID,GLV_TIMER_ONLY_ONCE, 700);
	/* ----------------------------------------------------------------------------------------------- */

	sample_createGuideThread();

	glvOnReDraw(glv_map_context);

	//CreateAPIServer();

	glvEventLoop(glv_dpy);

	glvDestroyNativeWindow(glv_map_window);
	glvDestroyNativeWindow(glv_hmi_window);

	glvCloseDisplay(glv_dpy);

	return(0);
}

static int map_draw_japan=0;
void set_map_draw_japan(int japan)
{
	map_draw_japan = japan;
}

int sample_hmi_keyboard_handle_key(
                    unsigned int key,
                    unsigned int state)
{
	if(glv_map_context == 0){
		return(0);
	}
    //fprintf(stderr, "Key is %d state is %d\n", key, state);
    if(state == GLV_KEYBOARD_KEY_STATE_PRESSED){
    	switch(key){
    	case 103:		// '↑'
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
		case 108:		// '↓'
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
		case 25:		// 'p'
			// route search
				sample_calc_demo_route();
			break;
		case 30:		// 'a'
			// own posi mode
			NC_MP_SetMapMoveWithCar(NC_MP_MAP_MAIN,1);
			glvOnReDraw(glv_map_context);
			break;
    	}
    }
    return(0);
}
