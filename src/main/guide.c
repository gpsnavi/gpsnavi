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
SMREALTIMEGUIDEDATA guide_info;

#define GUIDE_ON_TIMER			(1)
#define GUIDE_ON_GUIDE_START	(2)
#define GUIDE_ON_GUIDE_END		(3)

#define GUIDE_TIMER_ID			(1)
#define GUIDE_REPEAT_TIME		(300)

static void *guideThread(void *no_arg)
{
	int	rc = 0;
	pthread_msq_msg_t	rmsg = {};

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
				printf("NC_DM_GetCarMoveStatus = %d\n",NC_DM_GetCarMoveStatus());
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

				printf("guide.turnDir %d\n",guide_info.turnDir);
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
		printf("createGuideThread:Error: pthread_msq_create() failed\n");
		exit(-1);
	}
	// スレッド生成
	pret = pthread_create(&guide_threadId, NULL, guideThread, NULL);

	if (0 != pret) {
		printf("createGuideThread:Error: pthread_create() failed\n");
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
