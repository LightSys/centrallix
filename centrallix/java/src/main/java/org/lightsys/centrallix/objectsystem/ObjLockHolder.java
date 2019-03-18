package org.lightsys.centrallix.objectsystem;



public interface ObjLockHolder {
    int getFlags();
    ObjLock getLock();
    ObjSession getSession();
}
