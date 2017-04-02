/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 * Copyright (c) 2016  Aisin AW, Ltd.
 *
 * This program is licensed under GPL version 2 license.
 * See the LICENSE file distributed with this source file.
 */

/*
 * guide.c
 *
 *  Created on: 2015/11/05
 *      Author:t.aikawa
 */

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pthread_msq.h"
#include "pthread_timer.h"

#include "navicore.h"
#include "glview.h"
#include "navi.h"

static pthread_msq_id_t guide_queue = PTHREAD_MSQ_ID_INITIALIZER;
static pthread_t guide_threadId;
static SMREALTIMEGUIDEDATA guide_info;
static int current_guide_info_status = 0;
static SMREALTIMEGUIDEDATA current_guide_info;
static pthread_mutex_t pthread_current_guide_info_mutex = PTHREAD_MUTEX_INITIALIZER;

#define GUIDE_ON_TIMER			(1)
#define GUIDE_ON_GUIDE_START	(2)
#define GUIDE_ON_GUIDE_END		(3)

#define GUIDE_TIMER_ID			(1)
#define GUIDE_REPEAT_TIME		(600)

extern void *g_GeocordSHM;

int sample_get_guide_info(SMREALTIMEGUIDEDATA	*guide_info)
{
	int rc = 0;
	pthread_mutex_lock(&pthread_current_guide_info_mutex);
	if(current_guide_info_status != 0){
		memcpy(guide_info,&current_guide_info,sizeof(SMREALTIMEGUIDEDATA));
		rc = 1;
	}
	pthread_mutex_unlock(&pthread_current_guide_info_mutex);
	return(rc);
}

