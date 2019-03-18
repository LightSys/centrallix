package org.lightsys.centrallix.objectsystem;

import java.util.List;

public interface ObjPresentationHints {
    Object getConstraint();
    Object getDefaultExpr();
    Object getMinValue();
    Object getMaxValue();
    List getEnumList();
    String getEnumQuery();
    String getFormat();
    String getAllowChars();
    String getBadChars();
    int getLength();
    int getVisualLength();
    int getVisualLength2();
    int getBitmaskRO();
    int getStyle();
    int getStyleMask();
    int getGroupID();
    String getGroupName();
    int getOrderID();
    String getFriendlyName();
}
