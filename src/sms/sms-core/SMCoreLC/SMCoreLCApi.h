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
 * SMCoreLCBApi.h
 *
 *  Created on: 2015/11/09
 *      Author: masutani
 */

#ifndef SMCORELCBAPI_H_
#define SMCORELCBAPI_H_

//typedef void (*LC_LOCATORCBFUNCPTR)();

#ifdef __cplusplus
extern "C" {
#endif
Bool SC_SensorData_Initialize();
Bool SC_SensorData_Finalize();
void LC_LocationUpdateCallback();
E_SC_RESULT LC_InitLocator(const Char *aLocatorDirPath, const SMCARSTATE *aCarState);
E_SC_RESULT LC_SetLocationUpdateCallback(NC_LOCATORCBFUNCPTR pFunc);
E_SC_RESULT LC_GetLocatorVersion(char* locatorVer);
Bool LC_SetVehicleType(INT32 type);
#ifdef __cplusplus
}
#endif

#endif /* SMCORELCBAPI_H_ */
