/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMZLB_H_
#define SMZLB_H_

#define SMZLB_OK				0			// 正常
#define SMZLB_ERROR				-1			// エラー(gz圧縮/解凍)
#define SMZLB_PARAM_ERROR		-2			// パラメータエラー
#define SMZLB_MALLOC_ERROR		-4			// メモリ確保エラー
#define SMZLB_FILE_IO_ERROR		-5			// ファイルI/Oエラー

/* gz圧縮 */
int SMZLB_GzCompress(char *inFilePath, char *outFilePath);
#if 0	// おまけ
// gz解凍
int SMZLB_GzDeCompress(char *inFilePath, char *outFilePath);
// zlib圧縮
int SMZLB_ZlibCompress(char *inFilePath, char *outFilePath);
#endif
// zlib解凍
int SMZLB_ZlibDeCompress(char *inFilePath, char *outFilePath);


#endif /* SMZLB_H_ */

