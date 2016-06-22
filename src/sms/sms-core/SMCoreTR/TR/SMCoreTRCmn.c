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
 * SMCoreTRCmn.c
 *
 *  Created on: 2016/01/04
 *      Author: n.kanagawa
 */
#include "sms-core/SMCoreTR/SMCoreTRInternal.h"


time_t SC_TR_NowUTC()
{
	time_t t;

	// 現在のUTC秒取得
	t = time(NULL);

#ifdef _TR_DEBUG
	SC_LOG_DebugPrint(SC_TAG_TR, (Char*)"SC_TR_NowUTC [%d]%s", t, ctime(&t));
#endif

	return (t);
}

struct tm *SC_TR_NowTM()
{
	time_t		t = 0;
	struct tm	*pTm = NULL;

	t = SC_TR_NowUTC();
	pTm = gmtime(&t);

	return (pTm);
}

void SC_TR_ChgTM(TR_BIN_TIME binTime, struct tm *pTm)
{
	pTm->tm_sec		= 1;								// 秒
	pTm->tm_min		= binTime.b.mm;						// 分
	pTm->tm_hour	= binTime.b.hh;						// 時間
	pTm->tm_mday	= binTime.b.day;					// 日
	pTm->tm_mon		= binTime.b.manth - 1;				// 月
	pTm->tm_year	= (binTime.b.year + 2000) - 1900;	// 年
	pTm->tm_wday	= 0;								// 曜日
	pTm->tm_yday	= 0;								// 年内通算日
	pTm->tm_isdst	= -1;								// 夏時間
}

Bool SC_TR_CompTM(const struct tm *a, const struct tm *b)
{
	Bool	ret = false;
	time_t	aTime = 0;
	time_t	bTime = 0;

	aTime = mktime((struct tm*)a);
	bTime = mktime((struct tm*)b);

	if (aTime > bTime) {
		ret = true;
	}

#ifdef _TR_DEBUG
	SC_LOG_DebugPrint(SC_TAG_TR, (Char*)"SC_TR_CompTM[%d] a[%d %d/%02d/%02d %02d:%02d], b[%d %d/%02d/%02d %02d:%02d]",
			ret,
			aTime, a->tm_year+1900, a->tm_mon+1, a->tm_mday, a->tm_hour, a->tm_min,
			bTime, b->tm_year+1900, b->tm_mon+1, b->tm_mday, b->tm_hour, b->tm_min);
#endif

	return (ret);
}
