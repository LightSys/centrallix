package org.lightsys.centrallix.objectsystem;



public interface ObjEvent {
    String getXData();
    String getWhereClause();
    String getClassCode(); // size 16
    int getFlags();
    ObjEventHandler getHandler();
}
