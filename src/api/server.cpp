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
#include <signal.h>
#include "NaviTrace.h"
#include "server.h"
#include <pthread.h>

#include "genivi-navicore.h"
#include "genivi-mapviewer.h"

void dbusServerLoop(Navicore **naviCore, Mapviewer **mapViewer);

DBus::BusDispatcher dispatcher;

static pthread_t g_dbus_thread;
void *dbus_api_server(void *vp);

void leave_signal_handler(int sig)
{
    dispatcher.leave();
}

void dbusServerLoop(Navicore **naviCore, Mapviewer **mapViewer)
{
    signal(SIGTERM, leave_signal_handler);
    signal(SIGINT, leave_signal_handler);

    DBus::default_dispatcher = &dispatcher;
    DBus::Connection conn = DBus::Connection::SessionBus();
    conn.request_name("org.agl.gpsnavi");

    *naviCore = new Navicore(conn);
    *mapViewer = new Mapviewer(conn);
    TRACE_DEBUG("DBus server loop initialized");

	(*naviCore)->CreateSession(std::string("dummy"));
	(*mapViewer)->CreateSession(std::string("dummy"));
	
    dispatcher.enter();
}

void *dbus_api_server(void *vp)
{
	Navicore  *naviCore;
	Mapviewer *mapViewer;
	
	dbusServerLoop(&naviCore, &mapViewer);
	
    delete naviCore;
    delete mapViewer;
    
    pthread_exit(0);
}

void CreateAPIServer(void)
{
	pthread_attr_t attr;
	
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&g_dbus_thread, &attr, dbus_api_server, NULL);	
	pthread_attr_destroy(&attr);
}
