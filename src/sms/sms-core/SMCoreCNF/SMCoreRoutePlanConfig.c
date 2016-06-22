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


#define	RT_PLN_SEC_OPTION					"Option"
#define	RT_PLN_KEY_RPCONDITION				"RPCondition"
#define	RT_PLN_KEY_APPENDCONDITION			"AppendCondition"
#define	RT_PLN_KEY_VEHICLETYPE				"VehicleType"
#define	RT_PLN_KEY_USINGREGULATIONTYPE		"UsingRegulationType"
#define	RT_PLN_KEY_TOLL_TYPE				"TollType"

#define	RT_PLN_SEC_ROADAVERAGESPEED			"RoadAverageSpeed"
#define	RT_PLN_KEY_USINGROADMANUALLY		"UsingRoadManually"
#define	RT_PLN_KEY_NORMALROADSPEED			"NormalRoadSpeed"
#define	RT_PLN_KEY_NATIONALROADSPEED		"NationalRoadSpeed"
#define	RT_PLN_KEY_FREEWAYROADSPEED			"FreewayRoadSpeed"
#define	RT_PLN_KEY_AUTONORMALROADSPEED		"AutoNormalRoadSpeed"
#define	RT_PLN_KEY_AUTONATIONALROADSPEED	"AutoNationalRoadSpeed"
#define	RT_PLN_KEY_AUTOFREEWAYROADSPEED		"AutoFreewayRoadSpeed"

#define	RT_PLN_SEC_SPEEDFORTIME				"SpeedForTime"
#define	RT_PLN_KEY_SFT_FREE					"!Free"
#define	RT_PLN_KEY_SFT_TOLL					"!Toll"
#define	RT_PLN_KEY_ROADSPEED				"RoadSpeed"

#define	RT_PLN_SEC_TURN						"Turn"
#define	RT_PLN_KEY_ID						"ID"
#define	RT_PLN_KEY_LEFT						"Left"
#define	RT_PLN_KEY_STRAIGHT					"Straight"
#define	RT_PLN_KEY_RIGHT					"Right"
#define	RT_PLN_KEY_UTURN					"UTurn"

#define	RT_PLN_SEC_LEVELTURN				"LevelTurn"
#define	RT_PLN_KEY_LEVEL					"Level"
#define	RT_PLN_KEY_NATIONAL					"National"
#define	RT_PLN_KEY_MAIN						"Main"
#define	RT_PLN_KEY_NORMAL					"Normal"

#define	RT_PLN_SEC_VICS						"VICS"
#define	RT_PLN_KEY_VICSUSED					"VICSUsed"

#define	RT_PLN_SEC_ROADJAMSPEED				"RoadJamSpeed"
#define	RT_PLN_KEY_ROADTYPE					"RoadType"
#define	RT_PLN_KEY_JAMSPEED					"JamSpeed"
#define	RT_PLN_KEY_SLOWSPEED				"SlowSpeed"
#define	RT_PLN_KEY_FASTSPEED				"FastSpeed"

#define	RT_PLN_SEC_LANEPROCESS				"LaneProcess"
#define	RT_PLN_KEY_LANE0					"Lane0"
#define	RT_PLN_KEY_LANE1					"Lane1"
#define	RT_PLN_KEY_LANE2					"Lane2"
#define	RT_PLN_KEY_LANE3					"Lane3"
#define	RT_PLN_KEY_LANE4					"Lane4"
#define	RT_PLN_KEY_LANE5					"Lane5"

#define	RT_PLN_SEC_ROADPARAMETERS			"RoadParameters"
#define	RT_PLN_KEY_CONDITIONSIZE			"ConditionSize"
#define	RT_PLN_KEY_CONDITION				"Condition"
#define	RT_PLN_KEY_ROADCLASS0				"RoadClass0"
#define	RT_PLN_KEY_ROADCLASS1				"RoadClass1"
#define	RT_PLN_KEY_ROADCLASS2				"RoadClass2"
#define	RT_PLN_KEY_ROADCLASS3				"RoadClass3"
#define	RT_PLN_KEY_ROADCLASS4				"RoadClass4"
#define	RT_PLN_KEY_ROADCLASS5				"RoadClass5"
#define	RT_PLN_KEY_ROADCLASS6				"RoadClass6"
#define	RT_PLN_KEY_ROADCLASS7				"RoadClass7"
#define	RT_PLN_KEY_ROADCLASS8				"RoadClass8"
#define	RT_PLN_KEY_ROADCLASS9				"RoadClass9"
#define	RT_PLN_KEY_ROADCLASS10				"RoadClass10"
#define	RT_PLN_KEY_ROADCLASS11				"RoadClass11"
#define	RT_PLN_KEY_ROADCLASS12				"RoadClass12"
#define	RT_PLN_KEY_ROADCLASS13				"RoadClass13"
#define	RT_PLN_KEY_ROADCLASS14				"RoadClass14"
#define	RT_PLN_KEY_ROADCLASS15				"RoadClass15"
#define	RT_PLN_KEY_TOLL						"Toll"
#define	RT_PLN_KEY_DISTANCE					"Distance"
#define	RT_PLN_KEY_DISRANGESIZE				"DisrangeSize"
#define	RT_PLN_KEY_DISRANGE					"Disrange"
#define	RT_PLN_KEY_LENGTH					"Length"
#define	RT_PLN_KEY_FPARAMETER				"FParameter"
#define	RT_PLN_KEY_COUNTRANGESIZE			"CountrangeSize"
#define	RT_PLN_KEY_COUNTRANGE				"Countrange"
#define	RT_PLN_KEY_COUNT					"Count"
#define	RT_PLN_KEY_MINCOUNT					"MinCount"
#define	RT_PLN_KEY_UPCOUNT					"UpCount"
#define	RT_PLN_KEY_THINROADCOUNT			"ThinroadCount"

#define	RT_PLN_SEC_ROADTYPEPARAMETER		"RoadTypeParameter"
#define	RT_PLN_KEY_DUMMY					"DUMMY"
#define	RT_PLN_KEY_NOT_SEPARABLE			"NOT_SEPARABLE"
#define	RT_PLN_KEY_SEPARABLE				"SEPARABLE"
#define	RT_PLN_KEY_JUCTION					"JUCTION"
#define	RT_PLN_KEY_INNER					"INNER"
#define	RT_PLN_KEY_RAMP						"RAMP"
#define	RT_PLN_KEY_SERVICE					"SERVICE"
#define	RT_PLN_KEY_SA						"SA"

#define	RT_PLN_SEC_ROADFLAGPARAMETER		"RoadFlagParameter"
#define	RT_PLN_KEY_TOLLWAYEXITCOST			"TollwayExitCost"
#define	RT_PLN_KEY_BRIDGEFACTOR				"BridgeFactor"

#define	RT_PLN_SEC_PLANCOUNT				"PlanCount"
#define	RT_PLN_KEY_MINMEETCOUNT				"MinMeetCount"
#define	RT_PLN_KEY_MAXPLANNINGCOUNT			"MaxPlanningCount"

#define	RT_PLN_SEC_OTHER					"Other"
#define	RT_PLN_KEY_DETOURFACTOR				"DetourFactor"
#define	RT_PLN_KEY_DETOURDISTANCE			"DetourDistance"
#define	RT_PLN_KEY_REPLANDISTANCE			"ReplanDistance"
#define	RT_PLN_KEY_ROUTEMAXCOUNT			"RouteMaxCount"
#define	RT_PLN_KEY_WALKLIMITDISTANCE		"WalkLimitDistance"
#define	RT_PLN_KEY_WALKSPEED				"WalkSpeed"
#define	RT_PLN_KEY_LINKFOUNDDISTANCE		"LinkFoundDistance"
#define	RT_PLN_KEY_JAMDISTANCE				"JamDistance"
#define	RT_PLN_KEY_SHORTLINKLENGTH			"ShortLinkLength"
#define	RT_PLN_KEY_HIGHESTLEVEL				"HighestLevel"
#define	RT_PLN_KEY_CARDIRECTIONPROCESS		"CarDirectionProcess"

#define	RT_PLN_KEY_REPLANINTERVAL			"ReplanInterval"

#define	RT_PLN_SEC_REMOTE					"Remote"

