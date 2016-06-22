/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#include "SMUtilInternal.h"

#define SMZLB_INBUFSIZ	1024			// 入力バッファサイズ（任意）
#define SMZLB_OUTBUFSIZ	1024			// 出力バッファサイズ（任意）

//#define	SMZLB_LOG_ENABLE

/**
 * @brief gz圧縮
 * @param[in] inFilePath        圧縮するファイルパス
 * @param[in] outFilePath       圧縮後のファイルパス
 * @return 処理結果
 * 			SMZLB_OK:正常
 * 			SMZLB_ERROR:gz圧縮エラー
 * 			SMZLB_PARAM_ERROR:パラメータエラー
 * 			SMZLB_MALLOC_ERROR:メモリ確保エラー
 * 			SMZLB_FILE_IO_ERROR:ファイルI/Oエラー
 */
int SMZLB_GzCompress(char *inFilePath, char *outFilePath)
{
	int		ret = SMZLB_OK;
	char	*inBuf = NULL;
	char	*outBuf = NULL;
	z_stream	z = {};				// ライブラリとやりとりするための構造体
	FILE	*fin = NULL;				// 入力ファイル
	FILE	*fout = NULL;				// 出力ファイル
	int		len = 0;
	int		flush = 0;
	int		status = 0;

#ifdef	SMZLB_LOG_ENABLE
	__android_log_print(ANDROID_LOG_DEBUG, "SMZLB", "SMZLB_Compress START\n");
#endif

	do {
		// パラメータチェック
		if (NULL == inFilePath) {
#ifdef	SMZLB_LOG_ENABLE
			__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "param error[inFilePath]\n");
#endif
			ret = SMZLB_PARAM_ERROR;
			break;
		}
		if (NULL == outFilePath) {
#ifdef	SMZLB_LOG_ENABLE
			__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "param error[outFilePath]\n");
#endif
			ret = SMZLB_PARAM_ERROR;
			break;
		}

		// メモリ確保
		inBuf = (char*)malloc(SMZLB_INBUFSIZ);
		if (NULL == inBuf) {
#ifdef	SMZLB_LOG_ENABLE
			__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "inBuf malloc error\n");
#endif
			ret = SMZLB_MALLOC_ERROR;
			break;
		}
		outBuf = (char*)malloc(SMZLB_OUTBUFSIZ);
		if (NULL == outBuf) {
#ifdef	SMZLB_LOG_ENABLE
			__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "outBuf malloc error\n");
#endif
			ret = SMZLB_MALLOC_ERROR;
			break;
		}

		// ファイルオープン
		fin = fopen(inFilePath, "rb");
		if (NULL == fin) {
#ifdef	SMZLB_LOG_ENABLE
			__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "in fopen error[%d]\n", errno);
#endif
			ret = SMZLB_FILE_IO_ERROR;
			break;
		}
		fout = fopen(outFilePath, "wb");
		if (NULL == fout) {
#ifdef	SMZLB_LOG_ENABLE
			__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "out fopen error[%d]\n", errno);
#endif
			ret = SMZLB_FILE_IO_ERROR;
			break;
		}

		// すべてのメモリ管理をライブラリに任せる
		z.zalloc = Z_NULL;
		z.zfree = Z_NULL;
		z.opaque = Z_NULL;

		// 初期化
		if (Z_OK != deflateInit2(&z, Z_DEFAULT_COMPRESSION, Z_DEFLATED, (MAX_WBITS+16), 8, Z_DEFAULT_STRATEGY)) {
#ifdef	SMZLB_LOG_ENABLE
			__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "deflateInit2 error\n");
#endif
			ret = SMZLB_ERROR;
			break;
		}

		// 入力バッファ中のデータのバイト数
		z.avail_in = 0;
		// 出力ポインタ
		z.next_out = outBuf;
		// 出力バッファのサイズ
		z.avail_out = SMZLB_OUTBUFSIZ;

		// deflate() の第2引数は Z_NO_FLUSH にする
		flush = Z_NO_FLUSH;

		while (1) {
			if (0 == z.avail_in) {
				// 入力ポインタを入力バッファの先頭に
				z.next_in = inBuf;
				// ファイルリード
				z.avail_in = fread(inBuf, 1, SMZLB_INBUFSIZ, fin);
				if ((0 == z.avail_in) && (0 == feof(fin))) {
#ifdef	SMZLB_LOG_ENABLE
					__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "fread error[%d]\n", errno);
#endif
					ret = SMZLB_FILE_IO_ERROR;
					break;
				}

				// 入力が最後になったら deflate() の第2引数は Z_FINISH にする
				if (0 != feof(fin)) {
					flush = Z_FINISH;
				}
			}

			// 圧縮
			status = deflate(&z, flush);
			if (Z_STREAM_END == status) {
				// 完了
#ifdef	SMZLB_LOG_ENABLE
				__android_log_print(ANDROID_LOG_DEBUG, "SMZLB", "deflate finish\n");
#endif
				break;
			} else if (Z_OK != status) {
				// 圧縮エラー
#ifdef	SMZLB_LOG_ENABLE
				__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "deflate error[%d]\n", status);
