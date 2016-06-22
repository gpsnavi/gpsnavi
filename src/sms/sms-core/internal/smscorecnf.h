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
 * smscorecnf.h
 *
 *  Created on: 2015/11/06
 *      Author: masutani
 */

#ifndef SMSCORECNF_H_
#define SMSCORECNF_H_

// TODO
#ifdef __SMS_APPLE__
#define		SC_MAX_PATH					512										// パス最大長
#else
#define		SC_MAX_PATH					260										// パス最大長
#endif /* __SMS_APPLE__ */

#include "sms-core/SMCoreCNF/SMCoreCNFApi.h"

#endif /* SMSCORECNF_H_ */
