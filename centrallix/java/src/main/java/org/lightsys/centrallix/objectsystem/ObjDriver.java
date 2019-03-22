package org.lightsys.centrallix.objectsystem;

import java.util.List;

public interface ObjDriver {
    String getName(); // size 64
    List<String> getRootContentTypes();
    int getCapabilities();
    void open(ObjectInstance obj, int mask, ContentType systype, String usrtype, ObjTrxTree oxt);
    Object openChild();
    void close(Object infV, ObjTrxTree oxt);
    int create(ObjectInstance obj, int mask, ContentType systype, String usrtype, ObjTrxTree oxt);
    int delete(ObjectInstance obj, ObjTrxTree oxt);
    int deleteObj(Object infV, ObjTrxTree oxt);
    Object openQuery();
    int queryDelete();
    Object queryFetch();
    Object queryCreate();
    int queryClose();
    // named readData instead of read due to clash with JNA Structure class
    int readData(Object infV, String buffer, int maxcnt, int flags, ObjTrxTree oxt);
    // named readData instead of read due to clash with JNA Structure class
    int writeData();
    int getAttrType();
    int getAttrValue();
    String getFirstAttr();
    String getNextAttr();
    int setAttrValue();
    int addAttr();
    Object openAttr();
    String getFirstMethod();
    String getNextMethod();
    int executeMethod();
    ObjPresentationHints presentationHints();
    int info();
    int commit();
    int getQueryCoverageMask();
    int getQueryIdentityPath();
}
