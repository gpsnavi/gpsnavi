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
 * HMI_Icon.h
 *
 *  Created on: 2015/11/05
 *      Author:t.aikawa
 */

#ifndef _MP_ICON_H
#define _MP_ICON_H

#include "navicore.h"

#ifdef __cplusplus
extern "C" {
#endif

void  hmiMP_ICON_CreateResImage(void);

INT32 hmiMP_ICON_Load(char* p_Path, char* p_InfoPath);
INT32 hmiMP_ICON_Initialize(void);
INT32 hmiMP_ICON_Finalize(void);
INT32 hmiMP_ICON_Draw(FLOAT x, FLOAT y, FLOAT angle, FLOAT scale, INT32 iconID);
INT32 mapMP_ICON_Draw(FLOAT x, FLOAT y, FLOAT angle, FLOAT scale, INT32 iconID);

#ifdef __cplusplus
}
#endif

#endif // _MP_ICON_H