//static FILE		*fp;
static INT32	speedForTimeType;
static INT32	freeRoadSpeedCnt;
static INT32	tollRoadSpeedCnt;
static INT32	turnCnt;
static INT32	roadJamSpeedCnt;
static INT32	laneProcessCnt;
static Char		keyType[16];
static INT32	conditionCnt;
static INT32	disrangeCnt;
static INT32	countrangeCnt;

static void SC_CONFIG_CallBackSetValue(const Char *secName, const Char *keyName, const Char *value, void *config);
void SC_CONFIG_SetValueOption(const Char *secName, const Char *keyName, const Char *value, SC_ROUTE_PLAN_CONFIG *configData);
#if 0
void SC_CONFIG_SetValueRoadAverageSpeed(const Char *secName, const Char *keyName, const Char *value, SC_ROUTE_PLAN_CONFIG *configData);
#endif
void SC_CONFIG_SetValueSpeedForTime(const Char *secName, const Char *keyName, const Char *value, SC_ROUTE_PLAN_CONFIG *configData);
void SC_CONFIG_SetValueTurn(const Char *secName, const Char *keyName, const Char *value, SC_ROUTE_PLAN_CONFIG *configData);
#if 0
void SC_CONFIG_SetValueLevelTurn(const Char *secName, const Char *keyName, const Char *value, SC_ROUTE_PLAN_CONFIG *configData);
#endif
void SC_CONFIG_SetValueVICS(const Char *secName, const Char *keyName, const Char *value, SC_ROUTE_PLAN_CONFIG *configData);
void SC_CONFIG_SetValueRoadJamSpeed(const Char *secName, const Char *keyName, const Char *value, SC_ROUTE_PLAN_CONFIG *configData);
void SC_CONFIG_SetValueLaneProcess(const Char *secName, const Char *keyName, const Char *value, SC_ROUTE_PLAN_CONFIG *configData);
void SC_CONFIG_SetValueRoadParameters(const Char *secName, const Char *keyName, const Char *value, SC_ROUTE_PLAN_CONFIG *configData);
void SC_CONFIG_SetValueRoadTypeParameter(const Char *secName, const Char *keyName, const Char *value, SC_ROUTE_PLAN_CONFIG *configData);
void SC_CONFIG_SetValueRoadFlagParameter(const Char *secName, const Char *keyName, const Char *value, SC_ROUTE_PLAN_CONFIG *configData);
void SC_CONFIG_SetValuePlanCount(const Char *secName, const Char *keyName, const Char *value, SC_ROUTE_PLAN_CONFIG *configData);
void SC_CONFIG_SetValueOther(const Char *secName, const Char *keyName, const Char *value, SC_ROUTE_PLAN_CONFIG *configData);
void SC_CONFIG_SetValueRemote(const Char *secName, const Char *keyName, const Char *value, SC_ROUTE_PLAN_CONFIG *configData);


/**
 * @brief  RoutePlanConfig.iniファイルから値を読み込む
 * @param[in] fileName  RoutePlanConfig.iniファイルのフルパス
 * @param[in] config    RoutePlanConfig.iniファイルから取得した値を格納する構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_CONFIG_LoadRoutePlanConfig(const Char *fileName, SC_ROUTE_PLAN_CONFIG *config)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SC_CONFIG_INI_Func	func = SC_CONFIG_CallBackSetValue;
	FILE	*fp = NULL;

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == fileName) {
			SC_LOG_ErrorPrint(SC_TAG_DH, (Char*)"param error[fileName], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == config) {
			SC_LOG_ErrorPrint(SC_TAG_DH, (Char*)"param error[config], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// 変数初期化
		memset(config, 0, sizeof(SC_ROUTE_PLAN_CONFIG));
		speedForTimeType = -1;
		freeRoadSpeedCnt = 0;
		tollRoadSpeedCnt = 0;
		turnCnt = 0;
		roadJamSpeedCnt = 0;
		laneProcessCnt = 0;
		memset(keyType, 0, sizeof(keyType));
		conditionCnt = 0;
		disrangeCnt = 0;
		countrangeCnt = 0;

		// INIファイルオープン
		ret = SC_CONFIG_FileOpen(fileName, (Char*)"r", &fp);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_DH, (Char*)"SC_CONFIG_FileOpen error, " HERE);
			break;
		}

		// INIファイルから設定値取得
		ret = SC_CONFIG_GetIniFileValue(func, fp, config);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_DH, (Char*)"SC_CONFIG_GetIniFileValue error, " HERE);
			break;
		}
	} while (0);

	if (fp != NULL) {
		// INIファイルクローズ
		SC_CONFIG_FileClose(fileName, (Char*)"r", fp);
		fp = NULL;
	}

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}

/**
 * @brief  RoutePlanConfig.iniファイルに値を書き込む
 * @param[in] fileName  RoutePlanConfig.iniファイルのフルパス
 * @param[in] config    RoutePlanConfig.iniファイルから取得した値を格納する構造体のポインタ
 * @return 処理結果(E_SC_RESULT)
 */
