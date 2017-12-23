/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Aisin AW, Ltd
 *
 * This program is licensed under GPL version 2 license.
 * See the LICENSE file distributed with this source file.
 */
#include <stdio.h>
#include <vector>
#include <string>
#include <tuple>
#include "genivi-navicore.h"
#include "genivi-navicore-constants.h"
#include "NaviTrace.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "glview.h"
#include "navicore.h"

#include "navi.h"
#include "naviexport.h"

Navicore::Navicore( DBus::Connection &connection )
    : DBus::ObjectAdaptor(connection, "/org/genivi/navicore"),
      lastSession(0),lastRoute(0),client(""),IsSimulationMode(false),
      SimulationStatus(SIMULATION_STATUS_NO_SIMULATION)
{
}

::DBus::Struct< uint16_t, uint16_t, uint16_t, std::string > Navicore::SessionGetVersion()
{
    ::DBus::Struct<uint16_t, uint16_t, uint16_t, std::string> version;
    version._1 = 3;
    version._2 = 0;
    version._3 = 0;
    version._4 = std::string("22-01-2014");
    return version;
}

uint32_t Navicore::CreateSession_internal(void)
{
    return 0;
}

uint32_t Navicore::CreateSession(const std::string& client)
{
    if (lastSession != 0) // we only handle 1 session for now
    {
        TRACE_ERROR(" ");
        return 0;
    }

    if (CreateSession_internal() < 0) // error
    {
        TRACE_ERROR("fail to create session (%s)", client.c_str());
        return 0;
    }

    lastSession++;
    this->client = client;

    TRACE_INFO("SESSION ADAPTOR - Created session %d [%s]", lastSession, client.c_str());
    return lastSession;
}

void Navicore::DeleteSession(const uint32_t& sessionHandle)
{
    if (sessionHandle != lastSession || !sessionHandle) // we only handle 1 session for now
    {
        TRACE_ERROR("session: %" PRIu32, sessionHandle);
        return;
    }
}

int32_t Navicore::GetSessionStatus(const uint32_t& sessionHandle)
{
    if (sessionHandle == 1 && !NC_IsInitialized()) // we only handle 1 session for now
        return 1; // available
    return 0; // not available
}

std::vector< ::DBus::Struct< uint32_t, std::string > > Navicore::GetAllSessions()
{
    std::vector< ::DBus::Struct< uint32_t, std::string > > list;
    ::DBus::Struct< uint32_t, std::string > a;
    if (lastSession > 0)
    {
        a._1 = lastSession; a._2 = client;
        list.push_back(a);
    }
    return list;
}

::DBus::Struct< uint16_t, uint16_t, uint16_t, std::string > Navicore::RoutingGetVersion()
{
    ::DBus::Struct<uint16_t, uint16_t, uint16_t, std::string> version;
    version._1 = 3;
    version._2 = 0;
    version._3 = 0;
    version._4 = std::string("22-01-2014");
    return version;
}

uint32_t Navicore::CreateRoute(const uint32_t& sessionHandle)
{
    if (sessionHandle != lastSession || !sessionHandle)
    {
        TRACE_ERROR("session %" PRIu32, sessionHandle);
        return 0;
    }

    if (lastRoute != 0) return 0; // we only manage 1 route for now

    lastRoute++;
    TRACE_INFO("Create route %" PRIu32 " for session %" PRIu32, lastRoute, sessionHandle);
    return lastRoute;
}

void Navicore::DeleteRoute(const uint32_t& sessionHandle, const uint32_t& routeHandle)
{
    TRACE_INFO("Delete route %" PRIu32 " for session %" PRIu32, routeHandle, sessionHandle);

    if (sessionHandle != lastSession || routeHandle != lastRoute ||
        !sessionHandle || !routeHandle)
    {
        TRACE_ERROR("session %" PRIu32 ", route %" PRIu32, sessionHandle, routeHandle);
        return;
    }

    CancelRouteCalculation(sessionHandle, routeHandle);
    lastRoute = 0;
    route.clear();
}

void Navicore::SetCostModel(const uint32_t& sessionHandle, const uint32_t& routeHandle, const int32_t& costModel)
{
    TRACE_WARN("TODO: implement this function");
}

