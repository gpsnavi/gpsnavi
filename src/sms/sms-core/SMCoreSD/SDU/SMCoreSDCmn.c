/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#include "sms-core/SMCoreSD/SMCoreSDInternal.h"

UINT32	uploadErrorNum;

// 戻り値変換（E_SC_CAL_RESULT → E_SC_RESULT）
E_SC_RESULT SC_SD_ConvertResult(E_SC_CAL_RESULT calRet)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;

	switch (calRet) {
	case e_SC_CAL_RESULT_SUCCESS:
		ret = e_SC_RESULT_SUCCESS;
		break;
	case e_SC_CAL_RESULT_CANCEL:
		ret = e_SC_RESULT_CANCEL;
		break;
	case e_SC_CAL_RESULT_FAIL:
		ret = e_SC_RESULT_FAIL;
		break;
	case e_SC_CAL_RESULT_MALLOC_ERR:
		ret = e_SC_RESULT_MALLOC_ERR;
		break;
	case e_SC_CAL_RESULT_BADPARAM:
		ret = e_SC_RESULT_BADPARAM;
		break;
	case e_SC_CAL_RESULT_FILE_ACCESSERR:
		ret = e_SC_RESULT_FILE_ACCESSERR;
		break;
	case e_SC_CAL_RESULT_TCP_CONNECT_ERROR:
		ret = e_SC_RESULT_TCP_CONNECT_ERROR;
		break;
	case e_SC_CAL_RESULT_TCP_COMMUNICATION_ERR:
		ret = e_SC_RESULT_TCP_COMMUNICATION_ERR;
		break;
	case e_SC_CAL_RESULT_TCP_TIMEOUT:
		ret = e_SC_RESULT_TCP_TIMEOUT;
		break;
	case e_SC_CAL_RESULT_TCP_DISCONNECTED:
		ret = e_SC_RESULT_TCP_DISCONNECTED;
		break;
	default:
		ret = e_SC_RESULT_FAIL;
		break;
	}

	return (ret);
}
