package org.lightsys.centrallix.objectsystem;



public interface ObjVirtualAttr {
    _OVA Next();
    String getName(); // size 32
    Object getContext();
    int TypeFn();
    int GetFn();
    int SetFn();
    int FinalizeFn();
}
