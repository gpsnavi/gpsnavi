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
 * png.cpp
 *
 *  Created on: 2015/11/05
 *      Author:kanagawa
 */

#include <stdio.h>
#include <string.h>
#include "png.h"
#include "yspng.h"


char* readPngData(char* p_Path, int* p_Width, int* p_Height)
{
	if(NULL == p_Path) {
		return NULL;
	}

	YsRawPngDecoder* png = new YsRawPngDecoder;
	if(NULL == png) {
		return NULL;
	}

	png->Initialize();
	if(png->Decode(p_Path) != YSOK) {
		delete png;
		return NULL;
	}

	*p_Width = png->wid;
	*p_Height = png->hei;

	int size = (png->wid*png->hei*4);
	char* pBuf = new char [size];
	memcpy((void*)pBuf, (void*)png->rgba, size);

	delete png;

	return (char*)pBuf;
}

char* setPngData(char* data, int filesize,int* p_Width, int* p_Height)
{
	if(NULL == data) {
		return NULL;
	}

	YsRawPngDecoder* png = new YsRawPngDecoder;
	if(NULL == png) {
		return NULL;
	}

	png->Initialize();
	if(png->Decode(data,filesize) != YSOK) {
		delete png;
		return NULL;
	}

	*p_Width = png->wid;
	*p_Height = png->hei;

	int size = (png->wid*png->hei*4);
	char* pBuf = new char [size];
	memcpy((void*)pBuf, (void*)png->rgba, size);

	delete png;

	return (char*)pBuf;
}


