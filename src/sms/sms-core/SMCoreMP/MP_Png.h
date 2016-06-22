/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef _MP_PNG_H
#define _MP_PNG_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void*	MP_PING_HDL;

E_SC_RESULT MP_PNG_SetImageReadForFileCB(NC_IMAGEFUNCPTR pfunc);
E_SC_RESULT MP_PNG_SetImageReadForImageCB(NC_IMAGEFUNCPTR pfunc);
MP_PING_HDL MP_PNG_CreatePngForFile(char* pPath, INT32* pWidth, INT32* pHeight);
MP_PING_HDL MP_PNG_CreatePngForImage(char* pData, INT32* pWidth, INT32* pHeight,INT32 size);
void MP_PNG_DeletePng(MP_PING_HDL pngHdl);

#ifdef __cplusplus
}	// end of the 'extern "C"' block
#endif

#endif // _MP_PNG_H
