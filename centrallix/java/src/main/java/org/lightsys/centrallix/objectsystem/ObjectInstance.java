package org.lightsys.centrallix.objectsystem;

import java.util.List;

/**
 * Equivalent to "Object" struct in C.
 */
public interface ObjectInstance {
    int getMagic();
    ObjDriver getDriver();
    ObjDriver getTLowLevelDriver();
    ObjDriver getILowLevelDriver();
    Object getData();

    /**
     * If this is an attribute, this is the object it belongs to.
     */
    ObjectInstance Obj();
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
    ObjectInstance Prev();
    ObjectInstance Next();
    ObjectInfo getAdditionalInfo();
    Object getNotifyItem();
    ObjVirtualAttr getVAttrs();
    Object getEvalContext();
    Object getAttrExp();
    String getAttrExpName();
    DateTime getCacheExpire();
}