E_SC_RESULT SC_CONFIG_SaveRoutePlanConfig(const Char *fileName, SC_ROUTE_PLAN_CONFIG *config)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	FILE	*fp = NULL;
	INT32	num = 0;

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == fileName) {
			SC_LOG_ErrorPrint(SC_TAG_DH, (Char*)"param error[fileName], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}
		if (NULL == config) {
			SC_LOG_ErrorPrint(SC_TAG_DH, (Char*)"param error[config], " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// iniファイルオープン
		ret = SC_CONFIG_FileOpen(fileName, (Char*)"w", &fp);
		if (ret != e_SC_RESULT_SUCCESS) {
			SC_LOG_ErrorPrint(SC_TAG_DH, (Char*)"SC_CONFIG_FileOpen error, " HERE);
			break;
		}

		// [Option]
		fprintf(fp, "[%s]\n", RT_PLN_SEC_OPTION);
		// RPCondition
		fprintf(fp, "%s=%d\n", RT_PLN_KEY_RPCONDITION, config->option.rpCond);
		// AppendCondition
		fprintf(fp, "%s=%ld\n", RT_PLN_KEY_APPENDCONDITION, config->option.appendCond);
		// VehicleType
		fprintf(fp, "%s=%d\n", RT_PLN_KEY_VEHICLETYPE, config->option.vehicleType);
		// UsingRegulationType
		fprintf(fp, "%s=%d\n", RT_PLN_KEY_USINGREGULATIONTYPE, config->option.regulationType);
		// TollType
		fprintf(fp, "%s=%d\n", RT_PLN_KEY_TOLL_TYPE, config->option.tollType);
		fprintf(fp, "\n");

#if 0
		// [RoadAverageSpeed]
		fprintf(fp, "[%s]\n", RT_PLN_SEC_ROADAVERAGESPEED);
		// UsingRoadManually
		fprintf(fp, "%s=%d\n", RT_PLN_KEY_USINGROADMANUALLY, config->roadAverageSpeed.usingRoadManually);
		// NormalRoadSpeed
		fprintf(fp, "%s=%d\n", RT_PLN_KEY_NORMALROADSPEED, config->roadAverageSpeed.normalRoadSpeed);
		// NationalRoadSpeed
		fprintf(fp, "%s=%d\n", RT_PLN_KEY_NATIONALROADSPEED, config->roadAverageSpeed.nationalRoadSpeed);
		// FreewayRoadSpeed
		fprintf(fp, "%s=%d\n", RT_PLN_KEY_FREEWAYROADSPEED, config->roadAverageSpeed.freewayRoadSpeed);
		// AutoNormalRoadSpeed
		fprintf(fp, "%s=%d\n", RT_PLN_KEY_AUTONORMALROADSPEED, config->roadAverageSpeed.autoNormalRoadSpeed);
		// AutoNationalRoadSpeed
		fprintf(fp, "%s=%d\n", RT_PLN_KEY_AUTONATIONALROADSPEED, config->roadAverageSpeed.autoNationalRoadSpeed);
		// AutoFreewayRoadSpeed
		fprintf(fp, "%s=%d\n", RT_PLN_KEY_AUTOFREEWAYROADSPEED, config->roadAverageSpeed.autoFreewayRoadSpeed);
		fprintf(fp, "\n");
#endif

		// [SpeedForTime]
		fprintf(fp, "[%s]\n", RT_PLN_SEC_SPEEDFORTIME);
		// !Free
		fprintf(fp, "%s\n", RT_PLN_KEY_SFT_FREE);
		for (num = 0; num < FREE_ROAD_SPEED_NUM; num++) {
			// RoadSpeed
			fprintf(fp, "%s=%d\n", RT_PLN_KEY_ROADSPEED, config->speedForTime.freeRoadSpeed[num]);
		}
		// !Toll
		fprintf(fp, "%s\n", RT_PLN_KEY_SFT_TOLL);
		for (num = 0; num < TOLL_ROAD_SPEED_NUM; num++) {
			// RoadSpeed
			fprintf(fp, "%s=%d\n", RT_PLN_KEY_ROADSPEED, config->speedForTime.tollRoadSpeed[num]);
		}
		fprintf(fp, "\n");

		// [Turn]
		fprintf(fp, "[%s]\n", RT_PLN_SEC_TURN);
		for (num = 0; num < config->turn.turnNum; num++) {
			// ID
			fprintf(fp, "%s=%d\n", RT_PLN_KEY_ID, config->turn.turn[num].id);
			// Left
			fprintf(fp, "%s=%d\n", RT_PLN_KEY_LEFT, config->turn.turn[num].left);
			// Straight
			fprintf(fp, "%s=%d\n", RT_PLN_KEY_STRAIGHT, config->turn.turn[num].straight);
			// Right
			fprintf(fp, "%s=%d\n", RT_PLN_KEY_RIGHT, config->turn.turn[num].right);
			// UTurn
			fprintf(fp, "%s=%d\n", RT_PLN_KEY_UTURN, config->turn.turn[num].uturn);
			fprintf(fp, "\n");
		}

#if 0
		// [LevelTurn]
		fprintf(fp, "[%s]\n", RT_PLN_SEC_LEVELTURN);
		for (num = 0; num < config->levelTurn.levelTurnNum; num++) {
			// Level
			fprintf(fp, "%s=%d\n", RT_PLN_KEY_LEVEL, config->levelTurn.levelTurn[num].level);
			// National
			fprintf(fp, "%s=%d\n", RT_PLN_KEY_NATIONAL, config->levelTurn.levelTurn[num].national);
			// Main
			fprintf(fp, "%s=%d\n", RT_PLN_KEY_MAIN, config->levelTurn.levelTurn[num].main);
			// Normal
			fprintf(fp, "%s=%d\n", RT_PLN_KEY_NORMAL, config->levelTurn.levelTurn[num].normal);
			fprintf(fp, "\n");
		}
#endif

		// [VICS]
		fprintf(fp, "[%s]\n", RT_PLN_SEC_VICS);
		// VICSUsed
		fprintf(fp, "%s=%d\n", RT_PLN_KEY_VICSUSED, config->vics.vicsUsed);
		fprintf(fp, "\n");

		// [RoadJamSpeed]
		fprintf(fp, "[%s]\n", RT_PLN_SEC_ROADJAMSPEED);
		for (num = 0; num < ROAD_JAM_SPEED_NUM; num++) {
			// RoadType
			fprintf(fp, "%s=%d\n", RT_PLN_KEY_ROADTYPE, config->roadJamSpeed[num].roadType);
			// JamSpeed
			fprintf(fp, "%s=%d\n", RT_PLN_KEY_JAMSPEED, config->roadJamSpeed[num].jamSpeed);
			// SlowSpeed
			fprintf(fp, "%s=%d\n", RT_PLN_KEY_SLOWSPEED, config->roadJamSpeed[num].slowSpeed);
			// FastSpeed
			fprintf(fp, "%s=%d\n", RT_PLN_KEY_FASTSPEED, config->roadJamSpeed[num].fastSpeed);
			fprintf(fp, "\n");
		}

		// [LaneProcess]
		fprintf(fp, "[%s]\n", RT_PLN_SEC_LANEPROCESS);
		for (num = 0; num < LANE_PROCESS_NUM; num++) {
			// Lane0
			fprintf(fp, "%s=%.03f\n", RT_PLN_KEY_LANE0, config->laneProcess[num].lane[0]);
			// Lane1
			fprintf(fp, "%s=%.03f\n", RT_PLN_KEY_LANE1, config->laneProcess[num].lane[1]);
			// Lane2
			fprintf(fp, "%s=%.03f\n", RT_PLN_KEY_LANE2, config->laneProcess[num].lane[2]);
			// Lane3
			fprintf(fp, "%s=%.03f\n", RT_PLN_KEY_LANE3, config->laneProcess[num].lane[3]);
			// Lane4
			fprintf(fp, "%s=%.03f\n", RT_PLN_KEY_LANE4, config->laneProcess[num].lane[4]);
			// Lane5
			fprintf(fp, "%s=%.03f\n", RT_PLN_KEY_LANE5, config->laneProcess[num].lane[5]);
			fprintf(fp, "\n");
		}

		// [RoadParameters]
		fprintf(fp, "[%s]\n", RT_PLN_SEC_ROADPARAMETERS);
		// ConditionSize
		fprintf(fp, "%s=%d\n", RT_PLN_KEY_CONDITIONSIZE, config->roadParameters.conditionSize);
		fprintf(fp, "\n");
		for (num = 0; num < config->roadParameters.conditionSize; num++) {
			// Condition
			fprintf(fp, "%s=%d\n", RT_PLN_KEY_CONDITION, config->roadParameters.condition[num].condition);
			// RoadClass0
			fprintf(fp, "%s=%.01f\n", RT_PLN_KEY_ROADCLASS0, config->roadParameters.condition[num].roadClass[0]);
			// RoadClass1
			fprintf(fp, "%s=%.01f\n", RT_PLN_KEY_ROADCLASS1, config->roadParameters.condition[num].roadClass[1]);
			// RoadClass2
			fprintf(fp, "%s=%.01f\n", RT_PLN_KEY_ROADCLASS2, config->roadParameters.condition[num].roadClass[2]);
			// RoadClass3
			fprintf(fp, "%s=%.01f\n", RT_PLN_KEY_ROADCLASS3, config->roadParameters.condition[num].roadClass[3]);
			// RoadClass4
			fprintf(fp, "%s=%.01f\n", RT_PLN_KEY_ROADCLASS4, config->roadParameters.condition[num].roadClass[4]);
			// RoadClass5
			fprintf(fp, "%s=%.01f\n", RT_PLN_KEY_ROADCLASS5, config->roadParameters.condition[num].roadClass[5]);
			// RoadClass6
			fprintf(fp, "%s=%.01f\n", RT_PLN_KEY_ROADCLASS6, config->roadParameters.condition[num].roadClass[6]);
			// RoadClass7
			fprintf(fp, "%s=%.01f\n", RT_PLN_KEY_ROADCLASS7, config->roadParameters.condition[num].roadClass[7]);
			// RoadClass8
			fprintf(fp, "%s=%.01f\n", RT_PLN_KEY_ROADCLASS8, config->roadParameters.condition[num].roadClass[8]);
			// RoadClass9
			fprintf(fp, "%s=%.01f\n", RT_PLN_KEY_ROADCLASS9, config->roadParameters.condition[num].roadClass[9]);
			// RoadClass10
			fprintf(fp, "%s=%.01f\n", RT_PLN_KEY_ROADCLASS10, config->roadParameters.condition[num].roadClass[10]);
			// RoadClass11
			fprintf(fp, "%s=%.01f\n", RT_PLN_KEY_ROADCLASS11, config->roadParameters.condition[num].roadClass[11]);
			// RoadClass12
			fprintf(fp, "%s=%.01f\n", RT_PLN_KEY_ROADCLASS12, config->roadParameters.condition[num].roadClass[12]);
			// RoadClass13
			fprintf(fp, "%s=%.01f\n", RT_PLN_KEY_ROADCLASS13, config->roadParameters.condition[num].roadClass[13]);
			// RoadClass14
			fprintf(fp, "%s=%.01f\n", RT_PLN_KEY_ROADCLASS14, config->roadParameters.condition[num].roadClass[14]);
			// RoadClass15
			fprintf(fp, "%s=%.01f\n", RT_PLN_KEY_ROADCLASS15, config->roadParameters.condition[num].roadClass[15]);
			// Toll
			fprintf(fp, "%s=%.01f\n", RT_PLN_KEY_TOLL, config->roadParameters.condition[num].toll);
			// Distance
			fprintf(fp, "%s=%d\n", RT_PLN_KEY_DISTANCE, config->roadParameters.condition[num].distance);
			fprintf(fp, "\n");
		}

		// DisrangeSize
		fprintf(fp, "%s=%d\n", RT_PLN_KEY_DISRANGESIZE, config->roadParameters.disrangeSize);
		fprintf(fp, "\n");
		for (num = 0; num < config->roadParameters.disrangeSize; num++) {
			// Disrange
			fprintf(fp, "%s=%d\n", RT_PLN_KEY_DISRANGE, config->roadParameters.disrange[num].disrange);
			// Length
			fprintf(fp, "%s=%d\n", RT_PLN_KEY_LENGTH, config->roadParameters.disrange[num].length);
			// FParameter
			fprintf(fp, "%s=%d\n", RT_PLN_KEY_FPARAMETER, config->roadParameters.disrange[num].fparameter);
			fprintf(fp, "\n");
		}

		// CountrangeSize
		fprintf(fp, "%s=%d\n", RT_PLN_KEY_COUNTRANGESIZE, config->roadParameters.countrangeSize);
		fprintf(fp, "\n");
		for (num = 0; num < config->roadParameters.countrangeSize; num++) {
			// Countrange
			fprintf(fp, "%s=%d\n", RT_PLN_KEY_COUNTRANGE, config->roadParameters.countrange[num].countrange);
			// Length
			fprintf(fp, "%s=%d\n", RT_PLN_KEY_LENGTH, config->roadParameters.countrange[num].length);
			// Count
			fprintf(fp, "%s=%d\n", RT_PLN_KEY_COUNT, config->roadParameters.countrange[num].count);
			// Level
			fprintf(fp, "%s=%d\n", RT_PLN_KEY_LEVEL, config->roadParameters.countrange[num].level);
			fprintf(fp, "\n");
		}

		// MinCount
		fprintf(fp, "%s=%d\n", RT_PLN_KEY_MINCOUNT, config->roadParameters.minCount);
		// UpCount
		fprintf(fp, "%s=%d\n", RT_PLN_KEY_UPCOUNT, config->roadParameters.upCount);
		// ThinroadCount
		fprintf(fp, "%s=%d\n", RT_PLN_KEY_THINROADCOUNT, config->roadParameters.thinroadCount);
		fprintf(fp, "\n");

		// [RoadTypeParameter]
		fprintf(fp, "[%s]\n", RT_PLN_SEC_ROADTYPEPARAMETER);
		// DUMMY
		fprintf(fp, "%s=%0.1f\n", RT_PLN_KEY_DUMMY, config->roadTypeParameter.dummy);
		// NOT_SEPARABLE
		fprintf(fp, "%s=%0.1f\n", RT_PLN_KEY_NOT_SEPARABLE, config->roadTypeParameter.notSeparable);
		// SEPARABLE
		fprintf(fp, "%s=%0.1f\n", RT_PLN_KEY_SEPARABLE, config->roadTypeParameter.separable);
		// JUCTION
		fprintf(fp, "%s=%0.1f\n", RT_PLN_KEY_JUCTION, config->roadTypeParameter.juction);
		// INNER
		fprintf(fp, "%s=%0.1f\n", RT_PLN_KEY_INNER, config->roadTypeParameter.inner);
		// RAMP
		fprintf(fp, "%s=%0.2f\n", RT_PLN_KEY_RAMP, config->roadTypeParameter.ramp);
		// SERVICE
		fprintf(fp, "%s=%0.2f\n", RT_PLN_KEY_SERVICE, config->roadTypeParameter.service);
		// SA
		fprintf(fp, "%s=%0.2f\n", RT_PLN_KEY_SA, config->roadTypeParameter.sa);
		fprintf(fp, "\n");

		// [RoadFlagParameter]
		fprintf(fp, "[%s]\n", RT_PLN_SEC_ROADFLAGPARAMETER);
		// TollwayExitCost
		fprintf(fp, "%s=%d\n", RT_PLN_KEY_TOLLWAYEXITCOST, config->roadFlagParameter.tollwayExitCost);
		// BridgeFactor
		fprintf(fp, "%s=%.02f\n", RT_PLN_KEY_BRIDGEFACTOR, config->roadFlagParameter.bridgeFactor);
		fprintf(fp, "\n");

		// [PlanCount]
		fprintf(fp, "[%s]\n", RT_PLN_SEC_PLANCOUNT);
		// MinMeetCount
		fprintf(fp, "%s=%d\n", RT_PLN_KEY_MINMEETCOUNT, config->planCount.minMeetCount);
		// MaxPlanningCount
		fprintf(fp, "%s=%d\n", RT_PLN_KEY_MAXPLANNINGCOUNT, config->planCount.maxPlanningCount);
		fprintf(fp, "\n");

		// [Other]
		fprintf(fp, "[%s]\n", RT_PLN_SEC_OTHER);
		// DetourFactor
		fprintf(fp, "%s=%d\n", RT_PLN_KEY_DETOURFACTOR, config->other.detourFactor);
		// DetourDistance
		fprintf(fp, "%s=%d\n", RT_PLN_KEY_DETOURDISTANCE, config->other.detourDistance);
		// ReplanDistance
		fprintf(fp, "%s=%d\n", RT_PLN_KEY_REPLANDISTANCE, config->other.replanDistance);
		// RouteMaxCount
		fprintf(fp, "%s=%d\n", RT_PLN_KEY_ROUTEMAXCOUNT, config->other.routeMaxCount);
		// WalkLimitDistance
		fprintf(fp, "%s=%d\n", RT_PLN_KEY_WALKLIMITDISTANCE, config->other.walkLimitDistance);
		// WalkSpeed
		fprintf(fp, "%s=%.01f\n", RT_PLN_KEY_WALKSPEED, config->other.walkSpeed);
		// LinkFoundDistance
		fprintf(fp, "%s=%d\n", RT_PLN_KEY_LINKFOUNDDISTANCE, config->other.linkFoundDistance);
		// JamDistance
		fprintf(fp, "%s=%d\n", RT_PLN_KEY_JAMDISTANCE, config->other.jamDistance);
		// ShortLinkLength
		fprintf(fp, "%s=%d\n", RT_PLN_KEY_SHORTLINKLENGTH, config->other.shortLinkLength);
		// HighestLevel
		fprintf(fp, "%s=%.02f\n", RT_PLN_KEY_HIGHESTLEVEL, config->other.highestLevel);
		// CarDirectionProcess
		fprintf(fp, "%s=%d\n", RT_PLN_KEY_CARDIRECTIONPROCESS, config->other.carDirectionProcess);
		fprintf(fp, "\n");

		// [VICS]
		fprintf(fp, "[%s]\n", RT_PLN_SEC_VICS);
		// ReplanInterval
		fprintf(fp, "%s=%d\n", RT_PLN_KEY_REPLANINTERVAL, config->replan.replanInterval);
		fprintf(fp, "\n");

		// [Remote]
		fprintf(fp, "[%s]\n", RT_PLN_SEC_REMOTE);
		// Toll
		fprintf(fp, "%s=%s\n", RT_PLN_KEY_TOLL, config->remote.toll);
		fprintf(fp, "\n");
	} while (0);

	if (NULL != fp) {
		// INIファイルクローズ
		SC_CONFIG_FileClose(fileName, (Char*)"w", fp);
		fp = NULL;
	}

	SC_LOG_DebugPrint(SC_TAG_DH, SC_LOG_END);

	return (ret);
}


/**
 * @brief  INIファイルから読み込んだ値を設定するコールバック関数
 * @param[in] secName   INIファイルから読み込んだセクション名
 * @param[in] keyName   INIファイルから読み込んだキー名
 * @param[in] value     INIファイルから読み込んだ値
 */
void SC_CONFIG_CallBackSetValue(const Char *secName, const Char *keyName, const Char *value, void *config)
{
	SC_ROUTE_PLAN_CONFIG	*configData = NULL;

	// パラメータチェック
	if (NULL == secName) {
		SC_LOG_ErrorPrint(SC_TAG_DH, (Char*)"param error[secName], " HERE);
		return;
	}
	if (NULL == keyName) {
		SC_LOG_ErrorPrint(SC_TAG_DH, (Char*)"param error[keyName], " HERE);
		return;
	}
	if (NULL == value) {
		SC_LOG_ErrorPrint(SC_TAG_DH, (Char*)"param error[value], " HERE);
		return;
	}
	if (NULL == config) {
		SC_LOG_ErrorPrint(SC_TAG_DH, (Char*)"param error[config], " HERE);
		return;
	}
	configData = (SC_ROUTE_PLAN_CONFIG*)config;

	// [Option]
	if (0 == strcmp((char*)secName, RT_PLN_SEC_OPTION)) {
		SC_CONFIG_SetValueOption(secName, keyName, value, configData);
	}
#if 0
	// [RoadAverageSpeed]
	else if (0 == strcmp((char*)secName, RT_PLN_SEC_ROADAVERAGESPEED)) {
		SC_CONFIG_SetValueRoadAverageSpeed(secName, keyName, value, configData);
	}
#endif
	// [SpeedForTime]
	else if (0 == strcmp((char*)secName, RT_PLN_SEC_SPEEDFORTIME)) {
		SC_CONFIG_SetValueSpeedForTime(secName, keyName, value, configData);
	}
	// [Turn]
	else if (0 == strcmp((char*)secName, RT_PLN_SEC_TURN)) {
		SC_CONFIG_SetValueTurn(secName, keyName, value, configData);
	}
#if 0
	// [LevelTurn]
	else if (0 == strcmp((char*)secName, RT_PLN_SEC_LEVELTURN)) {
		SC_CONFIG_SetValueLevelTurn(secName, keyName, value, configData);
	}
#endif
	// [VICS]
	else if (0 == strcmp((char*)secName, RT_PLN_SEC_VICS)) {
		SC_CONFIG_SetValueVICS(secName, keyName, value, configData);
	}
	// [RoadJamSpeed]
	else if (0 == strcmp((char*)secName, RT_PLN_SEC_ROADJAMSPEED)) {
		SC_CONFIG_SetValueRoadJamSpeed(secName, keyName, value, configData);
	}
	// [LaneProcess]
	else if (0 == strcmp((char*)secName, RT_PLN_SEC_LANEPROCESS)) {
		SC_CONFIG_SetValueLaneProcess(secName, keyName, value, configData);
	}
	// [RoadParameters]
	else if (0 == strcmp((char*)secName, RT_PLN_SEC_ROADPARAMETERS)) {
		SC_CONFIG_SetValueRoadParameters(secName, keyName, value, configData);
	}
	// [RoadTypeParameter]
	else if (0 == strcmp((char*)secName, RT_PLN_SEC_ROADTYPEPARAMETER)) {
		SC_CONFIG_SetValueRoadTypeParameter(secName, keyName, value, configData);
	}
	// [RoadFlagParameter]
	else if (0 == strcmp((char*)secName, RT_PLN_SEC_ROADFLAGPARAMETER)) {
		SC_CONFIG_SetValueRoadFlagParameter(secName, keyName, value, configData);
	}
	// [PlanCount]
	else if (0 == strcmp((char*)secName, RT_PLN_SEC_PLANCOUNT)) {
		SC_CONFIG_SetValuePlanCount(secName, keyName, value, configData);
	}
	// [Other]
	else if (0 == strcmp((char*)secName, RT_PLN_SEC_OTHER)) {
		SC_CONFIG_SetValueOther(secName, keyName, value, configData);
	}
	// [Remote]
	else if (0 == strcmp((char*)secName, RT_PLN_SEC_REMOTE)) {
		SC_CONFIG_SetValueRemote(secName, keyName, value, configData);
	}
}

/**
 * @brief  INIファイルから読み込んだ値を設定する
 * @param[in] secName   INIファイルから読み込んだセクション名
 * @param[in] keyName   INIファイルから読み込んだキー名
 * @param[in] value     INIファイルから読み込んだ値
 */
void SC_CONFIG_SetValueOption(const Char *secName, const Char *keyName, const Char *value, SC_ROUTE_PLAN_CONFIG *configData)
{
	// RPCondition
	if (0 == strcmp((char*)keyName, RT_PLN_KEY_RPCONDITION)) {
		// value
		configData->option.rpCond = atoi((char*)value);
	}
	// AppendCondition
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_APPENDCONDITION)) {
		// value
		configData->option.appendCond = atol((char*)value);
	}
	// VehicleType
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_VEHICLETYPE)) {
		// value
		configData->option.vehicleType = atoi((char*)value);
	}
	// UsingRegulationType
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_USINGREGULATIONTYPE)) {
		// value
		configData->option.vehicleType = atoi((char*)value);
	}
	// TollType
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_TOLL_TYPE)) {
		// value
		configData->option.tollType = atoi((char*)value);
	}
}

