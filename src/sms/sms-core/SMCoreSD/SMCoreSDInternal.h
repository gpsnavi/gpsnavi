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
 * SMCoreSDInternal.h
 *
 *  Created on: 2015/11/05
 *      Author: masutani
 */

#ifndef SMCORESDINTERNAL_H_
#define SMCORESDINTERNAL_H_

#include "sms-core/internal/smsstd.h"
#include "sms-core/internal/smstypedef.h"
#include "sms-core/internal/smsutil.h"
#include "sms-core/internal/smscore.h"
#include "sms-core/internal/smscorecmn.h"

#include "sms-core/internal/smscoredal.h"

#include "sms-core/internal/smscal.h"
#include "sms-core/internal/smsccomcmn.h"
#include "sms-core/internal/smsccomdal.h"
#include "sms-core/internal/smsccom.h"

#include "SDD/SMCoreSDD.h"
#include "SDD/SMCoreSDDDispatch.h"
#include "SDD/SMCoreSDDThread.h"

#include "SDM/SMCoreSDM.h"
#include "SDM/SMCoreSDMDispatch.h"
#include "SDM/SMCoreSDMThread.h"

#include "SDT/SMCoreSDT.h"
#include "SDT/SMCoreSDTDispatch.h"
#include "SDT/SMCoreSDTThread.h"

#include "SDU/SMCoreSDU.h"
#include "SDU/SMCoreSDUDispatch.h"
#include "SDU/SMCoreSDUThread.h"

#include "SDU/SMCoreSDUJson.h"

E_SC_RESULT SC_SD_ConvertResult(E_SC_CAL_RESULT calRet);

#endif /* SMCORESDINTERNAL_H_ */
