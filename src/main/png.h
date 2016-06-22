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
 * png.h
 *
 *  Created on: 2015/11/05
 *      Author:kanagawa
 */

#ifndef _PNG_H
#define _PNG_H


#ifdef __cplusplus
extern "C" {
#endif

char* readPngData(char* p_Path, int* p_Width, int* p_Height);
char* setPngData(char* data, int filesize,int* p_Width, int* p_Height);

#ifdef __cplusplus
}	// end of the 'extern "C"' block
#endif

#endif // _PNG_H
