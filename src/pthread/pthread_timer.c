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
 * pthread_timer.c
 *
 *  Created on: 2015/11/05
 *      Author:t.aikawa
 */

#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include "pthread_timer.h"


#define PTHREAD_TIMER_STOP	(0)
#define PTHREAD_TIMER_START	(1)

typedef struct _pthreadTimerTable {
	pthread_t	threadId;
	int		userData;
	int		group;
	int 	id;
	int 	type;
	int 	active;
	int		mTime;	/* タイマーの待ち時間 */
	long	reqCount;
	struct timespec ts; /* 待ち時間の絶対値　mTime生成する */
	pthread_msq_id_t	*queue;
} PTHREADTIMERTABLE_t;

static sem_t timer_sem;
#define PTHREAD_TIMER_TABLE_MAX		(10)
static PTHREADTIMERTABLE_t pthread_timer_table[PTHREAD_TIMER_TABLE_MAX];

/* ----------------------------------------------------------------- */
static pthread_mutex_t pthread_timer_mutex = PTHREAD_MUTEX_INITIALIZER;

static int pthreadCalcTime(struct timespec *start_ts,int mTime,struct timespec *stop_ts)
{
	time_t tv_sec;
	__syscall_slong_t tv_nsec;

	tv_sec  = mTime / 1000;
	tv_nsec = (mTime % 1000) * 1000000;

	stop_ts->tv_sec  = start_ts->tv_sec + tv_sec + (start_ts->tv_nsec + tv_nsec) / 1000000000;
	stop_ts->tv_nsec = (start_ts->tv_nsec + tv_nsec) % 1000000000;
	return(PTHREAD_TIMER_OK);
}

int pthreadCreateTimer(pthread_t threadId,pthread_msq_id_t *queue,int userData,int group,int id,int type,int mTime)
{
	int i;

	if((type != PTHREAD_TIMER_ONLY_ONCE) &&
			(type != PTHREAD_TIMER_REPEAT)){
		return(PTHREAD_TIMER_ERROR);
	}

	if(id == 0){
		return(PTHREAD_TIMER_ERROR);
	}

	pthread_mutex_lock(&pthread_timer_mutex);

	for(i=0;i<PTHREAD_TIMER_TABLE_MAX;i++)
	{
		if((pthread_timer_table[i].id == id)&&
			(pthread_equal(pthread_timer_table[i].threadId,threadId)))
		{
			pthread_timer_table[i].group		= group;
			pthread_timer_table[i].userData		= userData;
			pthread_timer_table[i].type			= type;
			pthread_timer_table[i].active		= PTHREAD_TIMER_STOP;
			pthread_timer_table[i].mTime		= mTime;
			pthread_timer_table[i].ts.tv_sec	= 0;
			pthread_timer_table[i].ts.tv_nsec	= 0;
			//printf("pthreadCreateTimer:timer reuse  [%d] \n",i);
			pthread_mutex_unlock(&pthread_timer_mutex);
			return(PTHREAD_TIMER_OK);
		}
	}
	for(i=0;i<PTHREAD_TIMER_TABLE_MAX;i++)
	{
		if(pthread_timer_table[i].id == 0)
		{
			pthread_timer_table[i].threadId		= threadId;
			pthread_timer_table[i].userData		= userData;
			pthread_timer_table[i].group		= group;
			pthread_timer_table[i].id			= id;
			pthread_timer_table[i].type			= type;
			pthread_timer_table[i].active		= PTHREAD_TIMER_STOP;
			pthread_timer_table[i].mTime		= mTime;
			pthread_timer_table[i].ts.tv_sec	= 0;
			pthread_timer_table[i].ts.tv_nsec	= 0;
			pthread_timer_table[i].queue		= queue;
			//printf("pthreadCreateTimer:timer create [%d] \n",i);
			pthread_mutex_unlock(&pthread_timer_mutex);
			return(PTHREAD_TIMER_OK);
		}
	}

	pthread_mutex_unlock(&pthread_timer_mutex);
	printf("pthreadCreateTimer:timer tavle over [%d] \n",i);
	/* table over */
	return(PTHREAD_TIMER_ERROR);
}

int pthreadStartTimer(pthread_t threadId,int id)
{
    static struct timespec ts;
	int i;

	pthread_mutex_lock(&pthread_timer_mutex);

	for(i=0;i<PTHREAD_TIMER_TABLE_MAX;i++)
	{
		if((pthread_timer_table[i].id == id)&&
			(pthread_equal(pthread_timer_table[i].threadId,threadId)))
		{
			if (clock_gettime(CLOCK_REALTIME, &ts) == 0){
			    pthreadCalcTime(&ts,pthread_timer_table[i].mTime,&pthread_timer_table[i].ts);
				pthread_timer_table[i].active = PTHREAD_TIMER_START;
				sem_post(&timer_sem); /* タイマーの待を解除 */
			}
			break;
		}
	}
	pthread_mutex_unlock(&pthread_timer_mutex);

	return(PTHREAD_TIMER_OK);
}

