package org.lightsys.centrallix.objectsystem;

import java.util.List;

public interface ObjTrxTree {
    int getMagic();
    int getStatus();
    int getOpType();
    _OT Parent();
    _OT Parallel();
    _OT Next();
    _OT Prev();
    List getChildren();
    Object getObject();
    int getAllocObj();
    String getUsrType(); // size 80
    int getMask();
    ObjDriver getLLDriver();
    Object getLLParam();
    int getLinkCnt();
    String getPathPtr();
    String getAttrName(); // size 64
    int getAttrType();
    Object getAttrValue();
}
