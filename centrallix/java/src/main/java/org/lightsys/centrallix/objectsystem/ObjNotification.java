package org.lightsys.centrallix.objectsystem;



public interface ObjNotification {
    ObjReqNotifyItem get__Item();
    Object getObj();
    Object getContext();
    int getWhat();
    TObjData getNewAttrValue();
    int getPtrSize();
    int getOffset();
    int getNewSize();
    Object getPtr();
    int getIsDel();
    Thread getWorker();
}
