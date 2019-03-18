package org.lightsys.centrallix.objectsystem;

import java.util.List;

public interface ObjTrxTree {
    int getMagic();
    int getStatus();
    int getOpType();
    ObjTrxTree Parent();
    ObjTrxTree Parallel();
    ObjTrxTree Next();
    ObjTrxTree Prev();
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
