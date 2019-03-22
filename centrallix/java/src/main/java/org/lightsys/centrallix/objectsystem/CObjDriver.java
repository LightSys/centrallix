package org.lightsys.centrallix.objectsystem;

import com.sun.jna.Callback;
import com.sun.jna.Function;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;
import org.lightsys.centrallix.CentrallixException;

import java.util.List;

/**
 * Corresponds to obj.h / struct *pObjDriver
 */
public class CObjDriver extends Structure implements ObjDriver {

    // native struct fields

    public String Name;
    public CXArray RootContentTypes;
    public int Capabilities;
    public Function Open;
    public Function OpenChild;
    public Function Close;
    public Function Create;
    public Function Delete;
    public Function DeleteObj;
    public Function OpenQuery;
    public Function QueryDelete;
    public Function QueryFetch;
    public Function QueryCreate;
    public Function QueryClose;
    public Function Read;
    public Function Write;
    public Function GetAttrType;
    public Function GetAttrValue;
    public Function GetFirstAttr;
    public Function GetNextAttr;
    public Function SetAttrValue;
    public Function AddAttr;
    public Function OpenAttr;
    public Function GetFirstMethod;
    public Function GetNextMethod;
    public Function ExecuteMethod;
    public Function PresentationHints;
    public Function Info;
    public Function Commit;
    public Function GetQueryCoverageMask;
    public Function GetQueryIdentityPath;

    // Java interface methods

    @Override
    public String getName() {
        return Name;
    }

    @Override
    public List<String> getRootContentTypes() {
        return RootContentTypes.asList();
    }

    @Override
    public int getCapabilities() {
        return Capabilities;
    }

    @Override
    public ObjectInstance open(ObjectInstance obj, int mask, ContentType systype, String usrtype, ObjTrxTree oxt) {
        Pointer p = (Pointer) Open.invoke(Pointer.class, new Object[]{
                CObjectInstance.fromJava(obj),
                mask, systype,
                CContentType.fromJava(usrtype),
                oxt});
        if(p == Pointer.NULL){
            throw new CentrallixException();
        }
        return new CObjectInstance(p);
    }

    @Override
    public Object openChild() {
        return null;
    }

    @Override
    public void close(ObjectInstance obj, ObjTrxTree oxt) {
        // TODO I think inf_v (void*) is a pointer to an Object struct (ObjectInstance), but not 100% sure
        Pointer p = (Pointer) Open.invoke(Pointer.class, new Object[]{
                CObjectInstance.fromJava(obj),
                CObjTrxTree.fromJava(oxt),
                oxt});
        if(p == Pointer.NULL){
            throw new CentrallixException();
        }
    }

    @Override
    public int create(ObjectInstance obj, int mask, ContentType systype, String usrtype, ObjTrxTree oxt) {
        return 0;
    }

    @Override
    public int delete(ObjectInstance obj, ObjTrxTree oxt) {
        return 0;
    }

    @Override
    public int deleteObj(Object infV, ObjTrxTree oxt) {
        return 0;
    }

    @Override
    public Object openQuery() {
        return null;
    }

    @Override
    public int queryDelete() {
        return 0;
    }

    @Override
    public Object queryFetch() {
        return null;
    }

    @Override
    public Object queryCreate() {
        return null;
    }

    @Override
    public int queryClose() {
        return 0;
    }

    @Override
    public int readData(Object infV, String buffer, int maxcnt, int flags, ObjTrxTree oxt) {
        return 0;
    }

    @Override
    public int writeData() {
        return 0;
    }

    @Override
    public int getAttrType() {
        return 0;
    }

    @Override
    public int getAttrValue() {
        return 0;
    }

    @Override
    public String getFirstAttr() {
        return null;
    }

    @Override
    public String getNextAttr() {
        return null;
    }

    @Override
    public int setAttrValue() {
        return 0;
    }

    @Override
    public int addAttr() {
        return 0;
    }

    @Override
    public Object openAttr() {
        return null;
    }

    @Override
    public String getFirstMethod() {
        return null;
    }

    @Override
    public String getNextMethod() {
        return null;
    }

    @Override
    public int executeMethod() {
        return 0;
    }

    @Override
    public ObjPresentationHints presentationHints() {
        return null;
    }

    @Override
    public int info() {
        return 0;
    }

    @Override
    public int commit() {
        return 0;
    }

    @Override
    public int getQueryCoverageMask() {
        return 0;
    }

    @Override
    public int getQueryIdentityPath() {
        return 0;
    }

    public interface OpenFunc extends Callback {
        int invoke(CObjectInstance obj, int mask, CContentType systype, String usrtype, CObjTrxTree oxt);
    }

    public interface CloseFunc extends Callback {
        int invoke(Pointer inf_v, CObjTrxTree oxt);
    }
}