#endif
				ret = SMZLB_ERROR;
				break;
			}

			if (0 == z.avail_out) {
				// まとめて書き出す
				if (SMZLB_OUTBUFSIZ != fwrite(outBuf, 1, SMZLB_OUTBUFSIZ, fout)) {
#ifdef	SMZLB_LOG_ENABLE
					__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "fwrite error[%d]\n", errno);
#endif
					ret = SMZLB_FILE_IO_ERROR;
					break;
				}
				// 出力バッファ残量を元に戻す
				z.next_out = outBuf;
				// 出力ポインタを元に戻す
				z.avail_out = SMZLB_OUTBUFSIZ;
			}
		}

		// 残りを吐き出す
		len = (SMZLB_OUTBUFSIZ - z.avail_out);
		if (0 != len) {
			if (len != fwrite(outBuf, 1, len, fout)) {
#ifdef	SMZLB_LOG_ENABLE
				__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "fwrite error2[%d]\n", errno);
#endif
				ret = SMZLB_FILE_IO_ERROR;
				break;
			}
		}

		// 後始末
		status = deflateEnd(&z);
		if (Z_OK != status) {
#ifdef	SMZLB_LOG_ENABLE
			__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "deflateEnd error[%d]\n", status);
#endif
			ret = SMZLB_ERROR;
			break;
		}
	} while (0);

	// ファイルクローズ
	if (NULL != fin) {
		fclose(fin);
	}
	if (NULL != fout) {
		fclose(fout);
	}

	// メモリ解放
	if (NULL != inBuf) {
		free(inBuf);
	}
	if (NULL != outBuf) {
		free(outBuf);
	}

#ifdef	SMZLB_LOG_ENABLE
	__android_log_print(ANDROID_LOG_DEBUG, "SMZLB", "SMZLB_Compress END\n");
#endif

	return (ret);
}

#if 0	// おまけ
/**
 * @brief gz解凍
 * @param[in] inFilePath        解凍するファイルパス
 * @param[in] outFilePath       解凍後のファイルパス
 * @return 処理結果
 * 			SMZLB_OK:正常
 * 			SMZLB_ERROR:gz圧縮エラー
 * 			SMZLB_PARAM_ERROR:パラメータエラー
 * 			SMZLB_MALLOC_ERROR:メモリ確保エラー
 * 			SMZLB_FILE_IO_ERROR:ファイルI/Oエラー
 */
int SMZLB_GzDeCompress(char *inFilePath, char *outFilePath)
{
	int		ret = 0;
	char	*inBuf = NULL;
	char	*outBuf = NULL;
	z_stream	z = {};				// ライブラリとやりとりするための構造体
	FILE	*fin = NULL;				// 入力ファイル
	FILE	*fout = NULL;				// 出力ファイル
	int		len = 0;
	int		flush = 0;
	int		status = 0;

#ifdef	SMZLB_LOG_ENABLE
	__android_log_print(ANDROID_LOG_DEBUG, "SMZLB", "SMZLB_DeCompress START\n");
#endif

	do {
		// パラメータチェック
		if (NULL == inFilePath) {
#ifdef	SMZLB_LOG_ENABLE
			__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "param error[inFilePath]\n");
#endif
			ret = SMZLB_PARAM_ERROR;
			break;
		}
		if (NULL == outFilePath) {
#ifdef	SMZLB_LOG_ENABLE
			__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "param error[outFilePath]\n");
#endif
			ret = SMZLB_PARAM_ERROR;
			break;
		}

		// メモリ確保
		inBuf = (char*)malloc(SMZLB_INBUFSIZ);
		if (NULL == inBuf) {
#ifdef	SMZLB_LOG_ENABLE
			__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "inBuf malloc error\n");