int32_t Navicore::GetCostModel(const uint32_t& routeHandle)
{
    TRACE_WARN("TODO: implement this function");
}

std::vector< int32_t > Navicore::GetSupportedCostModels()
{
    TRACE_WARN("TODO: implement this function");
}

void Navicore::SetRoutePreferences(
    const uint32_t& sessionHandle,
    const uint32_t& routeHandle,
    const std::string& countryCode,
    const std::vector< ::DBus::Struct< int32_t, int32_t > >& roadPreferenceList,
    const std::vector< ::DBus::Struct< int32_t, int32_t > >& conditionPreferenceList)
{
    TRACE_WARN("TODO: implement this function");
}
void Navicore::GetRoutePreferences(
    const uint32_t& routeHandle,
    const std::string& countryCode,
    std::vector< ::DBus::Struct< int32_t, int32_t > >& roadPreferenceList,
    std::vector< ::DBus::Struct< int32_t, int32_t > >& conditionPreferenceList)
{
    TRACE_WARN("TODO: implement this function");
}

void Navicore::GetSupportedRoutePreferences(
    std::vector< ::DBus::Struct< int32_t, int32_t > >& routePreferencesList,
    std::vector< ::DBus::Struct< int32_t, int32_t > >& conditionPreferenceList)
{
    TRACE_WARN("TODO: implement this function");
}

void Navicore::SetRouteSchedule(
    const uint32_t& sessionHandle,
    const uint32_t& routeHandle,
    const std::map< int32_t, uint32_t >& routeSchedule)
{
    TRACE_WARN("TODO: implement this function");
}

std::map< int32_t, uint32_t > Navicore::GetRouteSchedule(
    const uint32_t& routeHandle,
    const std::vector< int32_t >& valuesToReturn)
{
    TRACE_WARN("TODO: implement this function");
}

void Navicore::SetTransportationMeans(
    const uint32_t& sessionHandle,
    const uint32_t& routeHandle,
    const std::vector< int32_t >& transportationMeansList)
{
    TRACE_WARN("TODO: implement this function");
}

std::vector< int32_t > Navicore::GetTransportationMeans(
    const uint32_t& routeHandle)
{
    TRACE_WARN("TODO: implement this function");
}

std::vector< int32_t > Navicore::GetSupportedTransportationMeans()
{
    TRACE_WARN("TODO: implement this function");
}

void Navicore::SetExcludedAreas(
    const uint32_t& sessionHandle,
    const uint32_t& routeHandle,
    const std::vector< std::vector< ::DBus::Struct< double, double > > >& excludedAreas)
{
    TRACE_WARN("TODO: implement this function");
}

std::vector< std::vector< ::DBus::Struct< double, double > > > Navicore::GetExcludedAreas(
    const uint32_t& routeHandle)
{
    TRACE_WARN("TODO: implement this function");
}

// private
void Navicore::SetIconVisibilityCoord(IconIndex index, bool visible,
    double lat, double lon, bool commit)
{
    /* start flag, dest flag and pin flag: */
    static const int icon_id[] =
        { IconNum::FLAG_START_NUM, IconNum::FLAG_DEST_NUM, IconNum::FLAG_PIN_NUM };

    demo_icon_info[index].IconID = icon_id[index];
    demo_icon_info[index].Latitude      = (INT32)(lat*1024.0*3600.0);
    demo_icon_info[index].Longititude   = (INT32)(lon*1024.0*3600.0);
    demo_disp_info[index] = visible ? 1 : 0;

    if (commit)
    {
        NC_DM_SetIconInfo(demo_icon_info, 3);
        NC_DM_SetDynamicUDIDisplay(demo_disp_info, 3);
    }
}

// private
void Navicore::SetIconVisibility(IconIndex index, bool visible, bool commit)
{
    /* start flag, dest flag and pin flag: */
    static const int icon_id[] =
        { IconNum::FLAG_START_NUM, IconNum::FLAG_DEST_NUM, IconNum::FLAG_PIN_NUM };

    demo_icon_info[index].IconID = icon_id[index];
    demo_disp_info[index] = visible ? 1 : 0;

    if (commit)
    {
        NC_DM_SetIconInfo(demo_icon_info, 3);
        NC_DM_SetDynamicUDIDisplay(demo_disp_info, 3);
    }
}

