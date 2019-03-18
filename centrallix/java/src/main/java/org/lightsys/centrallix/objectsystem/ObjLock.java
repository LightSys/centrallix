package org.lightsys.centrallix.objectsystem;

import java.util.List;

public interface ObjLock {
    int getFlags();
    List getHolders();
    Semaphore getWriteLock();
}
