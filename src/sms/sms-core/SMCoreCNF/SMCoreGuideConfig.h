/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCORE_GUIDE_CONFIG_H
#define SMCORE_GUIDE_CONFIG_H


//-----------------------------------
// 構造体定義
//-----------------------------------
typedef struct _SC_GUIDE_CONFIG {
#if 0
	// [BroadLength]
	struct {
		INT32	length;				// length
	} broadLength;
#endif
	// [GuideGraphicShowDist]
	struct {
		INT32	highdway;			// highdway
		INT32	normalway;			// normalway
	} guideGraphicShowDist;
	// [DynGraphicSize]
	struct {
		INT32	height;				// height
		INT32	width;				// width
	} dynGraphicSize;
	// [SignbroadShowDist]
	struct {
		INT32	distance;			// distance
	} signbroadShowDist;
	// [CarModeBoardFrequency]
	struct {
		INT32	size;				// size
		struct {
			INT32	selected;		// selected
		} list[21];
	} carModeBoardFrequency;
	// [CarModeGuideVoiceTrigger]
	struct {
		INT32	size;				// size
		struct {
			INT32	distance;		// distance
			INT32	advance;		// advance
			INT32	selected;		// selected
		} list[21];
	} carModeGuideVoiceTrigger;
	// [OnFootBoardFrequency]
	struct {
		INT32	size;				// size
		struct {
			INT32	selected;		// selected
		} list[21];
	} onFootBoardFrequency;
	// [OnFootGuideVoiceTrigger]
	struct {
		INT32	size;				// size
		struct {
			INT32	distance;		// distance
			INT32	advance;		// advance
			INT32	selected;		// selected
		} list[21];
	} onFootGuideVoiceTrigger;
	// [JoinpPoint]
	struct {
		INT32	size;				// size
		struct {
			INT32	distance;		// distance
			INT32	advance;		// advance
		} list[21];
	} joinpPoint;
	// [PlayInterval]
	struct {
		INT32	intertime;			// intertime
	} playInterval;
	// [TurnByTurnDis]
	struct {
		INT32	tbtDis;				// tbtDis
	} turnByTurnDis;
	// [ComplexGudPntDis]
	struct {
		INT32	hightwayDist;		// hightwayDist
		INT32	normalwayDist;		// normalwayDist
	} complexGudPntDis;
	// [SpeedLimited]
	struct {
		INT32	playInterval;		// PlayInterval
	} speedLimited;
	// [TrafficCamera]
	struct {
		INT32	minDisToShow;		// minDisToShow
		INT32	maxDisToShow;		// maxDisToShow
	} trafficCamera;
	// [CustomTrafficCameraBroadLowLevel]
	struct {
		INT32	size;				// size
		struct {
			INT32	distance;;		// distance
		} list[21];
	} customTrafficCameraBroadLowLevel;
	// [CustomTrafficCameraBroadNormalLevel]
	struct {
		INT32	size;				// size
		struct {
			INT32	distance;		// distance
		} list[21];
	} customTrafficCameraBroadNormalLevel;
	// [CustomTrafficCameraBroadHighLevel]
	struct {
		INT32	size;				// size
		struct {
			INT32	distance;		// distance
		} list[21];
	} customTrafficCameraBroadHighLevel;
	// [LinkSearchLength]
	struct {
		INT32	obverse;			// obverse
		INT32	reverse;			// reverse
	} linkSearchLength;
	// [WarningBroadNormalLevel]
	struct {
		INT32	size;				// size
		struct {
			INT32	distance;		// distance
			INT32	selected;		// selected
		} list[21];
	} warningBroadNormalLevel;
	// [HintOption]
	struct {
		INT32	hintDirection;		// HintDirection
	} hintOption;
} SC_GUIDE_CONFIG;

//-----------------------------------
// 外部I/F定義
//-----------------------------------
E_SC_RESULT SC_CONFIG_LoadGuideConfig(const Char *fileName, SC_GUIDE_CONFIG *config);
E_SC_RESULT SC_CONFIG_SaveGuideConfig(const Char *fileName, SC_GUIDE_CONFIG *config);

#endif // #ifndef SMCORE_GUIDE_CONFIG_H
