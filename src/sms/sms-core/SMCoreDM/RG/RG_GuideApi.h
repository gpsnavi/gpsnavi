/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef RG_GUIDEAPI_H_
#define RG_GUIDEAPI_H_

// 誘導情報リスト数取得 外部ＩＦ
E_SC_RESULT RG_API_GetGuideDataVol(UINT16 *, UINT16);

// 誘導情報リスト取得 外部ＩＦ
E_SC_RESULT RG_API_GetGuideData(SMGUIDEDATA *, UINT16);

#endif /* RG_GUIDEAPI_H_ */
