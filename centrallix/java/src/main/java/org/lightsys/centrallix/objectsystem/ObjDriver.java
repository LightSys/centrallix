package org.lightsys.centrallix.objectsystem;

import java.util.List;

public interface ObjDriver {

    String getName(); // size 64
    List<String> getRootContentTypes();
    int getCapabilities();

    ObjectInstance open(ObjectInstance obj, int mask, ContentType systype, String usrtype, ObjTrxTree oxt);
    Object openChild();
    void close(ObjectInstance obj, ObjTrxTree oxt);
    void create(ObjectInstance obj, int mask, ContentType systype, String usrtype, ObjTrxTree oxt);
    void delete(ObjectInstance obj, ObjTrxTree oxt);
    void deleteObj(Object infV, ObjTrxTree oxt);
    Object openQuery();
    void queryDelete();
    Object queryFetch();
    Object queryCreate();
    void queryClose();
    // named readData instead of read due to clash with JNA Structure class
    void readData(Object infV, String buffer, int maxcnt, int flags, ObjTrxTree oxt);
    // named readData instead of read due to clash with JNA Structure class
    void writeData();
    int getAttrType();
    int getAttrValue();
    String getFirstAttr();
    String getNextAttr();
    int setAttrValue();
    int addAttr();
    Object openAttr();
    String getFirstMethod();
    String getNextMethod();
    void executeMethod();
    ObjPresentationHints getPresentationHints();
    int info(); // TODO what does this return value represent?
    void commit();
    int getQueryCoverageMask();
    int getQueryIdentityPath();
}