#endif
			ret = SMZLB_MALLOC_ERROR;
			break;
		}
		outBuf = (char*)malloc(SMZLB_OUTBUFSIZ);
		if (NULL == outBuf) {
#ifdef	SMZLB_LOG_ENABLE
			__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "outBuf malloc error\n");
#endif
			ret = SMZLB_MALLOC_ERROR;
			break;
		}

		// ファイルオープン
		fin = fopen(inFilePath, "rb");
		if (NULL == fin) {
#ifdef	SMZLB_LOG_ENABLE
			__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "in fopen error[%d]\n", errno);
#endif
			ret = SMZLB_FILE_IO_ERROR;
			break;
		}
		fout = fopen(outFilePath, "wb");
		if (NULL == fout) {
#ifdef	SMZLB_LOG_ENABLE
			__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "out fopen error[%d]\n", errno);
#endif
			ret = SMZLB_FILE_IO_ERROR;
			break;
		}

		// すべてのメモリ管理をライブラリに任せる
		z.zalloc = Z_NULL;
		z.zfree = Z_NULL;
		z.opaque = Z_NULL;

		/* 初期化 */
		z.next_in = Z_NULL;
		z.avail_in = 0;
		if (Z_OK != inflateInit2(&z, (MAX_WBITS+16))) {
#ifdef	SMZLB_LOG_ENABLE
			__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "inflateInit2 error\n");
#endif
			ret = SMZLB_ERROR;
			break;
		}

		// 出力ポインタ
		z.next_out = outBuf;
		// 出力バッファ残量
		z.avail_out = SMZLB_OUTBUFSIZ;
		status = Z_OK;

		while (Z_STREAM_END != status) {
			if (0 == z.avail_in) {
				// 入力ポインタを元に戻す
				z.next_in = inBuf;
				// データを読む
				z.avail_in = fread(inBuf, 1, SMZLB_INBUFSIZ, fin);
				if ((0 == z.avail_in) && (0 == feof(fin))) {
#ifdef	SMZLB_LOG_ENABLE
					__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "fread error[%d]\n", errno);
#endif
					ret = SMZLB_FILE_IO_ERROR;
					break;
				}
			}
			// 解凍
			status = inflate(&z, Z_NO_FLUSH);
			if (Z_STREAM_END == status) {
				// 完了
#ifdef	SMZLB_LOG_ENABLE
				__android_log_print(ANDROID_LOG_DEBUG, "SMZLB", "inflate finish\n");
#endif
				break;
			} else if (Z_OK != status) {
				// 解凍エラー
#ifdef	SMZLB_LOG_ENABLE
				__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "inflate error[%d]\n", status);
#endif
				ret = SMZLB_ERROR;
				break;
			}

			if (0 == z.avail_out) {
				// まとめて書き出す
				if (SMZLB_OUTBUFSIZ != fwrite(outBuf, 1, SMZLB_OUTBUFSIZ, fout)) {
#ifdef	SMZLB_LOG_ENABLE
					__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "fwrite error[%d]\n", errno);
#endif
					ret = SMZLB_FILE_IO_ERROR;
					break;
				}
				// 出力ポインタを元に戻す
				z.next_out = outBuf;
				// 出力バッファ残量を元に戻す
				z.avail_out = SMZLB_OUTBUFSIZ;
			}
		}

		// 残りを吐き出す
		len = (SMZLB_OUTBUFSIZ - z.avail_out);
		if (0 != len) {
			if (len != fwrite(outBuf, 1, len, fout)) {
#ifdef	SMZLB_LOG_ENABLE
				__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "fwrite error2[%d]\n", errno);
#endif
				ret = SMZLB_FILE_IO_ERROR;
				break;
			}
		}

		// 後始末
		status = inflateEnd(&z);
		if (Z_OK != status) {
#ifdef	SMZLB_LOG_ENABLE
			__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "inflateEnd error[%d]\n", status);
#endif
			ret = SMZLB_ERROR;
			break;
		}
	} while (0);

	// ファイルクローズ
	if (NULL != fin) {
		fclose(fin);
	}
	if (NULL != fout) {
		fclose(fout);
	}

	// メモリ解放
	if (NULL != inBuf) {
		free(inBuf);
	}
	if (NULL != outBuf) {
		free(outBuf);
	}

#ifdef	SMZLB_LOG_ENABLE
	__android_log_print(ANDROID_LOG_DEBUG, "SMZLB", "SMZLB_DeCompress END\n");
