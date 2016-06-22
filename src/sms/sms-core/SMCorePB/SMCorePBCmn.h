/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SC_PB_CMN_H
#define SC_PB_CMN_H

extern Bool isProbeUpload;

#define	CC_ISPROBEUPLOAD()				(isProbeUpload)
#define	CC_SET_PROBEUPLOAD(isUpload)	(isProbeUpload = isUpload)

#endif // #ifndef SC_PB_CMN_H
