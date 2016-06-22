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

/**
 * @brief パッケージロック
 * @param [in]  pkgPath     パッケージ格納ディレクトリのフルパス
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_PkgLock(const Char *pkgPath)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	struct stat	st = {};

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// ディレクトリの有無チェック
		if (0 != stat(pkgPath, &st)) {
			// ディレクトリが存在しない場合は、正常終了する
			SCC_LOG_InfoPrint(SC_TAG_CC, "dir not found[%s](0x%08x), " HERE, pkgPath, errno);
			break;
		}

		// パッケージ削除
		ret = CC_DeleteDir(pkgPath);
		if (e_SC_RESULT_SUCCESS != ret) {
			SCC_LOG_ErrorPrint(SC_TAG_CC, "CC_DeleteDir error, " HERE);
			break;
		}
	} while (0);


	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}