void Navicore::SetWaypoints(
    const uint32_t& sessionHandle,
    const uint32_t& routeHandle,
    const bool& startFromCurrentPosition,
    const std::vector< std::map< int32_t, ::DBus::Struct< uint8_t, ::DBus::Variant > > >& waypointsList)
{
    int index = 0;

    TRACE_INFO("route %" PRIu32 " for session %" PRIu32, routeHandle, sessionHandle);

    // we only handle 1 session & 1 route for now
    if (sessionHandle != lastSession || routeHandle != lastRoute ||
        !sessionHandle || !routeHandle)
    {
        TRACE_ERROR("session %" PRIu32 ", route %" PRIu32, sessionHandle, routeHandle);
        return;
    }

    /* mask Pin flag: */
    SetIconVisibility(FLAG_PIN_IDX, false, false);

    /* memorize route, as we don't have a NC API to retrieve way points: */
    route = waypointsList;

    if (startFromCurrentPosition)
    {
        SMCARSTATE carState;

        if (NC_DM_GetCarState(&carState, e_SC_CARLOCATION_NOW) == NC_SUCCESS)
        {
            /* Add current position in the memorized list of waypoints: */
            std::map< int32_t, ::DBus::Struct< uint8_t, ::DBus::Variant > > new_entry;
            ::DBus::Struct< uint8_t, ::DBus::Variant > new_lat, new_lon;
            new_lat._1 = NAVICORE_LATITUDE;
            new_lat._2.writer().append_double(carState.coord.latitude/1024.0/3600.0);
            new_entry[NAVICORE_LATITUDE] = new_lat;
            new_lon._1 = NAVICORE_LONGITUDE;
            new_lon._2.writer().append_double(carState.coord.longitude/1024.0/3600.0);
            new_entry[NAVICORE_LONGITUDE] = new_lon;
            route.insert(route.begin(), new_entry); /* insert in first place */
            /* --- */

            /* Display Start icon: */
            SetIconVisibilityCoord(FLAG_START_IDX, true,
                carState.coord.latitude/1024.0/3600.0, carState.coord.longitude/1024.0/3600.0);
            index++;
            /* --- */
        }
        else
        {
            TRACE_ERROR("unable to get current car position");
        }
    }

    std::vector< std::map< int32_t, ::DBus::Struct< uint8_t, ::DBus::Variant > > >::const_iterator wp_map;
    for (wp_map = waypointsList.begin(); wp_map != waypointsList.end(); wp_map++)
    {
        if (index == 0 || index == (route.size()-1))
        {
            double newLat, newLon;

            TRACE_DEBUG("entry map");
            std::map< int32_t, ::DBus::Struct< uint8_t, ::DBus::Variant > >::const_iterator map;
            for (map = (*wp_map).begin(); map != (*wp_map).end(); map++)
            {
                TRACE_DEBUG("entry variant");
                if ((*map).first == NAVICORE_LATITUDE)
                {
                    newLat = (*map).second._2.reader().get_double();
                    TRACE_DEBUG("newLat = %f", newLat);
                }
                else if ((*map).first == NAVICORE_LONGITUDE)
                {
                    newLon = (*map).second._2.reader().get_double();
                    TRACE_DEBUG("newLon = %f", newLon);
                }
            }

            if (index == 0) {
                // Display Start icon:
                SetIconVisibilityCoord(FLAG_START_IDX, true, newLat, newLon);
            } else if (index == (route.size()-1)) {
                // Display Dest icon and update display (commit = true):
                SetIconVisibilityCoord(FLAG_DEST_IDX, true, newLat, newLon, true);
            }
        }
        index++;
    }

    sample_hmi_set_pin_mode(0);
    sample_hmi_request_mapDraw();
    sample_hmi_set_fource_update_mode();
    sample_hmi_request_update();
}

void Navicore::GetWaypoints(
    const uint32_t& routeHandle,
    bool& startFromCurrentPosition,
    std::vector< std::map< int32_t, ::DBus::Struct< uint8_t, ::DBus::Variant > > >& waypointsList)
{
    TRACE_INFO("route %" PRIu32, routeHandle);

    startFromCurrentPosition = false; // TODO: handle this parameter;

    if (routeHandle != lastRoute || !routeHandle)
    {
        TRACE_ERROR("route %" PRIu32, routeHandle);
        return;
    }

    waypointsList = route;
}