int pthreadStopTimer(pthread_t threadId,int id)
{
	int i;

	pthread_mutex_lock(&pthread_timer_mutex);

	for(i=0;i<PTHREAD_TIMER_TABLE_MAX;i++)
	{
		if((pthread_timer_table[i].id == id)&&
			(pthread_equal(pthread_timer_table[i].threadId,threadId)))
		{
			pthread_timer_table[i].active = PTHREAD_TIMER_STOP;
			pthread_timer_table[i].reqCount++;
			pthread_mutex_unlock(&pthread_timer_mutex);
			return(PTHREAD_TIMER_OK);
		}
	}
	pthread_mutex_unlock(&pthread_timer_mutex);
	return(PTHREAD_TIMER_OK);
}

int pthreadCheckTimer(pthread_t threadId,int id,int count)
{
	int i;

	pthread_mutex_lock(&pthread_timer_mutex);

	for(i=0;i<PTHREAD_TIMER_TABLE_MAX;i++)
	{
		if((pthread_timer_table[i].id == id)&&
			(pthread_equal(pthread_timer_table[i].threadId,threadId)))
		{
			if(pthread_timer_table[i].reqCount == count){
				pthread_mutex_unlock(&pthread_timer_mutex);
				return(PTHREAD_TIMER_OK);
			}
		}
	}
	pthread_mutex_unlock(&pthread_timer_mutex);

	return(PTHREAD_TIMER_ERROR);
}

int pthreadGroupStopTimer(pthread_t threadId,int group)
{
	int i;

	pthread_mutex_lock(&pthread_timer_mutex);

	for(i=0;i<PTHREAD_TIMER_TABLE_MAX;i++)
	{
		if((pthread_timer_table[i].group == group)&&
			(pthread_equal(pthread_timer_table[i].threadId,threadId)))
		{
			pthread_timer_table[i].active = PTHREAD_TIMER_STOP;
			pthread_timer_table[i].reqCount++;
		}
	}
	pthread_mutex_unlock(&pthread_timer_mutex);

	return(PTHREAD_TIMER_OK);
}
int pthreadAllStopTimer(pthread_t threadId)
{
	int i;

	pthread_mutex_lock(&pthread_timer_mutex);

	for(i=0;i<PTHREAD_TIMER_TABLE_MAX;i++)
	{
		if(pthread_equal(pthread_timer_table[i].threadId,threadId))
		{
			pthread_timer_table[i].active = PTHREAD_TIMER_STOP;
			pthread_timer_table[i].reqCount++;
		}
	}
	pthread_mutex_unlock(&pthread_timer_mutex);

	return(PTHREAD_TIMER_OK);
}
int pthreadDestroyTimer(void)
{
	int i;

	pthread_mutex_lock(&pthread_timer_mutex);

	for(i=0;i<PTHREAD_TIMER_TABLE_MAX;i++)
	{
		pthread_timer_table[i].id		= 0;
		pthread_timer_table[i].active	= PTHREAD_TIMER_STOP;
		pthread_timer_table[i].reqCount++;
	}
	pthread_mutex_unlock(&pthread_timer_mutex);

	sem_destroy(&timer_sem); /* セマフォを削除する */
	pthread_mutex_destroy(&pthread_timer_mutex); /* ミューテックスを破壊する */

	return(PTHREAD_TIMER_OK);
}

int pthreadGetTime(struct timespec *ts)
{
	int i;
	long min_sec,min_nsec;

    clock_gettime(CLOCK_REALTIME, ts);

	pthread_mutex_lock(&pthread_timer_mutex);

	min_sec  = -1;
	min_nsec = -1;

	for(i=0;i<PTHREAD_TIMER_TABLE_MAX;i++)
	{
		if(pthread_timer_table[i].active == PTHREAD_TIMER_START)
		{
			if(min_sec < 0){
				min_sec = pthread_timer_table[i].ts.tv_sec;
				min_nsec = pthread_timer_table[i].ts.tv_nsec;
			}else{
				if(min_sec > pthread_timer_table[i].ts.tv_sec){
					min_sec = pthread_timer_table[i].ts.tv_sec;
					min_nsec = pthread_timer_table[i].ts.tv_nsec;
				}else if(min_sec == pthread_timer_table[i].ts.tv_sec){
					if(min_nsec > pthread_timer_table[i].ts.tv_nsec){
						min_nsec = pthread_timer_table[i].ts.tv_nsec;
					}
				}
			}
		}
	}
	pthread_mutex_unlock(&pthread_timer_mutex);
	if(min_sec == -1){
		ts->tv_sec += 60;
		return(PTHREAD_TIMER_OK);
	}
	ts->tv_sec  = min_sec;
	ts->tv_nsec = min_nsec;
	return(PTHREAD_TIMER_OK);
}

