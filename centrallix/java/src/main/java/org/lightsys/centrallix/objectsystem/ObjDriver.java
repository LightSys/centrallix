package org.lightsys.centrallix.objectsystem;

import java.util.List;

public interface ObjDriver {

    String getName(); // size 64
    List<String> getRootContentTypes();
    int getCapabilities();

    Object open(ObjectInstance obj, int mask, ContentType systype, String usrtype, ObjTrxTree oxt);
    Object openChild(ObjectInstance obj, String name, int mode, int permissionMask, String type);
    void close(ObjectContext obj, ObjTrxTree oxt);
    void create(ObjectInstance obj, int mask, ContentType systype, String usrtype, ObjTrxTree oxt);

    /**
     * Delete an existing object.
     *
     * @param obj
     * @param oxt
     */
    void delete(ObjectContext obj, ObjTrxTree oxt);

    /**
     * Delete an object that is already open.
     *
     * @param obj
     * @param oxt
     */
    void deleteObj(ObjectContext obj, ObjTrxTree oxt);

    ObjQuery openQuery(ObjectContext obj, ObjQuery query, ObjTrxTree oxt);
    void queryDelete(ObjectContext obj, ObjQuery query, ObjTrxTree oxt);
    ObjectContext queryFetch(QueryContext qy, ObjectInstance obj, int mode, ObjTrxTree oxt);
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
