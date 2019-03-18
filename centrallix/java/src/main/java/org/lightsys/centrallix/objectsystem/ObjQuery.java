package org.lightsys.centrallix.objectsystem;



public interface ObjQuery {
    int getMagic();
    Object getObj();
    String getQyText();
    Object getTree();
    Object getObjList();
    Object getData();
    int getFlags();
    int getRowID();
    ObjQuerySort getSortInf();
    ObjDriver getDrv();
    ObjSession getQySession();
}
