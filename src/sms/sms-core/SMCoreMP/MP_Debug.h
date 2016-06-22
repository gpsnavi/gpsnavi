/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef _MP_DEBUG_H
#define _MP_DEBUG_H


#ifdef __cplusplus
extern "C" {
#endif

// エラータイプ
enum{
	ERR_TYPE_ICON_INITIALIZE = 0,	// アイコン初期化失敗

	ERR_TYPE_MAX
};

void MP_DEBUG_Initialize(void);
void MP_DEBUG_SetErrInfo(UINT32 errType);
void MP_DEBUG_DispInfo(void);
void MP_DEBUG_DispWarning(void);
void MP_DEBUG_Set1FrameDrawTime(DOUBLE sec);

#ifdef __cplusplus
}
#endif

#endif // _MP_DEBUG_H