#if 0
/**
 * @brief  INIファイルから読み込んだ値を設定する
 * @param[in] secName   INIファイルから読み込んだセクション名
 * @param[in] keyName   INIファイルから読み込んだキー名
 * @param[in] value     INIファイルから読み込んだ値
 */
void SC_CONFIG_SetValueRoadAverageSpeed(const Char *secName, const Char *keyName, const Char *value, SC_ROUTE_PLAN_CONFIG *configData)
{
	// UsingRoadManually
	if (0 == strcmp((char*)keyName, RT_PLN_KEY_USINGROADMANUALLY)) {
		// value
		configData->roadAverageSpeed.usingRoadManually = atoi((char*)value);
	}
	// NormalRoadSpeed
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_NORMALROADSPEED)) {
		// value
		configData->roadAverageSpeed.normalRoadSpeed = atoi((char*)value);
	}
	// NationalRoadSpeed
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_NATIONALROADSPEED)) {
		// value
		configData->roadAverageSpeed.nationalRoadSpeed = atoi((char*)value);
	}
	// FreewayRoadSpeed
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_FREEWAYROADSPEED)) {
		// value
		configData->roadAverageSpeed.freewayRoadSpeed = atoi((char*)value);
	}
	// AutoNormalRoadSpeed
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_AUTONORMALROADSPEED)) {
		// value
		configData->roadAverageSpeed.autoNormalRoadSpeed = atoi((char*)value);
	}
	// AutoNationalRoadSpeed
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_AUTONATIONALROADSPEED)) {
		// value
		configData->roadAverageSpeed.autoNationalRoadSpeed = atoi((char*)value);
	}
	// AutoFreewayRoadSpeed
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_AUTOFREEWAYROADSPEED)) {
		// value
		configData->roadAverageSpeed.autoFreewayRoadSpeed = atoi((char*)value);
	}
}
#endif

