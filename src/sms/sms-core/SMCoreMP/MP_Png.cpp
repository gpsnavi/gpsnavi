/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#include "SMCoreMPInternal.h"

static NC_IMAGEFUNCPTR gpFuncForFile = NULL;
static NC_IMAGEFUNCPTR gpFuncForImage = NULL;

E_SC_RESULT MP_PNG_SetImageReadForFileCB(NC_IMAGEFUNCPTR pfunc)
{
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	gpFuncForFile = pfunc;
	return (ret);
}

E_SC_RESULT MP_PNG_SetImageReadForImageCB(NC_IMAGEFUNCPTR pfunc)
{
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	gpFuncForImage = pfunc;
	return (ret);
}

MP_PING_HDL MP_PNG_CreatePngForFile(char* pPath, INT32* pWidth, INT32* pHeight)
{
	Bool bRet = true;
	NCBITMAPINFO info = {};

	if(NULL == gpFuncForFile) {
		return (NULL);
	}

	info.path = pPath;

	//画像読み込みCB
	bRet = (gpFuncForFile)(&info);
	if (!bRet) {
		return (NULL);
	}

	*pWidth = info.width;
	*pHeight = info.height;

	return ((MP_PING_HDL)info.pBitMap);
}

MP_PING_HDL MP_PNG_CreatePngForImage(char* pData, INT32* pWidth, INT32* pHeight,INT32 size)
{
	Bool bRet = true;
	NCBITMAPINFO info = {};

	if(NULL == gpFuncForImage) {
		return (NULL);
	}

	info.image = pData;
	info.dataSize = size;

	//画像読み込みCB
	bRet = (gpFuncForImage)(&info);
	if (!bRet) {
		return (NULL);
	}

	*pWidth = info.width;
	*pHeight = info.height;

	return ((MP_PING_HDL)info.pBitMap);
}

void MP_PNG_DeletePng(MP_PING_HDL pngHdl)
{
	if(NULL != pngHdl) {
		delete [] (UChar*)pngHdl;
	}
}
