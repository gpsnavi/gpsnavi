/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef _MP_ICON_H
#define _MP_ICON_H

#ifdef __cplusplus
extern "C" {
#endif

E_SC_RESULT MP_ICON_Initialize(char* p_Path, char* p_InfoPath);
E_SC_RESULT MP_ICON_Finalize(void);
E_SC_RESULT MP_ICON_Draw(FLOAT x, FLOAT y, FLOAT angle, FLOAT scale, INT32 iconID);

#ifdef __cplusplus
}
#endif

#endif // _MP_ICON_H
