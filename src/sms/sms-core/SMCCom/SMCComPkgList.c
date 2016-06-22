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
 * @brief パッケージ一覧取得（管理者モード）
 * @param [in]  dirPath     パッケージファイル格納ディレクトリのフルパス
 * @param [out] pkgInfo     パッケージ一覧
 * @param [out] pkgInfoNum  パッケージグループ数
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT CC_PkgList(const Char *dirPath, SMPACKAGEINFO *pkgInfo, INT32 *pkgInfoNum)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	DIR		*dir = NULL;
	struct dirent	*dent = NULL;
	//INT32	num = 0;

	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_START);

	do {
		// 初期化
		memset(pkgInfo, 0, sizeof(SMPACKAGEINFO));
		*pkgInfoNum = 0;

		// ディレクトリを開く
		dir = opendir((const char*)dirPath);
		if (NULL == dir) {
			if (ENOENT == errno) {
				// ディレクトリが存在しない場合は、正常終了する
				SCC_LOG_InfoPrint(SC_TAG_CC, "dir not found[%s], " HERE, dirPath);
			} else {
				SCC_LOG_ErrorPrint(SC_TAG_CC, "opendir error[%s] (0x%08x), " HERE, dirPath, errno);
				ret = e_SC_RESULT_FILE_ACCESSERR;
			}
			break;
		}

		// ディレクトリを読み込む
		while (NULL != (dent = readdir(dir))) {
			if (0 == strcmp(dent->d_name, ".") || 0 == strcmp(dent->d_name, "..")) {
				// 読み飛ばす
				continue;
			}

			if (DT_DIR == dent->d_type) {
				// ディレクトリ
				// ディレクトリのパス生成
				if ('/' == dirPath[strlen(dirPath) - 1]) {
					sprintf((char*)pkgInfo[*pkgInfoNum].packageFileName, "%s%s/", dirPath, dent->d_name);
				} else {
					sprintf((char*)pkgInfo[*pkgInfoNum].packageFileName, "%s/%s/", dirPath, dent->d_name);
				}
				(*pkgInfoNum)++;

				if (CC_CMN_PKGMGR_PACKAGE_MAXNUM <= *pkgInfoNum) {
					SCC_LOG_InfoPrint(SC_TAG_CC, "package list maxnum, " HERE);
					break;
				}
			}
		}

		// ディレクトリを閉じる
		closedir(dir);
	} while (0);


	SCC_LOG_DebugPrint(SC_TAG_CC, SCC_LOG_END);

	return (ret);
}