/**
 * @brief  INIファイルから読み込んだ値を設定する
 * @param[in] secName   INIファイルから読み込んだセクション名
 * @param[in] keyName   INIファイルから読み込んだキー名
 * @param[in] value     INIファイルから読み込んだ値
 */
void SC_CONFIG_SetValueSpeedForTime(const Char *secName, const Char *keyName, const Char *value, SC_ROUTE_PLAN_CONFIG *configData)
{
	if (0 == strcmp((char*)keyName, RT_PLN_KEY_SFT_FREE)) {
		speedForTimeType = 1;
	}
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_SFT_TOLL)) {
		speedForTimeType = 2;
	}
	// RoadSpeed
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_ROADSPEED)) {
		// Free
		if ((1 == speedForTimeType) && (FREE_ROAD_SPEED_NUM > freeRoadSpeedCnt)) {
			// value
			configData->speedForTime.freeRoadSpeed[freeRoadSpeedCnt] = atoi((char*)value);
			freeRoadSpeedCnt++;
		}
		// Toll
		else if ((2 == speedForTimeType) && (TOLL_ROAD_SPEED_NUM > tollRoadSpeedCnt)) {
			// value
			configData->speedForTime.tollRoadSpeed[tollRoadSpeedCnt] = atoi((char*)value);
			tollRoadSpeedCnt++;
		}
	}
}

/**
 * @brief  INIファイルから読み込んだ値を設定する
 * @param[in] secName   INIファイルから読み込んだセクション名
 * @param[in] keyName   INIファイルから読み込んだキー名
 * @param[in] value     INIファイルから読み込んだ値
 */
