/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#include "SMCComInternal.h"

//------------------------------------------------
// 変数定義
//------------------------------------------------

//------------------------------------------------
// 関数定義
//------------------------------------------------

/**
 * @brief バックアップデータを設定する
 * @param [IN] type         バックアップデータ種別
 * @param [IN] bkData       バックアップデータ
 * @param [IN] bkDataSize   バックアップデータサイズ
 * @param [IN] param        API関連パラメタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_SetBackupData(INT32 type, const Char *bkData, INT32 bkDataSize, T_CC_CMN_SMS_API_PRM *param)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	const Char	*data = bkData;
	INT32		len = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		if (0 == type) {
			len = sizeof(param->ApiPrmUser.login_id) +
				  sizeof(param->ApiPrmUser.password);
			if (len > bkDataSize) {
				SCC_LOG_ErrorPrint(SC_TAG_CC, (Char*)"CC_SetBackupData error " HERE);
				ret = e_SC_RESULT_BADPARAM;
				break;
			}

			// ログインID
			len = sizeof(param->ApiPrmUser.login_id);
			memcpy(param->ApiPrmUser.login_id, data, len);
			data += len;
			// パスワード
			len = sizeof(param->ApiPrmUser.password);
			memcpy(param->ApiPrmUser.password, data, len);
			data += len;
		}
	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}

/**
 * @brief バックアップデータを取得する
 * @param [IN] type         バックアップデータ種別
 * @param [OUT] bkData      バックアップデータ
 * @param [OUT] bkDataSize  バックアップデータサイズ
 * @param [OUT] param       API関連パラメタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_GetBackupData(INT32 type, Char *bkData, INT32 *bkDataSize, const T_CC_CMN_SMS_API_PRM *param)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	Char		*data = bkData;
	INT32		len = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	*bkDataSize = 0;

	do {
		if (0 == type) {
			// ログインID
			len = sizeof(param->ApiPrmUser.login_id);
			memcpy(data, param->ApiPrmUser.login_id, len);
			data += len;
			*bkDataSize += len;
			// パスワード
			len = sizeof(param->ApiPrmUser.password);
			memcpy(data, param->ApiPrmUser.password, len);
			data += len;
			*bkDataSize += len;
		}

		len = (*bkDataSize % 4);
		if (0 != len) {
			// 4バイト境界になるようにするため、0埋めする
			memset(data, 0, (4 - len));
			*bkDataSize += (4 - len);
		}
	} while (0);

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}
