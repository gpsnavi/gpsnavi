/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCAL_CMN_H
#define SMCAL_CMN_H

#define	SC_CAL_MUTEX_INITIALIZER	{}

//-----------------------------------
// 関数定義
//-----------------------------------
E_SC_CAL_RESULT SC_CAL_CreateMutex(SC_CAL_MUTEX *mutex);
E_SC_CAL_RESULT SC_CAL_DestroyMutex(SC_CAL_MUTEX *mutex);
E_SC_CAL_RESULT SC_CAL_LockMutex(SC_CAL_MUTEX *mutex);
E_SC_CAL_RESULT SC_CAL_UnLockMutex(SC_CAL_MUTEX *mutex);
E_SC_CAL_RESULT SC_CAL_CreateSemaphore(SC_CAL_SEMAPHORE *sem, UINT32 val);
E_SC_CAL_RESULT SC_CAL_DestroySemaphore(SC_CAL_SEMAPHORE *sem);
E_SC_CAL_RESULT SC_CAL_LockSemaphore(SC_CAL_SEMAPHORE *sem);
E_SC_CAL_RESULT SC_CAL_LockTimeSemaphore(SC_CAL_SEMAPHORE *sem, UINT32 sec);
E_SC_CAL_RESULT SC_CAL_UnLockSemaphore(SC_CAL_SEMAPHORE *sem);
INT32 SC_CAL_Trim(Char *str);
E_SC_CAL_RESULT SC_CAL_MakeDir(const Char *dirPath);

//-----------------------------------
// ログ出力
//-----------------------------------
#define	SC_CAL_TAG				(Char*)"SMCAL"

#define	_STR(x)					#x
#define	_STR2(x)				_STR(x)
#define	__SLINE__				_STR2(__LINE__)
#define	HERE					"FILE : " __FILE__ "(" __SLINE__ ")\n"

#define _CHECK_ARRAY(array, line)	sizeof(enum { argument_is_not_an_array_##line = ((array)==(const volatile void*)&(array)) })
#define _CHECK_ARRAY2(array, line)	_CHECK_ARRAY(array, line)
#define COUNTOF(array)				(_CHECK_ARRAY2(array, __LINE__), sizeof(array)/sizeof*(array))

#define SC_CAL_LOG_START		(Char*)"[START] %s()\n", __FUNCTION__
#define SC_CAL_LOG_END			(Char*)"[END]   %s()\n", __FUNCTION__

#define USE_IT(x)				(void)(x)	// 未使用引数に対するコンパイル時Warning警告対策

#define SC_CAL_MALLOC(size)		(SC_MEM_Alloc(size, e_MEM_TYPE_DYNAMIC))
#define SC_CAL_FREE(ptr)		({SC_MEM_Free(ptr, e_MEM_TYPE_DYNAMIC); ptr = NULL;})

#endif	// #ifdef SMCAL_CMN_H