void SC_CONFIG_SetValueTurn(const Char *secName, const Char *keyName, const Char *value, SC_ROUTE_PLAN_CONFIG *configData)
{
	// ID
	if (0 == strcmp((char*)keyName, "ID")) {
		if (TURN_NUM > configData->turn.turnNum) {
			// value
			configData->turn.turn[configData->turn.turnNum].id = atoi((char*)value);
			configData->turn.turnNum++;
		} else {
			SC_LOG_WarnPrint(SC_TAG_DH, (Char*)"[Turn] value maxnum over, " HERE);
		}
	}
	// Left
	else if (0 == strcmp((char*)keyName, "Left")) {
		if (0 < configData->turn.turnNum) {
			// value
			configData->turn.turn[configData->turn.turnNum - 1].left = atoi((char*)value);
		}
	}
	// Straight
	else if (0 == strcmp((char*)keyName, "Straight")) {
		if (0 < configData->turn.turnNum) {
			// value
			configData->turn.turn[configData->turn.turnNum - 1].straight = atoi((char*)value);
		}
	}
	// Right
	else if (0 == strcmp((char*)keyName, "Right")) {
		if (0 < configData->turn.turnNum) {
			// value
			configData->turn.turn[configData->turn.turnNum - 1].right = atoi((char*)value);
		}
	}
	// UTurn
	else if (0 == strcmp((char*)keyName, "UTurn")) {
		if (0 < configData->turn.turnNum) {
			// value
			configData->turn.turn[configData->turn.turnNum - 1].uturn = atoi((char*)value);
		}
	}
}

#if 0
/**
 * @brief  INIファイルから読み込んだ値を設定する
 * @param[in] secName   INIファイルから読み込んだセクション名
 * @param[in] keyName   INIファイルから読み込んだキー名
 * @param[in] value     INIファイルから読み込んだ値
 */
void SC_CONFIG_SetValueLevelTurn(const Char *secName, const Char *keyName, const Char *value, SC_ROUTE_PLAN_CONFIG *configData)
{
	// Level
	if (0 == strcmp((char*)keyName, RT_PLN_KEY_LEVEL)) {
		if (LEVEL_TURN_NUM > configData->levelTurn.levelTurnNum) {
			// value
			configData->levelTurn.levelTurn[configData->levelTurn.levelTurnNum].level = atoi((char*)value);
			configData->levelTurn.levelTurnNum++;
		} else {
			SC_LOG_WarnPrint(SC_TAG_DH, (Char*)"[LevelTurn] value maxnum over, " HERE);
		}
	}
	// National
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_NATIONAL)) {
		if (0 < configData->levelTurn.levelTurnNum) {
			// value
			configData->levelTurn.levelTurn[configData->levelTurn.levelTurnNum - 1].national = atoi((char*)value);
		}
	}
	// Main
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_MAIN)) {
		if (0 < configData->levelTurn.levelTurnNum) {
			// value
			configData->levelTurn.levelTurn[configData->levelTurn.levelTurnNum - 1].main = atoi((char*)value);
		}
	}
	// Normal
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_NORMAL)) {
		if (0 < configData->levelTurn.levelTurnNum) {
			// value
			configData->levelTurn.levelTurn[configData->levelTurn.levelTurnNum - 1].normal = atoi((char*)value);
		}
	}
}
#endif

/**
 * @brief  INIファイルから読み込んだ値を設定する
 * @param[in] secName   INIファイルから読み込んだセクション名
 * @param[in] keyName   INIファイルから読み込んだキー名
 * @param[in] value     INIファイルから読み込んだ値
 */
void SC_CONFIG_SetValueVICS(const Char *secName, const Char *keyName, const Char *value, SC_ROUTE_PLAN_CONFIG *configData)
{
	// VICSUSED
	if (0 == strcmp((char*)keyName, RT_PLN_KEY_VICSUSED)) {
		// value
		configData->vics.vicsUsed = atoi((char*)value);
	}
	// ReplanInterval
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_REPLANINTERVAL)) {
		// value
		configData->replan.replanInterval = atoi((char*)value);
	}
}

/**
 * @brief  INIファイルから読み込んだ値を設定する
 * @param[in] secName   INIファイルから読み込んだセクション名
 * @param[in] keyName   INIファイルから読み込んだキー名
 * @param[in] value     INIファイルから読み込んだ値
 */
void SC_CONFIG_SetValueRoadJamSpeed(const Char *secName, const Char *keyName, const Char *value, SC_ROUTE_PLAN_CONFIG *configData)
{
	// RoadType
	if (0 == strcmp((char*)keyName, RT_PLN_KEY_ROADTYPE)) {
		if ((0 <= roadJamSpeedCnt) && (ROAD_JAM_SPEED_NUM > roadJamSpeedCnt)) {
			// value
			configData->roadJamSpeed[roadJamSpeedCnt].roadType = atoi((char*)value);
			roadJamSpeedCnt++;
		} else {
			SC_LOG_WarnPrint(SC_TAG_DH, (Char*)"[RoadJamSpeed] RoadType=%s error, " HERE, value);
			roadJamSpeedCnt = -1;
		}
	}
	// JamSpeed
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_JAMSPEED)) {
		if (0 < roadJamSpeedCnt) {
			// value
			configData->roadJamSpeed[roadJamSpeedCnt - 1].jamSpeed = atoi((char*)value);
		}
	}
	// SlowSpeed
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_SLOWSPEED)) {
		if (0 < roadJamSpeedCnt) {
			// value
			configData->roadJamSpeed[roadJamSpeedCnt - 1].slowSpeed = atoi((char*)value);
		}
	}
	// FastSpeed
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_FASTSPEED)) {
		if (0 < roadJamSpeedCnt) {
			// value
			configData->roadJamSpeed[roadJamSpeedCnt - 1].fastSpeed = atoi((char*)value);
		}
	}
}

/**
 * @brief  INIファイルから読み込んだ値を設定する
 * @param[in] secName   INIファイルから読み込んだセクション名
 * @param[in] keyName   INIファイルから読み込んだキー名
 * @param[in] value     INIファイルから読み込んだ値
 */
void SC_CONFIG_SetValueLaneProcess(const Char *secName, const Char *keyName, const Char *value, SC_ROUTE_PLAN_CONFIG *configData)
{
	// Lane0
	if (0 == strcmp((char*)keyName, RT_PLN_KEY_LANE0)) {
		if ((0 <= laneProcessCnt) && (LANE_PROCESS_NUM > laneProcessCnt)) {
			// value
			configData->laneProcess[laneProcessCnt].lane[0] = atof((char*)value);
			laneProcessCnt++;
		} else {
			laneProcessCnt = -1;
		}
	}
	// Lane1
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_LANE1)) {
		if (0 < laneProcessCnt) {
			// value
			configData->laneProcess[laneProcessCnt - 1].lane[1] = atof((char*)value);
		}
	}
	// Lane2
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_LANE2)) {
		if (0 < laneProcessCnt) {
			// value
			configData->laneProcess[laneProcessCnt - 1].lane[2] = atof((char*)value);
		}
	}
	// Lane3
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_LANE3)) {
		if (0 < laneProcessCnt) {
			// value
			configData->laneProcess[laneProcessCnt - 1].lane[3] = atof((char*)value);
		}
	}
	// Lane4
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_LANE4)) {
		if (0 < laneProcessCnt) {
			// value
			configData->laneProcess[laneProcessCnt - 1].lane[4] = atof((char*)value);
		}
	}
	// Lane5
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_LANE5)) {
		if (0 < laneProcessCnt) {
			// value
			configData->laneProcess[laneProcessCnt - 1].lane[5] = atof((char*)value);
		}
	}
}

/**
 * @brief  INIファイルから読み込んだ値を設定する
 * @param[in] secName   INIファイルから読み込んだセクション名
 * @param[in] keyName   INIファイルから読み込んだキー名
 * @param[in] value     INIファイルから読み込んだ値
 */
