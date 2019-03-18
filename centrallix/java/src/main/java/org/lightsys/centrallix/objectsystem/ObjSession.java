package org.lightsys.centrallix.objectsystem;

import java.util.List;

public interface ObjSession {
    int getMagic();
    List getOpenObjects();
    List getOpenQueries();
    ObjTrxTree getTrx();
    XHashQueue getDirectoryCache();
    Handle_t getHandle();
}