static void *guideThread(void *no_arg)
{
	int	rc = 0;
	pthread_msq_msg_t	rmsg = {};

	pthread_mutex_init(&pthread_current_guide_info_mutex,NULL);

	pthreadCreateTimer(guide_threadId,&guide_queue,GUIDE_ON_TIMER,2000,GUIDE_TIMER_ID,PTHREAD_TIMER_REPEAT,GUIDE_REPEAT_TIME);

#if 0
	rc = pthreadStartTimer(guide_threadId,GUIDE_TIMER_ID);
	rc = pthreadStopTimer(guide_threadId,GUIDE_TIMER_ID);
#endif

	NC_Simulation_SetSpeed(20);	//	シミュレーションのステップ距離を設定する

	while (1) {
		// メッセージ初期化
		memset(&rmsg, 0, sizeof(pthread_msq_msg_t));

		// メッセージ受信
		rc = pthread_msq_msg_receive(&guide_queue, &rmsg);
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
		case GUIDE_ON_TIMER:
				// printf("guide timer\n");
				if(NC_Simulation_IsInSimu() == 0){
					pthreadStopTimer(guide_threadId,GUIDE_TIMER_ID);
					break;
				}
				//printf("NC_DM_GetCarMoveStatus = %d\n",NC_DM_GetCarMoveStatus());
#if 0
				if(SC_DM_GetCarMoveStatus() == 2){
					/* 到着 */
					SC_Guide_StopGuide();				//	経路誘導を終了する
					SC_Simulation_ExitSimulation();		//	シミュレーションを終了する
					pthreadStopTimer(guide_threadId,GUIDE_TIMER_ID);
					break;
				}
#endif
				NC_Guide_RunGuide();		//	経路誘導を実行する
				NC_Simulation_CalcNextPos();	//	GC_Simulation_SetSpeedで設定した距離だけ、シミュレーションでの車両位置を進める
		//		NC_Guide_RunGuide();		//	経路誘導を実行する
				NC_Guide_GetRealTimeInfo(&guide_info);	//	リアルタイムの案内情報を取得する

				{
					
					SMCARSTATE car;
					memset(&car ,0 ,sizeof(SMCARSTATE));
					
					NC_DM_GetCarState(&car, e_SC_CARLOCATION_NOW);
					
					memcpy(g_GeocordSHM,&car,sizeof(SMCARSTATE));
				}

				if((guide_info.turnDir > 0)&&(guide_info.turnDir < 22)){
					sample_set_demo_icon_guide_flag(&guide_info.coord);	// 交差点座標
				}else{
					sample_reset_demo_icon_guide_flag();	// 交差点座標
				}
				if((guide_info.turnDir > 0)&&(guide_info.turnDir < 25)){
					pthread_mutex_lock(&pthread_current_guide_info_mutex);
					memcpy(&current_guide_info,&guide_info,sizeof(SMREALTIMEGUIDEDATA));
					current_guide_info_status = 1;	// 誘導情報を有効にする
					pthread_mutex_unlock(&pthread_current_guide_info_mutex);
				}else{
					current_guide_info_status = 0;	// 誘導情報を無効にする
				}
#ifdef ENABLE_GIDE_TEXT
/* ------------------------------------------------------------------------------------------------- */
{
	char *guideText[]={
	/*[ 0]*/ "",
	/*[ 1]*/ "Uターン",
	/*[ 2]*/ "おおきく右方向です",
	/*[ 3]*/ "右方向です",
	/*[ 4]*/ "ななめ右方向です",
	/*[ 5]*/ "直進",
	/*[ 6]*/ "ななめ左方向です",
	/*[ 7]*/ "左方向です",
	/*[ 8]*/ "おおきく左方向です",
	/*[ 9]*/ "",
	/*[10]*/ "分岐の右方面へ",
	/*[11]*/ "分岐の左方面へ",
	/*[12]*/ "Roundabout出口0",
	/*[13]*/ "Roundabout出口1",
	/*[14]*/ "Roundabout出口2",
	/*[15]*/ "Roundabout出口3",
	/*[16]*/ "Roundabout出口4",
	/*[17]*/ "Roundabout出口5",
	/*[18]*/ "Roundabout出口6",
	/*[19]*/ "Roundabout出口7",
	/*[20]*/ "Roundabout出口8",
	/*[21]*/ "合流ポイント",
	/*[22]*/ "経由地",
	/*[23]*/ "目的地",
	/*[24]*/ "料金所",
	/*[25]*/ ""
	};
	printf("guide.turnDir %d , remainDistToNextTurn %ld m\n",guide_info.turnDir,guide_info.remainDistToNextTurn);
	if((guide_info.turnDir > 0)&&(guide_info.turnDir < 25)){
		printf("%s\n",guideText[guide_info.turnDir]);
	}
}
#endif //#ifdef ENABLE_GIDE_TEXT
/* ------------------------------------------------------------------------------------------------- */
#if 0
			// set value
			env->SetIntField(rtGUIDEDATA, fId_SMREALTIMEGUIDEDATA_iTurnDir, (jint) guide.turnDir);
			env->SetIntField(rtGUIDEDATA, fId_SMREALTIMEGUIDEDATA_iBypass, (jint) guide.bypass);
			env->SetIntField(rtGUIDEDATA, fId_SMREALTIMEGUIDEDATA_iRoadSituation, (jint) guide.roadSituation);
			env->SetIntField(rtGUIDEDATA, fId_SMREALTIMEGUIDEDATA_byNextBypassIndex, (jint) guide.nextBypassIndex);
			env->SetBooleanField(rtGUIDEDATA, fId_SMREALTIMEGUIDEDATA_bShowTrafficLight, (jboolean) guide.showTrafficLight);
			env->SetBooleanField(rtGUIDEDATA, fId_SMREALTIMEGUIDEDATA_bDestination, (jboolean) guide.destination);
			env->SetBooleanField(rtGUIDEDATA, fId_SMREALTIMEGUIDEDATA_bAheadPoint, (jboolean) guide.aheadPoint);
			env->SetBooleanField(rtGUIDEDATA, fId_SMREALTIMEGUIDEDATA_bValid, (jboolean) guide.valid);
			env->SetLongField(rtGUIDEDATA, fId_SMREALTIMEGUIDEDATA_lRemainTimeToNextPlace, (jlong) guide.remainTimeToNextPlace);
			env->SetLongField(rtGUIDEDATA, fId_SMREALTIMEGUIDEDATA_lRemainDistToNextPlace, (jlong) guide.remainDistToNextPlace);
			env->SetLongField(rtGUIDEDATA, fId_SMREALTIMEGUIDEDATA_lRemainDistToNextTurn, (jlong) guide.remainDistToNextTurn);
			env->SetLongField(rtGUIDEDATA, fId_SMREALTIMEGUIDEDATA_lPassedDistance, (jlong) guide.passedDistance);
			env->SetObjectField(rtGUIDEDATA, fId_SMREALTIMEGUIDEDATA_strNextBroadString, jstrBuf);
#endif
			sample_hmi_set_fource_update_mode();
			sample_hmi_request_mapDraw();
			break;
		case GUIDE_ON_GUIDE_START:
			current_guide_info_status = 0;	// 誘導情報を無効にする
#if 1		/* 誘導が開始しない・・・なぜかわからないけど、おまじない。 */
			NC_Simulation_StartSimulation();	//	シミュレーションを開始する
			NC_Guide_StartGuide();				//	経路誘導を開始する
			NC_Guide_StopGuide();				//	経路誘導を終了する
			NC_Simulation_ExitSimulation();		//	シミュレーションを終了する
#endif

			NC_Simulation_StartSimulation();	//	シミュレーションを開始する
			NC_Guide_StartGuide();				//	経路誘導を開始する
			pthreadStartTimer(guide_threadId,GUIDE_TIMER_ID);
			break;
		case GUIDE_ON_GUIDE_END:
			current_guide_info_status = 0;	// 誘導情報を無効にする
			NC_Guide_StopGuide();				//	経路誘導を終了する
			NC_Simulation_ExitSimulation();		//	シミュレーションを終了する
			pthreadStopTimer(guide_threadId,GUIDE_TIMER_ID);
			NC_RP_DeleteRouteResult();
			sample_clear_demo_route_icon();		//	デモ用アイコン　スタート、ゴール、誘導ピン　を消す
			sample_hmi_set_fource_update_mode();
			sample_hmi_request_mapDraw();
			break;
		default:
			break;
		}
	}
	return NULL;
}

void sample_createGuideThread(void)
{
	int		pret = 0;

	// メッセージキュー生成
	if (0 != pthread_msq_create(&guide_queue, 50)) {
		fprintf(stderr,"createGuideThread:Error: pthread_msq_create() failed\n");
		exit(-1);
	}
	// スレッド生成
	pret = pthread_create(&guide_threadId, NULL, guideThread, NULL);

	if (0 != pret) {
		fprintf(stderr,"createGuideThread:Error: pthread_create() failed\n");
	}
}

void sample_guide_request_start(void)
{
	pthread_msq_msg_t smsg;

	smsg.data[0] = GUIDE_ON_GUIDE_START;

	pthread_msq_msg_send(&guide_queue,&smsg,0);
}

void sample_guide_request_end(void)
{
	pthread_msq_msg_t smsg;

	smsg.data[0] = GUIDE_ON_GUIDE_END;

	pthread_msq_msg_send(&guide_queue,&smsg,0);
}
