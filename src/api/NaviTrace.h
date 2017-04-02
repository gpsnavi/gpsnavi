/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Aisin AW, Ltd
 *
 * This program is licensed under GPL version 2 license.
 * See the LICENSE file distributed with this source file.
 */
#ifndef __NAVI_TRACE_H__
#define __NAVI_TRACE_H__

#ifdef NAVI_TRACE_DEBUG
#define BLACK   "\033[30m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define PURPLE  "\033[35m"
#define DGREEN  "\033[6m"
#define WHITE   "\033[7m"
#define CYAN    "\x1b[36m"
#define NONE    "\033[0m"


#define TRACE_DEBUG(fmt, args...) do { fprintf(stderr, "[%s:%d] " CYAN "DEBUG" NONE "\033[0m: " fmt "\n", __func__, __LINE__, ##args); } while(0)
#define TRACE_INFO(fmt, args...)  do { fprintf(stderr, "[%s:%d] " GREEN "INFO" NONE ":  " fmt "\n", __func__, __LINE__, ##args); } while(0)
#define TRACE_WARN(fmt, args...)  do { fprintf(stderr, "[%s:%d] " YELLOW "WARN"  NONE":  " fmt "\n", __func__, __LINE__, ##args); } while(0)
#define TRACE_ERROR(fmt, args...) do { fprintf(stderr, "[%s:%d] " RED "ERROR" NONE ": " fmt "\n", __func__, __LINE__, ##args); } while(0)
#else
#define TRACE_DEBUG(fmt, args...)
#define TRACE_INFO(fmt, args...)
#define TRACE_WARN(fmt, args...)
#define TRACE_ERROR(fmt, args...)
#endif	//#ifdef NAVI_TRACE_DEBUG
#endif
