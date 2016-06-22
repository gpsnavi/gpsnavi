/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCORETHREAD_H_
#define SMCORETHREAD_H_

extern Bool isFinish;									// 終了するか否か
#define	SC_Thread_GetIsFinish()		(isFinish)			// 終了するか否か取得
#define	SC_Thread_SetIsFinish(flag)	(isFinish = flag)	// 終了するか否か設定

//-----------------------------------
// 型定義
//-----------------------------------
typedef	pthread_t	SC_THREAD_ID;						// スレッドID

//-----------------------------------
// 関数ポインタ定義
//-----------------------------------
typedef	void *(*SC_THREAD_MAIN_FUNC)(void *param);		// スレッドメイン関数ポインタ

/**
 * SMCoreThread
 */
//-----------------------------------
// 外部I/F定義
//-----------------------------------
E_SC_RESULT SC_Thread_Initialize();
E_SC_RESULT SC_Thread_Finalize();

E_SC_RESULT SC_Thread_Create(SC_THREAD_ID *threadId, SC_THREAD_MAIN_FUNC func, void *param);
E_SC_RESULT SC_Thread_Join(SC_THREAD_ID threadId);


#endif // #ifndef SMCORETHREAD_H_