void Navicore::CalculateRoute(
    const uint32_t& sessionHandle,
    const uint32_t& routeHandle)
{
    double newLat, newLon;
    SMRPPOINT *NcPointsTab;
    size_t size;
    int index = 0;

    TRACE_INFO("route %" PRIu32 " for session %" PRIu32, routeHandle, sessionHandle);

    // we only handle 1 session & 1 route for now
    if (sessionHandle != lastSession || routeHandle != lastRoute ||
        !sessionHandle || !routeHandle)
    {
        TRACE_ERROR("session %" PRIu32 ", route %" PRIu32, sessionHandle, routeHandle);
        return;
    }

    size = route.size();
    NcPointsTab = (SMRPPOINT*) calloc(size, sizeof(SMRPPOINT));
    TRACE_DEBUG("alloc array of size %zu", size);

    std::vector< std::map< int32_t, ::DBus::Struct< uint8_t, ::DBus::Variant > > >::const_iterator wp_map;
    for (wp_map = route.begin(); wp_map != route.end(); wp_map++)
    {
        TRACE_DEBUG("entry map");
        std::map< int32_t, ::DBus::Struct< uint8_t, ::DBus::Variant > >::const_iterator map;
        for (map = (*wp_map).begin(); map != (*wp_map).end(); map++)
        {
            TRACE_DEBUG("entry variant");
            if ((*map).first == NAVICORE_LATITUDE)
            {
                newLat = (*map).second._2.reader().get_double();
                TRACE_DEBUG("newLat = %f", newLat);
            }
            else if ((*map).first == NAVICORE_LONGITUDE)
            {
                newLon = (*map).second._2.reader().get_double();
                TRACE_DEBUG("newLon = %f", newLon);
            }
        }

        /* Add waypoint in the NC table: */
        TRACE_DEBUG("Add waypoint in the NC table");
        NcPointsTab[index].coord.latitude = newLat*1024.0*3600.0;
        NcPointsTab[index].coord.longitude = newLon*1024.0*3600.0;
        if (index == 0) {
            NcPointsTab[index].rpPointType = LST_START;
        } else if (index == (size-1)) {
            NcPointsTab[index].rpPointType = LST_DEST;
        } else {
            NcPointsTab[index].rpPointType = LST_NORMAL;
        }
        NcPointsTab[index].rpPointIndex = index;
        TRACE_DEBUG("insert point %d", index);
        index++;
        /* --- */
    }

    TRACE_DEBUG("calling NC_RP_PlanSingleRoute (%zu points)", size);
    NC_RP_PlanSingleRoute(&NcPointsTab[0], size);

    free(NcPointsTab);

    SetIconVisibility(FLAG_START_IDX, true);
    SetIconVisibility(FLAG_DEST_IDX, true);
    SetIconVisibility(FLAG_PIN_IDX, false, true);

    sample_hmi_set_pin_mode(0);
    sample_hmi_request_mapDraw();
    sample_hmi_set_fource_update_mode();
    sample_hmi_request_update();
}

void Navicore::CancelRouteCalculation(
    const uint32_t& sessionHandle,
    const uint32_t& routeHandle)
{
    TRACE_INFO("route %" PRIu32 " for session %" PRIu32, routeHandle, sessionHandle);

    // we only handle 1 session & 1 route for now
    if (sessionHandle != lastSession || routeHandle != lastRoute ||
        !sessionHandle || !routeHandle)
    {
        TRACE_ERROR("session %" PRIu32 ", route %" PRIu32, sessionHandle, routeHandle);
        return;
    }

    NC_RP_DeleteRouteResult();

    SetIconVisibility(FLAG_START_IDX, false);
    SetIconVisibility(FLAG_DEST_IDX, false);
    SetIconVisibility(FLAG_PIN_IDX, true, true);

    sample_hmi_set_pin_mode(1);
    sample_hmi_request_mapDraw();
    sample_hmi_set_fource_update_mode();
    sample_hmi_request_update();
}

