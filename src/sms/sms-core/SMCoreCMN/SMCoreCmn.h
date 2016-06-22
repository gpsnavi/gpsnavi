/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCORE_CMN_H
#define SMCORE_CMN_H

#define	SC_MUTEX_INITIALIZER	{}

//-----------------------------------
// ログ出力
//-----------------------------------
#define	SC_TAG_NC			"NaviCore"
#define	SC_TAG_CORE			"SMCore"
#define	SC_TAG_MEM			"SMCoreMem"
#define	SC_TAG_SHARE		"SMCoreShare"
#define	SC_TAG_CASH			"SMCoreCash"
#define	SC_TAG_FM			"SMCoreFM"
#define	SC_TAG_MP			"SMCoreMP"
#define	SC_TAG_RM			"SMCoreRM"
#define	SC_TAG_RC			"SMCoreRC"
#define	SC_TAG_RG			"SMCoreRG"
#define	SC_TAG_RT			"SMCoreRT"
#define	SC_TAG_DH			"SMCoreDH"
#define	SC_TAG_DHC			"SMCoreDHC"
#define	SC_TAG_LC			"SMCoreLC"
#define	SC_TAG_CC			"SMCC"
#define	SC_TAG_PM			"SMPM"
#define	SC_TAG_PU			"SMPU"
#define	SC_TAG_PT			"SMPT"
#define	SC_TAG_PDAL			"SMPDAL"
#define SC_TAG_SDM			"SMSDM"
#define SC_TAG_SDD			"SMSDD"
#define SC_TAG_SDU			"SMSDU"
#define SC_TAG_SDT			"SMSDT"
#define SC_TAG_SDP			"SMSDP"
#define	SC_TAG_DAL			"SMCoreDAL"
#define	SC_TAG_PAL			"SMCorePAL"
#define	SC_TAG_TR			"SMCoreTR"
#define	SC_TAG_TRT			"SMCoreTRT"


#define	_STR(x)				#x
#define	_STR2(x)			_STR(x)
#define	__SLINE__			_STR2(__LINE__)
#define	HERE				"FILE : " __FILE__ "(" __SLINE__ ")\n"

#define _CHECK_ARRAY(array, line)	sizeof(enum { argument_is_not_an_array_##line = ((array)==(const volatile void*)&(array)) })
#define _CHECK_ARRAY2(array, line)	_CHECK_ARRAY(array, line)
#ifdef __SMS_APPLE__
#define COUNTOF(array)				(sizeof(array)/sizeof(*(array)))
#else
#define COUNTOF(array)				(_CHECK_ARRAY2(array, __LINE__), sizeof(array)/sizeof*(array))
#endif /* __SMS_APPLE__ */

#define SC_LOG_START		"[START] %s()\n", __FUNCTION__
#define SC_LOG_END			"[END]   %s()\n", __FUNCTION__

#define USE_IT(x)			(void)(x)	// 未使用引数に対するコンパイル時Warning警告対策

#define SC_MALLOC(size)		(SC_MEM_Alloc(size, e_MEM_TYPE_DYNAMIC))
#define SC_FREE(ptr)		({SC_MEM_Free(ptr, e_MEM_TYPE_DYNAMIC); ptr = NULL;})

#define ISLITTLENDIAN		(IsLittleEndian())
// リトルエンディアンに変換する
#define CONVERT_ENDIAN_INT16(val)	(((val<<8) & 0xff00) | ((val>>8) & 0x00ff))
#define CONVERT_ENDIAN_INT32(val)	(((val<<24) & 0xff000000) | ((val<<8) & 0x00ff0000) | ((val>>8) & 0x0000ff00) | ((val>>24) & 0x000000ff))
#define CONVERT_LITTLE_ENDIAN_INT32(val)	((ISLITTLENDIAN) ? (val) : (CONVERT_ENDIAN_INT32(val)))
#define CONVERT_LITTLE_ENDIAN_INT16(val)	((ISLITTLENDIAN) ? (val) : (CONVERT_ENDIAN_INT16(val)))

// ビッグエンディアンをエンディアンに合わせて変換する
#define CONVERT_BIG_ENDIAN_INT32(val)		((ISLITTLENDIAN) ? (CONVERT_ENDIAN_INT32(val)) : (val))
#define CONVERT_BIG_ENDIAN_INT16(val)		((ISLITTLENDIAN) ? (CONVERT_ENDIAN_INT16(val)) : (val))

#endif	// #ifdef SMCORE_CMN_H
