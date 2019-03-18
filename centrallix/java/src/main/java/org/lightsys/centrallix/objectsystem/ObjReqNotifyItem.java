package org.lightsys.centrallix.objectsystem;

import java.util.List;

public interface ObjReqNotifyItem {
    ObjReqNotifyItem Next();
    ObjReqNotify getNotifyStruct();
    Object getObj();
    int getFlags();
    int CallbackFn();
    Object getCallerContext();
    MTSecContext getSavedSecContext();
    List getNotifications();
}
