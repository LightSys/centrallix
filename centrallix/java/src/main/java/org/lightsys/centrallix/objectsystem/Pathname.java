package org.lightsys.centrallix.objectsystem;



public interface Pathname {
    int getNElements();
    String getOpenCtlBuf();
    int getOpenCtlLen();
    int getOpenCtlCnt();
    int getLinkCnt();
}
