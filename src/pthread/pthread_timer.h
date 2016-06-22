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
 * pthread_timer.h
 *
 *  Created on: 2015/11/05
 *      Author:t.aikawa
 */

#ifndef INC_PTHREAD_TIMER_H
#define INC_PTHREAD_TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "pthread_msq.h"
//#include "SMUtilInternal.h"
//#include "SMPthreadMsq.h"

#define PTHREAD_TIMER_ONLY_ONCE		(1)
#define PTHREAD_TIMER_REPEAT		(2)

#define PTHREAD_TIMER_ERROR		(0)
#define PTHREAD_TIMER_OK		(1)

int pthreadCheckTimer(pthread_t threadId,int id,int count);
void pthreadInitTimer(void);
int pthreadCreateTimer(pthread_t threadId,pthread_msq_id_t *queue,int userData,int group,int id,int type,int mTime);
int pthreadStartTimer(pthread_t threadId,int id);
int pthreadStopTimer(pthread_t threadId,int id);


#ifdef __cplusplus
}
#endif

#endif	/* INC_PTHREAD_TIMER_H */
