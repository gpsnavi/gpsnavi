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
 * SMCoreMPInternal.h
 *
 *  Created on: 2015/11/06
 *      Author: n.kanagawa
 */

#ifndef SMCOREMPINTERNAL_H_
#define SMCOREMPINTERNAL_H_

#define _COP_MP_DEBUG

#include "sms-core/internal/smsstd.h"
#include "sms-core/internal/smstypedef.h"
#include "sms-core/internal/smsutil.h"
#include "sms-core/internal/smscore.h"
#include "sms-core/internal/smscorecmn.h"

#include "sms-core/internal/smscoredhc.h"
#include "sms-core/internal/smscoredal.h"


#define _GLES_2
#define _GL_PTHREAD_SAFE

#ifdef _GLES_1
#include <GLES/gl.h>
#else
#include <GLES2/gl2.h>
#include "es1emu/MP_Es1Emulation.h"
#endif


#include "MP_Def.h"
#include "SMCoreMPApi.h"

#include "MP_Draw.h"
#include "MP_GL.h"
#include "MP_Param.h"
#include "MP_Png.h"
#include "MP_Icon.h"
#include "MP_Font.h"
#include "MP_Texture.h"
#include "MP_Debug.h"

#ifdef __cplusplus
#include "SMParcelBasisAnalyze.h"
#include "SMCommonAnalyzeData.h"
#include "SMRoadShapeAnalyze.h"
#include "SMBkgdAnalyze.h"
#include "SMNameAnalyze.h"
#include "SMMarkAnalyze.h"
#include "SMRoadNameAnalyze.h"
#include "MP_FontMng.h"
#include "MP_CollisionMesh.h"
#include "MP_DrawMap.h"
#endif


#endif /* SMCOREMPINTERNAL_H_ */