void pthreadSendTime(void)
{
    static struct timespec crt_ts;
	pthread_msq_msg_t smsg;
	pthread_msq_id_t *queue;
	int i,time_flag;

	clock_gettime(CLOCK_REALTIME, &crt_ts);

	pthread_mutex_lock(&pthread_timer_mutex);

	for(i=0;i<PTHREAD_TIMER_TABLE_MAX;i++)
	{
		time_flag = 0;
		if(pthread_timer_table[i].active == PTHREAD_TIMER_START)
		{
			if(crt_ts.tv_sec > pthread_timer_table[i].ts.tv_sec){
				time_flag = 1;
			}else if(crt_ts.tv_sec == pthread_timer_table[i].ts.tv_sec){
				if(crt_ts.tv_nsec > pthread_timer_table[i].ts.tv_nsec){
					time_flag = 1;
				}
			}
			if(time_flag == 1){
				if(pthread_timer_table[i].type == PTHREAD_TIMER_ONLY_ONCE){
					pthread_timer_table[i].active = PTHREAD_TIMER_STOP;
				}else{
					pthreadCalcTime(&crt_ts,pthread_timer_table[i].mTime,&pthread_timer_table[i].ts);
			    }
				smsg.data[0] = pthread_timer_table[i].userData;
				smsg.data[1] = 0;
				smsg.data[2] = pthread_timer_table[i].group;
				smsg.data[3] = pthread_timer_table[i].id;
				smsg.data[4] = pthread_timer_table[i].reqCount;
				queue		 = pthread_timer_table[i].queue;

				pthread_mutex_unlock(&pthread_timer_mutex);

				if(queue != NULL){
					pthread_msq_msg_send(queue,&smsg,0);
				}

				pthread_mutex_lock(&pthread_timer_mutex);
			}
		}
	}
	pthread_mutex_unlock(&pthread_timer_mutex);
}

void *pthreadTimerProc(void* no_arg)
{
    static struct timespec ts;
    int rc;

	do{
	    pthreadGetTime(&ts);

	    //printf("pthreadTimerProc:sem_timedwait():call\n");
	    while ((rc = sem_timedwait(&timer_sem, &ts)) == -1 && errno == EINTR)
	        continue;       /* Restart if interrupted by handler */

#if 0
	    /* Check what happened */
	    if (rc == -1) {
	        if (errno == ETIMEDOUT){
	            printf("pthreadTimerProc:sem_timedwait:timed out\n");
	        	/* タイマーに設定した時間が経過した */
	        }else{
	        	printf("pthreadTimerProc:sem_timedwait:error (%d)\n",errno);
	        }
	    } else{
	        printf("pthreadTimerProc:sem_timedwait:succeeded\n");
	        /* タイマーの再設定要求が発生 */
	    }
#endif
	    pthreadSendTime();
	}while(1);

	return(NULL);
}

void pthreadInitTimer(void)
{
	int		pret = 0;
	int		i;
	pthread_t threadId;

	pthread_mutex_init(&pthread_timer_mutex,NULL);

	for(i=0;i<PTHREAD_TIMER_TABLE_MAX;i++)
	{
		pthread_timer_table[i].threadId		= 0;
		pthread_timer_table[i].group		= 0;
		pthread_timer_table[i].id			= 0;
		pthread_timer_table[i].type			= 0;
		pthread_timer_table[i].active		= PTHREAD_TIMER_STOP;
		pthread_timer_table[i].reqCount		= 0;
		pthread_timer_table[i].mTime		= 0;
		pthread_timer_table[i].ts.tv_sec	= 0;
		pthread_timer_table[i].ts.tv_nsec	= 0;
		pthread_timer_table[i].queue		= NULL;
	}
    if (sem_init(&timer_sem, 0, 0) == -1){
		printf("pthreadTimeProc:Error: sem_init() failed\n");
    	return;
    }

	// スレッド生成
	pret = pthread_create(&threadId, NULL, pthreadTimerProc, (void *)NULL);

	if (0 != pret) {

		return;
	}

	return;
}