#endif

	return (ret);
}

/**
 * @brief zlib圧縮
 * @param[in] inFilePath        圧縮するファイルパス
 * @param[in] outFilePath       圧縮後のファイルパス
 * @return 処理結果
 * 			SMZLB_OK:正常
 * 			SMZLB_ERROR:gz圧縮エラー
 * 			SMZLB_PARAM_ERROR:パラメータエラー
 * 			SMZLB_MALLOC_ERROR:メモリ確保エラー
 * 			SMZLB_FILE_IO_ERROR:ファイルI/Oエラー
 */
int SMZLB_ZlibCompress(char *inFilePath, char *outFilePath)
{
	int		ret = 0;
	char	*inBuf = NULL;
	char	*outBuf = NULL;
	z_stream	z = {};				// ライブラリとやりとりするための構造体
	FILE	*fin = NULL;				// 入力ファイル
	FILE	*fout = NULL;				// 出力ファイル
	int		len = 0;
	int		flush = 0;
	int		status = 0;

#ifdef	SMZLB_LOG_ENABLE
	__android_log_print(ANDROID_LOG_DEBUG, "SMZLB", "SMZLB_ZlibCompress START\n");
#endif

	do {
		// パラメータチェック
		if (NULL == inFilePath) {
#ifdef	SMZLB_LOG_ENABLE
			__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "param error[inFilePath]\n");
#endif
			ret = SMZLB_PARAM_ERROR;
			break;
		}
		if (NULL == outFilePath) {
#ifdef	SMZLB_LOG_ENABLE
			__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "param error[outFilePath]\n");
#endif
			ret = SMZLB_PARAM_ERROR;
			break;
		}

		// メモリ確保
		inBuf = (char*)malloc(SMZLB_INBUFSIZ);
		if (NULL == inBuf) {
#ifdef	SMZLB_LOG_ENABLE
			__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "inBuf malloc error\n");
#endif
			ret = SMZLB_MALLOC_ERROR;
			break;
		}
		outBuf = (char*)malloc(SMZLB_OUTBUFSIZ);
		if (NULL == outBuf) {
#ifdef	SMZLB_LOG_ENABLE
			__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "outBuf malloc error\n");
#endif
			ret = SMZLB_MALLOC_ERROR;
			break;
		}

		// ファイルオープン
		fin = fopen(inFilePath, "rb");
		if (NULL == fin) {
#ifdef	SMZLB_LOG_ENABLE
			__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "in fopen error[%d]\n", errno);
#endif
			ret = SMZLB_FILE_IO_ERROR;
			break;
		}
		fout = fopen(outFilePath, "wb");
		if (NULL == fout) {
#ifdef	SMZLB_LOG_ENABLE
			__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "out fopen error[%d]\n", errno);
#endif
			ret = SMZLB_FILE_IO_ERROR;
			break;
		}

		// すべてのメモリ管理をライブラリに任せる
		z.zalloc = Z_NULL;
		z.zfree = Z_NULL;
		z.opaque = Z_NULL;

		// 初期化
		// 第2引数は圧縮の度合。0～9 の範囲の整数で，0 は無圧縮
		// Z_DEFAULT_COMPRESSION (= 6) が標準
		if (Z_OK != deflateInit(&z, Z_DEFAULT_COMPRESSION)) {
#ifdef	SMZLB_LOG_ENABLE
			__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "deflateInit error\n");
#endif
			ret = SMZLB_ERROR;
			break;
		}

		// 入力バッファ中のデータのバイト数
		z.avail_in = 0;
		// 出力ポインタ
		z.next_out = outBuf;
		// 出力バッファのサイズ
		z.avail_out = SMZLB_OUTBUFSIZ;

		// deflate() の第2引数は Z_NO_FLUSH にする
		flush = Z_NO_FLUSH;

		while (1) {
			if (0 == z.avail_in) {
				// 入力ポインタを入力バッファの先頭に
				z.next_in = inBuf;
				// ファイルリード
				z.avail_in = fread(inBuf, 1, SMZLB_INBUFSIZ, fin);
				if ((0 == z.avail_in) && (0 == feof(fin))) {
#ifdef	SMZLB_LOG_ENABLE
					__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "fread error[%d]\n", errno);
#endif
					ret = SMZLB_FILE_IO_ERROR;
					break;
				}

				// 入力が最後になったら deflate() の第2引数は Z_FINISH にする
				if (SMZLB_INBUFSIZ > z.avail_in) {
					flush = Z_FINISH;
				}
			}

			// 圧縮
			status = deflate(&z, flush);
			if (Z_STREAM_END == status) {
				// 完了
#ifdef	SMZLB_LOG_ENABLE
				__android_log_print(ANDROID_LOG_DEBUG, "SMZLB", "deflate finish\n");
#endif
				break;
			} else if (Z_OK != status) {
				// 圧縮エラー
#ifdef	SMZLB_LOG_ENABLE
				__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "deflate error[%d]\n", status);
