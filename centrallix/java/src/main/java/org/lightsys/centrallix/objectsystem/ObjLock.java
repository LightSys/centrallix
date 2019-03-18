package org.lightsys.centrallix.objectsystem;

import java.util.List;
import java.util.concurrent.Semaphore;

public interface ObjLock {
    int getFlags();
    List getHolders();
    Semaphore getWriteLock();
}