void SC_CONFIG_SetValueRoadParameters(const Char *secName, const Char *keyName, const Char *value, SC_ROUTE_PLAN_CONFIG *configData)
{
	// ConditionSize
	if (0 == strcmp((char*)keyName, RT_PLN_KEY_CONDITIONSIZE)) {
		INT32	conditionSize = 0;
		strcpy((char*)keyType, RT_PLN_KEY_CONDITION);
		// value
		conditionSize = atoi((char*)value);
		if ((0 < conditionSize) && (CONDITION_NUM >= conditionSize)) {
			configData->roadParameters.conditionSize = conditionSize;
		} else {
			SC_LOG_WarnPrint(SC_TAG_DH, (Char*)"[RoadParameters] ConditionSize=%s error, " HERE, value);
			if (CONDITION_NUM < conditionSize) {
				configData->roadParameters.conditionSize = CONDITION_NUM;
			} else {
				configData->roadParameters.conditionSize = 0;
			}
		}
	}
	// DisrangeSize
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_DISRANGESIZE)) {
		INT32	disrangeSize = 0;
		strcpy((char*)keyType, RT_PLN_KEY_DISRANGE);
		// value
		disrangeSize = atoi((char*)value);
		if ((0 < disrangeSize) && (DISRANGE_NUM >= disrangeSize)) {
			configData->roadParameters.disrangeSize = disrangeSize;
		} else {
			SC_LOG_WarnPrint(SC_TAG_DH, (Char*)"[RoadParameters] DisrangeSize=%s error, " HERE, value);
			if (DISRANGE_NUM < disrangeSize) {
				configData->roadParameters.disrangeSize = DISRANGE_NUM;
			} else {
				configData->roadParameters.disrangeSize = 0;
			}
		}
	}
	// CountrangeSize
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_COUNTRANGESIZE)) {
		INT32	countrangeSize = 0;
		strcpy((char*)keyType, RT_PLN_KEY_COUNTRANGE);
		// value
		countrangeSize = atoi((char*)value);
		if ((0 < countrangeSize) && (COUNTRANGE_NUM >= countrangeSize)) {
			configData->roadParameters.countrangeSize = countrangeSize;
		} else {
			SC_LOG_WarnPrint(SC_TAG_DH, (Char*)"[RoadParameters] CountrangeSize=%s error, " HERE, value);
			if (COUNTRANGE_NUM < countrangeSize) {
				configData->roadParameters.countrangeSize = COUNTRANGE_NUM;
			} else {
				configData->roadParameters.countrangeSize = 0;
			}
		}
	}
	else if (0 == strcmp((char*)keyType, RT_PLN_KEY_CONDITION)) {
		// Condition
		if (0 == strcmp((char*)keyName, RT_PLN_KEY_CONDITION)) {
			// value
			if ((0 <= conditionCnt) && ((configData->roadParameters.conditionSize) > conditionCnt)) {
				configData->roadParameters.condition[conditionCnt].condition = atoi((char*)value);
				conditionCnt++;
			} else {
				SC_LOG_WarnPrint(SC_TAG_DH, (Char*)"[RoadParameters] Condition=%s error, " HERE, value);
				conditionCnt = -1;
			}
		}
		// RoadClass0
		else if (0 == strcmp((char*)keyName, RT_PLN_KEY_ROADCLASS0)) {
			if (0 < conditionCnt) {
				// value
				configData->roadParameters.condition[conditionCnt - 1].roadClass[0] = atof((char*)value);
			}
		}
		// RoadClass1
		else if (0 == strcmp((char*)keyName, RT_PLN_KEY_ROADCLASS1)) {
			if (0 < conditionCnt) {
				// value
				configData->roadParameters.condition[conditionCnt - 1].roadClass[1] = atof((char*)value);
			}
		}
		// RoadClass2
		else if (0 == strcmp((char*)keyName, RT_PLN_KEY_ROADCLASS2)) {
			if (0 < conditionCnt) {
				// value
				configData->roadParameters.condition[conditionCnt - 1].roadClass[2] = atof((char*)value);
			}
		}
		// RoadClass3
		else if (0 == strcmp((char*)keyName, RT_PLN_KEY_ROADCLASS3)) {
			if (0 < conditionCnt) {
				// value
				configData->roadParameters.condition[conditionCnt - 1].roadClass[3] = atof((char*)value);
			}
		}
		// RoadClass4
		else if (0 == strcmp((char*)keyName, RT_PLN_KEY_ROADCLASS4)) {
			if (0 < conditionCnt) {
				// value
				configData->roadParameters.condition[conditionCnt - 1].roadClass[4] = atof((char*)value);
			}
		}
		// RoadClass5
		else if (0 == strcmp((char*)keyName, RT_PLN_KEY_ROADCLASS5)) {
			if (0 < conditionCnt) {
				// value
				configData->roadParameters.condition[conditionCnt - 1].roadClass[5] = atof((char*)value);
			}
		}
		// RoadClass6
		else if (0 == strcmp((char*)keyName, RT_PLN_KEY_ROADCLASS6)) {
			if (0 < conditionCnt) {
				// value
				configData->roadParameters.condition[conditionCnt - 1].roadClass[6] = atof((char*)value);
			}
		}
		// RoadClass7
		else if (0 == strcmp((char*)keyName, RT_PLN_KEY_ROADCLASS7)) {
			if (0 < conditionCnt) {
				// value
				configData->roadParameters.condition[conditionCnt - 1].roadClass[7] = atof((char*)value);
			}
		}
		// RoadClass8
		else if (0 == strcmp((char*)keyName, RT_PLN_KEY_ROADCLASS8)) {
			if (0 < conditionCnt) {
				// value
				configData->roadParameters.condition[conditionCnt - 1].roadClass[8] = atof((char*)value);
			}
		}
		// RoadClass9
		else if (0 == strcmp((char*)keyName, RT_PLN_KEY_ROADCLASS9)) {
			if (0 < conditionCnt) {
				// value
				configData->roadParameters.condition[conditionCnt - 1].roadClass[9] = atof((char*)value);
			}
		}
		// RoadClass10
		else if (0 == strcmp((char*)keyName, RT_PLN_KEY_ROADCLASS10)) {
			if (0 < conditionCnt) {
				// value
				configData->roadParameters.condition[conditionCnt - 1].roadClass[10] = atof((char*)value);
			}
		}
		// RoadClass11
		else if (0 == strcmp((char*)keyName, RT_PLN_KEY_ROADCLASS11)) {
			if (0 < conditionCnt) {
				// value
				configData->roadParameters.condition[conditionCnt - 1].roadClass[11] = atof((char*)value);
			}
		}
		// RoadClass12
		else if (0 == strcmp((char*)keyName, RT_PLN_KEY_ROADCLASS12)) {
			if (0 < conditionCnt) {
				// value
				configData->roadParameters.condition[conditionCnt - 1].roadClass[12] = atof((char*)value);
			}
		}
		// RoadClass13
		else if (0 == strcmp((char*)keyName, RT_PLN_KEY_ROADCLASS13)) {
			if (0 < conditionCnt) {
				// value
				configData->roadParameters.condition[conditionCnt - 1].roadClass[13] = atof((char*)value);
			}
		}
		// RoadClass14
		else if (0 == strcmp((char*)keyName, RT_PLN_KEY_ROADCLASS14)) {
			if (0 < conditionCnt) {
				// value
				configData->roadParameters.condition[conditionCnt - 1].roadClass[14] = atof((char*)value);
			}
		}
		// RoadClass15
		else if (0 == strcmp((char*)keyName, RT_PLN_KEY_ROADCLASS15)) {
			if (0 < conditionCnt) {
				// value
				configData->roadParameters.condition[conditionCnt - 1].roadClass[15] = atof((char*)value);
			}
		}
		// Toll
		else if (0 == strcmp((char*)keyName, RT_PLN_KEY_TOLL)) {
			if (0 < conditionCnt) {
				// value
				configData->roadParameters.condition[conditionCnt - 1].toll = atof((char*)value);
			}
		}
		// Distance
		else if (0 == strcmp((char*)keyName, RT_PLN_KEY_DISTANCE)) {
			if (0 < conditionCnt) {
				// value
				configData->roadParameters.condition[conditionCnt - 1].distance = atoi((char*)value);
			}
		}
	}
	else if (0 == strcmp((char*)keyType, RT_PLN_KEY_DISRANGE)) {
		// Disrange
		if (0 == strcmp((char*)keyName, RT_PLN_KEY_DISRANGE)) {
			// value
			if ((0 <= disrangeCnt) && ((configData->roadParameters.disrangeSize) > disrangeCnt)) {
				configData->roadParameters.disrange[disrangeCnt].disrange = atoi((char*)value);
				disrangeCnt++;
			} else {
				SC_LOG_WarnPrint(SC_TAG_DH, (Char*)"[RoadParameters] Disrange=%s error, " HERE, value);
				disrangeCnt = -1;
			}
		}
		// Length
		else if (0 == strcmp((char*)keyName, RT_PLN_KEY_LENGTH)) {
			if (0 < disrangeCnt) {
				// value
				configData->roadParameters.disrange[disrangeCnt - 1].length = atoi((char*)value);
			}
		}
		// FParameter
		else if (0 == strcmp((char*)keyName, RT_PLN_KEY_FPARAMETER)) {
			if (0 < disrangeCnt) {
				// value
				configData->roadParameters.disrange[disrangeCnt - 1].fparameter = atoi((char*)value);
			}
		}
	}
	else if (0 == strcmp((char*)keyType, RT_PLN_KEY_COUNTRANGE)) {
		// Countrange
		if (0 == strcmp((char*)keyName, RT_PLN_KEY_COUNTRANGE)) {
			// value
			if ((0 <= countrangeCnt) && ((configData->roadParameters.countrangeSize) > countrangeCnt)) {
				configData->roadParameters.countrange[countrangeCnt].countrange = atoi((char*)value);
				countrangeCnt++;
			} else {
				SC_LOG_WarnPrint(SC_TAG_DH, (Char*)"[RoadParameters] Countrange=%s error, " HERE, value);
				countrangeCnt = -1;
			}
		}
		// Length
		else if (0 == strcmp((char*)keyName, RT_PLN_KEY_LENGTH)) {
			if (0 < countrangeCnt) {
				// value
				configData->roadParameters.countrange[countrangeCnt - 1].length = atoi((char*)value);
			}
		}
		// Count
		else if (0 == strcmp((char*)keyName, RT_PLN_KEY_COUNT)) {
			if (0 < countrangeCnt) {
				// value
				configData->roadParameters.countrange[countrangeCnt - 1].count = atoi((char*)value);
			}
		}
		// Level
		else if (0 == strcmp((char*)keyName, RT_PLN_KEY_LEVEL)) {
			if (0 < countrangeCnt) {
				// value
				configData->roadParameters.countrange[countrangeCnt - 1].level = atoi((char*)value);
			}
		}
	}
	// MinCount
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_MINCOUNT)) {
		configData->roadParameters.minCount = atoi((char*)value);
	}
	// UpCount
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_UPCOUNT)) {
		configData->roadParameters.upCount = atoi((char*)value);
	}
	// ThinroadCount
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_THINROADCOUNT)) {
		configData->roadParameters.thinroadCount = atoi((char*)value);
	}
}