#endif
				ret = SMZLB_ERROR;
				break;
			}

			if (0 == z.avail_out) {
				// まとめて書き出す
				if (SMZLB_OUTBUFSIZ != fwrite(outBuf, 1, SMZLB_OUTBUFSIZ, fout)) {
#ifdef	SMZLB_LOG_ENABLE
					__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "fwrite error[%d]\n", errno);
#endif
					ret = SMZLB_FILE_IO_ERROR;
					break;
				}
				// 出力バッファ残量を元に戻す
				z.next_out = outBuf;
				// 出力ポインタを元に戻す
				z.avail_out = SMZLB_OUTBUFSIZ;
			}
		}

		// 残りを吐き出す
		len = (SMZLB_OUTBUFSIZ - z.avail_out);
		if (0 != len) {
			if (len != fwrite(outBuf, 1, len, fout)) {
#ifdef	SMZLB_LOG_ENABLE
				__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "fwrite error2[%d]\n", errno);
#endif
				ret = SMZLB_FILE_IO_ERROR;
				break;
			}
		}

		// 後始末
		status = deflateEnd(&z);
		if (Z_OK != status) {
#ifdef	SMZLB_LOG_ENABLE
			__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "deflateEnd error[%d]\n", status);
#endif
			ret = SMZLB_ERROR;
			break;
		}
	} while (0);

	// ファイルクローズ
	if (NULL != fin) {
		fclose(fin);
	}
	if (NULL != fout) {
		fclose(fout);
	}

	// メモリ解放
	if (NULL != inBuf) {
		free(inBuf);
	}
	if (NULL != outBuf) {
		free(outBuf);
	}

#ifdef	SMZLB_LOG_ENABLE
	__android_log_print(ANDROID_LOG_DEBUG, "SMZLB", "SMZLB_ZlibCompress END\n");
#endif

	return (ret);
}
#endif	// おまけ

/**
 * @brief zlib解凍
 * @param[in] inFilePath        解凍するファイルパス
 * @param[in] outFilePath       解凍後のファイルパス
 * @return 処理結果
 * 			SMZLB_OK:正常
 * 			SMZLB_ERROR:gz圧縮エラー
 * 			SMZLB_PARAM_ERROR:パラメータエラー
 * 			SMZLB_MALLOC_ERROR:メモリ確保エラー
 * 			SMZLB_FILE_IO_ERROR:ファイルI/Oエラー
 */
int SMZLB_ZlibDeCompress(char *inFilePath, char *outFilePath)
{
	int		ret = 0;
	char	*inBuf = NULL;
	char	*outBuf = NULL;
	z_stream	z = {};				// ライブラリとやりとりするための構造体
	FILE	*fin = NULL;				// 入力ファイル
	FILE	*fout = NULL;				// 出力ファイル
	int		len = 0;
	int		flush = 0;
	int		status = 0;

#ifdef	SMZLB_LOG_ENABLE
	__android_log_print(ANDROID_LOG_DEBUG, "SMZLB", "SMZLB_ZlibDeCompress START\n");
#endif

	do {
		// パラメータチェック
		if (NULL == inFilePath) {
#ifdef	SMZLB_LOG_ENABLE
			__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "param error[inFilePath]\n");
#endif
			ret = SMZLB_PARAM_ERROR;
			break;
		}
		if (NULL == outFilePath) {
#ifdef	SMZLB_LOG_ENABLE
			__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "param error[outFilePath]\n");
#endif
			ret = SMZLB_PARAM_ERROR;
			break;
		}

		// メモリ確保
		inBuf = (char*)malloc(SMZLB_INBUFSIZ);
		if (NULL == inBuf) {
#ifdef	SMZLB_LOG_ENABLE
			__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "inBuf malloc error\n");
