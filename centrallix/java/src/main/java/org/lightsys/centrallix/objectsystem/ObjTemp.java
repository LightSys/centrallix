package org.lightsys.centrallix.objectsystem;



public interface ObjTemp {
    Handle_t getHandle();
    int getLinkCnt();
    Object getData();
    Long getCreateCnt();
}
