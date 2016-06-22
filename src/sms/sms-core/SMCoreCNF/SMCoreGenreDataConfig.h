/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCORE_GENRE_DATA_CONFIG_INI_H
#define SMCORE_GENRE_DATA_CONFIG_INI_H

#define FORMAT_LEN_MAX		32
#define DATE_LEN_MAX		32
#define GENRE_NAME_LEN_MAX	64

//-----------------------------------
// 構造体定義
//-----------------------------------
typedef struct _SC_GENRE_CONFIG {
	UINT32	code;									// ジャンルコード
	Char	name[GENRE_NAME_LEN_MAX];				// ジャンル名称

} SC_GENRE_CONFIG;

typedef struct _SC_GENRE_DATA_CONFIG {
	// [VERSION_INFO]
	struct {
		Char				format[FORMAT_LEN_MAX];	// FORMAT
		Char				date[DATE_LEN_MAX];		// DATE
	} versionInfo;

	// [GENRE1]
	struct {
		UINT32				num;					// NUM
		SC_GENRE_CONFIG		*rec;					// REC
	} genre;

} SC_GENRE_DATA_CONFIG;

//-----------------------------------
// 外部I/F定義
//-----------------------------------
E_SC_RESULT SC_CONFIG_LoadGenreDataConfig(const Char *fileName, SC_GENRE_DATA_CONFIG *config);
E_SC_RESULT SC_CONFIG_SaveGenreDataConfig(const Char *fileName, SC_GENRE_DATA_CONFIG *config);

#endif // #ifndef SMCORE_GENRE_DATA_CONFIG_INI_H
