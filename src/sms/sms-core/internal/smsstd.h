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
 * smsstd.h
 *
 *  Created on: 2015/11/05
 *      Author: masutani
 */

#ifndef SMSSTD_H_
#define SMSSTD_H_

/**
 * android
 */
#ifdef ANDROID
#include <android/log.h>
#endif // ANDROID

/**
 * standard
 */
#include <arpa/inet.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <math.h>
#include <float.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <resolv.h>
#include <semaphore.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <time.h>
#include <unistd.h>

/**
 * lib
 */
//#include <sqlite3/sqlite3.h>
#include <sqlite3.h>
#include <smsutil/smsutil.h>
#if 0	/* aikawa 2016/04/14 暫定対策 */
#include <tgz/untgz.h>
#endif
#ifdef NC_LOCATOR_INCLUDE	// AIKAWA.AIKAWA
#include <Hilocator.h>
#endif // NC_LOCATOR_INCLUDE
#if 0
#include <expat-2.1.0/expat.h>
#else
#include <expat.h>
#endif
#include <openssl/ssl.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
//#include <zlib/zlib.h>
#include <zlib.h>

/**
 * C++
 */
#ifdef __cplusplus
#include <string>
#include <list>
#include <vector>
#include <map>
#include <bitset>
#endif // __cplusplus

#endif /* SMSSTD_H_ */