std::vector< uint32_t > Navicore::CalculateRoutes(
    const uint32_t& sessionHandle,
    const std::vector< uint32_t >& calculatedRoutesList)
{
    TRACE_WARN("TODO: implement this function");
}

void Navicore::GetRouteSegments(
    const uint32_t& routeHandle,
    const int16_t& detailLevel,
    const std::vector< int32_t >& valuesToReturn,
                       const uint32_t& numberOfSegments,
                       const uint32_t& offset,
                       uint32_t& totalNumberOfSegments,
                       std::vector< std::map< int32_t,
                       ::DBus::Struct< uint8_t, ::DBus::Variant > > >& routeSegments)
{
    TRACE_WARN("TODO: implement this function");
}

std::map< int32_t, ::DBus::Struct< uint8_t, ::DBus::Variant > > Navicore::GetRouteOverview(
    const uint32_t& routeHandle,
    const std::vector< int32_t >& valuesToReturn)
{
    TRACE_WARN("TODO: implement this function");
}

::DBus::Struct< ::DBus::Struct< double, double >, ::DBus::Struct< double, double > > Navicore::GetRouteBoundingBox(
    const uint32_t& routeHandle)
{
    TRACE_WARN("TODO: implement this function");
}

std::vector< uint32_t > Navicore::GetAllRoutes()
{
    std::vector<uint32_t> ret;
    // we only handle 1 route:
    if (lastRoute == 1) ret.push_back(lastRoute);
    return ret;
}

void Navicore::SetBlockedRouteStretches(
    const uint32_t& sessionHandle,
    const uint32_t& routeHandle,
    const std::vector< ::DBus::Struct< uint32_t, uint32_t > >& blockParameters)
{
    TRACE_WARN("TODO: implement this function");
}

std::vector< ::DBus::Struct< uint32_t, uint32_t > > Navicore::GetBlockedRouteStretches(
    const uint32_t& routeHandle)
{
    TRACE_WARN("TODO: implement this function");
}

::DBus::Struct< uint16_t, uint16_t, uint16_t, std::string > Navicore::PositionGetVersion()
{
    ::DBus::Struct<uint16_t, uint16_t, uint16_t, std::string> version;
    version._1 = 3;
    version._2 = 0;
    version._3 = 0;
    version._4 = std::string("21-01-2014");
    return version;
}

void Navicore::FixSimulationStatus()
{
    /* if simu was launched from GUI and we were not informed : */
    if (IsSimulationMode == false && NC_Simulation_IsInSimu() == 1)
    {
        IsSimulationMode = true;
        SimulationStatus = SIMULATION_STATUS_RUNNING;
    }

    /* if simu was stopped from GUI and we were not informed : */
    if (IsSimulationMode == true && NC_Simulation_IsInSimu() == 0)
    {
        IsSimulationMode = false;
        SimulationStatus = SIMULATION_STATUS_NO_SIMULATION;
    }
}

void Navicore::SetSimulationMode(const uint32_t& sessionHandle, const bool& activate)
{
    TRACE_INFO("activate = %d (%d)", activate, IsSimulationMode);
    if (sessionHandle != lastSession || !sessionHandle) return;

    /* if simu was launched / stopped from GUI and we were not informed : */
    FixSimulationStatus();

    if (activate == IsSimulationMode) return; // nothing to do

    if (!activate && SimulationStatus != SIMULATION_STATUS_NO_SIMULATION)
    {
        TRACE_DEBUG("calling sample_guide_request_end");
        sample_guide_request_end();
        SimulationStatus = SIMULATION_STATUS_NO_SIMULATION;
    }
    else if (activate)
    {
        SimulationStatus = SIMULATION_STATUS_FIXED_POSITION;
    }

    IsSimulationMode = activate;
    TRACE_DEBUG("new state: %d, %d", IsSimulationMode, (int)SimulationStatus);
}

int32_t Navicore::GetSimulationStatus()
{
    TRACE_INFO("SimulationStatus: %d", (int)SimulationStatus);

    /* if simu was launched / stopped from GUI and we were not informed : */
    FixSimulationStatus();
    
    return SimulationStatus;
}

void Navicore::AddSimulationStatusListener()
{
    TRACE_WARN("TODO: implement this function");
}

