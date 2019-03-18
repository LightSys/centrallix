package org.lightsys.centrallix.objectsystem;

import java.util.List;

public interface ObjReqNotify {
    List getOpenSessions();
    XHashTable getTypeExtensions();
    XHashTable getDriverTypes();
    List getDrivers();
    XHashTable getTypes();
    List getTypeList();
    int getUseCnt();
    ObjDriver getTransLayer();
    ObjDriver getMultiQueryLayer();
    ObjDriver getInheritanceLayer();
    XHashTable getEventHandlers();
    XHashTable getEventsByXData();
    XHashTable getEventsByPath();
    List getEvents();
    ContentType getRootType();
    ObjDriver getRootDriver();
    HandleContext getSessionHandleCtx();
    XHashTable getNotifiesByPath();
    Long getPathID();
    List getLocks();
    HandleContext getTempObjects();
    ObjDriver getTempDriver();
    OSYS_t getOSYS();
    int getTotalFlags();
    List getRequests();
}
