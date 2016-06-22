/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCORE_POSITION_CONFIG_H
#define SMCORE_POSITION_CONFIG_H


//-----------------------------------
// 構造体定義
//-----------------------------------
typedef struct _SC_POSITION_CONFIG {
	// [Position Mode]
	struct {
		INT32	posMode;		// PosMode
	} positionMode;

	// [Speed Type]
	struct {
		INT32	speed;			// Speed
	} speedType;

	// [DR Scheme]
	struct {
		INT32	sy;				// SY
		INT32	fy;				// FY
		INT32	aio;			// AIO
	} drScheme;

	// [Read Signal Mode]
	struct {
		INT32	readMode;		// ReadMode
	} readSignalMode;

	// [Signal Type]
	struct {
		INT32	signalType;		// SignalType
	} signalType;

	// [MapMatch Intensity]
	struct {
		INT32	mmIntensity;	// MMIntensity
	} mapMatchIntensity;

	// [Serial Port]
	struct {
		INT32	gpsCom;			// GPSCOM
		INT32	gpsBaudRate;	// GPSBaudRate
		INT32	gyroCom;		// GyroCOM
		INT32	gyroBaudRate;	// GyroBaudRate
	} serialPort;

	// [Pulse Nums Per KM]
	struct {
		INT32	pulseNums;		// PulseNums
	} pulseNumsPerKM;

	// [Init Pos]
	struct {
		LONG	lon;			// Lon
		LONG	lat;			// Lat
	} initPos;

	// [Log Info]
	struct {
		INT32	log;			// Log
	} logInfo;

	// [Debug Info]
	struct {
		INT32	debug;			// Debug
	} debugInfo;

	// [Record Signal]
	struct {
		INT32	record;			// Record
	} recordSignal;
} SC_POSITION_CONFIG;
//-----------------------------------
// 外部I/F定義
//-----------------------------------

#endif // #ifndef SMCORE_POSITION_CONFIG_H