void Navicore::RemoveSimulationStatusListener()
{
    TRACE_WARN("TODO: implement this function");
}

void Navicore::SetSimulationSpeed(const uint32_t& sessionHandle, const uint8_t& speedFactor)
{
    TRACE_WARN("TODO: implement this function");
}

uint8_t Navicore::GetSimulationSpeed()
{
    TRACE_WARN("TODO: implement this function");
}

void Navicore::AddSimulationSpeedListener()
{
    TRACE_WARN("TODO: implement this function");
}

void Navicore::RemoveSimulationSpeedListener()
{
    TRACE_WARN("TODO: implement this function");
}

void Navicore::StartSimulation(const uint32_t& sessionHandle)
{
    TRACE_INFO("current status: %d, %d", IsSimulationMode, (int)SimulationStatus);
    if (sessionHandle != lastSession || !sessionHandle) return;

    if (!IsSimulationMode) return; // activate simulation first
    else if (SimulationStatus == SIMULATION_STATUS_RUNNING) return; // nothing to do

    TRACE_DEBUG("calling sample_guide_request_start");
    sample_guide_request_start();
    SimulationStatus = SIMULATION_STATUS_RUNNING;

    TRACE_DEBUG("new state: %d, %d", IsSimulationMode, (int)SimulationStatus);
}

void Navicore::PauseSimulation(const uint32_t& sessionHandle)
{
    TRACE_WARN("TODO: implement this function");
}

static inline int32_t CONVERT_TO_GENIVI_HEADING(int32_t angle)
{
    /**
    Hitachi heading angle:      Genivi heading angle:
    *        90                          0
    *   180      0                  270     90
    *       270                         180
    */
    return (450-angle) % 360;
}

std::map< int32_t, ::DBus::Struct< uint8_t, ::DBus::Variant > >
    Navicore::GetPosition(const std::vector< int32_t >& valuesToReturn)
{
    std::map< int32_t, ::DBus::Struct< uint8_t, ::DBus::Variant > > ret;
    SMCARSTATE carState;

    TRACE_INFO(" ");

    if (NC_DM_GetCarState(&carState, e_SC_CARLOCATION_NOW) != NC_SUCCESS)
    {
        TRACE_ERROR("NC_DM_GetCarState failed");
        return ret;
    }

    for (std::vector< int32_t >::const_iterator it = valuesToReturn.begin(); it != valuesToReturn.end(); it++)
    {
        ::DBus::Struct< uint8_t, ::DBus::Variant > s;
        s._1 = *it;

        if (s._1 == NAVICORE_LATITUDE) {
            s._2.writer().append_double(carState.coord.latitude/1024.0/3600.0);
            ret[NAVICORE_LATITUDE] = s;
            TRACE_INFO("\tlatitude: %f", (double)(carState.coord.latitude/1024.0/3600.0));
        } else if (s._1 == NAVICORE_LONGITUDE) {
            s._2.writer().append_double(carState.coord.longitude/1024.0/3600.0);
            ret[NAVICORE_LONGITUDE] = s;
            TRACE_INFO("\tlongitude: %f", (double)(carState.coord.longitude/1024.0/3600.0));
        /*} else if (s._1 == NAVICORE_TIMESTAMP) {
            s._2.writer().append_double(0.0); // TODO: not supported
            ret[NAVICORE_TIMESTAMP] = s;
            TRACE_INFO("\tGPS time: %s", carState.gpsTime);*/
        } else if (s._1 == NAVICORE_HEADING) {
            s._2.writer().append_uint32((uint32_t)CONVERT_TO_GENIVI_HEADING(carState.dir));
            ret[NAVICORE_HEADING] = s;
            TRACE_INFO("\tdirection: %" PRId32, carState.dir);
        /*} else if (s._1 == NAVICORE_SPEED) { // TODO: not supported
            s._2.writer().append_int32((int32_t)carState.speed);
            ret[NAVICORE_SPEED] = s;
            TRACE_INFO("\tspeed: %" PRId32 "(%f)", (int32_t)carState.speed, carState.speed);*/
        } else if (s._1 == NAVICORE_SIMULATION_MODE) {
            s._2.writer().append_bool(NC_Simulation_IsInSimu());
            ret[NAVICORE_SIMULATION_MODE] = s;
            TRACE_INFO("\tsimulation mode: %d", NC_Simulation_IsInSimu());
        }
    }

    return ret;
}

