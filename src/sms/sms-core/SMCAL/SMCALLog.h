/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCAL_LOG_H
#define SMCAL_LOG_H

E_SC_CAL_RESULT SC_CAL_LOG_Initialize(SMCALLOG *log);
E_SC_CAL_RESULT SC_CAL_LOG_Finalize(SMCALLOG *log);
E_SC_CAL_RESULT SC_CAL_LOG_Print(SMCALLOG *log, SC_CAL_LOG_LV lv, const Char *tag, const Char *fmt, ...);
E_SC_CAL_RESULT SC_CAL_LOG_DebugPrint(SMCALLOG *log, const Char *tag, const Char *fmt, ...);
E_SC_CAL_RESULT SC_CAL_LOG_InfoPrint(SMCALLOG *log, const Char *tag, const Char *fmt, ...);
E_SC_CAL_RESULT SC_CAL_LOG_WarnPrint(SMCALLOG *log, const Char *tag, const Char *fmt, ...);
E_SC_CAL_RESULT SC_CAL_LOG_ErrorPrint(SMCALLOG *log, const Char *tag, const Char *fmt, ...);

#endif // #ifndef SMCAL_LOG_H
