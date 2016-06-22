/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#include "SMCoreCNFInternal.h"


#define SC_CONFIG_FILE_EXTENSION_OLD			".old"		// ファイル拡張子(.old)
#define SC_CONFIG_FILE_EXTENSION_NEW			".new"		// ファイル拡張子(.new)

static Char oldFilePath[SC_MAX_PATH];
static Char newFilePath[SC_MAX_PATH];

/**
 * @brief ファイルオープン
 * @param [in]  fileName    INIファイル名（フルパス）
 * @param [in]  mode        ファイルオープンモード("w" or "r")
 * @param [out] fp          ファイルポインタ
 * @return ファイルポインタ
 */
E_SC_RESULT SC_CONFIG_FileOpen(const Char *fileName, const Char *mode, FILE **fp)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	const Char	*filePath = NULL;
	struct stat	st = {};

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == fileName) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "param error[fileName], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == fp) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "param error[fp], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		*fp = NULL;

		if ('w' == mode[0]) {
			// 既存ファイルのリネーム後のファイル名設定
			sprintf((char*)oldFilePath, "%s%s", fileName, SC_CONFIG_FILE_EXTENSION_OLD);
			// 新規作成するファイル名設定
			sprintf((char*)newFilePath, "%s%s", fileName, SC_CONFIG_FILE_EXTENSION_NEW);

			// 指定されたファイル存在チェック
			if (0 == stat((char*)fileName, &st)) {
				// 指定されたファイルが存在する場合
				// 新規作成するファイル(拡張子「.new」)オープン
				filePath = &newFilePath[0];
			} else {
				// 指定されたファイルが存在しない場合
				// 拡張子「.old」ファイルをオープン(作成する)
				filePath = &oldFilePath[0];
			}
			chmod((char*)filePath, S_IWUSR);
		} else if ('r' == mode[0]) {
			// 既存ファイルのリネーム後のファイル名設定
			sprintf((char*)oldFilePath, "%s%s", fileName, SC_CONFIG_FILE_EXTENSION_OLD);
			// 新規作成するファイル名設定
			sprintf((char*)newFilePath, "%s%s", fileName, SC_CONFIG_FILE_EXTENSION_NEW);

			// 指定されたファイル存在チェック
			if (0 == stat((char*)fileName, &st)) {
				// 指定されたファイルが存在する場合
				// 指定されたファイル名＋拡張子「.new」ファイルを削除する
				chmod((char*)newFilePath, S_IWUSR);
				unlink((char*)newFilePath);
			} else {
				// 指定されたファイルが存在しない場合
				// 指定されたファイル名＋拡張子「.new」ファイルするかチェック
				if (0 == stat((char*)newFilePath, &st)) {
					// 指定されたファイル名＋拡張子「.new」ファイルが存在する
					// 指定されたファイル名＋拡張子「.new」ファイルを指定されたファイル名にリネーム
					if (-1 == rename((char*)newFilePath, (char*)fileName)) {
						SC_LOG_ErrorPrint(SC_TAG_DH, "file rename(.new=>) error(0x%08x), " HERE, errno);
						ret = e_SC_RESULT_FILE_ACCESSERR;
						break;
					}

					// 指定されたファイル名＋拡張子「.old」ファイルを削除
					chmod((char*)oldFilePath, S_IWUSR);
					if (-1 == unlink((char*)oldFilePath)) {
						SC_LOG_ErrorPrint(SC_TAG_DH, "file unlink(.old) error(0x%08x), " HERE, errno);
						ret = e_SC_RESULT_FILE_ACCESSERR;
						break;
					}
				} else {
					// 指定されたファイル名＋拡張子「.new」ファイルが存在しない場合
					// 指定されたファイル名＋拡張子「.old」ファイル削除（エラーは無視する）
					chmod((char*)oldFilePath, S_IWUSR);
					unlink((char*)oldFilePath);
					ret = e_SC_RESULT_NODATA;
					break;
				}
			}

			// 指定されたファイルをオープン
			filePath = fileName;
		} else {
			SC_LOG_ErrorPrint(SC_TAG_DH, "param error[mode], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 指定されたファイルをオープン
		*fp = fopen((char*)filePath, (char*)mode);
		if (NULL == *fp) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "file open error(0x%08x), " HERE, errno);
			ret = e_SC_RESULT_FILE_ACCESSERR;
			break;
		}
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}

/**
 * @brief ファイルクローズ
 * @param [in] fileName     INIファイル名（フルパス）
 * @param [in] mode         ファイルオープンモード
 * @param [in] fp           ファイルポインタ
 */
