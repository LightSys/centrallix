package org.lightsys.centrallix.objectsystem;

import java.util.List;

public interface ObjectInstance {
    int getMagic();
    ObjDriver getDriver();
    ObjDriver getTLowLevelDriver();
    ObjDriver getILowLevelDriver();
    Object getData();
    _OF Obj();
    List getAttrs();
    Pathname getPathname();
    short getSubPtr();
    short getSubCnt();
    short getFlags();
    int getMode();
    ContentType getType();
    ObjSession getSession();
    int getLinkCnt();
    String getContentPtr();
    _OF Prev();
    _OF Next();
    ObjectInfo getAdditionalInfo();
    Object getNotifyItem();
    ObjVirtualAttr getVAttrs();
    Object getEvalContext();
    Object getAttrExp();
    String getAttrExpName();
    DateTime getCacheExpire();
}