#endif
			ret = SMZLB_MALLOC_ERROR;
			break;
		}
		outBuf = (char*)malloc(SMZLB_OUTBUFSIZ);
		if (NULL == outBuf) {
#ifdef	SMZLB_LOG_ENABLE
			__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "outBuf malloc error\n");
#endif
			ret = SMZLB_MALLOC_ERROR;
			break;
		}

		// ファイルオープン
		fin = fopen(inFilePath, "rb");
		if (NULL == fin) {
#ifdef	SMZLB_LOG_ENABLE
			__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "in fopen error[%d]\n", errno);
#endif
			ret = SMZLB_FILE_IO_ERROR;
			break;
		}
		fout = fopen(outFilePath, "wb");
		if (NULL == fout) {
#ifdef	SMZLB_LOG_ENABLE
			__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "out fopen error[%d]\n", errno);
#endif
			ret = SMZLB_FILE_IO_ERROR;
			break;
		}

		// すべてのメモリ管理をライブラリに任せる
		z.zalloc = Z_NULL;
		z.zfree = Z_NULL;
		z.opaque = Z_NULL;

		/* 初期化 */
		z.next_in = Z_NULL;
		z.avail_in = 0;
		if (Z_OK != inflateInit(&z)) {
#ifdef	SMZLB_LOG_ENABLE
			__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "inflateInit error\n");
#endif
			ret = SMZLB_ERROR;
			break;
		}

		// 出力ポインタ
		z.next_out = outBuf;
		// 出力バッファ残量
		z.avail_out = SMZLB_OUTBUFSIZ;
		status = Z_OK;

		while (Z_STREAM_END != status) {
			if (0 == z.avail_in) {
				// 入力ポインタを元に戻す
				z.next_in = inBuf;
				// データを読む
				z.avail_in = fread(inBuf, 1, SMZLB_INBUFSIZ, fin);
				if ((0 == z.avail_in) && (0 == feof(fin))) {
#ifdef	SMZLB_LOG_ENABLE
					__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "fread error[%d]\n", errno);
#endif
					ret = SMZLB_FILE_IO_ERROR;
					break;
				}
			}
			// 解凍
			status = inflate(&z, Z_NO_FLUSH);
			if (Z_STREAM_END == status) {
				// 完了
#ifdef	SMZLB_LOG_ENABLE
				__android_log_print(ANDROID_LOG_DEBUG, "SMZLB", "inflate finish\n");
#endif
				break;
			} else if (Z_OK != status) {
				// 解凍エラー
#ifdef	SMZLB_LOG_ENABLE
				__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "inflate error[%d]\n", status);
#endif
				ret = SMZLB_ERROR;
				break;
			}

			if (0 == z.avail_out) {
				// まとめて書き出す
				if (SMZLB_OUTBUFSIZ != fwrite(outBuf, 1, SMZLB_OUTBUFSIZ, fout)) {
#ifdef	SMZLB_LOG_ENABLE
					__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "fwrite error[%d]\n", errno);
#endif
					ret = SMZLB_FILE_IO_ERROR;
					break;
				}
				// 出力ポインタを元に戻す
				z.next_out = outBuf;
				// 出力バッファ残量を元に戻す
				z.avail_out = SMZLB_OUTBUFSIZ;
			}
		}

		// 残りを吐き出す
		len = (SMZLB_OUTBUFSIZ - z.avail_out);
		if (0 != len) {
			if (len != fwrite(outBuf, 1, len, fout)) {
#ifdef	SMZLB_LOG_ENABLE
				__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "fwrite error2[%d]\n", errno);
#endif
				ret = SMZLB_FILE_IO_ERROR;
				break;
			}
		}

		// 後始末
		status = inflateEnd(&z);
		if (Z_OK != status) {
#ifdef	SMZLB_LOG_ENABLE
			__android_log_print(ANDROID_LOG_ERROR, "SMZLB", "inflateEnd error[%d]\n", status);
#endif
			ret = SMZLB_ERROR;
			break;
		}
	} while (0);

	// ファイルクローズ
	if (NULL != fin) {
		fclose(fin);
	}
	if (NULL != fout) {
		fclose(fout);
	}

	// メモリ解放
	if (NULL != inBuf) {
		free(inBuf);
	}
	if (NULL != outBuf) {
		free(outBuf);
	}

#ifdef	SMZLB_LOG_ENABLE
	__android_log_print(ANDROID_LOG_DEBUG, "SMZLB", "SMZLB_ZlibDeCompress END\n");
#endif

	return (ret);
}

