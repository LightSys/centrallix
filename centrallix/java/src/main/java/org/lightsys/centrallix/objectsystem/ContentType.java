package org.lightsys.centrallix.objectsystem;

import java.util.List;

public interface ContentType {
    String getName(); // size 128
    String getDescription(); // size 256
    List getExtensions();
    List getIsA();
    int getFlags();
    List getRelatedTypes();
    List getRelationLevels();
    Object getTypeNameObjList();
    Object getTypeNameExpression();
}