void Navicore::SetPosition(
    const uint32_t& sessionHandle,
    const std::map< int32_t, ::DBus::Struct< uint8_t, ::DBus::Variant > >& position)
{
    TRACE_WARN("TODO: implement this function");
}

std::map< int32_t, ::DBus::Struct< uint8_t, ::DBus::Variant > >
    Navicore::GetAddress(const std::vector< int32_t >& valuesToReturn)
{
    TRACE_WARN("TODO: implement this function");
}

std::map< int32_t, ::DBus::Struct< uint8_t, ::DBus::Variant > >
    Navicore::GetPositionOnSegment(const std::vector< int32_t >& valuesToReturn)
{
    TRACE_WARN("TODO: implement this function");
}

std::map< int32_t, ::DBus::Struct< uint8_t, ::DBus::Variant > >
    Navicore::GetStatus(const std::vector< int32_t >& valuesToReturn)
{
    TRACE_WARN("TODO: implement this function");
}

::DBus::Struct< uint16_t, uint16_t, uint16_t, std::string > Navicore::GuidanceGetVersion()
{
    ::DBus::Struct<uint16_t, uint16_t, uint16_t, std::string> version;
    version._1 = 3;
    version._2 = 1;
    version._3 = 0;
    version._4 = std::string("03-03-2014");
    return version;
}

void Navicore::Navicore::StartGuidance(const uint32_t& sessionHandle, const uint32_t& routeHandle)
{
    TRACE_WARN("TODO: implement this function");
}

void Navicore::StopGuidance(const uint32_t& sessionHandle)
{
    TRACE_WARN("TODO: implement this function");
}

void Navicore::SetVoiceGuidance(const bool& activate, const std::string& voice)
{
    TRACE_WARN("TODO: implement this function");
}

void Navicore::GetGuidanceDetails(bool& voiceGuidance, bool& vehicleOnTheRoad, bool& isDestinationReached, int32_t& maneuver)
{
    TRACE_WARN("TODO: implement this function");
}

void Navicore::PlayVoiceManeuver()
{
    TRACE_WARN("TODO: implement this function");
}

void Navicore::GetWaypointInformation(const uint16_t& requestedNumberOfWaypoints, uint16_t& numberOfWaypoints, std::vector< ::DBus::Struct< uint32_t, uint32_t, int32_t, int32_t, int16_t, int16_t, bool, uint16_t > >& waypointsList)
{
    TRACE_WARN("TODO: implement this function");
}

void Navicore::GetDestinationInformation(uint32_t& offset, uint32_t& travelTime, int32_t& direction, int32_t& side, int16_t& timeZone, int16_t& daylightSavingTime)
{
    TRACE_WARN("TODO: implement this function");
}

void Navicore::GetManeuversList(const uint16_t& requestedNumberOfManeuvers, const uint32_t& maneuverOffset, uint16_t& numberOfManeuvers, std::vector< ::DBus::Struct< std::string, std::string, uint16_t, int32_t, uint32_t, std::vector< ::DBus::Struct< uint32_t, uint32_t, int32_t, int32_t, std::map< int32_t, ::DBus::Struct< uint8_t, ::DBus::Variant > > > > > >& maneuversList)
{
    TRACE_WARN("TODO: implement this function");
}

void Navicore::SetRouteCalculationMode(const uint32_t& sessionHandle, const int32_t& routeCalculationMode)
{
    TRACE_WARN("TODO: implement this function");
}

void Navicore::SkipNextManeuver(const uint32_t& sessionHandle)
{
   TRACE_WARN("TODO: implement this function");
}

void Navicore::GetGuidanceStatus(int32_t& guidanceStatus, uint32_t& routeHandle)
{
    TRACE_WARN("TODO: implement this function");
}

void Navicore::SetVoiceGuidanceSettings(const int32_t& promptMode)
{
    TRACE_WARN("TODO: implement this function");
}

int32_t Navicore::GetVoiceGuidanceSettings()
{
    TRACE_WARN("TODO: implement this function");
}