/**
 * @brief  INIファイルから読み込んだ値を設定する
 * @param[in] secName   INIファイルから読み込んだセクション名
 * @param[in] keyName   INIファイルから読み込んだキー名
 * @param[in] value     INIファイルから読み込んだ値
 */
void SC_CONFIG_SetValueRoadTypeParameter(const Char *secName, const Char *keyName, const Char *value, SC_ROUTE_PLAN_CONFIG *configData)
{
	// DUMMY
	if (0 == strcmp((char*)keyName, RT_PLN_KEY_DUMMY)) {
		// value
		configData->roadTypeParameter.dummy = atof((char*)value);
	}
	// NOT_SEPARABLE
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_NOT_SEPARABLE)) {
		// value
		configData->roadTypeParameter.notSeparable = atof((char*)value);
	}
	// SEPARABLE
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_SEPARABLE)) {
		// value
		configData->roadTypeParameter.separable = atof((char*)value);
	}
	// JUCTION
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_JUCTION)) {
		// value
		configData->roadTypeParameter.juction = atof((char*)value);
	}
	// INNER
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_INNER)) {
		// value
		configData->roadTypeParameter.inner = atof((char*)value);
	}
	// RAMP
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_RAMP)) {
		// value
		configData->roadTypeParameter.ramp = atof((char*)value);
	}
	// SERVICE
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_SERVICE)) {
		// value
		configData->roadTypeParameter.service = atof((char*)value);
	}
	// SA
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_SA)) {
		// value
		configData->roadTypeParameter.sa = atof((char*)value);
	}
}

/**
 * @brief  INIファイルから読み込んだ値を設定する
 * @param[in] secName   INIファイルから読み込んだセクション名
 * @param[in] keyName   INIファイルから読み込んだキー名
 * @param[in] value     INIファイルから読み込んだ値
 */
void SC_CONFIG_SetValueRoadFlagParameter(const Char *secName, const Char *keyName, const Char *value, SC_ROUTE_PLAN_CONFIG *configData)
{
	// TollwayExitCost
	if (0 == strcmp((char*)keyName, RT_PLN_KEY_TOLLWAYEXITCOST)) {
		// value
		configData->roadFlagParameter.tollwayExitCost = atoi((char*)value);
	}
	// BridgeFactor
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_BRIDGEFACTOR)) {
		// value
		configData->roadFlagParameter.bridgeFactor = atoi((char*)value);
	}
}

/**
 * @brief  INIファイルから読み込んだ値を設定する
 * @param[in] secName   INIファイルから読み込んだセクション名
 * @param[in] keyName   INIファイルから読み込んだキー名
 * @param[in] value     INIファイルから読み込んだ値
 */
void SC_CONFIG_SetValuePlanCount(const Char *secName, const Char *keyName, const Char *value, SC_ROUTE_PLAN_CONFIG *configData)
{
	// MinMeetCount
	if (0 == strcmp((char*)keyName, RT_PLN_KEY_MINMEETCOUNT)) {
		// value
		configData->planCount.minMeetCount = atoi((char*)value);
	}
	// MaxPlanningCount
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_MAXPLANNINGCOUNT)) {
		// value
		configData->planCount.maxPlanningCount = atoi((char*)value);
	}
}

/**
 * @brief  INIファイルから読み込んだ値を設定する
 * @param[in] secName   INIファイルから読み込んだセクション名
 * @param[in] keyName   INIファイルから読み込んだキー名
 * @param[in] value     INIファイルから読み込んだ値
 */
void SC_CONFIG_SetValueOther(const Char *secName, const Char *keyName, const Char *value, SC_ROUTE_PLAN_CONFIG *configData)
{
	// DetourFactor
	if (0 == strcmp((char*)keyName, RT_PLN_KEY_DETOURFACTOR)) {
		// value
		configData->other.detourFactor = atoi((char*)value);
	}
	// DetourDistance
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_DETOURDISTANCE)) {
		// value
		configData->other.detourDistance = atoi((char*)value);
	}
	// ReplanDistance
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_REPLANDISTANCE)) {
		// value
		configData->other.replanDistance = atoi((char*)value);
	}
	// RouteMaxCount
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_ROUTEMAXCOUNT)) {
		// value
		configData->other.routeMaxCount = atoi((char*)value);
	}
	// WalkLimitDistance
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_WALKLIMITDISTANCE)) {
		// value
		configData->other.walkLimitDistance = atoi((char*)value);
	}
	// WalkSpeed
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_WALKSPEED)) {
		// value
		configData->other.walkSpeed = atof((char*)value);
	}
	// LinkFoundDistance
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_LINKFOUNDDISTANCE)) {
		// value
		configData->other.linkFoundDistance = atoi((char*)value);
	}
	// JamDistance
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_JAMDISTANCE)) {
		// value
		configData->other.jamDistance = atoi((char*)value);
	}
	// ShortLinkLength
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_SHORTLINKLENGTH)) {
		// value
		configData->other.shortLinkLength = atoi((char*)value);
	}
	// HighestLevel
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_HIGHESTLEVEL)) {
		// value
		configData->other.highestLevel = atof((char*)value);
	}
	// CarDirectionProcess
	else if (0 == strcmp((char*)keyName, RT_PLN_KEY_CARDIRECTIONPROCESS)) {
		// value
		configData->other.carDirectionProcess = atoi((char*)value);
	}
}

/**
 * @brief  INIファイルから読み込んだ値を設定する
 * @param[in] secName   INIファイルから読み込んだセクション名
 * @param[in] keyName   INIファイルから読み込んだキー名
 * @param[in] value     INIファイルから読み込んだ値
 */
void SC_CONFIG_SetValueRemote(const Char *secName, const Char *keyName, const Char *value, SC_ROUTE_PLAN_CONFIG *configData)
{
	// Toll
	if (0 == strcmp((char*)keyName, RT_PLN_KEY_TOLL)) {
		if (SC_MAX_PATH > strlen((char*)value)) {
			// value
			strcpy((char*)configData->remote.toll, (char*)value);
		}
	}
}
