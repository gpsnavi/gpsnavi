/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Aisin AW, Ltd
 *
 * This program is licensed under GPL version 2 license.
 * See the LICENSE file distributed with this source file.
 */
#ifndef GENIVI_NAVICORE_H
#define GENIVI_NAVICORE_H

#include <dbus-c++/dbus.h>
#include "genivi-navigationcore-adaptor.h"

#include "glview.h"

class Navicore :
    public org::genivi::navigationcore::Session_adaptor,
    public org::genivi::navigationcore::Routing_adaptor,
    public org::genivi::navigationcore::MapMatchedPosition_adaptor,
    public org::genivi::navigationcore::Guidance_adaptor,
    public DBus::IntrospectableAdaptor,
    public DBus::ObjectAdaptor
{
    public:
        Navicore( DBus::Connection &connection );

        enum IconIndex
        {
            FLAG_START_IDX = 0,
            FLAG_DEST_IDX = 1,
            FLAG_PIN_IDX = 2
        };

        enum IconNum
        {
            FLAG_START_NUM = 21,
            FLAG_DEST_NUM = 22,
            FLAG_PIN_NUM = 3
        };

        enum SimulationStatus
        {
            SIMULATION_STATUS_NO_SIMULATION     = 544,
            SIMULATION_STATUS_RUNNING           = 545,
            //SIMULATION_STATUS_PAUSED          = 546, /* not supported yet */
            SIMULATION_STATUS_FIXED_POSITION    = 547
        };

        // Session interface
        ::DBus::Struct< uint16_t, uint16_t, uint16_t, std::string > SessionGetVersion();
        uint32_t CreateSession(const std::string& client);
        void DeleteSession(const uint32_t& sessionHandle);
        int32_t GetSessionStatus(const uint32_t& sessionHandle);
        std::vector< ::DBus::Struct< uint32_t, std::string > > GetAllSessions();

        // Routing interface
        ::DBus::Struct< uint16_t, uint16_t, uint16_t, std::string > RoutingGetVersion();
        uint32_t CreateRoute(const uint32_t& sessionHandle);
        void DeleteRoute(const uint32_t& sessionHandle, const uint32_t& routeHandle);
        void SetCostModel(const uint32_t& sessionHandle, const uint32_t& routeHandle, const int32_t& costModel);
        int32_t GetCostModel(const uint32_t& routeHandle);
        std::vector< int32_t > GetSupportedCostModels();
        void SetRoutePreferences(const uint32_t& sessionHandle, const uint32_t& routeHandle, const std::string& countryCode, const std::vector< ::DBus::Struct< int32_t, int32_t > >& roadPreferenceList, const std::vector< ::DBus::Struct< int32_t, int32_t > >& conditionPreferenceList);
        void GetRoutePreferences(const uint32_t& routeHandle, const std::string& countryCode, std::vector< ::DBus::Struct< int32_t, int32_t > >& roadPreferenceList, std::vector< ::DBus::Struct< int32_t, int32_t > >& conditionPreferenceList);
        void GetSupportedRoutePreferences(std::vector< ::DBus::Struct< int32_t, int32_t > >& routePreferencesList, std::vector< ::DBus::Struct< int32_t, int32_t > >& conditionPreferenceList);
        void SetRouteSchedule(const uint32_t& sessionHandle, const uint32_t& routeHandle, const std::map< int32_t, uint32_t >& routeSchedule);
        std::map< int32_t, uint32_t > GetRouteSchedule(const uint32_t& routeHandle, const std::vector< int32_t >& valuesToReturn);
        void SetTransportationMeans(const uint32_t& sessionHandle, const uint32_t& routeHandle, const std::vector< int32_t >& transportationMeansList);
        std::vector< int32_t > GetTransportationMeans(const uint32_t& routeHandle);
        std::vector< int32_t > GetSupportedTransportationMeans();
        void SetExcludedAreas(const uint32_t& sessionHandle, const uint32_t& routeHandle, const std::vector< std::vector< ::DBus::Struct< double, double > > >& excludedAreas);
        std::vector< std::vector< ::DBus::Struct< double, double > > > GetExcludedAreas(const uint32_t& routeHandle);
        void SetWaypoints(const uint32_t& sessionHandle, const uint32_t& routeHandle, const bool& startFromCurrentPosition, const std::vector< std::map< int32_t, ::DBus::Struct< uint8_t, ::DBus::Variant > > >& waypointsList);
        void GetWaypoints(const uint32_t& routeHandle, bool& startFromCurrentPosition, std::vector< std::map< int32_t, ::DBus::Struct< uint8_t, ::DBus::Variant > > >& waypointsList);
        void CalculateRoute(const uint32_t& sessionHandle, const uint32_t& routeHandle);
        void CancelRouteCalculation(const uint32_t& sessionHandle, const uint32_t& routeHandle);
        std::vector< uint32_t > CalculateRoutes(const uint32_t& sessionHandle, const std::vector< uint32_t >& calculatedRoutesList);
        void GetRouteSegments(const uint32_t& routeHandle, const int16_t& detailLevel, const std::vector< int32_t >& valuesToReturn, const uint32_t& numberOfSegments, const uint32_t& offset, uint32_t& totalNumberOfSegments, std::vector< std::map< int32_t, ::DBus::Struct< uint8_t, ::DBus::Variant > > >& routeSegments);
        std::map< int32_t, ::DBus::Struct< uint8_t, ::DBus::Variant > > GetRouteOverview(const uint32_t& routeHandle, const std::vector< int32_t >& valuesToReturn);
        ::DBus::Struct< ::DBus::Struct< double, double >, ::DBus::Struct< double, double > > GetRouteBoundingBox(const uint32_t& routeHandle);
        std::vector< uint32_t > GetAllRoutes();
        void SetBlockedRouteStretches(const uint32_t& sessionHandle, const uint32_t& routeHandle, const std::vector< ::DBus::Struct< uint32_t, uint32_t > >& blockParameters);
        std::vector< ::DBus::Struct< uint32_t, uint32_t > > GetBlockedRouteStretches(const uint32_t& routeHandle);

        // MapMatchedPosition interface
        ::DBus::Struct< uint16_t, uint16_t, uint16_t, std::string > PositionGetVersion();
        void SetSimulationMode(const uint32_t& sessionHandle, const bool& activate);
        int32_t GetSimulationStatus();
        void AddSimulationStatusListener();
        void RemoveSimulationStatusListener();
        void SetSimulationSpeed(const uint32_t& sessionHandle, const uint8_t& speedFactor);
        uint8_t GetSimulationSpeed();
        void AddSimulationSpeedListener();
        void RemoveSimulationSpeedListener();
        void StartSimulation(const uint32_t& sessionHandle);
        void PauseSimulation(const uint32_t& sessionHandle);
        std::map< int32_t, ::DBus::Struct< uint8_t, ::DBus::Variant > > GetPosition(const std::vector< int32_t >& valuesToReturn);
        void SetPosition(const uint32_t& sessionHandle, const std::map< int32_t, ::DBus::Struct< uint8_t, ::DBus::Variant > >& position);
        std::map< int32_t, ::DBus::Struct< uint8_t, ::DBus::Variant > > GetAddress(const std::vector< int32_t >& valuesToReturn);
        std::map< int32_t, ::DBus::Struct< uint8_t, ::DBus::Variant > > GetPositionOnSegment(const std::vector< int32_t >& valuesToReturn);
        std::map< int32_t, ::DBus::Struct< uint8_t, ::DBus::Variant > > GetStatus(const std::vector< int32_t >& valuesToReturn);

        // Guidance interface
        ::DBus::Struct< uint16_t, uint16_t, uint16_t, std::string > GuidanceGetVersion();
        void StartGuidance(const uint32_t& sessionHandle, const uint32_t& routeHandle);
        void StopGuidance(const uint32_t& sessionHandle);
        void SetVoiceGuidance(const bool& activate, const std::string& voice);
        void GetGuidanceDetails(bool& voiceGuidance, bool& vehicleOnTheRoad, bool& isDestinationReached, int32_t& maneuver);
        void PlayVoiceManeuver();
        void GetWaypointInformation(const uint16_t& requestedNumberOfWaypoints, uint16_t& numberOfWaypoints, std::vector< ::DBus::Struct< uint32_t, uint32_t, int32_t, int32_t, int16_t, int16_t, bool, uint16_t > >& waypointsList);
        void GetDestinationInformation(uint32_t& offset, uint32_t& travelTime, int32_t& direction, int32_t& side, int16_t& timeZone, int16_t& daylightSavingTime);
        void GetManeuversList(const uint16_t& requestedNumberOfManeuvers, const uint32_t& maneuverOffset, uint16_t& numberOfManeuvers, std::vector< ::DBus::Struct< std::string, std::string, uint16_t, int32_t, uint32_t, std::vector< ::DBus::Struct< uint32_t, uint32_t, int32_t, int32_t, std::map< int32_t, ::DBus::Struct< uint8_t, ::DBus::Variant > > > > > >& maneuversList);
        void SetRouteCalculationMode(const uint32_t& sessionHandle, const int32_t& routeCalculationMode);
        void SkipNextManeuver(const uint32_t& sessionHandle);
        void GetGuidanceStatus(int32_t& guidanceStatus, uint32_t& routeHandle);
        void SetVoiceGuidanceSettings(const int32_t& promptMode);
        int32_t GetVoiceGuidanceSettings();

    private:
        uint32_t CreateSession_internal(void);
        void SetIconVisibilityCoord(IconIndex index, bool visible,
            double lat = 0.0, double lon = 0.0, bool commit = false);
        void SetIconVisibility(IconIndex index, bool visible, bool commit = false);
        void FixSimulationStatus();

        uint32_t lastSession, lastRoute;
        std::string client;
        bool IsSimulationMode;
        SimulationStatus SimulationStatus;
        std::vector< std::map< int32_t, ::DBus::Struct< uint8_t, ::DBus::Variant > > > route;
};

#endif // GENIVI_NAVICORE_H