E_SC_RESULT SC_CONFIG_FileClose(const Char *fileName, const Char *mode, FILE *fp)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	struct stat	st = {};

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == fileName) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "param error[fileName], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == fp) {
			SC_LOG_ErrorPrint(SC_TAG_DH, "param error[fp], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// ファイルクローズ
		fclose(fp);

		if ('w' == mode[0]) {
			// 既存ファイルのリネーム後のファイル名設定
			sprintf((char*)oldFilePath, "%s%s", fileName, SC_CONFIG_FILE_EXTENSION_OLD);
			// 新規作成するファイル名設定
			sprintf((char*)newFilePath, "%s%s", fileName, SC_CONFIG_FILE_EXTENSION_NEW);

			// 指定されたファイル存在チェック
			if (0 == stat((char*)newFilePath, &st)) {
				// 既存のファイルをリネーム（拡張子「.old」を付加）
				if (-1 == rename((char*)fileName, (char*)oldFilePath)) {
					SC_LOG_ErrorPrint(SC_TAG_DH, "file rename(=>.old) error(0x%08x), " HERE, errno);
					ret = e_SC_RESULT_FILE_ACCESSERR;
					break;
				}

				// 新規作成したファイルをリネーム（拡張子「.new」を取る）
				if (-1 == rename((char*)newFilePath, (char*)fileName)) {
					SC_LOG_ErrorPrint(SC_TAG_DH, "file rename(.new=>) error(0x%08x), " HERE, errno);
					ret = e_SC_RESULT_FILE_ACCESSERR;
					break;
				}

				// 既存のファイル（拡張子「.old」ファイル削除）
				chmod((char*)oldFilePath, S_IWUSR);
				if (-1 == unlink((char*)oldFilePath)) {
					SC_LOG_ErrorPrint(SC_TAG_DH, "file unlink(.old) error(0x%08x), " HERE, errno);
					ret = e_SC_RESULT_FILE_ACCESSERR;
					break;
				}
			} else {
				// 拡張子「.old」を取ったファイル名にリネーム
				if (-1 == rename((char*)oldFilePath, (char*)fileName)) {
					SC_LOG_ErrorPrint(SC_TAG_DH, "file rename(.old=>) error(0x%08x), " HERE, errno);
					ret = e_SC_RESULT_FILE_ACCESSERR;
					break;
				}
			}
		}
		chmod((char*)fileName, S_IRUSR);
	} while (0);

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}

/**
 * @brief iniファイルを読み取り、キーの値を読み取るごとに引数の関数ポインタを呼び出して、読み取ったデータを通知する
 * @param [in] func     INIファイルの読み取り結果を通知するためのコールバック関数ポインタ
 * @param [in] fp       ファイルポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_CONFIG_GetIniFileValue(SC_CONFIG_INI_Func func, FILE *fp, void *config)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	Bool	isFoundSecName = false;					// セクション名が見つかったか
	Char	wk[512 + 1] ={};						// ワーク領域
	Char	secName[64 +1] ={};					// セクション名
	//Char	*keyName = NULL;						// キー名
	//Char	*value = NULL;							// 値
	Char	*chr = NULL;							// 文字列ポインタ
	const INT32	readSize = 512;						// ファイルリードサイズ

	// パラメータチェック
	if (NULL == func) {
		SC_LOG_ErrorPrint(SC_TAG_DH, (Char*)"param error[func], " HERE);
		return (e_SC_RESULT_BADPARAM);
	}
	if (NULL == fp) {
		SC_LOG_ErrorPrint(SC_TAG_DH, (Char*)"uninitialized, " HERE);
		return (e_SC_RESULT_FAIL);
	}

	while (1) {
		wk[0] = EOS;
		// 行単位リード
		if (NULL == fgets((char*)wk, readSize, fp)) {
			// EOF
			break;
		}
		if (';' == wk[0]) {
			// コメントは読み飛ばす
			continue;
		}
		// 改行コードをNULL終端文字に置換
		if ((1 <= strlen(wk)) &&
			(('\r' == wk[strlen((char*)wk) - 1]) || ('\n' == wk[strlen((char*)wk) - 1]))) {
			wk[strlen((char*)wk) - 1] = EOS;
		}
		if ((1 <= strlen(wk)) &&
			(('\r' == wk[strlen((char*)wk) - 1]) || ('\n' == wk[strlen((char*)wk) - 1]))) {
			wk[strlen((char*)wk) - 1] = EOS;
		}
		// 空白をトリム
		SC_Trim(wk);
		// セクション検索
		if ('[' == wk[0]) {
			// セクション名をコピー
			strcpy(secName, &wk[1]);
			chr = strchr(secName, ']');
			if (NULL != chr) {
				*chr = EOS;
			}
			// セクションの中を検索中に設定
			isFoundSecName = true;
			continue;
		}

		if (isFoundSecName) {
			if ('!' == wk[0]) {
				func(secName, wk, (Char*)"", config);
				continue;
			}

			// キー名検索
			//「=」の左側を取得
			chr = (Char*)strchr((char*)wk, '=');
			if (NULL != chr) {
				*chr = EOS;
				chr++;
				// 空白をトリム
				//SC_Trim(wk2);
				SC_Trim(wk);
				SC_Trim(chr);

				func(secName, wk, chr, config);
				ret = e_SC_RESULT_SUCCESS;
			}
		}
	}

	return (ret);
}

