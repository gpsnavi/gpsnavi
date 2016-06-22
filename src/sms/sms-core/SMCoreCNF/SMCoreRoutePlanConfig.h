/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCORE_ROUTE_PLAN_CONFIG_INI_H
#define SMCORE_ROUTE_PLAN_CONFIG_INI_H

#define	FREE_ROAD_SPEED_NUM				16
#define	TOLL_ROAD_SPEED_NUM				16
#define	TURN_NUM						30
#define	LEVEL_TURN_NUM					3
#define	ROAD_JAM_SPEED_NUM				3
#define	LANE_NUM						6
#define	LANE_PROCESS_NUM				2
#define	ROAD_CLASS_NUM					16
#define	CONDITION_NUM					10
#define	DISRANGE_NUM					10
#define	COUNTRANGE_NUM					10

//-----------------------------------
// 構造体定義
//-----------------------------------
typedef struct _SC_ROUTE_PLAN_CONFIG {
	// [Option]
	SMRPOPTION	option;
#if 0
	// [RoadAverageSpeed]
	struct {
		INT32	usingRoadManually;		// UsingRoadManually
		INT32	normalRoadSpeed;		// NormalRoadSpeed
		INT32	nationalRoadSpeed;		// NationalRoadSpeed
		INT32	freewayRoadSpeed;		// FreewayRoadSpeed
		INT32	autoNormalRoadSpeed;	// AutoNormalRoadSpeed
		INT32	autoNationalRoadSpeed;	// AutoNationalRoadSpeed
		INT32	autoFreewayRoadSpeed;	// AutoFreewayRoadSpeed
	} roadAverageSpeed;					// 未使用
#endif
	// [SpeedForTime]
	struct {
		INT32	freeRoadSpeed[FREE_ROAD_SPEED_NUM];	// free RoadSpeed
		INT32	tollRoadSpeed[TOLL_ROAD_SPEED_NUM];	// toll RoadSpeed
	} speedForTime;
	// [Turn]
	struct {
		INT32	turnNum;
		struct {
			INT32	id;						// ID
			INT32	left;					// Left
			INT32	straight;				// Straight
			INT32	right;					// Right
			INT32	uturn;					// UTurn
		} turn[TURN_NUM];
	} turn;
#if 0
	// [LevelTurn]
	struct {
		INT32	levelTurnNum;
		struct {
			INT32	level;					// Level
			INT32	national;				// National
			INT32	main;					// Main
			INT32	normal;					// Normal
		} levelTurn[LEVEL_TURN_NUM];
	} levelTurn;
#endif
	// [VICS]
	struct {
		INT32	vicsUsed;				// VICSUSED
	} vics;
	//[RoadJamSpeed]
	struct {
		INT32	roadType;				// RoadType
		INT32	jamSpeed;				// JamSpeed
		INT32	slowSpeed;				// SlowSpeed
		INT32	fastSpeed;				// FastSpeed
	} roadJamSpeed[ROAD_JAM_SPEED_NUM];
	// [LaneProcess]
	struct {
		FLOAT	lane[LANE_NUM];			// Lane
	} laneProcess[LANE_PROCESS_NUM];
	// [RoadParameters]
	struct {
		INT32	conditionSize;			// ConditionSize
		struct {
			INT32	condition;			// Condition
			FLOAT	roadClass[ROAD_CLASS_NUM];	// RoadClass
			FLOAT	toll;				// Toll
			INT32	distance;			// Distance
		} condition[CONDITION_NUM];
		INT32	disrangeSize;			// DisrangeSize
		struct {
			INT32	disrange;			// Disrange
			INT32	length;				// Length
			INT32	fparameter;			// FParameter
		} disrange[DISRANGE_NUM];
		INT32	countrangeSize;			// CountrangeSize
		struct {
			INT32	countrange;			// Countrange
			INT32	length;				// Length
			INT32	count;				// Count
			INT32	level;				// Level
		} countrange[COUNTRANGE_NUM];
		INT32	minCount;				// MinCount
		INT32	upCount;				// UpCount
		INT32	thinroadCount;			// ThinroadCount
	} roadParameters;
	// [RoadTypeParameter]
	struct {
		FLOAT	dummy;					// DUMMY
		FLOAT	notSeparable;			// NOT_SEPARABLE
		FLOAT	separable;				// SEPARABLE
		FLOAT	juction;				// JUCTION
		FLOAT	inner;					// INNER
		FLOAT	ramp;					// RAMP
		FLOAT	service;				// SERVICE
		FLOAT	sa;						// SA
	} roadTypeParameter;
	// [RoadFlagParameter]
	struct {
		INT32	tollwayExitCost;		// TollwayExitCost
		FLOAT	bridgeFactor;			// bridgeFactor
	} roadFlagParameter;
	// [PlanCount]
	struct {
		INT32	minMeetCount;			// MinMeetCount
		INT32	maxPlanningCount;		// MaxPlanningCount
	} planCount;
	// [Other]
	struct {
		INT32	detourFactor;			// DetourFactor
		INT32	detourDistance;			// DetourDistance
		INT32	replanDistance;			// ReplanDistance
		INT32	routeMaxCount;			// RouteMaxCount
		INT32	walkLimitDistance;		// WalkLimitDistance
		FLOAT	walkSpeed;				// WalkSpeed
		INT32	linkFoundDistance;		// LinkFoundDistance
		INT32	jamDistance;			// JamDistance
		INT32	shortLinkLength;		// ShortLinkLength
		FLOAT	highestLevel;			// HighestLevel
		INT32	carDirectionProcess;	// CarDirectionProcess
	} other;
	// [VICS]
	struct {
		INT32	replanInterval;			// ReplanInterval
	} replan;
	// [Remote]
	struct {
		Char	toll[SC_MAX_PATH];		// Toll
	} remote;
} SC_ROUTE_PLAN_CONFIG;

//-----------------------------------
// 外部I/F定義
//-----------------------------------
E_SC_RESULT SC_CONFIG_LoadRoutePlanConfig(const Char *fileName, SC_ROUTE_PLAN_CONFIG *config);
E_SC_RESULT SC_CONFIG_SaveRoutePlanConfig(const Char *fileName, SC_ROUTE_PLAN_CONFIG *config);

#endif // #ifndef SMCORE_ROUTE_PLAN_CONFIG_INI_H
